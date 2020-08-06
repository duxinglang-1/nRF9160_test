/****************************************Copyright (c)************************************************
** File name:			     datetime.c
** Last modified Date:          
** Last Version:		   
** Descriptions:		   使用的SDK版本-SDK_15.2
**						
** Created by:			谢彪
** Created date:		2019-12-31
** Version:			    1.0
** Descriptions:		系统日期时间管理
******************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <fs/nvs.h>
#include <drivers/flash.h>
#include "datetime.h"
#include "settings.h"
#include "lcd.h"
#include "font.h"

#define MAX_REBOOT 400
#define SLEEP_TIME 100

#define ADDRESS_ID 1
#define KEY_ID 2
#define RBT_CNT_ID 3
#define STRING_ID 4
#define LONG_ID 5


static struct nvs_fs fs;
struct flash_pages_info info;

sys_date_timer_t date_time = {0};


uint8_t GetWeekDayByDate(sys_date_timer_t date)
{
	uint8_t index = 4;//1970年1月1日是星期四 0=sunday
	uint32_t i,count=0;
	
	if(date.year < 1970)
		return 0xff;
	
	for(i=1970;i<date.year;i++)
	{
		if(i%4 == 0)	//闰年366天
			count += 366;
		else
			count += 365;
	}
	
	count = count%7;
	index = (index+count)%7;
	
	return index;
}

bool CheckSystemDateTimeIsValid(sys_date_timer_t systime)
{
	bool ret = true;
	
	if((systime.year<1970 || systime.year>9999)
		|| (systime.month==0 || systime.month>12) 
		|| (systime.day==0 
			|| ((systime.day>31)&&(systime.month==1||systime.month==3||systime.month==5||systime.month==7||systime.month==8||systime.month==10||systime.month==12))
			|| ((systime.day>30)&&(systime.month==4||systime.month==6||systime.month==9||systime.month==11))
			|| ((systime.day>29)&&(systime.month==2&&systime.year%4==0))
			|| ((systime.day>28)&&(systime.month==2&&systime.year%4!=0)))
		|| ((systime.hour>23)||(systime.minute>59)||(systime.second>59))
		|| (systime.week>6))
	{
		ret = false;
	}
	
	return ret;
}

void SetSystemDateTime(sys_date_timer_t systime)
{
	printk("systime: %04d-%02d-%02d, %02d:%02d:%02d\n", systime.year,systime.month,systime.day,systime.hour,systime.minute,systime.second);
	nvs_write(&fs, ADDRESS_ID, &systime, sizeof(sys_date_timer_t));
	printk("systime set ok!\n");
}

void GetSystemDateTime(sys_date_timer_t *systime)
{
	int err = 0;
	sys_date_timer_t mytime = {0};
	char buf[16];

	fs.offset = DT_FLASH_AREA_STORAGE_OFFSET;
	err = flash_get_page_info_by_offs(device_get_binding(DT_FLASH_DEV_NAME), fs.offset, &info);
	if(err)
	{
	    printk("Unable to get page info\n");
	}

	fs.sector_size = info.size;
	fs.sector_count = 3U;
	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	if(err)
	{
	    printk("Flash Init failed\n");
	}

	err = nvs_read(&fs, ADDRESS_ID, &mytime, sizeof(sys_date_timer_t));
	printk("mytime: %04d-%02d-%02d, %02d:%02d:%02d\n", mytime.year,mytime.month,mytime.day,mytime.hour,mytime.minute,mytime.second);
	
	memset(systime, 0, sizeof(sys_date_timer_t));
	
	if(!CheckSystemDateTimeIsValid(mytime))
	{
		mytime.year = SYSTEM_DEFAULT_YEAR;
		mytime.month = SYSTEM_DEFAULT_MONTH;
		mytime.day = SYSTEM_DEFAULT_DAY;
		mytime.hour = SYSTEM_DEFAULT_HOUR;
		mytime.minute = SYSTEM_DEFAULT_MINUTE;
		mytime.second = SYSTEM_DEFAULT_SECOND;
		mytime.week = GetWeekDayByDate(mytime);
		
		SetSystemDateTime(mytime);
	}
	
	memcpy(systime, (sys_date_timer_t*)&mytime, sizeof(sys_date_timer_t));
}

void GetSystemDateStrings(u8_t *str_date)
{
	switch(global_settings.date_format)
	{
	case DATE_FORMAT_YYYYMMDD:
		sprintf((char*)str_date, "%04d/%02d/%02d", date_time.year, date_time.month, date_time.day);
		break;
	case DATE_FORMAT_MMDDYYYY:
		sprintf((char*)str_date, "%02d/%02d/%04d", date_time.month, date_time.day, date_time.year);
		break;
	case DATE_FORMAT_DDMMYYYY:
		sprintf((char*)str_date, "%02d/%02d/%04d", date_time.day, date_time.month, date_time.year);
		break;
	}
}

void GetSystemTimeStrings(u8_t *str_time)
{
	u8_t *am_pm[2] = {"am", "pm"};

	switch(global_settings.time_format)
	{
	case TIME_FORMAT_24:
		sprintf((char*)str_time, "%02d:%02d:%02d", date_time.hour, date_time.minute, date_time.second);
		break;
	case TIME_FORMAT_12:
		sprintf((char*)str_time, "%02d:%02d:%02d %s", (date_time.hour>12 ? (date_time.hour-12):date_time.hour), date_time.minute, date_time.second, am_pm[date_time.hour/12]);
		break;
	}
}

void GetSystemWeekStrings(u8_t *str_week)
{
	u8_t *week_en[7] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	u8_t *week_cns[7] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
	u8_t *week_cnt[7] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};	
	u8_t *week_jpn[15] = {"にちようbaiび", "げつようび", "かようび", "すbaiいようび", "もくようび", "きんようび", "どようび"};

	switch(global_settings.language)
	{
	case LANGUAGE_CHS:
		strcpy((char*)str_week, (const char*)week_cns[date_time.week]);
		break;
	case LANGUAGE_CHT:
		strcpy((char*)str_week, (const char*)week_cnt[date_time.week]);
		break;
	case LANGUAGE_EN:
		strcpy((char*)str_week, (const char*)week_en[date_time.week]);
		break;
	case LANGUAGE_JPN:
		strcpy((char*)str_week, (const char*)week_jpn[date_time.week]);
		break;
	}
}

void IdleShowSystemDate(void)
{
	u16_t x,y,w,h;
	u8_t str_date[20] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;
	LCD_SetFontSize(FONT_SIZE_32);

	GetSystemDateStrings(str_date);
	LCD_MeasureString(str_date,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_DATE_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);	
	LCD_ShowString(x,y,str_date);

}

void IdleShowSystemTime(void)
{
	u16_t x,y,w,h;
	u8_t str_time[20] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;
	LCD_SetFontSize(FONT_SIZE_32);

	GetSystemTimeStrings(str_time);
	LCD_MeasureString(str_time,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_TIME_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);
	LCD_ShowString(x,y,str_time);
}

void IdleShowSystemWeek(void)
{
	u16_t x,y,w,h;
	u8_t str_week[20] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;
	LCD_SetFontSize(FONT_SIZE_32);

	GetSystemWeekStrings(str_week);
	LCD_MeasureString(str_week,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_WEEK_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);		
	LCD_ShowString(x,y,str_week);
}

void IdleShowSystemDateTime(void)
{
	IdleShowSystemTime();
	IdleShowSystemDate();
	IdleShowSystemWeek();
}
