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

#define PPG_TRIGGER_BY_MENU			0x01
#define	PPG_TRIGGER_BY_APP_ONE_KEY	0x02
#define	PPG_TRIGGER_BY_APP_HR		0x04
#define	PPG_TRIGGER_BY_HOURLY		0x08

extern u8_t g_ppg_trigger;
extern u16_t g_hr;
extern u16_t g_spo2;

extern void PPG_init(void);
extern void PPGMsgProcess(void);

/*heart rate*/
extern void GetHeartRate(u8_t *HR);
extern void APPStartPPG(void);

#endif/*__MAX32674_H__*/
