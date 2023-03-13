#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_main.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_flash.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_lcd.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_gpio.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_vibrate.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_network.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_pmu.c)

target_sources_ifdef(CONFIG_AUDIO_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_audio.c)
target_sources_ifdef(CONFIG_BLE_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_ble.c)
target_sources_ifdef(CONFIG_GPS_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_gps.c)
target_sources_ifdef(CONFIG_IMU_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_imu.c)
target_sources_ifdef(CONFIG_PPG_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_ppg.c)
target_sources_ifdef(CONFIG_TEMP_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_temp.c)
target_sources_ifdef(CONFIG_TOUCH_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_touch.c)
target_sources_ifdef(CONFIG_WIFI_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ft_wifi.c)