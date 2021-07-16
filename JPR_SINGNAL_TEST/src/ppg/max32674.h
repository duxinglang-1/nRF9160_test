/****************************************Copyright (c)************************************************
** File Name:			    max32674.h
** Descriptions:			PPG process head file
** Created By:				xie biao
** Created Date:			2021-05-19
** Modified Date:      		2021-05-19 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __MAX32674_H__
#define __MAX32674_H__

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>

/** @addtogroup  Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*ppgdev_write_ptr)(void *, u8_t, u8_t, u8_t*, u16_t);
typedef int32_t (*ppgdev_read_ptr) (void *, u8_t, u8_t, u8_t*, u16_t);

typedef struct {
  /** Component mandatory fields **/
  ppgdev_write_ptr  write_reg;
  ppgdev_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
}ppgdev_ctx_t;

extern ppgdev_ctx_t ppg_dev_ctx;

extern u16_t g_hr;
extern u16_t g_spo2;

extern void PPG_init(void);
extern void PPGMsgProcess(void);

/*heart rate*/
extern void GetHeartRate(u8_t *HR);

#endif/*__MAX32674_H__*/
