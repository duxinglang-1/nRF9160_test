/****************************************Copyright (c)************************************************
** File Name:			    audio.h
** Descriptions:			audio process head file
** Created By:				xie biao
** Created Date:			2021-03-04
** Modified Date:      		2021-05-08 
** Version:			    	V1.1
******************************************************************************************************/
#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>

extern void audio_init(void);
extern void AudioMsgProcess(void);
extern void SOSStopAlarm(void);
extern void SOSPlayAlarm(void);
extern void FallStopAlarm(void);
extern void FallPlayAlarmCn(void);
extern void FallPlayAlarmEn(void);

#endif/*__AUDIO_H__*/
