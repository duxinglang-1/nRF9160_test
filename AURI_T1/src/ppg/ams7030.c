/*
Last Update: 16/12/2020 by Jiahe
this code includes wrist tilt detection, step counter
Take 26Hz data rate
*/

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcd.h"
#include "settings.h"
#include "screen.h"
#include "ams7030.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(ams7030, CONFIG_LOG_DEFAULT_LEVEL);

#define PPG_DEV "I2C_1"
#define PPG_PORT "GPIO_0"

#define AMS7030_INT1_PIN	9
#define AMS7030_INT2_PIN	10
#define AMS7030_SDA_PIN		11
#define AMS7030_SCL_PIN		12

#define AMS7030_I2C_ADD     0x30

#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PULL_UP DT_ALIAS_SW0_GPIOS_FLAGS
#else
#define PULL_UP 0
#endif

#define EDGE (GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE)


static bool int1_event = false;
static bool int2_event = false;

static u8_t whoamI, rst;

static struct device *i2c_ppg;
static struct device *gpio_ppg;
static struct gpio_callback gpio_cb1,gpio_cb2;
amsdev_ctx_t ppg_dev_ctx;

u8_t g_heart_rate = 0;

static uint8_t init_i2c(void)
{
	i2c_ppg = device_get_binding(PPG_DEV);
	if(!i2c_ppg)
	{
		LOG_INF("ERROR SETTING UP I2C\r\n");
		return -1;
	}
	else
	{
		i2c_configure(i2c_ppg, I2C_SPEED_SET(I2C_SPEED_FAST));
		return 0;
	}
}

static int32_t platform_write(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;
	uint8_t data[len+1];

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(i2c_ppg, data, len+1, AMS7030_I2C_ADD);

	return rslt;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;

	rslt = i2c_write(i2c_ppg, &reg, 1, AMS7030_I2C_ADD);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ppg, bufp, len, AMS7030_I2C_ADD);
	}

	return rslt;
}

static void interrupt_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	int2_event = true; 
}

static void step_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	int1_event = true;
}

static uint8_t init_gpio(void)
{
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_DOWN|GPIO_INT_ACTIVE_HIGH|GPIO_INT_DEBOUNCE;

	gpio_ppg = device_get_binding(PPG_PORT);
	//steps interrupt
	gpio_pin_configure(gpio_ppg, AMS7030_INT1_PIN, flag);
	gpio_pin_disable_callback(gpio_ppg, AMS7030_INT1_PIN);
	gpio_init_callback(&gpio_cb1, step_event, BIT(AMS7030_INT1_PIN));
	gpio_add_callback(gpio_ppg, &gpio_cb1);
	gpio_pin_enable_callback(gpio_ppg, AMS7030_INT1_PIN);

	//fall|tilt interrupt
	gpio_pin_configure(gpio_ppg, AMS7030_INT2_PIN, flag);
	gpio_pin_disable_callback(gpio_ppg, AMS7030_INT2_PIN);
	gpio_init_callback(&gpio_cb2, interrupt_event, BIT(AMS7030_INT2_PIN));
	gpio_add_callback(gpio_ppg, &gpio_cb2);
	gpio_pin_enable_callback(gpio_ppg, AMS7030_INT2_PIN);

	//暂时将PPG供电关闭，防止LED常亮耗电
	gpio_pin_configure(gpio_ppg, 17, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, 17, 0);
	return 0;
}

void GetHeartRate(u8_t *HR)
{
	u32_t heart;

	while(1)
	{
		heart = sys_rand32_get();
		if(((heart%200)>=60) && ((heart%200)<=160))
		{
			*HR = (heart%200);
			break;
		}
	}
}

void PPG_init(void)
{
	LOG_INF("PPG_init\n");
	
	if(init_i2c() != 0)
		return;
	
	init_gpio();

	ppg_dev_ctx.write_reg = platform_write;
	ppg_dev_ctx.read_reg = platform_read;
	ppg_dev_ctx.handle = &i2c_ppg;

	LOG_INF("PPG_init done!\n");
}

void PPGMsgProcess(void)
{
	if(int1_event)
	{
		int1_event = false;
	}

	if(int2_event)	//tilt
	{
		int2_event = false;
	}
}

