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

//#define LCD_TYPE_PARALLEL		//����
//#define LCD_TYPE_I2C			//I2C
//#define LCD_TYPE_SPI			//SPI

#define IMG_FONT_FROM_FLASH		//ͼƬ���ֿ������ⲿFLASH��

#define LCD_BACKLIGHT_CONTROLED_BY_PMU	//��PMU������Ļ����

//LCD˯�߻���
extern bool lcd_sleep_in;
extern bool lcd_sleep_out;
extern bool lcd_is_sleeping;
extern bool sleep_out_by_wrist;

//LCD�Ŀ��Ⱥ͸߶�
extern uint16_t  LCD_WIDTH;
extern uint16_t  LCD_HEIGHT;

//LCD�Ļ�����ɫ�ͱ���ɫ	   
extern uint16_t  POINT_COLOR;//Ĭ�Ϻ�ɫ    
extern uint16_t  BACK_COLOR; //������ɫ.Ĭ��Ϊ��ɫ

//ϵͳ�����С
extern SYSTEM_FONT_SIZE system_font;

//������ɫ
#define WHITE         	 0xFFFF	//��ɫ
#define BLACK         	 0x0000	//��ɫ
#define BLUE         	 0x001F	//��ɫ
#define GBLUE			 0X07FF	//����ɫ
#define RED           	 0xF800	//��ɫ
#define MAGENTA       	 0xF81F	//õ���
#define GREEN         	 0x07E0	//��ɫ
#define CYAN          	 0x7FFF	//��ɫ
#define YELLOW        	 0xFFE0	//��ɫ
#define BROWN 			 0XBC40 //��ɫ
#define BRRED 			 0XFC07 //�غ�ɫ
#define GRAY  			 0X8430 //��ɫ
//GUI��ɫ

#define DARKBLUE      	 0X01CF	//����ɫ
#define LIGHTBLUE      	 0X7D7C	//ǳ��ɫ  
#define GRAYBLUE       	 0X5458 //����ɫ
//������ɫΪPANEL����ɫ 
 
#define LIGHTGREEN     	 0X841F //ǳ��ɫ
#define LIGHTGRAY        0XEF5B //ǳ��ɫ(PANNEL)
#define LGRAY 			 0XC618 //ǳ��ɫ(PANNEL),���屳��ɫ

#define LGRAYBLUE        0XA651 //ǳ����ɫ(�м����ɫ)
#define LBBLUE           0X2B12 //ǳ����ɫ(ѡ����Ŀ�ķ�ɫ)

#define X_min 0x0043		 //TP���Է�Χ��������
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
void LCD_ShowStrInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *p);
#endif/*LCD_VGM068A4W01_SH1106G*/
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len);
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len,uint8_t mode);
void LCD_ShowImg(uint16_t x, uint16_t y, unsigned char *color);
void LCD_ShowImg_From_Flash(uint16_t x, uint16_t y, uint32_t img_addr);
void LCD_SetFontSize(uint8_t font_size);
void LCD_MeasureString(uint8_t *p, uint16_t *width, uint16_t *height);
void LCD_get_pic_size(unsigned char *color, uint16_t *width, uint16_t *height);
void LCD_dis_pic_trans(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans);
void LCD_dis_pic_rotate(uint16_t x, uint16_t y, unsigned char *color, unsigned int rotate);
void LCD_dis_pic_trans_rotate(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans, unsigned int rotate);
#ifdef FONTMAKER_UNICODE_FONT
void LCD_MeasureUniString(uint16_t *p, uint16_t *width, uint16_t *height);
void LCD_ShowUniString(uint16_t x, uint16_t y, uint16_t *p);
void LCD_ShowUniStringInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *p);
#endif/*FONTMAKER_UNICODE_FONT*/

#endif
