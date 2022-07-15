/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/util.h>
#include <drivers/gps.h>
#include <modem/lte_lc.h>
#include "gps_controller.h"
#include "logger.h"

//#define GPS_CTR_DEBUG

/* Structure to hold GPS work information */
static struct device *gps_dev;
static atomic_t gps_is_enabled;
static atomic_t gps_is_active;
static struct k_work_q *app_work_q;
static struct k_delayed_work start_work;
static struct k_delayed_work stop_work;
static int gps_reporting_interval_seconds;

extern bool test_gps_flag;

static void start(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;
	struct gps_config gps_cfg = {
		.nav_mode = GPS_NAV_MODE_SINGLE_FIX,
		.power_mode = GPS_POWER_MODE_PERFORMANCE,
		.timeout = CONFIG_GPS_CONTROL_FIX_TRY_TIME,
		.interval = CONFIG_GPS_CONTROL_FIX_TRY_TIME +
			gps_reporting_interval_seconds
	};

	if(gps_dev == NULL)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("GPS controller is not initialized properly");
	#endif
		return;
	}

#ifdef CONFIG_GPS_CONTROL_PSM_ENABLE_ON_START
#ifdef GPS_CTR_DEBUG
	LOGD("Enabling PSM");
#endif
	err = lte_lc_psm_req(true);
	if(err)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("PSM request failed, error: %d", err);
	#endif
	}
	else
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("PSM enabled");
	#endif
	}
#endif /* CONFIG_GPS_CONTROL_PSM_ENABLE_ON_START */

	if(test_gps_flag)
	{
		gps_cfg.nav_mode = GPS_NAV_MODE_CONTINUOUS;
		gps_cfg.power_mode = GPS_POWER_MODE_DISABLED;
	}
	
	err = gps_start(gps_dev, &gps_cfg);
	if(err)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("Failed to enable GPS, error: %d", err);
	#endif
		return;
	}

	atomic_set(&gps_is_enabled, 1);
	gps_control_set_active(true);

#ifdef GPS_CTR_DEBUG
	LOGD("GPS started successfully. Searching for satellites ");
#endif
#if !defined(CONFIG_GPS_SIM)
#if IS_ENABLED(CONFIG_GPS_START_ON_MOTION)
#ifdef GPS_CTR_DEBUG
	LOGD("or as soon as %d seconds later when movement occurs.", CONFIG_GPS_CONTROL_FIX_CHECK_INTERVAL);
#endif
#endif
#endif
}

static void stop(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	if(gps_dev == NULL)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("GPS controller is not initialized");
	#endif
		return;
	}

#ifdef CONFIG_GPS_CONTROL_PSM_DISABLE_ON_STOP
#ifdef GPS_CTR_DEBUG
	LOGD("Disabling PSM");
#endif
	err = lte_lc_psm_req(false);
	if(err)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("PSM mode could not be disabled, error: %d", err);
	#endif
	}
#endif /* CONFIG_GPS_CONTROL_PSM_DISABLE_ON_STOP */

	err = gps_stop(gps_dev);
	if(err)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("Failed to disable GPS, error: %d", err);
	#endif
		return;
	}

	atomic_set(&gps_is_enabled, 0);
	gps_control_set_active(false);
#ifdef GPS_CTR_DEBUG
	LOGD("GPS operation was stopped");
#endif
}

bool gps_control_is_enabled(void)
{
	return atomic_get(&gps_is_enabled);
}

bool gps_control_is_active(void)
{
	return atomic_get(&gps_is_active);
}

bool gps_control_set_active(bool active)
{
	return atomic_set(&gps_is_active, active ? 1 : 0);
}

void gps_control_start(u32_t delay_ms)
{
	k_delayed_work_submit_to_queue(app_work_q, &start_work, delay_ms);
}

void gps_control_stop(u32_t delay_ms)
{
	k_delayed_work_submit_to_queue(app_work_q, &stop_work, delay_ms);
}

int gps_control_get_gps_reporting_interval(void)
{
	return gps_reporting_interval_seconds;
}

/** @brief Configures and starts the GPS device. */
int gps_control_init(struct k_work_q *work_q, gps_event_handler_t handler)
{
	int err;
	static bool is_init;

	if (is_init)
	{
		return -EALREADY;
	}

	if((work_q == NULL) || (handler == NULL))
	{
		return -EINVAL;
	}

	app_work_q = work_q;

	gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	if (gps_dev == NULL)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("Could not get %s device", CONFIG_GPS_DEV_NAME);
	#endif
		return -ENODEV;
	}

	err = gps_init(gps_dev, handler);
	if(err)
	{
	#ifdef GPS_CTR_DEBUG
		LOGD("Could not initialize GPS, error: %d", err);
	#endif
		return err;
	}

	k_delayed_work_init(&start_work, start);
	k_delayed_work_init(&stop_work, stop);

#if !defined(CONFIG_GPS_SIM)
	gps_reporting_interval_seconds =
		IS_ENABLED(CONFIG_GPS_START_ON_MOTION) ?
		CONFIG_GPS_CONTROL_FIX_CHECK_OVERDUE :
		CONFIG_GPS_CONTROL_FIX_CHECK_INTERVAL;
#else
	gps_reporting_interval_seconds = CONFIG_GPS_CONTROL_FIX_CHECK_INTERVAL;
#endif

#ifdef GPS_CTR_DEBUG
	LOGD("GPS initialized");
#endif

	is_init = true;

	return err;
}
