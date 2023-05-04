/****************************************Copyright (c)************************************************
** File Name:			    sleep.h
** Descriptions:			sleep message process head file
** Created By:				xie biao
** Created Date:			2022-05-26
** Modified Date:      		2022-05-26 
** Version:			    	V1.2
******************************************************************************************************/
#ifndef __SLEEP_H__
#define __SLEEP_H__

#define SLEEP_TIME_START	20
#define SLEEP_TIME_END		8

extern bool update_sleep_parameter;
extern bool reset_sleep_data;
extern uint16_t g_light_sleep;
extern uint16_t g_deep_sleep;

extern void UpdateSleepPara(void);
#endif/*__SLEEP_H__*/
