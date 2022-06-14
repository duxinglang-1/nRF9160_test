/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <string.h>

#define MY_PORT	"GPIO_0"
#define FLAG	(GPIO_DIR_OUT)
#define GPIO_LEVEL	0

struct device *gpio_lcd;
static struct gpio_callback gpio_cb1;

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", err);
}

void int_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	;
}

void main(void)
{
	u8_t i;
	
	//¶Ë¿Ú³õÊ¼»¯
	gpio_lcd = device_get_binding(MY_PORT);
	if(!gpio_lcd)
	{
		printk("Cannot bind gpio device\n");
		return;
	}

#if 1	//xb test
	for(i=0;i<32;i++)
	{
		gpio_pin_configure(gpio_lcd, i, FLAG);
		//gpio_pin_disable_callback(gpio_lcd, i);
		//gpio_init_callback(&gpio_cb1, int_event, BIT(i));
		//gpio_add_callback(gpio_lcd, &gpio_cb1);
		//gpio_pin_enable_callback(gpio_lcd, i);
	}

	for(i=0;i<32;i++)
	{
		gpio_pin_write(gpio_lcd, i, GPIO_LEVEL);
	}
#endif

	while(1)
	{
		//printk("The AT host sample started\n");
		k_cpu_idle();
	}
}
