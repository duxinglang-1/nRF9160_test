/****************************************Copyright (c)************************************************
** File Name:			    logger.h
** Descriptions:			log message process head file
** Created By:				xie biao
** Created Date:			2021-10-25
** Modified Date:      		2021-10-25 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdbool.h>
#include <stdint.h>
#include <nrf9160.h>
#include <zephyr.h>

#define LOG_BUFF_SIZE (1024)
/* 无自动添加函数名 */
#define LOGC(fmt, args...) LOGDD("", fmt, ##args)
/* 有自动添加函数名 */
#define LOGD(fmt, args...) LOGDD(__func__, fmt, ##args)
/* 有自动添加函数名 */
#define LOGM(fmt, args...) LOGDM(__func__, fmt, ##args)

#endif/*__LOGGER_H__*/
