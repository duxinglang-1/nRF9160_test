/****************************************Copyright (c)************************************************
** File Name:			    ft_main.c
** Descriptions:			Factory test main interface source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
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
#include "nb.h"
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "logger.h"
#include "ft_main.h"
#include "ft_gpio.h"
#include "ft_aging.h"

#define FT_MAIN_MENU_MAX_PER_PG	4
#define FT_SUB_MENU_MAX_PER_PG	3

#define FT_MENU_BG_W			169
#define FT_MENU_BG_H			38
#define FT_MENU_BG_X			((LCD_WIDTH-FT_MENU_BG_W)/2)
#define FT_MENU_BG_Y			38
#define FT_MENU_BG_OFFSET_Y		4
#define SETTINGS_MENU_STR_OFFSET_X	5			
#define SETTINGS_MENU_STR_OFFSET_Y	8

#define FT_MENU_CHECKED_W		20
#define FT_MENU_CHECKED_H		20
#define FT_MENU_CHECKED_X		(FT_MENU_BG_X+140)
#define FT_MENU_CHECKED_Y		(FT_MENU_BG_Y+9)

#define FT_MENU_STR_W			150
#define FT_MENU_STR_H			30
#define FT_MENU_STR_X			(FT_MENU_BG_X+5)
#define FT_MENU_STR_H			30
#define FT_MENU_STR_OFFSET_Y	8

static bool ft_running_flag = false;
static bool ft_main_redaw_flag = false;

uint8_t ft_main_menu_index = 0;
uint8_t g_ft_status = FT_STATUS_SMT;

bool ft_menu_checked[FT_MENU_MAX_COUNT] = {false};
ft_menu_t ft_menu = {0};
ft_smt_results_t ft_smt_results = {0};	//0:has not be tested; 1:test passed; 2:test failed;
ft_assem_results_t ft_assem_results = {0};	//0:has not be tested; 1:test passed; 2:test failed;

static void FactoryTestNextExit(void);
static void FactoryTestPreExit(void);

void FTMainDumpProc(void)
{
}

void FTMainMenuCurProc(void)
{
	ft_main_menu_index = FT_CURRENT;
	EnterFTMenuCur();
}

void FTMainMenuKeyProc(void)
{
	ft_main_menu_index = FT_KEY;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuKey();
}

void FTMainMenuLcdProc(void)
{
	ft_main_menu_index = FT_LCD;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuLcd();
}

void FTMainMenuTouchProc(void)
{
	ft_main_menu_index = FT_TOUCH;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuTouch();
}

void FTMainMenuTempProc(void)
{
	ft_main_menu_index = FT_TEMP;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuTemp();
}

void FTMainMenuWristProc(void)
{
#ifdef CONFIG_WRIST_CHECK_SUPPORT
	ft_main_menu_index = FT_WRIST;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuWrist();
#endif	
}

void FTMainMenuIMUProc(void)
{
	ft_main_menu_index = FT_IMU;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuIMU();
}

void FTMainMenuFlashProc(void)
{
	ft_main_menu_index = FT_FLASH;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuFlash();
}

void FTMainMenuSIMProc(void)
{
	ft_main_menu_index = FT_SIM;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuSIM();
}

void FTMainMenuBleProc(void)
{
	ft_main_menu_index = FT_BLE;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuBle();
}

void FTMainMenuPPGProc(void)
{
	ft_main_menu_index = FT_PPG;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuPPG();
}

void FTMainMenuPMUProc(void)
{
	ft_main_menu_index = FT_PMU;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuPMU();
}

void FTMainMenuVibrateProc(void)
{
	ft_main_menu_index = FT_VIBRATE;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuVibrate();
}

void FTMainMenuWifiProc(void)
{
#ifdef CONFIG_WIFI_SUPPORT
	ft_main_menu_index = FT_WIFI;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuWifi();	
#endif
}

void FTMainMenuNetProc(void)
{
	ft_main_menu_index = FT_NET;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuNet();
}

void FTMainMenuGPSProc(void)
{
	ft_main_menu_index = FT_GPS;
	if(g_ft_status == FT_STATUS_ASSEM)	//组装测试没有电流测试，按键测试排在第一位，序号需要从0开始
		ft_main_menu_index--;
	EnterFTMenuGPS();
}

static void FTMainMenuProcess(void)
{
	ft_menu.item[ft_menu.index].sel_handler();
}

void FTMainMenuPgUpProc(void)
{
	if(ft_menu.index < FT_MAIN_MENU_MAX_PER_PG*((ft_menu.count-1)/FT_MAIN_MENU_MAX_PER_PG))
	{
		ft_menu.index += FT_MAIN_MENU_MAX_PER_PG;
		if(ft_menu.index > (ft_menu.count-1))
			ft_menu.index = FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG);
		if(screen_id == SCREEN_ID_FACTORY_TEST)
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void FTMainMenuPgDownProc(void)
{
	if(ft_menu.index >= FT_MAIN_MENU_MAX_PER_PG)
	{
		ft_menu.index -= FT_MAIN_MENU_MAX_PER_PG;
		if(screen_id == SCREEN_ID_FACTORY_TEST)
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

const ft_menu_t FT_SMT_MENU_MAIN = 
{
	FT_MAIN,
	0,
	15,
	{
		//电流测试
		{
			{0x7535,0x6D41,0x6D4B,0x8BD5,0x0000},
			FTMainMenuCurProc,
		},
		//按键测试
		{
			{0x6309,0x952E,0x6D4B,0x8BD5,0x0000},
			FTMainMenuKeyProc,
		},
		//屏幕测试
		{
			{0x5C4F,0x5E55,0x6D4B,0x8BD5,0x0000},
			FTMainMenuLcdProc,
		},
		//触摸测试
		{
			{0x89E6,0x6478,0x6D4B,0x8BD5,0x0000},
			FTMainMenuTouchProc,
		},
		//温度测试
		{
			{0x6E29,0x5EA6,0x6D4B,0x8BD5,0x0000},
			FTMainMenuTempProc,
		},
		//脱腕测试
		//{
		//	{0x8131,0x8155,0x6D4B,0x8BD5,0x0000},
		//	FTMainMenuWristProc,
		//},
		//IMU测试
		{
			{0x0049,0x004D,0x0055,0x6D4B,0x8BD5,0x0000},
			FTMainMenuIMUProc,
		},
		//FLASH测试
		{
			{0x0046,0x004C,0x0041,0x0053,0x0048,0x6D4B,0x8BD5,0x0000},
			FTMainMenuFlashProc,
		},
		//SIM卡测试
		{
			{0x0053,0x0049,0x004D,0x5361,0x6D4B,0x8BD5,0x0000},
			FTMainMenuSIMProc,
		},
		//BLE测试
		{
			{0x0042,0x004C,0x0045,0x6D4B,0x8BD5,0x0000},
			FTMainMenuBleProc,
		},
		//PPG测试
		{
			{0x0050,0x0050,0x0047,0x6D4B,0x8BD5,0x0000},
			FTMainMenuPPGProc,
		},
		//充电测试
		{
			{0x5145,0x7535,0x6D4B,0x8BD5,0x0000},
			FTMainMenuPMUProc,
		},
		//震动测试
		{
			{0x9707,0x52A8,0x6D4B,0x8BD5,0x0000},
			FTMainMenuVibrateProc,
		},
		//WiFi测试
		{
			{0x0057,0x0069,0x0046,0x0069,0x6D4B,0x8BD5,0x0000},
			FTMainMenuWifiProc,
		},
		//网络测试
		{
			{0x7F51,0x7EDC,0x6D4B,0x8BD5,0x0000},
			FTMainMenuNetProc,
		},
		//GPS测试
		{
			{0x0047,0x0050,0x0053,0x6D4B,0x8BD5,0x0000},
			FTMainMenuGPSProc,
		},
	},	
	{	
		//page proc func
		FTMainMenuPgUpProc,
		FTMainMenuPgDownProc,
		FTMainDumpProc,
		FTMainDumpProc,
	},
};

const ft_menu_t FT_ASSEM_MENU_MAIN = 
{
	FT_MAIN,
	0,
	14,
	{
		//按键测试
		{
			{0x6309,0x952E,0x6D4B,0x8BD5,0x0000},
			FTMainMenuKeyProc,
		},
		//屏幕测试
		{
			{0x5C4F,0x5E55,0x6D4B,0x8BD5,0x0000},
			FTMainMenuLcdProc,
		},
		//触摸测试
		{
			{0x89E6,0x6478,0x6D4B,0x8BD5,0x0000},
			FTMainMenuTouchProc,
		},
		//温度测试
		{
			{0x6E29,0x5EA6,0x6D4B,0x8BD5,0x0000},
			FTMainMenuTempProc,
		},
		//IMU测试
		{
			{0x0049,0x004D,0x0055,0x6D4B,0x8BD5,0x0000},
			FTMainMenuIMUProc,
		},
		//FLASH测试
		{
			{0x0046,0x004C,0x0041,0x0053,0x0048,0x6D4B,0x8BD5,0x0000},
			FTMainMenuFlashProc,
		},
		//SIM卡测试
		{
			{0x0053,0x0049,0x004D,0x5361,0x6D4B,0x8BD5,0x0000},
			FTMainMenuSIMProc,
		},
		//BLE测试
		{
			{0x0042,0x004C,0x0045,0x6D4B,0x8BD5,0x0000},
			FTMainMenuBleProc,
		},
		//PPG测试
		{
			{0x0050,0x0050,0x0047,0x6D4B,0x8BD5,0x0000},
			FTMainMenuPPGProc,
		},
		//充电测试
		{
			{0x5145,0x7535,0x6D4B,0x8BD5,0x0000},
			FTMainMenuPMUProc,
		},
		//震动测试
		{
			{0x9707,0x52A8,0x6D4B,0x8BD5,0x0000},
			FTMainMenuVibrateProc,
		},
		//WiFi测试
		{
			{0x0057,0x0069,0x0046,0x0069,0x6D4B,0x8BD5,0x0000},
			FTMainMenuWifiProc,
		},
		//网络测试
		{
			{0x7F51,0x7EDC,0x6D4B,0x8BD5,0x0000},
			FTMainMenuNetProc,
		},
		//GPS测试
		{
			{0x0047,0x0050,0x0053,0x6D4B,0x8BD5,0x0000},
			FTMainMenuGPSProc,
		},
	},	
	{	
		//page proc func
		FTMainMenuPgUpProc,
		FTMainMenuPgDownProc,
		FTMainDumpProc,
		FTMainDumpProc,
	},
};

static void FactoryTestMainUpdate(void)
{
	uint8_t i,count;
	uint16_t x,y,w,h;
	uint16_t bg_clor = 0x2124;
	uint16_t green_clor = 0x07e0;
	uint16_t title_smt[6] = {0x0053,0x004D,0x0054,0x6D4B,0x8BD5,0x0000};//SMT测试
	uint16_t title_assem[5] = {0x7EC4,0x88C5,0x6D4B,0x8BD5,0x0000};//组装测试
	uint32_t img_addr[2] = {IMG_SELECT_ICON_NO_ADDR,IMG_SELECT_ICON_YES_ADDR};

	LCD_Clear(BLACK);
	LCD_SetFontSize(FONT_SIZE_20);
	if(g_ft_status == FT_STATUS_SMT)
	{		
		LCD_MeasureUniString(title_smt, &w, &h);
		LCD_ShowUniString((LCD_WIDTH-w)/2, 10, title_smt);
	}
	else
	{
		LCD_MeasureUniString(title_assem, &w, &h);
		LCD_ShowUniString((LCD_WIDTH-w)/2, 10, title_assem);
	}
	
	LCD_SetFontBgColor(bg_clor);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	for(i=0;i<FT_MAIN_MENU_MAX_PER_PG;i++)
	{
		if((FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG) + i) >= ft_menu.count)
			break;
		LCD_ShowImg_From_Flash(FT_MENU_BG_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);
		LCD_SetFontColor(WHITE);
	
		LCD_ShowUniString(FT_MENU_STR_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_STR_OFFSET_Y, ft_menu.item[i+FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG)].name);
		LCD_ShowImg_From_Flash(FT_MENU_CHECKED_X, FT_MENU_CHECKED_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), img_addr[ft_menu_checked[i+FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG)]]);
	
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_MENU_BG_X, 
									FT_MENU_BG_X+FT_MENU_BG_W, 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_BG_H, 
									ft_menu.item[i+FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG)].sel_handler);
	#endif
	}	

#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[0]);
	register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[1]);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestNextExit);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestPreExit);	
#endif		

	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();
}


static void FactoryTestMainShow(void)
{
	uint8_t;
	uint16_t i,x,y,w,h;
	uint16_t bg_clor = 0x2124;
	uint16_t green_clor = 0x07e0;
	uint16_t title_smt[6] = {0x0053,0x004D,0x0054,0x6D4B,0x8BD5,0x0000};//SMT测试
	uint16_t title_assem[5] = {0x7EC4,0x88C5,0x6D4B,0x8BD5,0x0000};//组装测试
	uint32_t img_addr[2] = {IMG_SELECT_ICON_NO_ADDR,IMG_SELECT_ICON_YES_ADDR};
	
	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	LCD_SetFontSize(FONT_SIZE_20);
	if(g_ft_status == FT_STATUS_SMT)
	{		
		LCD_MeasureUniString(title_smt, &w, &h);
		LCD_ShowUniString((LCD_WIDTH-w)/2, 10, title_smt);
	}
	else
	{
		LCD_MeasureUniString(title_assem, &w, &h);
		LCD_ShowUniString((LCD_WIDTH-w)/2, 10, title_assem);
	}
	
	LCD_SetFontBgColor(bg_clor);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	for(i=0;i<FT_MAIN_MENU_MAX_PER_PG;i++)
	{
		if((FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG) + i) >= ft_menu.count)
			break;
		LCD_ShowImg_From_Flash(FT_MENU_BG_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);
		LCD_SetFontColor(WHITE);
		LCD_ShowUniString(FT_MENU_STR_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_STR_OFFSET_Y, ft_menu.item[i+FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG)].name);
		LCD_ShowImg_From_Flash(FT_MENU_CHECKED_X, FT_MENU_CHECKED_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), img_addr[ft_menu_checked[i+FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG)]]);
		
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_MENU_BG_X, 
									FT_MENU_BG_X+FT_MENU_BG_W, 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_BG_H, 
									ft_menu.item[i+FT_MAIN_MENU_MAX_PER_PG*(ft_menu.index/FT_MAIN_MENU_MAX_PER_PG)].sel_handler);
	#endif
	}

	SetLeftKeyUpHandler(FactoryTestNextExit);
	SetRightKeyUpHandler(FTMainMenuProcess);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[0]);
	register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[1]);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestNextExit);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestPreExit);	
#endif		

	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();

	//xb add 2023-03-15 Turn off the modem after entering the ft menu to prevent jamming.
	FTPreReadyNet();
}

static void FactoryTestMainProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FactoryTestMainShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FactoryTestMainUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

static void FactoryTestPreExit(void)
{
	ft_running_flag = false;
	
	SetModemTurnOn();
	EnterSettings();
}

static void FactoryTestNextExit(void)
{
	ft_running_flag = false;
	
	SetModemTurnOn();
	EnterFTSmtResultsScreen();
}

void EnterFactoryTestScreen(void)
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

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST;	
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

void ReturnFTMainMenu(void)
{
	if(FactorySmtTestFinished())
	{
		g_ft_status = FT_STATUS_ASSEM;
		SaveFtStatusToInnerFlash(g_ft_status);
	}

	if(g_ft_status == FT_STATUS_SMT)
	{
		memcpy(&ft_menu, &FT_SMT_MENU_MAIN, sizeof(ft_menu_t));
	}
	else
	{
		memcpy(&ft_menu, &FT_ASSEM_MENU_MAIN, sizeof(ft_menu_t));
	}
	
	ft_menu.index = ft_main_menu_index;
	FactoryTestMainShow();
}

void EnterFactoryTest(void)
{
	uint8_t i,*p_ret;
	
	ft_running_flag = true;
	if(g_ft_status == FT_STATUS_SMT)
	{
		memcpy(&ft_menu, &FT_SMT_MENU_MAIN, sizeof(ft_menu_t));
		p_ret = (uint8_t*)&ft_smt_results;
	}
	else
	{
		memcpy(&ft_menu, &FT_ASSEM_MENU_MAIN, sizeof(ft_menu_t));
		p_ret = (uint8_t*)&ft_assem_results;
	}
	
	for(i=0;i<ft_menu.count;i++)
	{
		if(*(p_ret++) == 1)
			ft_menu_checked[i] = true;
		else
			ft_menu_checked[i] = false;
	}
	
	EnterFactoryTestScreen();
}

bool FactorySmtTestFinished(void)
{
	if((ft_smt_results.cur_ret == 1)
		&& (ft_smt_results.key_ret == 1)
		&& (ft_smt_results.lcd_ret == 1)
		&& (ft_smt_results.touch_ret == 1)
		&& (ft_smt_results.temp_ret == 1)
		&& (ft_smt_results.imu_ret == 1)
		&& (ft_smt_results.flash_ret == 1)
		&& (ft_smt_results.sim_ret == 1)
		&& (ft_smt_results.ble_ret == 1)
		&& (ft_smt_results.ppg_ret == 1)
		&& (ft_smt_results.pmu_ret == 1)
		&& (ft_smt_results.vib_ret == 1)
		&& (ft_smt_results.wifi_ret == 1)
		&& (ft_smt_results.gps_ret == 1)
		)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool FactoryTestActived(void)
{
	return ft_running_flag;
}

bool FactorySmtTestActived(void)
{
	if(ft_running_flag && (g_ft_status == FT_STATUS_SMT))
		return true;
	else
		return false;
}

bool FactoryAssemTestActived(void)
{
	if(ft_running_flag && (g_ft_status == FT_STATUS_ASSEM))
		return true;
	else
		return false;
}

void FactoryTestProccess(void)
{
	switch(ft_menu.id)
	{
	case FT_MAIN:
		FactoryTestMainProcess();
		break;
	case FT_CURRENT:
		FTMenuCurProcess();
		break;
	case FT_FLASH:
		FTMenuFlashProcess();
		break;
	case FT_LCD:
		FTMenuLcdProcess();
		break;
	case FT_KEY:
		FTMenuKeyProcess();
		break;
#ifdef CONFIG_WRIST_CHECK_SUPPORT		
	case FT_WRIST:
		FTMenuWristProcess();
		break;
#endif		
	case FT_VIBRATE:
		FTMenuVibrateProcess();
		break;
	case FT_NET:
		FTMenuNetProcess();
		break;
	case FT_AUDIO:
		break;
	case FT_BLE:
		FTMenuBleProcess();
		break;
	case FT_GPS:
		FTMenuGPSProcess();
		break;
	case FT_IMU:
		FTMenuIMUProcess();
		break;
	case FT_PMU:
		FTMenuPMUProcess();
		break;	
	case FT_PPG:
		FTMenuPPGProcess();
		break;
	case FT_SIM:
		FTMenuSIMProcess();
		break;
	case FT_TEMP:
		FTMenuTempProcess();
		break;
	case FT_TOUCH:
		FTMenuTouchProcess();
		break;
	#ifdef CONFIG_WIFI_SUPPORT		
	case FT_WIFI:
		FTMenuWifiProcess();
		break;
	#endif
	case FT_AGING:
		FTAgingTestProcess();
		break;
	}
}

void EnterFactoryTestResults(void)
{
	notify_infor infor = {0};
	uint16_t str_notify[LANGUAGE_MAX][35] = {
											#ifndef FW_FOR_CN
											  #ifdef LANGUAGE_EN_ENABLE
												{0x0049,0x0074,0x0020,0x0069,0x0073,0x0020,0x0062,0x0065,0x0069,0x006E,0x0067,0x0020,0x0064,0x0065,0x0076,0x0065,0x006C,0x006F,0x0070,0x0065,0x0064,0x002E,0x0000},//It is being developed.
											  #endif
											  #ifdef LANGUAGE_DE_ENABLE
												{0x0045,0x0073,0x0020,0x0077,0x0069,0x0072,0x0064,0x0020,0x0065,0x006E,0x0074,0x0077,0x0069,0x0063,0x006B,0x0065,0x006C,0x0074,0x002E,0x0000},//Es wird entwickelt.
											  #endif
											  #ifdef LANGUAGE_FR_ENABLE
												{0x0049,0x006C,0x0020,0x0065,0x0073,0x0074,0x0020,0x0065,0x006E,0x0020,0x0063,0x006F,0x0075,0x0072,0x0073,0x0020,0x0064,0x0065,0x0020,0x0064,0x00E9,0x0076,0x0065,0x006C,0x006F,0x0070,0x0070,0x0065,0x006D,0x0065,0x006E,0x0074,0x002E,0x0000},//Il est en cours de développement.
											  #endif
											  #ifdef LANGUAGE_IT_ENABLE
												{0x00C8,0x0020,0x0069,0x006E,0x0020,0x0066,0x0061,0x0073,0x0065,0x0020,0x0064,0x0069,0x0020,0x0073,0x0076,0x0069,0x006C,0x0075,0x0070,0x0070,0x006F,0x002E,0x0000},//? in fase di sviluppo.
											  #endif
											  #ifdef LANGUAGE_ES_ENABLE
												{0x0053,0x0065,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0064,0x0065,0x0073,0x0061,0x0072,0x0072,0x006F,0x006C,0x006C,0x0061,0x006E,0x0064,0x006F,0x002E,0x0000},//Se está desarrollando.
											  #endif
											  #ifdef LANGUAGE_PT_ENABLE
												{0x0045,0x0073,0x0074,0x00E1,0x0020,0x0073,0x0065,0x006E,0x0064,0x006F,0x0020,0x0064,0x0065,0x0073,0x0065,0x006E,0x0076,0x006F,0x006C,0x0076,0x0069,0x0064,0x006F,0x002E,0x0000},//Está sendo desenvolvido.
											  #endif
											  #ifdef LANGUAGE_PL_ENABLE
											  	{0x004A,0x0065,0x0073,0x0074,0x0020,0x0077,0x0020,0x0074,0x0072,0x0061,0x006B,0x0063,0x0069,0x0065,0x0020,0x006F,0x0070,0x0072,0x0061,0x0063,0x006F,0x0077,0x0079,0x0077,0x0061,0x006E,0x0069,0x0061,0x002E,0x0000},//Jest w trakcie opracowywania.
											  #endif
											  #ifdef LANGUAGE_SE_ENABLE
											  	{0x0044,0x0065,0x0074,0x0020,0x0068,0x00E5,0x006C,0x006C,0x0065,0x0072,0x0020,0x0070,0x00E5,0x0020,0x0061,0x0074,0x0074,0x0020,0x0075,0x0074,0x0076,0x0065,0x0063,0x006B,0x006C,0x0061,0x0073,0x002E,0x0000},//Det h?ller p? att utvecklas.
											  #endif
											  #ifdef LANGUAGE_JP_ENABLE
											  	{0x3053,0x306E,0x6A5F,0x80FD,0x306F,0x958B,0x767A,0x4E2D,0x3067,0x3059,0x3002,0x0000},//この機能は開発中です。
											  #endif
											  #ifdef LANGUAGE_KR_ENABLE
											  	{0xC774,0x0020,0xAE30,0xB2A5,0xC740,0x0020,0xAC1C,0xBC1C,0x0020,0xC911,0xC785,0xB2C8,0xB2E4,0x002E,0x0000},//? ??? ?? ????.
											  #endif
											  #ifdef LANGUAGE_RU_ENABLE
											  	{0x042D,0x0442,0x0430,0x0020,0x0444,0x0443,0x043D,0x043A,0x0446,0x0438,0x044F,0x0020,0x0432,0x0020,0x0440,0x0430,0x0437,0x0440,0x0430,0x0431,0x043E,0x0442,0x043A,0x0435,0x002E,0x0000},//Эта функция в разработке.
											  #endif
											  #ifdef LANGUAGE_AR_ENABLE
											  	{0x002E,0x0647,0x0631,0x064A,0x0648,0x0637,0x062A,0x0020,0x064A,0x0631,0x0627,0x062C,0x0000},//???? ??????.
											  #endif
											#else
											  #ifdef LANGUAGE_CN_ENABLE
												{0x6B64,0x529F,0x80FD,0x6B63,0x5728,0x5F00,0x53D1,0x3002,0x0000},//此功能正在开发。
											  #endif
											  #ifdef LANGUAGE_EN_ENABLE
												{0x0049,0x0074,0x0020,0x0069,0x0073,0x0020,0x0062,0x0065,0x0069,0x006E,0x0067,0x0020,0x0064,0x0065,0x0076,0x0065,0x006C,0x006F,0x0070,0x0065,0x0064,0x002E,0x0000},//It is being developed.
											  #endif
											#endif
											}; 

#ifdef CONFIG_QRCODE_SUPPORT
	if(g_ft_status == FT_STATUS_SMT)
		EnterFTSmtResultsScreen();
	else
		EnterFTAssemResultsScreen();
#else
	infor.x = 0;
	infor.y = 0;
	infor.w = LCD_WIDTH;
	infor.h = LCD_HEIGHT;
	infor.align = NOTIFY_ALIGN_CENTER;
	infor.type = NOTIFY_TYPE_POPUP;
	infor.img_count = 0;
	
	mmi_ucs2cpy(infor.text, (uint8_t*)&str_notify[global_settings.language]);
	DisplayPopUp(infor);
#endif/*CONFIG_QRCODE_SUPPORT*/
}

