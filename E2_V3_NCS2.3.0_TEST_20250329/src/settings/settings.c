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
		||defined(LANGUAGE_SE_ENABLE)\
		||defined(LANGUAGE_JP_ENABLE)\
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
#ifdef FW_FOR_CN	
	0,						//language
#else
	0,						//language
#endif
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

uint16_t MAIN_SEL_STR[][7][18] = 
{
#ifndef FW_FOR_CN
	{
		{0x004c,0x0061,0x006e,0x0067,0x0075,0x0061,0x0067,0x0065,0x0073,0x0000},//languages
		{0x0042,0x0072,0x0069,0x0067,0x0068,0x0074,0x006e,0x0065,0x0073,0x0073,0x0000},//Brightness
		{0x0054,0x0065,0x006d,0x0070,0x0020,0x0044,0x0069,0x0073,0x0070,0x006c,0x0061,0x0079,0x0000},//Temp display
		{0x0044,0x0065,0x0076,0x0069,0x0063,0x0065,0x0020,0x0049,0x006e,0x0066,0x006f,0x0000},//Device Info
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x0046,0x0061,0x0063,0x0074,0x006f,0x0072,0x0079,0x0020,0x0044,0x0065,0x0066,0x0061,0x0075,0x006c,0x0074,0x0000},//Factory Default
		{0x004f,0x0054,0x0041,0x0020,0x0055,0x0070,0x0064,0x0061,0x0074,0x0065,0x0000},//OTA Update
	},
	{
		{0x0053,0x0070,0x0072,0x0061,0x0063,0x0068,0x0065,0x006E,0x0000},//Sprachen
		{0x0048,0x0065,0x006C,0x006C,0x0069,0x0067,0x006B,0x0065,0x0069,0x0074,0x0000},//Helligkeit
		{0x0045,0x0069,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Einheit
		{0x0047,0x0065,0x0072,0x00E4,0x0074,0x0065,0x0020,0x0049,0x006E,0x0066,0x006F,0x0000},//Ger?te Info
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},////Caremate QR
		{0x0057,0x0065,0x0072,0x006B,0x0073,0x0072,0x0065,0x0073,0x0065,0x0074,0x0000},//Werksreset
		{0x004f,0x0054,0x0041,0x0020,0x0055,0x0070,0x0064,0x0061,0x0074,0x0065,0x0000},//OTA Update
	},
	{
		{0x004C,0x0061,0x006E,0x0067,0x0075,0x0065,0x0073,0x0000},//Langues
		{0x004C,0x0075,0x006D,0x0069,0x006E,0x006F,0x0073,0x0069,0x0074,0x00E9,0x0000},//Luminosit��
		{0x0055,0x006E,0x0069,0x0074,0x00E9,0x0000},//Unit��
		{0x0049,0x006E,0x0066,0x006F,0x0020,0x0061,0x0070,0x0070,0x0061,0x0072,0x0065,0x0069,0x006C,0x0000},//Info appareil
		{0x0051,0x0052,0x0020,0x0043,0x0061,0x0072,0x00E9,0x006D,0x0061,0x0074,0x0065,0x0000},//QR Car��mate
		{0x0055,0x0073,0x0069,0x006E,0x0065,0x0000},//Usine
		{0x004F,0x0054,0x0041,0x0000},//OTA
	},
	{
		{0x004C,0x0069,0x006E,0x0067,0x0075,0x0065,0x0000},//Lingue
		{0x004C,0x0075,0x006D,0x0069,0x006E,0x006F,0x0073,0x0069,0x0074,0x00E0,0x0000},//Luminosit��
		{0x0055,0x006E,0x0069,0x0074,0x00E0,0x0000},//Unit��
		{0x0049,0x006E,0x0066,0x006F,0x0020,0x0064,0x0069,0x0073,0x0070,0x002E,0x0000},//Info disp.
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},////Caremate QR
		{0x0046,0x0061,0x0062,0x0062,0x0072,0x0069,0x0063,0x0061,0x0000},//Fabbrica
		{0x004F,0x0054,0x0041,0x0000},//OTA
	},
	{
		{0x0049,0x0064,0x0069,0x006F,0x006D,0x0061,0x0073,0x0000},//Idiomas
		{0x0042,0x0072,0x0049,0x006C,0x006C,0x006F,0x0000},//Brillo
		{0x0055,0x006E,0x0069,0x0064,0x0061,0x0064,0x0000},//Unidad
		{0x0049,0x006E,0x0066,0x006F,0x0020,0x0064,0x0069,0x0073,0x0070,0x002E,0x0000},//Info disp.
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x0046,0x00E1,0x0062,0x0072,0x0069,0x0063,0x0061,0x0000},//F��brica
		{0x004F,0x0054,0x0041,0x0000},//OTA
	},
	{
		{0x0049,0x0064,0x0069,0x006F,0x006D,0x0061,0x0073,0x0000},//Idiomas
		{0x0042,0x0072,0x0069,0x006C,0x0068,0x006F,0x0000},//Brilho
		{0x0055,0x006E,0x0069,0x0064,0x0061,0x0064,0x0065,0x0000},//Unidade
		{0x0049,0x006E,0x0066,0x006F,0x0020,0x0064,0x0069,0x0073,0x0070,0x002E,0x0000},//Info disp.
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x0046,0x00E1,0x0062,0x0072,0x0069,0x0063,0x0061,0x0000},//F��brica
		{0x004F,0x0054,0x0041,0x0000},//OTA
	},
	{
		{0x004A,0x0119,0x007A,0x0079,0x006B,0x0069,0x0000},//J?zyki
		{0x004A,0x0061,0x0073,0x006E,0x006F,0x015B,0x0107,0x0000},//Jasno??
		{0x004A,0x0065,0x0064,0x006E,0x006F,0x0073,0x0074,0x006B,0x0061,0x0000},//Jednostka
		{0x0049,0x006E,0x0066,0x0020,0x006F,0x0020,0x0075,0x0072,0x007A,0x0105,0x0064,0x007A,0x0000},//Inf o urz?dz
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x0055,0x0073,0x0074,0x0061,0x0077,0x002E,0x0020,0x0066,0x0061,0x0062,0x0072,0x002E,0x0000},//Ustaw. fabr.
		{0x004F,0x0054,0x0041,0x0000},//OTA
	},
	{
		{0x0053,0x0070,0x0072,0x00E5,0x006B,0x0000},//Spr?k
		{0x004C,0x006A,0x0075,0x0073,0x0073,0x0074,0x0079,0x0072,0x006B,0x0061,0x0000},//Ljusstyrka
		{0x0045,0x006E,0x0068,0x0065,0x0074,0x0000},//Enhet
		{0x0045,0x006E,0x0068,0x0065,0x0074,0x0073,0x0069,0x006E,0x0066,0x006F,0x0000},//Enhetsinfo
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x0046,0x0061,0x0062,0x0072,0x002E,0x0020,0x00E5,0x0074,0x0065,0x0072,0x0073,0x0074,0x002E,0x0000},//Fabr. ?terst.
		{0x004F,0x0054,0x0041,0x0000},//OTA
	},
	{
		{0x8A00,0x8A9E,0x0000},//���Z
		{0x753B,0x9762,0x660E,0x5EA6,0x0000},//��������
		{0x4F53,0x6E29,0x8868,0x793A,0x0000},//���±�ʾ
		{0x30C7,0x30D0,0x30A4,0x30B9,0x60C5,0x5831,0x0000},//�ǥХ������
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x521D,0x671F,0x8A2D,0x5B9A,0x0000},//�����O��
		{0x004F,0x0054,0x0041,0x66F4,0x65B0,0x0000},//OTA����
	},
	{
		{0xC5B8,0xC5B4,0x0000},//??
		{0xD654,0xBA74,0x0020,0xBC1D,0xAE30,0x0000},//?? ??
		{0xCCB4,0xC628,0x0020,0xD45C,0xC2DC,0x0000},//?? ??
		{0xC7A5,0xCE58,0x0020,0xC815,0xBCF4,0x0000},//?? ??
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0xACF5,0xC7A5,0x0020,0xCD08,0xAE30,0xD654,0x0000},//?? ???
		{0x004F,0x0054,0x0041,0x0020,0xC5C5,0xADF8,0xB808,0xC774,0xB4DC,0x0000},//OTA ?????
	},
	{
		{0x042F,0x0437,0x044B,0x043A,0x0438,0x0000},//���٧�ܧ�
		{0x042F,0x0440,0x043A,0x043E,0x0441,0x0442,0x044C,0x0000},//����ܧ����
		{0x0415,0x0434,0x0438,0x043D,0x0438,0x0446,0x0430,0x0000},//���էڧߧڧ��
		{0x041E,0x0431,0x0020,0x0443,0x0441,0x0442,0x0440,0x043E,0x0439,0x0441,0x0442,0x0432,0x0435,0x0000},//���� ������ۧ��ӧ�
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x041F,0x043E,0x0020,0x0443,0x043C,0x043E,0x043B,0x0447,0x0430,0x043D,0x0438,0x044E,0x0000},//���� ��ާ�ݧ�ѧߧڧ�
		{0x004F,0x0054,0x0041,0x0000},//OTA
	},
	{
		{0x0627,0x0644,0x0644,0x063A,0x0627,0x062A,0x0000},//??????
		{0x0633,0x0637,0x0648,0x0639,0x0000},//????
		{0x062F,0x0631,0x062C,0x0629,0x0020,0x0627,0x0644,0x062D,0x0631,0x0627,0x0631,0x0629,0x00A0,0x0000},//???? ???????
		{0x0645,0x0639,0x0644,0x0648,0x0645,0x0627,0x062A,0x0020,0x062C,0x0647,0x0627,0x0632,0x0000},//??????? ????
		{0x0631,0x0645,0x0632,0x0020,0x0065,0x0074,0x0061,0x006D,0x0065,0x0072,0x0061,0x0043,0x0000},//Caremate ???
		{0x0639,0x062F,0x0627,0x062F,0x0627,0x062A,0x0020,0x0627,0x0644,0x0645,0x0635,0x0646,0x0639,0x0000},//?????? ??????
		{0x062A,0x062D,0x062F,0x064A,0x062B,0x0020,0x0041,0x0054,0x004F,0x0000},//OTA ?????
	},
#else
	{
		{0x8BED,0x8A00,0x0000},//����
		{0x5C4F,0x5E55,0x4EAE,0x5EA6,0x0000},//��Ļ����
		{0x4F53,0x6E29,0x663E,0x793A,0x0000},//������ʾ
		{0x8BBE,0x5907,0x4FE1,0x606F,0x0000},//�豸��Ϣ
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x4E8C,0x7EF4,0x7801,0x0000},//Caremate��ά��
		{0x6062,0x590D,0x51FA,0x5382,0x8BBE,0x7F6E,0x0000},//�ָ���������
		{0x004F,0x0054,0x0041,0x5347,0x7EA7,0x0000},//OTA����
	},
	{
		{0x004c,0x0061,0x006e,0x0067,0x0075,0x0061,0x0067,0x0065,0x0073,0x0000},//languages
		{0x0042,0x0072,0x0069,0x0067,0x0068,0x0074,0x006e,0x0065,0x0073,0x0073,0x0000},//Brightness
		{0x0054,0x0065,0x006d,0x0070,0x0020,0x0064,0x0069,0x0073,0x0070,0x006c,0x0061,0x0079,0x0000},//Temp display
		{0x0044,0x0065,0x0076,0x0069,0x0063,0x0065,0x0020,0x0049,0x006e,0x0066,0x006f,0x0000},//Device info
		{0x0043,0x0061,0x0072,0x0065,0x006D,0x0061,0x0074,0x0065,0x0020,0x0051,0x0052,0x0000},//Caremate QR
		{0x0046,0x0061,0x0063,0x0074,0x006f,0x0072,0x0079,0x0020,0x0064,0x0065,0x0066,0x0061,0x0075,0x006c,0x0074,0x0000},//Factory default
		{0x004f,0x0054,0x0041,0x0020,0x0055,0x0070,0x0064,0x0061,0x0074,0x0065,0x0000},//OTA Update
	},
#endif

};
const settings_menu_t SETTING_MAIN_MENU = 
{
	SETTINGS_MENU_MAIN,
	0,
	7,
	{
	#ifndef FW_FOR_CN
	  #ifdef LANGUAGE_EN_ENABLE
		{
			MAIN_SEL_STR[0][0],//language
			MAIN_SEL_STR[0][1],//Brightness
			MAIN_SEL_STR[0][2],//Temp display
			MAIN_SEL_STR[0][3],//Device info
			MAIN_SEL_STR[0][4],//Caremate QR
			MAIN_SEL_STR[0][5],//Factory default
			MAIN_SEL_STR[0][6],//OTA Update
		},
	  #endif
	  #ifdef LANGUAGE_DE_ENABLE
		{
			MAIN_SEL_STR[1][0],//Sprache
			MAIN_SEL_STR[1][1],//Helligkeit
			MAIN_SEL_STR[1][2],//Temp. Anzeige
			MAIN_SEL_STR[1][3],//Ger?te Info
			MAIN_SEL_STR[1][4],//Caremate QR
			MAIN_SEL_STR[1][5],//Werksreset
			MAIN_SEL_STR[1][6],//OTA Update
		},
	  #endif
	  #ifdef LANGUAGE_FR_ENABLE
		{
			MAIN_SEL_STR[2][0],//Langue
			MAIN_SEL_STR[2][1],//Luminosit��
			MAIN_SEL_STR[2][2],//Affichage temporaire
			MAIN_SEL_STR[2][3],//Info appareil
			MAIN_SEL_STR[2][4],//Caremate QR
			MAIN_SEL_STR[2][5],//Param��tre d'usine
			MAIN_SEL_STR[2][6],//Mise �� jour OTA
		},
	  #endif
	  #ifdef LANGUAGE_IT_ENABLE
		{
			MAIN_SEL_STR[3][0],//Lingua
			MAIN_SEL_STR[3][1],//Luminosit��
			MAIN_SEL_STR[3][2],//Unit�� di temp
			MAIN_SEL_STR[3][3],//Info disp.
			MAIN_SEL_STR[3][4],//Caremate QR
			MAIN_SEL_STR[3][5],//Impostazione di fabbrica
			MAIN_SEL_STR[3][6],//Aggiornamento OTA
		},
	  #endif
	  #ifdef LANGUAGE_ES_ENABLE
		{
			MAIN_SEL_STR[4][0],//Idioma
			MAIN_SEL_STR[4][1],//Luminosidad
			MAIN_SEL_STR[4][2],//Unidad Temp
			MAIN_SEL_STR[4][3],//Info disp.
			MAIN_SEL_STR[4][4],//Caremate QR
			MAIN_SEL_STR[4][5],//Ajustes de f��brica
			MAIN_SEL_STR[4][6],//Actualizaci��n OTA
		},
	  #endif
	  #ifdef LANGUAGE_PT_ENABLE
		{
			MAIN_SEL_STR[5][0],//Linguagem
			MAIN_SEL_STR[5][1],//Brilho
			MAIN_SEL_STR[5][2],//Unidade de temp
			MAIN_SEL_STR[5][3],//Info disp.
			MAIN_SEL_STR[5][4],////Caremate QR
			MAIN_SEL_STR[5][5],//Predefini??o de f��brica
			MAIN_SEL_STR[5][6],//Atualiza??o OTA
		},
	  #endif
	  #ifdef LANGUAGE_PL_ENABLE
		{
			MAIN_SEL_STR[6][0],//J?zyk
			MAIN_SEL_STR[6][1],//Jasno??
			MAIN_SEL_STR[6][2],//Wy?w. temp.
			MAIN_SEL_STR[6][3],//Inf o urz?dz
			MAIN_SEL_STR[6][4],//QR Opiekun
			MAIN_SEL_STR[6][5],//Ustaw. fabr.
			MAIN_SEL_STR[6][6],//Aktual. OTA
		},
	  #endif
	  #ifdef LANGUAGE_SE_ENABLE
		{
			MAIN_SEL_STR[7][0],//Spr?k
			MAIN_SEL_STR[7][1],//Ljusstyrka
			MAIN_SEL_STR[7][2],//Temp display
			MAIN_SEL_STR[7][3],//Enhetsinformation
			MAIN_SEL_STR[7][4],//Caremate QR
			MAIN_SEL_STR[7][5],//Fabr. ?terst.
			MAIN_SEL_STR[7][6],//OTA-uppdatering
		},
	  #endif
	  #ifdef LANGUAGE_JP_ENABLE
		{
			MAIN_SEL_STR[8][0],//���Z
			MAIN_SEL_STR[8][1],//��������
			MAIN_SEL_STR[8][2],//���±�ʾ
			MAIN_SEL_STR[8][3],//�ǥХ������
			MAIN_SEL_STR[8][4],//Caremate QR
			MAIN_SEL_STR[8][5],//�����O��
			MAIN_SEL_STR[8][6],//OTA����
		},
	  #endif
	  #ifdef LANGUAGE_KR_ENABLE
		{
			MAIN_SEL_STR[9][0],//??
			MAIN_SEL_STR[9][1],//??
			MAIN_SEL_STR[9][2],//?? ??
			MAIN_SEL_STR[9][3],//?? ??
			MAIN_SEL_STR[9][4],//????? QR
			MAIN_SEL_STR[9][5],//?? ???
			MAIN_SEL_STR[9][6],//OTA ????
		},
	  #endif
	  #ifdef LANGUAGE_RU_ENABLE
		{
			MAIN_SEL_STR[10][0],//���٧��
			MAIN_SEL_STR[10][1],//����ܧ����
			MAIN_SEL_STR[10][2],//�����ҧ�ѧا֧ߧڧ� ��֧ާ�֧�ѧ����
			MAIN_SEL_STR[10][3],//���ߧ���ާѧ�ڧ� ��� ������ۧ��ӧ�
			MAIN_SEL_STR[10][4],//���ѧ�֧ާѧ� QR
			MAIN_SEL_STR[10][5],//���ѧӧ�է�ܧڧ� �ߧѧ����ۧܧ� ��� ��ާ�ݧ�ѧߧڧ�
			MAIN_SEL_STR[10][6],//���ҧߧ�ӧݧ֧ߧڧ� OTA
		},
	  #endif
	  #ifdef LANGUAGE_AR_ENABLE
		{
			MAIN_SEL_STR[11][0],//???
			MAIN_SEL_STR[11][1],//????
			MAIN_SEL_STR[11][2],//??? ???? ???????
			MAIN_SEL_STR[11][3],//??????? ??????
			MAIN_SEL_STR[11][4],//?????? ??? ??
			MAIN_SEL_STR[11][5],//??????? ??????
			MAIN_SEL_STR[11][6],//????? ??? ??????
		},
	  #endif	
	#else
	  #ifdef LANGUAGE_CN_ENABLE
		{
			MAIN_SEL_STR[0][0],//����
			MAIN_SEL_STR[0][1],//��Ļ����
			MAIN_SEL_STR[0][2],//������ʾ
			MAIN_SEL_STR[0][3],//�豸��Ϣ
			MAIN_SEL_STR[0][4],//Caremate��ά��
			MAIN_SEL_STR[0][5],//�ָ���������
			MAIN_SEL_STR[0][6],//OTA����
		},
	  #endif
	  #ifdef LANGUAGE_EN_ENABLE
		{
			MAIN_SEL_STR[1][0],//language
			MAIN_SEL_STR[1][1],//Brightness
			MAIN_SEL_STR[1][2],//Temp display
			MAIN_SEL_STR[1][3],//Device info
			MAIN_SEL_STR[1][4],//Caremate QR
			MAIN_SEL_STR[1][5],//Factory default
			MAIN_SEL_STR[1][6],//OTA Update
		},
	  #endif	
	#endif
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

uint16_t LANGUAGE_SEL_STR[][12][10] =
{
#ifndef FW_FOR_CN
  	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
		{0x0044,0x0065,0x0075,0x0074,0x0073,0x0063,0x0068,0x0000},//Deutsch
		{0x0046,0x0072,0x0061,0x006E,0x00E7,0x0061,0x0069,0x0073,0x0000},//Fran?ais
		{0x0049,0x0074,0x0061,0x006C,0x0069,0x0061,0x006E,0x006F,0x0000},//Italiano
		{0x0045,0x0073,0x0070,0x0061,0x00F1,0x006F,0x006C,0x0000},//Espa?ol
		{0x0050,0x006F,0x0072,0x0074,0x0075,0x0067,0x0075,0x00EA,0x0073,0x0000},//Portugu��s
		{0x0050,0x006F,0x006C,0x0073,0x006B,0x0069,0x0000},//Polski
		{0x0053,0x0076,0x0065,0x006E,0x0073,0x006B,0x0061,0x0000},//Svenska
		{0x65E5,0x672C,0x8A9E,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0420,0x0443,0x0441,0x0441,0x043A,0x0438,0x0439,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
	{
		{0x0068,0x0073,0x0069,0x006C,0x0067,0x006E,0x0045,0x0000},//English
		{0x0068,0x0063,0x0073,0x0074,0x0075,0x0065,0x0044,0x0000},//Deutsch
		{0x0073,0x0069,0x0061,0x00E7,0x006E,0x0061,0x0072,0x0046,0x0000},//Fran?ais
		{0x006F,0x006E,0x0061,0x0069,0x006C,0x0061,0x0074,0x0049,0x0000},//Italiano
		{0x006C,0x006F,0x00F1,0x0061,0x0070,0x0073,0x0045,0x0000},//Espa?ol
		{0x0073,0x00EA,0x0075,0x0067,0x0075,0x0074,0x0072,0x006F,0x0050,0x0000},//Portugu��s
		{0x0069,0x006B,0x0073,0x006C,0x006F,0x0050,0x0000},//Polski
		{0x0061,0x006B,0x0073,0x006E,0x0065,0x0076,0x0053,0x0000},//Svenska
		{0x8A9E,0x672C,0x65E5,0x0000},//�ձ��Z
		{0xD55C,0xAD6D,0xC5B4,0x0000},//???
		{0x0439,0x0438,0x043A,0x0441,0x0441,0x0443,0x0420,0x0000},//������ܧڧ�
		{0x0627,0x0644,0x0639,0x0631,0x0628,0x064A,0x0629,0x0000},//???????
	},
#else
 	{
		{0x4E2D,0x6587,0x0000},//����
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
	},
 	{
		{0x4E2D,0x6587,0x0000},//����
		{0x0045,0x006E,0x0067,0x006C,0x0069,0x0073,0x0068,0x0000},//English
	},
#endif	
};

const settings_menu_t SETTING_MENU_LANGUAGE = 
{
	SETTINGS_MENU_LANGUAGE,
	0,
	LANGUAGE_MAX,
	{
	#ifndef FW_FOR_CN
	  #ifdef LANGUAGE_EN_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[0][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[0][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[0][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[0][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[0][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[0][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[0][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[0][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[0][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[0][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[0][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[0][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_DE_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[1][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[1][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[1][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[1][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[1][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[1][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[1][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[1][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[1][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[1][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[1][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[1][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_FR_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[2][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[2][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[2][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[2][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[2][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[2][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[2][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[2][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[2][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[2][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[2][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[2][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_IT_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[3][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[3][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[3][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[3][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[3][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[3][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[3][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[3][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[3][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[3][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[3][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[3][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_ES_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[4][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[4][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[4][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[4][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[4][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[4][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[4][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[4][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[4][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[4][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[4][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[4][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_PT_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[5][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[5][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[5][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[5][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[5][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[5][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[5][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[5][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[5][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[5][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[5][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[5][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_PL_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[6][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[6][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[6][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[6][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[6][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[6][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[6][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[6][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[6][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[6][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[6][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[6][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_SE_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[7][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[7][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[7][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[7][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[7][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[7][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[7][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[7][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[7][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[7][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[7][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[7][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_JP_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[8][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[8][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[8][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[8][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[8][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[8][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[8][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[8][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[8][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[8][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[8][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[8][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_KR_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[9][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[9][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[9][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[9][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[9][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[9][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[9][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[9][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[9][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[9][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[9][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[9][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_RU_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[10][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[10][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[10][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[10][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[10][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[10][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[10][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[10][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[10][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[10][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[10][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[10][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_AR_ENABLE
		{
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[11][0],//English
		#endif
		#ifdef LANGUAGE_DE_ENABLE
			LANGUAGE_SEL_STR[11][1],//Deutsch
		#endif
		#ifdef LANGUAGE_FR_ENABLE
			LANGUAGE_SEL_STR[11][2],//Fran?ais
		#endif
		#ifdef LANGUAGE_IT_ENABLE
			LANGUAGE_SEL_STR[11][3],//Italiano
		#endif
		#ifdef LANGUAGE_ES_ENABLE
			LANGUAGE_SEL_STR[11][4],//Espa?ol
		#endif
		#ifdef LANGUAGE_PT_ENABLE
			LANGUAGE_SEL_STR[11][5],//Portugu��s
		#endif
		#ifdef LANGUAGE_PL_ENABLE
			LANGUAGE_SEL_STR[11][6],//Polski
		#endif
		#ifdef LANGUAGE_SE_ENABLE
			LANGUAGE_SEL_STR[11][7],//Svenska
		#endif
		#ifdef LANGUAGE_JP_ENABLE
			LANGUAGE_SEL_STR[11][8],//�ձ��Z
		#endif
		#ifdef LANGUAGE_KR_ENABLE
			LANGUAGE_SEL_STR[11][9],//???
		#endif
		#ifdef LANGUAGE_RU_ENABLE
			LANGUAGE_SEL_STR[11][10],//������ܧڧ�
		#endif
		#ifdef LANGUAGE_AR_ENABLE
			LANGUAGE_SEL_STR[11][11],//�Ŧ˦˦Ǧͦɦ�?
		#endif	
		}
	  #endif  
	#else
	  #ifdef LANGUAGE_CN_ENABLE
		{
		#ifdef LANGUAGE_CN_ENABLE
			LANGUAGE_SEL_STR[0][0],//����
		#endif
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[0][1],//English
		#endif	
		},
	  #endif
	  #ifdef LANGUAGE_EN_ENABLE
		{
		#ifdef LANGUAGE_CN_ENABLE
			LANGUAGE_SEL_STR[1][0],//����
		#endif
		#ifdef LANGUAGE_EN_ENABLE
			LANGUAGE_SEL_STR[1][1],//English
		#endif	
		},
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
	},
	{	
		//page proc func
		SettingsMenuDumpProc,
		SettingsMenuDumpProc,
		SettingsMenuBrightness1Proc,
		SettingsMenuBrightness2Proc,		
	},
};

uint16_t TEMP_SEL_STR[][2][20] = 
{
#ifndef FW_FOR_CN
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x006A,0x0075,0x0073,0x007A,0x0061,0x0000},//Celsjusza
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
	{
		{0x6442,0x6C0F,0x5EA6,0x0000},//���϶�
		{0x83EF,0x6C0F,0x5EA6,0x0000},//�A�϶�
	},
	{
		{0xC12D,0xC528,0x0000},//??
		{0xD654,0xC528,0x0000},//??
	},
	{
		{0x0426,0x0435,0x043B,0x044C,0x0441,0x0438,0x0439,0x0000},//���֧ݧ��ڧ�
		{0x0424,0x0430,0x0440,0x0435,0x043D,0x0433,0x0435,0x0439,0x0442,0x0000},//���ѧ�֧ߧԧ֧ۧ�
	},
	{
		{0x0633,0x0650,0x0644,0x0633,0x064A,0x0648,0x0633,0x0000},//???????
		{0x0641,0x0647,0x0631,0x0646,0x0647,0x0627,0x064A,0x062A,0x0000},//????????
	},
#else
	{
		{0x6444,0x6C0F,0x5EA6,0x0000},//���϶�
		{0x534E,0x6C0F,0x5EA6,0x0000},//���϶�
	},
	{
		{0x0043,0x0065,0x006C,0x0073,0x0069,0x0075,0x0073,0x0000},//Celsius
		{0x0046,0x0061,0x0068,0x0072,0x0065,0x006E,0x0068,0x0065,0x0069,0x0074,0x0000},//Fahrenheit
	},
#endif
};

const settings_menu_t SETTING_MENU_TEMP = 
{
	SETTINGS_MENU_TEMP,
	0,
	2,
	{
	#ifndef FW_FOR_CN
	  #ifdef LANGUAGE_EN_ENABLE
		{
			TEMP_SEL_STR[0][0],//Celsius
			TEMP_SEL_STR[0][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_DE_ENABLE
		{
			TEMP_SEL_STR[1][0],//Celsius
			TEMP_SEL_STR[1][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_FR_ENABLE
		{
			TEMP_SEL_STR[2][0],//Celsius
			TEMP_SEL_STR[2][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_IT_ENABLE
		{
			TEMP_SEL_STR[3][0],//Celsius
			TEMP_SEL_STR[3][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_ES_ENABLE
		{
			TEMP_SEL_STR[4][0],//Celsius
			TEMP_SEL_STR[4][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_PT_ENABLE
		{
			TEMP_SEL_STR[5][0],//Celsius
			TEMP_SEL_STR[5][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_PL_ENABLE
		{
			TEMP_SEL_STR[6][0],//Celsjusza
			TEMP_SEL_STR[6][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_SE_ENABLE
		{
			TEMP_SEL_STR[7][0],//Celsius
			TEMP_SEL_STR[7][1],//Fahrenheit
		},
	  #endif
	  #ifdef LANGUAGE_JP_ENABLE
		{
			TEMP_SEL_STR[8][0],//���϶�
			TEMP_SEL_STR[8][1],//�A�϶�
		},
	  #endif
	  #ifdef LANGUAGE_KR_ENABLE
		{
			TEMP_SEL_STR[9][0],//??
			TEMP_SEL_STR[9][1],//??
		},
	  #endif
	  #ifdef LANGUAGE_RU_ENABLE
		{
			TEMP_SEL_STR[10][0],//���֧ݧ��ڧ�
			TEMP_SEL_STR[10][1],//��� ���ѧ�֧ߧԧ֧ۧ��
		},
	  #endif
	  #ifdef LANGUAGE_AR_ENABLE
		{
			TEMP_SEL_STR[11][0],//���Ŧ˦�?�Ϧ�
			TEMP_SEL_STR[11][1],//�ȦŦѦ�?�̦ŦӦѦ� �����ѦŦ�?�ɦ�
		},
	  #endif
	#else
	  #ifdef LANGUAGE_CN_ENABLE
		{
			TEMP_SEL_STR[0][0],//���϶�
			TEMP_SEL_STR[0][1],//���϶�
		},
	  #endif
	  #ifdef LANGUAGE_EN_ENABLE
		{
			TEMP_SEL_STR[1][0],//Celsius
			TEMP_SEL_STR[1][1],//Fahrenheit
		},
	  #endif
	#endif
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

uint16_t DEVICE_SEL_STR[][9][11] = 
{
#ifndef FW_FOR_CN
  #ifdef CONFIG_FACTORY_TEST_SUPPORT
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
 	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x0020,0x004E,0x006F,0x002E,0x003A,0x0000},//IMSI No.:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
 	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
 	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
  #else/*CONFIG_FACTORY_TEST_SUPPORT*/
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
  #endif/*CONFIG_FACTORY_TEST_SUPPORT*/
#else/*FW_FOR_CN*/
  #ifdef CONFIG_FACTORY_TEST_SUPPORT
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000},//ICCID:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
		{0x004D,0x004F,0x0044,0x0045,0x004D,0x003A,0x0000},//MODEM:
		{0x0050,0x0050,0x0047,0x003A,0x0000},//PPG:
		{0x0057,0x0069,0x0046,0x0069,0x003A,0x0000},//WiFi:
		{0x0042,0x004C,0x0045,0x003A,0x0000},//BLE:
		{0x0042,0x004C,0x0045,0x0020,0x004D,0x0041,0x0043,0x003A,0x0000},//BLE MAC:						
	},
  #else/*CONFIG_FACTORY_TEST_SUPPORT*/
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
	{
		{0x0049,0x004D,0x0045,0x0049,0x003A,0x0000},//IMEI:
		{0x0049,0x004D,0x0053,0x0049,0x003A,0x0000},//IMSI:
		{0x004D,0x0043,0x0055,0x003A,0x0000},//MCU:
	},
  #endif/*CONFIG_FACTORY_TEST_SUPPORT*/
#endif/*FW_FOR_CN*/
};

const settings_menu_t SETTING_MENU_DEVICE = 
{
	SETTINGS_MENU_DEVICE,
	0,
#ifdef CONFIG_FACTORY_TEST_SUPPORT
	9,
#else
	3,
#endif	
	{
	#ifndef FW_FOR_CN
	  #ifdef CONFIG_FACTORY_TEST_SUPPORT
	   #ifdef LANGUAGE_EN_ENABLE
		{
			DEVICE_SEL_STR[0][0],//IMEI:
			DEVICE_SEL_STR[0][1],//IMSI No.:
			DEVICE_SEL_STR[0][2],//ICCID No.:
			DEVICE_SEL_STR[0][3],//MCU:
			DEVICE_SEL_STR[0][4],//MODEM:
			DEVICE_SEL_STR[0][5],//PPG:
			DEVICE_SEL_STR[0][6],//WiFi:
			DEVICE_SEL_STR[0][7],//BLE:
			DEVICE_SEL_STR[0][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_DE_ENABLE
		{
			DEVICE_SEL_STR[1][0],//IMEI:
			DEVICE_SEL_STR[1][1],//IMSI No.:
			DEVICE_SEL_STR[1][2],//ICCID No.:
			DEVICE_SEL_STR[1][3],//MCU:
			DEVICE_SEL_STR[1][4],//MODEM:
			DEVICE_SEL_STR[1][5],//PPG:
			DEVICE_SEL_STR[1][6],//WiFi:
			DEVICE_SEL_STR[1][7],//BLE:
			DEVICE_SEL_STR[1][8],//BLE MAC:	
		},
	   #endif
	   #ifdef LANGUAGE_FR_ENABLE
		{
			DEVICE_SEL_STR[2][0],//IMEI:
			DEVICE_SEL_STR[2][1],//IMSI No.:
			DEVICE_SEL_STR[2][2],//ICCID No.:
			DEVICE_SEL_STR[2][3],//MCU:
			DEVICE_SEL_STR[2][4],//MODEM:
			DEVICE_SEL_STR[2][5],//PPG:
			DEVICE_SEL_STR[2][6],//WiFi:
			DEVICE_SEL_STR[2][7],//BLE:
			DEVICE_SEL_STR[2][8],//BLE MAC:	
		},
	   #endif
	   #ifdef LANGUAGE_IT_ENABLE
		{
			DEVICE_SEL_STR[3][0],//IMEI:
			DEVICE_SEL_STR[3][1],//IMSI No.:
			DEVICE_SEL_STR[3][2],//ICCID No.:
			DEVICE_SEL_STR[3][3],//MCU:
			DEVICE_SEL_STR[3][4],//MODEM:
			DEVICE_SEL_STR[3][5],//PPG:
			DEVICE_SEL_STR[3][6],//WiFi:
			DEVICE_SEL_STR[3][7],//BLE:
			DEVICE_SEL_STR[3][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_ES_ENABLE
		{
			DEVICE_SEL_STR[4][0],//IMEI:
			DEVICE_SEL_STR[4][1],//IMSI No.:
			DEVICE_SEL_STR[4][2],//ICCID No.:
			DEVICE_SEL_STR[4][3],//MCU:
			DEVICE_SEL_STR[4][4],//MODEM:
			DEVICE_SEL_STR[4][5],//PPG:
			DEVICE_SEL_STR[4][6],//WiFi:
			DEVICE_SEL_STR[4][7],//BLE:
			DEVICE_SEL_STR[4][8],//BLE MAC:					
		},
	   #endif
	   #ifdef LANGUAGE_PT_ENABLE
		{
			DEVICE_SEL_STR[5][0],//IMEI:
			DEVICE_SEL_STR[5][1],//IMSI No.:
			DEVICE_SEL_STR[5][2],//ICCID No.:
			DEVICE_SEL_STR[5][3],//MCU:
			DEVICE_SEL_STR[5][4],//MODEM:
			DEVICE_SEL_STR[5][5],//PPG:
			DEVICE_SEL_STR[5][6],//WiFi:
			DEVICE_SEL_STR[5][7],//BLE:
			DEVICE_SEL_STR[5][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_PL_ENABLE
		{
			DEVICE_SEL_STR[6][0],//IMEI:
			DEVICE_SEL_STR[6][1],//IMSI No.:
			DEVICE_SEL_STR[6][2],//ICCID No.:
			DEVICE_SEL_STR[6][3],//MCU:
			DEVICE_SEL_STR[6][4],//MODEM:
			DEVICE_SEL_STR[6][5],//PPG:
			DEVICE_SEL_STR[6][6],//WiFi:
			DEVICE_SEL_STR[6][7],//BLE:
			DEVICE_SEL_STR[6][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_SE_ENABLE
		{
			DEVICE_SEL_STR[7][0],//IMEI:
			DEVICE_SEL_STR[7][1],//IMSI No.:
			DEVICE_SEL_STR[7][2],//ICCID No.:
			DEVICE_SEL_STR[7][3],//MCU:
			DEVICE_SEL_STR[7][4],//MODEM:
			DEVICE_SEL_STR[7][5],//PPG:
			DEVICE_SEL_STR[7][6],//WiFi:
			DEVICE_SEL_STR[7][7],//BLE:
			DEVICE_SEL_STR[7][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_JP_ENABLE
		{
			DEVICE_SEL_STR[8][0],//IMEI:
			DEVICE_SEL_STR[8][1],//IMSI No.:
			DEVICE_SEL_STR[8][2],//ICCID No.:
			DEVICE_SEL_STR[8][3],//MCU:
			DEVICE_SEL_STR[8][4],//MODEM:
			DEVICE_SEL_STR[8][5],//PPG:
			DEVICE_SEL_STR[8][6],//WiFi:
			DEVICE_SEL_STR[8][7],//BLE:
			DEVICE_SEL_STR[8][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_KR_ENABLE
		{
			DEVICE_SEL_STR[9][0],//IMEI:
			DEVICE_SEL_STR[9][1],//IMSI No.:
			DEVICE_SEL_STR[9][2],//ICCID No.:
			DEVICE_SEL_STR[9][3],//MCU:
			DEVICE_SEL_STR[9][4],//MODEM:
			DEVICE_SEL_STR[9][5],//PPG:
			DEVICE_SEL_STR[9][6],//WiFi:
			DEVICE_SEL_STR[9][7],//BLE:
			DEVICE_SEL_STR[9][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_RU_ENABLE
		{
			DEVICE_SEL_STR[10][0],//IMEI:
			DEVICE_SEL_STR[10][1],//IMSI No.:
			DEVICE_SEL_STR[10][2],//ICCID No.:
			DEVICE_SEL_STR[10][3],//MCU:
			DEVICE_SEL_STR[10][4],//MODEM:
			DEVICE_SEL_STR[10][5],//PPG:
			DEVICE_SEL_STR[10][6],//WiFi:
			DEVICE_SEL_STR[10][7],//BLE:
			DEVICE_SEL_STR[10][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_AR_ENABLE
		{
			DEVICE_SEL_STR[11][0],//IMEI:
			DEVICE_SEL_STR[11][1],//IMSI No.:
			DEVICE_SEL_STR[11][2],//ICCID No.:
			DEVICE_SEL_STR[11][3],//MCU:
			DEVICE_SEL_STR[11][4],//MODEM:
			DEVICE_SEL_STR[11][5],//PPG:
			DEVICE_SEL_STR[11][6],//WiFi:
			DEVICE_SEL_STR[11][7],//BLE:
			DEVICE_SEL_STR[11][8],//BLE MAC:						
		},
	   #endif
	  #else
	   #ifdef LANGUAGE_EN_ENABLE
		{
			DEVICE_SEL_STR[0][0],//IMEI:
			DEVICE_SEL_STR[0][1],//IMSI No.:
			DEVICE_SEL_STR[0][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_DE_ENABLE
		{
			DEVICE_SEL_STR[1][0],//IMEI:
			DEVICE_SEL_STR[1][1],//IMSI No.:
			DEVICE_SEL_STR[1][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_FR_ENABLE
		{
			DEVICE_SEL_STR[2][0],//IMEI:
			DEVICE_SEL_STR[2][1],//IMSI No.:
			DEVICE_SEL_STR[2][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_IT_ENABLE
		{
			DEVICE_SEL_STR[3][0],//IMEI:
			DEVICE_SEL_STR[3][1],//IMSI No.:
			DEVICE_SEL_STR[3][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_ES_ENABLE
		{
			DEVICE_SEL_STR[4][0],//IMEI:
			DEVICE_SEL_STR[4][1],//IMSI No.:
			DEVICE_SEL_STR[4][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_PT_ENABLE
		{
			DEVICE_SEL_STR[5][0],//IMEI:
			DEVICE_SEL_STR[5][1],//IMSI No.:
			DEVICE_SEL_STR[5][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_PL_ENABLE
		{
			DEVICE_SEL_STR[6][0],//IMEI:
			DEVICE_SEL_STR[6][1],//IMSI No.:
			DEVICE_SEL_STR[6][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_SE_ENABLE
		{
			DEVICE_SEL_STR[7][0],//IMEI:
			DEVICE_SEL_STR[7][1],//IMSI No.:
			DEVICE_SEL_STR[7][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_JP_ENABLE
		{
			DEVICE_SEL_STR[8][0],//IMEI:
			DEVICE_SEL_STR[8][1],//IMSI No.:
			DEVICE_SEL_STR[8][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_KR_ENABLE
		{
			DEVICE_SEL_STR[9][0],//IMEI:
			DEVICE_SEL_STR[9][1],//IMSI No.:
			DEVICE_SEL_STR[9][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_RU_ENABLE
		{
			DEVICE_SEL_STR[10][0],//IMEI:
			DEVICE_SEL_STR[10][1],//IMSI No.:
			DEVICE_SEL_STR[10][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_AR_ENABLE
		{
			DEVICE_SEL_STR[11][0],//IMEI:
			DEVICE_SEL_STR[11][1],//IMSI No.:
			DEVICE_SEL_STR[11][2],//MCU:
		},
	   #endif
	  #endif/*CONFIG_FACTORY_TEST_SUPPORT*/
	#else
	  #ifdef CONFIG_FACTORY_TEST_SUPPORT
	   #ifdef LANGUAGE_CN_ENABLE
		{
			DEVICE_SEL_STR[0][0],//IMEI:
			DEVICE_SEL_STR[0][1],//IMSI No.:
			DEVICE_SEL_STR[0][2],//ICCID No.:
			DEVICE_SEL_STR[0][3],//MCU:
			DEVICE_SEL_STR[0][4],//MODEM:
			DEVICE_SEL_STR[0][5],//PPG:
			DEVICE_SEL_STR[0][6],//WiFi:
			DEVICE_SEL_STR[0][7],//BLE:
			DEVICE_SEL_STR[0][8],//BLE MAC:						
		},
	   #endif
	   #ifdef LANGUAGE_EN_ENABLE
		{
			DEVICE_SEL_STR[1][0],//IMEI:
			DEVICE_SEL_STR[1][1],//IMSI No.:
			DEVICE_SEL_STR[1][2],//ICCID No.:
			DEVICE_SEL_STR[1][3],//MCU:
			DEVICE_SEL_STR[1][4],//MODEM:
			DEVICE_SEL_STR[1][5],//PPG:
			DEVICE_SEL_STR[1][6],//WiFi:
			DEVICE_SEL_STR[1][7],//BLE:
			DEVICE_SEL_STR[1][8],//BLE MAC:						
		},
	   #endif	
	  #else
	   #ifdef LANGUAGE_CN_ENABLE
		{
			DEVICE_SEL_STR[0][0],//IMEI:
			DEVICE_SEL_STR[0][1],//IMSI No.:
			DEVICE_SEL_STR[0][2],//MCU:
		},
	   #endif
	   #ifdef LANGUAGE_EN_ENABLE
		{
			DEVICE_SEL_STR[1][0],//IMEI:
			DEVICE_SEL_STR[1][1],//IMSI No.:
			DEVICE_SEL_STR[1][2],//MCU:
		},
	   #endif
	  #endif/*CONFIG_FACTORY_TEST_SUPPORT*/
	#endif
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
#if defined(CONFIG_FOTA_DOWNLOAD)&&!defined(CONFIG_FACTORY_TEST_SUPPORT)
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
	if(global_settings.language != 0+settings_menu.index)
	{
		global_settings.language = 0+settings_menu.index;
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
	if(global_settings.language != 1+settings_menu.index)
	{
		global_settings.language = 1+settings_menu.index;
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
	if(global_settings.language != 2+settings_menu.index)
	{
		global_settings.language = 2+settings_menu.index;
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
