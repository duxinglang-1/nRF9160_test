/****************************************Copyright (c)************************************************
** File name:			Nb.h
** Last modified Date:          
** Last Version:		V1.0   
** Created by:			Ð»±ë
** Created date:		2020-06-30
** Version:			    1.0
** Descriptions:		NB-IoTÍ·ÎÄ¼þ
******************************************************************************************************/

typedef enum
{
	NB_SIG_LEVEL_NO,
	NB_SIG_LEVEL_0 = NB_SIG_LEVEL_NO,
	NB_SIG_LEVEL_1,
	NB_SIG_LEVEL_2,
	NB_SIG_LEVEL_3,
	NB_SIG_LEVEL_4,
	NB_SIG_LEVEL_MAX
}NB_SIGNL_LEVEL;

extern NB_SIGNL_LEVEL g_nb_sig;

extern void NBMsgProcess(void);
