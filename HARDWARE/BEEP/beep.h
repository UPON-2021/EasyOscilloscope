#ifndef __BEEP_H
#define __BEEP_H	 
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK��ӢSTM32������
//��������������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/2
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 
//�������˿ڶ���
#define BEEP PBout(8)	// BEEP,�������ӿ�		   

void BEEP_Init(void);	//��ʼ��
void BEEP_Short(void);	// beep 50����
void BEEP_Long(void);	// beep 800����
void BEEP_Three(void);	// beep 30ms ����

#endif

