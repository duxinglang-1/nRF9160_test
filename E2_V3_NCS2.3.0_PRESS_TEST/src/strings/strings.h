/****************************************Copyright (c)************************************************
** File Name:			    strings.h
** Descriptions:			strings head file
** Created By:				xie biao
** Created Date:			2025-10-10
** Modified Date:      		2025-10-10 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/types.h>

//strlib.bin库文件
//库文件结构：文件头+数据体
//	  文件头：字符总条数(2 bytes) + 语言总国别数(2 bytes) + 第1条字符串(第1国字符数据所在数据体的位置(4 bytes)+字符串的长度(4 bytes)+...+第m国字符数据所在数据体的位置(4 bytes)+字符串的长度(4 bytes)) + ... +  第n条字符串(第1国字符数据所在数据体的位置(4 bytes)+字符串的长度(4 bytes)+...+第m国字符数据所在数据体的位置(4 bytes)+字符串的长度(4 bytes))
//	  数据体：第1条字符串(第1国字符数据+...+第m国字符数据) + ... + 第n条字符串(第1国字符数据+...+第m国字符数据)
//国别依次为：Chinese、English、German、French、Italian、Spanish、Portuguese、Polish、Swedish、Japanese、Korea、Russian、Arabic

typedef struct
{
	uint32_t addr;
	uint32_t len;
}dataindex;

typedef struct
{
	uint16_t str_count;
	uint16_t lang_count;
	uint32_t index_len;
	dataindex *p_index;
}strlib_infor_t;

#endif/*__STRINGS_H__*/