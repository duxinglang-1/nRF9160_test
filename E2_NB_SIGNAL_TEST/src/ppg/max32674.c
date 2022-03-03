/****************************************Copyright (c)************************************************
** File Name:			    max32674.c
** Descriptions:			PPG process source file
** Created By:				xie biao
** Created Date:			2021-05-19
** Modified Date:      		2021-05-19 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <nrf_socket.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <nrfx.h>
#include "Max32674.h"
#include "screen.h"
#include "max_sh_interface.h"
#include "max_sh_api.h"
#include "datetime.h"
#include "external_flash.h"
#include "logger.h"

#define PPG_DEBUG

bool ppg_int_event = false;
bool ppg_fw_upgrade_flag = false;
bool ppg_start_flag = false;
bool ppg_test_flag = false;
bool ppg_stop_flag = false;
bool ppg_get_data_flag = false;
bool ppg_redraw_data_flag = false;
bool ppg_get_cal_flag = false;
bool ppg_bpt_is_calbraed = false;

u8_t ppg_power_flag = 0;	//0:关闭 1:正在启动 2:启动成功
static u8_t whoamI=0, rst=0;

u8_t g_ppg_trigger = 0;
u8_t g_ppg_alg_mode = ALG_MODE_HR_SPO2;
u8_t g_ppg_bpt_status = BPT_STATUS_GET_EST;

u16_t g_hr = 0;
u16_t g_bp_systolic = 0;	//收缩压
u16_t g_bp_diastolic = 0;	//舒张压
u16_t g_spo2 = 0;

static void ppg_auto_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_stop_timer, ppg_auto_stop_timerout, NULL);
static void ppg_get_data_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_get_hr_timer, ppg_get_data_timerout, NULL);

void SetCurDayBptRecData(bpt_data bpt)
{
	u8_t i,tmpbuf[512] = {0};
	ppg_bpt_rec2_data *p_bpt = NULL;
	
	SpiFlash_Read(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);

	p_bpt = tmpbuf+6*sizeof(ppg_bpt_rec2_data);
	if(((date_time.year > p_bpt->year)&&(p_bpt->year != 0xffff && p_bpt->year != 0x0000))
		||((date_time.year == p_bpt->year)&&(date_time.month > p_bpt->month)&&(p_bpt->month != 0xff && p_bpt->month != 0x00))
		||((date_time.month == p_bpt->month)&&(date_time.day > p_bpt->day)&&(p_bpt->day != 0xff && p_bpt->day != 0x00)))
	{//记录存满。整体前挪并把最新的放在最后
		ppg_bpt_rec2_data tmp_bpt = {0};

	#ifdef PPG_DEBUG
		LOGD("rec is full! bpt:%d,%d", bpt.systolic, bpt.diastolic);
	#endif
		tmp_bpt.year = date_time.year;
		tmp_bpt.month = date_time.month;
		tmp_bpt.day = date_time.day;
		memcpy(&tmp_bpt.bpt[date_time.hour], &bpt, sizeof(bpt_data));
		memcpy(&tmpbuf[0], &tmpbuf[sizeof(ppg_bpt_rec2_data)], 6*sizeof(ppg_bpt_rec2_data));
		memcpy(&tmpbuf[6*sizeof(ppg_bpt_rec2_data)], &tmp_bpt, sizeof(ppg_bpt_rec2_data));
		SpiFlash_Write(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	}
	else
	{
	#ifdef PPG_DEBUG
		LOGD("rec not full! bpt:%d,%d", bpt.systolic, bpt.diastolic);
	#endif
		for(i=0;i<7;i++)
		{
			p_bpt = tmpbuf + i*sizeof(ppg_bpt_rec2_data);
			if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)||(p_bpt->month == 0xff || p_bpt->month == 0x00)||(p_bpt->day == 0xff || p_bpt->day == 0x00))
			{
				p_bpt->year = date_time.year;
				p_bpt->month = date_time.month;
				p_bpt->day = date_time.day;
				memcpy(&p_bpt->bpt[date_time.hour], &bpt, sizeof(bpt_data));
				SpiFlash_Write(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
				break;
			}
			
			if((p_bpt->year == date_time.year)&&(p_bpt->month == date_time.month)&&(p_bpt->day == date_time.day))
			{
				memcpy(&p_bpt->bpt[date_time.hour], &bpt, sizeof(bpt_data));
				SpiFlash_Write(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
				break;
			}
		}
	}
}

void GetCurDayBptRecData(u8_t *databuf)
{
	u8_t i,tmpbuf[512] = {0};
	ppg_bpt_rec2_data bpt_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&bpt_rec2, &tmpbuf[i*sizeof(ppg_bpt_rec2_data)], sizeof(ppg_bpt_rec2_data));
		if((bpt_rec2.year == 0xffff || bpt_rec2.year == 0x0000)||(bpt_rec2.month == 0xff || bpt_rec2.month == 0x00)||(bpt_rec2.day == 0xff || bpt_rec2.day == 0x00))
			continue;
		
		if((bpt_rec2.year == date_time.year)&&(bpt_rec2.month == date_time.month)&&(bpt_rec2.day == date_time.day))
		{
			memcpy(databuf, bpt_rec2.bpt, sizeof(bpt_rec2.bpt));
			break;
		}
	}
}

void SetCurDaySpo2RecData(u8_t spo2)
{
	u8_t i,tmpbuf[512] = {0};
	ppg_spo2_rec2_data *p_spo2 = NULL;
	
	SpiFlash_Read(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);

	p_spo2 = tmpbuf+6*sizeof(ppg_spo2_rec2_data);
	if(((date_time.year > p_spo2->year)&&(p_spo2->year != 0xffff && p_spo2->year != 0x0000))
		||((date_time.year == p_spo2->year)&&(date_time.month > p_spo2->month)&&(p_spo2->month != 0xff && p_spo2->month != 0x00))
		||((date_time.month == p_spo2->month)&&(date_time.day > p_spo2->day)&&(p_spo2->day != 0xff && p_spo2->day != 0x00)))
	{//记录存满。整体前挪并把最新的放在最后
		ppg_spo2_rec2_data tmp_spo2 = {0};

	#ifdef PPG_DEBUG
		LOGD("rec is full! spo2:%d", spo2);
	#endif
		tmp_spo2.year = date_time.year;
		tmp_spo2.month = date_time.month;
		tmp_spo2.day = date_time.day;
		tmp_spo2.spo2[date_time.hour] = spo2;
		memcpy(&tmpbuf[0], &tmpbuf[sizeof(ppg_spo2_rec2_data)], 6*sizeof(ppg_spo2_rec2_data));
		memcpy(&tmpbuf[6*sizeof(ppg_spo2_rec2_data)], &tmp_spo2, sizeof(ppg_spo2_rec2_data));
		SpiFlash_Write(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	}
	else
	{
	#ifdef PPG_DEBUG
		LOGD("rec not full! spo2:%d", spo2);
	#endif
		for(i=0;i<7;i++)
		{
			p_spo2 = tmpbuf + i*sizeof(ppg_spo2_rec2_data);
			if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)||(p_spo2->month == 0xff || p_spo2->month == 0x00)||(p_spo2->day == 0xff || p_spo2->day == 0x00))
			{
				p_spo2->year = date_time.year;
				p_spo2->month = date_time.month;
				p_spo2->day = date_time.day;
				p_spo2->spo2[date_time.hour] = spo2;
				SpiFlash_Write(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
				break;
			}
			
			if((p_spo2->year == date_time.year)&&(p_spo2->month == date_time.month)&&(p_spo2->day == date_time.day))
			{
				p_spo2->spo2[date_time.hour] = spo2;
				SpiFlash_Write(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
				break;
			}
		}
	}
}

void GetCurDaySpo2RecData(u8_t *databuf)
{
	u8_t i,tmpbuf[512] = {0};
	ppg_spo2_rec2_data spo2_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&spo2_rec2, &tmpbuf[i*sizeof(ppg_spo2_rec2_data)], sizeof(ppg_spo2_rec2_data));
		if((spo2_rec2.year == 0xffff || spo2_rec2.year == 0x0000)||(spo2_rec2.month == 0xff || spo2_rec2.month == 0x00)||(spo2_rec2.day == 0xff || spo2_rec2.day == 0x00))
			continue;
		
		if((spo2_rec2.year == date_time.year)&&(spo2_rec2.month == date_time.month)&&(spo2_rec2.day == date_time.day))
		{
			memcpy(databuf, spo2_rec2.spo2, sizeof(spo2_rec2.spo2));
			break;
		}
	}
}

void SetCurDayHrRecData(u8_t hr)
{
	u8_t i,tmpbuf[512] = {0};
	ppg_hr_rec2_data *p_hr = NULL;
	
	SpiFlash_Read(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);

	p_hr = tmpbuf+6*sizeof(ppg_hr_rec2_data);
	if(((date_time.year > p_hr->year)&&(p_hr->year != 0xffff && p_hr->year != 0x0000))
		||((date_time.year == p_hr->year)&&(date_time.month > p_hr->month)&&(p_hr->month != 0xff && p_hr->month != 0x00))
		||((date_time.month == p_hr->month)&&(date_time.day > p_hr->day)&&(p_hr->day != 0xff && p_hr->day != 0x00)))
	{//记录存满。整体前挪并把最新的放在最后
		ppg_hr_rec2_data tmp_hr = {0};

	#ifdef PPG_DEBUG
		LOGD("rec is full! hr:%d", hr);
	#endif
		tmp_hr.year = date_time.year;
		tmp_hr.month = date_time.month;
		tmp_hr.day = date_time.day;
		tmp_hr.hr[date_time.hour] = hr;
		memcpy(&tmpbuf[0], &tmpbuf[sizeof(ppg_hr_rec2_data)], 6*sizeof(ppg_hr_rec2_data));
		memcpy(&tmpbuf[6*sizeof(ppg_hr_rec2_data)], &tmp_hr, sizeof(ppg_hr_rec2_data));
		SpiFlash_Write(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	}
	else
	{
	#ifdef PPG_DEBUG
		LOGD("rec not full! hr:%d,%d", hr);
	#endif
		for(i=0;i<7;i++)
		{
			p_hr = tmpbuf + i*sizeof(ppg_hr_rec2_data);
			if((p_hr->year == 0xffff || p_hr->year == 0x0000)||(p_hr->month == 0xff || p_hr->month == 0x00)||(p_hr->day == 0xff || p_hr->day == 0x00))
			{
				p_hr->year = date_time.year;
				p_hr->month = date_time.month;
				p_hr->day = date_time.day;
				p_hr->hr[date_time.hour] = hr;
				SpiFlash_Write(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
				break;
			}
			
			if((p_hr->year == date_time.year)&&(p_hr->month == date_time.month)&&(p_hr->day == date_time.day))
			{
				p_hr->hr[date_time.hour] = hr;
				SpiFlash_Write(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
				break;
			}
		}
	}
}

void GetCurDayHrRecData(u8_t *databuf)
{
	u8_t i,tmpbuf[512] = {0};
	ppg_hr_rec2_data hr_rec2 = {0};
	
	if(databuf == NULL)
		return;

	SpiFlash_Read(tmpbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	for(i=0;i<7;i++)
	{
		memcpy(&hr_rec2, &tmpbuf[i*sizeof(ppg_hr_rec2_data)], sizeof(ppg_hr_rec2_data));
		if((hr_rec2.year == 0xffff || hr_rec2.year == 0x0000)||(hr_rec2.month == 0xff || hr_rec2.month == 0x00)||(hr_rec2.day == 0xff || hr_rec2.day == 0x00))
			continue;
		
		if((hr_rec2.year == date_time.year)&&(hr_rec2.month == date_time.month)&&(hr_rec2.day == date_time.day))
		{
			memcpy(databuf, hr_rec2.hr, sizeof(hr_rec2.hr));
			break;
		}
	}
}

void GetHeartRate(u8_t *HR)
{
	u32_t heart;

	while(1)
	{
		heart = sys_rand32_get();
		if(((heart%200)>=60) && ((heart%200)<=160))
		{
			*HR = (heart%200);
			break;
		}
	}
}

void GetPPGData(u8_t *hr, u8_t *spo2, u8_t *systolic, u8_t *diastolic)
{
	if(hr != NULL)
		*hr = g_hr;
	
	if(spo2 != NULL)
		*spo2 = g_spo2;
	
	if(systolic != NULL)
		*systolic = 120;
	
	if(diastolic != NULL)
		*diastolic = 80;
}

bool PPGIsWorking(void)
{
	if(ppg_power_flag == 0)
		return false;
	else
		return true;
}

void PPGRedrawData(void)
{
	if(screen_id == SCREEN_ID_HR || screen_id == SCREEN_ID_SPO2 || screen_id == SCREEN_ID_BP)
	{
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void ppg_get_data_timerout(struct k_timer *timer_id)
{
	ppg_get_data_flag = true;
}

bool StartSensorhub(void)
{
	int status = -1;
	u8_t hubMode = 0x00;
	u8_t period = 0;
	
	SH_rst_to_APP_mode();
	
	status = sh_get_sensorhub_operating_mode(&hubMode);
	if((hubMode != 0x00) && (status != SS_SUCCESS))
	{
	#ifdef PPG_DEBUG
		LOGD("work mode is not app mode:%d", hubMode);
	#endif
		return false;
	}

	if(g_ppg_alg_mode == ALG_MODE_BPT)
	{
		if(!ppg_bpt_is_calbraed)
		{
			status = sh_check_bpt_cal_data();
			if(status)
			{
			#ifdef PPG_DEBUG
				LOGD("check bpt cal success");
			#endif
				sh_set_bpt_cal_data();
				//ppg_bpt_is_calbraed = true;
				g_ppg_bpt_status = BPT_STATUS_GET_EST;
			}
			else
			{
			#ifdef PPG_DEBUG
				LOGD("check bpt cal fail, req cal from algo");
			#endif
				sh_req_bpt_cal_data();
				g_ppg_bpt_status = BPT_STATUS_GET_CAL;

				k_timer_start(&ppg_get_hr_timer, K_MSEC(200), K_MSEC(200));
				return;
			}
		}
		else
		{
			g_ppg_bpt_status = BPT_STATUS_GET_EST;
		}
		
		//Enable AEC
		sh_set_cfg_wearablesuite_afeenable(1);
		//Enable automatic calculation of target PD current
		sh_set_cfg_wearablesuite_autopdcurrentenable(1);
		//Set minimum PD current to 12.5uA
		sh_set_cfg_wearablesuite_minpdcurrent(125);
		//Set initial PD current to 31.2uA
		sh_set_cfg_wearablesuite_initialpdcurrent(312);
		//Set target PD current to 31.2uA
		sh_set_cfg_wearablesuite_targetpdcurrent(312);
		//Enable SCD
		sh_set_cfg_wearablesuite_scdenable(1);
		//Set the output format to Sample Counter byte, Sensor Data and Algorithm
		sh_set_data_type(SS_DATATYPE_BOTH, true);
		//set fifo thresh
		sh_set_fifo_thresh(1);
		//Set the samples report period to 40ms(minimum is 32ms for BPT).
		sh_set_report_period(25);
		//Enable the sensor.
		sensorhub_enable_sensors();
		//set algo mode
		sh_set_cfg_wearablesuite_algomode(0x0);
		//set algo oper mode
		sensorhub_set_algo_operation_mode(SH_OPERATION_WHRM_BPT_MODE);
		g_algo_sensor_stat.bpt_algo_enabled = 1;
		//set algo submode
		sensorhub_set_algo_submode(SH_OPERATION_WHRM_BPT_MODE, SH_BPT_MODE_ESTIMATION);
		//enable algo
		sh_enable_algo_(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, SENSORHUB_MODE_BASIC);
		g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 1;

		k_timer_start(&ppg_get_hr_timer, K_MSEC(200), K_MSEC(500));		
	}
	else if(g_ppg_alg_mode == ALG_MODE_HR_SPO2)
	{
		//set fifo thresh
		sh_set_fifo_thresh(1);
		//Set the samples report period to 40ms(minimum is 32ms for BPT).
		sh_set_report_period(25);
		//Enable AEC
		sh_set_cfg_wearablesuite_afeenable(1);
		//Enable automatic calculation of target PD current
		sh_set_cfg_wearablesuite_autopdcurrentenable(1);
		//Set the output format to Sample Counter byte, Sensor Data and Algorithm
		sh_set_data_type(SS_DATATYPE_BOTH, true);
		//Enable the sensor.
		sensorhub_enable_sensors();
		//set algo oper mode
		sensorhub_set_algo_operation_mode(SH_OPERATION_WHRM_MODE);
		g_algo_sensor_stat.bpt_algo_enabled = 0;
		//Set the WAS algorithm operation mode to Continuous HRM + Continuous SpO2
		sh_set_cfg_wearablesuite_algomode(0x0);
		//enable algo
		sh_enable_algo_(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, SENSORHUB_MODE_BASIC);
		g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 1;

		k_timer_start(&ppg_get_hr_timer, K_MSEC(1*1000), K_MSEC(1*1000));
	}
	
	return true;
}

void PPGGetSensorHubData(void)
{
	bool get_bpt_flag = false;
	int ret = 0;
	int num_bytes_to_read = 0;
	u8_t hubStatus = 0;
	u8_t databuf[256] = {0};
	whrm_wspo2_suite_sensorhub_data sensorhub_out = {0};
	bpt_sensorhub_data bpt = {0};
	accel_data accel = {0};
	max86176_data max86176 = {0};

	ret = sh_get_sensorhub_status(&hubStatus);
#ifdef PPG_DEBUG	
	LOGD("ret:%d, hubStatus:%d", ret, hubStatus);
#endif
	if(hubStatus & SS_MASK_STATUS_FIFO_OUT_OVR)
	{
	#ifdef PPG_DEBUG
		LOGD("SS_MASK_STATUS_FIFO_OUT_OVR");
	#endif
	}

	if((0 == ret) && (hubStatus & SS_MASK_STATUS_DATA_RDY))
	{
		int u32_sampleCnt = 0;

	#ifdef PPG_DEBUG
		LOGD("FIFO ready");
	#endif

		num_bytes_to_read += SS_PACKET_COUNTERSIZE;
		if(g_algo_sensor_stat.max86176_enabled)
			num_bytes_to_read += SSMAX86176_MODE1_DATASIZE;
		if(g_algo_sensor_stat.accel_enabled)
			num_bytes_to_read += SSACCEL_MODE1_DATASIZE;
		if(g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1)
			num_bytes_to_read += SSWHRM_WSPO2_SUITE_MODE1_DATASIZE;
		if(g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2)
			num_bytes_to_read += SSWHRM_WSPO2_SUITE_MODE2_DATASIZE;
		if(g_algo_sensor_stat.bpt_algo_enabled)
			num_bytes_to_read += SSBPT_ALGO_DATASIZE;
		if(g_algo_sensor_stat.algo_raw_enabled)
			num_bytes_to_read += SSRAW_ALGO_DATASIZE;

		ret = sensorhub_get_output_sample_number(&u32_sampleCnt);
		if(ret == SS_SUCCESS)
		{
		#ifdef PPG_DEBUG
			LOGD("sample count is:%d", u32_sampleCnt);
		#endif
		}
		else
		{
		#ifdef PPG_DEBUG
			LOGD("read sample count fail:%d", ret);
		#endif
		}

		WAIT_MS(5);

		if(u32_sampleCnt > READ_SAMPLE_COUNT_MAX)
			u32_sampleCnt = READ_SAMPLE_COUNT_MAX;
		
		ret = sh_read_fifo_data(u32_sampleCnt, num_bytes_to_read, databuf, sizeof(databuf));
		if(ret == SS_SUCCESS)
		{
			u16_t heart_rate=0,spo2=0;
			u32_t i,j,index = 0;

			for(i=0,j=0;i<u32_sampleCnt;i++)
			{
				index = i * num_bytes_to_read + 1;

				if(g_ppg_alg_mode == ALG_MODE_BPT)
				{
					bpt_algo_data_rx(&bpt, &databuf[index+SS_PACKET_COUNTERSIZE + SSMAX86176_MODE1_DATASIZE + SSACCEL_MODE1_DATASIZE + SSWHRM_WSPO2_SUITE_MODE1_DATASIZE]);
				
				#ifdef PPG_DEBUG
					LOGD("bpt_status:%d, bpt_per:%d, bpt_sys:%d, bpt_dia:%d", bpt.status, bpt.perc_comp, bpt.sys_bp, bpt.dia_bp);
				#endif

					if(g_ppg_bpt_status == BPT_STATUS_GET_CAL)
					{
						if((bpt.status == 2) && (bpt.perc_comp == 100))
						{
							//get calbration data success
						#ifdef PPG_DEBUG
							LOGD("get calbration data success!");
						#endif
							//ppg_bpt_is_calbraed = true;
							sh_get_bpt_cal_data();
						
							PPGStopCheck();
							
							g_ppg_bpt_status = BPT_STATUS_GET_EST;
							PPGStartCheck();
						}
						
						return;
					}
					else if(g_ppg_bpt_status == BPT_STATUS_GET_EST)
					{
						g_bp_systolic = bpt.sys_bp;
						g_bp_diastolic = bpt.dia_bp;
							
						if((bpt.status == 2) && (bpt.perc_comp == 100))
						{
							//get bpt data success
						#ifdef PPG_DEBUG
							LOGD("get bpt data success!");
						#endif

							get_bpt_flag = true;
							PPGStopCheck();
						}
					}
				}
				else
				{
					//accel_data_rx(&accel, &databuf[index+SS_PACKET_COUNTERSIZE]);
					//max86176_data_rx(&max86176, &databuf[index+SS_PACKET_COUNTERSIZE + SSACCEL_MODE1_DATASIZE]);
					whrm_wspo2_suite_data_rx_mode1(&sensorhub_out, &databuf[index+SS_PACKET_COUNTERSIZE + SSMAX86176_MODE1_DATASIZE + SSACCEL_MODE1_DATASIZE]);
					
				#ifdef PPG_DEBUG
					LOGD("skin:%d, hr:%d, spo2:%d", sensorhub_out.scd_contact_state, sensorhub_out.hr, sensorhub_out.spo2);
				#endif

					if(sensorhub_out.scd_contact_state == 3)	//Skin contact state:0: Undetected 1: Off skin 2: On some subject 3: On skin
					{
						heart_rate += sensorhub_out.hr;
						spo2 += sensorhub_out.spo2;
						j++;
					}
				}
			}

			if(g_ppg_alg_mode == ALG_MODE_HR_SPO2)
			{
				if(j > 0)
				{
					heart_rate = heart_rate/j;
					spo2 = spo2/j;
				}
				g_hr = heart_rate/10 + ((heart_rate%10 > 4) ? 1 : 0);
				g_spo2 = spo2/10 + ((spo2%10 > 4) ? 1 : 0);
			#ifdef PPG_DEBUG
				LOGD("avra hr:%d, spo2:%d", heart_rate, spo2);
				LOGD("g_hr:%d, g_spo2:%d", g_hr, g_spo2);
			#endif	
			}
		}
		else
		{
		#ifdef PPG_DEBUG
			LOGD("read FIFO result fail:%d", ret);
		#endif
		}
	}
	else
	{
	#ifdef PPG_DEBUG
		LOGD("FIFO status is not ready:%d,%d", ret, hubStatus);
	#endif
	}

	if(g_ppg_alg_mode == ALG_MODE_BPT)
	{
		static bool flag = false;

		flag = !flag;
		if(flag || get_bpt_flag)
			ppg_redraw_data_flag = true;
	}
	else if(g_ppg_alg_mode == ALG_MODE_HR_SPO2)
	{
		ppg_redraw_data_flag = true;
	}
}

void APPStartHrSpo2(void)
{
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;
	ppg_start_flag = true;
}

void MenuStartHrSpo2(void)
{
	g_ppg_trigger |= TRIGGER_BY_MENU;
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;
	ppg_start_flag = true;
}

void MenuStopHrSpo2(void)
{
	g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_MENU);
	ppg_stop_flag = true;
}

void APPStartBpt(void)
{
	g_ppg_trigger |= TRIGGER_BY_APP;
	g_ppg_alg_mode = ALG_MODE_BPT;
	ppg_start_flag = true;
}

void MenuStartBpt(void)
{
	g_ppg_trigger |=TRIGGER_BY_MENU;
	g_ppg_alg_mode = ALG_MODE_BPT;
	ppg_start_flag = true;
}

void MenuStopBpt(void)
{
	g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_MENU);
	ppg_stop_flag = true;
}

void APPStartEcg(void)
{
	g_ppg_trigger |= TRIGGER_BY_APP;
	g_ppg_alg_mode = ALG_MODE_ECG;
	ppg_start_flag = true;
}

void MenuStartEcg(void)
{
	g_ppg_trigger |= TRIGGER_BY_MENU;
	g_ppg_alg_mode = ALG_MODE_ECG;
	ppg_start_flag = true;
}

void MenuStopEcg(void)
{
	g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_MENU);
	ppg_stop_flag = true;
}

void APPStartPPG(void)
{
	ppg_start_flag = true;
}

void MenuStartPPG(void)
{
	g_ppg_trigger |= TRIGGER_BY_MENU;
	ppg_start_flag = true;
}

void MenuStopPPG(void)
{
	g_ppg_trigger = 0;
	g_ppg_alg_mode = ALG_MODE_HR_SPO2;
	g_ppg_bpt_status = BPT_STATUS_GET_EST;
	ppg_stop_flag = true;
}

void PPGStartCheck(void)
{
	bool ret = false;

#ifdef PPG_DEBUG
	LOGD("ppg_power_flag:%d", ppg_power_flag);
#endif
	if(ppg_power_flag > 0)
		return;

	//Set_PPG_Power_On();
	SH_Power_On();

	ppg_power_flag = 1;

	ret = StartSensorhub();
	if(ret)
	{
		if(ppg_power_flag == 0)
		{
		#ifdef PPG_DEBUG
			LOGD("ppg hr has been stop!");
		#endif
			k_timer_stop(&ppg_get_hr_timer);
			return;
		}
	#ifdef PPG_DEBUG	
		LOGD("ppg hr start success!");
	#endif
		ppg_power_flag = 2;
		
		if((g_ppg_trigger&TRIGGER_BY_MENU) == 0)
			k_timer_start(&ppg_stop_timer, K_MSEC(60*1000), NULL);
	}
	else
	{
	#ifdef PPG_DEBUG
		LOGD("ppg hr start false!");
	#endif
		ppg_power_flag = 0;
	}
}

void PPGStopCheck(void)
{
	int status = -1;
#ifdef PPG_DEBUG
	LOGD("ppg_power_flag:%d", ppg_power_flag);
#endif
	if(ppg_power_flag == 0)
		return;

	k_timer_stop(&ppg_get_hr_timer);
		
	sensorhub_disable_sensor();
	sensorhub_disable_algo();

	SH_Power_Off();
	//Set_PPG_Power_Off();

	ppg_power_flag = 0;

	if((g_ppg_trigger&TRIGGER_BY_APP_ONE_KEY) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_APP_ONE_KEY);
		MCU_send_app_one_key_measure_data();
	}
	if((g_ppg_trigger&TRIGGER_BY_APP) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_APP);
		MCU_send_app_get_hr_data();
	}	
	if((g_ppg_trigger&TRIGGER_BY_MENU) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_MENU);
	}

	if((g_ppg_trigger&TRIGGER_BY_HOURLY) != 0)
	{
		g_ppg_trigger = g_ppg_trigger&(~TRIGGER_BY_HOURLY);
	}
}

void ppg_auto_stop_timerout(struct k_timer *timer_id)
{
	if((g_ppg_trigger&TRIGGER_BY_MENU) == 0)
		ppg_stop_flag = true;
}

void PPG_init(void)
{
#ifdef PPG_DEBUG
	LOGD("PPG_init");
#endif

	if(!sh_init_interface())
		return;

#ifdef PPG_DEBUG	
	LOGD("PPG_init done!");
#endif
}

void PPGMsgProcess(void)
{
	if(ppg_int_event)
	{
		ppg_int_event = false;
	}
	if(ppg_fw_upgrade_flag)
	{
		SH_OTA_upgrade_process();
		ppg_fw_upgrade_flag = false;
	}
	if(ppg_start_flag)
	{
	#ifdef PPG_DEBUG
		LOGD("PPG start!");
	#endif
		PPGStartCheck();
		ppg_start_flag = false;
	}
	if(ppg_stop_flag)
	{
	#ifdef PPG_DEBUG
		LOGD("PPG stop!");
	#endif
		PPGStopCheck();
		ppg_stop_flag = false;
	}
	
	if(ppg_get_cal_flag)
	{
		ppg_get_cal_flag = false;
	}
	
	if(ppg_get_data_flag)
	{
		PPGGetSensorHubData();
		ppg_get_data_flag = false;
	}
	if(ppg_redraw_data_flag)
	{
		PPGRedrawData();
		ppg_redraw_data_flag = false;
	}
}

