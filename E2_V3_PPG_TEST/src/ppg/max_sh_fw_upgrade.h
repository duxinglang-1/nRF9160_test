/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
********************************************************************************/
#ifndef __MAX_SH_FW_UPGRADE_H__
#define __MAX_SH_FW_UPGRADE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define PPG_REPORT_SIZE            18
#define ACCEL_REPORT_SIZE          6
#define ALGO_REPORT_SIZE           24 //56//24
#define ALGO_EXTENDED_REPORT_SIZE  56

#define MAX_WHRMWSPO2_SAMPLE_COUNT   45
#define MAX_WHRMWSPO2_FRAME_SIZE     (ALGO_EXTENDED_REPORT_SIZE + PPG_REPORT_SIZE + ACCEL_REPORT_SIZE)

#define sensHubAfeFundametalsamplePeriod    40
#define MAX_RAWPPG_SAMPLE_COUNT             45
#ifdef PPG_ACCEL_RAW_MODE
#define   RAWPPG_FRAME_SIZE            (6+20)
#else
#define   RAWPPG_FRAME_SIZE            20
#endif

//#define PPG_ACCEL_RAW_MODE

#define POLL_PERIOD_40MS       (0x1)

#define SS_OP_MODE_CONT_WHRM_CONT_SPO2     0x0
#define SS_OP_MODE_CONT_WHRM_ONE_SPO2      0x1
#define SS_OP_MODE_CONT_WHRM               0x2
#define SS_OP_MODE_SAMPLE_WHRM             0x3
#define SS_OP_MODE_SAMPLE_WHRM_ONE_SPO2    0x4
#define SS_OP_MODE_ACTIVITY_TARCK          0x5
#define SS_OP_MODE_SPO2_CALIB              0x6

enum{
	ALGO_REPORT_MODE_BASIC    = 1,
	ALGO_REPORT_MODE_EXTENDED = 2,
};

typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
} accel_mode1_data;

typedef struct {
	uint32_t green1;
	uint32_t ir;
	uint32_t red;
	uint32_t green2;
	uint32_t led5;
	uint32_t led6;
} max8614x_mode1_data;


typedef struct __attribute__((packed)){
	uint8_t current_operating_mode; // mode 1 & 2
	// WHRM data
	uint16_t hr;         	// mode 1 & 2
	uint8_t hr_conf;     	// mode 1 & 2
	uint16_t rr;         	// mode 1 & 2
	uint8_t rr_conf;		// mode 1 & 2
	uint8_t activity_class; // mode 1 & 2
	// WSPO2 data
	uint16_t r;						// mode 1 & 2
	uint8_t spo2_conf;		// mode 1 & 2
	uint16_t spo2;			// mode 1 & 2
	uint8_t percentComplete;		// mode 1 & 2
	uint8_t lowSignalQualityFlag;	// mode 1 & 2
	uint8_t motionFlag;				// mode 1 & 2
	uint8_t lowPiFlag;				// mode 1 & 2
	uint8_t unreliableRFlag;		// mode 1 & 2
	uint8_t spo2State;   			// mode 1 & 2
	uint8_t scd_contact_state;
	uint32_t ibi_offset;
} whrm_wspo2_suite_mode1_data;

typedef struct __attribute__((packed)){
	uint8_t current_operating_mode; // mode 1 & 2
	// WHRM data
	uint16_t hr;         	// mode 1 & 2
	uint8_t hr_conf;     	// mode 1 & 2
	uint16_t rr;         	// mode 1 & 2
	uint8_t rr_conf;		// mode 1 & 2
	uint8_t activity_class; // mode 1 & 2
	// WSPO2 data
	uint16_t r;						// mode 1 & 2
	uint8_t spo2_conf;		// mode 1 & 2
	uint16_t spo2;			// mode 1 & 2
	uint8_t percentComplete;		// mode 1 & 2
	uint8_t lowSignalQualityFlag;	// mode 1 & 2
	uint8_t motionFlag;				// mode 1 & 2
	uint8_t lowPiFlag;				// mode 1 & 2
	uint8_t unreliableRFlag;		// mode 1 & 2
	uint8_t spo2State;   			// mode 1 & 2
	uint8_t scd_contact_state;
    //Extended Report (mode2)
	uint32_t walk_steps;	// mode 2
	uint32_t run_steps;		// mode 2
	uint32_t kcal;			// mode 2
	uint32_t cadence;		// mode 2
	uint8_t  is_led_cur1_adj;	// mode 2
	uint16_t adj_led_cur1;	// mode 2
	uint8_t is_led_cur2_adj;// mode 2
	uint16_t adj_led_cur2;	// mode 2
	uint8_t is_led_cur3_adj;// mode 2
	uint16_t adj_led_cur3;	// mode 2
	uint8_t is_int_time_adj;	// mode 2
    uint8_t t_int_code;	// mode 2
	uint8_t is_f_smp_adj;	// mode 2
	uint8_t adj_f_smp;		// mode 2
	uint8_t smp_ave;		// mode 2
	uint8_t hrm_afe_state;  // mode 2
	uint8_t is_high_motion;	// mode 2

} whrm_wspo2_suite_mode2_data;


typedef struct{
	uint8_t   reportPeriod_in40msSteps ;
	uint8_t   algoSuiteOperatingMode   ;
	uint8_t   accelBehavior            ;
	uint32_t  poolPeriod_ms            ;
	uint8_t   isExtendedReport         ;
}sshub_meas_init_params_t;


enum data_report_mode{
	SSHUB_ALGO_RAW_DATA_REPORT_MODE = 1,
	SSHUB_RAW_DATA_REPORT_MODE      = 2,
	SSHUB_ACCEL_WAKEUP_REPORT_MODE  = 3,
	NO_DATA_CAPTURE_MODE            = 4
};

#define SSMAX8614X_REG_SIZE  		     1
#define MAX_NUM_WR_ACC_SAMPLES			 5
#define MIN_MACRO(a,b) ((a)<(b)?(a):(b))

struct {
   uint8_t  isScdSmEnabled      ;
   uint8_t  isHubAccelEnabled   ;
   uint8_t  hubAccelSkinoffWUFC ;
   uint8_t  hubAccelSkinoffATH  ;
   uint8_t  dataReportMode      ;
   uint8_t  isScdSmActive       ;
   uint32_t probingLedonPeriod  ;
   uint32_t probingWaitPeriod   ;
   uint32_t skinoffWaitPeriod   ;

} scdSmConfigStruct;

typedef enum  {
	sshub_ppg_report_mode_1  = 1,
    sshub_accel_wakeup_mode  = 2
} sensorhub_report_mode_t;

void sh_disable_sensor_algo(void);
int sh_measure_whrm_wspo2(uint8_t u8_rptRate, uint8_t u8_algoSuiteMode, uint8_t u8_dataType);
int sh_measure_whrm_wspo2_extend(uint8_t u8_rptRate, uint8_t u8_algoSuiteMode, uint8_t u8_dataType);
int measure_whrm_wspo2_extended_report(void);
#ifdef SH_OTA_DATA_STORE_IN_FLASH
int32_t SH_OTA_upgrade_process(void);
#else
int32_t SH_OTA_upgrade_process(uint8_t* u8p_FwData);
#endif
int sh_get_PPG_raw(void);

#endif/*__MAX_SH_FW_UPGRADE_H__*/
