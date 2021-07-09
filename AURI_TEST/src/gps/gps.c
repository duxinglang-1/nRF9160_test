/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <nrf_socket.h>
#include <net/socket.h>
#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_agps.h>
#endif
#include <stdio.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include "datetime.h"
#include "lcd.h"
#include "screen.h"
#include "uart_ble.h"
#include "Settings.h"
#include "nb.h"
#include "sos.h"
#include "gps_controller.h"
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(gps, CONFIG_LOG_DEFAULT_LEVEL);

#define AT_XSYSTEMMODE_GPSON      "AT\%XSYSTEMMODE=0,1,1,0"
#define AT_XSYSTEMMODE_GPSOFF     "AT\%XSYSTEMMODE=0,1,0,0"
#define AT_ACTIVATE_GPS     "AT+CFUN=31"
#define AT_DEACTIVATE_GPS	"AT+CFUN=30"
#define AT_ACTIVATE_LTE     "AT+CFUN=21"
#define AT_DEACTIVATE_LTE   "AT+CFUN=20"

#define GNSS_INIT_AND_START 1
#define GNSS_STOP           2
#define GNSS_RESTART        3
#define GNSS_CLOSE        	4

#define AT_CMD_SIZE(x) (sizeof(x) - 1)

#ifdef CONFIG_BOARD_NRF9160_PCA10090NS
#define AT_MAGPIO			"AT\%XMAGPIO=1,0,0,1,1,1574,1577"
#define AT_MAGPIO_CANCEL	"AT%XMAGPIO"
#define AT_COEX0			"AT\%XCOEX0=1,1,1570,1580"
#define AT_COEX0_CANCEL		"AT\%XCOEX0"
#endif

static void APP_Ask_GPS_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(app_wait_gps_timer, APP_Ask_GPS_Data_timerout, NULL);
static void APP_Send_GPS_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(app_send_gps_timer, APP_Send_GPS_Data_timerout, NULL);

static bool gps_is_on = false;

static s64_t gps_start_time=0,gps_fix_time=0;

static struct k_work_q *app_work_q;
static struct k_work send_agps_request_work;

static struct gps_pvt gps_pvt_data = {0};

bool app_gps_on = false;
bool app_gps_off = false;
bool ble_wait_gps = false;
bool sos_wait_gps = false;
bool fall_wait_gps = false;
bool location_wait_gps = false;
bool test_gps_flag = false;
bool gps_test_update_flag = false;

u8_t gps_test_info[256] = {0};

static void set_gps_enable(const bool enable);

K_SEM_DEFINE(lte_ready, 0, 1);

bool APP_GPS_data_send(bool fix_flag)
{
	bool ret = false;
	
	if(ble_wait_gps)
	{
		APP_get_gps_data_reply(fix_flag, gps_pvt_data);
		ble_wait_gps = false;
		ret = true;
	}

	if(sos_wait_gps)
	{
		sos_get_gps_data_reply(fix_flag, gps_pvt_data);
		sos_wait_gps = false;
		ret = true;
	}

	if(fall_wait_gps)
	{
		fall_get_gps_data_reply(fix_flag, gps_pvt_data);
		fall_wait_gps = false;
		ret = true;
	}

	if(location_wait_gps)
	{
		location_get_gps_data_reply(fix_flag, gps_pvt_data);
		location_wait_gps = false;
		ret = true;
	}

	return ret;
}

void APP_Ask_GPS_Data_timerout(struct k_timer *timer)
{
	app_gps_off = true;
	APP_GPS_data_send(false);
}

void APP_Ask_GPS_Data(void)
{
	static u8_t time_init = false;

#if 1
	if(!gps_is_on)
	{
		app_gps_on = true;
		k_timer_start(&app_wait_gps_timer, K_MSEC(3*60*1000), NULL);
	}
#else
	gps_pvt_data.datetime.year = 2020;
	gps_pvt_data.datetime.month = 11;
	gps_pvt_data.datetime.day = 4;
	gps_pvt_data.datetime.hour = 2;
	gps_pvt_data.datetime.minute = 20;
	gps_pvt_data.datetime.seconds = 40;
	gps_pvt_data.longitude = 114.025254;
	gps_pvt_data.latitude = 22.667808;

	APP_Ask_GPS_Data_timerout(NULL);
#endif
}

void APP_Send_GPS_Data_timerout(struct k_timer *timer)
{
	APP_GPS_data_send(true);
}

void APP_Ask_GPS_off(void)
{
	app_gps_off = true;
}

void gps_off(void)
{
	set_gps_enable(false);
}

bool gps_is_working(void)
{
	return gps_is_on;
}

void gps_on(void)
{
	set_gps_enable(true);
}

void gps_test_update(void)
{
	if(screen_id == SCREEN_ID_GPS_TEST)
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
}

void test_gps(void)
{
	test_gps_flag = true;
	EnterGPSTestScreen();
	gps_on();
}

static void set_gps_enable(const bool enable)
{
	if(enable == gps_control_is_enabled())
	{
		return;
	}

	if(enable)
	{
		if(at_cmd_write("AT+CFUN=1", NULL, 0, NULL) != 0)
		{
			LOG_INF("Can't turn on modem!");
			return;
		}
		
		LOG_INF("Starting GPS");
		gps_control_start(K_NO_WAIT);
		gps_is_on = true;
		gps_start_time = k_uptime_get();
	}
	else
	{
		LOG_INF("Stopping GPS");
		gps_control_stop(K_NO_WAIT);
		gps_is_on = false;
	}
}

static void send_agps_request(struct k_work *work)
{
#if defined(CONFIG_NRF_CLOUD_AGPS)
	int err;
	static s64_t last_request_timestamp = 0;
	
	ARG_UNUSED(work);
	
	nrf_cloud_start();
	
	if((last_request_timestamp != 0) &&
	    (k_uptime_get() - last_request_timestamp < K_HOURS(1)))
	{
		LOG_INF("A-GPS request was sent less than 1 hour ago");
		return;
	}

	LOG_INF("Sending A-GPS request");

	err = nrf_cloud_agps_request_all();
	if(err)
	{
		LOG_INF("A-GPS request failed, error: %d", err);
		return;
	}

	last_request_timestamp = k_uptime_get();

	LOG_INF("A-GPS request sent");
#endif /* defined(CONFIG_NRF_CLOUD_AGPS) */
}

static void gps_handler(struct device *dev, struct gps_event *evt)
{
	u8_t tmpbuf[128] = {0};
	
	switch (evt->type)
	{
	case GPS_EVT_SEARCH_STARTED:
		LOG_INF("GPS_EVT_SEARCH_STARTED\n");
		gps_control_set_active(true);
		break;
		
	case GPS_EVT_SEARCH_STOPPED:
		LOG_INF("GPS_EVT_SEARCH_STOPPED\n");
		gps_control_set_active(false);
		break;
		
	case GPS_EVT_SEARCH_TIMEOUT:
		LOG_INF("GPS_EVT_SEARCH_TIMEOUT\n");
		gps_control_set_active(false);
		break;
		
	case GPS_EVT_PVT:
		/* Don't spam logs */
		LOG_INF("GPS_EVT_PVT");
		
		if(test_gps_flag)
		{
			u8_t i,tracked;
			u8_t strbuf[128] = {0};

			memset(gps_test_info, 0x00, sizeof(gps_test_info));
			
			for(i=0;i<GPS_PVT_MAX_SV_COUNT;i++)
			{
				u8_t buf[128] = {0};
				
				if((evt->pvt.sv[i].sv > 0) && (evt->pvt.sv[i].sv < 32))
				{
					tracked++;
				#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
					if(tracked<8)
					{
						sprintf(buf, "%02d|", evt->pvt.sv[i].cn0/10);
						strcat(strbuf, buf);
					}
				#else
					sprintf(buf, "%02d|%02d;", evt->pvt.sv[i].sv, evt->pvt.sv[i].cn0/10);
					strcat(strbuf, buf);
				#endif
				}
			}

			sprintf(gps_test_info, "%02d,", tracked);
			strcat(gps_test_info, strbuf);
			gps_test_update_flag = true;
			
			//LOG_INF("%s\n",gps_test_info);
			//UpdataTestGPSInfo();
			//TestGPSShowInfor();
		}
		else
		{
			sprintf(tmpbuf, "Longitude:%f, Latitude:%f, Altitude:%f, Speed:%f, Heading:%f", 
								evt->pvt.longitude, 
								evt->pvt.latitude,
								evt->pvt.altitude,
								evt->pvt.speed,
								evt->pvt.heading);
			LOG_INF("%s",tmpbuf);
			
			LOG_INF("Date:       %02u-%02u-%02u", evt->pvt.datetime.year,
						       					  evt->pvt.datetime.month,
						       					  evt->pvt.datetime.day);
			LOG_INF("Time (UTC): %02u:%02u:%02u\n", evt->pvt.datetime.hour,
						       					  evt->pvt.datetime.minute,
						      					  evt->pvt.datetime.seconds);
		}
		break;
	
	case GPS_EVT_PVT_FIX:
		LOG_INF("GPS_EVT_PVT_FIX");

		if(test_gps_flag)
		{
			u8_t i,tracked;
			u8_t strbuf[128] = {0};

			memset(gps_test_info, 0x00, sizeof(gps_test_info));
			
			for(i=0;i<GPS_PVT_MAX_SV_COUNT;i++)
			{
				u8_t buf[128] = {0};
				
				if((evt->pvt.sv[i].sv > 0) && (evt->pvt.sv[i].sv < 32))
				{
					tracked++;
				#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
					if(tracked<8)
					{
						sprintf(buf, "%02d|", evt->pvt.sv[i].cn0/10);
						strcat(strbuf, buf);
					}
				#else
					sprintf(buf, "%02d|%02d;", evt->pvt.sv[i].sv, evt->pvt.sv[i].cn0/10);
					strcat(strbuf, buf);
				#endif
				}
			}

			sprintf(gps_test_info, "%02d,", tracked);
			strcat(gps_test_info, strbuf);
			gps_test_update_flag = true;
			
			//LOG_INF("%s\n",gps_test_info);
			//UpdataTestGPSInfo();
			//TestGPSShowInfor();
		}		
		else
		{
			sprintf(tmpbuf, "Longitude:%f, Latitude:%f", evt->pvt.longitude, evt->pvt.latitude);
			LOG_INF("%s\n",tmpbuf);
		
			memcpy(&gps_pvt_data, &(evt->pvt), sizeof(evt->pvt));
		}
		break;
		
	case GPS_EVT_NMEA:
		/* Don't spam logs */
		LOG_INF("GPS_EVT_NMEA\n");
		break;
		
	case GPS_EVT_NMEA_FIX:
		LOG_INF("GPS_EVT_NMEA_FIX");
		
		if(!test_gps_flag)
		{
			if(gps_fix_time == 0)
				gps_fix_time = k_uptime_get();
		
			LOG_INF("Position fix with NMEA data, fix time:%d", gps_fix_time-gps_start_time);
			LOG_INF("NMEA:%s\n", evt->nmea.buf);
		
			APP_Ask_GPS_off();

			if(k_timer_remaining_get(&app_wait_gps_timer) > 0)
				k_timer_stop(&app_wait_gps_timer);

			k_timer_start(&app_send_gps_timer, K_MSEC(1000), NULL);
		}
		break;
		
	case GPS_EVT_OPERATION_BLOCKED:
		LOG_INF("GPS_EVT_OPERATION_BLOCKED\n");
		break;
		
	case GPS_EVT_OPERATION_UNBLOCKED:
		LOG_INF("GPS_EVT_OPERATION_UNBLOCKED\n");
		break;
		
	case GPS_EVT_AGPS_DATA_NEEDED:
		LOG_INF("GPS_EVT_AGPS_DATA_NEEDED\n");
		k_work_submit_to_queue(app_work_q,
				       &send_agps_request_work);
		break;
		
	case GPS_EVT_ERROR:
		LOG_INF("GPS_EVT_ERROR\n");
		break;
		
	default:
		break;
	}
}

void GPS_init(struct k_work_q *work_q)
{
	app_work_q = work_q;

	k_work_init(&send_agps_request_work, send_agps_request);

	gps_control_init(app_work_q, gps_handler);
}

void GPSMsgProcess(void)
{
	if(app_gps_on)
	{
		app_gps_on = false;
		gps_on();
	}
	
	if(app_gps_off)
	{
		app_gps_off = false;
		gps_off();
	}
	
	if(gps_test_update_flag)
	{
		gps_test_update_flag = false;
		gps_test_update();
	}
	
	if(gps_is_working())
	{
		k_sleep(K_MSEC(1));
	}
}
