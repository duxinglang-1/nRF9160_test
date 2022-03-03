/****************************************Copyright (c)************************************************
** File Name:			    Lcd.c
** Descriptions:			LCD head file
** Created By:				xie biao
** Created Date:			2020-07-13
** Modified Date:      		2020-12-18 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __LCD_H__
#define __LCD_H__

#include <stdbool.h>
#include <stdint.h>
#include "font.h"
//#include "boards.h"

//#define LCD_R108101_GC9307
#define LCD_ORCZ010903C_GC9A01
//#define LCD_R154101_ST7796S
//#define LCD_LH096TIG11G_ST7735SV
//#define LCD_ORCT012210N_ST7789V2
//#define LCD_VGM068A4W01_SH1106G
//#define LCD_VGM096064A6W01_SP5090

//#define LCD_TYPE_PARALLEL		//并口
//#define LCD_TYPE_I2C			//I2C
//#define LCD_TYPE_SPI			//SPI

//#define IMG_FONT_FROM_FLASH		//图片和字库存放在外部FLASH中

//#define LCD_BACKLIGHT_CONTROLED_BY_PMU	//由PMU控制屏幕背光

//LCD睡眠唤醒
extern bool lcd_sleep_in;
extern bool lcd_sleep_out;
extern bool lcd_is_sleeping;
extern bool sleep_out_by_wrist;

//LCD的宽度和高度
extern uint16_t  LCD_WIDTH;
extern uint16_t  LCD_HEIGHT;

//LCD的画笔颜色和背景色	   
extern uint16_t  POINT_COLOR;//默认红色    
extern uint16_t  BACK_COLOR; //背景颜色.默认为白色

//系统字体大小
extern SYSTEM_FONT_SIZE system_font;

//画笔颜色
#define WHITE         	 0xFFFF	//白色
#define BLACK         	 0x0000	//黑色
#define BLUE         	 0x001F	//蓝色
#define GBLUE			 0X07FF	//蓝绿色
#define RED           	 0xF800	//红色
#define MAGENTA       	 0xF81F	//玫瑰红
#define GREEN         	 0x07E0	//绿色
#define CYAN          	 0x7FFF	//青色
#define YELLOW        	 0xFFE0	//黄色
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
//GUI颜色

#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色 
 
#define LIGHTGREEN     	 0X841F //浅绿色
#define LIGHTGRAY        0XEF5B //浅灰色(PANNEL)
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

#define X_min 0x0043		 //TP测试范围常量定义
#define X_max 0x07AE
#define Y_min 0x00A1
#define Y_max 0x0759

typedef enum
{
	LCD_BL_ALWAYS_ON,
	LCD_BL_AUTO,
	LCD_BL_OFF,
	LCD_BL_MAX
}LCD_BL_MODE;

void LCD_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_Pic_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, unsigned char *color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r);
void LCD_ShowString(uint16_t x, uint16_t y, uint8_t *p);
void LCD_ShowStringInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *p);
#ifdef LCD_VGM068A4W01_SH1106G
void LCD_ShowStrInRect(u16_t x, u16_t y, u16_t width, u16_t height, u8_t *p);
#endif/*LCD_VGM068A4W01_SH1106G*/
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len);
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len,uint8_t mode);
void LCD_ShowImg(u16_t x, u16_t y, unsigned char *color);
void LCD_ShowImg_From_Flash(u16_t x, u16_t y, u32_t img_addr);
void LCD_SetFontSize(uint8_t font_size);
void LCD_MeasureString(uint8_t *p, uint16_t *width, uint16_t *height);
void LCD_get_pic_size(unsigned char *color, uint16_t *width, uint16_t *height);
void LCD_dis_trans_pic(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans);
void LCD_dis_pic_rotate(uint16_t x, uint16_t y, unsigned char *color, unsigned int rotate);
void LCD_dis_trans_pic_rotate(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans, unsigned int rotate);
#ifdef FONTMAKER_UNICODE_FONT
void LCD_MeasureUniString(uint16_t *p, uint16_t *width, uint16_t *height);
void LCD_ShowUniString(u16_t x, u16_t y, u16_t *p);
void LCD_ShowUniStringInRect(u16_t x, u16_t y, u16_t width, u16_t height, u16_t *p);
#endif/*FONTMAKER_UNICODE_FONT*/

#endif
