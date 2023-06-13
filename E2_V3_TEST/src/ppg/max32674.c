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
#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <nrf_socket.h>
#include <random/rand32.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <nrfx.h>
#include "uart_ble.h"
#include "Max32674.h"
#include "screen.h"
#include "max_sh_interface.h"
#include "max_sh_api.h"
#include "datetime.h"
#include "inner_flash.h"
#include "external_flash.h"
#include "logger.h"

//#define PPG_DEBUG

#define PPG_HR_COUNT_MAX		30
#define PPG_HR_DEL_MIN_NUM		5
#define PPG_SPO2_COUNT_MAX		3
#define PPG_SPO2_DEL_MIN_NUM	1

bool ppg_int_event = false;
bool ppg_bpt_is_calbraed = false;
bool ppg_bpt_cal_need_update = false;
bool get_bpt_ok_flag = false;
bool get_hr_ok_flag = false;
bool get_spo2_ok_flag = false;

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

uint8_t ppg_power_flag = 0;	//0:关闭 1:正在启动 2:启动成功
static uint8_t whoamI=0, rst=0;

uint8_t g_ppg_trigger = 0;
uint8_t g_ppg_data = PPG_DATA_MAX;
uint8_t g_ppg_alg_mode = ALG_MODE_HR_SPO2;
uint8_t g_ppg_bpt_status = BPT_STATUS_GET_EST;
uint8_t g_ppg_ver[64] = {0};

uint8_t g_hr = 0;
uint8_t g_hr_menu = 0;
uint8_t g_spo2 = 0;
uint8_t g_spo2_menu = 0;
bpt_data g_bpt = {0};
bpt_data g_bpt_menu = {0};

static uint8_t temp_hr_count = 0;
static uint8_t temp_spo2_count = 0;
static uint8_t temp_hr[PPG_HR_COUNT_MAX] = {0};
static uint8_t temp_spo2[PPG_SPO2_COUNT_MAX] = {0};

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

void ClearAllBptRecData(void)
{
	uint8_t tmpbuf[PPG_BPT_REC2_DATA_SIZE] = {0xff};

	memset(&g_bpt, 0, sizeof(bpt_data));
	memset(&g_bpt_menu, 0, sizeof(bpt_data));	
	
	SpiFlash_Write(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
}

void SetCurDayBptRecData(bpt_data bpt)
{
	uint8_t i,tmpbuf[PPG_BPT_REC2_DATA_SIZE] = {0};
	ppg_bpt_rec2_data *p_bpt,tmp_bpt = {0};
	sys_date_timer_t temp_date = {0};

	if((bpt.systolic > PPG_BPT_SYS_MAX) || (bpt.systolic < PPG_BPT_SYS_MIN) 
		|| (bpt.diastolic > PPG_BPT_DIA_MAX) || (bpt.diastolic < PPG_BPT_DIA_MIN)
		)
	{
		memset(&bpt, 0, sizeof(bpt_data));
	}

	//It is saved before the hour, but recorded as the hour data, so hour needs to be increased by 1
	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	TimeIncrease(&temp_date, 60);
	
	tmp_bpt.year = temp_date.year;
	tmp_bpt.month = temp_date.month;
	tmp_bpt.day = temp_date.day;
	memcpy(&tmp_bpt.bpt[temp_date.hour], &bpt, sizeof(bpt_data));
	
	SpiFlash_Read(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	p_bpt = tmpbuf;
	if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
		||(p_bpt->month == 0xff || p_bpt->month == 0x00)
		||(p_bpt->day == 0xff || p_bpt->day == 0x00)
		||((p_bpt->year == temp_date.year)&&(p_bpt->month == temp_date.month)&&(p_bpt->day == temp_date.day))
		)
	{
		//直接覆盖写在第一条
		p_bpt->year = temp_date.year;
		p_bpt->month = temp_date.month;
		p_bpt->day = temp_date.day;
		memcpy(&p_bpt->bpt[temp_date.hour], &bpt, sizeof(bpt_data));
		SpiFlash_Write(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	}
	else if((temp_date.year < p_bpt->year)
			||((temp_date.year == p_bpt->year)&&(temp_date.month < p_bpt->month))
			||((temp_date.year == p_bpt->year)&&(temp_date.month == p_bpt->month)&&(temp_date.day < p_bpt->day))
			)
	{
		uint8_t databuf[PPG_BPT_REC2_DATA_SIZE] = {0};
		
		//插入新的第一条,旧的第一条到第六条往后挪，丢掉最后一个
		memcpy(&databuf[0*sizeof(ppg_bpt_rec2_data)], &tmp_bpt, sizeof(ppg_bpt_rec2_data));
		memcpy(&databuf[1*sizeof(ppg_bpt_rec2_data)], &tmpbuf[0*sizeof(ppg_bpt_rec2_data)], 6*sizeof(ppg_bpt_rec2_data));
		SpiFlash_Write(databuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	}
	else
	{
		uint8_t databuf[PPG_BPT_REC2_DATA_SIZE] = {0};
		
		//寻找合适的插入位置
		for(i=0;i<7;i++)
		{
			p_bpt = tmpbuf+i*sizeof(ppg_bpt_rec2_data);
			if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
				||(p_bpt->month == 0xff || p_bpt->month == 0x00)
				||(p_bpt->day == 0xff || p_bpt->day == 0x00)
				||((p_bpt->year == temp_date.year)&&(p_bpt->month == temp_date.month)&&(p_bpt->day == temp_date.day))
				)
			{
				//直接覆盖写
				p_bpt->year = temp_date.year;
				p_bpt->month = temp_date.month;
				p_bpt->day = temp_date.day;
				memcpy(&p_bpt->bpt[temp_date.hour], &bpt, sizeof(bpt_data));
				SpiFlash_Write(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
				return;
			}
			else if((temp_date.year > p_bpt->year)
				||((temp_date.year == p_bpt->year)&&(temp_date.month > p_bpt->month))
				||((temp_date.year == p_bpt->year)&&(temp_date.month == p_bpt->month)&&(temp_date.day > p_bpt->day))
				)
			{
				if(i < 6)
				{
					p_bpt++;
					if((temp_date.year < p_bpt->year)
						||((temp_date.year == p_bpt->year)&&(temp_date.month < p_bpt->month))
						||((temp_date.year == p_bpt->year)&&(temp_date.month == p_bpt->month)&&(temp_date.day < p_bpt->day))
						)
					{
						break;
					}
				}
			}
		}

		if(i<6)
		{
			//找到位置，插入新数据，老数据整体往后挪，丢掉最后一个
			memcpy(&databuf[0*sizeof(ppg_bpt_rec2_data)], &tmpbuf[0*sizeof(ppg_bpt_rec2_data)], (i+1)*sizeof(ppg_bpt_rec2_data));
			memcpy(&databuf[(i+1)*sizeof(ppg_bpt_rec2_data)], &tmp_bpt, sizeof(ppg_bpt_rec2_data));
			memcpy(&databuf[(i+2)*sizeof(ppg_bpt_rec2_data)], &tmpbuf[(i+1)*sizeof(ppg_bpt_rec2_data)], (7-(i+2))*sizeof(ppg_bpt_rec2_data));
			SpiFlash_Write(databuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
		}
		else
		{
			//未找到位置，直接接在末尾，老数据整体往前移，丢掉最前一个
			memcpy(&databuf[0*sizeof(ppg_bpt_rec2_data)], &tmpbuf[1*sizeof(ppg_bpt_rec2_data)], 6*sizeof(ppg_bpt_rec2_data));
			memcpy(&databuf[6*sizeof(ppg_bpt_rec2_data)], &tmp_bpt, sizeof(ppg_bpt_rec2_data));
			SpiFlash_Write(databuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
		}
	}
}

void GetCurDayBptRecData(uint8_t *databuf)
{
	uint8_t i,tmpbuf[PPG_BPT_REC2_DATA_SIZE] = {0};
	ppg_bpt_rec2_data bpt_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&bpt_rec2, &tmpbuf[i*sizeof(ppg_bpt_rec2_data)], sizeof(ppg_bpt_rec2_data));
		if((bpt_rec2.year == 0xffff || bpt_rec2.year == 0x0000)||(bpt_rec2.month == 0xff || bpt_rec2.month == 0x00)||(bpt_rec2.day == 0xff || bpt_rec2.day == 0x00))
			continue;
		
		if((bpt_rec2.year == date_time.year)&&(bpt_rec2.month == date_time.month)&&(bpt_rec2.day == date_time.day))
		{
			memcpy(databuf, bpt_rec2.bpt, sizeof(bpt_rec2.bpt));
			break;
		}
	}
}

void GetGivenTimeBptRecData(sys_date_timer_t date, bpt_data *bpt)
{
	uint8_t i,tmpbuf[PPG_BPT_REC2_DATA_SIZE] = {0};
	ppg_bpt_rec2_data bpt_rec2 = {0};

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(bpt == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&bpt_rec2, &tmpbuf[i*sizeof(ppg_bpt_rec2_data)], sizeof(ppg_bpt_rec2_data));
		if((bpt_rec2.year == 0xffff || bpt_rec2.year == 0x0000)||(bpt_rec2.month == 0xff || bpt_rec2.month == 0x00)||(bpt_rec2.day == 0xff || bpt_rec2.day == 0x00))
			continue;
		
		if((bpt_rec2.year == date.year)&&(bpt_rec2.month == date.month)&&(bpt_rec2.day == date.day))
		{
			memcpy(bpt, &bpt_rec2.bpt[date.hour], sizeof(bpt_data));
			break;
		}
	}
}

void ClearAllSpo2RecData(void)
{
	uint8_t tmpbuf[PPG_SPO2_REC2_DATA_SIZE] = {0xff};

	g_spo2 = 0;
	g_spo2_menu = 0;

	SpiFlash_Write(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
}

void SetCurDaySpo2RecData(uint8_t spo2)
{
	uint8_t i,tmpbuf[PPG_SPO2_REC2_DATA_SIZE] = {0};
	ppg_spo2_rec2_data *p_spo2,tmp_spo2 = {0};
	sys_date_timer_t temp_date = {0};
	
	if((spo2 > PPG_SPO2_MAX) || (spo2 < PPG_SPO2_MIN))
		spo2 = 0;

	//It is saved before the hour, but recorded as the hour data, so hour needs to be increased by 1
	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	TimeIncrease(&temp_date, 60);

	tmp_spo2.year = temp_date.year;
	tmp_spo2.month = temp_date.month;
	tmp_spo2.day = temp_date.day;
	tmp_spo2.spo2[temp_date.hour] = spo2;

	SpiFlash_Read(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	p_spo2 = tmpbuf;
	if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
		||(p_spo2->month == 0xff || p_spo2->month == 0x00)
		||(p_spo2->day == 0xff || p_spo2->day == 0x00)
		||((p_spo2->year == temp_date.year)&&(p_spo2->month == temp_date.month)&&(p_spo2->day == temp_date.day))
		)
	{
		//直接覆盖写在第一条
		p_spo2->year = temp_date.year;
		p_spo2->month = temp_date.month;
		p_spo2->day = temp_date.day;
		p_spo2->spo2[temp_date.hour] = spo2;
		SpiFlash_Write(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	}
	else if((temp_date.year < p_spo2->year)
			||((temp_date.year == p_spo2->year)&&(temp_date.month < p_spo2->month))
			||((temp_date.year == p_spo2->year)&&(temp_date.month == p_spo2->month)&&(temp_date.day < p_spo2->day))
			)
	{
		uint8_t databuf[PPG_SPO2_REC2_DATA_SIZE] = {0};
		
		//插入新的第一条,旧的第一条到第六条往后挪，丢掉最后一个
		memcpy(&databuf[0*sizeof(ppg_spo2_rec2_data)], &tmp_spo2, sizeof(ppg_spo2_rec2_data));
		memcpy(&databuf[1*sizeof(ppg_spo2_rec2_data)], &tmpbuf[0*sizeof(ppg_spo2_rec2_data)], 6*sizeof(ppg_spo2_rec2_data));
		SpiFlash_Write(databuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	}
	else
	{
		uint8_t databuf[PPG_SPO2_REC2_DATA_SIZE] = {0};
		
		//寻找合适的插入位置
		for(i=0;i<7;i++)
		{
			p_spo2 = tmpbuf+i*sizeof(ppg_spo2_rec2_data);
			if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
				||(p_spo2->month == 0xff || p_spo2->month == 0x00)
				||(p_spo2->day == 0xff || p_spo2->day == 0x00)
				||((p_spo2->year == temp_date.year)&&(p_spo2->month == temp_date.month)&&(p_spo2->day == temp_date.day))
				)
			{
				//直接覆盖写
				p_spo2->year = temp_date.year;
				p_spo2->month = temp_date.month;
				p_spo2->day = temp_date.day;
				p_spo2->spo2[temp_date.hour] = spo2;
				SpiFlash_Write(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
				return;
			}
			else if((temp_date.year > p_spo2->year)
				||((temp_date.year == p_spo2->year)&&(temp_date.month > p_spo2->month))
				||((temp_date.year == p_spo2->year)&&(temp_date.month == p_spo2->month)&&(temp_date.day > p_spo2->day))
				)
			{
				if(i < 6)
				{
					p_spo2++;
					if((temp_date.year < p_spo2->year)
						||((temp_date.year == p_spo2->year)&&(temp_date.month < p_spo2->month))
						||((temp_date.year == p_spo2->year)&&(temp_date.month == p_spo2->month)&&(temp_date.day < p_spo2->day))
						)
					{
						break;
					}
				}
			}
		}

		if(i<6)
		{
			//找到位置，插入新数据，老数据整体往后挪，丢掉最后一个
			memcpy(&databuf[0*sizeof(ppg_spo2_rec2_data)], &tmpbuf[0*sizeof(ppg_spo2_rec2_data)], (i+1)*sizeof(ppg_spo2_rec2_data));
			memcpy(&databuf[(i+1)*sizeof(ppg_spo2_rec2_data)], &tmp_spo2, sizeof(ppg_spo2_rec2_data));
			memcpy(&databuf[(i+2)*sizeof(ppg_spo2_rec2_data)], &tmpbuf[(i+1)*sizeof(ppg_spo2_rec2_data)], (7-(i+2))*sizeof(ppg_spo2_rec2_data));
			SpiFlash_Write(databuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
		}
		else
		{
			//未找到位置，直接接在末尾，老数据整体往前移，丢掉最前一个
			memcpy(&databuf[0*sizeof(ppg_spo2_rec2_data)], &tmpbuf[1*sizeof(ppg_spo2_rec2_data)], 6*sizeof(ppg_spo2_rec2_data));
			memcpy(&databuf[6*sizeof(ppg_spo2_rec2_data)], &tmp_spo2, sizeof(ppg_spo2_rec2_data));
			SpiFlash_Write(databuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
		}
	}
}

void GetCurDaySpo2RecData(uint8_t *databuf)
{
	uint8_t i,tmpbuf[PPG_SPO2_REC2_DATA_SIZE] = {0};
	ppg_spo2_rec2_data spo2_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&spo2_rec2, &tmpbuf[i*sizeof(ppg_spo2_rec2_data)], sizeof(ppg_spo2_rec2_data));
		if((spo2_rec2.year == 0xffff || spo2_rec2.year == 0x0000)||(spo2_rec2.month == 0xff || spo2_rec2.month == 0x00)||(spo2_rec2.day == 0xff || spo2_rec2.day == 0x00))
			continue;
		
		if((spo2_rec2.year == date_time.year)&&(spo2_rec2.month == date_time.month)&&(spo2_rec2.day == date_time.day))
		{
			memcpy(databuf, spo2_rec2.spo2, sizeof(spo2_rec2.spo2));
			break;
		}
	}
}

void GetGivenTimeSpo2RecData(sys_date_timer_t date, uint8_t *spo2)
{
	uint8_t i,tmpbuf[PPG_SPO2_REC2_DATA_SIZE] = {0};
	ppg_spo2_rec2_data spo2_rec2 = {0};

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(spo2 == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&spo2_rec2, &tmpbuf[i*sizeof(ppg_spo2_rec2_data)], sizeof(ppg_spo2_rec2_data));
		if((spo2_rec2.year == 0xffff || spo2_rec2.year == 0x0000)||(spo2_rec2.month == 0xff || spo2_rec2.month == 0x00)||(spo2_rec2.day == 0xff || spo2_rec2.day == 0x00))
			continue;
		
		if((spo2_rec2.year == date.year)&&(spo2_rec2.month == date.month)&&(spo2_rec2.day == date.day))
		{
			*spo2 = spo2_rec2.spo2[date.hour];
			break;
		}
	}
}

void ClearAllHrRecData(void)
{
	uint8_t tmpbuf[PPG_HR_REC2_DATA_SIZE] = {0xff};

	g_hr = 0;
	g_hr_menu = 0;

	SpiFlash_Write(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
}

void SetCurDayHrRecData(uint8_t hr)
{
	uint8_t i,tmpbuf[PPG_HR_REC2_DATA_SIZE] = {0};
	ppg_hr_rec2_data *p_hr,tmp_hr = {0};
	sys_date_timer_t temp_date = {0};
	
	if((hr > PPG_HR_MAX) || (hr < PPG_HR_MIN))
		hr = 0;

	//It is saved before the hour, but recorded as the hour data, so hour needs to be increased by 1
	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	TimeIncrease(&temp_date, 60);

	tmp_hr.year = temp_date.year;
	tmp_hr.month = temp_date.month;
	tmp_hr.day = temp_date.day;
	tmp_hr.hr[temp_date.hour] = hr;
	
	SpiFlash_Read(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	p_hr = tmpbuf;
	if((p_hr->year == 0xffff || p_hr->year == 0x0000)
		||(p_hr->month == 0xff || p_hr->month == 0x00)
		||(p_hr->day == 0xff || p_hr->day == 0x00)
		||((p_hr->year == temp_date.year)&&(p_hr->month == temp_date.month)&&(p_hr->day == temp_date.day))
		)
	{
		//直接覆盖写在第一条
		p_hr->year = temp_date.year;
		p_hr->month = temp_date.month;
		p_hr->day = temp_date.day;
		p_hr->hr[temp_date.hour] = hr;
		SpiFlash_Write(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	}
	else if((temp_date.year < p_hr->year)
			||((temp_date.year == p_hr->year)&&(temp_date.month < p_hr->month))
			||((temp_date.year == p_hr->year)&&(temp_date.month == p_hr->month)&&(temp_date.day < p_hr->day))
			)
	{
		uint8_t databuf[PPG_HR_REC2_DATA_SIZE] = {0};
		
		//插入新的第一条,旧的第一条到第六条往后挪，丢掉最后一个
		memcpy(&databuf[0*sizeof(ppg_hr_rec2_data)], &tmp_hr, sizeof(ppg_hr_rec2_data));
		memcpy(&databuf[1*sizeof(ppg_hr_rec2_data)], &tmpbuf[0*sizeof(ppg_hr_rec2_data)], 6*sizeof(ppg_hr_rec2_data));
		SpiFlash_Write(databuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	}
	else
	{
		uint8_t databuf[PPG_HR_REC2_DATA_SIZE] = {0};
		
		//寻找合适的插入位置
		for(i=0;i<7;i++)
		{
			p_hr = tmpbuf+i*sizeof(ppg_hr_rec2_data);
			if((p_hr->year == 0xffff || p_hr->year == 0x0000)
				||(p_hr->month == 0xff || p_hr->month == 0x00)
				||(p_hr->day == 0xff || p_hr->day == 0x00)
				||((p_hr->year == temp_date.year)&&(p_hr->month == temp_date.month)&&(p_hr->day == temp_date.day))
				)
			{
				//直接覆盖写
				p_hr->year = temp_date.year;
				p_hr->month = temp_date.month;
				p_hr->day = temp_date.day;
				p_hr->hr[temp_date.hour] = hr;
				SpiFlash_Write(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
				return;
			}
			else if((temp_date.year > p_hr->year)
				||((temp_date.year == p_hr->year)&&(temp_date.month > p_hr->month))
				||((temp_date.year == p_hr->year)&&(temp_date.month == p_hr->month)&&(temp_date.day > p_hr->day))
				)
			{
				if(i < 6)
				{
					p_hr++;
					if((temp_date.year < p_hr->year)
						||((temp_date.year == p_hr->year)&&(temp_date.month < p_hr->month))
						||((temp_date.year == p_hr->year)&&(temp_date.month == p_hr->month)&&(temp_date.day < p_hr->day))
						)
					{
						break;
					}
				}
			}
		}

		if(i<6)
		{
			//找到位置，插入新数据，老数据整体往后挪，丢掉最后一个
			memcpy(&databuf[0*sizeof(ppg_hr_rec2_data)], &tmpbuf[0*sizeof(ppg_hr_rec2_data)], (i+1)*sizeof(ppg_hr_rec2_data));
			memcpy(&databuf[(i+1)*sizeof(ppg_hr_rec2_data)], &tmp_hr, sizeof(ppg_hr_rec2_data));
			memcpy(&databuf[(i+2)*sizeof(ppg_hr_rec2_data)], &tmpbuf[(i+1)*sizeof(ppg_hr_rec2_data)], (7-(i+2))*sizeof(ppg_hr_rec2_data));
			SpiFlash_Write(databuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
		}
		else
		{
			//未找到位置，直接接在末尾，老数据整体往前移，丢掉最前一个
			memcpy(&databuf[0*sizeof(ppg_hr_rec2_data)], &tmpbuf[1*sizeof(ppg_hr_rec2_data)], 6*sizeof(ppg_hr_rec2_data));
			memcpy(&databuf[6*sizeof(ppg_hr_rec2_data)], &tmp_hr, sizeof(ppg_hr_rec2_data));
			SpiFlash_Write(databuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
		}
	}
}

void GetCurDayHrRecData(uint8_t *databuf)
{
	uint8_t i,tmpbuf[PPG_HR_REC2_DATA_SIZE] = {0};
	ppg_hr_rec2_data hr_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&hr_rec2, &tmpbuf[i*sizeof(ppg_hr_rec2_data)], sizeof(ppg_hr_rec2_data));
		if((hr_rec2.year == 0xffff || hr_rec2.year == 0x0000)||(hr_rec2.month == 0xff || hr_rec2.month == 0x00)||(hr_rec2.day == 0xff || hr_rec2.day == 0x00))
			continue;
		
		if((hr_rec2.year == date_time.year)&&(hr_rec2.month == date_time.month)&&(hr_rec2.day == date_time.day))
		{
			memcpy(databuf, hr_rec2.hr, sizeof(hr_rec2.hr));
			break;
		}
	}
}

void GetGivenTimeHrRecData(sys_date_timer_t date, uint8_t *hr)
{
	uint8_t i,tmpbuf[PPG_HR_REC2_DATA_SIZE] = {0};
	ppg_hr_rec2_data hr_rec2 = {0};

	if(!CheckSystemDateTimeIsValid(date))
		return;	
	if(hr == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&hr_rec2, &tmpbuf[i*sizeof(ppg_hr_rec2_data)], sizeof(ppg_hr_rec2_data));
		if((hr_rec2.year == 0xffff || hr_rec2.year == 0x0000)||(hr_rec2.month == 0xff || hr_rec2.month == 0x00)||(hr_rec2.day == 0xff || hr_rec2.day == 0x00))
			continue;
		
		if((hr_rec2.year == date.year)&&(hr_rec2.month == date.month)&&(hr_rec2.day == date.day))
		{
			*hr = hr_rec2.hr[date.hour];
			break;
		}
	}
}

void GetPPGData(uint8_t *hr, uint8_t *spo2, uint8_t *systolic, uint8_t *diastolic)
{
	if(hr != NULL)
		*hr = g_hr;
	
	if(spo2 != NULL)
		*spo2 = g_spo2;
	
	if(systolic != NULL)
		*systolic = 120;
	
	if(diastolic != NULL)
		*diastolic = 80;
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

bool PPGIsWorking(void)
{
	if(ppg_power_flag == 0)
		return false;
	else
		return true;
}

void PPGRedrawData(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		if(g_ppg_data == PPG_DATA_HR)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HR;
		else if(g_ppg_data == PPG_DATA_SPO2)
			scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPO2;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
	else if(screen_id == SCREEN_ID_HR || screen_id == SCREEN_ID_SPO2 || screen_id == SCREEN_ID_BP)
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

	g_ppg_trigger |= TRIGGER_BY_HOURLY;
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
					//accel_data_rx(&accel, &databuf[index+SS_PACKET_COUNTERSIZE]);
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

					if(hr > PPG_HR_MIN)
					{
					#if 0	//xb test 2023.04.03 Modify the hr measurement data filtering mode. 
						for(i=0;i<sizeof(temp_hr)/sizeof(temp_hr[0]);i++)
						{
							uint8_t k;
							
							if(temp_hr[i] == 0)
							{
								temp_hr[i] = hr;
								break;
							}
							else if(temp_hr[i] >= hr)
							{
								for(k=sizeof(temp_hr)/sizeof(temp_hr[0])-1;k>=i+1;k--)
								{
									temp_hr[k] = temp_hr[k-1];
								}
								temp_hr[i] = hr;
								break;
							}
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
					#else
						temp_hr_count++;
						if(temp_hr_count >= sizeof(temp_hr)/sizeof(temp_hr[0]))
						{
							temp_hr_count = 0;

							g_hr = hr;
							get_hr_ok_flag = true;
							ppg_stop_flag = true;
						#ifdef PPG_DEBUG
							LOGD("get hr success! hr:%d", g_hr);
						#endif
						}
					#endif
					}
				}
				else if(g_ppg_data == PPG_DATA_SPO2)
				{
					uint8_t spo2 = 0;
					
					spo2 = blood_oxy/10 + ((blood_oxy%10 > 4) ? 1 : 0);
				#ifdef PPG_DEBUG
					LOGD("spo2:%d", spo2);
				#endif
					if(spo2 >= PPG_SPO2_MIN)
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
	else if((g_ppg_data == PPG_DATA_SPO2)&&(screen_id == SCREEN_ID_SPO2 || screen_id == SCREEN_ID_IDLE))
	{
		ppg_redraw_data_flag = true;
	}
	else if((g_ppg_data == PPG_DATA_HR)&&(screen_id == SCREEN_ID_HR || screen_id == SCREEN_ID_IDLE))
	{
		ppg_redraw_data_flag = true;
	}
}

void ppg_delay_start_timerout(struct k_timer *timer_id)
{
	ppg_delay_start_flag = true;
}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void FTTrigger(void)
{
	g_ppg_trigger = TRIGGER_BY_FT;
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;
	g_ppg_data = PPG_DATA_HR;
	ppg_start_flag = true;
}

void FTStartPPG(void)
{
	ft_start_hr = true;
}

void FTStopPPG(void)
{
	ppg_stop_flag = true;
}
#endif

void TimerStartHr(void)
{
	g_hr = 0;
	temp_hr_count = 0;
	memset(&temp_hr, 0x00, sizeof(temp_hr));	
	get_hr_ok_flag = false;
	
	if(is_wearing())
	{
		if(IsInPPGScreen())
		{
			
			notify_infor infor = {0};
			uint16_t str_note[LANGUAGE_MAX][80] = {
													{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
													{0x0044,0x0069,0x0065,0x0020,0x005A,0x0065,0x0069,0x0074,0x006D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0073,0x0074,0x0065,0x0068,0x0074,0x0020,0x006B,0x0075,0x0072,0x007A,0x0020,0x0062,0x0065,0x0076,0x006F,0x0072,0x002C,0x0020,0x0075,0x006E,0x0064,0x0020,0x0064,0x0069,0x0065,0x0073,0x0065,0x0020,0x004D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0069,0x0073,0x0074,0x0020,0x0076,0x006F,0x0072,0x0062,0x0065,0x0069,0x0021,0x0000},//Die Zeitmessung steht kurz bevor, und diese Messung ist vorbei!
													{0x5B9A,0x65F6,0x6D4B,0x91CF,0x5373,0x5C06,0x5F00,0x59CB,0xFF0C,0x672C,0x6B21,0x6D4B,0x91CF,0x7ED3,0x675F,0xFF01,0x0000},//定时测量即将开始，本次测量结束！
												};

			PPGScreenStopTimer();
			if(PPGIsWorking())
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

			infor.img_count = 0;
			mmi_ucs2cpy(infor.text, (uint8_t*)&str_note[global_settings.language]);

			DisplayPopUp(infor);

			g_ppg_data = PPG_DATA_HR;
			k_timer_start(&ppg_delay_start_timer, K_MSEC((NOTIFY_TIMER_INTERVAL+1)*1000), K_NO_WAIT);
		}
		else
		{
			g_ppg_trigger |= TRIGGER_BY_HOURLY;
			g_ppg_alg_mode = ALG_MODE_HR_SPO2;
			g_ppg_data = PPG_DATA_HR;
			ppg_start_flag = true;	
		}
	}
}

void APPStartHr(void)
{
	uint8_t hr = 0;
	
	g_hr = 0;
	temp_hr_count = 0;
	memset(&temp_hr, 0x00, sizeof(temp_hr));
	get_hr_ok_flag = false;

	if(is_wearing())
	{
		g_ppg_trigger |= TRIGGER_BY_APP;
		g_ppg_alg_mode = ALG_MODE_HR_SPO2;
		g_ppg_data = PPG_DATA_HR;
		ppg_start_flag = true;	
	}
	else
	{
		MCU_send_app_get_ppg_data(PPG_DATA_HR, &hr);
	}
}

void MenuTriggerHr(void)
{
	if(!is_wearing())
	{
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

		infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
		infor.img_count = 1;

		DisplayPopUp(infor);
		
		return;
	}

	if(PPGIsWorkingTiming())
	{
		notify_infor infor = {0};
		uint16_t str_note[LANGUAGE_MAX][64] = {
											{0x0054,0x0068,0x0065,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0069,0x0073,0x0020,0x0072,0x0075,0x006E,0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
											{0x0044,0x0065,0x0072,0x0020,0x0053,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x006C,0x00E4,0x0075,0x0066,0x0074,0x002C,0x0020,0x0062,0x0069,0x0074,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0073,0x0070,0x00E4,0x0074,0x0065,0x0072,0x0020,0x006E,0x006F,0x0063,0x0068,0x0020,0x0065,0x0069,0x006E,0x006D,0x0061,0x006C,0x0021,0x0000},//Der Sensor l?uft, bitte versuchen Sie es sp?ter noch einmal!
											{0x4F20,0x611F,0x5668,0x6B63,0x5728,0x8FD0,0x884C,0xFF0C,0x8BF7,0x7A0D,0x540E,0x518D,0x8BD5,0xFF01,0x0000},//传感器正在运行，请稍后再试！
										};

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

		infor.img_count = 0;
		mmi_ucs2cpy(infor.text, (uint8_t*)str_note[global_settings.language]);

		DisplayPopUp(infor);
		return;
	}

	g_hr = 0;
	g_hr_menu = 0;
	temp_hr_count = 0;
	memset(&temp_hr, 0x00, sizeof(temp_hr));
	get_hr_ok_flag = false;
	
	g_ppg_trigger |= TRIGGER_BY_MENU;
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;
	g_ppg_data = PPG_DATA_HR;
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

void TimerStartSpo2(void)
{
	g_spo2 = 0;
	temp_spo2_count	= 0;
	memset(&temp_spo2, 0x00, sizeof(temp_spo2));
	get_spo2_ok_flag = false;
	
	if(is_wearing())
	{
		if(IsInPPGScreen())
		{
			
			notify_infor infor = {0};
			uint16_t str_note[LANGUAGE_MAX][80] = {
													{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
													{0x0044,0x0069,0x0065,0x0020,0x005A,0x0065,0x0069,0x0074,0x006D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0073,0x0074,0x0065,0x0068,0x0074,0x0020,0x006B,0x0075,0x0072,0x007A,0x0020,0x0062,0x0065,0x0076,0x006F,0x0072,0x002C,0x0020,0x0075,0x006E,0x0064,0x0020,0x0064,0x0069,0x0065,0x0073,0x0065,0x0020,0x004D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0069,0x0073,0x0074,0x0020,0x0076,0x006F,0x0072,0x0062,0x0065,0x0069,0x0021,0x0000},//Die Zeitmessung steht kurz bevor, und diese Messung ist vorbei!
													{0x5B9A,0x65F6,0x6D4B,0x91CF,0x5373,0x5C06,0x5F00,0x59CB,0xFF0C,0x672C,0x6B21,0x6D4B,0x91CF,0x7ED3,0x675F,0xFF01,0x0000},//定时测量即将开始，本次测量结束！
												};

			PPGScreenStopTimer();
			if(PPGIsWorking())
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

			infor.img_count = 0;
			mmi_ucs2cpy(infor.text, (uint8_t*)str_note[global_settings.language]);

			DisplayPopUp(infor);

			g_ppg_data = PPG_DATA_SPO2;
			k_timer_start(&ppg_delay_start_timer, K_MSEC((NOTIFY_TIMER_INTERVAL+1)*1000), K_NO_WAIT);
		}
		else
		{
			g_ppg_trigger |= TRIGGER_BY_HOURLY;
			g_ppg_alg_mode = ALG_MODE_HR_SPO2;
			g_ppg_data = PPG_DATA_SPO2;
			ppg_start_flag = true;	
		}
	}
}

void APPStartSpo2(void)
{
	uint8_t spo2 = 0;
	
	g_spo2 = 0;
	temp_spo2_count	= 0;
	memset(&temp_spo2, 0x00, sizeof(temp_spo2));	
	get_spo2_ok_flag = false;

	if(is_wearing())
	{
		g_ppg_trigger |= TRIGGER_BY_APP;
		g_ppg_alg_mode = ALG_MODE_HR_SPO2;
		g_ppg_data = PPG_DATA_SPO2;
		ppg_start_flag = true;	
	}
	else
	{
		MCU_send_app_get_ppg_data(PPG_DATA_SPO2, &spo2);
	}
}

void MenuTriggerSpo2(void)
{
	if(!is_wearing())
	{
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

		infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
		infor.img_count = 1;

		DisplayPopUp(infor);
		
		return;
	}
	
	if(PPGIsWorkingTiming())
	{
		notify_infor infor = {0};
		uint16_t str_note[LANGUAGE_MAX][64] = {
											{0x0054,0x0068,0x0065,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0069,0x0073,0x0020,0x0072,0x0075,0x006E,0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
											{0x0044,0x0065,0x0072,0x0020,0x0053,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x006C,0x00E4,0x0075,0x0066,0x0074,0x002C,0x0020,0x0062,0x0069,0x0074,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0073,0x0070,0x00E4,0x0074,0x0065,0x0072,0x0020,0x006E,0x006F,0x0063,0x0068,0x0020,0x0065,0x0069,0x006E,0x006D,0x0061,0x006C,0x0021,0x0000},//Der Sensor l?uft, bitte versuchen Sie es sp?ter noch einmal!
											{0x4F20,0x611F,0x5668,0x6B63,0x5728,0x8FD0,0x884C,0xFF0C,0x8BF7,0x7A0D,0x540E,0x518D,0x8BD5,0xFF01,0x0000},//传感器正在运行，请稍后再试！
										};
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

		infor.img_count = 0;
		mmi_ucs2cpy(infor.text, (uint8_t*)str_note[global_settings.language]);

		DisplayPopUp(infor);
		return;
	}

	g_spo2 = 0;
	g_spo2_menu = 0;
	temp_spo2_count	= 0;
	memset(&temp_spo2, 0x00, sizeof(temp_spo2));	
	get_spo2_ok_flag = false;

	g_ppg_trigger |= TRIGGER_BY_MENU;
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;
	g_ppg_data = PPG_DATA_SPO2;
	ppg_start_flag = true;
}

void MenuStartSpo2(void)
{
	menu_start_spo2 = true;
}

void MenuStopSpo2(void)
{
	ppg_stop_flag = true;
}

void TimerStartBpt(void)
{
	memset(&g_bpt, 0, sizeof(bpt_data));
	memset(&g_bpt_menu, 0, sizeof(bpt_data));
	get_bpt_ok_flag = false;

	if(is_wearing())
	{
		if(IsInPPGScreen())
		{
			
			notify_infor infor = {0};
			uint16_t str_note[LANGUAGE_MAX][80] = {
													{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
													{0x0044,0x0069,0x0065,0x0020,0x005A,0x0065,0x0069,0x0074,0x006D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0073,0x0074,0x0065,0x0068,0x0074,0x0020,0x006B,0x0075,0x0072,0x007A,0x0020,0x0062,0x0065,0x0076,0x006F,0x0072,0x002C,0x0020,0x0075,0x006E,0x0064,0x0020,0x0064,0x0069,0x0065,0x0073,0x0065,0x0020,0x004D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0069,0x0073,0x0074,0x0020,0x0076,0x006F,0x0072,0x0062,0x0065,0x0069,0x0021,0x0000},//Die Zeitmessung steht kurz bevor, und diese Messung ist vorbei!
													{0x5B9A,0x65F6,0x6D4B,0x91CF,0x5373,0x5C06,0x5F00,0x59CB,0xFF0C,0x672C,0x6B21,0x6D4B,0x91CF,0x7ED3,0x675F,0xFF01,0x0000},//定时测量即将开始，本次测量结束！
												};

			PPGScreenStopTimer();
			if(PPGIsWorking())
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

			infor.img_count = 0;
			mmi_ucs2cpy(infor.text, (uint8_t*)str_note[global_settings.language]);

			DisplayPopUp(infor);

			g_ppg_data = PPG_DATA_BPT;
			k_timer_start(&ppg_delay_start_timer, K_MSEC((NOTIFY_TIMER_INTERVAL+1)*1000), K_NO_WAIT);
		}
		else
		{
			g_ppg_trigger |= TRIGGER_BY_HOURLY;
			g_ppg_alg_mode = ALG_MODE_BPT;
			g_ppg_data = PPG_DATA_BPT;
			ppg_start_flag = true;
		}
	}
}

void APPStartBpt(void)
{
	uint8_t tmpbuf[2] = {0};
	
	memset(&g_bpt, 0x00, sizeof(bpt_data));

	get_bpt_ok_flag = false;
	
	if(is_wearing())
	{
		g_ppg_trigger |= TRIGGER_BY_APP;
		g_ppg_alg_mode = ALG_MODE_BPT;
		g_ppg_data = PPG_DATA_BPT;
		ppg_start_flag = true;
	}
	else
	{
		MCU_send_app_get_ppg_data(PPG_DATA_BPT, tmpbuf);
	}
}

void MenuTriggerBpt(void)
{
	if(!is_wearing())
	{
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

		infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
		infor.img_count = 1;

		DisplayPopUp(infor);
		
		return;
	}
	
	if(PPGIsWorkingTiming())
	{
		notify_infor infor = {0};
		uint16_t str_note[LANGUAGE_MAX][64] = {
											{0x0054,0x0068,0x0065,0x0020,0x0073,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x0069,0x0073,0x0020,0x0072,0x0075,0x006E,0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
											{0x0044,0x0065,0x0072,0x0020,0x0053,0x0065,0x006E,0x0073,0x006F,0x0072,0x0020,0x006C,0x00E4,0x0075,0x0066,0x0074,0x002C,0x0020,0x0062,0x0069,0x0074,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0073,0x0070,0x00E4,0x0074,0x0065,0x0072,0x0020,0x006E,0x006F,0x0063,0x0068,0x0020,0x0065,0x0069,0x006E,0x006D,0x0061,0x006C,0x0021,0x0000},//Der Sensor l?uft, bitte versuchen Sie es sp?ter noch einmal!
											{0x4F20,0x611F,0x5668,0x6B63,0x5728,0x8FD0,0x884C,0xFF0C,0x8BF7,0x7A0D,0x540E,0x518D,0x8BD5,0xFF01,0x0000},//传感器正在运行，请稍后再试！
										};
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

		infor.img_count = 0;
		mmi_ucs2cpy(infor.text, (uint8_t*)str_note[global_settings.language]);

		DisplayPopUp(infor);
		return;
	}

	memset(&g_bpt, 0x00, sizeof(bpt_data));
	get_bpt_ok_flag = false;
	g_ppg_trigger |=TRIGGER_BY_MENU;
	g_ppg_alg_mode = ALG_MODE_BPT;
	g_ppg_data = PPG_DATA_BPT;
	ppg_start_flag = true;
}

void MenuStartBpt(void)
{
	menu_start_bpt = true;
}

void MenuStopBpt(void)
{
	ppg_stop_flag = true;
}

void TimerStartEcg(void)
{
	g_ppg_trigger |= TRIGGER_BY_HOURLY;
	g_ppg_alg_mode = ALG_MODE_ECG;
	g_ppg_data = PPG_DATA_ECG;
	ppg_start_flag = true;
}

void APPStartEcg(void)
{
	g_ppg_trigger |= TRIGGER_BY_APP;
	g_ppg_alg_mode = ALG_MODE_ECG;
	g_ppg_data = PPG_DATA_ECG;
	ppg_start_flag = true;
}

void MenuStartEcg(void)
{
	if(!is_wearing())
	{
		notify_infor infor = {0};
		
		infor.x = 0;
		infor.y = 0;
		infor.w = LCD_WIDTH;
		infor.h = LCD_HEIGHT;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		infor.img[0] = IMG_WRIST_OFF_ICON_ADDR;
		infor.img_count = 1;

		DisplayPopUp(infor);
		
		return;
	}
	
	if(PPGIsWorking())
	{
		notify_infor infor = {0};
		uint16_t str_note[LANGUAGE_MAX][48] = {
												{0x006E,0x0069,0x006E,0x0067,0x002C,0x0020,0x0070,0x006C,0x0065,0x0061,0x0073,0x0065,0x0020,0x0074,0x0072,0x0079,0x0020,0x0061,0x0067,0x0061,0x0069,0x006E,0x0020,0x006C,0x0061,0x0074,0x0065,0x0072,0x0021,0x0000},//The sensor is running, please try again later!
												{0x0020,0x0062,0x0069,0x0074,0x0074,0x0065,0x0020,0x0076,0x0065,0x0072,0x0073,0x0075,0x0063,0x0068,0x0065,0x006E,0x0020,0x0053,0x0069,0x0065,0x0020,0x0065,0x0073,0x0020,0x0073,0x0070,0x00E4,0x0074,0x0065,0x0072,0x0020,0x006E,0x006F,0x0063,0x0068,0x0020,0x0065,0x0069,0x006E,0x006D,0x0061,0x006C,0x0021,0x0000},//Der Sensor l?uft, bitte versuchen Sie es sp?ter noch einmal!
												{0x4F20,0x611F,0x5668,0x6B63,0x5728,0x8FD0,0x884C,0xFF0C,0x8BF7,0x7A0D,0x540E,0x518D,0x8BD5,0xFF01,0x0000},//传感器正在运行，请稍后再试！
											};
		
		infor.x = 0;
		infor.y = 0;
		infor.w = LCD_WIDTH;
		infor.h = LCD_HEIGHT;

		infor.align = NOTIFY_ALIGN_CENTER;
		infor.type = NOTIFY_TYPE_POPUP;

		infor.img_count = 0;
		mmi_ucs2cpy(infor.text, (uint8_t*)str_note[global_settings.language]);

		DisplayPopUp(infor);
		return;
	}

	g_ppg_trigger |= TRIGGER_BY_MENU;
	g_ppg_alg_mode = ALG_MODE_ECG;
	g_ppg_data = PPG_DATA_ECG;
	ppg_start_flag = true;
}

void MenuStopEcg(void)
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
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_HOURLY);
		switch(g_ppg_data)
		{
		case PPG_DATA_HR:
			SetCurDayHrRecData(g_hr);
			break;
		case PPG_DATA_SPO2:
			SetCurDaySpo2RecData(g_spo2);
			break;
		case PPG_DATA_BPT:
			SetCurDayBptRecData(g_bpt);
			break;
		}
	}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
	if((g_ppg_trigger&TRIGGER_BY_FT) != 0)
	{
		FTPPGStatusUpdate(0, 0);
		return;
	}
#endif

	last_health.timestamp.year = date_time.year;
	last_health.timestamp.month = date_time.month; 
	last_health.timestamp.day = date_time.day;
	last_health.timestamp.hour = date_time.hour;
	last_health.timestamp.minute = date_time.minute;
	last_health.timestamp.second = date_time.second;
	last_health.timestamp.week = date_time.week;
	if(g_ppg_alg_mode == ALG_MODE_HR_SPO2)
	{
		last_health.hr = g_hr;
		last_health.spo2 = g_spo2;
	}
	else if((g_ppg_alg_mode == ALG_MODE_BPT)&&(g_ppg_bpt_status == BPT_STATUS_GET_EST))
	{
		last_health.systolic = g_bpt.systolic;
		last_health.diastolic = g_bpt.diastolic;
	}
	save_cur_health_to_record(&last_health);
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

	get_cur_health_from_record(&last_health);
	if((last_health.timestamp.year == date_time.year)
		&&(last_health.timestamp.month == date_time.month)
		&&(last_health.timestamp.day == date_time.day)
		)
	{
		g_hr = last_health.hr;
		g_spo2 = last_health.spo2;
		g_bpt.systolic = last_health.systolic;
		g_bpt.diastolic = last_health.diastolic;
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
		FTTrigger();
		ft_start_hr = false;
	}
#endif

	if(menu_start_hr)
	{
		MenuTriggerHr();
		menu_start_hr = false;
	}
	
	if(menu_start_spo2)
	{
		MenuTriggerSpo2();
		menu_start_spo2 = false;
	}
	
	if(menu_start_bpt)
	{
		MenuTriggerBpt();
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
			TimerStartHr();
			break;

		case PPG_DATA_SPO2:
			TimerStartSpo2();
			break;

		case PPG_DATA_BPT:
			TimerStartBpt();
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

