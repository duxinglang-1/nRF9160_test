/****************************************Copyright (c)************************************************
** File Name:			    sos.h
** Descriptions:			sos message process head file
** Created By:				xie biao
** Created Date:			2021-01-28
** Modified Date:      		2021-01-28 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __SOS_H__
#define __SOS_H__

#include <zephyr/types.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOS_SENDING_TIMEOUT 5

typedef enum
{
	SOS_STATUS_IDLE,
	SOS_STATUS_SENDING,
	SOS_STATUS_SENT,
	SOS_STATUS_RECEIVED,
	SOS_STATUS_CANCEL,
	SOS_STATUS_MAX
}SOS_STATUS;

extern SOS_STATUS sos_state;
extern bool sos_wait_gps;

extern void SOSStart(void);
extern void SOSCancel(void);


#ifdef __cplusplus
}
#endif

#endif/*__SOS_H__*/
