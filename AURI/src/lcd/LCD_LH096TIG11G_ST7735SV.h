#include "lcd.h"

#ifdef LCD_LH096TIG11G_ST7735SV

//#include "boards.h"

//接口定义
//extern xdata unsigned char buffer[512];
//------------------------------------------------------
#define COL 80			//宽
#define ROW 160			//高


//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

//LCM
#define LCD_PORT	"GPIO_0"
#define LCD_DEV 	"SPI_3"

#define CS		30
#define	RST		16
#define	RS		17
#define	SCL		18
#define	SDA		19

//TP 
#define TP_0			12
#define TP_1			12

//LEDK(LED背光)
#define LEDK	31

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
