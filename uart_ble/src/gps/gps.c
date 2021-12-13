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
#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/cloud.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_agps.h>
#include "cloud_codec.h"
#endif

#include "logger.h"

#define GPS_DEBUG

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

static s64_t gps_start_time=0,gps_fix_time=0,gps_local_time=0;

static struct k_work_q *app_work_q;
static struct k_work send_agps_request_work;

static struct cloud_backend *cloud_backend;

static struct gps_pvt gps_pvt_data = {0};

bool gps_on_flag = false;
bool gps_off_flag = false;
bool ble_wait_gps = false;
bool sos_wait_gps = false;
bool fall_wait_gps = false;
bool location_wait_gps = false;
bool test_gps_flag = false;
bool gps_test_start_flag = false;
bool gps_test_update_flag = false;
bool gps_send_data_flag = false;

u8_t gps_test_info[256] = {0};

enum error_type
{
	ERROR_CLOUD,
	ERROR_BSD_RECOVERABLE,
	ERROR_LTE_LC,
	ERROR_SYSTEM_FAULT
};

static void set_gps_enable(const bool enable);

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

#ifdef CONFIG_IMU_SUPPORT
	if(fall_wait_gps)
	{
		fall_get_gps_data_reply(fix_flag, gps_pvt_data);
		fall_wait_gps = false;
		ret = true;
	}
#endif

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
	if(!test_gps_flag)
		gps_off_flag = true;

	gps_send_data_flag = true;
}

void APP_Ask_GPS_Data(void)
{
	static u8_t time_init = false;

#if 1
	if(!gps_is_on)
	{
		gps_on_flag = true;
		k_timer_start(&app_wait_gps_timer, K_MSEC(5*60*1000), NULL);
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
	gps_local_time = 1000;
	
	APP_Ask_GPS_Data_timerout(NULL);
#endif
}

void APP_Send_GPS_Data_timerout(struct k_timer *timer)
{
	gps_send_data_flag = true;
}

void APP_Ask_GPS_off(void)
{
	if(!test_gps_flag)
		gps_off_flag = true;
}

void gps_off(void)
{
	set_gps_enable(false);
}

bool gps_is_working(void)
{
#ifdef GPS_DEBUG
	LOGD("gps_is_on:%d", gps_is_on);
#endif
	return gps_is_on;
}

void gps_on(void)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uart_sleep_out();
#endif
	set_gps_enable(true);
}

void gps_test_update(void)
{
	if(screen_id == SCREEN_ID_GPS_TEST)
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
}

void MenuStartGPS(void)
{
	test_gps_flag = true;
	gps_on_flag = true;
}

void MenuStopGPS(void)
{
	test_gps_flag = false;
	gps_off_flag = true;
}

void test_gps_on(void)
{
	test_gps_flag = true;
	EnterGPSTestScreen();
	gps_on();
}

void test_gps_off(void)
{
	test_gps_flag = false;
	gps_off();
	EnterIdleScreen();
}

static void set_gps_enable(const bool enable)
{
	if(enable == gps_control_is_enabled())
	{
		return;
	}

	if(enable)
	{
		DisConnectMqttLink();
		SetModemTurnOff();
		
		if(at_cmd_write("AT+CFUN=31", NULL, 0, NULL) != 0)
		{
		#ifdef GPS_DEBUG
			LOGD("Can't turn on modem for gps!");
		#endif
		}
		
	#ifdef GPS_DEBUG	
		LOGD("Starting GPS");
	#endif
	
		gps_control_start(K_NO_WAIT);

		gps_fix_time = 0;
		gps_local_time = 0;
		gps_start_time = k_uptime_get();

		gps_is_on = true;
	}
	else
	{
	#ifdef GPS_DEBUG
		LOGD("Stopping GPS");
	#endif
	
		gps_control_stop(K_NO_WAIT);

		if(at_cmd_write("AT+CFUN=30", NULL, 0, NULL) != 0)
		{
		#ifdef GPS_DEBUG
			LOGD("Can't turn off modem for gps!");
		#endif
		}

		gps_is_on = false;
	}
}

#ifdef CONFIG_NRF_CLOUD_AGPS
void error_handler(enum error_type err_type, int err_code)
{
	if(err_type == ERROR_CLOUD)
	{
	#if defined(CONFIG_LTE_LINK_CONTROL)
		/* Turn off and shutdown modem */
		LOG_ERR("LTE link disconnect");

		int err = lte_lc_power_off();

		if(err)
		{
			LOG_ERR("lte_lc_power_off failed: %d", err);
		}
	#endif /* CONFIG_LTE_LINK_CONTROL */
	}

	switch(err_type)
	{
	case ERROR_CLOUD:
		/* Blinking all LEDs ON/OFF in pairs (1 and 4, 2 and 3)
		 * if there is an application error.
		 */
		LOG_ERR("Error of type ERROR_CLOUD: %d", err_code);
		break;
		
	case ERROR_BSD_RECOVERABLE:
		/* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
		 * if there is a recoverable error.
		 */
		LOG_ERR("Error of type ERROR_BSD_RECOVERABLE: %d", err_code);
		break;
		
	default:
		/* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
		 * undefined error.
		 */
		LOG_ERR("Unknown error type: %d, code: %d",	err_type, err_code);
		break;
	}
}

void cloud_error_handler(int err)
{
	error_handler(ERROR_CLOUD, err);
}

void cloud_connect_error_handler(enum cloud_connect_result err)
{
	char *backend_name = "invalid";

	if(err == CLOUD_CONNECT_RES_SUCCESS)
	{
		return;
	}

	LOG_INF("Failed to connect to cloud, error %d", err);
	
	switch(err)
	{
	case CLOUD_CONNECT_RES_ERR_NOT_INITD:
		LOG_INF("Cloud back-end has not been initialized");
		break;
		
	case CLOUD_CONNECT_RES_ERR_NETWORK:
		LOG_INF("Network error, check cloud configuration");
		break;

	case CLOUD_CONNECT_RES_ERR_BACKEND:
		if(cloud_backend && cloud_backend->config &&
		    cloud_backend->config->name)
		{
			backend_name = cloud_backend->config->name;
		}
		
		LOG_INF("An error occurred specific to the cloud back-end: %s", backend_name);
		break;

	case CLOUD_CONNECT_RES_ERR_PRV_KEY:
		LOG_INF("Ensure device has a valid private key");
		break;

	case CLOUD_CONNECT_RES_ERR_CERT:
		LOG_INF("Ensure device has a valid CA and client certificate");
		break;

	case CLOUD_CONNECT_RES_ERR_CERT_MISC:
		LOG_INF("A certificate/authorization error has occurred");
		break;

	case CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA:
		LOG_INF("Connect timeout. SIM card may be out of data");
		break;

	case CLOUD_CONNECT_RES_ERR_MISC:
		break;

	default:
		LOG_INF("Unhandled connect error");
		break;
	}

	k_thread_suspend(k_current_get());
}

static void cloud_cmd_handler(struct cloud_command *cmd)
{
	if((cmd->channel == CLOUD_CHANNEL_GPS) &&
	    (cmd->group == CLOUD_CMD_GROUP_CFG_SET) &&
	    (cmd->type == CLOUD_CMD_ENABLE))
	{
		set_gps_enable(cmd->data.sv.state == CLOUD_CMD_STATE_TRUE);
	}
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);

	switch(evt->type)
	{
	case CLOUD_EVT_CONNECTED:
		LOG_INF("CLOUD_EVT_CONNECTED");
		break;

	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
		break;

	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		break;

	case CLOUD_EVT_ERROR:
		LOG_INF("CLOUD_EVT_ERROR");
		break;

	case CLOUD_EVT_DATA_SENT:
		LOG_INF("CLOUD_EVT_DATA_SENT");
		break;
		
	case CLOUD_EVT_DATA_RECEIVED:
	{
		int err;

		LOG_INF("CLOUD_EVT_DATA_RECEIVED");
		err = cloud_decode_command(evt->data.msg.buf);
		if (err == 0) {
			/* Cloud decoder has handled the data */
			return;
		}

	#if defined(CONFIG_NRF_CLOUD_AGPS)
		/* The decoder didn't handle the data, check if it's A-GPS data
		 */
		err = nrf_cloud_agps_process(evt->data.msg.buf,
					     evt->data.msg.len,
					     NULL);
		if (err)
		{
			LOG_INF("Data was not valid A-GPS data, err: %d", err);
			break;
		}

		LOG_INF("A-GPS data processed");
	#endif /* defined(CONFIG_GPS_USE_AGPS) */
		break;
	}
	
	case CLOUD_EVT_PAIR_REQUEST:
		LOG_INF("CLOUD_EVT_PAIR_REQUEST");
		break;
		
	case CLOUD_EVT_PAIR_DONE:
		LOG_INF("CLOUD_EVT_PAIR_DONE");
		break;
		
	case CLOUD_EVT_FOTA_DONE:
		LOG_INF("CLOUD_EVT_FOTA_DONE");
		break;
		
	default:
		LOG_INF("Unknown cloud event type: %d", evt->type);
		break;
	}
}

static void nrf_cloud_start(void)
{
	int ret;

	cloud_backend = cloud_get_binding("NRF_CLOUD");
	__ASSERT(cloud_backend != NULL, "nRF Cloud backend not found");

	ret = cloud_init(cloud_backend, cloud_event_handler);
	if(ret)
	{
		LOG_ERR("Cloud backend could not be initialized, error: %d", ret);
		cloud_error_handler(ret);
	}

	ret = cloud_decode_init(cloud_cmd_handler);
	if(ret)
	{
		LOG_ERR("Cloud command decoder could not be initialized, error: %d", ret);
		cloud_error_handler(ret);
	}

	ret = cloud_connect(cloud_backend);
	if(ret != CLOUD_CONNECT_RES_SUCCESS)
	{
		cloud_connect_error_handler(ret);
	}

	struct pollfd fds[] = {
		{
		.fd = cloud_backend->config->socket,
		.events = POLLIN
		}
	};

	while(true)
	{
		ret = poll(fds, ARRAY_SIZE(fds),
		cloud_keepalive_time_left(cloud_backend));
		if(ret < 0)
		{
			LOG_INF("poll() returned an error: %d", ret);
			error_handler(ERROR_CLOUD, ret);
			continue;
		}

		if(ret == 0)
		{
			cloud_ping(cloud_backend);
			continue;
		}

		if((fds[0].revents & POLLIN) == POLLIN)
		{
			cloud_input(cloud_backend);
		}

		if((fds[0].revents & POLLNVAL) == POLLNVAL)
		{
			LOG_INF("Socket error: POLLNVAL");
			LOG_INF("The cloud socket was unexpectedly closed.");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if((fds[0].revents & POLLHUP) == POLLHUP)
		{
			LOG_INF("Socket error: POLLHUP");
			LOG_INF("Connection was closed by the cloud.");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if((fds[0].revents & POLLERR) == POLLERR)
		{
			LOG_INF("Socket error: POLLERR");
			LOG_INF("Cloud connection was unexpectedly closed.");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}
	}

	cloud_disconnect(cloud_backend);
}
#endif

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
	#ifdef GPS_DEBUG
		LOGD("A-GPS request was sent less than 1 hour ago");
	#endif
		return;
	}
#ifdef GPS_DEBUG
	LOGD("Sending A-GPS request");
#endif
	err = nrf_cloud_agps_request_all();
	if(err)
	{
	#ifdef GPS_DEBUG
		LOGD("A-GPS request failed, error: %d", err);
	#endif
		return;
	}

	last_request_timestamp = k_uptime_get();
#ifdef GPS_DEBUG
	LOGD("A-GPS request sent");
#endif
#endif /* defined(CONFIG_NRF_CLOUD_AGPS) */
}

static void gps_handler(struct device *dev, struct gps_event *evt)
{
	u8_t tmpbuf[128] = {0};
	
	switch (evt->type)
	{
	case GPS_EVT_SEARCH_STARTED:
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_SEARCH_STARTED");
	#endif
		gps_control_set_active(true);
		break;
		
	case GPS_EVT_SEARCH_STOPPED:
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_SEARCH_STOPPED");
	#endif
		gps_control_set_active(false);
		break;
		
	case GPS_EVT_SEARCH_TIMEOUT:
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_SEARCH_TIMEOUT");
	#endif
		gps_control_set_active(false);
		break;
		
	case GPS_EVT_PVT:
		/* Don't spam logs */
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_PVT");
	#endif	
		if(test_gps_flag)
		{
			u8_t i,tracked;
			u8_t strbuf[256] = {0};

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
		#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			sprintf(gps_test_info, "%02d,", tracked);
		#else
			sprintf(gps_test_info, "%02d\n", tracked);
		#endif
			strcat(gps_test_info, strbuf);
			gps_test_update_flag = true;
		}
		else
		{
		#ifdef GPS_DEBUG
			s32_t lon,lat;

			lon = evt->pvt.longitude*1000000;
			lat = evt->pvt.latitude*1000000;
			sprintf(tmpbuf, "Longitude:%d.%06d, Latitude:%d.%06d", lon/1000000, lon%1000000, lat/1000000, lat%1000000);
			LOGD("%s",tmpbuf);
			LOGD("Date:       %02u-%02u-%02u", evt->pvt.datetime.year,
						       					  evt->pvt.datetime.month,
						       					  evt->pvt.datetime.day);
			LOGD("Time (UTC): %02u:%02u:%02u", evt->pvt.datetime.hour,
						       					  evt->pvt.datetime.minute,
						      					  evt->pvt.datetime.seconds);
		#endif	
		}
		break;
	
	case GPS_EVT_PVT_FIX:
	#ifdef GPS_DEBUG	
		LOGD("GPS_EVT_PVT_FIX");
	#endif
		if(test_gps_flag)
		{
			u8_t i,tracked;
			u8_t strbuf[128] = {0};
			s32_t lon,lat;
			
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

		#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			sprintf(gps_test_info, "%02d,", tracked);
			strcat(gps_test_info, strbuf);
			if(tracked < 7)
				strcat(gps_test_info, "\n \n");	//2行没显示满，多换行一行
			else
				strcat(gps_test_info, "\n");
			
			if(gps_fix_time > 0)
			{
				sprintf(strbuf, "fix:%dS", gps_local_time/1000);
				strcat(gps_test_info, strbuf);
			}
		#else
			sprintf(gps_test_info, "%02d\n", tracked);
			strcat(gps_test_info, strbuf);
			strcat(gps_test_info, "\n \n");
			
			lon = evt->pvt.longitude*1000000;
			lat = evt->pvt.latitude*1000000;
			sprintf(strbuf, "Longitude:   %d.%06d\nLatitude:    %d.%06d\n", lon/1000000, lon%1000000, lat/1000000, lat%1000000);
			strcat(gps_test_info, strbuf);
	
			if(gps_fix_time > 0)
			{
				sprintf(strbuf, "fix time:    %dS", gps_local_time/1000);
				strcat(gps_test_info, strbuf);
			}
		#endif
		
			gps_test_update_flag = true;
		}		
		else
		{
		#ifdef GPS_DEBUG
			s32_t lon,lat;

			lon = evt->pvt.longitude*1000000;
			lat = evt->pvt.latitude*1000000;
			sprintf(tmpbuf, "Longitude:%d.%06d,Latitude:%d.%06d", lon/1000000, lon%1000000, lat/1000000, lat%1000000);
			LOGD("%s",tmpbuf);
		#endif	
			memcpy(&gps_pvt_data, &(evt->pvt), sizeof(evt->pvt));
		}
		break;
		
	case GPS_EVT_NMEA:
		/* Don't spam logs */
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_NMEA");
	#endif
		break;
		
	case GPS_EVT_NMEA_FIX:
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_NMEA_FIX");
	#endif
		if(gps_fix_time == 0)
		{
			gps_fix_time = k_uptime_get();
			gps_local_time = gps_fix_time-gps_start_time;
		}
		
		if(!test_gps_flag)
		{
		#ifdef GPS_DEBUG
			LOGD("Position fix with NMEA data, fix time:%d", gps_local_time);
			LOGD("NMEA:%s", evt->nmea.buf);
		#endif
			APP_Ask_GPS_off();

			if(k_timer_remaining_get(&app_wait_gps_timer) > 0)
				k_timer_stop(&app_wait_gps_timer);

			k_timer_start(&app_send_gps_timer, K_MSEC(1000), NULL);
		}
		break;
		
	case GPS_EVT_OPERATION_BLOCKED:
	#ifdef GPS_DEBUG	
		LOGD("GPS_EVT_OPERATION_BLOCKED");
	#endif
		break;
		
	case GPS_EVT_OPERATION_UNBLOCKED:
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_OPERATION_UNBLOCKED");
	#endif
		break;
		
	case GPS_EVT_AGPS_DATA_NEEDED:
	#ifdef GPS_DEBUG
		LOGD("GPS_EVT_AGPS_DATA_NEEDED");
	#endif
	#if defined(CONFIG_NRF_CLOUD_AGPS)
		k_work_submit_to_queue(app_work_q,
				       &send_agps_request_work);
	#endif
		break;
		
	case GPS_EVT_ERROR:
	#ifdef GPS_DEBUG	
		LOGD("GPS_EVT_ERROR");
	#endif
		break;
		
	default:
		break;
	}
}

void GPS_init(struct k_work_q *work_q)
{
	app_work_q = work_q;

#if defined(CONFIG_NRF_CLOUD_AGPS)
	k_work_init(&send_agps_request_work, send_agps_request);
#endif

	gps_control_init(app_work_q, gps_handler);
}

void GPSMsgProcess(void)
{
	if(gps_test_start_flag)
	{
		gps_test_start_flag = false;
		test_gps_on();
	}
	if(gps_on_flag)
	{
		gps_on_flag = false;
		gps_on();
	}
	
	if(gps_off_flag)
	{
		gps_off_flag = false;
		gps_off();
	}

	if(gps_send_data_flag)
	{
		gps_send_data_flag = false;
		if(gps_local_time > 0)
		{
			APP_GPS_data_send(true);
		}
		else
		{
			APP_GPS_data_send(false);
		}
	}
	
	if(gps_test_update_flag)
	{
		gps_test_update_flag = false;
		gps_test_update();
	}
	
	if(gps_is_on)
	{
		k_sleep(K_MSEC(50));
	}
}