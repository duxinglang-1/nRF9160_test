#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lcd.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/LCD_LH096TIG11G_ST7735SV.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/LCD_R154101_ST7796S.c)