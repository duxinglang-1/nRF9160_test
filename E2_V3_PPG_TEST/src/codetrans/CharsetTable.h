/****************************************Copyright (c)************************************************
** File Name:			    charsettable.h
** Descriptions:			charactor encoding table head file
** Created By:				xie biao
** Created Date:			2020-12-28
** Modified Date:      		2020-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __CHARSETTABLE_H__
#define __CHARSETTABLE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include "codetrans.h"

#ifdef __MMI_CHSET_BIG5__
extern const key_index_t g_chset_ucs2_to_big5_key_LSB_index[];
extern const key_index_t g_chset_big5_to_ucs2_key_LSB_index[];
extern const unsigned char g_chset_ucs2_to_big5_key_MSB[];
extern const unsigned char g_chset_big5_to_ucs2_key_MSB[];
extern const unsigned short g_chset_ucs2_to_big5_table[];
extern const unsigned short g_chset_big5_to_ucs2_table[];
#endif /* __MMI_CHSET_BIG5__ */ 

#ifdef __MMI_CHSET_GB2312__
extern const key_index_t g_chset_ucs2_to_gb2312_key_LSB_index[];
extern const key_index_t g_chset_gb2312_to_ucs2_key_LSB_index[];
extern const unsigned char g_chset_ucs2_to_gb2312_key_MSB[];
extern const MSB_Mapping_Struct g_chset_gb2312_to_ucs2_key_MSB[];
extern const unsigned short g_chset_ucs2_to_gb2312_table[];
extern const unsigned short g_chset_gb2312_to_ucs2_table[];
extern unsigned short mmi_chset_gb2312_to_ucs2_MSB_size(void);
#endif /* __MMI_CHSET_GB2312__ */ 

#ifdef __MMI_CHSET_HKSCS__
extern const key_index_t g_chset_ucs2_to_hkscs_key_LSB_index[];
extern const key_index_t g_chset_hkscs_to_ucs2_key_LSB_index[];
extern const unsigned char g_chset_ucs2_to_hkscs_key_MSB[];
extern const unsigned char g_chset_hkscs_to_ucs2_key_MSB[];
extern const unsigned short g_chset_ucs2_to_hkscs_table[];
extern const unsigned short g_chset_hkscs_to_ucs2_table[];
#endif /* __MMI_CHSET_HKSCS__ */ 

#ifdef __MMI_CHSET_GB18030__
typedef struct
{
    u16_t gb_code;
    u16_t ucs2_code;
} mmi_chset_gb18030_ucs2_2_byte_struct;

#if 0
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
#endif /* 0 */ 

typedef struct
{
    u16_t gb_code_high;
    u16_t gb_code_low;
    u16_t ucs2_code;
} mmi_chset_gb18030_ucs2_4_byte_struct;

#if 0
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
#endif /* 0 */ 

extern const mmi_chset_gb18030_ucs2_2_byte_struct g_chset_gb18030_to_ucs2_2_byte_tbl[];
extern const mmi_chset_gb18030_ucs2_4_byte_struct g_chset_gb18030_to_ucs2_4_byte_tbl[];
extern const mmi_chset_gb18030_ucs2_2_byte_struct g_chset_ucs2_to_gb18030_2_byte_tbl[];

extern u32_t mmi_chset_gb18030_ucs2_2byte_size(void);
extern u32_t mmi_chset_gb18030_ucs2_4byte_size(void);
extern u32_t mmi_chset_ucs2_gb18030_2byte_size(void);

#endif /* __MMI_CHSET_GB18030__ */ 

#ifdef __MMI_CHSET_SJIS__
extern const key_index_t g_chset_ucs2_to_sjis_key_LSB_index[];
extern const key_index_t g_chset_sjis_to_ucs2_key_LSB_index[];
extern const unsigned char g_chset_ucs2_to_sjis_key_MSB[];
extern const unsigned char g_chset_sjis_to_ucs2_key_MSB[];
extern const unsigned short g_chset_ucs2_to_sjis_table[];
extern const unsigned short g_chset_sjis_to_ucs2_table[];
#endif /* __MMI_CHSET_SJIS__ */


#endif/*__CHARSETTABLE_H__*/
