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

#define LSM6DSO_INT1_PIN	09
#define LSM6DSO_INT2_PIN	10
#define LSM6DSO_SDA_PIN		11
#define LSM6DSO_SCL_PIN		12

#define BUT_PIN		6  //pin for button1-press interrupt

#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PULL_UP DT_ALIAS_SW0_GPIOS_FLAGS
#else
#define PULL_UP 0
#endif

#define EDGE (GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE)

static axis3bit16_t data_raw_acceleration;
static axis3bit16_t data_raw_angular_rate;

stmdev_ctx_t dev_ctx;
lsm6dso_pin_int1_route_t int1_route;
lsm6dso_pin_int2_route_t int2_route;
lsm6dso_all_sources_t all_source;

static float acceleration_mg[3];
static float angular_rate_mdps[3];
static float acceleration_g[3];
static float angular_rate_dps[3];

static uint8_t whoamI, rst;
struct device *LSM6DSO_I2C;
uint8_t is_tilt;
static uint16_t steps; //step counter
struct device *interrupt;
struct device *gpiob;
static struct gpio_callback gpio_cb;
static struct gpio_callback gpio_cb2;

volatile bool single_tap_event = false;
volatile bool button_flag = false;

uint8_t init_i2c(void)
{
	LSM6DSO_I2C = device_get_binding(I2C_DEV);
	if(!LSM6DSO_I2C)
	{
		printf("ERROR SETTING UP I2C\r\n");
		return -1;
	}
	else
	{
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
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(LSM6DSO_I2C, data, len+1, LSM6DSO_I2C_ADD);

	return rslt;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;

	rslt = i2c_write(LSM6DSO_I2C, &reg, 1, LSM6DSO_I2C_ADD);
	if(rslt == 0)
	{
		rslt = i2c_read(LSM6DSO_I2C, bufp, len, LSM6DSO_I2C_ADD);
	}

	return rslt;
}

void interrupt_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	single_tap_event = true;
}

void button_pressed(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
	printf("Button pressed\n");
	button_flag = true;
}

uint8_t init_gpio(void)
{
	interrupt = device_get_binding(DT_GPIO_P0_DEV_NAME);
	gpiob = device_get_binding(DT_GPIO_P0_DEV_NAME);

	gpio_pin_configure(interrupt, LSM6DSO_INT2_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_DOWN | GPIO_INT_ACTIVE_HIGH)); //configure the single tap/tilt interrupt
	gpio_pin_configure(gpiob, BUT_PIN, (GPIO_DIR_IN | GPIO_INT | PULL_UP | EDGE | GPIO_INT_ACTIVE_LOW | GPIO_INT_ACTIVE_HIGH));      //configure the button1-press interrupt

	gpio_init_callback(&gpio_cb, interrupt_event, BIT(LSM6DSO_INT2_PIN));
	gpio_init_callback(&gpio_cb2, button_pressed, BIT(BUT_PIN));

	gpio_add_callback(interrupt, &gpio_cb);
	gpio_add_callback(gpiob, &gpio_cb2);

	gpio_pin_enable_callback(gpiob, BUT_PIN);
	gpio_pin_enable_callback(interrupt,LSM6DSO_INT2_PIN);

	printf("GPIO Init Done\r\n");

	return 0;
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
	}while(rst);

	lsm6dso_i3c_disable_set(&dev_ctx, LSM6DSO_I3C_DISABLE);

	lsm6dso_xl_full_scale_set(&dev_ctx, LSM6DSO_2g);
	lsm6dso_gy_full_scale_set(&dev_ctx, LSM6DSO_250dps);
	lsm6dso_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

	lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_417Hz);
	lsm6dso_gy_data_rate_set(&dev_ctx, LSM6DSO_GY_ODR_417Hz);

	lsm6dso_tap_detection_on_z_set(&dev_ctx, PROPERTY_ENABLE);
	lsm6dso_tap_detection_on_y_set(&dev_ctx, PROPERTY_ENABLE);
	lsm6dso_tap_detection_on_x_set(&dev_ctx, PROPERTY_ENABLE);

	lsm6dso_tap_threshold_x_set(&dev_ctx, 0x09);
	lsm6dso_tap_threshold_y_set(&dev_ctx, 0x09);
	lsm6dso_tap_threshold_z_set(&dev_ctx, 0x09);

	lsm6dso_tap_quiet_set(&dev_ctx, 0x03);
	lsm6dso_tap_shock_set(&dev_ctx, 0x03);

	lsm6dso_tap_mode_set(&dev_ctx, LSM6DSO_ONLY_SINGLE);

	lsm6dso_tilt_sens_set(&dev_ctx, PROPERTY_ENABLE); 

	lsm6dso_pedo_sens_set(&dev_ctx, LSM6DSO_PEDO_BASE_MODE);

	lsm6dso_pin_int2_route_get(&dev_ctx, &int2_route);
	int2_route.md2_cfg.int2_single_tap = PROPERTY_ENABLE;
	int2_route.emb_func_int2.int2_tilt = PROPERTY_ENABLE;
	lsm6dso_pin_int2_route_set(&dev_ctx, &int2_route);

	printf("Sensor Init Done\r\n");
}

void test_sensor(void)
{
	u8_t tmpbuf[128] = {0};

	sprintf(tmpbuf, "test_motion");
	LCD_ShowString(20,80,tmpbuf);
	
	init_i2c();
	init_gpio();
	
	dev_ctx.write_reg = platform_write;
	dev_ctx.read_reg = platform_read;
	dev_ctx.handle = &LSM6DSO_I2C;  

	sensor_init();
	lsm6dso_steps_reset(&dev_ctx); //reset step counter
	
	while(0)
	{
		if(single_tap_event)
		{		
			lsm6dso_tilt_flag_data_ready_get(&dev_ctx, &is_tilt); //is_tilt = true when a tilt is detected
			if (is_tilt)
			{
				printf("tilt detected\n");	//¼ì²âµ½·­µ½
			}
			else
			{
				printf("tap detected\n");	//¼ì²âµ½´¥Åö
			}
			single_tap_event = false;
		}

		if(button_flag)
		{
			lsm6dso_number_of_steps_get(&dev_ctx, (uint8_t*)&steps);
			printf("steps :%d\r\n", steps);
			button_flag = false;
		}
	}
}

void motion_sensor_msg_proc(void)
{
	u8_t tmpbuf[128] = {0};
	extern bool lcd_sleep_out;
	
	if(single_tap_event)
	{		
		single_tap_event = false;
		
		lsm6dso_tilt_flag_data_ready_get(&dev_ctx, &is_tilt); //is_tilt = true when a tilt is detected
		if(is_tilt)
		{
			printf("tilt detected\n");	//¼ì²âµ½·­µ½

			lcd_sleep_out = true;
			sprintf(tmpbuf, "tilt trige lcd sleep out!");
			LCD_ShowString(20,80,tmpbuf);

		}
		else
		{
			printf("tap detected\n");	//¼ì²âµ½´¥Åö
		}
	}

	if(button_flag)
	{
		button_flag = false;
		
		lsm6dso_number_of_steps_get(&dev_ctx, (uint8_t*)&steps);
		printf("steps :%d\r\n", steps);

		sprintf(tmpbuf, "step is:%d", steps);
		LCD_ShowString(20,100,tmpbuf);
	}
}