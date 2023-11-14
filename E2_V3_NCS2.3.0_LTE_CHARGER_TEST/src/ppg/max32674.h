/****************************************Copyright (c)************************************************
** File Name:			    max32674.h
** Descriptions:			PPG process head file
** Created By:				xie biao
** Created Date:			2021-05-19
** Modified Date:      		2021-05-19 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __MAX32674_H__
#define __MAX32674_H__

#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define PPG_CHECK_HR_MENU			60
#define PPG_CHECK_SPO2_MENU			150
#define PPG_CHECK_BPT_MENU			150

#define PPG_CHECK_HR_TIMELY			1
#define PPG_CHECK_SPO2_TIMELY		2
#define PPG_CHECK_BPT_TIMELY		3

#define PPG_HR_MAX		150
#define PPG_HR_MIN		30
#define PPG_SPO2_MAX	100
#define	PPG_SPO2_MIN	95
#define PPG_BPT_SYS_MAX	180
#define PPG_BPT_SYS_MIN	30
#define PPG_BPT_DIA_MAX	180
#define PPG_BPT_DIA_MIN	30

typedef enum
{
	PPG_DATA_HR,
	PPG_DATA_SPO2,
	PPG_DATA_BPT,
	PPG_DATA_ECG,
	PPG_DATA_MAX
}PPG_DATA_TYPE;

typedef enum
{
	ALG_MODE_HR_SPO2,
	ALG_MODE_BPT,
	ALG_MODE_ECG,
	ALG_MODE_MAX
}PPG_ALG_MODE;

typedef enum
{
	PPG_STATUS_PREPARE,
	PPG_STATUS_MEASURING,
	PPG_STATUS_MEASURE_FAIL,
	PPG_STATUS_MEASURE_OK,
	PPG_STATUS_NOTIFY,
	PPG_STATUS_MAX
}PPG_WORK_STATUS;

typedef enum
{
	BPT_STATUS_GET_CAL,		//从SH获取校验数据
	BPT_STATUS_SET_CAL,		//设置校验数据到SH
	BPT_STATUS_GET_EST,		//从SH获取血压数据
	BPT_STATUS_MAX
}PPG_BPT_WORK_STATUS;

typedef enum
{
	TRIGGER_BY_MENU			=	0x01,
	TRIGGER_BY_APP_ONE_KEY	=	0x02,
	TRIGGER_BY_APP			=	0x04,
	TRIGGER_BY_HOURLY		=	0x08,
	TRIGGER_BY_FT			=	0x10,
}PPG_TRIGGER_SOURCE;

typedef enum
{
	PPG_REC2_HR,
	PPG_REC2_SPO2,
	PPG_REC2_BPT
}PPG_REC2_DATA_TYPE;

typedef struct
{
	uint8_t systolic;
	uint8_t diastolic;
}bpt_data;

//单次测量
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t hr;
}ppg_hr_rec1_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t spo2;
}ppg_spo2_rec1_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	bpt_data bpt;
}ppg_bpt_rec1_data;

//整点测量
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hr[24];
}ppg_hr_rec2_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t spo2[24];
}ppg_spo2_rec2_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	bpt_data bpt[24];
}ppg_bpt_rec2_data;

extern bool get_bpt_ok_flag;
extern bool get_hr_ok_flag;
extern bool get_spo2_ok_flag;
extern bool ppg_bpt_is_calbraed;
extern bool ppg_bpt_cal_need_update;

extern uint8_t g_ppg_trigger;
extern uint8_t g_ppg_ver[64];
extern uint8_t ppg_test_info[256];

extern uint8_t g_hr;
extern uint8_t g_hr_menu;
extern uint8_t g_spo2;
extern uint8_t g_spo2_menu;
extern bpt_data g_bpt;
extern bpt_data g_bpt_menu;

extern PPG_WORK_STATUS g_ppg_status;

extern void PPG_init(void);
extern void PPGMsgProcess(void);

/*heart rate*/
extern void SetCurDayBptRecData(bpt_data bpt);
extern void GetCurDayBptRecData(uint8_t *databuf);
extern void SetCurDaySpo2RecData(uint8_t spo2);
extern void GetCurDaySpo2RecData(uint8_t *databuf);
extern void SetCurDayHrRecData(uint8_t hr);
extern void GetCurDayHrRecData(uint8_t *databuf);
extern void StartPPG(PPG_DATA_TYPE data_type, PPG_TRIGGER_SOURCE trigger_type);

#endif/*__MAX32674_H__*/
