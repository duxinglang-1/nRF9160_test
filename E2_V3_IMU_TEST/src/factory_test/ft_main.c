/****************************************Copyright (c)************************************************
** File Name:			    ft_main.c
** Descriptions:			Factory test main interface source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
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

#define FT_MAIN_MENU_MAX_PER_PG	4
#define FT_SUB_MENU_MAX_PER_PG	3

#define FT_MENU_BG_W			169
#define FT_MENU_BG_H			38
#define FT_MENU_BG_X			((LCD_WIDTH-FT_MENU_BG_W)/2)
#define FT_MENU_BG_Y			38
#define FT_MENU_BG_OFFSET_Y		4
#define SETTINGS_MENU_STR_OFFSET_X	5			
#define SETTINGS_MENU_STR_OFFSET_Y	8

#define FT_MENU_STR_W			150
#define FT_MENU_STR_H			30
#define FT_MENU_STR_X			(FT_MENU_BG_X+5)
#define FT_MENU_STR_H			30
#define FT_MENU_STR_OFFSET_Y	8

static bool ft_running_flag = false;
static bool ft_main_redaw_flag = false;

uint8_t ft_main_menu_index = 0;

ft_menu_t ft_menu = {0};

void FTMainDumpProc(void)
{
}

void FTMainMenu1Proc(void)
{
	ft_main_menu_index = 0;
	EnterFTMenuCur();
}

void FTMainMenu2Proc(void)
{
	ft_main_menu_index = 1;
	EnterFTMenuKey();
}

void FTMainMenu3Proc(void)
{
	ft_main_menu_index = 2;
	EnterFTMenuLcd();
}

void FTMainMenu4Proc(void)
{
	ft_main_menu_index = 3;
	EnterFTMenuTouch();
}

void FTMainMenu5Proc(void)
{
	ft_main_menu_index = 4;
	EnterFTMenuTemp();
}

void FTMainMenu6Proc(void)
{
	ft_main_menu_index = 5;
	EnterFTMenuWrist();
}

void FTMainMenu7Proc(void)
{
	ft_main_menu_index = 6;
	EnterFTMenuIMU();
}

void FTMainMenu8Proc(void)
{
	ft_main_menu_index = 7;
	EnterFTMenuFlash();
}

void FTMainMenu9Proc(void)
{
	ft_main_menu_index = 8;
	EnterFTMenuSIM();
}

void FTMainMenu10Proc(void)
{
	ft_main_menu_index = 9;
	EnterFTMenuBle();
}

void FTMainMenu11Proc(void)
{
	ft_main_menu_index = 10;
	EnterFTMenuPPG();
}

void FTMainMenu12Proc(void)
{
	ft_main_menu_index = 11;
	EnterFTMenuPMU();
}

void FTMainMenu13Proc(void)
{
	ft_main_menu_index = 12;
	EnterFTMenuVibrate();
}

void FTMainMenu14Proc(void)
{
	ft_main_menu_index = 13;
	EnterFTMenuWifi();	
}

void FTMainMenu15Proc(void)
{
	ft_main_menu_index = 14;
	EnterFTMenuNet();
}

void FTMainMenu16Proc(void)
{
	ft_main_menu_index = 15;
	EnterFTMenuGPS();
}

static void FTMainMenuProcess(void)
{
	switch(ft_menu.index)
	{
	case 0:
		FTMainMenu1Proc();
		break;
	case 1:
		FTMainMenu2Proc();
		break;
	case 2:
		FTMainMenu3Proc();
		break;
	case 3:
		FTMainMenu4Proc();
		break;
	case 4:
		FTMainMenu5Proc();
		break;
	case 5:
		FTMainMenu6Proc();
		break;
	case 6:
		FTMainMenu7Proc();
		break;
	case 7:
		FTMainMenu8Proc();
		break;
	case 8:
		FTMainMenu9Proc();
		break;
	case 9:
		FTMainMenu10Proc();
		break;
	case 10:
		FTMainMenu11Proc();
		break;
	case 11:
		FTMainMenu12Proc();
		break;
	case 12:
		FTMainMenu13Proc();
		break;
	case 13:
		FTMainMenu14Proc();
		break;
	case 14:
		FTMainMenu15Proc();
		break;
	case 15:
		FTMainMenu16Proc();
		break;
	}
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
	16,
	{
		{0x7535,0x6D41,0x6D4B,0x8BD5,0x0000},//µÁ¡˜≤‚ ‘
		{0x6309,0x952E,0x6D4B,0x8BD5,0x0000},//∞¥º¸≤‚ ‘
		{0x5C4F,0x5E55,0x6D4B,0x8BD5,0x0000},//∆¡ƒª≤‚ ‘
		{0x89E6,0x6478,0x6D4B,0x8BD5,0x0000},//¥•√˛≤‚ ‘
		{0x6E29,0x5EA6,0x6D4B,0x8BD5,0x0000},//Œ¬∂»≤‚ ‘
		{0x8131,0x8155,0x6D4B,0x8BD5,0x0000},//Õ—ÕÛ≤‚ ‘
		{0x0049,0x004D,0x0055,0x6D4B,0x8BD5,0x0000},				//IMU≤‚ ‘
		{0x0046,0x004C,0x0041,0x0053,0x0048,0x6D4B,0x8BD5,0x0000},	//FLASH≤‚ ‘
		{0x0053,0x0049,0x004D,0x5361,0x6D4B,0x8BD5,0x0000},			//SIMø®≤‚ ‘
		{0x0042,0x004C,0x0045,0x6D4B,0x8BD5,0x0000},				//BLE≤‚ ‘
		{0x0050,0x0050,0x0047,0x6D4B,0x8BD5,0x0000},				//PPG≤‚ ‘
		{0x5145,0x7535,0x6D4B,0x8BD5,0x0000},						//≥‰µÁ≤‚ ‘
		{0x9707,0x52A8,0x6D4B,0x8BD5,0x0000},						//’∂Ø≤‚ ‘
		{0x0057,0x0069,0x0046,0x0069,0x6D4B,0x8BD5,0x0000},			//WiFi≤‚ ‘
		{0x7F51,0x7EDC,0x6D4B,0x8BD5,0x0000},						//Õ¯¬Á≤‚ ‘
		{0x0047,0x0050,0x0053,0x6D4B,0x8BD5,0x0000},				//GPS≤‚ ‘
	},	
	{
		FTMainMenu1Proc,
		FTMainMenu2Proc,
		FTMainMenu3Proc,
		FTMainMenu4Proc,
		FTMainMenu5Proc,
		FTMainMenu6Proc,
		FTMainMenu7Proc,
		FTMainMenu8Proc,
		FTMainMenu9Proc,
		FTMainMenu10Proc,
		FTMainMenu11Proc,
		FTMainMenu12Proc,
		FTMainMenu13Proc,
		FTMainMenu14Proc,
		FTMainMenu15Proc,
		FTMainMenu16Proc,
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

	LCD_Clear(BLACK);
	LCD_SetFontSize(FONT_SIZE_20);
	LCD_SetFontBgColor(bg_clor);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	for(i=0;i<4;i++)
	{
		LCD_ShowImg_From_Flash(FT_MENU_BG_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);
		LCD_SetFontColor(WHITE);
	
		LCD_MeasureUniString(ft_menu.name[i+4*(ft_menu.index/4)], &w, &h);
		LCD_ShowUniString(FT_MENU_STR_X+(FT_MENU_STR_W-w)/2, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_STR_OFFSET_Y, ft_menu.name[i+4*(ft_menu.index/4)]);
	
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_MENU_BG_X, 
									FT_MENU_BG_X+FT_MENU_BG_W, 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_BG_H, 
									ft_menu.sel_handler[i+4*(ft_menu.index/4)]);
	#endif
	}	

#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[0]);
	register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[1]);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestExit);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestExit);	
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
	
	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	
	LCD_SetFontSize(FONT_SIZE_20);
	LCD_SetFontBgColor(bg_clor);

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	for(i=0;i<4;i++)
	{
		LCD_ShowImg_From_Flash(FT_MENU_BG_X, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), IMG_SET_BG_ADDR);
		LCD_SetFontColor(WHITE);
	
		LCD_MeasureUniString(ft_menu.name[i+4*(ft_menu.index/4)], &w, &h);
		LCD_ShowUniString(FT_MENU_STR_X+(FT_MENU_STR_W-w)/2, FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_STR_OFFSET_Y, ft_menu.name[i+4*(ft_menu.index/4)]);
	
	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_MENU_BG_X, 
									FT_MENU_BG_X+FT_MENU_BG_W, 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y), 
									FT_MENU_BG_Y+i*(FT_MENU_BG_H+FT_MENU_BG_OFFSET_Y)+FT_MENU_BG_H, 
									ft_menu.sel_handler[i+4*(ft_menu.index/4)]);
	#endif
	}

	SetLeftKeyUpHandler(FTMainMenuProcess);
	SetRightKeyUpHandler(FactoryTestExit);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_MOVING_UP, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[0]);
	register_touch_event_handle(TP_EVENT_MOVING_DOWN, 0, LCD_WIDTH, 0, LCD_HEIGHT, ft_menu.pg_handler[1]);
	register_touch_event_handle(TP_EVENT_MOVING_LEFT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestExit);
	register_touch_event_handle(TP_EVENT_MOVING_RIGHT, 0, LCD_WIDTH, 0, LCD_HEIGHT, FactoryTestExit);	
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

void FactoryTestExit(void)
{
	ft_running_flag = false;
	
	SetModemTurnOn();
	EntryIdleScr();
}

void EnterFactoryTestScreen(void)
{
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
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
	if(!ft_running_flag)
		return;

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
	case FT_WIFI:
		FTMenuWifiProcess();
		break;
	}
}
