/****************************************Copyright (c)************************************************
** File Name:			    transfer_cache.c
** Descriptions:			Data transfer cache pool source file
** Created By:				xie biao
** Created Date:			2021-03-23
** Modified Date:      		2021-04-19 
** Version:			    	V1.1
******************************************************************************************************/
#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include "transfer_cache.h"
#include "logger.h"

//#define TRANSFER_LOG

bool cache_is_empty(CacheInfo *data_cache)
{
	if(data_cache->cache == NULL || data_cache->count == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool add_data_into_cache(CacheInfo *data_cache, uint8_t *data, uint32_t len)
{
	DataNode *pnew;

	if(data_cache->cache == NULL)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif
	
		data_cache->count = 0;
		data_cache->cache = NULL;

		data_cache->tail = k_malloc(sizeof(DataNode));
		if(data_cache->tail == NULL) 
			return false;
	
		memset(data_cache->tail, 0, sizeof(DataNode));
		data_cache->tail->data = k_malloc(len+1);
		if(data_cache->tail->data == NULL) 
		{
			k_free(data_cache->tail);
			data_cache->tail = NULL;
			return false;
		}
		
		data_cache->tail->len = len;
		memset(data_cache->tail->data, 0, len+1);
		memcpy(data_cache->tail->data, data, len);

		data_cache->cache = data_cache->tail;
		data_cache->count = 1;
		return true;
	}
	else
	{
	#ifdef TRANSFER_LOG
		LOGD("002");
	#endif

		if(data_cache->count > NODE_CACHE_MAX)
		{
			delete_data_from_cache(data_cache);
		}
		
		if(data_cache->tail == NULL)
		{
			data_cache->tail = data_cache->cache;
			while(1)
			{
				if(data_cache->tail->next == NULL)
					break;
				else
					data_cache->tail = data_cache->tail->next;
			}
		}

		pnew = k_malloc(sizeof(DataNode));
		if(pnew == NULL) 
			return false;

		memset(pnew, 0, sizeof(DataNode));
		pnew->data = k_malloc(len+1);
		if(pnew->data == NULL) 
		{
			k_free(pnew);
			pnew = NULL;
			return false;
		}
		
		pnew->len = len;
		memset(pnew->data, 0, len+1);
		memcpy(pnew->data, data, len);

		data_cache->tail->next = pnew;
		data_cache->tail = pnew;

		data_cache->count++;
		
		return true;
	}
}

bool get_data_from_cache(CacheInfo *data_cache, uint8_t **buf, uint32_t *len)
{
	if(data_cache->cache == NULL || data_cache->count == 0)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif
	
		buf = NULL;
		*len = 0;
		
		return false;
	}
	else
	{
	#ifdef TRANSFER_LOG
		LOGD("002");
	#endif

		if(data_cache->head == NULL)
			data_cache->head = data_cache->cache;

		*buf = data_cache->head->data;
		*len = data_cache->head->len;

		return true;
	}
}

bool delete_data_from_cache(CacheInfo *data_cache)
{
	if(data_cache->cache == NULL || data_cache->count == 0)
	{
		return false;
	}
	else
	{
		data_cache->head = data_cache->cache->next;
		
		k_free(data_cache->cache->data);
		data_cache->cache->data = NULL;
		data_cache->cache->len = 0;
		k_free(data_cache->cache);
		data_cache->cache = NULL;

		data_cache->count--;
		data_cache->cache = data_cache->head;
		
		return true;
	}
}
