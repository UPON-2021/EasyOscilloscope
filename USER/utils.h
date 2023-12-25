#ifndef UTILS_DEFINED_H
#define UTILS_DEFINED_H
#include "sys.h"
#include "led.h"
#include "time.h"
#include "const.h"
#include "usart.h"
#include "beep.h"
#include "math.h"
#include "stm32f10x.h"

void WaitUntilSampingFinished(IN u8* flag); 
void CollectDataProcessor(IN u32 adcx[NPT],OUT u32* adcmax,OUT u32* adcmin,OUT int long fftin[NPT]);
void GetPowerMag(IN int long fftin [NPT],IN u16 pre,OUT int long fftout [NPT],OUT u16* frequency);
void UsartMessageProcessor(IN OUT u16* pre,IN OUT int* uint_voltage);

#endif