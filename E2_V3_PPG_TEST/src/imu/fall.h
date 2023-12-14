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

#ifdef __cplusplus
extern "C" {
#endif

#define FALL_ICON_W			100
#define FALL_ICON_H			104
#define FALL_ICON_X			((LCD_WIDTH-FALL_ICON_W)/2)
#define FALL_ICON_Y			20
#define FALL_YES_W			60
#define FALL_YES_H			60
#define FALL_YES_X			45
#define FALL_YES_Y			145
#define FALL_NO_W			60
#define FALL_NO_H			60
#define FALL_NO_X			135
#define FALL_NO_Y			145


#define FALL_NOTIFY_RECT_W	180
#define FALL_NOTIFY_RECT_H	120
#define FALL_NOTIFY_RECT_X	((LCD_WIDTH-FALL_NOTIFY_RECT_W)/2)
#define FALL_NOTIFY_RECT_Y	((LCD_HEIGHT-FALL_NOTIFY_RECT_H)/2)

#define FALL_NOTIFY_TIMEOUT 	30
#define FALL_SENDING_TIMEOUT 	2
#define FALL_SENT_TIMEOUT		2
#define FALL_CANCEL_TIMEOUT 	2
#define FALL_IDLE_TIMEOUT		2


typedef enum
{
	FALL_STATUS_IDLE,
	FALL_STATUS_NOTIFY,
	FALL_STATUS_SENDING,
	FALL_STATUS_SENT,
	FALL_STATUS_RECEIVED,
	FALL_STATUS_CANCEL,
	FALL_STATUS_CANCELED,
	FALL_STATUS_MAX
}FALL_STATUS;

extern FALL_STATUS fall_state;
extern uint8_t fall_trigger_time[16];

extern void FallTrigger(void);
extern void FallAlarmStart(void);
extern void FallAlarmConfirm(void);
extern void FallAlarmCancel(void);
extern bool FallIsRunning(void);

#ifdef __cplusplus
}
#endif

#endif/*__FALL_H__*/
