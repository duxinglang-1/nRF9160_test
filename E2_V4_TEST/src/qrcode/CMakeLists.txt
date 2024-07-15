#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qr_encode.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bitstream.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mask.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mmask.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mqrspec.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qrencode.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qrinput.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qrspec.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/rscode.c)
#target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/split.c)