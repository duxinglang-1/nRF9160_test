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
#define GPIO_DERECT	GPIO_DIR_OUT
#define GPIO_LEVEL	0

struct device *gpio_lcd;

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", err);
}

void main(void)
{
	//¶Ë¿Ú³õÊ¼»¯
	gpio_lcd = device_get_binding(MY_PORT);
	if(!gpio_lcd)
	{
		printk("Cannot bind gpio device\n");
		return;
	}

#if 1	//xb test
	gpio_pin_configure(gpio_lcd, 0, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 0, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 1, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 1, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 2, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 2, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 3, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 3, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 4, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 4, GPIO_LEVEL);
	
	gpio_pin_configure(gpio_lcd, 5, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 5, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 6, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 6, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 7, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 7, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 8, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 8, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 9, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 9, GPIO_LEVEL);
	
	gpio_pin_configure(gpio_lcd, 10, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 10, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 11, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 11, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 12, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 12, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 13, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 13, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 14, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 14, GPIO_LEVEL);
	
	gpio_pin_configure(gpio_lcd, 15, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 15, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 16, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 16, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 17, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 17, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 18, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 18, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 19, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 19, GPIO_LEVEL);	
	
	gpio_pin_configure(gpio_lcd, 20, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 20, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 21, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 21, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 22, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 22, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 23, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 23, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 24, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 24, GPIO_LEVEL);

	gpio_pin_configure(gpio_lcd, 25, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 25, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 26, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 26, GPIO_LEVEL);
	gpio_pin_configure(gpio_lcd, 27, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 27, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 28, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 28, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 29, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 29, GPIO_LEVEL);

	gpio_pin_configure(gpio_lcd, 30, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 30, GPIO_LEVEL);	
	gpio_pin_configure(gpio_lcd, 31, GPIO_DERECT);
	gpio_pin_write(gpio_lcd, 31, GPIO_LEVEL);
#endif

	while(1)
	{
		//printk("The AT host sample started\n");
		k_cpu_idle();
	}
}
