/****************************************Copyright (c)************************************************
** File Name:			    dps368.h
** Descriptions:			dps368 interface process source file
** Created By:				xie biao
** Created Date:			2024-06-18
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include "pressure.h"
#include "dps368.h"
#include "logger.h"
#ifdef CONFIG_CRC_SUPPORT
#include "crc_check.h"
#endif

static struct device *i2c_pressure;
static struct device *gpio_pressure;
static struct gpio_callback gpio_cb;

static bool pressure_trige_flag = false;

pressure_ctx_t pressure_dev_ctx;

#ifdef GPIO_ACT_I2C
static void I2C_INIT(void)
{
	if(gpio_pressure == NULL)
		gpio_pressure = DEVICE_DT_GET(PRESSURE_PORT);

	gpio_pin_configure(gpio_pressure, PRESSURE_SCL, GPIO_OUTPUT);
	gpio_pin_configure(gpio_pressure, PRESSURE_SDA, GPIO_OUTPUT);
	gpio_pin_set(gpio_pressure, PRESSURE_SCL, 1);
	gpio_pin_set(gpio_pressure, PRESSURE_SDA, 1);
}

static void I2C_SDA_OUT(void)
{
	gpio_pin_configure(gpio_pressure, PRESSURE_SDA, GPIO_OUTPUT);
}

static void I2C_SDA_IN(void)
{
	gpio_pin_configure(gpio_pressure, PRESSURE_SDA, GPIO_INPUT);
}

static void I2C_SDA_H(void)
{
	gpio_pin_set(gpio_pressure, PRESSURE_SDA, 1);
}

static void I2C_SDA_L(void)
{
	gpio_pin_set(gpio_pressure, PRESSURE_SDA, 0);
}

static void I2C_SCL_H(void)
{
	gpio_pin_set(gpio_pressure, PRESSURE_SCL, 1);
}

static void I2C_SCL_L(void)
{
	gpio_pin_set(gpio_pressure, PRESSURE_SCL, 0);
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
		val = gpio_pin_get_raw(gpio_pressure, PRESSURE_SDA);
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
		val = gpio_pin_get_raw(gpio_pressure, PRESSURE_SDA);
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

static int32_t platform_write(struct device *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	uint8_t data[len+1];
	uint32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
#ifdef GPIO_ACT_I2C
	rslt = I2C_write_data(DPS368_I2C_ADDRESS, data, len+1);
#else	
	rslt = i2c_write(handle, data, len+1, DPS368_I2C_ADDRESS);
#endif
	return rslt;
}

static int32_t platform_read(struct device *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	uint32_t rslt = 0;

#ifdef GPIO_ACT_I2C
	rslt = I2C_write_data(DPS368_I2C_ADDRESS, &reg, 1);
	if(rslt == 0)
	{
		rslt = I2C_read_data(DPS368_I2C_ADDRESS, bufp, len);
	}
#else
	rslt = i2c_write(handle, &reg, 1, DPS368_I2C_ADDRESS);
	if(rslt == 0)
	{
		rslt = i2c_read(handle, bufp, len, DPS368_I2C_ADDRESS);
	}
#endif	
	return rslt;
}

void PressureInterruptHandle(void)
{
	pressure_trige_flag = true;
}

void DPS368_GetChipID(uint8_t *data)
{
	pressure_dev_ctx.read(pressure_dev_ctx.handle, REG_ID, data, 1);
}

static void init_gpio(void)
{
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_UP;
	
	gpio_pressure = DEVICE_DT_GET(PRESSURE_PORT);
	if(!gpio_pressure)
		return;

	gpio_pin_configure(gpio_pressure, PRESSURE_POW_EN, GPIO_OUTPUT);
	gpio_pin_set(gpio_pressure, PRESSURE_POW_EN, 1);
	k_sleep(K_MSEC(20));
	
	//interrupt
	gpio_pin_configure(gpio_pressure, PRESSURE_EINT, flag);
	gpio_pin_interrupt_configure(gpio_pressure, PRESSURE_EINT, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb, PressureInterruptHandle, BIT(PRESSURE_EINT));
	gpio_add_callback(gpio_pressure, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_pressure, PRESSURE_EINT, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);
}

static void init_i2c(void)
{
#ifdef GPIO_ACT_I2C
	I2C_INIT();
#else
	i2c_pressure = DEVICE_DT_GET(PRESSURE_DEV);
	if(!i2c_pressure)
	{
		return;
	}
	else
	{
		i2c_configure(i2c_pressure, I2C_SPEED_SET(I2C_SPEED_FAST));
	}
#endif
}

bool DPS368_Init(void)
{
	uint8_t chip_id;
	
#ifdef PRESSURE_DEBUG
	LOGD("begin");
#endif
	init_gpio();
	init_i2c();

	pressure_dev_ctx.write = platform_write;
	pressure_dev_ctx.read  = platform_read;
#ifdef GPIO_ACT_I2C
	pressure_dev_ctx.handle   = NULL;
#else
	pressure_dev_ctx.handle   = i2c_pressure;
#endif

	DPS368_GetChipID(&chip_id);
#ifdef PRESSURE_DEBUG
	LOGD("chip id:0x%02X", chip_id);
#endif
	if(chip_id == DPS368_CHIP_ID)
		return true;
	else
		return false;
}

void DPS368_Start(void)
{

}

void DPS368_Stop(void)
{
}

void DPS368MsgProcess(void)
{
	if(pressure_trige_flag)
	{
		pressure_trige_flag = false;
	}
}

bool GetPressure(float *skin_temp, float *body_temp)
{
	bool flag=false;
	uint8_t crc=0;
	uint8_t databuf[10] = {0};
	uint16_t trans_temp = 0;

	return flag;
}
