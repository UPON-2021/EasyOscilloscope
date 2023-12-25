/**********************************************************
����ʾ����
ADC(TIME����)->DMA
PA4ͨ��DACͨ��1������Ҳ���Ƶ����TIME3��Ƶֵ����
KEY_WAKE���Ƹ�������ͣ�����ϴ���pc��
KEY0���Ӳ���Ƶ��
KEY1���Ͳ���Ƶ��
��ʱ��3�����ж�DACͨ��1������Ҳ�

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
#include "const.h"

void InitBufInArray(u16); // ���Ҳ��������
void sinout(void);        // ���Ҳ����

int long fftin[NPT];    // FFT����
int long fftout[NPT];   // FFT���
int long fftshift[NPT]; // ����������FFT

// u32 FFT_Mag[NPT/2]={0};//��Ƶ����
u32 magout[NPT]; // ģ�����Ҳ����������

u8 isdisplayfft    = 0; // �Ƿ�չʾFFT��Ľ��
u8 isSendDebuginfo = 0; // �Ƿ��͵�����Ϣ

u32 currentadc;      // ʵʱ��������
u32 adcx[NPT];       // adc��ֵ����
u32 adcmax;          // �������ֵ
u32 adcmin;          // ������Сֵ
u8 adc_flag  = 0;    // ����������־
u8 key_flag  = 0;    // ����ɨ���־
u8 show_flag = 1;    // ������ͣ��־
u16 T        = 2000; // ��ʱ��2����ֵ
u16 pre      = 36;   // ��ʱ��2Ԥ��Ƶֵ
u32 fre;             // ����Ƶ�� kHz
u16 frequency;       // ����Ƶ��
uint16_t AD0, AD1;

int uint_voltage = 660; // �����굥λ�̶� mv/div

u16 t = 0;
u16 key; // ����ֵ

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
    // InitBufInArray();
    TIM3_Int_Init(39, 71);         // 72MHz/40/72=25kHz   25kHz/1024��25Hz ���Ҳ�Ƶ��ԼΪ24.5Hz
    TIM2_PWM_Init(T - 1, pre - 1); // ���Ƶ��72000000/1/2000=3.6KHz
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

    while (1) {
        WaitUntilSampingFinished(IN OUT & adc_flag);
        // DEBUG CODE
        InitBufInArray(freq_test);
        for (u32 i = 0; i <= NPT; i += 1) {
            adcx[i] = magout[i];
        }

        CollectDataProcessor(IN adcx, OUT & adcmax, OUT & adcmin, OUT fftin);
        GetPowerMag(IN fftin, IN pre, OUT fftout, OUT & frequency);

        if (show_flag == 1) {
            UpdateInformation(IN pre, IN uint_voltage, IN adcmax, IN adcmin, IN frequency);
            if (isdisplayfft == 1) {
                fft2shift(IN fftout, OUT fftshift);
                UpdateWindow(IN DRAWFFT, IN uint_voltage, IN fftshift);
            } else {
                UpdateWindow(IN DRAWLINE, IN uint_voltage, IN adcx);
            }
        }

        UsartMessageProcessor(IN OUT & pre, IN OUT & uint_voltage);
        if (isSendDebuginfo == 1) {
            SendDebugInfo(IN pre, IN uint_voltage, IN adcmax, IN adcmin, IN frequency, IN fftin, IN fftout, IN fftshift, IN adcx);
            isSendDebuginfo = 0;
        }
        freq_test += 1;
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

/******************************************************************
��������:sinout()
��������:���Ҳ����
����˵��:
��    ע:���˺������ڶ�ʱ���ж��У���ģ��������Ҳ�
*******************************************************************/
void sinout(void)
{
    static u16 i = 0;
    DAC_SetChannel1Data(DAC_Align_12b_R, magout[i]);
    i++;
    if (i >= NPT)
        i = 0;
}

/******************************************************************
��飺DMA�ж�������������һ�Σ�����1024�Σ���
      ������洢��adcx[]���������У��ȴ��������ݴ���
*******************************************************************/
void DMA1_Channel1_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC1) != RESET) {
        adcx[t] = currentadc;
        t++;
        if (t == NPT) {
            t        = 0;
            adc_flag = 1;
            DMA_Cmd(DMA1_Channel1, DISABLE); // ʹ��DMA
        }
    }
    DMA_ClearITPendingBit(DMA1_IT_TC1);
}

/******************************************************************
��飺��ʱ��3�жϷ��������������Ҳ����
      ÿ����һ���жϸı�һ��DCA���ֵ
*******************************************************************/
void TIM3_IRQHandler(void) // TIM3�ж�
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) // ���ָ����TIM�жϷ������:TIM �ж�Դ
    {
        sinout();
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // ���TIMx���жϴ�����λ:TIM �ж�Դ
    }
}

/******************************************************************
��飺�����ⲿ�ж����ڰ����Ķ�ȡ
WK_UP�������Ʋ�����ʾ�ĸ��º���ͣ
KEY1�������Ͳ���Ƶ��
KEY0�������Ӳ���Ƶ��
*******************************************************************/
void EXTI0_IRQHandler(void)
{
    delay_ms(10);   // ����
    if (WK_UP == 1) // WK_UP����
    {
        BEEP = 1;
        delay_ms(50);
        BEEP      = 0;
        show_flag = !show_flag;

        POINT_COLOR = MAGENTA;
        if (show_flag)
            LCD_ShowString(260, 128, 200, 16, 16, "ing...");
        else
            LCD_ShowString(260, 128, 200, 16, 16, "stop");
        Serial_Printf("\r\nmv/div%d", uint_voltage);
        Serial_Printf("\r\nmax(mv)%d", adcmax);
        Serial_Printf("\r\nmin(mv)%d", adcmin);
        Serial_Printf("\r\nvpp(mv)%d", adcmax - adcmin);
        Serial_Printf("\r\nf(Hz)%d", frequency);
        Serial_Printf("\r\nOSR(Hz)%d", fre);
    }
    EXTI_ClearITPendingBit(EXTI_Line0); // ���LINE0�ϵ��жϱ�־λ
}

void EXTI3_IRQHandler(void)
{
    u32 temp = 0;
    u8 flag  = 0; // ��ⳤ�����Ƕ̰�
    delay_ms(10); // ����
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

        uint_voltage = uint_voltage + 10;
        if (uint_voltage > 1000) { uint_voltage = 660; }
        // pre  = pre + 5;
        // if (pre > 72) {
        //     pre = 1;
        // }
    }

    EXTI_ClearITPendingBit(EXTI_Line3); // ���LINE3�ϵ��жϱ�־λ
}

void EXTI4_IRQHandler(void)
{
    // delay_ms(10);  // ����
    // if (KEY0 == 0) // ����KEY0
    // {
    //     BEEP = 1;
    //     delay_ms(50);
    //     BEEP = 0;
    //     pre  = pre - 5;
    //     if (pre <= 1) {
    //         pre = 1;
    //     }
    //     TIM_PrescalerConfig(TIM2, pre - 1, TIM_PSCReloadMode_Immediate);
    // }

    u32 temp = 0;
    u8 flag  = 0; // ��ⳤ�����Ƕ̰�
    delay_ms(10); // ����
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
        uint_voltage = uint_voltage - 10;
        if (uint_voltage < 0) { uint_voltage = 660; }
        // pre  = pre - 5;
        // if (pre <= 1) {
        //     pre = 1;
        // }
    }

    EXTI_ClearITPendingBit(EXTI_Line4); // ���LINE4�ϵ��жϱ�־λ
}
