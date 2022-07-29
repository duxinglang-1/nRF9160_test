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
	uint32_t img_addr;
	struct node *next;
};

typedef struct
{
	uint8_t count;				//动画帧数
	uint32_t interval;			//帧间隔
	uint16_t x;				//显示X坐标
	uint16_t y;				//显示Y坐标
	bool loop;				//是否循环播放
	ShowFinishCB callback;	//播放结束回调函数
	struct node *cache;		//帧图像数据链表
}AnimaShowInfo;

typedef struct node AnimaNode;

extern bool AnimaIsShowing(void);
extern void AnimaStopShow(void);
extern void AnimaPaushShow(void);
extern void AnimaResumeShow(void);
extern void AnimaShow(uint16_t x, uint16_t y, uint32_t *anima_img, uint8_t anima_count, uint32_t interview, bool loop_flag, ShowFinishCB callback);
#endif/*__ANIMATION_H__*/
