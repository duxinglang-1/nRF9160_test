/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <nrf_socket.h>
#include <net/socket.h>
#include <stdio.h>

extern bool got_first_fix;
extern bool gps_data_incoming;

extern nrf_gnss_data_frame_t last_fix;

extern void gps_init(void);
extern void gps_data_receive(void);