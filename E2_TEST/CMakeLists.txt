#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(E2)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/fs/nvs)

add_subdirectory(src/datetime)
add_subdirectory(src/alarm)
add_subdirectory(src/font)
add_subdirectory(src/image)
add_subdirectory(src/lcd)
add_subdirectory(src/key)
add_subdirectory(src/settings)
add_subdirectory(src/inner_flash)
add_subdirectory(src/external_flash)
add_subdirectory(src/uart_ble)
add_subdirectory(src/imei)
add_subdirectory(src/gps)
add_subdirectory(src/nb)
add_subdirectory(src/screen)
add_subdirectory(src/pmu)
add_subdirectory(src/sos)
add_subdirectory(src/codetrans)
add_subdirectory(src/ucs2)
add_subdirectory(src/communicate)
add_subdirectory(src/logger)
#add_subdirectory(src/cryptoauthentication)

add_subdirectory_ifdef(CONFIG_ANIMATION_SUPPORT src/animation)
add_subdirectory_ifdef(CONFIG_TOUCH_SUPPORT src/tp)
add_subdirectory_ifdef(CONFIG_IMU_SUPPORT src/imu)
add_subdirectory_ifdef(CONFIG_PPG_SUPPORT src/ppg)
add_subdirectory_ifdef(CONFIG_AUDIO_SUPPORT src/audio)
add_subdirectory_ifdef(CONFIG_SYNC_SUPPORT src/sync)
add_subdirectory_ifdef(CONFIG_WIFI src/wifi)
add_subdirectory_ifdef(CONFIG_WATCHDOG src/watchdog)
add_subdirectory_ifdef(CONFIG_FOTA_DOWNLOAD src/fota)
