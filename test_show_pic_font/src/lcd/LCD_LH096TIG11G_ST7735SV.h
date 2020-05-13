#include "lcd.h"

#ifdef LCD_LH096TIG11G_ST7735SV

//#include "boards.h"

//接口定义
//extern xdata unsigned char buffer[512];
//------------------------------------------------------
#define ROW 160			//显示的行、
#define COL 80			//列数

//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

//LCM
#define	CS 				NRF_GPIO_PIN_MAP(0,4)	
#define	RST				NRF_GPIO_PIN_MAP(0,28)
#define	RS				NRF_GPIO_PIN_MAP(0,29)
#define	SCL				NRF_GPIO_PIN_MAP(0,30)
#define	SDA				NRF_GPIO_PIN_MAP(0,31)

//TP 
#define TP_0			NRF_GPIO_PIN_MAP(1,12)
#define TP_1			NRF_GPIO_PIN_MAP(1,12)

//LEDK(LED背光)
#define LEDK			NRF_GPIO_PIN_MAP(0,3)

#define X_min 0x0043		 //TP测试范围常量定义
#define X_max 0x07AE
#define Y_min 0x00A1
#define Y_max 0x0759
//------------------------------------------------------

extern void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h);
extern void WriteOneDot(unsigned int color);
extern void Write_Data(uint8_t i);
extern void LCD_Clear(uint16_t color);
extern void LCD_Init(void);
extern void LCD_SleepIn(void);
extern void LCD_SleepOut(void);

#endif/*LCD_LH096TIG11G_ST7735SV*/
