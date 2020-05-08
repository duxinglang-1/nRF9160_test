/*
* Copyright (c) 2019 Nordic Semiconductor ASA
*
* SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
*/
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <dk_buttons_and_leds.h>

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

  led_blink_test_main();
}