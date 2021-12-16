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

//存储时间和设置项的地址ID
#define DATETIME_ID 				1
#define SETTINGS_ID 				2
//存储索引的地址ID
//实时数据ID
#define CUR_SPORT_ID				3
#define CUR_HEALTH_ID				4
#define CUR_LOCAL_ID				5
//定时数据的地址ID
#define SPORT_INDEX_ADDR_ID 		6
#define HEALTH_INDEX_ADDR_ID        8
#define LOCAL_INDEX_ADDR_ID			10
//储存总条数的地址ID
#define SPORT_COUNT_ADDR_ID    		7	
#define HEALTH_COUNT_ADDR_ID        9
#define LOCAL_COUNT_ADDR_ID         11
//存储索引的起止编号
#define SPORT_INDEX_BEGIN			1000
#define SPORT_INDEX_MAX				1100
#define HEALTH_INDEX_BEGIN			2000
#define HEALTH_INDEX_MAX			2100
#define LOCAL_INDEX_BEGIN			3000
#define LOCAL_INDEX_MAX				3100

typedef enum
{
	RECORD_TYPE_LOCATION,
	RECORD_TYPE_HEALTH,
	RECORD_TYPE_SPORT,
	RECORD_TYPE_MAX
}ENUM_RECORD_TYPE;

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
	u8_t systolic;		//收缩压
	u8_t diastolic;	//舒张压
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

extern bool save_cur_local_to_record(local_record_t *local_data);
extern bool save_cur_health_to_record(health_record_t *health_data);
extern bool save_cur_sport_to_record(sport_record_t *sport_data);

extern bool get_cur_local_from_record(local_record_t *local_data);
extern bool get_cur_health_from_record(health_record_t *health_data);
extern bool get_cur_sport_from_record(sport_record_t *sport_data);

extern bool save_local_to_record(local_record_t *local_data);
extern bool save_health_to_record(health_record_t *health_data);
extern bool save_sport_to_record(sport_record_t *sport_data);

extern bool get_local_from_record(local_record_t *local_data, u32_t index);
extern bool get_health_from_record(health_record_t *health_data, u32_t index);
extern bool get_sport_from_record(sport_record_t *sport_data, u32_t index);

extern bool get_last_sport_from_record(sport_record_t *sport_data);
extern bool get_last_health_from_record(health_record_t *health_data);
extern bool get_last_local_from_record(local_record_t *local_data);

extern bool get_local_from_record_by_time(local_record_t *local_data, sys_date_timer_t begin_time, u32_t index);
extern bool get_health_from_record_by_time(health_record_t *health_data, sys_date_timer_t begin_time, u32_t index);
extern bool get_sport_from_record_by_time(sport_record_t *sport_data, sys_date_timer_t begin_time, u32_t index);

#endif/*__INNER_FLASH_H__*/