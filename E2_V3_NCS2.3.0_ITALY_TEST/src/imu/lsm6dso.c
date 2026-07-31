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
#ifdef CONFIG_SLEEP_SUPPORT
#include "sleep.h"
#endif
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif
#include "logger.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h" // SCC
#endif

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
//lsm6dso_all_sources_t all_source;
bool fall_check_flag = false;

/*fall + tap trigger FSM*/
/*
const uint8_t falltrigger[] = {
      0x91, 0x00, 0x18, 0x00, 0x0E, 0x00, 0xCD, 0x3C,
      0x66, 0x36, 0xA8, 0x00, 0x00, 0x06, 0x23, 0X00,
      0x33, 0x63, 0x33, 0xA5, 0x57, 0x44, 0x22, 0X00,
     };
*/

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

static bool imu_check_ok = false;
static uint8_t whoamI, rst;
static struct k_work_q *imu_work_q;
static struct device *i2c_imu;
static struct device *gpio_imu = NULL;
static struct gpio_callback gpio_cb1,gpio_cb2;

static axis3bit16_t data_raw_acceleration;
static float acceleration_mg[3];

#ifdef CONFIG_STEP_SUPPORT
/*
 * Step-count data flow:
 *
 *   LSM6DSO 16-bit counter -> cadence/burst filter -> daily totals -> flash/UI
 *
 * The sensor remains the only source of steps. The software filter does not
 * detect steps from acceleration samples; it only decides whether changes in
 * the hardware counter belong to a plausible walking session.
 */
bool reset_steps = false;
bool imu_redraw_steps_flag = true;
/* Total restored before the current hardware-counter session started. */
uint16_t g_last_steps = 0;
/* Public daily totals derived from the restored and newly accepted steps. */
uint16_t g_steps = 0;
uint16_t g_calorie = 0;
uint16_t g_distance = 0;
/* Runtime state for validating changes in the 16-bit hardware counter. */
static bool hardware_step_enabled = false;
static bool hardware_walk_confirmed = false;
static bool hardware_step_filter_initialized = false;
static uint16_t hardware_last_raw_steps = 0;
static uint16_t hardware_accepted_steps = 0;
static uint16_t hardware_pending_steps = 0;
static uint32_t hardware_last_step_time = 0;

/* Candidate hardware steps required before a walking session is confirmed. */
#define HARDWARE_STEP_CONFIRM_STEPS      8
/* Allowed average time per step. Values outside this range reset candidates. */
#define HARDWARE_STEP_MIN_INTERVAL_MS    400
#define HARDWARE_STEP_MAX_INTERVAL_MS    2200
/* A longer gap ends the candidate/confirmed walking session. */
#define HARDWARE_STEP_RESET_INTERVAL_MS  3000
/* Larger fast counter jumps are treated as vibration or movement bursts. */
#define HARDWARE_STEP_MAX_BURST_DELTA    3
static void ResetHardwareStepFilter(void);
static bool ReadHardwareSteps(uint16_t *steps);

/**
 * Clear the live daily totals, reset the sensor/filter, and invalidate all
 * persisted hourly step history.
 */
void ClearAllStepRecData(void)
{
	uint8_t tmpbuf[STEP_REC2_DATA_SIZE] = {0xff};

	g_last_steps = 0;
	g_steps = 0;
	g_distance = 0;
	g_calorie = 0;
	lsm6dso_steps_reset(&imu_dev_ctx);
    ResetHardwareStepFilter();
		
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
	//xb add 2026.06.25 only 24 pieces of data from 1 hour to 23:59 hours are stored in the position of array[0]~array[23].
	if((temp_date.hour == 23) && (temp_date.minute == 59))
		temp_date.hour = 23;
	else
		temp_date.hour--;
	tmp_step.steps[temp_date.hour] = data;

	SpiFlash_Read(tmpbuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
	p_step = tmpbuf;
	if((p_step->year == 0xffff || p_step->year == 0x0000)
		||(p_step->month == 0xff || p_step->month == 0x00)
		||(p_step->day == 0xff || p_step->day == 0x00)
		||((p_step->year == temp_date.year)&&(p_step->month == temp_date.month)&&(p_step->day == temp_date.day))
		)
	{
		//直接覆盖写在第一条
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
		
		//插入新的第一条,旧的第一条到第六条往后挪，丢掉最后一个
		memcpy(&databuf[0*sizeof(step_rec2_data)], &tmp_step, sizeof(step_rec2_data));
		memcpy(&databuf[1*sizeof(step_rec2_data)], &tmpbuf[0*sizeof(step_rec2_data)], 6*sizeof(step_rec2_data));
		SpiFlash_Write(databuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
	}
	else
	{
		uint8_t databuf[STEP_REC2_DATA_SIZE] = {0};
		
		//寻找合适的插入位置
		for(i=0;i<7;i++)
		{
			p_step = tmpbuf+i*sizeof(step_rec2_data);
			if((p_step->year == 0xffff || p_step->year == 0x0000)
				||(p_step->month == 0xff || p_step->month == 0x00)
				||(p_step->day == 0xff || p_step->day == 0x00)
				||((p_step->year == temp_date.year)&&(p_step->month == temp_date.month)&&(p_step->day == temp_date.day))
				)
			{
				//直接覆盖写
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
			//找到位置，插入新数据，老数据整体往后挪，丢掉最后一个
			memcpy(&databuf[0*sizeof(step_rec2_data)], &tmpbuf[0*sizeof(step_rec2_data)], (i+1)*sizeof(step_rec2_data));
			memcpy(&databuf[(i+1)*sizeof(step_rec2_data)], &tmp_step, sizeof(step_rec2_data));
			memcpy(&databuf[(i+2)*sizeof(step_rec2_data)], &tmpbuf[(i+1)*sizeof(step_rec2_data)], (7-(i+2))*sizeof(step_rec2_data));
			SpiFlash_Write(databuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
		}
		else
		{
			//未找到位置，直接接在末尾，老数据整体往前移，丢掉最前一个
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
			if (g_steps >= g_last_steps)
			{
				g_steps -= g_last_steps;
			}
			g_distance = (global_settings.person.step_length*g_steps)/100;
			g_calorie = (0.8*global_settings.person.weight*g_distance)/1000;
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

void imu_sensor_init(void)
{
	//Activity detection
    //Set duration for Activity detection to 9.62 ms (= 1 * 1 / ODR_XL)
    lsm6dso_wkup_dur_set(&imu_dev_ctx, 0x01);
    //Set duration for Inactivity detection to 4.92 s (= 1 * 512 / ODR_XL)
    lsm6dso_act_sleep_dur_set(&imu_dev_ctx, 0x01);
    //Set Activity/Inactivity threshold to 31.25 mg (= 1* FS_XL / 2^6)
    lsm6dso_wkup_threshold_set(&imu_dev_ctx, 0x01);
    //Inactivity configuration: XL to 12.5 in LP, gyro to Power-Down
    //lsm6dso_act_mode_set(&imu_dev_ctx, LSM6DSO_XL_12Hz5_GY_PD);

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


	sensor_reset_init();

#if 0 //def CONFIG_STEP_SUPPORT
	// 启用LPF2滤波器，使用滤波会出现无法计步的问题
    lsm6dso_xl_filter_lp2_set(&imu_dev_ctx, PROPERTY_ENABLE);
	// 启用快速稳定模式（上电时滤波器快速稳定）
    lsm6dso_xl_fast_settling_set(&imu_dev_ctx, PROPERTY_ENABLE);
	// 高通滤波
	//lsm6dso_xl_hp_path_internal_set(&imu_dev_ctx, LSM6DSO_USE_HPF);
	//lsm6dso_xl_hp_path_on_out_set(&imu_dev_ctx, LSM6DSO_HP_ODR_DIV_200);
#endif
}

void sensor_reset_init(void)
{
	uint8_t attempt;
#ifdef CONFIG_STEP_SUPPORT
	bool restart_step_counting = hardware_step_enabled;

	/*
	 * A software reset disables the hardware pedometer. Keep the software
	 * state synchronized so IMUMsgProcess() can retry if restoration fails.
	 */
	hardware_step_enabled = false;
	hardware_walk_confirmed = false;
	hardware_pending_steps = 0;
#endif
	//lsm6dso_reset_set(&imu_dev_ctx, PROPERTY_ENABLE);
	//lsm6dso_reset_get(&imu_dev_ctx, &rst);
	if(lsm6dso_reset_set(&imu_dev_ctx, PROPERTY_ENABLE) != 0)
		return;

	rst = PROPERTY_ENABLE;

	for(attempt = 0; attempt < 100; attempt++)
	{
		if(lsm6dso_reset_get(&imu_dev_ctx, &rst) != 0)
			return;

		if(rst == PROPERTY_DISABLE)
			break;

		k_sleep(K_MSEC(1));
	}

	if(rst != PROPERTY_DISABLE)
	{
		/* Reset did not complete within 100 ms. */
		return;
	}

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
	lsm6dso_gy_data_rate_set(&imu_dev_ctx, LSM6DSO_GY_ODR_104Hz);
	
	lsm6dso_xl_power_mode_set(&imu_dev_ctx, LSM6DSO_LOW_NORMAL_POWER_MD);
	lsm6dso_gy_power_mode_set(&imu_dev_ctx, LSM6DSO_GY_NORMAL);

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
	
	// route wrist tilt to INT1 pin
	lsm6dso_pin_int1_route_get(&imu_dev_ctx, &int1_route);
	int1_route.fsm_int1_a.int1_fsm1 = PROPERTY_ENABLE;
	int1_route.emb_func_int1.int1_step_detector = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);

#ifdef CONFIG_STEP_SUPPORT
	/*
	 * StepCountingStart() restores debounce, advanced false-step rejection,
	 * interrupt mode and the software step filter. It also preserves g_steps.
	 */
	if(restart_step_counting && global_settings.step_is_on)
	{
		StepCountingStart();
	}
#endif
	
	// route tap and activity to INT2 pin
	lsm6dso_pin_int2_route_get(&imu_dev_ctx, &int2_route);
	int2_route.md2_cfg.int2_single_tap = PROPERTY_ENABLE;
	int2_route.md2_cfg.int2_sleep_change = PROPERTY_ENABLE;
	lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_route);

	lsm6dso_timestamp_set(&imu_dev_ctx, 1);
}

/**
 * @brief  LSM6DSO进入休眠模式，最小化电流消耗
 */
void imu_sensor_off(void)
{
    //  禁用所有嵌入式功能（必须在关闭传感器前）
    // 禁用计步器
    lsm6dso_pedo_md_t pedo_mode = LSM6DSO_PEDO_DISABLE;
    lsm6dso_pedo_sens_set(&imu_dev_ctx, pedo_mode);
#ifdef CONFIG_STEP_SUPPORT
	hardware_step_enabled = false;
	hardware_walk_confirmed = false;
	hardware_pending_steps = 0;
#endif
    
    // 禁用倾斜检测
    uint8_t tilt_enable = 0;
    lsm6dso_tilt_sens_set(&imu_dev_ctx, tilt_enable);
    
    // 禁用FSM（有限状态机）
    lsm6dso_emb_fsm_enable_t fsm_enable = {0};
	fsm_enable.fsm_enable_a.fsm1_en = PROPERTY_DISABLE;
    lsm6dso_fsm_enable_set(&imu_dev_ctx, &fsm_enable);
    
    //  禁用所有中断
    
    // 禁用INT1所有中断
    lsm6dso_pin_int1_route_t int1_route = {0};
    lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);
    
    // 禁用INT2所有中断
    lsm6dso_pin_int2_route_t int2_route = {0};
    lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_route);
    
    // 关闭FIFO
	lsm6dso_fifo_xl_batch_set(&imu_dev_ctx, LSM6DSO_XL_NOT_BATCHED);
	lsm6dso_fifo_gy_batch_set(&imu_dev_ctx, LSM6DSO_GY_NOT_BATCHED);

	lsm6dso_fifo_mode_t fifo_mode = LSM6DSO_BYPASS_MODE;
    lsm6dso_fifo_mode_set(&imu_dev_ctx, fifo_mode);
    
    // 关键：关闭传感器（最大程度省电）**
    
    // 关闭加速度计（设置ODR为OFF）
    lsm6dso_odr_xl_t xl_odr = LSM6DSO_XL_ODR_OFF;
    lsm6dso_xl_data_rate_set(&imu_dev_ctx, xl_odr);
    
    // 关闭陀螺仪（设置ODR为OFF）
    lsm6dso_odr_g_t gy_odr = LSM6DSO_GY_ODR_OFF;
    lsm6dso_gy_data_rate_set(&imu_dev_ctx, gy_odr);
    
    // 启用陀螺仪睡眠模式
    uint8_t gy_sleep_mode = PROPERTY_ENABLE;
    lsm6dso_gy_sleep_mode_set(&imu_dev_ctx, gy_sleep_mode);
    
    // 设置超低功耗模式
    lsm6dso_xl_hm_mode_t xl_power_mode = LSM6DSO_ULTRA_LOW_POWER_MD;
    lsm6dso_xl_power_mode_set(&imu_dev_ctx, xl_power_mode);
    
    lsm6dso_g_hm_mode_t gy_power_mode = LSM6DSO_GY_HIGH_PERFORMANCE;
    lsm6dso_gy_power_mode_set(&imu_dev_ctx, gy_power_mode);
}

static bool sensor_init(void)
{
	lsm6dso_device_id_get(&imu_dev_ctx, &whoamI);
	if(whoamI != LSM6DSO_ID)
		return false;

	imu_sensor_init();

#ifdef CONFIG_STEP_SUPPORT
	/* Start from a known raw counter before the user setting enables it. */
	lsm6dso_steps_reset(&imu_dev_ctx);
#endif

	return true;
}

/*@brief Get real time X/Y/Z reading in mg
*
*/
void get_sensor_reading(float *sensor_x, float *sensor_y, float *sensor_z)
{
	uint8_t reg;

	lsm6dso_xl_flag_data_ready_get(&imu_dev_ctx, &reg);
	if(reg)
	{
		memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
		lsm6dso_acceleration_raw_get(&imu_dev_ctx, data_raw_acceleration.u8bit);
		acceleration_mg[0] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[0]);
		acceleration_mg[1] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[1]);
		acceleration_mg[2] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[2]);
	}

	*sensor_x = acceleration_mg[0];
	*sensor_y = acceleration_mg[1];
	*sensor_z = acceleration_mg[2];
}

#ifdef CONFIG_STEP_SUPPORT
/**
 * Start a new hardware-counter session without losing the restored daily total.
 *
 * The LSM6DSO advanced mode enables false-positive rejection and low-energy
 * gait adaptation. Resetting the sensor counter prevents old raw counts from
 * being added again; g_last_steps preserves the total already shown to users.
 */
void StepCountingStart(void)
{
	lsm6dso_sensitivity();
	lsm6dso_pedo_int_mode_set(&imu_dev_ctx, LSM6DSO_EVERY_STEP);
	lsm6dso_pedo_sens_set(&imu_dev_ctx, LSM6DSO_FALSE_STEP_REJ_ADV_MODE);
	lsm6dso_steps_reset(&imu_dev_ctx);
	ResetHardwareStepFilter();
	g_last_steps = g_steps;
	hardware_step_enabled = true;
}

/**
 * Disable hardware step detection and discard any unconfirmed candidate steps.
 * Accepted daily totals remain available through GetSportData().
 */
void StepCountingStop(void)
{
	lsm6dso_pedo_sens_set(&imu_dev_ctx, LSM6DSO_PEDO_DISABLE);
	hardware_step_enabled = false;
	hardware_walk_confirmed = false;
	hardware_pending_steps = 0;
}

/**
 * Persist the current aggregate totals and their timestamp as the latest sport
 * record. Hourly history is maintained separately by SetCurDayStepRecData().
 */
static void SaveStepSportData(void)
{
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

/**
 * Return the cadence filter to an empty, uninitialized state.
 *
 * The next successful raw-counter read becomes a baseline and is deliberately
 * not counted. hardware_accepted_steps is relative to the current sensor
 * counter session, so callers preserve any earlier total in g_last_steps.
 */
static void ResetHardwareStepFilter(void)
{
	hardware_walk_confirmed = false;
	hardware_step_filter_initialized = false;
	hardware_last_raw_steps = 0;
	hardware_accepted_steps = 0;
	hardware_pending_steps = 0;
	hardware_last_step_time = k_uptime_get();
}

void ReSetImuSteps(void)
{
	lsm6dso_steps_reset(&imu_dev_ctx);
	ResetHardwareStepFilter();

	g_last_steps = 0;
	g_steps = 0;
	g_distance = 0;
	g_calorie = 0;
	SaveStepSportData();
}

static bool ReadHardwareSteps(uint16_t *steps)
{
	uint8_t step_raw[2] = {0};

	if(steps == NULL)
		return false;

	*steps = 0;
	if(lsm6dso_number_of_steps_get(&imu_dev_ctx, step_raw) != 0)
		return false;

	*steps = (uint16_t)step_raw[0] | ((uint16_t)step_raw[1] << 8);
	return true;
}

/**
 * Validate a new raw hardware-counter value.
 *
 * The filter rejects counter resets, long gaps, implausible cadence, and fast
 * multi-step bursts. Valid changes are buffered until a walking session reaches
 * HARDWARE_STEP_CONFIRM_STEPS; the complete buffer is then accepted so startup
 * steps are not lost.
 *
 * Returns true only when hardware_accepted_steps increased.
 */
static bool AcceptHardwareStepDelta(uint16_t hardware_steps)
{
	uint16_t delta;
	uint32_t now = k_uptime_get();
	uint32_t elapsed;
	uint32_t interval;

	/* Establish a baseline; raw counts that predate this read are not owned here. */
	if(!hardware_step_filter_initialized)
	{
		hardware_last_raw_steps = hardware_steps;
		hardware_last_step_time = now;
		hardware_step_filter_initialized = true;
		return false;
	}

	/* A lower value means the 16-bit sensor counter was reset or wrapped. */
	if(hardware_steps < hardware_last_raw_steps)
	{
		hardware_last_raw_steps = hardware_steps;
		hardware_walk_confirmed = false;
		hardware_pending_steps = 0;
		hardware_last_step_time = now;
		return false;
	}

	delta = hardware_steps - hardware_last_raw_steps;
	if(delta == 0)
		return false;

	hardware_last_raw_steps = hardware_steps;
	elapsed = now - hardware_last_step_time;

	/* End the previous session and discard its first isolated post-gap event. */
	if(elapsed > HARDWARE_STEP_RESET_INTERVAL_MS)
	{
		hardware_walk_confirmed = false;
		hardware_pending_steps = 0;
		hardware_last_step_time = now;
		return false;
	}

	/* Polling may observe several hardware steps, so use their average interval. */
	interval = elapsed / delta;
	/* Reject a large, rapid counter jump typical of vibration or hand movement. */
	if((delta > HARDWARE_STEP_MAX_BURST_DELTA) && (interval < 700))
	{
		hardware_walk_confirmed = false;
		hardware_pending_steps = 0;
		hardware_last_step_time = now;
		return false;
	}

	/* Reject movement outside the supported walking cadence. */
	if((interval < HARDWARE_STEP_MIN_INTERVAL_MS) || (interval > HARDWARE_STEP_MAX_INTERVAL_MS))
	{
		hardware_walk_confirmed = false;
		hardware_pending_steps = 0;
		hardware_last_step_time = now;
		return false;
	}

	hardware_last_step_time = now;
	if(!hardware_walk_confirmed)
	{
		/* Hold candidates until enough consecutive hardware steps confirm walking. */
		hardware_pending_steps += delta;
		if(hardware_pending_steps < HARDWARE_STEP_CONFIRM_STEPS)
			return false;

		hardware_accepted_steps += hardware_pending_steps;
		hardware_pending_steps = 0;
		hardware_walk_confirmed = true;
		return true;
	}

	/* Once confirmed, accept each subsequent cadence-valid hardware delta. */
	hardware_accepted_steps += delta;
	return true;
}

void UpdateIMUData(void)
{
	uint16_t hardware_steps = 0;
	uint16_t previous_steps = g_steps;
	bool day_changed = !((last_sport.step_rec.timestamp.year == date_time.year)
		&&(last_sport.step_rec.timestamp.month == date_time.month)
		&&(last_sport.step_rec.timestamp.day == date_time.day));

#ifdef IMU_DEBUG
	LOGD("day_changed:%d", day_changed);
#endif

	if(day_changed)
	{
		g_last_steps = 0;
		lsm6dso_steps_reset(&imu_dev_ctx);
		ResetHardwareStepFilter();
	}
	else
	{
		if(!ReadHardwareSteps(&hardware_steps))
		{
		#ifdef IMU_DEBUG
			LOGD("read hardware steps false!");
		#endif
			return;
		}

		if(!AcceptHardwareStepDelta(hardware_steps))
		{
		#ifdef IMU_DEBUG
			LOGD("accept hardware steps false!");
		#endif
			return;
		}
	}


	g_steps = g_last_steps + hardware_accepted_steps;
	g_distance = (global_settings.person.step_length*g_steps)/100;
	g_calorie = (0.8*global_settings.person.weight*g_distance)/1000;

#ifdef IMU_DEBUG
	LOGD("hardware_accepted_steps:%d, g_steps:%d, previous_steps:%d", hardware_accepted_steps, g_steps, previous_steps);
#endif

	if(!day_changed && (g_steps == previous_steps))
		return;

#ifdef IMU_DEBUG
	LOGD("g_steps:%d,g_distance:%d,g_calorie:%d", g_steps, g_distance, g_calorie);
#endif

	SaveStepSportData();
	imu_redraw_steps_flag = true;
}

void GetSportData(uint16_t *steps, uint16_t *calorie, uint16_t *distance)
{
	if(hardware_step_enabled && imu_check_ok)
	{
		UpdateIMUData();
	}

	if(steps != NULL)
		*steps = g_steps;
	if(calorie != NULL)
		*calorie = g_calorie;
	if(distance != NULL)
		*distance = g_distance;
}

/*@Set hardware pedometer sensitivity*/
void lsm6dso_sensitivity(void)
{
	uint8_t deb_step = 3;//6;
	uint8_t delay_time[2] = {0x00, 0x00};
	//uint8_t delay_time[2] = {0x29, 0x00};
	//uint8_t delay_time[2] = {0x34, 0x00};
	//uint8_t delay_time[2] = {0x3D, 0x00};

	lsm6dso_pedo_debounce_steps_set(&imu_dev_ctx, &deb_step);
	lsm6dso_pedo_steps_period_set(&imu_dev_ctx, delay_time);
}
#endif

uint8_t IMU_GetID(void)
{
	uint8_t sensor_id = 0;
	
	lsm6dso_device_id_get(&imu_dev_ctx, &sensor_id);
	return sensor_id;
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

#ifdef CONFIG_FALL_DETECT_SUPPORT
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
	if(1
		#ifdef CONFIG_PPG_SUPPORT
		 && CheckSCC()
		#endif
		)
	{
		SCC_check_ok = true;
	}
}
#endif

void IMU_init(struct k_work_q *work_q)
{
#ifdef IMU_DEBUG
	LOGD("IMU_init");
#endif

#ifdef CONFIG_STEP_SUPPORT
	get_cur_sport_from_record(&last_sport);
#ifdef IMU_DEBUG
	LOGD("%04d/%02d/%02d last_steps:%d", last_sport.step_rec.timestamp.year,last_sport.step_rec.timestamp.month,last_sport.step_rec.timestamp.day,last_sport.step_rec.steps);
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

#ifdef CONFIG_STEP_SUPPORT
	if(global_settings.step_is_on)
	{
		StepCountingStart();
	}
	else
	{
		StepCountingStop();
	}
#endif

#ifdef CONFIG_SLEEP_SUPPORT
	if(global_settings.sleep_is_on)
	{
		StartSleepTimeMonitor();
	}
#endif

#ifdef IMU_DEBUG
	LOGD("IMU_init done!");
#endif
}

#ifdef CONFIG_STEP_SUPPORT
void IMURedrawSteps(void)
{
	if(screen_id == SCREEN_ID_STEPS 
		|| screen_id == SCREEN_ID_SLEEP
		|| screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_SPORT;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}
#endif

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

#ifdef CONFIG_STEP_SUPPORT
	if(global_settings.step_is_on && !hardware_step_enabled)
	{
		StepCountingStart();
	}
	else if(!global_settings.step_is_on && hardware_step_enabled)
	{
		StepCountingStop();
	}
#endif

	if(int1_event)	//tilt or step
	{
		bool tilt_detected = false;
	#ifdef CONFIG_STEP_SUPPORT
		bool step_detected = false;
		lsm6dso_all_sources_t status;
	#endif

	#ifdef IMU_DEBUG
		LOGD("int1 evt!");
	#endif
		int1_event = false;

		if(!imu_check_ok || !is_wearing())
			return;

	#ifdef CONFIG_STEP_SUPPORT
		if(lsm6dso_all_sources_get(&imu_dev_ctx, &status) == 0)
		{
			tilt_detected = status.fsm_status_a.is_fsm1;
			if(global_settings.step_is_on)
			{
				step_detected = status.emb_func_status.is_step_det;
			}
		}
	#else
		tilt_detected = is_tilt();
	#endif

		if(tilt_detected)
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
	#ifdef CONFIG_STEP_SUPPORT
		if(step_detected)
		{
		#ifdef IMU_DEBUG
			LOGD("steps trigger!");
		#endif
			UpdateIMUData();
		}
	#endif
	}

#ifdef CONFIG_STEP_SUPPORT	
	if(reset_steps)
	{
		reset_steps = false;

		if(!imu_check_ok)
			return;

		ReSetImuSteps(); 
		imu_redraw_steps_flag = true;
	}

	if(imu_redraw_steps_flag)
	{
		imu_redraw_steps_flag = false;

		if(!imu_check_ok)
			return;

		IMURedrawSteps();
	}
#endif

#ifdef CONFIG_SLEEP_SUPPORT
	if(update_sleep_parameter)
	{
		update_sleep_parameter = false;

		if(!imu_check_ok)
			return;

		UpdateSleepPara();
	}

	if(reset_sleep_data)
	{
		reset_sleep_data = false;

		if(!imu_check_ok)
			return;

		SleepDataReset();
	}
#endif

#ifdef CONFIG_FALL_DETECT_SUPPORT
	if (int2_event && global_settings.fall_check) //fall
	{
	#ifdef IMU_DEBUG
		LOGD("int2 evt!");
	#endif

		int2_event = false;

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
			fall_check_flag = true;
			fall_detection();
			fall_check_flag = false;
		#endif
		}
	}

	if(RUN_FD_FLAG)
	{
		RUN_FD_FLAG = false;
		fall_detection();
	}

	if(fall_result)
	{
		fall_result = false;

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
	  #ifdef CONFIG_PPG_SUPPORT
		StartSCC();
	  #endif
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
	
	if(fall_check_flag)
	{
		//k_sleep(K_MSEC(3));
	}
#endif
}
#endif/*CONFIG_IMU_SUPPORT*/
