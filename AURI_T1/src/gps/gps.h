/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <nrf_socket.h>
#include <net/socket.h>
#include <stdio.h>

extern bool app_gps_on;
extern bool app_gps_off;

extern bool ble_wait_gps;
extern bool sos_wait_gps;
extern bool fall_wait_gps;

extern void GPS_init(struct k_work_q *work_q);
extern void GPSMsgProcess(void);
