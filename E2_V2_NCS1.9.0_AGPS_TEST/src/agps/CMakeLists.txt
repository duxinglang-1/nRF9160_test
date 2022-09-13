#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/agps.c)

target_sources(app PRIVATE ../agps/assistance_supl.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/assitance_supl.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/assitance.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/assitance_minimal.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mcc_location_table.c)

#zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_NRF_CLOUD agps/assistance.c)
#zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_SUPL agps/assistance_supl.c)
#zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL agps/assistance_minimal.c)
#zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL agps/mcc_location_table.c)