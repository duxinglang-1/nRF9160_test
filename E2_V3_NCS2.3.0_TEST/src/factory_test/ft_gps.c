/****************************************Copyright (c)************************************************
** File Name:			    ft_gps.c
** Descriptions:			Factory test gps module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <modem/lte_lc.h>
#include <string.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "gps.h"
#include "ft_main.h"
#include "ft_gps.h"
				
#define FT_GPS_TITLE_W				150
#define FT_GPS_TITLE_H				40
#define FT_GPS_TITLE_X				((LCD_WIDTH-FT_GPS_TITLE_W)/2)
#define FT_GPS_TITLE_Y				20

#define FT_GPS_MENU_STR_W			150
#define FT_GPS_MENU_STR_H			25
#define FT_GPS_MENU_STR_X			((LCD_WIDTH-FT_GPS_MENU_STR_W)/2)
#define FT_GPS_MENU_STR_Y			80
#define FT_GPS_MENU_STR_OFFSET_Y	5

#define FT_GPS_SLE1_STR_W			70
#define FT_GPS_SLE1_STR_H			30
#define FT_GPS_SLE1_STR_X			40
#define FT_GPS_SLE1_STR_Y			170
#define FT_GPS_SLE2_STR_W			70
#define FT_GPS_SLE2_STR_H			30
#define FT_GPS_SLE2_STR_X			130
#define FT_GPS_SLE2_STR_Y			170

#define FT_GPS_RET_STR_W			120
#define FT_GPS_RET_STR_H			60
#define FT_GPS_RET_STR_X			((LCD_WIDTH-FT_GPS_RET_STR_W)/2)
#define FT_GPS_RET_STR_Y			((LCD_HEIGHT-FT_GPS_RET_STR_H)/2)

#define FT_GPS_NOTIFY_W				200
#define FT_GPS_NOTIFY_H				40
#define FT_GPS_NOTIFY_X				((LCD_WIDTH-FT_GPS_NOTIFY_W)/2)
#define FT_GPS_NOTIFY_Y				100
				
#define FT_GPS_TEST_TIMEROUT	3*60

static bool ft_gps_stop_flag = false;
static bool ft_gps_check_ok = false;
static bool ft_gps_checking = false;
static bool ft_gps_start_flag = false;
static bool ft_gps_fixed = false;
static bool update_show_flag = false;

static int32_t ft_gps_lon=0,ft_gps_lat=0;

static void GPSTestTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(gps_test_timer, GPSTestTimerOutCallBack, NULL);

static void FTMenuGPSDumpProc(void){}

const ft_menu_t FT_MENU_GPS = 
{
	FT_GPS,
	0,
	0,
	{
		{
			{0x0000},
			FTMenuGPSDumpProc,
		},
	},
	{	
		//page proc func
		FTMenuGPSDumpProc,
		FTMenuGPSDumpProc,
		FTMenuGPSDumpProc,
		FTMenuGPSDumpProc,
	},
};

static void FTMenuGPSSle1Hander(void)
{
	ExitFTMenuGPS();
}

static void FTMenuGPSSle2Hander(void)
{
	ExitFTMenuGPS();
}

void FTMenuGPSInit(void)
{
	lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_GPS, LTE_LC_SYSTEM_MODE_PREFER_AUTO);
}

static void FTMenuGPSStopTest(void)
{
	ft_gps_checking = false;
	k_timer_stop(&gps_test_timer);
	FTStopGPS();
	SetModemTurnOff();
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuGPSStartTest(void)
{
	ft_gps_checking = true;
	FTMenuGPSInit();
	SetModemTurnOn();
	FTStartGPS();
	k_timer_start(&gps_test_timer, K_SECONDS(FT_GPS_TEST_TIMEROUT), K_NO_WAIT);
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void GPSTestTimerOutCallBack(struct k_timer *timer_id)
{
	ft_gps_stop_flag = true;
}

static void FTMenuGPSUpdate(void)
{
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
							  };

	if(ft_gps_checking)
	{
		uint8_t tmpbuf[512] = {0};
		
		if(!update_show_flag)
		{
			update_show_flag = true;
			LCD_Fill(FT_GPS_NOTIFY_X, FT_GPS_NOTIFY_Y, FT_GPS_NOTIFY_W, FT_GPS_NOTIFY_H, BLACK);
			LCD_SetFontSize(FONT_SIZE_20);
		}

		LCD_Fill((LCD_WIDTH-180)/2, 60, 180, 100, BLACK);
		mmi_asc_to_ucs2(tmpbuf, gps_test_info);
		LCD_ShowUniStringInRect((LCD_WIDTH-180)/2, 60, 180, 100, (uint16_t*)tmpbuf);
	}
	else
	{
		update_show_flag = false;
		
		LCD_Set_BL_Mode(LCD_BL_AUTO);

		//pass or fail
		LCD_SetFontSize(FONT_SIZE_52);
		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);
		LCD_MeasureUniString(ret_str[ft_gps_check_ok], &w, &h);
		LCD_ShowUniString(FT_GPS_RET_STR_X+(FT_GPS_RET_STR_W-w)/2, FT_GPS_RET_STR_Y+(FT_GPS_RET_STR_H-h)/2, ret_str[ft_gps_check_ok]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();

		if(ft_gps_check_ok)
			ft_results.gps_ret = 1;
		else
			ft_results.gps_ret = 2;
		
		SaveFactoryTestResults(ft_results);
	}
}

static void FTMenuGPSShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[8] = {0x0047,0x0050,0x0053,0x6D4B,0x8BD5,0x0000};//GPS²âÊÔ
	uint16_t sle_str[2][5] = {
								{0x5B8C,0x6210,0x0000},//Íê³É
								{0x9000,0x51FA,0x0000},//ÍË³ö
							  };
	uint16_t notify_str[10] = {0x641C,0x5BFB,0x536B,0x661F,0x4FE1,0x53F7,0x2026,0x0000};//ËÑÑ°ÎÀÐÇÐÅºÅ¡­

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_GPS_TITLE_X+(FT_GPS_TITLE_W-w)/2, FT_GPS_TITLE_Y, title_str);
	LCD_MeasureUniString(notify_str, &w, &h);
	LCD_ShowUniString(FT_GPS_NOTIFY_X+(FT_GPS_NOTIFY_W-w)/2, FT_GPS_NOTIFY_Y+(FT_GPS_NOTIFY_H-h)/2, notify_str);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_GPS_SLE1_STR_X+(FT_GPS_SLE1_STR_W-w)/2;
	y = FT_GPS_SLE1_STR_Y+(FT_GPS_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_GPS_SLE1_STR_X, FT_GPS_SLE1_STR_Y, FT_GPS_SLE1_STR_W, FT_GPS_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_GPS_SLE2_STR_X+(FT_GPS_SLE2_STR_W-w)/2;
	y = FT_GPS_SLE2_STR_Y+(FT_GPS_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_GPS_SLE2_STR_X, FT_GPS_SLE2_STR_Y, FT_GPS_SLE2_STR_W, FT_GPS_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuGPSSle1Hander);
	SetRightKeyUpHandler(FTMenuGPSSle2Hander);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_GPS_SLE1_STR_X, FT_GPS_SLE1_STR_X+FT_GPS_SLE1_STR_W, FT_GPS_SLE1_STR_Y, FT_GPS_SLE1_STR_Y+FT_GPS_SLE1_STR_H, FTMenuGPSSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_GPS_SLE2_STR_X, FT_GPS_SLE2_STR_X+FT_GPS_SLE2_STR_W, FT_GPS_SLE2_STR_Y, FT_GPS_SLE2_STR_Y+FT_GPS_SLE2_STR_H, FTMenuGPSSle2Hander);
#endif		
}

void FTMenuGPSProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuGPSShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuGPSUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}

	if(ft_gps_stop_flag)
	{
		FTMenuGPSStopTest();
		ft_gps_stop_flag = false;
	}
}

void FTGPSStartFail(void)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_GPS))
		FTMenuGPSStopTest();
}

bool IsFTGPSTesting(void)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_GPS))
		return true;
	else
		return false;
}

void FTGPSStatusUpdate(bool flag)
{
	static uint8_t count = 0;

	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_GPS))
	{
		ft_gps_fixed = flag;

		if(flag)
		{
			count++;
			if(count > 3)
			{
				count = 0;
				ft_gps_check_ok = true;
				ft_menu_checked[ft_main_menu_index] = true;
				FTMenuGPSStopTest();
			}
		}

		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}		
}

void ExitFTMenuGPS(void)
{
	ft_gps_checking = false;
	ft_gps_check_ok = false;
	k_timer_stop(&gps_test_timer);
	FTStopGPS();
	SetModemTurnOff();
	ReturnFTMainMenu();
}

void EnterFTMenuGPS(void)
{
	ft_gps_check_ok = false;
	ft_gps_fixed = false;
	ft_gps_lon=0;
	ft_gps_lat=0;
	update_show_flag = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_GPS, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;

	FTMenuGPSStartTest();
}

