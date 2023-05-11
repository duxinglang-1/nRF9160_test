/****************************************Copyright (c)************************************************
** File Name:			    Key.h
** Descriptions:			Key message process head file
** Created By:				xie biao
** Created Date:			2020-07-13
** Modified Date:      		2021-09-29 
** Version:			    	V1.2
******************************************************************************************************/
#ifndef __KEY_H__
#define __KEY_H__

#include <zephyr/types.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DK_NO_BTNS_MSK   (0)
#define DK_BTN1          0
#define DK_BTN2          1
#define DK_BTN1_MSK      BIT(DK_BTN1)
#define DK_BTN2_MSK      BIT(DK_BTN2)
#define DK_ALL_BTNS_MSK  (DK_BTN1_MSK | DK_BTN2_MSK)

#define KEY_UP 			0
#define KEY_DOWN 		1
#define KEY_LONG_PRESS	2

#define TIMER_FOR_LONG_PRESSED 3*1000

typedef void (*FuncPtr) (void);
typedef FuncPtr (* get_func_ptr)(uint8_t key_code, uint8_t key_type);

typedef enum
{
	ACTIVE_LOW,
	ACTIVE_HIGH
}KEY_ACTIVE;

typedef enum
{
	KEY_SOS,
	KEY_POWER,
	KEY_MAX
}KEY_CODE;

typedef enum
{
	KEY_EVENT_UP,
	KEY_EVENT_DOWN,
	KEY_EVENT_LONG_PRESS,
	KEY_EVENT_MAX
}KEY_EVENT_TYPE;

typedef enum 
{
	STATE_WAITING,
	STATE_SCANNING,
}KEY_STATUS;

#define KEY_SOFT_LEFT	KEY_POWER
#define KEY_SOFT_RIGHT	KEY_SOS

typedef struct
{
	const char * const port;
	const uint8_t number;
	const uint8_t active_flag;
}key_cfg;

typedef struct
{
	bool flag[KEY_MAX][KEY_EVENT_MAX];
	FuncPtr func[KEY_MAX][KEY_EVENT_MAX];
}key_event_msg;

typedef void (*button_handler_t)(uint32_t button_state, uint32_t has_changed);

struct button_handler
{
	button_handler_t cb; /**< Callback function. */
	sys_snode_t node; /**< Linked list node, for internal use. */
};

extern bool touch_flag;

extern void ClearAllKeyHandler(void);
extern void ClearKeyHandler(uint8_t keycode, uint8_t keytype);
extern void ClearLeftKeyUpHandler(void);
extern void ClearLeftKeyDownHandler(void);
extern void ClearLeftKeyLongPressHandler(void);
extern void ClearRightKeyUpHandler(void);
extern void ClearRightKeyDownHandler(void);
extern void ClearRightKeyLongPressHandler(void);
extern void SetKeyHandler(FuncPtr funcPtr, uint8_t keycode, uint8_t keytype);
extern void SetLeftKeyUpHandler(FuncPtr funcPtr);
extern void SetLeftKeyDownHandler(FuncPtr funcPtr);
extern void SetLeftKeyLongPressHandler(FuncPtr funcPtr);
extern void SetRightKeyUpHandler(FuncPtr funcPtr);
extern void SetRightKeyDownHandler(FuncPtr funcPtr);
extern void SetRightKeyLongPressHandler(FuncPtr funcPtr);
extern void KeyMsgProcess(void);

#ifdef __cplusplus
}
#endif

#endif/*__KEY_H__*/
