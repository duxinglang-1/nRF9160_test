#ifndef __MAX20353_H__
#define __MAX20353_H__

#define BATTERY_VOLTAGE_LOW_NOTIFY	(3.55)
#define BATTERY_VOLTAGE_SHUTDOWN	(3.40)

#define MOTOR_TYPE_ERM	//ת������
//#define MOTOR_TYPE_LRA	//��������

#define BATTERY_SOC_GAUGE	//xb add 20201124 �����ƹ��ܵĴ���
#define BATTERT_NTC_CHECK	//xb add 20210106 �������NTC�¶ȼ��

typedef enum
{
	BAT_CHARGING_NO,
	BAT_CHARGING_PROGRESS,
	BAT_CHARGING_FINISHED,
	BAT_CHARGING_MAX
}BAT_CHARGER_STATUS;

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
extern bool pmu_check_temp_flag;
extern bool vibrate_start_flag;
extern bool vibrate_stop_flag;

extern u8_t g_bat_soc;
extern BAT_CHARGER_STATUS g_chg_status;
extern BAT_LEVEL_STATUS g_bat_level;

extern void test_pmu(void);
extern void pmu_init(void);
extern void Set_Screen_Backlight_On(void);
extern void Set_Screen_Backlight_Off(void);
extern void SystemShutDown(void);
extern void PMUMsgProcess(void);
extern void GetBatterySocString(u8_t *str_utc);
#endif/*__MAX20353_H__*/