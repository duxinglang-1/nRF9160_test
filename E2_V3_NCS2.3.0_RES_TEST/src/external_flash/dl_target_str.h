/****************************************Copyright (c)************************************************
** File Name:			    dl_target_str.h
** Descriptions:			data download for strings process head file
** Created By:				xie biao
** Created Date:			2025-10-14
** Modified Date:      		2025-10-14 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __DL_TARGET_STR_H__
#define __DL_TARGET_STR_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool dl_target_str_identify(const void *const buf);
extern int dl_target_str_init(size_t file_size, dl_target_callback_t cb);
extern int dl_target_str_offset_get(size_t *offset);
extern int dl_target_str_write(const void *const buf, size_t len);
extern int dl_target_str_done(bool successful);

#endif/*__DL_TARGET_STR_H__*/
