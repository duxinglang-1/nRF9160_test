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
#include "lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

//pwron
#define PWRON_STR_W		198
#define PWRON_STR_H		25
#define PWRON_STR_X		((LCD_WIDTH-PWRON_STR_W)/2)
#define PWRON_STR_Y		135
#define PWRON_LOGO_W	223
#define PWRON_LOGO_H	30
#define PWRON_LOGO_X	((LCD_WIDTH-PWRON_LOGO_W)/2)
#define PWRON_LOGO_Y	((LCD_HEIGHT-PWRON_LOGO_H)/2)

//date&time
#define IDLE_TIME_X	43
#define IDLE_TIME_Y	52
#define IDLE_DATE_MON_X	149
#define IDLE_DATE_MON_Y	120
#define IDLE_DATE_DAY_X	110
#define IDLE_DATE_DAY_Y	120
#define IDLE_WEEK_X	45
#define IDLE_WEEK_Y	120

//ble
#define IDLE_BLE_X		45
#define IDLE_BLE_Y		0
#define IDLE_BLE_W		7
#define IDLE_BLE_H		16

//network mode
#define IDLE_NET_MODE_W		12
#define IDLE_NET_MODE_H		16
#define IDLE_NET_MODE_X		108
#define IDLE_NET_MODE_Y		35

//battery soc
#define IDLE_BAT_W		30
#define IDLE_BAT_H		16
#define IDLE_BAT_X		169
#define IDLE_BAT_Y		35
#define IDLE_BAT_PERCENT_W		44
#define IDLE_BAT_PERCENT_H		18
#define IDLE_BAT_PERCENT_X		120
#define IDLE_BAT_PERCENT_Y		33
#define IDLE_BAT_INNER_RECT_W	22
#define IDLE_BAT_INNER_RECT_H	10
#define IDLE_BAT_INNER_RECT_X	172
#define IDLE_BAT_INNER_RECT_Y	38

//NB signal
#define IDLE_SIGNAL_W		54
#define IDLE_SIGNAL_H		12
#define IDLE_SIGNAL_X		45
#define IDLE_SIGNAL_Y		36

//step
#define IDLE_STEPS_BG_W		176
#define IDLE_STEPS_BG_H		57
#define IDLE_STEPS_BG_X		32
#define IDLE_STEPS_BG_Y		183
#define IDLE_STEPS_NUM_W	18
#define IDLE_STEPS_NUM_H	36
#define IDLE_STEPS_NUM_X	91
#define IDLE_STEPS_NUM_Y	192

//idle hr
#define IDLE_HR_BG_W		60
#define IDLE_HR_BG_H		60
#define IDLE_HR_BG_X		45
#define IDLE_HR_BG_Y		158
#define IDLE_HR_ICON_W	27
#define IDLE_HR_ICON_H	24
#define IDLE_HR_ICON_X	62
#define IDLE_HR_ICON_Y	165
#define IDLE_HR_NUM_W	33
#define IDLE_HR_NUM_H	20
#define IDLE_HR_NUM_X	59
#define IDLE_HR_NUM_Y	188

//idle temp
#define IDLE_TEMP_BG_W		60
#define IDLE_TEMP_BG_H		60
#define IDLE_TEMP_BG_X		135
#define IDLE_TEMP_BG_Y		158
#define IDLE_TEMP_ICON_W	20
#define IDLE_TEMP_ICON_H	24
#define IDLE_TEMP_ICON_X	155
#define IDLE_TEMP_ICON_Y	165
#define IDLE_TEMP_NUM_W		40
#define IDLE_TEMP_NUM_H		20
#define IDLE_TEMP_NUM_X		145
#define IDLE_TEMP_NUM_Y		188

//notify
#define NOTIFY_TEXT_MAX_LEN		80
#define NOTIFY_TIMER_INTERVAL	5

//sport
#define IMU_SEP_LINE_W				165
#define IMU_SEP_LINE_H				2
#define IMU_SEP_LINE_X				((LCD_WIDTH-IMU_SEP_LINE_W)/2)
#define IMU_SEP_LINE_Y				119
//steps
#define IMU_STEP_ICON_W				38
#define IMU_STEP_ICON_H				53
#define IMU_STEP_ICON_X				((LCD_WIDTH-IMU_STEP_ICON_W)/2)
#define IMU_STEP_ICON_Y				15
#define IMU_STEP_UNIT_W				49
#define IMU_STEP_UNIT_H				22
#define IMU_STEP_UNIT_X				151
#define IMU_STEP_UNIT_Y				89
#define IMU_STEP_STR_W				106
#define IMU_STEP_STR_H				30
#define IMU_STEP_STR_X				((LCD_WIDTH-IMU_STEP_STR_W)/2)
#define IMU_STEP_STR_Y				82
//calorie
#define IMU_CAL_ICON_W				28
#define IMU_CAL_ICON_H				36
#define IMU_CAL_ICON_X				57
#define IMU_CAL_ICON_Y				130
#define IMU_CAL_UNIT_W				25
#define IMU_CAL_UNIT_H				14
#define IMU_CAL_UNIT_X				85
#define IMU_CAL_UNIT_Y				182
#define IMU_CAL_STR_W				55
#define IMU_CAL_STR_H				27
#define IMU_CAL_STR_X				27
#define IMU_CAL_STR_Y				171
//distance
#define IMU_DIS_ICON_W				29
#define IMU_DIS_ICON_H				36
#define IMU_DIS_ICON_X				153
#define IMU_DIS_ICON_Y				130
#define IMU_DIS_UNIT_W				17
#define IMU_DIS_UNIT_H				14
#define IMU_DIS_UNIT_X				189
#define IMU_DIS_UNIT_Y				183
#define IMU_DIS_STR_W				62
#define IMU_DIS_STR_H				27
#define IMU_DIS_STR_X				125
#define IMU_DIS_STR_Y				171

//sleep
//total
#define SLEEP_SEP_LINE_W			176
#define SLEEP_SEP_LINE_H			2
#define SLEEP_SEP_LINE_X			((LCD_WIDTH-SLEEP_SEP_LINE_W)/2)
#define SLEEP_SEP_LINE_Y			119
#define SLEEP_TOTAL_ICON_W			46
#define SLEEP_TOTAL_ICON_H			50
#define SLEEP_TOTAL_ICON_X			((LCD_WIDTH-SLEEP_TOTAL_ICON_W)/2)
#define SLEEP_TOTAL_ICON_Y			20
#define SLEEP_TOTAL_UNIT_HR_W		19
#define SLEEP_TOTAL_UNIT_HR_H		22
#define SLEEP_TOTAL_UNIT_HR_X		93
#define SLEEP_TOTAL_UNIT_HR_Y		91
#define SLEEP_TOTAL_UNIT_MIN_W		33
#define SLEEP_TOTAL_UNIT_MIN_H		22
#define SLEEP_TOTAL_UNIT_MIN_X		162
#define SLEEP_TOTAL_UNIT_MIN_Y		91
#define SLEEP_TOTAL_STR_HR_W		43
#define SLEEP_TOTAL_STR_HR_H		42
#define SLEEP_TOTAL_STR_HR_X		55
#define SLEEP_TOTAL_STR_HR_Y		82
#define SLEEP_TOTAL_STR_MIN_W		43		
#define SLEEP_TOTAL_STR_MIN_H		42
#define SLEEP_TOTAL_STR_MIN_X		125
#define SLEEP_TOTAL_STR_MIN_Y		82
//light sleep
#define SLEEP_LIGHT_ICON_W			36
#define SLEEP_LIGHT_ICON_H			36
#define SLEEP_LIGHT_ICON_X			156
#define SLEEP_LIGHT_ICON_Y			130
#define SLEEP_LIGHT_STR_W			62		
#define SLEEP_LIGHT_STR_H			27
#define SLEEP_LIGHT_STR_X			143
#define SLEEP_LIGHT_STR_Y			171
//deep sleep
#define SLEEP_DEEP_ICON_W			36
#define SLEEP_DEEP_ICON_H			36
#define SLEEP_DEEP_ICON_X			48
#define SLEEP_DEEP_ICON_Y			130
#define SLEEP_DEEP_STR_W			62		
#define SLEEP_DEEP_STR_H			27
#define SLEEP_DEEP_STR_X			35
#define SLEEP_DEEP_STR_Y			171

//heart rate
#define HR_ICON_W					34
#define HR_ICON_H					31
#define HR_ICON_X					42
#define HR_ICON_Y					35
#define HR_UNIT_W					45
#define HR_UNIT_H					22
#define HR_UNIT_X					151
#define HR_UNIT_Y					46
#define HR_NUM_W					65
#define HR_NUM_H					42
#define HR_NUM_X					84
#define HR_NUM_Y					30
#define HR_BG_W						201
#define HR_BG_H						87
#define HR_BG_X						((LCD_WIDTH-HR_BG_W)/2)
#define HR_BG_Y						80
#define HR_UP_ARRAW_W				14				
#define HR_UP_ARRAW_H				17
#define HR_UP_ARRAW_X				50
#define HR_UP_ARRAW_Y				178
#define HR_UP_NUM_W					39				
#define HR_UP_NUM_H					27
#define HR_UP_NUM_X					67
#define HR_UP_NUM_Y					173
#define HR_DOWN_ARRAW_W				14
#define HR_DOWN_ARRAW_H				17
#define HR_DOWN_ARRAW_X				144
#define HR_DOWN_ARRAW_Y				177
#define HR_DOWN_NUM_W				39				
#define HR_DOWN_NUM_H				27
#define HR_DOWN_NUM_X				163
#define HR_DOWN_NUM_Y				173
#define HR_REC_DATA_X				20
#define HR_REC_DATA_Y				146
#define HR_REC_DATA_W				4
#define HR_REC_DATA_H				66
#define HR_REC_DATA_OFFSET_X		7
//spo2
#define SPO2_ICON_W					29
#define SPO2_ICON_H					46
#define SPO2_ICON_X					63
#define SPO2_ICON_Y					30
#define SPO2_NUM_W					76
#define SPO2_NUM_H					42
#define SPO2_NUM_X					102
#define SPO2_NUM_Y					30
#define SPO2_BG_W					192
#define SPO2_BG_H					87
#define SPO2_BG_X					((LCD_WIDTH-SPO2_BG_W)/2)
#define SPO2_BG_Y					80
#define SPO2_UP_ARRAW_W				14
#define SPO2_UP_ARRAW_H				17
#define SPO2_UP_ARRAW_X				43
#define SPO2_UP_ARRAW_Y				183
#define SPO2_UP_NUM_W				49
#define SPO2_UP_NUM_H				27
#define SPO2_UP_NUM_X				60
#define SPO2_UP_NUM_Y				178
#define SPO2_DOWN_ARRAW_W			14
#define SPO2_DOWN_ARRAW_H			17
#define SPO2_DOWN_ARRAW_X			131
#define SPO2_DOWN_ARRAW_Y			183
#define SPO2_DOWN_NUM_W				49
#define SPO2_DOWN_NUM_H				27
#define SPO2_DOWN_NUM_X				148
#define SPO2_DOWN_NUM_Y				178
#define SPO2_REC_DATA_X				25
#define SPO2_REC_DATA_Y				146
#define SPO2_REC_DATA_W				4
#define SPO2_REC_DATA_H				58
#define SPO2_REC_DATA_OFFSET_X		7
//blood pressure
#define BP_ICON_W					22
#define BP_ICON_H					40
#define BP_ICON_X					54
#define BP_ICON_Y					31
#define BP_NUM_W					105
#define BP_NUM_H					31
#define BP_NUM_X					81
#define BP_NUM_Y					36
#define BP_BG_W						208
#define BP_BG_H						87
#define BP_BG_X						((LCD_WIDTH-BP_BG_W)/2)
#define BP_BG_Y						80
#define BP_UP_ARRAW_W				17
#define BP_UP_ARRAW_H				17
#define BP_UP_ARRAW_X				43
#define BP_UP_ARRAW_Y				183
#define BP_UP_NUM_W					51
#define BP_UP_NUM_H					17
#define BP_UP_NUM_X					63
#define BP_UP_NUM_Y					183
#define BP_DOWN_ARRAW_W				12
#define BP_DOWN_ARRAW_H				16
#define BP_DOWN_ARRAW_X				133
#define BP_DOWN_ARRAW_Y				183
#define BP_DOWN_NUM_W				49
#define BP_DOWN_NUM_H				17
#define BP_DOWN_NUM_X				148
#define BP_DOWN_NUM_Y				183
#define BP_REC_DATA_X				17
#define BP_REC_DATA_Y				146
#define BP_REC_DATA_W				4
#define BP_REC_DATA_H				58
#define BP_REC_DATA_OFFSET_X		7

//temperature
#define TEMP_ICON_W					89
#define TEMP_ICON_H					110
#define TEMP_ICON_X					((LCD_WIDTH-TEMP_ICON_W)/2)
#define TEMP_ICON_Y					30
#define TEMP_UNIT_W					16
#define TEMP_UNIT_H					16
#define TEMP_UNIT_X					170
#define TEMP_UNIT_Y					145
#define TEMP_NUM_W					100
#define TEMP_NUM_H					26
#define TEMP_NUM_X					70
#define TEMP_NUM_Y					150
#define TEMP_RUNNING_ANI_W			81
#define TEMP_RUNNING_ANI_H			13
#define TEMP_RUNNING_ANI_X			((LCD_WIDTH-TEMP_RUNNING_ANI_W)/2)
#define TEMP_RUNNING_ANI_Y			212

//settings
#define SETTINGS_MAIN_MENU_MAX_PER_PG	4
#define SETTINGS_SUB_MENU_MAX_PER_PG	3

#define SETTINGS_MENU_BG_W			169
#define SETTINGS_MENU_BG_H			38
#define SETTINGS_MENU_BG_X			((LCD_WIDTH-SETTINGS_MENU_BG_W)/2)
#define SETTINGS_MENU_BG_Y			38
#define SETTINGS_MENU_BG_OFFSET_Y	4
#define SETTINGS_MENU_STR_OFFSET_X	5			
#define SETTINGS_MENU_STR_OFFSET_Y	8

#define SETTINGS_MENU_INFO_BG_W		170
#define SETTINGS_MENU_INFO_BG_H		50
#define SETTINGS_MENU_INFO_BG_X		((LCD_WIDTH-SETTINGS_MENU_INFO_BG_W)/2)
#define SETTINGS_MENU_INFO_BG_Y		40
#define SETTINGS_MENU_INFO_BG_OFFSET_Y	5
#define SETTINGS_MENU_INFO_STR_OFFSET_X	5			
#define SETTINGS_MENU_INFO_STR_OFFSET_Y	8

#define SETTINGS_MEUN_PAGE_DOT_W	6
#define SETTINGS_MEUN_PAGE_DOT_H	22
#define SETTINGS_MEUN_PAGE_DOT_X	226
#define SETTINGS_MEUN_PAGE_DOT_Y	((LCD_HEIGHT-SETTINGS_MEUN_PAGE_DOT_H)/2)
#define SETTINGS_MENU_TEMP_UNIT_W	35
#define SETTINGS_MENU_TEMP_UNIT_H	36
#define SETTINGS_MENU_TEMP_UNIT_X	161
#define SETTINGS_MENU_TEMP_UNIT_Y	39

#define SETTINGS_MENU_SEL_DOT_W		20
#define SETTINGS_MENU_SEL_DOT_H		20
#define SETTINGS_MENU_SEL_DOT_X		142
#define SETTINGS_MENU_SEL_DOT_Y		15

#define SETTINGS_MENU_BK_LEVEL_W	110
#define SETTINGS_MENU_BK_LEVEL_H	162
#define SETTINGS_MENU_BK_LEVEL_X	65
#define SETTINGS_MENU_BK_LEVEL_Y	20
#define SETTINGS_MENU_BK_INC_W		65
#define SETTINGS_MENU_BK_INC_H		99
#define SETTINGS_MENU_BK_INC_X		175
#define SETTINGS_MENU_BK_INC_Y		120
#define SETTINGS_MENU_BK_DEC_W		65
#define SETTINGS_MENU_BK_DEC_H		99
#define SETTINGS_MENU_BK_DEC_X		0
#define SETTINGS_MENU_BK_DEC_Y		120

#define SETTINGS_MENU_RESET_ICON_W	60
#define SETTINGS_MENU_RESET_ICON_H	60
#define SETTINGS_MENU_RESET_ICON_X	((LCD_WIDTH-SETTINGS_MENU_RESET_ICON_W)/2)
#define SETTINGS_MENU_RESET_ICON_Y	20
#define SETTINGS_MENU_RESET_NO_W		40
#define SETTINGS_MENU_RESET_NO_H		40
#define SETTINGS_MENU_RESET_NO_X		44
#define SETTINGS_MENU_RESET_NO_Y		165
#define SETTINGS_MENU_RESET_YES_W		40
#define SETTINGS_MENU_RESET_YES_H		40
#define SETTINGS_MENU_RESET_YES_X		156
#define SETTINGS_MENU_RESET_YES_Y		165
#define SETTINGS_MENU_RESET_LOGO_W		90
#define SETTINGS_MENU_RESET_LOGO_H		90
#define SETTINGS_MENU_RESET_LOGO_X		((LCD_WIDTH-SETTINGS_MENU_RESET_LOGO_W)/2)
#define SETTINGS_MENU_RESET_LOGO_Y		40
#define SETTINGS_MENU_RESET_STR_W		150
#define SETTINGS_MENU_RESET_STR_H		40
#define SETTINGS_MENU_RESET_STR_X		((LCD_WIDTH-SETTINGS_MENU_RESET_STR_W)/2)
#define SETTINGS_MENU_RESET_STR_Y		95
#define SETTINGS_MENU_RESET_NOTIFY_W		194
#define SETTINGS_MENU_RESET_NOTIFY_H		40
#define SETTINGS_MENU_RESET_NOTIFY_X		((LCD_WIDTH-SETTINGS_MENU_RESET_NOTIFY_W)/2)
#define SETTINGS_MENU_RESET_NOTIFY_Y		150

//power off
#define PWR_OFF_ICON_W				115
#define PWR_OFF_ICON_H				127
#define PWR_OFF_ICON_X				((LCD_WIDTH-PWR_OFF_ICON_W)/2)
#define PWR_OFF_ICON_Y				((LCD_HEIGHT-PWR_OFF_ICON_H)/2)	
#define POW_OFF_RUNNING_ANI_W		81
#define POW_OFF_RUNNING_ANI_H		13
#define POW_OFF_RUNNING_ANI_X		((LCD_WIDTH-POW_OFF_RUNNING_ANI_W)/2)
#define POW_OFF_RUNNING_ANI_Y		212

//idle screen update event
#define SCREEN_EVENT_UPDATE_NO			0x00000000
#define SCREEN_EVENT_UPDATE_SIG			0x00000001
#define SCREEN_EVENT_UPDATE_BAT			0x00000002
#define SCREEN_EVENT_UPDATE_TIME		0x00000004
#define SCREEN_EVENT_UPDATE_DATE		0x00000008
#define SCREEN_EVENT_UPDATE_WEEK		0x00000010
#define SCREEN_EVENT_UPDATE_SPORT		0x00000020
#define SCREEN_EVENT_UPDATE_HR			0x00000040
#define SCREEN_EVENT_UPDATE_SPO2		0x00000080
#define	SCREEN_EVENT_UPDATE_BP			0x00000100
#define SCREEN_EVENT_UPDATE_SLEEP		0x00000200
#define SCREEN_EVENT_UPDATE_SOS			0x00000400
#define SCREEN_EVENT_UPDATE_WRIST		0x00000800
#define SCREEN_EVENT_UPDATE_SYNC		0x00001000
#define SCREEN_EVENT_UPDATE_FOTA		0x00002000
#define SCREEN_EVENT_UPDATE_DL			0x00004000
#define SCREEN_EVENT_UPDATE_TEMP		0x00008000
#define SCREEN_EVENT_UPDATE_NET_MODE	0x00010000

//screen ID
typedef enum
{
	SCREEN_ID_BOOTUP,
	SCREEN_ID_IDLE,
	SCREEN_ID_ALARM,
	SCREEN_ID_FIND_DEVICE,
	SCREEN_ID_HR,
	SCREEN_ID_SPO2,
	SCREEN_ID_ECG,
	SCREEN_ID_BP,
	SCREEN_ID_TEMP,
	SCREEN_ID_SOS,
	SCREEN_ID_SLEEP,
	SCREEN_ID_STEPS,
	SCREEN_ID_FALL,
	SCREEN_ID_WRIST,
	SCREEN_ID_SETTINGS,
	SCREEN_ID_GPS_TEST,
	SCREEN_ID_NB_TEST,
	SCREEN_ID_WIFI_TEST,
	SCREEN_ID_BLE_TEST,
	SCREEN_ID_NOTIFY,
	SCREEN_ID_POWEROFF,
	SCREEN_ID_SYNC,
	SCREEN_ID_FOTA,
	SCREEN_ID_DL,
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
extern void GoBackHistoryScreen(void);
extern void ScreenMsgProcess(void);
extern void ExitNotifyScreen(void);
extern void EnterFOTAScreen(void);
extern void DisplayPopUp(u8_t *message);

#ifdef __cplusplus
}
#endif

#endif/*__SCREEN_H__*/
