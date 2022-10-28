/****************************************Copyright (c)************************************************
** File Name:			    dl_target_font.c
** Descriptions:			data download for font process source file
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
#define FONT_HEADER_MAGIC 0x96f3b83d
	
static uint32_t rece_count = 0;

bool dl_target_font_identify(const void *const buf)
{
	//return *((const u32_t *)buf) == FONT_HEADER_MAGIC;
	return true;
}

int dl_target_font_init(size_t file_size, dl_target_callback_t cb)
{
	rece_count = 0;
	return 0;
}

int dl_target_font_offset_get(size_t *out)
{
	*out = rece_count;
	return 0;
}

int dl_target_font_write(const void *const buf, size_t len)
{
	static int32_t last_index = -1;
	int32_t cur_index;
	uint32_t PageByteRemain,addr,datalen=len;
	
	addr = FONT_DATA_ADDR+rece_count;
	SpiFlash_Write_Buf(buf, addr, len);
	rece_count += len;
	return 0;
}

int dl_target_font_done(bool successful)
{
	int err = 0;

	if(successful)
	{
		LOGD("font data upgrade scheduled. Reset the device to apply");
	}
	else
	{
		LOGD("ui data upgrade aborted.");
	}

	return err;
}

