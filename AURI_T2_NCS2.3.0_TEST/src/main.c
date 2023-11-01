/*
* Copyright (c) 2019 Nordic Semiconductor ASA
*
* SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
*/

#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>
#include <power/reboot.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <modem/nrf_modem_lib.h>
#include <dk_buttons_and_leds.h>
#include "lcd.h"
#include "datetime.h"
#include "font.h"
#include "img.h"
#include "inner_flash.h"
#include "external_flash.h"
#include "uart_ble.h"
#include "settings.h"
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "Max20353.h"
#ifdef CONFIG_IMU_SUPPORT
#include "lsm6dso.h"
#endif
#ifdef CONFIG_ALARM_SUPPORT
#include "Alarm.h"
#endif
#include "gps.h"
#include "screen.h"
#include "codetrans.h"
#ifdef CONFIG_AUDIO_SUPPORT
#include "audio.h"
#endif
#ifdef CONFIG_WATCHDOG
#include "watchdog.h"
#endif
#include "logger.h"

//#define ANALOG_CLOCK
#define DIGITAL_CLOCK
#define PI 3.1415926

static bool sys_pwron_completed_flag = false;
static uint8_t show_pic_count = 0;//图片显示顺序

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(nb_stack_area,
		      4096);
static struct k_work_q nb_work_q;

#ifdef CONFIG_IMU_SUPPORT
K_THREAD_STACK_DEFINE(imu_stack_area,
              2048);
static struct k_work_q imu_work_q;
#endif

K_THREAD_STACK_DEFINE(gps_stack_area,
              2048);
static struct k_work_q gps_work_q;

#if defined(ANALOG_CLOCK)
static void test_show_analog_clock(void);
#elif defined(DIGITAL_CLOCK)
static void test_show_digital_clock(void);
#endif
static void idle_show_time(void);

#ifdef ANALOG_CLOCK
void ClearAnalogHourPic(int hour)
{
	uint16_t offset_x=4,offset_y=4;
	uint16_t hour_x,hour_y,hour_w,hour_h;

	LCD_get_pic_size(clock_hour_1_31X31,&hour_w,&hour_h);
	hour_x = (LCD_WIDTH)/2;
	hour_y = (LCD_HEIGHT)/2;

	if((hour%12) == 3)
		LCD_Fill(hour_x-offset_x,hour_y+offset_y-hour_h,hour_w,hour_h,BLACK);
	else if((hour%12) == 6)
		LCD_Fill(hour_x-offset_x,hour_y-offset_y,hour_w,hour_h,BLACK);
	else if((hour%12) == 9)
		LCD_Fill(hour_x+offset_x-hour_w,hour_y-offset_y,hour_w,hour_h,BLACK);
	else if((hour%12) == 0)
		LCD_Fill(hour_x+offset_x-hour_w,hour_y+offset_y-hour_h,hour_w,hour_h,BLACK);
}

void DrawAnalogHourPic(int hour)
{
	uint16_t offset_x=4,offset_y=4;
	uint16_t hour_x,hour_y,hour_w,hour_h;
	unsigned int *hour_pic[3] = {clock_hour_1_31X31,clock_hour_2_31X31,clock_hour_3_31X31};

	LCD_get_pic_size(clock_hour_1_31X31,&hour_w,&hour_h);
	hour_x = (LCD_WIDTH)/2;
	hour_y = (LCD_HEIGHT)/2;

	if((hour%12)<3)
		LCD_dis_pic_rotate(hour_x-offset_x,hour_y+offset_y-hour_h,hour_pic[hour%3],0);
	else if(((hour%12)>=3) && ((hour%12)<6))
		LCD_dis_pic_rotate(hour_x-offset_x,hour_y-offset_y,hour_pic[hour%3],90);
	else if(((hour%12)>=6) && ((hour%12)<9))
		LCD_dis_pic_rotate(hour_x+offset_x-hour_w,hour_y-offset_y,hour_pic[hour%3],180);
	else if(((hour%12)>=9) && ((hour%12)<12))
		LCD_dis_pic_rotate(hour_x+offset_x-hour_w,hour_y+offset_y-hour_h,hour_pic[hour%3],270);
}

void ClearAnalogMinPic(int minute)
{
	uint16_t offset_x=4,offset_y=4;
	uint16_t min_x,min_y,min_w,min_h;
	
	LCD_get_pic_size(clock_min_1_31X31,&min_w,&min_h);
	min_x = (LCD_WIDTH)/2;
	min_y = (LCD_HEIGHT)/2;

	if(minute == 15)
		LCD_Fill(min_x-offset_x,min_y+offset_y-min_h,min_w,min_h,BLACK);
	else if(minute == 30)
		LCD_Fill(min_x-offset_x,min_y-offset_y,min_w,min_h,BLACK);
	else if(minute == 45)
		LCD_Fill(min_x+offset_x-min_w,min_y-offset_y,min_w,min_h,BLACK);
	else if(minute == 0)
		LCD_Fill(min_x+offset_x-min_w,min_y+offset_y-min_h,min_w,min_h,BLACK);
}

void DrawAnalogMinPic(int hour, int minute)
{
	uint16_t offset_x=4,offset_y=4;
	uint16_t min_x,min_y,min_w,min_h;
	unsigned int *min_pic[15] = {clock_min_1_31X31,clock_min_2_31X31,clock_min_3_31X31,clock_min_4_31X31,clock_min_5_31X31,\
								 clock_min_6_31X31,clock_min_7_31X31,clock_min_8_31X31,clock_min_9_31X31,clock_min_10_31X31,\
								 clock_min_11_31X31,clock_min_12_31X31,clock_min_13_31X31,clock_min_14_31X31,clock_min_15_31X31};
	
	LCD_get_pic_size(clock_min_1_31X31,&min_w,&min_h);
	min_x = (LCD_WIDTH)/2;
	min_y = (LCD_HEIGHT)/2;

	if(minute<15)
	{
		if((hour%12)<3)							//分针时针有重叠，透明显示
		{
			DrawAnalogHourPic(hour);
			LCD_dis_trans_pic_rotate(min_x-offset_x,min_y+offset_y-min_h,min_pic[minute%15],BLACK,0);
		}
		else if((hour%12)==3)		//临界点，分针不透明显示，但是不能遮盖时针
		{
			LCD_dis_pic_rotate(min_x-offset_x,min_y+offset_y-min_h,min_pic[minute%15],0);
			DrawAnalogHourPic(hour);
		}
		else
			LCD_dis_pic_rotate(min_x-offset_x,min_y+offset_y-min_h,min_pic[minute%15],0);
	}
	else if((minute>=15) && (minute<30))
	{
		if(((hour%12)>=3) && ((hour%12)<6))	//分针时针有重叠，透明显示
		{
			DrawAnalogHourPic(hour);
			LCD_dis_trans_pic_rotate(min_x-offset_x,min_y-offset_y,min_pic[minute%15],BLACK,90);
		}
		else if((hour%12)==6)		//临界点，分针不透明显示，但是不能遮盖时针
		{
			LCD_dis_pic_rotate(min_x-offset_x,min_y-offset_y,min_pic[minute%15],90);
			DrawAnalogHourPic(hour);
		}
		else
			LCD_dis_pic_rotate(min_x-offset_x,min_y-offset_y,min_pic[minute%15],90);
	}
	else if((minute>=30) && (minute<45))
	{
		if(((hour%12)>=6) && ((hour%12)<9))	//分针时针有重叠，透明显示
		{
			DrawAnalogHourPic(hour);
			LCD_dis_trans_pic_rotate(min_x+offset_x-min_w,min_y-offset_y,min_pic[minute%15],BLACK,180);
		}
		else if((hour%12)==9)		//临界点，分针不透明显示，但是不能遮盖时针
		{
			LCD_dis_pic_rotate(min_x+offset_x-min_w,min_y-offset_y,min_pic[minute%15],180);
			DrawAnalogHourPic(hour);
		}
		else
			LCD_dis_pic_rotate(min_x+offset_x-min_w,min_y-offset_y,min_pic[minute%15],180);
	}
	else if((minute>=45) && (minute<60))
	{
		if((hour%12)>=9)	//分针时针有重叠，透明显示
		{
			DrawAnalogHourPic(hour);
			LCD_dis_trans_pic_rotate(min_x+offset_x-min_w,min_y+offset_y-min_h,min_pic[minute%15],BLACK,270);
		}
		else if((hour%12)==0)		//临界点，分针不透明显示，但是不能遮盖时针
		{
			LCD_dis_pic_rotate(min_x+offset_x-min_w,min_y+offset_y-min_h,min_pic[minute%15],270);
			DrawAnalogHourPic(hour);
		}
		else
			LCD_dis_pic_rotate(min_x+offset_x-min_w,min_y+offset_y-min_h,min_pic[minute%15],270);
	}
}
#endif

/***************************************************************************
* 描  述 : idle_show_digit_clock 待机界面显示数字时钟
* 入  参 : 无 
* 返回值 : 无
**************************************************************************/
void idle_show_digital_clock(void)
{
	IdleShowSystemTime();
	
	if(show_date_time_first || ((date_time_changed&0x38) != 0))
	{
		IdleShowSystemDate();
		IdleShowSystemWeek();

		if(show_date_time_first)
			show_date_time_first = false;
		if((date_time_changed&0x38) != 0)
			date_time_changed = date_time_changed&0xC7;//清空日期变化标志位
	}
}

#ifdef ANALOG_CLOCK
/***************************************************************************
* 描  述 : idle_show_analog_clock 待机界面显示模拟时钟
* 入  参 : 无 
* 返回值 : 无
**************************************************************************/
void idle_show_analog_clock(void)
{
	uint8_t str_date[20] = {0};
	uint8_t str_week[20] = {0};
	uint8_t *week_cn[7] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
	uint8_t *week_en[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	uint16_t str_w,str_h;

	POINT_COLOR=WHITE;								//画笔颜色
	BACK_COLOR=BLACK;  								//背景色 
	
	LCD_SetFontSize(FONT_SIZE_16);

	sprintf((char*)str_date, "%02d/%02d", date_time.day,date_time.month);
	if(global_settings.language == LANGUAGE_CHN)
		strcpy(str_week, week_cn[date_time.week]);
	else
		strcpy(str_week, week_en[date_time.week]);
	
	if(show_date_time_first)
	{
		show_date_time_first = false;
		DrawAnalogHourPic(date_time.hour);
		DrawAnalogMinPic(date_time.hour, date_time.minute);

		LCD_MeasureString(str_week, &str_w, &str_h);
		LCD_ShowString((LCD_WIDTH-str_w)/2, 15, str_week);

		LCD_MeasureString(str_date, &str_w, &str_h);
		LCD_ShowString((LCD_WIDTH-str_w)/2, 33, str_date);
	}
	else if(date_time_changed != 0)
	{
		if((date_time_changed&0x04) != 0)
		{
			DrawAnalogHourPic(date_time.hour);
			date_time_changed = date_time_changed&0xFB;
		}	
		if((date_time_changed&0x02) != 0)//分钟有变化
		{
			DrawAnalogHourPic(date_time.hour);
			DrawAnalogMinPic(date_time.hour, date_time.minute);
			date_time_changed = date_time_changed&0xFD;
		}
		if((date_time_changed&0x38) != 0)
		{
			LCD_MeasureString(str_week, &str_w, &str_h);
			LCD_ShowString((LCD_WIDTH-str_w)/2, 15, str_week);

			LCD_MeasureString(str_date, &str_w, &str_h);
			LCD_ShowString((LCD_WIDTH-str_w)/2, 33, str_date);

			date_time_changed = date_time_changed&0xC7;//清空日期变化标志位
		}
	}
}
#endif

void idle_show_clock_background(void)
{
	LCD_Clear(BLACK);
	BACK_COLOR=BLACK;
	POINT_COLOR=WHITE;
	
	LCD_SetFontSize(FONT_SIZE_16);

#ifdef ANALOG_CLOCK	
	if(global_settings.idle_colck_mode == CLOCK_MODE_ANALOG)
	{
		LCD_ShowImg(0,0,clock_bg_80X160);
	}
#endif
}

/***************************************************************************
* 描  述 : idle_show_time 待机界面显示时间
* 入  参 : 无 
* 返回值 : 无
**************************************************************************/
void idle_show_time(void)
{	
	if(global_settings.idle_colck_mode == CLOCK_MODE_ANALOG)
	{
	#ifdef ANALOG_CLOCK
		if((date_time_changed&0x02) != 0)
		{
			ClearAnalogMinPic(date_time.minute);//擦除以前的分钟
		}
		if((date_time_changed&0x04) != 0)
		{
			ClearAnalogHourPic(date_time.hour);//擦除以前的时钟
		}

		idle_show_analog_clock();
	#endif
	}
	else
	{
	#ifdef DIGITAL_CLOCK
		if((date_time_changed&0x01) != 0)
		{
			date_time_changed = date_time_changed&0xFE;
			BlockWrite((LCD_WIDTH-8*8)/2+6*8,(LCD_HEIGHT-16)/2,2*8,16);//擦除以前的秒钟
		}
		if((date_time_changed&0x02) != 0)
		{
			date_time_changed = date_time_changed&0xFD;
			BlockWrite((LCD_WIDTH-8*8)/2+3*8,(LCD_HEIGHT-16)/2,2*8,16);//擦除以前的分钟
		}
		if((date_time_changed&0x04) != 0)
		{
			date_time_changed = date_time_changed&0xFB;
			BlockWrite((LCD_WIDTH-8*8)/2,(LCD_HEIGHT-16)/2,2*8,16);//擦除以前的时钟
		}
		
		if((date_time_changed&0x38) != 0)
		{			
			BlockWrite((LCD_WIDTH-10*8)/2,(LCD_HEIGHT-16)/2+30,10*8,16);
		}
		
		idle_show_digital_clock();
	#endif
	}
}

void test_show_analog_clock(void)
{
	uint32_t err_code;
	
	global_settings.idle_colck_mode = CLOCK_MODE_ANALOG;
	
	idle_show_clock_background();
	idle_show_time();
}

void test_show_digital_clock(void)
{
	uint32_t err_code;
	
	global_settings.idle_colck_mode == CLOCK_MODE_DIGITAL;
	
	idle_show_clock_background();
	idle_show_time();
}

void test_show_image(void)
{
	uint8_t i=LCD_WIDTH;
	uint16_t x,y,w=0,h=0;

	LOGD("test_show_image");
	
	LCD_Clear(BLACK);
	LCD_ShowImg(0, 0, logo_5);

	//LCD_ShowImg_From_Flash(0, 0, IMG_AURI_LOGO_ADDR);
	
	//LCD_get_pic_size(peppa_pig_160X160, &w, &h);
	//LCD_dis_pic_rotate(0,200,peppa_pig_160X160,270);
	//LCD_ShowImg(0, 0, peppa_pig_160X160);
	//LCD_get_pic_size_from_flash(IMG_RM_LOGO_240X240_ADDR, &w, &h);
	//LCD_dis_pic_from_flash(0, 0, IMG_RM_LOGO_240X240_ADDR);
	while(0)
	{
	#if 0
		switch(i)
		{
			case 0:
				LCD_Clear(BLACK);
				//LCD_ShowImg(w*0,h*0,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*0,h*0,peppa_pig_80X160,WHITE);
				//LCD_dis_pic_from_flash(w*0, h*0, IMG_PEPPA_160X160_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,0);
				//LCD_dis_trans_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,0);
				break;
			case 1:
				LCD_Clear(WHITE);
				//LCD_ShowImg(w*1,h*0,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*1,h*0,peppa_pig_80X160,WHITE);
				//LCD_dis_pic_from_flash(w*1, h*0, IMG_PEPPA_160X160_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,90);
				//LCD_dis_trans_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,90);
				break;
			case 2:
				//LCD_ShowImg(w*1,h*1,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*1,h*1,peppa_pig_80X160,WHITE);
				//LCD_dis_pic_from_flash(w*1, h*1, IMG_PEPPA_160X160_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,180);
				//LCD_dis_trans_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,180);
				break;
			case 3:
				//LCD_ShowImg(w*0,h*1,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*0,h*1,peppa_pig_80X160,WHITE);
				//LCD_dis_pic_from_flash(w*0, h*1, IMG_PEPPA_160X160_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,270);
				//LCD_dis_trans_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,270);
				break;
			case 4:
				LCD_Fill(w*0,h*0,w,h,BLACK);
				break;
			case 5:
				LCD_Fill(w*1,h*0,w,h,BLACK);
				break;
			case 6:
				LCD_Fill(w*1,h*1,w,h,BLACK);
				break;
			case 7:
				LCD_Fill(w*0,h*1,w,h,BLACK);
				break;
		}
	#endif

		i--;
		if(i==0)
			i=LCD_WIDTH-1;

		//LCD_Clear(BLACK);
		LCD_ShowImg(i, 0, logo_5);
		
		k_sleep(K_MSEC(100));								//软件延时1000ms
	}
}

void test_show_stripe(void)
{
	uint16_t x,y,w,h;
	uint8_t i;
	uint16_t color[] = {WHITE,BLACK,RED,GREEN,BLUE,GBLUE,MAGENTA,CYAN,YELLOW,BROWN,BRRED,GRAY};
	
	h = LCD_HEIGHT/8;

	for(i=0;i<8;i++)
	{
		LCD_Fill(0, h*i, LCD_WIDTH, h, color[i%5]);
	}
}

void test_show_color(void)
{
	uint8_t i=0;

	LOGD("test_show_color");
	
	while(1)
	{
		switch(i)
		{
			case 0:
				LCD_Clear(WHITE);
				break;
			case 1:
				LCD_Clear(BLACK);
				break;
			case 2:
				LCD_Clear(RED);
				break;
			case 3:
				LCD_Clear(GREEN);
				break;
			case 4:
				LCD_Clear(BLUE);
				break;
			case 5:
				LCD_Clear(YELLOW);
				break;
			case 6:
				LCD_Clear(GBLUE);
				break;
			case 7:
				LCD_Clear(MAGENTA);
				break;
			case 8:
				LCD_Clear(CYAN);
				break;
			case 9:
				LCD_Clear(BROWN);
				break;
			case 10:
				LCD_Clear(BRRED);
				break;
			case 11:
				LCD_Clear(GRAY);
				break;					
		}
		
		i++;
		if(i>=12)
			i=0;
		
		k_sleep(K_MSEC(1000));								//软件延时1000ms
	}
}

void test_show_string(void)
{
	uint16_t x,y,w,h;
	uint8_t enbuf[64] = {0};
	uint8_t cnbuf[64] = {0};
	uint8_t jpbuf[64] = {0};
	uint16_t en_unibuf[64] = {0x0041,0x0075,0x0067,0x0075,0x0073,0x0074,0x0020,0x0053,0x0068,0x0065,0x006E,0x007A,0x0068,0x0065,0x006E,0x0020,0x0044,0x0049,0x0067,0x0049,0x0074,0x0061,0x006C,0x0020,0x004C,0x0074,0x0064,0x0000};
	uint16_t cn_unibuf[64] = {0x6DF1,0x5733,0x5E02,0x5965,0x79D1,0x65AF,0x6570,0x7801,0x6709,0x9650,0x516C,0x53F8,0x0000};
	uint16_t jp_unibuf[64] = {0x6DF1,0x30BB,0x30F3,0x5E02,0x30AA,0x30FC,0x30B3,0x30B9,0x30C7,0x30B8,0x30BF,0x30EB,0x6709,0x9650,0x4F1A,0x793E,0x0000};

	LCD_Clear(BLACK);
	
	POINT_COLOR=WHITE;								//画笔颜色
	BACK_COLOR=BLACK;  								//背景色 

#ifdef FONTMAKER_UNICODE_FONT
#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);					//设置字体大小	
#elif defined(FONT_24)
	LCD_SetFontSize(FONT_SIZE_24);					//设置字体大小
#elif defined(FONT_16)
	LCD_SetFontSize(FONT_SIZE_16);					//设置字体大小
#endif
	LCD_MeasureUniString(en_unibuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = 30;	
	LCD_ShowUniString(x,y,en_unibuf);

	LCD_MeasureUniString(cn_unibuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;
	LCD_ShowUniString(x,y,cn_unibuf);

	LCD_MeasureUniString(jp_unibuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;
	LCD_ShowUniString(x,y,jp_unibuf);

#if 0
#ifdef FONT_24
	LCD_SetFontSize(FONT_SIZE_24);					//设置字体大小
#endif
	LCD_MeasureUniString(cn_unibuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;	
	LCD_ShowUniString(x,y,cn_unibuf);
	
	LCD_MeasureUniString(en_unibuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;	
	LCD_ShowUniString(x,y,en_unibuf);

#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);					//设置字体大小
#endif
	LCD_MeasureUniString(cn_unibuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;	
	LCD_ShowUniString(x,y,cn_unibuf);
	
	LCD_MeasureUniString(en_unibuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;
	LCD_ShowUniString(x,y,en_unibuf);
#endif
#else
	strcpy(enbuf, "A6");
	strcpy(cnbuf, "哈");
	strcpy(jpbuf, "深セン市オーコスデジタル有限会社");

	x = 0;//(w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;	
	y = 0;

#ifdef FONT_8
	LCD_SetFontSize(FONT_SIZE_8);					//设置字体大小
#endif
	LCD_MeasureString(enbuf,&w,&h);
	LCD_ShowString(x,y,enbuf);

#ifdef FONT_16
	x += w;
	LCD_SetFontSize(FONT_SIZE_16);
	LCD_MeasureString(enbuf,&w,&h);
	LCD_ShowString(x,y,enbuf);
#endif

#ifdef FONT_24
	x += w;
	LCD_SetFontSize(FONT_SIZE_24);
	LCD_MeasureString(enbuf,&w,&h);
	LCD_ShowString(x,y,enbuf);	
#endif

#ifdef FONT_32
	x += w;
	LCD_SetFontSize(FONT_SIZE_32);
	LCD_ShowString(x,y,enbuf);
#endif	
	//LCD_MeasureString(cnbuf,&w,&h);
	//x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	//y = y + h + 2;
	//LCD_ShowString(x,y,cnbuf);
	
	//LCD_MeasureString(jpbuf,&w,&h);
	//x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	//y = y + h + 2;
	//LCD_ShowString(x,y,jpbuf);

#if 0
#ifdef FONT_24
	LCD_SetFontSize(FONT_SIZE_24);					//设置字体大小
#endif
	LCD_MeasureString(cnbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;	
	LCD_ShowString(x,y,cnbuf);
	
	LCD_MeasureString(enbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;	
	LCD_ShowString(x,y,enbuf);

#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);					//设置字体大小
#endif
	LCD_MeasureString(cnbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;	
	LCD_ShowString(x,y,cnbuf);
	
	LCD_MeasureString(enbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;
	LCD_ShowString(x,y,enbuf);
#endif
#endif
}


static void modem_init(void)
{
	nrf_modem_lib_init(NORMAL_MODE);
	boot_write_img_confirmed();
}

void system_init(void)
{
	k_sleep(K_MSEC(500));//xb test 2022-03-11 启动时候延迟0.5S,等待其他外设完全启动

	modem_init();

#ifdef CONFIG_FOTA_DOWNLOAD
	fota_init();
#endif

	InitSystemSettings();

#ifdef CONFIG_IMU_SUPPORT
	init_imu_int1();//xb add 2022-05-27
#endif
	pmu_init();
	key_init();
	LCD_Init();
	flash_init();
	
	ShowBootUpLogo();

#ifdef CONFIG_AUDIO_SUPPORT	
	audio_init();
#endif
	ble_init();
#ifdef CONFIG_PPG_SUPPORT	
	PPG_init();
#endif
#ifdef CONFIG_IMU_SUPPORT
	IMU_init(&imu_work_q);
#endif
	LogInit();

	NB_init(&nb_work_q);
	GPS_init(&gps_work_q);
}

void work_init(void)
{
	k_work_queue_start(&nb_work_q, nb_stack_area,
					K_THREAD_STACK_SIZEOF(nb_stack_area),
					CONFIG_APPLICATION_WORKQUEUE_PRIORITY,NULL);
#ifdef CONFIG_IMU_SUPPORT	
	k_work_queue_start(&imu_work_q, imu_stack_area,
					K_THREAD_STACK_SIZEOF(imu_stack_area),
					CONFIG_APPLICATION_WORKQUEUE_PRIORITY,NULL);
#endif
	k_work_queue_start(&gps_work_q, gps_stack_area,
					K_THREAD_STACK_SIZEOF(gps_stack_area),
					CONFIG_APPLICATION_WORKQUEUE_PRIORITY,NULL);	
	
	if(IS_ENABLED(CONFIG_WATCHDOG))
	{
		watchdog_init_and_start(&k_sys_work_q);
	}
}

bool system_is_completed(void)
{
	return sys_pwron_completed_flag;
}

void system_init_completed(void)
{
	if(!sys_pwron_completed_flag)
		sys_pwron_completed_flag = true;
}

/***************************************************************************
* 描  述 : main函数 
* 入  参 : 无 
* 返回值 : int 类型
**************************************************************************/
int main(void)
{
	work_init();
	system_init();

//	test_show_string();
//	test_show_image();
//	test_show_color();
//	test_show_stripe();
//	test_nvs();
//	test_flash();
//	test_uart_ble();
//	test_sensor();
//	test_show_digital_clock();
//	test_sensor();
//	test_pmu();
//	test_crypto();
//	test_imei();
//	test_tp();
//	test_gps_on();
//	test_nb();
//	test_i2c();
//	test_bat_soc();

	while(1)
	{
		KeyMsgProcess();
		TimeMsgProcess();
		NBMsgProcess();
	#ifdef CONFIG_WIFI_SUPPORT	
		WifiProcess();
	#endif
		GPSMsgProcess();
		PMUMsgProcess();
	#ifdef CONFIG_IMU_SUPPORT	
		IMUMsgProcess();
	#ifdef CONFIG_FALL_DETECT_SUPPORT
		FallMsgProcess();
	#endif
	#endif
	#ifdef CONFIG_PPG_SUPPORT	
		PPGMsgProcess();
	#endif
		LCDMsgProcess();
	#ifdef CONFIG_TOUCH_SUPPORT
		TPMsgProcess();
	#endif
	#ifdef CONFIG_ALARM_SUPPORT
		AlarmMsgProcess();
	#endif
		SettingsMsgPorcess();
		SOSMsgProc();
		UartMsgProc();
		ScreenMsgProcess();
	#ifdef CONFIG_FOTA_DOWNLOAD
		FotaMsgProc();
	#endif
	#ifdef CONFIG_AUDIO_SUPPORT
		AudioMsgProcess();
	#endif
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaMsgProcess();
	#endif
	#ifdef CONFIG_SYNC_SUPPORT
		SyncMsgProcess();
	#endif
		LogMsgProcess();
		system_init_completed();
		k_cpu_idle();
	}
}
