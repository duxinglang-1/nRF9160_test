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
#include <zephyr.h>
#include <device.h>

#define PPG_CHECK_SPO2_TIMELY		4
#define PPG_CHECK_BPT_TIMELY		3

#define PPG_HR_MAX		150
#define PPG_HR_MIN		30
#define PPG_SPO2_MAX	100
#define	PPG_SPO2_MIN	80
#define PPG_BPT_SYS_MAX	180
#define PPG_BPT_SYS_MIN	30
#define PPG_BPT_DIA_MAX	180
#define PPG_BPT_DIA_MIN	30

typedef enum
{
	ALG_MODE_HR_SPO2,
	ALG_MODE_BPT,
	ALG_MODE_ECG,
	ALG_MODE_MAX
}PPG_ALG_MODE;

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
}PPG_TARGGER_SOUCE;

typedef enum
{
	PPG_REC2_HR,
	PPG_REC2_SPO2,
	PPG_REC2_BPT
}PPG_REC2_DATA_TYPE;

typedef struct
{
	u8_t systolic;
	u8_t diastolic;
}bpt_data;

//单次测量
typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t hour;
	u8_t min;
	u8_t sec;
	u8_t hr;
}ppg_hr_rec1_data;

typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t hour;
	u8_t min;
	u8_t sec;
	u8_t spo2;
}ppg_spo2_rec1_data;

typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t hour;
	u8_t min;
	u8_t sec;
	bpt_data bpt;
}ppg_bpt_rec1_data;

//整点测量
typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t hr[24];
}ppg_hr_rec2_data;

typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t spo2[24];
}ppg_spo2_rec2_data;

typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	bpt_data bpt[24];
}ppg_bpt_rec2_data;

extern bool get_bpt_ok_flag;
extern bool get_hr_ok_flag;
extern bool get_spo2_ok_flag;

extern u8_t g_ppg_trigger;
extern u8_t g_ppg_ver[64];

extern u8_t g_hr;
extern u8_t g_hr_timing;
extern u8_t g_spo2;
extern u8_t g_spo2_timing;
extern bpt_data g_bpt;
extern bpt_data g_bpt_timing;

extern void PPG_init(void);
extern void PPGMsgProcess(void);

/*heart rate*/
extern void SetCurDayBptRecData(bpt_data bpt);
extern void GetCurDayBptRecData(u8_t *databuf);
extern void SetCurDaySpo2RecData(u8_t spo2);
extern void GetCurDaySpo2RecData(u8_t *databuf);
extern void SetCurDayHrRecData(u8_t hr);
extern void GetCurDayHrRecData(u8_t *databuf);
extern void GetHeartRate(u8_t *HR);
extern void APPStartHrSpo2(void);
extern void APPStartBpt(void);
extern void APPStartEcg(void);
extern void TimerStartHrSpo2(void);
extern void TimerStartBpt(void);
extern void TimerStartEcg(void);

#endif/*__MAX32674_H__*/
