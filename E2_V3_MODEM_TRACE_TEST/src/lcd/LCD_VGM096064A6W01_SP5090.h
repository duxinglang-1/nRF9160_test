/****************************************Copyright (c)************************************************
** File Name:			    LCD_VGM068A4W01_SH1106G.h
** Descriptions:			The VGM068A4W01_SH1106G screen drive head file
** Created By:				xie biao
** Created Date:			2021-01-07
** Modified Date:      		2021-01-07
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr\types.h>
#include "lcd.h"

#ifdef LCD_VGM096064A6W01_SP5090

//#include "boards.h"

//接口定义
//extern xdata unsigned char buffer[512];
//------------------------------------------------------
#define COL 		96			//宽
#define ROW 		64			//高
#define PAGE_H		8			//每个page像素点高为8
#define PAGE_MAX	8			//总共8个page

#define LCD_TYPE_SPI			//SPI

//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

//LCM
#define LCD_PORT	"GPIO_0"
#define LCD_DEV 	"SPI_3"

#define CS		23
#define	RST		24
#define	RS		21
#define	SCL		22
#define	SDA		20
#define VDD		18

//LEDK(LED背光)
#define LEDK	31
#define LEDA	14

#define X_min 0x0043		 //TP测试范围常量定义
#define X_max 0x07AE
#define Y_min 0x00A1
#define Y_max 0x0759

#define LCD_DATA_LEN ((COL*ROW)/8)

//------------------------------------------------------

extern u8_t lcd_data_buffer[2*LCD_DATA_LEN];

extern void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h);
extern void WriteOneDot(u16_t color);
extern void Write_Data(u8_t i);
extern void LCD_Init(void);
extern void LCD_Clear(u16_t color);
extern void LCD_SleepIn(void);
extern void LCD_SleepOut(void);
extern void LCD_ResetBL_Timer(void);
extern void LCD_Set_BL_Mode(LCD_BL_MODE mode);
extern LCD_BL_MODE LCD_Get_BL_Mode(void);
#endif/*LCD_VGM096064A6W01_SP5090*/
