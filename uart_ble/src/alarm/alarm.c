/****************************************Copyright (c)************************************************
** File name:			    alarm.c
** Last modified Date:          
** Last Version:		   
** Descriptions:		   	使用的ncs版本-1.2		
** Created by:				谢彪
** Created date:			2020-11-03
** Version:			    	1.0
** Descriptions:			系统闹钟管理C文件
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include "settings.h"
#include "datetime.h"
#include "lcd.h"

#define ALARM_VIB_REPEAT_MAX	5
#define ALARM_VIB_ON_SEC		500
#define ALARM_VIB_OFF_SEC		200

#define FIND_VIN_REPEAT_MAX		10
#define FIND_VIB_ON_SEC			1000
#define FIND_VIB_OFF_SEC		200

static u8_t count = 0;
static bool vibrating = false;

static struct k_timer alarm_timer;
static struct k_timer find_timer;

bool vibrate_start_flag = false;
bool vibrate_stop_flag = false;
bool alarm_is_running = false;
bool find_is_running = false;

extern bool lcd_sleep_out;
extern bool show_date_time_first;

extern void VibrateStart(void);
extern void VibrateStop(void);

void AlarmRemindStop(void)
{
	vibrate_stop_flag = true;

	alarm_is_running = false;
	vibrating = false;
	count = 0;

	k_timer_stop(&alarm_timer);

	screen_id = SCREEN_IDLE;
	show_date_time_first = true;
}

void AlarmRemindTimeout(struct k_timer *timer)
{
	if(vibrating)
	{
		vibrate_stop_flag = true;
		vibrating = false;

		count--;
		if(count>0)
		{
			k_timer_start(&alarm_timer, K_MSEC(ALARM_VIB_OFF_SEC), NULL);
		}
		else
		{
			AlarmRemindStop();
		}
	}
	else
	{
		vibrate_start_flag = true;
		vibrating = true;

		k_timer_start(&alarm_timer, K_MSEC(ALARM_VIB_ON_SEC), NULL);
	}
}

void AlarmRemindStart(void)
{
	lcd_sleep_out = true;
	alarm_is_running = true;
	
	count = ALARM_VIB_REPEAT_MAX;
	vibrating = true;

	vibrate_start_flag = true;

	k_timer_start(&alarm_timer, K_MSEC(ALARM_VIB_ON_SEC), NULL);
}

void AlarmRemindCheck(sys_date_timer_t time)
{
	u8_t i=0;
	bool flag=false;

	for(i=0;i<ALARM_MAX;i++)
	{
		if((global_settings.alarm[i].is_on)
			&&(global_settings.alarm[i].hour == time.hour)
			&&(global_settings.alarm[i].minute == time.minute))
		{
			switch(time.week)
			{
			case 0://Sunday
				if(global_settings.alarm[i].repeat&0x01 != 0)
				{
					flag = true;
				}
				break;
			case 1://Monday
				if(global_settings.alarm[i].repeat&0x40 != 0)
				{
					flag = true;
				}
				break;
			case 2://Tuesday
				if(global_settings.alarm[i].repeat&0x20 != 0)
				{
					flag = true;
				}
				break;
			case 3://Wednesday
				if(global_settings.alarm[i].repeat&0x10 != 0)
				{
					flag = true;
				}
				break;
			case 4://Thursday
				if(global_settings.alarm[i].repeat&0x08 != 0)
				{
					flag = true;
				}
				break;
			case 5://Friday
				if(global_settings.alarm[i].repeat&0x04 != 0)
				{
					flag = true;
				}
				break;
			case 6://Saturday
				if(global_settings.alarm[i].repeat&0x02 != 0)
				{
					flag = true;
				}
				break;
			default:
				break;
			}
			
			if(global_settings.alarm[i].repeat == 0)
			{
				flag = true;
				global_settings.alarm[i].is_on = false;
				need_save_settings = true;
			}

			if(flag)
			{
				AlarmRemindEntryScreen();
				break;
			}
		}
	}
}

void AlarmRemindEntryScreen(void)
{
	u16_t x,y,w,h;

	screen_id = SCREEN_ALARM;

	LCD_Clear(BLACK);
	LCD_MeasureString("The alarm is coming!",&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = (h > LCD_HEIGHT)? 0 : (LCD_HEIGHT-h)/2;
	
	LCD_ShowString(x,y,"The alarm is coming!");

	AlarmRemindStart();
}

void FindDeviceStop(void)
{
	vibrate_stop_flag = true;

	find_is_running = false;
	vibrating = false;
	count = 0;

	k_timer_stop(&find_timer);

	screen_id = SCREEN_IDLE;
	show_date_time_first = true;
}

void FindDeviceTimeout(struct k_timer *timer)
{
	if(vibrating)
	{
		vibrate_stop_flag = true;
		vibrating = false;

		count--;
		if(count>0)
		{
			k_timer_start(&find_timer, K_MSEC(FIND_VIB_OFF_SEC), NULL);
		}
		else
		{
			FindDeviceStop();
		}
	}
	else
	{
		vibrate_start_flag = true;
		vibrating = true;
		
		k_timer_start(&find_timer, K_MSEC(FIND_VIB_ON_SEC), NULL);
	}
}

void FindDeviceStart(void)
{
	lcd_sleep_out = true;
	find_is_running = true;
	
	count = FIND_VIN_REPEAT_MAX;
	vibrating = true;

	vibrate_start_flag = true;

	k_timer_start(&find_timer, K_MSEC(FIND_VIB_ON_SEC), NULL);
}

void FindDeviceEntryScreen(void)
{
	u16_t x,y,w,h;

	screen_id = SCREEN_FIND_DEVICE;

	LCD_Clear(BLACK);
	LCD_MeasureString("The find device is coming!",&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = (h > LCD_HEIGHT)? 0 : (LCD_HEIGHT-h)/2;
	
	LCD_ShowString(x,y,"The find device is coming!");

	AlarmRemindStart();
}

void AlarmRemindInit(void)
{
	k_timer_init(&alarm_timer, AlarmRemindTimeout, NULL);
	k_timer_init(&find_timer, FindDeviceTimeout, NULL);
}

