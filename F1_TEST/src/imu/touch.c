
#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(touch, CONFIG_LOG_DEFAULT_LEVEL);

#define Touch_Pin     6

struct device *gpiob;
volatile bool touch_flag = false;
static uint8_t val = 0U;
static u8_t init = false;

bool is_wearing(void)
{
#if 0	//xb add 2020-12-11 JPR项目没有脱腕检测，默认一直佩戴
	return true;
#else
	if(!init)
	{
		init = true;
		gpiob = device_get_binding(DT_GPIO_P0_DEV_NAME);
		gpio_pin_configure(gpiob, Touch_Pin, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_DOWN | GPIO_INT_ACTIVE_HIGH));
	}
	
	gpio_pin_read(gpiob, Touch_Pin, &val);

	if(val == 0)
	{
		LOG_INF("is wearing!\n");
		return true;
	}
	else if (val == 1)
	{
		LOG_INF("not wearing!\n");
		return false;
	}
#endif
}
