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
#include <nrf_socket.h>
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

bool sos_wait_gps = false;

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

void sos_get_location_data_reply(nrf_gnss_pvt_data_frame_t gps_data)
{
	u8_t reply[128] = {0};
	u8_t tmpbuf[8] = {0};
	u32_t tmp1;
	double tmp2;

	//latitude
	if(gps_data.latitude < 0)
	{
		strcat(reply, "-");
		gps_data.latitude = -gps_data.latitude;
	}

	tmp1 = (u32_t)(gps_data.latitude);	//经度整数部分
	tmp2 = gps_data.latitude - tmp1;		//经度小数部分
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (u32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/10000));
	strcat(reply, tmpbuf);
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/100));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1));
	strcat(reply, tmpbuf);

	//semicolon
	strcat(reply, ";");
	
	//longitude
	if(gps_data.longitude < 0)
	{
		strcat(reply, "-");
		gps_data.longitude = -gps_data.longitude;
	}

	tmp1 = (u32_t)(gps_data.longitude);	//经度整数部分
	tmp2 = gps_data.longitude - tmp1;	//经度小数部分
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (u32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/10000));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/100));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1));
	strcat(reply, tmpbuf);	

	NBSendSosData(reply, strlen(reply));
}

void SOSStart(void)
{
	sos_state = SOS_STATUS_SENDING;

	EnterSOSScreen();

	SOS_Ask_GPS_Data();
	k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), NULL);
}

void SOSMsgProc(void)
{
	
}

