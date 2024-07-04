/****************************************Copyright (c)************************************************
** File Name:			    lps22df.c
** Descriptions:			lps22df interface process source file
** Created By:				xie biao
** Created Date:			2024-06-24
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
#include "lps22df.h"
#include "logger.h"

#ifdef PRESSURE_LPS22DF

#define ARRAYNUM(a) (sizeof(a)/sizeof(a[0]))

static struct device *i2c_pressure;
static struct device *gpio_pressure;
static struct gpio_callback gpio_cb;

int32_t LPS22DF_tmp = 0;
int32_t LPS22DF_prs = 0;

lps22df_settings_t lps22df_settings = 
{
	//INTERRUPT_CFG:
	0b0,		//PHE: Enable interrupt generation on pressure high event. Default value: 0
				//		0: disable interrupt request;
				//		1: enable interrupt request on pressure value higher than preset threshold)
	0b0,		//PLE: Enable interrupt generation on pressure low event. Default value: 0
				//		0: disable interrupt request;
				//		1: enable interrupt request on pressure value lower than preset threshold.
	0b0,		//LIR: Latch interrupt request to the INT_SOURCE (24h) register. Default value: 0
				//		0: interrupt request not latched; 
				//		1: interrupt request latched.
	0b0,		//NA
	0b0,		//RESET_AZ: Reset AUTOZERO function. Default value: 0
				//			0: normal mode; 
				//			1: reset AUTOZERO function.
	0b0,		//AUTOZERO: Enable AUTOZERO function. Default value: 0
				//			0: normal mode; 
				//			1: AUTOZERO enabled.
	0b0,		//RESET_ARP: Reset AUTOREFP function. Default value: 0
				//				0: normal mode; 
				//				1: reset AUTOREFP function.
	0b0,		//AUTOREFP: Enable AUTOREFP function. Default value: 0
				//			0: normal mode; 
				//			1: AUTOREFP enabled
	//THS_P
	0x0000,		//THS[14:0]
	//IF_CTRL:
	0b0,		//NA
	0b0,		//CS_PU_DIS: Disable pull-up on the CS pin. Default value: 0
				//				0: CS pin with pull-up; 
				//				1: CS pin pull-up disconnected.
	0b0,		//INT_PD_DIS: Disable pull-down on the INT pin. Default value: 0
				//				0: INT pin with pull-down; 
				//				1: INT pin pull-down disconnected.
	0b0,		//SDO_PU_EN: Enable pull-up on the SDO pin. Default value: 0
				//				0: SDO pin pull-up disconnected; 
				//				1: SDO pin with pull-up.
	0b0,		//SDA_PU_EN: Enable pull-up on the SDA pin. Default value: 0
				//				0: SDA pin pull-up disconnected; 
				//				1: SDA pin with pull-up.
	0b0,		//SIM: SPI serial interface mode selection. Default value: 0
				//			0: 4-wire interface; 
				//			1: 3-wire interface.
	0b0,		//I2C_I3C_DIS: Disable I2C and I3C digital interfaces. Default value: 0
				//				0: enable I2C and I3C digital interfaces; 
				//				1: disable I2C and I3C digital interfaces.
	0b0,		//INT_EN_I3C: Enable INT pin with MIPI I3CSM. If the INT_EN_I3C bit is set, the INT pin is polarized as OUT. Default value: 0
				//				0: INT disabled with MIPI I3CSM; 
				//				1: INT enabled with MIPI I3CSM.
	//CTRL_REG1
	0b111,		//AVG[2:0]: Average selection. Default value: 000
				//AVG[2:0] 	Averaging of pressure and temperature
				//	000 		4
				//	001 		8
				//	010 		16
				//	011 		32
				//	100 		64
				//	101 		128
				//	111 		512
	0b0010,		//ODR[3:0]: Output data rate selection. Default value: 0000
				//ODR[3:0] 	ODR of pressure, temperature
				//	0000 		Power-down / one-shot
				//	0001 		1 Hz
				//	0010 		4 Hz
				//	0011 		10 Hz
				//	0100 		25 Hz
				//	0101 		50 Hz
				//	0110 		75 Hz
				//	0111 		100 Hz
				//	1xxx 		200 Hz
	0b0,		//NA
	//CTRL_REG2
	0b0,		//ONESHOT: Enable one-shot mode. Default value: 0
				// 			0: idle mode; 
				//			1: a new dataset is acquired.
	0b0,		//NA
	0b0,		//SWRESET: Software reset. Default value: 0
				//			0: normal mode; 
				//			1: software reset. The bit is self-cleared when the reset is completed.
	0b1,		//BDU: Block data update. Default value: 0
				//		0: continuous update;
				//		1: output registers not updated until MSB and LSB have been read.
	0b1,		//EN_LPFP: Enable low-pass filter on pressure data. Default value: 0
				//			0: disable, 
				//			1: enable.
	0b0,		//LFPF_CFG: Low-pass filter configuration. Default value: 0
				//			0: ODR/4; 
				//			1: ODR/9.
	0b0,		//ZERO
	0b0,		//BOOT: Reboot memory content. Default value: 0
				//			0: normal mode; 
				//			1: reboot memory content.
	//CTRL_REG3
	0b1,		//IF_ADD_INC: Register address automatically incremented during a multiple byte access with a serial interface (I2C or SPI). Default value: 1
				//				0: disable, 
				//				1: enable.
	0b0,		//PP_OD: Push-pull/open-drain selection on interrupt pin. Default value: 0
				//			0: push-pull; 
				//			1: open-drain.
	0b0,		//ZERO_1
	0b1,		//INT_H_L: Select interrupt active-high, active-low. Default value: 0
				//			0: active-high; 
				//			1: active-low.
	0b0000,		//ZERO_2
	//CTRL_REG4
	0b1,		//INT_F_OVR: FIFO overrun status on INT pin. Default value: 0
				//				0: not overwritten; 
				//				1: at least one sample in the FIFO has been overwritten.
	0b1,		//INT_F_WTM: FIFO threshold (watermark) status on INT pin. Default value: 0
				//				0: FIFO is lower than WTM level; 
				//				1: FIFO is equal to or higher than WTM level.
	0b1,		//INT_F_FULL: FIFO full flag on INT pin. Default value: 0
				//				0: FIFO empty; 
				//				1: FIFO full with 128 unread samples.
	0b0,		//NA
	0b1,		//INT_EN: Interrupt signal on INT pin. Default value: 0
				//			0: disable; 
				//			1: enable.
	0b1,		//DRDY: Date-ready signal on INT pin. Default value: 0
				//		0: disable; 
				//		1: enable.
	0b1,		//DRDY_PLS: Data-ready pulsed on INT pin. Default value: 0
				//			0: disable; 
				//			1: enable data-ready pulsed on INT pin, pulse width around 5 μs.
	0b0,		//ZERO
	//FIFO_CTRL
	0b11,		//F_MODE[1:0]: Selects triggered FIFO modes. Default value: 00
	0b0,		//TRIG_MODES: Enables triggered FIFO modes. Default value: 0
				//TRIG_MODES 	F_MODE[1:0] 	Mode
				//		x 			00 			Bypass
				//		0 			01 			FIFO mode
				//		0 			1x 			Continuous (dynamic-stream)
				//		1 			01 			Bypass-to-FIFO
				//		1 			10 			Bypass-to-continuous (dynamic-stream)
				//		1 			11 			Continuous (dynamic-stream)-to-FIFO
	0b1,		//STOP_ON_WTM: Stop-on-FIFO watermark. Enables FIFO watermark level use. Default value: 0
				//				0: disable; 
				//				1: enable.
	0b0000,		//ZERO
	//FIFO_WTM
	0x20,		//WTM[6:0]
	//REF_P
	0x0000,		//REFL[15:0]
	//I3C_IF_CTRL_ADD
	0b00,		//I3C_Bus_Avb_Sel[1:0]
				//These bits are used to select the bus available time when I3C IBI is used. Default value: 00
				//	00: bus available time equal to 50 μsec;
				//	01: bus available time equal to 2 μsec;
				//	10: bus available time equal to 1 msec;
				//	11: bus available time equal to 25 msec)
	0b000,		//ZERO_1
	0b0,		//ASF_ON: Enables antispike filters. Default value: 0
				//			0: antispike filters are managed by protocol and turned off after the broadcast address;
				//			1: antispike filters on SCL and SDA lines are always enabled.
	0b0,		//ZERO_2
	0b1,		//ONE
	//RPDS
	0x0000,		//RPDS[15:0]
	//MEAS_CTRL
	MEAS_ONE_SHOT
};

void LPS22DF_Delayms(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

void LPS22DF_Delayus(unsigned int dly)
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
	rslt = I2C_write_data(LPS22DF_I2C_ADDRESS, data, len+1);
#else	
	rslt = i2c_write(handle, data, len+1, LPS22DF_I2C_ADDRESS);
#endif
	return rslt;
}

static int32_t platform_read(struct device *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	uint32_t rslt = 0;

#ifdef GPIO_ACT_I2C
	rslt = I2C_write_data(LPS22DF_I2C_ADDRESS, &reg, 1);
	if(rslt == 0)
	{
		rslt = I2C_read_data(LPS22DF_I2C_ADDRESS, bufp, len);
	}
#else
	rslt = i2c_write(handle, &reg, 1, LPS22DF_I2C_ADDRESS);
	if(rslt == 0)
	{
		rslt = i2c_read(handle, bufp, len, LPS22DF_I2C_ADDRESS);
	}
#endif	
	return rslt;
}

/******************************************************************************
 写入多个字节
******************************************************************************/
int LPS22DF_WriteRegMulti(LPS22DF_REG reg, uint8_t *value, uint8_t len)
{
	int32_t ret;

	ret = pressure_dev_ctx.write_reg(pressure_dev_ctx.handle, reg, value, len);
	if(ret != 0)
	{
		ret = LPS22DF_ERROR; 
	}
	else
	{ 
		ret = LPS22DF_NO_ERROR;
	}
	
	return ret;
}

/******************************************************************************
 写入一个字节
******************************************************************************/
int LPS22DF_WriteReg(LPS22DF_REG reg, uint8_t value)
{ 
    int32_t ret;

	ret = pressure_dev_ctx.write_reg(pressure_dev_ctx.handle, reg, &value, 1);
	if(ret != 0)
	{
		ret = LPS22DF_ERROR;  
	}
	else
	{
		ret = LPS22DF_NO_ERROR; 
	}

	return ret;
}

/******************************************************************************
 读取一个字节
******************************************************************************/
int LPS22DF_ReadReg(LPS22DF_REG reg, uint8_t *value)
{
    int32_t ret;

	ret = pressure_dev_ctx.read_reg(pressure_dev_ctx.handle, reg, value, 1);
    if(ret != 0)
    {
        ret = LPS22DF_ERROR;
    }
    else
    {
        ret = LPS22DF_NO_ERROR;
    }
	
    return ret;
}

/******************************************************************************
 读取多个字节
******************************************************************************/
int LPS22DF_ReadRegMulti(LPS22DF_REG reg, uint8_t *value, uint8_t len)
{
    int32_t ret;

	ret = pressure_dev_ctx.read_reg(pressure_dev_ctx.handle, reg, value, len);
    if(ret != 0)
        ret = LPS22DF_ERROR;
    else
        ret = LPS22DF_NO_ERROR;
	
    return ret;
}

void LPS22DF_GetTwosComplement(int32_t *raw, uint8_t length)
{
	if(*raw & ((uint32_t)1<<(length - 1)))
	{
		*raw -= (uint32_t)1<<length;
	}
}

void LPS22DF_SetInterruptCfg(void)
{
	//Interrupt mode for pressure acquisition configuration (R/W)
	//b7		b6			b5			b4			b3	b2	b1	b0
	//AUTOREFP RESET_ARP	AUTOZERO	RESET_AZ	-	LIR	PLE	PHE
	//AUTOREFP: Enable AUTOREFP function. 
	//				Default value: 0 (0: normal mode; 1: AUTOREFP enabled)
	//RESET_ARP: Reset AUTOREFP function. 
	//				Default value: 0 (0: normal mode; 1: reset AUTOREFP function)
	//AUTOZERO: Enable AUTOZERO function. 
	//				Default value: 0 (0: normal mode; 1: AUTOZERO enabled)
	//RESET_AZ: Reset AUTOZERO function. 
	//				Default value: 0 (0: normal mode; 1: reset AUTOZERO function)
	//LIR: Latch interrupt request to the INT_SOURCE (24h) register. 
	//		Default value: 0 (0: interrupt request not latched; 1: interrupt request latched)
	//PLE: Enable interrupt generation on pressure low event. 
	//		Default value: 0 (0: disable interrupt request; 1: enable interrupt request on pressure value lower than preset threshold)
	//PHE: Enable interrupt generation on pressure high event. 
	//		Default value: 0 (0: disable interrupt request; 1: enable interrupt request on pressure value higher than preset threshold)
	uint8_t data = *(uint8_t*)&lps22df_settings.int_ctrl;
	
	LPS22DF_WriteReg(REG_INTERRUPT_CFG, data);
}

void LPS22DF_GetInterruptCfg(uint8_t *data)
{
	LPS22DF_ReadReg(REG_INTERRUPT_CFG, data);
}

void LPS22DF_SetThrePrs(void)
{
	//THS_P_L:THS[7:0]
	//THS_P_H:THS[14:8]
	uint8_t ths_p[2] = {0};

	ths_p[0] = (uint8_t)(lps22df_settings.ths_p&0x00ff);
	ths_p[1] = (lps22df_settings.ths_p>>8);
	
	LPS22DF_WriteRegMulti(REG_THS_P_L, ths_p, 2);
}

void LPS22DF_GetThrePrs(uint16_t *data)
{
	//THS_P_L:THS[7:0]
	//THS_P_H:THS[14:8]
	uint8_t ths_p[2] = {0};
	
	LPS22DF_ReadRegMulti(REG_THS_P_L, ths_p, 2);
	*data = ((uint16_t)ths_p[1]<<8) | (uint16_t)ths_p[0];
}

void LPS22DF_SetIFCtrl(uint8_t data)
{
	//Interface control register
	//b7			b6			b5	b4			b3			b2			b1			b0
	//INT_EN_I3C 	I2C_I3C_DIS SIM SDA_PU_EN 	SDO_PU_EN 	INT_PD_DIS 	CS_PU_DIS 	-
	//INT_EN_I3C: Enable INT pin with MIPI I3CSM. If the INT_EN_I3C bit is set, the INT pin is polarized as OUT. 
	//				Default value: 0 (0: INT disabled with MIPI I3CSM; 1: INT enabled with MIPI I3CSM)
	//I2C_I3C_DIS: Disable I2C and I3C digital interfaces. 
	//				Default value: 0 (0: enable I2C and I3C digital interfaces; 1: disable I2C and I3C digital interfaces)
	//SIM: SPI serial interface mode selection. 
	//		Default value: 0 (0: 4-wire interface; 1: 3-wire interface)
	//SDA_PU_EN: Enable pull-up on the SDA pin. 
	//				Default value: 0 (0: SDA pin pull-up disconnected; 1: SDA pin with pull-up)
	//SDO_PU_EN: Enable pull-up on the SDO pin. 
	//				Default value: 0 (0: SDO pin pull-up disconnected; 1: SDO pin with pull-up)
	//INT_PD_DIS: Disable pull-down on the INT pin. 
	//				Default value: 0 (0: INT pin with pull-down; 1: INT pin pull-down disconnected)
	//CS_PU_DIS: Disable pull-up on the CS pin. 
	//				Default value: 0 (0: CS pin with pull-up; 1: CS pin pull-up disconnected)

	LPS22DF_WriteReg(REG_IF_CTRL, data);
}

void LPS22DF_GetIFCtrl(uint8_t *data)
{
	LPS22DF_ReadReg(REG_IF_CTRL, data);
}

void LPS22DF_SetCtrl1(void)
{
	//Control register 1
	//b7	b6b5b4b3		b2b1b0
	//0		ODR[3:0]		AVG[2:0]
	//ODR[3:0]: ODR of pressure, temperature
	// 	0000 Power-down / one-shot
	//	0001 1 Hz
	//	0010 4 Hz
	//	0011 10 Hz
	//	0100 25Hz
	//	0101 50 Hz
	//	0110 75 Hz
	//	0111 100 Hz
	//	1xxx 200 Hz
	//AVG[2:0]: Averaging of pressure and temperature
	//	000 4
	//	001 8
	//	010 16
	//	011 32
	//	100 64
	//	101 128
	//	111 512
	uint8_t data = *(uint8_t*)&lps22df_settings.reg1_ctrl;

	switch(lps22df_settings.meas_ctrl)
	{
	case MEAS_ONE_SHOT:
		data = data&0x07;
		break;
		
	case MEAS_CONTINOUS:
		break;
	}

	LPS22DF_WriteReg(REG_CTRL_REG1, (data&0x7f));
}

void LPS22DF_GetCtrl1(uint8_t *data)
{
	LPS22DF_ReadReg(REG_CTRL_REG1, data);
}

void LPS22DF_SetCtrl2(void)
{
	//Control register 2
	//b7	b6	b5			b4		b3	b2		b1	b0
	//BOOT 	0 	LFPF_CFG 	EN_LPFP BDU SWRESET - 	ONESHOT
	//BOOT: Reboot memory content. 
	//			Default value: 0 (0: normal mode; 1: reboot memory content)
	//LFPF_CFG: Low-pass filter configuration. 
	//				Default value: 0 (0: ODR/4; 1: ODR/9)
	//EN_LPFP: Enable low-pass filter on pressure data. 
	//			Default value: 0 (0: disable, 1: enable)
	//BDU: Block data update. 
	//		Default value: 0 (0: continuous update; 1: output registers not updated until MSB and LSB have been read)
	//SWRESET: Software reset. 
	//			Default value: 0 (0: normal mode; 1: software reset). 
	//			The bit is self-cleared when the reset is completed.
	//ONESHOT: Enable one-shot mode. 
	//			Default value: 0 (0: idle mode; 1: a new dataset is acquired)
	uint8_t data = *(uint8_t*)&lps22df_settings.reg2_ctrl;

	data = data|0x01;
	LPS22DF_WriteReg(REG_CTRL_REG2, data);
}

void LPS22DF_GetCtrl2(uint8_t *data)
{
	LPS22DF_ReadReg(REG_CTRL_REG2, data);
}

void LPS22DF_SetCtrl3(void)
{
	//Control register 3
	//b7b6b5b4	b3		b2	b1		b0
	//0	0 0 0	INT_H_L 0 	PP_OD 	IF_ADD_INC
	//INT_H_L: Select interrupt active-high, active-low. 
	//			Default value: 0 (0: active-high; 1: active-low)
	//PP_OD: Push-pull/open-drain selection on interrupt pin. 
	//			Default value: 0 (0: push-pull; 1: open-drain)
	//IF_ADD_INC: Register address automatically incremented during a multiple byte access with a serial interface (I2C or SPI). 
	//			Default value: 1 (0: disable, 1: enable)
	uint8_t data;

	data = *(uint8_t*)&lps22df_settings.reg3_ctrl;
	LPS22DF_WriteReg(REG_CTRL_REG3, data);
}

void LPS22DF_GetCtrl3(uint8_t *data)
{
	LPS22DF_ReadReg(REG_CTRL_REG3, data);
}

void LPS22DF_SetCtrl4(void)
{
	//Control register 4
	//b7	b6			b5		b4		b3	b2			b1			b0
	//0 	DRDY_PLS 	DRDY 	INT_EN 	- 	INT_F_FULL 	INT_F_WTM 	INT_F_OVR
	//DRDY_PLS: Data-ready pulsed on INT pin. 
	//				Default value: 0 (0: disable; 1: enable data-ready pulsed on INT pin, pulse width around 5 μs)
	//DRDY: Date-ready signal on INT pin. 
	//			Default value: 0 (0: disable; 1: enable)
	//INT_EN: Interrupt signal on INT pin. 
	//			Default value: 0 (0: disable; 1: enable)
	//INT_F_FULL: FIFO full flag on INT pin. 
	//				Default value: 0 (0: FIFO empty; 1: FIFO full with 128 unread samples)
	//INT_F_WTM: FIFO threshold (watermark) status on INT pin. 
	//				Default value: 0 (0: FIFO is lower than WTM level; 1: FIFO is equal to or higher than WTM level)
	//INT_F_OVR: FIFO overrun status on INT pin. 
	//				Default value: 0 (0: not overwritten; 1: at least one sample in the FIFO has been overwritten)
	uint8_t data,status;

	data = *(uint8_t*)&lps22df_settings.reg4_ctrl;
	
	switch(lps22df_settings.meas_ctrl)
	{
	case MEAS_ONE_SHOT:
		data = data&0x00;//After polling the status, read the results without interruption to avoid falling detection can't obtain data directly.
		break;
		
	case MEAS_CONTINOUS:
		LPS22DF_GetFIFOCtrl(&status);
		switch(status&0x07)
		{
		case 0b000://Bypass
		case 0b100://Bypass
			data = data&0x70;//Trigger interrupt based on FIFO status rather than data being prepared
			break;
			
		case 0b101://Bypass-to-FIFO
		case 0b110://Bypass-to-continuous (dynamic-stream)
			data = data&0x77;//Trigger interrupt based on FIFO status rather than data being prepared
			break;
			
		case 0b001://FIFO mode
		case 0b010://Continuous (dynamic-stream)
		case 0b011://Continuous (dynamic-stream)
		case 0b111://Continuous (dynamic-stream)-to-FIFO
		default:
			data = data&0x17;//Trigger interrupt based on FIFO status rather than data being prepared
			break;
		}
		break;
	}

	LPS22DF_WriteReg(REG_CTRL_REG4, data);
}

void LPS22DF_GetCtrl4(uint8_t *data)
{
	LPS22DF_ReadReg(REG_CTRL_REG4, data);
}

void LPS22DF_SetFIFOCtrl(void)
{
	//FIFO control register
	//b7b6b5b4	b3			b2			b1b0
	//0 0 0 0	STOP_ON_WTM TRIG_MODES 	F_MODE[1:0]
	//STOP_ON_WTM: Stop-on-FIFO watermark. Enables FIFO watermark level use. 
	//				Default value: 0 (0: disable; 1: enable)
	//TRIG_MODES: Enables triggered FIFO modes. Default value: 0
	//F_MODE[1:0]: FIFO mode selection
	//				TRIG_MODES	F_MODE	Mode
	//					x 		  00 	Bypass
	//					0 		  01 	FIFO mode
	//					0 		  1x 	Continuous (dynamic-stream)
	//					1 		  01 	Bypass-to-FIFO
	//					1 		  10 	Bypass-to-continuous (dynamic-stream)
	//					1 		  11 	Continuous (dynamic-stream)-to-FIFO
	uint8_t data = *(uint8_t*)&lps22df_settings.fifo_ctrl;

	switch(lps22df_settings.meas_ctrl)
	{
	case MEAS_ONE_SHOT:
		data = data&0xFC;
		break;
		
	case MEAS_CONTINOUS:
		break;
	}
	
	LPS22DF_WriteReg(REG_FIFO_CTRL, data);
}

void LPS22DF_GetFIFOCtrl(uint8_t *data)
{
	LPS22DF_ReadReg(REG_FIFO_CTRL, data);
}

void LPS22DF_SetFIFOWTM(void)
{
	//FIFO threshold setting register
	//b7	b6b5b4b3b2b1b0
	//0 	WTM[6:0]
	//WTM[6:0]: FIFO threshold. Watermark level setting. Default value: 0000000
	uint8_t data = *(uint8_t*)&lps22df_settings.fifo_wtm;
	
	LPS22DF_WriteReg(REG_FIFO_WTM, (data&0x7f));
}

void LPS22DF_GetFIFOWTM(uint8_t *data)
{
	LPS22DF_ReadReg(REG_FIFO_WTM, data);
}

void LPS22DF_SetREFP(void)
{
	//Reference pressure
	//REF_P_L:REFL[7:0]
	//REF_P_H:REFH[15:8]
	uint8_t ref_p[2] = {0};

	ref_p[0] = (uint8_t)(lps22df_settings.ref_p&0x00ff);
	ref_p[1] = lps22df_settings.ref_p>>8;
	LPS22DF_WriteRegMulti(REG_REF_P_L, ref_p, 2);
}

void LPS22DF_GetREFP(uint16_t *data)
{
	//Reference pressure
	//REF_P_L:REFL[7:0]
	//REF_P_H:REFH[15:8]
	uint8_t ref_p[2] = {0};
	
	LPS22DF_ReadRegMulti(REG_REF_P_L, ref_p, 2);
	*data = ((uint16_t)ref_p[1]<<8) | (uint16_t)ref_p[0];
}

void LPS22DF_SetI3CCtrl(void)
{
	//I3C interface Control register
	//b7	b6	b5		b4b3b2	b1b0
	//1 	0 	ASF_ON 	0 0 0	I3C_Bus_Avb_Sel[1:0]
	//ASF_ON: Enables antispike filters. 
	//			Default value: 0
	//			 0: antispike filters are managed by protocol and turned off after the broadcast address;
	//			 1: antispike filters on SCL and SDA lines are always enabled
	//I3C_Bus_Avb_Sel[1:0]: These bits are used to select the bus available time when I3C IBI is used. 
	//							Default value: 00
	//							00: bus available time equal to 50 μsec;
	//							01: bus available time equal to 2 μsec;
	//							10: bus available time equal to 1 msec;
	//							11: bus available time equal to 25 msec
	uint8_t data = *(uint8_t*)&lps22df_settings.i3c_if_ctrl;

	LPS22DF_WriteReg(REG_I3C_IF_CTRL, (data&0xA3));
}

void LPS22DF_GetI3CCtrl(uint8_t *data)
{
	LPS22DF_ReadReg(REG_I3C_IF_CTRL, data);
} 

void LPS22DF_SetPRDS(void)
{
	//Pressure offset
	//RPDS_L:RPDS[7:0]
	//RPDS_H:RPDS[15:8]
	uint8_t offset_p[2] = {0};

	offset_p[0] = (uint8_t)(lps22df_settings.rpds&0x00ff);
	offset_p[1] = (lps22df_settings.rpds>>8);
	LPS22DF_WriteRegMulti(REG_RPDS_L, offset_p, 2);
}

void LPS22DF_GetPRDS(uint16_t *data)
{
	//Pressure offset
	//RPDS_L:RPDS[7:0]
	//RPDS_H:RPDS[15:8]
	uint8_t offset_p[2] = {0};
	
	LPS22DF_ReadRegMulti(REG_RPDS_L, offset_p, 2);
	*data = ((uint16_t)offset_p[1]<<8) | (uint16_t)offset_p[0];
}

void LPS22DF_Reset(void)
{
	uint8_t data = 0xff;

	LPS22DF_WriteReg(REG_CTRL_REG2, 0x04);
	LPS22DF_Delayms(100);
}

void LPS22DF_GetChipID(uint8_t *data)
{
	LPS22DF_ReadReg(REG_WHO_AM_I, data);
}

void LPS22DF_GetINTSource(uint8_t *data)
{
	//Interrupt source (read only) register for differential pressure. 
	//A read at this address clears the INT_SOURCE register itself.
	//b7		b6b5b4b3	b2	b1	b0
	//BOOT_ON 		0 		IA 	PL 	PH
	//BOOT_ON: Indication that the boot (reboot) phase is running.
	//			0: boot phase not running; 
	//			1: boot phase is running
	//IA: Interrupt active.
	//		0: no interrupt has been generated;
	//		1: one or more interrupt events have been generated.
	//PL: Differential pressure low.
	//		0: no interrupt has been generated;
	//		1: low differential pressure event has occurred.
	//PH: Differential pressure high.
	//		0: no interrupt has been generated;
	//		1: high differential pressure event has occurred.
	
	LPS22DF_ReadReg(REG_INT_SOURCE, data);
}

void LPS22DF_GetFIFOStatus1(uint8_t *data)
{
	//FIFO status1 register (read only)
	//b7b6b5b4b3b2b1b0
	//FSS[7:0]
	//FSS[7:0]: FIFO stored data level, number of unread samples stored in FIFO.
	//				00000000: FIFO empty; 
	//				10000000: FIFO full, 128 unread samples
	
	LPS22DF_ReadReg(REG_FIFO_STATUS1, data);
}

void LPS22DF_GetFIFOStatus2(uint8_t *data)
{
	//FIFO status2 register (read only)
	//b7			b6			b5				b4b3b2b1b0
	//FIFO_WTM_IA 	FIFO_OVR_IA FIFO_FULL_IA 		-
	//FIFO_WTM_IA: FIFO threshold (watermark) status. 
	//				Default value: 0
	//					0: FIFO filling is lower than threshold level;
	//					1: FIFO filling is equal or higher than threshold level.
	//FIFO_OVR_IA: FIFO overrun status. Default value: 0
	//					0: FIFO is not completely full;
	//					1: FIFO is full and at least one sample in the FIFO has been overwritten.
	//FIFO_FULL_IA: FIFO full status. Default value: 0
	//					0: FIFO is not completely filled;
	//					1: FIFO is completely filled, no samples overwritten
	
	LPS22DF_ReadReg(REG_FIFO_STATUS2, data);
}

void LPS22DF_GetStatus(uint8_t *data)
{
	//Status register (read only). This register is updated every ODR cycle.
	//b7b6			b5		b4		b3b2	b1		b0
	//  -			T_OR	P_OR	  -		T_DA	P_DA
	//T_OR: Temperature data overrun.
	//			0: no overrun has occurred;
	//			1: a new data for temperature has overwritten the previous data
	//P_OR: Pressure data overrun.
	//			0: no overrun has occurred;
	//			1: new data for pressure has overwritten the previous data
	//T_DA: Temperature data available.
	//			0: new data for temperature is not yet available;
	//			1: new temperature data is generated
	//P_DA: Pressure data available.
	//			0: new data for pressure is not yet available;
	//			1: new pressure data is generated)
	
	LPS22DF_ReadReg(REG_STATUS, data);
}

void LPS22DF_GetTempRawData(int32_t *data)
{
	//Temperature output value (read only)
	//TEMP_OUT_L:TOUT[7:0]
	//TEMP_OUT_H:TOUT[15:8]
	uint8_t status, temp_p[2] = {0};

	if(lps22df_settings.meas_ctrl == MEAS_ONE_SHOT)
	{
		while(1)
		{
			LPS22DF_GetStatus(&status);
			if((status&0x02) == 0x02)
				break;
		}
	}
	
	LPS22DF_ReadRegMulti(REG_TEMP_OUT_L, temp_p, sizeof(temp_p));
	*data = (uint32_t)temp_p[1]<<8 | (uint32_t)temp_p[0];
	LPS22DF_GetTwosComplement(data, 16);
#ifdef PRESSURE_DEBUG
	LOGD("tmp:0x%02X, %d", *data, *data);
#endif	
}

void LPS22DF_GetPrsRawData(int32_t *data)
{
	//Pressure output value (read only)
	//PRESS_OUT_XL:POUT[7:0]
	//PRESS_OUT_L: POUT[15:8]
	//PRESS_OUT_H: POUT[23:16]
	uint8_t status, data_p[3] = {0};

	if(lps22df_settings.meas_ctrl == MEAS_ONE_SHOT)
	{
		while(1)
		{
			LPS22DF_GetStatus(&status);
			if((status&0x01) == 0x01)
				break;
		}
	}
	
	LPS22DF_ReadRegMulti(REG_PRESSURE_OUT_XL, data_p, sizeof(data_p));
	*data = (uint32_t)data_p[2]<<16 | (uint32_t)data_p[1]<<8 | (uint32_t)data_p[0];
	LPS22DF_GetTwosComplement(data, 24);
#ifdef PRESSURE_DEBUG
	LOGD("psr:0x%02X, %d", *data, *data);
#endif		
}

void LPS22DF_CalculateTmp(void)
{
	float Tcomp;

	Tcomp = (float)LPS22DF_tmp/100;
#ifdef PRESSURE_DEBUG
	LOGD("Tcomp:%f", Tcomp);
#endif

	g_tmp = Tcomp;
}

void LPS22DF_CalculatePsr(void)
{
	float Pcomp;

	Pcomp = (float)LPS22DF_prs/4096;	//hPa
	Pcomp *= 100;
#ifdef PRESSURE_DEBUG
	LOGD("Pcomp:%f", Pcomp);
#endif

	pressure_get_ok = true;
	g_prs = Pcomp;
}

void LPS22DF_CalculateFIFOData(void)
{
	//FIFO pressure output (read only)
	//FIFO_DATA_OUT_PRESS_XL:FIFO_P[7:0]
	//FIFO_DATA_OUT_PRESS_L: FIFO_P[15:8]
	//FIFO_DATA_OUT_PRESS_H: FIFO_P[23:16]
	uint8_t i,count=LPS22DF_FIFO_LEVELS,data_p[3*LPS22DF_FIFO_LEVELS] = {0};
	int32_t psr_total = 0;
	
	LPS22DF_ReadRegMulti(REG_FIFO_DATA_OUT_PRESS_XL, data_p, sizeof(data_p));
	if(lps22df_settings.fifo_wtm > 0 && lps22df_settings.fifo_wtm < LPS22DF_FIFO_LEVELS)
		count = lps22df_settings.fifo_wtm;
	for(i=0;i<lps22df_settings.fifo_wtm;i++)
	{
		int32_t psr;
		
		psr = (uint32_t)data_p[3*i+2]<<16 | (uint32_t)data_p[3*i+1]<<8 | (uint32_t)data_p[3*i+0];
		LPS22DF_GetTwosComplement(&psr, 24);
		psr_total += psr;
	}

	LPS22DF_prs = psr_total/lps22df_settings.fifo_wtm;
	LPS22DF_CalculatePsr();
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

bool LPS22DF_Init(void)
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

	LPS22DF_GetChipID(&chip_id);
#ifdef PRESSURE_DEBUG
	LOGD("chip id:0x%02X", chip_id);
#endif
	if(chip_id == LPS22DF_CHIP_ID)
	{
		uint8_t data;

		LPS22DF_Reset();
		//set the threshold value for pressure interrupt event
		LPS22DF_SetThrePrs();
		//set interrupt mode for pressure acquisition configuration
		LPS22DF_SetInterruptCfg();
		//set interrupt active mode
		LPS22DF_SetCtrl3();
		//set fifo ctrl&wtm
		LPS22DF_SetFIFOCtrl();
		LPS22DF_SetFIFOWTM();
		//set reference pressure
		LPS22DF_SetREFP();
		//set pressure offset
		LPS22DF_SetPRDS();
		
		if(lps22df_settings.meas_ctrl == MEAS_CONTINOUS)
		{
			LPS22DF_Start(lps22df_settings.meas_ctrl);
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

void LPS22DF_Start(LPS22DF_MEAS_CTRL meas_type)
{
	switch(meas_type)
	{
	case MEAS_ONE_SHOT:
		lps22df_settings.meas_ctrl = meas_type;
		
		//set ODR
		LPS22DF_SetCtrl1();
		//set fifo ctrl
		LPS22DF_SetFIFOCtrl();
		//set interrupt active event
		LPS22DF_SetCtrl4();
		//start a single measurement of pressure and temperature
		LPS22DF_SetCtrl2();

		LPS22DF_GetTempRawData(&LPS22DF_tmp);
		LPS22DF_CalculateTmp();
			
		LPS22DF_GetPrsRawData(&LPS22DF_prs);
		LPS22DF_CalculatePsr();
		break;
		
	case MEAS_CONTINOUS:
		lps22df_settings.meas_ctrl = meas_type;

		//set ODR
		LPS22DF_SetCtrl1();
		//set fifo ctrl
		LPS22DF_SetFIFOCtrl();
		//set interrupt active event
		LPS22DF_SetCtrl4();
		//start a single measurement of pressure and temperature
		LPS22DF_SetCtrl2();
		break;
	}
}

void LPS22DF_Stop(void)
{
}

void LPS22DFMsgProcess(void)
{
	if(pressure_start_flag)
	{
		LPS22DF_Start(MEAS_ONE_SHOT);
		pressure_start_flag = false;
	}

	if(pressure_stop_flag)
	{
		pressure_stop_flag = false;
	}
	
	if(pressure_interrupt_flag)
	{
		uint8_t fifoctrl,status,fifo_status1,fifo_status2;
		uint8_t data;

		LPS22DF_GetFIFOCtrl(&fifoctrl);
		switch(fifoctrl&0x07)
		{
		case 0b000://Bypass mode
		case 0b100:	
			//In bypass mode, the FIFO is not operational and it remains empty.
			//Switching to bypass mode is also used to reset the FIFO. Passing through bypass mode is mandatory when switching between different FIFO buffer operating modes.
			//As described in the next figure, for each channel only the first address is used. When new data is available, the older data is overwritten.
			LPS22DF_GetStatus(&status);
			if((status&0x02) == 0x02)
			{
			#ifdef PRESSURE_DEBUG
				LOGD("Temperature data available!");
			#endif
				
				LPS22DF_GetTempRawData(&LPS22DF_tmp);
				LPS22DF_CalculateTmp();
			}
			if((status&0x01) == 0x01)
			{
			#ifdef PRESSURE_DEBUG
				LOGD("Pressure data available!");
			#endif
				LPS22DF_GetPrsRawData(&LPS22DF_prs);
				LPS22DF_CalculatePsr();
			}
			break;
			
		case 0b001://FIFO mode
			//In FIFO mode, data from the output PRESS_OUT_XL (28h), PRESS_OUT_L (29h), and PRESS_OUT_H (2Ah) are stored in the FIFO until it is full.
			//To reset FIFO content, in order to select bypass mode the value 000 must be written in FIFO_CTRL (14h). 
			//After this reset command, it is possible to restart FIFO mode by writing the value 001 in FIFO_CTRL (14h).
			//The FIFO buffer memorizes 128 levels of data, but the depth of the FIFO can be resized/reduced by setting the FIFO_CTRL (14h)(STOP_ON_WTM) bit. 
			//If the STOP_ON_WTM bit is set to 1, FIFO depth is limited to FIFO_WTM (15h)(WTM[6:0]) data.
			LPS22DF_GetFIFOStatus2(&fifo_status2);
		#ifdef PRESSURE_DEBUG
			LOGD("fifo_status2:0x%02X", fifo_status2);
		#endif	
			if((fifo_status2&0x80) == 0x80)
			{
				LPS22DF_GetFIFOStatus1(&fifo_status1);
			#ifdef PRESSURE_DEBUG
				LOGD("fifo_status1:0x%02X", fifo_status1);
			#endif
				LPS22DF_CalculateFIFOData();
			}

			data = *(uint8_t*)&lps22df_settings.fifo_ctrl;
			//reset FIFO content
			LPS22DF_WriteReg(REG_FIFO_CTRL, data&0xF8);
			//restart FIFO mode
			LPS22DF_WriteReg(REG_FIFO_CTRL, data);
			break;
			
		case 0b011://Continuous (dynamic-stream) mode
			//In continuous (dynamic-stream) mode, after emptying the FIFO, the first new sample that arrives, becomes the first to be read in a subsequent read burst. 
			//In this way, the number of new data available in FIFO does not depend on the previous read.
			//In continuous (dynamic-stream) mode FIFO_STATUS1 (25h)(FSS[7:0]) is the number of new pressure samples available in the FIFO buffer.
			//Continuous (dynamic-stream) is intended to be used to read FIFO_STATUS1 (25h)(FSS[7:0]) samples when it is not possible to guarantee reading data within 1/ODR time period.
			//Also, a FIFO threshold interrupt on the INT_DRDY pin through CTRL_REG3 (12h)(INT_F_WTM) can be enabled in order to read data from the FIFO and leave free memory slots for incoming data.
			LPS22DF_GetFIFOStatus2(&fifo_status2);
			if((fifo_status2&0x80) == 0x80)
			{
				LPS22DF_CalculateFIFOData();
			}
			break;
			
		case 0b111://Continuous (dynamic-stream)-to-FIFO mode
			//In continuous (dynamic-stream)-to-FIFO mode, data are stored in FIFO and FIFO operates in continuous (dynamic-stream) mode behavior until it switches to FIFO mode behavior when INT_SOURCE (24h)(IA) rises to 1.
			//An interrupt generator has to be set to the desired configuration through INTERRUPT_CFG (0Bh).
			LPS22DF_GetINTSource(&status);
		#ifdef PRESSURE_DEBUG
			LOGD("INT source:0x%02X", status);
		#endif
			if((status&0x04) == 0x04)
			{
			#ifdef PRESSURE_DEBUG
				LOGD("switch to FIFO mode");
			#endif

				data = *(uint8_t*)&lps22df_settings.fifo_ctrl;
				//reset FIFO content
				LPS22DF_WriteReg(REG_FIFO_CTRL, data&0xF8);
				//restart current mode
				LPS22DF_WriteReg(REG_FIFO_CTRL, data);
			}
			else
			{
				//Continuous mode
				LPS22DF_GetFIFOStatus1(&fifo_status1);
			#ifdef PRESSURE_DEBUG
				LOGD("fifo_status1:0x%02X", fifo_status1);
			#endif
				LPS22DF_GetFIFOStatus2(&fifo_status2);
			#ifdef PRESSURE_DEBUG
				LOGD("fifo_status2:0x%02X", fifo_status2);
			#endif	
				if((fifo_status2&0x80) == 0x80)
				{
					LPS22DF_CalculateFIFOData();
				}
			}
			break;
			
		case 0b110://Bypass-to-continuous (dynamic-stream)
			//In bypass-to-continuous (dynamic-stream) mode, FIFO operates in bypass mode until it switches to continuous (dynamic-stream) mode behavior when INT_SOURCE (24h)(IA) rises to 1, then FIFO behavior keeps behaving like in continuous (dynamic-stream) mode.
			//An interrupt generator has to be set to the desired configuration through INTERRUPT_CFG (0Bh).
			LPS22DF_GetINTSource(&status);
		#ifdef PRESSURE_DEBUG
			LOGD("INT source:0x%02X", status);
		#endif
			if((status&0x04) == 0x04)
			{
			#ifdef PRESSURE_DEBUG
				LOGD("switch to continuous mode");
			#endif

				LPS22DF_GetFIFOStatus2(&fifo_status2);
				if((fifo_status2&0x80) == 0x80)
				{
					LPS22DF_CalculateFIFOData();
				}
			}
			else
			{
				//Bypass mode
				LPS22DF_GetStatus(&status);
				if((status&0x02) == 0x02)
				{
				#ifdef PRESSURE_DEBUG
					LOGD("Temperature data available!");
				#endif
					
					LPS22DF_GetTempRawData(&LPS22DF_tmp);
					LPS22DF_CalculateTmp();
				}
				if((status&0x01) == 0x01)
				{
				#ifdef PRESSURE_DEBUG
					LOGD("Pressure data available!");
				#endif
					LPS22DF_GetPrsRawData(&LPS22DF_prs);
					LPS22DF_CalculatePsr();
				}
			}
			break;
			
		case 0b101://Bypass-to-FIFO mode
			//In bypass-to-FIFO mode, FIFO behavior switches when the INT_SOURCE (24h)(IA) bit rises for the first time. 
			//When the INT_SOURCE (24h)(IA) bit is equal to 0, FIFO behaves like in bypass mode. Once the INT_SOURCE (24h)(IA) bit rises to 1, FIFO behavior switches and keeps behaving like in FIFO mode.
			//An interrupt generator has to be set to the desired configuration through INTERRUPT_CFG (0Bh)
			LPS22DF_GetINTSource(&status);
		#ifdef PRESSURE_DEBUG
			LOGD("INT source:0x%02X", status);
		#endif
			if((status&0x04) == 0x04)
			{
			#ifdef PRESSURE_DEBUG
				LOGD("switch to FIFO mode");
			#endif

				LPS22DF_GetFIFOStatus2(&fifo_status2);
				if((fifo_status2&0x80) == 0x80)
				{
					LPS22DF_CalculateFIFOData();
				}

				data = *(uint8_t*)&lps22df_settings.fifo_ctrl;
				//reset FIFO content
				LPS22DF_WriteReg(REG_FIFO_CTRL, data&0xF8);
				//restart FIFO mode
				LPS22DF_WriteReg(REG_FIFO_CTRL, data);
			}
			else
			{
				//Bypass mode
				LPS22DF_GetStatus(&status);
				if((status&0x02) == 0x02)
				{
				#ifdef PRESSURE_DEBUG
					LOGD("Temperature data available!");
				#endif
					
					LPS22DF_GetTempRawData(&LPS22DF_tmp);
					LPS22DF_CalculateTmp();
				}
				if((status&0x01) == 0x01)
				{
				#ifdef PRESSURE_DEBUG
					LOGD("Pressure data available!");
				#endif
					LPS22DF_GetPrsRawData(&LPS22DF_prs);
					LPS22DF_CalculatePsr();
				}
			}
			break;
		}
		
		pressure_interrupt_flag = false;
	}
}

#endif/*PRESSURE_LPS22DF*/
