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
#include "lsm6dso.h"
#if defined(PRESSURE_DPS368)
#include "dps368.h"
#elif defined(PRESSURE_LPS22DF)
#include "lps22df.h"
#endif

#define BARO_WINDOW_SAMPLES		10
#define BARO_DELTA_H_MIN		0.20f //0.2 meters or 20 CM

bool pressure_check_ok = false;
bool pressure_get_ok = false;
bool pressure_start_flag = false;
bool pressure_stop_flag = false;
bool pressure_interrupt_flag = false;
bool fall_result_pre_flag = false;
bool fall_pre_check = false;
bool is_FallTrigger = false;

static uint8_t chip_id = 0x00;

float g_prs = 0.0;
float g_tmp = 0.0;
float fall_prs = 0.0;
float pre_1 = 0.0;
static int prs_check = 0;
float altitude = 0.0;
float psr_rat = 0.0;
const float p0 = 101325.00;
const float factor = 0.1903;
const float scale = 44330.00;

extern volatile bool fall_result;
extern bool fall_check_flag;

pressure_ctx_t pressure_dev_ctx;

void PressureStop(void)
{
	pressure_stop_flag = true;
}

bool GetPressure(float *psr)
{
	//LOGD("pressure_check_ok:%d", pressure_check_ok);
	if(!pressure_check_ok || psr == NULL)
		return false;

#ifdef PRESSURE_DPS368
	if((dps368_settings.meas_cfg.ctrl == MEAS_CONTI_PRS)
		|| (dps368_settings.meas_cfg.ctrl == MEAS_CONTI_TMP)
		|| (dps368_settings.meas_cfg.ctrl == MEAS_CONTI_PRS_TMP)
		)
		return false;

	g_prs = 0.0;
	altitude = 0.0;
	psr_rat = 0.0;
	pressure_get_ok = false;
	static float altitude_buffer[BARO_WINDOW_SAMPLES];
	static uint8_t baro_idx = 0;

	DPS368_Start(MEAS_CMD_PSR);
#elif defined(PRESSURE_LPS22DF)
	if(lps22df_settings.meas_ctrl == MEAS_CONTINOUS)
		return false;

	g_prs = 0.0;
	pressure_get_ok = false;
	
	LPS22DF_Start(MEAS_ONE_SHOT);
#endif

	*psr = g_prs;

	psr_rat = g_prs/p0;
	altitude = scale * (1.0 - powf(psr_rat, factor));

	altitude_buffer[baro_idx] = altitude;
	baro_idx = (baro_idx + 1) % BARO_WINDOW_SAMPLES;

#ifdef PRESSURE_DEBUG
	LOGD("g_prs:%f", g_prs);
	//LOGD("g_prs:%f, altitude:%f", g_prs,altitude);
	//LOGD("alt:%f", altitude_buffer[9]);
#endif

	//*psr = g_prs;
	if(prs_check_flag)
 	{
		prs_check_flag = false;
		float oldest_alt = altitude_buffer[(baro_idx + 1) % BARO_WINDOW_SAMPLES];
		float delta_h = oldest_alt - altitude;
		#ifdef PRESSURE_DEBUG
		LOGD("DeltaH: %f", delta_h);
		#endif
		if (delta_h > BARO_DELTA_H_MIN) 
		{
			//FallTrigger();
			is_FallTrigger = true;
		}
 	}

	if(is_FallTrigger)	
	{
		if(SCC_check_ok)
		{
			SCC_check_ok = false;
			is_FallTrigger = false;

			FallTrigger();
		}
	}

	return true;
}

uint8_t Pressure_ReadID(void)
{
	if(chip_id == 0x00)
	{
	#ifdef PRESSURE_DPS368
		DPS368_GetChipID(&chip_id);
	#elif defined(PRESSURE_LPS22DF)
		LPS22DF_GetChipID(&chip_id);
	#endif
	}

	return chip_id;
}

void pressure_init(void)
{
#ifdef PRESSURE_DPS368
	pressure_check_ok = DPS368_Init();
#elif defined(PRESSURE_LPS22DF)
	pressure_check_ok = LPS22DF_Init();
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
#elif defined(PRESSURE_LPS22DF)
	LPS22DFMsgProcess();
#endif

	//GetPressure(&pre_1); // Obtain barometric value
	
	#if 1
	if (prs_check >= 3)
	{
		//if(fall_check_flag)
			//return;
			
		prs_check = 0;

		GetPressure(&pre_1); // Obtain barometric value
	}
	prs_check ++;
	#endif
	

	#if 0

	if (fall_pre_check)
	{
		fall_pre_check = false;
		GetPressure(&fall_prs); 
		fall_result_pre_flag = true;
	}
	#endif

}

