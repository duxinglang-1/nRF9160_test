/****************************************Copyright (c)************************************************
** File Name:			    sync.h
** Descriptions:			data synchronism process head file
** Created By:				xie biao
** Created Date:			2021-12-16
** Modified Date:      		2021-12-16 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __SYNC_H__
#define __SYNC_H__

#include <zephyr/types.h>
#include <sys/slist.h>
#include "lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

//synchronism data
#define SYNC_ICON_W					100
#define SYNC_ICON_H					104
#define SYNC_ICON_X					((LCD_WIDTH-SYNC_ICON_W)/2)
#define SYNC_ICON_Y					10
#define SYNC_STR_W					92
#define SYNC_STR_H					45
#define SYNC_STR_X					((LCD_WIDTH-SYNC_STR_W)/2)
#define SYNC_STR_Y					133
#define SYNC_RUNNING_ANI_W			81
#define SYNC_RUNNING_ANI_H			13
#define SYNC_RUNNING_ANI_X			((LCD_WIDTH-SYNC_RUNNING_ANI_W)/2)
#define SYNC_RUNNING_ANI_Y			212

typedef enum
{
	SYNC_STATUS_IDLE,
	SYNC_STATUS_LINKING,	
	SYNC_STATUS_SENT,
	SYNC_STATUS_FAIL,
	SYNC_STATUS_MAX
}SYNC_STATUS;

extern SYNC_STATUS sync_state;

extern void SyncDataStart(void);
extern void SyncDataStop(void);

#ifdef __cplusplus
}
#endif

#endif/*__SYNC_H__*/
