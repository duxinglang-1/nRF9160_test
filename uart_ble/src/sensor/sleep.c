#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

uint16_t light_sleep_time = 0;
uint16_t deep_sleep_time = 0;
int waggle_level[12] = {0};
int hour_time = 0;

extern bool clear_data;

void set_sleep_parameter(int light_sleep, int deep_sleep,int *waggle) /* 回传睡眠参数*/
{
	/* 每次需从系统nvram 读出存储数据，防止系统异常重启时睡眠数据丢失*/
	deep_sleep_time = deep_sleep;  
	light_sleep_time = light_sleep;
	memset(waggle_level, 0, sizeof(waggle_level));
	strncpy(waggle_level, waggle,10);
}

void Set_Gsensor_data(signed short x, signed short y, signed short z, int setp,int hr,int hour,int charging)
{	
  int test = 0 , i = 0;
  static int move = 0;
  static int rtc_sec = 0;
  static int gsensor = 0;
  static int move_flag = 0;
  static uint16_t watch_state = 0;
  static int waggle_flag = 0;
  static int sedentary_time_temp = 0;

    if(charging){ /* 充电中不执行*/
	return;
    }
		
    test = abs(x+y+z);
    hour_time = hour;

    if((abs(gsensor-test)) >= 100)  /* 判断手表是否晃动，阈值根据gsensor 灵敏度调整*/
    {
      watch_state = 0;
    }
    else
    {
      watch_state ++;
    }
	
    if(((abs(gsensor-test)) >= 200)||(hr != 0))
    {
      if(hour < 6)
      waggle_level[hour] ++;  /* 统计手表0点到6点佩戴状态，阈值根据gsensor 灵敏度调整，0为未佩戴，非0值为佩戴*/
    }
	
    gsensor = test;
	
    if((rtc_sec % 60) == 0)  /* 一分钟演算一次数据*/
    {
		if(((move <setp)&&(move>0))||(move_flag > 0))  /*  move_flag  移动过后第一次静止默认为移动*/
		{
			if((move_flag == 0)||((move <setp)&&(move>0)))
			move_flag = 1;
			else if(move_flag > 0)
			move_flag --;
			if(hour<8)
			sedentary_time_temp ++;
		}	
                                
		else if((watch_state <= 60)&&(move_flag == 0))
		{
			if(hour<8)
			sedentary_time_temp ++;   /*  睡眠监测时间段，累加走动时间*/
			else
			sedentary_time_temp = 0;
		}
		
		if((hour>=20)||(hour<8))  /*输入时间是 24小时制 ，监测时间段晚上8点到早上8点 */
		{
			if((hour >= 6)&&(hour <= 8)&&(waggle_flag == 0))
			{
				for(i = 0; i < 6; i++)	
				{
				   if(waggle_level[i]==0)
				   waggle_flag ++; /* 计算晚上0点到6点几个小时是佩戴状态 */
				}
			}
			
			if((watch_state >= (3600*2))||(waggle_flag >= 3)||/* (3)晚上2个3时终端没有动，判断为晚上睡觉没有佩戴过，累计睡眠、久坐数据清空*/
			((hour<8)&&(sedentary_time_temp > 180)))  /* 晚上睡眠监测时间累加150分钟又走动或者久坐，默认睡眠监测数据为0*/
			{
				sedentary_time_temp = 0;
				light_sleep_time = 0;				 
				deep_sleep_time = 0;
                                clear_data = true;
			}
			else if((move == setp)&&(watch_state >= 60)) /* 在期间没有走动过 */
			{
				if(watch_state >= (60*10))/* 晚上15分钟以上未晃动过，判断为深度睡眠*/
				deep_sleep_time ++; 
				else
				light_sleep_time ++; 
			}
			
		}
		else
		{
			move = 0;	 
			gsensor = 0;
			rtc_sec = 0;
			move_flag = 0;
			watch_state = 0;
			waggle_flag = 0;
			waggle_flag = 0;
			deep_sleep_time = 0; 
			light_sleep_time = 0;
			sedentary_time_temp = 0;			memset(waggle_level,0,sizeof(waggle_level)); /* 晃动等级 */
		}
    }

    rtc_sec ++;
    move = setp;
}

int get_light_sleep_time(void) /* 浅睡时长*/
{
  return light_sleep_time*1.1;
}

int get_deep_sleep_time(void) /* 深睡时长*/
{
  return deep_sleep_time*1.1;
}

