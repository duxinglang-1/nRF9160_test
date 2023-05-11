/****************************************Copyright (c)************************************************
** File Name:			    sleep.c
** Descriptions:			sleep message process source file
** Created By:				xie biao
** Created Date:			2020-10-28
** Modified Date:      		2022-05-26 
** Version:			    	V1.2
******************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "lsm6dso.h"
#include "sleep.h"
#include "datetime.h"
#include "external_flash.h"
#include "max20353.h"
#include "logger.h"

uint16_t last_light_sleep = 0;
uint16_t last_deep_sleep = 0;
uint16_t light_sleep_time = 0;
uint16_t deep_sleep_time = 0;
uint16_t g_light_sleep = 0;
uint16_t g_deep_sleep = 0;

static int waggle_level[12] = {0};
static int hour_time = 0;
static int move = 0;
static int rtc_sec = 0;
static int gsensor = 0;
static int move_flag = 0;
static uint16_t watch_state = 0;
static int waggle_flag = 0;
static int sedentary_time_temp = 0;

bool reset_sleep_data = false;
bool update_sleep_parameter = false;
static struct k_timer sleep_timer;

void ClearAllSleepRecData(void)
{
	uint8_t tmpbuf[SLEEP_REC2_DATA_SIZE] = {0xff};

	last_light_sleep = 0;
	last_deep_sleep = 0;
	light_sleep_time = 0;
	deep_sleep_time = 0;
	g_light_sleep = 0;
	g_deep_sleep = 0;

	SpiFlash_Write(tmpbuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
}

void SetCurDaySleepRecData(sleep_data data)
{
	uint8_t i,tmpbuf[SLEEP_REC2_DATA_SIZE] = {0};
	sleep_rec2_data *p_sleep,tmp_sleep = {0};
	sys_date_timer_t temp_date = {0};
	
	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));

	tmp_sleep.year = temp_date.year;
	tmp_sleep.month = temp_date.month;
	tmp_sleep.day = temp_date.day;
	memcpy(&tmp_sleep.sleep[temp_date.hour], &data, sizeof(sleep_data));

	SpiFlash_Read(tmpbuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
	p_sleep = tmpbuf;
	if((p_sleep->year == 0xffff || p_sleep->year == 0x0000)
		||(p_sleep->month == 0xff || p_sleep->month == 0x00)
		||(p_sleep->day == 0xff || p_sleep->day == 0x00)
		||((p_sleep->year == temp_date.year)&&(p_sleep->month == temp_date.month)&&(p_sleep->day == temp_date.day))
		)
	{
		//直接覆盖写在第一条
		p_sleep->year = temp_date.year;
		p_sleep->month = temp_date.month;
		p_sleep->day = temp_date.day;
		memcpy(&p_sleep->sleep[temp_date.hour], &data, sizeof(sleep_data));
		SpiFlash_Write(tmpbuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
	}
	else if((temp_date.year < p_sleep->year)
			||((temp_date.year == p_sleep->year)&&(temp_date.month < p_sleep->month))
			||((temp_date.year == p_sleep->year)&&(temp_date.month == p_sleep->month)&&(temp_date.day < p_sleep->day))
			)
	{
		uint8_t databuf[SLEEP_REC2_DATA_SIZE] = {0};
		
		//插入新的第一条,旧的第一条到第六条往后挪，丢掉最后一个
		memcpy(&databuf[0*sizeof(sleep_rec2_data)], &tmp_sleep, sizeof(sleep_rec2_data));
		memcpy(&databuf[1*sizeof(sleep_rec2_data)], &tmpbuf[0*sizeof(sleep_rec2_data)], 6*sizeof(sleep_rec2_data));
		SpiFlash_Write(databuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
	}
	else
	{
		uint8_t databuf[SLEEP_REC2_DATA_SIZE] = {0};
		
		//寻找合适的插入位置
		for(i=0;i<7;i++)
		{
			p_sleep = tmpbuf+i*sizeof(sleep_rec2_data);
			if((p_sleep->year == 0xffff || p_sleep->year == 0x0000)
				||(p_sleep->month == 0xff || p_sleep->month == 0x00)
				||(p_sleep->day == 0xff || p_sleep->day == 0x00)
				||((p_sleep->year == temp_date.year)&&(p_sleep->month == temp_date.month)&&(p_sleep->day == temp_date.day))
				)
			{
				//直接覆盖写
				p_sleep->year = temp_date.year;
				p_sleep->month = temp_date.month;
				p_sleep->day = temp_date.day;
				memcpy(&p_sleep->sleep[temp_date.hour], &data, sizeof(sleep_data));
				SpiFlash_Write(tmpbuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
				return;
			}
			else if((temp_date.year > p_sleep->year)
				||((temp_date.year == p_sleep->year)&&(temp_date.month > p_sleep->month))
				||((temp_date.year == p_sleep->year)&&(temp_date.month == p_sleep->month)&&(temp_date.day > p_sleep->day))
				)
			{
				if(i < 6)
				{
					p_sleep++;
					if((temp_date.year < p_sleep->year)
						||((temp_date.year == p_sleep->year)&&(temp_date.month < p_sleep->month))
						||((temp_date.year == p_sleep->year)&&(temp_date.month == p_sleep->month)&&(temp_date.day < p_sleep->day))
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
			memcpy(&databuf[0*sizeof(sleep_rec2_data)], &tmpbuf[0*sizeof(sleep_rec2_data)], (i+1)*sizeof(sleep_rec2_data));
			memcpy(&databuf[(i+1)*sizeof(sleep_rec2_data)], &tmp_sleep, sizeof(sleep_rec2_data));
			memcpy(&databuf[(i+2)*sizeof(sleep_rec2_data)], &tmpbuf[(i+1)*sizeof(sleep_rec2_data)], (7-(i+2))*sizeof(sleep_rec2_data));
			SpiFlash_Write(databuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
		}
		else
		{
			//未找到位置，直接接在末尾，老数据整体往前移，丢掉最前一个
			memcpy(&databuf[0*sizeof(sleep_rec2_data)], &tmpbuf[1*sizeof(sleep_rec2_data)], 6*sizeof(sleep_rec2_data));
			memcpy(&databuf[6*sizeof(sleep_rec2_data)], &tmp_sleep, sizeof(sleep_rec2_data));
			SpiFlash_Write(databuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
		}
	}	
}

void GetCurDaySleepRecData(uint8_t *databuf)
{
	uint8_t i,tmpbuf[SLEEP_REC2_DATA_SIZE] = {0};
	sleep_rec2_data sleep_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&sleep_rec2, &tmpbuf[i*sizeof(sleep_rec2_data)], sizeof(sleep_rec2_data));
		if((sleep_rec2.year == 0xffff || sleep_rec2.year == 0x0000)||(sleep_rec2.month == 0xff || sleep_rec2.month == 0x00)||(sleep_rec2.day == 0xff || sleep_rec2.day == 0x00))
			continue;
		
		if((sleep_rec2.year == date_time.year)&&(sleep_rec2.month == date_time.month)&&(sleep_rec2.day == date_time.day))
		{
			memcpy(databuf, sleep_rec2.sleep, sizeof(sleep_rec2.sleep));
			break;
		}
	}
}

void GetGivenTimeSleepRecData(sys_date_timer_t date, sleep_data *sleep)
{
	uint8_t i,tmpbuf[SLEEP_REC2_DATA_SIZE] = {0};
	sleep_rec2_data sleep_rec2 = {0};

	if(!CheckSystemDateTimeIsValid(date))
		return;
	if(sleep == NULL)
		return;

	SpiFlash_Read(tmpbuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&sleep_rec2, &tmpbuf[i*sizeof(sleep_rec2_data)], sizeof(sleep_rec2_data));
		if((sleep_rec2.year == 0xffff || sleep_rec2.year == 0x0000)||(sleep_rec2.month == 0xff || sleep_rec2.month == 0x00)||(sleep_rec2.day == 0xff || sleep_rec2.day == 0x00))
			continue;
		
		if((sleep_rec2.year == date.year)&&(sleep_rec2.month == date.month)&&(sleep_rec2.day == date.day))
		{
			memcpy(sleep, &sleep_rec2.sleep[date.hour], sizeof(sleep_data));
			break;
		}
	}
}

void set_sleep_parameter(int light_sleep, int deep_sleep,int *waggle) /* 回传睡眠参数*/
{
	/* 每次需从系统nvram 读出存储数据，防止系统异常重启时睡眠数据丢失*/
	deep_sleep_time = deep_sleep;  
	light_sleep_time = light_sleep;
	memset(waggle_level, 0, sizeof(waggle_level));
	strncpy(waggle_level, waggle,10);
}

void Set_Gsensor_data(signed short x, signed short y, signed short z, int step, int hr, int hour, int minute, int charging)
{	
	bool save_flag = false;
	int test=0,i=0;

	if(charging || !is_wearing())
	{
		//充电中或者没有佩戴的时候不执行
		return;
	}

	test = abs(x+y+z);
	hour_time = hour;

	if((abs(gsensor-test)) >= 120)  /* 判断手表是否晃动，阈值根据gsensor 灵敏度调整*/
	{
		watch_state = 0;
	}
	else
	{
		watch_state++;
	}

	if(((abs(gsensor-test)) >= 200)||(hr != 0))
	{
		if(hour < 6)
			waggle_level[hour]++;  /* 统计手表0点到6点佩戴状态，阈值根据gsensor 灵敏度调整，0为未佩戴，非0值为佩戴*/
	}

	gsensor = test;

	if((rtc_sec % 60) == 0)  /* 一分钟演算一次数据*/
	{
		if(((move <step)&&(move>0))||(move_flag > 0))  /*  move_flag  移动过后第一次静止默认为移动*/
		{
			if((move_flag == 0)||((move <step)&&(move>0)))
				move_flag = 1;
			else if(move_flag > 0)
				move_flag--;

			if(hour<SLEEP_TIME_END)
				sedentary_time_temp++;
		}	
		                    
		else if((watch_state <= 60)&&(move_flag == 0))
		{
			if(hour<SLEEP_TIME_END)
				sedentary_time_temp++;   /*  睡眠监测时间段，累加走动时间*/
			else
				sedentary_time_temp = 0;
		}

		if((hour>=SLEEP_TIME_START)||(hour<SLEEP_TIME_END))  /*输入时间是 24小时制 ，监测时间段晚上8点到早上8点 */
		{
			static bool start_flag = false;
			
			if((hour==SLEEP_TIME_START)&&(minute==0))//正式启动当天睡眠监测，清空上一天的数据
			{
				if(start_flag)
				{
					start_flag = false;
					
					move = 0;	 
					gsensor = 0;
					rtc_sec = 0;
					move_flag = 0;
					watch_state = 0;
					waggle_flag = 0;
					waggle_flag = 0;
					sedentary_time_temp = 0;			
					memset(waggle_level,0,sizeof(waggle_level)); /* 晃动等级 */
					
					last_light_sleep = 0;
					last_deep_sleep = 0;
					light_sleep_time = 0;
					deep_sleep_time = 0;
					g_light_sleep = 0;
					g_deep_sleep = 0;

					save_flag = true;
				}
			}
			else
			{
				if(!start_flag)
					start_flag = true;
				
				if((hour>=6)&&(hour<=SLEEP_TIME_END)&&(waggle_flag==0))
				{
					for(i=0;i<6;i++)	
					{
						if(waggle_level[i]==0)
							waggle_flag++; /* 计算晚上0点到6点几个小时是佩戴状态 */
					}
				}

				if((move == step)&&(watch_state >= 60)) /* 在期间没有走动过 */
				{
					if(is_wearing())	//xb add 2021-03-02 脱腕之后不计算睡眠
					{
						if(watch_state >= (60*10))/* 晚上15分钟以上未晃动过，判断为深度睡眠*/
							deep_sleep_time++; 
						else
							light_sleep_time++;
					}

					g_light_sleep = light_sleep_time + last_light_sleep;
					g_deep_sleep = deep_sleep_time + last_deep_sleep;

					save_flag = true;
				}
			}

			if(save_flag)
			{
				last_sport.timestamp.year = date_time.year;
				last_sport.timestamp.month = date_time.month; 
				last_sport.timestamp.day = date_time.day;
				last_sport.timestamp.hour = date_time.hour;
				last_sport.timestamp.minute = date_time.minute;
				last_sport.timestamp.second = date_time.second;
				last_sport.timestamp.week = date_time.week;
				last_sport.deep_sleep = g_deep_sleep;
				last_sport.light_sleep = g_light_sleep;
				save_cur_sport_to_record(&last_sport);	
			}
		}	
	}

	rtc_sec++;
	move = step;
}

int get_light_sleep_time(void) /* 浅睡时长*/
{
	return light_sleep_time*1.0;
}

int get_deep_sleep_time(void) /* 深睡时长*/
{
	return deep_sleep_time*1.0;
}

static void sleep_timer_handler(struct k_timer *timer)
{
	update_sleep_parameter = true;
}

void SleepDataReset(void)
{
	move = 0;	 
	gsensor = 0;
	rtc_sec = 0;
	move_flag = 0;
	watch_state = 0;
	waggle_flag = 0;
	waggle_flag = 0;
	sedentary_time_temp = 0;			
	memset(waggle_level,0,sizeof(waggle_level)); /* 晃动等级 */
	
	last_light_sleep = 0;
	last_deep_sleep = 0;
	light_sleep_time = 0;
	deep_sleep_time = 0;
	g_light_sleep = 0;
	g_deep_sleep = 0;

	last_sport.timestamp.year = date_time.year;
	last_sport.timestamp.month = date_time.month; 
	last_sport.timestamp.day = date_time.day;
	last_sport.timestamp.hour = date_time.hour;
	last_sport.timestamp.minute = date_time.minute;
	last_sport.timestamp.second = date_time.second;
	last_sport.timestamp.week = date_time.week;
	last_sport.deep_sleep = g_deep_sleep;
	last_sport.light_sleep = g_light_sleep;
	save_cur_sport_to_record(&last_sport);		
}

void SleepDataInit(bool reset_flag)
{
	bool flag = false;
		
	if((last_sport.timestamp.year == date_time.year)
		&&(last_sport.timestamp.month == date_time.month)
		&&(last_sport.timestamp.day == date_time.day)
		)
	{
		//The last record and current time are on the same day
		if(((date_time.hour >= SLEEP_TIME_START)&&(last_sport.timestamp.hour >= SLEEP_TIME_START))
			||((date_time.hour < SLEEP_TIME_END)&&(last_sport.timestamp.hour < SLEEP_TIME_END))
			)
		{
			//in the same sleep period.
			flag = true;
		}
		else if(((date_time.hour >= SLEEP_TIME_END)&&(date_time.hour < SLEEP_TIME_START))
				||((last_sport.timestamp.hour >= SLEEP_TIME_END)&&(last_sport.timestamp.hour < SLEEP_TIME_START))
				)
		{
			//in the same non-sleep time period.
			flag = true;
		}
	}
	else
	{
		sys_date_timer_t timestamp;

		memcpy(&timestamp, &last_sport.timestamp, sizeof(sys_date_timer_t));
		DateIncreaseOne(&timestamp);

		if((timestamp.year == date_time.year)
			&&(timestamp.month == date_time.month)
			&&(timestamp.day == date_time.day)
			)
		{
			//The last recorded time was yesterday.
			if((timestamp.hour >= SLEEP_TIME_START)&&(date_time.hour < SLEEP_TIME_END))
			{
				//The last recorded time and current time are consecutive sleep periods.
				flag = true;
			}
		}
	}

	if(reset_flag)
	{
		if(!flag)
		{
			g_light_sleep -= last_light_sleep;
			g_deep_sleep -= last_deep_sleep;
			last_light_sleep = 0;
			last_deep_sleep = 0;
		}
	}
	else
	{
		if(flag)
		{
			last_light_sleep = last_sport.light_sleep;
			last_deep_sleep = last_sport.deep_sleep;
			g_light_sleep = last_light_sleep;
			g_deep_sleep = last_deep_sleep; 	
		}
	}
}

void StartSleepTimeMonitor(void)
{
	SleepDataInit(false);

	k_timer_init(&sleep_timer, sleep_timer_handler, NULL);
	k_timer_start(&sleep_timer, K_MSEC(1000), K_MSEC(1000));
}

void GetSleepTimeData(uint16_t *deep_sleep, uint16_t *light_sleep)
{
	*deep_sleep = g_deep_sleep;
	*light_sleep = g_light_sleep;
}

void GetSleepInfor(void)
{
	uint16_t deep_sleep, light_sleep;

	GetSleepTimeData(&deep_sleep, &light_sleep);

	//LOGD("deep_sleep:%d, light_sleep:%d", deep_sleep, light_sleep);
}

void UpdateSleepPara(void)
{
	uint16_t steps;
	float sensor_x,sensor_y,sensor_z;
	int chg = 0;

	if(g_chg_status != BAT_CHARGING_NO)
		chg = 1;
	
	get_sensor_reading(&sensor_x, &sensor_y, &sensor_z);
	GetImuSteps(&steps);
	Set_Gsensor_data((signed short)sensor_x, (signed short)sensor_x, (signed short)sensor_x, steps, 80, date_time.hour, date_time.minute, chg);
}
