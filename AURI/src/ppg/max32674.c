/****************************************Copyright (c)************************************************
** File Name:			    max32674.c
** Descriptions:			PPG process source file
** Created By:				xie biao
** Created Date:			2021-05-19
** Modified Date:      		2021-05-19 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <nrf_socket.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <nrfx.h>
#include "Max32674.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(ppg, CONFIG_LOG_DEFAULT_LEVEL);

#define PPG_DEV 	"I2C_1"
#define PPG_PORT 	"GPIO_0"

#define PPG_SDA_PIN		30
#define PPG_SCL_PIN		31
#define PPG_INT_PIN		13
#define PPG_MFIO_PIN	14
#define PPG_RST_PIN		16
#define PPG_EN_PIN		17

#define MAX32674_REG_STATUS		0x00
#define MAX32674_I2C_ADD     0x55

#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PULL_UP DT_ALIAS_SW0_GPIOS_FLAGS
#else
#define PULL_UP 0
#endif

#define EDGE (GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE)

static bool int_event = false;

static u8_t whoamI, rst;

static struct device *i2c_ppg;
static struct device *gpio_ppg;
static struct gpio_callback gpio_cb;
ppgdev_ctx_t ppg_dev_ctx;

u8_t g_heart_rate = 0;

static u8_t init_i2c(void)
{
	i2c_ppg = device_get_binding(PPG_DEV);
	if(!i2c_ppg)
	{
		LOG_INF("ERROR SETTING UP I2C\r\n");
		return -1;
	}
	else
	{
		i2c_configure(i2c_ppg, I2C_SPEED_SET(I2C_SPEED_STANDARD));

		LOG_INF("Value of NRF_TWIM1_NS->PSEL.SCL: %ld \n",NRF_TWIM1_NS->PSEL.SCL);
		LOG_INF("Value of NRF_TWIM1_NS->PSEL.SDA: %ld \n",NRF_TWIM1_NS->PSEL.SDA);
		LOG_INF("Value of NRF_TWIM1_NS->FREQUENCY: %ld \n",NRF_TWIM1_NS->FREQUENCY);
		LOG_INF("26738688 -> 100k\n");
		LOG_INF("67108864 -> 250k\n");
		LOG_INF("104857600 -> 400k\n");	
		return 0;
	}
}

static s32_t platform_write(void *handle, u8_t cmd_family, u8_t cmd_index, u8_t* bufp, u16_t len)
{
	u32_t rslt = 0;
	u8_t data[len+2];

	data[0] = cmd_family;
	data[1] = cmd_index;
	memcpy(&data[2], bufp, len);
	rslt = i2c_write(i2c_ppg, data, len+2, MAX32674_I2C_ADD);

	return rslt;
}

static s32_t platform_read(void *handle, u8_t cmd_family, u8_t cmd_index, u8_t* bufp, u16_t len)
{
	u32_t rslt = 0;
	u8_t data[len+2];

	data[0] = cmd_family;
	data[1] = cmd_index;
	rslt = i2c_write(i2c_ppg, data, 2, MAX32674_I2C_ADD);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ppg, bufp, len, MAX32674_I2C_ADD);
	}

	return rslt;
}

static void interrupt_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	int_event = true; 
}

static uint8_t init_gpio(void)
{
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_DOWN|GPIO_INT_ACTIVE_HIGH|GPIO_INT_DEBOUNCE;

	gpio_ppg = device_get_binding(PPG_PORT);
	
	//interrupt
	gpio_pin_configure(gpio_ppg, PPG_INT_PIN, flag);
	gpio_pin_disable_callback(gpio_ppg, PPG_INT_PIN);
	gpio_init_callback(&gpio_cb, interrupt_event, BIT(PPG_INT_PIN));
	gpio_add_callback(gpio_ppg, &gpio_cb);
	gpio_pin_enable_callback(gpio_ppg, PPG_INT_PIN);

	//PPG供电打开
	gpio_pin_configure(gpio_ppg, PPG_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_EN_PIN, 1);

	//PPG模式选择(bootload\application)
	gpio_pin_configure(gpio_ppg, PPG_RST_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_ppg, PPG_MFIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 0);
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 1);
	k_sleep(K_MSEC(50));
	
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

s32_t max32674_read_reg(ppgdev_ctx_t* ctx, u8_t cmd, u8_t index, u8_t* data, u16_t len)
{
	s32_t ret;

	ret = ctx->read_reg(ctx->handle, cmd, index, data, len);
	return ret;
}

s32_t max32674_write_reg(ppgdev_ctx_t* ctx, u8_t cmd, u8_t index, u8_t* data, u16_t len)
{
	s32_t ret;

	ret = ctx->write_reg(ctx->handle, cmd, index, data, len);
	return ret;
}

void GetPPGVersion(ppgdev_ctx_t *ctx, u8_t *buff)
{
	LOG_INF("[%s]\n", __func__);
	
	max32674_read_reg(ctx, 0x81, 0x00, buff, 1);
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

	//GetPPGVersion(&ppg_dev_ctx, &whoamI);
	
	LOG_INF("PPG_init done!\n");
}

void PPGMsgProcess(void)
{
	if(int_event)
	{
		int_event = false;
	}

	//GetPPGVersion(&ppg_dev_ctx, &whoamI);
	//k_sleep(K_MSEC(1000));
}

