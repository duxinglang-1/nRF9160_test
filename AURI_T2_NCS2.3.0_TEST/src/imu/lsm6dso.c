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

static struct k_work_q *imu_work_q;
static struct k_work imu_work;

static bool imu_check_ok = false;
static uint8_t whoamI, rst;
static struct device *i2c_imu;
static struct device *gpio_imu = NULL;
static struct gpio_callback gpio_cb1,gpio_cb2;

bool reset_steps = false;
bool imu_redraw_steps_flag = true;
#ifdef CONFIG_FALL_DETECT_SUPPORT
static bool fall_check_flag = false;
#endif

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

bool sensor_init(void)
{
	lsm6dso_device_id_get(&imu_dev_ctx, &whoamI);
	if(whoamI != LSM6DSO_ID)
		return false;

	lsm6dso_reset_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_reset_get(&imu_dev_ctx, &rst);

	lsm6dso_i3c_disable_set(&imu_dev_ctx, LSM6DSO_I3C_DISABLE);

	lsm6dso_xl_full_scale_set(&imu_dev_ctx, LSM6DSO_2g);
	lsm6dso_gy_full_scale_set(&imu_dev_ctx, LSM6DSO_250dps);
	lsm6dso_block_data_update_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_watermark_set(&imu_dev_ctx, PATTERN_LEN);
	lsm6dso_fifo_stop_on_wtm_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_mode_set(&imu_dev_ctx, LSM6DSO_STREAM_TO_FIFO_MODE);

	lsm6dso_fifo_xl_batch_set(&imu_dev_ctx, LSM6DSO_XL_BATCHED_AT_52Hz);
	lsm6dso_fifo_gy_batch_set(&imu_dev_ctx, LSM6DSO_GY_BATCHED_AT_52Hz);

	lsm6dso_xl_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_52Hz);
	lsm6dso_gy_data_rate_set(&imu_dev_ctx, LSM6DSO_GY_ODR_52Hz);

	lsm6dso_xl_power_mode_set(&imu_dev_ctx, LSM6DSO_LOW_NORMAL_POWER_MD);

	lsm6dso_tap_detection_on_z_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_tap_detection_on_y_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_tap_detection_on_x_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_tap_threshold_x_set(&imu_dev_ctx, 0x12);
	lsm6dso_tap_threshold_y_set(&imu_dev_ctx, 0x12);
	lsm6dso_tap_threshold_z_set(&imu_dev_ctx, 0x12);

	lsm6dso_tap_quiet_set(&imu_dev_ctx, 0x03);
	lsm6dso_tap_shock_set(&imu_dev_ctx, 0x03);

	lsm6dso_tap_mode_set(&imu_dev_ctx, LSM6DSO_ONLY_SINGLE);

	lsm6dso_pedo_sens_set(&imu_dev_ctx, LSM6DSO_PEDO_BASE_MODE);

	lsm6dso_int_notification_set(&imu_dev_ctx, LSM6DSO_BASE_PULSED_EMB_LATCHED);

	lsm6dso_long_cnt_int_value_set(&imu_dev_ctx, 0x0000U);
	lsm6dso_fsm_start_address_set(&imu_dev_ctx, LSM6DSO_START_FSM_ADD);
	lsm6dso_fsm_number_of_programs_set(&imu_dev_ctx, 2);

	lsm6dso_fsm_enable_get(&imu_dev_ctx, &fsm_enable);
	fsm_enable.fsm_enable_a.fsm1_en = PROPERTY_ENABLE;
	fsm_enable.fsm_enable_a.fsm2_en = PROPERTY_ENABLE;
	lsm6dso_fsm_enable_set(&imu_dev_ctx, &fsm_enable);

	lsm6dso_fsm_data_rate_set(&imu_dev_ctx, LSM6DSO_ODR_FSM_26Hz);

	fsm_addr = LSM6DSO_START_FSM_ADD;

	lsm6dso_ln_pg_write(&imu_dev_ctx, fsm_addr, (uint8_t*)lsm6so_prg_wrist_tilt,
                      sizeof(lsm6so_prg_wrist_tilt));
	fsm_addr += sizeof(lsm6so_prg_wrist_tilt);

	lsm6dso_ln_pg_write(&imu_dev_ctx, fsm_addr, (uint8_t*)falltrigger,
                      sizeof(falltrigger));

	/* route single tap, wrist tilt to INT2 pin*/
	lsm6dso_pin_int2_route_get(&imu_dev_ctx, &int2_route);
	//int2_route.md2_cfg.int2_single_tap = PROPERTY_ENABLE;
	int2_route.fsm_int2_a.int2_fsm1 = PROPERTY_ENABLE;
	lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_route);

	/* route step counter to INT1 pin*/
	lsm6dso_pin_int1_route_get(&imu_dev_ctx, &int1_route);
	int1_route.emb_func_int1.int1_step_detector = PROPERTY_ENABLE;
	//int1_route.fsm_int1_a.int1_fsm1 = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);

	lsm6dso_timestamp_set(&imu_dev_ctx, 1);

	return true;
}

void sensor_reset(void)
{  
	lsm6dso_reset_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_reset_get(&imu_dev_ctx, &rst);

	lsm6dso_i3c_disable_set(&imu_dev_ctx, LSM6DSO_I3C_DISABLE);

	//max supported frames at 3KB FIFO, 512*6 = 3072. Each frame has 6 axis and each axis is 2 byte in size.
	lsm6dso_fifo_watermark_set(&imu_dev_ctx, ACC_GYRO_FIFO_BUF_LEN);
	lsm6dso_fifo_stop_on_wtm_set(&imu_dev_ctx, PROPERTY_ENABLE);

	lsm6dso_fifo_mode_set(&imu_dev_ctx, LSM6DSO_STREAM_MODE);

	lsm6dso_data_ready_mode_set(&imu_dev_ctx, LSM6DSO_DRDY_PULSED);
/*
	lsm6dso_pin_int1_route_get(&imu_dev_ctx, &int1_route);
	int1_route.int1_ctrl.int1_fifo_th = PROPERTY_ENABLE;
	lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);*/

	lsm6dso_fifo_xl_batch_set(&imu_dev_ctx, LSM6DSO_XL_BATCHED_AT_52Hz);
	lsm6dso_fifo_gy_batch_set(&imu_dev_ctx, LSM6DSO_GY_BATCHED_AT_52Hz);

	lsm6dso_xl_full_scale_set(&imu_dev_ctx, LSM6DSO_2g);
	lsm6dso_gy_full_scale_set(&imu_dev_ctx, LSM6DSO_250dps);
	lsm6dso_block_data_update_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_xl_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_52Hz);
	lsm6dso_gy_data_rate_set(&imu_dev_ctx, LSM6DSO_GY_ODR_52Hz);
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
		acceleration_mg[0] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[0]);
		acceleration_mg[1] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[1]);
		acceleration_mg[2] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[2]);
	}

	*sensor_x = acceleration_mg[0];
	*sensor_y = acceleration_mg[1];
	*sensor_z = acceleration_mg[2];
}

void historic_buffer(void)
{
	uint16_t histBuff_counter = 0;
	uint16_t i = 0;

	while(1)
	{
		uint16_t num = 0;
		uint8_t waterm = 0;
		lsm6dso_fifo_tag_t reg_tag;
		axis3bit16_t dummy;

		lsm6dso_fifo_wtm_flag_get(&imu_dev_ctx, &waterm);
		if(waterm>0)
		{
			lsm6dso_fifo_data_level_get(&imu_dev_ctx, &num);

			while(num--)
			{
				lsm6dso_fifo_sensor_tag_get(&imu_dev_ctx, &reg_tag);
				switch (reg_tag)
				{
				case LSM6DSO_XL_NC_TAG:
					memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
					lsm6dso_fifo_out_raw_get(&imu_dev_ctx, data_raw_acceleration.u8bit);
					acceleration_mg[0] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[0]);
					acceleration_mg[1] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[1]);
					acceleration_mg[2] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[2]);

					acceleration_g[0]   = acceleration_mg[0]/1000;
					acceleration_g[1]   = acceleration_mg[1]/1000;
					acceleration_g[2]   = acceleration_mg[2]/1000;

					acc_x_hist_buffer[i] = acceleration_g[0]; //[i]
					acc_y_hist_buffer[i] = acceleration_g[1];
					acc_z_hist_buffer[i] = acceleration_g[2];   

					histBuff_counter++;
					//LOGD("%d, Axyz, %4.2f, %4.2f, %4.2f",
					//			i, acc_x_hist_buffer[i], acc_y_hist_buffer[i], acc_z_hist_buffer[i]);
					break;

				case LSM6DSO_GYRO_NC_TAG:
					memset(data_raw_angular_rate.u8bit, 0x00, 3*sizeof(int16_t));
					lsm6dso_fifo_out_raw_get(&imu_dev_ctx, data_raw_angular_rate.u8bit);
					angular_rate_mdps[0] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[0]);
					angular_rate_mdps[1] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[1]);
					angular_rate_mdps[2] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[2]);

					angular_rate_dps[0] = angular_rate_mdps[0]/1000;
					angular_rate_dps[1] = angular_rate_mdps[1]/1000;
					angular_rate_dps[2] = angular_rate_mdps[2]/1000;

					gyro_x_hist_buffer[i] = angular_rate_dps[0];
					gyro_y_hist_buffer[i] = angular_rate_dps[1];
					gyro_z_hist_buffer[i] = angular_rate_dps[2];

					histBuff_counter++;
					i++;
					//LOGD("%d, Gxyz, %4.2f, %4.2f, %4.2f",
					//				i, angular_rate_dps[0], angular_rate_dps[1], angular_rate_dps[2]);
					break;

				default:
					memset(dummy.u8bit, 0x00, 3 * sizeof(int16_t));
					lsm6dso_fifo_out_raw_get(&imu_dev_ctx, dummy.u8bit);
					break;
				}         
			}

			//lsm6dso_fifo_mode_set(&imu_dev_ctx, LSM6DSO_BYPASS_MODE); //clear FIFO contents
			//lsm6dso_fifo_mode_set(&imu_dev_ctx, LSM6DSO_BYPASS_TO_FIFO_MODE); //switch to bypass_to_fifo mode

			if(histBuff_counter == PATTERN_LEN)
			{
				//LOGD("HIST BUFFER FLAG ON");
				hist_buff_flag = true;
				break;
			}
		}
		else
		{
			break;
		}
	}
}

void curr_vrif_buffers(void)
{
	uint16_t buff_counter = 0;

	sensor_reset();
	
	while(1)
	{
		uint16_t num = 0;
		uint8_t waterm = 0;
		uint8_t i_rev = ACC_GYRO_FIFO_BUF_LEN-1;
		uint8_t j_rev = ACC_GYRO_FIFO_BUF_LEN-1;
		uint8_t k_rev = ACC_GYRO_FIFO_BUF_LEN-1;

		lsm6dso_fifo_wtm_flag_get(&imu_dev_ctx, &waterm);
		if(waterm>0)
		{
			lsm6dso_fifo_data_level_get(&imu_dev_ctx, &num);
			while(num--)
			{
				memset(data_raw_angular_rate.u8bit, 0x00, 3*sizeof(int16_t));
				lsm6dso_fifo_out_raw_get(&imu_dev_ctx, data_raw_angular_rate.u8bit);
				angular_rate_mdps[0] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[0]);
				angular_rate_mdps[1] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[1]);
				angular_rate_mdps[2] = lsm6dso_from_fs250_to_mdps(data_raw_angular_rate.i16bit[2]);

				angular_rate_dps[0]  = angular_rate_mdps[0]/1000;
				angular_rate_dps[1]  = angular_rate_mdps[1]/1000;
				angular_rate_dps[2]  = angular_rate_mdps[2]/1000;
				gyro_tempX[num]      = angular_rate_dps[0];
				gyro_tempY[num]      = angular_rate_dps[1];
				gyro_tempZ[num]      = angular_rate_dps[2];

				memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
				lsm6dso_fifo_out_raw_get(&imu_dev_ctx, data_raw_acceleration.u8bit);
				acceleration_mg[0] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[0]);
				acceleration_mg[1] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[1]);
				acceleration_mg[2] = lsm6dso_from_fs2_to_mg(data_raw_acceleration.i16bit[2]);

				acceleration_g[0]  = acceleration_mg[0]/1000;
				acceleration_g[1]  = acceleration_mg[1]/1000;
				acceleration_g[2]  = acceleration_mg[2]/1000;
				accel_tempX[num]   = acceleration_g[0];
				accel_tempY[num]   = acceleration_g[1];
				accel_tempZ[num]   = acceleration_g[2];
				buff_counter++;

				if(buff_counter >= ACC_GYRO_FIFO_BUF_LEN && buff_counter < 2*ACC_GYRO_FIFO_BUF_LEN)
				{
					for (uint8_t i = 0; i < ACC_GYRO_FIFO_BUF_LEN; i++)
					{
						acc_x_cur_buffer[i]  = accel_tempX[i_rev];
						acc_y_cur_buffer[i]  = accel_tempY[i_rev];
						acc_z_cur_buffer[i]  = accel_tempZ[i_rev];
						gyro_x_cur_buffer[i] = gyro_tempX[i_rev];
						gyro_y_cur_buffer[i] = gyro_tempY[i_rev];
						gyro_z_cur_buffer[i] = gyro_tempZ[i_rev];
						buff_counter++;
						i_rev--;

						//LOGD("%d, Cur_Axyz, %4.2f, %4.2f, %4.2f",
						//			i, acc_x_cur_buffer[i], acc_y_cur_buffer[i], acc_z_cur_buffer[i]);
					}
				}

				if(buff_counter >= 3*ACC_GYRO_FIFO_BUF_LEN && buff_counter < 4*ACC_GYRO_FIFO_BUF_LEN)
				{
					for (uint8_t j = 0; j < ACC_GYRO_FIFO_BUF_LEN; j++)
					{
						acc_x_vrif_buffer[j]  = accel_tempX[j_rev];
						acc_y_vrif_buffer[j]  = accel_tempY[j_rev];
						acc_z_vrif_buffer[j]  = accel_tempZ[j_rev];
						gyro_x_vrif_buffer[j] = gyro_tempX[j_rev];
						gyro_y_vrif_buffer[j] = gyro_tempY[j_rev];
						gyro_z_vrif_buffer[j] = gyro_tempZ[j_rev];
						buff_counter++;
						j_rev--;

						//LOGD("%d, Veri_Axyz, %4.2f, %4.2f, %4.2f",
						//			j, acc_x_vrif_buffer[j], acc_y_vrif_buffer[j], acc_z_vrif_buffer[j]);
					}
				}

				if(buff_counter >= 5*ACC_GYRO_FIFO_BUF_LEN && buff_counter < 6*ACC_GYRO_FIFO_BUF_LEN)
				{
					for (uint8_t k = 0; k < ACC_GYRO_FIFO_BUF_LEN; k++)
					{
						acc_x_vrif_buffer_1[k]  = accel_tempX[k_rev];
						acc_y_vrif_buffer_1[k]  = accel_tempY[k_rev];
						acc_z_vrif_buffer_1[k]  = accel_tempZ[k_rev];
						gyro_x_vrif_buffer_1[k] = gyro_tempX[k_rev];
						gyro_y_vrif_buffer_1[k] = gyro_tempY[k_rev];
						gyro_z_vrif_buffer_1[k] = gyro_tempZ[k_rev];
						buff_counter++;
						k_rev--;

						//LOGD("%d, Veri_Axyz_1, %4.2f, %4.2f, %4.2f",
						//			k, acc_x_vrif_buffer_1[k], acc_y_vrif_buffer_1[k], acc_z_vrif_buffer_1[k]);
					}
				}
			}

			if(buff_counter == 6*ACC_GYRO_FIFO_BUF_LEN)
			{
				curr_vrif_buff_flag = true;
				return;
			}
		}
	}
}

static float angle_analyse_fifo(void)
{
	volatile float angle_degree=0,avg_index=50;
	volatile float start_avg_accel_x=0,start_avg_accel_y=0,start_avg_accel_z=0;
	volatile float end_avg_accel_x=0, end_avg_accel_y=0, end_avg_accel_z=0;
	volatile float num=0,denom=0;
	volatile double angle=0;
	volatile uint16_t i;

	for(i = 0; i < avg_index; i++ )
	{       
		start_avg_accel_x += acc_x_hist_buffer[i];
		start_avg_accel_y += acc_y_hist_buffer[i];
		start_avg_accel_z += acc_z_hist_buffer[i];
	}
        
	start_avg_accel_x /=avg_index;										//get average for each axis
	start_avg_accel_y /=avg_index;
	start_avg_accel_z /=avg_index;

	start_avg_accel_x = get_acc_magn(start_avg_accel_x);			//get acc magnitude
	start_avg_accel_y = get_acc_magn(start_avg_accel_y);
	start_avg_accel_z = get_acc_magn(start_avg_accel_z);

	for(i = ACC_GYRO_FIFO_BUF_LEN-avg_index; i < ACC_GYRO_FIFO_BUF_LEN; i++)
	{
		end_avg_accel_x += acc_x_cur_buffer[i];
		end_avg_accel_y += acc_y_cur_buffer[i];
		end_avg_accel_z += acc_z_cur_buffer[i];
	}

	end_avg_accel_x /=avg_index;										//get average for each axis
	end_avg_accel_y /=avg_index;
	end_avg_accel_z /=avg_index;

	end_avg_accel_x = get_acc_magn(end_avg_accel_x);			//get acc magnitude
	end_avg_accel_y = get_acc_magn(end_avg_accel_y);
	end_avg_accel_z = get_acc_magn(end_avg_accel_z);

	num= (start_avg_accel_x*end_avg_accel_x) + (start_avg_accel_y*end_avg_accel_y) + (start_avg_accel_z*end_avg_accel_z);
	denom= (pow(start_avg_accel_x,2) + pow(start_avg_accel_y,2) + pow(start_avg_accel_z,2)) * (pow(end_avg_accel_x,2)+pow(end_avg_accel_y,2)+pow(end_avg_accel_z,2));
	angle=acos(num/sqrt(denom));

	angle_degree=angle *(180.0f/3.14159265f);						//get angle in degree

	return angle_degree;

}

static float acceleration_analyse_fifo(void)
{
	volatile uint16_t i;
	volatile float acc_magn_square = 0, max_acc_magn_square = 0;
	/*
	* Compute Accelerometer's Magnitude
	*/
	for(i=0;i<ACC_GYRO_FIFO_BUF_LEN*2;i++)
	{
		//acc magnitude square to avoid sqrt() call to save time.
		if(i<ACC_GYRO_FIFO_BUF_LEN)
		{
			acc_magn_square = pow(get_acc_magn(acc_x_hist_buffer[i]),2)+pow(get_acc_magn(acc_y_hist_buffer[i]),2)+pow(get_acc_magn(acc_z_hist_buffer[i]),2);
		}
		else
		{
			acc_magn_square = pow(get_acc_magn(acc_x_cur_buffer[i-ACC_GYRO_FIFO_BUF_LEN]),2)+pow(get_acc_magn(acc_y_cur_buffer[i-ACC_GYRO_FIFO_BUF_LEN]),2)+pow(get_acc_magn(acc_z_cur_buffer[i-ACC_GYRO_FIFO_BUF_LEN]),2);
		}
		
		if(acc_magn_square > max_acc_magn_square) max_acc_magn_square = acc_magn_square;	//get the maximum acc magnitude square

	}
	return max_acc_magn_square;		//do once sqrt() to get acc magnitude
}


static float gyroscope_analyse_fifo(void)
{
	volatile uint16_t i;
	volatile float gyro_magn_square = 0, max_gyro_magn_square = 0;
	/*
	* Compute Gyroscope's Magnitude for all data after fall
	*/
	for(i=0;i<ACC_GYRO_FIFO_BUF_LEN*2;i++)
	{
		if(i < ACC_GYRO_FIFO_BUF_LEN)
		{
			//gyroscope magnitude square to avoid sqrt() call to save time.
			// gyro_magn_square = pow(get_gyro_magn(gyro_x_cur_buffer[i]),2)+pow(get_gyro_magn(gyro_y_cur_buffer[i]),2)+pow(get_gyro_magn(gyro_z_cur_buffer[i]),2);
			gyro_magn_square = pow(get_gyro_magn(gyro_y_hist_buffer[i]),2)+pow(get_gyro_magn(gyro_z_hist_buffer[i]),2);
		}
		else
		{
			gyro_magn_square = pow(get_gyro_magn(gyro_y_cur_buffer[i-ACC_GYRO_FIFO_BUF_LEN]),2)+pow(get_gyro_magn(gyro_z_cur_buffer[i-ACC_GYRO_FIFO_BUF_LEN]),2);
		}
		
		if(gyro_magn_square > max_gyro_magn_square) max_gyro_magn_square = gyro_magn_square;	//get the maximum gyroscope magnitude square
		
	}
	// return (sqrt(max_gyro_magn_square));		//do once sqrt() to get gyroscope magnitude
	return sqrt(max_gyro_magn_square);		//do once sqrt() to get gyroscope magnitude

}

static float fall_verification_fifo_skip(void)
{
	volatile uint16_t i;
	volatile float std_deviation = 0, variance=0,average = 0;

	memset(verify_acc_magn,0x00,VERIFY_DATA_BUF_LEN);
	//compute acc magnitude
	for(i=0;i<VERIFY_DATA_BUF_LEN;i++)
	{
		verify_acc_magn[i] = sqrt(pow(get_acc_magn(acc_x_vrif_buffer_1[i]),2)+ pow(get_acc_magn(acc_y_vrif_buffer_1[i]),2) + pow(get_acc_magn(acc_z_vrif_buffer_1[i]),2));
		average += verify_acc_magn[i];
	}

	average /= (float)VERIFY_DATA_BUF_LEN;

	//compute variance and standard deviation to a base 0.5g
	for(i=0;i<VERIFY_DATA_BUF_LEN;i++)
	{
		variance += pow((verify_acc_magn[i]-average),2);
	}
	std_deviation = sqrt(variance/(float)(VERIFY_DATA_BUF_LEN));

	return std_deviation;
}

/**@brief Function for getting input degree according to angle or gyroscope magnitude.
 *
 * @return input degree in float.
 */
static float get_input_degree(float x, float a, float b, float c, float d)
{
	volatile float re_val=0;

	if(d == b)                  // Rshoulder
	{
		if(x >= b)                     re_val = 1;
		else if(x > a && x < b)        re_val = (x - a) / (b - a);
		else if(x <= a)                re_val = 0;
	}
	else if(d == c)             // Triangle
	{
		if(x <= a)                     re_val = 0;
		else if(x == b)                re_val = 1;
		else if(x < b)                 re_val = (x - a) / (b - a);
		else if(x >= c)                re_val = 0;
		else if(x > b)                 re_val = (c - x) / (c - b);
	}
	else if(d == a)             //Lshoulder
	{
		if(x <= c)                     re_val = 1;
		else if(x > c && x < d)        re_val = (d - x) / (d - c);
		else if(x >= d)                re_val = 0;
	}
	else re_val = 0;
	
	return re_val;
}

/**@brief Function for getting weight according to memship.
 *
 * @return weight in uint8_t.
 */
static uint8_t get_output_from_memship(uint8_t memship)
{
	if(memship == LOW_MS)    
		return WEIGHT_VALUE_10;
	else if(memship == MEDIUM_MS) 
		return WEIGHT_VALUE_30;
	else if(memship == HIGH_MS)   
		return WEIGHT_VALUE_50;
	else 
		return 0;
}

/**@brief Function for getting fuzzy output.
 *
 * @return fuzzy analysis output in float.
 */
static float fuzzy_analyse(float angle, float max_gyro_magn)
{
	volatile uint8_t i=0;
	volatile float fire_strength[9];
	volatile float sum_firestrenths = 0;
	volatile float output_value=0;

	volatile float low_angle_degree=0, medium_angle_degree=0, high_angle_degree=0;
	volatile float low_gyro_magnitude_degree=0, medium_gyro_magnitude_degree=0, high_gyro_magnitude_degree=0;
	volatile float current_angle = angle;  // should compute this value continously from sensor
	volatile float current_max_gyro_magn = max_gyro_magn;

	//===================================================1-Fuzzification
	low_angle_degree =    get_input_degree(current_angle, MEDIUM_ANGLE, 0, LOW_ANGLE, MEDIUM_ANGLE);
	medium_angle_degree = get_input_degree(current_angle, LOW_ANGLE, MEDIUM_ANGLE, HIGH_ANGLE, HIGH_ANGLE);
	high_angle_degree =   get_input_degree(current_angle, MEDIUM_ANGLE, HIGH_ANGLE, 0, HIGH_ANGLE);

	low_gyro_magnitude_degree =    get_input_degree(current_max_gyro_magn, MEDIUM_GYRO_MAGNITUDE, 0, LOW_GYRO_MAGNITUDE, MEDIUM_GYRO_MAGNITUDE);
	medium_gyro_magnitude_degree = get_input_degree(current_max_gyro_magn, LOW_GYRO_MAGNITUDE, MEDIUM_GYRO_MAGNITUDE, HIGH_GYRO_MAGNITUDE, HIGH_GYRO_MAGNITUDE);
	high_gyro_magnitude_degree =   get_input_degree(current_max_gyro_magn, MEDIUM_GYRO_MAGNITUDE, HIGH_GYRO_MAGNITUDE, 0, HIGH_GYRO_MAGNITUDE);

	//===================================================2-Fire Strength
	fire_strength[0] = min(low_angle_degree   , low_gyro_magnitude_degree);
	fire_strength[1] = min(low_angle_degree   , medium_gyro_magnitude_degree);
	fire_strength[2] = min(low_angle_degree   , high_gyro_magnitude_degree);
	fire_strength[3] = min(medium_angle_degree, low_gyro_magnitude_degree);
	fire_strength[4] = min(medium_angle_degree, medium_gyro_magnitude_degree);
	fire_strength[5] = min(medium_angle_degree, high_gyro_magnitude_degree);
	fire_strength[6] = min(high_angle_degree  , low_gyro_magnitude_degree);
	fire_strength[7] = min(high_angle_degree  , medium_gyro_magnitude_degree);
	fire_strength[8] = min(high_angle_degree  , high_gyro_magnitude_degree);

	//======================================================3- linguistic and numric output
	for (i = 0; i < 9; i++)
	{
		output_value += fire_strength[i] * get_output_from_memship(suspicion_rules[i][2]);
		sum_firestrenths += fire_strength[i];
	}

	output_value /= sum_firestrenths;
	return output_value;
}

/*@brief Fall detection analyse
*
* @return If fall return 1, otherwise return 0
*/
void fall_detection(void)
{
	historic_buffer();
	LOGD("fall detecting 1");

	if(hist_buff_flag)
	{
		curr_vrif_buffers();
		hist_buff_flag = false;
		LOGD("fall detecting 2");
	}

	if(curr_vrif_buff_flag)
	{       
		acc_magn_square = acceleration_analyse_fifo();
		cur_angle = angle_analyse_fifo();
		cur_max_gyro_magn = gyroscope_analyse_fifo();
		cur_fuzzy_output = fuzzy_analyse(cur_angle, cur_max_gyro_magn);
		std_devi = fall_verification_fifo_skip();

		/*uncomment to display info on screen: 
		*  
		LCD_Clear(0);
		sprintf(tmpbuf, "cur_angle is: %f", cur_angle);
		LCD_ShowString(20,80,tmpbuf); 
		sprintf(tmpbuf, "cur_max_gyro_magn: %f", cur_max_gyro_magn);
		LCD_ShowString(20,100,tmpbuf);
		sprintf(tmpbuf, "STD is: %f", std_devi);
		LCD_ShowString(20,120,tmpbuf);
		sprintf(tmpbuf, "Fuzzy Output is: %f", cur_fuzzy_output);
		LCD_ShowString(20,140,tmpbuf);
		*/

		if(cur_fuzzy_output > FUZZY_OUT_THRES_DEF)
		{
			if(std_devi < STD_VARIANCE_THRES_DEF)
			{
				fall_result = true;
				//break;
			}
			else
			{
				fall_result = false; //std not satisfied
			}
		}
		else
		{
			fall_result = false; //fuzzy output not satisfied
		}
		
		curr_vrif_buff_flag = false;
		sensor_init(); //resets the algorithm, will work continuosly on every tap
		LOGD("fall detecting 3");
	} 
}

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
	
	StepCheckSendLocationData(g_steps);
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
*/
void lsm6dso_sensitivity(void)
{
	//Set the debounce steps
	uint8_t deb_step = 5;
	lsm6dso_pedo_debounce_steps_set(&imu_dev_ctx, &deb_step);

	//Set the sensitivity of the sensor
	uint8_t delay_time[2] = {0x00U, 0x32U};
	//Lower Limit is 0 and Upper Limit is 50(32 in Hex), the delay time is 320ms
	lsm6dso_pedo_steps_period_set(&imu_dev_ctx, &delay_time);
}

static void mt_fall_detection(struct k_work *work)
{
	if(int1_event)	//steps or tilt
	{
		LOGD("int1 evt!");
		int1_event = false;

		if(!imu_check_ok)
			return;
		
	#ifdef CONFIG_PPG_SUPPORT
		if(PPGIsWorking())
			return;
	#endif

		if(!is_wearing())
			return;
		
		is_tilt();
		if(wrist_tilt)
		{
			LOGD("tilt trigger!");
			
			wrist_tilt = false;

			if(lcd_is_sleeping && global_settings.wake_screen_by_wrist)
			{
				sleep_out_by_wrist = true;
				lcd_sleep_out = true;
			}
		}
		else
		{
			LOGD("steps trigger!");
			
			UpdateIMUData();
			imu_redraw_steps_flag = true;	
		}
	}

#if 0	//xb add 2021-11-29 屏蔽了摔倒检测功能
	if(int2_event) //fall
	{
		LOGD("int2 evt!");
		
		int2_event = false;

		if(!imu_check_ok)
			return;

	#ifdef CONFIG_PPG_SUPPORT
		if(PPGIsWorking())
			return;
	#endif

		if(!is_wearing())
			return;

		fall_detection();
		if(fall_result)
		{
			LOGD("Fall trigger!");
			
			fall_result = false;
			lcd_sleep_out = true;
			
			FallAlarmStart();
		}
        else
        {
			LOGD("Not Fall.");
        }
	}
#endif

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

	if(update_sleep_parameter)
	{
		update_sleep_parameter = false;

		if(!imu_check_ok)
			return;

	#ifdef CONFIG_PPG_SUPPORT
		if(PPGIsWorking())
			return;
	#endif

		UpdateSleepPara();
	}
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
	k_work_init(&imu_work, mt_fall_detection);
	
	if(init_i2c() != 0)
		return;
	
	init_gpio();

	imu_dev_ctx.write_reg = platform_write;
	imu_dev_ctx.read_reg = platform_read;
	imu_dev_ctx.handle = i2c_imu;

	imu_check_ok = sensor_init();
	if(!imu_check_ok)
		return;
	
	lsm6dso_steps_reset(&imu_dev_ctx); //reset step counter
	lsm6dso_sensitivity();
	StartSleepTimeMonitor();

	LOGD("IMU_init done!");
}

/*@brief Check if a wrist tilt happend
*
* @return If tilt detected, wrist_tilt=true, otherwise false
*/
void is_tilt(void)
{
	lsm6dso_all_sources_t status;

	lsm6dso_all_sources_get(&imu_dev_ctx, &status);
	if(status.fsm_status_a.is_fsm1)
	{ 
		//tilt detected
		wrist_tilt = true;
	}
}

/*@brief disable the wrist tilt detection
*
*/
void disable_tilt_detection(void)
{
	lsm6dso_all_sources_t status;
	
	lsm6dso_fsm_enable_get(&imu_dev_ctx, &fsm_enable);
	fsm_enable.fsm_enable_a.fsm1_en = PROPERTY_DISABLE;
	lsm6dso_fsm_enable_set(&imu_dev_ctx, &fsm_enable);
}

/*@brief enable the wrist tilt detection
*
*/
void enable_tilt_detection(void)
{
	lsm6dso_all_sources_t status;
	
	lsm6dso_fsm_enable_get(&imu_dev_ctx, &fsm_enable);
	fsm_enable.fsm_enable_a.fsm1_en = PROPERTY_ENABLE;
	lsm6dso_fsm_enable_set(&imu_dev_ctx, &fsm_enable);
}

void test_i2c(void)
{
	struct device *i2c_dev;
	struct device *dev0;

	dev0 = device_get_binding("GPIO_0");
	gpio_pin_configure(dev0, 0, GPIO_OUTPUT);
	gpio_pin_set(dev0, 0, 1);

	LOGD("Starting i2c scanner...");

	i2c_dev = DEVICE_DT_GET(IMU_DEV);
	if(!i2c_dev)
	{
		LOGD("I2C: Device driver not found.");
		return;
	}
	i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD));
	uint8_t error = 0u;

	LOGD("Value of NRF_TWIM1_NS->PSEL.SCL: %ld",NRF_TWIM1_NS->PSEL.SCL);
	LOGD("Value of NRF_TWIM1_NS->PSEL.SDA: %ld",NRF_TWIM1_NS->PSEL.SDA);
	LOGD("Value of NRF_TWIM1_NS->FREQUENCY: %ld",NRF_TWIM1_NS->FREQUENCY);
	LOGD("26738688 -> 100k");
	LOGD("67108864 -> 250k");
	LOGD("104857600 -> 400k");

	for (uint8_t i = 0; i < 0x7f; i++)
	{
		struct i2c_msg msgs[1];
		uint8_t dst = 1;

		/* Send the address to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 1U;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		error = i2c_transfer(i2c_dev, &msgs[0], 1, i);
		if(error == 0)
		{
			LOGD("0x%2x device address found on I2C Bus", i);
		}
		else
		{
			//LOGD("error %d", error);
		}
	}
}

void IMURedrawSteps(void)
{
	if(screen_id == SCREEN_ID_STEPS 
		|| screen_id == SCREEN_ID_DISTANCE 
		|| screen_id == SCREEN_ID_CALORIE
		)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void IMUMsgProcess(void)
{
#ifdef CONFIG_FOTA_DOWNLOAD
	if(fota_is_running())
		return;
#endif

	if(int1_event)	//steps
	{
		LOGD("int1 evt!");
		int1_event = false;

		if(!imu_check_ok)
			return;
		
	#ifdef CONFIG_PPG_SUPPORT
		if(PPGIsWorking())
			return;
	#endif

		if(!is_wearing())
			return;
		
		LOGD("steps trigger!");
		
		UpdateIMUData();
		imu_redraw_steps_flag = true;	
	}
		
	if(int2_event) //tilt
	{
		LOGD("int2 evt!");
		
		int2_event = false;

		if(!imu_check_ok)
			return;

	#ifdef CONFIG_PPG_SUPPORT
		if(PPGIsWorking())
			return;
	#endif

		if(!is_wearing())
			return;

		is_tilt();
		if(wrist_tilt)
		{
			LOGD("tilt trigger!");
			
			wrist_tilt = false;

			if(lcd_is_sleeping && global_settings.wake_screen_by_wrist)
			{
				sleep_out_by_wrist = true;
				lcd_sleep_out = true;
			}
		}
	}
	
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

	if(update_sleep_parameter)
	{
		update_sleep_parameter = false;

		if(!imu_check_ok)
			return;

	#ifdef CONFIG_PPG_SUPPORT
		if(PPGIsWorking())
			return;
	#endif

		UpdateSleepPara();
	}
}
#endif/*CONFIG_IMU_SUPPORT*/
