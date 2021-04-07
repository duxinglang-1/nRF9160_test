/*
* Copyright (c) 2019 Nordic Semiconductor ASA
*
* SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
*/

#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include <drivers/uart.h>
#include <dk_buttons_and_leds.h>
#include "datetime.h"
#include "Settings.h"
#include "Uart_ble.h"
#include "CST816.h"
#include "gps.h"
#include "max20353.h"
#include "screen.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(uart0_test, CONFIG_LOG_DEFAULT_LEVEL);

#define BLE_DEV	"UART_0"

#define BUF_MAXSIZE	1024

#define PACKET_HEAD	0xAB
#define PACKET_END	0x88

#define HEART_RATE_ID			0xFF31			//心率
#define BLOOD_OXYGEN_ID			0xFF32			//血氧
#define BLOOD_PRESSURE_ID		0xFF33			//血压
#define	ONE_KEY_MEASURE_ID		0xFF34			//一键测量
#define	PULL_REFRESH_ID			0xFF35			//下拉刷新
#define	SLEEP_DETAILS_ID		0xFF36			//睡眠详情
#define	FIND_DEVICE_ID			0xFF37			//查找手环
#define SMART_NOTIFY_ID			0xFF38			//智能提醒
#define	ALARM_SETTING_ID		0xFF39			//闹钟设置
#define USER_INFOR_ID			0xFF40			//用户信息
#define	SEDENTARY_ID			0xFF41			//久坐提醒
#define	SHAKE_SCREEN_ID			0xFF42			//抬手亮屏
#define	MEASURE_HOURLY_ID		0xFF43			//整点测量设置
#define	SHAKE_PHOTO_ID			0xFF44			//摇一摇拍照
#define	LANGUAGE_SETTING_ID		0xFF45			//中英日文切换
#define	TIME_24_SETTING_ID		0xFF46			//12/24小时设置
#define	FIND_PHONE_ID			0xFF47			//查找手机回复
#define	WEATHER_INFOR_ID		0xFF48			//天气信息下发
#define	TIME_SYNC_ID			0xFF49			//时间同步
#define	TARGET_STEPS_ID			0xFF50			//目标步数
#define	BATTERY_LEVEL_ID		0xFF51			//电池电量
#define	FIRMWARE_INFOR_ID		0xFF52			//固件版本号
#define	FACTORY_RESET_ID		0xFF53			//清除手环数据
#define	ECG_ID					0xFF54			//心电
#define	LOCATION_ID				0xFF55			//获取定位信息
#define	DATE_FORMAT_ID			0xFF56			//年月日格式设置
#define NOTIFY_CONTENT_ID		0xFF57			//智能提醒内容
#define CHECK_WHITELIST_ID		0xFF58			//判断手机ID是否在手环白名单
#define INSERT_WHITELIST_ID`	0xFF59			//将手机ID插入白名单
#define DEVICE_SEND_128_RAND_ID	0xFF60			//手环发送随机的128位随机数
#define PHONE_SEND_128_AES_ID	0xFF61			//手机发送AES 128 CBC加密数据给手环
#define WIFI_SCAN_DATA_ID		0xFF62			//扫描到的wifi信号数据

#define	BLE_CONNECT_ID			0xFFB0			//BLE断连提醒
#define	CTP_NOTIFY_ID			0xFFB1			//CTP触屏消息
#define GET_NRF52810_VER_ID		0xFFB2			//获取52810版本号
#define GET_BLE_MAC_ADDR_ID		0xFFB3			//获取BLE MAC地址
#define GET_BLE_STATUS_ID		0xFFB4			//获取BLE当前工作状态	0:关闭 1:休眠 2:广播 3:连接
#define SET_BEL_WORK_MODE_ID	0xFFB5			//设置BLE工作模式		0:关闭 1:打开 2:唤醒 3:休眠

static u32_t rece_len=0;

static u8_t rx_buf[BUF_MAXSIZE]={0};
static u8_t tx_buf[BUF_MAXSIZE]={0};

static struct device *uart_ble;

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

struct uart_data_t
{
	void  *fifo_reserved;
	u8_t    data[BUF_MAXSIZE];
	u16_t   len;
};

bool BLE_is_connected = false;

u8_t ble_mac_addr[6] = {0};
u8_t str_nrf52810_ver[128] = {0};

ENUM_BLE_STATUS g_ble_status = BLE_STATUS_BROADCAST;
ENUM_BLE_MODE g_ble_mode = BLE_MODE_TURN_OFF;

extern bool app_find_device;

static void MCU_send_heart_rate(void);

void ble_connect_or_disconnect_handle(u8_t *buf, u32_t len)
{
	LOG_INF("BLE status:%x\n", buf[6]);
	
	if(buf[6] == 0x01)				//查看control值
		BLE_is_connected = true;
	else if(buf[6] == 0x00)
		BLE_is_connected = false;
	else
		BLE_is_connected = false;
}

void CTP_notify_handle(u8_t *buf, u32_t len)
{
	u8_t tmpbuf[128] = {0};
	u8_t tp_type = TP_EVENT_MAX;
	u16_t tp_x,tp_y;
	
	LOG_INF("%x,%x,%x,%x,%x,%x\n",buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]);
	switch(buf[5])
	{
	case GESTURE_NONE:
		sprintf(tmpbuf, "GESTURE_NONE        ");
		break;
	case GESTURE_MOVING_UP:
		tp_type = TP_EVENT_MOVING_UP;
		sprintf(tmpbuf, "MOVING_UP   ");
		break;
	case GESTURE_MOVING_DOWN:
		tp_type = TP_EVENT_MOVING_DOWN;
		sprintf(tmpbuf, "MOVING_DOWN ");
		break;
	case GESTURE_MOVING_LEFT:
		tp_type = TP_EVENT_MOVING_LEFT;
		sprintf(tmpbuf, "MOVING_LEFT ");
		break;
	case GESTURE_MOVING_RIGHT:
		tp_type = TP_EVENT_MOVING_RIGHT;
		sprintf(tmpbuf, "MOVING_RIGHT");
		break;
	case GESTURE_SINGLE_CLICK:
		tp_type = TP_EVENT_SINGLE_CLICK;
		sprintf(tmpbuf, "SINGLE_CLICK");
		break;
	case GESTURE_DOUBLE_CLICK:
		tp_type = TP_EVENT_DOUBLE_CLICK;
		sprintf(tmpbuf, "DOUBLE_CLICK");
		break;
	case GESTURE_LONG_PRESS:
		tp_type = TP_EVENT_LONG_PRESS;
		sprintf(tmpbuf, "LONG_PRESS  ");
		break;
	}

	if(tp_type != TP_EVENT_MAX)
	{
		tp_x = buf[7]*0x100+buf[8];
		tp_y = buf[9]*0x100+buf[10];
		touch_panel_event_handle(tp_type, tp_x, tp_y);
	}
}

void wifi_sacn_notify_handle(u8_t *buf, u32_t len)
{
	u8_t tmpbuf[128] = {0};
	u8_t tp_type = TP_EVENT_MAX;
	u16_t tp_x,tp_y;
	
	LOG_INF("%x,%x,%x,%x,%x,%x\n",buf[5],buf[6],buf[7],buf[8],buf[9],buf[10]);
	
}

void APP_set_find_device(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (FIND_DEVICE_ID>>8);		
	reply[reply_len++] = (u8_t)(FIND_DEVICE_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	app_find_device = true;	
}

void APP_set_language(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	
	if(buf[7] == 0x00)
		global_settings.language = LANGUAGE_CHN;
	else if(buf[7] == 0x01)
		global_settings.language = LANGUAGE_EN;
	else if(buf[7] == 0x02)
		global_settings.language = LANGUAGE_JPN;
	else
		global_settings.language = LANGUAGE_EN;
		
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (LANGUAGE_SETTING_ID>>8);		
	reply[reply_len++] = (u8_t)(LANGUAGE_SETTING_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_WEEK;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
	
	need_save_settings = true;
}

void APP_set_time_24_format(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	
	if(buf[7] == 0x00)
		global_settings.time_format = TIME_FORMAT_24;//24 format
	else if(buf[7] == 0x01)
		global_settings.time_format = TIME_FORMAT_12;//12 format
	else
		global_settings.time_format = TIME_FORMAT_24;//24 format

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (TIME_24_SETTING_ID>>8);
	reply[reply_len++] = (u8_t)(TIME_24_SETTING_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_TIME;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
	
	need_save_settings = true;	
}


void APP_set_date_format(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	
	if(buf[7] == 0x00)
		global_settings.date_format = DATE_FORMAT_YYYYMMDD;
	else if(buf[7] == 0x01)
		global_settings.date_format = DATE_FORMAT_MMDDYYYY;
	else if(buf[7] == 0x02)
		global_settings.date_format = DATE_FORMAT_DDMMYYYY;
	else
		global_settings.date_format = DATE_FORMAT_YYYYMMDD;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (DATE_FORMAT_ID>>8);		
	reply[reply_len++] = (u8_t)(DATE_FORMAT_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_DATE;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}

	need_save_settings = true;
}

void APP_set_date_time(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	sys_date_timer_t datetime = {0};
	
	datetime.year = 256*buf[7]+buf[8];
	datetime.month = buf[9];
	datetime.day = buf[10];
	datetime.hour = buf[11];
	datetime.minute = buf[12];
	datetime.second = buf[13];
	
	if(CheckSystemDateTimeIsValid(datetime))
	{
		datetime.week = GetWeekDayByDate(datetime);
		memcpy(&date_time, &datetime, sizeof(sys_date_timer_t));

		if(screen_id == SCREEN_ID_IDLE)
		{
			scr_msg[screen_id].para |= (SCREEN_EVENT_UPDATE_TIME|SCREEN_EVENT_UPDATE_DATE|SCREEN_EVENT_UPDATE_WEEK);
			scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
		}
		need_save_time = true;
	}

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (TIME_SYNC_ID>>8);		
	reply[reply_len++] = (u8_t)(TIME_SYNC_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);	
}

void APP_set_alarm(u8_t *buf, u32_t len)
{
	u8_t result=0,reply[128] = {0};
	u32_t i,index,reply_len = 0;
	alarm_infor_t infor = {0};

	index = buf[7];
	if(index <= 7)
	{
		infor.is_on = buf[8];	//on\off
		infor.hour = buf[9];	//hour
		infor.minute = buf[10];//minute
		infor.repeat = buf[11];//repeat from monday to sunday, for example:0x1111100 means repeat in workday

		if((buf[9]<=23)&&(buf[10]<=59)&&(buf[11]<=0x7f))
		{
			result = 0x80;
			memcpy((alarm_infor_t*)&global_settings.alarm[index], (alarm_infor_t*)&infor, sizeof(alarm_infor_t));
			need_save_settings = true;
		}
	}
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (ALARM_SETTING_ID>>8);		
	reply[reply_len++] = (u8_t)(ALARM_SETTING_ID&0x00ff);
	//status
	reply[reply_len++] = result;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);	
}

void APP_set_PHD_interval(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	
	if(buf[6] == 1)
		global_settings.phd_infor.is_on = true;
	else
		global_settings.phd_infor.is_on = false;

	global_settings.phd_infor.interval = buf[7];
	need_save_settings = true;
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (MEASURE_HOURLY_ID>>8);		
	reply[reply_len++] = (u8_t)(MEASURE_HOURLY_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	need_save_settings = true;	
}

void APP_set_wake_screen_by_wrist(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	
	if(buf[6] == 1)
		global_settings.wake_screen_by_wrist = true;
	else
		global_settings.wake_screen_by_wrist = false;
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (SHAKE_SCREEN_ID>>8);		
	reply[reply_len++] = (u8_t)(SHAKE_SCREEN_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	need_save_settings = true;
}

void APP_set_factory_reset(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (FACTORY_RESET_ID>>8);		
	reply[reply_len++] = (u8_t)(FACTORY_RESET_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	need_reset_settings = true;
}

void APP_set_target_steps(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	LOG_INF("APP_set_target_steps: %02X,%02X\n", buf[7], buf[8]);

	global_settings.target_steps = buf[7]*100+buf[8];
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (TARGET_STEPS_ID>>8);		
	reply[reply_len++] = (u8_t)(TARGET_STEPS_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	need_save_settings = true;
}

void APP_get_current_data(u8_t *buf, u32_t len)
{
	u8_t wake,reply[128] = {0};
	u16_t steps,calorie,distance,light_sleep,deep_sleep;	
	u32_t i,reply_len = 0;

	GetSportData(&steps, &calorie, &distance);
	GetSleepTimeData(&deep_sleep, &light_sleep);
	wake = 8;
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x11;
	//data ID
	reply[reply_len++] = (PULL_REFRESH_ID>>8);		
	reply[reply_len++] = (u8_t)(PULL_REFRESH_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//Step
	reply[reply_len++] = (steps>>8);
	reply[reply_len++] = (u8_t)(steps&0x00ff);
	//Calorie
	reply[reply_len++] = (calorie>>8);
	reply[reply_len++] = (u8_t)(calorie&0x00ff);
	//Distance
	reply[reply_len++] = (distance>>8);
	reply[reply_len++] = (u8_t)(distance&0x00ff);
	//Shallow Sleep
	reply[reply_len++] = (light_sleep>>8);
	reply[reply_len++] = (u8_t)(light_sleep&0x00ff);
	//Deep Sleep
	reply[reply_len++] = (deep_sleep>>8);
	reply[reply_len++] = (u8_t)(deep_sleep&0x00ff);
	//Wake
	reply[reply_len++] = wake;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	//上传心率数据
	MCU_send_heart_rate();
}

void APP_get_location_data(u8_t *buf, u32_t len)
{
	ble_wait_gps = true;
	APP_Ask_GPS_Data();
}

void APP_get_location_data_reply(nrf_gnss_pvt_data_frame_t gps_data)
{
	u8_t tmpgps;
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;
	u32_t tmp1;
	double tmp2;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x17;
	//data ID
	reply[reply_len++] = (LOCATION_ID>>8);		
	reply[reply_len++] = (u8_t)(LOCATION_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	
	//UTC data&time
	//year
	reply[reply_len++] = gps_data.datetime.year>>8;
	reply[reply_len++] = (u8_t)(gps_data.datetime.year&0x00FF);
	//month
	reply[reply_len++] = gps_data.datetime.month;
	//day
	reply[reply_len++] = gps_data.datetime.day;
	//hour
	reply[reply_len++] = gps_data.datetime.hour;
	//minute
	reply[reply_len++] = gps_data.datetime.minute;
	//seconds
	reply[reply_len++] = gps_data.datetime.seconds;
	
	//longitude
	tmpgps = 'E';
	if(gps_data.longitude < 0)
	{
		tmpgps = 'W';
		gps_data.longitude = -gps_data.longitude;
	}
	//direction	E\W
	reply[reply_len++] = tmpgps;
	tmp1 = (u32_t)(gps_data.longitude); //经度整数部分
	tmp2 = gps_data.longitude - tmp1;	//经度小数部分
	//degree int
	reply[reply_len++] = tmp1;//整数部分
	tmp1 = (u32_t)(tmp2*1000000);
	//degree dot1~2
	reply[reply_len++] = (u8_t)(tmp1/10000);
	tmp1 = tmp1%10000;
	//degree dot3~4
	reply[reply_len++] = (u8_t)(tmp1/100);
	tmp1 = tmp1%100;
	//degree dot5~6
	reply[reply_len++] = (u8_t)(tmp1);	
	//latitude
	tmpgps = 'N';
	if(gps_data.latitude < 0)
	{
		tmpgps = 'S';
		gps_data.latitude = -gps_data.latitude;
	}
	//direction N\S
	reply[reply_len++] = tmpgps;
	tmp1 = (u32_t)(gps_data.latitude);	//经度整数部分
	tmp2 = gps_data.latitude - tmp1;	//经度小数部分
	//degree int
	reply[reply_len++] = tmp1;//整数部分
	tmp1 = (u32_t)(tmp2*1000000);
	//degree dot1~2
	reply[reply_len++] = (u8_t)(tmp1/10000);
	tmp1 = tmp1%10000;
	//degree dot3~4
	reply[reply_len++] = (u8_t)(tmp1/100);
	tmp1 = tmp1%100;
	//degree dot5~6
	reply[reply_len++] = (u8_t)(tmp1);
					
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

void APP_get_battery_level(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (BATTERY_LEVEL_ID>>8);		
	reply[reply_len++] = (u8_t)(BATTERY_LEVEL_ID&0x00ff);
	//status
	switch(g_chg_status)
	{
	case BAT_CHARGING_NO:
		reply[reply_len++] = 0x00;
		break;
		
	case BAT_CHARGING_PROGRESS:
	case BAT_CHARGING_FINISHED:
		reply[reply_len++] = 0x01;
		break;
	}
	//control
	reply[reply_len++] = 0x00;
	//bat level
	reply[reply_len++] = g_bat_soc;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

void APP_get_firmware_version(u8_t *buf, u32_t len)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (FIRMWARE_INFOR_ID>>8);		
	reply[reply_len++] = (u8_t)(FIRMWARE_INFOR_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//fm ver
	reply[reply_len++] = (0x02<<4)+0x00;	//V2.0
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

void APP_get_heart_rate(u8_t *buf, u32_t len)
{
	u8_t heart_rate,reply[128] = {0};
	u32_t i,reply_len = 0;

	switch(buf[5])
	{
	case 0x01://实时测量心率
		break;
	case 0x02://单词测量心率
		break;
	}

	switch(buf[6])
	{
	case 0://关闭传感器
		break;
	case 1://打开传感器
		break;
	}
	
	GetHeartRate(&heart_rate);
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (HEART_RATE_ID>>8);		
	reply[reply_len++] = (u8_t)(HEART_RATE_ID&0x00ff);
	//status
	reply[reply_len++] = 0x02;
	//control
	reply[reply_len++] = 0x01;
	//heart rate
	reply[reply_len++] = heart_rate;	//V2.0
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

//APP回复手环查找手机
void APP_reply_find_phone(u8_t *buf, u32_t len)
{
	u32_t i;

	LOG_INF("APP_reply_find_phone\n");
	
	for(i=0;i<len;i++)
	{
		LOG_INF("i:%d, data:%02X\n", i, buf[i]);
	}
}

void MCU_get_nrf52810_ver(void)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (GET_NRF52810_VER_ID>>8);		
	reply[reply_len++] = (u8_t)(GET_NRF52810_VER_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

void MCU_get_ble_mac_address(void)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (GET_BLE_MAC_ADDR_ID>>8);		
	reply[reply_len++] = (u8_t)(GET_BLE_MAC_ADDR_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

void MCU_get_ble_status(void)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (GET_BLE_STATUS_ID>>8);		
	reply[reply_len++] = (u8_t)(GET_BLE_STATUS_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

//设置BLE工作模式		0:关闭 1:打开 2:唤醒 3:休眠
void MCU_set_ble_work_mode(u8_t work_mode)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (SET_BEL_WORK_MODE_ID>>8);		
	reply[reply_len++] = (u8_t)(SET_BEL_WORK_MODE_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = work_mode;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);	
}

//手环查找手机
void MCU_send_find_phone(void)
{
	u8_t reply[128] = {0};
	u32_t i,reply_len = 0;

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (FIND_PHONE_ID>>8);		
	reply[reply_len++] = (u8_t)(FIND_PHONE_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//data
	reply[reply_len++] = 0x01;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);	

}

//手环上报整点心率数据
void MCU_send_heart_rate(void)
{
	u8_t heart_rate,reply[128] = {0};
	u32_t i,reply_len = 0;

	GetHeartRate(&heart_rate);
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x0D;
	//data ID
	reply[reply_len++] = (PULL_REFRESH_ID>>8);		
	reply[reply_len++] = (u8_t)(PULL_REFRESH_ID&0x00ff);
	//status
	reply[reply_len++] = 0x81;
	//control
	reply[reply_len++] = 0x00;
	//year
	reply[reply_len++] = (u8_t)(date_time.year>>8);
	reply[reply_len++] = (u8_t)(date_time.year&0x00ff);
	//month
	reply[reply_len++] = date_time.month;
	//day
	reply[reply_len++] = date_time.day;
	//hour
	reply[reply_len++] = date_time.hour;
	//minute
	reply[reply_len++] = date_time.minute;
	//data
	reply[reply_len++] = heart_rate;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);	

}

void get_nrf52810_ver_response(u8_t *buf, u32_t len)
{
	u32_t i;

	for(i=0;i<len-9;i++)
	{
		str_nrf52810_ver[i] = buf[7+i];
	}

	LOG_INF("str_nrf52810_ver:%s\n", str_nrf52810_ver);
}

void get_ble_mac_address_response(u8_t *buf, u32_t len)
{
	u32_t i;

	for(i=0;i<6;i++)
	{
		ble_mac_addr[i] = buf[7+i];
	}

	LOG_INF("ble_mac_addr %02X:%02X:%02X:%02X:%02X:%02X\n",
							ble_mac_addr[0],
							ble_mac_addr[1],
							ble_mac_addr[2],
							ble_mac_addr[3],
							ble_mac_addr[4],
							ble_mac_addr[5]
							);
}

void get_ble_status_response(u8_t *buf, u32_t len)
{
	LOG_INF("BLE_status:%d\n", buf[6]);

	g_ble_status = buf[6];
}

/**********************************************************************************
*Name: ble_receive_date_handle
*Function:  处理蓝牙接收到的数据
*Parameter: 
*			Input:
*				buf 接收到的数据
*				len 接收到的数据长度
*			Output:
*				none
*			Return:
*				void
*Description:
*	接收到的数据包的格式如下:
*	包头			数据长度		操作		状态类型	控制		数据1	数据…		校验		包尾
*	(StarFrame)		(Data length)	(ID)		(Status)	(Control)	(Data1)	(Data…)	(CRC8)		(EndFrame)
*	(1 bytes)		(2 byte)		(2 byte)	(1 byte)	(1 byte)	(可选)	(可选)		(1 bytes)	(1 bytes)
*
*	例子如下表所示：
*	Offset	Field		Size	Value(十六进制)		Description
*	0		StarFrame	1		0xAB				起始帧
*	1		Data length	2		0x0000-0xFFFF		数据长度,从ID开始一直到包尾
*	3		Data ID		2		0x0000-0xFFFF	    ID
*	5		Status		1		0x00-0xFF	        Status
*	6		Control		1		0x00-0x01			控制
*	7		Data0		1-14	0x00-0xFF			数据0
*	8+n		CRC8		1		0x00-0xFF			数据校验,从包头开始到CRC前一位
*	9+n		EndFrame	1		0x88				结束帧
**********************************************************************************/
void ble_receive_date_handle(u8_t *buf, u32_t len)
{
	u8_t CRC_data=0,data_status;
	u16_t data_len,data_ID;
	u32_t i;

	if((buf[0] != PACKET_HEAD) || (buf[len-1] != PACKET_END))	//format is error
	{
		LOG_INF("format is error! HEAD:%x, END:%x\n", buf[0], buf[len-1]);
		return;
	}

	for(i=0;i<len-2;i++)
		CRC_data = CRC_data+buf[i];

	if(CRC_data != buf[len-2])									//crc is error
	{
		LOG_INF("CRC is error! data:%x, CRC:%x\n", buf[len-2], CRC_data);
		return;
	}

	data_len = buf[1]*256+buf[2];
	data_ID = buf[3]*256+buf[4];

	switch(data_ID)
	{
	case HEART_RATE_ID:			//心率
		APP_get_heart_rate(buf, len);
		break;
	case BLOOD_OXYGEN_ID:		//血氧
		break;
	case BLOOD_PRESSURE_ID:		//血压
		break;
	case ONE_KEY_MEASURE_ID:	//一键测量
		break;
	case PULL_REFRESH_ID:		//下拉刷新
		APP_get_current_data(buf, len);
		break;
	case SLEEP_DETAILS_ID:		//睡眠详情
		break;
	case FIND_DEVICE_ID:		//查找手环
		APP_set_find_device(buf, len);
		break;
	case SMART_NOTIFY_ID:		//智能提醒
		break;
	case ALARM_SETTING_ID:		//闹钟设置
		APP_set_alarm(buf, len);
		break;
	case USER_INFOR_ID:			//用户信息
		break;
	case SEDENTARY_ID:			//久坐提醒
		break;
	case SHAKE_SCREEN_ID:		//抬手亮屏
		APP_set_wake_screen_by_wrist(buf, len);
		break;
	case MEASURE_HOURLY_ID:		//整点测量设置
		APP_set_PHD_interval(buf, len);
		break;
	case SHAKE_PHOTO_ID:		//摇一摇拍照
		break;
	case LANGUAGE_SETTING_ID:	//中英日文切换
		APP_set_language(buf, len);
		break;
	case TIME_24_SETTING_ID:	//12/24小时设置
		APP_set_time_24_format(buf, len);
		break;
	case FIND_PHONE_ID:			//查找手机回复
		APP_reply_find_phone(buf, len);
		break;
	case WEATHER_INFOR_ID:		//天气信息下发
		break;
	case TIME_SYNC_ID:			//时间同步
		APP_set_date_time(buf, len);
		break;
	case TARGET_STEPS_ID:		//目标步数
		APP_set_target_steps(buf, len);
		break;
	case BATTERY_LEVEL_ID:		//电池电量
		APP_get_battery_level(buf, len);
		break;
	case FIRMWARE_INFOR_ID:		//固件版本号
		APP_get_firmware_version(buf, len);
		break;
	case FACTORY_RESET_ID:		//清除手环数据
		APP_set_factory_reset(buf, len);
		break;
	case ECG_ID:				//心电
		break;
	case LOCATION_ID:			//获取定位信息
		APP_get_location_data(buf, len);
		break;
	case DATE_FORMAT_ID:		//年月日格式
		APP_set_date_format(buf, len);
		break;
	case BLE_CONNECT_ID:		//BLE断连提醒
		ble_connect_or_disconnect_handle(buf, len);
		break;
	case CTP_NOTIFY_ID:
		CTP_notify_handle(buf, len);
		break;
	case WIFI_SCAN_DATA_ID:
		wifi_sacn_notify_handle(buf, len);
		break;
	case GET_NRF52810_VER_ID:
		get_nrf52810_ver_response(buf, len);
		break;
	case GET_BLE_MAC_ADDR_ID:
		get_ble_mac_address_response(buf, len);
		break;
	case GET_BLE_STATUS_ID:
		get_ble_status_response(buf, len);
		break;
	case SET_BEL_WORK_MODE_ID:
		break;
	default:
		LOG_INF("data_id is unknown! \n");
		break;
	}
}

void ble_send_date_handle(u8_t *buf, u32_t len)
{
	LOG_INF("ble_send_date_handle\n");

	uart_fifo_fill(uart_ble, buf, len);
	uart_irq_tx_enable(uart_ble); 
}

static void uart_receive_data(u8_t data, u32_t datalen)
{
	LOG_INF("uart_rece:%02X\n", data);
	
	if(data == 0xAB)
	{
		memset(rx_buf, 0, sizeof(rx_buf));
		rece_len = 0;
	}
	
	rx_buf[rece_len++] = data;
	if(rece_len == (256*rx_buf[1]+rx_buf[2]+3))	//receivive complete
	{
		ble_receive_date_handle(rx_buf, rece_len);

		memset(rx_buf, 0, sizeof(rx_buf));
		rece_len = 0;
	}
	else				//continue receive
	{
	}
}

void uart_send_data(void)
{
	LOG_INF("uart_send_data\n");
	
	uart_fifo_fill(uart_ble, "Hello World!", strlen("Hello World!"));
	uart_irq_tx_enable(uart_ble); 
}

static void uart_cb(struct device *x)
{
	u8_t tmpbyte = 0;
	u32_t len=0;

	uart_irq_update(x);

	if(uart_irq_rx_ready(x)) 
	{
		while((len = uart_fifo_read(x, &tmpbyte, 1)) > 0)
			uart_receive_data(tmpbyte, 1);
	}
	
	if(uart_irq_tx_ready(x))
	{
		struct uart_data_t *buf;
		u16_t written = 0;

		buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		/* Nothing in the FIFO, nothing to send */
		if(!buf)
		{
			uart_irq_tx_disable(x);
			return;
		}

		while(buf->len > written)
		{
			written += uart_fifo_fill(x, &buf->data[written], buf->len - written);
		}

		while (!uart_irq_tx_complete(x))
		{
			/* Wait for the last byte to get
			* shifted out of the module
			*/
		}

		if (k_fifo_is_empty(&fifo_uart_tx_data))
		{
			uart_irq_tx_disable(x);
		}

		k_free(buf);
	}
}

void ble_init(void)
{
	LOG_INF("ble_init\n");
	
	uart_ble = device_get_binding(BLE_DEV);
	if(!uart_ble)
	{
		LOG_INF("Could not get %s device\n", BLE_DEV);
		return;
	}

	uart_irq_callback_set(uart_ble, uart_cb);
	uart_irq_rx_enable(uart_ble);
}

void test_uart_ble(void)
{
	LOG_INF("test_uart_ble\n");
	
	//ble_init();

	while(1)
	{
		ble_send_date_handle("Hello World!", strlen("Hello World!"));
		k_sleep(K_MSEC(1000));
	}
}

