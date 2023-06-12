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
#include "gps.h"
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

//LCD屏幕的高度和宽度
uint16_t LCD_WIDTH = COL;
uint16_t LCD_HEIGHT = ROW;

//LCD的画笔颜色和背景色	   
uint16_t POINT_COLOR=WHITE;	//画笔颜色
uint16_t BACK_COLOR=BLACK;  //背景色 

//默认字体大小
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

#ifdef FONTMAKER_UNICODE_FONT
font_uni_infor uni_infor = {0};
#endif

//快速画点
//x,y:坐标
//color:颜色
void LCD_Fast_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{	   
	BlockWrite(x,y,1,1);	//定坐标
	WriteOneDot(color);				//画点函数	
}	 

//在指定区域内填充单个颜色
//(x,y),(w,h):填充矩形对角坐标,区域大小为:w*h   
//color:要填充的颜色
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
		WriteOneDot(color); //显示颜色 
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
		WriteOneDot(color); //显示颜色 
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

//在指定区域内填充指定颜色块	(显示图片)		 
//(x,y),(w,h):填充矩形对角坐标,区域大小为:w*h   
//color:要填充的颜色
void LCD_Pic_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, unsigned char *color)
{  
	uint16_t high,width;
	uint16_t i,j;
	uint8_t databuf[2*COL] = {0};
	
	width=256*color[2]+color[3]; 			//获取图片宽度
	high=256*color[4]+color[5];			//获取图片高度
	
 	for(i=0;i<h;i++)
	{
		BlockWrite(x,y+i,w,1);	  	//设置刷新位置

	#ifdef LCD_TYPE_SPI
		for(j=0;j<w;j++)
		{
			databuf[2*j] = color[8+2*(i*width+j)];
			databuf[2*j+1] = color[8+2*(i*width+j)+1];
		}

		DispData(2*w, databuf);
	#else
		for(j=0;j<w;j++)
			WriteDispData(color[8+2*(i*width+j)],color[8+2*(i*width+j)+1]);	//显示颜色 
	#endif
	}			
} 

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol;
	
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_Fast_DrawPoint(uRow,uCol,POINT_COLOR);//画点 
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

//画矩形	  
//(x1,y1),(x2,y2):矩形的对角坐标
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

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //判断下个点位置的标志
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
		//使用Bresenham算法画圆     
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
	uint8_t cbyte=0;		//行扫描，每个字符每一行占用的字节数
	uint16_t csize=0;		//得到字体一个字符对应点阵集所占的字节数	
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
			return; 						//没有的字库
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
	BlockWrite(x,y,w,h);	//设置刷新位置
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
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;

			}
			if((x-x0)==cbyte)
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //超区域了
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;				
			}
			if((x-x0)==cbyte)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
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
* Function:显示fontmaker工具生成的bin格式的英文变宽点阵字库
* Description:
* 	检索表: 
* 	从00000010h 开始，每 4 个字节表示一个字符的检索信息， 且从字符 0x0 开始。故空格字符（' '，编码为 0x20）的检索信息
* 	（文件头的长度+字符编码*4 = 0x10 + 0x20 *4 = 0x90， 即 000000090h）为：10 04 00 10，即得出一个 32 位数为： 
* 	0x10000410（十六进制） --- （00010000 00000000 00000100 00010000）. 
* 	高 6 位，表示当前字符的宽度。 故得出 000100 -- 4 （字库宽度为 4 ）
* 	低 26 位，表示当期字符的点阵数据的偏移地址。故得出 00 00000000 00000100 00010000 -- 0x410 （点阵信息的起始地址为 0x410) 
* 
* 	点阵数据 
* 	由于空格字符的起始地址为 0x410，且数据长度为：（（字体宽度+7）/8）* 字体高度 = ((4+7)/8)*16 = 16. 
* 	故取如下 16 字节，即为空格字符的点阵数据。
*********************************************************************************************************************/
uint8_t LCD_Show_Mbcs_Char_from_flash(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
	uint8_t temp,t1,t;
	uint16_t y0=y,x0=x,w,h;
	uint8_t cbyte=0;		//行扫描，每个字符每一行占用的字节数(英文宽度是字宽的一半)
	uint16_t csize=0;		//得到字体一个字符对应点阵集所占的字节数	
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
			return; 						//没有的字库
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
	BlockWrite(x,y,w,h);	//设置刷新位置
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
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;
			}
			if((x-x0)==cbyte)
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; //超区域了
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;				
			}
			if((x-x0)==cbyte)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}
		#endif
		}
	}

	return cbyte;
}

/*********************************************************************************************************************
* Name:LCD_Show_MBCS_CJK_Char_from_flash
* Function:显示fontmaker工具生成的bin格式的CJK等宽点阵字库
* Description:
* 除去文件头 16 Byte 外，其它数据都是纯字符点阵数据。 
* 因为 GB2312（简体中文） 的首个字符为：0xA1A1， 它的点阵数据起始地址为 0x10（除去文件头），
* 数据长度为：（（字体高度+7）/8）* 字体高度 = ((16+7)/8)*16=32 
* 故从 0x10 开始连续取 32 字节，即为字符 0xA1A1 的点阵数据。
* 
* 	点阵数据 
* 	由于空格字符的起始地址为 0x410，且数据长度为：（（字体宽度+7）/8）* 字体高度 = ((4+7)/8)*16 = 16. 
* 	故取如下 16 字节，即为空格字符的点阵数据。
*********************************************************************************************************************/
uint8_t LCD_Show_Mbcs_CJK_Char_from_flash(uint16_t x, uint16_t y, uint16_t num, uint8_t mode)
{
	uint8_t temp,t1,t;
	uint16_t x0=x,y0=y,w,h;
	uint8_t cbyte=0;					//行扫描，每个字符每一行占用的字节数
	uint16_t csize=0;					//得到字体一个字符对应点阵集所占的字节数
	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[256] = {0};
	uint32_t i=0,index,font_addr,data_addr=0;
	uint8_t R_code=0xFF&(num>>8);		//区码
	uint8_t C_code=0xFF&num;			//位码
	
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
			return; 						//没有的字库
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
	BlockWrite(x,y,w,h);	//设置刷新位置
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
			if(x>=LCD_WIDTH)							//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;							//超区域了
				t=t+(cbyte-(t%cbyte))-1;				//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;
			}
			if((x-x0)==(system_font))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;							//超区域了
				break;
			}			
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)							//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;				//超区域了
				t=t+(cbyte-(t%cbyte))-1;				//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;			
			}
			if((x-x0)==system_font)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;				//超区域了
				break;
			}
		#endif
		} 
	} 

	return cbyte;
}
#else
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//在指定位置显示flash中一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowEn_from_flash(uint16_t x,uint16_t y,uint8_t num)
{
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);		//列行扫描，每个字符每一列占用的字节数(英文宽度是字宽的一半)
	uint16_t csize=cbyte*(system_font/2);					//得到字体一个字符对应点阵集所占的字节数	
	uint8_t fontbuf[1024] = {0};
 	uint8_t databuf[128] = {0};
	uint32_t i=0;
	
	num=num-' ';	//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
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

//在指定位置显示flash中一个中文字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowCn_from_flash(uint16_t x, uint16_t y, uint16_t num)
{  							  
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);		//列行扫描，每个字符每一列占用的字节数(英文宽度是字宽的一半)
	uint16_t csize=cbyte*system_font;						//得到字体一个字符对应点阵集所占的字节数	
	uint8_t fontbuf[2*1024] = {0};
 	uint8_t databuf[256] = {0};
	uint16_t index=0;	
	uint32_t i=0;
	
	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(区码-1)+(位码-1))*32
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

//在指定位置显示flash中一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar_from_flash(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
	uint8_t temp,t1,t;
	uint8_t cbyte=(system_font/2)/8+(((system_font/2)%8)?1:0);		//行扫描，每个字符每一行占用的字节数(英文宽度是字宽的一半)
	uint16_t csize=cbyte*system_font;		//得到字体一个字符对应点阵集所占的字节数	
 	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[256] = {0};
	uint16_t y0=y,x0=x,w=(system_font/2),h=system_font;
	uint32_t i=0;
	
	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
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
			return;							//没有的字库
	}

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h); 	//设置刷新位置
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
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;

			}
			if((x-x0)==(system_font/2))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;				
			}
			if((x-x0)==(system_font/2))
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; 	//超区域了
				break;
			}
		#endif
		}
	}
}    

//在指定位置显示flash中一个中文字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChineseChar_from_flash(uint16_t x,uint16_t y,uint16_t num,uint8_t mode)
{  							  
	uint8_t temp,t1,t;
	uint16_t x0=x,y0=y,w=system_font,h=system_font;
	uint16_t index=0;
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);		//行扫描，每个字符每一行占用的字节数
	uint16_t csize=cbyte*(system_font);						//得到字体一个字符对应点阵集所占的字节数	
	uint8_t databuf[2*1024] = {0};
	uint8_t fontbuf[256] = {0};
	uint32_t i=0;
	
	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(区码-1)+(位码-1))*32
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
			return;								//没有的字库
	}	

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h); 	//设置刷新位置
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
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;
			}
			if((x-x0)==(system_font))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}			
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;			
			}
			if((x-x0)==system_font)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}
		#endif
		} 
	} 
} 

#endif/*FONTMAKER_UNICODE_FONT*/

//获取flash中的图片尺寸
//color:图片数据指针
//width:获取到的图片宽度输出地址
//height:获取到的图片高度输出地址
void LCD_get_pic_size_from_flash(uint32_t pic_addr, uint16_t *width, uint16_t *height)
{
	uint8_t databuf[6] = {0};

	SpiFlash_Read(databuf, pic_addr, 6);
	*width = 256*databuf[2]+databuf[3]; 			//获取图片宽度
	*height = 256*databuf[4]+databuf[5];			//获取图片高度
}

//指定位置显示flash中的图片
//pic_addr:图片在flash中的地址
//x:图片显示X坐标
//y:图片显示Y坐标
void LCD_dis_pic_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr)
{
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w=256*databuf[2]+databuf[3]; 			//获取图片宽度
	h=256*databuf[4]+databuf[5];			//获取图片高度

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
	
	BlockWrite(x,y,show_w,show_h);	//设置刷新位置

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
			WriteDispData(databuf[2*i],databuf[2*i+1]);	//显示颜色 
	#endif
	}
}

//指定位置显示flash中的图片,带颜色过滤
//x:图片显示X坐标
//y:图片显示Y坐标
//pic_addr:图片在flash中的地址
//trans:需要过滤的颜色
void LCD_dis_pic_trans_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr, uint16_t trans)
{
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w=256*databuf[2]+databuf[3]; 			//获取图片宽度
	h=256*databuf[4]+databuf[5];			//获取图片高度

	pic_addr += 8;

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;
	
	BlockWrite(x,y,show_w,show_h);	//设置刷新位置

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
			WriteDispData(databuf[2*i],databuf[2*i+1]);	//显示颜色 
	#endif
	}
}

//指定位置旋转特定角度显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
//rotate:旋转角度,0,90,180,270,
void LCD_dis_pic_rotate_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr, unsigned int rotate)
{
	uint16_t h,w,show_w,show_h;
	uint32_t i,j=0,k=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w=256*databuf[2]+databuf[3]; 			//获取图片宽度
	h=256*databuf[4]+databuf[5];			//获取图片高度

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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
		#endif
		}
		break;
	}
}

//指定中心位置任意角度显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
//angle:角度,0~360,
void LCD_dis_pic_angle_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr, unsigned int angle)
{
	uint16_t c_x,c_y,c_r,h,w,show_w,show_h;
	int16_t offset_x,offset_y;
	int32_t i,j=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	SpiFlash_Read(databuf, pic_addr, 8);

	w = 256*databuf[2]+databuf[3]; 			//获取图片宽度
	h = 256*databuf[4]+databuf[5];			//获取图片高度
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
				BlockWrite(c_x+offset_x, c_y-offset_y, 1, 1);	//设置刷新位置
				j++;
			}
			
			WriteDispData(databuf[2*i], databuf[2*i+1]); //显示颜色 
		}
	}		
}

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//指定位置显示flash中的图片
//pic_addr:图片在flash中的地址
//x:图片显示X坐标
//y:图片显示Y坐标
void LCD_dis_img_from_flash(uint16_t x, uint16_t y, uint32_t pic_addr)
{
	uint16_t h,w,show_w,show_h;
	uint8_t i=0;
	uint8_t databuf[COL]={0};
	uint32_t offset=6,datelen,showlen=0,readlen=COL;
	
	SpiFlash_Read(databuf, pic_addr, 8);
	w=databuf[2]+256*databuf[3]; 			//获取图片宽度
	h=databuf[4]+256*databuf[5];			//获取图片高度

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

	BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
			
		BlockWrite(x,y+i,show_w,show_h);	//设置刷新位置
	}
}
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

//指定位置显示flash中的图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
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
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
void LCD_ShowEn(uint16_t x,uint16_t y,uint8_t num)
{
 	uint8_t i,databuf[128] = {0};
	
	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）

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

//在指定位置显示一个中文字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
void LCD_ShowCn(uint16_t x, uint16_t y, uint16_t num)
{  							  
	uint8_t i,databuf[128] = {0};
	uint16_t index=0;

	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(区码-1)+(位码-1))*32
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

//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
    uint8_t temp,t1,t,i=0;
	uint16_t y0=y,x0=x,w=(system_font/2),h=system_font;
	uint8_t cbyte=(system_font/2)/8+(((system_font/2)%8)?1:0);		//行扫描，每个字符每一行占用的字节数(英文宽度是字宽的一半)
	uint16_t csize=cbyte*system_font;		//得到字体一个字符对应点阵集所占的字节数	
	uint8_t fontbuf[256] = {0};	
 	uint8_t databuf[2*COL] = {0};

	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			memcpy(fontbuf, asc2_1608[num], csize);	//调用1608字体
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			memcpy(fontbuf, asc2_2412[num], csize);	//调用2412字体
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			memcpy(fontbuf, asc2_3216[num], csize);	//调用3216字体
			break;
	#endif
	#ifdef FONT_48
		case FONT_SIZE_48:
			memcpy(fontbuf, asc2_4824[num], csize);	//调用4824字体
			break;
	#endif
	#ifdef FONT_64
		case FONT_SIZE_64:
			memcpy(fontbuf, asc2_6432[num], csize); //调用6432字体
			break;
	#endif
		default:
			return;							//没有的字库
	}

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//设置刷新位置
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
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;

				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;

			}
			if((x-x0)==(system_font/2))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;				
			}
			if((x-x0)==(system_font/2))
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return; 	//超区域了
				break;
			}
		#endif
		}	
	}  	    	   	 	  
}

//在指定位置显示一个中文字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChineseChar(uint16_t x,uint16_t y,uint16_t num,uint8_t mode)
{  							  
	uint8_t temp,t1,t,i=0;
	uint16_t x0=x,y0=y,w=system_font,h=system_font;
	uint16_t index=0;
	uint8_t cbyte=system_font/8+((system_font%8)?1:0);				//行扫描，每个字符每一行占用的字节数
	uint16_t csize=cbyte*(system_font);								//得到字体一个字符对应点阵集所占的字节数	
	uint8_t fontbuf[256] = {0};
	uint8_t databuf[2*COL] = {0};

	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(区码-1)+(位码-1))*32
	switch(system_font)
	{
	#if 0	//def FONT_16
		case FONT_SIZE_16:
			memcpy(fontbuf, chinese_1616[index], csize);	//调用1608字体
			break;
	#endif
	#if 0	//def FONT_24
		case FONT_SIZE_24:
			memcpy(fontbuf, chinese_2424[index], csize);	//调用2424字体
			break;
	#endif
	#if 0	//def FONT_32
		case FONT_SIZE_32:
			memcpy(fontbuf, chinese_3232[index], csize);	//调用3232字体
			break;
	#endif
		default:
			return;								//没有的字库
	}	

#ifdef LCD_TYPE_SPI
	if((x+w)>=LCD_WIDTH)
		w = LCD_WIDTH - x;
	if((y+h)>=LCD_HEIGHT)
		h = LCD_HEIGHT - y;
	BlockWrite(x,y,w,h);	//设置刷新位置
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
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;

				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;
			}
			if((x-x0)==(system_font))
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}			
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;			
			}
			if((x-x0)==system_font)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}
		#endif
		} 
	}  	    	   	 	  
}  

#ifdef FONTMAKER_MBCS_FONT
/*********************************************************************************************************************
* Name:LCD_Show_MBCS_Char
* Function:显示fontmaker工具生成的bin格式的点阵字库
* Description:
* 	检索表: 
* 	从00000010h 开始，每 4 个字节表示一个字符的检索信息， 且从字符 0x0 开始。故空格字符（' '，编码为 0x20）的检索信息
* 	（文件头的长度+字符编码*4 = 0x10 + 0x20 *4 = 0x90， 即 000000090h）为：10 04 00 10，即得出一个 32 位数为： 
* 	0x10000410（十六进制） --- （00010000 00000000 00000100 00010000）. 
* 	高 6 位，表示当前字符的宽度。 故得出 000100 -- 4 （字库宽度为 4 ）
* 	低 26 位，表示当期字符的点阵数据的偏移地址。故得出 00 00000000 00000100 00010000 -- 0x410 （点阵信息的起始地址为 0x410) 
* 
* 	点阵数据 
* 	由于空格字符的起始地址为 0x410，且数据长度为：（（字体宽度+7）/8）* 字体高度 = ((4+7)/8)*16 = 16. 
* 	故取如下 16 字节，即为空格字符的点阵数据。
*********************************************************************************************************************/
uint8_t LCD_Show_Mbcs_Char(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
	uint8_t temp,t1,t,i=0,*ptr_font;
	uint16_t y0=y,x0=x,w,h;
	uint8_t cbyte=0;		//行扫描，每个字符每一行占用的字节数(英文宽度是字宽的一半)
	uint16_t csize=0;		//得到字体一个字符对应点阵集所占的字节数	
 	uint8_t databuf[2*COL] = {0};
	uint32_t index_addr,data_addr=0;

	switch(system_font)
	{
	#ifdef FONT_16
		case FONT_SIZE_16:
			ptr_font=asc2_16_rm; 	 		//调用1608字体
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			ptr_font=asc2_24_rm;			//调用2412字体
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			ptr_font=asc2_32_rm;			//调用3216字体
			break;
	#endif
		default:
			return 0;						//没有的字库
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
	BlockWrite(x,y,w,h);	//设置刷新位置
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
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				DispData(2*i, databuf);
				i=0;

				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return cbyte;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;

			}
			if((x-x0)==cbyte)
			{
				DispData(2*i, databuf);
				i=0;
				
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return cbyte;	//超区域了
				break;
			}
		#else
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			x++;
			if(x>=LCD_WIDTH)				//超出行区域，直接显示下一行
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				t=t+(cbyte-(t%cbyte))-1;	//获取下一行对应的字节，注意for循环会增加1，所以这里先提前减去1
				break;				
			}
			if((x-x0)==cbyte)
			{
				x=x0;
				y++;
				if(y>=LCD_HEIGHT)return;	//超区域了
				break;
			}
		#endif
		}	
	}  	 

	return cbyte;
}
#endif/*FONTMAKER_MBCS_FONT*/

//获取图片尺寸
//color:图片数据指针
//width:获取到的图片宽度输出地址
//height:获取到的图片高度输出地址
void LCD_get_pic_size(unsigned char *color, uint16_t *width, uint16_t *height)
{
	*width = 256*color[2]+color[3];
	*height = 256*color[4]+color[5];
}

//指定位置显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
void LCD_dis_pic(uint16_t x, uint16_t y, unsigned char *color)
{  
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	w=256*color[2]+color[3]; 			//获取图片宽度
	h=256*color[4]+color[5];			//获取图片高度

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
			WriteDispData(databuf[2*i],databuf[2*i+1]);	//显示颜色 
	#endif
	}
}

//指定位置显示图片,带颜色过滤
//x:图片显示X坐标
//y:图片显示Y坐标
//color:图片数据指针
//trans:需要过滤的颜色
void LCD_dis_pic_trans(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans)
{  
	uint16_t h,w,show_w,show_h;
	uint16_t i;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;

	w=256*color[2]+color[3]; 			//获取图片宽度
	h=256*color[4]+color[5];			//获取图片高度

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;

	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
			WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
	#endif
	}
}


//指定位置旋转特定角度显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
//rotate:旋转角度,0,90,180,270,
void LCD_dis_pic_rotate(uint16_t x, uint16_t y, unsigned char *color, unsigned int rotate)
{
	uint16_t h,w,show_w,show_h;
	uint32_t i,j=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	w=256*color[2]+color[3]; 			//获取图片宽度
	h=256*color[4]+color[5];			//获取图片高度

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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
		#endif
		}
		break;
	}
}

//指定位置旋转角度显示图片,带颜色过滤
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
//rotate:旋转角度,0,90,180,270,
void LCD_dis_pic_trans_rotate(uint16_t x, uint16_t y, unsigned char *color, uint16_t trans, unsigned int rotate)
{
	uint16_t h,w,show_w,show_h;
	uint32_t i,j=0;
	uint8_t databuf[LCD_DATA_LEN]={0};
	uint32_t offset=8,datelen,showlen=0,readlen=LCD_DATA_LEN;
	
	w=256*color[2]+color[3]; 			//获取图片宽度
	h=256*color[4]+color[5];			//获取图片高度

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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]);	//显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
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

		BlockWrite(x,y,show_w,show_h);	//设置刷新位置
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
				WriteDispData(databuf[2*i],databuf[2*i+1]); //显示颜色 
		#endif
		}		
		break;
	}
}

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//指定位置显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
void LCD_dis_img(uint16_t x, uint16_t y, unsigned char *color)
{  
	uint16_t h,w,show_w,show_h;
	uint8_t i=0;
	uint8_t databuf[COL]={0};
	uint32_t offset=6,datelen,showlen=0,readlen=0;
	
	w=color[2]+256*color[3]; 			//获取图片宽度
	h=color[4]+256*color[5];			//获取图片高度

	if((x+w)>LCD_WIDTH)
		show_w = LCD_WIDTH-x;
	else
		show_w = w;
	
	if((y+h)>LCD_HEIGHT)
		show_h = LCD_HEIGHT-y;
	else
		show_h = h;

	y = y/PAGE_MAX;
	BlockWrite(x,y,show_w,show_h);	//设置刷新位置

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
			
		BlockWrite(x,y+i,show_w,show_h);	//设置刷新位置
	}
}
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

#ifdef FONTMAKER_UNICODE_FONT
//根据字体测量字符的宽度
//word:unicode字符
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
			ptr_font=uni_16_rm; 	 	//调用1608字体
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			ptr_font=uni_24_rm;			//调用2412字体
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			ptr_font=uni_32_rm;			//调用3216字体
			break;
	#endif
		default:
			return 0;							//没有的字库
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

//根据字体测量字符串的长度和高度
//p:字符串指针
//width,height:返回的字符串宽度和高度变量地址
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
//根据字体测量字符的宽度
//byte:字符
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
			ptr_font=asc2_16_rm; 	 	//调用1608字体
			break;
	#endif
	#ifdef FONT_24
		case FONT_SIZE_24:
			ptr_font=asc2_24_rm;			//调用2412字体
			break;
	#endif
	#ifdef FONT_32
		case FONT_SIZE_32:
			ptr_font=asc2_32_rm;			//调用3216字体
			break;
	#endif
		default:
			return 0;							//没有的字库
	}
	
	index_addr = FONT_MBCS_HEAD_LEN+4*byte;
	width = ptr_font[index_addr+3]>>2;
#endif

	return width;
}
#endif

//指定位置显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
void LCD_ShowImg(uint16_t x, uint16_t y, unsigned char *color)
{
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_dis_img(x, y, color);
#else
	LCD_dis_pic(x, y, color);
#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/
}

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
//在指定矩形区域内显示中英文字符串
//x,y:起点坐标
//width,height:区域大小 (height<PAGE_MAX) 
//*p:字符串起始地址	
void LCD_ShowStrInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *p)
{
	uint16_t str_x=x,str_y=y,str_w,str_h;
	uint16_t phz=0;
	uint8_t y_offset = (system_font/8);
	
	str_w = width+x;
	str_h = height+y;

	while(*p)
	{       
		if(str_x>=str_w){str_x=x;str_y+=y_offset;}
		if(*p=='\n'){str_x=x;str_y+=y_offset;p++;}
		if(str_y>=str_h)break;//退出
		if(*p==0x00)break;//退出
		if(*p<0x80)
		{
		#ifdef IMG_FONT_FROM_FLASH
			LCD_ShowEn_from_flash(str_x,str_y,*p);
		#else
			LCD_ShowEn(str_x,str_y,*p);
		#endif
			str_x+=system_font/2;
			p++;
		}
		else if(*(p+1))
		{
			phz = *p<<8;
			phz += *(p+1);
		#ifdef IMG_FONT_FROM_FLASH
			LCD_ShowCn_from_flash(str_x,str_y,phz);
		#else
			LCD_ShowCn(str_x,str_y,phz);
		#endif
			str_x+=system_font;
			p+=2;
		}        
	}
}

#endif/*LCD_VGM068A4W01_SH1106G||LCD_VGM096064A6W01_SP5090*/

//在指定矩形区域内显示中英文字符串
//x,y:起点坐标
//width,height:区域大小  
//*p:字符串起始地址	
void LCD_ShowStringInRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t *p)
{
	uint16_t str_x=x,str_y=y,str_w,str_h;
	uint16_t w,phz=0;

	str_w = width+x;
	str_h = height+y;
	while(*p)
	{       
		if(str_x>=width){str_x=x;str_y+=system_font;}
		if(*p=='\n'){str_x=x;str_y+=system_font;p++;}
		if(str_y>=height)break;//退出
		if(*p==0x00)break;//退出
		if(*p<0x80)
		{
		#ifdef IMG_FONT_FROM_FLASH
		  #ifdef FONTMAKER_UNICODE_FONT
			w = LCD_Show_Uni_Char_from_flash(str_x,str_y,*p,0);
		  	str_x += w;
		  #elif defined(FONTMAKER_MBCS_FONT)

		  #else	
			LCD_ShowChar_from_flash(str_x,str_y,*p,0);
		  #endif
		#else
			LCD_ShowChar(str_x,str_y,*p,0);
		#endif
			str_x+=system_font/2;
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
			LCD_ShowChineseChar_from_flash(str_x,str_y,phz,0);
		  #endif
		#else
			LCD_ShowChineseChar(str_x,str_y,phz,0);
		#endif
			str_x+=system_font;
			p+=2;
		}        
	}
}

//显示中英文字符串
//x,y:起点坐标
//*p:字符串起始地址	
void LCD_ShowString(uint16_t x,uint16_t y,uint8_t *p)
{
	uint16_t str_x=x,str_y=y,str_w,str_h;
	uint8_t x0=x;
	uint16_t width,phz=0;

	while(*p)
	{       
		if(str_x>=LCD_WIDTH)break;//退出
		if(str_y>=LCD_HEIGHT)break;//退出
		if(*p<0x80)
		{
		#ifdef IMG_FONT_FROM_FLASH
		  #ifdef FONTMAKER_UNICODE_FONT
		  	width = LCD_Show_Uni_Char_from_flash(str_x,str_y,*p,0);
		  	str_x += width;
		  #elif defined(FONTMAKER_MBCS_FONT)
			width = LCD_Show_Mbcs_Char_from_flash(str_x,str_y,*p,0);
		  	str_x += width;
		  #else
		   #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowEn_from_flash(str_x,str_y,*p);
		   #else
			LCD_ShowChar_from_flash(str_x,str_y,*p,0);
		   #endif
		  	str_x += system_font/2;
		  #endif
		#else
		  #ifdef FONTMAKER_MBCS_FONT
			width = LCD_Show_Mbcs_Char(str_x,str_y,*p,0);
		  	str_x += width;
		  #else
		   #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowEn(str_x,str_y,*p);
		   #else
			LCD_ShowChar(str_x,str_y,*p,0);
		   #endif
		  	str_x += system_font/2;
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
			LCD_Show_Mbcs_CJK_Char_from_flash(str_x,str_y,phz,0);
		  #else
		   #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowCn_from_flash(str_x,str_y,phz);
		   #else
			LCD_ShowChineseChar_from_flash(str_x,str_y,phz,0);
		   #endif
		  #endif
		#else
		  #if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_ShowCn(str_x,str_y,phz);
		  #else
			LCD_ShowChineseChar(str_x,str_y,phz,0);
		  #endif
		#endif
			str_x+=system_font;
			p+=2;
		}        
	}
}

#ifdef FONTMAKER_UNICODE_FONT
void LCD_ShowUniStringInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *p)
{
	uint16_t str_x=x,str_y=y,str_w,str_h;
	uint8_t w=0;
	uint16_t end=0x000a;
	
	str_w = width+x;
	str_h = height+y;
	while(*p)
	{       
		if(str_x>=str_w){str_x=x;str_y+=system_font;}
		if(*p==end){str_x=x;str_y+=system_font;p++;}
		if(str_y>=str_h)break;//退出
		if(*p==0x0000)break;//退出
		w = LCD_Measure_Uni_Byte(*p);
		if((str_x+w)>=str_w){str_x=x;str_y+=system_font;}
		if(str_y>=str_h)break;//退出
		w = LCD_Show_Uni_Char_from_flash(str_x,str_y,*p,0);
		str_x += w;
		p++;
	}
}

//显示中英文字符串
//x,y:起点坐标
//*p:字符串起始地址	
void LCD_ShowUniString(uint16_t x, uint16_t y, uint16_t *p)
{
	uint16_t str_x=x,str_y=y;
	uint8_t width;

	while(*p)
	{       
		if(str_x>=LCD_WIDTH)break;//退出
		if(str_y>=LCD_HEIGHT)break;//退出

		width = LCD_Show_Uni_Char_from_flash(str_x,str_y,*p,0);
		str_x += width;
		p++;
	}
}
#endif/*FONTMAKER_UNICODE_FONT*/

#if 0
//m^n函数
//返回值:m^n次方.
uint32_t LCD_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}

//显示数字,高位为0,则不显示
//x,y :起点坐标	 
//len :数字的位数
//color:颜色 
//num:数值(0~4294967295);	 
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

//显示数字,高位为0,还是显示
//x,y:起点坐标
//num:数值(0~999999999);	 
//len:长度(即要显示的位数)
//mode:
//[7]:0,不填充;1,填充0.
//[6:1]:保留
//[0]:0,非叠加显示;1,叠加显示.
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
#endif

//根据字体测量字符串的长度和高度
//p:字符串指针
//width,height:返回的字符串宽度和高度变量地址
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

//恢复字体画笔默认颜色
void LCD_ReSetFontColor(void)
{
	POINT_COLOR = WHITE;
}

//设置字体画笔颜色
void LCD_SetFontColor(uint16_t color)
{
	POINT_COLOR = color;
}

//恢复字体背景默认颜色
void LCD_ReSetFontBgColor(void)
{
	BACK_COLOR = BLACK;
}

//设置字体背景颜色
void LCD_SetFontBgColor(uint16_t color)
{
	BACK_COLOR = color;
}

//设置系统字体
//font_size:枚举字体大小
void LCD_SetFontSize(uint8_t font_size)
{
	if(font_size > FONT_SIZE_MIN && font_size < FONT_SIZE_MAX)
		system_font = font_size;
}

void LCDMsgProcess(void)
{
	if(lcd_sleep_in)
	{
		lcd_sleep_in = false;

		if(LCD_Get_BL_Mode() != LCD_BL_ALWAYS_ON)
		{
			LCD_BL_Off();

			if((screen_id == SCREEN_ID_STEPS)
				||(screen_id == SCREEN_ID_SLEEP)
				||(screen_id == SCREEN_ID_SETTINGS)
				)
			{
				EnterIdleScreen();
			}
			
			LCD_SleepIn();
		}
	}

	if(lcd_sleep_out)
	{	
		lcd_sleep_out = false;
	
		LCD_SleepOut();
		
		if(IsInIdleScreen())
		{
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TIME|SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK|SCREEN_EVENT_UPDATE_SIG|SCREEN_EVENT_UPDATE_NET_MODE|SCREEN_EVENT_UPDATE_BAT;
			IdleScreenProcess();
		}

		LCD_BL_On();
	}
}
