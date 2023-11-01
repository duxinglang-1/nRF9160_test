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
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi3), okay)
#define FLASH_DEVICE DT_NODELABEL(spi3)
#else
#error "spi3 devicetree node is disabled"
#define FLASH_DEVICE	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define FLASH_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define FLASH_PORT	""
#endif
 
#define FLASH_NAME 		"W25Q64FW"
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
#define SPIFlash_BLOCK_SIZE		(64*1024)
#define	SPIFlash_BLOCK_NUMBLE	128

#define	SPI_TXRX_MAX_LEN		(1024*4)	//255	(1024*2)

/***************************************************img start*********************************************************/
//IMG在flash里占用4M的空间(0x000000~0x3FFFFF)
#define IMG_START_ADDR			0x000000
#define IMG_OFFSET				4

//img ver
#define IMG_VER_ADDR		IMG_START_ADDR
#define IMG_VER_SIZE		(16)
#define IMG_VER_END			(IMG_VER_ADDR+IMG_VER_SIZE)
//img data
#define IMG_DATA_ADDR		(IMG_VER_END)
//auri logo
#define IMG_AURI_LOGO_ADDR	IMG_START_ADDR
#define IMG_AURI_LOGO_SIZE	(2*80*160+8)
#define IMG_AURI_LOGO_END	(IMG_AURI_LOGO_ADDR+IMG_AURI_LOGO_SIZE)

//BATTERY
#define IMG_BAT_0_ADDR 		(IMG_AURI_LOGO_END+IMG_OFFSET)
#define IMG_BAT_0_SIZE 		(58)
#define IMG_BAT_0_END 		(IMG_BAT_0_ADDR+IMG_BAT_0_SIZE)
#define IMG_BAT_1_ADDR 		(IMG_BAT_0_END+IMG_OFFSET)
#define IMG_BAT_1_SIZE 		(58)
#define IMG_BAT_1_END 		(IMG_BAT_1_ADDR+IMG_BAT_1_SIZE)
#define IMG_BAT_2_ADDR 		(IMG_BAT_1_END+IMG_OFFSET)
#define IMG_BAT_2_SIZE 		(58)
#define IMG_BAT_2_END 		(IMG_BAT_2_ADDR+IMG_BAT_2_SIZE)
#define IMG_BAT_3_ADDR 		(IMG_BAT_2_END+IMG_OFFSET)
#define IMG_BAT_3_SIZE 		(58)
#define IMG_BAT_3_END 		(IMG_BAT_3_ADDR+IMG_BAT_3_SIZE)
#define IMG_BAT_4_ADDR 		(IMG_BAT_3_END+IMG_OFFSET)
#define IMG_BAT_4_SIZE 		(58)
#define IMG_BAT_4_END 		(IMG_BAT_4_ADDR+IMG_BAT_4_SIZE)
#define IMG_BAT_5_ADDR 		(IMG_BAT_4_END+IMG_OFFSET)
#define IMG_BAT_5_SIZE 		(58)
#define IMG_BAT_5_END 		(IMG_BAT_5_ADDR+IMG_BAT_5_SIZE)

//signal
#define IMG_SIG_0_ADDR 		(IMG_BAT_5_END+IMG_OFFSET)
#define IMG_SIG_0_SIZE 		(26)
#define IMG_SIG_0_END 		(IMG_SIG_0_ADDR+IMG_SIG_0_SIZE)
#define IMG_SIG_1_ADDR 		(IMG_SIG_0_END+IMG_OFFSET)
#define IMG_SIG_1_SIZE 		(26)
#define IMG_SIG_1_END 		(IMG_SIG_1_ADDR+IMG_SIG_1_SIZE)
#define IMG_SIG_2_ADDR 		(IMG_SIG_1_END+IMG_OFFSET)
#define IMG_SIG_2_SIZE 		(26)
#define IMG_SIG_2_END 		(IMG_SIG_2_ADDR+IMG_SIG_2_SIZE)
#define IMG_SIG_3_ADDR 		(IMG_SIG_2_END+IMG_OFFSET)
#define IMG_SIG_3_SIZE 		(26)
#define IMG_SIG_3_END 		(IMG_SIG_3_ADDR+IMG_SIG_3_SIZE)
#define IMG_SIG_4_ADDR 		(IMG_SIG_3_ADDR+IMG_OFFSET)
#define IMG_SIG_4_SIZE 		(26)
#define IMG_SIG_4_END 		(IMG_SIG_4_ADDR+IMG_SIG_4_SIZE)

//colon
#define IMG_COLON_ADDR 		(IMG_SIG_4_END+IMG_OFFSET)
#define IMG_COLON_SIZE 		(26)
#define IMG_COLON_END 		(IMG_COLON_ADDR+IMG_COLON_SIZE)
#define IMG_NO_COLON_ADDR 	(IMG_COLON_END+IMG_OFFSET)
#define IMG_NO_COLON_SIZE 	(26)
#define IMG_NO_COLON_END 	(IMG_NO_COLON_ADDR+IMG_NO_COLON_SIZE)

//big num
#define IMG_BIG_NUM_0_ADDR 	(IMG_NO_COLON_END+IMG_OFFSET)
#define IMG_BIG_NUM_0_SIZE 	(54)
#define IMG_BIG_NUM_0_END 	(IMG_BIG_NUM_0_ADDR+IMG_BIG_NUM_0_SIZE)
#define IMG_BIG_NUM_1_ADDR 	(IMG_BIG_NUM_0_END+IMG_OFFSET)
#define IMG_BIG_NUM_1_SIZE 	(54)
#define IMG_BIG_NUM_1_END 	(IMG_BIG_NUM_1_ADDR+IMG_BIG_NUM_1_SIZE)
#define IMG_BIG_NUM_2_ADDR 	(IMG_BIG_NUM_1_END+IMG_OFFSET)
#define IMG_BIG_NUM_2_SIZE 	(54)
#define IMG_BIG_NUM_2_END 	(IMG_BIG_NUM_2_ADDR+IMG_BIG_NUM_2_SIZE)
#define IMG_BIG_NUM_3_ADDR 	(IMG_BIG_NUM_2_END+IMG_OFFSET)
#define IMG_BIG_NUM_3_SIZE 	(54)
#define IMG_BIG_NUM_3_END 	(IMG_BIG_NUM_3_ADDR+IMG_BIG_NUM_3_SIZE)
#define IMG_BIG_NUM_4_ADDR 	(IMG_BIG_NUM_3_END+IMG_OFFSET)
#define IMG_BIG_NUM_4_SIZE 	(54)
#define IMG_BIG_NUM_4_END 	(IMG_BIG_NUM_4_ADDR+IMG_BIG_NUM_4_SIZE)
#define IMG_BIG_NUM_5_ADDR 	(IMG_BIG_NUM_4_END+IMG_OFFSET)
#define IMG_BIG_NUM_5_SIZE 	(54)
#define IMG_BIG_NUM_5_END 	(IMG_BIG_NUM_5_ADDR+IMG_BIG_NUM_5_SIZE)
#define IMG_BIG_NUM_6_ADDR 	(IMG_BIG_NUM_5_END+IMG_OFFSET)
#define IMG_BIG_NUM_6_SIZE 	(54)
#define IMG_BIG_NUM_6_END 	(IMG_BIG_NUM_6_ADDR+IMG_BIG_NUM_6_SIZE)
#define IMG_BIG_NUM_7_ADDR 	(IMG_BIG_NUM_6_END+IMG_OFFSET)
#define IMG_BIG_NUM_7_SIZE 	(54)
#define IMG_BIG_NUM_7_END 	(IMG_BIG_NUM_7_ADDR+IMG_BIG_NUM_7_SIZE)
#define IMG_BIG_NUM_8_ADDR 	(IMG_BIG_NUM_7_END+IMG_OFFSET)
#define IMG_BIG_NUM_8_SIZE 	(54)
#define IMG_BIG_NUM_8_END 	(IMG_BIG_NUM_8_ADDR+IMG_BIG_NUM_8_SIZE)
#define IMG_BIG_NUM_9_ADDR 	(IMG_BIG_NUM_8_END+IMG_OFFSET)
#define IMG_BIG_NUM_9_SIZE 	(54)
#define IMG_BIG_NUM_9_END 	(IMG_BIG_NUM_9_ADDR+IMG_BIG_NUM_9_SIZE)

//small num
#define IMG_NUM_0_ADDR 		(IMG_BIG_NUM_9_END+IMG_OFFSET)
#define IMG_NUM_0_SIZE 		(46)
#define IMG_NUM_0_END 		(IMG_NUM_0_ADDR+IMG_NUM_0_SIZE)
#define IMG_NUM_1_ADDR 		(IMG_NUM_0_END+IMG_OFFSET)
#define IMG_NUM_1_SIZE 		(46)
#define IMG_NUM_1_END 		(IMG_NUM_1_ADDR+IMG_NUM_1_SIZE)
#define IMG_NUM_2_ADDR 		(IMG_NUM_1_END+IMG_OFFSET)
#define IMG_NUM_2_SIZE 		(46)
#define IMG_NUM_2_END 		(IMG_NUM_2_ADDR+IMG_NUM_2_SIZE)
#define IMG_NUM_3_ADDR 		(IMG_NUM_2_END+IMG_OFFSET)
#define IMG_NUM_3_SIZE 		(46)
#define IMG_NUM_3_END 		(IMG_NUM_3_ADDR+IMG_NUM_3_SIZE)
#define IMG_NUM_4_ADDR 		(IMG_NUM_3_END+IMG_OFFSET)
#define IMG_NUM_4_SIZE 		(46)
#define IMG_NUM_4_END 		(IMG_NUM_4_ADDR+IMG_NUM_4_SIZE)
#define IMG_NUM_5_ADDR 		(IMG_NUM_4_END+IMG_OFFSET)
#define IMG_NUM_5_SIZE 		(46)
#define IMG_NUM_5_END 		(IMG_NUM_5_ADDR+IMG_NUM_5_SIZE)
#define IMG_NUM_6_ADDR 		(IMG_NUM_5_END+IMG_OFFSET)
#define IMG_NUM_6_SIZE 		(46)
#define IMG_NUM_6_END 		(IMG_NUM_6_ADDR+IMG_NUM_6_SIZE)
#define IMG_NUM_7_ADDR 		(IMG_NUM_6_END+IMG_OFFSET)
#define IMG_NUM_7_SIZE 		(46)
#define IMG_NUM_7_END 		(IMG_NUM_7_ADDR+IMG_NUM_7_SIZE)
#define IMG_NUM_8_ADDR 		(IMG_NUM_7_END+IMG_OFFSET)
#define IMG_NUM_8_SIZE 		(46)
#define IMG_NUM_8_END 		(IMG_NUM_8_ADDR+IMG_NUM_8_SIZE)
#define IMG_NUM_9_ADDR 		(IMG_NUM_8_END+IMG_OFFSET)
#define IMG_NUM_9_SIZE 		(46)
#define IMG_NUM_9_END 		(IMG_NUM_9_ADDR+IMG_NUM_9_SIZE)

//fall
#define IMG_FALL_ICON_ADDR 	(IMG_NUM_9_END+IMG_OFFSET)
#define IMG_FALL_ICON_SIZE 	(114)
#define IMG_FALL_ICON_END 	(IMG_FALL_ICON_ADDR+IMG_FALL_ICON_SIZE)
#define IMG_FALL_CN_ADDR 	(IMG_FALL_ICON_END+IMG_OFFSET)
#define IMG_FALL_CN_SIZE 	(194)
#define IMG_FALL_CN_END 	(IMG_FALL_CN_ADDR+IMG_FALL_CN_SIZE)
#define IMG_FALL_EN_ADDR 	(IMG_FALL_CN_END+IMG_OFFSET)
#define IMG_FALL_EN_SIZE 	(114)
#define IMG_FALL_EN_END 	(IMG_FALL_EN_ADDR+IMG_FALL_EN_SIZE)

//sleep
#define IMG_SLP_ICON_ADDR 	(IMG_FALL_EN_END+IMG_OFFSET)
#define IMG_SLP_ICON_SIZE 	(78)
#define IMG_SLP_ICON_END 	(IMG_SLP_ICON_ADDR+IMG_SLP_ICON_SIZE)

//hour,min
#define IMG_HOUR_CN_ADDR 	(IMG_SLP_ICON_END+IMG_OFFSET)
#define IMG_HOUR_CN_SIZE 	(38)
#define IMG_HOUR_CN_END 	(IMG_HOUR_CN_ADDR+IMG_HOUR_CN_SIZE)
#define IMG_MIN_CN_ADDR 	(IMG_HOUR_CN_END+IMG_OFFSET)
#define IMG_MIN_CN_SIZE 	(38)
#define IMG_MIN_CN_END 		(IMG_MIN_CN_ADDR+IMG_MIN_CN_SIZE)
#define IMG_HOUR_EN_ADDR 	(IMG_MIN_CN_END+IMG_OFFSET)
#define IMG_HOUR_EN_SIZE 	(30)
#define IMG_HOUR_EN_END 	(IMG_HOUR_EN_ADDR+IMG_HOUR_EN_SIZE)
#define IMG_MIN_EN_ADDR 	(IMG_HOUR_EN_END+IMG_OFFSET)
#define IMG_MIN_EN_SIZE 	(34)
#define IMG_MIN_EN_END 		(IMG_MIN_EN_ADDR+IMG_MIN_EN_SIZE)

//steps
#define IMG_STEP_ICON_ADDR 	(IMG_MIN_EN_END+IMG_OFFSET)
#define IMG_STEP_ICON_SIZE 	(62)
#define IMG_STEP_ICON_END 	(IMG_STEP_ICON_ADDR+IMG_STEP_ICON_SIZE)

//sos
#define IMG_SOS_ADDR 		(IMG_STEP_ICON_END+IMG_OFFSET)
#define IMG_SOS_SIZE 		(314)
#define IMG_SOS_END 		(IMG_SOS_ADDR+IMG_SOS_SIZE)
#define IMG_SOS_RECE_ADDR 	(IMG_SOS_END+IMG_OFFSET)
#define IMG_SOS_RECE_SIZE 	(342)
#define IMG_SOS_RECE_END 	(IMG_SOS_RECE_ADDR+IMG_SOS_RECE_SIZE)
#define IMG_SOS_SEND_ADDR 	(IMG_SOS_RECE_END+IMG_OFFSET)
#define IMG_SOS_SENDS_SIZE 	(342)
#define IMG_SOS_SEND_END 	(IMG_SOS_SEND_ADDR+IMG_SOS_SENDS_SIZE)

//wrist
#define IMG_WRIST_ICON_ADDR (IMG_SOS_SEND_END+IMG_OFFSET)
#define IMG_WRIST_ICON_SIZE (90)
#define IMG_WRIST_ICON_END 	(IMG_WRIST_ICON_ADDR+IMG_WRIST_ICON_SIZE)
#define IMG_WRIST_CN_ADDR 	(IMG_WRIST_ICON_END+IMG_OFFSET)
#define IMG_WRIST_CN_SIZE 	(194)
#define IMG_WRIST_CN_END 	(IMG_WRIST_CN_ADDR+IMG_WRIST_CN_SIZE)
#define IMG_WRIST_EN_ADDR 	(IMG_WRIST_CN_END+IMG_OFFSET)
#define IMG_WRIST_EN_SIZE 	(226)
#define IMG_WRIST_EN_END 	(IMG_WRIST_EN_ADDR+IMG_WRIST_EN_SIZE)


#define IMG_END_ADDR						0x3FFFFF
/***************************************************img end*********************************************************/

/***************************************************font start*********************************************************/
//FONT在flash里占用3M的空间(0x400000~0x6FFFFF)
#define FONT_START_ADDR			0x400000
#define FONT_OFFSET				4

#define FONT_VER_ADDR			(FONT_START_ADDR)
#define FONT_VER_SIZE			(16)
#define FONT_VER_END			(FONT_VER_ADDR+FONT_VER_SIZE)

#define FONT_DATA_ADDR			(FONT_VER_END)

#define FONT_ASC_0804_ADDR		FONT_START_ADDR
#define FONT_ASC_0804_WIDTH		4
#define FONT_ASC_0804_SIZE 		(96*4)
#define FONT_ASC_0804_END		(FONT_ASC_0804_ADDR+FONT_ASC_0804_SIZE)

#define FONT_ASC_1608_ADDR		(FONT_ASC_0804_END+FONT_OFFSET)
#define FONT_ASC_1608_WIDTH		16
#define FONT_ASC_1608_SIZE 		(96*16)
#define FONT_ASC_1608_END		(FONT_ASC_1608_ADDR+FONT_ASC_1608_SIZE)

#define FONT_ASC_2412_ADDR		(FONT_ASC_1608_END+FONT_OFFSET)
#define FONT_ASC_2412_WIDTH		36
#define FONT_ASC_2412_SIZE 		(96*36)
#define FONT_ASC_2412_END		(FONT_ASC_2412_ADDR+FONT_ASC_2412_SIZE)

#define FONT_ASC_3216_ADDR		(FONT_ASC_2412_END+FONT_OFFSET)
#define FONT_ASC_3216_WIDTH		64
#define FONT_ASC_3216_SIZE 		(96*64)
#define FONT_ASC_3216_END		(FONT_ASC_3216_ADDR+FONT_ASC_3216_SIZE)

#define FONT_CHN_SM_0808_ADDR	(FONT_ASC_3216_END+FONT_OFFSET)
#define FONT_CHN_SM_0808_WIDTH	8
#define FONT_CHN_SM_0808_SIZE 	(8178*8)
#define FONT_CHN_SM_0808_END	(FONT_CHN_SM_0808_ADDR+FONT_CHN_SM_0808_SIZE)

#define FONT_CHN_SM_1616_ADDR	(FONT_CHN_SM_0808_END+FONT_OFFSET)
#define FONT_CHN_SM_1616_WIDTH	32
#define FONT_CHN_SM_1616_SIZE 	(8178*32)
#define FONT_CHN_SM_1616_END	(FONT_CHN_SM_1616_ADDR+FONT_CHN_SM_1616_SIZE)

#define FONT_CHN_SM_2424_ADDR	(FONT_CHN_SM_1616_END+FONT_OFFSET)
#define FONT_CHN_SM_2424_WIDTH	72
#define FONT_CHN_SM_2424_SIZE 	(8178*72)
#define FONT_CHN_SM_2424_END	(FONT_CHN_SM_2424_ADDR+FONT_CHN_SM_2424_SIZE)

#define FONT_CHN_SM_3232_ADDR	(FONT_CHN_SM_2424_END+FONT_OFFSET)
#define FONT_CHN_SM_3232_WIDTH	128
#define FONT_CHN_SM_3232_SIZE 	(8178*128)
#define FONT_CHN_SM_3232_END	(FONT_CHN_SM_3232_ADDR+FONT_CHN_SM_3232_SIZE)

#define FONT_RM_ASC_16_ADDR		(FONT_CHN_SM_3232_END+FONT_OFFSET)
#define FONT_RM_ASC_16_SIZE 	(6768)
#define FONT_RM_ASC_16_END		(FONT_RM_ASC_16_ADDR+FONT_RM_ASC_16_SIZE)

#define FONT_RM_JIS_16_ADDR		(FONT_RM_ASC_16_END+FONT_OFFSET)
#define FONT_RM_JIS_16_SIZE		(360976)
#define FONT_RM_JIS_16_END		(FONT_RM_JIS_16_ADDR+FONT_RM_JIS_16_SIZE)

#define FONT_RM_UNI_16_ADDR		(FONT_RM_JIS_16_END+FONT_OFFSET)
#define FONT_RM_UNI_16_SIZE		(617952)
#define FONT_RM_UNI_16_END		(FONT_RM_UNI_16_ADDR+FONT_RM_UNI_16_SIZE)

#define FONT_END_ADDR			0x6FFFFF
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
//STEP
//单次测量(100组数据)
#define STEP_REC1_DATA_ADDR					(TEMP_REC2_DATA_END)
#define STEP_REC1_DATA_SIZE					(100*(7+2))
#define STEP_REC1_DATA_END					(STEP_REC1_DATA_ADDR+STEP_REC1_DATA_SIZE)
//整点测量(7天数据)
#define STEP_REC2_DATA_ADDR					(STEP_REC1_DATA_END)
#define STEP_REC2_DATA_SIZE					(7*(4+24*2))
#define STEP_REC2_DATA_END					(STEP_REC2_DATA_ADDR+STEP_REC2_DATA_SIZE)
//SLEEP
//单次测量(100组数据)
#define SLEEP_REC1_DATA_ADDR				(STEP_REC2_DATA_END)
#define SLEEP_REC1_DATA_SIZE				(100*(7+4))
#define SLEEP_REC1_DATA_END					(SLEEP_REC1_DATA_ADDR+SLEEP_REC1_DATA_SIZE)
//整点测量(7天数据)
#define SLEEP_REC2_DATA_ADDR				(SLEEP_REC1_DATA_END)
#define SLEEP_REC2_DATA_SIZE				(7*(4+24*4))
#define SLEEP_REC2_DATA_END					(SLEEP_REC2_DATA_ADDR+SLEEP_REC2_DATA_SIZE)

#define DATA_END_ADDR						0x7fffff
/****************************************************date end********************************************************/


void SPI_Flash_Init(void);
uint16_t SpiFlash_ReadID(void);

uint8_t SpiFlash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size);
uint8_t SpiFlash_Read(uint8_t *pBuffer,uint32_t ReadAddr,uint32_t size);
uint8_t SpiFlash_Write(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum);
uint8_t SpiFlash_Write_Buf(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum);
void SPIFlash_Erase_Sector(uint32_t SecAddr);
void SPIFlash_Erase_Block(uint32_t BlockAddr);
void SPIFlash_Erase_Chip(void);

extern void test_flash(void);

#endif/*__EXTERNAL_FLASH_H__*/
