/****************************************Copyright (c)************************************************
** File Name:			    max32674.h
** Descriptions:			PPG process head file
** Created By:				xie biao
** Created Date:			2021-05-19
** Modified Date:      		2021-05-19 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __MAX32674_H__
#define __MAX32674_H__

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>

typedef enum
{
	ALG_MODE_HR_SPO2,
	ALG_MODE_BPT,
	ALG_MODE_ECG,
	ALG_MODE_MAX,
	ALG_MODE_RAW
}PPG_ALG_MODE;

typedef enum
{
	BPT_STATUS_GET_CAL,		//��SH��ȡУ������
	BPT_STATUS_SET_CAL,		//����У�����ݵ�SH
	BPT_STATUS_GET_EST,		//��SH��ȡѪѹ����
	BPT_STATUS_MAX
}PPG_BPT_WORK_STATUS;

typedef enum
{
	TRIGGER_BY_MENU=0x01,
	TRIGGER_BY_APP_ONE_KEY=0x02,
	TRIGGER_BY_APP=0x04,
	TRIGGER_BY_HOURLY=0x40
}PPG_TARGGER_SOUCE;

typedef enum
{
	PPG_REC2_HR,
	PPG_REC2_SPO2,
	PPG_REC2_BPT
}PPG_REC2_DATA_TYPE;

typedef struct
{
	uint8_t systolic;
	uint8_t diastolic;
}bpt_data;

//���ʲ���
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

//�������
typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hr[24];
}ppg_hr_rec2_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t spo2[24];
}ppg_spo2_rec2_data;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	bpt_data bpt[24];
}ppg_bpt_rec2_data;

extern uint8_t g_ppg_trigger;
extern uint8_t g_ppg_ver[64];

extern uint16_t g_hr;
extern uint16_t g_spo2;
extern uint16_t g_bp_systolic;		//����ѹ
extern uint16_t g_bp_diastolic;	//����ѹ

extern void PPG_init(void);
extern void PPGMsgProcess(void);

/*heart rate*/
extern void GetCurDayHrRecData(uint8_t *databuf);
extern void GetHeartRate(uint8_t *HR);
extern void APPStartHrSpo2(void);
extern void APPStartBpt(void);
extern void APPStartEcg(void);
extern void APPStartPPG(void);

typedef void (*sh_read_fifo_callback)(uint32_t cnt, uint32_t u32_sampleSize, uint8_t *buf);

typedef struct{
	uint8_t   biometricOpMode          ;    //selection among HR SPO2 and BPT: 0 - raw data, no Algo 1->WAS (HR+SPO2), 2->BPT, 3 ->WAS + BPT
	uint8_t   algoWasOperatingMode     ;    //working mode for HR and SPO2
	uint8_t   algoWasRptMode           ;    //output format for HR and SPO2
	uint8_t   algoBptMode              ;    //BPT calibration mode or BPT estimation mode
	uint8_t   bpt_ref_syst             ;
	uint8_t   bpt_ref_dias             ;
	uint8_t   reportPeriod_in40msSteps ;    //sample report rate
	uint8_t   FifoDataType             ;    //enable for PPG raw data, algorithm data
	uint8_t   FifoSampleSize           ;    //sample size is fixed by setting by Algo enabling and  data type enabling
	uint8_t   AccelType                ;    //Accl in sensor hub , or host Accl
	uint8_t   isRawDataEnabled         ;
	uint8_t   isAlgoDataEnabled        ;
	uint32_t  tmrPeriod_ms             ;
	int32_t*  s32_bptCabliVector       ;
	int32_t*  u8p_FifoBuf              ;    //pointer to output fifo data
	sh_read_fifo_callback sh_fn_cb     ;    //callback function after reading FIFO
}sshub_ctrl_params_t;

extern sshub_ctrl_params_t  sh_ctrl_HR_SPO2_param;
extern sshub_ctrl_params_t  sh_ctrl_WAS_BPT_CALIB_param;
extern sshub_ctrl_params_t  sh_ctrl_WAS_BPT_EST_param;
extern sshub_ctrl_params_t  sh_ctrl_rawdata_param;
extern sshub_ctrl_params_t  sh_ctrl_ecg_param ;

extern int32_t sh_enter_app_mode(void* param);
extern int32_t sh_enter_bl_mode(void* param);
extern int32_t sh_disable_sensor(void* param);
extern int32_t sh_start_HR_SPO2_mode(void* param);
extern int32_t sh_start_WAS_BPT_mode(void* param);
extern int32_t sh_start_rawdata_mode(void* param);
extern int32_t sh_start_ecg_mode(void* param);
extern void sh_FIFO_polling_handler(void);

#endif/*__MAX32674_H__*/
