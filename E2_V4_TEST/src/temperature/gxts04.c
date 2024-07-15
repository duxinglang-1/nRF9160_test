/****************************************Copyright (c)************************************************
** File Name:			    gxts04.h
** Descriptions:			gxts04 interface process source file
** Created By:				xie biao
** Created Date:			2021-12-24
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include "temp.h"
#include "gxts04.h"
#include "logger.h"
#ifdef CONFIG_CRC_SUPPORT
#include "crc_check.h"
#endif

static struct device *i2c_temp;
static struct device *gpio_temp;

#ifdef CONFIG_CRC_SUPPORT
static CRC_8 crc_8_CUSTOM = {0x31,0xff,0x00,false,false};
#endif

static uint32_t measure_count = 0;
static float t_sensor = 0.0;		//传感器温度值
static float t_body = 0.0; 			//显示的温度值
static float t_predict = 0.0;		//预测的人体温度值
static float t_temp80 = 0.0;		//预测的人体温度值

#ifdef GPIO_ACT_I2C
static void I2C_INIT(void)
{
	if(gpio_temp == NULL)
		gpio_temp = DEVICE_DT_GET(TEMP_PORT);

	gpio_pin_configure(gpio_temp, TEMP_SCL, GPIO_OUTPUT);
	gpio_pin_configure(gpio_temp, TEMP_SDA, GPIO_OUTPUT);
	gpio_pin_set(gpio_temp, TEMP_SCL, 1);
	gpio_pin_set(gpio_temp, TEMP_SDA, 1);
}

static void I2C_SDA_OUT(void)
{
	gpio_pin_configure(gpio_temp, TEMP_SDA, GPIO_OUTPUT);
}

static void I2C_SDA_IN(void)
{
	gpio_pin_configure(gpio_temp, TEMP_SDA, GPIO_INPUT);
}

static void I2C_SDA_H(void)
{
	gpio_pin_set(gpio_temp, TEMP_SDA, 1);
}

static void I2C_SDA_L(void)
{
	gpio_pin_set(gpio_temp, TEMP_SDA, 0);
}

static void I2C_SCL_H(void)
{
	gpio_pin_set(gpio_temp, TEMP_SCL, 1);
}

static void I2C_SCL_L(void)
{
	gpio_pin_set(gpio_temp, TEMP_SCL, 0);
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
		val = gpio_pin_get_raw(gpio_temp, TEMP_SDA);
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
		val = gpio_pin_get_raw(gpio_temp, TEMP_SDA);
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
#endif/*GPIO_ACT_I2C*/

static int32_t platform_write(struct device *handle, uint8_t *bufp, uint16_t len)
{
	int32_t rslt = 0;

#ifdef GPIO_ACT_I2C
	rslt = I2C_write_data(GXTS04_I2C_ADDR, bufp, len);
#else
	rslt = i2c_write(handle, bufp, len, GXTS04_I2C_ADDR);
#endif
	return rslt;
}

static int32_t platform_read(struct device *handle, uint8_t *bufp, uint16_t len)
{
	int32_t rslt = 0;

#ifdef GPIO_ACT_I2C
	rslt = I2C_read_data(GXTS04_I2C_ADDR, bufp, len);
#else
	rslt = i2c_read(handle, bufp, len, GXTS04_I2C_ADDR);
#endif
	return rslt;
}

static uint8_t init_i2c(void)
{
#ifdef GPIO_ACT_I2C
	I2C_INIT();
	return 0;
#else
	i2c_temp = DEVICE_DT_GET(TEMP_DEV);
	if(!i2c_temp)
	{
		return -1;
	} 
	else
	{
		i2c_configure(i2c_temp, I2C_SPEED_SET(I2C_SPEED_FAST));
		return 0;
	}
#endif	
}

static int32_t gxts04_read_data(uint16_t cmd, uint8_t* bufp, uint16_t len)
{
	uint8_t data[2] = {0};
	uint32_t rslt = 0;

	data[0] = (uint8_t)(0x00ff&(cmd>>8));
	data[1] = (uint8_t)(0x00ff&cmd);

	rslt = temp_dev_ctx.write(i2c_temp, data, 2);
	if(rslt == 0)
	{
		if((cmd == CMD_MEASURE_LOW_POWER) || (cmd == CMD_MEASURE_NORMAL))
		{
			k_sleep(K_MSEC(20));
		}
		
		rslt = temp_dev_ctx.read(i2c_temp, bufp, len);
	}

	return rslt;
}

static int32_t gxts04_write_data(uint16_t cmd)
{
	uint8_t data[2] = {0};
	uint32_t rslt = 0;

	data[0] = (uint8_t)(0x00ff&(cmd>>8));
	data[1] = (uint8_t)(0x00ff&cmd);

	rslt = temp_dev_ctx.write(i2c_temp, data, 2);
	return rslt;
}

static void temp_sensor_init(void)
{
	uint8_t databuf[2] = {0};
	uint16_t HardwareID = 0;

	gxts04_write_data(CMD_RESET);
	k_usleep(200);

	gxts04_write_data(CMD_WAKEUP);
	gxts04_read_data(CMD_READ_ID, &databuf, 2);
	gxts04_write_data(CMD_SLEEP);

	HardwareID = databuf[0]*0x100 + databuf[1];
#ifdef TEMP_DEBUG	
	LOGD("temp id:%x", HardwareID);
#endif
}

bool gxts04_init(void)
{
	if(init_i2c() != 0)
		return;

	temp_dev_ctx.write = platform_write;
	temp_dev_ctx.read  = platform_read;
#ifdef GPIO_ACT_I2C
	temp_dev_ctx.handle   = NULL;
#else
	temp_dev_ctx.handle   = i2c_temp;
#endif

	temp_sensor_init();
	//if(HardwareID != GXTS04_ID)
	//	return false;
	//else
		return true;
}

void gxts04_start(void)
{
	MAX20353_LDO1Config();
	temp_sensor_init();
	
	measure_count = 0;
	t_sensor = 0.0;
	t_body = 0.0;
	t_predict = 0.0;
	t_temp80 = 0.0;
}

void gxts04_stop(void)
{
	measure_count = 0;
}

bool GetTemperature(float *skin_temp, float *body_temp)
{
	bool flag=false;
	uint8_t crc=0;
	uint8_t databuf[10] = {0};
	uint16_t trans_temp = 0;

	if(!CheckSCC()
	#ifdef CONFIG_FACTORY_TEST_SUPPORT
		&& !IsFTTempTesting()
		&& !IsFTTempAging()
	#endif
		)
	{
		return false;
	}
	
	gxts04_write_data(CMD_WAKEUP);
	gxts04_read_data(CMD_MEASURE_LOW_POWER, &databuf, 10);
	gxts04_write_data(CMD_SLEEP);

#ifdef TEMP_DEBUG
	LOGD("temp:%02x,%02x,%02x", databuf[0],databuf[1],databuf[2]);
#endif

#ifdef CONFIG_CRC_SUPPORT
	crc = crc8_cal(databuf, 2, crc_8_CUSTOM);
  #ifdef TEMP_DEBUG
	LOGD("crc:%02x", crc);
  #endif
	if(crc != databuf[2])
		return false;
#endif

	trans_temp = databuf[0]*0x100 + databuf[1];
	t_sensor = 175.0*(float)trans_temp/65535.0-45.0;
	if(t_sensor > 99.9)
		t_sensor = 0.0;
	*skin_temp = t_sensor;
	*body_temp = 0;

#ifdef TEMP_DEBUG
	LOGD("count:%d, real temp:%d.%d", measure_count, (int16_t)(t_sensor*10)/10, (int16_t)(t_sensor*10)%10);
#endif

	if(t_sensor > 28)			//如果上一次测温大于32，那么开始计数
	{
		measure_count = measure_count+1;
	}
	else if(measure_count == 2000)
	{
		measure_count = 2000;
	}
	else
	{
		measure_count = 0;
	}

	if(measure_count == 0)
	{
		t_body = t_sensor; 
		t_predict = 0;
	}
	else if((measure_count > 0)&&(measure_count <20))
	{
		t_body = t_sensor;
	}
	else if(measure_count == 20)
	{
		t_body = t_sensor;
		t_temp80 = t_sensor;
		if((t_sensor > 36)&&(t_sensor <= 41))
			t_predict = 36.9 + (t_sensor-36)*4.1/5;
		else if((t_sensor > 28)&&(t_sensor <= 36))
			t_predict = 36.1 + (t_sensor-28)*0.8/8; 
		else
			t_predict = t_sensor;
	}
	else if((measure_count > 20)&&(measure_count <= 25))
	{
		t_body = ((measure_count-20)*0.8*(t_predict-t_temp80))/5 + t_temp80;
	}

	else if((measure_count > 25)&&(measure_count <= 30))
	{
		t_body = t_predict - ((30-measure_count)*0.2*(t_predict-t_temp80))/5;
	}
	else
	{
		if((t_sensor > 36)&&(t_sensor <= 41))
			t_body = 36.9 + (t_sensor-36)*4.1/5;
		else if((t_sensor > 28)&&(t_sensor <= 36))
			t_body = 36.1 + (t_sensor-28)*0.8/8;
		else
			t_body = t_sensor;

	#ifdef CONFIG_FACTORY_TEST_SUPPORT
		if(!IsFTTempAging())
	#endif
			flag = true;
	}

	*body_temp = t_body;

#ifdef TEMP_DEBUG
	LOGD("flag:%d, t_temp80:%d.%d, t_body:%d.%d", flag, (int16_t)(t_temp80*10)/10, (int16_t)(t_temp80*10)%10, (int16_t)(t_body*10)/10, (int16_t)(t_body*10)%10);
#endif

	return flag;
}
