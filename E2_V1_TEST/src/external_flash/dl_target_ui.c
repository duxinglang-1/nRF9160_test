/****************************************Copyright (c)************************************************
** File Name:			    dl_target_ui.c
** Descriptions:			data download for ui process source file
** Created By:				xie biao
** Created Date:			2021-12-28
** Modified Date:      		2021-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#include <string.h>
#include <zephyr.h>
#include <drivers/flash.h>
#include <pm_config.h>
#include "external_flash.h"
#include "dl_target.h"
#include "logger.h"

#define MAX_FILE_SEARCH_LEN 500
#define UI_HEADER_MAGIC 0x96f3b83d

static u32_t rece_count = 0;

bool dl_target_ui_identify(const void *const buf)
{
	//return *((const u32_t *)buf) == UI_HEADER_MAGIC;
	return true;
}

int dl_target_ui_init(size_t file_size, dl_target_callback_t cb)
{
	rece_count = 0;
	return 0;
}

int dl_target_ui_offset_get(size_t *out)
{
	*out = rece_count;
	return 0;
}

int dl_target_ui_write(const void *const buf, size_t len)
{
	static s32_t last_index = -1;
	s32_t cur_index;
	u32_t PageByteRemain,addr,datalen=len;
	
	addr = IMG_DATA_ADDR+rece_count;
	cur_index = addr/SPIFlash_SECTOR_SIZE;
	if(cur_index > last_index)
	{
		last_index = cur_index;
		SPIFlash_Erase_Sector(last_index*SPIFlash_SECTOR_SIZE);
	}
	
	PageByteRemain = SPIFlash_SECTOR_SIZE - addr%SPIFlash_SECTOR_SIZE;
	if(PageByteRemain < datalen)
	{
		datalen -= PageByteRemain;
		while(1)
		{
			SPIFlash_Erase_Sector((++last_index)*SPIFlash_SECTOR_SIZE);
			if(datalen > SPIFlash_SECTOR_SIZE)
				datalen -= SPIFlash_SECTOR_SIZE;
			else
				break;
		}
	}
	
	SpiFlash_Write_Buf(buf, addr, len);
	rece_count += len;
	return 0;
}

int dl_target_ui_done(bool successful)
{
	int err = 0;

	if(successful)
	{
		LOGD("ui data upgrade scheduled. Reset the device to apply");
	}
	else
	{
		LOGD("ui data upgrade aborted.");
	}

	return err;
}
