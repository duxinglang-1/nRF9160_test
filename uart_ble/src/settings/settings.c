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

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(settings, CONFIG_LOG_DEFAULT_LEVEL);

#define DATETIME_ID 1
#define SETTINGS_ID 2

static bool nvs_init_flag = false;

static struct nvs_fs fs;
static struct flash_pages_info info;

bool need_save_settings = false;
bool need_save_time = false;
bool need_reset_settings = false;

global_settings_t global_settings = {0};

extern sys_date_timer_t date_time;

const sys_date_timer_t FACTORY_DEFAULT_TIME = 
{
	2020,
	01,
	01,
	00,
	00,
	00,
	3		//0=sunday
};

const global_settings_t FACTORY_DEFAULT_SETTINGS = 
{
	false,					//system inited flag
	false,					//heart rate turn on
	false,					//blood pressure turn on
	false,					//blood oxygen turn on		
	true,					//wake screen by wrist
	0x0000,					//target steps
	TIME_FORMAT_24,			//24 format
	LANGUAGE_EN,			//language
	DATE_FORMAT_YYYYMMDD,	//date format
	CLOCK_MODE_DIGITAL,		//colck mode
	BACKLIGHT_ALWAYS_ON,	//backlight time
	{true,1},				//PHD
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

static int nvs_setup(void)
{	
	int err;	

	fs.offset = DT_FLASH_AREA_STORAGE_OFFSET;	
	err = flash_get_page_info_by_offs(device_get_binding(DT_FLASH_DEV_NAME), fs.offset, &info);	
	if(err)
	{		
		//LOG_INF("Unable to get page info");
		return err;
	}	

	fs.sector_size = info.size;
	fs.sector_count = 6U;
	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	if(err)
	{
		//LOG_INF("Flash Init failed\n");
		return err;
	}

	nvs_init_flag = true;
	return err;
}

void SaveSystemDateTime(void)
{
	nvs_write(&fs, DATETIME_ID, &date_time, sizeof(sys_date_timer_t));
}

void ResetSystemTime(void)
{
	memcpy(&date_time, &FACTORY_DEFAULT_TIME, sizeof(sys_date_timer_t));
	SaveSystemDateTime();
}

void InitSystemDateTime(void)
{
	int err = 0;
	sys_date_timer_t mytime = {0};

	err = nvs_read(&fs, DATETIME_ID, &date_time, sizeof(sys_date_timer_t));
	if(err < 0)
	{
		LOG_INF("get datetime err:%d\n", err);
	}
	
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
	nvs_write(&fs, SETTINGS_ID, &global_settings, sizeof(global_settings_t));
}

void ResetSystemSettings(void)
{
	memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
	SaveSystemSettings();
}

void InitSystemSettings(void)
{
	int err;

	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
			LOG_INF("Flash Init failed, return!\n");
			return;
		}
	}
	
	err = nvs_read(&fs, SETTINGS_ID, &global_settings, sizeof(global_settings_t));
	if(err < 0)
	{
		LOG_INF("get settins err:%d\n", err);
	}

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
