/**************************************************************************
 *Name        : CST816.h
 *Author      : xie biao
 *Version     : V1.0
 *Create      : 2020-08-21
 *Copyright   : August
**************************************************************************/
#ifdef CONFIG_TOUCH_SUPPORT
#ifndef __CST816_H__
#define __CST816_H__

#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <stdio.h>

#define CTP_PORT 	"GPIO_0"
#define CTP_DEV 	"I2C_1"

#define CTP_RESET		16
#define CTP_EINT		25
#define CTP_SCL			1
#define CTP_SDA			0

#define CST816_I2C_ADDRESS	 	0x15
#define CST816_CHIP_ID			0xB4

#define CST816_REG_GESTURE				0x01
#define CST816_REG_FINGER_NUM			0x02
#define CST816_REG_XPOS_H				0x03
#define CST816_REG_XPOS_L				0x04
#define CST816_REG_YPOS_H				0x05
#define CST816_REG_YPOS_L				0x06
#define CST816_REG_BPC0_H				0xB0
#define CST816_REG_BPC0_L				0xB1
#define CST816_REG_BPC1_H				0xB2
#define CST816_REG_BPC1_L				0xB3
#define CST816_REG_CHIPID				0xA7
#define CST816_REG_PROJID				0xA8
#define CST816_REG_FW_VER				0xA9
#define CST816_REG_SLEEP_MODE			0xE5
#define CST816_REG_ERR_RESET			0xEA
#define CST816_REG_LONG_PRESS_TICK		0xEB
#define CST816_REG_MOTION_MASK			0xEC
#define CST816_REG_IRQ_PLUSE_WIDTH		0xED
#define CST816_REG_NOR_SCAN_PER			0xEE
#define CST816_REG_MOTION_SL_ANGLE		0xEF
#define CST816_REG_LP_SCAN_RAW1_H		0xF0
#define CST816_REG_LP_SCAN_RAW1_L		0xF1
#define CST816_REG_LP_SCAN_RAW2_H		0xF2
#define CST816_REG_LP_SCAN_RAW2_L		0xF3
#define CST816_REG_LP_AUTO_WAKE_TIME	0xF4
#define CST816_REG_LP_SCAN_TH			0xF5
#define CST816_REG_LP_SCAN_WIN			0xF6
#define CST816_REG_LP_SCAN_FREQ			0xF7
#define CST816_REG_LP_SCAN_IDAC			0xF8
#define CST816_REG_AUTO_SLEEP_TIME		0xF9
#define CST816_REG_IRQ_CTL				0xFA
#define CST816_REG_AUTO_RESET			0xFB
#define CST816_REG_LONG_PRESS_TIME		0xFC
#define CST816_REG_IO_CTL				0xFD
#define CST816_REG_DIS_AUTO_SLEEP		0xFE

#define GESTURE_NONE			0x00
#define GESTURE_MOVING_UP		0x02
#define GESTURE_MOVING_DOWN		0x01
#define GESTURE_MOVING_LEFT		0x03
#define GESTURE_MOVING_RIGHT	0x04
#define GESTURE_SINGLE_CLICK	0x05
#define GESTURE_DOUBLE_CLICK	0x0B
#define GESTURE_LONG_PRESS		0x0C

typedef enum
{
	TP_EVENT_NONE,
	TP_EVENT_MOVING_UP,
	TP_EVENT_MOVING_DOWN,
	TP_EVENT_MOVING_LEFT,
	TP_EVENT_MOVING_RIGHT,
	TP_EVENT_SINGLE_CLICK,
	TP_EVENT_DOUBLE_CLICK,
	TP_EVENT_LONG_PRESS,
	TP_EVENT_MAX
}TP_EVENT;

typedef struct
{
	TP_EVENT evt_id;
	u16_t x_pos;
	u16_t y_pos;
}tp_message;

typedef void (*tp_handler_t)(void);

struct tpnode
{
	u16_t x_begin;
	u16_t x_end;
	u16_t y_begin;
	u16_t y_end;
	TP_EVENT evt_id;
	tp_handler_t func;
	struct node *next;
};

typedef struct
{
	u32_t count;
	struct tpnode *cache;
}TPInfo;

typedef struct tpnode TpEventNode;


extern bool tp_trige_flag;
extern bool tp_redraw_flag;

extern tp_message tp_msg;

extern void CST816_init(void);
extern void tp_interrupt_proc(void);
extern void TPMsgProcess(void);
extern void unregister_touch_event_handle(TP_EVENT tp_type, u16_t x_start, u16_t x_stop, u16_t y_start, u16_t y_stop, tp_handler_t touch_handler);
extern void register_touch_event_handle(TP_EVENT tp_type, u16_t x_start, u16_t x_stop, u16_t y_start, u16_t y_stop, tp_handler_t touch_handler);
#endif/*__CST816_H__*/
#endif/*CONFIG_TOUCH_SUPPORT*/
