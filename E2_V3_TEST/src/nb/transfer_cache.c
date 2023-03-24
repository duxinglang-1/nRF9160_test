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
static DataNode *nb_send_head = NULL;
static DataNode *nb_send_tail = NULL;
static DataNode *nb_rece_head = NULL;
static DataNode *nb_rece_tail = NULL;

static CacheInfo uart_send_cache = {0};
static CacheInfo uart_rece_cache = {0};
static DataNode *uart_send_head = NULL;
static DataNode *uart_send_tail = NULL;
static DataNode *uart_rece_head = NULL;
static DataNode *uart_rece_tail = NULL;

bool send_cache_is_empty(void)
{
	if(g_nb_send_cache.cache == NULL || g_nb_send_cache.count == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool add_data_into_send_cache(uint8_t *data, uint32_t len)
{
	DataNode *pnew;

	if(g_nb_send_cache.cache == NULL)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif
	
		g_nb_send_cache.count = 0;
		g_nb_send_cache.cache = NULL;

		nb_send_tail = k_malloc(sizeof(DataNode));
		if(nb_send_tail == NULL) 
			return false;
	
		memset(nb_send_tail, 0, sizeof(DataNode));
		nb_send_tail->data = k_malloc(len+1);
		if(nb_send_tail->data == NULL) 
		{
			k_free(nb_send_tail);
			nb_send_tail = NULL;
			return false;
		}
		
		nb_send_tail->len = len;
		memset(nb_send_tail->data, 0, len+1);
		memcpy(nb_send_tail->data, data, len);

		g_nb_send_cache.cache = nb_send_tail;
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
		
		if(nb_send_tail == NULL)
		{
			nb_send_tail = g_nb_send_cache.cache;
			while(1)
			{
				if(nb_send_tail->next == NULL)
					break;
				else
					nb_send_tail = nb_send_tail->next;
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

		nb_send_tail->next = pnew;
		nb_send_tail = pnew;

		g_nb_send_cache.count++;
		
		return true;
	}
}

bool get_data_from_send_cache(uint8_t **buf, uint32_t *len)
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

		if(nb_send_head == NULL)
			nb_send_head = g_nb_send_cache.cache;

		*buf = nb_send_head->data;
		*len = nb_send_head->len;

	#if 0	//xb add 2021-03-23 只获取不删除，确认发送OK之后再删除
		nb_send_head = nb_send_head->next;
		g_nb_send_cache.cache = nb_send_head;
		
		if(nb_send_head == NULL)
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
		nb_send_head = g_nb_send_cache.cache->next;
		
		k_free(g_nb_send_cache.cache->data);
		g_nb_send_cache.cache->data = NULL;
		g_nb_send_cache.cache->len = 0;
		k_free(g_nb_send_cache.cache);
		g_nb_send_cache.cache = NULL;

		g_nb_send_cache.count--;
		g_nb_send_cache.cache = nb_send_head;
		
		return true;
	}
}

bool add_data_into_rece_cache(uint8_t *data, uint32_t len)
{
	DataNode *pnew;

	if(g_nb_rece_cache.cache == NULL)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif

		g_nb_rece_cache.count = 0;
		g_nb_rece_cache.cache = NULL;

		nb_rece_tail = k_malloc(sizeof(DataNode));
		if(nb_rece_tail == NULL) 
			return false;
	
		memset(nb_rece_tail, 0, sizeof(DataNode));
		nb_rece_tail->data = k_malloc(len+1);
		if(nb_rece_tail->data == NULL) 
		{
			k_free(nb_rece_tail);
			nb_rece_tail = NULL;
			return false;
		}
		
		nb_rece_tail->len = len;
		memset(nb_rece_tail->data, 0, len+1);
		memcpy(nb_rece_tail->data, data, len);

		g_nb_rece_cache.cache = nb_rece_tail;
		g_nb_rece_cache.count = 1;
		return true;
	}
	else
	{
	#ifdef TRANSFER_LOG
		LOGD("002");
	#endif

		if(nb_rece_tail == NULL)
		{
			nb_rece_tail = g_nb_rece_cache.cache;
			while(1)
			{
				if(nb_rece_tail->next == NULL)
					break;
				else
					nb_rece_tail = nb_rece_tail->next;
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

		nb_rece_tail->next = pnew;
		nb_rece_tail = pnew;

		g_nb_rece_cache.count++;
		
		return true;
	}
}

bool get_data_from_rece_cache(uint8_t **buf, uint32_t *len)
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

		if(nb_rece_head == NULL)
			nb_rece_head = g_nb_rece_cache.cache;

		*buf = nb_rece_head->data;
		*len = nb_rece_head->len;

	#if 0	//xb add 2021-03-23 只获取不删除，确认发送OK之后再删除
		nb_rece_head = nb_rece_head->next;
		g_nb_rece_cache.cache = nb_rece_head;
		
		if(nb_rece_head == NULL)
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
		nb_rece_head = g_nb_rece_cache.cache->next;
		
		k_free(g_nb_rece_cache.cache->data);
		g_nb_rece_cache.cache->data = NULL;
		g_nb_rece_cache.cache->len = 0;
		k_free(g_nb_rece_cache.cache);
		g_nb_rece_cache.cache = NULL;

		g_nb_rece_cache.count--;
		g_nb_rece_cache.cache = nb_rece_head;
		
		return true;
	}
}

bool uart_send_cache_is_empty(void)
{
	if(uart_send_cache.cache == NULL || uart_send_cache.count == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool uart_add_data_into_send_cache(uint8_t *data, uint32_t len)
{
	DataNode *pnew;

	if(uart_send_cache.cache == NULL)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif
	
		uart_send_cache.count = 0;
		uart_send_cache.cache = NULL;

		uart_send_tail = k_malloc(sizeof(DataNode));
		if(uart_send_tail == NULL) 
			return false;
	
		memset(uart_send_tail, 0, sizeof(DataNode));
		uart_send_tail->data = k_malloc(len+1);
		if(uart_send_tail->data == NULL) 
		{
			k_free(uart_send_tail);
			uart_send_tail = NULL;
			return false;
		}
		
		uart_send_tail->len = len;
		memset(uart_send_tail->data, 0, len+1);
		memcpy(uart_send_tail->data, data, len);

		uart_send_cache.cache = uart_send_tail;
		uart_send_cache.count = 1;
		return true;
	}
	else
	{
	#ifdef TRANSFER_LOG
		LOGD("002");
	#endif

		if(uart_send_cache.count > SEND_NODE_CACHE_MAX)
		{
			uart_delete_data_from_send_cache();
		}
		
		if(uart_send_tail == NULL)
		{
			uart_send_tail = uart_send_cache.cache;
			while(1)
			{
				if(uart_send_tail->next == NULL)
					break;
				else
					uart_send_tail = uart_send_tail->next;
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

		uart_send_tail->next = pnew;
		uart_send_tail = pnew;

		uart_send_cache.count++;
		
		return true;
	}
}

bool uart_get_data_from_send_cache(uint8_t **buf, uint32_t *len)
{
	if(uart_send_cache.cache == NULL || uart_send_cache.count == 0)
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

		if(uart_send_head == NULL)
			uart_send_head = uart_send_cache.cache;

		*buf = uart_send_head->data;
		*len = uart_send_head->len;

	#if 0	//xb add 2021-03-23 只获取不删除，确认发送OK之后再删除
		nb_send_head = nb_send_head->next;
		g_nb_send_cache.cache = nb_send_head;
		
		if(nb_send_head == NULL)
			g_nb_send_cache.count = 0;
	#endif
	
		return true;
	}
}

bool uart_delete_data_from_send_cache(void)
{
	if(uart_send_cache.cache == NULL || uart_send_cache.count == 0)
	{
		return false;
	}
	else
	{
		uart_send_head = uart_send_cache.cache->next;
		
		k_free(uart_send_cache.cache->data);
		uart_send_cache.cache->data = NULL;
		uart_send_cache.cache->len = 0;
		k_free(uart_send_cache.cache);
		uart_send_cache.cache = NULL;

		uart_send_cache.count--;
		uart_send_cache.cache = uart_send_head;
		
		return true;
	}
}

bool uart_add_data_into_rece_cache(uint8_t *data, uint32_t len)
{
	DataNode *pnew;

	if(uart_rece_cache.cache == NULL)
	{
	#ifdef TRANSFER_LOG
		LOGD("001");
	#endif

		uart_rece_cache.count = 0;
		uart_rece_cache.cache = NULL;

		uart_rece_tail = k_malloc(sizeof(DataNode));
		if(uart_rece_tail == NULL) 
			return false;
	
		memset(uart_rece_tail, 0, sizeof(DataNode));
		uart_rece_tail->data = k_malloc(len+1);
		if(uart_rece_tail->data == NULL) 
		{
			k_free(nb_rece_tail);
			nb_rece_tail = NULL;
			return false;
		}
		
		uart_rece_tail->len = len;
		memset(uart_rece_tail->data, 0, len+1);
		memcpy(uart_rece_tail->data, data, len);

		uart_rece_cache.cache = uart_rece_tail;
		uart_rece_cache.count = 1;
		return true;
	}
	else
	{
	#ifdef TRANSFER_LOG
		LOGD("002");
	#endif

		if(uart_rece_tail == NULL)
		{
			uart_rece_tail = uart_rece_cache.cache;
			while(1)
			{
				if(uart_rece_tail->next == NULL)
					break;
				else
					uart_rece_tail = uart_rece_tail->next;
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

		uart_rece_tail->next = pnew;
		uart_rece_tail = pnew;

		uart_rece_cache.count++;
		
		return true;
	}
}

bool uart_get_data_from_rece_cache(uint8_t **buf, uint32_t *len)
{
	if(uart_rece_cache.cache == NULL || uart_rece_cache.count == 0)
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

		if(uart_rece_head == NULL)
			uart_rece_head = uart_rece_cache.cache;

		*buf = uart_rece_head->data;
		*len = uart_rece_head->len;

	#if 0	//xb add 2021-03-23 只获取不删除，确认发送OK之后再删除
		nb_rece_head = nb_rece_head->next;
		g_nb_rece_cache.cache = nb_rece_head;
		
		if(nb_rece_head == NULL)
			g_nb_rece_cache.count = 0;
	#endif
	
		return true;
	}
}

bool uart_delete_data_from_rece_cache(void)
{
	if(uart_rece_cache.cache == NULL || uart_rece_cache.count == 0)
	{
		return false;
	}
	else
	{
		uart_rece_head = uart_rece_cache.cache->next;
		
		k_free(uart_rece_cache.cache->data);
		uart_rece_cache.cache->data = NULL;
		uart_rece_cache.cache->len = 0;
		k_free(uart_rece_cache.cache);
		uart_rece_cache.cache = NULL;

		uart_rece_cache.count--;
		uart_rece_cache.cache = uart_rece_head;
		
		return true;
	}
}

