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
	LANGUAGE_CHS,	//simplified Chinese
	LANGUAGE_CHT,	//Traditional Chinese
	LANGUAGE_JPN,	//Japanese
	LANGUAGE_MAX
}LANGUAGE_SET;

typedef struct{
	bool is_on;
	u8_t hour;
	u8_t minute;
	u8_t repeat;	//全是1就是每天提醒，全是0就是只提醒一次，0x1111100就是工作日提醒，其他就是自定义
}alarm_infor_t;

typedef struct{
	u8_t count;
	alarm_infor_t infor[ALARM_MAX];
}alarm_settings_t;

typedef struct{
	TIME_FORMAT time_format;
	LANGUAGE_SET language;
	DATE_FORMAT date_format;
	bool hr_is_on;	//heart rate
	bool bp_is_on;	//blood pressure
	bool bo_is_on;	//blood oxygen
	alarm_settings_t alarm;
}global_settings_t;

extern global_settings_t global_settings;


