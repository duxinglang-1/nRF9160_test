/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ELP_CONNECT_H_
#define ELP_CONNECT_H_

/**@file elp_connect.h
 *
 * @brief nRF52/nRF91 inter-connect protocol.
 * @{
 */

#include <stdlib.h>
#include <zephyr/types.h>


/** Unsolicited notification type. */


/**
 * @typedef data_handler_t
 * @brief Callback when data is received from inter_connect interface.
 *
 * @param data_type Type of data, see ic_cmd_type.
 * @param data_buf Data buffer.
 * @param data_len Length of data buffer.
 */
typedef void (*data_handler_t)(u8_t data_type, const u8_t *data_buf,
				u8_t data_len);


/** @brief UARTs. */
enum select_uart {
	UART_0,
	UART_1,
	UART_2,
	UART_3
};
/** @brief Initialize the library.
 *
 * @param data_handler Callback handler for received data.
 *
 * @retval 0 If the operation was successful.
 *	Otherwise, a (negative) error code is returned.
 */
#if 0
int inter_connect_init(data_handler_t data_handler);
#else
int inter_connect_init(data_handler_t data_handler, enum select_uart uart_selected);
#endif

/** @brief Uninitialize the library.
 *
 * @retval 0 If the operation was successful.
 *	Otherwise, a (negative) error code is returned.
 */
int inter_connect_uninit(void);

/** @brief Send data or command response to the peer.
 *
 * @param data_type Type of response, see ic_cmd_type.
 * @param data_buf Data buffer.
 * @param data_len Length of data buffer.
 *
 * @retval 0 If the operation was successful.
 *	Otherwise, a (negative) error code is returned.
 */
#if 0
int inter_connect_send( u8_t *data_buff, u8_t data_len);
#else
int inter_connect_send( enum select_uart uart_sel, u8_t *data_buff, u8_t data_len);
#endif


/** @brief Send unsolicited response to the peer.
 *
 * @param data_type Type of notification, see ic_notify_type.
 * @param data_buf Data buffer.
 * @param data_len Length of data buffer.
 *

/** @} */

#endif /* ELP_CONNECT_H_ */
