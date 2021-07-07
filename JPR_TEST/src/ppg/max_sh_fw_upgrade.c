/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
********************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "external_flash.h"
#include "max_sh_interface.h"
#include "max_sh_fw_upgrade.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(max_sh_fw_upgrade, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef SH_OTA_DATA_STORE_IN_FLASH
s32_t SH_OTA_upgrade_process(void)
{
	s32_t s32_status;
	u8_t u8_rxbuf[3]={0};

	LOG_INF("start to upgrade MAX32674 firmware\n");

	//hardware method to enter BL mode
	SH_rst_to_BL_mode();

	//set software work mode command
	s32_status = sh_put_in_bootloader();
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("set bl mode fail, %x\n", s32_status);
		return s32_status;
	}

	//check MCU type
	s32_status = sh_get_bootloader_MCU_tye(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Read MCU type fail, %x\n", s32_status);
		return s32_status;
	}
	LOG_INF("MCU type = %d \n", u8_rxbuf[0]);

	//check working mode and FW version
	s32_status = sh_get_hub_fw_version(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("read FW version fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("FW version is %d.%d.%d \n", u8_rxbuf[0], u8_rxbuf[1], u8_rxbuf[2]);
	}

	//read page size
	u16_t u16_pageSize;
	s32_status = sh_get_bootloader_pagesz(&u16_pageSize);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("read FW version fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("page size is %d \n", u16_pageSize);
	}

	//set page number
	u8_t u8_pageNumber;
	SpiFlash_Read(&u8_pageNumber, PPG_ALGO_FW_ADDR+BL_PAGE_COUNT_INDEX, sizeof(u8_pageNumber));
	s32_status = sh_set_bootloader_numberofpages(u8_pageNumber);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("set page count fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("set page count done %d\n", u8_pageNumber);
	}

	//Set vector bytes
	u8_t u8p_ivData[BL_AES_NONCE_SIZE] = {0};
	SpiFlash_Read(u8p_ivData, PPG_ALGO_FW_ADDR+BL_IV_INDEX, BL_AES_NONCE_SIZE);
	s32_status = sh_set_bootloader_iv(u8p_ivData);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Set the vector bytes fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("Setting the vector bytes is done \n");
	}

	//Set auth bytes
	u8_t u8p_authData[BL_AES_AUTH_SIZE];
	SpiFlash_Read(u8p_authData, PPG_ALGO_FW_ADDR+BL_AUTH_INDEX, BL_AES_AUTH_SIZE);
	s32_status = sh_set_bootloader_auth(u8p_authData);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Set the authentication fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("Setting the authentication is done \n");
	}

	u32_t u32_partialSize = BL_FLASH_PARTIAL_SIZE;
	s32_status = sh_set_bootloader_partial_write_size(u32_partialSize);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Set partial write size fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("Set partial write size done %d \n", u32_partialSize);
	}

	s32_status = sh_set_bootloader_erase();
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Erase flash fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("Erasing flash is done \n");
	}

	s32_status = sh_set_bootloader_flashpages(PPG_ALGO_FW_ADDR, u8_pageNumber);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Write page fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("All page is flashed \n");
	}

	SH_rst_to_APP_mode();

	//check MCU type
	s32_status = sh_get_bootloader_MCU_tye(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Read MCU type fail, %x \n", s32_status);
		return s32_status;
	}
	LOG_INF("MCU type = %d \n", u8_rxbuf[0]);

	//check working mode and FW version
	s32_status = sh_get_hub_fw_version(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("read FW version fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("FW version is %d.%d.%d \n", u8_rxbuf[0], u8_rxbuf[1], u8_rxbuf[2]);
	}

	return s32_status;
}

#else

s32_t SH_OTA_upgrade_process(u8_t* u8p_FwData)
{
	s32_t s32_status;
	u8_t u8_rxbuf[3];

	LOG_INF("start to upgrade MAX32674 firmware \n");

	//hardware method to enter BL mode
	SH_rst_to_BL_mode();

	//set software work mode command
	s32_status = sh_put_in_bootloader();
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("set bl mode fail, %x\n", s32_status);
		return s32_status;
	}

	//check MCU type
	s32_status = sh_get_bootloader_MCU_tye(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Read MCU type fail, %x\n", s32_status);
		return s32_status;
	}
	LOG_INF("MCU type = %d \n", u8_rxbuf[0]);

	//check working mode and FW version
	s32_status = sh_get_hub_fw_version(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("read FW version fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("FW version is %d.%d.%d \n", u8_rxbuf[0], u8_rxbuf[1], u8_rxbuf[2]);
	}

	//read page size
	u16_t u16_pageSize;
	s32_status = sh_get_bootloader_pagesz(&u16_pageSize);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("read FW version fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("page size is %x \n", u16_pageSize);
	}

	//set page number
	u8_t u8_pageNumber = u8p_FwData[BL_PAGE_COUNT_INDEX];
	s32_status =  sh_set_bootloader_numberofpages(u8_pageNumber);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("set page count fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("set page count is done \n");
	}

	//Set vector bytes
	u8_t* u8p_ivData = &u8p_FwData[BL_IV_INDEX];
	s32_status =  sh_set_bootloader_iv(u8p_ivData);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Set the  vector bytes fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("Setting the  vector bytes is done \n");
	}

	//Set vector bytes
	u8_t* u8p_authData = &u8p_FwData[BL_AUTH_INDEX];
	s32_status = sh_set_bootloader_auth(u8p_authData);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Set the  authentication fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("Setting the authentication is done \n");
	}

	s32_status =  sh_set_bootloader_erase();
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Erase flash fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("Erasing flash is done \n");
	}

	s32_status = sh_set_bootloader_flashpages(u8p_FwData, u8_pageNumber);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Write page fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("All page is flashed \n");
	}

	SH_rst_to_APP_mode();

	//check MCU type
	s32_status = sh_get_bootloader_MCU_tye(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("Read MCU type fail, %x \n", s32_status);
		return s32_status;
	}
	LOG_INF("MCU type = %d \n", u8_rxbuf[0]);

	//check working mode and FW version
	s32_status = sh_get_hub_fw_version(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
		LOG_INF("read FW version fail %x \n", s32_status);
		return s32_status;
	}
	else
	{
		LOG_INF("FW version is %d.%d.%d \n", u8_rxbuf[0], u8_rxbuf[1], u8_rxbuf[2]);
	}

	return s32_status;
}
#endif/*SH_OTA_DATA_STORE_IN_FLASH*/

