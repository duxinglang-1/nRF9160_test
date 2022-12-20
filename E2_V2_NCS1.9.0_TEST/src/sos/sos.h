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

#define SOS_IDLE_TIMEOUT	2
#define SOS_SENDING_TIMEOUT 2
#define SOS_SENT_TIMEOUT	2

#define SOS_ICON_W			156
#define SOS_ICON_H			131
#define SOS_ICON_X			((LCD_WIDTH-SOS_ICON_W)/2)
#define SOS_ICON_Y			((LCD_HEIGHT-SOS_ICON_H)/2)

#define SOS_NOTIFY_RECT_W	180
#define SOS_NOTIFY_RECT_H	120
#define SOS_NOTIFY_RECT_X	((LCD_WIDTH-SOS_NOTIFY_RECT_W)/2)
#define SOS_NOTIFY_RECT_Y	((LCD_HEIGHT-SOS_NOTIFY_RECT_H)/2)

typedef enum
{
	SOS_STATUS_IDLE,
	SOS_STATUS_READY,
	SOS_STATUS_SENDING,
	SOS_STATUS_SENT,
	SOS_STATUS_RECEIVED,
	SOS_STATUS_CANCEL,
	SOS_STATUS_MAX
}SOS_STATUS;

extern SOS_STATUS sos_state;
extern uint8_t sos_trigger_time[16];

extern void SOSTrigger(void);
extern void SOSStart(void);
extern void SOSCancel(void);
extern void SOSSChangrStatus(void);
#ifdef __cplusplus
}
#endif

#endif/*__SOS_H__*/
