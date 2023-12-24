#include "ui.h"

u16 TABLE[15] ={16,32,48,64,80,96,112,128,144,160,176,192,208,224,240};//标点横坐标

void DrawUI(void)
{
	u16 x,i;
	static u16 h; 
	
	POINT_COLOR=GREEN;	  
	LCD_ShowString(5,8,200,24,24,"UPON");
	
	POINT_COLOR=GRAY;
	LCD_ShowString(190,13,200,16,16,"mV/div");	
	LCD_ShowString(260,5,200,16,16,"max(mv):");
	LCD_ShowString(260,45,200,16,16,"min(mv):");
	LCD_ShowString(260,85,200,16,16,"vpp(mv):");
	LCD_ShowString(260,165,200,16,16,"f(Hz):");
	LCD_ShowString(260,200,200,16,16,"OSR:");  //采样率	
	LCD_ShowString(304,220,200,16,16,"Hz");
	LCD_ShowString(350,5,200,16,16,"f(Hz):");
	LCD_ShowString(350,45,200,16,16,"DUTY(%):");
    //LCD_ShowString(350,85,200,16,16,"AD0:");
    //LCD_ShowString(350,128,200,16,16,"AD1:");	
	
	POINT_COLOR=BRRED;
	LCD_ShowString(60,13,200,16,16,"IN:PA6,PB13");
	 
	POINT_COLOR=BLUE;
	//LCD_ShowNum(150,13,V,4,16);//mv/div
	
	POINT_COLOR=WHITE;			
	draw_line(0,0,0,200);
	draw_line(256,0,256,200);
	draw_line(0,0,256,0);		
	draw_line(0,200,256,200);
	
	for(x=0;x<256;x++)
	{
		draw_point(x,100,WHITE);
		if(x == TABLE[h])	
		{
			draw_line(x,1,x,3);
			draw_line(x,101,x,103);
			draw_line(x,199,x,197);
			h++;
			if(h>=16) h=0;
		}	
		if(x==128) 
		{
			draw_line(x,1,x,199);
			for(i=10;i<200;i+=10)
			{
				draw_line(125,i,127,i);
			}
		}
	}
	
	POINT_COLOR=MAGENTA;	
	LCD_ShowString(260,128,200,16,16,"ing...");
}

void draw_point(u16 a,u16 b,u16 color)
{							    
	LCD_Fast_DrawPoint(a,240-b,color);
}

void draw_line(u16 x1,u16 y1,u16 x2,u16 y2)
{
	LCD_DrawLine(x1,240-y1,x2,240-y2);
}