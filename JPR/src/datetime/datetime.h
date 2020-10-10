#ifndef __DATETIME_H__
#define __DATETIME_H__

#include <stdbool.h>
#include <stdint.h>

#define SYSTEM_STARTING_YEAR	1920
#define SYSTEM_STARTING_WEEK	4		// 0=sunday

#define SYSTEM_DEFAULT_YEAR		2020
#define SYSTEM_DEFAULT_MONTH	1
#define SYSTEM_DEFAULT_DAY		1
#define SYSTEM_DEFAULT_HOUR		0
#define SYSTEM_DEFAULT_MINUTE	0
#define SYSTEM_DEFAULT_SECOND	0

#define IDLE_DATE_SHOW_X	0
#define IDLE_DATE_SHOW_Y	101
#define IDLE_TIME_SHOW_X	0
#define IDLE_TIME_SHOW_Y	64
#define IDLE_WEEK_SHOW_X	0
#define IDLE_WEEK_SHOW_Y	138

typedef struct
{
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  week;
}sys_date_timer_t;

extern sys_date_timer_t date_time;

extern void InitSystemDateTime(void);
extern void SaveSystemDateTime(void);
extern void IdleShowSystemDate(void);
extern void IdleShowSystemTime(void);
extern void IdleShowSystemWeek(void);
extern void IdleShowSystemDateTime(void);
extern void StartSystemDateTime(void);
extern void UpdateSystemTime(void);

#endif/*__DATETIME_H__*/
