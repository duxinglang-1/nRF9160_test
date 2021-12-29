#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external_flash.c)
target_sources_ifdef(CONFIG_DL_TARGET_UI app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dl_target_ui.c)
target_sources_ifdef(CONFIG_DL_TARGET_FONT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dl_target_font.c)