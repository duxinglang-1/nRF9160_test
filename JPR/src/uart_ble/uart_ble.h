/*
* Copyright (c) 2019 Nordic Semiconductor ASA
*
* SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
*/

//0:关闭 1:休眠 2:广播 3:连接
typedef enum
{
	BLE_STATUS_OFF,
	BLE_STATUS_SLEEP,
	BLE_STATUS_BROADCAST,
	BLE_STATUS_CONNECTED,
	BLE_STATUS_MAX
}ENUM_BLE_STATUS;

//0:关闭 1:打开 2:唤醒 3:休眠
typedef enum
{
	BLE_MODE_TURN_OFF,
	BLE_MODE_TURN_ON,
	BLE_MODE_WAKE_UP,
	BLE_MODE_GOTO_SLEEP,
	BLE_MODE_MAX
}ENUM_BLE_MODE;

extern bool APP_wait_gps;
extern void uart_ble_test(void);
extern void APP_get_location_data_reply(u8_t *buf, u32_t len);
extern void MCU_get_nrf52810_ver(void);
extern void MCU_get_ble_mac_address(void);
extern void MCU_get_ble_status(void);
extern void MCU_set_ble_work_mode(u8_t work_mode);