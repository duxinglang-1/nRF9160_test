/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/gps.h>

extern bool app_gps_on;
extern bool app_gps_off;

extern bool ble_wait_gps;
extern bool sos_wait_gps;
extern bool fall_wait_gps;
extern bool location_wait_gps;
extern bool test_gps_flag;

extern u8_t gps_test_info[256];

extern void GPS_init(struct k_work_q *work_q);
extern void GPSMsgProcess(void);