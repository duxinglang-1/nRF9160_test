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

//#define UI_STYLE_HEALTH_BAR

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
#define IDLE_TIME_W		153
#define IDLE_TIME_H		60
#define IDLE_TIME_X		((LCD_WIDTH-IDLE_TIME_W)/2)
#define IDLE_TIME_Y		50
#define IDLE_TIME_NUM_W		34
#define IDLE_TIME_NUM_H		60
#define IDLE_TIME_COLON_W	17
#define IDLE_TIME_COLON_H	60
#define IDLE_DATE_MON_W		90
#define IDLE_DATE_MON_H		26
#define IDLE_DATE_MON_X		141
#define IDLE_DATE_MON_Y		106
#define IDLE_DATE_DAY_W		22
#define IDLE_DATE_DAY_H		20
#define IDLE_DATE_DAY_X		114
#define IDLE_DATE_DAY_Y		109
#define IDLE_DATE_NUM_W		12
#define IDLE_DATE_NUM_H		20
#define IDLE_WEEK_W			52
#define IDLE_WEEK_H			26
#define IDLE_WEEK_X			63
#define IDLE_WEEK_Y			106

//ble
#define IDLE_BLE_W		50
#define IDLE_BLE_H		30
#define IDLE_BLE_X		((LCD_WIDTH-IDLE_BLE_W)/2)
#define IDLE_BLE_Y		5

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

//idle circle bg
#define IDLE_CIRCLE_BG_W	192
#define IDLE_CIRCLE_BG_H	95
#define IDLE_CIRCLE_BG_X	24
#define IDLE_CIRCLE_BG_Y	140

//idle hr
#define IDLE_HR_BG_W		66
#define IDLE_HR_BG_H		66
#define IDLE_HR_BG_X		21
#define IDLE_HR_BG_Y		134
#define IDLE_HR_ICON_W		22
#define IDLE_HR_ICON_H		20
#define IDLE_HR_ICON_X		42
#define IDLE_HR_ICON_Y		142
#define IDLE_HR_STR_W		50
#define IDLE_HR_STR_H		20
#define IDLE_HR_STR_X		(IDLE_HR_BG_X+(IDLE_HR_BG_W-IDLE_HR_STR_W)/2)
#define IDLE_HR_STR_Y		165
#define IDLE_HR_NUM_W		12
#define IDLE_HR_NUM_H		20

//idle step
#define IDLE_STEPS_BG_W		66
#define IDLE_STEPS_BG_H		66
#define IDLE_STEPS_BG_X		87
#define IDLE_STEPS_BG_Y		167
#define IDLE_STEPS_ICON_W	14
#define IDLE_STEPS_ICON_H	20
#define IDLE_STEPS_ICON_X	113
#define IDLE_STEPS_ICON_Y	171
#define IDLE_STEPS_STR_W	60
#define IDLE_STEPS_STR_H	20
#define IDLE_STEPS_STR_X	(IDLE_STEPS_BG_X+(IDLE_STEPS_BG_W-IDLE_STEPS_STR_W)/2)
#define IDLE_STEPS_STR_Y	192
#define IDLE_STEPS_NUM_W	12
#define IDLE_STEPS_NUM_H	20

//idle temp
#define IDLE_TEMP_BG_W		66
#define IDLE_TEMP_BG_H		66
#define IDLE_TEMP_BG_X		153
#define IDLE_TEMP_BG_Y		134
#define IDLE_TEMP_ICON_W	17
#define IDLE_TEMP_ICON_H	20
#define IDLE_TEMP_ICON_X	177
#define IDLE_TEMP_ICON_Y	142
#define IDLE_TEMP_STR_W		50
#define IDLE_TEMP_STR_H		20
#define IDLE_TEMP_STR_X		(IDLE_TEMP_BG_X+(IDLE_TEMP_BG_W-IDLE_TEMP_STR_W)/2)
#define IDLE_TEMP_STR_Y		165
#define IDLE_TEMP_NUM_W		12
#define IDLE_TEMP_NUM_H		20
#define IDLE_TEMP_DOT_W		6
#define IDLE_TEMP_DOT_H		20


//notify
#define NOTIFY_IMG_MAX_COUNT	10
#define NOTIFY_TEXT_MAX_LEN		512
#define NOTIFY_TIMER_INTERVAL	5

//sport
#define IMU_SEP_LINE_W				99
#define IMU_SEP_LINE_H				1
#define IMU_SEP_LINE_X				((LCD_WIDTH-IMU_SEP_LINE_W)/2)
#define IMU_SEP_LINE_Y				140
#define IMU_NUM_W					22
#define IMU_NUM_H					38
//steps
#define IMU_STEP_ICON_W				28
#define IMU_STEP_ICON_H				38
#define IMU_STEP_ICON_X				40
#define IMU_STEP_ICON_Y				63
#define IMU_STEP_UNIT_W				35
#define IMU_STEP_UNIT_H				15
#define IMU_STEP_UNIT_X				184
#define IMU_STEP_UNIT_Y				82
#define IMU_STEP_STR_W				105
#define IMU_STEP_STR_H				38
#define IMU_STEP_STR_X				77
#define IMU_STEP_STR_Y				63
//calorie
#define IMU_CAL_ICON_W				28
#define IMU_CAL_ICON_H				38
#define IMU_CAL_ICON_X				40
#define IMU_CAL_ICON_Y				113
#define IMU_CAL_UNIT_W				29
#define IMU_CAL_UNIT_H				14
#define IMU_CAL_UNIT_X				163
#define IMU_CAL_UNIT_Y				134
#define IMU_CAL_STR_W				84
#define IMU_CAL_STR_H				38
#define IMU_CAL_STR_X				77
#define IMU_CAL_STR_Y				113
//sleep
#define IMU_SLEEP_ICON_W			30
#define IMU_SLEEP_ICON_H			33
#define IMU_SLEEP_ICON_X			40
#define IMU_SLEEP_ICON_Y			166
#define IMU_SLEEP_H_UNIT_W			9
#define IMU_SLEEP_H_UNIT_H			14
#define IMU_SLEEP_H_UNIT_X			122
#define IMU_SLEEP_H_UNIT_Y			185
#define IMU_SLEEP_M_UNIT_W			13
#define IMU_SLEEP_M_UNIT_H			14
#define IMU_SLEEP_M_UNIT_X			177
#define IMU_SLEEP_M_UNIT_Y			185
#define IMU_SLEEP_H_STR_W			43
#define IMU_SLEEP_H_STR_H			38
#define IMU_SLEEP_H_STR_X			77
#define IMU_SLEEP_H_STR_Y			163
#define IMU_SLEEP_M_STR_W			43
#define IMU_SLEEP_M_STR_H			38
#define IMU_SLEEP_M_STR_X			132
#define IMU_SLEEP_M_STR_Y			163

//distance
#define IMU_DIS_ICON_W				22
#define IMU_DIS_ICON_H				28
#define IMU_DIS_ICON_X				148
#define IMU_DIS_ICON_Y				145
#define IMU_DIS_UNIT_W				21
#define IMU_DIS_UNIT_H				14
#define IMU_DIS_UNIT_X				182
#define IMU_DIS_UNIT_Y				186
#define IMU_DIS_STR_W				56
#define IMU_DIS_STR_H				28
#define IMU_DIS_STR_X				126
#define IMU_DIS_STR_Y				178

//sleep
//total
#define SLEEP_SEP_LINE_W			123
#define SLEEP_SEP_LINE_H			1
#define SLEEP_SEP_LINE_X			((LCD_WIDTH-SLEEP_SEP_LINE_W)/2)
#define SLEEP_SEP_LINE_Y			140
#define SLEEP_TOTAL_ICON_W			37
#define SLEEP_TOTAL_ICON_H			40
#define SLEEP_TOTAL_ICON_X			((LCD_WIDTH-SLEEP_TOTAL_ICON_W)/2)
#define SLEEP_TOTAL_ICON_Y			56
#define SLEEP_TOTAL_UNIT_HR_W		13
#define SLEEP_TOTAL_UNIT_HR_H		20
#define SLEEP_TOTAL_UNIT_HR_X		104
#define SLEEP_TOTAL_UNIT_HR_Y		116
#define SLEEP_TOTAL_UNIT_MIN_W		19
#define SLEEP_TOTAL_UNIT_MIN_H		20
#define SLEEP_TOTAL_UNIT_MIN_X		163
#define SLEEP_TOTAL_UNIT_MIN_Y		116
#define SLEEP_TOTAL_NUM_W			22
#define SLEEP_TOTAL_NUM_H			38
#define SLEEP_TOTAL_STR_HR_W		44
#define SLEEP_TOTAL_STR_HR_H		38
#define SLEEP_TOTAL_STR_HR_X		59
#define SLEEP_TOTAL_STR_HR_Y		100
#define SLEEP_TOTAL_STR_MIN_W		44		
#define SLEEP_TOTAL_STR_MIN_H		38
#define SLEEP_TOTAL_STR_MIN_X		118
#define SLEEP_TOTAL_STR_MIN_Y		100
//light sleep
#define SLEEP_LIGHT_ICON_W			28
#define SLEEP_LIGHT_ICON_H			28
#define SLEEP_LIGHT_ICON_X			151
#define SLEEP_LIGHT_ICON_Y			145
#define SLEEP_LIGHT_UNIT_HR_W		9
#define SLEEP_LIGHT_UNIT_HR_H		14
#define SLEEP_LIGHT_UNIT_HR_X		154
#define SLEEP_LIGHT_UNIT_HR_Y		187
#define SLEEP_LIGHT_UNIT_MIN_W		13
#define SLEEP_LIGHT_UNIT_MIN_H		14
#define SLEEP_LIGHT_UNIT_MIN_X		189
#define SLEEP_LIGHT_UNIT_MIN_Y		187
#define SLEEP_LIGHT_NUM_W			14
#define SLEEP_LIGHT_NUM_H			24
#define SLEEP_LIGHT_STR_HR_W		28
#define SLEEP_LIGHT_STR_HR_H		24
#define SLEEP_LIGHT_STR_HR_X		127
#define SLEEP_LIGHT_STR_HR_Y		178
#define SLEEP_LIGHT_STR_MIN_W		28		
#define SLEEP_LIGHT_STR_MIN_H		24
#define SLEEP_LIGHT_STR_MIN_X		162
#define SLEEP_LIGHT_STR_MIN_Y		178
//deep sleep
#define SLEEP_DEEP_ICON_W			28
#define SLEEP_DEEP_ICON_H			28
#define SLEEP_DEEP_ICON_X			61
#define SLEEP_DEEP_ICON_Y			145
#define SLEEP_DEEP_UNIT_HR_W		9
#define SLEEP_DEEP_UNIT_HR_H		14
#define SLEEP_DEEP_UNIT_HR_X		65
#define SLEEP_DEEP_UNIT_HR_Y		187
#define SLEEP_DEEP_UNIT_MIN_W		13
#define SLEEP_DEEP_UNIT_MIN_H		14
#define SLEEP_DEEP_UNIT_MIN_X		100
#define SLEEP_DEEP_UNIT_MIN_Y		187
#define SLEEP_DEEP_NUM_W			14
#define SLEEP_DEEP_NUM_H			24
#define SLEEP_DEEP_STR_HR_W			28
#define SLEEP_DEEP_STR_HR_H			24
#define SLEEP_DEEP_STR_HR_X			38
#define SLEEP_DEEP_STR_HR_Y			178
#define SLEEP_DEEP_STR_MIN_W		28		
#define SLEEP_DEEP_STR_MIN_H		24
#define SLEEP_DEEP_STR_MIN_X		73
#define SLEEP_DEEP_STR_MIN_Y		178


#ifdef UI_STYLE_HEALTH_BAR
//heart rate
#define HR_ICON_W					34
#define HR_ICON_H					31
#define HR_ICON_X					42
#define HR_ICON_Y					35
#define HR_UNIT_W					45
#define HR_UNIT_H					22
#define HR_UNIT_X					151
#define HR_UNIT_Y					42
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
#define HR_UP_ARRAW_X				43
#define HR_UP_ARRAW_Y				183
#define HR_UP_NUM_W					39				
#define HR_UP_NUM_H					27
#define HR_UP_NUM_X					60
#define HR_UP_NUM_Y					178
#define HR_DOWN_ARRAW_W				14
#define HR_DOWN_ARRAW_H				17
#define HR_DOWN_ARRAW_X				131
#define HR_DOWN_ARRAW_Y				183
#define HR_DOWN_NUM_W				39				
#define HR_DOWN_NUM_H				27
#define HR_DOWN_NUM_X				148
#define HR_DOWN_NUM_Y				178
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
#define BP_NUM_W					92
#define BP_NUM_H					31
#define BP_NUM_X					75
#define BP_NUM_Y					40
#define BP_UNIT_X					166
#define BP_UNIT_Y					51
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
#define TEMP_ICON_W					34
#define TEMP_ICON_H					42
#define TEMP_ICON_X					55
#define TEMP_ICON_Y					31
#define TEMP_NUM_W					73
#define TEMP_NUM_H					31
#define TEMP_NUM_X					97
#define TEMP_NUM_Y					36
#define TEMP_UINT_X					180
#define TEMP_UINT_Y					40
#define TEMP_BG_W					192
#define TEMP_BG_H					85
#define TEMP_BG_X					((LCD_WIDTH-TEMP_BG_W)/2)
#define TEMP_BG_Y					80
#define TEMP_UP_ARRAW_W				14
#define TEMP_UP_ARRAW_H				17
#define TEMP_UP_ARRAW_X				43
#define TEMP_UP_ARRAW_Y				183
#define TEMP_UP_NUM_W				51
#define TEMP_UP_NUM_H				17
#define TEMP_UP_NUM_X				60
#define TEMP_UP_NUM_Y				183
#define TEMP_DOWN_ARRAW_W			14
#define TEMP_DOWN_ARRAW_H			17
#define TEMP_DOWN_ARRAW_X			133
#define TEMP_DOWN_ARRAW_Y			183
#define TEMP_DOWN_NUM_W				49
#define TEMP_DOWN_NUM_H				17
#define TEMP_DOWN_NUM_X				148
#define TEMP_DOWN_NUM_Y				183
#define TEMP_REC_DATA_X				25
#define TEMP_REC_DATA_Y				146
#define TEMP_REC_DATA_W				4
#define TEMP_REC_DATA_H				58
#define TEMP_REC_DATA_OFFSET_X		7
#else/*UI_STYLE_HEALTH_BAR*/
//heart rate
#define HR_ICON_W					66
#define HR_ICON_H					60
#define HR_ICON_X					((LCD_WIDTH-HR_ICON_W)/2)
#define HR_ICON_Y					66
#define HR_NOTIFY_W					LCD_WIDTH
#define HR_NOTIFY_H					30
#define HR_NOTIFY_X					((LCD_WIDTH-HR_NOTIFY_W)/2)
#define HR_NOTIFY_Y					140
#define HR_NUM_W					24
#define HR_NUM_H					42
#define HR_STR_W					72
#define HR_STR_H					42
#define HR_STR_X					((LCD_WIDTH-HR_STR_W)/2)
#define HR_STR_Y					133
#define HR_UNIT_W					45
#define HR_UNIT_H					22
#define HR_UNIT_X					146
#define HR_UNIT_Y					152
#define HR_BG_W						LCD_WIDTH
#define HR_BG_H						180
#define HR_BG_X						((LCD_WIDTH-HR_BG_W)/2)
#define HR_BG_Y						60
#define HR_UP_ARRAW_W				14				
#define HR_UP_ARRAW_H				17
#define HR_UP_ARRAW_X				50
#define HR_UP_ARRAW_Y				188
#define HR_UP_NUM_W					14				
#define HR_UP_NUM_H					24
#define HR_UP_STR_W					42
#define HR_UP_STR_H					24
#define HR_UP_STR_X					67
#define HR_UP_STR_Y					184
#define HR_DOWN_ARRAW_W				14
#define HR_DOWN_ARRAW_H				17
#define HR_DOWN_ARRAW_X				146
#define HR_DOWN_ARRAW_Y				188
#define HR_DOWN_NUM_W				14				
#define HR_DOWN_NUM_H				24
#define HR_DOWN_STR_W				42				
#define HR_DOWN_STR_H				24
#define HR_DOWN_STR_X				163
#define HR_DOWN_STR_Y				184

//spo2
#define SPO2_ICON_W					44
#define SPO2_ICON_H					60
#define SPO2_ICON_X					((LCD_WIDTH-SPO2_ICON_W)/2)
#define SPO2_ICON_Y					66
#define SPO2_NOTIFY_W				LCD_WIDTH
#define SPO2_NOTIFY_H				30
#define SPO2_NOTIFY_X				((LCD_WIDTH-SPO2_NOTIFY_W)/2)
#define SPO2_NOTIFY_Y				140
#define SPO2_NUM_W					24
#define SPO2_NUM_H					42
#define SPO2_PERC_W					39
#define SPO2_PERC_H					42
#define SPO2_STR_W					72
#define SPO2_STR_H					42
#define SPO2_STR_X					((LCD_WIDTH-SPO2_STR_W)/2)
#define SPO2_STR_Y					133
#define SPO2_UNIT_W					45
#define SPO2_UNIT_H					22
#define SPO2_UNIT_X					((LCD_WIDTH-SPO2_UNIT_W)/2)
#define SPO2_UNIT_Y					212
#define SPO2_BG_W					192
#define SPO2_BG_H					87
#define SPO2_BG_X					((LCD_WIDTH-SPO2_BG_W)/2)
#define SPO2_BG_Y					80
#define SPO2_UP_ARRAW_W				14
#define SPO2_UP_ARRAW_H				17
#define SPO2_UP_ARRAW_X				47
#define SPO2_UP_ARRAW_Y				188
#define SPO2_UP_NUM_W				14
#define SPO2_UP_NUM_H				24
#define SPO2_UP_PERC_W				23
#define SPO2_UP_PERC_H				24
#define SPO2_UP_STR_W				52
#define SPO2_UP_STR_H				24
#define SPO2_UP_STR_X				64
#define SPO2_UP_STR_Y				184
#define SPO2_DOWN_ARRAW_W			14
#define SPO2_DOWN_ARRAW_H			17
#define SPO2_DOWN_ARRAW_X			126
#define SPO2_DOWN_ARRAW_Y			188
#define SPO2_DOWN_NUM_W				14
#define SPO2_DOWN_NUM_H				24
#define SPO2_DOWN_PERC_W			23
#define SPO2_DOWN_PERC_H			24
#define SPO2_DOWN_STR_W				52
#define SPO2_DOWN_STR_H				24
#define SPO2_DOWN_STR_X				143
#define SPO2_DOWN_STR_Y				184

//blood pressure
#define BP_ICON_W					33
#define BP_ICON_H					60
#define BP_ICON_X					((LCD_WIDTH-BP_ICON_W)/2)
#define BP_ICON_Y					66
#define BP_NOTIFY_W					LCD_WIDTH
#define BP_NOTIFY_H					30
#define BP_NOTIFY_X					((LCD_WIDTH-BP_NOTIFY_W)/2)
#define BP_NOTIFY_Y					140
#define BP_NUM_W					24
#define BP_NUM_H					42
#define BP_SLASH_W					17
#define BP_SLASH_H					45
#define BP_STR_W					162
#define BP_STR_H					45
#define BP_STR_X					((LCD_WIDTH-BP_STR_W)/2)
#define BP_STR_Y					160
#define BP_UNIT_W					39
#define BP_UNIT_H					14
#define BP_UNIT_X					((LCD_WIDTH-BP_UNIT_W)/2)
#define BP_UNIT_Y					208
#define BP_BG_W						208
#define BP_BG_H						87
#define BP_BG_X						((LCD_WIDTH-BP_BG_W)/2)
#define BP_BG_Y						80
#define BP_UP_ARRAW_W				17
#define BP_UP_ARRAW_H				17
#define BP_UP_ARRAW_X				40
#define BP_UP_ARRAW_Y				183
#define BP_UP_NUM_W					9
#define BP_UP_NUM_H					16
#define BP_UP_SLASH_W				7
#define BP_UP_SLASH_H				17
#define BP_UP_STR_W					62
#define BP_UP_STR_H					17
#define BP_UP_STR_X					57
#define BP_UP_STR_Y					183
#define BP_DOWN_ARRAW_W				17
#define BP_DOWN_ARRAW_H				17
#define BP_DOWN_ARRAW_X				131
#define BP_DOWN_ARRAW_Y				183
#define BP_DOWN_NUM_W				9
#define BP_DOWN_NUM_H				16
#define BP_DOWN_SLASH_W				7
#define BP_DOWN_SLASH_H				17
#define BP_DOWN_STR_W				62
#define BP_DOWN_STR_H				17
#define BP_DOWN_STR_X				148
#define BP_DOWN_STR_Y				183

//temperature
#define TEMP_ICON_W					49
#define TEMP_ICON_H					60
#define TEMP_ICON_X					((LCD_WIDTH-TEMP_ICON_W)/2)
#define TEMP_ICON_Y					66
#define TEMP_NOTIFY_W				LCD_WIDTH
#define TEMP_NOTIFY_H				30
#define TEMP_NOTIFY_X				((LCD_WIDTH-TEMP_NOTIFY_W)/2)
#define TEMP_NOTIFY_Y				140
#define TEMP_NUM_W					24
#define TEMP_NUM_H					42
#define TEMP_DOT_W					12
#define TEMP_DOT_H					42
#define TEMP_STR_W					110
#define TEMP_STR_H					48
#define TEMP_STR_X					((LCD_WIDTH-TEMP_STR_W)/2)
#define TEMP_STR_Y					133
#define TEMP_UNIT_W					23
#define TEMP_UNIT_H					38
#define TEMP_UNIT_X					152
#define TEMP_UNIT_Y					136
#define TEMP_BG_W					192
#define TEMP_BG_H					85
#define TEMP_BG_X					((LCD_WIDTH-TEMP_BG_W)/2)
#define TEMP_BG_Y					80
#define TEMP_UP_ARRAW_W				14
#define TEMP_UP_ARRAW_H				17
#define TEMP_UP_ARRAW_X				47
#define TEMP_UP_ARRAW_Y				188
#define TEMP_UP_NUM_W				14
#define TEMP_UP_NUM_H				24
#define TEMP_UP_DOT_W				7
#define TEMP_UP_DOT_H				24
#define TEMP_UP_STR_W				51
#define TEMP_UP_STR_H				17
#define TEMP_UP_STR_X				64
#define TEMP_UP_STR_Y				184
#define TEMP_DOWN_ARRAW_W			14
#define TEMP_DOWN_ARRAW_H			17
#define TEMP_DOWN_ARRAW_X			128
#define TEMP_DOWN_ARRAW_Y			188
#define TEMP_DOWN_NUM_W				14
#define TEMP_DOWN_NUM_H				24
#define TEMP_DOWN_DOT_W				7
#define TEMP_DOWN_DOT_H				24
#define TEMP_DOWN_STR_W				49
#define TEMP_DOWN_STR_H				17
#define TEMP_DOWN_STR_X				145
#define TEMP_DOWN_STR_Y				184
#endif/*UI_STYLE_HEALTH_BAR*/

//settings
#define SETTINGS_MAIN_MENU_MAX_PER_PG	3
#define SETTINGS_SUB_MENU_MAX_PER_PG	3

#define SETTINGS_MENU_BG_W			170
#define SETTINGS_MENU_BG_H			50
#define SETTINGS_MENU_BG_X			((LCD_WIDTH-SETTINGS_MENU_BG_W)/2)
#define SETTINGS_MENU_BG_Y			40
#define SETTINGS_MENU_BG_OFFSET_Y	5
#define SETTINGS_MENU_STR_OFFSET_X	5			
#define SETTINGS_MENU_STR_OFFSET_Y	15

#define SETTINGS_MEUN_PAGE_DOT_W	6
#define SETTINGS_MEUN_PAGE_DOT_H	22
#define SETTINGS_MEUN_PAGE_DOT_X	226
#define SETTINGS_MEUN_PAGE_DOT_Y	((LCD_HEIGHT-SETTINGS_MEUN_PAGE_DOT_H)/2)

#define SETTINGS_MENU_TEMP_UNIT_W	35
#define SETTINGS_MENU_TEMP_UNIT_H	36
#define SETTINGS_MENU_TEMP_UNIT_X	161
#define SETTINGS_MENU_TEMP_UNIT_Y	157

#define SETTINGS_MENU_QR_ICON_W		25
#define SETTINGS_MENU_QR_ICON_H		25
#define SETTINGS_MENU_QR_ICON_X		171
#define SETTINGS_MENU_QR_ICON_Y		108

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
#define PWR_OFF_ICON_W				127
#define PWR_OFF_ICON_H				127
#define PWR_OFF_ICON_X				((LCD_WIDTH-PWR_OFF_ICON_W)/2)
#define PWR_OFF_ICON_Y				((LCD_HEIGHT-PWR_OFF_ICON_H)/2)	
#define POW_OFF_RUNNING_ANI_W		81
#define POW_OFF_RUNNING_ANI_H		13
#define POW_OFF_RUNNING_ANI_X		((LCD_WIDTH-POW_OFF_RUNNING_ANI_W)/2)
#define POW_OFF_RUNNING_ANI_Y		212

//Notify
#define NOTIFY_RECT_W		240
#define NOTIFY_RECT_H		240
#define NOTIFY_RECT_X		((LCD_WIDTH-NOTIFY_RECT_W)/2)
#define NOTIFY_RECT_Y		((LCD_HEIGHT-NOTIFY_RECT_H)/2)

//fall
#define FALL_NOTIFY_STR_W	160
#define FALL_NOTIFY_STR_H	80
#define FALL_NOTIFY_STR_X	((LCD_WIDTH-FALL_NOTIFY_STR_W)/2)
#define FALL_NOTIFY_STR_Y	((LCD_HEIGHT-FALL_NOTIFY_STR_H)/2)

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
#define SCREEN_EVENT_UPDATE_POP_IMG		0x00020000
#define SCREEN_EVENT_UPDATE_POP_STR		0x00040000
#define SCREEN_EVENT_UPDATE_BLE			0x00080000


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
	SCREEN_ID_FACTORY_TEST,
	SCREEN_ID_DEVICE_INFOR,
	SCREEN_ID_MAX
}SCREEN_ID_ENUM;

typedef enum
{
	SCREEN_ACTION_NO,
	SCREEN_ACTION_ENTER,
	SCREEN_ACTION_UPDATE,
	SCREEN_ACTION_EXIT,
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
	uint32_t para;
}screen_msg;

typedef enum
{
	NOTIFY_TYPE_POPUP,
	NOTIFY_TYPE_CONFIRM,
	NOTIFY_TYPE_NOTIFY,
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
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	uint8_t img_count;
	uint32_t img[NOTIFY_IMG_MAX_COUNT];
	uint16_t text[NOTIFY_TEXT_MAX_LEN+1];
}notify_infor;

extern SCREEN_ID_ENUM screen_id;
extern SCREEN_ID_ENUM history_screen_id;

extern screen_msg scr_msg[SCREEN_ID_MAX];
extern notify_infor notify_msg;

extern void ShowBootUpLogo(void);
extern void EnterIdleScreen(void);
extern void EnterAlarmScreen(void);
extern void EnterFindDeviceScreen(void);
#ifdef CONFIG_WIFI_SUPPORT
extern void EnterWifiTestScreen(void);
#endif
extern void EnterGPSTestScreen(void);
extern void EnterNBTestScreen(void);
extern void EnterPoweroffScreen(void);
extern void GoBackHistoryScreen(void);
extern void ScreenMsgProcess(void);
extern void ExitNotify(void);
extern void EnterFOTAScreen(void);
extern void DisplayPopUp(notify_infor infor);
#ifdef CONFIG_QRCODE_SUPPORT
extern void EnterDeviceScreen(void);
#endif
extern void EnterFallScreen(void);

#ifdef __cplusplus
}
#endif

#endif/*__SCREEN_H__*/
