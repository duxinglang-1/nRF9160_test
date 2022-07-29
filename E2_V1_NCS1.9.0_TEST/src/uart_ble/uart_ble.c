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
#include <drivers/gpio.h>

#include "logger.h"
#include "datetime.h"
#include "Settings.h"
#include "Uart_ble.h"
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
//#include "gps.h"
#include "max20353.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif
#include "screen.h"
#include "inner_flash.h"
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif

//#define UART_DEBUG

#define BLE_DEV			"UART_0"
#define BLE_PORT		"GPIO_0"
#define BLE_INT_PIN		27
#define BLE_WAKE_PIN	25

#define BUF_MAXSIZE	1024

#define PACKET_HEAD	0xAB
#define PACKET_END	0x88

#define BLE_WORK_MODE_ID		0xFF10			//52810����״̬����
#define HEART_RATE_ID			0xFF31			//����
#define BLOOD_OXYGEN_ID			0xFF32			//Ѫ��
#define BLOOD_PRESSURE_ID		0xFF33			//Ѫѹ
#define	ONE_KEY_MEASURE_ID		0xFF34			//һ������
#define	PULL_REFRESH_ID			0xFF35			//����ˢ��
#define	SLEEP_DETAILS_ID		0xFF36			//˯������
#define	FIND_DEVICE_ID			0xFF37			//�����ֻ�
#define SMART_NOTIFY_ID			0xFF38			//��������
#define	ALARM_SETTING_ID		0xFF39			//��������
#define USER_INFOR_ID			0xFF40			//�û���Ϣ
#define	SEDENTARY_ID			0xFF41			//��������
#define	SHAKE_SCREEN_ID			0xFF42			//̧������
#define	MEASURE_HOURLY_ID		0xFF43			//�����������
#define	SHAKE_PHOTO_ID			0xFF44			//ҡһҡ����
#define	LANGUAGE_SETTING_ID		0xFF45			//��Ӣ�����л�
#define	TIME_24_SETTING_ID		0xFF46			//12/24Сʱ����
#define	FIND_PHONE_ID			0xFF47			//�����ֻ��ظ�
#define	WEATHER_INFOR_ID		0xFF48			//������Ϣ�·�
#define	TIME_SYNC_ID			0xFF49			//ʱ��ͬ��
#define	TARGET_STEPS_ID			0xFF50			//Ŀ�경��
#define	BATTERY_LEVEL_ID		0xFF51			//��ص���
#define	FIRMWARE_INFOR_ID		0xFF52			//�̼��汾��
#define	FACTORY_RESET_ID		0xFF53			//����ֻ�����
#define	ECG_ID					0xFF54			//�ĵ�
#define	LOCATION_ID				0xFF55			//��ȡ��λ��Ϣ
#define	DATE_FORMAT_ID			0xFF56			//�����ո�ʽ����
#define NOTIFY_CONTENT_ID		0xFF57			//������������
#define CHECK_WHITELIST_ID		0xFF58			//�ж��ֻ�ID�Ƿ����ֻ�������
#define INSERT_WHITELIST_ID`	0xFF59			//���ֻ�ID���������
#define DEVICE_SEND_128_RAND_ID	0xFF60			//�ֻ����������28λ�����
#define PHONE_SEND_128_AES_ID	0xFF61			//�ֻ�����AES 128 CBC�������ݸ��ֻ�

#define	BLE_CONNECT_ID			0xFFB0			//BLE��������
#define	CTP_NOTIFY_ID			0xFFB1			//CTP������Ϣ
#define GET_NRF52810_VER_ID		0xFFB2			//��ȡ52810�汾��
#define GET_BLE_MAC_ADDR_ID		0xFFB3			//��ȡBLE MAC��ַ
#define GET_BLE_STATUS_ID		0xFFB4			//��ȡBLE��ǰ����״̬	0:�ر� 1:���� 2:�㲥 3:����
#define SET_BEL_WORK_MODE_ID	0xFFB5			//����BLE����ģʽ		0:�ر� 1:�� 2:���� 3:����

bool blue_is_on = true;
bool uart_send_flag = false;
bool get_ble_info_flag = false;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
bool uart_sleep_flag = false;
bool uart_wake_flag = false;
bool uart_is_waked = true;
#define UART_WAKE_HOLD_TIME_SEC		(10)
#define UART_SLEEP_DELAY_TIME_SEC	(10)
#endif

static bool redraw_blt_status_flag = false;

static ENUM_DATA_TYPE uart_data_type=DATA_TYPE_BLE;
static ENUM_BLE_WORK_MODE ble_work_mode=BLE_WORK_NORMAL;

static uint32_t rece_len=0;
static uint32_t send_len=0;
static uint8_t rx_buf[BUF_MAXSIZE]={0};
static uint8_t tx_buf[BUF_MAXSIZE]={0};

static struct device *uart_ble = NULL;
static struct device *gpio_ble = NULL;
static struct gpio_callback gpio_cb;

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static sys_date_timer_t refresh_time = {0};

struct uart_data_t
{
	void  *fifo_reserved;
	uint8_t    data[BUF_MAXSIZE];
	uint16_t   len;
};

bool g_ble_connected = false;

uint8_t g_ble_mac_addr[20] = {0};
uint8_t g_nrf52810_ver[128] = {0};

ENUM_BLE_STATUS g_ble_status = BLE_STATUS_BROADCAST;
ENUM_BLE_MODE g_ble_mode = BLE_MODE_TURN_OFF;

extern bool app_find_device;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void UartSleepInCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(uart_sleep_in_timer, UartSleepInCallBack, NULL);
#endif
static void GetBLEInfoCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(get_ble_info_timer, GetBLEInfoCallBack, NULL);

static void MCU_send_heart_rate(void);

void ble_connect_or_disconnect_handle(uint8_t *buf, uint32_t len)
{
#ifdef UART_DEBUG
	LOGD("BLE status:%x", buf[6]);
#endif

	if(buf[6] == 0x01)				//�鿴controlֵ
		g_ble_connected = true;
	else if(buf[6] == 0x00)
		g_ble_connected = false;
	else
		g_ble_connected = false;

	redraw_blt_status_flag = true;
}

#ifdef CONFIG_TOUCH_SUPPORT
void CTP_notify_handle(uint8_t *buf, uint32_t len)
{
	uint8_t tp_type = TP_EVENT_MAX;
	uint16_t tp_x,tp_y;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	switch(buf[5])
	{
	case GESTURE_NONE:
		tp_type = TP_EVENT_NONE;
		break;
	case GESTURE_MOVING_UP:
		tp_type = TP_EVENT_MOVING_UP;
		break;
	case GESTURE_MOVING_DOWN:
		tp_type = TP_EVENT_MOVING_DOWN;
		break;
	case GESTURE_MOVING_LEFT:
		tp_type = TP_EVENT_MOVING_LEFT;
		break;
	case GESTURE_MOVING_RIGHT:
		tp_type = TP_EVENT_MOVING_RIGHT;
		break;
	case GESTURE_SINGLE_CLICK:
		tp_type = TP_EVENT_SINGLE_CLICK;
		break;
	case GESTURE_DOUBLE_CLICK:
		tp_type = TP_EVENT_DOUBLE_CLICK;
		break;
	case GESTURE_LONG_PRESS:
		tp_type = TP_EVENT_LONG_PRESS;
		break;
	}

	if(tp_type != TP_EVENT_MAX)
	{
		tp_x = (0x0f&buf[7])<<8 | buf[8];
		tp_y = (0x0f&buf[9])<<8 | buf[10];
		touch_panel_event_handle(tp_type, tp_x, tp_y);
	}
}
#endif

void APP_set_find_device(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG	
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (FIND_DEVICE_ID>>8);		
	reply[reply_len++] = (uint8_t)(FIND_DEVICE_ID&0x00ff);
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

void APP_set_language(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	if(buf[7] == 0x00)
		global_settings.language = LANGUAGE_CHN;
	else if(buf[7] == 0x01)
		global_settings.language = LANGUAGE_EN;
	else if(buf[7] == 0x02)
		global_settings.language = LANGUAGE_DE;
	else
		global_settings.language = LANGUAGE_EN;
		
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (LANGUAGE_SETTING_ID>>8);		
	reply[reply_len++] = (uint8_t)(LANGUAGE_SETTING_ID&0x00ff);
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

void APP_set_time_24_format(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

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
	reply[reply_len++] = (uint8_t)(TIME_24_SETTING_ID&0x00ff);
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


void APP_set_date_format(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

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
	reply[reply_len++] = (uint8_t)(DATE_FORMAT_ID&0x00ff);
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

void APP_set_date_time(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;
	sys_date_timer_t datetime = {0};

#ifdef UART_DEBUG
	LOGD("begin");
#endif

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
	reply[reply_len++] = (uint8_t)(TIME_SYNC_ID&0x00ff);
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

void APP_set_alarm(uint8_t *buf, uint32_t len)
{
	uint8_t result=0,reply[128] = {0};
	uint32_t i,index,reply_len = 0;
	alarm_infor_t infor = {0};

#ifdef UART_DEBUG
	LOGD("begin");
#endif

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
	reply[reply_len++] = (uint8_t)(ALARM_SETTING_ID&0x00ff);
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

void APP_set_PHD_interval(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("flag:%d, interval:%d", buf[6], buf[7]);
#endif

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
	reply[reply_len++] = (uint8_t)(MEASURE_HOURLY_ID&0x00ff);
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

void APP_set_wake_screen_by_wrist(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("flag:%d", buf[6]);
#endif

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
	reply[reply_len++] = (uint8_t)(SHAKE_SCREEN_ID&0x00ff);
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

void APP_set_factory_reset(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (FACTORY_RESET_ID>>8);		
	reply[reply_len++] = (uint8_t)(FACTORY_RESET_ID&0x00ff);
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

void APP_set_target_steps(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("steps:%d", buf[7]*100+buf[8]);
#endif

	global_settings.target_steps = buf[7]*100+buf[8];
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (TARGET_STEPS_ID>>8);		
	reply[reply_len++] = (uint8_t)(TARGET_STEPS_ID&0x00ff);
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

#ifdef CONFIG_PPG_SUPPORT
void APP_get_one_key_measure_data(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("setting:%d", buf[6]);
#endif

	if(buf[6] == 1)//����
	{
		g_ppg_trigger |= TRIGGER_BY_APP_ONE_KEY; 
		APPStartHrSpo2();
	}
	else
	{
		//packet head
		reply[reply_len++] = PACKET_HEAD;
		//data_len
		reply[reply_len++] = 0x00;
		reply[reply_len++] = 0x0A;
		//data ID
		reply[reply_len++] = (ONE_KEY_MEASURE_ID>>8);		
		reply[reply_len++] = (uint8_t)(ONE_KEY_MEASURE_ID&0x00ff);
		//status
		reply[reply_len++] = 0x80;
		//control
		reply[reply_len++] = 0x00;
		//data hr
		reply[reply_len++] = 0x00;
		//data spo2
		reply[reply_len++] = 0x00;
		//data systolic
		reply[reply_len++] = 0x00;
		//data diastolic
		reply[reply_len++] = 0x00;
		//CRC
		reply[reply_len++] = 0x00;
		//packet end
		reply[reply_len++] = PACKET_END;

		for(i=0;i<(reply_len-2);i++)
			reply[reply_len-2] += reply[i];

		ble_send_date_handle(reply, reply_len);
	}
}
#endif

void APP_get_current_data(uint8_t *buf, uint32_t len)
{
	uint8_t wake,reply[128] = {0};
	uint16_t steps=0,calorie=0,distance=0;
	uint16_t light_sleep=0,deep_sleep=0;	
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("%04d/%02d/%02d %02d:%02d", 2000 + buf[7],buf[8],buf[9],buf[10],buf[11]);
#endif

	refresh_time.year = 2000 + buf[7];
	refresh_time.month = buf[8];
	refresh_time.day = buf[9];
	refresh_time.hour = buf[10];
	refresh_time.minute = buf[11];

#ifdef CONFIG_IMU_SUPPORT	
	GetSportData(&steps, &calorie, &distance);
	GetSleepTimeData(&deep_sleep, &light_sleep);
#endif
	
	wake = 8;
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x11;
	//data ID
	reply[reply_len++] = (PULL_REFRESH_ID>>8);		
	reply[reply_len++] = (uint8_t)(PULL_REFRESH_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	//Step
	reply[reply_len++] = (steps>>8);
	reply[reply_len++] = (uint8_t)(steps&0x00ff);
	//Calorie
	reply[reply_len++] = (calorie>>8);
	reply[reply_len++] = (uint8_t)(calorie&0x00ff);
	//Distance
	reply[reply_len++] = (distance>>8);
	reply[reply_len++] = (uint8_t)(distance&0x00ff);
	//Shallow Sleep
	reply[reply_len++] = (light_sleep>>8);
	reply[reply_len++] = (uint8_t)(light_sleep&0x00ff);
	//Deep Sleep
	reply[reply_len++] = (deep_sleep>>8);
	reply[reply_len++] = (uint8_t)(deep_sleep&0x00ff);
	//Wake
	reply[reply_len++] = wake;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);

	//�ϴ���������
#ifdef CONFIG_PPG_SUPPORT	
	MCU_send_heart_rate();
#endif
}

void APP_get_location_data(uint8_t *buf, uint32_t len)
{
#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//ble_wait_gps = true; //commented because of no GPS use
	APP_Ask_GPS_Data();
}

void APP_get_gps_data_reply(bool flag, struct gps_pvt gps_data)
{
	uint8_t tmpgps;
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;
	uint32_t tmp1;
	double tmp2;

	if(!flag)
		return;
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x17;
	//data ID
	reply[reply_len++] = (LOCATION_ID>>8);		
	reply[reply_len++] = (uint8_t)(LOCATION_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x00;
	
	//UTC data&time
	//year
	reply[reply_len++] = gps_data.datetime.year>>8;
	reply[reply_len++] = (uint8_t)(gps_data.datetime.year&0x00FF);
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
	tmp1 = (uint32_t)(gps_data.longitude); //������������
	tmp2 = gps_data.longitude - tmp1;	//����С������
	//degree int
	reply[reply_len++] = tmp1;//��������
	tmp1 = (uint32_t)(tmp2*1000000);
	//degree dot1~2
	reply[reply_len++] = (uint8_t)(tmp1/10000);
	tmp1 = tmp1%10000;
	//degree dot3~4
	reply[reply_len++] = (uint8_t)(tmp1/100);
	tmp1 = tmp1%100;
	//degree dot5~6
	reply[reply_len++] = (uint8_t)(tmp1);	
	//latitude
	tmpgps = 'N';
	if(gps_data.latitude < 0)
	{
		tmpgps = 'S';
		gps_data.latitude = -gps_data.latitude;
	}
	//direction N\S
	reply[reply_len++] = tmpgps;
	tmp1 = (uint32_t)(gps_data.latitude);	//������������
	tmp2 = gps_data.latitude - tmp1;	//����С������
	//degree int
	reply[reply_len++] = tmp1;//��������
	tmp1 = (uint32_t)(tmp2*1000000);
	//degree dot1~2
	reply[reply_len++] = (uint8_t)(tmp1/10000);
	tmp1 = tmp1%10000;
	//degree dot3~4
	reply[reply_len++] = (uint8_t)(tmp1/100);
	tmp1 = tmp1%100;
	//degree dot5~6
	reply[reply_len++] = (uint8_t)(tmp1);
					
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

void APP_get_battery_level(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (BATTERY_LEVEL_ID>>8);		
	reply[reply_len++] = (uint8_t)(BATTERY_LEVEL_ID&0x00ff);
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

void APP_get_firmware_version(uint8_t *buf, uint32_t len)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (FIRMWARE_INFOR_ID>>8);		
	reply[reply_len++] = (uint8_t)(FIRMWARE_INFOR_ID&0x00ff);
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

#ifdef CONFIG_PPG_SUPPORT
void APP_get_heart_rate(uint8_t *buf, uint32_t len)
{
	uint8_t heart_rate,reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("type:%d, flag:%d", buf[5], buf[6]);
#endif

	switch(buf[5])
	{
	case 0x01://ʵʱ��������
		break;
	case 0x02://���β�������
		break;
	}

	switch(buf[6])
	{
	case 0://�رմ�����
		break;
	case 1://�򿪴�����
		break;
	}

	if(buf[6] == 0)
	{
		//packet head
		reply[reply_len++] = PACKET_HEAD;
		//data_len
		reply[reply_len++] = 0x00;
		reply[reply_len++] = 0x07;
		//data ID
		reply[reply_len++] = (HEART_RATE_ID>>8);		
		reply[reply_len++] = (uint8_t)(HEART_RATE_ID&0x00ff);
		//status
		reply[reply_len++] = buf[5];
		//control
		reply[reply_len++] = buf[6];
		//heart rate
		reply[reply_len++] = 0;
		//CRC
		reply[reply_len++] = 0x00;
		//packet end
		reply[reply_len++] = PACKET_END;

		for(i=0;i<(reply_len-2);i++)
			reply[reply_len-2] += reply[i];

		ble_send_date_handle(reply, reply_len);	
	}
	else
	{
		g_ppg_trigger |= TRIGGER_BY_APP; 
		APPStartHrSpo2();
	}
}
#endif

//APP�ظ��ֻ������ֻ�
void APP_reply_find_phone(uint8_t *buf, uint32_t len)
{
	uint32_t i;

#ifdef UART_DEBUG
	LOGD("begin");
#endif
}

void MCU_get_nrf52810_ver(void)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (GET_NRF52810_VER_ID>>8);		
	reply[reply_len++] = (uint8_t)(GET_NRF52810_VER_ID&0x00ff);
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
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (GET_BLE_MAC_ADDR_ID>>8);		
	reply[reply_len++] = (uint8_t)(GET_BLE_MAC_ADDR_ID&0x00ff);
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
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (GET_BLE_STATUS_ID>>8);		
	reply[reply_len++] = (uint8_t)(GET_BLE_STATUS_ID&0x00ff);
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

//����BLE����ģʽ		0:�ر� 1:�� 2:���� 3:����
void MCU_set_ble_work_mode(uint8_t work_mode)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x06;
	//data ID
	reply[reply_len++] = (SET_BEL_WORK_MODE_ID>>8);		
	reply[reply_len++] = (uint8_t)(SET_BEL_WORK_MODE_ID&0x00ff);
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

//�ֻ������ֻ�
void MCU_send_find_phone(void)
{
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (FIND_PHONE_ID>>8);		
	reply[reply_len++] = (uint8_t)(FIND_PHONE_ID&0x00ff);
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

//�ֻ��ϱ�һ����������
void MCU_send_app_one_key_measure_data(void)
{
	uint8_t heart_rate,spo2,systolic,diastolic;
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	GetPPGData(&heart_rate, &spo2, &systolic, &diastolic);
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x0A;
	//data ID
	reply[reply_len++] = (ONE_KEY_MEASURE_ID>>8);		
	reply[reply_len++] = (uint8_t)(ONE_KEY_MEASURE_ID&0x00ff);
	//status
	reply[reply_len++] = 0x80;
	//control
	reply[reply_len++] = 0x01;
	//data hr
	reply[reply_len++] = heart_rate;
	//data spo2
	reply[reply_len++] = spo2;
	//data systolic
	reply[reply_len++] = systolic;
	//data diastolic
	reply[reply_len++] = diastolic;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

void MCU_send_app_get_hr_data(void)
{
	uint8_t heart_rate;
	uint8_t reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	GetPPGData(&heart_rate, NULL, NULL, NULL);
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x07;
	//data ID
	reply[reply_len++] = (HEART_RATE_ID>>8);		
	reply[reply_len++] = (uint8_t)(HEART_RATE_ID&0x00ff);
	//status
	reply[reply_len++] = 0x02;
	//control
	reply[reply_len++] = 0x01;
	//heart rate
	reply[reply_len++] = heart_rate;
	//CRC
	reply[reply_len++] = 0x00;
	//packet end
	reply[reply_len++] = PACKET_END;

	for(i=0;i<(reply_len-2);i++)
		reply[reply_len-2] += reply[i];

	ble_send_date_handle(reply, reply_len);
}

//�ֻ��ϱ�������������
void MCU_send_heart_rate(void)
{
	uint8_t heart_rate,reply[128] = {0};
	uint32_t i,reply_len = 0;

#ifdef UART_DEBUG
	LOGD("begin");
#endif

	GetHeartRate(&heart_rate);
	
	//packet head
	reply[reply_len++] = PACKET_HEAD;
	//data_len
	reply[reply_len++] = 0x00;
	reply[reply_len++] = 0x0D;
	//data ID
	reply[reply_len++] = (PULL_REFRESH_ID>>8);		
	reply[reply_len++] = (uint8_t)(PULL_REFRESH_ID&0x00ff);
	//status
	reply[reply_len++] = 0x81;
	//control
	reply[reply_len++] = 0x00;
	//year
	reply[reply_len++] = (uint8_t)(date_time.year>>8);
	reply[reply_len++] = (uint8_t)(date_time.year&0x00ff);
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

void nrf52810_report_work_mode(uint8_t *buf, uint32_t len)
{
#ifdef UART_DEBUG
	LOGD("mode:%d", buf[5]);
#endif

	if(buf[5] == 1)
	{
		ble_work_mode = BLE_WORK_NORMAL;
	}
	else
	{
		ble_work_mode = BLE_WORK_DFU;
		if(g_ble_connected)
		{
			g_ble_connected = false;
			redraw_blt_status_flag = true;
		}
	}
}

void get_nrf52810_ver_response(uint8_t *buf, uint32_t len)
{
	uint32_t i;

	for(i=0;i<len-9;i++)
	{
		g_nrf52810_ver[i] = buf[7+i];
	}

#ifdef UART_DEBUG
	LOGD("nrf52810_ver:%s", g_nrf52810_ver);
#endif
}

void get_ble_mac_address_response(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t mac_addr[6] = {0};
	
	for(i=0;i<6;i++)
		mac_addr[i] = buf[7+i];

	sprintf(g_ble_mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X", 
							mac_addr[0],
							mac_addr[1],
							mac_addr[2],
							mac_addr[3],
							mac_addr[4],
							mac_addr[5]);
#ifdef UART_DEBUG
	LOGD("ble_mac_addr:%s", g_ble_mac_addr);
#endif
}

void get_ble_status_response(uint8_t *buf, uint32_t len)
{
#ifdef UART_DEBUG
	LOGD("BLE_status:%d", buf[6]);
#endif

	g_ble_status = buf[6];
}

/**********************************************************************************
*Name: ble_receive_data_handle
*Function:  �����������յ�������
*Parameter: 
*			Input:
*				buf ���յ�������
*				len ���յ������ݳ���
*			Output:
*				none
*			Return:
*				void
*Description:
*	���յ������ݰ��ĸ�ʽ����:
*	��ͷ			���ݳ���		����		״̬����	����		����1	���ݡ�		У��		��β
*	(StarFrame)		(Data length)	(ID)		(Status)	(Control)	(Data1)	(Data��)	(CRC8)		(EndFrame)
*	(1 bytes)		(2 byte)		(2 byte)	(1 byte)	(1 byte)	(��ѡ)	(��ѡ)		(1 bytes)	(1 bytes)
*
*	�������±���ʾ��
*	Offset	Field		Size	Value(ʮ������)		Description
*	0		StarFrame	1		0xAB				��ʼ֡
*	1		Data length	2		0x0000-0xFFFF		���ݳ���,��ID��ʼһֱ����β
*	3		Data ID		2		0x0000-0xFFFF	    ID
*	5		Status		1		0x00-0xFF	        Status
*	6		Control		1		0x00-0x01			����
*	7		Data0		1-14	0x00-0xFF			����0
*	8+n		CRC8		1		0x00-0xFF			����У��,�Ӱ�ͷ��ʼ��CRCǰһλ
*	9+n		EndFrame	1		0x88				����֡
**********************************************************************************/
void ble_receive_data_handle(uint8_t *buf, uint32_t len)
{
	uint8_t CRC_data=0,data_status;
	uint16_t data_len,data_ID;
	uint32_t i;

#ifdef UART_DEBUG
	//LOGD("receive:%s", buf);
#endif

	if((buf[0] != PACKET_HEAD) || (buf[len-1] != PACKET_END))	//format is error
	{
	#ifdef UART_DEBUG
		LOGD("format is error! HEAD:%x, END:%x", buf[0], buf[len-1]);
	#endif
		return;
	}

	for(i=0;i<len-2;i++)
		CRC_data = CRC_data+buf[i];

	if(CRC_data != buf[len-2])									//crc is error
	{
	#ifdef UART_DEBUG
		LOGD("CRC is error! data:%x, CRC:%x", buf[len-2], CRC_data);
	#endif
		return;
	}

	data_len = buf[1]*256+buf[2];
	data_ID = buf[3]*256+buf[4];

	switch(data_ID)
	{
	case BLE_WORK_MODE_ID:
		nrf52810_report_work_mode(buf, len);//52810����״̬
		break;
	case HEART_RATE_ID:			//����
	#ifdef CONFIG_PPG_SUPPORT
		APP_get_heart_rate(buf, len);
	#endif
		break;
	case BLOOD_OXYGEN_ID:		//Ѫ��
		break;
	case BLOOD_PRESSURE_ID:		//Ѫѹ
		break;
	case ONE_KEY_MEASURE_ID:	//һ������
	#ifdef CONFIG_PPG_SUPPORT
		APP_get_one_key_measure_data(buf, len);
	#endif
		break;
	case PULL_REFRESH_ID:		//����ˢ��
		APP_get_current_data(buf, len);
		break;
	case SLEEP_DETAILS_ID:		//˯������
		break;
	case FIND_DEVICE_ID:		//�����ֻ�
		APP_set_find_device(buf, len);
		break;
	case SMART_NOTIFY_ID:		//��������
		break;
	case ALARM_SETTING_ID:		//��������
		APP_set_alarm(buf, len);
		break;
	case USER_INFOR_ID:			//�û���Ϣ
		break;
	case SEDENTARY_ID:			//��������
		break;
	case SHAKE_SCREEN_ID:		//̧������
		APP_set_wake_screen_by_wrist(buf, len);
		break;
	case MEASURE_HOURLY_ID:		//�����������
		APP_set_PHD_interval(buf, len);
		break;
	case SHAKE_PHOTO_ID:		//ҡһҡ����
		break;
	case LANGUAGE_SETTING_ID:	//��Ӣ�����л�
		APP_set_language(buf, len);
		break;
	case TIME_24_SETTING_ID:	//12/24Сʱ����
		APP_set_time_24_format(buf, len);
		break;
	case FIND_PHONE_ID:			//�����ֻ��ظ�
		APP_reply_find_phone(buf, len);
		break;
	case WEATHER_INFOR_ID:		//������Ϣ�·�
		break;
	case TIME_SYNC_ID:			//ʱ��ͬ��
		APP_set_date_time(buf, len);
		break;
	case TARGET_STEPS_ID:		//Ŀ�경��
		APP_set_target_steps(buf, len);
		break;
	case BATTERY_LEVEL_ID:		//��ص���
		APP_get_battery_level(buf, len);
		break;
	case FIRMWARE_INFOR_ID:		//�̼��汾��
		APP_get_firmware_version(buf, len);
		break;
	case FACTORY_RESET_ID:		//����ֻ�����
		APP_set_factory_reset(buf, len);
		break;
	case ECG_ID:				//�ĵ�
		break;
	case LOCATION_ID:			//��ȡ��λ��Ϣ
		//APP_get_location_data(buf, len);
		break;
	case DATE_FORMAT_ID:		//�����ո�ʽ
		APP_set_date_format(buf, len);
		break;
	case BLE_CONNECT_ID:		//BLE��������
		ble_connect_or_disconnect_handle(buf, len);
		break;
	case CTP_NOTIFY_ID:
	#ifdef CONFIG_TOUCH_SUPPORT
		CTP_notify_handle(buf, len);
	#endif
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
	#ifdef UART_DEBUG	
		LOGD("data_id is unknown!");
	#endif
		break;
	}
}

void ble_wakeup_nrf52810(void)
{
	if(!gpio_ble)
	{
		gpio_ble = device_get_binding(BLE_PORT);
		if(!gpio_ble)
		{
		#ifdef UART_DEBUG
			LOGD("Cannot bind gpio device");
		#endif
			return;
		}

		gpio_pin_configure(gpio_ble, BLE_WAKE_PIN, GPIO_OUTPUT);
	}

	gpio_pin_set(gpio_ble, BLE_WAKE_PIN, 1);
	k_sleep(K_MSEC(10));
	gpio_pin_set(gpio_ble, BLE_WAKE_PIN, 0);
}

//xb add 2021-11-01 ����52810�Ĵ�����Ҫʱ�䣬����ֱ���ڽ��յ��ж�����ʱ�ȴ�����ֹ����
void uart_send_data_handle(void)
{
	switch(uart_data_type)
	{
	case DATA_TYPE_BLE:
		ble_wakeup_nrf52810();
	#ifdef CONFIG_WIFI
		switch_to_ble();
	#endif	
		break;

	case DATA_TYPE_WIFI:
	#ifdef CONFIG_WIFI
		switch_to_ble();
	#endif	
		
		break;
	}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uart_sleep_out();
#endif

	uart_fifo_fill(uart_ble, tx_buf, send_len);
	uart_irq_tx_enable(uart_ble); 	
}

void ble_send_date_handle(uint8_t *buf, uint32_t len)
{
#ifdef UART_DEBUG
	LOGD("begin");
#endif

	send_len = len;
	memcpy(tx_buf, buf, len);
	
	uart_data_type = DATA_TYPE_BLE;
	uart_send_flag = true;
}

#ifdef CONFIG_WIFI
void wifi_send_data_handle(uint8_t *buf, uint32_t len)
{
#ifdef UART_DEBUG
	LOGD("cmd:%s", buf);
#endif

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uart_sleep_out();
#endif

	switch_to_wifi();

	uart_fifo_fill(uart_ble, buf, len);
	uart_irq_tx_enable(uart_ble);
}
#endif

static void uart_receive_data(uint8_t data, uint32_t datalen)
{
	static uint32_t data_len = 0;

#ifdef UART_DEBUG
	//LOGD("rece_len:%d, data_len:%d data:%x", rece_len, data_len, data);
#endif

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	if(!uart_is_waked)
		return;
#endif

    if(blue_is_on)
    {
    	if((rece_len == 0) && (data != PACKET_HEAD))
			return;
		
        rx_buf[rece_len++] = data;
		if(rece_len == 3)
			data_len = (256*rx_buf[1]+rx_buf[2]+3);
		
        if(rece_len == data_len)	
        {
            ble_receive_data_handle(rx_buf, rece_len);
            
            memset(rx_buf, 0, sizeof(rx_buf));
            rece_len = 0;
			data_len = 0;
        }
		else if((rece_len >= data_len)&&(data == PACKET_END))
        {
            memset(rx_buf, 0, sizeof(rx_buf));
            rece_len = 0;
			data_len = 0;
        }
		else if(rece_len >= BUF_MAXSIZE)
		{
			memset(rx_buf, 0, sizeof(rx_buf));
            rece_len = 0;
			data_len = 0;
		}		
    }
#ifdef CONFIG_WIFI	
    else if(wifi_is_on)
    {
        rx_buf[rece_len++] = data;

        if((rx_buf[rece_len-2] == 0x4F) && (rx_buf[rece_len-1] == 0x4B))	//"OK"
        {
        	wifi_receive_data_handle(rx_buf, rece_len);
			
            memset(rx_buf, 0, sizeof(rx_buf));
            rece_len = 0;
        }
		else if(rece_len == BUF_MAXSIZE)
		{
			memset(rx_buf, 0, sizeof(rx_buf));
            rece_len = 0;
		}
   }
#endif	
}

void uart_send_data(void)
{
#ifdef UART_DEBUG
	LOGD("begin");
#endif

	uart_fifo_fill(uart_ble, "Hello World!", strlen("Hello World!"));
	uart_irq_tx_enable(uart_ble); 
}

static void uart_cb(struct device *x)
{
	uint8_t tmpbyte = 0;
	uint32_t len=0;

	uart_irq_update(x);

	if(uart_irq_rx_ready(x)) 
	{
		while((len = uart_fifo_read(x, &tmpbyte, 1)) > 0)
			uart_receive_data(tmpbyte, 1);
	}
	
	if(uart_irq_tx_ready(x))
	{
		struct uart_data_t *buf;
		uint16_t written = 0;

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

void uart_sleep_out(void)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	if(k_timer_remaining_get(&uart_sleep_in_timer) > 0)
		k_timer_stop(&uart_sleep_in_timer);
	k_timer_start(&uart_sleep_in_timer, K_SECONDS(UART_WAKE_HOLD_TIME_SEC), NULL);

	if(uart_is_waked)
		return;
	
	device_set_power_state(uart_ble, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
	uart_irq_rx_enable(uart_ble);
	uart_irq_tx_enable(uart_ble);
	
	k_sleep(K_MSEC(10));
	
	uart_is_waked = true;

#ifdef UART_DEBUG
	LOGD("uart set active success!");
#endif
#endif
}

void uart_sleep_in(void)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	if(!uart_is_waked)
		return;
	
	uart_irq_rx_disable(uart_ble);
	uart_irq_tx_disable(uart_ble);
	device_set_power_state(uart_ble, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);

	k_sleep(K_MSEC(10));
	
	uart_is_waked = false;

#ifdef UART_DEBUG
	LOGD("uart set low power success!");
#endif
#endif
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void ble_interrupt_event(struct device *interrupt, struct gpio_callback *cb, uint32_t pins)
{
#ifdef UART_DEBUG
	LOGD("begin");
#endif
	uart_wake_flag = true;
}

void UartSleepInCallBack(struct k_timer *timer_id)
{
#ifdef UART_DEBUG
	LOGD("begin");
#endif
	uart_sleep_flag = true;
}
#endif

static void GetBLEInfoCallBack(struct k_timer *timer_id)
{
	get_ble_info_flag = true;
}

void ble_init(void)
{
	int flag = GPIO_INPUT|GPIO_INT_ENABLE|GPIO_INT_EDGE|GPIO_PULL_DOWN|GPIO_INT_HIGH_1|GPIO_INT_DEBOUNCE;
#ifdef UART_DEBUG
	LOGD("begin");
#endif
	uart_ble = device_get_binding(BLE_DEV);
	if(!uart_ble)
	{
	#ifdef UART_DEBUG
		LOGD("Could not get %s device", BLE_DEV);
	#endif
		return;
	}
	
	uart_irq_callback_set(uart_ble, uart_cb);
	uart_irq_rx_enable(uart_ble);

#ifdef CONFIG_WIFI
	wifi_disable();
	switch_to_ble();
#endif

	gpio_ble = device_get_binding(BLE_PORT);
	if(!gpio_ble)
	{
	#ifdef UART_DEBUG
		LOGD("Could not get %s port", BLE_PORT);
	#endif
		return;
	}	
	gpio_pin_configure(gpio_ble, BLE_WAKE_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ble, BLE_WAKE_PIN, 0);

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	gpio_pin_configure(gpio_ble, BLE_INT_PIN, flag);
	gpio_pin_disable_callback(gpio_ble, BLE_INT_PIN);
	gpio_init_callback(&gpio_cb, ble_interrupt_event, BIT(BLE_INT_PIN));
	gpio_add_callback(gpio_ble, &gpio_cb);
	gpio_pin_enable_callback(gpio_ble, BLE_INT_PIN);
	k_timer_start(&uart_sleep_in_timer, K_SECONDS(UART_WAKE_HOLD_TIME_SEC), NULL);
#endif

	k_timer_start(&get_ble_info_timer, K_SECONDS(2), K_NO_WAIT);
}

void UartMsgProc(void)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	if(uart_wake_flag)
	{
	#ifdef UART_DEBUG
		LOGD("uart_wake!");
	#endif
		uart_wake_flag = false;
		uart_sleep_out();
	}

	if(uart_sleep_flag)
	{
	#ifdef UART_DEBUG
		LOGD("uart_sleep!");
	#endif
		uart_sleep_flag = false;		
		uart_sleep_in();
	}
#endif	
	if(uart_send_flag)
	{
		uart_send_data_handle();
		uart_send_flag = false;
	}
	if(redraw_blt_status_flag)
	{
		IdleShowBleStatus(g_ble_connected);
		redraw_blt_status_flag = false;
	}
	if(get_ble_info_flag)
	{
		static uint8_t index = 0;

		if(index == 0)
		{
			index = 1;
			MCU_get_nrf52810_ver();
			k_timer_start(&get_ble_info_timer, K_MSEC(100), K_NO_WAIT);
		}
		else
		{
			MCU_get_ble_mac_address();
		}
		
		get_ble_info_flag = false;
	}
}

void test_uart_ble(void)
{
#ifdef UART_DEBUG
	LOGD("begin");
#endif

	while(1)
	{
		ble_send_date_handle("Hello World!\n", strlen("Hello World!\n"));
		k_sleep(K_MSEC(1000));
	}
}
