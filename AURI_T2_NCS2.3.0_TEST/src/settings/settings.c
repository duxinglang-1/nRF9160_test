#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/fs/nvs.h>
#include <dk_buttons_and_leds.h>
#include "screen.h"
#include "settings.h"
#include "datetime.h"
#ifdef CONFIG_ALARM_SUPPORT
#include "alarm.h"
#endif
#include "lcd.h"
#include "codetrans.h"
#include "inner_flash.h"
#include "logger.h"

bool need_save_settings = false;
bool need_save_time = false;
bool need_reset_settings = false;
bool need_reset_bk_level = false;

uint8_t g_fw_version[64] = VERSION_STR LANG_BRANCH;

RESET_STATUS g_reset_status = RESET_STATUS_IDLE;

static bool reset_redraw_flag = false;
static bool reset_start_flag = false;
static bool reset_reboot_flag = false;

static uint8_t main_menu_index_bk = 0;

global_settings_t global_settings = {0};

static void FactoryResetCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(reset_timer, FactoryResetCallBack, NULL);
static void FactoryResetStartCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(reset_start_timer, FactoryResetStartCallBack, NULL);

extern sys_date_timer_t date_time;

const sys_date_timer_t FACTORY_DEFAULT_TIME = 
{
	2023,
	1,
	1,
	0,
	0,
	0,
	0		//0=sunday
};

const global_settings_t FACTORY_DEFAULT_SETTINGS = 
{
	SETTINGS_STATUS_NORMAL,	//status flag
	false,					//heart rate turn on
	false,					//blood pressure turn on
	false,					//blood oxygen turn on		
	true,					//wake screen by wrist
	false,					//wrist off check
	3,						//location type: 1:only wifi,2:only gps,3:wifi+gps,4:gps+wifi
	0,						//target steps
	60,						//health interval
	TEMP_UINT_C,			//Centigrade
	TIME_FORMAT_24,			//24 format
	LANGUAGE_EN,			//language
	DATE_FORMAT_YYYYMMDD,	//date format
	CLOCK_MODE_DIGITAL,		//colck mode
	BACKLIGHT_10_SEC,		//backlight time
	BACKLIGHT_LEVEL_8,		//backlight level
	{true,1},				//PHD
	{500,60},				//position interval
	{120,75},				//pb calibration
	{						//alarm
		{false,0,0,0},		
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
	},
};

void FactoryResetCallBack(struct k_timer *timer_id)
{
	switch(g_reset_status)
	{
	case RESET_STATUS_IDLE:
		break;

	case RESET_STATUS_RUNNING:
		g_reset_status = RESET_STATUS_FAIL;
		reset_redraw_flag = true;
		break;

	case RESET_STATUS_SUCCESS:
		reset_reboot_flag = true;
		break;

	case RESET_STATUS_FAIL:
		g_reset_status = RESET_STATUS_IDLE;
		reset_redraw_flag = true;
		break;
	}
}

void FactoryResetStartCallBack(struct k_timer *timer_id)
{
	reset_start_flag = true;
}

void SaveSystemDateTime(void)
{
	SaveDateTimeToInnerFlash(date_time);
}

void ResetSystemTime(void)
{
	memcpy(&date_time, &FACTORY_DEFAULT_TIME, sizeof(sys_date_timer_t));
	SaveSystemDateTime();
}

void InitSystemDateTime(void)
{
	sys_date_timer_t mytime = {0};

	ReadDateTimeFromInnerFlash(&mytime);
	
	if(!CheckSystemDateTimeIsValid(mytime))
	{
		memcpy(&mytime, &FACTORY_DEFAULT_TIME, sizeof(sys_date_timer_t));
	}
	memcpy(&date_time, &mytime, sizeof(sys_date_timer_t));

	SaveSystemDateTime();
	StartSystemDateTime();
}

void SaveSystemSettings(void)
{
	SaveSettingsToInnerFlash(global_settings);
}

void ResetSystemSettings(void)
{
	memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
	SaveSystemSettings();
}

void InitSystemSettings(void)
{
	int err;

	ReadSettingsFromInnerFlash(&global_settings);

	switch(global_settings.flag)
	{
	case SETTINGS_STATUS_INIT:
		ResetInnerFlash();
		memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
		SaveSystemSettings();
		break;

	case SETTINGS_STATUS_OTA:
		memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
		SaveSystemSettings();
		break;
		
	case SETTINGS_STATUS_NORMAL:
		break;		
	}

	InitSystemDateTime();
#ifdef CONFIG_ALARM_SUPPORT	
	AlarmRemindInit();
#endif
	mmi_chset_init();
}

void ResetFactoryDefault(void)
{
	ResetSystemTime();
	ResetSystemSettings();

	clear_cur_local_in_record();
	clear_local_in_record();	
	clear_cur_health_in_record();
	clear_health_in_record();
#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
	clear_cur_sport_in_record();
	clear_sport_in_record();
#endif

#ifdef CONFIG_IMU_SUPPORT
#ifdef CONFIG_STEP_SUPPORT
	ClearAllStepRecData();
#endif
#ifdef CONFIG_SLEEP_SUPPORT
	ClearAllSleepRecData();
#endif
#endif

#ifdef CONFIG_PPG_SUPPORT
	ClearAllHrRecData();
	ClearAllSpo2RecData();
	ClearAllBptRecData();
	sh_clear_bpt_cal_data();
#endif

#ifdef CONFIG_TEMP_SUPPORT
	ClearAllTempRecData();
#endif

	LogClear();
}

void SettingsMsgPorcess(void)
{
	if(need_save_time)
	{
		SaveSystemDateTime();
		need_save_time = false;
	}
	
	if(need_save_settings)
	{
		need_save_settings = false;
		SaveSystemSettings();
		SendSettingsData();
	}

	if(need_reset_settings)
	{
		need_reset_settings = false;
		k_timer_start(&reset_timer, K_MSEC(1000), K_NO_WAIT);
		ResetFactoryDefault();
	}
	
	if(reset_start_flag)
	{
		reset_start_flag = false;
		k_timer_start(&reset_timer, K_MSEC(1000), K_NO_WAIT);
		ResetFactoryDefault();
	}
	
	if(reset_reboot_flag)
	{
		reset_reboot_flag = false;
		LCD_Clear(BLACK);
		sys_reboot(0);
	}
}
