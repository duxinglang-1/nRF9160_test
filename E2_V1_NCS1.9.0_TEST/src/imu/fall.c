/****************************************Copyright (c)************************************************
** File Name:				fall.c
** Descriptions:			sfallos message process source file
** Created By:				xie biao
** Created Date:			2021-09-24
** Modified Date:			2021-09-24 
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
#include "settings.h"
#include "sos.h"
#include "Max20353.h"
#include "Alarm.h"
#include "lcd.h"
#include "key.h"
#include "gps.h"
#include "screen.h"
#include "fall.h"
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif
#include "logger.h"

FALL_STATUS fall_state = FALL_STATUS_IDLE;

static bool fall_trigger_flag = false;
static bool fall_send_flag = false;
static bool fall_end_flag = false;
static bool fall_start_gps_flag = false;

uint8_t fall_trigger_time[16] = {0};

static void FallTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(fall_timer, FallTimerOutCallBack, NULL);
static void FallStartGPSCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(fall_gps_timer, FallStartGPSCallBack, NULL);

static void FallEnd(void)
{
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif

#ifdef CONFIG_AUDIO_SUPPORT	
	FallStopAlarm();
#endif
	k_timer_stop(&fall_timer);

	fall_state = FALL_STATUS_IDLE;
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	EnterIdleScreen();
}

static void FallTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_FALL)
	{
		switch(fall_state)
		{
		case FALL_STATUS_IDLE:
			break;
			
		case FALL_STATUS_NOTIFY:
			fall_state = FALL_STATUS_SENDING;
			fall_send_flag = true;
			ClearAllKeyHandler();
			k_timer_start(&fall_timer, K_SECONDS(FALL_SENDING_TIMEOUT), NULL);
			break;
		
		case FALL_STATUS_SENDING:
			fall_state = FALL_STATUS_SENT;
			k_timer_start(&fall_timer, K_SECONDS(FALL_SEND_OK_TIMEOUT), NULL);
			break;
			
		case FALL_STATUS_SENT:
			fall_end_flag = true;
			break;
			
		case FALL_STATUS_CANCEL:
			fall_end_flag = true;
			break;

		case FALL_STATUS_CANCELED:
			break;
		}
		
		scr_msg[SCREEN_ID_FALL].act = SCREEN_ACTION_UPDATE;
	}
}

void FallStartGPSCallBack(struct k_timer *timer_id)
{
	fall_start_gps_flag = true;
}

#ifdef CONFIG_WIFI
void fall_get_wifi_data_reply(wifi_infor wifi_data)
{
	uint8_t reply[256] = {0};
	uint32_t count=3,i;

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

	NBSendFallWifiData(reply, strlen(reply));
}
#endif

void fall_get_gps_data_reply(bool flag, struct gps_pvt gps_data)
{
	uint8_t reply[128] = {0};
	uint8_t tmpbuf[8] = {0};
	uint32_t tmp1;
	double tmp2;

	if(!flag)
		return;
	
	//latitude
	if(gps_data.latitude < 0)
	{
		strcat(reply, "-");
		gps_data.latitude = -gps_data.latitude;
	}

	tmp1 = (uint32_t)(gps_data.latitude);	//������������
	tmp2 = gps_data.latitude - tmp1;		//����С������
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (uint32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/10000));
	strcat(reply, tmpbuf);
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/100));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1));
	strcat(reply, tmpbuf);

	//semicolon
	strcat(reply, ";");
	
	//longitude
	if(gps_data.longitude < 0)
	{
		strcat(reply, "-");
		gps_data.longitude = -gps_data.longitude;
	}

	tmp1 = (uint32_t)(gps_data.longitude);	//������������
	tmp2 = gps_data.longitude - tmp1;	//����С������
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (uint32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/10000));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/100));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1));
	strcat(reply, tmpbuf);

	//semicolon
	strcat(reply, ";");

	//sos trigger time
	strcat(reply, fall_trigger_time);

	//semicolon
	strcat(reply, ";");
	
	NBSendFallGpsData(reply, strlen(reply));
}

void FallAlarmCancel(void)
{
	if(fall_state == FALL_STATUS_NOTIFY)
	{
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaStopShow();
	#endif
	#ifdef CONFIG_AUDIO_SUPPORT
		FallStopAlarm();
	#endif
		fall_state = FALL_STATUS_CANCEL;
		k_timer_start(&fall_timer, K_SECONDS(FALL_CANCEL_TIMEOUT), NULL);

		scr_msg[SCREEN_ID_FALL].act = SCREEN_ACTION_UPDATE;
	}
}

void FallStart(void)
{
	fall_trigger_flag = true;
}

void FallAlarmStart(void)
{
	if(FallIsRunning())
		return;

	fall_state = FALL_STATUS_NOTIFY;
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	EnterFallScreen();

#ifdef CONFIG_AUDIO_SUPPORT	
	if(global_settings.language == LANGUAGE_CHN)
		FallPlayAlarmCn();
	else
		FallPlayAlarmEn();
#endif

	GetSystemTimeSecString(fall_trigger_time);

	SetLeftKeyUpHandler(FallAlarmCancel);

	k_timer_start(&fall_timer, K_SECONDS(FALL_NOTIFY_TIMEOUT), NULL);
}

void FallAlarmSend(void)
{
	uint8_t delay;
	
#ifdef CONFIG_WIFI
	fall_wait_wifi = true;
	APP_Ask_wifi_data();
#endif

	if(nb_is_connected())
		delay = 30;
	else
		delay = 90;

	k_timer_start(&fall_gps_timer, K_SECONDS(delay), NULL);
}

bool FallIsRunning(void)
{
	if(fall_state > FALL_STATUS_IDLE)
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

void FallMsgProcess(void)
{
	if(fall_trigger_flag)
	{
		FallAlarmStart();
		fall_trigger_flag = false;
	}

	if(fall_start_gps_flag)
	{
		fall_wait_gps = true;
		APP_Ask_GPS_Data();
		fall_start_gps_flag = false;
	}
	
	if(fall_send_flag)
	{
		FallAlarmSend();
		fall_send_flag = false;
	}
	
	if(fall_end_flag)
	{
		FallEnd();
		fall_end_flag = false;
	}
}
