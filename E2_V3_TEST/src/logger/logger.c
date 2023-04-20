/****************************************Copyright (c)************************************************
** File Name:			    logger.c
** Descriptions:			log message process source file
** Created By:				xie biao
** Created Date:			2021-10-25
** Modified Date:      		2021-10-25 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include "datetime.h"
#include "logger.h"
#include "external_flash.h"
#include "transfer_cache.h"
#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(E2, CONFIG_LOG_DEFAULT_LEVEL);

//#define TEST_DEBUG

#define LOG_COUNT_ADDR		(0x160000)
#define LOG_DATA_BEGIN_ADDR	(0x160000+4)
#define LOG_DATA_END_ADDR	(FONT_START_ADDR)

static bool log_save_flag = false;

static uint8_t countbuf[4] = {0};
static uint32_t write_count = 0;
static uint32_t write_len = 0;
static uint8_t logbuf[SPIFlash_SECTOR_SIZE] = {0};
static uint8_t buf[LOG_BUFF_SIZE] = {0};

static CacheInfo log_save_cache = {0};

static void LogWriteData(uint8_t *data, uint32_t datalen, DATA_TYPE type);
static void LogSaveDataCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(log_save_data_timer, LogSaveDataCallBack, NULL);

void LOGDD(const char *fun_name, const char *fmt, ...)
{
	uint16_t str_len = 0;
	int n = 0;
	uint32_t timemap=0;
	va_list args;

#ifdef TEST_DEBUG
	memset(buf, 0, sizeof(buf));
	timemap = (k_uptime_get()%1000);
	va_start(args, fmt);
	sprintf(buf, "[%02d:%02d:%02d:%03d]..%s>>", 
						date_time.hour, 
						date_time.minute, 
						date_time.second, 
						timemap,
						fun_name
						);
	
	str_len = strlen(buf);

#if defined(WIN32)
	n = str_len + _vsnprintf(&buf[str_len], sizeof(buf), fmt, args);
#else
	n = str_len + vsnprintf(&buf[str_len], sizeof(buf), fmt, args);
#endif /* WIN32 */
	va_end(args);

	//这里N操作100的长度直接退出，打太长可能会导致重启
	if(n > LOG_BUFF_SIZE - 4)
	{
		return;
	}

	if(n > 0)
	{
		*(buf + n) = '\n';
		n++;
		
		LOG_INF("%s", log_strdup(buf));
	}
#endif	
}

void log_write_data_to_flash(uint8_t *buf, uint32_t len)
{
	static int32_t last_index = -1;
	int32_t cur_index;
	uint32_t PageByteRemain,addr,datalen=len;

	if((write_len >= LOG_DATA_END_ADDR) || (write_len + len >= LOG_DATA_END_ADDR))
		return;

	addr = LOG_DATA_BEGIN_ADDR+write_len;
	cur_index = addr/SPIFlash_SECTOR_SIZE;
	if(cur_index > last_index)
	{
		last_index = cur_index;
		SPIFlash_Erase_Sector(last_index*SPIFlash_SECTOR_SIZE);
	}
	
	PageByteRemain = SPIFlash_SECTOR_SIZE - addr%SPIFlash_SECTOR_SIZE;
	if(PageByteRemain < datalen)
	{
		datalen -= PageByteRemain;
		while(1)
		{
			SPIFlash_Erase_Sector((++last_index)*SPIFlash_SECTOR_SIZE);
			if(datalen > SPIFlash_SECTOR_SIZE)
				datalen -= SPIFlash_SECTOR_SIZE;
			else
				break;
		}
	}
	
	SpiFlash_Write_Buf(buf, addr, len);
	write_len += len;

	write_count++;
	countbuf[0] = (uint8_t)(write_count>>0);
	countbuf[1] = (uint8_t)(write_count>>8);
	countbuf[2] = (uint8_t)(write_count>>16);
	countbuf[3] = (uint8_t)(write_count>>24);
	SpiFlash_Write(countbuf, LOG_COUNT_ADDR, sizeof(countbuf));
	return 0;
}

void log_read_from_flash(void)
{
	uint32_t i,j,k=0,len,addr = LOG_DATA_BEGIN_ADDR;
	uint32_t rec_count = 0;
	
	SpiFlash_Read(&countbuf, LOG_COUNT_ADDR, sizeof(countbuf));
	rec_count = countbuf[0]+0x100*countbuf[1]+0x10000*countbuf[2]+0x1000000*countbuf[3];
	if((rec_count == 0) || (rec_count == 0xffffffff))
		return;

	while(addr < LOG_DATA_END_ADDR)
	{
		SpiFlash_Read(logbuf, addr, SPIFlash_SECTOR_SIZE);
		for(i=0,j=0;i<SPIFlash_SECTOR_SIZE;)
		{
			uint8_t tmpdata;

			tmpdata = logbuf[i++];
			if(tmpdata > 0x00 && tmpdata < 0x7f)
			{
				buf[j++] = tmpdata;
				if(buf[j-1] == '\n')
				{
					buf[j-1] = 0x00;
					j = 0;
					LOG_INF("%s", log_strdup(buf));
					k++;
					if(k == rec_count)
						return;
				}
				else
				{
					if(j == (sizeof(buf) - 1))
					{
						buf[j] = 0x00;
						j=0;
						LOG_INF("%s", log_strdup(buf));
						k++;
						if(k == rec_count)
							return;
					}
				}
			}
		}
				
		addr += SPIFlash_SECTOR_SIZE;
	}
}

void LOGDM(const char *fun_name, const char *fmt, ...)
{
	uint16_t str_len = 0;
	int n = 0;
	uint32_t timemap=0;
	va_list args;

#if 0//def TEST_DEBUG
	memset(buf, 0, sizeof(buf));
	timemap = (k_uptime_get()%1000);
	va_start(args, fmt);
	sprintf(buf, "[%02d:%02d:%02d:%03d]..%s>>", 
						date_time.hour, 
						date_time.minute, 
						date_time.second, 
						timemap,
						fun_name
						);
	
	str_len = strlen(buf);

#if defined(WIN32)
	n = str_len + _vsnprintf(&buf[str_len], sizeof(buf), fmt, args);
#else
	n = str_len + vsnprintf(&buf[str_len], sizeof(buf), fmt, args);
#endif /* WIN32 */
	va_end(args);

	//这里N操作100的长度直接退出，打太长可能会导致重启
	if(n > LOG_BUFF_SIZE - 4)
	{
		return;
	}

	if(n > 0)
	{

		*(buf + n) = '\n';
		n++;

		LogWriteData(buf, n, DATA_TRANSFER);
	}
#endif	
}

static void LogSaveDataCallBack(struct k_timer *timer)
{
	log_save_flag = true;
}

static void LogSaveDataStart(void)
{
	k_timer_start(&log_save_data_timer, K_MSEC(200), K_NO_WAIT);
}

static void LogWriteData(uint8_t *data, uint32_t datalen, DATA_TYPE type)
{
	int ret;
	
	ret = add_data_into_cache(&log_save_cache, data, datalen, type);
	if(ret)
	{
		LogSaveDataStart();
	}
}

static void LogSaveData(void)
{
	uint8_t data_type,*p_data;
	uint32_t data_len;
	int ret;

	ret = get_data_from_cache(&log_save_cache, &p_data, &data_len, &data_type);
	if(ret)
	{
		log_write_data_to_flash(p_data, data_len);
		delete_data_from_cache(&log_save_cache);
		k_timer_start(&log_save_data_timer, K_MSEC(200), K_NO_WAIT);
	}
}

void LogMsgProcess(void)
{
	if(log_save_flag)
	{
		LogSaveData();
		log_save_flag = false;
	}
}

void LogInit(void)
{
	uint32_t addr;
	
	for(addr=LOG_COUNT_ADDR;addr<(LOG_DATA_END_ADDR);addr+=SPIFlash_SECTOR_SIZE)
	{
		SPIFlash_Erase_Sector(addr);
	}
}
