#ifndef __MAX20353_H__
#define __MAX20353_H__

#define BATTERY_VOLTAGE_LOW_NOTIFY	(3.55)
#define BATTERY_VOLTAGE_SHUTDOWN	(3.40)

#define MOTOR_TYPE_ERM	//转子马达
//#define MOTOR_TYPE_LRA	//线性马达

typedef enum
{
	BAT_CHARGING_NO,
	BAT_CHARGING_PROGRESS,
	BAT_CHARGING_FINISHED,
	BAT_CHARGING_MAX
}BAT_CHAEGER_STATUS;

typedef enum
{
	BAT_LEVEL_VERY_LOW,
	BAT_LEVEL_LOW,
	BAT_LEVEL_NORMAL,
	BAT_LEVEL_GOOD,
	BAT_LEVEL_MAX
}BAT_LEVEL_STATUS;

extern bool pmu_trige_flag;
extern bool pmu_alert_flag;
extern bool vibrate_start_flag;
extern bool vibrate_stop_flag;

extern u8_t g_bat_soc;
extern BAT_CHAEGER_STATUS g_chg_status;
extern BAT_LEVEL_STATUS g_bat_level;

extern void test_pmu(void);
extern void pmu_init(void);
extern void MAX20353_Init(void);
extern void Set_Screen_Backlight_On(void);
extern void Set_Screen_Backlight_Off(void);
extern void SystemShutDown(void);
extern void PMUMsgProcess(void);
#endif/*__MAX20353_H__*/
