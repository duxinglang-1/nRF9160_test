/****************************************Copyright (c)************************************************
** File name:			external_flash.c
** Last modified Date:          
** Last Version:		V1.0   
** Created by:			л��
** Created date:		2020-06-30
** Version:			    1.0
** Descriptions:		���flash����Դ�ļ�
******************************************************************************************************/
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <zephyr.h>
#include <device.h>
#include <stdio.h>
#include <string.h>
#include "img.h"
#include "font.h"
#include "external_flash.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max_fw_data_674_1_1_0.h"
#endif
#include "logger.h"

//#define FLASH_DEBUG

struct device *spi_flash;
struct device *gpio_flash;

//SPI���ͻ������飬ʹ��EasyDMAʱһ��Ҫ����Ϊstatic����
static uint8_t    spi_tx_buf[6] = {0};  
//SPI���ջ������飬ʹ��EasyDMAʱһ��Ҫ����Ϊstatic����
static uint8_t    spi_rx_buf[6] = {0};  

//SPI���ͻ������飬ʹ��EasyDMAʱһ��Ҫ����Ϊstatic����
//static uint8_t    my_tx_buf[4096] = {0};
//SPI���ͻ������飬ʹ��EasyDMAʱһ��Ҫ����Ϊstatic����
//static uint8_t    my_rx_buf[4096] = {0};

static struct spi_buf_set tx_bufs,rx_bufs;
static struct spi_buf tx_buff,rx_buff;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

void SpiFlash_CS_LOW(void)
{
	gpio_pin_write(gpio_flash, FLASH_CS_PIN, 0);
}

void SpiFlash_CS_HIGH(void)
{
	gpio_pin_write(gpio_flash, FLASH_CS_PIN, 1);
}

/*****************************************************************************
** ��  ����д��һ���ֽ�
** ��  ����Dat����д�������
** ����ֵ����
******************************************************************************/
void Spi_WriteOneByte(uint8_t Dat)
{   
	int err;

	spi_tx_buf[0] = Dat;
	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	SpiFlash_CS_LOW();
	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, NULL);
	SpiFlash_CS_HIGH();	
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}
	else
	{
	#ifdef FLASH_DEBUG
		LOGD("ok");
	#endif
	}
}
/*****************************************************************************
** ��  ����дʹ��
** ��  ������
** ����ֵ����
******************************************************************************/
static void SpiFlash_Write_Enable(void)
{
	Spi_WriteOneByte(SPIFlash_WriteEnable);
}
/*****************************************************************************
** ��  ������ȡW25Q64FWоƬID
** ��  ������
** ����ֵ��16λID��W25Q64FWоƬIDΪ��0xEF16
******************************************************************************/
uint16_t SpiFlash_ReadID(void)
{
	int err;
	uint16_t dat = 0;

	//׼������
	spi_tx_buf[0] = SPIFlash_ReadID;
	spi_tx_buf[1] = 0x00;
	spi_tx_buf[2] = 0x00;
	spi_tx_buf[3] = 0x00;
	spi_tx_buf[4] = 0xFF;
	spi_tx_buf[5] = 0xFF;
	
	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 6;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	memset(spi_rx_buf, 0, sizeof(spi_rx_buf));
	
	rx_buff.buf = spi_rx_buf;
	rx_buff.len = 6;
	rx_bufs.buffers = &rx_buff;
	rx_bufs.count = 1;

	SpiFlash_CS_LOW();
	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, &rx_bufs);
	SpiFlash_CS_HIGH();
	
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}
	else
	{
	#ifdef FLASH_DEBUG
		LOGD("TX sent: %x,%x,%x,%x,%x,%x", 
			spi_tx_buf[0],
			spi_tx_buf[1],
			spi_tx_buf[2],
			spi_tx_buf[3],
			spi_tx_buf[4],
			spi_tx_buf[5]
			);
		
		LOGD("RX recv: %x,%x,%x,%x,%x,%x", 
			spi_rx_buf[0],
			spi_rx_buf[1],
			spi_rx_buf[2],
			spi_rx_buf[3],
			spi_rx_buf[4],
			spi_rx_buf[5]
			);
	#endif	
		//����������������ֽڲ��Ƕ�ȡ��ID
		dat|=spi_rx_buf[4]<<8;  
		dat|=spi_rx_buf[5];

	#ifdef FLASH_DEBUG	
		LOGD("flash ID: %x", dat);
	#endif
	}



	return dat;
}
/*****************************************************************************
** ��  ������ȡW25Q64FW״̬�Ĵ���
** ��  ������
** ����ֵ��
******************************************************************************/
static uint8_t SpiFlash_ReadSR(void)
{
	int err;

	spi_tx_buf[0] = SPIFlash_ReadStatusReg;
	spi_tx_buf[1] = 0x00;
	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 2;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	memset(spi_rx_buf, 0, sizeof(spi_rx_buf));
	rx_buff.buf = spi_rx_buf;
	rx_buff.len = 2;
	rx_bufs.buffers = &rx_buff;
	rx_bufs.count = 1;

	SpiFlash_CS_LOW();
	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, &rx_bufs);
	SpiFlash_CS_HIGH();
	
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}
	else
	{
	#ifdef FLASH_DEBUG
		LOGD("StatusReg: %x", spi_rx_buf[1]);
	#endif
	}
	
	return spi_rx_buf[1];
}

//�ȴ�W25Q64FW����
void SpiFlash_Wait_Busy(void)   
{   
	while((SpiFlash_ReadSR()&0x01)==0x01);  		// �ȴ�BUSYλ���
} 
/*****************************************************************************
** ��  ��������������W25Q64FW��С�Ĳ�����λ������
** ��  ����[in]SecAddr��������ַ
** ����ֵ����
******************************************************************************/
void SPIFlash_Erase_Sector(uint32_t SecAddr)
{
	int err;

	//����дʹ������
	SpiFlash_Write_Enable();

	//������������
	spi_tx_buf[0] = SPIFlash_SecErase;		
	//24λ��ַ
	spi_tx_buf[1] = (uint8_t)((SecAddr&0x00ff0000)>>16);
	spi_tx_buf[2] = (uint8_t)((SecAddr&0x0000ff00)>>8);
	spi_tx_buf[3] = (uint8_t)SecAddr;
	
	tx_buff.buf = spi_tx_buf;
	tx_buff.len = SPIFLASH_CMD_LENGTH;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	SpiFlash_CS_LOW();
	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, NULL);
	SpiFlash_CS_HIGH();	
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}
	
	//�ȴ�W25Q64FW��ɲ���
	SpiFlash_Wait_Busy();
}
/*****************************************************************************
** ��  ����ȫƬ����W25Q64FW��ȫƬ���������ʱ�����ֵΪ��40��
** ��  ������
** ����ֵ����
******************************************************************************/
void SPIFlash_Erase_Chip(void)
{
	int err;
	
	//����дʹ������
	SpiFlash_Write_Enable();
	
	//ȫƬ��������
	spi_tx_buf[0] = SPIFlash_ChipErase;
	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	SpiFlash_CS_LOW();
	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, NULL);
	SpiFlash_CS_HIGH();
	
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}

	//�ȴ�W25Q64FW��ɲ���
	SpiFlash_Wait_Busy();	
}
/*****************************************************************************
** ��  ������ָ���ĵ�ַд������,���д��ĳ��Ȳ��ܳ����õ�ַ����ҳ���ʣ��ռ�
**         *pBuffer:ָ���д������ݻ���
**         WriteAddr:д�����ʼ��ַ
**         WriteBytesNum:д����ֽ�����һ�����256���ֽ�
** ����ֵ��RET_SUCCESS
******************************************************************************/
uint8_t SpiFlash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size)
{
	int err;
	
	//���д������ݳ����Ƿ�Ϸ���д�볤�Ȳ��ܳ���ҳ��Ĵ�С
	if (size > (SPIFlash_PAGE_SIZE - (WriteAddr%SPIFlash_PAGE_SIZE)))
	{
		return false;
	}

	if(size == 0) 
		return false;

	//����дʹ������
	SpiFlash_Write_Enable();
	
	//ҳ�������
	spi_tx_buf[0] = SPIFlash_PageProgram;
	//24λ��ַ���ߵ�ַ��ǰ
	spi_tx_buf[1] = (uint8_t)((WriteAddr&0x00ff0000)>>16);
	spi_tx_buf[2] = (uint8_t)((WriteAddr&0x0000ff00)>>8);
	spi_tx_buf[3] = (uint8_t)WriteAddr;

	tx_buff.buf = spi_tx_buf;
	tx_buff.len = SPIFLASH_CMD_LENGTH;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	SpiFlash_CS_LOW();
	
	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}

	tx_buff.buf = pBuffer;
	tx_buff.len = size;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}

	SpiFlash_CS_HIGH();
	
	//�ȴ�W25Q64FW��ɲ���
	SpiFlash_Wait_Busy();

	return true;
}
/*****************************************************************************
** ��  ������ָ���ĵ�ַд�����ݣ���д����ҳ
**         *pBuffer:ָ���д�������
**         WriteAddr:д�����ʼ��ַ
**         size:д����ֽ���
** ����ֵ��RET_SUCCESS
******************************************************************************/
uint8_t SpiFlash_Write_Buf(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size)
{
    uint32_t PageByteRemain = 0;
	
	//������ʼ��ַ����ҳ���ʣ��ռ�
    PageByteRemain = SPIFlash_PAGE_SIZE - WriteAddr%SPIFlash_PAGE_SIZE;
	//�����̵����ݳ��Ȳ�����ҳ���ʣ��ռ䣬������ݳ��ȵ���size
    if(size <= PageByteRemain)
    {
        PageByteRemain = size;
    }
	//�ִα�̣�ֱ�����е����ݱ�����
    while(true)
    {
        //���PageByteRemain���ֽ�
		SpiFlash_Write_Page(pBuffer,WriteAddr,PageByteRemain);
		//��������ɣ��˳�ѭ��
        if(size == PageByteRemain)
        {
            break;
        }
        else
        {
            //������ȡ���ݵĻ����ַ
			pBuffer += PageByteRemain;
			//�����̵�ַ
            WriteAddr += PageByteRemain;
			//���ݳ��ȼ�ȥPageByteRemain
            size -= PageByteRemain;
			//�����´α�̵����ݳ���
            if(size > SPIFlash_PAGE_SIZE)
            {
                PageByteRemain = SPIFlash_PAGE_SIZE;
            }
            else
            {
                PageByteRemain = size;
            }
        }
    }
    return true;
}

/*****************************************************************************
** ��  ������ָ���ĵ�ַ����ָ�����ȵ�����
** ��  ����pBuffer��ָ���Ŷ������ݵ��׵�ַ       
**         ReadAddr�����������ݵ���ʼ��ַ
**         size���������ֽ�����ע��size���ܳ���pBuffer�Ĵ�С��������������
** ����ֵ��
******************************************************************************/
uint8_t SpiFlash_Read(uint8_t *pBuffer,uint32_t ReadAddr,uint32_t size)
{
	int err;
	uint32_t read_size;
	
	spi_tx_buf[0] = SPIFlash_ReadData;
	//24λ��ַ���ߵ�ַ��ǰ
	spi_tx_buf[1] = (uint8_t)((ReadAddr&0x00ff0000)>>16);
	spi_tx_buf[2] = (uint8_t)((ReadAddr&0x0000ff00)>>8);
	spi_tx_buf[3] = (uint8_t)ReadAddr;

	tx_buff.buf = spi_tx_buf;
	tx_buff.len = SPIFLASH_CMD_LENGTH;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	SpiFlash_CS_LOW();
	
	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef FLASH_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}

	//��ʼ��ȡ����
	while(size!=0)
	{
		if(size<=SPI_TXRX_MAX_LEN)
		{
			read_size = size;
			size = 0;
		}
		else
		{
			read_size = SPI_TXRX_MAX_LEN;
			size -= SPI_TXRX_MAX_LEN;
		}

		rx_buff.buf = pBuffer;
		rx_buff.len = read_size;
		rx_bufs.buffers = &rx_buff;
		rx_bufs.count = 1;

		err = spi_transceive(spi_flash, &spi_cfg, NULL, &rx_bufs);
		if(err)
		{
		#ifdef FLASH_DEBUG
			LOGD("SPI error: %d", err);
		#endif
		}
		
		pBuffer += read_size;
	}

	SpiFlash_CS_HIGH();
	
    return true;
}


/*****************************************************************************
** ��  ����������������W25Q64FW�Ĺܽ�,�ر�ע��д�Ĺ�����CSҪһֱ��Ч�����ܽ���SPI�Զ�����
** ��  �Σ���
** ����ֵ����
******************************************************************************/
void SPI_Flash_Init(void)
{
	spi_flash = device_get_binding(FLASH_DEVICE);
	if (!spi_flash) 
	{
	#ifdef FLASH_DEBUG
		LOGD("Could not get %s device", FLASH_DEVICE);
	#endif
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	spi_cfg.frequency = 8000000;
	spi_cfg.slave = 0;
}

void flash_init(void)
{
#ifdef FLASH_DEBUG
	LOGD("flash_init");
#endif

	gpio_flash = device_get_binding(FLASH_PORT);
	if(!gpio_flash)
	{
	#ifdef FLASH_DEBUG
		LOGD("Cannot bind gpio device");
	#endif
		return;
	}

	gpio_pin_configure(gpio_flash, FLASH_CS_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_flash, FLASH_CS_PIN, 1);

	SPI_Flash_Init();
}

u8_t strbuf[4096] = {0};

void test_flash_write_and_read(u8_t *buf, u32_t len)
{
	static s32_t last_index = -1;
	s32_t cur_index;
	u32_t PageByteRemain,addr=0;
	u32_t date_len = len;
		
	LOGD("len:%d", len);
	
	addr = IMG_START_ADDR;
#if 1
	cur_index = addr/SPIFlash_SECTOR_SIZE;
	if(cur_index > last_index)
	{
		last_index = cur_index;
		SPIFlash_Erase_Sector(last_index*SPIFlash_SECTOR_SIZE);
		PageByteRemain = SPIFlash_SECTOR_SIZE - addr%SPIFlash_SECTOR_SIZE;
		if(PageByteRemain < len)
		{
			len -= PageByteRemain;
			while(1)
			{
				SPIFlash_Erase_Sector((++last_index)*SPIFlash_SECTOR_SIZE);
				if(len >= SPIFlash_SECTOR_SIZE)
					len -= SPIFlash_SECTOR_SIZE;
				else
					break;
			}
		}
	}
#endif

	SpiFlash_Write_Buf(buf, IMG_PEPPA_80X160_ADDR, date_len);
	SpiFlash_Read(strbuf, IMG_PEPPA_80X160_ADDR, 4096);	
	LCD_ShowImg_From_Flash(0, 0, addr);
}

void test_flash(void)
{
	uint16_t flash_id;
	uint16_t len;
	u8_t tmpbuf[128] = {0};
	
	//flash_init();

	LCD_ShowString(0,0,"FLASH���Կ�ʼ");

	flash_id = SpiFlash_ReadID();
	sprintf(tmpbuf, "FLASH ID:%X", flash_id);
	LCD_ShowString(0,20,tmpbuf);

#if 1
	//д֮ǰ��Ҫ��ִ�в�������
	LCD_ShowString(0,40,"FLASH��ʼ����...");
	SPIFlash_Erase_Chip();
	//SPIFlash_Erase_Sector(0);
	LCD_ShowString(0,60,"FLASH�����ɹ�!");
#endif

#if 0	
	//д������
	//LCD_ShowString(0,80,"FLASHд��ͼƬ1����...");
	//SpiFlash_Write_Buf(peppa_pig_80X160, IMG_PEPPA_80X160_ADDR, IMG_PEPPA_80X160_SIZE);
	//LCD_ShowString(0,100,"FLASHд��ͼƬ1�ɹ�!");

	//д������
	//LCD_ShowString(0,80,"FLASHд��ͼƬ2����...");
	//SpiFlash_Write_Buf(peppa_pig_160X160, IMG_PEPPA_160X160_ADDR, IMG_PEPPA_160X160_SIZE);
	//LCD_ShowString(0,100,"FLASHд��ͼƬ2�ɹ�!");

	//д������
	//LCD_ShowString(0,80,"FLASHд��ͼƬ3����...");
	//SpiFlash_Write_Buf(peppa_pig_240X240_1, IMG_PEPPA_240X240_ADDR, 57608);
	//SpiFlash_Write_Buf(peppa_pig_240X240_2, IMG_PEPPA_240X240_ADDR+57608, 57600);
	//LCD_ShowString(0,100,"FLASHд��ͼƬ3�ɹ�!");

	//д������
	//LCD_ShowString(0,80,"FLASHд��ͼƬ4����...");
	//SpiFlash_Write_Buf(peppa_pig_320X320_1, IMG_PEPPA_320X320_ADDR, 51208);
	//SpiFlash_Write_Buf(peppa_pig_320X320_2, IMG_PEPPA_320X320_ADDR+51208, 51200);
	//SpiFlash_Write_Buf(peppa_pig_320X320_3, IMG_PEPPA_320X320_ADDR+51208+51200, 51200);
	//SpiFlash_Write_Buf(peppa_pig_320X320_4, IMG_PEPPA_320X320_ADDR+51208+2*51200, 51200);
	//LCD_ShowString(0,100,"FLASHд��ͼƬ4�ɹ�!");

	//д������
	//LCD_ShowString(0,80,"FLASHд��RM_LOGOͼƬ����...");
	//SpiFlash_Write_Buf(RM_LOGO_240X240_1, IMG_RM_LOGO_240X240_ADDR, 57608);
	//SpiFlash_Write_Buf(RM_LOGO_240X240_2, IMG_RM_LOGO_240X240_ADDR+57608, 57600);
	//LCD_ShowString(0,100,"FLASHд��RM_LOGOͼƬ�ɹ�!");	
#endif

#if 0
	//д������
	LCD_ShowString(0,80,"FLASHд��16X08Ӣ���ֿ�...");
	SpiFlash_Write_Buf(asc2_1608, FONT_ASC_1608_ADDR, FONT_ASC_1608_SIZE);
	LCD_ShowString(0,100,"FLASHд��16X08Ӣ�ĳɹ�");

	//д������
	LCD_ShowString(0,120,"FLASHд��24X12Ӣ���ֿ�...");
	SpiFlash_Write_Buf(asc2_2412, FONT_ASC_2412_ADDR, FONT_ASC_2412_SIZE);
	LCD_ShowString(0,140,"FLASHд��24X12Ӣ�ĳɹ�");

	//д������
	LCD_ShowString(0,160,"FLASHд��32X16Ӣ���ֿ�...");
	SpiFlash_Write_Buf(asc2_3216, FONT_ASC_3216_ADDR, FONT_ASC_3216_SIZE);
	LCD_ShowString(0,180,"FLASHд��32X16Ӣ�ĳɹ�");
#endif

#if 0
	//д������
	//LCD_ShowString(0,80,"FLASHд��16X16�����ֿ�...");
	//SpiFlash_Write_Buf(chinese_1616_1, FONT_CHN_SM_1616_ADDR+(2726*32)*0, (2726*32));
	//SpiFlash_Write_Buf(chinese_1616_2, FONT_CHN_SM_1616_ADDR+(2726*32)*1, (2726*32));
	//SpiFlash_Write_Buf(chinese_1616_3, FONT_CHN_SM_1616_ADDR+(2726*32)*2, (2726*32));	
	//LCD_ShowString(0,100,"FLASHд��16X16���ĳɹ�");
#endif

#if 0
	//д������
	//LCD_ShowString(0,80,"FLASHд��24X24�����ֿ�...");
	//SpiFlash_Write_Buf(chinese_2424_1, FONT_CHN_SM_2424_ADDR+(1200*72)*0, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_2, FONT_CHN_SM_2424_ADDR+(1200*72)*1, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_3, FONT_CHN_SM_2424_ADDR+(1200*72)*2, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_4, FONT_CHN_SM_2424_ADDR+(1200*72)*3, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_5, FONT_CHN_SM_2424_ADDR+(1200*72)*4, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_6, FONT_CHN_SM_2424_ADDR+(1200*72)*5, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_7, FONT_CHN_SM_2424_ADDR+(1200*72)*6, (977*72));
	//LCD_ShowString(0,100,"FLASHд��24X24���ĳɹ�");
#endif

#if 0
	//д������
	//LCD_ShowString(0,80,"FLASHд��32X32�����ֿ�...");
	//SpiFlash_Write_Buf(chinese_3232_1, FONT_CHN_SM_3232_ADDR+(700*128)*0, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_2, FONT_CHN_SM_3232_ADDR+(700*128)*1, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_3, FONT_CHN_SM_3232_ADDR+(700*128)*2, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_4, FONT_CHN_SM_3232_ADDR+(700*128)*3, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_5, FONT_CHN_SM_3232_ADDR+(700*128)*4, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_6, FONT_CHN_SM_3232_ADDR+(700*128)*5, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_7, FONT_CHN_SM_3232_ADDR+(700*128)*6, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_8, FONT_CHN_SM_3232_ADDR+(700*128)*7, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_9, FONT_CHN_SM_3232_ADDR+(700*128)*8, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_10, FONT_CHN_SM_3232_ADDR+(700*128)*9, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_11, FONT_CHN_SM_3232_ADDR+(700*128)*10, (700*128));
	//SpiFlash_Write_Buf(chinese_3232_12, FONT_CHN_SM_3232_ADDR+(700*128)*11, (478*128));
	//LCD_ShowString(0,100,"FLASHд��32X32���ĳɹ�");	
#endif

#if 0
	//д������
	LCD_ShowString(0,80,"FLASHд��RM16X08Ӣ���ֿ�...");
	SpiFlash_Write_Buf(asc2_16_rm, FONT_RM_ASC_16_ADDR, FONT_RM_ASC_16_SIZE);
	LCD_ShowString(0,100,"FLASHд��RM16X08Ӣ�ĳɹ�");	
#endif

#if 0
	//д������
	LCD_ShowString(0,80,"FLASHд��RM16X16�����ֿ�...");
	//SpiFlash_Write_Buf(RM_JIS_16_1, FONT_RM_JIS_16_ADDR+72192*0, 72192);
	//SpiFlash_Write_Buf(RM_JIS_16_2, FONT_RM_JIS_16_ADDR+72192*1, 72192);
	//SpiFlash_Write_Buf(RM_JIS_16_3, FONT_RM_JIS_16_ADDR+72192*2, 72192);	
	//SpiFlash_Write_Buf(RM_JIS_16_4, FONT_RM_JIS_16_ADDR+72192*3, 72192);
	SpiFlash_Write_Buf(RM_JIS_16_5, FONT_RM_JIS_16_ADDR+72192*4, 72208);
	LCD_ShowString(0,100,"FLASHд��RM16X16���ĳɹ�");
#endif

#if 0
	//д������
	LCD_ShowString(0,80,"FLASHд��RM16X16�����ֿ�...");
	//SpiFlash_Write_Buf(RM_UNI_16_1, FONT_RM_UNI_16_ADDR+88288*0, 88288);
	//SpiFlash_Write_Buf(RM_UNI_16_2, FONT_RM_UNI_16_ADDR+88288*1, 88288);
	//SpiFlash_Write_Buf(RM_UNI_16_3, FONT_RM_UNI_16_ADDR+88288*2, 88288);	
	//SpiFlash_Write_Buf(RM_UNI_16_4, FONT_RM_UNI_16_ADDR+88288*3, 88288);
	//SpiFlash_Write_Buf(RM_UNI_16_5, FONT_RM_UNI_16_ADDR+88288*4, 88288);
	//SpiFlash_Write_Buf(RM_UNI_16_6, FONT_RM_UNI_16_ADDR+88288*5, 88288);
	SpiFlash_Write_Buf(RM_UNI_16_7, FONT_RM_UNI_16_ADDR+88288*6, 88224);
	LCD_ShowString(0,100,"FLASHд��RM16X16���ĳɹ�");
#endif

#if 0
	//д������
	//LCD_ShowString(0,80,"FLASHд��RM24X24�����ֿ�...");
	//SpiFlash_Write_Buf(RM_UNI_24_1, FONT_RM_UNI_24_ADDR+89600*0, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_2, FONT_RM_UNI_24_ADDR+89600*1, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_3, FONT_RM_UNI_24_ADDR+89600*2, 89600);	
	//SpiFlash_Write_Buf(RM_UNI_24_4, FONT_RM_UNI_24_ADDR+89600*3, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_5, FONT_RM_UNI_24_ADDR+89600*4, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_6, FONT_RM_UNI_24_ADDR+89600*5, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_7, FONT_RM_UNI_24_ADDR+89600*6, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_8, FONT_RM_UNI_24_ADDR+89600*7, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_9, FONT_RM_UNI_24_ADDR+89600*8, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_10, FONT_RM_UNI_24_ADDR+89600*9, 89600);
	//SpiFlash_Write_Buf(RM_UNI_24_11, FONT_RM_UNI_24_ADDR+89600*10, 80388);
	//LCD_ShowString(0,100,"FLASHд��RM24X24���ĳɹ�");
#endif

#if 0
	//д��ppg(max32674�����㷨)
	LCD_ShowString(0,80,"FLASHд��PPG�㷨...");
	//SpiFlash_Write_Buf(Msbl_674_V1_1_0_001, PPG_ALGO_FW_ADDR+72000*0, 72000);
	//SpiFlash_Write_Buf(Msbl_674_V1_1_0_002, PPG_ALGO_FW_ADDR+72000*1, 72000);
	//SpiFlash_Write_Buf(Msbl_674_V1_1_0_003, PPG_ALGO_FW_ADDR+72000*2, 72000);
	SpiFlash_Write_Buf(Msbl_674_V1_1_0_004, PPG_ALGO_FW_ADDR+72000*3, 63152);
	LCD_ShowString(0,100,"FLASHд��PPG�㷨�ɹ�");
#endif
	//��������
	//SpiFlash_Read(my_rx_buf,0,len);
	//LCD_ShowString(0,140,"FLASH��������:");
	//LCD_ShowString(0,160,my_rx_buf);

#if 0
	test_flash_write_and_read(peppa_pig_80X160, 25608);
#endif
}