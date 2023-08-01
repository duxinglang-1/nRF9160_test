#include <zephyr\types.h>
#include "lcd.h"

#ifdef LCD_ORCZ010903C_GC9A01

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
#define	RST		24
#define	RS		21
#define	SCL		28
#define	SDA		29
#define VDD		20

//LEDK(LED背光)
#define LEDK	31
#define LEDA	14

#define X_min 0x0043		 //TP测试范围常量定义
#define X_max 0x07AE
#define Y_min 0x00A1
#define Y_max 0x0759

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
#endif/*LCD_LH096TIG11G_ST7735SV*/
