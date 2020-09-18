/*
* Copyright (c) 2019 Nordic Semiconductor ASA
*
* SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
*/

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <stdio.h>
#include <sys/printk.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <dk_buttons_and_leds.h>
#include "lcd.h"
#include "datetime.h"
#include "font.h"
#include "img.h"
#include "inner_flash.h"
#include "external_flash.h"
#include "uart_ble.h"
#include "settings.h"
//#include "CST816.h"
//#include "Max20353.h"

//#define ANALOG_CLOCK
#define DIGITAL_CLOCK

#define PI 3.1415926

static u8_t show_pic_count = 0;//图片显示顺序

u8_t date_time_changed = 0;//通过位来判断日期时间是否有变化，从第6位算起，分表表示年月日时分秒

bool lcd_sleep_in = false;
bool lcd_sleep_out = false;
bool sys_pwr_off = false;
bool sys_time_count = false;
bool show_date_time_first = true;
bool update_time = false;
bool update_date = false;
bool update_week = false;
bool update_date_time = false;


#if defined(ANALOG_CLOCK)
static void test_show_analog_clock(void);
#elif defined(DIGITAL_CLOCK)
static void test_show_digital_clock(void);
#endif
static void idle_show_time(void);


#ifdef ANALOG_CLOCK
void ClearAnalogHourPic(int hour)
{
	u16_t offset_x=4,offset_y=4;
	u16_t hour_x,hour_y,hour_w,hour_h;

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
	u16_t offset_x=4,offset_y=4;
	u16_t hour_x,hour_y,hour_w,hour_h;
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
	u16_t offset_x=4,offset_y=4;
	u16_t min_x,min_y,min_w,min_h;
	
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
	u16_t offset_x=4,offset_y=4;
	u16_t min_x,min_y,min_w,min_h;
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
	u8_t str_date[20] = {0};
	u8_t str_week[20] = {0};
	u8_t *week_cn[7] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
	u8_t *week_en[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	u16_t str_w,str_h;

	POINT_COLOR=WHITE;								//画笔颜色
	BACK_COLOR=BLACK;  								//背景色 
	
	LCD_SetFontSize(FONT_SIZE_16);

	sprintf((char*)str_date, "%02d/%02d", date_time.day,date_time.month);
	if(language_mode == 0)
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
		LCD_dis_pic(0,0,clock_bg_80X160);
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
	u32_t err_code;
	
	global_settings.idle_colck_mode = CLOCK_MODE_ANALOG;
	
	idle_show_clock_background();
	idle_show_time();
}

void test_show_digital_clock(void)
{
	u32_t err_code;
	
	global_settings.idle_colck_mode == CLOCK_MODE_DIGITAL;
	
	idle_show_clock_background();
	idle_show_time();
}

void test_show_image(void)
{
	u8_t i=0;
	u16_t x,y,w=0,h=0;

	printk("test_show_image\n");
	
	LCD_Clear(BLACK);
	
	//LCD_get_pic_size(peppa_pig_160X160, &w, &h);
	//LCD_dis_pic_rotate(0,200,peppa_pig_160X160,270);
	//LCD_dis_pic(0, 0, peppa_pig_160X160);
	LCD_get_pic_size_from_flash(IMG_PEPPA_240X240_ADDR, &w, &h);
	LCD_dis_pic_from_flash(0, 0, IMG_PEPPA_240X240_ADDR);
	while(0)
	{
		switch(i)
		{
			case 0:
				//LCD_dis_pic(w*0,h*0,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*0,h*0,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*0, h*0, IMG_PEPPA_160X160_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,0);
				//LCD_dis_trans_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,0);
				break;
			case 1:
				//LCD_dis_pic(w*1,h*0,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*1,h*0,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*1, h*0, IMG_PEPPA_160X160_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,90);
				//LCD_dis_trans_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,90);
				break;
			case 2:
				//LCD_dis_pic(w*1,h*1,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*1,h*1,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*1, h*1, IMG_PEPPA_160X160_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,180);
				//LCD_dis_trans_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,180);
				break;
			case 3:
				//LCD_dis_pic(w*0,h*1,peppa_pig_160X160);
				//LCD_dis_trans_pic(w*0,h*1,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*0, h*1, IMG_PEPPA_160X160_ADDR);
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
		
		i++;
		if(i>=8)
			i=0;
		
		k_sleep(K_MSEC(1000));								//软件延时1000ms
	}
}

void test_show_color(void)
{
	u8_t i=0;

	printk("test_show_image\n");
	
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
				LCD_Clear(BLUE);
				break;
			case 3:
				LCD_Clear(BRED);
				break;
			case 4:
				LCD_Clear(GRED);
				break;
			case 5:
				LCD_Clear(GBLUE);
				break;
			case 6:
				LCD_Clear(RED);
				break;
			case 7:
				LCD_Clear(MAGENTA);
				break;
			case 8:
				LCD_Clear(GREEN);
				break;
			case 9:
				LCD_Clear(CYAN);
				break;
			case 10:
				LCD_Clear(YELLOW);
				break;
			case 11:
				LCD_Clear(BROWN);
				break;
			case 12:
				LCD_Clear(BRRED);
				break;
			case 13:
				LCD_Clear(GRAY);
				break;					
		}
		
		i++;
		if(i>=14)
			i=0;
		
		k_sleep(K_MSEC(1000));								//软件延时1000ms
	}
}

void test_show_string(void)
{
	u16_t x,y,w,h;
	
	POINT_COLOR=WHITE;								//画笔颜色
	BACK_COLOR=BLACK;  								//背景色 

#ifdef FONT_16
	LCD_SetFontSize(FONT_SIZE_16);					//设置字体大小
#endif
	LCD_MeasureString("深圳市奥科斯数码有限公司",&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = 40;	
	LCD_ShowString(x,y,"深圳市奥科斯数码有限公司");
	
	LCD_MeasureString("2015-2020 August International Ltd. All Rights Reserved.",&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 10;	
	LCD_ShowString(x,y,"2015-2020 August International Ltd. All Rights Reserved.");

#ifdef FONT_24
	LCD_SetFontSize(FONT_SIZE_24);					//设置字体大小
#endif
	LCD_MeasureString("Rawmec Business Park, Plumpton Road, Hoddesdon",&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 10;	
	LCD_ShowString(x,y,"Rawmec Business Park, Plumpton Road, Hoddesdon");
	
	LCD_MeasureString("深圳市龙华观澜环观南路凯美广场A座",&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 10;	
	LCD_ShowString(x,y,"深圳市龙华观澜环观南路凯美广场A座");

#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);					//设置字体大小
#endif
	LCD_MeasureString("2020-01-03 16:30:45",&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 10;	
	LCD_ShowString(x,y,"2020-01-03 16:30:45");
}

/**@brief Initializes buttons and LEDs, using the DK buttons and LEDs
 * library.
 */
static void buttons_leds_init(void)
{
	int err;

	err = dk_leds_init();
	if (err)
	{
		printk("Could not initialize leds, err code: %d\n", err);
	}

	err = dk_set_leds_state(0x00, DK_ALL_LEDS_MSK);
	if (err)
	{
		printk("Could not set leds state, err code: %d\n", err);
	}
}

void system_init(void)
{
	InitSystemSettings();
	buttons_leds_init();
	key_init();
	//pmu_init();
	LCD_Init();
	flash_init();
	ble_init();
}

extern void motion_sensor_msg_proc(void);
/***************************************************************************
* 描  述 : main函数 
* 入  参 : 无 
* 返回值 : int 类型
**************************************************************************/
int main(void)
{
	bool count = false;

	printk("main\n");

	system_init();
	dk_set_led(DK_LED1,1);

//	test_show_string();
//	test_show_image();
//	test_nvs();
//	test_flash();
//	test_uart_ble();
//	test_sensor();
//	test_show_digital_clock();
	test_sensor();
//	test_pmu();
//	test_crypto();
//	test_imei();
//	test_tp();

//	pmu_alert_proc();

	while(1)
	{
	#if 0
		if(update_time || update_date || update_week || update_date_time)
		{
			if(update_date_time || show_date_time_first)
			{
				update_date_time = false;
				show_date_time_first = false;
				IdleShowSystemDateTime();
			}
			else if(update_date)
			{
				update_date = false;
				IdleShowSystemDate();
			}
			else if(update_week)
			{
				update_week = false;
				IdleShowSystemWeek();
			}
			else
			{
				update_time = false;
				IdleShowSystemTime();
			}
			
			count = !count;
			dk_set_led(DK_LED2,count);
		}
	#endif
		if(sys_time_count)
		{
			sys_time_count = false;
			UpdateSystemTime();
		}

		if(need_save_time)
		{
			need_save_time = false;
			SaveSystemDateTime();
		}

		if(need_save_settings)
		{
			need_save_settings = false;
			SaveSystemSettings();
		}
	#if 0
		if(tp_trige_flag)
		{
			tp_trige_flag = false;
			tp_interrupt_proc();
		}
	#endif
	
	#if 0
		if(pmu_trige_flag)
		{
			pmu_trige_flag = false;
			pmu_interrupt_proc();
		}
		if(pmu_alert_flag)
		{
			pmu_alert_flag = false;
			pmu_alert_proc();
		}
	#endif
	
		if(lcd_sleep_in)
		{
			lcd_sleep_in = false;
			LCD_SleepIn();
		}

		if(lcd_sleep_out)
		{
			lcd_sleep_out = false;
			LCD_SleepOut();
		}
	#if 0	
		if(sys_pwr_off)
		{
			sys_pwr_off = false;
			SystemShutDown();
		}
	#endif	

		motion_sensor_msg_proc();
	
		k_cpu_idle();
	}
}
