/****************************************Copyright (c)************************************************
** File Name:			    inner_flash.c
** Descriptions:			nrf9160 inner flash management source file
** Created By:				xie biao
** Created Date:			2021-01-07
** Modified Date:      		2021-07-12
** Version:			    	V1.1
******************************************************************************************************/
#include <fs/nvs.h>
#include <drivers/flash.h>
#include <device.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "inner_flash.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(inner_flash, CONFIG_LOG_DEFAULT_LEVEL);

//#define INNER_FLASH_DEBUG

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

#define SPORT_RECORD_ID 		10
#define SPORT_LENGTH_ID    		11	

int count;
u16_t heart_rate = 1000, blood_oxygen = 2000;	
u16_t sport_record = 0, sport_last = 0;

static bool nvs_init_flag = false;

static struct nvs_fs fs;
static struct flash_pages_info info;

static int nvs_setup(void)
{	
	int err;	

	fs.offset = DT_FLASH_AREA_STORAGE_OFFSET;	
	err = flash_get_page_info_by_offs(device_get_binding(DT_FLASH_DEV_NAME), fs.offset, &info);	
	if(err)
	{
	#ifdef INNER_FLASH_DEBUG
		LOG_INF("Unable to get page info");	
	#endif
	}	

	fs.sector_size = info.size;
	fs.sector_count = 6U;
	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	if(err)
	{
	#ifdef INNER_FLASH_DEBUG
		LOG_INF("Flash Init failed\n");
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
			LOG_INF("Flash Init failed, return!\n");
		#endif
			return;
		}
	}

	err = nvs_read(&fs, SETTINGS_ID, settings, sizeof(global_settings_t));
	if(err < 0)
	{
	#ifdef INNER_FLASH_DEBUG
		LOG_INF("get settins err:%d\n", err);
	#endif
	}
}

void SaveSettingsToInnerFlash(global_settings_t settings)
{
	nvs_write(&fs, SETTINGS_ID, &settings, sizeof(global_settings_t));
}

void ReadDateTimeFromInnerFlash(sys_date_timer_t *time)
{
	int err = 0;
	
	if(!nvs_init_flag)
	{
		err = nvs_setup();
		if(err)
		{
		#ifdef INNER_FLASH_DEBUG
			LOG_INF("Flash Init failed, return!\n");
		#endif
			return;
		}
	}

	err = nvs_read(&fs, DATETIME_ID, time, sizeof(sys_date_timer_t));
	if(err < 0)
	{
	#ifdef INNER_FLASH_DEBUG
		LOG_INF("get datetime err:%d\n", err);
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
		LOG_INF("nvs_setup failed\n");
	#endif
	}

	freespace = nvs_calc_free_space(&fs);

#ifdef INNER_FLASH_DEBUG	
	LOG_INF("Remaining free space in nvs sector is %d Bytes\n", freespace);
#endif

	for(int i=0; i<ARRAY_SIZE(results); i++)
	{
	#ifdef INNER_FLASH_DEBUG
		LOG_INF("Writing %s to NVS\n", results[i]);
	#endif
	
		bytes_written = nvs_write(&fs, i, results[i], strlen(results[i]));

	#ifdef INNER_FLASH_DEBUG
		LOG_INF("Bytes written to nvs: %d at ID %d\n", bytes_written, i);
	#endif

		freespace = nvs_calc_free_space(&fs);

	#ifdef INNER_FLASH_DEBUG
		LOG_INF("Remaining free space in nvs sector is %d Bytes\n", freespace);
	#endif
	}

	k_sleep(K_MSEC(5000));

	for(int i=0; i<ARRAY_SIZE(results); i++)
	{
		bytes_read = nvs_read(&fs, i, nvs_rx_buff, sizeof(nvs_rx_buff));
	#ifdef INNER_FLASH_DEBUG
		LOG_INF("Bytes read from nvs: %d at ID %d\n", bytes_read, i);
		LOG_INF("Data read from nvs: %s at ID %d\n", nvs_rx_buff, i);
	#endif
	}
}

bool save_local_to_record(local_record_t *local_data)
{
	return true;
}

bool save_health_to_record(health_record_t *health_data)
{
	return true;
}

bool save_sport_to_record(sport_record_t *sport_data)
{
	return true;
}

bool get_local_record(local_record_t *local_data, u32_t index)
{
	return true;
}

bool get_local_record_from_time(local_record_t *local_data, sys_date_timer_t begin_time, u32_t index)
{
	return true;
}

bool get_health_record(health_record_t *health_data, u32_t index)
{
	return true;
}

bool get_health_record_from_time(health_record_t *health_data, sys_date_timer_t begin_time, u32_t index)
{
	return true;
}

bool get_sport_record(sport_record_t *sport_data, u32_t index)
{
	return true;
}

bool get_sport_record_from_time(sport_record_t *sport_data, sys_date_timer_t begin_time, u32_t index)
{
	return true;
}

bool Nvs_Write_SportData(struct nvs_fs *fs, const void *data,size_t len)
{
	u16_t nvs_rx = 0;
	ssize_t bytes_written,bytes_read,err;

	bytes_read = nvs_read(fs, SPORT_RECORD_ID, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read <= 0)
	{
		sport_record = 1000;
		sport_last = 0;
	}
	else
	{
		sport_record = nvs_rx;
		bytes_read = nvs_read(fs, SPORT_LENGTH_ID, &nvs_rx, sizeof(nvs_rx));
		if(bytes_read <= 0)
		{
			return false;
		}
		sport_last = nvs_rx;
	}

	if(sport_last == 100)
	{
		if((sport_last == 100)&&(sport_record == 1100))
		{
			sport_record = 1000;
		}
		sport_record +=1;
			
		bytes_written = nvs_write(fs,sport_record, data, len);
		if(bytes_written<=0)
		{
			return false;
		}
		bytes_written = nvs_write(fs,SPORT_RECORD_ID, &sport_record, sizeof(sport_record));
		if(bytes_written<=0)
		{
			return false;
		}
	}
	else
	{
		sport_record +=1;
		sport_last +=1;
		bytes_written = nvs_write(fs,sport_record, data, len);
		
		if(bytes_written<=0)
		{
			return false;
		}
		bytes_written = nvs_write(fs,SPORT_RECORD_ID, &sport_record, sizeof(sport_record));
		if(bytes_written<=0)
		{
			return false;
		}
		
		bytes_written = nvs_write(fs,SPORT_LENGTH_ID, &sport_last, sizeof(sport_last));
		if(bytes_written<=0)
		{
			return false;
		}
	}

	return true;
}

bool Get_SportData(void *data, u16_t index)
{
	u16_t node,nvs_rx;
	ssize_t bytes_written, bytes_read,err;

	bytes_read = nvs_read(&fs, SPORT_LENGTH_ID, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read<=0)
	{
		return false;
	}
	sport_last = nvs_rx;
	bytes_read = nvs_read(&fs, SPORT_RECORD_ID, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read<=0)
	{
		return false;
	}
	sport_record = nvs_rx;
	if(sport_last==100)
	{
		node = sport_record+index;
		if(node>1100)
			node = node-100;
		bytes_read = nvs_read(&fs, node, data, 20);
		if(bytes_read<=0)
		{
		return false;
		}
	}
	else
	{
		if(index > sport_last)
			return false;
			
		bytes_read = nvs_read(&fs, (index + 1000), data, 20);
		if(bytes_read<=0)
		{
			return false;
		}
	}
		
    return true;
}


bool Get_SportData_from_time(void *data, sys_date_timer_t begin_time,u16_t index)
{	
	u16_t record,node,nvs_rx,i;
	ssize_t bytes_written, bytes_read,err;
	sport_record_t test;
	
	bytes_read = nvs_read(&fs, SPORT_LENGTH_ID, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read<=0)
	{
		return false;
	}
	
	sport_last = nvs_rx;
	bytes_read = nvs_read(&fs, SPORT_RECORD_ID, &nvs_rx, sizeof(nvs_rx));
	if(bytes_read<=0)
	{
		return false;
	}
	sport_record = nvs_rx;
	
	if(sport_last==100)
	{
		record = sport_record;
		for(i=0;i<=100;i++)
		{		

			if(i==100)
				return false;	
				
			record +=1;			
			if(record > 1100)
				record = record - 100;
				
			bytes_read = nvs_read(&fs, record, &test, 20);
			if(bytes_read<=0)
			{
				return false;
			}	
			
			if((test.timestamp.year == begin_time.year)&&(test.timestamp.month == begin_time.month)&&(test.timestamp.day == begin_time.day)
				&&(test.timestamp.hour == begin_time.hour)&&(test.timestamp.minute == begin_time.minute)&&(test.timestamp.second == begin_time.second)
				&&(test.timestamp.week == begin_time.week))
			{
				break;
			}			
		}

		node = record+index;
		if((node-100)>sport_record)
			return false;

		if(node>1100)
			node= node-100;
		bytes_read = nvs_read(&fs, node, data, 20);
		if(bytes_read<=0)
		{
			return false;
		}		
	}
	else
	{
		if((index + 1000) > sport_record)
			return false;

		for(i=1001;i<=1100;i++)	
		{
			if(i==1100)
			{
				return false;
			}
			bytes_read = nvs_read(&fs, i, &test, 20);
			if(bytes_read<=0)
			{
				return false;
			}
			if((test.timestamp.year == begin_time.year)&&(test.timestamp.month == begin_time.month)&&(test.timestamp.day == begin_time.day)
				&&(test.timestamp.hour == begin_time.hour)&&(test.timestamp.minute == begin_time.minute)&&(test.timestamp.second == begin_time.second)
				&&(test.timestamp.week == begin_time.week))
			{
				break;
			}
		}
		
		if((i+index) > sport_record)
			return false;

		bytes_read = nvs_read(&fs, (i+index), data, 20);
		if(bytes_read<=0)
		{
				return false;
		}
	}

	return true;
}
