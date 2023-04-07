#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lsm6dso.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lsm6dso_reg.c)
target_sources_ifdef(CONFIG_SLEEP_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sleep.c)
target_sources_ifdef(CONFIG_FALL_DETECT_SUPPORT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fall.c)

set(LIB_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(awLib STATIC IMPORTED GLOBAL)
set_target_properties(awLib PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/awLib/MotionAW_CM33F_wc32_ot.a)
target_link_libraries(app PUBLIC awLib)

add_library(smLib STATIC IMPORTED GLOBAL)
set_target_properties(smLib PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/smLib/MotionSM_CM33F_wc32_ot.a)
target_link_libraries(app PUBLIC smLib)

add_library(pwLib STATIC IMPORTED GLOBAL)
set_target_properties(pwLib PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/pwLib/MotionPW_CM33F_wc32_ot.a)
target_link_libraries(app PUBLIC pwLib)
