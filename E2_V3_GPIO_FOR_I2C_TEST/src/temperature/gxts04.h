/****************************************Copyright (c)************************************************
** File Name:			    gxts04.h
** Descriptions:			gxts04 interface process head file
** Created By:				xie biao
** Created Date:			2021-12-24
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __GXTS04_H__
#define __GXTS04_H__

#define I2C1_NODE DT_NODELABEL(i2c1)
#if DT_NODE_HAS_STATUS(I2C1_NODE, okay)
#define TEMP_DEV	DT_LABEL(I2C1_NODE)
#else
/* A build error here means your board does not have I2C enabled. */
#error "i2c1 devicetree node is disabled"
#define TEMP_DEV	""
#endif

#define TEMP_PORT 	"GPIO_0"

#define TEMP_ALRTB		7
#define TEMP_EINT		8
#define TEMP_SCL		31
#define TEMP_SDA		30

#define GXTS04_ID	0x20F0

#define GXTS04_I2C_ADDR		    0x70

#define CMD_SLEEP				0xB098
#define CMD_WAKEUP				0x3517
#define CMD_MEASURE_NORMAL		0x7866
#define CMD_MEASURE_LOW_POWER	0x609C
#define CMD_RESET				0x805D
#define CMD_READ_ID				0xEFC8

extern bool gxts04_init(void);
extern void gxts04_start(void);
extern void gxts04_stop(void);
extern bool GetTemperature(float *skin_temp, float *body_temp);

#endif/*__GXTS04_H__*/