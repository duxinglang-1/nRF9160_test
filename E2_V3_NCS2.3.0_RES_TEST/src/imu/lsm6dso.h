#ifdef CONFIG_IMU_SUPPORT

#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "inner_flash.h"

typedef struct
{
	uint16_t deep;
	uint16_t light;
}sleep_data;

//单次测量
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint16_t steps;
}step_rec1_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	sleep_data sleep;
}sleep_rec1_data;

//整点测量
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint16_t steps[24];
}step_rec2_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	sleep_data sleep[24];
}sleep_rec2_data;

#ifdef CONFIG_FALL_DETECT_SUPPORT
extern bool fall_wait_gps;
extern bool fall_result; //fall_result = true when fall happens
extern void fall_detection(void); //fall detection algorithm, if a fall happens, bool fall_result = true;
#endif

#ifdef CONFIG_STEP_SUPPORT
extern bool reset_steps;
extern sport_record_t last_sport;
extern uint16_t g_last_steps;
extern uint16_t g_steps;
extern uint16_t g_calorie;
extern uint16_t g_distance;

extern void GetImuSteps(uint16_t *steps);
extern void ReSetImuSteps(void);
extern void UpdateIMUData(void);
extern void GetSportData(uint16_t *steps, uint16_t *calorie, uint16_t *distance);
extern void lsm6dso_sensitivity(void);
#endif

extern void IMU_init(struct k_work_q *work_q);
extern void IMUMsgProcess(void);

/*fall detection*/
extern bool int2_event; //trigger of fall-detection

/*wrist tilt*/
extern bool is_tilt(void); //detect if a tilt happened
//extern void disable_tilt_detection(void); //disable wrist tilt detection
//extern void enable_tilt_detection(void); //enable wrist tilt detection

extern bool int1_event;

// imu off
extern void imu_sensor_off(void);

#endif/*CONFIG_IMU_SUPPORT*/
