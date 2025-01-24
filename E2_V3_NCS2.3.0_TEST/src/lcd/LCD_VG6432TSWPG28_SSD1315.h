/****************************************Copyright (c)************************************************
** File Name:			    LCD_VG6432TSWPG28_SSD1315.h
** Descriptions:			The LCD_VG6432TSWPG28_SSD1315 screen drive head file
** Created By:				xie biao
** Created Date:			2025-01-16
** Modified Date:      		2025-01-16
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr\types.h>
#include "lcd.h"

#ifdef LCD_VG6432TSWPG28_SSD1315

//------------------------------------------------------
#define COL 		64			//宽
#define ROW 		32			//高
#define PAGE_H		8			//每个page像素点高为8
#define PAGE_MAX	4			//总共4个page

#define LCD_TYPE_SPI			//SPI

//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

#define LCD_I2C_ADDR	(0x78 >> 1)
#define	LCD_RST		13

//LEDK(LED背光)
#define LEDK	31
#define LEDA	14

#define LCD_X_OFFSET	(0x20)

#define LCD_DATA_LEN ((COL*ROW)/8)

typedef enum
{
	SCROLL_H_RIGHT 			= 0x26,
	SCROLL_H_LEFT 			= 0x27,
	SCROLL_V_AND_H_RIGHT 	= 0x29,
	SCROLL_V_AND_H_LEFT		= 0x2A,
	SCROLL_CONTENT_RIGHT	= 0x2C,
	SCROLL_CONTENT_LEFT		= 0x2D,
	SCROLL_MAX				= 0xFF,
}LCD_SCROLLING_DERECTION;

typedef enum
{
	FADE_DISABLE			= 0b00,
	FADA_OUT				= 0b10,
	FADE_BLINKING  			= 0b11,
}LCD_FADE_OUT_MODE;
//------------------------------------------------------

extern uint8_t lcd_data_buffer[2*LCD_DATA_LEN];

extern void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h);
extern void WriteOneDot(uint16_t color);
extern void Write_Data(uint8_t i);
extern void LCD_Init(void);
extern void LCD_Clear(uint16_t color);
extern void LCD_SleepIn(void);
extern void LCD_SleepOut(void);
extern void LCD_ResetBL_Timer(void);
extern void LCD_Set_BL_Mode(LCD_BL_MODE mode);
extern LCD_BL_MODE LCD_Get_BL_Mode(void);
#endif/*LCD_R108101_GC9307*/
