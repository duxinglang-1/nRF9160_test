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

/*****************************************************************************
 * FUNCTION
 *  delete_anima_info_link
 * DESCRIPTION
 *  Delete an animated display list
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
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
	anima_show.interval = 0;
	anima_show.loop = false;
	anima_show.callback = NULL;
}

/*****************************************************************************
 * FUNCTION
 *  add_anima_info_link
 * DESCRIPTION
 *  Create an animated display list
 * PARAMETERS
 *	anima_img:	Array of picture pointers that need to be animated 
 *	anima_count:Number of images that need to be animated
 * RETURNS
 *  Nothing
 *****************************************************************************/
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

/*****************************************************************************
 * FUNCTION
 *  AnimaStopShow
 * DESCRIPTION
 *  Stop animation display
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void AnimaStopShow(void)
{
	LOG_INF("[%s] 001\n", __func__);
	k_timer_stop(&anima_redraw_timer);
	delete_anima_info_link();
}

/*****************************************************************************
 * FUNCTION
 *  AnimaStop
 * DESCRIPTION
 *  Stop animation display for other application
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void AnimaStop(void)
{
	LOG_INF("[%s] 001\n", __func__);
	anima_stop_flag = true;
}

/*****************************************************************************
 * FUNCTION
 *  AnimaShow
 * DESCRIPTION
 *  Start animation display
 * PARAMETERS
 *	x: 			The x starting coordinates of the animation playback
 *	y: 			The y starting coordinates of the animation playback
 *	anima_img: 	Array of picture pointers that need to be animated 
 *	anima_count:Number of images that need to be animated
 *	interview:	The interval between frames
 *	loop_flag: 	Animation show mode (true:loop show false:only one round show)
 *	callback: 	The callback function at the end of animation playback (only valid in round mode)
 * RETURNS
 *  Nothing
 *****************************************************************************/
void AnimaShow(u16_t x, u16_t y, u32_t *anima_img, u8_t anima_count, u32_t interview, bool loop_flag, ShowFinishCB callback)
{
	if(anima_img == NULL)
		return;

	add_anima_info_link(anima_img, anima_count);

	anima_show.interval = interview;
	anima_show.x = x;
	anima_show.y = y;
	anima_show.loop = loop_flag;
	anima_show.callback = callback;
	
	anima_head = anima_show.cache;
	
	LCD_ShowImg(anima_show.x, anima_show.y, (unsigned char*)anima_show.cache->img_addr);
	
	if(anima_show.count > 1)
	{
		LOG_INF("[%s] 001\n", __func__);
		k_timer_start(&anima_redraw_timer, K_MSEC(anima_show.interval), NULL);
	}
	else
	{
		LOG_INF("[%s] 002\n", __func__);
		k_timer_start(&anima_redraw_timer, K_MSEC(ANIMA_SHOW_ONE_DELAY), NULL);
	}
}

/*****************************************************************************
 * FUNCTION
 *  AnimaShowNextImg
 * DESCRIPTION
 *  Displays the next frame of the animation
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
static void AnimaShowNextImg(void)
{
	anima_head = anima_head->next;
	if(anima_head == NULL)
	{		
		if(anima_show.loop)	//循环播放
		{
			LOG_INF("[%s] 001\n", __func__);
			anima_head = anima_show.cache;
			LCD_ShowImg(anima_show.x, anima_show.y, (unsigned char*)anima_head->img_addr);
			k_timer_start(&anima_redraw_timer, K_MSEC(anima_show.interval), NULL);
		}
		else				//播放结束
		{
			LOG_INF("[%s] 002\n", __func__);
			if(anima_show.callback != NULL)
				anima_show.callback();
			AnimaStopShow();
		}
	}
	else
	{
		LOG_INF("[%s] 003\n", __func__);
		LCD_ShowImg(anima_show.x, anima_show.y, (unsigned char*)anima_head->img_addr);
		k_timer_start(&anima_redraw_timer, K_MSEC(anima_show.interval), NULL);
	}
}

/*****************************************************************************
 * FUNCTION
 *  AnimaShowNextImg
 * DESCRIPTION
 *  Displays the next frame of the animation
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
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

