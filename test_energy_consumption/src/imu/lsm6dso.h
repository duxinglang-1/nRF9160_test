#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>

extern bool reset_steps;

extern u16_t g_steps;
extern u16_t g_calorie;
extern u16_t g_distance;

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
extern void Set_Gsensor_data(signed short x, signed short y, signed short z, int setp,int hr,int hour,int charging);
extern int get_light_sleep_time(void); //return light sleep time in minutes
extern int get_deep_sleep_time(void); //return deep sleep time in minutes
