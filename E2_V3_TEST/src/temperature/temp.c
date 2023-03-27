/****************************************Copyright (c)************************************************
** File Name:			    temp.c
** Descriptions:			temperature message process source file
** Created By:				xie biao
** Created Date:			2021-12-24
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include "external_flash.h"
#include "datetime.h"
#include "lcd.h"
#include "screen.h"
#include "uart_ble.h"
#include "temp.h"
#include "inner_flash.h"
#include "logger.h"
#if defined(TEMP_GXTS04)
#include "gxts04.h"
#elif defined(TEMP_MAX30208)
#include "max30208.h"
#elif defined(TEMP_CT1711)
#include "ct1711.h"
#endif

static bool temp_check_ok = false;
static bool temp_get_data_flag = false;
static bool temp_start_flag = false;
static bool temp_test_flag = false;
static bool temp_stop_flag = false;
static bool temp_redraw_data_flag = false;
static bool temp_power_flag = false;
static bool menu_start_temp = false;
static bool ft_start_temp = false;

bool get_temp_ok_flag = false;

TEMP_WORK_STATUS g_temp_status = TEMP_STATUS_PREPARE;

uint8_t g_temp_trigger = 0;
float g_temp_skin = 0.0;
float g_temp_body = 0.0;
float g_temp_menu = 0.0;

static void temp_auto_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_stop_timer, temp_auto_stop_timerout, NULL);
static void temp_menu_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_menu_stop_timer, temp_menu_stop_timerout, NULL);
static void temp_get_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(temp_check_timer, temp_get_timerout, NULL);

static void temp_auto_stop_timerout(struct k_timer *timer_id)
{
	if((g_temp_trigger&TEMP_TRIGGER_BY_MENU) == 0)
		temp_stop_flag = true;
}

static void temp_menu_stop_timerout(struct k_timer *timer_id)
{
	if(screen_id == SCREEN_ID_TEMP)
	{
		g_temp_status = TEMP_STATUS_MEASURE_FAIL;
		
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TEMP;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

static void temp_get_timerout(struct k_timer *timer_id)
{
	temp_get_data_flag = true;
}

void ClearAllTempRecData(void)
{
	uint8_t tmpbuf[TEMP_REC2_DATA_SIZE] = {0xff};

	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	g_temp_menu = 0.0;

	SpiFlash_Write(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
}

void SetCurDayTempRecData(float data)
{
	uint8_t i,tmpbuf[TEMP_REC2_DATA_SIZE] = {0};
	uint16_t deca_temp = data*10;
	temp_rec2_data *p_temp,tmp_temp = {0};
	sys_date_timer_t temp_date = {0};

	if((deca_temp > TEMP_MAX) || (deca_temp < TEMP_MIN))
		deca_temp = 0;

	//It is saved before the hour, but recorded as the hour data, so hour needs to be increased by 1
	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	TimeIncrease(&temp_date, 60);
	
	tmp_temp.year = temp_date.year;
	tmp_temp.month = temp_date.month;
	tmp_temp.day = temp_date.day;
	tmp_temp.deca_temp[temp_date.hour] = deca_temp;
	
	
	SpiFlash_Read(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	p_temp = tmpbuf;
	if((p_temp->year == 0xffff || p_temp->year == 0x0000)
		||(p_temp->month == 0xff || p_temp->month == 0x00)
		||(p_temp->day == 0xff || p_temp->day == 0x00)
		||((p_temp->year == temp_date.year)&&(p_temp->month == temp_date.month)&&(p_temp->day == temp_date.day))
		)
	{
		//直接覆盖写在第一条
		p_temp->year = temp_date.year;
		p_temp->month = temp_date.month;
		p_temp->day = temp_date.day;
		p_temp->deca_temp[temp_date.hour] = deca_temp;
		SpiFlash_Write(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	}
	else if((temp_date.year < p_temp->year)
			||((temp_date.year == p_temp->year)&&(temp_date.month < p_temp->month))
			||((temp_date.year == p_temp->year)&&(temp_date.month == p_temp->month)&&(temp_date.day < p_temp->day))
			)
	{
		uint8_t databuf[TEMP_REC2_DATA_SIZE] = {0};
		
		//插入新的第一条,旧的第一条到第六条往后挪，丢掉最后一个
		memcpy(&databuf[0*sizeof(temp_rec2_data)], &tmp_temp, sizeof(temp_rec2_data));
		memcpy(&databuf[1*sizeof(temp_rec2_data)], &tmpbuf[0*sizeof(temp_rec2_data)], 6*sizeof(temp_rec2_data));
		SpiFlash_Write(databuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	}
	else
	{
		uint8_t databuf[TEMP_REC2_DATA_SIZE] = {0};
		
		//寻找合适的插入位置
		for(i=0;i<7;i++)
		{
			p_temp = tmpbuf+i*sizeof(temp_rec2_data);
			if((p_temp->year == 0xffff || p_temp->year == 0x0000)
				||(p_temp->month == 0xff || p_temp->month == 0x00)
				||(p_temp->day == 0xff || p_temp->day == 0x00)
				||((p_temp->year == temp_date.year)&&(p_temp->month == temp_date.month)&&(p_temp->day == temp_date.day))
				)
			{
				//直接覆盖写
				p_temp->year = temp_date.year;
				p_temp->month = temp_date.month;
				p_temp->day = temp_date.day;
				p_temp->deca_temp[temp_date.hour] = deca_temp;
				SpiFlash_Write(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
				return;
			}
			else if((temp_date.year > p_temp->year)
				||((temp_date.year == p_temp->year)&&(temp_date.month > p_temp->month))
				||((temp_date.year == p_temp->year)&&(temp_date.month == p_temp->month)&&(temp_date.day > p_temp->day))
				)
			{
				if(i < 6)
				{
					p_temp++;
					if((temp_date.year < p_temp->year)
						||((temp_date.year == p_temp->year)&&(temp_date.month < p_temp->month))
						||((temp_date.year == p_temp->year)&&(temp_date.month == p_temp->month)&&(temp_date.day < p_temp->day))
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
			memcpy(&databuf[0*sizeof(temp_rec2_data)], &tmpbuf[0*sizeof(temp_rec2_data)], (i+1)*sizeof(temp_rec2_data));
			memcpy(&databuf[(i+1)*sizeof(temp_rec2_data)], &tmp_temp, sizeof(temp_rec2_data));
			memcpy(&databuf[(i+2)*sizeof(temp_rec2_data)], &tmpbuf[(i+1)*sizeof(temp_rec2_data)], (7-(i+2))*sizeof(temp_rec2_data));
			SpiFlash_Write(databuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
		}
		else
		{
			//未找到位置，直接接在末尾，老数据整体往前移，丢掉最前一个
			memcpy(&databuf[0*sizeof(temp_rec2_data)], &tmpbuf[1*sizeof(temp_rec2_data)], 6*sizeof(temp_rec2_data));
			memcpy(&databuf[6*sizeof(temp_rec2_data)], &tmp_temp, sizeof(temp_rec2_data));
			SpiFlash_Write(databuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
		}
	}
}

void GetCurDayTempRecData(uint16_t *databuf)
{
	uint8_t i,tmpbuf[TEMP_REC2_DATA_SIZE] = {0};
	temp_rec2_data temp_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&temp_rec2, &tmpbuf[i*sizeof(temp_rec2_data)], sizeof(temp_rec2_data));
		if((temp_rec2.year == 0xffff || temp_rec2.year == 0x0000)||(temp_rec2.month == 0xff || temp_rec2.month == 0x00)||(temp_rec2.day == 0xff || temp_rec2.day == 0x00))
			continue;
		
		if((temp_rec2.year == date_time.year)&&(temp_rec2.month == date_time.month)&&(temp_rec2.day == date_time.day))
		{
			memcpy(databuf, temp_rec2.deca_temp, sizeof(temp_rec2.deca_temp));
			break;
		}
	}
}

void GetGivenTimeTempRecData(sys_date_timer_t date, uint16_t *temp)
{
	uint8_t i,tmpbuf[TEMP_REC2_DATA_SIZE] = {0};
	temp_rec2_data temp_rec2 = {0};

	if(!CheckSystemDateTimeIsValid(date))
		return;	
	if(temp == NULL)
		return;

	SpiFlash_Read(tmpbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&temp_rec2, &tmpbuf[i*sizeof(temp_rec2_data)], sizeof(temp_rec2_data));
		if((temp_rec2.year == 0xffff || temp_rec2.year == 0x0000)||(temp_rec2.month == 0xff || temp_rec2.month == 0x00)||(temp_rec2.day == 0xff || temp_rec2.day == 0x00))
			continue;
		
		if((temp_rec2.year == date.year)&&(temp_rec2.month == date.month)&&(temp_rec2.day == date.day))
		{
			*temp = temp_rec2.deca_temp[date.hour];
			break;
		}
	}
}

bool IsInTempScreen(void)
{
	if(screen_id == SCREEN_ID_TEMP)
		return true;
	else
		return false;
}

bool TempIsWorkingTiming(void)
{
	if((g_temp_trigger&TEMP_TRIGGER_BY_HOURLY) != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool TempIsWorking(void)
{
	if(temp_power_flag == false)
		return false;
	else
		return true;
}

void TempStop(void)
{
	temp_stop_flag = true;
}

void TempRedrawData(void)
{
	if((screen_id == SCREEN_ID_IDLE)
		||(screen_id == SCREEN_ID_TEMP)
		)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TEMP;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void TimerStartTemp(void)
{
	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	g_temp_menu = 0.0;
	get_temp_ok_flag = false;

	if(is_wearing())
	{
		g_temp_trigger |= TEMP_TRIGGER_BY_HOURLY;
		temp_start_flag = true;
	}
}

void APPStartTemp(void)
{
	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	get_temp_ok_flag = false;

	if(is_wearing())
	{
		g_temp_trigger |= TEMP_TRIGGER_BY_APP;
		temp_start_flag = true;
	}
}

void MenuTriggerTemp(void)
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

	g_temp_skin = 0.0;
	g_temp_body = 0.0;
	get_temp_ok_flag = false;
	g_temp_trigger |= TEMP_TRIGGER_BY_MENU;
	temp_start_flag = true;
}

void MenuStartTemp(void)
{
	menu_start_temp = true;
}

void MenuStopTemp(void)
{
	temp_stop_flag = true;
}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void FTStartTemp(void)
{
	g_temp_trigger |= TEMP_TRIGGER_BY_FT;
	
	if(!TempIsWorking())
	{
		g_temp_skin = 0.0;
		g_temp_body = 0.0;
		get_temp_ok_flag = false;
		temp_start_flag = true;
	}
}

void FTStopTemp(void)
{
	temp_stop_flag = true;
}
#endif

void temp_init(void)
{
	get_cur_health_from_record(&last_health);
	if(last_health.timestamp.day == date_time.day)
	{
		g_temp_body = (float)(last_health.deca_temp/10.0);
	}

#ifdef TEMP_GXTS04
	temp_check_ok = gxts04_init();
#elif defined(TEMP_MAX30208)
	temp_check_ok = max30208_init();
#elif defined(TEMP_CT1711)
	temp_check_ok = ct1711_init();
#endif
}

void TempMsgProcess(void)
{
	if(temp_get_data_flag)
	{
		bool ret;
		float temp_1=0.0,temp_2=0.0;
		
		temp_get_data_flag = false;

		if(!temp_check_ok)
			return;

		ret = GetTemperature(&temp_1, &temp_2);
		if(temp_1 > 0.0)
		{
			g_temp_skin = temp_1;
			g_temp_body = temp_2;
			temp_redraw_data_flag = true;
		}

		if(ret)
		{
			temp_stop_flag = true;
			get_temp_ok_flag = true;
		}
	#ifdef CONFIG_FACTORY_TEST_SUPPORT
		FTTempStatusUpdate();
	#endif
	}

	if(menu_start_temp)
	{
		MenuTriggerTemp();
		menu_start_temp = false;
	}
	
	if(temp_start_flag)
	{
		temp_start_flag = false;
		if(temp_power_flag)
			return;
		
	#ifdef TEMP_GXTS04	
		gxts04_start();
	#endif
		temp_power_flag = true;
	
		k_timer_start(&temp_check_timer, K_MSEC(1*1000), K_MSEC(1*1000));

		if((g_temp_trigger&TEMP_TRIGGER_BY_HOURLY) == TEMP_TRIGGER_BY_HOURLY)
		{
			k_timer_start(&temp_stop_timer, K_MSEC(TEMP_CHECK_TIMELY*60*1000), K_NO_WAIT);
		}
	#ifndef UI_STYLE_HEALTH_BAR	
		else if((g_temp_trigger&TEMP_TRIGGER_BY_MENU) == TEMP_TRIGGER_BY_MENU)
		{
			k_timer_start(&temp_menu_stop_timer, K_SECONDS(TEMP_CHECK_MENU), K_NO_WAIT);
		}
	#endif
	}

	if(temp_stop_flag)
	{
		temp_stop_flag = false;
		if(!temp_power_flag)
			return;
		
	#ifdef TEMP_GXTS04	
		gxts04_stop();
	#endif
	
		temp_power_flag = false;
		k_timer_stop(&temp_check_timer);
		k_timer_stop(&temp_stop_timer);
		k_timer_stop(&temp_menu_stop_timer);

	#ifdef CONFIG_BLE_SUPPORT
		if((g_temp_trigger&TEMP_TRIGGER_BY_APP) != 0)
		{
			uint8_t data[2] = {0};
			uint16_t deca_temp = 0;
			
			g_temp_trigger = g_temp_trigger&(~TEMP_TRIGGER_BY_APP);

			deca_temp = g_temp_body*10;
			data[0] = deca_temp>>8;
			data[1] = (uint8_t)(deca_temp&0x00ff);
			MCU_send_app_get_temp_data(data);
		}
	#endif	
		if((g_temp_trigger&TEMP_TRIGGER_BY_MENU) != 0)
		{
			g_temp_trigger = g_temp_trigger&(~TEMP_TRIGGER_BY_MENU);
			g_temp_menu = g_temp_body;

		#ifdef CONFIG_BLE_SUPPORT
			if(g_ble_connected)
			{
				uint8_t data[2] = {0};
				uint16_t deca_temp = 0;

				deca_temp = g_temp_body*10;
				data[0] = deca_temp>>8;
				data[1] = (uint8_t)(deca_temp&0x00ff);
				MCU_send_app_get_temp_data(data);
			}
		#endif
		
			SyncSendHealthData();
			g_temp_menu = 0;
		}
		if((g_temp_trigger&TEMP_TRIGGER_BY_HOURLY) != 0)
		{
			g_temp_trigger = g_temp_trigger&(~TEMP_TRIGGER_BY_HOURLY);
			SetCurDayTempRecData(g_temp_body);
		}
	#ifdef CONFIG_FACTORY_TEST_SUPPORT	
		if((g_temp_trigger&TEMP_TRIGGER_BY_FT) != 0)
		{
			g_temp_trigger = g_temp_trigger&(~TEMP_TRIGGER_BY_FT);
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
		last_health.deca_temp = (uint16_t)(g_temp_body*10);
		save_cur_health_to_record(&last_health);
	}
	
	if(temp_redraw_data_flag)
	{
		TempRedrawData();
		temp_redraw_data_flag = false;
	}
}

