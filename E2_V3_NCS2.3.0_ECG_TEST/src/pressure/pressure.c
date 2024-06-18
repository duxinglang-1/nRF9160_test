/****************************************Copyright (c)************************************************
** File Name:			    pressure.c
** Descriptions:			pressure message process source file
** Created By:				xie biao
** Created Date:			2024-06-18
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include "external_flash.h"
#include "lcd.h"
#include "screen.h"
#include "uart_ble.h"
#include "inner_flash.h"
#include "logger.h"
#include "pressure.h"
#if defined(PRESSURE_DPS368)
#include "dps368.h"
#endif

static bool pressure_check_ok = false;
static bool pressure_get_data_flag = false;
static bool pressure_start_flag = false;
static bool pressure_test_flag = false;
static bool pressure_stop_flag = false;
static bool pressure_power_flag = false;
static bool menu_start_pressure = false;
static bool ft_start_pressure = false;

bool get_pressure_ok_flag = false;

static void pressure_auto_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(pressure_stop_timer, pressure_auto_stop_timerout, NULL);
static void pressure_menu_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(pressure_menu_stop_timer, pressure_menu_stop_timerout, NULL);
static void pressure_get_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(pressure_check_timer, pressure_get_timerout, NULL);

static void pressure_auto_stop_timerout(struct k_timer *timer_id)
{
	pressure_stop_flag = true;
}

static void pressure_menu_stop_timerout(struct k_timer *timer_id)
{
}

static void pressure_get_timerout(struct k_timer *timer_id)
{
	pressure_get_data_flag = true;
}

bool PressureIsWorking(void)
{
	if(pressure_power_flag == false)
		return false;
	else
		return true;
}

void PressureStop(void)
{
	pressure_stop_flag = true;
}

#if 0
void StartTemp(TEMP_TRIGGER_SOUCE trigger_type)
{
	notify_infor infor = {0};

	infor.x = 0;
	infor.y = 0;
	infor.w = LCD_WIDTH;
	infor.h = LCD_HEIGHT;
	infor.align = NOTIFY_ALIGN_CENTER;
	infor.type = NOTIFY_TYPE_POPUP;


	if(1
	#ifdef CONFIG_FACTORY_TEST_SUPPORT
		&& !IsFTTempTesting()
	#endif
		)
	{
		StartSCC();
	}

	switch(trigger_type)
	{
	case TEMP_TRIGGER_BY_HOURLY:
		g_temp_hourly = 0.0;
	case TEMP_TRIGGER_BY_APP:
	case TEMP_TRIGGER_BY_FT:
		if(!is_wearing())
		{
			return;
		}
		break;
		
	case TEMP_TRIGGER_BY_MENU:
		if(!is_wearing())
		{
			infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
			infor.img_count = 1;

			DisplayPopUp(infor);
			return;
		}

		g_temp_menu = 0.0;
		break;
	}

	g_temp_trigger |= trigger_type;
	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	get_temp_ok_flag = false;

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

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void FTStartTemp(void)
{
	ft_start_temp = true;
}

void FTStopTemp(void)
{
	temp_stop_flag = true;
}
#endif
#endif

void pressure_init(void)
{
#ifdef PRESSURE_DPS368
	pressure_check_ok = DPS368_Init();
#endif
	if(!pressure_check_ok)
		return;
	
#ifdef PRESSURE_DEBUG
	LOGD("done!");
#endif
}

void PressureMsgProcess(void)
{
#ifdef PRESSURE_DPS368
	DPS368MsgProcess();
#endif
}

