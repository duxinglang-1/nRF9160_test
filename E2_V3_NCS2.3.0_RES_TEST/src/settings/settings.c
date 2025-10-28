#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/fs/nvs.h>
#include <dk_buttons_and_leds.h>
#include "screen.h"
#include "settings.h"
#include "datetime.h"
#ifdef CONFIG_ALARM_SUPPORT
#include "alarm.h"
#endif
#ifdef CONFIG_FACTORY_TEST_SUPPORT
#include "ft_main.h"
#endif
#include "lcd.h"
#include "codetrans.h"
#include "inner_flash.h"
#include "logger.h"

#ifdef FW_FOR_CN
	#ifdef LANGUAGE_CN_ENABLE
	#else
		#error "In the version of chinese, language chinese must to be enabled"
	#endif
#else
	#if defined(LANGUAGE_EN_ENABLE)\
		||defined(LANGUAGE_DE_ENABLE)\
		||defined(LANGUAGE_FR_ENABLE)\
		||defined(LANGUAGE_IT_ENABLE)\
		||defined(LANGUAGE_ES_ENABLE)\
		||defined(LANGUAGE_PT_ENABLE)\
		||defined(LANGUAGE_PL_ENABLE)\
		||defined(LANGUAGE_SV_ENABLE)\
		||defined(LANGUAGE_JA_ENABLE)\
		||defined(LANGUAGE_KR_ENABLE)\
		||defined(LANGUAGE_RU_ENABLE)\
		||defined(LANGUAGE_AR_ENABLE)
	#else
		#error "In the version of foreign, at least one foreign language needs to be enabled"
	#endif
#endif/*FW_FOR_CN*/

bool need_save_settings = false;
bool need_save_time = false;
bool need_reset_settings = false;
bool need_reset_bk_level = false;
bool g_language_r2l = false;

uint8_t g_fw_version[64] = VERSION_STR LANG_BRANCH FALL_BRANCH;

RESET_STATUS g_reset_status = RESET_STATUS_IDLE;

static bool reset_redraw_flag = false;
static bool reset_start_flag = false;
static bool reset_reboot_flag = false;

static uint8_t main_menu_index_bk = 0;

global_settings_t global_settings = {0};
settings_menu_t settings_menu = {0};

static void FactoryResetCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(reset_timer, FactoryResetCallBack, NULL);
static void FactoryResetStartCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(reset_start_timer, FactoryResetStartCallBack, NULL);

extern sys_date_timer_t date_time;

static void SettingsMenuDumpProc(void);
static void SettingsMainMenu1Proc(void);
static void SettingsMainMenu2Proc(void);
static void SettingsMainMenu3Proc(void);
static void SettingsMainMenu4Proc(void);
static void SettingsMainMenu5Proc(void);
static void SettingsMainMenu6Proc(void);
static void SettingsMainMenu7Proc(void);
static void SettingsMenuLang1Proc(void);
static void SettingsMenuLang2Proc(void);
static void SettingsMenuLang3Proc(void);
static void SettingsMenuReset1Proc(void);
static void SettingsMenuReset2Proc(void);
static void SettingsMenuCaremateQRProc(void);
static void SettingsMenuOTAProc(void);
static void SettingsMenuBrightness1Proc(void);
static void SettingsMenuBrightness2Proc(void);
static void SettingsMenuBrightness3Proc(void);
static void SettingsMenuTemp1Proc(void);
static void SettingsMenuTemp2Proc(void);
static void SettingsMenuPgUpProc(void);
static void SettingsMenuPgDownProc(void);
static void SettingsMenuPgLeftProc(void);
static void SettingsMenuPgRightProc(void);
static void SettingsMenuDeviceProc(void);
static void SettingsMenuSIMProc(void);
static void SettingsMenuFWProc(void);

const sys_date_timer_t FACTORY_DEFAULT_TIME = 
{
	2023,
	1,
	1,
	0,
	0,
	0,
	0		//0=sunday
};

const RES_LANGUAGES_ID LANG_MENU_ITEM[] = 
{
#ifndef FW_FOR_CN
  #ifdef LANGUAGE_EN_ENABLE
	LANGUAGE_EN,					//English
  #endif	
  #ifdef LANGUAGE_DE_ENABLE
	LANGUAGE_DE,					//Deutsch
  #endif
  #ifdef LANGUAGE_FR_ENABLE
	LANGUAGE_FR,					//French
  #endif
  #ifdef LANGUAGE_IT_ENABLE
	LANGUAGE_IT,					//Italian
  #endif
  #ifdef LANGUAGE_ES_ENABLE
	LANGUAGE_ES,					//Spanish
  #endif
  #ifdef LANGUAGE_PT_ENABLE
	LANGUAGE_PT,					//Portuguese
  #endif
  #ifdef LANGUAGE_PL_ENABLE
	LANGUAGE_PL,					//Polish
  #endif
  #ifdef LANGUAGE_SV_ENABLE
	LANGUAGE_SV,					//Swedish
  #endif	
  #ifdef LANGUAGE_JA_ENABLE	
	LANGUAGE_JA,					//Japanese
  #endif	
  #ifdef LANGUAGE_KR_ENABLE	
	LANGUAGE_KR,					//Korea
  #endif	
  #ifdef LANGUAGE_RU_ENABLE	
	LANGUAGE_RU,					//Russian
  #endif	
  #ifdef LANGUAGE_AR_ENABLE	
	LANGUAGE_AR,					//Arabic
  #endif	
#else
  #ifdef LANGUAGE_CN_ENABLE
	LANGUAGE_CN,					//Chinese
  #endif	
  #ifdef LANGUAGE_EN_ENABLE	
	LANGUAGE_EN,					//English
  #endif	
#endif
};

const RES_STRINGS_ID LANGUAGE_MENU_STR[] = 
{
#ifndef FW_FOR_CN
  #ifdef LANGUAGE_EN_ENABLE
	STR_ID_LANGUAGE_MENU_EN,
  #endif
  #ifdef LANGUAGE_DE_ENABLE
	STR_ID_LANGUAGE_MENU_DE,
  #endif
  #ifdef LANGUAGE_FR_ENABLE
	STR_ID_LANGUAGE_MENU_FR,
  #endif
  #ifdef LANGUAGE_IT_ENABLE
	STR_ID_LANGUAGE_MENU_IT,
  #endif
  #ifdef LANGUAGE_ES_ENABLE
	STR_ID_LANGUAGE_MENU_ES,
  #endif
  #ifdef LANGUAGE_PT_ENABLE
	STR_ID_LANGUAGE_MENU_PT,
  #endif
  #ifdef LANGUAGE_PL_ENABLE
	STR_ID_LANGUAGE_MENU_PL,
  #endif
  #ifdef LANGUAGE_SV_ENABLE
	STR_ID_LANGUAGE_MENU_SV,
  #endif
  #ifdef LANGUAGE_JA_ENABLE
	STR_ID_LANGUAGE_MENU_JA,
  #endif
  #ifdef LANGUAGE_KR_ENABLE
	STR_ID_LANGUAGE_MENU_KO,
  #endif
  #ifdef LANGUAGE_RU_ENABLE
	STR_ID_LANGUAGE_MENU_RU,
  #endif
  #ifdef LANGUAGE_AR_ENABLE
	STR_ID_LANGUAGE_MENU_AR,
  #endif
#else
  #ifdef LANGUAGE_CN_ENABLE
	STR_ID_LANGUAGE_MENU_CN,
  #endif
  #ifdef LANGUAGE_EN_ENABLE
	STR_ID_LANGUAGE_MENU_EN,
  #endif
#endif
};

const global_settings_t FACTORY_DEFAULT_SETTINGS = 
{
	SETTINGS_STATUS_NORMAL,	//status flag
	true,					//temp turn on
	true,					//heart rate turn on
	true,					//blood pressure turn on
	true,					//blood oxygen turn on		
	true,					//wake screen by wrist
	false,					//wrist off check
	true,					//fall check
	3,						//location type: 1:only wifi,2:only gps,3:wifi+gps,4:gps+wifi
	0,						//target steps
	60,						//health interval
	TEMP_UINT_C,			//Centigrade
	TIME_FORMAT_24,			//24 format
	LANG_MENU_ITEM[0],		//language
	DATE_FORMAT_YYYYMMDD,	//date format
	CLOCK_MODE_DIGITAL,		//colck mode
	BACKLIGHT_10_SEC,		//backlight time
	BACKLIGHT_LEVEL_2,		//backlight level
	{true,1},				//PHD
	{500,60},				//position interval
	{120,75},				//pb calibration
	{						//alarm
		{false,0,0,0},		
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
		{false,0,0,0},
	},
};

const settings_menu_t SETTING_MAIN_MENU = 
{
	SETTINGS_MENU_MAIN,
	0,
	7,
	{
		STR_ID_LANGUAGES,
		STR_ID_SCR_BRIGHT,
		STR_ID_TEMP_DSP,
		STR_ID_DEVICE_INFO,
		STR_ID_CAREMATE_QR,
		STR_ID_FACTORY_DEFAULT,
		STR_ID_OTA,
	},
	{
		//select proc func
		SettingsMainMenu1Proc,
		SettingsMainMenu2Proc,
		SettingsMainMenu3Proc,
		SettingsMainMenu4Proc,
		SettingsMainMenu5Proc,
		SettingsMainMenu6Proc,
		SettingsMainMenu7Proc,
	},
	{	
		//page proc func
		SettingsMenuPgUpProc,
		SettingsMenuPgDownProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},
};

const settings_menu_t SETTING_MENU_LANGUAGE = 
{
	SETTINGS_MENU_LANGUAGE,
	0,
	sizeof(LANG_MENU_ITEM)/sizeof(LANG_MENU_ITEM[0]),
	{
	#ifndef FW_FOR_CN
	  #ifdef LANGUAGE_EN_ENABLE
		STR_ID_LANGUAGE_MENU_EN,
	  #endif
	  #ifdef LANGUAGE_DE_ENABLE
		STR_ID_LANGUAGE_MENU_DE,
	  #endif
	  #ifdef LANGUAGE_FR_ENABLE
		STR_ID_LANGUAGE_MENU_FR,
	  #endif
	  #ifdef LANGUAGE_IT_ENABLE
		STR_ID_LANGUAGE_MENU_IT,
	  #endif
	  #ifdef LANGUAGE_ES_ENABLE
		STR_ID_LANGUAGE_MENU_ES,
	  #endif
	  #ifdef LANGUAGE_PT_ENABLE
		STR_ID_LANGUAGE_MENU_PT,
	  #endif
	  #ifdef LANGUAGE_PL_ENABLE
		STR_ID_LANGUAGE_MENU_PL,
	  #endif
	  #ifdef LANGUAGE_SV_ENABLE
		STR_ID_LANGUAGE_MENU_SV,
	  #endif
	  #ifdef LANGUAGE_JA_ENABLE
		STR_ID_LANGUAGE_MENU_JA,
	  #endif
	  #ifdef LANGUAGE_KR_ENABLE
		STR_ID_LANGUAGE_MENU_KO,
	  #endif
	  #ifdef LANGUAGE_RU_ENABLE
		STR_ID_LANGUAGE_MENU_RU,
	  #endif
	  #ifdef LANGUAGE_AR_ENABLE
		STR_ID_LANGUAGE_MENU_AR,
	  #endif
	#else
	  #ifdef LANGUAGE_CN_ENABLE
		STR_ID_LANGUAGE_MENU_CN,
	  #endif
	  #ifdef LANGUAGE_EN_ENABLE
		STR_ID_LANGUAGE_MENU_EN,
	  #endif
	#endif
	},
	{
		SettingsMenuLang1Proc,
		SettingsMenuLang2Proc,
		SettingsMenuLang3Proc,
	},
	{	
		//page proc func
		SettingsMenuPgUpProc,
		SettingsMenuPgDownProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};

const settings_menu_t SETTING_MENU_CAREMATE_QR = 
{
	SETTINGS_MENU_CAREMATE_QR,
	0,
	1,
	{
		0x0000,
	},
	{
		SettingsMenuCaremateQRProc,
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};

const settings_menu_t SETTING_MENU_FACTORY_RESET = 
{
	SETTINGS_MENU_FACTORY_RESET,
	0,
	2,
	{
		0x0000,
		0x0000,
	},
	{
		SettingsMenuReset1Proc,
		SettingsMenuReset2Proc,
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};

const settings_menu_t SETTING_MENU_OTA_UPDATE = 
{
	SETTINGS_MENU_OTA,
	0,
	1,
	{
		0x0000,
	},
	{
		SettingsMenuOTAProc,
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};


const settings_menu_t SETTING_MENU_BRIGHTNESS = 
{
	SETTINGS_MENU_BRIGHTNESS,
	0,
	2,
	{
		0x0000,
		0x0000,
	},	
	{
		SettingsMenuBrightness1Proc,
		SettingsMenuBrightness2Proc,
		SettingsMenuBrightness3Proc,
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuBrightness1Proc,
		SettingsMenuBrightness2Proc,		
	},
};

const settings_menu_t SETTING_MENU_TEMP = 
{
	SETTINGS_MENU_TEMP,
	0,
	2,
	{
		STR_ID_CELSIUS,
		STR_ID_FAHRENHEIT,
	},
	{
		SettingsMenuTemp1Proc,
		SettingsMenuTemp2Proc,
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};

const settings_menu_t SETTING_MENU_DEVICE = 
{
	SETTINGS_MENU_DEVICE,
	0,
	3,
	{
		STR_ID_DEVICE_IMEI,		//IMEI:
		STR_ID_DEVICE_IMSI,		//IMSI:
		STR_ID_DEVICE_MCU,		//MCU:
	},
	{
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
	},
	{	
		//page proc func
		SettingsMenuPgUpProc,
		SettingsMenuPgDownProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};

void FactoryResetCallBack(struct k_timer *timer_id)
{
	switch(g_reset_status)
	{
	case RESET_STATUS_IDLE:
		break;

	case RESET_STATUS_RUNNING:
		g_reset_status = RESET_STATUS_FAIL;
		reset_redraw_flag = true;
		break;

	case RESET_STATUS_SUCCESS:
		reset_reboot_flag = true;
		break;

	case RESET_STATUS_FAIL:
		g_reset_status = RESET_STATUS_IDLE;
		reset_redraw_flag = true;
		break;
	}
}

void FactoryResetStartCallBack(struct k_timer *timer_id)
{
	reset_start_flag = true;
}

void SaveSystemDateTime(void)
{
	SaveDateTimeToInnerFlash(date_time);
}

void ResetSystemTime(void)
{
	memcpy(&date_time, &FACTORY_DEFAULT_TIME, sizeof(sys_date_timer_t));
	SaveSystemDateTime();
}

void InitSystemDateTime(void)
{
	sys_date_timer_t mytime = {0};

	ReadDateTimeFromInnerFlash(&mytime);
	
	if(!CheckSystemDateTimeIsValid(mytime))
	{
		memcpy(&mytime, &FACTORY_DEFAULT_TIME, sizeof(sys_date_timer_t));
	}
	memcpy(&date_time, &mytime, sizeof(sys_date_timer_t));

	SaveSystemDateTime();
	StartSystemDateTime();
}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void SaveFactoryTestResults(FT_STATUS type, void *ret)
{
	SaveFtResultsToInnerFlash(type, ret);
}

void ResetFactoryTestResults(void)
{
	memset(&ft_smt_results, 0, sizeof(ft_smt_results_t));
	SaveFactoryTestResults(FT_STATUS_SMT, &ft_smt_results);

	memset(&ft_assem_results, 0, sizeof(ft_assem_results_t));
	SaveFactoryTestResults(FT_STATUS_ASSEM, &ft_assem_results);

	g_ft_status = FT_STATUS_SMT;
	SaveFtStatusToInnerFlash(g_ft_status);
}

void InitFactoryTestResults(void)
{
	ReadFtStatusFromInnerFlash(&g_ft_status);
	ReadFtResultsFromInnerFlash(FT_STATUS_SMT, &ft_smt_results);
	ReadFtResultsFromInnerFlash(FT_STATUS_ASSEM, &ft_assem_results);
	
	if(FactorySmtTestFinished())
	{
		g_ft_status = FT_STATUS_ASSEM;
		SaveFtStatusToInnerFlash(g_ft_status);
	}
}
#endif

void SaveSystemSettings(void)
{
	SaveSettingsToInnerFlash(global_settings);
}

void ResetSystemSettings(void)
{
	memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
	SaveSystemSettings();
}

void InitSystemSettings(void)
{
	int err;

	ReadSettingsFromInnerFlash(&global_settings);

	switch(global_settings.flag)
	{
	case SETTINGS_STATUS_INIT:
		ResetInnerFlash();
		memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
		SaveSystemSettings();
		break;

	case SETTINGS_STATUS_OTA:
		memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
		SaveSystemSettings();
		break;
		
	case SETTINGS_STATUS_NORMAL:
		break;		
	}

	switch(global_settings.language)
	{
#ifndef FW_FOR_CN
  #ifdef LANGUAGE_AR_ENABLE
	case LANGUAGE_AR:
		g_language_r2l = true;
		break;
  #endif		
#endif		
	default:
		g_language_r2l = false;
		break;
	}
	
	InitSystemDateTime();

#ifdef CONFIG_FACTORY_TEST_SUPPORT
	InitFactoryTestResults();
#endif

#ifdef CONFIG_ALARM_SUPPORT	
	AlarmRemindInit();
#endif
	mmi_chset_init();
}

void ResetLocalData(void)
{
	clear_cur_local_in_record();
	clear_local_in_record();
}

void ResetHealthData(void)
{
#if defined(CONFIG_PPG_SUPPORT)||defined(CONFIG_TEMP_SUPPORT)
	clear_cur_health_in_record();
	clear_health_in_record();
#endif

#ifdef CONFIG_PPG_SUPPORT
	ClearAllHrRecData();
	ClearAllSpo2RecData();
	ClearAllBptRecData();
	sh_clear_bpt_cal_data();
#endif

#ifdef CONFIG_TEMP_SUPPORT
	ClearAllTempRecData();
#endif
}

void ResetSportData(void)
{
#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
	clear_cur_sport_in_record();
	clear_sport_in_record();
#endif

#ifdef CONFIG_IMU_SUPPORT
#ifdef CONFIG_STEP_SUPPORT
	ClearAllStepRecData();
#endif

#ifdef CONFIG_SLEEP_SUPPORT
	ClearAllSleepRecData();
#endif
#endif
}

void ResetFactoryDefault(void)
{
#ifdef CONFIG_FACTORY_TEST_SUPPORT
	ResetFactoryTestResults();	
#endif

	ResetSystemTime();
	ResetSystemSettings();

	ResetLocalData();
	ResetHealthData();
	ResetSportData();

	LogClear();

	if((screen_id == SCREEN_ID_SETTINGS) && (settings_menu.id == SETTINGS_MENU_FACTORY_RESET))
	{
		if(k_timer_remaining_get(&reset_timer) > 0)
			k_timer_stop(&reset_timer);
		
		g_reset_status = RESET_STATUS_SUCCESS;
		reset_redraw_flag = true;
	}
}

void ResetUpdateStatus(void)
{
	switch(g_reset_status)
	{
	case RESET_STATUS_IDLE:
		memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
		settings_menu.index = main_menu_index_bk;
		break;

	case RESET_STATUS_RUNNING:
		break;

	case RESET_STATUS_SUCCESS:
	case RESET_STATUS_FAIL:
		k_timer_start(&reset_timer, K_MSEC(1000), K_NO_WAIT);
		break;
	}
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuDumpProc(void)
{
}

void SettingsMainMenu1Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_LANGUAGE, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu2Proc(void)
{
	main_menu_index_bk = settings_menu.index;

	memcpy(&settings_menu, &SETTING_MENU_BRIGHTNESS, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}	
}

void SettingsMainMenu3Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_TEMP, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu4Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_DEVICE, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu5Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_CAREMATE_QR, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu6Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_FACTORY_RESET, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu7Proc(void)
{
#if defined(CONFIG_FOTA_DOWNLOAD)
	extern uint8_t g_new_fw_ver[64];

	if(((strcmp(g_new_fw_ver,g_fw_version) != 0) && (strlen(g_new_fw_ver) > 0))
		#ifdef NB_SIGNAL_TEST
		 || 1
		#endif
		)
	{
		fota_start();
	}
	else
#endif
	{
		main_menu_index_bk = settings_menu.index;
		memcpy(&settings_menu, &SETTING_MENU_OTA_UPDATE, sizeof(settings_menu_t));

		if(screen_id == SCREEN_ID_SETTINGS)
		{
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
		}
	}
}

void SettingsMenuLang1Proc(void)
{
	if(global_settings.language != LANG_MENU_ITEM[settings_menu.index+0])
	{
		global_settings.language = LANG_MENU_ITEM[settings_menu.index+0];
		need_save_settings = true;
	}

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuLang2Proc(void)
{
	if(global_settings.language != LANG_MENU_ITEM[settings_menu.index+1])
	{
		global_settings.language = LANG_MENU_ITEM[settings_menu.index+1];
		need_save_settings = true;
	}

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuLang3Proc(void)
{
	if(global_settings.language != LANG_MENU_ITEM[settings_menu.index+2])
	{
		global_settings.language = LANG_MENU_ITEM[settings_menu.index+2];
		need_save_settings = true;
	}

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuReset1Proc(void)
{
	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuReset2Proc(void)
{
	g_reset_status = RESET_STATUS_RUNNING;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}

	k_timer_start(&reset_start_timer, K_MSEC(100), K_NO_WAIT);
}

void SettingsMenuCaremateQRProc(void)
{
	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuOTAProc(void)
{
	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuBrightness1Proc(void)
{
	if(global_settings.backlight_level > BACKLIGHT_LEVEL_MIN)
	{
		global_settings.backlight_level--;
		need_save_settings = true;
		need_reset_bk_level = true;

		if(screen_id == SCREEN_ID_SETTINGS)
		{
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
		}
	}
}

void SettingsMenuBrightness2Proc(void)
{
	if(global_settings.backlight_level < BACKLIGHT_LEVEL_MAX)
	{
		global_settings.backlight_level++;
		need_save_settings = true;
		need_reset_bk_level = true;

		if(screen_id == SCREEN_ID_SETTINGS)
		{
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
		}
	}
}

void SettingsMenuBrightness3Proc(void)
{
	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuTemp1Proc(void)
{
	if(global_settings.temp_unit != TEMP_UINT_C)
	{
		global_settings.temp_unit = TEMP_UINT_C;
		need_save_settings = true;
	}

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuTemp2Proc(void)
{
	if(global_settings.temp_unit != TEMP_UINT_F)
	{
		global_settings.temp_unit = TEMP_UINT_F;
		need_save_settings = true;
	}

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuDeviceProc(void)
{
	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuPgUpProc(void)
{
	uint8_t count;
	
	if(settings_menu.id == SETTINGS_MENU_MAIN)
		count = SETTINGS_MAIN_MENU_MAX_PER_PG;
	else
		count = SETTINGS_SUB_MENU_MAX_PER_PG;
	
	if(settings_menu.index < (settings_menu.count - count))
	{
		settings_menu.index += count;
		if(screen_id == SCREEN_ID_SETTINGS)
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuPgDownProc(void)
{
	uint8_t count;

	if(settings_menu.id == SETTINGS_MENU_MAIN)
		count = SETTINGS_MAIN_MENU_MAX_PER_PG;
	else
		count = SETTINGS_SUB_MENU_MAX_PER_PG;
	
	if(settings_menu.index >= count)
	{
		settings_menu.index -= count;
		if(screen_id == SCREEN_ID_SETTINGS)
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuPgLeftProc(void)
{
}

void SettingsMenuPgRightProc(void)
{
}

void SettingsMsgPorcess(void)
{
	if(need_save_time)
	{
		SaveSystemDateTime();
		need_save_time = false;
	}
	
	if(need_save_settings)
	{
		need_save_settings = false;
		
		switch(global_settings.language)
		{
	#ifndef FW_FOR_CN
	  #ifdef LANGUAGE_AR_ENABLE
		case LANGUAGE_AR:
			g_language_r2l = true;
			break;
	  #endif
	#endif		
		default:
			g_language_r2l = false;
			break;
		}
		
		SaveSystemSettings();
		SendSettingsData();
	}

	if(need_reset_settings)
	{
		need_reset_settings = false;
		k_timer_start(&reset_timer, K_MSEC(1000), K_NO_WAIT);
		ResetFactoryDefault();
	}
	if(need_reset_bk_level)
	{
		need_reset_bk_level = false;
		Set_Screen_Backlight_Level(global_settings.backlight_level);
	}
	if(reset_redraw_flag)
	{
		reset_redraw_flag = false;
		ResetUpdateStatus();
	}
	if(reset_start_flag)
	{
		reset_start_flag = false;
		k_timer_start(&reset_timer, K_MSEC(1000), K_NO_WAIT);
		ResetFactoryDefault();
	}
	if(reset_reboot_flag)
	{
		reset_reboot_flag = false;
		LCD_Clear(BLACK);
		sys_reboot(0);
	}
}

void EnterSettings(void)
{
	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));

	EnterSettingsScreen();
}
