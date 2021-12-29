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
#include "dl_target.h"
#include "logger.h"

#define MAX_FILE_SEARCH_LEN 500
#define UI_HEADER_MAGIC 0x96f3b83d

static u32_t rece_count = 0;

bool dl_target_ui_identify(const void *const buf)
{
	LOGD("begin");

	return *((const u32_t *)buf) == UI_HEADER_MAGIC;
}

int dl_target_ui_init(size_t file_size, dl_target_callback_t cb)
{
	LOGD("begin");

	rece_count = 0;
	
	return 0;
}

int dl_target_ui_offset_get(size_t *out)
{
	LOGD("begin");
		
	*out = rece_count;
	return 0;
}

int dl_target_ui_write(const void *const buf, size_t len)
{
	LOGD("begin");

	rece_count += len;
	return 0;
}

int dl_target_ui_done(bool successful)
{
	int err = 0;

	if(successful)
	{
		LOGD("MCUBoot image upgrade scheduled. Reset the device to apply");
	}
	else
	{
		LOGD("MCUBoot image upgrade aborted.");
	}

	return err;
}
