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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#define L(x) ASCTOUCS2(x)

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t mmi_ucs2strlen(const uint8_t *arrOut);
extern uint8_t *mmi_ucs2cpy(uint8_t *strDestination, const uint8_t *strSource);
extern uint8_t *mmi_ucs2ncpy(uint8_t *strDestination, const uint8_t *strSource, uint32_t len);
extern uint8_t *mmi_ucs2cat(uint8_t *strDestination, const uint8_t *strSource);
extern uint8_t *mmi_ucs2ncat(uint8_t *strDestination, const uint8_t *strSource, uint32_t len);
extern int32_t mmi_ucs2cmp(const uint8_t *string1, const uint8_t *string2);
extern uint8_t *mmi_ucs2chr(const uint8_t *strSrc, uint16_t c);
extern uint8_t *mmi_ucs2str(const uint8_t *str1, const uint8_t *str2);
extern uint16_t mmi_asc_to_ucs2(uint8_t *pOutBuffer, uint8_t *pInBuffer);
extern uint16_t mmi_ucs2_to_asc(uint8_t *pOutBuffer, uint8_t *pInBuffer);
extern uint16_t *ASCTOUCS2(uint8_t *x);
#ifdef __cplusplus
}
#endif

#endif/*__UCS2_H__*/
