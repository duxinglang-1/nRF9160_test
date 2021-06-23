/****************************************Copyright (c)************************************************
** File Name:			    max32674.c
** Descriptions:			PPG process source file
** Created By:				xie biao
** Created Date:			2021-05-19
** Modified Date:      		2021-05-19 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <nrf_socket.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <nrfx.h>
#include "Max32674.h"
#include "max_sh_interface.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(ppg, CONFIG_LOG_DEFAULT_LEVEL);

bool ppg_int_event = false;
bool ppg_fw_upgrade_flag = false;

static u8_t whoamI=0, rst=0;

ppgdev_ctx_t ppg_dev_ctx;

u8_t g_heart_rate = 0;

void GetHeartRate(u8_t *HR)
{
	u32_t heart;

	while(1)
	{
		heart = sys_rand32_get();
		if(((heart%200)>=60) && ((heart%200)<=160))
		{
			*HR = (heart%200);
			break;
		}
	}
}

void GetPPGVersion(ppgdev_ctx_t *ctx, u8_t *buff)
{
	//max32674_read_reg(ctx, 0x81, 0x01, buff, 1);

	LOG_INF("[%s] version:%x\n", __func__, buff);
}

void PPG_init(void)
{
	LOG_INF("PPG_init\n");

	if(!sh_init_interface())
		return;
	
	LOG_INF("PPG_init done!\n");
}

void PPGMsgProcess(void)
{
	if(ppg_int_event)
	{
		ppg_int_event = false;
	}
	if(ppg_fw_upgrade_flag)
	{
		SH_OTA_upgrade_process();
		ppg_fw_upgrade_flag = false;
	}
	//GetPPGVersion(&ppg_dev_ctx, &whoamI);
	//k_sleep(K_MSEC(1000));
}

