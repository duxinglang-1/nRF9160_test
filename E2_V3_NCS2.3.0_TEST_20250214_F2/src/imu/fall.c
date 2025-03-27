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

		//fall trigger time
		strcat(reply, fall_trigger_time);

		//semicolon
		strcat(reply, ";");
		
		NBSendFallGpsData(reply, strlen(reply));
	}
}

void FallRecLocatNotify(uint8_t *strmsg)
{
	uint16_t strtitle[LANGUAGE_MAX][2][40] = {
											#ifndef FW_FOR_CN
											  #ifdef LANGUAGE_EN_ENABLE
												{
													//Fall location near "XXXXXXXXXXXXX" posted to CareMate.
													{0x0046,0x0061,0x006C,0x006C,0x0020,0x006C,0x006F,0x0063,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x006E,0x0065,0x0061,0x0072,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x006F,0x0073,0x0074,0x0065,0x0064,0x0020,0x0074,0x006F,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_DE_ENABLE
												{
													//Herbst standort in der N?he von "XXXXXXXXXXXXX" gepostet auf CareMate.
													{0x0048,0x0065,0x0072,0x0062,0x0073,0x0074,0x0020,0x0073,0x0074,0x0061,0x006E,0x0064,0x006F,0x0072,0x0074,0x0020,0x0069,0x006E,0x0020,0x0064,0x0065,0x0072,0x0020,0x004E,0x00E4,0x0068,0x0065,0x0020,0x0076,0x006F,0x006E,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0067,0x0065,0x0070,0x006F,0x0073,0x0074,0x0065,0x0074,0x0020,0x0061,0x0075,0x0066,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_FR_ENABLE
												{
													//Emplacement d'automne pr��s de "xxxxxxxxxxxxxx" publi�� sur CareMate.
													{0x0045,0x006D,0x0070,0x006C,0x0061,0x0063,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0064,0x0027,0x0061,0x0075,0x0074,0x006F,0x006D,0x006E,0x0065,0x0020,0x0070,0x0072,0x00E8,0x0073,0x0020,0x0064,0x0065,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x0075,0x0062,0x006C,0x0069,0x00E9,0x0020,0x0073,0x0075,0x0072,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_IT_ENABLE
												{
													//Posizione di autunno vicino a "xxxxxxxxxxxx" pubblicato a Caremate.
													{0x0050,0x006F,0x0073,0x0069,0x007A,0x0069,0x006F,0x006E,0x0065,0x0020,0x0064,0x0069,0x0020,0x0061,0x0075,0x0074,0x0075,0x006E,0x006E,0x006F,0x0020,0x0076,0x0069,0x0063,0x0069,0x006E,0x006F,0x0020,0x0061,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x0075,0x0062,0x0062,0x006C,0x0069,0x0063,0x0061,0x0074,0x006F,0x0020,0x0061,0x0020,0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_ES_ENABLE
												{
													//Ubicaci��n de oto?o cerca de "XXXXXXXXXXXXX" Publicado en Caremate.
													{0x0055,0x0062,0x0069,0x0063,0x0061,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0064,0x0065,0x0020,0x006F,0x0074,0x006F,0x00F1,0x006F,0x0020,0x0063,0x0065,0x0072,0x0063,0x0061,0x0020,0x0064,0x0065,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0050,0x0075,0x0062,0x006C,0x0069,0x0063,0x0061,0x0064,0x006F,0x0020,0x0065,0x006E,0x0020,0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_PT_ENABLE
												{
													//Localiza??o de outono perto de "xxxxxxxxxxxxxx" postada no Caremate.
													{0x004C,0x006F,0x0063,0x0061,0x006C,0x0069,0x007A,0x0061,0x00E7,0x00E3,0x006F,0x0020,0x0064,0x0065,0x0020,0x006F,0x0075,0x0074,0x006F,0x006E,0x006F,0x0020,0x0070,0x0065,0x0072,0x0074,0x006F,0x0020,0x0064,0x0065,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x006F,0x0073,0x0074,0x0061,0x0064,0x0061,0x0020,0x006E,0x006F,0x0020,0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_PL_ENABLE
												{
													//Lokalizacja jesieni w pobli?u "XXXXXXXXXXXXX" opublikowana w CareMate.
													{0x004C,0x006F,0x006B,0x0061,0x006C,0x0069,0x007A,0x0061,0x0063,0x006A,0x0061,0x0020,0x006A,0x0065,0x0073,0x0069,0x0065,0x006E,0x0069,0x0020,0x0077,0x0020,0x0070,0x006F,0x0062,0x006C,0x0069,0x017C,0x0075,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x006F,0x0070,0x0075,0x0062,0x006C,0x0069,0x006B,0x006F,0x0077,0x0061,0x006E,0x0061,0x0020,0x0077,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_SE_ENABLE
												{
													//H?stplats n?ra "XXXXXXXXXXXXX" publicerat p? CareMate.
													{0x0048,0x00F6,0x0073,0x0074,0x0070,0x006C,0x0061,0x0074,0x0073,0x0020,0x006E,0x00E4,0x0072,0x0061,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x0075,0x0062,0x006C,0x0069,0x0063,0x0065,0x0072,0x0061,0x0074,0x0020,0x0070,0x00E5,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_JP_ENABLE
												{
													//��XXXXXXXXXXXXXXX����������������CareMate��Ͷ�夵��ޤ�����
													{0x300C,0x0000},
													{0x300D,0x4ED8,0x8FD1,0x306E,0x843D,0x4E0B,0x5834,0x6240,0x304C,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x306B,0x6295,0x7A3F,0x3055,0x308C,0x307E,0x3057,0x305F,0x3002,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_KR_ENABLE
												{
													//CareMate? ??? "XXXXXXXXXXXXX" ??? ?? ??.
													{0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0xC5D0,0x0020,0xAC8C,0xC2DC,0xB41C,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0xADFC,0xCC98,0xC758,0x0020,0xAC00,0xC744,0x0020,0xC704,0xCE58,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_RU_ENABLE
												{
													//���֧��� ��ѧէ֧ߧڧ� ��ܧ�ݧ� "XXXXXXXXXXXXXXX" ����ҧݧڧܧ�ӧѧߧ� �� CareMate.
													{0x041C,0x0435,0x0441,0x0442,0x043E,0x0020,0x043F,0x0430,0x0434,0x0435,0x043D,0x0438,0x044F,0x0020,0x043E,0x043A,0x043E,0x043B,0x043E,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x043E,0x043F,0x0443,0x0431,0x043B,0x0438,0x043A,0x043E,0x0432,0x0430,0x043D,0x043E,0x0020,0x0432,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_AR_ENABLE
												{
													//?? ??? ???? ?????? ?????? ?? "XXXXXXXXXXXX" ??? CareMate
													{0x062A,0x0645,0x0020,0x0646,0x0634,0x0631,0x0020,0x0645,0x0648,0x0642,0x0639,0x0020,0x0627,0x0644,0x0633,0x0642,0x0648,0x0637,0x0020,0x0628,0x0627,0x0644,0x0642,0x0631,0x0628,0x0020,0x0645,0x0646,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0639,0x0644,0x0649,0x0020,0x0643,0x064A,0x0631,0x0645,0x062A,0x002E,0x0000},
												},
											  #endif
											#else
											  #ifdef LANGUAGE_CN_ENABLE
												{
													//"XXXXXXXXXXX"������ˤ�������ѷ��͸��������ѡ�
													{0x201C,0x0000},
													{0x201D,0x9644,0x8FD1,0x7684,0x6454,0x5012,0x62A5,0x8B66,0x5DF2,0x53D1,0x9001,0x7ED9,0x60A8,0x7684,0x4EB2,0x53CB,0x3002,0x0000},
												},
											  #endif
											  #ifdef LANGUAGE_EN_ENABLE
												{
													//Fall location near "XXXXXXXXXXXXX" posted to CareMate.
													{0x0046,0x0061,0x006C,0x006C,0x0020,0x006C,0x006F,0x0063,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x006E,0x0065,0x0061,0x0072,0x0020,0x0022,0x0000},
													{0x0022,0x0020,0x0070,0x006F,0x0073,0x0074,0x0065,0x0064,0x0020,0x0074,0x006F,0x0020,0x0043,0x0061,0x0072,0x0065,0x004D,0x0061,0x0074,0x0065,0x002E,0x0000},
												},
											  #endif
											#endif
											};
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

		mmi_ucs2cpy(infor.text, strtitle[global_settings.language][0]);
		mmi_ucs2cat(infor.text, strtmp);
		mmi_ucs2cat(infor.text, strtitle[global_settings.language][1]);

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
	if(global_settings.language == LANGUAGE_CHN)
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
