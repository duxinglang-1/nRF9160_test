/****************************************Copyright (c)************************************************
** File Name:			    ft_main.h
** Descriptions:			Factory test main interface head file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __FT_MAIN_H__
#define __FT_MAIN_H__

#define FT_MENU_MAX_COUNT	20
#define FT_MENU_NAME_MAX	11

typedef void(*ft_menu_handler)(void);

typedef enum
{
	FT_CURRENT,
	FT_KEY,
	FT_LCD,
	FT_TOUCH,
	FT_TEMP,
	FT_IMU,
	FT_FLASH,
	FT_SIM,
	FT_BLE,
	FT_PPG,
	FT_PMU,
	FT_VIBRATE,
	FT_WIFI,
	FT_NET,
	FT_GPS,
	FT_MAIN,
	FT_AUDIO,
	FT_WRIST,
	FT_AGING,
	FT_MAX
}FT_MENU_ID;

typedef struct
{
	uint16_t name[FT_MENU_NAME_MAX];
	ft_menu_handler sel_handler;
}ft_item_t;

typedef struct
{
	FT_MENU_ID id;
	uint8_t index;
	uint8_t count;
	ft_item_t item[FT_MENU_MAX_COUNT];
	ft_menu_handler pg_handler[4];
}ft_menu_t;

extern uint8_t ft_main_menu_index;
extern bool ft_menu_checked[FT_MENU_MAX_COUNT];
extern ft_menu_t ft_menu;
extern const ft_menu_t FT_MENU_MAIN;

extern void FactoryTestExit(void);
extern bool FactryTestActived(void);
extern void FactoryTestProccess(void);
extern void EnterFactoryTest(void);
extern void FTMainMenuCurProc(void);
extern void FTMainMenuKeyProc(void);
extern void FTMainMenuLcdProc(void);
extern void FTMainMenuTouchProc(void);
extern void FTMainMenuTempProc(void);
extern void FTMainMenuWristProc(void);
extern void FTMainMenuIMUProc(void);
extern void FTMainMenuFlashProc(void);
extern void FTMainMenuSIMProc(void);
extern void FTMainMenuBleProc(void);
extern void FTMainMenuPPGProc(void);
extern void FTMainMenuPMUProc(void);
extern void FTMainMenuVibrateProc(void);
extern void FTMainMenuWifiProc(void);
extern void FTMainMenuNetProc(void);
extern void FTMainMenuGPSProc(void);

#endif/*__FT_MAIN_H__*/
