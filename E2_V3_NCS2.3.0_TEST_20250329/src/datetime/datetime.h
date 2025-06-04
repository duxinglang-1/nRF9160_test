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

extern bool update_time;
extern bool update_date;
extern bool update_week;
extern bool update_date_time;
extern bool sys_time_count;
extern bool show_date_time_first;
extern uint8_t date_time_changed;//ͨ��λ���ж�����ʱ���Ƿ��б仯���ӵ�6λ���𣬷ֱ��ʾ������ʱ����

extern sys_date_timer_t date_time;

extern void InitSystemDateTime(void);
extern void SaveSystemDateTime(void);
extern void IdleShowSystemDate(void);
extern void IdleShowSystemTime(void);
extern void IdleShowSystemWeek(void);
extern void IdleShowSystemDateTime(void);
extern void StartSystemDateTime(void);
extern void StopSystemDateTime(void);
extern void UpdateSystemTime(void);
extern void RedrawSystemTime(void);
extern void TimeIncrease(sys_date_timer_t *date, uint32_t minutes);
extern void TimeDecrease(sys_date_timer_t *date, uint32_t minutes);
extern void GetSystemTimeSecString(uint8_t *str_utc);
#endif/*__DATETIME_H__*/
