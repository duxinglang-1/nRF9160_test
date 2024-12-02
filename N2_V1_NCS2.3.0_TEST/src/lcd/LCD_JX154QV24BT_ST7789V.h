/****************************************Copyright (c)************************************************
** File Name:			    LCD_JX154QV24BT_ST7789V.h
** Descriptions:			LCD JX154QV24BT ST7789V head file
** Created By:				xie biao
** Created Date:			2024-11-29
** Modified Date:      		2024-11-29 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr\types.h>
#include "lcd.h"

#ifdef LCD_JX154QV24BT_ST7789V

//#include "boards.h"

//接口定义
//extern xdata unsigned char buffer[512];
//------------------------------------------------------
#define COL 240			//宽
#define ROW 240			//高

#define LCD_TYPE_SPI			//SPI

//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

//LCM
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi3), okay)
#define LCD_DEV DT_NODELABEL(spi3)
#else
#error "spi3 devicetree node is disabled"
#define LCD_DEV	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define LCD_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define LCD_PORT	""
#endif

#define CS		22
#define	RST		21
#define	RS		24

#define LCD_DATA_LEN 4096

//------------------------------------------------------

#ifdef LCD_PIXEL_DEPTH_24
extern uint8_t lcd_data_buffer[3*LCD_DATA_LEN];
#elif defined(LCD_PIXEL_DEPTH_16)
extern uint8_t lcd_data_buffer[2*LCD_DATA_LEN];
#endif

extern void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h);
extern void WriteOneDot(unsigned int color);
extern void Write_Data(uint8_t i);
extern void LCD_Init(void);
extern void LCD_Clear(uint32_t color);
extern void LCD_SleepIn(void);
extern void LCD_SleepOut(void);
extern void LCD_ResetBL_Timer(void);
extern void LCD_Set_BL_Mode(LCD_BL_MODE mode);
extern LCD_BL_MODE LCD_Get_BL_Mode(void);
#endif/*LCD_JX154QV24BT_ST7789V*/
