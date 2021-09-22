/****************************************Copyright (c)************************************************
** File Name:			    animation.h
** Descriptions:			animation show process head file
** Created By:				xie biao
** Created Date:			2021-09-17
** Modified Date:      		2021-09-17 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __ANIMATION_H__
#define __ANIMATION_H__

typedef void (*ShowFinishCB)(void);

struct node
{
	u32_t img_addr;
	struct node *next;
};

typedef struct
{
	u8_t count;				//动画帧数
	u32_t interval;			//帧间隔
	u16_t x;				//显示X坐标
	u16_t y;				//显示Y坐标
	bool loop;				//是否循环播放
	ShowFinishCB callback;	//播放结束回调函数
	struct node *cache;		//帧图像数据链表
}AnimaShowInfo;

typedef struct node AnimaNode;

extern void AnimaStopShow(void);
extern void AnimaShow(u16_t x, u16_t y, u32_t *anima_img, u8_t anima_count, u32_t interview, bool loop_flag, ShowFinishCB callback);
#endif/*__ANIMATION_H__*/
