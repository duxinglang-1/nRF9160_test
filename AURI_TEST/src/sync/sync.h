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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	SYNC_STATUS_IDLE,
	SYNC_STATUS_SENDING,
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
