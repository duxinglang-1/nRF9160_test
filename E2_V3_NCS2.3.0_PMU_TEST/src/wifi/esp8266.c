/****************************************Copyright (c)************************************************
** File Name:			    esp8266.c
** Descriptions:			wifi process source file
** Created By:				xie biao
** Created Date:			2021-03-29
** Modified Date:      		2021-03-29 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <stdio.h>
#include <string.h>
#include <dk_buttons_and_leds.h>
#include "esp8266.h"
#include "uart_ble.h"
#include "screen.h"
#include "lcd.h"
#include "logger.h"
#include "transfer_cache.h"
#ifdef CONFIG_SYNC_SUPPORT
#include "sync.h"
#endif

#define WIFI_DEBUG

#define WIFI_EN_PIN		4	//WIFI EN，使用WIFI需要拉低此脚
#define WIFI_RST_PIN	3

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
#define WIFI_DEV DT_NODELABEL(uart0)
#else
#error "uart0 devicetree node is disabled"
#define WIFI_DEV	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define WIFI_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define WIFI_PORT	""
#endif

#define WIFI_RETRY_COUNT_MAX	5
#define BUF_MAXSIZE	2048

#define WIFI_AUTO_OFF_TIME_SEC	(1)

uint8_t g_wifi_mac_addr[20] = {0};
uint8_t g_wifi_ver[20] = {0};

static uint8_t retry = 0;
static uint32_t rece_len=0;
static uint32_t send_len=0;
static uint8_t data_buf[BUF_MAXSIZE] = {0};

static uint8_t rx_buf[BUF_MAXSIZE]={0};
static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static struct device *uart_wifi = NULL;
static struct device *gpio_wifi = NULL;

static struct uart_data_t
{
	void  *fifo_reserved;
	uint8_t    data[BUF_MAXSIZE];
	uint16_t   len;
};

WIFI_STATUS wifi_work_status = WIFI_STATUS_ON;
bool sos_wait_wifi = false;
bool fall_wait_wifi = false;
bool location_wait_wifi = false;
bool wifi_is_on = false;
#ifdef CONFIG_PM_DEVICE
bool uart_wifi_sleep_flag = false;
bool uart_wifi_wake_flag = false;
bool uart_wifi_is_waked = true;
#endif

uint8_t wifi_test_info[256] = {0};

static bool app_wifi_on = false;
static bool wifi_on_flag = false;
static bool wifi_off_flag = false;
static bool test_wifi_flag = false;
static bool wifi_test_update_flag = false;
static bool wifi_rescanning_flag = false;
static bool wifi_wait_timerout_flag = false;
static bool wifi_off_retry_flag = false;
static bool wifi_off_ok_flag = false;
static bool wifi_get_infor_flag = false;
static bool wifi_connect_ap_flag = false;
static bool wifi_connect_server_flag = false;
static bool wifi_send_cmd_flag = false;
static bool wifi_send_data_flag = false;
static bool wifi_rece_data_flag = false;
static bool wifi_rece_frame_flag = false;
static bool wifi_parse_data_flag = false;

static CacheInfo wifi_send_cmd_cache = {0};
static CacheInfo wifi_send_data_cache = {0};
static CacheInfo wifi_rece_data_cache = {0};
static CacheInfo wifi_rece_cache = {0};

static wifi_infor wifi_data = {0};

static void WifiConnectApCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_connect_ap_timer, WifiConnectApCallBack, NULL);
static void WifiConnectServerCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_connect_server_timer, WifiConnectServerCallBack, NULL);
static void WifiSendCmdCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_send_cmd_timer, WifiSendCmdCallBack, NULL);
static void WifiSendDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_send_data_timer, WifiSendDataCallBack, NULL);
static void WifiReceDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_rece_data_timer, WifiReceDataCallBack, NULL);
static void WifiParseDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_parse_data_timer, WifiParseDataCallBack, NULL);
static void WifiReceFrameCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_rece_frame_timer, WifiReceFrameCallBack, NULL);
static void WifiGetInforCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_get_infor_timer, WifiGetInforCallBack, NULL);
static void APP_Ask_wifi_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_scan_timer, APP_Ask_wifi_Data_timerout, NULL);
static void wifi_rescan_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_rescan_timer, wifi_rescan_timerout, NULL);
static void wifi_off_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_off_retry_timer, wifi_off_timerout, NULL);
static void wifi_turn_off_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_turn_off_timer, wifi_turn_off_timerout, NULL);

static void WifiSendCmdStart(void);
static void WifiSendCmdStop(void);
static void WifiSendDataStart(void);
static void WifiSendDataStop(void);
static void WifiReceData(uint8_t *data, uint32_t datalen);

static void WifiConnectApCallBack(struct k_timer *timer_id)
{
	wifi_connect_ap_flag = true;
}

static void WifiConnectServerCallBack(struct k_timer *timer_id)
{
	wifi_connect_server_flag = true;
}

static void WifiSendCmdCallBack(struct k_timer *timer_id)
{
	wifi_send_cmd_flag = true;
}

static void WifiSendDataCallBack(struct k_timer *timer)
{
	wifi_send_data_flag = true;
}

static void WifiParseDataCallBack(struct k_timer *timer_id)
{
	wifi_parse_data_flag = true;
}

static void WifiReceDataCallBack(struct k_timer *timer_id)
{
	wifi_rece_data_flag = true;
}

static void WifiReceFrameCallBack(struct k_timer *timer_id)
{
	wifi_rece_frame_flag = true;
}

static void APP_Ask_wifi_Data_timerout(struct k_timer *timer_id)
{
	wifi_wait_timerout_flag = true;
}

static void wifi_rescan_timerout(struct k_timer *timer_id)
{
	wifi_rescanning_flag = true;
}

static void wifi_off_timerout(struct k_timer *timer_id)
{
	wifi_off_retry_flag = true;
}

static void wifi_turn_off_timerout(struct k_timer *timer_id)
{
	wifi_off_flag = true;
}

void wifi_scanned_wait_timerout(void)
{
	retry++;
	if(retry < WIFI_RETRY_COUNT_MAX)
	{
		wifi_rescanning_flag = true;
		memset(&wifi_data, 0, sizeof(wifi_data));
		k_timer_start(&wifi_scan_timer, K_MSEC(5000), K_NO_WAIT);	
	}
	else
	{
		retry = 0;
		app_wifi_on = false;
		wifi_turn_off();

		if(sos_wait_wifi)
		{
			sos_get_wifi_data_reply(wifi_data);	
			sos_wait_wifi = false;
		}
	#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_FALL_DETECT_SUPPORT)
		if(fall_wait_wifi)
		{
			fall_get_wifi_data_reply(wifi_data);	
			fall_wait_wifi = false;
		}
	#endif
		if(location_wait_wifi)
		{
			location_get_wifi_data_reply(wifi_data);
			location_wait_wifi = false;
		}	
	}
}

void wifi_get_scanned_data(void)
{
	app_wifi_on = false;
	retry = 0;
	
	if(k_timer_remaining_get(&wifi_scan_timer) > 0)
		k_timer_stop(&wifi_scan_timer);
	
	if(sos_wait_wifi)
	{
		sos_get_wifi_data_reply(wifi_data);	
		sos_wait_wifi = false;
	}

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_FALL_DETECT_SUPPORT)
	if(fall_wait_wifi)
	{
		fall_get_wifi_data_reply(wifi_data);	
		fall_wait_wifi = false;
	}
#endif

	if(location_wait_wifi)
	{
		location_get_wifi_data_reply(wifi_data);
		location_wait_wifi = false;
	}
}

void APP_Ask_wifi_data(void)
{
#ifdef WIFI_DEBUG
	LOGD("begin");
#endif
	if(!app_wifi_on)
	{
		if(k_timer_remaining_get(&wifi_turn_off_timer) > 0)
			k_timer_stop(&wifi_turn_off_timer);

		retry = 0;
		app_wifi_on = true;
		memset(&wifi_data, 0, sizeof(wifi_data));
		
		wifi_turn_on_and_scanning();
		k_timer_start(&wifi_scan_timer, K_MSEC(5*1000), K_NO_WAIT);	
	}
}

/*============================================================================
* Function Name  : Send_Cmd_To_Esp8285
* Description    : 向ESP8265送命令
* Input          : cmd:发送的命令字符串;waittime:等待时间(单位:10ms)
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void Send_Cmd_To_Esp8285(uint8_t *cmd, uint32_t WaitTime)
{
	WifiSendCmd(cmd, strlen(cmd));//发送命令
}

/*============================================================================
* Function Name  : IsInWifiScreen
* Description    : 处于wifi测试界面
* Input          : None
* Output         : None
* Return         : bool
* CALL           : 可被外部调用
==============================================================================*/
bool IsInWifiScreen(void)
{
	if(screen_id == SCREEN_ID_WIFI_TEST)
		return true;
	else
		return false;
}

/*============================================================================
* Function Name  : wifi_is_working
* Description    : wifi功能正在运行
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
bool wifi_is_working(void)
{
#ifdef WIFI_DEBUG
	LOGD("wifi_is_on:%d", wifi_is_on);
#endif

	if(wifi_is_on)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*============================================================================
* Function Name  : wifi_enable
* Description    : Esp8285_EN使能，高电平有效
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void wifi_enable(void)
{
	gpio_pin_set(gpio_wifi, WIFI_RST_PIN, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set(gpio_wifi, WIFI_RST_PIN, 0);
	
	gpio_pin_set(gpio_wifi, WIFI_EN_PIN, 1);
	k_sleep(K_MSEC(100));

	wifi_work_status = WIFI_STATUS_ON;
}

/*============================================================================
* Function Name  : wifi_disable
* Description    : Esp8285_EN使能禁止，高电平有效
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void wifi_disable(void)
{
	gpio_pin_set(gpio_wifi, WIFI_EN_PIN, 0);

	wifi_work_status = WIFI_STATUS_OFF;
}

/*============================================================================
* Function Name  : wifi_start_scanning
* Description    : ESP8285模块启动WiFi信号扫描
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/ 	
void wifi_start_scanning(void)
{
	//设置工作模式 1:station模式 2:AP模式 3:兼容AP+station模式
	WifiSendCmd("AT+CWMODE=1\r\n", strlen("AT+CWMODE=1\r\n"));
	//设置AT+CWLAP信号的排序方式：按RSSI排序，只显示信号强度和MAC模式
	WifiSendCmd("AT+CWLAPOPT=1,12\r\n", strlen("AT+CWLAPOPT=1,12\r\n"));
	//启动扫描
	WifiSendCmd("AT+CWLAP\r\n", strlen("AT+CWLAP\r\n"));
}

/*============================================================================
* Function Name  : wifi_turn_on_and_scanning
* Description    : ESPP8285 init
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void wifi_turn_on_and_scanning(void)
{
	wifi_is_on = true;

#ifdef WIFI_DEBUG	
	LOGD("begin");
#endif

	wifi_enable();
	wifi_start_scanning();
}

void wifi_turn_off_success(void)
{
#ifdef WIFI_DEBUG	
	LOGD("begin");
#endif

	gpio_pin_set(gpio_wifi, WIFI_EN_PIN, 0);
	wifi_off_retry_flag = false;
	k_timer_stop(&wifi_off_retry_timer);

	wifi_is_on = false;
#ifdef CONFIG_PM_DEVICE	
	uart_wifi_sleep_flag = true;
#endif
}

void wifi_turn_off(void)
{
#ifdef WIFI_DEBUG	
	LOGD("begin");
#endif
	wifi_disable();

	wifi_is_on = false;
#ifdef CONFIG_PM_DEVICE	
	uart_wifi_sleep_flag = true;
#endif
	wifi_work_status = WIFI_STATUS_OFF;
}

void wifi_rescanning(void)
{
	if(!wifi_is_on)
		return;

	//设置AT+CWLAP信号的排序方式：按RSSI排序，只显示信号强度和MAC模式
	WifiSendCmd("AT+CWLAPOPT=1,12\r\n", strlen("AT+CWLAPOPT=1,12\r\n"));
	WifiSendCmd("AT+CWLAP\r\n", strlen("AT+CWLAP\r\n"));
}

void *wifi_payload_packet_http(uint8_t *data, uint32_t datalen, uint8_t *httpdata, uint32_t *httpdata_len)
{
	uint8_t tmpbuf[1024] = {0};
	uint32_t len = 0;

	memset(data_buf, 0x00, sizeof(data_buf));
	sprintf(tmpbuf, "{\"data\":\"%s\"}", data);
	len = strlen(tmpbuf);
	sprintf(data_buf, "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: %s\r\nContent-length: %d\r\n\r\n",
						WIFI_SERVER_HTTP_URL, 
						WIFI_SERVER_HTTP_HOST, 
						WIFI_SERVER_HTTP_TYPE, 
						len);

	strcat(data_buf, tmpbuf);
	strcpy(httpdata, data_buf);
	*httpdata_len = strlen(httpdata);
}

void wifi_send_payload(uint8_t *data, uint32_t datalen)
{
#ifdef WIFI_DEBUG
	LOGD("payload:%d, %s", datalen, data);
#endif

	if(wifi_work_status == WIFI_STATUS_OFF)
	{
		wifi_enable();
		WifiSendCmd(WIFI_CMD_SET_CWMODE, strlen(WIFI_CMD_SET_CWMODE));
	}
	else if(wifi_work_status == WIFI_STATUS_ON)
	{
		wifi_work_status = WIFI_STATUS_CONNECTING_TO_AP;
		WifiSendCmd(WIFI_CMD_CONNECT_AP, strlen(WIFI_CMD_CONNECT_AP));
	}
	else if(wifi_work_status != WIFI_STATUS_CONNECTED_TO_SERVER)
	{
		wifi_work_status = WIFI_STATUS_CONNECTING_TO_SERVER;
		WifiSendCmd(WIFI_CMD_CONNECT_SERVER, strlen(WIFI_CMD_CONNECT_SERVER));
	}

	WifiSendData(data, datalen);
	if(wifi_work_status == WIFI_STATUS_CONNECTED_TO_SERVER)
		WifiSendDataStart();

	k_timer_start(&wifi_turn_off_timer, K_SECONDS(30), K_NO_WAIT);	
}

void wifi_get_payload(uint8_t *data, uint32_t datalen)
{
	//HTTP/1.1 200 
	//Content-Type: text/plain;charset=UTF-8
	//Content-Length: 32
	//Date: Fri, 08 Dec 2023 07:33:31 GMT
	//
	//context
	uint8_t *ptr1,*ptr2;
	uint8_t tmpbuf[128] = {0};
	uint32_t len = 0;

	if(strstr(data, WIFI_HTTP_RSP_OK))
	{
	#ifdef CONFIG_SYNC_SUPPORT
		SyncNetWorkCallBack(SYNC_STATUS_SENT);
	#endif
	}
	
	ptr1 = strstr(data, WIFI_HTTP_RSP_LENGTH);
	if(ptr1)
	{
		ptr1 += (strlen(WIFI_HTTP_RSP_LENGTH));
		ptr2 = strstr(ptr1, "\r\n");
		if(ptr2)
		{
			memcpy(tmpbuf, ptr1, ptr2-ptr1);
			len = atoi(tmpbuf);
		}
	}

	if(len > 0)
	{
		ptr1 = strstr(data, WIFI_HTTP_HEAD_ENDING);
		if(ptr1)
		{
			ptr1 += strlen(WIFI_HTTP_HEAD_ENDING);
		#ifdef WIFI_DEBUG
			LOGD("payload:%d, %s", len, ptr1);
		#endif
			WifiReceData(ptr1, len);
		}
	}
}

/*============================================================================
* Function Name  : wifi_receive_data_handle
* Description    : NRF9160 接收 ESP8285发来的AP扫描信息进行处理
* Input          : buf:数据缓存 len:数据长度
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void wifi_receive_data_handle(uint8_t *buf, uint32_t len)
{
	int ret;
	uint8_t count = 0;
	uint8_t tmpbuf[256] = {0};
	uint8_t *ptr = buf;
	uint8_t *ptr1 = NULL;
	uint8_t *ptr2 = NULL;
	uint8_t okbuf[] = "\r\nOK";
	bool flag = false;

#ifdef WIFI_DEBUG
	LOGD("receive:%s", buf);
#endif

	if(strstr(ptr, WIFI_SLEEP_REPLY))
	{
	#ifdef WIFI_DEBUG
		LOGD("sleep reply");
	#endif
		wifi_off_ok_flag = true;
		return;
	}

	if(strstr(ptr, WIFI_CMD_SET_CWMODE))
	{
	#ifdef WIFI_DEBUG
		LOGD("set cwmode");
	#endif
		//AT+CWMODE=1
		//
		//OK
		
		if(!cache_is_empty(&wifi_send_data_cache))
			k_timer_start(&wifi_connect_ap_timer, K_MSEC(50), K_NO_WAIT);
		
		return;
	}

	if(strstr(ptr, WIFI_GET_MAC_REPLY))
	{
	#ifdef WIFI_DEBUG
		LOGD("mac reply");
	#endif
		//AT+CIPAPMAC_DEF?
		//+CIPAPMAC_DEF:"ff:ff:ff:ff:ff:ff"
		//\r\n
		//OK
		//\r\n
		ptr1 = strstr(ptr, WIFI_DATA_MAC_BEGIN);
		if(ptr1)
		{
			ptr1++;
			ptr2 = strstr(ptr1, WIFI_DATA_MAC_BEGIN);
			if(ptr2)
			{
				memcpy(g_wifi_mac_addr, ptr1, ptr2-ptr1);
			}
		}
		return;
	}

	if(strstr(ptr, WIFI_CMD_GET_VER))
	{
	#ifdef WIFI_DEBUG
		LOGD("ver reply");
	#endif
		//AT+GMR
		//AT version:1.6.2.0(Apr 13 2018 11:10:59)
		//SDK version:2.2.1(6ab97e9)
		//compile time:Jun  7 2018 19:34:26
		//Bin version(Wroom 02):1.6.2
		//OK
		//\r\n
		ptr1 = strstr(ptr, WIFI_DATA_VER_BIN);
		if(ptr1)
		{
			ptr1++;
			ptr1 = strstr(ptr1, WIFI_DATA_SEP_COLON);
			if(ptr1)
			{
				ptr1++;
				ptr2 = strstr(ptr1, WIFI_DATA_END);
				if(ptr2)
				{
					memcpy(g_wifi_ver, ptr1, ptr2-ptr1);
				}
			}
		}

		//wifi_off_flag = true;
		return;
	}

	if(strstr(ptr,WIFI_DATA_HEAD))
	{
	#ifdef WIFI_DEBUG
		LOGD("cwlap reply");
	#endif
		//+CWLAP:(-61,"f4:84:8d:8e:9f:eb")
		//+CWLAP:(-67,"da:f1:5b:ff:f2:bc")
		//+CWLAP:(-67,"e2:c1:13:2d:9e:47")
		//+CWLAP:(-73,"7c:94:2a:39:9f:50")
		//+CWLAP:(-76,"52:c2:e8:c6:fa:1e")
		//+CWLAP:(-80,"80:ea:07:73:96:1a")
		//\r\n
		//OK
		//\r\n 
		while(1)
		{
			uint8_t len;
		    uint8_t str_rssi[8]={0};
			uint8_t str_mac[32]={0};

			//head
			ptr1 = strstr(ptr,WIFI_DATA_HEAD);
			if(ptr1 == NULL)
			{
				ptr2 = ptr;
				goto loop;
			}

			//scaned data flag
			flag = true;
			
			//rssi
			ptr += strlen(WIFI_DATA_HEAD);
			ptr1 = strstr(ptr,WIFI_DATA_RSSI_BEGIN);         //取字符串中的之后的字符
		#if 0	
			ptr = ptr1+1;
			ptr1 = strstr(ptr,",");         //ecn
			ptr = ptr1+1;
			ptr1 = strstr(ptr,",");         //ssid
		#endif	
			if(ptr1 == NULL)
			{
				ptr2 = ptr;
				goto loop;
			}
			
			ptr2 = strstr(ptr1+1,WIFI_DATA_RSSI_END);
			if(ptr2 == NULL)
			{
				ptr2 = ptr1+1;
				goto loop;
			}

			len = ptr2 - (ptr1+1);
			if(len > 4)
			{
				goto loop;
			}
			
			memcpy(str_rssi, ptr1+1, len);

			//MAC
			ptr1 = strstr(ptr2,WIFI_DATA_MAC_BEGIN);
			if(ptr1 == NULL)
			{
				goto loop;
			}

			ptr2 = strstr(ptr1+1,WIFI_DATA_MAC_END);
			if(ptr2 == NULL)
			{
				ptr2 = ptr1+1;
				goto loop;
			}

			len = ptr2 - (ptr1+1);
			if(len != 17)
			{
				goto loop;
			}

			memcpy(str_mac, ptr1+1, len);
			
			if(test_wifi_flag)
			{
				uint8_t buf[128] = {0};

				count++;
				if(count<=6)
				{
				#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
					sprintf(buf, "%02d|", -(atoi(str_rssi)));
				#else
					sprintf(buf, "%s|%02d\n", str_mac, -(atoi(str_rssi)));
				#endif
					strcat(tmpbuf, buf);
				}
			}
			else
			{
				strcpy(wifi_data.node[wifi_data.count].rssi, str_rssi);
				strcpy(wifi_data.node[wifi_data.count].mac, str_mac);
				
				wifi_data.count++;
				if(wifi_data.count == WIFI_NODE_MAX)
					break;
			}

		loop:
			ptr = ptr2+1;
			if(*ptr == 0x00)
				break;
		}

		if(test_wifi_flag)
		{
			if(count > 0)
			{
				memset(wifi_test_info,0,sizeof(wifi_test_info));
				sprintf(wifi_test_info, "%d\n", count);
				strcat(wifi_test_info, tmpbuf);
				wifi_test_update_flag = true;

			#ifdef CONFIG_FACTORY_TEST_SUPPORT
				FTWifiStatusUpdate(count);
			#endif
			}
		}
		else
		{
			if(flag && (wifi_data.count >= WIFI_LOCAL_MIN_COUNT))	//扫描有效数据
			{
				wifi_get_scanned_data();
				wifi_off_flag = true;
			}
		}
	}	

	if(strstr(ptr,WIFI_GOT_IP))
	{
		//WIFI CONNECTED
		//WIFI GOT IP

	#ifdef WIFI_DEBUG
		LOGD("got ip reply");
	#endif
		wifi_work_status = WIFI_STATUS_CONNECTED_TO_AP;

		if(!cache_is_empty(&wifi_send_data_cache))
			k_timer_start(&wifi_connect_server_timer, K_MSEC(1000), K_NO_WAIT);
		
		return;
	}

	if(strstr(ptr,WIFI_DISCONNECT_AP))
	{
	#ifdef WIFI_DEBUG
		LOGD("disconnect ap");
	#endif
		wifi_work_status = WIFI_STATUS_ON;
	}
	
	if(strstr(ptr,WIFI_CONNECTED_SERVER)
		||strstr(ptr,WIFI_ALREAY_CONNECTED_SERVER))
	{
	#ifdef WIFI_DEBUG
		LOGD("connected server");
	#endif
		wifi_work_status = WIFI_STATUS_CONNECTED_TO_SERVER;
		
		if(!cache_is_empty(&wifi_send_data_cache))
			WifiSendDataStart();
	}

	if(strstr(ptr,WIFI_DISCONNECTED_SERVER))
	{
	#ifdef WIFI_DEBUG
		LOGD("disconnect server");
	#endif
		wifi_work_status = WIFI_STATUS_CONNECTED_TO_AP;

		WifiSendDataStop();
		return;
	}

	if(strstr(ptr,WIFI_RECEIVE_DATA))
	{
		//+IPD,n:xxxxxxxxxx				//	received	n	bytes,		data=xxxxxxxxxxx
		uint8_t tmpbuf[8] = {0};
		uint32_t datelen = 0;

	#ifdef WIFI_DEBUG
		LOGD("IPD reply");
	#endif	
		ptr1 = strstr(ptr,WIFI_RECEIVE_DATA);
		ptr1 += strlen(WIFI_RECEIVE_DATA);
		ptr2 = strstr(ptr1, ":");
		memcpy(tmpbuf, ptr1, ptr2-ptr1);
		datelen = atoi(tmpbuf);
		wifi_get_payload(ptr2+1, datelen);
	}

	if(strstr(ptr,WIFI_CMD_SEND_START)
		||strstr(ptr,WIFI_SEND_DATA_OK))
	{
	#ifdef WIFI_DEBUG
		LOGD("continue send data");
	#endif
		delete_data_from_cache(&wifi_send_data_cache);
		k_timer_start(&wifi_send_data_timer, K_MSEC(100), K_NO_WAIT);
	}

	if(strstr(ptr,WIFI_IS_BUSY))
	{
	#ifdef WIFI_DEBUG
		LOGD("busy p...");
	#endif
		if(wifi_work_status == WIFI_STATUS_CONNECTING_TO_AP)
		{
			if(!cache_is_empty(&wifi_send_data_cache))
				k_timer_start(&wifi_connect_ap_timer, K_MSEC(100), K_NO_WAIT);
		}

		if(wifi_work_status == WIFI_STATUS_CONNECTING_TO_SERVER)
		{
			if(!cache_is_empty(&wifi_send_data_cache))
				k_timer_start(&wifi_connect_server_timer, K_MSEC(100), K_NO_WAIT);
		}
	}
}

void MenuStartWifi(void)
{
	wifi_on_flag = true;
	test_wifi_flag = true;
}

void MenuStopWifi(void)
{
	wifi_off_flag = true;
	test_wifi_flag = false;	
}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void FTStartWifi(void)
{
	wifi_on_flag = true;
	test_wifi_flag = true;
}

void FTStopWifi(void)
{
	wifi_off_flag = true;
	test_wifi_flag = false;	
}
#endif

void wifi_test_update(void)
{
	if(screen_id == SCREEN_ID_WIFI_TEST)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void test_wifi(void)
{
	MenuStartWifi();
}

static void WifiGetInforCallBack(struct k_timer *timer_id)
{
	wifi_get_infor_flag = true;
}

#ifdef CONFIG_PM_DEVICE
static void UartWifiSleepInCallBack(struct k_timer *timer_id)
{
#ifdef WIFI_DEBUG
	LOGD("begin");
#endif
	uart_wifi_sleep_flag = true;
}

void uart_wifi_sleep_out(void)
{
	if(uart_wifi_is_waked)
		return;

	pm_device_action_run(uart_wifi, PM_DEVICE_ACTION_RESUME);
	uart_wifi_is_waked = true;

	k_sleep(K_MSEC(50));
	
#ifdef WIFI_DEBUG
	LOGD("uart wifi set active success!");
#endif
}

void uart_wifi_sleep_in(void)
{	
	if(!uart_wifi_is_waked)
		return;

	pm_device_action_run(uart_wifi, PM_DEVICE_ACTION_SUSPEND);
	uart_wifi_is_waked = false;

#ifdef WIFI_DEBUG
	LOGD("uart wifi set low power success!");
#endif
}
#endif

void wifi_send_data_handle(uint8_t *buf, uint32_t len)
{
#ifdef WIFI_DEBUG
	LOGD("cmd:%s", buf);
#endif

	if(k_timer_remaining_get(&wifi_turn_off_timer) > 0)
		k_timer_stop(&wifi_turn_off_timer);
	k_timer_start(&wifi_turn_off_timer, K_SECONDS(30), K_NO_WAIT);	

#ifdef CONFIG_PM_DEVICE
	uart_wifi_sleep_out();
#endif

	uart_fifo_fill(uart_wifi, buf, len);
	uart_irq_tx_enable(uart_wifi);
}

static void WifiParseReceData(void)
{
	uint8_t data_type,*p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_cache(&wifi_rece_data_cache, &p_data, &data_len, &data_type);
	if(ret)
	{
		ParseData(p_data, data_len);
		delete_data_from_cache(&wifi_rece_data_cache);
		
		k_timer_start(&wifi_parse_data_timer, K_MSEC(50), K_NO_WAIT);
	}
}

static void WifiParseReceDataStart(void)
{
	k_timer_start(&wifi_parse_data_timer, K_MSEC(500), K_NO_WAIT);
}

static void WifiReceData(uint8_t *data, uint32_t datalen)
{
	uint8_t *ptr=data,*ptr1,*ptr2;
	uint8_t begin[] = "{";
	uint8_t end[] = "}";
	
	while(1)
	{
		uint8_t tmpbuf[1024] = {0};
		
		ptr1 = strstr(ptr, begin);
		if(ptr1 == NULL)
			break;

		ptr2 = strstr(ptr1, end);
		if(ptr2 == NULL)
			continue;

		memcpy(tmpbuf, ptr1, ptr2-ptr1+1);
		add_data_into_cache(&wifi_rece_data_cache, tmpbuf, ptr2-ptr1+1, DATA_TRANSFER);

		ptr = ptr2+1;
	}
	
	WifiParseReceDataStart();
}

static void WifiSendCmdProc(void)
{
	uint8_t data_type,*p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_cache(&wifi_send_cmd_cache, &p_data, &data_len, &data_type);
	if(ret)
	{
		wifi_send_data_handle(p_data, data_len);
		delete_data_from_cache(&wifi_send_cmd_cache);

		k_timer_start(&wifi_send_cmd_timer, K_MSEC(100), K_NO_WAIT);
	}
}

static void WifiSendCmdStart(void)
{
	k_timer_start(&wifi_send_cmd_timer, K_MSEC(100), K_NO_WAIT);
}

static void WifiSendCmdStop(void)
{
	k_timer_stop(&wifi_send_cmd_timer);
}

void WifiSendCmd(uint8_t *data, uint32_t datalen)
{
	int ret;

	ret = add_data_into_cache(&wifi_send_cmd_cache, data, datalen, DATA_TRANSFER);
	WifiSendCmdStart();
}

static void WifiSendDataProc(void)
{
	uint8_t data_type,*p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_cache(&wifi_send_data_cache, &p_data, &data_len, &data_type);
	if(ret)
	{
	#ifdef WIFI_DEBUG
		LOGD("send");
	#endif
		wifi_send_data_handle(p_data, data_len);
	}
}

static void WifiSendDataStart(void)
{
	k_timer_start(&wifi_send_data_timer, K_MSEC(100), K_NO_WAIT);
}

static void WifiSendDataStop(void)
{
#ifdef WIFI_DEBUG
	LOGD("begin");
#endif
	k_timer_stop(&wifi_send_data_timer);
}

void WifiSendData(uint8_t *data, uint32_t datalen)
{
	uint8_t cmdbuf[32] = {0};
	uint8_t databuf[2048] = {0};
	uint32_t len = 0;

	wifi_payload_packet_http(data, datalen, databuf, &len);
	sprintf(cmdbuf, "%s%d\r\n", WIFI_CMD_SEND_START, len);
	add_data_into_cache(&wifi_send_data_cache, cmdbuf, strlen(cmdbuf), DATA_TRANSFER);
	add_data_into_cache(&wifi_send_data_cache, databuf, len, DATA_TRANSFER);
}

static void WifiReceDataProc(void)
{
	uint8_t data_type,*p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_cache(&wifi_rece_cache, &p_data, &data_len, &data_type);
	if(ret)
	{
		wifi_receive_data_handle(p_data, data_len);
		delete_data_from_cache(&wifi_rece_cache);

		k_timer_start(&wifi_rece_data_timer, K_MSEC(50), K_NO_WAIT);
	}
}

static void WifiReceDataStart(void)
{
	k_timer_start(&wifi_rece_data_timer, K_MSEC(50), K_NO_WAIT);
}

static void WifiReceFrameData(uint8_t *data, uint32_t datalen)
{
	int ret;

	if(1)//((data[datalen-4] == 0x4F) && (data[datalen-3] == 0x4B))	//"OK"
	{
		ret = add_data_into_cache(&wifi_rece_cache, data, datalen, DATA_TRANSFER);
		WifiReceDataStart();
	}
}

static void uart_cb(struct device *x)
{
	uint8_t tmpbyte = 0;
	uint32_t len=0;

	uart_irq_update(x);

	if(uart_irq_rx_ready(x)) 
	{
		if(rece_len >= BUF_MAXSIZE)
			rece_len = 0;

		while((len = uart_fifo_read(x, &rx_buf[rece_len], BUF_MAXSIZE-rece_len)) > 0)
		{
			rece_len += len;
			k_timer_start(&wifi_rece_frame_timer, K_MSEC(20), K_NO_WAIT);
		}
	}
	
	if(uart_irq_tx_ready(x))
	{
		struct uart_data_t *buf;
		uint16_t written = 0;

		buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		/* Nothing in the FIFO, nothing to send */
		if(!buf)
		{
			uart_irq_tx_disable(x);
			return;
		}

		while(buf->len > written)
		{
			written += uart_fifo_fill(x, &buf->data[written], buf->len - written);
		}

		while (!uart_irq_tx_complete(x))
		{
			/* Wait for the last byte to get
			* shifted out of the module
			*/
		}

		if (k_fifo_is_empty(&fifo_uart_tx_data))
		{
			uart_irq_tx_disable(x);
		}

		k_free(buf);
	}
}

void WifiMsgProcess(void)
{
	static uint8_t wifi_sleep_retry = 0;
	
#ifdef CONFIG_PM_DEVICE
	if(uart_wifi_wake_flag)
	{
	#ifdef WIFI_DEBUG
		LOGD("uart_wake!");
	#endif
		uart_wifi_wake_flag = false;
		uart_wifi_sleep_out();
	}

	if(uart_wifi_sleep_flag)
	{
	#ifdef WIFI_DEBUG
		LOGD("uart_sleep!");
	#endif
		uart_wifi_sleep_flag = false;
		uart_wifi_sleep_in();
	}
#endif

	if(wifi_connect_ap_flag)
	{
		wifi_work_status = WIFI_STATUS_CONNECTING_TO_AP;
		WifiSendCmd(WIFI_CMD_CONNECT_AP, strlen(WIFI_CMD_CONNECT_AP));
		wifi_connect_ap_flag = false;
	}

	if(wifi_connect_server_flag)
	{
		wifi_work_status = WIFI_STATUS_CONNECTING_TO_SERVER;
		WifiSendCmd(WIFI_CMD_CONNECT_SERVER, strlen(WIFI_CMD_CONNECT_SERVER));
		wifi_connect_server_flag = false;
	}
	
	if(wifi_send_cmd_flag)
	{
		WifiSendCmdProc();
		wifi_send_cmd_flag = false;
	}

	if(wifi_parse_data_flag)
	{
		WifiParseReceData();
		wifi_parse_data_flag = false;
	}
	
	if(wifi_send_data_flag)
	{
	#ifdef WIFI_DEBUG
		LOGD("wifi_send_data_flag");
	#endif
		WifiSendDataProc();
		wifi_send_data_flag = false;
	}

	if(wifi_rece_data_flag)
	{
		WifiReceDataProc();
		wifi_rece_data_flag = false;
	}
	
	if(wifi_rece_frame_flag)
	{
		WifiReceFrameData(rx_buf, rece_len);
		rece_len = 0;
		wifi_rece_frame_flag = false;
	}

	if(wifi_get_infor_flag)
	{
		wifi_get_infor_flag = false;
		wifi_get_infor();
	}
	
	if(wifi_on_flag)
	{
		wifi_on_flag = false;

		if(k_timer_remaining_get(&wifi_turn_off_timer) > 0)
			k_timer_stop(&wifi_turn_off_timer);
		
		memset(&wifi_data, 0, sizeof(wifi_data));
		wifi_turn_on_and_scanning();

		if(test_wifi_flag)
		{
			k_timer_start(&wifi_rescan_timer, K_MSEC(5000), K_MSEC(5000));	
		}
	}
	
	if(wifi_off_flag)
	{
		wifi_off_flag = false;

		if(!cache_is_empty(&wifi_send_cmd_cache)
			|| (!cache_is_empty(&wifi_send_data_cache)&&(wifi_work_status == WIFI_STATUS_CONNECTED_TO_SERVER))
			)
		{
		#ifdef WIFI_DEBUG
			LOGD("wifi_off_flag 001");
		#endif
			k_timer_start(&wifi_turn_off_timer, K_SECONDS(10), K_NO_WAIT);	
		}
		else
		{
		#ifdef WIFI_DEBUG
			LOGD("wifi_off_flag 002");
		#endif
			wifi_turn_off();
		
			if(k_timer_remaining_get(&wifi_rescan_timer) > 0)
				k_timer_stop(&wifi_rescan_timer);
			if(k_timer_remaining_get(&wifi_turn_off_timer) > 0)
				k_timer_stop(&wifi_turn_off_timer);
		}
	}

	if(wifi_rescanning_flag)
	{
		wifi_rescanning_flag = false;
		wifi_rescanning();
	}

	if(wifi_off_ok_flag)
	{
		wifi_off_ok_flag = false;
		wifi_sleep_retry = 0;
		wifi_turn_off_success();
	}
	
	if(wifi_off_retry_flag)
	{
		wifi_off_retry_flag = false;
		wifi_sleep_retry++;
		if(wifi_sleep_retry > 3)
			wifi_off_ok_flag = true;
		else
			wifi_off_flag = true;
	}
	
	if(wifi_wait_timerout_flag)
	{
		wifi_wait_timerout_flag = false;
		wifi_scanned_wait_timerout();
	}
	
	if(wifi_test_update_flag)
	{
		wifi_test_update_flag = false;
		wifi_test_update();
	}	
}

void wifi_get_infor(void)
{
	static uint8_t count=0;

	count++;
	switch(count)
	{
	case 1:
		//���ù���ģʽ 1:stationģʽ 2:APģʽ 3:����AP+stationģʽ
		WifiSendCmd(WIFI_CMD_SET_CWMODE, strlen(WIFI_CMD_SET_CWMODE));
		//��ȡMac��ַ
		WifiSendCmd(WIFI_CMD_GET_MAC, strlen(WIFI_CMD_GET_MAC));
		//��ȡ�汾��Ϣ
		WifiSendCmd(WIFI_CMD_GET_VER, strlen(WIFI_CMD_GET_VER));
		k_timer_start(&wifi_get_infor_timer, K_SECONDS(3), K_NO_WAIT);
		break;
	case 2:
		//����·����
		//WifiSendCmd("AT+CWLAP=\"HUAWEI-3YN3AN\"\r\n", strlen("AT+CWLAP=\"HUAWEI-3YN3AN\"\r\n"));
		WifiSendCmd(WIFI_CMD_SEARCH_AP, strlen(WIFI_CMD_SEARCH_AP));
		k_timer_start(&wifi_get_infor_timer, K_SECONDS(5), K_NO_WAIT);
		break;
	case 3:
		//����·����
		//WifiSendCmd("AT+CWJAP=\"HUAWEI-3YN3AN\",\"km320000\"\r\n", strlen("AT+CWJAP=\"HUAWEI-3YN3AN\",\"km320000\"\r\n"));
		WifiSendCmd(WIFI_CMD_CONNECT_AP, strlen(WIFI_CMD_CONNECT_AP));
		k_timer_start(&wifi_get_infor_timer, K_SECONDS(10), K_NO_WAIT);
		break;
	case 4:
		//��ѯ�豸IP
		//WifiSendCmd("AT+CIFSR\r\n", strlen("AT+CIFSR\r\n"));
		WifiSendCmd(WIFI_CMD_CHECK_STA_IP, strlen(WIFI_CMD_CHECK_STA_IP));
		k_timer_start(&wifi_get_infor_timer, K_SECONDS(5), K_NO_WAIT);
		break;
	case 5:
		//���ӷ�����IP
		//WifiSendCmd("AT+CIPSTART=\"TCP\",\"192.168.3.30\",60000\r\n", strlen("AT+CIPSTART=\"TCP\",\"192.168.3.30\",60000\r\n"));
		WifiSendCmd(WIFI_CMD_CONNECT_SERVER, strlen(WIFI_CMD_CONNECT_SERVER));
		k_timer_start(&wifi_get_infor_timer, K_SECONDS(5), K_NO_WAIT);
		break;
	case 6:
		//�����������͵��ֽ�����
		//WifiSendData("{1:1:0:0:351358811149555:T12:460082515704981,898604A52121C0114981,72,+8,100,V3.4.1_20231123BC,1.3.5,50.5.57,1.7.4,ff:ff:ff:ff:ff:ff,V1.0.3_20230322,D9:9F:3B:71:E1:2C,20231208112905}", 
		//				strlen("{1:1:0:0:351358811149555:T12:460082515704981,898604A52121C0114981,72,+8,100,V3.4.1_20231123BC,1.3.5,50.5.57,1.7.4,ff:ff:ff:ff:ff:ff,V1.0.3_20230322,D9:9F:3B:71:E1:2C,20231208112905}")
		//				);
		SendPowerOnData();
		SendSettingsData();
	#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT)) 
		SendMissingSportData();
	#endif
	#if defined(CONFIG_PPG_SUPPORT)||defined(CONFIG_TEMP_SUPPORT)
		SendMissingHealthData();
	#endif
		break;
	}
	
	k_timer_start(&wifi_turn_off_timer, K_SECONDS(10), K_NO_WAIT);	
}

void wifi_init(void)
{
#ifdef WIFI_DEBUG
	LOGD("begin");
#endif

	uart_wifi = DEVICE_DT_GET(WIFI_DEV);
	if(!uart_wifi)
	{
	#ifdef WIFI_DEBUG
		LOGD("Could not get uart!");
	#endif
		return;
	}

	uart_irq_callback_set(uart_wifi, uart_cb);
	uart_irq_rx_enable(uart_wifi);

	gpio_wifi = DEVICE_DT_GET(WIFI_PORT);
	if(!gpio_wifi)
	{
	#ifdef WIFI_DEBUG
		LOGD("Could not get gpio!");
	#endif
		return;
	}

	gpio_pin_configure(gpio_wifi, WIFI_RST_PIN, GPIO_OUTPUT);
	gpio_pin_configure(gpio_wifi, WIFI_EN_PIN, GPIO_OUTPUT);

	gpio_pin_set(gpio_wifi, WIFI_RST_PIN, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set(gpio_wifi, WIFI_RST_PIN, 0);
	
	gpio_pin_set(gpio_wifi, WIFI_EN_PIN, 1);
	k_sleep(K_MSEC(100));

	k_timer_start(&wifi_get_infor_timer, K_SECONDS(3), K_NO_WAIT);
}
