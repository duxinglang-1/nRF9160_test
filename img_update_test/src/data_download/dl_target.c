/****************************************Copyright (c)************************************************
** File Name:			    dl_target.c
** Descriptions:			data to target process source file
** Created By:				xie biao
** Created Date:			2021-12-29
** Modified Date:      		2021-12-29 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include "dl_target.h"
#include "logger.h"

#define DEF_DL_TARGET(name) \
static const struct dl_target dl_target_ ## name  = { \
	.init = dl_target_ ## name ## _init, \
	.offset_get = dl_target_## name ##_offset_get, \
	.write = dl_target_ ## name ## _write, \
	.done = dl_target_ ## name ## _done, \
}

#ifdef CONFIG_DL_TARGET_UI
#include "dl_target_ui.h"
DEF_DL_TARGET(ui);
#endif
#ifdef CONFIG_DFU_TARGET_FONT
#include "dl_target_font.h"
DEF_DL_TARGET(font);
#endif

#define MIN_SIZE_IDENTIFY_BUF 32

static const struct dl_target *cur_target;

int dl_target_img_type(const void *const buf, size_t len)
{
#ifdef CONFIG_DL_TARGET_UI
	if(dl_target_ui_identify(buf))
	{
		return DL_TARGET_IMAGE_TYPE_UI;
	}
#endif
#ifdef CONFIG_DL_TARGET_FONT
	if(dl_target_font_identify(buf))
	{
		return DL_TARGET_IMAGE_TYPE_FONT;
	}
#endif

	if(len < MIN_SIZE_IDENTIFY_BUF)
	{
		return -EAGAIN;
	}

	LOGD("No supported image type found");
	return -ENOTSUP;
}

int dl_target_init(int img_type, size_t file_size, dl_target_callback_t cb)
{
	const struct dl_target *new_target = NULL;

#ifdef CONFIG_DL_TARGET_UI
	if(img_type == DL_TARGET_IMAGE_TYPE_UI)
	{
		new_target = &dl_target_ui;
	}
#endif
#ifdef CONFIG_DL_TARGET_FONT
	if(img_type == DL_TARGET_IMAGE_TYPE_FONT)
	{
		new_target = &dl_target_font;
	}
#endif

	if(new_target == NULL)
	{
		LOGD("Unknown image type");
		return -ENOTSUP;
	}

	/* The user is re-initializing with an previously aborted target.
	 * Avoid re-initializing generally to ensure that the download can
	 * continue where it left off. Re-initializing is required for modem
	 * upgrades to re-open the DFU socket that is closed on abort.
	 */
	if(new_target == cur_target
	   && img_type != DL_TARGET_IMAGE_TYPE_FONT)
	{
		return 0;
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
		LOGD("Unable to clean up dfu_target");
		return err;
	}

	if(successful)
	{
		cur_target = NULL;
	}

	return 0;
}

