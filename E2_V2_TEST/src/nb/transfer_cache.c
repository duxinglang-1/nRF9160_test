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

static CacheInfo g_nb_send_cache = {0};
static CacheInfo g_nb_rece_cache = {0};

static DataNode *send_head = NULL;
static DataNode *send_tail = NULL;

static DataNode *rece_head = NULL;
static DataNode *rece_tail = NULL;

bool add_data_into_send_cache(u8_t *data, u32_t len)
{
	DataNode *pnew;

	if(g_nb_send_cache.cache == NULL)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif
	
		g_nb_send_cache.count = 0;
		g_nb_send_cache.cache = NULL;

		send_tail = k_malloc(sizeof(DataNode));
		if(send_tail == NULL) 
			return false;
	
		memset(send_tail, 0, sizeof(DataNode));
		send_tail->data = k_malloc(len+1);
		if(send_tail->data == NULL) 
		{
			k_free(send_tail);
			send_tail = NULL;
			return false;
		}
		
		send_tail->len = len;
		memset(send_tail->data, 0, len+1);
		memcpy(send_tail->data, data, len);

		g_nb_send_cache.cache = send_tail;
		g_nb_send_cache.count = 1;
		return true;
	}
	else
	{
	#ifdef TRANSFER_LOG
		LOGD("002");
	#endif

		if(g_nb_send_cache.count > SEND_NODE_CACHE_MAX)
		{
			delete_data_from_send_cache();
		}
		
		if(send_tail == NULL)
		{
			send_tail = g_nb_send_cache.cache;
			while(1)
			{
				if(send_tail->next == NULL)
					break;
				else
					send_tail = send_tail->next;
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

		send_tail->next = pnew;
		send_tail = pnew;

		g_nb_send_cache.count++;
		
		return true;
	}
}

bool get_data_from_send_cache(u8_t **buf, u32_t *len)
{
	if(g_nb_send_cache.cache == NULL || g_nb_send_cache.count == 0)
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

		if(send_head == NULL)
			send_head = g_nb_send_cache.cache;

		*buf = send_head->data;
		*len = send_head->len;

	#if 0	//xb add 2021-03-23 只获取不删除，确认发送OK之后再删除
		send_head = send_head->next;
		g_nb_send_cache.cache = send_head;
		
		if(send_head == NULL)
			g_nb_send_cache.count = 0;
	#endif
	
		return true;
	}
}

bool delete_data_from_send_cache(void)
{
	if(g_nb_send_cache.cache == NULL || g_nb_send_cache.count == 0)
	{
		return false;
	}
	else
	{
		send_head = g_nb_send_cache.cache->next;
		
		k_free(g_nb_send_cache.cache->data);
		g_nb_send_cache.cache->data = NULL;
		g_nb_send_cache.cache->len = 0;
		k_free(g_nb_send_cache.cache);
		g_nb_send_cache.cache = NULL;

		g_nb_send_cache.count--;
		g_nb_send_cache.cache = send_head;
		
		return true;
	}
}

bool add_data_into_rece_cache(u8_t *data, u32_t len)
{
	DataNode *pnew;

	if(g_nb_rece_cache.cache == NULL)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif

		g_nb_rece_cache.count = 0;
		g_nb_rece_cache.cache = NULL;

		rece_tail = k_malloc(sizeof(DataNode));
		if(rece_tail == NULL) 
			return false;
	
		memset(rece_tail, 0, sizeof(DataNode));
		rece_tail->data = k_malloc(len+1);
		if(rece_tail->data == NULL) 
		{
			k_free(rece_tail);
			rece_tail = NULL;
			return false;
		}
		
		rece_tail->len = len;
		memset(rece_tail->data, 0, len+1);
		memcpy(rece_tail->data, data, len);

		g_nb_rece_cache.cache = rece_tail;
		g_nb_rece_cache.count = 1;
		return true;
	}
	else
	{
	#ifdef TRANSFER_LOG
		LOGD("002");
	#endif

		if(rece_tail == NULL)
		{
			rece_tail = g_nb_rece_cache.cache;
			while(1)
			{
				if(rece_tail->next == NULL)
					break;
				else
					rece_tail = rece_tail->next;
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

		rece_tail->next = pnew;
		rece_tail = pnew;

		g_nb_rece_cache.count++;
		
		return true;
	}
}

bool get_data_from_rece_cache(u8_t **buf, u32_t *len)
{
	if(g_nb_rece_cache.cache == NULL || g_nb_rece_cache.count == 0)
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

		if(rece_head == NULL)
			rece_head = g_nb_rece_cache.cache;

		*buf = rece_head->data;
		*len = rece_head->len;

	#if 0	//xb add 2021-03-23 只获取不删除，确认发送OK之后再删除
		rece_head = rece_head->next;
		g_nb_rece_cache.cache = rece_head;
		
		if(rece_head == NULL)
			g_nb_rece_cache.count = 0;
	#endif
	
		return true;
	}
}

bool delete_data_from_rece_cache(void)
{
	if(g_nb_rece_cache.cache == NULL || g_nb_rece_cache.count == 0)
	{
		return false;
	}
	else
	{
		rece_head = g_nb_rece_cache.cache->next;
		
		k_free(g_nb_rece_cache.cache->data);
		g_nb_rece_cache.cache->data = NULL;
		g_nb_rece_cache.cache->len = 0;
		k_free(g_nb_rece_cache.cache);
		g_nb_rece_cache.cache = NULL;

		g_nb_rece_cache.count--;
		g_nb_rece_cache.cache = rece_head;
		
		return true;
	}
}
