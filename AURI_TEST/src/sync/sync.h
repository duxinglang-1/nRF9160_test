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
#define SYNC_ICON_W	59
#define SYNC_ICON_H	32
#define SYNC_ICON_X	23
#define SYNC_ICON_Y	0
#define SYNC_TEXT_W	96
#define SYNC_TEXT_H	32
#define SYNC_TEXT_X	((LCD_WIDTH-SYNC_TEXT_W)/2)
#define SYNC_TEXT_Y	32
#define SYNC_ANIMA_W	21
#define SYNC_ANIMA_H	8
#define SYNC_ANIMA_X	((LCD_WIDTH-SYNC_ANIMA_W)/2)
#define SYNC_ANIMA_Y	56

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
