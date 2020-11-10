#ifndef __MAX20353_H__
#define __MAX20353_H__

#define BATTERY_VOLTAGE_LOW_NOTIFY	(3.55)
#define BATTERY_VOLTAGE_SHUTDOWN	(3.40)

#define MOTOR_TYPE_ERM	//转子马达
//#define MOTOR_TYPE_LRA	//线性马达

extern bool pmu_trige_flag;
extern bool pmu_alert_flag;

extern void test_pmu(void);
extern void pmu_init(void);
extern void MAX20353_Init(void);
extern void Set_Screen_Backlight_On(void);
extern void Set_Screen_Backlight_Off(void);
extern void SystemShutDown(void);

#endif/*__MAX20353_H__*/
