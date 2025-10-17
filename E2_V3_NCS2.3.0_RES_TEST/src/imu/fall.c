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
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <nrf_socket.h>
#include <zephyr/drivers/gpio.h>
#include <nrfx.h>
#include "settings.h"
#include "sos.h"
#include "Max20353.h"
#ifdef CONFIG_ALARM_SUPPORT
#include "Alarm.h"
#endif
#include "lcd.h"
#include "key.h"
#include "gps.h"
#include "screen.h"
#include "strdef.h"
#include "fall.h"
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif
#include "codetrans.h"
#include "logger.h"

//#define FALL_DEBUG

FALL_STATUS fall_state = FALL_STATUS_IDLE;

static bool fall_trigger_flag = false;
static bool fall_send_flag = false;
static bool fall_end_flag = false;
static bool fall_start_gps_flag = false;
#ifdef CONFIG_WIFI_SUPPORT
static bool fall_start_wifi_flag = false;
#endif
static bool fall_status_change_flag = false;

uint8_t fall_trigger_time[16] = {0};

static void FallTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(fall_timer, FallTimerOutCallBack, NULL);
static void FallStartGPSCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(fall_gps_timer, FallStartGPSCallBack, NULL);
#ifdef CONFIG_WIFI_SUPPORT
static void FallStartWifiCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(fall_wifi_timer, FallStartWifiCallBack, NULL);
#endif

static void FallVibOn(void)
{
	vibrate_on(VIB_RHYTHMIC, 1000, 500);
}

static void FallVibOff(void)
{
	vibrate_off();
}

static void FallEnd(void)
{
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif
#ifdef CONFIG_AUDIO_SUPPORT	
	FallStopAlarm();
#endif

	FallVibOff();

	k_timer_stop(&fall_timer);

	fall_state = FALL_STATUS_IDLE;
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	EnterIdleScreen();
}

static void FallTimerOutCallBack(struct k_timer *timer_id)
{
	fall_status_change_flag = true;
}

static void FallStatusUpdate(void)
{
#ifdef FALL_DEBUG
	LOGD("fall_state:%d", fall_state);
#endif

	k_timer_stop(&fall_timer);

	switch(fall_state)
	{
	case FALL_STATUS_IDLE:
		break;

	case FALL_STATUS_NOTIFY:
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaStop();
	#endif
	#ifdef CONFIG_AUDIO_SUPPORT
		FallStopAlarm();
	#endif
		FallVibOff();

		fall_send_flag = true;
		fall_state = FALL_STATUS_IDLE;
		if(screen_id == SCREEN_ID_FALL)
		{
			EnterIdleScreen();
		}
		break;

	case FALL_STATUS_SENDING:
		fall_state = FALL_STATUS_SENT;
		break;

	case FALL_STATUS_SENT:
		if(screen_id == SCREEN_ID_FALL)
		{
		#ifdef CONFIG_ANIMATION_SUPPORT 
			AnimaStop();
		#endif			
		}
		
		FallVibOff();
		fall_state = FALL_STATUS_RECEIVED;
		break;

	case FALL_STATUS_RECEIVED:
		fall_state = FALL_STATUS_IDLE;
		if(screen_id == SCREEN_ID_FALL)
		{
			EnterIdleScreen();
		}
		break;

	case FALL_STATUS_CANCEL:
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaStop();
	#endif
	#ifdef CONFIG_AUDIO_SUPPORT
		FallStopAlarm();
	#endif
		FallVibOff();
	
		fall_state = FALL_STATUS_IDLE;
		if(screen_id == SCREEN_ID_FALL)
		{
			EnterIdleScreen();
		}
		break;
	}

	if(screen_id == SCREEN_ID_FALL)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}

	if(fall_state != FALL_STATUS_IDLE)
		k_timer_start(&fall_timer, K_SECONDS(FALL_SENDING_TIMEOUT), K_NO_WAIT);

}

void FallStartGPSCallBack(struct k_timer *timer_id)
{
	fall_start_gps_flag = true;
}

#ifdef CONFIG_WIFI_SUPPORT
static void FallStartWifiCallBack(struct k_timer *timer_id)
{
	fall_start_wifi_flag = true;
}

void fall_get_wifi_data_reply(wifi_infor wifi_data)
{
	uint8_t reply[256] = {0};
	uint32_t count=3,i;

#ifdef FALL_DEBUG
	LOGD("begin");
#endif

	if(wifi_data.count < WIFI_LOCAL_MIN_COUNT)
	{
		switch(global_settings.location_type)
		{
		case 1://only wifi
			break;
		case 2://only gps
			break;
		case 3://wifi+gps
			if(nb_is_connected())
				k_timer_start(&fall_gps_timer, K_SECONDS(30), K_NO_WAIT);
			else
				k_timer_start(&fall_gps_timer, K_SECONDS(90), K_NO_WAIT);
			break;
		case 4://gps+wifi
			break;	
		}
	}
	else
	{
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

		strcat(reply, ",");
		strcat(reply, fall_trigger_time);
		NBSendFallWifiData(reply, strlen(reply));
	}
}
#endif

void fall_get_gps_data_reply(bool flag, struct nrf_modem_gnss_pvt_data_frame gps_data)
{
	uint8_t reply[256] = {0};
	uint8_t tmpbuf[8] = {0};
	uint32_t tmp1;
	double tmp2;

	if(!flag)
	{
		switch(global_settings.location_type)
		{
		case 1://only wifi
			break;
		case 2://only gps
			break;
		case 3://wifi+gps
			break;
		case 4://gps+wifi
		#ifdef CONFIG_WIFI_SUPPORT
			k_timer_start(&fall_wifi_timer, K_SECONDS(2), K_NO_WAIT);
		#endif
			break;
		}
	}
	else
	{
		//latitude
		if(gps_data.latitude < 0)
		{
			strcat(reply, "-");
			gps_data.latitude = -gps_data.latitude;
		}

		tmp1 = (uint32_t)(gps_data.latitude);	//经度整数部分
		tmp2 = gps_data.latitude - tmp1;		//经度小数部分
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

		tmp1 = (uint32_t)(gps_data.longitude);	//经度整数部分
		tmp2 = gps_data.longitude - tmp1;	//经度小数部分
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

		//fall trigger time
		strcat(reply, fall_trigger_time);

		//semicolon
		strcat(reply, ";");
		
		NBSendFallGpsData(reply, strlen(reply));
	}
}

void FallRecLocatNotify(uint8_t *strmsg)
{
	uint8_t strtmp[512] = {0};
	notify_infor infor = {0};

	fall_state = FALL_STATUS_RECEIVED;
	FallStatusUpdate();
	
	if(IsInIdleScreen())
	{
		mmi_chset_convert(
							MMI_CHSET_UTF8, 
							MMI_CHSET_UCS2,
							strmsg,
							strtmp,
							sizeof(strtmp));

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_20);
	#else		
		LCD_SetFontSize(FONT_SIZE_16);
	#endif

		infor.w = 200;
		infor.h = 120;
		infor.x = (LCD_WIDTH-infor.w)/2;
		infor.y = (LCD_HEIGHT-infor.h)/2;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		infor.img_count = 0;

		StrCpyByID(infor.text, STR_ID_FALL_LOCATION_FRONT);
		mmi_ucs2cat(infor.text, strtmp);
		StrCatByID(infor.text, STR_ID_FALL_LOCATION_REAR);

		DisplayPopUp(infor);

		lcd_sleep_out = true;
		vibrate_on(VIB_ONCE, 100, 0);
	}
}

void FallAlarmConfirm(void)
{
	if(fall_state == FALL_STATUS_NOTIFY)
	{
		FallStatusUpdate();
	}
}

void FallAlarmCancel(void)
{
	if(fall_state == FALL_STATUS_NOTIFY)
	{
		fall_state = FALL_STATUS_CANCEL;
		FallStatusUpdate();
	}
}

void FallChangrStatus(void)
{
	k_timer_start(&fall_timer, K_SECONDS(FALL_NOTIFY_TIMEOUT), K_NO_WAIT);
}

void FallTrigger(void)
{
	fall_trigger_flag = true;
}

void FallAlarmStart(void)
{
#ifdef FALL_DEBUG
	LOGD("begin");
#endif

	if(FallIsRunning())
		return;

	lcd_sleep_out = true;
	fall_state = FALL_STATUS_NOTIFY;
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	EnterFallScreen();
	
	FallVibOn();
#ifdef CONFIG_AUDIO_SUPPORT	
	if(global_settings.language == LANGUAGE_CN)
		FallPlayAlarmCn();
	else
		FallPlayAlarmEn();
#endif
}

void FallAlarmSend(void)
{
	uint8_t delay;

#ifdef FALL_DEBUG
	LOGD("begin");
#endif

	GetSystemTimeSecString(fall_trigger_time);
	SendFallAlarmData();

	if(nb_is_connected())
		delay = 30;
	else
		delay = 90;

	switch(global_settings.location_type)
	{
	case 1://only wifi
	case 3://wifi+gps
	#ifdef CONFIG_WIFI_SUPPORT
		fall_wait_wifi = true;
		APP_Ask_wifi_data();
	#endif
		break;

	case 2://only gps
	case 4://gps+wifi
		k_timer_start(&fall_gps_timer, K_SECONDS(delay), K_NO_WAIT);
		break;
	}
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

#ifdef CONFIG_WIFI_SUPPORT
	if(fall_start_wifi_flag)
	{
		fall_wait_wifi = true;
		APP_Ask_wifi_data();
		fall_start_wifi_flag = false;
	}
#endif
	
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

	if(fall_status_change_flag)
	{
		FallStatusUpdate();
		fall_status_change_flag = false;
	}
}
