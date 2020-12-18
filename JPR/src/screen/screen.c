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
#include "datetime.h"
#include "max20353.h"
#include "lsm6dso.h"
#include "external_flash.h"
#include "screen.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(screen, CONFIG_LOG_DEFAULT_LEVEL);

SCREEN_ID_ENUM screen_id = SCREEN_ID_BOOTUP;
SCREEN_ID_ENUM history_screen_id = SCREEN_ID_BOOTUP;
screen_msg scr_msg[SCREEN_ID_MAX] = {0};

void ShowBootUpLogo(void)
{
	u16_t x,y,w,h;

	LCD_get_pic_size_from_flash(IMG_RM_LOGO_240X240_ADDR, &w, &h);
	x = (w > LCD_WIDTH ? 0 : (LCD_WIDTH-w)/2);
	y = (h > LCD_HEIGHT ? 0 : (LCD_HEIGHT-h)/2);
	LCD_dis_pic_from_flash(0, 0, IMG_RM_LOGO_240X240_ADDR);
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

	GetSystemDateStrings(str_date);
	LCD_MeasureString(str_date,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = IDLE_DATE_SHOW_Y;
	LCD_Fill(0, y, LCD_WIDTH, h, BACK_COLOR);	
	LCD_ShowString(x,y,str_date);
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
	LCD_MeasureString(strbuf, &w, &h);
	x = (w > BAT_SUBJECT_W ? BAT_SUBJECT_X : (BAT_SUBJECT_W-w)/2);
	y = (h > BAT_SUBJECT_H ? BAT_SUBJECT_Y : (BAT_SUBJECT_H-h)/2);
	LCD_ShowString(BAT_SUBJECT_X+x, BAT_SUBJECT_Y+y, strbuf);
}

void IdleShowBatSoc(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[10] = {0};

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
	LCD_MeasureString(strbuf, &w, &h);
	x = (w > BAT_SUBJECT_W ? BAT_SUBJECT_X : (BAT_SUBJECT_W-w)/2);
	y = (h > BAT_SUBJECT_H ? BAT_SUBJECT_Y : (BAT_SUBJECT_H-h)/2);
	LCD_ShowString(BAT_SUBJECT_X+x, BAT_SUBJECT_Y+y, strbuf);	
}

void IdleUpdateSportData(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};

	LCD_SetFontSize(FONT_SIZE_16);

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
}

void IdleShowSportData(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_SetFontSize(FONT_SIZE_16);
		
	LCD_Fill(IMU_STEPS_SHOW_X,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "S:%d", g_steps);
	LCD_ShowString(IMU_STEPS_SHOW_X, IMU_STEPS_SHOW_Y, strbuf);

	LCD_Fill(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "D:%d", g_distance);
	LCD_ShowString(IMU_STEPS_SHOW_X+IMU_STEPS_SHOW_W/3, IMU_STEPS_SHOW_Y, strbuf);

	LCD_Fill(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_Y,IMU_STEPS_SHOW_W/3,IMU_STEPS_SHOW_H,BLACK);
	sprintf(strbuf, "C:%d", g_calorie);
	LCD_ShowString(IMU_STEPS_SHOW_X+2*IMU_STEPS_SHOW_W/3, IMU_STEPS_SHOW_Y, strbuf);
}

void IdleScreenProcess(void)
{
	switch(scr_msg[SCREEN_ID_IDLE].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATED;
		
		LCD_Clear(BLACK);
		IdleShowBatSoc();
		IdleShowDateTime();
		IdleShowSportData();
		break;
		
	case SCREEN_ACTION_UPDATE:
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
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SPORT)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SPORT);
			IdleUpdateSportData();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SLEEP)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SLEEP);
		}

		if(scr_msg[SCREEN_ID_IDLE].para == SCREEN_EVENT_UPDATE_NO)
			scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_NO;
	}
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
		
		LCD_SetFontSize(FONT_SIZE_24);
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
		
		LCD_SetFontSize(FONT_SIZE_24);
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

void EnterIdleScreen(void)
{
	if(screen_id == SCREEN_ID_IDLE)
		return;
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_IDLE;
	scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATING;
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
		case SCREEN_ID_HR:
			break;
		case SCREEN_ID_ECG:
			break;
		case SCREEN_ID_BP:
			break;
		case SCREEN_ID_SETTINGS:
			break;
		case SCREEN_ID_GPS_TEST:
			break;
		case SCREEN_ID_NB_TEST:
			break;
		}
	}
}

