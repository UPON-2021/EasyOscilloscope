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

#include "const.h"

void lcd_huadian(u16 a,u16 b,u16 color);//���㺯��
void lcd_huaxian(u16 x1,u16 y1,u16 x2,u16 y2);//���ߺ���
void window(void);//������
//void clear_point(u16 mode);//������ʾ����ǰ��
void InitBufInArray(void);//���Ҳ��������
void sinout(void);//���Ҳ����
void GetPowerMag(void);//FFT�任�����Ƶ��

int long fftin [NPT];//FFT����
int long fftout[NPT];//FFT���
u32 FFT_Mag[NPT/2]={0};//��Ƶ����
u16 magout[NPT];//ģ�����Ҳ����������

//u16 table[15] ={16,32,48,64,80,96,112,128,144,160,176,192,208,224,240};//��������
u32 currentadc;//ʵʱ��������
u32 adcx[NPT];//adc��ֵ����
u32 adcmax;//�������ֵ����Сֵ
u32 adcmin;
u8 adc_flag=0;//����������־
u8 key_flag=0;//����ɨ���־
u8 show_flag=1;//������ͣ��־
u16 T=2000;//��ʱ��2����ֵ
u16 pre=36;//��ʱ��2Ԥ��Ƶֵ
u32 fre;//����Ƶ�� kHz
u16 F;//����Ƶ��
uint16_t AD0, AD1;

int uint_voltage = 660; //�����굥λ�̶� mv/div
// int V=660;
u16 temp=0;//��ֵ����Ƶ�ʳɷ�
u16 t=0;
u16 key;//����ֵ

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
	TIM3_Int_Init(39,71);	//72MHz/40/72=25kHz   25kHz/1024��25Hz ���Ҳ�Ƶ��ԼΪ24.5Hz
	TIM2_PWM_Init(T-1,pre-1);	//���Ƶ��72000000/1/2000=3.6KHz
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
		//�ȴ��������
		while(adc_flag==0)
		{
			LED1=!LED1;
			delay_ms(100);
		}
		adc_flag=0;
		
		//��ȡ�����Сֵ
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
		
		adcmax=adcmax*0.8;   //0.8 �� 3300/4096	
		adcmin=adcmin*0.8;
		
		LCD_ShowNum(270,25,adcmax,4,16);	//��ʾ���ֵ
		LCD_ShowNum(270,65,adcmin,4,16);	//��ʾ��Сֵ
		LCD_ShowNum(270,105,adcmax-adcmin,4,16);	//��ʾ��ֵ
//		LCD_ShowNum(360,105, AD0,4,16);
//	    LCD_ShowNum(360,148 ,AD1,4,16);
	
		if(show_flag==1)
		{
			UpdateInformation(pre,uint_voltage);
			UpdateWindow(1,adcx);
			//clear_point(1);    //������ʾ����ǰ�У��������߻���
		}	
		
		LED0=!LED0;
		DMA_Cmd(DMA1_Channel1,ENABLE);//ʹ��DMA1-CH1
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
��������:sinout()
��������:���Ҳ����
����˵��:
��    ע:���˺������ڶ�ʱ���ж��У���ģ��������Ҳ�
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
��������:GetPowerMag()
��������:�������г����ֵ
����˵��:
������ע:�Ƚ�lBufOutArray�ֽ��ʵ��(X)���鲿(Y)��Ȼ������ֵ(sqrt(X*X+Y*Y)
*******************************************************************/
void GetPowerMag(void)
{
    float X,Y,Mag,magmax;//ʵ�����鲿����Ƶ�ʷ�ֵ������ֵ
    u16 i;
	
	//������cr4_fft_1024_stm32
	cr4_fft_1024_stm32(fftout, fftin, NPT);	
	
    for(i=1; i<NPT/2; i++)
    {
		X = (fftout[i] << 16) >> 16;
		Y = (fftout[i] >> 16);
		
		Mag = sqrt(X * X + Y * Y); 
		FFT_Mag[i]=Mag;//���뻺�棬�����������
		//��ȡ���Ƶ�ʷ��������ֵ
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
��飺DMA�ж�������������һ�Σ�����1024�Σ���
	  ������洢��adcx[]���������У��ȴ��������ݴ���
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
			DMA_Cmd(DMA1_Channel1, DISABLE);        //ʹ��DMA
		}
	}
	DMA_ClearITPendingBit(DMA1_IT_TC1);
}

/******************************************************************
��飺��ʱ��3�жϷ��������������Ҳ����
	  ÿ����һ���жϸı�һ��DCA���ֵ
*******************************************************************/
void TIM3_IRQHandler(void)   //TIM3�ж�
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //���ָ����TIM�жϷ������:TIM �ж�Դ 
		{
			sinout();
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //���TIMx���жϴ�����λ:TIM �ж�Դ 
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
	delay_ms(10);//����
	if(WK_UP==1)	 	 //WK_UP����
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
	EXTI_ClearITPendingBit(EXTI_Line0); //���LINE0�ϵ��жϱ�־λ  
}
 
void EXTI3_IRQHandler(void)
{
	delay_ms(10);//����
	if(KEY1==0)	 //����KEY1
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
	EXTI_ClearITPendingBit(EXTI_Line3);  //���LINE3�ϵ��жϱ�־λ  
}

void EXTI4_IRQHandler(void)
{
	delay_ms(10);//����
	if(KEY0==0)	 //����KEY0
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
	EXTI_ClearITPendingBit(EXTI_Line4);  //���LINE4�ϵ��жϱ�־λ  
}
