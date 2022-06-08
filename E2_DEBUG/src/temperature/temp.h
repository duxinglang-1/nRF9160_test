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

#define TEMP_CHECK_TIMELY	2

//#define TEMP_DEBUG

//sensor trigger type
typedef enum
{
	TEMP_TRIGGER_BY_MENU	=	0x01,
	TEMP_TRIGGER_BY_APP		=	0x02,
	TEMP_TRIGGER_BY_HOURLY	=	0x04,
}TEMP_TARGGER_SOUCE;

//单次测量
typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t hour;
	u8_t min;
	u8_t sec;
	u16_t deca_temp;	//实际温度放大10倍(36.5*10)
}temp_rec1_data;

//整点测量
typedef struct
{
	u16_t year;
	u8_t month;
	u8_t day;
	u16_t deca_temp[24];	//实际温度放大10倍(36.5*10)
}temp_rec2_data;

extern bool get_temp_ok_flag;

extern float g_temp_skin;
extern float g_temp_body;
extern float g_temp_timing;

extern void SetCurDayTempRecData(float data);
extern void GetCurDayTempRecData(u16_t *databuf);
#endif/*__TEMP_H__*/

