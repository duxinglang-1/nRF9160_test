/****************************************Copyright (c)************************************************
** File Name:			    strings.c
** Descriptions:			strings source file
** Created By:				xie biao
** Created Date:			2025-10-10
** Modified Date:      		2025-10-10 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include "external_flash.h"
#include "settings.h"
#include "strings.h"
#include "strdef.h"

strlib_infor_t strlib_infor = {0};

uint16_t *GetStrDataFromFlashByID(RES_STRINGS_ID str_id)
{
	uint8_t data[4] = {0};
	dataindex *ptr_index;
	uint16_t *ptr_data;
	uint32_t i;

	if(str_id >= STR_ID_MAX)
		return NULL;

	if(strlib_infor.str_count == 0 || strlib_infor.lang_count == 0)
	{
		SpiFlash_Read(data, STR_DATA_ADDR, 4);
		strlib_infor.str_count = (data[1]<<8) + data[0];
		strlib_infor.lang_count = (data[3]<<8) + data[2];
		strlib_infor.index_len = strlib_infor.str_count*strlib_infor.lang_count*sizeof(dataindex);
		if(strlib_infor.index_len == 0)
			return NULL;
		
		strlib_infor.p_index = k_malloc(strlib_infor.lang_count*sizeof(dataindex));
		if(strlib_infor.p_index == NULL)
		{
			return NULL;
		}
	}

	SpiFlash_Read((uint8_t*)strlib_infor.p_index, STR_DATA_ADDR+4+str_id*strlib_infor.lang_count*sizeof(dataindex), strlib_infor.lang_count*sizeof(dataindex));
	ptr_index = strlib_infor.p_index;
	for(i=0;i<global_settings.language;i++)
		ptr_index++;
	
	ptr_data = k_malloc(ptr_index->len+2);
	if(ptr_data)
	{
		memset((void*)ptr_data, 0, ptr_index->len+2);
		SpiFlash_Read((uint8_t*)ptr_data, STR_DATA_ADDR+4+strlib_infor.index_len+ptr_index->addr, ptr_index->len);
		return ptr_data;
	}
	else
	{
		return NULL;
	}
}

void StrCpyByID(uint8_t *strDestination, RES_STRINGS_ID str_id)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		mmi_ucs2cpy(strDestination, (uint8_t*)ptr);
		k_free(ptr);
	}
}

void StrCatByID(uint8_t *strDestination, RES_STRINGS_ID str_id)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		mmi_ucs2cat(strDestination, (uint8_t*)ptr);
		k_free(ptr);
	}
}

void StrSmartCpyByID(uint8_t *strDestination, RES_STRINGS_ID str_id, uint32_t len)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		mmi_ucs2smartcpy(strDestination, (uint8_t*)ptr, len);
		k_free(ptr);
	}
}

void LCD_MeasureUniStr(RES_STRINGS_ID str_id, uint16_t *width, uint16_t *height)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		LCD_MeasureUniString(ptr, width, height);
		k_free(ptr);
	}
}

void LCD_ShowUniStr(uint16_t x, uint16_t y, RES_STRINGS_ID str_id)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		LCD_ShowUniString(x, y, ptr);
		k_free(ptr);
	}
}

void LCD_ShowUniStrRtoL(uint16_t x, uint16_t y, RES_STRINGS_ID str_id)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		LCD_ShowUniStringRtoL(x, y, ptr);
		k_free(ptr);
	}
}

void LCD_SmartShowUniStr(uint16_t x, uint16_t y, RES_STRINGS_ID str_id)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		LCD_SmartShowUniString(x, y, ptr);
		k_free(ptr);
	}
}

void LCD_ShowUniStrInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, RES_STRINGS_ID str_id)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		LCD_ShowUniStringInRect(x, y, width, height, ptr);
		k_free(ptr);
	}
}

void LCD_ShowUniStrRtoLInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, RES_STRINGS_ID str_id)
{
	uint16_t *ptr;

	if(str_id >= STR_ID_MAX)
		return;

	ptr = GetStrDataFromFlashByID(str_id);
	if(ptr)
	{
		LCD_ShowUniStringRtoLInRect(x, y, width, height, ptr);
		k_free(ptr);
	}
}