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
#include <drivers/flash.h>
#include "datetime.h"
#include "settings.h"
#include "lcd.h"
#include "font.h"

static struct k_timer clock_timer;

sys_date_timer_t date_time = {0};
sys_date_timer_t last_date_time = {0};

extern bool sys_time_count;
extern bool update_time;
extern bool update_date;
extern bool update_date_time;

extern u8_t date_time_changed;

void UpdateSystemTime(void)
{
	printk("clock_timer_handler\n");

   	memcpy(&last_date_time, &date_time, sizeof(sys_date_timer_t));
	
	date_time.second++;
	if(date_time.second > 59)
	{
		date_time.second = 0;
		date_time.minute++;
		date_time_changed = date_time_changed|0x02;
		//date_time_changed = date_time_changed|0x04;//分针在变动的同时，时针也会同步缓慢变动
		if(date_time.minute > 59)
		{
			date_time.minute = 0;
			date_time.hour++;
			date_time_changed = date_time_changed|0x04;
			if(date_time.hour > 23)
			{
				date_time.hour = 0;
				date_time.day++;
				date_time.week++;
				if(date_time.week > 6)
					date_time.week = 0;
				date_time_changed = date_time_changed|0x08;
				if(date_time.month == 1 \
				|| date_time.month == 3 \
				|| date_time.month == 5 \
				|| date_time.month == 7 \
				|| date_time.month == 8 \
				|| date_time.month == 10 \
				|| date_time.month == 12)
				{
					if(date_time.day > 31)
					{
						date_time.day = 1;
						date_time.month++;
						date_time_changed = date_time_changed|0x10;
						if(date_time.month > 12)
						{
							date_time.month = 1;
							date_time.year++;
							date_time_changed = date_time_changed|0x20;
						}
					}
				}
				else if(date_time.month == 4 \
					|| date_time.month == 6 \
					|| date_time.month == 9 \
					|| date_time.month == 11)
				{
					if(date_time.day > 30)
					{
						date_time.day = 1;
						date_time.month++;
						date_time_changed = date_time_changed|0x10;
						if(date_time.month > 12)
						{
							date_time.month = 1;
							date_time.year++;
							date_time_changed = date_time_changed|0x20;
						}
					}
				}
				else
				{
					uint8_t Leap = date_time.year%4;
					
					if(date_time.day > (28+Leap))
					{
						date_time.day = 1;
						date_time.month++;
						date_time_changed = date_time_changed|0x10;
						if(date_time.month > 12)
						{
							date_time.month = 1;
							date_time.year++;
							date_time_changed = date_time_changed|0x20;
						}
					}
				}

				update_date_time = true;
			}
		}
	}
	date_time_changed = date_time_changed|0x01;
	update_time = true;
	
	//每分钟保存一次时间
	if((date_time_changed&0x02) != 0)
	{
		SaveSystemDateTime();
	}	
}

static void clock_timer_handler(struct k_timer *timer)
{
	sys_time_count = true;
}

void StartSystemDateTime(void)
{
	k_timer_init(&clock_timer, clock_timer_handler, NULL);
	k_timer_start(&clock_timer, K_MSEC(1000), K_MSEC(1000));
}

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

void GetSysteAmPmStrings(u8_t *str_ampm)
{
	u8_t *am_pm[2] = {"am", "pm"};

	switch(global_settings.time_format)
	{
	case TIME_FORMAT_24:
		sprintf((char*)str_ampm, "  ");
		break;
	case TIME_FORMAT_12:
		sprintf((char*)str_ampm, "%s", am_pm[date_time.hour/12]);
		break;
	}
}

void GetSystemTimeStrings(u8_t *str_time)
{
	switch(global_settings.time_format)
	{
	case TIME_FORMAT_24:
		sprintf((char*)str_time, "%02d:%02d:%02d", date_time.hour, date_time.minute, date_time.second);
		break;
	case TIME_FORMAT_12:
		sprintf((char*)str_time, "%02d:%02d:%02d", (date_time.hour>12 ? (date_time.hour-12):date_time.hour), date_time.minute, date_time.second);
		break;
	}
}

void GetSystemWeekStrings(u8_t *str_week)
{
	u8_t *week_en[7] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	u8_t *week_chn[7] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
	u8_t *week_jpn[15] = {"にちようbaiび", "げつようび", "かようび", "すbaiいようび", "もくようび", "きんようび", "どようび"};

	switch(global_settings.language)
	{
	case LANGUAGE_CHN:
		strcpy((char*)str_week, (const char*)week_chn[date_time.week]);
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
	u8_t str_ampm[5] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;
	LCD_SetFontSize(FONT_SIZE_32);

	GetSystemTimeStrings(str_time);
	LCD_MeasureString(str_time,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_TIME_SHOW_Y;
	LCD_ShowString(x,y,str_time);

	LCD_SetFontSize(FONT_SIZE_16);
	GetSysteAmPmStrings(str_ampm);
	x = x+w+5;
	y = IDLE_TIME_SHOW_Y+14;
	LCD_ShowString(x,y,str_ampm);	
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
