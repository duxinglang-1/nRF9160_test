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
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <nrf_socket.h>
#include <zephyr/drivers/gpio.h>
#include <nrfx.h>
#include "sos.h"
#include "Max20353.h"
#ifdef CONFIG_ALARM_SUPPORT
#include "Alarm.h"
#endif
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
static bool sos_wait_wifi_addr_flag = false;
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
static void SOSWaitWifiAddrCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sos_wait_wifi_addr_timer, SOSWaitWifiAddrCallBack, NULL);
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
			AnimaStop();
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

static void SOSWaitWifiAddrCallBack(struct k_timer *timer_id)
{
	sos_wait_wifi_addr_flag = true;
}

void sos_get_wifi_data_reply(wifi_infor wifi_data)
{
	uint8_t reply[256] = {0};
	uint32_t count=3,i;

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

		switch(global_settings.location_type)
		{
		case 1://only wifi
		case 2://only gps
		case 4://gps+wifi
			break;	
		case 3://wifi+gps
			if(nb_is_connected())
				k_timer_start(&sos_wait_wifi_addr_timer, K_SECONDS(30), K_NO_WAIT);
			else
				k_timer_start(&sos_wait_wifi_addr_timer, K_SECONDS(90), K_NO_WAIT);
			break;
		}
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
											#ifndef FW_FOR_CN
												{
													//SOS location near "XXXXXXXXXXXXX" posted to CareMate.
													{0x0053,0x004F,0x0053,0x0020,0x006C,0x006F,0x0063,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x006E,0x0065,0x0061,0x0072,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x006F,0x0073,0x0074,0x0065,0x0064,0x0020,0x0074,0x006F,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//SOS -Standort in der N?he von "xxxxxxxxxxxxxx" geschrieben an CareMate.
													{0x0053,0x004F,0x0053,0x0020,0x002D,0x0053,0x0074,0x0061,0x006E,0x0064,0x006F,0x0072,0x0074,0x0020,0x0069,0x006E,0x0020,0x0064,0x0065,0x0072,0x0020,0x004E,0x00E4,0x0068,0x0065,0x0020,0x0076,0x006F,0x006E,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0067,0x0065,0x0073,0x0063,0x0068,0x0072,0x0069,0x0065,0x0062,0x0065,0x006E,0x0020,0x0061,0x006E,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//Emplacement SOS près de "xxxxxxxxxxxxxx" publié sur CareMate.
													{0x0045,0x006D,0x0070,0x006C,0x0061,0x0063,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0053,0x004F,0x0053,0x0020,0x0070,0x0072,0x00E8,0x0073,0x0020,0x0064,0x0065,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x0075,0x0062,0x006C,0x0069,0x00E9,0x0020,0x0073,0x0075,0x0072,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//Posizione SOS vicino a "xxxxxxxxxxxx" pubblicato a CareMate.
													{0x0050,0x006F,0x0073,0x0069,0x007A,0x0069,0x006F,0x006E,0x0065,0x0020,0x0053,0x004F,0x0053,0x0020,0x0076,0x0069,0x0063,0x0069,0x006E,0x006F,0x0020,0x0061,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x0075,0x0062,0x0062,0x006C,0x0069,0x0063,0x0061,0x0074,0x006F,0x0020,0x0061,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//Ubicación SOS cerca de "XXXXXXXXXXXXX" Publicado en CareMate.
													{0x0055,0x0062,0x0069,0x0063,0x0061,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0053,0x004F,0x0053,0x0020,0x0063,0x0065,0x0072,0x0063,0x0061,0x0020,0x0064,0x0065,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0050,0x0075,0x0062,0x006C,0x0069,0x0063,0x0061,0x0064,0x006F,0x0020,0x0065,0x006E,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//Localiza??o do SOS Perto de "XXXXXXXXXXXXX" Postado no CareMate.
													{0x004C,0x006F,0x0063,0x0061,0x006C,0x0069,0x007A,0x0061,0x00E7,0x00E3,0x006F,0x0020,0x0064,0x006F,0x0020,0x0053,0x004F,0x0053,0x0020,0x0050,0x0065,0x0072,0x0074,0x006F,0x0020,0x0064,0x0065,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0050,0x006F,0x0073,0x0074,0x0061,0x0064,0x006F,0x0020,0x006E,0x006F,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//Lokalizacja SOS w pobli?u "XXXXXXXXXXXXX" opublikowana w CareMate.
													{0x004C,0x006F,0x006B,0x0061,0x006C,0x0069,0x007A,0x0061,0x0063,0x006A,0x0061,0x0020,0x0053,0x004F,0x0053,0x0020,0x0077,0x0020,0x0070,0x006F,0x0062,0x006C,0x0069,0x017C,0x0075,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x006F,0x0070,0x0075,0x0062,0x006C,0x0069,0x006B,0x006F,0x0077,0x0061,0x006E,0x0061,0x0020,0x0077,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//SOS-plats n?ra "XXXXXXXXXXXXX" har lagts upp p? CareMate.
													{0x0053,0x004F,0x0053,0x2013,0x0070,0x006C,0x0061,0x0074,0x0073,0x0020,0x006E,0x00E4,0x0072,0x0061,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0068,0x0061,0x0072,0x0020,0x006C,0x0061,0x0067,0x0074,0x0073,0x0020,0x0075,0x0070,0x0070,0x0020,0x0070,0x00E5,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//「XXXXXXXXXXXXXXX」付近のSOS位置情報がCareMateに投稿されました。
													{0x300C,0x0000},
													{0x300D,0x4ED8,0x8FD1,0x306E,0x0053,0x004F,0x0053,0x4F4D,0x7F6E,0x60C5,0x5831,0x304C,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x306B,0x6295,0x7A3F,0x3055,0x308C,0x307E,0x3057,0x305F,0x3002,0x0000},
												},
												{
													//CareMate? ??? "XXXXXXXXXXXXX" ??? SOS ??.
													{0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0xC5D0,0x0020,0xAC8C,0xC2DC,0xB41C,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0xADFC,0xCC98,0xC758,0x0020,0x0053,0x004F,0x0053,0x0020,0xC704,0xCE58,0x002E,0x0000},
												},
												{
													//Местоположение SOS рядом с ?XXXXXXXXXXXXX? отправлено в CareMate.
													{0x041C,0x0435,0x0441,0x0442,0x043E,0x043F,0x043E,0x043B,0x043E,0x0436,0x0435,0x043D,0x0438,0x0435,0x0020,0x0053,0x004F,0x0053,0x0020,0x0440,0x044F,0x0434,0x043E,0x043C,0x0020,0x0441,0x0020,0x00AB,0x0000},
													{0x00BB,0x0020,0x043E,0x0442,0x043F,0x0440,0x0430,0x0432,0x043B,0x0435,0x043D,0x043E,0x0020,0x0432,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
												{
													//?? ??? ???? SOS ?????? ?? "XXXXXXXXXXXX" ??? CareMate.
													{0x062A,0x0645,0x0020,0x0646,0x0634,0x0631,0x0020,0x0645,0x0648,0x0642,0x0639,0x0020,0x0627,0x0644,0x0627,0x0633,0x062A,0x063A,0x0627,0x062B,0x0629,0x0020,0x0628,0x0627,0x0644,0x0642,0x0631,0x0628,0x0020,0x0645,0x0646,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0639,0x0644,0x0649,0x0020,0x0643,0x064A,0x0631,0x0645,0x064A,0x062A,0x002E,0x0000},
												},
											#else
												{
													//"XXXXXXXXXXX"附近的SOS已发送给您的亲友。
													{0x201C,0x0000},
													{0x201D,0x9644,0x8FD1,0x7684,0x0053,0x004F,0x0053,0x5DF2,0x53D1,0x9001,0x7ED9,0x60A8,0x7684,0x4EB2,0x53CB,0x3002,0x0000},
												},
												{
													//SOS location near "XXXXXXXXXXXXX" posted to CareMate.
													{0x0053,0x004F,0x0053,0x0020,0x006C,0x006F,0x0063,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x006E,0x0065,0x0061,0x0072,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x006F,0x0073,0x0074,0x0065,0x0064,0x0020,0x0074,0x006F,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											#endif
											};
	uint8_t strtmp[512] = {0};
	notify_infor infor = {0};

#ifdef CONFIG_WIFI_SUPPORT
	k_timer_stop(&sos_wait_wifi_addr_timer);
#endif

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
		return true;
	}
	else
	{
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

	if(sos_wait_wifi_addr_flag)
	{
		switch(global_settings.location_type)
		{
		case 1://only wifi
		case 2://only gps
		case 4://gps+wifi
			break;			
		case 3://wifi+gps
			k_timer_start(&sos_gps_timer, K_SECONDS(5), K_NO_WAIT);
			break;
		}
		sos_wait_wifi_addr_flag = false;
	}
#endif

	if(sos_status_change_flag)
	{
		SOSStatusUpdate();
		sos_status_change_flag = false;
	}
}

