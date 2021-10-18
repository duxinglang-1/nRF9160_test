/****************************************Copyright (c)************************************************
** File Name:			    esp8266.c
** Descriptions:			wifi process source file
** Created By:				xie biao
** Created Date:			2021-03-29
** Modified Date:      		2021-03-29 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <string.h>
#include <drivers/uart.h>
#include <dk_buttons_and_leds.h>
#include "esp8266.h"
#include "uart_ble.h"
#include "screen.h"
#include "lcd.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(wifi, CONFIG_LOG_DEFAULT_LEVEL);

#define WIFI_DEBUG

#define UART_CTRL_PIN 	1	//拉低切换到WIFI，拉高切换到BLE
#define WIFI_EN_PIN		11	//WIFI EN，使用WIFI需要拉低此脚
#define CTRL_GPIO		"GPIO_0"

#define WIFI_RETRY_COUNT_MAX	5
#define BUF_MAXSIZE	1024

static u8_t retry = 0;
static struct device *ctrl_switch = NULL;

bool sos_wait_wifi = false;
bool fall_wait_wifi = false;
bool location_wait_wifi = false;
bool wifi_is_on = false;

static bool app_wifi_on = false;
static bool wifi_on_flag = false;
static bool wifi_off_flag = false;
static bool test_wifi_flag = false;
static bool wifi_test_update_flag = false;
static bool wifi_rescanning_flag = false;
static bool wifi_wait_timerout_flag = false;
static bool wifi_off_retry_flag = false;
static bool wifi_off_ok_flag = false;

static wifi_infor wifi_data = {0};

u8_t wifi_test_info[256] = {0};

static void APP_Ask_wifi_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_scan_timer, APP_Ask_wifi_Data_timerout, NULL);
static void wifi_rescan_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_rescan_timer, wifi_rescan_timerout, NULL);
static void wifi_off_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_off_retry_timer, wifi_off_timerout, NULL);

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

void wifi_scanned_wait_timerout(void)
{
	retry++;
	if(retry < WIFI_RETRY_COUNT_MAX)
	{
		wifi_rescanning_flag = true;
		memset(&wifi_data, 0, sizeof(wifi_data));
		k_timer_start(&wifi_scan_timer, K_MSEC(5000), NULL);	
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
	#ifdef CONFIG_IMU_SUPPORT
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

#ifdef CONFIG_IMU_SUPPORT
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
	LOG_INF("[%s]\n", __func__);
#endif
	if(!app_wifi_on)
	{
		retry = 0;
		app_wifi_on = true;
		memset(&wifi_data, 0, sizeof(wifi_data));
		
		wifi_turn_on_and_scanning();
		k_timer_start(&wifi_scan_timer, K_MSEC(5*1000), NULL);	
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
void Send_Cmd_To_Esp8285(u8_t *cmd, u32_t WaitTime)
{
	wifi_send_data_handle(cmd, strlen(cmd));//发送命令
	if(WaitTime > 0)
		delay_ms(WaitTime);
}

/*============================================================================
* Function Name  : switch_to_ble
* Description    : switch turn on to ble
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void switch_to_ble(void)
{
	if(blue_is_on)
		return;
	
	if(ctrl_switch == NULL)
	{
		ctrl_switch = device_get_binding(CTRL_GPIO);
		if(!ctrl_switch)
		{
		#ifdef WIFI_DEBUG
			LOG_INF("Could not get %s device\n", CTRL_GPIO);
		#endif
			return;
		}
	}
	
    gpio_pin_configure(ctrl_switch, UART_CTRL_PIN, GPIO_DIR_OUT);
    gpio_pin_write(ctrl_switch, UART_CTRL_PIN, 1);

	wifi_is_on = false;
    blue_is_on = true;
}

/*============================================================================
* Function Name  : wifi_turn_ON
* Description    : wifi on
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void switch_to_wifi(void)
{
	if(wifi_is_on)
		return;
	
	if(ctrl_switch == NULL)
	{
		ctrl_switch = device_get_binding(CTRL_GPIO);
		if(!ctrl_switch)
		{
		#ifdef WIFI_DEBUG
			LOG_INF("Could not get %s device\n", CTRL_GPIO);
		#endif
			return;
		}
	}
	
	gpio_pin_configure(ctrl_switch, UART_CTRL_PIN, GPIO_DIR_OUT);
	gpio_pin_write(ctrl_switch, UART_CTRL_PIN, 0);

	blue_is_on = false;
	wifi_is_on = true;
}

/*============================================================================
* Function Name  : wifi_enable
* Description    : Esp8285_EN使能，低电平有效
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void wifi_enable(void)
{
	if(ctrl_switch == NULL)
	{
		ctrl_switch = device_get_binding(CTRL_GPIO);
		if(!ctrl_switch)
		{
		#ifdef WIFI_DEBUG
			LOG_INF("Could not get %s device\n", CTRL_GPIO);
		#endif
			return;
		}
	}

	gpio_pin_configure(ctrl_switch, WIFI_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(ctrl_switch, WIFI_EN_PIN, 1);
	k_sleep(K_MSEC(100));
	gpio_pin_write(ctrl_switch, WIFI_EN_PIN, 0);
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
	Send_Cmd_To_Esp8285(WIFI_SLEEP_CMD,30);
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
	Send_Cmd_To_Esp8285("AT+CWMODE=3\r\n",30);
	//设置AT+CWLAP信号的排序方式：按RSSI排序，只显示信号强度和MAC模式
	Send_Cmd_To_Esp8285("AT+CWLAPOPT=1,12\r\n",30);
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
	switch_to_wifi();
	wifi_enable();
	wifi_start_scanning();
}

void wifi_turn_off_success(void)
{
#ifdef WIFI_DEBUG	
	LOG_INF("[%s]\n", __func__);
#endif

	wifi_off_retry_flag = false;
	k_timer_stop(&wifi_off_retry_timer);
	
	switch_to_ble();
}

void wifi_turn_off(void)
{
#ifdef WIFI_DEBUG	
	LOG_INF("[%s]\n", __func__);
#endif
	wifi_disable();
	k_timer_start(&wifi_off_retry_timer, K_MSEC(1000), NULL);
}

void wifi_rescanning(void)
{
	if(!wifi_is_on)
		return;

	//设置AT+CWLAP信号的排序方式：按RSSI排序，只显示信号强度和MAC模式
	Send_Cmd_To_Esp8285("AT+CWLAPOPT=1,12\r\n",30);
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
void wifi_receive_data_handle(u8_t *buf, u32_t len)
{
	u8_t count = 0;
	u8_t tmpbuf[256] = {0};
	u8_t *ptr = buf;
	u8_t *ptr1 = NULL;
	u8_t *ptr2 = NULL;
	bool flag = false;

#ifdef WIFI_DEBUG	
	LOG_INF("[%s] receive:%s\n", __func__, buf);
#endif

	if(strstr(ptr, WIFI_SLEEP_REPLY))
	{
		wifi_off_ok_flag = true;
		return;
	}
	
	while(1)
	{
		u8_t len;
	    u8_t str_rssi[8]={0};
		u8_t str_mac[32]={0};

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
			u8_t buf[128] = {0};

			count++;
		#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			if(count<12)
			{
				sprintf(buf, "%02d|", -(atoi(str_rssi)));
				strcat(tmpbuf, buf);
			}
		#else
			sprintf(buf, "%s|%02d;", str_mac, -(atoi(str_rssi)));
			strcat(tmpbuf, buf);
		#endif
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
		if(count>0)
		{
			sprintf(wifi_test_info, "%02d,", count);
			strcat(wifi_test_info, tmpbuf);
			wifi_test_update_flag = true;
		}
	}
	else
	{
		if(flag && (wifi_data.count >= 3))	//扫描有效数据
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

void wifi_test_update(void)
{
	if(screen_id == SCREEN_ID_WIFI_TEST)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void WifiProcess(void)
{
	if(wifi_on_flag)
	{
		wifi_on_flag = false;
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
	}

	if(wifi_rescanning_flag)
	{
		wifi_rescanning_flag = false;
		wifi_rescanning();
	}

	if(wifi_off_ok_flag)
	{
		wifi_off_ok_flag = false;
		wifi_turn_off_success();
	}
	
	if(wifi_off_retry_flag)
	{
		wifi_off_retry_flag = false;
		wifi_turn_off();
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
