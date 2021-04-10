/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <init.h>
#include <hal/nrf_gpio.h>
#include "elp_connect.h"

#include <drivers/uart.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define BUF_MAXSIZE 1024

static u32_t rece_len=0;

static u8_t rx_buf[BUF_MAXSIZE]={0};
static u8_t tx_buf[BUF_MAXSIZE]={0};

static struct device *uart_ble;

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

struct uart_data_t
{
	void  *fifo_reserved;
	u8_t    data[BUF_MAXSIZE];
	u16_t   len;
};

static void data_handler(u8_t cmd_type, const u8_t *data_buf, u8_t data_len)
{
  u8_t i;

  for(i=0;i<data_len;i++)
  {
      LOG_INF("len:%d, data[%d]:%02x", data_len, i, data_buf[i]);   
  }

   // LOG_INF("%s, %s, %d\n",cmd_type,data_buf,data_len);
}

void start_execute(void)
{
    int err;
    LOG_INF("here is to start uart example\n");
    err = inter_connect_init(data_handler,UART_0);
    if (err) {
	LOG_ERR("Init inter_connect error: %d", err);
	return;
    }	
}

void ble_send_date_handle(u8_t *buf, u32_t len)
{
	LOG_INF("ble_send_date_handle\n");

	uart_fifo_fill(uart_ble, buf, len);
	uart_irq_tx_enable(uart_ble); 
}

static void uart_receive_data(u8_t data, u32_t datalen)
{
	LOG_INF("uart_rece:%02X\n", data);
	
	rx_buf[rece_len++] = data;
	//if(rece_len == (256*rx_buf[1]+rx_buf[2]+3))	//receivive complete
	//{
	//	ble_receive_date_handle(rx_buf, rece_len);

	//	memset(rx_buf, 0, sizeof(rx_buf));
	//	rece_len = 0;
	//}
	//else				//continue receive
	//{
	//}
}

static void uart_cb(struct device *x)
{
	u8_t tmpbyte = 0;
	u32_t len=0;

	//LOG_INF("uart_cb\n");

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

void ble_init(void)
{
	LOG_INF("ble_init\n");
	
	uart_ble = device_get_binding("UART_0");
	if(!uart_ble)
	{
		LOG_INF("Could not get %s device\n", uart_ble);
		return;
	}

	uart_irq_callback_set(uart_ble, uart_cb);
	uart_irq_rx_enable(uart_ble);
}

void main(void)
{
	//start_execute();
	ble_init();

    while(1)
    {
        k_sleep(K_MSEC(1000));    
    }
}

