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

extern void GetSystemDateTime(sys_date_timer_t *systime);
extern void SetSystemDateTime(sys_date_timer_t systime);

#endif/*__DATETIME_H__*/
