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
#include "temp.h"

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

//华邦Flash ID
#define	W25Q64DW_ID		0xEF16
#define	W25Q128JW_ID	0xEF17
//旺宏Flash ID
#define MX25R6435F_ID	0xC217
//瑞萨
#define AT25SL0641C_ID	0x1F68

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
//IMG在flash里占用2M的空间(0x000000~0x1FFFFF)
#define IMG_START_ADDR						0x000000
//img ver
#define IMG_VER_ADDR						IMG_START_ADDR
#define IMG_VER_SIZE						(16)
#define IMG_VER_END							(IMG_VER_ADDR+IMG_VER_SIZE)
//img data
#define IMG_DATA_ADDR						(IMG_VER_END)
#define IMG_END_ADDR						0x1FFFFF
/***************************************************img end*********************************************************/

/***************************************************font start*********************************************************/
//FONT在flash里占用5M的空间(0x200000~0x6FFFFF)
#define FONT_START_ADDR						0x200000

#define FONT_VER_ADDR						(FONT_START_ADDR)
#define FONT_VER_SIZE						(16)
#define FONT_VER_END						(FONT_VER_ADDR+FONT_VER_SIZE)

#define FONT_DATA_ADDR						(FONT_VER_END)

#define FONT_EN_UNI_16_ADDR					(FONT_DATA_ADDR)
#define FONT_EN_UNI_16_SIZE					(26452)
#define FONT_EN_UNI_16_END					(FONT_EN_UNI_16_ADDR+FONT_EN_UNI_16_SIZE)

#define FONT_EN_UNI_20_ADDR					(FONT_EN_UNI_16_END)
#define FONT_EN_UNI_20_SIZE					(1177116)
#define FONT_EN_UNI_20_END					(FONT_EN_UNI_20_ADDR+FONT_EN_UNI_20_SIZE)

#define FONT_EN_UNI_28_ADDR					(FONT_EN_UNI_20_END)
#define FONT_EN_UNI_28_SIZE					(2203276)
#define FONT_EN_UNI_28_END					(FONT_EN_UNI_28_ADDR+FONT_EN_UNI_28_SIZE)

#define FONT_EN_UNI_36_ADDR					(FONT_EN_UNI_28_END)
#define FONT_EN_UNI_36_SIZE					(76508)
#define FONT_EN_UNI_36_END					(FONT_EN_UNI_36_ADDR+FONT_EN_UNI_36_SIZE)

#define FONT_EN_UNI_52_ADDR					(FONT_EN_UNI_36_END)
#define FONT_EN_UNI_52_SIZE					(137620)
#define FONT_EN_UNI_52_END					(FONT_EN_UNI_52_ADDR+FONT_EN_UNI_52_SIZE)

#define FONT_EN_UNI_68_ADDR					(FONT_EN_UNI_52_END)
#define FONT_EN_UNI_68_SIZE					(255832)
#define FONT_EN_UNI_68_END					(FONT_EN_UNI_68_ADDR+FONT_EN_UNI_68_SIZE)

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

/*************************************************health data begin**************************************************/
//记录数据 flash里占用128K的空间(0x780000~0x79ffff)
#define DATA_START_ADDR						0x780000
//PPG DATA
#define PPG_BPT_CAL_DATA_ADDR				(DATA_START_ADDR)
#define PPG_BPT_CAL_DATA_SIZE				(240)
#define PPG_BPT_CAL_DATA_END				(PPG_BPT_CAL_DATA_ADDR+PPG_BPT_CAL_DATA_SIZE)
//单次测量(100组数据)
#define PPG_HR_REC1_DATA_ADDR				(PPG_BPT_CAL_DATA_END)
#define PPG_HR_REC1_DATA_SIZE				(100*sizeof(ppg_hr_rec1_data))//800
#define PPG_HR_REC1_DATA_END				(PPG_HR_REC1_DATA_ADDR+PPG_HR_REC1_DATA_SIZE)

#define PPG_SPO2_REC1_DATA_ADDR				(PPG_HR_REC1_DATA_END)
#define PPG_SPO2_REC1_DATA_SIZE				(100*sizeof(ppg_spo2_rec1_data))//800
#define PPG_SPO2_REC1_DATA_END				(PPG_SPO2_REC1_DATA_ADDR+PPG_SPO2_REC1_DATA_SIZE)

#define PPG_BPT_REC1_DATA_ADDR				(PPG_SPO2_REC1_DATA_END)
#define PPG_BPT_REC1_DATA_SIZE				(100*sizeof(ppg_bpt_rec1_data))//900
#define PPG_BPT_REC1_DATA_END				(PPG_BPT_REC1_DATA_ADDR+PPG_BPT_REC1_DATA_SIZE)
//整点测量(7天数据)
#define PPG_HR_REC2_DATA_ADDR				(PPG_BPT_REC1_DATA_END)
#define PPG_HR_REC2_DATA_SIZE				(PPG_REC2_MAX_COUNT*sizeof(hr_rec2_nod))//4704
#define PPG_HR_REC2_DATA_END				(PPG_HR_REC2_DATA_ADDR+PPG_HR_REC2_DATA_SIZE)

#define PPG_SPO2_REC2_DATA_ADDR				(PPG_HR_REC2_DATA_END)
#define PPG_SPO2_REC2_DATA_SIZE				(PPG_REC2_MAX_COUNT*sizeof(spo2_rec2_nod))//4704
#define PPG_SPO2_REC2_DATA_END				(PPG_SPO2_REC2_DATA_ADDR+PPG_SPO2_REC2_DATA_SIZE)

#define PPG_BPT_REC2_DATA_ADDR				(PPG_SPO2_REC2_DATA_END)
#define PPG_BPT_REC2_DATA_SIZE				(PPG_REC2_MAX_COUNT*sizeof(bpt_rec2_nod))//5376
#define PPG_BPT_REC2_DATA_END				(PPG_BPT_REC2_DATA_ADDR+PPG_BPT_REC2_DATA_SIZE)

//TEMP DATA
//单次测量(100组数据)
#define TEMP_REC1_DATA_ADDR					(PPG_BPT_REC2_DATA_END)
#define TEMP_REC1_DATA_SIZE					(100*sizeof(temp_rec1_data))//900
#define TEMP_REC1_DATA_END					(TEMP_REC1_DATA_ADDR+TEMP_REC1_DATA_SIZE)
//整点测量(7天数据)
#define TEMP_REC2_DATA_ADDR					(TEMP_REC1_DATA_END)
#define TEMP_REC2_DATA_SIZE					(TEMP_REC2_MAX_COUNT*sizeof(temp_rec2_nod))//5376
#define TEMP_REC2_DATA_END					(TEMP_REC2_DATA_ADDR+TEMP_REC2_DATA_SIZE)

//IMU DATA
//STEP
//单次测量(100组数据)
#define STEP_REC1_DATA_ADDR					(TEMP_REC2_DATA_END)
#define STEP_REC1_DATA_SIZE					(100*(7+2))//900
#define STEP_REC1_DATA_END					(STEP_REC1_DATA_ADDR+STEP_REC1_DATA_SIZE)
//整点测量(7天数据)
#define STEP_REC2_DATA_ADDR					(STEP_REC1_DATA_END)
#define STEP_REC2_DATA_SIZE					(7*(4+24*2))//364
#define STEP_REC2_DATA_END					(STEP_REC2_DATA_ADDR+STEP_REC2_DATA_SIZE)
//SLEEP
//单次测量(100组数据)
#define SLEEP_REC1_DATA_ADDR				(STEP_REC2_DATA_END)
#define SLEEP_REC1_DATA_SIZE				(100*(7+4))//1100
#define SLEEP_REC1_DATA_END					(SLEEP_REC1_DATA_ADDR+SLEEP_REC1_DATA_SIZE)
//整点测量(7天数据)
#define SLEEP_REC2_DATA_ADDR				(SLEEP_REC1_DATA_END)
#define SLEEP_REC2_DATA_SIZE				(7*(4+24*4))//700
#define SLEEP_REC2_DATA_END					(SLEEP_REC2_DATA_ADDR+SLEEP_REC2_DATA_SIZE)

#define DATA_END_ADDR						0x79ffff
/*************************************************health date end****************************************************/

/************************************************strlib data begin***************************************************/
//字符串数据 flash里占用384K的空间(0x7a0000~0x800000)
#define STR_START_ADDR					0x7a0000

#define STR_VER_ADDR					STR_START_ADDR
#define STR_VER_SIZE					(16)
#define STR_VER_END						(STR_VER_ADDR+STR_VER_SIZE)

#define STR_DATA_ADDR					STR_VER_END
#define STR_DATA_SIZE					(31122)
#define STR_DATA_END					(STR_DATA_ADDR+STR_DATA_SIZE)

#define STR_END_ADDR					0x800000
/*************************************************strlib data end****************************************************/

extern uint8_t g_ui_ver[16];
extern uint8_t g_font_ver[16];
extern uint8_t g_str_ver[16];
extern uint8_t g_ppg_algo_ver[16];

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
