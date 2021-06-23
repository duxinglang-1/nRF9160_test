/****************************************Copyright (c)************************************************
** File Name:			    max_sh_interface.h
** Descriptions:			max sensor hub interface head file
** Created By:				xie biao
** Created Date:			2021-06-21
** Modified Date:      		2021-06-21 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __MAX_SH_INTERFACE_H__
#define __MAX_SH_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>

#define BL_AES_NONCE_SIZE     11
#define BL_AES_AUTH_SIZE      16
#define BL_MAX_PAGE_SIZE      8192
#define BL_FLASH_CMD_LEN      2
#define BL_CHECK_BYTE_LEN     16
#define BL_PAGE_W_DLY_TIME    700
#define BL_ERASE_DELAY	      2000

#define BL_PAGE_COUNT_INDEX   0x44
#define BL_IV_INDEX           0x28
#define BL_AUTH_INDEX         0x34
#define BL_ST_PAGE_IDEX       0x4C

#define SS_DUMP_REG_SLEEP_MS        (100)
#define SS_ENABLE_SENSOR_SLEEP_MS   (20)
#define SS_DEFAULT_CMD_SLEEP_MS     (2)
#define SS_WAIT_BETWEEN_TRIES_MS    (2)
#define SS_CMD_WAIT_PULLTRANS_MS    (5)
#define SS_FEEDFIFO_CMD_SLEEP_MS	(30)

// Sensor/Algo indicies
#define SH_SENSORIDX_MAX8614X	0x00
#define SH_SENSORIDX_MAX30205	0x01
#define SH_SENSORIDX_MAX30001	0x02
#define SH_SENSORIDX_MAX30101	0x03
#define SH_SENSORIDX_ACCEL	    0x04
#define SH_NUM_CURRENT_SENSORS	5

#define SH_ALGOIDX_AGC	        	0x00
#define SH_ALGOIDX_AEC				0x01
#define SH_ALGOIDX_WHRM				0x02
#define SH_ALGOIDX_ECG				0x03
#define SH_ALGOIDX_BPT				0x04
#define SH_ALGOIDX_WSPO2        	0x05
#define SS_ALGOIDX_WHRM_WSPO2_SUITE 0x07

#define SH_NUM_CURRENT_ALGOS	6

#define PADDING_BYTE            (0xEE)
#define DATA_BYTE               (0xED)


#define SH_INPUT_DATA_DIRECT_SENSOR	0x00
#define SH_INPUT_DATA_FROM_HOST		0x01

#define SS_FAM_R_STATUS		0x00
	#define SS_CMDIDX_STATUS	0x00
		#define SS_SHIFT_STATUS_ERR				0
		#define SS_MASK_STATUS_ERR				(0x07 << SS_SHIFT_STATUS_ERR)
		#define SS_SHIFT_STATUS_DATA_RDY		3
		#define SS_MASK_STATUS_DATA_RDY			(1 << SS_SHIFT_STATUS_DATA_RDY)
		#define SS_SHIFT_STATUS_FIFO_OUT_OVR	4
		#define SS_MASK_STATUS_FIFO_OUT_OVR		(1 << SS_SHIFT_STATUS_FIFO_OUT_OVR)
		#define SS_SHIFT_STATUS_FIFO_IN_OVR		5
		#define SS_MASK_STATUS_FIFO_IN_OVR		(1 << SS_SHIFT_STATUS_FIFO_IN_OVR)

		#define SS_SHIFT_STATUS_LOG_OVR			6
		#define SS_MASK_STATUS_LOG_OVR			(1 << SS_SHIFT_STATUS_LOG_OVR)

		#define SS_SHIFT_STATUS_LOG_RDY			7
		#define SS_MASK_STATUS_LOG_RDY			(1 << SS_SHIFT_STATUS_LOG_RDY)



#define SS_FAM_W_MODE	0x01
#define SS_FAM_R_MODE	0x02
	#define SS_CMDIDX_MODE	0x00
		#define SS_SHIFT_MODE_SHDN		0
		#define SS_MASK_MODE_SHDN		(1 << SS_SHIFT_MODE_SHDN)
		#define SS_SHIFT_MODE_RESET		1
		#define SS_MASK_MODE_RESET		(1 << SS_SHIFT_MODE_RESET)
		#define SS_SHIFT_MODE_FIFORESET	2
		#define SS_MASK_MODE_FIFORESET	(1 << SS_SHIFT_MODE_FIFORESET)
		#define SS_SHIFT_MODE_BOOTLDR	3
		#define SS_MASK_MODE_BOOTLDR	(1 << SS_SHIFT_MODE_BOOTLDR)

/*MYG*/
#define SH_MODE_REQUEST_RET_BYTES        (2)
#define SH_MODE_REQUEST_DELAY            (2)
#define SH_STATUS_REQUEST_RET_BYTES      (2)
#define SH_STATUS_REQUEST_DELAY          (2)



#define SS_I2C_READ		0x03

#define SS_FAM_W_COMMCHAN	0x10
#define SS_FAM_R_COMMCHAN	0x11
	#define SS_CMDIDX_OUTPUTMODE	0x00
		#define SS_SHIFT_OUTPUTMODE_DATATYPE	0
		#define SS_MASK_OUTPUTMODE_DATATYPE		(0x03 << SS_SHIFT_OUTPUTMODE_DATATYPE)
			#define SS_DATATYPE_PAUSE				0
			#define SS_DATATYPE_RAW					1
			#define SS_DATATYPE_ALGO				2
			#define SS_DATATYPE_BOTH				3
		#define SS_SHIFT_OUTPUTMODE_SC_EN		2
		#define SS_MASK_OUTPUTMODE_SC_EN		(1 << SS_SHIFT_OUTPUTMODE_SC_EN)
	#define SS_CMDIDX_FIFOAFULL		0x01
    #define SS_CMDIDX_REPORTPERIOD	0x02

#define SS_FAM_R_OUTPUTFIFO	0x12
	#define SS_CMDIDX_OUT_NUMSAMPLES	0x00
	#define SS_CMDIDX_READFIFO		    0x01

#define SS_FAM_R_INPUTFIFO						0x13
	#define SS_CMDIDX_SAMPLE_SIZE				0x00
	#define SS_CMDIDX_INPUT_FIFO_SIZE			0x01
	#define SS_CMDIDX_SENSOR_FIFO_SIZE			0x02
	#define SS_CMDIDX_NUM_SAMPLES_SENSOR_FIFO	0x03
	#define SS_CMDIDX_NUM_SAMPLES_INPUT_FIFO	0x04

#define SS_FAM_W_INPUTFIFO	0x14
	#define SS_CMDIDN_WRITEFIFO		0x00
	#define SS_CMDIDX_WRITE_FIFO    0x00

#define SS_FAM_W_WRITEREG		0x40
#define SS_FAM_R_READREG		0x41
#define SS_FAM_R_REGATTRIBS		0x42
#define SS_FAM_R_DUMPREG		0x43

#define SS_FAM_W_SENSORMODE		0x44
#define SS_FAM_R_SENSORMODE		0x45
#define SS_FAM_W_SENSOR_CONFIG	0x46
	#define SS_CFGIDX_OPERATING_CONFIG	0x00


//TODO: Fill in known configuration parameters
#define SS_FAM_W_ALGOCONFIG	0x50
#define SS_FAM_R_ALGOCONFIG	0x51
// config for WHRM+WSPO2 ALGO SUITE
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_SPO2_CAL		                0x00
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_WSPO2_MOTION_PERIOD          0x01
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_WSPO2_MOTION_THRESHOLD       0x02
	#define SS_CFGIDX_WHRM_WSPO2_SUITE_WSPO2_AFE_TIMEOUT       		0x03
	#define SS_CFGIDX_WHRM_WSPO2_SUITE_WSPO2_TIMEOUT       			0x04
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_INITIAL_HR       			0x05
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_PERSON_HEIGHT       			0x06
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_PERSON_WEIGHT       			0x07
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_PERSON_AGE       			0x08
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_PERSON_GENDER       			0x09
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_ALGO_MODE        	        0x0A
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_AEC_ENABLE           	    0x0B
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_SCD_ENABLE           	    0x0C
	#define SS_CFGIDX_WHRM_WSPO2_SUITE_TARGET_PD_CURRENT_PERIOD     0x0D
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_MOTION_MAG_THRESHOLD         0x0E
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_MIN_PD_CURRENT               0x0F
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_INIT_PD_CURRENT              0x10
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_TARGET_PD_CURRENT            0x11
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_AUTO_PD_CURRENT_ENABLE       0x12
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_MIN_INTEGRATION_TIME         0x13
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_MIN_SAMPLING_AVERAGE         0x14
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_MAX_INTEGRATION_TIME         0x15
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_MAX_SAMPLING_AVERAGE         0x16
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_WHRMLEDPDCONFIGURATION       0x17
    #define SS_CFGIDX_WHRM_WSPO2_SUITE_SPO2LEDPDCONFIGURATION       0x18


#define SS_FAM_W_ALGOMODE	0x52
#define SS_FAM_R_ALGOMODE	0x53

#define SS_FAM_W_EXTERNSENSORMODE	0x60
#define SS_FAM_R_EXTERNSENSORMODE	0x61

#define SS_FAM_R_SELFTEST    0x70

#define SS_FAM_W_BOOTLOADER	0x80
	#define SS_CMDIDX_SETIV			0x00
	#define SS_CMDIDX_SETAUTH		0x01
	#define SS_CMDIDX_SETNUMPAGES	0x02
	#define SS_CMDIDX_ERASE			0x03
	#define SS_CMDIDX_SENDPAGE		0x04
	#define SS_CMDIDX_ERASE_PAGE	0x05
#define SS_FAM_R_BOOTLOADER	0x81
	#define SS_CMDIDX_BOOTFWVERSION	0x00
	#define SS_CMDIDX_PAGESIZE		0x01

#define SS_FAM_W_BOOTLOADER_CFG	0x82
#define SS_FAM_R_BOOTLOADER_CFG	0x83
	#define SS_CMDIDX_BL_SAVE		0x00
	#define SS_CMDIDX_BL_ENTRY		0x01
		#define SS_BL_CFG_ENTER_BL_MODE		0x00
		#define SS_BL_CFG_EBL_PIN			0x01
		#define SS_BL_CFG_EBL_POL			0x02
	#define SS_CMDIDX_BL_EXIT		0x02
		#define SS_BL_CFG_EXIT_BL_MODE		0x00
		#define SS_BL_CFG_TIMEOUT			0x01

/* Enable logging/debugging */
#define SS_FAM_R_LOG				0x90
	#define SS_CMDIDX_R_LOG_DATA	0x00
	#define SS_CMDIDX_R_LOG_LEN		0x01

	#define SS_CMDIDX_R_LOG_LEVEL	0x02
		#define SS_LOG_DISABLE		0x00
		#define SS_LOG_CRITICAL		0x01
		#define SS_LOG_ERROR		0x02
		#define SS_LOG_INFO			0x04
		#define SS_LOG_DEBUG		0x08

#define SS_FAM_W_LOG_CFG			0x91
	#define SS_CMDIDX_LOG_GET_LEVEL	0x00
	#define SS_CMDIDX_LOG_SET_LEVEL	0x01

#define SS_FAM_R_IDENTITY			0xFF
	#define SS_CMDIDX_PLATTYPE		0x00
	#define SS_CMDIDX_PARTID		0x01
	#define SS_CMDIDX_REVID			0x02
	#define SS_CMDIDX_FWVERSION		0x03
	#define SS_CMDIDX_AVAILSENSORS	0x04
	#define SS_CMDIDX_DRIVERVER		0x05
	#define SS_CMDIDX_AVAILALGOS	0x06
	#define SS_CMDIDX_ALGOVER		0x07

#define SS_FAM_R_AUTHSEQUENCE       0xB2
#define SS_FAM_R_INITPARAMS         0xB3
#define SS_FAM_W_DHPUBLICKEY        0xB4
#define SS_FAM_R_DHPUBLICKEY        0xB5


/* Newly added ones; checko for collosion or repeats with the ones above */
#define SS_RESET_TIME	10
#define SS_STARTUP_TO_BTLDR_TIME	20
#define SS_STARTUP_TO_MAIN_APP_TIME	1000

#define SS_MAX_SUPPORTED_SENSOR_NUM	0xFE
#define SS_MAX_SUPPORTED_ALGO_NUM	0xFE

#define SS_APPPLICATION_MODE   0x00
#define SS_BOOTLOADER_MODE     0x08

#define CONFIG_ENABLE   (0x01)
#define CONFIG_DISABLE  (0x00)

#define SH_OTA_DATA_STORE_IN_FLASH
#define WAIT_MS wait_ms

typedef enum{
	SS_SUCCESS             =0x00,
	SS_ERR_COMMAND         =0x01,
	SS_ERR_UNAVAILABLE     =0x02,
	SS_ERR_DATA_FORMAT     =0x03,
	SS_ERR_INPUT_VALUE     =0x04,
	SS_ERR_BTLDR_GENERAL   =0x80,
	SS_ERR_BTLDR_CHECKSUM  =0x81,
	SS_ERR_TRY_AGAIN       =0xFE,
	SS_ERR_UNKNOWN         =0xFF,
}SS_STATUS;


/**
* @brief	Func to write to sensor hub via sending generic command byte sequences
*
* @param[in]	tx_buf   - command byte sequence
* @param[in]	tx_len   - command byte sequence length in bytes
* @param[in]	sleep_ms - time to wait for sensor hub to report statuss
*
* @return 1 byte status: 0x00 (SS_SUCCESS) on success
*/
int sh_write_cmd(uint8_t *tx_buf,
		         int tx_len,
				 int sleep_ms);


/**
* @brief	Func to write to sensor hub via sending generic command byte sequences and data bytes
*
* @param[in]	cmd_bytes      - command byte sequence
* @param[in]	cmd_bytes_len  - command byte sequence length in bytes
* @param[in]    data           - data byte array to be sent following cmd bytes
* @param[in]    data_len       - data array size in bytes
* @param[in]    cmd_delay_ms   - time to wait for sensor hub to report status
*
* @return 1 byte status: 0x00 (SS_SUCCESS) on success
*/
int sh_write_cmd_with_data(uint8_t *cmd_bytes,
		                   int cmd_bytes_len,
                           uint8_t *data,
						   int data_len,
                           int cmd_delay_ms);


/**
* @brief	Func to read from sensor hub via sending generic command byte sequences
*
* @param[in]	cmd_bytes      - command byte sequence
* @param[in]	cmd_bytes_len  - command byte sequence length in bytes
* @param[in]    data           - data byte array to be sent following cmd bytes
* @param[in]    data_len       - data array size in bytes
* @param[out]   rxbuf          - byte buffer to store incoming data (including status byte)
* @param[in]    rxbuf_sz       - incoming data buffer size in bytes ( to prevent overflow)
* @param[in]    cmd_delay_ms   - time to wait for sensor hub to report status
*
* @return 1 byte status: 0x00 (SS_SUCCESS) on success
*/
int sh_read_cmd(uint8_t *cmd_bytes,
		        int cmd_bytes_len,
	            uint8_t *data,
				int data_len,
	            uint8_t *rxbuf,
				int rxbuf_sz,
                int sleep_ms);

void wait_ms(int ms);
void SH_rst_to_BL_mode(void);
void SH_rst_to_APP_mode(void);

void sh_start_hub_event_poll( int pollPeriod_ms);
void stop_hub_event_poll(void);
void sh_clear_mfio_event_flag(void);
bool sh_has_mfio_event(void);

extern bool sh_init_interface(void);
extern void sh_get_BL_version(void);
extern void sh_get_APP_version(void);

#endif/*__MAX_SH_INTERFACE_H__*/
