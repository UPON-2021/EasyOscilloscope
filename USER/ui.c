#include "ui.h"

u16 TABLE[15] = {16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240};

//
// UI绘制
//
void DrawUI(void)
{
    u16 x, i;
    static u16 h;

    POINT_COLOR = GREEN;
    LCD_ShowString(5, 8, 200, 24, 24, "MADE BY UPON");

    POINT_COLOR = GRAY;
    LCD_ShowString(190, 13, 200, 16, 16, "mV/div");
    LCD_ShowString(280, 5, 200, 16, 16, "max(mv):");
    LCD_ShowString(280, 45, 200, 16, 16, "min(mv):");
    LCD_ShowString(280, 85, 200, 16, 16, "vpp(mv):");
    LCD_ShowString(324, 220, 200, 16, 16, "Hz");
    // LCD_ShowString(370,5,200,16,16," f(Hz):");
    LCD_ShowString(370, 45, 200, 16, 16, " DUTY(%):");

    POINT_COLOR = RED;
    LCD_ShowString(260, 165, 200, 16, 16, "Samping f(Hz):");
    LCD_ShowString(260, 200, 200, 16, 16, "Samping OSR:"); // 采样率
    // LCD_ShowString(350,85,200,16,16,"AD0:");
    // LCD_ShowString(350,128,200,16,16,"AD1:");

    POINT_COLOR = BRRED;
    LCD_ShowString(5, 250, 200, 16, 16, "INPUT -> PA6");

    POINT_COLOR = BLUE;
    // LCD_ShowNum(150,13,V,4,16);//mv/div

    POINT_COLOR = WHITE;
    draw_line(0, 0, 0, 200);
    draw_line(256, 0, 256, 200);
    draw_line(0, 0, 256, 0);
    draw_line(0, 200, 256, 200);

    for (x = 0; x < 256; x++) {
        draw_point(x, 100, WHITE);
        if (x == TABLE[h]) {
            draw_line(x, 1, x, 3);
            draw_line(x, 101, x, 103);
            draw_line(x, 199, x, 197);
            h++;
            if (h >= 16) h = 0;
        }
        if (x == 128) {
            draw_line(x, 1, x, 199);
            for (i = 10; i < 200; i += 10) {
                draw_line(125, i, 127, i);
            }
        }
    }

    POINT_COLOR = MAGENTA;
    LCD_ShowString(260, 128, 200, 16, 16, "ing...");
}

//
// 示波器窗口更新
// mode=1时绘制 data的波形
//
void UpdateWindow(IN u16 mode, IN int uint_voltage, IN u32 data[1024])
{
    u16 x, i, past_vol, pre_vol;
    static u16 h;

    for (x = 0; x < 256; x++) {
        POINT_COLOR = BLACK; // 按列清除
        if (x != 128)        // 去除y轴列清除
            draw_line(x, 4, x, 196);

        // 绘制坐标
        POINT_COLOR = WHITE;
        draw_line(0, 0, 0, 200);
        draw_point(x, 100, WHITE);
        if (x == TABLE[h]) {
            draw_line(x, 1, x, 3);
            draw_line(x, 101, x, 103);
            draw_line(x, 199, x, 197);
            h++;
            if (h >= 16) h = 0;
        }
        if (x == 128) {
            draw_line(x, 1, x, 199);
            for (i = 10; i < 200; i += 10) {
                draw_line(125, i, 127, i);
            }
        }

        pre_vol = 60 + data[x] / 32768.0 * uint_voltage;

        // 波形更新
        if (mode == DRAWLINE) {
            POINT_COLOR = YELLOW;
            if (x > 0 && x < 255 && x != 128) // 去除第一个，最后一个以及y轴上点的绘制
                draw_line(x, past_vol, x + 1, pre_vol);
        }
        if (mode == DRAWFFT) {
            POINT_COLOR = YELLOW;
            if (x > 0 && x < 255 && x != 128) // 去除第一个，最后一个以及y轴上点的绘制
                draw_line(x, past_vol - 40, x + 1, pre_vol - 40);
        }

        past_vol = pre_vol;
    }
}

void UpdateInformation(IN u16 pre, IN int uint_voltage, IN u32 adcmax, IN u32 adcmin, IN u16 frequency, IN u16 isDebug,IN u8 duty)
{
    u16 fre;
    POINT_COLOR = BLUE;
    fre         = 36000 / pre;               // 更新采样频率
    LCD_ShowNum(281, 220, fre, 5, 16);       // 更新采样率显示
    LCD_ShowNum(281, 180, frequency, 5, 16); // 显示频率

    LCD_ShowNum(150, 13, uint_voltage, 4, 16);

    LCD_ShowNum(290, 25, adcmax, 4, 16);           // 显示最大值
    LCD_ShowNum(290, 65, adcmin, 4, 16);           // 显示最小值
    LCD_ShowNum(290, 105, adcmax - adcmin, 4, 16); // 显示幅值

    LCD_ShowNum(380,65, duty,4,16);

    // LCD_ShowString()
    if (isDebug >= 1) {
        POINT_COLOR = CYAN;
        switch (isDebug) {
            case 1:
                LCD_ShowString(5, 280, 200, 16, 16, "Debug Mode Sin ");
                break;
            case 2:
                LCD_ShowString(5, 280, 200, 16, 16, "Debug Mode Squre ");
                break;
            case 3:
                // LCD_ShowString(5, 280, 200, 16, 16, "If you are seiing ths, may be something wrong... ");
                break;
            default:
                break;
        }
        // LCD_ShowString(5, 280, 200, 16, 16, "Debug Mode   ");
    } else {
        POINT_COLOR = BLUE;
        LCD_ShowString(5, 280, 200, 16, 16, "Normal Mode            ");
    }

    // LCD_ShowNum(360,25, IC_GetFreq(),4,16);
    
}

void draw_point(u16 a, u16 b, u16 color)
{
    LCD_Fast_DrawPoint(a, 240 - b, color);
}

void draw_line(u16 x1, u16 y1, u16 x2, u16 y2)
{
    LCD_DrawLine(x1, 240 - y1, x2, 240 - y2);
}