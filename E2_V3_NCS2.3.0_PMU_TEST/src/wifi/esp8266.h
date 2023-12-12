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

#define WIFI_AP_SSID				"\"HUAWEI-3YN3AN\""
#define WIFI_AP_PASSWORD			"\"km320000\""
#define WIFI_SERVER_ADDR			"\"47.107.51.89\""
#define WIFI_SERVER_PORT			"8088"
#define WIFI_SERVER_HTTP_HOST		"47.107.51.89"
#define WIFI_SERVER_HTTP_URL		"/e2Data"
#define WIFI_SERVER_HTTP_TYPE		"application/json"//"application/x-www-form-urlencoded"
#define WIFI_HTTP_RSP_OK			"HTTP/1.1 200"
#define WIFI_HTTP_RSP_LENGTH		"Content-Length: "
#define WIFI_HTTP_HEAD_ENDING		"\r\n\r\n"

#define WIFI_NODE_MAX	5
#define WIFI_DATA_HEAD				"+CWLAP:"
#define WIFI_DATA_RSSI_BEGIN		"("
#define WIFI_DATA_RSSI_END			","
#define WIFI_DATA_MAC_BEGIN			"\""
#define WIFI_DATA_MAC_END			"\")"
#define WIFI_DATA_BRACKET_BEIN		"<"
#define WIFI_DATA_BRACKET_END		">"
#define WIFI_DATA_SEP_COLON			":"
#define WIFI_CMD_SLEEP				"AT+GSLP=0\r\n"
#define WIFI_SLEEP_REPLY			"AT+GSLP=0\r\n\r\nOK"
#define WIFI_CMD_GET_VER			"AT+GMR\r\n"
#define WIFI_CMD_GET_MAC			"AT+CIPAPMAC_DEF?\r\n"
#define WIFI_GET_MAC_REPLY			"+CIPAPMAC_DEF"
#define WIFI_DATA_VER_BIN			"Bin version"
#define WIFI_DATA_END				"\r\n"

#define WIFI_CMD_SET_CWMODE				"AT+CWMODE=1\r\n"
#define WIFI_CMD_SEARCH_AP				"AT+CWLAP="WIFI_AP_SSID"\r\n"
#define WIFI_CMD_CONNECT_AP				"AT+CWJAP="WIFI_AP_SSID","WIFI_AP_PASSWORD"\r\n"
#define WIFI_CMD_CHECK_STA_IP			"AT+CIFSR\r\n"
#define WIFI_CMD_CHECK_STATUS			"AT+CIPSTATUS\r\n"
#define WIFI_CMD_CONNECT_SERVER			"AT+CIPSTART=\"TCP\","WIFI_SERVER_ADDR","WIFI_SERVER_PORT"\r\n"
#define WIFI_CMD_DISCONECT_SERVER		"AT+CIPCLOSE"
#define WIFI_CMD_SEND_START				"AT+CIPSEND="

#define WIFI_DISCONNECT_AP				"WIFI DISCONNECT"
#define WIFI_CONNECTED_AP				"WIFI CONNECTED"
#define WIFI_GOT_IP						"WIFI GOT IP"
#define WIFI_CONNECTED_SERVER			"CONNECT\r\n\r\nOK"
#define WIFI_ALREAY_CONNECTED_SERVER	"ALREADY CONNECTED"
#define WIFI_DISCONNECTED_SERVER		"CLOSED"
#define WIFI_SEND_DATA_OK				"SEND OK"
#define WIFI_RECEIVE_DATA				"+IPD,"
#define WIFI_IS_BUSY					"busy p...\r\n"

#define WIFI_LOCAL_MIN_COUNT	3

typedef enum
{
	WIFI_STATUS_OFF,
	WIFI_STATUS_ON,
	WIFI_STATUS_GOT_AP,
	WIFI_STATUS_CONNECTING_TO_AP,
	WIFI_STATUS_CONNECTED_TO_AP,
	WIFI_STATUS_CONNECTING_TO_SERVER,
	WIFI_STATUS_CONNECTED_TO_SERVER,
	WIFI_STATUS_MAX
}WIFI_STATUS;

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

extern bool wifi_is_working(void);
extern void ble_turn_on(void);
extern void wifi_receive_data_handle(uint8_t *buf, uint32_t len);
extern void wifi_send_payload(uint8_t *data, uint32_t datalen);
#endif/*__ESP8266_H__*/