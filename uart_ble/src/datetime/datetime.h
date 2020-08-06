#ifndef __DATETIME_H__
#define __DATETIME_H__

#include <stdbool.h>
#include <stdint.h>

#define SYSTEM_DEFAULT_YEAR		2019
#define SYSTEM_DEFAULT_MONTH	12
#define SYSTEM_DEFAULT_DAY		31
#define SYSTEM_DEFAULT_HOUR		23
#define SYSTEM_DEFAULT_MINUTE	59
#define SYSTEM_DEFAULT_SECOND	35
#define SYSTEM_DATE_TIME_ADDR	0x000FF000

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
