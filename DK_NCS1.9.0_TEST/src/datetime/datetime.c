/****************************************Copyright (c)************************************************
** File name:			     datetime.c
** Last modified Date:          
** Last Version:		   
** Descriptions:		   使用的ncs版本-1.2.0
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
#ifdef CONFIG_IMU_SUPPORT
#include "lsm6dso.h"
#endif
#include "max20353.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif
#ifdef CONFIG_TEMP_SUPPORT
#include "temp.h"
#endif
#include "screen.h"
#include "nb.h"
#include "ucs2.h"
#include "logger.h"

//#define DATETIME_DEBUG

#define SEC_START_YEAR		1970
#define SEC_START_MONTH		1
#define SEC_START_DAY		1
#define SEC_START_HOUR		0
#define SEC_START_MINUTE	0
#define SEC_START_SECOND	0

#define SEC_PER_MINUTE		60
#define SEC_PER_HOUR		(SEC_PER_MINUTE*60)
#define SEC_PER_DAY			(SEC_PER_HOUR*24)
#define SEC_PER_SMALL_YEAR	(SEC_PER_DAY*365)
#define SEC_PER_BIG_YEAR	(SEC_PER_DAY*366)

sys_date_timer_t date_time = {0};
sys_date_timer_t last_date_time = {0};

bool update_time = false;
bool update_date = false;
bool update_week = false;
bool update_date_time = false;
bool sys_time_count = false;
bool show_date_time_first = true;

uint8_t date_time_changed = 0;//通过位来判断日期时间是否有变化，从第6位算起，分表表示年月日时分秒
uint64_t laststamp = 0;

static void clock_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(clock_timer, clock_timer_handler, NULL);


uint8_t CheckYearIsLeap(uint32_t years)
{
	if(((years%4 == 0) && (years%100 != 0))||(years%400 == 0))
		return 1;
	else
		return 0;
}

uint8_t GetWeekDayByDate(sys_date_timer_t date)
{
	uint8_t flag=0,index=SYSTEM_STARTING_WEEK;//1900年1月1日是星期一 0=sunday
	uint32_t i,count=0;
	
	if(date.year < SYSTEM_STARTING_YEAR)
		return 0xff;

	for(i=SYSTEM_STARTING_YEAR;i<date.year;i++)
	{
		if(((i%4 == 0)&&(i%100 != 0))||(i%400 == 0))	//闰年366天
			count += 366;
		else
			count += 365;
	}

	if(CheckYearIsLeap(date.year))
		flag = 1;
	
	switch(date.month)
	{
	case 1:
		count += 0;
		break;
	case 2:
		count += 31;
		break;
	case 3:
		count += (31+(28+flag));
		break;
	case 4:
		count += (2*31+(28+flag));
		break;
	case 5:
		count += (2*31+30+(28+flag));
		break;
	case 6:
		count += (3*31+30+(28+flag));
		break;
	case 7:
		count += (3*31+2*30+(28+flag));
		break;
	case 8:
		count += (4*31+2*30+(28+flag));
		break;
	case 9:
		count += (5*31+2*30+(28+flag));
		break;
	case 10:
		count += (5*31+3*30+(28+flag));
		break;
	case 11:
		count += (6*31+3*30+(28+flag));
		break;
	case 12:
		count += (6*31+4*30+(28+flag));
		break;			
	}

	count += (date.day-1);
	
	count = count%7;
	index = (index+count)%7;
	
	return index;
}

void DateIncreaseOne(sys_date_timer_t *date)
{
	(*date).day++;
	if((*date).month == 1 \
	|| (*date).month == 3 \
	|| (*date).month == 5 \
	|| (*date).month == 7 \
	|| (*date).month == 8 \
	|| (*date).month == 10 \
	|| (*date).month == 12)
	{
		if((*date).day > 31)
		{
			(*date).day = 1;
			(*date).month++;
			if((*date).month > 12)
			{
				(*date).month = 1;
				(*date).year++;
			}
		}
	}
	else if((*date).month == 4 \
		|| (*date).month == 6 \
		|| (*date).month == 9 \
		|| (*date).month == 11)
	{
		if((*date).day > 30)
		{
			(*date).day = 1;
			(*date).month++;
			if((*date).month > 12)
			{
				(*date).month = 1;
				(*date).year++;
			}
		}
	}
	else
	{
		uint8_t Leap = 0;

		if(CheckYearIsLeap((*date).year))
			Leap = 1;
		
		if((*date).day > (28+Leap))
		{
			(*date).day = 1;
			(*date).month++;
			if((*date).month > 12)
			{
				(*date).month = 1;
				(*date).year++;
			}
		}
	}	

	(*date).week = GetWeekDayByDate((*date));

	if(screen_id == SCREEN_ID_IDLE)
		scr_msg[screen_id].para |= (SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK);
}

void DateDecreaseOne(sys_date_timer_t *date)
{
	if((*date).day > 1)
	{
		(*date).day--;
	}
	else
	{
		if((*date).month == 1 \
		|| (*date).month == 2 \
		|| (*date).month == 4 \
		|| (*date).month == 6 \
		|| (*date).month == 8 \
		|| (*date).month == 9 \
		|| (*date).month == 11)
		{
			(*date).day = 31;
			if((*date).month == 1)
			{
				(*date).year--;
				(*date).month = 12;
			}
			else
			{
				(*date).month--;
			}
		}
		else if((*date).month == 5 \
			|| (*date).month == 7 \
			|| (*date).month == 10 \
			|| (*date).month == 12)
		{
			(*date).day = 30;
			(*date).month--;
		}
		else
		{
			uint8_t Leap = 0;

			if(CheckYearIsLeap((*date).year))
				Leap = 1;

			(*date).day = (28+Leap);
			(*date).month--;
		}			
	}

	(*date).week = GetWeekDayByDate((*date));

	if(screen_id == SCREEN_ID_IDLE)
		scr_msg[screen_id].para |= (SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK);
}

void TimeIncrease(sys_date_timer_t *date, uint32_t minutes)
{
	uint8_t m_add,h_add,day_add;
	
	m_add = minutes%60;
	h_add = minutes/60;
	day_add = h_add/24;

#ifdef DATETIME_DEBUG
	LOGD("m_add:%d, h_add:%d", m_add, h_add);
#endif

	(*date).minute += m_add;
	if((*date).minute > 59)
	{
		(*date).minute = (*date).minute%60;
		h_add++;
	}
	
	(*date).hour += h_add;
	if((*date).hour > 23)
	{
		(*date).hour = (*date).hour%24;
		day_add++;
	}	
	
	while(day_add>0)
	{
		DateIncreaseOne(date);
		day_add--;
	}

	(*date).week = GetWeekDayByDate((*date));
	
	if(screen_id == SCREEN_ID_IDLE)
		scr_msg[screen_id].para |= (SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK);	
}

void TimeDecrease(sys_date_timer_t *date, uint32_t minutes)
{
	uint8_t m_dec,h_dec,day_dec;

	m_dec = minutes%60;
	h_dec = minutes/60;
	day_dec = h_dec/24;

	if((*date).minute >= m_dec)
	{
		(*date).minute -= m_dec;
	}
	else
	{
		(*date).minute = ((*date).minute+60)-m_dec;
		h_dec++;
	}

	if((*date).hour >= h_dec)
	{
		(*date).hour -= h_dec;
	}
	else
	{
		(*date).hour = ((*date).hour+24)-h_dec;
		day_dec++;
	}
	
	while(day_dec>0)
	{
		DateDecreaseOne(date);
		day_dec--;
	}

	(*date).week = GetWeekDayByDate((*date));
	
	if(screen_id == SCREEN_ID_IDLE)
		scr_msg[screen_id].para |= (SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK);	
}

void RedrawSystemTime(void)
{
	if(screen_id == SCREEN_ID_IDLE)
		scr_msg[screen_id].para |= (SCREEN_EVENT_UPDATE_TIME|SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK);
}

void UpdateSystemTime(void)
{
	uint64_t timestamp,timeskip;
	static uint64_t timeoffset=0;

   	memcpy(&last_date_time, &date_time, sizeof(sys_date_timer_t));

	if(screen_id == SCREEN_ID_IDLE)
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TIME;

	timestamp = k_uptime_get();
	timeskip = timestamp - laststamp;
	laststamp = timestamp;

	timeoffset += (timeskip%1000);
	if(timeoffset >= 1000)
	{
		timeskip += 1000;
		timeoffset -= 1000;
	}

	date_time.second += (timeskip/1000);
	if(date_time.second > 59)
	{
		date_time.minute += date_time.second/60;
		date_time.second = date_time.second%60;
		date_time_changed = date_time_changed|0x02;
		//date_time_changed = date_time_changed|0x04;//分针在变动的同时，时针也会同步缓慢变动
		if(date_time.minute > 59)
		{
			date_time.hour += date_time.minute/60;
			date_time.minute = date_time.minute%60;
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
					uint8_t Leap = 0;

					if(CheckYearIsLeap(date_time.year))
						Leap = 1;
					
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
				
				if(screen_id == SCREEN_ID_IDLE)
					scr_msg[screen_id].para |= (SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK);
			}
		}
	}
	date_time_changed = date_time_changed|0x01;
	
	//每分钟保存一次时间
	if((date_time_changed&0x02) != 0)
	{
		SaveSystemDateTime();
		date_time_changed = date_time_changed&0xFD;

	#ifndef NB_SIGNAL_TEST
		if(1
		  #ifdef CONFIG_FOTA_DOWNLOAD
			&& (!fota_is_running())
		  #endif/*CONFIG_FOTA_DOWNLOAD*/
		  #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
			&& (!dl_is_running())
		  #endif/*CONFIG_DATA_DOWNLOAD_SUPPORT*/
		)
		{
			//The sensor needs to be turned on in advance. 
			//For example, the data at 2:00 should be measured at 1:(48-TEMP_CHECK_TIMELY).
			bool check_flag = false;
			
			switch(global_settings.health_interval)
			{
			case 60://0/1/2/3/4/5/6/7/8/9/10/11/12/13/14/15/16/17/18/19/20/21/22/23
				check_flag = true;
				break;
			case 120://1/3/5/7/9/11/13/15/17/19/21/23
				if(date_time.hour%2 == 0)
				{
					check_flag = true;
				}
				break;
			case 180://2/5/8/11/14/17/20/23
				if(date_time.hour%3 == 1)
				{
					check_flag = true;
				}
				break;
			case 240://3/7/11/15/19/23
				if(date_time.hour%4 == 2)
				{
					check_flag = true;
				}
				break;
			case 360://5/11/17/23
				if(date_time.hour%6 == 4)
				{
					check_flag = true;
				}
				break;
			}

			if(check_flag == true)
			{
			#ifdef CONFIG_TEMP_SUPPORT
				if(date_time.minute == 48-TEMP_CHECK_TIMELY)
				{	
					TimerStartTemp();
				}
			#endif
			#ifdef CONFIG_PPG_SUPPORT
				if(date_time.minute == 49-PPG_CHECK_HR_TIMELY)
				{
					TimerStartHr();
				}
				if(date_time.minute == 53-PPG_CHECK_BPT_TIMELY)
				{
					TimerStartBpt();
				}
				if(date_time.minute == 59-PPG_CHECK_SPO2_TIMELY)
				{
					TimerStartSpo2();
				}
			#endif/*CONFIG_PPG_SUPPORT*/
			}
			
			AlarmRemindCheck(date_time);
			//TimeCheckSendLocationData();
		}
	#endif
	}

	if((date_time_changed&0x04) != 0)
	{		
		date_time_changed = date_time_changed&0xFB;

	#ifndef NB_SIGNAL_TEST
		if(1
  			#ifdef CONFIG_FOTA_DOWNLOAD
				&& (!fota_is_running())
 			#endif/*CONFIG_FOTA_DOWNLOAD*/
			#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
				&& (!dl_is_running())
			#endif/*CONFIG_DATA_DOWNLOAD_SUPPORT*/
			)
		{
			bool send_flag = false;

		#ifdef CONFIG_TEMP_SUPPORT
			SetCurDayTempRecData(g_temp_timing);
			g_temp_timing = 0.0;
		#endif
		#ifdef CONFIG_PPG_SUPPORT
			SetCurDayHrRecData(g_hr_timing);
			g_hr_timing = 0;
			SetCurDaySpo2RecData(g_spo2_timing);
			g_spo2_timing = 0;
			SetCurDayBptRecData(g_bpt_timing);
			memset(&g_bpt_timing, 0, sizeof(g_bpt_timing));
		#endif		
		#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
			if((date_time_changed&0x08) != 0)
				g_steps = 0;
			SetCurDayStepRecData(g_steps);
			g_steps = 0;
		#endif
			
			switch(global_settings.health_interval)
			{
			case 60://0/1/2/3/4/5/6/7/8/9/10/11/12/13/14/15/16/17/18/19/20/21/22/23
				send_flag = true;
				break;
			case 120://1/3/5/7/9/11/13/15/17/19/21/23
				if(date_time.hour%2 == 1)
				{
					send_flag = true;
				}
				break;
			case 180://2/5/8/11/14/17/20/23
				if(date_time.hour%3 == 2)
				{
					send_flag = true;
				}
				break;
			case 240://3/7/11/15/19/23
				if(date_time.hour%4 == 3)
				{
					send_flag = true;
				}
				break;
			case 360://5/11/17/23
				if(date_time.hour%6 == 5)
				{
					send_flag = true;
				}
				break;
			}

			if(send_flag 
				|| (date_time.hour == 00) //xb add 2022-05-25 Before the date changes, the data of the current day is forced to be uploaded to prevent data loss.
				)
			{
				TimeCheckSendHealthData();
			}
		}
	#endif		
	}

	if((date_time_changed&0x08) != 0)
	{
		date_time_changed = date_time_changed&0xF7;
	#ifdef CONFIG_IMU_SUPPORT
		reset_steps = true;
	#endif
	}
}

static void clock_timer_handler(struct k_timer *timer)
{
	sys_time_count = true;
}

void StartSystemDateTime(void)
{
	k_timer_start(&clock_timer, K_MSEC(1000), K_MSEC(1000));
}

void StopSystemDateTime(void)
{
	k_timer_stop(&clock_timer);
}

bool CheckSystemDateTimeIsValid(sys_date_timer_t systime)
{
	bool ret = true;

	if((systime.year<SYSTEM_STARTING_YEAR || systime.year>9999)
		|| ((systime.month==0)||(systime.month>12)) 
		|| (systime.day==0) 
		|| ((systime.day>31)&&((systime.month==1)||(systime.month==3)||(systime.month==5)||(systime.month==7)||(systime.month==8)||(systime.month==10)||(systime.month==12)))
		|| ((systime.day>30)&&((systime.month==4)||(systime.month==6)||(systime.month==9)||(systime.month==11)))
		|| ((systime.day>29)&&((systime.month==2)&&(systime.year%4==0)))
		|| ((systime.day>28)&&((systime.month==2)&&(systime.year%4!=0)))
		|| ((systime.hour>23)||(systime.minute>59)||(systime.second>59))
		|| (systime.week>6))
	{
		ret = false;
	}
	
	return ret;
}

void GetSystemTimeSecString(uint8_t *str_utc)
{
	uint32_t i;
	uint32_t total_sec,total_day=0;

	sprintf(str_utc, "%04d%02d%02d%02d%02d%02d", 
						date_time.year,
						date_time.month,
						date_time.day,
						date_time.hour,
						date_time.minute,
						date_time.second);
}

void GetSystemDateStrings(uint8_t *str_date)
{
	uint8_t tmpbuf[128] = {0};
	
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

#ifdef FONTMAKER_UNICODE_FONT
	strcpy(tmpbuf, str_date);
	mmi_asc_to_ucs2(str_date, tmpbuf);
#endif
}

void GetSysteAmPmStrings(uint8_t *str_ampm)
{
	uint8_t flag = 0;
	uint8_t *am_pm[2] = {"am", "pm"};
	uint8_t tmpbuf[128] = {0};

	if(date_time.hour > 12)
		flag = 1;
	
	switch(global_settings.time_format)
	{
	case TIME_FORMAT_24:
		sprintf((char*)str_ampm, "  ");
		break;
	case TIME_FORMAT_12:
		sprintf((char*)str_ampm, "%s", am_pm[flag]);
		break;
	}

#ifdef FONTMAKER_UNICODE_FONT
	strcpy(tmpbuf, str_ampm);
	mmi_asc_to_ucs2(str_ampm, tmpbuf);
#endif

}

void GetSystemTimeStrings(uint8_t *str_time)
{
	uint8_t tmpbuf[128] = {0};
	
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

void GetSystemWeekStrings(uint8_t *str_week)
{
	uint8_t *week_en[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	uint8_t *week_chn[7] = {"日", "一", "二", "三", "四", "五", "六"};
	uint8_t tmpbuf[128] = {0};

	switch(global_settings.language)
	{
	case LANGUAGE_CHN:
		strcpy((char*)str_week, (const char*)week_chn[date_time.week]);
		break;
	case LANGUAGE_EN:
		strcpy((char*)str_week, (const char*)week_en[date_time.week]);
		break;
	}
}

void TimeMsgProcess(void)
{
	if(sys_time_count)
	{
		sys_time_count = false;
		UpdateSystemTime();
		
		if(lcd_is_sleeping)
			return;
		
		if(screen_id == SCREEN_ID_IDLE)
		{
			if(charger_is_connected&&(g_chg_status == BAT_CHARGING_PROGRESS))
				scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BAT;
			
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
		}
		
	}
}
