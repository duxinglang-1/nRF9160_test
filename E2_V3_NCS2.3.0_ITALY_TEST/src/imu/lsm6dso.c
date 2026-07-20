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
#define SOFTWARE_STEP

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
#ifdef SOFTWARE_STEP
typedef struct {
    float x;
    float y;
    float z;
} AccelData_t;

typedef struct {
    float x;
    float y;
    float z;
} GyroData_t;

AccelData_t raw_accel;
GyroData_t raw_gyro;
static axis3bit16_t data_raw_angular_rate;
static float angular_rate_mdps[3];
//uint64_t last_sample_time = 0;
static float variance_history[20] = {0};
static uint8_t var_index = 0;

static bool step_miscalculation = false;

//static uint16_t last_step_num_imu = 0; // imu上一次的步数
static uint16_t total_step_count = 0;       // 总步数 Total steps
//static uint16_t miscounting_steps = 0; // 误计步
static float threshold = 0;              // 动态阈值
static float acc_mean_square = 0;
static float ang_variance = 0;
uint32_t last_step_time = 0;
uint32_t min_step_interval = 200; //最小步频时间间隔(ms),Minimum step frequency time interval (ms)
uint32_t max_step_interval = 2500; //最大步频时间间隔(ms),Maximum step frequency time interval (ms)
static uint16_t debounce_steps = 0; 
static bool debounce_steps_buf = true;
#define STEP_DEBOUNCE_BASE 8 // 消抖步数，Dampening steps
uint64_t current_time = 0;

static float threshold_history[10] = {0};
static uint8_t threshold_index = 0;

static float angular_history[10] = {0};
static uint8_t angular_index = 0;

static float step_history[50] = {0};
static uint8_t step_index = 0;

static float difference_history[20] = {0};
static uint8_t difference_index = 0;

static float step_frequency_history[100] = {0};
static uint8_t step_frequency_index = 0;

static float valid_amplitude = 0; // 最小峰谷差值系数门限
//static bool is_amplitude = false;
static bool amplitude_threshold = true;
//static bool peak_buf = false;
//static bool step_peak = false;
static int step_status = 0;
static float current_peak = 0;
static float current_wave = 0;
//static bool wave_buf = false;
static float frequency_buf = 0;
//static bool is_frequency = false;
static bool frequency_threshold = true;
#endif

bool reset_steps = false;
bool imu_redraw_steps_flag = true;
uint16_t g_last_steps = 0;
uint16_t g_steps = 0;
uint16_t g_calorie = 0;
uint16_t g_distance = 0;

static float prev_acceleration[3] = {0};
static int arm_swing_counter = 0;
static bool detected_walking = false;

void ClearAllStepRecData(void)
{
	uint8_t tmpbuf[STEP_REC2_DATA_SIZE] = {0xff};

	g_last_steps = 0;
	g_steps = 0;
	g_distance = 0;
	g_calorie = 0;

	#ifdef SOFTWARE_STEP
	total_step_count = 0;
	#endif
	//miscounting_steps = 0;
	//last_step_num_imu = 0;
		
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

#ifdef CONFIG_STEP_SUPPORT
	//Enable step counts algorithm
	//lsm6dso_pedo_sens_set(&imu_dev_ctx, LSM6DSO_FALSE_STEP_REJ_ADV_MODE); // LSM6DSO_PEDO_BASE_MODE 虚假步数抑制高级模式
#endif

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
	//int1_route.emb_func_int1.int1_step_detector = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);
	
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
void StepCountingStart(void)
{

}

void StepCountingStop(void)
{

}

void ReSetImuSteps(void)
{
	lsm6dso_steps_reset(&imu_dev_ctx);

	g_last_steps = 0;
	g_steps = 0;
	g_distance = 0;
	g_calorie = 0;

	#ifdef SOFTWARE_STEP
	total_step_count = 0;
	#endif
	//miscounting_steps = 0;
	//last_step_num_imu = 0;
	
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
	uint16_t steps = 0,temporary_steps = 0;
	GetImuSteps(&steps);
	
	//LOGD("steps:%d", steps);
#if 0
	if (steps >= last_step_num_imu)
	{
		if (step_miscalculation)
		{ 
			temporary_steps = steps - last_step_num_imu;
			uint32_t time_diff = current_time - last_step_time;

			if ((temporary_steps > 5) && (time_diff < min_step_interval))
			{
				total_step_count += 4;
			}
			else
			{
				total_step_count += (steps - last_step_num_imu);
			}

			last_step_time = current_time;
		}
		else
		{
			miscounting_steps += (steps - last_step_num_imu);
		}
	}

	last_step_num_imu = steps;
#endif

	//LOGD("total_step_%d",total_step_count);
	//LOGD("miscounting_steps_%d",miscounting_steps);

	//g_steps = total_step_count+g_last_steps;
	//LOGD("g_steps:%d", g_steps);
	
	g_distance = (global_settings.person.step_length*g_steps)/100;
	g_calorie = (0.8*global_settings.person.weight*g_distance)/1000;

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

/*@Set Sensor sensitivity
// 参数换算表（针对不同ODR）
104Hz ODR参数换算：
- 采样间隔：1/104 ≈ 9.6ms
- 300ms = 300/9.6 ≈ 31 个采样点
- 400ms = 400/9.6 ≈ 41 个采样点  
- 500ms = 500/9.6 ≈ 52 个采样点
- 600ms = 600/9.6 ≈ 62 个采样点

推荐参数：
- deb_step: 15-25 (对应144-240ms)
- delay_time: 30-60 (对应288-576ms，建议52即500ms)
*/
void lsm6dso_sensitivity(void)
{ 
	//Set the debounce steps典型值为 0x00（默认）到 0x07，对应连续检测 1~8次 有效步态信号后才会计步。若 buff = 0x03，表示需连续检测到 4次 有效步态信号才会触发一次步数增加。
	uint8_t deb_step = 10;
	lsm6dso_pedo_debounce_steps_set(&imu_dev_ctx, &deb_step);

	//Set the sensitivity of the sensor,该函数用于设置两次有效步之间的最小时间间隔（单位：毫秒），避免因高频振动或快速动作导致单次动作被误判为多步。
	uint8_t delay_time[10] = {0x62, 0x00}; // 62
	//Lower Limit is 0 and Upper Limit is 50(32 in Hex), the delay time is 320ms
	// 建议改为 0x000F(~300ms) 或 0x0014(~400ms)，能有效过滤手持抖动等误触发，同时不漏计正常步行。
	// 加速度ODR为26Hz：300ms对应值为15(0x0F),400ms对应值为20(0x14)
	// 加速度ODR为52Hz: 300ms对应值为30(0x1E),400ms对应值为41(0x29)
	// 加速度ODR为104Hz: 300ms对应值为61(0x3D),400ms对应值为81(0x51)
	lsm6dso_pedo_steps_period_set(&imu_dev_ctx, &delay_time);
}
#endif

#ifdef CONFIG_STEP_SUPPORT
#ifdef SOFTWARE_STEP
/**
 * 读取加速度计数据_Read the data from the accelerometer
 */
bool LSM6DSO_ReadAcceleration(AccelData_t *accel) {
   uint8_t reg;

	lsm6dso_xl_flag_data_ready_get(&imu_dev_ctx, &reg);
	if(reg)
	{
		memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
		lsm6dso_acceleration_raw_get(&imu_dev_ctx, data_raw_acceleration.u8bit);
		acceleration_mg[0] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[0]); // 2g
		acceleration_mg[1] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[1]);
		acceleration_mg[2] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[2]);

		accel->x = acceleration_mg[0]/1000;
    	accel->y = acceleration_mg[1]/1000;
   		accel->z = acceleration_mg[2]/1000;

		/*accel->x = acceleration_mg[0];
    	accel->y = acceleration_mg[1];
   		accel->z = acceleration_mg[2];*/

		/*accel->x = acceleration_mg[0];
    	accel->y = acceleration_mg[1];
   		accel->z = acceleration_mg[2];
		
		accel->x = (accel->x < 0) ? -accel->x : accel->x;
		accel->y = (accel->y < 0) ? -accel->y : accel->y;
		accel->z = (accel->z < 0) ? -accel->z : accel->z;
		
		// 计算合成加速度的平方和 (归一化处理)
		accel->x /= 10.0f;
		accel->y /= 10.0f;
		accel->z /= 10.0f;*/

		//LOGD(acceleration_mg[0],"X:%d");
		 return true;
	}
	else 
	{
		return false;
	}
}
// 角速度数据
bool LSM6DSO_ReadAngular(GyroData_t *gyro)
{
	 uint8_t reg;
	lsm6dso_gy_flag_data_ready_get(&imu_dev_ctx, &reg);
	if (reg)
	{
		memset(data_raw_angular_rate.u8bit, 0x00, 3*sizeof(int16_t));
		lsm6dso_angular_rate_raw_get(&imu_dev_ctx, data_raw_angular_rate.u8bit);
		angular_rate_mdps[0] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[0]);
		angular_rate_mdps[1] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[1]);
		angular_rate_mdps[2] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[2]);

		gyro->x = angular_rate_mdps[0]/1000;
		gyro->y = angular_rate_mdps[1]/1000;
		gyro->z = angular_rate_mdps[2]/1000;

		return true;
	}
	else
	{
		return false;
	}

}


/**
 * 计算合加速度并去重力
 */
float CalculateMagnitude(AccelData_t *accel) {

	#if 0
		float x = 0, y = 0, z = 0;

		x = accel->x / 1000;
		y = accel->y / 1000;
		z = accel->z / 1000;
		
		float magnitude = sqrtf(x*x + y*y + z*z);
   		return magnitude; //fabsf(magnitude);
	#endif

	float magnitude = sqrtf(accel->x*accel->x + accel->y*accel->y + accel->z*accel->z);
   	return magnitude;
}

/**
 * 过滤
 */
bool ContextAwareFilter(float magnitude) {

	#if 0
		float x = 0, y = 0, z = 0;
		x = sensor_x / 1000;
		y = sensor_y / 1000;
		z = sensor_z / 1000;
	#endif

    //float delta_mag = sqrtf(x*x + y*y + z*z);
    //LOGD("222_%f",delta_mag);
    // 更新方差历史_Update variance history
    variance_history[var_index] = magnitude;
    var_index = (var_index + 1) % 20;
    
    // 计算加速度方差_Calculate the variance of acceleration
    float mean = 0, variance = 0;
    for (int i = 0; i < 20; i++) {
        mean += variance_history[i];
    }
    mean /= 20.0f;
    
    for (int i = 0; i < 20; i++) {
        float diff = variance_history[i] - mean;
        variance += diff * diff;
    }
    variance /= 20.0f;

    //LOGD("variance_%f",variance);
    if (variance < 0.005) { // 0.015
        // 变化太小，可能静止或车辆匀速行驶_The change is too small, possibly resulting in a stationary state or vehicle movement.
		
        return false;
    }
    
    if (variance > 2.0f) { // 1.0f
        // 变化太大，可能是剧烈振动或冲击_The change is too significant. It might be due to intense vibration or impact.
		
         return false;
    }
    
    return true;
}

void update_threshold(float magnitude) {
	// 计算均值 Calculate the mean value
    threshold_history[threshold_index] = magnitude;
    threshold_index = (threshold_index + 1) % 10;
    
    float mean1 = 0, variance = 0;
    for (int i = 0; i < 10; i++) {
        mean1 += threshold_history[i];
    }
    mean1 /= 10.0f;

#if 0
	//标准差
	 for (int i = 0; i < 10; i++) {
        float diff = threshold_history[i] - mean1;
        variance += diff * diff;
    }
    variance /= 10.0f;
	//LOGD("variance_%f",variance);
#endif

	//均方根 Root Mean Square
	float sum_sq1 = 0;
	for (int i = 0; i < 10; i++) {
		sum_sq1 += threshold_history[i] * threshold_history[i];
	}
	acc_mean_square = sqrtf(sum_sq1 / 10.0f);

	//LOGD("mean1_%f",mean1);
	threshold = mean1; //mean1 - mean1 * 0.3; //mean1; //+ 0.3 * variance; // 0.5~1.0

}
// 角速度均方根 Root mean square of angular velocity
float update_angular_root_mean_square(void)
{
	float angular_mag = sqrtf(raw_gyro.x*raw_gyro.x + raw_gyro.y*raw_gyro.y + raw_gyro.z*raw_gyro.z);
	//LOGD("angu_%f",angular_mag);
	angular_history[angular_index] = angular_mag;
	angular_index = (angular_index + 1) % 10;

	// 均方根 RMS = √(Σ(xi2) / N)
	float sum_sq = 0;
	for (int i = 0; i < 10; i++) {
		sum_sq += angular_history[i] * angular_history[i];
	}
	float rms = sqrtf(sum_sq / 10.0f);

	// 角速度方差 Angular velocity variance
    float mean = 0, variance = 0;
    for (int i = 0; i < 10; i++) {
        mean += angular_history[i];
    }
    mean /= 10.0f;
    
    for (int i = 0; i < 10; i++) {
        float diff = angular_history[i] - mean;
        variance += diff * diff;
    }
     variance /= 10.0f;
	 ang_variance = variance / 10000;

	return rms;
}

void stepCountDataUpdated(void)
{
	g_steps = total_step_count+g_last_steps;;
	g_distance = (global_settings.person.step_length*g_steps)/100;
	g_calorie = (0.8*global_settings.person.weight*g_distance)/1000;

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

	imu_redraw_steps_flag = true;
}

// 峰谷差值系数门限
void PeakValleyDifferenceComparison(float difference_value)
{
	difference_history[difference_index] = difference_value;
	difference_index = (difference_index + 1) % 20;

	float min_dif = 0,difference = 0;
	for (int i = 0; i < 20; i++)
	{
		difference = difference_history[i];

		if (difference > 0)
		{
			min_dif = (difference < min_dif) ? difference : min_dif;
		}
	}

	valid_amplitude = min_dif * 0.5;
}

// 2秒（200个采样）平均间隔步频门限
#if 1
void averageStepFrequencyThreshold(uint32_t time_diff)
{
	step_frequency_history[step_frequency_index] = time_diff;
	step_frequency_index = (step_frequency_index + 1) % 100;

	float average_interval = 0, toal_value = 0, avg_freq= 0, instant_freq = 0;
	for (int i = 0; i < 100; i++)
	{
		toal_value += step_frequency_history[i];
	}
	average_interval = toal_value / 100.0f;

	avg_freq = 60 / average_interval;
	instant_freq = 60 / (float)time_diff;
	frequency_buf = fabs(instant_freq - avg_freq);

	//frequency_buf = fabs(((float)time_diff - average_interval) / average_interval);
	//LOGD("frequency_buf_%f",frequency_buf);
}
#endif

void softwareStepAlgorithm(float magnitude,uint32_t timestamp)
{
	step_history[step_index] = magnitude;
    step_index = (step_index + 1) % 50;
	
	uint8_t n = step_index;
	if (n >= 2)
	{
		#if 1
		// 波峰
		if ((step_history[n-2] > step_history[n-3]) && (step_history[n-2] > step_history[n-1]))
		{
			current_peak = step_history[n-2];

			#if 1
			switch (step_status)
			{
			case 0:
				step_status = 1;
				break;

			case 1:
				step_status = 2;
				break;
			
			default:
				step_status = 1;
				break;
			}
			#endif
		}
		#endif

		#if 1
		// 波谷
		if ((step_history[n-2] < step_history[n-3]) && (step_history[n-2] < step_history[n-1]))
		{
			current_wave = step_history[n-2];

			switch (step_status)
			{
			case 0:
				step_status = 1;
				break;

			case 1:
				step_status = 2;
				break;
			
			default:
				step_status = 1;
				break;
			}
		}
		#endif
	}

	if (step_status == 2)
	{
		float difference_value = current_peak - current_wave;

		if (last_step_time ==0)
		{
			last_step_time = timestamp;
		}
		uint32_t time_diff = timestamp - last_step_time;
		//LOGD("time_%d",time_diff);

		averageStepFrequencyThreshold(time_diff);

		if ((time_diff > max_step_interval*6))
		{
			debounce_steps_buf = true;
			debounce_steps = 0;
			last_step_time = timestamp;
		}

		// 峰谷差值系数门限
		if (debounce_steps > STEP_DEBOUNCE_BASE)
		{
			amplitude_threshold = (difference_value > valid_amplitude) ? true : false;
		}
		else
		{
			amplitude_threshold = true;
		}
		
		// 平均步频门限
		#if 1
		if (debounce_steps > STEP_DEBOUNCE_BASE)
		{
			frequency_threshold = (frequency_buf < 0.5) ? true : false;
			//LOGD("frequency_buf2_%f",frequency_buf);
		}
		else
		{
			frequency_threshold = true;
		}
		#endif
		
		// 步伐时间门限+峰谷差值系数门限+平均步频门限
		if ((time_diff > min_step_interval) && (time_diff < max_step_interval))
		{
			last_step_time = timestamp;

			if(amplitude_threshold && frequency_threshold)
			{
				debounce_steps ++;
				if (debounce_steps > STEP_DEBOUNCE_BASE)
				{
					if (debounce_steps_buf)
					{
						debounce_steps_buf = false;
						total_step_count = total_step_count + debounce_steps;
					}

					total_step_count ++;

					stepCountDataUpdated();
				}
			}
			// 更新门限阈值
			PeakValleyDifferenceComparison(difference_value);
		}
		
	}
}

/**
 * 主计步处理函数——Main step counting processing function
 * 检测到有效步伐返回true,If a valid step is detected, return true.
 */
bool StepCounter_Process(uint32_t timestamp) {

    float magnitude = 0;

    // 读取原始加速度数据_Read the original acceleration data
    if (!LSM6DSO_ReadAcceleration(&raw_accel)) {
        return false;
    }

    // 计算合加速度_Calculate the resultant acceleration
    magnitude = CalculateMagnitude(&raw_accel);
    //LOGD("heACC:%f",magnitude);

	// 滤波
    //magnitude = Biquad_Filter(&g_bp_filter, magnitude);
	//LOGD("KmACC:%f",magnitude);
	
	if (!ContextAwareFilter(magnitude)) {
      	return false;
  	}

#if 1
	// 更新阈值（使用滑动平均）Update threshold (using moving average)
    update_threshold(magnitude);

	 // 检测步数：当前值超过阈值+基础阈值时认为是一步 
	 // Number of steps detected: When the current value exceeds the threshold plus the base threshold, it is considered as one step.
	 //LOGD("threshold:%f",threshold);

	// 角速度R
    if (!LSM6DSO_ReadAngular(&raw_gyro)) {
         return false;
    }
	
	//角速度均方根
	float ang_square = update_angular_root_mean_square();
	//LOGD("ang_square_%f",ang_square);
	//LOGD("acc_square_%f",acc_mean_square);

	// Z轴稳定性验证：行走时垂直方向相对稳定
	float z_stability = fabsf(raw_accel.z);
	//LOGD("z_stability_%f",z_stability);
	// 角速度方差
	//LOGD("ang_variance_%f",ang_variance);

	// 过滤异常
	float ratio = ang_square / (acc_mean_square * 50.0f);
	//LOGD("ratio_%f",ratio);
	if ((ratio > 6.0f) || (ratio < 0.6f))
	{
		return false;
	}
	
	if ((ang_square < 290) && (z_stability <= 0.6f) && (acc_mean_square > 0.9) /*&& (magnitude >= threshold)/*&& (ang_variance < 1.5)*/)
	{
		softwareStepAlgorithm(magnitude,timestamp);
		
		return true;
	}
	else
	{
		return false;
	}
#endif

    return false;
}

//开始计步循环调用_Start the step-counting loop call
void update_step_loop(void) {
   
   current_time = k_uptime_get();//毫秒_Millisecond
   //uint32_t time2 = (date_time.second + date_time.minute*60 +date_time.hour*60*60) * 1000;
    
    // 52Hz采样
    if (1) //(current_time - last_sample_time >= (1000/52)) 
	{
        //last_sample_time = current_time + (1000/52);

        // 计步算法
		step_miscalculation = StepCounter_Process(current_time);
    }

}

uint16_t getSoftwareStep(void)
{
	return g_steps;
}
#endif

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

#ifdef CONFIG_STEP_SUPPORT
	//lsm6dso_steps_reset(&imu_dev_ctx); //reset step counter
	//lsm6dso_sensitivity();
#endif

#ifdef CONFIG_SLEEP_SUPPORT
	if(global_settings.sleep_is_on)
		StartSleepTimeMonitor();
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

#ifdef SOFTWARE_STEP
	if (global_settings.step_is_on)
	{
		update_step_loop(); // 计步算法_Step counting algorithm
	}
#endif

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
	#ifdef CONFIG_STEP_SUPPORT	
		else
		{
		#ifdef IMU_DEBUG	
			LOGD("steps trigger!");
		#endif	
			//UpdateIMUData();
			//imu_redraw_steps_flag = true;
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
