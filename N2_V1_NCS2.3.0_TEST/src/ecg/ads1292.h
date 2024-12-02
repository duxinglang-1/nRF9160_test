/****************************************Copyright (c)************************************************
** File Name:			    ads1292.h
** Descriptions:			Sensor head file for ADS1292
** Created By:				xie biao
** Created Date:			2024-04-11
** Modified Date:      		2024-04-11
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __ADS1292_H__
#define __ADS1292_H__

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

//SPIÒý½Å¶¨Òå
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi3), okay)
#define ECG_DEVICE DT_NODELABEL(spi3)
#else
//#error "spi2 devicetree node is disabled"
#define ECG_DEVICE	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define ECG_PORT DT_NODELABEL(gpio0)
#else
#error "gpio1 devicetree node is disabled"
#define ECG_PORT	""
#endif
 
#define ECG_CS_PIN			(11)
#define ECG_CLK_PIN			(12)
#define ECG_MOSI_PIN		(13)
#define ECG_MISO_PIN		(14)
#define ECG_DRDY_PIN		(15)
#define ECG_RESET_PIN		(2)
#define ECG_START_PIN		(3)

#define	SPI_TXRX_MAX_LEN	(1024*4)

typedef enum
{
	ADS_SYS_COM_WAKEUP 		= 0x02,
	ADS_SYS_COM_STANDBY 	= 0x04,
	ADS_SYS_COM_RESET		= 0x06,
	ADS_SYS_COM_START		= 0x08,
	ADS_SYS_COM_STOP		= 0x0A,
	ADS_SYS_COM_OFFSETCAL	= 0x1A,
	ADS_SYS_COM_MAX			= 0xFF
}ADS_SYS_COMMAND;

typedef enum
{
	ADS_DATA_COM_RDATAC		= 0x10,
	ADS_DATA_COM_SDATAC		= 0x11,
	ADS_DATA_COM_RDATA		= 0x12,
	ADS_DATA_COM_MAX		= 0xFF
}ADS_DATA_COMMAND;

typedef enum
{
	ADS_REG_COM_RREG		= 0x20,
	ADS_RED_COM_WREG		= 0x40,
	ADS_RED_COM_MAX			= 0xFF
}ADS_REG_COMMAND;

typedef enum
{
	ADS_REG_ID				= 0x00,
	ADS_REG_CFG1			= 0x01,
	ADS_REG_CFG2			= 0x02,
	ADS_REG_LOFF			= 0x03,
	ADS_REG_CH1SET			= 0x04,
	ADS_REG_CH2SET			= 0x05,
	ADS_REG_RLD_SENS		= 0x06,
	ADS_REG_LOFF_SENS		= 0x07,
	ADS_REG_LOFF_STAT		= 0x08,
	ADS_REG_RESP1			= 0x09,
	ADS_REG_RESP2			= 0x0A,
	ADS_REG_GPIO			= 0x0B,
	ADS_REG_MAX
}ADS_RED_ADDR;

typedef enum
{
	ADS1191_16BIT	 		= 0x00,
	ADS1192_16BIT			= 0x01,
	ADS1291_24BIT			= 0x10,
	ADS1292_24BIT			= 0x11,
	ADS_SENSOR_MAX
}ADS_SENSOR_TYPE;

typedef struct
{
	uint8_t state;
	uint8_t SamplingRate;
	uint8_t command;
}ADS1x9x_state_t;

typedef enum
{
	ECG_STATE_IDLE,
	ECG_STATE_DATA_STREAMING,
	ECG_STATE_ACQUIRE_DATA,
	ECG_STATE_DOWNLOAD,
	ECG_STATE_RECORDING,
	ECG_STATE_DATA_LOGGER,
	ECG_STATE_MAX
}ECG_RECORDER_STATE;

extern ADS1x9x_state_t ECG_Recoder_state;
extern void ADS1x9x_Init(void);
extern void ADS1x9x_Msg_Process(void);

#endif/*__ADS1292_H__*/
