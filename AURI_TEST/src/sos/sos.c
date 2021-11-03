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
#include "gps.h"
#include "screen.h"
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif
#include "logger.h"

SOS_STATUS sos_state = SOS_STATUS_IDLE;

static bool sos_trigger_flag = false;
static bool sos_start_gps_flag = false;

u8_t sos_trigger_time[16] = {0};

static void SOSTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_timer, SOSTimerOutCallBack, NULL);
static void SOSStartGPSCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_gps_timer, SOSStartGPSCallBack, NULL);

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
			EnterIdleScreen();
			break;
		
		case SOS_STATUS_CANCEL:
			sos_state = SOS_STATUS_IDLE;
			EnterIdleScreen();
			break;
		}
		
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SOS;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;

		if(sos_state != SOS_STATUS_IDLE)
			k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), NULL);
	}
}

void SOSStartGPSCallBack(struct k_timer *timer_id)
{
	sos_start_gps_flag = true;
}

#ifdef CONFIG_WIFI
void sos_get_wifi_data_reply(wifi_infor wifi_data)
{
	u8_t reply[256] = {0};
	u32_t count=3,i;

	if(wifi_data.count > 0)
		count = wifi_data.count;
	
	strcat(reply, "3,");
	for(i=0;i<count;i++)
	{
		strcat(reply, wifi_data.node[i].mac);
		strcat(reply, "&");
		strcat(reply, wifi_data.node[i].rssi);
		strcat(reply, "&");
		if(i < (count-1))
			strcat(reply, "|");
	}

	NBSendSosWifiData(reply, strlen(reply));
}
#endif

void sos_get_gps_data_reply(bool flag, struct gps_pvt gps_data)
{
	u8_t reply[256] = {0};
	u8_t tmpbuf[8] = {0};
	u32_t tmp1;
	double tmp2;

	if(!flag)
		return;
	
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

	//semicolon
	strcat(reply, ";");

	//sos trigger time
	strcat(reply, sos_trigger_time);

	//semicolon
	strcat(reply, ";");
	
	NBSendSosGpsData(reply, strlen(reply));
}

void SOSTrigger(void)
{
	sos_trigger_flag = true;
}

bool SOSIsRunning(void)
{
	if(sos_state > SOS_STATUS_IDLE)
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

void SOSSChangrStatus(void)
{
	AnimaStopShow();
	k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), NULL);
}

void SOSStart(void)
{
	u8_t delay;
	
	LOGD("begin");

	if(sos_state != SOS_STATUS_IDLE)
	{
		LOGD("sos is running!");
		return;
	}

#ifdef CONFIG_AUDIO_SUPPORT
	SOSPlayAlarm();
#endif

	GetSystemTimeSecString(sos_trigger_time);

#ifdef CONFIG_WIFI
	sos_wait_wifi = true;
	APP_Ask_wifi_data();
#endif

	lcd_sleep_out = true;
	sos_state = SOS_STATUS_SENDING;
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	EnterSOSScreen();
	
	if(nb_is_connected())
		delay = 30;
	else
		delay = 90;

	k_timer_start(&sos_gps_timer, K_SECONDS(delay), NULL);
}

void SOSMsgProc(void)
{
	if(sos_trigger_flag)
	{
		SOSStart();
		sos_trigger_flag = false;
	}

	if(sos_start_gps_flag)
	{
		sos_wait_gps = true;
		APP_Ask_GPS_Data();
		sos_start_gps_flag = false;
	}
}

