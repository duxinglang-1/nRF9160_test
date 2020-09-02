#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <dk_buttons_and_leds.h>
#include "key.h"
#include "Max20353.h"

#define TIMER_FOR_LONG_PRESSED 1000

#define KEY_DOWN 		1
#define KEY_UP 			0
#define KEY_LONG_PRESS	2

#define KEY_1			BIT(0)
#define KEY_2			BIT(1)
#define KEY_3			BIT(2)
#define KEY_4			BIT(3)
#define KEY_SOS			BIT(4)	//SOS 26
#define KEY_PWR			BIT(5)	//power 15

extern bool lcd_sleep_in;
extern bool lcd_sleep_out;
extern bool lcd_is_sleeping;
extern bool sys_pwr_off;

static struct k_timer g_long_press_timer_id;
static u8_t flag;
static u32_t keycode;
static u32_t keytype;

static void key_event_handler(uint8_t key_code, uint8_t key_type)
{
	//printk("key_code:%d, key_type:%d, KEY_1:%d,KEY_2:%d,KEY_3:%d,KEY_4:%d,KEY_5:%d,KEY_6:%d\n", 
	//						key_code, key_type,
	//						KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6);

	switch(key_code)
	{
	case KEY_1:
		switch(key_type)
		{
		case KEY_DOWN:
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			break;
		}
		break;

	case KEY_2:
		switch(key_type)
		{
		case KEY_DOWN:
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			break;
		}		
		break;

	case KEY_3:
		switch(key_type)
		{
		case KEY_DOWN:
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			break;
		}		
		break;

	case KEY_4:
		switch(key_type)
		{
		case KEY_DOWN:
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			break;
		}
		break;
	case KEY_SOS:
		switch(key_type)
		{
		case KEY_DOWN:
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			break;
		}
		break;
	case KEY_PWR:
		switch(key_type)
		{
		case KEY_DOWN:
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			sys_pwr_off = true;
			break;
		}
		break;	
	}

	//Any key will wakeup lcd
	lcd_sleep_out = 1;
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

	printk("key_init\n");
	
	err = dk_buttons_init(button_handler);
	if (err)
	{
		printk("Could not initialize buttons, err code: %d\n", err);
		return;
	}

	k_timer_init(&g_long_press_timer_id, long_press_timer_handler, NULL);
}