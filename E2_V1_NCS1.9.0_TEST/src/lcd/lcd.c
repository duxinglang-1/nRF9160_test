/****************************************Copyright (c)************************************************
** File Name:			    Lcd.c
** Descriptions:			LCD source file
** Created By:				xie biao
** Created Date:			2020-07-13
** Modified Date:      		2020-12-18 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <nrf9160.h>
#include <zephyr.h>
#include <math.h>
#include "lcd.h"
#include "settings.h"
//#include "gps.h"
#include "nb.h"
#include "font.h"
#include "external_flash.h"
#include "max20353.h"
#include "screen.h"
#include "logger.h"

#if defined(LCD_ORCZ010903C_GC9A01)
#include "LCD_ORCZ010903C_GC9A01.h"
#elif defined(LCD_R108101_GC9307)
#include "LCD_R108101_GC9307.h"
#elif defined(LCD_ORCT012210N_ST7789V2)
#include "LCD_ORCT012210N_ST7789V2.h"
#elif defined(LCD_R154101_ST7796S)
#include "LCD_R154101_ST7796S.h"
#elif defined(LCD_LH096TIG11G_ST7735SV)
#include "LCD_LH096TIG11G_ST7735SV.h"
#elif defined(LCD_VGM068A4W01_SH1106G)
#include "LCD_VGM068A4W01_SH1106G.h"
#elif defined(LCD_VGM096064A6W01_SP5090)
#include "LCD_VGM096064A6W01_SP5090.h"
#endif 

#define PI	(3.1415926)

//LCD��Ļ�ĸ߶ȺͿ���
uint16_t LCD_WIDTH = COL;
uint16_t LCD_HEIGHT = ROW;

//LCD�Ļ�����ɫ�ͱ���ɫ	   
uint16_t POINT_COLOR=WHITE;	//������ɫ
uint16_t BACK_COLOR=BLACK;  //����ɫ 

//Ĭ��������
#ifdef FONT_16
SYSTEM_FONT_SIZE system_font = FONT_SIZE_16;
#elif defined(FONT_20)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_20;
#elif defined(FONT_24)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_24;
#elif defined(FONT_28)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_28;
#elif defined(FONT_32)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_32;
#elif defined(FONT_36)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_36;
#elif defined(FONT_48)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_48;
#elif defined(FONT_52)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_52;
#elif defined(FONT_64)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_64;
#elif defined(FONT_68)
SYSTEM_FONT_SIZE system_font = FONT_SIZE_68;
#else
SYSTEM_FONT_SIZE system_font = FONT_SIZE_16;
#endif

bool lcd_sleep_in = false;
bool lcd_sleep_out = false;
bool lcd_is_sleeping = true;
bool sleep_out_by_wrist = false;

static uint8_t imgbuf[LCD_DATA_LEN] = {0};

#ifdef FONTMAKER_UNICODE_FONT
font_uni_infor uni_infor = {0};
#endif

//���ٻ���
//x,y:����
//color:��ɫ
void LCD_Fast_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{	   
	BlockWrite(x,y,1,1);	//������
	WriteOneDot(color);				//���㺯��	
}	 

//��ָ����������䵥�����
//(x,y),(w,h):�����ζԽ�����,�����С�w*h   
//color:Ҫ������ɫ
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
void LCD_FillExtra(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color)
{          
	uint32_t i;

	if((x+w)>LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>LCD_HEIGHT)
		h = LCD_HEIGHT - y;

	y = y/PAGE_MAX;
	h = h/8;
	
	if(color != 0x00)
		color = 0xff;
	
#ifdef LCD_TYPE_SPI
	for(i=0;i<h;i++)
	{
		BlockWrite(x,y+i,w,h);
		DispColor(w, color);
	}
#else
	for(i=0;i<(w*h);i++)
		WriteOneDot(color); //��ʾ��ɫ 
#endif
}
#endif


void LCD_FillColor(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{          
	uint32_t i;

	if((x+w)>LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>LCD_HEIGHT)
		h = LCD_HEIGHT - y;

	BlockWrite(x,y,w,h);

#ifdef LCD_TYPE_SPI
	DispColor((w*h), color);
#else
	for(i=0;i<(w*h);i++)
		WriteOneDot(color); //��ʾ��ɫ 
#endif
}

void LCD_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{          
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_FillExtra(x, y, w, h, (uint8_t)(color&0x00ff));
#else
	LCD_FillColor(x, y, w, h, color);
#endif
}

//��ָ�����������ָ����ɫ��(��ʾͼƬ)		 
//(x,y),(w,h):�����ζԽ�����,�����С�w*h   
//color:Ҫ������ɫ
void LCD_Pic_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, unsigned char *color)
{  
	uint16_t high,width;
	uint16_t i,j;
	uint8_t databuf[2*COL] = {0};
	
	width=256*color[2]+color[3]; 			//��ȡͼƬ����
	high=256*color[4]+color[5];			//��ȡͼƬ�߶�
	
 	for(i=0;i<h;i++)
	{
		BlockWrite(x,y+i,w,1);	  	//����ˢ��λ��

	#ifdef LCD_TYPE_SPI
		for(j=0;j<w;j++)
		{
			databuf[2*j] = color[8+2*(i*width+j)];
			databuf[2*j+1] = color[8+2*(i*width+j)+1];
		}

		DispData(2*w, databuf);
	#else
		for(j=0;j<w;j++)
			WriteDispData(color[8+2*(i*width+j)],color[8+2*(i*width+j)+1]);	//��ʾ��ɫ 
	#endif
	}			
} 

//����
//x1,y1:�������
//x2,y2:�յ�����  
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol;
	
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if(delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//�������
	{  
		LCD_Fast_DrawPoint(uRow,uCol,POINT_COLOR);//���� 
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    

//������	  
//(x1,y1),(x2,y2):���εĶԽ�����
void LCD_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
#ifdef LCD_TYPE_SPI
	BlockWrite(x,y,w,1);
	DispColor(w, POINT_COLOR);
	BlockWrite(x,y,1,h);
	DispColor(h, POINT_COLOR);
	BlockWrite(x,y+h,w,1);
	DispColor(w, POINT_COLOR);
	BlockWrite(x+w,y,1,h);
	DispColor(h, POINT_COLOR);	
#else
	LCD_DrawLine(x,y,x+w,y);
	LCD_DrawLine(x,y,x,y+h);
	LCD_DrawLine(x,y+h,x+w,y+h);
	LCD_DrawLine(x+w,y,x+w,y+h);
#endif
}

//��ָ��λ�û�һ��ָ����С��Բ
//(x,y):���ĵ�
//r    :�뾶
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //�ж��¸���λ�õı�־
	while(a<=b)
	{
		LCD_Fast_DrawPoint(x0+a,y0-b,POINT_COLOR);//5
		LCD_Fast_DrawPoint(x0+b,y0-a,POINT_COLOR);//0 
		LCD_Fast_DrawPoint(x0+b,y0+a,POINT_COLOR);//4
		LCD_Fast_DrawPoint(x0+a,y0+b,POINT_COLOR);//6
		
		LCD_Fast_DrawPoint(x0-a,y0+b,POINT_COLOR);//1
		LCD_Fast_DrawPoint(x0-b,y0+a,POINT_COLOR);
		LCD_Fast_DrawPoint(x0-a,y0-b,POINT_COLOR);//2
		LCD_Fast_DrawPoint(x0-b,y0-a,POINT_COLOR);//7 
                	         
		a++;
		//ʹ��Bresenham�㷨��Բ     
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
} 

#ifdef IMG_FONT_FROM_FLASH

#ifdef FONTMAKER_UNICODE_FONT
uint8_t LCD_Show_Uni_Char_from_flash(uint16_t x, uint16_t y, uint16_t num, uint8_t mode)
{
	uint16_t temp,t1,t;
	uint16_t y0=y,x0=x,w,h;
	uint8_t cbyte=0;		//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���
	uint16_t csize=0;		//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t sect=0;
	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[1024] = {0};
	uint8_t headbuf[FONT_UNI_HEAD_LEN] = {0};
	uint8_t secbuf[8*FONT_UNI_SECT_LEN] = {0};
	uint32_t i=0,index_addr,font_addr,data_addr=0;

	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			font_addr = FONT_EN_UNI_16_ADDR;
			break;
	#endif		
	#ifdef FONT_20
		case FONT_SIZE_20:
			font_addr = FONT_EN_UNI_20_ADDR;
			break;
	#endif
	#ifdef FONT_28
		case FONT_SIZE_28:
			font_addr = FONT_EN_UNI_28_ADDR;
			break;
	#endif
	#ifdef FONT_36
		case FONT_SIZE_36:
			font_addr = FONT_EN_UNI_36_ADDR;
			break;
	#endif
	#ifdef FONT_52
		case FONT_SIZE_52:
			font_addr = FONT_EN_UNI_52_ADDR;
			break;
	#endif
	#ifdef FONT_68
		case FONT_SIZE_68:
			font_addr = FONT_EN_UNI_68_ADDR;
			break;
	#endif	
		default:
			return; 						//û�е��ֿ�
	}

	//read head data
	SpiFlash_Read((uint8_t*)&uni_infor.head, font_addr, FONT_UNI_HEAD_LEN);
	if((uni_infor.head.id[0] != FONT_UNI_HEAD_FLAG_0)
		||(uni_infor.head.id[1] != FONT_UNI_HEAD_FLAG_1)
		||(uni_infor.head.id[2] != FONT_UNI_HEAD_FLAG_2))
	{
		return;
	}

	//read sect data
	SpiFlash_Read((uint8_t*)&uni_infor.sect, font_addr+FONT_UNI_HEAD_LEN, uni_infor.head.sect_num*FONT_UNI_SECT_LEN);
	
	//read index data
	for(i=0;i<uni_infor.head.sect_num;i++)
	{
		if((num>=uni_infor.sect[i].first_char)&&((num<=uni_infor.sect[i].last_char)))
		{
			index_addr = (num-uni_infor.sect[i].first_char)*4+uni_infor.sect[i].index_addr;
			break;
		}
	}
	
	SpiFlash_Read(fontbuf, font_addr+index_addr, 4);
	uni_infor.index.font_addr = 0x03ffffff&(fontbuf[0]+0x100*fontbuf[1]+0x10000*fontbuf[2]+0x1000000*fontbuf[3]);
	uni_infor.index.width = fontbuf[3]>>2;
	cbyte = uni_infor.index.width;
	csize = ((cbyte+7)/8)*system_font;
	//read font data
	if(csize > sizeof(fontbuf))
		csize = sizeof(fontbuf);
	SpiFlash_Read(fontbuf, font_addr+uni_infor.index.font_addr, csize);

	w = cbyte;
	h = system_font;
	
#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//����ˢ��λ��
#endif

	for(t=0;t<csize;t++)
	{		
		temp = fontbuf[t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;

			}
			if((x-x0)==cbyte)
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //��������
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;				
			}
			if((x-x0)==cbyte)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}
		#endif
		}
	}

	return cbyte;
}

#elif defined(FONTMAKER_MBCS_FONT)
/*********************************************************************************************************************
* Name:LCD_Show_MBCS_Char_from_flash
* Function:��ʾfontmaker�������ɵ�bin��ʽ��Ӣ�ı�������ֿ�
* Description:
* 	������: 
* 	��00000010h ��ʼ��ÿ 4 ���ֽڱ�ʾһ���ַ��ļ�����Ϣ�� �Ҵ��ַ� 0x0 ��ʼ���ʿո��ַ���' '������Ϊ 0x20���ļ�����Ϣ
* 	���ļ�ͷ�ĳ���+�ַ�����*4 = 0x10 + 0x20 *4 = 0x90�� �� 000000090h��Ϊ��10 04 00 10�����ó�һ�� 32 λ��Ϊ�� 
* 	0x10000410��ʮ�����ƣ� --- ��00010000 00000000 00000100 00010000��. 
* 	�� 6 λ����ʾ��ǰ�ַ��Ŀ��ȡ� �ʵó� 000100 -- 4 ���ֿ�����4 ��
* 	�� 26 λ����ʾ�����ַ��ĵ������ݵ�ƫ�Ƶ�ַ���ʵó� 00 00000000 00000100 00010000 -- 0x410 ��������Ϣ����ʼ��ַΪ 0x410) 
* 
* 	�������� 
* 	���ڿո��ַ�����ʼ��ַΪ 0x410�������ݳ���Ϊ�������������7��/8��* ����߶�= ((4+7)/8)*16 = 16. 
* 	��ȡ���� 16 �ֽڣ���Ϊ�ո��ַ��ĵ������ݡ�
*********************************************************************************************************************/
uint8_t LCD_Show_Mbcs_Char_from_flash(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
	uint8_t temp,t1,t;
	uint16_t y0=y,x0=x,w,h;
	uint8_t cbyte=0;		//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���(Ӣ�Ŀ������ֿ���һ��)
	uint16_t csize=0;		//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[256] = {0};
	uint32_t i=0,index_addr,font_addr,data_addr=0;

	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			font_addr = FONT_RM_ASC_16_ADDR;
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			font_addr = FONT_RM_ASC_24_ADDR;
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			font_addr = FONT_RM_ASC_32_ADDR;
			break;
	#endif
		default:
			return; 						//û�е��ֿ�
	}

	index_addr = FONT_MBCS_HEAD_LEN+4*num;
	SpiFlash_Read(fontbuf, font_addr+index_addr, 4);
	data_addr = fontbuf[0]+0x100*fontbuf[1]+0x10000*fontbuf[2];
	cbyte = fontbuf[3]>>2;
	csize = ((cbyte+7)/8)*system_font;	
	SpiFlash_Read(fontbuf, font_addr+data_addr, csize);

	w = cbyte;
	h = system_font;
	
#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//����ˢ��λ��
#endif

	for(t=0;t<csize;t++)
	{		
		temp = fontbuf[t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;
			}
			if((x-x0)==cbyte)
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //��������
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;				
			}
			if((x-x0)==cbyte)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}
		#endif
		}
	}

	return cbyte;
}

/*********************************************************************************************************************
* Name:LCD_Show_MBCS_CJK_Char_from_flash
* Function:��ʾfontmaker�������ɵ�bin��ʽ��CJK�ȿ������ֿ�
* Description:
* ��ȥ�ļ�ͷ 16 Byte �⣬�������ݶ��Ǵ��ַ��������ݡ� 
* ��Ϊ GB2312���������ģ� ���׸��ַ�Ϊ��0xA1A1�� ���ĵ���������ʼ��ַΪ 0x10����ȥ�ļ�ͷ����
* ���ݳ���Ϊ����������߶�7��/8��* ����߶�= ((16+7)/8)*16=32 
* �ʴ� 0x10 ��ʼ����ȡ 32 �ֽڣ���Ϊ�ַ� 0xA1A1 �ĵ������ݡ�
* 
* 	�������� 
* 	���ڿո��ַ�����ʼ��ַΪ 0x410�������ݳ���Ϊ�������������7��/8��* ����߶�= ((4+7)/8)*16 = 16. 
* 	��ȡ���� 16 �ֽڣ���Ϊ�ո��ַ��ĵ������ݡ�
*********************************************************************************************************************/
uint8_t LCD_Show_Mbcs_CJK_Char_from_flash(uint16_t x, uint16_t y, uint16_t num, uint8_t mode)
{
	uint8_t temp,t1,t;
	uint16_t x0=x,y0=y,w,h;
	uint8_t cbyte=0;					//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���
	uint16_t csize=0;					//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[256] = {0};
	uint32_t i=0,index,font_addr,data_addr=0;
	uint8_t R_code=0xFF&(num>>8);		//����
	uint8_t C_code=0xFF&num;			//λ��
	
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			font_addr = FONT_RM_JIS_16_ADDR;
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			font_addr = FONT_RM_JIS_24_ADDR;
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			font_addr = FONT_RM_JIS_32_ADDR;
			break;
	#endif
		default:
			return; 						//û�е��ֿ�
	}

	switch(global_settings.language)
	{
	case LANGUAGE_JPN:
		if((R_code>=0x81)&&(R_code<=0x9F))
		{
			if((C_code>=0x40)&&(C_code<=0x7E))
				index = (R_code-0x81)*188+(C_code-0x40);		//188= 0x7E-0x40+1)+(0xFC-0x80+1);	
			else if((C_code>=0x80)&&(C_code<=0xFC))
				index = (R_code-0x81)*188+(C_code-0x80)+63; 	//63 = 0x7E-0x40+1;
		}
		else if((R_code>=0xE0)&&(R_code<=0xFC))
		{
			if((C_code>=0x40)&&(C_code<=0x7E))
				index = 5828+(R_code-0xE0)*188+(C_code-0x40);	//5828 = 188 * (0x9F-0x81+1);
			else if((C_code>=0x80)&&(C_code<=0xFC))
				index = 5828+(R_code-0xE0)*188+(C_code-0x80)+63;
		}
		break;
		
	case LANGUAGE_CHN:
		if(((R_code>=0xA1)&&(R_code<=0xFE))&&((C_code>=0xA1)&&(C_code<=0xFE)))
		{
			index = 94*(R_code-0xA1)+(C_code-0xA1);
		}
		break;
	}
	
	cbyte = system_font;
	csize = ((cbyte+7)/8)*system_font;
	data_addr = FONT_MBCS_HEAD_LEN + csize*index;
	SpiFlash_Read(fontbuf, font_addr+data_addr, csize);

	w = cbyte;
	h = system_font;

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//����ˢ��λ��
#endif

	for(t=0;t<csize;t++)
	{
		temp = fontbuf[t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)							//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;							//��������
				t=t+(cbyte-(t%cbyte))-1;				//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;
			}
			if((x-x0)==(system_font))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;							//��������
				break;
			}			
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)							//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;				//��������
				t=t+(cbyte-(t%cbyte))-1;				//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;			
			}
			if((x-x0)==system_font)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;				//��������
				break;
			}
		#endif
		} 
	} 

	return cbyte;
}
#else
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//��ָ��λ����ʾflash��һ���ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowEn_from_flash(uint16_t x,uint16_t y,uint8_t num)
{
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);		//����ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���(Ӣ�Ŀ������ֿ���һ��)
	uint16_t csize=cbyte*(system_font/2);					//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t fontbuf[1024] = {0};
 	uint8_t databuf[128] = {0};
	uint32_t i=0;
	
	num=num-' ';	//�õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩
	switch(system_font)
	{
	#ifdef FONT_8
		case FONT_SIZE_8:
			SpiFlash_Read(fontbuf, FONT_ASC_0804_ADDR+csize*num, csize);
			BlockWrite(x, y, (system_font/2),1);
			memcpy(databuf, fontbuf, (system_font/2));
			DispData((system_font/2), databuf);
			break;
	#endif
	
	#ifdef FONT_16
		case FONT_SIZE_16:
			SpiFlash_Read(fontbuf, FONT_ASC_1608_ADDR+csize*num, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &fontbuf[(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif

	#ifdef FONT_24
		case FONT_SIZE_24:
			SpiFlash_Read(fontbuf, FONT_ASC_2412_ADDR+csize*num, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &fontbuf[(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif

	#ifdef FONT_32
		case FONT_SIZE_32:
			SpiFlash_Read(fontbuf, FONT_ASC_3216_ADDR+csize*num, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &fontbuf[(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif

	#ifdef FONT_48
		case FONT_SIZE_48:
			SpiFlash_Read(fontbuf, FONT_ASC_4824_ADDR+csize*num, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &fontbuf[(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif

	#ifdef FONT_64
		case FONT_SIZE_64:
			SpiFlash_Read(fontbuf, FONT_ASC_6432_ADDR+csize*num, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &fontbuf[(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif
	}
}   

//��ָ��λ����ʾflash��һ�������ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowCn_from_flash(uint16_t x, uint16_t y, uint16_t num)
{  							  
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);		//����ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���(Ӣ�Ŀ������ֿ���һ��)
	uint16_t csize=cbyte*system_font;						//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t fontbuf[2*1024] = {0};
 	uint8_t databuf[256] = {0};
	uint16_t index=0;	
	uint32_t i=0;
	
	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(����-1)+(λ��-1))*32
	switch(system_font)
	{
	#ifdef FONT_8
		case FONT_SIZE_8:
			SpiFlash_Read(fontbuf, FONT_CHN_SM_0808_ADDR+csize*index, csize);
			BlockWrite(x, y, system_font, 1);
			memcpy(databuf, fontbuf, system_font);
			DispData(system_font, databuf);
			break;
	#endif
	
	#ifdef FONT_16
		case FONT_SIZE_16:
			SpiFlash_Read(fontbuf, FONT_CHN_SM_1616_ADDR+csize*index, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x, y+i, system_font,1);
				memcpy(databuf, &fontbuf[system_font*i], system_font);
				DispData(system_font, databuf);
			}
			break;
	#endif

	#ifdef FONT_24
		case FONT_SIZE_24:
			SpiFlash_Read(fontbuf, FONT_CHN_SM_2424_ADDR+csize*index, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x, y+i, system_font, 1);
				memcpy(databuf, &fontbuf[system_font*i], system_font);
				DispData(system_font, databuf);
			}
			break;
	#endif

	#ifdef FONT_32
		case FONT_SIZE_32:
			SpiFlash_Read(fontbuf, FONT_CHN_SM_3232_ADDR+csize*index, csize);
			for(i=0;i<cbyte;i++)
			{
				BlockWrite(x, y+i, system_font, 1);
				memcpy(databuf, &fontbuf[system_font*i], system_font);
				DispData(system_font, databuf);
			}
			break;
	#endif
	}
} 
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

//��ָ��λ����ʾflash��һ���ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowChar_from_flash(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
	uint8_t temp,t1,t;
	uint8_t cbyte=(system_font/2)/8+(((system_font/2)%8)?1:0);		//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���(Ӣ�Ŀ������ֿ���һ��)
	uint16_t csize=cbyte*system_font;		//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
 	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[256] = {0};
	uint16_t y0=y,x0=x,w=(system_font/2),h=system_font;
	uint32_t i=0;
	
	num=num-' ';//�õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			SpiFlash_Read(fontbuf, FONT_ASC_1608_ADDR+csize*num, csize);
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			SpiFlash_Read(fontbuf, FONT_ASC_2412_ADDR+csize*num, csize);
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			SpiFlash_Read(fontbuf, FONT_ASC_3216_ADDR+csize*num, csize);
			break;
	#endif
	#ifdef FONT_48
		case FONT_SIZE_48:
			SpiFlash_Read(fontbuf, FONT_ASC_4824_ADDR+csize*num, csize);
			break;
	#endif
	#ifdef FONT_64
		case FONT_SIZE_64:
			SpiFlash_Read(fontbuf, FONT_ASC_6432_ADDR+csize*num, csize);
			break;
	#endif
		default:
			return;							//û�е��ֿ�
	}

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h); 	//����ˢ��λ��
#endif

	for(t=0;t<csize;t++)
	{		
		temp = fontbuf[t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;

			}
			if((x-x0)==(system_font/2))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;				
			}
			if((x-x0)==(system_font/2))
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; 	//��������
				break;
			}
		#endif
		}
	}
}    

//��ָ��λ����ʾflash��һ�������ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowChineseChar_from_flash(uint16_t x,uint16_t y,uint16_t num,uint8_t mode)
{  							  
	uint8_t temp,t1,t;
	uint16_t x0=x,y0=y,w=system_font,h=system_font;
	uint16_t index=0;
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);		//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���
	uint16_t csize=cbyte*(system_font);						//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[256] = {0};
	uint32_t i=0;
	
	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(����-1)+(λ��-1))*32
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			SpiFlash_Read(fontbuf, FONT_CHN_SM_1616_ADDR+csize*index+t, csize);
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			SpiFlash_Read(fontbuf, FONT_CHN_SM_2424_ADDR+csize*index+t, csize);
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			SpiFlash_Read(fontbuf, FONT_CHN_SM_3232_ADDR+csize*index+t, csize);
			break;
	#endif
		default:
			return;								//û�е��ֿ�
	}	

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h); 	//����ˢ��λ��
#endif

	for(t=0;t<csize;t++)
	{
		temp = fontbuf[t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;
			}
			if((x-x0)==(system_font))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}			
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;			
			}
			if((x-x0)==system_font)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}
		#endif
		} 
	} 
} 

#endif/*FONTMAKER_UNICODE_FONT*/

//��ȡflash�е�ͼƬ�ߴ�
//color:ͼƬ����ָ��
//width:��ȡ����ͼƬ����������
//height:��ȡ����ͼƬ�߶�������
void LCD_get_pic_size_from_flash(uint32_t pic_addr, uint16_t *width, uint16_t *height)
{
	uint8_t databuf[6] = {0};

	SpiFlash_Read(databuf, pic_addr, 6);
	*width = 256*databuf[2]+databuf[3]; 			//��ȡͼƬ����
	*height = 256*databuf[4]+databuf[5];			//��ȡͼƬ�߶�
}

//ָ��λ����ʾflash�е�ͼƬ
//pic_addr:ͼƬ��flash�еĵ�ַ
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
void LCD_dis_pic_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr)
{
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w=256*databuf[2]+databuf[3]; 			//��ȡͼƬ����
	h=256*databuf[4]+databuf[5];			//��ȡͼƬ�߶�

	pic_addr += 8;

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	if(show_w > LCD_WIDTH)
		show_w = LCD_WIDTH;
	if(show_h > LCD_HEIGHT)
		show_h = LCD_HEIGHT;
	
	BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��

	datelen = 2*show_w*show_h;
	if(show_w < w)
		readlen = 2*show_w;
	if(readlen > LCD_DATA_LEN)
		readlen = LCD_DATA_LEN;
	
	while(datelen)
	{
		if(datelen < readlen)
		{
			readlen = datelen;
			datelen = 0;
		}
		else
		{
			readlen = readlen;
			datelen -= readlen;
		}
		
		memset(databuf, 0, LCD_DATA_LEN);
		SpiFlash_Read(databuf, pic_addr, readlen);
		
		if(show_w < w)
			pic_addr += 2*w;
		else
			pic_addr += readlen;

	#ifdef LCD_TYPE_SPI
		DispData(readlen, databuf);
	#else
		for(i=0;i<(readlen/2);i++)
			WriteDispData(databuf[2*i],databuf[2*i+1]);	//��ʾ��ɫ 
	#endif
	}
}

//ָ��λ����ʾflash�е�ͼƬ,����ɫ����
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
//pic_addr:ͼƬ��flash�еĵ�ַ
//trans:��Ҫ���˵���ɫ
void LCD_dis_pic_trans_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr, uint16_t trans)
{
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w=256*databuf[2]+databuf[3]; 			//��ȡͼƬ����
	h=256*databuf[4]+databuf[5];			//��ȡͼƬ�߶�

	pic_addr += 8;

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;
	
	BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��

	datelen = 2*show_w*show_h;
	if(show_w < w)
		readlen = 2*show_w;
	
	while(datelen)
	{
		if(datelen < readlen)
		{
			readlen = datelen;
			datelen = 0;
		}
		else
		{
			readlen = readlen;
			datelen -= readlen;
		}
		
		memset(databuf, 0, LCD_DATA_LEN);
		SpiFlash_Read(databuf, pic_addr, readlen);
		for(i=0;i<(readlen/2);i++)
		{
			if(BACK_COLOR == (256*databuf[2*i]+databuf[2*i+1]))
			{
				databuf[2*i] = trans>>8;
				databuf[2*i+1] = trans;
			}
		}
		
		if(show_w < w)
			pic_addr += 2*w;
		else
			pic_addr += readlen;

	#ifdef LCD_TYPE_SPI
		DispData(readlen, databuf);
	#else
		for(i=0;i<(readlen/2);i++)
			WriteDispData(databuf[2*i],databuf[2*i+1]);	//��ʾ��ɫ 
	#endif
	}
}

//ָ��λ����ת�ض��Ƕ���ʾͼƬ
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
//rotate:��ת�Ƕ�,0,90,180,270,
void LCD_dis_pic_rotate_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr, unsigned int rotate)
{
	uint16_t h,w,show_w,show_h;
	uint32_t i,j=0,k=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w=256*databuf[2]+databuf[3]; 			//��ȡͼƬ����
	h=256*databuf[4]+databuf[5];			//��ȡͼƬ�߶�

	pic_addr += 8;

	switch(rotate)
	{
	case 0:
		offset = pic_addr;
		
		if((x+w)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = w;
		
		if((y+h)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = h;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < w)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			SpiFlash_Read(databuf, offset, readlen);
			
			if(show_w < w)
				offset += 2*w;
			else
				offset += readlen;

		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}		
		break;
		
	case 90:
		offset = pic_addr + 2*w*(h-1);
	
		if((x+h)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = h;
		
		if((y+w)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = w;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < h)
			readlen = 2*show_w;
		
		while(datelen)
		{
			uint32_t len;
			
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			
			for(i=0;i<(readlen/2);i++,j++)
			{
				SpiFlash_Read(&databuf[2*i], (offset+2*((j/h)-w*(j%h))), 2);
			}
			
			if(show_w < h)
			{
				offset += 2;
				j = 0;
			}
			
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}	
		break;
		
	case 180:
		offset = pic_addr + 2*(w*h-1);
		
		if((x+w)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = w;
		
		if((y+h)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = h;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < w)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			for(i=0;i<(readlen/2);i++,j++)
			{
				SpiFlash_Read(&databuf[2*i], (offset-2*j), 2);
			}

			if(show_w < w)
			{
				offset -= 2*w;
				j = 0;
			}
			
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}
		break;
		
	case 270:
		offset = pic_addr + 2*(w-1);
			
		if((x+h)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = h;
		
		if((y+w)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = w;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < h)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			
			for(i=0;i<(readlen/2);i++,j++)
			{
				SpiFlash_Read(&databuf[2*i], (offset-2*(j/h)+2*w*(j%h)), 2);
			}

			if(show_w < w)
			{
				offset -= 2;
				j = 0;
			}
			
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}
		break;
	}
}

//ָ������λ������Ƕ���ʾͼ�
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
//angle:�Ƕ�,0~360,
void LCD_dis_pic_angle_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr, unsigned int angle)
{
	uint16_t c_x,c_y,c_r,h,w,show_w,show_h;
	int16_t offset_x,offset_y;
	int32_t i,j=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w = 256*databuf[2]+databuf[3]; 			//��ȡͼƬ����
	h = 256*databuf[4]+databuf[5];			//��ȡͼƬ�߶�
	c_x = LCD_WIDTH/2;
	c_y = LCD_HEIGHT/2;
	c_r = w;
	
	pic_addr += 8;

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	datelen = 2*show_w*show_h;
	if(show_w < w)
		readlen = 2*show_w;

	//LOGD("c_x:%d, c_y:%d", c_x, c_y);
	while(datelen)
	{
		if(datelen < readlen)
		{
			readlen = datelen;
			datelen = 0;
		}
		else
		{
			readlen = readlen;
			datelen -= readlen;
		}
		
		memset(databuf, 0, LCD_DATA_LEN);
		SpiFlash_Read(databuf, pic_addr, readlen);
		
		if(show_w < w)
			pic_addr += 2*w;
		else
			pic_addr += readlen;

		for(i=(readlen/2)-2;i>0;i--)
		{
			if(((i%show_h) == 0) && (j <= c_r))
			{
				offset_y = j*sin(angle*PI/180);
				offset_x = j*cos(angle*PI/180);
				//LOGD("i:%d,angle:%d,c_r:%d,offset_x:%d,offset_y:%d", i,angle,c_r,offset_x,offset_y);
				BlockWrite(c_x+offset_x, c_y-offset_y, 1, 1);	//����ˢ��λ��
				j++;
			}
			
			WriteDispData(databuf[2*i], databuf[2*i+1]); //��ʾ��ɫ 
		}
	}		
}

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//ָ��λ����ʾflash�е�ͼƬ
//pic_addr:ͼƬ��flash�еĵ�ַ
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
void LCD_dis_img_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr)
{
	uint16_t h,w,show_w,show_h;
	uint8_t i=0;
	uint8_t databuf[COL]={0};
	uint32_t offset=6,datelen,showlen=0,readlen=COL;
	
	SpiFlash_Read(databuf, pic_addr, 8);
	w=databuf[2]+256*databuf[3]; 			//��ȡͼƬ����
	h=databuf[4]+256*databuf[5];			//��ȡͼƬ�߶�

	pic_addr += 6;

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	if(show_w > LCD_WIDTH)
		show_w = LCD_WIDTH;
	if(show_h > LCD_HEIGHT)
		show_h = LCD_HEIGHT;

	BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
	datelen = show_w*(show_h/8+((show_h%8)?1:0));
	if(show_w < w)
		readlen = show_w;
	if(readlen > LCD_DATA_LEN)
		readlen = LCD_DATA_LEN;
	
	while(datelen)
	{
		if(datelen < readlen)
		{
			readlen = datelen;
			datelen = 0;
		}
		else
		{
			readlen = readlen;
			datelen -= readlen;
		}
		
		memset(databuf, 0, COL);
		SpiFlash_Read(databuf, pic_addr, readlen);
		DispData(readlen, databuf);
		
		if(show_w < w)
			pic_addr += w;
		else
			pic_addr += readlen;

		i++;
		if(((y%PAGE_MAX)+i)>3)
			return;
			
		BlockWrite(x,y+i,show_w,show_h);	//����ˢ��λ��
	}
}
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

//ָ��λ����ʾflash�е�ͼƬ
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
void LCD_ShowImg_From_Flash(uint16_t x, uint16_t y, uint32_t img_addr)
{
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_dis_img_from_flash(x, y, img_addr);
#else
	LCD_dis_pic_from_flash(x, y, img_addr);
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/
}
#endif/*IMG_FONT_FROM_FLASH*/

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//��ָ��λ����ʾһ���ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
void LCD_ShowEn(uint16_t x,uint16_t y,uint8_t num)
{
 	uint8_t i,databuf[128] = {0};
	
	num=num-' ';//�õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩

	switch(system_font)
	{
	#ifdef FONT_8
		case FONT_SIZE_8:
			BlockWrite(x,y,(system_font/2),1);
			memcpy(databuf, &asc2_SH1106_0804[num][0],(system_font/2));
			DispData((system_font/2), databuf);
			break;
	#endif
	
	#ifdef FONT_16
		case FONT_SIZE_16:
			for(i=0;i<system_font/8;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &asc2_SH1106_1608[num][(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif

	#ifdef FONT_24
		case FONT_SIZE_24:
			for(i=0;i<system_font/8;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &asc2_SH1106_2412[num][(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif

	#ifdef FONT_32
		case FONT_SIZE_32:
			for(i=0;i<system_font/8;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &asc2_SH1106_3216[num][(system_font/2)*i],(system_font/2));
				DispData((system_font/2), databuf);
			}
			break;
	#endif
	}
}

//��ָ��λ����ʾһ�������ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
void LCD_ShowCn(uint16_t x, uint16_t y, uint16_t num)
{  							  
	uint8_t i,databuf[128] = {0};
	uint16_t index=0;

	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(����-1)+(λ��-1))*32
	switch(system_font)
	{
	#if 0//def FONT_8
		case FONT_SIZE_8:
			BlockWrite(x,y+i,system_font,1);
			memcpy(databuf, &chinse_SH1106_0808[index][system_font*i],system_font);
			DispData(system_font, databuf);
			break;
	#endif
	
	#if 0//def FONT_16
		case FONT_SIZE_16:
			for(i=0;i<system_font/8;i++)
			{
				BlockWrite(x,y+i,system_font,1);
				memcpy(databuf, &chinse_SH1106_1616[index][system_font*i],system_font);
				DispData(system_font, databuf);
			}
			break;
	#endif

	#if 0//def FONT_24
		case FONT_SIZE_24:
			for(i=0;i<system_font/8;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &chinse_SH1106_2424[index][system_font*i],system_font);
				DispData(system_font, databuf);
			}
			break;
	#endif

	#if 0//def FONT_32
		case FONT_SIZE_32:
			for(i=0;i<system_font/8;i++)
			{
				BlockWrite(x,y+i,(system_font/2),1);
				memcpy(databuf, &chinse_SH1106_3232[index][system_font*i],system_font);
				DispData(system_font, databuf);
			}
			break;
	#endif
	}
}  
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

//��ָ��λ����ʾһ���ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
    uint8_t temp,t1,t,i=0;
	uint16_t y0=y,x0=x,w=(system_font/2),h=system_font;
	uint8_t cbyte=(system_font/2)/8+(((system_font/2)%8)?1:0);		//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���(Ӣ�Ŀ������ֿ���һ��)
	uint16_t csize=cbyte*system_font;		//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t fontbuf[256] = {0};	
 	uint8_t databuf[2*COL] = {0};

	num=num-' ';//�õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			memcpy(fontbuf, asc2_1608[num], csize);	//����1608����
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			memcpy(fontbuf, asc2_2412[num], csize);	//����2412����
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			memcpy(fontbuf, asc2_3216[num], csize);	//����3216����
			break;
	#endif
	#ifdef FONT_48
		case FONT_SIZE_48:
			memcpy(fontbuf, asc2_4824[num], csize);	//����4824����
			break;
	#endif
	#ifdef FONT_64
		case FONT_SIZE_64:
			memcpy(fontbuf, asc2_6432[num], csize); //����6432����
			break;
	#endif
		default:
			return;							//û�е��ֿ�
	}

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//����ˢ��λ��
#endif
		
	for(t=0;t<csize;t++)
	{
		temp = fontbuf[t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;

				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;

			}
			if((x-x0)==(system_font/2))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;				
			}
			if((x-x0)==(system_font/2))
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; 	//��������
				break;
			}
		#endif
		}	
	}  	    	   	 	  
}

//��ָ��λ����ʾһ�������ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowChineseChar(uint16_t x,uint16_t y,uint16_t num,uint8_t mode)
{  							  
	uint8_t temp,t1,t,i=0;
	uint16_t x0=x,y0=y,w=system_font,h=system_font;
	uint16_t index=0;
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);				//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���
	uint16_t csize=cbyte*(system_font);								//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
	uint8_t fontbuf[256] = {0};
	uint8_t databuf[2*COL] = {0};

	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(����-1)+(λ��-1))*32
	switch(system_font)
	{
	#if 0	//def FONT_16
		case FONT_SIZE_16:
			memcpy(fontbuf, chinese_1616[index], csize);	//����1608����
			break;
	#endif
	#if 0	//def FONT_24
		case FONT_SIZE_24:
			memcpy(fontbuf, chinese_2424[index], csize);	//����2424����
			break;
	#endif
	#if 0	//def FONT_32
		case FONT_SIZE_32:
			memcpy(fontbuf, chinese_3232[index], csize);	//����3232����
			break;
	#endif
		default:
			return;								//û�е��ֿ�
	}	

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//����ˢ��λ��
#endif

	for(t=0;t<csize;t++)
	{	
		temp = fontbuf[t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;

				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;
			}
			if((x-x0)==(system_font))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}			
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;			
			}
			if((x-x0)==system_font)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}
		#endif
		} 
	}  	    	   	 	  
}  

#ifdef FONTMAKER_MBCS_FONT
/*********************************************************************************************************************
* Name:LCD_Show_MBCS_Char
* Function:��ʾfontmaker�������ɵ�bin��ʽ�ĵ����ֿ�
* Description:
* 	������: 
* 	��00000010h ��ʼ��ÿ 4 ���ֽڱ�ʾһ���ַ��ļ�����Ϣ�� �Ҵ��ַ� 0x0 ��ʼ���ʿո��ַ���' '������Ϊ 0x20���ļ�����Ϣ
* 	���ļ�ͷ�ĳ���+�ַ�����*4 = 0x10 + 0x20 *4 = 0x90�� �� 000000090h��Ϊ��10 04 00 10�����ó�һ�� 32 λ��Ϊ�� 
* 	0x10000410��ʮ�����ƣ� --- ��00010000 00000000 00000100 00010000��. 
* 	�� 6 λ����ʾ��ǰ�ַ��Ŀ��ȡ� �ʵó� 000100 -- 4 ���ֿ�����4 ��
* 	�� 26 λ����ʾ�����ַ��ĵ������ݵ�ƫ�Ƶ�ַ���ʵó� 00 00000000 00000100 00010000 -- 0x410 ��������Ϣ����ʼ��ַΪ 0x410) 
* 
* 	�������� 
* 	���ڿո��ַ�����ʼ��ַΪ 0x410�������ݳ���Ϊ�������������7��/8��* ����߶�= ((4+7)/8)*16 = 16. 
* 	��ȡ���� 16 �ֽڣ���Ϊ�ո��ַ��ĵ������ݡ�
*********************************************************************************************************************/
uint8_t LCD_Show_Mbcs_Char(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
	uint8_t temp,t1,t,i=0,*ptr_font;
	uint16_t y0=y,x0=x,w,h;
	uint8_t cbyte=0;		//��ɨ�裬ÿ���ַ�ÿһ��ռ�õ��ֽ���(Ӣ�Ŀ������ֿ���һ��)
	uint16_t csize=0;		//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
 	uint8_t databuf[2*COL] = {0};
	uint32_t index_addr,data_addr=0;

	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			ptr_font=asc2_16_rm; 	 		//����1608����
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			ptr_font=asc2_24_rm;			//����2412����
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			ptr_font=asc2_32_rm;			//����3216����
			break;
	#endif
		default:
			return 0;						//û�е��ֿ�
	}

	index_addr = FONT_MBCS_HEAD_LEN+4*num;
	data_addr = ptr_font[index_addr]+0x100*ptr_font[index_addr+1]+0x10000*ptr_font[index_addr+2];
	cbyte = ptr_font[index_addr+3]>>2;
	csize = ((cbyte+7)/8)*system_font;

	w = cbyte;
	h = 1;
	
#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//����ˢ��λ��
#endif
	
	for(t=0;t<csize;t++)
	{
		temp=ptr_font[data_addr+t];
		for(t1=0;t1<8;t1++)
		{
		#ifdef LCD_TYPE_SPI
			if(temp&0x80)
			{
				databuf[2*i] = POINT_COLOR>>8;
				databuf[2*i+1] = POINT_COLOR;
			}
			else if(mode==0)
			{
				databuf[2*i] = BACK_COLOR>>8;
				databuf[2*i+1] = BACK_COLOR;
			}
			
			temp<<=1;
			i++;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				DispData(2*i, databuf);
				i=0;

				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return cbyte;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;

			}
			if((x-x0)==cbyte)
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return cbyte;	//��������
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//����������ֱ����ʾ��һ��
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				t=t+(cbyte-(t%cbyte))-1;	//��ȡ��һ�ж�Ӧ���ֽڣ�ע��forѭ��������1��������������ǰ��ȥ1
				break;				
			}
			if((x-x0)==cbyte)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//��������
				break;
			}
		#endif
		}	
	}  	 

	return cbyte;
}
#endif/*FONTMAKER_MBCS_FONT*/

//��ȡͼƬ�ߴ�
//color:ͼƬ����ָ��
//width:��ȡ����ͼƬ����������
//height:��ȡ����ͼƬ�߶�������
void LCD_get_pic_size(unsigned char *color, uint16_t *width, uint16_t *height)
{
	*width = 256*color[2]+color[3];
	*height = 256*color[4]+color[5];
}

//ָ��λ����ʾͼƬ
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
void LCD_dis_pic(uint16_t x, uint16_t y, unsigned char *color)
{  
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	w=256*color[2]+color[3]; 			//��ȡͼƬ����
	h=256*color[4]+color[5];			//��ȡͼƬ�߶�

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
	datelen = 2*show_w*show_h;
	if(show_w < w)
		readlen = 2*show_w;
	
	while(datelen)
	{
		if(datelen < readlen)
		{
			readlen = datelen;
			datelen = 0;
		}
		else
		{
			readlen = readlen;
			datelen -= readlen;
		}
		
		memset(databuf, 0, LCD_DATA_LEN);
		memcpy(databuf, &color[offset], readlen);
		
		if(show_w < w)
			offset += 2*w;
		else
			offset += readlen;

	#ifdef LCD_TYPE_SPI
		DispData(readlen, databuf);
	#else
		for(i=0;i<(readlen/2);i++)
			WriteDispData(databuf[2*i],databuf[2*i+1]);	//��ʾ��ɫ 
	#endif
	}
}

//ָ��λ����ʾͼƬ,����ɫ����
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
//color:ͼƬ����ָ��
//trans:��Ҫ���˵���ɫ
void LCD_dis_pic_trans(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans)
{  
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;

	w=256*color[2]+color[3]; 			//��ȡͼƬ����
	h=256*color[4]+color[5];			//��ȡͼƬ�߶�

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;

	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
	datelen = 2*show_w*show_h;
	if(show_w < w)
		readlen = 2*show_w;

	while(datelen)
	{
		if(datelen < readlen)
		{
			readlen = datelen;
			datelen = 0;
		}
		else
		{
			readlen = readlen;
			datelen -= readlen;
		}
		
		memset(databuf, 0, LCD_DATA_LEN);
		memcpy(databuf, &color[offset], readlen);
		
		if(show_w < w)
			offset += 2*w;
		else
			offset += readlen;

		for(i=0;i<(readlen/2);i++)
		{
			if(BLACK == (256*databuf[2*i]+databuf[2*i+1]))
			{
				databuf[2*i] = trans>>8;
				databuf[2*i+1] = trans;
			}
		}
		
	#ifdef LCD_TYPE_SPI
		DispData(readlen, databuf);
	#else
		for(i=0;i<(readlen/2);i++)
			WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
	#endif
	}
}


//ָ��λ����ת�ض��Ƕ���ʾͼƬ
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
//rotate:��ת�Ƕ�,0,90,180,270,
void LCD_dis_pic_rotate(uint16_t x, uint16_t y, unsigned char *color, unsigned int rotate)
{
	uint16_t h,w,show_w,show_h;
	uint32_t i,j=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	w=256*color[2]+color[3]; 			//��ȡͼƬ����
	h=256*color[4]+color[5];			//��ȡͼƬ�߶�

	switch(rotate)
	{
	case 0:
		if((x+w)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = w;
		
		if((y+h)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = h;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < w)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			memcpy(databuf, &color[offset], readlen);
			
			if(show_w < w)
				offset += 2*w;
			else
				offset += readlen;

		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//��ʾ��ɫ 
		#endif
		}		
		break;
		
	case 90:
		offset += 2*w*(h-1);
	
		if((x+h)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = h;
		
		if((y+w)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = w;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < h)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			for(i=0;i<(readlen/2);i++,j++)
			{
				databuf[2*i] = color[(offset+2*((j/h)-w*(j%h)))];
				databuf[2*i+1] = color[(offset+2*((j/h)-w*(j%h)))+1];
			}

			if(show_w < h)
			{
				offset += 2;
				j = 0;
			}
			
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//��ʾ��ɫ 
		#endif
		}	
		break;
		
	case 180:
		offset += 2*(w*h-1);
		
		if((x+w)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = w;
		
		if((y+h)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = h;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < w)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			for(i=0;i<(readlen/2);i++,j++)
			{
				databuf[2*i] = color[(offset-2*j)];
				databuf[2*i+1] = color[(offset-2*j)+1];
			}

			if(show_w < w)
			{
				offset -= 2*w;
				j = 0;
			}
			
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}
		break;
		
	case 270:
		offset += 2*(w-1);
			
		if((x+h)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = h;
		
		if((y+w)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = w;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < h)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			for(i=0;i<(readlen/2);i++,j++)
			{
				databuf[2*i] = color[(offset-2*(j/h)+2*w*(j%h))];
				databuf[2*i+1] = color[(offset-2*(j/h)+2*w*(j%h))+1];
			}

			if(show_w < w)
			{
				offset -= 2;
				j = 0;
			}
			
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}
		break;
	}
}

//ָ��λ����ת�Ƕ���ʾͼƬ,����ɫ����
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
//rotate:��ת�Ƕ�,0,90,180,270,
void LCD_dis_pic_trans_rotate(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans, unsigned int rotate)
{
	uint16_t h,w,show_w,show_h;
	uint32_t i,j=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	w=256*color[2]+color[3]; 			//��ȡͼƬ����
	h=256*color[4]+color[5];			//��ȡͼƬ�߶�

	switch(rotate)
	{
	case 0:
		if((x+w)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = w;
		
		if((y+h)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = h;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < w)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			memcpy(databuf, &color[offset], readlen);
			
			if(show_w < w)
				offset += 2*w;
			else
				offset += readlen;

			for(i=0;i<(readlen/2);i++)
			{
				if(trans == (256*databuf[2*i]+databuf[2*i+1]))
				{
					databuf[2*i] = BACK_COLOR>>8;
					databuf[2*i+1] = BACK_COLOR;
				}
			}
			
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//��ʾ��ɫ 
		#endif
		}		
		break;
		
	case 90:
		offset += 2*w*(h-1);
	
		if((x+h)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = h;
		
		if((y+w)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = w;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < h)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			for(i=0;i<(readlen/2);i++,j++)
			{
				databuf[2*i] = color[(offset+2*((j/h)-w*(j%h)))];
				databuf[2*i+1] = color[(offset+2*((j/h)-w*(j%h)))+1];
			}

			if(show_w < h)
			{
				offset += 2;
				j = 0;
			}

			for(i=0;i<(readlen/2);i++)
			{
				if(trans == (256*databuf[2*i]+databuf[2*i+1]))
				{
					databuf[2*i] = BACK_COLOR>>8;
					databuf[2*i+1] = BACK_COLOR;
				}
			}
					
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//��ʾ��ɫ 
		#endif
		}
		break;
		
	case 180:
		offset += 2*(w*h-1);
		
		if((x+w)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = w;
		
		if((y+h)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = h;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < w)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			for(i=0;i<(readlen/2);i++,j++)
			{
				databuf[2*i] = color[(offset-2*j)];
				databuf[2*i+1] = color[(offset-2*j)+1];
			}

			if(show_w < w)
			{
				offset -= 2*w;
				j = 0;
			}

			for(i=0;i<(readlen/2);i++)
			{
				if(trans == (256*databuf[2*i]+databuf[2*i+1]))
				{
					databuf[2*i] = BACK_COLOR>>8;
					databuf[2*i+1] = BACK_COLOR;
				}
			}	
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}		
		break;
		
	case 270:
		offset += 2*(w-1);
			
		if((x+h)>LCD_WIDTH)
			show_w = LCD_WIDTH-x;
		else
			show_w = h;
		
		if((y+w)>LCD_HEIGHT)
			show_h = LCD_HEIGHT-y;
		else
			show_h = w;

		BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��
		datelen = 2*show_w*show_h;
		if(show_w < h)
			readlen = 2*show_w;
		
		while(datelen)
		{
			if(datelen < readlen)
			{
				readlen = datelen;
				datelen = 0;
			}
			else
			{
				readlen = readlen;
				datelen -= readlen;
			}
			
			memset(databuf, 0, LCD_DATA_LEN);
			for(i=0;i<(readlen/2);i++,j++)
			{
				databuf[2*i] = color[(offset-2*(j/h)+2*w*(j%h))];
				databuf[2*i+1] = color[(offset-2*(j/h)+2*w*(j%h))+1];
			}

			if(show_w < w)
			{
				offset -= 2;
				j = 0;
			}

			for(i=0;i<(readlen/2);i++)
			{
				if(trans == (256*databuf[2*i]+databuf[2*i+1]))
				{
					databuf[2*i] = BACK_COLOR>>8;
					databuf[2*i+1] = BACK_COLOR;
				}
			}
					
		#ifdef LCD_TYPE_SPI
			DispData(readlen, databuf);
		#else
			for(i=0;i<(readlen/2);i++)
				WriteDispData(databuf[2*i],databuf[2*i+1]); //��ʾ��ɫ 
		#endif
		}		
		break;
	}
}

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//ָ��λ����ʾͼƬ
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
void LCD_dis_img(uint16_t x, uint16_t y, unsigned char *color)
{  
	uint16_t h,w,show_w,show_h;
	uint8_t i=0;
	uint8_t databuf[COL]={0};
	uint32_t offset=6,datelen,showlen=0,readlen=0;
	
	w=color[2]+256*color[3]; 			//��ȡͼƬ����
	h=color[4]+256*color[5];			//��ȡͼƬ�߶�

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	y = y/PAGE_MAX;
	BlockWrite(x,y,show_w,show_h);	//����ˢ��λ��

	datelen = show_w*(show_h/8+((show_h%8)?1:0));
	if(show_w < w)
		readlen = show_w;
	else
		readlen = w;
	
	while(datelen)
	{
		if(datelen < readlen)
		{
			readlen = datelen;
			datelen = 0;
		}
		else
		{
			readlen = readlen;
			datelen -= readlen;
		}
		
		memset(databuf, 0, COL);
		memcpy(databuf, &color[offset], readlen);
		DispData(readlen, databuf);

		if(show_w < w)
			offset += w;
		else
			offset += readlen;

		i++;
		if(((y%PAGE_MAX)+i)>(PAGE_MAX-1))
			return;
			
		BlockWrite(x,y+i,show_w,show_h);	//����ˢ��λ��
	}
}
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

//ָ��λ����ʾͼƬ
//color:ͼƬ����ָ��
//x:ͼƬ��ʾX����
//y:ͼƬ��ʾY����
void LCD_ShowImg(uint16_t x, uint16_t y, unsigned char *color)
{
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_dis_img(x, y, color);
#else
	LCD_dis_pic(x, y, color);
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/
}

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//��ָ��������������ʾ��Ӣ���ַ���
//x,y:�������
//width,height:������(height<PAGE_MAX) 
//*p:�ַ�����ʼ��ַ	
void LCD_ShowStrInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *p)
{
	uint8_t x0=x;
	uint16_t phz=0;
	uint8_t y_offset = (system_font/8);
	
	width+=x;

	while(*p)
	{       
		if(x>=width){x=x0;y+=y_offset;}
		if(*p=='\n'){x=x0;y+=y_offset;p++;}
		if(y>=height)break;//�˳�
		if(*p==0x00)break;//�˳�
		if(*p<0x80)
		{
		#ifdef IMG_FONT_FROM_FLASH
			LCD_ShowEn_from_flash(x,y,*p);
		#else
			LCD_ShowEn(x,y,*p);
		#endif
			x+=system_font/2;
			p++;
		}
		else if(*(p+1))
		{
			phz = *p<<8;
			phz += *(p+1);
		#ifdef IMG_FONT_FROM_FLASH
			LCD_ShowCn_from_flash(x,y,phz);
		#else
			LCD_ShowCn(x,y,phz);
		#endif
			x+=system_font;
			p+=2;
		}        
	}
}

#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

//��ָ��������������ʾ��Ӣ���ַ���
//x,y:�������
//width,height:������ 
//*p:�ַ�����ʼ��ַ	
void LCD_ShowStringInRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t *p)
{
	uint8_t x0=x;
	uint16_t phz=0;

	width+=x;
	height+=y;
	while(*p)
	{       
		if(x>=width){x=x0;y+=system_font;}
		if(*p=='\n'){x=x0;y+=system_font;p++;}
		if(y>=height)break;//�˳�
		if(*p==0x00)break;//�˳�
		if(*p<0x80)
		{
		#ifdef IMG_FONT_FROM_FLASH
		  #ifdef FONTMAKER_UNICODE_FONT

		  #elif defined(FONTMAKER_MBCS_FONT)

		  #else	
			LCD_ShowChar_from_flash(x,y,*p,0);
		  #endif
		#else
			LCD_ShowChar(x,y,*p,0);
		#endif
			x+=system_font/2;
			p++;
		}
		else if(*(p+1))
		{
			phz = *p<<8;
			phz += *(p+1);
		#ifdef IMG_FONT_FROM_FLASH
		  #ifdef FONTMAKER_UNICODE_FONT

		  #elif defined(FONTMAKER_MBCS_FONT)

		  #else
			LCD_ShowChineseChar_from_flash(x,y,phz,0);
		  #endif
		#else
			LCD_ShowChineseChar(x,y,phz,0);
		#endif
			x+=system_font;
			p+=2;
		}        
	}
}

//��ʾ��Ӣ���ַ���
//x,y:�������
//*p:�ַ�����ʼ��ַ	
void LCD_ShowString(uint16_t x,uint16_t y,uint8_t *p)
{
	uint8_t x0=x;
	uint16_t width,phz=0;

	while(*p)
	{       
		if(x>=LCD_WIDTH)break;//�˳�
		if(y>=LCD_HEIGHT)break;//�˳�
		if(*p<0x80)
		{
		#ifdef IMG_FONT_FROM_FLASH
		  #ifdef FONTMAKER_UNICODE_FONT
		  	width = LCD_Show_Uni_Char_from_flash(x,y,*p,0);
		  	x += width;
		  #elif defined(FONTMAKER_MBCS_FONT)
			width = LCD_Show_Mbcs_Char_from_flash(x,y,*p,0);
		  	x += width;
		  #else
		   #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowEn_from_flash(x,y,*p);
		   #else
			LCD_ShowChar_from_flash(x,y,*p,0);
		   #endif
		  	x += system_font/2;
		  #endif
		#else
		  #ifdef FONTMAKER_MBCS_FONT
			width = LCD_Show_Mbcs_Char(x,y,*p,0);
		  	x += width;
		  #else
		   #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowEn(x,y,*p);
		   #else
			LCD_ShowChar(x,y,*p,0);
		   #endif
		  	x += system_font/2;
		  #endif
		#endif
			
			p++;
		}
		else if(*(p+1))
		{
			phz = *p<<8;
			phz += *(p+1);
		#ifdef IMG_FONT_FROM_FLASH
		  #ifdef FONTMAKER_UNICODE_FONT
		  
		  #elif defined(FONTMAKER_MBCS_FONT)
			LCD_Show_Mbcs_CJK_Char_from_flash(x,y,phz,0);
		  #else
		   #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowCn_from_flash(x,y,phz);
		   #else
			LCD_ShowChineseChar_from_flash(x,y,phz,0);
		   #endif
		  #endif
		#else
		  #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowCn(x,y,phz);
		  #else
			LCD_ShowChineseChar(x,y,phz,0);
		  #endif
		#endif
			x+=system_font;
			p+=2;
		}        
	}
}

#ifdef FONTMAKER_UNICODE_FONT
void LCD_ShowUniStringInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *p)
{
	uint8_t x0=x;
	uint16_t end=0x000d;
	
	width+=x;
	height+=y;
	while(*p)
	{       
		if(x>=width){x=x0;y+=system_font;}
		if(*p==end){x=x0;y+=system_font;p++;}
		if(y>=height)break;//�˳�
		if(*p==0x0000)break;//�˳�

		width = LCD_Show_Uni_Char_from_flash(x,y,*p,0);
		x += width;
		p++;
	}
}

//��ʾ��Ӣ���ַ���
//x,y:�������
//*p:�ַ�����ʼ��ַ	
void LCD_ShowUniString(uint16_t x, uint16_t y, uint16_t *p)
{
	uint8_t x0=x;
	uint8_t width;

	while(*p)
	{       
		if(x>=LCD_WIDTH)break;//�˳�
		if(y>=LCD_HEIGHT)break;//�˳�

		width = LCD_Show_Uni_Char_from_flash(x,y,*p,0);
		x += width;
		p++;
	}
}
#endif/*FONTMAKER_UNICODE_FONT*/

//m^n����
//����ֵ:m^n�η�.
uint32_t LCD_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}

//��ʾ����,��λΪ0,����ʾ
//x,y :������� 
//len :���ֵ�λ��
//color:��ɫ 
//num:��ֵ(0~4294967295);	 
void LCD_ShowNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
			#ifdef IMG_FONT_FROM_FLASH
				LCD_ShowChar_from_flash(x+(system_font/2)*t,y,' ',0);
			#else
				LCD_ShowChar(x+(system_font/2)*t,y,' ',0);
			#endif
				continue;
			}
			else 
				enshow=1; 
		 	 
		}
	#ifdef IMG_FONT_FROM_FLASH
		LCD_ShowChar_from_flash(x+(system_font/2)*t,y,temp+'0',0);
	#else
	 	LCD_ShowChar(x+(system_font/2)*t,y,temp+'0',0); 
	#endif
	}
}

//��ʾ����,��λΪ0,������ʾ
//x,y:�������
//num:��ֵ(0~999999999);	 
//len:����(��Ҫ��ʾ��λ��)
//mode:
//[7]:0,�����1,���.
//[6:1]:����
//[0]:0,�ǵ�����ʾ;1,������ʾ.
void LCD_ShowxNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t mode)
{  
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				if(mode&0X80)
				{
				#ifdef IMG_FONT_FROM_FLASH
					LCD_ShowChar_from_flash(x+(system_font/2)*t,y,'0',mode&0X01);
				#else
					LCD_ShowChar(x+(system_font/2)*t,y,'0',mode&0X01);
				#endif
				}
				else 
				{
				#ifdef IMG_FONT_FROM_FLASH
					LCD_ShowChar_from_flash(x+(system_font/2)*t,y,' ',mode&0X01);
				#else
					LCD_ShowChar(x+(system_font/2)*t,y,' ',mode&0X01); 
				#endif
				}
				
 				continue;
			}
			else 
				enshow=1; 
		}
	#ifdef IMG_FONT_FROM_FLASH
		LCD_ShowChar_from_flash(x+(system_font/2)*t,y,temp+'0',mode&0X01);
	#else
	 	LCD_ShowChar(x+(system_font/2)*t,y,temp+'0',mode&0X01);
	#endif
	}
} 

#ifdef FONTMAKER_UNICODE_FONT
//������������ַ��Ŀ���
//word:unicode�ַ�
uint8_t LCD_Measure_Uni_Byte(uint16_t word)
{
	uint8_t width,*ptr_font;
	uint8_t fontbuf[4] = {0};	
	uint32_t i,index_addr,font_addr=0;

#ifdef IMG_FONT_FROM_FLASH
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			font_addr = FONT_EN_UNI_16_ADDR;
			break;
	#endif
	#ifdef FONT_20
		case FONT_SIZE_20:
			font_addr = FONT_EN_UNI_20_ADDR;
			break;
	#endif
	#ifdef FONT_28
		case FONT_SIZE_28:
			font_addr = FONT_EN_UNI_28_ADDR;
			break;
	#endif
	#ifdef FONT_36
		case FONT_SIZE_36:
			font_addr = FONT_EN_UNI_36_ADDR;
			break;
	#endif
	#ifdef FONT_52
		case FONT_SIZE_52:
			font_addr = FONT_EN_UNI_52_ADDR;
			break;
	#endif
	#ifdef FONT_68
		case FONT_SIZE_68:
			font_addr = FONT_EN_UNI_68_ADDR;
			break;
	#endif
	
		default:
			return;
	}

	//read head data
	SpiFlash_Read((uint8_t*)&uni_infor.head, font_addr, FONT_UNI_HEAD_LEN);
	if((uni_infor.head.id[0] != FONT_UNI_HEAD_FLAG_0)
		||(uni_infor.head.id[1] != FONT_UNI_HEAD_FLAG_1)
		||(uni_infor.head.id[2] != FONT_UNI_HEAD_FLAG_2))
	{
		return;
	}
	
	//read sect data
	SpiFlash_Read((uint8_t*)&uni_infor.sect, font_addr+FONT_UNI_HEAD_LEN, uni_infor.head.sect_num*FONT_UNI_SECT_LEN);
	
	//read index data
	for(i=0;i<uni_infor.head.sect_num;i++)
	{
		if((word>=uni_infor.sect[i].first_char)&&((word<=uni_infor.sect[i].last_char)))
		{
			index_addr = (word-uni_infor.sect[i].first_char)*4+uni_infor.sect[i].index_addr;
			break;
		}
	}
	
	SpiFlash_Read(fontbuf, font_addr+index_addr, 4);
	width = fontbuf[3]>>2;
#else
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			ptr_font=uni_16_rm; 	 	//����1608����
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			ptr_font=uni_24_rm;			//����2412����
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			ptr_font=uni_32_rm;			//����3216����
			break;
	#endif
		default:
			return 0;							//û�е��ֿ�
	}

	//read head data
	memcpy((uint8_t*)&uni_infor.head, ptr_font, FONT_UNI_HEAD_LEN);
	if((uni_infor.head.id[0] != FONT_UNI_HEAD_FLAG_0)
		||(uni_infor.head.id[1] != FONT_UNI_HEAD_FLAG_1)
		||(uni_infor.head.id[2] != FONT_UNI_HEAD_FLAG_2))
	{
		return;
	}
	
	//read sect data
	memcpy((uint8_t*)&uni_infor.sect, ptr_font[FONT_UNI_HEAD_LEN], uni_infor.head.sect_num*FONT_UNI_SECT_LEN);
	
	//read index data
	for(i=0;i<uni_infor.head.sect_num;i++)
	{
		if((word>=uni_infor.sect[i].first_char)&&((word<=uni_infor.sect[i].last_char)))
		{
			index_addr = (word-uni_infor.sect[i].first_char)*4+uni_infor.sect[i].index_addr;
			break;
		}
	}
	
	width = ptr_font[index_addr+3]>>2;
#endif

	return width;
}

//������������ַ����ĳ��Ⱥ͸߶�
//p:�ַ���ָ��
//width,height:���ص��ַ������Ⱥ͸߶ȱ�����ַ
void LCD_MeasureUniString(uint16_t *p, uint16_t *width, uint16_t *height)
{
	uint8_t font_size;

	*width = 0;
	*height = 0;

	if(p == NULL)
		return;

	(*height) = system_font;

	while(*p)
	{
		(*width) += LCD_Measure_Uni_Byte(*p);
		p++;
	}  
}

#elif defined(FONTMAKER_MBCS_FONT)
//������������ַ��Ŀ���
//byte:�ַ�
uint8_t LCD_Measure_Mbcs_Byte(uint8_t byte)
{
	uint8_t width,*ptr_font;
	uint8_t fontbuf[4] = {0};	
	uint32_t index_addr,font_addr=0;

#ifdef IMG_FONT_FROM_FLASH
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			font_addr = FONT_RM_ASC_16_ADDR;
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			font_addr = FONT_RM_ASC_24_ADDR;
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			font_addr = FONT_RM_ASC_32_ADDR;
			break;
	#endif
		default:
			return;
	}

	index_addr = FONT_MBCS_HEAD_LEN+4*byte;
	SpiFlash_Read(fontbuf, font_addr+index_addr, 4);
	width = fontbuf[3]>>2;
#else
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			ptr_font=asc2_16_rm; 	 	//����1608����
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			ptr_font=asc2_24_rm;			//����2412����
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			ptr_font=asc2_32_rm;			//����3216����
			break;
	#endif
		default:
			return 0;							//û�е��ֿ�
	}
	
	index_addr = FONT_MBCS_HEAD_LEN+4*byte;
	width = ptr_font[index_addr+3]>>2;
#endif

	return width;
}
#endif

//������������ַ����ĳ��Ⱥ͸߶�
//p:�ַ���ָ��
//width,height:���ص��ַ������Ⱥ͸߶ȱ�����ַ
void LCD_MeasureString(uint8_t *p, uint16_t *width,uint16_t *height)
{
	uint8_t font_size;

	*width = 0;
	*height = 0;

	if(p == NULL || strlen((const char *)p) == 0)
		return;

	(*height) = system_font;

	while(*p)
	{
		if(*p<0x80)
		{
		#ifdef FONTMAKER_UNICODE_FONT
			(*width) += LCD_Measure_Uni_Byte(*p);
		#elif defined(FONTMAKER_MBCS_FONT)
			(*width) += LCD_Measure_Mbcs_Byte(*p);
		#else
			(*width) += system_font/2;
		#endif
			p++;
		}
		else if(*(p+1))
		{
			(*width) += system_font;
			p += 2;
		}        
	}  
}

//�ָ����廭��Ĭ����ɫ
void LCD_ReSetFontColor(void)
{
	POINT_COLOR = WHITE;
}

//�������廭����ɫ
void LCD_SetFontColor(uint16_t color)
{
	POINT_COLOR = color;
}

//�ָ����屳��Ĭ����ɫ
void LCD_ReSetFontBgColor(void)
{
	BACK_COLOR = BLACK;
}

//�������屳����ɫ
void LCD_SetFontBgColor(uint16_t color)
{
	BACK_COLOR = color;
}

//����ϵͳ����
//font_size:ö��������
void LCD_SetFontSize(uint8_t font_size)
{
	if(font_size > FONT_SIZE_MIN && font_size < FONT_SIZE_MAX)
		system_font = font_size;
}

void LCDMsgProcess(void)
{
	if(lcd_sleep_in)
	{
		LCD_BL_Off();
		LCD_SleepIn();
		lcd_sleep_in = false;
	}

	if(lcd_sleep_out)
	{	
		LCD_SleepOut();
		pmu_alert_proc();
		if(IsInIdleScreen())
		{
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TIME|SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK|SCREEN_EVENT_UPDATE_SIG|SCREEN_EVENT_UPDATE_NET_MODE|SCREEN_EVENT_UPDATE_BAT;
		}
		
		LCD_BL_On();
		lcd_sleep_out = false;
	}
}
