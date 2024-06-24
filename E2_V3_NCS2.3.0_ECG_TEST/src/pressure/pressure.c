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
static bool pressure_stop_flag = false;

float g_prs = 0.0;

void PressureStop(void)
{
	pressure_stop_flag = true;
}

bool GetPressure(float *psr)
{
	if(!pressure_check_ok || psr == NULL)
		return false;

#ifdef PRESSURE_DPS368
	DPS368_Start(MEAS_CMD_PSR);
#endif

	*psr = g_prs;
	return true;
}

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

