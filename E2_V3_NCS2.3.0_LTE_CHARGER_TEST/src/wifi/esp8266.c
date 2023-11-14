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

//#define WIFI_DEBUG

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
static bool wifi_send_data_flag = false;
static bool wifi_rece_data_flag = false;
static bool wifi_rece_frame_flag = false;

static CacheInfo wifi_send_cache = {0};
static CacheInfo wifi_rece_cache = {0};

static wifi_infor wifi_data = {0};

static void WifiSendDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_send_data_timer, WifiSendDataCallBack, NULL);
static void WifiReceDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_rece_data_timer, WifiReceDataCallBack, NULL);
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

static void WifiSendDataCallBack(struct k_timer *timer)
{
	wifi_send_data_flag = true;
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
	WifiSendData(cmd, strlen(cmd));//发送命令
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
	Send_Cmd_To_Esp8285("AT+CWMODE=3\r\n",100);
	//设置AT+CWLAP信号的排序方式：按RSSI排序，只显示信号强度和MAC模式
	Send_Cmd_To_Esp8285("AT+CWLAPOPT=1,12\r\n",50);
	//启动扫描
	Send_Cmd_To_Esp8285("AT+CWLAP\r\n",0);
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
}

void wifi_rescanning(void)
{
	if(!wifi_is_on)
		return;

	//设置AT+CWLAP信号的排序方式：按RSSI排序，只显示信号强度和MAC模式
	Send_Cmd_To_Esp8285("AT+CWLAPOPT=1,12\r\n",50);
	Send_Cmd_To_Esp8285("AT+CWLAP\r\n", 0);
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
	uint8_t count = 0;
	uint8_t tmpbuf[256] = {0};
	uint8_t *ptr = buf;
	uint8_t *ptr1 = NULL;
	uint8_t *ptr2 = NULL;
	bool flag = false;

#ifdef WIFI_DEBUG	
	LOGD("receive:%s", buf);
#endif

	if(strstr(ptr, WIFI_SLEEP_REPLY))
	{
		wifi_off_ok_flag = true;
		return;
	}

	if(strstr(ptr, WIFI_GET_MAC_REPLY))
	{
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

	if(strstr(ptr, WIFI_GET_VER))
	{
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

		wifi_off_flag = true;
		return;
	}

	if(strstr(ptr,WIFI_DATA_HEAD))
	{
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
			ptr1 = strstr(ptr,WIFI_DATA_RSSI_BEGIN);         //取字符串中的,之后的字符
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

#ifdef CONFIG_PM_DEVICE
	uart_wifi_sleep_out();
#endif

	uart_fifo_fill(uart_wifi, buf, len);
	uart_irq_tx_enable(uart_wifi);
}

static void WifiSendDataProc(void)
{
	uint8_t data_type,*p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_cache(&wifi_send_cache, &p_data, &data_len, &data_type);
	if(ret)
	{
		wifi_send_data_handle(p_data, data_len);
		delete_data_from_cache(&wifi_send_cache);

		k_timer_start(&wifi_send_data_timer, K_MSEC(50), K_NO_WAIT);
	}
}

static void WifiSendDataStart(void)
{
	k_timer_start(&wifi_send_data_timer, K_MSEC(50), K_NO_WAIT);
}

void WifiSendData(uint8_t *data, uint32_t datalen)
{
	int ret;

	ret = add_data_into_cache(&wifi_send_cache, data, datalen, DATA_TRANSFER);
	WifiSendDataStart();
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

	if((data[datalen-4] == 0x4F) && (data[datalen-3] == 0x4B))	//"OK"
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
			k_timer_start(&wifi_rece_frame_timer, K_MSEC(10), K_NO_WAIT);
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

	if(wifi_send_data_flag)
	{
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
		wifi_turn_off();
		
		if(k_timer_remaining_get(&wifi_rescan_timer) > 0)
			k_timer_stop(&wifi_rescan_timer);
		if(k_timer_remaining_get(&wifi_turn_off_timer) > 0)
			k_timer_stop(&wifi_turn_off_timer);
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
	//设置工作模式 1:station模式 2:AP模式 3:兼容AP+station模式
	Send_Cmd_To_Esp8285("AT+CWMODE=3\r\n",50);
	//获取Mac地址
	Send_Cmd_To_Esp8285(WIFI_GET_MAC_CMD,50);
	//获取版本信息
	Send_Cmd_To_Esp8285(WIFI_GET_VER,50);

	k_timer_start(&wifi_turn_off_timer, K_SECONDS(5), K_NO_WAIT);	
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
