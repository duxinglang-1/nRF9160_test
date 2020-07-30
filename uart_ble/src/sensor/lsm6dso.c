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
#include "lsm6dso_reg.h"

typedef union{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

#ifdef CONFIG_SOC_NRF9160
#define I2C_DEV "I2C_1"
#else
#define I2C_DEV "I2C_1"
#endif

#define LSM6DSO_I2C_ADD     LSM6DSO_I2C_ADD_L >> 1 //need to shift 1 bit to the right.

static axis3bit16_t data_raw_acceleration;
static axis3bit16_t data_raw_angular_rate;

stmdev_ctx_t dev_ctx;
lsm6dso_pin_int1_route_t int1_route;

static float acceleration_mg[3];
static float angular_rate_mdps[3];
static float acceleration_g[3];
static float angular_rate_dps[3];

static uint8_t whoamI, rst;
struct device *LSM6DSO_I2C;

uint8_t init_i2c(){
  LSM6DSO_I2C = device_get_binding(I2C_DEV);
  if (!LSM6DSO_I2C) {
    printf("ERROR SETTING UP I2C\r\n");
    return -1;
  } else {
    i2c_configure(LSM6DSO_I2C, I2C_SPEED_SET(I2C_SPEED_FAST));
    printf("I2C CONFIGURED\r\n");
    return 0;
  }
}

static int32_t platform_write(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
  uint32_t rslt = 0;
  uint8_t data[len+1];
  data[0] = reg;
  for(uint8_t i = 0; i < len; i++){
    data[i+1] = bufp[i];
  }
  rslt = i2c_write(LSM6DSO_I2C, data, len+1, LSM6DSO_I2C_ADD);

  return rslt;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;

	rslt = i2c_write(LSM6DSO_I2C, &reg, 1, LSM6DSO_I2C_ADD);
	if(rslt == 0){
		rslt = i2c_read(LSM6DSO_I2C, bufp, len, LSM6DSO_I2C_ADD);
	}
	
	return rslt;
}

void sensor_init(void)
{
	lsm6dso_device_id_get(&dev_ctx, &whoamI);
	if(whoamI != LSM6DSO_ID)
		while(1);

	lsm6dso_reset_set(&dev_ctx, PROPERTY_ENABLE);
	do 
	{
		lsm6dso_reset_get(&dev_ctx, &rst);
	}while (rst);

	lsm6dso_i3c_disable_set(&dev_ctx, LSM6DSO_I3C_DISABLE);

	//max supported frames at 3KB FIFO, 512*6 = 3072. Each frame has 6 axis and each axis is 2 byte in size.
	lsm6dso_fifo_watermark_set(&dev_ctx, 500);
	lsm6dso_fifo_stop_on_wtm_set(&dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_mode_set(&dev_ctx, LSM6DSO_FIFO_MODE);

	lsm6dso_data_ready_mode_set(&dev_ctx, LSM6DSO_DRDY_PULSED);

	lsm6dso_pin_int1_route_get(&dev_ctx, &int1_route);
	int1_route.int1_ctrl.int1_fifo_th = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&dev_ctx, &int1_route);

	lsm6dso_fifo_xl_batch_set(&dev_ctx, LSM6DSO_XL_BATCHED_AT_104Hz);
	lsm6dso_fifo_gy_batch_set(&dev_ctx, LSM6DSO_GY_BATCHED_AT_104Hz);

	lsm6dso_xl_full_scale_set(&dev_ctx, LSM6DSO_2g);
	lsm6dso_gy_full_scale_set(&dev_ctx, LSM6DSO_250dps);
	lsm6dso_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
	lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_104Hz);
	lsm6dso_gy_data_rate_set(&dev_ctx, LSM6DSO_GY_ODR_104Hz);
}

void test_sensor(void)
{
	init_i2c();

	dev_ctx.write_reg = platform_write;
	dev_ctx.read_reg = platform_read;
	dev_ctx.handle = &LSM6DSO_I2C;  

	sensor_init();

	while(1)
	{
		uint16_t num = 0;
		uint8_t waterm = 0;
		lsm6dso_fifo_tag_t reg_tag;
		axis3bit16_t dummy;

		lsm6dso_fifo_wtm_flag_get(&dev_ctx, &waterm);
		if(waterm>0)
		{
			lsm6dso_fifo_data_level_get(&dev_ctx, &num);
			printf("NUM: %d\r\n", num);

			while(num--)
			{
				lsm6dso_fifo_sensor_tag_get(&dev_ctx, &reg_tag);
				switch (reg_tag)
				{
				case LSM6DSO_XL_NC_TAG:
					memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
					lsm6dso_fifo_out_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
					acceleration_mg[0] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[0]);
					acceleration_mg[1] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[1]);
					acceleration_mg[2] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[2]);

					acceleration_g[0]   = acceleration_mg[0]/1000;
					acceleration_g[1]   = acceleration_mg[1]/1000;
					acceleration_g[2]   = acceleration_mg[2]/1000;

					printf("No.: %d, Axyz: %4.2f, %4.2f, %4.2f\r\n", 
							num, acceleration_g[0], acceleration_g[1], acceleration_g[2]);
					break;

				case LSM6DSO_GYRO_NC_TAG:
					memset(data_raw_angular_rate.u8bit, 0x00, 3*sizeof(int16_t));
					lsm6dso_fifo_out_raw_get(&dev_ctx, data_raw_angular_rate.u8bit);
					angular_rate_mdps[0] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[0]);
					angular_rate_mdps[1] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[1]);
					angular_rate_mdps[2] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[2]);

					angular_rate_dps[0] = angular_rate_mdps[0]/1000;
					angular_rate_dps[1] = angular_rate_mdps[1]/1000;
					angular_rate_dps[2] = angular_rate_mdps[2]/1000;

					printf("No.: %d, Gxyz: %4.2f, %4.2f, %4.2f\r\n",
							num, angular_rate_dps[0], angular_rate_dps[1], angular_rate_dps[2]);
					break;

				default:
					memset(dummy.u8bit, 0x00, 3 * sizeof(int16_t));
					lsm6dso_fifo_out_raw_get(&dev_ctx, dummy.u8bit);
					break;
				}
			}
		}
	}
}