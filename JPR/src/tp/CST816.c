/**************************************************************************
 *Name        : CST816.c
 *Author      : xie biao
 *Version     : V1.0
 *Create      : 2020-08-21
 *Copyright   : August
**************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <dk_buttons_and_leds.h>
#include "CST816.h"
#include "CST816_update.h"

bool tp_trige_flag = false;

struct device *i2c_ctp;
struct device *gpio_ctp;

static struct gpio_callback gpio_cb;

static u8_t init_i2c(void)
{
	i2c_ctp = device_get_binding(CTP_DEV);
	if(!i2c_ctp)
	{
		printf("ERROR SETTING UP I2C\r\n");
		return -1;
	} 
	else
	{
		i2c_configure(i2c_ctp, I2C_SPEED_SET(I2C_SPEED_FAST));
		printf("I2C CONFIGURED\r\n");
		return 0;
	}
}

static s32_t platform_write(u8_t reg, u8_t *bufp, u16_t len)
{
	u8_t data[len+1];
	u32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(i2c_ctp, data, len+1, CST816_I2C_ADDRESS);
	printf("rslt:%d\n", rslt);

	return rslt;
}

static s32_t platform_read(u8_t reg, u8_t *bufp, u16_t len)
{
	u32_t rslt = 0;

	rslt = i2c_write(i2c_ctp, &reg, 1, CST816_I2C_ADDRESS);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ctp, bufp, len, CST816_I2C_ADDRESS);
	}
	printf("rslt:%d\n", rslt);
	return rslt;
}

static s32_t platform_write_word(u16_t reg, u8_t *bufp, u16_t len)
{
	u8_t data[len+2];
	u32_t rslt = 0;

#if 1
	data[0] = reg&0xFF;
	data[1] = reg>>8;
#else
	data[0] = reg>>8;
	data[1] = reg&0xFF;
#endif

	memcpy(&data[2], bufp, len);
	rslt = i2c_write(i2c_ctp, data, len+2, CST816_I2C_ADDRESS);
	printf("rslt:%d\n", rslt);

	return rslt;
}

static s32_t platform_read_word(u16_t reg, u8_t *bufp, u16_t len)
{
	u8_t data[2];
	u32_t rslt = 0;

#if 1
	data[0] = reg&0xFF;
	data[1] = reg>>8;
#else
	data[0] = reg>>8;
	data[1] = reg&0xFF;
#endif

	rslt = i2c_write(i2c_ctp, data, 2, CST816_I2C_ADDRESS);
	printf("rslt:%d\n", rslt);

	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ctp, bufp, len, CST816_I2C_ADDRESS);
	}

	printf("rslt:%d\n", rslt);
	return rslt;
}

static int cst816s_enter_bootmode(void)
{
	u8_t retryCnt = 50;
	u8_t cmd[3] = {0};

	gpio_pin_write(gpio_ctp, CTP_RESET, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_write(gpio_ctp, CTP_RESET, 1);
	k_sleep(K_MSEC(10));

	while(retryCnt--)
	{
		cmd[0] = 0xAB;
		platform_write_word(0xA001,cmd,1);
		k_sleep(K_MSEC(10));
		platform_read_word(0xA003,cmd,1);
		
		if(cmd[0] != 0x55)
		{
			k_sleep(K_MSEC(10));
			continue;
		}
		else
		{
			return 0;
		}
		k_sleep(K_MSEC(10));

	}
	return -1;
}

#define PER_LEN	512

static int cst816s_update(u16_t startAddr, u16_t len, const unsigned char *src)
{
	u8_t cmd[10];
	u8_t data_send[PER_LEN];
	u32_t sum_len;
	u32_t i,k_data=0,b_data=0;

	sum_len = 0;
	k_data=len/PER_LEN;
	for(i=0;i<k_data;i++)
	{
		cmd[0] = startAddr&0xFF;
		cmd[1] = startAddr>>8;
		platform_write_word(0xA014,cmd,2);
		//TP_HRS_WriteBytes_updata(0x6A<<1,0xA014,cmd,2,REG_LEN_2B);
		memcpy(data_send,src,PER_LEN);
		platform_write_word(0xA018,data_send,PER_LEN);
		//TP_HRS_WriteBytes_updata(0x6A<<1,0xA018,data_send,PER_LEN,REG_LEN_2B);
		k_sleep(K_MSEC(10));
		//nrf_delay_ms(10);
		cmd[0] = 0xEE;
		platform_write_word(0xA004,cmd,1);
		//TP_HRS_WriteBytes_updata(0x6A<<1,0xA004,cmd,1,REG_LEN_2B);
		k_sleep(K_MSEC(50));
		//nrf_delay_ms(50);

		{
			uint8_t retrycnt = 50;
			while(retrycnt--)
			{
				cmd[0] = 0;
				platform_read_word(0xA005,cmd,1);
				//TP_HRS_read_updata(0x6A<<1,0xA005,cmd,1,REG_LEN_2B);
				if(cmd[0] == 0x55)
				{
					cmd[0] = 0;
					break;
				}
				k_sleep(K_MSEC(10));
				//nrf_delay_ms(10);
			}
		}
		startAddr += PER_LEN;
		src += PER_LEN;
		sum_len += PER_LEN;

	}

	// exit program mode
	cmd[0] = 0x00;
	platform_write_word(0xA003,cmd,1);
	//TP_HRS_WriteBytes_updata(0x6A<<1,0xA003,cmd,1,REG_LEN_2B);
	return 0;
}

static u32_t cst816s_read_checksum(void)
{
	union
	{
		u32_t sum;
		u8_t buf[4];
	}checksum;
	
	u8_t cmd[3];
	char readback[4] = {0};

	if(cst816s_enter_bootmode() == -1)
	{
		return -1;
	}

	cmd[0] = 0;
	platform_write_word(0xA003,cmd,1);
	//TP_HRS_WriteBytes_updata(0x6A<<1,0xA003,cmd,1,REG_LEN_2B);
	k_sleep(K_MSEC(500));
	//nrf_delay_ms(500);

	checksum.sum = 0;
	platform_read_word(0xA008,checksum.buf,2);
	//TP_HRS_read_updata(0x6A<<1,0xA008,checksum.buf,2,REG_LEN_2B);
	//LOG(LEVEL_INFO,"checksum.sum=%x \r\n",checksum.sum);

	return checksum.sum;
}


bool ctp_hynitron_update(void)
{
	u8_t lvalue;
	u8_t write_data[2];
	u8_t tmpbuf[128]={0};
	bool temp_result = true;

	if(cst816s_enter_bootmode() == 0)
	{
		LCD_ShowString(20,120,"enter bootmode!");
		
		if(sizeof(app_bin)>10)
		{
			u16_t startAddr = app_bin[1];
			u16_t length = app_bin[3];					
			u16_t checksum = app_bin[5];
			
			startAddr <<= 8; startAddr |= app_bin[0];
			length <<= 8; length |= app_bin[2];
			checksum <<= 8; checksum |= app_bin[4];  

			LCD_ShowString(20,140,"check if need update!");
			
			if(cst816s_read_checksum()!= checksum)
			{
				LCD_ShowString(20,160,"start update!");
				
				cst816s_update(startAddr, length, &app_bin[6]);
				cst816s_read_checksum();

				LCD_ShowString(20,180,"complete update!");
			}
		}
		return true;
	}
	return false;
}

void touch_panel_event_handle(tp_event tp_type, u16_t x_pos, u16_t y_pos)
{
	switch(tp_type)
	{
	case TP_EVENT_MOVING_UP:
		printk("tp moving up!\n");
		break;
	case TP_EVENT_MOVING_DOWN:
		printk("tp moving down!\n");
		break;
	case TP_EVENT_MOVING_LEFT:
		printk("tp moving left!\n");
		break;
	case TP_EVENT_MOVING_RIGHT:
		printk("tp moving right!\n");
		break;
	case TP_EVENT_SINGLE_CLICK:
		printk("tp single click! x:%d, y:%d\n", x_pos,y_pos);
		break;
	case TP_EVENT_DOUBLE_CLICK:
		printk("tp double click! x:%d, y:%d\n", x_pos,y_pos);
		break;
	case TP_EVENT_LONG_PRESS:
		printk("tp long press! x:%d, y:%d\n", x_pos,y_pos);
		break;
	case TP_EVENT_MAX:
		break;
	}
}

void CaptouchInterruptHandle(void)
{
	tp_trige_flag = true;
}

void tp_interrupt_proc(void)
{
	u8_t tmpbuf[128] = {0};
	u8_t TP_type = TP_EVENT_MAX;
	u8_t tp_temp[10]={0};

	platform_read(CST816_REG_GESTURE, &tp_temp[0], 1);//手势
	platform_read(CST816_REG_FINGER_NUM, &tp_temp[1], 1);//手指个数
	platform_read(CST816_REG_XPOS_L, &tp_temp[2], 1);//x坐标低位
	platform_read(CST816_REG_YPOS_L, &tp_temp[3], 1);//y坐标低位

	printk("tp_temp=%x,%x,%x,%x\n",tp_temp[0],tp_temp[1],tp_temp[2],tp_temp[3]);
	switch(tp_temp[0])
	{
	case GESTURE_NONE:
		sprintf(tmpbuf, "GESTURE_NONE        ");
		break;
	case GESTURE_MOVING_UP:
		TP_type = TP_EVENT_MOVING_UP;
		sprintf(tmpbuf, "MOVING_UP   ");
		break;
	case GESTURE_MOVING_DOWN:
		TP_type = TP_EVENT_MOVING_DOWN;
		sprintf(tmpbuf, "MOVING_DOWN ");
		break;
	case GESTURE_MOVING_LEFT:
		TP_type = TP_EVENT_MOVING_LEFT;
		sprintf(tmpbuf, "MOVING_LEFT ");
		break;
	case GESTURE_MOVING_RIGHT:
		TP_type = TP_EVENT_MOVING_RIGHT;
		sprintf(tmpbuf, "MOVING_RIGHT");
		break;
	case GESTURE_SINGLE_CLICK:
		TP_type = TP_EVENT_SINGLE_CLICK;
		sprintf(tmpbuf, "SINGLE_CLICK");
		break;
	case GESTURE_DOUBLE_CLICK:
		TP_type = TP_EVENT_DOUBLE_CLICK;
		sprintf(tmpbuf, "DOUBLE_CLICK");
		break;
	case GESTURE_LONG_PRESS:
		TP_type = TP_EVENT_LONG_PRESS;
		sprintf(tmpbuf, "LONG_PRESS  ");
		break;
	}

	LCD_ShowString(20,120,tmpbuf);
	
	sprintf(tmpbuf, "x:%03d, y:%03d", tp_temp[2], tp_temp[3]);
	LCD_ShowString(20,140,tmpbuf);
	
	if(TP_type != TP_EVENT_MAX)
	{
		touch_panel_event_handle(TP_type, tp_temp[2], tp_temp[3]);
	}
}

void CST816_init(void)
{
	u8_t tmpbuf[128] = {0};
	u8_t tp_temp_id=0;
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_LOW|GPIO_INT_DEBOUNCE;
	
  	//端口初始化
  	gpio_ctp = device_get_binding(CTP_PORT);
	if(!gpio_ctp)
	{
		printk("Cannot bind gpio device\n");
		return;
	}

	init_i2c();

	gpio_pin_configure(gpio_ctp, CTP_EINT, flag);
	gpio_pin_disable_callback(gpio_ctp, CTP_EINT);
	gpio_init_callback(&gpio_cb, CaptouchInterruptHandle, BIT(CTP_EINT));
	gpio_add_callback(gpio_ctp, &gpio_cb);
	gpio_pin_enable_callback(gpio_ctp, CTP_EINT);

	gpio_pin_configure(gpio_ctp, CTP_RESET, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ctp, CTP_RESET, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_write(gpio_ctp, CTP_RESET, 1);
	k_sleep(K_MSEC(50));

	platform_read(CST816_REG_CHIPID, &tp_temp_id, 1);
	if(tp_temp_id == CST816_CHIP_ID)
	{
		printk("It's CST816!\n");
	}

	sprintf(tmpbuf, "CTP ID:%x", tp_temp_id);
	LCD_ShowString(20,100,tmpbuf);

	//ctp_hynitron_update();
}


void test_tp(void)
{
	u8_t tmpbuf[128] = {0};

	sprintf(tmpbuf, "test_tp");
	LCD_ShowString(20,80,tmpbuf);
	CST816_init();
}
