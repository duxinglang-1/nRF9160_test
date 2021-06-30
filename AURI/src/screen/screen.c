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
#include "datetime.h"
#include "max20353.h"
#include "lsm6dso.h"
#include "external_flash.h"
#include "screen.h"
#include "ucs2.h"
#include "nb.h"
#include "sos.h"
#include "gps.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(screen, CONFIG_LOG_DEFAULT_LEVEL);

static void NotifyTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(notify_timer, NotifyTimerOutCallBack, NULL);


SCREEN_ID_ENUM screen_id = SCREEN_ID_BOOTUP;
SCREEN_ID_ENUM history_screen_id = SCREEN_ID_BOOTUP;
screen_msg scr_msg[SCREEN_ID_MAX] = {0};
notify_infor notify_msg = {0};

void ShowBootUpLogo(void)
{
	u16_t x,y,w,h;

#ifdef IMG_FONT_FROM_FLASH
	LCD_get_pic_size_from_flash(IMG_AURI_LOGO_ADDR, &w, &h);
	x = (w > LCD_WIDTH ? 0 : (LCD_WIDTH-w)/2);
	y = (h > LCD_HEIGHT ? 0 : (LCD_HEIGHT-h)/2);
	LCD_ShowImg_From_Flash(0, 0, IMG_AURI_LOGO_ADDR);
#else
	//LCD_get_pic_size(jjph_gc_96X32, &w, &h);
	//x = (w > LCD_WIDTH ? 0 : (LCD_WIDTH-w)/2);
	//y = (h > LCD_HEIGHT ? 0 : (LCD_HEIGHT-h)/2);
	//LCD_ShowImg(0, 0, jjph_gc_96X32);
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
	EntryIdleScreen();
#endif
}

void NotifyTimerOutCallBack(struct k_timer *timer_id)
{
	ExitNotifyScreen();
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

void IdleShowSystemTime(void)
{
	static bool colon_flag = true;
	
#ifdef IMG_FONT_FROM_FLASH
	u32_t img_addr[10] = {IMG_BIG_NUM_0_ADDR,IMG_BIG_NUM_1_ADDR,IMG_BIG_NUM_2_ADDR,IMG_BIG_NUM_3_ADDR,IMG_BIG_NUM_4_ADDR,
						  IMG_BIG_NUM_5_ADDR,IMG_BIG_NUM_6_ADDR,IMG_BIG_NUM_7_ADDR,IMG_BIG_NUM_8_ADDR,IMG_BIG_NUM_9_ADDR};
	u32_t img_colon_addr[2] = {IMG_NO_COLON_ADDR,IMG_COLON_ADDR};
#else
	unsigned char *img[10] = {IMG_BIG_NUM_0,IMG_BIG_NUM_1,IMG_BIG_NUM_2,IMG_BIG_NUM_3,IMG_BIG_NUM_4,
							  IMG_BIG_NUM_5,IMG_BIG_NUM_6,IMG_BIG_NUM_7,IMG_BIG_NUM_8,IMG_BIG_NUM_9};
	unsigned char *img_colon[2] = {IMG_NO_COLON,IMG_COLON};
#endif

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(IDLE_HOUR_H_X, IDLE_HOUR_H_Y, img_addr[date_time.hour/10]);
	LCD_ShowImg_From_Flash(IDLE_HOUR_L_X, IDLE_HOUR_L_Y, img_addr[date_time.hour%10]);
	LCD_ShowImg_From_Flash(IDLE_COLON_X, IDLE_COLON_Y, img_colon_addr[colon_flag]);
	LCD_ShowImg_From_Flash(IDLE_MIN_H_X, IDLE_MIN_H_Y, img_addr[date_time.minute/10]);
	LCD_ShowImg_From_Flash(IDLE_MIN_L_X, IDLE_MIN_L_Y, img_addr[date_time.minute%10]);
#else
	LCD_ShowImg(IDLE_HOUR_H_X, IDLE_HOUR_H_Y, img[date_time.hour/10]);
	LCD_ShowImg(IDLE_HOUR_L_X, IDLE_HOUR_L_Y, img[date_time.hour%10]);
	LCD_ShowImg(IDLE_COLON_X, IDLE_COLON_Y, img_colon[colon_flag]);
	LCD_ShowImg(IDLE_MIN_H_X, IDLE_MIN_H_Y, img[date_time.minute/10]);
	LCD_ShowImg(IDLE_MIN_L_X, IDLE_MIN_L_Y, img[date_time.minute%10]);
#endif

	colon_flag = !colon_flag;
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
	//IdleShowSystemDate();
	//IdleShowSystemWeek();
}

void IdleUpdateBatSoc(void)
{
	u16_t x,y,w,h;
	u8_t strbuf[10] = {0};
	u8_t tmpbuf[128] = {0};
	
	//LCD_Fill(BAT_SUBJECT_X+1,BAT_SUBJECT_Y+1,BAT_SUBJECT_W-2,BAT_SUBJECT_H-2,BLACK);
	
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
#ifdef IMG_FONT_FROM_FLASH
	u32_t img_addr[5] = {IMG_SIG_0_ADDR,IMG_SIG_1_ADDR,IMG_SIG_2_ADDR,IMG_SIG_3_ADDR,IMG_SIG_4_ADDR};
#else
	unsigned char *img[5] = {IMG_SIG_0,IMG_SIG_1,IMG_SIG_2,IMG_SIG_3,IMG_SIG_4};
#endif

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(NB_SIGNAL_X, NB_SIGNAL_Y, img_addr[g_nb_sig]);
#else
	LCD_ShowImg(NB_SIGNAL_X, NB_SIGNAL_Y, img[g_nb_sig]);
#endif
}

void IdleShowBatSoc(void)
{
	static u8_t index = 0;
#ifdef IMG_FONT_FROM_FLASH
	u32_t img_addr[6] = {IMG_BAT_0_ADDR,IMG_BAT_1_ADDR,IMG_BAT_2_ADDR,IMG_BAT_3_ADDR,IMG_BAT_4_ADDR,IMG_BAT_5_ADDR};
#else
	unsigned char *img[6] = {IMG_BAT_0,IMG_BAT_1,IMG_BAT_2,IMG_BAT_3,IMG_BAT_4,IMG_BAT_5};
#endif

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
	
#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(BAT_LEVEL_X, BAT_LEVEL_Y, img_addr[index]);
#else
	LCD_ShowImg(BAT_LEVEL_X, BAT_LEVEL_Y, img[index]);
#endif
}

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
			//IdleShowSystemDate();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_WEEK)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_WEEK);
			//IdleShowSystemWeek();
		}
		if(scr_msg[SCREEN_ID_IDLE].para&SCREEN_EVENT_UPDATE_SPORT)
		{
			scr_msg[SCREEN_ID_IDLE].para &= (~SCREEN_EVENT_UPDATE_SPORT);
			//IdleUpdateSportData();
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

void NotifyScreenProcess(void)
{
	u16_t rect_x,rect_y,rect_w=180,rect_h=120;
	u16_t x,y,w,h;
	u16_t offset_w=4,offset_h=4;

	switch(scr_msg[SCREEN_ID_NOTIFY].act)
	{
	case SCREEN_ACTION_ENTER:
		scr_msg[SCREEN_ID_NOTIFY].act = SCREEN_ACTION_NO;
		scr_msg[SCREEN_ID_NOTIFY].status = SCREEN_STATUS_CREATED;
				
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
				while(line_no<line_count)
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
						i -= 2;
						byte_no -= 2;
						tmpbuf[i] = 0x00;

						LCD_MeasureString(tmpbuf, &w, &h);
						x = ((rect_w-2*offset_w)-w)/2;
						x += (rect_x+offset_w);
						LCD_ShowString(x,y,tmpbuf);

						y += line_h;
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
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_ADDR;
	#else
		img = IMG_SOS;
	#endif
		break;
	
	case SOS_STATUS_SENT:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_SEND_ADDR;
	#else
		img = IMG_SOS_SEND;
	#endif
		break;
	
	case SOS_STATUS_RECEIVED:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_RECE_ADDR;
	#else
		img = IMG_SOS_RECE;
	#endif
		break;
	
	case SOS_STATUS_CANCEL:
	#ifdef IMG_FONT_FROM_FLASH
		img_addr = IMG_SOS_ADDR;
	#else
		img = IMG_SOS;
	#endif
		break;
	}

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(SOS_X, SOS_Y, img_addr);
#else
	LCD_ShowImg(SOS_X, SOS_Y, img);
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

void SleepShowStatus(void)
{
	u16_t x,y,img_hour_w,img_hour_h;
	u16_t deep_sleep,light_sleep,total_sleep;
	u32_t img_h_addr,img_m_addr;
	u8_t *img_h,*img_m;
#ifdef IMG_FONT_FROM_FLASH
	u32_t img_addr[10] = {IMG_NUM_0_ADDR,IMG_NUM_1_ADDR,IMG_NUM_2_ADDR,IMG_NUM_3_ADDR,IMG_NUM_4_ADDR,
						  IMG_NUM_5_ADDR,IMG_NUM_6_ADDR,IMG_NUM_7_ADDR,IMG_NUM_8_ADDR,IMG_NUM_9_ADDR};
#else
	unsigned char *img[10] = {IMG_NUM_0,IMG_NUM_1,IMG_NUM_2,IMG_NUM_3,IMG_NUM_4,
							  IMG_NUM_5,IMG_NUM_6,IMG_NUM_7,IMG_NUM_8,IMG_NUM_9};
#endif

	GetSleepTimeData(&deep_sleep, &light_sleep);
	total_sleep = deep_sleep + light_sleep;

	LCD_Clear(BLACK);

	switch(global_settings.language)
	{
	case LANGUAGE_EN:
	#ifdef IMG_FONT_FROM_FLASH
		img_h_addr = IMG_HOUR_EN_ADDR;
		img_m_addr = IMG_MIN_EN_ADDR;
	#else
		img_h = IMG_HOUR_EN;
		img_m = IMG_MIN_EN;
	#endif

		img_hour_w = SLEEP_EN_HOUR_W;
		img_hour_h = SLEEP_EN_HOUR_H;
		break;
		
	case LANGUAGE_CHN:
	#ifdef IMG_FONT_FROM_FLASH
		img_h_addr = IMG_HOUR_CN_ADDR;
		img_m_addr = IMG_MIN_CN_ADDR;
	#else
		img_h = IMG_HOUR_CN;
		img_m = IMG_MIN_CN;
	#endif

		img_hour_w = SLEEP_CN_HOUR_W;
		img_hour_h = SLEEP_CN_HOUR_H;	
		break;
	}

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(SLEEP_ICON_X, SLEEP_ICON_Y, IMG_SLP_ICON_ADDR);
	x = SLEEP_NUM_X;
	y = SLEEP_NUM_Y;
	LCD_ShowImg_From_Flash(x, y, img_addr[(total_sleep/60)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(total_sleep/60)%10]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_h_addr);
	x += img_hour_w;
	LCD_ShowImg_From_Flash(x, y, img_addr[(total_sleep%60)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(total_sleep%60)%10]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_m_addr);
#else
	LCD_ShowImg(SLEEP_ICON_X, SLEEP_ICON_Y, IMG_SLP_ICON);
	x = SLEEP_NUM_X;
	y = SLEEP_NUM_Y;
	LCD_ShowImg(x, y, img[(total_sleep/60)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(total_sleep/60)%10]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img_h);
	x += img_hour_w;
	LCD_ShowImg(x, y, img[(total_sleep%60)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(total_sleep%60)%10]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img_m);
#endif
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
		break;
	}
	
	scr_msg[SCREEN_ID_SLEEP].act = SCREEN_ACTION_NO;
}

void StepsUpdateStatus(void)
{
	u16_t s_count;
	u16_t x,y;
#ifdef IMG_FONT_FROM_FLASH
	u32_t img_addr[10] = {IMG_NUM_0_ADDR,IMG_NUM_1_ADDR,IMG_NUM_2_ADDR,IMG_NUM_3_ADDR,IMG_NUM_4_ADDR,
						  IMG_NUM_5_ADDR,IMG_NUM_6_ADDR,IMG_NUM_7_ADDR,IMG_NUM_8_ADDR,IMG_NUM_9_ADDR};
#else
	unsigned char *img[10] = {IMG_NUM_0,IMG_NUM_1,IMG_NUM_2,IMG_NUM_3,IMG_NUM_4,
							  IMG_NUM_5,IMG_NUM_6,IMG_NUM_7,IMG_NUM_8,IMG_NUM_9};
#endif

	GetImuSteps(&s_count);

#ifdef IMG_FONT_FROM_FLASH
	x = STEPS_NUM_X;
	y = STEPS_NUM_Y;
	LCD_ShowImg_From_Flash(x, y, img_addr[s_count/10000]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(s_count%10000)/1000]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(s_count%1000)/100]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(s_count%100)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[s_count%10]);
#else
	x = STEPS_NUM_X;
	y = STEPS_NUM_Y;
	LCD_ShowImg(x, y, img[s_count/10000]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(s_count%10000)/1000]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(s_count%1000)/100]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(s_count%100)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[s_count%10]);
#endif
}

void StepsShowStatus(void)
{
	u16_t s_count;
	u16_t x,y;
#ifdef IMG_FONT_FROM_FLASH
	u32_t img_addr[10] = {IMG_NUM_0_ADDR,IMG_NUM_1_ADDR,IMG_NUM_2_ADDR,IMG_NUM_3_ADDR,IMG_NUM_4_ADDR,
						  IMG_NUM_5_ADDR,IMG_NUM_6_ADDR,IMG_NUM_7_ADDR,IMG_NUM_8_ADDR,IMG_NUM_9_ADDR};
#else
	unsigned char *img[10] = {IMG_NUM_0,IMG_NUM_1,IMG_NUM_2,IMG_NUM_3,IMG_NUM_4,
							  IMG_NUM_5,IMG_NUM_6,IMG_NUM_7,IMG_NUM_8,IMG_NUM_9};
#endif

	GetImuSteps(&s_count);

	LCD_Clear(BLACK);

#ifdef IMG_FONT_FROM_FLASH
	LCD_ShowImg_From_Flash(STEPS_ICON_X, STEPS_ICON_X, IMG_STEP_ICON_ADDR);
	x = STEPS_NUM_X;
	y = STEPS_NUM_Y;
	LCD_ShowImg_From_Flash(x, y, img_addr[s_count/10000]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(s_count%10000)/1000]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(s_count%1000)/100]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[(s_count%100)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg_From_Flash(x, y, img_addr[s_count%10]);
#else
	LCD_ShowImg(STEPS_ICON_X, STEPS_ICON_Y, IMG_STEP_ICON);
	x = STEPS_NUM_X;
	y = STEPS_NUM_Y;
	LCD_ShowImg(x, y, img[s_count/10000]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(s_count%10000)/1000]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(s_count%1000)/100]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[(s_count%100)/10]);
	x += SMALL_NUM_W;
	LCD_ShowImg(x, y, img[s_count%10]);
#endif
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

void WristShowStatus(void)
{
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
#ifdef LCD_VGM068A4W01_SH1106G
	LCD_Clear(BLACK);
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, gps_test_info);
#else
	LCD_Fill(30, 50, 190, 160, BLACK);
	LCD_ShowStringInRect(30, 50, 180, 160, gps_test_info);
#endif/*LCD_VGM068A4W01_SH1106G*/
}

void TestGPSShowInfor(void)
{
	u32_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);
	
#ifdef LCD_VGM068A4W01_SH1106G
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, gps_test_info);
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
#ifdef LCD_VGM068A4W01_SH1106G
	LCD_Clear(BLACK);
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, nb_test_info);
#else
	LCD_Fill(30, 50, 190, 160, BLACK);
	LCD_ShowStringInRect(30, 50, 180, 160, nb_test_info);
#endif
}

void TestNBShowInfor(void)
{
	u32_t x,y,w,h;
	u8_t strbuf[128] = {0};
	
	LCD_Clear(BLACK);
#ifdef LCD_VGM068A4W01_SH1106G
	LCD_ShowStrInRect(0, 0, LCD_WIDTH, LCD_HEIGHT, nb_test_info);
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
		EntryIdleScreen();
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

void EntryIdleScreen(void)
{
	if(screen_id == SCREEN_ID_IDLE)
		return;

	k_timer_stop(&notify_timer);

	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_IDLE;	
	scr_msg[SCREEN_ID_IDLE].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_IDLE].status = SCREEN_STATUS_CREATING;
}

void EntryMainMenuScreen(void)
{
	static u8_t scr_index=0;
	u16_t scr_id[3] = {SCREEN_ID_IDLE,SCREEN_ID_SLEEP,SCREEN_ID_STEPS};

	if(screen_id == SCREEN_ID_SOS)
		return;
	
	scr_index++;
	if(scr_index>=ARRAY_SIZE(scr_id))
		scr_index = 0;
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = scr_id[scr_index];	
	scr_msg[scr_id[scr_index]].act = SCREEN_ACTION_ENTER;
	scr_msg[scr_id[scr_index]].status = SCREEN_STATUS_CREATING;
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
		case SCREEN_ID_SOS:
			SOSScreenProcess();
			break;
		case SCREEN_ID_SLEEP:
			SleepScreenProcess();
			break;
		case SCREEN_ID_STEPS:
			StepsScreenProcess();
			break;
		case SCREEN_ID_FALL:
			FallScreenProcess();
			break;
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
		case SCREEN_ID_NOTIFY:
			NotifyScreenProcess();
			break;
		}
	}
}

