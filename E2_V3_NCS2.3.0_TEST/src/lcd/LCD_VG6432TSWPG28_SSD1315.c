/****************************************Copyright (c)************************************************
** File Name:			    LCD_VG6432TSWPG28_SSD1315.c
** Descriptions:			The LCD_VG6432TSWPG28_SSD1315 screen drive source file
** Created By:				xie biao
** Created Date:			2025-01-16
** Modified Date:      		2025-01-16
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#include "lcd.h"
#include "font.h" 
#include "settings.h"
#ifdef LCD_BACKLIGHT_CONTROLED_BY_PMU
#include "Max20353.h"
#endif
#include "logger.h"

#ifdef LCD_VG6432TSWPG28_SSD1315
#include "LCD_VG6432TSWPG28_SSD1315.h"

#define GPIO_ACT_I2C

#ifdef GPIO_ACT_I2C
#define LCD_SCL		1
#define LCD_SDA		0

#else/*GPIO_ACT_I2C*/

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define LCD_DEV DT_NODELABEL(i2c1)
#else
#error "i2c1 devicetree node is disabled"
#define LCD_DEV	""
#endif

#define LCD_SCL		31
#define LCD_SDA		30
#endif/*GPIO_ACT_I2C*/

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define LCD_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define LCD_PORT	""
#endif

struct device *i2c_lcd;
struct device *gpio_lcd;

static struct k_timer backlight_timer;

static LCD_BL_MODE bl_mode = LCD_BL_AUTO;

static uint8_t tx_buffer[8] = {0};
static uint8_t rx_buffer[8] = {0};

uint8_t lcd_data_buffer[2*LCD_DATA_LEN] = {0};

static void Delay_ms(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

static void Delay_us(unsigned int dly)
{
	k_usleep(dly);
}

#ifdef GPIO_ACT_I2C
static void I2C_INIT(void)
{
	if(gpio_lcd == NULL)
		gpio_lcd = DEVICE_DT_GET(LCD_PORT);

	gpio_pin_configure(gpio_lcd, LCD_SCL, GPIO_OUTPUT);
	gpio_pin_configure(gpio_lcd, LCD_SDA, GPIO_OUTPUT);
	gpio_pin_set(gpio_lcd, LCD_SCL, 1);
	gpio_pin_set(gpio_lcd, LCD_SDA, 1);
}

static void I2C_SDA_OUT(void)
{
	gpio_pin_configure(gpio_lcd, LCD_SDA, GPIO_OUTPUT);
}

static void I2C_SDA_IN(void)
{
	gpio_pin_configure(gpio_lcd, LCD_SDA, GPIO_INPUT);
}

static void I2C_SDA_H(void)
{
	gpio_pin_set(gpio_lcd, LCD_SDA, 1);
}

static void I2C_SDA_L(void)
{
	gpio_pin_set(gpio_lcd, LCD_SDA, 0);
}

static void I2C_SCL_H(void)
{
	gpio_pin_set(gpio_lcd, LCD_SCL, 1);
}

static void I2C_SCL_L(void)
{
	gpio_pin_set(gpio_lcd, LCD_SCL, 0);
}

//产生起始信号
static void I2C_Start(void)
{
	I2C_SDA_OUT();

	I2C_SDA_H();
	I2C_SCL_H();
	I2C_SDA_L();
	I2C_SCL_L();
}

//产生停止信号
static void I2C_Stop(void)
{
	I2C_SDA_OUT();

	I2C_SCL_L();
	I2C_SDA_L();
	I2C_SCL_H();
	I2C_SDA_H();
}

//主机产生应答信号ACK
static void I2C_Ack(void)
{
	I2C_SDA_OUT();
	
	I2C_SDA_L();
	I2C_SCL_L();
	I2C_SCL_H();
	I2C_SCL_L();
}

//主机不产生应答信号NACK
static void I2C_NAck(void)
{
	I2C_SDA_OUT();
	
	I2C_SDA_H();
	I2C_SCL_L();
	I2C_SCL_H();
	I2C_SCL_L();
}

//等待从机应答信号
//返回值：1 接收应答失败
//		  0 接收应答成功
static uint8_t I2C_Wait_Ack(void)
{
	uint8_t val,tempTime=0;

	I2C_SDA_IN();
	I2C_SCL_H();

	while(1)
	{
		val = gpio_pin_get_raw(gpio_lcd, LCD_SDA);
		if(val == 0)
			break;
		
		tempTime++;
		if(tempTime>250)
		{
			I2C_Stop();
			return 1;
		}	 
	}

	I2C_SCL_L();
	return 0;
}

//I2C 发送一个字节
static uint8_t I2C_Write_Byte(uint8_t txd)
{
	uint8_t i=0;

	I2C_SDA_OUT();
	I2C_SCL_L();//拉低时钟开始数据传输

	for(i=0;i<8;i++)
	{
		if((txd&0x80)>0) //0x80  1000 0000
			I2C_SDA_H();
		else
			I2C_SDA_L();

		txd<<=1;
		I2C_SCL_H();
		I2C_SCL_L();
	}

	return I2C_Wait_Ack();
}

//I2C 读取一个字节
static void I2C_Read_Byte(bool ack, uint8_t *data)
{
	uint8_t i=0,receive=0,val=0;

	I2C_SDA_IN();
	I2C_SCL_L();

	for(i=0;i<8;i++)
	{
		I2C_SCL_H();
		receive<<=1;
		val = gpio_pin_get_raw(gpio_lcd, LCD_SDA);
		if(val == 1)
			receive++;
		I2C_SCL_L();
	}

	if(ack == false)
		I2C_NAck();
	else
		I2C_Ack();

	*data = receive;
}

static uint8_t I2C_write_data(uint8_t addr, uint8_t *databuf, uint32_t len)
{
	uint32_t i;

	addr = (addr<<1);

	I2C_Start();
	if(I2C_Write_Byte(addr))
		goto err;

	for(i=0;i<len;i++)
	{
		if(I2C_Write_Byte(databuf[i]))
			goto err;
	}

	I2C_Stop();
	return 0;
	
err:
	return -1;
}

static uint8_t I2C_read_data(uint8_t addr, uint8_t *databuf, uint32_t len)
{
	uint32_t i;

	addr = (addr<<1)|1;

	I2C_Start();
	if(I2C_Write_Byte(addr))
		goto err;

	for(i=0;i<len;i++)
	{
		if(i == len-1)
			I2C_Read_Byte(false, &databuf[i]);
		else
			I2C_Read_Byte(true, &databuf[i]);
	}
	I2C_Stop();
	return 0;
	
err:
	return -1;
}
#endif

static bool LCD_I2C_Init(void)
{
#ifdef GPIO_ACT_I2C
	I2C_INIT();
	return true;
#else
	i2c_lcd = DEVICE_DT_GET(LCD_DEV);
	if(!i2c_lcd)
	{
		return false;
	} 
	else
	{
		i2c_configure(i2c_lcd, I2C_SPEED_SET(I2C_SPEED_FAST));
		return true;
	}

#if 0
	LOGD("Value of NRF_TWIM1_NS->PSEL.SCL: %ld",NRF_TWIM1_NS->PSEL.SCL);
	LOGD("Value of NRF_TWIM1_NS->PSEL.SDA: %ld",NRF_TWIM1_NS->PSEL.SDA);
	LOGD("Value of NRF_TWIM1_NS->FREQUENCY: %ld",NRF_TWIM1_NS->FREQUENCY);
	LOGD("26738688 -> 100k");
	LOGD("67108864 -> 250k");
	LOGD("104857600 -> 400k");

	for(uint8_t i = 0; i < 0x7f; i++)
	{
		struct i2c_msg msgs[1];
		uint8_t dst = 1;
		int err;

		/* Send the address to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 1U;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		err = i2c_transfer(i2c_lcd, &msgs[0], 1, i);
		if(err == 0)
		{
		#if 1
			LOGD("0x%2x device address found on I2C Bus", i);
			break;
		#endif
		}
		else
		{
		}
	}

	return true;
#endif

#endif
}

static void LCD_I2C_Write(uint8_t *buf, uint32_t len)
{
#ifdef GPIO_ACT_I2C
	I2C_write_data(LCD_I2C_ADDR, buf, len);
#else
	i2c_write(i2c_lcd, buf, len, LCD_I2C_ADDR);
#endif
}

static void backlight_timer_handler(struct k_timer *timer)
{
	lcd_sleep_in = true;
}

//----------------------------------------------------------------------
//写寄存器函数
//i:寄存器值
void WriteComm(uint8_t i)
{
	uint8_t data[2];

	data[0] = 0x00;
	data[1] = i;
	LCD_I2C_Write(data, 2);
}

//写LCD数据
//i:要写入的值
void WriteData(uint8_t i)
{
	uint8_t data[2];
	
	data[0] = 0x40;
	data[1] = i;
	LCD_I2C_Write(data, 2);
}

void WriteDispData(uint8_t Data)
{
	WriteData(Data);
}

//LCD画点函数
//color:要填充的颜色 1为点亮，0为熄灭
void WriteOneDot(uint16_t color)
{ 
	WriteDispData((uint8_t)(0x00ff&color));
}

////////////////////////////////////////////////测试函数//////////////////////////////////////////
void BlockWrite(unsigned int x,unsigned int y,unsigned int w,unsigned int h) //reentrant
{
	uint8_t x0;
	
	if(x >= COL)
		x = 0;
	if(y >= ROW)
		y = 0;

	x0 = x+0x20;//屏起始坐标对应IC的起始坐标偏移
	
	WriteComm(0xb0+y%PAGE_MAX);
	WriteComm((x0&0xf0)>>4|0x10);		  // Set Higher Column Start Address for Page Addressing Mode
	WriteComm(x0&0x0f);  
}

void DispColor(uint32_t total, uint8_t color)
{
	uint32_t i,remain;      

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
		
		LCD_I2C_Write(lcd_data_buffer, remain);

		if(remain == total)
			break;

		total -= remain;
	}
}

void DispData(uint32_t total, uint8_t *data)
{
	uint32_t i,remain;      

	while(1)
	{
		if(total <= 2*LCD_DATA_LEN)
			remain = total;
		else
			remain = 2*LCD_DATA_LEN;
		
		for(i=0;i<remain;i++)
		{
			WriteData(data[i]);
		}
		
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
	Delay_ms(10); 

	if(rx_buffer[0] == 0x89 && rx_buffer[1] == 0xF0)
		return true;
	else
		return false;
}

//清屏函数
//color:要清屏的填充色
void LCD_Clear(uint16_t color)
{
	uint8_t i,page,data;

	if(color==BLACK)
		data = 0x00;
	else
		data = 0xff;
	
	for(page=0;page<PAGE_MAX;page++)
	{
		WriteComm(0xb0+page);
		WriteComm((32&0xf0)>>4|0x10);
		WriteComm(32&0x0f);
		
		for(i=0;i<COL;i++)
			WriteData(data);
	}
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

	WriteComm(0xae);
	
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
	
	WriteComm(0xaf);

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
	
	err = LCD_I2C_Init();
	if(err == 0)
		return;

	gpio_pin_configure(gpio_lcd, LCD_RST, GPIO_OUTPUT);
	gpio_pin_set(gpio_lcd, LCD_RST, 0);
	Delay_ms(100);
	gpio_pin_set(gpio_lcd, LCD_RST, 1);
	Delay_ms(100);

	WriteComm(0xAE); //Set Display Off

#if 1
	WriteComm(0x00); /*set lower column address*/
	WriteComm(0x12); /*set higher column address*/

	WriteComm(0x00); /*set display start line*/
	
	WriteComm(0xB0); /*set page address*/
	
	WriteComm(0x81); /*contract control*/
	WriteComm(0xff); /*128*/
	
	WriteComm(0xA0); /*set segment(left or right) remap A0:X[0]=0b: column address 0 is mapped to SEG0 (RESET),A1:X[0]=1b: column address 127 is mapped to SEG0*/
	
	WriteComm(0xA6); /*normal / reverse*/
	
	WriteComm(0xA8); /*multiplex ratio*/
	WriteComm(0x1F); /*duty = 1/32*/
	
	WriteComm(0xC0); /*Com scan direction(up or down) C0:normal mode (RESET) Scan from COM0 to COM[N –1], C8:remapped mode. Scan from COM[N-1] to COM0*/
	
	WriteComm(0xD3); /*set display offset*/
	WriteComm(0x00);
	
	WriteComm(0xD5); /*set osc division*/
	WriteComm(0x80);
	
	WriteComm(0xD9); /*set pre-charge period*/
	WriteComm(0x1f);
	
	WriteComm(0xDA); /*set COM pins*/
	WriteComm(0x12);
	
	WriteComm(0xdb); /*set vcomh*/
	WriteComm(0x40);
	
	WriteComm(0x8d); /*set charge pump enable*/
	WriteComm(0x14);
	
	WriteComm(0xAF); /*display ON*/
#else
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

	WriteComm(0xAF); //Set Display On
#endif

	LCD_Clear(WHITE);
	Delay_ms(1000);
	LCD_Clear(BLACK);
	Delay_ms(1000);

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

#endif/*LCD_VG6432TSWPG28_SSD1315*/
