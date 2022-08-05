/****************************************Copyright (c)************************************************
** File Name:			    sync.c
** Descriptions:			data synchronism process source file
** Created By:				xie biao
** Created Date:			2021-12-16
** Modified Date:      		2021-12-16 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <nrf_socket.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <nrfx.h>
#include "lcd.h"
#include "sync.h"
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif
#include "screen.h"
#ifdef CONFIG_ANIMATION_SUPPORT
#include "animation.h"
#endif
#include "logger.h"

SYNC_STATUS sync_state = SYNC_STATUS_IDLE;

static bool sync_start_flag = false;
static bool sync_status_change_flag = false;
static bool sync_redraw_flag = false;

static void SyncTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sync_timer, SyncTimerOutCallBack, NULL);

void SyncTimerOutCallBack(struct k_timer *timer_id)
{
	sync_status_change_flag = true;
}

void SyncStatusUpdate(void)
{
	switch(sync_state)
	{
	case SYNC_STATUS_IDLE:
		break;

	case SYNC_STATUS_LINKING:
		SyncNetWorkCallBack(SYNC_STATUS_FAIL);
		break;

	case SYNC_STATUS_SENT:
	case SYNC_STATUS_FAIL:
		sync_state = SYNC_STATUS_IDLE;
		if(screen_id == SCREEN_ID_SYNC)
		{
			ExitSyncDataScreen();
		}
		break;
	}
}

bool SyncIsRunning(void)
{
	if(sync_state > SYNC_STATUS_IDLE)
	{
		//LOGD("true");
		return true;
	}
	else
	{
		//LOGD("false");
		return false;
	}
}

void SyncDataStop(void)
{
	k_timer_stop(&sync_timer);
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
	sync_state = SYNC_STATUS_IDLE;
}

void MenuStartSync(void)
{
	sync_start_flag = true;
}

void SyncDataStart(void)
{
	u8_t delay;
	
	if(sync_state != SYNC_STATUS_IDLE)
	{
		return;
	}

	ClearLeftKeyUpHandler();
	sync_state = SYNC_STATUS_LINKING;
	SyncUpdateStatus();
	SyncSendHealthData();
	k_timer_start(&sync_timer, K_SECONDS(60), NULL);
}

void SyncNetWorkCallBack(SYNC_STATUS status)
{
	if(sync_state != SYNC_STATUS_IDLE)
	{
		sync_state = status;

		switch(sync_state)
		{
		case SYNC_STATUS_IDLE:
			break;
			
		case SYNC_STATUS_LINKING:
			if(screen_id == SCREEN_ID_SYNC)
			{
				SyncUpdateStatus();
			}
			SyncSendHealthData();
			break;
			
		case SYNC_STATUS_SENT:
		case SYNC_STATUS_FAIL:
			k_timer_stop(&sync_timer);
		#ifdef CONFIG_ANIMATION_SUPPORT 
			AnimaStopShow();
		#endif
			if(screen_id == SCREEN_ID_SYNC)
			{
				sync_redraw_flag = true;
			}	
			k_timer_start(&sync_timer, K_SECONDS(3), NULL);
			break;
		}
	}
}

void SyncMsgProcess(void)
{
	if(sync_start_flag)
	{
		SyncDataStart();
		sync_start_flag = false;
	}

	if(sync_redraw_flag)
	{
		sync_redraw_flag = false;
		SyncUpdateStatus();
	}
	
	if(sync_status_change_flag)
	{
		SyncStatusUpdate();
		sync_status_change_flag = false;
	}
}

