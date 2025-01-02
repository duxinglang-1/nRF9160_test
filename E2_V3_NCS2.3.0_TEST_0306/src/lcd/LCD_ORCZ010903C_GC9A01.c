#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "lcd.h"
#include "font.h"
#include "settings.h"
#include "logger.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif
#ifdef LCD_ORCZ010903C_GC9A01
#include "LCD_ORCZ010903C_GC9A01.h"

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
		printk("Unable to get GPIO SPI CS device\n");
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
	
	tx_buff.buf = txbuf;
	tx_buff.len = txbuflen;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	rx_buff.buf = rxbuf;
	rx_buff.len = rxbuflen;
	rx_bufs.buffers = &rx_buff;
	rx_bufs.count = 1;

#ifdef SPI_MUIT_BY_CS
	LCD_CS_LOW();
#endif
	err = spi_transceive(spi_lcd, &spi_cfg, &tx_bufs, &rx_bufs);
#ifdef SPI_MUIT_BY_CS
	LCD_CS_HIGH();
#endif
	if(err)
	{
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
	uint8_t buf[4] = {0};
	
	WriteComm(0x2A);
	buf[0] = x>>8;
	buf[1] = x;
	buf[2] = (x+w-1)>>8;
	buf[3] = (x+w-1);
	DispData(4, buf);

	WriteComm(0x2B);
	buf[0] = y>>8;
	buf[1] = y;
	buf[2] = (y+h-1)>>8;
	buf[3] = (y+h-1);
	DispData(4, buf);
	
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
	uint32_t i,remain,sent=0;      

	gpio_pin_set(gpio_lcd, RS, 1);
	
	while(1)
	{
		if(total <= 2*LCD_DATA_LEN)
			remain = total;
		else
			remain = 2*LCD_DATA_LEN;

		memcpy(lcd_data_buffer, &data[sent], remain);
		LCD_SPI_Transceive(lcd_data_buffer, remain, NULL, 0);

		if(remain == total)
			break;

		total -= remain;
		sent += remain;
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
#else
	gpio_pin_set(gpio_lcd, LEDA, 1);																											 
#endif	
}

//背光关闭
void LCD_BL_Off(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_Off();
#else
	gpio_pin_set(gpio_lcd, LEDA, 0);
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

  	//端口初始化
  	gpio_lcd = DEVICE_DT_GET(LCD_PORT);
	if(!gpio_lcd)
	{
		return;
	}

	gpio_pin_configure(gpio_lcd, LEDA, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, CS, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, RST, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, RS, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, VDD, GPIO_OUTPUT);

	gpio_pin_set(gpio_lcd, VDD, 1);
	
	LCD_SPI_Init();

	gpio_pin_set(gpio_lcd, RST, 1);
	Delay(10);
	gpio_pin_set(gpio_lcd, RST, 0);
	Delay(10);
	gpio_pin_set(gpio_lcd, RST, 1);
	Delay(120);

	WriteComm(0xFE);			 
	WriteComm(0xEF); 

	WriteComm(0xEB);	
	WriteData(0x14); 

	WriteComm(0x84);			
	WriteData(0x40); 

	WriteComm(0x88);			
	WriteData(0x0A);

	WriteComm(0x89);			
	WriteData(0x21); 

	WriteComm(0x8A);			
	WriteData(0x00); 

	WriteComm(0x8B);			
	WriteData(0x80); 

	WriteComm(0x8C);			
	WriteData(0x01); 

	WriteComm(0x8D);			
	WriteData(0x01); 

	WriteComm(0xB6);			
	WriteData(0x00); 
	WriteData(0x00);

	WriteComm(0x36);
#ifdef LCD_SHOW_ROTATE_180	
	WriteData(0x88);
#else
	WriteData(0x48);
#endif

	WriteComm(0x3A);			
	WriteData(0x05); 

	WriteComm(0x90);			
	WriteData(0x08);
	WriteData(0x08);
	WriteData(0x08);
	WriteData(0x08); 

	WriteComm(0xBD);			
	WriteData(0x06);

	WriteComm(0xBC);			
	WriteData(0x00);	

	WriteComm(0xFF);			
	WriteData(0x60);
	WriteData(0x01);
	WriteData(0x04);

	WriteComm(0xC3);			
	WriteData(0x2F);
	WriteComm(0xC4);			
	WriteData(0x2F);

	WriteComm(0xC9);			
	WriteData(0x22);

	WriteComm(0xBE);			
	WriteData(0x11); 

	WriteComm(0xE1);			
	WriteData(0x10);
	WriteData(0x0E);

	WriteComm(0xDF);			
	WriteData(0x21);
	WriteData(0x0c);
	WriteData(0x02);

	WriteComm(0xF0);   
	WriteData(0x45);
	WriteData(0x09);
	WriteData(0x08);
	WriteData(0x08);
	WriteData(0x26);
	WriteData(0x2A);

	WriteComm(0xF1);	
	WriteData(0x43);
	WriteData(0x70);
	WriteData(0x72);
	WriteData(0x36);
	WriteData(0x37);  
	WriteData(0x6F);

	WriteComm(0xF2);   
	WriteData(0x45);
	WriteData(0x09);
	WriteData(0x08);
	WriteData(0x08);
	WriteData(0x26);
	WriteData(0x2A);

	WriteComm(0xF3);   
	WriteData(0x43);
	WriteData(0x70);
	WriteData(0x72);
	WriteData(0x36);
	WriteData(0x37); 
	WriteData(0x6F);

	WriteComm(0xED);	
	WriteData(0x1B); 
	WriteData(0x0B); 

	WriteComm(0xAE);			
	WriteData(0x77);

	WriteComm(0xCD);			
	WriteData(0x63);		

	WriteComm(0x70);			
	WriteData(0x07);
	WriteData(0x07);
	WriteData(0x04);
	WriteData(0x06); 
	WriteData(0x0F); 
	WriteData(0x09);
	WriteData(0x07);
	WriteData(0x08);
	WriteData(0x03);

	WriteComm(0xE8);			
	WriteData(0x34); 

	WriteComm(0x62);			
	WriteData(0x18);
	WriteData(0x0D);
	WriteData(0x71);
	WriteData(0xED);
	WriteData(0x70); 
	WriteData(0x70);
	WriteData(0x18);
	WriteData(0x0F);
	WriteData(0x71);
	WriteData(0xEF);
	WriteData(0x70); 
	WriteData(0x70);

	WriteComm(0x63);			
	WriteData(0x18);
	WriteData(0x11);
	WriteData(0x71);
	WriteData(0xF1);
	WriteData(0x70); 
	WriteData(0x70);
	WriteData(0x18);
	WriteData(0x13);
	WriteData(0x71);
	WriteData(0xF3);
	WriteData(0x70); 
	WriteData(0x70);

	WriteComm(0x64);			
	WriteData(0x28);
	WriteData(0x29);
	WriteData(0xF1);
	WriteData(0x01);
	WriteData(0xF1);
	WriteData(0x00);
	WriteData(0x07);

	WriteComm(0x66);			
	WriteData(0x3C);
	WriteData(0x00);
	WriteData(0xCD);
	WriteData(0x67);
	WriteData(0x45);
	WriteData(0x45);
	WriteData(0x10);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0x00);

	WriteComm(0x67);			
	WriteData(0x00);
	WriteData(0x3C);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0x01);
	WriteData(0x54);
	WriteData(0x10);
	WriteData(0x32);
	WriteData(0x98);

	WriteComm(0x74);			
	WriteData(0x10);	
	WriteData(0x85);	
	WriteData(0x80);
	WriteData(0x00); 
	WriteData(0x00); 
	WriteData(0x4E);
	WriteData(0x00);					

	WriteComm(0x98);			
	WriteData(0x3e);
	WriteData(0x07);

	WriteComm(0x35);	
	WriteComm(0x21);
	Delay(10);

	WriteComm(0x11);
	Delay(120);
	WriteComm(0x29);
	Delay(20);
	WriteComm(0x2C);	

	LCD_Clear(BLACK);		//清屏为黑色
	Delay(30);

	//点亮背光
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#else
	//gpio_pin_set(gpio_lcd, LEDK, 0);
	gpio_pin_set(gpio_lcd, LEDA, 1);
#endif

	lcd_is_sleeping = false;

	k_timer_init(&backlight_timer, backlight_timer_handler, NULL);

	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);	
}

#endif
