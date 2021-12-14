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
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif
#include "logger.h"

static u8_t scr_index=0;

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

static char *logo_img[] = 
{
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR,
	IMG_PEPPA_240X240_ADDR
};

void ShowBootUpLogo(void)
{
	u8_t i,count=0;
	u16_t x,y,w,h;

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(PWRON_LOGO_X, PWRON_LOGO_Y, logo_img, ARRAY_SIZE(logo_img), 200, false, EnterIdleScreen);
#else
  #ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(PWRON_LOGO_X, PWRON_LOGO_Y, IMG_PEPPA_240X240_ADDR);
  #else
	LCD_ShowImg(PWRON_LOGO_X, PWRON_LOGO_Y, IMG_PEPPA_240X240_ADDR);
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
		MenuStartPPG();
	#endif
	}
	else if(screen_id == SCREEN_ID_GPS_TEST)
	{
		MenuStartGPS();
	}
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

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);
#elif defined(FONT_24)
	LCD_SetFontSize(FONT_SIZE_24);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif

#ifdef FONTMAKER_UNICODE_FONT
	GetSystemDateStrings(str_date);
	LCD_MeasureUniString(str_date,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_DATE_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);	
	LCD_ShowUniString(x,y,str_date);
	
#else

	GetSystemDateStrings(str_date);
	LCD_MeasureString(str_date,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_DATE_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);	
	LCD_ShowString(x,y,str_date);
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
	u16_t x,y,w,h,offset;
	u8_t str_time[20] = {0};
	u8_t str_ampm[5] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;
	
#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);
	offset = 16;
#elif defined(FONT_24)
	LCD_SetFontSize(FONT_SIZE_24);
	offset = 8;
#else
	LCD_SetFontSize(FONT_SIZE_16);
	offset = 0;
#endif

#ifdef FONTMAKER_UNICODE_FONT
	GetSystemTimeStrings(str_time);
	LCD_MeasureUniString(str_time,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_TIME_SHOW_Y;
	LCD_ShowUniString(x,y,str_time);

	LCD_SetFontSize(FONT_SIZE_16);
	GetSysteAmPmStrings(str_ampm);
	x = x+w+5;
	y = IDLE_TIME_SHOW_Y+offset;
	LCD_ShowUniString(x,y,str_ampm);

#else

	GetSystemTimeStrings(str_time);
	LCD_MeasureString(str_time,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_TIME_SHOW_Y;
	LCD_ShowString(x,y,str_time);

	LCD_SetFontSize(FONT_SIZE_16);
	GetSysteAmPmStrings(str_ampm);
	x = x+w+5;
	y = IDLE_TIME_SHOW_Y+offset;
	LCD_ShowString(x,y,str_ampm);
#endif

	//xb test 2021-07-14 增加一个脱腕状态显示
#if 1	
	if(is_wearing())
	{
		LCD_MeasureString("wear on ",&w,&h);
		x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
		y = 210;
		LCD_ShowString(x,y,"wear on ");
	}
	else
	{
		LCD_MeasureString("wear off",&w,&h);
		x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
		y = 210;
		LCD_ShowString(x,y,"wear off");
	}
#else
	sprintf(str_time, "TG steps:%05d", global_settings.target_steps);
	LCD_MeasureString(str_time, &w, &h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = 210;
	LCD_ShowString(x,y,str_time);
#endif	
}

void IdleShowSystemWeek(void)
{
	u16_t x,y,w,h;
	u8_t str_week[128] = {0};

	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;

#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);
#elif defined(FONT_24)
	LCD_SetFontSize(FONT_SIZE_24);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif

	GetSystemWeekStrings(str_week);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_MeasureUniString(str_week,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_WEEK_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);
	LCD_ShowUniString(x,y,str_week);

#else
	//xb add 2020-11-06
	if(global_settings.language == LANGUAGE_CHN)
		strcpy(str_week,"It has no chinese font!");
	else if(global_settings.language == LANGUAGE_JPN)
		strcpy(str_week,"It has no japanese font!");
	//xb end

	LCD_MeasureString(str_week,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_WEEK_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);
	LCD_ShowString(x,y,str_week);
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
	u16_t x,y,w,h;
	u8_t strbuf[10] = {0};
	u8_t tmpbuf[128] = {0};
	
	LCD_Fill(BAT_SUBJECT_X+1,BAT_SUBJECT_Y+1,BAT_SUBJECT_W-2,BAT_SUBJECT_H-2,BLACK);
	
	switch(g_chg_status)
	{
	case BAT_CHARGING_NO:
		sprintf(strbuf, "%02d", g_bat_soc);
		break;
	case BAT_CHARGING_PROGRESS:
		strcpy(strbuf, "CHG");
		break;
	case BAT_CHARGING_FINISHED:
		strcpy(strbuf, "OK");
		break;
	}

	LCD_SetFontSize(FONT_SIZE_16);
	
#ifdef FONTMAKER_UNICODE_FONT
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_MeasureUniString(tmpbuf, &w, &h);
	x = (w > BAT_SUBJECT_W ? BAT_SUBJECT_X : (BAT_SUBJECT_W-w)/2);
	y = (h > BAT_SUBJECT_H ? BAT_SUBJECT_Y : (BAT_SUBJECT_H-h)/2);
	LCD_ShowUniString(BAT_SUBJECT_X+x, BAT_SUBJECT_Y+y, tmpbuf);
	
#else

	LCD_MeasureString(strbuf, &w, &h);
	x = (w > BAT_SUBJECT_W ? BAT_SUBJECT_X : (BAT_SUBJECT_W-w)/2);
	y = (h > BAT_SUBJECT_H ? BAT_SUBJECT_Y : (BAT_SUBJECT_H-h)/2);
	LCD_ShowString(BAT_SUBJECT_X+x, BAT_SUBJECT_Y+y, strbuf);
#endif
}

void IdleShowSignal(void)
{
	u8_t i;
	
	LCD_Fill(NB_SIGNAL_FILL_RECT_X-1,NB_SIGNAL_FILL_RECT_Y-1,(4*NB_SIGNAL_FILL_RECT_W+3*NB_SIGNAL_FILL_RECT_OFFSET_X)+2,NB_SIGNAL_FILL_RECT_H+2,BLACK);

	for(i=0;i<4;i++)
		LCD_DrawRectangle(NB_SIGNAL_FILL_RECT_X+i*(NB_SIGNAL_FILL_RECT_W+NB_SIGNAL_FILL_RECT_OFFSET_X),NB_SIGNAL_FILL_RECT_Y,NB_SIGNAL_FILL_RECT_W,NB_SIGNAL_FILL_RECT_H);
	
	for(i=0;i<g_nb_sig;i++)
		LCD_Fill(NB_SIGNAL_FILL_RECT_X+i*(NB_SIGNAL_FILL_RECT_W+NB_SIGNAL_FILL_RECT_OFFSET_X),NB_SIGNAL_FILL_RECT_Y,NB_SIGNAL_FILL_RECT_W,NB_SIGNAL_FILL_RECT_H,WHITE);
}

void IdleShowBatSoc(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[10] = {0};
	u8_t tmpbuf[128] = {0};
	
	LCD_DrawRectangle(BAT_POSITIVE_X,BAT_POSITIVE_Y,BAT_POSITIVE_W,BAT_POSITIVE_H);
	LCD_DrawRectangle(BAT_SUBJECT_X,BAT_SUBJECT_Y,BAT_SUBJECT_W,BAT_SUBJECT_H);
	LCD_Fill(BAT_SUBJECT_X+1,BAT_SUBJECT_Y+1,BAT_SUBJECT_W-2,BAT_SUBJECT_H-2,BLACK);
	
	switch(g_chg_status)
	{
	case BAT_CHARGING_NO:
		sprintf(strbuf, "%02d", g_bat_soc);
		break;
	case BAT_CHARGING_PROGRESS:
		strcpy(strbuf, "CHG");
		break;
	case BAT_CHARGING_FINISHED:
		strcpy(strbuf, "OK");
		break;
	}

	LCD_SetFontSize(FONT_SIZE_16);
	
#ifdef FONTMAKER_UNICODE_FONT
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_MeasureUniString(tmpbuf, &w, &h);
	x = (w > BAT_SUBJECT_W ? BAT_SUBJECT_X : (BAT_SUBJECT_W-w)/2);
	y = (h > BAT_SUBJECT_H ? BAT_SUBJECT_Y : (BAT_SUBJECT_H-h)/2);
	LCD_ShowUniString(BAT_SUBJECT_X+x, BAT_SUBJECT_Y+y, tmpbuf);
	
#else
	
	LCD_MeasureString(strbuf, &w, &h);
	x = (w > BAT_SUBJECT_W ? BAT_SUBJECT_X : (BAT_SUBJECT_W-w)/2);
	y = (h > BAT_SUBJECT_H ? BAT_SUBJECT_Y : (BAT_SUBJECT_H-h)/2);
	LCD_ShowString(BAT_SUBJECT_X+x, BAT_SUBJECT_Y+y, strbuf);
#endif
}

#ifdef CONFIG_IMU_SUPPORT
void IdleUpdateSportData(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LCD_SetFontSize(FONT_SIZE_16);

#ifdef FONTMAKER_UNICODE_FONT
	mmi_asc_to_ucs2(tmpbuf,"S:");
	LCD_MeasureUniString(tmpbuf, &w, &h);		
	LCD_Fill(IMU_STEPS_SHOW_X+w, IMU_STEPS_SHOW_Y, 50, IMU_STEPS_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_steps);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(IMU_STEPS_SHOW_X+w, IMU_STEPS_SHOW_Y, tmpbuf);

	mmi_asc_to_ucs2(tmpbuf,"D:");
	LCD_MeasureUniString(tmpbuf, &w, &h);
	LCD_Fill(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, 50, IMU_STEPS_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_distance);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, tmpbuf);

	mmi_asc_to_ucs2(tmpbuf,"C:");
	LCD_MeasureUniString(tmpbuf, &w, &h);
	LCD_Fill(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, 50, IMU_STEPS_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_calorie);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, tmpbuf);

#else

	LCD_MeasureString("S:", &w, &h);		
	LCD_Fill(IMU_STEPS_SHOW_X+w, IMU_STEPS_SHOW_Y, 50, IMU_STEPS_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_steps);
	LCD_ShowString(IMU_STEPS_SHOW_X+w, IMU_STEPS_SHOW_Y, strbuf);

	LCD_MeasureString("D:", &w, &h);
	LCD_Fill(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, 50, IMU_STEPS_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_distance);
	LCD_ShowString(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, strbuf);

	LCD_MeasureString("C:", &w, &h);
	LCD_Fill(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, 50, IMU_STEPS_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_calorie);
	LCD_ShowString(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3+w, IMU_STEPS_SHOW_Y, strbuf);
#endif
}

void IdleShowSportData(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LCD_SetFontSize(FONT_SIZE_16);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_Fill(IMU_STEPS_SHOW_X,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "S:%d", g_steps);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(IMU_STEPS_SHOW_X, IMU_STEPS_SHOW_Y, tmpbuf);

	LCD_Fill(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "D:%d", g_distance);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3, IMU_STEPS_SHOW_Y, tmpbuf);

	LCD_Fill(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "C:%d", g_calorie);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3, IMU_STEPS_SHOW_Y, tmpbuf);

#else
	LCD_Fill(IMU_STEPS_SHOW_X,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "S:%d", g_steps);
	LCD_ShowString(IMU_STEPS_SHOW_X, IMU_STEPS_SHOW_Y, strbuf);

	LCD_Fill(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "D:%d", g_distance);
	LCD_ShowString(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3, IMU_STEPS_SHOW_Y, strbuf);

	LCD_Fill(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "C:%d", g_calorie);
	LCD_ShowString(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3, IMU_STEPS_SHOW_Y, strbuf);
#endif
}
#endif

void IdleUpdateHealthData(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LCD_SetFontSize(FONT_SIZE_16);

#ifdef FONTMAKER_UNICODE_FONT
	mmi_asc_to_ucs2(tmpbuf,"HR:");
	LCD_MeasureUniString(tmpbuf, &w, &h);		
	LCD_Fill(PPG_DATA_SHOW_X+w, PPG_DATA_SHOW_Y, 50, PPG_DATA_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_hr);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(PPG_DATA_SHOW_X+w, PPG_DATA_SHOW_Y, tmpbuf);

	mmi_asc_to_ucs2(tmpbuf,"SPO2:");
	LCD_MeasureUniString(tmpbuf, &w, &h);
	LCD_Fill(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2+w, PPG_DATA_SHOW_Y, 50, PPG_DATA_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_spo2);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2+w, PPG_DATA_SHOW_Y, tmpbuf);

#else
#ifdef CONFIG_PPG_SUPPORT
	LCD_MeasureString("HR:", &w, &h);		
	LCD_Fill(PPG_DATA_SHOW_X+w, PPG_DATA_SHOW_Y, 50, PPG_DATA_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_hr);
	LCD_ShowString(PPG_DATA_SHOW_X+w, PPG_DATA_SHOW_Y, strbuf);

	LCD_MeasureString("SPO2:", &w, &h);
	LCD_Fill(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2+w, PPG_DATA_SHOW_Y, 50, PPG_DATA_SHOW_H, BLACK);
	sprintf(strbuf, "%d", g_spo2);
	LCD_ShowString(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2+w, PPG_DATA_SHOW_Y, strbuf);
#endif	
#endif
}

void IdleShowHealthData(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	u8_t tmpbuf[128] = {0};
	
	LCD_SetFontSize(FONT_SIZE_16);

#ifdef FONTMAKER_UNICODE_FONT
	LCD_Fill(PPG_DATA_SHOW_X,PPG_DATA_SHOW_Y,PPG_DATA_SHOW_W/3,PPG_DATA_SHOW_H,BLACK);
	sprintf(strbuf, "HR:%d", g_hr);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(PPG_DATA_SHOW_X, PPG_DATA_SHOW_Y, tmpbuf);

	LCD_Fill(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2,PPG_DATA_SHOW_Y,PPG_DATA_SHOW_W/2,PPG_DATA_SHOW_H,BLACK);
	sprintf(strbuf, "SPO2:%d", g_spo2);
	mmi_asc_to_ucs2(tmpbuf,strbuf);
	LCD_ShowUniString(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2, PPG_DATA_SHOW_Y, tmpbuf);

#else
#ifdef CONFIG_PPG_SUPPORT
	LCD_Fill(PPG_DATA_SHOW_X,PPG_DATA_SHOW_Y,PPG_DATA_SHOW_W/2,PPG_DATA_SHOW_H,BLACK);
	sprintf(strbuf, "HR:%d", g_hr);
	LCD_ShowString(PPG_DATA_SHOW_X, PPG_DATA_SHOW_Y, strbuf);

	LCD_Fill(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2,PPG_DATA_SHOW_Y,PPG_DATA_SHOW_W/2,PPG_DATA_SHOW_H,BLACK);
	sprintf(strbuf, "SPO2:%d", g_spo2);
	LCD_ShowString(PPG_DATA_SHOW_X+PPG_DATA_SHOW_W/2, PPG_DATA_SHOW_Y, strbuf);
#endif
#endif
}

void IdleScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_IDLE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATED;
		
		LCD_Clear(BLACK);
		IdleShowSignal();
		IdleShowBatSoc();
		IdleShowDateTime();
	#ifdef CONFIG_IMU_SUPPORT	
		IdleShowSportData();
	#endif
		IdleShowHealthData();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SIG)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SIG);
			IdleShowSignal();
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
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_HEALTH)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_HEALTH);
			IdleUpdateHealthData();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SLEEP)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SLEEP);
		}

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

	#ifdef FONT_24
		LCD_SetFontSize(FONT_SIZE_24);
	#else
		LCD_SetFontSize(FONT_SIZE_16);
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
		
	#ifdef FONT_24
		LCD_SetFontSize(FONT_SIZE_24);
	#else
		LCD_SetFontSize(FONT_SIZE_16);
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

#ifdef CONFIG_PPG_SUPPORT
void HeartRateScreenProcess(void)
{
	u16_t x,y,w,h;
	u8_t notify[64] = "Heart Rate";
	u8_t tmpbuf[64] = {0};
	
	switch(scr_msg[SCREEN_ID_HR].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_HR].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_HR].status = SCREEN_STATUS_CREATED;
				
		LCD_Clear(BLACK);
		LCD_SetFontSize(FONT_SIZE_24);
		LCD_MeasureString(notify,&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 50;
		LCD_ShowString(x,y,notify);
		
		LCD_SetFontSize(FONT_SIZE_32);
		strcpy(tmpbuf, "0");
		LCD_MeasureString(tmpbuf,&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 120;
		LCD_ShowString(x,y,tmpbuf);
		break;
		
	case SCREEN_ACTION_UPDATE:
		LCD_SetFontSize(FONT_SIZE_32);
		LCD_MeasureString("0000",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 120-2;
		LCD_Fill(x, y, w, h+4, BLACK);
		
		sprintf(tmpbuf, "%d", g_hr);
		LCD_MeasureString(tmpbuf,&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 120;
		LCD_ShowString(x,y,tmpbuf);
		break;
	}
	
	scr_msg[SCREEN_ID_HR].act = SCREEN_ACTION_NO;
}
#endif

void ShowStringsInRect(u16_t rect_x, u16_t rect_y, u16_t rect_w, u16_t rect_h, SYSTEM_FONT_SIZE font_size, u8_t *strbuf)
{
	u16_t x,y,w,h;
	u16_t offset_w=4,offset_h=4;

	LCD_SetFontSize(font_size);
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

void NotifyShowStrings(u16_t rect_x, u16_t rect_y, u16_t rect_w, u16_t rect_h, SYSTEM_FONT_SIZE font_size, u8_t *strbuf)
{
	LCD_DrawRectangle(rect_x, rect_y, rect_w, rect_h);
	LCD_Fill(rect_x+1, rect_y+1, rect_w-2, rect_h-2, BLACK);
	
	ShowStringsInRect(rect_x, rect_y, rect_w, rect_h, font_size, strbuf);	
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
	
	LCD_SetFontSize(FONT_SIZE_16);
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

void SOSShowStatus(void)
{
	u32_t img_addr;
	u8_t *img;
	
	LCD_Clear(BLACK);

	switch(sos_state)
	{
	case SOS_STATUS_IDLE:
		break;
		
	case SOS_STATUS_SENDING:
	#if 0
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_ADDR;
	#else
		img = IMG_SOS;
	#endif
	#endif
		NotifyShowStrings(SOS_NOTIFY_RECT_X, SOS_NOTIFY_RECT_Y, SOS_NOTIFY_RECT_W, SOS_NOTIFY_RECT_H, FONT_SIZE_16, "SOS is sending, please wait a few seconds!");
		break;
	
	case SOS_STATUS_SENT:
	#if 0
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_SEND_ADDR;
	#else
		img = IMG_SOS_SEND;
	#endif
	#endif
		NotifyShowStrings(SOS_NOTIFY_RECT_X, SOS_NOTIFY_RECT_Y, SOS_NOTIFY_RECT_W, SOS_NOTIFY_RECT_H, FONT_SIZE_16, "SOS has been sent!");
		break;
	
	case SOS_STATUS_RECEIVED:
	#if 0
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_RECE_ADDR;
	#else
		img = IMG_SOS_RECE;
	#endif
	#endif
		NotifyShowStrings(SOS_NOTIFY_RECT_X, SOS_NOTIFY_RECT_Y, SOS_NOTIFY_RECT_W, SOS_NOTIFY_RECT_H, FONT_SIZE_16, "Server has received the SOS alert!");
		break;
	
	case SOS_STATUS_CANCEL:
	#if 0	
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_ADDR;
	#else
		img = IMG_SOS;
	#endif
	#endif
		NotifyShowStrings(SOS_NOTIFY_RECT_X, SOS_NOTIFY_RECT_Y, SOS_NOTIFY_RECT_W, SOS_NOTIFY_RECT_H, FONT_SIZE_16, "SOS has been canceled!");
		break;
	}

#if 0
#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(SOS_X, SOS_Y, img_addr);
#else
	LCD_ShowImg(SOS_X, SOS_Y, img);
#endif
#endif
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
			SOSShowStatus();
		}

		if(scr_msg[SCREEN_ID_SOS].para == SCREEN_EVENT_UPDATE_NO)
			scr_msg[SCREEN_ID_SOS].act = SCREEN_ACTION_NO;
		break;
	}
}

#ifdef CONFIG_FOTA_DOWNLOAD
void FOTAShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t str_title[] = "FOTA RUNNING";

	LCD_Clear(BLACK);
	//LCD_DrawRectangle(FOTA_NOTIFY_RECT_X, FOTA_NOTIFY_RECT_Y, FOTA_NOTIFY_RECT_W, FOTA_NOTIFY_RECT_H);
	//LCD_Fill(FOTA_NOTIFY_RECT_X+1, FOTA_NOTIFY_RECT_Y+1, FOTA_NOTIFY_RECT_W-1, FOTA_NOTIFY_RECT_H-1, BLACK);
	
	LCD_SetFontSize(FONT_SIZE_16);
	LCD_MeasureString(str_title, &w, &h);
	x = (w > (FOTA_NOTIFY_RECT_W-2*FOTA_NOTIFY_OFFSET_W))? 0 : ((FOTA_NOTIFY_RECT_W-2*FOTA_NOTIFY_OFFSET_W)-w)/2;
	x += (FOTA_NOTIFY_RECT_X+FOTA_NOTIFY_OFFSET_W);
	y = 20;
	LCD_ShowString(x,y,str_title);

	ShowStringsInRect(FOTA_NOTIFY_STRING_X, 
					  FOTA_NOTIFY_STRING_Y, 
					  FOTA_NOTIFY_STRING_W, 
					  FOTA_NOTIFY_STRING_H, 
					  FONT_SIZE_16, 
					  "Make sure the battery is at least 20% full and don't do anything during the upgrade!");

	LCD_DrawRectangle(FOTA_NOTIFY_YES_X, FOTA_NOTIFY_YES_Y, FOTA_NOTIFY_YES_W, FOTA_NOTIFY_YES_H);
	LCD_MeasureString("SOS(Y)", &w, &h);
	x = FOTA_NOTIFY_YES_X+(FOTA_NOTIFY_YES_W-w)/2;
	y = FOTA_NOTIFY_YES_Y+(FOTA_NOTIFY_YES_H-h)/2;	
	LCD_ShowString(x,y,"SOS(Y)");

	LCD_DrawRectangle(FOTA_NOTIFY_NO_X, FOTA_NOTIFY_NO_Y, FOTA_NOTIFY_NO_W, FOTA_NOTIFY_NO_H);
	LCD_MeasureString("PWR(N)", &w, &h);
	x = FOTA_NOTIFY_NO_X+(FOTA_NOTIFY_NO_W-w)/2;
	y = FOTA_NOTIFY_NO_Y+(FOTA_NOTIFY_NO_H-h)/2;	
	LCD_ShowString(x,y,"PWR(N)");

	SetLeftKeyUpHandler(fota_start_confirm);
	SetRightKeyUpHandler(fota_exit);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FOTA_NOTIFY_YES_X, FOTA_NOTIFY_YES_X+FOTA_NOTIFY_YES_W, FOTA_NOTIFY_YES_Y, FOTA_NOTIFY_YES_Y+FOTA_NOTIFY_YES_H, fota_start_confirm);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FOTA_NOTIFY_NO_X, FOTA_NOTIFY_NO_X+FOTA_NOTIFY_NO_W, FOTA_NOTIFY_NO_Y, FOTA_NOTIFY_NO_Y+FOTA_NOTIFY_NO_H, fota_exit);
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
		LCD_Fill(FOTA_NOTIFY_RECT_X+1, FOTA_NOTIFY_STRING_Y, FOTA_NOTIFY_RECT_W-1, FOTA_NOTIFY_RECT_H-(FOTA_NOTIFY_STRING_Y-FOTA_NOTIFY_RECT_Y)-1, BLACK);
		ShowStringsInRect(FOTA_NOTIFY_STRING_X,
						  FOTA_NOTIFY_STRING_Y,
						  FOTA_NOTIFY_STRING_W,
						  FOTA_NOTIFY_STRING_H,
						  FONT_SIZE_16,
						  "Linking to server...");

		Key_Event_Unregister_Handler();
		break;
		
	case FOTA_STATUS_DOWNLOADING:
		if(!flag)
		{
			flag = true;
			
			LCD_Fill(FOTA_NOTIFY_STRING_X, FOTA_NOTIFY_STRING_Y, FOTA_NOTIFY_STRING_W, FOTA_NOTIFY_STRING_H, BLACK);
			ShowStringsInRect(FOTA_NOTIFY_STRING_X, 
							  FOTA_NOTIFY_STRING_Y,
							  FOTA_NOTIFY_STRING_W,
							  40,
							  FONT_SIZE_16,
							  "Downloading data...");
			
			LCD_DrawRectangle(FOTA_NOTIFY_PRO_X, FOTA_NOTIFY_PRO_Y, FOTA_NOTIFY_PRO_W, FOTA_NOTIFY_PRO_H);
			LCD_Fill(FOTA_NOTIFY_PRO_X+1, FOTA_NOTIFY_PRO_Y+1, FOTA_NOTIFY_PRO_W-1, FOTA_NOTIFY_PRO_H-1, BLACK);

			sprintf(pro_buf, "%3d%%", g_fota_progress);
			LCD_MeasureString(pro_buf, &w, &h);
			pro_str_x = ((FOTA_NOTIFY_RECT_W-2*FOTA_NOTIFY_OFFSET_W)-w)/2;
			pro_str_x += (FOTA_NOTIFY_RECT_X+FOTA_NOTIFY_OFFSET_W);
			pro_str_y = FOTA_NOTIFY_PRO_Y + FOTA_NOTIFY_PRO_H + 5;
			
			LCD_ShowString(pro_str_x,pro_str_y, pro_buf);
		}
		else
		{
			pro_len = (g_fota_progress*FOTA_NOTIFY_PRO_W)/100;
			LCD_Fill(FOTA_NOTIFY_PRO_X+1, FOTA_NOTIFY_PRO_Y+1, pro_len, FOTA_NOTIFY_PRO_H-1, WHITE);

			sprintf(pro_buf, "%3d%%", g_fota_progress);
			LCD_ShowString(pro_str_x, pro_str_y, pro_buf);
		}

		Key_Event_Unregister_Handler();
		break;
		
	case FOTA_STATUS_FINISHED:
		flag = false;
		
		LCD_Fill(FOTA_NOTIFY_RECT_X+1, FOTA_NOTIFY_STRING_Y, FOTA_NOTIFY_RECT_W-1, FOTA_NOTIFY_RECT_H-(FOTA_NOTIFY_STRING_Y-FOTA_NOTIFY_RECT_Y)-1, BLACK);
		ShowStringsInRect(FOTA_NOTIFY_STRING_X,
						  FOTA_NOTIFY_STRING_Y,
						  FOTA_NOTIFY_STRING_W,
						  FOTA_NOTIFY_STRING_H,
						  FONT_SIZE_16,
						  "It upgraded successfully! Do you want to reboot the device immediately?");

		LCD_DrawRectangle(FOTA_NOTIFY_YES_X, FOTA_NOTIFY_YES_Y, FOTA_NOTIFY_YES_W, FOTA_NOTIFY_YES_H);
		LCD_MeasureString("SOS(Y)", &w, &h);
		x = FOTA_NOTIFY_YES_X+(FOTA_NOTIFY_YES_W-w)/2;
		y = FOTA_NOTIFY_YES_Y+(FOTA_NOTIFY_YES_H-h)/2;	
		LCD_ShowString(x,y,"SOS(Y)");

		LCD_DrawRectangle(FOTA_NOTIFY_NO_X, FOTA_NOTIFY_NO_Y, FOTA_NOTIFY_NO_W, FOTA_NOTIFY_NO_H);
		LCD_MeasureString("PWR(N)", &w, &h);
		x = FOTA_NOTIFY_NO_X+(FOTA_NOTIFY_NO_W-w)/2;
		y = FOTA_NOTIFY_NO_Y+(FOTA_NOTIFY_NO_H-h)/2;	
		LCD_ShowString(x,y,"PWR(N)");

		SetLeftKeyUpHandler(fota_reboot_confirm);
		SetRightKeyUpHandler(fota_exit);
		break;
		
	case FOTA_STATUS_ERROR:
		flag = false;

		LCD_Fill(FOTA_NOTIFY_RECT_X+1, FOTA_NOTIFY_STRING_Y, FOTA_NOTIFY_RECT_W-1, FOTA_NOTIFY_RECT_H-(FOTA_NOTIFY_STRING_Y-FOTA_NOTIFY_RECT_Y)-1, BLACK);
		ShowStringsInRect(FOTA_NOTIFY_STRING_X,
						  FOTA_NOTIFY_STRING_Y,
						  FOTA_NOTIFY_STRING_W,
						  FOTA_NOTIFY_STRING_H,
						  FONT_SIZE_16,
						  "It failed to upgrade! Please check the network or server.");

		LCD_DrawRectangle((LCD_WIDTH-FOTA_NOTIFY_YES_W)/2, FOTA_NOTIFY_YES_Y, FOTA_NOTIFY_YES_W, FOTA_NOTIFY_YES_H);
		LCD_MeasureString("SOS(Y)", &w, &h);
		x = (LCD_WIDTH-FOTA_NOTIFY_YES_W)/2+(FOTA_NOTIFY_YES_W-w)/2;
		y = FOTA_NOTIFY_YES_Y+(FOTA_NOTIFY_YES_H-h)/2;	
		LCD_ShowString(x,y,"SOS(Y)");

		SetLeftKeyUpHandler(fota_exit);
		SetRightKeyUpHandler(fota_exit);
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
	EnterIdleScreen();
}

void EnterFOTAScreen(void)
{
	if(screen_id == SCREEN_ID_FOTA)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FOTA;	
	scr_msg[SCREEN_ID_FOTA].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FOTA].status = SCREEN_STATUS_CREATING;
}
#endif/*CONFIG_FOTA_DOWNLOAD*/

#ifdef CONFIG_IMU_SUPPORT
void SleepScreenProcess(void)
{
	u16_t x,y,w,h;
	u16_t deep_sleep,light_sleep;
	u8_t notify[64] = "Sleep";
	u8_t tmpbuf[64] = {0};
	
	switch(scr_msg[SCREEN_ID_SLEEP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_SLEEP].status = SCREEN_STATUS_CREATED;
				
		LCD_Clear(BLACK);
		LCD_SetFontSize(FONT_SIZE_24);
		LCD_MeasureString(notify,&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 50;
		LCD_ShowString(x,y,notify);

		GetSleepTimeData(&deep_sleep, &light_sleep);
		LCD_SetFontSize(FONT_SIZE_16);
		LCD_MeasureString("TOTAL_SLEEP:        ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 100;
		LCD_ShowString(x,y,"TOTAL_SLEEP:        ");
		sprintf(tmpbuf, "%d (min)", deep_sleep+light_sleep);
		LCD_ShowString(x+13*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("DEEP_SLEEP:        ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 120;
		LCD_ShowString(x,y,"DEEP_SLEEP:        ");
		sprintf(tmpbuf, "%d (min)", deep_sleep);
		LCD_ShowString(x+12*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("LIGHT_SLEEP:        ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 140;
		LCD_ShowString(x,y,"LIGHT_SLEEP:        ");
		sprintf(tmpbuf, "%d (min)", light_sleep);
		LCD_ShowString(x+13*FONT_SIZE_16/2, y, tmpbuf);
		break;
		
	case SCREEN_ACTION_UPDATE:
		GetSleepTimeData(&deep_sleep, &light_sleep);
		LCD_SetFontSize(FONT_SIZE_16);
		LCD_MeasureString("TOTAL_SLEEP:        ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 100;
		LCD_ShowString(x,y,"TOTAL_SLEEP:        ");
		sprintf(tmpbuf, "%d (min)", deep_sleep+light_sleep);
		LCD_ShowString(x+13*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("DEEP_SLEEP:        ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 120;
		LCD_ShowString(x,y,"DEEP_SLEEP:        ");
		sprintf(tmpbuf, "%d (min)", deep_sleep);
		LCD_ShowString(x+12*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("LIGHT_SLEEP:        ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 140;
		LCD_ShowString(x,y,"LIGHT_SLEEP:        ");
		sprintf(tmpbuf, "%d (min)", light_sleep);
		LCD_ShowString(x+13*FONT_SIZE_16/2, y, tmpbuf);
		break;
	}
	
	scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
}

void StepsScreenProcess(void)
{
	u16_t x,y,w,h;
	u16_t steps,calorie,distance;
	u8_t notify[64] = "Sport";
	u8_t tmpbuf[64] = {0};
	
	switch(scr_msg[SCREEN_ID_STEPS].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_STEPS].status = SCREEN_STATUS_CREATED;
				
		LCD_Clear(BLACK);
		LCD_SetFontSize(FONT_SIZE_24);
		LCD_MeasureString(notify,&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 50;
		LCD_ShowString(x,y,notify);

		GetSportData(&steps, &calorie, &distance);
		LCD_SetFontSize(FONT_SIZE_16);
		
		LCD_MeasureString("Steps:     ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 100;
		LCD_ShowString(x, y, "Steps:     ");
		sprintf(tmpbuf, "%d", steps);
		LCD_ShowString(x+7*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("Distance:     ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 120;
		LCD_ShowString(x,y,"Distance:     ");
		sprintf(tmpbuf, "%d", distance);
		LCD_ShowString(x+10*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("Calorie:     ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 140;
		LCD_ShowString(x,y,"Calorie:     ");
		sprintf(tmpbuf, "%d", calorie);
		LCD_ShowString(x+9*FONT_SIZE_16/2, y, tmpbuf);		
		break;
		
	case SCREEN_ACTION_UPDATE:
		GetSportData(&steps, &calorie, &distance);
		LCD_SetFontSize(FONT_SIZE_16);
		
		LCD_MeasureString("Steps:     ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 100;
		LCD_ShowString(x, y, "Steps:     ");
		sprintf(tmpbuf, "%d", steps);
		LCD_ShowString(x+7*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("Distance:     ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 120;
		LCD_ShowString(x,y,"Distance:     ");
		sprintf(tmpbuf, "%d", distance);
		LCD_ShowString(x+10*FONT_SIZE_16/2, y, tmpbuf);

		LCD_MeasureString("Calorie:     ",&w,&h);
		x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
		y = 140;
		LCD_ShowString(x,y,"Calorie:     ");
		sprintf(tmpbuf, "%d", calorie);
		LCD_ShowString(x+9*FONT_SIZE_16/2, y, tmpbuf);
		break;
	}
	
	scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_NO;
}

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
#endif

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

void TestGPSUpdateInfor(void)
{
	LCD_Fill((LCD_WIDTH-194)/2, 50, 194, 160, BLACK);
	LCD_SetFontSize(FONT_SIZE_16);
	LCD_ShowStringInRect((LCD_WIDTH-192)/2, 50, 192, 160, gps_test_info);
}

void TestGPSShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);
	strcpy(strbuf, "GPS TESTING");
	LCD_SetFontSize(FONT_SIZE_16);
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

		TestNBUpdateINfor();
		break;
		
	case SCREEN_ACTION_UPDATE:
		TestNBUpdateINfor();
		break;
	}
	
	scr_msg[SCREEN_ID_NB_TEST].act = SCREEN_ACTION_NO;
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

#if defined(CONFIG_PPG_SUPPORT)
	SetLeftKeyUpHandler(EnterHRScreen);
	SetRightKeyUpHandler(EnterIdleScreen);
#else
	SetLeftKeyUpHandler(EnterGPSTestScreen);
	SetRightKeyUpHandler(EnterIdleScreen);
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

#ifdef CONFIG_IMU_SUPPORT
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

#ifdef CONFIG_FOTA_DOWNLOAD
	SetLeftKeyUpHandler(fota_start);
	SetRightKeyUpHandler(fota_exit);
#else
	SetLeftKeyUpHandler(ExitStepsScreen);
	SetRightKeyUpHandler(ExitStepsScreen);
#endif
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

	SetLeftKeyUpHandler(EnterStepsScreen);
	SetRightKeyUpHandler(ExitSleepScreen);
}
#endif

void ExitGPSTestScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	
	if(gps_is_working())
		MenuStopGPS();
	
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

#ifdef CONFIG_IMU_SUPPORT
	SetLeftKeyUpHandler(EnterSleepScreen);
	SetRightKeyUpHandler(ExitGPSTestScreen);
#elif defined(CONFIG_FOTA_DOWNLOAD)
	SetLeftKeyUpHandler(fota_start);
	SetRightKeyUpHandler(fota_exit);
#else
	SetLeftKeyUpHandler(ExitGPSTestScreen);
	SetRightKeyUpHandler(ExitGPSTestScreen);
#endif
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
}

#ifdef CONFIG_PPG_SUPPORT
void ExitHRScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	
	if(PPGIsWorking())
		MenuStopPPG();
		
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

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

	SetLeftKeyUpHandler(EnterGPSTestScreen);
	SetRightKeyUpHandler(ExitHRScreen);
}
#endif/*CONFIG_PPG_SUPPORT*/

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

void poweroff_leftkeyfunc(void)
{
	key_pwroff_flag = true;
}

void poweroff_rightkeyfunc(void)
{
	EnterIdleScreen();
}

void EnterPoweroffScreen(void)
{
	if(screen_id == SCREEN_ID_POWEROFF)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_POWEROFF;	
	scr_msg[SCREEN_ID_POWEROFF].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_POWEROFF].status = SCREEN_STATUS_CREATING;		
}

void PowerOffShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t str_title[] = "POWER OFF";
	
	LCD_DrawRectangle(PWR_OFF_NOTIFY_RECT_X, PWR_OFF_NOTIFY_RECT_Y, PWR_OFF_NOTIFY_RECT_W, PWR_OFF_NOTIFY_RECT_H);
	LCD_Fill(PWR_OFF_NOTIFY_RECT_X+1, PWR_OFF_NOTIFY_RECT_Y+1, PWR_OFF_NOTIFY_RECT_W-1, PWR_OFF_NOTIFY_RECT_H-1, BLACK);
	
	LCD_SetFontSize(FONT_SIZE_16);
	LCD_MeasureString(str_title, &w, &h);
	x = (w > (PWR_OFF_NOTIFY_RECT_W-2*PWR_OFF_NOTIFY_OFFSET_W))? 0 : ((PWR_OFF_NOTIFY_RECT_W-2*PWR_OFF_NOTIFY_OFFSET_W)-w)/2;
	x += (PWR_OFF_NOTIFY_RECT_X+PWR_OFF_NOTIFY_OFFSET_W);
	y = PWR_OFF_NOTIFY_RECT_Y+2;
	LCD_ShowString(x,y,str_title);

	ShowStringsInRect(PWR_OFF_NOTIFY_STRING_X, 
					  PWR_OFF_NOTIFY_STRING_Y, 
					  PWR_OFF_NOTIFY_STRING_W, 
					  PWR_OFF_NOTIFY_STRING_H, 
					  FONT_SIZE_16, 
					  "Are you sure you want to turn it off?");

	LCD_DrawRectangle(PWR_OFF_NOTIFY_YES_X, PWR_OFF_NOTIFY_YES_Y, PWR_OFF_NOTIFY_YES_W, PWR_OFF_NOTIFY_YES_H);
	LCD_MeasureString("SOS(Y)", &w, &h);
	x = PWR_OFF_NOTIFY_YES_X+(PWR_OFF_NOTIFY_YES_W-w)/2;
	y = PWR_OFF_NOTIFY_YES_Y+(PWR_OFF_NOTIFY_YES_H-h)/2;	
	LCD_ShowString(x,y,"SOS(Y)");

	LCD_DrawRectangle(PWR_OFF_NOTIFY_NO_X, PWR_OFF_NOTIFY_NO_Y, PWR_OFF_NOTIFY_NO_W, PWR_OFF_NOTIFY_NO_H);
	LCD_MeasureString("PWR(N)", &w, &h);
	x = PWR_OFF_NOTIFY_NO_X+(PWR_OFF_NOTIFY_NO_W-w)/2;
	y = PWR_OFF_NOTIFY_NO_Y+(PWR_OFF_NOTIFY_NO_H-h)/2;	
	LCD_ShowString(x,y,"PWR(N)");

	SetLeftKeyUpHandler(poweroff_leftkeyfunc);
	SetRightKeyUpHandler(poweroff_rightkeyfunc);
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, PWR_OFF_NOTIFY_YES_X, PWR_OFF_NOTIFY_YES_X+PWR_OFF_NOTIFY_YES_W, PWR_OFF_NOTIFY_YES_Y, PWR_OFF_NOTIFY_YES_Y+PWR_OFF_NOTIFY_YES_H, poweroff_leftkeyfunc);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, PWR_OFF_NOTIFY_NO_X, PWR_OFF_NOTIFY_NO_X+PWR_OFF_NOTIFY_NO_W, PWR_OFF_NOTIFY_NO_Y, PWR_OFF_NOTIFY_NO_Y+PWR_OFF_NOTIFY_NO_H, poweroff_rightkeyfunc);
#endif
}

void PowerOffScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_POWEROFF].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_POWEROFF].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_POWEROFF].status = SCREEN_STATUS_CREATED;

		PowerOffShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		break;
	}
	
	scr_msg[SCREEN_ID_POWEROFF].act = SCREEN_ACTION_NO;
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
			HeartRateScreenProcess();
			break;
	#endif
		case SCREEN_ID_ECG:
			break;
		case SCREEN_ID_BP:
			break;
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
	#ifdef CONFIG_FOTA_DOWNLOAD
		case SCREEN_ID_FOTA:
			FOTAScreenProcess();
			break;
	#endif
		}
	}
}

