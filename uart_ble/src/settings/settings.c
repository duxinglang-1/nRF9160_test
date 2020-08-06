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

#define DATETIME_ID 1
#define SETTINGS_ID 2

static bool nvs_init_flag = false;

static struct nvs_fs fs;
static struct flash_pages_info info;

bool need_save_settings = false;
bool need_save_time = false;

global_settings_t global_settings = {0};

extern sys_date_timer_t date_time;

//const global_settings_t FACTORY_DEFAULT_DATA = {
//	TIME_FORMAT_24,			//24 format
//	false,					//heart rate turn on
//	false,					//blood pressure turn on
//	false,					//blood oxygen turn on					
//	{
//		0,					//alarm
//		{false,0,0,0},
//		{false,0,0,0},
//		{false,0,0,0},
//		{false,0,0,0},
//		{false,0,0,0},
//		{false,0,0,0},
//		{false,0,0,0},
//		{false,0,0,0},
//	}
//};

static int nvs_setup(void)
{	
	int err;	

	fs.offset = DT_FLASH_AREA_STORAGE_OFFSET;	
	err = flash_get_page_info_by_offs(device_get_binding(DT_FLASH_DEV_NAME), fs.offset, &info);	
	if(err)
	{		
		printk("Unable to get page info");
		return err;
	}	

	fs.sector_size = info.size;
	fs.sector_count = 6U;
	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	if(err)
	{
		printk("Flash Init failed\n");
		return err;
	}

	nvs_init_flag = true;
	return err;
}

void SaveSystemDateTime(void)
{
	printk("date_time: %04d-%02d-%02d, %02d:%02d:%02d\n", date_time.year,date_time.month,date_time.day,date_time.hour,date_time.minute,date_time.second);
	nvs_write(&fs, DATETIME_ID, &date_time, sizeof(sys_date_timer_t));
	printk("date_time set ok!\n");
}

void InitSystemDateTime(void)
{
	int err = 0;
	sys_date_timer_t mytime = {0};

	err = nvs_read(&fs, DATETIME_ID, &mytime, sizeof(sys_date_timer_t));
	if(err < 0)
	{
		printk("get datetime err:%d\n", err);
	}
	
	printk("mytime: %04d-%02d-%02d, %02d:%02d:%02d\n", mytime.year,mytime.month,mytime.day,mytime.hour,mytime.minute,mytime.second);
	
	memset(date_time, 0, sizeof(sys_date_timer_t));
	if(!CheckSystemDateTimeIsValid(mytime))
	{
		mytime.year = SYSTEM_DEFAULT_YEAR;
		mytime.month = SYSTEM_DEFAULT_MONTH;
		mytime.day = SYSTEM_DEFAULT_DAY;
		mytime.hour = SYSTEM_DEFAULT_HOUR;
		mytime.minute = SYSTEM_DEFAULT_MINUTE;
		mytime.second = SYSTEM_DEFAULT_SECOND;
		mytime.week = GetWeekDayByDate(mytime);
	}
	
	memcpy(&date_time, &mytime, sizeof(sys_date_timer_t));
	SaveSystemDateTime();
	
	StartSystemDateTime();
}

void InitSystemSettings(void)
{
	int err;
	global_settings_t settings = {0};

	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
			printk("Flash Init failed, return!\n");
			return;
		}
	}
	
	err = nvs_read(&fs, SETTINGS_ID, &settings, sizeof(global_settings_t));
	if(err < 0)
	{
		printk("get settins err:%d\n", err);
	}

	memcpy(&global_settings, &settings, sizeof(global_settings_t));

	InitSystemDateTime();
}

void SaveSystemSettings(void)
{
	int err;
	
	err = nvs_write(&fs, SETTINGS_ID, &global_settings, sizeof(global_settings_t));
	printk("save settings err:%d\n", err);
}

void ResetSystemSettings(void)
{
	//memcpy(global_settings, FACTORY_DEFAULT_DATA, sizeof(global_settings_t));

	SaveSysetemSettings(global_settings);
}

