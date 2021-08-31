/****************************************Copyright (c)************************************************
** File Name:			    Key.c
** Descriptions:			Key message process source file
** Created By:				xie biao
** Created Date:			2020-07-13
** Modified Date:      		2020-10-10 
** Version:			    	V1.1
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <nrfx.h>
#include "key.h"
#include "Max20353.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif
#include "Alarm.h"
#include "lcd.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(key, CONFIG_LOG_DEFAULT_LEVEL);

static u8_t flag;
static u32_t keycode;
static u32_t keytype;

static KEY_STATUS state;
static button_handler_t button_handler_cb;
static atomic_t my_buttons;
static sys_slist_t button_handlers;

#define KEY_SOS			BIT(0)
#define KEY_PWR			BIT(1)
#define KEY_TOUCH		BIT(2)

static const key_cfg button_pins[] = 
{
	{DT_ALIAS_SW0_GPIOS_CONTROLLER, 26, ACTIVE_LOW},
	{DT_ALIAS_SW0_GPIOS_CONTROLLER, 15, ACTIVE_LOW},
	{DT_ALIAS_SW0_GPIOS_CONTROLLER, 06, ACTIVE_LOW},
};

static struct device *button_devs[ARRAY_SIZE(button_pins)];
static struct gpio_callback gpio_cb;
static struct k_spinlock lock;
static struct k_delayed_work buttons_scan;
static struct k_mutex button_handler_mut;
static struct k_timer g_long_press_timer_id;

static bool touch_flag = false;

bool key_pwroff_flag = false;

extern bool gps_on_flag;
extern bool ppg_fw_upgrade_flag;
extern bool ppg_start_flag;
extern bool ppg_stop_flag;
extern bool get_modem_info_flag;
extern u8_t g_ppg_trigger;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
extern bool uart_wake_flag;
extern bool uart_sleep_flag;
#endif


typedef void (*VoidFunc)(void);

VoidFunc leftkey_handler_cb = NULL,rightkey_handler_cb = NULL;

void Key_Event_register_Handler(VoidFunc leftkeyfunc,VoidFunc rightkeyfunc)
{
	leftkey_handler_cb = leftkeyfunc;
	rightkey_handler_cb = rightkeyfunc;
}

void Key_Event_Unregister_Handler(void)
{
	leftkey_handler_cb = NULL;
	rightkey_handler_cb = NULL;
}

bool is_wearing(void)
{
	return touch_flag;
}

static void key_event_handler(u8_t key_code, u8_t key_type)
{
	LOG_INF("key_code:%d, key_type:%d, KEY_SOS:%d,KEY_PWR:%d\n", key_code, key_type, KEY_SOS, KEY_PWR);
	
	if(key_code == KEY_TOUCH)
	{
		switch(key_type)
		{
		case KEY_DOWN://´÷ÉÏ
			if(!touch_flag)
			{
				touch_flag = true;
			}			
			break;
		case KEY_UP://ÍÑÏÂ
			if(touch_flag)
			{
				touch_flag = false;
			}
			break;
		case KEY_LONG_PRESS:
			break;
		}		
	}
	
	if(!system_is_completed())
		return;

	if(lcd_is_sleeping)
	{
		if(key_type == KEY_UP)
		{
			sleep_out_by_wrist = false;
			lcd_sleep_out = true;
		}
		
		return;
	}

	switch(key_code)
	{
	case KEY_SOS:
		switch(key_type)
		{
		case KEY_DOWN:
			if(leftkey_handler_cb != NULL)
				leftkey_handler_cb();
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			SOSStart();
			break;
		}
		break;
	case KEY_PWR:
		switch(key_type)
		{
		case KEY_DOWN:
			if(rightkey_handler_cb != NULL)
				rightkey_handler_cb();
			break;
		case KEY_UP:
			break;
		case KEY_LONG_PRESS:
			EnterPoweroffScreen();
			break;
		}
		break;
	}

	if(key_code != KEY_TOUCH)
	{
		if(alarm_is_running)
		{
			AlarmRemindStop();
		}

		if(find_is_running)
		{
			FindDeviceStop();
		}

		//ExitNotifyScreen();	
	}
}

static void button_handler(u32_t button_state, u32_t has_changed)
{
	int err_code;

	u32_t buttons = (button_state & has_changed);

	LOG_INF("button_state:%d, has_changed:%d\n", button_state, has_changed);

	keycode = has_changed;
	keytype = (buttons>0 ? KEY_DOWN:KEY_UP);

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

static int callback_ctrl(bool enable)
{
	int err = 0;

	/* This must be done with irqs disabled to avoid pin callback
	 * being fired before others are still not activated.
	 */
	for(size_t i = 0; (i < ARRAY_SIZE(button_pins)) && !err; i++)
	{
		if(enable)
		{
			err = gpio_pin_enable_callback(button_devs[i], button_pins[i].number);
		}
		else
		{
			err = gpio_pin_disable_callback(button_devs[i], button_pins[i].number);
		}
	}

	return err;
}

static u32_t get_buttons(void)
{
	bool actived_low;
	u32_t ret = 0;
	
	for(size_t i = 0; i < ARRAY_SIZE(button_pins); i++)
	{
		u32_t val;

		if(gpio_pin_read(button_devs[i], button_pins[i].number, &val))
		{
			LOG_INF("Cannot read gpio pin");
			return 0;
		}

		switch(button_pins[i].active_flag)
		{
		case ACTIVE_LOW:
			actived_low = true;
			break;
		case ACTIVE_HIGH:
			actived_low = false;
			break;
		}

		if((!val && actived_low)||
			(val && !actived_low))
		{
			ret |= 1U << i;
		}
	}

	return ret;
}

static void button_handlers_call(u32_t button_state, u32_t has_changed)
{
	struct button_handler *handler;

	if(button_handler_cb != NULL)
	{
		button_handler_cb(button_state, has_changed);
	}

	if(IS_ENABLED(CONFIG_DK_LIBRARY_DYNAMIC_BUTTON_HANDLERS))
	{
		k_mutex_lock(&button_handler_mut, K_FOREVER);
		SYS_SLIST_FOR_EACH_CONTAINER(&button_handlers, handler, node)
		{
			handler->cb(button_state, has_changed);
		}
		k_mutex_unlock(&button_handler_mut);
	}
}

static void buttons_scan_fn(struct k_work *work)
{
	static u32_t last_button_scan;
	static bool initial_run = true;
	u32_t button_scan;

	button_scan = get_buttons();
	atomic_set(&my_buttons, (atomic_val_t)button_scan);

	if(!initial_run)
	{
		if(button_scan != last_button_scan)
		{
			u32_t has_changed = (button_scan ^ last_button_scan);
			button_handlers_call(button_scan, has_changed);
		}
	}
	else
	{
		initial_run = false;
	}

	last_button_scan = button_scan;

	if (button_scan != 0)
	{
		int err = k_delayed_work_submit(&buttons_scan, CONFIG_DK_LIBRARY_BUTTON_SCAN_INTERVAL);
		if(err)
		{
			LOG_INF("Cannot add work to workqueue");
		}
	}
	else
	{
		/* If no button is pressed module can switch to callbacks */
		int err = 0;

		k_spinlock_key_t key = k_spin_lock(&lock);

		switch (state)
		{
		case STATE_SCANNING:
			state = STATE_WAITING;
			err = callback_ctrl(true);
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		k_spin_unlock(&lock, key);

		if(err)
		{
			LOG_INF("Cannot enable callbacks");
		}
	}
}

static int set_trig_mode(int trig_mode)
{
	int err = 0;
	int flag1 = (GPIO_PUD_PULL_UP | GPIO_INT_ACTIVE_LOW);
	int flag2 = (GPIO_PUD_PULL_DOWN | GPIO_INT_ACTIVE_HIGH);
	int flags;
	
	for(size_t i = 0; (i < ARRAY_SIZE(button_pins)) && !err; i++)
	{
		switch(button_pins[i].active_flag)
		{
		case ACTIVE_LOW:
			flags = flag1 | (GPIO_DIR_IN | GPIO_INT | trig_mode);
			break;
		case ACTIVE_HIGH:
			flags = flag2 | (GPIO_DIR_IN | GPIO_INT | trig_mode);
			break;
		}
		
		err = gpio_pin_configure(button_devs[i], button_pins[i].number, flags);
	}

	return err;
}

static void read_buttons(u32_t *button_state, u32_t *has_changed)
{
	static u32_t last_state;
	u32_t current_state = atomic_get(&my_buttons);

	if(button_state != NULL)
	{
		*button_state = current_state;
	}

	if(has_changed != NULL)
	{
		*has_changed = (current_state ^ last_state);
	}

	last_state = current_state;
}

static void button_pressed(struct device *gpio_dev, struct gpio_callback *cb, u32_t pins)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Disable GPIO interrupt */
	int err = callback_ctrl(false);

	if(err)
	{
		LOG_INF("Cannot disable callbacks");
	}

	switch (state)
	{
	case STATE_WAITING:
		state = STATE_SCANNING;
		k_delayed_work_submit(&buttons_scan, 1);
		break;

	case STATE_SCANNING:
	default:
		/* Invalid state */
		__ASSERT_NO_MSG(false);
		break;
	}

	k_spin_unlock(&lock, key);
}

static int buttons_init(button_handler_t button_handler)
{
	int err;

	button_handler_cb = button_handler;

	if(IS_ENABLED(CONFIG_DK_LIBRARY_DYNAMIC_BUTTON_HANDLERS))
	{
		k_mutex_init(&button_handler_mut);
	}

	for(size_t i = 0; i < ARRAY_SIZE(button_pins); i++)
	{
		button_devs[i] = device_get_binding(button_pins[i].port);
		if (!button_devs[i])
		{
			LOG_INF("Cannot bind gpio device");
			return -ENODEV;
		}

		switch(button_pins[i].active_flag)
		{
		case ACTIVE_LOW:
			err = gpio_pin_configure(button_devs[i], button_pins[i].number, GPIO_DIR_IN | GPIO_PUD_PULL_UP);
			break;
		case ACTIVE_HIGH:
			err = gpio_pin_configure(button_devs[i], button_pins[i].number, GPIO_DIR_IN | GPIO_PUD_PULL_DOWN);
			break;
		}

		if(err)
		{
			LOG_INF("Cannot configure button gpio");
			return err;
		}
	}

	err = set_trig_mode(GPIO_INT_LEVEL);
	if(err)
	{
		LOG_INF("Cannot set interrupt mode");
		return err;
	}

	u32_t pin_mask = 0;

	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++)
	{
		/* Module starts in scanning mode and will switch to
		 * callback mode if no button is pressed.
		 */
		err = gpio_pin_disable_callback(button_devs[i], button_pins[i].number);
		if(err)
		{
			LOG_INF("Cannot disable callbacks()");
			return err;
		}

		pin_mask |= BIT(button_pins[i].number);
	}

	gpio_init_callback(&gpio_cb, button_pressed, pin_mask);

	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++)
	{
		err = gpio_add_callback(button_devs[i], &gpio_cb);
		if(err)
		{
			LOG_INF("Cannot add callback");
			return err;
		}
	}

	k_delayed_work_init(&buttons_scan, buttons_scan_fn);

	state = STATE_SCANNING;

	err = k_delayed_work_submit(&buttons_scan, 0);
	if(err)
	{
		LOG_INF("Cannot add work to workqueue");
		return err;
	}

	read_buttons(NULL, NULL);

	return 0;
}

void key_init(void)
{
	int err;

	LOG_INF("key_init\n");
	
	err = buttons_init(button_handler);
	if (err)
	{
		LOG_INF("Could not initialize buttons, err code: %d\n", err);
		return;
	}

	k_timer_init(&g_long_press_timer_id, long_press_timer_handler, NULL);
}
