/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <drivers/uart.h>

#include "elp_connect.h"

#define CONFIG_UART_0_NAME	 "UART_0"
#define CONFIG_UART_1_NAME	 "UART_1"
#define CONFIG_UART_2_NAME	 "UART_2"
#define CONFIG_UART_3_NAME	 "UART_3"
#define CONFIG_INTER_CONNECT_UART_BUF_SIZE	1024

LOG_MODULE_REGISTER(conn, CONFIG_LOG_DEFAULT_LEVEL);

#if 0
/** @brief UARTs. */
enum select_uart {
	UART_0,
	UART_1,
	UART_2,
	UART_3
};
#endif

static data_handler_t data_handler_cb;
static struct device *uart_dev0;
static struct device *uart_dev1;
static struct device *uart_dev2;
static struct device *uart_dev3;
static u8_t rx_buff0[CONFIG_INTER_CONNECT_UART_BUF_SIZE];
static u8_t tx_buff0[CONFIG_INTER_CONNECT_UART_BUF_SIZE];
static u8_t rx_buff1[CONFIG_INTER_CONNECT_UART_BUF_SIZE];
static u8_t tx_buff1[CONFIG_INTER_CONNECT_UART_BUF_SIZE];
static u8_t rx_buff2[CONFIG_INTER_CONNECT_UART_BUF_SIZE];
static u8_t tx_buff2[CONFIG_INTER_CONNECT_UART_BUF_SIZE];
static u8_t rx_buff3[CONFIG_INTER_CONNECT_UART_BUF_SIZE];
static u8_t tx_buff3[CONFIG_INTER_CONNECT_UART_BUF_SIZE];

static bool module0_initialized;
static bool module1_initialized;
static bool module2_initialized;
static bool module3_initialized;

static struct k_work rx0_data_handle_work;
static struct k_work rx1_data_handle_work;
static struct k_work rx2_data_handle_work;
static struct k_work rx3_data_handle_work;
#define DEBUG_UART 1


static void rx0_data_handle(struct k_work *work)
{      
	ARG_UNUSED(work);
	if (data_handler_cb == NULL) {
		LOG_ERR("Not itialized");
		return;
	}

#if DEBUG_UART
	//printk("here to print received data\n");
	LOG_INF("here to print received data\n");
	//LOG_INF("%s\n", (char *)rx_buff0);
	printk("%s\n", (char *)rx_buff0);	
    memset(rx_buff0, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
#else
	data_handler_cb(rx_buff0[PROT_TYP_POS],
			&rx_buff0[PROT_LEN_POS+1],
			rx_buff0[PROT_LEN_POS]);
#endif
	uart_irq_rx_enable(uart_dev0);
}

static void rx1_data_handle(struct k_work *work)
{
        
	ARG_UNUSED(work);
        

	if (data_handler_cb == NULL) {
		LOG_ERR("Not itialized");
		return;
	}

#if DEBUG_UART
	//printk("here to print received data\n");
	printk("%s\n", (char *)rx_buff1);
        memset(rx_buff1, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);

#else
	data_handler_cb(rx_buff1[PROT_TYP_POS],
			&rx_buff1[PROT_LEN_POS+1],
			rx_buff1[PROT_LEN_POS]);
#endif
	uart_irq_rx_enable(uart_dev1);
}

static void rx2_data_handle(struct k_work *work)
{
        
	ARG_UNUSED(work);
        

	if (data_handler_cb == NULL) {
		LOG_ERR("Not itialized");
		return;
	}

#if DEBUG_UART
	//printk("here to print received data\n");
	printk("%s\n", (char *)rx_buff2);
        memset(rx_buff2, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);

#else
	data_handler_cb(rx_buff2[PROT_TYP_POS],
			&rx_buff2[PROT_LEN_POS+1],
			rx_buff2[PROT_LEN_POS]);
#endif
	uart_irq_rx_enable(uart_dev2);
}

static void rx3_data_handle(struct k_work *work)
{
        
	ARG_UNUSED(work);
        

	if (data_handler_cb == NULL) {
		LOG_ERR("Not itialized");
		return;
	}

#if DEBUG_UART
	//printk("here to print received data\n");
	printk("%s\n", (char *)rx_buff3);
        memset(rx_buff3, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);

#else
	data_handler_cb(rx_buff3[PROT_TYP_POS],
			&rx_buff3[PROT_LEN_POS+1],
			rx_buff3[PROT_LEN_POS]);
#endif
	uart_irq_rx_enable(uart_dev3);
}

int inter_connect_send( enum select_uart uart_sel, u8_t *data_buff, u8_t data_len)
{
#if 0
  uart_poll_out(uart_dev, data_len);
#else
	if(data_len == 0) 
		return -1;
	for (size_t i = 0; i < data_len; i++) {

		switch (uart_sel) {
		case UART_0:
			uart_poll_out(uart_dev0, data_buff[i]);
			break;
		case UART_1:
			uart_poll_out(uart_dev1, data_buff[i]);
			break;
		case UART_2:
			uart_poll_out(uart_dev2, data_buff[i]);
			break;
		case UART_3:
			uart_poll_out(uart_dev3, data_buff[i]);
			break;
		default:
			LOG_ERR("Unknown UART instance %d", uart_sel);
			return -EINVAL;
		
	}
	}

	memset(data_buff, 0x00, data_len);
#endif

        return 0;
}


static void uart0_rx_handler(u8_t character)
{   
	static size_t cmd_len0;
	size_t pos;
#if 0 //DEBUG_UART
	LOG_INF("received %c", character );
#endif
	pos = cmd_len0;
	cmd_len0 += 1;
	/* Detect buffer overflow or zero length */
	if (cmd_len0 > CONFIG_INTER_CONNECT_UART_BUF_SIZE) {
		LOG_ERR("Buffer overflow, dropping '%c'", character);
		cmd_len0 = CONFIG_INTER_CONNECT_UART_BUF_SIZE;
		return;
	} else if (cmd_len0 < 1) {
		LOG_ERR("Invalid packet length: %d", cmd_len0);
		cmd_len0 = 0;
		return;
	}
	rx_buff0[pos] = character;
	/* Check if all packet is received. */
#if DEBUG_UART
	if(rx_buff0[pos] =='\n')
	{
		memset(tx_buff0, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		memcpy(tx_buff0, rx_buff0, cmd_len0);
		//printk("to send\n");
		inter_connect_send(UART_0, tx_buff0, cmd_len0);
		//k_sleep(100);
		goto send;
	}
#else
	if (pos == (PROT_LEN_POS + pkt_length + 1)) {
		goto send;
	}
#endif

	return;

send:
	uart_irq_rx_disable(uart_dev0);
	k_work_submit(&rx0_data_handle_work);
	cmd_len0 = 0;
}



static void uart1_rx_handler(u8_t character)
{

    
	static size_t cmd_len1;
  
	size_t pos;
#if DEBUG_UART
	LOG_INF("received:%02X", character);
#endif

	pos = cmd_len1;
	cmd_len1 += 1;


	/* Detect buffer overflow or zero length */
	if (cmd_len1 > CONFIG_INTER_CONNECT_UART_BUF_SIZE) {
		LOG_ERR("Buffer overflow, dropping '%c'", character);
		cmd_len1 = CONFIG_INTER_CONNECT_UART_BUF_SIZE;
		return;
	} else if (cmd_len1 < 1) {
		LOG_ERR("Invalid packet length: %d", cmd_len1);
		cmd_len1 = 0;
		return;
	}

	rx_buff1[pos] = character;

	/* Check if all packet is received. */
	#if DEBUG_UART
	if(rx_buff1[pos] =='\n')
	{
		memset(tx_buff1, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		memcpy(tx_buff1, rx_buff1, cmd_len1);
		//printk("to send\n");
		inter_connect_send(UART_1, tx_buff1, cmd_len1);
		//k_sleep(100);
		goto send;
	}

	#else
	if (pos == (PROT_LEN_POS + pkt_length + 1)) {
		goto send;
	}
	#endif
	return;

send:
	uart_irq_rx_disable(uart_dev1);
	k_work_submit(&rx1_data_handle_work);
	cmd_len1 = 0;

}


static void uart2_rx_handler(u8_t character)
{

    
	static size_t cmd_len2;
  
	size_t pos;
	#if 0//DEBUG_UART
	LOG_INF("received %c", character );
	#endif

	pos = cmd_len2;
	cmd_len2 += 1;


	/* Detect buffer overflow or zero length */
	if (cmd_len2 > CONFIG_INTER_CONNECT_UART_BUF_SIZE) {
		LOG_ERR("Buffer overflow, dropping '%c'", character);
		cmd_len2 = CONFIG_INTER_CONNECT_UART_BUF_SIZE;
		return;
	} else if (cmd_len2 < 1) {
		LOG_ERR("Invalid packet length: %d", cmd_len2);
		cmd_len2 = 0;
		return;
	}

	rx_buff2[pos] = character;

	/* Check if all packet is received. */
	#if DEBUG_UART
	if(rx_buff2[pos] =='\n')
	{
		memset(tx_buff2, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		memcpy(tx_buff2, rx_buff2, cmd_len2);
		//printk("to send\n");
		inter_connect_send(UART_2, tx_buff2, cmd_len2);
		//k_sleep(100);
		goto send;
	}

	#else
	if (pos == (PROT_LEN_POS + pkt_length + 1)) {
		goto send;
	}
	#endif
	return;

send:
	uart_irq_rx_disable(uart_dev2);
	k_work_submit(&rx2_data_handle_work);
	cmd_len2 = 0;

}



static void uart3_rx_handler(u8_t character)
{

    
	static size_t cmd_len3;
  
	size_t pos;
	#if 0//DEBUG_UART
	LOG_INF("received %c", character );
	#endif

	pos = cmd_len3;
	cmd_len3 += 1;


	/* Detect buffer overflow or zero length */
	if (cmd_len3 > CONFIG_INTER_CONNECT_UART_BUF_SIZE) {
		LOG_ERR("Buffer overflow, dropping '%c'", character);
		cmd_len3 = CONFIG_INTER_CONNECT_UART_BUF_SIZE;
		return;
	} else if (cmd_len3 < 1) {
		LOG_ERR("Invalid packet length: %d", cmd_len3);
		cmd_len3 = 0;
		return;
	}

	rx_buff3[pos] = character;

	/* Check if all packet is received. */
	#if DEBUG_UART
	if(rx_buff3[pos] =='\n')
	{
		memset(tx_buff3, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		memcpy(tx_buff3, rx_buff3, cmd_len3);
		//printk("to send\n");
		inter_connect_send(UART_3, tx_buff3, cmd_len3);
		//k_sleep(100);
		goto send;
	}

	#else
	if (pos == (PROT_LEN_POS + pkt_length + 1)) {
		goto send;
	}
	#endif
	return;

send:
	uart_irq_rx_disable(uart_dev3);
	k_work_submit(&rx3_data_handle_work);
	cmd_len3 = 0;

}


static void if0_isr(struct device *dev)
{
	u8_t character;
	u32_t i,len=0;
	u8_t tmpbuf[256] = {0};

	uart_irq_update(dev);
	if(uart_irq_rx_ready(dev))
	{
		/* keep reading until drain all */
		while (uart_fifo_read(dev, &character, 1))
		{
			//uart0_rx_handler(character);
			tmpbuf[len++] = character;
		}

		for(i=0;i<len;i++)
		{
			LOG_INF("len:%d, data[%d]:%02x", len, i, tmpbuf[i]);  
		}
	}
}

static void if1_isr(struct device *dev)
{
	u8_t character;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		/* keep reading until drain all */
		while (uart_fifo_read(dev, &character, 1)) {
			uart1_rx_handler( character);
		}
	}
}

static void if2_isr(struct device *dev)
{
	u8_t character;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		/* keep reading until drain all */
		while (uart_fifo_read(dev, &character, 1)) {
			uart2_rx_handler( character);
		}
	}
}

static void if3_isr(struct device *dev)
{
	u8_t character;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		/* keep reading until drain all */
		while (uart_fifo_read(dev, &character, 1)) {
			uart3_rx_handler(character);
		}
	}
}
#if 0
static int if_uart_init(char *uart_dev_name)
{
	int err;
	uart_dev = device_get_binding(uart_dev_name);
	if (uart_dev == NULL) {
		LOG_ERR("Cannot bind %s", uart_dev_name);
		return -EINVAL;
	}
	err = uart_err_check(uart_dev);
	if (err) {
		LOG_WRN("UART check failed: %d", err);
	}
	uart_irq_callback_set(uart_dev, if_isr);
	return 0;
}
#endif

int inter_connect_init(data_handler_t data_handler, enum select_uart uart_sel)
{
	char *uart_dev_name;
	int err;
	enum select_uart uart_id = uart_sel;

	if (data_handler == NULL) {
		LOG_ERR("Data handler is null");
		return -EINVAL;
	}
	data_handler_cb = data_handler;

	/* Choose which UART to use */
	switch (uart_id) {
	case UART_0:

		uart_dev0 = device_get_binding(CONFIG_UART_0_NAME);
		if (uart_dev0 == NULL) {
		LOG_ERR("Cannot bind %s", CONFIG_UART_0_NAME);
		return -EINVAL;
		}
		err = uart_err_check(uart_dev0);
		if (err) {
		LOG_WRN("UART check failed: %d", err);
		}
		uart_irq_callback_set(uart_dev0, if0_isr);
		k_work_init(&rx0_data_handle_work, rx0_data_handle);
		memset(rx_buff0, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		uart_irq_rx_enable(uart_dev0);
		module0_initialized = true;
		break;

	case UART_1:
		uart_dev1 = device_get_binding(CONFIG_UART_1_NAME);
		
		if (uart_dev1 == NULL) {
		LOG_ERR("Cannot bind %s", CONFIG_UART_1_NAME);
		return -EINVAL;
		}
		err = uart_err_check(uart_dev1);
		if (err) {
		LOG_WRN("UART check failed: %d", err);
		}
	
		uart_irq_callback_set(uart_dev1, if1_isr);
		k_work_init(&rx1_data_handle_work, rx1_data_handle);
		memset(rx_buff1, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		uart_irq_rx_enable(uart_dev1);
		module1_initialized = true;
		break;
		
	case UART_2:
		uart_dev2 = device_get_binding(CONFIG_UART_2_NAME);
		
		if (uart_dev2 == NULL) {
		LOG_ERR("Cannot bind %s", CONFIG_UART_2_NAME);
		return -EINVAL;
		}
		err = uart_err_check(uart_dev2);
		if (err) {
		LOG_WRN("UART check failed: %d", err);
		}
		uart_irq_callback_set(uart_dev2, if2_isr);
		k_work_init(&rx2_data_handle_work, rx2_data_handle);
		memset(rx_buff2, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		uart_irq_rx_enable(uart_dev2);
		module2_initialized = true;
		break;
	case UART_3:
			uart_dev3 = device_get_binding(CONFIG_UART_3_NAME);
		
		if (uart_dev3 == NULL) {
		LOG_ERR("Cannot bind %s", CONFIG_UART_3_NAME);
		return -EINVAL;
		}
		err = uart_err_check(uart_dev3);
		if (err) {
		LOG_WRN("UART check failed: %d", err);
		}
		
		uart_irq_callback_set(uart_dev3, if3_isr);
		k_work_init(&rx3_data_handle_work, rx3_data_handle);
		memset(rx_buff3, 0x00, CONFIG_INTER_CONNECT_UART_BUF_SIZE);
		uart_irq_rx_enable(uart_dev3);
		module3_initialized = true;
		break;
	default:
		LOG_ERR("Unknown UART instance %d", uart_id);
		return -EINVAL;
	}


	return err;
}

int inter_connect_uninit(void)
{
	int err = 0;

#if defined(CONFIG_DEVICE_POWER_MANAGEMENT)


	err = device_set_power_state(uart_dev0, DEVICE_PM_OFF_STATE,
				NULL, NULL);
	if (err != 0) {
		LOG_WRN("Can't power off uart err=%d", err);
	}

        err = device_set_power_state(uart_dev1, DEVICE_PM_OFF_STATE,
				NULL, NULL);
	if (err != 0) {
		LOG_WRN("Can't power off uart err=%d", err);
	}

        err = device_set_power_state(uart_dev2, DEVICE_PM_OFF_STATE,
				NULL, NULL);
	if (err != 0) {
		LOG_WRN("Can't power off uart err=%d", err);
	}

        err = device_set_power_state(uart_dev3, DEVICE_PM_OFF_STATE,
				NULL, NULL);
	if (err != 0) {
		LOG_WRN("Can't power off uart err=%d", err);
	}
#endif
	return 0;
}


