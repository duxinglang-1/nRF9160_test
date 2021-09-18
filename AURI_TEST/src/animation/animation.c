/****************************************Copyright (c)************************************************
** File Name:			    animation.c
** Descriptions:			animation show process source file
** Created By:				xie biao
** Created Date:			2021-09-17
** Modified Date:      		2021-09-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>
#include "lcd.h"
#include "font.h"
#include "settings.h"
#include "screen.h"
#include "animation.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(animation, CONFIG_LOG_DEFAULT_LEVEL);

#define ANIMA_SHOW_INTERVIEW	200		//多张图片之间显示间隔
#define ANIMA_SHOW_ONE_DELAY	200		//只有一张持续显示时间

static bool anima_stop_flag = false;
static bool anima_redraw_flag = false;

static AnimaShowInfo anima_show = {0};
static AnimaNode *anima_head = NULL;

static void AnimaTimerCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(anima_redraw_timer, AnimaTimerCallBack, NULL);

static void AnimaTimerCallBack(struct k_timer *timer_id)
{
	anima_redraw_flag = true;
}

void delete_anima_info_link(void)
{
	while(anima_show.cache != NULL && anima_show.count != 0)
	{
		anima_head = anima_show.cache->next;
		
		anima_show.cache->img_addr = 0;
		k_free(anima_show.cache);
		anima_show.cache = NULL;

		anima_show.count--;
		anima_show.cache = anima_head;
	}

	anima_show.x = 0;
	anima_show.y = 0;
	anima_show.loop = false;
	anima_show.callback = NULL;
}


void add_anima_info_link(u32_t *anima_img, u8_t anima_count)
{
	u8_t i;
	AnimaNode *pnew = NULL;
	
	if(anima_show.cache != NULL && anima_show.count != 0)
		delete_anima_info_link();

	if(anima_img == NULL)
		return;

	anima_show.count = anima_count;
	anima_head = anima_show.cache;
	for(i=0;i<anima_show.count;i++)
	{
		pnew = k_malloc(sizeof(AnimaNode));
		if(pnew == NULL) 
			return;
	
		memset(pnew, 0, sizeof(AnimaNode));

		pnew->img_addr = anima_img[i];
		if(anima_head == NULL)
		{
			anima_show.cache = pnew;
			anima_head = anima_show.cache;
		}
		else
		{
			anima_head->next = pnew;
			anima_head = anima_head->next;
		}
	}

	anima_show.count = i;
}

void AnimaStopShow(void)
{
	k_timer_stop(&anima_redraw_timer);
	delete_anima_info_link();
}

void AnimaShow(u16_t x, u16_t y, u32_t *anima_img, u8_t anima_count, bool loop_flag, ShowFinishCB callback)
{
	if(anima_img == NULL)
		return;

	anima_show.x = x;
	anima_show.y = y;
	anima_show.loop = loop_flag;
	anima_show.callback = callback;
	add_anima_info_link(anima_img, anima_count);

	anima_head = anima_show.cache;
	
	LCD_ShowImg(anima_show.x, anima_show.y, (unsigned char*)anima_show.cache->img_addr);
	
	if(anima_show.count > 1)
	{
		k_timer_start(&anima_redraw_timer, K_MSEC(ANIMA_SHOW_INTERVIEW), NULL);
	}
	else
	{
		k_timer_start(&anima_redraw_timer, K_MSEC(ANIMA_SHOW_ONE_DELAY), NULL);
	}
}

static void AnimaShowNextImg(void)
{
	anima_head = anima_head->next;
	if(anima_head == NULL)
	{		
		if(anima_show.loop)	//循环播放
		{
			anima_head = anima_show.cache;
			LCD_ShowImg(anima_show.x, anima_show.y, (unsigned char*)anima_head->img_addr);
			k_timer_start(&anima_redraw_timer, K_MSEC(ANIMA_SHOW_INTERVIEW), NULL);
		}
		else				//播放结束
		{
			if(anima_show.callback != NULL)
				anima_show.callback();
			AnimaStopShow();
		}
	}
	else
	{
		LCD_ShowImg(anima_show.x, anima_show.y, (unsigned char*)anima_head->img_addr);
		k_timer_start(&anima_redraw_timer, K_MSEC(ANIMA_SHOW_INTERVIEW), NULL);
	}
}

void AnimaMsgProcess(void)
{
	if(anima_redraw_flag)
	{
		AnimaShowNextImg();
		anima_redraw_flag = false;
	}
	if(anima_stop_flag)
	{
		AnimaStopShow();
		anima_stop_flag = false;
	}
}

