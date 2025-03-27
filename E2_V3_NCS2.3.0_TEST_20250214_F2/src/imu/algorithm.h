#ifndef ALGORITHM_H__
#define ALGORITHM_H__

// Versions: August.FD_20240626
// 0.15F,45.0f,20,45,85,185,305,<65,4g

#include <stdio.h> 
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include "lsm6dso_reg.h"

typedef union{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

#define LSM6DSO_INT1_PIN	9
#define LSM6DSO_INT2_PIN	10
#define LSM6DSO_SDA_PIN		11
#define LSM6DSO_SCL_PIN		12

#define TWI_INSTANCE_ID               0
#define LSM6DSO_I2C_ADD               LSM6DSO_I2C_ADD_L >> 1 //need to shift 1 bit to the right.
#define ACC_GYRO_FIFO_BUF_LEN         200
#define VERIFY_DATA_BUF_LEN           200
#define PATTERN_LEN                   2*ACC_GYRO_FIFO_BUF_LEN
#define TWI_BUFFER_SIZE               14
#define TWI_TIMEOUT                   1000
#define INT_PIN                       10

//define low, medium, high memership
#define LOW_MS                        1
#define MEDIUM_MS                     2
#define HIGH_MS                       3

//define the memberships of Angle input
#define LOW_ANGLE                     15
#define MEDIUM_ANGLE                  40
#define HIGH_ANGLE                    80

//define the memberships of Gyroscope's Magnitude input
#define LOW_GYRO_MAGNITUDE            80
#define MEDIUM_GYRO_MAGNITUDE         180
#define HIGH_GYRO_MAGNITUDE           300

//define weight value
#define	WEIGHT_VALUE_10               10
#define	WEIGHT_VALUE_30               30
#define	WEIGHT_VALUE_50               50

//define default thresholds 
#define ACC_MAGN_TRIGGER_THRES_DEF    9.0f 	 // 9.0 for acc trigger, 7.0-10.0的范围内设置。数值越小越容易通过判定
#define FUZZY_OUT_THRES_DEF           43.0f //46.0f	 // 46.0 for fuzzy output, 40.0-50.0的范围内调整。数值越小越容易通过判定
#define STD_SVMG_SECOND_STAGE         12.0f
#define STD_VARIANCE_THRES_DEF        0.15f  	 // 0.15 for Standard Deviation, 0.1-0.2的范围内调整。数值越大越容易通过判定
#define PEAKS_NO_THRES                8       //number of peaks threshold
#define PEAK_THRES                    11.0f
#define MAX_GYROSCOPE_THRESHOLD       200  //max gyroscope threshold
#define MAX_ANGLE_THRESHOLD           40   
#define	get_acc_magn(x)               x   //do nothing
#define	get_gyro_magn(x)              x   //do noting


//define minimum function
#define min(a,b) ((a)<(b)?(a):(b))

extern stmdev_ctx_t imu_dev_ctx;
extern lsm6dso_pin_int2_route_t int2_route;


void fall_detection(void); //(char *check_code);



#endif


