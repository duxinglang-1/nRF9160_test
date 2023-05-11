#include <stdlib.h>
#include <drivers/gpio.h>
#include "lcd.h"
#include "font.h" 
#include "settings.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif

#ifdef LCD_R154101_ST7796S
#include "LCD_R154101_ST7796S.h"

static struct device *gpio_lcd;

static struct k_timer backlight_timer;

static LCD_BL_MODE bl_mode = LCD_BL_AUTO;

uint8_t m_db_list[8] = DBS_LIST;	//定义屏幕数据接口数组


//LCD延时函数
void Delay(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

static void backlight_timer_handler(struct k_timer *timer)
{
	lcd_sleep_in = true;
}

//数据接口函数
//i:8位数据
void Write_Data(uint8_t i) 
{
	uint8_t t;
	
	for(t = 0; t < 8; t++)
	{
		if(i & 0x01)						//将数据写入数据接口
			gpio_pin_write(gpio_lcd, m_db_list[t], 1);
		else
			gpio_pin_write(gpio_lcd, m_db_list[t], 0);
		
		i >>= 1;							//数据右移一位
	}
}

//----------------------------------------------------------------------
//写寄存器函数
//i:寄存器值
void WriteComm(unsigned int i)
{
	gpio_pin_write(gpio_lcd, CS, 0);				//CS置0
	gpio_pin_write(gpio_lcd, RD, 1);				//RD置1
	gpio_pin_write(gpio_lcd, RS, 0);				//RS清0
		
	Write_Data(i);  

	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
	gpio_pin_write(gpio_lcd, CS, 1);	
}

//写LCD数据
//i:要写入的值
void WriteData(unsigned int i)
{
	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RD, 1);
	gpio_pin_write(gpio_lcd, RS, 1);

	Write_Data(i);  

	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
	gpio_pin_write(gpio_lcd, CS, 1);
}

void WriteDispData(unsigned char DataH,unsigned char DataL)
{
	Write_Data(DataH);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
	
	Write_Data(DataL);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
}

//LCD画点函数
//color:要填充的颜色
void WriteOneDot(unsigned int color)
{ 
	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RD, 1);
	gpio_pin_write(gpio_lcd, RS, 1);

	Write_Data(color>>8);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);

	Write_Data(color);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);

	gpio_pin_write(gpio_lcd, CS, 1);
}

 void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h) //reentrant
{
	//ST7796
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

void DispColor(u32_t total, u16_t color)
{
	u32_t i;

	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);
	gpio_pin_write(gpio_lcd, RD, 1);

	for(i=0;i<total;i++)
	{
		Write_Data(color>>8);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);

		Write_Data(color);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
	}

	gpio_pin_write(gpio_lcd, CS, 1);
}

void DispData(u32_t total, u8_t *data)
{
	u32_t i;      

	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);
	gpio_pin_write(gpio_lcd, RD, 1);

	for(i=0;i<total;i++)
	{
		Write_Data(data[i]);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
	}

	gpio_pin_write(gpio_lcd, CS, 1);
}

//测试函数（显示RGB条纹）
void DispBand(void)	 
{
	unsigned int i,j,k;
	//unsigned int color[8]={0x001f,0x07e0,0xf800,0x07ff,0xf81f,0xffe0,0x0000,0xffff};
	unsigned int color[8]={0xf800,0xf800,0x07e0,0x07e0,0x001f,0x001f,0xffff,0xffff};//0x94B2
	//unsigned int gray16[]={0x0000,0x1082,0x2104,0x3186,0x42,0x08,0x528a,0x630c,0x738e,0x7bcf,0x9492,0xa514,0xb596,0xc618,0xd69a,0xe71c,0xffff};

   	BlockWrite(0,0,COL,ROW);

	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RD, 1);
	gpio_pin_write(gpio_lcd, RS, 1);
	
	for(i=0;i<8;i++)
	{
		for(j=0;j<ROW/8;j++)
		{
	        for(k=0;k<COL;k++)
			{
				Write_Data(color[i]>>8);
				gpio_pin_write(gpio_lcd, WR, 0);
				gpio_pin_write(gpio_lcd, WR, 1);

				Write_Data(color[i]);
				gpio_pin_write(gpio_lcd, WR, 0);
				gpio_pin_write(gpio_lcd, WR, 1);
			} 
		}
	}
	for(j=0;j<(ROW%8);j++)
	{
		for(k=0;k<COL;k++)
		{
			Write_Data(color[7]>>8);
			gpio_pin_write(gpio_lcd, WR, 0);
			gpio_pin_write(gpio_lcd, WR, 1);

			Write_Data(color[7]);
			gpio_pin_write(gpio_lcd, WR, 0);
			gpio_pin_write(gpio_lcd, WR, 1);
		} 
	}

	gpio_pin_write(gpio_lcd, CS, 1);
}

//测试函数（画边框）
void DispFrame(void)
{
	unsigned int i,j;
	
	BlockWrite(0,0,COL,ROW);

	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RD, 1);
	gpio_pin_write(gpio_lcd, RS, 1);
		
	Write_Data(0xf8);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
	
	Write_Data(0x00);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);

	for(i=0;i<COL-2;i++)
	{
		Write_Data(0xFF);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
		
		Write_Data(0xFF);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
	}
	
	Write_Data(0x00);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
	
	Write_Data(0x1F);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);

	for(j=0;j<ROW-2;j++)
	{
		Write_Data(0xf8);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
		
		Write_Data(0x00);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
		
		for(i=0;i<COL-2;i++)
		{
			Write_Data(0x00);
			gpio_pin_write(gpio_lcd, WR, 0);
			gpio_pin_write(gpio_lcd, WR, 1);
			
			Write_Data(0x00);
			gpio_pin_write(gpio_lcd, WR, 0);
			gpio_pin_write(gpio_lcd, WR, 1);
		}
		
		Write_Data(0x00);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
		
		Write_Data(0x1f);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
	}

	Write_Data(0xf8);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
	
	Write_Data(0x00);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);

	for(i=0;i<COL-2;i++)
	{
		Write_Data(0xff);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
		
		Write_Data(0xff);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
	}
	
	Write_Data(0x00);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);
	
	Write_Data(0x1f);
	gpio_pin_write(gpio_lcd, WR, 0);
	gpio_pin_write(gpio_lcd, WR, 1);

	gpio_pin_write(gpio_lcd, CS, 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////

//清屏函数
//color:要清屏的填充色
void LCD_Clear(uint16_t color)
{
	uint32_t index=0;      
	uint32_t totalpoint=ROW;
	totalpoint*=COL; 			//得到总点数
	
	BlockWrite(0,0,COL,ROW);//定位

	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);
	gpio_pin_write(gpio_lcd, RD, 1);

	for(index=0;index<totalpoint;index++)
	{
		Write_Data(color>>8);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);

		Write_Data(color);
		gpio_pin_write(gpio_lcd, WR, 0);
		gpio_pin_write(gpio_lcd, WR, 1);
	}

	gpio_pin_write(gpio_lcd, CS, 0);
}

//背光打开
void LCD_BL_On(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#else
	gpio_pin_write(gpio_lcd, LEDK_1, 0);
	gpio_pin_write(gpio_lcd, LEDK_2, 0);																										 
#endif
}

//背光关闭
void LCD_BL_Off(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_Off();
#else
	gpio_pin_write(gpio_lcd, LEDK_1, 1);
	gpio_pin_write(gpio_lcd, LEDK_2, 1);
#endif
}

//屏幕睡眠
void LCD_SleepIn(void)
{
	if(lcd_is_sleeping)
		return;

	WriteComm(0x28);	
	WriteComm(0x10);  		//Sleep in	
	Delay(120);             //延时120ms

	lcd_is_sleeping = true;
}

//屏幕唤醒
void LCD_SleepOut(void)
{
	uint16_t bk_time;

	if(k_timer_remaining_get(&backlight_timer) > 0)
		k_timer_stop(&backlight_timer);
	
	if(global_settings.backlight_time != 0)
	{
		bk_time = global_settings.backlight_time;
		//xb add 2020-12-31 抬手亮屏5秒后自动息屏
		if(sleep_out_by_wrist)
		{
			sleep_out_by_wrist = false;
			bk_time = 5;
		}

		switch(bl_mode)
		{
		case LCD_BL_ALWAYS_ON:
		case LCD_BL_OFF:
			break;

		case LCD_BL_AUTO:
			k_timer_start(&backlight_timer, K_SECONDS(bk_time), K_NO_WAIT);
			break;
		}
	}

	if(!lcd_is_sleeping)
		return;
	
	WriteComm(0x11);  		//Sleep out	
	Delay(120);             //延时120ms
	WriteComm(0x29);	
	
	lcd_is_sleeping = false;
}

//屏幕重置背光延时
void LCD_ResetBL_Timer(void)
{
	if((bl_mode == LCD_BL_ALWAYS_ON) || (bl_mode == LCD_BL_OFF))
		return;
	
	if(k_timer_remaining_get(&backlight_timer) > 0)
		k_timer_stop(&backlight_timer);
	
	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), NULL);
}

//获取屏幕当前背光模式
LCD_BL_MODE LCD_Get_BL_Mode(void)
{
	return bl_mode;
}

//屏幕背光模式设置
void LCD_Set_BL_Mode(LCD_BL_MODE mode)
{
	if(bl_mode == mode)
		return;
	
	switch(mode)
	{
	case LCD_BL_ALWAYS_ON:
		k_timer_stop(&backlight_timer);
		if(lcd_is_sleeping)
			LCD_SleepOut();
		break;

	case LCD_BL_AUTO:
		k_timer_stop(&backlight_timer);
		if(global_settings.backlight_time != 0)
			k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);
		break;

	case LCD_BL_OFF:
		k_timer_stop(&backlight_timer);
		LCD_BL_Off();
		if(!lcd_is_sleeping)
			LCD_SleepIn();
		break;
	}

	bl_mode = mode;
}


//LCD初始化函数
void LCD_Init(void)
{
	int i;
	
	//端口初始化
	gpio_lcd = device_get_binding(LCD_PORT);

	gpio_pin_configure(gpio_lcd, CS, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RST, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RS, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, WR, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RD, GPIO_DIR_OUT);

	gpio_pin_configure(gpio_lcd, LEDK_1, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, LEDK_2, GPIO_DIR_OUT);

	for(i=0;i<8;i++)
	{
		gpio_pin_configure(gpio_lcd, m_db_list[i], GPIO_DIR_OUT);
	}

	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(10);

	gpio_pin_write(gpio_lcd, RST, 0);
	Delay(10);

	gpio_pin_write(gpio_lcd, RST, 1);
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

	LCD_Clear(WHITE);		//清屏为黑色
	Delay(30);

	//点亮背光
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#else
	gpio_pin_write(gpio_lcd, LEDK_1, 0);
	gpio_pin_write(gpio_lcd, LEDK_2, 0);
#endif

	lcd_is_sleeping = false;

	k_timer_init(&backlight_timer, backlight_timer_handler, NULL);

	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), NULL);	
}

#endif
