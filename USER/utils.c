#include "utils.h"

//
// 等待采样完成
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
// 处理数据
// IN: adcx 输入adc数值缓存
// OUT: adcmax 最大值 adcmin 最小值 fftin傅里叶变换数组
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
        *adcmax=*adcmax*0.8;   //0.8 ≈ 3300/4096	
		*adcmin=*adcmin*0.8;
}

//
// 计算各次谐波幅值
// IN: fftin 输入的原序列  pre 定时器2预分频值
// OUT:fftout 输出的变换序列 frequency 最大的频率成分
// 先将lBufOutArray分解成实部(X)和虚部(Y)，然后计算幅值(sqrt(X*X+Y*Y)
//
void GetPowerMag(IN int long fftin [NPT],IN u16 pre,OUT int long fftout [NPT],OUT u16* frequency)
{
    float X,Y,Mag,magmax;//实部，虚部，各频率幅值，最大幅值
    u16 i,temp;
	
	//调用自cr4_fft_1024_stm32
	cr4_fft_1024_stm32(fftout, fftin, NPT);	
	
    for(i=1; i<NPT/2; i++)
    {
		X = (fftout[i] << 16) >> 16;
		Y = (fftout[i] >> 16);
		
		Mag = sqrt(X * X + Y * Y); 
		//FFT_Mag[i]=Mag;//存入缓存，用于输出查验
		//获取最大频率分量及其幅值
		if(Mag > magmax)
		{
			magmax = Mag;
			temp = i; //幅值最大的频率成分
		}
    }
	*frequency=(u16)(temp/(pre/36.0));		
}

//
// 将fft的结果对数处理
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
// 处理串口消息
// pre 
//
void UsartMessageProcessor(IN OUT u16* pre,IN OUT int* uint_voltage )
{
		DMA_Cmd(DMA1_Channel1,ENABLE);//使能DMA1-CH1
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
// 检测长按还是短按,控制蜂鸣器给出对应提示
// 弃用 函数指针不会写 
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


