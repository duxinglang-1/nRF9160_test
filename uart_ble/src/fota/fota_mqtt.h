/****************************************Copyright (c)************************************************
** File Name:			    fota_mqtt.h
** Descriptions:			fota by mqtt protocal process head file
** Created By:				xie biao
** Created Date:			2021-03-29
** Modified Date:      		2021-03-29 
** Version:			    	V1.0
******************************************************************************************************/
#ifdef CONFIG_FOTA_DOWNLOAD
#ifndef __FOTA_MQTT_H__
#define __FOTA_MQTT_H__

#include "lcd.h"

#define FOTA_NOTIFY_OFFSET_W	10
#define FOTA_NOTIFY_OFFSET_H	10

#define FOTA_NOTIFY_RECT_W		200
#define FOTA_NOTIFY_RECT_H		200
#define FOTA_NOTIFY_RECT_X		((LCD_WIDTH-FOTA_NOTIFY_RECT_W)/2)
#define FOTA_NOTIFY_RECT_Y		((LCD_HEIGHT-FOTA_NOTIFY_RECT_H)/2)

#define FOTA_NOTIFY_STRING_W	(FOTA_NOTIFY_RECT_W-10)
#define	FOTA_NOTIFY_STRING_H	(90)
#define	FOTA_NOTIFY_STRING_X	((LCD_WIDTH-FOTA_NOTIFY_STRING_W)/2)
#define	FOTA_NOTIFY_STRING_Y	(FOTA_NOTIFY_RECT_Y+(FOTA_NOTIFY_RECT_H-FOTA_NOTIFY_STRING_H)/2)

#define FOTA_NOTIFY_PRO_W		(FOTA_NOTIFY_RECT_W-20)
#define FOTA_NOTIFY_PRO_H		20
#define FOTA_NOTIFY_PRO_X		(FOTA_NOTIFY_RECT_X+(FOTA_NOTIFY_RECT_W-FOTA_NOTIFY_PRO_W)/2)
#define FOTA_NOTIFY_PRO_Y		(FOTA_NOTIFY_STRING_Y+40)

#define FOTA_NOTIFY_YES_W		60
#define FOTA_NOTIFY_YES_H		40
#define FOTA_NOTIFY_YES_X		(FOTA_NOTIFY_RECT_X+FOTA_NOTIFY_OFFSET_W)
#define FOTA_NOTIFY_YES_Y		(FOTA_NOTIFY_RECT_Y+FOTA_NOTIFY_RECT_H-FOTA_NOTIFY_YES_H-FOTA_NOTIFY_OFFSET_H)

#define FOTA_NOTIFY_NO_W		60
#define FOTA_NOTIFY_NO_H		40
#define FOTA_NOTIFY_NO_X		(FOTA_NOTIFY_RECT_X+(FOTA_NOTIFY_RECT_W-FOTA_NOTIFY_NO_W-FOTA_NOTIFY_OFFSET_W))
#define FOTA_NOTIFY_NO_Y		(FOTA_NOTIFY_RECT_Y+FOTA_NOTIFY_RECT_H-FOTA_NOTIFY_YES_H-FOTA_NOTIFY_OFFSET_H)



typedef enum
{
	FOTA_STATUS_PREPARE,
	FOTA_STATUS_LINKING,
	FOTA_STATUS_DOWNLOADING,
	FOTA_STATUS_FINISHED,
	FOTA_STATUS_ERROR,
	FOTA_STATUS_MAX
}FOTA_STATUS_ENUM;

extern u8_t g_fota_progress;

extern void fota_work_init(struct k_work_q *work_q);
extern void fota_init(void);
extern void fota_start(void);
extern void fota_start_confirm(void);
extern void fota_reboot_confirm(void);
extern void fota_exit(void);
extern bool fota_is_running(void);

extern FOTA_STATUS_ENUM get_fota_status(void);

#endif/*__FOTA_MQTT_H__*/
#endif/*CONFIG_FOTA_DOWNLOAD*/