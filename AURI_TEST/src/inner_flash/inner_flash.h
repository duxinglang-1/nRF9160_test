/****************************************Copyright (c)************************************************
** File Name:			    inner_flash.h
** Descriptions:			nrf9160 inner flash management head file
** Created By:				xie biao
** Created Date:			2021-01-07
** Modified Date:      		2021-07-12
** Version:			    	V1.1
******************************************************************************************************/
#ifndef __INNER_FLASH_H__
#define __INNER_FLASH_H__

#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <drivers/gps.h>
#include "datetime.h"
#include "Settings.h"

#define DATETIME_ID 			1
#define SETTINGS_ID 			2
#define SPORT_RECORD_ID 		10
#define SPORT_LENGTH_ID    		11	

typedef struct
{
	sys_date_timer_t timestamp;
	u16_t steps;
	u16_t distance;
	u16_t calorie;
	u16_t light_sleep;
	u16_t deep_sleep;
}sport_record_t;

typedef struct
{
	sys_date_timer_t timestamp;
	u8_t hr;
	u8_t spo2;
	u8_t systolic;		// ’Àı—π
	u8_t diastolic;	// Ê’≈—π
}health_record_t;

typedef struct
{
	sys_date_timer_t timestamp;
	double latitude;
	double longitude;
	float altitude;
	float speed;
	float heading;
	struct gps_datetime datetime;
}local_record_t;

extern void test_nvs(void);
extern void ReadSettingsFromInnerFlash(global_settings_t *settings);
extern void SaveSettingsToInnerFlash(global_settings_t settings);
extern void ReadDateTimeFromInnerFlash(sys_date_timer_t *time);
extern void SaveDateTimeToInnerFlash(sys_date_timer_t time);
extern bool save_local_to_record(local_record_t *local_data);
extern bool save_health_to_record(health_record_t *health_data);
extern bool save_sport_to_record(sport_record_t *sport_data);
extern bool get_local_record(local_record_t *local_data, u32_t index);
extern bool get_local_record_from_time(local_record_t *local_data, sys_date_timer_t begin_time, u32_t index);
extern bool get_health_record(health_record_t *health_data, u32_t index);
extern bool get_health_record_from_time(health_record_t *health_data, sys_date_timer_t begin_time, u32_t index);
extern bool get_sport_record(sport_record_t *sport_data, u32_t index);
extern bool get_sport_record_from_time(sport_record_t *sport_data, sys_date_timer_t begin_time, u32_t index);

#endif/*__INNER_FLASH_H__*/