/****************************************Copyright (c)************************************************
** File Name:			    dl_target_ppg.c
** Descriptions:			data download for ppg process source file
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
#define PPG_HEADER_MAGIC 0x96f3b83d

static u32_t rece_count = 0;

bool dl_target_ppg_identify(const void *const buf)
{
	LOGD("begin");

	//return *((const u32_t *)buf) == UI_HEADER_MAGIC;
	return true;
}

int dl_target_ppg_init(size_t file_size, dl_target_callback_t cb)
{
	LOGD("begin");

	rece_count = 0;
	return 0;
}

int dl_target_ppg_offset_get(size_t *out)
{
	LOGD("begin");
		
	*out = rece_count;
	return 0;
}

int dl_target_ppg_write(const void *const buf, size_t len)
{
	static s32_t last_index = -1;
	s32_t cur_index;
	u32_t PageByteRemain,addr=0;
	
	LOGD("rece_count:%d, len:%d", rece_count, len);

	addr = PPG_ALGO_FW_ADDR+rece_count;
	
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
	
	SpiFlash_Write_Buf(buf, (PPG_ALGO_FW_ADDR+rece_count), len);

	rece_count += len;
	return 0;
}

int dl_target_ppg_done(bool successful)
{
	int err = 0;

	if(successful)
	{
		LOGD("ppg data upgrade scheduled. Reset the device to apply");
	}
	else
	{
		LOGD("ppg date upgrade aborted.");
	}

	return err;
}
