#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "lcd.h"
#include "font.h" 
#include "settings.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif

#ifdef LCD_R108101_GC9307
#include "LCD_R108101_GC9307.h"

#define SPI_BUF_LEN	8

struct device *spi_lcd;
struct device *gpio_lcd;

struct spi_buf_set tx_bufs,rx_bufs;
struct spi_buf tx_buff,rx_buff;

static struct k_timer backlight_timer;
static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

static LCD_BL_MODE bl_mode = LCD_BL_AUTO;

static u8_t tx_buffer[SPI_BUF_LEN] = {0};
static u8_t rx_buffer[SPI_BUF_LEN] = {0};

u8_t lcd_data_buffer[2*LCD_DATA_LEN] = {0};	//xb add 20200702 a pix has 2 byte data

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

	spi_cs_ctr.gpio_dev = DEVICE_DT_GET(LCD_PORT);
	if (!spi_cs_ctr.gpio_dev)
	{
		return;
	}

	spi_cs_ctr.gpio_pin = CS;
	spi_cs_ctr.delay = 0U;
	spi_cfg.cs = &spi_cs_ctr;
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
void WriteComm(u8_t i)
{
	gpio_pin_write(gpio_lcd, RS, 0);
	Write_Data(i);
}

//дLCD����
//i:Ҫд���ֵ
void WriteData(u8_t i)
{
	gpio_pin_write(gpio_lcd, RS, 1);
	Write_Data(i);  
}

void WriteDispData(u8_t DataH,u8_t DataL)
{
	gpio_pin_write(gpio_lcd, RS, 1);

	lcd_data_buffer[0] = DataH;
	lcd_data_buffer[1] = DataL;
	
	LCD_SPI_Transceive(lcd_data_buffer, 2, NULL, 0);
}

//LCD���㺯��
//color:Ҫ������ɫ
void WriteOneDot(unsigned int color)
{ 
	WriteDispData(color>>8, color);
}

////////////////////////////////////////////////���Ժ���//////////////////////////////////////////
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

void DispColor(u32_t total, u16_t color)
{
	u32_t i,remain;      

	gpio_pin_write(gpio_lcd, RS, 1);
	
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

void DispData(u32_t total, u8_t *data)
{
	u32_t i,remain;      

	gpio_pin_write(gpio_lcd, RS, 1);
	
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

//���Ժ�������ʾRGB���ƣ�
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

//���Ժ��������߿�
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

//��������
//color:Ҫ���������ɫ
void LCD_Clear(uint16_t color)
{
	BlockWrite(0,0,COL,ROW);//��λ

	DispColor(COL*ROW, color);
} 

//�����
void LCD_BL_On(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_On();
#else
	gpio_pin_write(gpio_lcd, LEDA, 1);																										   
#endif
}

//����ر�
void LCD_BL_Off(void)
{
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
	Set_Screen_Backlight_Off();
#else
	gpio_pin_write(gpio_lcd, LEDA, 0);
#endif
}

//��Ļ˯��
void LCD_SleepIn(void)
{
	if(lcd_is_sleeping)
		return;

	WriteComm(0x28);	
	WriteComm(0x10);  		//Sleep in	
	Delay(120);             //��ʱ120ms

	lcd_is_sleeping = true;
}

//��Ļ����
void LCD_SleepOut(void)
{
	u16_t bk_time;
	
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
	
	WriteComm(0x11);  		//Sleep out	
	Delay(120);             //��ʱ120ms
	WriteComm(0x29);

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
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), NULL);
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

	gpio_pin_configure(gpio_lcd, LEDA, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, CS, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RST, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RS, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, VDD, GPIO_DIR_OUT);

	gpio_pin_write(gpio_lcd, VDD, 1);
	
	LCD_SPI_Init();

	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(10);
	gpio_pin_write(gpio_lcd, RST, 0);
	Delay(10);
	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(120);

	WriteComm(0x11);     //Sleep out
	Delay(120);          //Delay 120ms

	WriteComm(0xfe);
	WriteComm(0xef);	
			
	WriteComm(0x36);	
	WriteData(0x48);	
	WriteComm(0x3a);	
	WriteData(0x05);	
		
	WriteComm(0x86);	
	WriteData(0x98);	
	WriteComm(0x89);	
	WriteData(0x03);
		
	WriteComm(0x8b);	
	WriteData(0x80);	
		
	WriteComm(0x8d);	
	WriteData(0x33);	
	WriteComm(0x8e);	
	WriteData(0x8f);	

	//inversion
	WriteComm(0xe8);
	WriteData(0x12);
	WriteData(0x00);	

	WriteComm(0xc3);	
	WriteData(0x20);

	WriteComm(0xc4);	
	WriteData(0x30);

	WriteComm(0xc9);	
	WriteData(0x08);

	WriteComm(0xff);
	WriteData(0x62);

	WriteComm(0x99);	
	WriteData(0x3e);
	WriteComm(0x9d);	
	WriteData(0x4b);
	WriteComm(0x98);	
	WriteData(0x3e);
	WriteComm(0x9c);	
	WriteData(0x4b);

	WriteComm(0xf0);
	WriteData(0x13);
	WriteData(0x14);
	WriteData(0x07);
	WriteData(0x05);
	WriteData(0xf0);
	WriteData(0x29);

	WriteComm(0xf1);
	WriteData(0x3e);
	WriteData(0x92);
	WriteData(0x90);
	WriteData(0x21);
	WriteData(0x23);
	WriteData(0x9f);

	WriteComm(0xf2);
	WriteData(0x13);
	WriteData(0x14);
	WriteData(0x07);
	WriteData(0x05);
	WriteData(0xf0);
	WriteData(0x29);

	WriteComm(0xf3);
	WriteData(0x3e);
	WriteData(0x92);
	WriteData(0x90);
	WriteData(0x21);
	WriteData(0x23);
	WriteData(0x9f);

	WriteComm(0x11);
	Delay(120);
	WriteComm(0x29);
	WriteComm(0x2c);

	LCD_Clear(BLACK);		//����Ϊ��ɫ
	Delay(30);

	//��������
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
