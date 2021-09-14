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

#define UART_CTRL_PIN 	1	//拉高切换到WIFI，拉低切换到BLE
#define WIFI_EN_PIN		11	//WIFI EN，使用WIFI需要拉高此脚
#define CTRL_GPIO		"GPIO_0"

#define BUF_MAXSIZE	1024

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

static wifi_infor wifi_data = {0};

u8_t wifi_test_info[256] = {0};

static void APP_Ask_wifi_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_scan_timer, APP_Ask_wifi_Data_timerout, NULL);
static void wifi_rescan_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_rescan_timer, wifi_rescan_timerout, NULL);

static void APP_Ask_wifi_Data_timerout(struct k_timer *timer_id)
{
	LOG_INF("[%s]\n", __func__);
	app_wifi_on = false;
	wifi_turn_off();

	if(sos_wait_wifi)
	{
		sos_get_wifi_data_reply(wifi_data);	
		sos_wait_wifi = false;
	}

	if(fall_wait_wifi)
	{
		fall_get_wifi_data_reply(wifi_data);	
		fall_wait_wifi = false;
	}

	if(location_wait_wifi)
	{
		location_get_wifi_data_reply(wifi_data);
		location_wait_wifi = false;
	}
}

static void wifi_rescan_timerout(struct k_timer *timer_id)
{
	LOG_INF("[%s]\n", __func__);
	wifi_rescanning_flag = true;
}

void wifi_get_scanned_data(void)
{
	app_wifi_on = false;
	
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
	u8_t i;
	u8_t *str_mac[6] = {"94:77:2b:24:22:6c","7c:94:2a:39:9f:50","7c:94:2a:39:9f:54","","",""};
	u8_t *str_rssi[6] = {"-54","-87","-62","","",""};

#if 1
	LOG_INF("[%s]\n", __func__);

	if(!app_wifi_on)
	{
		app_wifi_on = true;
		memset(&wifi_data, 0, sizeof(wifi_data));
		
		wifi_turn_on_and_scanning();
		k_timer_start(&wifi_scan_timer, K_MSEC(30*1000), NULL);	
	}
#else
	wifi_data.count = 3;
	for(i=0;i<wifi_data.count;i++)
	{
		strcpy(wifi_data.node[i].rssi, str_rssi[i]);
		strcpy(wifi_data.node[i].mac, str_mac[i]);
	}

	k_timer_start(&wifi_scan_timer, K_MSEC(10*1000), NULL);
#endif	
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
			LOG_INF("Could not get %s device\n", CTRL_GPIO);
			return;
		}
	}
	
    gpio_pin_configure(ctrl_switch, UART_CTRL_PIN, GPIO_DIR_OUT);
    gpio_pin_write(ctrl_switch, UART_CTRL_PIN, 0);

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
			LOG_INF("Could not get %s device\n", CTRL_GPIO);
			return;
		}
	}
	
	gpio_pin_configure(ctrl_switch, UART_CTRL_PIN, GPIO_DIR_OUT);
	//gpio_pin_write(ctrl_switch, UART_CTRL_PIN, 1);
	gpio_pin_write(ctrl_switch, UART_CTRL_PIN, 0);

	blue_is_on = false;
	wifi_is_on = true;
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
	ctrl_switch = device_get_binding(CTRL_GPIO);
	if(!ctrl_switch)
	{
		LOG_INF("Could not get %s device\n", CTRL_GPIO);
		return;
	}

	gpio_pin_configure(ctrl_switch, WIFI_EN_PIN, GPIO_DIR_OUT);
	//gpio_pin_write(ctrl_switch, WIFI_EN_PIN, 1);
	gpio_pin_write(ctrl_switch, WIFI_EN_PIN, 0);
}

/*============================================================================
* Function Name  : wifi_disable
* Description    : Esp8285_EN使能禁止，低电平有效
* Input          : None
* Output         : None
* Return         : None
* CALL           : 可被外部调用
==============================================================================*/
void wifi_disable(void)
{
	ctrl_switch = device_get_binding(CTRL_GPIO);
	if(!ctrl_switch)
	{
		LOG_INF("Could not get %s device\n", CTRL_GPIO);
		return;
	}

	gpio_pin_configure(ctrl_switch, WIFI_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(ctrl_switch, WIFI_EN_PIN, 0);
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
	Send_Cmd_To_Esp8285("AT+CWLAPOPT=1,12\r\n",30);
	//Send_Cmd_To_Esp8285("AT+CWLAPOPT=1,4\r\n",30);
	k_sleep(K_MSEC(100));
	Send_Cmd_To_Esp8285("AT+CWLAP\r\n",50);
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
	k_sleep(K_MSEC(20));
	wifi_enable();
	wifi_start_scanning();
}

void wifi_turn_off(void)
{
	LOG_INF("[%s]\n", __func__);

	wifi_disable();
	switch_to_ble();
}

void wifi_rescanning(void)
{
	LOG_INF("[%s] wifi_is_on:%d\n", __func__, wifi_is_on);

	if(!wifi_is_on)
		return;

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
	u8_t *ptr = buf;
	u8_t *ptr1 = NULL;
	u8_t *ptr2 = NULL;

	LOG_INF("[%s] receive:%s\n", __func__, buf);
	
	while(1)
	{
		//rssi
		ptr1 = strstr(ptr,"-");         //取字符串中的,之后的字符
		if(ptr1 == NULL)
			break;
		ptr2 = strstr(ptr1+1,",");
		if(ptr2 == NULL)
			break;
		
		memcpy(wifi_data.node[wifi_data.count].rssi, ptr1+1, ptr2 - (ptr1+1));

		//MAC
		ptr1 = strstr(ptr2,"\"");
		if(ptr1 == NULL)
			break;
		ptr2 = strstr(ptr1+1,"\"");
		if(ptr2 == NULL)
			break;
		
		memcpy(wifi_data.node[wifi_data.count].mac, ptr1+1, ptr2 - (ptr1+1));

		wifi_data.count++;
		if(wifi_data.count == MAX_SCANNED_WIFI_NODE)
			break;
		
		ptr = ptr2+1;
		if(*ptr == 0x00)
			break;
	}

	wifi_get_scanned_data();

	LOG_INF("[%s] test_wifi_flag:%d\n", __func__, test_wifi_flag);
	if(test_wifi_flag)
	{
		u8_t i,j;
		u8_t tmpbuf[128] = {0};

		LOG_INF("[%s] 001\n", __func__);
		sprintf(wifi_test_info, "%02d,", wifi_data.count);
		
		for(i=0,j=0;i<wifi_data.count;i++)
		{
			u8_t buf[128] = {0};

			j++;
		#if defined(LCD_VGM068A4W01_SH1106G)||defined(LCD_VGM096064A6W01_SP5090)
			if(j<12)
			{
				sprintf(buf, "%02d|", atoi(wifi_data.node[i].rssi));
				strcat(tmpbuf, buf);
			}
		#else
			sprintf(buf, "%s|%02d;", wifi_data.node[i].mac, atoi(wifi_data.node[i].rssi));
			strcat(tmpbuf, buf);
		#endif
		}
		
		strcat(wifi_test_info, tmpbuf);
		wifi_test_update_flag = true;
	}
	else
	{
		LOG_INF("[%s] 002\n", __func__);
		wifi_turn_off();
	}
}

void MenuStartWifi(void)
{
	LOG_INF("[%s]\n", __func__);
	wifi_on_flag = true;
	test_wifi_flag = true;
}

void MenuStopWifi(void)
{
	LOG_INF("[%s]\n", __func__);
	//wifi_off_flag = true;
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
		LOG_INF("[%s] wifi_off_flag:%d\n", __func__, wifi_off_flag);
		wifi_off_flag = false;
		wifi_turn_off();
		
		if(k_timer_remaining_get(&wifi_rescan_timer) > 0)
		{
			k_timer_stop(&wifi_rescan_timer);
		}
	}
	if(wifi_rescanning_flag)
	{
		wifi_rescanning_flag = false;
		wifi_rescanning();
	}
	if(wifi_test_update_flag)
	{
		wifi_test_update_flag = false;
		wifi_test_update();
	}	
}
