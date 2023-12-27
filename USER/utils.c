#include "utils.h"

//
// 等待采样完成
//
void WaitUntilSampingFinished(u8 *flag)
{
    while (*flag == 0) {
        LED1 = 1;
        delay_ms(100);
    }
    LED1  = 0;
    *flag = 0;
}

//
// 处理数据
// IN: adcx 输入adc数值缓存
// OUT: adcmax 最大值 adcmin 最小值 fftin傅里叶变换数组
//
void CollectDataProcessor(IN u32 adcx[NPT], OUT u32 *adcmax, OUT u32 *adcmin, OUT int long fftin[NPT], OUT u8 *duty)
{
    u16 i;
    u16 temp;
    *adcmax = adcx[1];
    *adcmin = adcx[1];
    for (i = 0; i < NPT; i++) {
        fftin[i] = 0;
        fftin[i] = adcx[i] << 16;
        if (adcx[i] > 0) {
            temp += 1;
        }
        if (adcx[i] >= *adcmax) {
            *adcmax = adcx[i];
        }
        if (adcx[i] <= *adcmin) {
            *adcmin = adcx[i];
        }
    }
    *duty   = (u8)(temp*100 / NPT);
    *adcmax = *adcmax * 0.8; // 0.8 ≈ 3300/4096
    *adcmin = *adcmin * 0.8;
}

//
// 计算各次谐波幅值
// IN: fftin 输入的原序列  pre 定时器2预分频值
// OUT:fftout 输出的变换序列 frequency 最大的频率成分
// 先将lBufOutArray分解成实部(X)和虚部(Y)，然后计算幅值(sqrt(X*X+Y*Y)
//
void GetPowerMag(IN int long fftin[NPT], IN u16 pre, OUT int long fftout[NPT], OUT u16 *frequency)
{
    float X, Y, Mag, magmax; // 实部，虚部，各频率幅值，最大幅值
    u16 i, temp;

    // 调用自cr4_fft_1024_stm32
    cr4_fft_1024_stm32(fftout, fftin, NPT);

    for (i = 1; i < NPT / 2; i++) {
        X = (fftout[i] << 16) >> 16;
        Y = (fftout[i] >> 16);

        Mag = sqrt(X * X + Y * Y);
        // FFT_Mag[i]=Mag;//存入缓存，用于输出查验
        // 获取最大频率分量及其幅值
        if (Mag > magmax) {
            magmax = Mag;
            temp   = i; // 幅值最大的频率成分
        }
    }
    *frequency = (u16)(temp / (pre / 36.0));
}

//
// 将fft的结果对数处理
//
void fft2shift(IN int long fftout[NPT], OUT long fftshift[NPT])
{
    u16 i;
    for (i = 0; i < NPT; i++) {
        fftshift[i] = (long)(log10(abs(fftout[i])) / LOG2 * 200);
    }
}

//
// 处理串口消息
// pre
//
void UsartMessageProcessor(IN OUT u16 *pre, IN OUT int *uint_voltage)
{
    DMA_Cmd(DMA1_Channel1, ENABLE); // 使能DMA1-CH1
    delay_ms(100);
    if (Serial_RxFlag == 1) {
        if (strcmp(Serial_RxPacket, "F+") == 0) {
            *pre = *pre - 5;
            if (*pre > 72) { *pre = 1; }
        }
        if (strcmp(Serial_RxPacket, "F-") == 0) {
            *pre = *pre + 5;
            if (*pre > 72) {
                *pre = 1;
            }
        }

        if (strcmp(Serial_RxPacket, "V+") == 0) {
            *uint_voltage = *uint_voltage + 10;
            if (*uint_voltage > 1000) { *uint_voltage = 660; }
        }
        if (strcmp(Serial_RxPacket, "V-") == 0) {
            *uint_voltage = *uint_voltage - 10;
            if (*uint_voltage < 0) { *uint_voltage = 660; }
        }
        Serial_RxFlag = 0;
    }
}

//
// 向串口发送当前状态信息
//
void SendUsartStatusMessage(IN u16 pre, IN u16 frequency, IN u32 adcmax, IN u32 adcmin, IN u16 uint_voltage, IN u16 fre)
{
    Serial_Printf("\r\n============STATUS===============");
    // Serial_Printf("\r\nmv/div%d", uint_voltage);
    // Serial_Printf("\r\nmax(mv)%d", adcmax);
    // Serial_Printf("\r\nmin(mv)%d", adcmin);
    // Serial_Printf("\r\nvpp(mv)%d", adcmax - adcmin);
    // Serial_Printf("\r\nf(Hz)%d", frequency);
    // Serial_Printf("\r\nOSR(Hz)%d", fre);

    Serial_Printf("\r\nuint_voltage  %d mv/div", uint_voltage);
    Serial_Printf("\r\nadcmax        %d mv", adcmax);
    Serial_Printf("\r\nadcmin        %d mv", adcmin);
    Serial_Printf("\r\nvpp %d mv", adcmax - adcmin);
    Serial_Printf("\r\nfrequency     %d Hz", frequency);
    Serial_Printf("\r\nOSR           %d Hz", fre);
    Serial_Printf("\r\n============END==================");
}

//
// 检测长按还是短按,控制蜂鸣器给出对应提示
// 弃用 函数指针不会写
//
// void CheckKeyPress(IN u8 (*key)(GPIO_TypeDef *, uint16_t), OUT u8 *flag)
// {
//     *flag    = 0;
//     u32 temp = 0;

// 	//调用 输入的函数指针

//     while ( == 0) {
//         temp += 1;
//         delay_ms(1);
//         if (temp >= 3000) {
//             *flag = 1;
//             BEEP_Long();
//             return;
//         }
//     }
//     if (*flag == 0) {
//         BEEP_Short();
//     }
//     return;
// }

// void SendDebugInfo(IN u16 pre, IN u16 frequency, IN u32 adcmax, IN u32 adcmin, IN u16 uint_voltage)
// {
// 	u16 fre;
// 	fre = 36000 / pre; // 更新采样频率
// 	Serial_Printf("\r\n============DEBUG=========");
// 	Serial_Printf("\r\npre:%d", pre);
// 	Serial_Printf("\r\nfre:%d", fre);
// 	Serial_Printf("\r\nfrequency:%d", frequency);
// 	Serial_Printf("\r\nadcmax:%d", adcmax);
// 	Serial_Printf("\r\nadcmin:%d", adcmin);
// 	Serial_Printf("\r\nuint_voltage:%d", uint_voltage);
// 	Serial_Printf("\r\n=============END==========");
// }

void SendDebugInfo(IN u16 pre, IN u16 frequency, IN u32 adcmax, IN u32 adcmin, IN u16 uint_voltage, IN int long fftin[NPT], int long fftout[NPT], int long fftshift[NPT], u32 adcx[NPT], u32 magount[NPT])
{
    u16 fre;
    fre = 36000 / pre; // 更新采样频率
    Serial_Printf("\r\n============DEBUG=========");
    Serial_Printf("\r\npre:%d", pre);
    Serial_Printf("\r\nfre:%d", fre);
    Serial_Printf("\r\nfrequency:%d", frequency);
    Serial_Printf("\r\nadcmax:%d", adcmax);
    Serial_Printf("\r\nadcmin:%d", adcmin);
    Serial_Printf("\r\nuint_voltage:%d", uint_voltage);
    Serial_Printf("\r\n=============END==========");
    Serial_Printf("\r\n============FFTIN=========");
    for (int i = 0; i < NPT; i++) {
        Serial_Printf("\r\nfftin[%d]:%d", i, fftin[i]);
    }
    Serial_Printf("\r\n============FFTOUT=========");
    for (int i = 0; i < NPT; i++) {
        Serial_Printf("\r\nfftout[%d]:%d", i, fftout[i]);
    }
    Serial_Printf("\r\n============FFTSHIFT=========");
    for (int i = 0; i < NPT; i++) {
        Serial_Printf("\r\nfftshift[%d]:%d", i, fftshift[i]);
    }
    Serial_Printf("\r\n============ADCX=========");
    for (int i = 0; i < NPT; i++) {
        Serial_Printf("\r\nadcx[%d]:%d", i, adcx[i]);
    }
    Serial_Printf("\r\n=============END==========");
    Serial_Printf("\r\n============MAGOUT=========");
    for (int i = 0; i < NPT; i++) {
        Serial_Printf("\r\nmagout[%d]:%d", i, magount[i]);
    }
    Serial_Printf("\r\n=============END==========");
}