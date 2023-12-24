#ifndef UI
#define UI

#include "sys.h"
#include "lcd.h"
#include "const.h"

void DrawUI(void);
void draw_point(u16 a,u16 b,u16 color);
void draw_line(u16 x1,u16 y1,u16 x2,u16 y2);
void UpdateWindow(u16 mode,u32 data[1024]);
void UpdateInformation(u16 pre,int uint_voltage);
#endif 
