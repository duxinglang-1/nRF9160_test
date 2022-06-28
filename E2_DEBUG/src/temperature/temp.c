/****************************************Copyright (c)************************************************
** File Name:			    temp.c
** Descriptions:			temperature message process source file
** Created By:				xie biao
** Created Date:			2021-12-24
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include "external_flash.h"
#include "datetime.h"
#include "lcd.h"
#include "screen.h"
#include "temp.h"
#include "inner_flash.h"
#include "logger.h"
#if defined(TEMP_GXTS04)
#include "gxts04.h"
#elif defined(TEMP_MAX30208)
#include "max30208.h"
#elif defined(TEMP_CT1711)
#include "ct1711.h"
#endif

static bool temp_check_ok = false;
static bool temp_get_data_flag = false;
static bool temp_start_flag = false;
static bool temp_test_flag = false;
static bool temp_stop_flag = false;
static bool temp_redraw_data_flag = false;
static bool temp_power_flag = false;
static bool menu_start_temp = false;

bool get_temp_ok_flag = false;

u8_t g_temp_trigger = 0;
float g_temp_skin = 0.0;
float g_temp_body = 0.0;
float g_temp_timing = 0.0;

static void temp_auto_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_stop_timer, temp_auto_stop_timerout, NULL);
static void temp_get_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_check_timer, temp_get_timerout, NULL);

static void temp_auto_stop_timerout(struct k_timer *timer_id)
{
	if((g_temp_trigger&TEMP_TRIGGER_BY_MENU) == 0)
		temp_stop_flag = true;
}

static void temp_get_timerout(struct k_timer *timer_id)
{
	temp_get_data_flag = true;
}

void ClearAllTempRecData(void)
{
	u8_t tmpbuf[TEMP_REC2_DATA_SIZE] = {0xff};

	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	g_temp_timing = 0.0;

	SpiFlash_Write(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
}

void SetCurDayTempRecData(float data)
{
	u8_t i,tmpbuf[TEMP_REC2_DATA_SIZE] = {0};
	temp_rec2_data *p_temp = NULL;
	u16_t deca_temp = data*10;

	if((deca_temp > TEMP_MAX) || (deca_temp < TEMP_MIN))
		deca_temp = 0;
	
	SpiFlash_Read(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	p_temp = tmpbuf+6*sizeof(temp_rec2_data);
	if(((date_time.year > p_temp->year)&&(p_temp->year != 0xffff && p_temp->year != 0x0000))
		||((date_time.year == p_temp->year)&&(date_time.month > p_temp->month)&&(p_temp->month != 0xff && p_temp->month != 0x00))
		||((date_time.month == p_temp->month)&&(date_time.day > p_temp->day)&&(p_temp->day != 0xff && p_temp->day != 0x00)))
	{//记录存满。整体前挪并把最新的放在最后
		temp_rec2_data tmp_temp = {0};

	#ifdef TEMP_DEBUG
		LOGD("rec is full! temp:%0.1f", data);
	#endif
		tmp_temp.year = date_time.year;
		tmp_temp.month = date_time.month;
		tmp_temp.day = date_time.day;
		tmp_temp.deca_temp[date_time.hour] = deca_temp;
		memcpy(&tmpbuf[0], &tmpbuf[sizeof(temp_rec2_data)], 6*sizeof(temp_rec2_data));
		memcpy(&tmpbuf[6*sizeof(temp_rec2_data)], &tmp_temp, sizeof(temp_rec2_data));
		SpiFlash_Write(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	}
	else
	{
	#ifdef TEMP_DEBUG
		LOGD("rec not full! temp:%0.1f", data);
	#endif
		for(i=0;i<7;i++)
		{
			p_temp = tmpbuf + i*sizeof(temp_rec2_data);
			if((p_temp->year == 0xffff || p_temp->year == 0x0000)||(p_temp->month == 0xff || p_temp->month == 0x00)||(p_temp->day == 0xff || p_temp->day == 0x00))
			{
				p_temp->year = date_time.year;
				p_temp->month = date_time.month;
				p_temp->day = date_time.day;
				p_temp->deca_temp[date_time.hour] = deca_temp;
				SpiFlash_Write(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
				break;
			}
			
			if((p_temp->year == date_time.year)&&(p_temp->month == date_time.month)&&(p_temp->day == date_time.day))
			{
				p_temp->deca_temp[date_time.hour] = deca_temp;
				SpiFlash_Write(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
				break;
			}
		}
	}
}

void GetCurDayTempRecData(u16_t *databuf)
{
	u8_t i,tmpbuf[TEMP_REC2_DATA_SIZE] = {0};
	temp_rec2_data temp_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&temp_rec2, &tmpbuf[i*sizeof(temp_rec2_data)], sizeof(temp_rec2_data));
		if((temp_rec2.year == 0xffff || temp_rec2.year == 0x0000)||(temp_rec2.month == 0xff || temp_rec2.month == 0x00)||(temp_rec2.day == 0xff || temp_rec2.day == 0x00))
			continue;
		
		if((temp_rec2.year == date_time.year)&&(temp_rec2.month == date_time.month)&&(temp_rec2.day == date_time.day))
		{
			memcpy(databuf, temp_rec2.deca_temp, sizeof(temp_rec2.deca_temp));
			break;
		}
	}
}

bool IsInTempScreen(void)
{
	if(screen_id == SCREEN_ID_TEMP)
		return true;
	else
		return false;
}

bool TempIsWorking(void)
{
	if(temp_power_flag == false)
		return false;
	else
		return true;
}

void TempStop(void)
{
	temp_stop_flag = true;
}

void TempRedrawData(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TEMP;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
	else if(screen_id == SCREEN_ID_TEMP)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void TimerStartTemp(void)
{
	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	g_temp_timing = 0.0;
	get_temp_ok_flag = false;

	if(is_wearing())
	{
		g_temp_trigger |= TEMP_TRIGGER_BY_HOURLY;
		temp_start_flag = true;
	}
}

void APPStartTemp(void)
{
	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	get_temp_ok_flag = false;

	if(is_wearing())
	{
		g_temp_trigger |= TEMP_TRIGGER_BY_APP;
		temp_start_flag = true;
	}
}

void MenuTriggerTemp(void)
{
	if(!is_wearing())
	{
		notify_infor infor = {0};
		
		infor.x = 0;
		infor.y = 0;
		infor.w = LCD_WIDTH;
		infor.h = LCD_HEIGHT;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
		infor.img_count = 1;

		DisplayPopUp(infor);
		
		return;
	}

	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	get_temp_ok_flag = false;
	g_temp_trigger |= TEMP_TRIGGER_BY_MENU;
	temp_start_flag = true;
}

void MenuStartTemp(void)
{
	menu_start_temp = true;
}

void MenuStopTemp(void)
{
	temp_stop_flag = true;
}

void temp_init(void)
{
	get_cur_health_from_record(&last_health);
	if(last_health.timestamp.day == date_time.day)
	{
		g_temp_body = (float)(last_health.deca_temp/10.0);
	}

#ifdef TEMP_GXTS04
	temp_check_ok = gxts04_init();
#elif defined(TEMP_MAX30208)
	temp_check_ok = max30208_init();
#elif defined(TEMP_CT1711)
	temp_check_ok = ct1711_init();
#endif
}

void TempMsgProcess(void)
{
	if(temp_get_data_flag)
	{
		bool ret;
		float temp_1=0.0,temp_2=0.0;
		
		temp_get_data_flag = false;

		if(!temp_check_ok)
			return;

		ret = GetTemperature(&temp_1, &temp_2);
		if(temp_1 > 0.0)
		{
			g_temp_skin = temp_1;
			g_temp_body = temp_2;
			temp_redraw_data_flag = true;
		}

		if(ret)
		{
			temp_stop_flag = true;
			get_temp_ok_flag = true;
		}
	}

	if(menu_start_temp)
	{
		MenuTriggerTemp();
		menu_start_temp = false;
	}
	
	if(temp_start_flag)
	{
		temp_start_flag = false;
		if(temp_power_flag)
			return;
		
	#ifdef TEMP_GXTS04	
		gxts04_start();
	#endif
		temp_power_flag = true;
	
		k_timer_start(&temp_check_timer, K_MSEC(1*1000), K_MSEC(1*1000));

		if((g_temp_trigger&TEMP_TRIGGER_BY_MENU) == 0)
			k_timer_start(&temp_stop_timer, K_MSEC(TEMP_CHECK_TIMELY*60*1000), NULL);
	}

	if(temp_stop_flag)
	{
		temp_stop_flag = false;
		if(!temp_power_flag)
			return;
		
	#ifdef TEMP_GXTS04	
		gxts04_stop();
	#endif
	
		temp_power_flag = false;
		k_timer_stop(&temp_check_timer);
		k_timer_stop(&temp_stop_timer);

		if((g_temp_trigger&TEMP_TRIGGER_BY_APP) != 0)
		{
			g_temp_trigger = g_temp_trigger&(~TEMP_TRIGGER_BY_APP);
		}	
		if((g_temp_trigger&TEMP_TRIGGER_BY_MENU) != 0)
		{
			g_temp_trigger = g_temp_trigger&(~TEMP_TRIGGER_BY_MENU);
		}
		if((g_temp_trigger&TEMP_TRIGGER_BY_HOURLY) != 0)
		{
			g_temp_trigger = g_temp_trigger&(~TEMP_TRIGGER_BY_HOURLY);
			g_temp_timing = g_temp_body;
		}

		last_health.timestamp.year = date_time.year;
		last_health.timestamp.month = date_time.month; 
		last_health.timestamp.day = date_time.day;
		last_health.timestamp.hour = date_time.hour;
		last_health.timestamp.minute = date_time.minute;
		last_health.timestamp.second = date_time.second;
		last_health.timestamp.week = date_time.week;
		last_health.deca_temp = (u16_t)(g_temp_body*10);
		save_cur_health_to_record(&last_health);
	}
	
	if(temp_redraw_data_flag)
	{
		TempRedrawData();
		temp_redraw_data_flag = false;
	}
}

