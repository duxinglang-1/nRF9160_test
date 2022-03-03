#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>

#define ALARM_MAX	8

typedef enum{
	TIME_FORMAT_24,
	TIME_FORMAT_12,
	TIME_FORMAT_MAX
}TIME_FORMAT;

typedef enum{
	DATE_FORMAT_YYYYMMDD,
	DATE_FORMAT_MMDDYYYY,
	DATE_FORMAT_DDMMYYYY,
	DATE_FORMAT_MAX
}DATE_FORMAT;

typedef enum
{
	LANGUAGE_EN,	//English
	LANGUAGE_CHN,	//Chinese
	LANGUAGE_JPN,	//Japanese
	LANGUAGE_MAX
}LANGUAGE_SET;

typedef enum
{
	CLOCK_MODE_DIGITAL,
	CLOCK_MODE_ANALOG,
	CLOCK_MODE_MAX
}CLOCK_MODE;

typedef enum
{
	BACKLIGHT_5_SEC=5,
	BACKLIGHT_10_SEC=10,
	BACKLIGHT_15_SEC=15,
	BACKLIGHT_30_SEC=30,
	BACKLIGHT_1_MIN=60,
	BACKLIGHT_2_MIN=120,
	BACKLIGHT_5_MIN=300,
	BACKLIGHT_10_MIN=600,
	BACKLIGHT_ALWAYS_ON=0,
	BACKLIGHT_MAX
}BACKLIGHT_TIME;

typedef enum
{
	BACKLIGHT_LEVEL_MIN,
	BACKLIGHT_LEVEL_1=BACKLIGHT_LEVEL_MIN,
	BACKLIGHT_LEVEL_2,
	BACKLIGHT_LEVEL_3,
	BACKLIGHT_LEVEL_4,
	BACKLIGHT_LEVEL_5,
	BACKLIGHT_LEVEL_6,
	BACKLIGHT_LEVEL_7,
	BACKLIGHT_LEVEL_8,
	BACKLIGHT_LEVEL_MAX=BACKLIGHT_LEVEL_8
}BACKLIGHT_LEVEL;

typedef struct{
	bool is_on;
	u8_t hour;
	u8_t minute;
	u8_t repeat;	//全是1就是每天提醒，全是0就是只提醒一次，0x1111100就是工作日提醒，其他就是自定义
}alarm_infor_t;

typedef struct{
	bool is_on;
	u8_t interval;
}phd_measure_t;		//整点测量

typedef struct{
	u32_t steps;
	u32_t time;
}location_interval_t;

typedef struct{
	u8_t systolic;		//收缩压
	u8_t diastolic;		//舒张压
}bp_calibra_t;

typedef struct{
	bool init;		//system inited flag
	bool hr_is_on;	//heart rate
	bool bp_is_on;	//blood pressure
	bool bo_is_on;	//blood oxygen
	bool wake_screen_by_wrist;
	bool wrist_off_check;
	u16_t target_steps;
	u32_t health_interval;
	TIME_FORMAT time_format;
	LANGUAGE_SET language;
	DATE_FORMAT date_format;
	CLOCK_MODE idle_colck_mode;
	BACKLIGHT_TIME backlight_time;
	BACKLIGHT_LEVEL backlight_level;
	phd_measure_t phd_infor;
	location_interval_t dot_interval;
	bp_calibra_t bp_calibra;
	alarm_infor_t alarm[ALARM_MAX];
}global_settings_t;

extern bool need_save_time;
extern bool need_save_settings;
extern bool need_reset_settings;

extern u8_t screen_id;

extern global_settings_t global_settings;

extern void InitSystemSettings(void);
extern void SaveSystemSettings(void);


#endif/*__SETTINGS_H__*/
