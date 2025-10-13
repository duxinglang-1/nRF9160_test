/****************************************Copyright (c)************************************************
** File Name:			    inner_flash.c
** Descriptions:			nrf9160 inner flash management source file
** Created By:				xie biao
** Created Date:			2021-01-07
** Modified Date:      		2021-07-12
** Version:			    	V1.1
******************************************************************************************************/
#include <zephyr/fs/nvs.h>
#include <flash_map_pm.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "inner_flash.h"
#ifdef CONFIG_IMU_SUPPORT
#include "lsm6dso.h"
#endif
#ifdef CONFIG_FACTORY_TEST_SUPPORT
#include "ft_main.h"
#endif
#include "logger.h"

//#define INNER_FLASH_DEBUG

#define STORAGE_NODE_LABEL storage
#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define value1  "53.760241,-5.147095,1.023,11:20:22"
#define value2  "53.760241,-5.147095,1.023,11:20:23"
#define value3  "53.760241,-5.147095,1.023,11:20:24"
#define value4  "53.760241,-5.147095,1.023,11:20:25"
#define value5  "53.760241,-5.147095,1.023,11:20:26"
#define value6  "53.760241,-5.147095,1.023,11:20:27"
#define value7  "53.760241,-5.147095,1.023,11:20:28"
#define value8  "53.760241,-5.147095,1.023,11:20:29"
#define value9  "53.760241,-5.147095,1.023,11:20:30"
#define value10  "53.760241,-5.147095,1.023,11:20:31"
#define value11  "53.760241,-5.147095,1.023,11:20:32"
#define value12  "53.760241,-5.147095,1.023,11:20:33"

static const char results[][60] = { value1,value2,value3,value4,value5,value6,value7,value8,value9,value10,value11,value12};

static bool nvs_init_flag = false;

static struct nvs_fs fs;
static struct flash_pages_info info;

sport_record_t last_sport = {0};
health_record_t last_health = {0};

static int nvs_setup(void)
{	
    struct device *flash_dev;
	int err;	

	//flash_dev = FLASH_AREA_DEVICE(STORAGE_NODE_LABEL);
	fs.flash_device = NVS_PARTITION_DEVICE;
	if(!device_is_ready(fs.flash_device))
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("Flash device %s is not ready", fs.flash_device->name);
	#endif	
		return;
	}
	fs.offset = NVS_PARTITION_OFFSET;
	err = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (err)
	{		
	#ifdef INNER_FLASH_DEBUG
		LOGD("Unable to get page info");
	#endif
		return;
	}

	fs.sector_size = info.size;
	fs.sector_count = 6U;

    err = nvs_mount(&fs);
	if(err)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("Flash Init failed");
	#endif
	}

	nvs_init_flag = true;
	return err;
}

void ReadSettingsFromInnerFlash(global_settings_t *settings)
{
	int err = 0;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return;
		}
	}

	err = nvs_read(&fs, SETTINGS_ID, settings, sizeof(global_settings_t));
	if(err < 0)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("get settins err:%d", err);
	#endif
	}
}

void ResetInnerFlash(void)
{
	int err;
	
	nvs_clear(&fs);

	err = nvs_setup();
	if(err)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("Flash Init failed, return!");
	#endif
		return;
	}
}

void SaveSettingsToInnerFlash(global_settings_t settings)
{
	nvs_write(&fs, SETTINGS_ID, &settings, sizeof(global_settings_t));
}

#ifdef CONFIG_FACTORY_TEST_SUPPORT
void ReadFtStatusFromInnerFlash(uint8_t *status)
{
	nvs_read(&fs, FT_STATUS_ID, status, sizeof(uint8_t));
}

void ReadFtResultsFromInnerFlash(FT_STATUS status, void *results)
{
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return;
		}
	}

	switch(status)
	{
	case FT_STATUS_SMT:
		nvs_read(&fs, FT_SMT_RESULTS_ID, results, sizeof(ft_smt_results_t));
		break;
		
	case FT_STATUS_ASSEM:
		nvs_read(&fs, FT_ASSEM_RESULTS_ID, results, sizeof(ft_assem_results_t));
		break;
	}
}

void SaveFtStatusToInnerFlash(uint8_t status)
{
	nvs_write(&fs, FT_STATUS_ID, &status, sizeof(uint8_t));
}

void SaveFtResultsToInnerFlash(FT_STATUS status, void *results)
{
	switch(status)
	{
	case FT_STATUS_SMT:
		nvs_write(&fs, FT_SMT_RESULTS_ID, results, sizeof(ft_smt_results_t));
		break;
		
	case FT_STATUS_ASSEM:
		nvs_write(&fs, FT_ASSEM_RESULTS_ID, results, sizeof(ft_assem_results_t));
		break;
	}
}
#endif

void ReadDateTimeFromInnerFlash(sys_date_timer_t *time)
{
	int err = 0;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return;
		}
	}

	err = nvs_read(&fs, DATETIME_ID, time, sizeof(sys_date_timer_t));
	if(err < 0)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("get datetime err:%d", err);
	#endif
	}
}

void SaveDateTimeToInnerFlash(sys_date_timer_t time)
{
	nvs_write(&fs, DATETIME_ID, &time, sizeof(sys_date_timer_t));
}

void test_nvs(void)
{
	int err;
	char nvs_rx_buff[39] = {0};
	ssize_t bytes_written,bytes_read,freespace;
	
	err = nvs_setup();
	if(err)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("nvs_setup failed");
	#endif
	}

	freespace = nvs_calc_free_space(&fs);

#ifdef INNER_FLASH_DEBUG	
	LOGD("Remaining free space in nvs sector is %d Bytes", freespace);
#endif

	for(int i=0; i<ARRAY_SIZE(results); i++)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("Writing %s to NVS", results[i]);
	#endif
	
		bytes_written = nvs_write(&fs, i, results[i], strlen(results[i]));

	#ifdef INNER_FLASH_DEBUG
		LOGD("Bytes written to nvs: %d at ID %d", bytes_written, i);
	#endif

		freespace = nvs_calc_free_space(&fs);

	#ifdef INNER_FLASH_DEBUG
		LOGD("Remaining free space in nvs sector is %d Bytes", freespace);
	#endif
	}

	k_sleep(K_MSEC(5000));

	for(int i=0; i<ARRAY_SIZE(results); i++)
	{
		bytes_read = nvs_read(&fs, i, nvs_rx_buff, sizeof(nvs_rx_buff));
	#ifdef INNER_FLASH_DEBUG
		LOGD("Bytes read from nvs: %d at ID %d", bytes_read, i);
		LOGD("Data read from nvs: %s at ID %d", nvs_rx_buff, i);
	#endif
	}
}

bool clear_current_data_in_record(ENUM_RECORD_TYPE record_type)
{
	ssize_t bytes_written;
	uint16_t addr_id;
	uint32_t data_len;
	imu_data_u data = {0};
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}

	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		addr_id = CUR_LOCAL_ID;
		data_len = sizeof(local_record_t);
		break;

	case RECORD_TYPE_HEALTH:
		addr_id = CUR_HEALTH_ID;
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		addr_id = CUR_SPORT_ID;
		data_len = sizeof(sport_record_t);
		break;
	}

	bytes_written = nvs_write(&fs, addr_id, &data, data_len);
	if(bytes_written <= 0)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("save current %d_data fail!", record_type);
	#endif
		return false;
	}

#ifdef INNER_FLASH_DEBUG
	LOGD("save current %d_data success!", record_type);
#endif
	return true;	
}

void clear_cur_local_in_record(void)
{
	clear_current_data_in_record(RECORD_TYPE_LOCATION);
}

void clear_cur_health_in_record(void)
{
	clear_current_data_in_record(RECORD_TYPE_HEALTH);
}

#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
void clear_cur_sport_in_record(void)
{
	memset(&last_sport, 0, sizeof(last_sport));
	
	clear_current_data_in_record(RECORD_TYPE_SPORT);
}
#endif

bool save_current_data_to_record(void *data, ENUM_RECORD_TYPE record_type)
{
	ssize_t bytes_written;
	uint16_t addr_id;
	uint32_t data_len;
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}

	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		addr_id = CUR_LOCAL_ID;
		data_len = sizeof(local_record_t);
		break;

	case RECORD_TYPE_HEALTH:
		addr_id = CUR_HEALTH_ID;
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		addr_id = CUR_SPORT_ID;
		data_len = sizeof(sport_record_t);
		break;
	}

	bytes_written = nvs_write(&fs, addr_id, data, data_len);
	if(bytes_written <= 0)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("save current %d_data fail!", record_type);
	#endif
		return false;
	}

#ifdef INNER_FLASH_DEBUG
	LOGD("save current %d_data success!", record_type);
#endif
	return true;	
}

bool save_cur_local_to_record(local_record_t *local_data)
{
	return save_current_data_to_record(local_data, RECORD_TYPE_LOCATION);
}

bool save_cur_health_to_record(health_record_t *health_data)
{
	return save_current_data_to_record(health_data, RECORD_TYPE_HEALTH);
}

#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
bool save_cur_sport_to_record(sport_record_t *sport_data)
{
	return save_current_data_to_record(sport_data, RECORD_TYPE_SPORT);
}
#endif

bool get_current_data_from_record(void *data, ENUM_RECORD_TYPE record_type)
{
	ssize_t bytes_read;
	uint16_t addr_id;
	uint32_t data_len;
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}
	
	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		addr_id = CUR_LOCAL_ID;
		data_len = sizeof(local_record_t);
		break;

	case RECORD_TYPE_HEALTH:
		addr_id = CUR_HEALTH_ID;
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		addr_id = CUR_SPORT_ID;
		data_len = sizeof(sport_record_t);
		break;
	}

	bytes_read = nvs_read(&fs, addr_id, data, data_len);
	if(bytes_read <= 0)
	{
	#ifdef INNER_FLASH_DEBUG
		LOGD("get current %d_data fail!", record_type);
	#endif
		return false;
	}

#ifdef INNER_FLASH_DEBUG
	LOGD("get current %d_data success!", record_type);
#endif
	return true;	
}

bool get_cur_local_from_record(local_record_t *local_data)
{
	return get_current_data_from_record(local_data, RECORD_TYPE_LOCATION);
}

bool get_cur_health_from_record(health_record_t *health_data)
{
	return get_current_data_from_record(health_data, RECORD_TYPE_HEALTH);
}

#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
bool get_cur_sport_from_record(sport_record_t *sport_data)
{
	return get_current_data_from_record(sport_data, RECORD_TYPE_SPORT);
}
#endif

bool save_data_to_record(void *data, ENUM_RECORD_TYPE record_type)
{
	uint16_t nvs_rx=0;
	ssize_t bytes_written,bytes_read;
	uint16_t index_addr_id,count_addr_id;
	uint16_t index_begin,index_max,count_max;
	uint16_t tmp_index,tmp_count;
	uint32_t data_len;
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}
	
	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		index_addr_id = LOCAL_INDEX_ADDR_ID;
		count_addr_id = LOCAL_COUNT_ADDR_ID;
		index_begin = LOCAL_INDEX_BEGIN;
		index_max = LOCAL_INDEX_MAX;
		count_max = (LOCAL_INDEX_MAX-LOCAL_INDEX_BEGIN);
		data_len = sizeof(local_record_t);
		break;

	case RECORD_TYPE_HEALTH:
		index_addr_id = HEALTH_INDEX_ADDR_ID;
		count_addr_id = HEALTH_COUNT_ADDR_ID;
		index_begin = HEALTH_INDEX_BEGIN;
		index_max = HEALTH_INDEX_MAX;
		count_max = (HEALTH_INDEX_MAX-HEALTH_INDEX_BEGIN);
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		index_addr_id = SPORT_INDEX_ADDR_ID;
		count_addr_id = SPORT_COUNT_ADDR_ID;
		index_begin = SPORT_INDEX_BEGIN;
		index_max = SPORT_INDEX_MAX;
		count_max = (SPORT_INDEX_MAX-SPORT_INDEX_BEGIN);
		data_len = sizeof(sport_record_t);
		break;
	}

	bytes_read = nvs_read(&fs, index_addr_id, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
	{
		tmp_index = index_begin;
		tmp_count = 0;
	}
	else
	{
		//���һ����¼��ID
		tmp_index = nvs_rx;
		
		bytes_read = nvs_read(&fs, count_addr_id, &nvs_rx, sizeof(nvs_rx));
		if(bytes_read <= 0)
			return false;
		//�洢��������
		tmp_count = nvs_rx;
	}

	if(tmp_count == count_max)
	{
		//�Ѿ��������ӵ�һ��λ�ø��Ǵ洢��ע��洢�������ʼ��ż�1
		if(tmp_index == index_max)
			tmp_index = index_begin;
		tmp_index += 1;
			
		bytes_written = nvs_write(&fs, tmp_index, data, data_len);
		if(bytes_written <= 0)
			return false;

		bytes_written = nvs_write(&fs, index_addr_id, &tmp_index, sizeof(tmp_index));
		if(bytes_written <= 0)
			return false;
	}
	else
	{
		//δ�������ڵ�ǰ���һ����¼����һ��λ�ô洢����
		tmp_index += 1;
		tmp_count += 1;
		bytes_written = nvs_write(&fs, tmp_index, data, data_len);
		if(bytes_written <= 0)
			return false;

		bytes_written = nvs_write(&fs, index_addr_id, &tmp_index, sizeof(tmp_index));
		if(bytes_written <= 0)
			return false;
		
		bytes_written = nvs_write(&fs, count_addr_id, &tmp_count, sizeof(tmp_count));
		if(bytes_written <= 0)
			return false;
	}

	return true;	
}

bool save_local_to_record(local_record_t *local_data)
{
	return save_data_to_record(local_data, RECORD_TYPE_LOCATION);
}

bool save_health_to_record(health_record_t *health_data)
{
	return save_data_to_record(health_data, RECORD_TYPE_HEALTH);
}

bool save_sport_to_record(sport_record_t *sport_data)
{
	return save_data_to_record(sport_data, RECORD_TYPE_SPORT);
}

bool clear_data_in_record(ENUM_RECORD_TYPE record_type)
{
	uint16_t nvs_rx=0;
	ssize_t bytes_written,bytes_read;
	uint16_t index_addr_id,count_addr_id;
	uint16_t index_begin,index_max,count_max;
	uint16_t tmp_index,tmp_count;
	uint32_t data_len;
	imu_data_u data = {0};
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}
	
	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		index_addr_id = LOCAL_INDEX_ADDR_ID;
		count_addr_id = LOCAL_COUNT_ADDR_ID;
		index_begin = LOCAL_INDEX_BEGIN;
		index_max = LOCAL_INDEX_MAX;
		count_max = (LOCAL_INDEX_MAX-LOCAL_INDEX_BEGIN);
		data_len = sizeof(local_record_t);
		break;

	case RECORD_TYPE_HEALTH:
		index_addr_id = HEALTH_INDEX_ADDR_ID;
		count_addr_id = HEALTH_COUNT_ADDR_ID;
		index_begin = HEALTH_INDEX_BEGIN;
		index_max = HEALTH_INDEX_MAX;
		count_max = (HEALTH_INDEX_MAX-HEALTH_INDEX_BEGIN);
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		index_addr_id = SPORT_INDEX_ADDR_ID;
		count_addr_id = SPORT_COUNT_ADDR_ID;
		index_begin = SPORT_INDEX_BEGIN;
		index_max = SPORT_INDEX_MAX;
		count_max = (SPORT_INDEX_MAX-SPORT_INDEX_BEGIN);
		data_len = sizeof(sport_record_t);
		break;
	}

	tmp_index = index_begin;
	tmp_count = 0;
	
	bytes_written = nvs_write(&fs, tmp_index, &data, data_len);
	if(bytes_written <= 0)
		return false;

	bytes_written = nvs_write(&fs, index_addr_id, &tmp_index, sizeof(tmp_index));
	if(bytes_written <= 0)
		return false;
	
	bytes_written = nvs_write(&fs, count_addr_id, &tmp_count, sizeof(tmp_count));
	if(bytes_written <= 0)
		return false;

	return true;	
}

void clear_local_in_record(void)
{
	clear_data_in_record(RECORD_TYPE_LOCATION);
}

void clear_health_in_record(void)
{
	clear_data_in_record(RECORD_TYPE_HEALTH);
}

#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
void clear_sport_in_record(void)
{
	clear_data_in_record(RECORD_TYPE_SPORT);
}
#endif

bool get_date_from_record(void *databuf, uint32_t index, ENUM_RECORD_TYPE record_type)
{
	uint16_t nvs_rx=0;
	ssize_t bytes_read;
	uint16_t index_addr_id,count_addr_id;
	uint16_t index_begin,index_max,count_max;
	uint16_t tmp_index,tmp_count;
	uint32_t data_len;
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}
	
	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		index_addr_id = LOCAL_INDEX_ADDR_ID;
		count_addr_id = LOCAL_COUNT_ADDR_ID;
		index_begin = LOCAL_INDEX_BEGIN;
		index_max = LOCAL_INDEX_MAX;
		count_max = (LOCAL_INDEX_MAX-LOCAL_INDEX_BEGIN);
		data_len = sizeof(local_record_t);
		break;
		
	case RECORD_TYPE_HEALTH:
		index_addr_id = HEALTH_INDEX_ADDR_ID;
		count_addr_id = HEALTH_COUNT_ADDR_ID;
		index_begin = HEALTH_INDEX_BEGIN;
		index_max = HEALTH_INDEX_MAX;
		count_max = (HEALTH_INDEX_MAX-HEALTH_INDEX_BEGIN);
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		index_addr_id = SPORT_INDEX_ADDR_ID;
		count_addr_id = SPORT_COUNT_ADDR_ID;
		index_begin = SPORT_INDEX_BEGIN;
		index_max = SPORT_INDEX_MAX;
		count_max = (SPORT_INDEX_MAX-SPORT_INDEX_BEGIN);
		data_len = sizeof(sport_record_t);
		break;
	}

	bytes_read = nvs_read(&fs, count_addr_id, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
		return false;

	//�洢��������
	tmp_count = nvs_rx;
	if(tmp_count <= 0)
		return false;
	
	bytes_read = nvs_read(&fs, index_addr_id, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
		return false;
	//���һ����¼��ID	
	tmp_index = nvs_rx;

	if(tmp_count == count_max)
	{
		uint16_t tmp_id;

		if(index == 0 || index > count_max)
			return false;

		//�����һ����¼��ʼ����ȡ�ڼ�����¼���ڴ˻����ϼ�index���
		tmp_id = tmp_index + index;
		if(tmp_id > index_max)
			tmp_id -= count_max;

		bytes_read = nvs_read(&fs, tmp_index, databuf, data_len);
		if(bytes_read<=0)
			return false;
	}
	else
	{
		//δ��������ȡ�ڼ������Ǵ洢ID(�������ʼID��1)
		if(index > tmp_count)
			return false;
			
		bytes_read = nvs_read(&fs, (index + index_begin), databuf, data_len);
		if(bytes_read <= 0)
			return false;
	}

	return true;
}

bool get_local_from_record(local_record_t *local_data, uint32_t index)
{
	return get_date_from_record(local_data, index, RECORD_TYPE_LOCATION);
}

bool get_health_from_record(health_record_t *health_data, uint32_t index)
{
	return get_date_from_record(health_data, index, RECORD_TYPE_HEALTH);
}

#ifdef CONFIG_IMU_SUPPORT
bool get_sport_from_record(sport_record_t *sport_data, uint32_t index)
{
	return get_date_from_record(sport_data, index, RECORD_TYPE_SPORT);
}
#endif

bool get_last_data_from_record(void *databuf, ENUM_RECORD_TYPE record_type)
{
	uint16_t nvs_rx=0;
	ssize_t bytes_read;
	uint16_t index_addr_id,count_addr_id;
	uint16_t index_begin,index_max,count_max;
	uint16_t tmp_index,tmp_count;
	uint32_t data_len;
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}
	
	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		index_addr_id = LOCAL_INDEX_ADDR_ID;
		count_addr_id = LOCAL_COUNT_ADDR_ID;
		index_begin = LOCAL_INDEX_BEGIN;
		index_max = LOCAL_INDEX_MAX;
		count_max = (LOCAL_INDEX_MAX-LOCAL_INDEX_BEGIN);
		data_len = sizeof(local_record_t);
		break;
		
	case RECORD_TYPE_HEALTH:
		index_addr_id = HEALTH_INDEX_ADDR_ID;
		count_addr_id = HEALTH_COUNT_ADDR_ID;
		index_begin = HEALTH_INDEX_BEGIN;
		index_max = HEALTH_INDEX_MAX;
		count_max = (HEALTH_INDEX_MAX-HEALTH_INDEX_BEGIN);
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		index_addr_id = SPORT_INDEX_ADDR_ID;
		count_addr_id = SPORT_COUNT_ADDR_ID;
		index_begin = SPORT_INDEX_BEGIN;
		index_max = SPORT_INDEX_MAX;
		count_max = (SPORT_INDEX_MAX-SPORT_INDEX_BEGIN);
		data_len = sizeof(sport_record_t);
		break;
	}

	bytes_read = nvs_read(&fs, count_addr_id, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
		return false;

	tmp_count = nvs_rx;
	if(tmp_count <= 0)
		return false;
	
	bytes_read = nvs_read(&fs, index_addr_id, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
		return false;
	
	tmp_index = nvs_rx;

	bytes_read = nvs_read(&fs, tmp_index, databuf, data_len);
	if(bytes_read <= 0)
		return false;
	
	return true;	
}

bool get_last_local_from_record(local_record_t *local_data)  
{
	return get_last_data_from_record(local_data, RECORD_TYPE_LOCATION);
}

bool get_last_health_from_record(health_record_t *health_data)
{
	return get_last_data_from_record(health_data, RECORD_TYPE_HEALTH);
}

bool get_last_sport_from_record(sport_record_t *sport_data)
{
	return get_last_data_from_record(sport_data, RECORD_TYPE_SPORT);
}

bool get_data_from_record_by_time_and_index(void *databuf, uint8_t data_type, sys_date_timer_t time, uint32_t index, ENUM_RECORD_TYPE record_type)
{
	uint16_t nvs_rx=0;
	ssize_t bytes_read;
	uint16_t index_addr_id,count_addr_id;
	uint16_t index_begin,index_max,count_max;
	uint16_t tmp_index,tmp_count;
	uint32_t i,data_len;
	int err;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOGD("Flash Init failed, return!");
		#endif
			return false;
		}
	}
	
	switch(record_type)
	{
	case RECORD_TYPE_LOCATION:
		index_addr_id = LOCAL_INDEX_ADDR_ID;
		count_addr_id = LOCAL_COUNT_ADDR_ID;
		index_begin = LOCAL_INDEX_BEGIN;
		index_max = LOCAL_INDEX_MAX;
		count_max = (LOCAL_INDEX_MAX-LOCAL_INDEX_BEGIN);
		data_len = sizeof(local_record_t);
		break;
		
	case RECORD_TYPE_HEALTH:
		index_addr_id = HEALTH_INDEX_ADDR_ID;
		count_addr_id = HEALTH_COUNT_ADDR_ID;
		index_begin = HEALTH_INDEX_BEGIN;
		index_max = HEALTH_INDEX_MAX;
		count_max = (HEALTH_INDEX_MAX-HEALTH_INDEX_BEGIN);
		data_len = sizeof(health_record_t);
		break;

	case RECORD_TYPE_SPORT:
		index_addr_id = SPORT_INDEX_ADDR_ID;
		count_addr_id = SPORT_COUNT_ADDR_ID;
		index_begin = SPORT_INDEX_BEGIN;
		index_max = SPORT_INDEX_MAX;
		count_max = (SPORT_INDEX_MAX-SPORT_INDEX_BEGIN);
		data_len = sizeof(sport_record_t);
		break;
	}

	bytes_read = nvs_read(&fs, count_addr_id, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
		return false;
	
	tmp_count = nvs_rx;

	if(tmp_count <= 0)
		return false;
	
	bytes_read = nvs_read(&fs, index_addr_id, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
		return false;

	tmp_index = nvs_rx;
	
	if(tmp_count == count_max)
	{
		uint16_t tmp_id;
		uint32_t left_count;
		sys_date_timer_t *p_time;
		
		tmp_id = tmp_index;
		for(i=0;i<count_max;i++)
		{				
			tmp_id++;
			if(tmp_id > index_max)
				tmp_id -= count_max;
				
			bytes_read = nvs_read(&fs, tmp_id, databuf, data_len);   
			if(bytes_read <= 0)
				return false;

			switch(record_type)
			{
			case RECORD_TYPE_LOCATION:
				{
					local_record_t* local_data;

					local_data = (local_record_t*)databuf;
					switch(data_type)
					{
					case RECORD_LOCATION_GPS:
						p_time = (sys_date_timer_t*)&(local_data->gps_rec.timestamp);
						break;
					case RECORD_LOCATION_WIFI:
					#ifdef CONFIG_WIFI_SUPPORT	
						p_time = (sys_date_timer_t*)&(local_data->wifi_rec.timestamp);
					#endif
						break;
					}
				}
				break;
				
			case RECORD_TYPE_HEALTH:
				{
					health_record_t* health_data;

					health_data = (health_record_t*)databuf;
					switch(data_type)
					{
					case RECORD_HEALTH_HR:
						p_time = (sys_date_timer_t*)&(health_data->hr_rec.timestamp);
						break;
					case RECORD_HEALTH_SPO2:
						p_time = (sys_date_timer_t*)&(health_data->spo2_rec.timestamp);
						break;
					case RECORD_HEALTH_BPT:
						p_time = (sys_date_timer_t*)&(health_data->bpt_rec.timestamp);
						break;
					case RECORD_HEALTH_ECG:
						break;
					}
					
				}
				break;

			case RECORD_TYPE_SPORT:
				{
					sport_record_t* sport_data;

					sport_data = (sport_record_t*)databuf;
					switch(data_type)
					{
					case RECORD_SPORT_STEP:
						p_time = (sys_date_timer_t*)&(sport_data->step_rec.timestamp);
						break;
					case RECORD_SPORT_SLEEP:
						p_time = (sys_date_timer_t*)&(sport_data->sleep_rec.timestamp);
						break;
					}
				}
				break;
			}
			
			if(p_time->year > time.year)
			{
				break;
			}
			else if(p_time->year == time.year)
			{
				if(p_time->month > time.month)
				{
					break;	
				}
				else if(p_time->month == time.month)
				{
					if(p_time->day > time.day)
					{
						break;
					}
					else if(p_time->day == time.day)
					{
						if(p_time->hour > time.hour)
						{
							break;
						}
						else if(p_time->hour == time.hour)
						{
							if(p_time->minute > time.minute)
							{
								break;
							}
							else if(p_time->second == time.minute)
							{
								if(p_time->second >= time.second)
								{
									break;
								}
							}
						}
					}
				}
			}
		}

		if(i == count_max)
			return false;

		//����ʣ����������ļ�¼����
		left_count = (index_max-i)+1;
		if(tmp_index > index_max)
			left_count += (tmp_index-index_begin);

		if((index > 1) && (index <= left_count))	//xb add 2021-12-01 �Ƚϵ�ʱ���Ѿ�ȡ���˵�һ�����ݣ���˺���ĵ�һ������ʵ�����ǵڶ�������
		{
			index--;
			index += tmp_id;
			if(index > index_max)
				index -= count_max;

			if(index > index_max)
				return false;
			
			bytes_read = nvs_read(&fs, index, databuf, data_len); 
			if(bytes_read <= 0)
				return false;
		}
	}
	else
	{
		sys_date_timer_t *p_time;
		
		if((index + index_begin) > tmp_index)
			return false;

		for(i=(index_begin+1);i<=tmp_index;i++)	
		{
			bytes_read = nvs_read(&fs, i, databuf, data_len);     
			if(bytes_read <= 0)
				return false;

			switch(record_type)
			{
			case RECORD_TYPE_LOCATION:
				{
					local_record_t* local_data;

					local_data = (local_record_t*)databuf;
					switch(data_type)
					{
					case RECORD_LOCATION_GPS:
						p_time = (sys_date_timer_t*)&(local_data->gps_rec.timestamp);
						break;
					case RECORD_LOCATION_WIFI:
					#ifdef CONFIG_WIFI_SUPPORT	
						p_time = (sys_date_timer_t*)&(local_data->wifi_rec.timestamp);
					#endif
						break;
					}
				}
				break;
				
			case RECORD_TYPE_HEALTH:
				{
					health_record_t* health_data;

					health_data = (health_record_t*)databuf;
					switch(data_type)
					{
					case RECORD_HEALTH_HR:
						p_time = (sys_date_timer_t*)&(health_data->hr_rec.timestamp);
						break;
					case RECORD_HEALTH_SPO2:
						p_time = (sys_date_timer_t*)&(health_data->spo2_rec.timestamp);
						break;
					case RECORD_HEALTH_BPT:
						p_time = (sys_date_timer_t*)&(health_data->bpt_rec.timestamp);
						break;
					case RECORD_HEALTH_ECG:
						break;
					}
				}
				break;

			case RECORD_TYPE_SPORT:
				{
					sport_record_t* sport_data;

					sport_data = (sport_record_t*)databuf;
					switch(data_type)
					{
					case RECORD_SPORT_STEP:
						p_time = (sys_date_timer_t*)&(sport_data->step_rec.timestamp);
						break;
					case RECORD_SPORT_SLEEP:
						p_time = (sys_date_timer_t*)&(sport_data->sleep_rec.timestamp);
						break;
					}
				}
				break;
			}

			if(p_time->year > time.year)
			{
				break;
			}
			else if(p_time->year == time.year)
			{
				if(p_time->month > time.month)
				{
					break;	
				}
				else if(p_time->month == time.month)
				{
					if(p_time->day > time.day)
					{
						break;
					}
					else if(p_time->day == time.day)
					{
						if(p_time->hour > time.hour)
						{
							break;
						}
						else if(p_time->hour == time.hour)
						{
							if(p_time->minute > time.minute)
							{
								break;
							}
							else if(p_time->second == time.minute)
							{
								if(p_time->second >= time.second)
								{
									break;
								}
							}
						}
					}
				}
			}
		}

		if(i > tmp_index)
			return false;

		if(index > 1)	//xb add 2021-12-01 �Ƚϵ�ʱ���Ѿ�ȡ���˵�һ�����ݣ���˺���ĵ�һ������ʵ�����ǵڶ�������
		{
			index--;
			index += i;
			if(index > tmp_index)
				return false;

			bytes_read = nvs_read(&fs, index, databuf, data_len);  
			if(bytes_read <= 0)
				return false;
		}
	}

	return true;	
}

bool get_local_from_record_by_time(local_record_t *local_data, ENUM_RECORD_LOCATION_TYPE type, sys_date_timer_t begin_time, uint32_t index)
{
	return get_data_from_record_by_time_and_index(local_data, type, begin_time, index, RECORD_TYPE_LOCATION);
}

bool get_health_from_record_by_time(health_record_t *health_data, ENUM_RECORD_HEALTH_TYPE type, sys_date_timer_t begin_time, uint32_t index)
{
	return get_data_from_record_by_time_and_index(health_data, type, begin_time, index, RECORD_TYPE_HEALTH);
}

bool get_sport_from_record_by_time(sport_record_t *sport_data, ENUM_RECORD_SPORT_TYPE type, sys_date_timer_t begin_time, uint32_t index)
{	
	return get_data_from_record_by_time_and_index(sport_data, type, begin_time, index, RECORD_TYPE_SPORT);
}

