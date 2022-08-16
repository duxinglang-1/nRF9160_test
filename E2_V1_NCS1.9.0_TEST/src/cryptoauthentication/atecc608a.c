#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include <sys/printk.h>
#include <dk_buttons_and_leds.h>
#include "atecc608a.h"

#define CRYPTO_DEV "I2C_1"

static mcdev_ctx_t dev_ctx;

static uint8_t whoamI, rst;
struct device *ATECC608A_I2C;

static void init_i2c(void)
{
	ATECC608A_I2C = device_get_binding(CRYPTO_DEV);
	if(!ATECC608A_I2C)
	{
		return 0;
	}

	i2c_configure(ATECC608A_I2C, I2C_SPEED_SET(I2C_SPEED_FAST));
}

static int8_t writei2c(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    uint32_t rslt = 0;
    uint8_t data[len+1];
	
    data[0] = reg_addr;
	
    for(uint16_t i=0; i<len; i++)
	{
        data[i+1]=reg_data[i];
    }
	
    rslt = i2c_write(ATECC608A_I2C, data, len+1, ATECC608A_I2C_ADD);
    return rslt;
}

static int8_t readi2c(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    uint32_t rslt = 0;

    rslt = i2c_write(ATECC608A_I2C, &reg_addr, len, ATECC608A_I2C_ADD);

    if(rslt == 0)
	{
        rslt = i2c_read(ATECC608A_I2C, reg_data, len, ATECC608A_I2C_ADD);
    }
    return rslt;
}

void delay_ms(uint32_t period)
{
    k_sleep(K_MSEC(period));
}

void atecc608a_init(void)
{
	//lsm6dso_device_id_get(&dev_ctx, &whoamI);
	while(1)
	{
		readi2c(ATECC608A_I2C_ADD, ATECC608A_WHO_AM_I,&whoamI,1);
		if(whoamI != ATECC608A_ID)
			k_sleep(K_MSEC(1000));
		else
			break;
	}
}

void test_crypto(void)
{
	init_i2c();

	dev_ctx.write_reg = writei2c;
	dev_ctx.read_reg = readi2c;
	dev_ctx.handle = &ATECC608A_I2C;

	atecc608a_init();

	while(0)
	{
		k_sleep(K_MSEC(500));
	}
}

