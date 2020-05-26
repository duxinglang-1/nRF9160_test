//This is a spi sample to initalize spi driver of nRF9160 board.
//No clock and slave select connection are required.
//Simple connect MOSI to MISO i.e. pin 11 to 12. A data loop back. 
//Data sent by the board is read by the board.

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

#define BUTTON_1			BIT(0)
#define BUTTON_2			BIT(1)
#define SWITCH_1			BIT(2)
#define SWITCH_2			BIT(3)


#define PI 3.1415926

static bool show_date_time_first = true;
static bool update_date_time = false;

static u8_t show_pic_count = 0;//图片显示顺序
static sys_date_timer_t date_time = {0};
static sys_date_timer_t last_date_time = {0};

static u8_t language_mode = 0;//0:chinese  1:english
static u8_t clock_mode = 0;//0:analog  1:digital
static u8_t date_time_changed = 0;//通过位来判断日期时间是否有变化，从第6位算起，分表表示年月日时分秒

static struct k_timer clock_timer;

bool lcd_sleep_in = false;
bool lcd_sleep_out = false;

static void test_show_analog_clock(void);
static void test_show_digital_clock(void);
static void idle_show_time(void);


//Timer时间回调函数
static void clock_timer_handler(struct k_timer *timer)
{
	printk("clock_timer_handler\n");

    update_date_time = true;
}

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

/***************************************************************************
* 描  述 : idle_show_digit_clock 待机界面显示数字时钟
* 入  参 : 无 
* 返回值 : 无
**************************************************************************/
void idle_show_digital_clock(void)
{
	u16_t x,y,w,h;
	u8_t str_time[20] = {0};
	u8_t str_date[20] = {0};
	u8_t str_week[20] = {0};
	u8_t *week[7] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
	
	POINT_COLOR=WHITE;
	BACK_COLOR=BLACK;
	LCD_SetFontSize(FONT_SIZE_16);
	
	sprintf((char*)str_time, "%02d:%02d:%02d", date_time.hour, date_time.minute, date_time.second);
	sprintf((char*)str_date, "%04d/%02d/%02d", date_time.year, date_time.month, date_time.day);
	strcpy((char*)str_week, (const char*)week[date_time.week]);
	
	LCD_MeasureString(str_time,&w,&h);
	x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
	y = (LCD_HEIGHT > h) ? (LCD_HEIGHT-h)/2 : 0;
	LCD_ShowString(x,y,str_time);//时间
	
	if(show_date_time_first)
	{
		show_date_time_first = false;
		
		LCD_MeasureString(str_date,&w,&h);
		x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
		y = (LCD_HEIGHT > h) ? (LCD_HEIGHT-h)/2+30 : 0;
		LCD_ShowString(x,y,str_date);//日期

		LCD_MeasureString(str_week,&w,&h);
		x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
		y = (LCD_HEIGHT > h) ? (LCD_HEIGHT-h)/2+60 : 0;
		LCD_ShowString(x,y,str_week);//星期
	}
	else if((date_time_changed&0x38) != 0)//日期有改变
	{
		LCD_MeasureString(str_date,&w,&h);
		x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
		y = (LCD_HEIGHT > h) ? (LCD_HEIGHT-h)/2+30 : 0;
		LCD_ShowString(x,y,str_date);//日期

		LCD_MeasureString(str_week,&w,&h);
		x = (LCD_WIDTH > w) ? (LCD_WIDTH-w)/2 : 0;
		y = (LCD_HEIGHT > h) ? (LCD_HEIGHT-h)/2+60 : 0;
		LCD_ShowString(x,y,str_week);//星期
		
		date_time_changed = date_time_changed&0xC7;//清空日期变化标志位
	}
}

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

void idle_show_clock_background(void)
{
	LCD_Clear(BLACK);
	BACK_COLOR=BLACK;
	POINT_COLOR=WHITE;
	
	LCD_SetFontSize(FONT_SIZE_16);
	
	if(clock_mode == 0)
	{
		LCD_dis_pic(0,0,clock_bg_80X160);
		LCD_ShowString(40,120,"88");
	}
}

/***************************************************************************
* 描  述 : idle_show_time 待机界面显示时间
* 入  参 : 无 
* 返回值 : 无
**************************************************************************/
void idle_show_time(void)
{	
	memcpy(&last_date_time, &date_time, sizeof(sys_date_timer_t));
	
	date_time.second++;
	if(date_time.second > 59)
	{
		date_time.second = 0;
		date_time.minute++;
		date_time_changed = date_time_changed|0x02;
		//date_time_changed = date_time_changed|0x04;//分针在变动的同时，时针也会同步缓慢变动
		if(date_time.minute > 59)
		{
			date_time.minute = 0;
			date_time.hour++;
			date_time_changed = date_time_changed|0x04;
			if(date_time.hour > 23)
			{
				date_time.hour = 0;
				date_time.day++;
				date_time.week++;
				if(date_time.week > 6)
					date_time.week = 0;
				date_time_changed = date_time_changed|0x08;
				if(date_time.month == 1 \
				|| date_time.month == 3 \
				|| date_time.month == 5 \
				|| date_time.month == 7 \
				|| date_time.month == 8 \
				|| date_time.month == 10 \
				|| date_time.month == 12)
				{
					if(date_time.day > 31)
					{
						date_time.day = 1;
						date_time.month++;
						date_time_changed = date_time_changed|0x10;
						if(date_time.month > 12)
						{
							date_time.month = 1;
							date_time.year++;
							date_time_changed = date_time_changed|0x20;
						}
					}
				}
				else if(date_time.month == 4 \
					|| date_time.month == 6 \
					|| date_time.month == 9 \
					|| date_time.month == 11)
				{
					if(date_time.day > 30)
					{
						date_time.day = 1;
						date_time.month++;
						date_time_changed = date_time_changed|0x10;
						if(date_time.month > 12)
						{
							date_time.month = 1;
							date_time.year++;
							date_time_changed = date_time_changed|0x20;
						}
					}
				}
				else
				{
					uint8_t Leap = date_time.year%4;
					
					if(date_time.day > (28+Leap))
					{
						date_time.day = 1;
						date_time.month++;
						date_time_changed = date_time_changed|0x10;
						if(date_time.month > 12)
						{
							date_time.month = 1;
							date_time.year++;
							date_time_changed = date_time_changed|0x20;
						}
					}
				}
			}
		}
	}
	date_time_changed = date_time_changed|0x01;		

	//每分钟保存一次时间
	if((date_time_changed&0x02) != 0)
	{
		SetSystemDateTime(date_time);
	}

	if(clock_mode == 0)
	{
		if((date_time_changed&0x02) != 0)
		{
			ClearAnalogMinPic(date_time.minute);//擦除以前的分钟
		}
		if((date_time_changed&0x04) != 0)
		{
			ClearAnalogHourPic(date_time.hour);//擦除以前的时钟
		}

		idle_show_analog_clock();
	}
	else
	{
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
	}
}

void test_show_analog_clock(void)
{
	u32_t err_code;
	
	clock_mode = 0;
	
	GetSystemDateTime(&date_time);
	
	idle_show_clock_background();
	idle_show_time();

	k_timer_init(&clock_timer, clock_timer_handler, NULL);
	k_timer_start(&clock_timer, K_MSEC(1000), K_MSEC(1000));
}

void test_show_digital_clock(void)
{
	u32_t err_code;
	
	clock_mode = 1;
	
	GetSystemDateTime(&date_time);
	
	idle_show_clock_background();
	idle_show_time();

	k_timer_init(&clock_timer, clock_timer_handler, NULL);
	k_timer_start(&clock_timer, K_MSEC(1000), K_MSEC(1000));
}

void test_show_image(void)
{
	u8_t i=0;
	u16_t x,y,w,h;

	printk("test_show_image\n");
	
	LCD_Clear(BLACK);
	LCD_get_pic_size(clock_hour_1_31X31, &w, &h);
	while(1)
	{
		switch(i)
		{
			case 0:
				LCD_dis_pic(w*0,h*0,clock_hour_1_31X31);
				break;
			case 1:
				LCD_dis_pic(w*1,h*0,clock_hour_1_31X31);
				break;
			case 2:
				LCD_dis_pic(w*1,h*1,clock_hour_1_31X31);
				break;
			case 3:
				LCD_dis_pic(w*0,h*1,clock_hour_1_31X31);
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

void test_show_string(void)
{
	u16_t x,y,w,h;
	
	LCD_Clear(BLACK);								//清屏
	
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
	buttons_leds_init();

	key_init();
	
	LCD_Init();	
}

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
	
	test_show_string();
//	test_show_image();
//	test_show_analog_clock();
//	test_show_digital_clock();
	
	while(true)
	{
		if(update_date_time)
		{
			update_date_time = false;
			idle_show_time();

			count = !count;
			dk_set_led(DK_LED2,count);

			printk("blink led2, count:%d\n", count);
		}

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
	}
}
