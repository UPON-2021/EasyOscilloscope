#ifndef UI_H
#define UI_H

#include "sys.h"
#include "lcd.h"
#include "const.h"

void DrawUI(void);
void draw_point(u16 a,u16 b,u16 color);
void draw_line(u16 x1,u16 y1,u16 x2,u16 y2);
void UpdateWindow(u16 mode,int uint_voltage,u32 data[1024]);
void UpdateInformation(IN u16 pre,IN int uint_voltage, IN u32 adcmax, IN u32 adcmin, IN u16 frequency, IN u16 isDebug);
#endif 

