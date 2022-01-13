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

#define TEMP_DEBUG

static struct device *i2c_temp;
static struct device *gpio_temp;

#ifdef CONFIG_CRC_SUPPORT
static CRC_8 crc_8_CUSTOM = {0x31,0xff,0x00,false,false};
#endif

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
	if(HardwareID != GXTS04_ID)
		return false;
	else
		return true;
}

bool GetTemperature(float *temp)
{
	u8_t crc=0;
	u8_t databuf[10] = {0};
	u16_t trans_temp = 0;
	float real_temp = 0.0;
	
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
	real_temp = 175.0*(float)trans_temp/65535.0-45.0;
	*temp = real_temp;
#ifdef TEMP_DEBUG
	LOGD("real temp:%d.%d", (s16_t)(real_temp*10)/10, (s16_t)(real_temp*10)%10);
#endif
	return true;
}
