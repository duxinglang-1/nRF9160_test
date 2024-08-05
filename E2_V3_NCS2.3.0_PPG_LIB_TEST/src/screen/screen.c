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
#include <string.h>
#include <zephyr/kernel.h>
#include "settings.h"
#include "lcd.h"
#include "font.h"
#include "img.h"
#include "key.h"
#include "datetime.h"
#include "max20353.h"
#ifdef CONFIG_PPG_SUPPORT
#include "ppg.h"
#endif
#ifdef CONFIG_IMU_SUPPORT
#include "lsm6dso.h"
#include "fall.h"
#ifdef CONFIG_SLEEP_SUPPORT
#include "sleep.h"
#endif/*CONFIG_SLEEP_SUPPORT*/
#endif
#include "external_flash.h"
#include "screen.h"
#include "ucs2.h"
#include "nb.h"
#include "sos.h"
#ifdef CONFIG_ALARM_SUPPORT
#include "alarm.h"
#endif
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
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif/*CONFIG_WIFI_SUPPORT*/
#ifdef CONFIG_SYNC_SUPPORT
#include "sync.h"
#endif/*CONFIG_SYNC_SUPPORT*/
#ifdef CONFIG_TEMP_SUPPORT
#include "temp.h"
#endif/*CONFIG_TEMP_SUPPORT*/
#ifdef CONFIG_FACTORY_TEST_SUPPORT
#include "ft_main.h"
#include "ft_aging.h"
#endif/*CONFIG_FACTORY_TEST_SUPPORT*/
#include "logger.h"

static uint8_t scr_index = 0;
static uint8_t bat_charging_index = 0;
static bool exit_notify_flag = false;
static bool entry_idle_flag = false;
static bool entry_setting_bk_flag = false;

static void NotifyTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(notify_timer, NotifyTimerOutCallBack, NULL);
static void MainMenuTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(mainmenu_timer, MainMenuTimerOutCallBack, NULL);
#ifdef CONFIG_PPG_SUPPORT
static void PPGStatusTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_status_timer, PPGStatusTimerOutCallBack, NULL);
#endif
#ifdef CONFIG_TEMP_SUPPORT
static void TempStatusTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_status_timer, TempStatusTimerOutCallBack, NULL);
#endif

SCREEN_ID_ENUM screen_id = SCREEN_ID_BOOTUP;
SCREEN_ID_ENUM history_screen_id = SCREEN_ID_BOOTUP;
screen_msg scr_msg[SCREEN_ID_MAX] = {0};
notify_infor notify_msg = {0};

extern bool key_pwroff_flag;
extern uint8_t g_rsrp;

static void EnterHRScreen(void);

#ifdef IMG_FONT_FROM_FLASH
static uint32_t logo_img[] = 
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

static uint16_t still_str[LANGUAGE_MAX][16] = {
											#ifndef FW_FOR_CN
												{0x0053,0x0074,0x0061,0x0079,0x0020,0x0073,0x0074,0x0069,0x006C,0x006C,0x0000},//Stay still
												{0x0052,0x0075,0x0068,0x0069,0x0067,0x0020,0x0068,0x0061,0x006C,0x0074,0x0065,0x006E,0x0000},//Ruhig halten
												{0x0052,0x0065,0x0073,0x0074,0x0065,0x0072,0x0020,0x0069,0x006D,0x006D,0x006F,0x0062,0x0069,0x006C,0x0065,0x0000},//Rester immobile
												{0x0053,0x0074,0x0061,0x0069,0x0020,0x0066,0x0065,0x0072,0x006D,0x006F,0x0000},//Stai fermo
												{0x004E,0x006F,0x0020,0x0074,0x0065,0x0020,0x006D,0x0075,0x0065,0x0076,0x0061,0x0073,0x0000},//No te muevas
												{0x0046,0x0069,0x0071,0x0075,0x0065,0x0020,0x0070,0x0061,0x0072,0x0061,0x0064,0x006F,0x0000},//Fique parado
											#else
												{0x4FDD,0x6301,0x9759,0x6B62,0x0000},//保持静止
												{0x0053,0x0074,0x0061,0x0079,0x0020,0x0073,0x0074,0x0069,0x006C,0x006C,0x0000},//Stay still
											#endif
											  };
static uint16_t Incon_1_str[LANGUAGE_MAX][35] = {
												#ifndef FW_FOR_CN
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0073,0x0069,0x0076,0x0065,0x0020,0x0072,0x0065,0x0074,0x0072,0x0079,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0000},//Inconclusive retry later
													{0x0046,0x0065,0x0068,0x006C,0x0065,0x0072,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0065,0x0072,0x006E,0x0065,0x0075,0x0074,0x0000},//Fehler versuchen Sie es erneut
													{0x004E,0x006F,0x006E,0x0020,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0061,0x006E,0x0074,0x0020,0x0072,0x00E9,0x0065,0x0073,0x0073,0x0061,0x0079,0x0065,0x007A,0x0020,0x0070,0x006C,0x0075,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0000},//Non concluant, réessayez plus tard
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0064,0x0065,0x006E,0x0074,0x0065,0x0020,0x0072,0x0069,0x0070,0x0072,0x006F,0x0076,0x0061,0x0072,0x0065,0x0020,0x0070,0x0069,0x00F9,0x0020,0x0074,0x0061,0x0072,0x0064,0x0069,0x0000},//Inconcludente, riprovare più tardi
													{0x004E,0x006F,0x0020,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0079,0x0065,0x006E,0x0074,0x0065,0x0020,0x0069,0x006E,0x0074,0x00E9,0x006E,0x0074,0x0065,0x006C,0x006F,0x0020,0x006D,0x00E1,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0065,0x0000},//No concluyente, inténtelo más tarde
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0073,0x0069,0x0076,0x006F,0x0020,0x0074,0x0065,0x006E,0x0074,0x0065,0x0020,0x006D,0x0061,0x0069,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0065,0x0000},//Inconclusivo, tente mais tarde
												#else
													{0x6D4B,0x91CF,0x5931,0x8D25,0xFF0C,0x8BF7,0x7A0D,0x540E,0x91CD,0x8BD5,0x0000},//测量失败，请稍后重试
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0073,0x0069,0x0076,0x0065,0x0020,0x0072,0x0065,0x0074,0x0072,0x0079,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0000},//Inconclusive retry later
												#endif
												};
static uint16_t Incon_2_str[LANGUAGE_MAX][15] = {
												#ifndef FW_FOR_CN
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0073,0x0069,0x0076,0x0065,0x0000},//Inconclusive
													{0x0046,0x0065,0x0068,0x006C,0x0065,0x0072,0x0000},//Fehler
													{0x004E,0x006F,0x006E,0x0020,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0061,0x006E,0x0074,0x0000},//Non concluant
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0064,0x0065,0x006E,0x0074,0x0065,0x0000},//Inconcludente
													{0x004E,0x006F,0x0020,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0079,0x0065,0x006E,0x0074,0x0065,0x0000},//No concluyente
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0073,0x0069,0x0076,0x006F,0x0000},//Inconclusivo
												#else
													{0x6D4B,0x91CF,0x5931,0x8D25,0x0000},//测量失败
													{0x0049,0x006E,0x0063,0x006F,0x006E,0x0063,0x006C,0x0075,0x0073,0x0069,0x0076,0x0065,0x0000},//Inconclusive
												#endif
												};
static uint16_t still_retry_str[LANGUAGE_MAX][37] = {
													#ifndef FW_FOR_CN
														{0x004B,0x0065,0x0065,0x0070,0x0020,0x0073,0x0074,0x0069,0x006C,0x006C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0072,0x0065,0x0074,0x0072,0x0079,0x0000},//Keep still and retry
														{0x0052,0x0075,0x0068,0x0069,0x0067,0x0020,0x0068,0x0061,0x006C,0x0074,0x0065,0x006E,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0065,0x0072,0x006E,0x0065,0x0075,0x0074,0x0000},//Ruhig halten versuchen Sie es erneut
														{0x0052,0x0065,0x0073,0x0074,0x0065,0x0072,0x0020,0x0069,0x006D,0x006D,0x006F,0x0062,0x0069,0x006C,0x0065,0x0020,0x0065,0x0074,0x0020,0x0072,0x00E9,0x0065,0x0073,0x0073,0x0061,0x0079,0x0065,0x0072,0x0000},//Rester immobile et réessayer
														{0x0052,0x0069,0x006D,0x0061,0x006E,0x0065,0x0072,0x0065,0x0020,0x0066,0x0065,0x0072,0x006D,0x0069,0x0020,0x0065,0x0020,0x0072,0x0069,0x0074,0x0065,0x006E,0x0074,0x0061,0x0072,0x0065,0x0000},//Rimanere fermi e ritentare
														{0x004E,0x006F,0x0020,0x0074,0x0065,0x0020,0x006D,0x0075,0x0065,0x0076,0x0061,0x0073,0x0020,0x0079,0x0020,0x0076,0x0075,0x0065,0x006C,0x0076,0x0065,0x0020,0x0061,0x0020,0x0069,0x006E,0x0074,0x0065,0x006E,0x0074,0x0061,0x0072,0x006C,0x006F,0x0000},//No te muevas y vuelve a intentarlo
														{0x0046,0x0069,0x0071,0x0075,0x0065,0x0020,0x0070,0x0061,0x0072,0x0061,0x0064,0x006F,0x0020,0x0065,0x0020,0x0074,0x0065,0x006E,0x0074,0x0065,0x0020,0x006E,0x006F,0x0076,0x0061,0x006D,0x0065,0x006E,0x0074,0x0065,0x0000},//Fique parado e tente novamente
													#else
														{0x4FDD,0x6301,0x9759,0x6B62,0xFF0C,0x91CD,0x65B0,0x6D4B,0x91CF,0x0000},//保持静止，重新测量
														{0x004B,0x0065,0x0065,0x0070,0x0020,0x0073,0x0074,0x0069,0x006C,0x006C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0072,0x0065,0x0074,0x0072,0x0079,0x0000},//Keep still and retry
													#endif
													};

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
	uint8_t i,count=0;
	uint16_t x,y,w,h;

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

void MainMenuTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_GPS_TEST)
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
#ifdef CONFIG_WIFI_SUPPORT	
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
				if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
					dl_font_start();
			#if defined(CONFIG_PPG_SUPPORT)
				else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
					dl_ppg_start();
			#endif
				else
				{
					if(get_dl_status() == DL_STATUS_FINISHED)
						dl_reboot_confirm();
					else
						EntryIdleScr();
				}
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
				else
			#endif		
				{
					if(get_dl_status() == DL_STATUS_FINISHED)
						dl_reboot_confirm();
					else
						EntryIdleScr();
				}
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
				if(get_dl_status() == DL_STATUS_FINISHED)
					dl_reboot_confirm();
				else
					EntryIdleScr();
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
#ifdef UI_STYLE_HEALTH_BAR
	else if(screen_id == SCREEN_ID_HR)
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
#endif/*UI_STYLE_HEALTH_BAR*/
}

#ifdef CONFIG_BLE_SUPPORT
void IdleShowBleStatus(void)
{
	uint16_t x,y,w,h;
	uint16_t ble_link_str[] = {0x0042,0x004C,0x0045,0x0000};//BLE

	LCD_SetFontSize(FONT_SIZE_28);

	if(g_ble_connected)
	{
		LCD_MeasureUniString(ble_link_str, &w, &h);
		LCD_ShowUniString(IDLE_BLE_X+(IDLE_BLE_W-w)/2, IDLE_BLE_Y+(IDLE_BLE_H-w)/2, ble_link_str);
	}
	else
	{
		LCD_FillColor(IDLE_BLE_X, IDLE_BLE_Y, IDLE_BLE_W, IDLE_BLE_H, BLACK);
	}
}
#endif

void IdleShowSystemDate(void)
{
	uint16_t x,y,w,h;
	uint8_t str_date[20] = {0};
	uint8_t tmpbuf[128] = {0};
#ifdef FONTMAKER_UNICODE_FONT	
	uint16_t str_mon[LANGUAGE_MAX][12][5] = {
											#ifndef FW_FOR_CN
												{
													{0x004A,0x0061,0x006E,0x0000,0x0000},//Jan
													{0x0046,0x0065,0x0062,0x0000,0x0000},//Feb
													{0x004D,0x0061,0x0072,0x0000,0x0000},//Mar
													{0x0041,0x0070,0x0072,0x0000,0x0000},//Apr
													{0x004D,0x0061,0x0079,0x0000,0x0000},//May
													{0x004A,0x0075,0x006E,0x0000,0x0000},//Jun
													{0x004A,0x0075,0x006C,0x0000,0x0000},//Jul
													{0x0041,0x0075,0x0067,0x0000,0x0000},//Aug
													{0x0053,0x0065,0x0070,0x0074,0x0000},//Sept
													{0x004F,0x0063,0x0074,0x0000,0x0000},//Oct
													{0x004E,0x006F,0x0076,0x0000,0x0000},//Nov
													{0x0044,0x0065,0x0063,0x0000,0x0000},//Dec
												},
												{
													{0x004A,0x0061,0x006E,0x0000,0x0000},//Jan
													{0x0046,0x0065,0x0062,0x0000,0x0000},//Feb
													{0x004D,0x00E4,0x0072,0x007A,0x0000},//M鋜z
													{0x0041,0x0070,0x0072,0x0000,0x0000},//Apr
													{0x004D,0x0061,0x0069,0x0000,0x0000},//Mai
													{0x004A,0x0075,0x006E,0x0000,0x0000},//Jun
													{0x004A,0x0075,0x006C,0x0000,0x0000},//Jul
													{0x0041,0x0075,0x0067,0x0000,0x0000},//Aug
													{0x0053,0x0065,0x0070,0x0074,0x0000},//Sept
													{0x004F,0x006B,0x0074,0x0000,0x0000},//Okt
													{0x004E,0x006F,0x0076,0x0000,0x0000},//Nov
													{0x0044,0x0065,0x007A,0x0000,0x0000},//Dez
												},
												{
													{0x004A,0x0061,0x006E,0x0000,0x0000},//Jan
													{0x0046,0x00E9,0x0076,0x0000,0x0000},//Fév
													{0x004D,0x0061,0x0072,0x0000,0x0000},//Mar
													{0x0041,0x0076,0x0072,0x0000,0x0000},//Avr
													{0x004D,0x0061,0x0069,0x0000,0x0000},//Mai
													{0x004A,0x0075,0x0069,0x006E,0x0000},//Juin
													{0x004A,0x0075,0x0069,0x006C,0x0000},//Juil
													{0x0041,0x006F,0x00FB,0x0074,0x0000},//Ao?t
													{0x0053,0x0065,0x0070,0x0074,0x0000},//Sept
													{0x004F,0x0063,0x0074,0x0000,0x0000},//Oct
													{0x004E,0x006F,0x0076,0x0000,0x0000},//Nov
													{0x0044,0x00E9,0x0063,0x0000,0x0000},//Déc
												},
												{
													{0x0047,0x0065,0x006E,0x0000},//Gen
													{0x0046,0x0065,0x0062,0x0000},//Feb
													{0x004D,0x0061,0x0072,0x0000},//Mar
													{0x0041,0x0070,0x0072,0x0000},//Apr
													{0x004D,0x0061,0x0067,0x0000},//Mag
													{0x0047,0x0069,0x0075,0x0000},//Giu
													{0x004C,0x0075,0x0067,0x0000},//Lug
													{0x0041,0x0067,0x006F,0x0000},//Ago
													{0x0053,0x0065,0x0074,0x0000},//Set
													{0x004F,0x0074,0x0074,0x0000},//Ott
													{0x004E,0x006F,0x0076,0x0000},//Nov
													{0x0044,0x0069,0x0063,0x0000},//Dic
												},
												{
													{0x0045,0x006E,0x0065,0x0000},//Ene
													{0x0046,0x0065,0x0062,0x0000},//Feb
													{0x004D,0x0061,0x0072,0x0000},//Mar
													{0x0041,0x0062,0x0072,0x0000},//Abr
													{0x004D,0x0061,0x0079,0x0000},//May
													{0x004A,0x0075,0x006E,0x0000},//Jun
													{0x004A,0x0075,0x006C,0x0000},//Jul
													{0x0041,0x0067,0x006F,0x0000},//Ago
													{0x0053,0x0065,0x0070,0x0000},//Sep
													{0x004F,0x0063,0x0074,0x0000},//Oct
													{0x004E,0x006F,0x0076,0x0000},//Nov
													{0x0044,0x0069,0x0063,0x0000},//Dic
												},
												{
													{0x004A,0x0061,0x006E,0x0000},//Jan
													{0x0046,0x0065,0x0076,0x0000},//Fev
													{0x004D,0x0061,0x0072,0x0000},//Mar
													{0x0041,0x0062,0x0072,0x0000},//Abr
													{0x004D,0x0061,0x0069,0x0000},//Mai
													{0x004A,0x0075,0x006E,0x0000},//Jun
													{0x004A,0x0075,0x006C,0x0000},//Jul
													{0x0041,0x0067,0x006F,0x0000},//Ago
													{0x0053,0x0065,0x0074,0x0000},//Set
													{0x004F,0x0063,0x0074,0x0000},//Oct
													{0x004E,0x006F,0x0076,0x0000},//Nov
													{0x0044,0x0065,0x007A,0x0000},//Dez
												},
											#else
												{
													{0x4E00,0x6708,0x0000,0x0000,0x0000},//一月
													{0x4E8C,0x6708,0x0000,0x0000,0x0000},//二月
													{0x4E09,0x6708,0x0000,0x0000,0x0000},//三月
													{0x56DB,0x6708,0x0000,0x0000,0x0000},//四月
													{0x4E94,0x6708,0x0000,0x0000,0x0000},//五月
													{0x516D,0x6708,0x0000,0x0000,0x0000},//六月
													{0x4E03,0x6708,0x0000,0x0000,0x0000},//七月
													{0x516B,0x6708,0x0000,0x0000,0x0000},//八月
													{0x4E5D,0x6708,0x0000,0x0000,0x0000},//九月
													{0x5341,0x6708,0x0000,0x0000,0x0000},//十月
													{0x5341,0x4E00,0x6708,0x0000,0x0000},//十一月
													{0x5341,0x4E8C,0x6708,0x0000,0x0000},//十二月
												},
												{
													{0x004A,0x0061,0x006E,0x0000,0x0000},//Jan
													{0x0046,0x0065,0x0062,0x0000,0x0000},//Feb
													{0x004D,0x0061,0x0072,0x0000,0x0000},//Mar
													{0x0041,0x0070,0x0072,0x0000,0x0000},//Apr
													{0x004D,0x0061,0x0079,0x0000,0x0000},//May
													{0x004A,0x0075,0x006E,0x0000,0x0000},//Jun
													{0x004A,0x0075,0x006C,0x0000,0x0000},//Jul
													{0x0041,0x0075,0x0067,0x0000,0x0000},//Aug
													{0x0053,0x0065,0x0070,0x0074,0x0000},//Sept
													{0x004F,0x0063,0x0074,0x0000,0x0000},//Oct
													{0x004E,0x006F,0x0076,0x0000,0x0000},//Nov
													{0x0044,0x0065,0x0063,0x0000,0x0000},//Dec
												},
											#endif
											};
	uint16_t str_mon_cn[2] = {0x6708,0x0000};
	uint16_t str_day_cn[2] = {0x65E5,0x0000};
#else	
	uint8_t *str_mon[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sept","Oct","Nov","Dec"};
#endif
	uint32_t img_24_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
							 IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};
	uint32_t img_20_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							 IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);

	switch(global_settings.language)
	{
  	#ifdef FW_FOR_CN
	case LANGUAGE_CHN:
		LCD_FillColor(IDLE_DATE_MON_CN_X, IDLE_DATE_MON_CN_Y, IDLE_DATE_MON_CN_W, IDLE_DATE_MON_CN_H, BLACK);

		x = IDLE_DATE_MON_CN_X;
		y = IDLE_DATE_MON_CN_Y;
		if(date_time.month > 9)
		{
			LCD_ShowImg_From_Flash(x, y+2, img_24_num[date_time.month/10]);
			x += IDLE_DATE_NUM_CN_W;
		}
		LCD_ShowImg_From_Flash(x, y+2, img_24_num[date_time.month%10]);
		x += IDLE_DATE_NUM_CN_W;
		LCD_ShowUniString(x, y, str_mon_cn);
		LCD_MeasureUniString(str_mon_cn, &w, &h);

		x += w;
		if(date_time.day > 9)
		{
			LCD_ShowImg_From_Flash(x, y+2, img_24_num[date_time.day/10]);
			x += IDLE_DATE_NUM_CN_W;
		}
		LCD_ShowImg_From_Flash(x, y+2, img_24_num[date_time.day%10]);
		x += IDLE_DATE_NUM_CN_W;
		LCD_ShowUniString(x, y, str_day_cn);
		break;
  	#endif
	
	default:
		LCD_FillColor(IDLE_DATE_DAY_EN_X, IDLE_DATE_DAY_EN_Y, IDLE_DATE_DAY_EN_W, IDLE_DATE_DAY_EN_H, BLACK);

		LCD_ShowImg_From_Flash(IDLE_DATE_DAY_EN_X+0*IDLE_DATE_NUM_EN_W, IDLE_DATE_DAY_EN_Y+4, img_20_num[date_time.day/10]);
		LCD_ShowImg_From_Flash(IDLE_DATE_DAY_EN_X+1*IDLE_DATE_NUM_EN_W, IDLE_DATE_DAY_EN_Y+4, img_20_num[date_time.day%10]);
	
		LCD_FillColor(IDLE_DATE_MON_EN_X, IDLE_DATE_MON_EN_Y, IDLE_DATE_MON_EN_W, IDLE_DATE_MON_EN_H, BLACK);
		LCD_ShowUniString(IDLE_DATE_MON_EN_X, IDLE_DATE_MON_EN_Y, str_mon[global_settings.language][date_time.month-1]);
		break;
	}
#else
	LCD_SetFontSize(FONT_SIZE_32);

	sprintf((char*)str_date, "%02d", date_time.day);
	LCD_ShowString(IDLE_DATE_DAY_X,IDLE_DATE_DAY_Y,str_date);
	strcpy((char*)str_date, str_mon[date_time.month-1]);
	LCD_ShowString(IDLE_DATE_MON_X,IDLE_DATE_MON_Y,str_date);
#endif
}

void IdleShowSystemTime(void)
{
	static bool flag = false;
	uint32_t img_num[10] = {IMG_FONT_60_NUM_0_ADDR,IMG_FONT_60_NUM_1_ADDR,IMG_FONT_60_NUM_2_ADDR,IMG_FONT_60_NUM_3_ADDR,IMG_FONT_60_NUM_4_ADDR,
							IMG_FONT_60_NUM_5_ADDR,IMG_FONT_60_NUM_6_ADDR,IMG_FONT_60_NUM_7_ADDR,IMG_FONT_60_NUM_8_ADDR,IMG_FONT_60_NUM_9_ADDR};

	flag = !flag;

	LCD_ShowImg_From_Flash(IDLE_TIME_X+0*IDLE_TIME_NUM_W, IDLE_TIME_Y, img_num[date_time.hour/10]);
	LCD_ShowImg_From_Flash(IDLE_TIME_X+1*IDLE_TIME_NUM_W, IDLE_TIME_Y, img_num[date_time.hour%10]);
	
	if(flag)
		LCD_ShowImg_From_Flash(IDLE_TIME_X+2*IDLE_TIME_NUM_W, IDLE_TIME_Y, IMG_FONT_60_COLON_ADDR);
	else
		LCD_Fill(IDLE_TIME_X+2*IDLE_TIME_NUM_W, IDLE_TIME_Y, IDLE_TIME_COLON_W, IDLE_TIME_COLON_H, BLACK);
	
	LCD_ShowImg_From_Flash(IDLE_TIME_X+2*IDLE_TIME_NUM_W+IDLE_TIME_COLON_W, IDLE_TIME_Y, img_num[date_time.minute/10]);
	LCD_ShowImg_From_Flash(IDLE_TIME_X+3*IDLE_TIME_NUM_W+IDLE_TIME_COLON_W, IDLE_TIME_Y, img_num[date_time.minute%10]);
}

void IdleShowSystemWeek(void)
{
	uint16_t x,y,w,h;
#ifdef FONTMAKER_UNICODE_FONT	
	uint16_t str_week[LANGUAGE_MAX][7][4] = {
											#ifndef FW_FOR_CN
												{
													{0x0053,0x0075,0x006E,0x0000},//Sun
													{0x004D,0x006F,0x006E,0x0000},//Mon
													{0x0054,0x0075,0x0065,0x0000},//Tue
													{0x0057,0x0065,0x0064,0x0000},//Wed
													{0x0054,0x0068,0x0075,0x0000},//Thu
													{0x0046,0x0072,0x0069,0x0000},//Fri
													{0x0053,0x0061,0x0074,0x0000},//Sat
												},
												{
													{0x0053,0x006F,0x006E,0x0000},//Son
													{0x004D,0x006F,0x006E,0x0000},//Mon
													{0x0044,0x0069,0x0065,0x0000},//Die
													{0x004D,0x0069,0x0074,0x0000},//Mit
													{0x0044,0x006F,0x006E,0x0000},//Don
													{0x0046,0x0072,0x0065,0x0000},//Fre
													{0x0053,0x0061,0x006D,0x0000},//Sam
												},
												{
													{0x0044,0x0069,0x006D,0x0000},//Dim
													{0x004C,0x0075,0x006E,0x0000},//Lun
													{0x004D,0x0061,0x0072,0x0000},//Mar
													{0x004D,0x0065,0x0072,0x0000},//Mer
													{0x004A,0x0065,0x0075,0x0000},//Jeu
													{0x0056,0x0065,0x006E,0x0000},//Ven
													{0x0053,0x0061,0x006D,0x0000},//Sam
												},
												{
													{0x0044,0x006F,0x006D,0x0000},//Dom
													{0x004C,0x0075,0x006E,0x0000},//Lun
													{0x004D,0x0061,0x0072,0x0000},//Mar
													{0x004D,0x0065,0x0072,0x0000},//Mer
													{0x0047,0x0069,0x006F,0x0000},//Gio
													{0x0056,0x0065,0x006E,0x0000},//Ven
													{0x0053,0x0061,0x0062,0x0000},//Sab
												},
												{
													{0x0044,0x006F,0x006D,0x0000},//Dom
													{0x004C,0x0075,0x006E,0x0000},//Lun
													{0x004D,0x0061,0x0072,0x0000},//Mar
													{0x004D,0x0069,0x00E9,0x0000},//Mié
													{0x004A,0x0075,0x0065,0x0000},//Jue
													{0x0056,0x0069,0x0065,0x0000},//Vie
													{0x0053,0x00E1,0x0062,0x0000},//Sáb
												},
												{
													{0x0044,0x006F,0x006D,0x0000},//Dom
													{0x0053,0x0065,0x0067,0x0000},//Seg
													{0x0054,0x0065,0x0072,0x0000},//Ter
													{0x0051,0x0075,0x0061,0x0000},//Qua
													{0x0051,0x0075,0x0069,0x0000},//Qui
													{0x0053,0x0065,0x0078,0x0000},//Sex
													{0x0053,0x00E1,0x0062,0x0000},//Sáb
												},
											#else	
												{
				 									{0x5468,0x65E5,0x0000},//周日
													{0x5468,0x4E00,0x0000},//周一
													{0x5468,0x4E8C,0x0000},//周二
													{0x5468,0x4E09,0x0000},//周三
													{0x5468,0x56DB,0x0000},//周四
													{0x5468,0x4E94,0x0000},//周五
													{0x5468,0x516D,0x0000},//周六
												},
												{
													{0x0053,0x0075,0x006E,0x0000},//Sun
													{0x004D,0x006F,0x006E,0x0000},//Mon
													{0x0054,0x0075,0x0065,0x0000},//Tue
													{0x0057,0x0065,0x0064,0x0000},//Wed
													{0x0054,0x0068,0x0075,0x0000},//Thu
													{0x0046,0x0072,0x0069,0x0000},//Fri
													{0x0053,0x0061,0x0074,0x0000},//Sat
												},
											#endif
											};
#else
	uint8_t str_week[128] = {0};
#endif
	
	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);

	switch(global_settings.language)
	{
  	#ifdef FW_FOR_CN	
	case LANGUAGE_CHN:
		x = IDLE_WEEK_CN_X;
		y = IDLE_WEEK_CN_Y;
		break;
 	#endif

	default:
		x = IDLE_WEEK_EN_X;
		y = IDLE_WEEK_EN_Y;	
		break;
	}	
	LCD_FillColor(x, y, IDLE_WEEK_EN_W, IDLE_WEEK_EN_H, BLACK);
	LCD_ShowUniString(x, y, str_week[global_settings.language][date_time.week]);
#else
	LCD_SetFontSize(FONT_SIZE_32);
	GetSystemWeekStrings(str_week);
	LCD_ShowString(IDLE_WEEK_EN_X, IDLE_WEEK_EN_Y, str_week);
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
	uint16_t w,h;
	uint8_t strbuf[128] = {0};

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
		
		if(g_bat_soc >= 7)
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
	uint16_t w,h;
	uint8_t strbuf[128] = {0};

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
		if(g_bat_soc >= 7)
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
	uint32_t img_add[5] = {IMG_SIG_0_ADDR, IMG_SIG_1_ADDR, IMG_SIG_2_ADDR, IMG_SIG_3_ADDR, IMG_SIG_4_ADDR};

	if(nb_is_connected())
	{
		LCD_ShowImg_From_Flash(IDLE_SIGNAL_X, IDLE_SIGNAL_Y, img_add[g_nb_sig]);
	}
	else
	{
		LCD_ShowImg_From_Flash(IDLE_SIGNAL_X, IDLE_SIGNAL_Y, img_add[0]);
	}
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
	else
	{
		LCD_Fill(IDLE_NET_MODE_X, IDLE_NET_MODE_Y, IDLE_NET_MODE_W, IDLE_NET_MODE_H, BLACK);
	}
}

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
void IdleUpdateSportData(void)
{
	uint16_t bg_color = 0x00c3;
	uint8_t i,count=1;
	uint16_t steps_show;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	steps_show = last_sport.step_rec.steps;

	LCD_Fill(IDLE_STEPS_STR_X, IDLE_STEPS_STR_Y, IDLE_STEPS_STR_W, IDLE_STEPS_STR_H, bg_color);

	while(1)
	{
		if(steps_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(IDLE_STEPS_STR_X+(IDLE_STEPS_STR_W-count*IDLE_STEPS_NUM_W)/2+i*IDLE_STEPS_NUM_W, IDLE_STEPS_STR_Y, img_num[steps_show/divisor]);
		steps_show = steps_show%divisor;
		divisor = divisor/10;
	}
}

void IdleShowSportData(void)
{
	uint16_t bg_color = 0x00c3;
	uint8_t i,count=1;
	uint16_t steps_show;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	steps_show = last_sport.step_rec.steps;
	
	LCD_ShowImg_From_Flash(IDLE_STEPS_BG_X, IDLE_STEPS_BG_Y, IMG_IDLE_STEP_BG_ADDR);
	LCD_dis_pic_trans_from_flash(IDLE_STEPS_ICON_X, IDLE_STEPS_ICON_Y, IMG_IDLE_STEP_ICON_ADDR, bg_color);

	while(1)
	{
		if(steps_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(IDLE_STEPS_STR_X+(IDLE_STEPS_STR_W-count*IDLE_STEPS_NUM_W)/2+i*IDLE_STEPS_NUM_W, IDLE_STEPS_STR_Y, img_num[steps_show/divisor]);
		steps_show = steps_show%divisor;
		divisor = divisor/10;
	}
}
#endif

#ifdef CONFIG_PPG_SUPPORT
bool IsInPPGScreen(void)
{
	if(screen_id == SCREEN_ID_HR || screen_id == SCREEN_ID_SPO2 || screen_id == SCREEN_ID_BP)
		return true;
	else
		return false;
}

void IdleUpdateHrData(void)
{
	uint16_t bg_color = 0x1820;
	uint8_t i,hr_show,count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	hr_show = last_health.hr_rec.hr;
	
	LCD_Fill(IDLE_HR_STR_X, IDLE_HR_STR_Y, IDLE_HR_STR_W, IDLE_HR_STR_H, bg_color);

	while(1)
	{
		if(hr_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(IDLE_HR_STR_X+(IDLE_HR_STR_W-count*IDLE_HR_NUM_W)/2+i*IDLE_HR_NUM_W, IDLE_HR_STR_Y, img_num[hr_show/divisor]);
		hr_show = hr_show%divisor;
		divisor = divisor/10;
	}
}

void IdleShowHrData(void)
{
	uint16_t bg_color = 0x1820;
	uint8_t i,hr_show,count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	hr_show = last_health.hr_rec.hr;

	LCD_ShowImg_From_Flash(IDLE_HR_BG_X, IDLE_HR_BG_Y, IMG_IDLE_HR_BG_ADDR);
	LCD_dis_pic_trans_from_flash(IDLE_HR_ICON_X, IDLE_HR_ICON_Y, IMG_IDLE_HR_ICON_ADDR, bg_color);

	while(1)
	{
		if(hr_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(IDLE_HR_STR_X+(IDLE_HR_STR_W-count*IDLE_HR_NUM_W)/2+i*IDLE_HR_NUM_W, IDLE_HR_STR_Y, img_num[hr_show/divisor]);
		hr_show = hr_show%divisor;
		divisor = divisor/10;
	}
}

void IdleUpdateSPO2Data(void)
{
	uint16_t bg_color = 0x1820;
	uint8_t i,spo2_show,count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	spo2_show = last_health.spo2_rec.spo2;
	
	LCD_Fill(IDLE_SPO2_STR_X, IDLE_SPO2_STR_Y, IDLE_SPO2_STR_W, IDLE_SPO2_STR_H, bg_color);

	while(1)
	{
		if(spo2_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(IDLE_SPO2_STR_X+(IDLE_SPO2_STR_W-(IDLE_SPO2_PERC_W+count*IDLE_SPO2_NUM_W))/2+i*IDLE_SPO2_NUM_W, IDLE_SPO2_STR_Y, img_num[spo2_show/divisor]);
		spo2_show = spo2_show%divisor;
		divisor = divisor/10;
	}
	LCD_ShowImg_From_Flash(IDLE_SPO2_STR_X+(IDLE_SPO2_STR_W-(IDLE_SPO2_PERC_W+count*IDLE_SPO2_NUM_W))/2+i*IDLE_SPO2_NUM_W, IDLE_SPO2_STR_Y, IMG_FONT_20_PERC_ADDR);
}

void IdleShowSPO2Data(void)
{
	uint16_t bg_color = 0x1820;
	uint8_t i,spo2_show,count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	spo2_show = last_health.spo2_rec.spo2;

	LCD_ShowImg_From_Flash(IDLE_SPO2_BG_X, IDLE_SPO2_BG_Y, IMG_IDLE_SPO2_BG_ADDR);
	LCD_dis_pic_trans_from_flash(IDLE_SPO2_ICON_X, IDLE_SPO2_ICON_Y, IMG_IDLE_SPO2_ICON_ADDR, bg_color);

	while(1)
	{
		if(spo2_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(IDLE_SPO2_STR_X+(IDLE_SPO2_STR_W-(IDLE_SPO2_PERC_W+count*IDLE_SPO2_NUM_W))/2+i*IDLE_SPO2_NUM_W, IDLE_SPO2_STR_Y, img_num[spo2_show/divisor]);
		spo2_show = spo2_show%divisor;
		divisor = divisor/10;
	}
	LCD_ShowImg_From_Flash(IDLE_SPO2_STR_X+(IDLE_SPO2_STR_W-(IDLE_SPO2_PERC_W+count*IDLE_SPO2_NUM_W))/2+i*IDLE_SPO2_NUM_W, IDLE_SPO2_STR_Y, IMG_FONT_20_PERC_ADDR);
}

#endif

#ifdef CONFIG_TEMP_SUPPORT
void IdleUpdateTempData(void)
{
	uint16_t x,y,w,h;
	uint16_t bg_color = 0x00c3;
	uint8_t i,count=1;
	uint16_t temp_show;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	if(global_settings.temp_unit == TEMP_UINT_C)
	{
		LCD_dis_pic_trans_from_flash(IDLE_TEMP_ICON_X, IDLE_TEMP_ICON_Y, IMG_IDLE_TEMP_C_ICON_ADDR, bg_color);
		temp_show = last_health.temp_rec.deca_temp;
	}
	else
	{
		LCD_dis_pic_trans_from_flash(IDLE_TEMP_ICON_X, IDLE_TEMP_ICON_Y, IMG_IDLE_TEMP_F_ICON_ADDR, bg_color);
		temp_show = round((32+1.8*(last_health.temp_rec.deca_temp/10.0))*10.0);
	}

	while(1)
	{
		if(temp_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	x = IDLE_TEMP_STR_X+(IDLE_TEMP_STR_W-count*IDLE_TEMP_NUM_W-IDLE_TEMP_DOT_W)/2;
	y = IDLE_TEMP_STR_Y;

	if(count == 1)
	{
		x = IDLE_TEMP_STR_X+(IDLE_TEMP_STR_W-2*IDLE_TEMP_NUM_W-IDLE_TEMP_DOT_W)/2;
		LCD_ShowImg_From_Flash(x, y, img_num[temp_show/10]);
		x += IDLE_TEMP_NUM_W;
		LCD_ShowImg_From_Flash(x, y, IMG_FONT_20_DOT_ADDR);
		x += IDLE_TEMP_DOT_W;
		LCD_ShowImg_From_Flash(x, y, img_num[temp_show%10]);
	}
	else
	{
		for(i=0;i<(count+1);i++)
		{
			if(i == count-1)
			{
				LCD_ShowImg_From_Flash(x, y, IMG_FONT_20_DOT_ADDR);
				x += IDLE_TEMP_DOT_W;
			}
			else
			{
				LCD_ShowImg_From_Flash(x, y, img_num[temp_show/divisor]);
				temp_show = temp_show%divisor;
				divisor = divisor/10;
				x += IDLE_TEMP_NUM_W;
			}
		}	
	}
}

void IdleShowTempData(void)
{
	uint16_t x,y,w,h;
	uint16_t bg_color = 0x00c3;
	uint8_t i,count=1;
	uint16_t temp_show;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_20_NUM_0_ADDR,IMG_FONT_20_NUM_1_ADDR,IMG_FONT_20_NUM_2_ADDR,IMG_FONT_20_NUM_3_ADDR,IMG_FONT_20_NUM_4_ADDR,
							IMG_FONT_20_NUM_5_ADDR,IMG_FONT_20_NUM_6_ADDR,IMG_FONT_20_NUM_7_ADDR,IMG_FONT_20_NUM_8_ADDR,IMG_FONT_20_NUM_9_ADDR};

	LCD_ShowImg_From_Flash(IDLE_TEMP_BG_X, IDLE_TEMP_BG_Y, IMG_IDLE_TEMP_BG_ADDR);
	if(global_settings.temp_unit == TEMP_UINT_C)
	{
		LCD_dis_pic_trans_from_flash(IDLE_TEMP_ICON_X, IDLE_TEMP_ICON_Y, IMG_IDLE_TEMP_C_ICON_ADDR, bg_color);
		temp_show = last_health.temp_rec.deca_temp;
	}
	else
	{
		LCD_dis_pic_trans_from_flash(IDLE_TEMP_ICON_X, IDLE_TEMP_ICON_Y, IMG_IDLE_TEMP_F_ICON_ADDR, bg_color);
		temp_show = round((32+1.8*(last_health.temp_rec.deca_temp/10.0))*10.0);
	}

	while(1)
	{
		if(temp_show/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	x = IDLE_TEMP_STR_X+(IDLE_TEMP_STR_W-count*IDLE_TEMP_NUM_W-IDLE_TEMP_DOT_W)/2;
	y = IDLE_TEMP_STR_Y;

	if(count == 1)
	{
		x = IDLE_TEMP_STR_X+(IDLE_TEMP_STR_W-2*IDLE_TEMP_NUM_W-IDLE_TEMP_DOT_W)/2;
		LCD_ShowImg_From_Flash(x, y, img_num[temp_show/10]);
		x += IDLE_TEMP_NUM_W;
		LCD_ShowImg_From_Flash(x, y, IMG_FONT_20_DOT_ADDR);
		x += IDLE_TEMP_DOT_W;
		LCD_ShowImg_From_Flash(x, y, img_num[temp_show%10]);
	}
	else
	{
		for(i=0;i<(count+1);i++)
		{
			if(i == count-1)
			{
				LCD_ShowImg_From_Flash(x, y, IMG_FONT_20_DOT_ADDR);
				x += IDLE_TEMP_DOT_W;
			}
			else
			{
				LCD_ShowImg_From_Flash(x, y, img_num[temp_show/divisor]);
				temp_show = temp_show%divisor;
				divisor = divisor/10;
				x += IDLE_TEMP_NUM_W;
			}
		}
	}
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
	#ifdef CONFIG_BLE_SUPPORT	
		IdleShowBleStatus();
	#endif	
	#ifdef CONFIG_PPG_SUPPORT
		IdleShowHrData();
		IdleShowSPO2Data();
	#endif
	#if 0//defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
		IdleShowSportData();
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
	#ifdef CONFIG_BLE_SUPPORT	
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_BLE)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_BLE);
			IdleShowBleStatus();
		}
	#endif
	#if 0//defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
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
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SPO2)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SPO2);
			IdleUpdateSPO2Data();
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
	AnimaStop();
#endif

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);

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
	uint32_t img_anima[3] = {IMG_RUNNING_ANI_1_ADDR, IMG_RUNNING_ANI_2_ADDR, IMG_RUNNING_ANI_3_ADDR};

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(POW_OFF_RUNNING_ANI_X, POW_OFF_RUNNING_ANI_Y, img_anima, ARRAY_SIZE(img_anima), 1000, true, NULL);
#endif	
}

void PowerOffShowStatus(void)
{
	uint16_t x,y,w,h;

	LCD_Clear(BLACK);

	LCD_ShowImg_From_Flash(PWR_OFF_ICON_X, PWR_OFF_ICON_Y, IMG_PWROFF_BUTTON_ADDR);

	SetLeftKeyUpHandler(poweroff_cancel);
	SetLeftKeyLongPressHandler(poweroff_confirm);
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
	register_touch_event_handle(TP_EVENT_LONG_PRESS, PWR_OFF_ICON_X, PWR_OFF_ICON_X+PWR_OFF_ICON_W, PWR_OFF_ICON_Y, PWR_OFF_ICON_Y+PWR_OFF_ICON_H, poweroff_confirm);

	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);

 #ifdef NB_SIGNAL_TEST
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
 #elif defined(CONFIG_FACTORY_TEST_SUPPORT)
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterFTAgingTest);
 #else
  #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
  	if((strlen(g_ui_ver) == 0) 
		|| (strlen(g_font_ver) == 0)
		|| (strlen(g_ppg_algo_ver) == 0)
		)
	{
		SPIFlash_Read_DataVer(g_ui_ver, g_font_ver, g_ppg_algo_ver);
	}

   #if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
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
  #else
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
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

void SettingsUpdateStatus(void)
{
	uint8_t i,count;
	uint16_t x,y,w,h;
	uint16_t bg_clor = 0x2124;
	uint16_t green_clor = 0x07e0;
	
	k_timer_stop(&mainmenu_timer);

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
			uint16_t lang_sle_str[LANGUAGE_MAX][10] = {
													#ifndef FW_FOR_CN
														{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
														{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
														{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
														{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
														{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
														{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Português
													#else
														{0x4E2D,0x6587,0x0000},//中文
														{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
													#endif	
													};
			uint16_t menu_sle_str[LANGUAGE_MAX][3][14] = {
														#ifndef FW_FOR_CN
															{
																{0x0056,0x0069,0x0065,0x0077,0x0000},//View
																{0x0000},//null
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0000},//Reset
															},
															{
																{0x00DC,0x0062,0x0065,0x0072,0x0070,0x0072,0x00FC,0x0066,0x0065,0x006E,0x0000},//?berprüfen
																{0x0000},//null
																{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0000},//Zurücksetzen
															},
															{
																{0x0056,0x006F,0x0069,0x0072,0x0000},//Voir
																{0x0000},//null
																{0x0052,0x00E9,0x0069,0x006E,0x0069,0x0074,0x0069,0x0061,0x006C,0x0069,0x0073,0x0065,0x0072,0x0000},//Réinitialiser
															},
															{
																{0x0056,0x0069,0x0073,0x0074,0x0061,0x0000},//Vista
																{0x0000},//null
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0000},//Reset
															},
															{
																{0x0056,0x0065,0x0072,0x0000},//Ver
																{0x0000},//null
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0000},//Reset
															},
															{
																{0x0056,0x0065,0x0072,0x0000},//Ver
																{0x0000},//null
																{0x0052,0x0065,0x0064,0x0065,0x0066,0x0069,0x006E,0x0069,0x0072,0x0000},//Redefinir
															},
														#else	
															{
																{0x67E5,0x770B,0x0000},//查看
																{0x0000},//空白
																{0x91CD,0x7F6E,0x0000},//重置
															},
															{
																{0x0056,0x0069,0x0065,0x0077,0x0000},//View
																{0x0000},//null
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0000},//Reset
															},
														#endif	
														  };
			uint16_t level_str[LANGUAGE_MAX][4][10] = {
													#ifndef FW_FOR_CN
														{
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000},//Level 1
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000},//Level 2
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000},//Level 3
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000},//Level 4
														},
														{
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000},//Level 1
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000},//Level 2
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000},//Level 3
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000},//Level 4
														},
														{
															{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0031,0x0000},//Niveau 1
															{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0032,0x0000},//Niveau 2
															{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0033,0x0000},//Niveau 3
															{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0034,0x0000},//Niveau 4
														},
														{
															{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0031,0x0000},//Livello 1
															{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0032,0x0000},//Livello 2
															{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0033,0x0000},//Livello 3
															{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0034,0x0000},//Livello 4
														},
														{
															{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0031,0x0000},//Nivel 1
															{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0032,0x0000},//Nivel 2
															{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0033,0x0000},//Nivel 3
															{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0034,0x0000},//Nivel 4
														},
														{
															{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0031,0x0000},//Nível 1
															{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0032,0x0000},//Nível 2
															{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0033,0x0000},//Nível 3
															{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0034,0x0000},//Nível 4
														},
													#else	
														{
															{0x7B49,0x7EA7,0x0031,0x0000},//等级1
															{0x7B49,0x7EA7,0x0032,0x0000},//等级2
															{0x7B49,0x7EA7,0x0033,0x0000},//等级3
															{0x7B49,0x7EA7,0x0034,0x0000},//等级4
														},
														{
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000},//Level 1
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000},//Level 2
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000},//Level 3
															{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000},//Level 4
														},
													#endif
													};

			uint32_t img_addr[2] = {IMG_SET_TEMP_UNIT_C_ICON_ADDR, IMG_SET_TEMP_UNIT_F_ICON_ADDR};

			entry_setting_bk_flag = false;
			
			LCD_Clear(BLACK);
			
			for(i=0;i<SETTINGS_MAIN_MENU_MAX_PER_PG;i++)
			{
				uint16_t tmpbuf[128] = {0};

				if((settings_menu.index + i) >= settings_menu.count)
					break;
				
				LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);
				
			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(WHITE);
			
				mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)settings_menu.name[global_settings.language][i+settings_menu.index], MENU_NAME_STR_MAX);
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
									SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
									tmpbuf);

				LCD_SetFontColor(green_clor);
				switch(settings_menu.index)
				{
				case 0:
					switch(i)
					{
					case 0:
						memset(tmpbuf, 0, sizeof(tmpbuf));
						mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)lang_sle_str[global_settings.language], MENU_OPT_STR_MAX);
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
												SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
												tmpbuf);
						break;
					case 1:
						memset(tmpbuf, 0, sizeof(tmpbuf));
						mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)level_str[global_settings.language][global_settings.backlight_level], MENU_OPT_STR_MAX+2);
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
												SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
												tmpbuf);
						break;
					case 2:
						LCD_ShowImg_From_Flash(SETTINGS_MENU_TEMP_UNIT_X, SETTINGS_MENU_TEMP_UNIT_Y, img_addr[global_settings.temp_unit]);
						break;
					}
					break;
				case 3:
					if(i == 1)
					{
						LCD_ShowImg_From_Flash(SETTINGS_MENU_QR_ICON_X, SETTINGS_MENU_QR_ICON_Y, IMG_SET_QR_ICON_ADDR);
					}
					else
					{
						memset(tmpbuf, 0, sizeof(tmpbuf));
						mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)menu_sle_str[global_settings.language][i], MENU_OPT_STR_MAX);
						LCD_MeasureUniString(tmpbuf, &w, &h);
						LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
											tmpbuf);
					}
					break;
				case 6:
					memset(tmpbuf, 0, sizeof(tmpbuf));
					mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)menu_sle_str[global_settings.language][i], MENU_OPT_STR_MAX);
					LCD_MeasureUniString(tmpbuf, &w, &h);
					LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
												SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
												tmpbuf);
					break;
				}
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_BG_X, 
											SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W, 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_BG_H, 
											settings_menu.sel_handler[i+SETTINGS_MAIN_MENU_MAX_PER_PG*(settings_menu.index/SETTINGS_MAIN_MENU_MAX_PER_PG)]);
			
			#endif
			}

		#ifdef CONFIG_TOUCH_SUPPORT
		 #ifdef NB_SIGNAL_TEST
			register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
		 #elif defined(CONFIG_FACTORY_TEST_SUPPORT)
			register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterFactoryTest);
		 #else
		  #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
			if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
			{
				register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
			}
			else if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
			{
				register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
			}
		   #if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
			else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
			{
				register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
			}
		   #endif
			else
		  #endif/*CONFIG_DATA_DOWNLOAD_SUPPORT*/		
			{
				register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
			}	
		 #endif/*NB_SIGNAL_TEST*/

		 #ifdef NB_SIGNAL_TEST
		  #ifdef CONFIG_WIFI_SUPPORT
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterWifiTestScreen);
		  #else
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterGPSTestScreen);
		  #endif
		 #elif defined(CONFIG_FACTORY_TEST_SUPPORT)
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
		 #elif defined(CONFIG_SYNC_SUPPORT)
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
		 #elif defined(CONFIG_PPG_SUPPORT)
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
		 #elif defined(CONFIG_TEMP_SUPPORT)
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
		 #elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
		  #ifdef CONFIG_SLEEP_SUPPORT
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
		  #elif defined(CONFIG_STEP_SUPPORT)
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
		  #endif
		 #else
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
		 #endif/*NB_SIGNAL_TEST*/
		#endif/*CONFIG_TOUCH_SUPPORT*/
		}
		break;
		
	case SETTINGS_MENU_LANGUAGE:
		{
			LCD_Clear(BLACK);
			
			if(settings_menu.count > SETTINGS_SUB_MENU_MAX_PER_PG)
				count = (settings_menu.count - settings_menu.index >= SETTINGS_SUB_MENU_MAX_PER_PG) ? SETTINGS_SUB_MENU_MAX_PER_PG : settings_menu.count - settings_menu.index;
			else
				count = settings_menu.count;
			
			for(i=0;i<count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

				if((i+settings_menu.index) == global_settings.language)
					LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_YES_ADDR);
				else
					LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_NO_ADDR);

			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(WHITE);
				LCD_MeasureUniString(settings_menu.name[global_settings.language][i+settings_menu.index], &w, &h);
				LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
										SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
										settings_menu.name[global_settings.language][i+settings_menu.index]);
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_BG_X, 
											SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W, 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_BG_H, 
											settings_menu.sel_handler[i+settings_menu.index]);
			#endif
			}
		}
		break;
		
	case SETTINGS_MENU_FACTORY_RESET:
		{
			uint16_t tmpbuf[128] = {0};
			
			LCD_Clear(BLACK);
			LCD_SetFontBgColor(BLACK);
			
			switch(g_reset_status)
			{
			case RESET_STATUS_IDLE:
				{
					uint16_t str_ready[LANGUAGE_MAX][44] = {
															#ifndef FW_FOR_CN
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0020,0x0074,0x006F,0x0020,0x0074,0x0068,0x0065,0x0020,0x0066,0x0061,0x0063,0x0074,0x006F,0x0072,0x0079,0x0020,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0073,0x0000},//Reset to the factory settings
																{0x0041,0x0075,0x0066,0x0020,0x0064,0x0069,0x0065,0x0020,0x0057,0x0065,0x0072,0x006B,0x0073,0x0065,0x0069,0x006E,0x0073,0x0074,0x0065,0x006C,0x006C,0x0075,0x006E,0x0067,0x0065,0x006E,0x0020,0x007A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0000},//Auf die Werkseinstellungen zurücksetzen
																{0x0052,0x00E9,0x0069,0x006E,0x0069,0x0074,0x0069,0x0061,0x006C,0x0069,0x0073,0x0065,0x0072,0x0020,0x006C,0x0065,0x0073,0x0020,0x0070,0x0061,0x0072,0x0061,0x006D,0x00E8,0x0074,0x0072,0x0065,0x0073,0x0020,0x0064,0x0027,0x0075,0x0073,0x0069,0x006E,0x0065,0x0000},//Réinitialiser les paramètres d'usine
																{0x0052,0x0069,0x0070,0x0072,0x0069,0x0073,0x0074,0x0069,0x006E,0x0061,0x0020,0x006C,0x0065,0x0020,0x0069,0x006D,0x0070,0x006F,0x0073,0x0074,0x0061,0x007A,0x0069,0x006F,0x006E,0x0069,0x0020,0x0064,0x0069,0x0020,0x0066,0x0061,0x0062,0x0062,0x0072,0x0069,0x0063,0x0061,0x0000},//Ripristina le impostazioni di fabbrica
																{0x0052,0x0065,0x0073,0x0074,0x0061,0x0062,0x006C,0x0065,0x0063,0x0065,0x0072,0x0020,0x006C,0x006F,0x0073,0x0020,0x0076,0x0061,0x006C,0x006F,0x0072,0x0065,0x0073,0x0020,0x0064,0x0065,0x0020,0x0066,0x00E1,0x0062,0x0072,0x0069,0x0063,0x0061,0x0000},//Restablecer los valores de fábrica
																{0x0052,0x0065,0x0064,0x0065,0x0066,0x0069,0x006E,0x0069,0x0072,0x0020,0x0070,0x0061,0x0072,0x0061,0x0020,0x0061,0x0073,0x0020,0x0063,0x006F,0x006E,0x0066,0x0069,0x0067,0x0075,0x0072,0x0061,0x00E7,0x00F5,0x0065,0x0073,0x0020,0x0064,0x0065,0x0020,0x0066,0x00E1,0x0062,0x0072,0x0069,0x0063,0x0061,0x0000},//Redefinir para as configura??es de fábrica
															#else
																{0x6062,0x590D,0x5230,0x51FA,0x5382,0x8BBE,0x7F6E,0x0000},//恢复到出厂设置
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0020,0x0074,0x006F,0x0020,0x0074,0x0068,0x0065,0x0020,0x0066,0x0061,0x0063,0x0074,0x006F,0x0072,0x0079,0x0020,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0073,0x0000},//Reset to the factory settings
															#endif	
															};
					
					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_ICON_X, SETTINGS_MENU_RESET_ICON_Y, IMG_RESET_LOGO_ADDR);
					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_NO_X, SETTINGS_MENU_RESET_NO_Y, IMG_RESET_NO_ADDR);
					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_YES_X, SETTINGS_MENU_RESET_YES_Y, IMG_RESET_YES_ADDR);

					mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_ready[global_settings.language], MENU_NOTIFY_STR_MAX);
					LCD_MeasureUniString(tmpbuf, &w, &h);
					LCD_ShowUniString(SETTINGS_MENU_RESET_STR_X+(SETTINGS_MENU_RESET_STR_W-w)/2, 
										SETTINGS_MENU_RESET_STR_Y+(SETTINGS_MENU_RESET_STR_H-h)/2, 
										tmpbuf);
				
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
					uint16_t str_running[LANGUAGE_MAX][28] = {
															#ifndef FW_FOR_CN
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0020,0x0069,0x006E,0x0020,0x0070,0x0072,0x006F,0x0067,0x0072,0x0065,0x0073,0x0073,0x0000},//Resetting in progress
																{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0075,0x006E,0x0067,0x0020,0x006C,0x00E4,0x0075,0x0066,0x0074,0x0000},//Zurücksetzung l?uft
																{0x0052,0x00E9,0x0069,0x006E,0x0069,0x0074,0x0069,0x0061,0x006C,0x0069,0x0073,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x0065,0x006E,0x0020,0x0063,0x006F,0x0075,0x0072,0x0073,0x0000},//Réinitialisation en cours
																{0x0052,0x0069,0x0070,0x0072,0x0069,0x0073,0x0074,0x0069,0x006E,0x0061,0x0020,0x0069,0x006E,0x0020,0x0063,0x006F,0x0072,0x0073,0x006F,0x0000},//Ripristina in corso
																{0x0052,0x0065,0x0069,0x006E,0x0069,0x0063,0x0069,0x0061,0x006C,0x0069,0x007A,0x0061,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0065,0x006E,0x0020,0x0063,0x0075,0x0072,0x0073,0x006F,0x0000},//Reinicialización en curso
																{0x0052,0x0065,0x0064,0x0065,0x0066,0x0069,0x006E,0x0069,0x00E7,0x00E3,0x006F,0x0020,0x0065,0x006D,0x0020,0x0061,0x006E,0x0064,0x0061,0x006D,0x0065,0x006E,0x0074,0x006F,0x0000},//Redefini??o em andamento
															#else
																{0x91CD,0x7F6E,0x8FDB,0x884C,0x4E2D,0x0000},//重置进行中
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0074,0x0069,0x006E,0x0067,0x0020,0x0069,0x006E,0x0020,0x0070,0x0072,0x006F,0x0067,0x0072,0x0065,0x0073,0x0073,0x0000},//Resetting in progress
															#endif
															  };
					uint32_t img_addr[8] = {
											IMG_RESET_ANI_1_ADDR,
											IMG_RESET_ANI_2_ADDR,
											IMG_RESET_ANI_3_ADDR,
											IMG_RESET_ANI_4_ADDR,
											IMG_RESET_ANI_5_ADDR,
											IMG_RESET_ANI_6_ADDR,
											IMG_RESET_ANI_7_ADDR,
											IMG_RESET_ANI_8_ADDR
										};

					mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_running[global_settings.language], MENU_NOTIFY_STR_MAX);
					LCD_MeasureUniString(tmpbuf, &w, &h);
					LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, 
										SETTINGS_MENU_RESET_NOTIFY_Y+(SETTINGS_MENU_RESET_NOTIFY_H-h)/2, 
										tmpbuf);

				#ifdef CONFIG_ANIMATION_SUPPORT
					AnimaShow(SETTINGS_MENU_RESET_LOGO_X, SETTINGS_MENU_RESET_LOGO_Y, img_addr, ARRAY_SIZE(img_addr), 300, true, NULL);
				#endif	
				}
				break;
				
			case RESET_STATUS_SUCCESS:
				{
					uint16_t str_success[LANGUAGE_MAX][26] = {
															#ifndef FW_FOR_CN
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0020,0x0053,0x0075,0x0063,0x0063,0x0065,0x0073,0x0073,0x0066,0x0075,0x006C,0x0000},//Reset Successful
																{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0020,0x0065,0x0072,0x0066,0x006F,0x006C,0x0067,0x0072,0x0065,0x0069,0x0063,0x0068,0x0000},//Zurücksetzen erfolgreich
																{0x0052,0x00E9,0x0069,0x006E,0x0069,0x0074,0x0069,0x0061,0x006C,0x0069,0x0073,0x0065,0x0072,0x0020,0x006C,0x0065,0x0020,0x0073,0x0075,0x0063,0x0063,0x00E8,0x0073,0x0000},//Réinitialiser le succès
																{0x0052,0x0069,0x0070,0x0072,0x0069,0x0073,0x0074,0x0069,0x006E,0x006F,0x0020,0x0072,0x0069,0x0075,0x0073,0x0063,0x0069,0x0074,0x006F,0x0000},//Ripristino riuscito
																{0x0052,0x0065,0x0073,0x0074,0x0061,0x0062,0x006C,0x0065,0x0063,0x0069,0x006D,0x0069,0x0065,0x006E,0x0074,0x006F,0x0020,0x00E9,0x0078,0x0069,0x0074,0x006F,0x0000},//Restablecimiento éxito
																{0x0052,0x0065,0x0064,0x0065,0x0066,0x0069,0x006E,0x0069,0x0072,0x0020,0x0062,0x0065,0x006D,0x0020,0x002D,0x0073,0x0075,0x0063,0x0065,0x0064,0x0069,0x0064,0x006F,0x0000},//Redefinir bem -sucedido
															#else
																{0x51FA,0x5382,0x8BBE,0x7F6E,0x6062,0x590D,0x6210,0x529F,0x0000},//出厂设置恢复成功
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0020,0x0053,0x0075,0x0063,0x0063,0x0065,0x0073,0x0073,0x0066,0x0075,0x006C,0x0000},//Reset Successful
															#endif
															  };
					
				#ifdef CONFIG_ANIMATION_SUPPORT
					AnimaStop();
				#endif

					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_LOGO_X, SETTINGS_MENU_RESET_LOGO_Y, IMG_RESET_SUCCESS_ADDR);

					mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_success[global_settings.language], MENU_NOTIFY_STR_MAX);
					LCD_MeasureUniString(tmpbuf, &w, &h);
					LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, 
										SETTINGS_MENU_RESET_NOTIFY_Y+(SETTINGS_MENU_RESET_NOTIFY_H-h)/2, 
										tmpbuf);
				}
				break;
				
			case RESET_STATUS_FAIL:
				{
					uint16_t str_fail[LANGUAGE_MAX][30] = {
															#ifndef FW_FOR_CN
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0020,0x0046,0x0061,0x0069,0x006C,0x0065,0x0064,0x0000},//Reset Failed
																{0x005A,0x0075,0x0072,0x00FC,0x0063,0x006B,0x0073,0x0065,0x0074,0x007A,0x0065,0x006E,0x0020,0x0066,0x0065,0x0068,0x006C,0x0067,0x0065,0x0073,0x0063,0x0068,0x006C,0x0061,0x0067,0x0065,0x006E,0x0000},//Zurücksetzen fehlgeschlagen
																{0x0052,0x00E9,0x0069,0x006E,0x0069,0x0074,0x0069,0x0061,0x006C,0x0069,0x0073,0x0065,0x0072,0x0020,0x0061,0x0020,0x00E9,0x0063,0x0068,0x006F,0x0075,0x00E9,0x0000},//Réinitialiser a échoué
																{0x0052,0x0069,0x0070,0x0072,0x0069,0x0073,0x0074,0x0069,0x006E,0x006F,0x0020,0x0066,0x0061,0x006C,0x006C,0x0069,0x0074,0x006F,0x0000},//Ripristino fallito
																{0x0052,0x0065,0x0073,0x0074,0x0061,0x0062,0x006C,0x0065,0x0063,0x0069,0x006D,0x0069,0x0065,0x006E,0x0074,0x006F,0x0020,0x0066,0x0061,0x006C,0x006C,0x0069,0x0064,0x006F,0x0000},//Restablecimiento fallido
																{0x0046,0x0061,0x006C,0x0068,0x0061,0x0020,0x006E,0x0061,0x0020,0x0072,0x0065,0x0064,0x0065,0x0066,0x0069,0x006E,0x0069,0x00E7,0x00E3,0x006F,0x0000},//Falha na redefini??o
															#else
																{0x51FA,0x5382,0x8BBE,0x7F6E,0x6062,0x590D,0x5931,0x8D25,0x0000},//出厂设置恢复失败
																{0x0052,0x0065,0x0073,0x0065,0x0074,0x0020,0x0046,0x0061,0x0069,0x006C,0x0065,0x0064,0x0000},//Reset Failed
															#endif
														   };

				#ifdef CONFIG_ANIMATION_SUPPORT
					AnimaStop();
				#endif

					LCD_ShowImg_From_Flash(SETTINGS_MENU_RESET_LOGO_X, SETTINGS_MENU_RESET_LOGO_Y, IMG_RESET_FAIL_ADDR);

					mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_fail[global_settings.language], MENU_NOTIFY_STR_MAX);
					LCD_MeasureUniString(tmpbuf, &w, &h);
					LCD_ShowUniString(SETTINGS_MENU_RESET_NOTIFY_X+(SETTINGS_MENU_RESET_NOTIFY_W-w)/2, 
										SETTINGS_MENU_RESET_NOTIFY_Y+(SETTINGS_MENU_RESET_NOTIFY_H-h)/2, 
										tmpbuf);
				}
				break;
			}
		}
		break;
		
	case SETTINGS_MENU_OTA:
		{
			uint16_t tmpbuf[128] = {0};
			uint16_t str_notify[LANGUAGE_MAX][28] = {
													#ifndef FW_FOR_CN
														{0x0049,0x0074,0x0020,0x0069,0x0073,0x0020,0x0074,0x0068,0x0065,0x0020,0x006C,0x0061,0x0074,0x0065,0x0073,0x0074,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0000},//It is the latest version
														{0x0045,0x0073,0x0020,0x0069,0x0073,0x0074,0x0020,0x0064,0x0069,0x0065,0x0020,0x006E,0x0065,0x0075,0x0065,0x0073,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0000},//Es ist die neueste version
														{0x0043,0x0027,0x0065,0x0073,0x0074,0x0020,0x006C,0x0061,0x0020,0x0064,0x0065,0x0072,0x006E,0x0069,0x00E8,0x0072,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0000},//C'est la dernière version
														{0x00C8,0x0020,0x006C,0x0027,0x0075,0x006C,0x0074,0x0069,0x006D,0x0061,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0065,0x0000},//? l'ultima versione
														{0x0045,0x0073,0x0020,0x006C,0x0061,0x0020,0x00FA,0x006C,0x0074,0x0069,0x006D,0x0061,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0000},//Es la última version
														{0x00C9,0x0020,0x0061,0x0020,0x0076,0x0065,0x0072,0x0073,0x00E3,0x006F,0x0020,0x006D,0x0061,0x0069,0x0073,0x0020,0x0072,0x0065,0x0063,0x0065,0x006E,0x0074,0x0065,0x0000},//? a vers?o mais recente
													#else
														{0x5DF2,0x662F,0x6700,0x65B0,0x7248,0x672C,0x0000},//已是最新版本
														{0x0049,0x0074,0x0020,0x0069,0x0073,0x0020,0x0074,0x0068,0x0065,0x0020,0x006C,0x0061,0x0074,0x0065,0x0073,0x0074,0x0020,0x0076,0x0065,0x0072,0x0073,0x0069,0x006F,0x006E,0x0000},//It is the latest version
													#endif
													};
			
			LCD_Clear(BLACK);
			LCD_SetFontBgColor(BLACK);

			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_notify[global_settings.language], MENU_NOTIFY_STR_MAX);
			LCD_MeasureUniString(tmpbuf, &w, &h);
			LCD_ShowUniString((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, tmpbuf);

		#ifdef CONFIG_TOUCH_SUPPORT
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
										0, 
										LCD_WIDTH, 
										0, 
										LCD_HEIGHT, 
										settings_menu.sel_handler[0]);
		#endif	
		}
		break;
		
	case SETTINGS_MENU_BRIGHTNESS:
		{
			uint32_t img_addr[4] = {IMG_BKL_LEVEL_1_ADDR,IMG_BKL_LEVEL_2_ADDR,IMG_BKL_LEVEL_3_ADDR,IMG_BKL_LEVEL_4_ADDR};

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
		}
		break;
		
	case SETTINGS_MENU_TEMP:
		{
			LCD_Clear(BLACK);
			
			for(i=0;i<settings_menu.count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

				if(i == global_settings.temp_unit)
					LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_YES_ADDR);
				else
					LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X+SETTINGS_MENU_SEL_DOT_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_SEL_DOT_Y, IMG_SELECT_ICON_NO_ADDR);
				
			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(WHITE);
				LCD_MeasureUniString(settings_menu.name[global_settings.language][i], &w, &h);
				LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
										SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
										settings_menu.name[global_settings.language][i]);
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
		}
		break;
		
	case SETTINGS_MENU_DEVICE:
		{
			uint16_t imei_str[IMEI_MAX_LEN+1] = {0};
			uint16_t imsi_str[IMSI_MAX_LEN+1] = {0};
			uint16_t mcu_str[20] = {0x0000};
		#ifdef CONFIG_FACTORY_TEST_SUPPORT
			uint16_t iccid_str[ICCID_MAX_LEN+1] = {0};
			uint16_t modem_str[20] = {0x0000};	
			uint16_t ppg_str[20] = {0x0000};
			uint16_t wifi_str[20] = {0x0000};
			uint16_t ble_str[20] = {0x0000};
			uint16_t ble_mac_str[64] = {0};
			uint16_t *menu_sle_str[9] = {imei_str,imsi_str,iccid_str,mcu_str,modem_str,ppg_str,wifi_str,ble_str,ble_mac_str};
		#else
			uint16_t *menu_sle_str[3] = {imei_str,imsi_str,mcu_str};
		#endif
			uint16_t menu_color = 0x9CD3;

			LCD_Clear(BLACK);

			mmi_asc_to_ucs2((uint8_t*)imei_str, g_imei);
			mmi_asc_to_ucs2((uint8_t*)imsi_str, g_imsi);
			mmi_asc_to_ucs2((uint8_t*)mcu_str, g_fw_version);
		#ifdef CONFIG_FACTORY_TEST_SUPPORT
			mmi_asc_to_ucs2((uint8_t*)iccid_str, g_iccid);
			mmi_asc_to_ucs2((uint8_t*)modem_str, &g_modem[12]);	
		  #ifdef CONFIG_PPG_SUPPORT	
			mmi_asc_to_ucs2((uint8_t*)ppg_str, g_ppg_ver);
		  #else
			mmi_asc_to_ucs2((uint8_t*)ppg_str, "NO");
		  #endif
		  #ifdef CONFIG_WIFI_SUPPORT
			mmi_asc_to_ucs2((uint8_t*)wifi_str, g_wifi_ver);
		  #else
			mmi_asc_to_ucs2((uint8_t*)wifi_str, "NO");
		  #endif
			mmi_asc_to_ucs2((uint8_t*)ble_str, &g_nrf52810_ver[15]);
			mmi_asc_to_ucs2((uint8_t*)ble_mac_str, g_ble_mac_addr);
		#endif
		
			if(settings_menu.count > SETTINGS_SUB_MENU_MAX_PER_PG)
				count = (settings_menu.count - settings_menu.index >= SETTINGS_SUB_MENU_MAX_PER_PG) ? SETTINGS_SUB_MENU_MAX_PER_PG : settings_menu.count - settings_menu.index;
			else
				count = settings_menu.count;
			
			for(i=0;i<count;i++)
			{
				LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

			#ifdef FONTMAKER_UNICODE_FONT
				LCD_SetFontColor(menu_color);
				LCD_SetFontSize(FONT_SIZE_20);
				LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
										SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y-5,
										settings_menu.name[global_settings.language][i+settings_menu.index]);

				LCD_SetFontColor(WHITE);
			  #ifdef CONFIG_FACTORY_TEST_SUPPORT	
				if((settings_menu.index == 0) && (i == 2))
					LCD_SetFontSize(FONT_SIZE_16);
			  #endif	
				LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
										SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_STR_OFFSET_Y+15,
										menu_sle_str[i+settings_menu.index]);
			#endif

			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
											SETTINGS_MENU_BG_X, 
											SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W, 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), 
											SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+SETTINGS_MENU_BG_H, 
											settings_menu.sel_handler[i+settings_menu.index]);
			#endif
			}
		}
		break;
		
	case SETTINGS_MENU_CAREMATE_QR:
		{
			uint16_t tmpbuf[128] = {0};
			uint16_t str_notify[LANGUAGE_MAX][35] = {
													#ifndef FW_FOR_CN
														{0x0049,0x0074,0x0020,0x0069,0x0073,0x0020,0x0062,0x0065,0x0069,0x006E,0x0067,0x0020,0x0064,0x0065,0x0076,0x0065,0x006C,0x006F,0x0070,0x0065,0x0064,0x002E,0x0000},//It is being developed.
														{0x0045,0x0073,0x0020,0x0077,0x0069,0x0072,0x0064,0x0020,0x0065,0x006E,0x0074,0x0077,0x0069,0x0063,0x006B,0x0065,0x006C,0x0074,0x002E,0x0000},//Es wird entwickelt.
														{0x0049,0x006C,0x0020,0x0065,0x0073,0x0074,0x0020,0x0065,0x006E,0x0020,0x0063,0x006F,0x0075,0x0072,0x0073,0x0020,0x0064,0x0065,0x0020,0x0064,0x00E9,0x0076,0x0065,0x006C,0x006F,0x0070,0x0070,0x0065,0x006D,0x0065,0x006E,0x0074,0x002E,0x0000},//Il est en cours de développement.
														{0x00C8,0x0020,0x0069,0x006E,0x0020,0x0066,0x0061,0x0073,0x0065,0x0020,0x0064,0x0069,0x0020,0x0073,0x0076,0x0069,0x006C,0x0075,0x0070,0x0070,0x006F,0x002E,0x0000},//? in fase di sviluppo.
														{0x0053,0x0065,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0064,0x0065,0x0073,0x0061,0x0072,0x0072,0x006F,0x006C,0x006C,0x0061,0x006E,0x0064,0x006F,0x002E,0x0000},//Se está desarrollando.
														{0x0045,0x0073,0x0074,0x00E1,0x0020,0x0073,0x0065,0x006E,0x0064,0x006F,0x0020,0x0064,0x0065,0x0073,0x0065,0x006E,0x0076,0x006F,0x006C,0x0076,0x0069,0x0064,0x006F,0x002E,0x0000},//Está sendo desenvolvido.
													#else
														{0x6B64,0x529F,0x80FD,0x6B63,0x5728,0x5F00,0x53D1,0x3002,0x0000},//此功能正在开发。
														{0x0049,0x0074,0x0020,0x0069,0x0073,0x0020,0x0062,0x0065,0x0069,0x006E,0x0067,0x0020,0x0064,0x0065,0x0076,0x0065,0x006C,0x006F,0x0070,0x0065,0x0064,0x002E,0x0000},//It is being developed.
													#endif
													}; 
			LCD_Clear(BLACK);
			LCD_ReSetFontBgColor();
			LCD_ReSetFontColor();

		#ifdef CONFIG_QRCODE_SUPPORT
			show_QR_code(strlen(SETTINGS_CAREMATE_URL), SETTINGS_CAREMATE_URL);
		#else
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_notify[global_settings.language], MENU_NOTIFY_STR_MAX);
			LCD_MeasureUniString(tmpbuf, &w, &h);
			LCD_ShowUniString((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, tmpbuf);
		#endif/*CONFIG_QRCODE_SUPPORT*/
		
		#ifdef CONFIG_TOUCH_SUPPORT
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.sel_handler[0]);
			register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.sel_handler[0]);
			register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.sel_handler[0]);
			register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.sel_handler[0]);
			register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.sel_handler[0]);
		#endif
		}
		break;
	}

	if(settings_menu.count > SETTINGS_MAIN_MENU_MAX_PER_PG)
	{		
		if(settings_menu.count > 3*SETTINGS_MAIN_MENU_MAX_PER_PG)
		{
			uint32_t page4_img[4] = {IMG_SET_PG4_1_ADDR,IMG_SET_PG4_2_ADDR,IMG_SET_PG4_3_ADDR,IMG_SET_PG4_4_ADDR};
			
			LCD_ShowImg_From_Flash(SETTINGS_MEUN_PAGE_DOT_X, SETTINGS_MEUN_PAGE_DOT_Y, page4_img[settings_menu.index/SETTINGS_MAIN_MENU_MAX_PER_PG]);
		}
		else if(settings_menu.count > 2*SETTINGS_MAIN_MENU_MAX_PER_PG)
		{
			uint32_t page3_img[3] = {IMG_SET_PG3_1_ADDR,IMG_SET_PG3_2_ADDR,IMG_SET_PG3_3_ADDR};
			
			LCD_ShowImg_From_Flash(SETTINGS_MEUN_PAGE_DOT_X, SETTINGS_MEUN_PAGE_DOT_Y, page3_img[settings_menu.index/SETTINGS_MAIN_MENU_MAX_PER_PG]);
		}
		else
		{
			uint32_t page2_img[2] = {IMG_SET_PG2_1_ADDR,IMG_SET_PG2_2_ADDR};
			
			LCD_ShowImg_From_Flash(SETTINGS_MEUN_PAGE_DOT_X, SETTINGS_MEUN_PAGE_DOT_Y, page2_img[settings_menu.index/SETTINGS_MAIN_MENU_MAX_PER_PG]);
		}

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[0]);
		register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[1]);
	#endif		
	}

	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();

#ifndef CONFIG_FACTORY_TEST_SUPPORT
	k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
#endif
}

void SettingsShowStatus(void)
{
	uint16_t i,x,y,w,h;
	uint16_t menu_sle_str[LANGUAGE_MAX][11] = {
											#ifndef FW_FOR_CN
												{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
												{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
												{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
												{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
												{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
												{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Português
											#else
												{0x4E2D,0x6587,0x0000},//中文
												{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
											#endif	
											};
	uint16_t level_str[LANGUAGE_MAX][4][11] = {
											#ifndef FW_FOR_CN
												{
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000},//Level 1
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000},//Level 2
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000},//Level 3
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000},//Level 4
												},
												{
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000},//Level 1
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000},//Level 2
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000},//Level 3
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000},//Level 4
												},
												{
													{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0031,0x0000},//Niveau 1
													{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0032,0x0000},//Niveau 2
													{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0033,0x0000},//Niveau 3
													{0x004E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0034,0x0000},//Niveau 4
												},
												{
													{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0031,0x0000},//Livello 1
													{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0032,0x0000},//Livello 2
													{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0033,0x0000},//Livello 3
													{0x004C,0x0069,0x0076,0x0065,0x006C,0x006C,0x006F,0x0020,0x0034,0x0000},//Livello 4
												},
												{
													{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0031,0x0000},//Nivel 1
													{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0032,0x0000},//Nivel 2
													{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0033,0x0000},//Nivel 3
													{0x004E,0x0069,0x0076,0x0065,0x006C,0x0020,0x0034,0x0000},//Nivel 4
												},
												{
													{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0031,0x0000},//Nível 1
													{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0032,0x0000},//Nível 2
													{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0033,0x0000},//Nível 3
													{0x004E,0x00ED,0x0076,0x0065,0x006C,0x0020,0x0034,0x0000},//Nível 4
												},
											#else	
												{
													{0x7B49,0x7EA7,0x0031,0x0000},//等级1
													{0x7B49,0x7EA7,0x0032,0x0000},//等级2
													{0x7B49,0x7EA7,0x0033,0x0000},//等级3
													{0x7B49,0x7EA7,0x0034,0x0000},//等级4
												},
												{
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0031,0x0000},//Level 1
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0032,0x0000},//Level 2
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0033,0x0000},//Level 3
													{0x004c,0x0065,0x0076,0x0065,0x006c,0x0020,0x0034,0x0000},//Level 4
												},
											#endif
											};
	uint32_t img_addr[2] = {IMG_SET_TEMP_UNIT_C_ICON_ADDR, IMG_SET_TEMP_UNIT_F_ICON_ADDR};
	uint16_t bg_clor = 0x2124;
	uint16_t green_clor = 0x07e0;
	
	LCD_Clear(BLACK);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	LCD_SetFontBgColor(bg_clor);

	for(i=0;i<SETTINGS_MAIN_MENU_MAX_PER_PG;i++)
	{
		uint16_t tmpbuf[128] = {0};
		
		LCD_ShowImg_From_Flash(SETTINGS_MENU_BG_X, SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y), IMG_SET_INFO_BG_ADDR);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontColor(WHITE);

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)settings_menu.name[global_settings.language][i], MENU_NAME_STR_MAX);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_STR_OFFSET_X,
							SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
							tmpbuf);

		LCD_SetFontColor(green_clor);
		switch(i)
		{
		case 0:
			LCD_MeasureUniString(menu_sle_str[global_settings.language], &w, &h);
			LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
									SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
									menu_sle_str[global_settings.language]);
			break;
		case 1:
			LCD_MeasureUniString(level_str[global_settings.language][global_settings.backlight_level], &w, &h);
			LCD_ShowUniString(SETTINGS_MENU_BG_X+SETTINGS_MENU_BG_W-SETTINGS_MENU_STR_OFFSET_X-w,
									SETTINGS_MENU_BG_Y+i*(SETTINGS_MENU_BG_H+SETTINGS_MENU_BG_OFFSET_Y)+(SETTINGS_MENU_BG_H-h)/2,
									level_str[global_settings.language][global_settings.backlight_level]);
			break;
		case 2:
			LCD_ShowImg_From_Flash(SETTINGS_MENU_TEMP_UNIT_X, SETTINGS_MENU_TEMP_UNIT_Y, img_addr[global_settings.temp_unit]);
			break;
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

	if(settings_menu.count > SETTINGS_MAIN_MENU_MAX_PER_PG)
	{
		LCD_ShowImg_From_Flash(SETTINGS_MEUN_PAGE_DOT_X, SETTINGS_MEUN_PAGE_DOT_Y, IMG_SET_PG3_1_ADDR);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[0]);
		register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, settings_menu.pg_handler[1]);
	#endif		
	}

	entry_setting_bk_flag = false;
	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();

#ifndef CONFIG_FACTORY_TEST_SUPPORT
	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
#endif
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
	AnimaStop();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif
#ifdef CONFIG_WIFI_SUPPORT
	if(IsInWifiScreen()&&wifi_is_working())
		MenuStopWifi();
#endif

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SETTINGS;	
	scr_msg[SCREEN_ID_SETTINGS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SETTINGS].status = SCREEN_STATUS_CREATING;

#ifdef NB_SIGNAL_TEST
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#elif defined(CONFIG_FACTORY_TEST_SUPPORT)
	SetLeftKeyUpHandler(EnterFactoryTest);
#else
 #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
 	if((strlen(g_ui_ver) == 0) 
		|| (strlen(g_font_ver) == 0)
		|| (strlen(g_ppg_algo_ver) == 0)
		)
	{
		SPIFlash_Read_DataVer(g_ui_ver, g_font_ver, g_ppg_algo_ver);
	}
 
  	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		SetLeftKeyUpHandler(dl_img_start);
	}
	else if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		SetLeftKeyUpHandler(dl_font_start);
	}
  #if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
	{
		SetLeftKeyUpHandler(dl_ppg_start);
	}
  #endif
  	else
 #endif/*CONFIG_DATA_DOWNLOAD_SUPPORT*/
  	{
  		SetLeftKeyUpHandler(EnterPoweroffScreen);
  	}
#endif/*NB_SIGNAL_TEST*/
	SetRightKeyUpHandler(ExitSettingsScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

 #ifdef NB_SIGNAL_TEST
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 #elif defined(CONFIG_FACTORY_TEST_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterFactoryTest);
 #else
  #ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_img_start);
	}
	else if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_font_start);
	}
   #if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_ppg_start);
	}
   #endif
	else
  #endif/*CONFIG_DATA_DOWNLOAD_SUPPORT*/
	{
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 	}
 #endif/*NB_SIGNAL_TEST*/

 #ifdef NB_SIGNAL_TEST
  #ifdef CONFIG_WIFI_SUPPORT
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterWifiTestScreen);
  #else
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterGPSTestScreen);
  #endif
 #elif defined(CONFIG_FACTORY_TEST_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
 #else
  #ifdef CONFIG_SYNC_SUPPORT
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
  #elif defined(CONFIG_PPG_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #elif defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
   #ifdef CONFIG_SLEEP_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
   #elif defined(CONFIG_STEP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
   #endif
  #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
  #endif/*CONFIG_SYNC_SUPPORT*/
 #endif/*NB_SIGNAL_TEST*/
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
	k_timer_start(&mainmenu_timer, K_SECONDS(3), K_NO_WAIT);
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStop();
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
	
  #ifdef CONFIG_PPG_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterBPScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
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

void SyncUpdateStatus(void)
{
	uint32_t img_anima[3] = {IMG_RUNNING_ANI_1_ADDR, IMG_RUNNING_ANI_2_ADDR, IMG_RUNNING_ANI_3_ADDR};
	
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
		AnimaStop();
	#endif
	
		LCD_ShowImg_From_Flash(SYNC_FINISH_ICON_X, SYNC_FINISH_ICON_Y, IMG_SYNC_FINISH_ADDR);
		LCD_ShowImg_From_Flash(SYNC_RUNNING_ANI_X, SYNC_RUNNING_ANI_Y, IMG_RUNNING_ANI_1_ADDR);

		SetLeftKeyUpHandler(ExitSyncDataScreen);
		break;
		
	case SYNC_STATUS_FAIL:
	#ifdef CONFIG_ANIMATION_SUPPORT 
		AnimaStop();
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
static uint8_t img_flag = 0;
static uint8_t temp_retry_left = 2;
static uint8_t tempdata[TEMP_REC2_MAX_DAILY*sizeof(temp_rec2_nod)] = {0};

void TempShowNumByImg(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t num_img_w, uint32_t *img_addr, uint32_t separa_img_addr, float data)
{
	uint8_t i,count=1;
	uint16_t x1,temp_body;
	uint32_t divisor=10;
	uint16_t img_w,img_h;
	
	if(global_settings.temp_unit == TEMP_UINT_C)
	{
		temp_body = round(data*10.0);
	}
	else
	{
		temp_body = round((32+1.8*data)*10.0);
	}

	while(1)
	{
		if(temp_body/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	LCD_Fill(x, y, w, h, BLACK);
	LCD_get_pic_size_from_flash(separa_img_addr, &img_w, &img_h);
	x1 = x;
	if(count == 1)
	{
		x1 = x+(w-2*num_img_w-img_w)/2;
		LCD_ShowImg_From_Flash(x1, y, img_addr[temp_body/10]);
		x1 += num_img_w;
		LCD_ShowImg_From_Flash(x1, y, separa_img_addr);
		x1 += img_w;
		LCD_ShowImg_From_Flash(x1, y, img_addr[temp_body%10]);
	}
	else
	{
		for(i=0;i<(count+1);i++)
		{
			if(i == count-1)
			{
				LCD_ShowImg_From_Flash(x1, y, separa_img_addr);
				x1 += img_w;
			}
			else
			{
				LCD_ShowImg_From_Flash(x1, y, img_addr[temp_body/divisor]);
				temp_body = temp_body%divisor;
				divisor = divisor/10;
				x1 += num_img_w;
			}
		}
	}
}

void TempScreenStopTimer(void)
{
	k_timer_stop(&mainmenu_timer);
#ifdef UI_STYLE_HEALTH_BAR
	k_timer_stop(&temp_status_timer);
#endif
}

static void TempStatusTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_TEMP)
	{
		switch(g_temp_status)
		{
		case TEMP_STATUS_PREPARE:
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TEMP;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case TEMP_STATUS_MEASURING:
			if(get_temp_ok_flag)
			{
				g_temp_status = TEMP_STATUS_MEASURE_OK;
			}
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TEMP;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case TEMP_STATUS_MEASURE_FAIL:
			if(temp_retry_left > 0)
			{
				g_temp_status = TEMP_STATUS_NOTIFY;
				scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TEMP;
				scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			}
			else
			{
				g_temp_status = TEMP_STATUS_MAX;
				EntryIdleScr();
			}
			break;

		case TEMP_STATUS_MEASURE_OK:
			g_temp_status = TEMP_STATUS_MAX;
			EntryIdleScr();
			break;

		case TEMP_STATUS_NOTIFY:
			g_temp_status = TEMP_STATUS_PREPARE;
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TEMP;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
		}
	}
}

void TempUpdateStatus(void)
{
	uint16_t x,y,w,h;
	uint8_t tmpbuf[64] = {0};
	uint8_t strbuf[64] = {0};
	uint32_t img_anima[3] = {IMG_TEMP_BIG_ICON_1_ADDR,IMG_TEMP_BIG_ICON_2_ADDR,IMG_TEMP_BIG_ICON_3_ADDR};

#ifdef UI_STYLE_HEALTH_BAR
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
		k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
	}
#else/*UI_STYLE_HEALTH_BAR*/
	uint8_t i,count=1;
	uint16_t temp_body;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_42_NUM_0_ADDR,IMG_FONT_42_NUM_1_ADDR,IMG_FONT_42_NUM_2_ADDR,IMG_FONT_42_NUM_3_ADDR,IMG_FONT_42_NUM_4_ADDR,
							IMG_FONT_42_NUM_5_ADDR,IMG_FONT_42_NUM_6_ADDR,IMG_FONT_42_NUM_7_ADDR,IMG_FONT_42_NUM_8_ADDR,IMG_FONT_42_NUM_9_ADDR};
	uint32_t img_small_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
								IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};

	switch(g_temp_status)
	{
	case TEMP_STATUS_PREPARE:
		LCD_Fill(TEMP_NOTIFY_X, TEMP_NOTIFY_Y, TEMP_NOTIFY_W, TEMP_NOTIFY_H, BLACK);
		
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(TEMP_NOTIFY_X+(TEMP_NOTIFY_W-w)/2, TEMP_NOTIFY_Y, tmpbuf);
		
		MenuStartTemp();
		g_temp_status = TEMP_STATUS_MEASURING;
		break;
		
	case TEMP_STATUS_MEASURING:
		img_flag++;
		if(img_flag >= 3)
			img_flag = 0;
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, img_anima[img_flag]);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else
		LCD_SetFontSize(FONT_SIZE_32);
	#endif

		if(get_temp_ok_flag)
		{
			LCD_Fill(TEMP_NOTIFY_X, TEMP_NOTIFY_Y, TEMP_NOTIFY_W, TEMP_NOTIFY_H, BLACK);
			LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_BIG_ICON_3_ADDR);

			if(global_settings.temp_unit == TEMP_UINT_C)
			{
				temp_body = round(g_temp_body*10.0);
			}
			else
			{
				temp_body = round((32+1.8*g_temp_body)*10.0);
			}
		
			while(1)
			{
				if(temp_body/divisor > 0)
				{
					count++;
					divisor = divisor*10;
				}
				else
				{
					divisor = divisor/10;
					break;
				}
			}
		
			x = TEMP_STR_X+(TEMP_STR_W-count*TEMP_NUM_W-TEMP_DOT_W)/2;
			y = TEMP_STR_Y;
		
			if(count == 1)
			{
				x = TEMP_STR_X+(TEMP_STR_W-2*TEMP_NUM_W-TEMP_DOT_W)/2;
				LCD_ShowImg_From_Flash(x, y, img_num[temp_body/10]);
				x += TEMP_NUM_W;
				LCD_ShowImg_From_Flash(x, y, IMG_FONT_42_DOT_ADDR);
				x += TEMP_DOT_W;
				LCD_ShowImg_From_Flash(x, y, img_num[temp_body%10]);
				x += TEMP_NUM_W;
			}
			else
			{
				for(i=0;i<(count+1);i++)
				{
					if(i == count-1)
					{
						LCD_ShowImg_From_Flash(x, y, IMG_FONT_42_DOT_ADDR);
						x += TEMP_DOT_W;
					}
					else
					{
						LCD_ShowImg_From_Flash(x, y, img_num[temp_body/divisor]);
						temp_body = temp_body%divisor;
						divisor = divisor/10;
						x += TEMP_NUM_W;
					}
				}
			}
			
			if(global_settings.temp_unit == TEMP_UINT_C)
				LCD_ShowImg_From_Flash(x, TEMP_UNIT_Y, IMG_TEMP_UNIT_C_ADDR);
			else
				LCD_ShowImg_From_Flash(x, TEMP_UNIT_Y, IMG_TEMP_UNIT_F_ADDR);
		
			MenuStopTemp();
			k_timer_start(&temp_status_timer, K_SECONDS(5), K_NO_WAIT);
		}
		break;
		
	case TEMP_STATUS_MEASURE_OK:
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_BIG_ICON_3_ADDR);
		TempShowNumByImg(TEMP_UP_STR_X, TEMP_UP_STR_Y, TEMP_UP_STR_W, TEMP_UP_STR_H, TEMP_UP_NUM_W, img_small_num, IMG_FONT_24_DOT_ADDR, (float)last_health.deca_temp_max/10.0);
		TempShowNumByImg(TEMP_DOWN_STR_X, TEMP_DOWN_STR_Y, TEMP_DOWN_STR_W, TEMP_DOWN_STR_H, TEMP_DOWN_NUM_W, img_small_num, IMG_FONT_24_DOT_ADDR, (float)last_health.deca_temp_min/10.0);
		k_timer_start(&temp_status_timer, K_SECONDS(2), K_NO_WAIT);
		break;
		
	case TEMP_STATUS_MEASURE_FAIL:
		MenuStopTemp();

		LCD_Fill(TEMP_NOTIFY_X, TEMP_NOTIFY_Y, TEMP_NOTIFY_W, TEMP_NOTIFY_H, BLACK);
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_BIG_ICON_3_ADDR);
		
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		temp_retry_left--;
		if(temp_retry_left == 0)
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_1_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		else
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_2_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(TEMP_NOTIFY_X+(TEMP_NOTIFY_W-w)/2, TEMP_NOTIFY_Y, tmpbuf);		

		k_timer_start(&temp_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
		
	case TEMP_STATUS_NOTIFY:
		LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_BIG_ICON_3_ADDR);

	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_retry_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(TEMP_NOTIFY_X+(TEMP_NOTIFY_W-w)/2, TEMP_NOTIFY_Y, tmpbuf);

		k_timer_start(&temp_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
	}
#endif/*UI_STYLE_HEALTH_BAR*/
}

void TempShowStatus(void)
{
	uint16_t i,x,y,w,h;
	uint8_t tmpbuf[128] = {0};
	temp_rec2_nod *p_temp;
	float temp_max = 0.0, temp_min = 0.0;
	uint16_t temp[24] = {0};
	uint16_t color = 0x05DF;
	uint16_t title_str[LANGUAGE_MAX][24] = {
											#ifndef FW_FOR_CN
												{0x0042,0x006F,0x0064,0x0079,0x0020,0x0054,0x0065,0x006D,0x0070,0x0065,0x0072,0x0061,0x0074,0x0075,0x0072,0x0065,0x0000},//Body Temperature
												{0x004B,0x00F6,0x0072,0x0070,0x0065,0x0072,0x0074,0x0065,0x006D,0x0070,0x0065,0x0072,0x0061,0x0074,0x0075,0x0072,0x0000},//K?rpertemperatur
												{0x0054,0x0065,0x006D,0x0070,0x00E9,0x0072,0x0061,0x0074,0x0075,0x0072,0x0065,0x0020,0x0063,0x006F,0x0072,0x0070,0x006F,0x0072,0x0065,0x006C,0x006C,0x0065,0x0000},//Température corporelle
												{0x0054,0x0065,0x006D,0x0070,0x0065,0x0072,0x0061,0x0074,0x0075,0x0072,0x0061,0x0020,0x0043,0x006F,0x0072,0x0070,0x006F,0x0072,0x0065,0x0061,0x0000},//Temperatura Corporea
												{0x0054,0x0065,0x006D,0x0070,0x0065,0x0072,0x0061,0x0074,0x0075,0x0072,0x0061,0x0020,0x0063,0x006F,0x0072,0x0070,0x006F,0x0072,0x0061,0x006C,0x0000},//Temperatura corporal
												{0x0054,0x0065,0x006D,0x0070,0x0065,0x0072,0x0061,0x0074,0x0075,0x0072,0x0061,0x0020,0x0063,0x006F,0x0072,0x0070,0x006F,0x0072,0x0061,0x006C,0x0000},//Temperatura corporal
											#else
												{0x4F53,0x6E29,0x0000},//体温
												{0x0042,0x006F,0x0064,0x0079,0x0020,0x0054,0x0065,0x006D,0x0070,0x0065,0x0072,0x0061,0x0074,0x0075,0x0072,0x0065,0x0000},//Body Temperature
											#endif
											};

#ifdef UI_STYLE_HEALTH_BAR
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

	memset(&tempdata, 0x00, sizeof(tempdata));
	GetCurDayTempRecData(&tempdata);
	p_temp = (temp_rec2_nod*)&tempdata;
	for(i=0;i<TEMP_REC2_MAX_DAILY;i++)
	{
		if(p_temp->year == 0x0000
			|| p_temp->month == 0x00
			|| p_temp->day == 0x00
			)
		{
			break;
		}
		
		if((p_temp->deca_temp >= TEMP_MIN) && (p_temp->deca_temp <= TEMP_MAX))
		{
			if((temp_max == 0.0) && (temp_min == 0.0))
			{
				temp_max = (float)p_temp->deca_temp/10.0;
				temp_min = (float)p_temp->deca_temp/10.0;
			}
			else
			{
				if(p_temp->deca_temp/10.0 > temp_max)
					temp_max = (float)p_temp->deca_temp/10.0;
				if(p_temp->deca_temp/10.0 < temp_min)
					temp_min = (float)p_temp->deca_temp/10.0;
			}

			LCD_Fill(TEMP_REC_DATA_X+TEMP_REC_DATA_OFFSET_X*i, TEMP_REC_DATA_Y-(temp[i]/10.0-32.0)*15/2, TEMP_REC_DATA_W, (temp[i]/10.0-32.0)*15/2, color);
		}

		p_temp++;
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
	LCD_ShowString(TEMP_NUM_X+(TEMP_NUM_W-w)/2, TEMP_NUM_Y+(TEMP_NUM_H-h)/2, tmpbuf);

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
	LCD_ShowString(TEMP_UP_NUM_X+(TEMP_UP_NUM_W-w)/2, TEMP_UP_NUM_Y+(TEMP_UP_NUM_H-h)/2, tmpbuf);

	if(global_settings.temp_unit == TEMP_UINT_C)
		sprintf(tmpbuf, "%0.1f", temp_min);
	else
		sprintf(tmpbuf, "%0.1f", temp_min*1.8+32);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(TEMP_DOWN_NUM_X+(TEMP_DOWN_NUM_W-w)/2, TEMP_DOWN_NUM_Y+(TEMP_DOWN_NUM_H-h)/2, tmpbuf);

#else/*UI_STYLE_HEALTH_BAR*/
	uint8_t count=1;
	uint16_t temp_body;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
							IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};

	LCD_ShowImg_From_Flash(TEMP_ICON_X, TEMP_ICON_Y, IMG_TEMP_BIG_ICON_3_ADDR);
	LCD_ShowImg_From_Flash(TEMP_UP_ARRAW_X, TEMP_UP_ARRAW_Y, IMG_TEMP_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(TEMP_DOWN_ARRAW_X, TEMP_DOWN_ARRAW_Y, IMG_TEMP_DOWN_ARRAW_ADDR);

  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);
  #else	
	LCD_SetFontSize(FONT_SIZE_24);
  #endif

	mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)title_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
	LCD_MeasureUniString(tmpbuf, &w, &h);
	LCD_ShowUniString(TEMP_NOTIFY_X+(TEMP_NOTIFY_W-w)/2, TEMP_NOTIFY_Y, tmpbuf);

#if 0	//xb add 2024-03-14 The results of the last manual measurement are also included in the judgment.
	memset(&tempdata, 0x00, sizeof(tempdata));
	GetCurDayTempRecData(&tempdata);
	p_temp = (temp_rec2_nod*)&tempdata;
	for(i=0;i<TEMP_REC2_MAX_DAILY;i++)
	{
		if(p_temp->year == 0x0000
			|| p_temp->month == 0x00
			|| p_temp->day == 0x00
			)
		{
			break;
		}
		
		if((p_temp->deca_temp >= TEMP_MIN) && (p_temp->deca_temp <= TEMP_MAX))
		{
			if((temp_max == 0.0) && (temp_min == 0.0))
			{
				temp_max = (float)p_temp->deca_temp/10.0;
				temp_min = (float)p_temp->deca_temp/10.0;
			}
			else
			{
				if(p_temp->deca_temp/10.0 > temp_max)
					temp_max = (float)p_temp->deca_temp/10.0;
				if(p_temp->deca_temp/10.0 < temp_min)
					temp_min = (float)p_temp->deca_temp/10.0;
			}
		}

		p_temp++;
	}
	TempShowNumByImg(TEMP_UP_STR_X, TEMP_UP_STR_Y, TEMP_UP_STR_W, TEMP_UP_STR_H, TEMP_UP_NUM_W, img_num, IMG_FONT_24_DOT_ADDR, temp_max);
	TempShowNumByImg(TEMP_DOWN_STR_X, TEMP_DOWN_STR_Y, TEMP_DOWN_STR_W, TEMP_DOWN_STR_H, TEMP_DOWN_NUM_W, img_num, IMG_FONT_24_DOT_ADDR, temp_min);
#else
	TempShowNumByImg(TEMP_UP_STR_X, TEMP_UP_STR_Y, TEMP_UP_STR_W, TEMP_UP_STR_H, TEMP_UP_NUM_W, img_num, IMG_FONT_24_DOT_ADDR, (float)last_health.deca_temp_max/10.0);
	TempShowNumByImg(TEMP_DOWN_STR_X, TEMP_DOWN_STR_Y, TEMP_DOWN_STR_W, TEMP_DOWN_STR_H, TEMP_DOWN_NUM_W, img_num, IMG_FONT_24_DOT_ADDR, (float)last_health.deca_temp_min/10.0);
#endif

	k_timer_start(&temp_status_timer, K_SECONDS(2), K_NO_WAIT);
#endif/*UI_STYLE_HEALTH_BAR*/
}

void TempScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_TEMP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_TEMP].status = SCREEN_STATUS_CREATED;

		LCD_Clear(BLACK);
	#ifndef UI_STYLE_HEALTH_BAR	
		IdleShowSignal();
		IdleShowNetMode();
		IdleShowBatSoc();
	#endif	
		TempShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
	#ifndef UI_STYLE_HEALTH_BAR	
		if(scr_msg[SCREEN_ID_TEMP].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_TEMP].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
		}
		if(scr_msg[SCREEN_ID_TEMP].para&SCREEN_EVENT_UPDATE_NET_MODE)
		{
			scr_msg[SCREEN_ID_TEMP].para &= (~SCREEN_EVENT_UPDATE_NET_MODE);	
			IdleShowNetMode();
		}
		if(scr_msg[SCREEN_ID_TEMP].para&SCREEN_EVENT_UPDATE_BAT)
		{
			scr_msg[SCREEN_ID_TEMP].para &= (~SCREEN_EVENT_UPDATE_BAT);
			IdleUpdateBatSoc();
		}
	#endif	
		if(scr_msg[SCREEN_ID_TEMP].para&SCREEN_EVENT_UPDATE_TEMP)
		{
			scr_msg[SCREEN_ID_TEMP].para &= (~SCREEN_EVENT_UPDATE_TEMP);
			TempUpdateStatus();
		}
		break;
	}

	scr_msg[SCREEN_ID_TEMP].act = SCREEN_ACTION_NO;
}

void ExitTempScreen(void)
{
	k_timer_stop(&mainmenu_timer);
#ifndef UI_STYLE_HEALTH_BAR	
	k_timer_stop(&temp_status_timer);
#endif
	

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
#ifdef UI_STYLE_HEALTH_BAR
	k_timer_start(&mainmenu_timer, K_SECONDS(3), K_NO_WAIT);
#else
	k_timer_stop(&temp_status_timer);
  #ifdef CONFIG_PPG_SUPPORT	
	k_timer_stop(&ppg_status_timer);
  #endif
#endif

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStop();
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
#ifndef UI_STYLE_HEALTH_BAR	
	g_temp_status = TEMP_STATUS_PREPARE;
	img_flag = 0;
	temp_retry_left = 2;
#endif

#ifdef CONFIG_PPG_SUPPORT
	SetLeftKeyUpHandler(EnterSPO2Screen);
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
 #elif defined(CONFIG_SYNC_SUPPORT)
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
 #else
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
 #endif
 
 #ifdef CONFIG_PPG_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
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
#endif/*CONFIG_TEMP_SUPPORT*/

#ifdef CONFIG_PPG_SUPPORT
static uint8_t img_index = 0;
static uint8_t ppg_retry_left = 2;
static uint8_t ppgdata[PPG_REC2_MAX_DAILY*sizeof(bpt_rec2_nod)] = {0};
static notify_infor infor = {0};
static uint16_t str_timing_note[LANGUAGE_MAX][90] = {
													#ifndef FW_FOR_CN
														{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
														{0x0044,0x0069,0x0065,0x0020,0x005A,0x0065,0x0069,0x0074,0x006D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x0065,0x0074,0x0020,0x006B,0x0075,0x0072,0x007A,0x0020,0x0076,0x006F,0x0072,0x0020,0x0064,0x0065,0x006D,0x0020,0x0053,0x0074,0x0061,0x0072,0x0074,0x0020,0x0075,0x006E,0x0064,0x0020,0x0064,0x0069,0x0065,0x0073,0x0065,0x0020,0x004D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0069,0x0073,0x0074,0x0020,0x0076,0x006F,0x0072,0x0062,0x0065,0x0069,0x0021,0x0000},//Die Zeitmessung startet kurz vor dem Start und diese Messung ist vorbei!
														{0x004C,0x0061,0x0020,0x006D,0x0065,0x0073,0x0075,0x0072,0x0065,0x0020,0x0064,0x0075,0x0020,0x0073,0x0079,0x006E,0x0063,0x0068,0x0072,0x006F,0x006E,0x0069,0x0073,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x0065,0x0073,0x0074,0x0020,0x0073,0x0075,0x0072,0x0020,0x006C,0x0065,0x0020,0x0070,0x006F,0x0069,0x006E,0x0074,0x0020,0x0064,0x0065,0x0020,0x0063,0x006F,0x006D,0x006D,0x0065,0x006E,0x0063,0x0065,0x0072,0x002C,0x0020,0x0065,0x0074,0x0020,0x0063,0x0065,0x0074,0x0074,0x0065,0x0020,0x006D,0x0065,0x0073,0x0075,0x0072,0x0065,0x0020,0x0065,0x0073,0x0074,0x0020,0x0074,0x0065,0x0072,0x006D,0x0069,0x006E,0x00E9,0x0065,0x0021,0x0000},//La mesure du synchronisation est sur le point de commencer, et cette mesure est terminée!
														{0x004C,0x0061,0x0020,0x006D,0x0069,0x0073,0x0075,0x0072,0x0061,0x007A,0x0069,0x006F,0x006E,0x0065,0x0020,0x0064,0x0065,0x0069,0x0020,0x0074,0x0065,0x006D,0x0070,0x0069,0x0020,0x0073,0x0074,0x0061,0x0020,0x0070,0x0065,0x0072,0x0020,0x0069,0x006E,0x0069,0x007A,0x0069,0x0061,0x0072,0x0065,0x0020,0x0065,0x0020,0x0071,0x0075,0x0065,0x0073,0x0074,0x0061,0x0020,0x006D,0x0069,0x0073,0x0075,0x0072,0x0061,0x007A,0x0069,0x006F,0x006E,0x0065,0x0020,0x00E8,0x0020,0x0066,0x0069,0x006E,0x0069,0x0074,0x0061,0x0021,0x0000},//La misurazione dei tempi sta per iniziare e questa misurazione è finita!
														{0x004C,0x0061,0x0020,0x006D,0x0065,0x0064,0x0069,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0064,0x0065,0x0020,0x0074,0x0069,0x0065,0x006D,0x0070,0x006F,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0061,0x0020,0x0070,0x0075,0x006E,0x0074,0x006F,0x0020,0x0064,0x0065,0x0020,0x0063,0x006F,0x006D,0x0065,0x006E,0x007A,0x0061,0x0072,0x002C,0x0020,0x00A1,0x0079,0x0020,0x0065,0x0073,0x0074,0x0061,0x0020,0x006D,0x0065,0x0064,0x0069,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0068,0x0061,0x0020,0x0074,0x0065,0x0072,0x006D,0x0069,0x006E,0x0061,0x0064,0x006F,0x0021,0x0000},//La medición de tiempo está a punto de comenzar, ?y esta medición ha terminado!
														{0x0041,0x0020,0x006D,0x0065,0x0064,0x0069,0x00E7,0x00E3,0x006F,0x0020,0x0064,0x0065,0x0020,0x0074,0x0065,0x006D,0x0070,0x006F,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0070,0x0072,0x0065,0x0073,0x0074,0x0065,0x0073,0x0020,0x0061,0x0020,0x0063,0x006F,0x006D,0x0065,0x00E7,0x0061,0x0072,0x002C,0x0020,0x0065,0x0020,0x0065,0x0073,0x0073,0x0061,0x0020,0x006D,0x0065,0x0064,0x0069,0x00E7,0x00E3,0x006F,0x0020,0x0061,0x0063,0x0061,0x0062,0x006F,0x0075,0x0021,0x0000},//A medi??o de tempo está prestes a come?ar, e essa medi??o acabou!
													#else
														{0x5B9A,0x65F6,0x6D4B,0x91CF,0x5373,0x5C06,0x5F00,0x59CB,0xFF0C,0x672C,0x6B21,0x6D4B,0x91CF,0x7ED3,0x675F,0xFF01,0x0000},//定时测量即将开始，本次测量结束！
														{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
													#endif
													};
static uint16_t str_running_note[LANGUAGE_MAX][70] = {
													#ifndef FW_FOR_CN
														{0x0054,0x0068,0x0065,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0069,0x0073,0x0020,0x0072,0x0075,0x006E,0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
														{0x0044,0x0065,0x0072,0x0020,0x0053,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x006C,0x00E4,0x0075,0x0066,0x0074,0x002C,0x0020,0x0062,0x0069,0x0074,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0073,0x0070,0x00E4,0x0074,0x0065,0x0072,0x0020,0x0065,0x0072,0x006E,0x0065,0x0075,0x0074,0x0021,0x0000},//Der Sensor l?uft, bitte versuchen Sie es sp?ter erneut!
														{0x004C,0x0065,0x0020,0x0063,0x0061,0x0070,0x0074,0x0065,0x0075,0x0072,0x0020,0x0065,0x0073,0x0074,0x0020,0x0065,0x006E,0x0020,0x0063,0x006F,0x0075,0x0072,0x0073,0x0020,0x0064,0x0027,0x0065,0x0078,0x00E9,0x0063,0x0075,0x0074,0x0069,0x006F,0x006E,0x002C,0x0020,0x0076,0x0065,0x0075,0x0069,0x006C,0x006C,0x0065,0x007A,0x0020,0x0072,0x00E9,0x0065,0x0073,0x0073,0x0061,0x0079,0x0065,0x0072,0x0020,0x0070,0x006C,0x0075,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0021,0x0000},//Le capteur est en cours d'exécution, veuillez réessayer plus tard!
														{0x0049,0x006C,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0065,0x0020,0x00E8,0x0020,0x0069,0x006E,0x0020,0x0065,0x0073,0x0065,0x0063,0x0075,0x007A,0x0069,0x006F,0x006E,0x0065,0x002C,0x0020,0x0072,0x0069,0x0070,0x0072,0x006F,0x0076,0x0061,0x0020,0x0070,0x0069,0x00F9,0x0020,0x0074,0x0061,0x0072,0x0064,0x0069,0x0021,0x0000},//Il sensore è in esecuzione, riprova più tardi!
														{0x0045,0x006C,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0073,0x0065,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0065,0x006A,0x0065,0x0063,0x0075,0x0074,0x0061,0x006E,0x0064,0x006F,0x002C,0x0020,0x00A1,0x0069,0x006E,0x0074,0x00E9,0x006E,0x0074,0x0065,0x006C,0x006F,0x0020,0x0064,0x0065,0x0020,0x006E,0x0075,0x0065,0x0076,0x006F,0x0020,0x006D,0x00E1,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0065,0x0021,0x0000},//El sensor se está ejecutando, ?inténtelo de nuevo más tarde!
														{0x004F,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0066,0x0075,0x006E,0x0063,0x0069,0x006F,0x006E,0x0061,0x006E,0x0064,0x006F,0x002C,0x0020,0x0074,0x0065,0x006E,0x0074,0x0065,0x0020,0x006E,0x006F,0x0076,0x0061,0x006D,0x0065,0x006E,0x0074,0x0065,0x0020,0x006D,0x0061,0x0069,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0065,0x0021,0x0000},//O sensor está funcionando, tente novamente mais tarde!
													#else
														{0x4F20,0x611F,0x5668,0x6B63,0x5728,0x8FD0,0x884C,0xFF0C,0x8BF7,0x7A0D,0x540E,0x518D,0x8BD5,0xFF01,0x0000},//传感器正在运行，请稍后再试！
														{0x0054,0x0068,0x0065,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0069,0x0073,0x0020,0x0072,0x0075,0x006E,0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
													#endif
													};

void PPGPopShowIsWoring(void)
{
	infor.x = 0;
	infor.y = 0;
	infor.w = LCD_WIDTH;
	infor.h = LCD_HEIGHT;
	infor.align = NOTIFY_ALIGN_CENTER;
	infor.type = NOTIFY_TYPE_POPUP;
	
	infor.img_count = 0;
	
	mmi_ucs2cpy(infor.text, (uint8_t*)str_running_note[global_settings.language]);
	DisplayPopUp(infor);
}

void PPGPopShowIsWristOff(void)
{
	infor.x = 0;
	infor.y = 0;
	infor.w = LCD_WIDTH;
	infor.h = LCD_HEIGHT;
	infor.align = NOTIFY_ALIGN_CENTER;
	infor.type = NOTIFY_TYPE_POPUP;
	
	infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
	infor.img_count = 1;
	
	DisplayPopUp(infor);
}

void PPGPopShowTimedWorkReady(void)
{
	infor.x = 0;
	infor.y = 0;
	infor.w = LCD_WIDTH;
	infor.h = LCD_HEIGHT;
	infor.align = NOTIFY_ALIGN_CENTER;
	infor.type = NOTIFY_TYPE_POPUP;

	infor.img_count = 0;
	
	mmi_ucs2cpy(infor.text, (uint8_t*)&str_timing_note[global_settings.language]);
	DisplayPopUp(infor);
}

void PPGRedrawHourlyData(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		if((g_hr_hourly >= PPG_HR_MIN)&&(g_hr_hourly <= PPG_HR_MAX))
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
		if((g_spo2_hourly >= PPG_SPO2_MIN)&&(g_spo2_hourly <= PPG_SPO2_MAX))
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void PPGRedrawData(void)
{
	if(screen_id == SCREEN_ID_HR || screen_id == SCREEN_ID_SPO2 || screen_id == SCREEN_ID_BP)
	{
		if(screen_id == SCREEN_ID_HR)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
		else if(screen_id == SCREEN_ID_SPO2)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
		else if(screen_id == SCREEN_ID_BP)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BP;
		
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void PPGShowNumByImg(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t num_img_w, uint32_t *img_addr, uint8_t data)
{
	uint8_t i,count=1;
	uint32_t divisor=10;

	while(1)
	{
		if(data/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	LCD_Fill(x, y, w, h, BLACK);
	
	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(x+i*num_img_w, y, img_addr[data/divisor]);
		data = data%divisor;
		divisor = divisor/10;
	}
}

void PPGShowBpNumByImg(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t num_img_w, uint32_t *num_img_addr, uint32_t separa_img_addr, bpt_data data)
{
	uint8_t i,count1=1,count2=1;
	uint16_t x1;
	uint32_t divisor1=10,divisor2=10;

	while(1)
	{
		if(data.systolic/divisor1 > 0)
		{
			count1++;
			divisor1 = divisor1*10;
		}
		else
		{
			divisor1 = divisor1/10;
			break;
		}
	}
	while(1)
	{
		if(data.diastolic/divisor2 > 0)
		{
			count2++;
			divisor2 = divisor2*10;
		}
		else
		{
			divisor2 = divisor2/10;
			break;
		}
	}

	LCD_Fill(x, y, w, h, BLACK);
	
	x1 = x;
	for(i=0;i<(count1+count2+1);i++)
	{
		if(i < count1)
		{
			LCD_ShowImg_From_Flash(x1, y, num_img_addr[data.systolic/divisor1]);
			x1 += num_img_w;
			data.systolic = data.systolic%divisor1;
			divisor1 = divisor1/10;
		}
		else if(i == count1)
		{
			uint16_t img_w,img_h;
			
			LCD_get_pic_size_from_flash(separa_img_addr, &img_w, &img_h);
			LCD_ShowImg_From_Flash(x1, y, separa_img_addr);
			x1 += img_w;
		}
		else
		{
			LCD_ShowImg_From_Flash(x1, y, num_img_addr[data.diastolic/divisor2]);
			x1 += num_img_w;
			data.diastolic = data.diastolic%divisor2;
			divisor2 = divisor2/10;
		}
	}
}

void PPGShowNumWithUnitByImg(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t num_img_w, uint32_t *num_img_addr, uint32_t unit_img_addr, uint8_t data)
{
	uint8_t i,count=1;
	uint32_t divisor=10;

	while(1)
	{
		if(data/divisor > 0)
		{
			count++;
			divisor = divisor*10;
		}
		else
		{
			divisor = divisor/10;
			break;
		}
	}

	LCD_Fill(x, y, w, h, BLACK);

	for(i=0;i<count;i++)
	{
		LCD_ShowImg_From_Flash(x+i*num_img_w, y, num_img_addr[data/divisor]);
		data = data%divisor;
		divisor = divisor/10;
	}
	LCD_ShowImg_From_Flash(x+i*num_img_w, y, unit_img_addr);
}

void PPGScreenStopTimer(void)
{
	k_timer_stop(&mainmenu_timer);
#ifndef UI_STYLE_HEALTH_BAR	
	k_timer_stop(&ppg_status_timer);
#endif
}

static void PPGStatusTimerOutCallBack(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_HR)
	{
		switch(g_ppg_status)
		{
		case PPG_STATUS_PREPARE:
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case PPG_STATUS_MEASURING:
			if(get_hr_ok_flag)
			{
				g_ppg_status = PPG_STATUS_MEASURE_OK;
			}
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case PPG_STATUS_MEASURE_FAIL:
			if(ppg_retry_left > 0)
			{
				g_ppg_status = PPG_STATUS_NOTIFY;
				scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
				scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			}
			else
			{
				g_ppg_status = PPG_STATUS_MAX;
				EntryIdleScr();
			}
			break;

		case PPG_STATUS_MEASURE_OK:
			g_ppg_status = PPG_STATUS_MAX;
			EntryIdleScr();
			break;

		case PPG_STATUS_NOTIFY:
			g_ppg_status = PPG_STATUS_PREPARE;
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
		}
	}
	else if(screen_id == SCREEN_ID_SPO2)
	{
		switch(g_ppg_status)
		{
		case PPG_STATUS_PREPARE:
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case PPG_STATUS_MEASURING:
			if(get_spo2_ok_flag)
			{
				g_ppg_status = PPG_STATUS_MEASURE_OK;
			}
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case PPG_STATUS_MEASURE_FAIL:
			if(ppg_retry_left > 0)
			{
				g_ppg_status = PPG_STATUS_NOTIFY;
				scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
				scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			}
			else
			{
				g_ppg_status = PPG_STATUS_MAX;
				EntryIdleScr();
			}
			break;

		case PPG_STATUS_MEASURE_OK:
			g_ppg_status = PPG_STATUS_MAX;
			EntryIdleScr();
			break;

		case PPG_STATUS_NOTIFY:
			g_ppg_status = PPG_STATUS_PREPARE;
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
		}
	}
	else if(screen_id == SCREEN_ID_BP)
	{
		switch(g_ppg_status)
		{
		case PPG_STATUS_PREPARE:
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BP;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case PPG_STATUS_MEASURING:
			if(get_bpt_ok_flag)
			{
				g_ppg_status = PPG_STATUS_MEASURE_OK;
			}
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BP;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
			
		case PPG_STATUS_MEASURE_FAIL:
			if(ppg_retry_left > 0)
			{
				g_ppg_status = PPG_STATUS_NOTIFY;
				scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BP;
				scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			}
			else
			{
				g_ppg_status = PPG_STATUS_MAX;
				EntryIdleScr();
			}
			break;

		case PPG_STATUS_MEASURE_OK:
			g_ppg_status = PPG_STATUS_MAX;
			EntryIdleScr();
			break;

		case PPG_STATUS_NOTIFY:
			g_ppg_status = PPG_STATUS_PREPARE;
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BP;
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
			break;
		}
	}
}

void BPUpdateStatus(void)
{
	uint16_t x,y,w,h;
	uint8_t tmpbuf[128] = {0};
	uint8_t strbuf[64] = {0};
#ifdef UI_STYLE_HEALTH_BAR
	uint32_t img_anima[3] = {IMG_BP_ICON_ANI_1_ADDR,IMG_BP_ICON_ANI_2_ADDR,IMG_BP_ICON_ANI_3_ADDR};
#else
	uint32_t img_anima[3] = {IMG_BP_BIG_ICON_1_ADDR,IMG_BP_BIG_ICON_2_ADDR,IMG_BP_BIG_ICON_3_ADDR};
#endif

#ifdef UI_STYLE_HEALTH_BAR
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
		k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
	}

#else/*UI_STYLE_HEALTH_BAR*/

	uint8_t i,count1=1,count2=1;
	bpt_data bpt = {0};
	uint32_t divisor1=10,divisor2=10;
	uint32_t img_num[10] = {IMG_FONT_42_NUM_0_ADDR,IMG_FONT_42_NUM_1_ADDR,IMG_FONT_42_NUM_2_ADDR,IMG_FONT_42_NUM_3_ADDR,IMG_FONT_42_NUM_4_ADDR,
							IMG_FONT_42_NUM_5_ADDR,IMG_FONT_42_NUM_6_ADDR,IMG_FONT_42_NUM_7_ADDR,IMG_FONT_42_NUM_8_ADDR,IMG_FONT_42_NUM_9_ADDR};
	uint32_t img_small_num[10] = {IMG_FONT_16_NUM_0_ADDR,IMG_FONT_16_NUM_1_ADDR,IMG_FONT_16_NUM_2_ADDR,IMG_FONT_16_NUM_3_ADDR,IMG_FONT_16_NUM_4_ADDR,
								IMG_FONT_16_NUM_5_ADDR,IMG_FONT_16_NUM_6_ADDR,IMG_FONT_16_NUM_7_ADDR,IMG_FONT_16_NUM_8_ADDR,IMG_FONT_16_NUM_9_ADDR};

	switch(g_ppg_status)
	{
	case PPG_STATUS_PREPARE:
		LCD_Fill(BP_NOTIFY_X, BP_NOTIFY_Y, BP_NOTIFY_W, BP_NOTIFY_H, BLACK);
		
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(BP_NOTIFY_X+(BP_NOTIFY_W-w)/2, BP_NOTIFY_Y, tmpbuf);
		
		MenuStartBpt();
		g_ppg_status = PPG_STATUS_MEASURING;
		break;
		
	case PPG_STATUS_MEASURING:
		img_index++;
		if(img_index >= 3)
			img_index = 0;
		LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, img_anima[img_index]);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else
		LCD_SetFontSize(FONT_SIZE_32);
	#endif

		if(get_bpt_ok_flag)
		{
			LCD_Fill(BP_NOTIFY_X, BP_NOTIFY_Y, BP_NOTIFY_W, BP_NOTIFY_H, BLACK);

			memcpy(&bpt, &g_bpt, sizeof(bpt_data));

			while(1)
			{
				if(bpt.systolic/divisor1 > 0)
				{
					count1++;
					divisor1 = divisor1*10;
				}
				else
				{
					divisor1 = divisor1/10;
					break;
				}
			}
			
			while(1)
			{
				if(bpt.diastolic/divisor2 > 0)
				{
					count2++;
					divisor2 = divisor2*10;
				}
				else
				{
					divisor2 = divisor2/10;
					break;
				}
			}

			x = BP_STR_X+(BP_STR_W-(count1+count2)*BP_NUM_W-BP_SLASH_W)/2;
			y = HR_STR_Y;
			
			for(i=0;i<(count1+count2+1);i++)
			{
				if(i < count1)
				{
					LCD_ShowImg_From_Flash(x, y, img_num[bpt.systolic/divisor1]);
					x += BP_NUM_W;
					bpt.systolic = bpt.systolic%divisor1;
					divisor1 = divisor1/10;
				}
				else if(i == count1)
				{
					LCD_ShowImg_From_Flash(x, y, IMG_FONT_42_SLASH_ADDR);
					x += BP_SLASH_W;
				}
				else
				{
					LCD_ShowImg_From_Flash(x, y, img_num[bpt.diastolic/divisor2]);
					x += BP_NUM_W;
					bpt.diastolic = bpt.diastolic%divisor2;
					divisor2 = divisor2/10;
				}
			}

			MenuStopBpt();
			k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		}
		break;
		
	case PPG_STATUS_MEASURE_OK:
		LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, IMG_BP_BIG_ICON_3_ADDR);
		PPGShowBpNumByImg(BP_UP_STR_X, BP_UP_STR_Y, BP_UP_STR_W, BP_UP_STR_H, BP_UP_NUM_W, img_small_num, IMG_FONT_16_SLASH_ADDR, last_health.bpt_max);
		PPGShowBpNumByImg(BP_DOWN_STR_X, BP_DOWN_STR_Y, BP_DOWN_STR_W, BP_DOWN_STR_H, BP_DOWN_NUM_W, img_small_num, IMG_FONT_16_SLASH_ADDR, last_health.bpt_min);
		k_timer_start(&ppg_status_timer, K_SECONDS(2), K_NO_WAIT);
		break;
		
	case PPG_STATUS_MEASURE_FAIL:
		MenuStopBpt();

		LCD_Fill(BP_NOTIFY_X, BP_NOTIFY_Y, BP_NOTIFY_W, BP_NOTIFY_H, BLACK);
		LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, IMG_BP_BIG_ICON_3_ADDR);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		ppg_retry_left--;
		if(ppg_retry_left == 0)
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_1_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		else
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_2_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		LCD_MeasureUniString(tmpbuf,&w,&h);
		LCD_ShowUniString(BP_NOTIFY_X+(BP_NOTIFY_W-w)/2, BP_NOTIFY_Y, tmpbuf);

		k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
		
	case PPG_STATUS_NOTIFY:
		LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, IMG_BP_BIG_ICON_3_ADDR);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_retry_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(BP_NOTIFY_X+(BP_NOTIFY_W-w)/2, BP_NOTIFY_Y, tmpbuf);

		k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
	}
#endif/*UI_STYLE_HEALTH_BAR*/
}

void BPShowStatus(void)
{
	uint16_t i,x,y,w,h;
	uint8_t tmpbuf[128] = {0};
	bpt_rec2_nod *p_bpt;
	bpt_data bpt_max={0},bpt_min={0};
	uint16_t title_str[LANGUAGE_MAX][21] = {
											#ifndef FW_FOR_CN
												{0x0042,0x006C,0x006F,0x006F,0x0064,0x0020,0x0050,0x0072,0x0065,0x0073,0x0073,0x0075,0x0072,0x0065,0x0000},//Blood Pressure
												{0x0042,0x006C,0x0075,0x0074,0x0064,0x0072,0x0075,0x0063,0x006B,0x0000},//Blutdruck
												{0x0050,0x0072,0x0065,0x0073,0x0073,0x0069,0x006F,0x006E,0x0020,0x0061,0x0072,0x0074,0x00E9,0x0072,0x0069,0x0065,0x006C,0x006C,0x0065,0x0000},//Pression art閞ielle
												{0x0050,0x0072,0x0065,0x0073,0x0073,0x0069,0x006F,0x006E,0x0065,0x0020,0x0053,0x0061,0x006E,0x0067,0x0075,0x0069,0x0067,0x006E,0x0061,0x0000},//Pressione Sanguigna
												{0x0050,0x0072,0x0065,0x0073,0x0069,0x00F3,0x006E,0x0020,0x0061,0x0072,0x0074,0x0065,0x0072,0x0069,0x0061,0x006C,0x0000},//Presi髇 arterial
												{0x0050,0x0072,0x0065,0x0073,0x0073,0x00E3,0x006F,0x0020,0x0061,0x0072,0x0074,0x0065,0x0072,0x0069,0x0061,0x006C,0x0000},//Press鉶 arterial
											#else
												{0x8840,0x538B,0x0000},//血压
												{0x0042,0x006C,0x006F,0x006F,0x0064,0x0020,0x0050,0x0072,0x0065,0x0073,0x0073,0x0075,0x0072,0x0065,0x0000},//Blood Pressure
											#endif
											};

#ifdef UI_STYLE_HEALTH_BAR
	LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, IMG_BP_ICON_ANI_2_ADDR);
	LCD_ShowImg_From_Flash(BP_BG_X, BP_BG_Y, IMG_BP_BG_ADDR);
	LCD_ShowImg_From_Flash(BP_UNIT_X, BP_UNIT_Y, IMG_BP_UNIT_ADDR);
	LCD_ShowImg_From_Flash(BP_UP_ARRAW_X, BP_UP_ARRAW_Y, IMG_BP_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(BP_DOWN_ARRAW_X, BP_DOWN_ARRAW_Y, IMG_BP_DOWN_ARRAW_ADDR);

	memset(&ppgdata, 0x00, sizeof(ppgdata));
	GetCurDayBptRecData(&ppgdata);
	p_bpt = (bpt_rec2_nod*)&ppgdata;
	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		if(p_bpt->year == 0x0000 
			|| p_bpt->month == 0x00 
			|| p_bpt->day == 0x00
			)
		{
			break;
		}
		
		if((p_bpt->bpt.systolic >= PPG_BPT_SYS_MIN) && (p_bpt->bpt.systolic <= PPG_BPT_SYS_MAX)
			&& (p_bpt->bpt.diastolic >= PPG_BPT_DIA_MIN) && (p_bpt->bpt.diastolic >= PPG_BPT_DIA_MIN)
			)
		{
			if((bpt_max.systolic == 0) && (bpt_max.diastolic == 0)
				&& (bpt_min.systolic == 0) && (bpt_min.diastolic == 0))
			{
				memcpy(&bpt_max, &(p_bpt->bpt), sizeof(bpt_data));
				memcpy(&bpt_min, &(p_bpt->bpt), sizeof(bpt_data));
			}
			else
			{	
				if(p_bpt->bpt.systolic > bpt_max.systolic)
					memcpy(&bpt_max, &(p_bpt->bpt), sizeof(bpt_data));
				if(p_bpt->bpt.systolic < bpt_min.systolic)
					memcpy(&bpt_min, &(p_bpt->bpt), sizeof(bpt_data));
			}

			LCD_Fill(BP_REC_DATA_X+BP_REC_DATA_OFFSET_X*i, BP_REC_DATA_Y-(p_bpt->bpt.systolic-30)*15/30, BP_REC_DATA_W, (p_bpt->bpt.systolic-30)*15/30, YELLOW);
			LCD_Fill(BP_REC_DATA_X+BP_REC_DATA_OFFSET_X*i, BP_REC_DATA_Y-(p_bpt->bpt.diastolic-30)*15/30, BP_REC_DATA_W, (p_bpt->bpt.diastolic-30)*15/30, RED);
		}

		p_bpt++;
	}

  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);
  #else		
	LCD_SetFontSize(FONT_SIZE_24);
  #endif
	sprintf(tmpbuf, "%d/%d", 0, 0);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(BP_NUM_X+(BP_NUM_W-w)/2, BP_NUM_Y+(BP_NUM_H-h)/2, tmpbuf);
	
  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
  #else
	LCD_SetFontSize(FONT_SIZE_16);
  #endif
	sprintf(tmpbuf, "%d/%d", bpt_max.systolic, bpt_max.diastolic);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(BP_UP_NUM_X+(BP_UP_NUM_W-w)/2, BP_UP_NUM_Y+(BP_UP_NUM_H-h)/2, tmpbuf);

	sprintf(tmpbuf, "%d/%d", bpt_min.systolic, bpt_min.diastolic);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(BP_DOWN_NUM_X+(BP_DOWN_NUM_W-w)/2, BP_DOWN_NUM_Y+(BP_DOWN_NUM_H-h)/2, tmpbuf);

#else/*UI_STYLE_HEALTH_BAR*/
	uint8_t count1=1,count2=1;
	uint32_t divisor1=10,divisor2=10;
	uint32_t img_num[10] = {IMG_FONT_16_NUM_0_ADDR,IMG_FONT_16_NUM_1_ADDR,IMG_FONT_16_NUM_2_ADDR,IMG_FONT_16_NUM_3_ADDR,IMG_FONT_16_NUM_4_ADDR,
							IMG_FONT_16_NUM_5_ADDR,IMG_FONT_16_NUM_6_ADDR,IMG_FONT_16_NUM_7_ADDR,IMG_FONT_16_NUM_8_ADDR,IMG_FONT_16_NUM_9_ADDR};

	LCD_ShowImg_From_Flash(BP_ICON_X, BP_ICON_Y, IMG_BP_BIG_ICON_3_ADDR);
	LCD_ShowImg_From_Flash(BP_UP_ARRAW_X, BP_UP_ARRAW_Y, IMG_BP_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(BP_DOWN_ARRAW_X, BP_DOWN_ARRAW_Y, IMG_BP_DOWN_ARRAW_ADDR);

  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);
  #else	
	LCD_SetFontSize(FONT_SIZE_24);
  #endif

	mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)title_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
	LCD_MeasureUniString(tmpbuf, &w, &h);
	LCD_ShowUniString(BP_NOTIFY_X+(BP_NOTIFY_W-w)/2, BP_NOTIFY_Y, tmpbuf);

#if 0 //xb add 2024-03-14 The results of the last manual measurement are also included in the judgment.
	memset(&ppgdata, 0x00, sizeof(ppgdata));
	GetCurDayBptRecData(&ppgdata);
	p_bpt = (bpt_rec2_nod*)&ppgdata;
	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		if(p_bpt->year == 0x0000 
			|| p_bpt->month == 0x00 
			|| p_bpt->day == 0x00
			)
		{
			break;
		}
		
		if((p_bpt->bpt.systolic >= PPG_BPT_SYS_MIN) && (p_bpt->bpt.systolic <= PPG_BPT_SYS_MAX)
			&& (p_bpt->bpt.diastolic >= PPG_BPT_DIA_MIN) && (p_bpt->bpt.diastolic >= PPG_BPT_DIA_MIN)
			)
		{
			if((bpt_max.systolic == 0) && (bpt_max.diastolic == 0)
				&& (bpt_min.systolic == 0) && (bpt_min.diastolic == 0))
			{
				memcpy(&bpt_max, &(p_bpt->bpt), sizeof(bpt_data));
				memcpy(&bpt_min, &(p_bpt->bpt), sizeof(bpt_data));
			}
			else
			{	
				if(p_bpt->bpt.systolic > bpt_max.systolic)
					memcpy(&bpt_max, &(p_bpt->bpt), sizeof(bpt_data));
				if(p_bpt->bpt.systolic < bpt_min.systolic)
					memcpy(&bpt_min, &(p_bpt->bpt), sizeof(bpt_data));
			}
		}

		p_bpt++;
	}	
	PPGShowBpNumByImg(BP_UP_STR_X, BP_UP_STR_Y, BP_UP_STR_W, BP_UP_STR_H, BP_UP_NUM_W, img_num, IMG_FONT_16_SLASH_ADDR, bpt_max);
	PPGShowBpNumByImg(BP_DOWN_STR_X, BP_DOWN_STR_Y, BP_DOWN_STR_W, BP_DOWN_STR_H, BP_DOWN_NUM_W, img_num, IMG_FONT_16_SLASH_ADDR, bpt_min);
#else
	PPGShowBpNumByImg(BP_UP_STR_X, BP_UP_STR_Y, BP_UP_STR_W, BP_UP_STR_H, BP_UP_NUM_W, img_num, IMG_FONT_16_SLASH_ADDR, last_health.bpt_max);
	PPGShowBpNumByImg(BP_DOWN_STR_X, BP_DOWN_STR_Y, BP_DOWN_STR_W, BP_DOWN_STR_H, BP_DOWN_NUM_W, img_num, IMG_FONT_16_SLASH_ADDR, last_health.bpt_min);
#endif

	k_timer_start(&ppg_status_timer, K_SECONDS(2), K_NO_WAIT);
#endif/*UI_STYLE_HEALTH_BAR*/
}

void BPScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_BP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_BP].status = SCREEN_STATUS_CREATED;

		LCD_Clear(BLACK);
	#ifndef UI_STYLE_HEALTH_BAR	
		IdleShowSignal();
		IdleShowNetMode();
		IdleShowBatSoc();
	#endif	
		BPShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
	#ifndef UI_STYLE_HEALTH_BAR	
		if(scr_msg[SCREEN_ID_BP].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_BP].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
		}
		if(scr_msg[SCREEN_ID_BP].para&SCREEN_EVENT_UPDATE_NET_MODE)
		{
			scr_msg[SCREEN_ID_BP].para &= (~SCREEN_EVENT_UPDATE_NET_MODE);	
			IdleShowNetMode();
		}
		if(scr_msg[SCREEN_ID_BP].para&SCREEN_EVENT_UPDATE_BAT)
		{
			scr_msg[SCREEN_ID_BP].para &= (~SCREEN_EVENT_UPDATE_BAT);
			IdleUpdateBatSoc();
		}
	#endif
		if(scr_msg[SCREEN_ID_BP].para&SCREEN_EVENT_UPDATE_BP)
		{
			scr_msg[SCREEN_ID_BP].para &= (~SCREEN_EVENT_UPDATE_BP);
			BPUpdateStatus();
		}
		break;
	}
	
	scr_msg[SCREEN_ID_BP].act = SCREEN_ACTION_NO;
}

void ExitBPScreen(void)
{
	k_timer_stop(&mainmenu_timer);
#ifndef UI_STYLE_HEALTH_BAR	
	k_timer_stop(&ppg_status_timer);
#endif
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
#ifdef UI_STYLE_HEALTH_BAR
	k_timer_start(&mainmenu_timer, K_SECONDS(3), K_NO_WAIT);
#else
	k_timer_stop(&ppg_status_timer);
  #ifdef CONFIG_TEMP_SUPPORT
	k_timer_stop(&temp_status_timer);
  #endif
#endif

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
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
#ifndef UI_STYLE_HEALTH_BAR	
	g_ppg_status = PPG_STATUS_PREPARE;
	ppg_retry_left = 2;
#endif

#ifdef CONFIG_SYNC_SUPPORT
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
	SetRightKeyUpHandler(ExitBPScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

 #ifdef CONFIG_SYNC_SUPPORT
 	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
 #else
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings); 
 #endif
 
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSPO2Screen);
#endif
}

void SPO2UpdateStatus(void)
{
	uint8_t strbuf[64] = {0};
	uint8_t tmpbuf[128] = {0};
	uint16_t w,h,len;
	uint16_t dot_str[2] = {0x002e,0x0000};
#ifdef UI_STYLE_HEALTH_BAR
	unsigned char *img_anima[3] = {IMG_SPO2_ANI_1_ADDR, IMG_SPO2_ANI_2_ADDR, IMG_SPO2_ANI_3_ADDR};
#else
	unsigned char *img_anima[3] = {IMG_SPO2_BIG_ICON_1_ADDR, IMG_SPO2_BIG_ICON_2_ADDR, IMG_SPO2_BIG_ICON_3_ADDR};
#endif

#ifdef UI_STYLE_HEALTH_BAR
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
	LCD_Fill(SPO2_NUM_X, SPO2_NUM_Y, SPO2_NUM_W, SPO2_NUM_H, BLACK);
	LCD_ShowString(SPO2_NUM_X+(SPO2_NUM_W-w)/2, SPO2_NUM_Y+(SPO2_NUM_H-h)/2, tmpbuf);

	if(get_spo2_ok_flag)
	{
		k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
	}

#else/*UI_STYLE_HEALTH_BAR*/
	uint8_t i,spo2=g_spo2,count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_42_NUM_0_ADDR,IMG_FONT_42_NUM_1_ADDR,IMG_FONT_42_NUM_2_ADDR,IMG_FONT_42_NUM_3_ADDR,IMG_FONT_42_NUM_4_ADDR,
							IMG_FONT_42_NUM_5_ADDR,IMG_FONT_42_NUM_6_ADDR,IMG_FONT_42_NUM_7_ADDR,IMG_FONT_42_NUM_8_ADDR,IMG_FONT_42_NUM_9_ADDR};
	uint32_t img_small_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
									IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};

	switch(g_ppg_status)
	{
	case PPG_STATUS_PREPARE:
		LCD_Fill(SPO2_NOTIFY_X, SPO2_NOTIFY_Y, SPO2_NOTIFY_W, SPO2_NOTIFY_H, BLACK);
		
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(SPO2_NOTIFY_X+(SPO2_NOTIFY_W-w)/2, SPO2_NOTIFY_Y, tmpbuf);
		
		MenuStartSpo2();
		g_ppg_status = PPG_STATUS_MEASURING;
		break;
		
	case PPG_STATUS_MEASURING:
		img_index++;
		if(img_index >= 3)
			img_index = 0;
		LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, img_anima[img_index]);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else
		LCD_SetFontSize(FONT_SIZE_32);
	#endif

		if(get_spo2_ok_flag)
		{
			LCD_Fill(SPO2_NOTIFY_X, SPO2_NOTIFY_Y, SPO2_NOTIFY_W, SPO2_NOTIFY_H, BLACK);

			while(1)
			{
				if(spo2/divisor > 0)
				{
					count++;
					divisor = divisor*10;
				}
				else
				{
					divisor = divisor/10;
					break;
				}
			}

			for(i=0;i<count;i++)
			{
				LCD_ShowImg_From_Flash(SPO2_STR_X+(SPO2_STR_W-count*SPO2_NUM_W-SPO2_PERC_W)/2+i*SPO2_NUM_W, SPO2_STR_Y, img_num[spo2/divisor]);
				spo2 = spo2%divisor;
				divisor = divisor/10;
			}
			LCD_ShowImg_From_Flash(SPO2_STR_X+(SPO2_STR_W-count*SPO2_NUM_W-SPO2_PERC_W)/2+i*SPO2_NUM_W, SPO2_STR_Y, IMG_FONT_42_PERC_ADDR);
		
			MenuStopSpo2();
			k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		}
		break;
		
	case PPG_STATUS_MEASURE_OK:
		LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, IMG_SPO2_BIG_ICON_3_ADDR);
		PPGShowNumWithUnitByImg(SPO2_UP_STR_X, SPO2_UP_STR_Y, SPO2_UP_STR_W, SPO2_UP_STR_H, SPO2_UP_NUM_W, img_small_num, IMG_FONT_24_PERC_ADDR, last_health.spo2_max);
		PPGShowNumWithUnitByImg(SPO2_DOWN_STR_X, SPO2_DOWN_STR_Y, SPO2_DOWN_STR_W, SPO2_DOWN_STR_H, SPO2_DOWN_NUM_W, img_small_num, IMG_FONT_24_PERC_ADDR, last_health.spo2_min);
		k_timer_start(&ppg_status_timer, K_SECONDS(2), K_NO_WAIT);
		break;
		
	case PPG_STATUS_MEASURE_FAIL:
		MenuStopSpo2();

		LCD_Fill(SPO2_NOTIFY_X, SPO2_NOTIFY_Y, SPO2_NOTIFY_W, SPO2_NOTIFY_H, BLACK);
		LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, IMG_SPO2_BIG_ICON_3_ADDR);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		ppg_retry_left--;
		if(ppg_retry_left == 0)
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_1_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		else
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_2_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		LCD_MeasureUniString(tmpbuf,&w,&h);
		LCD_ShowUniString(SPO2_NOTIFY_X+(SPO2_NOTIFY_W-w)/2, SPO2_NOTIFY_Y, tmpbuf);
		
		k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
		
	case PPG_STATUS_NOTIFY:
		LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, IMG_SPO2_BIG_ICON_3_ADDR);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_retry_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(SPO2_NOTIFY_X+(SPO2_NOTIFY_W-w)/2, SPO2_NOTIFY_Y, tmpbuf);

		k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
	}
#endif/*UI_STYLE_HEALTH_BAR*/
}

void SPO2ShowStatus(void)
{
	uint8_t tmpbuf[128] = {0};
	spo2_rec2_nod *p_spo2;
	uint8_t spo2_max=0,spo2_min=0;
	uint16_t i,w,h;
	uint16_t title_str[LANGUAGE_MAX][25] = {
											#ifndef FW_FOR_CN
												{0x0042,0x006C,0x006F,0x006F,0x0064,0x0020,0x004F,0x0078,0x0079,0x0067,0x0065,0x006E,0x0000},//Blood Oxygen
												{0x0042,0x006C,0x0075,0x0074,0x0073,0x0061,0x0075,0x0065,0x0072,0x0073,0x0074,0x006F,0x0066,0x0066,0x0000},//Blutsauerstoff
												{0x004F,0x0078,0x0079,0x0067,0x00E8,0x006E,0x0065,0x0020,0x0073,0x0061,0x006E,0x0067,0x0075,0x0069,0x006E,0x0000},//Oxyg鑞e sanguin
												{0x0073,0x0061,0x0074,0x0075,0x0072,0x0061,0x007A,0x0069,0x006F,0x006E,0x0065,0x0020,0x0064,0x0069,0x0020,0x006F,0x0073,0x0073,0x0069,0x0067,0x0065,0x006E,0x006F,0x0000},//saturazione di ossigeno
												{0x004F,0x0078,0x00ED,0x0067,0x0065,0x006E,0x006F,0x0020,0x0064,0x0065,0x0020,0x0073,0x0061,0x006E,0x0067,0x0072,0x0065,0x0000},//Oxígeno de sangre
												{0x0053,0x0061,0x0074,0x0075,0x0072,0x0061,0x00E7,0x00E3,0x006F,0x0020,0x0064,0x0065,0x0020,0x006F,0x0078,0x0069,0x0067,0x00EA,0x006E,0x0069,0x006F,0x0000},//Satura??o de oxigênio
											#else
												{0x8840,0x6C27,0x0000},//血氧
												{0x0042,0x006C,0x006F,0x006F,0x0064,0x0020,0x004F,0x0078,0x0079,0x0067,0x0065,0x006E,0x0000},//Blood Oxygen
											#endif
							 				};

#ifdef UI_STYLE_HEALTH_BAR
	LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, IMG_SPO2_ANI_2_ADDR);
	LCD_ShowImg_From_Flash(SPO2_BG_X, SPO2_BG_Y, IMG_SPO2_BG_ADDR);
	LCD_ShowImg_From_Flash(SPO2_UP_ARRAW_X, SPO2_UP_ARRAW_Y, IMG_SPO2_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(SPO2_DOWN_ARRAW_X, SPO2_DOWN_ARRAW_Y, IMG_SPO2_DOWN_ARRAW_ADDR);
	
	memset(&ppgdata, 0x00, sizeof(ppgdata));
	GetCurDaySpo2RecData(&ppgdata);
	p_spo2 = (spo2_rec2_nod*)&ppgdata;
	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		if(p_spo2->year == 0x0000
			|| p_spo2->month == 0x00
			|| p_spo2->day == 0x00
			)
		{
			break;
		}
		
		if((p_spo2->spo2 >= PPG_SPO2_MIN) && (p_spo2->spo2 <= PPG_SPO2_MAX))
		{
			if((spo2_max == 0) && (spo2_min == 0))
			{
				spo2_max = p_spo2->spo2;
				spo2_min = p_spo2->spo2;
			}
			else
			{
				if(p_spo2->spo2 > spo2_max)
					spo2_max = p_spo2->spo2;
				if(p_spo2->spo2 < spo2_min)
					spo2_min = p_spo2->spo2;
			}
			
			LCD_Fill(SPO2_REC_DATA_X+SPO2_REC_DATA_OFFSET_X*i, SPO2_REC_DATA_Y-(p_spo2->spo2-80)*3, SPO2_REC_DATA_W, (p_spo2->spo2-80)*3, BLUE);
		}

		p_spo2++;
	}

  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
  #else
	LCD_SetFontSize(FONT_SIZE_32);
  #endif
	sprintf(tmpbuf, "%d%%", 0);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(SPO2_NUM_X+(SPO2_NUM_W-w)/2, SPO2_NUM_Y+(SPO2_NUM_H-h)/2, tmpbuf);

	sprintf(tmpbuf, "%d", spo2_max);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(SPO2_UP_NUM_X+(SPO2_UP_NUM_W-w)/2, SPO2_UP_NUM_Y+(SPO2_UP_NUM_H-h)/2, tmpbuf);

	sprintf(tmpbuf, "%d", spo2_min);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(SPO2_DOWN_NUM_X+(SPO2_DOWN_NUM_W-w)/2, SPO2_DOWN_NUM_Y+(SPO2_DOWN_NUM_H-h)/2, tmpbuf);

#else/*UI_STYLE_HEALTH_BAR*/

	uint8_t count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
							IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};

	LCD_ShowImg_From_Flash(SPO2_ICON_X, SPO2_ICON_Y, IMG_SPO2_BIG_ICON_3_ADDR);
	LCD_ShowImg_From_Flash(SPO2_UP_ARRAW_X, SPO2_UP_ARRAW_Y, IMG_SPO2_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(SPO2_DOWN_ARRAW_X, SPO2_DOWN_ARRAW_Y, IMG_SPO2_DOWN_ARRAW_ADDR);

  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);
  #else	
	LCD_SetFontSize(FONT_SIZE_24);
  #endif

	mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)title_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
	LCD_MeasureUniString(tmpbuf, &w, &h);
	LCD_ShowUniString(SPO2_NOTIFY_X+(SPO2_NOTIFY_W-w)/2, SPO2_NOTIFY_Y, tmpbuf);

#if 0	//xb add 2024-03-14 The results of the last manual measurement are also included in the judgment.
	memset(&ppgdata, 0x00, sizeof(ppgdata));
	GetCurDaySpo2RecData(&ppgdata);
	p_spo2 = (spo2_rec2_nod*)&ppgdata;
	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		if(p_spo2->year == 0x0000
			|| p_spo2->month == 0x00
			|| p_spo2->day == 0x00
			)
		{
			break;
		}
		
		if((p_spo2->spo2 >= PPG_SPO2_MIN) && (p_spo2->spo2 <= PPG_SPO2_MAX))
		{
			if((spo2_max == 0) && (spo2_min == 0))
			{
				spo2_max = p_spo2->spo2;
				spo2_min = p_spo2->spo2;
			}
			else
			{
				if(p_spo2->spo2 > spo2_max)
					spo2_max = p_spo2->spo2;
				if(p_spo2->spo2 < spo2_min)
					spo2_min = p_spo2->spo2;
			}
		}

		p_spo2++;
	}

	PPGShowNumWithUnitByImg(SPO2_UP_STR_X, SPO2_UP_STR_Y, SPO2_UP_STR_W, SPO2_UP_STR_H, SPO2_UP_NUM_W, img_num, IMG_FONT_24_PERC_ADDR, spo2_max);
	PPGShowNumWithUnitByImg(SPO2_DOWN_STR_X, SPO2_DOWN_STR_Y, SPO2_DOWN_STR_W, SPO2_DOWN_STR_H, SPO2_DOWN_NUM_W, img_num, IMG_FONT_24_PERC_ADDR, spo2_min);
#else
	PPGShowNumWithUnitByImg(SPO2_UP_STR_X, SPO2_UP_STR_Y, SPO2_UP_STR_W, SPO2_UP_STR_H, SPO2_UP_NUM_W, img_num, IMG_FONT_24_PERC_ADDR, last_health.spo2_max);
	PPGShowNumWithUnitByImg(SPO2_DOWN_STR_X, SPO2_DOWN_STR_Y, SPO2_DOWN_STR_W, SPO2_DOWN_STR_H, SPO2_DOWN_NUM_W, img_num, IMG_FONT_24_PERC_ADDR, last_health.spo2_min);
#endif

	k_timer_start(&ppg_status_timer, K_SECONDS(2), K_NO_WAIT);
#endif/*UI_STYLE_HEALTH_BAR*/
}

void SPO2ScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_SPO2].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SPO2].status = SCREEN_STATUS_CREATED;

		LCD_Clear(BLACK);
	#ifndef UI_STYLE_HEALTH_BAR	
		IdleShowSignal();
		IdleShowNetMode();
		IdleShowBatSoc();
	#endif
		SPO2ShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
	#ifndef UI_STYLE_HEALTH_BAR	
		if(scr_msg[SCREEN_ID_SPO2].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_SPO2].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
		}
		if(scr_msg[SCREEN_ID_SPO2].para&SCREEN_EVENT_UPDATE_NET_MODE)
		{
			scr_msg[SCREEN_ID_SPO2].para &= (~SCREEN_EVENT_UPDATE_NET_MODE);	
			IdleShowNetMode();
		}
		if(scr_msg[SCREEN_ID_SPO2].para&SCREEN_EVENT_UPDATE_BAT)
		{
			scr_msg[SCREEN_ID_SPO2].para &= (~SCREEN_EVENT_UPDATE_BAT);
			IdleUpdateBatSoc();
		}
	#endif	
		if(scr_msg[SCREEN_ID_SPO2].para&SCREEN_EVENT_UPDATE_SPO2)
		{
			scr_msg[SCREEN_ID_SPO2].para &= (~SCREEN_EVENT_UPDATE_SPO2);
			SPO2UpdateStatus();
		}
		break;
	}
	
	scr_msg[SCREEN_ID_SPO2].act = SCREEN_ACTION_NO;
}

void ExitSPO2Screen(void)
{
	k_timer_stop(&mainmenu_timer);
#ifndef UI_STYLE_HEALTH_BAR	
	k_timer_stop(&ppg_status_timer);
#endif
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
#ifdef UI_STYLE_HEALTH_BAR
	k_timer_start(&mainmenu_timer, K_SECONDS(3), K_NO_WAIT);
#else
	k_timer_stop(&ppg_status_timer);
  #ifdef CONFIG_TEMP_SUPPORT
	k_timer_stop(&temp_status_timer);
  #endif
#endif

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
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
#ifndef UI_STYLE_HEALTH_BAR	
	g_ppg_status = PPG_STATUS_PREPARE;
	ppg_retry_left = 2;
#endif

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
	uint8_t tmpbuf[128] = {0};
	uint16_t w,h;
	uint8_t strbuf[64] = {0};
	
#ifdef UI_STYLE_HEALTH_BAR
	unsigned char *img_anima[2] = {IMG_HR_ICON_ANI_1_ADDR, IMG_HR_ICON_ANI_2_ADDR};
#else
	unsigned char *img_anima[2] = {IMG_HR_BIG_ICON_1_ADDR, IMG_HR_BIG_ICON_2_ADDR};
#endif

#ifdef UI_STYLE_HEALTH_BAR
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
	LCD_Fill(HR_NUM_X, HR_NUM_Y, HR_NUM_W, HR_NUM_H, BLACK);
	LCD_ShowString(HR_NUM_X+(HR_NUM_W-w)/2, HR_NUM_Y+(HR_NUM_H-h)/2, tmpbuf);

	if(get_hr_ok_flag)
	{
		k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
	}
	
#else/*UI_STYLE_HEALTH_BAR*/
	uint8_t i,hr=g_hr,count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_42_NUM_0_ADDR,IMG_FONT_42_NUM_1_ADDR,IMG_FONT_42_NUM_2_ADDR,IMG_FONT_42_NUM_3_ADDR,IMG_FONT_42_NUM_4_ADDR,
							IMG_FONT_42_NUM_5_ADDR,IMG_FONT_42_NUM_6_ADDR,IMG_FONT_42_NUM_7_ADDR,IMG_FONT_42_NUM_8_ADDR,IMG_FONT_42_NUM_9_ADDR};
	uint32_t img_small_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
								IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};

	switch(g_ppg_status)
	{
	case PPG_STATUS_PREPARE:
		LCD_Fill(HR_NOTIFY_X, HR_NOTIFY_Y, HR_NOTIFY_W, HR_NOTIFY_H, BLACK);
		
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(HR_NOTIFY_X+(HR_NOTIFY_W-w)/2, HR_NOTIFY_Y, tmpbuf);
		
		MenuStartHr();
		g_ppg_status = PPG_STATUS_MEASURING;
		break;
		
	case PPG_STATUS_MEASURING:
		img_index++;
		if(img_index >= 2)
			img_index = 0;
		LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, img_anima[img_index]);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_36);
	#else
		LCD_SetFontSize(FONT_SIZE_32);
	#endif

		if(get_hr_ok_flag)
		{
			LCD_Fill(HR_NOTIFY_X, HR_NOTIFY_Y, HR_NOTIFY_W, HR_NOTIFY_H, BLACK);

			while(1)
			{
				if(hr/divisor > 0)
				{
					count++;
					divisor = divisor*10;
				}
				else
				{
					divisor = divisor/10;
					break;
				}
			}

			for(i=0;i<count;i++)
			{
				LCD_ShowImg_From_Flash(HR_STR_X+(HR_STR_W-count*HR_NUM_W)/2+i*HR_NUM_W, HR_STR_Y, img_num[hr/divisor]);
				hr = hr%divisor;
				divisor = divisor/10;
			}
			LCD_ShowImg_From_Flash(HR_STR_X+(HR_STR_W-count*HR_NUM_W)/2+i*HR_NUM_W, HR_UNIT_Y, IMG_HR_BPM_ADDR);

			MenuStopHr();

			k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		}
		break;
		
	case PPG_STATUS_MEASURE_OK:
		LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, IMG_HR_BIG_ICON_2_ADDR);
		PPGShowNumByImg(HR_UP_STR_X, HR_UP_STR_Y, HR_UP_STR_W, HR_UP_STR_H, HR_UP_NUM_W, img_small_num, last_health.hr_max);
		PPGShowNumByImg(HR_DOWN_STR_X, HR_DOWN_STR_Y, HR_DOWN_STR_W, HR_DOWN_STR_H, HR_DOWN_NUM_W, img_small_num, last_health.hr_min);
		k_timer_start(&ppg_status_timer, K_SECONDS(2), K_NO_WAIT);
		break;
		
	case PPG_STATUS_MEASURE_FAIL:
		MenuStopHr();
		
		LCD_Fill(HR_NOTIFY_X, HR_NOTIFY_Y, HR_NOTIFY_W, HR_NOTIFY_H, BLACK);
		LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, IMG_HR_BIG_ICON_2_ADDR);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else	
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		ppg_retry_left--;
		if(ppg_retry_left == 0)
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_1_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		else
		{
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)Incon_2_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		}
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(HR_NOTIFY_X+(HR_NOTIFY_W-w)/2, HR_NOTIFY_Y, tmpbuf);
			
		k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
		
	case PPG_STATUS_NOTIFY:
		LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, IMG_HR_BIG_ICON_2_ADDR);
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_28);
	#else
		LCD_SetFontSize(FONT_SIZE_24);
	#endif

		mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)still_retry_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(HR_NOTIFY_X+(HR_NOTIFY_W-w)/2, HR_NOTIFY_Y, tmpbuf);

		k_timer_start(&ppg_status_timer, K_SECONDS(5), K_NO_WAIT);
		break;
	}
#endif/*UI_STYLE_HEALTH_BAR*/
}

void HRShowStatus(void)
{
	uint8_t tmpbuf[128] = {0};
	hr_rec2_nod *p_hr;
	uint8_t hr_max=0,hr_min=0;
	uint16_t i,w,h;
	uint16_t title_str[LANGUAGE_MAX][21] = {
											#ifndef FW_FOR_CN
												{0x0048,0x0065,0x0061,0x0072,0x0074,0x0020,0x0052,0x0061,0x0074,0x0065,0x0000},//Heart Rate
												{0x0050,0x0075,0x006C,0x0073,0x0073,0x0063,0x0068,0x006C,0x0061,0x0067,0x0000},//Pulsschlag
												{0x0052,0x0079,0x0074,0x0068,0x006D,0x0065,0x0020,0x0063,0x0061,0x0072,0x0064,0x0069,0x0061,0x0071,0x0075,0x0065,0x0000},//Rythme cardiaque
												{0x0046,0x0072,0x0065,0x0071,0x0075,0x0065,0x006E,0x007A,0x0061,0x0020,0x0063,0x0061,0x0072,0x0064,0x0069,0x0061,0x0063,0x0000},//Frequenza cardiac
												{0x0052,0x0069,0x0074,0x006D,0x006F,0x0020,0x0063,0x0061,0x0072,0x0064,0x0069,0x0061,0x0063,0x0000},//Ritmo cardiac
												{0x0046,0x0072,0x0065,0x0071,0x0075,0x00EA,0x006E,0x0063,0x0069,0x0061,0x0020,0x0063,0x0061,0x0072,0x0064,0x00ED,0x0061,0x0063,0x0061,0x0000},//Frequência cardíaca
											#else
												{0x5FC3,0x7387,0x0000},//心率
												{0x0048,0x0065,0x0061,0x0072,0x0074,0x0020,0x0052,0x0061,0x0074,0x0065,0x0000},//Heart Rate
											#endif
							 				};

#ifdef UI_STYLE_HEALTH_BAR
	LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, IMG_HR_ICON_ANI_2_ADDR);
	LCD_ShowImg_From_Flash(HR_UNIT_X, HR_UNIT_Y, IMG_HR_BPM_ADDR);
	LCD_ShowImg_From_Flash(HR_BG_X, HR_BG_Y, IMG_HR_BG_ADDR);
	LCD_ShowImg_From_Flash(HR_UP_ARRAW_X, HR_UP_ARRAW_Y, IMG_HR_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(HR_DOWN_ARRAW_X, HR_DOWN_ARRAW_Y, IMG_HR_DOWN_ARRAW_ADDR);
	
	memset(&ppgdata, 0x00, sizeof(ppgdata));
	GetCurDayHrRecData(&ppgdata);
	p_hr = (hr_rec2_nod*)&ppgdata;
	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		if(p_hr->year == 0x0000
			|| p_hr->month == 0x00
			|| p_hr->day == 0x00
			)
		{
			break;
		}
		
		if((p_hr->hr >= PPG_HR_MIN) && (p_hr->hr <= PPG_HR_MAX))
		{
			if((hr_max == 0) && (hr_min == 0))
			{
				hr_max = p_hr->hr;
				hr_min = p_hr->hr;
			}
			else
			{
				if(p_hr->hr > hr_max)
					hr_max = p_hr->hr;
				if(p_hr->hr < hr_min)
					hr_min = p_hr->hr;
			}

			LCD_Fill(HR_REC_DATA_X+HR_REC_DATA_OFFSET_X*i, HR_REC_DATA_Y-p_hr->hr*20/50, HR_REC_DATA_W, p_hr->hr*20/50, RED);
		}

		p_hr++;
	}

  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_36);
  #else	
	LCD_SetFontSize(FONT_SIZE_32);
  #endif
	sprintf(tmpbuf, "%d", 0);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(HR_NUM_X+(HR_NUM_W-w)/2, HR_NUM_Y+(HR_NUM_H-h)/2, tmpbuf);

	sprintf(tmpbuf, "%d", hr_max);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(HR_UP_NUM_X+(HR_UP_NUM_W-w)/2, HR_UP_NUM_Y+(HR_UP_NUM_H-h)/2, tmpbuf);

	sprintf(tmpbuf, "%d", hr_min);
	LCD_MeasureString(tmpbuf,&w,&h);
	LCD_ShowString(HR_DOWN_NUM_X+(HR_DOWN_NUM_W-w)/2, HR_DOWN_NUM_Y+(HR_DOWN_NUM_H-h)/2, tmpbuf);
	
#else/*UI_STYLE_HEALTH_BAR*/

	uint8_t count=1;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
							IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};

	LCD_ShowImg_From_Flash(HR_ICON_X, HR_ICON_Y, IMG_HR_BIG_ICON_1_ADDR);
	LCD_ShowImg_From_Flash(HR_UP_ARRAW_X, HR_UP_ARRAW_Y, IMG_HR_UP_ARRAW_ADDR);
	LCD_ShowImg_From_Flash(HR_DOWN_ARRAW_X, HR_DOWN_ARRAW_Y, IMG_HR_DOWN_ARRAW_ADDR);

  #ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_28);
  #else	
	LCD_SetFontSize(FONT_SIZE_24);
  #endif

	mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)title_str[global_settings.language], MENU_NOTIFY_STR_MAX-8);
	LCD_MeasureUniString(tmpbuf,&w,&h);
	LCD_ShowUniString(HR_NOTIFY_X+(HR_NOTIFY_W-w)/2, HR_NOTIFY_Y, tmpbuf);

#if 0	//xb add 2024-03-14 The results of the last manual measurement are also included in the judgment.
	memset(&ppgdata, 0x00, sizeof(ppgdata));
	GetCurDayHrRecData(&ppgdata);
	p_hr = (hr_rec2_nod*)&ppgdata;
	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		if(p_hr->year == 0x0000
			|| p_hr->month == 0x00
			|| p_hr->day == 0x00
			)
		{
			break;
		}
		
		if((p_hr->hr >= PPG_HR_MIN) && (p_hr->hr <= PPG_HR_MAX))
		{
			if((hr_max == 0) && (hr_min == 0))
			{
				hr_max = p_hr->hr;
				hr_min = p_hr->hr;
			}
			else
			{
				if(p_hr->hr > hr_max)
					hr_max = p_hr->hr;
				if(p_hr->hr < hr_min)
					hr_min = p_hr->hr;
			}
		}

		p_hr++;
	}
	PPGShowNumByImg(HR_UP_STR_X, HR_UP_STR_Y, HR_UP_STR_W, HR_UP_STR_H, HR_UP_NUM_W, img_num, hr_max);
	PPGShowNumByImg(HR_DOWN_STR_X, HR_DOWN_STR_Y, HR_DOWN_STR_W, HR_DOWN_STR_H, HR_UP_NUM_W, img_num, hr_min);
#else
	PPGShowNumByImg(HR_UP_STR_X, HR_UP_STR_Y, HR_UP_STR_W, HR_UP_STR_H, HR_UP_NUM_W, img_num, last_health.hr_max);
	PPGShowNumByImg(HR_DOWN_STR_X, HR_DOWN_STR_Y, HR_DOWN_STR_W, HR_DOWN_STR_H, HR_UP_NUM_W, img_num, last_health.hr_min);
#endif

	k_timer_start(&ppg_status_timer, K_SECONDS(2), K_NO_WAIT);

#endif/*UI_STYLE_HEALTH_BAR*/
}

void HRScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_HR].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_HR].status = SCREEN_STATUS_CREATED;

		LCD_Clear(BLACK);
	#ifndef UI_STYLE_HEALTH_BAR	
		IdleShowSignal();
		IdleShowNetMode();
		IdleShowBatSoc();
	#endif
		HRShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
	#ifndef UI_STYLE_HEALTH_BAR
		if(scr_msg[SCREEN_ID_HR].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_HR].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
		}
		if(scr_msg[SCREEN_ID_HR].para&SCREEN_EVENT_UPDATE_NET_MODE)
		{
			scr_msg[SCREEN_ID_HR].para &= (~SCREEN_EVENT_UPDATE_NET_MODE);	
			IdleShowNetMode();
		}
		if(scr_msg[SCREEN_ID_HR].para&SCREEN_EVENT_UPDATE_BAT)
		{
			scr_msg[SCREEN_ID_HR].para &= (~SCREEN_EVENT_UPDATE_BAT);
			IdleUpdateBatSoc();
		}
	#endif
		if(scr_msg[SCREEN_ID_HR].para&SCREEN_EVENT_UPDATE_HR)
		{
			scr_msg[SCREEN_ID_HR].para &= (~SCREEN_EVENT_UPDATE_HR);
			HRUpdateStatus();
		}
		break;
	}
	
	scr_msg[SCREEN_ID_HR].act = SCREEN_ACTION_NO;
}

void ExitHRScreen(void)
{
	k_timer_stop(&mainmenu_timer);
#ifndef UI_STYLE_HEALTH_BAR	
	k_timer_stop(&ppg_status_timer);
#endif
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
#ifdef UI_STYLE_HEALTH_BAR
	k_timer_start(&mainmenu_timer, K_SECONDS(3), K_NO_WAIT);
#else
	k_timer_stop(&ppg_status_timer);
  #ifdef CONFIG_TEMP_SUPPORT
	k_timer_stop(&temp_status_timer);
  #endif
#endif

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
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
#ifndef UI_STYLE_HEALTH_BAR
	g_ppg_status = PPG_STATUS_PREPARE;
	ppg_retry_left = 2;
#endif

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

  #if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
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
#endif/*CONFIG_PPG_SUPPORT*/

static uint16_t str_x,str_y,str_w,str_h;
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
	uint32_t len;

	k_timer_stop(&notify_timer);
	k_timer_stop(&mainmenu_timer);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
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

	len = mmi_ucs2strlen((uint8_t*)notify_msg.text);
	if(len > NOTIFY_TEXT_MAX_LEN)
		len = NOTIFY_TEXT_MAX_LEN;
	
	if(notify_msg.type == NOTIFY_TYPE_POPUP)
	{
		k_timer_start(&notify_timer, K_SECONDS(NOTIFY_TIMER_INTERVAL), K_NO_WAIT);
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

void ShowStringsInRect(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint8_t *strbuf)
{
	uint16_t x,y,w,h;
	uint16_t offset_w=4,offset_h=4;

	LCD_MeasureString(strbuf, &w, &h);

	if(w > (rect_w-2*offset_w))
	{
		uint8_t line_count,line_no,line_max;
		uint16_t line_h=(h+offset_h);
		uint16_t byte_no=0,text_len;

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
			uint8_t tmpbuf[128] = {0};
			uint8_t i=0;

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

void NotifyShowStrings(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t *anima_img, uint8_t anima_count, uint8_t *strbuf)
{
	LCD_DrawRectangle(rect_x, rect_y, rect_w, rect_h);
	LCD_Fill(rect_x+1, rect_y+1, rect_w-2, rect_h-2, BLACK);
	ShowStringsInRect(rect_x, rect_y, rect_w, rect_h, strbuf);	
}

void NotifyUpdate(void)
{
	uint16_t x,y,w=0,h=0;
	uint16_t offset_w=4,offset_h=4;

	switch(notify_msg.align)
	{
	case NOTIFY_ALIGN_CENTER:
		if(scr_msg[SCREEN_ID_NOTIFY].para&SCREEN_EVENT_UPDATE_POP_IMG)
		{
			scr_msg[SCREEN_ID_NOTIFY].para &= (~SCREEN_EVENT_UPDATE_POP_IMG);

		#ifdef CONFIG_ANIMATION_SUPPORT
			AnimaStop();
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
				uint8_t line_count,line_no,line_max;
				uint16_t line_h=(h+offset_h);
				uint16_t byte_no=0,text_len;

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
					uint16_t tmpbuf[128] = {0};
					uint8_t i=0;

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
	uint16_t x,y,w=0,h=0;
	uint16_t offset_w=4,offset_h=4;

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
			uint8_t line_count,line_no,line_max;
			uint16_t line_h=(h+offset_h);
			uint16_t byte_no=0,text_len;

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
				uint16_t tmpbuf[128] = {0};
				uint8_t i=0;

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
	uint16_t x,y,w,h;
	uint8_t str_title[128] = {0};

	LCD_Clear(BLACK);

	switch(g_dl_data_type)
	{
	case DL_DATA_IMG:
		strcpy(str_title, "UI OTA");
		break;
	case DL_DATA_FONT:
		strcpy(str_title, "FONT OTA");
		break;
	case DL_DATA_PPG:
		strcpy(str_title, "PPG_AG OTA");
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
	k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
}

void DlUpdateStatus(void)
{
	uint16_t pro_len;
	uint16_t x,y,w,h;
	uint8_t pro_buf[16] = {0};
	uint8_t strbuf[256] = {0};
	static bool flag = false;
	static uint16_t pro_str_x,pro_str_y;

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
							  "Upgrading......");
			
			LCD_DrawRectangle(DL_NOTIFY_PRO_X, DL_NOTIFY_PRO_Y, DL_NOTIFY_PRO_W, DL_NOTIFY_PRO_H);
			LCD_Fill(DL_NOTIFY_PRO_X+1, DL_NOTIFY_PRO_Y+1, DL_NOTIFY_PRO_W-1, DL_NOTIFY_PRO_H-1, BLACK);

			sprintf(pro_buf, "%d%%", g_dl_progress);
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

			sprintf(pro_buf, "%d%%", g_dl_progress);
			memset(strbuf, 0x00, sizeof(strbuf));
			mmi_asc_to_ucs2(strbuf, pro_buf);
			LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
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
		LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
		x = DL_NOTIFY_YES_X+(DL_NOTIFY_YES_W-w)/2;
		y = DL_NOTIFY_YES_Y+(DL_NOTIFY_YES_H-h)/2;	
		LCD_ShowUniString(x,y,(uint16_t*)strbuf);

		LCD_DrawRectangle(DL_NOTIFY_NO_X, DL_NOTIFY_NO_Y, DL_NOTIFY_NO_W, DL_NOTIFY_NO_H);
		mmi_asc_to_ucs2(strbuf, "PWR(N)");
		LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
		x = DL_NOTIFY_NO_X+(DL_NOTIFY_NO_W-w)/2;
		y = DL_NOTIFY_NO_Y+(DL_NOTIFY_NO_H-h)/2;	
		LCD_ShowUniString(x,y,(uint16_t*)strbuf);

		SetRightKeyUpHandler(dl_reboot_confirm);
		SetLeftKeyUpHandler(dl_exit);
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_YES_X-10, DL_NOTIFY_YES_X+DL_NOTIFY_YES_W+10, DL_NOTIFY_YES_Y-10, DL_NOTIFY_YES_Y+DL_NOTIFY_YES_H+10, dl_reboot_confirm);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, DL_NOTIFY_NO_X-10, DL_NOTIFY_NO_X+DL_NOTIFY_NO_W+10, DL_NOTIFY_NO_Y-10, DL_NOTIFY_NO_Y+DL_NOTIFY_NO_H+10, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_prev);
	#endif	

		k_timer_stop(&mainmenu_timer);
		k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
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
		LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
		x = (LCD_WIDTH-DL_NOTIFY_YES_W)/2+(DL_NOTIFY_YES_W-w)/2;
		y = DL_NOTIFY_YES_Y+(DL_NOTIFY_YES_H-h)/2;	
		LCD_ShowUniString(x,y,(uint16_t*)strbuf);

		SetLeftKeyUpHandler(dl_exit);
		SetRightKeyUpHandler(dl_exit);
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, (LCD_WIDTH-DL_NOTIFY_YES_W)/2-10, (LCD_WIDTH-DL_NOTIFY_YES_W)/2+DL_NOTIFY_YES_W+10, DL_NOTIFY_YES_Y-10, DL_NOTIFY_YES_Y+DL_NOTIFY_YES_H+10, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_exit);
		register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, dl_prev);	
	#endif	

		k_timer_stop(&mainmenu_timer);
		k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
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
	if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
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
	if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
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

#if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
void PrevDlPpgScreen(void)
{
	if((strcmp(g_new_font_ver,g_font_ver) != 0) && (strlen(g_new_font_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
		dl_font_start();
	else if((strcmp(g_new_ui_ver,g_ui_ver) != 0) && (strlen(g_new_ui_ver) > 0) && (strcmp(g_new_fw_ver, g_fw_version) == 0))
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
	AnimaStop();
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
	uint16_t x,y,w,h;
	uint16_t tmpbuf[128] = {0};
	uint16_t str_notify[LANGUAGE_MAX][42] = {
											#ifndef FW_FOR_CN
												{0x0043,0x006F,0x006E,0x0074,0x0069,0x006E,0x0075,0x0065,0x0020,0x0074,0x006F,0x0020,0x0075,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0074,0x0068,0x0065,0x0020,0x0066,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x003F,0x0000},//Continue to upgrade the firmware?
												{0x0041,0x006B,0x0074,0x0075,0x0061,0x006C,0x0069,0x0073,0x0069,0x0065,0x0072,0x0075,0x006E,0x0067,0x0020,0x0064,0x0065,0x0072,0x0020,0x0046,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x0020,0x0066,0x006F,0x0072,0x0074,0x0066,0x0061,0x0068,0x0072,0x0065,0x006E,0x003F,0x0000},//Aktualisierung der Firmware fortfahren?
												{0x0043,0x006F,0x006E,0x0074,0x0069,0x006E,0x0075,0x0065,0x0072,0x0020,0x006C,0x0061,0x0020,0x006D,0x0069,0x0073,0x0065,0x0020,0x00E0,0x0020,0x006A,0x006F,0x0075,0x0072,0x0020,0x0064,0x0075,0x0020,0x0066,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x003F,0x0000},//Continuer la mise à jour du firmware?
												{0x0043,0x006F,0x006E,0x0074,0x0069,0x006E,0x0075,0x0061,0x0020,0x0061,0x0020,0x0061,0x0067,0x0067,0x0069,0x006F,0x0072,0x006E,0x0061,0x0072,0x0065,0x0020,0x0069,0x006C,0x0020,0x0066,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x003F,0x0000},//Continua a aggiornare il firmware?
												{0x00BF,0x0053,0x0065,0x0067,0x0075,0x0069,0x0072,0x0020,0x0061,0x0063,0x0074,0x0075,0x0061,0x006C,0x0069,0x007A,0x0061,0x006E,0x0064,0x006F,0x0020,0x0065,0x006C,0x0020,0x0066,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x003F,0x0000},//?Seguir actualizando el firmware?
												{0x0043,0x006F,0x006E,0x0074,0x0069,0x006E,0x0075,0x0061,0x0072,0x0020,0x0061,0x0074,0x0075,0x0061,0x006C,0x0069,0x007A,0x0061,0x006E,0x0064,0x006F,0x0020,0x006F,0x0020,0x0066,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x003F,0x0000},//Continuar atualizando o firmware?
											#else
												{0x7EE7,0x7EED,0x5347,0x7EA7,0x56FA,0x4EF6,0xFF1F,0x0000},//继续升级固件？
												{0x0043,0x006F,0x006E,0x0074,0x0069,0x006E,0x0075,0x0065,0x0020,0x0074,0x006F,0x0020,0x0075,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0074,0x0068,0x0065,0x0020,0x0066,0x0069,0x0072,0x006D,0x0077,0x0061,0x0072,0x0065,0x003F,0x0000},//Continue to upgrade the firmware?
											#endif
											};

	LCD_Clear(BLACK);
	
	LCD_ShowImg_From_Flash(FOTA_LOGO_X, FOTA_LOGO_Y, IMG_OTA_LOGO_ADDR);
	LCD_ShowImg_From_Flash(FOTA_YES_X, FOTA_YES_Y, IMG_OTA_YES_ADDR);
	LCD_ShowImg_From_Flash(FOTA_NO_X, FOTA_NO_Y, IMG_OTA_NO_ADDR);

#ifdef FONTMAKER_UNICODE_FONT
	switch(global_settings.language)
	{
	case LANGUAGE_EN:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*12);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][12], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-12));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;

  #ifndef FW_FOR_CN		
	case LANGUAGE_DE:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*19);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][19], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-19));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;
	case LANGUAGE_FR:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*20);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][20], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-20));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;
	case LANGUAGE_ITA:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*11);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][11], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-11));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;
	case LANGUAGE_ES:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*21);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][21], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-21));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;
	case LANGUAGE_PT:
		memcpy(tmpbuf, &str_notify[global_settings.language][0], 2*22);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y, tmpbuf);
		
		memset(tmpbuf, 0x0000, sizeof(tmpbuf));
		memcpy(tmpbuf, &str_notify[global_settings.language][22], 2*(mmi_ucs2strlen(str_notify[global_settings.language])-22));
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+h+2, tmpbuf);
		break;
		
  #else
  	case LANGUAGE_CHN:
		LCD_MeasureUniString(str_notify[global_settings.language], &w, &h);
		LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+(FOTA_START_STR_H-h)/2, str_notify[global_settings.language]);
		break;
  #endif
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
	uint16_t pro_len;
	uint16_t x,y,w,h;
	uint8_t pro_buf[16] = {0};
	uint16_t tmpbuf[128] = {0};
	static bool flag = false;
	static uint16_t pro_str_x,pro_str_y;

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
			uint16_t str_notify[LANGUAGE_MAX][28] = {
													#ifndef FW_FOR_CN
														{0x004C,0x0069,0x006E,0x006B,0x0069,0x006E,0x0067,0x0020,0x0074,0x006F,0x0020,0x0073,0x0065,0x0072,0x0076,0x0065,0x0072,0x002E,0x002E,0x002E,0x0000},//Linking to server...
														{0x0056,0x0065,0x0072,0x006C,0x0069,0x006E,0x006B,0x0075,0x006E,0x0067,0x0020,0x007A,0x0075,0x006D,0x0020,0x0053,0x0065,0x0072,0x0076,0x0065,0x0072,0x0020,0x002E,0x002E,0x002E,0x0000},//Verlinkung zum Server ...
														{0x004C,0x0069,0x0065,0x006E,0x0020,0x0076,0x0065,0x0072,0x0073,0x0020,0x006C,0x0065,0x0020,0x0073,0x0065,0x0072,0x0076,0x0065,0x0075,0x0072,0x0020,0x002E,0x002E,0x002E,0x0000},//Lien vers le serveur ...
														{0x0043,0x006F,0x006C,0x006C,0x0065,0x0067,0x0061,0x006D,0x0065,0x006E,0x0074,0x006F,0x0020,0x0061,0x006C,0x0020,0x0073,0x0065,0x0072,0x0076,0x0065,0x0072,0x0020,0x002E,0x002E,0x002E,0x0000},//Collegamento al server ...
														{0x0045,0x006E,0x006C,0x0061,0x0063,0x0065,0x0020,0x0061,0x006C,0x0020,0x0073,0x0065,0x0072,0x0076,0x0069,0x0064,0x006F,0x0072,0x0020,0x002E,0x002E,0x002E,0x0000},//Enlace al servidor ...
														{0x0056,0x0069,0x006E,0x0063,0x0075,0x006C,0x0061,0x006E,0x0064,0x006F,0x0020,0x0061,0x006F,0x0020,0x0073,0x0065,0x0072,0x0076,0x0069,0x0064,0x006F,0x0072,0x0020,0x002E,0x002E,0x002E,0x0000},//Vinculando ao servidor ...
													#else
														{0x8FDE,0x63A5,0x670D,0x52A1,0x5668,0x002E,0x002E,0x002E,0x0000},//连接服务器...
														{0x004C,0x0069,0x006E,0x006B,0x0069,0x006E,0x0067,0x0020,0x0074,0x006F,0x0020,0x0073,0x0065,0x0072,0x0076,0x0065,0x0072,0x002E,0x002E,0x002E,0x0000},//Linking to server...
													#endif
													};

			LCD_Fill(0, FOTA_YES_Y, LCD_WIDTH, FOTA_YES_H, BLACK);
			LCD_Fill(0, FOTA_START_STR_Y, LCD_WIDTH, FOTA_START_STR_H, BLACK);

		#ifdef FONTMAKER_UNICODE_FONT
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_notify[global_settings.language], MENU_NOTIFY_STR_MAX);
			LCD_MeasureUniString(tmpbuf, &w, &h);
			LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+(FOTA_START_STR_H-h)/2, tmpbuf);
		#endif

			ClearAllKeyHandler();
		}
		break;
		
	case FOTA_STATUS_DOWNLOADING:
		if(!flag)
		{
			uint16_t str_notify[LANGUAGE_MAX][32] = {
													#ifndef FW_FOR_CN
														{0x0044,0x0061,0x0074,0x0061,0x0020,0x0064,0x006F,0x0077,0x006E,0x006C,0x006F,0x0061,0x0064,0x0069,0x006E,0x0067,0x002E,0x002E,0x002E,0x0000},//Data downloading...
														{0x0044,0x0061,0x0074,0x0065,0x006E,0x0020,0x0068,0x0065,0x0072,0x0075,0x006E,0x0074,0x0065,0x0072,0x006C,0x0061,0x0064,0x0065,0x006E,0x0020,0x002E,0x002E,0x002E,0x0000},//Daten herunterladen ...
														{0x0054,0x00E9,0x006C,0x00E9,0x0063,0x0068,0x0061,0x0072,0x0067,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0064,0x0065,0x0073,0x0020,0x0064,0x006F,0x006E,0x006E,0x00E9,0x0065,0x0073,0x0020,0x002E,0x002E,0x002E,0x0000},//Téléchargement des données ...
														{0x0053,0x0063,0x0061,0x0072,0x0069,0x0063,0x0061,0x0072,0x0065,0x0020,0x0069,0x0020,0x0064,0x0061,0x0074,0x0069,0x0020,0x002E,0x002E,0x002E,0x0000},//Scaricare i dati ...
														{0x0044,0x0065,0x0073,0x0063,0x0061,0x0072,0x0067,0x0061,0x0020,0x0064,0x0065,0x0020,0x0064,0x0061,0x0074,0x006F,0x0073,0x0020,0x002E,0x002E,0x002E,0x0000},//Descarga de datos ...
														{0x0042,0x0061,0x0069,0x0078,0x0061,0x006E,0x0064,0x006F,0x0020,0x0064,0x0061,0x0064,0x006F,0x0073,0x0020,0x002E,0x002E,0x002E,0x0000},//Baixando dados ...
													#else
														{0x6570,0x636E,0x4E0B,0x8F7D,0x4E2D,0x002E,0x002E,0x002E,0x0000},//数据下载中…
														{0x0044,0x0061,0x0074,0x0061,0x0020,0x0064,0x006F,0x0077,0x006E,0x006C,0x006F,0x0061,0x0064,0x0069,0x006E,0x0067,0x002E,0x002E,0x002E,0x0000},//Data downloading...
													#endif
													};
			
			flag = true;
			
			LCD_Fill(0, FOTA_START_STR_Y, LCD_WIDTH, FOTA_START_STR_H, BLACK);

			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_notify[global_settings.language], MENU_NOTIFY_STR_MAX);
			LCD_MeasureUniString(tmpbuf, &w, &h);
			LCD_ShowUniString(FOTA_START_STR_X+(FOTA_START_STR_W-w)/2, FOTA_START_STR_Y+(FOTA_START_STR_H-h)/2, tmpbuf);
					
			LCD_DrawRectangle(FOTA_PROGRESS_X, FOTA_PROGRESS_Y, FOTA_PROGRESS_W, FOTA_PROGRESS_H);
			LCD_Fill(FOTA_PROGRESS_X+1, FOTA_PROGRESS_Y+1, FOTA_PROGRESS_W-1, FOTA_PROGRESS_H-1, BLACK);
			
			sprintf(pro_buf, "%d%%", g_fota_progress);
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
			
			sprintf(pro_buf, "%d%%", g_fota_progress);
			memset(tmpbuf, 0x0000, sizeof(tmpbuf));
			mmi_asc_to_ucs2(tmpbuf, pro_buf);
			LCD_MeasureUniString(tmpbuf, &w, &h);
			LCD_ShowUniString(pro_str_x, pro_str_y, tmpbuf);
		}		

		ClearAllKeyHandler();
		break;
		
	case FOTA_STATUS_FINISHED:
		{
			static bool is_finished = false;
			uint16_t str_notify[LANGUAGE_MAX][25] = {
													#ifndef FW_FOR_CN
														{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0063,0x006F,0x006D,0x0070,0x006C,0x0065,0x0074,0x0065,0x0064,0x0000},//Upgrade completed
														{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0065,0x0072,0x0066,0x006F,0x006C,0x0067,0x0072,0x0065,0x0069,0x0063,0x0068,0x0000},//Upgrade erfolgreich
														{0x004D,0x0069,0x0073,0x0065,0x0020,0x00E0,0x0020,0x006E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0074,0x0065,0x0072,0x006D,0x0069,0x006E,0x00E9,0x0065,0x0000},//Mise à niveau terminée
														{0x0041,0x0067,0x0067,0x0069,0x006F,0x0072,0x006E,0x0061,0x006D,0x0065,0x006E,0x0074,0x006F,0x0020,0x0072,0x0069,0x0075,0x0073,0x0063,0x0069,0x0074,0x006F,0x0000},//Aggiornamento riuscito
														{0x0041,0x0063,0x0074,0x0075,0x0061,0x006C,0x0069,0x007A,0x0061,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0065,0x0078,0x0069,0x0074,0x006F,0x0073,0x0061,0x0000},//Actualización exitosa
														{0x0041,0x0074,0x0075,0x0061,0x006C,0x0069,0x007A,0x0061,0x0072,0x0020,0x0062,0x0065,0x006D,0x0020,0x002D,0x0073,0x0075,0x0063,0x0065,0x0064,0x0069,0x0064,0x006F,0x0000},//Atualizar bem -sucedido
													#else
														{0x5347,0x7EA7,0x6210,0x529F,0x0000},//升级成功
														{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0063,0x006F,0x006D,0x0070,0x006C,0x0065,0x0074,0x0065,0x0064,0x0000},//Upgrade completed
													#endif
													};
			
			flag = false;

			if(!is_finished)
			{
				is_finished = true;
				
				LCD_Clear(BLACK);
				LCD_ShowImg_From_Flash(FOTA_FINISH_ICON_X, FOTA_FINISH_ICON_Y, IMG_OTA_FINISH_ICON_ADDR);
				
			#ifdef FONTMAKER_UNICODE_FONT
				mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_notify[global_settings.language], MENU_NOTIFY_STR_MAX);
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(FOTA_FINISH_STR_X+(FOTA_FINISH_STR_W-w)/2, FOTA_FINISH_STR_Y+(FOTA_FINISH_STR_H-h)/2, tmpbuf);
			#endif
			
				SetLeftKeyUpHandler(fota_reboot_confirm);
				SetRightKeyUpHandler(fota_reboot_confirm);
			#ifdef CONFIG_TOUCH_SUPPORT
				register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, fota_reboot_confirm);
			#endif
			}
		}
		break;
		
	case FOTA_STATUS_ERROR:
		{
			uint16_t str_notify[LANGUAGE_MAX][27] = {
													#ifndef FW_FOR_CN
														{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0066,0x0061,0x0069,0x006C,0x0065,0x0064,0x0000},//Upgrade failed
														{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0066,0x0065,0x0068,0x006C,0x0067,0x0065,0x0073,0x0063,0x0068,0x006C,0x0061,0x0067,0x0065,0x006E,0x0000},//Upgrade fehlgeschlagen
														{0x004C,0x0061,0x0020,0x006D,0x0069,0x0073,0x0065,0x0020,0x00E0,0x0020,0x006E,0x0069,0x0076,0x0065,0x0061,0x0075,0x0020,0x0061,0x0020,0x00E9,0x0063,0x0068,0x006F,0x0075,0x00E9,0x0000},//La mise à niveau a échoué
														{0x0041,0x0067,0x0067,0x0069,0x006F,0x0072,0x006E,0x0061,0x006D,0x0065,0x006E,0x0074,0x006F,0x0020,0x0066,0x0061,0x006C,0x006C,0x0069,0x0074,0x006F,0x0000},//Aggiornamento fallito
														{0x0045,0x0072,0x0072,0x006F,0x0072,0x0020,0x0064,0x0065,0x0020,0x0061,0x0063,0x0074,0x0075,0x0061,0x006C,0x0069,0x007A,0x0061,0x0063,0x0069,0x00F3,0x006E,0x0000},//Error de actualización
														{0x0046,0x0061,0x006C,0x0068,0x0061,0x0020,0x006E,0x0061,0x0020,0x0061,0x0074,0x0075,0x0061,0x006C,0x0069,0x007A,0x0061,0x00E7,0x00E3,0x006F,0x0000},//Falha na atualiza??o
													#else
														{0x5347,0x7EA7,0x5931,0x8D25,0x0000},//升级失败
														{0x0055,0x0070,0x0067,0x0072,0x0061,0x0064,0x0065,0x0020,0x0066,0x0061,0x0069,0x006C,0x0065,0x0064,0x0000},//Upgrade failed
													#endif
													};
			
			flag = false;

			LCD_Clear(BLACK);
			LCD_ShowImg_From_Flash(FOTA_FAIL_ICON_X, FOTA_FAIL_ICON_Y, IMG_OTA_FAILED_ICON_ADDR);
			
		#ifdef FONTMAKER_UNICODE_FONT
			mmi_ucs2smartcpy((uint8_t*)tmpbuf, (uint8_t*)str_notify[global_settings.language], MENU_NOTIFY_STR_MAX);
			LCD_MeasureUniString(tmpbuf, &w, &h);
			LCD_ShowUniString(FOTA_FAIL_STR_X+(FOTA_FAIL_STR_W-w)/2, FOTA_FAIL_STR_Y+(FOTA_FAIL_STR_H-h)/2, tmpbuf);
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
  #if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
	else if((strcmp(g_new_ppg_ver,g_ppg_algo_ver) != 0) && (strlen(g_new_ppg_ver) > 0))
	{
		dl_ppg_start();
	}
  #endif
	else
#endif
	{
		EntryIdleScr();
	}
}

void EnterFOTAScreen(void)
{
	if(screen_id == SCREEN_ID_FOTA)
		return;

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStop();
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
void FallUpdateStatus(void)
{
}

void FallShowStatus(void)
{
	LCD_Clear(BLACK);

	LCD_ShowImg_From_Flash(FALL_ICON_X, FALL_ICON_Y, IMG_FALL_ICON_ADDR);
	LCD_ShowImg_From_Flash(FALL_YES_X, FALL_YES_Y, IMG_FALL_YES_ADDR);
	LCD_ShowImg_From_Flash(FALL_NO_X, FALL_NO_Y, IMG_FALL_NO_ADDR);

	FallChangrStatus();

	//SetLeftKeyUpHandler(FallAlarmConfirm);
	//SetRightKeyUpHandler(FallAlarmCancel);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FALL_YES_X, FALL_YES_X+FALL_YES_W, FALL_YES_Y, FALL_YES_Y+FALL_YES_H, FallAlarmConfirm);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FALL_NO_X, FALL_NO_X+FALL_NO_W, FALL_NO_Y, FALL_NO_Y+FALL_NO_H, FallAlarmCancel);
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
		FallUpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_FALL].act = SCREEN_ACTION_NO;
}

void EnterFallScreen(void)
{
	if(screen_id == SCREEN_ID_FALL)
		return;

	k_timer_stop(&notify_timer);
	k_timer_stop(&mainmenu_timer);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
#endif
#ifdef NB_SIGNAL_TEST
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

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FALL;	
	scr_msg[SCREEN_ID_FALL].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FALL].status = SCREEN_STATUS_CREATING;

	ClearAllKeyHandler();
}
#endif/*CONFIG_FALL_DETECT_SUPPORT*/

#ifdef CONFIG_SLEEP_SUPPORT
void SleepUpdateStatus(void)
{
	uint16_t total_sleep,deep_sleep,light_sleep;
	uint32_t img_big_num[10] = {IMG_FONT_38_NUM_0_ADDR,IMG_FONT_38_NUM_1_ADDR,IMG_FONT_38_NUM_2_ADDR,IMG_FONT_38_NUM_3_ADDR,IMG_FONT_38_NUM_4_ADDR,
								IMG_FONT_38_NUM_5_ADDR,IMG_FONT_38_NUM_6_ADDR,IMG_FONT_38_NUM_7_ADDR,IMG_FONT_38_NUM_8_ADDR,IMG_FONT_38_NUM_9_ADDR};
	uint32_t img_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
							IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};
				

	GetSleepTimeData(&deep_sleep, &light_sleep);
	total_sleep = deep_sleep+light_sleep;

	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_HR_X+0*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_HR_Y, img_big_num[(total_sleep/60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_HR_X+1*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_HR_Y, img_big_num[(total_sleep/60)%10]);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_MIN_X+0*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_MIN_Y, img_big_num[(total_sleep%60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_MIN_X+1*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_MIN_Y, img_big_num[(total_sleep%60)%10]);

	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_HR_X+0*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_HR_Y, img_num[(deep_sleep/60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_HR_X+1*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_HR_Y, img_num[(deep_sleep/60)%10]);
	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_MIN_X+0*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_MIN_Y, img_num[(deep_sleep%60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_MIN_X+1*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_MIN_Y, img_num[(deep_sleep%60)%10]);

	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_HR_X+0*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_HR_Y, img_num[(light_sleep/60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_HR_X+1*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_HR_Y, img_num[(light_sleep/60)%10]);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_MIN_X+0*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_MIN_Y, img_num[(light_sleep%60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_MIN_X+1*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_MIN_Y, img_num[(light_sleep%60)%10]);
}

void SleepShowStatus(void)
{
	uint8_t strbuf[64] = {0};
	uint16_t total_sleep,deep_sleep,light_sleep;
	uint32_t img_big_num[10] = {IMG_FONT_38_NUM_0_ADDR,IMG_FONT_38_NUM_1_ADDR,IMG_FONT_38_NUM_2_ADDR,IMG_FONT_38_NUM_3_ADDR,IMG_FONT_38_NUM_4_ADDR,
							IMG_FONT_38_NUM_5_ADDR,IMG_FONT_38_NUM_6_ADDR,IMG_FONT_38_NUM_7_ADDR,IMG_FONT_38_NUM_8_ADDR,IMG_FONT_38_NUM_9_ADDR};
	uint32_t img_num[10] = {IMG_FONT_24_NUM_0_ADDR,IMG_FONT_24_NUM_1_ADDR,IMG_FONT_24_NUM_2_ADDR,IMG_FONT_24_NUM_3_ADDR,IMG_FONT_24_NUM_4_ADDR,
							IMG_FONT_24_NUM_5_ADDR,IMG_FONT_24_NUM_6_ADDR,IMG_FONT_24_NUM_7_ADDR,IMG_FONT_24_NUM_8_ADDR,IMG_FONT_24_NUM_9_ADDR};

	
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_ICON_X, SLEEP_TOTAL_ICON_Y, IMG_SLEEP_ANI_3_ADDR);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_UNIT_HR_X, SLEEP_TOTAL_UNIT_HR_Y, IMG_SLEEP_BIG_H_ADDR);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_UNIT_MIN_X, SLEEP_TOTAL_UNIT_MIN_Y, IMG_SLEEP_BIG_M_ADDR);
	LCD_ShowImg_From_Flash(SLEEP_SEP_LINE_X, SLEEP_SEP_LINE_Y, IMG_SLEEP_LINE_ADDR);

	LCD_ShowImg_From_Flash(SLEEP_DEEP_ICON_X, SLEEP_DEEP_ICON_Y, IMG_SLEEP_BEGIN_ADDR);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_ICON_X, SLEEP_LIGHT_ICON_Y, IMG_SLEEP_END_ADDR);

	GetSleepTimeData(&deep_sleep, &light_sleep);
	total_sleep = deep_sleep+light_sleep;

	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_HR_X+0*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_HR_Y, img_big_num[(total_sleep/60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_HR_X+1*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_HR_Y, img_big_num[(total_sleep/60)%10]);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_MIN_X+0*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_MIN_Y, img_big_num[(total_sleep%60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_TOTAL_STR_MIN_X+1*SLEEP_TOTAL_NUM_W, SLEEP_TOTAL_STR_MIN_Y, img_big_num[(total_sleep%60)%10]);

	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_HR_X+0*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_HR_Y, img_num[(deep_sleep/60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_HR_X+1*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_HR_Y, img_num[(deep_sleep/60)%10]);
	LCD_ShowImg_From_Flash(SLEEP_DEEP_UNIT_HR_X, SLEEP_DEEP_STR_HR_Y, IMG_FONT_24_COLON_ADDR);
	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_MIN_X+0*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_MIN_Y, img_num[(deep_sleep%60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_DEEP_STR_MIN_X+1*SLEEP_DEEP_NUM_W, SLEEP_DEEP_STR_MIN_Y, img_num[(deep_sleep%60)%10]);

	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_HR_X+0*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_HR_Y, img_num[(light_sleep/60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_HR_X+1*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_HR_Y, img_num[(light_sleep/60)%10]);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_UNIT_HR_X, SLEEP_DEEP_STR_HR_Y, IMG_FONT_24_COLON_ADDR);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_MIN_X+0*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_MIN_Y, img_num[(light_sleep%60)/10]);
	LCD_ShowImg_From_Flash(SLEEP_LIGHT_STR_MIN_X+1*SLEEP_LIGHT_NUM_W, SLEEP_LIGHT_STR_MIN_Y, img_num[(light_sleep%60)%10]);
}

void SleepScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_SLEEP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_SLEEP].status = SCREEN_STATUS_CREATED;

		LCD_Clear(BLACK);
		IdleShowSignal();
		IdleShowNetMode();
		IdleShowBatSoc();
		SleepShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_SLEEP].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_SLEEP].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
		}
		if(scr_msg[SCREEN_ID_SLEEP].para&SCREEN_EVENT_UPDATE_NET_MODE)
		{
			scr_msg[SCREEN_ID_SLEEP].para &= (~SCREEN_EVENT_UPDATE_NET_MODE);	
			IdleShowNetMode();
		}
		if(scr_msg[SCREEN_ID_SLEEP].para&SCREEN_EVENT_UPDATE_BAT)
		{
			scr_msg[SCREEN_ID_SLEEP].para &= (~SCREEN_EVENT_UPDATE_BAT);
			IdleUpdateBatSoc();
		}
		if(scr_msg[SCREEN_ID_SLEEP].para&SCREEN_EVENT_UPDATE_SPORT)
		{
			scr_msg[SCREEN_ID_SLEEP].para &= (~SCREEN_EVENT_UPDATE_SPORT);
			SleepUpdateStatus();
		}
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

	k_timer_stop(&mainmenu_timer);
#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStop();
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

	screen_id = SCREEN_ID_SLEEP;	
	scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SLEEP].status = SCREEN_STATUS_CREATING;

#if defined(CONFIG_PPG_SUPPORT)
	SetLeftKeyUpHandler(EnterHRScreen);
#elif defined(CONFIG_TEMP_SUPPORT)
	SetLeftKeyUpHandler(EnterTempScreen);
#elif defined(CONFIG_SYNC_SUPPORT)
	SetLeftKeyUpHandler(EnterSyncDataScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
	SetRightKeyUpHandler(ExitSleepScreen);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();

 #ifdef CONFIG_PPG_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
 #elif defined(CONFIG_TEMP_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
 #elif defined(CONFIG_SYNC_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen); 
 #else
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
 #endif

 #ifdef CONFIG_STEP_SUPPORT
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
 #else
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
 #endif
#endif
}
#endif/*CONFIG_SLEEP_SUPPORT*/

#ifdef CONFIG_STEP_SUPPORT
void StepUpdateStatus(void)
{
	uint8_t i,language,count=1;
	uint16_t steps,calorie,distance;
	uint16_t total_sleep,deep_sleep,light_sleep;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_38_NUM_0_ADDR,IMG_FONT_38_NUM_1_ADDR,IMG_FONT_38_NUM_2_ADDR,IMG_FONT_38_NUM_3_ADDR,IMG_FONT_38_NUM_4_ADDR,
								IMG_FONT_38_NUM_5_ADDR,IMG_FONT_38_NUM_6_ADDR,IMG_FONT_38_NUM_7_ADDR,IMG_FONT_38_NUM_8_ADDR,IMG_FONT_38_NUM_9_ADDR};
	uint32_t step_unit[2] = {IMG_STEP_UNIT_EN_ICON_ADDR,IMG_STEP_UNIT_CN_ICON_ADDR};
	uint32_t cal_uint[2] = {IMG_STEP_KCAL_EN_ADDR,IMG_STEP_KCAL_CN_ADDR};
	
	switch(global_settings.language)
	{
	#ifdef FW_FOR_CN	
	case LANGUAGE_CHN:
		language = 1;
		break;
	#endif

	default:
		language = 0;
		break;
	}

	LCD_Fill(IMU_STEP_STR_X, IMU_STEP_STR_Y, LCD_WIDTH, IMU_NUM_H, BLACK);
	LCD_Fill(IMU_CAL_STR_X, IMU_CAL_STR_X, LCD_WIDTH, IMU_NUM_H, BLACK);

	GetSportData(&steps, &calorie, &distance);

	divisor = 10000;
	for(i=0;i<5;i++)
	{
		LCD_ShowImg_From_Flash(IMU_STEP_STR_X+(IMU_STEP_STR_W-5*IMU_NUM_W)/2+i*IMU_NUM_W, IMU_STEP_STR_Y, img_num[steps/divisor]);
		steps = steps%divisor;
		divisor = divisor/10;
	}
	LCD_ShowImg_From_Flash(IMU_STEP_STR_X+(IMU_STEP_STR_W-5*IMU_NUM_W)/2+5*IMU_NUM_W, IMU_STEP_UNIT_Y, step_unit[language]);

	divisor = 1000;
	for(i=0;i<4;i++)
	{
		LCD_ShowImg_From_Flash(IMU_CAL_STR_X+(IMU_CAL_STR_W-4*IMU_NUM_W)/2+i*IMU_NUM_W, IMU_CAL_STR_Y, img_num[calorie/divisor]);
		calorie = calorie%divisor;
		divisor = divisor/10;
	}
	LCD_ShowImg_From_Flash(IMU_CAL_STR_X+(IMU_CAL_STR_W-4*IMU_NUM_W)/2+i*IMU_NUM_W, IMU_CAL_UNIT_Y, cal_uint[language]);

#ifdef CONFIG_SLEEP_SUPPORT
	GetSleepTimeData(&deep_sleep, &light_sleep);
	total_sleep = deep_sleep+light_sleep;
	LCD_ShowImg_From_Flash(IMU_SLEEP_H_STR_X+0*IMU_NUM_W, IMU_SLEEP_H_STR_Y, img_num[(total_sleep/60)/10]);
	LCD_ShowImg_From_Flash(IMU_SLEEP_H_STR_X+1*IMU_NUM_W, IMU_SLEEP_H_STR_Y, img_num[(total_sleep/60)%10]);
	LCD_ShowImg_From_Flash(IMU_SLEEP_M_STR_X+0*IMU_NUM_W, IMU_SLEEP_M_STR_Y, img_num[(total_sleep%60)/10]);
	LCD_ShowImg_From_Flash(IMU_SLEEP_M_STR_X+1*IMU_NUM_W, IMU_SLEEP_M_STR_Y, img_num[(total_sleep%60)%10]);
#endif	
}

void StepShowStatus(void)
{
	uint8_t i,language,count=1;
	uint16_t steps,calorie,distance;
	uint16_t total_sleep,deep_sleep,light_sleep;
	uint32_t divisor=10;
	uint32_t img_num[10] = {IMG_FONT_38_NUM_0_ADDR,IMG_FONT_38_NUM_1_ADDR,IMG_FONT_38_NUM_2_ADDR,IMG_FONT_38_NUM_3_ADDR,IMG_FONT_38_NUM_4_ADDR,
							IMG_FONT_38_NUM_5_ADDR,IMG_FONT_38_NUM_6_ADDR,IMG_FONT_38_NUM_7_ADDR,IMG_FONT_38_NUM_8_ADDR,IMG_FONT_38_NUM_9_ADDR};
	uint32_t step_unit[2] = {IMG_STEP_UNIT_EN_ICON_ADDR,IMG_STEP_UNIT_CN_ICON_ADDR};
	uint32_t cal_uint[2] = {IMG_STEP_KCAL_EN_ADDR,IMG_STEP_KCAL_CN_ADDR};
	
	switch(global_settings.language)
	{
	#ifdef FW_FOR_CN	
	case LANGUAGE_CHN:
		language = 1;
		break;
	#endif

	default:
		language = 0;
		break;
	}

	LCD_ShowImg_From_Flash(IMU_STEP_ICON_X, IMU_STEP_ICON_Y, IMG_STEP_STEP_ICON_ADDR);
	LCD_ShowImg_From_Flash(IMU_CAL_ICON_X, IMU_CAL_ICON_Y, IMG_STEP_CAL_ICON_ADDR);

	GetSportData(&steps, &calorie, &distance);

	divisor = 10000;
	for(i=0;i<5;i++)
	{
		LCD_ShowImg_From_Flash(IMU_STEP_STR_X+(IMU_STEP_STR_W-5*IMU_NUM_W)/2+i*IMU_NUM_W, IMU_STEP_STR_Y, img_num[steps/divisor]);
		steps = steps%divisor;
		divisor = divisor/10;
	}
	LCD_ShowImg_From_Flash(IMU_STEP_STR_X+(IMU_STEP_STR_W-5*IMU_NUM_W)/2+5*IMU_NUM_W, IMU_STEP_UNIT_Y, step_unit[language]);

	divisor = 1000;
	for(i=0;i<4;i++)
	{
		LCD_ShowImg_From_Flash(IMU_CAL_STR_X+(IMU_CAL_STR_W-4*IMU_NUM_W)/2+i*IMU_NUM_W, IMU_CAL_STR_Y, img_num[calorie/divisor]);
		calorie = calorie%divisor;
		divisor = divisor/10;
	}
	LCD_ShowImg_From_Flash(IMU_CAL_STR_X+(IMU_CAL_STR_W-4*IMU_NUM_W)/2+i*IMU_NUM_W, IMU_CAL_UNIT_Y, cal_uint[language]);

#ifdef CONFIG_SLEEP_SUPPORT
	GetSleepTimeData(&deep_sleep, &light_sleep);
	total_sleep = deep_sleep+light_sleep;

	LCD_ShowImg_From_Flash(IMU_SLEEP_ICON_X, IMU_SLEEP_ICON_Y, IMG_STEP_SLEEP_ICON_ADDR);
	LCD_ShowImg_From_Flash(IMU_SLEEP_H_UNIT_X, IMU_SLEEP_H_UNIT_Y, IMG_SLEEP_H_ADDR);
	LCD_ShowImg_From_Flash(IMU_SLEEP_M_UNIT_X, IMU_SLEEP_M_UNIT_Y, IMG_SLEEP_M_ADDR);
	
	LCD_ShowImg_From_Flash(IMU_SLEEP_H_STR_X+0*IMU_NUM_W, IMU_SLEEP_H_STR_Y, img_num[(total_sleep/60)/10]);
	LCD_ShowImg_From_Flash(IMU_SLEEP_H_STR_X+1*IMU_NUM_W, IMU_SLEEP_H_STR_Y, img_num[(total_sleep/60)%10]);
	LCD_ShowImg_From_Flash(IMU_SLEEP_M_STR_X+0*IMU_NUM_W, IMU_SLEEP_M_STR_Y, img_num[(total_sleep%60)/10]);
	LCD_ShowImg_From_Flash(IMU_SLEEP_M_STR_X+1*IMU_NUM_W, IMU_SLEEP_M_STR_Y, img_num[(total_sleep%60)%10]);
#endif	
}

void StepsScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_STEPS].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_STEPS].status = SCREEN_STATUS_CREATED;

		LCD_Clear(BLACK);
		IdleShowSignal();
		IdleShowNetMode();
		IdleShowBatSoc();
		StepShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_STEPS].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_STEPS].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
		}
		if(scr_msg[SCREEN_ID_STEPS].para&SCREEN_EVENT_UPDATE_NET_MODE)
		{
			scr_msg[SCREEN_ID_STEPS].para &= (~SCREEN_EVENT_UPDATE_NET_MODE);	
			IdleShowNetMode();
		}
		if(scr_msg[SCREEN_ID_STEPS].para&SCREEN_EVENT_UPDATE_BAT)
		{
			scr_msg[SCREEN_ID_STEPS].para &= (~SCREEN_EVENT_UPDATE_BAT);
			IdleUpdateBatSoc();
		}
		if(scr_msg[SCREEN_ID_STEPS].para&SCREEN_EVENT_UPDATE_SPORT)
		{
			scr_msg[SCREEN_ID_STEPS].para &= (~SCREEN_EVENT_UPDATE_SPORT);
			StepUpdateStatus();
		}
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
	AnimaStop();
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
#elif defined(CONFIG_PPG_SUPPORT)
	SetLeftKeyUpHandler(EnterHRScreen);
#elif defined(CONFIG_TEMP_SUPPORT)
	SetLeftKeyUpHandler(EnterTempScreen);
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
  #elif defined(CONFIG_PPG_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #elif defined(CONFIG_SYNC_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
  #else
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
  #endif
  
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterIdleScreen);
#endif
}
#endif/*CONFIG_STEP_SUPPORT*/
#endif/*CONFIG_IMU_SUPPORT*/

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
	AnimaStop();
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
#elif defined(CONFIG_FACTORY_TEST_SUPPORT)
	SetLeftKeyUpHandler(EnterSettings);
#else
  #if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
   #ifdef CONFIG_STEP_SUPPORT
	SetLeftKeyUpHandler(EnterStepsScreen);
   #elif defined(CONFIG_SLEEP_SUPPORT)
	SetLeftKeyUpHandler(EnterSleepScreen);
   #endif
  #elif defined(CONFIG_PPG_SUPPORT)
	SetLeftKeyUpHandler(EnterHRScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
	SetLeftKeyUpHandler(EnterTempScreen);
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
 #elif defined(CONFIG_FACTORY_TEST_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
 	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 #else
  #if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
   #ifdef CONFIG_STEP_SUPPORT
   	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterStepsScreen);
   #elif defined(CONFIG_SLEEP_SUPPORT)
   	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSleepScreen);
   #endif
  #elif defined(CONFIG_PPG_SUPPORT)
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterHRScreen);
  #elif defined(CONFIG_TEMP_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterTempScreen);
  #elif defined(CONFIG_SYNC_SUPPORT)
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSyncDataScreen);
  #else
  	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
  #endif
  
  	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterPoweroffScreen);
 #endif
#endif	
}

#ifdef CONFIG_ALARM_SUPPORT
void AlarmScreenProcess(void)
{
	uint16_t rect_x,rect_y,rect_w=180,rect_h=80;
	uint16_t x,y,w,h;
	uint8_t notify[20] = "Alarm Notify!";

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

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	SetLeftKeyUpHandler(AlarmRemindStop);
	SetRightKeyUpHandler(AlarmRemindStop);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, AlarmRemindStop);
	register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, AlarmRemindStop);
	register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, AlarmRemindStop);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, AlarmRemindStop);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, AlarmRemindStop);
#endif	
}

void FindDeviceScreenProcess(void)
{
	uint16_t rect_x,rect_y,rect_w=180,rect_h=80;
	uint16_t x,y,w,h;
	uint8_t notify[128] = "Find Device!";

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

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	SetLeftKeyUpHandler(FindDeviceStop);
	SetRightKeyUpHandler(FindDeviceStop);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, FindDeviceStop);
	register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, FindDeviceStop);
	register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, FindDeviceStop);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FindDeviceStop);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FindDeviceStop);
#endif	
}
#endif

#ifdef NB_SIGNAL_TEST
#ifdef CONFIG_WIFI_SUPPORT
void TestWifiUpdateInfor(void)
{
	uint8_t tmpbuf[512] = {0};
	
	LCD_Fill((LCD_WIDTH-180)/2, 50, 180, 180, BLACK);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(tmpbuf, wifi_test_info);
	LCD_ShowUniStringInRect((LCD_WIDTH-180)/2, 50, 180, 180, (uint16_t*)tmpbuf);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
	LCD_ShowStringInRect((LCD_WIDTH-180)/2, 50, 180, 180, wifi_test_info);
#endif
}

void TestWifiShowInfor(void)
{
	uint16_t x,y,w,h;
	uint8_t strbuf[512] = {0};
	
	LCD_Clear(BLACK);
	
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(strbuf, "WIFI TESTING");
	LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
	LCD_ShowUniString((LCD_WIDTH-w)/2, 20, (uint16_t*)strbuf);
	mmi_asc_to_ucs2(strbuf, "Wifi Starting...");
	LCD_ShowUniStringInRect((LCD_WIDTH-180)/2, 50, 180, 180, (uint16_t*)strbuf);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
	strcpy(strbuf, "WIFI TESTING");
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect((LCD_WIDTH-180)/2, 50, 180, 180, "Wifi Starting...");
#endif
}

void TestWifiScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_WIFI_TEST].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_WIFI_TEST].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_WIFI_TEST].status = SCREEN_STATUS_CREATED;

		TestWifiShowInfor();
		break;
		
	case SCREEN_ACTION_UPDATE:
		TestWifiUpdateInfor();
		break;
	}
	
	scr_msg[SCREEN_ID_WIFI_TEST].act = SCREEN_ACTION_NO;
}

void ExitWifiTestScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	
	if(wifi_is_working())
		MenuStopWifi();

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	EnterIdleScreen();
}

void EnterWifiTestScreen(void)
{
	if(screen_id == SCREEN_ID_WIFI_TEST)
		return;

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), K_NO_WAIT);
#ifdef CONFIG_PPG_SUPPORT
	PPGStopCheck();
#endif
	if(gps_is_working())
		MenuStopGPS();
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_WIFI_TEST;	
	scr_msg[SCREEN_ID_WIFI_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_WIFI_TEST].status = SCREEN_STATUS_CREATING;

	SetLeftKeyUpHandler(EnterSettings);
	SetRightKeyUpHandler(ExitWifiTestScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterGPSTestScreen);
#endif	
}
#endif

void TestGPSUpdateInfor(void)
{
	uint8_t tmpbuf[512] = {0};
	
	LCD_Fill((LCD_WIDTH-194)/2, 50, 194, 160, BLACK);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(tmpbuf, gps_test_info);
	LCD_ShowUniStringInRect((LCD_WIDTH-194)/2, 50, 194, 160, (uint16_t*)tmpbuf);
#else	
	LCD_SetFontSize(FONT_SIZE_16);
	LCD_ShowStringInRect((LCD_WIDTH-194)/2, 50, 194, 160, gps_test_info);
#endif
}

void TestGPSShowInfor(void)
{
	uint16_t x,y,w,h;
	uint8_t strbuf[512] = {0};
	
	LCD_Clear(BLACK);
	
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(strbuf, "GPS TESTING");
	LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
	LCD_ShowUniString((LCD_WIDTH-w)/2, 20, (uint16_t*)strbuf);
	mmi_asc_to_ucs2(strbuf, "GPS Starting...");
	LCD_ShowUniStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, (uint16_t*)strbuf);
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

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), K_NO_WAIT);
#ifdef CONFIG_PPG_SUPPORT
	PPGStopCheck();
#endif
#ifdef CONFIG_WIFI_SUPPORT
	if(wifi_is_working())
		MenuStopWifi();
#endif
	MenuStopNB();
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_GPS_TEST; 
	scr_msg[SCREEN_ID_GPS_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_GPS_TEST].status = SCREEN_STATUS_CREATING;

#ifdef CONFIG_WIFI_SUPPORT
	SetLeftKeyUpHandler(EnterWifiTestScreen);
#else
	SetLeftKeyUpHandler(EnterSettings);
#endif
	SetRightKeyUpHandler(ExitGPSTestScreen);

#ifdef CONFIG_TOUCH_SUPPORT
  #ifdef CONFIG_WIFI_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterWifiTestScreen);
  #else
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterSettings);
  #endif
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, EnterNBTestScreen);
#endif	
}

void TestNBUpdateINfor(void)
{
	uint8_t tmpbuf[512] = {0};

	if(screen_id == SCREEN_ID_NB_TEST)
	{
		LCD_Fill(30, 50, 190, 160, BLACK);
	#ifdef FONTMAKER_UNICODE_FONT
		mmi_asc_to_ucs2(tmpbuf, nb_test_info);
		LCD_ShowUniStringInRect(30, 50, 180, 160, (uint16_t*)tmpbuf);	
	#else
		LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);
	#endif
	}
}

void TestNBShowInfor(void)
{
	uint16_t x,y,w,h;
	uint8_t strbuf[512] = {0};
	
	LCD_Clear(BLACK);
#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
	mmi_asc_to_ucs2(strbuf, "NB-IoT TESTING");
	LCD_MeasureUniString((uint16_t*)strbuf, &w, &h);
	LCD_ShowUniString((LCD_WIDTH-w)/2, 20, (uint16_t*)strbuf);
	mmi_asc_to_ucs2(strbuf, nb_test_info);
	LCD_ShowUniStringInRect(30, 50, 180, 160, (uint16_t*)strbuf);	
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
	k_timer_start(&mainmenu_timer, K_SECONDS(5), K_NO_WAIT);
	
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
#endif

#ifdef CONFIG_QRCODE_SUPPORT
void DeviceShowQRStatus(void)
{
	uint8_t buf[512] = {0};
	
	LCD_Clear(BLACK);

	//IMEI
	strcpy(buf, "IMEI:");
	strcat(buf, g_imei);
	strcat(buf, ",");
	//IMSI
	strcat(buf, "IMSI:");
	strcat(buf, g_imsi);
	strcat(buf, ",");
	//ICCID
	strcat(buf, "ICCID:");
	strcat(buf, g_iccid);
	strcat(buf, ",");
	//nrf9160 FW ver
	strcat(buf, "MCU:");
	strcat(buf, g_fw_version);
	strcat(buf, ",");
#ifdef CONFIG_WIFI_SUPPORT	
	//WIFI FW ver
	strcat(buf, "WIFI:");
	strcat(buf, g_wifi_ver);
	strcat(buf, ",");
	//WIFI Mac
	strcat(buf, "WIFI MAC:");
	strcat(buf, g_wifi_mac_addr);
	strcat(buf, ",");
#else
	//WIFI FW ver
	strcat(buf, "WIFI:");
	strcat(buf, "NO");
	strcat(buf, ",");
	//WIFI Mac
	strcat(buf, "WIFI MAC:");
	strcat(buf, "NO");
	strcat(buf, ",");
#endif
	//BLE FW ver
	strcat(buf, "BLE:");
	strcat(buf, &g_nrf52810_ver[15]);
	strcat(buf, ",");
	//BLE Mac
	strcat(buf, "BLE MAC:");
	strcat(buf, g_ble_mac_addr);
	strcat(buf, ",");
#ifdef CONFIG_PPG_SUPPORT	
	//PPG ver
	strcat(buf, "PPG:");
	strcat(buf, g_ppg_ver);
	strcat(buf, ",");
#else
	//PPG ver
	strcat(buf, "PPG:");
	strcat(buf, "NO");
	strcat(buf, ",");
#endif
	//Modem ver
	strcat(buf, "MODEM:");
	strcat(buf, &g_modem[12]);

	show_QR_code(strlen(buf), buf);
}

void DeviceScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_DEVICE_INFOR].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_DEVICE_INFOR].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_DEVICE_INFOR].status = SCREEN_STATUS_CREATED;

		DeviceShowQRStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		DeviceShowQRStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_DEVICE_INFOR].act = SCREEN_ACTION_NO;
}

void ExitDeviceScreen(void)
{
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	EnterIdleScreen();
}

void EnterDeviceScreen(void)
{
	if(screen_id == SCREEN_ID_DEVICE_INFOR)
		return;

	k_timer_stop(&mainmenu_timer);

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStop();
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

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_DEVICE_INFOR;	
	scr_msg[SCREEN_ID_DEVICE_INFOR].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_DEVICE_INFOR].status = SCREEN_STATUS_CREATING;

	SetLeftKeyUpHandler(ExitDeviceScreen);
	SetRightKeyUpHandler(ExitDeviceScreen);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitDeviceScreen);
	register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitDeviceScreen);
	register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitDeviceScreen);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitDeviceScreen);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, ExitDeviceScreen);
#endif	
}
#endif

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
		AnimaStop();
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
	uint32_t img_anima[4] = {IMG_SOS_ANI_1_ADDR,IMG_SOS_ANI_2_ADDR,IMG_SOS_ANI_3_ADDR,IMG_SOS_ANI_4_ADDR};
	
	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(SOS_ICON_X, SOS_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#endif

	SOSChangrStatus();
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
	AnimaStop();
#endif
#ifdef NB_SIGNAL_TEST
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

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_SOS;	
	scr_msg[SCREEN_ID_SOS].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_SOS].status = SCREEN_STATUS_CREATING;

	ClearAllKeyHandler();
#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif
	
}

#if 0
void WristShowStatus(void)
{
	uint16_t x,y;
	uint32_t img_addr;
	uint8_t *img;

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

	#ifdef FW_FOR_CN	
	case LANGUAGE_CHN:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_WRIST_CN_ADDR;
	#else
		img = IMG_WRIST_CN;
	#endif
		x = WRIST_CN_TEXT_X;
		y = WRIST_CN_TEXT_Y;	
		break;
	#endif
	}

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(FALL_ICON_X, FALL_ICON_Y, IMG_WRIST_ICON_ADDR);
	LCD_ShowImg_From_Flash(x, y, img_addr);
#else
	LCD_ShowImg(FALL_ICON_X, FALL_ICON_Y, IMG_WRIST_ICON);
	LCD_ShowImg(x, y, img);
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

	k_timer_start(&notify_timer, K_SECONDS(NOTIFY_TIMER_INTERVAL), K_NO_WAIT);
}
#endif

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
	#ifdef CONFIG_ALARM_SUPPORT	
		case SCREEN_ID_ALARM:
			AlarmScreenProcess();
			break;
		case SCREEN_ID_FIND_DEVICE:
			FindDeviceScreenProcess();
			break;
	#endif		
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
	  #ifdef CONFIG_STEP_SUPPORT
		case SCREEN_ID_STEPS:
			StepsScreenProcess();
			break;
	  #endif
	  #ifdef CONFIG_SLEEP_SUPPORT
		case SCREEN_ID_SLEEP:
			SleepScreenProcess();
			break;
	  #endif
	  #ifdef CONFIG_FALL_DETECT_SUPPORT
		case SCREEN_ID_FALL:
			FallScreenProcess();
			break;
	  #endif
	#endif
	#if 0
		case SCREEN_ID_WRIST:
			WristScreenProcess();
			break;
	#endif
		case SCREEN_ID_SETTINGS:
			SettingsScreenProcess();
			break;
	#ifdef NB_SIGNAL_TEST
	  #ifdef CONFIG_WIFI_SUPPORT
		case SCREEN_ID_WIFI_TEST:
			TestWifiScreenProcess();
			break;
	  #endif
		case SCREEN_ID_GPS_TEST:
			TestGPSScreenProcess();
			break;
		case SCREEN_ID_NB_TEST:
			TestNBScreenProcess();
			break;
	#endif
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
	#ifdef CONFIG_QRCODE_SUPPORT
		case SCREEN_ID_DEVICE_INFOR:
			DeviceScreenProcess();
			break;
	#endif
		}
	}
}
