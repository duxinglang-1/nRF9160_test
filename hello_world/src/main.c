/*
* Copyright (c) 2019 Nordic Semiconductor ASA
*
* SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
*/
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <dk_buttons_and_leds.h>

#define MY_PORT	"GPIO_0"

/* Interval in milliseconds between each time status LEDs are updated. */
#define LEDS_UPDATE_INTERVAL	        500

/* Interval in microseconds between each time LEDs are updated when indicating
 * that an error has occurred.
 */
#define LEDS_ERROR_UPDATE_INTERVAL      250000

#define BUTTON_1			BIT(0)
#define BUTTON_2			BIT(1)
#define SWITCH_1			BIT(2)
#define SWITCH_2			BIT(3)

#define LED_ON(x)			(x)
#define LED_BLINK(x)			((x) << 8)
#define LED_GET_ON(x)			((x) & 0xFF)
#define LED_GET_BLINK(x)		(((x) >> 8) & 0xFF)

struct display_led_blink
{
  int led_on_mask;
  int led_off_mask;
};

static int index=0,flag=0,step=1;
static int delay_ms=100;

static struct display_led_blink led_blink_loop[4] = {{DK_LED1_MSK, DK_LED2_MSK | DK_LED3_MSK | DK_LED4_MSK},{DK_LED2_MSK, DK_LED1_MSK | DK_LED3_MSK | DK_LED4_MSK},{DK_LED4_MSK, DK_LED1_MSK | DK_LED2_MSK | DK_LED3_MSK},{DK_LED3_MSK, DK_LED1_MSK | DK_LED2_MSK | DK_LED4_MSK}};
static struct k_timer led_blink_timer;

struct device *gpio_lcd;

/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(u32_t button_state, u32_t has_changed)
{
  u32_t buttons = (button_state & has_changed);

  if (buttons & BUTTON_1) 
  {
    if(flag == 1)
    {
      flag = 0;
    }
    else
    {
      if(step == 1)
      {
        if(delay_ms < 2000)
          delay_ms += 100;
        else
          step = 0;
      }
        
      if(step == 0)
      {
        if(delay_ms >= 200)
          delay_ms -= 100;
        else
          step = 1;
      }
    }
  }

  if (buttons & BUTTON_2) 
  {
    if(flag == 0)
    {
      flag = 1;
    }
    else
    {
      if(step == 1)
      {
        if(delay_ms < 2000)
          delay_ms += 100;
        else
          step = 0;
      }
        
      if(step == 0)
      {
        if(delay_ms >= 200)
          delay_ms -= 100;
        else
          step = 1;
      }
    }
  }
}

/**@brief Initializes buttons and LEDs, using the DK buttons and LEDs
 * library.
 */
static void buttons_leds_init(void)
{
  int err;

  err = dk_buttons_init(button_handler);
  if (err)
  {
    printk("Could not initialize buttons, err code: %d\n", err);
  }

  err = dk_leds_init();
  if (err)
  {
    printk("Could not initialize leds, err code: %d\n", err);
  }

  err = dk_set_leds_state(0x00, DK_ALL_LEDS_MSK);
  if (err)
  {
    printk("Could not set leds state, err code: %d\n", err);
  }
}

static void led_blink_show_timerout(struct k_timer *timer)
{
  printk("led_blink_show_timerout\n");

  if(flag == 1)
  {
    index++;
    if(index >= 4)
      index = 0;
  }
  else
  {
    if(index == 0)
      index = 4;
    index--;
  }
  
  dk_set_leds_state(led_blink_loop[index].led_on_mask, led_blink_loop[index].led_off_mask);

  //k_timer_stop(&led_blink_timer);
  k_timer_start(&led_blink_timer, K_MSEC(delay_ms), K_NO_WAIT);
}

void led_blink_test_main(void)
{
  buttons_leds_init();

  k_timer_init(&led_blink_timer, led_blink_show_timerout, NULL);
  k_timer_start(&led_blink_timer, K_MSEC(delay_ms), K_NO_WAIT);
}

void main(void)
{
	printk("Application started\n");

 	//¶Ë¿Ú³õÊ¼»¯
  	gpio_lcd = device_get_binding(MY_PORT);
	if(!gpio_lcd)
	{
		printk("Cannot bind gpio device\n");
		return;
	}

#if 1	//xb test
	gpio_pin_configure(gpio_lcd, 0, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 0, 1);
	gpio_pin_write(gpio_lcd, 0, 0);
	gpio_pin_configure(gpio_lcd, 1, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 1, 1);
	gpio_pin_write(gpio_lcd, 1, 0);
	gpio_pin_configure(gpio_lcd, 2, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 2, 1);
	gpio_pin_write(gpio_lcd, 2, 0);
	gpio_pin_configure(gpio_lcd, 3, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 3, 1);
	gpio_pin_write(gpio_lcd, 3, 0);
	gpio_pin_configure(gpio_lcd, 4, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 4, 1);
	gpio_pin_write(gpio_lcd, 4, 0);
	
	gpio_pin_configure(gpio_lcd, 5, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 5, 1);
	gpio_pin_write(gpio_lcd, 5, 0);
	gpio_pin_configure(gpio_lcd, 6, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 6, 1);
	gpio_pin_write(gpio_lcd, 6, 0);
	gpio_pin_configure(gpio_lcd, 7, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 7, 1);
	gpio_pin_write(gpio_lcd, 7, 0);
	gpio_pin_configure(gpio_lcd, 8, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 8, 1);
	gpio_pin_write(gpio_lcd, 8, 0);
	gpio_pin_configure(gpio_lcd, 9, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 9, 1);
	gpio_pin_write(gpio_lcd, 9, 0);
	
	gpio_pin_configure(gpio_lcd, 10, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 10, 1);
	gpio_pin_write(gpio_lcd, 10, 0);
	gpio_pin_configure(gpio_lcd, 11, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 11, 1);
	gpio_pin_write(gpio_lcd, 11, 0);	
	gpio_pin_configure(gpio_lcd, 12, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 12, 1);
	gpio_pin_write(gpio_lcd, 12, 0);
	gpio_pin_configure(gpio_lcd, 13, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 13, 1);
	gpio_pin_write(gpio_lcd, 13, 0);	
	gpio_pin_configure(gpio_lcd, 14, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 14, 1);
	gpio_pin_write(gpio_lcd, 14, 0);
	
	gpio_pin_configure(gpio_lcd, 15, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 15, 1);
	gpio_pin_write(gpio_lcd, 15, 0);	
	gpio_pin_configure(gpio_lcd, 16, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 16, 1);
	gpio_pin_write(gpio_lcd, 16, 0);	
	gpio_pin_configure(gpio_lcd, 17, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 17, 1);
	gpio_pin_write(gpio_lcd, 17, 0);	
	gpio_pin_configure(gpio_lcd, 18, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 18, 1);
	gpio_pin_write(gpio_lcd, 18, 0);
	gpio_pin_configure(gpio_lcd, 19, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 19, 1);
	gpio_pin_write(gpio_lcd, 19, 0);	
	
	gpio_pin_configure(gpio_lcd, 20, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 20, 1);
	gpio_pin_write(gpio_lcd, 20, 0);	
	gpio_pin_configure(gpio_lcd, 21, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 21, 1);
	gpio_pin_write(gpio_lcd, 21, 0);	
	gpio_pin_configure(gpio_lcd, 22, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 22, 1);
	gpio_pin_write(gpio_lcd, 22, 0);
	gpio_pin_configure(gpio_lcd, 23, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 23, 1);
	gpio_pin_write(gpio_lcd, 23, 0);
	gpio_pin_configure(gpio_lcd, 24, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 24, 1);
	gpio_pin_write(gpio_lcd, 24, 0);

	gpio_pin_configure(gpio_lcd, 25, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 25, 1);
	gpio_pin_write(gpio_lcd, 25, 0);
	gpio_pin_configure(gpio_lcd, 26, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 26, 1);
	gpio_pin_write(gpio_lcd, 26, 0);
	gpio_pin_configure(gpio_lcd, 27, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 27, 1);
	gpio_pin_write(gpio_lcd, 27, 0);	
	gpio_pin_configure(gpio_lcd, 28, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 28, 1);
	gpio_pin_write(gpio_lcd, 28, 0);	
	gpio_pin_configure(gpio_lcd, 29, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 29, 1);
	gpio_pin_write(gpio_lcd, 29, 0);

	gpio_pin_configure(gpio_lcd, 30, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 30, 1);
	gpio_pin_write(gpio_lcd, 30, 0);	
	gpio_pin_configure(gpio_lcd, 31, GPIO_DIR_OUT);
	gpio_pin_write(gpio_lcd, 31, 1);
	gpio_pin_write(gpio_lcd, 31, 0);
#endif

  led_blink_test_main();
}