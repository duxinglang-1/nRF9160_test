#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <fs/nvs.h>
#include <drivers/flash.h>
#include <dk_buttons_and_leds.h>
#include "settings.h"
#include "datetime.h"
#include "alarm.h"
#include "lcd.h"
#include "codetrans.h"
#include "inner_flash.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(settings, CONFIG_LOG_DEFAULT_LEVEL);

bool need_save_settings = false;
bool need_save_time = false;
bool need_reset_settings = false;

global_settings_t global_settings = {0};

extern sys_date_timer_t date_time;

const sys_date_timer_t FACTORY_DEFAULT_TIME = 
{
	2020,
	1,
	1,
	0,
	0,
	0,
	3		//0=sunday
};

const global_settings_t FACTORY_DEFAULT_SETTINGS = 
{
	false,					//system inited flag
	false,					//heart rate turn on
	false,					//blood pressure turn on
	false,					//blood oxygen turn on		
	true,					//wake screen by wrist
	true,					//wrist off check
	0,						//target steps
	60,						//health interval
	TIME_FORMAT_24,			//24 format
	LANGUAGE_CHN,			//language
	DATE_FORMAT_YYYYMMDD,	//date format
	CLOCK_MODE_DIGITAL,		//colck mode
	BACKLIGHT_15_SEC,		//backlight time
	BACKLIGHT_LEVEL_8,		//backlight level
	{true,1},				//PHD
	{500,60},				//position interval
	{120,70},				//pb calibration
	{						//alarm
		{false,0,0,0},		
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
	}
};

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

	if(!global_settings.init)
	{
		memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
		SaveSystemSettings();
	}

	InitSystemDateTime();
	AlarmRemindInit();

	mmi_chset_init();
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
	}

	if(need_reset_settings)
	{
		need_reset_settings = false;
		ResetSystemSettings();
		ResetSystemTime();

		lcd_sleep_out = true;
		update_date_time = true;
	}
}
