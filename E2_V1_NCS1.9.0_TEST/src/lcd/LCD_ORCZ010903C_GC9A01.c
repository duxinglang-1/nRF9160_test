#include <drivers/spi.h>
#include <drivers/gpio.h>
#include "lcd.h"
#include "font.h"
#include "settings.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif

#ifdef LCD_ORCZ010903C_GC9A01
#include "LCD_ORCZ010903C_GC9A01.h"

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

void SpiLcd_CS_LOW(void)
{
	gpio_pin_set(gpio_lcd, CS, 0);
}
void SpiLcd_CS_HIGH(void)
{
	gpio_pin_set(gpio_lcd, CS, 1);
}
static void LCD_SPI_Init(void)
{
	spi_lcd = device_get_binding(LCD_DEV);
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

	SpiLcd_CS_LOW();
	err = spi_transceive(spi_lcd, &spi_cfg, &tx_bufs, &rx_bufs);
	SpiLcd_CS_HIGH();
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
	gpio_pin_set(gpio_lcd, RS, 0); //gpio_pi_write() reworked to gpio_pin_set(), see zephyr release note 2.2.1 (https://docs.zephyrproject.org/latest/releases/release-notes-2.2.html)
	Write_Data(i);
}

//дLCD����
//i:Ҫд����
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
//color:Ҫ����������
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



	WriteComm(0x28);	
	WriteComm(0x10);  		//Sleep in	
	Delay(120);             //��ʱ120ms

	lcd_is_sleeping = true;
}

//��Ļ����
void LCD_SleepOut(void)
{
	if(k_timer_remaining_get(&backlight_timer) > 0)
		k_timer_stop(&backlight_timer);
	
	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);

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
	if(bl_mode == LCD_BL_ALWAYS_ON)
		return;
	
	if(k_timer_remaining_get(&backlight_timer) > 0)
		k_timer_stop(&backlight_timer);
	
	if(global_settings.backlight_time != 0)
		k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);
}

//��Ļ����ģʽ����
void LCD_Set_BL_Mode(LCD_BL_MODE mode)
{
	if(bl_mode == mode)
		return;
	
	switch(mode)
	{
	case LCD_BL_ALWAYS_ON:
		if(lcd_is_sleeping)
			LCD_SleepOut();
		if(k_timer_remaining_get(&backlight_timer) > 0)
			k_timer_stop(&backlight_timer);
		break;

	case LCD_BL_AUTO:
		if(k_timer_remaining_get(&backlight_timer) > 0)
			k_timer_stop(&backlight_timer);
		if(global_settings.backlight_time != 0)
			k_timer_start(&backlight_timer, K_SECONDS(global_settings.backlight_time), K_NO_WAIT);
		break;

	case LCD_BL_OFF:
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
  	gpio_lcd = device_get_binding(LCD_PORT);
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
	WriteData(0x48);

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

	LCD_Clear(BLACK);		//����Ϊ��ɫ
	Delay(30);

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

#endif
