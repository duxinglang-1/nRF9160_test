/****************************************Copyright (c)************************************************
** File Name:			    ft_wifi.c
** Descriptions:			Factory test wifi module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#include <string.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_wifi.h"
			
#define FT_WIFI_TITLE_W				150
#define FT_WIFI_TITLE_H				40
#define FT_WIFI_TITLE_X				((LCD_WIDTH-FT_WIFI_TITLE_W)/2)
#define FT_WIFI_TITLE_Y				20

#define FT_WIFI_MENU_STR_W			150
#define FT_WIFI_MENU_STR_H			30
#define FT_WIFI_MENU_STR_X			((LCD_WIDTH-FT_WIFI_MENU_STR_W)/2)
#define FT_WIFI_MENU_STR_Y			80
#define FT_WIFI_MENU_STR_OFFSET_Y	5

#define FT_WIFI_SLE1_STR_W			70
#define FT_WIFI_SLE1_STR_H			30
#define FT_WIFI_SLE1_STR_X			40
#define FT_WIFI_SLE1_STR_Y			170
#define FT_WIFI_SLE2_STR_W			70
#define FT_WIFI_SLE2_STR_H			30
#define FT_WIFI_SLE2_STR_X			130
#define FT_WIFI_SLE2_STR_Y			170

#define FT_WIFI_RET_STR_W			120
#define FT_WIFI_RET_STR_H			60
#define FT_WIFI_RET_STR_X			((LCD_WIDTH-FT_WIFI_RET_STR_W)/2)
#define FT_WIFI_RET_STR_Y			((LCD_HEIGHT-FT_WIFI_RET_STR_H)/2)
#define FT_WIFI_NOTIFY_W			200
#define FT_WIFI_NOTIFY_H			40
#define FT_WIFI_NOTIFY_X			((LCD_WIDTH-FT_WIFI_NOTIFY_W)/2)
#define FT_WIFI_NOTIFY_Y			100
			
#define FT_WIFI_TEST_TIMEROUT	30
		
static bool ft_wifi_check_ok = false;
static bool ft_wifi_checking = false;
static bool update_show_flag = false;

static void WifiTestTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_test_timer, WifiTestTimerOutCallBack, NULL);

static void FTMenuWifiDumpProc(void){}

const ft_menu_t FT_MENU_WIFI = 
{
	FT_WIFI,
	0,
	0,
	{
		{0x0000},
	},
	{
		FTMenuWifiDumpProc,
	},
	{	
		//page proc func
		FTMenuWifiDumpProc,
		FTMenuWifiDumpProc,
		FTMenuWifiDumpProc,
		FTMenuWifiDumpProc,
	},
};

static void FTMenuWifiSle1Hander(void)
{
	FTMainMenu15Proc();
}

static void FTMenuWifiSle2Hander(void)
{
	ExitFTMenuWifi();
}

static void FTMenuWifiStopTest(void)
{
	ft_wifi_checking = false;
	k_timer_stop(&wifi_test_timer);
	FTStopWifi();
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuWifiStartTest(void)
{
	ft_wifi_checking = true;
	FTStartWifi();
	k_timer_start(&wifi_test_timer, K_SECONDS(FT_WIFI_TEST_TIMEROUT), K_NO_WAIT);
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void WifiTestTimerOutCallBack(struct k_timer *timer_id)
{
	FTMenuWifiStopTest();
}

static void FTMenuWifiUpdate(void)
{
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
							  };

	if(ft_wifi_checking)
	{
		uint8_t tmpbuf[512] = {0};
		
		if(!update_show_flag)
		{
			update_show_flag = true;
			LCD_Fill(FT_WIFI_NOTIFY_X, FT_WIFI_NOTIFY_Y, FT_WIFI_NOTIFY_W, FT_WIFI_NOTIFY_H, BLACK);
			LCD_SetFontSize(FONT_SIZE_20);
		}

		LCD_Fill((LCD_WIDTH-160)/2, 60, 160, 100, BLACK);
		mmi_asc_to_ucs2(tmpbuf, wifi_test_info);
		LCD_ShowUniStringInRect((LCD_WIDTH-160)/2, 60, 160, 100, (uint16_t*)tmpbuf);
	}
	else
	{
		update_show_flag = false;
		LCD_Set_BL_Mode(LCD_BL_AUTO);

		//pass or fail
		LCD_SetFontSize(FONT_SIZE_52);
		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);
		LCD_MeasureUniString(ret_str[ft_wifi_check_ok], &w, &h);
		LCD_ShowUniString(FT_WIFI_RET_STR_X+(FT_WIFI_RET_STR_W-w)/2, FT_WIFI_RET_STR_Y+(FT_WIFI_RET_STR_H-h)/2, ret_str[ft_wifi_check_ok]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();
	}
}

static void FTMenuWifiShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[8] = {0x0057,0x0049,0x0046,0x0049,0x6D4B,0x8BD5,0x0000};//WIFI测试
	uint16_t sle_str[2][5] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							  };
	uint16_t notify_str[9] = {0x6B63,0x5728,0x626B,0x63CF,0x4FE1,0x53F7,0x2026,0x0000};//正在扫描信号…

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_WIFI_TITLE_X+(FT_WIFI_TITLE_W-w)/2, FT_WIFI_TITLE_Y, title_str);
	LCD_MeasureUniString(notify_str, &w, &h);
	LCD_ShowUniString(FT_WIFI_NOTIFY_X+(FT_WIFI_NOTIFY_W-w)/2, FT_WIFI_NOTIFY_Y+(FT_WIFI_NOTIFY_H-h)/2, notify_str);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_WIFI_SLE1_STR_X+(FT_WIFI_SLE1_STR_W-w)/2;
	y = FT_WIFI_SLE1_STR_Y+(FT_WIFI_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_WIFI_SLE1_STR_X, FT_WIFI_SLE1_STR_Y, FT_WIFI_SLE1_STR_W, FT_WIFI_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_WIFI_SLE2_STR_X+(FT_WIFI_SLE2_STR_W-w)/2;
	y = FT_WIFI_SLE2_STR_Y+(FT_WIFI_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_WIFI_SLE2_STR_X, FT_WIFI_SLE2_STR_Y, FT_WIFI_SLE2_STR_W, FT_WIFI_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuWifiSle1Hander);
	SetRightKeyUpHandler(FTMenuWifiSle2Hander);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_WIFI_SLE1_STR_X, FT_WIFI_SLE1_STR_X+FT_WIFI_SLE1_STR_W, FT_WIFI_SLE1_STR_Y, FT_WIFI_SLE1_STR_Y+FT_WIFI_SLE1_STR_H, FTMenuWifiSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_WIFI_SLE2_STR_X, FT_WIFI_SLE2_STR_X+FT_WIFI_SLE2_STR_W, FT_WIFI_SLE2_STR_Y, FT_WIFI_SLE2_STR_Y+FT_WIFI_SLE2_STR_H, FTMenuWifiSle2Hander);
#endif		
}

void FTMenuWifiProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuWifiShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuWifiUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void FTWifiStatusUpdate(uint8_t node_count)
{
	static uint8_t count = 0;

	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_WIFI))
	{
		if(node_count > 0)
		{
			count++;
			if(count > 3)
			{
				count = 0;
				ft_wifi_check_ok = true;
				ft_menu_checked[ft_main_menu_index] = true;
				FTMenuWifiStopTest();
			}
		}

		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}
}

void ExitFTMenuWifi(void)
{
	ft_wifi_checking = false;
	ft_wifi_check_ok = false;
	k_timer_stop(&wifi_test_timer);
	ReturnFTMainMenu();
}

void EnterFTMenuWifi(void)
{
	ft_wifi_check_ok = false;
	update_show_flag = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_WIFI, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;

	FTMenuWifiStartTest();
}

