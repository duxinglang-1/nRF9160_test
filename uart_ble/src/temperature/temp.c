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

float g_temp = 0.0;

static void temp_get_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_check_timer, temp_get_timerout, NULL);

static void temp_get_timerout(struct k_timer *timer_id)
{
	temp_get_data_flag = true;
}

void temp_init(void)
{
#ifdef TEMP_GXTS04
	temp_check_ok = gxts04_init();
#elif defined(TEMP_MAX30208)
	temp_check_ok = max30208_init();
#elif defined()
	temp_check_ok = ct1711_init();
#endif

	k_timer_start(&temp_check_timer, K_MSEC(5000), K_MSEC(5000));
}

void TempMsgProcess(void)
{
	if(temp_get_data_flag)
	{
		bool ret;
		float temperature;
		
		temp_get_data_flag = false;

		if(!temp_check_ok)
			return;
		
		ret = GetTemperature(&temperature);
		if(ret)
		{
			g_temp = temperature;
		}
	}
}

