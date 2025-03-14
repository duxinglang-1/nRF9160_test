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
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif
#include "codetrans.h"
#include "logger.h"

SOS_STATUS sos_state = SOS_STATUS_IDLE;

static bool sos_trigger_flag = false;
static bool sos_start_gps_flag = false;
static bool sos_status_change_flag = false;

u8_t sos_trigger_time[16] = {0};

static void SOSTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_timer, SOSTimerOutCallBack, NULL);
static void SOSStartGPSCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_gps_timer, SOSStartGPSCallBack, NULL);

void SOSTimerOutCallBack(struct k_timer *timer_id)
{
	sos_status_change_flag = true;
}

void SOSStatusUpdate(void)
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
		#ifdef CONFIG_ANIMATION_SUPPORT	
			AnimaStopShow();
		#endif	
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
		//return;
	
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

void SOSRecLocatNotify(u8_t *strmsg)
{
	u16_t strtitle[LANGUAGE_MAX][2][40] = {
											{
												//SOS and the location XXXXXXXXXXXXX has been sent to your CareMate.
												{0x0053,0x004F,0x0053,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0065,0x0020,0x006C,0x006F,0x0063,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x0022,0x0000},
												{0x0022,0x0020,0x0068,0x0061,0x0073,0x0020,0x0062,0x0065,0x0065,0x006E,0x0020,0x0073,0x0065,0x006E,0x0074,0x0020,0x0074,0x006F,0x0020,0x0079,0x006F,0x0075,0x0072,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
											},
											{
												//SOS und der Standort XXXXXXXXXXXXX wurde an Ihren CareMate gesendet.
												{0x0053,0x004F,0x0053,0x0020,0x0075,0x006E,0x0064,0x0020,0x0064,0x0065,0x0072,0x0020,0x0053,0x0074,0x0061,0x006E,0x0064,0x006F,0x0072,0x0074,0x0020,0x0022,0x0000},
												{0x0022,0x0020,0x0077,0x0075,0x0072,0x0064,0x0065,0x0020,0x0061,0x006E,0x0020,0x0049,0x0068,0x0072,0x0065,0x006E,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x0020,0x0067,0x0065,0x0073,0x0065,0x006E,0x0064,0x0065,0x0074,0x002E,0x0000},
											},
											{
												//SOS和位置XXXXXXXXXXX已发送给您的亲友。
												{0x0053,0x004F,0x0053,0x548C,0x4F4D,0x7F6E,0x201C,0x0000},
												{0x201D,0x5DF2,0x53D1,0x9001,0x7ED9,0x60A8,0x7684,0x4EB2,0x53CB,0x3002,0x0000},
											},
										};
	u8_t strtmp[512] = {0};
	notify_infor infor = {0};

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

void SOSSChangrStatus(void)
{
	k_timer_start(&sos_timer, K_SECONDS(SOS_SENDING_TIMEOUT), NULL);
}

void SOSStart(void)
{
	u8_t delay;

	if(sos_state != SOS_STATUS_IDLE)
	{
		//LOGD("sos is running!");
		return;
	}

#ifdef CONFIG_AUDIO_SUPPORT
	SOSPlayAlarm();
#endif

	GetSystemTimeSecString(sos_trigger_time);

#ifdef CONFIG_WIFI
	sos_wait_wifi = true;
	APP_Ask_wifi_data();
#else
	SendSosAlarmData();
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

	if(sos_status_change_flag)
	{
		SOSStatusUpdate();
		sos_status_change_flag = false;
	}
}

