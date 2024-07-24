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

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <nrf_modem_gnss.h>
#include "datetime.h"
#include "Settings.h"
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif
#ifdef CONFIG_PPG_SUPPORT
#include "ppg.h"
#endif

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

typedef enum
{
	RECORD_LOCATION_GPS,
	RECORD_LOCATION_WIFI,
	RECORD_LOCATION_MAX,
}ENUM_RECORD_LOCATION_TYPE;

typedef enum
{
	RECORD_HEALTH_HR,
	RECORD_HEALTH_SPO2,
	RECORD_HEALTH_BPT,
	RECORD_HEALTH_ECG,
	RECORD_HEALTH_MAX,
}ENUM_RECORD_HEALTH_TYPE;

typedef enum
{
	RECORD_SPORT_STEP,
	RECORD_SPORT_SLEEP,
	RECORD_SPORT_MAX,
}ENUM_RECORD_SPORT_TYPE;

typedef struct
{
	sys_date_timer_t timestamp;
	uint16_t steps;
	uint16_t distance;
	uint16_t calorie;	
}step_record_t;

typedef struct
{
	sys_date_timer_t timestamp;
	uint16_t light_sleep;
	uint16_t deep_sleep;
}sleep_record_t;

typedef struct
{
	step_record_t step_rec;
	sleep_record_t sleep_rec;
}sport_record_t;

typedef struct
{
	sys_date_timer_t timestamp;
	double latitude;
	double longitude;
	float altitude;
	float speed;
	float heading;
	struct nrf_modem_gnss_datetime datetime;
}gps_record_t;

#ifdef CONFIG_WIFI_SUPPORT
typedef struct
{
	sys_date_timer_t timestamp;
	wifi_infor wifi_node;
}wifi_record_t;
#endif

typedef struct
{
	gps_record_t gps_rec;
#ifdef CONFIG_WIFI_SUPPORT
	wifi_record_t wifi_rec;
#endif
}local_record_t;

typedef union
{
	sport_record_t sport;
	health_record_t health;
	local_record_t local;
}imu_data_u;

extern sport_record_t last_sport;
extern health_record_t last_health;

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

extern bool get_local_from_record(local_record_t *local_data, uint32_t index);
extern bool get_health_from_record(health_record_t *health_data, uint32_t index);
extern bool get_sport_from_record(sport_record_t *sport_data, uint32_t index);

extern bool get_last_sport_from_record(sport_record_t *sport_data);
extern bool get_last_health_from_record(health_record_t *health_data);
extern bool get_last_local_from_record(local_record_t *local_data);

extern bool get_local_from_record_by_time(local_record_t *local_data, ENUM_RECORD_LOCATION_TYPE type, sys_date_timer_t begin_time, uint32_t index);
extern bool get_health_from_record_by_time(health_record_t *health_data, ENUM_RECORD_HEALTH_TYPE type, sys_date_timer_t begin_time, uint32_t index);
extern bool get_sport_from_record_by_time(sport_record_t *sport_data, ENUM_RECORD_SPORT_TYPE type, sys_date_timer_t begin_time, uint32_t index);

#endif/*__INNER_FLASH_H__*/