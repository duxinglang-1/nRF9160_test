/****************************************Copyright (c)************************************************
** File Name:			    ppg_interface_config.c
** Descriptions:			PPG interface config source file
** Created By:				xie biao
** Created Date:			2024-06-14
** Modified Date:      		2024-06-14 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include "inner_flash.h"
#include "external_flash.h"
#include "ppg.h"

ppgdev_ctx_t ppg_dev_ctx = {0};

static struct device *i2c_ppg = NULL;
static struct device *gpio_ppg = NULL;
static struct gpio_callback gpio_cb;

extern void ppg_get_data_timerout(struct k_timer *timer_id);
extern void ppg_set_appmode_timerout(struct k_timer *timer_id);
extern void ppg_auto_stop_timerout(struct k_timer *timer_id);
extern void ppg_skin_check_timerout(struct k_timer *timer_id);
extern void ppg_menu_stop_timerout(struct k_timer *timer_id);
extern void ppg_delay_start_timerout(struct k_timer *timer_id);
extern void ppg_bpt_est_start_timerout(struct k_timer *timer_id);

const ppg_timer_t ppg_timer[7] = 
{
	{PPG_TIMER_APPMODE, 		NULL,	ppg_set_appmode_timerout, 	NULL},
	{PPG_TIMER_AUTO_STOP, 		NULL,	ppg_auto_stop_timerout, 	NULL},
	{PPG_TIMER_MENU_STOP, 		NULL,	ppg_menu_stop_timerout, 	NULL},
	{PPG_TIMER_GET_HR, 			NULL,	ppg_get_data_timerout, 		NULL},
	{PPG_TIMER_DELAY_START, 	NULL,	ppg_delay_start_timerout, 	NULL},
	{PPG_TIMER_BPT_EST_START, 	NULL,	ppg_bpt_est_start_timerout, NULL},
	{PPG_TIMER_SKIN_CHECK, 		NULL,	ppg_skin_check_timerout, 	NULL},
};

extern bool ppg_int_event;

void PPG_Delay_ms(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

void PPG_Delay_us(unsigned int dly)
{
	k_usleep(dly);
}

#ifdef GPIO_ACT_I2C
void I2C_INIT(void)
{
	if(gpio_ppg == NULL)
		gpio_ppg = DEVICE_DT_GET(PPG_PORT);

	gpio_pin_configure(gpio_ppg, PPG_SCL_PIN, GPIO_OUTPUT);
	gpio_pin_configure(gpio_ppg, PPG_SDA_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ppg, PPG_SCL_PIN, 1);
	gpio_pin_set(gpio_ppg, PPG_SDA_PIN, 1);
}

void I2C_SDA_OUT(void)
{
	gpio_pin_configure(gpio_ppg, PPG_SDA_PIN, GPIO_OUTPUT);
}

void I2C_SDA_IN(void)
{
	gpio_pin_configure(gpio_ppg, PPG_SDA_PIN, GPIO_INPUT);
}

void I2C_SDA_H(void)
{
	gpio_pin_set(gpio_ppg, PPG_SDA_PIN, 1);
}

void I2C_SDA_L(void)
{
	gpio_pin_set(gpio_ppg, PPG_SDA_PIN, 0);
}

void I2C_SCL_H(void)
{
	gpio_pin_set(gpio_ppg, PPG_SCL_PIN, 1);
}

void I2C_SCL_L(void)
{
	gpio_pin_set(gpio_ppg, PPG_SCL_PIN, 0);
}

//产生起始信号
void I2C_Start(void)
{
	I2C_SDA_OUT();

	I2C_SDA_H();
	I2C_SCL_H();
	I2C_SDA_L();
	I2C_SCL_L();
}

//产生停止信号
void I2C_Stop(void)
{
	I2C_SDA_OUT();

	I2C_SCL_L();
	I2C_SDA_L();
	I2C_SCL_H();
	I2C_SDA_H();
}

//主机产生应答信号ACK
void I2C_Ack(void)
{
	I2C_SDA_OUT();
	
	I2C_SDA_L();
	I2C_SCL_L();
	I2C_SCL_H();
	I2C_SCL_L();
}

//主机不产生应答信号NACK
void I2C_NAck(void)
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
uint8_t I2C_Wait_Ack(void)
{
	uint8_t val,tempTime=0;

	I2C_SDA_IN();
	I2C_SCL_H();

	while(1)
	{
		val = gpio_pin_get_raw(gpio_ppg, PPG_SDA_PIN);
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
uint8_t I2C_Write_Byte(uint8_t txd)
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
void I2C_Read_Byte(bool ack, uint8_t *data)
{
	uint8_t i=0,receive=0,val=0;

	I2C_SDA_IN();
	I2C_SCL_L();

	for(i=0;i<8;i++)
	{
		I2C_SCL_H();
		receive<<=1;
		val = gpio_pin_get_raw(gpio_ppg, PPG_SDA_PIN);
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

uint8_t I2C_write_data(uint8_t addr, uint8_t *databuf, uint32_t len)
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

uint8_t I2C_read_data(uint8_t addr, uint8_t *databuf, uint32_t len)
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

static bool ppg_init_i2c(void)
{
#ifdef GPIO_ACT_I2C
	I2C_INIT();
	return true;
#else
	i2c_ppg = DEVICE_DT_GET(PPG_DEV);
	if(!i2c_ppg)
	{
#ifdef PMU_DEBUG
		LOGD("ERROR SETTING UP I2C");
#endif
		return false;
	} 
	else
	{
		i2c_configure(i2c_ppg, I2C_SPEED_SET(I2C_SPEED_FAST));
		return true;
	}
#endif	
}

static void interrupt_event(struct device *interrupt, struct gpio_callback *cb, uint32_t pins)
{
	ppg_int_event = true; 
}

void PPG_INT_HIGH(void)
{
	gpio_pin_set(gpio_ppg, PPG_INT_PIN, 1);
}

void PPG_INT_LOW(void)
{
	gpio_pin_set(gpio_ppg, PPG_INT_PIN, 0);
}

void PPG_EN_HIGH(void)
{
	gpio_pin_set(gpio_ppg, PPG_EN_PIN, 1);
}

void PPG_EN_LOW(void)
{
	gpio_pin_set(gpio_ppg, PPG_EN_PIN, 0);
}

void PPG_I2C_EN_HIGH(void)
{
	if(gpio_ppg == NULL)
		gpio_ppg = DEVICE_DT_GET(PPG_PORT);

	gpio_pin_configure(gpio_ppg, PPG_I2C_EN_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ppg, PPG_I2C_EN_PIN, 1);
}

void PPG_I2C_EN_LOW(void)
{
	if(gpio_ppg == NULL)
		gpio_ppg = DEVICE_DT_GET(PPG_PORT);

	gpio_pin_configure(gpio_ppg, PPG_I2C_EN_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ppg, PPG_I2C_EN_PIN, 0);
}

void PPG_RST_HIGH(void)
{
	gpio_pin_set(gpio_ppg, PPG_RST_PIN, 1);
}

void PPG_RST_LOW(void)
{
	gpio_pin_set(gpio_ppg, PPG_RST_PIN, 0);
}

void PPG_MFIO_HIGH(void)
{
	gpio_pin_set(gpio_ppg, PPG_MFIO_PIN, 1);
}

void PPG_MFIO_LOW(void)
{
	gpio_pin_set(gpio_ppg, PPG_MFIO_PIN, 0);
}

static void ppg_init_gpio(void)
{
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_UP;

	if(gpio_ppg == NULL)
		gpio_ppg = DEVICE_DT_GET(PPG_PORT);

#if 0	//xb add 20230228 Set the PPG interrupt pin as input to prevent leakage.
	//interrupt
	gpio_pin_configure(gpio_ppg, PPG_INT_PIN, flag);
	gpio_pin_interrupt_configure(gpio_ppg, PPG_INT_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb, interrupt_event, BIT(PPG_INT_PIN));
	gpio_add_callback(gpio_ppg, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_ppg, PPG_INT_PIN, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);
#else
	gpio_pin_configure(gpio_ppg, PPG_INT_PIN, GPIO_INPUT);
#endif	
	gpio_pin_configure(gpio_ppg, PPG_EN_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ppg, PPG_EN_PIN, 1);
	k_sleep(K_MSEC(20));

	gpio_pin_configure(gpio_ppg, PPG_I2C_EN_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ppg, PPG_I2C_EN_PIN, 1);

	gpio_pin_configure(gpio_ppg, 29, GPIO_OUTPUT);
	gpio_pin_set(gpio_ppg, 29, 1);
	
	//PPG模式选择(bootload\application)
	gpio_pin_configure(gpio_ppg, PPG_RST_PIN, GPIO_OUTPUT);
	gpio_pin_configure(gpio_ppg, PPG_MFIO_PIN, GPIO_OUTPUT);
}

static int32_t platform_write(struct device *handle, uint8_t *tx_buf, uint32_t tx_len)
{
	uint32_t rslt = 0;

#ifdef GPIO_ACT_I2C
	rslt = I2C_write_data(MAX32674_I2C_ADD, tx_buf, tx_len);
#else
	rslt = i2c_write(handle, tx_buf, tx_len, MAX32674_I2C_ADD);
#endif
	return rslt;
}

static int32_t platform_read(struct device *handle, uint8_t *rx_buf, uint32_t rx_len)
{
	uint32_t rslt = 0;

#ifdef GPIO_ACT_I2C
	rslt = I2C_read_data(MAX32674_I2C_ADD, rx_buf, rx_len);
#else
	rslt = i2c_read(handle, rx_buf, rx_len, MAX32674_I2C_ADD);
#endif
	return rslt;
}

static void ppg_init_timer(void)
{
	uint8_t i;

	for(i=0;i<(sizeof(ppg_timer)/sizeof(ppg_timer[0]));i++)
	{
		k_timer_init(&ppg_timer[i].timer_id, ppg_timer[i].expiry_fn, ppg_timer[i].stop_fn);
	}
}

void ppg_interface_init(void)
{
	ppg_init_gpio();
	ppg_init_i2c();
	ppg_init_timer();
	
	ppg_dev_ctx.write = platform_write;
	ppg_dev_ctx.read  = platform_read;
#ifdef GPIO_ACT_I2C
	ppg_dev_ctx.handle	  = NULL;
#else
	ppg_dev_ctx.handle	  = i2c_ppg;
#endif
}

void ppg_save_7_days_data(uint8_t *data, PPG_REC2_DATA_TYPE type)
{
	switch(type)
	{
	case PPG_REC2_HR:
		SpiFlash_Write(data, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
		break;

	case PPG_REC2_SPO2:
		SpiFlash_Write(data, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
		break;
		
	case PPG_REC2_BPT:
		SpiFlash_Write(data, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
		break;
	}
}

void ppg_read_7_days_data(uint8_t *data, PPG_REC2_DATA_TYPE type)
{
	switch(type)
	{
	case PPG_REC2_HR:
		SpiFlash_Read(data, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
		break;

	case PPG_REC2_SPO2:
		SpiFlash_Read(data, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
		break;
		
	case PPG_REC2_BPT:
		SpiFlash_Read(data, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
		break;
	}

}

void ppg_save_last_data(health_record_t *data)
{
	save_cur_health_to_record(data);
}

void ppg_read_last_data(health_record_t *data)
{
	get_cur_health_from_record(data);
}

void ppg_start_timer(PPG_TIMER_NAME name, uint32_t duration_ms, uint32_t period_ms)
{
	k_timer_start(&ppg_timer[name].timer_id, K_MSEC(duration_ms), K_MSEC(period_ms));
}

void ppg_stop_timer(PPG_TIMER_NAME name)
{
	k_timer_stop(&ppg_timer[name].timer_id);
}
