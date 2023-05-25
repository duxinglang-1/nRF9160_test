/****************************************Copyright (c)************************************************
** File Name:			    dl_target.c
** Descriptions:			data to target process source file
** Created By:				xie biao
** Created Date:			2021-12-29
** Modified Date:      		2021-12-29 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include "dl_target.h"
#include "logger.h"

#define DEF_DL_TARGET(name) \
static const struct dl_target dl_target_ ## name  = { \
	.init = dl_target_ ## name ## _init, \
	.offset_get = dl_target_## name ##_offset_get, \
	.write = dl_target_ ## name ## _write, \
	.done = dl_target_ ## name ## _done, \
}

#ifdef CONFIG_IMG_DATA_UPDATE
#include "dl_target_ui.h"
DEF_DL_TARGET(ui);
#endif
#ifdef CONFIG_FONT_DATA_UPDATE
#include "dl_target_font.h"
DEF_DL_TARGET(font);
#endif
#if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
#include "dl_target_ppg.h"
DEF_DL_TARGET(ppg);
#endif

#define MIN_SIZE_IDENTIFY_BUF 32

static const struct dl_target *cur_target;


int dl_target_init(DL_DATA_TYPE data_type, size_t file_size, dl_target_callback_t cb)
{
	const struct dl_target *new_target = NULL;

	switch(data_type)
	{
	#ifdef CONFIG_IMG_DATA_UPDATE
	case DL_DATA_IMG:
		new_target = &dl_target_ui;
		break;
	#endif

	#ifdef CONFIG_FONT_DATA_UPDATE
	case DL_DATA_FONT:
		new_target = &dl_target_font;
		break;
	#endif

	#if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
	case DL_DATA_PPG:
		new_target = &dl_target_ppg;
		break;
	#endif
	}

	if(new_target == NULL)
	{
		return -ENOTSUP;
	}

	cur_target = new_target;

	return cur_target->init(file_size, cb);
}

int dl_target_offset_get(size_t *offset)
{
	if(cur_target == NULL)
	{
		return -EACCES;
	}

	return cur_target->offset_get(offset);
}

int dl_target_write(const void *const buf, size_t len)
{
	if(cur_target == NULL || buf == NULL)
	{
		return -EACCES;
	}

	return cur_target->write(buf, len);
}

int dl_target_done(bool successful)
{
	int err;

	if(cur_target == NULL)
	{
		return -EACCES;
	}

	err = cur_target->done(successful);
	if(err != 0)
	{
		return err;
	}

	if(successful)
	{
		cur_target = NULL;
	}

	return 0;
}

