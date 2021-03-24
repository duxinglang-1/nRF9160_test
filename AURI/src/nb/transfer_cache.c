/****************************************Copyright (c)************************************************
** File Name:			    transfer_cache.c
** Descriptions:			Data transfer cache pool source file
** Created By:				xie biao
** Created Date:			2021-03-23
** Modified Date:      		2021-03-23 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include "transfer_cache.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(transfer_cache, CONFIG_LOG_DEFAULT_LEVEL);

static CacheInfo g_nb_cache = {0};

static DataNode *head = NULL;
static DataNode *tail = NULL;

bool add_data_into_cache(u8_t *data, u32_t len)
{
	DataNode *pnew;

	if(g_nb_cache.cache == NULL)
	{
		LOG_INF("%s: begin 001\n", __func__);
		
		g_nb_cache.count = 0;
		g_nb_cache.cache = NULL;

		tail = k_malloc(sizeof(DataNode));
		if(tail == NULL) 
			return false;
	
		memset(tail, 0, sizeof(DataNode));
		tail->data = k_malloc(sizeof(len));
		if(tail->data == NULL) 
		{
			k_free(tail);
			tail = NULL;
			return false;
		}
		
		tail->len = len;
		memcpy(tail->data, data, len);

		g_nb_cache.cache = tail;
		g_nb_cache.count = 1;
		return true;
	}
	else
	{
		LOG_INF("%s: begin 002\n", __func__);
		
		if(tail == NULL)
		{
			tail = g_nb_cache.cache;
			while(1)
			{
				if(tail->next == NULL)
					break;
				else
					tail = tail->next;
			}
		}

		pnew = k_malloc(sizeof(DataNode));
		if(pnew == NULL) 
			return false;

		memset(pnew, 0, sizeof(DataNode));
		pnew->data = k_malloc(sizeof(len));
		if(pnew->data == NULL) 
		{
			k_free(pnew);
			pnew = NULL;
			return false;
		}
		
		pnew->len = len;
		memcpy(pnew->data, data, len);

		tail->next = pnew;
		tail = pnew;

		g_nb_cache.count++;
		
		return true;
	}
}

bool get_data_from_cache(u8_t **buf, u32_t *len)
{
	if(g_nb_cache.cache == NULL || g_nb_cache.count == 0)
	{
		LOG_INF("%s: begin 001\n", __func__);
		
		buf = NULL;
		*len = 0;
		
		return false;
	}
	else
	{
		LOG_INF("%s: begin 002\n", __func__);
		
		if(head == NULL)
			head = g_nb_cache.cache;

		*buf = head->data;
		*len = head->len;

	#if 0	//xb add 2021-03-23 只获取不删除，确认发送OK之后再删除
		head = head->next;
		g_nb_cache.cache = head;
		
		if(head == NULL)
			g_nb_cache.count = 0;
	#endif
	
		return true;
	}
}

bool delete_data_from_cache(void)
{
	if(g_nb_cache.cache == NULL || g_nb_cache.count == 0)
	{
		return false;
	}
	else
	{
		head = g_nb_cache.cache->next;
		
		k_free(g_nb_cache.cache->data);
		g_nb_cache.cache->data = NULL;
		g_nb_cache.cache->len = 0;
		k_free(g_nb_cache.cache);
		g_nb_cache.cache = NULL;

		g_nb_cache.count--;
		g_nb_cache.cache = head;
		
		return true;
	}
}
