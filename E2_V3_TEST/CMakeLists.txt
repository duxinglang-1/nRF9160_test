#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.20.0)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spm.conf")
  set(spm_CONF_FILE
    prj.conf
    ${CMAKE_CURRENT_LIST_DIR}/spm.conf
  )
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mcuboot.conf")
  set(mcuboot_CONF_FILE
    prj.conf
    ${CMAKE_CURRENT_LIST_DIR}/mcuboot.conf
  )
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spm.overlay")
  set(spm_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/spm.overlay")
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mcuboot.overlay")
  set(mcuboot_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/mcuboot.overlay")
endif()

#include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(E2)

#FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE src/main.c)
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
add_subdirectory_ifdef(CONFIG_TEMP_SUPPORT src/temperature)
add_subdirectory_ifdef(CONFIG_CRC_SUPPORT src/crc_check)
add_subdirectory_ifdef(CONFIG_SYNC_SUPPORT src/sync)
add_subdirectory_ifdef(CONFIG_WIFI_SUPPORT src/wifi)
add_subdirectory_ifdef(CONFIG_WATCHDOG src/watchdog)
add_subdirectory_ifdef(CONFIG_FOTA_DOWNLOAD src/fota)
add_subdirectory_ifdef(CONFIG_DATA_DOWNLOAD_SUPPORT src/data_download)
add_subdirectory_ifdef(CONFIG_BARCODE_SUPPORT src/barcode)
add_subdirectory_ifdef(CONFIG_QRCODE_SUPPORT src/qrcode)