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
static bool exit_notify_flag = false;
static bool entry_idle_flag = false;
static bool entry_setting_bk_flag = false;

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
	IMG_PWRON_ANI_1_ADDR,
	IMG_PWRON_ANI_1_ADDR,
	IMG_PWRON_ANI_1_ADDR,
	IMG_PWRON_ANI_1_ADDR,
	IMG_PWRON_ANI_1_ADDR
};
#endif

void EnterSettingsScreen(void);
void EnterSyncDataScreen(void);
void EnterTempScreen(void);
void EnterSPO2Screen(void);
void EnterBPScreen(void);
void EnterSleepScreen(void);
void EnterStepsScreen(void);

void ShowBootUpLogoFinished(void)
{
	EnterIdleScreen();
}

void ShowBootUpLogo(void)
{
	u8_t i,count=0;
	u16_t x,y,w,h;

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(PWRON_LOGO_X, PWRON_LOGO_Y, logo_img, ARRAY_SIZE(logo_img), 200, false, ShowBootUpLogoFinished);
#else
  #ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(PWRON_LOGO_X, PWRON_LOGO_Y, IMG_PWRON_ANI_6_ADDR);
  #else
	LCD_ShowImg(PWRON_LOGO_X, PWRON_LOGO_Y, IMG_PWRON_ANI_6_ADDR);
  #endif
  
	k_sleep(K_MSEC(1000));
	ShowBootUpLogoFinished();
#endif
}

extern bool ppg_start_flag;
extern bool gps_test_start_flag;
void MainMenuTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_HR)
	{
	#ifdef CONFIG_PPG_SUPPORT
		if(get_hr_ok_flag)
		{
			EntryIdleScr();
		}
		else
		{
			MenuStartHr();
		}
	#endif
	}
	else if(screen_id == SCREEN_ID_SPO2)
	{
	#ifdef CONFIG_PPG_SUPPORT
		if(get_spo2_ok_flag)
		{
			EntryIdleScr();
		}
		else
		{
			MenuStartSpo2();
		}
	#endif
	}
	else if(screen_id == SCREEN_ID_BP)
	{
	#ifdef CONFIG_PPG_SUPPORT
		if(get_bpt_ok_flag)
		{
			EntryIdleScr();
		}
		else
		{
			MenuStartBpt();
		}
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
#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
	else if(screen_id == SCREEN_ID_DL)
	{
		switch(g_dl_data_type)
		{
		case DL_DATA_IMG:
			switch(get_dl_status())
			{
			case DL_STATUS_PREPARE:
				dl_start();
				break;
				
			case DL_STATUS_FINISHED:
			case DL_STATUS_ERROR:
				if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0))
					dl_font_start();
			#if defined(CONFIG_PPG_SUPPORT)
				else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
					dl_ppg_start();
			#endif
				else
					dl_reboot_confirm();
				break;
			}
			break;

		case DL_DATA_FONT:
			switch(get_dl_status())
			{
			case DL_STATUS_PREPARE:
				dl_start();
				break;

			case DL_STATUS_FINISHED:
			case DL_STATUS_ERROR:
			#if defined(CONFIG_PPG_SUPPORT)
				if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
					dl_ppg_start();
			#endif
				else
					dl_reboot_confirm();
				break;
			}
			break;
			
		case DL_DATA_PPG:
			switch(get_dl_status())
			{
			case DL_STATUS_PREPARE:
				dl_start();
				break;

			case DL_STATUS_FINISHED:
			case DL_STATUS_ERROR:
				dl_reboot_confirm();
				break;
			}
			break;
		}
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
		if(get_temp_ok_flag)
		{
			EntryIdleScr();
		}
		else
		{
			MenuStartTemp();
		}
	}
#endif
	else if(screen_id == SCREEN_ID_SETTINGS)
	{
		ExitSettingsScreen();
	}
	else if(screen_id == SCREEN_ID_POWEROFF)
	{
		EntryIdleScr();
	}
}

void IdleShowSystemDate(void)
{
	u16_t x,y,w,h;
	u8_t str_date[20] = {0};
	u8_t tmpbuf[128] = {0};
#ifdef FONTMAKER_UNICODE_FONT	
	u16_t str_mon[LANGUAGE_MAX][12][5] = {
										{
											{0x004A,0x0061,0x006E,0x0000,0x0000},//"Jan"
											{0x0046,0x0065,0x0062,0x0000,0x0000},//"Feb"
											{0x004D,0x0061,0x0072,0x0000,0x0000},//"Mar"
											{0x0041,0x0070,0x0072,0x0000,0x0000},//"Apr"
											{0x004D,0x0061,0x0079,0x0000,0x0000},//"May"
											{0x004A,0x0075,0x006E,0x0000,0x0000},//"Jun"
											{0x004A,0x0075,0x006C,0x0000,0x0000},//"Jul"
											{0x0041,0x0075,0x0067,0x0000,0x0000},//"Aug"
											{0x0053,0x0065,0x0070,0x0074,0x0000},//"Sept"
											{0x004F,0x0063,0x0074,0x0000,0x0000},//"Oct"
											{0x004E,0x006F,0x0076,0x0000,0x0000},//"Nov"
											{0x0044,0x0065,0x0063,0x0000,0x0000} //"Dec"
										},
										{
											{0x004A,0x0061,0x006E,0x0000,0x0000},//"Jan"
											{0x0046,0x0065,0x0062,0x0000,0x0000},//"Feb"
											{0x004D,0x00E4,0x0072,0x007A,0x0000},//"M鋜z"
											{0x0041,0x0070,0x0072,0x0000,0x0000},//"Apr"
											{0x004D,0x0061,0x0069,0x0000,0x0000},//"Mai"
											{0x004A,0x0075,0x006E,0x0000,0x0000},//"Jun"
											{0x004A,0x0075,0x006C,0x0000,0x0000},//"Jul"
											{0x0041,0x0075,0x0067,0x0000,0x0000},//"Aug"
											{0x0053,0x0065,0x0070,0x0074,0x0000},//"Sept"
											{0x004F,0x006B,0x0074,0x0000,0x0000},//"Okt"
											{0x004E,0x006F,0x0076,0x0000,0x0000},//"Nov"
											{0x0044,0x0065,0x007A,0x0000,0x0000} //"Dez"
										},
										{
											{0x004A,0x0061,0x006E,0x0000,0x0000},//"Jan"
											{0x0046,0x0065,0x0062,0x0000,0x0000},//"Feb"
											{0x004D,0x0061,0x0072,0x0000,0x0000},//"Mar"
											{0x0041,0x0070,0x0072,0x0000,0x0000},//"Apr"
											{0x004D,0x0061,0x0079,0x0000,0x0000},//"May"
											{0x004A,0x0075,0x006E,0x0000,0x0000},//"Jun"
											{0x004A,0x0075,0x006C,0x0000,0x0000},//"Jul"
											{0x0041,0x0075,0x0067,0x0000,0x0000},//"Aug"
											{0x0053,0x0065,0x0070,0x0074,0x0000},//"Sept"
											{0x004F,0x0063,0x0074,0x0000,0x0000},//"Oct"
											{0x004E,0x006F,0x0076,0x0000,0x0000},//"Nov"
											{0x0044,0x0065,0x0063,0x0000,0x0000} //"Dec"
										},
									};
#else	
	u8_t *str_mon[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sept","Oct","Nov","Dec"};
#endif

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);

	LCD_FillColor(IDLE_DATE_DAY_X, IDLE_DATE_DAY_Y, IDLE_DATE_DAY_W, IDLE_DATE_DAY_H, BLACK);
	sprintf((char*)str_date, "%02d", date_time.day);
	mmi_asc_to_ucs2(tmpbuf, str_date);
	LCD_ShowUniString(IDLE_DATE_DAY_X, IDLE_DATE_DAY_Y, (u16_t*)tmpbuf);

	LCD_FillColor(IDLE_DATE_MON_X, IDLE_DATE_MON_Y, IDLE_DATE_MON_W, IDLE_DATE_MON_H, BLACK);
	LCD_ShowUniString(IDLE_DATE_MON_X, IDLE_DATE_MON_Y, str_mon[global_settings.language][date_time.month-1]);
#else
	LCD_SetFontSize(FONT_SIZE_32);

	sprintf((char*)str_date, "%02d", date_time.day);
	LCD_ShowString(IDLE_DATE_DAY_X,IDLE_DATE_DAY_Y,str_date);
	strcpy((char*)str_date, str_mon[date_time.month-1]);
	LCD_ShowString(IDLE_DATE_MON_X,IDLE_DATE_MON_Y,str_date);
#endif
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
#ifdef FONTMAKER_UNICODE_FONT	
	u16_t str_week[LANGUAGE_MAX][7][4] = {
										{
											{0x0053,0x0075,0x006E,0x0000},//"Sun"
											{0x004D,0x006F,0x006E,0x0000},//"Mon"
											{0x0054,0x0075,0x0065,0x0000},//"Tue"
											{0x0057,0x0065,0x0064,0x0000},//"Wed"
											{0x0054,0x0068,0x0075,0x0000},//"Thu"
											{0x0046,0x0072,0x0069,0x0000},//"Fri"
											{0x0053,0x0061,0x0074,0x0000} //"Sat"
										},
										{
											{0x0053,0x006F,0x0000},//So
											{0x004D,0x006F,0x0000},//Mo
											{0x0044,0x0049,0x0000},//DI
											{0x004D,0x0049,0x0000},//MI
											{0x0044,0x006F,0x0000},//Do
											{0x0046,0x0072,0x0000},//Fr
											{0x0053,0x0061,0x0000} //Sa
										},
										{
		 									{0x65E5,0x0000},//"日"
											{0x4E00,0x0000},//"一"
											{0x4E8C,0x0000},//"二"
											{0x4E09,0x0000},//"三"
											{0x56DB,0x0000},//"四"
											{0x4E94,0x0000},//"五"
											{0x516D,0x0000} //"六"
										},
									};
#else
	u8_t str_week[128] = {0};
#endif
	
	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
	LCD_FillColor(IDLE_WEEK_X, IDLE_WEEK_Y, IDLE_WEEK_W, IDLE_WEEK_H, BLACK);
	LCD_ShowUniString(IDLE_WEEK_X, IDLE_WEEK_Y, str_week[global_settings.language][date_time.week]);
#else
	LCD_SetFontSize(FONT_SIZE_32);
	GetSystemWeekStrings(str_week);
	LCD_ShowString(IDLE_WEEK_X, IDLE_WEEK_Y, str_week);
#endif
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

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
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
	#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
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
	k_timer_stop(&mainmenu_timer);
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
	k_timer_stop(&mainmenu_timer);
	EnterIdleScreen();
}

void EnterPoweroffScreen(void)
{
	if(screen_id == SCREEN_ID_POWEROFF)
		return;

#ifdef NB_SIGNAL_TEST
	if(gps_is_working())
		MenuStopGPS();
#endif

#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);

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
	u32_t img_anima[3] = {IMG_RUNNING_ANI_1_ADDR, IMG_RUNNING_ANI_2_ADDR, IMG_RUNNING_ANI_3_ADDR};

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

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
	SetLeftKeyLongPressHandler(poweroff_confirm);
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, PWR_OFF_ICON_X, PWR_OFF_ICON_X+PWR_OFF_ICON_W, PWR_OFF_ICON_Y, PWR_OFF_ICON_Y+PWR_OFF_ICON_H, poweroff_confirm);

	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);

 #ifdef NB_SIGNAL_TEST
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterGPSTestScreen);
 #else
  #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
   #ifdef CONFIG_PPG_DATA_UPDATE
   	if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
  	{
  		register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
   	}
	else
   #endif	
	{
		if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
		{
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
		}
		else if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
		{
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
		}
		else
		{
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
		}
	} 
  #endif
 #endif 
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

void SettingsUpdateStatus(void)
{
	u8_t i,count;
	u16_t x,y,w,h;
	u16_t bg_clor = 0x2124;
	u16_t green_clor = 0x07e0;
	
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	LCD_SetFontBgColor(bg_clor);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	switch(settings_menu.id)
	{
	case SETTINGS_MENU_MAIN:
		{
			u16_t menu_sle_str[LANGUAGE_MAX][3][20] = {
														{
															{0x0045,0x006e,0x0067,0x006c,0x0069,0x0073,0x0068,0x0000},//English
															{0x0052,0x0065,0x0073,0x0065,0x0074,0x0000},//Reset
															{0x0056,0x0069,0x0065,0x0077,0x0000},//View
														},
														{
															{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
															{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0000},//Zurücksetzen
															{0x0041,0x006E,0x0073,0x0069,0x0063,0x0068,0x0074,0x0000},//Ansicht
														},
														{
															{0x4E2D,0x6587,0x0000},//中文
															{0x91CD,0x7F6E,0x0000},//重置
															{0x67E5,0x770B,0x0000},//查看
														},
													  };
			u16_t level_1_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000};
			u16_t level_2_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000};
			u16_t level_3_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000};
			u16_t level_4_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000};
			u16_t *level_str[4] = {level_1_str,level_2_str,level_3_str,level_4_str};
			u32_t img_addr[2] = {IMG_SET_TEMP_UNIT_C_ICON_ADDR, IMG_SET_TEMP_UNIT_F_ICON_ADDR};

			entry_setting_bk_flag = false;
			
			LCD_Clear(BLACK);
			
			for(i=0;i<4;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);
				
			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(WHITE);
			
				if(settings_menu.index == 4)
				{
					LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
											settings_menu.name[global_settings.language][i+4]);
			
					LCD_SetFontColor(green_clor);
					if(i == 0)
					{
						LCD_ShowImg_From_Flash(SETTINGS_MENU_TEMP_UNIT_X, SETTINGS_MENU_TEMP_UNIT_Y, img_addr[global_settings.temp_unit]);
					}
					else
					{
						LCD_MeasureUniString(menu_sle_str[global_settings.language][2], &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
												SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
												menu_sle_str[global_settings.language][2]);
					}
				}
				else
				{
					LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
											settings_menu.name[global_settings.language][i]);
			
					LCD_SetFontColor(green_clor);
					if(i == 3)
					{
						LCD_MeasureUniString(level_str[global_settings.backlight_level], &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
												SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
												level_str[global_settings.backlight_level]);
					}
					else
					{
						LCD_MeasureUniString(menu_sle_str[global_settings.language][i], &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
												SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
												menu_sle_str[global_settings.language][i]);
					}
				}
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_BG_X, 
											SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W, 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_BG_H, 
											settings_menu.sel_handler[i+4*(settings_menu.index/4)]);
			
			 #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
			  	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
				{
					register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
			  	}
			  	else if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
			  	{
					register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
			  	}
			  #ifdef CONFIG_PPG_DATA_UPDATE
			  	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
				{
					register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
			  	}
			  #endif
			  	else
			 #endif
			  	{
			  		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
			  	}

			 #ifdef CONFIG_SYNC_SUPPORT
				register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
			 #elif defined(CONFIG_TEMP_SUPPORT)
				register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
			 #elif defined(CONFIG_PPG_SUPPORT)
				register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
			 #elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
			  #ifdef CONFIG_SLEEP_SUPPORT
				register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
			  #elif defined(CONFIG_STEP_SUPPORT)
				register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
			  #endif
			 #else
				register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
			 #endif
			#endif
			}	
		}
		break;
		
	case SETTINGS_MENU_LANGUAGE:
		{
			LCD_Clear(BLACK);
				
			for(i=0;i<settings_menu.count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

				if(i == global_settings.language)
					LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_YES_ADDR);
				else
					LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_NO_ADDR);
				
			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(WHITE);
			
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
										SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y+7,
										settings_menu.name[global_settings.language][i]
										);
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_INFO_BG_X, 
											SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_BG_W, 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_INFO_BG_H, 
											settings_menu.sel_handler[i]
											);
			#endif
			}

			k_timer_stop(&mainmenu_timer);
			k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
		}
		break;
		
	case SETTINGS_MENU_FACTORY_RESET:
		{
			u16_t tmpbuf[128] = {0};

			LCD_Clear(BLACK);
			LCD_SetFontBgColor(BLACK);
			
			switch(g_reset_status)
			{
			case RESET_STATUS_IDLE:
				{
					u16_t str_ready[LANGUAGE_MAX][36] = {
															{0x0052,0x0065,0x0073,0x0065,0x0074,0x0020,0x0074,0x006F,0x0020,0x0074,0x0068,0x0065,0x0020,0x0066,0x0061,0x0063,0x0074,0x006F,0x0072,0x0079,0x0020,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0073,0x0000},//Reset to the factory settings
															{0x0041,0x0075,0x0066,0x0020,0x0057,0x0065,0x0072,0x006B,0x0073,0x0065,0x0069,0x006E,0x0073,0x0074,0x0065,0x006C,0x006C,0x0075,0x006E,0x0067,0x0065,0x006E,0x0020,0x007A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0000},//Auf Werkseinstellungen zurücksetzen
															{0x6062,0x590D,0x5230,0x51FA,0x5382,0x8BBE,0x7F6E,0x0000},//恢复到出厂设置
														};
					
					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_ICON_X, SETTINGS_MENU_RESET_ICON_Y, IMG_RESET_LOGO_ADDR);
					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_NO_X, SETTINGS_MENU_RESET_NO_Y, IMG_RESET_NO_ADDR);
					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_YES_X, SETTINGS_MENU_RESET_YES_Y, IMG_RESET_YES_ADDR);

					if(global_settings.language == LANGUAGE_DE)
					{
						memcpy(tmpbuf, &str_ready[global_settings.language][0], 2*22);
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_STR_X+(SETTINGS_MENU_RESET_STR_W-w)/2, SETTINGS_MENU_RESET_STR_Y, tmpbuf);
						
						memset(tmpbuf, 0x0000, sizeof(tmpbuf));
						memcpy(tmpbuf, &str_ready[global_settings.language][22], 2*(mmi_ucs2strlen(str_ready[global_settings.language])-22));
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_STR_X+(SETTINGS_MENU_RESET_STR_W-w)/2, SETTINGS_MENU_RESET_STR_Y+h+2, tmpbuf);
					}
					else
					{
						LCD_MeasureUniString(str_ready[global_settings.language], &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_STR_X+(SETTINGS_MENU_RESET_STR_W-w)/2, SETTINGS_MENU_RESET_STR_Y+(SETTINGS_MENU_RESET_STR_H-h)/2, str_ready[global_settings.language]);
					}
					
				#ifdef CONFIG_TOUCH_SUPPORT
					register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
												SETTINGS_MENU_RESET_NO_X, 
												SETTINGS_MENU_RESET_NO_X+SETTINGS_MENU_RESET_NO_W, 
												SETTINGS_MENU_RESET_NO_Y, 
												SETTINGS_MENU_RESET_NO_Y+SETTINGS_MENU_RESET_NO_H, 
												settings_menu.sel_handler[0]);
					register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
												SETTINGS_MENU_RESET_YES_X, 
												SETTINGS_MENU_RESET_YES_X+SETTINGS_MENU_RESET_YES_W, 
												SETTINGS_MENU_RESET_YES_Y, 
												SETTINGS_MENU_RESET_YES_Y+SETTINGS_MENU_RESET_YES_H, 
												settings_menu.sel_handler[1]);
				#endif	
				}
				break;
				
			case RESET_STATUS_RUNNING:
				{
					u16_t str_running[LANGUAGE_MAX][22] = {
															{0x0052,0x0065,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0020,0x0069,0x006E,0x0020,0x0070,0x0072,0x006F,0x0067,0x0072,0x0065,0x0073,0x0073,0x0000},//Resetting in progress
															{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0020,0x0069,0x006D,0x0020,0x0047,0x0061,0x006E,0x0067,0x0065,0x0000},//Zurücksetzen im Gange
															{0x91CD,0x7F6E,0x8FDB,0x884C,0x4E2D,0x0000},//重置进行中
														  };
					u32_t img_addr[8] = {
											IMG_RESET_ANI_1_ADDR,
											IMG_RESET_ANI_2_ADDR,
											IMG_RESET_ANI_3_ADDR,
											IMG_RESET_ANI_4_ADDR,
											IMG_RESET_ANI_5_ADDR,
											IMG_RESET_ANI_6_ADDR,
											IMG_RESET_ANI_7_ADDR,
											IMG_RESET_ANI_8_ADDR
										};
					
					LCD_MeasureUniString(str_running[global_settings.language], &w, &h);
					LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, SETTINGS_MENU_RESET_NOTIFY_Y+(SETTINGS_MENU_RESET_NOTIFY_H-h)/2, str_running[global_settings.language]);

				#ifdef CONFIG_ANIMATION_SUPPORT
					AnimaShow(SETTINGS_MENU_RESET_LOGO_X, SETTINGS_MENU_RESET_LOGO_Y, img_addr, ARRAY_SIZE(img_addr), 300, true, NULL);
				#endif	
				}
				break;
				
			case RESET_STATUS_SUCCESS:
				{
					u16_t str_success[LANGUAGE_MAX][33] = {
															{0x0046,0x0061,0x0063,0x0074,0x006F,0x0072,0x0079,0x0020,0x0072,0x0065,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0020,0x0063,0x006F,0x006D,0x0070,0x006C,0x0065,0x0074,0x0065,0x0000},//Factory resetting complete
															{0x0057,0x0065,0x0072,0x006B,0x0073,0x0065,0x0069,0x006E,0x0073,0x0074,0x0065,0x006C,0x006C,0x0075,0x006E,0x0067,0x0065,0x006E,0x0020,0x0061,0x0062,0x0067,0x0065,0x0073,0x0063,0x0068,0x006C,0x006F,0x0073,0x0073,0x0065,0x006E,0x0000},//Werkseinstellungen abgeschlossen
															{0x51FA,0x5382,0x8BBE,0x7F6E,0x6062,0x590D,0x6210,0x529F,0x0000},//出厂设置恢复成功
														  };
					
				#ifdef CONFIG_ANIMATION_SUPPORT
					AnimaStopShow();
				#endif

					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_LOGO_X, SETTINGS_MENU_RESET_LOGO_Y, IMG_RESET_SUCCESS_ADDR);
				
					if(global_settings.language == LANGUAGE_DE)
					{
						memcpy(tmpbuf, &str_success[global_settings.language][0], 2*19);
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, SETTINGS_MENU_RESET_NOTIFY_Y, tmpbuf);
						
						memset(tmpbuf, 0x0000, sizeof(tmpbuf));
						memcpy(tmpbuf, &str_success[global_settings.language][19], 2*(mmi_ucs2strlen(str_success[global_settings.language])-19));
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, SETTINGS_MENU_RESET_NOTIFY_Y+h+2, tmpbuf);
					}
					else
					{
						LCD_MeasureUniString(str_success[global_settings.language], &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, SETTINGS_MENU_RESET_NOTIFY_Y+(SETTINGS_MENU_RESET_NOTIFY_H-h)/2, str_success[global_settings.language]);
					}

					k_timer_stop(&mainmenu_timer);
					k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
				}
				break;
				
			case RESET_STATUS_FAIL:
				{
					u16_t str_fail[LANGUAGE_MAX][51] = {
															{0x0046,0x0061,0x0063,0x0074,0x006F,0x0072,0x0079,0x0020,0x0072,0x0065,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0020,0x0066,0x0061,0x0069,0x006C,0x0065,0x0064,0x0000},//Factory resetting failed
															{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0020,0x0061,0x0075,0x0066,0x0020,0x0057,0x0065,0x0072,0x006B,0x0073,0x0065,0x0069,0x006E,0x0073,0x0074,0x0065,0x006C,0x006C,0x0075,0x006E,0x0067,0x0065,0x006E,0x0020,0x0066,0x0065,0x0068,0x006C,0x0067,0x0065,0x0073,0x0063,0x0068,0x006C,0x0061,0x0067,0x0065,0x006E,0x0000},//Zurücksetzen auf Werkseinstellungen fehlgeschlagen
															{0x51FA,0x5382,0x8BBE,0x7F6E,0x6062,0x590D,0x5931,0x8D25,0x0000},//出厂设置恢复失败
													   };

				#ifdef CONFIG_ANIMATION_SUPPORT
					AnimaStopShow();
				#endif

					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_LOGO_X, SETTINGS_MENU_RESET_LOGO_Y, IMG_RESET_FAIL_ADDR);
				
					if(global_settings.language == LANGUAGE_DE)
					{
						memcpy(tmpbuf, &str_fail[global_settings.language][0], 2*24);
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, SETTINGS_MENU_RESET_NOTIFY_Y, tmpbuf);
						
						memset(tmpbuf, 0x0000, sizeof(tmpbuf));
						memcpy(tmpbuf, &str_fail[global_settings.language][24], 2*(mmi_ucs2strlen(str_fail[global_settings.language])-24));
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, SETTINGS_MENU_RESET_NOTIFY_Y+h+2, tmpbuf);
					}
					else
					{
						LCD_MeasureUniString(str_fail[global_settings.language], &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, SETTINGS_MENU_RESET_NOTIFY_Y+(SETTINGS_MENU_RESET_NOTIFY_H-h)/2, str_fail[global_settings.language]);
					}

					k_timer_stop(&mainmenu_timer);
					k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
				}
				break;
			}
		}
		break;
		
	case SETTINGS_MENU_OTA:
		{
			u16_t tmpbuf[128] = {0};
			u16_t str_notify[LANGUAGE_MAX][27] = {
													{0x0049,0x0074,0x0020,0x0069,0x0073,0x0020,0x0074,0x0068,0x0065,0x0020,0x006C,0x0061,0x0074,0x0065,0x0073,0x0074,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0000},//It is the latest version
													{0x0045,0x0073,0x0020,0x0069,0x0073,0x0074,0x0020,0x0064,0x0049,0x0065,0x0020,0x006E,0x0065,0x0075,0x0065,0x0073,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0000},//Es ist die neueste version
													{0x5DF2,0x662F,0x6700,0x65B0,0x7248,0x672C,0x0000},//已是最新版本
												};
			
			LCD_Clear(BLACK);
			LCD_SetFontBgColor(BLACK);
			
			LCD_MeasureUniString(str_notify[global_settings.language], &w, &h);
			LCD_ShowUniString((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, str_notify[global_settings.language]);

		#ifdef CONFIG_TOUCH_SUPPORT
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
										0, 
										LCD_WIDTH, 
										0, 
										LCD_HEIGHT, 
										settings_menu.sel_handler[0]);
		#endif	

			k_timer_stop(&mainmenu_timer);
			k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
		}
		break;
		
	case SETTINGS_MENU_BRIGHTNESS:
		{
			u32_t img_addr[4] = {IMG_BKL_LEVEL_1_ADDR,IMG_BKL_LEVEL_2_ADDR,IMG_BKL_LEVEL_3_ADDR,IMG_BKL_LEVEL_4_ADDR};

			if(entry_setting_bk_flag == false)
			{
				entry_setting_bk_flag = true;
				LCD_Clear(BLACK);
				LCD_ShowImg_From_Flash(SETTINGS_MENU_BK_DEC_X, SETTINGS_MENU_BK_DEC_Y, IMG_BKL_DEC_ICON_ADDR);
				LCD_ShowImg_From_Flash(SETTINGS_MENU_BK_INC_X, SETTINGS_MENU_BK_INC_Y, IMG_BKL_INC_ICON_ADDR);
			}
			
			LCD_ShowImg_From_Flash(SETTINGS_MENU_BK_LEVEL_X, SETTINGS_MENU_BK_LEVEL_Y, img_addr[global_settings.backlight_level]);

		#ifdef CONFIG_TOUCH_SUPPORT
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
										SETTINGS_MENU_BK_DEC_X, 
										SETTINGS_MENU_BK_DEC_X+SETTINGS_MENU_BK_DEC_W, 
										SETTINGS_MENU_BK_DEC_Y, 
										SETTINGS_MENU_BK_DEC_Y+SETTINGS_MENU_BK_DEC_H, 
										settings_menu.sel_handler[0]
										);
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
										SETTINGS_MENU_BK_INC_X, 
										SETTINGS_MENU_BK_INC_X+SETTINGS_MENU_BK_INC_W, 
										SETTINGS_MENU_BK_INC_Y, 
										SETTINGS_MENU_BK_INC_Y+SETTINGS_MENU_BK_INC_H, 
										settings_menu.sel_handler[1]
										);
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
										0, 
										LCD_WIDTH, 
										0, 
										SETTINGS_MENU_BK_INC_Y-10, 
										settings_menu.sel_handler[2]
										);			
		
			register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[2]);
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[3]);
		#endif

			k_timer_stop(&mainmenu_timer);
			k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
		}
		break;
		
	case SETTINGS_MENU_TEMP:
		{
			LCD_Clear(BLACK);
			
			for(i=0;i<settings_menu.count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

				if(i == global_settings.temp_unit)
					LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_YES_ADDR);
				else
					LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_NO_ADDR);
				
			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(WHITE);
			
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
										SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y+7,
										settings_menu.name[global_settings.language][i]);
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_INFO_BG_X, 
											SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_BG_W, 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y), 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_BG_H, 
											settings_menu.sel_handler[i]);
			#endif
			}

			k_timer_stop(&mainmenu_timer);
			k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
		}
		break;
		
	case SETTINGS_MENU_DEVICE:
		{
			u16_t ble_str[64] = {0};
			u16_t wifi_str[64] = {0};
			u16_t imei_str[IMEI_MAX_LEN+1] = {0};
			u16_t *menu_sle_str[3] = {ble_str,wifi_str,imei_str};
			u16_t menu_color = 0x9CD3;

			LCD_Clear(BLACK);
				
			mmi_asc_to_ucs2((u8_t*)ble_str, g_ble_mac_addr);
			mmi_asc_to_ucs2((u8_t*)wifi_str, "NO");
			mmi_asc_to_ucs2((u8_t*)imei_str, g_imei);
			menu_sle_str[0] = ble_str;
			menu_sle_str[1] = wifi_str;
			menu_sle_str[2] = imei_str;

			if(settings_menu.count > SETTINGS_SUB_MENU_MAX_PER_PG)
				count = (settings_menu.count - settings_menu.index >= SETTINGS_SUB_MENU_MAX_PER_PG) ? SETTINGS_SUB_MENU_MAX_PER_PG : settings_menu.count - settings_menu.index;
			else
				count = settings_menu.count;
			
			for(i=0;i<count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(menu_color);
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
										SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y-5,
										settings_menu.name[global_settings.language][i+settings_menu.index]);

				LCD_SetFontColor(WHITE);
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
										SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y+15,
										menu_sle_str[i+settings_menu.index]);
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_INFO_BG_X, 
											SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_BG_W, 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_INFO_BG_H, 
											settings_menu.sel_handler[i+settings_menu.index]);
			#endif
			}
		}
		break;
		
	case SETTINGS_MENU_SIM:
		{
			u16_t imsi_str[IMSI_MAX_LEN+1] = {0};
			u16_t iccid_str[ICCID_MAX_LEN+1] = {0};
			u16_t *menu_sle_str[2] = {imsi_str,iccid_str};
			u16_t menu_color = 0x9CD3;

			LCD_Clear(BLACK);
			
			mmi_asc_to_ucs2((u8_t*)imsi_str, g_imsi);
			mmi_asc_to_ucs2((u8_t*)iccid_str, g_iccid);
			menu_sle_str[0] = imsi_str;
			menu_sle_str[1] = iccid_str;

			if(settings_menu.count > SETTINGS_SUB_MENU_MAX_PER_PG)
				count = (settings_menu.count - settings_menu.index >= SETTINGS_SUB_MENU_MAX_PER_PG) ? SETTINGS_SUB_MENU_MAX_PER_PG : settings_menu.count - settings_menu.index;
			else
				count = settings_menu.count;
			
			for(i=0;i<count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

				LCD_SetFontSize(FONT_SIZE_20);
				
			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(menu_color);
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
									SETTINGS_MENU_INFO_BG_Y+(i%settings_menu.count)*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y-5,
									settings_menu.name[global_settings.language][i+settings_menu.index]);

				LCD_SetFontColor(WHITE);
				if(i == 1)
					LCD_SetFontSize(FONT_SIZE_16);
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
									SETTINGS_MENU_INFO_BG_Y+(i%settings_menu.count)*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y+15,
									menu_sle_str[i+settings_menu.index]);
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_INFO_BG_X, 
											SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_BG_W, 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_INFO_BG_H, 
											settings_menu.sel_handler[i+settings_menu.index]
											);
			#endif
			}
		}
		break;
		
	case SETTINGS_MENU_FW:
		{
			u16_t mcu_str[20] = {0x0000};
			u16_t wifi_str[20] = {0x0000};
			u16_t ble_str[20] = {0x0000};
			u16_t ppg_str[20] = {0x0000};
			u16_t modem_str[20] = {0x0000};			
			u16_t *menu_sle_str[5] = {mcu_str,wifi_str,ble_str,ppg_str,modem_str};
			u16_t menu_color = 0x9CD3;

			LCD_Clear(BLACK);
			
			mmi_asc_to_ucs2((u8_t*)mcu_str, g_fw_version);
			mmi_asc_to_ucs2((u8_t*)wifi_str, "NO");
			mmi_asc_to_ucs2((u8_t*)ble_str, &g_nrf52810_ver[15]);
		#ifdef CONFIG_PPG_SUPPORT	
			mmi_asc_to_ucs2((u8_t*)ppg_str, g_ppg_ver);
		#else
			mmi_asc_to_ucs2((u8_t*)ppg_str, "NO");
		#endif
			mmi_asc_to_ucs2((u8_t*)modem_str, &g_modem[12]);
			menu_sle_str[0] = mcu_str;
			menu_sle_str[1] = wifi_str;
			menu_sle_str[2] = ble_str;
			menu_sle_str[3] = ppg_str;
			menu_sle_str[4] = modem_str;
			
			if(settings_menu.count > SETTINGS_SUB_MENU_MAX_PER_PG)
				count = (settings_menu.count - settings_menu.index >= SETTINGS_SUB_MENU_MAX_PER_PG) ? SETTINGS_SUB_MENU_MAX_PER_PG : settings_menu.count - settings_menu.index;
			else
				count = settings_menu.count;
			
			for(i=0;i<count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_INFO_BG_X, SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(menu_color);
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
									SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y-5,
									settings_menu.name[global_settings.language][i+settings_menu.index]);
			
				LCD_SetFontColor(WHITE);
				LCD_ShowUniString(SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_STR_OFFSET_X,
									SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_INFO_BG_OFFSET_Y)+SETTINGS_MENU_INFO_STR_OFFSET_Y+15,
									menu_sle_str[i+settings_menu.index]);
			#endif	

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_INFO_BG_X, 
											SETTINGS_MENU_INFO_BG_X+SETTINGS_MENU_INFO_BG_W, 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_INFO_BG_Y+i*(SETTINGS_MENU_INFO_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_INFO_BG_H, 
											settings_menu.sel_handler[i+settings_menu.index]
											);
			#endif
			}
		}
		break;
	}

	if(((settings_menu.id == SETTINGS_MENU_MAIN) && (settings_menu.count > SETTINGS_MAIN_MENU_MAX_PER_PG))
		|| ((settings_menu.id != SETTINGS_MENU_MAIN) && (settings_menu.count > SETTINGS_SUB_MENU_MAX_PER_PG)))
	{
		if(((settings_menu.id == SETTINGS_MENU_MAIN)&&(settings_menu.index == SETTINGS_MAIN_MENU_MAX_PER_PG))
			||((settings_menu.id != SETTINGS_MENU_MAIN)&&(settings_menu.index == SETTINGS_SUB_MENU_MAX_PER_PG))
			)
			LCD_ShowImg_From_Flash(SETTINGS_MEUN_PAGE_DOT_X, SETTINGS_MEUN_PAGE_DOT_Y, IMG_SET_PG_2_ADDR);
		else
			LCD_ShowImg_From_Flash(SETTINGS_MEUN_PAGE_DOT_X, SETTINGS_MEUN_PAGE_DOT_Y, IMG_SET_PG_1_ADDR);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[0]);
		register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[1]);
	#endif		
	}

	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();
}

void SettingsShowStatus(void)
{
	u16_t i,x,y,w,h;
	u16_t menu_sle_str[LANGUAGE_MAX][3][20] = {
													{
														{0x0045,0x006e,0x0067,0x006c,0x0069,0x0073,0x0068,0x0000},//English
														{0x0052,0x0065,0x0073,0x0065,0x0074,0x0000},//Reset
														{0x0056,0x0069,0x0065,0x0077,0x0000},//View
													},
													{
														{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
														{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0000},//Zurücksetzen
														{0x0041,0x006E,0x0073,0x0069,0x0063,0x0068,0x0074,0x0000},//Ansicht
													},
													{
														{0x4E2D,0x6587,0x0000},//中文
														{0x91CD,0x7F6E,0x0000},//重置
														{0x67E5,0x770B,0x0000},//查看
													},
											  };
	u16_t level_1_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000};
	u16_t level_2_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000};
	u16_t level_3_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000};
	u16_t level_4_str[] = {0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000};
	u16_t *level_str[4] = {level_1_str,level_2_str,level_3_str,level_4_str};
	u32_t img_addr[2] = {IMG_SET_TEMP_UNIT_C_ICON_ADDR, IMG_SET_TEMP_UNIT_F_ICON_ADDR};
	u16_t bg_clor = 0x2124;
	u16_t green_clor = 0x07e0;
	
	LCD_Clear(BLACK);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	LCD_SetFontBgColor(bg_clor);

	for(i=0;i<4;i++)
	{
		LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontColor(WHITE);
		LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
								SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
								settings_menu.name[global_settings.language][i]);

		LCD_SetFontColor(green_clor);
		if(i == 3)
		{
			LCD_MeasureUniString(level_str[global_settings.backlight_level], &w, &h);
			LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
									SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
									level_str[global_settings.backlight_level]);
		}
		else
		{
			LCD_MeasureUniString(menu_sle_str[global_settings.language][i], &w, &h);
			LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
									SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y,
									menu_sle_str[global_settings.language][i]);
		}
	#endif	



	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									SETTINGS_MENU_BG_X, 
									SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W, 
									SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
									SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_BG_H, 
									settings_menu.sel_handler[i]);
	#endif
	}

	if(settings_menu.count > 4)
	{
		LCD_ShowImg_From_Flash(SETTINGS_MEUN_PAGE_DOT_X, SETTINGS_MEUN_PAGE_DOT_Y, IMG_SET_PG_1_ADDR);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[0]);
		register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[1]);
	#endif		
	}

	entry_setting_bk_flag = false;
	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();
}

void SettingsScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_SETTINGS].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SETTINGS].status = SCREEN_STATUS_CREATED;
		SettingsShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		SettingsUpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_SETTINGS].act = SCREEN_ACTION_NO;
}

void ExitSettingsScreen(void)
{
	EntryIdleScr();
}

void EnterSettingsScreen(void)
{
	if(screen_id == SCREEN_ID_SETTINGS)
		return;

	k_timer_stop(&mainmenu_timer);
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SETTINGS;	
	scr_msg[SCREEN_ID_SETTINGS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SETTINGS].status = SCREEN_STATUS_CREATING;

#ifndef NB_SIGNAL_TEST
 #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
  	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		SetLeftKeyUpHandler(dl_img_start);
	}
	else if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		SetLeftKeyUpHandler(dl_font_start);
	}
  #ifdef CONFIG_PPG_DATA_UPDATE
	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
	{
		SetLeftKeyUpHandler(dl_ppg_start);
	}
  #endif
  	else
 #endif
#endif
  	{
  		SetLeftKeyUpHandler(EnterPoweroffScreen);
  	}

	SetRightKeyUpHandler(ExitSettingsScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

 #ifndef NB_SIGNAL_TEST
  #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
	}
	else if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
	}
   #ifdef CONFIG_PPG_DATA_UPDATE
	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
	}
   #endif
	else
  #endif
 #endif 
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 	}

 #ifndef NB_SIGNAL_TEST
  #ifdef CONFIG_SYNC_SUPPORT
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
  #elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
   #ifdef CONFIG_SLEEP_SUPPORT
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
   #elif defined(CONFIG_STEP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
   #endif
  #elif defined(CONFIG_PPG_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
  #endif
 #else
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterGPSTestScreen);
 #endif
#endif
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

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SYNC;
	scr_msg[SCREEN_ID_SYNC].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SYNC].status = SCREEN_STATUS_CREATING;

	SetLeftKeyUpHandler(EnterSettings);
	SetRightKeyUpHandler(ExitSyncDataScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
	
  #if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))	
   #ifdef CONFIG_SLEEP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
   #elif defined(CONFIG_STEP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
   #endif
  #elif defined(CONFIG_PPG_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
  #endif
#endif
}

void SyncUpdateStatus(void)
{
	u32_t img_anima[3] = {IMG_RUNNING_ANI_1_ADDR, IMG_RUNNING_ANI_2_ADDR, IMG_RUNNING_ANI_3_ADDR};
	
	switch(sync_state)
	{
	case SYNC_STATUS_IDLE:
		break;

	case SYNC_STATUS_LINKING:
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaShow(SYNC_RUNNING_ANI_X, SYNC_RUNNING_ANI_Y, img_anima, ARRAY_SIZE(img_anima), 1000, true, NULL);
	#endif
		break;
		
	case SYNC_STATUS_SENT:
	#ifdef CONFIG_ANIMATION_SUPPORT 
		AnimaStopShow();
	#endif
	
		LCD_ShowImg_From_Flash(SYNC_FINISH_ICON_X, SYNC_FINISH_ICON_Y, IMG_SYNC_FINISH_ADDR);
		LCD_ShowImg_From_Flash(SYNC_RUNNING_ANI_X, SYNC_RUNNING_ANI_Y, IMG_RUNNING_ANI_1_ADDR);

		SetLeftKeyUpHandler(ExitSyncDataScreen);
		break;
		
	case SYNC_STATUS_FAIL:
	#ifdef CONFIG_ANIMATION_SUPPORT 
		AnimaStopShow();
	#endif
	
		LCD_ShowImg_From_Flash(SYNC_FINISH_ICON_X, SYNC_FINISH_ICON_Y, IMG_SYNC_ERR_ADDR);
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
	u16_t x,y,w,h;
	u8_t tmpbuf[64] = {0};

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else
	LCD_SetFontSize(FONT_SIZE_32);
#endif

	if(global_settings.temp_unit == TEMP_UINT_C)
		sprintf(tmpbuf, "%0.1f", g_temp_body);
	else
		sprintf(tmpbuf, "%0.1f", g_temp_body*1.8+32);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = TEMP_NUM_X+(TEMP_NUM_W-w)/2;
	y = TEMP_NUM_Y+(TEMP_NUM_H-h)/2;
	LCD_Fill(TEMP_NUM_X, TEMP_NUM_Y, TEMP_NUM_W, TEMP_NUM_H, BLACK);
	LCD_ShowString(x,y,tmpbuf);

	if(get_temp_ok_flag)
	{
		k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);

	#if 0	
		notify_infor infor = {0};

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_68);
	#else		
		LCD_SetFontSize(FONT_SIZE_64);
	#endif

		infor.w = 180;
		infor.h = 80;
		infor.x = (LCD_WIDTH-infor.w)/2;
		infor.y = (LCD_HEIGHT-infor.h)/2;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		mmi_asc_to_ucs2(infor.text, tmpbuf);
		infor.img_count = 0;

		DisplayPopUp(infor);
	#endif	
	}
}

void TempShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t i,tmpbuf[64] = {0};
	float temp_max = 0.0, temp_min = 0.0;
	u16_t temp[24] = {0};
	u16_t color = 0x05DF;

	LCD_Clear(BLACK);

	if(global_settings.temp_unit == TEMP_UINT_C)
	{		
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_ICON_C_ADDR);
		LCD_ShowImg_From_Flash(TEMP_BG_X, TEMP_BG_Y, IMG_TEMP_C_BG_ADDR);
		LCD_ShowImg_From_Flash(TEMP_UINT_X, TEMP_UINT_Y, IMG_TEMP_UNIT_C_ADDR);
	}
	else
	{
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_ICON_F_ADDR);
		LCD_ShowImg_From_Flash(TEMP_BG_X, TEMP_BG_Y, IMG_TEMP_F_BG_ADDR);
		LCD_ShowImg_From_Flash(TEMP_UINT_X, TEMP_UINT_Y, IMG_TEMP_UNIT_F_ADDR);
	}
	LCD_ShowImg_From_Flash(TEMP_UP_ARRAW_X, TEMP_UP_ARRAW_Y, IMG_TEMP_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(TEMP_DOWN_ARRAW_X, TEMP_DOWN_ARRAW_Y, IMG_TEMP_DOWN_ARRAW_ADDR);

	GetCurDayTempRecData(temp);
	for(i=0;i<24;i++)
	{
		if((temp[i] >= TEMP_MIN) && (temp[i] <= TEMP_MAX))
		{
			if((temp_max == 0.0) && (temp_min == 0.0))
			{
				temp_max = (float)temp[i]/10.0;
				temp_min = (float)temp[i]/10.0;
			}
			else
			{
				if(temp[i]/10.0 > temp_max)
					temp_max = (float)temp[i]/10.0;
				if(temp[i]/10.0 < temp_min)
					temp_min = (float)temp[i]/10.0;
			}

			LCD_Fill(TEMP_REC_DATA_X+TEMP_REC_DATA_OFFSET_X*i, TEMP_REC_DATA_Y-(temp[i]/10.0-32.0)*15/2, TEMP_REC_DATA_W, (temp[i]/10.0-32.0)*15/2, color);
		}
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else		
	LCD_SetFontSize(FONT_SIZE_32);
#endif

	if(global_settings.temp_unit == TEMP_UINT_C)
		sprintf(tmpbuf, "%0.1f", 0);
	else
		sprintf(tmpbuf, "%0.1f", 0);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = TEMP_NUM_X+(TEMP_NUM_W-w)/2;
	y = TEMP_NUM_Y+(TEMP_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);
#else		
	LCD_SetFontSize(FONT_SIZE_24);
#endif

	if(global_settings.temp_unit == TEMP_UINT_C)
		sprintf(tmpbuf, "%0.1f", temp_max);
	else
		sprintf(tmpbuf, "%0.1f", temp_max*1.8+32);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = TEMP_UP_NUM_X+(TEMP_UP_NUM_W-w)/2;
	y = TEMP_UP_NUM_Y+(TEMP_UP_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);

	if(global_settings.temp_unit == TEMP_UINT_C)
		sprintf(tmpbuf, "%0.1f", temp_min);
	else
		sprintf(tmpbuf, "%0.1f", temp_min*1.8+32);
	LCD_MeasureString(tmpbuf,&w,&h);
	x = TEMP_DOWN_NUM_X+(TEMP_DOWN_NUM_W-w)/2;
	y = TEMP_DOWN_NUM_Y+(TEMP_DOWN_NUM_H-h)/2;
	LCD_ShowString(x,y,tmpbuf);
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
	if(!TempIsWorkingTiming())
		MenuStopTemp();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterTempScreen(void)
{
	if(screen_id == SCREEN_ID_TEMP)
		return;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_TEMP;
	scr_msg[SCREEN_ID_TEMP].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_TEMP].status = SCREEN_STATUS_CREATING;

	get_temp_ok_flag = false;

#ifdef CONFIG_PPG_SUPPORT
	SetLeftKeyUpHandler(EnterSPO2Screen);
#elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
 #ifdef CONFIG_STEP_SUPPORT
	SetLeftKeyUpHandler(EnterStepsScreen);
 #elif defined(CONFIG_SLEEP_SUPPORT)
	SetLeftKeyUpHandler(EnterSleepScreen);
 #endif
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
	SetRightKeyUpHandler(ExitTempScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

 #ifdef CONFIG_PPG_SUPPORT
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSPO2Screen);
 #elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
  #ifdef CONFIG_STEP_SUPPORT
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
  #elif defined(CONFIG_SLEEP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
  #endif
 #elif defined(CONFIG_SYNC_SUPPORT)
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
 #else
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
 #endif
 
 #ifdef CONFIG_PPG_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
 #else
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
 #endif
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
	LCD_SetFontSize(FONT_SIZE_28);
#else		
	LCD_SetFontSize(FONT_SIZE_24);
#endif
	sprintf(tmpbuf, "%d/%d", g_bpt.systolic, g_bpt.diastolic);
	LCD_MeasureString(tmpbuf, &w, &h);
	x = BP_NUM_X+(BP_NUM_W-w)/2;
	y = BP_NUM_Y+(BP_NUM_H-h)/2;
	LCD_Fill(BP_NUM_X, BP_NUM_Y, BP_NUM_W, BP_NUM_H, BLACK);
	LCD_ShowString(x,y,tmpbuf);

	if(get_bpt_ok_flag)
	{
		k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);

	#if 0
		notify_infor infor = {0};

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_52);
	#else		
		LCD_SetFontSize(FONT_SIZE_48);
	#endif
		
		infor.w = 180;
		infor.h = 80;
		infor.x = (LCD_WIDTH-infor.w)/2;
		infor.y = (LCD_HEIGHT-infor.h)/2;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		mmi_asc_to_ucs2(infor.text, tmpbuf);
		infor.img_count = 0;

		DisplayPopUp(infor);
	#endif	
	}
}

void BPShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t i,tmpbuf[64] = {0};
	bpt_data bpt_max={0},bpt_min={0},bpt[24] = {0};

	LCD_Clear(BLACK);
	
	LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, IMG_BP_ICON_ANI_2_ADDR);
	LCD_ShowImg_From_Flash(BP_BG_X, BP_BG_Y, IMG_BP_BG_ADDR);
	LCD_ShowImg_From_Flash(BP_UNIT_X, BP_UNIT_Y, IMG_BP_UNIT_ADDR);
	LCD_ShowImg_From_Flash(BP_UP_ARRAW_X, BP_UP_ARRAW_Y, IMG_BP_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(BP_DOWN_ARRAW_X, BP_DOWN_ARRAW_Y, IMG_BP_DOWN_ARRAW_ADDR);

	GetCurDayBptRecData(bpt);
	for(i=0;i<24;i++)
	{
		if((bpt[i].systolic >= PPG_BPT_SYS_MIN) && (bpt[i].systolic <= PPG_BPT_SYS_MAX)
			&& (bpt[i].diastolic >= PPG_BPT_DIA_MIN) && (bpt[i].diastolic >= PPG_BPT_DIA_MIN)
			)
		{
			if((bpt_max.systolic == 0) && (bpt_max.diastolic == 0)
				&& (bpt_min.systolic == 0) && (bpt_min.diastolic == 0))
			{
				memcpy(&bpt_max, &bpt[i], sizeof(bpt_data));
				memcpy(&bpt_min, &bpt[i], sizeof(bpt_data));
			}
			else
			{	
				if(bpt[i].systolic > bpt_max.systolic)
					memcpy(&bpt_max, &bpt[i], sizeof(bpt_data));
				if(bpt[i].systolic < bpt_min.systolic)
					memcpy(&bpt_min, &bpt[i], sizeof(bpt_data));
			}

			LCD_Fill(BP_REC_DATA_X+BP_REC_DATA_OFFSET_X*i, BP_REC_DATA_Y-(bpt[i].systolic-30)*15/30, BP_REC_DATA_W, (bpt[i].systolic-30)*15/30, YELLOW);
			LCD_Fill(BP_REC_DATA_X+BP_REC_DATA_OFFSET_X*i, BP_REC_DATA_Y-(bpt[i].diastolic-30)*15/30, BP_REC_DATA_W, (bpt[i].diastolic-30)*15/30, RED);
		}		
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);
#else		
	LCD_SetFontSize(FONT_SIZE_24);
#endif
	sprintf(tmpbuf, "%d/%d", 0, 0);
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

	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterBPScreen(void)
{
	if(screen_id == SCREEN_ID_BP)
		return;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_BP;	
	scr_msg[SCREEN_ID_BP].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_BP].status = SCREEN_STATUS_CREATING;

	get_bpt_ok_flag = false;
	img_index = 0;

#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
  #ifdef CONFIG_STEP_SUPPORT
	SetLeftKeyUpHandler(EnterStepsScreen);
  #elif defined(CONFIG_SLEEP_SUPPORT)
	SetLeftKeyUpHandler(EnterSleepScreen);
  #endif
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
	SetRightKeyUpHandler(ExitBPScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

 #if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
  #ifdef CONFIG_STEP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
  #elif defined(CONFIG_SLEEP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
  #endif
 #elif defined(CONFIG_SYNC_SUPPORT)
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
 #else
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings); 
 #endif
 
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSPO2Screen);
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

	if(get_spo2_ok_flag)
	{
		k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);

	#if 0	
		notify_infor infor = {0};

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_68);
	#else		
		LCD_SetFontSize(FONT_SIZE_64);
	#endif
		
		infor.w = 180;
		infor.h = 80;
		infor.x = (LCD_WIDTH-infor.w)/2;
		infor.y = (LCD_HEIGHT-infor.h)/2;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		mmi_asc_to_ucs2(infor.text, tmpbuf);
		infor.img_count = 0;

		DisplayPopUp(infor);
	#endif	
	}
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
		if((spo2[i] >= PPG_SPO2_MIN) && (spo2[i] <= PPG_SPO2_MAX))
		{
			if((spo2_max == 0) && (spo2_min == 0))
			{
				spo2_max = spo2[i];
				spo2_min = spo2[i];
			}
			else
			{
				if(spo2[i] > spo2_max)
					spo2_max = spo2[i];
				if(spo2[i] < spo2_min)
					spo2_min = spo2[i];
			}
			
			LCD_Fill(SPO2_REC_DATA_X+SPO2_REC_DATA_OFFSET_X*i, SPO2_REC_DATA_Y-(spo2[i]-80)*3, SPO2_REC_DATA_W, (spo2[i]-80)*3, BLUE);
		}
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else
	LCD_SetFontSize(FONT_SIZE_32);
#endif
	sprintf(tmpbuf, "%d%%", 0);
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

	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterSPO2Screen(void)
{
	if(screen_id == SCREEN_ID_SPO2)
		return;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(IsInTempScreen()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
	
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SPO2;	
	scr_msg[SCREEN_ID_SPO2].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SPO2].status = SCREEN_STATUS_CREATING;

	get_spo2_ok_flag = false;
	img_index = 0;
	
	SetLeftKeyUpHandler(EnterBPScreen);
	SetRightKeyUpHandler(ExitSPO2Screen);
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
  #ifdef CONFIG_TEMP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
  #endif
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

	if(get_hr_ok_flag)
	{
		k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);

	#if 0	
		notify_infor infor = {0};

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_68);
	#else		
		LCD_SetFontSize(FONT_SIZE_64);
	#endif
		
		infor.w = 120;
		infor.h = 80;
		infor.x = (LCD_WIDTH-infor.w)/2;
		infor.y = (LCD_HEIGHT-infor.h)/2;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		mmi_asc_to_ucs2(infor.text, tmpbuf);
		infor.img_count = 0;

		DisplayPopUp(infor);
	#endif	
	}
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
		if((hr[i] >= PPG_HR_MIN) && (hr[i] <= PPG_HR_MAX))
		{
			if((hr_max == 0) && (hr_min == 0))
			{
				hr_max = hr[i];
				hr_min = hr[i];
			}
			else
			{
				if(hr[i] > hr_max)
					hr_max = hr[i];
				if(hr[i] < hr_min)
					hr_min = hr[i];
			}

			LCD_Fill(HR_REC_DATA_X+HR_REC_DATA_OFFSET_X*i, HR_REC_DATA_Y-hr[i]*20/50, HR_REC_DATA_W, hr[i]*20/50, RED);
		}
	}

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
#else	
	LCD_SetFontSize(FONT_SIZE_32);
#endif
	sprintf(tmpbuf, "%d", 0);
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

	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterHRScreen(void)
{
	if(screen_id == SCREEN_ID_HR)
		return;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(IsInTempScreen()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
	
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_HR;	
	scr_msg[SCREEN_ID_HR].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_HR].status = SCREEN_STATUS_CREATING;

	get_hr_ok_flag = false;
	img_index = 0;

#ifdef CONFIG_TEMP_SUPPORT
	SetLeftKeyUpHandler(EnterTempScreen);
#else
	SetLeftKeyUpHandler(EnterSPO2Screen);
#endif
	SetRightKeyUpHandler(ExitHRScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
  #ifdef CONFIG_TEMP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #else
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSPO2Screen);
  #endif
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
#endif	
}
#endif/*CONFIG_PPG_SUPPORT*/

static u16_t str_x,str_y,str_w,str_h;
void EnterNotifyScreen(void)
{
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_NOTIFY;	
	scr_msg[SCREEN_ID_NOTIFY].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_NOTIFY].status = SCREEN_STATUS_CREATING;

	SetLeftKeyUpHandler(ExitNotify);
	SetRightKeyUpHandler(ExitNotify);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitNotify);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitNotify);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitNotify);
#endif	
}

void DisplayPopUp(notify_infor infor)
{
	u32_t len;

	k_timer_stop(&notify_timer);
	k_timer_stop(&mainmenu_timer);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		PPGStopCheck();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(IsInTempScreen()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_SYNC_SUPPORT
	if(SyncIsRunning())
		SyncDataStop();
#endif

	memcpy(&notify_msg, &infor, sizeof(notify_infor));

	len = mmi_ucs2strlen((u8_t*)notify_msg.text);
	if(len > NOTIFY_TEXT_MAX_LEN)
		len = NOTIFY_TEXT_MAX_LEN;
	
	if(notify_msg.type == NOTIFY_TYPE_POPUP)
	{
		k_timer_start(&notify_timer, K_SECONDS(NOTIFY_TIMER_INTERVAL), NULL);
	}
	
	EnterNotifyScreen();
}

void ExitNotify(void)
{
	if(screen_id == SCREEN_ID_NOTIFY)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_EXIT;
	}
}

void ExitNotifyScreen(void)
{
	if(screen_id == SCREEN_ID_NOTIFY)
	{
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaStop();
	#endif
		k_timer_stop(&notify_timer);
		EnterIdleScreen();
	}
}

void NotifyTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_NOTIFY)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_EXIT;
	}
}

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

void NotifyShowStrings(u16_t rect_x, u16_t rect_y, u16_t rect_w, u16_t rect_h, u32_t *anima_img, u8_t anima_count, u8_t *strbuf)
{
	LCD_DrawRectangle(rect_x, rect_y, rect_w, rect_h);
	LCD_Fill(rect_x+1, rect_y+1, rect_w-2, rect_h-2, BLACK);
	ShowStringsInRect(rect_x, rect_y, rect_w, rect_h, strbuf);	
}

void NotifyUpdate(void)
{
	u16_t x,y,w=0,h=0;
	u16_t offset_w=4,offset_h=4;

	switch(notify_msg.align)
	{
	case NOTIFY_ALIGN_CENTER:
		if(scr_msg[SCREEN_ID_NOTIFY].para&SCREEN_EVENT_UPDATE_POP_IMG)
		{
			scr_msg[SCREEN_ID_NOTIFY].para &= (~SCREEN_EVENT_UPDATE_POP_IMG);

		#ifdef CONFIG_ANIMATION_SUPPORT
			AnimaStopShow();
		#endif
			LCD_Fill(notify_msg.x+1, notify_msg.y+1, notify_msg.w-2, (notify_msg.h*2)/3-2, BLACK);
			
			if(notify_msg.img != NULL && notify_msg.img_count > 0)
			{	
				LCD_get_pic_size_from_flash(notify_msg.img[0], &w, &h);
			#ifdef CONFIG_ANIMATION_SUPPORT
				AnimaShow(notify_msg.x+(notify_msg.w-w)/2, notify_msg.y+(notify_msg.h-h)/2, notify_msg.img, notify_msg.img_count, 500, true, NULL);
			#else
				LCD_ShowImg_From_Flash(notify_msg.x+(notify_msg.w-w)/2, notify_msg.y+(notify_msg.h-h)/2, notify_msg.img[0]);
			#endif
			}

			str_y = notify_msg.y+(notify_msg.h*2/3);
			str_h = notify_msg.h*1/3;
		}
		
		if(scr_msg[SCREEN_ID_NOTIFY].para&SCREEN_EVENT_UPDATE_POP_STR)
		{
			scr_msg[SCREEN_ID_NOTIFY].para &= (~SCREEN_EVENT_UPDATE_POP_STR);
			
			LCD_Fill(notify_msg.x+1, (notify_msg.h*2)/3+1, notify_msg.w-2, (notify_msg.h*1)/3-2, BLACK);
			LCD_MeasureUniString(notify_msg.text, &w, &h);
			if(w > (str_w-2*offset_w))
			{
				u8_t line_count,line_no,line_max;
				u16_t line_h=(h+offset_h);
				u16_t byte_no=0,text_len;

				line_max = (str_h-2*offset_h)/line_h;
				line_count = w/(str_w-2*offset_w) + ((w%(str_w-offset_w) != 0)? 1 : 0);
				if(line_count > line_max)
					line_count = line_max;

				line_no = 0;
				text_len = mmi_ucs2strlen(notify_msg.text);
				y = ((str_h-2*offset_h)-line_count*line_h)/2;
				y += (str_y+offset_h);
				while(line_no < line_count)
				{
					u16_t tmpbuf[128] = {0};
					u8_t i=0;

					tmpbuf[i++] = notify_msg.text[byte_no++];
					LCD_MeasureUniString(tmpbuf, &w, &h);
					while(w < (str_w-2*offset_w))
					{
						if(byte_no < text_len)
						{
							tmpbuf[i++] = notify_msg.text[byte_no++];
							LCD_MeasureUniString(tmpbuf, &w, &h);
						}
						else
							break;
					}

					if(byte_no < text_len)
					{
						i--;
						byte_no--;
						tmpbuf[i] = 0x00;

						LCD_MeasureUniString(tmpbuf, &w, &h);
						x = ((str_w-2*offset_w)-w)/2;
						x += (str_x+offset_w);
						LCD_ShowUniString(x,y,tmpbuf);

						y += line_h;
						line_no++;
					}
					else
					{
						LCD_MeasureUniString(tmpbuf, &w, &h);
						x = ((str_w-2*offset_w)-w)/2;
						x += (str_x+offset_w);
						LCD_ShowUniString(x,y,tmpbuf);
						break;
					}
				}
			}
			else if(w > 0)
			{
				x = ((str_w-2*offset_w)-w)/2;
				y = (h > (str_h-2*offset_h))? 0 : ((str_h-2*offset_h)-h)/2;
				x += (str_x+offset_w);
				y += (str_y+offset_h);
				LCD_ShowUniString(x,y,notify_msg.text);				
			}
		}
		break;
		
	case NOTIFY_ALIGN_BOUNDARY:
		x = (notify_msg.x+offset_w);
		y = (notify_msg.y+offset_h);
		LCD_ShowUniStringInRect(x, y, (notify_msg.w-2*offset_w), (notify_msg.h-2*offset_h), notify_msg.text);
		break;
	}
}

void NotifyShow(void)
{
	u16_t x,y,w=0,h=0;
	u16_t offset_w=4,offset_h=4;

	if(((notify_msg.img == NULL) || (notify_msg.img_count == 0)) 
		&& ((notify_msg.x != 0)&&(notify_msg.y != 0)&&((notify_msg.w != LCD_WIDTH))&&(notify_msg.h != LCD_HEIGHT)))
	{
		LCD_DrawRectangle(notify_msg.x, notify_msg.y, notify_msg.w, notify_msg.h);
	}

	LCD_Fill(notify_msg.x+1, notify_msg.y+1, notify_msg.w-2, notify_msg.h-2, BLACK);

	switch(notify_msg.align)
	{
	case NOTIFY_ALIGN_CENTER:
		str_x = notify_msg.x;
		str_y = notify_msg.y;
		str_w = notify_msg.w;
		str_h = notify_msg.h;
			
		if(notify_msg.img != NULL && notify_msg.img_count > 0)
		{	
			LCD_get_pic_size_from_flash(notify_msg.img[0], &w, &h);
		#ifdef CONFIG_ANIMATION_SUPPORT
			AnimaShow(notify_msg.x+(notify_msg.w-w)/2, notify_msg.y+(notify_msg.h-h)/2, notify_msg.img, notify_msg.img_count, 500, true, NULL);
		#else
			LCD_ShowImg_From_Flash(notify_msg.x+(notify_msg.w-w)/2, notify_msg.y+(notify_msg.h-h)/2, notify_msg.img[0]);
		#endif

			str_y = notify_msg.y+(notify_msg.h*2/3);
			str_h = notify_msg.h*1/3;
		}

		LCD_MeasureUniString(notify_msg.text, &w, &h);
		if(w > (str_w-2*offset_w))
		{
			u8_t line_count,line_no,line_max;
			u16_t line_h=(h+offset_h);
			u16_t byte_no=0,text_len;

			line_max = (str_h-2*offset_h)/line_h;
			line_count = w/(str_w-2*offset_w) + ((w%(str_w-offset_w) != 0)? 1 : 0);
			if(line_count > line_max)
				line_count = line_max;

			line_no = 0;
			text_len = mmi_ucs2strlen(notify_msg.text);
			y = ((str_h-2*offset_h)-line_count*line_h)/2;
			y += (str_y+offset_h);
			while(line_no < line_count)
			{
				u16_t tmpbuf[128] = {0};
				u8_t i=0;

				tmpbuf[i++] = notify_msg.text[byte_no++];
				LCD_MeasureUniString(tmpbuf, &w, &h);
				while(w < (str_w-2*offset_w))
				{
					if(byte_no < text_len)
					{
						tmpbuf[i++] = notify_msg.text[byte_no++];
						LCD_MeasureUniString(tmpbuf, &w, &h);
					}
					else
						break;
				}

				if(byte_no < text_len)
				{
					i--;
					byte_no--;
					tmpbuf[i] = 0x00;

					LCD_MeasureUniString(tmpbuf, &w, &h);
					x = ((str_w-2*offset_w)-w)/2;
					x += (str_x+offset_w);
					LCD_ShowUniString(x,y,tmpbuf);

					y += line_h;
					line_no++;
				}
				else
				{
					LCD_MeasureUniString(tmpbuf, &w, &h);
					x = ((str_w-2*offset_w)-w)/2;
					x += (str_x+offset_w);
					LCD_ShowUniString(x,y,tmpbuf);
					break;
				}
			}
		}
		else if(w > 0)
		{
			x = ((str_w-2*offset_w)-w)/2;
			y = (h > (str_h-2*offset_h))? 0 : ((str_h-2*offset_h)-h)/2;
			x += (str_x+offset_w);
			y += (str_y+offset_h);
			LCD_ShowUniString(x,y,notify_msg.text);				
		}
		break;
		
	case NOTIFY_ALIGN_BOUNDARY:
		x = (notify_msg.x+offset_w);
		y = (notify_msg.y+offset_h);
		LCD_ShowUniStringInRect(x, y, (notify_msg.w-2*offset_w), (notify_msg.h-2*offset_h), notify_msg.text);
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
		NotifyUpdate();
		break;

	case SCREEN_ACTION_EXIT:
		ExitNotifyScreen();
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

	SetRightKeyUpHandler(dl_start);
	SetLeftKeyUpHandler(dl_exit);
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_YES_X, DL_NOTIFY_YES_X+DL_NOTIFY_YES_W, DL_NOTIFY_YES_Y, DL_NOTIFY_YES_Y+DL_NOTIFY_YES_H, dl_start);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_NO_X, DL_NOTIFY_NO_X+DL_NOTIFY_NO_W, DL_NOTIFY_NO_Y, DL_NOTIFY_NO_Y+DL_NOTIFY_NO_H, dl_exit);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_exit);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_prev);
#endif

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);
}

void DlUpdateStatus(void)
{
	u16_t pro_len;
	u16_t x,y,w,h;
	u8_t pro_buf[16] = {0};
	u8_t strbuf[256] = {0};
	static bool flag = false;
	static u16_t pro_str_x,pro_str_y;

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

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
			memset(strbuf, 0x00, sizeof(strbuf));
			mmi_asc_to_ucs2(strbuf, pro_buf);
			LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
			pro_str_x = DL_NOTIFY_PRO_NUM_X+(DL_NOTIFY_PRO_NUM_W-w)/2;
			pro_str_y = DL_NOTIFY_PRO_NUM_Y+(DL_NOTIFY_PRO_NUM_H-h)/2;
			LCD_ShowUniString(pro_str_x,pro_str_y, (uint16_t*)strbuf);
		}
		else
		{
			pro_len = (g_dl_progress*DL_NOTIFY_PRO_W)/100;
			LCD_Fill(DL_NOTIFY_PRO_X+1, DL_NOTIFY_PRO_Y+1, pro_len, DL_NOTIFY_PRO_H-1, WHITE);

			sprintf(pro_buf, "%3d%%", g_dl_progress);
			memset(strbuf, 0x00, sizeof(strbuf));
			mmi_asc_to_ucs2(strbuf, pro_buf);
			LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
			pro_str_x = DL_NOTIFY_PRO_NUM_X+(DL_NOTIFY_PRO_NUM_W-w)/2;
			pro_str_y = DL_NOTIFY_PRO_NUM_Y+(DL_NOTIFY_PRO_NUM_H-h)/2;
			LCD_ShowUniString(pro_str_x,pro_str_y, (uint16_t*)strbuf);
		}

		ClearAllKeyHandler();
		break;
		
	case DL_STATUS_FINISHED:
		flag = false;
		
		LCD_Fill(DL_NOTIFY_RECT_X+1, DL_NOTIFY_STRING_Y, DL_NOTIFY_RECT_W-1, DL_NOTIFY_RECT_H-(DL_NOTIFY_STRING_Y-DL_NOTIFY_RECT_Y)-1, BLACK);
		switch(g_dl_data_type)
		{
		case DL_DATA_IMG:
			strcpy(strbuf, "Img upgraded successfully!");
			break;
		case DL_DATA_FONT:
			strcpy(strbuf, "Font upgraded successfully!");
			break;
		case DL_DATA_PPG:
			strcpy(strbuf, "PPG Algo upgraded successfully!");
			break;
		}
		ShowStringsInRect(DL_NOTIFY_STRING_X,
						  DL_NOTIFY_STRING_Y,
						  DL_NOTIFY_STRING_W,
						  DL_NOTIFY_STRING_H,
						  strbuf);

		LCD_DrawRectangle(DL_NOTIFY_YES_X, DL_NOTIFY_YES_Y, DL_NOTIFY_YES_W, DL_NOTIFY_YES_H);
		mmi_asc_to_ucs2(strbuf, "SOS(Y)");
		LCD_MeasureUniString(strbuf, &w, &h);
		x = DL_NOTIFY_YES_X+(DL_NOTIFY_YES_W-w)/2;
		y = DL_NOTIFY_YES_Y+(DL_NOTIFY_YES_H-h)/2;	
		LCD_ShowUniString(x,y,strbuf);

		LCD_DrawRectangle(DL_NOTIFY_NO_X, DL_NOTIFY_NO_Y, DL_NOTIFY_NO_W, DL_NOTIFY_NO_H);
		mmi_asc_to_ucs2(strbuf, "PWR(N)");
		LCD_MeasureUniString(strbuf, &w, &h);
		x = DL_NOTIFY_NO_X+(DL_NOTIFY_NO_W-w)/2;
		y = DL_NOTIFY_NO_Y+(DL_NOTIFY_NO_H-h)/2;	
		LCD_ShowUniString(x,y,strbuf);

		SetRightKeyUpHandler(dl_reboot_confirm);
		SetLeftKeyUpHandler(dl_exit);
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_YES_X-10, DL_NOTIFY_YES_X+DL_NOTIFY_YES_W+10, DL_NOTIFY_YES_Y-10, DL_NOTIFY_YES_Y+DL_NOTIFY_YES_H+10, dl_reboot_confirm);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_NO_X-10, DL_NOTIFY_NO_X+DL_NOTIFY_NO_W+10, DL_NOTIFY_NO_Y-10, DL_NOTIFY_NO_Y+DL_NOTIFY_NO_H+10, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_prev);
	#endif	

		k_timer_stop(&mainmenu_timer);
		k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);
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
		mmi_asc_to_ucs2(strbuf, "SOS(Y)");
		LCD_MeasureUniString(strbuf, &w, &h);
		x = (LCD_WIDTH-DL_NOTIFY_YES_W)/2+(DL_NOTIFY_YES_W-w)/2;
		y = DL_NOTIFY_YES_Y+(DL_NOTIFY_YES_H-h)/2;	
		LCD_ShowUniString(x,y,strbuf);

		SetLeftKeyUpHandler(dl_exit);
		SetRightKeyUpHandler(dl_exit);
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, (LCD_WIDTH-DL_NOTIFY_YES_W)/2-10, (LCD_WIDTH-DL_NOTIFY_YES_W)/2+DL_NOTIFY_YES_W+10, DL_NOTIFY_YES_Y-10, DL_NOTIFY_YES_Y+DL_NOTIFY_YES_H+10, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_prev);	
	#endif	

		k_timer_stop(&mainmenu_timer);
		k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);
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
void PrevDlImgScreen(void)
{
	EnterSettings();
}

void ExitDlImgScreen(void)
{
	if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0))
		dl_font_start();
#if defined(CONFIG_PPG_SUPPORT)
	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
		dl_ppg_start();
#endif
	else
		EnterPoweroffScreen();
}
#endif

#ifdef CONFIG_FONT_DATA_UPDATE
void PrevDlFontScreen(void)
{
	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0))
		dl_img_start();
	else
		EnterSettings();
}

void ExitDlFontScreen(void)
{
#if defined(CONFIG_PPG_SUPPORT)
	if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
		dl_ppg_start();
	else
#endif
	EnterPoweroffScreen();
}
#endif

#ifdef CONFIG_PPG_DATA_UPDATE
void PrevDlPpgScreen(void)
{
	if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0))
		dl_font_start();
	else if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0))
		dl_img_start();
	else
		EnterSettings();
}

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
	u16_t x,y,w,h;
	u16_t tmpbuf[128] = {0};
	u16_t str_notify[LANGUAGE_MAX][40] = {
											{0x0043,0x006F,0x006E,0x0074,0x0069,0x006E,0x0075,0x0065,0x0020,0x0074,0x006F,0x0020,0x0075,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0074,0x0068,0x0065,0x0020,0x0066,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x003F,0x0000},//Continue to upgrade the firmware?
											{0x0041,0x006B,0x0074,0x0075,0x0061,0x006C,0x0069,0x0073,0x0069,0x0065,0x0072,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0064,0x0069,0x0065,0x0020,0x0046,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x0020,0x0077,0x0065,0x0069,0x0074,0x0065,0x0072,0x003F,0x0000},//Aktualisieren Sie die Firmware weiter?
											{0x7EE7,0x7EED,0x5347,0x7EA7,0x56FA,0x4EF6,0xFF1F,0x0000},//继续升级固件？
										};

	LCD_Clear(BLACK);
	
	LCD_ShowImg_From_Flash(FOTA_LOGO_X, FOTA_LOGO_Y, IMG_OTA_LOGO_ADDR);
	LCD_ShowImg_From_Flash(FOTA_YES_X, FOTA_YES_Y, IMG_OTA_YES_ADDR);
	LCD_ShowImg_From_Flash(FOTA_NO_X, FOTA_NO_Y, IMG_OTA_NO_ADDR);

#ifdef FONTMAKER_UNICODE_FONT
	switch(global_settings.language)
	{
	case LANGUAGE_CHN:
		LCD_MeasureUniString(str_notify[global_settings.language], &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+(FOTA_START_STR_H-h)/2, str_notify[global_settings.language]);
		break;
		
	case LANGUAGE_EN:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*12);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][12], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-12));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;
		
	case LANGUAGE_DE:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*18);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][18], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-18));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;
	}
#endif

	SetRightKeyUpHandler(fota_excu);
	SetLeftKeyUpHandler(fota_exit);
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FOTA_YES_X-30, FOTA_YES_X+FOTA_YES_W+30, FOTA_YES_Y-30, FOTA_YES_Y+FOTA_YES_H+30, fota_excu);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FOTA_NO_X-30, FOTA_NO_X+FOTA_NO_W+30, FOTA_NO_Y-30, FOTA_NO_Y+FOTA_NO_H+30, fota_exit);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_exit);	
#endif
}

void FOTAUpdateStatus(void)
{
	u16_t pro_len;
	u16_t x,y,w,h;
	u8_t pro_buf[16] = {0};
	u16_t tmpbuf[128] = {0};
	static bool flag = false;
	static u16_t pro_str_x,pro_str_y;

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif	
	switch(get_fota_status())
	{
	case FOTA_STATUS_PREPARE:
		flag = false;
		break;
		
	case FOTA_STATUS_LINKING:
		{
			u16_t str_notify[LANGUAGE_MAX][26] = {
													{0x004C,0x0069,0x006E,0x006B,0x0069,0x006E,0x0067,0x0020,0x0074,0x006F,0x0020,0x0073,0x0065,0x0072,0x0076,0x0065,0x0072,0x002E,0x002E,0x002E,0x0000},//Linking to server...
													{0x0056,0x0065,0x0072,0x006B,0x006E,0x00FC,0x0070,0x0066,0x0075,0x006E,0x0067,0x0020,0x006D,0x0069,0x0074,0x0020,0x0053,0x0065,0x0072,0x0076,0x0065,0x0072,0x002E,0x002E,0x002E,0x0000},//Verknüpfung mit Server...
													{0x8FDE,0x63A5,0x670D,0x52A1,0x5668,0x002E,0x002E,0x002E,0x0000},//连接服务器...
												};

			LCD_Fill(0, FOTA_YES_Y, LCD_WIDTH, FOTA_YES_H, BLACK);
			LCD_Fill(0, FOTA_START_STR_Y, LCD_WIDTH, FOTA_START_STR_H, BLACK);

		#ifdef FONTMAKER_UNICODE_FONT
			LCD_MeasureUniString(str_notify[global_settings.language], &w, &h);
			LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+(FOTA_START_STR_H-h)/2, str_notify[global_settings.language]);
		#endif

			ClearAllKeyHandler();
		}
		break;
		
	case FOTA_STATUS_DOWNLOADING:
		if(!flag)
		{
			u16_t str_notify[LANGUAGE_MAX][32] = {
													{0x0044,0x0061,0x0074,0x0061,0x0020,0x0064,0x006F,0x0077,0x006E,0x006C,0x006F,0x0061,0x0064,0x0069,0x006E,0x0067,0x002E,0x002E,0x002E,0x0000},//Data downloading...
													{0x0044,0x0061,0x0074,0x0065,0x006E,0x0020,0x0077,0x0065,0x0072,0x0064,0x0065,0x006E,0x0020,0x0068,0x0065,0x0072,0x0075,0x006E,0x0074,0x0065,0x0072,0x0067,0x0065,0x006C,0x0061,0x0064,0x0065,0x006E,0x002E,0x002E,0x002E,0x0000},//Daten werden heruntergeladen...
													{0x6570,0x636E,0x4E0B,0x8F7D,0x4E2D,0x002E,0x002E,0x002E,0x0000},//数据下载中…
												};
			
			flag = true;
			
			LCD_Fill(0, FOTA_START_STR_Y, LCD_WIDTH, FOTA_START_STR_H, BLACK);

		#ifdef FONTMAKER_UNICODE_FONT
			switch(global_settings.language)
			{
			case LANGUAGE_CHN:
			case LANGUAGE_EN:
				LCD_MeasureUniString(str_notify[global_settings.language], &w, &h);
				LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+(FOTA_START_STR_H-h)/2, str_notify[global_settings.language]);
				break;
				
			case LANGUAGE_DE:
				memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*13);
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
				
				memset(tmpbuf, 0x0000, sizeof(tmpbuf));
				memcpy(tmpbuf, &str_notify[global_settings.language][13], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-13));
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
				break;
			}
		#endif
			
			LCD_DrawRectangle(FOTA_PROGRESS_X, FOTA_PROGRESS_Y, FOTA_PROGRESS_W, FOTA_PROGRESS_H);
			LCD_Fill(FOTA_PROGRESS_X+1, FOTA_PROGRESS_Y+1, FOTA_PROGRESS_W-1, FOTA_PROGRESS_H-1, BLACK);
			
			sprintf(pro_buf, "%3d%%", g_fota_progress);
		#ifdef FONTMAKER_UNICODE_FONT
			LCD_SetFontSize(FONT_SIZE_20);
		#else	
			LCD_SetFontSize(FONT_SIZE_16);
		#endif
			memset(tmpbuf, 0x0000, sizeof(tmpbuf));
			mmi_asc_to_ucs2(tmpbuf, pro_buf);
			LCD_MeasureUniString(tmpbuf, &w, &h);
			pro_str_x = FOTA_PRO_NUM_X+(FOTA_PRO_NUM_W-w)/2;
			pro_str_y = FOTA_PRO_NUM_Y+(FOTA_PRO_NUM_H-h)/2;
			LCD_ShowUniString(pro_str_x,pro_str_y, tmpbuf);
		}
		else
		{
			pro_len = (g_fota_progress*FOTA_PROGRESS_W)/100;
			LCD_Fill(FOTA_PROGRESS_X+1, FOTA_PROGRESS_Y+1, pro_len, FOTA_PROGRESS_H-1, WHITE);
			
			sprintf(pro_buf, "%3d%%", g_fota_progress);
			memset(tmpbuf, 0x0000, sizeof(tmpbuf));
			mmi_asc_to_ucs2(tmpbuf, pro_buf);
			LCD_ShowUniString(pro_str_x, pro_str_y, tmpbuf);
		}		

		ClearAllKeyHandler();
		break;
		
	case FOTA_STATUS_FINISHED:
		{
			u16_t str_notify[LANGUAGE_MAX][22] = {
													{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0063,0x006F,0x006D,0x0070,0x006C,0x0065,0x0074,0x0065,0x0064,0x0000},//Upgrade completed
													{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0061,0x0062,0x0067,0x0065,0x0073,0x0063,0x0068,0x006C,0x006F,0x0073,0x0073,0x0065,0x006E,0x0000},//Upgrade abgeschlossen
													{0x5347,0x7EA7,0x6210,0x529F,0x0000},//升级成功
												};
			
			flag = false;
		
			LCD_Clear(BLACK);
			LCD_ShowImg_From_Flash(FOTA_FINISH_ICON_X, FOTA_FINISH_ICON_Y, IMG_OTA_FINISH_ICON_ADDR);
			
		#ifdef FONTMAKER_UNICODE_FONT
			LCD_MeasureUniString(str_notify[global_settings.language], &w, &h);
			LCD_ShowUniString(FOTA_FINISH_STR_X+(FOTA_FINISH_STR_W-w)/2, FOTA_FINISH_STR_Y+(FOTA_FINISH_STR_H-h)/2, str_notify[global_settings.language]);
		#endif
		
			SetLeftKeyUpHandler(fota_reboot_confirm);
			SetRightKeyUpHandler(fota_reboot_confirm);
		#ifdef CONFIG_TOUCH_SUPPORT
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_reboot_confirm);
		#endif
		}
		break;
		
	case FOTA_STATUS_ERROR:
		{
			u16_t str_notify[LANGUAGE_MAX][23] = {
													{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0066,0x0061,0x0069,0x006C,0x0065,0x0064,0x0000},//Upgrade failed
													{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0066,0x0065,0x0068,0x006C,0x0067,0x0065,0x0073,0x0063,0x0068,0x006C,0x0061,0x0067,0x0065,0x006E,0x0000},//Upgrade fehlgeschlagen
													{0x5347,0x7EA7,0x5931,0x8D25,0x0000},//升级失败
												};
			
			flag = false;

			LCD_Clear(BLACK);
			LCD_ShowImg_From_Flash(FOTA_FAIL_ICON_X, FOTA_FAIL_ICON_Y, IMG_OTA_FAILED_ICON_ADDR);
			
		#ifdef FONTMAKER_UNICODE_FONT
			LCD_MeasureUniString(str_notify[global_settings.language], &w, &h);
			LCD_ShowUniString(FOTA_FAIL_STR_X+(FOTA_FAIL_STR_W-w)/2, FOTA_FAIL_STR_Y+(FOTA_FAIL_STR_H-h)/2, str_notify[global_settings.language]);
		#endif	

			SetLeftKeyUpHandler(fota_exit);
			SetRightKeyUpHandler(fota_exit);
		#ifdef CONFIG_TOUCH_SUPPORT
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_exit);
		#endif	
		}
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
	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		dl_img_start();
	}
	else if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		dl_font_start();
	}
  #ifdef CONFIG_PPG_DATA_UPDATE
	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
	{
		dl_ppg_start();
	}
  #endif
	else
#endif
	{
		EnterPoweroffScreen();
	}
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
#ifdef CONFIG_FALL_DETECT_SUPPORT
void FallShowStatus(void)
{
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
#endif/*CONFIG_FALL_DETECT_SUPPORT*/

#ifdef CONFIG_SLEEP_SUPPORT
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

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(IsInTempScreen()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif

	k_timer_stop(&mainmenu_timer);
	LCD_Set_BL_Mode(LCD_BL_AUTO);

#ifdef CONFIG_SYNC_SUPPORT
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
	SetRightKeyUpHandler(ExitSleepScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

 #ifdef CONFIG_SYNC_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen); 
 #else
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
 #endif

 #ifdef CONFIG_STEP_SUPPORT
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
 #elif defined(CONFIG_PPG_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
 #elif defined(CONFIG_TEMP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
 #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
 #endif
#endif
}
#endif/*CONFIG_SLEEP_SUPPORT*/

#ifdef CONFIG_STEP_SUPPORT
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
		LCD_ShowString(IMU_STEP_STR_X+(IMU_STEP_STR_W-w)/2, IMU_STEP_STR_Y, strbuf);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif
		sprintf(strbuf, "%d", calorie);
		LCD_MeasureString(strbuf,&w,&h);
		LCD_ShowString(IMU_CAL_UNIT_X-w-5, IMU_CAL_STR_Y, strbuf);	

		sprintf(strbuf, "%d.%d%d", (distance/1000), ((distance%1000)/100), ((distance%100)/10));
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
		LCD_ShowString(IMU_STEP_STR_X+(IMU_STEP_STR_W-w)/2, IMU_STEP_STR_Y, strbuf);

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

	k_timer_stop(&mainmenu_timer);
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(IsInTempScreen()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif
	LCD_Set_BL_Mode(LCD_BL_AUTO);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_STEPS;	
	scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_STEPS].status = SCREEN_STATUS_CREATING;

#ifdef CONFIG_SLEEP_SUPPORT
	SetLeftKeyUpHandler(EnterSleepScreen);
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
	SetRightKeyUpHandler(ExitStepsScreen);
	
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

  #ifdef CONFIG_SLEEP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
  #elif defined(CONFIG_SYNC_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
  #else
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
  #endif
  
  #ifdef CONFIG_PPG_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
  #endif
#endif
}
#endif/*CONFIG_STEP_SUPPORT*/
#endif/*CONFIG_IMU_SUPPORT*/

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

void EntryIdleScr(void)
{
	entry_idle_flag = true;
}

void EnterIdleScreen(void)
{
	if(screen_id == SCREEN_ID_IDLE)
		return;

	k_timer_stop(&notify_timer);
	k_timer_stop(&mainmenu_timer);
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif
#ifdef NB_SIGNAL_TEST
	MenuStopNB();
	if(gps_is_working())
		MenuStopGPS();
#endif	
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		PPGStopCheck();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(IsInTempScreen()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_SYNC_SUPPORT
	if(SyncIsRunning())
		SyncDataStop();
#endif

	LCD_Set_BL_Mode(LCD_BL_AUTO);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_IDLE;
	scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATING;

#ifdef NB_SIGNAL_TEST
	SetLeftKeyUpHandler(EnterNBTestScreen);
#else
#ifdef CONFIG_PPG_SUPPORT
	SetLeftKeyUpHandler(EnterHRScreen);
#elif defined(CONFIG_TEMP_SUPPORT)
	SetLeftKeyUpHandler(EnterTempScreen);
#elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
 #ifdef CONFIG_STEP_SUPPORT
	SetLeftKeyUpHandler(EnterStepsScreen);
 #elif defined(CONFIG_SLEEP_SUPPORT)
	SetLeftKeyUpHandler(EnterSleepScreen);
 #endif
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
#endif
	SetRightKeyLongPressHandler(SOSTrigger);
	SetRightKeyUpHandler(EnterIdleScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
 #ifdef NB_SIGNAL_TEST
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterNBTestScreen);
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 #else
  #ifdef CONFIG_PPG_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
   #ifdef CONFIG_STEP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
   #elif defined(CONFIG_SLEEP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
   #endif
  #elif defined(CONFIG_SYNC_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
  #else
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
  #endif
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
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
	u8_t tmpbuf[512] = {0};
	
	LCD_Fill((LCD_WIDTH-194)/2, 50, 194, 160, BLACK);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(tmpbuf, gps_test_info);
	LCD_ShowUniStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, (u16_t*)tmpbuf);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
	LCD_ShowStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, gps_test_info);
#endif
}

void TestGPSShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[512] = {0};
	
	LCD_Clear(BLACK);
	
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(strbuf, "GPS TESTING");
	LCD_MeasureUniString((u16_t*)strbuf, &w, &h);
	LCD_ShowUniString((LCD_WIDTH-w)/2, 20, (u16_t*)strbuf);
	mmi_asc_to_ucs2(strbuf, "GPS Starting...");
	LCD_ShowUniStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, (u16_t*)strbuf);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
	strcpy(strbuf, "GPS TESTING");
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, "GPS Starting...");
#endif
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

	MenuStopNB();

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	SetLeftKeyUpHandler(EnterPoweroffScreen);
	SetRightKeyUpHandler(ExitGPSTestScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterNBTestScreen);
#endif	
}

void TestNBUpdateINfor(void)
{
	u8_t tmpbuf[512] = {0};

	LCD_Fill(30, 50, 190, 160, BLACK);
#ifdef FONTMAKER_UNICODE_FONT
	mmi_asc_to_ucs2(tmpbuf, nb_test_info);
	LCD_ShowUniStringInRect(30, 50, 180, 160, (u16_t*)tmpbuf);	
#else
	LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);
#endif
}

void TestNBShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[512] = {0};
	
	LCD_Clear(BLACK);
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(strbuf, "NB-IoT TESTING");
	LCD_MeasureUniString((u16_t*)strbuf, &w, &h);
	LCD_ShowUniString((LCD_WIDTH-w)/2, 20, (u16_t*)strbuf);
	mmi_asc_to_ucs2(strbuf, nb_test_info);
	LCD_ShowUniStringInRect(30, 50, 180, 160, (u16_t*)strbuf);	
#else	
	LCD_SetFontSize(FONT_SIZE_16);
	strcpy(strbuf, "NB-IoT TESTING");
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);
#endif
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
	k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);
	
	if(gps_is_working())
		MenuStopGPS();

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	SetLeftKeyUpHandler(EnterGPSTestScreen);
	SetRightKeyUpHandler(ExitNBTestScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterGPSTestScreen);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
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

	k_timer_stop(&notify_timer);
	k_timer_stop(&mainmenu_timer);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif
#ifdef NB_SIGNAL_TEST
	if(gps_is_working())
		MenuStopGPS();
#endif	
#ifdef CONFIG_PPG_SUPPORT
	if(PPGIsWorking())
		PPGStopCheck();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(IsInTempScreen()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_SYNC_SUPPORT
	if(SyncIsRunning())
		SyncDataStop();
#endif

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SOS;	
	scr_msg[SCREEN_ID_SOS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SOS].status = SCREEN_STATUS_CREATING;

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

	screen_id = history_screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_ENTER;
	scr_msg[history_screen_id].status = SCREEN_STATUS_CREATING;	
}

void ScreenMsgProcess(void)
{
	if(entry_idle_flag)
	{
		EnterIdleScreen();
		entry_idle_flag = false;
	}
	
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
	  #ifdef CONFIG_SLEEP_SUPPORT
		case SCREEN_ID_SLEEP:
			SleepScreenProcess();
			break;
	  #endif
	  #ifdef CONFIG_STEP_SUPPORT
		case SCREEN_ID_STEPS:
			StepsScreenProcess();
			break;
	  #endif
	  #ifdef CONFIG_FALL_DETECT_SUPPORT
		case SCREEN_ID_FALL:
			FallScreenProcess();
			break;
	  #endif
	#endif
		case SCREEN_ID_WRIST:
			WristScreenProcess();
			break;				
		case SCREEN_ID_SETTINGS:
			SettingsScreenProcess();
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
