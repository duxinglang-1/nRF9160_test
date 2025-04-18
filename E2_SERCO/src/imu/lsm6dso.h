#ifdef CONFIG_IMU_SUPPORT

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include "inner_flash.h"

//单次测量
typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t hour;
	u8_t min;
	u8_t sec;
	u16_t steps;
}step_rec1_data;

//整点测量
typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u16_t steps[24];
}step_rec2_data;

extern bool reset_steps;
#ifdef CONFIG_FALL_DETECT_SUPPORT
extern bool fall_wait_gps;
#endif
extern u16_t g_last_steps;
extern u16_t g_steps;
extern u16_t g_calorie;
extern u16_t g_distance;

extern sport_record_t last_sport;

extern void IMU_init(struct k_work_q *work_q);
extern void IMUMsgProcess(void);

/*fall detection*/
extern bool int2_event; //trigger of fall-detection
extern bool fall_result; //fall_result = true when fall happens
extern void fall_detection(void); //fall detection algorithm, if a fall happens, bool fall_result = true;

/*wrist tilt*/
extern void is_tilt(void); //detect if a tilt happened
extern bool wrist_tilt; //wrist_tilt = true if a tilt happened
extern void disable_tilt_detection(void); //disable wrist tilt detection
extern void enable_tilt_detection(void); //enable wrist tilt detection

/*step counter*/
extern bool int1_event;
extern void GetImuSteps(u16_t *steps);
extern void ReSetImuSteps(void);
extern void UpdateIMUData(void);
extern void GetSportData(u16_t *steps, u16_t *calorie, u16_t *distance);
extern void lsm6dso_sensitivity(void);

/*sleep monitor*/
extern void Set_Gsensor_data(signed short x, signed short y, signed short z, int step, int hr, int hour, int minute, int charging);
extern int get_light_sleep_time(void); //return light sleep time in minutes
extern int get_deep_sleep_time(void); //return deep sleep time in minutes

#endif/*CONFIG_IMU_SUPPORT*/
