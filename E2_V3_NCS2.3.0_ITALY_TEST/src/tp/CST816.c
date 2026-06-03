/**************************************************************************
 *Name        : CST816.c
 *Author      : xie biao
 *Version     : V1.0
 *Create      : 2020-08-21
 *Copyright   : August
**************************************************************************/
#ifdef CONFIG_TOUCH_SUPPORT

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <dk_buttons_and_leds.h>
#include "lcd.h"
#include "CST816.h"
#include "CST816S_update.h"
#include "CST816T_update.h"
#include "logger.h"

//#define TP_DEBUG
//#define TP_TEST

#if defined(TP_DEBUG)||defined(TP_TEST)
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define TP_DEV DT_NODELABEL(i2c1)
#else
#error "i2c1 devicetree node is disabled"
#define TP_DEV	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define TP_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define TP_PORT	""
#endif

#define TP_RESET		16
#define TP_EINT			25
#define TP_SCL			1
#define TP_SDA			0
#endif

bool tp_int_flag = false;
bool tp_trige_flag = false;
bool tp_redraw_flag = false;

uint8_t tp_chip_id = 0x00;
uint8_t tp_fw_ver = 0x00;

tp_message tp_msg = {0};

struct device *i2c_ctp;
struct device *gpio_ctp;

static struct gpio_callback gpio_cb;

static TPInfo tp_event_info = {0};
static TpEventNode *tp_event_tail = NULL;

#if defined(TP_DEBUG)||defined(TP_TEST)
static uint8_t init_i2c(void)
{
	i2c_ctp = DEVICE_DT_GET(TP_DEV);
	if(!i2c_ctp)
	{
	#ifdef TP_DEBUG
		LOGD("ERROR SETTING UP I2C");
	#endif
		return -1;
	} 
	else
	{
		i2c_configure(i2c_ctp, I2C_SPEED_SET(I2C_SPEED_FAST));
		return 0;
	}
}

static int32_t platform_write(uint8_t reg, uint8_t *bufp, uint16_t len)
{
	uint8_t data[len+1];
	uint32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(i2c_ctp, data, len+1, TP_I2C_ADDRESS);
	return rslt;
}

static int32_t platform_read(uint8_t reg, uint8_t *bufp, uint16_t len)
{
	uint32_t rslt = 0;

	rslt = i2c_write(i2c_ctp, &reg, 1, TP_I2C_ADDRESS);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ctp, bufp, len, TP_I2C_ADDRESS);
	}
	return rslt;
}

static int32_t platform_write_word(uint16_t reg, uint8_t *bufp, uint16_t len)
{
	uint8_t data[len+2];
	uint32_t rslt = 0;

	data[0] = reg>>8;
	data[1] = reg&0xFF;

	memcpy(&data[2], bufp, len);
	rslt = i2c_write(i2c_ctp, data, len+2, TP_UPDATE_I2C_ADDRESS);
	return rslt;
}

static int32_t platform_read_word(uint16_t reg, uint8_t *bufp, uint16_t len)
{
	uint8_t data[2];
	uint32_t rslt = 0;

	data[0] = reg>>8;
	data[1] = reg&0xFF;

	rslt = i2c_write(i2c_ctp, data, 2, TP_UPDATE_I2C_ADDRESS);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ctp, bufp, len, TP_UPDATE_I2C_ADDRESS);
	}

	return rslt;
}

static void tp_reset(void)
{
	gpio_pin_configure(gpio_ctp, TP_RESET, GPIO_OUTPUT);
	gpio_pin_set(gpio_ctp, TP_RESET, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_set(gpio_ctp, TP_RESET, 1);
	k_sleep(K_MSEC(50));
}

static int cst816s_enter_bootmode(void)
{
	uint8_t retryCnt = 50;
	uint8_t cmd[3] = {0};

	gpio_pin_set(gpio_ctp, TP_RESET, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_set(gpio_ctp, TP_RESET, 1);
	k_sleep(K_MSEC(10));

	while(retryCnt--)
	{
		cmd[0] = 0xAB;
		platform_write_word(0xA001,cmd,1);
		k_sleep(K_MSEC(10));
		platform_read_word(0xA003,cmd,1);
	
		if((cmd[0] != 0x55) && (cmd[0] != 0xC1))
		{
			k_sleep(K_MSEC(10));
			continue;
		}
		else
		{
			return 0;
		}
		k_sleep(K_MSEC(10));

	}
	return -1;
}

#define PER_LEN	512

static int cst816s_update(uint16_t startAddr, uint16_t len, unsigned char *src)
{
	uint8_t cmd[10] = {0};
	uint32_t sum_len = 0;
	uint32_t i,k_data=0,b_data=0;

	if(cst816s_enter_bootmode() == -1)
		return -1;

	sum_len = 0;
	k_data=len/PER_LEN;
	for(i=0;i<k_data;i++)
	{
		uint8_t retrycnt = 50;

		if(sum_len >= len)
			return -1;
		
		cmd[0] = startAddr&0xFF;
		cmd[1] = startAddr>>8;
		platform_write_word(0xA014,cmd,2);
		k_sleep(K_MSEC(2));
		
		platform_write_word(0xA018,src,PER_LEN);
		k_sleep(K_MSEC(4));

		cmd[0] = 0xEE;
		platform_write_word(0xA004,cmd,1);
		k_sleep(K_MSEC(100));

		while(retrycnt--)
		{
			cmd[0] = 0;
			platform_read_word(0xA005,cmd,1);
			if(cmd[0] == 0x55)
			{
				cmd[0] = 0;
				break;
			}
			k_sleep(K_MSEC(10));
		}

		startAddr += PER_LEN;
		src += PER_LEN;
		sum_len += PER_LEN;
	}

	//exit program mode
	cmd[0] = 0x00;
	platform_write_word(0xA003,cmd,1);
	return 0;
}

static uint32_t cst816s_read_checksum(void)
{
	union
	{
		uint32_t sum;
		uint8_t buf[4];
	}checksum;
	
	uint8_t cmd[3];
	char readback[4] = {0};

	if(cst816s_enter_bootmode() == -1)
	{
		return -1;
	}

	cmd[0] = 0;
	platform_write_word(0xA003,cmd,1);
	k_sleep(K_MSEC(500));

	checksum.sum = 0;
	platform_read_word(0xA008,checksum.buf,2);
	return checksum.sum;
}


bool ctp_hynitron_update(void)
{
	uint8_t lvalue;
	uint8_t write_data[2];
	uint8_t tmpbuf[128]={0};
	bool temp_result = true;

	if(cst816s_enter_bootmode() == 0)
	{
		uint8_t *ptr = NULL;
		uint16_t startAddr;
		uint16_t length;					
		uint16_t checksum;
		
		switch(tp_chip_id)
		{
		case TP_CST816S:
			if(sizeof(cst816s_app_bin) > 10)
				ptr = cst816s_app_bin;
			else
				return false;
			break;
			
		case TP_CST816T:
			if(sizeof(cst816t_app_bin) > 10)
				ptr = cst816t_app_bin;
			else
				return false;
			break;
		}

		startAddr = *(ptr+1)<<8 | *(ptr+0);
		length = *(ptr+3)<<8 | *(ptr+2);
		checksum = *(ptr+5)<<8 | *(ptr+4);  

		if(cst816s_read_checksum()!= checksum)
		{
			LCD_ShowString(20,160,"start update!");
			
			cst816s_update(startAddr, length, ptr+6);
			cst816s_read_checksum();

			LCD_ShowString(20,180,"complete update!");
		}

		tp_reset();

		return true;
	}
	return false;
}
#endif

void clear_all_touch_event_handle(void)
{
	TpEventNode *pnext;
	
	if(tp_event_info.cache == NULL || tp_event_info.count == 0)
	{
		return;
	}
	else
	{
		pnext = tp_event_info.cache;

		do
		{
			tp_event_info.cache = pnext->next;
			tp_event_info.count--;
			k_free(pnext);
			pnext = tp_event_info.cache;
		}while(pnext != NULL);
	}
}

void unregister_touch_event_handle(TP_EVENT tp_type, uint16_t x_start, uint16_t x_stop, uint16_t y_start, uint16_t y_stop, tp_handler_t touch_handler)
{
	TpEventNode *ppre,*pnext;
	
	if(tp_event_info.cache == NULL || tp_event_info.count == 0)
	{
		return;
	}
	else
	{
		ppre = NULL;
		pnext = tp_event_info.cache;
		
		do
		{
			if((pnext->x_begin == x_start)&&(pnext->x_end == x_stop)&&(pnext->y_begin == y_start)&&(pnext->y_end == y_stop)
				&&(pnext->evt_id == tp_type)
				&&(pnext->func == touch_handler))
			{
				if(pnext == tp_event_info.cache)
				{
					tp_event_info.cache = pnext->next;
					tp_event_info.count--;
					k_free(pnext);
				}
				else if(pnext == tp_event_tail)
				{
					tp_event_tail = ppre;
					tp_event_tail->next = NULL;
					tp_event_info.count--;
					k_free(pnext);
				}
				else
				{
					ppre = pnext->next;
					tp_event_info.count--;
					k_free(pnext);
				}

				return;
			}
			else
			{
				ppre = pnext;
				pnext = pnext->next;
			}
				
		}while(pnext != NULL);
	}
}

void register_touch_event_handle(TP_EVENT tp_type, uint16_t x_start, uint16_t x_stop, uint16_t y_start, uint16_t y_stop, tp_handler_t touch_handler)
{
	TpEventNode *pnew;

	if(tp_event_info.cache == NULL)
	{
		tp_event_info.count = 0;
		tp_event_info.cache = NULL;

		tp_event_tail = k_malloc(sizeof(TpEventNode));
		if(tp_event_tail == NULL) 
			return;
	
		memset(tp_event_tail, 0, sizeof(TpEventNode));
		
		tp_event_tail->x_begin = x_start;
		tp_event_tail->x_end = x_stop;
		tp_event_tail->y_begin = y_start;
		tp_event_tail->y_end = y_stop;
		tp_event_tail->evt_id = tp_type;
		tp_event_tail->func = touch_handler;
		tp_event_tail->next = NULL;
		
		tp_event_info.cache = tp_event_tail;
		tp_event_info.count = 1;
	}
	else
	{
		tp_event_tail = tp_event_info.cache;
		while(1)
		{
			if((x_start == tp_event_tail->x_begin)&&(x_stop == tp_event_tail->x_end)&&(y_start == tp_event_tail->y_begin)&&(y_stop == tp_event_tail->y_end)
				&&(tp_event_tail->evt_id == tp_type))
			{
				tp_event_tail->func = touch_handler;
				break;
			}
			else
			{
				if(tp_event_tail->next == NULL)
				{
					pnew = k_malloc(sizeof(TpEventNode));
					if(pnew == NULL) 
						return;

					memset(pnew, 0, sizeof(TpEventNode));
					
					pnew->x_begin = x_start;
					pnew->x_end = x_stop;
					pnew->y_begin = y_start;
					pnew->y_end = y_stop;
					pnew->evt_id = tp_type;
					pnew->func = touch_handler;
					pnew->next = NULL;

					tp_event_tail->next = pnew;
					tp_event_tail = pnew;
					tp_event_info.count++;
					break;
				}
				else
				{
					tp_event_tail = tp_event_tail->next;
				}
			}
		}
	}
}

bool check_touch_event_handle(TP_EVENT tp_type, uint16_t x_pos, uint16_t y_pos)
{
	TpEventNode *pnew;
	
	if(tp_event_info.cache == NULL || tp_event_info.count == 0)
	{
		return false;
	}
	else
	{
		pnew = tp_event_info.cache;
		
		do
		{
			if(pnew->evt_id == tp_type)
			{
				if((pnew->evt_id == TP_EVENT_MOVING_UP)
					||(pnew->evt_id == TP_EVENT_MOVING_DOWN)
					||(pnew->evt_id == TP_EVENT_MOVING_LEFT)
					||(pnew->evt_id == TP_EVENT_MOVING_RIGHT))
				{
					if(pnew->func != NULL)
						pnew->func();
					return true;
				}
				else if((x_pos >= pnew->x_begin)
						&&(x_pos <= pnew->x_end)
						&&(y_pos >= pnew->y_begin)
						&&(y_pos <= pnew->y_end))
				{
					if(pnew->func != NULL)
						pnew->func();
					return true;
				}
				else
				{
					pnew = pnew->next;
				}
			}
			else
			{
				pnew = pnew->next;
			}
				
		}while(pnew != NULL);
		return false;
	}
}

void touch_panel_event_handle(TP_EVENT tp_type, uint16_t x_pos, uint16_t y_pos)
{
	uint8_t strbuf[128] = {0};
	uint16_t x,y,w,h;

	if(lcd_is_sleeping
	#ifdef CONFIG_FACTORY_TEST_SUPPORT
		&&!IsFTCurrentTest()
	#endif
		)
	{
		sleep_out_by_wrist = false;
		lcd_sleep_out = true;
		return;
	}
	
	LCD_ResetBL_Timer();

	switch(tp_type)
	{
	case TP_EVENT_NONE:
	#ifdef TP_DEBUG
		LOGD("tp none!");
	#endif
		break;
	case TP_EVENT_MOVING_UP:
	#ifdef TP_DEBUG
		LOGD("tp moving up! x:%d, y:%d", x_pos,y_pos);
	#endif
		break;
	case TP_EVENT_MOVING_DOWN:
	#ifdef TP_DEBUG
		LOGD("tp moving down! x:%d, y:%d", x_pos,y_pos);
	#endif
		break;
	case TP_EVENT_MOVING_LEFT:
	#ifdef TP_DEBUG
		LOGD("tp moving left! x:%d, y:%d", x_pos,y_pos);
	#endif
		break;
	case TP_EVENT_MOVING_RIGHT:
	#ifdef TP_DEBUG
		LOGD("tp moving right! x:%d, y:%d", x_pos,y_pos);
	#endif
		break;
	case TP_EVENT_SINGLE_CLICK:
	#ifdef TP_DEBUG
		LOGD("tp single click! x:%d, y:%d", x_pos,y_pos);
	#endif
		break;
	case TP_EVENT_DOUBLE_CLICK:
	#ifdef TP_DEBUG
		LOGD("tp double click! x:%d, y:%d", x_pos,y_pos);
	#endif
		break;
	case TP_EVENT_LONG_PRESS:
	#ifdef TP_DEBUG
		LOGD("tp long press! x:%d, y:%d", x_pos,y_pos);
	#endif
		break;
	case TP_EVENT_MAX:
		break;
	}

#ifdef LCD_SHOW_ROTATE_180
	if((tp_type == TP_EVENT_MOVING_UP)
		|| (tp_type == TP_EVENT_MOVING_DOWN)
		|| (tp_type == TP_EVENT_MOVING_LEFT)
		|| (tp_type == TP_EVENT_MOVING_RIGHT))
	{
		if(tp_type == TP_EVENT_MOVING_UP)
			tp_msg.evt_id = TP_EVENT_MOVING_DOWN;
		else if(tp_type == TP_EVENT_MOVING_DOWN)
			tp_msg.evt_id = TP_EVENT_MOVING_UP;
		else if(tp_type == TP_EVENT_MOVING_LEFT)
			tp_msg.evt_id = TP_EVENT_MOVING_RIGHT;
		else if(tp_type == TP_EVENT_MOVING_RIGHT)
			tp_msg.evt_id = TP_EVENT_MOVING_LEFT;
	}
	else
	{
		tp_msg.evt_id = tp_type;
	}
	tp_msg.x_pos = LCD_WIDTH - x_pos;
	tp_msg.y_pos = LCD_HEIGHT - y_pos;
#else
	tp_msg.evt_id = tp_type;
	tp_msg.x_pos = x_pos;
	tp_msg.y_pos = y_pos;
#endif

#ifdef TP_TEST //xb test 2022-04-21
	tp_redraw_flag = true;
#endif
	tp_trige_flag = true;
}

#if defined(TP_DEBUG)||defined(TP_TEST)
void CaptouchInterruptHandle(void)
{
	tp_int_flag = true;
}

void tp_interrupt_proc(void)
{
	uint8_t TP_type = TP_EVENT_MAX;
	uint8_t tp_temp[10]={0};
	uint16_t x_pos,y_pos;
	
	platform_read(TP_REG_GESTURE, &tp_temp[0], 1);//手势
	platform_read(TP_REG_FINGER_NUM, &tp_temp[1], 1);//手指个数
	platform_read(TP_REG_XPOS_H, &tp_temp[2], 1);//x坐标高位 (&0x0f,取低4位)
	platform_read(TP_REG_XPOS_L, &tp_temp[3], 1);//x坐标低位
	platform_read(TP_REG_YPOS_H, &tp_temp[4], 1);//y坐标高位 (&0x0f,取低4位)
	platform_read(TP_REG_YPOS_L, &tp_temp[5], 1);//y坐标低位

#ifdef TP_DEBUG
	LOGD("tp_temp=%x,%x,%x,%x,%x,%x",tp_temp[0],tp_temp[1],tp_temp[2],tp_temp[3],tp_temp[4],tp_temp[5]);
#endif
	switch(tp_temp[0])
	{
	case GESTURE_NONE:
		TP_type = TP_EVENT_NONE;
		break;
	case GESTURE_MOVING_UP:
		TP_type = TP_EVENT_MOVING_UP;
		break;
	case GESTURE_MOVING_DOWN:
		TP_type = TP_EVENT_MOVING_DOWN;
		break;
	case GESTURE_MOVING_LEFT:
		TP_type = TP_EVENT_MOVING_LEFT;
		break;
	case GESTURE_MOVING_RIGHT:
		TP_type = TP_EVENT_MOVING_RIGHT;
		break;
	case GESTURE_SINGLE_CLICK:
		TP_type = TP_EVENT_SINGLE_CLICK;
		break;
	case GESTURE_DOUBLE_CLICK:
		TP_type = TP_EVENT_DOUBLE_CLICK;
		break;
	case GESTURE_LONG_PRESS:
		TP_type = TP_EVENT_LONG_PRESS;
		break;
	}

	x_pos = (0x0f&tp_temp[2])<<8 | tp_temp[3];
	y_pos = (0x0f&tp_temp[4])<<8 | tp_temp[5];
	
	if(TP_type != TP_EVENT_MAX)
	{
		touch_panel_event_handle(TP_type, x_pos, y_pos);
	}
}

void tp_get_id(uint8_t *chip_id, uint8_t *fw_ver)
{
	platform_read(TP_REG_CHIPID, chip_id, 1);
	platform_read(TP_REG_FW_VER, fw_ver, 1);

#ifdef TP_DEBUG
	LOGD("chip_id:0x%x, fw_ver:0x%x", *chip_id, *fw_ver);
#endif
}

void tp_set_auto_sleep(void)
{
	uint8_t data[2] = {0};
	
	switch(tp_chip_id)
	{
	case TP_CST816S:
		data[0] = 0;
		break;
		
	case TP_CST816T:
		data[0] = 0;
		break;
	}

	platform_write(TP_REG_DIS_AUTO_SLEEP, &data[0], 1);
}

void tp_init(void)
{
	uint8_t tmpbuf[128] = {0};
	uint8_t tp_temp_id=0;
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_UP;
	
  	//端口初始化
  	gpio_ctp = DEVICE_DT_GET(TP_PORT);
	if(!gpio_ctp)
	{
	#ifdef TP_DEBUG
		LOGD("Cannot bind gpio device");
	#endif
		return;
	}

	init_i2c();

	gpio_pin_configure(gpio_ctp, TP_EINT, flag);
	gpio_pin_interrupt_configure(gpio_ctp, TP_EINT, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb, CaptouchInterruptHandle, BIT(TP_EINT));
	gpio_add_callback(gpio_ctp, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_ctp, TP_EINT, GPIO_INT_EDGE_FALLING);

	tp_reset();

	tp_get_id(&tp_chip_id, &tp_fw_ver);
	switch(tp_chip_id)
	{
	case TP_CST816S:
		if(tp_fw_ver < 0x02)
		{
			ctp_hynitron_update();
		}
		break;

	case TP_CST816T:
	#ifdef YKL_CST816T	
		if(tp_fw_ver < 0x02)
	#elif defined(ORC_CST816T)
		if(tp_fw_ver < 0x05)
	#endif
		{
			ctp_hynitron_update();
		}
		break;
		
	}
	tp_set_auto_sleep();
}


void test_tp(void)
{
	uint16_t w,h;
	uint8_t tmpbuf[128] = {0};

#ifdef FONTMAKER_UNICODE_FONT
	mmi_asc_to_ucs2(tmpbuf, "TP_TEST");
	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(tmpbuf, &w, &h);
	LCD_ShowUniString((LCD_WIDTH-w)/2,20,tmpbuf);
#else
	sprintf(tmpbuf, "TP_TEST");
	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureString(tmpbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2,20,tmpbuf);
#endif	
	//tp_init();
}
#endif

void tp_show_infor(void)
{
	uint16_t w,h;
	uint8_t tmpbuf[128] = {0};
	
	switch(tp_msg.evt_id)
	{
	case TP_EVENT_NONE:
		strcpy(tmpbuf, "NONE        ");
		break;
	case TP_EVENT_MOVING_UP:
		strcpy(tmpbuf, "MOVING_UP   ");
		break;
	case TP_EVENT_MOVING_DOWN:
		strcpy(tmpbuf, "MOVING_DOWN ");
		break;
	case TP_EVENT_MOVING_LEFT:
		strcpy(tmpbuf, "MOVING_LEFT ");
		break;
	case TP_EVENT_MOVING_RIGHT:
		strcpy(tmpbuf, "MOVING_RIGHT");
		break;
	case TP_EVENT_SINGLE_CLICK:
		strcpy(tmpbuf, "SINGLE_CLICK");
		break;
	case TP_EVENT_DOUBLE_CLICK:
		strcpy(tmpbuf, "DOUBLE_CLICK");
		break;
	case TP_EVENT_LONG_PRESS:
		strcpy(tmpbuf, "LONG_PRESS  ");
		break;
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureString(tmpbuf, &w, &h);
	LCD_Fill(0, 80, LCD_WIDTH, h, BLACK);
	LCD_ShowString((LCD_WIDTH-w)/2,80,tmpbuf);	
	
	sprintf(tmpbuf, "x:%03d, y:%03d", tp_msg.x_pos, tp_msg.y_pos);
	LCD_MeasureString(tmpbuf, &w, &h);
	LCD_ShowString((LCD_WIDTH-w)/2,110,tmpbuf);
}

void TPMsgProcess(void)
{
#if defined(TP_DEBUG)||defined(TP_TEST)
	if(tp_int_flag)
	{
		tp_int_flag = false;
		tp_interrupt_proc();
	}
#endif
	if(tp_trige_flag)
	{
		tp_trige_flag = false;
		check_touch_event_handle(tp_msg.evt_id, tp_msg.x_pos, tp_msg.y_pos);
	}
	
	if(tp_redraw_flag)
	{
		tp_redraw_flag = false;
		tp_show_infor();
	}
}
#endif
