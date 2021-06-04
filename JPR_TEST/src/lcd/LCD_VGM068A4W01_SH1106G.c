/****************************************Copyright (c)************************************************
** File Name:			    LCD_VGM068A4W01_SH1106G.c
** Descriptions:			The VGM068A4W01_SH1106G screen drive source file
** Created By:				xie biao
** Created Date:			2021-01-07
** Modified Date:      		2021-01-07
** Version:			    	V1.0
******************************************************************************************************/
#include <drivers/spi.h>
#include <drivers/gpio.h>

#include "lcd.h"
#include "font.h" 
#include "settings.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif

#ifdef LCD_VGM068A4W01_SH1106G
#include "LCD_VGM068A4W01_SH1106G.h"

#define SPI_BUF_LEN	8

struct device *spi_lcd;
struct device *gpio_lcd;

struct spi_buf_set tx_bufs,rx_bufs;
struct spi_buf tx_buff,rx_buff;

static struct k_timer backlight_timer;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

static u8_t tx_buffer[SPI_BUF_LEN] = {0};
static u8_t rx_buffer[SPI_BUF_LEN] = {0};

u8_t lcd_data_buffer[2*LCD_DATA_LEN] = {0};	//xb add 20200702 a pix has 2 byte data

static void LCD_SPI_Init(void)
{
	spi_lcd = device_get_binding(LCD_DEV);
	if(!spi_lcd) 
	{
		printk("Could not get %s device\n", LCD_DEV);
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	spi_cfg.frequency = 4000000;
	spi_cfg.slave = 0;
}

static void LCD_SPI_Transceive(u8_t *txbuf, u32_t txbuflen, u8_t *rxbuf, u32_t rxbuflen)
{
	int err;
	
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
		printk("SPI error: %d\n", err);
	}

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
//i:寄存器值
void WriteComm(u8_t i)
{
	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 0);

	Write_Data(i);

	gpio_pin_write(gpio_lcd, CS, 1);
}

//写LCD数据
//i:要写入的值
void WriteData(u8_t i)
{
	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);

	Write_Data(i);

	gpio_pin_write(gpio_lcd, CS, 1);
}

void WriteDispData(u8_t Data)
{
	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);

	lcd_data_buffer[0] = Data;
	
	LCD_SPI_Transceive(lcd_data_buffer, 1, NULL, 0);

	gpio_pin_write(gpio_lcd, CS, 1);
}

//LCD画点函数
//color:要填充的颜色 1为点亮，0为熄灭
void WriteOneDot(u16_t color)
{ 
	WriteDispData((u8_t)(0x00ff&color));
}

////////////////////////////////////////////////测试函数//////////////////////////////////////////
void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h) //reentrant
{
	u8_t x0;
	
	if(x >= COL)
		x = 0;
	if(y >= ROW)
		y = 0;

	x0 = x+0x12;//屏起始坐标对应IC的起始坐标偏移
	
	WriteComm(0xb0+y%PAGE_MAX);
	WriteComm((x0&0xf0)>>4|0x10);		  // Set Higher Column Start Address for Page Addressing Mode
	WriteComm(x0&0x0f);  
}

void DispColor(u32_t total, u8_t color)
{
	u32_t i,remain;      

	gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);
	
	while(1)
	{
		if(total <= LCD_DATA_LEN)
			remain = total;
		else
			remain = LCD_DATA_LEN;
		
		for(i=0;i<remain;i++)
		{
			lcd_data_buffer[i] = color;
		}
		
		LCD_SPI_Transceive(lcd_data_buffer, remain, NULL, 0);

		if(remain == total)
			break;

		total -= remain;
	}

	gpio_pin_write(gpio_lcd, CS, 1);
}

void DispDate(u32_t total, u8_t *data)
{
	u32_t i,remain;      

	gpio_pin_write(gpio_lcd, RS, 1);
	gpio_pin_write(gpio_lcd, CS, 0);
	
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

	gpio_pin_write(gpio_lcd, CS, 1);
}

//测试函数（显示RGB条纹）
void DispBand(void)	 
{
	u32_t i;
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

	WriteDispData(0x00);

	for(i=0;i<COL-2;i++)
	{
		WriteDispData(0xFF);
	}
	
	WriteDispData(0x1F);

	for(j=0;j<ROW-2;j++)
	{
		WriteDispData(0x00);
 
		for(i=0;i<COL-2;i++)
		{
			WriteDispData(0x00);
		}

		WriteDispData(0x1f);
	}

	WriteDispData(0x00);
 
	for(i=0;i<COL-2;i++)
	{
		WriteDispData(0xff);
	}

	WriteDispData(0x1f);
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
void LCD_Clear(u16_t color)
{
	u8_t page,data;

	if(color==BLACK)
		data = 0x00;
	else
		data = 0xff;
	
	for(page=0;page<PAGE_MAX;page++)
	{
		WriteComm(0xb0+page);
		WriteComm(0x11);
		WriteComm(0x02);

		DispColor(COL,data);
	}
} 

//屏幕睡眠
void LCD_SleepIn(void)
{
	if(lcd_is_sleeping)
		return;

	//关闭背光
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_Off();
#else
	//gpio_pin_write(gpio_lcd, LEDK, 1);
	gpio_pin_write(gpio_lcd, LEDA, 0);
#endif

	WriteComm(0xae);
	
	lcd_is_sleeping = true;
}

//屏幕唤醒
void LCD_SleepOut(void)
{
	u16_t bk_time;
	
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

		k_timer_start(&backlight_timer, K_SECONDS(bk_time), NULL);
	}

	if(!lcd_is_sleeping)
		return;
	
	WriteComm(0xaf);
	
	//点亮背光
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#else
	//gpio_pin_write(gpio_lcd, LEDK, 0);
	gpio_pin_write(gpio_lcd, LEDA, 1);                                                                                                         
#endif

	lcd_is_sleeping = false;
}

//LCD初始化函数
void LCD_Init(void)
{
	int err;
	
	printk("LCD_Init\n");
	
  	//端口初始化
  	gpio_lcd = device_get_binding(LCD_PORT);
	if(!gpio_lcd)
	{
		printk("Cannot bind gpio device\n");
		return;
	}

	gpio_pin_configure(gpio_lcd, LEDA, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, CS, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RST, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RS, GPIO_DIR_OUT);
	
	LCD_SPI_Init();

	gpio_pin_write(gpio_lcd, CS, 1);
	gpio_pin_write(gpio_lcd, RST, 0);
	Delay(30);
	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(30);

	WriteComm(0xAE); //Set Display Off
	WriteComm(0xD5); //Display divide ratio/osc. freq. mode
	WriteComm(0x51);
	WriteComm(0xA8); //Multiplex ration mode:
	WriteComm(0x1F);
	WriteComm(0xD3); //Set Display Offset
	WriteComm(0x10);
	WriteComm(0x40); //Set Display Start Line
	WriteComm(0xAD); //DC-DC Control Mode Set
	WriteComm(0x8B); //DC-DC ON/OFF Mode Set
	WriteComm(0x31); //Set Pump voltage value
	WriteComm(0xA1); //Segment Remap
	WriteComm(0xC8); //Sst COM Output Scan Direction
	WriteComm(0xDA); //Common pads hardware: alternative
	WriteComm(0x12);
	WriteComm(0x81); //Contrast control
	WriteComm(0xA0);
	WriteComm(0xD9); //Set pre-charge period
	WriteComm(0x22);
	WriteComm(0xDB); //VCOM deselect level mode
	WriteComm(0x25);
	WriteComm(0xA4); //Set Entire Display On/Off
	WriteComm(0xA6); //Set Normal Display

	LCD_Clear(BLACK);

	WriteComm(0xAF); //Set Display On

	//点亮背光
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#else
	//gpio_pin_write(gpio_lcd, LEDK, 0);
	gpio_pin_write(gpio_lcd, LEDA, 1);
#endif

	lcd_is_sleeping = false;

	k_timer_init(&backlight_timer, backlight_timer_handler, NULL);

	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), NULL);
}

#endif/*LCD_R108101_GC9307*/
