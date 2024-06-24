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

#define ARRAYNUM(a) (sizeof(a)/sizeof(a[0]))

static struct device *i2c_pressure;
static struct device *gpio_pressure;
static struct gpio_callback gpio_cb;

static bool pressure_start_flag = false;
static bool pressure_stop_flag = false;
static bool pressure_interrupt_flag = false;

static int32_t m_c0,m_c1,m_c00,m_c01,m_c10,m_c11,m_c20,m_c21,m_c30;

pressure_ctx_t pressure_dev_ctx;

uint8_t DPS368_coefs[18] = 
{
	0x00,		//c0 		0x10 c0 [11:4]
	0x00,		//c0/c1	 	0x11 c0 [3:0] 	c1 	[11:8]
	0x00,		//c1 		0x12 c1	[7:0]
	0x00,		//c00 		0x13 c00 [19:12]
	0x00,		//c00 		0x14 c00 [11:4]
	0x00,		//c00/c10 	0x15 c00 [3:0] 	c10 [19:16]
	0x00,		//c10 		0x16 c10 [15:8]
	0x00,		//c10 		0x17 c10 [7:0]
	0x00,		//c01 		0x18 c01 [15:8]
	0x00,		//c01 		0x19 c01 [7:0]
	0x00,		//c11 		0x1A c11 [15:8]
	0x00,		//c11 		0x1B c11 [7:0]
	0x00,		//c20 		0x1C c20 [15:8]
	0x00,		//c20 		0x1D c20 [7:0]
	0x00,		//c21 		0x1E c21 [15:8]
	0x00,		//c21 		0x1F c21 [7:0]
	0x00,		//c30 		0x20 c30 [15:8]
	0x00,		//c30 		0x21 c30 [7:0]
};

int32_t DPS368_tmp = 0;
int32_t DPS368_prs = 0;

static dps368_settings_t dps368_settings = 
{
	0x03,		//TMP_PRC: 	
				// 0000 - single. (Default) - Measurement time 3.6 ms.
				// 0001 - 2 times.
				// 0010 - 4 times.
				// 0011 - 8 times.
				// 0100 - 16 times.
				// 0101 - 32 times.
				// 0110 - 64 times..
				// 0111 - 128 times.	
	0x02,		//TMP_RATE:	
				// 000 - 1 measurement pr. sec.
				// 001 - 2 measurements pr. sec.
				// 010 - 4 measurements pr. sec.
				// 011 - 8 measurements pr. sec.
				// 100 - 16 measurements pr. sec.
				// 101 - 32 measurements pr. sec.
				// 110 - 64 measurements pr. sec.
				// 111 - 128 measurements pr. sec.						
	0x1, 		//TMP_EXT:	
				// 0 - Internal sensor (in ASIC)
				// 1 - External sensor (in pressure sensor MEMS element)

	0x03,		//PM_PRC:	
				// 0000 - Single. (Low Precision)
				// 0001 - 2 times (Low Power).
				// 0010 - 4 times.
				// 0011 - 8 times.
				// 0100 - 16 times (Standard).
				// 0101 - 32 times.
				// 0110 - 64 times (High Precision).
				// 0111 - 128 times.						
	0x02,		//PM_RATE:	
				// 000 - 1 measurements pr. sec.
				// 001 - 2 measurements pr. sec.
				// 010 - 4 measurements pr. sec.
				// 011 - 8 measurements pr. sec.
				// 100 - 16 measurements pr. sec.
				// 101 - 32 measurements pr. sec.
				// 110 - 64 measurements pr. sec.
				// 111 - 128 measurements pr. sec.
	0x00,		//NA
	
	MEAS_CONTI_PRS_TMP,	//MEAS_MODE
	0x00,				//NA
	
				//INT_CFG
	1,			//Set SPI mode: 
				// 0 - 4-wire interface.
				// 1 - 3-wire interface.	
	1,			//Enable the FIFO: 
				// 0 - Disable.
				// 1 - Enable.		
	0,			//Pressure result bit-shift: 
				// 0 - no shift.
				// 1 - shift result right in data register. 
				// Note: Must be set to '1' when the oversampling rate is >8 times.		
	0,			//Temperature result bit-shift: 
				// 0 - no shift. 
				// 1 - shift result right in data register. 
				// Note: Must be set to '1' when the oversampling rate is >8 times.		
	1,			//Generate interupt when a pressure measurement is ready: 
				// 0 - Disable. 
				// 1 - Enable.	
	1,			//Generate interupt when a pressure measurement is ready: 
				// 0 - Disable. 
				// 1 - Enable.	
	1,			//Generate interupt when the FIFO is full: 
				// 0 - Disable. 
				// 1 - Enable.		
	0,			//Interupt (on SDO pin) active level: 
				// 0 - Active low. 
				// 1 - Active high.	
};

static int32_t com_scale[8] = 
{
	 524288,	//0b0000 1 (single)
	1572864,	//0b0001 2 times (Low Power)
	3670016,	//0b0010 4 times
	7864320,	//0b0011 8 times
	 253952,	//0b0100 16 times (Standard)
	 516096,	//0b0101 32 times
	1040384,	//0b0110 64 times (High Precision)
	2088960		//0b0111 128 times
};

void DPS368_Delayms(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

void DPS368_Delayus(unsigned int dly)
{
	k_usleep(dly);
}

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

void PressureInterruptHandle(void)
{
	pressure_interrupt_flag = true;
}

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

/******************************************************************************
 写入多个字节
******************************************************************************/
int DPS368_WriteRegMulti(DPS368_REG reg, uint8_t *value, uint8_t len)
{
	int32_t ret;

	ret = pressure_dev_ctx.write_reg(pressure_dev_ctx.handle, reg, value, len);
	if(ret != 0)
	{
		ret = DPS368_ERROR; 
	}
	else
	{ 
		ret = DPS368_NO_ERROR;
	}
	
	return ret;
}

/******************************************************************************
 写入一个字节
******************************************************************************/
int DPS368_WriteReg(DPS368_REG reg, uint8_t value)
{ 
    int32_t ret;

	ret = pressure_dev_ctx.write_reg(pressure_dev_ctx.handle, reg, &value, 1);
	if(ret != 0)
	{
		ret = DPS368_ERROR;  
	}
	else
	{
		ret = DPS368_NO_ERROR; 
	}

	return ret;
}

/******************************************************************************
 读取一个字节
******************************************************************************/
int DPS368_ReadReg(DPS368_REG reg, uint8_t *value)
{
    int32_t ret;

	ret = pressure_dev_ctx.read_reg(pressure_dev_ctx.handle, reg, value, 1);
    if(ret != 0)
    {
        ret = DPS368_ERROR;
    }
    else
    {
        ret = DPS368_NO_ERROR;
    }
	
    return ret;
}

/******************************************************************************
 读取多个字节
******************************************************************************/
int DPS368_ReadRegMulti(DPS368_REG reg, uint8_t *value, uint8_t len)
{
    int32_t ret;

	ret = pressure_dev_ctx.read_reg(pressure_dev_ctx.handle, reg, value, len);
    if(ret != 0)
        ret = DPS368_ERROR;
    else
        ret = DPS368_NO_ERROR;
	
    return ret;
}

void DPS368_GetChipID(uint8_t *data)
{
	DPS368_ReadReg(REG_ID, data);
}

void DPS368_GetMeasTmpCfg(uint8_t *data)
{
	DPS368_ReadReg(REG_TMP_CFG, data);
}

void DPS368_SetMeasTmpCfg(void)
{
	//Temperature measurement configuration
	//b7		b6	b5	b4		b3	b2	b1	b0
	//TMP_EXT 	TMP_RATE[2:0] 	TMP_PRC[3:0]
	//TMP_EXT:
	//Temperature measurement
	// 0 - Internal sensor (in ASIC)
	// 1 - External sensor (in pressure sensor MEMS element)
	//TMP_RATE[2:0]:
	//Temperature measurement rate:
	// 000 - 1 measurement pr. sec.
	// 001 - 2 measurements pr. sec.
	// 010 - 4 measurements pr. sec.
	// 011 - 8 measurements pr. sec.
	// 100 - 16 measurements pr. sec.
	// 101 - 32 measurements pr. sec.
	// 110 - 64 measurements pr. sec.
	// 111 - 128 measurements pr. sec.
	//Applicable for measurements in Background mode only
	//TMP_PRC[3:0]:
	//Temperature oversampling (precision):
	// 0000 - single. (Default) - Measurement time 3.6 ms.
	// Note: Following are optional, and may not be relevant:
	// 0001 - 2 times.
	// 0010 - 4 times.
	// 0011 - 8 times.
	// 0100 - 16 times.
	// 0101 - 32 times.
	// 0110 - 64 times..
	// 0111 - 128 times.
	// 1xxx - Reserved.
	uint8_t data;

	//data = dps368_settings.tmp_ext<<7 | dps368_settings.tmp_rate<<4 | dps368_settings.tmp_prc;
	data = *(uint8_t*)&dps368_settings.tmp_cfg;
#ifdef PRESSURE_DEBUG
	LOGD("data:0x%02X, ext:%d, rate:%d, prc:%d", data, dps368_settings.tmp_cfg.ext, dps368_settings.tmp_cfg.rate, dps368_settings.tmp_cfg.prc);
#endif
	DPS368_WriteReg(REG_TMP_CFG, data);
}

void DPS368_GetMeasPrsCfg(uint8_t *data)
{
	DPS368_ReadReg(REG_PRS_CFG, data);
}

void DPS368_SetMeasPrsCfg(void)
{
	//Pressure measurement configuration
	//b7	b6	b5	b4		b3	b2	b1	b0
	//-		PM_RATE[2:0]	PM_PRC[3:0]
	//PM_RATE[2:0]:
	//Pressure measurement rate:
	// 000 - 1 measurements pr. sec.
	// 001 - 2 measurements pr. sec.
	// 010 - 4 measurements pr. sec.
	// 011 - 8 measurements pr. sec.
	// 100 - 16 measurements pr. sec.
	// 101 - 32 measurements pr. sec.
	// 110 - 64 measurements pr. sec.
	// 111 - 128 measurements pr. sec.
	//Applicable for measurements in Background mode only
	//PM_PRC[3:0]:
	//Pressure oversampling rate:
	// 0000 - Single. (Low Precision)
	// 0001 - 2 times (Low Power).
	// 0010 - 4 times.
	// 0011 - 8 times.
	// 0100 *)- 16 times (Standard).
	// 0101 *) - 32 times.
	// 0110 *) - 64 times (High Precision).
	// 0111 *) - 128 times.
	// 1xxx - Reserved
	//*) Note: Use in combination with a bit shift. See Interrupt and FIFO configuration (CFG_REG) register
	uint8_t data;

	//data = dps368_settings.pm_rate<<4 | dps368_settings.pm_prc;
	data = *(uint8_t*)&dps368_settings.prs_cfg;
	DPS368_WriteReg(REG_PRS_CFG, data);
}

void DPS368_GetMeasModeCfg(DPS368_MEAS_CTRL *data)
{
	//Setup measurement mode.
	//b7		b6			b5		b4			b3	b2	b1	b0
	//COEF_RDY SENSOR_RDY 	TMP_RDY PRS_RDY 	- 	MEAS_CTRL[2:0]
	//COEF_RDY: Coefficients will be read to the Coefficents Registers after startup:
	//			0 - Coefficients are not available yet.
	//			1 - Coefficients are available.
	//SENSOR_RDY: The pressure sensor is running through self initialization after start-up.
	//				0 - Sensor initialization not complete
	//				1 - Sensor initialization complete
	// It is recommend not to start measurements until the sensor has completed the self intialization.
	//TMP_RDY: Temperature measurement ready
	//			1 - New temperature measurement is ready. Cleared when temperature measurement is read.
	//PRS_RDY: Pressure measurement ready
	//			1 - New pressure measurement is ready. Cleared when pressurement measurement is read.
	//MEAS_CTRL: Set measurement mode and type:
	//				Standby Mode
	//				000 - Idle / Stop background measurement
	//				Command Mode
	//				001 - Pressure measurement
	//				010 - Temperature measurement
	//				011 - na.
	//				100 - na.
	//				Background Mode
	//				101 - Continous pressure measurement
	//				110 - Continous temperature measurement
	//				111 - Continous pressure and temperature measurement
	
	DPS368_ReadReg(REG_MEAS_CFG, data);
}

void DPS368_SetMeasModeCfg(DPS368_MEAS_CTRL mode)
{
	//Setup measurement mode.
	//b7		b6			b5		b4			b3	b2	b1	b0
	//COEF_RDY SENSOR_RDY 	TMP_RDY PRS_RDY 	- 	MEAS_CTRL[2:0]
	//COEF_RDY: Coefficients will be read to the Coefficents Registers after startup:
	//			0 - Coefficients are not available yet.
	//			1 - Coefficients are available.
	//SENSOR_RDY: The pressure sensor is running through self initialization after start-up.
	//				0 - Sensor initialization not complete
	//				1 - Sensor initialization complete
	// It is recommend not to start measurements until the sensor has completed the self intialization.
	//TMP_RDY: Temperature measurement ready
	//			1 - New temperature measurement is ready. Cleared when temperature measurement is read.
	//PRS_RDY: Pressure measurement ready
	//			1 - New pressure measurement is ready. Cleared when pressurement measurement is read.
	//MEAS_CTRL: Set measurement mode and type:
	//				Standby Mode
	//				000 - Idle / Stop background measurement
	//				Command Mode
	//				001 - Pressure measurement
	//				010 - Temperature measurement
	//				011 - na.
	//				100 - na.
	//				Background Mode
	//				101 - Continous pressure measurement
	//				110 - Continous temperature measurement
	//				111 - Continous pressure and temperature measurement

	dps368_settings.meas_cfg.ctrl = (mode&0x07);
	DPS368_WriteReg(REG_MEAS_CFG, (mode&0x07));
}

void DPS368_GetConfig(uint8_t *data)
{
	DPS368_ReadReg(REG_CFG, data);
}

void DPS368_SetConfig(void)
{
	//Configuration of interupts, measurement data shift, and FIFO enable.
	//b7		b6		b5		b4		b3		b2		b1		b0
	//INT_HL INT_FIFO INT_TMP INT_PRS T_SHIFT P_SHIFT FIFO_EN SPI_MODE
	//INT_HL:Interupt (on SDO pin) active level:
	//			0 - Active low.
	//			1 - Active high.
	//INT_FIFO:Generate interupt when the FIFO is full:
	//			0 - Disable.
	//			1 - Enable.
	//INT_TMP:Generate interupt when a temperature measurement is ready:
	//			0 - Disable.
	//			1 - Enable.
	//INT_PRS:Generate interupt when a pressure measurement is ready:
	//			0 - Disable.
	//			1 - Enable.
	//T_SHIFT:Temperature result bit-shift
	//			0 - no shift.
	//			1 - shift result right in data register.
	//Note: Must be set to '1' when the oversampling rate is >8 times.
	//P_SHIFT:Pressure result bit-shift
	//			0 - no shift.
	//			1 - shift result right in data register.
	//Note: Must be set to '1' when the oversampling rate is >8 times.
	//FIFO_EN:Enable the FIFO:
	//			0 - Disable.
	//			1 - Enable.
	//SPI_MODE:Set SPI mode:
	//			0 - 4-wire interface.
	//			1 - 3-wire interface.
	uint8_t data;

	if(dps368_settings.tmp_cfg.prc > 0b0011)
		dps368_settings.int_cfg.t_shift = 1;
	else
		dps368_settings.int_cfg.t_shift = 0;

	if(dps368_settings.prs_cfg.prc > 0b0011)
		dps368_settings.int_cfg.p_shift = 1;
	else
		dps368_settings.int_cfg.p_shift = 0;
	
	switch(dps368_settings.meas_cfg.ctrl)
	{
	case MEAS_CMD_PSR:
	case MEAS_CMD_TMP:
		dps368_settings.int_cfg.fifo_en = 0;
		break;

	case MEAS_CONTI_PRS:
	case MEAS_CONTI_TMP:
	case MEAS_CONTI_PRS_TMP:
		dps368_settings.int_cfg.fifo_en = 1;
		break;
	}
	data = *(uint8_t*)&dps368_settings.int_cfg;
	DPS368_WriteReg(REG_CFG, data);
}

void DPS368_Reset(void)
{
	//FIFO flush and soft reset.
	//b7			b6 b5 b4	b3 b2 b1 b0
	//FIFO_FLUSH 	   - 		SOFT_RST[3:0]
	//FIFO_FLUSH:
	// 1 - Empty FIFO
	// After reading out all data from the FIFO, write '1' to clear all old data.
	//SOFT_RST: 
	// Write '1001' to generate a soft reset. A soft reset will run though the same sequences as in power-on reset.

	DPS368_WriteReg(REG_RESET, 0x89);
}

void DPS368_GetTwosComplement(int32_t *raw, uint8_t length)
{
	if(*raw & ((uint32_t)1<<(length - 1)))
	{
		*raw -= (uint32_t)1<<length;
	}
}

void DPS368_GetCoefs(void)
{
	DPS368_ReadRegMulti(REG_COEF, DPS368_coefs, sizeof(DPS368_coefs));
#ifdef PRESSURE_DEBUG
	LOGD("coef:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", 
							DPS368_coefs[0],DPS368_coefs[1],DPS368_coefs[2],DPS368_coefs[3],DPS368_coefs[4],DPS368_coefs[5],
							DPS368_coefs[6],DPS368_coefs[7],DPS368_coefs[8],DPS368_coefs[9],DPS368_coefs[10],DPS368_coefs[11],
							DPS368_coefs[12],DPS368_coefs[13],DPS368_coefs[14],DPS368_coefs[15],DPS368_coefs[16],DPS368_coefs[17]);
#endif
	//compose coefficients from buffer content
	m_c0 = ((uint32_t)DPS368_coefs[0]<<4) | (((uint32_t)DPS368_coefs[1]>>4)&0x0F);
	DPS368_GetTwosComplement(&m_c0, 12);
	m_c1 = (((uint32_t)DPS368_coefs[1]&0x0F)<<8) | (uint32_t)DPS368_coefs[2];
	DPS368_GetTwosComplement(&m_c1, 12);
	m_c00 = ((uint32_t)DPS368_coefs[3]<<12) | ((uint32_t)DPS368_coefs[4]<<4) | (((uint32_t)DPS368_coefs[5]>>4)&0x0F);
	DPS368_GetTwosComplement(&m_c00, 20);
	m_c10 = (((uint32_t)DPS368_coefs[5]&0x0F)<<16) | ((uint32_t)DPS368_coefs[6]<<8) | (uint32_t)DPS368_coefs[7];
	DPS368_GetTwosComplement(&m_c10, 20);
	m_c01 = ((uint32_t)DPS368_coefs[8]<<8) | (uint32_t)DPS368_coefs[9];
	DPS368_GetTwosComplement(&m_c01, 16);
	m_c11 = ((uint32_t)DPS368_coefs[10]<<8) | (uint32_t)DPS368_coefs[11];
	DPS368_GetTwosComplement(&m_c11, 16);
	m_c20 = ((uint32_t)DPS368_coefs[12]<<8) | (uint32_t)DPS368_coefs[13];
	DPS368_GetTwosComplement(&m_c20, 16);
	m_c21 = ((uint32_t)DPS368_coefs[14]<<8) | (uint32_t)DPS368_coefs[15];
	DPS368_GetTwosComplement(&m_c21, 16);
	m_c30 = ((uint32_t)DPS368_coefs[16]<<8) | (uint32_t)DPS368_coefs[17];
	DPS368_GetTwosComplement(&m_c30, 16);
#ifdef PRESSURE_DEBUG
	LOGD("c0:%d,c1:%d,c00:%d,c10:%d,c01:%d,c11:%d,c20:%d,c21:%d,c30:%d", m_c0,m_c1,m_c00,m_c10,m_c01,m_c11,m_c20,m_c21,m_c30);
#endif	
}

void DPS368_GetTmpRawData(int32_t *data)
{
	uint8_t status,reg_tmp[3] = {0};

	if(dps368_settings.meas_cfg.ctrl == MEAS_CMD_TMP)
	{
		while(1)
		{
			DPS368_GetMeasModeCfg(&status);
			if((status&0x20) == 0x20)
				break;
		}
	}

	DPS368_ReadRegMulti(REG_TMP_B2, reg_tmp, sizeof(reg_tmp));
	*data = (uint32_t)reg_tmp[0]<<16 | (uint32_t)reg_tmp[1]<<8 | (uint32_t)reg_tmp[2];
	DPS368_GetTwosComplement(data, 24);
#ifdef PRESSURE_DEBUG
	LOGD("tmp:0x%02X, %d", *data, *data);
#endif	
}

void DPS368_GetPrsRawData(int32_t *data)
{
	DPS368_MEAS_CTRL mode;
	uint8_t status,reg_psr[3] = {0};

	if(dps368_settings.meas_cfg.ctrl == MEAS_CMD_PSR)
	{
		while(1)
		{
			DPS368_GetMeasModeCfg(&status);
			if((status&0x10) == 0x10)
				break;
		}
	}
	
	DPS368_ReadRegMulti(REG_PSR_B2, reg_psr, sizeof(reg_psr));
	*data = (uint32_t)reg_psr[0]<<16 | (uint32_t)reg_psr[1]<<8 | (uint32_t)reg_psr[2];
	DPS368_GetTwosComplement(data, 24);
#ifdef PRESSURE_DEBUG
	LOGD("prs:0x%02X, %d", *data, *data);
#endif	
}

void DPS368_CalculateTmp(void)
{
	//1. Read the temperature calibration coefficients ( c0 and c1 ) from the Calibration Coefficients (COEF)register.
	//Note: The coefficients read from the coefficient register are 12 bit 2′s complement numbers.
	//2. Choose scaling factor kT (for temperature) based on the chosen precision rate. The scaling factors are	listed in Table 9.
	//3. Read the temperature result from the temperature register or FIFO.
	//Note: The temperature measurements read from the temperature result register (or FIFO) are 24 bit 2′s complement numbers.
	//4. Calculate scaled measurement results.
	//		Traw_sc = Traw/kT 
	//5. Calculate compensated measurement results.
	//		Tcomp (°C) = c0*0.5 + c1*Traw_sc
	float Traw_sc,Tcomp;

	Traw_sc = (float)DPS368_tmp/com_scale[dps368_settings.tmp_cfg.prc];
	Tcomp = m_c0*0.5 + m_c1*Traw_sc;
#ifdef PRESSURE_DEBUG
	LOGD("Tcomp:%f,Traw_sc:%f", Tcomp, Traw_sc);
#endif	
}

void DPS368_CalculatePrs(void)
{
	//1. Read the pressure calibration coefficients (c00, c10, c20, c30, c01, c11, and c21) from the Calibration Coefficient register.
	//Note: The coefficients read from the coefficient register are 2's complement numbers.
	//2. Choose scaling factors kT (for temperature) and kP (for pressure) based on the chosen precision rate. The scaling factors are listed in Table 9.
	//3. Read the pressure and temperature result from the registers or FIFO.
	//Note: The measurements read from the result registers (or FIFO) are 24 bit 2′s complement numbers.	
	//Depending on the chosen measurement rates, the temperature may not have been measured since the last pressure measurement.
	//4. Calculate scaled measurement results.
	//		Praw_sc = Praw/kP
	//		Traw_sc = Traw/kT
	//5. Calculate compensated measurement results.
	//		Pcomp(Pa) = c00 + Praw_sc*(c10 + Praw_sc *(c20+ Praw_sc *c30)) + Traw_sc *c01 + Traw_sc *Praw_sc *(c11+Praw_sc*c21)
	float Traw_sc,Praw_sc,Pcomp;

	Praw_sc = (float)DPS368_prs/com_scale[dps368_settings.prs_cfg.prc];
	Traw_sc = (float)DPS368_tmp/com_scale[dps368_settings.tmp_cfg.prc];
	Pcomp = m_c00 + Praw_sc*(m_c10 + Praw_sc*(m_c20 + Praw_sc*m_c30)) + Traw_sc*m_c01 + Traw_sc*Praw_sc*(m_c11 + Praw_sc*m_c21);
#ifdef PRESSURE_DEBUG
	LOGD("Pcomp:%f,Praw_sc:%f", Pcomp, Praw_sc);
#endif

	g_prs = Pcomp;
}

void DPS368_CalculateFIFOData(void)
{
	int32_t data;
	float Traw_sc,Praw_sc,Pcomp;

	DPS368_GetPrsRawData(&data);
#ifdef PRESSURE_DEBUG
	LOGD("data:%x", data);
#endif	
	switch(data&0x00000001)
	{
	case 0://tmp
		DPS368_tmp = data;
		DPS368_CalculateTmp();
		break;
	case 1://prs
		DPS368_prs = data;
		DPS368_CalculatePrs();
		break;
	}
}

void DPS368_GetIntStatus(uint8_t *status)
{
	//Interrupt status register. The register is cleared on read.
	//b7 b6 b5 b4 b3	b2				b1		b0
	//    	-			INT_FIFO_FULL 	INT_TMP INT_PRS
	//INT_FIFO_FULL:Status of FIFO interrupt
	//				0 - Interrupt not active
	//				1 - Interrupt active
	//INT_TMP:Status of temperature measurement interrupt
	//				0 - Interrupt not active
	//				1 - Interrupt active
	//INT_PRS:Status of pressure measurement interrupt
	//				0 - Interrupt not active
	//				1 - Interrupt active
	
	DPS368_ReadReg(REG_INT_STS, status);
}

void DPS368_GetFIFOStatus(uint8_t *status)
{
	//FIFO status register
	//b7 b6 b5 b4 b3 b2		b1			b0
	//		-				FIFO_FULL	FIFO_EMPTY
	//FIFO_FULL: 0 - The FIFO is not full, 1 - The FIFO is full
	//FIFO_EMPTY: 0 - The FIFO is not empty, 1 - The FIFO is empty
	
	DPS368_ReadReg(REG_FIFO_STS, status);
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

	pressure_dev_ctx.write_reg = platform_write;
	pressure_dev_ctx.read_reg  = platform_read;
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
	{
		uint8_t data;

		while(1)
		{
			DPS368_GetMeasModeCfg(&data);
			if((data&0xC0) == 0xC0)
				break;
		}
		
		DPS368_SetMeasTmpCfg();
		DPS368_SetMeasPrsCfg();
		DPS368_GetCoefs();

		if((dps368_settings.meas_cfg.ctrl == MEAS_CONTI_PRS)
			|| (dps368_settings.meas_cfg.ctrl == MEAS_CONTI_TMP)
			|| (dps368_settings.meas_cfg.ctrl == MEAS_CONTI_PRS_TMP)
			)
		{
			DPS368_Start(dps368_settings.meas_cfg.ctrl);
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

void pressure_start(void)
{
	pressure_start_flag = true;
}

void pressure_stop(void)
{
	pressure_stop_flag = true;
}

void DPS368_Start(DPS368_MEAS_CTRL work_mode)
{
	switch(work_mode)
	{
	case MEAS_STANDBY:		//- Idle / Stop background measurement
		DPS368_SetMeasModeCfg(dps368_settings.meas_cfg.ctrl);
		break;

	case MEAS_CMD_PSR:		// - Pressure measurement
		DPS368_SetConfig();
		DPS368_SetMeasModeCfg(MEAS_CMD_TMP);
		DPS368_GetTmpRawData(&DPS368_tmp);
		DPS368_CalculateTmp();

		DPS368_SetMeasModeCfg(MEAS_CMD_PSR);
		DPS368_GetPrsRawData(&DPS368_prs);
		DPS368_CalculatePrs();
	
		DPS368_SetMeasModeCfg(MEAS_STANDBY);	
		break;
		
	case MEAS_CMD_TMP:		// - Temperature measurement:
		DPS368_SetConfig();
		DPS368_SetMeasModeCfg(MEAS_CMD_TMP);
		DPS368_GetTmpRawData(&DPS368_tmp);
		DPS368_CalculateTmp();
		DPS368_SetMeasModeCfg(MEAS_STANDBY);
		break;

	case MEAS_CONTI_PRS:		// - Continous pressure measurement
	case MEAS_CONTI_TMP:		// - Continous temperature measurement
	case MEAS_CONTI_PRS_TMP:	// - Continous pressure and temperature measurement:
		DPS368_SetConfig();
		DPS368_SetMeasModeCfg(dps368_settings.meas_cfg.ctrl);
		break;
	}
}

void DPS368_Stop(void)
{
	DPS368_SetMeasModeCfg(MEAS_STANDBY);
}

void DPS368MsgProcess(void)
{
	if(pressure_start_flag)
	{
		DPS368_Start(MEAS_CMD_PSR);
		pressure_start_flag = false;
	}

	if(pressure_stop_flag)
	{
		DPS368_Stop();
		pressure_stop_flag = false;
	}
	
	if(pressure_interrupt_flag)
	{
		uint8_t status;

		DPS368_GetIntStatus(&status);
	#ifdef PRESSURE_DEBUG
		LOGD("int status:%x", status);
	#endif
		if((status&0x01) == 0x01)
		{
		#ifdef PRESSURE_DEBUG
			LOGD("int_prs");
		#endif
			DPS368_GetPrsRawData(&DPS368_prs);
			DPS368_CalculatePrs();
		}
		
		if((status&0x02) == 0x02)
		{
		#ifdef PRESSURE_DEBUG
			LOGD("int_tmp");
		#endif
			DPS368_GetTmpRawData(&DPS368_tmp);
			DPS368_CalculateTmp();	
		}

		if((status&0x04) == 0x04)
		{
		#ifdef PRESSURE_DEBUG
			LOGD("int_fifo_full");
		#endif
			while(1)
			{
				DPS368_CalculateFIFOData();

				DPS368_GetFIFOStatus(&status);
			#ifdef PRESSURE_DEBUG
				LOGD("fifo status:%x", status);
			#endif
				if((status&0x01) == 0x01)
				{
					break;
				}
			}
		}
	
		pressure_interrupt_flag = false;
	}
}
