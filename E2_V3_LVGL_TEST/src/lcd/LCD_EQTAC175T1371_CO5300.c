/****************************************Copyright (c)************************************************
** File Name:			    LCD_EQTAC175T1371_CO5300.c
** Descriptions:			The LCD_EQTAC175T1371_CO5300 screen drive source file
** Created By:				xie biao
** Created Date:			2025-05-22
** Modified Date:      		2025-05-22
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "lcd.h"
#include "font.h"
#include "settings.h"
#include "logger.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif
#ifdef LCD_EQTAC175T1371_CO5300
#include "LCD_EQTAC175T1371_CO5300.h"

#define SPI_BUF_LEN	8
#define SPI_MUIT_BY_CS

struct device *spi_lcd;
struct device *gpio_lcd;

struct spi_buf_set tx_bufs,rx_bufs;
struct spi_buf tx_buff,rx_buff;

static struct k_timer backlight_timer;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

static LCD_BL_MODE bl_mode = LCD_BL_AUTO;

static uint8_t tx_buffer[SPI_BUF_LEN] = {0};
static uint8_t rx_buffer[SPI_BUF_LEN] = {0};

uint8_t lcd_data_buffer[2*LCD_DATA_LEN] = {0};	//xb add 20200702 a pix has 2 byte data

#ifdef SPI_MUIT_BY_CS
void LCD_CS_LOW(void)
{
	gpio_pin_set(gpio_lcd, CS, 0);
}

void LCD_CS_HIGH(void)
{
	gpio_pin_set(gpio_lcd, CS, 1);
}
#endif

static void LCD_SPI_Init(void)
{
	spi_lcd = DEVICE_DT_GET(LCD_DEV);
	if(!spi_lcd) 
	{
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	spi_cfg.frequency = 8000000;
	spi_cfg.slave = 0;

#ifndef SPI_MUIT_BY_CS
	spi_cs_ctr.gpio_dev = DEVICE_DT_GET(LCD_PORT);
	if (!spi_cs_ctr.gpio_dev)
	{
		LOGD("Unable to get GPIO SPI CS device");
		return;
	}

	spi_cs_ctr.gpio_pin = CS;
	spi_cs_ctr.delay = 0U;
	spi_cfg.cs = &spi_cs_ctr;
#endif
}

static void LCD_SPI_Transceive(uint8_t *txbuf, uint32_t txbuflen, uint8_t *rxbuf, uint32_t rxbuflen)
{
	int err;

#ifdef SPI_MUIT_BY_CS
	LCD_CS_LOW();
#endif
	
	tx_buff.buf = txbuf;
	tx_buff.len = txbuflen;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	rx_buff.buf = rxbuf;
	rx_buff.len = rxbuflen;
	rx_bufs.buffers = &rx_buff;
	rx_bufs.count = 1;
		
	err = spi_transceive(spi_lcd, &spi_cfg, &tx_bufs, &rx_bufs);
	if(err)
	{
		LOGD("SPI err: %d", err);
	}

#ifdef SPI_MUIT_BY_CS
	LCD_CS_HIGH();
#endif	
}

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
	lcd_data_buffer[0] = i;
	
	LCD_SPI_Transceive(lcd_data_buffer, 1, NULL, 0);
}

//----------------------------------------------------------------------
//写寄存器函数
void ReadComm(uint8_t i, uint8_t *data, uint8_t len)
{
	lcd_data_buffer[0] = i;
	
	gpio_pin_set(gpio_lcd, RS, 0);

	LCD_SPI_Transceive(lcd_data_buffer, 1, data, len);
}

//i:寄存器值
void WriteComm(uint8_t i)
{
	gpio_pin_set(gpio_lcd, RS, 0);
	Write_Data(i);
}

//写LCD数据
//i:要写入的值
void WriteData(uint8_t i)
{
	gpio_pin_set(gpio_lcd, RS, 1);
	Write_Data(i);  
}

void WriteDispData(uint8_t DataH,uint8_t DataL)
{
	gpio_pin_set(gpio_lcd, RS, 1);

	lcd_data_buffer[0] = DataH;
	lcd_data_buffer[1] = DataL;
	
	LCD_SPI_Transceive(lcd_data_buffer, 2, NULL, 0);
}

//LCD画点函数
//color:要填充的颜色
void WriteOneDot(unsigned int color)
{ 
	WriteDispData(color>>8, color);
}

////////////////////////////////////////////////测试函数//////////////////////////////////////////
void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h) //reentrant
{
	x += 0;
	y += 0;

	WriteComm(0x2A);             
	WriteData(x>>8);             
	WriteData(x);             
	WriteData((x+w-1)>>8);             
	WriteData((x+w-1));             

	WriteComm(0x2B);             
	WriteData(y>>8);             
	WriteData(y);             
	WriteData((y+h-1)>>8);//	WriteData((Yend+1)>>8);             
	WriteData((y+h-1));//	WriteData(Yend+1);   	

	WriteComm(0x2c);
}

void DispColor(uint32_t total, uint16_t color)
{
	uint32_t i,remain;      

	gpio_pin_set(gpio_lcd, RS, 1);
	
	while(1)
	{
		if(total <= LCD_DATA_LEN)
			remain = total;
		else
			remain = LCD_DATA_LEN;
		
		for(i=0;i<remain;i++)
		{
			lcd_data_buffer[2*i] = color>>8;
			lcd_data_buffer[2*i+1] = color;
		}
		
		LCD_SPI_Transceive(lcd_data_buffer, 2*remain, NULL, 0);

		if(remain == total)
			break;

		total -= remain;
	}
}

void DispData(uint32_t total, uint8_t *data)
{
	uint32_t i,remain;      

	gpio_pin_set(gpio_lcd, RS, 1);
	
	while(1)
	{
		if(total <= 2*LCD_DATA_LEN)
			remain = total;
		else
			remain = 2*LCD_DATA_LEN;
		
		for(i=0;i<remain;i++)
		{
			lcd_data_buffer[i] = data[i];
		}
		
		LCD_SPI_Transceive(lcd_data_buffer, remain, NULL, 0);

		if(remain == total)
			break;

		total -= remain;
	}
}

//测试函数（显示RGB条纹）
void DispBand(void)	 
{
	uint32_t i;
	unsigned int color[8]={0xf800,0x07e0,0x001f,0xFFE0,0XBC40,0X8430,0x0000,0xffff};//0x94B2

	for(i=0;i<8;i++)
	{
		DispColor(COL*(ROW/8), color[i]);
	}

	DispColor(COL*(ROW%8), color[7]);
}

//测试函数（画边框）
void DispFrame(void)
{
	unsigned int i,j;

	BlockWrite(0,0,COL,ROW);

	WriteDispData(0xf8, 0x00);

	for(i=0;i<COL-2;i++)
	{
		WriteDispData(0xFF, 0xFF);
	}
	
	WriteDispData(0x00, 0x1F);

	for(j=0;j<ROW-2;j++)
	{
		WriteDispData(0xf8, 0x00);
 
		for(i=0;i<COL-2;i++)
		{
			WriteDispData(0x00, 0x00);
		}

		WriteDispData(0x00, 0x1f);
	}

	WriteDispData(0xf8, 0x00);
 
	for(i=0;i<COL-2;i++)
	{
		WriteDispData(0xff, 0xff);
	}

	WriteDispData(0x00, 0x1f);
}

//////////////////////////////////////////////////////////////////////////////////////////////
bool LCD_CheckID(void)
{
	WriteComm(0x04);
	Delay(10); 

	if(rx_buffer[0] == 0x89 && rx_buffer[1] == 0xF0)
		return true;
	else
		return false;
}

//清屏函数
//color:要清屏的填充色
void LCD_Clear(uint16_t color)
{
	BlockWrite(0,0,COL,ROW);//定位

	DispColor(COL*ROW, color);
} 

//背光打开
void LCD_BL_On(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#endif	
}

//背光关闭
void LCD_BL_Off(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_Off();
#endif
}

//屏幕睡眠
void LCD_SleepIn(void)
{
	if(lcd_is_sleeping)
		return;

	WriteComm(0x28);//Display off	
	WriteComm(0x10);//Sleep in	
	Delay(120);

	//gpio_pin_set(gpio_lcd, EN, 0);

	LOGD("lcd sleep in!");

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

	//gpio_pin_set(gpio_lcd, EN, 1);
	
	WriteComm(0x11);//Sleep out	
	Delay(120);     
	WriteComm(0x29);//Display on
	
	LOGD("lcd sleep out!");

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
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);
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
	int err;
	uint8_t buffer[8] = {0};
	
  	gpio_lcd = DEVICE_DT_GET(LCD_PORT);
	if(!gpio_lcd)
	{
		return;
	}

#ifdef SPI_MUIT_BY_CS
	gpio_pin_configure(gpio_lcd, CS, GPIO_OUTPUT);
#endif
	gpio_pin_configure(gpio_lcd, RST, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, RS, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, EN, GPIO_OUTPUT);

	LCD_SPI_Init();

	gpio_pin_set(gpio_lcd, EN, 1);
	
	gpio_pin_set(gpio_lcd, RST, 1);
	Delay(10);
	gpio_pin_set(gpio_lcd, RST, 0);
	Delay(10);
	gpio_pin_set(gpio_lcd, RST, 1);
	Delay(120);

	//Reset dual & quad SPI to single SPI 
	WriteComm(0xFE);
	WriteData(0x00);
	//SPI write SRAM disable
	WriteComm(0xC4);
	WriteData(0x80);
	//turn off the Tearing Effect output signal
	WriteComm(0x35);
	WriteData(0x00);
	//55 RGB565, 66 RGB666, 77 RGB888
	WriteComm(0x3A);
	WriteData(0x55);
	//Display direction 00 x inc,y inc;40 x dec,y inc;80 x inc,y dec;C0 x dec,y dec 
	WriteComm(0x36);
	WriteData(0x00);
	//Display Dimming Control On/Off.
	WriteComm(0x53);
	WriteData(0x20);
	//adjust brightness value. 00~FF
	WriteComm(0x51);
	WriteData(0xFF);
	//Column Address Set
	WriteComm(0x2A);     
	WriteData(0x00);   
	WriteData(0x00);   //0
	WriteData(0x01);   
	WriteData(0x85);   //389
	//Row Address Set
	WriteComm(0x2B);     
	WriteData(0x00);   
	WriteData(0x00);   
	WriteData(0x01);   
	WriteData(0xC1);   //449
	//Sleep out
	WriteComm(0x11);
	Delay(120);
	//Display On
	WriteComm(0x29);
	//Memory Start Write
	WriteComm(0x2C);

	LCD_Clear(BLACK);		//清屏为黑色
	Delay(30);
	
	//点亮背光
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#endif

	lcd_is_sleeping = false;

	k_timer_init(&backlight_timer, backlight_timer_handler, NULL);

	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);	
}

#endif
