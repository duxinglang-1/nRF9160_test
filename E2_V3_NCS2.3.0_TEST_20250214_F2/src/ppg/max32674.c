/****************************************Copyright (c)************************************************
** File Name:			    max32674.c
** Descriptions:			PPG process source file
** Created By:				xie biao
** Created Date:			2021-05-19
** Modified Date:      		2021-05-19 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/random/rand32.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <nrf_socket.h>
#include <nrfx.h>
#include "uart_ble.h"
#include "Max32674.h"
#include "screen.h"
#include "max_sh_interface.h"
#include "max_sh_api.h"
#include "inner_flash.h"
#include "external_flash.h"
#include "logger.h"

//#define PPG_DEBUG

#define PPG_HR_COUNT_MAX		10
#define PPG_HR_DEL_MIN_NUM		6
#define PPG_SPO2_COUNT_MAX		3
#define PPG_SPO2_DEL_MIN_NUM	1
#define PPG_SCC_COUNT_MAX		5

bool ppg_int_event = false;
bool ppg_bpt_is_calbraed = false;
bool ppg_bpt_cal_need_update = false;
bool get_bpt_ok_flag = false;
bool get_hr_ok_flag = false;
bool get_spo2_ok_flag = false;
bool ppg_skin_contacted_flag = false;

sys_date_timer_t g_health_check_time = {0};
PPG_WORK_STATUS g_ppg_status = PPG_STATUS_PREPARE;

static bool ppg_appmode_init_flag = false;

static bool ppg_fw_upgrade_flag = false;
static bool ppg_delay_start_flag = false;
static bool ppg_start_flag = false;
static bool ppg_test_flag = false;
static bool ppg_stop_flag = false;
static bool ppg_get_data_flag = false;
static bool ppg_redraw_data_flag = false;
static bool ppg_get_cal_flag = false;
static bool ppg_stop_cal_flag = false;
static bool menu_start_hr = false;
static bool menu_start_spo2 = false;
static bool menu_start_bpt = false;
#ifdef CONFIG_FACTORY_TEST_SUPPORT
static bool ft_start_hr = false;
#endif

uint8_t ppg_test_info[256] = {0};

uint8_t ppg_power_flag = 0;	//0:�ر� 1:�������� 2:�����ɹ�
static uint8_t whoamI=0, rst=0;

uint8_t g_ppg_trigger = 0;
uint8_t g_ppg_data = PPG_DATA_MAX;
uint8_t g_ppg_alg_mode = ALG_MODE_HR_SPO2;
uint8_t g_ppg_bpt_status = BPT_STATUS_GET_EST;
uint8_t g_ppg_ver[64] = {0};

uint8_t g_hr = 0;
uint8_t g_hr_menu = 0;
uint8_t g_hr_hourly = 0;
uint8_t g_spo2 = 0;
uint8_t g_spo2_menu = 0;
uint8_t g_spo2_hourly = 0;
bpt_data g_bpt = {0};
bpt_data g_bpt_menu = {0};
bpt_data g_bpt_hourly = {0};

static uint8_t scc_check_sum = 0;
static uint8_t SCC_COMPARE_MAX = PPG_SCC_COUNT_MAX;

static uint8_t temp_hr_count = 0;
static uint8_t temp_spo2_count = 0;
static uint8_t temp_hr[PPG_HR_COUNT_MAX] = {0};
static uint8_t temp_spo2[PPG_SPO2_COUNT_MAX] = {0};
static uint8_t rec2buf[PPG_BPT_REC2_DATA_SIZE] = {0};
static uint8_t databuf[PPG_BPT_REC2_DATA_SIZE] = {0};

static uint16_t str_timing_note[LANGUAGE_MAX][90] = {
													#ifndef FW_FOR_CN
													  #ifdef LANGUAGE_EN_ENABLE
														{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
													  #endif
													  #ifdef LANGUAGE_DE_ENABLE
														{0x0044,0x0069,0x0065,0x0020,0x005A,0x0065,0x0069,0x0074,0x006D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x0065,0x0074,0x0020,0x006B,0x0075,0x0072,0x007A,0x0020,0x0076,0x006F,0x0072,0x0020,0x0064,0x0065,0x006D,0x0020,0x0053,0x0074,0x0061,0x0072,0x0074,0x0020,0x0075,0x006E,0x0064,0x0020,0x0064,0x0069,0x0065,0x0073,0x0065,0x0020,0x004D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0069,0x0073,0x0074,0x0020,0x0076,0x006F,0x0072,0x0062,0x0065,0x0069,0x0021,0x0000},//Die Zeitmessung startet kurz vor dem Start und diese Messung ist vorbei!
													  #endif
													  #ifdef LANGUAGE_FR_ENABLE
														{0x004C,0x0061,0x0020,0x006D,0x0065,0x0073,0x0075,0x0072,0x0065,0x0020,0x0064,0x0075,0x0020,0x0073,0x0079,0x006E,0x0063,0x0068,0x0072,0x006F,0x006E,0x0069,0x0073,0x0061,0x0074,0x0069,0x006F,0x006E,0x0020,0x0065,0x0073,0x0074,0x0020,0x0073,0x0075,0x0072,0x0020,0x006C,0x0065,0x0020,0x0070,0x006F,0x0069,0x006E,0x0074,0x0020,0x0064,0x0065,0x0020,0x0063,0x006F,0x006D,0x006D,0x0065,0x006E,0x0063,0x0065,0x0072,0x002C,0x0020,0x0065,0x0074,0x0020,0x0063,0x0065,0x0074,0x0074,0x0065,0x0020,0x006D,0x0065,0x0073,0x0075,0x0072,0x0065,0x0020,0x0065,0x0073,0x0074,0x0020,0x0074,0x0065,0x0072,0x006D,0x0069,0x006E,0x00E9,0x0065,0x0021,0x0000},//La mesure du synchronisation est sur le point de commencer, et cette mesure est termin��e!
													  #endif
													  #ifdef LANGUAGE_IT_ENABLE
														{0x004C,0x0061,0x0020,0x006D,0x0069,0x0073,0x0075,0x0072,0x0061,0x007A,0x0069,0x006F,0x006E,0x0065,0x0020,0x0064,0x0065,0x0069,0x0020,0x0074,0x0065,0x006D,0x0070,0x0069,0x0020,0x0073,0x0074,0x0061,0x0020,0x0070,0x0065,0x0072,0x0020,0x0069,0x006E,0x0069,0x007A,0x0069,0x0061,0x0072,0x0065,0x0020,0x0065,0x0020,0x0071,0x0075,0x0065,0x0073,0x0074,0x0061,0x0020,0x006D,0x0069,0x0073,0x0075,0x0072,0x0061,0x007A,0x0069,0x006F,0x006E,0x0065,0x0020,0x00E8,0x0020,0x0066,0x0069,0x006E,0x0069,0x0074,0x0061,0x0021,0x0000},//La misurazione dei tempi sta per iniziare e questa misurazione �� finita!
													  #endif
													  #ifdef LANGUAGE_ES_ENABLE
														{0x004C,0x0061,0x0020,0x006D,0x0065,0x0064,0x0069,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0064,0x0065,0x0020,0x0074,0x0069,0x0065,0x006D,0x0070,0x006F,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0061,0x0020,0x0070,0x0075,0x006E,0x0074,0x006F,0x0020,0x0064,0x0065,0x0020,0x0063,0x006F,0x006D,0x0065,0x006E,0x007A,0x0061,0x0072,0x002C,0x0020,0x00A1,0x0079,0x0020,0x0065,0x0073,0x0074,0x0061,0x0020,0x006D,0x0065,0x0064,0x0069,0x0063,0x0069,0x00F3,0x006E,0x0020,0x0068,0x0061,0x0020,0x0074,0x0065,0x0072,0x006D,0x0069,0x006E,0x0061,0x0064,0x006F,0x0021,0x0000},//La medici��n de tiempo est�� a punto de comenzar, ?y esta medici��n ha terminado!
													  #endif
													  #ifdef LANGUAGE_PT_ENABLE
														{0x0041,0x0020,0x006D,0x0065,0x0064,0x0069,0x00E7,0x00E3,0x006F,0x0020,0x0064,0x0065,0x0020,0x0074,0x0065,0x006D,0x0070,0x006F,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0070,0x0072,0x0065,0x0073,0x0074,0x0065,0x0073,0x0020,0x0061,0x0020,0x0063,0x006F,0x006D,0x0065,0x00E7,0x0061,0x0072,0x002C,0x0020,0x0065,0x0020,0x0065,0x0073,0x0073,0x0061,0x0020,0x006D,0x0065,0x0064,0x0069,0x00E7,0x00E3,0x006F,0x0020,0x0061,0x0063,0x0061,0x0062,0x006F,0x0075,0x0021,0x0000},//A medi??o de tempo est�� prestes a come?ar, e essa medi??o acabou!
													  #endif
													  #ifdef LANGUAGE_PL_ENABLE
														{0x0050,0x006F,0x006D,0x0069,0x0061,0x0072,0x0020,0x0063,0x007A,0x0061,0x0073,0x0075,0x0020,0x007A,0x0061,0x0072,0x0061,0x007A,0x0020,0x0073,0x0069,0x0119,0x0020,0x0072,0x006F,0x007A,0x0070,0x006F,0x0063,0x007A,0x006E,0x0069,0x0065,0x0020,0x0069,0x0020,0x0064,0x006F,0x0062,0x0069,0x0065,0x0067,0x0142,0x0020,0x006B,0x006F,0x0144,0x0063,0x0061,0x0021,0x0000},//Pomiar czasu zaraz si? rozpocznie i dobieg? ko��ca!
													  #endif
													  #ifdef LANGUAGE_SE_ENABLE
														{0x0054,0x0069,0x0064,0x0073,0x006D,0x00E4,0x0074,0x006E,0x0069,0x006E,0x0067,0x0065,0x006E,0x0020,0x00E4,0x0072,0x0020,0x0070,0x00E5,0x0020,0x0076,0x00E4,0x0067,0x0020,0x0061,0x0074,0x0074,0x0020,0x0062,0x00F6,0x0072,0x006A,0x0061,0x002C,0x0020,0x006F,0x0063,0x0068,0x0020,0x0064,0x0065,0x006E,0x006E,0x0061,0x0020,0x006D,0x00E4,0x0074,0x006E,0x0069,0x006E,0x0067,0x0020,0x00E4,0x0072,0x0020,0x00F6,0x0076,0x0065,0x0072,0x0021,0x0000},//Tidsm?tningen ?r p? v?g att b?rja, och denna m?tning ?r ?ver!
													  #endif
													  #ifdef LANGUAGE_JP_ENABLE
														{0x3053,0x308C,0x304B,0x3089,0x30BF,0x30A4,0x30DF,0x30F3,0x30B0,0x8A08,0x6E2C,0x304C,0x59CB,0x307E,0x308A,0x3001,0x4ECA,0x56DE,0x306E,0x8A08,0x6E2C,0x306F,0x7D42,0x4E86,0x3067,0x3059,0xFF01,0x0000},//���줫�饿���ߥ�Ӌ�y��ʼ�ޤꡢ��ؤ�Ӌ�y�ϽK�ˤǤ���
													  #endif
													  #ifdef LANGUAGE_KR_ENABLE
														{0xD0C0,0xC774,0xBC0D,0x0020,0xCE21,0xC815,0xC774,0x0020,0xACE7,0x0020,0xC2DC,0xC791,0xB418,0xB824,0x0020,0xD558,0xACE0,0x002C,0x0020,0xC774,0x0020,0xCE21,0xC815,0xC740,0x0020,0xB05D,0xB0AC,0xC2B5,0xB2C8,0xB2E4,0x0021,0x0000},//??? ??? ? ???? ??, ? ??? ?????!
													  #endif
													  #ifdef LANGUAGE_RU_ENABLE
														{0x0418,0x0437,0x043C,0x0435,0x0440,0x0435,0x043D,0x0438,0x0435,0x0020,0x0432,0x0440,0x0435,0x043C,0x0435,0x043D,0x0438,0x0020,0x0432,0x043E,0x0442,0x2013,0x0432,0x043E,0x0442,0x0020,0x043D,0x0430,0x0447,0x043D,0x0435,0x0442,0x0441,0x044F,0x002C,0x0020,0x0438,0x0020,0x044D,0x0442,0x043E,0x0020,0x0438,0x0437,0x043C,0x0435,0x0440,0x0435,0x043D,0x0438,0x0435,0x0020,0x0443,0x0436,0x0435,0x0020,0x0437,0x0430,0x043A,0x043E,0x043D,0x0447,0x0435,0x043D,0x043E,0x0021,0x0000},//���٧ާ֧�֧ߧڧ� �ӧ�֧ާ֧ߧ� �ӧ��-�ӧ�� �ߧѧ�ߧ֧���, �� ���� �ڧ٧ާ֧�֧ߧڧ� ��ا� �٧ѧܧ�ߧ�֧ߧ�!
													  #endif
													  #ifdef LANGUAGE_AR_ENABLE
														{0x0625,0x0646,0x0020,0x0639,0x0645,0x0644,0x064A,0x0629,0x0020,0x0642,0x064A,0x0627,0x0633,0x0020,0x0627,0x0644,0x062A,0x0648,0x0642,0x064A,0x062A,0x0020,0x0639,0x0644,0x0649,0x0020,0x0648,0x0634,0x0643,0x0020,0x0623,0x0646,0x0020,0x062A,0x0628,0x062F,0x0623,0x060C,0x0020,0x0648,0x0642,0x062F,0x0020,0x0625,0x0646,0x062A,0x0647,0x0649,0x0020,0x0647,0x0630,0x0627,0x0020,0x0627,0x0644,0x0642,0x064A,0x0627,0x0633,0x0021,0x0000},//?? ????? ???? ??????? ??? ??? ?? ????? ??? ????? ??? ??????!
													  #endif
													#else
													  #ifdef LANGUAGE_CN_ENABLE
														{0x5B9A,0x65F6,0x6D4B,0x91CF,0x5373,0x5C06,0x5F00,0x59CB,0xFF0C,0x672C,0x6B21,0x6D4B,0x91CF,0x7ED3,0x675F,0xFF01,0x0000},//��ʱ����������ʼ�����β���������
													  #endif
													  #ifdef LANGUAGE_EN_ENABLE
														{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
													  #endif
													#endif
													};
static uint16_t str_running_note[LANGUAGE_MAX][70] = {
													#ifndef FW_FOR_CN
													  #ifdef LANGUAGE_EN_ENABLE
														{0x0054,0x0068,0x0065,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0069,0x0073,0x0020,0x0072,0x0075,0x006E,0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
													  #endif
													  #ifdef LANGUAGE_DE_ENABLE
														{0x0044,0x0065,0x0072,0x0020,0x0053,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x006C,0x00E4,0x0075,0x0066,0x0074,0x002C,0x0020,0x0062,0x0069,0x0074,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0073,0x0070,0x00E4,0x0074,0x0065,0x0072,0x0020,0x0065,0x0072,0x006E,0x0065,0x0075,0x0074,0x0021,0x0000},//Der Sensor l?uft, bitte versuchen Sie es sp?ter erneut!
													  #endif
													  #ifdef LANGUAGE_FR_ENABLE
														{0x004C,0x0065,0x0020,0x0063,0x0061,0x0070,0x0074,0x0065,0x0075,0x0072,0x0020,0x0065,0x0073,0x0074,0x0020,0x0065,0x006E,0x0020,0x0063,0x006F,0x0075,0x0072,0x0073,0x0020,0x0064,0x0027,0x0065,0x0078,0x00E9,0x0063,0x0075,0x0074,0x0069,0x006F,0x006E,0x002C,0x0020,0x0076,0x0065,0x0075,0x0069,0x006C,0x006C,0x0065,0x007A,0x0020,0x0072,0x00E9,0x0065,0x0073,0x0073,0x0061,0x0079,0x0065,0x0072,0x0020,0x0070,0x006C,0x0075,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0021,0x0000},//Le capteur est en cours d'ex��cution, veuillez r��essayer plus tard!
													  #endif
													  #ifdef LANGUAGE_IT_ENABLE
														{0x0049,0x006C,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0065,0x0020,0x00E8,0x0020,0x0069,0x006E,0x0020,0x0065,0x0073,0x0065,0x0063,0x0075,0x007A,0x0069,0x006F,0x006E,0x0065,0x002C,0x0020,0x0072,0x0069,0x0070,0x0072,0x006F,0x0076,0x0061,0x0020,0x0070,0x0069,0x00F9,0x0020,0x0074,0x0061,0x0072,0x0064,0x0069,0x0021,0x0000},//Il sensore �� in esecuzione, riprova pi�� tardi!
													  #endif
													  #ifdef LANGUAGE_ES_ENABLE
														{0x0045,0x006C,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0073,0x0065,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0065,0x006A,0x0065,0x0063,0x0075,0x0074,0x0061,0x006E,0x0064,0x006F,0x002C,0x0020,0x00A1,0x0069,0x006E,0x0074,0x00E9,0x006E,0x0074,0x0065,0x006C,0x006F,0x0020,0x0064,0x0065,0x0020,0x006E,0x0075,0x0065,0x0076,0x006F,0x0020,0x006D,0x00E1,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0065,0x0021,0x0000},//El sensor se est�� ejecutando, ?int��ntelo de nuevo m��s tarde!
													  #endif
													  #ifdef LANGUAGE_PT_ENABLE
														{0x004F,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0065,0x0073,0x0074,0x00E1,0x0020,0x0066,0x0075,0x006E,0x0063,0x0069,0x006F,0x006E,0x0061,0x006E,0x0064,0x006F,0x002C,0x0020,0x0074,0x0065,0x006E,0x0074,0x0065,0x0020,0x006E,0x006F,0x0076,0x0061,0x006D,0x0065,0x006E,0x0074,0x0065,0x0020,0x006D,0x0061,0x0069,0x0073,0x0020,0x0074,0x0061,0x0072,0x0064,0x0065,0x0021,0x0000},//O sensor est�� funcionando, tente novamente mais tarde!
													  #endif
													  #ifdef LANGUAGE_PL_ENABLE
														{0x0043,0x007A,0x0075,0x006A,0x006E,0x0069,0x006B,0x0020,0x0064,0x007A,0x0069,0x0061,0x0142,0x0061,0x002E,0x0020,0x0053,0x0070,0x0072,0x00F3,0x0062,0x0075,0x006A,0x0020,0x0070,0x006F,0x006E,0x006F,0x0077,0x006E,0x0069,0x0065,0x0020,0x0070,0x00F3,0x017A,0x006E,0x0069,0x0065,0x006A,0x0021,0x0000},//Czujnik dzia?a. Spr��buj ponownie p��?niej!
													  #endif
													  #ifdef LANGUAGE_SE_ENABLE
														{0x0053,0x0065,0x006E,0x0073,0x006F,0x0072,0x006E,0x0020,0x00E4,0x0072,0x0020,0x0069,0x0067,0x00E5,0x006E,0x0067,0x002C,0x0020,0x0066,0x00F6,0x0072,0x0073,0x00F6,0x006B,0x0020,0x0069,0x0067,0x0065,0x006E,0x0020,0x0073,0x0065,0x006E,0x0061,0x0072,0x0065,0x0021,0x0000},//Sensorn ?r ig?ng, f?rs?k igen senare!
													  #endif
													  #ifdef LANGUAGE_JP_ENABLE
														{0x30BB,0x30F3,0x30B5,0x30FC,0x306F,0x52D5,0x4F5C,0x4E2D,0x3067,0x3059,0x3002,0x3057,0x3070,0x3089,0x304F,0x3057,0x3066,0x304B,0x3089,0x3082,0x3046,0x4E00,0x5EA6,0x304A,0x8A66,0x3057,0x304F,0x3060,0x3055,0x3044,0x3002,0x0000},//���󥵩`�τ����ФǤ������Ф餯���Ƥ���⤦һ�Ȥ�ԇ������������
													  #endif
													  #ifdef LANGUAGE_KR_ENABLE
														{0xC13C,0xC11C,0xAC00,0x0020,0xC791,0xB3D9,0x0020,0xC911,0xC785,0xB2C8,0xB2E4,0x002E,0x0020,0xB098,0xC911,0xC5D0,0x0020,0xB2E4,0xC2DC,0x0020,0xC2DC,0xB3C4,0xD558,0xC138,0xC694,0x0021,0x0000},//??? ?? ????. ??? ?? ?????!
													  #endif
													  #ifdef LANGUAGE_RU_ENABLE
														{0x0414,0x0430,0x0442,0x0447,0x0438,0x043A,0x0020,0x0440,0x0430,0x0431,0x043E,0x0442,0x0430,0x0435,0x0442,0x002C,0x0020,0x043F,0x043E,0x043F,0x0440,0x043E,0x0431,0x0443,0x0439,0x0442,0x0435,0x0020,0x0435,0x0449,0x0435,0x0020,0x0440,0x0430,0x0437,0x0020,0x043F,0x043E,0x0437,0x0436,0x0435,0x0021,0x0000},//���ѧ��ڧ� ��ѧҧ��ѧ֧�, ������ҧ�ۧ�� �֧�� ��ѧ� ���٧ا�!
													  #endif
													  #ifdef LANGUAGE_AR_ENABLE
														{0x062A,0x0645,0x0020,0x062A,0x0634,0x063A,0x064A,0x0644,0x0020,0x0627,0x0644,0x0645,0x0633,0x062A,0x0634,0x0639,0x0631,0x060C,0x0020,0x064A,0x0631,0x062C,0x0649,0x0020,0x0627,0x0644,0x0645,0x062D,0x0627,0x0648,0x0644,0x0629,0x0020,0x0645,0x0631,0x0629,0x0020,0x0623,0x062E,0x0631,0x0649,0x0020,0x0644,0x0627,0x062D,0x0642,0x064B,0x0627,0x0021,0x0000},//?? ????? ????????? ???? ???????? ??? ???? ??????!
													  #endif
													#else
													  #ifdef LANGUAGE_CN_ENABLE
														{0x4F20,0x611F,0x5668,0x6B63,0x5728,0x8FD0,0x884C,0xFF0C,0x8BF7,0x7A0D,0x540E,0x518D,0x8BD5,0xFF01,0x0000},//�������������У����Ժ����ԣ�
													  #endif
													  #ifdef LANGUAGE_EN_ENABLE
														{0x0054,0x0068,0x0065,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0069,0x0073,0x0020,0x0072,0x0075,0x006E,0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
													  #endif
													#endif
													};

static void ppg_set_appmode_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_appmode_timer, ppg_set_appmode_timerout, NULL);
static void ppg_auto_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_stop_timer, ppg_auto_stop_timerout, NULL);
static void ppg_menu_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_menu_stop_timer, ppg_menu_stop_timerout, NULL);
static void ppg_get_data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_get_hr_timer, ppg_get_data_timerout, NULL);
static void ppg_delay_start_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_delay_start_timer, ppg_delay_start_timerout, NULL);
static void ppg_bpt_est_start_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_bpt_est_start_timer, ppg_bpt_est_start_timerout, NULL);
static void ppg_skin_check_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_skin_check_timer, ppg_skin_check_timerout, NULL);

void ClearAllBptRecData(void)
{
	memset(&g_bpt, 0, sizeof(bpt_data));
	memset(&g_bpt_menu, 0, sizeof(bpt_data));	
	memset(&rec2buf, 0xff, sizeof(rec2buf));
	
	SpiFlash_Write(rec2buf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
}

void SetCurDayBptRecData(sys_date_timer_t time_stamp, bpt_data bpt)
{
	uint16_t i;
	bpt_rec2_nod *p_bpt,tmp_bpt = {0};

	tmp_bpt.year = time_stamp.year;
	tmp_bpt.month = time_stamp.month;
	tmp_bpt.day = time_stamp.day;
	tmp_bpt.hour = time_stamp.hour;
	tmp_bpt.min = time_stamp.minute;
	memcpy(&tmp_bpt.bpt, &bpt, sizeof(bpt_data));

	memset(&databuf, 0x00, sizeof(databuf));
	memset(&rec2buf, 0x00, sizeof(rec2buf));
	
	SpiFlash_Read(rec2buf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	p_bpt = (bpt_rec2_nod*)rec2buf;
	if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
		||(p_bpt->month == 0xff || p_bpt->month == 0x00)
		||(p_bpt->day == 0xff || p_bpt->day == 0x00)
		||(p_bpt->hour == 0xff || p_bpt->min == 0xff)
		||((p_bpt->year == tmp_bpt.year)
			&&(p_bpt->month == tmp_bpt.month)
			&&(p_bpt->day == tmp_bpt.day)
			&&(p_bpt->hour == tmp_bpt.hour)
			&&(p_bpt->min == tmp_bpt.min))
		)
	{
		//ֱ�Ӹ���д�ڵ�һ��
		memcpy(p_bpt, &tmp_bpt, sizeof(bpt_rec2_nod));
		SpiFlash_Write(rec2buf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	}
	else if((tmp_bpt.year < p_bpt->year)
			||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month < p_bpt->month))
			||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day < p_bpt->day))
			||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day == p_bpt->day)&&(tmp_bpt.hour < p_bpt->hour))
			||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day == p_bpt->day)&&(tmp_bpt.hour == p_bpt->hour)&&(tmp_bpt.min < p_bpt->min))
			)
	{
		//�����µ�����,�ɵ���������Ų���������һ��
		memcpy(&databuf[0*sizeof(bpt_rec2_nod)], &tmp_bpt, sizeof(bpt_rec2_nod));
		memcpy(&databuf[1*sizeof(bpt_rec2_nod)], &rec2buf[0*sizeof(bpt_rec2_nod)], PPG_BPT_REC2_DATA_SIZE-sizeof(bpt_rec2_nod));
		SpiFlash_Write(databuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	}
	else
	{
		//Ѱ�Һ��ʵĲ���λ��
		for(i=0;i<PPG_BPT_REC2_DATA_SIZE/(sizeof(bpt_rec2_nod));i++)
		{
			p_bpt = rec2buf+i*sizeof(bpt_rec2_nod);
			if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
				||(p_bpt->month == 0xff || p_bpt->month == 0x00)
				||(p_bpt->day == 0xff || p_bpt->day == 0x00)
				||(p_bpt->hour == 0xff || p_bpt->min == 0xff)
				||((p_bpt->year == tmp_bpt.year)
					&&(p_bpt->month == tmp_bpt.month)
					&&(p_bpt->day == tmp_bpt.day)
					&&(p_bpt->hour == tmp_bpt.hour)
					&&(p_bpt->min == tmp_bpt.min))
				)
			{
				//ֱ�Ӹ���д
				memcpy(p_bpt, &tmp_bpt, sizeof(bpt_rec2_nod));
				SpiFlash_Write(rec2buf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
				return;
			}
			else if((tmp_bpt.year > p_bpt->year)
					||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month > p_bpt->month))
					||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day > p_bpt->day))
					||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day == p_bpt->day)&&(tmp_bpt.hour > p_bpt->hour))
					||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day == p_bpt->day)&&(tmp_bpt.hour == p_bpt->hour)&&(tmp_bpt.min > p_bpt->min))
					)
			{
				if(i < (PPG_BPT_REC2_DATA_SIZE/sizeof(bpt_rec2_nod)-1))
				{
					p_bpt++;
					if((tmp_bpt.year < p_bpt->year)
						||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month < p_bpt->month))
						||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day < p_bpt->day))
						||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day == p_bpt->day)&&(tmp_bpt.hour < p_bpt->hour))
						||((tmp_bpt.year == p_bpt->year)&&(tmp_bpt.month == p_bpt->month)&&(tmp_bpt.day == p_bpt->day)&&(tmp_bpt.hour == p_bpt->hour)&&(tmp_bpt.min < p_bpt->min))
						)
					{
						break;
					}
				}
			}
		}

		if(i < (PPG_BPT_REC2_DATA_SIZE/sizeof(bpt_rec2_nod)-1))
		{
			//�ҵ�λ�ã����������ݣ���������������Ų���������һ��
			memcpy(&databuf[0*sizeof(bpt_rec2_nod)], &rec2buf[0*sizeof(bpt_rec2_nod)], (i+1)*sizeof(bpt_rec2_nod));
			memcpy(&databuf[(i+1)*sizeof(bpt_rec2_nod)], &tmp_bpt, sizeof(bpt_rec2_nod));
			memcpy(&databuf[(i+2)*sizeof(bpt_rec2_nod)], &rec2buf[(i+1)*sizeof(bpt_rec2_nod)], PPG_BPT_REC2_DATA_SIZE-(i+1)*sizeof(bpt_rec2_nod));
			SpiFlash_Write(databuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
		}
		else
		{
			//δ�ҵ�λ�ã�ֱ�ӽ���ĩβ��������������ǰ�ƣ�������ǰһ��
			memcpy(&databuf[0*sizeof(bpt_rec2_nod)], &rec2buf[1*sizeof(bpt_rec2_nod)], PPG_BPT_REC2_DATA_SIZE-sizeof(bpt_rec2_nod));
			memcpy(&databuf[PPG_BPT_REC2_DATA_SIZE-sizeof(bpt_rec2_nod)], &tmp_bpt, sizeof(bpt_rec2_nod));
			SpiFlash_Write(databuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
		}
	}
}

void GetCurDayBptRecData(uint8_t *databuf)
{
	uint16_t i,j=0;
	bpt_rec2_nod *p_bpt;
	
	if(databuf == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	p_bpt = (bpt_rec2_nod*)rec2buf;
	for(i=0;i<PPG_BPT_REC2_DATA_SIZE/sizeof(bpt_rec2_nod);i++)
	{
		if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
			||(p_bpt->month == 0xff || p_bpt->month == 0x00)
			||(p_bpt->day == 0xff || p_bpt->day == 0x00)
			||(p_bpt->hour == 0xff || p_bpt->min == 0xff)
			)
		{
			break;
		}
		else if((p_bpt->year < date_time.year)
			||((p_bpt->year == date_time.year)&&(p_bpt->month < date_time.month))
			||((p_bpt->year == date_time.year)&&(p_bpt->month == date_time.month)&&(p_bpt->day < date_time.day))
			)
		{
			p_bpt++;
			continue;
		}
		else if((p_bpt->year > date_time.year)
				||((p_bpt->year == date_time.year)&&(p_bpt->month > date_time.month))
				||((p_bpt->year == date_time.year)&&(p_bpt->month == date_time.month)&&(p_bpt->day > date_time.day))
				)
		{
			break;
		}
		else
		{
			memcpy(&databuf[j*sizeof(bpt_rec2_nod)], p_bpt, sizeof(bpt_rec2_nod));
			j++;
		}
		
		p_bpt++;
	}
}

void GetGivenDayBptRecData(sys_date_timer_t date, uint8_t *databuf)
{
	uint16_t i,j=0;
	bpt_rec2_nod *p_bpt;

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(databuf == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	p_bpt = (bpt_rec2_nod*)rec2buf;
	for(i=0;i<PPG_BPT_REC2_DATA_SIZE/sizeof(bpt_rec2_nod);i++)
	{
		if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
			||(p_bpt->month == 0xff || p_bpt->month == 0x00)
			||(p_bpt->day == 0xff || p_bpt->day == 0x00)
			||(p_bpt->hour == 0xff || p_bpt->min == 0xff)
			)
		{
			break;
		}
		else if((p_bpt->year < date.year)
			||((p_bpt->year == date.year)&&(p_bpt->month < date.month))
			||((p_bpt->year == date.year)&&(p_bpt->month == date.month)&&(p_bpt->day < date.day))
			)
		{
			p_bpt++;
			continue;
		}
		else if((p_bpt->year > date.year)
				||((p_bpt->year == date.year)&&(p_bpt->month > date.month))
				||((p_bpt->year == date.year)&&(p_bpt->month == date.month)&&(p_bpt->day > date.day))
				)
		{
			break;
		}
		else
		{
			memcpy(&databuf[j*sizeof(bpt_rec2_nod)], p_bpt, sizeof(bpt_rec2_nod));
			j++;
		}

		p_bpt++;
	}
}

void GetGivenTimeBptRecData(sys_date_timer_t date, bpt_data *bpt)
{
	uint16_t i;
	bpt_rec2_nod *p_bpt;

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(bpt == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	p_bpt = (bpt_rec2_nod*)rec2buf;
	for(i=0;i<PPG_BPT_REC2_DATA_SIZE/sizeof(bpt_rec2_nod);i++)
	{
		if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
			||(p_bpt->month == 0xff || p_bpt->month == 0x00)
			||(p_bpt->day == 0xff || p_bpt->day == 0x00)
			||(p_bpt->hour == 0xff || p_bpt->min == 0xff)
			)
		{
			break;
		}
		else if((p_bpt->year == date.year)
				&&(p_bpt->month == date.month)
				&&(p_bpt->day == date.day)
				&&(p_bpt->hour == date.hour)
				&&(p_bpt->min == date.minute)
				)
		{
			memcpy(bpt, &p_bpt->bpt, sizeof(bpt_data));
			break;
		}

		p_bpt++;
	}
}

void ClearAllSpo2RecData(void)
{
	g_spo2 = 0;
	g_spo2_menu = 0;
	memset(&rec2buf, 0xff, sizeof(rec2buf));

	SpiFlash_Write(rec2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
}

void SetCurDaySpo2RecData(sys_date_timer_t time_stamp, uint8_t spo2)
{
	uint16_t i;
	spo2_rec2_nod *p_spo2,tmp_spo2 = {0};
	
	tmp_spo2.year = time_stamp.year;
	tmp_spo2.month = time_stamp.month;
	tmp_spo2.day = time_stamp.day;
	tmp_spo2.hour = time_stamp.hour;
	tmp_spo2.min = time_stamp.minute;
	tmp_spo2.spo2 = spo2;
	
	memset(&databuf, 0x00, sizeof(databuf));
	memset(&rec2buf, 0x00, sizeof(rec2buf));

	SpiFlash_Read(rec2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	p_spo2 = (spo2_rec2_nod*)rec2buf;
	if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
		||(p_spo2->month == 0xff || p_spo2->month == 0x00)
		||(p_spo2->day == 0xff || p_spo2->day == 0x00)
		||(p_spo2->hour == 0xff || p_spo2->min == 0xff)
		||((p_spo2->year == tmp_spo2.year)
			&&(p_spo2->month == tmp_spo2.month)
			&&(p_spo2->day == tmp_spo2.day)
			&&(p_spo2->hour == tmp_spo2.hour)
			&&(p_spo2->min == tmp_spo2.min))
		)
	{
		//ֱ�Ӹ���д�ڵ�һ��
		memcpy(p_spo2, &tmp_spo2, sizeof(spo2_rec2_nod));
		SpiFlash_Write(rec2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	}
	else if((tmp_spo2.year < p_spo2->year)
			||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month < p_spo2->month))
			||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day < p_spo2->day))
			||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day == p_spo2->day)&&(tmp_spo2.hour < p_spo2->hour))
			||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day == p_spo2->day)&&(tmp_spo2.hour == p_spo2->hour)&&(tmp_spo2.min < p_spo2->min))
			)
	{
		//�����µ�����,�ɵ���������Ų���������һ��
		memcpy(&databuf[0*sizeof(spo2_rec2_nod)], &tmp_spo2, sizeof(spo2_rec2_nod));
		memcpy(&databuf[1*sizeof(spo2_rec2_nod)], &rec2buf[0*sizeof(spo2_rec2_nod)], PPG_SPO2_REC2_DATA_SIZE-sizeof(spo2_rec2_nod));
		SpiFlash_Write(databuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	}
	else
	{
		//Ѱ�Һ��ʵĲ���λ��
		for(i=0;i<PPG_SPO2_REC2_DATA_SIZE/sizeof(spo2_rec2_nod);i++)
		{
			p_spo2 = rec2buf+i*sizeof(spo2_rec2_nod);
			if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
				||(p_spo2->month == 0xff || p_spo2->month == 0x00)
				||(p_spo2->day == 0xff || p_spo2->day == 0x00)
				||(p_spo2->hour == 0xff || p_spo2->min == 0xff)
				||((p_spo2->year == tmp_spo2.year)
					&&(p_spo2->month == tmp_spo2.month)
					&&(p_spo2->day == tmp_spo2.day)
					&&(p_spo2->hour == tmp_spo2.hour)
					&&(p_spo2->min == tmp_spo2.min))
				)
			{
				//ֱ�Ӹ���д
				memcpy(p_spo2, &tmp_spo2, sizeof(spo2_rec2_nod));
				SpiFlash_Write(rec2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
				return;
			}
			else if((tmp_spo2.year > p_spo2->year)
					||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month > p_spo2->month))
					||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day > p_spo2->day))
					||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day == p_spo2->day)&&(tmp_spo2.hour > p_spo2->hour))
					||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day == p_spo2->day)&&(tmp_spo2.hour == p_spo2->hour)&&(tmp_spo2.min > p_spo2->min))
					)
			{
				if(i < (PPG_SPO2_REC2_DATA_SIZE/sizeof(spo2_rec2_nod)-1))
				{
					p_spo2++;
					if((tmp_spo2.year < p_spo2->year)
						||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month < p_spo2->month))
						||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day < p_spo2->day))
						||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day == p_spo2->day)&&(tmp_spo2.hour < p_spo2->hour))
						||((tmp_spo2.year == p_spo2->year)&&(tmp_spo2.month == p_spo2->month)&&(tmp_spo2.day == p_spo2->day)&&(tmp_spo2.hour == p_spo2->hour)&&(tmp_spo2.min < p_spo2->min))
						)
					{
						break;
					}
				}
			}
		}

		if(i < (PPG_SPO2_REC2_DATA_SIZE/sizeof(spo2_rec2_nod)-1))
		{
			//�ҵ�λ�ã����������ݣ���������������Ų���������һ��
			memcpy(&databuf[0*sizeof(spo2_rec2_nod)], &rec2buf[0*sizeof(spo2_rec2_nod)], (i+1)*sizeof(spo2_rec2_nod));
			memcpy(&databuf[(i+1)*sizeof(spo2_rec2_nod)], &tmp_spo2, sizeof(spo2_rec2_nod));
			memcpy(&databuf[(i+2)*sizeof(spo2_rec2_nod)], &rec2buf[(i+1)*sizeof(spo2_rec2_nod)], PPG_SPO2_REC2_DATA_SIZE-(i+1)*sizeof(spo2_rec2_nod));
			SpiFlash_Write(databuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
		}
		else
		{
			//δ�ҵ�λ�ã�ֱ�ӽ���ĩβ��������������ǰ�ƣ�������ǰһ��
			memcpy(&databuf[0*sizeof(spo2_rec2_nod)], &rec2buf[1*sizeof(spo2_rec2_nod)], PPG_SPO2_REC2_DATA_SIZE-sizeof(spo2_rec2_nod));
			memcpy(&databuf[PPG_SPO2_REC2_DATA_SIZE-sizeof(spo2_rec2_nod)], &tmp_spo2, sizeof(spo2_rec2_nod));
			SpiFlash_Write(databuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
		}
	}
}

void GetCurDaySpo2RecData(uint8_t *databuf)
{
	uint16_t i,j=0;
	spo2_rec2_nod *p_spo2;
	
	if(databuf == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	p_spo2 = (spo2_rec2_nod*)rec2buf;
	for(i=0;i<PPG_SPO2_REC2_DATA_SIZE/sizeof(spo2_rec2_nod);i++)
	{
		if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
			||(p_spo2->month == 0xff || p_spo2->month == 0x00)
			||(p_spo2->day == 0xff || p_spo2->day == 0x00)
			||(p_spo2->hour == 0xff || p_spo2->min == 0xff)
			)
		{
			break;
		}
		else if((p_spo2->year < date_time.year)
			||((p_spo2->year == date_time.year)&&(p_spo2->month < date_time.month))
			||((p_spo2->year == date_time.year)&&(p_spo2->month == date_time.month)&&(p_spo2->day < date_time.day))
			)
		{
			p_spo2++;
			continue;
		}
		else if((p_spo2->year > date_time.year)
				||((p_spo2->year == date_time.year)&&(p_spo2->month > date_time.month))
				||((p_spo2->year == date_time.year)&&(p_spo2->month == date_time.month)&&(p_spo2->day > date_time.day))
				)
		{
			break;
		}
		else
		{
			memcpy(&databuf[j*sizeof(spo2_rec2_nod)], p_spo2, sizeof(spo2_rec2_nod));
			j++;
		}

		p_spo2++;
	}
}

void GetGivenDaySpo2RecData(sys_date_timer_t date, uint8_t *databuf)
{
	uint16_t i,j=0;
	spo2_rec2_nod *p_spo2;

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(databuf == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	p_spo2 = (bpt_rec2_nod*)rec2buf;
	for(i=0;i<PPG_SPO2_REC2_DATA_SIZE/sizeof(spo2_rec2_nod);i++)
	{
		if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
			||(p_spo2->month == 0xff || p_spo2->month == 0x00)
			||(p_spo2->day == 0xff || p_spo2->day == 0x00)
			||(p_spo2->hour == 0xff || p_spo2->min == 0xff)
			)
		{
			break;
		}
		else if((p_spo2->year < date.year)
			||((p_spo2->year == date.year)&&(p_spo2->month < date.month))
			||((p_spo2->year == date.year)&&(p_spo2->month == date.month)&&(p_spo2->day < date.day))
			)
		{
			p_spo2++;
			continue;
		}
		else if((p_spo2->year > date.year)
				||((p_spo2->year == date.year)&&(p_spo2->month > date.month))
				||((p_spo2->year == date.year)&&(p_spo2->month == date.month)&&(p_spo2->day > date.day))
				)
		{
			break;
		}
		else
		{
			memcpy(&databuf[j*sizeof(spo2_rec2_nod)], p_spo2, sizeof(spo2_rec2_nod));
			j++;
		}

		p_spo2++;
	}
}

void GetGivenTimeSpo2RecData(sys_date_timer_t date, uint8_t *spo2)
{
	uint16_t i;
	spo2_rec2_nod *p_spo2;

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(spo2 == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	p_spo2 = (spo2_rec2_nod*)rec2buf;
	for(i=0;i<PPG_SPO2_REC2_DATA_SIZE/sizeof(spo2_rec2_nod);i++)
	{
		if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
			||(p_spo2->month == 0xff || p_spo2->month == 0x00)
			||(p_spo2->day == 0xff || p_spo2->day == 0x00)
			||(p_spo2->hour == 0xff || p_spo2->min == 0xff)
			)
		{
			break;
		}
		
		if((p_spo2->year == date.year)
			&&(p_spo2->month == date.month)
			&&(p_spo2->day == date.day)
			&&(p_spo2->hour == date.hour)
			&&(p_spo2->min == date.minute)
			)
		{
			*spo2 = p_spo2->spo2;
			break;
		}

		p_spo2++;
	}
}

void ClearAllHrRecData(void)
{
	g_hr = 0;
	g_hr_menu = 0;
	memset(&rec2buf, 0xff, sizeof(rec2buf));

	SpiFlash_Write(rec2buf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
}

void SetCurDayHrRecData(sys_date_timer_t time_stamp, uint8_t hr)
{
	uint16_t i;
	hr_rec2_nod *p_hr,tmp_hr = {0};
	
	tmp_hr.year = time_stamp.year;
	tmp_hr.month = time_stamp.month;
	tmp_hr.day = time_stamp.day;
	tmp_hr.hour = time_stamp.hour;
	tmp_hr.min = time_stamp.minute;
	tmp_hr.hr = hr;

	memset(&databuf, 0x00, sizeof(databuf));
	memset(&rec2buf, 0x00, sizeof(rec2buf));
	
	SpiFlash_Read(rec2buf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	p_hr = (hr_rec2_nod*)rec2buf;
	if((p_hr->year == 0xffff || p_hr->year == 0x0000)
		||(p_hr->month == 0xff || p_hr->month == 0x00)
		||(p_hr->day == 0xff || p_hr->day == 0x00)
		||(p_hr->hour == 0xff || p_hr->min == 0xff)
		||((p_hr->year == tmp_hr.year)
			&&(p_hr->month == tmp_hr.month)
			&&(p_hr->day == tmp_hr.day)
			&&(p_hr->hour == tmp_hr.hour)
			&&(p_hr->min == tmp_hr.min))
		)
	{
		//ֱ�Ӹ���д�ڵ�һ��
		memcpy(p_hr, &tmp_hr, sizeof(hr_rec2_nod));
		SpiFlash_Write(rec2buf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	}
	else if((tmp_hr.year < p_hr->year)
			||((tmp_hr.year == p_hr->year)&&(tmp_hr.month < p_hr->month))
			||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day < p_hr->day))
			||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day == p_hr->day)&&(tmp_hr.hour < p_hr->hour))
			||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day == p_hr->day)&&(tmp_hr.hour == p_hr->hour)&&(tmp_hr.min < p_hr->min))
			)
	{
		//�����µ�����,�ɵ���������Ų���������һ��
		memcpy(&databuf[0*sizeof(hr_rec2_nod)], &tmp_hr, sizeof(hr_rec2_nod));
		memcpy(&databuf[1*sizeof(hr_rec2_nod)], &rec2buf[0*sizeof(hr_rec2_nod)], PPG_HR_REC2_DATA_SIZE-sizeof(hr_rec2_nod));
		SpiFlash_Write(databuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	}
	else
	{
		//Ѱ�Һ��ʵĲ���λ��
		for(i=0;i<PPG_HR_REC2_DATA_SIZE/sizeof(hr_rec2_nod);i++)
		{
			p_hr = rec2buf+i*sizeof(hr_rec2_nod);
			if((p_hr->year == 0xffff || p_hr->year == 0x0000)
				||(p_hr->month == 0xff || p_hr->month == 0x00)
				||(p_hr->day == 0xff || p_hr->day == 0x00)
				||(p_hr->hour == 0xff || p_hr->min == 0xff)
				||((p_hr->year == tmp_hr.year)
					&&(p_hr->month == tmp_hr.month)
					&&(p_hr->day == tmp_hr.day)
					&&(p_hr->hour == tmp_hr.hour)
					&&(p_hr->min == tmp_hr.min))
				)
			{
				//ֱ�Ӹ���д
				memcpy(p_hr, &tmp_hr, sizeof(hr_rec2_nod));
				SpiFlash_Write(rec2buf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
				return;
			}
			else if((tmp_hr.year > p_hr->year)
					||((tmp_hr.year == p_hr->year)&&(tmp_hr.month > p_hr->month))
					||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day > p_hr->day))
					||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day == p_hr->day)&&(tmp_hr.hour > p_hr->hour))
					||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day == p_hr->day)&&(tmp_hr.hour == p_hr->hour)&&(tmp_hr.min > p_hr->min))
					)
			{
				if(i < (PPG_HR_REC2_DATA_SIZE/sizeof(hr_rec2_nod)-1))
				{
					p_hr++;
					if((tmp_hr.year < p_hr->year)
						||((tmp_hr.year == p_hr->year)&&(tmp_hr.month < p_hr->month))
						||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day < p_hr->day))
						||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day == p_hr->day)&&(tmp_hr.hour < p_hr->hour))
						||((tmp_hr.year == p_hr->year)&&(tmp_hr.month == p_hr->month)&&(tmp_hr.day == p_hr->day)&&(tmp_hr.hour == p_hr->hour)&&(tmp_hr.min < p_hr->min))
						)
					{
						break;
					}
				}
			}
		}

		if(i < (PPG_HR_REC2_DATA_SIZE/sizeof(hr_rec2_nod)-1))
		{
			//�ҵ�λ�ã����������ݣ���������������Ų���������һ��
			memcpy(&databuf[0*sizeof(hr_rec2_nod)], &rec2buf[0*sizeof(hr_rec2_nod)], (i+1)*sizeof(hr_rec2_nod));
			memcpy(&databuf[(i+1)*sizeof(hr_rec2_nod)], &tmp_hr, sizeof(hr_rec2_nod));
			memcpy(&databuf[(i+2)*sizeof(hr_rec2_nod)], &rec2buf[(i+1)*sizeof(hr_rec2_nod)], PPG_HR_REC2_DATA_SIZE-(i+1)*sizeof(hr_rec2_nod));
			SpiFlash_Write(databuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
		}
		else
		{
			//δ�ҵ�λ�ã�ֱ�ӽ���ĩβ��������������ǰ�ƣ�������ǰһ��
			memcpy(&databuf[0*sizeof(hr_rec2_nod)], &rec2buf[1*sizeof(hr_rec2_nod)], PPG_HR_REC2_DATA_SIZE-sizeof(hr_rec2_nod));
			memcpy(&databuf[PPG_HR_REC2_DATA_SIZE-sizeof(hr_rec2_nod)], &tmp_hr, sizeof(hr_rec2_nod));
			SpiFlash_Write(databuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
		}
	}
}

void GetCurDayHrRecData(uint8_t *databuf)
{
	uint16_t i,j=0;
	hr_rec2_nod *p_hr;
	
	if(databuf == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	p_hr = (hr_rec2_nod*)rec2buf;
	for(i=0;i<PPG_HR_REC2_DATA_SIZE/sizeof(hr_rec2_nod);i++)
	{
		if((p_hr->year == 0xffff || p_hr->year == 0x0000)
			||(p_hr->month == 0xff || p_hr->month == 0x00)
			||(p_hr->day == 0xff || p_hr->day == 0x00)
			||(p_hr->hour == 0xff || p_hr->min == 0xff)
			)
		{
			break;
		}
		else if((p_hr->year < date_time.year)
			||((p_hr->year == date_time.year)&&(p_hr->month < date_time.month))
			||((p_hr->year == date_time.year)&&(p_hr->month == date_time.month)&&(p_hr->day < date_time.day))
			)
		{
			p_hr++;
			continue;
		}
		else if((p_hr->year > date_time.year)
				||((p_hr->year == date_time.year)&&(p_hr->month > date_time.month))
				||((p_hr->year == date_time.year)&&(p_hr->month == date_time.month)&&(p_hr->day > date_time.day))
				)
		{
			break;
		}
		else
		{
			memcpy(&databuf[j*sizeof(hr_rec2_nod)], p_hr, sizeof(hr_rec2_nod));
			j++;
		}

		p_hr++;
	}
}

void GetGivenDayHrRecData(sys_date_timer_t date, uint8_t *databuf)
{
	uint16_t i,j=0;
	hr_rec2_nod *p_hr;

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(databuf == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	p_hr = (hr_rec2_nod*)rec2buf;
	for(i=0;i<PPG_HR_REC2_DATA_SIZE/sizeof(hr_rec2_nod);i++)
	{
		if((p_hr->year == 0xffff || p_hr->year == 0x0000)
			||(p_hr->month == 0xff || p_hr->month == 0x00)
			||(p_hr->day == 0xff || p_hr->day == 0x00)
			||(p_hr->hour == 0xff || p_hr->min == 0xff)
			)
		{
			break;
		}
		else if((p_hr->year < date.year)
			||((p_hr->year == date.year)&&(p_hr->month < date.month))
			||((p_hr->year == date.year)&&(p_hr->month == date.month)&&(p_hr->day < date.day))
			)
		{
			p_hr++;
			continue;
		}
		else if((p_hr->year > date.year)
				||((p_hr->year == date.year)&&(p_hr->month > date.month))
				||((p_hr->year == date.year)&&(p_hr->month == date.month)&&(p_hr->day > date.day))
				)
		{
			break;
		}
		else
		{
			memcpy(&databuf[j*sizeof(hr_rec2_nod)], p_hr, sizeof(hr_rec2_nod));
			j++;
		}

		p_hr++;
	}
}

void GetGivenTimeHrRecData(sys_date_timer_t date, uint8_t *hr)
{
	uint16_t i;
	hr_rec2_nod *p_hr;

	if(!CheckSystemDateTimeIsValid(date))
		return;	
	if(hr == NULL)
		return;

	memset(&rec2buf, 0x00, sizeof(rec2buf));
	SpiFlash_Read(rec2buf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	p_hr = (hr_rec2_nod*)rec2buf;
	for(i=0;i<PPG_SPO2_REC2_DATA_SIZE/sizeof(spo2_rec2_nod);i++)
	{
		if((p_hr->year == 0xffff || p_hr->year == 0x0000)
			||(p_hr->month == 0xff || p_hr->month == 0x00)
			||(p_hr->day == 0xff || p_hr->day == 0x00)
			||(p_hr->hour == 0xff || p_hr->min == 0xff)
			)
		{
			break;
		}
		
		if((p_hr->year == date.year)
			&&(p_hr->month == date.month)
			&&(p_hr->day == date.day)
			&&(p_hr->hour == date.hour)
			&&(p_hr->min == date.minute)
			)
		{
			*hr = p_hr->hr;
			break;
		}

		p_hr++;
	}
}

void UpdateLastPPGData(sys_date_timer_t time_stamp, PPG_DATA_TYPE type, void *data)
{
	switch(type)
	{
	case PPG_DATA_HR:
		{
			uint8_t *p_hr = data;
			
			memcpy(&last_health.hr_rec.timestamp, &time_stamp, sizeof(sys_date_timer_t));
			last_health.hr_rec.hr = *p_hr;
			if(*p_hr > last_health.hr_max)
			{
				if(last_health.hr_min == 0)
				{
					if(last_health.hr_max > 0)
						last_health.hr_min = last_health.hr_max;
					else
						last_health.hr_min = *p_hr;
				}
				last_health.hr_max = *p_hr;
			}
			else if(*p_hr < last_health.hr_min)
			{
				last_health.hr_min = *p_hr;
			}
		}
		break;
		
	case PPG_DATA_SPO2:
		{
			uint8_t *p_spo2 = data;
			
			memcpy(&last_health.spo2_rec.timestamp, &time_stamp, sizeof(sys_date_timer_t));
			last_health.spo2_rec.spo2 = *p_spo2;
			if(*p_spo2 > last_health.spo2_max)
			{
				if(last_health.spo2_min == 0)
				{
					if(last_health.spo2_max > 0)
						last_health.spo2_min = last_health.spo2_max;
					else
						last_health.spo2_min = *p_spo2;
				}
				last_health.spo2_max = *p_spo2;
			}
			else if(*p_spo2 < last_health.spo2_min)
			{
				last_health.spo2_min = *p_spo2;
			}
		}
		break;
		
	case PPG_DATA_BPT:
		{
			bpt_data *p_bpt = data;
			
			memcpy(&last_health.bpt_rec.timestamp, &date_time, sizeof(sys_date_timer_t));
			memcpy(&last_health.bpt_rec.bpt, p_bpt, sizeof(bpt_data));
			if(p_bpt->systolic > last_health.bpt_max.systolic)
			{
				if(last_health.bpt_min.systolic == 0)
				{
					if(last_health.bpt_max.systolic > 0)
						last_health.bpt_min.systolic = last_health.bpt_max.systolic;
					else
						last_health.bpt_min.systolic = p_bpt->systolic;
				}
				last_health.bpt_max.systolic = p_bpt->systolic;
			}
			else if(p_bpt->systolic < last_health.bpt_min.systolic)
			{
				last_health.bpt_min.systolic = p_bpt->systolic;
			}
			
			if(p_bpt->diastolic > last_health.bpt_max.diastolic)
			{
				if(last_health.bpt_min.diastolic == 0)
				{
					if(last_health.bpt_max.diastolic > 0)
						last_health.bpt_min.diastolic = last_health.bpt_max.diastolic;
					else
						last_health.bpt_min.diastolic = p_bpt->diastolic;
				}
				last_health.bpt_max.diastolic = p_bpt->diastolic;
			}
			else if(p_bpt->diastolic < last_health.bpt_min.diastolic)
			{
				last_health.bpt_min.diastolic = p_bpt->diastolic;
			}
		}
		break;
	}

	save_cur_health_to_record(&last_health);
}

void GetPPGData(uint8_t *hr, uint8_t *spo2, uint8_t *systolic, uint8_t *diastolic)
{
	if(hr != NULL)
		*hr = g_hr;
	
	if(spo2 != NULL)
		*spo2 = g_spo2;
	
	if(systolic != NULL)
		*systolic = g_bpt.systolic;
	
	if(diastolic != NULL)
		*diastolic = g_bpt.diastolic;
}

bool IsInPPGScreen(void)
{
	if(screen_id == SCREEN_ID_HR || screen_id == SCREEN_ID_SPO2 || screen_id == SCREEN_ID_BP)
		return true;
	else
		return false;
}

bool PPGIsWorkingTiming(void)
{
	if((g_ppg_trigger&TRIGGER_BY_HOURLY) != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool PPGIsSccCheck(void)
{
	if((g_ppg_trigger&TRIGGER_BY_SCC) != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool PPGIsWorking(void)
{
	if(ppg_power_flag == 0)
		return false;
	else
		return true;
}

void PPGRedrawHourlyData(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		if((g_hr_hourly >= PPG_HR_MIN)&&(g_hr_hourly <= PPG_HR_MAX))
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
		if((g_spo2_hourly >= PPG_SPO2_MIN)&&(g_spo2_hourly <= PPG_SPO2_MAX))
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void PPGRedrawData(void)
{
	if(screen_id == SCREEN_ID_HR || screen_id == SCREEN_ID_SPO2 || screen_id == SCREEN_ID_BP)
	{
		if(screen_id == SCREEN_ID_HR)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
		else if(screen_id == SCREEN_ID_SPO2)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
		else if(screen_id == SCREEN_ID_BP)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BP;
		
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void ppg_get_data_timerout(struct k_timer *timer_id)
{
	ppg_get_data_flag = true;
}

static void ppg_set_appmode_timerout(struct k_timer *timer_id)
{
	ppg_appmode_init_flag = true;
}

bool PPGSenSorSet(void)
{
	int status = -1;
	uint8_t hubMode = 0x00;
	uint8_t period = 0;

	status = sh_get_sensorhub_operating_mode(&hubMode);
	if((hubMode != 0x00) && (status != SS_SUCCESS))
	{
	#ifdef PPG_DEBUG
		LOGD("work mode is not app mode:%d", hubMode);
	#endif
		return false;
	}

	if(g_ppg_alg_mode == ALG_MODE_BPT)
	{
		if(!ppg_bpt_is_calbraed)
		{
			status = sh_check_bpt_cal_data();
			if(status && !ppg_bpt_cal_need_update)
			{
			#ifdef PPG_DEBUG
				LOGD("check bpt cal success");
			#endif
				sh_set_bpt_cal_data();
				//ppg_bpt_is_calbraed = true;
				g_ppg_bpt_status = BPT_STATUS_GET_EST;
			}
			else
			{
			#ifdef PPG_DEBUG
				LOGD("check bpt cal fail, req cal from algo, (%d/%d)", global_settings.bp_calibra.systolic, global_settings.bp_calibra.diastolic);
			#endif
				sh_req_bpt_cal_data();
				g_ppg_bpt_status = BPT_STATUS_GET_CAL;

				k_timer_start(&ppg_get_hr_timer, K_MSEC(200), K_MSEC(200));
				return;
			}
		}
		else
		{
			g_ppg_bpt_status = BPT_STATUS_GET_EST;
		}
		
		//Enable AEC
		sh_set_cfg_wearablesuite_afeenable(1);
		//Enable automatic calculation of target PD current
		sh_set_cfg_wearablesuite_autopdcurrentenable(1);
		//Set minimum PD current to 12.5uA
		sh_set_cfg_wearablesuite_minpdcurrent(125);
		//Set initial PD current to 31.2uA
		sh_set_cfg_wearablesuite_initialpdcurrent(312);
		//Set target PD current to 31.2uA
		sh_set_cfg_wearablesuite_targetpdcurrent(312);
		//Enable SCD
		sh_set_cfg_wearablesuite_scdenable(1);
		//Set the output format to Sample Counter byte, Sensor Data and Algorithm
		sh_set_data_type(SS_DATATYPE_BOTH, true);
		//set fifo thresh
		sh_set_fifo_thresh(1);
		//Set the samples report period to 40ms(minimum is 32ms for BPT).
		sh_set_report_period(25);
		//Enable the sensor.
		sensorhub_enable_sensors();
		//set algo mode
		sh_set_cfg_wearablesuite_algomode(0x0);
		//set algo oper mode
		sensorhub_set_algo_operation_mode(SH_OPERATION_WHRM_BPT_MODE);
		g_algo_sensor_stat.bpt_algo_enabled = 1;
		//set algo submode
		sensorhub_set_algo_submode(SH_OPERATION_WHRM_BPT_MODE, SH_BPT_MODE_ESTIMATION);
		//enable algo
		sh_enable_algo_(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, SENSORHUB_MODE_BASIC);
		g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 1;

		k_timer_start(&ppg_get_hr_timer, K_MSEC(200), K_MSEC(500));
	}
	else if(g_ppg_alg_mode == ALG_MODE_HR_SPO2)
	{
		//set fifo thresh
		sh_set_fifo_thresh(1);
		//Set the samples report period to 40ms(minimum is 32ms for BPT).
		sh_set_report_period(25);
		//Enable AEC
		sh_set_cfg_wearablesuite_afeenable(1);
		//Enable automatic calculation of target PD current
		sh_set_cfg_wearablesuite_autopdcurrentenable(1);
		//Set the output format to Sample Counter byte, Sensor Data and Algorithm
		sh_set_data_type(SS_DATATYPE_BOTH, true);
		//Enable the sensor.
		sensorhub_enable_sensors();
		//set algo oper mode
		sensorhub_set_algo_operation_mode(SH_OPERATION_WHRM_MODE);
		g_algo_sensor_stat.bpt_algo_enabled = 0;
		//Set the WAS algorithm operation mode to Continuous HRM + Continuous SpO2
		sh_set_cfg_wearablesuite_algomode(0x0);
		//enable algo
		sh_enable_algo_(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, SENSORHUB_MODE_BASIC);
		g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 1;

		k_timer_start(&ppg_get_hr_timer, K_MSEC(1*1000), K_MSEC(1*1000));
	}
	
	return true;
}

void StartSensorhubCallBack(void)
{
	bool ret = false;
	
	ret = PPGSenSorSet();
	if(ret)
	{
		if(ppg_power_flag == 0)
		{
		#ifdef PPG_DEBUG
			LOGD("ppg hr has been stop!");
		#endif
			k_timer_stop(&ppg_get_hr_timer);
			return;
		}
	#ifdef PPG_DEBUG	
		LOGD("ppg hr start success!");
	#endif
		ppg_power_flag = 2;

		if((g_ppg_trigger&TRIGGER_BY_HOURLY) == TRIGGER_BY_HOURLY)
		{
			if(g_ppg_data == PPG_DATA_HR)
			{
				k_timer_start(&ppg_stop_timer, K_MSEC(PPG_CHECK_HR_TIMELY*60*1000), K_NO_WAIT);
			}
			else if(g_ppg_data == PPG_DATA_SPO2)
			{
				k_timer_start(&ppg_stop_timer, K_MSEC(PPG_CHECK_SPO2_TIMELY*60*1000), K_NO_WAIT);
			}
			else if(g_ppg_data == PPG_DATA_BPT)
			{
				k_timer_start(&ppg_stop_timer, K_MSEC(PPG_CHECK_BPT_TIMELY*60*1000), K_NO_WAIT);
			}
		}
	#ifndef UI_STYLE_HEALTH_BAR	
		else if((g_ppg_trigger&TRIGGER_BY_MENU) == TRIGGER_BY_MENU)
		{
			if(g_ppg_data == PPG_DATA_HR)
			{
				k_timer_start(&ppg_menu_stop_timer, K_SECONDS(PPG_CHECK_HR_MENU), K_NO_WAIT);
			}
			else if(g_ppg_data == PPG_DATA_SPO2)
			{
				k_timer_start(&ppg_menu_stop_timer, K_SECONDS(PPG_CHECK_SPO2_MENU), K_NO_WAIT);
			}
			else if(g_ppg_data == PPG_DATA_BPT)
			{
				k_timer_start(&ppg_menu_stop_timer, K_SECONDS(PPG_CHECK_BPT_MENU), K_NO_WAIT);
			}
		}
	#endif
		else if((g_ppg_trigger&TRIGGER_BY_MENU) == TRIGGER_BY_APP)
		{
			if(g_ppg_data == PPG_DATA_HR)
			{
				k_timer_start(&ppg_menu_stop_timer, K_SECONDS(PPG_CHECK_HR_MENU), K_NO_WAIT);
			}
			else if(g_ppg_data == PPG_DATA_SPO2)
			{
				k_timer_start(&ppg_menu_stop_timer, K_SECONDS(PPG_CHECK_SPO2_MENU), K_NO_WAIT);
			}
			else if(g_ppg_data == PPG_DATA_BPT)
			{
				k_timer_start(&ppg_menu_stop_timer, K_SECONDS(PPG_CHECK_BPT_MENU), K_NO_WAIT);
			}
		}
	}
	else
	{
	#ifdef PPG_DEBUG
		LOGD("ppg hr start false!");
	#endif
		ppg_power_flag = 0;
	}
}

void StartSensorhub(void)
{
	SH_set_to_APP_mode();
	k_timer_start(&ppg_appmode_timer, K_MSEC(2000), K_NO_WAIT);
}

static void ppg_bpt_est_start_timerout(struct k_timer *timer_id)
{
#ifdef PPG_DEBUG	
	LOGD("begin");
#endif

	g_ppg_alg_mode = ALG_MODE_BPT;
	g_ppg_data = PPG_DATA_BPT;
	g_ppg_bpt_status = BPT_STATUS_GET_EST;

	ppg_start_flag = true;
}

void PPGRestartToBpt(void)
{
	k_timer_start(&ppg_bpt_est_start_timer, K_MSEC(500), K_NO_WAIT);
}

void PPGGetSensorHubData(void)
{
	int ret = 0;
	int num_bytes_to_read = 0;
	uint8_t hubStatus = 0;
	uint8_t databuf[READ_SAMPLE_COUNT_MAX*SS_NORMAL_BPT_PACKAGE_SIZE] = {0};
	whrm_wspo2_suite_sensorhub_data sensorhub_out = {0};
	bpt_sensorhub_data bpt = {0};
	accel_data accel = {0};
	max86176_data max86176 = {0};
	notify_infor infor = {0};
#ifdef FONTMAKER_UNICODE_FONT
    LCD_SetFontSize(FONT_SIZE_20);
#else		
    LCD_SetFontSize(FONT_SIZE_16);
#endif	
    infor.x = 0;
	infor.y = 0;
	infor.w = LCD_WIDTH;
	infor.h = LCD_HEIGHT;
	infor.align = NOTIFY_ALIGN_CENTER;
	infor.type = NOTIFY_TYPE_POPUP;

	ret = sh_get_sensorhub_status(&hubStatus);
#ifdef PPG_DEBUG	
	LOGD("ret:%d, hubStatus:%d", ret, hubStatus);
#endif
	if(hubStatus & SS_MASK_STATUS_FIFO_OUT_OVR)
	{
	#ifdef PPG_DEBUG
		LOGD("SS_MASK_STATUS_FIFO_OUT_OVR");
	#endif
	}

	if((0 == ret) && (hubStatus & SS_MASK_STATUS_DATA_RDY))
	{
		int u32_sampleCnt = 0;

	#ifdef PPG_DEBUG
		LOGD("FIFO ready");
	#endif

		num_bytes_to_read += SS_PACKET_COUNTERSIZE;
		if(g_algo_sensor_stat.max86176_enabled)
			num_bytes_to_read += SSMAX86176_MODE1_DATASIZE;
		if(g_algo_sensor_stat.accel_enabled)
			num_bytes_to_read += SSACCEL_MODE1_DATASIZE;
		if(g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1)
			num_bytes_to_read += SSWHRM_WSPO2_SUITE_MODE1_DATASIZE;
		if(g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2)
			num_bytes_to_read += SSWHRM_WSPO2_SUITE_MODE2_DATASIZE;
		if(g_algo_sensor_stat.bpt_algo_enabled)
			num_bytes_to_read += SSBPT_ALGO_DATASIZE;
		if(g_algo_sensor_stat.algo_raw_enabled)
			num_bytes_to_read += SSRAW_ALGO_DATASIZE;

		ret = sensorhub_get_output_sample_number(&u32_sampleCnt);
		if(ret == SS_SUCCESS)
		{
		#ifdef PPG_DEBUG
			LOGD("sample count is:%d", u32_sampleCnt);
		#endif
		}
		else
		{
		#ifdef PPG_DEBUG
			LOGD("read sample count fail:%d", ret);
		#endif
		}

		WAIT_MS(5);

		if(u32_sampleCnt > READ_SAMPLE_COUNT_MAX)
			u32_sampleCnt = READ_SAMPLE_COUNT_MAX;
		
		ret = sh_read_fifo_data(u32_sampleCnt, num_bytes_to_read, databuf, sizeof(databuf));
		if(ret == SS_SUCCESS)
		{
			uint16_t heart_rate=0,blood_oxy=0;
			uint32_t i,j,index = 0;
			uint8_t scd_status = 0;
			static uint8_t count=0;

			for(i=0,j=0;i<u32_sampleCnt;i++)
			{
				index = i * num_bytes_to_read + 1;

				if(g_ppg_alg_mode == ALG_MODE_BPT)
				{
					bpt_algo_data_rx(&bpt, &databuf[index+SS_PACKET_COUNTERSIZE + SSMAX86176_MODE1_DATASIZE + SSACCEL_MODE1_DATASIZE + SSWHRM_WSPO2_SUITE_MODE1_DATASIZE]);
				
				#ifdef PPG_DEBUG
					LOGD("bpt_status:%d, bpt_per:%d, bpt_sys:%d, bpt_dia:%d", bpt.status, bpt.perc_comp, bpt.sys_bp, bpt.dia_bp);
					switch(bpt.status)
					{
					case 0:
						LOGD("No signal");
						break;
					case 1:
						LOGD("User calibration/estimation in progress ");
						break;
					case 2:
						LOGD("Success");					
						break;
					case 3:
						LOGD("Weak signal");						
						break;
					case 4:
						LOGD("Motion");						
						break;
					case 5:
						LOGD("Estimation failure");						
						break;
					case 6:
						LOGD("Calibration partially complete");							
						break;
					case 7:
						LOGD("Subject initialization failure");					
						break;
					case 8:
						LOGD("Initialization completed");						
						break;
					case 9:
						LOGD("Calibration reference BP trending error");						
						break;
					case 10:
						LOGD("Calibration reference Inconsistency 1 error");						
						break;
					case 11:
						LOGD("Calibration reference Inconsistency 2 error");						
						break;
					case 12:
						LOGD("Calibration reference Inconsistency 3 error");							
						break;
					case 13:
						LOGD("Calibration reference count mismatch");					
						break;
					case 14:
						LOGD("Calibration reference are out of limits (systolic 80 to 180, diastolic 50 to 120)");						
						break;
					case 15:
						LOGD("Number of calibrations exceed maximum");						
						break;
					case 16:
						LOGD("Pulse pressure out of range");							
						break;
					case 17:
						LOGD("Heart rate out of range");					
						break;
					case 18:
						LOGD("Heart rate is above resting");						
						break;
					case 19:
						LOGD("Perfusion Index is out of range");						
						break;
					case 20:
						LOGD("Estimation error, try again");							
						break;
					case 21:
						LOGD("BPT estimate is out of range from calibration references (systolic +-30, diastolic +-20) ");					
						break;
					case 22:
						LOGD("BPT estimate is beyond the maximum limits (systolic 80 to 180, diastolic 50 to 120)");				
						break;
					default:
						LOGD("Unknow");
						break;
					}
				#endif

					if(g_ppg_bpt_status == BPT_STATUS_GET_CAL)
					{
						if((bpt.status == 2) && (bpt.perc_comp == 100))
						{
							//get calbration data success
						#ifdef PPG_DEBUG
							LOGD("get calbration data success!");
						#endif
							//ppg_bpt_is_calbraed = true;
							sh_get_bpt_cal_data();
							ppg_bpt_cal_need_update = false;

							ppg_stop_cal_flag = true;
							PPGRestartToBpt();
						}
						
						return;
					}
					else if(g_ppg_bpt_status == BPT_STATUS_GET_EST)
					{
						g_bpt.systolic = bpt.sys_bp;
						g_bpt.diastolic = bpt.dia_bp;
							
						if((bpt.status == 2) && (bpt.perc_comp == 100))
						{
							//get bpt data success
						#ifdef PPG_DEBUG
							LOGD("get bpt data success!");
						#endif

							get_bpt_ok_flag = true;
							ppg_stop_flag = true;
						}
					}
				}
				else if(g_ppg_alg_mode == ALG_MODE_HR_SPO2)
				{
				#ifdef CONFIG_FACTORY_TEST_SUPPORT
					if(IsFTPPGTesting())
						max86176_data_rx(&max86176, &databuf[index+SS_PACKET_COUNTERSIZE + SSACCEL_MODE1_DATASIZE]);
				#endif	
					
					whrm_wspo2_suite_data_rx_mode1(&sensorhub_out, &databuf[index+SS_PACKET_COUNTERSIZE + SSMAX86176_MODE1_DATASIZE + SSACCEL_MODE1_DATASIZE]);
					
				#ifdef PPG_DEBUG
					LOGD("skin:%d, hr:%d, spo2:%d", sensorhub_out.scd_contact_state, sensorhub_out.hr, sensorhub_out.spo2);
				#endif

					if(sensorhub_out.scd_contact_state == 3)	//Skin contact state:0: Undetected 1: Off skin 2: On some subject 3: On skin
					{
						heart_rate += sensorhub_out.hr;
						blood_oxy += sensorhub_out.spo2;
						j++;
					}
				}
			}

		#ifdef CONFIG_FACTORY_TEST_SUPPORT
			if(IsFTPPGTesting())
			{
				uint8_t ft_hr,ft_spo2;

				ft_hr = sensorhub_out.hr/10 + ((sensorhub_out.hr%10 > 4) ? 1 : 0);
				ft_spo2 = sensorhub_out.spo2/10 + ((sensorhub_out.spo2%10 > 4) ? 1 : 0);
				sprintf(ppg_test_info, "Green: %d, %d\nIR:           %d, %d\nRed:       %d, %d\nSkin:      %d\nHR:          %d   SPO2:      %d", 
																				max86176.led1,max86176.led2,
																				max86176.led3,max86176.led4,
																				max86176.led5,max86176.led6,
																				sensorhub_out.scd_contact_state,
																				ft_hr,ft_spo2);

				FTPPGStatusUpdate(ft_hr, ft_spo2);
				return;
			}
		#endif

			if(u32_sampleCnt > 1)
				index = (u32_sampleCnt-1) * num_bytes_to_read + 1;
			else
				index = 1;

			if((((g_ppg_trigger&TRIGGER_BY_SCC) != 0) || (g_ppg_trigger == TRIGGER_BY_MENU))
			#ifdef CONFIG_FACTORY_TEST_SUPPORT
				&& !IsFTPPGTesting()
				&& !IsFTPPGAging()
			#endif
				)
			{
				sensorhub_get_output_scd_state(&databuf[index + SS_PACKET_COUNTERSIZE + SSMAX86176_MODE1_DATASIZE + SSACCEL_MODE1_DATASIZE], &scd_status);
			#ifdef PPG_DEBUG
				LOGD("scd_status:%d", scd_status);
			#endif
				if(!ppg_stop_flag)
				{
					if(scd_status == 3)
						scc_check_sum = SCC_COMPARE_MAX;
					else
						scc_check_sum--;

					count++;
					if(count >= SCC_COMPARE_MAX)
					{
						count = 0;
						if(scc_check_sum != 0)
							ppg_skin_contacted_flag = true;
						else
							ppg_skin_contacted_flag = false;

						k_timer_stop(&ppg_skin_check_timer);
						scc_check_sum = SCC_COMPARE_MAX;
					#ifdef PPG_DEBUG
						LOGD("scc check completed! scc_check_sum:%d,flag:%d", scc_check_sum,ppg_skin_contacted_flag);
					#endif
						if(g_ppg_trigger == TRIGGER_BY_SCC)
						{	
							ppg_stop_flag = true;
							return;
						}
						else
						{
							if(!ppg_skin_contacted_flag)
							{
							#ifdef PPG_DEBUG
								LOGD("No Skin Contact - PPG Stopped");
							#endif
								ppg_stop_flag = true;
								if((g_ppg_trigger&TRIGGER_BY_MENU) != 0)
								{
									infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
									infor.img_count = 1;
									DisplayPopUp(infor);
								}
								return;
							}
						}
					}
				}
			}

			if(g_ppg_alg_mode == ALG_MODE_HR_SPO2)
			{
				if(j > 0)
				{
					heart_rate = heart_rate/j;
					blood_oxy = blood_oxy/j;
				}

			#ifdef PPG_DEBUG
				LOGD("avra hr:%d, spo2:%d", heart_rate, blood_oxy);
			#endif
				if(g_ppg_data == PPG_DATA_HR)
				{
					uint8_t hr = 0;
					
					hr = heart_rate/10 + ((heart_rate%10 > 4) ? 1 : 0);
				#ifdef PPG_DEBUG
					LOGD("hr:%d", hr);
				#endif

					if((hr >= PPG_HR_MIN)&&(hr <= PPG_HR_MAX))
					{
						for(i=0;i<sizeof(temp_hr)/sizeof(temp_hr[0]);i++)
						{
							uint8_t k;
							
							if(temp_hr[i] == 0)
							{
								temp_hr[i] = hr;
								break;
							}
						#if 0	//xb add 2024-02-01 The heart rate is no longer sorted, and the first few sets of data are directly filtered
							else if(temp_hr[i] >= hr)
							{
								for(k=sizeof(temp_hr)/sizeof(temp_hr[0])-1;k>=i+1;k--)
								{
									temp_hr[k] = temp_hr[k-1];
								}
								temp_hr[i] = hr;
								break;
							}
						#endif	
						}

					#ifdef PPG_DEBUG
						for(i=0;i<sizeof(temp_hr)/sizeof(temp_hr[0]);i++)
						{		
							LOGD("temp_hr:%d", temp_hr[i]);
						}
						LOGD("temp_hr_count:%d", temp_hr_count);
					#endif

						temp_hr_count++;
						if(temp_hr_count >= sizeof(temp_hr)/sizeof(temp_hr[0]))
						{
							uint16_t hr_data = 0;
							
							for(i=PPG_HR_DEL_MIN_NUM;i<temp_hr_count;i++)
							{
								hr_data += temp_hr[i];
							}

							g_hr = hr_data/(temp_hr_count-PPG_HR_DEL_MIN_NUM);
							temp_hr_count = 0;
							
							get_hr_ok_flag = true;
							ppg_stop_flag = true;
						#ifdef PPG_DEBUG
							LOGD("get hr success! hr:%d", g_hr);
						#endif
						}
					}
				}
				else if(g_ppg_data == PPG_DATA_SPO2)
				{
					uint8_t spo2 = 0;
					
					spo2 = blood_oxy/10 + ((blood_oxy%10 > 4) ? 1 : 0);
				#ifdef PPG_DEBUG
					LOGD("spo2:%d", spo2);
				#endif
					if((spo2 >= PPG_SPO2_MIN)&&(spo2 <= PPG_SPO2_MAX))
					{
						for(i=0;i<sizeof(temp_spo2)/sizeof(temp_spo2[0]);i++)
						{
							uint8_t k;
							
							if(temp_spo2[i] == 0)
							{
								temp_spo2[i] = spo2;
								break;
							}
							else if(temp_spo2[i] >= spo2)
							{
								for(k=sizeof(temp_spo2)/sizeof(temp_spo2[0])-1;k>=i+1;k--)
								{
									temp_spo2[k] = temp_spo2[k-1];
								}
								temp_spo2[i] = spo2;
								break;
							}
						}

					#ifdef PPG_DEBUG
						for(i=0;i<sizeof(temp_spo2)/sizeof(temp_spo2[0]);i++)
						{		
							LOGD("temp_spo2:%d", temp_spo2[i]);
						}
						LOGD("temp_spo2_count:%d", temp_spo2_count);
					#endif

						temp_spo2_count++;
						if(temp_spo2_count >= sizeof(temp_spo2)/sizeof(temp_spo2[0]))
						{
							uint16_t spo2_data = 0;
							
							for(i=PPG_SPO2_DEL_MIN_NUM;i<temp_spo2_count;i++)
							{
								spo2_data += temp_spo2[i];
							}
							
							g_spo2 = spo2_data/(temp_spo2_count-PPG_SPO2_DEL_MIN_NUM);
							temp_spo2_count = 0;

							get_spo2_ok_flag = true;
							ppg_stop_flag = true;
						#ifdef PPG_DEBUG
							LOGD("get spo2 success! spo2:%d", g_spo2);
						#endif
						}
					}
				}
			}
		}
		else
		{
		#ifdef PPG_DEBUG
			LOGD("read FIFO result fail:%d", ret);
		#endif
		}
	}
	else
	{
	#ifdef PPG_DEBUG
		LOGD("FIFO status is not ready:%d,%d", ret, hubStatus);
	#endif
	}

	if((g_ppg_data == PPG_DATA_BPT)&&(screen_id == SCREEN_ID_BP))
	{
		static bool flag = false;
		
		flag = !flag;
		if(flag || get_bpt_ok_flag)
			ppg_redraw_data_flag = true;
	}
	else if((g_ppg_data == PPG_DATA_SPO2)&&(screen_id == SCREEN_ID_SPO2))
	{
		ppg_redraw_data_flag = true;
	}
	else if((g_ppg_data == PPG_DATA_HR)&&(screen_id == SCREEN_ID_HR))
	{
		ppg_redraw_data_flag = true;
	}
}

void ppg_delay_start_timerout(struct k_timer *timer_id)
{
	ppg_delay_start_flag = true;
}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void FTStartPPG(void)
{
	ft_start_hr = true;
}

void FTStopPPG(void)
{
	ppg_stop_flag = true;
}
#endif

void StartSCC(void)
{
	if(PPGIsWorking())
		return;

	g_ppg_trigger |= TRIGGER_BY_SCC;
	g_ppg_data = PPG_DATA_HR;
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;

	ppg_skin_contacted_flag = true;
    ppg_start_flag = true;
	k_timer_start(&ppg_skin_check_timer, K_SECONDS(10), K_NO_WAIT);
}

bool CheckSCC(void)
{
	return ppg_skin_contacted_flag;
}

void StartPPG(PPG_DATA_TYPE data_type, PPG_TRIGGER_SOURCE trigger_type)
{
	notify_infor infor = {0};

#ifdef PPG_DEBUG	
	LOGD("data:%d, type:%d", data_type, trigger_type);
#endif

	if(PPGIsSccCheck())
		PPGStopCheck();

#ifdef FONTMAKER_UNICODE_FONT
	LCD_SetFontSize(FONT_SIZE_20);
#else
	LCD_SetFontSize(FONT_SIZE_16);
#endif
	infor.x = 0;
	infor.y = 0;
	infor.w = LCD_WIDTH;
	infor.h = LCD_HEIGHT;
	infor.align = NOTIFY_ALIGN_CENTER;
	infor.type = NOTIFY_TYPE_POPUP;

	switch(trigger_type)
	{
	case TRIGGER_BY_HOURLY:
		if(!is_wearing())
		{
			return;
		}

		if(IsInPPGScreen())
		{
			PPGScreenStopTimer();
			if(PPGIsWorking())
				PPGStopCheck();

			infor.img_count = 0;
			mmi_ucs2cpy(infor.text, (uint8_t*)&str_timing_note[global_settings.language]);
			DisplayPopUp(infor);

			g_ppg_data = data_type;
			k_timer_start(&ppg_delay_start_timer, K_MSEC((NOTIFY_TIMER_INTERVAL+1)*1000), K_NO_WAIT);
			return;
		}

		switch(data_type)
		{
		case PPG_DATA_HR:
			g_hr_hourly = 0;
			break;
		case PPG_DATA_SPO2:
			g_spo2_hourly = 0;
			break;
		case PPG_DATA_BPT:
			memset(&g_bpt_hourly, 0x00, sizeof(bpt_data));
			break;
		}
		break;
		
	case TRIGGER_BY_MENU:
		if(!is_wearing())
		{
			infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
			infor.img_count = 1;
			DisplayPopUp(infor);
			return;
		}

		if(PPGIsWorkingTiming())
		{
			infor.img_count = 0;
			mmi_ucs2cpy(infor.text, (uint8_t*)str_running_note[global_settings.language]);
			DisplayPopUp(infor);
			return;
		}

		switch(data_type)
		{
		case PPG_DATA_HR:
			g_hr_menu = 0;
			break;
		case PPG_DATA_SPO2:
			g_spo2_menu = 0;
			break;
		case PPG_DATA_BPT:
			memset(&g_bpt_menu, 0x00, sizeof(bpt_data));
			break;
		}
		break;

#ifdef CONFIG_BLE_SUPPORT
	case TRIGGER_BY_APP_ONE_KEY:
		if(!is_wearing())
		{
			MCU_send_app_one_key_measure_data();
			return;
		}
		if(PPGIsWorking())
		{
			if(g_ppg_data == PPG_DATA_HR)
				g_ppg_trigger |= trigger_type;
			else
				MCU_send_app_one_key_measure_data();

			return;
		}
		break;
		
	case TRIGGER_BY_APP:
		if(!is_wearing())
		{
			uint8_t hr = 0;
			
			MCU_send_app_get_ppg_data(data_type, &hr);
			return;
		}
		if(PPGIsWorking())
		{
			if(g_ppg_data == PPG_DATA_HR)
				g_ppg_trigger |= trigger_type;
			else
				MCU_send_app_get_ppg_data(data_type, &g_hr);

			return;
		}
		break;
#endif

	case TRIGGER_BY_FT:
		g_ppg_trigger = TRIGGER_BY_FT;
		g_ppg_alg_mode = ALG_MODE_HR_SPO2;
		g_ppg_data = PPG_DATA_HR;
		ppg_start_flag = true;
		return;
		
	default:
		return;
	}

	g_ppg_trigger |= trigger_type;
	g_ppg_data = data_type;
	switch(data_type)
	{
	case PPG_DATA_HR:
		g_ppg_alg_mode = ALG_MODE_HR_SPO2;
		g_hr = 0;
		temp_hr_count = 0;
		memset(&temp_hr, 0x00, sizeof(temp_hr));	
		get_hr_ok_flag = false;
		break;
		
	case PPG_DATA_SPO2:
		g_ppg_alg_mode = ALG_MODE_HR_SPO2;
		g_spo2 = 0;
		temp_spo2_count	= 0;
		memset(&temp_spo2, 0x00, sizeof(temp_spo2));
		get_spo2_ok_flag = false;
		break;
		
	case PPG_DATA_BPT:
		g_ppg_alg_mode = ALG_MODE_BPT;
		memset(&g_bpt, 0, sizeof(bpt_data));
		get_bpt_ok_flag = false;
		break;
	}

	ppg_start_flag = true;
}

void MenuStartHr(void)
{
	menu_start_hr = true;
}

void MenuStopHr(void)
{
	ppg_stop_flag = true;
}

void MenuStartSpo2(void)
{
	menu_start_spo2 = true;
}

void MenuStopSpo2(void)
{
	ppg_stop_flag = true;
}

void MenuStartBpt(void)
{
	menu_start_bpt = true;
}

void MenuStopBpt(void)
{
	ppg_stop_flag = true;
}

void MenuStartPPG(void)
{
	g_ppg_trigger |= TRIGGER_BY_MENU;
	ppg_start_flag = true;
}

void MenuStopPPG(void)
{
	g_ppg_trigger = 0;
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;
	g_ppg_bpt_status = BPT_STATUS_GET_EST;
	ppg_stop_flag = true;
}

void PPGStartCheck(void)
{
#ifdef PPG_DEBUG
	LOGD("ppg_power_flag:%d", ppg_power_flag);
#endif
	if(ppg_power_flag > 0)
		return;

	if(g_ppg_alg_mode == ALG_MODE_BPT)
		SCC_COMPARE_MAX = 3*PPG_SCC_COUNT_MAX;
	else
		SCC_COMPARE_MAX = PPG_SCC_COUNT_MAX;
	scc_check_sum = SCC_COMPARE_MAX;
	
	PPG_Enable();
	PPG_Power_On();
	PPG_i2c_on();
	
	ppg_power_flag = 1;

	StartSensorhub();
}

void PPGStopCheck(void)
{
	int status = -1;
	
#ifdef PPG_DEBUG
	LOGD("ppg_power_flag:%d", ppg_power_flag);
#endif
	k_timer_stop(&ppg_appmode_timer);
	k_timer_stop(&ppg_stop_timer);
	k_timer_stop(&ppg_menu_stop_timer);
	k_timer_stop(&ppg_get_hr_timer);
	k_timer_stop(&ppg_delay_start_timer);
	k_timer_stop(&ppg_bpt_est_start_timer);
	k_timer_stop(&ppg_skin_check_timer);

	if(ppg_power_flag == 0)
		return;

	sensorhub_disable_sensor();
	sensorhub_disable_algo();

	PPG_i2c_off();
	PPG_Power_Off();
	PPG_Disable();

	ppg_power_flag = 0;

#ifdef CONFIG_BLE_SUPPORT
	if((g_ppg_trigger&TRIGGER_BY_APP_ONE_KEY) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_APP_ONE_KEY);
		MCU_send_app_one_key_measure_data();
	}
	if((g_ppg_trigger&TRIGGER_BY_APP) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_APP);
		switch(g_ppg_data)
		{
		case PPG_DATA_HR:
			MCU_send_app_get_ppg_data(g_ppg_data, &g_hr);
			break;
		case PPG_DATA_SPO2:
			MCU_send_app_get_ppg_data(g_ppg_data, &g_spo2);
			break;
		case PPG_DATA_BPT:
			MCU_send_app_get_ppg_data(g_ppg_data, (uint8_t*)&g_bpt);
			break;
		}
	}
#endif	
	if((g_ppg_trigger&TRIGGER_BY_MENU) != 0)
	{
		bool flag = false;
		
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_MENU);
		switch(g_ppg_data)
		{
		case PPG_DATA_HR:
			if(get_hr_ok_flag)
			{
				flag = true;
				g_hr_menu = g_hr;
				UpdateLastPPGData(date_time, PPG_DATA_HR, &g_hr_menu);

			#ifdef CONFIG_BLE_SUPPORT	
				if(g_ble_connected)
				{
					MCU_send_app_get_ppg_data(PPG_DATA_HR, &g_hr);
				}
			#endif
			}
			break;
		case PPG_DATA_SPO2:
			if(get_spo2_ok_flag)
			{
				flag = true;
				g_spo2_menu = g_spo2;
				UpdateLastPPGData(date_time, PPG_DATA_SPO2, &g_spo2_menu);
				
			#ifdef CONFIG_BLE_SUPPORT	
				if(g_ble_connected)
				{
					MCU_send_app_get_ppg_data(PPG_DATA_SPO2, &g_spo2);
				}
			#endif
			}
			break;
		case PPG_DATA_BPT:
			if(get_bpt_ok_flag)
			{
				if(g_ppg_bpt_status == BPT_STATUS_GET_EST)
				{
					flag = true;
					memcpy(&g_bpt_menu, &g_bpt, sizeof(bpt_data));
					UpdateLastPPGData(date_time, PPG_DATA_BPT, &g_bpt_menu);
					
				#ifdef CONFIG_BLE_SUPPORT	
					if(g_ble_connected)
					{
						MCU_send_app_get_ppg_data(PPG_DATA_BPT, (uint8_t*)&g_bpt);
					}
				#endif	
				}
			}
			break;
		}

		if(flag)
		{
			SyncSendHealthData();
			g_hr_menu = 0;
			g_spo2_menu = 0;
			memset(&g_bpt_menu, 0x00, sizeof(bpt_data));
		}
	}
	if((g_ppg_trigger&TRIGGER_BY_HOURLY) != 0)
	{
		uint8_t tmp_hr,tmp_spo2;
		bpt_data tmp_bpt;

		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_HOURLY);
		switch(g_ppg_data)
		{
		case PPG_DATA_HR:
			tmp_hr = g_hr;
			g_hr_hourly = g_hr;
			if(!ppg_skin_contacted_flag)
				tmp_hr = 0xFE;
			SetCurDayHrRecData(g_health_check_time, tmp_hr);
			StartPPG(PPG_DATA_BPT, TRIGGER_BY_HOURLY);
			break;
			
		case PPG_DATA_SPO2:
			tmp_spo2 = g_spo2;
			g_spo2_hourly = g_spo2;
			if(!ppg_skin_contacted_flag)
				tmp_spo2 = 0xFE;
			SetCurDaySpo2RecData(g_health_check_time, tmp_spo2);
			break;
			
		case PPG_DATA_BPT:
			memcpy(&tmp_bpt, &g_bpt, sizeof(bpt_data));
			memcpy(&g_bpt_hourly, &g_bpt, sizeof(bpt_data));
			if(!ppg_skin_contacted_flag)
				memset(&tmp_bpt, 0xFE, sizeof(bpt_data));
			SetCurDayBptRecData(g_health_check_time, tmp_bpt);
			StartPPG(PPG_DATA_SPO2, TRIGGER_BY_HOURLY);
			break;
		}
	}

	if((g_ppg_trigger&TRIGGER_BY_SCC) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_SCC);
	}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
	if((g_ppg_trigger&TRIGGER_BY_FT) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_FT);
		FTPPGStatusUpdate(0, 0);
		return;
	}
#endif
}

void PPGStopBptCal(void)
{
	k_timer_stop(&ppg_get_hr_timer);
	
	sensorhub_disable_sensor();
	sensorhub_disable_algo();

	PPG_i2c_off();
	PPG_Power_Off();
	PPG_Disable();

	ppg_power_flag = 0;
}

static void ppg_auto_stop_timerout(struct k_timer *timer_id)
{
	ppg_stop_flag = true;
}

static void ppg_skin_check_timerout(struct k_timer *timer_id)
{
	if(g_ppg_trigger == TRIGGER_BY_SCC)
		ppg_stop_flag = true;
}

static void ppg_menu_stop_timerout(struct k_timer *timer_id)
{
	ppg_stop_flag = true;
	
	if((screen_id == SCREEN_ID_HR)
		||(screen_id == SCREEN_ID_SPO2)
		||(screen_id == SCREEN_ID_BP)
		)
	{
		g_ppg_status = PPG_STATUS_MEASURE_FAIL;

		if(screen_id == SCREEN_ID_HR)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
		else if(screen_id == SCREEN_ID_SPO2)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
		else if(screen_id == SCREEN_ID_BP)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BP;
		
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void PPG_init(void)
{
#ifdef PPG_DEBUG
	LOGD("PPG_init");
#endif

	//Display the last record within 7 days.
	get_cur_health_from_record(&last_health);
	DateIncrease(&last_health.hr_rec.timestamp, 7);
	if(DateCompare(last_health.hr_rec.timestamp, date_time) > 0)
	{
		g_hr = last_health.hr_rec.hr;
	}

	DateIncrease(&last_health.spo2_rec.timestamp, 7);
	if(DateCompare(last_health.spo2_rec.timestamp, date_time) > 0)
	{
		g_spo2 = last_health.spo2_rec.spo2;
	}

	DateIncrease(&last_health.bpt_rec.timestamp, 7);
	if(DateCompare(last_health.bpt_rec.timestamp, date_time) > 0)
	{
		g_bpt.systolic = last_health.bpt_rec.bpt.systolic;
		g_bpt.diastolic = last_health.bpt_rec.bpt.diastolic;
	}

	if(!sh_init_interface())
		return;

#ifdef PPG_DEBUG	
	LOGD("PPG_init done!");
#endif
}

void PPGMsgProcess(void)
{
	if(ppg_int_event)
	{
		ppg_int_event = false;
	}
	
	if(ppg_fw_upgrade_flag)
	{
		SH_OTA_upgrade_process();
		ppg_fw_upgrade_flag = false;
	}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
	if(ft_start_hr)
	{
		StartPPG(PPG_DATA_HR, TRIGGER_BY_FT);
		ft_start_hr = false;
	}
#endif

	if(menu_start_hr)
	{
		StartPPG(PPG_DATA_HR, TRIGGER_BY_MENU);
		menu_start_hr = false;
	}
	
	if(menu_start_spo2)
	{
		StartPPG(PPG_DATA_SPO2, TRIGGER_BY_MENU);
		menu_start_spo2 = false;
	}
	
	if(menu_start_bpt)
	{
		StartPPG(PPG_DATA_BPT, TRIGGER_BY_MENU);
		menu_start_bpt = false;
	}
	
	if(ppg_start_flag)
	{
	#ifdef PPG_DEBUG
		LOGD("PPG start!");
	#endif
		PPGStartCheck();
		ppg_start_flag = false;
	}
	
	if(ppg_stop_flag)
	{
	#ifdef PPG_DEBUG
		LOGD("PPG stop!");
	#endif
		PPGStopCheck();
		ppg_stop_flag = false;
	}

	if(ppg_stop_cal_flag)
	{
	#ifdef PPG_DEBUG	
		LOGD("bpt cal stop");
	#endif
		PPGStopBptCal();
		ppg_stop_cal_flag = false;
	}
	
	if(ppg_get_cal_flag)
	{
		ppg_get_cal_flag = false;
	}
	
	if(ppg_get_data_flag)
	{
		PPGGetSensorHubData();
		ppg_get_data_flag = false;
	}
	
	if(ppg_redraw_data_flag)
	{
		PPGRedrawData();
		ppg_redraw_data_flag = false;
	}

	if(ppg_delay_start_flag)
	{
		switch(g_ppg_data)
		{
		case PPG_DATA_HR:
			StartPPG(PPG_DATA_HR, TRIGGER_BY_HOURLY);
			break;

		case PPG_DATA_SPO2:
			StartPPG(PPG_DATA_SPO2, TRIGGER_BY_HOURLY);
			break;

		case PPG_DATA_BPT:
			StartPPG(PPG_DATA_BPT, TRIGGER_BY_HOURLY);
			break;
		}

		ppg_delay_start_flag = false;
	}

	if(ppg_appmode_init_flag)
	{
		StartSensorhubCallBack();
		ppg_appmode_init_flag = false;
	}
}
