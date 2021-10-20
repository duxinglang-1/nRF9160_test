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
typedef FuncPtr (* get_func_ptr)(u8_t key_code, u8_t key_type);

typedef enum
{
	ACTIVE_LOW,
	ACTIVE_HIGH
}KEY_ACTIVE;

typedef enum
{
	KEY_SOS,
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

#define KEY_SOFT_LEFT	KEY_SOS
#define KEY_SOFT_RIGHT	KEY_SOS

typedef struct
{
	const char * const port;
	const u8_t number;
	const u8_t active_flag;
}key_cfg;

typedef struct
{
	bool flag[KEY_MAX][KEY_EVENT_MAX];
	FuncPtr func[KEY_MAX][KEY_EVENT_MAX];
}key_event_msg;

/**
 * @typedef button_handler_t
 * @brief Callback that is executed when a button state change is detected.
 *
 * @param button_state Bitmask of button states.
 * @param has_changed Bitmask that shows which buttons have changed.
 */
typedef void (*button_handler_t)(u32_t button_state, u32_t has_changed);

/** Button handler list entry. */
struct button_handler {
	button_handler_t cb; /**< Callback function. */
	sys_snode_t node; /**< Linked list node, for internal use. */
};

/** @brief Initialize the library to read the button state.
 *
 *  @param  button_handler Callback handler for button state changes.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int dk_buttons_init(button_handler_t button_handler);

/** @brief Read current button states.
 *
 *  @param button_state Bitmask of button states.
 *  @param has_changed Bitmask that shows which buttons have changed.
 */
void dk_read_buttons(u32_t *button_state, u32_t *has_changed);

/** @brief Get current button state from internal variable.
 *
 *  @return Bitmask of button states.
 */
u32_t dk_get_buttons(void);

/** @brief Set value of LED pins as specified in one bitmask.
 *
 *  @param  leds Bitmask that defines which LEDs to turn on and off.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */

extern void ClearAllKeyHandler(void);
extern void SetKeyHandler(FuncPtr funcPtr, u8_t keycode, u8_t keytype);
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
