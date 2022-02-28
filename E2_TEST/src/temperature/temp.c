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
#include "lcd.h"
#include "screen.h"
#include "temp.h"
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

u8_t g_temp_trigger = 0;
float g_temp_skin = 0.0;
float g_temp_body = 0.0;

static void temp_get_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_check_timer, temp_get_timerout, NULL);

static void temp_get_timerout(struct k_timer *timer_id)
{
	temp_get_data_flag = true;
}

bool TempIsWorking(void)
{
	if(temp_power_flag == false)
		return false;
	else
		return true;
}

void TempRedrawData(void)
{
	if(screen_id == SCREEN_ID_TEMP)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void APPStartTemp(void)
{
	temp_start_flag = true;
}

void MenuStartTemp(void)
{
	g_temp_trigger |= TEMP_TRIGGER_BY_MENU;
	temp_start_flag = true;
}

void MenuStopTemp(void)
{
	temp_stop_flag = true;
}

void temp_init(void)
{
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
		float temp_1,temp_2;
		
		temp_get_data_flag = false;

		if(!temp_check_ok)
			return;

		ret = GetTemperature(&temp_1, &temp_2);
		if(ret)
		{
			g_temp_skin = temp_1;
			g_temp_body = temp_2;
			temp_redraw_data_flag = true;
		}
	}

	if(temp_start_flag)
	{
	#ifdef TEMP_GXTS04	
		gxts04_start();
	#endif
		temp_power_flag = true;
		k_timer_start(&temp_check_timer, K_MSEC(1*1000), K_MSEC(1*1000));
		temp_start_flag = false;
	}

	if(temp_stop_flag)
	{
	#ifdef TEMP_GXTS04	
		gxts04_stop();
	#endif
		temp_power_flag = false;
		k_timer_stop(&temp_check_timer);
		temp_stop_flag = false;
	}
	
	if(temp_redraw_data_flag)
	{
		TempRedrawData();
		temp_redraw_data_flag = false;
	}
}

