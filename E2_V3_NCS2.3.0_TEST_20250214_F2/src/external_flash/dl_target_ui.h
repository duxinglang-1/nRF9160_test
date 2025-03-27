/****************************************Copyright (c)************************************************
** File Name:			    dl_target_ui.h
** Descriptions:			data download for ui process head file
** Created By:				xie biao
** Created Date:			2021-12-28
** Modified Date:      		2021-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __DL_TARGET_UI_H__
#define __DL_TARGET_UI_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool dl_target_ui_identify(const void *const buf);
extern int dl_target_ui_init(size_t file_size, dl_target_callback_t cb);
extern int dl_target_ui_offset_get(size_t *offset);
extern int dl_target_ui_write(const void *const buf, size_t len);
extern int dl_target_ui_done(bool successful);

#endif/*__DL_TARGET_UI_H__*/
