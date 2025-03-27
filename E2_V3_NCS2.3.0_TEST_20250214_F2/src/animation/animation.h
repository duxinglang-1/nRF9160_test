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

struct aninode
{
	uint32_t img_addr;
	struct aninode *next;
};

typedef struct
{
	uint8_t count;				//����֡��
	uint32_t interval;			//֡���
	uint16_t x;				//��ʾX����
	uint16_t y;				//��ʾY����
	bool loop;				//�Ƿ�ѭ������
	ShowFinishCB callback;	//���Ž����ص�����
	struct aninode *cache;		//֡ͼ����������
}AnimaShowInfo;

typedef struct aninode AnimaNode;

extern bool AnimaIsShowing(void);
extern void AnimaStopShow(void);
extern void AnimaPaushShow(void);
extern void AnimaResumeShow(void);
extern void AnimaStop(void);
extern void AnimaShow(uint16_t x, uint16_t y, uint32_t *anima_img, uint8_t anima_count, uint32_t interview, bool loop_flag, ShowFinishCB callback);
#endif/*__ANIMATION_H__*/
