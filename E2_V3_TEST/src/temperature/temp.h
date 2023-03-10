/****************************************Copyright (c)************************************************
** File Name:			    temp.h
** Descriptions:			temperature message process head file
** Created By:				xie biao
** Created Date:			2021-12-24
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __TEMP_H__
#define __TEMP_H__

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <stdio.h>

//sensor mode
#define TEMP_GXTS04
//#define TEMP_MAX30208
//#define TEMP_CT1711

//sensor interface type
#define TEMP_IF_I2C
#define TEMP_IF_SINGLE_LINE

#define TEMP_CHECK_MENU				60
#define TEMP_CHECK_TIMELY			2
#ifndef CONFIG_PPG_SUPPORT
#define PPG_CHECK_HR_TIMELY			0
#define PPG_CHECK_SPO2_TIMELY		0
#define PPG_CHECK_BPT_TIMELY		0
#endif

#define TEMP_MAX			420
#define TEMP_MIN			320

//#define TEMP_DEBUG

typedef enum
{
	TEMP_STATUS_PREPARE,
	TEMP_STATUS_MEASURING,
	TEMP_STATUS_MEASURE_FAIL,
	TEMP_STATUS_MEASURE_OK,
	TEMP_STATUS_NOTIFY,
	TEMP_STATUS_MAX
}TEMP_WORK_STATUS;

//sensor trigger type
typedef enum
{
	TEMP_TRIGGER_BY_MENU	=	0x01,
	TEMP_TRIGGER_BY_APP		=	0x02,
	TEMP_TRIGGER_BY_HOURLY	=	0x04,
	TEMP_TRIGGER_BY_FT		=	0x08,
}TEMP_TARGGER_SOUCE;

//单次测量
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint16_t deca_temp;	//实际温度放大10倍(36.5*10)
}temp_rec1_data;

//整点测量
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint16_t deca_temp[24];	//实际温度放大10倍(36.5*10)
}temp_rec2_data;

extern bool get_temp_ok_flag;

extern float g_temp_skin;
extern float g_temp_body;
extern float g_temp_menu;

extern TEMP_WORK_STATUS g_temp_status;

extern void SetCurDayTempRecData(float data);
extern void GetCurDayTempRecData(uint16_t *databuf);
#endif/*__TEMP_H__*/

