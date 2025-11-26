#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/max32674.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/max86176.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/max_sh_interface.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/max_sh_fw_upgrade.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/max_sh_api.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/max_sh_config_api.c)