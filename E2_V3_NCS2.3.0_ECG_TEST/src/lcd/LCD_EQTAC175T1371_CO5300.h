/****************************************Copyright (c)************************************************
** File Name:			    LCD_EQTAC175T1371_CO5300.c
** Descriptions:			The LCD_EQTAC175T1371_CO5300 screen drive source file
** Created By:				xie biao
** Created Date:			2025-05-22
** Modified Date:      		2025-05-22
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr\types.h>
#include "lcd.h"

#ifdef LCD_EQTAC175T1371_CO5300

//接口定义
//extern xdata unsigned char buffer[512];
//------------------------------------------------------
#define COL 390			//宽
#define ROW 450			//高

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

#define CS		10
#define	RST		11
#define	RS		2
#define	EN		12
#define	LEDA	13

#define LCD_DATA_LEN 4096

//------------------------------------------------------

extern uint8_t lcd_data_buffer[2*LCD_DATA_LEN];

extern void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h);
extern void WriteOneDot(unsigned int color);
extern void Write_Data(uint8_t i);
extern void LCD_Init(void);
extern void LCD_Clear(uint16_t color);
extern void LCD_SleepIn(void);
extern void LCD_SleepOut(void);
extern void LCD_ResetBL_Timer(void);
extern void LCD_Set_BL_Mode(LCD_BL_MODE mode);
extern LCD_BL_MODE LCD_Get_BL_Mode(void);
#endif/*LCD_AM175T1216_CO5300*/
