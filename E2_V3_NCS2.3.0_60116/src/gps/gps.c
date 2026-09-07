/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <date_time.h>
#include <logger.h>
#include "screen.h"
#include "gps.h"

//#define GPS_DEBUG

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE) || defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
static struct k_work_q gnss_work_q;

//#define CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND
#define GNSS_WORKQ_THREAD_STACK_SIZE 2304
#define GNSS_WORKQ_THREAD_PRIORITY   5

K_THREAD_STACK_DEFINE(gnss_workq_stack_area, GNSS_WORKQ_THREAD_STACK_SIZE);
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE || CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
#include "assistance.h"

static struct nrf_modem_gnss_agps_data_frame last_agps;
static struct k_work agps_data_get_work;
static volatile bool requesting_assistance;
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
static struct k_work_delayable ttff_test_got_fix_work;
static struct k_work_delayable ttff_test_prepare_work;
static struct k_work ttff_test_start_work;
static uint32_t time_to_fix;
static uint32_t time_blocked;
#endif

static struct k_work_q *app_work_q;
static struct k_work gps_data_get_work;
static void gps_data_get_work_fn(struct k_work *item);

static void APP_Ask_GPS_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(app_wait_gps_timer, APP_Ask_GPS_Data_timerout, NULL);
static void APP_Send_GPS_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(app_send_gps_timer, APP_Send_GPS_Data_timerout, NULL);

static bool gps_is_on = false;

static int64_t gps_start_time=0,gps_fix_time=0,gps_local_time=0;

bool gps_on_flag = false;
bool gps_off_flag = false;
bool ble_wait_gps = false;
bool sos_wait_gps = false;
#ifdef CONFIG_FALL_DETECT_SUPPORT
bool fall_wait_gps = false;
#endif
bool location_wait_gps = false;
bool test_gps_flag = false;
bool gps_test_update_flag = false;
bool gps_send_data_flag = false;

uint8_t gps_test_info[256] = {0};

static const char update_indicator[] = {'\\', '|', '/', '-'};

static struct nrf_modem_gnss_pvt_data_frame last_pvt;

K_MSGQ_DEFINE(nmea_queue, sizeof(struct nrf_modem_gnss_nmea_data_frame *), 10, 4);
static K_SEM_DEFINE(pvt_data_sem, 0, 1);
static K_SEM_DEFINE(time_sem, 0, 1);

static struct k_poll_event events[2] = 
{
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&pvt_data_sem, 0),
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&nmea_queue, 0),
};

BUILD_ASSERT(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS) ||
	     IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS) ||
	     IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS),
	     "CONFIG_LTE_NETWORK_MODE_LTE_M_GPS, "
	     "CONFIG_LTE_NETWORK_MODE_NBIOT_GPS or "
	     "CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS must be enabled");

static void gnss_event_handler(int event)
{
	int retval;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	switch (event)
	{
	case NRF_MODEM_GNSS_EVT_PVT:
		retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if(retval == 0)
		{
			k_sem_give(&pvt_data_sem);
		}
		break;

	#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	case NRF_MODEM_GNSS_EVT_FIX:
		/* Time to fix is calculated here, but it's printed from a delayed work to avoid
		 * messing up the NMEA output.
		 */
		time_to_fix = (k_uptime_get() - gps_start_time) / 1000;
		k_work_schedule_for_queue(&gnss_work_q, &ttff_test_got_fix_work, K_MSEC(100));
		k_work_schedule_for_queue(&gnss_work_q, &ttff_test_prepare_work, K_SECONDS(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_INTERVAL));
		break;
	#endif /* CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

	case NRF_MODEM_GNSS_EVT_NMEA:
		nmea_data = k_malloc(sizeof(struct nrf_modem_gnss_nmea_data_frame));
		if(nmea_data == NULL)
		{
		#ifdef GPS_DEBUG
			LOGD("Failed to allocate memory for NMEA");
		#endif
			break;
		}

		retval = nrf_modem_gnss_read(nmea_data,
					     sizeof(struct nrf_modem_gnss_nmea_data_frame),
					     NRF_MODEM_GNSS_DATA_NMEA);
		if(retval == 0)
		{
			retval = k_msgq_put(&nmea_queue, &nmea_data, K_NO_WAIT);
		}

		if(retval != 0)
		{
			k_free(nmea_data);
		}
		break;

	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
	#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
		if(!test_gps_flag)
		{
			retval = nrf_modem_gnss_read(&last_agps,
						     sizeof(last_agps),
						     NRF_MODEM_GNSS_DATA_AGPS_REQ);
			if(retval == 0)
			{
				k_work_submit_to_queue(&gnss_work_q, &agps_data_get_work);
			}
		}
	#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */
		break;

	default:
		break;
	}
}

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
static void agps_data_get_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

#if defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_SUPL)
	/* SUPL doesn't usually provide satellite real time integrity information. If GNSS asks
	 * only for satellite integrity, the request should be ignored.
	 */
	if (last_agps.sv_mask_ephe == 0 
		&& last_agps.sv_mask_alm == 0 
		&& last_agps.data_flags == NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST)
	{
	#ifdef GPS_DEBUG
		LOGD("Ignoring assistance request for only satellite integrity");
	#endif
		return;
	}
#endif/* CONFIG_GNSS_SAMPLE_ASSISTANCE_SUPL */

#if defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL)
	/* With minimal assistance, the request should be ignored if no GPS time or position
	 * is requested.
	 */
	if (!(last_agps.data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST)
		&& !(last_agps.data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST))
	{
	#ifdef GPS_DEBUG
		LOGD("Ignoring assistance request because no GPS time or position is requested");
	#endif
		return;
	}
#endif /* CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL */

	requesting_assistance = true;

#ifdef GPS_DEBUG
	LOGD("Assistance data needed, ephe 0x%08x, alm 0x%08x, flags 0x%02x",
		last_agps.sv_mask_ephe,
		last_agps.sv_mask_alm,
		last_agps.data_flags);
#endif

#if defined(CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND)
	lte_connect();
#endif /* CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND */

	err = assistance_request(&last_agps);
	if(err)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to request assistance data");
	#endif
	}

#if defined(CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND)
	lte_disconnect();
#endif /* CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND */

	requesting_assistance = false;
}
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
static void ttff_test_got_fix_work_fn(struct k_work *item)
{
#ifdef GPS_DEBUG
	LOGD("Time to fix: %u", time_to_fix);
#endif
	if(time_blocked > 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Time GNSS was blocked by LTE: %u", time_blocked);
	#endif
	}

#ifdef GPS_DEBUG
	LOGD("Sleeping for %u seconds", CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_INTERVAL);
#endif
}

static int ttff_test_force_cold_start(void)
{
	int err;
	uint32_t delete_mask;

#ifdef GPS_DEBUG
	LOGD("Deleting GNSS data");
#endif

	/* Delete everything else except the TCXO offset. */
	delete_mask = NRF_MODEM_GNSS_DELETE_EPHEMERIDES |
		      NRF_MODEM_GNSS_DELETE_ALMANACS |
		      NRF_MODEM_GNSS_DELETE_IONO_CORRECTION_DATA |
		      NRF_MODEM_GNSS_DELETE_LAST_GOOD_FIX |
		      NRF_MODEM_GNSS_DELETE_GPS_TOW |
		      NRF_MODEM_GNSS_DELETE_GPS_WEEK |
		      NRF_MODEM_GNSS_DELETE_UTC_DATA |
		      NRF_MODEM_GNSS_DELETE_GPS_TOW_PRECISION;

	/* With minimal assistance, we want to keep the factory almanac. */
	if(IS_ENABLED(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL))
	{
		delete_mask &= ~NRF_MODEM_GNSS_DELETE_ALMANACS;
	}

	err = nrf_modem_gnss_nv_data_delete(delete_mask);
	if(err)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to delete GNSS data");
	#endif
		return -1;
	}

	return 0;
}

static void ttff_test_prepare_work_fn(struct k_work *item)
{
	/* Make sure GNSS is stopped before next start. */
	nrf_modem_gnss_stop();

	if(IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_COLD_START))
	{
		if(ttff_test_force_cold_start() != 0)
		{
			return;
		}
	}

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
	if(IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_COLD_START))
	{
		/* All A-GPS data is always requested before GNSS is started. */
		last_agps.sv_mask_ephe = 0xffffffff;
		last_agps.sv_mask_alm = 0xffffffff;
		last_agps.data_flags =
			NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST |
			NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST |
			NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST |
			NRF_MODEM_GNSS_AGPS_POSITION_REQUEST |
			NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST;

		k_work_submit_to_queue(&gnss_work_q, &agps_data_get_work);
	}
	else
	{
		/* Start and stop GNSS to trigger possible A-GPS data request. If new A-GPS
		 * data is needed it is fetched before GNSS is started.
		 */
		nrf_modem_gnss_start();
		nrf_modem_gnss_stop();
	}
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

	k_work_submit_to_queue(&gnss_work_q, &ttff_test_start_work);
}

static void ttff_test_start_work_fn(struct k_work *item)
{
#ifdef GPS_DEBUG
	LOGD("Starting GNSS");
#endif

    if(nrf_modem_gnss_start() != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to start GNSS");
	#endif
		return;
	}

	gps_start_time = k_uptime_get();
	time_blocked = 0;
}
#endif /* CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

static void date_time_evt_handler(const struct date_time_evt *evt)
{
	k_sem_give(&time_sem);
}

static int modem_init(void)
{
    uint8_t tmpbuf[128] = {0};

 	if(strlen(CONFIG_GNSS_SAMPLE_AT_MAGPIO) > 0)
	{
		if(nrf_modem_at_printf("%s", CONFIG_GNSS_SAMPLE_AT_MAGPIO) != 0)
		{
		#ifdef GPS_DEBUG
			LOGD("Failed to set MAGPIO configuration");
		#endif
			return -1;
		}
	}

	if(strlen(CONFIG_GNSS_SAMPLE_AT_COEX0) > 0)
	{
		if(nrf_modem_at_printf("%s", CONFIG_GNSS_SAMPLE_AT_COEX0) != 0)
		{
		#ifdef GPS_DEBUG
			LOGD("Failed to set COEX0 configuration");
		#endif
			return -1;
		}
	}

	return 0;
}

static int gps_work_init(void)
{
	int err = 0;

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE) || defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	struct k_work_queue_config cfg = {
		.name = "gnss_work_q",
		.no_yield = false
	};

	k_work_queue_start(
		&gnss_work_q,
		gnss_workq_stack_area,
		K_THREAD_STACK_SIZEOF(gnss_workq_stack_area),
		GNSS_WORKQ_THREAD_PRIORITY,
		&cfg);
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE || CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

#if !defined(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)
	k_work_init(&agps_data_get_work, agps_data_get_work_fn);

	err = assistance_init(&gnss_work_q);
#endif /* !CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE */

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	k_work_init_delayable(&ttff_test_got_fix_work, ttff_test_got_fix_work_fn);
	k_work_init_delayable(&ttff_test_prepare_work, ttff_test_prepare_work_fn);
	k_work_init(&ttff_test_start_work, ttff_test_start_work_fn);
#endif /* CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */

	k_work_init(&gps_data_get_work, gps_data_get_work_fn);
	return err;
}

static int gnss_init_and_start(void)
{
	/* Configure GNSS. */
	if(nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to set GNSS event handler");
	#endif
		return -1;
	}

	/* Enable all supported NMEA messages. */
	uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_RMC_MASK |
			     NRF_MODEM_GNSS_NMEA_GGA_MASK |
			     NRF_MODEM_GNSS_NMEA_GLL_MASK |
			     NRF_MODEM_GNSS_NMEA_GSA_MASK |
			     NRF_MODEM_GNSS_NMEA_GSV_MASK;

	if(nrf_modem_gnss_nmea_mask_set(nmea_mask) != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to set GNSS NMEA mask");
	#endif
		return -1;
	}

	/* This use case flag should always be set. */
	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

	if(IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_PERIODIC) 
		&& !IS_ENABLED(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE))
	{
		/* Disable GNSS scheduled downloads when assistance is used. */
		use_case |= NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE;
	}

	if(IS_ENABLED(CONFIG_GNSS_SAMPLE_LOW_ACCURACY))
	{
		use_case |= NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
	}

	if(nrf_modem_gnss_use_case_set(use_case) != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to set GNSS use case");
	#endif
	}

#if defined(CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK)
	if(nrf_modem_gnss_elevation_threshold_set(CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK) != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to set elevation threshold");
	#endif
		return -1;
	}
#ifdef GPS_DEBUG
	LOGD("Set elevation threshold to %u", CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK);
#endif
#endif

#if defined(CONFIG_GNSS_SAMPLE_MODE_CONTINUOUS)
	/* Default to no power saving. */
	uint8_t power_mode = NRF_MODEM_GNSS_PSM_DISABLED;

#if defined(GNSS_SAMPLE_POWER_SAVING_MODERATE)
	power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_PERFORMANCE;
#elif defined(GNSS_SAMPLE_POWER_SAVING_HIGH)
	power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_POWER;
#endif

	if(nrf_modem_gnss_power_mode_set(power_mode) != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to set GNSS power saving mode");
	#endif
		return -1;
	}
#endif /* CONFIG_GNSS_SAMPLE_MODE_CONTINUOUS */

	/* Default to continuous tracking. */
	uint16_t fix_retry = 0;
	uint16_t fix_interval = 1;

#if defined(CONFIG_GNSS_SAMPLE_MODE_PERIODIC)
	fix_retry = CONFIG_GNSS_SAMPLE_PERIODIC_TIMEOUT;
	fix_interval = CONFIG_GNSS_SAMPLE_PERIODIC_INTERVAL;
#elif defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	/* Single fix for TTFF test mode. */
	fix_retry = 0;
	fix_interval = 0;
#endif

	if(nrf_modem_gnss_fix_retry_set(fix_retry) != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to set GNSS fix retry");
	#endif
		return -1;
	}

	if(nrf_modem_gnss_fix_interval_set(fix_interval) != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to set GNSS fix interval");
	#endif
		return -1;
	}

#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
	k_work_schedule_for_queue(&gnss_work_q, &ttff_test_prepare_work, K_NO_WAIT);
#else /* !CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST */
	if(nrf_modem_gnss_start() != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to start GNSS");
	#endif
		return -1;
	}
#endif

	return 0;
}

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked   = 0;
	uint8_t in_fix    = 0;
	uint8_t unhealthy = 0;

	for(int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i)
	{
		if(pvt_data->sv[i].sv > 0)
		{
			tracked++;

			if(pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX)
			{
				in_fix++;
			}

			if(pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY)
			{
				unhealthy++;
			}
		}
	}

#ifdef GPS_DEBUG
	LOGD("Tracking: %2d Using: %2d Unhealthy: %d\n", tracked, in_fix, unhealthy);
#endif
}

static void print_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
    uint8_t tmpbuf[256] = {0};

#ifdef GPS_DEBUG
    int32_t lon, lat;

    lon = pvt_data->longitude*1000000;
    lat = pvt_data->latitude*1000000;
    sprintf(tmpbuf, "Longitude:%d.%06d, Latitude:%d.%06d", lon/1000000, lon%1000000, lat/1000000, lat%1000000);
    LOGD("%s",tmpbuf);
	LOGD("Date:           %04u-%02u-%02u\n",
	       pvt_data->datetime.year,
	       pvt_data->datetime.month,
	       pvt_data->datetime.day);
	LOGD("Time (UTC):     %02u:%02u:%02u.%03u\n",
	       pvt_data->datetime.hour,
	       pvt_data->datetime.minute,
	       pvt_data->datetime.seconds,
	       pvt_data->datetime.ms);
#endif
}

static void gps_data_get_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	uint8_t cnt = 0;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	for(;;)
	{
		(void)k_poll(events, 2, K_FOREVER);

		if(events[0].state == K_POLL_STATE_SEM_AVAILABLE 
			&& k_sem_take(events[0].sem, K_NO_WAIT) == 0)
		{
			/* New PVT data available */

			if(!IS_ENABLED(CONFIG_GNSS_SAMPLE_NMEA_ONLY))
			{
				//print_satellite_stats(&last_pvt);
			#ifdef GPS_DEBUG
				LOGD("pvt");
			#endif

				if(test_gps_flag)
				{
					uint8_t i,tracked = 0,flag = 0,valid_sv = 0;
					uint8_t strbuf[256] = {0};
					int32_t lon,lat;
					
					memset(gps_test_info, 0x00, sizeof(gps_test_info));
					
					for(i=0;i<NRF_MODEM_GNSS_MAX_SATELLITES;i++)
					{
						uint8_t buf[256] = {0};
						
						if(last_pvt.sv[i].sv > 0)
						{
							tracked++;
						#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
							if(tracked<8)
							{
								if(flag == 0)
								{
									flag = 1;
									sprintf(buf, "%02d", last_pvt.sv[i].cn0/10);
								}
								else
								{
									sprintf(buf, "|%02d", last_pvt.sv[i].cn0/10);
								}
								
								strcat(strbuf, buf);
							}
						#else
						  #ifdef CONFIG_FACTORY_TEST_SUPPORT
							if(IsFTGPSTesting())
							{
								if(last_pvt.sv[i].cn0/10 >= 40)
									valid_sv++;
								
								if(flag == 0)
								{
									flag = 1;
									sprintf(buf, "%02d", last_pvt.sv[i].cn0/10);
								}
								else
								{
									sprintf(buf, "|%02d", last_pvt.sv[i].cn0/10);
								}
							}
							else
						  #endif
							{
								if(flag == 0)
								{
									flag = 1;
									sprintf(buf, "%02d|%02d", last_pvt.sv[i].sv, last_pvt.sv[i].cn0/10);
								}
								else
								{
									sprintf(buf, ";%02d|%02d", last_pvt.sv[i].sv, last_pvt.sv[i].cn0/10);
								}
						  	}
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

				#ifdef CONFIG_FACTORY_TEST_SUPPORT
					if(valid_sv >= 3)
						FTGPSStatusUpdate(true);
					else
						FTGPSStatusUpdate(false);
				#endif

					gps_test_update_flag = true;
				}

				if(last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED)
				{
				#ifdef GPS_DEBUG
					LOGD("GNSS operation blocked by LTE");
				#endif
				}
				
				if(last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME)
				{
				#ifdef GPS_DEBUG
					LOGD("Insufficient GNSS time windows");
				#endif
				}
				
				if(last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT)
				{
				#ifdef GPS_DEBUG
					LOGD("Sleep period(s) between PVT notifications");
				#endif
				}

				if(last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID)
				{
				#ifdef GPS_DEBUG
					LOGD("NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID");
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
					#endif
						APP_Ask_GPS_off();

						if(k_timer_remaining_get(&app_wait_gps_timer) > 0)
							k_timer_stop(&app_wait_gps_timer);

						k_timer_start(&app_send_gps_timer, K_MSEC(1000), K_NO_WAIT);
					}
					else
					{
						uint8_t strbuf[256] = {0};
						int32_t lon,lat;
						
					#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
						strcat(gps_test_info, "\n");	//2行没显示满，多换行一行
						if(gps_fix_time > 0)
						{
							sprintf(strbuf, "fix:%dS", gps_local_time/1000);
							strcat(gps_test_info, strbuf);
						}
					#else
						strcat(gps_test_info, "\n");
						
						lon = last_pvt.longitude*1000000;
						lat = last_pvt.latitude*1000000;
						sprintf(strbuf, "Lon:   %d.%06d\nLat:    %d.%06d\n", lon/1000000, abs(lon)%1000000, lat/1000000, abs(lat)%1000000);
						strcat(gps_test_info, strbuf);

					#ifdef CONFIG_FACTORY_TEST_SUPPORT
						if(IsFTGPSTesting())
						{
							//xb add 2023-03-15 ft gps test don't need show fix time
						}
						else
					#endif
						{
							if(gps_fix_time > 0)
							{
								sprintf(strbuf, "fix time:    %dS", gps_local_time/1000);
								strcat(gps_test_info, strbuf);
							}
						}
					#endif

					#ifdef CONFIG_FACTORY_TEST_SUPPORT
						FTGPSStatusUpdate(true);
					#endif

						gps_test_update_flag = true;
					}
					
				#ifdef GPS_DEBUG
					print_fix_data(&last_pvt);
				#endif
				}
				else
				{
				#ifdef GPS_DEBUG
					LOGD("Seconds since last fix: %lld", (k_uptime_get() - gps_start_time) / 1000);
				#endif
					cnt++;
				#ifdef GPS_DEBUG
					LOGD("Searching [%c]", update_indicator[cnt%4]);
				#endif
				}
			}
		#if defined(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST)
			else
			{
				/* Calculate the time GNSS has been blocked by LTE. */
				if(last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED)
				{
					time_blocked++;
				}
			}
		#endif/*CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST*/
		}
		if(events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE 
			&& k_msgq_get(events[1].msgq, &nmea_data, K_NO_WAIT) == 0)
		{
			/* New NMEA data available */
		#ifdef GPS_DEBUG
			LOGD("NMEA:%s", nmea_data->nmea_str);
		#endif

			k_free(nmea_data);
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;

		if(!gps_is_on)
			break;
	}
}

bool APP_GPS_data_send(bool fix_flag)
{
	bool ret = false;

#ifdef CONFIG_BLE_SUPPORT	
	if(ble_wait_gps)
	{
		APP_get_gps_data_reply(fix_flag, last_pvt);
		ble_wait_gps = false;
		ret = true;
	}
#endif

	if(sos_wait_gps)
	{
		sos_get_gps_data_reply(fix_flag, last_pvt);
		sos_wait_gps = false;
		ret = true;
	}

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_FALL_DETECT_SUPPORT)
	if(fall_wait_gps)
	{
		fall_get_gps_data_reply(fix_flag, last_pvt);
		fall_wait_gps = false;
		ret = true;
	}
#endif

	if(location_wait_gps)
	{
		location_get_gps_data_reply(fix_flag, last_pvt);
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
	static uint8_t time_init = false;

#if 1
	if(!gps_is_on)
	{
		gps_on_flag = true;
		k_timer_start(&app_wait_gps_timer, K_SECONDS(APP_WAIT_GPS_TIMEOUT), K_NO_WAIT);
	}
#else
	last_pvt.datetime.year = 2020;
	last_pvt.datetime.month = 11;
	last_pvt.datetime.day = 4;
	last_pvt.datetime.hour = 2;
	last_pvt.datetime.minute = 20;
	last_pvt.datetime.seconds = 40;
	last_pvt.longitude = 114.025254;
	last_pvt.latitude = 22.667808;
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

bool gps_is_working(void)
{
#ifdef GPS_DEBUG
	LOGD("gps_is_on:%d", gps_is_on);
#endif
	return gps_is_on;
}

void gps_turn_off(void)
{
#ifdef GPS_DEBUG
	LOGD("begin");
#endif

	nrf_modem_gnss_stop();
	gps_is_on = false;
}

void gps_off(void)
{
	if(!gps_is_on)
		return;

	gps_turn_off();
}

bool gps_turn_on(void)
{
	uint8_t cnt = 0;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	if(modem_init() != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to initialize modem");
	#endif
		return false;
	}

	if(gnss_init_and_start() != 0)
	{
	#ifdef GPS_DEBUG
		LOGD("Failed to initialize and start GNSS");
	#endif
		return false;
	}

	gps_is_on = true;
	gps_fix_time = 0;
	gps_local_time = 0;
	memset(&last_pvt, 0, sizeof(last_pvt));
	gps_start_time = k_uptime_get();

#ifdef GPS_DEBUG
	LOGD("begin");
#endif

	k_work_submit_to_queue(app_work_q, &gps_data_get_work);

	return true;
}

void gps_on(void)
{
	bool ret = false;
	
	if(gps_is_on)
		return;
	
	ret = gps_turn_on();
#ifdef CONFIG_FACTORY_TEST_SUPPORT
	if(!ret)
	{
		FTGPSStartFail();
	}
#endif
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
	gps_off_flag = true;
}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void FTStartGPS(void)
{
	test_gps_flag = true;
	gps_on();
}

void FTStopGPS(void)
{
	gps_off();
}
#endif

void GPS_init(struct k_work_q *work_q)
{
	app_work_q = work_q;
	gps_work_init();
}

void GPSTestInit(void)
{
	lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_GPS, LTE_LC_SYSTEM_MODE_PREFER_AUTO);
}

void GPSTestUnInit(void)
{
#if IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)
	lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_NBIOT_GPS, LTE_LC_SYSTEM_MODE_PREFER_AUTO);
#elif IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)
	lte_lc_system_mode_set(LTE_LC_SYSTEM_MODE_LTEM_GPS, LTE_LC_SYSTEM_MODE_PREFER_AUTO);
#endif
}

void GPSMsgProcess(void)
{
	if(gps_on_flag)
	{
		gps_on_flag = false;
		if(test_gps_flag)
		{
			SetModemTurnOff();
			GPSTestInit();
			SetModemTurnOn();
		}
		gps_on();
	}
	
	if(gps_off_flag)
	{
		gps_off_flag = false;
		gps_off();
		if(test_gps_flag)
		{
			SetModemTurnOff();
			GPSTestUnInit();
			SetModemTurnOn();
			test_gps_flag = false;
		}
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
		k_sleep(K_MSEC(5));
	}
}
