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

extern u8_t g_fota_progress;

extern void fota_work_init(struct k_work_q *work_q);
extern void fota_init(void);
extern void fota_start(void);
extern bool fota_is_running(void);

#endif/*__FOTA_MQTT_H__*/

#endif/*CONFIG_FOTA_DOWNLOAD*/