#include <zephyr\types.h>
#include "lcd.h"

#ifdef LCD_ORCZ010903C_GC9A01

//�ӿڶ���
//extern xdata unsigned char buffer[512];
//------------------------------------------------------
#define COL 240			//��
#define ROW 240			//��

#define LCD_TYPE_SPI			//SPI

//LCD�Ļ�����ɫ�ͱ���ɫ	   
extern uint16_t  POINT_COLOR;//Ĭ�Ϻ�ɫ    
extern uint16_t  BACK_COLOR; //������ɫ.Ĭ��Ϊ��ɫ

//LCM
#define LCD_PORT	"GPIO_0"
#define LCD_DEV 	"SPI_3"

#define CS		23
#define	RST		24
#define	RS		21
#define	SCL		22
#define	SDA		20
#define VDD		18

//LEDK(LED����)
#define LEDK	31
#define LEDA	14

#define X_min 0x0043		 //TP���Է�Χ��������
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

#endif/*LCD_LH096TIG11G_ST7735SV*/
