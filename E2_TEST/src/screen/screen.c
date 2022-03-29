/****************************************Copyright (c)************************************************
** File name:			    screen.c
** Last modified Date:          
** Last Version:		   
** Descriptions:		   	使用的ncs版本-1.2		
** Created by:				谢彪
** Created date:			2020-12-16
** Version:			    	1.0
** Descriptions:			屏幕UI管理C文件
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include "settings.h"
#include "lcd.h"
#include "font.h"
#include "img.h"
#include "key.h"
#include "datetime.h"
#include "max20353.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif
#ifdef CONFIG_IMU_SUPPORT
#include "lsm6dso.h"
#include "fall.h"
#endif
#include "external_flash.h"
#include "screen.h"
#include "ucs2.h"
#include "nb.h"
#include "sos.h"
#include "alarm.h"
#include "gps.h"
#include "uart_ble.h"
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#ifdef CONFIG_FOTA_DOWNLOAD
#include "fota_mqtt.h"
#endif/*CONFIG_FOTA_DOWNLOAD*/
#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
#include "data_download.h"
#endif/*CONFIG_DATA_DOWNLOAD_SUPPORT*/
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif/*CONFIG_WIFI*/
#ifdef CONFIG_SYNC_SUPPORT
#include "sync.h"
#endif/*CONFIG_SYNC_SUPPORT*/
#ifdef CONFIG_TEMP_SUPPORT
#include "temp.h"
#endif/*CONFIG_TEMP_SUPPORT*/
#include "logger.h"

static u8_t scr_index = 0;
static u8_t bat_charging_index = 0;

static void NotifyTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(notify_timer, NotifyTimerOutCallBack, NULL);
static void MainMenuTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(mainmenu_timer, MainMenuTimerOutCallBack, NULL);

SCREEN_ID_ENUM screen_id = SCREEN_ID_BOOTUP;
SCREEN_ID_ENUM history_screen_id = SCREEN_ID_BOOTUP;
screen_msg scr_msg[SCREEN_ID_MAX] = {0};
notify_infor notify_msg = {0};

extern bool key_pwroff_flag;
extern u8_t g_rsrp;

static void EnterHRScreen(void);

#ifdef IMG_FONT_FROM_FLASH
static u32_t logo_img[] = 
{
	IMG_PWRON_ANI_1_ADDR,
	IMG_PWRON_ANI_2_ADDR,
	IMG_PWRON_ANI_3_ADDR,
	IMG_PWRON_ANI_4_ADDR,
	IMG_PWRON_ANI_5_ADDR,
	IMG_PWRON_ANI_6_ADDR
};
#else
static char *logo_img[] = 
{
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR
};
#endif

void ShowBootUpLogo(void)
{
	u8_t i,count=0;
	u16_t x,y,w,h;

#ifdef IMG_FONT_FROM_FLASH
	//LCD_ShowImg_From_Flash(PWRON_STR_X, PWRON_STR_Y, IMG_ALARM_ANI_1_ADDR);
#endif

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(PWRON_LOGO_X, PWRON_LOGO_Y, logo_img, ARRAY_SIZE(logo_img), 200, false, EnterIdleScreen);
#else
  #ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(PWRON_LOGO_X, PWRON_LOGO_Y, IMG_PWRON_ANI_6_ADDR);
  #else
	LCD_ShowImg(PWRON_LOGO_X, PWRON_LOGO_Y, IMG_PWRON_ANI_6_ADDR);
  #endif
	k_sleep(K_MSEC(1000));
	EnterIdleScreen();
#endif
}

void ExitNotifyScreen(void)
{
#if 0
	if(screen_id == SCREEN_ID_NOTIFY)
	{
		k_timer_stop(&notify_timer);
		GoBackHistoryScreen();
	}
#else
	sos_state = SOS_STATUS_IDLE;
	k_timer_stop(&notify_timer);
	EnterIdleScreen();
#endif
}

void NotifyTimerOutCallBack(struct k_timer *timer_id)
{
	ExitNotifyScreen();
}

extern bool ppg_start_flag;
extern bool gps_test_start_flag;
void MainMenuTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_HR)
	{
	#ifdef CONFIG_PPG_SUPPORT
		MenuStartHrSpo2();
	#endif
	}
	else if(screen_id == SCREEN_ID_SPO2)
	{
	#ifdef CONFIG_PPG_SUPPORT
		MenuStartHrSpo2();
	#endif
	}
	else if(screen_id == SCREEN_ID_BP)
	{
	#ifdef CONFIG_PPG_SUPPORT
		MenuStartBpt();
	#endif
	}
	else if(screen_id == SCREEN_ID_GPS_TEST)
	{
		MenuStartGPS();
	}
	else if(screen_id == SCREEN_ID_NB_TEST)
	{
		MenuStartNB();
	}
	else if(screen_id == SCREEN_ID_BLE_TEST)
	{
		
	}
#ifdef CONFIG_WIFI	
	else if(screen_id == SCREEN_ID_WIFI_TEST)
	{
		MenuStartWifi();
	}
#endif	
#ifdef CONFIG_FOTA_DOWNLOAD	
	else if(screen_id == SCREEN_ID_FOTA)
	{
		MenuStartFOTA();
	}
#endif
#ifdef CONFIG_SYNC_SUPPORT
	else if(screen_id == SCREEN_ID_SYNC)
	{
		MenuStartSync();
	}
#endif
#ifdef CONFIG_TEMP_SUPPORT
	else if(screen_id == SCREEN_ID_TEMP)
	{
		MenuStartTemp();
	}
#endif
}

void EnterNotifyScreen(void)
{
	if(screen_id == SCREEN_ID_NOTIFY)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_NOTIFY;	
	scr_msg[SCREEN_ID_NOTIFY].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_NOTIFY].status = SCREEN_STATUS_CREATING;	
}

void DisplayPopUp(u8_t *message)
{
	u32_t len;
	
	notify_msg.type = NOTIFY_TYPE_POPUP;
	notify_msg.align = NOTIFY_ALIGN_CENTER;
	
	len = strlen(message);
	if(len > NOTIFY_TEXT_MAX_LEN)
		len = NOTIFY_TEXT_MAX_LEN;
	memset(notify_msg.text, 0x00, sizeof(notify_msg.text));
	memcpy(notify_msg.text, message, len);

	if(notify_msg.type == NOTIFY_TYPE_POPUP)
	{
		k_timer_start(&notify_timer, K_SECONDS(NOTIFY_TIMER_INTERVAL), NULL);
	}
	
	EnterNotifyScreen();
}
void IdleShowSystemDate(void)
{
	u16_t x,y,w,h;
	u8_t str_date[20] = {0};
	u8_t *str_mon[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sept","Oct","Nov","Dec"};
	
	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else
	LCD_SetFontSize(FONT_SIZE_32);
#endif

	sprintf((char*)str_date, "%02d", date_time.day);
	LCD_MeasureString(str_date,&w,&h);
	x = IDLE_DATE_DAY_X;
	y = IDLE_DATE_DAY_Y;
	LCD_ShowString(x,y,str_date);

	strcpy((char*)str_date, str_mon[date_time.month-1]);
	LCD_MeasureString(str_date,&w,&h);
	x = IDLE_DATE_MON_X;
	y = IDLE_DATE_MON_Y;
	LCD_ShowString(x,y,str_date);
}

void IdleShowBleStatus(bool flag)
{
	if(flag)
	{
		//LCD_ShowImg(IDLE_BLE_X, IDLE_BLE_Y, IMG_BLE_LINK);
		LCD_ShowString(IDLE_BLE_X,IDLE_BLE_Y,"BLE");
	}
	else
	{
		//LCD_ShowImg(IDLE_BLE_X, IDLE_BLE_Y, IMG_BLE_UNLINK);
		LCD_ShowString(IDLE_BLE_X,IDLE_BLE_Y,"   ");
	}
}

void IdleShowSystemTime(void)
{
	static bool flag = true;
	u16_t x,y,w,h;
	u8_t str_time_h[4] = {0};
	u8_t str_time_m[4] = {0};
	u8_t str_time_dot[4] = {0};
	u8_t str_ampm[5] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_68);
#else
	LCD_SetFontSize(FONT_SIZE_64);
#endif

	switch(global_settings.time_format)
	{
	case TIME_FORMAT_24:
		sprintf((char*)str_time_h, "%02d", date_time.hour);
		break;

	case TIME_FORMAT_12:
		sprintf((char*)str_time_h, "%02d", (date_time.hour>12 ? (date_time.hour-12):date_time.hour));
		break;
	}
	sprintf((char*)str_time_m, "%02d", date_time.minute);

	if(flag)
		strcpy((char*)str_time_dot, ":");
	else
		strcpy((char*)str_time_dot, " ");
	
	flag = !flag;
	
	x = IDLE_TIME_X;
	y = IDLE_TIME_Y;
	LCD_ShowString(x,y,str_time_h);
	LCD_ShowString(x+68,y-4,str_time_dot);
	LCD_ShowString(x+86,y,str_time_m);
}

void IdleShowSystemWeek(void)
{
	u16_t x,y,w,h;
	u8_t str_week[128] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else
	LCD_SetFontSize(FONT_SIZE_32);
#endif

	GetSystemWeekStrings(str_week);
	LCD_MeasureString(str_week,&w,&h);
	x = IDLE_WEEK_X;
	y = IDLE_WEEK_Y;
	LCD_ShowString(x,y,str_week);
}

void IdleShowDateTime(void)
{
	IdleShowSystemTime();
	IdleShowSystemDate();
	IdleShowSystemWeek();
}

void IdleUpdateBatSoc(void)
{
	static bool flag = true;
	u16_t w,h;
	u8_t strbuf[128] = {0};

	if(g_bat_soc >= 100)
		sprintf(strbuf, "%d%%", g_bat_soc);
	else if(g_bat_soc >= 10)
		sprintf(strbuf, "   %d%%", g_bat_soc);
	else
		sprintf(strbuf, "      %d%%", g_bat_soc);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
#endif

	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((IDLE_BAT_X-w)-2, IDLE_BAT_PERCENT_Y, strbuf);

	if(charger_is_connected && (g_chg_status == BAT_CHARGING_PROGRESS))
	{
		if(flag)
		{
			flag = false;
			LCD_ShowImg_From_Flash(IDLE_BAT_X, IDLE_BAT_Y, IMG_BAT_RECT_WHITE_ADDR);
		}
		
		bat_charging_index++;
		if(bat_charging_index > 10)
		 bat_charging_index = 0;

		if(bat_charging_index == 0)
			LCD_Fill(IDLE_BAT_INNER_RECT_X, IDLE_BAT_INNER_RECT_Y, IDLE_BAT_INNER_RECT_W, IDLE_BAT_INNER_RECT_H, BLACK);
		else
			LCD_Fill(IDLE_BAT_INNER_RECT_X, IDLE_BAT_INNER_RECT_Y, (bat_charging_index*IDLE_BAT_INNER_RECT_W)/10, IDLE_BAT_INNER_RECT_H, GREEN);
	}
	else
	{
		flag = true;
		bat_charging_index = g_bat_soc/10;
		
		if(g_bat_soc >= 10)
		{
			LCD_ShowImg_From_Flash(IDLE_BAT_X, IDLE_BAT_Y, IMG_BAT_RECT_WHITE_ADDR);
			LCD_Fill(IDLE_BAT_INNER_RECT_X, IDLE_BAT_INNER_RECT_Y, (g_bat_soc*IDLE_BAT_INNER_RECT_W)/100, IDLE_BAT_INNER_RECT_H, GREEN);
		}
		else
		{
			LCD_ShowImg_From_Flash(IDLE_BAT_X, IDLE_BAT_Y, IMG_BAT_RECT_RED_ADDR);
			LCD_Fill(IDLE_BAT_INNER_RECT_X, IDLE_BAT_INNER_RECT_Y, (g_bat_soc*IDLE_BAT_INNER_RECT_W)/100, IDLE_BAT_INNER_RECT_H, RED);
		}
	}
}

void IdleShowBatSoc(void)
{
	u16_t w,h;
	u8_t strbuf[128] = {0};

	if(g_bat_soc >= 100)
		sprintf(strbuf, "%d%%", g_bat_soc);
	else if(g_bat_soc >= 10)
		sprintf(strbuf, "   %d%%", g_bat_soc);
	else
		sprintf(strbuf, "      %d%%", g_bat_soc);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif

	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((IDLE_BAT_X-w)-2, IDLE_BAT_PERCENT_Y, strbuf);

	bat_charging_index = g_bat_soc/10;
	
	if(charger_is_connected && (g_chg_status == BAT_CHARGING_PROGRESS))
	{
		LCD_ShowImg_From_Flash(IDLE_BAT_X, IDLE_BAT_Y, IMG_BAT_RECT_WHITE_ADDR);
		LCD_Fill(IDLE_BAT_INNER_RECT_X, IDLE_BAT_INNER_RECT_Y, (g_bat_soc*IDLE_BAT_INNER_RECT_W)/100, IDLE_BAT_INNER_RECT_H, GREEN);
	}
	else
	{
		if(g_bat_soc >= 20)
		{
			LCD_ShowImg_From_Flash(IDLE_BAT_X, IDLE_BAT_Y, IMG_BAT_RECT_WHITE_ADDR);
			LCD_Fill(IDLE_BAT_INNER_RECT_X, IDLE_BAT_INNER_RECT_Y, (g_bat_soc*IDLE_BAT_INNER_RECT_W)/100, IDLE_BAT_INNER_RECT_H, GREEN);
		}
		else
		{
			LCD_ShowImg_From_Flash(IDLE_BAT_X, IDLE_BAT_Y, IMG_BAT_RECT_RED_ADDR);
			LCD_Fill(IDLE_BAT_INNER_RECT_X, IDLE_BAT_INNER_RECT_Y, (g_bat_soc*IDLE_BAT_INNER_RECT_W)/100, IDLE_BAT_INNER_RECT_H, RED);
		}
	}
}

void IdleShowSignal(void)
{
	u32_t img_add[5] = {IMG_SIG_0_ADDR, IMG_SIG_1_ADDR, IMG_SIG_2_ADDR, IMG_SIG_3_ADDR, IMG_SIG_4_ADDR};

	LCD_ShowImg_From_Flash(IDLE_SIGNAL_X, IDLE_SIGNAL_Y, img_add[g_nb_sig]);
}

void IdleShowNetMode(void)
{
	if(nb_is_connected())
	{
		if(g_net_mode == NET_MODE_NB)
		{
			LCD_ShowImg_From_Flash(IDLE_NET_MODE_X, IDLE_NET_MODE_Y, IMG_IDLE_NET_NB_ADDR);	
		}
		else
		{
			LCD_ShowImg_From_Flash(IDLE_NET_MODE_X, IDLE_NET_MODE_Y, IMG_IDLE_NET_LTEM_ADDR);	
		}
	}
}

#ifdef CONFIG_IMU_SUPPORT
void IdleUpdateSportData(void)
{
}

void IdleShowSportData(void)
{
}
#endif

#ifdef CONFIG_PPG_SUPPORT
void IdleUpdateHrData(void)
{
	u16_t w,h;
	u8_t strbuf[8] = {0};
	u16_t bg_color = 0x1820;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
#endif

	LCD_Fill(IDLE_HR_NUM_X, IDLE_HR_NUM_Y, IDLE_HR_NUM_W, IDLE_HR_NUM_H, bg_color);
	sprintf(strbuf, "%d", g_hr);
	LCD_MeasureString(strbuf, &w, &h);
	BACK_COLOR=bg_color;
	LCD_ShowString(IDLE_HR_NUM_X+(IDLE_HR_NUM_W-w)/2, IDLE_HR_NUM_Y+(IDLE_HR_NUM_H-h)/2, strbuf);
	BACK_COLOR=BLACK;
}

void IdleShowHrData(void)
{
	u16_t w,h;
	u8_t strbuf[8] = {0};
	u16_t bg_color = 0x1820;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif

	LCD_ShowImg_From_Flash(IDLE_HR_BG_X, IDLE_HR_BG_Y, IMG_IDLE_HR_BG_ADDR);
	LCD_dis_pic_trans_from_flash(IDLE_HR_ICON_X, IDLE_HR_ICON_Y, IMG_IDLE_HR_ICON_ADDR, bg_color);
			
	LCD_Fill(IDLE_HR_NUM_X, IDLE_HR_NUM_Y, IDLE_HR_NUM_W, IDLE_HR_NUM_H, bg_color);
	sprintf(strbuf, "%d", g_hr);
	LCD_MeasureString(strbuf, &w, &h);
	BACK_COLOR=bg_color;
	LCD_ShowString(IDLE_HR_NUM_X+(IDLE_HR_NUM_W-w)/2, IDLE_HR_NUM_Y+(IDLE_HR_NUM_H-h)/2, strbuf);
	BACK_COLOR=BLACK;
}
#endif

#ifdef CONFIG_TEMP_SUPPORT
void IdleUpdateTempData(void)
{
	u16_t w,h;
	u8_t strbuf[8] = {0};
	u16_t bg_color = 0x00c3;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif

	LCD_Fill(IDLE_TEMP_NUM_X, IDLE_TEMP_NUM_Y, IDLE_TEMP_NUM_W, IDLE_TEMP_NUM_H, bg_color);
	sprintf(strbuf, "%d", g_hr);
	LCD_MeasureString(strbuf, &w, &h);
	BACK_COLOR=bg_color;
	LCD_ShowString(IDLE_TEMP_NUM_X+(IDLE_TEMP_NUM_W-w)/2, IDLE_TEMP_NUM_Y+(IDLE_TEMP_NUM_H-h)/2, strbuf);
	BACK_COLOR=BLACK;
}

void IdleShowTempData(void)
{
	u16_t w,h;
	u8_t strbuf[8] = {0};
	u16_t bg_color = 0x00c3;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif

	LCD_ShowImg_From_Flash(IDLE_TEMP_BG_X, IDLE_TEMP_BG_Y, IMG_IDLE_TEMP_BG_ADDR);
	if(global_settings.temp_unit == TEMP_UINT_C)
	{
		LCD_dis_pic_trans_from_flash(IDLE_TEMP_ICON_X, IDLE_TEMP_ICON_Y, IMG_IDLE_TEMP_C_ICON_ADDR, bg_color);
		sprintf(strbuf, "%0.1f", g_temp_body);
	}
	else
	{
		LCD_dis_pic_trans_from_flash(IDLE_TEMP_ICON_X, IDLE_TEMP_ICON_Y, IMG_IDLE_TEMP_F_ICON_ADDR, bg_color);
		sprintf(strbuf, "%0.1f", 32+1.8*g_temp_body);
	}

	LCD_Fill(IDLE_TEMP_NUM_X, IDLE_TEMP_NUM_Y, IDLE_TEMP_NUM_W, IDLE_TEMP_NUM_H, bg_color);
	LCD_MeasureString(strbuf, &w, &h);
	BACK_COLOR=bg_color;
	LCD_ShowString(IDLE_TEMP_NUM_X+(IDLE_TEMP_NUM_W-w)/2, IDLE_TEMP_NUM_Y+(IDLE_TEMP_NUM_H-h)/2, strbuf);
	BACK_COLOR=BLACK;
}

#endif

void IdleScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_IDLE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATED;
		
		LCD_Clear(BLACK);
		IdleShowSignal();
		IdleShowNetMode();
		IdleShowBatSoc();
		IdleShowDateTime();
	#ifdef CONFIG_PPG_SUPPORT
		IdleShowHrData();
	#endif
	#ifdef CONFIG_TEMP_SUPPORT
		IdleShowTempData();
	#endif
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_NET_MODE)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_NET_MODE);	
			IdleShowNetMode();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_BAT)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_BAT);
			IdleUpdateBatSoc();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_TIME)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_TIME);
			IdleShowSystemTime();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_DATE)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_DATE);
			IdleShowSystemDate();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_WEEK)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_WEEK);
			IdleShowSystemWeek();
		}
	#ifdef CONFIG_IMU_SUPPORT	
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SPORT)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SPORT);
			IdleUpdateSportData();
		}
	#endif
	#ifdef CONFIG_PPG_SUPPORT
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_HR)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_HR);
			IdleUpdateHrData();
		}
	#endif
	#ifdef CONFIG_TEMP_SUPPORT
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_TEMP)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_TEMP);
			IdleUpdateTempData();
		}
	#endif
		if(scr_msg[SCREEN_ID_IDLE].para == SCREEN_EVENT_UPDATE_NO)
			scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_NO;
	}
}

bool IsInIdleScreen(void)
{
	if(screen_id == SCREEN_ID_IDLE)
		return true;
	else
		return false;
}

void AlarmScreenProcess(void)
{
	u16_t rect_x,rect_y,rect_w=180,rect_h=80;
	u16_t x,y,w,h;
	u8_t notify[128] = "Alarm Notify!";

	switch(scr_msg[SCREEN_ID_ALARM].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_ALARM].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_ALARM].status = SCREEN_STATUS_CREATED;
				
		rect_x = (LCD_WIDTH-rect_w)/2;
		rect_y = (LCD_HEIGHT-rect_h)/2;
		
		LCD_DrawRectangle(rect_x, rect_y, rect_w, rect_h);
		LCD_Fill(rect_x+1, rect_y+1, rect_w-2, rect_h-2, BLACK);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif
		LCD_MeasureString(notify,&w,&h);
		x = (w > rect_w)? 0 : (rect_w-w)/2;
		y = (h > rect_h)? 0 : (rect_h-h)/2;
		x += rect_x;
		y += rect_y;
		LCD_ShowString(x,y,notify);
		break;
		
	case SCREEN_ACTION_UPDATE:
		break;
	}
	
	scr_msg[SCREEN_ID_ALARM].act = SCREEN_ACTION_NO;
}

void poweroff_confirm(void)
{
	ClearAllKeyHandler();

	if(screen_id == SCREEN_ID_POWEROFF)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_FOTA;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}

	key_pwroff_flag = true;
}

void poweroff_cancel(void)
{
	EnterIdleScreen();
}

void EnterPoweroffScreen(void)
{
	if(screen_id == SCREEN_ID_POWEROFF)
		return;

	if(gps_is_working())
		MenuStopGPS();

#ifdef CONFIG_PPG_SUPPORT
	if(PPGIsWorking())
		MenuStopPPG();
#endif

#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking())
		MenuStopTemp();
#endif

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif

	k_timer_stop(&mainmenu_timer);

	LCD_Set_BL_Mode(LCD_BL_AUTO);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_POWEROFF;	
	scr_msg[SCREEN_ID_POWEROFF].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_POWEROFF].status = SCREEN_STATUS_CREATING;		
}

void PowerOffUpdateStatus(void)
{
	u32_t *img_anima[3] = {IMG_RUNNING_ANI_1_ADDR, IMG_RUNNING_ANI_2_ADDR, IMG_RUNNING_ANI_3_ADDR};

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(POW_OFF_RUNNING_ANI_X, POW_OFF_RUNNING_ANI_Y, img_anima, ARRAY_SIZE(img_anima), 1000, true, NULL);
#endif	
}

void PowerOffShowStatus(void)
{
	u16_t x,y,w,h;

	LCD_Clear(BLACK);

	LCD_ShowImg_From_Flash(PWR_OFF_ICON_X, PWR_OFF_ICON_Y, IMG_PWROFF_BUTTON_ADDR);

	SetLeftKeyUpHandler(poweroff_cancel);
	SetRightKeyUpHandler(poweroff_confirm);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, PWR_OFF_ICON_X, PWR_OFF_ICON_X+PWR_OFF_ICON_W, PWR_OFF_ICON_Y, PWR_OFF_ICON_Y+PWR_OFF_ICON_H, poweroff_confirm);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, poweroff_cancel);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, poweroff_cancel);
#endif
}

void PowerOffScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_POWEROFF].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_POWEROFF].status = SCREEN_STATUS_CREATED;

		PowerOffShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		PowerOffUpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_POWEROFF].act = SCREEN_ACTION_NO;
}

void FindDeviceScreenProcess(void)
{
	u16_t rect_x,rect_y,rect_w=180,rect_h=80;
	u16_t x,y,w,h;
	u8_t notify[128] = "Find Device!";

	switch(scr_msg[SCREEN_ID_FIND_DEVICE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_FIND_DEVICE].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_FIND_DEVICE].status = SCREEN_STATUS_CREATED;
				
		rect_x = (LCD_WIDTH-rect_w)/2;
		rect_y = (LCD_HEIGHT-rect_h)/2;
		
		LCD_DrawRectangle(rect_x, rect_y, rect_w, rect_h);
		LCD_Fill(rect_x+1, rect_y+1, rect_w-2, rect_h-2, BLACK);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif
		LCD_MeasureString(notify,&w,&h);
		x = (w > rect_w)? 0 : (rect_w-w)/2;
		y = (h > rect_h)? 0 : (rect_h-h)/2;
		x += rect_x;
		y += rect_y;
		LCD_ShowString(x,y,notify);
		break;
		
	case SCREEN_ACTION_UPDATE:
		break;
	}
	
	scr_msg[SCREEN_ID_FIND_DEVICE].act = SCREEN_ACTION_NO;
}

#ifdef CONFIG_SYNC_SUPPORT
void ExitSyncDataScreen(void)
{
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	SyncDataStop();
	EnterIdleScreen();
}

void EnterSyncDataScreen(void)
{
	if(screen_id == SCREEN_ID_SYNC)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SYNC;
	scr_msg[SCREEN_ID_SYNC].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SYNC].status = SCREEN_STATUS_CREATING;

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking())
		MenuStopTemp();
#endif
	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

#ifdef CONFIG_FOTA_DOWNLOAD
	SetLeftKeyUpHandler(fota_start);
#elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
	SetLeftKeyUpHandler(dl_img_start);
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_font_start);
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_ppg_start);
  #endif
#else
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#endif
	SetRightKeyUpHandler(ExitSyncDataScreen);

#ifdef CONFIG_TOUCH_SUPPORT
 #ifdef CONFIG_FOTA_DOWNLOAD
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_start);
 #elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
  #endif
 #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 #endif
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitSyncDataScreen);
#endif
}

void SyncUpdateStatus(void)
{
	unsigned char *img_anima[3] = {IMG_RUNNING_ANI_1_ADDR, IMG_RUNNING_ANI_2_ADDR, IMG_RUNNING_ANI_3_ADDR};
	
	switch(sync_state)
	{
	case SYNC_STATUS_IDLE:
		break;

	case SYNC_STATUS_LINKING:
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaShow(SYNC_RUNNING_ANI_X, SYNC_RUNNING_ANI_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
	#endif
		break;
		
	case SYNC_STATUS_SENT:
	#ifdef CONFIG_ANIMATION_SUPPORT 
		AnimaStopShow();
	#endif
	
		LCD_ShowImg_From_Flash(SYNC_ICON_X, SYNC_ICON_Y, IMG_SYNC_FINISH_ADDR);
		LCD_ShowImg_From_Flash(SYNC_RUNNING_ANI_X, SYNC_RUNNING_ANI_Y, IMG_RUNNING_ANI_1_ADDR);

		SetLeftKeyUpHandler(ExitSyncDataScreen);
		break;
		
	case SYNC_STATUS_FAIL:
	#ifdef CONFIG_ANIMATION_SUPPORT 
		AnimaStopShow();
	#endif
	
		LCD_ShowImg_From_Flash(SYNC_ICON_X, SYNC_ICON_Y, IMG_SYNC_ERR_ADDR);
		LCD_ShowImg_From_Flash(SYNC_RUNNING_ANI_X, SYNC_RUNNING_ANI_Y, IMG_RUNNING_ANI_1_ADDR);

		SetLeftKeyUpHandler(ExitSyncDataScreen);
		break;
	}
}

void SyncShowStatus(void)
{
	LCD_Clear(BLACK);

	LCD_ShowImg_From_Flash(SYNC_ICON_X, SYNC_ICON_Y, IMG_SYNC_LOGO_ADDR);
	LCD_ShowImg_From_Flash(SYNC_RUNNING_ANI_X, SYNC_RUNNING_ANI_Y, IMG_RUNNING_ANI_1_ADDR);
}

void SyncScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_SYNC].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SYNC].status = SCREEN_STATUS_CREATED;

		SyncShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_SYNC].para&SCREEN_EVENT_UPDATE_SYNC)
			scr_msg[SCREEN_ID_SYNC].para &= (~SCREEN_EVENT_UPDATE_SYNC);

		SyncUpdateStatus();
		break;
	}

	scr_msg[SCREEN_ID_SYNC].act = SCREEN_ACTION_NO;
}
#endif/*CONFIG_SYNC_SUPPORT*/

#ifdef CONFIG_TEMP_SUPPORT
void TempUpdateStatus(void)
{
	static u8_t index = 0;
	u16_t x,y,w,h;
	u8_t tmpbuf[64] = {0};
	u32_t img_addr[3] = {IMG_RUNNING_ANI_1_ADDR, IMG_RUNNING_ANI_2_ADDR, IMG_RUNNING_ANI_3_ADDR};

#ifdef CONFIG_ANIMATION_SUPPORT
	if(TempIsWorking())
	{
		index++;
		if(index >= 4)
			index = 0;
		LCD_ShowImg_From_Flash(TEMP_RUNNING_ANI_X, TEMP_RUNNING_ANI_Y, img_addr[index]);
	}
	else
	{
		AnimaStop();
		index = 0;
	}
#endif

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_52);
#else
	LCD_SetFontSize(FONT_SIZE_48);
#endif

	if(g_temp_body >= 10.0)
		sprintf(tmpbuf, "%0.1f", g_temp_body);
	else
		sprintf(tmpbuf, " %0.1f", g_temp_body);
	LCD_MeasureString(tmpbuf, &w, &h);
	LCD_ShowString(TEMP_NUM_X+(TEMP_NUM_W-w)/2, TEMP_NUM_Y+(TEMP_NUM_H-h)/2, tmpbuf);
}

void TempShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t tmpbuf[64] = {0};

	LCD_Clear(BLACK);
	
	if(global_settings.temp_unit == TEMP_UINT_C)
	{
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_ICON_C_ADDR);
		LCD_ShowImg_From_Flash(TEMP_UNIT_X, TEMP_UNIT_Y, IMG_TEMP_UNIT_C_ADDR);
	}
	else
	{
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_ICON_F_ADDR);
		LCD_ShowImg_From_Flash(TEMP_UNIT_X, TEMP_UNIT_Y, IMG_TEMP_UNIT_F_ADDR);
	}
	LCD_ShowImg_From_Flash(TEMP_RUNNING_ANI_X, TEMP_RUNNING_ANI_Y, IMG_RUNNING_ANI_1_ADDR);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_52);
#else
	LCD_SetFontSize(FONT_SIZE_48);
#endif

	sprintf(tmpbuf, "%d", 0);
	LCD_MeasureString(tmpbuf, &w, &h);
	LCD_ShowString(TEMP_NUM_X+(TEMP_NUM_W-w)/2, TEMP_NUM_Y+(TEMP_NUM_H-h)/2, tmpbuf);
}

void TempScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_TEMP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_TEMP].status = SCREEN_STATUS_CREATED;
		TempShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		TempUpdateStatus();
		break;
	}

	scr_msg[SCREEN_ID_TEMP].act = SCREEN_ACTION_NO;
}

void ExitTempScreen(void)
{
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif
	MenuStopTemp();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterTempScreen(void)
{
	if(screen_id == SCREEN_ID_TEMP)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_TEMP;
	scr_msg[SCREEN_ID_TEMP].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_TEMP].status = SCREEN_STATUS_CREATING;

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
#ifdef CONFIG_SYNC_SUPPORT
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#elif defined(CONFIG_FOTA_DOWNLOAD)
	SetLeftKeyUpHandler(fota_start);
#elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
	SetLeftKeyUpHandler(dl_img_start);
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_font_start);
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_ppg_start);
  #endif
#else
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#endif
	SetRightKeyUpHandler(ExitTempScreen);

#ifdef CONFIG_TOUCH_SUPPORT
 #ifdef CONFIG_SYNC_SUPPORT
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
 #elif defined(CONFIG_FOTA_DOWNLOAD)
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_start);
 #elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
  #elif defined(CONFIG_FONT_DATA_UPDATE)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
  #elif defined(CONFIG_PPG_DATA_UPDATE)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
  #endif
 #else
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 #endif
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitTempScreen);
#endif
}
#endif/*CONFIG_TEMP_SUPPORT*/

#ifdef CONFIG_PPG_SUPPORT
static u8_t img_index = 0;

void BPUpdateStatus(void)
{
	u16_t x,y,w,h;
	u8_t tmpbuf[64] = {0};
	u32_t img_anima[3] = {IMG_BP_ICON_ANI_1_ADDR,IMG_BP_ICON_ANI_2_ADDR,IMG_BP_ICON_ANI_3_ADDR};

	img_index++;
	if(img_index >= 3)
		img_index = 0;
	LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, img_anima[img_index]);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else
	LCD_SetFontSize(FONT_SIZE_32);
#endif

	sprintf(tmpbuf, "%d/%d", g_bp_systolic, g_bp_diastolic);
	LCD_MeasureString(tmpbuf, &w, &h);
	x = BP_NUM_X+(BP_NUM_W-w)/2;
	y = BP_NUM_Y+(BP_NUM_H-h)/2;
	LCD_Fill(BP_NUM_X, BP_NUM_Y, BP_NUM_W, BP_NUM_H, BLACK);
	LCD_ShowString(x,y,tmpbuf);
}

void BPShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t i,tmpbuf[64] = {0};
	bpt_data bpt_max={0},bpt_min={0},bpt[24] = {0};

	LCD_Clear(BLACK);
	
	LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, IMG_BP_ICON_ANI_2_ADDR);
	LCD_ShowImg_From_Flash(BP_BG_X, BP_BG_Y, IMG_BP_BG_ADDR);
	LCD_ShowImg_From_Flash(BP_UP_ARRAW_X, BP_UP_ARRAW_Y, IMG_BP_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(BP_DOWN_ARRAW_X, BP_DOWN_ARRAW_Y, IMG_BP_DOWN_ARRAW_ADDR);

	GetCurDayBptRecData(bpt);
	for(i=0;i<24;i++)
	{
		if(bpt_max.systolic == 0 || bpt_max.diastolic == 0 || bpt_min.systolic == 0 || bpt_min.diastolic == 0)
		{
			if(bpt[i].systolic > 0 && bpt[i].diastolic > 0)
			{
				memcpy(&bpt_max, &bpt[i], sizeof(bpt_data));
				memcpy(&bpt_min, &bpt[i], sizeof(bpt_data));
			}
		}
		else
		{	
			if(bpt[i].systolic > bpt_max.systolic)
				memcpy(&bpt_max, &bpt[i], sizeof(bpt_data));
			if(bpt[i].systolic < bpt_min.systolic)
				memcpy(&bpt_min, &bpt[i], sizeof(bpt_data));
		}
		
		if(bpt[i].systolic > 30)
			LCD_Fill(BP_REC_DATA_X+BP_REC_DATA_OFFSET_X*i, BP_REC_DATA_Y-(bpt[i].systolic-30)*15/30, BP_REC_DATA_W, (bpt[i].systolic-30)*15/30, YELLOW);
		if(bpt[i].diastolic > 30)
			LCD_Fill(BP_REC_DATA_X+BP_REC_DATA_OFFSET_X*i, BP_REC_DATA_Y-(bpt[i].diastolic-30)*15/30, BP_REC_DATA_W, (bpt[i].diastolic-30)*15/30, RED);
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else		
	LCD_SetFontSize(FONT_SIZE_32);
#endif
	strcpy(tmpbuf, "0");
	LCD_MeasureString(tmpbuf,&w,&h);
	x = BP_NUM_X+(BP_NUM_W-w)/2;
	y = BP_NUM_Y+(BP_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	sprintf(tmpbuf, "%d/%d", bpt_max.systolic, bpt_max.diastolic);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = BP_UP_NUM_X+(BP_UP_NUM_W-w)/2;
	y = BP_UP_NUM_Y+(BP_UP_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

	sprintf(tmpbuf, "%d/%d", bpt_min.systolic, bpt_min.diastolic);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = BP_DOWN_NUM_X+(BP_DOWN_NUM_W-w)/2;
	y = BP_DOWN_NUM_Y+(BP_DOWN_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);
}

void BPScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_BP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_BP].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_BP].status = SCREEN_STATUS_CREATED;
				
		BPShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		BPUpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_BP].act = SCREEN_ACTION_NO;
}

void ExitBPScreen(void)
{
	k_timer_stop(&mainmenu_timer);

	img_index = 0;
	
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif

	if(PPGIsWorking())
		MenuStopPPG();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterBPScreen(void)
{
	if(screen_id == SCREEN_ID_BP)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_BP;	
	scr_msg[SCREEN_ID_BP].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_BP].status = SCREEN_STATUS_CREATING;

	img_index = 0;
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif
	
	if(PPGIsWorking())
		MenuStopPPG();

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
#ifdef CONFIG_TEMP_SUPPORT
	SetLeftKeyUpHandler(EnterTempScreen);
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#elif defined(CONFIG_FOTA_DOWNLOAD)
	SetLeftKeyUpHandler(fota_start);
#elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
	SetLeftKeyUpHandler(dl_img_start);
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_font_start);
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_ppg_start);
  #endif
#else
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#endif
	SetRightKeyUpHandler(ExitBPScreen);

#ifdef CONFIG_TOUCH_SUPPORT
 #ifdef CONFIG_TEMP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);  
 #elif defined(CONFIG_SYNC_SUPPORT)
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
 #elif defined(CONFIG_FOTA_DOWNLOAD)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_start); 
 #elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);  
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);  
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);  
  #endif
 #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen); 
 #endif
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitBPScreen);
#endif
}

void SPO2UpdateStatus(void)
{
	u16_t x,y,w,h;
	u8_t tmpbuf[64] = {0};
	unsigned char *img_anima[3] = {IMG_SPO2_ANI_1_ADDR, IMG_SPO2_ANI_2_ADDR, IMG_SPO2_ANI_3_ADDR};

	img_index++;
	if(img_index >= 3)
		img_index = 0;
	LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, img_anima[img_index]);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else	
	LCD_SetFontSize(FONT_SIZE_32);
#endif

	sprintf(tmpbuf, "%d%%", g_spo2);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = SPO2_NUM_X+(SPO2_NUM_W-w)/2;
	y = SPO2_NUM_Y+(SPO2_NUM_H-h)/2;
	LCD_Fill(SPO2_NUM_X, SPO2_NUM_Y, SPO2_NUM_W, SPO2_NUM_H, BLACK);
	LCD_ShowString(x,y,tmpbuf);
}

void SPO2ShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t i,tmpbuf[64] = {0};
	u8_t spo2_max=0,spo2_min=0,spo2[24] = {0};

	LCD_Clear(BLACK);
	
	LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, IMG_SPO2_ANI_2_ADDR);
	LCD_ShowImg_From_Flash(SPO2_BG_X, SPO2_BG_Y, IMG_SPO2_BG_ADDR);
	LCD_ShowImg_From_Flash(SPO2_UP_ARRAW_X, SPO2_UP_ARRAW_Y, IMG_SPO2_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(SPO2_DOWN_ARRAW_X, SPO2_DOWN_ARRAW_Y, IMG_SPO2_DOWN_ARRAW_ADDR);

	GetCurDaySpo2RecData(spo2);
	for(i=0;i<24;i++)
	{
		if(spo2_max == 0 || spo2_min == 0)
		{
			if(spo2[i] > 0)
			{
				spo2_max = spo2[i];
				spo2_min = spo2[i];
			}
		}
		else
		{
			if(spo2[i] > spo2_max)
				spo2_max = spo2[i];
			if(spo2[i] < spo2_min)
				spo2_min = spo2[i];
		}
		
		if(spo2[i] >= 80)
			LCD_Fill(SPO2_REC_DATA_X+SPO2_REC_DATA_OFFSET_X*i, SPO2_REC_DATA_Y-(spo2[i]-80)*3, SPO2_REC_DATA_W, (spo2[i]-80)*3, BLUE);
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else
	LCD_SetFontSize(FONT_SIZE_32);
#endif
	strcpy(tmpbuf, "0");
	LCD_MeasureString(tmpbuf,&w,&h);
	x = SPO2_NUM_X+(SPO2_NUM_W-w)/2;
	y = SPO2_NUM_Y+(SPO2_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

	sprintf(tmpbuf, "%d", spo2_max);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = SPO2_UP_NUM_X+(SPO2_UP_NUM_W-w)/2;
	y = SPO2_UP_NUM_Y+(SPO2_UP_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

	sprintf(tmpbuf, "%d", spo2_min);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = SPO2_DOWN_NUM_X+(SPO2_DOWN_NUM_W-w)/2;
	y = SPO2_DOWN_NUM_Y+(SPO2_DOWN_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);
}

void SPO2ScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_SPO2].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SPO2].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_SPO2].status = SCREEN_STATUS_CREATED;
				
		SPO2ShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		SPO2UpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_SPO2].act = SCREEN_ACTION_NO;
}

void ExitSPO2Screen(void)
{
	k_timer_stop(&mainmenu_timer);

	img_index = 0;
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif

	if(PPGIsWorking())
		MenuStopPPG();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterSPO2Screen(void)
{
	if(screen_id == SCREEN_ID_SPO2)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SPO2;	
	scr_msg[SCREEN_ID_SPO2].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SPO2].status = SCREEN_STATUS_CREATING;

	img_index = 0;
	
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif
		
	if(PPGIsWorking())
		MenuStopPPG();
	
	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	SetLeftKeyUpHandler(EnterBPScreen);
	SetRightKeyUpHandler(ExitSPO2Screen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen); 
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitSPO2Screen);
#endif	
}

void HRUpdateStatus(void)
{
	u16_t x,y,w,h;
	u8_t tmpbuf[64] = {0};
	unsigned char *img_anima[2] = {IMG_HR_ICON_ANI_1_ADDR, IMG_HR_ICON_ANI_2_ADDR};

	img_index++;
	if(img_index >= 2)
		img_index = 0;
	LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, img_anima[img_index]);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else
	LCD_SetFontSize(FONT_SIZE_32);
#endif
	sprintf(tmpbuf, "%d", g_hr);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = HR_NUM_X+(HR_NUM_W-w)/2;
	y = HR_NUM_Y+(HR_NUM_H-h)/2;
	LCD_Fill(HR_NUM_X, HR_NUM_Y, HR_NUM_W, HR_NUM_H, BLACK);
	LCD_ShowString(x,y,tmpbuf);
}

void HRShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t i,tmpbuf[64] = {0};
	u8_t hr_max=0,hr_min=0,hr[24] = {0};
	
	LCD_Clear(BLACK);
	
	LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, IMG_HR_ICON_ANI_2_ADDR);
	LCD_ShowImg_From_Flash(HR_UNIT_X, HR_UNIT_Y, IMG_HR_BPM_ADDR);
	LCD_ShowImg_From_Flash(HR_BG_X, HR_BG_Y, IMG_HR_BG_ADDR);
	LCD_ShowImg_From_Flash(HR_UP_ARRAW_X, HR_UP_ARRAW_Y, IMG_HR_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(HR_DOWN_ARRAW_X, HR_DOWN_ARRAW_Y, IMG_HR_DOWN_ARRAW_ADDR);

	GetCurDayHrRecData(hr);
	for(i=0;i<24;i++)
	{
		if(hr_max == 0 || hr_min == 0)
		{
			if(hr[i] > 0)
			{	
				hr_max = hr[i];
				hr_min = hr[i];
			}
		}
		else
		{
			if(hr[i] > hr_max)
				hr_max = hr[i];
			if(hr[i] < hr_min)
				hr_min = hr[i];
		}
		
		if(hr[i] > 30)
			LCD_Fill(HR_REC_DATA_X+HR_REC_DATA_OFFSET_X*i, HR_REC_DATA_Y-(hr[i]-30)*15/30, HR_REC_DATA_W, (hr[i]-30)*15/30, RED);
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else	
	LCD_SetFontSize(FONT_SIZE_32);
#endif
	strcpy(tmpbuf, "0");
	LCD_MeasureString(tmpbuf,&w,&h);
	x = HR_NUM_X+(HR_NUM_W-w)/2;
	y = HR_NUM_Y+(HR_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

	sprintf(tmpbuf, "%d", hr_max);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = HR_UP_NUM_X+(HR_UP_NUM_W-w)/2;
	y = HR_UP_NUM_Y+(HR_UP_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

	sprintf(tmpbuf, "%d", hr_min);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = HR_DOWN_NUM_X+(HR_DOWN_NUM_W-w)/2;
	y = HR_DOWN_NUM_Y+(HR_DOWN_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);
}

void HRScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_HR].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_HR].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_HR].status = SCREEN_STATUS_CREATED;
				
		HRShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		HRUpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_HR].act = SCREEN_ACTION_NO;
}

void ExitHRScreen(void)
{
	k_timer_stop(&mainmenu_timer);

	img_index = 0;
	
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif

	if(PPGIsWorking())
		MenuStopPPG();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterHRScreen(void)
{
	if(screen_id == SCREEN_ID_HR)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_HR;	
	scr_msg[SCREEN_ID_HR].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_HR].status = SCREEN_STATUS_CREATING;

	img_index = 0;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	SetLeftKeyUpHandler(EnterSPO2Screen);
	SetRightKeyUpHandler(ExitHRScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSPO2Screen); 
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitHRScreen);
#endif	
}
#endif/*CONFIG_PPG_SUPPORT*/

void ShowStringsInRect(u16_t rect_x, u16_t rect_y, u16_t rect_w, u16_t rect_h, u8_t *strbuf)
{
	u16_t x,y,w,h;
	u16_t offset_w=4,offset_h=4;

	LCD_MeasureString(strbuf, &w, &h);

	if(w > (rect_w-2*offset_w))
	{
		u8_t line_count,line_no,line_max;
		u16_t line_h=(h+offset_h);
		u16_t byte_no=0,text_len;

		line_max = (rect_h-2*offset_h)/line_h;
		line_count = w/(rect_w-2*offset_w) + ((w%(rect_w-offset_w) != 0)? 1 : 0);
		if(line_count > line_max)
			line_count = line_max;

		line_no = 0;
		text_len = strlen(strbuf);
		y = ((rect_h-2*offset_h)-line_count*line_h)/2;
		y += (rect_y+offset_h);
		while(line_no < line_count)
		{
			u8_t tmpbuf[128] = {0};
			u8_t i=0;

			tmpbuf[i++] = strbuf[byte_no++];
			LCD_MeasureString(tmpbuf, &w, &h);
			while(w < (rect_w-2*offset_w))
			{
				if(byte_no < text_len)
				{
					tmpbuf[i++] = strbuf[byte_no++];
					LCD_MeasureString(tmpbuf, &w, &h);
				}
				else
				{
					break;
				}
			}

			if(byte_no < text_len)
			{
				//first few rows
				i--;
				byte_no--;
				tmpbuf[i] = 0x00;

				x = (rect_x+offset_w);
				LCD_ShowString(x,y,tmpbuf);

				y += line_h;
				line_no++;
			}
			else
			{
				//last row
				x = (rect_x+offset_w);
				LCD_ShowString(x,y,tmpbuf);
				break;
			}
		}
	}
	else
	{
		x = (w > (rect_w-2*offset_w))? 0 : ((rect_w-2*offset_w)-w)/2;
		y = (h > (rect_h-2*offset_h))? 0 : ((rect_h-2*offset_h)-h)/2;
		x += (rect_x+offset_w);
		y += (rect_y+offset_h);
		LCD_ShowString(x,y,strbuf);				
	}
}

void NotifyShowStrings(u16_t rect_x, u16_t rect_y, u16_t rect_w, u16_t rect_h, u8_t *strbuf)
{
	LCD_DrawRectangle(rect_x, rect_y, rect_w, rect_h);
	LCD_Fill(rect_x+1, rect_y+1, rect_w-2, rect_h-2, BLACK);
	ShowStringsInRect(rect_x, rect_y, rect_w, rect_h, strbuf);	
}

void NotifyShow(void)
{
	u16_t rect_x,rect_y,rect_w=180,rect_h=120;
	u16_t x,y,w,h;
	u16_t offset_w=4,offset_h=4;

	rect_x = (LCD_WIDTH-rect_w)/2;
	rect_y = (LCD_HEIGHT-rect_h)/2;
	
	LCD_DrawRectangle(rect_x, rect_y, rect_w, rect_h);
	LCD_Fill(rect_x+1, rect_y+1, rect_w-2, rect_h-2, BLACK);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	LCD_MeasureString(notify_msg.text, &w, &h);
	switch(notify_msg.align)
	{
	case NOTIFY_ALIGN_CENTER:
		if(w > (rect_w-2*offset_w))
		{
			u8_t line_count,line_no,line_max;
			u16_t line_h=(h+offset_h);
			u16_t byte_no=0,text_len;

			line_max = (rect_h-2*offset_h)/line_h;
			line_count = w/(rect_w-2*offset_w) + ((w%(rect_w-offset_w) != 0)? 1 : 0);
			if(line_count > line_max)
				line_count = line_max;

			line_no = 0;
			text_len = strlen(notify_msg.text);
			y = ((rect_h-2*offset_h)-line_count*line_h)/2;
			y += (rect_y+offset_h);
			while(line_no < line_count)
			{
				u8_t tmpbuf[128] = {0};
				u8_t i=0;

				tmpbuf[i++] = notify_msg.text[byte_no++];
				LCD_MeasureString(tmpbuf, &w, &h);
				while(w < (rect_w-2*offset_w))
				{
					if(byte_no < text_len)
					{
						tmpbuf[i++] = notify_msg.text[byte_no++];
						LCD_MeasureString(tmpbuf, &w, &h);
					}
					else
						break;
				}

				if(byte_no < text_len)
				{
					i--;
					byte_no--;
					tmpbuf[i] = 0x00;

					LCD_MeasureString(tmpbuf, &w, &h);
					x = ((rect_w-2*offset_w)-w)/2;
					x += (rect_x+offset_w);
					LCD_ShowString(x,y,tmpbuf);

					y += line_h;
					line_no++;
				}
				else
				{
					LCD_MeasureString(tmpbuf, &w, &h);
					x = ((rect_w-2*offset_w)-w)/2;
					x += (rect_x+offset_w);
					LCD_ShowString(x,y,tmpbuf);

					break;
				}
			}
		}
		else
		{
			x = (w > (rect_w-2*offset_w))? 0 : ((rect_w-2*offset_w)-w)/2;
			y = (h > (rect_h-2*offset_h))? 0 : ((rect_h-2*offset_h)-h)/2;
			x += (rect_x+offset_w);
			y += (rect_y+offset_h);
			LCD_ShowString(x,y,notify_msg.text);				
		}
		break;
	case NOTIFY_ALIGN_BOUNDARY:
		x = (rect_x+offset_w);
		y = (rect_y+offset_h);
		LCD_ShowStringInRect(x, y, (rect_w-2*offset_w), (rect_h-2*offset_h), notify_msg.text);
		break;
	}
}

void NotifyScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_NOTIFY].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_NOTIFY].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_NOTIFY].status = SCREEN_STATUS_CREATED;
				
		NotifyShow();
		break;
		
	case SCREEN_ACTION_UPDATE:
		break;
	}
	
	scr_msg[SCREEN_ID_NOTIFY].act = SCREEN_ACTION_NO;

}

#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
void DlShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t str_title[128] = {0};

	LCD_Clear(BLACK);
	//LCD_DrawRectangle(DL_NOTIFY_RECT_X, DL_NOTIFY_RECT_Y, DL_NOTIFY_RECT_W, DL_NOTIFY_RECT_H);
	//LCD_Fill(DL_NOTIFY_RECT_X+1, DL_NOTIFY_RECT_Y+1, DL_NOTIFY_RECT_W-1, DL_NOTIFY_RECT_H-1, BLACK);

	switch(g_dl_data_type)
	{
	case DL_DATA_IMG:
		strcpy(str_title, "UI UPGRADING");
		break;
	case DL_DATA_FONT:
		strcpy(str_title, "FONT UPGRADING");
		break;
	case DL_DATA_PPG:
		strcpy(str_title, "PPG_AG UPGRADING");
		break;
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	LCD_MeasureString(str_title, &w, &h);
	x = (w > (DL_NOTIFY_RECT_W-2*DL_NOTIFY_OFFSET_W))? 0 : ((DL_NOTIFY_RECT_W-2*DL_NOTIFY_OFFSET_W)-w)/2;
	x += (DL_NOTIFY_RECT_X+DL_NOTIFY_OFFSET_W);
	y = 25;
	LCD_ShowString(x,y,str_title);

	ShowStringsInRect(DL_NOTIFY_STRING_X, 
					  DL_NOTIFY_STRING_Y, 
					  DL_NOTIFY_STRING_W, 
					  DL_NOTIFY_STRING_H, 
					  "Make sure the battery is more than 80% full or the charger is connected.");

	LCD_DrawRectangle(DL_NOTIFY_YES_X, DL_NOTIFY_YES_Y, DL_NOTIFY_YES_W, DL_NOTIFY_YES_H);
	LCD_MeasureString("SOS(Y)", &w, &h);
	x = DL_NOTIFY_YES_X+(DL_NOTIFY_YES_W-w)/2;
	y = DL_NOTIFY_YES_Y+(DL_NOTIFY_YES_H-h)/2;	
	LCD_ShowString(x,y,"SOS(Y)");

	LCD_DrawRectangle(DL_NOTIFY_NO_X, DL_NOTIFY_NO_Y, DL_NOTIFY_NO_W, DL_NOTIFY_NO_H);
	LCD_MeasureString("PWR(N)", &w, &h);
	x = DL_NOTIFY_NO_X+(DL_NOTIFY_NO_W-w)/2;
	y = DL_NOTIFY_NO_Y+(DL_NOTIFY_NO_H-h)/2;	
	LCD_ShowString(x,y,"PWR(N)");

	SetLeftKeyUpHandler(dl_start_confirm);
	SetRightKeyUpHandler(dl_exit);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_YES_X, DL_NOTIFY_YES_X+DL_NOTIFY_YES_W, DL_NOTIFY_YES_Y, DL_NOTIFY_YES_Y+DL_NOTIFY_YES_H, dl_start_confirm);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_NO_X, DL_NOTIFY_NO_X+DL_NOTIFY_NO_W, DL_NOTIFY_NO_Y, DL_NOTIFY_NO_Y+DL_NOTIFY_NO_H, dl_exit);
#endif
}

void DlUpdateStatus(void)
{
	u16_t pro_len;
	u16_t x,y,w,h;
	u8_t pro_buf[16] = {0};
	u8_t strbuf[256] = {0};
	static bool flag = false;
	static u16_t pro_str_x,pro_str_y;
	
	switch(get_dl_status())
	{
	case DL_STATUS_PREPARE:
		flag = false;
		break;
		
	case DL_STATUS_LINKING:
		LCD_Fill(DL_NOTIFY_RECT_X+1, DL_NOTIFY_STRING_Y, DL_NOTIFY_RECT_W-1, DL_NOTIFY_RECT_H-(DL_NOTIFY_STRING_Y-DL_NOTIFY_RECT_Y)-1, BLACK);
		ShowStringsInRect(DL_NOTIFY_STRING_X,
						  DL_NOTIFY_STRING_Y,
						  DL_NOTIFY_STRING_W,
						  DL_NOTIFY_STRING_H,
						  "Linking to server...");

		ClearAllKeyHandler();
		break;
		
	case DL_STATUS_DOWNLOADING:
		if(!flag)
		{
			flag = true;
			
			LCD_Fill(DL_NOTIFY_STRING_X, DL_NOTIFY_STRING_Y, DL_NOTIFY_STRING_W, DL_NOTIFY_STRING_H, BLACK);
			ShowStringsInRect(DL_NOTIFY_STRING_X, 
							  DL_NOTIFY_STRING_Y,
							  DL_NOTIFY_STRING_W,
							  40,
							  "Downloading data...");
			
			LCD_DrawRectangle(DL_NOTIFY_PRO_X, DL_NOTIFY_PRO_Y, DL_NOTIFY_PRO_W, DL_NOTIFY_PRO_H);
			LCD_Fill(DL_NOTIFY_PRO_X+1, DL_NOTIFY_PRO_Y+1, DL_NOTIFY_PRO_W-1, DL_NOTIFY_PRO_H-1, BLACK);

			sprintf(pro_buf, "%3d%%", g_dl_progress);
			LCD_MeasureString(pro_buf, &w, &h);
			pro_str_x = ((DL_NOTIFY_RECT_W-2*DL_NOTIFY_OFFSET_W)-w)/2;
			pro_str_x += (DL_NOTIFY_RECT_X+DL_NOTIFY_OFFSET_W);
			pro_str_y = DL_NOTIFY_PRO_Y + DL_NOTIFY_PRO_H + 5;
			
			LCD_ShowString(pro_str_x,pro_str_y, pro_buf);
		}
		else
		{
			pro_len = (g_dl_progress*DL_NOTIFY_PRO_W)/100;
			LCD_Fill(DL_NOTIFY_PRO_X+1, DL_NOTIFY_PRO_Y+1, pro_len, DL_NOTIFY_PRO_H-1, WHITE);

			sprintf(pro_buf, "%3d%%", g_dl_progress);
			LCD_ShowString(pro_str_x, pro_str_y, pro_buf);
		}

		ClearAllKeyHandler();
		break;
		
	case DL_STATUS_FINISHED:
		flag = false;
		
		LCD_Fill(DL_NOTIFY_RECT_X+1, DL_NOTIFY_STRING_Y, DL_NOTIFY_RECT_W-1, DL_NOTIFY_RECT_H-(DL_NOTIFY_STRING_Y-DL_NOTIFY_RECT_Y)-1, BLACK);
		switch(g_dl_data_type)
		{
		case DL_DATA_IMG:
			strcpy(strbuf, "Img upgraded successfully! Do you want to reboot the device immediately?");
			break;
		case DL_DATA_FONT:
			strcpy(strbuf, "Font upgraded successfully! Do you want to reboot the device immediately?");
			break;
		case DL_DATA_PPG:
			strcpy(strbuf, "PPG Algo upgraded successfully! Do you want to reboot the device immediately?");
			break;
		}
		ShowStringsInRect(DL_NOTIFY_STRING_X,
						  DL_NOTIFY_STRING_Y,
						  DL_NOTIFY_STRING_W,
						  DL_NOTIFY_STRING_H,
						  strbuf);

		LCD_DrawRectangle(DL_NOTIFY_YES_X, DL_NOTIFY_YES_Y, DL_NOTIFY_YES_W, DL_NOTIFY_YES_H);
		LCD_MeasureString("SOS(Y)", &w, &h);
		x = DL_NOTIFY_YES_X+(DL_NOTIFY_YES_W-w)/2;
		y = DL_NOTIFY_YES_Y+(DL_NOTIFY_YES_H-h)/2;	
		LCD_ShowString(x,y,"SOS(Y)");

		LCD_DrawRectangle(DL_NOTIFY_NO_X, DL_NOTIFY_NO_Y, DL_NOTIFY_NO_W, DL_NOTIFY_NO_H);
		LCD_MeasureString("PWR(N)", &w, &h);
		x = DL_NOTIFY_NO_X+(DL_NOTIFY_NO_W-w)/2;
		y = DL_NOTIFY_NO_Y+(DL_NOTIFY_NO_H-h)/2;	
		LCD_ShowString(x,y,"PWR(N)");

		SetLeftKeyUpHandler(dl_reboot_confirm);
		SetRightKeyUpHandler(dl_exit);
		break;
		
	case DL_STATUS_ERROR:
		flag = false;

		LCD_Fill(DL_NOTIFY_RECT_X+1, DL_NOTIFY_STRING_Y, DL_NOTIFY_RECT_W-1, DL_NOTIFY_RECT_H-(DL_NOTIFY_STRING_Y-DL_NOTIFY_RECT_Y)-1, BLACK);
		switch(g_dl_data_type)
		{
		case DL_DATA_IMG:
			strcpy(strbuf, "Img failed to upgrade! Please check the network or server.");
			break;
		case DL_DATA_FONT:
			strcpy(strbuf, "Font failed to upgrade! Please check the network or server.");
			break;
		case DL_DATA_PPG:
			strcpy(strbuf, "PPG Algo failed to upgrade! Please check the network or server.");
			break;
		}	
		ShowStringsInRect(DL_NOTIFY_STRING_X,
						  DL_NOTIFY_STRING_Y,
						  DL_NOTIFY_STRING_W,
						  DL_NOTIFY_STRING_H,
						  strbuf);

		LCD_DrawRectangle((LCD_WIDTH-DL_NOTIFY_YES_W)/2, DL_NOTIFY_YES_Y, DL_NOTIFY_YES_W, DL_NOTIFY_YES_H);
		LCD_MeasureString("SOS(Y)", &w, &h);
		x = (LCD_WIDTH-DL_NOTIFY_YES_W)/2+(DL_NOTIFY_YES_W-w)/2;
		y = DL_NOTIFY_YES_Y+(DL_NOTIFY_YES_H-h)/2;	
		LCD_ShowString(x,y,"SOS(Y)");

		SetLeftKeyUpHandler(dl_exit);
		SetRightKeyUpHandler(dl_exit);
		break;
		
	case DL_STATUS_MAX:
		flag = false;
		break;
	}
}

void DlScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_DL].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_DL].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_DL].status = SCREEN_STATUS_CREATED;

		DlShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_DL].para&SCREEN_EVENT_UPDATE_DL)
		{
			scr_msg[SCREEN_ID_DL].para &= (~SCREEN_EVENT_UPDATE_DL);
			DlUpdateStatus();
		}

		if(scr_msg[SCREEN_ID_DL].para == SCREEN_EVENT_UPDATE_NO)
			scr_msg[SCREEN_ID_DL].act = SCREEN_ACTION_NO;
		break;
	}
}

#ifdef CONFIG_IMG_DATA_UPDATE
void ExitDlImgScreen(void)
{
#ifdef CONFIG_FONT_DATA_UPDATE
	dl_font_start();
#elif defined(CONFIG_PPG_DATA_UPDATE)
	dl_ppg_start();
#else
	EnterPoweroffScreen();
#endif
}
#endif

#ifdef CONFIG_FONT_DATA_UPDATE
void ExitDlFontScreen(void)
{
#ifdef CONFIG_PPG_DATA_UPDATE
	dl_ppg_start();
#else
	EnterPoweroffScreen();
#endif
}
#endif

#ifdef CONFIG_PPG_DATA_UPDATE
void ExitDlPpgScreen(void)
{
	EnterPoweroffScreen();
}
#endif


void EnterDlScreen(void)
{
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking())
		MenuStopTemp();
#endif

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_DL;	
	scr_msg[SCREEN_ID_DL].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_DL].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);
}
#endif/*CONFIG_DATA_DOWNLOAD_SUPPORT*/

#ifdef CONFIG_FOTA_DOWNLOAD
void FOTAShowStatus(void)
{
	LCD_Clear(BLACK);
	
	LCD_ShowImg_From_Flash(FOTA_LOGO_X, FOTA_LOGO_Y, IMG_OTA_LOGO_ADDR);
	LCD_ShowImg_From_Flash(FOTA_START_STR_X, FOTA_START_STR_Y, IMG_OTA_STR_ADDR);
	LCD_ShowImg_From_Flash(FOTA_YES_X, FOTA_YES_Y, IMG_OTA_YES_ADDR);
	LCD_ShowImg_From_Flash(FOTA_NO_X, FOTA_NO_Y, IMG_OTA_NO_ADDR);

	SetLeftKeyUpHandler(fota_excu);
	SetRightKeyUpHandler(fota_exit);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FOTA_YES_X, FOTA_YES_X+FOTA_YES_W, FOTA_YES_Y, FOTA_YES_Y+FOTA_YES_H, fota_excu);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FOTA_NO_X, FOTA_NO_X+FOTA_NO_W, FOTA_NO_Y, FOTA_NO_Y+FOTA_NO_H, fota_exit);
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_excu);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_exit);	
#endif
}

void FOTAUpdateStatus(void)
{
	u16_t pro_len;
	u16_t x,y,w,h;
	u8_t pro_buf[16] = {0};
	static bool flag = false;
	static u16_t pro_str_x,pro_str_y;
	
	switch(get_fota_status())
	{
	case FOTA_STATUS_PREPARE:
		flag = false;
		break;
		
	case FOTA_STATUS_LINKING:
		LCD_Fill(0, FOTA_YES_Y, LCD_WIDTH, FOTA_YES_H, BLACK);
		LCD_Fill(0, FOTA_START_STR_Y, LCD_WIDTH, FOTA_START_STR_H, BLACK);
		
		LCD_ShowImg_From_Flash(FOTA_RUNNING_STR_X, FOTA_RUNNING_STR_Y, IMG_OTA_RUNNING_STR_ADDR);

		ClearAllKeyHandler();
		break;
		
	case FOTA_STATUS_DOWNLOADING:
		if(!flag)
		{
			flag = true;
			
			LCD_Fill(0, FOTA_START_STR_Y, LCD_WIDTH, FOTA_START_STR_H, BLACK);
			LCD_ShowImg_From_Flash(FOTA_DOWNLOADING_STR_X, FOTA_DOWNLOADING_STR_Y, IMG_OTA_DOWNLOADING_ADDR);
			
			LCD_DrawRectangle(FOTA_PROGRESS_X, FOTA_PROGRESS_Y, FOTA_PROGRESS_W, FOTA_PROGRESS_H);
			LCD_Fill(FOTA_PROGRESS_X+1, FOTA_PROGRESS_Y+1, FOTA_PROGRESS_W-1, FOTA_PROGRESS_H-1, BLACK);
			
			sprintf(pro_buf, "%3d%%", g_fota_progress);
		#ifdef FONTMAKER_UNICODE_FONT
			LCD_SetFontSize(FONT_SIZE_20);
		#else	
			LCD_SetFontSize(FONT_SIZE_16);
		#endif
			LCD_MeasureString(pro_buf, &w, &h);
			pro_str_x = FOTA_PRO_NUM_X+(FOTA_PRO_NUM_W-w)/2;
			pro_str_y = FOTA_PRO_NUM_Y+(FOTA_PRO_NUM_H-h)/2;
			
			LCD_ShowString(pro_str_x,pro_str_y, pro_buf);

		}
		else
		{
			pro_len = (g_fota_progress*FOTA_PROGRESS_W)/100;
			LCD_Fill(FOTA_PROGRESS_X+1, FOTA_PROGRESS_Y+1, pro_len, FOTA_PROGRESS_H-1, WHITE);
			
			sprintf(pro_buf, "%3d%%", g_fota_progress);
			LCD_ShowString(pro_str_x, pro_str_y, pro_buf);
		}		

		ClearAllKeyHandler();
		break;
		
	case FOTA_STATUS_FINISHED:
		flag = false;
		
		LCD_Clear(BLACK);

		LCD_ShowImg_From_Flash(FOTA_FINISH_ICON_X, FOTA_FINISH_ICON_Y, IMG_OTA_FINISH_ICON_ADDR);
		LCD_ShowImg_From_Flash(FOTA_FINISH_STR_X, FOTA_FINISH_STR_Y, IMG_OTA_SUCCESS_STR_ADDR);
		SetLeftKeyUpHandler(fota_reboot_confirm);
		SetRightKeyUpHandler(fota_reboot_confirm);
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_reboot_confirm);
	#endif
		break;
		
	case FOTA_STATUS_ERROR:
		flag = false;

		LCD_Clear(BLACK);

		LCD_ShowImg_From_Flash(FOTA_FAIL_ICON_X, FOTA_FAIL_ICON_Y, IMG_OTA_FAILED_ICON_ADDR);
		LCD_ShowImg_From_Flash(FOTA_FAIL_STR_X, FOTA_FAIL_STR_Y, IMG_OTA_FAIL_STR_ADDR);

		SetLeftKeyUpHandler(fota_exit);
		SetRightKeyUpHandler(fota_exit);
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_exit);
	#endif	
		break;
		
	case FOTA_STATUS_MAX:
		flag = false;
		break;
	}
}

void FOTAScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_FOTA].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_FOTA].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_FOTA].status = SCREEN_STATUS_CREATED;

		FOTAShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_FOTA].para&SCREEN_EVENT_UPDATE_FOTA)
		{
			scr_msg[SCREEN_ID_FOTA].para &= (~SCREEN_EVENT_UPDATE_FOTA);
			FOTAUpdateStatus();
		}

		if(scr_msg[SCREEN_ID_FOTA].para == SCREEN_EVENT_UPDATE_NO)
			scr_msg[SCREEN_ID_FOTA].act = SCREEN_ACTION_NO;
		break;
	}
}

void ExitFOTAScreen(void)
{
#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
#ifdef CONFIG_IMG_DATA_UPDATE
	dl_img_start();
#elif defined(CONFIG_FONT_DATA_UPDATE)
	dl_font_start();
#elif defined(CONFIG_PPG_DATA_UPDATE)
	dl_ppg_start();
#endif
#else
	EnterPoweroffScreen();
#endif
}

void EnterFOTAScreen(void)
{
	if(screen_id == SCREEN_ID_FOTA)
		return;

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking())
		MenuStopTemp();
#endif

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FOTA;	
	scr_msg[SCREEN_ID_FOTA].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FOTA].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);
}
#endif/*CONFIG_FOTA_DOWNLOAD*/

#ifdef CONFIG_IMU_SUPPORT
void FallShowStatus(void)
{
#if 0
	u16_t x,y;
	u32_t img_addr;
	u8_t *img;

	LCD_Clear(BLACK);

	switch(global_settings.language)
	{
	case LANGUAGE_EN:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_FALL_EN_ADDR;
	#else
		img = IMG_FALL_EN;
	#endif
		x = FALL_EN_TEXT_X;
		y = FALL_CN_TEXT_Y;
		break;
		
	case LANGUAGE_CHN:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_FALL_CN_ADDR;
	#else
		img = IMG_FALL_CN;
	#endif
		x = FALL_CN_TEXT_X;
		y = FALL_CN_TEXT_Y;	
		break;
	}

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(FALL_ICON_X, FALL_ICON_Y, IMG_FALL_ICON_ADDR);
	LCD_ShowImg_From_Flash(x, y, img_addr);
#else
	LCD_ShowImg(FALL_ICON_X, FALL_ICON_Y, IMG_FALL_ICON);
	LCD_ShowImg(x, y, img);
#endif
#endif
}

void FallScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_FALL].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_FALL].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_FALL].status = SCREEN_STATUS_CREATED;

		FallShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		break;
	}
	
	scr_msg[SCREEN_ID_FALL].act = SCREEN_ACTION_NO;
}

void SleepScreenProcess(void)
{
	u16_t x,y,w,h;
	u16_t deep_sleep,light_sleep;
	u8_t tmpbuf[64] = {0};
	
	switch(scr_msg[SCREEN_ID_SLEEP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_SLEEP].status = SCREEN_STATUS_CREATED;
				
		LCD_Clear(BLACK);
		
		LCD_ShowImg_From_Flash(SLEEP_TOTAL_ICON_X, SLEEP_TOTAL_ICON_Y, IMG_SLEEP_ANI_3_ADDR);
		LCD_ShowImg_From_Flash(SLEEP_TOTAL_UNIT_HR_X, SLEEP_TOTAL_UNIT_HR_Y, IMG_SLEEP_HOUR_ADDR);
		LCD_ShowImg_From_Flash(SLEEP_TOTAL_UNIT_MIN_X, SLEEP_TOTAL_UNIT_MIN_Y, IMG_SLEEP_MIN_ADDR);
		LCD_ShowImg_From_Flash(SLEEP_SEP_LINE_X, SLEEP_SEP_LINE_Y, IMG_SLEEP_LINE_ADDR);
		LCD_ShowImg_From_Flash(SLEEP_LIGHT_ICON_X, SLEEP_LIGHT_ICON_Y, IMG_SLEEP_LIGHT_ICON_ADDR);
		LCD_ShowImg_From_Flash(SLEEP_DEEP_ICON_X, SLEEP_DEEP_ICON_Y, IMG_SLEEP_DEEP_ICON_ADDR);

		GetSleepTimeData(&deep_sleep, &light_sleep);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else	
		LCD_SetFontSize(FONT_SIZE_32);
	#endif
		sprintf(tmpbuf, "%02d", (deep_sleep+light_sleep)/60);
		LCD_ShowString(SLEEP_TOTAL_STR_HR_X, SLEEP_TOTAL_STR_HR_Y, tmpbuf);
		sprintf(tmpbuf, "%02d", (deep_sleep+light_sleep)%60);
		LCD_ShowString(SLEEP_TOTAL_STR_MIN_X, SLEEP_TOTAL_STR_MIN_Y, tmpbuf);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif
		sprintf(tmpbuf, "%02d:%02d", light_sleep/60, light_sleep%60);
		LCD_ShowString(SLEEP_LIGHT_STR_X, SLEEP_LIGHT_STR_Y, tmpbuf);
		sprintf(tmpbuf, "%02d:%02d", deep_sleep/60, deep_sleep%60);
		LCD_ShowString(SLEEP_DEEP_STR_X, SLEEP_DEEP_STR_Y, tmpbuf);
		break;
		
	case SCREEN_ACTION_UPDATE:
		GetSleepTimeData(&deep_sleep, &light_sleep);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else	
		LCD_SetFontSize(FONT_SIZE_32);
	#endif
		sprintf(tmpbuf, "%02d", (deep_sleep+light_sleep)/60);
		LCD_ShowString(SLEEP_TOTAL_STR_HR_X, SLEEP_TOTAL_STR_HR_Y, tmpbuf);
		sprintf(tmpbuf, "%02d", (deep_sleep+light_sleep)%60);
		LCD_ShowString(SLEEP_TOTAL_STR_MIN_X, SLEEP_TOTAL_STR_MIN_Y, tmpbuf);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif
		sprintf(tmpbuf, "%02d:%02d", light_sleep/60, light_sleep%60);
		LCD_ShowString(SLEEP_LIGHT_STR_X, SLEEP_LIGHT_STR_Y, tmpbuf);
		sprintf(tmpbuf, "%02d:%02d", deep_sleep/60, deep_sleep%60);
		LCD_ShowString(SLEEP_DEEP_STR_X, SLEEP_DEEP_STR_Y, tmpbuf);
		break;
	}
	
	scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
}

void ExitSleepScreen(void)
{
	EnterIdleScreen();
}

void EnterSleepScreen(void)
{
	if(screen_id == SCREEN_ID_SLEEP)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SLEEP;	
	scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SLEEP].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);

	MenuStopGPS();

#ifdef CONFIG_PPG_SUPPORT
	SetLeftKeyUpHandler(EnterHRScreen);
#elif defined(CONFIG_TEMP_SUPPORT)
	SetLeftKeyUpHandler(EnterTempScreen);
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#elif defined(CONFIG_FOTA_DOWNLOAD)
	SetLeftKeyUpHandler(fota_start);
#elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
	SetLeftKeyUpHandler(dl_img_start);
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_font_start);
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_ppg_start);
  #endif
#else
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#endif
	SetRightKeyUpHandler(ExitSleepScreen);

#ifdef CONFIG_TOUCH_SUPPORT
 #ifdef CONFIG_PPG_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen); 
 #elif defined(CONFIG_TEMP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);  
 #elif defined(CONFIG_SYNC_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen); 
 #elif defined(CONFIG_FOTA_DOWNLOAD)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_start); 
 #elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);   
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);   
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);  
 #endif
#else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
#endif
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitSleepScreen);
#endif
}

void StepsScreenProcess(void)
{
	u16_t x,y,w,h;
	u16_t steps,calorie,distance;
	u8_t strbuf[64] = {0};
	u8_t tmpbuf[64] = {0};
	
	switch(scr_msg[SCREEN_ID_STEPS].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_STEPS].status = SCREEN_STATUS_CREATED;
				
		LCD_Clear(BLACK);
	
		LCD_ShowImg_From_Flash(IMU_STEP_ICON_X, IMU_STEP_ICON_Y, IMG_STEP_ANI_1_ADDR);
		LCD_ShowImg_From_Flash(IMU_STEP_UNIT_X, IMU_STEP_UNIT_Y, IMG_STEP_ICON_ADDR);
		LCD_ShowImg_From_Flash(IMU_SEP_LINE_X, IMU_SEP_LINE_Y, IMG_STEP_LINE_ADDR);
		LCD_ShowImg_From_Flash(IMU_CAL_ICON_X, IMU_CAL_ICON_Y, IMG_STEP_CAL_ICON_ADDR);
		LCD_ShowImg_From_Flash(IMU_CAL_UNIT_X, IMU_CAL_UNIT_Y, IMG_STEP_KCAL_ADDR);
		LCD_ShowImg_From_Flash(IMU_DIS_ICON_X, IMU_DIS_ICON_Y, IMG_STEP_DIS_ICON_ADDR);
		LCD_ShowImg_From_Flash(IMU_DIS_UNIT_X, IMU_DIS_UNIT_Y, IMG_STEP_KM_ADDR);

		GetSportData(&steps, &calorie, &distance);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else	
		LCD_SetFontSize(FONT_SIZE_32);
	#endif
		sprintf(strbuf, "%d", steps);
		LCD_MeasureString(strbuf,&w,&h);
		LCD_ShowString(IMU_STEP_UNIT_X-w-5, IMU_STEP_STR_Y, strbuf);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif
		sprintf(strbuf, "%d", calorie);
		LCD_MeasureString(strbuf,&w,&h);
		LCD_ShowString(IMU_CAL_UNIT_X-w-5, IMU_CAL_STR_Y, strbuf);	

		sprintf(strbuf, "%d.%d", (distance/1000), (distance%1000));
		LCD_MeasureString(strbuf,&w,&h);
		LCD_ShowString(IMU_DIS_UNIT_X-w-5, IMU_DIS_STR_Y, strbuf);
		break;
		
	case SCREEN_ACTION_UPDATE:
		GetSportData(&steps, &calorie, &distance);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else
		LCD_SetFontSize(FONT_SIZE_32);
	#endif	
		sprintf(strbuf, "%0d", steps);
		LCD_MeasureString(strbuf,&w,&h);
		LCD_Fill(IMU_STEP_STR_X, IMU_STEP_STR_Y, IMU_STEP_STR_W, IMU_STEP_STR_H, BLACK);
		LCD_ShowString(IMU_STEP_UNIT_X-w-5, IMU_STEP_STR_Y, strbuf);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif	
		sprintf(strbuf, "%d", calorie);
		LCD_MeasureString(strbuf,&w,&h);
		LCD_Fill(IMU_CAL_STR_X, IMU_CAL_STR_Y, IMU_CAL_STR_W, IMU_CAL_STR_H, BLACK);
		LCD_ShowString(IMU_CAL_UNIT_X-w-5, IMU_CAL_STR_Y, strbuf);	

		sprintf(strbuf, "%d.%d", (distance/1000), (distance%1000));
		LCD_MeasureString(strbuf,&w,&h);
		LCD_Fill(IMU_DIS_STR_X, IMU_DIS_STR_Y, IMU_DIS_STR_W, IMU_DIS_STR_H, BLACK);
		LCD_ShowString(IMU_DIS_UNIT_X-w-5, IMU_DIS_STR_Y, strbuf);
		break;
	}
	
	scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_NO;
}

void ExitStepsScreen(void)
{
	EnterIdleScreen();
}

void EnterStepsScreen(void)
{
	if(screen_id == SCREEN_ID_STEPS)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_STEPS;	
	scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_STEPS].status = SCREEN_STATUS_CREATING;

	SetLeftKeyUpHandler(EnterSleepScreen);
	SetRightKeyUpHandler(ExitStepsScreen);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitStepsScreen);
#endif
}
#endif

void SettingsScreenProcess(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[64] = {0};
	u8_t tmpbuf[64] = {0};
	
	switch(scr_msg[SCREEN_ID_SETTINGS].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SETTINGS].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_SETTINGS].status = SCREEN_STATUS_CREATED;
				
		LCD_Clear(BLACK);
		
		break;
		
	case SCREEN_ACTION_UPDATE:

		break;
	}
	
	scr_msg[SCREEN_ID_SETTINGS].act = SCREEN_ACTION_NO;
}

void ExitSettingsScreen(void)
{
	EnterIdleScreen();
}

void EnterSettingsScreen(void)
{
	if(screen_id == SCREEN_ID_SETTINGS)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SETTINGS;	
	scr_msg[SCREEN_ID_SETTINGS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SETTINGS].status = SCREEN_STATUS_CREATING;

	ClearLeftKeyUpHandler();
	SetRightKeyUpHandler(ExitSettingsScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	//register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
	//register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitStepsScreen);
#endif
}

void WristShowStatus(void)
{
#if 0
	u16_t x,y;
	u32_t img_addr;
	u8_t *img;

	LCD_Clear(BLACK);

	switch(global_settings.language)
	{
	case LANGUAGE_EN:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_WRIST_EN_ADDR;
	#else
		img = IMG_WRIST_EN;
	#endif
		x = WRIST_EN_TEXT_X;
		y = WRIST_EN_TEXT_Y;
		break;
		
	case LANGUAGE_CHN:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_WRIST_CN_ADDR;
	#else
		img = IMG_WRIST_CN;
	#endif
		x = WRIST_CN_TEXT_X;
		y = WRIST_CN_TEXT_Y;	
		break;
	}

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(FALL_ICON_X, FALL_ICON_Y, IMG_WRIST_ICON_ADDR);
	LCD_ShowImg_From_Flash(x, y, img_addr);
#else
	LCD_ShowImg(FALL_ICON_X, FALL_ICON_Y, IMG_WRIST_ICON);
	LCD_ShowImg(x, y, img);
#endif
#endif
}

void WristScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_WRIST].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_WRIST].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_WRIST].status = SCREEN_STATUS_CREATED;

		WristShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		break;
	}
	
	scr_msg[SCREEN_ID_WRIST].act = SCREEN_ACTION_NO;
}

void EnterIdleScreen(void)
{
	if(screen_id == SCREEN_ID_IDLE)
		return;

	k_timer_stop(&notify_timer);
	
	k_timer_stop(&mainmenu_timer);
	if(gps_is_working())
		MenuStopGPS();
#ifdef CONFIG_PPG_SUPPORT
	PPGStopCheck();
#endif

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_IDLE;
	scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATING;

#ifdef NB_SIGNAL_TEST
	SetLeftKeyUpHandler(EnterNBTestScreen);
#else
#ifdef CONFIG_IMU_SUPPORT
	SetLeftKeyUpHandler(EnterStepsScreen);
#elif defined(CONFIG_PPG_SUPPORT)
	SetLeftKeyUpHandler(EnterHRScreen);
#elif defined(CONFIG_TEMP_SUPPORT)
	SetLeftKeyUpHandler(EnterTempScreen);
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#elif defined(CONFIG_FOTA_DOWNLOAD)
	SetLeftKeyUpHandler(fota_start);
#elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
  #ifdef CONFIG_IMG_DATA_UPDATE
	SetLeftKeyUpHandler(dl_img_start);
  #elif defined(CONFIG_FONT_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_font_start);
  #elif defined(CONFIG_PPG_DATA_UPDATE)
	SetLeftKeyUpHandler(dl_ppg_start);
  #endif
#else
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#endif
#endif
	SetLeftKeyLongPressHandler(SOSTrigger);
	SetRightKeyUpHandler(EnterIdleScreen);

#ifdef CONFIG_TOUCH_SUPPORT
 #ifdef NB_SIGNAL_TEST
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterNBTestScreen);
 #else
  #ifdef CONFIG_IMU_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);  
  #elif defined(CONFIG_PPG_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #elif defined(CONFIG_SYNC_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
  #elif defined(CONFIG_FOTA_DOWNLOAD)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_start);
  #elif defined(CONFIG_DATA_DOWNLOAD_SUPPORT)
   #ifdef CONFIG_IMG_DATA_UPDATE
   	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
   #elif defined(CONFIG_FONT_DATA_UPDATE)
   	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
   #elif defined(CONFIG_PPG_DATA_UPDATE)
   	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
   #endif
  #else
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
  #endif
 #endif
#endif	
}

void EnterAlarmScreen(void)
{
	if(screen_id == SCREEN_ID_ALARM)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_ALARM;	
	scr_msg[SCREEN_ID_ALARM].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_ALARM].status = SCREEN_STATUS_CREATING;	
}

void EnterFindDeviceScreen(void)
{
	if(screen_id == SCREEN_ID_FIND_DEVICE)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FIND_DEVICE;	
	scr_msg[SCREEN_ID_FIND_DEVICE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FIND_DEVICE].status = SCREEN_STATUS_CREATING;
}

void TestGPSUpdateInfor(void)
{
	LCD_Fill((LCD_WIDTH-194)/2, 50, 194, 160, BLACK);
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	LCD_ShowStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, gps_test_info);
}

void TestGPSShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);
	strcpy(strbuf, "GPS TESTING");
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, "GPS Starting...");
}

void TestGPSScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_GPS_TEST].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_GPS_TEST].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_GPS_TEST].status = SCREEN_STATUS_CREATED;

		TestGPSShowInfor();
		break;
		
	case SCREEN_ACTION_UPDATE:
		TestGPSUpdateInfor();
		break;
	}
	
	scr_msg[SCREEN_ID_GPS_TEST].act = SCREEN_ACTION_NO;
}

void ExitGPSTestScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	
	if(gps_is_working())
		MenuStopGPS();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterGPSTestScreen(void)
{
	if(screen_id == SCREEN_ID_GPS_TEST)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_GPS_TEST;	
	scr_msg[SCREEN_ID_GPS_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_GPS_TEST].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

#ifdef CONFIG_PPG_SUPPORT
	PPGStopCheck();
#endif

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	SetLeftKeyUpHandler(EnterPoweroffScreen);
	SetRightKeyUpHandler(ExitGPSTestScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitGPSTestScreen);
#endif	
}

void TestNBUpdateINfor(void)
{
	LCD_Fill(30, 50, 190, 160, BLACK);
	LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);
}

void TestNBShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	strcpy(strbuf, "NB-IoT TESTING");
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);

}

void TestNBScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_NB_TEST].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_NB_TEST].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_NB_TEST].status = SCREEN_STATUS_CREATED;

		TestNBShowInfor();
		break;
		
	case SCREEN_ACTION_UPDATE:
		TestNBUpdateINfor();
		break;
	}
	
	scr_msg[SCREEN_ID_NB_TEST].act = SCREEN_ACTION_NO;
}

void ExitNBTestScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	
	LCD_Set_BL_Mode(LCD_BL_AUTO);

	EnterIdleScreen();
}

void EnterNBTestScreen(void)
{
	if(screen_id == SCREEN_ID_NB_TEST)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_NB_TEST;	
	scr_msg[SCREEN_ID_NB_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_NB_TEST].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
	
	if(gps_is_working())
		MenuStopGPS();

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	SetLeftKeyUpHandler(EnterGPSTestScreen);
	SetRightKeyUpHandler(ExitNBTestScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterGPSTestScreen);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitNBTestScreen);
#endif
}

void SOSUpdateStatus(void)
{
	switch(sos_state)
	{
	case SOS_STATUS_IDLE:
		break;
		
	case SOS_STATUS_SENDING:
		break;
	
	case SOS_STATUS_SENT:
	#ifdef CONFIG_ANIMATION_SUPPORT	
		AnimaStopShow();
	#endif

		LCD_ShowImg_From_Flash(SOS_ICON_X, SOS_ICON_Y, IMG_SOS_ANI_4_ADDR);
		sos_state = SOS_STATUS_RECEIVED;
		break;
	
	case SOS_STATUS_RECEIVED:
		break;
	
	case SOS_STATUS_CANCEL:
		break;
	}
}

void SOSShowStatus(void)
{
	u32_t img_anima[4] = {IMG_SOS_ANI_1_ADDR,IMG_SOS_ANI_2_ADDR,IMG_SOS_ANI_3_ADDR,IMG_SOS_ANI_4_ADDR};
	
	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(SOS_ICON_X, SOS_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#endif

	SOSSChangrStatus();
}

void SOSScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_SOS].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SOS].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_SOS].status = SCREEN_STATUS_CREATED;

		SOSShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_SOS].para&SCREEN_EVENT_UPDATE_SOS)
		{
			scr_msg[SCREEN_ID_SOS].para &= (~SCREEN_EVENT_UPDATE_SOS);
			SOSUpdateStatus();
		}

		if(scr_msg[SCREEN_ID_SOS].para == SCREEN_EVENT_UPDATE_NO)
			scr_msg[SCREEN_ID_SOS].act = SCREEN_ACTION_NO;
		break;
	}
}

void EnterSOSScreen(void)
{
	if(screen_id == SCREEN_ID_SOS)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SOS;	
	scr_msg[SCREEN_ID_SOS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SOS].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);
	MenuStopGPS();

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif

	ClearAllKeyHandler();
}

void EnterFallScreen(void)
{
	if(screen_id == SCREEN_ID_FALL)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FALL;	
	scr_msg[SCREEN_ID_FALL].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FALL].status = SCREEN_STATUS_CREATING;

	k_timer_start(&notify_timer, K_SECONDS(NOTIFY_TIMER_INTERVAL), NULL);
}

void ExitWristScreen(void)
{
	if(screen_id == SCREEN_ID_WRIST)
	{
		EnterIdleScreen();
	}
}

void EnterWristScreen(void)
{
	if(screen_id == SCREEN_ID_WRIST)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_WRIST;	
	scr_msg[SCREEN_ID_WRIST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_WRIST].status = SCREEN_STATUS_CREATING;

	k_timer_start(&notify_timer, K_SECONDS(NOTIFY_TIMER_INTERVAL), NULL);
}

void UpdataTestGPSInfo(void)
{
	if(screen_id == SCREEN_ID_GPS_TEST)
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
}

void GoBackHistoryScreen(void)
{
	SCREEN_ID_ENUM scr_id;
	
	scr_id = screen_id;
	scr_msg[scr_id].act = SCREEN_ACTION_NO;
	scr_msg[scr_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_IDLE;
	scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATING;	
}

void ScreenMsgProcess(void)
{
	if(scr_msg[screen_id].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[screen_id].status != SCREEN_STATUS_CREATED)
			scr_msg[screen_id].act = SCREEN_ACTION_ENTER;

		switch(screen_id)
		{
		case SCREEN_ID_IDLE:
			IdleScreenProcess();
			break;
		case SCREEN_ID_ALARM:
			AlarmScreenProcess();
			break;
		case SCREEN_ID_FIND_DEVICE:
			FindDeviceScreenProcess();
			break;
	#ifdef CONFIG_PPG_SUPPORT	
		case SCREEN_ID_HR:
			HRScreenProcess();
			break;
		case SCREEN_ID_SPO2:
			SPO2ScreenProcess();
			break;
		case SCREEN_ID_ECG:
			break;
		case SCREEN_ID_BP:
			BPScreenProcess();
			break;
	#endif		
		case SCREEN_ID_SOS:
			SOSScreenProcess();
			break;
	#ifdef CONFIG_IMU_SUPPORT	
		case SCREEN_ID_SLEEP:
			SleepScreenProcess();
			break;
		case SCREEN_ID_STEPS:
			StepsScreenProcess();
			break;
		case SCREEN_ID_FALL:
			FallScreenProcess();
			break;	
	#endif
		case SCREEN_ID_WRIST:
			WristScreenProcess();
			break;				
		case SCREEN_ID_SETTINGS:
			break;
		case SCREEN_ID_GPS_TEST:
			TestGPSScreenProcess();
			break;
		case SCREEN_ID_NB_TEST:
			TestNBScreenProcess();
			break;
		case SCREEN_ID_POWEROFF:
			PowerOffScreenProcess();
			break;
		case SCREEN_ID_NOTIFY:
			NotifyScreenProcess();
			break;
	#ifdef CONFIG_SYNC_SUPPORT	
		case SCREEN_ID_SYNC:
			SyncScreenProcess();
			break;
	#endif		
	#ifdef CONFIG_FOTA_DOWNLOAD
		case SCREEN_ID_FOTA:
			FOTAScreenProcess();
			break;
	#endif
	#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
		case SCREEN_ID_DL:
			DlScreenProcess();
			break;
	#endif
	#ifdef CONFIG_TEMP_SUPPORT
		case SCREEN_ID_TEMP:
			TempScreenProcess();
			break;
	#endif
		}
	}
}

