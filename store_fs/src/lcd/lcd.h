#ifndef __LCD_H__
#define __LCD_H__

#include <stdint.h>
//#include "boards.h"

//#define LCD_R154101_ST7796S
//#define LCD_LH096TIG11G_ST7735SV
#define LCD_ORCT012210N_ST7789V2

//#define LCD_TYPE_PARALLEL		//并口
//#define LCD_TYPE_I2C			//I2C
#define LCD_TYPE_SPI			//SPI

//LCD的宽度和高度
extern uint16_t  LCD_WIDTH;
extern uint16_t  LCD_HEIGHT;

//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
//GUI颜色

#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色 
 
#define LIGHTGREEN     	 0X841F //浅绿色
//#define LIGHTGRAY        0XEF5B //浅灰色(PANNEL)
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

#define X_min 0x0043		 //TP测试范围常量定义
#define X_max 0x07AE
#define Y_min 0x00A1
#define Y_max 0x0759

void LCD_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_Pic_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, unsigned char *color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r);
void LCD_ShowString(uint16_t x, uint16_t y, uint8_t *p);
void LCD_ShowStringInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *p);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len);
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len,uint8_t mode);
void LCD_SetFontSize(uint8_t font_size);
void LCD_MeasureString(uint8_t *p, uint16_t *width, uint16_t *height);
void LCD_get_pic_size(unsigned char *color, uint16_t *width, uint16_t *height);
void LCD_dis_pic(uint16_t x,uint16_t y, unsigned char *color);
void LCD_dis_trans_pic(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans);
void LCD_dis_pic_rotate(uint16_t x, uint16_t y, unsigned char *color, unsigned int rotate);
void LCD_dis_trans_pic_rotate(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans, unsigned int rotate);

#endif
