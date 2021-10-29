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
#define WIFI_SLEEP_CMD			"AT+GSLP=0\r\n"
#define WIFI_SLEEP_REPLY		"AT+GSLP=0\r\n\r\nOK"

typedef struct
{
    u8_t rssi[8];
	u8_t mac[32];
}wifi_node_infor;

typedef struct
{
	u8_t count;
	wifi_node_infor node[WIFI_NODE_MAX];
}wifi_infor;

extern bool wifi_is_on;
extern bool sos_wait_wifi;
extern bool fall_wait_wifi;
extern bool location_wait_wifi;

extern u8_t wifi_test_info[256];

extern bool wifi_is_working(void);
extern void ble_turn_on(void);
extern void wifi_receive_data_handle(u8_t *buf, u32_t len);
#endif/*__ESP8266_H__*/