#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(test_show_pic_font)

# NORDIC SDK APP START
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
# NORDIC SDK APP END

# Application sources
add_subdirectory(src/datetime)
add_subdirectory(src/font)
add_subdirectory(src/image)
add_subdirectory(src/lcd)
