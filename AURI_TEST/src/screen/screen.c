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
extern bool wifi_update_flag;

static void EnterHRScreen(void);

static char *logo_img[] = 
{
	logo_1,
	logo_2,
	logo_3,
	logo_4,
	logo_5
};

void ShowBootUpLogo(void)
{
	u8_t i,count=0;
	u16_t x,y,w,h;

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(PWRON_LOGO_X, PWRON_LOGO_Y, logo_img, ARRAY_SIZE(logo_img), 200, false, EnterIdleScreen);
#else
	LCD_ShowImg(PWRON_LOGO_X, PWRON_LOGO_Y, logo_5);
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
	unsigned char *img_num[10] = {IMG_DATE_NUM_0,IMG_DATE_NUM_1,IMG_DATE_NUM_2,IMG_DATE_NUM_3,IMG_DATE_NUM_4,
							  		IMG_DATE_NUM_5,IMG_DATE_NUM_6,IMG_DATE_NUM_7,IMG_DATE_NUM_8,IMG_DATE_NUM_9};
	unsigned char *img_month[12] = {IMG_MON_JAN,IMG_MON_FEB,IMG_MON_MAR,IMG_MON_APR,IMG_MON_MAY,IMG_MON_JUN,
							  		IMG_MON_JUL,IMG_MON_AUG,IMG_MON_SEP,IMG_MON_OCT,IMG_MON_NOV,IMG_MON_DEC};

	switch(global_settings.language)
	{
	case LANGUAGE_EN:
	case LANGUAGE_JPN:
		LCD_ShowImg(IDLE_DATA_NUM_1_X, IDLE_DATA_NUM_1_Y, img_num[date_time.day/10]);
		LCD_ShowImg(IDLE_DATA_NUM_2_X, IDLE_DATA_NUM_2_Y, img_num[date_time.day%10]);
		LCD_ShowImg(IDLE_MONTH_X, IDLE_MONTH_Y, img_month[date_time.month-1]);
		break;

	case LANGUAGE_CHN:
		LCD_ShowImg(IDLE_DATA_NUM_1_X, IDLE_DATA_NUM_1_Y, img_num[date_time.month/10]);
		LCD_ShowImg(IDLE_DATA_NUM_2_X, IDLE_DATA_NUM_2_Y, img_num[date_time.month%10]);
		LCD_ShowImg(IDLE_DATE_LINK_X, IDLE_DATE_LINK_Y, IMG_DATE_LINK);
		LCD_ShowImg(IDLE_DATA_NUM_3_X, IDLE_DATA_NUM_3_Y, img_num[date_time.day/10]);
		LCD_ShowImg(IDLE_DATA_NUM_4_X, IDLE_DATA_NUM_4_Y, img_num[date_time.day%10]);
		break;
	}
}

void IdleShowBleStatus(bool flag)
{
	if(flag)
	{
		LCD_ShowImg(IDLE_BLE_X, IDLE_BLE_Y, IMG_BLE_LINK);
	}
	else
	{
		LCD_ShowImg(IDLE_BLE_X, IDLE_BLE_Y, IMG_BLE_UNLINK);
	}
}

void IdleShowTime12Format(bool flag)
{
	unsigned char *img_cn[2] = {IMG_AM_CN, IMG_PM_CN};
	unsigned char *img_en[2] = {IMG_AM_EN, IMG_PM_EN};

	if(flag)
	{
		switch(global_settings.language)
		{
		case LANGUAGE_EN:
		case LANGUAGE_JPN:
			LCD_ShowImg(IDLE_AM_PM_X, IDLE_AM_PM_Y, img_en[date_time.hour/12]);
			break;

		case LANGUAGE_CHN:
			LCD_ShowImg(IDLE_AM_PM_X, IDLE_AM_PM_Y, img_cn[date_time.hour/12]);
			break;
		}
	}
	else
	{
		LCD_Fill(IDLE_AM_PM_X, IDLE_AM_PM_Y, IDLE_AM_PM_W, IDLE_AM_PM_H, BLACK);
	}
}

void IdleShowSystemTime(void)
{
	static bool colon_flag = true;
	u8_t hour;
	unsigned char *img[10] = {IMG_TIME_NUM_0,IMG_TIME_NUM_1,IMG_TIME_NUM_2,IMG_TIME_NUM_3,IMG_TIME_NUM_4,
							  IMG_TIME_NUM_5,IMG_TIME_NUM_6,IMG_TIME_NUM_7,IMG_TIME_NUM_8,IMG_TIME_NUM_9};
	unsigned char *img_colon[2] = {IMG_TIME_SPE_NO,IMG_TIME_SPE};

	switch(global_settings.time_format)
	{
	case TIME_FORMAT_12:
		if(date_time.hour > 12)
			hour -= 12;
		else
			hour = date_time.hour;

		IdleShowTime12Format(true);
		break;
	case TIME_FORMAT_24:
		hour = date_time.hour;
		IdleShowTime12Format(false);
		break;
	}

	LCD_ShowImg(IDLE_HOUR_H_X, IDLE_HOUR_H_Y, img[hour/10]);
	LCD_ShowImg(IDLE_HOUR_L_X, IDLE_HOUR_L_Y, img[hour%10]);
	LCD_ShowImg(IDLE_COLON_X, IDLE_COLON_Y, img_colon[colon_flag]);
	LCD_ShowImg(IDLE_MIN_H_X, IDLE_MIN_H_Y, img[date_time.minute/10]);
	LCD_ShowImg(IDLE_MIN_L_X, IDLE_MIN_L_Y, img[date_time.minute%10]);

	colon_flag = !colon_flag;
}

void IdleShowSystemWeek(void)
{
	unsigned char *img_week_cn[7] = {IMG_WEEK_7,IMG_WEEK_1,IMG_WEEK_2,IMG_WEEK_3,IMG_WEEK_4,IMG_WEEK_5,IMG_WEEK_6};
	unsigned char *img_week_en[7] = {IMG_WEEK_SUN,IMG_WEEK_MON,IMG_WEEK_TUE,IMG_WEEK_WED,IMG_WEEK_THU,IMG_WEEK_FRI,IMG_WEEK_SAT};
	unsigned char *img_week;

	if(global_settings.language == LANGUAGE_CHN)
		img_week = img_week_cn[date_time.week];
	else
		img_week = img_week_en[date_time.week];
	
	LCD_ShowImg(IDLE_WEEK_SHOW_X, IDLE_WEEK_SHOW_Y, img_week);
}

void IdleShowDateTime(void)
{
	IdleShowSystemTime();
	IdleShowSystemDate();
	IdleShowSystemWeek();
}

void IdleShowSignal(void)
{
	unsigned char *img[5] = {IMG_SIG_0,IMG_SIG_1,IMG_SIG_2,IMG_SIG_3,IMG_SIG_4};

	LCD_ShowImg(NB_SIGNAL_X, NB_SIGNAL_Y, img[g_nb_sig]);
}

void IdleShowBatSoc(void)
{
	static u8_t index = 0;

	unsigned char *img[6] = {IMG_BAT_0,IMG_BAT_1,IMG_BAT_2,IMG_BAT_3,IMG_BAT_4,IMG_BAT_5};

	if(charger_is_connected&&g_chg_status == BAT_CHARGING_PROGRESS)
	{
		index++;
		if(index>=6)
			index = 0;
	}
	else
	{
		index = g_bat_level;
	}
	
	LCD_ShowImg(BAT_LEVEL_X, BAT_LEVEL_Y, img[index]);
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

void IdleScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_IDLE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATED;
		
		LCD_Clear(BLACK);
		IdleShowSignal();
		IdleShowDateTime();
		IdleShowBleStatus(g_ble_connected);
		IdleShowBatSoc();
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
			IdleShowBatSoc();
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

void FindShowStatus(void)
{
	unsigned char *img_anima[3] = {IMG_FIND_ICON_1,IMG_FIND_ICON_2,IMG_FIND_ICON_3};
	unsigned char *img[2] = {IMG_FIND_CN,IMG_FIND_EN};
	
	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(FIND_ICON_X, FIND_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#else
	LCD_ShowImg(FIND_ICON_X, FIND_ICON_Y, IMG_FIND_ICON_3);
#endif

	if(global_settings.language == LANGUAGE_CHN)
		LCD_ShowImg(FIND_TEXT_X, FIND_TEXT_Y, img[0]);
	else
		LCD_ShowImg(FIND_TEXT_X, FIND_TEXT_Y, img[1]);	
}

void FindDeviceScreenProcess(void)
{
	u16_t rect_x,rect_y,rect_w=180,rect_h=80;
	u16_t x,y,w,h;

	switch(scr_msg[SCREEN_ID_FIND_DEVICE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_FIND_DEVICE].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_FIND_DEVICE].status = SCREEN_STATUS_CREATED;
				
		FindShowStatus();
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
		LCD_Clear(BLACK);
		LCD_ShowImg(MSG_SENDING_X, MSG_SENDING_Y, IMG_SENDING_3);
		LCD_ShowImg(MSG_SEND_OK_X, MSG_SEND_OK_Y, IMG_SENDING_OK);
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
	unsigned char *img_anima[9] = {IMG_SENDING_1,IMG_SENDING_2,IMG_SENDING_3,
								   IMG_SENDING_1,IMG_SENDING_2,IMG_SENDING_3,
								   IMG_SENDING_1,IMG_SENDING_2,IMG_SENDING_3};
	unsigned char *img_icon[2] = {IMG_SOS_ICON,IMG_SENDING_OK};
	
	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(MSG_SENDING_X, MSG_SENDING_Y, img_anima, ARRAY_SIZE(img_anima), 500, false, SOSSChangrStatus);
#else
	LCD_ShowImg(MSG_SENDING_X, MSG_SENDING_Y, IMG_SENDING_3);
#endif

	LCD_ShowImg(SOS_ICON_X, SOS_ICON_Y, img_icon[0]);
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

#ifdef CONFIG_IMU_SUPPORT
void SleepUpdateStatus(void)
{
	u16_t total_sleep,deep_sleep,light_sleep;
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};
	
	GetSleepTimeData(&deep_sleep, &light_sleep);
	total_sleep = deep_sleep+light_sleep;
	
	LCD_ShowImg(SLEEP_NUM_1_X, SLEEP_NUM_1_Y, img_num[(total_sleep/60)/10]);
	LCD_ShowImg(SLEEP_NUM_2_X, SLEEP_NUM_2_Y, img_num[(total_sleep/60)%10]);
	LCD_ShowImg(SLEEP_NUM_3_X, SLEEP_NUM_3_Y, img_num[(total_sleep%60)/10]);
	LCD_ShowImg(SLEEP_NUM_4_X, SLEEP_NUM_4_Y, img_num[(total_sleep%60)%10]);
}

void SleepShowStatus(void)
{
	u16_t total_sleep,deep_sleep,light_sleep;
	unsigned char *img_anima[4] = {IMG_SLEEP_ICON_1,IMG_SLEEP_ICON_2,IMG_SLEEP_ICON_3,IMG_SLEEP_ICON_4};
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};
	unsigned char *img_uint_hr[2] = {IMG_SLEEP_HR_CN, IMG_SLEEP_HR_EN};
	unsigned char *img_uint_min[2] = {IMG_SLEEP_MIN_CN, IMG_SLEEP_MIN_EN};
	
	GetSleepTimeData(&deep_sleep, &light_sleep);
	total_sleep = deep_sleep+light_sleep;
	
	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(SLEEP_ICON_X, SLEEP_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#else
	LCD_ShowImg(SLEEP_ICON_X, SLEEP_ICON_Y, IMG_SLEEP_ICON_4);
#endif

	LCD_ShowImg(SLEEP_NUM_1_X, SLEEP_NUM_1_Y, img_num[(total_sleep/60)/10]);
	LCD_ShowImg(SLEEP_NUM_2_X, SLEEP_NUM_2_Y, img_num[(total_sleep/60)%10]);
	LCD_ShowImg(SLEEP_NUM_3_X, SLEEP_NUM_3_Y, img_num[(total_sleep%60)/10]);
	LCD_ShowImg(SLEEP_NUM_4_X, SLEEP_NUM_4_Y, img_num[(total_sleep%60)%10]);

	if(global_settings.language == LANGUAGE_CHN)
	{
		LCD_ShowImg(SLEEP_HOUR_X, SLEEP_HOUR_Y, img_uint_hr[0]);
		LCD_ShowImg(SLEEP_MIN_X, SLEEP_MIN_Y, img_uint_min[0]);
	}
	else
	{
		LCD_ShowImg(SLEEP_HOUR_X, SLEEP_HOUR_Y, img_uint_hr[1]);
		LCD_ShowImg(SLEEP_MIN_X, SLEEP_MIN_Y, img_uint_min[1]);
	}
}

void SleepScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_SLEEP].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_SLEEP].status = SCREEN_STATUS_CREATED;

		SleepShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		SleepUpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
}

void StepsUpdateStatus(void)
{
	u16_t steps;
	u16_t offset_x;
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};
	
	GetSportData(&steps,NULL,NULL);

	if(global_settings.language == LANGUAGE_CHN)
		offset_x = STEPS_CN_OFFSET;
	else
		offset_x = 0;

	LCD_ShowImg(STEPS_NUM_1_X+offset_x, STEPS_NUM_1_Y, img_num[steps/10000]);
	LCD_ShowImg(STEPS_NUM_2_X+offset_x, STEPS_NUM_2_Y, img_num[(steps%10000)/1000]);
	LCD_ShowImg(STEPS_NUM_3_X+offset_x, STEPS_NUM_3_Y, img_num[(steps%1000)/100]);
	LCD_ShowImg(STEPS_NUM_4_X+offset_x, STEPS_NUM_4_Y, img_num[(steps%100)/10]);
	LCD_ShowImg(STEPS_NUM_5_X+offset_x, STEPS_NUM_5_Y, img_num[steps%10]);	
}

void StepsShowStatus(void)
{
	u16_t steps;
	u16_t offset_x;
	unsigned char *img_anima[2] = {IMG_STEP_ICON_1, IMG_STEP_ICON_2};
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};
	unsigned char *img_uint[2] = {IMG_STEP_CN, IMG_STEP_EN};
	
	GetSportData(&steps,NULL,NULL);

	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(STEPS_ICON_X, STEPS_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#else
	LCD_ShowImg(STEPS_ICON_X, STEPS_ICON_Y, IMG_STEP_ICON_1);
#endif

	if(global_settings.language == LANGUAGE_CHN)
		offset_x = STEPS_CN_OFFSET;
	else
		offset_x = 0;

	LCD_ShowImg(STEPS_NUM_1_X+offset_x, STEPS_NUM_1_Y, img_num[steps/10000]);
	LCD_ShowImg(STEPS_NUM_2_X+offset_x, STEPS_NUM_2_Y, img_num[(steps%10000)/1000]);
	LCD_ShowImg(STEPS_NUM_3_X+offset_x, STEPS_NUM_3_Y, img_num[(steps%1000)/100]);
	LCD_ShowImg(STEPS_NUM_4_X+offset_x, STEPS_NUM_4_Y, img_num[(steps%100)/10]);
	LCD_ShowImg(STEPS_NUM_5_X+offset_x, STEPS_NUM_5_Y, img_num[steps%10]);

	//if(global_settings.language == LANGUAGE_CHN)
	//	LCD_ShowImg(STEPS_UNIT_X+offset_x, STEPS_UNIT_Y, img_uint[0]);
	//else
	//	LCD_ShowImg(STEPS_UNIT_X, STEPS_UNIT_Y, img_uint[1]);	
}

void StepsScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_STEPS].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_STEPS].status = SCREEN_STATUS_CREATED;

		StepsShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_STEPS].para&SCREEN_EVENT_UPDATE_SPORT)
			scr_msg[SCREEN_ID_STEPS].para &= (~SCREEN_EVENT_UPDATE_SPORT);

		StepsUpdateStatus();
		break;
	}

	scr_msg[SCREEN_ID_STEPS].act = SCREEN_ACTION_NO;
}

void DistanceUpdateStatus(void)
{
	u16_t distance;
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};

	GetSportData(NULL,NULL,&distance);

	LCD_ShowImg(DIS_NUM_1_X, DIS_NUM_1_Y, img_num[distance/10000]);
	LCD_ShowImg(DIS_NUM_2_X, DIS_NUM_2_Y, img_num[(distance%10000)/1000]);
	LCD_ShowImg(DIS_DOT_X, DIS_DOT_Y, IMG_DOT);
	LCD_ShowImg(DIS_NUM_3_X, DIS_NUM_3_Y, img_num[(distance%100)/10]);
	LCD_ShowImg(DIS_NUM_4_X, DIS_NUM_4_Y, img_num[distance%10]);
}

void DistanceShowStatus(void)
{
	u16_t distance;
	unsigned char *img_anima[2] = {IMG_DISTANCE_ICON_1, IMG_DISTANCE_ICON_2};
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};
	unsigned char *img_km[2] = {IMG_KM_CN, IMG_KM_EN};
	
	GetSportData(NULL,NULL,&distance);

	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaShow(DIS_ICON_X, DIS_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#else
	LCD_ShowImg(DIS_ICON_X, DIS_ICON_Y, IMG_DISTANCE_ICON_1);
#endif

	LCD_ShowImg(DIS_NUM_1_X, DIS_NUM_1_Y, img_num[distance/10000]);
	LCD_ShowImg(DIS_NUM_2_X, DIS_NUM_2_Y, img_num[(distance%10000)/1000]);
	LCD_ShowImg(DIS_DOT_X, DIS_DOT_Y, IMG_DOT);
	LCD_ShowImg(DIS_NUM_3_X, DIS_NUM_3_Y, img_num[(distance%100)/10]);
	LCD_ShowImg(DIS_NUM_4_X, DIS_NUM_4_Y, img_num[distance%10]);
	
	if(global_settings.language == LANGUAGE_CHN)
		LCD_ShowImg(DIS_KM_X, DIS_KM_Y, img_km[0]);
	else
		LCD_ShowImg(DIS_KM_X, DIS_KM_Y, img_km[1]);
}

void DistanceScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_DISTANCE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_DISTANCE].status = SCREEN_STATUS_CREATED;

		DistanceShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_DISTANCE].para&SCREEN_EVENT_UPDATE_SPORT)
			scr_msg[SCREEN_ID_DISTANCE].para &= (~SCREEN_EVENT_UPDATE_SPORT);

		DistanceUpdateStatus();
		break;
	}

	scr_msg[SCREEN_ID_DISTANCE].act = SCREEN_ACTION_NO;
}

void CalorieUpdateStatus(void)
{
	u16_t distance;
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};

	GetSportData(NULL,NULL,&distance);

	LCD_ShowImg(DIS_NUM_1_X, DIS_NUM_1_Y, img_num[distance/10000]);
	LCD_ShowImg(DIS_NUM_2_X, DIS_NUM_2_Y, img_num[(distance%10000)/1000]);
	LCD_ShowImg(DIS_DOT_X, DIS_DOT_Y, IMG_DOT);
	LCD_ShowImg(DIS_NUM_3_X, DIS_NUM_3_Y, img_num[(distance%1000)/100]);
	LCD_ShowImg(DIS_NUM_4_X, DIS_NUM_4_Y, img_num[distance%100]);
}

void CalorieShowStatus(void)
{
	u16_t calorie;
	unsigned char *img_anima[2] = {IMG_CAL_ICON_1, IMG_CAL_ICON_2};
	unsigned char *img_num[10] = {IMG_MID_NUM_0,IMG_MID_NUM_1,IMG_MID_NUM_2,IMG_MID_NUM_3,IMG_MID_NUM_4,
							  IMG_MID_NUM_5,IMG_MID_NUM_6,IMG_MID_NUM_7,IMG_MID_NUM_8,IMG_MID_NUM_9};
	unsigned char *img_cal[2] = {IMG_CAL_CN, IMG_CAL_EN};
	
	GetSportData(NULL,&calorie,NULL);

	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaShow(CAL_ICON_X, CAL_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#else
	LCD_ShowImg(CAL_ICON_X, CAL_ICON_Y, IMG_CAL_ICON_1);
#endif

	LCD_ShowImg(CAL_NUM_1_X, CAL_NUM_1_Y, img_num[calorie/1000]);
	LCD_ShowImg(CAL_NUM_2_X, CAL_NUM_2_Y, img_num[(calorie%1000)/100]);
	LCD_ShowImg(CAL_NUM_3_X, CAL_NUM_3_Y, img_num[(calorie%100)/10]);
	LCD_ShowImg(CAL_NUM_4_X, CAL_NUM_4_Y, img_num[calorie%10]);
	
	if(global_settings.language == LANGUAGE_CHN)
		LCD_ShowImg(CAL_UNIT_X, CAL_UNIT_Y, img_cal[0]);
	else
		LCD_ShowImg(CAL_UNIT_X, CAL_UNIT_Y, img_cal[1]);
}

void CalorieScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_CALORIE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_CALORIE].status = SCREEN_STATUS_CREATED;

		CalorieShowStatus();
		break;
		
	case SCREEN_ACTION_UPDATE:
		if(scr_msg[SCREEN_ID_CALORIE].para&SCREEN_EVENT_UPDATE_SPORT)
			scr_msg[SCREEN_ID_CALORIE].para &= (~SCREEN_EVENT_UPDATE_SPORT);

		CalorieUpdateStatus();
		break;
	}

	scr_msg[SCREEN_ID_CALORIE].act = SCREEN_ACTION_NO;
}

void FallUpdateStatus(void)
{
	unsigned char *img_anima_send[3] = {IMG_SENDING_1, IMG_SENDING_2, IMG_SENDING_3};
	unsigned char *img_fall[2] = {IMG_FALL_CN, IMG_FALL_EN};
	unsigned char *img_cancel[2] = {IMG_FALL_MSG_CANCEL_CN, IMG_FALL_MSG_CANCEL_EN};

	LCD_Clear(BLACK);
	
	switch(fall_state)
	{
	case FALL_STATUS_IDLE:
		break;
		
	case FALL_STATUS_NOTIFY:
		break;
	
	case FALL_STATUS_SENDING:
	#ifdef CONFIG_ANIMATION_SUPPORT	
		AnimaShow(MSG_SENDING_X, MSG_SENDING_Y, img_anima_send, ARRAY_SIZE(img_anima_send), 500, true, NULL);
	#else
		LCD_ShowImg(MSG_SENDING_X, MSG_SENDING_Y, IMG_SENDING_3);
	#endif
	
		if(global_settings.language == LANGUAGE_CHN)
			LCD_ShowImg(FALL_TEXT_X, FALL_TEXT_Y, img_fall[0]);
		else
			LCD_ShowImg(FALL_TEXT_X, FALL_TEXT_Y, img_fall[1]);	
		break;
		
	case FALL_STATUS_SENT:
	#ifdef CONFIG_ANIMATION_SUPPORT	
		AnimaStopShow();
	#endif
		LCD_Clear(BLACK);
		LCD_ShowImg(MSG_SENDING_X, MSG_SENDING_Y, IMG_SENDING_3);
		LCD_ShowImg(MSG_SEND_OK_X, MSG_SEND_OK_Y, IMG_SENDING_OK);
		break;
		
	case FALL_STATUS_CANCEL:
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaStopShow();
	#endif
		LCD_Clear(BLACK);
		if(global_settings.language == LANGUAGE_CHN)
			LCD_ShowImg(FALL_CANCEL_CN_X, FALL_CANCEL_CN_Y, img_cancel[0]);
		else
			LCD_ShowImg(FALL_CANCEL_EN_X, FALL_CANCEL_EN_Y, img_cancel[1]);	
		break;

	case FALL_STATUS_CANCELED:
		break;
	}
}

void FallShowStatus(void)
{
	unsigned char *img_anima[3] = {IMG_FALL_ICON_1, IMG_FALL_ICON_2,IMG_FALL_ICON_3};
	unsigned char *img_fall[2] = {IMG_FALL_CN, IMG_FALL_EN};

	LCD_Clear(BLACK);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaShow(FALL_ICON_X, FALL_ICON_Y, img_anima, ARRAY_SIZE(img_anima), 500, true, NULL);
#else
	LCD_ShowImg(FALL_ICON_X, FALL_ICON_Y, IMG_FALL_ICON_3);
#endif

	if(global_settings.language == LANGUAGE_CHN)
		LCD_ShowImg(FALL_TEXT_X, FALL_TEXT_Y, img_fall[0]);
	else
		LCD_ShowImg(FALL_TEXT_X, FALL_TEXT_Y, img_fall[1]);
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
#endif

void WristShowStatus(void)
{
//	unsigned char *img[2] = {IMG_WRIST_OFF_CN, IMG_WRIST_OFF_EN};

//	if(global_settings.language == LANGUAGE_CHN)
//		LCD_ShowImg(CAL_UNIT_X, CAL_UNIT_Y, img[0]);
//	else
//		LCD_ShowImg(CAL_UNIT_X, CAL_UNIT_Y, img[1]);
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

#ifdef CONFIG_FOTA_DOWNLOAD
void FOTAShowStatus(void)
{
	u16_t x,y,w,h;
	u8_t str_title[] = "FOTA RUNNING";

	LCD_Clear(BLACK);
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_ShowStrInRect(FOTA_NOTIFY_STRING_X, 
					  FOTA_NOTIFY_STRING_Y, 
					  FOTA_NOTIFY_STRING_W, 
					  FOTA_NOTIFY_STRING_H, 
					  "FOTA will be started after 5 seconds!");
#else
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
#endif
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
	#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
		LCD_Clear(BLACK);
		LCD_ShowStrInRect(FOTA_NOTIFY_STRING_X, 
						  FOTA_NOTIFY_STRING_Y, 
						  FOTA_NOTIFY_STRING_W, 
						  FOTA_NOTIFY_STRING_H, 
						  "Linking to server...");
	#else
		LCD_Fill(FOTA_NOTIFY_RECT_X+1, FOTA_NOTIFY_STRING_Y, FOTA_NOTIFY_RECT_W-1, FOTA_NOTIFY_RECT_H-(FOTA_NOTIFY_STRING_Y-FOTA_NOTIFY_RECT_Y)-1, BLACK);
		ShowStringsInRect(FOTA_NOTIFY_STRING_X,
						  FOTA_NOTIFY_STRING_Y,
						  FOTA_NOTIFY_STRING_W,
						  FOTA_NOTIFY_STRING_H,
						  FONT_SIZE_16,
						  "Linking to server...");
	#endif
		ClearAllKeyHandler();
		break;
		
	case FOTA_STATUS_DOWNLOADING:
		if(!flag)
		{
			flag = true;
		#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			LCD_Clear(BLACK);
			LCD_ShowStrInRect(FOTA_NOTIFY_STRING_X, 
							  FOTA_NOTIFY_STRING_Y, 
							  FOTA_NOTIFY_STRING_W, 
							  FOTA_NOTIFY_STRING_H, 
							  "Downloading data...");
		#else
			LCD_Fill(FOTA_NOTIFY_STRING_X, FOTA_NOTIFY_STRING_Y, FOTA_NOTIFY_STRING_W, FOTA_NOTIFY_STRING_H, BLACK);
			ShowStringsInRect(FOTA_NOTIFY_STRING_X, 
							  FOTA_NOTIFY_STRING_Y,
							  FOTA_NOTIFY_STRING_W,
							  40,
							  FONT_SIZE_16,
							  "Downloading data...");
			LCD_DrawRectangle(FOTA_NOTIFY_PRO_X, FOTA_NOTIFY_PRO_Y, FOTA_NOTIFY_PRO_W, FOTA_NOTIFY_PRO_H);
			LCD_Fill(FOTA_NOTIFY_PRO_X+1, FOTA_NOTIFY_PRO_Y+1, FOTA_NOTIFY_PRO_W-1, FOTA_NOTIFY_PRO_H-1, BLACK);
		#endif	
			

			sprintf(pro_buf, "%3d%%", g_fota_progress);
			LCD_MeasureString(pro_buf, &w, &h);
			pro_str_x = ((FOTA_NOTIFY_RECT_W-2*FOTA_NOTIFY_OFFSET_W)-w)/2;
			pro_str_x += (FOTA_NOTIFY_RECT_X+FOTA_NOTIFY_OFFSET_W);
			pro_str_y = FOTA_NOTIFY_PRO_Y + FOTA_NOTIFY_PRO_H + 8;
			
			LCD_ShowString(pro_str_x,pro_str_y, pro_buf);
		}
		else
		{
			pro_len = (g_fota_progress*FOTA_NOTIFY_PRO_W)/100;
			LCD_Fill(FOTA_NOTIFY_PRO_X+1, FOTA_NOTIFY_PRO_Y+1, pro_len, FOTA_NOTIFY_PRO_H-1, WHITE);

			sprintf(pro_buf, "%3d%%", g_fota_progress);
			LCD_ShowString(pro_str_x, pro_str_y, pro_buf);
		}

		ClearAllKeyHandler();
		break;
		
	case FOTA_STATUS_FINISHED:
		flag = false;
	#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
		LCD_Clear(BLACK);
		LCD_ShowStrInRect(FOTA_NOTIFY_STRING_X, 
						  FOTA_NOTIFY_STRING_Y, 
						  FOTA_NOTIFY_STRING_W, 
						  FOTA_NOTIFY_STRING_H, 
						  "Upgraded finished! pls reboot!");
	#else
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
	#endif

		SetLeftKeyUpHandler(fota_reboot_confirm);
		break;
		
	case FOTA_STATUS_ERROR:
		flag = false;

	#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
		LCD_ShowStrInRect(FOTA_NOTIFY_STRING_X, 
						  FOTA_NOTIFY_STRING_Y, 
						  FOTA_NOTIFY_STRING_W, 
						  FOTA_NOTIFY_STRING_H, 
						  "Upgrade failed! pls check the network or server.");
	#else
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
	#endif

		SetLeftKeyUpHandler(fota_exit);
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

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(5), NULL);

	SetLeftKeyUpHandler(EnterPoweroffScreen);
}
#endif/*CONFIG_FOTA_DOWNLOAD*/

void TestGPSUpdateInfor(void)
{
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_Clear(BLACK);
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, gps_test_info);
#else
	LCD_Fill(30, 50, 190, 160, BLACK);
	LCD_ShowStringInRect(30, 50, 180, 160, gps_test_info);
#endif/*LCD_VGM068A4W01_SH1106G*/
}

void TestGPSShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);
	
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, "GPS TESTING");
#else
	strcpy(strbuf, "GPS TESTING");
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect(30, 50, 180, 160, gps_test_info);
#endif/*LCD_VGM068A4W01_SH1106G*/
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
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_Clear(BLACK);
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, nb_test_info);
#else
	LCD_Fill(30, 50, 190, 160, BLACK);
	LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);
#endif
}

void TestNBShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, "NB-IoT TESTING");
#else	
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

void TestBLEShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, "BLE TESTING");
	LCD_ShowStrInRect(0, 2, LCD_WIDTH, LCD_HEIGHT, "please test it on your phone's APP");
#else	
	strcpy(strbuf, "NB-IoT TESTING");
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect(30, 50, 180, 160, "BLE TESTING");
#endif
}

void TestBLEUpdateINfor(void)
{
}

void TestBLEScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_BLE_TEST].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_BLE_TEST].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_BLE_TEST].status = SCREEN_STATUS_CREATED;

		TestBLEShowInfor();
		break;
		
	case SCREEN_ACTION_UPDATE:
		TestBLEUpdateINfor();
		break;
	}
	
	scr_msg[SCREEN_ID_BLE_TEST].act = SCREEN_ACTION_NO;
}

#ifdef CONFIG_WIFI
void TestWIFIShowInfor(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);

#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, "WIFI TESTING");
#else	
	strcpy(strbuf, "WIFI TESTING");
	LCD_MeasureString(strbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2, 20, strbuf);
	LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);
#endif
}

void TestWIFIUpdateINfor(void)
{
#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
	LCD_Clear(BLACK);
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, wifi_test_info);
#else
	LCD_Fill(30, 50, 190, 160, BLACK);
	LCD_ShowStringInRect(30, 50, 180, 160, wifi_test_info);
#endif
}

void TestWIFIScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_WIFI_TEST].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_WIFI_TEST].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_WIFI_TEST].status = SCREEN_STATUS_CREATED;

		TestWIFIShowInfor();
		break;
		
	case SCREEN_ACTION_UPDATE:
		TestWIFIUpdateINfor();
		break;
	}
	
	scr_msg[SCREEN_ID_WIFI_TEST].act = SCREEN_ACTION_NO;
}
#endif

#ifdef CONFIG_IMU_SUPPORT
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
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_FOTA_DOWNLOAD
	SetLeftKeyUpHandler(fota_start);
#else
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#endif
}

void ExitCalorieScreen(void)
{
	EnterIdleScreen();
}

void EnterCalorieScreen(void)
{
	if(screen_id == SCREEN_ID_CALORIE)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_CALORIE;	
	scr_msg[SCREEN_ID_CALORIE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_CALORIE].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);
	
	MenuStopGPS();
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
	SetLeftKeyUpHandler(EnterSleepScreen);
}

void ExitDistanceScreen(void)
{
	EnterIdleScreen();
}

void EnterDistanceScreen(void)
{
	if(screen_id == SCREEN_ID_DISTANCE)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_DISTANCE;	
	scr_msg[SCREEN_ID_DISTANCE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_DISTANCE].status = SCREEN_STATUS_CREATING;

	k_timer_stop(&mainmenu_timer);

	MenuStopGPS();
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
	SetLeftKeyUpHandler(EnterCalorieScreen);
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

	SetLeftKeyUpHandler(EnterDistanceScreen);
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
}

#endif

void EnterIdleScreen(void)
{
	if(screen_id == SCREEN_ID_IDLE)
		return;

	LCD_Set_BL_Mode(LCD_BL_AUTO);

#ifdef CONFIG_ANIMATION_SUPPORT
	AnimaStopShow();
#endif

	k_timer_stop(&notify_timer);
	k_timer_stop(&mainmenu_timer);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_IDLE;
	scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATING;

#if defined(CONFIG_PPG_SUPPORT)
	SetLeftKeyUpHandler(EnterHRScreen);
#elif defined(CONFIG_IMU_SUPPORT)
	SetLeftKeyUpHandler(EnterStepsScreen);
#else
	SetLeftKeyUpHandler(EnterPoweroffScreen);
#endif
	SetLeftKeyLongPressHandler(SOSTrigger);
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

void ExitFindDeviceScreen(void)
{
	if(screen_id == SCREEN_ID_FIND_DEVICE)
	{
		EnterIdleScreen();
	}	
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

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FindDeviceStop);
}


void poweroff_leftkeyfunc(void)
{
	key_pwroff_flag = true;
	
	scr_msg[SCREEN_ID_POWEROFF].act = SCREEN_ACTION_UPDATE;

	ClearAllKeyHandler();
}

void poweroff_rightkeyfunc(void)
{
	EnterIdleScreen();
}

void ExitPoweroffScreen(void)
{
	k_timer_stop(&mainmenu_timer);
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

	k_timer_stop(&mainmenu_timer);

#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif

	SetLeftKeyUpHandler(ExitPoweroffScreen);
	SetLeftKeyLongPressHandler(poweroff_leftkeyfunc);
}

void PowerOffUpdateStatus(void)
{
	unsigned char *img_pwroff[2] = {IMG_PWROFF_CN, IMG_PWROFF_EN};

	LCD_Clear(BLACK);

	if(global_settings.language == LANGUAGE_CHN)
		LCD_ShowImg(PWR_OFF_CN_X, PWR_OFF_CN_Y, img_pwroff[0]);
	else
		LCD_ShowImg(PWR_OFF_EN_X, PWR_OFF_EN_Y, img_pwroff[1]);
}

void PowerOffShowStatus(void)
{
	unsigned char *img_pwroff_key[2] = {IMG_PWROFF_KEY_CN, IMG_PWROFF_KEY_EN};
	
	LCD_Clear(BLACK);

	LCD_ShowImg(PWR_OFF_ICON_X, PWR_OFF_ICON_Y, IMG_PWROFF_ICON);
	if(global_settings.language == LANGUAGE_CHN)
		LCD_ShowImg(PWR_OFF_KEY_CN_X, PWR_OFF_KEY_CN_Y, img_pwroff_key[0]);
	else
		LCD_ShowImg(PWR_OFF_KEY_EN_X, PWR_OFF_KEY_EN_Y, img_pwroff_key[1]);
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
		PowerOffUpdateStatus();
		break;
	}
	
	scr_msg[SCREEN_ID_POWEROFF].act = SCREEN_ACTION_NO;
}

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

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	k_timer_stop(&mainmenu_timer);
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

#ifdef CONFIG_PPG_SUPPORT
	PPGStopCheck();
#endif

	SetLeftKeyUpHandler(EnterNBTestScreen);
}

void ExitBLETestScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	
	EnterIdleScreen();
}

void EnterBLETestScreen(void)
{
	if(screen_id == SCREEN_ID_BLE_TEST)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_BLE_TEST;	
	scr_msg[SCREEN_ID_BLE_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_BLE_TEST].status = SCREEN_STATUS_CREATING;

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	k_timer_stop(&mainmenu_timer);
#ifdef CONFIG_WIFI	
	MenuStopWifi();
#endif
	SetLeftKeyUpHandler(EnterPoweroffScreen);
}

#ifdef CONFIG_WIFI
void ExitWifiTestScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	MenuStopWifi();
	EnterIdleScreen();
}

void EnterWifiTestScreen(void)
{
	if(screen_id == SCREEN_ID_WIFI_TEST)
		return;

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_WIFI_TEST;	
	scr_msg[SCREEN_ID_WIFI_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_WIFI_TEST].status = SCREEN_STATUS_CREATING;	

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	k_timer_stop(&mainmenu_timer);
	MenuStopNB();
	
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);

	SetLeftKeyUpHandler(EnterBLETestScreen);
}
#endif

void ExitNBTestScreen(void)
{
	k_timer_stop(&mainmenu_timer);
	MenuStopNB();
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

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	k_timer_stop(&mainmenu_timer);
	MenuStopGPS();
	
	k_timer_start(&mainmenu_timer, K_SECONDS(3), NULL);
#ifdef CONFIG_WIFI
	SetLeftKeyUpHandler(EnterWifiTestScreen);
#else
	SetLeftKeyUpHandler(EnterBLETestScreen);
#endif
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
}
#endif/*CONFIG_PPG_SUPPORT*/

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
		case SCREEN_ID_DISTANCE:
			DistanceScreenProcess();
			break;
		case SCREEN_ID_CALORIE:
			CalorieScreenProcess();
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
		case SCREEN_ID_BLE_TEST:
			TestBLEScreenProcess();
			break;
	#ifdef CONFIG_WIFI		
		case SCREEN_ID_WIFI_TEST:
			TestWIFIScreenProcess();
			break;
	#endif		
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

