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
#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <dk_buttons_and_leds.h>
#include "lcd.h"
#include "CST816.h"
#include "CST816_update.h"
#include "logger.h"

//#define TP_DEBUG

bool tp_trige_flag = false;
bool tp_redraw_flag = false;

tp_message tp_msg = {0};

struct device *i2c_ctp;
struct device *gpio_ctp;

static struct gpio_callback gpio_cb;

static TPInfo tp_event_info = {0};
static TpEventNode *tp_event_tail = NULL;

static u8_t init_i2c(void)
{
	i2c_ctp = device_get_binding(CTP_DEV);
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

static s32_t platform_write(u8_t reg, u8_t *bufp, u16_t len)
{
	u8_t data[len+1];
	u32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(i2c_ctp, data, len+1, CST816_I2C_ADDRESS);
	return rslt;
}

static s32_t platform_read(u8_t reg, u8_t *bufp, u16_t len)
{
	u32_t rslt = 0;

	rslt = i2c_write(i2c_ctp, &reg, 1, CST816_I2C_ADDRESS);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ctp, bufp, len, CST816_I2C_ADDRESS);
	}
	return rslt;
}

static s32_t platform_write_word(u16_t reg, u8_t *bufp, u16_t len)
{
	u8_t data[len+2];
	u32_t rslt = 0;

#if 1
	data[0] = reg&0xFF;
	data[1] = reg>>8;
#else
	data[0] = reg>>8;
	data[1] = reg&0xFF;
#endif

	memcpy(&data[2], bufp, len);
	rslt = i2c_write(i2c_ctp, data, len+2, CST816_I2C_ADDRESS);
	return rslt;
}

static s32_t platform_read_word(u16_t reg, u8_t *bufp, u16_t len)
{
	u8_t data[2];
	u32_t rslt = 0;

#if 1
	data[0] = reg&0xFF;
	data[1] = reg>>8;
#else
	data[0] = reg>>8;
	data[1] = reg&0xFF;
#endif

	rslt = i2c_write(i2c_ctp, data, 2, CST816_I2C_ADDRESS);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_ctp, bufp, len, CST816_I2C_ADDRESS);
	}

	return rslt;
}

static int cst816s_enter_bootmode(void)
{
	u8_t retryCnt = 50;
	u8_t cmd[3] = {0};

	gpio_pin_write(gpio_ctp, CTP_RESET, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_write(gpio_ctp, CTP_RESET, 1);
	k_sleep(K_MSEC(10));

	while(retryCnt--)
	{
		cmd[0] = 0xAB;
		platform_write_word(0xA001,cmd,1);
		k_sleep(K_MSEC(10));
		platform_read_word(0xA003,cmd,1);
		
		if(cmd[0] != 0x55)
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

static int cst816s_update(u16_t startAddr, u16_t len, const unsigned char *src)
{
	u8_t cmd[10];
	u8_t data_send[PER_LEN];
	u32_t sum_len;
	u32_t i,k_data=0,b_data=0;

	sum_len = 0;
	k_data=len/PER_LEN;
	for(i=0;i<k_data;i++)
	{
		cmd[0] = startAddr&0xFF;
		cmd[1] = startAddr>>8;
		platform_write_word(0xA014,cmd,2);
		//TP_HRS_WriteBytes_updata(0x6A<<1,0xA014,cmd,2,REG_LEN_2B);
		memcpy(data_send,src,PER_LEN);
		platform_write_word(0xA018,data_send,PER_LEN);
		//TP_HRS_WriteBytes_updata(0x6A<<1,0xA018,data_send,PER_LEN,REG_LEN_2B);
		k_sleep(K_MSEC(10));
		//nrf_delay_ms(10);
		cmd[0] = 0xEE;
		platform_write_word(0xA004,cmd,1);
		//TP_HRS_WriteBytes_updata(0x6A<<1,0xA004,cmd,1,REG_LEN_2B);
		k_sleep(K_MSEC(50));
		//nrf_delay_ms(50);

		{
			uint8_t retrycnt = 50;
			while(retrycnt--)
			{
				cmd[0] = 0;
				platform_read_word(0xA005,cmd,1);
				//TP_HRS_read_updata(0x6A<<1,0xA005,cmd,1,REG_LEN_2B);
				if(cmd[0] == 0x55)
				{
					cmd[0] = 0;
					break;
				}
				k_sleep(K_MSEC(10));
				//nrf_delay_ms(10);
			}
		}
		startAddr += PER_LEN;
		src += PER_LEN;
		sum_len += PER_LEN;

	}

	// exit program mode
	cmd[0] = 0x00;
	platform_write_word(0xA003,cmd,1);
	//TP_HRS_WriteBytes_updata(0x6A<<1,0xA003,cmd,1,REG_LEN_2B);
	return 0;
}

static u32_t cst816s_read_checksum(void)
{
	union
	{
		u32_t sum;
		u8_t buf[4];
	}checksum;
	
	u8_t cmd[3];
	char readback[4] = {0};

	if(cst816s_enter_bootmode() == -1)
	{
		return -1;
	}

	cmd[0] = 0;
	platform_write_word(0xA003,cmd,1);
	//TP_HRS_WriteBytes_updata(0x6A<<1,0xA003,cmd,1,REG_LEN_2B);
	k_sleep(K_MSEC(500));
	//nrf_delay_ms(500);

	checksum.sum = 0;
	platform_read_word(0xA008,checksum.buf,2);
	//TP_HRS_read_updata(0x6A<<1,0xA008,checksum.buf,2,REG_LEN_2B);
	//LOG(LEVEL_INFO,"checksum.sum=%x \r\n",checksum.sum);

	return checksum.sum;
}


bool ctp_hynitron_update(void)
{
	u8_t lvalue;
	u8_t write_data[2];
	u8_t tmpbuf[128]={0};
	bool temp_result = true;

	if(cst816s_enter_bootmode() == 0)
	{
		LCD_ShowString(20,120,"enter bootmode!");
		
		if(sizeof(app_bin)>10)
		{
			u16_t startAddr = app_bin[1];
			u16_t length = app_bin[3];					
			u16_t checksum = app_bin[5];
			
			startAddr <<= 8; startAddr |= app_bin[0];
			length <<= 8; length |= app_bin[2];
			checksum <<= 8; checksum |= app_bin[4];  

			LCD_ShowString(20,140,"check if need update!");
			
			if(cst816s_read_checksum()!= checksum)
			{
				LCD_ShowString(20,160,"start update!");
				
				cst816s_update(startAddr, length, &app_bin[6]);
				cst816s_read_checksum();

				LCD_ShowString(20,180,"complete update!");
			}
		}
		return true;
	}
	return false;
}

void clear_all_touch_event_handle(void)
{
	TpEventNode *pnext;
	
	if(tp_event_info.cache == NULL || tp_event_info.count == 0)
	{
	#ifdef TP_DEBUG
		LOGD("001");
	#endif
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

	#ifdef TP_DEBUG
		LOGD("002");
	#endif
	}
}

void unregister_touch_event_handle(TP_EVENT tp_type, u16_t x_start, u16_t x_stop, u16_t y_start, u16_t y_stop, tp_handler_t touch_handler)
{
	TpEventNode *ppre,*pnext;
	
	if(tp_event_info.cache == NULL || tp_event_info.count == 0)
	{
	#ifdef TP_DEBUG
		LOGD("001");
	#endif
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
				#ifdef TP_DEBUG
					LOGD("002");
				#endif
					tp_event_info.cache = pnext->next;
					tp_event_info.count--;
					k_free(pnext);
				}
				else if(pnext == tp_event_tail)
				{
				#ifdef TP_DEBUG
					LOGD("003");
				#endif
					tp_event_tail = ppre;
					tp_event_tail->next = NULL;
					tp_event_info.count--;
					k_free(pnext);
				}
				else
				{
				#ifdef TP_DEBUG
					LOGD("004");
				#endif
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
	#ifdef TP_DEBUG
		LOGD("005");
	#endif
	}
}

void register_touch_event_handle(TP_EVENT tp_type, u16_t x_start, u16_t x_stop, u16_t y_start, u16_t y_stop, tp_handler_t touch_handler)
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

bool check_touch_event_handle(TP_EVENT tp_type, u16_t x_pos, u16_t y_pos)
{
	TpEventNode *pnew;
	
	if(tp_event_info.cache == NULL || tp_event_info.count == 0)
	{
	#ifdef TP_DEBUG
		LOGD("001");
	#endif
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
				#ifdef TP_DEBUG
					LOGD("002");
				#endif
					return true;
				}
				else if((x_pos >= pnew->x_begin)
						&&(x_pos <= pnew->x_end)
						&&(y_pos >= pnew->y_begin)
						&&(y_pos <= pnew->y_end))
				{
					if(pnew->func != NULL)
						pnew->func();
				#ifdef TP_DEBUG
					LOGD("003");
				#endif
					return true;
				}
				else
				{
				#ifdef TP_DEBUG
					LOGD("004");
				#endif
					pnew = pnew->next;
				}
			}
			else
			{
			#ifdef TP_DEBUG
				LOGD("005");
			#endif
				pnew = pnew->next;
			}
				
		}while(pnew != NULL);
	#ifdef TP_DEBUG
		LOGD("006");
	#endif
		return false;
	}
}

void touch_panel_event_handle(TP_EVENT tp_type, u16_t x_pos, u16_t y_pos)
{
	u8_t tmpbuf[128] = {0};
	u8_t strbuf[128] = {0};
	u16_t x,y,w,h;

	switch(tp_type)
	{
	case TP_EVENT_NONE:
	#ifdef TP_DEBUG
		LOGD("tp none!");
	#endif
		sprintf(strbuf, "GESTURE_NONE");
		break;
	case TP_EVENT_MOVING_UP:
	#ifdef TP_DEBUG
		LOGD("tp moving up! x:%d, y:%d", x_pos,y_pos);
	#endif
		sprintf(strbuf, "MOVING_UP   ");
		break;
	case TP_EVENT_MOVING_DOWN:
	#ifdef TP_DEBUG
		LOGD("tp moving down! x:%d, y:%d", x_pos,y_pos);
	#endif
		sprintf(strbuf, "MOVING_DOWN ");
		break;
	case TP_EVENT_MOVING_LEFT:
	#ifdef TP_DEBUG
		LOGD("tp moving left! x:%d, y:%d", x_pos,y_pos);
	#endif
		sprintf(strbuf, "MOVING_LEFT ");
		break;
	case TP_EVENT_MOVING_RIGHT:
	#ifdef TP_DEBUG
		LOGD("tp moving right! x:%d, y:%d", x_pos,y_pos);
	#endif
		sprintf(strbuf, "MOVING_RIGHT");
		break;
	case TP_EVENT_SINGLE_CLICK:
	#ifdef TP_DEBUG
		LOGD("tp single click! x:%d, y:%d", x_pos,y_pos);
	#endif
		sprintf(strbuf, "SINGLE_CLICK");
		break;
	case TP_EVENT_DOUBLE_CLICK:
	#ifdef TP_DEBUG
		LOGD("tp double click! x:%d, y:%d", x_pos,y_pos);
	#endif
		sprintf(strbuf, "DOUBLE_CLICK");
		break;
	case TP_EVENT_LONG_PRESS:
	#ifdef TP_DEBUG
		LOGD("tp long press! x:%d, y:%d", x_pos,y_pos);
	#endif
		sprintf(strbuf, "LONG_PRESS  ");
		break;
	case TP_EVENT_MAX:
		break;
	}

	check_touch_event_handle(tp_type, x_pos, y_pos);
}

void CaptouchInterruptHandle(void)
{
	tp_trige_flag = true;
}

void tp_interrupt_proc(void)
{
	u8_t TP_type = TP_EVENT_MAX;
	u8_t tp_temp[10]={0};
	u16_t x_pos,y_pos;
	
	platform_read(CST816_REG_GESTURE, &tp_temp[0], 1);//手势
	platform_read(CST816_REG_FINGER_NUM, &tp_temp[1], 1);//手指个数
	platform_read(CST816_REG_XPOS_H, &tp_temp[2], 1);//x坐标高位 (&0x0f,取低4位)
	platform_read(CST816_REG_XPOS_L, &tp_temp[3], 1);//x坐标低位
	platform_read(CST816_REG_YPOS_H, &tp_temp[4], 1);//y坐标低位 (&0x0f,取低4位)
	platform_read(CST816_REG_YPOS_L, &tp_temp[5], 1);//y坐标低位

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

void CST816_init(void)
{
	u8_t tmpbuf[128] = {0};
	u8_t tp_temp_id=0;
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_LOW|GPIO_INT_DEBOUNCE;
	
  	//端口初始化
  	gpio_ctp = device_get_binding(CTP_PORT);
	if(!gpio_ctp)
	{
	#ifdef TP_DEBUG
		LOGD("Cannot bind gpio device");
	#endif
		return;
	}

	init_i2c();

	gpio_pin_configure(gpio_ctp, CTP_EINT, flag);
	gpio_pin_disable_callback(gpio_ctp, CTP_EINT);
	gpio_init_callback(&gpio_cb, CaptouchInterruptHandle, BIT(CTP_EINT));
	gpio_add_callback(gpio_ctp, &gpio_cb);
	gpio_pin_enable_callback(gpio_ctp, CTP_EINT);

	gpio_pin_configure(gpio_ctp, CTP_RESET, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ctp, CTP_RESET, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_write(gpio_ctp, CTP_RESET, 1);
	k_sleep(K_MSEC(50));

	platform_read(CST816_REG_CHIPID, &tp_temp_id, 1);
	if(tp_temp_id == CST816_CHIP_ID)
	{
	#ifdef TP_DEBUG
		LOGD("It's CST816!");
	#endif
	}

	sprintf(tmpbuf, "CTP ID:%x", tp_temp_id);
	LCD_ShowString(20,100,tmpbuf);

	//ctp_hynitron_update();
}


void test_tp(void)
{
	u8_t tmpbuf[128] = {0};

	sprintf(tmpbuf, "test_tp");
	LCD_ShowString(20,80,tmpbuf);
	//CST816_init();
}

void tp_show_infor(void)
{
	u8_t tmpbuf[128] = {0};
	
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
	
	LCD_ShowString(20,120,tmpbuf);
	
	sprintf(tmpbuf, "x:%5d, y:%5d", tp_msg.x_pos, tp_msg.y_pos);
	LCD_ShowString(20,140,tmpbuf);
}

void TPMsgProcess(void)
{
	if(tp_trige_flag)
	{
		tp_trige_flag = false;
		tp_interrupt_proc();
	}

	if(tp_redraw_flag)
	{
		tp_redraw_flag = false;
		tp_show_infor();
	}
}
#endif
