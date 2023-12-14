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

bool ft_menu_checked[FT_MENU_MAX_COUNT] = {false};
ft_menu_t ft_menu = {0};

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
	EnterFTMenuKey();
}

void FTMainMenuLcdProc(void)
{
	ft_main_menu_index = FT_LCD;
	EnterFTMenuLcd();
}

void FTMainMenuTouchProc(void)
{
	ft_main_menu_index = FT_TOUCH;
	EnterFTMenuTouch();
}

void FTMainMenuTempProc(void)
{
	ft_main_menu_index = FT_TEMP;
	EnterFTMenuTemp();
}

void FTMainMenuWristProc(void)
{
	ft_main_menu_index = FT_WRIST;
	EnterFTMenuWrist();
}

void FTMainMenuIMUProc(void)
{
	ft_main_menu_index = FT_IMU;
	EnterFTMenuIMU();
}

void FTMainMenuFlashProc(void)
{
	ft_main_menu_index = FT_FLASH;
	EnterFTMenuFlash();
}

void FTMainMenuSIMProc(void)
{
	ft_main_menu_index = FT_SIM;
	EnterFTMenuSIM();
}

void FTMainMenuBleProc(void)
{
	ft_main_menu_index = FT_BLE;
	EnterFTMenuBle();
}

void FTMainMenuPPGProc(void)
{
	ft_main_menu_index = FT_PPG;
	EnterFTMenuPPG();
}

void FTMainMenuPMUProc(void)
{
	ft_main_menu_index = FT_PMU;
	EnterFTMenuPMU();
}

void FTMainMenuVibrateProc(void)
{
	ft_main_menu_index = FT_VIBRATE;
	EnterFTMenuVibrate();
}

void FTMainMenuWifiProc(void)
{
#ifdef CONFIG_WIFI_SUPPORT
	ft_main_menu_index = FT_WIFI;
	EnterFTMenuWifi();	
#endif
}

void FTMainMenuNetProc(void)
{
	ft_main_menu_index = FT_NET;
	EnterFTMenuNet();
}

void FTMainMenuGPSProc(void)
{
	ft_main_menu_index = FT_GPS;
	EnterFTMenuGPS();
}

static void FTMainMenuProcess(void)
{
	ft_menu.item[ft_menu.index].sel_handler();
}

void FTMainMenuPgUpProc(void)
{
	uint8_t count = FT_MAIN_MENU_MAX_PER_PG;
	
	if(ft_menu.index < (ft_menu.count - count))
	{
		ft_menu.index += count;
		if(screen_id == SCREEN_ID_FACTORY_TEST)
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void FTMainMenuPgDownProc(void)
{
	uint8_t count = FT_MAIN_MENU_MAX_PER_PG;

	if(ft_menu.index >= count)
	{
		ft_menu.index -= count;
		if(screen_id == SCREEN_ID_FACTORY_TEST)
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

const ft_menu_t FT_MENU_MAIN = 
{
	FT_MAIN,
	0,
	15,
	{
		//µÁ¡˜≤‚ ‘
		{
			{0x7535,0x6D41,0x6D4B,0x8BD5,0x0000},
			FTMainMenuCurProc,
		},
		//∞¥º¸≤‚ ‘
		{
			{0x6309,0x952E,0x6D4B,0x8BD5,0x0000},
			FTMainMenuKeyProc,
		},
		//∆¡ƒª≤‚ ‘
		{
			{0x5C4F,0x5E55,0x6D4B,0x8BD5,0x0000},
			FTMainMenuLcdProc,
		},
		//¥•√˛≤‚ ‘
		{
			{0x89E6,0x6478,0x6D4B,0x8BD5,0x0000},
			FTMainMenuTouchProc,
		},
		//Œ¬∂»≤‚ ‘
		{
			{0x6E29,0x5EA6,0x6D4B,0x8BD5,0x0000},
			FTMainMenuTempProc,
		},
		//Õ—ÕÛ≤‚ ‘
		//{
		//	{0x8131,0x8155,0x6D4B,0x8BD5,0x0000},
		//	FTMainMenuWristProc,
		//},
		//IMU≤‚ ‘
		{
			{0x0049,0x004D,0x0055,0x6D4B,0x8BD5,0x0000},
			FTMainMenuIMUProc,
		},
		//FLASH≤‚ ‘
		{
			{0x0046,0x004C,0x0041,0x0053,0x0048,0x6D4B,0x8BD5,0x0000},
			FTMainMenuFlashProc,
		},
		//SIMø®≤‚ ‘
		{
			{0x0053,0x0049,0x004D,0x5361,0x6D4B,0x8BD5,0x0000},
			FTMainMenuSIMProc,
		},
		//BLE≤‚ ‘
		{
			{0x0042,0x004C,0x0045,0x6D4B,0x8BD5,0x0000},
			FTMainMenuBleProc,
		},
		//PPG≤‚ ‘
		{
			{0x0050,0x0050,0x0047,0x6D4B,0x8BD5,0x0000},
			FTMainMenuPPGProc,
		},
		//≥‰µÁ≤‚ ‘
		{
			{0x5145,0x7535,0x6D4B,0x8BD5,0x0000},
			FTMainMenuPMUProc,
		},
		//’∂Ø≤‚ ‘
		{
			{0x9707,0x52A8,0x6D4B,0x8BD5,0x0000},
			FTMainMenuVibrateProc,
		},
		//WiFi≤‚ ‘
		{
			{0x0057,0x0069,0x0046,0x0069,0x6D4B,0x8BD5,0x0000},
			FTMainMenuWifiProc,
		},
		//Õ¯¬Á≤‚ ‘
		{
			{0x7F51,0x7EDC,0x6D4B,0x8BD5,0x0000},
			FTMainMenuNetProc,
		},
		//GPS≤‚ ‘
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
	uint32_t img_addr[2] = {IMG_SELECT_ICON_NO_ADDR,IMG_SELECT_ICON_YES_ADDR};

	LCD_Clear(BLACK);
	LCD_SetFontSize(FONT_SIZE_20);
	LCD_SetFontBgColor(bg_clor);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	for(i=0;i<4;i++)
	{
		if((4*(ft_menu.index/4) + i) >= ft_menu.count)
			break;
		LCD_ShowImg_From_Flash(FT_MENU_BG_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);
		LCD_SetFontColor(WHITE);
	
		LCD_ShowUniString(FT_MENU_STR_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_STR_OFFSET_Y, ft_menu.item[i+4*(ft_menu.index/4)].name);
		LCD_ShowImg_From_Flash(FT_MENU_CHECKED_X, FT_MENU_CHECKED_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), img_addr[ft_menu_checked[i+4*(ft_menu.index/4)]]);
	
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_MENU_BG_X, 
									FT_MENU_BG_X+FT_MENU_BG_W, 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_BG_H, 
									ft_menu.item[i+4*(ft_menu.index/4)].sel_handler);
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
	uint32_t img_addr[2] = {IMG_SELECT_ICON_NO_ADDR,IMG_SELECT_ICON_YES_ADDR};
	
	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	LCD_SetFontSize(FONT_SIZE_20);
	LCD_SetFontBgColor(bg_clor);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	for(i=0;i<4;i++)
	{
		if((4*(ft_menu.index/4) + i) >= ft_menu.count)
			break;
		LCD_ShowImg_From_Flash(FT_MENU_BG_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);
		LCD_SetFontColor(WHITE);
		LCD_ShowUniString(FT_MENU_STR_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_STR_OFFSET_Y, ft_menu.item[i+4*(ft_menu.index/4)].name);
		LCD_ShowImg_From_Flash(FT_MENU_CHECKED_X, FT_MENU_CHECKED_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), img_addr[ft_menu_checked[i+4*(ft_menu.index/4)]]);
		
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_MENU_BG_X, 
									FT_MENU_BG_X+FT_MENU_BG_W, 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_BG_H, 
									ft_menu.item[i+4*(ft_menu.index/4)].sel_handler);
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
	EnterSettingsScreen();
}

static void FactoryTestNextExit(void)
{
	ft_running_flag = false;
	
	SetModemTurnOn();
	EnterFTAgingTest();
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

	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST;	
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

void ReturnFTMainMenu(void)
{
	memcpy(&ft_menu, &FT_MENU_MAIN, sizeof(ft_menu_t));
	ft_menu.index = ft_main_menu_index;
	FactoryTestMainShow();
}

void EnterFactoryTest(void)
{
	ft_running_flag = true;
	memcpy(&ft_menu, &FT_MENU_MAIN, sizeof(ft_menu_t));

	EnterFactoryTestScreen();
}

bool FactryTestActived(void)
{
	return ft_running_flag;
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
	case FT_WRIST:
		FTMenuWristProcess();
		break;
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
