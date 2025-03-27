#ifndef __MAX20353_H__
#define __MAX20353_H__

#define BATTERY_VOLTAGE_LOW_NOTIFY	(3.55)
#define BATTERY_VOLTAGE_SHUTDOWN	(3.40)

#define MOTOR_TYPE_ERM	//ת�����
//#define MOTOR_TYPE_LRA	//�������

#define BATTERY_SOC_GAUGE	//xb add 20201124 �����ƹ��ܵĴ���
#define BATTERT_NTC_CHECK	//xb add 20210106 �������NTC�¶ȼ��

#define GPIO_ACT_I2C

#ifdef BATTERY_SOC_GAUGE
#define VERIFY_AND_FIX 1
#define LOAD_MODEL !(VERIFY_AND_FIX)
#define EMPTY_ADJUSTMENT		0
#define FULL_ADJUSTMENT			100
#define RCOMP0					64
#define TEMP_COUP				(-1.96875)
#define TEMP_CODOWN				(-7.875)
#define TEMP_CODOWNN10			(-3.90625)
#define OCVTEST					57984
#define SOCCHECKA				113
#define SOCCHECKB				115
#define BITS					18
#define RCOMPSEG				0x0100
#define INI_OCVTEST_HIGH_BYTE 	(OCVTEST>>8)
#define INI_OCVTEST_LOW_BYTE	(OCVTEST&0x00ff)
#endif

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

typedef enum
{
	VIB_ONCE,
	VIB_CONTINUITY,
	VIB_RHYTHMIC,
	VIB_MAX
}VIBRATE_MODE;

typedef struct
{
	VIBRATE_MODE work_mode;
	uint32_t on_time;
	uint32_t off_time;
}vibrate_msg_t;

extern bool pmu_trige_flag;
extern bool pmu_alert_flag;
extern bool pmu_check_temp_flag;
extern bool vibrate_start_flag;
extern bool vibrate_stop_flag;
extern bool charger_is_connected;

extern uint8_t g_bat_soc;
extern BAT_CHARGER_STATUS g_chg_status;
extern BAT_LEVEL_STATUS g_bat_level;

extern void test_pmu(void);
extern void pmu_init(void);
extern void Set_Screen_Backlight_On(void);
extern void Set_Screen_Backlight_Off(void);
extern void Set_PPG_Power_On(void);
extern void Set_PPG_Power_Off(void);
extern void SystemShutDown(void);
extern void PMUMsgProcess(void);
extern void GetBatterySocString(uint8_t *str_utc);
extern void system_power_off(uint8_t flag);
#endif/*__MAX20353_H__*/
