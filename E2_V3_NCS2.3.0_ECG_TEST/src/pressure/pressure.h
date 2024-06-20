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
#define PRESSURE_DPS368

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

extern bool get_pressure_ok_flag;

#endif/*__PRESSURE_H__*/

