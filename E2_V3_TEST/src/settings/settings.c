#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <fs/nvs.h>
#include <drivers/flash.h>
#include <dk_buttons_and_leds.h>
#include "screen.h"
#include "settings.h"
#include "datetime.h"
#include "alarm.h"
#include "lcd.h"
#include "codetrans.h"
#include "inner_flash.h"
#include "logger.h"

bool need_save_settings = false;
bool need_save_time = false;
bool need_reset_settings = false;
bool need_reset_bk_level = false;

uint8_t g_fw_version[64] = "V3.0.1_20221209";

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
static void SettingsMainMenu8Proc(void);
static void SettingsMenuLang1Proc(void);
static void SettingsMenuLang2Proc(void);
static void SettingsMenuLang3Proc(void);
static void SettingsMenuReset1Proc(void);
static void SettingsMenuReset2Proc(void);
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
	2022,
	1,
	1,
	0,
	0,
	0,
	6		//0=sunday
};

const global_settings_t FACTORY_DEFAULT_SETTINGS = 
{
	true,					//system inited flag
	false,					//heart rate turn on
	false,					//blood pressure turn on
	false,					//blood oxygen turn on		
	true,					//wake screen by wrist
	false,					//wrist off check
	0,						//target steps
	60,						//health interval
	TEMP_UINT_C,			//Centigrade
	TIME_FORMAT_24,			//24 format
	LANGUAGE_EN,			//language
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
	}
};

const settings_menu_t SETTING_MAIN_MENU = 
{
	SETTINGS_MENU_MAIN,
	0,
	8,
	{
		{
			{0x004c,0x0061,0x006e,0x0067,0x0075,0x0061,0x0067,0x0065,0x0000},//language
			{0x0046,0x0061,0x0063,0x0074,0x006f,0x0072,0x0079,0x0020,0x0064,0x0065,0x0066,0x0061,0x0075,0x006c,0x0074,0x0000},//Factory default
			{0x004f,0x0054,0x0041,0x0020,0x0055,0x0070,0x0064,0x0061,0x0074,0x0065,0x0000},//OTA Update
			{0x0042,0x0072,0x0069,0x0067,0x0068,0x0074,0x006e,0x0065,0x0073,0x0073,0x0000},//Brightness
			{0x0054,0x0065,0x006d,0x0070,0x0020,0x0064,0x0069,0x0073,0x0070,0x006c,0x0061,0x0079,0x0000},//Temp display
			{0x0044,0x0065,0x0076,0x0069,0x0063,0x0065,0x0020,0x0049,0x006e,0x0066,0x006f,0x0000},//Device info
			{0x0053,0x0049,0x004d,0x0020,0x0049,0x006e,0x0066,0x006f,0x0000},//SIM Info
			{0x0046,0x0057,0x0020,0x0049,0x006e,0x0066,0x006f,0x0000},//FW Info
		},
		{
			{0x0053,0x0070,0x0072,0x0061,0x0063,0x0068,0x0065,0x0000},//Sprache
			{0x0057,0x0065,0x0072,0x006B,0x0073,0x0065,0x0049,0x006E,0x0073,0x0074,0x0065,0x006C,0x006C,0x0075,0x006E,0x0067,0x0000},//Werkseinstellung
			{0x004f,0x0054,0x0041,0x0020,0x0055,0x0070,0x0064,0x0061,0x0074,0x0065,0x0000},//OTA Update
			{0x0048,0x0065,0x006C,0x006C,0x0069,0x0067,0x006B,0x0065,0x0069,0x0074,0x0000},//Helligkeit
			{0x0054,0x0065,0x006D,0x0070,0x0020,0x0041,0x006E,0x007A,0x0065,0x0069,0x0067,0x0065,0x0000},//Temp Anzeige
			{0x0047,0x0065,0x0072,0x00E4,0x0074,0x0065,0x0020,0x0049,0x006E,0x0066,0x006F,0x0000},//Ger?te Info
			{0x0053,0x0049,0x004d,0x0020,0x0049,0x006e,0x0066,0x006f,0x0000},//SIM Info
			{0x0046,0x0057,0x0020,0x0049,0x006e,0x0066,0x006f,0x0000},//FW Info
		},
		{
			{0x8BED,0x8A00,0x0000},//语言
			{0x6062,0x590D,0x51FA,0x5382,0x8BBE,0x7F6E,0x0000},//恢复出厂设置
			{0x004F,0x0054,0x0041,0x5347,0x7EA7,0x0000},//OTA升级
			{0x80CC,0x5149,0x4EAE,0x5EA6,0x0000},//背光亮度
			{0x4F53,0x6E29,0x663E,0x793A,0x0000},//体温显示
			{0x8BBE,0x5907,0x4FE1,0x606F,0x0000},//设备信息
			{0x0053,0x0049,0x004D,0x4FE1,0x606F,0x0000},//SIM信息
			{0x56FA,0x4EF6,0x4FE1,0x606F,0x0000},//固件信息
		},			
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
		SettingsMainMenu8Proc,
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
	3,
	{
		{
			{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
			{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
			{0x4E2D,0x6587,0x0000},//中文
		},
		{
			{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
			{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
			{0x4E2D,0x6587,0x0000},//中文
		},
		{
			{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
			{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
			{0x4E2D,0x6587,0x0000},//中文
		},		
	},
	{
		SettingsMenuLang1Proc,
		SettingsMenuLang2Proc,
		SettingsMenuLang3Proc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
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
		{
			{0x0000},
			{0x0000},
		},
		{
			{0x0000},
			{0x0000},
		},
		{
			{0x0000},
			{0x0000},
		},		
	},
	{
		SettingsMenuReset1Proc,
		SettingsMenuReset2Proc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
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
		{
			{0x0000},
		},
		{
			{0x0000},
		},
		{
			{0x0000},
		},		
	},
	{
		SettingsMenuOTAProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
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
		{
			{0x0000},
			{0x0000},
		},
		{
			{0x0000},
			{0x0000},
		},
		{
			{0x0000},
			{0x0000},
		},
	},	
	{
		SettingsMenuBrightness1Proc,
		SettingsMenuBrightness2Proc,
		SettingsMenuBrightness3Proc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,		
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
		{
			{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
			{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
		},
		{
			{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
			{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
		},
		{
			{0x6444,0x6C0F,0x5EA6,0x0000},//摄氏度
			{0x534E,0x6C0F,0x5EA6,0x0000},//华氏度
		},
	},
	{
		SettingsMenuTemp1Proc,
		SettingsMenuTemp2Proc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
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
		{
			{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:
			{0x0057,0x0049,0x0046,0x0049,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//WIFI MAC:
			{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		},
		{
			{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:
			{0x0057,0x0049,0x0046,0x0049,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//WIFI MAC:
			{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		},
		{
			{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:
			{0x0057,0x0049,0x0046,0x0049,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//WIFI MAC:
			{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		},		
	},
	{
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
		SettingsMenuDeviceProc,
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};

const settings_menu_t SETTING_MENU_SIM = 
{
	SETTINGS_MENU_SIM,
	0,
	2,
	{
		{
			{0x0049,0x004D,0x0053,0x0049,0x0020,0x004E,0x006F,0x003A,0x0000},//IMSI No:
			{0x0049,0x0043,0x0043,0x0049,0x0044,0x0020,0x004E,0x006F,0x003A,0x0000},//ICCID No:
		},
		{
			{0x0049,0x004D,0x0053,0x0049,0x0020,0x004E,0x006F,0x003A,0x0000},//IMSI No:
			{0x0049,0x0043,0x0043,0x0049,0x0044,0x0020,0x004E,0x006F,0x003A,0x0000},//ICCID No:
		},
		{
			{0x0049,0x004D,0x0053,0x0049,0x0020,0x004E,0x006F,0x003A,0x0000},//IMSI No:
			{0x0049,0x0043,0x0043,0x0049,0x0044,0x0020,0x004E,0x006F,0x003A,0x0000},//ICCID No:
		},
	},
	{
		SettingsMenuSIMProc,
		SettingsMenuSIMProc,
		SettingsMenuSIMProc,
		SettingsMenuSIMProc,
		SettingsMenuSIMProc,
		SettingsMenuSIMProc,
		SettingsMenuSIMProc,
		SettingsMenuSIMProc,
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
	},	
};

const settings_menu_t SETTING_MENU_FW = 
{
	SETTINGS_MENU_FW,
	0,
	5,
	{
		{
			{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
			{0x0057,0x0049,0x0046,0x0049,0x003A,0x0000},//WIFI:
			{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
			{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
			{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM: 
		},
		{
			{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
			{0x0057,0x0049,0x0046,0x0049,0x003A,0x0000},//WIFI:
			{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
			{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
			{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM: 
		},
		{
			{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
			{0x0057,0x0049,0x0046,0x0049,0x003A,0x0000},//WIFI:
			{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
			{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
			{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM: 
		},
	},	
	{
		SettingsMenuFWProc,
		SettingsMenuFWProc,
		SettingsMenuFWProc,
		SettingsMenuFWProc,
		SettingsMenuFWProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
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

	if(!global_settings.init)
	{
		memcpy(&global_settings, &FACTORY_DEFAULT_SETTINGS, sizeof(global_settings_t));
		SaveSystemSettings();
	}

	InitSystemDateTime();
	AlarmRemindInit();

	mmi_chset_init();
}

void ResetFactoryDefault(void)
{
	ResetSystemTime();
	ResetSystemSettings();

	clear_cur_local_in_record();
	clear_local_in_record();	
	clear_cur_health_in_record();
	clear_health_in_record();
#ifdef CONFIG_IMU_SUPPORT
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

#ifdef CONFIG_PPG_SUPPORT
	ClearAllHrRecData();
	ClearAllSpo2RecData();
	ClearAllBptRecData();
	sh_clear_bpt_cal_data();
#endif

#ifdef CONFIG_TEMP_SUPPORT
	ClearAllTempRecData();
#endif

	if(k_timer_remaining_get(&reset_timer) > 0)
		k_timer_stop(&reset_timer);

	g_reset_status = RESET_STATUS_SUCCESS;
	reset_redraw_flag = true;
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

	memcpy(&settings_menu, &SETTING_MENU_FACTORY_RESET, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}	
}

void SettingsMainMenu3Proc(void)
{
#ifdef CONFIG_FOTA_DOWNLOAD
	extern uint8_t g_new_fw_ver[64];

#ifdef NB_SIGNAL_TEST
	if(strcmp(g_new_fw_ver,g_fw_version) >= 0)
#else
	if(strcmp(g_new_fw_ver,g_fw_version) > 0)
#endif		
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

void SettingsMainMenu4Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_BRIGHTNESS, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu5Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_TEMP, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu6Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_DEVICE, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu7Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_SIM, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMainMenu8Proc(void)
{
	main_menu_index_bk = settings_menu.index;
	
	memcpy(&settings_menu, &SETTING_MENU_FW, sizeof(settings_menu_t));

	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuLang1Proc(void)
{
	global_settings.language = LANGUAGE_EN;
	need_save_settings = true;

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuLang2Proc(void)
{
	global_settings.language = LANGUAGE_DE;
	need_save_settings = true;

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuLang3Proc(void)
{
	global_settings.language = LANGUAGE_CHN;
	need_save_settings = true;

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
	global_settings.temp_unit = TEMP_UINT_C;
	need_save_settings = true;

	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuTemp2Proc(void)
{
	global_settings.temp_unit = TEMP_UINT_F;
	need_save_settings = true;

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

void SettingsMenuSIMProc(void)
{
	memcpy(&settings_menu, &SETTING_MAIN_MENU, sizeof(settings_menu_t));
	settings_menu.index = main_menu_index_bk;
	
	if(screen_id == SCREEN_ID_SETTINGS)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void SettingsMenuFWProc(void)
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
		SaveSystemSettings();
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
