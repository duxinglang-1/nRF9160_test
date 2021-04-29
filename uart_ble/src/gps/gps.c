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
#include <drivers/gps.h>
#include <stdio.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include "lcd.h"
#include "uart_ble.h"
#include "Settings.h"
#include "nb.h"
#include "sos.h"
#include "gps_controller.h"
#include "esp8266.h"

#ifdef CONFIG_SUPL_CLIENT_LIB
#include <supl_os_client.h>
#include <supl_session.h>
#include "supl_support.h"
#endif

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(gps, CONFIG_LOG_DEFAULT_LEVEL);

//#define SHOW_LOG_IN_SCREEN		//xb add 20201029 将GPS测试状态LOG信息显示在屏幕上

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
static void gps_data_wait_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(gps_data_timer, gps_data_wait_timerout, NULL);

static u8_t cnt = 0;
static const char update_indicator[] = {'\\', '|', '/', '-'};
static const char at_commands_activate_gps[][31]  = 
{
	AT_XSYSTEMMODE_GPSON,
#ifdef CONFIG_BOARD_NRF9160_PCA10090NS
	AT_MAGPIO,
	AT_COEX0,
#endif
	AT_ACTIVATE_GPS
};
static const char at_commands_deactivate_gps[][31]  = 
{
	AT_DEACTIVATE_GPS,
#ifdef CONFIG_BOARD_NRF9160_PCA10090NS
	AT_COEX0_CANCEL,
	AT_MAGPIO_CANCEL,
#endif
	AT_XSYSTEMMODE_GPSOFF
};

static int gnss_fd;
static char nmea_strings[10][NRF_GNSS_NMEA_MAX_LEN];
static u32_t nmea_string_cnt;

static bool gps_is_inited = false;
static bool gps_is_on = false;
static bool update_terminal;

static u64_t fix_timestamp;
static s64_t gps_start_time;

static struct k_work_q *app_work_q;
static struct k_work send_agps_request_work;

static struct gps_nmea gps_data;


bool got_first_fix = false;
bool gps_data_incoming = false;
bool app_gps_on = false;
bool app_gps_off = false;
bool ble_wait_gps = false;
bool sos_wait_gps = false;
bool fall_wait_gps = false;
bool location_wait_gps = false;

nrf_gnss_data_frame_t last_fix = {0};

extern bool show_date_time_first;

static void set_gps_enable(const bool enable);

K_SEM_DEFINE(lte_ready, 0, 1);

#ifdef SHOW_LOG_IN_SCREEN
static u8_t tmpbuf[128] = {0};

static void show_infor(u8_t *strbuf)
{
	LCD_Clear(BLACK);
	LCD_ShowStringInRect(30,50,180,160,strbuf);
}
#endif

static int setup_modem(void)
{
	int i;

	for(i = 0; i < ARRAY_SIZE(at_commands_activate_gps); i++)
	{
		if(at_cmd_write(at_commands_activate_gps[i], NULL, 0, NULL) != 0)
		{
			return -1;
		}
	}

	return 0;
}

static int reset_modem(void)
{
	int i;

	for(i = 0; i < ARRAY_SIZE(at_commands_deactivate_gps); i++)
	{
		if(at_cmd_write(at_commands_deactivate_gps[i], NULL, 0, NULL) != 0)
		{
			return -1;
		}
	}

	return 0;
}

#ifdef CONFIG_SUPL_CLIENT_LIB
/* Accepted network statuses read from modem */
static const char status1[] = "+CEREG: 1";
static const char status2[] = "+CEREG:1";
static const char status3[] = "+CEREG: 5";
static const char status4[] = "+CEREG:5";

static void wait_for_lte(void *context, const char *response)
{
	if (!memcmp(status1, response, AT_CMD_SIZE(status1)) ||
		!memcmp(status2, response, AT_CMD_SIZE(status2)) ||
		!memcmp(status3, response, AT_CMD_SIZE(status3)) ||
		!memcmp(status4, response, AT_CMD_SIZE(status4))) {
		k_sem_give(&lte_ready);
	}
}

static int activate_lte(bool activate)
{
	if(activate)
	{
		if(at_cmd_write(AT_ACTIVATE_LTE, NULL, 0, NULL) != 0)
		{
			return -1;
		}

		at_notif_register_handler(NULL, wait_for_lte);
		if(at_cmd_write("AT+CEREG=2", NULL, 0, NULL) != 0)
		{
			return -1;
		}

		k_sem_take(&lte_ready, K_FOREVER);

		at_notif_deregister_handler(NULL, wait_for_lte);
		if(at_cmd_write("AT+CEREG=0", NULL, 0, NULL) != 0)
		{
			return -1;
		}
	}
	else
	{
		if(at_cmd_write(AT_DEACTIVATE_LTE, NULL, 0, NULL) != 0)
		{
			return -1;
		}
	}

	return 0;
}
#endif


static int gnss_ctrl(uint32_t ctrl)
{
	int retval;
	nrf_gnss_fix_retry_t fix_retry    = 0;
	nrf_gnss_fix_interval_t fix_interval = 1;
	nrf_gnss_delete_mask_t delete_mask  = 0;
	nrf_gnss_nmea_mask_t nmea_mask = NRF_GNSS_NMEA_GSV_MASK |
					       			NRF_GNSS_NMEA_GSA_MASK |
					       			NRF_GNSS_NMEA_GLL_MASK |
					       			NRF_GNSS_NMEA_GGA_MASK |
					       			NRF_GNSS_NMEA_RMC_MASK;

	if(ctrl == GNSS_INIT_AND_START)
	{
		gnss_fd = nrf_socket(NRF_AF_LOCAL,
				     NRF_SOCK_DGRAM,
				     NRF_PROTO_GNSS);

		if(gnss_fd >= 0)
		{
			LOG_INF("GPS Socket created\n");
		#ifdef SHOW_LOG_IN_SCREEN
			show_infor("GPS Socket created");
		#endif
		}
		else
		{
			LOG_INF("Could not init socket (err: %d)\n", gnss_fd);
		#ifdef SHOW_LOG_IN_SCREEN	
			sprintf(tmpbuf, "Could not init socket (err: %d)", gnss_fd);
			show_infor(tmpbuf);
		#endif
			return -1;
		}

		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_FIX_RETRY,
					&fix_retry,
					sizeof(fix_retry));
		if(retval != 0)
		{
			LOG_INF("Failed to set fix retry value\n");
		#ifdef SHOW_LOG_IN_SCREEN
			show_infor("Failed to set fix retry value");
		#endif
			return -1;
		}

		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_FIX_INTERVAL,
					&fix_interval,
					sizeof(fix_interval));
		if(retval != 0)
		{
			LOG_INF("Failed to set fix interval value\n");
		#ifdef SHOW_LOG_IN_SCREEN	
			show_infor("Failed to set fix interval value");
		#endif	
			return -1;
		}

		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_NMEA_MASK,
					&nmea_mask,
					sizeof(nmea_mask));
		if(retval != 0)
		{
			LOG_INF("Failed to set nmea mask\n");
		#ifdef SHOW_LOG_IN_SCREEN	
			show_infor("Failed to set nmea mask");
		#endif	
			return -1;
		}
	}

	if((ctrl == GNSS_INIT_AND_START) || (ctrl == GNSS_RESTART))
	{
		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_START,
					&delete_mask,
					sizeof(delete_mask));
		if(retval != 0)
		{
			LOG_INF("Failed to start GPS\n");
		#ifdef SHOW_LOG_IN_SCREEN	
			show_infor("Failed to start GPS");
		#endif	
			return -1;
		}
	}

	if(ctrl == GNSS_STOP)
	{
		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_STOP,
					&delete_mask,
					sizeof(delete_mask));
		if(retval != 0)
		{
			LOG_INF("Failed to stop GPS\n");
		#ifdef SHOW_LOG_IN_SCREEN
			show_infor("Failed to stop GPS");
		#endif
			return -1;
		}
	}

	if(ctrl == GNSS_CLOSE)
	{
		retval = nrf_close(gnss_fd);
		if(retval != 0)
		{
			LOG_INF("Failed to close GPS\n");
			return -1;
		}
	}

	return 0;
}

static int init_app(void)
{
	int retval;

	if(setup_modem() != 0)
	{
		LOG_INF("Failed to initialize modem\n");
	#ifdef SHOW_LOG_IN_SCREEN	
		show_infor("Failed to initialize modem");
	#endif	
		return -1;
	}

	retval = gnss_ctrl(GNSS_INIT_AND_START);

	return retval;
}

static void print_satellite_stats(nrf_gnss_data_frame_t *pvt_data)
{
	u8_t tracked = 0;
	u8_t in_fix = 0;
	u8_t unhealthy = 0;
	u8_t tmpbuf[512] = {0};
	int i;

	for(i = 0; i < NRF_GNSS_MAX_SATELLITES; ++i)
	{
		if ((pvt_data->pvt.sv[i].sv > 0) && (pvt_data->pvt.sv[i].sv < 33))
		{
			tracked++;

			if (pvt_data->pvt.sv[i].flags & NRF_GNSS_SV_FLAG_USED_IN_FIX)
			{
				in_fix++;
			}

			if (pvt_data->pvt.sv[i].flags & NRF_GNSS_SV_FLAG_UNHEALTHY)
			{
				unhealthy++;
			}
		}
	}

	LOG_INF("Tracking: %d Using: %d Unhealthy: %d", tracked, in_fix, unhealthy);
	LOG_INF("\nSeconds since last fix %lld\n", (k_uptime_get() - fix_timestamp) / 1000);
#ifdef SHOW_LOG_IN_SCREEN
	sprintf(tmpbuf, "Tracking:%d,Using:%d,Unhealthy:%d,Seconds since last fix:%lld", 
					tracked, in_fix, unhealthy, (k_uptime_get() - fix_timestamp) / 1000);
	show_infor(tmpbuf);
#endif	
}

static void print_pvt_data(nrf_gnss_data_frame_t *pvt_data)
{
	u8_t tmpbuf[512] = {0};

	LCD_Fill(0,20,240,200,BLACK);
	
	LOG_INF("Longitude:  %f\n", pvt_data->pvt.longitude);
	LOG_INF("Latitude:   %f\n", pvt_data->pvt.latitude);
	LOG_INF("Altitude:   %f\n", pvt_data->pvt.altitude);
	LOG_INF("Speed:      %f\n", pvt_data->pvt.speed);
	LOG_INF("Heading:    %f\n", pvt_data->pvt.heading);
	LOG_INF("Date:       %02u-%02u-%02u", pvt_data->pvt.datetime.year,
					       					pvt_data->pvt.datetime.month,
					       					pvt_data->pvt.datetime.day);
	LOG_INF("Time (UTC): %02u:%02u:%02u", pvt_data->pvt.datetime.hour,
					       					pvt_data->pvt.datetime.minute,
					      					pvt_data->pvt.datetime.seconds);
#ifdef SHOW_LOG_IN_SCREEN
	sprintf(tmpbuf, "Longitude:  %f\nLatitude:   %f\nAltitude:   %f\nSpeed:      %f\nHeading:    %f\nDate:       %02u-%02u-%02u\nTime (UTC): %02u:%02u:%02u", 
					pvt_data->pvt.longitude,
					pvt_data->pvt.latitude,
					pvt_data->pvt.altitude,
					pvt_data->pvt.speed,
					pvt_data->pvt.heading,
					pvt_data->pvt.datetime.year,
					pvt_data->pvt.datetime.month,
					pvt_data->pvt.datetime.day,
					pvt_data->pvt.datetime.hour,
					pvt_data->pvt.datetime.minute,
					pvt_data->pvt.datetime.seconds);
	show_infor(tmpbuf);
#endif
}

static void print_nmea_data(void)
{
	int i;
	
	LOG_INF("[%s] :%d\n", __func__, nmea_string_cnt);
	
	for(i = 0; i < nmea_string_cnt; ++i)
	{
		LOG_INF("%s", nmea_strings[i]);
	}
}

int process_gps_data(nrf_gnss_data_frame_t *gps_data)
{
	int retval;

	retval = nrf_recv(gnss_fd,
			  gps_data,
			  sizeof(nrf_gnss_data_frame_t),
			  NRF_MSG_DONTWAIT);

	LOG_INF("[%s] :%d\n", __func__, retval);
	if(retval > 0) 
	{	
		switch (gps_data->data_id)
		{
		case NRF_GNSS_PVT_DATA_ID:
			if((gps_data->pvt.flags & NRF_GNSS_PVT_FLAG_FIX_VALID_BIT)	== NRF_GNSS_PVT_FLAG_FIX_VALID_BIT)
			{
				if (!got_first_fix)
				{
					got_first_fix = true;
				}

				fix_timestamp = k_uptime_get();
				memcpy(&last_fix,
				       gps_data,
				       sizeof(nrf_gnss_data_frame_t));

				nmea_string_cnt = 0;
				update_terminal = true;
			}
			print_nmea_data();
			nmea_string_cnt = 0;
			break;

		case NRF_GNSS_NMEA_DATA_ID:
			if (nmea_string_cnt < 10) 
			{
				memset(nmea_strings[nmea_string_cnt],
				       0x00,
				       NRF_GNSS_NMEA_MAX_LEN);
				memcpy(nmea_strings[nmea_string_cnt++],
				       gps_data->nmea,
				       retval);
			}
			break;

		case NRF_GNSS_AGPS_DATA_ID:
		#ifdef CONFIG_SUPL_CLIENT_LIB
			LOG_INF("\033[1;1H");
			LOG_INF("\033[2J");
			LOG_INF("New AGPS data requested, contacting SUPL server, flags %d\n",
			       gps_data->agps.data_flags);
		#ifdef SHOW_LOG_IN_SCREEN
			sprintf(tmpbuf, "New AGPS data requested, contacting SUPL server, flags %d", gps_data->agps.data_flags);
			show_infor(tmpbuf);
		#endif
			gnss_ctrl(GNSS_STOP);
			activate_lte(true);
			LOG_INF("Established LTE link\n");
		#ifdef SHOW_LOG_IN_SCREEN
			show_infor("Established LTE link");
		#endif	
			if(open_supl_socket() == 0)
			{
				printf("Starting SUPL session\n");
			#ifdef SHOW_LOG_IN_SCREEN
				show_infor("Starting SUPL session");
			#endif	
				supl_session(&gps_data->agps);
				LOG_INF("Done\n");
			#ifdef SHOW_LOG_IN_SCREEN
				show_infor("Done");
			#endif	
				close_supl_socket();
			}
			activate_lte(false);
			gnss_ctrl(GNSS_RESTART);
			k_sleep(K_MSEC(2000));
		#endif
			break;

		default:
			break;
		}
	}

	return retval;
}

#ifdef CONFIG_SUPL_CLIENT_LIB
int inject_agps_type(void *agps,
		     size_t agps_size,
		     nrf_gnss_agps_data_type_t type,
		     void *user_data)
{
	ARG_UNUSED(user_data);
	int retval = nrf_sendto(gnss_fd,
				agps,
				agps_size,
				0,
				&type,
				sizeof(type));

	if(retval != 0)
	{
		LOG_INF("Failed to send AGNSS data, type: %d (err: %d)\n",
		       type,
		       errno);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "Failed to send AGNSS data, type: %d (err: %d)", type, errno);
		show_infor(tmpbuf);
	#endif	
		return -1;
	}

	LOG_INF("Injected AGPS data, flags: %d, size: %d\n", type, agps_size);
#ifdef SHOW_LOG_IN_SCREEN
	sprintf(tmpbuf, "Injected AGPS data, flags: %d, size: %d", type, agps_size);
	show_infor(tmpbuf);
#endif

	return 0;
}
#endif

bool APP_GPS_data_send(bool fix_flag)
{
	bool ret = false;
	
	if(ble_wait_gps)
	{
		APP_get_gps_data_reply(fix_flag, last_fix.pvt);
		ble_wait_gps = false;
		ret = true;
	}

	if(sos_wait_gps)
	{
		sos_get_gps_data_reply(fix_flag, last_fix.pvt);
		sos_wait_gps = false;
		ret = true;
	}

	if(fall_wait_gps)
	{
		fall_get_gps_data_reply(fix_flag, last_fix.pvt);
		fall_wait_gps = false;
		ret = true;
	}

	if(location_wait_gps)
	{
		location_get_gps_data_reply(fix_flag, last_fix.pvt);
		location_wait_gps = false;
		ret = true;
	}

	return ret;
}

void APP_Ask_GPS_Data_timerout(struct k_timer *timer)
{
	u8_t str_gps[20] = {0};
	u32_t tmp1;
	double tmp2;

	app_gps_off = true;

	APP_GPS_data_send(false);
}

void APP_Ask_GPS_Data(void)
{
	static u8_t time_init = false;

#if 0
	if(!app_gps_on)
	{
		app_gps_on = true;
		k_timer_start(&app_wait_gps_timer, K_MSEC(3*60*1000), NULL);
	}
#else
	last_fix.pvt.datetime.year = 2020;
	last_fix.pvt.datetime.month = 11;
	last_fix.pvt.datetime.day = 4;
	last_fix.pvt.datetime.hour = 2;
	last_fix.pvt.datetime.minute = 20;
	last_fix.pvt.datetime.seconds = 40;
	last_fix.pvt.longitude = 114.025254;
	last_fix.pvt.latitude = 22.667808;

	APP_Ask_GPS_Data_timerout(NULL);
	//k_timer_start(&app_wait_gps_timer, K_MSEC(1*60*1000), NULL);
#endif
}

void APP_Ask_GPS_off(void)
{
	app_gps_off = true;
}

void gps_data_receive(void)
{
	nrf_gnss_data_frame_t gps_data;

	do
	{
		/* Loop until we don't have more
		 * data to read
		 */
	}while(process_gps_data(&gps_data) > 0);

	if(!got_first_fix)
	{
		cnt++;
		LOG_INF("\033[1;1H");
		LOG_INF("\033[2J");
		print_satellite_stats(&gps_data);
		LOG_INF("\nScanning [%c] ",
				update_indicator[cnt%4]);
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "Scanning [%c] ", update_indicator[cnt%4]);
		show_infor(tmpbuf);
	#endif	
	}

	if(((k_uptime_get() - fix_timestamp) >= 1) && (got_first_fix))
	{
		LOG_INF("\033[1;1H");
		LOG_INF("\033[2J");

		print_satellite_stats(&gps_data);

		LOG_INF("---------------------------------\n");
		print_pvt_data(&last_fix);
		LOG_INF("\n");
		print_nmea_data();
		LOG_INF("---------------------------------");

		update_terminal = false;

		if(((k_uptime_get() - fix_timestamp) >= 5))
		{
			if(k_timer_remaining_get(&app_wait_gps_timer) > 0)
				k_timer_stop(&app_wait_gps_timer);
			if(k_timer_remaining_get(&gps_data_timer) > 0)
				k_timer_stop(&gps_data_timer);

			if(APP_GPS_data_send(true))
			{
				APP_Ask_GPS_off();
				return;
			}
		}
	}

	if(!gps_is_on)
	{
	#ifdef SHOW_LOG_IN_SCREEN
		show_infor("GPS is been turn off, return");
	#endif
		return;
	}
	
	print_nmea_data();
}

void gps_data_wait_timerout(struct k_timer *timer)
{
	gps_data_incoming = true;
}

void gpsinit(void)
{
	if(init_app() != 0)
	{
		return;
	}

	gps_is_inited = true;
}

void gps_restart(void)
{
	gnss_ctrl(GNSS_RESTART);
}

void gps_off(void)
{
#if 1
	set_gps_enable(false);
#else

	if(!gps_is_on)
	{
		LOG_INF("gps is been truned off\n");
	#ifdef SHOW_LOG_IN_SCREEN	
		show_infor("gps is been truned off");
	#endif

		GoBackHistoryScreen();
		return;
	}
	
	gps_is_on = false;
	got_first_fix = false;
	gps_data_incoming = false;

	k_timer_stop(&gps_data_timer);
	k_timer_stop(&app_wait_gps_timer);
	
	gnss_ctrl(GNSS_STOP);
	//gnss_ctrl(GNSS_CLOSE);

	//if(reset_modem() != 0)
	//{
	//	LOG_INF("Failed to reset modem\n");
	//#ifdef SHOW_LOG_IN_SCREEN	
	//	show_infor("Failed to reset modem");
	//#endif	
	//}

	GoBackHistoryScreen();
#endif	
}

void gps_on(void)
{
	u8_t tmpbuf[128] = {0};

#if 1
	set_gps_enable(true);
#else


#ifdef CONFIG_SUPL_CLIENT_LIB
	static struct supl_api supl_api = {
		.read       = supl_read,
		.write      = supl_write,
		.handler    = inject_agps_type,
		.logger     = supl_logger,
		.counter_ms = k_uptime_get
	};
#endif

	if(gps_is_on)
	{
		LOG_INF("gps is been truned on\n");
	#ifdef SHOW_LOG_IN_SCREEN	
		show_infor("gps is been truned on");
	#endif
		return;
	}

	EnterGPSTestScreen();

	LOG_INF("Staring GPS application\n");
#ifdef SHOW_LOG_IN_SCREEN
	show_infor("Staring GPS application");
#endif

	if(gps_is_inited == false)
	{
		gpsinit();
	}
	else
	{
		gps_restart();
	}

	cnt = 0;
	gps_is_on = true;
	
#ifdef CONFIG_SUPL_CLIENT_LIB
	int rc = supl_init(&supl_api);
	if (rc != 0)
	{
		//return;
	}
#endif

	LOG_INF("Getting GPS data...\n");
#ifdef SHOW_LOG_IN_SCREEN
	show_infor("Getting GPS data...");
#endif

	k_timer_start(&gps_data_timer, K_MSEC(500), K_MSEC(1000));
#endif
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
	if(gps_data_incoming)
	{
		gps_data_incoming = false;
		gps_data_receive();
	}
}

void test_gps(void)
{
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
		LOG_INF("Starting GPS");
		k_work_submit_to_queue(app_work_q,
				       &send_agps_request_work);
		gps_control_start(K_NO_WAIT);

		gps_start_time = k_uptime_get();
	}
	else
	{
		LOG_INF("Stopping GPS");
		gps_control_stop(K_NO_WAIT);
	}
}

static void send_agps_request(struct k_work *work)
{
	ARG_UNUSED(work);

#if defined(CONFIG_NRF_CLOUD_AGPS)
	int err;
	static s64_t last_request_timestamp;

	if((last_request_timestamp != 0) &&
	    (k_uptime_get() - last_request_timestamp < K_HOURS(1)))
	{
		LOG_WRN("A-GPS request was sent less than 1 hour ago");
		return;
	}

	LOG_INF("Sending A-GPS request");

	err = nrf_cloud_agps_request_all();
	if(err)
	{
		LOG_ERR("A-GPS request failed, error: %d", err);
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
		LOG_INF("GPS_EVT_SEARCH_STARTED");
		gps_control_set_active(true);
		break;
		
	case GPS_EVT_SEARCH_STOPPED:
		LOG_INF("GPS_EVT_SEARCH_STOPPED");
		gps_control_set_active(false);
		break;
		
	case GPS_EVT_SEARCH_TIMEOUT:
		LOG_INF("GPS_EVT_SEARCH_TIMEOUT");
		gps_control_set_active(false);
		break;
		
	case GPS_EVT_PVT:
		/* Don't spam logs */
		LOG_INF("GPS_EVT_PVT");
		
	#ifdef SHOW_LOG_IN_SCREEN
		sprintf(tmpbuf, "Longitude:  %f\nLatitude:   %f\nAltitude:   %f\nSpeed:      %f\nHeading:    %f\nDate:       %02d-%02d-%02d\nTime (UTC): %02d:%02d:%02d", 
					evt->pvt.longitude,
					evt->pvt.latitude,
					evt->pvt.altitude,
					evt->pvt.speed,
					evt->pvt.heading,
					evt->pvt.datetime.year,
					evt->pvt.datetime.month,
					evt->pvt.datetime.day,
					evt->pvt.datetime.hour,
					evt->pvt.datetime.minute,
					evt->pvt.datetime.seconds);	
		show_infor(tmpbuf);
	#else
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
		LOG_INF("Time (UTC): %02u:%02u:%02u", evt->pvt.datetime.hour,
					       					  evt->pvt.datetime.minute,
					      					  evt->pvt.datetime.seconds);
	#endif		
		break;
	
	case GPS_EVT_PVT_FIX:
		LOG_INF("GPS_EVT_PVT_FIX");
		sprintf(tmpbuf, "Longitude:%f, Latitude:%f\n", evt->pvt.longitude, evt->pvt.latitude);
		LOG_INF("%s",tmpbuf);
		break;
		
	case GPS_EVT_NMEA:
		/* Don't spam logs */
		LOG_INF("NMEA:%s\n", evt->nmea.buf);
		break;
		
	case GPS_EVT_NMEA_FIX:
		LOG_INF("Position fix with NMEA data, fix time:%d\n", k_uptime_get()-gps_start_time);
		LOG_INF("NMEA:%s\n", evt->nmea.buf);

		memcpy(gps_data.buf, evt->nmea.buf, evt->nmea.len);
		gps_data.len = evt->nmea.len;

		gps_control_set_active(false);
		break;
		
	case GPS_EVT_OPERATION_BLOCKED:
		LOG_INF("GPS_EVT_OPERATION_BLOCKED");
		break;
		
	case GPS_EVT_OPERATION_UNBLOCKED:
		LOG_INF("GPS_EVT_OPERATION_UNBLOCKED");
		break;
		
	case GPS_EVT_AGPS_DATA_NEEDED:
		LOG_INF("GPS_EVT_AGPS_DATA_NEEDED");
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