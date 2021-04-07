/****************************************Copyright (c)************************************************
** File Name:			    transfer_cache.h
** Descriptions:			Data transfer cache pool head file
** Created By:				xie biao
** Created Date:			2021-03-23
** Modified Date:      		2021-03-23 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __TRANSFER_CACHE_H__
#define __TRANSFER_CACHE_H__

#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>

struct node
{
	u32_t len;
	void *data;
	struct node *next;
};

typedef struct
{
	u32_t count;
	struct node *cache;
}CacheInfo;

typedef struct node DataNode;

extern bool add_data_into_cache(u8_t *data, u32_t len);
extern bool get_data_from_cache(u8_t **buf, u32_t *len);
extern bool delete_data_from_cache(void);

#endif/*__TRANSFER_CACHE_H__*/
