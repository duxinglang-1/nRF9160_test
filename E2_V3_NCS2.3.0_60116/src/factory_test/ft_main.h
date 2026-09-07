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

typedef enum
{
	FT_STATUS_SMT,
	FT_STATUS_ASSEM,
	FT_STATUS_MAX
}FT_STATUS;

typedef struct
{
	uint16_t name[FT_MENU_NAME_MAX];
	ft_menu_handler sel_handler;
}ft_item_t;

typedef struct
{
	uint8_t cur_ret;
	uint8_t key_ret;
	uint8_t lcd_ret;
	uint8_t touch_ret;
	uint8_t temp_ret;
	uint8_t imu_ret;
	uint8_t flash_ret;
	uint8_t sim_ret;
	uint8_t ble_ret;
	uint8_t ppg_ret;
	uint8_t pmu_ret;
	uint8_t vib_ret;
	uint8_t wifi_ret;
	uint8_t net_ret;
	uint8_t gps_ret;
	uint8_t audio_ret;
	uint8_t wrist_ret;
}ft_smt_results_t;

typedef struct
{
	uint8_t key_ret;
	uint8_t lcd_ret;
	uint8_t touch_ret;
	uint8_t temp_ret;
	uint8_t imu_ret;
	uint8_t flash_ret;
	uint8_t sim_ret;
	uint8_t ble_ret;
	uint8_t ppg_ret;
	uint8_t pmu_ret;
	uint8_t vib_ret;
	uint8_t wifi_ret;
	uint8_t net_ret;
	uint8_t gps_ret;
	uint8_t audio_ret;
	uint8_t wrist_ret;
}ft_assem_results_t;

typedef struct
{
	FT_MENU_ID id;
	uint8_t index;
	uint8_t count;
	ft_item_t item[FT_MENU_MAX_COUNT];
	ft_menu_handler pg_handler[4];
}ft_menu_t;

extern uint8_t ft_main_menu_index;
extern uint8_t g_ft_status;
extern bool ft_menu_checked[FT_MENU_MAX_COUNT];
extern ft_menu_t ft_menu;
extern ft_smt_results_t ft_smt_results;
extern ft_assem_results_t ft_assem_results;
extern const ft_menu_t FT_SMT_MENU_MAIN;
extern const ft_menu_t FT_ASSEM_MENU_MAIN;

extern bool FactorySmtTestFinished(void);
extern bool FactoryTestActived(void);
extern void FactoryTestProccess(void);
extern void EnterFactoryTest(void);
extern void EnterFactorySmtTest(void);
extern void EnterFactoryAssemTest(void);
extern void EnterFactoryTestResults(void);
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
