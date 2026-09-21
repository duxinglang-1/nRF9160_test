/****************************************Copyright (c)************************************************
** File Name:			    dl_target_ppg.h
** Descriptions:			data download for ppg process head file
** Created By:				xie biao
** Created Date:			2021-12-28
** Modified Date:      		2021-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __DL_TARGET_PPG_H__
#define __DL_TARGET_PPG_H__

#ifdef CONFIG_PPG_SUPPORT
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool dl_target_ppg_identify(const void *const buf);
extern int dl_target_ppg_init(size_t file_size, dl_target_callback_t cb);
extern int dl_target_ppg_offset_get(size_t *offset);
extern int dl_target_ppg_write(const void *const buf, size_t len);
extern int dl_target_ppg_done(bool successful);

#endif/*CONFIG_PPG_SUPPORT*/

#endif/*__DL_TARGET_PPG_H__*/
