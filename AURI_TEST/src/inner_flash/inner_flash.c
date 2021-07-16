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
static struct nvs_fs fs;
static struct flash_pages_info info;

int nvs_setup(void)
{	
	int err;	

	fs.offset = DT_FLASH_AREA_STORAGE_OFFSET;	
	err = flash_get_page_info_by_offs(device_get_binding(DT_FLASH_DEV_NAME), fs.offset, &info);	
	if(err)
	{		
		printk("Unable to get page info");	
	}	

	fs.sector_size = info.size;
	fs.sector_count = 6U;
	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	if(err)
	{
		printk("Flash Init failed\n");
	}
	err = nvs_clear(&fs);
	if(err)
	{
		printk("nvs_clear failed\n");
	}
	return err;
}

void test_nvs(void)
{
	int err;
	char nvs_rx_buff[39] = {0};
	ssize_t bytes_written,bytes_read,freespace;
	
	err = nvs_setup();
	if(err)
	{
		printk("nvs_setup failed\n");
	}

	freespace = nvs_calc_free_space(&fs);
	printk("Remaining free space in nvs sector is %d Bytes\n", freespace);
	
	for(int i=0; i<ARRAY_SIZE(results); i++)
	{
		printk("Writing %s to NVS\n", results[i]);
		bytes_written = nvs_write(&fs, i, results[i], strlen(results[i]));
		printk("Bytes written to nvs: %d at ID %d\n", bytes_written, i);

		freespace = nvs_calc_free_space(&fs);
		printk("Remaining free space in nvs sector is %d Bytes\n", freespace);
	}

	k_sleep(K_MSEC(5000));

	for(int i=0; i<ARRAY_SIZE(results); i++)
	{
		bytes_read = nvs_read(&fs, i, nvs_rx_buff, sizeof(nvs_rx_buff));
		printk("Bytes read from nvs: %d at ID %d\n", bytes_read, i);
		printk("Data read from nvs: %s at ID %d\n", nvs_rx_buff, i);
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

