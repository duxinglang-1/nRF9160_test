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
#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include "gxts04.h"
#include "logger.h"
#ifdef CONFIG_CRC_SUPPORT
#include "crc_check.h"
#endif

//#define TEMP_DEBUG

static struct device *i2c_temp;
static struct device *gpio_temp;

#ifdef CONFIG_CRC_SUPPORT
static CRC_8 crc_8_CUSTOM = {0x31,0xff,0x00,false,false};
#endif

static u32_t measure_count = 0;
static float t_sensor = 0.0;		//�������¶�ֵ
static float t_body = 0.0; 			//��ʾ���¶�ֵ
static float t_predict = 0.0;		//Ԥ��������¶�ֵ
static float t_temp80 = 0.0;		//Ԥ��������¶�ֵ


static u8_t init_i2c(void)
{
	i2c_temp = device_get_binding(TEMP_DEV);
	if(!i2c_temp)
	{
		return -1;
	} 
	else
	{
		i2c_configure(i2c_temp, I2C_SPEED_SET(I2C_SPEED_FAST));
		return 0;
	}
}

static s32_t gxts04_read_data(u16_t cmd, u8_t* bufp, u16_t len)
{
	u8_t data[2] = {0};
	u32_t rslt = 0;

	data[0] = (u8_t)(0x00ff&(cmd>>8));
	data[1] = (u8_t)(0x00ff&cmd);

	rslt = i2c_write(i2c_temp, data, 2, GXTS04_I2C_ADDR);
	if(rslt == 0)
	{
		if((cmd == CMD_MEASURE_LOW_POWER) || (cmd == CMD_MEASURE_NORMAL))
		{
			k_sleep(K_MSEC(20));
		}
		
		rslt = i2c_read(i2c_temp, bufp, len, GXTS04_I2C_ADDR);
	}

	return rslt;
}

static s32_t gxts04_write_data(u16_t cmd)
{
	u8_t data[2] = {0};
	u32_t rslt = 0;

	data[0] = (u8_t)(0x00ff&(cmd>>8));
	data[1] = (u8_t)(0x00ff&cmd);

	rslt = i2c_write(i2c_temp, data, 2, GXTS04_I2C_ADDR);
	return rslt;
}

bool gxts04_init(void)
{
	u8_t databuf[2] = {0};
	u16_t HardwareID = 0;

	if(init_i2c() != 0)
		return;
	
	gxts04_write_data(CMD_WAKEUP);
	gxts04_read_data(CMD_READ_ID, &databuf, 2);
	gxts04_write_data(CMD_SLEEP);

	HardwareID = databuf[0]*0x100 + databuf[1];
#ifdef TEMP_DEBUG	
	LOGD("temp id:%x", HardwareID);
#endif
	//if(HardwareID != GXTS04_ID)
	//	return false;
	//else
		return true;
}

void gxts04_start(void)
{
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
	u8_t crc=0;
	u8_t databuf[10] = {0};
	u16_t trans_temp = 0;
	
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
	*skin_temp = t_sensor;
	*body_temp = 0;

#ifdef TEMP_DEBUG
	LOGD("count:%d, real temp:%d.%d", measure_count, (s16_t)(t_sensor*10)/10, (s16_t)(t_sensor*10)%10);
#endif

	if(t_sensor > 32)			//�����һ�β��´���32����ô��ʼ����
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
		else if((t_sensor > 32)&&(t_sensor <= 36))
			t_predict = 36.1 + (t_sensor-32)*0.8/4; 
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
		else if((t_sensor > 32)&&(t_sensor <= 36))
			t_body = 36.1 + (t_sensor-32)*0.8/4;
		else
			t_body = t_sensor;
	}

	*body_temp = t_body;

#ifdef TEMP_DEBUG
	LOGD("count:%d, t_predict:%d.%d, t_temp80:%d.%d, t_body:%d.%d", measure_count, (s16_t)(t_predict*10)/10, (s16_t)(t_predict*10)%10, (s16_t)(t_temp80*10)/10, (s16_t)(t_temp80*10)%10, (s16_t)(t_body*10)/10, (s16_t)(t_body*10)%10);
#endif

	return true;
}