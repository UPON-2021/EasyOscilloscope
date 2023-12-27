/**********************************************************
简易示波器
ADC(TIME触发)->DMA
PA4通过DAC通道1输出正弦波，频率由TIME3分频值控制
KEY_WAKE控制更新与暂停截屏上传到pc端
KEY0增加采样频率
KEY1降低采样频率
定时器3产生中断DAC通道1输出正弦波

***********************************************************/
#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "lcd.h"
#include "adc.h"
#include "dma.h"
#include "timer.h"
#include "table_fft.h"
#include "stm32_dsp.h"
#include "math.h"
#include "key.h"
#include "BEEP.h"
#include "dac.h"
#include "exti.h"
#include <stdio.h>
#include <stdarg.h>
#include "IC.h"
#include "ui.h"
#include "utils.h"
#include "debug.h"
#include "const.h"

void InitBufInArray(u16); // 正弦波输出缓存

int long fftin[NPT];    // FFT输入
int long fftout[NPT];   // FFT输出
int long fftshift[NPT]; // 对数处理后的FFT

// u32 FFT_Mag[NPT/2]={0};//幅频特性
u32 magout[NPT]; // 模拟正弦波输出缓存区

u8 isdisplayfft    = 0; // 是否展示FFT后的结果
u8 isSendDebuginfo = 0; // 是否发送调试信息
u8 isDebug         = 0; // 是否调试
u8 debugStatus     = 0; // 调试状态

u32 currentadc;      // 实时采样数据
u32 adcx[NPT];       // adc数值缓存
u32 adcxbuff[NPT];   // 第二个缓存
u32 adcmax;          // 采样最大值
u32 adcmin;          // 采样最小值
u8 adc_flag  = 0;    // 采样结束标志
u8 key_flag  = 0;    // 按键扫描标志
u8 show_flag = 1;    // 更新暂停标志
u16 T        = 2000; // 定时器2重载值
u16 pre      = 36;   // 定时器2预分频值
u32 fre;             // 采样频率 kHz
u8 duty = 0;        // 占空比
u32 frequency;       // 波形频率
uint16_t AD0, AD1;

int uint_voltage = 660; // 纵坐标单位刻度 mv/div

u16 t = 0;
u16 key; // 按键值

void init(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    MYDMA1_Config(DMA1_Channel1, (u32)&ADC1->DR, (u32)&currentadc, 1);
    uart_init(115200);
    delay_init();
    LED_Init();
    EXTIX_Init();
    LCD_Init();
    Adc_Init();
    BEEP_Init();
    InitBufInArray(16);
    TIM3_Int_Init(39, 71);         // 72MHz/40/72=25kHz   25kHz/1024≈25Hz 正弦波频率约为24.5Hz
    TIM2_PWM_Init(T - 1, pre - 1); // 最大频率72000000/1/2000=3.6KHz
    Dac1_Init();
    LCD_Clear(BLACK);
    //	AD0 = AD_GetValue(ADC_Channel_8);
    //	AD1 = AD_GetValue(ADC_Channel_9);
}

int main()
{
    u16 freq_test = 0;
    init();
    DrawUI();
    Greeting();

    while (1) {
        WaitUntilSampingFinished(IN OUT & adc_flag);
        // DEBUG CODE
        // InitBufInArray(freq_test);
        // for (u32 i = 0; i <= NPT; i += 1) {
        //     adcx[i] = magout[i];
        // }
        if (isDebug > 0) {
            DebugDataProcessor(IN OUT & isDebug, OUT & isdisplayfft, IN OUT & isSendDebuginfo, OUT adcx, IN OUT & debugStatus, OUT magout);
        }

        CollectDataProcessor(IN adcx, OUT & adcmax, OUT & adcmin, OUT fftin,OUT &duty);
        // GetPowerMag(IN fftin, IN pre, OUT fftout, OUT & frequency);
        GetPowerMag(IN adcx, IN pre, OUT fftout, OUT & frequency);

        if (show_flag == 1) {
            UpdateInformation(IN pre, IN uint_voltage, IN adcmax, IN adcmin, IN frequency, IN isDebug,IN duty);
            if (isdisplayfft == 1) {
                fft2shift(IN fftout, OUT fftshift);
                UpdateWindow(IN DRAWFFT, IN uint_voltage, IN fftshift);
            } else {
                UpdateWindow(IN DRAWLINE, IN uint_voltage, IN adcx);
            }
        }

        UsartMessageProcessor(IN OUT & pre, IN OUT & uint_voltage);
        if (isSendDebuginfo == 1) {
            SendDebugInfo(IN pre, IN uint_voltage, IN adcmax, IN adcmin, IN frequency, IN fftin, IN fftout, IN fftshift, IN adcx, IN magout);
            isSendDebuginfo = 0;
        }
        // freq_test += 1;
    }
}

void InitBufInArray(u16 frequency)
{
    u32 i;
    float fx;
    for (i = 0; i < NPT; i++) {
        fx        = sin((PI2 * i) / (NPT / frequency));
        magout[i] = (u32)(2048 + 2048 * fx);
    }
}

void sinout(void)
{
	static u16 i=0;
	DAC_SetChannel1Data(DAC_Align_12b_R,magout[i]);
	i++;
	if(i>=NPT)
		i=0;
}

/******************************************************************
简介：DMA中断用于完整采样一次（采样1024次），
      并将其存储于adcx[]缓存数组中，等待后续数据处理
*******************************************************************/
void DMA1_Channel1_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC1) != RESET) {
        adcx[t] = currentadc;
        t++;
        if (t == NPT) {
            t        = 0;
            adc_flag = 1;
            DMA_Cmd(DMA1_Channel1, DISABLE); // 使能DMA
        }
    }
    DMA_ClearITPendingBit(DMA1_IT_TC1);
}

/******************************************************************
简介：定时器3中断服务函数，用于正弦波输出
      每进入一次中断改变一次DCA输出值
*******************************************************************/
void TIM3_IRQHandler(void) // TIM3中断
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) // 检查指定的TIM中断发生与否:TIM 中断源
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // 清除TIMx的中断待处理位:TIM 中断源
    }
}

/******************************************************************
简介：三个外部中断用于按键的读取
WK_UP按键控制波形显示的更新和暂停
KEY1按键降低采样频率
KEY0按键增加采样频率
*******************************************************************/
void EXTI0_IRQHandler(void)
{
    u32 temp = 0;
    u8 flag  = 0; // 检测长按还是短按
    delay_ms(10); // 消抖

    while (WK_UP == 1) {
        delay_ms(1);
        if (temp >= 3000) {
            delay_ms(1);
            flag = 1;
            isDebug++;
            BEEP_Three();
        }
        temp += 1;
    }
    if (flag == 0) {
        BEEP_Short();
        show_flag   = !show_flag;
        POINT_COLOR = MAGENTA;
        if (show_flag) {
            LCD_ShowString(260, 128, 200, 16, 16, "ing...");
        } else {
            LCD_ShowString(260, 128, 200, 16, 16, "stop");
            SendUsartStatusMessage(IN pre, IN frequency, IN adcmax, IN adcmin, IN uint_voltage, IN fre);
        }
    }
    EXTI_ClearITPendingBit(EXTI_Line0); // 清除LINE0上的中断标志位
}

void EXTI3_IRQHandler(void)
{
    u32 temp = 0;
    u8 flag  = 0; // 检测长按还是短按
    delay_ms(10); // 消抖
    while (KEY1 == 0) {
        delay_ms(1);
        if (temp >= 3000) {
            flag         = 1;
            isdisplayfft = !isdisplayfft;
            BEEP_Long();
        }
        temp += 1;
    }
    if (flag == 0) {
        BEEP_Short();
        pre = pre + 1;
        if (pre > 80) { pre = 100; }
        TIM_PrescalerConfig(TIM2,pre,TIM_PSCReloadMode_Immediate);
        // uint_voltage = uint_voltage + 10;
        // if (uint_voltage > 1000) { uint_voltage = 660; }
    }

    EXTI_ClearITPendingBit(EXTI_Line3); // 清除LINE3上的中断标志位
}

void EXTI4_IRQHandler(void)
{
    u32 temp = 0;
    u8 flag  = 0; // 检测长按还是短按
    delay_ms(10); // 消抖
    while (KEY0 == 0) {
        delay_ms(1);
        if (temp >= 3000) {
            flag            = 1;
            isSendDebuginfo = 1;
            BEEP_Long();
        }
        temp += 1;
    }
    if (flag == 0) {
        BEEP_Short();
        pre = pre - 1;
        if (pre < 1) { pre = 1; }
        TIM_PrescalerConfig(TIM2,pre,TIM_PSCReloadMode_Immediate);
        // uint_voltage = uint_voltage - 10;
        // if (uint_voltage < 0) { uint_voltage = 660; }
    }

    EXTI_ClearITPendingBit(EXTI_Line4); // 清除LINE4上的中断标志位
}
