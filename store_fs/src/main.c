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
#include "lcd.h"
#include "font.h"
#include "img.h"
#include "inner_flash.h"
#include "external_flash.h"
#include "settings.h"
//#include "CST816.h"
//#include "Max20353.h"


#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(uart_ble, CONFIG_LOG_DEFAULT_LEVEL);


static u8_t show_pic_count = 0;//图片显示顺序

void test_show_image(void)
{
	u8_t i=0;
	u16_t x,y,w=0,h=0;

	LOG_INF("test_show_image\n");
	
	LCD_Clear(BLACK);
	
	//LCD_get_pic_size(peppa_pig_160X160, &w, &h);
	//LCD_dis_pic_rotate(0,200,peppa_pig_160X160,270);
	//LCD_dis_pic(0, 0, peppa_pig_160X160);
	LCD_get_pic_size_from_flash(IMG_RM_LOGO_240X240_ADDR, &w, &h);
	LCD_dis_pic_from_flash(0, 0, IMG_RM_LOGO_240X240_ADDR);
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

	LOG_INF("test_show_image\n");
	
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
	u8_t cnbuf[128] = {0};
	u8_t enbuf[128] = {0};
	
	POINT_COLOR=WHITE;								//画笔颜色
	BACK_COLOR=BLACK;  								//背景色 

	strcpy(cnbuf, "深圳市奥科斯数码有限公司");
	strcpy(enbuf, "2015-2020 August International Ltd. All Rights Reserved.");

#ifdef FONT_16
	LCD_SetFontSize(FONT_SIZE_16);					//设置字体大小
#endif
	LCD_MeasureString(cnbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = 20;	
	LCD_ShowString(x,y,cnbuf);
	
	LCD_MeasureString(enbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;
	LCD_ShowString(x,y,enbuf);

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
}

void BootUpShowLoGo(void)
{
	u16_t x,y,w=0,h=0;
		
	if(screen_id == SCREEN_BOOTUP)
	{
		LCD_get_pic_size_from_flash(IMG_RM_LOGO_240X240_ADDR, &w, &h);
		x = (w > LCD_WIDTH ? 0 : (LCD_WIDTH-w)/2);
		y = (h > LCD_HEIGHT ? 0 : (LCD_HEIGHT-h)/2);
		LCD_dis_pic_from_flash(0, 0, IMG_RM_LOGO_240X240_ADDR);
	}
}

void EntryIdleScreen(void)
{
	screen_id = SCREEN_IDLE;
}

void system_init(void)
{
	flash_init();
	pmu_init();
	LCD_Init();
	BootUpShowLoGo();

	InitSystemSettings();

	key_init();

	EntryIdleScreen();
}

extern void motion_sensor_msg_proc(void);
/***************************************************************************
* 描  述 : main函数 
* 入  参 : 无 
* 返回值 : int 类型
**************************************************************************/
int main(void)
{
	system_init();

	test_show_string();
//	test_show_image();
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
//	test_gps();
//	test_nb();

	while(1)
	{
		PMUMsgProcess();
		LCDMsgProcess();

		SettingsMsgPorcess();
		
		k_cpu_idle();
	}
}
