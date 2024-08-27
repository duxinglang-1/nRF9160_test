/****************************************Copyright (c)************************************************
** File Name:			    ppg.h
** Descriptions:			PPG libraly head file
** Created By:				xie biao
** Created Date:			2024-06-13
** Modified Date:      		2024-06-13 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __PPG_H__
#define __PPG_H__

#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include "datetime.h"
#include "external_flash.h"

//#define GPIO_ACT_I2C

#ifdef GPIO_ACT_I2C
#define PPG_SCL_PIN		0
#define PPG_SDA_PIN		1
#else/*GPIO_ACT_I2C*/
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define PPG_DEV DT_NODELABEL(i2c1)
#else
#error "i2c1 devicetree node is disabled"
#define PPG_DEV	""
#endif
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define PPG_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define PPG_PORT	""
#endif

#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PULL_UP DT_ALIAS_SW0_GPIOS_FLAGS
#else
#define PULL_UP 0
#endif
#define EDGE (GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE)

#define MAX32674_I2C_ADD     0x55

#define PPG_INT_PIN		13
#define PPG_MFIO_PIN	14
#define PPG_RST_PIN		16
#define PPG_EN_PIN		17
#define PPG_I2C_EN_PIN	29

#define PPG_CHECK_HR_MENU			60
#define PPG_CHECK_SPO2_MENU			150
#define PPG_CHECK_BPT_MENU			150

#define PPG_CHECK_HR_TIMELY			1
#define PPG_CHECK_SPO2_TIMELY		2
#define PPG_CHECK_BPT_TIMELY		3
#ifndef CONFIG_TEMP_SUPPORT
#define TEMP_CHECK_TIMELY			0
#endif

#define PPG_HR_MAX		150
#define PPG_HR_MIN		30
#define PPG_SPO2_MAX	100
#define	PPG_SPO2_MIN	90
#define PPG_BPT_SYS_MAX	180
#define PPG_BPT_SYS_MIN	30
#define PPG_BPT_DIA_MAX	180
#define PPG_BPT_DIA_MIN	30

#define PPG_REC2_MAX_HOURLY	(4)
#define PPG_REC2_MAX_DAILY	(PPG_REC2_MAX_HOURLY*24)
#define PPG_REC2_MAX_COUNT	(PPG_REC2_MAX_DAILY*7)

typedef enum
{
	PPG_DATA_HR,
	PPG_DATA_SPO2,
	PPG_DATA_BPT,
	PPG_DATA_ECG,
	PPG_DATA_MAX
}PPG_DATA_TYPE;

typedef enum
{
	ALG_MODE_HR_SPO2,
	ALG_MODE_BPT,
	ALG_MODE_ECG,
	ALG_MODE_MAX
}PPG_ALG_MODE;

typedef enum
{
	PPG_STATUS_PREPARE,
	PPG_STATUS_MEASURING,
	PPG_STATUS_MEASURE_FAIL,
	PPG_STATUS_MEASURE_OK,
	PPG_STATUS_NOTIFY,
	PPG_STATUS_MAX
}PPG_WORK_STATUS;

typedef enum
{
	BPT_STATUS_GET_CAL,		//从SH获取校验数据
	BPT_STATUS_SET_CAL,		//设置校验数据到SH
	BPT_STATUS_GET_EST,		//从SH获取血压数据
	BPT_STATUS_MAX
}PPG_BPT_WORK_STATUS;

typedef enum
{
	TRIGGER_BY_MENU			=	0x01,
	TRIGGER_BY_APP_ONE_KEY	=	0x02,
	TRIGGER_BY_APP			=	0x04,
	TRIGGER_BY_HOURLY		=	0x08,
	TRIGGER_BY_FT			=	0x10,
	TRIGGER_BY_SCC			=	0x20,
}PPG_TRIGGER_SOURCE;

typedef enum
{
	PPG_REC2_HR,
	PPG_REC2_SPO2,
	PPG_REC2_BPT
}PPG_REC2_DATA_TYPE;

typedef enum
{
	PPG_TIMER_APPMODE,
	PPG_TIMER_AUTO_STOP,
	PPG_TIMER_MENU_STOP,
	PPG_TIMER_GET_HR,
	PPG_TIMER_DELAY_START,
	PPG_TIMER_BPT_EST_START,
	PPG_TIMER_SKIN_CHECK,
	PPG_TIER_MAX
}PPG_TIMER_NAME;

typedef int32_t (*ppgdev_write_ptr)(struct device *handle, uint8_t *tx_buf, uint32_t tx_len);
typedef int32_t (*ppgdev_read_ptr)(struct device *handle, uint8_t *rx_buf, uint32_t rx_len);

typedef struct
{
  /** Component mandatory fields **/
  ppgdev_write_ptr  write;
  ppgdev_read_ptr   read;
  /** Customizable optional pointer **/
  struct device *handle;
} ppgdev_ctx_t;

typedef struct
{
	uint8_t systolic;
	uint8_t diastolic;
}bpt_data;

typedef struct
{
	sys_date_timer_t timestamp;
	uint8_t hr;
}hr_record_t;

typedef struct
{
	sys_date_timer_t timestamp;
	uint8_t spo2;
}spo2_record_t;

typedef struct
{
	sys_date_timer_t timestamp;
	bpt_data bpt;
}bpt_record_t;

typedef struct
{
	sys_date_timer_t timestamp;
	uint16_t deca_temp;///实际温度放大10倍(36.5*10)
}temp_record_t;

typedef struct
{
	hr_record_t hr_rec;
	spo2_record_t spo2_rec;
	bpt_record_t bpt_rec;
	temp_record_t temp_rec;
	uint8_t hr_max;
	uint8_t hr_min;
	uint8_t spo2_max;
	uint8_t spo2_min;
	bpt_data bpt_max;
	bpt_data bpt_min;
	uint16_t deca_temp_max;
	uint16_t deca_temp_min;
}health_record_t;

//单次测量
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t hr;
}ppg_hr_rec1_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t spo2;
}ppg_spo2_rec1_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	bpt_data bpt;
}ppg_bpt_rec1_data;

//定时测量(最低5分钟间隔)
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t hr;
}hr_rec2_nod;

typedef struct
{
	hr_rec2_nod data[PPG_REC2_MAX_DAILY];
}ppg_hr_rec2_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t spo2;
}spo2_rec2_nod;

typedef struct
{
	spo2_rec2_nod data[PPG_REC2_MAX_DAILY];
}ppg_spo2_rec2_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	bpt_data bpt;
}bpt_rec2_nod;

typedef struct
{
	bpt_rec2_nod data[PPG_REC2_MAX_DAILY];
}ppg_bpt_rec2_data;

typedef struct
{
	PPG_TIMER_NAME name;
	k_timer_expiry_t expiry_fn;
	k_timer_stop_t stop_fn;
}ppg_timer_config_t;

typedef struct
{
	struct k_timer timer_id;
	ppg_timer_config_t timer_config;
}ppg_timer_t;

extern bool get_bpt_ok_flag;
extern bool get_hr_ok_flag;
extern bool get_spo2_ok_flag;
extern bool ppg_bpt_is_calbraed;
extern bool ppg_bpt_cal_need_update;
extern bool ppg_skin_contacted_flag;

extern uint8_t g_ppg_trigger;
extern uint8_t g_ppg_ver[64];
extern uint8_t ppg_test_info[256];

extern uint8_t g_hr;
extern uint8_t g_hr_menu;
extern uint8_t g_hr_hourly;
extern uint8_t g_spo2;
extern uint8_t g_spo2_menu;
extern uint8_t g_spo2_hourly;
extern bpt_data g_bpt;
extern bpt_data g_bpt_menu;
extern bpt_data g_bpt_hourly;

extern health_record_t last_health;
extern sys_date_timer_t g_health_check_time;
extern PPG_WORK_STATUS g_ppg_status;
extern ppgdev_ctx_t ppg_dev_ctx;

//PPG module initialization
extern void PPG_init(void);
//PPG module message processing
extern void PPGMsgProcess(void);
//PPG is woring
extern bool PPGIsWorking(void);
//PPG is in timed triggering operation
extern bool PPGIsWorkingTiming(void);

//Save the blood pressure data measured at this scheduled time to the daily record.
extern void SetCurDayBptRecData(sys_date_timer_t time_stamp, bpt_data bpt);
//Obtain all timed blood pressure records for the current day
extern void GetCurDayBptRecData(uint8_t *databuf);
//Obtain all timed blood pressure records for the given day
extern void GetGivenDayBptRecData(sys_date_timer_t date, uint8_t *databuf);
//Obtain blood pressure measurements at the given time
extern void GetGivenTimeBptRecData(sys_date_timer_t date, bpt_data *bpt);
//Save the blood oxygen data measured at this scheduled time to the daily record
extern void SetCurDaySpo2RecData(sys_date_timer_t time_stamp, uint8_t spo2);
//Obtain all timed blood oxygen records for the current day
extern void GetCurDaySpo2RecData(uint8_t *databuf);
//Obtain all timed blood oxygen records for the given day
extern void GetGivenDaySpo2RecData(sys_date_timer_t date, uint8_t *databuf);
//Obtain the blood oxygen measurement value at the given time
extern void GetGivenTimeSpo2RecData(sys_date_timer_t date, uint8_t *spo2);
//Save the heart rate data measured at this scheduled time to the daily record
extern void SetCurDayHrRecData(sys_date_timer_t time_stamp, uint8_t hr);
//Obtain all timed heart rate records for the current day
extern void GetCurDayHrRecData(uint8_t *databuf);
//Obtain all timed heart rate records for the given day
extern void GetGivenDayHrRecData(sys_date_timer_t date, uint8_t *databuf);
//Obtain the heart rate measurement value at the given time
extern void GetGivenTimeHrRecData(sys_date_timer_t date, uint8_t *hr);
//PPG detection startup interface function
extern void StartPPG(PPG_DATA_TYPE data_type, PPG_TRIGGER_SOURCE trigger_type);
//Activate wrist detachment detection
extern void StartSCC(void);
//Determine whether the wrist has been removed
extern bool CheckSCC(void);

//Reset the comparison data record to zero
extern void PPGCompareDataReset(void);
//Update PPG data records and display.
extern void PPGUpdateRecord(void);
//Upload manual measurement data to the server
extern void SyncSendHealthData(void);
//Upload scheduled measurement data to the server
extern void TimeCheckSendHealthData(void);
#endif/*__PPG_H__*/

