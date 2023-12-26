#include "debug.h"

void Greeting(void)
{
    // Serial_Printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
    Serial_Printf("\r\n==============================================================================\r\n");
    Serial_Printf(" _____                 ___           _ _                                \r\n");
    Serial_Printf("| ____|__ _ ___ _   _ / _ \\ ___  ___(_) | ___  ___  ___ ___  _ __   ___ \r\n");
    Serial_Printf("|  _| / _` / __| | | | | | / __|/ __| | |/ _ \\/ __|/ __/ _ \\| '_ \\ / _ \\ \r\n");
    Serial_Printf("| |__| (_| \\__ \\ |_| | |_| \\__ \\ (__| | | (_) \\__ \\ (_| (_) | |_) |  __/\r\n");
    Serial_Printf("|_____\\__,_|___/\\__, |\\___/|___/\\___|_|_|\\___/|___/\\___\\___/| .__/ \\___|\r\n");
    Serial_Printf("                |___/                                       |_|         \r\n");
    Serial_Printf("Made By UPON  v1.0\r\n");
    Serial_Printf("\r\n==============================================================================\r\n");
}

// 处理串口接收到的debug信息
void DebugDataProcessor(IN OUT u8 *isDebug, IN OUT u8 isdisplayfft, IN OUT u8 isSendDebuginfo, OUT u32 adcx[NPT], IN OUT u8 *debugStatus, IN OUT u32 magout[NPT])
{
    if (*debugStatus == 0) {
        send_menu();
        *debugStatus = 1;
    }
    // debug_message_processor();
    switch (*isDebug) {
        case 1:
            init_sin_buf_array(100, OUT magout);
            replace_buf_array(IN magout, OUT adcx);
            break;
        case 2:
            init_square_buf_array(50,7, OUT magout);
            replace_buf_array(IN magout, OUT adcx);
            break;
        case 3:
            *isDebug     = 0;
            *debugStatus = 0;
            break;
        default:
            *isDebug = 0;
            break;
    }
}

void debug_data_generator();

u8 debug_message_processor()
{
    u8 result = 0;
    if (Serial_RxFlag == 1) {
        Serial_Printf("[*] Get input: %s\r\n", Serial_RxPacket);
        if (strcmp(Serial_RxPacket, "1") == 0) {
            result = USER_INPUT_CHOOSE_SIN_WAVE;
        } else if (strcmp(Serial_RxPacket, "2") == 0) {
            result = USER_INPUT_CHOOSE_SQUARE_WAVE;
        } else if (strcmp(Serial_RxPacket, "3") == 0) {
            result = USER_INPUT_CHOOSE_EXIT_DEBUG;
        } else {
            result = USER_INPUT_INVALID;
            Serial_Printf("[-] Invalid input, please input again.\r\n");
        }
        Serial_RxFlag = 0;
    }
    return result;
}

void init_sin_buf_array(IN u16 frequency, OUT u32 magout[NPT])
{

    u32 i;
    float fx;
    for (i = 0; i < NPT; i++) {
        fx        = sin((PI2 * i) / (NPT / frequency));
        magout[i] = (u32)(2048 + 2048 * fx);
    }
}

void init_square_buf_array(IN u16 duty,IN u16 frequency, OUT u32 magout[NPT])
{
    u32 i;
    float fx;
    for (i = 0; i < NPT; i++) {
        fx = sin((PI2 * i) / (NPT / frequency));
        if (fx > 0) {
            magout[i] = 4096;
        } else {
            magout[i] = 0;
        }
    }
}

void replace_buf_array(IN u32 magout[NPT], OUT u32 adcx[NPT])
{
    for (u32 i = 0; i < NPT; i++) {
        adcx[i] = magout[i];
    }
}

void send_menu(void)
{
    Serial_Printf("[*] Debug mode on.\r\n");
    //     Serial_Printf("[*] Debug mode on, please input command:\r\n");
    //     Serial_Printf("[-] 1. Send sin wave.\r\n");
    //     Serial_Printf("[-] 2. Send square wave.\r\n");
    //     Serial_Printf("[-] 3. Close debug mode.\r\n");
}