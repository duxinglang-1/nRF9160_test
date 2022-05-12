/****************************************Copyright (c)************************************************
** File Name:			    Key.c
** Descriptions:			Key message process source file
** Created By:				xie biao
** Created Date:			2020-07-13
** Modified Date:      		2021-09-29 
** Version:			    	V1.2
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
#include "logger.h"

//#define KEY_DEBUG
#define WEAR_CHECK_SUPPORT	//脱腕检测功能

static bool key_trigger_flag = false;
static bool wear_off_trigger_flag = false;

static u8_t flag;
static u32_t keycode;
static u32_t keytype;

static KEY_STATUS state;
static button_handler_t button_handler_cb;
static atomic_t my_buttons;
static sys_slist_t button_handlers;

#ifdef WEAR_CHECK_SUPPORT
#define WEAR_PORT 	"GPIO_0"
#define WEAR_PIN	06
static struct device *gpio_wear;
static struct gpio_callback gpio_wear_cb;
static void wear_off_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(wear_off_timer, wear_off_timerout, NULL);
#endif

#define SOS			BIT(0)
#define POW			BIT(1)

static const key_cfg button_pins[] = 
{
	{DT_ALIAS_SW0_GPIOS_CONTROLLER, 26, ACTIVE_LOW},
	{DT_ALIAS_SW0_GPIOS_CONTROLLER, 15, ACTIVE_LOW},
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

static key_event_msg key_msg = {0};

void ClearAllKeyHandler(void)
{
	u8_t i,j;

	for(i=0;i<KEY_MAX;i++)
	{
		for(j=0;j<KEY_EVENT_MAX;j++)
		{
			key_msg.flag[i][j] = false;
			key_msg.func[i][j] = NULL;
		}
	}
}

void SetKeyHandler(FuncPtr funcPtr, u8_t keycode, u8_t keytype)
{
	key_msg.func[keycode][keytype] = funcPtr;
}

void ClearKeyHandler(u8_t keycode, u8_t keytype)
{
	key_msg.func[keycode][keytype] = NULL;
}

void SetLeftKeyUpHandler(FuncPtr funcPtr)
{
	key_msg.func[KEY_SOFT_LEFT][KEY_EVENT_UP] = funcPtr;
}

void SetLeftKeyDownHandler(FuncPtr funcPtr)
{
	key_msg.func[KEY_SOFT_LEFT][KEY_EVENT_DOWN] = funcPtr;
}

void SetLeftKeyLongPressHandler(FuncPtr funcPtr)
{
	key_msg.func[KEY_SOFT_LEFT][KEY_EVENT_LONG_PRESS] = funcPtr;
}

void ClearLeftKeyUpHandler(void)
{
	key_msg.func[KEY_SOFT_LEFT][KEY_EVENT_UP] = NULL;
}

void ClearLeftKeyDownHandler(void)
{
	key_msg.func[KEY_SOFT_LEFT][KEY_EVENT_DOWN] = NULL;
}

void ClearLeftKeyLongPressHandler(void)
{
	key_msg.func[KEY_SOFT_LEFT][KEY_EVENT_LONG_PRESS] = NULL;
}

void SetRightKeyUpHandler(FuncPtr funcPtr)
{
	key_msg.func[KEY_SOFT_RIGHT][KEY_EVENT_UP] = funcPtr;
}

void SetRightKeyDownHandler(FuncPtr funcPtr)
{
	key_msg.func[KEY_SOFT_RIGHT][KEY_EVENT_DOWN] = funcPtr;
}

void SetRightKeyLongPressHandler(FuncPtr funcPtr)
{
	key_msg.func[KEY_SOFT_RIGHT][KEY_EVENT_LONG_PRESS] = funcPtr;
}

void ClearRightKeyUpHandler(void)
{
	key_msg.func[KEY_SOFT_RIGHT][KEY_EVENT_UP] = NULL;
}

void ClearRightKeyDownHandler(void)
{
	key_msg.func[KEY_SOFT_RIGHT][KEY_EVENT_DOWN] = NULL;
}

void ClearRightKeyLongPressHandler(void)
{
	key_msg.func[KEY_SOFT_RIGHT][KEY_EVENT_LONG_PRESS] = NULL;
}

FuncPtr GetKeyHandler(u8_t keycode, u8_t keytype)
{
    FuncPtr ptr;
    
    if(keycode >= KEY_MAX)
    {
        ptr = NULL;
    }
    else
    {
        if (keytype < KEY_EVENT_MAX)
        {
            ptr = (key_msg.func[keycode][keytype]);
        }
        else
        {
            ptr = NULL;
        }
    }

    return ptr;
}

void ExecKeyHandler(u8_t keycode, u8_t keytype)
{
	u8_t i;
	FuncPtr curr_func_ptr;

	for(i=0;i<KEY_MAX;i++)
	{
		if(BIT(i) == keycode)
			break;
	}

	if(i >= KEY_MAX)
		return;

	curr_func_ptr = GetKeyHandler(i, keytype);
	if(curr_func_ptr)
	{  
		key_msg.flag[i][keytype] = true;
		key_trigger_flag = true;
	}
}

bool is_wearing(void)
{
	return touch_flag;
}

static void key_event_handler(u8_t key_code, u8_t key_type)
{
#ifdef KEY_DEBUG
	LOGD("key_code:%d, key_type:%d, KEY_SOS:%d", key_code, key_type, KEY_SOS);
#endif

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

	LCD_ResetBL_Timer();

	ExecKeyHandler(key_code, key_type);

	if(alarm_is_running)
	{
		AlarmRemindStop();
	}

	if(find_is_running)
	{
		FindDeviceStop();
	}
}

static void button_handler(u32_t button_state, u32_t has_changed)
{
	int err_code;

	u32_t buttons = (button_state & has_changed);

#ifdef KEY_DEBUG
	LOGD("button_state:%d, has_changed:%d", button_state, has_changed);
#endif

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
			return err;
		}
	}

	err = set_trig_mode(GPIO_INT_LEVEL);
	if(err)
	{
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
			return err;
		}
	}

	k_delayed_work_init(&buttons_scan, buttons_scan_fn);

	state = STATE_SCANNING;

	err = k_delayed_work_submit(&buttons_scan, 0);
	if(err)
	{
		return err;
	}

	read_buttons(NULL, NULL);

	return 0;
}

#ifdef WEAR_CHECK_SUPPORT
static void wear_off_timerout(struct k_timer *timer_id)
{
#ifdef KEY_DEBUG
	LOGD("begin!");
#endif

	wear_off_trigger_flag = true;
}

void WearInterruptHandle(void)
{
	u32_t val;

	if(gpio_pin_read(gpio_wear, WEAR_PIN, &val))
		return;

	if(!val)
	{
	#ifdef KEY_DEBUG
		LOGD("wear on!");
	#endif
		touch_flag = true;
		k_timer_stop(&wear_off_timer);
	}
	else
	{
	#ifdef KEY_DEBUG
		LOGD("wear off!");
	#endif
		touch_flag = false;
		k_timer_start(&wear_off_timer, K_MSEC(2000), NULL);
	}
}

void wear_init(void)
{
	bool rst;
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_INT_DOUBLE_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_LOW|GPIO_INT_DEBOUNCE;

  	//端口初始化
  	gpio_wear = device_get_binding(WEAR_PORT);
	if(!gpio_wear)
		return;

	//wear interrupt
	gpio_pin_configure(gpio_wear, WEAR_PIN, flag);
	gpio_pin_disable_callback(gpio_wear, WEAR_PIN);
	gpio_init_callback(&gpio_wear_cb, WearInterruptHandle, BIT(WEAR_PIN));
	gpio_add_callback(gpio_wear, &gpio_wear_cb);
	gpio_pin_enable_callback(gpio_wear, WEAR_PIN);

	WearInterruptHandle();
}
#endif

void KeyMsgProcess(void)
{
	u8_t i,j;

	if(key_trigger_flag)
	{
		for(i=0;i<KEY_MAX;i++)
		{
			for(j=0;j<KEY_EVENT_MAX;j++)
			{
				if(key_msg.flag[i][j] == true && key_msg.func[i][j] != NULL)
				{
					key_msg.func[i][j]();
					key_msg.flag[i][j] = false;
					break;
				}
			}
		}
		
		key_trigger_flag = false;
	}
	
	if(wear_off_trigger_flag)
	{
		if(0
		#ifdef CONFIG_PPG_SUPPORT
			|| PPGIsWorking()
		#endif
		#ifdef CONFIG_TEMP_SUPPORT
			|| TempIsWorking()
		#endif
			)
		{
			EnterIdleScreen();
		}
	
		wear_off_trigger_flag = false;
	}
}

void key_init(void)
{
	int err;
	u32_t ret = 0;

	err = buttons_init(button_handler);
	if (err)
	{
		return;
	}

	k_timer_init(&g_long_press_timer_id, long_press_timer_handler, NULL);

#ifdef WEAR_CHECK_SUPPORT
	wear_init();
#endif	
}
