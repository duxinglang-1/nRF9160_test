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

#define FOTA_ICON_W		34
#define FOTA_ICON_H		32
#define FOTA_ICON_X		((LCD_WIDTH-FOTA_ICON_W)/2)
#define FOTA_ICON_Y		0

#define FOTA_TEXT_W		96
#define	FOTA_TEXT_H		32
#define	FOTA_TEXT_X		((LCD_WIDTH-FOTA_TEXT_W)/2)
#define	FOTA_TEXT_Y		32

#define FOTA_DOWNLOADING_TEXT_W		96
#define FOTA_DOWNLOADING_TEXT_H		32
#define FOTA_DOWNLOADING_TEXT_X		((LCD_WIDTH-FOTA_TEXT_W)/2)
#define FOTA_DOWNLOADING_TEXT_Y		0

#define FOTA_DOWNLOADING_PRO_W		80
#define FOTA_DOWNLOADING_PRO_H		8
#define FOTA_DOWNLOADING_PRO_X		((LCD_WIDTH-FOTA_DOWNLOADING_PRO_W)/2)
#define FOTA_DOWNLOADING_PRO_Y		32

#define FOTA_DOWNLOADING_NUM_W		10
#define FOTA_DOWNLOADING_NUM_H		24

#define FOTA_DOWNLOADING_PERCENT_W	13
#define FOTA_DOWNLOADING_PERCENT_H	24
#define FOTA_DOWNLOADING_PERCENT_X	((LCD_WIDTH-FOTA_DOWNLOADING_PERCENT_W+20)/2)
#define FOTA_DOWNLOADING_PERCENT_Y	40

typedef enum
{
	FOTA_STATUS_PREPARE,
	FOTA_STATUS_LINKING,
	FOTA_STATUS_DOWNLOADING,
	FOTA_STATUS_FINISHED,
	FOTA_STATUS_ERROR,
	FOTA_STATUS_MAX
}FOTA_STATUS_ENUM;

extern uint8_t g_fota_progress;

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