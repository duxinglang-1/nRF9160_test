#include <stdlib.h>
#include <drivers/gpio.h>
#include "lcd.h"
#include "font.h" 

#ifdef LCD_R154101_ST7796S
#include "LCD_R154101_ST7796S.h"

struct device *lcd_gpio;

uint8_t m_db_list[8] = DBS_LIST;	//定义屏幕数据接口数组

//LCD延时函数
void Delay(unsigned int dly)
{
	k_sleep(dly);
}

//数据接口函数
//i:8位数据
void Write_Data(uint8_t i) 
{
	uint8_t t;
	
	for(t = 0; t < 8; t++)
	{
		if(i & 0x01)						//将数据写入数据接口
			gpio_pin_write(lcd_gpio, m_db_list[t], 1);
		else
			gpio_pin_write(lcd_gpio, m_db_list[t], 0);
		
		i >>= 1;							//数据右移一位
	}
}

//----------------------------------------------------------------------
//写寄存器函数
//i:寄存器值
void WriteComm(unsigned int i)
{
	gpio_pin_write(lcd_gpio, CS, 0);				//CS置0
	gpio_pin_write(lcd_gpio, RD, 1);				//RD置1
	gpio_pin_write(lcd_gpio, RS, 0);				//RS清0
		
	Write_Data(i);  

	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
	gpio_pin_write(lcd_gpio, CS, 1);	
}

//写LCD数据
//i:要写入的值
void WriteData(unsigned int i)
{
	gpio_pin_write(lcd_gpio, CS, 0);
	gpio_pin_write(lcd_gpio, RD, 1);
	gpio_pin_write(lcd_gpio, RS, 1);

	Write_Data(i);  

	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
	gpio_pin_write(lcd_gpio, CS, 1);
}

void WriteDispData(unsigned char DataH,unsigned char DataL)
{
	Write_Data(DataH);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
	
	Write_Data(DataL);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
}

//LCD画点函数
//color:要填充的颜色
void WriteOneDot(unsigned int color)
{ 
	gpio_pin_write(lcd_gpio, CS, 0);
	gpio_pin_write(lcd_gpio, RD, 1);
	gpio_pin_write(lcd_gpio, RS, 1);

	Write_Data(color>>8);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);

	Write_Data(color);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);

	gpio_pin_write(lcd_gpio, CS, 1);
}

//LCD初始化函数
void LCD_Init(void)
{
	int i;
	
	//端口初始化
	lcd_gpio = device_get_binding(LCD_PORT);

	gpio_pin_configure(lcd_gpio, CS, GPIO_DIR_OUT);
	gpio_pin_configure(lcd_gpio, RST, GPIO_DIR_OUT);
	gpio_pin_configure(lcd_gpio, RS, GPIO_DIR_OUT);
	gpio_pin_configure(lcd_gpio, WR, GPIO_DIR_OUT);
	gpio_pin_configure(lcd_gpio, RD, GPIO_DIR_OUT);

	gpio_pin_configure(lcd_gpio, LEDK_1, GPIO_DIR_OUT);
	gpio_pin_configure(lcd_gpio, LEDK_2, GPIO_DIR_OUT);

	for(i=0;i<8;i++)
	{
		gpio_pin_configure(lcd_gpio, m_db_list[i], GPIO_DIR_OUT);
	}

	gpio_pin_write(lcd_gpio, RST, 1);
	Delay(10);

	gpio_pin_write(lcd_gpio, RST, 0);
	Delay(10);

	gpio_pin_write(lcd_gpio, RST, 1);
	Delay(120);

	WriteComm(0x36);
	WriteData(0x48);
	WriteComm(0x3a);
	WriteData(0x05);

	WriteComm(0xF0);
	WriteData(0xC3);
	WriteComm(0xF0);
	WriteData(0x96);

	WriteComm(0xb1);
	WriteData(0xa0);
	WriteData(0x10);

	WriteComm(0xb4);
	WriteData(0x00);

	WriteComm(0xb5);
	WriteData(0x40);
	WriteData(0x40);
	WriteData(0x00);
	WriteData(0x04);

	WriteComm(0xb6);
	WriteData(0x8a);
	WriteData(0x07);
	WriteData(0x27);

	WriteComm(0xb9);
	WriteData(0x02);

	WriteComm(0xc5);
	WriteData(0x2e);

	WriteComm(0xE8);
	WriteData(0x40);
	WriteData(0x8a);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0x29);
	WriteData(0x19);
	WriteData(0xa5);
	WriteData(0x93);

	WriteComm(0xe0);
	WriteData(0xf0);
	WriteData(0x07);
	WriteData(0x0e);
	WriteData(0x0a);
	WriteData(0x08);
	WriteData(0x25);
	WriteData(0x38);
	WriteData(0x43);
	WriteData(0x51);
	WriteData(0x38);
	WriteData(0x14);
	WriteData(0x12);
	WriteData(0x32);
	WriteData(0x3f);

	WriteComm(0xe1);
	WriteData(0xf0);
	WriteData(0x08);
	WriteData(0x0d);
	WriteData(0x09);
	WriteData(0x09);
	WriteData(0x26);
	WriteData(0x39);
	WriteData(0x45);
	WriteData(0x52);
	WriteData(0x07);
	WriteData(0x13);
	WriteData(0x16);
	WriteData(0x32);
	WriteData(0x3f);

	WriteComm(0xf0);
	WriteData(0x3c);
	WriteComm(0xf0);
	WriteData(0x69);
	WriteComm(0x11);
	Delay(150);  
	WriteComm(0x21);
	WriteComm(0x29);

	//点亮背光
	gpio_pin_write(lcd_gpio, LEDK_1, 0);
	gpio_pin_write(lcd_gpio, LEDK_2, 0);
				
	LCD_Clear(WHITE);		//清屏为黑色
}

 
////////////////////////////////////////////////测试函数//////////////////////////////////////////
void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h) //reentrant
{
	//ILI9327
	WriteComm(0x2A);             
	WriteData(x>>8);             
	WriteData(x);             
	WriteData((x+w)>>8);             
	WriteData((x+w));             
	
	WriteComm(0x2B);             
	WriteData(y>>8);             
	WriteData(y);             
	WriteData((y+h)>>8);//	WriteData((Yend+1)>>8);             
	WriteData((y+h));//	WriteData(Yend+1);   	

	WriteComm(0x2c);
}

void DispColor(unsigned int color)
{
	unsigned int i,j;

	BlockWrite(0,0,COL-1,ROW-1);

	gpio_pin_write(lcd_gpio, CS, 0);
	gpio_pin_write(lcd_gpio, RS, 1);
	gpio_pin_write(lcd_gpio, RD, 1);

	for(i=0;i<ROW;i++)
	{
	    for(j=0;j<COL;j++)
		{    
			Write_Data(color>>8);
			gpio_pin_write(lcd_gpio, WR, 0);
			gpio_pin_write(lcd_gpio, WR, 1);

			Write_Data(color);
			gpio_pin_write(lcd_gpio, WR, 0);
			gpio_pin_write(lcd_gpio, WR, 1);
		}
	}

	gpio_pin_write(lcd_gpio, CS, 1);
}

//测试函数（显示RGB条纹）
void DispBand(void)	 
{
	unsigned int i,j,k;
	//unsigned int color[8]={0x001f,0x07e0,0xf800,0x07ff,0xf81f,0xffe0,0x0000,0xffff};
	unsigned int color[8]={0xf800,0xf800,0x07e0,0x07e0,0x001f,0x001f,0xffff,0xffff};//0x94B2
	//unsigned int gray16[]={0x0000,0x1082,0x2104,0x3186,0x42,0x08,0x528a,0x630c,0x738e,0x7bcf,0x9492,0xa514,0xb596,0xc618,0xd69a,0xe71c,0xffff};

   	BlockWrite(0,0,COL-1,ROW-1);

	gpio_pin_write(lcd_gpio, CS, 0);
	gpio_pin_write(lcd_gpio, RD, 1);
	gpio_pin_write(lcd_gpio, RS, 1);
	
	for(i=0;i<8;i++)
	{
		for(j=0;j<ROW/8;j++)
		{
	        for(k=0;k<COL;k++)
			{
				Write_Data(color[i]>>8);
				gpio_pin_write(lcd_gpio, WR, 0);
				gpio_pin_write(lcd_gpio, WR, 1);

				Write_Data(color[i]);
				gpio_pin_write(lcd_gpio, WR, 0);
				gpio_pin_write(lcd_gpio, WR, 1);
			} 
		}
	}
	for(j=0;j<(ROW%8);j++)
	{
		for(k=0;k<COL;k++)
		{
			Write_Data(color[7]>>8);
			gpio_pin_write(lcd_gpio, WR, 0);
			gpio_pin_write(lcd_gpio, WR, 1);

			Write_Data(color[7]);
			gpio_pin_write(lcd_gpio, WR, 0);
			gpio_pin_write(lcd_gpio, WR, 1);
		} 
	}

	gpio_pin_write(lcd_gpio, CS, 1);
}

//测试函数（画边框）
void DispFrame(void)
{
	unsigned int i,j;
	
	BlockWrite(0,0,COL-1,ROW-1);

	gpio_pin_write(lcd_gpio, CS, 0);
	gpio_pin_write(lcd_gpio, RD, 1);
	gpio_pin_write(lcd_gpio, RS, 1);
		
	Write_Data(0xf8);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
	
	Write_Data(0x00);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);

	for(i=0;i<COL-2;i++)
	{
		Write_Data(0xFF);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
		
		Write_Data(0xFF);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
	}
	
	Write_Data(0x00);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
	
	Write_Data(0x1F);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);

	for(j=0;j<ROW-2;j++)
	{
		Write_Data(0xf8);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
		
		Write_Data(0x00);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
		
		for(i=0;i<COL-2;i++)
		{
			Write_Data(0x00);
			gpio_pin_write(lcd_gpio, WR, 0);
			gpio_pin_write(lcd_gpio, WR, 1);
			
			Write_Data(0x00);
			gpio_pin_write(lcd_gpio, WR, 0);
			gpio_pin_write(lcd_gpio, WR, 1);
		}
		
		Write_Data(0x00);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
		
		Write_Data(0x1f);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
	}

	Write_Data(0xf8);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
	
	Write_Data(0x00);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);

	for(i=0;i<COL-2;i++)
	{
		Write_Data(0xff);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
		
		Write_Data(0xff);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
	}
	
	Write_Data(0x00);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);
	
	Write_Data(0x1f);
	gpio_pin_write(lcd_gpio, WR, 0);
	gpio_pin_write(lcd_gpio, WR, 1);

	gpio_pin_write(lcd_gpio, CS, 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////

//清屏函数
//color:要清屏的填充色
void LCD_Clear(uint16_t color)
{
	uint32_t index=0;      
	uint32_t totalpoint=ROW;
	totalpoint*=COL; 			//得到总点数
	
	BlockWrite(0,0,COL-1,ROW-1);//定位

	gpio_pin_write(lcd_gpio, CS, 0);
	gpio_pin_write(lcd_gpio, RS, 1);
	gpio_pin_write(lcd_gpio, RD, 1);

	for(index=0;index<totalpoint;index++)
	{
		Write_Data(color>>8);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);

		Write_Data(color);
		gpio_pin_write(lcd_gpio, WR, 0);
		gpio_pin_write(lcd_gpio, WR, 1);
	}

	gpio_pin_write(lcd_gpio, CS, 0);
} 

#endif
