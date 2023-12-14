/****************************************Copyright (c)************************************************
** File Name:			    ft_aging.c
** Descriptions:			Aging test head file
** Created By:				xie biao
** Created Date:			2023-12-12
** Modified Date:      		2023-12-12 
** Version:			    	V1.0
******************************************************************************************************/

typedef enum
{
	AGING_BEGIN,
	AGING_PPG = AGING_BEGIN,
	AGING_WIFI,
	AGING_GPS,
	AGING_VIB,
	AGING_TEMP,
	AGING_MAX
}FT_AGING_STATUS;

extern void FTAgingTestProcess(void);
extern void FTAgingTest(void);