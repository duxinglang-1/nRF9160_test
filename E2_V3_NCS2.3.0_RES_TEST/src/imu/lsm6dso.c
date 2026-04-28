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
#include "max32674.h" // SCC

//#define IMU_DEBUG
//#define SOFTWARE_STEP

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
uint64_t last_sample_time = 0;
static float variance_history[20] = {0};
static uint8_t var_index = 0;
static uint8_t peak_flag = 0; // 波峰

uint32_t last_step_time = 0;   // 上次步伐时间,Last step time
uint32_t min_step_interval = 70; //最小步频时间间隔(ms),Minimum step frequency time interval (ms)
uint32_t max_step_interval = 2000; //最大步频时间间隔(ms),Maximum step frequency time interval (ms)

static uint32_t peak_time = 0; // 波峰时间
static uint32_t trough_time = 0; // 波谷时间
#define STEP_THRESHOLD_BASE 0      // 步数检测基础阈值 Step count detection threshold
 
// 当前步数
static uint32_t total_step_count = 0;       // 总步数 Total steps
static float threshold = 0;              // 动态阈值
static uint32_t debounce_steps = 0; // 消抖步数，Dampening steps
static float mean_square = 0;

static float threshold_history[6] = {0};
static uint8_t threshold_index = 0;

static float angular_history[6] = {0};
static uint8_t angular_index = 0;

static float step_buffer[100] = {0};
static uint8_t step_index = 0;
#endif

#ifdef CONFIG_STEP_SUPPORT
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
			g_steps -= g_last_steps;
			g_distance = 0.7*g_steps;
			g_calorie = (0.55*60*g_distance)/1000;
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
	lsm6dso_device_id_get(&imu_dev_ctx, &whoamI);
	if(whoamI != LSM6DSO_ID)
		return;

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

	//Activity detection
    //Set duration for Activity detection to 9.62 ms (= 1 * 1 / ODR_XL)
    lsm6dso_wkup_dur_set(&imu_dev_ctx, 0x01);
    //Set duration for Inactivity detection to 4.92 s (= 1 * 512 / ODR_XL)
    lsm6dso_act_sleep_dur_set(&imu_dev_ctx, 0x01);
    //Set Activity/Inactivity threshold to 31.25 mg (= 1* FS_XL / 2^6)
    lsm6dso_wkup_threshold_set(&imu_dev_ctx, 0x01);
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
	int1_route.emb_func_int1.int1_step_detector = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);
	
#ifdef CONFIG_STEP_SUPPORT
	//Enable step counts algorithm
	lsm6dso_pedo_sens_set(&imu_dev_ctx, LSM6DSO_FALSE_STEP_REJ_ADV_MODE); // LSM6DSO_PEDO_BASE_MODE 虚假步数抑制高级模式
#endif
	
	// route tap and activity to INT2 pin
	lsm6dso_pin_int2_route_get(&imu_dev_ctx, &int2_route);
	int2_route.md2_cfg.int2_single_tap = PROPERTY_ENABLE;
	int2_route.md2_cfg.int2_sleep_change = PROPERTY_ENABLE;
	lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_route);

	lsm6dso_timestamp_set(&imu_dev_ctx, 1);

#ifdef CONFIG_STEP_SUPPORT
	// 启用LPF2滤波器
    lsm6dso_xl_filter_lp2_set(&imu_dev_ctx, PROPERTY_ENABLE);
	// 启用快速稳定模式（上电时滤波器快速稳定）
    lsm6dso_xl_fast_settling_set(&imu_dev_ctx, PROPERTY_ENABLE);
	// 高通滤波
	lsm6dso_xl_hp_path_internal_set(&imu_dev_ctx, LSM6DSO_USE_HPF);
	lsm6dso_xl_hp_path_on_out_set(&imu_dev_ctx, LSM6DSO_HP_ODR_DIV_200);
#endif
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
    
    // 禁用嵌入式功能中断
	lsm6dso_pin_int1_route_t int1_config = {0};
    lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_config);
    
    lsm6dso_pin_int2_route_t int2_config = {0};
    lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_config);
    
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
	g_calorie = (0.55*65*g_distance)/1000;

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
	uint8_t deb_step = 20;
	lsm6dso_pedo_debounce_steps_set(&imu_dev_ctx, &deb_step);

	//Set the sensitivity of the sensor,该函数用于设置两次有效步之间的最小时间间隔（单位：毫秒），避免因高频振动或快速动作导致单次动作被误判为多步。
	uint8_t delay_time[10] = {0x68, 0x00}; // 62
	//Lower Limit is 0 and Upper Limit is 50(32 in Hex), the delay time is 320ms
	// 建议改为 0x000F(~300ms) 或 0x0014(~400ms)，能有效过滤手持抖动等误触发，同时不漏计正常步行。
	// 加速度ODR为26Hz：300ms对应值为15(0x0F),400ms对应值为20(0x14)
	// 加速度ODR为52Hz: 300ms对应值为30(0x1E),400ms对应值为41(0x29)
	// 加速度ODR为104Hz: 300ms对应值为61(0x3D),400ms对应值为81(0x51)
	lsm6dso_pedo_steps_period_set(&imu_dev_ctx, &delay_time);
}
#endif

#ifdef SOFTWARE_STEP

typedef struct {
    float prev_x, prev_y, prev_z;
    float gravity_x, gravity_y, gravity_z;
} SimpleStepFilter_t;
static SimpleStepFilter_t step_filter;
AccelData_t input;
void SimpleStepFilter_Init(SimpleStepFilter_t *filter)
{
    memset(filter, 0, sizeof(SimpleStepFilter_t));
    filter->gravity_z = 1.0f;  // 初始重力估计
}
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
		lsm6dso_fifo_out_raw_get(&imu_dev_ctx, data_raw_angular_rate.u8bit);
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
 * 计步器初始化,Step counter initialization
 */
void StepCounter_Init() {

	min_step_interval = 70; //最小步频时间间隔(ms),Minimum step frequency time interval (ms)
	max_step_interval = 1500; //最大步频时间间隔(ms),Maximum step frequency time interval (ms)
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

	#if 0
	// 滤波
		// 1. 一阶低通滤波（α=0.3）
		input.x = 0.3f * accel->x + 0.7f * step_filter.prev_x;
		input.y = 0.3f * accel->y + 0.7f * step_filter.prev_y;
		input.z = 0.3f * accel->z + 0.7f * step_filter.prev_z;
		
		step_filter.prev_x = input.x;
		step_filter.prev_y = input.y;
		step_filter.prev_z = input.z;
		
		// 2. 自适应重力去除
		step_filter.gravity_x = 0.02f * input.x + 0.98f * step_filter.gravity_x;
		step_filter.gravity_y = 0.02f * input.y + 0.98f * step_filter.gravity_y;
		step_filter.gravity_z = 0.02f * input.z + 0.98f * step_filter.gravity_z;
		
		input.x -= step_filter.gravity_x;
		input.y -= step_filter.gravity_y;
		input.z -= step_filter.gravity_z;

		float magnitude = sqrtf(input.x*input.x + input.y*input.y + input.z*input.z); //-1.0f;
   		return magnitude;
	#endif

	float magnitude = sqrtf(accel->x*accel->x + accel->y*accel->y + accel->z*accel->z);
   	return magnitude;
}

/**
 * 过滤
 */
bool ContextAwareFilter(AccelData_t *accel) {

	#if 0
		float x = 0, y = 0, z = 0;
		x = accel->x / 1000;
		y = accel->y / 1000;
		z = accel->z / 1000;
	#endif

    float delta_mag = sqrtf(accel->x*accel->x + accel->y*accel->y + accel->z*accel->z);
    //LOGD("222_%f",delta_mag);
    // 更新方差历史_Update variance history
    variance_history[var_index] = delta_mag;
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

    LOGD("variance_%f",variance);
    if (variance < 0.015) { // 0.015
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
    threshold_index = (threshold_index + 1) % 6;
    
    float mean1 = 0, variance = 0;
    for (int i = 0; i < 6; i++) {
        mean1 += threshold_history[i];
    }
    mean1 /= 6.0f;

	//标准差
	 for (int i = 0; i < 6; i++) {
        float diff = threshold_history[i] - mean1;
        variance += diff * diff;
    }
    variance /= 6.0f;
	//LOGD("variance_%f",variance);

	//均方根
	float sum_sq1 = 0;
	for (int i = 0; i < 6; i++) {
		sum_sq1 += threshold_history[i] * threshold_history[i];
	}
	float rms1 = sqrtf(sum_sq1 / 6.0f);
	mean_square = rms1;

	//LOGD("mean1_%f",mean1);
	threshold = mean1; //+ 0.3 * variance; // 0.5~1.0

}
// 角速度均方根79、69
float update_angular_root_mean_square(void)
{
	float angular_mag = sqrtf(raw_gyro.x*raw_gyro.x + raw_gyro.y*raw_gyro.y/* + raw_gyro.z*raw_gyro.*/);
	//LOGD("angu_%f",angular_mag);
	angular_history[angular_index] = angular_mag;
	angular_index = (angular_index + 1) % 6;

	// 均方根 RMS = √(Σ(xi2) / N)
	float sum_sq = 0;
	for (int i = 0; i < 6; i++) {
		sum_sq += angular_history[i] * angular_history[i];
	}
	float rms = sqrtf(sum_sq / 6.0f);

	return rms;
}

/**
 * 一阶低通滤波器 (IIR)
 * alpha: 滤波系数 0~1, 越小越平滑, 延迟越大
 *   alpha = dt / (RC + dt), 典型值 0.1~0.3
 */
static AccelData_t lpf_prev = {0};
static bool lpf_initialized = false;

AccelData_t Accel_LowPassFilter(AccelData_t *input, float alpha) {
    AccelData_t output;
    if (!lpf_initialized) {
        lpf_prev = *input;
        lpf_initialized = true;
    }
    output.x = alpha * input->x + (1.0f - alpha) * lpf_prev.x;
    output.y = alpha * input->y + (1.0f - alpha) * lpf_prev.y;
    output.z = alpha * input->z + (1.0f - alpha) * lpf_prev.z;
    lpf_prev = output;
    return output;
}


/**
 * 滑动平均滤波器 (FIR)
 * window_size: 窗口大小, 建议 3~8
 */
#define MA_WINDOW_SIZE 5
static float ma_buffer[MA_WINDOW_SIZE] = {0};
static uint8_t ma_index = 0;
static bool ma_filled = false;

float Accel_MovingAverage(float value) {
    ma_buffer[ma_index] = value;
    ma_index = (ma_index + 1) % MA_WINDOW_SIZE;
    if (ma_index == 0) ma_filled = true;
    
    uint8_t count = ma_filled ? MA_WINDOW_SIZE : ma_index;
    float sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum += ma_buffer[i];
    }
    return sum / (float)count;
}

/**
 * 简易卡尔曼滤波器
 * 适用于合加速度的平滑处理
 */
typedef struct {
    float q;  // 过程噪声协方差, 典型 0.001~0.01
    float r;  // 测量噪声协方差, 典型 0.1~1.0
    float x;  // 估计值
    float p;  // 估计误差协方差
    bool init;
} KalmanFilter_t;

static KalmanFilter_t kf = {0.005f, 0.5f, 0, 1.0f, false};

float Accel_KalmanFilter(float measurement) {
    if (!kf.init) {
        kf.x = measurement;
        kf.init = true;
    }
    // 预测
    kf.p = kf.p + kf.q;
    // 更新
    float k = kf.p / (kf.p + kf.r);  // 卡尔曼增益
    kf.x = kf.x + k * (measurement - kf.x);
    kf.p = (1.0f - k) * kf.p;
    return kf.x;
}

/**
 * 带通滤波器 = 低通 + 高通(去直流/重力)
 * 去除重力偏移(约1g)，保留步行频率信号(0.5~3Hz)
 * 使用方法: 先 LowPass 再去均值
 */
static float bp_mean = 0;
static bool bp_initialized = false;

float Accel_BandPassFilter(float magnitude, float lpf_alpha, float mean_alpha) {
    // 第一步: 低通滤波平滑噪声
    float lpf_out;
    if (!bp_initialized) {
        bp_mean = magnitude;
        bp_initialized = true;
    }
    lpf_out = lpf_alpha * magnitude + (1.0f - lpf_alpha) * bp_mean;
    
    // 第二步: 减去滑动均值去除重力/直流分量
    bp_mean = mean_alpha * magnitude + (1.0f - mean_alpha) * bp_mean;
    
    return lpf_out - bp_mean;
}

/**
 * 重置所有滤波器状态
 */
void Accel_FilterReset(void) {
	lpf_initialized = false;
    memset(&lpf_prev, 0, sizeof(lpf_prev));
    bp_initialized = false;
    bp_mean = 0;
    memset(ma_buffer, 0, sizeof(ma_buffer));
    ma_index = 0;
    ma_filled = false;
    kf.init = false;
    kf.p = 1.0f;
}

/**
 * @brief 检测是否为真实步数（通过加速度幅度判断）
 * @param acceleration_mg 实时三轴加速度值(mg)
 * @return true为有效步数，false为无效
 */
bool is_valid_step_detection(float acceleration_x, float acceleration_y, float acceleration_z) {
    // 计算总加速度幅值
    float total_acceleration = sqrtf(acceleration_x * acceleration_x + 
                                    acceleration_y * acceleration_y + 
                                    acceleration_z * acceleration_z);
    
	//LOGD("acc_%f",total_acceleration);
	LOGD("acc_z_%f",acceleration_z);

    // 4g量程下，正常步行加速度幅值通常在1.0g-2.2g之间
    // 手臂摆动通常幅值较小(<1.0g)或过大(>2.8g)
    if (total_acceleration < 1000 || total_acceleration > 2800) {
        //return false; // 幅值不在正常步数范围内
    }
    
    // 检查Z轴（通常是垂直方向）加速度变化特征
    // 步行时Z轴会有明显的周期性变化
    if (acceleration_z > 100 || acceleration_z < -100) {
        // Z轴幅值过大，可能是跳跃或其他剧烈运动
        return false;
    }
    
    // 检查X轴和Y轴的相对幅值
    // 步行时X轴（前进方向）和Y轴（侧向）应该有特定比例关系
    float horizontal_accel = sqrtf(acceleration_x * acceleration_x + acceleration_y * acceleration_y);
	//LOGD("horizontal_%f",horizontal_accel);
    if (horizontal_accel > 2000) {
        // 水平方向加速度过大，可能是跑步或快速移动
       // return false;
    }
    //LOGD("step_1");
    return true;
}

/**
 * @brief 针对手臂摆动过滤算法
 */
void advanced_arm_swing_filtering(void) {
    float accel_x, accel_y, accel_z;
    get_sensor_reading(&accel_x, &accel_y, &accel_z);
    
	//是否行走
	if (!is_valid_step_detection(accel_x, accel_y, accel_z))
	{
		return;
	}
	
    // 计算加速度变化率
    float delta_x = fabsf(accel_x - prev_acceleration[0]);
    float delta_y = fabsf(accel_y - prev_acceleration[1]);
    float delta_z = fabsf(accel_z - prev_acceleration[2]);
    
    float total_delta = delta_x + delta_y + delta_z;
    LOGD("total_%f",total_delta);
    // 如果加速度变化过于频繁且幅值较小，可能是手臂摆动
    if (total_delta < 300 && !detected_walking) {  // 阈值可调
        arm_swing_counter++;
        if (arm_swing_counter > 10) {  // 连续10次小变化，认为是手臂摆动
            // 可以暂时忽略计步或降低灵敏度
            detected_walking = false;
        }
    } else {
        arm_swing_counter = 0;
        detected_walking = true;
		//LOGD("step_2");
    }
    
    prev_acceleration[0] = accel_x;
    prev_acceleration[1] = accel_y;
    prev_acceleration[2] = accel_z;
}

/**
 * 主计步处理函数——Main step counting processing function
 * 检测到有效步伐返回true,If a valid step is detected, return true.
 */
bool StepCounter_Process(uint32_t timestamp) {

    float magnitude = 0, gyro_magn = 0;
	float gyro_x = 0, gyro_y = 0, gyro_z = 0;
	if ((last_step_time == 0) ) // 第一次运行
	{
		last_step_time = timestamp;
	}
    
    // 读取原始加速度数据_Read the original acceleration data
    if (!LSM6DSO_ReadAcceleration(&raw_accel)) {
        return false;
    }
	
    // 计算合加速度_Calculate the resultant acceleration
    magnitude = CalculateMagnitude(&raw_accel);
    LOGD("heACC:%f",magnitude);

	magnitude = Accel_KalmanFilter(magnitude);//Accel_MovingAverage(Accel_KalmanFilter(magnitude));// Accel_BandPassFilter(magnitude, 0.2f, 0.01f);
	//LOGD("KmACC:%f",filtered);

	if (!ContextAwareFilter(&raw_accel)) {
      	return false;
  	}

	#if 1
	// 更新阈值（使用滑动平均）Update threshold (using moving average)
    update_threshold(magnitude);

	 // 检测步数：当前值超过阈值+基础阈值时认为是一步 
	 // Number of steps detected: When the current value exceeds the threshold plus the base threshold, it is considered as one step.
	 LOGD("threshold:%f",threshold);
	 
	 #if 1
	 uint64_t time_diff = timestamp - last_step_time;
	 if ((time_diff < min_step_interval) && (last_step_time > 0))
	 {
		  return false;
	 }


	 if ((time_diff > max_step_interval*3) && (debounce_steps > 0))
	 {
		debounce_steps = 0; //重置去抖步数，Reset the anti-shake count
		last_step_time = timestamp;
	 }
	 #endif

	#if 0
	//uint64_t time_diff_1 = timestamp - last_step_time;
	if (0)// ((time_diff_1 > max_step_interval*10) && (debounce_steps > 0)) // 不活动超过20秒
	 {
		debounce_steps = 0; //重置去抖步数，Reset the anti-shake count
		last_step_time = timestamp;
		step_index = 0;
	 }

	step_buffer[step_index] = magnitude;
    step_index = (step_index + 1) % 100;
	if (step_index > 2)
	{
		if ( (step_buffer[step_index-2] > step_buffer[step_index-3]) &&
        (step_buffer[step_index-2] > step_buffer[step_index-1])) 
		{	// 波峰
			
			peak_time = timestamp;
			peak_flag = 1;

			#if 0
			// 时间
			uint64_t time_diff = timestamp - last_step_time;
			LOGD("time_%d",time_diff);
			if ((time_diff > min_step_interval) && (time_diff < max_step_interval))
			{
				debounce_steps ++;
				if (debounce_steps > 5)
				{
					// 检测到一步，Detected one step
				total_step_count++;
				// 补偿前15步，The first 15 steps before compensation
				if (debounce_steps == 6)
				{
					total_step_count = total_step_count + 5;
				}
				g_steps = total_step_count;
				}
		
				//last_step_time = timestamp;
			}

			//Wave_valley_flag = 1;
			// 静止后的时间重置
			if (time_diff > (max_step_interval * 3))
			{
				last_step_time = timestamp;
			}

			last_step_time = timestamp;
			#endif
			
		}
		#if 1
		if ((peak_flag == 1) && (step_buffer[step_index-2] < step_buffer[step_index-3]) &&
        (step_buffer[step_index-2] < step_buffer[step_index-1])) 
		{	// 波谷
			peak_flag = 0;

			// 时间
			uint64_t time_diff = timestamp - peak_time;
			LOGD("time_%d",time_diff);
			if ((time_diff > min_step_interval) && (time_diff < max_step_interval))
			{
				// 检测到一步，Detected one step
				total_step_count++;
				g_steps = total_step_count;
			}

			// 静止后的时间重置
			if (time_diff > (max_step_interval * 3))
			{
				peak_time = timestamp;
			}
		}
		#endif
		
	}
	#endif

	// 角速度
    if (!LSM6DSO_ReadAngular(&raw_gyro)) {
        return false;
    }
	
	//角速度均方值
	float ang_square = update_angular_root_mean_square();
	//LOGD("ang_square_%f",ang_square);

	if (ang_square < 120)
	{
		//return false;
	}

	//LOGD("mean_%f",mean_square);
	if (mean_square < 1.1)
	{
		//return false;
	}

	float accel_x, accel_y, accel_z;
    get_sensor_reading(&accel_x, &accel_y, &accel_z);
	//是否行走
	if (!is_valid_step_detection(accel_x, accel_y, accel_z))
	{
		//return false;
	}

	#if 1
	if (magnitude > (threshold + STEP_THRESHOLD_BASE))
	{
		// 时间
		//uint64_t time_diff = timestamp - last_step_time;
		//LOGD("time_%d",time_diff);
		if (1)//((time_diff > min_step_interval) && (time_diff < max_step_interval))
		{
			//LOGD("STEP_MA:%f",magnitude);
			debounce_steps ++;
			//有效步数大于20步开始计步，Counting begins when the number of steps is greater than 10.
			if (debounce_steps > 20)
			{
				// 检测到一步，Detected one step
				total_step_count++;
			
				// 补偿前20步，The first 20 steps before compensation
				if (debounce_steps == 21)
				{
					total_step_count = total_step_count + 20;
				}
				
				g_steps = total_step_count;
				g_distance = 0.7*g_steps;
				g_calorie = (0.55*65*g_distance)/1000;
			}
		}

		last_step_time = timestamp;
		
    }
	#endif
	
	#endif

    return false;
}

//开始计步循环调用_Start the step-counting loop call
void update_step_loop(void) {
   
   uint64_t current_time = k_uptime_get();//毫秒_Millisecond
    
    // 52Hz采样
    //if (current_time - last_sample_time >= (1000/52)) {
        //last_sample_time = current_time;
        
        // 处理传感器数据
        if (StepCounter_Process(current_time)) {
           //.
        }
    //}
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
	if(CheckSCC())
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
	lsm6dso_steps_reset(&imu_dev_ctx); //reset step counter
	lsm6dso_sensitivity();
#endif

#ifdef CONFIG_SLEEP_SUPPORT
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
			UpdateIMUData();
			imu_redraw_steps_flag = true;
		}
	#endif
	}

#ifdef SOFTWARE_STEP
	//update_step_loop(); // 计步算法_Step counting algorithm
#endif

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
	
	if(fall_check_flag)
	{
		//k_sleep(K_MSEC(3));
	}
#endif
}
#endif/*CONFIG_IMU_SUPPORT*/
