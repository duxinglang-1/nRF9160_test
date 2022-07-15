#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external_flash.c)
if (CONFIG_DATA_DOWNLOAD_SUPPORT)
target_sources_ifdef(CONFIG_IMG_DATA_UPDATE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dl_target_ui.c)
target_sources_ifdef(CONFIG_FONT_DATA_UPDATE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dl_target_font.c)
target_sources_ifdef(CONFIG_PPG_DATA_UPDATE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dl_target_ppg.c)
endif()