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

//SPI引脚定义
#define FLASH_DEVICE 	"SPI_2"
#define FLASH_NAME 		"W25Q64FW"
#define FLASH_PORT		"GPIO_0"
#define CS		2
#define CLK		3
#define MOSI	4
#define MISO	5

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
#define	SPIFlash_SECTOR_SIZE	(1024*4)
#define	SPI_TXRX_MAX_LEN		(1024*4)	//255	(1024*2)

#define	FLASH_BLOCK_NUMBLE		128
#define	FLASH_PAGE_NUMBLE		32768

//IMG在flash里占用4M的空间(0x000000~0x3FFFFF)
#define IMG_START_ADDR			0x000000
#define IMG_OFFSET				4

#define IMG_PEPPA_80X160_ADDR	IMG_START_ADDR
#define IMG_PEPPA_80X160_SIZE	(2*80*160+8)
#define IMG_PEPPA_80X160_END	(IMG_PEPPA_80X160_ADDR+IMG_PEPPA_80X160_SIZE)

#define IMG_PEPPA_160X160_ADDR	(IMG_PEPPA_80X160_END+IMG_OFFSET)
#define IMG_PEPPA_160X160_SIZE	(2*160*160+8)
#define IMG_PEPPA_160X160_END	(IMG_PEPPA_160X160_ADDR+IMG_PEPPA_160X160_SIZE)

#define IMG_PEPPA_240X240_ADDR	(IMG_PEPPA_160X160_END+IMG_OFFSET)
#define IMG_PEPPA_240X240_SIZE	(2*240*240+8)
#define IMG_PEPPA_240X240_END	(IMG_PEPPA_240X240_ADDR+IMG_PEPPA_240X240_SIZE)

#define IMG_PEPPA_320X320_ADDR	(IMG_PEPPA_240X240_END+IMG_OFFSET)
#define IMG_PEPPA_320X320_SIZE	(2*320*320+8)
#define IMG_PEPPA_320X320_END	(IMG_PEPPA_320X320_ADDR+IMG_PEPPA_320X320_SIZE)

#define IMG_LOGO_96X32_ADDR		(IMG_PEPPA_320X320_END+IMG_OFFSET)
#define IMG_LOGO_96X32_SZIE		(96*(32/8)+6)
#define IMG_LOGO_96X32_END		(IMG_LOGO_96X32_ADDR+IMG_LOGO_96X32_SZIE)

#define IMG_END_ADDR			0x3FFFFF

//FONT在flash里占用2M的空间(0x400000~0x5FFFFF)
#define FONT_START_ADDR			0x400000
#define FONT_OFFSET				4

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

#define FONT_END_ADDR			0x5FFFFF


void SPI_Flash_Init(void);
uint16_t SpiFlash_ReadID(void);

uint8_t SpiFlash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size);
uint8_t SpiFlash_Read(uint8_t *pBuffer,uint32_t ReadAddr,uint32_t size);
void SPIFlash_Erase_Sector(uint32_t SecAddr);
void SPIFlash_Erase_Chip(void);
uint8_t SpiFlash_Write_Buf(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum);
extern void test_flash(void);

#endif/*__EXTERNAL_FLASH_H__*/
