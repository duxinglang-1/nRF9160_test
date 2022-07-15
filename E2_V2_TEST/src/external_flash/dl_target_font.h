/****************************************Copyright (c)************************************************
** File Name:			    dl_target_font.h
** Descriptions:			data download for font process head file
** Created By:				xie biao
** Created Date:			2021-12-28
** Modified Date:      		2021-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __DL_TARGET_FONT_H__
#define __DL_TARGET_FONT_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool dl_target_font_identify(const void *const buf);
extern int dl_target_font_init(size_t file_size, dl_target_callback_t cb);
extern int dl_target_font_offset_get(size_t *offset);
extern int dl_target_font_write(const void *const buf, size_t len);
extern int dl_target_font_done(bool successful);

#endif/*__DL_TARGET_FONT_H__*/
