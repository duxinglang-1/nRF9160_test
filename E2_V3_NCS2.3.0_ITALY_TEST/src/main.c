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
#ifdef CONFIG_TEMP_SUPPORT
#include "temp.h"
#endif
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif/*CONFIG_WIFI_SUPPORT*/
#include "logger.h"

static bool sys_pwron_completed_flag = false;

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(nb_stack_area, 4096);
static struct k_work_q nb_work_q;

#ifdef CONFIG_IMU_SUPPORT
K_THREAD_STACK_DEFINE(imu_stack_area, 2048);
static struct k_work_q imu_work_q;
#endif

K_THREAD_STACK_DEFINE(gps_stack_area, 4096);
static struct k_work_q gps_work_q;

void test_show_image(void)
{
	uint8_t i=0;
	uint16_t x,y,w=0,h=0;
	
	LCD_Clear(BLACK);
	
	//LCD_get_pic_size(peppa_pig_160X160, &w, &h);
	//LCD_dis_pic_rotate(0,200,peppa_pig_160X160,270);
	//LCD_dis_pic(0, 0, peppa_pig_160X160);
	LCD_MeasureImage(IMG_ID_STEP_UNIT, &w, &h);
	LCD_ShowImage((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, IMG_ID_STEP_UNIT);
	//LCD_dis_pic_rotate_from_flash((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, IMG_ANALOG_CLOCK_HAND_HOUR_ADDR, 270);
	//LCD_dis_pic_angle_from_flash(0, 0, IMG_ANALOG_CLOCK_HAND_SEC_ADDR, 360);
	while(0)
	{
	#if 0	
		switch(i)
		{
			case 0:
				//LCD_dis_pic(w*0,h*0,peppa_pig_160X160);
				//LCD_dis_pic_trans(w*0,h*0,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*0, h*0, IMG_ANALOG_CLOCK_BG_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,0);
				//LCD_dis_pic_trans_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,0);
				break;
			case 1:
				//LCD_dis_pic(w*1,h*0,peppa_pig_160X160);
				//LCD_dis_pic_trans(w*1,h*0,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*1, h*0, IMG_ANALOG_CLOCK_BG_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,90);
				//LCD_dis_pic_trans_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,90);
				break;
			case 2:
				//LCD_dis_pic(w*1,h*1,peppa_pig_160X160);
				//LCD_dis_pic_trans(w*1,h*1,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*1, h*1, IMG_ANALOG_CLOCK_BG_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,180);
				//LCD_dis_pic_trans_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,180);
				break;
			case 3:
				//LCD_dis_pic(w*0,h*1,peppa_pig_160X160);
				//LCD_dis_pic_trans(w*0,h*1,peppa_pig_80X160,WHITE);
				LCD_dis_pic_from_flash(w*0, h*1, IMG_ANALOG_CLOCK_BG_ADDR);
				//LCD_dis_pic_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,270);
				//LCD_dis_pic_trans_rotate((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,peppa_pig_160X160,WHITE,270);
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

		i++;
		if(i==23)
			i = 0;
		
		k_sleep(K_MSEC(1000));								//软件延时1000ms
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
	uint8_t tmpbuf[256] = {0};
	uint16_t test_buf[] = {0x6DF1,0x5733,0x5E02,0x0041,0x0075,0x0067,0x0075,0x0073,0x0074,0x6570,0x7801,0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0000};//深圳市August数码012345

	LCD_Clear(BLACK);
	
	POINT_COLOR=WHITE;								//画笔颜色
	BACK_COLOR=BLACK;  								//背景色 

#ifdef FONTMAKER_UNICODE_FONT
#if 0//def FONT_68
	LCD_SetFontSize(FONT_SIZE_68);					//设置字体大小
#elif 0//defined(FONT_48)
	LCD_SetFontSize(FONT_SIZE_48);					//设置字体大小
#elif 0//defined(FONT_32)
	LCD_SetFontSize(FONT_SIZE_32);					//设置字体大小	
#elif defined(FONT_28)
	//LCD_SetFontSize(FONT_SIZE_28);					//锟斤拷锟斤拷锟斤拷锟斤拷锟叫?
#elif defined(FONT_20)
	//LCD_SetFontSize(FONT_SIZE_20);		
#elif defined(FONT_24)
	//LCD_SetFontSize(FONT_SIZE_24);					//锟斤拷锟斤拷锟斤拷锟斤拷锟叫?
#elif defined(FONT_16)
	//LCD_SetFontSize(FONT_SIZE_16);					//锟斤拷锟斤拷锟斤拷锟斤拷锟叫?
#endif
	//LCD_MeasureUniString(test_buf, &w, &h);

	LCD_SetFontSize(FONT_SIZE_16);
	LCD_MeasureUniString(test_buf, &w, &h);
	LCD_ShowUniString(10,80,test_buf);
	LCD_SetFontSize(FONT_SIZE_20);
	LCD_MeasureUniString(test_buf, &w, &h);
	LCD_ShowUniString(10,100,test_buf);
	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(test_buf, &w, &h);
	LCD_ShowUniString(10,130,test_buf);
	//LCD_ShowUniString(x,y,test_buf);

#else

	strcpy(enbuf, "August Shenzhen Digital Ltd");
	strcpy(cnbuf, "深圳市奥科斯数码有限公司");
	strcpy(jpbuf, "深セン市オーコスデジタル有限会社");

#ifdef FONT_32
	LCD_SetFontSize(FONT_SIZE_32);					//设置字体大小	
#elif defined(FONT_24)
	LCD_SetFontSize(FONT_SIZE_24);					//设置字体大小
#elif defined(FONT_16)
	LCD_SetFontSize(FONT_SIZE_16);					//设置字体大小
#endif

	LCD_MeasureString(enbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;	
	LCD_ShowString(x,y,enbuf);
	
	LCD_MeasureString(cnbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;
	LCD_ShowString(x,y,cnbuf);
	
	LCD_MeasureString(jpbuf,&w,&h);
	x = (w > LCD_WIDTH)? 0 : (LCD_WIDTH-w)/2;
	y = y + h + 2;
	LCD_ShowString(x,y,jpbuf);

#endif
}

void test_notify(void)
{
	notify_infor infor = {0};
	uint16_t str_note[LANGUAGE_MAX][80] = {
											{0x0054,0x0068,0x0065,0x0020,0x0074,0x0069,0x006D,0x0069,0x006E,0x0067,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x0061,0x0062,0x006F,0x0075,0x0074,0x0020,0x0074,0x006F,0x0020,0x0073,0x0074,0x0061,0x0072,0x0074,0x002C,0x0020,0x0061,0x006E,0x0064,0x0020,0x0074,0x0068,0x0069,0x0073,0x0020,0x006D,0x0065,0x0061,0x0073,0x0075,0x0072,0x0065,0x006D,0x0065,0x006E,0x0074,0x0020,0x0069,0x0073,0x0020,0x006F,0x0076,0x0065,0x0072,0x0021,0x0000},//The timing measurement is about to start, and this measurement is over!
											{0x0044,0x0069,0x0065,0x0020,0x005A,0x0065,0x0069,0x0074,0x006D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0073,0x0074,0x0065,0x0068,0x0074,0x0020,0x006B,0x0075,0x0072,0x007A,0x0020,0x0062,0x0065,0x0076,0x006F,0x0072,0x002C,0x0020,0x0075,0x006E,0x0064,0x0020,0x0064,0x0069,0x0065,0x0073,0x0065,0x0020,0x004D,0x0065,0x0073,0x0073,0x0075,0x006E,0x0067,0x0020,0x0069,0x0073,0x0074,0x0020,0x0076,0x006F,0x0072,0x0062,0x0065,0x0069,0x0021,0x0000},//Die Zeitmessung steht kurz bevor, und diese Messung ist vorbei!
											{0x5B9A,0x65F6,0x6D4B,0x91CF,0x5373,0x5C06,0x5F00,0x59CB,0xFF0C,0x672C,0x6B21,0x6D4B,0x91CF,0x7ED3,0x675F,0xFF01,0x0000},//定时测量即将开始，本次测量结束！
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
	mmi_ucs2cpy(infor.text, (uint8_t*)str_note[0]);

	DisplayPopUp(infor);
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
#ifdef CONFIG_PPG_SUPPORT
	PPG_i2c_off();
#endif
	pmu_init();
	key_init();
	LCD_Init();
	flash_init();
	
	ShowBootUpLogo();
	
#ifdef CONFIG_PPG_SUPPORT	
	PPG_init();
#endif
#ifdef CONFIG_AUDIO_SUPPORT	
	audio_init();
#endif
	ble_init();
#ifdef CONFIG_WIFI_SUPPORT
	wifi_init();
#endif
#ifdef CONFIG_IMU_SUPPORT
	IMU_init(&imu_work_q);
#endif
#ifdef CONFIG_TEMP_SUPPORT
	temp_init();
#endif
#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
	dl_init();
#endif
#ifdef TEST_DEBUG
	LogInit();
#endif

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
//	test_imei_for_qr();
//	test_tp();
//	test_gps_on();
//	test_nb();
//	test_i2c();
//	test_bat_soc();
//	test_notify();
//	test_wifi();
//	LogInit();

	while(1)
	{
		KeyMsgProcess();
		TimeMsgProcess();
		NBMsgProcess();
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
	#ifdef CONFIG_WIFI_SUPPORT
		WifiMsgProcess();
	#endif
		UartMsgProc();
	#ifdef CONFIG_ANIMATION_SUPPORT
		AnimaMsgProcess();
	#endif		
		ScreenMsgProcess();
	#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
		DlMsgProc();
	#endif
	#ifdef CONFIG_FOTA_DOWNLOAD
		FotaMsgProc();
	#endif
	#ifdef CONFIG_AUDIO_SUPPORT
		AudioMsgProcess();
	#endif
	#ifdef CONFIG_SYNC_SUPPORT
		SyncMsgProcess();
	#endif
	#ifdef CONFIG_TEMP_SUPPORT
		TempMsgProcess();
	#endif
	#ifdef CONFIG_FACTORY_TEST_SUPPORT
		FactoryTestProccess();
	#endif
	#ifdef TEST_DEBUG
		LogMsgProcess();
	#endif
		system_init_completed();
		k_cpu_idle();
	}
}
