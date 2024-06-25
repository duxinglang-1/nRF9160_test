/****************************************Copyright (c)************************************************
** File Name:			    pressure.h
** Descriptions:			pressure message process head file
** Created By:				xie biao
** Created Date:			2024-06-18
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __PRESSURE_H__
#define __PRESSURE_H__

#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <math.h>
#include "datetime.h"

#define PRESSURE_DEBUG

//sensor mode
//#define PRESSURE_DPS368
#define PRESSURE_LPS22DF

typedef int32_t (*pressure_write_ptr)(struct device *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
typedef int32_t (*pressure_read_ptr)(struct device *handle, uint8_t reg, uint8_t *bufp, uint16_t len);

typedef struct
{
  /** Component mandatory fields **/
  pressure_write_ptr  write_reg;
  pressure_read_ptr   read_reg;
  /** Customizable optional pointer **/
  struct device *handle;
}pressure_ctx_t;

extern bool pressure_check_ok;
extern bool pressure_start_flag;
extern bool pressure_stop_flag;
extern bool pressure_interrupt_flag;

extern float g_prs;
extern float g_tmp;

extern pressure_ctx_t pressure_dev_ctx;

#endif/*__PRESSURE_H__*/

