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

struct node
{
	uint32_t len;
	void *data;
	struct node *next;
};

typedef struct
{
	uint32_t count;
	struct node *cache;
}CacheInfo;

typedef struct node DataNode;

extern bool add_data_into_send_cache(uint8_t *data, uint32_t len);
extern bool get_data_from_send_cache(uint8_t **buf, uint32_t *len);
extern bool delete_data_from_send_cache(void);
extern bool add_data_into_rece_cache(uint8_t *data, uint32_t len);
extern bool get_data_from_rece_cache(uint8_t **buf, uint32_t *len);
extern bool delete_data_from_rece_cache(void);

#endif/*__TRANSFER_CACHE_H__*/
