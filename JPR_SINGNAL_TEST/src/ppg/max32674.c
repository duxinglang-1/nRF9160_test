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

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(ppg, CONFIG_LOG_DEFAULT_LEVEL);

bool ppg_int_event = false;
bool ppg_is_power_on = false;
bool ppg_fw_upgrade_flag = false;
bool ppg_start_flag = false;
bool ppg_stop_flag = false;
bool ppg_get_hr_spo2 = false;
bool ppg_redraw_data_flag = false;

static u8_t whoamI=0, rst=0;

ppgdev_ctx_t ppg_dev_ctx;

u16_t g_hr = 0;
u16_t g_spo2 = 0;
u64_t timestamp = 0;


static void ppg_get_hr_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(ppg_get_hr_timer, ppg_get_hr_timerout, NULL);

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

void GetPPGVersion(ppgdev_ctx_t *ctx, u8_t *buff)
{
	//max32674_read_reg(ctx, 0x81, 0x01, buff, 1);

	LOG_INF("[%s] version:%x\n", __func__, buff);
}

void PPGRedrawData(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_HEALTH;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void ppg_get_hr_timerout(struct k_timer *timer_id)
{
	ppg_get_hr_spo2 = true;
}

void PPGGetSensorHubData(void)
{
	int ret = 0;
	int num_bytes_to_read = 0;
	u8_t hubStatus = 0;
	u8_t databuf[150] = {0};
	whrm_wspo2_suite_sensorhub_data sensorhub_out = {0};
	accel_data accel = {0};
	max86176_data max86176 = {0};

	ret = sh_get_sensorhub_status(&hubStatus);
	LOG_INF("sh_get_sensorhub_status ret = %d, hubStatus =  %d \n", ret, hubStatus);

	if(hubStatus & SS_MASK_STATUS_FIFO_OUT_OVR)
	{
		LOG_INF("SS_MASK_STATUS_FIFO_OUT_OVR\n");
	}

	if((0 == ret) && (hubStatus & SS_MASK_STATUS_DATA_RDY))
	{
		LOG_INF("FIFO ready \n");

		int u32_sampleCnt = 0;

		num_bytes_to_read = SS_PACKET_COUNTERSIZE + SSMAX86176_MODE1_DATASIZE + SSACCEL_MODE1_DATASIZE + SSWHRM_WSPO2_SUITE_MODE1_DATASIZE;

		ret = sensorhub_get_output_sample_number(&u32_sampleCnt);
		if(ret == SS_SUCCESS)
		{
			LOG_INF("sample count is %d \n", u32_sampleCnt);
		}
		else
		{
			LOG_INF("read sample count fail %d \n", ret);
		}

		WAIT_MS(5);

		ret = sh_read_fifo_data(u32_sampleCnt, num_bytes_to_read, databuf, sizeof(databuf));
		if(ret == SS_SUCCESS)
		{
		#if 0
			LOG_INF("read FIFO result success!\n");

			for (int i = 0; i< num_bytes_to_read; i ++)
			{
				LOG_INF("%x,", databuf[i]);
			}
			LOG_INF("\n");
		#endif

			u32_t i,index = 0;

			for(i=0;i<u32_sampleCnt;i++)
			{
				index = i * SS_NORMAL_PACKAGE_SIZE + 1;
				
				accel_data_rx(&accel, &databuf[index+SS_PACKET_COUNTERSIZE]);
				max86176_data_rx(&max86176, &databuf[index+SS_PACKET_COUNTERSIZE + SSACCEL_MODE1_DATASIZE]);
				whrm_wspo2_suite_data_rx_mode1(&sensorhub_out, &databuf[index+SS_PACKET_COUNTERSIZE + SSMAX86176_MODE1_DATASIZE + SSACCEL_MODE1_DATASIZE] );

				g_hr = sensorhub_out.hr/10 + ((sensorhub_out.hr%10 > 4) ? 1 : 0);
				g_spo2 = sensorhub_out.spo2/10 + ((sensorhub_out.spo2%10 > 4) ? 1 : 0);

				LOG_INF("hr:%d, SpO2:%d\n",  sensorhub_out.hr, sensorhub_out.spo2);
				LOG_INF("g_hr:%d, g_spo2:%d\n",  g_hr, g_spo2);
			}

		}
		else
		{
			LOG_INF("read FIFO result fail %d \n", ret);
		}
	}
	else
	{
		LOG_INF(" FIFO status is not ready  %d, %d \n", ret, hubStatus);
	}	
}

#if 1
bool demoSensorhub(void)
{
	int status = -1;
	u8_t hubMode = 0x00;

	status = sh_get_sensorhub_operating_mode(&hubMode);
	if((hubMode != 0x00) && (status != SS_SUCCESS))
	{
		LOG_INF("work mode is not app mode %d \n", hubMode);
		return false;
	}
	else
	{
		LOG_INF("work mode is app mode %d \n", hubMode);
	}

	status = sh_set_data_type(SS_DATATYPE_BOTH, true);
	if(status != SS_SUCCESS)
	{
		LOG_INF("sh_set_data_type eorr %d \n", status);
		return false;
	}

	status = sh_set_fifo_thresh(1);
	if(status != SS_SUCCESS)
	{
		LOG_INF("sh_set_fifo_thresh eorr %d \n", status);
		return false;
	}

	status = sh_set_report_period(25);  //1Hz or 25Hz
	if(status != SS_SUCCESS)
	{
		LOG_INF("sh_set_fifo_thresh eorr %d \n", status);
		return false;
	}

	status = sh_sensor_enable_(SH_SENSORIDX_ACCEL, 1, SH_INPUT_DATA_DIRECT_SENSOR);
	if (status != SS_SUCCESS)
	{
		LOG_INF("sh_sensor_enable_ACC eorr %d \n", status);
		return false;
	}

	status = sh_sensor_enable_(SH_SENSORIDX_MAX86176, 1, SH_INPUT_DATA_DIRECT_SENSOR);
	if(status != SS_SUCCESS)
	{
		LOG_INF("sh_sensor_enable_MAX86176 eorr %d \n", status);
		return false;
	}

	//set algorithm operation mode
	sh_set_cfg_wearablesuite_algomode(0x0);

	status = sh_enable_algo_(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X , (int) SENSORHUB_MODE_BASIC);
	if(status != SS_SUCCESS)
	{
		LOG_INF("sh_sensor_enable_MAX86176 eorr %d \n", status);
		return false;
	}

	//config a timer to poll FIFO
	//set timer to 5 seconds
	k_timer_start(&ppg_get_hr_timer, K_MSEC(1*1000), K_MSEC(1*1000));

	return true;
}

#else
void demoSensorhub(void)
{
	int ret = 0;
	sensorhub_output sensorhub_out;

	memset(&sensorhub_out, 0, sizeof(sensorhub_output));

	ret = sensorhub_interface_init();
	LOG_INF("sensorhub_interface_init ret %d \n", ret);

	ret = sensorhub_enable_sensors();
	LOG_INF("sensorhub_enable_sensors ret %d \n", ret);

	ret = sensorhub_enable_algo(SENSORHUB_MODE_BASIC);
	LOG_INF("sensorhub_enable_algo ret %d \n", ret);

	while(1)
	{
		ret = sensorhub_get_result(&sensorhub_out);
		LOG_INF("sensorhub_get_result ret %d \n", ret);

		if(0 == ret)
		{
			LOG_INF("HR:%d, SpO2:%d\n",  sensorhub_out.algo_data.hr, sensorhub_out.algo_data.spo2);
		}

		k_sleep(K_MSEC(100));
	}
}
#endif

void PPGStopCheck(void)
{
	int status = -1;

	if(!ppg_is_power_on)
		return;
	
	status = sh_sensor_disable(SH_SENSORIDX_MAX86176);
	if(status != SS_SUCCESS)
		LOG_INF("sh_sensor_disble_MAX86176 eorr %d \n", status);

	status = sh_sensor_disable(SH_SENSORIDX_ACCEL);
	if(status != SS_SUCCESS)
		LOG_INF("sh_sensor_disable_ACC eorr %d \n", status);

	status = sh_disable_algo(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X);
	if(status != SS_SUCCESS)
		LOG_INF("sh_sensor_disble_algo eorr %d \n", status);

	k_timer_stop(&ppg_get_hr_timer);
	timestamp = k_uptime_get();
	ppg_is_power_on = false;
}

void PPGStartCheck(void)
{
	bool ret = false;
	u64_t starttime;
	
	if(ppg_is_power_on)
		return;

	starttime = k_uptime_get();
	if((starttime-timestamp) < 8000)
		return;
	
	ret = demoSensorhub();
	if(ret)
	{
		ppg_is_power_on = true;
	}
}

void PPG_init(void)
{
	LOG_INF("PPG_init\n");

	if(!sh_init_interface())
		return;
	
	LOG_INF("PPG_init done!\n");
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
		LOG_INF("PPG start!\n");
		PPGStartCheck();
		ppg_start_flag = false;
	}
	if(ppg_stop_flag)
	{
		LOG_INF("PPG stop!\n");
		PPGStopCheck();
		ppg_stop_flag = false;
	}
	if(ppg_get_hr_spo2)
	{
		PPGGetSensorHubData();
		ppg_get_hr_spo2 = false;
		ppg_redraw_data_flag = true;
	}
	if(ppg_redraw_data_flag)
	{
		PPGRedrawData();
		ppg_redraw_data_flag = false;
	}
	
	//GetPPGVersion(&ppg_dev_ctx, &whoamI);
	//k_sleep(K_MSEC(1000));
}
