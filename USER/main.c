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

#include "const.h"

void lcd_huadian(u16 a,u16 b,u16 color);//画点函数
void lcd_huaxian(u16 x1,u16 y1,u16 x2,u16 y2);//画线函数
void window(void);//主界面
//void clear_point(u16 mode);//更新显示屏当前列
void InitBufInArray(void);//正弦波输出缓存
void sinout(void);//正弦波输出
void GetPowerMag(void);//FFT变换，输出频率

int long fftin [NPT];//FFT输入
int long fftout[NPT];//FFT输出
u32 FFT_Mag[NPT/2]={0};//幅频特性
u16 magout[NPT];//模拟正弦波输出缓存区

//u16 table[15] ={16,32,48,64,80,96,112,128,144,160,176,192,208,224,240};//标点横坐标
u32 currentadc;//实时采样数据
u32 adcx[NPT];//adc数值缓存
u32 adcmax;//采样最大值和最小值
u32 adcmin;
u8 adc_flag=0;//采样结束标志
u8 key_flag=0;//按键扫描标志
u8 show_flag=1;//更新暂停标志
u16 T=2000;//定时器2重载值
u16 pre=36;//定时器2预分频值
u32 fre;//采样频率 kHz
u16 F;//波形频率
uint16_t AD0, AD1;

int uint_voltage = 660; //纵坐标单位刻度 mv/div
// int V=660;
u16 temp=0;//幅值最大的频率成分
u16 t=0;
u16 key;//按键值

void init(void){
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	MYDMA1_Config(DMA1_Channel1,(u32)&ADC1->DR,(u32)&currentadc,1);
	uart_init(9600);
	delay_init();
	LED_Init();
	EXTIX_Init();
	LCD_Init();
	Adc_Init();
	BEEP_Init();
	InitBufInArray();
	TIM3_Int_Init(39,71);	//72MHz/40/72=25kHz   25kHz/1024≈25Hz 正弦波频率约为24.5Hz
	TIM2_PWM_Init(T-1,pre-1);	//最大频率72000000/1/2000=3.6KHz
	Dac1_Init();
	LCD_Clear(BLACK);
	//	AD0 = AD_GetValue(ADC_Channel_8);
	//	AD1 = AD_GetValue(ADC_Channel_9);
}

int main()
{
	u16 i;
	init();
	DrawUI();

	while(1)
	{
		//等待采样完成
		while(adc_flag==0)
		{
			LED1=!LED1;
			delay_ms(100);
		}
		adc_flag=0;
		
		//获取最大最小值
		adcmax=adcx[1];
		adcmin=adcx[1];
		for(i=0;i<NPT;i++)
		{
			fftin[i] = 0;
			fftin[i] = adcx[i] << 16;
			
			if(adcx[i] >= adcmax)
			{			
				adcmax = adcx[i];
			}			
			if(adcx[i] <= adcmin)
			{			
				adcmin = adcx[i];
			}						
		}
		
		POINT_COLOR=BLUE;
		GetPowerMag();
		
		adcmax=adcmax*0.8;   //0.8 ≈ 3300/4096	
		adcmin=adcmin*0.8;
		
		LCD_ShowNum(270,25,adcmax,4,16);	//显示最大值
		LCD_ShowNum(270,65,adcmin,4,16);	//显示最小值
		LCD_ShowNum(270,105,adcmax-adcmin,4,16);	//显示幅值
//		LCD_ShowNum(360,105, AD0,4,16);
//	    LCD_ShowNum(360,148 ,AD1,4,16);
	
		if(show_flag==1)
		{
			UpdateInformation(pre,uint_voltage);
			UpdateWindow(1,adcx);
			//clear_point(1);    //更新显示屏当前列，采用连线绘制
		}	
		
		LED0=!LED0;
		DMA_Cmd(DMA1_Channel1,ENABLE);//使能DMA1-CH1
		delay_ms(100);
		if (Serial_RxFlag == 1)
		   {
			if (strcmp(Serial_RxPacket, "F+") == 0)
			{pre=pre-5;
				if(pre>72)
		     {pre=1;}
		      }
			if (strcmp(Serial_RxPacket, "F-") == 0)
			{
			   pre=pre+5;
		       if(pre>72)
		    {
			   pre=1;
			
		    }			
			}
			
			if (strcmp(Serial_RxPacket, "V+") == 0)
			{uint_voltage=uint_voltage+10;
			if(uint_voltage>1000){uint_voltage=660;}
			}
			if (strcmp(Serial_RxPacket, "V-") == 0)
			{uint_voltage=uint_voltage-10;
			if(uint_voltage<0){uint_voltage=660;}
			}
		Serial_RxFlag = 0;
		}
	}
}

void InitBufInArray(void)        
{
    u16 i;
    float fx;
    for(i=0; i<NPT; i++)
    {
        fx = sin((PI2*i)/NPT);
        magout[i] = (u16)(2048+2048*fx);
    }
}

/******************************************************************
函数名称:sinout()
函数功能:正弦波输出
参数说明:
备    注:将此函数置于定时器中断中，可模拟输出正弦波
*******************************************************************/
void sinout(void)
{
	static u16 i=0;
	DAC_SetChannel1Data(DAC_Align_12b_R,magout[i]);
	i++;
	if(i>=NPT)
		i=0;
}



/******************************************************************
函数名称:GetPowerMag()
函数功能:计算各次谐波幅值
参数说明:
备　　注:先将lBufOutArray分解成实部(X)和虚部(Y)，然后计算幅值(sqrt(X*X+Y*Y)
*******************************************************************/
void GetPowerMag(void)
{
    float X,Y,Mag,magmax;//实部，虚部，各频率幅值，最大幅值
    u16 i;
	
	//调用自cr4_fft_1024_stm32
	cr4_fft_1024_stm32(fftout, fftin, NPT);	
	
    for(i=1; i<NPT/2; i++)
    {
		X = (fftout[i] << 16) >> 16;
		Y = (fftout[i] >> 16);
		
		Mag = sqrt(X * X + Y * Y); 
		FFT_Mag[i]=Mag;//存入缓存，用于输出查验
		//获取最大频率分量及其幅值
		if(Mag > magmax)
		{
			magmax = Mag;
			temp = i;
		}
    }
	F=(u16)(temp/(pre/36.0));
//	if(T==1000)		F=(u32)((double)temp/NPT*1000  );	
//	if(T==100)		F=(u32)((double)temp/NPT*10010 );
//	if(T==10)		F=(u32)((double)temp/NPT*100200);
//	if(T==2)		F=(u32)((double)temp/NPT*249760);
	
	LCD_ShowNum(260,180,F,5,16);		
//		LCD_ShowNum(280,200,temp,4,16);					
//		LCD_ShowNum(280,220,(u32)(magmax*2.95),5,16);			
}

/******************************************************************
简介：DMA中断用于完整采样一次（采样1024次），
	  并将其存储于adcx[]缓存数组中，等待后续数据处理
*******************************************************************/	
void DMA1_Channel1_IRQHandler(void) 
{
	if(DMA_GetITStatus(DMA1_IT_TC1)!=RESET)
	{
		adcx[t]=currentadc;
		t++;
		if(t==NPT)
		{
			t=0;
			adc_flag=1;
			DMA_Cmd(DMA1_Channel1, DISABLE);        //使能DMA
		}
	}
	DMA_ClearITPendingBit(DMA1_IT_TC1);
}

/******************************************************************
简介：定时器3中断服务函数，用于正弦波输出
	  每进入一次中断改变一次DCA输出值
*******************************************************************/
void TIM3_IRQHandler(void)   //TIM3中断
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{
			sinout();
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx的中断待处理位:TIM 中断源 
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
	delay_ms(10);//消抖
	if(WK_UP==1)	 	 //WK_UP按键
	{	
		BEEP=1;
		delay_ms(50);
		BEEP=0;
		show_flag=!show_flag;
		
		POINT_COLOR=MAGENTA;
		if(show_flag)
			LCD_ShowString(260,128,200,16,16,"ing...");
		else
			LCD_ShowString(260,128,200,16,16,"stop");
		Serial_Printf("\r\nmv/div%d", uint_voltage);
		Serial_Printf("\r\nmax(mv)%d", adcmax);
		Serial_Printf("\r\nmin(mv)%d", adcmin);
		Serial_Printf("\r\nvpp(mv)%d", adcmax-adcmin);
		Serial_Printf("\r\nf(Hz)%d", F);
		Serial_Printf("\r\nOSR(Hz)%d", fre);
	}
	EXTI_ClearITPendingBit(EXTI_Line0); //清除LINE0上的中断标志位  
}
 
void EXTI3_IRQHandler(void)
{
	delay_ms(10);//消抖
	if(KEY1==0)	 //按键KEY1
	{	
		BEEP=1;
		delay_ms(50);
		BEEP=0;
		pre=pre+5;
		if(pre>72)
		{
			pre=1;
		}
		TIM_PrescalerConfig(TIM2,pre-1,TIM_PSCReloadMode_Immediate);
	}		 
	EXTI_ClearITPendingBit(EXTI_Line3);  //清除LINE3上的中断标志位  
}

void EXTI4_IRQHandler(void)
{
	delay_ms(10);//消抖
	if(KEY0==0)	 //按键KEY0
	{
		BEEP=1;
		delay_ms(50);
		BEEP=0;
		pre=pre-5;
		if(pre<=1)
		{
			pre=1;
		}
		TIM_PrescalerConfig(TIM2,pre-1,TIM_PSCReloadMode_Immediate);
	}		 
	EXTI_ClearITPendingBit(EXTI_Line4);  //清除LINE4上的中断标志位  
}
