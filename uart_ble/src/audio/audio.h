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
#include <zephyr.h>

extern void audio_init(void);
extern void audio_play_alarm(void);
extern void audio_play_chn_voice(void);
extern void audio_play_en_voice(void);
extern void audio_stop(void);


#endif/*__AUDIO_H__*/
