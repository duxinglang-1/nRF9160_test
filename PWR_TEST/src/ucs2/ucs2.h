/****************************************Copyright (c)************************************************
** File Name:			    ucs2.c
** Descriptions:			Operations related to Unicode coded strings head file
** Created By:				xie biao
** Created Date:			2020-12-29
** Modified Date:      		2020-12-29 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __UCS2_H__
#define __UCS2_H__

#include <zephyr/types.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

extern s32_t mmi_ucs2strlen(const u8_t *arrOut);
extern u8_t *mmi_ucs2cpy(u8_t *strDestination, const u8_t *strSource);
extern u8_t *mmi_ucs2cat(u8_t *strDestination, const u8_t *strSource);
extern s32_t mmi_ucs2cmp(const u8_t *string1, const u8_t *string2);
extern u8_t *mmi_ucs2chr(const u8_t *strSrc, u16_t c);
extern u8_t *mmi_ucs2str(const u8_t *str1, const u8_t *str2);
extern u16_t mmi_asc_to_ucs2(u8_t *pOutBuffer, u8_t *pInBuffer);
extern u16_t mmi_ucs2_to_asc(u8_t *pOutBuffer, u8_t *pInBuffer);

#ifdef __cplusplus
}
#endif

#endif/*__UCS2_H__*/
