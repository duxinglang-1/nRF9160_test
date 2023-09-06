#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>

//#define FW_FOR_CN	//中文版本

#define ALARM_MAX	8
#define MENU_MAX_COUNT	10
#define MENU_NAME_MAX	30

#ifdef FW_FOR_CN
#define SETTINGS_CAREMATE_URL	"https://caremate.audarhealth.cn/login"
#else
#define SETTINGS_CAREMATE_URL	"https://caremate.audarhealth.com/login"
#endif

typedef void(*menu_handler)(void);

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
	LANGUAGE_BEGIN,
#ifndef FW_FOR_CN
	LANGUAGE_EN = LANGUAGE_BEGIN,	//English
	LANGUAGE_DE,					//Deutsch
	LANGUAGE_FR,					//French
	LANGUAGE_ITA,					//Italian
	LANGUAGE_ES,					//Spanish
	LANGUAGE_PT,					//Portuguese
#else
	LANGUAGE_CHN = LANGUAGE_BEGIN,	//Chinese
	LANGUAGE_EN,					//English
#endif	
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
	BACKLIGHT_LEVEL_MAX=BACKLIGHT_LEVEL_4
}BACKLIGHT_LEVEL;

typedef enum
{
	TEMP_UINT_C,
	TEMP_UINT_F,
	TEMP_UINT_MAX
}TEMP_UNIT;

typedef enum
{
	SETTINGS_MENU_MAIN,
	SETTINGS_MENU_LANGUAGE,
	SETTINGS_MENU_FACTORY_RESET,
	SETTINGS_MENU_CAREMATE_QR,
	SETTINGS_MENU_OTA,
	SETTINGS_MENU_BRIGHTNESS,
	SETTINGS_MENU_TEMP,
	SETTINGS_MENU_DEVICE,
	SETTINGS_MENU_SIM,
	SETTINGS_MENU_FW,
	SETTINGS_MENU_MAX
}MENU_ID;

typedef enum
{
	RESET_STATUS_IDLE,
	RESET_STATUS_RUNNING,
	RESET_STATUS_SUCCESS,
	RESET_STATUS_FAIL,
	RESET_STATUS_MAX
}RESET_STATUS;

typedef struct
{
	MENU_ID id;
	uint8_t index;
	uint8_t count;
	uint16_t name[LANGUAGE_MAX][MENU_MAX_COUNT][MENU_NAME_MAX];;
	menu_handler sel_handler[MENU_MAX_COUNT];
	menu_handler pg_handler[4];
}settings_menu_t;

typedef struct
{
	bool is_on;
	uint8_t hour;
	uint8_t minute;
	uint8_t repeat;	//全是1就是每天提醒，全是0就是只提醒一次，0x1111100就是工作日提醒，其他就是自定义
}alarm_infor_t;

typedef struct
{
	bool is_on;
	uint8_t interval;
}phd_measure_t;		//整点测量

typedef struct
{
	uint32_t steps;
	uint32_t time;
}location_interval_t;

typedef struct
{
	uint8_t systolic;		//收缩压
	uint8_t diastolic;		//舒张压
}bp_calibra_t;

typedef struct
{
	bool init;		//system inited flag
	bool hr_is_on;	//heart rate
	bool bp_is_on;	//blood pressure
	bool bo_is_on;	//blood oxygen
	bool wake_screen_by_wrist;
	bool wrist_off_check;
	uint8_t location_type;	//1:only wifi,2:only gps,3:wifi+gps,4:gps+wifi
	uint16_t target_steps;
	uint32_t health_interval;
	TEMP_UNIT temp_unit;
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

extern uint8_t screen_id;
extern uint8_t g_fw_version[64];

extern global_settings_t global_settings;
extern settings_menu_t settings_menu;
extern RESET_STATUS g_reset_status;

extern void InitSystemSettings(void);
extern void SaveSystemSettings(void);
extern void SettingMenuInit(void);
extern void EnterSettings(void);
extern void ResetFactoryDefault(void);
#endif/*__SETTINGS_H__*/
