#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <dk_buttons_and_leds.h>
#include "key.h"

#define TIMER_FOR_LONG_PRESSED 1000

#define KEY_DOWN 		1
#define KEY_UP 			0
#define KEY_LONG_PRESS	2

#define KEY_1			BIT(0)
#define KEY_2			BIT(1)
#define KEY_3			BIT(2)
#define KEY_4			BIT(3)

extern bool lcd_sleep_in;
extern bool lcd_sleep_out;
extern bool lcd_is_sleeping;

static struct k_timer g_long_press_timer_id;
static u8_t flag;
static u32_t keycode;
static u32_t keytype;

static void key_event_handler(uint8_t key_code, uint8_t key_type)
{
	switch(key_code)
	{
	case KEY_1:
		switch(key_type)
		{
		case KEY_DOWN:
			dk_set_led(DK_LED1,1);
			break;
		case KEY_UP:
			dk_set_led(DK_LED1,0);
			break;
		case KEY_LONG_PRESS:
			break;
		}
		break;

	case KEY_2:
		switch(key_type)
		{
		case KEY_DOWN:
			dk_set_led(DK_LED2,1);
			break;
		case KEY_UP:
			dk_set_led(DK_LED2,0);
			break;
		case KEY_LONG_PRESS:
			break;
		}		
		break;

	case KEY_3:
		switch(key_type)
		{
		case KEY_DOWN:
			dk_set_led(DK_LED3,1);
			break;
		case KEY_UP:
			dk_set_led(DK_LED3,0);
			break;
		case KEY_LONG_PRESS:
			break;
		}		
		break;

	case KEY_4:
		switch(key_type)
		{
		case KEY_DOWN:
			dk_set_led(DK_LED4,1);	
			break;
		case KEY_UP:
			dk_set_led(DK_LED4,0);
			break;
		case KEY_LONG_PRESS:
			break;
		}
		break;		
	}

	if(key_type == KEY_LONG_PRESS)
	{
		dk_set_leds_state(DK_ALL_LEDS_MSK,DK_NO_LEDS_MSK);
	}
	
	if(key_type == KEY_UP)
	{
		dk_set_leds_state(DK_NO_LEDS_MSK,DK_ALL_LEDS_MSK);
		
		//if(lcd_is_sleeping)
		//	lcd_sleep_out = 1;
		//else
		//	lcd_sleep_in = 1;
	}
}

static void button_handler(u32_t button_state, u32_t has_changed)
{
	int err_code;

	u32_t buttons = (button_state & has_changed);

	keycode = has_changed;
	keytype = button_state/has_changed;
	
	switch(keytype)
	{
	case KEY_DOWN:
		k_timer_start(&g_long_press_timer_id, K_MSEC(TIMER_FOR_LONG_PRESSED), K_NO_WAIT);
		break;
		
	case KEY_UP:
		k_timer_stop(&g_long_press_timer_id);
		break;

	case KEY_LONG_PRESS:
		break;
	}

	key_event_handler(keycode, keytype);
}

static void long_press_timer_handler(struct k_timer *timer)
{
    key_event_handler(keycode, KEY_LONG_PRESS);
}


void key_init(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err)
	{
		printk("Could not initialize buttons, err code: %d\n", err);
		return;
	}

	k_timer_init(&g_long_press_timer_id, long_press_timer_handler, NULL);
}