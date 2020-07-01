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
#include "font.h"
#include "img.h"
#include "inner_flash.h"
#include "external_flash.h"

bool lcd_sleep_in = false;
bool lcd_sleep_out = false;


void test_show_image(void)
{
	u8_t i=0;
	u16_t x,y,w,h;

	printk("test_show_image\n");
	
	LCD_Clear(BLACK);
	
	LCD_get_pic_size(peppa_pig_80X160, &w, &h);
	while(1)
	{
		switch(i)
		{
			case 0:
				LCD_dis_pic(w*0,h*0,peppa_pig_80X160);
				break;
			case 1:
				LCD_dis_pic(w*1,h*0,peppa_pig_80X160);
				break;
			case 2:
				LCD_dis_pic(w*1,h*1,peppa_pig_80X160);
				break;
			case 3:
				LCD_dis_pic(w*0,h*1,peppa_pig_80X160);
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
	
	//LCD_Clear(BLACK);								//清屏
	
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

//	test_nvs();
	test_flash();

	test_show_string();
	
	while(true)
	{
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
