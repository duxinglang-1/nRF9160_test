/****************************************Copyright (c)************************************************
** File Name:			    ft_touch.h
** Descriptions:			Factory test touch module head file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __FT_TOUCH_H__
#define __FT_TOUCH_H__

#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif

typedef enum
{
	FT_TP_NONE,
	FT_TP_CHECKED,
}FT_TOUCH_STATUS;

typedef enum
{
	FT_TP_SINGLE_CLICK,
	FT_TP_MOVING_UP,
	FT_TP_MOVING_DOWN,
	FT_TP_MOVING_LEFT,
	FT_TP_MOVING_RIGHT,
	FT_TP_MAX
}FT_TOUCH_EVENT;

typedef struct
{
	FT_TOUCH_EVENT check_item;
	uint8_t check_count;
	uint8_t check_index;
	FT_TOUCH_STATUS status;
}ft_touch_t;

typedef void(*ft_tp_click_handler)(void);

typedef struct
{
	uint16_t circle_x;
	uint16_t circle_y;
	uint16_t circle_r;
	uint16_t str_x;
	uint16_t str_y;
	uint16_t str_w;
	uint16_t str_h;
	ft_tp_click_handler func;
}ft_touch_click_t;

#endif/*__FT_TOUCH_H__*/
