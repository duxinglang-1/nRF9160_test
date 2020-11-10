#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>

extern bool read_imu_steps;
extern bool reset_imu_steps;

extern void IMU_init(void);
extern void GetImuSteps(void);
extern void ReSetImuSteps(void);
extern void GetSportData(u16_t *steps, u16_t *calorie, u16_t *distance);