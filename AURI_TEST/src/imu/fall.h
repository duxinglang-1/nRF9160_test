/****************************************Copyright (c)************************************************
** File Name:			    fall.h
** Descriptions:			fall message process head file
** Created By:				xie biao
** Created Date:			2021-09-24
** Modified Date:      		2021-09-24 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __FALL_H__
#define __FALL_H__

#include <zephyr/types.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FALL_NOTIFY_TIMEOUT 	14
#define FALL_SENDING_TIMEOUT 	5
#define FALL_SEND_OK_TIMEOUT	2
#define FALL_CANCEL_TIMEOUT 	2


typedef enum
{
	FALL_STATUS_IDLE,
	FALL_STATUS_NOTIFY,
	FALL_STATUS_SENDING,
	FALL_STATUS_SENT,
	FALL_STATUS_CANCEL,
	FALL_STATUS_CANCELED,
	FALL_STATUS_MAX
}FALL_STATUS;

extern FALL_STATUS fall_state;
extern u8_t fall_trigger_time[16];

extern void FallStart(void);
extern void FallAlarmStart(void);
extern void FallAlarmCancel(void);
extern bool FallIsRunning(void);

#ifdef __cplusplus
}
#endif

#endif/*__FALL_H__*/
