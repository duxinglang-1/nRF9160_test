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
#include "settings.h"
#include "gps.h"
#include "screen.h"
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif
#include "codetrans.h"
#include "logger.h"

SOS_STATUS sos_state = SOS_STATUS_IDLE;

static bool sos_trigger_flag = false;
static bool sos_start_gps_flag = false;
#ifdef CONFIG_WIFI_SUPPORT
static bool sos_start_wifi_flag = false;
#endif
static bool sos_status_change_flag = false;

uint8_t sos_trigger_time[16] = {0};

static void SOSTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_timer, SOSTimerOutCallBack, NULL);
static void SOSStartGPSCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_gps_timer, SOSStartGPSCallBack, NULL);
#ifdef CONFIG_WIFI_SUPPORT
static void SOSStartWifiCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_wifi_timer, SOSStartWifiCallBack, NULL);
#endif

void SOSTimerOutCallBack(struct k_timer *timer_id)
{
	sos_status_change_flag = true;
}

void SOSStatusUpdate(void)
{
	k_timer_stop(&sos_timer);

	switch(sos_state)
	{
	case SOS_STATUS_IDLE:
		break;
		
	case SOS_STATUS_SENDING:
		sos_state = SOS_STATUS_SENT;
		break;
	
	case SOS_STATUS_SENT:
		if(screen_id == SCREEN_ID_SOS)
		{
		#ifdef CONFIG_ANIMATION_SUPPORT	
			AnimaStopShow();
		#endif			
		}
		sos_state = SOS_STATUS_RECEIVED;
		break;
	
	case SOS_STATUS_RECEIVED:
		sos_state = SOS_STATUS_IDLE;
		if(screen_id == SCREEN_ID_SOS)
		{
			EnterIdleScreen();
		}
		break;
	
	case SOS_STATUS_CANCEL:
		sos_state = SOS_STATUS_IDLE;
		if(screen_id == SCREEN_ID_SOS)
		{
			EnterIdleScreen();
		}
		break;
	}
	
	if(screen_id == SCREEN_ID_SOS)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SOS;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
	
	if(sos_state != SOS_STATUS_IDLE)
		k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), K_NO_WAIT);
}

void SOSStartGPSCallBack(struct k_timer *timer_id)
{
	sos_start_gps_flag = true;
}

#ifdef CONFIG_WIFI_SUPPORT
static void SOSStartWifiCallBack(struct k_timer *timer_id)
{
	sos_start_wifi_flag = true;
}

void sos_get_wifi_data_reply(wifi_infor wifi_data)
{
	uint8_t reply[256] = {0};
	uint32_t count=3,i;

	if(wifi_data.count < 3)
	{
		switch(global_settings.location_type)
		{
		case 1://only wifi
			break;
		case 2://only gps
			break;
		case 3://wifi+gps
			if(nb_is_connected())
				k_timer_start(&sos_gps_timer, K_SECONDS(30), K_NO_WAIT);
			else
				k_timer_start(&sos_gps_timer, K_SECONDS(90), K_NO_WAIT);
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
		strcat(reply, sos_trigger_time);
		NBSendSosWifiData(reply, strlen(reply));
	}
}
#endif

void sos_get_gps_data_reply(bool flag, struct nrf_modem_gnss_pvt_data_frame gps_data)
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
			k_timer_start(&sos_wifi_timer, K_SECONDS(2), K_NO_WAIT);
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

		//sos trigger time
		strcat(reply, sos_trigger_time);

		//semicolon
		strcat(reply, ";");
		
		NBSendSosGpsData(reply, strlen(reply));
	}
}

void SOSRecLocatNotify(uint8_t *strmsg)
{
	uint16_t strtitle[LANGUAGE_MAX][2][40] = {
											{
												//SOS location near "XXXXXXXXXXXXX" posted to CareMate.
												{0x0053,0x004F,0x0053,0x0020,0x006C,0x006F,0x0063,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x006E,0x0065,0x0061,0x0072,0x0020,0x0022,0x0000},
												{0x0022,0x0020,0x0070,0x006F,0x0073,0x0074,0x0065,0x0064,0x0020,0x0074,0x006F,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
											},
											{
												//SOS Standort in der N?he von "XXXXXXXXXXXXX" auf CareMate gepostet.
												{0x0053,0x004F,0x0053,0x0020,0x0053,0x0074,0x0061,0x006E,0x0064,0x006F,0x0072,0x0074,0x0020,0x0069,0x006E,0x0020,0x0064,0x0065,0x0072,0x0020,0x004E,0x00E4,0x0068,0x0065,0x0020,0x0076,0x006F,0x006E,0x0020,0x0022,0x0000},
												{0x0022,0x0020,0x0061,0x0075,0x0066,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x0020,0x0067,0x0065,0x0070,0x006F,0x0073,0x0074,0x0065,0x0074,0x002E,0x0000},
											},
											{
												//"XXXXXXXXXXX"附近的SOS已发送给您的亲友。
												{0x201C,0x0000},
												{0x201D,0x9644,0x8FD1,0x7684,0x0053,0x004F,0x0053,0x5DF2,0x53D1,0x9001,0x7ED9,0x60A8,0x7684,0x4EB2,0x53CB,0x3002,0x0000},
											},
										};
	uint8_t strtmp[512] = {0};
	notify_infor infor = {0};

	sos_state = SOS_STATUS_RECEIVED;
	SOSStatusUpdate();
	
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

		mmi_ucs2cpy(infor.text, strtitle[global_settings.language][0]);
		mmi_ucs2cat(infor.text, strtmp);
		mmi_ucs2cat(infor.text, strtitle[global_settings.language][1]);

		DisplayPopUp(infor);

		lcd_sleep_out = true;
		vibrate_on(VIB_ONCE, 100, 0);
	}
}

void SOSTrigger(void)
{
	sos_trigger_flag = true;
}

bool SOSIsRunning(void)
{
	if(sos_state > SOS_STATUS_IDLE)
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

void SOSChangrStatus(void)
{
	k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), K_NO_WAIT);
}

void SOSStart(void)
{
	uint8_t delay;

	if(sos_state != SOS_STATUS_IDLE)
	{
		//LOGD("sos is running!");
		return;
	}

#ifdef CONFIG_AUDIO_SUPPORT
	SOSPlayAlarm();
#endif

	GetSystemTimeSecString(sos_trigger_time);

	SendSosAlarmData();

	lcd_sleep_out = true;
	sos_state = SOS_STATUS_SENDING;
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	EnterSOSScreen();

	if(nb_is_connected())
		delay = 30;
	else
		delay = 90;

	switch(global_settings.location_type)
	{
	case 1://only wifi
	case 3://wifi+gps
	#ifdef CONFIG_WIFI_SUPPORT
		sos_wait_wifi = true;
		APP_Ask_wifi_data();
	#endif
		break;

	case 2://only gps
	case 4://gps+wifi
		k_timer_start(&sos_gps_timer, K_SECONDS(delay), K_NO_WAIT);
		break;
	}
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

#ifdef CONFIG_WIFI_SUPPORT
	if(sos_start_wifi_flag)
	{
		sos_wait_wifi = true;
		APP_Ask_wifi_data();
		sos_start_wifi_flag = false;
	}
#endif

	if(sos_status_change_flag)
	{
		SOSStatusUpdate();
		sos_status_change_flag = false;
	}
}

