/****************************************Copyright (c)************************************************
** File name:			external_flash.h
** Last modified Date:          
** Last Version:		V1.0   
** Created by:			谢彪
** Created date:		2020-06-30
** Version:			    1.0
** Descriptions:		外挂flash驱动头文件
******************************************************************************************************/
#ifndef __EXTERNAL_FLASH_H__
#define __EXTERNAL_FLASH_H__

#include <stdint.h>
#include "font.h"

//SPI引脚定义
#define FLASH_DEVICE 	"SPI_3"
#define FLASH_NAME 		"W25Q64FW"
#define FLASH_PORT		"GPIO_0"
#define FLASH_CS_PIN		(2)
#define FLASH_CLK_PIN		(22)
#define FLASH_MOSI_PIN		(20)
#define FLASH_MISO_PIN		(5)

//W25Q64 ID
#define	W25Q64_ID	0XEF16

//SPI Flash命令定义
#define	SPIFlash_WriteEnable	0x06  //写使能命令
#define	SPIFlash_WriteDisable	0x04  //写禁止命令
#define	SPIFlash_PageProgram	0x02  //页写入命令
#define	SPIFlash_ReadStatusReg	0x05  //读状态寄存器1
#define	SPIFlash_WriteStatusReg	0x01  //写状态寄存器1
#define	SPIFlash_ReadData		0x03  //读数据命令
#define	SPIFlash_SecErase		0x20  //扇区擦除
#define	SPIFlash_BlockErase		0xD8  //块擦除
#define	SPIFlash_ChipErase		0xC7  //全片擦除
#define	SPIFlash_ReadID			0x90  //读取ID

#define	SPIFLASH_CMD_LENGTH		0x04
#define	SPIFLASH_WRITE_BUSYBIT	0x01

#define	SPIFlash_PAGE_SIZE		256
#define SPIFlash_PAGE_NUMBER	32768

#define	SPIFlash_SECTOR_SIZE	4096
#define SPIFlash_SECTOR_NUMBER	2048

#define	SPI_TXRX_MAX_LEN		(1024*4)	//255	(1024*2)

#define SPIFlash_BLOCK_SIZE		64
#define	SPIFlash_BLOCK_NUMBLE	128


/***************************************************img start*********************************************************/
//IMG在flash里占用4M的空间(0x000000~0x3FFFFF)
#define IMG_START_ADDR						0x000000
//img ver
#define IMG_VER_ADDR						IMG_START_ADDR
#define IMG_VER_SIZE						(16)
#define IMG_VER_END							(IMG_VER_ADDR+IMG_VER_SIZE)
//img data
#define IMG_DATA_ADDR						IMG_VER_END
//analog clock(6)
#define IMG_ANALOG_CLOCK_BG_ADDR			(IMG_DATA_ADDR)
#define IMG_ANALOG_CLOCK_BG_SIZE			(115208)
#define IMG_ANALOG_CLOCK_BG_END				(IMG_ANALOG_CLOCK_BG_ADDR+IMG_ANALOG_CLOCK_BG_SIZE)
#define IMG_ANALOG_CLOCK_DOT_WHITE_ADDR		(IMG_ANALOG_CLOCK_BG_END)
#define IMG_ANALOG_CLOCK_DOT_WHITE_SIZE		(458)
#define IMG_ANALOG_CLOCK_DOT_WHITE_END		(IMG_ANALOG_CLOCK_DOT_WHITE_ADDR+IMG_ANALOG_CLOCK_DOT_WHITE_SIZE)
#define IMG_ANALOG_CLOCK_DOT_RED_ADDR		(IMG_ANALOG_CLOCK_DOT_WHITE_END)
#define IMG_ANALOG_CLOCK_DOT_RED_SIZE		(250)
#define IMG_ANALOG_CLOCK_DOT_RED_END		(IMG_ANALOG_CLOCK_DOT_RED_ADDR+IMG_ANALOG_CLOCK_DOT_RED_SIZE)
#define IMG_ANALOG_CLOCK_HAND_SEC_ADDR		(IMG_ANALOG_CLOCK_DOT_RED_END)
#define IMG_ANALOG_CLOCK_HAND_SEC_SIZE		(1652)
#define IMG_ANALOG_CLOCK_HAND_SEC_END		(IMG_ANALOG_CLOCK_HAND_SEC_ADDR+IMG_ANALOG_CLOCK_HAND_SEC_SIZE)
#define IMG_ANALOG_CLOCK_HAND_MIN_ADDR		(IMG_ANALOG_CLOCK_HAND_SEC_END)
#define IMG_ANALOG_CLOCK_HAND_MIN_SIZE		(2864)
#define IMG_ANALOG_CLOCK_HAND_MIN_END		(IMG_ANALOG_CLOCK_HAND_MIN_ADDR+IMG_ANALOG_CLOCK_HAND_MIN_SIZE)
#define IMG_ANALOG_CLOCK_HAND_HOUR_ADDR		(IMG_ANALOG_CLOCK_HAND_MIN_END)
#define IMG_ANALOG_CLOCK_HAND_HOUR_SIZE		(2080)
#define IMG_ANALOG_CLOCK_HAND_HOUR_END		(IMG_ANALOG_CLOCK_HAND_HOUR_ADDR+IMG_ANALOG_CLOCK_HAND_HOUR_SIZE)
//battery(9)
#define IMG_BAT_CHRING_ANI_1_ADDR			(IMG_ANALOG_CLOCK_HAND_HOUR_END)
#define IMG_BAT_CHRING_ANI_1_SIZE			(23708)
#define IMG_BAT_CHRING_ANI_1_END			(IMG_BAT_CHRING_ANI_1_ADDR+IMG_BAT_CHRING_ANI_1_SIZE)
#define IMG_BAT_CHRING_ANI_2_ADDR			(IMG_BAT_CHRING_ANI_1_END)
#define IMG_BAT_CHRING_ANI_2_SIZE			(23708)
#define IMG_BAT_CHRING_ANI_2_END			(IMG_BAT_CHRING_ANI_2_ADDR+IMG_BAT_CHRING_ANI_2_SIZE)
#define IMG_BAT_CHRING_ANI_3_ADDR			(IMG_BAT_CHRING_ANI_2_END)
#define IMG_BAT_CHRING_ANI_3_SIZE			(23708)
#define IMG_BAT_CHRING_ANI_3_END			(IMG_BAT_CHRING_ANI_3_ADDR+IMG_BAT_CHRING_ANI_3_SIZE)
#define IMG_BAT_CHRING_ANI_4_ADDR			(IMG_BAT_CHRING_ANI_3_END)
#define IMG_BAT_CHRING_ANI_4_SIZE			(23708)
#define IMG_BAT_CHRING_ANI_4_END			(IMG_BAT_CHRING_ANI_4_ADDR+IMG_BAT_CHRING_ANI_4_SIZE)
#define IMG_BAT_CHRING_ANI_5_ADDR			(IMG_BAT_CHRING_ANI_4_END)
#define IMG_BAT_CHRING_ANI_5_SIZE			(23708)
#define IMG_BAT_CHRING_ANI_5_END			(IMG_BAT_CHRING_ANI_5_ADDR+IMG_BAT_CHRING_ANI_5_SIZE)
#define IMG_BAT_FULL_ICON_ADDR				(IMG_BAT_CHRING_ANI_5_END)
#define IMG_BAT_FULL_ICON_SIZE				(115208)
#define IMG_BAT_FULL_ICON_END				(IMG_BAT_FULL_ICON_ADDR+IMG_BAT_FULL_ICON_SIZE)
#define IMG_BAT_LOW_ICON_ADDR				(IMG_BAT_FULL_ICON_END)
#define IMG_BAT_LOW_ICON_SIZE				(23708)
#define IMG_BAT_LOW_ICON_END				(IMG_BAT_LOW_ICON_ADDR+IMG_BAT_LOW_ICON_SIZE)
#define IMG_BAT_RECT_RED_ADDR				(IMG_BAT_LOW_ICON_END)
#define IMG_BAT_RECT_RED_SIZE				(968)
#define IMG_BAT_RECT_RED_END				(IMG_BAT_RECT_RED_ADDR+IMG_BAT_RECT_RED_SIZE)
#define IMG_BAT_RECT_WHITE_ADDR				(IMG_BAT_RECT_RED_END)
#define IMG_BAT_RECT_WHITE_SIZE				(968)
#define IMG_BAT_RECT_WHITE_END				(IMG_BAT_RECT_WHITE_ADDR+IMG_BAT_RECT_WHITE_SIZE)
//back light(6)
#define IMG_BKL_DEC_ICON_ADDR				(IMG_BAT_RECT_WHITE_END)
#define IMG_BKL_DEC_ICON_SIZE				(12878)
#define IMG_BKL_DEC_ICON_END				(IMG_BKL_DEC_ICON_ADDR+IMG_BKL_DEC_ICON_SIZE)
#define IMG_BKL_INC_ICON_ADDR				(IMG_BKL_DEC_ICON_END)
#define IMG_BKL_INC_ICON_SIZE				(12878)
#define IMG_BKL_INC_ICON_END				(IMG_BKL_INC_ICON_ADDR+IMG_BKL_INC_ICON_SIZE)
#define IMG_BKL_LEVEL_1_ADDR				(IMG_BKL_INC_ICON_END)
#define IMG_BKL_LEVEL_1_SIZE				(35648)
#define IMG_BKL_LEVEL_1_END					(IMG_BKL_LEVEL_1_ADDR+IMG_BKL_LEVEL_1_SIZE)
#define IMG_BKL_LEVEL_2_ADDR				(IMG_BKL_LEVEL_1_END)
#define IMG_BKL_LEVEL_2_SIZE				(35648)
#define IMG_BKL_LEVEL_2_END					(IMG_BKL_LEVEL_2_ADDR+IMG_BKL_LEVEL_2_SIZE)
#define IMG_BKL_LEVEL_3_ADDR				(IMG_BKL_LEVEL_2_END)
#define IMG_BKL_LEVEL_3_SIZE				(35648)
#define IMG_BKL_LEVEL_3_END					(IMG_BKL_LEVEL_3_ADDR+IMG_BKL_LEVEL_3_SIZE)
#define IMG_BKL_LEVEL_4_ADDR				(IMG_BKL_LEVEL_3_END)
#define IMG_BKL_LEVEL_4_SIZE				(35648)
#define IMG_BKL_LEVEL_4_END					(IMG_BKL_LEVEL_4_ADDR+IMG_BKL_LEVEL_4_SIZE)
//blood pressure(7)
#define IMG_BP_BG_ADDR						(IMG_BKL_LEVEL_4_END)
#define IMG_BP_BG_SIZE						(33158)
#define IMG_BP_BG_END						(IMG_BP_BG_ADDR+IMG_BP_BG_SIZE)
#define IMG_BP_DOWN_ARRAW_ADDR				(IMG_BP_BG_END)
#define IMG_BP_DOWN_ARRAW_SIZE				(392)
#define	IMG_BP_DOWN_ARRAW_END				(IMG_BP_DOWN_ARRAW_ADDR+IMG_BP_DOWN_ARRAW_SIZE)
#define	IMG_BP_ICON_ANI_1_ADDR				(IMG_BP_DOWN_ARRAW_END)
#define IMG_BP_ICON_ANI_1_SIZE				(1768)
#define IMG_BP_ICON_ANI_1_END				(IMG_BP_ICON_ANI_1_ADDR+IMG_BP_ICON_ANI_1_SIZE)
#define IMG_BP_ICON_ANI_2_ADDR				(IMG_BP_ICON_ANI_1_END)
#define IMG_BP_ICON_ANI_2_SIZE				(1768)
#define IMG_BP_ICON_ANI_2_END				(IMG_BP_ICON_ANI_2_ADDR+IMG_BP_ICON_ANI_2_SIZE)
#define IMG_BP_ICON_ANI_3_ADDR				(IMG_BP_ICON_ANI_2_END)
#define IMG_BP_ICON_ANI_3_SIZE				(1768)
#define IMG_BP_ICON_ANI_3_END				(IMG_BP_ICON_ANI_3_ADDR+IMG_BP_ICON_ANI_3_SIZE)
#define IMG_BP_SPE_LINE_ADDR				(IMG_BP_ICON_ANI_3_END)
#define IMG_BP_SPE_LINE_SIZE				(504)
#define IMG_BP_SPE_LINE_END					(IMG_BP_SPE_LINE_ADDR+IMG_BP_SPE_LINE_SIZE)
#define IMG_BP_UNIT_ADDR					(IMG_BP_SPE_LINE_END)
#define IMG_BP_UNIT_SIZE					(1100)
#define IMG_BP_UNIT_END						(IMG_BP_UNIT_ADDR+IMG_BP_UNIT_SIZE)
#define IMG_BP_UP_ARRAW_ADDR				(IMG_BP_UNIT_END)
#define IMG_BP_UP_ARRAW_SIZE				(586)
#define IMG_BP_UP_ARRAW_END					(IMG_BP_UP_ARRAW_ADDR+IMG_BP_UP_ARRAW_SIZE)
//function option(10)
#define IMG_FUN_OPT_BP_ICON_ADDR			(IMG_BP_UP_ARRAW_END)
#define IMG_FUN_OPT_BP_ICON_SIZE			(4240)
#define IMG_FUN_OPT_BP_ICON_END				(IMG_FUN_OPT_BP_ICON_ADDR+IMG_FUN_OPT_BP_ICON_SIZE)
#define IMG_FUN_OPT_HR_ICON_ADDR			(IMG_FUN_OPT_BP_ICON_END)
#define IMG_FUN_OPT_HR_ICON_SIZE			(4240)
#define IMG_FUN_OPT_HR_ICON_END				(IMG_FUN_OPT_HR_ICON_ADDR+IMG_FUN_OPT_HR_ICON_SIZE)
#define IMG_FUN_OPT_PGDN_ICON_ADDR			(IMG_FUN_OPT_HR_ICON_END)
#define IMG_FUN_OPT_PGDN_ICON_SIZE			(272)
#define IMG_FUN_OPT_PGDN_ICON_END			(IMG_FUN_OPT_PGDN_ICON_ADDR+IMG_FUN_OPT_PGDN_ICON_SIZE)
#define IMG_FUN_OPT_PGUP_ICON_ADDR			(IMG_FUN_OPT_PGDN_ICON_END)
#define IMG_FUN_OPT_PGUP_ICON_SIZE			(272)
#define IMG_FUN_OPT_PGUP_ICON_END			(IMG_FUN_OPT_PGUP_ICON_ADDR+IMG_FUN_OPT_PGUP_ICON_SIZE)
#define IMG_FUN_OPT_SETTING_ICON_ADDR		(IMG_FUN_OPT_PGUP_ICON_END)
#define IMG_FUN_OPT_SETTING_ICON_SIZE		(4240)
#define IMG_FUN_OPT_SETTING_ICON_END		(IMG_FUN_OPT_SETTING_ICON_ADDR+IMG_FUN_OPT_SETTING_ICON_SIZE)
#define IMG_FUN_OPT_SLEEP_ICON_ADDR			(IMG_FUN_OPT_SETTING_ICON_END)
#define IMG_FUN_OPT_SLEEP_ICON_SIZE			(4240)
#define IMG_FUN_OPT_SLEEP_ICON_END			(IMG_FUN_OPT_SLEEP_ICON_ADDR+IMG_FUN_OPT_SLEEP_ICON_SIZE)
#define IMG_FUN_OPT_SPO2_ICON_ADDR			(IMG_FUN_OPT_SLEEP_ICON_END)
#define IMG_FUN_OPT_SPO2_ICON_SIZE			(4240)
#define IMG_FUN_OPT_SPO2_ICON_END			(IMG_FUN_OPT_SPO2_ICON_ADDR+IMG_FUN_OPT_SPO2_ICON_SIZE)
#define IMG_FUN_OPT_STEP_ICON_ADDR			(IMG_FUN_OPT_SPO2_ICON_END)
#define IMG_FUN_OPT_STEP_ICON_SIZE			(4240)
#define IMG_FUN_OPT_STEP_ICON_END			(IMG_FUN_OPT_STEP_ICON_ADDR+IMG_FUN_OPT_STEP_ICON_SIZE)
#define IMG_FUN_OPT_SYNC_ICON_ADDR			(IMG_FUN_OPT_STEP_ICON_END)
#define IMG_FUN_OPT_SYNC_ICON_SIZE			(4240)
#define IMG_FUN_OPT_SYNC_ICON_END			(IMG_FUN_OPT_SYNC_ICON_ADDR+IMG_FUN_OPT_SYNC_ICON_SIZE)
#define IMG_FUN_OPT_TEMP_ICON_ADDR			(IMG_FUN_OPT_SYNC_ICON_END)
#define IMG_FUN_OPT_TEMP_ICON_SIZE			(4240)
#define IMG_FUN_OPT_TEMP_ICON_END			(IMG_FUN_OPT_TEMP_ICON_ADDR+IMG_FUN_OPT_TEMP_ICON_SIZE)
//heart rate(6)
#define IMG_HR_BG_ADDR						(IMG_FUN_OPT_TEMP_ICON_END)
#define IMG_HR_BG_SIZE						(33548)
#define IMG_HR_BG_END						(IMG_HR_BG_ADDR+IMG_HR_BG_SIZE)
#define IMG_HR_BPM_ADDR						(IMG_HR_BG_END)
#define IMG_HR_BPM_SIZE						(1988)
#define IMG_HR_BPM_END						(IMG_HR_BPM_ADDR+IMG_HR_BPM_SIZE)
#define IMG_HR_DOWN_ARRAW_ADDR				(IMG_HR_BPM_END)
#define IMG_HR_DOWN_ARRAW_SIZE				(548)
#define IMG_HR_DOWN_ARRAW_END				(IMG_HR_DOWN_ARRAW_ADDR+IMG_HR_DOWN_ARRAW_SIZE)
#define	IMG_HR_ICON_ANI_1_ADDR				(IMG_HR_DOWN_ARRAW_END)
#define	IMG_HR_ICON_ANI_1_SIZE				(2116)
#define	IMG_HR_ICON_ANI_1_END				(IMG_HR_ICON_ANI_1_ADDR+IMG_HR_ICON_ANI_1_SIZE)
#define	IMG_HR_ICON_ANI_2_ADDR				(IMG_HR_ICON_ANI_1_END)
#define	IMG_HR_ICON_ANI_2_SIZE				(2116)
#define	IMG_HR_ICON_ANI_2_END				(IMG_HR_ICON_ANI_2_ADDR+IMG_HR_ICON_ANI_2_SIZE)
#define IMG_HR_UP_ARRAW_ADDR				(IMG_HR_ICON_ANI_2_END)
#define IMG_HR_UP_ARRAW_SIZE				(484)
#define IMG_HR_UP_ARRAW_END					(IMG_HR_UP_ARRAW_ADDR+IMG_HR_UP_ARRAW_SIZE)
//idle hr(2)
#define IMG_IDLE_HR_BG_ADDR					(IMG_HR_UP_ARRAW_END)
#define IMG_IDLE_HR_BG_SIZE					(7208)
#define IMG_IDLE_HR_BG_END					(IMG_IDLE_HR_BG_ADDR+IMG_IDLE_HR_BG_SIZE)
#define IMG_IDLE_HR_ICON_ADDR				(IMG_IDLE_HR_BG_END)
#define IMG_IDLE_HR_ICON_SIZE				(1304)
#define IMG_IDLE_HR_ICON_END				(IMG_IDLE_HR_ICON_ADDR+IMG_IDLE_HR_ICON_SIZE)
//idle net mode(2)
#define IMG_IDLE_NET_LTEM_ADDR				(IMG_IDLE_HR_ICON_END)
#define IMG_IDLE_NET_LTEM_SIZE				(344)
#define IMG_IDLE_NET_LTEM_END				(IMG_IDLE_NET_LTEM_ADDR+IMG_IDLE_NET_LTEM_SIZE)
#define IMG_IDLE_NET_NB_ADDR				(IMG_IDLE_NET_LTEM_END)
#define IMG_IDLE_NET_NB_SIZE				(316)
#define IMG_IDLE_NET_NB_END					(IMG_IDLE_NET_NB_ADDR+IMG_IDLE_NET_NB_SIZE)
//idle temp(3)
#define IMG_IDLE_TEMP_BG_ADDR				(IMG_IDLE_NET_NB_END)
#define IMG_IDLE_TEMP_BG_SIZE				(7208)
#define IMG_IDLE_TEMP_BG_END				(IMG_IDLE_TEMP_BG_ADDR+IMG_IDLE_TEMP_BG_SIZE)
#define IMG_IDLE_TEMP_C_ICON_ADDR			(IMG_IDLE_TEMP_BG_END)
#define IMG_IDLE_TEMP_C_ICON_SIZE			(968)
#define IMG_IDLE_TEMP_C_ICON_END			(IMG_IDLE_TEMP_C_ICON_ADDR+IMG_IDLE_TEMP_C_ICON_SIZE)
#define IMG_IDLE_TEMP_F_ICON_ADDR			(IMG_IDLE_TEMP_C_ICON_END)
#define IMG_IDLE_TEMP_F_ICON_SIZE			(968)
#define IMG_IDLE_TEMP_F_ICON_END			(IMG_IDLE_TEMP_F_ICON_ADDR+IMG_IDLE_TEMP_F_ICON_SIZE)
//ota(5)
#define IMG_OTA_FAILED_ICON_ADDR			(IMG_IDLE_TEMP_F_ICON_END)
#define IMG_OTA_FAILED_ICON_SIZE			(16208)
#define IMG_OTA_FAILED_ICON_END				(IMG_OTA_FAILED_ICON_ADDR+IMG_OTA_FAILED_ICON_SIZE)
#define IMG_OTA_FINISH_ICON_ADDR			(IMG_OTA_FAILED_ICON_END)
#define IMG_OTA_FINISH_ICON_SIZE			(16208)
#define IMG_OTA_FINISH_ICON_END				(IMG_OTA_FINISH_ICON_ADDR+IMG_OTA_FINISH_ICON_SIZE)
#define IMG_OTA_LOGO_ADDR					(IMG_OTA_FINISH_ICON_END)
#define IMG_OTA_LOGO_SIZE					(7208)
#define IMG_OTA_LOGO_END					(IMG_OTA_LOGO_ADDR+IMG_OTA_LOGO_SIZE)
#define IMG_OTA_NO_ADDR						(IMG_OTA_LOGO_END)
#define IMG_OTA_NO_SIZE						(3208)
#define IMG_OTA_NO_END						(IMG_OTA_NO_ADDR+IMG_OTA_NO_SIZE)
#define IMG_OTA_YES_ADDR					(IMG_OTA_NO_END)
#define IMG_OTA_YES_SIZE					(3208)
#define IMG_OTA_YES_END						(IMG_OTA_YES_ADDR+IMG_OTA_YES_SIZE)
//power off(1)
#define IMG_PWROFF_BUTTON_ADDR				(IMG_OTA_YES_END)
#define IMG_PWROFF_BUTTON_SIZE				(32266)
#define IMG_PWROFF_BUTTON_END				(IMG_PWROFF_BUTTON_ADDR+IMG_PWROFF_BUTTON_SIZE)
//power on(6)
#define IMG_PWRON_ANI_1_ADDR				(IMG_PWROFF_BUTTON_END)
#define IMG_PWRON_ANI_1_SIZE				(13388)
#define IMG_PWRON_ANI_1_END					(IMG_PWRON_ANI_1_ADDR+IMG_PWRON_ANI_1_SIZE)
#define IMG_PWRON_ANI_2_ADDR				(IMG_PWRON_ANI_1_END)
#define IMG_PWRON_ANI_2_SIZE				(13388)
#define IMG_PWRON_ANI_2_END					(IMG_PWRON_ANI_2_ADDR+IMG_PWRON_ANI_2_SIZE)
#define IMG_PWRON_ANI_3_ADDR				(IMG_PWRON_ANI_2_END)
#define IMG_PWRON_ANI_3_SIZE				(13388)
#define IMG_PWRON_ANI_3_END					(IMG_PWRON_ANI_3_ADDR+IMG_PWRON_ANI_3_SIZE)
#define IMG_PWRON_ANI_4_ADDR				(IMG_PWRON_ANI_3_END)
#define IMG_PWRON_ANI_4_SIZE				(13388)
#define IMG_PWRON_ANI_4_END					(IMG_PWRON_ANI_4_ADDR+IMG_PWRON_ANI_4_SIZE)
#define IMG_PWRON_ANI_5_ADDR				(IMG_PWRON_ANI_4_END)
#define IMG_PWRON_ANI_5_SIZE				(13388)
#define IMG_PWRON_ANI_5_END					(IMG_PWRON_ANI_5_ADDR+IMG_PWRON_ANI_5_SIZE)
#define IMG_PWRON_ANI_6_ADDR				(IMG_PWRON_ANI_5_END)
#define IMG_PWRON_ANI_6_SIZE				(13388)
#define IMG_PWRON_ANI_6_END					(IMG_PWRON_ANI_6_ADDR+IMG_PWRON_ANI_6_SIZE)
//reset(13)
#define IMG_RESET_ANI_1_ADDR				(IMG_PWRON_ANI_6_END)
#define IMG_RESET_ANI_1_SIZE				(16208)
#define IMG_RESET_ANI_1_END					(IMG_RESET_ANI_1_ADDR+IMG_RESET_ANI_1_SIZE)
#define IMG_RESET_ANI_2_ADDR				(IMG_RESET_ANI_1_END)
#define IMG_RESET_ANI_2_SIZE				(16208)
#define IMG_RESET_ANI_2_END					(IMG_RESET_ANI_2_ADDR+IMG_RESET_ANI_2_SIZE)
#define IMG_RESET_ANI_3_ADDR				(IMG_RESET_ANI_2_END)
#define IMG_RESET_ANI_3_SIZE				(16208)
#define IMG_RESET_ANI_3_END					(IMG_RESET_ANI_3_ADDR+IMG_RESET_ANI_3_SIZE)
#define IMG_RESET_ANI_4_ADDR				(IMG_RESET_ANI_3_END)
#define IMG_RESET_ANI_4_SIZE				(16208)
#define IMG_RESET_ANI_4_END					(IMG_RESET_ANI_4_ADDR+IMG_RESET_ANI_4_SIZE)
#define IMG_RESET_ANI_5_ADDR				(IMG_RESET_ANI_4_END)
#define IMG_RESET_ANI_5_SIZE				(16208)
#define IMG_RESET_ANI_5_END					(IMG_RESET_ANI_5_ADDR+IMG_RESET_ANI_5_SIZE)
#define IMG_RESET_ANI_6_ADDR				(IMG_RESET_ANI_5_END)
#define IMG_RESET_ANI_6_SIZE				(16208)
#define IMG_RESET_ANI_6_END					(IMG_RESET_ANI_6_ADDR+IMG_RESET_ANI_6_SIZE)
#define IMG_RESET_ANI_7_ADDR				(IMG_RESET_ANI_6_END)
#define IMG_RESET_ANI_7_SIZE				(16208)
#define IMG_RESET_ANI_7_END					(IMG_RESET_ANI_7_ADDR+IMG_RESET_ANI_7_SIZE)
#define IMG_RESET_ANI_8_ADDR				(IMG_RESET_ANI_7_END)
#define IMG_RESET_ANI_8_SIZE				(16208)
#define IMG_RESET_ANI_8_END					(IMG_RESET_ANI_8_ADDR+IMG_RESET_ANI_8_SIZE)
#define IMG_RESET_FAIL_ADDR					(IMG_RESET_ANI_8_END)
#define IMG_RESET_FAIL_SIZE					(16208)
#define IMG_RESET_FAIL_END					(IMG_RESET_FAIL_ADDR+IMG_RESET_FAIL_SIZE)
#define IMG_RESET_LOGO_ADDR					(IMG_RESET_FAIL_END)
#define IMG_RESET_LOGO_SIZE					(7208)
#define IMG_RESET_LOGO_END					(IMG_RESET_LOGO_ADDR+IMG_RESET_LOGO_SIZE)
#define IMG_RESET_NO_ADDR					(IMG_RESET_LOGO_END)
#define IMG_RESET_NO_SIZE					(3208)
#define IMG_RESET_NO_END					(IMG_RESET_NO_ADDR+IMG_RESET_NO_SIZE)
#define IMG_RESET_SUCCESS_ADDR				(IMG_RESET_NO_END)
#define IMG_RESET_SUCCESS_SIZE				(16208)
#define IMG_RESET_SUCCESS_END				(IMG_RESET_SUCCESS_ADDR+IMG_RESET_SUCCESS_SIZE)
#define IMG_RESET_YES_ADDR					(IMG_RESET_SUCCESS_END)
#define IMG_RESET_YES_SIZE					(3208)
#define IMG_RESET_YES_END					(IMG_RESET_YES_ADDR+IMG_RESET_YES_SIZE)
//3 dot gif(3)
#define IMG_RUNNING_ANI_1_ADDR				(IMG_RESET_YES_END)
#define IMG_RUNNING_ANI_1_SIZE				(2114)
#define IMG_RUNNING_ANI_1_END				(IMG_RUNNING_ANI_1_ADDR+IMG_RUNNING_ANI_1_SIZE)
#define IMG_RUNNING_ANI_2_ADDR				(IMG_RUNNING_ANI_1_END)
#define IMG_RUNNING_ANI_2_SIZE				(2114)
#define IMG_RUNNING_ANI_2_END				(IMG_RUNNING_ANI_2_ADDR+IMG_RUNNING_ANI_2_SIZE)
#define IMG_RUNNING_ANI_3_ADDR				(IMG_RUNNING_ANI_2_END)
#define IMG_RUNNING_ANI_3_SIZE				(2114)
#define IMG_RUNNING_ANI_3_END				(IMG_RUNNING_ANI_3_ADDR+IMG_RUNNING_ANI_3_SIZE)
//select icon(2)
#define IMG_SELECT_ICON_NO_ADDR				(IMG_RUNNING_ANI_3_END)
#define IMG_SELECT_ICON_NO_SIZE				(808)
#define IMG_SELECT_ICON_NO_END				(IMG_SELECT_ICON_NO_ADDR+IMG_SELECT_ICON_NO_SIZE)
#define IMG_SELECT_ICON_YES_ADDR			(IMG_SELECT_ICON_NO_END)
#define IMG_SELECT_ICON_YES_SIZE			(808)
#define IMG_SELECT_ICON_YES_END				(IMG_SELECT_ICON_YES_ADDR+IMG_SELECT_ICON_YES_SIZE)
//settings(8)
#define IMG_SET_BG_ADDR						(IMG_SELECT_ICON_YES_END)
#define IMG_SET_BG_SIZE						(12852)
#define IMG_SET_BG_END						(IMG_SET_BG_ADDR+IMG_SET_BG_SIZE)
#define IMG_SET_INFO_BG_ADDR				(IMG_SET_BG_END)
#define IMG_SET_INFO_BG_SIZE				(17008)
#define IMG_SET_INFO_BG_END					(IMG_SET_INFO_BG_ADDR+IMG_SET_INFO_BG_SIZE)
#define IMG_SET_PG_1_ADDR					(IMG_SET_INFO_BG_END)
#define IMG_SET_PG_1_SIZE					(272)
#define IMG_SET_PG_1_END					(IMG_SET_PG_1_ADDR+IMG_SET_PG_1_SIZE)
#define IMG_SET_PG_2_ADDR					(IMG_SET_PG_1_END)
#define IMG_SET_PG_2_SIZE					(272)
#define IMG_SET_PG_2_END					(IMG_SET_PG_2_ADDR+IMG_SET_PG_2_SIZE)
#define IMG_SET_SWITCH_OFF_ICON_ADDR		(IMG_SET_PG_2_END)
#define IMG_SET_SWITCH_OFF_ICON_SIZE		(4152)
#define IMG_SET_SWITCH_OFF_ICON_END			(IMG_SET_SWITCH_OFF_ICON_ADDR+IMG_SET_SWITCH_OFF_ICON_SIZE)
#define IMG_SET_SWITCH_ON_ICON_ADDR			(IMG_SET_SWITCH_OFF_ICON_END)
#define IMG_SET_SWITCH_ON_ICON_SIZE			(4152)
#define IMG_SET_SWITCH_ON_ICON_END			(IMG_SET_SWITCH_ON_ICON_ADDR+IMG_SET_SWITCH_ON_ICON_SIZE)
#define IMG_SET_TEMP_UNIT_C_ICON_ADDR		(IMG_SET_SWITCH_ON_ICON_END)
#define IMG_SET_TEMP_UNIT_C_ICON_SIZE		(2528)
#define IMG_SET_TEMP_UNIT_C_ICON_END		(IMG_SET_TEMP_UNIT_C_ICON_ADDR+IMG_SET_TEMP_UNIT_C_ICON_SIZE)
#define IMG_SET_TEMP_UNIT_F_ICON_ADDR		(IMG_SET_TEMP_UNIT_C_ICON_END)
#define IMG_SET_TEMP_UNIT_F_ICON_SIZE		(2528)
#define IMG_SET_TEMP_UNIT_F_ICON_END		(IMG_SET_TEMP_UNIT_F_ICON_ADDR+IMG_SET_TEMP_UNIT_F_ICON_SIZE)
//signal(5)
#define IMG_SIG_0_ADDR						(IMG_SET_TEMP_UNIT_F_ICON_END)
#define IMG_SIG_0_SIZE						(1304)
#define IMG_SIG_0_END						(IMG_SIG_0_ADDR+IMG_SIG_0_SIZE)
#define IMG_SIG_1_ADDR						(IMG_SIG_0_END)
#define IMG_SIG_1_SIZE						(1304)
#define IMG_SIG_1_END						(IMG_SIG_1_ADDR+IMG_SIG_1_SIZE)
#define IMG_SIG_2_ADDR						(IMG_SIG_1_END)
#define IMG_SIG_2_SIZE						(1304)
#define IMG_SIG_2_END						(IMG_SIG_2_ADDR+IMG_SIG_2_SIZE)
#define IMG_SIG_3_ADDR						(IMG_SIG_2_END)
#define IMG_SIG_3_SIZE						(1304)
#define IMG_SIG_3_END						(IMG_SIG_3_ADDR+IMG_SIG_3_SIZE)
#define IMG_SIG_4_ADDR						(IMG_SIG_3_END)
#define IMG_SIG_4_SIZE						(1304)
#define IMG_SIG_4_END						(IMG_SIG_4_ADDR+IMG_SIG_4_SIZE)
//sleep(6)
#define IMG_SLEEP_ANI_3_ADDR				(IMG_SIG_4_END)
#define IMG_SLEEP_ANI_3_SIZE				(4608)
#define IMG_SLEEP_ANI_3_END					(IMG_SLEEP_ANI_3_ADDR+IMG_SLEEP_ANI_3_SIZE)
#define IMG_SLEEP_DEEP_ICON_ADDR			(IMG_SLEEP_ANI_3_END)
#define IMG_SLEEP_DEEP_ICON_SIZE			(2600)
#define IMG_SLEEP_DEEP_ICON_END				(IMG_SLEEP_DEEP_ICON_ADDR+IMG_SLEEP_DEEP_ICON_SIZE)
#define IMG_SLEEP_HOUR_ADDR					(IMG_SLEEP_DEEP_ICON_END)
#define IMG_SLEEP_HOUR_SIZE					(844)
#define IMG_SLEEP_HOUR_END					(IMG_SLEEP_HOUR_ADDR+IMG_SLEEP_HOUR_SIZE)
#define IMG_SLEEP_LIGHT_ICON_ADDR			(IMG_SLEEP_HOUR_END)
#define IMG_SLEEP_LIGHT_ICON_SIZE			(2600)
#define IMG_SLEEP_LIGHT_ICON_END			(IMG_SLEEP_LIGHT_ICON_ADDR+IMG_SLEEP_LIGHT_ICON_SIZE)
#define IMG_SLEEP_LINE_ADDR					(IMG_SLEEP_LIGHT_ICON_END)
#define IMG_SLEEP_LINE_SIZE					(712)
#define IMG_SLEEP_LINE_END					(IMG_SLEEP_LINE_ADDR+IMG_SLEEP_LINE_SIZE)
#define IMG_SLEEP_MIN_ADDR					(IMG_SLEEP_LINE_END)
#define IMG_SLEEP_MIN_SIZE					(1460)
#define IMG_SLEEP_MIN_END					(IMG_SLEEP_MIN_ADDR+IMG_SLEEP_MIN_SIZE)
//sos(4)
#define IMG_SOS_ANI_1_ADDR					(IMG_SLEEP_MIN_END)
#define IMG_SOS_ANI_1_SIZE					(40880)
#define IMG_SOS_ANI_1_END					(IMG_SOS_ANI_1_ADDR+IMG_SOS_ANI_1_SIZE)
#define IMG_SOS_ANI_2_ADDR					(IMG_SOS_ANI_1_END)
#define IMG_SOS_ANI_2_SIZE					(40880)
#define IMG_SOS_ANI_2_END					(IMG_SOS_ANI_2_ADDR+IMG_SOS_ANI_2_SIZE)
#define IMG_SOS_ANI_3_ADDR					(IMG_SOS_ANI_2_END)
#define IMG_SOS_ANI_3_SIZE					(40880)
#define IMG_SOS_ANI_3_END					(IMG_SOS_ANI_3_ADDR+IMG_SOS_ANI_3_SIZE)
#define IMG_SOS_ANI_4_ADDR					(IMG_SOS_ANI_3_END)
#define IMG_SOS_ANI_4_SIZE					(40880)
#define IMG_SOS_ANI_4_END					(IMG_SOS_ANI_4_ADDR+IMG_SOS_ANI_4_SIZE)
//spo2(6)
#define IMG_SPO2_ANI_1_ADDR					(IMG_SOS_ANI_4_END)
#define IMG_SPO2_ANI_1_SIZE					(2328)
#define IMG_SPO2_ANI_1_END					(IMG_SPO2_ANI_1_ADDR+IMG_SPO2_ANI_1_SIZE)
#define IMG_SPO2_ANI_2_ADDR					(IMG_SPO2_ANI_1_END)
#define IMG_SPO2_ANI_2_SIZE					(2328)
#define IMG_SPO2_ANI_2_END					(IMG_SPO2_ANI_2_ADDR+IMG_SPO2_ANI_2_SIZE)
#define IMG_SPO2_ANI_3_ADDR					(IMG_SPO2_ANI_2_END)
#define IMG_SPO2_ANI_3_SIZE					(2328)
#define IMG_SPO2_ANI_3_END					(IMG_SPO2_ANI_3_ADDR+IMG_SPO2_ANI_3_SIZE)
#define IMG_SPO2_BG_ADDR					(IMG_SPO2_ANI_3_END)
#define IMG_SPO2_BG_SIZE					(33158)
#define IMG_SPO2_BG_END						(IMG_SPO2_BG_ADDR+IMG_SPO2_BG_SIZE)
#define IMG_SPO2_DOWN_ARRAW_ADDR			(IMG_SPO2_BG_END)
#define IMG_SPO2_DOWN_ARRAW_SIZE			(484)
#define IMG_SPO2_DOWN_ARRAW_END				(IMG_SPO2_DOWN_ARRAW_ADDR+IMG_SPO2_DOWN_ARRAW_SIZE)
#define IMG_SPO2_UP_ARRAW_ADDR				(IMG_SPO2_DOWN_ARRAW_END)
#define IMG_SPO2_UP_ARRAW_SIZE				(484)
#define IMG_SPO2_UP_ARRAW_END				(IMG_SPO2_UP_ARRAW_ADDR+IMG_SPO2_UP_ARRAW_SIZE)
//sport achieve(4)
#define IMG_SPORT_ACHIEVE_BG_ADDR			(IMG_SPO2_UP_ARRAW_END)
#define IMG_SPORT_ACHIEVE_BG_SIZE			(14888)
#define IMG_SPORT_ACHIEVE_BG_END			(IMG_SPORT_ACHIEVE_BG_ADDR+IMG_SPORT_ACHIEVE_BG_SIZE)
#define IMG_SPORT_ACHIEVE_ICON_ADDR			(IMG_SPORT_ACHIEVE_BG_END)
#define IMG_SPORT_ACHIEVE_ICON_SIZE			(2120)
#define IMG_SPORT_ACHIEVE_ICON_END			(IMG_SPORT_ACHIEVE_ICON_ADDR+IMG_SPORT_ACHIEVE_ICON_SIZE)
#define IMG_SPORT_ACHIEVE_LOGO_ADDR			(IMG_SPORT_ACHIEVE_ICON_END)
#define IMG_SPORT_ACHIEVE_LOGO_SIZE			(38650)
#define IMG_SPORT_ACHIEVE_LOGO_END			(IMG_SPORT_ACHIEVE_LOGO_ADDR+IMG_SPORT_ACHIEVE_LOGO_SIZE)
//step(6)
#define IMG_STEP_ANI_1_ADDR					(IMG_SPORT_ACHIEVE_LOGO_END)
#define IMG_STEP_ANI_1_SIZE					(4036)
#define IMG_STEP_ANI_1_END					(IMG_STEP_ANI_1_ADDR+IMG_STEP_ANI_1_SIZE)
#define IMG_STEP_CAL_ICON_ADDR				(IMG_STEP_ANI_1_END)
#define IMG_STEP_CAL_ICON_SIZE				(2024)
#define IMG_STEP_CAL_ICON_END				(IMG_STEP_CAL_ICON_ADDR+IMG_STEP_CAL_ICON_SIZE)
#define IMG_STEP_DIS_ICON_ADDR				(IMG_STEP_CAL_ICON_END)
#define IMG_STEP_DIS_ICON_SIZE				(2096)
#define IMG_STEP_DIS_ICON_END				(IMG_STEP_DIS_ICON_ADDR+IMG_STEP_DIS_ICON_SIZE)
#define IMG_STEP_KCAL_ADDR					(IMG_STEP_DIS_ICON_END)
#define IMG_STEP_KCAL_SIZE					(708)
#define IMG_STEP_KCAL_END					(IMG_STEP_KCAL_ADDR+IMG_STEP_KCAL_SIZE)
#define IMG_STEP_KM_ADDR					(IMG_STEP_KCAL_END)
#define IMG_STEP_KM_SIZE					(484)
#define IMG_STEP_KM_END						(IMG_STEP_KM_ADDR+IMG_STEP_KM_SIZE)
#define IMG_STEP_LINE_ADDR					(IMG_STEP_KM_END)
#define IMG_STEP_LINE_SIZE					(668)
#define IMG_STEP_LINE_END					(IMG_STEP_LINE_ADDR+IMG_STEP_LINE_SIZE)
//sync(3)
#define IMG_SYNC_ERR_ADDR					(IMG_STEP_LINE_END)
#define IMG_SYNC_ERR_SIZE					(42128)
#define IMG_SYNC_ERR_END					(IMG_SYNC_ERR_ADDR+IMG_SYNC_ERR_SIZE)
#define IMG_SYNC_FINISH_ADDR				(IMG_SYNC_ERR_END)
#define IMG_SYNC_FINISH_SIZE				(42128)
#define IMG_SYNC_FINISH_END					(IMG_SYNC_FINISH_ADDR+IMG_SYNC_FINISH_SIZE)
#define IMG_SYNC_LOGO_ADDR					(IMG_SYNC_FINISH_END)
#define IMG_SYNC_LOGO_SIZE					(36458)
#define IMG_SYNC_LOGO_END					(IMG_SYNC_LOGO_ADDR+IMG_SYNC_LOGO_SIZE)
//temperature(4)
#define IMG_TEMP_ICON_C_ADDR				(IMG_SYNC_LOGO_END)
#define IMG_TEMP_ICON_C_SIZE				(2568)
#define IMG_TEMP_ICON_C_END					(IMG_TEMP_ICON_C_ADDR+IMG_TEMP_ICON_C_SIZE)
#define IMG_TEMP_ICON_F_ADDR				(IMG_TEMP_ICON_C_END)
#define IMG_TEMP_ICON_F_SIZE				(2864)
#define IMG_TEMP_ICON_F_END					(IMG_TEMP_ICON_F_ADDR+IMG_TEMP_ICON_F_SIZE)
#define IMG_TEMP_UNIT_C_ADDR				(IMG_TEMP_ICON_F_END)
#define IMG_TEMP_UNIT_C_SIZE				(1756)
#define IMG_TEMP_UNIT_C_END					(IMG_TEMP_UNIT_C_ADDR+IMG_TEMP_UNIT_C_SIZE)
#define IMG_TEMP_UNIT_F_ADDR				(IMG_TEMP_UNIT_C_END)
#define IMG_TEMP_UNIT_F_SIZE				(1756)
#define IMG_TEMP_UNIT_F_END					(IMG_TEMP_UNIT_F_ADDR+IMG_TEMP_UNIT_F_SIZE)
#define IMG_TEMP_C_BG_ADDR					(IMG_TEMP_UNIT_F_END)
#define IMG_TEMP_C_BG_SIZE					(31968)
#define IMG_TEMP_C_BG_END					(IMG_TEMP_C_BG_ADDR+IMG_TEMP_C_BG_SIZE)
#define IMG_TEMP_F_BG_ADDR					(IMG_TEMP_C_BG_END)
#define IMG_TEMP_F_BG_SIZE					(33158)
#define IMG_TEMP_F_BG_END					(IMG_TEMP_F_BG_ADDR+IMG_TEMP_F_BG_SIZE)
#define IMG_TEMP_DOWN_ARRAW_ADDR			(IMG_TEMP_F_BG_END)
#define IMG_TEMP_DOWN_ARRAW_SIZE			(484)
#define IMG_TEMP_DOWN_ARRAW_END				(IMG_TEMP_DOWN_ARRAW_ADDR+IMG_TEMP_DOWN_ARRAW_SIZE)
#define IMG_TEMP_UP_ARRAW_ADDR				(IMG_TEMP_DOWN_ARRAW_END)
#define IMG_TEMP_UP_ARRAW_SIZE				(484)
#define IMG_TEMP_UP_ARRAW_END				(IMG_TEMP_UP_ARRAW_ADDR+IMG_TEMP_UP_ARRAW_SIZE)
//wrist off(2)
#define IMG_WRIST_OFF_ICON_ADDR				(IMG_TEMP_UP_ARRAW_END)
#define IMG_WRIST_OFF_ICON_SIZE				(23372)
#define IMG_WRIST_OFF_ICON_END				(IMG_WRIST_OFF_ICON_ADDR+IMG_WRIST_OFF_ICON_SIZE)

#define IMG_END_ADDR						0x3FFFFF
/***************************************************img end*********************************************************/

/***************************************************font start*********************************************************/
//FONT在flash里占用3M的空间(0x400000~0x6FFFFF)
#define FONT_START_ADDR						0x400000

#define FONT_VER_ADDR						(FONT_START_ADDR)
#define FONT_VER_SIZE						(16)
#define FONT_VER_END						(FONT_VER_ADDR+FONT_VER_SIZE)

#define FONT_DATA_ADDR						FONT_VER_END

#ifdef FONTMAKER_UNICODE_FONT
#define FONT_EN_UNI_16_ADDR					(FONT_DATA_ADDR)
#define FONT_EN_UNI_16_SIZE					(6748)
#define FONT_EN_UNI_16_END					(FONT_EN_UNI_16_ADDR+FONT_EN_UNI_16_SIZE)

#define FONT_EN_UNI_20_ADDR					(FONT_EN_UNI_16_END)
#define FONT_EN_UNI_20_SIZE					(459868)
#define FONT_EN_UNI_20_END					(FONT_EN_UNI_20_ADDR+FONT_EN_UNI_20_SIZE)

#define FONT_EN_UNI_28_ADDR					(FONT_EN_UNI_20_END)
#define FONT_EN_UNI_28_SIZE					(789208)
#define FONT_EN_UNI_28_END					(FONT_EN_UNI_28_ADDR+FONT_EN_UNI_28_SIZE)

#define FONT_EN_UNI_36_ADDR					(FONT_EN_UNI_28_END)
#define FONT_EN_UNI_36_SIZE					(1240332)
#define FONT_EN_UNI_36_END					(FONT_EN_UNI_36_ADDR+FONT_EN_UNI_36_SIZE)

#define FONT_EN_UNI_52_ADDR					(FONT_EN_UNI_36_END)
#define FONT_EN_UNI_52_SIZE					(39472)
#define FONT_EN_UNI_52_END					(FONT_EN_UNI_52_ADDR+FONT_EN_UNI_52_SIZE)

#define FONT_EN_UNI_68_ADDR					(FONT_EN_UNI_52_END)
#define FONT_EN_UNI_68_SIZE					(73052)
#define FONT_EN_UNI_68_END					(FONT_EN_UNI_68_ADDR+FONT_EN_UNI_68_SIZE)

#elif defined(FONTMAKER_MBCS_FONT)

#else
#define FONT_ASC_1608_ADDR					(FONT_DATA_ADDR)
#define FONT_ASC_1608_WIDTH					(16)
#define FONT_ASC_1608_SIZE 					(96*16)
#define FONT_ASC_1608_END					(FONT_ASC_1608_ADDR+FONT_ASC_1608_SIZE)

#define FONT_ASC_2412_ADDR					(FONT_ASC_1608_END)
#define FONT_ASC_2412_WIDTH					(48)
#define FONT_ASC_2412_SIZE 					(96*48)
#define FONT_ASC_2412_END					(FONT_ASC_2412_ADDR+FONT_ASC_2412_SIZE)

#define FONT_ASC_3216_ADDR					(FONT_ASC_2412_END)
#define FONT_ASC_3216_WIDTH					(64)
#define FONT_ASC_3216_SIZE 					(96*64)
#define FONT_ASC_3216_END					(FONT_ASC_3216_ADDR+FONT_ASC_3216_SIZE)

#define FONT_ASC_4824_ADDR					(FONT_ASC_3216_END)
#define FONT_ASC_4824_WIDTH					(144)
#define FONT_ASC_4824_SIZE 					(96*144)
#define FONT_ASC_4824_END					(FONT_ASC_4824_ADDR+FONT_ASC_4824_SIZE)

#define FONT_ASC_6432_ADDR					(FONT_ASC_4824_END)
#define FONT_ASC_6432_WIDTH					(256)
#define FONT_ASC_6432_SIZE 					(96*255)
#define FONT_ASC_6432_END					(FONT_ASC_6432_ADDR+FONT_ASC_6432_SIZE)

#define FONT_CHN_SM_1616_ADDR				(FONT_ASC_6432_END)
#define FONT_CHN_SM_1616_WIDTH				(32)
#define FONT_CHN_SM_1616_SIZE 				(8178*32)
#define FONT_CHN_SM_1616_END				(FONT_CHN_SM_1616_ADDR+FONT_CHN_SM_1616_SIZE)

#define FONT_CHN_SM_2424_ADDR				(FONT_CHN_SM_1616_END)
#define FONT_CHN_SM_2424_WIDTH				(72)
#define FONT_CHN_SM_2424_SIZE 				(8178*72)
#define FONT_CHN_SM_2424_END				(FONT_CHN_SM_2424_ADDR+FONT_CHN_SM_2424_SIZE)

#define FONT_CHN_SM_3232_ADDR				(FONT_CHN_SM_2424_END)
#define FONT_CHN_SM_3232_WIDTH				(128)
#define FONT_CHN_SM_3232_SIZE 				(8178*128)
#define FONT_CHN_SM_3232_END				(FONT_CHN_SM_3232_ADDR+FONT_CHN_SM_3232_SIZE)

#endif

#define FONT_END_ADDR						0x6FFFFF
/***************************************************font end*********************************************************/

/************************************************ppg algo begin******************************************************/
//PPG算法 flash里占用512K的空间(0x700000~0x780000)
#define PPG_ALGO_START_ADDR					0x700000

#define PPG_ALGO_VER_ADDR					PPG_ALGO_START_ADDR
#define PPG_ALGO_VER_SIZE					(16)
#define PPG_ALGO_VER_END					(PPG_ALGO_VER_ADDR+PPG_ALGO_VER_SIZE)

#define PPG_ALGO_FW_ADDR					(PPG_ALGO_VER_END)
#define PPG_ALGO_FW_SIZE 					(353024)
#define PPG_ALGO_FW_END						(PPG_ALGO_FW_ADDR+PPG_ALGO_FW_SIZE)

#define PPG_ALGO_END_ADDR					0x77ffff
/***************************************************ppg algo end*****************************************************/

/***************************************************data begin*******************************************************/
//记录数据 flash里占用512K的空间(0x780000~0x7fffff)
#define DATA_START_ADDR						0x780000
//PPG DATA
#define PPG_BPT_CAL_DATA_ADDR				(DATA_START_ADDR)
#define PPG_BPT_CAL_DATA_SIZE				(240)
#define PPG_BPT_CAL_DATA_END				(PPG_BPT_CAL_DATA_ADDR+PPG_BPT_CAL_DATA_SIZE)
//单次测量(100组数据)
#define PPG_HR_REC1_DATA_ADDR				(PPG_BPT_CAL_DATA_END)
#define PPG_HR_REC1_DATA_SIZE				(100*(7+1))
#define PPG_HR_REC1_DATA_END				(PPG_HR_REC1_DATA_ADDR+PPG_HR_REC1_DATA_SIZE)

#define PPG_SPO2_REC1_DATA_ADDR				(PPG_HR_REC1_DATA_END)
#define PPG_SPO2_REC1_DATA_SIZE				(100*(7+1))
#define PPG_SPO2_REC1_DATA_END				(PPG_SPO2_REC1_DATA_ADDR+PPG_SPO2_REC1_DATA_SIZE)

#define PPG_BPT_REC1_DATA_ADDR				(PPG_SPO2_REC1_DATA_END)
#define PPG_BPT_REC1_DATA_SIZE				(100*(7+2))
#define PPG_BPT_REC1_DATA_END				(PPG_BPT_REC1_DATA_ADDR+PPG_BPT_REC1_DATA_SIZE)
//整点测量(7天数据)
#define PPG_HR_REC2_DATA_ADDR				(PPG_BPT_REC1_DATA_END)
#define PPG_HR_REC2_DATA_SIZE				(7*(4+24*1))
#define PPG_HR_REC2_DATA_END				(PPG_HR_REC2_DATA_ADDR+PPG_HR_REC2_DATA_SIZE)

#define PPG_SPO2_REC2_DATA_ADDR				(PPG_HR_REC2_DATA_END)
#define PPG_SPO2_REC2_DATA_SIZE				(7*(4+24*1))
#define PPG_SPO2_REC2_DATA_END				(PPG_SPO2_REC2_DATA_ADDR+PPG_SPO2_REC2_DATA_SIZE)

#define PPG_BPT_REC2_DATA_ADDR				(PPG_SPO2_REC2_DATA_END)
#define PPG_BPT_REC2_DATA_SIZE				(7*(4+24*2))
#define PPG_BPT_REC2_DATA_END				(PPG_BPT_REC2_DATA_ADDR+PPG_BPT_REC2_DATA_SIZE)

//TEMP DATA
//单次测量(100组数据)
#define TEMP_REC1_DATA_ADDR					(PPG_BPT_REC2_DATA_END)
#define TEMP_REC1_DATA_SIZE					(100*(7+2))
#define TEMP_REC1_DATA_END					(TEMP_REC1_DATA_ADDR+TEMP_REC1_DATA_SIZE)
//整点测量(7天数据)
#define TEMP_REC2_DATA_ADDR					(TEMP_REC1_DATA_END)
#define TEMP_REC2_DATA_SIZE					(7*(4+24*2))
#define TEMP_REC2_DATA_END					(TEMP_REC2_DATA_ADDR+TEMP_REC2_DATA_SIZE)

//IMU DATA
//单次测量(100组数据)
#define STEP_REC1_DATA_ADDR					(TEMP_REC2_DATA_END)
#define STEP_REC1_DATA_SIZE					(100*(7+2))
#define STEP_REC1_DATA_END					(STEP_REC1_DATA_ADDR+STEP_REC1_DATA_SIZE)
//整点测量(7天数据)
#define STEP_REC2_DATA_ADDR					(STEP_REC1_DATA_END)
#define STEP_REC2_DATA_SIZE					(7*(4+24*2))
#define STEP_REC2_DATA_END					(STEP_REC2_DATA_ADDR+STEP_REC2_DATA_SIZE)

#define DATA_END_ADDR						0x7fffff
/****************************************************date end********************************************************/


extern u8_t g_ui_ver[16];
extern u8_t g_font_ver[16];
extern u8_t g_ppg_algo_ver[16];

void SPI_Flash_Init(void);
uint16_t SpiFlash_ReadID(void);

uint8_t SpiFlash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size);
uint8_t SpiFlash_Read(uint8_t *pBuffer,uint32_t ReadAddr,uint32_t size);
uint8_t SpiFlash_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum);
uint8_t SpiFlash_Write_Buf(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum);
void SPIFlash_Erase_Sector(uint32_t SecAddr);
void SPIFlash_Erase_Chip(void);

extern void test_flash(void);

#endif/*__EXTERNAL_FLASH_H__*/
