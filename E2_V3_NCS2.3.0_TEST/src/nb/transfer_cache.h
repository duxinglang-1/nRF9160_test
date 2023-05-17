/****************************************Copyright (c)************************************************
** File Name:			    transfer_cache.h
** Descriptions:			Data transfer cache pool head file
** Created By:				xie biao
** Created Date:			2021-03-23
** Modified Date:      		2021-04-19 
** Version:			    	V1.1
******************************************************************************************************/
#ifndef __TRANSFER_CACHE_H__
#define __TRANSFER_CACHE_H__

#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>

#define NODE_CACHE_MAX	50

typedef enum
{
	DATA_TRANSFER,
	DATA_RESOURCE,
	DATA_RESOURCE_VER,
	DATA_MAX
}DATA_TYPE;

struct datanode
{
	uint32_t len;
	DATA_TYPE type;
	void *data;
	struct datanode *next;
};

typedef struct
{
	uint32_t count;
	struct datanode *cache;
	struct datanode *head;
	struct datanode *tail;
}CacheInfo;

typedef struct datanode DataNode;

extern bool add_data_into_cache(CacheInfo *data_cache, uint8_t *data, uint32_t len, DATA_TYPE type);
extern bool get_data_from_cache(CacheInfo *data_cache, uint8_t **buf, uint32_t *len, DATA_TYPE *type);
extern bool delete_data_from_cache(CacheInfo *data_cache);

#endif/*__TRANSFER_CACHE_H__*/
