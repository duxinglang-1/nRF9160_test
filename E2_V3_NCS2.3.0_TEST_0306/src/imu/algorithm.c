#include <string.h>
//#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#include <math.h>
//#include <stdbool.h>
#include "algorithm.h"
#include "logger.h"

//#define IMU_DEBUG

//static int32_t check_ok = 0;

volatile bool hist_buff_flag      = false;
volatile bool curr_vrif_buff_flag = false;
volatile bool fall_result = false;
static axis3bit16_t data_raw_acceleration;
static axis3bit16_t data_raw_angular_rate;
static float acceleration_mg[3];
static float angular_rate_mdps[3];
static float acceleration_g[3];
static float angular_rate_dps[3];
static uint8_t rst;
float verify_acc_magn[VERIFY_DATA_BUF_LEN*2] = {0}; // 1
float std_devi=0;
float acc_magn_square=0, cur_angle=0, cur_max_gyro_magn=0, cur_fuzzy_output=0;
//char tmp_buf[128] = {0};
//uint8_t twi_tx_buffer[TWI_BUFFER_SIZE];

//char check_log[256] = {0};

float acc_x_hist_buffer[PATTERN_LEN]  = {0}, acc_y_hist_buffer[PATTERN_LEN]   = {0}, acc_z_hist_buffer[PATTERN_LEN]   = {0};
float gyro_x_hist_buffer[PATTERN_LEN] = {0}, gyro_y_hist_buffer[PATTERN_LEN]  = {0}, gyro_z_hist_buffer[PATTERN_LEN]  = {0};

//float accel_tempX[ACC_GYRO_FIFO_BUF_LEN] = {0}, accel_tempY[ACC_GYRO_FIFO_BUF_LEN] = {0}, accel_tempZ[ACC_GYRO_FIFO_BUF_LEN] = {0}; // 1
//float gyro_tempX[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_tempY[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_tempZ[ACC_GYRO_FIFO_BUF_LEN]  = {0}; // 1

float acc_x_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, acc_y_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0}, acc_z_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0};
//float gyro_x_cur_buffer[ACC_GYRO_FIFO_BUF_LEN] = {0}, gyro_y_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}; // 1
float gyro_y_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0};

//float acc_x_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, acc_y_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0}, acc_z_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0}; // 1
//float gyro_x_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN] = {0}, gyro_y_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}; // 1

float acc_x_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]  = {0}, acc_y_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]   = {0}, acc_z_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]   = {0};
//float gyro_x_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN] = {0}, gyro_y_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]  = {0}; // 1

volatile uint8_t suspicion_rules[9][3] =
{
  LOW_MS     , LOW_MS     , LOW_MS    ,
  LOW_MS     , MEDIUM_MS  , LOW_MS    ,
  LOW_MS     , HIGH_MS    , LOW_MS    ,
  MEDIUM_MS  , LOW_MS     , LOW_MS    ,
  MEDIUM_MS  , MEDIUM_MS  , MEDIUM_MS ,
  MEDIUM_MS  , HIGH_MS    , HIGH_MS   ,
  HIGH_MS    , LOW_MS     , LOW_MS    ,
  HIGH_MS    , MEDIUM_MS  , MEDIUM_MS , // MEDIUM_MS  , MEDIUM_MS ,
  HIGH_MS    , HIGH_MS    , HIGH_MS
};

static uint8_t CRC8Table[256] = 
{
 0x00,0x07,0x0e,0x09,0x1c,0x1b,0x12,0x15,0x38,0x3f,0x36,0x31,0x24,0x23,0x2a,0x2d,
 0x70,0x77,0x7e,0x79,0x6c,0x6b,0x62,0x65,0x48,0x4f,0x46,0x41,0x54,0x53,0x5a,0x5d,
 0xe0,0xe7,0xee,0xe9,0xfc,0xfb,0xf2,0xf5,0xd8,0xdf,0xd6,0xd1,0xc4,0xc3,0xca,0xcd,
 0x90,0x97,0x9e,0x99,0x8c,0x8b,0x82,0x85,0xa8,0xaf,0xa6,0xa1,0xb4,0xb3,0xba,0xbd,
 0xc7,0xc0,0xc9,0xce,0xdb,0xdc,0xd5,0xd2,0xff,0xf8,0xf1,0xf6,0xe3,0xe4,0xed,0xea,
 0xb7,0xb0,0xb9,0xbe,0xab,0xac,0xa5,0xa2,0x8f,0x88,0x81,0x86,0x93,0x94,0x9d,0x9a,
 0x27,0x20,0x29,0x2e,0x3b,0x3c,0x35,0x32,0x1f,0x18,0x11,0x16,0x03,0x04,0x0d,0x0a,
 0x57,0x50,0x59,0x5e,0x4b,0x4c,0x45,0x42,0x6f,0x68,0x61,0x66,0x73,0x74,0x7d,0x7a,
 0x89,0x8e,0x87,0x80,0x95,0x92,0x9b,0x9c,0xb1,0xb6,0xbf,0xb8,0xad,0xaa,0xa3,0xa4,
 0xf9,0xfe,0xf7,0xf0,0xe5,0xe2,0xeb,0xec,0xc1,0xc6,0xcf,0xc8,0xdd,0xda,0xd3,0xd4,
 0x69,0x6e,0x67,0x60,0x75,0x72,0x7b,0x7c,0x51,0x56,0x5f,0x58,0x4d,0x4a,0x43,0x44,
 0x19,0x1e,0x17,0x10,0x05,0x02,0x0b,0x0c,0x21,0x26,0x2f,0x28,0x3d,0x3a,0x33,0x34,
 0x4e,0x49,0x40,0x47,0x52,0x55,0x5c,0x5b,0x76,0x71,0x78,0x7f,0x6a,0x6d,0x64,0x63,
 0x3e,0x39,0x30,0x37,0x22,0x25,0x2c,0x2b,0x06,0x01,0x08,0x0f,0x1a,0x1d,0x14,0x13,
 0xae,0xa9,0xa0,0xa7,0xb2,0xb5,0xbc,0xbb,0x96,0x91,0x98,0x9f,0x8a,0x8d,0x84,0x83,
 0xde,0xd9,0xd0,0xd7,0xc2,0xc5,0xcc,0xcb,0xe6,0xe1,0xe8,0xef,0xfa,0xfd,0xf4,0xf3
 };


static void sensor_reset(void)
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

	lsm6dso_fifo_xl_batch_set(&imu_dev_ctx, LSM6DSO_XL_BATCHED_AT_104Hz);
	lsm6dso_fifo_gy_batch_set(&imu_dev_ctx, LSM6DSO_GY_BATCHED_AT_104Hz);

	lsm6dso_xl_full_scale_set(&imu_dev_ctx, LSM6DSO_4g); //4
	lsm6dso_gy_full_scale_set(&imu_dev_ctx, LSM6DSO_250dps);
	lsm6dso_block_data_update_set(&imu_dev_ctx, PROPERTY_ENABLE);
	lsm6dso_xl_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_104Hz);
	lsm6dso_gy_data_rate_set(&imu_dev_ctx, LSM6DSO_XL_ODR_104Hz);
}


static void historic_buffer(void)
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
					acceleration_mg[0] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[0]);
					acceleration_mg[1] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[1]);
					acceleration_mg[2] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[2]);

					acceleration_g[0]   = acceleration_mg[0]/1000;
					acceleration_g[1]   = acceleration_mg[1]/1000;
					acceleration_g[2]   = acceleration_mg[2]/1000;

					acc_x_hist_buffer[i] = acceleration_g[0]; //[i]
					acc_y_hist_buffer[i] = acceleration_g[1];
					acc_z_hist_buffer[i] = acceleration_g[2];   

					histBuff_counter++;
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

static void curr_vrif_buffers(void)
{
	float gyro_tempX[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_tempY[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_tempZ[ACC_GYRO_FIFO_BUF_LEN]  = {0};
	float accel_tempX[ACC_GYRO_FIFO_BUF_LEN] = {0}, accel_tempY[ACC_GYRO_FIFO_BUF_LEN] = {0}, accel_tempZ[ACC_GYRO_FIFO_BUF_LEN] = {0};

	float gyro_x_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN] = {0}, gyro_y_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0};
	float acc_x_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, acc_y_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0}, acc_z_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0};

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
				acceleration_mg[0] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[0]);
				acceleration_mg[1] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[1]);
				acceleration_mg[2] = lsm6dso_from_fs4_to_mg(data_raw_acceleration.i16bit[2]);

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
						gyro_x_vrif_buffer[i] = gyro_tempX[i_rev]; // gyro_x_cur_buffer
						gyro_y_vrif_buffer[i] = gyro_tempY[i_rev]; // gyro_y_cur_buffer
						gyro_z_vrif_buffer[i] = gyro_tempZ[i_rev]; // gyro_z_cur_buffer
						buff_counter++;
						i_rev--;
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
					}
				}

				if(buff_counter >= 5*ACC_GYRO_FIFO_BUF_LEN && buff_counter < 6*ACC_GYRO_FIFO_BUF_LEN)
				{
					for (uint8_t k = 0; k < ACC_GYRO_FIFO_BUF_LEN; k++)
					{
						acc_x_vrif_buffer[k]  = accel_tempX[k_rev]; //acc_x_vrif_buffer_1
						acc_y_vrif_buffer[k]  = accel_tempY[k_rev]; // _1
						acc_z_vrif_buffer[k]  = accel_tempZ[k_rev]; // _1
						gyro_x_vrif_buffer[k] = gyro_tempX[k_rev]; //gyro_x_vrif_buffer_1
						gyro_y_vrif_buffer[k] = gyro_tempY[k_rev]; //gyro_y_vrif_buffer_1
						gyro_z_vrif_buffer[k] = gyro_tempZ[k_rev]; //gyro_z_vrif_buffer_1
						buff_counter++;
						k_rev--;
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

// 角度分析
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

//
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
		
		if(acc_magn_square > max_acc_magn_square) 
			max_acc_magn_square = acc_magn_square;	//get the maximum acc magnitude square
	}
	return max_acc_magn_square;		//do once sqrt() to get acc magnitude
}

// 
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
		
		if(gyro_magn_square > max_gyro_magn_square)
			max_gyro_magn_square = gyro_magn_square;	//get the maximum gyroscope magnitude square
	}
	// return (sqrt(max_gyro_magn_square));		//do once sqrt() to get gyroscope magnitude
	return sqrt(max_gyro_magn_square);		//do once sqrt() to get gyroscope magnitude

}

//
static float fall_verification_fifo_skip(void)
{
	volatile uint16_t i;
	volatile float std_deviation = 0, variance=0,average = 0;
	//float verify_acc_magn[VERIFY_DATA_BUF_LEN*2] = {0};

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

#if 0
/*******************************************************************************************************
* description:
*	Get the string in the buffer by check the separator
* param:
*  inbuffer: [in] The source buffer that include the string we want to find.
*  outbuffer: [OUT] The dest buffer that we want
*  sign:[in] the end key char of the string that we need find in the source buffer.
*  index:[in] the index number of the key char in the source string
* retval:
*  true: It was found
*  false: It wasn't found
* author:
*  xie biao 2019-12-19
*******************************************************************************************************/
bool get_parameter_in_strbuffer(uint8_t *inbuffer, uint8_t *outbuffer, uint8_t *sign, int32_t index)
{
	uint32_t i,j=0;
	uint8_t *ptr1,*ptr2=NULL;
	if(inbuffer == NULL || outbuffer == NULL || sign == NULL || index == 0)
		return NULL;

	ptr1 = inbuffer;
	while(1)
	{
		ptr2 = strstr(ptr1, sign);
		if(ptr2 != NULL)
		{
			j++;
			if(j == index)
			{
				memcpy(outbuffer, ptr1, ptr2-ptr1);
				return true;
			}
			else
			{
				ptr1 = ptr2+strlen(sign);
				ptr2 = NULL;
			}
		}
		else
		{
			return false;
		}
	}
}

#if 0
void author_data_encode(uint8_t *buffer, uint32_t *buflen)
{
	uint8_t key, crc = 0;
	uint32_t i, tick;
	hal_rtc_time_t curtime;

	sprintf(check_log, "author_data_encode begin. buffer:%s, buflen:%d", buffer, *buflen);
	//my_print(check_log);

	/*Set random seed*/
	//hal_rtc_time_t curtime; 
	hal_rtc_get_time(&curtime);
	srand((unsigned int)curtime.rtc_milli_sec);
	key = rand()%0xff;
	if(key == 0)
		key = 0x31;
	
	sprintf(check_log, "author_data_encode 001 key:%02x", key);
	//my_print(check_log);

	for(i=0;i<(*buflen);i++)
	{
		buffer[i] = buffer[i]^key;
	}

	buffer[(*buflen)] = key;
	(*buflen)++;

	for(i=0;i<(*buflen);i++)
	{
		crc = crc^buffer[i];
		crc = CRC8Table[crc];
	}
	crc = crc^0x00;
	sprintf(check_log, "author_data_encode 002 crc:%02x", crc);
	//my_print(check_log);

	buffer[*buflen] = crc;
	(*buflen)++;

	sprintf(check_log, "author_data_encode end buf:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
							buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],
							buffer[7],buffer[8],buffer[9],buffer[10],buffer[11],buffer[12]);
	//my_print(check_log);
}
#endif
/*******************************************************************************************************
* description:
*	Verify the device is or not authorized
* param:
*  ID: [in] Chip ID of the device
*  IMEI:[in] IMEI of the device
*  buf:[in] Authorization data obtained by the device from the server,
*           sample:
*				chip id+IMEI+ok+key+crc
*					MT2503A_SN1234567&355287000480974&1&FE07
*				chip id+IMEI+err+key+crc
*					MT2503A_SN1234567&355287000480974&-2&FE07
*  len:[in] Authorization data lenth
* retval:
*  1:success;
*  -1: Data format error;
*  -2: Data CRC verification failed;
*  -3: Chip id is out of range;
*  -4: IMEI is out of range;
*  -5: The validity period has expired
*  -6: Illegal use of chip ID
*  -7: Illegal use of IMEI
* author:
*  xie biao 2019-12-19
*******************************************************************************************************/
int32_t author_data_decode(uint8_t *chip_ID, char *IMEI, char *buf, uint32_t len)
{
	uint8_t data, crc = 0;
	uint32_t i = 0;
	int32_t result;
	uint8_t buffer[256] = {0};
	uint8_t idbuf[128] = {0};
	uint8_t imeibuf[32] = {0};
	uint8_t retbuf[10] = {0};

	if(buf == NULL || len == 0)
		return -1;   //buf is empty
	//sprintf(check_log, "author_check len:%d, buf:%s", len, buf);
	//my_print(check_log);

	len = len/2;
	while(i < len)
	{
		uint8_t tmphex;

		if(*buf >= '0' && *buf <= '9')
			tmphex = *buf - 48;
		else
			tmphex = 0x0a + (*buf - 'a');
		buffer[i] = tmphex;
		buf++;

		if(*buf >= '0' && *buf <= '9')
			tmphex = *buf - 48;
		else
			tmphex = 0x0a + (*buf - 'a');
		buffer[i] = (buffer[i]<<4) + tmphex;
		buf++;
		i++;
	}

	/*sprintf(check_log, "author_check len:%d buffer:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
						len, buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],
						buffer[7],buffer[8],buffer[9],buffer[50],buffer[51],buffer[52]);*/
	//my_print(check_log);

	for(i = 0; i < len - 1; i++)
	{
		data = buffer[i];
		crc = crc^data;
		crc = CRC8Table[crc];  //CRC check from table
	}
	crc = crc^0x00;         //CRC calculate

	if(crc != buffer[len -1])
	{
		//sprintf(check_log, "check crc error crc:%02x, buffer[end]:%02x", crc, buffer[len-1]);
		//my_print(check_log); 
	}                          //CRC check error
	//sprintf(check_log, "check key. key:%02x", buffer[len-2]);
	//my_print(check_log); 

	for(i=0;i<len-2;i++)
	{
		buffer[i] = buffer[i]^buffer[len-2];							//buffer decoding
	}

	buffer[len - 1] = 0x00;
	buffer[len - 2] = 0x00;

	//sprintf(check_log, "check 0 %s", buffer);
	//my_print(check_log); 
	if(!get_parameter_in_strbuffer(buffer, idbuf, "&", 1))  //Can't find chip ID
		return -1;
	if(strcmp(idbuf, chip_ID) != 0)                              //chip ID check error
	{
		//sprintf(check_log, "Chip ID wrong. idbuf:%s, ID:%s", idbuf, chip_ID);
		//my_print(check_log); 
		return -6;
	}

	if(!get_parameter_in_strbuffer(buffer, imeibuf, "&", 2))	//don't find IMEI
		return -1;
	if(strcmp(imeibuf, IMEI) != 0)							//IMEI check error
	{
		//sprintf(check_log, "IMEI wrong. imeibuf:%s, IMEI:%s", imeibuf, IMEI);
		//my_print(check_log); 
		return -7;
	}

	if(!get_parameter_in_strbuffer(buffer, retbuf, "&", 3))	//don't find result
		return -1;
	result = atoi(retbuf);
	//sprintf(check_log, "Other wrong. retbuf:%s, result:%d", retbuf, result);
	//my_print(check_log);

	switch(result)
	{
	case 1:
		//my_print("fall_detection_author_check check: ok!");
		//author_check_success = true;
		return 1;
	case -1:
		//my_print("fall_detection_author_check check: Data format erro!");
		return -1;
	case -2:
		//my_print("fall_detection_author_check check: Data CRC verification failed!");
		return -2;
	case -3:
		//my_print("fall_detection_author_check check: Chip id is out of range!");
		return -3;
	case -4:
		//my_print("fall_detection_author_check check: IMEI is out of range!");
		return -4;
	case -5:
		//my_print("fall_detection_author_check check: The validity period has expired!");
		return -5;
	case -6:
		//my_print("fall_detection_author_check check: Illegal use of chip ID!");
		return -6;
	case -7:
		//my_print("fall_detection_author_check check: Illegal use of IMEI!");
		return -7;
	default:
		//my_print("fall_detection_author_check check: Unkonw erro!");
		return -1;
	}
	//return 0;
}
#endif

void fall_detection(void)//(char *check_code)
{
	//uint32_t len;

#if 0
	if(check_ok != 1)
	{
		len = strlen(check_code);
		check_ok = author_data_decode("UIS8910_SN1234567",imeiData2,check_code, len); // UIS8910_SN1234567     MT2503A_SN1234567
	}
#endif

	if(1) // (check_ok==1)
	{
		historic_buffer();

		if(hist_buff_flag)
		{
			curr_vrif_buffers();
			hist_buff_flag = false;
		}

		if(curr_vrif_buff_flag)
		{   
			acc_magn_square = acceleration_analyse_fifo();
			if(acc_magn_square > 38) // 4g=38, 2g=10
			{
				cur_angle = angle_analyse_fifo();
				cur_max_gyro_magn = gyroscope_analyse_fifo();
				cur_fuzzy_output = fuzzy_analyse(cur_angle, cur_max_gyro_magn);
				std_devi = fall_verification_fifo_skip();

				if(cur_fuzzy_output > FUZZY_OUT_THRES_DEF)
				{
					if(std_devi < STD_VARIANCE_THRES_DEF)
					{
						fall_result = true;
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
			} 
			else
			{
				fall_result = false;
			}
			curr_vrif_buff_flag = false;
			//imu_sensor_init(); //resets the algorithm, will work continuosly on every tap
		}
	}
	else
	{
		fall_result = false;
	}
}
