/*
Last Update: 9/03/2022 by Zabdiel
ULTRA LOW POWER AND INACTIVITY MODE
-Functions: wrist tilt detection, step counter
-Fall detection disabled
-26Hz data rate
*/
#ifdef CONFIG_IMU_SUPPORT

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <nrf_socket.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lsm6dso.h"
#include "lsm6dso_reg.h"
#include "algorithm.h"
#include "lcd.h"
//#include "gps.h"
#include "settings.h"
#include "screen.h"
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif
#include "logger.h"

//#define IMU_DEBUG

#define I2C1_NODE DT_NODELABEL(i2c1)
#if DT_NODE_HAS_STATUS(I2C1_NODE, okay)
#define IMU_DEV	DT_LABEL(I2C1_NODE)
#else
/* A build error here means your board does not have I2C enabled. */
#error "i2c1 devicetree node is disabled"
#define IMU_DEV	""
#endif

#define IMU_PORT "GPIO_0"

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
static u8_t whoamI, rst;
static struct device *i2c_imu;
static struct device *gpio_imu;
static struct gpio_callback gpio_cb1,gpio_cb2;

bool reset_steps = false;
bool imu_redraw_steps_flag = true;

u16_t g_last_steps = 0;
u16_t g_steps = 0;
u16_t g_calorie = 0;
u16_t g_distance = 0;

sport_record_t last_sport = {0};

extern bool update_sleep_parameter;

#ifdef XB_TEST
#define SCL_PIN		12
#define SDA_PIN		11

void I2C_INIT(void)
{
	if(gpio_imu == NULL)
		gpio_imu = device_get_binding(IMU_PORT);

	gpio_pin_configure(gpio_imu, SCL_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_imu, SDA_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_imu, SCL_PIN, 1);
	gpio_pin_write(gpio_imu, SDA_PIN, 1);
}

void I2C_SDA_OUT(void)
{
	gpio_pin_configure(gpio_imu, SDA_PIN, GPIO_DIR_OUT);
}

void I2C_SDA_IN(void)
{
	gpio_pin_configure(gpio_imu, SDA_PIN, GPIO_DIR_IN);
}

void I2C_SDA_H(void)
{
	gpio_pin_write(gpio_imu, SDA_PIN, 1);
}

void I2C_SDA_L(void)
{
	gpio_pin_write(gpio_imu, SDA_PIN, 0);
}

void I2C_SCL_H(void)
{
	gpio_pin_write(gpio_imu, SCL_PIN, 1);
}

void I2C_SCL_L(void)
{
	gpio_pin_write(gpio_imu, SCL_PIN, 0);
}

void Delay_ms(unsigned int dly)
{
	k_sleep(dly);
}

void Delay_us(unsigned int dly)
{
	k_usleep(dly);
}

//������ʼ�ź�
void I2C_Start(void)
{
	I2C_SDA_OUT();

	I2C_SDA_H();
	I2C_SCL_H();
	//Delay_us(10);
	I2C_SDA_L();
	//Delay_us(10);
	I2C_SCL_L();
}

//����ֹͣ�ź�
void I2C_Stop(void)
{
	I2C_SDA_OUT();

	I2C_SCL_L();
	I2C_SDA_L();
	I2C_SCL_H();
	//Delay_us(10);
	I2C_SDA_H();
	//Delay_us(10);
}

//��������Ӧ���ź�ACK
void I2C_Ack(void)
{
	I2C_SDA_OUT();
	I2C_SCL_L();
	I2C_SDA_OUT();
	I2C_SDA_L();
	//Delay_us(10);
	I2C_SCL_H();
	//Delay_us(10);
	I2C_SCL_L();
}

//����������Ӧ���ź�NACK
void I2C_NAck(void)
{
	I2C_SDA_OUT();
	I2C_SCL_L();

	I2C_SDA_H();
	I2C_SCL_H();
	//Delay_us(10);
	I2C_SCL_L();
}

//�ȴ��ӻ�Ӧ���ź�
//����ֵ��1 ����Ӧ��ʧ��
//		  0 ����Ӧ��ɹ�
u8_t I2C_Wait_Ack(void)
{
	u8_t val,tempTime=0;

	I2C_SDA_IN();

	//I2C_SDA_H();
	I2C_SCL_H();

	while(1)
	{
		gpio_pin_read(gpio_imu, SDA_PIN, &val);
		if(val == 0)
			break;
		
		tempTime++;
		if(tempTime>10)
		{
			I2C_Stop();
			return 1;
		}	 
	}

	I2C_SCL_L();
	return 0;
}

//I2C ����һ���ֽ�
u8_t I2C_Write_Byte(u8_t txd)
{
	u8_t i=0;

	I2C_SDA_OUT();
	I2C_SCL_L();//����ʱ�ӿ�ʼ���ݴ���

	for(i=0;i<8;i++)
	{
		if((txd&0x80)>0) //0x80  1000 0000
			I2C_SDA_H();
		else
			I2C_SDA_L();

		txd<<=1;
		I2C_SCL_H();
		I2C_SCL_L();
	}

	return I2C_Wait_Ack();
}

//I2C ��ȡһ���ֽ�
void I2C_Read_Byte(bool ack, u8_t *data)
{
   u8_t i=0,receive=0,val=0;

   I2C_SDA_IN();
   for(i=0;i<8;i++)
   {
   		I2C_SCL_L();
		I2C_SCL_H();

		receive<<=1;
		gpio_pin_read(gpio_imu, SDA_PIN, &val);
		if(val == 1)
		   receive++;
   }

   	if(ack == 0)
	   	I2C_NAck();
	else
		I2C_Ack();

	*data = receive;
}

u8_t I2C_write_data(u8_t addr, u8_t *databuf, u16_t len)
{
	u8_t i;

	addr = (addr<<1);

	I2C_Start();
	if(I2C_Write_Byte(addr))
		goto err;

	for(i=0;i<len;i++)
	{
		if(I2C_Write_Byte(databuf[i]))
			goto err;
	}

	I2C_Stop();
	return 0;
	
err:
	return -1;
}

u8_t I2C_read_data(u8_t addr, u8_t *databuf, u16_t len)
{
	u8_t i;

	addr = (addr<<1)|1;

	I2C_Start();
	if(I2C_Write_Byte(addr))
		goto err;

	for(i=0;i<len;i++)
	{
		I2C_Read_Byte(false, &databuf[i]);
	}
	I2C_Stop();
	return 0;
	
err:
	return -1;
}
#endif

static uint8_t init_i2c(void)
{
#ifdef XB_TEST
	I2C_INIT();
#else
	i2c_imu = device_get_binding(IMU_DEV);
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
#endif	
}

static int32_t platform_write(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;
	uint8_t data[len+1];

	data[0] = reg;
	memcpy(&data[1], bufp, len);
#ifdef XB_TEST
	rslt = I2C_write_data(LSM6DSO_I2C_ADD, data, len+1);
#else
	rslt = i2c_write(i2c_imu, data, len+1, LSM6DSO_I2C_ADD);
#endif
	return rslt;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t* bufp, uint16_t len)
{
	uint32_t rslt = 0;

#ifdef XB_TEST
	rslt = I2C_write_data(LSM6DSO_I2C_ADD, &reg, 1);
	if(rslt == 0)
	{
		rslt = I2C_read_data(LSM6DSO_I2C_ADD, bufp, len);
	}
#else
	rslt = i2c_write(i2c_imu, &reg, 1, LSM6DSO_I2C_ADD);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_imu, bufp, len, LSM6DSO_I2C_ADD);
	}
#endif
	return rslt;
}

void interrupt_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	int2_event = true;
}

void step_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	int1_event = true;
}

void init_imu_int1(void)
{
	if(gpio_imu == NULL)
		gpio_imu = device_get_binding(IMU_PORT);
	gpio_pin_configure(gpio_imu, LSM6DSO_INT1_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_imu, LSM6DSO_INT1_PIN, 0);
}

uint8_t init_gpio(void)
{
	int flag = GPIO_INPUT|GPIO_INT_ENABLE|GPIO_INT_EDGE|GPIO_PULL_DOWN|GPIO_INT_HIGH_1|GPIO_INT_DEBOUNCE;

	gpio_imu = device_get_binding(IMU_PORT);
	//steps interrupt
	gpio_pin_configure(gpio_imu, LSM6DSO_INT1_PIN, flag);
	//gpio_pin_disable_callback(gpio_imu, LSM6DSO_INT1_PIN); //this API no longer supported in zephyr version 2.7.0
        gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT1_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb1, step_event, BIT(LSM6DSO_INT1_PIN));
	gpio_add_callback(gpio_imu, &gpio_cb1);
	//gpio_pin_enable_callback(gpio_imu, LSM6DSO_INT1_PIN); //this API no longer supported in zephyr version 2.7.0
        gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT1_PIN, GPIO_INT_ENABLE);

	//tilt interrupt
	gpio_pin_configure(gpio_imu, LSM6DSO_INT2_PIN, flag);
	//gpio_pin_disable_callback(gpio_imu, LSM6DSO_INT2_PIN); //this API no longer supported in zephyr version 2.7.0
        gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT2_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb2, interrupt_event, BIT(LSM6DSO_INT2_PIN));
	gpio_add_callback(gpio_imu, &gpio_cb2);
	//gpio_pin_enable_callback(gpio_imu, LSM6DSO_INT2_PIN); //this API no longer supported in zephyr version 2.7.0
        gpio_pin_interrupt_configure(gpio_imu, LSM6DSO_INT2_PIN, GPIO_INT_ENABLE);

	return 0;
}

/* ULTRA LOW POWER & INACTIVITY MODE IMPLEMENTED. FIFO DISABLED*/
static bool sensor_init(void){

  lsm6dso_device_id_get(&imu_dev_ctx, &whoamI);
  if(whoamI != LSM6DSO_ID)
		return false;
  
  lsm6dso_reset_set(&imu_dev_ctx, PROPERTY_ENABLE);
  lsm6dso_reset_get(&imu_dev_ctx, &rst);

  lsm6dso_i3c_disable_set(&imu_dev_ctx, LSM6DSO_I3C_DISABLE);
  lsm6dso_fifo_mode_set(&imu_dev_ctx, LSM6DSO_BYPASS_MODE);    
  
  lsm6dso_xl_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_26Hz);
  lsm6dso_gy_data_rate_set(&imu_dev_ctx, LSM6DSO_GY_ODR_OFF);
  lsm6dso_xl_full_scale_set(&imu_dev_ctx, LSM6DSO_2g);

  // ULTRA LOW POWER & INACTIVITY MODE
  lsm6dso_xl_power_mode_set(&imu_dev_ctx, LSM6DSO_ULTRA_LOW_POWER_MD);
  /* Set duration for Activity detection to 38 ms (= 1 * 1 / ODR_XL) */
  lsm6dso_wkup_dur_set(&imu_dev_ctx, 0x01);
  /* Set duration for Inactivity detection to 19.69 s (= 1 * 512 / ODR_XL) */
  lsm6dso_act_sleep_dur_set(&imu_dev_ctx, 0x01);
  /* Set Activity/Inactivity threshold to 312.5 mg */
  lsm6dso_wkup_threshold_set(&imu_dev_ctx, 0x05);
  /* Inactivity configuration: XL to 12.5 in LP, gyro to Power-Down */
  lsm6dso_act_mode_set(&imu_dev_ctx, LSM6DSO_XL_12Hz5_GY_PD);
  /* Enable interrupt generation on Inactivity INT1 pin */
  lsm6dso_pin_int1_route_get(&imu_dev_ctx, &int1_route);
  int1_route.md1_cfg.int1_sleep_change = PROPERTY_ENABLE; 
  lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);

  /*Step Counter enable*/
  lsm6dso_pin_int1_route_get(&imu_dev_ctx, &int1_route);
  int1_route.emb_func_int1.int1_step_detector = PROPERTY_ENABLE;
  lsm6dso_pin_int1_route_set(&imu_dev_ctx, &int1_route);

  /* Enable False Positive Rejection. */
  lsm6dso_pedo_sens_set(&imu_dev_ctx, LSM6DSO_FALSE_STEP_REJ); 
  lsm6dso_steps_reset(&imu_dev_ctx);

  /* Tilt enable */
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
  /* wrist tilt to INT2 pin*/
  lsm6dso_pin_int2_route_get(&imu_dev_ctx, &int2_route);
  int2_route.fsm_int2_a.int2_fsm1 = PROPERTY_ENABLE;
  lsm6dso_pin_int2_route_set(&imu_dev_ctx, &int2_route);
  
  return true;
}


/*@brief Get real time X/Y/Z reading in mg
*
*/
void get_sensor_reading(float *sensor_x, float *sensor_y, float *sensor_z)
{
	u8_t reg;

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


void ReSetImuSteps(void)
{
	lsm6dso_steps_reset(&imu_dev_ctx);

	g_last_steps = 0;
	g_steps = 0;
	g_distance = 0;
	g_calorie = 0;
	
	last_sport.timestamp.year = date_time.year;
	last_sport.timestamp.month = date_time.month; 
	last_sport.timestamp.day = date_time.day;
	last_sport.timestamp.hour = date_time.hour;
	last_sport.timestamp.minute = date_time.minute;
	last_sport.timestamp.second = date_time.second;
	last_sport.timestamp.week = date_time.week;
	last_sport.steps = g_steps;
	last_sport.distance = g_distance;
	last_sport.calorie = g_calorie;
	save_cur_sport_to_record(&last_sport);	
}

void GetImuSteps(u16_t *steps)
{
	lsm6dso_number_of_steps_get(&imu_dev_ctx, steps);
}

void UpdateIMUData(void)
{
	u16_t steps;
	
	GetImuSteps(&steps);

	g_steps = steps+g_last_steps;
	g_distance = 0.7*g_steps;
	g_calorie = (0.8214*60*g_distance)/1000;

#ifdef IMU_DEBUG
	LOGD("g_steps:%d,g_distance:%d,g_calorie:%d", g_steps, g_distance, g_calorie);
#endif

	last_sport.timestamp.year = date_time.year;
	last_sport.timestamp.month = date_time.month; 
	last_sport.timestamp.day = date_time.day;
	last_sport.timestamp.hour = date_time.hour;
	last_sport.timestamp.minute = date_time.minute;
	last_sport.timestamp.second = date_time.second;
	last_sport.timestamp.week = date_time.week;
	last_sport.steps = g_steps;
	last_sport.distance = g_distance;
	last_sport.calorie = g_calorie;
	save_cur_sport_to_record(&last_sport);
	
	//StepCheckSendLocationData(g_steps);
}

void GetSportData(u16_t *steps, u16_t *calorie, u16_t *distance)
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
	uint8_t deb_step = 15;
	lsm6dso_pedo_debounce_steps_set(&imu_dev_ctx, &deb_step);

	//Set the sensitivity of the sensor
	uint8_t delay_time[2] = {0x00U, 0x32U};
	//Lower Limit is 0 and Upper Limit is 50(32 in Hex), the delay time is 320ms
	lsm6dso_pedo_steps_period_set(&imu_dev_ctx, &delay_time);
}


/*	NOT USED

static void mt_fall_detection(struct k_work *work)
{
	if(int1_event)	//steps or tilt
	{
	#ifdef IMU_DEBUG
		LOGD("int1 evt!");
	#endif
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
		#ifdef IMU_DEBUG
			LOGD("tilt trigger!");
		#endif
		
			wrist_tilt = false;

			if(lcd_is_sleeping && global_settings.wake_screen_by_wrist)
			{
				sleep_out_by_wrist = true;
				lcd_sleep_out = true;
			}
		}
		else
		{
		#ifdef IMU_DEBUG
			LOGD("steps trigger!");
		#endif
			
			UpdateIMUData();
			imu_redraw_steps_flag = true;	
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
*/


void IMU_init(struct k_work_q *work_q)
{
#ifdef IMU_DEBUG
	LOGD("IMU_init");
#endif

	get_cur_sport_from_record(&last_sport);
#ifdef IMU_DEBUG
	LOGD("%04d/%02d/%02d last_steps:%d", last_sport.timestamp.year,last_sport.timestamp.month,last_sport.timestamp.day,last_sport.steps);
#endif
	if(last_sport.timestamp.day == date_time.day)
	{
		g_last_steps = last_sport.steps;
		g_steps = last_sport.steps;
		g_distance = last_sport.distance;
		g_calorie = last_sport.calorie;
	}

	imu_work_q = work_q;
	//k_work_init(&imu_work, mt_fall_detection);		NOT USED
	
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
#ifdef IMU_DEBUG
	LOGD("IMU_init done!");
#endif
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
	gpio_pin_write(dev0, 0, 1);
#ifdef IMU_DEBUG
	LOGD("Starting i2c scanner...");
#endif
	i2c_dev = device_get_binding(IMU_DEV);
	if(!i2c_dev)
	{
	#ifdef IMU_DEBUG
		LOGD("I2C: Device driver not found.");
	#endif
		return;
	}
	i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD));
	uint8_t error = 0u;
#ifdef IMU_DEBUG
	LOGD("Value of NRF_TWIM1_NS->PSEL.SCL: %ld",NRF_TWIM1_NS->PSEL.SCL);
	LOGD("Value of NRF_TWIM1_NS->PSEL.SDA: %ld",NRF_TWIM1_NS->PSEL.SDA);
	LOGD("Value of NRF_TWIM1_NS->FREQUENCY: %ld",NRF_TWIM1_NS->FREQUENCY);
	LOGD("26738688 -> 100k");
	LOGD("67108864 -> 250k");
	LOGD("104857600 -> 400k");
#endif
	for (u8_t i = 0; i < 0x7f; i++)
	{
		struct i2c_msg msgs[1];
		u8_t dst = 1;

		/* Send the address to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 1U;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		error = i2c_transfer(i2c_dev, &msgs[0], 1, i);
		if(error == 0)
		{
		#ifdef IMU_DEBUG
			LOGD("0x%2x device address found on I2C Bus", i);
		#endif
		}
		else
		{
		#ifdef IMU_DEBUG
			//LOGD("error %d", error);
		#endif
		}
	}
}

void IMURedrawSteps(void)
{
	if(screen_id == SCREEN_ID_STEPS)
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
	#ifdef IMU_DEBUG
		LOGD("int1 evt!");
	#endif
		int1_event = false;

		if(!imu_check_ok)
			return;
		
	#ifdef CONFIG_PPG_SUPPORT
		if(PPGIsWorking())
			return;
	#endif

		if(!is_wearing())
			return;

	#ifdef IMU_DEBUG	
		LOGD("steps trigger!");
	#endif
		UpdateIMUData();
		imu_redraw_steps_flag = true;	
	}
		
	if(int2_event) //tilt
	{
	#ifdef IMU_DEBUG
		LOGD("int2 evt!");
	#endif
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
		#ifdef IMU_DEBUG
			LOGD("tilt trigger!");
		#endif
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