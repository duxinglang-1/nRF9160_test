/****************************************Copyright (c)************************************************
** File name:			    screen.h
** Last modified Date:          
** Last Version:		   
** Descriptions:		   	使用的ncs版本-1.2		
** Created by:				谢彪
** Created date:			2020-12-16
** Version:			    	1.0
** Descriptions:			屏幕UI管理H文件
******************************************************************************************************/
#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <zephyr/types.h>
#include <sys/slist.h>
#include "font.h"

#ifdef __cplusplus
extern "C" {
#endif

//time
#define IDLE_HOUR_H_X	8
#define IDLE_HOUR_H_Y	16
#define IDLE_HOUR_L_X	25
#define IDLE_HOUR_L_Y	16

#define IDLE_COLON_X	42
#define IDLE_COLON_Y	16

#define IDLE_MIN_H_X	54
#define IDLE_MIN_H_Y	16
#define IDLE_MIN_L_X	71
#define IDLE_MIN_L_Y	16

//AM/PM
#define IDLE_AM_PM_X	55
#define IDLE_AM_PM_Y	0
#define IDLE_AM_PM_W	14
#define IDLE_AM_PM_H	16

//ble
#define IDLE_BLE_X		45
#define IDLE_BLE_Y		0
#define IDLE_BLE_W		7
#define IDLE_BLE_H		16

//date
#define IDLE_MONTH_X	65
#define IDLE_MONTH_Y	40

#define IDLE_DATA_NUM_1_X	42
#define IDLE_DATA_NUM_1_Y	40
#define IDLE_DATA_NUM_2_X	52
#define IDLE_DATA_NUM_2_Y	40
#define IDLE_DATA_NUM_3_X	68
#define IDLE_DATA_NUM_3_Y	40
#define IDLE_DATA_NUM_4_X	78
#define IDLE_DATA_NUM_4_Y	40

#define IDLE_DATE_LINK_X	62
#define IDLE_DATE_LINK_Y	40

//week
#define IDLE_WEEK_SHOW_X	8
#define IDLE_WEEK_SHOW_Y	40

//NB signal
#define NB_SIGNAL_X			2
#define NB_SIGNAL_Y			0

//battery soc
#define BAT_LEVEL_X		75
#define BAT_LEVEL_Y		0

//sport
#define IMU_STEPS_SHOW_X	15
#define IMU_STEPS_SHOW_Y	160
#define IMU_STEPS_SHOW_W	210
#define IMU_STEPS_SHOW_H	20

//SOS
#define SOS_X	9
#define SOS_Y	0

//steps
#define STEPS_ICON_X	0
#define STEPS_ICON_Y	0
#define STEPS_CN_OFFSET	5
#define STEPS_UNIT_X	75
#define STEPS_UNIT_Y	32
#define STEPS_NUM_1_X	4
#define STEPS_NUM_1_Y	32
#define STEPS_NUM_2_X	18
#define STEPS_NUM_2_Y	32
#define STEPS_NUM_3_X	32
#define STEPS_NUM_3_Y	32
#define STEPS_NUM_4_X	44
#define STEPS_NUM_4_Y	32
#define STEPS_NUM_5_X	58
#define STEPS_NUM_5_Y	32

//sleep
#define SLEEP_ICON_X	0
#define SLEEP_ICON_Y	0
#define SLEEP_HOUR_X	36
#define SLEEP_HOUR_Y	32
#define SLEEP_MIN_X		80
#define SLEEP_MIN_Y		32
#define SLEEP_NUM_1_X	6
#define SLEEP_NUM_1_Y	32
#define SLEEP_NUM_2_X	21
#define SLEEP_NUM_2_Y	32
#define SLEEP_NUM_3_X	49
#define SLEEP_NUM_3_Y	32
#define SLEEP_NUM_4_X	64
#define SLEEP_NUM_4_Y	32

//distance
#define DIS_ICON_X		0
#define DIS_ICON_Y		0
#define DIS_KM_X		69
#define DIS_KM_Y		32
#define DIS_NUM_1_X		4
#define DIS_NUM_1_Y		32
#define DIS_NUM_2_X		19
#define DIS_NUM_2_Y		32
#define DIS_DOT_X		34
#define DIS_DOT_Y		31
#define DIS_NUM_3_X		39
#define DIS_NUM_3_Y		32
#define DIS_NUM_4_X		54
#define DIS_NUM_4_Y		32

//calorie
#define CAL_ICON_X		0
#define CAL_ICON_Y		0
#define CAL_UNIT_X		67
#define CAL_UNIT_Y		32
#define CAL_NUM_1_X		9
#define CAL_NUM_1_Y		32
#define CAL_NUM_2_X		24
#define CAL_NUM_2_Y		32
#define CAL_NUM_3_X		39
#define CAL_NUM_3_Y		32
#define CAL_NUM_4_X		54
#define CAL_NUM_4_Y		32

//fall
#define FALL_ICON_X	9
#define FALL_ICON_Y	0
#define FALL_CN_TEXT_X	39
#define FALL_CN_TEXT_Y	0
#define FALL_EN_TEXT_X	50
#define FALL_EN_TEXT_Y	0

//wrist
#define WRIST_ICON_X	9
#define WRIST_ICON_Y	0
#define WRIST_CN_TEXT_X	39
#define WRIST_CN_TEXT_Y	0
#define WRIST_EN_TEXT_X	31
#define WRIST_EN_TEXT_Y	0

//find
#define FIND_ICON_X		0
#define FIND_ICON_Y		0
#define FIND_TEXT_X		0
#define FIND_TEXT_Y		32

//idle screen update event
#define SCREEN_EVENT_UPDATE_NO			0x00000000
#define SCREEN_EVENT_UPDATE_SIG			0x00000001
#define SCREEN_EVENT_UPDATE_BAT			0x00000002
#define SCREEN_EVENT_UPDATE_TIME		0x00000004
#define SCREEN_EVENT_UPDATE_DATE		0x00000008
#define SCREEN_EVENT_UPDATE_WEEK		0x00000010
#define SCREEN_EVENT_UPDATE_SPORT		0x00000020
#define SCREEN_EVENT_UPDATE_HEALTH		0x00000040
#define SCREEN_EVENT_UPDATE_SLEEP		0x00000080
#define SCREEN_EVENT_UPDATE_SOS			0x00000100
#define SCREEN_EVENT_UPDATE_WRIST		0x00000200
#define SCREEN_EVENT_UPDATE_FOTA		0x00004000


//notify
#define NOTIFY_TEXT_MAX_LEN		80
#define NOTIFY_TIMER_INTERVAL	5


//power off confirm
#define PWR_OFF_NOTIFY_OFFSET_W		4
#define PWR_OFF_NOTIFY_OFFSET_H		4

#define PWR_OFF_NOTIFY_RECT_W		180
#define PWR_OFF_NOTIFY_RECT_H		130
#define PWR_OFF_NOTIFY_RECT_X		((LCD_WIDTH-PWR_OFF_NOTIFY_RECT_W)/2)
#define PWR_OFF_NOTIFY_RECT_Y		((LCD_HEIGHT-PWR_OFF_NOTIFY_RECT_H)/2)

#define PWR_OFF_NOTIFY_STRING_W		(PWR_OFF_NOTIFY_RECT_W-10)
#define	PWR_OFF_NOTIFY_STRING_H		(50)
#define	PWR_OFF_NOTIFY_STRING_X		((LCD_WIDTH-PWR_OFF_NOTIFY_STRING_W)/2)
#define	PWR_OFF_NOTIFY_STRING_Y		(PWR_OFF_NOTIFY_RECT_Y+20)

#define PWR_OFF_NOTIFY_YES_W		60
#define PWR_OFF_NOTIFY_YES_H		40
#define PWR_OFF_NOTIFY_YES_X		(PWR_OFF_NOTIFY_RECT_X+10)
#define PWR_OFF_NOTIFY_YES_Y		(PWR_OFF_NOTIFY_RECT_Y+(PWR_OFF_NOTIFY_RECT_H-PWR_OFF_NOTIFY_YES_H-PWR_OFF_NOTIFY_OFFSET_H))

#define PWR_OFF_NOTIFY_NO_W			60
#define PWR_OFF_NOTIFY_NO_H			40
#define PWR_OFF_NOTIFY_NO_X			(PWR_OFF_NOTIFY_RECT_X+(PWR_OFF_NOTIFY_RECT_W-PWR_OFF_NOTIFY_NO_W-10))
#define PWR_OFF_NOTIFY_NO_Y			(PWR_OFF_NOTIFY_RECT_Y+(PWR_OFF_NOTIFY_RECT_H-PWR_OFF_NOTIFY_NO_H-PWR_OFF_NOTIFY_OFFSET_H))


//screen ID
typedef enum
{
	SCREEN_ID_BOOTUP,
	SCREEN_ID_IDLE,
	SCREEN_ID_ALARM,
	SCREEN_ID_FIND_DEVICE,
	SCREEN_ID_HR,
	SCREEN_ID_ECG,
	SCREEN_ID_BP,
	SCREEN_ID_SOS,
	SCREEN_ID_SLEEP,
	SCREEN_ID_STEPS,
	SCREEN_ID_DISTANCE,
	SCREEN_ID_CALORIE,
	SCREEN_ID_FALL,
	SCREEN_ID_WRIST,
	SCREEN_ID_SETTINGS,
	SCREEN_ID_GPS_TEST,
	SCREEN_ID_NB_TEST,
	SCREEN_ID_WIFI_TEST,
	SCREEN_ID_BLE_TEST,
	SCREEN_ID_NOTIFY,
	SCREEN_ID_POWEROFF,
	SCREEN_ID_FOTA,
	SCREEN_ID_MAX
}SCREEN_ID_ENUM;

typedef enum
{
	SCREEN_ACTION_NO,
	SCREEN_ACTION_ENTER,
	SCREEN_ACTION_UPDATE,
	SCREEN_ACTION_MAX
}SCREEN_ACTION_ENUM;

typedef enum
{
	SCREEN_STATUS_NO,
	SCREEN_STATUS_CREATING,
	SCREEN_STATUS_CREATED,
	SCREEN_STATUS_MAX
}SCREEN_STATUS_ENUM;

typedef struct
{
	SCREEN_STATUS_ENUM status;
	SCREEN_ACTION_ENUM act;
	u32_t para;
}screen_msg;

typedef enum
{
	NOTIFY_TYPE_POPUP,
	NOTIFY_TYPE_CONFIRM,
	NOTIFY_TYPE_MAX
}NOTIFY_TYPE_ENUM;

typedef enum
{
	NOTIFY_ALIGN_CENTER,
	NOTIFY_ALIGN_BOUNDARY,
	NOTIFY_ALIGN_MAX
}NOTIFY_ALIGN_ENUM;

typedef struct
{
	NOTIFY_TYPE_ENUM type;
	NOTIFY_ALIGN_ENUM align;
	u8_t *img;
	u32_t img_addr;
	u8_t text[NOTIFY_TEXT_MAX_LEN+1];
}notify_infor;

extern SCREEN_ID_ENUM screen_id;
extern screen_msg scr_msg[SCREEN_ID_MAX];

extern void ShowBootUpLogo(void);
extern void EnterIdleScreen(void);
extern void EnterAlarmScreen(void);
extern void EnterFindDeviceScreen(void);
extern void EnterGPSTestScreen(void);
extern void EnterNBTestScreen(void);
extern void EnterPoweroffScreen(void);
extern void GoBackHistoryScreen(void);
extern void ScreenMsgProcess(void);
extern void ExitNotifyScreen(void);
extern void EnterFOTAScreen(void);
extern void DisplayPopUp(u8_t *message);

#ifdef __cplusplus
}
#endif

#endif/*__SCREEN_H__*/
