/****************************************Copyright (c)************************************************
** File Name:			    LCD_VGM068A4W01_SH1106G.c
** Descriptions:			The VGM068A4W01_SH1106G screen drive source file
** Created By:				xie biao
** Created Date:			2021-01-07
** Modified Date:      		2021-01-07
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include "lcd.h"
#include "font.h" 
#include "settings.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif

#ifdef LCD_VGM096064A6W01_SP5090
#include "LCD_VGM096064A6W01_SP5090.h"

#define SPI_BUF_LEN	8

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

	err = spi_transceive(spi_lcd, &spi_cfg, &tx_bufs, &rx_bufs);
	if(err)
	{
	}

}

//LCD��ʱ����
void Delay(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

static void backlight_timer_handler(struct k_timer *timer)
{
	lcd_sleep_in = true;
}

//���ݽӿں���
//i:8λ����
void Write_Data(uint8_t i) 
{	
	lcd_data_buffer[0] = i;
	
	LCD_SPI_Transceive(lcd_data_buffer, 1, NULL, 0);
}

//----------------------------------------------------------------------
//д�Ĵ�������
//i:�Ĵ���ֵ
void WriteComm(uint8_t i)
{
	gpio_pin_set(gpio_lcd, CS, 0);
	gpio_pin_set(gpio_lcd, RS, 0);

	Write_Data(i);

	gpio_pin_set(gpio_lcd, CS, 1);
}

//дLCD����
//i:Ҫд���ֵ
void WriteData(uint8_t i)
{
	gpio_pin_set(gpio_lcd, CS, 0);
	gpio_pin_set(gpio_lcd, RS, 1);

	Write_Data(i);

	gpio_pin_set(gpio_lcd, CS, 1);
}

void WriteDispData(uint8_t Data)
{
	gpio_pin_set(gpio_lcd, CS, 0);
	gpio_pin_set(gpio_lcd, RS, 1);

	lcd_data_buffer[0] = Data;
	
	LCD_SPI_Transceive(lcd_data_buffer, 1, NULL, 0);

	gpio_pin_set(gpio_lcd, CS, 1);
}

//LCD���㺯��
//color:Ҫ������ɫ 1Ϊ������0ΪϨ��
void WriteOneDot(uint16_t color)
{ 
	WriteDispData((uint8_t)(0x00ff&color));
}

////////////////////////////////////////////////���Ժ���//////////////////////////////////////////
void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h) //reentrant
{
	uint8_t x0;
	
	if(x >= COL)
		x = 0;
	if(y >= ROW)
		y = 0;

	x0 = x+0x12;//����ʼ�����ӦIC����ʼ����ƫ��
	
	WriteComm(0xb0+y%PAGE_MAX);
	WriteComm((x0&0xf0)>>4|0x10);		  // Set Higher Column Start Address for Page Addressing Mode
	WriteComm(x0&0x0f);  
}

void DispColor(uint32_t total, uint8_t color)
{
	uint32_t i,remain;      

	gpio_pin_set(gpio_lcd, CS, 0);
	gpio_pin_set(gpio_lcd, RS, 1);
	
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

	gpio_pin_set(gpio_lcd, CS, 1);
}

void DispData(uint32_t total, uint8_t *data)
{
	uint32_t i,remain;      

	gpio_pin_set(gpio_lcd, CS, 0);
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

	gpio_pin_set(gpio_lcd, CS, 1);
}

//���Ժ�������ʾRGB���ƣ�
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

//���Ժ��������߿�
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

//��������
//color:Ҫ���������ɫ
void LCD_Clear(uint16_t color)
{
	uint8_t page,data;

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

//�����
void LCD_BL_On(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#else
	gpio_pin_set(gpio_lcd, LEDA, 1);																										   
#endif
}

//����ر�
void LCD_BL_Off(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_Off();
#else
	gpio_pin_set(gpio_lcd, LEDA, 0);
#endif
}

//��Ļ˯��
void LCD_SleepIn(void)
{
	if(lcd_is_sleeping)
		return;

	WriteComm(0xae);
	
	lcd_is_sleeping = true;
}

//��Ļ����
void LCD_SleepOut(void)
{
	uint16_t bk_time;
	
	if(k_timer_remaining_get(&backlight_timer) > 0)
		k_timer_stop(&backlight_timer);

	if(global_settings.backlight_time != 0)
	{
		bk_time = global_settings.backlight_time;
		//xb add 2020-12-31 ̧������5����Զ�Ϣ��
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
	
	WriteComm(0xaf);

	lcd_is_sleeping = false;
}

//��Ļ���ñ�����ʱ
void LCD_ResetBL_Timer(void)
{
	if((bl_mode == LCD_BL_ALWAYS_ON) || (bl_mode == LCD_BL_OFF))
		return;
	
	if(k_timer_remaining_get(&backlight_timer) > 0)
		k_timer_stop(&backlight_timer);
	
	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);
}

//��ȡ��Ļ��ǰ����ģʽ
LCD_BL_MODE LCD_Get_BL_Mode(void)
{
	return bl_mode;
}

//��Ļ����ģʽ����
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

//LCD��ʼ������
void LCD_Init(void)
{
	int err;
	
  	//�˿ڳ�ʼ��
  	gpio_lcd = DEVICE_DT_GET(LCD_PORT);
	if(!gpio_lcd)
	{
		return;
	}

	gpio_pin_configure(gpio_lcd, LEDA, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, CS, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, RST, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, RS, GPIO_OUTPUT);
	
	LCD_SPI_Init();

	gpio_pin_set(gpio_lcd, CS, 1);
	gpio_pin_set(gpio_lcd, RST, 0);
	Delay(30);
	gpio_pin_set(gpio_lcd, RST, 1);
	Delay(30);

	WriteComm(0xAE); //Set Display Off
	WriteComm(0xD5); //Display divide ratio/osc. freq. mode
	WriteComm(0x51);
	WriteComm(0xA8); //Multiplex ration mode:63
	WriteComm(0x3F);
	WriteComm(0xD3); //Set Display Offset
	WriteComm(0x00);
	WriteComm(0x40); //Set Display Start Line
	WriteComm(0xAD); //DC-DC Control Mode Set
	WriteComm(0x8B); //DC-DC ON/OFF Mode Set
	WriteComm(0x33); //Set Pump voltage value
	WriteComm(0xA0); //Segment Remap
	WriteComm(0xC0); //Sst COM Output Scan Direction
	WriteComm(0xDA); //Common pads hardware: alternative
	WriteComm(0x12);
	WriteComm(0x81); //Contrast control
	WriteComm(0xA0);
	WriteComm(0xD9); //Set pre-charge period
	WriteComm(0x22);
	WriteComm(0xDB); //VCOM deselect level mode
	WriteComm(0x2B);
	WriteComm(0xA4); //Set Entire Display On/Off
	WriteComm(0xA6); //Set Normal Display

	LCD_Clear(BLACK);
	Delay(30);

	WriteComm(0xAF); //Set Display On

	//��������
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

#endif/*LCD_R108101_GC9307*/
