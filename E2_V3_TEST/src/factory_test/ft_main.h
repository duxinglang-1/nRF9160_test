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

#define FT_MENU_LANGUAGE	2
#define FT_MENU_MAX_COUNT	4
#define FT_MENU_NAME_MAX	50

typedef void(*ft_menu_handler)(void);

typedef enum
{
	FT_MAIN,
	FT_FLASH,
	FT_LCD,
	FT_KEY,
	FT_WRIST,
	FT_VIABRATE,
	FT_NET,
	FT_AUDIO,
	FT_BLE,
	FT_GPS,
	FT_IMU,
	FT_PPG,
	FT_TEMP,
	FT_TOUCH,
	FT_WIFI,
	FT_MAX
}FT_MENU_ID;

typedef struct
{
	FT_MENU_ID id;
	uint8_t index;
	uint8_t count;
	uint16_t name[FT_MENU_LANGUAGE][FT_MENU_MAX_COUNT][FT_MENU_NAME_MAX];;
	ft_menu_handler sel_handler[FT_MENU_MAX_COUNT];
	ft_menu_handler pg_handler[4];
}ft_menu_t;

extern uint8_t ft_main_menu_index;

extern ft_menu_t ft_menu;

extern void FactoryTestExit(void);
extern bool FactryTestActived(void);
extern void FactoryTestProccess(void);
extern void EnterFactoryTest(void);
#endif/*__FT_MAIN_H__*/
