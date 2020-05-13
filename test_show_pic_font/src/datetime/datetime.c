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
#include "datetime.h"
//#include "nrf_nvmc.h"

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
	uint32_t date,time,week;								
	uint8_t tmpbuf[20] = {0};
	
	//nrf_nvmc_page_erase(SYSTEM_DATE_TIME_ADDR);				//擦除页
#if 1
	//nrf_nvmc_write_bytes(SYSTEM_DATE_TIME_ADDR, (uint8_t *)&systime, sizeof(sys_date_timer_t));
#else	
	sprintf((char*)tmpbuf,"%04d%02d%02d",systime.year,systime.month,systime.day);
	date = atoi((char*)tmpbuf);
	nrf_nvmc_write_word(SYSTEM_DATE_ADDR,date);		//写入日期
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf((char*)tmpbuf,"%02d%02d%02d",systime.hour,systime.minute,systime.second);
	time = atoi((char*)tmpbuf);
	nrf_nvmc_write_word(SYSTEM_TIME_ADDR,time);		//写入时间
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf((char*)tmpbuf,"%02d",systime.week);
	week = atoi((char*)tmpbuf);
	nrf_nvmc_write_word(SYSTEM_WEEK_ADDR,week);		//写入星期
#endif
}

void GetSystemDateTime(sys_date_timer_t *systime)
{
	uint32_t *pdat;
	sys_date_timer_t mytime = {0};
	
	memset(systime, 0, sizeof(sys_date_timer_t));
	
	pdat = (uint32_t *)SYSTEM_DATE_TIME_ADDR;
	mytime.year = 0x0000ffff&(*pdat);
	mytime.month = 0x000000ff&((*pdat)>>16);
	mytime.day = 0x000000ff&((*pdat)>>24);
	
	pdat += 1;
	mytime.hour = 0x000000ff&(*pdat);
	mytime.minute = 0x000000ff&((*pdat)>>8);
	mytime.second = 0x000000ff&((*pdat)>>16);
	mytime.week = 0x000000ff&((*pdat)>>24);
	
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

