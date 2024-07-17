/****************************************Copyright (c)************************************************
** File Name:			    esp8266.h
** Descriptions:			wifi process head file
** Created By:				xie biao
** Created Date:			2021-03-29
** Modified Date:      		2021-03-29 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __ESP8266_H__
#define __ESP8266_H__

#define WIFI_NODE_MAX	5
#define WIFI_DATA_HEAD			"+CWLAP:"
#define WIFI_DATA_RSSI_BEGIN	"("
#define WIFI_DATA_RSSI_END		","
#define WIFI_DATA_MAC_BEGIN		"\""
#define WIFI_DATA_MAC_END		"\")"
#define WIFI_DATA_BRACKET_BEIN	"<"
#define WIFI_DATA_BRACKET_END	">"
#define WIFI_DATA_SEP_COLON		":"
#define WIFI_SLEEP_CMD			"AT+GSLP=0\r\n"
#define WIFI_SLEEP_REPLY		"AT+GSLP=0\r\n\r\nOK"
#define WIFI_GET_VER			"AT+GMR\r\n"
#define WIFI_GET_MAC_CMD		"AT+CIPAPMAC_DEF?\r\n"
#define WIFI_GET_MAC_REPLY		"+CIPAPMAC_DEF"
#define WIFI_DATA_VER_BIN		"Bin version"
#define WIFI_DATA_END			"\r\n"

#define WIFI_LOCAL_MIN_COUNT	3

typedef struct
{
	uint8_t rssi[8];
	uint8_t mac[32];
}wifi_node_infor;

typedef struct
{
	uint8_t count;
	wifi_node_infor node[WIFI_NODE_MAX];
}wifi_infor;

extern bool wifi_is_on;
extern bool sos_wait_wifi;
extern bool fall_wait_wifi;
extern bool location_wait_wifi;

#ifdef CONFIG_PM_DEVICE
extern bool uart_wifi_sleep_flag;
extern bool uart_wifi_wake_flag;
extern bool uart_wifi_is_waked;
#endif

extern uint8_t g_wifi_mac_addr[20];
extern uint8_t g_wifi_ver[20];
extern uint8_t wifi_test_info[256];
extern wifi_infor wifi_data;

extern bool wifi_is_working(void);
extern void ble_turn_on(void);
extern void wifi_receive_data_handle(uint8_t *buf, uint32_t len);
#endif/*__ESP8266_H__*/