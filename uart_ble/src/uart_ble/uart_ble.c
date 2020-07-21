/*
* Copyright (c) 2019 Nordic Semiconductor ASA
*
* SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
*/

#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include <drivers/uart.h>


#define UART_DEV	"UART_1"
#define BUF_MAXSIZE	1024

#define PACKET_HEAD	0xAB
#define PACKET_END	0x88

#define HEART_RATE_ID			0xFF31			//心率
#define BLOOD_OXYGEN_ID			0xFF32			//血氧
#define BLOOD_PRESSURE_ID		0xFF33			//血压
#define	ONE_KEY_MEASURE_ID		0xFF34			//一键测量
#define	PULL_REFRESH_ID			0xFF35			//下拉刷新
#define	SLEEP_DETAILS_ID		0xFF36			//睡眠详情
#define	FIND_DEVICE_ID			0xFF37			//查找手环
#define SMART_NOTIFY_ID			0xFF38			//智能提醒
#define	ALARM_SETTING_ID		0xFF39			//闹钟设置
#define USER_INFOR_ID			0xFF40			//用户信息
#define	SEDENTARY_ID			0xFF41			//久坐提醒
#define	SHAKE_SCREEN_ID			0xFF42			//抬手亮屏
#define	MEASURE_HOURLY_ID		0xFF43			//整点测量设置
#define	SHAKE_PHOTO_ID			0xFF44			//摇一摇拍照
#define	LANGUAGE_SETTING_ID		0xFF45			//中英日文切换
#define	TIME_24_SETTING_ID		0xFF46			//12/24小时设置
#define	FIND_PHONE_ID			0xFF47			//查找手机回复
#define	WEATHER_INFOR_ID		0xFF48			//天气信息下发
#define	TIME_SYNC_ID			0xFF49			//时间同步
#define	TARGET_STEPS_ID			0xFF50			//目标步数
#define	BATTERY_LEVEL_ID		0xFF51			//电池电量
#define	FIRMWARE_INFOR_ID		0xFF52			//固件版本号
#define	FACTORY_RESET_ID		0xFF53			//清除手环数据
#define	ECG_ID					0xFF54			//心电
#define	LOCATION_ID				0xFF55			//获取定位信息
#define	BLE_CONNECT_ID			0xFF60			//BLE断连提醒

#define BLE_CONNECTED			0x55			//BLE已经连接
#define BLE_DSICONNECTED		0xAA			//BLE已经断开

static u32_t rece_len=0;

static u8_t rx_buf[BUF_MAXSIZE]={0};
static u8_t tx_buf[BUF_MAXSIZE]={0};

static struct device *uart_dev;

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

struct uart_data_t {
	void  *fifo_reserved;
	u8_t    data[BUF_MAXSIZE];
	u16_t   len;
};

bool BLE_is_connected = false;

void ble_connect_or_disconnect_handle(u8_t *buf, u32_t len)
{
	printk("BLE status:%x\n", buf[6]);
	
	if(buf[6] == 0x01)				//查看control值
		BLE_is_connected = true;
	else if(buf[6] == 0x00)
		BLE_is_connected = false;
	else
		BLE_is_connected = false;
}

/**********************************************************************************
*Name: ble_receive_date_handle
*Function:  处理蓝牙接收到的数据
*Parameter: 
*			Input:
*				buf 接收到的数据
*				len 接收到的数据长度
*			Output:
*				none
*			Return:
*				void
*Description:
*	接收到的数据包的格式如下:
*	包头			数据长度		操作		状态类型	控制		数据1	数据…		校验		包尾
*	(StarFrame)		(Data length)	(ID)		(Status)	(Control)	(Data1)	(Data…)	(CRC8)		(EndFrame)
*	(1 bytes)		(2 byte)		(2 byte)	(1 byte)	(1 byte)	(可选)	(可选)		(1 bytes)	(1 bytes)
*
*	例子如下表所示：
*	Offset	Field		Size	Value(十六进制)		Description
*	0		StarFrame	1		0xAB				起始帧
*	1		Data length	2		0x0000-0xFFFF		数据长度
*	3		Data ID		2		0x0000-0xFFFF	    ID
*	5		Status		1		0x00-0xFF	        Status
*	6		Control		1		0x00-0x01			控制
*	7		Data0		1-14	0x00-0xFF			数据0
*	8+n		CRC8		1		0x00-0xFF			数据校验
*	9+n		EndFrame	1		0x88				结束帧
**********************************************************************************/
void ble_receive_date_handle(u8_t *buf, u32_t len)
{
	u8_t CRC_data,data_status;
	u16_t data_len,data_ID;
	u32_t i;
	
	if((buf[0] != PACKET_HEAD) || (buf[len-1] != PACKET_END))	//format is error
	{
		printk("format is error! HEAD:%x, END:%x\n", buf[0], buf[len-1]);
		return;
	}

	for(i=0;i<len-2;i++)
		CRC_data = CRC_data+buf[i];

	if(CRC_data != buf[len-2])									//crc is error
	{
		printk("CRC is error! data:%x, CRC:%x\n", buf[len-2], CRC_data);
		return;
	}

	data_len = buf[1]*256+buf[2];
	data_ID = buf[3]*256+buf[4];

	switch(data_ID)
	{
	case HEART_RATE_ID:			//心率
		break;
	case BLOOD_OXYGEN_ID:		//血氧
		break;
	case BLOOD_PRESSURE_ID:		//血压
		break;
	case ONE_KEY_MEASURE_ID:	//一键测量
		break;
	case PULL_REFRESH_ID:		//下拉刷新
		break;
	case SLEEP_DETAILS_ID:		//睡眠详情
		break;
	case FIND_DEVICE_ID:		//查找手环
		break;
	case SMART_NOTIFY_ID:		//智能提醒
		break;
	case ALARM_SETTING_ID:		//闹钟设置
		break;
	case USER_INFOR_ID:			//用户信息
		break;
	case SEDENTARY_ID:			//久坐提醒
		break;
	case SHAKE_SCREEN_ID:		//抬手亮屏
		break;
	case MEASURE_HOURLY_ID:		//整点测量设置
		break;
	case SHAKE_PHOTO_ID:		//摇一摇拍照
		break;
	case LANGUAGE_SETTING_ID:	//中英日文切换
		break;
	case TIME_24_SETTING_ID:	//12/24小时设置
		break;
	case FIND_PHONE_ID:			//查找手机回复
		break;
	case WEATHER_INFOR_ID:		//天气信息下发
		break;
	case TIME_SYNC_ID:			//时间同步
		break;
	case TARGET_STEPS_ID:		//目标步数
		break;
	case BATTERY_LEVEL_ID:		//电池电量
		break;
	case FIRMWARE_INFOR_ID:		//固件版本号
		break;
	case FACTORY_RESET_ID:		//清除手环数据
		break;
	case ECG_ID:				//心电
		break;
	case LOCATION_ID:			//获取定位信息
		break;
	case BLE_CONNECT_ID:		//BLE断连提醒
		ble_connect_or_disconnect_handle(buf, len);
		break;
	}
}

void ble_send_date_handle(u8_t *buf, u32_t len)
{
	uart_fifo_fill(uart_dev, buf, len);
	uart_irq_tx_enable(uart_dev); 
}

static void uart_receive_data(u8_t data, u32_t datalen)
{
	//printk("rece_len:%d, rx_data:%x\n", rece_len, data);

	rx_buf[rece_len++] = data;
	if(data == 0x88)	//receivive complete
	{
		ble_receive_date_handle(rx_buf, rece_len);

		memset(rx_buf, 0, sizeof(rx_buf));
		rece_len = 0;
	}
	else				//continue receive
	{
	}
}

void uart_send_data(void)
{
	uart_fifo_fill(uart_dev, "Hello World", strlen("Hello World"));
	uart_irq_tx_enable(uart_dev); 
}

static void uart_cb(struct device *x)
{
	u8_t tmpbyte = 0;
	u32_t len=0;

	uart_irq_update(x);

	if(uart_irq_rx_ready(x)) 
	{
		while((len = uart_fifo_read(x, &tmpbyte, 1)) > 0)
			uart_receive_data(tmpbyte, 1);
	}
	
	if(uart_irq_tx_ready(x))
	{
		struct uart_data_t *buf;
		u16_t written = 0;

		buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		/* Nothing in the FIFO, nothing to send */
		if(!buf)
		{
			uart_irq_tx_disable(x);
			return;
		}

		while(buf->len > written)
		{
			written += uart_fifo_fill(x, &buf->data[written], buf->len - written);
		}

		while (!uart_irq_tx_complete(x))
		{
			/* Wait for the last byte to get
			* shifted out of the module
			*/
		}

		if (k_fifo_is_empty(&fifo_uart_tx_data))
		{
			uart_irq_tx_disable(x);
		}

		k_free(buf);
	}
}

static void uart_init(void)
{
	uart_dev = device_get_binding(UART_DEV);
	if(!uart_dev)
	{
		//LCD_ShowString(0,20,"UART初始化失败!");
	}
	else
	{
		//LCD_ShowString(0,20,"UART初始化成功!");

		uart_irq_callback_set(uart_dev, uart_cb);
		uart_irq_rx_enable(uart_dev);
	}	
}

void test_uart_ble(void)
{
	printk("test_uart_ble\n");
	
	//LCD_ShowString(0,0,"UART开始测试");

	uart_init();

	while(1)
	{
		ble_send_date_handle("Hello World", strlen("Hello World"));
		k_sleep(K_MSEC(1000));
	}
}

