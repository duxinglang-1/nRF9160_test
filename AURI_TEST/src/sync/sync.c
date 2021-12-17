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
#include "logger.h"

SYNC_STATUS sync_state = SYNC_STATUS_IDLE;

static void SyncTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sync_timer, SyncTimerOutCallBack, NULL);

void SyncTimerOutCallBack(struct k_timer *timer_id)
{
	switch(sync_state)
	{
	case SYNC_STATUS_IDLE:
		break;

	case SYNC_STATUS_SENDING:
		break;

	case SYNC_STATUS_SENT:
	case SYNC_STATUS_FAIL:
		sync_state = SYNC_STATUS_IDLE;
		if(screen_id == SCREEN_ID_SYNC)
		{
			EnterIdleScreen();
		}
		break;
	}
}

bool SyncIsRunning(void)
{
	if(sync_state > SYNC_STATUS_IDLE)
	{
		LOGD("true");
		return true;
	}
	else
	{
		LOGD("false");
		return false;
	}
}

void SyncDataStop(void)
{
	sync_state = SYNC_STATUS_IDLE;

	k_timer_stop(&sync_timer);
}

void SyncDataStart(void)
{
	u8_t delay;
	
	LOGD("begin");

	if(sync_state != SYNC_STATUS_IDLE)
	{
		LOGD("sync is running!");
		return;
	}

	sync_state = SYNC_STATUS_SENDING;
	SyncUpdateStatus();
	SyncSendHealthData();

	ClearLeftKeyUpHandler();
}

void SyncStatusChange(SYNC_STATUS status)
{
	LOGD("status:%d", status);
	
	if(sync_state != SYNC_STATUS_IDLE)
	{
		sync_state = status;

		switch(sync_state)
		{
		case SYNC_STATUS_IDLE:
			break;
			
		case SYNC_STATUS_SENDING:
			break;
			
		case SYNC_STATUS_SENT:
		case SYNC_STATUS_FAIL:
			SyncUpdateStatus();
			k_timer_start(&sync_timer, K_SECONDS(2), NULL);
			break;
		}
	}
}
