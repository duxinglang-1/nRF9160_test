#include <stdlib.h>
#include <stdint.h>
#include "lcd.h"
#include "font.h"
 
#if defined(LCD_R154101_ST7796S)
#include "LCD_R154101_ST7796S.h"
#elif defined(LCD_LH096TIG11G_ST7735SV)
#include "LCD_LH096TIG11G_ST7735SV.h"
#elif defined(LCD_ORCT012210N_ST7789V2)
#include "LCD_ORCT012210N_ST7789V2.h"
#endif 


//LCD屏幕的高度和宽度
uint16_t LCD_WIDTH = COL;
uint16_t LCD_HEIGHT = ROW;

//LCD的画笔颜色和背景色	   
uint16_t POINT_COLOR=WHITE;	//画笔颜色
uint16_t BACK_COLOR=BLACK;  //背景色 

//默认字体大小
#ifdef FONT_16
system_font_size system_font = FONT_SIZE_16;
#elif defined(FONT_24)
system_font_size system_font = FONT_SIZE_24;
#elif defined(FONT_32)
system_font_size system_font = FONT_SIZE_32;
#else
system_font_size system_font = FONT_SIZE_16;
#endif

//快速画点
//x,y:坐标
//color:颜色
void LCD_Fast_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{	   
	BlockWrite(x,y,1,1);	//定坐标
	WriteOneDot(color);				//画点函数	
}	 

//在指定区域内填充单个颜色
//(x,y),(w,h):填充矩形对角坐标,区域大小为:w*h   
//color:要填充的颜色
void LCD_Fill(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color)
{          
	uint16_t i,j;

	for(i=y;i<=(y+h);i++)
	{
		BlockWrite(x,i,w,y+h-i);
		
	#ifdef LCD_TYPE_SPI
		DispColor(w, color);
	#else
		for(j=0;j<w;j++)
			WriteOneDot(color); //显示颜色 
	#endif
	}	 
}  
//在指定区域内填充指定颜色块	(显示图片)		 
//(x,y),(w,h):填充矩形对角坐标,区域大小为:w*h   
//color:要填充的颜色
void LCD_Color_Fill(uint16_t x,uint16_t y,uint16_t w,uint16_t h,unsigned int *color)
{  
	uint16_t i,j;

 	for(i=0;i<h;i++)
	{
		BlockWrite(x,y+i,w,h-i);
		
	#ifdef LCD_TYPE_SPI
		DispColor(w, color[i*w+j]);
	#else
		for(j=0;j<w;j++)
			WriteOneDot(color[i*w+j]);	//显示颜色 
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
	
	if(delta_x>0)
		incx=1; //设置单步方向 
	else if(delta_x==0)
		incx=0;//垂直线 
	else 
	{
		incx=-1;
		delta_x=-delta_x;
	} 

	if(delta_y>0)
		incy=1; 
	else if(delta_y==0)
		incy=0;//水平线 
	else
	{
		incy=-1;
		delta_y=-delta_y;
	} 

	if(delta_x>delta_y)
		distance=delta_x; //选取基本增量坐标轴 
	else 
		distance=delta_y; 

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
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r)
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

//获取图片尺寸
//color:图片数据指针
//width:获取到的图片宽度输出地址
//height:获取到的图片高度输出地址
void LCD_get_pic_size(unsigned int *color, uint16_t *width, uint16_t *height)
{
	*width = 255*color[2]+color[3];
	*height = 255*color[4]+color[5];
}

//指定位置显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
void LCD_dis_pic(uint16_t x,uint16_t y,unsigned int *color)
{  
	uint16_t h,w;
	uint16_t i,j;
	u8_t databuf[2*COL] = {0};
	
	w=255*color[2]+color[3]; 			//获取图片宽度
	h=255*color[4]+color[5];			//获取图片高度
	if(w > COL)
		w = COL;
	if(h > ROW)
		h = ROW;
	
 	for(i=0;i<h;i++)
	{
		BlockWrite(x,y+i,w,1);	  	//设置刷新位置

	#ifdef LCD_TYPE_SPI
		for(j=0;j<w;j++)
		{
			databuf[2*j] = color[8+i*w+j]>>8;
			databuf[2*j+1] = color[8+i*w+j];
		}

		DispDate(2*w, databuf);
	#else
		for(j=0;j<w;j++)
			WriteOneDot(color[8+i*w+j]);	//显示颜色 
	#endif
	}		  
}

//指定位置显示图片,带透明色过滤
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
void LCD_dis_trans_pic(uint16_t x,uint16_t y,unsigned int *color,uint16_t trans)
{  
	uint16_t h,w;
	uint16_t i,j;
	
	w=255*color[2]+color[3]; 			//获取图片宽度
	h=255*color[4]+color[5];			//获取图片高度
	if(w > COL)
		w = COL;
	if(h > ROW)
		h = ROW;

 	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			if(trans != color[8+i*w+j])
			{
				BlockWrite(x+j,y+i,1,1);	  	//设置刷新位置
				WriteOneDot(color[8+i*w+j]);	//显示不透明的颜色 
			}
		}
	}		  
}


//指定位置旋转角度显示图片
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
//rotate:旋转角度,0,90,180,270,
void LCD_dis_pic_rotate(uint16_t x,uint16_t y,unsigned int *color,unsigned int rotate)
{
	uint16_t w,h;
	uint16_t i,j;
	
	w=255*color[2]+color[3]; 			//获取图片宽度
	h=255*color[4]+color[5];			//获取图片高度
	if(w > COL)
		w = COL;
	if(h > ROW)
		h = ROW;

	switch(rotate)
	{
	case 0:
		for(i=0;i<h;i++)
		{
			u8_t databuf[2*COL] = {0};
			
			BlockWrite(x,y+i,w,1);	  	//设置刷新位置

		#ifdef LCD_TYPE_SPI
			for(j=0;j<w;j++)
			{
				databuf[2*j] = color[8+i*w+j]>>8;
				databuf[2*j+1] = color[8+i*w+j];
			}

			DispDate(2*w, databuf);
		#else
			for(j=0;j<w;j++)
				WriteOneDot(color[8+i*w+j]);	//显示颜色 
		#endif
		}	
		break;
		
	case 90:
		for(i=0;i<w;i++)
		{
			u8_t databuf[2*ROW] = {0};
			
			BlockWrite(x,y+i,h,1);	  	//设置刷新位置

		#ifdef LCD_TYPE_SPI
			for(j=0;j<h;j++)
			{
				databuf[2*j] = color[8+(i+w*(h-1)-j*w)]>>8;
				databuf[2*j+1] = color[8+(i+w*(h-1)-j*w)];
			}

			DispDate(2*h, databuf);
		#else
			for(j=0;j<h;j++)
				WriteOneDot(color[8+(i+w*(h-1)-j*w)]);	//显示颜色 
		#endif	
		}
		break;
		
	case 180:
		for(i=0;i<h;i++)
		{
			u8_t databuf[2*COL] = {0};
			
			BlockWrite(x,y+i,w,1);	  	//设置刷新位置
			
		#ifdef LCD_TYPE_SPI
			for(j=0;j<w;j++)
			{
				databuf[2*j] = color[8+(w*h-1)-w*i-j]>>8;
				databuf[2*j+1] = color[8+(w*h-1)-w*i-j];
			}

			DispDate(2*w, databuf);
		#else
			for(j=0;j<w;j++)
				WriteOneDot(color[8+(w*h-1)-w*i-j]);	//显示颜色 
		#endif	
		}		
		break;
		
	case 270:
		for(i=0;i<w;i++)
		{
			u8_t databuf[2*ROW] = {0};
			
			BlockWrite(x,y+i,h,1);	  	//设置刷新位置

		#ifdef LCD_TYPE_SPI
			for(j=0;j<h;j++)
			{
				databuf[2*j] = color[8+(w-1-i+w*j)]>>8;
				databuf[2*j+1] = color[8+(w-1-i+w*j)];
			}

			DispDate(2*h, databuf);
		#else
			for(j=0;j<h;j++)
				WriteOneDot(color[8+(w-1-i+w*j)]);	//显示颜色 
		#endif		
		}		
		break;
	}
}

//指定位置旋转角度显示图片,带透明色过滤
//color:图片数据指针
//x:图片显示X坐标
//y:图片显示Y坐标
//rotate:旋转角度,0,90,180,270,
void LCD_dis_trans_pic_rotate(uint16_t x,uint16_t y,unsigned int *color,uint16_t trans,unsigned int rotate)
{
	uint16_t w,h;
	uint16_t i,j;
	
	w=255*color[2]+color[3]; 			//获取图片宽度
	h=255*color[4]+color[5];			//获取图片高度

	switch(rotate)
	{
	case 0:
		for(i=0;i<h;i++)
		{
			for(j=0;j<w;j++)
			{
				if(trans != color[8+i*w+j])
				{
					BlockWrite(x+j,y+i,1,1);	  	//设置刷新位置
					WriteOneDot(color[8+i*w+j]);	//显示颜色
				}
			}
		}	
		break;
		
	case 90:
		for(i=0;i<w;i++)
		{
			for(j=0;j<h;j++)
			{
				if(trans != color[8+(i+w*(h-1)-j*w)])
				{
					BlockWrite(x+j,y+i,1,1);	  	//设置刷新位置
					WriteOneDot(color[8+(i+w*(h-1)-j*w)]);	//显示颜色 
				}
			}
		}
		break;
		
	case 180:
		for(i=0;i<h;i++)
		{
			for(j=0;j<w;j++)
			{
				if(trans != color[8+(w*h-1)-w*i-j])
				{
					BlockWrite(x+j,y+i,1,1);	  	//设置刷新位置
					WriteOneDot(color[8+(w*h-1)-w*i-j]);	//显示颜色 
				}
			}
		}		
		break;
		
	case 270:
		for(i=0;i<w;i++)
		{
			for(j=0;j<h;j++)
			{
				if(trans != color[8+(w-1-i+w*j)])
				{
					BlockWrite(x+j,y+i,1,1);	  	//设置刷新位置
					WriteOneDot(color[8+(w-1-i+w*j)]);	//显示颜色 
				}
			}
		}		
		break;
	}
}

//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint8_t mode)
{
    uint8_t temp,t1,t;
	uint16_t y0=y;
	uint8_t csize=(system_font/8+((system_font%8)?1:0))*(system_font/2);		//得到字体一个字符对应点阵集所占的字节数	
 	
	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	for(t=0;t<csize;t++)
	{
		switch(system_font)
		{
		#ifdef FONT_16
			case FONT_SIZE_16:
				temp=asc2_1608[num][t]; 	 	//调用1608字体
				break;
		#endif
		#ifdef FONT_24
			case FONT_SIZE_24:
				temp=asc2_2412[num][t];			//调用2412字体
				break;
		#endif
		#ifdef FONT_32
			case FONT_SIZE_32:
				temp=asc2_3216[num][t];			//调用3216字体
				break;
		#endif
			default:
				return;							//没有的字库
		}
								
		for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)
				LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)
				LCD_Fast_DrawPoint(x,y,BACK_COLOR);

			temp<<=1;
			y++;
			
			if(y>=LCD_HEIGHT)
				return;		//超区域了
				
			if((y-y0)==system_font)
			{
				y=y0;
				x++;
				if(x>=LCD_WIDTH)
					return;	//超区域了
					
				break;
			}
		}  	 
	}  	    	   	 	  
}   

//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChineseChar(uint16_t x,uint16_t y,uint16_t num,uint8_t mode)
{  							  
	uint8_t temp,t1,t;
	uint16_t y0=y;
	uint16_t index=0;
	
	uint8_t csize=(system_font/8+((system_font%8)?1:0))*(system_font);		//得到字体一个字符对应点阵集所占的字节数	
 	index=94*((num>>8)-0xa0-1)+1*((num&0x00ff)-0xa0-1);			//offset = (94*(区码-1)+(位码-1))*32
	for(t=0;t<csize;t++)
	{	
		switch(system_font)
		{
		#ifdef FONT_16
			case FONT_SIZE_16:
				temp=chinese_1616[index][t]; 	 	//调用1616字体
				break;
		#endif
		#ifdef FONT_24
			case FONT_SIZE_24:
				temp=chinese_2424[index][t];		//调用2424字体
				break;
		#endif
		#ifdef FONT_32
			case FONT_SIZE_32:
				temp=chinese_3232[index][t];		//调用3232字体
				break;
		#endif
			default:
				return;								//没有的字库
		}	

		for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			y++;
			if(y>=LCD_HEIGHT)return;		//超区域了
			if((y-y0)==system_font)
			{
				y=y0;
				x++;
				if(x>=LCD_WIDTH)return;	//超区域了
				break;
			}
		} 
	}  	    	   	 	  
}   

//在指定矩形区域内显示中英文字符串
//x,y:起点坐标
//width,height:区域大小  
//*p:字符串起始地址	
void LCD_ShowStringInRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t *p)
{
	uint8_t x0=x;
	uint16_t phz=0;
	
	width+=x;
	height+=y;
    while(*p)
    {       
        if(x>=width){x=x0;y+=system_font;}
        if(y>=height)break;//退出
		if(*p<0x80)
		{
			LCD_ShowChar(x,y,*p,0);
			x+=system_font/2;
			p++;
		}
		else if(*(p+1))
        {
			phz = *p<<8;
			phz += *(p+1);
			LCD_ShowChineseChar(x,y,phz,0);
			x+=system_font;
			p+=2;
		}        
    }
}

//显示中英文字符串
//x,y:起点坐标
//*p:字符串起始地址	
void LCD_ShowString(uint16_t x,uint16_t y,uint8_t *p)
{
	uint8_t x0=x;
	uint16_t phz=0;
	
    while(*p)
    {       
        if(x>=LCD_WIDTH)break;//退出
        if(y>=LCD_HEIGHT)break;//退出
		if(*p<0x80)
		{
			LCD_ShowChar(x,y,*p,0);
			x+=system_font/2;
			p++;
		}
		else if(*(p+1))
        {
			phz = *p<<8;
			phz += *(p+1);
			LCD_ShowChineseChar(x,y,phz,0);
			x+=system_font;
			p+=2;
		}        
    }
}

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
				LCD_ShowChar(x+(system_font/2)*t,y,' ',0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(system_font/2)*t,y,temp+'0',0); 
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
				if(mode&0X80)LCD_ShowChar(x+(system_font/2)*t,y,'0',mode&0X01);  
				else LCD_ShowChar(x+(system_font/2)*t,y,' ',mode&0X01);  
 				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(system_font/2)*t,y,temp+'0',mode&0X01); 
	}
} 

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
			(*width) += system_font/2;
			p++;
		}
		else if(*(p+1))
        {
			(*width) += system_font;
			p += 2;
		}        
    }  
}

//设置系统字体
//font_size:枚举字体大小
void LCD_SetFontSize(uint8_t font_size)
{
	if(font_size > FONT_SIZE_MIN && font_size < FONT_SIZE_MAX)
		system_font = font_size;
}
