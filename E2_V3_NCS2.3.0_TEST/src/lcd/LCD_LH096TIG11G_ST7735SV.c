#include <drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "lcd.h"
#include "font.h"
#include "settings.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif

#ifdef LCD_LH096TIG11G_ST7735SV
#include "LCD_LH096TIG11G_ST7735SV.h"

#define SPI_BUF_LEN	8

struct device *spi_lcd;
struct device *gpio_lcd;

struct spi_buf_set tx_bufs,rx_bufs;
struct spi_buf tx_buff,rx_buff;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

static struct k_timer backlight_timer;

static LCD_BL_MODE bl_mode = LCD_BL_AUTO;

static u8_t tx_buffer[SPI_BUF_LEN] = {0};
static u8_t rx_buffer[SPI_BUF_LEN] = {0};

u8_t lcd_data_buffer[2*LCD_DATA_LEN] = {0};	//xb add 20200702 a pix has 2 byte data


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

	spi_cs_ctr.gpio_dev = device_get_binding(LCD_PORT);
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
	int ret;
	struct spi_buf_set tx_bufs;
	struct spi_buf tx_buff;

	tx_buffer[0] = i;
	tx_buff.buf = tx_buffer;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;
	ret = spi_write(spi_lcd, &spi_cfg, &tx_bufs);
	if(ret)
	{
	}
}

//----------------------------------------------------------------------
//写寄存器函数
//i:寄存器值
void WriteComm(u8_t i)
{
	//gpio_pin_write(gpio_lcd, CS, 0);//CS置0
	gpio_pin_write(gpio_lcd, RS, 0);//RS清0

	Write_Data(i);  

	//gpio_pin_write(gpio_lcd, CS, 1);
}

//写LCD数据
//i:要写入的值
void WriteData(u8_t i)
{
	//gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);
		
	Write_Data(i);  

	//gpio_pin_write(gpio_lcd, CS, 1);
}

void WriteDispData(u8_t DataH,u8_t DataL)
{
	int ret;
	struct spi_buf_set tx_bufs;
	struct spi_buf tx_buff;

	tx_buffer[0] = DataH;
	tx_buffer[1] = DataL;
	
	tx_buff.buf = tx_buffer;
	tx_buff.len = 2;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;
	ret = spi_write(spi_lcd, &spi_cfg, &tx_bufs);
	if(ret)
	{
	}
}

//LCD画点函数
//color:要填充的颜色
void WriteOneDot(unsigned int color)
{ 
	//gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);

	WriteDispData(color>>8, color);

	//gpio_pin_write(gpio_lcd, CS, 1);
}

////////////////////////////////////////////////测试函数//////////////////////////////////////////
void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h) //reentrant
{
	x += 26;
	y += 1;

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

//测试函数（显示RGB条纹）
void DispBand(void)	 
{
	unsigned int i,j,k;
	//unsigned int color[8]={0x001f,0x07e0,0xf800,0x07ff,0xf81f,0xffe0,0x0000,0xffff};
	unsigned int color[8]={0xf800,0xf800,0x07e0,0x07e0,0x001f,0x001f,0xffff,0xffff};//0x94B2
	//unsigned int gray16[]={0x0000,0x1082,0x2104,0x3186,0x42,0x08,0x528a,0x630c,0x738e,0x7bcf,0x9492,0xa514,0xb596,0xc618,0xd69a,0xe71c,0xffff};

	BlockWrite(0,0,COL,ROW);

	//gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);

	for(i=0;i<8;i++)
	{
		for(j=0;j<ROW/8;j++)
		{
			for(k=0;k<COL;k++)
			{
				WriteDispData(color[i]>>8, color[i]);
			} 
		}
	}
	for(j=0;j<(ROW%8);j++)
	{
		for(k=0;k<COL;k++)
		{
			WriteDispData(color[7]>>8, color[7]);
		} 
	}

	//gpio_pin_write(gpio_lcd, CS, 1);
}

//测试函数（画边框）
void DispFrame(void)
{
	unsigned int i,j;

	BlockWrite(0,0,COL,ROW);

	//gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);

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

	//gpio_pin_write(gpio_lcd, CS, 1);
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
	u32_t index=0;      
	u32_t totalpoint=ROW;

	totalpoint*=COL; 			//得到总点数

	BlockWrite(0,0,COL,ROW);//定位

	//gpio_pin_write(gpio_lcd, CS, 0);
	gpio_pin_write(gpio_lcd, RS, 1);

	for(index=0;index<totalpoint;index++)
	{
		WriteDispData(color>>8, color);
	}

	//gpio_pin_write(gpio_lcd, CS, 1);
} 

//背光打开
void LCD_BL_On(void)
{
	gpio_pin_write(gpio_lcd, LEDK, 0);
}

//背光关闭
void LCD_BL_Off(void)
{
	gpio_pin_write(gpio_lcd, LEDK, 1);
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
	int err;

  	//端口初始化
  	gpio_lcd = device_get_binding(LCD_PORT);
	if(!gpio_lcd)
	{
		return;
	}

	gpio_pin_configure(gpio_lcd, LEDK, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, CS, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RST, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_lcd, RS, GPIO_DIR_OUT);

	LCD_SPI_Init();

	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(10);
	gpio_pin_write(gpio_lcd, RST, 0);
	Delay(10);
	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(120);

	WriteComm(0x11);     //Sleep out
	Delay(120);          //Delay 120ms

	WriteComm(0xB1);     //Normal mode
	WriteData(0x05);   
	WriteData(0x3C);   
	WriteData(0x3C);  

	WriteComm(0xB2);     //Idle mode
	WriteData(0x05);   
	WriteData(0x3C);   
	WriteData(0x3C);  

	WriteComm(0xB3);     //Partial mode
	WriteData(0x05);   
	WriteData(0x3C);   
	WriteData(0x3C);   
	WriteData(0x05);   
	WriteData(0x3C);   
	WriteData(0x3C);  

	WriteComm(0xB4);     //Dot inversion
	WriteData(0x00); 

	WriteComm(0xB6);    //column inversion
	WriteData(0xB4); 
	WriteData(0xF0);	

	WriteComm(0xC0);     //AVDD GVDD
	WriteData(0xAB);   
	WriteData(0x0B);   
	WriteData(0x04);  

	WriteComm(0xC1);     //VGH VGL
	WriteData(0xC5);   	//C0

	WriteComm(0xC2);     //Normal Mode
	WriteData(0x0D);   
	WriteData(0x00);  

	WriteComm(0xC3);     //Idle
	WriteData(0x8D);   
	WriteData(0x6A);  

	WriteComm(0xC4);     //Partial+Full
	WriteData(0x8D);   
	WriteData(0xEE); 

	WriteComm(0xC5);     //VCOM
	WriteData(0x0F);  

	WriteComm(0x36); 	//MX,MY,RGB mode
	WriteData(0xC8); 	//my mx ml MV,rgb,000; =1,=MH=MX=MY=ML=0 and RGB filter panel 
	    
	WriteComm(0xE0);     //positive gamma
	WriteData(0x07);   
	WriteData(0x0E);   
	WriteData(0x08);   
	WriteData(0x07);   
	WriteData(0x10);   
	WriteData(0x07);   
	WriteData(0x02);   
	WriteData(0x07);   
	WriteData(0x09);   
	WriteData(0x0F);   
	WriteData(0x25);   
	WriteData(0x36);   
	WriteData(0x00);   
	WriteData(0x08);   
	WriteData(0x04);   
	WriteData(0x10); 

	WriteComm(0xE1);     //negative gamma
	WriteData(0x0A);   
	WriteData(0x0D);   
	WriteData(0x08);   
	WriteData(0x07);   
	WriteData(0x0F);   
	WriteData(0x07);   
	WriteData(0x02);   
	WriteData(0x07);   
	WriteData(0x09);   
	WriteData(0x0F);   
	WriteData(0x25);   
	WriteData(0x35);   
	WriteData(0x00);   
	WriteData(0x09);   
	WriteData(0x04);   
	WriteData(0x10);

	WriteComm(0xFC);    
	WriteData(0x80);  

	WriteComm(0xF0);    
	WriteData(0x11);  

	WriteComm(0xD6);    
	WriteData(0xCB);  

	WriteComm(0x3A);     
	WriteData(0x05);  

	WriteComm(0x21);     //Display inversion
	WriteComm(0x29);     //Display on

	LCD_Clear(BLACK);		//清屏为黑色
	Delay(30);

	//点亮背光
	gpio_pin_write(gpio_lcd, LEDK, 0);

	lcd_is_sleeping = false;	
}

#endif
