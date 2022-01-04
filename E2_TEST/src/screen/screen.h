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

//logo
#define PWRON_LOGO_W	COL
#define PWRON_LOGO_H	ROW
#define PWRON_LOGO_X	0
#define PWRON_LOGO_Y	0

//date&time
#define IDLE_TIME_SHOW_X	0
#ifdef FONT_32
#define IDLE_TIME_SHOW_Y	54
#elif defined(FONT_24)
#define IDLE_TIME_SHOW_Y	64
#elif defined(FONT_16)
#define IDLE_TIME_SHOW_Y	74
#endif
#define IDLE_DATE_SHOW_X	0
#define IDLE_DATE_SHOW_Y	(IDLE_TIME_SHOW_Y+system_font)
#define IDLE_WEEK_SHOW_X	0
#define IDLE_WEEK_SHOW_Y	(IDLE_DATE_SHOW_Y+system_font)

//ble
#define IDLE_BLE_X		45
#define IDLE_BLE_Y		0
#define IDLE_BLE_W		7
#define IDLE_BLE_H		16

//battery soc
#define BAT_PS_OFFSET_H		5
#define BAT_POSITIVE_X		85
#define BAT_POSITIVE_Y		30
#define BAT_POSITIVE_W		10
#define BAT_POSITIVE_H		10
#define BAT_SUBJECT_X		(BAT_POSITIVE_X+BAT_POSITIVE_W)
#define BAT_SUBJECT_Y		(BAT_POSITIVE_Y-BAT_PS_OFFSET_H)
#define BAT_SUBJECT_W		60
#define BAT_SUBJECT_H		(BAT_POSITIVE_H+2*BAT_PS_OFFSET_H)

//NB signal
#define NB_SIGNAL_FILL_RECT_OFFSET_X	10
#define NB_SIGNAL_FILL_RECT_W			6
#define NB_SIGNAL_FILL_RECT_H			6
#define NB_SIGNAL_FILL_RECT_X			(LCD_WIDTH-(4*NB_SIGNAL_FILL_RECT_W+3*NB_SIGNAL_FILL_RECT_OFFSET_X))/2
#define NB_SIGNAL_FILL_RECT_Y			8	
	

//sport
#define IMU_STEPS_SHOW_X	15
#define IMU_STEPS_SHOW_Y	160
#define IMU_STEPS_SHOW_W	210
#define IMU_STEPS_SHOW_H	20

//health
#define PPG_DATA_SHOW_X	40
#define PPG_DATA_SHOW_Y	190
#define PPG_DATA_SHOW_W	160
#define PPG_DATA_SHOW_H	20

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
#define SCREEN_EVENT_UPDATE_FOTA		0x00000400
#define SCREEN_EVENT_UPDATE_DL			0x00000800

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
	SCREEN_ID_FALL,
	SCREEN_ID_WRIST,
	SCREEN_ID_SETTINGS,
	SCREEN_ID_GPS_TEST,
	SCREEN_ID_NB_TEST,
	SCREEN_ID_NOTIFY,
	SCREEN_ID_POWEROFF,
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
