/****************************************Copyright (c)************************************************
** File Name:				transfer_cache.h
** Descriptions:			Data transfer cache pool head file
** Created By:				xie biao
** Created Date:			2021-03-25
** Modified Date:			2021-03-25 
** Version:					V1.0
******************************************************************************************************/
#ifndef __UART_BLE_H__
#define __UART_BLE_H__

#include <drivers/gps.h>

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

typedef enum
{
	BLE_WORK_NORMAL,
	BLE_WORK_DFU,
	BLE_WORK_MAX
}ENUM_BLE_WORK_MODE;

extern bool g_ble_connected;
extern ENUM_BLE_STATUS g_ble_status;
extern ENUM_BLE_MODE g_ble_mode;

extern uint8_t g_ble_mac_addr[20];
extern uint8_t g_nrf52810_ver[128];

extern void uart_ble_test(void);
extern void MCU_get_nrf52810_ver(void);
extern void MCU_get_ble_mac_address(void);
extern void MCU_get_ble_status(void);
extern void MCU_set_ble_work_mode(uint8_t work_mode);
extern void MCU_send_find_phone(void);

#endif/*__UART_BLE_H__*/
