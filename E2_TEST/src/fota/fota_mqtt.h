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

#define FOTA_LOGO_W				60
#define FOTA_LOGO_H				60
#define FOTA_LOGO_X				((LCD_WIDTH-FOTA_LOGO_W)/2)
#define FOTA_LOGO_Y				20
#define FOTA_YES_W				40
#define FOTA_YES_H				40
#define FOTA_YES_X				156
#define FOTA_YES_Y				165
#define FOTA_NO_W				40
#define FOTA_NO_H				40
#define FOTA_NO_X				45
#define FOTA_NO_Y				165
#define FOTA_BG_W				240
#define FOTA_BG_H				31
#define FOTA_BG_X				((LCD_WIDTH-FOTA_BG_W)/2)
#define FOTA_BG_Y				154
#define FOTA_START_STR_W		210
#define FOTA_START_STR_H		47
#define FOTA_START_STR_X		((LCD_WIDTH-FOTA_START_STR_W)/2)
#define FOTA_START_STR_Y		95
#define FOTA_RUNNING_STR_W		153
#define FOTA_RUNNING_STR_H		18
#define FOTA_RUNNING_STR_X		((LCD_WIDTH-FOTA_RUNNING_STR_W)/2)
#define FOTA_RUNNING_STR_Y		192
#define FOTA_DOWNLOADING_STR_W	162
#define FOTA_DOWNLOADING_STR_H	23
#define FOTA_DOWNLOADING_STR_X	((LCD_WIDTH-FOTA_DOWNLOADING_STR_W)/2)
#define FOTA_DOWNLOADING_STR_Y	95
#define FOTA_PROGRESS_W			180
#define FOTA_PROGRESS_H			14
#define FOTA_PROGRESS_X			((LCD_WIDTH-FOTA_PROGRESS_W)/2)
#define FOTA_PROGRESS_Y			142
#define FOTA_PRO_NUM_W			58
#define FOTA_PRO_NUM_H			33
#define FOTA_PRO_NUM_X			((LCD_WIDTH-FOTA_PRO_NUM_W)/2)
#define FOTA_PRO_NUM_Y			166
#define FOTA_FINISHED_W			240
#define FOTA_FINISHED_H			240
#define FOTA_FINISHED_X			((LCD_WIDTH-FOTA_FINISHED_W)/2)
#define FOTA_FINISHED_Y			((LCD_HEIGHT-FOTA_FINISHED_H)/2)
#define FOTA_FAIL_ICON_W		104
#define FOTA_FAIL_ICON_H		104
#define FOTA_FAIL_ICON_X		((LCD_WIDTH-FOTA_FAIL_ICON_W)/2)
#define FOTA_FAIL_ICON_Y		25
#define FOTA_FAIL_STR_W			99
#define FOTA_FAIL_STR_H			18
#define FOTA_FAIL_STR_X			((LCD_WIDTH-FOTA_FAIL_STR_W)/2)
#define FOTA_FAIL_STR_Y			192


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
extern void fota_excu(void);
extern void fota_start_confirm(void);
extern void fota_reboot_confirm(void);
extern void fota_exit(void);
extern bool fota_is_running(void);

extern FOTA_STATUS_ENUM get_fota_status(void);

#endif/*__FOTA_MQTT_H__*/
#endif/*CONFIG_FOTA_DOWNLOAD*/