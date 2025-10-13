/****************************************Copyright (c)************************************************
** File Name:			    ft_aging.c
** Descriptions:			Aging test source file
** Created By:				xie biao
** Created Date:			2023-12-12
** Modified Date:      		2023-12-12 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif/*CONFIG_WIFI_SUPPORT*/
#include "max20353.h"
#include "nb.h"
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "logger.h"
#include "ft_main.h"
#include "ft_aging.h"

#define FT_AGING_TITLE_W				100
#define FT_AGING_TITLE_H				40
#define FT_AGING_TITLE_X				((LCD_WIDTH-FT_AGING_TITLE_W)/2)
#define FT_AGING_TITLE_Y				20

#define FT_AGING_MENU_STR_W				150
#define FT_AGING_MENU_STR_H				30
#define FT_AGING_MENU_STR_X				((LCD_WIDTH-FT_AGING_MENU_STR_W)/2)
#define FT_AGING_MENU_STR_Y				80
#define FT_AGING_MENU_STR_OFFSET_Y		5

#define FT_AGING_SLE1_STR_W				70
#define FT_AGING_SLE1_STR_H				30
#define FT_AGING_SLE1_STR_X				40
#define FT_AGING_SLE1_STR_Y				170
#define FT_AGING_SLE2_STR_W				70
#define FT_AGING_SLE2_STR_H				30
#define FT_AGING_SLE2_STR_X				130
#define FT_AGING_SLE2_STR_Y				170

#define FT_AGING_STATUS_STR_W			LCD_WIDTH
#define FT_AGING_STATUS_STR_H			40
#define FT_AGING_STATUS_STR_X			((LCD_WIDTH-FT_AGING_STATUS_STR_W)/2)
#define FT_AGING_STATUS_STR_Y			100	

static bool ft_aging_change_flag = false;
static bool ft_aging_stop_flag = false;
static bool ft_aging_is_running = false;
static bool ft_aging_update_show_flag = false;

static FT_AGING_STATUS aging_status = AGING_BEGIN;

static void FTaAgingDumpProc(void){}

const ft_menu_t FT_MENU_AGING = 
{
	FT_AGING,
	0,
	2,
	{
		//按SOS键启动测试
		{
			{0x6309,0x0053,0x004F,0x0053,0x952E,0x542F,0x52A8,0x6D4B,0x8BD5,0x0000},
			FTaAgingDumpProc,
		},
		//测试时按SOS键停止测试
		{
			{0x6D4B,0x8BD5,0x65F6,0x6309,0x0053,0x004F,0x0053,0x952E,0x505C,0x6B62,0x6D4B,0x8BD5,0x0000},
			FTaAgingDumpProc,
		},
	},
	{	
		//page proc func
		FTaAgingDumpProc,
		FTaAgingDumpProc,
		FTaAgingDumpProc,
		FTaAgingDumpProc,
	},
};

static void FtAgingChangeCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(ft_aging_change_timer, FtAgingChangeCallBack, NULL);
static void FtAgingStopCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(ft_aging_stop_timer, FtAgingStopCallBack, NULL);

static void AgingStartTest(void);
static void AgingTestPreExit(void);
static void AgingTestNextExit(void);

bool IsFTPPGAging(void)
{
	if((screen_id == SCREEN_ID_AGING_TEST)&&(aging_status == AGING_PPG))
		return true;
	else
		return false;
}

bool IsFTTempAging(void)
{
	if((screen_id == SCREEN_ID_AGING_TEST)&&(aging_status == AGING_TEMP))
		return true;
	else
		return false;
}

static void FtAgingChangeCallBack(struct k_timer *timer_id)
{
	ft_aging_change_flag = true;

	aging_status++;
	if(aging_status == AGING_MAX)
		aging_status = AGING_BEGIN;
}

static void FtAgingStopCallBack(struct k_timer *timer_id)
{
	ft_aging_stop_flag = true;
}

static void AgingStopTest(void)
{
	k_timer_stop(&ft_aging_stop_timer);
	k_timer_stop(&ft_aging_change_timer);
	ft_aging_is_running = false;
	ft_aging_update_show_flag = false;

	switch(aging_status)
	{
	case AGING_PPG:
		FTStopPPG();
		break;
	case AGING_WIFI:
		FTStopWifi();
		break;
	case AGING_GPS:
		FTStopGPS();
		break;
	case AGING_VIB:
		vibrate_off();
		break;
	case AGING_TEMP:
		FTStopTemp();
		break;
	}
	scr_msg[SCREEN_ID_AGING_TEST].act = SCREEN_ACTION_UPDATE;
}

static void AgingStartTest(void)
{
	ft_aging_is_running = true;
	ft_aging_change_flag = true;
	k_timer_start(&ft_aging_stop_timer, FT_AGING_CONTINUE_TIME, K_NO_WAIT);
}

static void AgingTestPreExit(void)
{
	AgingStopTest();
	EnterFTAssemResultsScreen();
}

static void AgingTestNextExit(void)
{
	AgingStopTest();
	EnterPoweroffScreen();
}

static void AgingTestUpdate(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x9707,0x52A8,0x6D4B,0x8BD5,0x0000};//震动测试
	uint16_t status_str[2][5] = {
									{0x6D4B,0x8BD5,0x505C,0x6B62,0x0000},//测试停止
									{0x6D4B,0x8BD5,0x542F,0x52A8,0x0000},//测试启动
								};
	uint16_t ppg_str[10] = {0x0050,0x0050,0x0047,0x8001,0x5316,0x6D4B,0x8BD5,0x4E2D,0x2026,0x0000};//PPG老化测试中…
	uint16_t wifi_str[11] = {0x0057,0x0069,0x0046,0x0069,0x8001,0x5316,0x6D4B,0x8BD5,0x4E2D,0x2026,0x0000};//WiFi老化测试中…
	uint16_t gps_str[10] = {0x0047,0x0050,0x0053,0x8001,0x5316,0x6D4B,0x8BD5,0x4E2D,0x2026,0x0000};//GPS老化测试中…
	uint16_t vib_str[9] = {0x9707,0x52A8,0x8001,0x5316,0x6D4B,0x8BD5,0x4E2D,0x2026,0x0000};//震动老化测试中…
	uint16_t temp_str[9] = {0x6E29,0x5EA6,0x8001,0x5316,0x6D4B,0x8BD5,0x4E2D,0x2026,0x0000};//温度老化测试中…
	uint16_t show_str[16] = {0};
	
	if(ft_aging_is_running)
	{	
		if(!ft_aging_update_show_flag)
		{
			ft_aging_update_show_flag = true;
			
			LCD_SetFontSize(FONT_SIZE_28);

			LCD_Fill(FT_AGING_MENU_STR_X, FT_AGING_MENU_STR_Y, FT_AGING_MENU_STR_W, 2*(FT_AGING_MENU_STR_H+FT_AGING_MENU_STR_OFFSET_Y), BLACK);
			
			ClearAllKeyHandler();
			SetLeftKeyUpHandler(AgingTestNextExit);
			SetRightKeyUpHandler(AgingStopTest);
		}

		switch(aging_status)
		{
		case AGING_PPG:
			mmi_ucs2cpy((uint8_t*)show_str, (uint8_t*)ppg_str);
			break;
		case AGING_WIFI:
			mmi_ucs2cpy((uint8_t*)show_str, (uint8_t*)wifi_str);
			break;
		case AGING_GPS:
			mmi_ucs2cpy((uint8_t*)show_str, (uint8_t*)gps_str);
			break;
		case AGING_VIB:
			mmi_ucs2cpy((uint8_t*)show_str, (uint8_t*)vib_str);
			break;
		case AGING_TEMP:
			mmi_ucs2cpy((uint8_t*)show_str, (uint8_t*)temp_str);
			break;
		}

		LCD_Fill(FT_AGING_STATUS_STR_X, FT_AGING_STATUS_STR_Y, FT_AGING_STATUS_STR_W, FT_AGING_STATUS_STR_H, BLACK);
		LCD_SetFontSize(FONT_SIZE_28);
		LCD_MeasureUniString(show_str, &w, &h);
		LCD_ShowUniString(FT_AGING_STATUS_STR_X+(FT_AGING_STATUS_STR_W-w)/2, FT_AGING_STATUS_STR_Y+(FT_AGING_STATUS_STR_H-h)/2, show_str);
	}
	else
	{
		ft_aging_update_show_flag = false;
		
		LCD_SetFontSize(FONT_SIZE_28);

		LCD_Fill(FT_AGING_STATUS_STR_X, FT_AGING_STATUS_STR_Y, FT_AGING_STATUS_STR_W, FT_AGING_STATUS_STR_H, BLACK);
		LCD_MeasureUniString(status_str[0], &w, &h);
		LCD_ShowUniString(FT_AGING_STATUS_STR_X+(FT_AGING_STATUS_STR_W-w)/2, FT_AGING_STATUS_STR_Y+(FT_AGING_STATUS_STR_H-h)/2, status_str[0]);

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(AgingTestNextExit);
		SetRightKeyUpHandler(AgingStartTest);
	}
}

static void AgingTestShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x8001,0x5316,0x6D4B,0x8BD5,0x0000};//老化测试

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_AGING_TITLE_X+(FT_AGING_TITLE_W-w)/2, FT_AGING_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_20);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.item[i].name, &w, &h);
		LCD_ShowUniString(FT_AGING_MENU_STR_X+(FT_AGING_MENU_STR_W-w)/2, FT_AGING_MENU_STR_Y+(FT_AGING_MENU_STR_H-h)/2+i*(FT_AGING_MENU_STR_H+FT_AGING_MENU_STR_OFFSET_Y), ft_menu.item[i].name);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_AGING_MENU_STR_X, 
									FT_AGING_MENU_STR_X+FT_AGING_MENU_STR_W, 
									FT_AGING_MENU_STR_Y+i*(FT_AGING_MENU_STR_H+FT_AGING_MENU_STR_OFFSET_Y), 
									FT_AGING_MENU_STR_Y+i*(FT_AGING_MENU_STR_H+FT_AGING_MENU_STR_OFFSET_Y)+FT_AGING_MENU_STR_H, 
									ft_menu.item[i].sel_handler);
	#endif
	}

#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, AgingTestNextExit);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, AgingTestPreExit);	
#endif	

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(AgingTestNextExit);
	SetRightKeyUpHandler(AgingStartTest);
}

void FTAgingTestProcess(void)
{
	if(scr_msg[SCREEN_ID_AGING_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_AGING_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_AGING_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_AGING_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_AGING_TEST].status = SCREEN_STATUS_CREATED;
			AgingTestShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			AgingTestUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_AGING_TEST].act = SCREEN_ACTION_NO;
	}

	if(ft_aging_change_flag)
	{
		switch(aging_status)
		{
		#ifdef CONFIG_PPG_SUPPORT	
		case AGING_PPG:
			FTStopTemp();
			FTStartPPG();
			k_timer_start(&ft_aging_change_timer, K_SECONDS(60), K_NO_WAIT);
			break;
		#endif
	
		#ifdef CONFIG_WIFI_SUPPORT		
		case AGING_WIFI:
			FTStopPPG();
			FTStartWifi();
			k_timer_start(&ft_aging_change_timer, K_SECONDS(60), K_NO_WAIT);
			break;
		#endif
	
		case AGING_GPS:
			FTStopWifi();
			FTMenuGPSInit();
			SetModemTurnOn();
			FTStartGPS();
			k_timer_start(&ft_aging_change_timer, K_SECONDS(60), K_NO_WAIT);
			break;
	
		case AGING_VIB:
			FTStopGPS();
			SetModemTurnOff();
			vibrate_on(VIB_RHYTHMIC, 1000, 1000);
			k_timer_start(&ft_aging_change_timer, K_SECONDS(60), K_NO_WAIT);
			break;
		
		#ifdef CONFIG_TEMP_SUPPORT	
		case AGING_TEMP:
			vibrate_off();
			FTStartTemp();
			k_timer_start(&ft_aging_change_timer, K_SECONDS(60), K_NO_WAIT);
			break;
		#endif	
		}

		scr_msg[SCREEN_ID_AGING_TEST].act = SCREEN_ACTION_UPDATE;
		ft_aging_change_flag = false;
	}

	if(ft_aging_stop_flag)
	{
		AgingStopTest();
		ft_aging_stop_flag = false;
	}
}

void EnterFTAgingTest(void)
{
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStop();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking())
		MenuStopTemp();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(PPGIsWorking())
		MenuStopPPG();
#endif
#ifdef CONFIG_WIFI_SUPPORT
	if(wifi_is_working())
		MenuStopWifi();
#endif
	if(gps_is_working())
		MenuStopGPS();
	
	vibrate_off();

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	aging_status = AGING_BEGIN;
	memcpy(&ft_menu, &FT_MENU_AGING, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_AGING_TEST;	
	scr_msg[SCREEN_ID_AGING_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_AGING_TEST].status = SCREEN_STATUS_CREATING;
}

