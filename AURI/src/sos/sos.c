/****************************************Copyright (c)************************************************
** File Name:			    sos.c
** Descriptions:			sos message process source file
** Created By:				xie biao
** Created Date:			2021-01-28
** Modified Date:      		2021-01-28 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <nrfx.h>
#include "sos.h"
#include "Max20353.h"
#include "Alarm.h"
#include "lcd.h"
#include "screen.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(sos, CONFIG_LOG_DEFAULT_LEVEL);

SOS_STATUS sos_state = SOS_STATUS_IDLE;

static void SOSTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_timer, SOSTimerOutCallBack, NULL);

void SOSTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_SOS)
	{
		switch(sos_state)
		{
		case SOS_STATUS_IDLE:
			break;
			
		case SOS_STATUS_SENDING:
			sos_state = SOS_STATUS_SENT;
			break;
		
		case SOS_STATUS_SENT:
			sos_state = SOS_STATUS_RECEIVED;
			break;
		
		case SOS_STATUS_RECEIVED:
			sos_state = SOS_STATUS_IDLE;
			ExitNotifyScreen();
			break;
		
		case SOS_STATUS_CANCEL:
			sos_state = SOS_STATUS_IDLE;
			ExitNotifyScreen();
			break;
		}
		
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SOS;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;

		if(sos_state != SOS_STATUS_IDLE)
			k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), NULL);
	}
}

void SOSStart(void)
{
	sos_state = SOS_STATUS_SENDING;

	EnterSOSScreen();
	k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), NULL);
}
