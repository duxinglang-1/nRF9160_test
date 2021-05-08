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
#include <string.h>
#include <drivers/uart.h>
#include <dk_buttons_and_leds.h>
#include "esp8266.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(wifi_esp8266, CONFIG_LOG_DEFAULT_LEVEL);

#define UART_CTRL_PIN 	1	//拉高切换到WIFI，拉低切换到BLE
#define WIFI_DEV		"UART_0"
#define WIFI_EN_PIN		11	//WIFI EN，使用WIFI需要拉高此脚

#define BUF_MAXSIZE	1024

static struct device *uart_wifi;

static bool app_wifi_on = false;

bool sos_wait_wifi = false;
bool fall_wait_wifi = false;
bool location_wait_wifi = false;

static u32_t rece_len=0;

static u8_t rx_buf[BUF_MAXSIZE]={0};
static u8_t tx_buf[BUF_MAXSIZE]={0};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

struct uart_data_t
{
	void  *fifo_reserved;
	u8_t data[BUF_MAXSIZE];
	u16_t len;
};

static wifi_infor wifi_data = {0};

static void APP_Ask_wifi_Data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wifi_scan_timer, APP_Ask_wifi_Data_timerout, NULL);

static void APP_Ask_wifi_Data_timerout(struct k_timer *timer_id)
{
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

void wifi_get_scanned_data(u32_t count, u32_t datalen, u8_t *databuf)
{

}

void APP_Ask_wifi_data(void)
{
	u8_t i;
	u8_t *str_mac[6] = {"94:77:2b:24:22:6c","7c:94:2a:39:9f:50","7c:94:2a:39:9f:54","","",""};

#if 0
	if(!app_wifi_on)
	{
		app_wifi_on = true;
		k_timer_start(&wifi_scan_timer, K_MSEC(1*60*1000), NULL);	
	}
#else
	wifi_data.count = 3;
	for(i=0;i<wifi_data.count;i++)
	{
		wifi_data.node[i].rssi = -(50+i);
		strcpy(wifi_data.node[i].mac, str_mac[i]);
	}

	//APP_Ask_wifi_Data_timerout(NULL);
	k_timer_start(&wifi_scan_timer, K_MSEC(10*1000), NULL);
#endif	
}

void wifi_scanned_data_proc(u8_t *buf, u32_t len)
{
	
}

static void uart_receive_data(u8_t data, u32_t datalen)
{
	LOG_INF("uart_rece:%02X\n", data);

#if 0
	if(data == 0xAB)
	{
		memset(rx_buf, 0, sizeof(rx_buf));
		rece_len = 0;
	}
	
	rx_buf[rece_len++] = data;
	if(rece_len == (256*rx_buf[1]+rx_buf[2]+3))	//receivive complete
	{
		ble_receive_date_handle(rx_buf, rece_len);

		memset(rx_buf, 0, sizeof(rx_buf));
		rece_len = 0;
	}
	else				//continue receive
	{
	}
#endif	
}

static void uart_send_data(void)
{
	LOG_INF("uart_send_data\n");
	
	uart_fifo_fill(uart_wifi, "Hello World!", strlen("Hello World!"));
	uart_irq_tx_enable(uart_wifi); 
}

static void uart_cb(struct device *x)
{
	u8_t tmpbyte = 0;
	u32_t len=0;

	uart_irq_update(x);

	if(uart_irq_rx_ready(x)) 
	{
		while((len = uart_fifo_read(x, &tmpbyte, 1)) > 0)
			uart_receive_data(tmpbyte, 1);
	}
	
	if(uart_irq_tx_ready(x))
	{
		struct uart_data_t *buf;
		u16_t written = 0;

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

void wifi_init(void)
{
	LOG_INF("ble_init\n");
	
	uart_wifi = device_get_binding(WIFI_DEV);
	if(!uart_wifi)
	{
		LOG_INF("Could not get %s device\n", WIFI_DEV);
		return;
	}

	uart_irq_callback_set(uart_wifi, uart_cb);
	uart_irq_rx_enable(uart_wifi);
}

