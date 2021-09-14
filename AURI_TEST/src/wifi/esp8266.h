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

#define MAX_SCANNED_WIFI_NODE	5

typedef struct
{
    u8_t rssi[8];
	u8_t mac[32];
}wifi_node_infor;

typedef struct
{
	u8_t count;
	wifi_node_infor node[MAX_SCANNED_WIFI_NODE];
}wifi_infor;


typedef struct
  {

	 char rssi[10];
	 
  }wifi_node;


typedef struct 
{
	u8_t count;
	wifi_node  node[50];

}wifi_infor1;

extern bool wifi_is_on;
extern bool sos_wait_wifi;
extern bool fall_wait_wifi;
extern bool location_wait_wifi;

extern u8_t wifi_test_info[256];

extern void ble_turn_on(void);
extern void wifi_receive_data_handle(u8_t *buf, u32_t len);
#endif/*__ESP8266_H__*/