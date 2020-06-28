#include <drivers/spi.h>
#include <drivers/gpio.h>

#include "lcd.h"
#include "font.h" 

#ifdef LCD_ORCT012210N_ST7789V2
#include "LCD_ORCT012210N_ST7789V2.h"

#define SPI_DEV "SPI_3"
#define SPI_BUF_LEN	8

struct device *spi_lcd;
struct device *gpio_lcd;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

static u8_t tx_buffer[SPI_BUF_LEN] = {0};
static u8_t rx_buffer[SPI_BUF_LEN] = {0};

bool lcd_is_sleeping = true;

static void spi_init(void)
{
	printk("spi_init\n");
	
	spi_lcd = device_get_binding(SPI_DEV);
	if (!spi_lcd) 
	{
		printk("Could not get %s device\n", SPI_DEV);
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	spi_cfg.frequency = 4000000;
	spi_cfg.slave = 0;

	spi_cs_ctr.gpio_dev = device_get_binding(LCD_PORT);
	if (!spi_cs_ctr.gpio_dev)
	{
		printk("Unable to get GPIO SPI CS device\n");
		return;
	}

	spi_cs_ctr.gpio_pin = CS;
	spi_cs_ctr.delay = 0U;
	spi_cfg.cs = &spi_cs_ctr;
}

//LCD延时函数
void Delay(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

//数据接口函数
//i:8位数据
void Write_Data(uint8_t i) 
{	
	struct spi_buf_set tx_bufs;
	struct spi_buf tx_buff;

	tx_buffer[0] = i;
	tx_buff.buf = tx_buffer;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;
	spi_write(spi_lcd, &spi_cfg, &tx_bufs);
}

//----------------------------------------------------------------------
//写寄存器函数
//i:寄存器值
void WriteComm(u8_t i)
{
	gpio_pin_write(gpio_lcd, RS, 0);
	Write_Data(i);
}

//写LCD数据
//i:要写入的值
void WriteData(u8_t i)
{
	gpio_pin_write(gpio_lcd, RS, 1);
	Write_Data(i);  
}

void WriteDispData(u8_t DataH,u8_t DataL)
{
	struct spi_buf_set tx_bufs;
	struct spi_buf tx_buff;

	tx_buffer[0] = DataH;
	tx_buffer[1] = DataL;
	
	tx_buff.buf = tx_buffer;
	tx_buff.len = 2;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;
	spi_write(spi_lcd, &spi_cfg, &tx_bufs);
}

//LCD画点函数
//color:要填充的颜色
void WriteOneDot(unsigned int color)
{ 
	gpio_pin_write(gpio_lcd, RS, 1);
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

	gpio_pin_write(gpio_lcd, RS, 1);

	for(i=0;i<ROW;i++)
	{
		for(j=0;j<COL;j++)
		{    
			WriteDispData(color>>8, color);
		}
	}
}

//测试函数（显示RGB条纹）
void DispBand(void)	 
{
	unsigned int i,j,k;
	unsigned int color[8]={0xf800,0xf800,0x07e0,0x07e0,0x001f,0x001f,0xffff,0xffff};//0x94B2

	BlockWrite(0,0,COL-1,ROW-1);

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
}

//测试函数（画边框）
void DispFrame(void)
{
	unsigned int i,j;

	BlockWrite(0,0,COL-1,ROW-1);

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

	BlockWrite(0,0,COL-1,ROW-1);//定位

	gpio_pin_write(gpio_lcd, RS, 1);

	for(index=0;index<totalpoint;index++)
	{
		WriteDispData(color>>8, color);
	}
} 

//屏幕睡眠
void LCD_SleepIn(void)
{
	if(lcd_is_sleeping)
		return;
	
	WriteComm(0x28);	
	WriteComm(0x10);  		//Sleep in	
	Delay(120);             //延时120ms

	//关闭背光
	//gpio_pin_write(gpio_lcd, LEDK, 1);
	gpio_pin_write(gpio_lcd, LEDA, 0);

	lcd_is_sleeping = true;
}

//屏幕唤醒
void LCD_SleepOut(void)
{
	if(!lcd_is_sleeping)
		return;
	
	WriteComm(0x11);  		//Sleep out	
	Delay(120);             //延时120ms
	WriteComm(0x29);

	//点亮背光
	//gpio_pin_write(gpio_lcd, LEDK, 0);
	gpio_pin_write(gpio_lcd, LEDA, 1);                                                                                                         
	
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
	gpio_pin_configure(gpio_lcd, VDD, GPIO_DIR_OUT);

	gpio_pin_write(gpio_lcd, VDD, 1);
	
	spi_init();

	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(10);
	gpio_pin_write(gpio_lcd, RST, 0);
	Delay(10);
	gpio_pin_write(gpio_lcd, RST, 1);
	Delay(120);

	WriteComm(0x11);     //Sleep out
	Delay(120);          //Delay 120ms

	//--ST7789S Frame rate setting--// 
	//--Display Setting--//
	WriteComm(0x36);
	WriteData(0x00);
	WriteComm(0x3a);
	WriteData(0x55);
	WriteComm(0x21);
	WriteComm(0x2a);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0xef);
	WriteComm(0x2b);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0xef);
	//--ST7789V Frame rate setting--//
	WriteComm(0xb2);
	WriteData(0x0c);
	WriteData(0x0c);
	WriteData(0x00);
	WriteData(0x33);
	WriteData(0x33);
	WriteComm(0xb7);
	WriteData(0x35);
	//--ST7789V Power setting--//
	WriteComm(0xbb);
	WriteData(0x1f);
	WriteComm(0xc0);
	WriteData(0x2c);
	WriteComm(0xc2);
	WriteData(0x01);
	WriteComm(0xc3);
	WriteData(0x12);
	WriteComm(0xc4);
	WriteData(0x20);
	WriteComm(0xc6);
	WriteData(0x0f);
	WriteComm(0xd0);
	WriteData(0xa4);
	WriteData(0xa1);
	//--ST7789V gamma setting--//
	WriteComm(0xe0);
	WriteData(0xd0);
	WriteData(0x08);
	WriteData(0x11);
	WriteData(0x08);
	WriteData(0x0c);
	WriteData(0x15);
	WriteData(0x39);
	WriteData(0x33);
	WriteData(0x50);
	WriteData(0x36);
	WriteData(0x13);
	WriteData(0x14);
	WriteData(0x29);
	WriteData(0x2d);
	WriteComm(0xe1);
	WriteData(0xd0);
	WriteData(0x08);
	WriteData(0x10);
	WriteData(0x08);
	WriteData(0x06);
	WriteData(0x06);
	WriteData(0x39);
	WriteData(0x44);
	WriteData(0x51);
	WriteData(0x0b);
	WriteData(0x16);
	WriteData(0x14);
	WriteData(0x2f);
	WriteData(0x31);
	//WriteComm(0xE7);
	WriteComm(0x29);
	WriteComm(0x2C);

	//点亮背光
	//gpio_pin_write(gpio_lcd, LEDK, 0);
	gpio_pin_write(gpio_lcd, LEDA, 1);

	lcd_is_sleeping = false;

	LCD_Clear(BLACK);		//清屏为黑色
}

#endif
