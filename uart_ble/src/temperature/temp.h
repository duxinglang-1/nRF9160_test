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

//sensor trigger type
#define TEMP_TRIGGER_BY_MENU			0x01		//menu trigger
#define	TEMP_TRIGGER_BY_APP_ONE_KEY		0x02		//one-key check trigger
#define	TEMP_TRIGGER_BY_HOURLY			0x04		//auto check hourly

extern float g_temp_skin;
extern float g_temp_body;

#endif/*__TEMP_H__*/

