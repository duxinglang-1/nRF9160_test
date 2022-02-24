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
	TRIGGER_BY_MENU=0x01,
	TRIGGER_BY_APP_ONE_KEY=0x02,
	TRIGGER_BY_APP=0x04,
	TRIGGER_BY_HOURLY=0x40
}PPG_TARGGER_SOUCE;

extern u8_t g_ppg_trigger;
extern u16_t g_hr;
extern u16_t g_spo2;
extern u16_t g_bp_systolic;		//收缩压
extern u16_t g_bp_diastolic;	//舒张压

extern void PPG_init(void);
extern void PPGMsgProcess(void);

/*heart rate*/
extern void GetHeartRate(u8_t *HR);
extern void APPStartHrSpo2(void);
extern void APPStartBpt(void);
extern void APPStartEcg(void);
extern void APPStartPPG(void);

#endif/*__MAX32674_H__*/
