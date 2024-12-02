/*******************************************************************************
* Copyright (C) 2018 Maxim Integrated Products, Inc., All rights Reserved.
*
* This software is protected by copyright laws of the United States and
* of foreign countries. This material may also be protected by patent laws
* and technology transfer regulations of the United States and of foreign
* countries. This software is furnished under a license agreement and/or a
* nondisclosure agreement and may only be used or reproduced in accordance
* with the terms of those agreements. Dissemination of this information to
* any party or parties not specified in the license agreement and/or
* nondisclosure agreement is expressly prohibited.
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
*******************************************************************************
*/
#include <string.h>
#include "max_sh_api.h"
#include "max_sh_interface.h"
#include "logger.h"

typedef void (*parser) (void * , uint8_t *);

typedef enum{
	TIMESTAMP_DATA			=	0,
	MAX86176_MODE1_DATA,
	WHRM_SPO2_SUITE_MODE1,
	WHRM_SPO2_SUTITE_MODE2,
	ACCEL_MODE1_DATA,
	BPT_ALGO_DATA,
	RAW_ALGO_DATA,
	DATA_FORMAT_MAX
} data_format_t;

typedef struct{
	data_format_t data_type;
	int data_size;
	parser fp_parse;
} ss_instance_parser_t;

/* Data parser functions */
static void data_time_stamper(void * p_time_stamp, uint8_t* data_ptr);
void accel_data_rx(void * p_accel_data, uint8_t* data_ptr);
void max86176_data_rx(void * p_ppg_data, uint8_t* data_ptr);
void whrm_wspo2_suite_data_rx_mode1(void * p_algo_data_mode1, uint8_t* data_ptr);
void whrm_wspo2_suite_data_rx_mode2(void * p_algo_data_mode2, uint8_t* data_ptr);
void bpt_algo_data_rx(void * p_bpt_algo_data, uint8_t* data_ptr);
void raw_algo_data_rx(void * p_bpt_algo_data, uint8_t* data_ptr);

algo_sensor_status_t g_algo_sensor_stat;

static ss_instance_parser_t parsers[SH_NUM_CURRENT_ALGOS] =
{
		{TIMESTAMP_DATA,		 0, 									data_time_stamper					},
		{MAX86176_MODE1_DATA, 	 SSMAX86176_MODE1_DATASIZE,				max86176_data_rx					},
		{WHRM_SPO2_SUITE_MODE1,	 SSWHRM_WSPO2_SUITE_MODE1_DATASIZE,		whrm_wspo2_suite_data_rx_mode1		},
		{WHRM_SPO2_SUTITE_MODE2, SSWHRM_WSPO2_SUITE_MODE2_DATASIZE,		whrm_wspo2_suite_data_rx_mode2		},
		{ACCEL_MODE1_DATA,		 SSACCEL_MODE1_DATASIZE,				accel_data_rx                       },
		{BPT_ALGO_DATA,          SSBPT_ALGO_DATASIZE,		            bpt_algo_data_rx		            },
		{RAW_ALGO_DATA,          SSRAW_ALGO_DATASIZE,		            raw_algo_data_rx		            },
};

int sensorhub_get_output_scd_state(uint8_t *databuf, uint8_t *status)
{
	whrm_wspo2_suite_sensorhub_data *sample = (whrm_wspo2_suite_sensorhub_data *) databuf;

	*status = databuf[19];
}

int sensorhub_get_output_sample_number(int * p_number_of_sample)
{
	int ret = 0;

	if(NULL == p_number_of_sample)
	{
		ret = E_BAD_PARAM;
	}

	if(E_NO_ERROR == ret)
	{
		ret = sh_num_avail_samples(p_number_of_sample);
	}

	return ret;
}

int sensorhub_get_result(sensorhub_output *  p_result)
{
	int ret = 0;
	int num_bytes_to_read = 0;
	uint8_t hubStatus = 0;
	uint8_t databuf[256];
	uint8_t *ptr;

	if(NULL == p_result)
	{
		ret = E_BAD_PARAM;
	}

	if(E_NO_ERROR == ret)
	{
		ret = sh_get_sensorhub_status(&hubStatus);
		//LOGD("sh_get_sensorhub_status ret:%d", ret);
	}

	//LOGD("sensorhub status:%d", hubStatus);

	if(hubStatus & SS_MASK_STATUS_DATA_RDY)
	{
		//LOGD("SS_MASK_STATUS_DATA_RDY");
	}
	else
	{
		ret = E_NONE_AVAIL;
	}

	if(hubStatus & SS_MASK_STATUS_FIFO_OUT_OVR)
	{
		//LOGD("SS_MASK_STATUS_FIFO_OUT_OVR");
	}

	if(0 == ret)
	{
		memset(p_result, 0, sizeof(sensorhub_output));
		memset(databuf, 0, sizeof(databuf));

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

		int avail_samples;
		if(hubStatus & SS_MASK_STATUS_DATA_RDY) {
			sh_num_avail_samples(&avail_samples);
			//LOGD("ava samp = %d", avail_samples );
		}
		ptr  = &databuf[2];

		WAIT_MS(5);

		ret = sh_read_fifo_data(1, num_bytes_to_read, databuf, sizeof(databuf));

		if(g_algo_sensor_stat.accel_enabled)
		{
			parsers[ACCEL_MODE1_DATA].fp_parse(&p_result->acc_data, ptr);
			ptr += parsers[ACCEL_MODE1_DATA].data_size;
		}

		if(g_algo_sensor_stat.max86176_enabled)
		{
			parsers[MAX86176_MODE1_DATA].fp_parse(&p_result->ppg_data, ptr);
			ptr += parsers[MAX86176_MODE1_DATA].data_size;
		}

		if(g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1)
		{
			parsers[WHRM_SPO2_SUITE_MODE1].fp_parse(&p_result->algo_data, ptr);
			ptr += parsers[WHRM_SPO2_SUITE_MODE1].data_size;
		}

		if(g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2)
		{
			parsers[WHRM_SPO2_SUTITE_MODE2].fp_parse(&p_result->algo_data, ptr);
			ptr += parsers[WHRM_SPO2_SUTITE_MODE2].data_size;
		}

		if(g_algo_sensor_stat.bpt_algo_enabled)
		{
           //MYG add to parsers
			parsers[BPT_ALGO_DATA].fp_parse(&p_result->bpt_data, ptr);
			ptr += parsers[BPT_ALGO_DATA].data_size;
		}

		if(g_algo_sensor_stat.algo_raw_enabled)
		{
           //MYG add to parsers
			parsers[RAW_ALGO_DATA].fp_parse(&p_result->bpt_data, ptr);
			ptr += parsers[RAW_ALGO_DATA].data_size;
		}		
	}
	else
	{
		ret = E_NONE_AVAIL;
	}

	return ret;
}

int sensorhub_get_version(uint8_t algoVersion[3])
{
	int ret = 0;
	ret = sh_get_algo_version(algoVersion);

	return ret;
}

int sensorhub_reset_sensor_configuration()
{
	int ret = 0;

	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x10, 0x01, 1);

	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x10, (0x01 << 1), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x15, (0x1E), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x18, (0x80), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x19, (0x9F), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x1A, (0x3F), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x1C, (0x1 << 5), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x1D, (0x05), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x1E, (0x1F), 1);
	ret |= sh_set_reg(SH_SENSORIDX_MAX86176, 0x86, (0x01 << 6), 1);

	return ret;
}

static void data_time_stamper(void * p_time_stamp, uint8_t* data_ptr)
{

}

void accel_data_rx(void * p_accel_data, uint8_t* data_ptr)
{
	accel_data * sample = (accel_data  *)p_accel_data;

	sample->x = (data_ptr[0] << 8) | data_ptr[1];
	sample->y = (data_ptr[2] << 8) | data_ptr[3];
	sample->z = (data_ptr[4] << 8) | data_ptr[5];

	//LOGD("x:%d, y:%d, z:%d", sample->x, sample->y, sample->z);
}

void max86176_data_rx(void * p_ppg_data, uint8_t* data_ptr)
{
	max86176_data * sample = (max86176_data *)p_ppg_data;

	sample->led1 = ((data_ptr[0] << 16) | (data_ptr[1] << 8) | data_ptr[2]) & 0xFFFFF;		//green1, PD1
	sample->led2 = ((data_ptr[3] << 16) | (data_ptr[4] << 8) | data_ptr[5]) & 0xFFFFF;		//green2, PD2
	sample->led3 = ((data_ptr[6] << 16) | (data_ptr[7] << 8) | data_ptr[8]) & 0xFFFFF;  	//IR, PD1
	sample->led4 = ((data_ptr[9] << 16) | (data_ptr[10] << 8) | data_ptr[11]) & 0xFFFFF;	//IR, PD2
	sample->led5 = ((data_ptr[12] << 16) | (data_ptr[13] << 8) | data_ptr[14]) & 0xFFFFF;	//red, PD1
	sample->led6 = ((data_ptr[15] << 16) | (data_ptr[16] << 8) | data_ptr[17]) & 0xFFFFF; 	//red, PD2

	//LOGD("led1:%d, led2:%d, led3:%d, led4:%d, led5:%d, led6:%d", sample->led1, sample->led2, sample->led3, sample->led4, sample->led5, sample->led6);	
}

void whrm_wspo2_suite_data_rx_mode1(void * p_algo_data_mode1, uint8_t* data_ptr)
{
	whrm_wspo2_suite_sensorhub_data *sample = (whrm_wspo2_suite_sensorhub_data *) p_algo_data_mode1;

	sample->current_operating_mode = data_ptr[0];

	/* WHRM Data */
	sample->hr = (data_ptr[1] << 8) | data_ptr[2];
	sample->hr_conf = data_ptr[3];
	sample->rr = (data_ptr[4] << 8) | data_ptr[5];
	sample->rr_conf = data_ptr[6];
	sample->activity_class = data_ptr[7];

	/* WSPO2 Data */
	sample->r = (data_ptr[8] << 8) | data_ptr[9];
	sample->spo2_conf = data_ptr[10];
	sample->spo2 = (data_ptr[11] << 8) | data_ptr[12]; // already x10;
	sample->percentComplete = data_ptr[13];
	sample->lowSignalQualityFlag = data_ptr[14];
	sample->motionFlag = data_ptr[15];
	sample->lowPiFlag = data_ptr[16];
	sample->unreliableRFlag = data_ptr[17];
	sample->spo2State = data_ptr[18];
	sample->scd_contact_state = data_ptr[19];

	//LOGD("HR:%d, SPO2:%d", sample->hr, sample->spo2);
}

void whrm_wspo2_suite_data_rx_mode2(void * p_algo_data_mode2, uint8_t* data_ptr)
{
	whrm_wspo2_suite_sensorhub_data *sample = (whrm_wspo2_suite_sensorhub_data *) p_algo_data_mode2;

	sample->current_operating_mode = data_ptr[0];

	/* WHRM Data */
	sample->hr = (data_ptr[1] << 8) | data_ptr[2];
	sample->hr_conf = data_ptr[3];
	sample->rr = (data_ptr[4] << 8) | data_ptr[5];
	sample->rr_conf = data_ptr[6];
	sample->activity_class = data_ptr[7];

	/* WSPO2 Data */
	sample->r = (data_ptr[8] << 8) | data_ptr[9];
	sample->spo2_conf = data_ptr[10];
	sample->spo2 = (data_ptr[11] << 8) | data_ptr[12]; // already x10;
	sample->percentComplete = data_ptr[13];
	sample->lowSignalQualityFlag = data_ptr[14];
	sample->motionFlag = data_ptr[15];
	sample->lowPiFlag = data_ptr[16];
	sample->unreliableRFlag = data_ptr[17];
	sample->spo2State = data_ptr[18];
	sample->scd_contact_state = data_ptr[19];

	/* Extended report */
	sample->walk_steps = (data_ptr[20] << 24) | (data_ptr[21]  << 16) | (data_ptr[22] << 8) | data_ptr[23];
	sample->run_steps = (data_ptr[24] << 24) | (data_ptr[25] << 16) | (data_ptr[26] << 8) | data_ptr[27];
	sample->kcal = (data_ptr[28] << 24) | (data_ptr[29] << 16) | (data_ptr[30] << 8) | data_ptr[31];
	sample->totalActEnergy = (data_ptr[32] << 24) | (data_ptr[33] << 16) | (data_ptr[34] << 8) | data_ptr[35];

	int ind = SH_PPG_SIGNAL_GREEN0;
	int idx = 35;
	for(ind = SH_PPG_SIGNAL_GREEN0; ind < SH_PPG_SIGNAL_MAX; ++ind)
	{
		uint16_t curr = (data_ptr[idx+1] << 8) | data_ptr[idx+2];
		uint8_t int_time = data_ptr[idx+3];
		uint8_t avg = data_ptr[idx+4];
		uint8_t dac_off = data_ptr[idx+5];

		sample->afe_requests[ind].led_curr.is_led_curr_update_requested = (curr >> 15);
		sample->afe_requests[ind].led_curr.requested_led_curr = (curr & 0x7FFF);

		sample->afe_requests[ind].int_time.is_int_time_update_requested = (int_time >> 7);
		sample->afe_requests[ind].int_time.requested_int_time = (int_time & 0x7F);

		sample->afe_requests[ind].sample_averaging.is_averaging_update_requested = (avg >> 7);
		sample->afe_requests[ind].sample_averaging.requested_avg = (avg & 0x7F);

		sample->afe_requests[ind].dac_offset.is_dac_offset_update_requested = (dac_off >> 7);
		sample->afe_requests[ind].dac_offset.dac_offset = (dac_off & 0x7F);

		idx += 5;
	}


	sample->hrm_afe_state = data_ptr[56];
	sample->is_high_motion = data_ptr[57];
}

void bpt_algo_data_rx(void * p_bpt_algo_data, uint8_t* data_ptr)
{
	bpt_sensorhub_data *sample = (bpt_sensorhub_data *) p_bpt_algo_data;

	sample->status    =  data_ptr[0];
	sample->perc_comp =  data_ptr[1];
	sample->sys_bp    =  data_ptr[2];
	sample->dia_bp    =  data_ptr[3];
	sample->flags     =  (data_ptr[4] << 8) | data_ptr[5];
	sample->reserved  =  (data_ptr[6] << 8) | data_ptr[7];


	uint16_t bpt_flags = (data_ptr[4] << 8) | data_ptr[5];
	uint8_t est_error = bpt_flags & 0b0000000000100000;

	//LOGD("bptr = %d %d %d %d %d" ,  data_ptr[0] ,  data_ptr[1] , data_ptr[2] , data_ptr[3], est_error);
	//LOGD("bpt exec cnt = %" , (int)((data_ptr[6] << 8) | data_ptr[7]));
}

void raw_algo_data_rx( void * p_bpt_algo_data, uint8_t* data_ptr)
{
	return;
}

