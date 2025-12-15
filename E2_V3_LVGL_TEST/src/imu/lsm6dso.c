#ifdef CONFIG_IMU_SUPPORT

#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <nrf_socket.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lsm6dso.h"
#include "lsm6dso_reg.h"
#include "algorithm.h"
#include "lcd.h"
#include "gps.h"
#include "settings.h"
#include "screen.h"
#include "external_flash.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h" // SCC
#endif
#ifdef CONFIG_SLEEP_SUPPORT
#include "sleep.h"
#endif
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif
#include "logger.h"

//#define IMU_DEBUG

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define IMU_DEV DT_NODELABEL(i2c1)
#else
#error "i2c1 devicetree node is disabled"
#define IMU_DEV	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define IMU_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define IMU_PORT	""
#endif

#define LSM6DSO_I2C_ADD     LSM6DSO_I2C_ADD_L >> 1 //need to shift 1 bit to the right.

#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PULL_UP DT_ALIAS_SW0_GPIOS_FLAGS
#else
#define PULL_UP 0
#endif

#define EDGE (GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE)

uint16_t activity_num = 0;
uint16_t tap_count = 0;

lsm6dso_emb_fsm_enable_t fsm_enable;
uint16_t fsm_addr;
lsm6dso_pin_int1_route_t int1_route;
lsm6dso_pin_int2_route_t int2_route;
stmdev_ctx_t imu_dev_ctx;

bool SCC_check_ok = false;

bool int1_event = false;
bool int2_event = false;
bool RUN_FD_FLAG = false;

#ifdef CONFIG_FALL_DETECT_SUPPORT
lsm6dso_all_sources_t all_source;

/*
// Ì§ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
const uint8_t lsm6so_prg_wrist_tilt[] = {
  0x52, 0x00, 0x14, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x80, 0x00, 
  0x00, 0x0D, 0x06, 0x23, 0x00, 0x53, 0x33, 0x74, 0x44, 0x22,
};
*/

/*fall + tap trigger FSM*/
const uint8_t falltrigger[] = {
      0x91, 0x00, 0x18, 0x00, 0x0E, 0x00, 0xCD, 0x3C,
      0x66, 0x36, 0xA8, 0x00, 0x00, 0x06, 0x23, 0X00,
      0x33, 0x63, 0x33, 0xA5, 0x57, 0x44, 0x22, 0X00,
     };

static void imu_activity_confirm_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(imu_activity_timer, imu_activity_confirm_timerout, NULL);

static void fall_scc_confirm_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(fall_scc_timer, fall_scc_confirm_timerout, NULL);

static void tap_detection_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(tap_detect_timer, tap_detection_timerout, NULL);

static void imu_activity_confirm_timerout(struct k_timer *timer_id)
{
	//LOGD("Activity Num: %d", activity_num);

	if(activity_num<75)
	{
		#if 0 // SCC detection
		StartSCC();
		k_timer_start(&fall_scc_timer, K_SECONDS(9), K_NO_WAIT);
		#else
		FallTrigger();
		#endif
	}

	activity_num = 0;
}

static void tap_detection_timerout(struct k_timer *timer_id)
{
	//LOGD("Tap Timer Up. Total Tap: %d", tap_count);
	if(tap_count <= 3)
	{
		RUN_FD_FLAG = true;
	}
	else
	{
		RUN_FD_FLAG = false;
		//LOGD("Too many taps. Possible false alarm.");
	}
	tap_count = 0;
}
#endif

// new version from ucf file. check wrist_tilt.ucf file.
const uint8_t lsm6so_prg_wrist_tilt[] = {
  0x52, 0x00, 0x14, 0x00, 0x0D, 0x00, 0x8E, 0x31, 0x20, 0x00, 
  0x00, 0x0D, 0x06, 0x23, 0x00, 0x53, 0x33, 0x74, 0x44, 0x22,
};

static struct k_work_q *imu_work_q;

static bool imu_check_ok = false;
static uint8_t whoamI, rst;
static struct device *i2c_imu;
static struct device *gpio_imu = NULL;
static struct gpio_callback gpio_cb1,gpio_cb2;

bool reset_steps = false;
bool imu_redraw_steps_flag = true;
//#ifdef CONFIG_FALL_DETECT_SUPPORT
//static bool fall_check_flag = false;
//#endif

uint16_t g_last_steps = 0;
uint16_t g_steps = 0;
uint16_t g_calorie = 0;
uint16_t g_distance = 0;

#ifdef CONFIG_STEP_SUPPORT
void ClearAllStepRecData(void)
{
	uint8_t tmpbuf[STEP_REC2_DATA_SIZE] = {0xff};

	g_last_steps = 0;
	g_steps = 0;
	g_distance = 0;
	g_calorie = 0;
		
	SpiFlash_Write(tmpbuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
}

void SetCurDayStepRecData(uint16_t data)
{
	uint8_t i,tmpbuf[STEP_REC2_DATA_SIZE] = {0};
	step_rec2_data *p_step,tmp_step = {0};
	sys_date_timer_t temp_date = {0};
	
	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));

	tmp_step.year = temp_date.year;
	tmp_step.month = temp_date.month;
	tmp_step.day = temp_date.day;
	tmp_step.steps[temp_date.hour] = data;

	SpiFlash_Read(tmpbuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
	p_step = tmpbuf;
	if((p_step->year == 0xffff || p_step->year == 0x0000)
		||(p_step->month == 0xff || p_step->month == 0x00)
		||(p_step->day == 0xff || p_step->day == 0x00)
		||((p_step->year == temp_date.year)&&(p_step->month == temp_date.month)&&(p_step->day == temp_date.day))
		)
	{
		//Ö±½Ó¸²¸ÇÐ´ÔÚµÚÒ»Ìõ
		p_step->year = temp_date.year;
		p_step->month = temp_date.month;
		p_step->day = temp_date.day;
		p_step->steps[temp_date.hour] = data;
		SpiFlash_Write(tmpbuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
	}
	else if((temp_date.year < p_step->year)
			||((temp_date.year == p_step->year)&&(temp_date.month < p_step->month))
			||((temp_date.year == p_step->year)&&(temp_date.month == p_step->month)&&(temp_date.day < p_step->day))
			)
	{
		uint8_t databuf[STEP_REC2_DATA_SIZE] = {0};
		
		//²åÈëÐÂµÄµÚÒ»Ìõ,¾ÉµÄµÚÒ»Ìõµ½µÚÁùÌõÍùºóÅ²£¬¶ªµô×îºóÒ»¸ö
		memcpy(&databuf[0*sizeof(step_rec2_data)], &tmp_step, sizeof(step_rec2_data));
		memcpy(&databuf[1*sizeof(step_rec2_data)], &tmpbuf[0*sizeof(step_rec2_data)], 6*sizeof(step_rec2_data));
		SpiFlash_Write(databuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
	}
	else
	{
		uint8_t databuf[STEP_REC2_DATA_SIZE] = {0};
		
		//Ñ°ÕÒºÏÊÊµÄ²åÈëÎ»ÖÃ
		for(i=0;i<7;i++)
		{
			p_step = tmpbuf+i*sizeof(step_rec2_data);
			if((p_step->year == 0xffff || p_step->year == 0x0000)
				||(p_step->month == 0xff || p_step->month == 0x00)
				||(p_step->day == 0xff || p_step->day == 0x00)
				||((p_step->year == temp_date.year)&&(p_step->month == temp_date.month)&&(p_step->day == temp_date.day))
				)
			{
				//Ö±½Ó¸²¸ÇÐ´
				p_step->year = temp_date.year;
				p_step->month = temp_date.month;
				p_step->day = temp_date.day;
				p_step->steps[temp_date.hour] = data;
				SpiFlash_Write(tmpbuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
				return;
			}
			else if((temp_date.year > p_step->year)
				||((temp_date.year == p_step->year)&&(temp_date.month > p_step->month))
				||((temp_date.year == p_step->year)&&(temp_date.month == p_step->month)&&(temp_date.day > p_step->day))
				)
			{
				if(i < 6)
				{
					p_step++;
					if((temp_date.year < p_step->year)
						||((temp_date.year == p_step->year)&&(temp_date.month < p_step->month))
						||((temp_date.year == p_step->year)&&(temp_date.month == p_step->month)&&(temp_date.day < p_step->day))
						)
					{
						break;
					}
				}
			}
		}

		if(i<6)
		{
			//ÕÒµ½Î»ÖÃ£¬²åÈëÐÂÊý¾Ý£¬ÀÏÊý¾ÝÕûÌåÍùºóÅ²£¬¶ªµô×îºóÒ»¸ö
			memcpy(&databuf[0*sizeof(step_rec2_data)], &tmpbuf[0*sizeof(step_rec2_data)], (i+1)*sizeof(step_rec2_data));
			memcpy(&databuf[(i+1)*sizeof(step_rec2_data)], &tmp_step, sizeof(step_rec2_data));
			memcpy(&databuf[(i+2)*sizeof(step_rec2_data)], &tmpbuf[(i+1)*sizeof(step_rec2_data)], (7-(i+2))*sizeof(step_rec2_data));
			SpiFlash_Write(databuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
		}
		else
		{
			//Î´ÕÒµ½Î»ÖÃ£¬Ö±½Ó½ÓÔÚÄ©Î²£¬ÀÏÊý¾ÝÕûÌåÍùÇ°ÒÆ£¬¶ªµô×îÇ°Ò»¸ö
			memcpy(&databuf[0*sizeof(step_rec2_data)], &tmpbuf[1*sizeof(step_rec2_data)], 6*sizeof(step_rec2_data));
			memcpy(&databuf[6*sizeof(step_rec2_data)], &tmp_step, sizeof(step_rec2_data));
			SpiFlash_Write(databuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
		}
	}	
}

void GetCurDayStepRecData(uint16_t *databuf)
{
	uint8_t i,tmpbuf[STEP_REC2_DATA_SIZE] = {0};
	step_rec2_data step_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&step_rec2, &tmpbuf[i*sizeof(step_rec2_data)], sizeof(step_rec2_data));
		if((step_rec2.year == 0xffff || step_rec2.year == 0x0000)||(step_rec2.month == 0xff || step_rec2.month == 0x00)||(step_rec2.day == 0xff || step_rec2.day == 0x00))
			continue;
		
		if((step_rec2.year == date_time.year)&&(step_rec2.month == date_time.month)&&(step_rec2.day == date_time.day))
		{
			memcpy(databuf, step_rec2.steps, sizeof(step_rec2.steps));
			break;
		}
	}
}

void StepsDataInit(bool reset_flag)
{
	bool flag = false;
	
	if((last_sport.step_rec.timestamp.year == date_time.year)
		&&(last_sport.step_rec.timestamp.month == date_time.month)
		&&(last_sport.step_rec.timestamp.day == date_time.day)
		)
	{
		flag = true;
	}

	if(reset_flag)
	{
		if(!flag)
		{
			g_steps -= g_last_steps;
			g_distance = 0.7*g_steps;
			g_calorie = (0.8214*60*g_distance)/1000;
			g_last_steps = 0;

			imu_redraw_steps_flag = true;
		}
	}
	else
	{
		if(flag)
		{
			g_last_steps = last_sport.step_rec.steps;
			g_steps = last_sport.step_rec.steps;
			g_distance = last_sport.step_rec.distance;
			g_calorie = last_sport.step_rec.calorie;
		}
	}
}
#endif

static uint8_t init_i2c(void)
{
	i2c_imu = DEVICE_DT_GET(IMU_DEV);
	if(!i2c_imu)
	{
	#ifdef IMU_DEBUG
		LOGD("ERROR SETTING UP I2C");
	#endif
		return -1;
	}
	else
	{
		i2c_configure(i2c_imu, I2C_SPEED_SET(I2C_SPEED_FAST));
		return 0;
	}
}

static int32_t platform_write(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;
	uint8_t data[len+1];

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(i2c_imu, data, len+1, LSM6DSO_I2C_ADD);

	return rslt;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;

	rslt = i2c_write(i2c_imu, &reg, 1, LSM6DSO_I2C_ADD);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_imu, bufp, len, LSM6DSO_I2C_ADD);
	}

	return rslt;
}

void interrupt_event(struct device *interrupt, struct gpio_callback *cb, uint32_t pins)
{
	int2_event = true;
}

void step_event(struct device *interrupt, struct gpio_callback *cb, uint32_t pins)
{
	int1_event = true;
}

void init_imu_int1(void)
{
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_DOWN;

	if(gpio_imu == NULL)
		gpio_imu = DEVICE_DT_GET(IMU_PORT);
	gpio_pin_configure(gpio_imu, LSM6DSO_INT1_PIN, flag);
}

uint8_t init_gpio(void)
{
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_DOWN;

	if(gpio_imu == NULL)
		gpio_imu = DEVICE_DT_GET(IMU_PORT);

	//steps&tilt interrupt
	gpio_pin_configure(gpio_imu, LSM6DSO_INT1_PIN, flag);
    gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT1_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb1, step_event, BIT(LSM6DSO_INT1_PIN));
	gpio_add_callback(gpio_imu, &gpio_cb1);
    gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT1_PIN, GPIO_INT_ENABLE|GPIO_INT_EDGE_RISING);

	//fall interrupt
	gpio_pin_configure(gpio_imu, LSM6DSO_INT2_PIN, flag);
    gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT2_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb2, interrupt_event, BIT(LSM6DSO_INT2_PIN));
	gpio_add_callback(gpio_imu, &gpio_cb2);
    gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT2_PIN, GPIO_INT_ENABLE|GPIO_INT_EDGE_RISING);

	return 0;
}

#ifdef CONFIG_FALL_DETECT_SUPPORT
void imu_sensor_init(void)
{
	lsm6dso_reset_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_reset_get(&imu_dev_ctx, &rst);

	lsm6dso_i3c_disable_set(&imu_dev_ctx, LSM6DSO_I3C_DISABLE);

	lsm6dso_xl_full_scale_set(&imu_dev_ctx, LSM6DSO_4g); 
	lsm6dso_gy_full_scale_set(&imu_dev_ctx, LSM6DSO_250dps);
	lsm6dso_block_data_update_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_watermark_set(&imu_dev_ctx, 400);
	lsm6dso_fifo_stop_on_wtm_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_mode_set(&imu_dev_ctx, LSM6DSO_STREAM_TO_FIFO_MODE);

	lsm6dso_fifo_xl_batch_set(&imu_dev_ctx, LSM6DSO_XL_BATCHED_AT_104Hz);
	lsm6dso_fifo_gy_batch_set(&imu_dev_ctx, LSM6DSO_GY_BATCHED_AT_104Hz);

	lsm6dso_xl_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_104Hz); 
	lsm6dso_gy_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_104Hz);
	
	lsm6dso_xl_power_mode_set(&imu_dev_ctx, LSM6DSO_LOW_NORMAL_POWER_MD);
	lsm6dso_gy_power_mode_set(&imu_dev_ctx, LSM6DSO_GY_NORMAL);

	//Activity detection
    //Set duration for Activity detection to 9.62 ms (= 1 * 1 / ODR_XL)
    lsm6dso_wkup_dur_set(&imu_dev_ctx, 0x01); // ï¿½î¶¯ï¿½ï¿½ï¿½Ñ¼ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
    //Set duration for Inactivity detection to 4.92 s (= 1 * 512 / ODR_XL) ï¿½ï¿½ï¿½Ã»î¶¯Ä£Ê½ï¿½Â½ï¿½ï¿½ï¿½Ë¯ï¿½ï¿½Ä£Ê½Ö®Ç°ï¿½Ä³ï¿½ï¿½ï¿½Ê±ï¿½ï¿½
    lsm6dso_act_sleep_dur_set(&imu_dev_ctx, 0x01); // ï¿½ï¿½ï¿½è¶¨ï¿½Ä»î¶¯Ë¯ï¿½ß³ï¿½ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½Ã»ï¿½Ð½ï¿½Ò»ï¿½ï¿½ï¿½Ä»î¶¯ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ô¶ï¿½ï¿½ï¿½ï¿½ï¿½Í¹ï¿½ï¿½ï¿½Ä£Ê?
    //Set Activity/Inactivity threshold to 31.25 mg (= 1* FS_XL / 2^6)
    lsm6dso_wkup_threshold_set(&imu_dev_ctx, 0x01); // ï¿½ï¿½ï¿½Ã»ï¿½ï¿½ï¿½ï¿½ï¿½Öµ,ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½âµ½ï¿½Ä¼ï¿½ï¿½Ù¶È»ï¿½ï¿½ï¿½Ù¶ÈµÈ²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ÖµÊ±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ÓµÍ¹ï¿½ï¿½ï¿½Ä£Ê½ï¿½ï¿½ï¿½Ñµï¿½ï¿½î¶?Ä£Ê½,
	//  ï¿½ï¿½ï¿½Ã½Ï¸ßµï¿½ï¿½ï¿½Öµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ÑµÄ·ï¿½ï¿½Õ£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ü»á½µï¿½Í¶Ô½ï¿½Ð¡ï¿½ï¿½ï¿½ï¿½ï¿½Ä¼ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
    //Inactivity configuration: XL to 12.5 in LP, gyro to Power-Down
    //lsm6dso_act_mode_set(&imu_dev_ctx, LSM6DSO_XL_12Hz5_GY_PD);

	//Tap detection 
	lsm6dso_tap_detection_on_z_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_tap_detection_on_y_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_tap_detection_on_x_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_tap_threshold_z_set(&imu_dev_ctx, 0x16); // 31*(2/2^5) = 1.9375 = 1937.5 mg
	lsm6dso_tap_threshold_y_set(&imu_dev_ctx, 0x16); // 16
	lsm6dso_tap_threshold_x_set(&imu_dev_ctx, 0x16);

	lsm6dso_tap_quiet_set(&imu_dev_ctx, 0x03); // 3*(4/104) = 0.115384 seconds = 115.384 ms where 104 is the current ODR
	lsm6dso_tap_shock_set(&imu_dev_ctx, 0x03); // 3*(8/104) = 0.230769 seconds = 230.769 ms where 104 is the current ODR
	lsm6dso_tap_mode_set(&imu_dev_ctx, LSM6DSO_ONLY_SINGLE);

	lsm6dso_int_notification_set(&imu_dev_ctx, LSM6DSO_BASE_PULSED_EMB_LATCHED);
	
	//Tilt FSM
	lsm6dso_long_cnt_int_value_set(&imu_dev_ctx, 0x0000U);
	lsm6dso_fsm_start_address_set(&imu_dev_ctx, LSM6DSO_START_FSM_ADD);
	lsm6dso_fsm_number_of_programs_set(&imu_dev_ctx, 1);
	lsm6dso_fsm_enable_get(&imu_dev_ctx, &fsm_enable);
	fsm_enable.fsm_enable_a.fsm1_en = PROPERTY_ENABLE;
	//fsm_enable.fsm_enable_a.fsm2_en = PROPERTY_DISABLE;
	lsm6dso_fsm_enable_set(&imu_dev_ctx, &fsm_enable);  
	lsm6dso_fsm_data_rate_set(&imu_dev_ctx, LSM6DSO_ODR_FSM_26Hz);
	fsm_addr = LSM6DSO_START_FSM_ADD;

	lsm6dso_ln_pg_write(&imu_dev_ctx, fsm_addr, (uint8_t*)lsm6so_prg_wrist_tilt, 
						sizeof(lsm6so_prg_wrist_tilt));
	fsm_addr += sizeof(lsm6so_prg_wrist_tilt);
	
	// route wrist tilt to INT1 pin
	lsm6dso_pin_int1_route_get(&imu_dev_ctx, &int1_route);
	int1_route.fsm_int1_a.int1_fsm1 = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);
	
	// route tap and activity to INT2 pin
	lsm6dso_pin_int2_route_get(&imu_dev_ctx, &int2_route);
	int2_route.md2_cfg.int2_single_tap = PROPERTY_ENABLE;
	int2_route.md2_cfg.int2_sleep_change = PROPERTY_ENABLE;
	lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_route);

	lsm6dso_timestamp_set(&imu_dev_ctx, 1);
}
#else
void imu_sensor_init(void)
{
	lsm6dso_reset_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_reset_get(&imu_dev_ctx, &rst);

	lsm6dso_i3c_disable_set(&imu_dev_ctx, LSM6DSO_I3C_DISABLE);

	lsm6dso_xl_full_scale_set(&imu_dev_ctx, LSM6DSO_2g);
	//lsm6dso_gy_full_scale_set(&imu_dev_ctx, LSM6DSO_250dps);
	lsm6dso_block_data_update_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_watermark_set(&imu_dev_ctx, 104);
	lsm6dso_fifo_stop_on_wtm_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_mode_set(&imu_dev_ctx, LSM6DSO_STREAM_TO_FIFO_MODE);

	lsm6dso_fifo_xl_batch_set(&imu_dev_ctx, LSM6DSO_XL_BATCHED_AT_26Hz);
	lsm6dso_fifo_gy_batch_set(&imu_dev_ctx, LSM6DSO_GY_NOT_BATCHED);

	lsm6dso_xl_power_mode_set(&imu_dev_ctx, LSM6DSO_ULTRA_LOW_POWER_MD);

  	lsm6dso_xl_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_26Hz);
  	lsm6dso_gy_data_rate_set(&imu_dev_ctx, LSM6DSO_GY_ODR_OFF);

	lsm6dso_int_notification_set(&imu_dev_ctx, LSM6DSO_BASE_PULSED_EMB_LATCHED);
	
	/*Tilt enable */
	lsm6dso_long_cnt_int_value_set(&imu_dev_ctx, 0x0000U);
	lsm6dso_fsm_start_address_set(&imu_dev_ctx, LSM6DSO_START_FSM_ADD);
	lsm6dso_fsm_number_of_programs_set(&imu_dev_ctx, 1);
	lsm6dso_fsm_enable_get(&imu_dev_ctx, &fsm_enable);
	fsm_enable.fsm_enable_a.fsm1_en = PROPERTY_ENABLE;
	lsm6dso_fsm_enable_set(&imu_dev_ctx, &fsm_enable);  
	lsm6dso_fsm_data_rate_set(&imu_dev_ctx, LSM6DSO_ODR_FSM_26Hz);
	fsm_addr = LSM6DSO_START_FSM_ADD;

	lsm6dso_ln_pg_write(&imu_dev_ctx, fsm_addr, 
						(uint8_t*)lsm6so_prg_wrist_tilt, 
						sizeof(lsm6so_prg_wrist_tilt));
	fsm_addr += sizeof(lsm6so_prg_wrist_tilt);
	
	/* route wrist tilt to INT1 pin*/
	lsm6dso_pin_int1_route_get(&imu_dev_ctx, &int1_route);
	int1_route.fsm_int1_a.int1_fsm1 = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);

	// Activity enable
	// Set duration for Activity detection to 38 ms (= 1 * 1 / ODR_XL) where ODR_XL = 26Hz
	lsm6dso_wkup_dur_set(&imu_dev_ctx, 0x01);
	// Set duration for Inactivity detection to 19.69 s (= 1 * 512 / ODR_XL) where ODR_XL = 26Hz
	lsm6dso_act_sleep_dur_set(&imu_dev_ctx, 0x01);
	// Set Activity/Inactivity threshold to 31.25 mg (= 1 * FS_XL / 2^6) where FS_XL = 2g (most sensitive threshold)
	lsm6dso_wkup_threshold_set(&imu_dev_ctx, 0x01);
	// Inactivity configuration: XL to 12.5 in LP, gyro to Power-Down
	lsm6dso_act_mode_set(&imu_dev_ctx, LSM6DSO_XL_12Hz5_GY_PD);
	
	// route inactivity to INT2 pin
	lsm6dso_pin_int2_route_get(&imu_dev_ctx, &int2_route);
	int1_route.md1_cfg.int1_sleep_change = PROPERTY_ENABLE;
	lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_route);

	lsm6dso_timestamp_set(&imu_dev_ctx, 1);
}
#endif

static bool sensor_init(void)
{
	lsm6dso_device_id_get(&imu_dev_ctx, &whoamI);
	if(whoamI != LSM6DSO_ID)
		return false;

	imu_sensor_init();
	return true;
}

/*@brief Get real time X/Y/Z reading in mg
*
*/
/*void get_sensor_reading(float *sensor_x, float *sensor_y, float *sensor_z)
{
	uint8_t reg;

	lsm6dso_xl_flag_data_ready_get(&imu_dev_ctx, &reg);
	if(reg)
	{
		memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
		lsm6dso_acceleration_raw_get(&imu_dev_ctx, data_raw_acceleration.u8bit);
		acceleration_mg[0] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[0]);
		acceleration_mg[1] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[1]);
		acceleration_mg[2] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[2]);
	}

	*sensor_x = acceleration_mg[0];
	*sensor_y = acceleration_mg[1];
	*sensor_z = acceleration_mg[2];
}*/

#ifdef CONFIG_STEP_SUPPORT
void ReSetImuSteps(void)
{
	lsm6dso_steps_reset(&imu_dev_ctx);

	g_last_steps = 0;
	g_steps = 0;
	g_distance = 0;
	g_calorie = 0;
	
	last_sport.step_rec.timestamp.year = date_time.year;
	last_sport.step_rec.timestamp.month = date_time.month; 
	last_sport.step_rec.timestamp.day = date_time.day;
	last_sport.step_rec.timestamp.hour = date_time.hour;
	last_sport.step_rec.timestamp.minute = date_time.minute;
	last_sport.step_rec.timestamp.second = date_time.second;
	last_sport.step_rec.timestamp.week = date_time.week;
	last_sport.step_rec.steps = g_steps;
	last_sport.step_rec.distance = g_distance;
	last_sport.step_rec.calorie = g_calorie;
	save_cur_sport_to_record(&last_sport);	
}

void GetImuSteps(uint16_t *steps)
{
	lsm6dso_number_of_steps_get(&imu_dev_ctx, steps);
}

void UpdateIMUData(void)
{
	uint16_t steps;
	
	GetImuSteps(&steps);

	g_steps = steps+g_last_steps;
	g_distance = 0.7*g_steps;
	g_calorie = (0.8214*60*g_distance)/1000;

#ifdef IMU_DEBUG
	LOGD("g_steps:%d,g_distance:%d,g_calorie:%d", g_steps, g_distance, g_calorie);
#endif

	last_sport.step_rec.timestamp.year = date_time.year;
	last_sport.step_rec.timestamp.month = date_time.month; 
	last_sport.step_rec.timestamp.day = date_time.day;
	last_sport.step_rec.timestamp.hour = date_time.hour;
	last_sport.step_rec.timestamp.minute = date_time.minute;
	last_sport.step_rec.timestamp.second = date_time.second;
	last_sport.step_rec.timestamp.week = date_time.week;
	last_sport.step_rec.steps = g_steps;
	last_sport.step_rec.distance = g_distance;
	last_sport.step_rec.calorie = g_calorie;
	save_cur_sport_to_record(&last_sport);
	
	//StepCheckSendLocationData(g_steps);
}

void GetSportData(uint16_t *steps, uint16_t *calorie, uint16_t *distance)
{
	if(steps != NULL)
		*steps = g_steps;
	if(calorie != NULL)
		*calorie = g_calorie;
	if(distance != NULL)
		*distance = g_distance;
}
#endif

uint8_t IMU_GetID(void)
{
	uint8_t sensor_id = 0;
	
	lsm6dso_device_id_get(&imu_dev_ctx, &sensor_id);
	return sensor_id;
}

void IMU_init(struct k_work_q *work_q)
{
#ifdef IMU_DEBUG
	LOGD("IMU_init");
#endif

#ifdef CONFIG_STEP_SUPPORT
	get_cur_sport_from_record(&last_sport);
#ifdef IMU_DEBUG
	LOGD("%04d/%02d/%02d last_steps:%d", last_sport.timestamp.year,last_sport.timestamp.month,last_sport.timestamp.day,last_sport.steps);
#endif
	StepsDataInit(false);
#endif

	imu_work_q = work_q;

	if(init_i2c() != 0)
		return;
	
	init_gpio();

	imu_dev_ctx.write_reg = platform_write;
	imu_dev_ctx.read_reg = platform_read;
	imu_dev_ctx.handle = i2c_imu;

	imu_check_ok = sensor_init();
	if(!imu_check_ok)
		return;

#ifdef IMU_DEBUG
	LOGD("IMU_init done!");
#endif
}

/*@brief Check if a wrist tilt happend
*
* @return If tilt detected, return true, otherwise false
*/
bool is_tilt(void)
{
	bool ret = false;
	lsm6dso_all_sources_t status;

	lsm6dso_all_sources_get(&imu_dev_ctx, &status);
	if(status.fsm_status_a.is_fsm1)
	{ 
		//tilt detected
		ret = true;
	}

	return ret;
}

bool is_tap(void)
{
	bool ret = false;
	lsm6dso_all_sources_t status1;

	lsm6dso_all_sources_get(&imu_dev_ctx, &status1);
	if(status1.tap_src.single_tap)
	{ 
		//tap detected
		ret = true;
	}

	return ret;
}

void tap_detection(void)
{
	tap_count++;
	//LOGD("Tap Num: %d", tap_count);
}

static void fall_scc_confirm_timerout(struct k_timer *timer_id)
{
	if(CheckSCC())
	{
		SCC_check_ok = true;
	}
}

void IMUMsgProcess(void)
{
	if(0
		#ifdef CONFIG_FOTA_DOWNLOAD
			|| (fota_is_running())
		#endif
		#ifdef CONFIG_DATA_DOWNLOAD_SUPPORT
			|| (dl_is_running())
		#endif
		#ifdef CONFIG_FACTORY_TEST_SUPPORT
			|| (FactoryTestActived())
		#endif
		)
	{
		return;
	}

	if(int1_event)	//tilt
	{
	#ifdef IMU_DEBUG
		LOGD("int1 evt!");
	#endif
		int1_event = false;

		if(!imu_check_ok || !is_wearing())
			return;

		if(is_tilt())
		{
		#ifdef IMU_DEBUG
			LOGD("tilt trigger!");
		#endif
			if(lcd_is_sleeping && global_settings.wake_screen_by_wrist)
			{
				sleep_out_by_wrist = true;
				lcd_sleep_out = true;
			}
		}
	}

	#ifdef CONFIG_FALL_DETECT_SUPPORT
	if(int2_event && global_settings.fall_check) //fall
	{
	#ifdef IMU_DEBUG
		LOGD("int2 evt!");
	#endif

		if(!imu_check_ok || !is_wearing())
			return;
		
		if(is_tap())
		{
		#if 0 //Tap detection
			k_timer_start(&tap_detect_timer, K_SECONDS(3), K_NO_WAIT);

			if(k_timer_remaining_get(&tap_detect_timer)>=0)
			{
				tap_detection();
			}
		#else
			fall_detection();
		#endif
		}

		int2_event = false;
	}

	if(RUN_FD_FLAG)
	{
		RUN_FD_FLAG = false;
		fall_detection();
	}

	if(fall_result)
	{
		fall_result = false;
		//LOGD("Checking for activity");

		#if 0 // Activity detection
		k_timer_start(&imu_activity_timer, K_SECONDS(3), K_NO_WAIT);

		while(1)
		{
			lsm6dso_all_sources_t status3;

			lsm6dso_all_sources_get(&imu_dev_ctx, &status3);
			if(status3.wake_up_src.wu_ia)
			{
				activity_num++;
				//LOGD("Acitvity detected: %d", activity_num);
			}

			if(k_timer_remaining_get(&imu_activity_timer) == 0)
			{
				break;
			}
		}
		#else
		#if 1 // SCC detections
		StartSCC();
		k_timer_start(&fall_scc_timer, K_SECONDS(9), K_NO_WAIT);
		#else
		FallTrigger();
		#endif
		#endif
	}
	
	if(SCC_check_ok)
	{
		SCC_check_ok = false;

		FallTrigger();
	}

	//if(fall_check_flag)
	//{
		//k_sleep(K_MSEC(3));
	//}
#endif
}
#endif/*CONFIG_IMU_SUPPORT*/
