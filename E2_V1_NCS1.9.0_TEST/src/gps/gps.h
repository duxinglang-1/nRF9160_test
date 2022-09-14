/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/gps.h>
#include <nrf_modem_gnss.h>

extern bool gps_on_flag;
extern bool gps_off_flag;

extern bool ble_wait_gps;
extern bool sos_wait_gps;
#ifdef CONFIG_FALL_DETECT_SUPPORT
extern bool fall_wait_gps;
#endif
extern bool location_wait_gps;
extern bool test_gps_flag;

extern uint8_t gps_test_info[256];

extern void test_gps_on(void);
extern void GPS_init(void);
extern void GPSMsgProcess(void);
