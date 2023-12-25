#include "utils.h"

//
// �ȴ��������
//
void WaitUntilSampingFinished(u8* flag)
{
    while(*flag==0)
    {
        LED1=1;
        delay_ms(100);
    }
    LED1=0;
    *flag=0;
}

//
// ��������
// IN: adcx ����adc��ֵ����
// OUT: adcmax ���ֵ adcmin ��Сֵ fftin����Ҷ�任����
//
void CollectDataProcessor(IN u32 adcx[NPT],OUT u32* adcmax,OUT u32* adcmin,OUT int long fftin[NPT]){
    	u16 i;
        *adcmax=adcx[1];
		*adcmin=adcx[1];
		for(i=0;i<NPT;i++)
		{
			fftin[i] = 0;
			fftin[i] = adcx[i] << 16;
			
			if(adcx[i] >= *adcmax)
			{			
				*adcmax = adcx[i];
			}			
			if(adcx[i] <= *adcmin)
			{			
				*adcmin = adcx[i];
			}						
		}
        *adcmax=*adcmax*0.8;   //0.8 �� 3300/4096	
		*adcmin=*adcmin*0.8;
}

//
// �������г����ֵ
// IN: fftin �����ԭ����  pre ��ʱ��2Ԥ��Ƶֵ
// OUT:fftout ����ı任���� frequency ����Ƶ�ʳɷ�
// �Ƚ�lBufOutArray�ֽ��ʵ��(X)���鲿(Y)��Ȼ������ֵ(sqrt(X*X+Y*Y)
//
void GetPowerMag(IN int long fftin [NPT],IN u16 pre,OUT int long fftout [NPT],OUT u16* frequency)
{
    float X,Y,Mag,magmax;//ʵ�����鲿����Ƶ�ʷ�ֵ������ֵ
    u16 i,temp;
	
	//������cr4_fft_1024_stm32
	cr4_fft_1024_stm32(fftout, fftin, NPT);	
	
    for(i=1; i<NPT/2; i++)
    {
		X = (fftout[i] << 16) >> 16;
		Y = (fftout[i] >> 16);
		
		Mag = sqrt(X * X + Y * Y); 
		//FFT_Mag[i]=Mag;//���뻺�棬�����������
		//��ȡ���Ƶ�ʷ��������ֵ
		if(Mag > magmax)
		{
			magmax = Mag;
			temp = i; //��ֵ����Ƶ�ʳɷ�
		}
    }
	*frequency=(u16)(temp/(pre/36.0));		
}

//
// ��fft�Ľ����������
//
// void fft2shift(IN int long fftout[NPT],OUT double fftshift[NPT])
// {
// 	u16 i;
// 	for (i=0;i<NPT;i++)
// 	{
// 		fftshift[i] = log10
// 	}
// }

//
// ��������Ϣ
// pre 
//
void UsartMessageProcessor(IN OUT u16* pre,IN OUT int* uint_voltage )
{
		DMA_Cmd(DMA1_Channel1,ENABLE);//ʹ��DMA1-CH1
		delay_ms(100);
		if (Serial_RxFlag == 1)
		{
			if (strcmp(Serial_RxPacket, "F+") == 0)
			{*pre=*pre-5;
				if(*pre>72)
		     {*pre=1;}
		      }
			if (strcmp(Serial_RxPacket, "F-") == 0)
			{
			   *pre=*pre+5;
		       if(*pre>72)
		    {
			   *pre=1;
			
		    }			
			}
			
			if (strcmp(Serial_RxPacket, "V+") == 0)
			{*uint_voltage=*uint_voltage+10;
			if(*uint_voltage>1000){*uint_voltage=660;}
			}
			if (strcmp(Serial_RxPacket, "V-") == 0)
			{*uint_voltage=*uint_voltage-10;
			if(*uint_voltage<0){*uint_voltage=660;}
			}
			Serial_RxFlag = 0;
		}
}


//
// ��ⳤ�����Ƕ̰�,���Ʒ�����������Ӧ��ʾ
// ���� ����ָ�벻��д 
//
// void CheckKeyPress(IN u8 (*key)(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin),OUT u8* flag)
// {
// 	*flag = 0;
// 	u32 temp = 0;
// 	while (key == 0){
// 		temp += 1;
// 		delay_ms(1);
// 		if (temp >= 3000)
// 		{
// 			*flag = 1;
// 			BEEP_Long();
// 			return ;
// 		}

// 	}
// 	if (*flag == 0)
// 		{
// 			BEEP_Short();
// 		}
// 	return ;
// }


