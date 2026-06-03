/****************************************Copyright (c)************************************************
** File Name:				img.c
** Descriptions:			img source file
** Created By:				xie biao
** Created Date:			2020-01-02
** Modified Date:			2026-04-15 
** Version: 				V2.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include "external_flash.h"
#include "lcd.h"
#include "settings.h"
#include "img.h"

imglib_infor_t imglib_infor = {0};

uint32_t GetImageDataFromFlashByID(RES_IMG_ID img_id)
{
	uint8_t data[2] = {0};
	dataindex *ptr_index;
	uint32_t i,img_addr;

	if(img_id >= IMG_ID_MAX)
		return 0;

	if(imglib_infor.img_count == 0)
	{
		uint16_t img_num;
		
		SpiFlash_Read(data, IMG_DATA_ADDR, 2);
		img_num = (data[1]<<8) + data[0];
		if((img_num == 0x0000)||(img_num == 0xFFFF))
			return 0;

		imglib_infor.img_count = img_num;
		imglib_infor.index_len = imglib_infor.img_count*sizeof(dataindex);
	}

	SpiFlash_Read((uint8_t*)&imglib_infor.index, (IMG_DATA_ADDR+2+img_id*sizeof(dataindex)), sizeof(dataindex));
	img_addr = IMG_DATA_ADDR+2+imglib_infor.index_len+imglib_infor.index.addr;
	return img_addr;
}

void LCD_MeasureImage(RES_IMG_ID img_id, uint16_t *width, uint16_t *height)
{
	uint32_t addr;

	if(img_id >= IMG_ID_MAX)
		return;

	addr = GetImageDataFromFlashByID(img_id);
	if(addr > 0)
	{
		LCD_get_pic_size_from_flash(addr, width, height);
	}
}

void LCD_ShowImage(uint16_t x, uint16_t y, RES_IMG_ID img_id)
{
	uint32_t addr;

	if(img_id >= IMG_ID_MAX)
		return;

	addr = GetImageDataFromFlashByID(img_id);
	if(addr > 0)
	{
	#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
		LCD_dis_img_from_flash(x, y, addr);
	#else
		LCD_dis_pic_from_flash(x, y, addr);
	#endif
	}
}

void LCD_ShowImageTrans(uint16_t x, uint16_t y, uint16_t trans, RES_IMG_ID img_id)
{
	uint32_t addr;

	if(img_id >= IMG_ID_MAX)
		return;

	addr = GetImageDataFromFlashByID(img_id);
	if(addr > 0)
	{
		LCD_dis_pic_trans_from_flash(x, y, addr, trans);
	}
}

void LCD_ShowImageRotate(uint16_t x, uint16_t y, uint16_t rotate, RES_IMG_ID img_id)
{
	uint32_t addr;

	if(img_id >= IMG_ID_MAX)
		return;

	addr = GetImageDataFromFlashByID(img_id);
	if(addr > 0)
	{
		LCD_dis_pic_rotate_from_flash(x, y, addr, rotate);
	}
}

void LCD_ShowImageAngle(uint16_t x, uint16_t y, uint16_t trans, uint32_t angle, RES_IMG_ID img_id)
{
	uint32_t addr;

	if(img_id >= IMG_ID_MAX)
		return;

	addr = GetImageDataFromFlashByID(img_id);
	if(addr > 0)
	{
		LCD_dis_pic_angle_from_flash(x, y, addr, angle);
	}
}

