#include "lcd.h"

#ifdef LCD_R154101_ST7796S

#include "boards.h"

//接口定义
//extern xdata unsigned char buffer[512];
//------------------------------------------------------
#define ROW 320			//显示的行、
#define COL 320			//列数

//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

//LCM
#define	CS0 			NRF_GPIO_PIN_MAP(1,13)	
#define	RST				NRF_GPIO_PIN_MAP(1,8)
#define	RS				NRF_GPIO_PIN_MAP(1,14)
#define	WR0				NRF_GPIO_PIN_MAP(1,15)
#define	RD0				NRF_GPIO_PIN_MAP(0,02)

//DB0~7
#define DB0				NRF_GPIO_PIN_MAP(1,06)
#define DB1				NRF_GPIO_PIN_MAP(1,07)
#define DB2				NRF_GPIO_PIN_MAP(1,05)
#define DB3				NRF_GPIO_PIN_MAP(0,27)
#define DB4				NRF_GPIO_PIN_MAP(1,04)
#define DB5				NRF_GPIO_PIN_MAP(1,03)
#define DB6				NRF_GPIO_PIN_MAP(0,26)
#define DB7				NRF_GPIO_PIN_MAP(1,02)

#define DBS_LIST { DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7 }

//TP 
#define TP_CS			NRF_GPIO_PIN_MAP(1,12)

//LEDK(LED背光)
#define LEDK_1          NRF_GPIO_PIN_MAP(1,10)
#define LEDK_2          NRF_GPIO_PIN_MAP(1,11)

#define X_min 0x0043		 //TP测试范围常量定义
#define X_max 0x07AE
#define Y_min 0x00A1
#define Y_max 0x0759
//------------------------------------------------------

extern void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h);
extern void WriteOneDot(unsigned int color);
extern void LCD_Init(void);
extern void Write_Data(uint8_t i);
extern void LCD_Clear(uint16_t color);

#endif/*LCD_R154101_ST7796S*/
