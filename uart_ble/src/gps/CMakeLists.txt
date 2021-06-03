#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gps.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gps_controller.c)
target_sources_ifdef(CONFIG_NRF_CLOUD_AGPS
							 app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_codec.c)