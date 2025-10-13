/****************************************Copyright (c)************************************************
** File Name:			    ft_wifi.c
** Descriptions:			Factory test gpio(key&wrist) module head file
** Created By:				xie biao
** Created Date:			2023-02-20
** Modified Date:      		2023-02-20 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __FT_GPIO_H__
#define __FT_GPIO_H__

#include "key.h"

typedef enum
{
	FT_KEY_NONE,
	FT_KEY_DOWN,
	FT_KEY_UP,
	FT_KEY_DONE,
}FT_KEY_STATUS;

typedef struct
{
	KEY_CODE id;
	FT_KEY_STATUS status;
}ft_key_t;

extern void EnterFTMenuKey(void);

#endif/*__FT_GPIO_H__*/