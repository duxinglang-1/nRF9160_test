/****************************************Copyright (c)************************************************
** File name:			external_flash.c
** Last modified Date:          
** Last Version:		V1.0   
** Created by:			谢彪
** Created date:		2020-06-30
** Version:			    1.0
** Descriptions:		外挂flash驱动源文件
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

struct device *spi_flash;
struct device *gpio_flash;

//SPI发送缓存数组，使用EasyDMA时一定要定义为static类型
static uint8_t    spi_tx_buf[6] = {0};  
//SPI接收缓存数组，使用EasyDMA时一定要定义为static类型
static uint8_t    spi_rx_buf[6] = {0};  

//SPI发送缓存数组，使用EasyDMA时一定要定义为static类型
//static uint8_t    my_tx_buf[4096] = {0};
//SPI发送缓存数组，使用EasyDMA时一定要定义为static类型
//static uint8_t    my_rx_buf[4096] = {0};

static struct spi_buf_set tx_bufs,rx_bufs;
static struct spi_buf tx_buff,rx_buff;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

void SpiFlash_CS_LOW(void)
{
	gpio_pin_write(gpio_flash, CS, 0);
}

void SpiFlash_CS_HIGH(void)
{
	gpio_pin_write(gpio_flash, CS, 1);
}

/*****************************************************************************
** 描  述：写入一个字节
** 参  数：Dat：待写入的数据
** 返回值：无
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
		printk("SPI error: %d\n", err);
	}
	else
	{
		printk("ok\n");
	}
}
/*****************************************************************************
** 描  述：写使能
** 参  数：无
** 返回值：无
******************************************************************************/
static void SpiFlash_Write_Enable(void)
{
	Spi_WriteOneByte(SPIFlash_WriteEnable);
}
/*****************************************************************************
** 描  述：读取W25Q64FW芯片ID
** 参  数：无
** 返回值：16位ID，W25Q64FW芯片ID为：0xEF16
******************************************************************************/
uint16_t SpiFlash_ReadID(void)
{
	int err;
	uint16_t dat = 0;

	//准备数据
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
		printk("SPI error: %d\n", err);
	}
	else
	{
		printk("TX sent: %x,%x,%x,%x,%x,%x\n", 
			spi_tx_buf[0],
			spi_tx_buf[1],
			spi_tx_buf[2],
			spi_tx_buf[3],
			spi_tx_buf[4],
			spi_tx_buf[5]
			);
		
		printk("RX recv: %x,%x,%x,%x,%x,%x\n", 
			spi_rx_buf[0],
			spi_rx_buf[1],
			spi_rx_buf[2],
			spi_rx_buf[3],
			spi_rx_buf[4],
			spi_rx_buf[5]
			);
		
		//接收数组最后两个字节才是读取的ID
		dat|=spi_rx_buf[4]<<8;  
		dat|=spi_rx_buf[5];
		
		printk("flash ID: %x\n", dat);
	}



	return dat;
}
/*****************************************************************************
** 描  述：读取W25Q64FW状态寄存器
** 参  数：无
** 返回值：
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
		printk("SPI error: %d\n", err);
	}
	else
	{
		printk("StatusReg: %x\n", spi_rx_buf[1]);
	}
	
	return spi_rx_buf[1];
}

//等待W25Q64FW就绪
void SpiFlash_Wait_Busy(void)   
{   
	while((SpiFlash_ReadSR()&0x01)==0x01);  		// 等待BUSY位清空
} 
/*****************************************************************************
** 描  述：擦除扇区，W25Q64FW最小的擦除单位是扇区
** 参  数：[in]SecAddr：扇区地址
** 返回值：无
******************************************************************************/
void SPIFlash_Erase_Sector(uint32_t SecAddr)
{
	int err;

	//发送写使能命令
	SpiFlash_Write_Enable();

	//扇区擦除命令
	spi_tx_buf[0] = SPIFlash_SecErase;		
	//24位地址
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
		printk("SPI error: %d\n", err);
	}
	
	//等待W25Q64FW完成操作
	SpiFlash_Wait_Busy();
}
/*****************************************************************************
** 描  述：全片擦除W25Q64FW，全片擦除所需的时间典型值为：40秒
** 参  数：无
** 返回值：无
******************************************************************************/
void SPIFlash_Erase_Chip(void)
{
	int err;
	
	//发送写使能命令
	SpiFlash_Write_Enable();
	
	//全片擦除命令
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
		printk("SPI error: %d\n", err);
	}

	//等待W25Q64FW完成操作
	SpiFlash_Wait_Busy();	
}
/*****************************************************************************
** 描  述：向指定的地址写入数据,最大写入的长度不能超过该地址所处页面的剩余空间
**         *pBuffer:指向待写入的数据缓存
**         WriteAddr:写入的起始地址
**         WriteBytesNum:写入的字节数，一次最多256个字节
** 返回值：RET_SUCCESS
******************************************************************************/
uint8_t SpiFlash_Write_Page(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size)
{
	int err;
	
	//检查写入的数据长度是否合法，写入长度不能超过页面的大小
	if (size > (SPIFlash_PAGE_SIZE - (WriteAddr%SPIFlash_PAGE_SIZE)))
	{
		return false;
	}

	if(size == 0) 
		return false;

	//发送写使能命令
	SpiFlash_Write_Enable();
	
	//页编程命令
	spi_tx_buf[0] = SPIFlash_PageProgram;
	//24位地址，高地址在前
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
		printk("SPI error: %d\n", err);
	}

	tx_buff.buf = pBuffer;
	tx_buff.len = size;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	err = spi_transceive(spi_flash, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
		printk("SPI error: %d\n", err);
	}

	SpiFlash_CS_HIGH();
	
	//等待W25Q64FW完成操作
	SpiFlash_Wait_Busy();

	return true;
}
/*****************************************************************************
** 描  述：向指定的地址写入数据，可写入多个页
**         *pBuffer:指向待写入的数据
**         WriteAddr:写入的起始地址
**         size:写入的字节数
** 返回值：RET_SUCCESS
******************************************************************************/
uint8_t SpiFlash_Write_Buf(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t size)
{
    uint32_t PageByteRemain = 0;
	
	//计算起始地址所处页面的剩余空间
    PageByteRemain = SPIFlash_PAGE_SIZE - WriteAddr%SPIFlash_PAGE_SIZE;
	//如果编程的数据长度不大于页面的剩余空间，编程数据长度等于size
    if(size <= PageByteRemain)
    {
        PageByteRemain = size;
    }
	//分次编程，直到所有的数据编程完成
    while(true)
    {
        //编程PageByteRemain个字节
		SpiFlash_Write_Page(pBuffer,WriteAddr,PageByteRemain);
		//如果编程完成，退出循环
        if(size == PageByteRemain)
        {
            break;
        }
        else
        {
            //计算编程取数据的缓存地址
			pBuffer += PageByteRemain;
			//计算编程地址
            WriteAddr += PageByteRemain;
			//数据长度减去PageByteRemain
            size -= PageByteRemain;
			//计算下次编程的数据长度
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
** 描  述：从指定的地址读出指定长度的数据
** 参  数：pBuffer：指向存放读出数据的首地址       
**         ReadAddr：待读出数据的起始地址
**         size：读出的字节数，注意size不能超过pBuffer的大小，否则数组会溢出
** 返回值：
******************************************************************************/
uint8_t SpiFlash_Read(uint8_t *pBuffer,uint32_t ReadAddr,uint32_t size)
{
	int err;
	uint8_t read_size;
	
	spi_tx_buf[0] = SPIFlash_ReadData;
	//24位地址，高地址在前
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
		printk("SPI error: %d\n", err);
	}

	//开始读取数据
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
			printk("SPI error: %d\n", err);
		}
		
		pBuffer += read_size;
	}

	SpiFlash_CS_HIGH();
	
    return true;
}


/*****************************************************************************
** 描  述：配置用于驱动W25Q64FW的管脚,特别注意写的过程中CS要一直有效，不能交给SPI自动控制
** 入  参：无
** 返回值：无
******************************************************************************/
void SPI_Flash_Init(void)
{
	spi_flash = device_get_binding(FLASH_DEVICE);
	if (!spi_flash) 
	{
		printk("Could not get %s device\n", FLASH_DEVICE);
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	spi_cfg.frequency = 4000000;
	spi_cfg.slave = 0;
}

void flash_init(void)
{
	printk("flash_init\n");
		
	gpio_flash = device_get_binding(FLASH_PORT);
	if(!gpio_flash)
	{
		printk("Cannot bind gpio device\n");
		return;
	}

	gpio_pin_configure(gpio_flash, CS, GPIO_DIR_OUT);

	SPI_Flash_Init();
}

void test_flash(void)
{
	uint16_t flash_id;
	uint16_t len;
	u8_t tmpbuf[128] = {0};
	
	//flash_init();


	LCD_ShowString(0,0,"FLASH测试开始");

	flash_id = SpiFlash_ReadID();
	sprintf(tmpbuf, "FLASH ID:%X", flash_id);
	LCD_ShowString(0,20,tmpbuf);

#if 0
	//写之前需要先执行擦除操作
	LCD_ShowString(0,40,"FLASH开始擦除...");
	SPIFlash_Erase_Chip();
	//SPIFlash_Erase_Sector(0);
	LCD_ShowString(0,60,"FLASH擦除成功!");
#endif

#if 0	
	//写入数据
	//LCD_ShowString(0,80,"FLASH写入图片1数据...");
	//SpiFlash_Write_Buf(peppa_pig_80X160, IMG_PEPPA_80X160_ADDR, IMG_PEPPA_80X160_SIZE);
	//LCD_ShowString(0,100,"FLASH写入图片1成功!");

	//写入数据
	//LCD_ShowString(0,120,"FLASH写入图片2数据...");
	//SpiFlash_Write_Buf(peppa_pig_160X160, IMG_PEPPA_160X160_ADDR, IMG_PEPPA_160X160_SIZE);
	//LCD_ShowString(0,140,"FLASH写入图片2成功!");

	//写入数据
	LCD_ShowString(0,160,"FLASH写入图片3数据...");
	//SpiFlash_Write_Buf(peppa_pig_240X240_1, IMG_PEPPA_240X240_ADDR, 57608);
	SpiFlash_Write_Buf(peppa_pig_240X240_2, IMG_PEPPA_240X240_ADDR+57608, 57600);
	LCD_ShowString(0,180,"FLASH写入图片3成功!");

	//写入数据
	//LCD_ShowString(0,200,"FLASH写入图片4数据...");
	//SpiFlash_Write_Buf(peppa_pig_320X320, IMG_PEPPA_320X320_ADDR, IMG_PEPPA_320X320_SIZE);
	//LCD_ShowString(0,220,"FLASH写入图片4成功!");
#endif

#if 0
	//写入数据
	LCD_ShowString(0,160,"FLASH写入16X08英文字库...");
	SpiFlash_Write_Buf(asc2_1608, FONT_ASC_1608_ADDR, FONT_ASC_1608_SIZE);
	LCD_ShowString(0,180,"FLASH写入16X08英文成功");

	//写入数据
	LCD_ShowString(0,40,"FLASH写入24X12英文字库...");
	SpiFlash_Write_Buf(asc2_2412, FONT_ASC_2412_ADDR, FONT_ASC_2412_SIZE);
	LCD_ShowString(0,60,"FLASH写入24X12英文成功");

	//写入数据
	LCD_ShowString(0,80,"FLASH写入32X16英文字库...");
	SpiFlash_Write_Buf(asc2_3216, FONT_ASC_3216_ADDR, FONT_ASC_3216_SIZE);
	LCD_ShowString(0,100,"FLASH写入32X16英文成功");
#endif

#if 0
	//写入数据
	LCD_ShowString(0,120,"FLASH写入16X16中文字库...");
	//SpiFlash_Write_Buf(chinese_1616_1, FONT_CHN_SM_1616_ADDR+(2726*32)*0, (2726*32));
	//SpiFlash_Write_Buf(chinese_1616_2, FONT_CHN_SM_1616_ADDR+(2726*32)*1, (2726*32));
	SpiFlash_Write_Buf(chinese_1616_3, FONT_CHN_SM_1616_ADDR+(2726*32)*2, (2726*32));	
	LCD_ShowString(0,140,"FLASH写入16X16中文成功");
#endif

#if 0
	//写入数据
	LCD_ShowString(0,160,"FLASH写入24X24中文字库...");
	//SpiFlash_Write_Buf(chinese_2424_1, FONT_CHN_SM_2424_ADDR+(1200*72)*0, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_2, FONT_CHN_SM_2424_ADDR+(1200*72)*1, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_3, FONT_CHN_SM_2424_ADDR+(1200*72)*2, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_4, FONT_CHN_SM_2424_ADDR+(1200*72)*3, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_5, FONT_CHN_SM_2424_ADDR+(1200*72)*4, (1200*72));
	//SpiFlash_Write_Buf(chinese_2424_6, FONT_CHN_SM_2424_ADDR+(1200*72)*5, (1200*72));
	SpiFlash_Write_Buf(chinese_2424_7, FONT_CHN_SM_2424_ADDR+(1200*72)*6, (977*72));
	LCD_ShowString(0,180,"FLASH写入24X24中文成功");
#endif

#if 0
	//写入数据
	LCD_ShowString(0,200,"FLASH写入32X32中文字库...");
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
	SpiFlash_Write_Buf(chinese_3232_12, FONT_CHN_SM_3232_ADDR+(700*128)*11, (478*128));
	LCD_ShowString(0,220,"FLASH写入32X32中文成功");	
#endif
	
	//读出数据
	//SpiFlash_Read(my_rx_buf,0,len);
	//LCD_ShowString(0,140,"FLASH读出数据:");
	//LCD_ShowString(0,160,my_rx_buf);
}
