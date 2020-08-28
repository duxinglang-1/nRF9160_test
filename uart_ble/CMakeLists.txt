#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(store_fs)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/fs/nvs)

add_subdirectory(src/datetime)
add_subdirectory(src/font)
add_subdirectory(src/image)
add_subdirectory(src/lcd)
add_subdirectory(src/key)
add_subdirectory(src/settings)
add_subdirectory(src/inner_flash)
add_subdirectory(src/external_flash)
add_subdirectory(src/uart_ble)
add_subdirectory(src/imei)
add_subdirectory(src/tp)
#add_subdirectory(src/sensor)
#add_subdirectory(src/pmu)
#add_subdirectory(src/cryptoauthentication)