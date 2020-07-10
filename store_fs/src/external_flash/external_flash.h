/****************************************Copyright (c)************************************************
** File name:			external_flash.h
** Last modified Date:          
** Last Version:		V1.0   
** Created by:			Ð»±ë
** Created date:		2020-06-30
** Version:			    1.0
** Descriptions:		Íâ¹ÒflashÇý¶¯Í·ÎÄ¼þ
******************************************************************************************************/
#ifndef __EXTERNAL_FLASH_H__
#define __EXTERNAL_FLASH_H__

#include <stdint.h>

//SPIÒý½Å¶¨Òå
#define FLASH_DEVICE 	"SPI_2"
#define FLASH_NAME 		"W25Q64FW"
#define FLASH_PORT		"GPIO_0"
#define CS		2
#define CLK		3
#define MOSI	4
#define MISO	5

//W25Q64 ID
#define	W25Q64_ID	0XEF16

//SPI FlashÃüÁî¶¨Òå
#define	SPIFlash_WriteEnable	0x06  //Ð´Ê¹ÄÜÃüÁî
#define	SPIFlash_WriteDisable	0x04  //Ð´½ûÖ¹ÃüÁî
#define	SPIFlash_PageProgram	0x02  //Ò³Ð´ÈëÃüÁî
#define	SPIFlash_ReadStatusReg	0x05  //¶Á×´Ì¬¼Ä´æÆ÷1
#define	SPIFlash_WriteStatusReg	0x01  //Ð´×´Ì¬¼Ä´æÆ÷1
#define	SPIFlash_ReadData		0x03  //¶ÁÊý¾ÝÃüÁî
#define	SPIFlash_SecErase		0x20  //ÉÈÇø²Á³ý
#define	SPIFlash_BlockErase		0xD8  //¿é²Á³ý
#define	SPIFlash_ChipErase		0xC7  //È«Æ¬²Á³ý
#define	SPIFlash_ReadID			0x90  //¶ÁÈ¡ID

#define	SPIFLASH_CMD_LENGTH		0x04
#define	SPIFLASH_WRITE_BUSYBIT	0x01

#define	SPIFlash_PAGE_SIZE		256
#define	SPIFlash_SECTOR_SIZE	(1024*4)
#define	SPI_TXRX_MAX_LEN		255

#define	FLASH_BLOCK_NUMBLE		128
#define	FLASH_PAGE_NUMBLE		32768

#define IMG_OFFSET				10
#define PEPPA_PIG_80X160_ADDR	0
#define PEPPA_PIG_80X160_SIZE	(2*80*160+8)
#define PEPPA_PIG_160X160_ADDR	(PEPPA_PIG_80X160_SIZE+IMG_OFFSET)
#define PEPPA_PIG_160X160_SIZE	(2*160*160+8)
#define PEPPA_PIG_240X240_ADDR	(PEPPA_PIG_160X160_SIZE+IMG_OFFSET)
#define PEPPA_PIG_240X240_SIZE	(2*240*240+8)
#define PEPPA_PIG_320X320_ADDR	(PEPPA_PIG_240X240_SIZE+IMG_OFFSET)
#define PEPPA_PIG_320X320_SIZE	(2*320*320+8)


void SPI_Flash_Init(void);
uint16_t SpiFlash_ReadID(void);

uint8_t SpiFlash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size);
uint8_t SpiFlash_Read(uint8_t *pBuffer,uint32_t ReadAddr,uint32_t size);
void SPIFlash_Erase_Sector(uint32_t SecAddr);
void SPIFlash_Erase_Chip(void);
uint8_t SpiFlash_Write_Buf(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t WriteBytesNum);
extern void test_flash(void);

#endif/*__EXTERNAL_FLASH_H__*/
