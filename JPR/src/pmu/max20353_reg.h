#ifndef __MAX20353_REG_H__
#define __MAX20353_REG_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include <stdint.h>
#include <math.h>

extern struct device *max20353_I2C;

#define PMU_DEV "I2C_2"
#define PMU_PORT "GPIO_0"

#define PMU_ALRTB		7
#define PMU_EINT		8
#define PMU_SCL			31
#define PMU_SDA			30

#define MAX20353_HARDWARE_ID	0x03
#define MAX20353_FIRMWARE_ID	0x02

#define MAX20353_I2C_ADDR		    (0x50 >> 1)
#define MAX20353_I2C_WR_ADDR		((MAX20353_I2C_ADDR << 1))
#define MAX20353_I2C_RD_ADDR		((MAX20353_I2C_ADDR << 1) | 1)

#define MAX20353_NO_ERROR   0
#define MAX20353_ERROR      -1

#define MAX20353_I2C_ADDR_FUEL_GAUGE    0x6C

#define MAX20353_LDO_MIN_MV 	800
#define MAX20353_LDO_MAX_MV 	3600
#define MAX20353_LDO_STEP_MV 	100

#define MAX20353_OFF_COMMAND 	0xB2

typedef enum
{
	REG_HARDWARE_ID		= 0x00,		///< HardwareID Register
	REG_FIRMWARE_REV	= 0x01,		///< FirmwareID Register
	//					= 0x02,		///<
	REG_INT0			= 0x03,		///< Int0 Register
	REG_INT1			= 0x04,		///< Int1 Register
	REG_INT2			= 0x05,		///< Int2 Register
	REG_STATUS0			= 0x06,		///< Status Register 0
	REG_STATUS1			= 0x07,		///< Status Register 1
	REG_STATUS2			= 0x08,		///< Status Register 2
	REG_STATUS3			= 0x09,		///< Status Register 2
	//					= 0x0A,		///<
	REG_SYSTEM_ERROR	= 0x0B,		///< SystemError Register
	REG_INT_MASK0		= 0x0C,		///< IntMask0 Register
	REG_INT_MASK1		= 0x0D,		///< IntMask1 Register
	REG_INT_MASK2		= 0x0E,		///< IntMask1 Register
	REG_AP_DATOUT0		= 0x0F,     ///< APDataOut0 Register
	REG_AP_DATOUT1		= 0x10,     ///< APDataOut1 Register
	REG_AP_DATOUT2		= 0x11,     ///< APDataOut2 Register
	REG_AP_DATOUT3		= 0x12,     ///< APDataOut3 Register
	REG_AP_DATOUT4		= 0x13,     ///< APDataOut4 Register
	REG_AP_DATOUT5		= 0x14,     ///< APDataOut5 Register
	REG_AP_DATOUT6		= 0x15,     ///< APDataOut6 Register
	REG_AP_CMDOUT		= 0x17,     ///< APCmdOut Register
	REG_AP_RESPONSE		= 0x18,     ///< APResponse Register
	REG_AP_DATAIN0		= 0x19,
	REG_AP_DATAIN1		= 0x1A,
	REG_AP_DATAIN2		= 0x1B,
	REG_AP_DATAIN3		= 0x1C,
	REG_AP_DATAIN4		= 0x1D,
	REG_AP_DATAIN5		= 0x1E,
	//					= 0x1F,		///<
	REG_LDO_DIRECT		= 0x20,
	REG_MPC_DIRECTWRITE	= 0x21,
	REG_MPC_DIRECTRED	= 0x22,

	REG_HPT_RAMADDR		= 0x28,
	REG_HPT_RAMDATAH	= 0x29,
	REG_HPT_RAMDATAM	= 0x2A,
	REG_HPT_RAMDATAL	= 0x2B,

	REG_LED_STEP_DIRECT	= 0x2C,
	REG_LED0_DIRECT		= 0x2D,
	REG_LED1_DIRECT		= 0x2E,
	REG_LED2_DIRECT		= 0x2F,

	REG_HPT_DIRECT0		= 0x30,
	REG_HPT_DIRECT1		= 0x31,
	REG_HPT_RTI2CAMP	= 0x32,
	REG_HPT_PATRAMADDR  = 0x33,

	REG_LDO1_CONFIG_WRITE = 0x40,
	REG_LDO1_CONFIG_READ  = 0x41,
	REG_LDO2_CONFIG_WRITE = 0x42,
	REG_LDO2_CONFIG_READ  = 0x43
}max20353_reg_t;

/** @addtogroup  Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  *
**/
typedef int32_t (*maxdev_write_ptr)(struct device *handle, u8_t reg, u8_t *bufp, u16_t len);
typedef int32_t (*maxdev_read_ptr)(struct device *handle, u8_t reg, u8_t *bufp, u16_t len);

typedef struct {
  /** Component mandatory fields **/
  maxdev_write_ptr  write_reg;
  maxdev_read_ptr   read_reg;
  /** Customizable optional pointer **/
  struct device *handle;
} maxdev_ctx_t;


int MAX20353_AppWrite(uint8_t dataoutlen);
int MAX20353_WriteRegMulti(max20353_reg_t reg, uint8_t *value, uint8_t len);
int MAX20353_WriteReg(max20353_reg_t reg, uint8_t value);
int MAX20353_ReadReg(max20353_reg_t reg, uint8_t *value);
int MAX20353_ReadRegMulti(max20353_reg_t reg, uint8_t *value, uint8_t len);

int MAX20353_BoostConfig(void);
int MAX20353_ChargePumpConfig(void);
int MAX20353_BuckBoostConfig(void);
int MAX20353_HapticConfig(void);
int MAX20353_InitRAM(void);

int MAX20353_RAMPatStart(unsigned char RAMPatAddr);
int MAX20353_Pattern1(int Mode);
int MAX20353_Pattern2(int Mode);
int MAX20353_DirectWrite(int Amplitude, int time); 

extern int MAX20353_LED1(int IStep, int Amplitude, bool flag);
extern void MAX20353_Init(void);

#ifdef __cplusplus
}
#endif

#endif/*__MAX20353_REG_H__*/
