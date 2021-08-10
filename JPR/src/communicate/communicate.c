/****************************************Copyright (c)************************************************
** File Name:			    communicate.c
** Descriptions:			communicate source file
** Created By:				xie biao
** Created Date:			2021-04-28
** Modified Date:      		2021-04-28 
** Version:			    	V1.0
******************************************************************************************************/
#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "settings.h"
#include "nb.h"
#include "gps.h"
#ifdef CONFIG_WIFI
#include "esp8266.h"
#endif
#include "datetime.h"
#include "communicate.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(communicate, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_WIFI
/*****************************************************************************
 * FUNCTION
 *  location_get_wifi_data_reply
 * DESCRIPTION
 *  定位协议包获取WiFi数据之后的上传数据包处理
 * PARAMETERS
 *  wifi_data       [IN]       wifi数据结构体
 * RETURNS
 *  Nothing
 *****************************************************************************/
void location_get_wifi_data_reply(wifi_infor wifi_data)
{
	u8_t reply[256] = {0};
	u32_t i;

	if(wifi_data.count > 0)
	{
		strcat(reply, "3,");
		for(i=0;i<wifi_data.count;i++)
		{
			strcat(reply, wifi_data.node[i].mac);
			strcat(reply, "&");
			strcat(reply, wifi_data.node[i].rssi);
			strcat(reply, "&");
			if(i < (wifi_data.count-1))
				strcat(reply, "|");
		}

		NBSendLocationData(reply, strlen(reply));
	}
}
#endif

/*****************************************************************************
 * FUNCTION
 *  location_get_gps_data_reply
 * DESCRIPTION
 *  定位协议包获取GPS数据之后的上传数据包处理
 * PARAMETERS
 *	flag			[IN]		GPS数据获取标记, ture:成功 false:失败
 *  gps_data       	[IN]		GPS数据结构体
 * RETURNS
 *  Nothing
 *****************************************************************************/
void location_get_gps_data_reply(bool flag, struct gps_pvt gps_data)
{
	u8_t reply[128] = {0};
	u8_t tmpbuf[8] = {0};
	u32_t tmp1;
	double tmp2;

	if(!flag)
	{
	#ifdef CONFIG_WIFI
		location_wait_wifi = true;
		APP_Ask_wifi_data();
	#endif
		return;
	}
	
	strcpy(reply, "4,");
	
	//latitude
	if(gps_data.latitude < 0)
	{
		strcat(reply, "-");
		gps_data.latitude = -gps_data.latitude;
	}

	tmp1 = (u32_t)(gps_data.latitude);	//经度整数部分
	tmp2 = gps_data.latitude - tmp1;	//经度小数部分
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (u32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/10000));
	strcat(reply, tmpbuf);
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/100));
	strcat(reply, tmpbuf);
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1));
	strcat(reply, tmpbuf);

	//semicolon
	strcat(reply, "|");
	
	//longitude
	if(gps_data.longitude < 0)
	{
		strcat(reply, "-");
		gps_data.longitude = -gps_data.longitude;
	}

	tmp1 = (u32_t)(gps_data.longitude);	//经度整数部分
	tmp2 = gps_data.longitude - tmp1;	//经度小数部分
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (u32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/10000));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1/100));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (u8_t)(tmp1));
	strcat(reply, tmpbuf);

	NBSendLocationData(reply, strlen(reply));
}

/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendHealthData
 * DESCRIPTION
 *  定时检测并上传健康数据包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void TimeCheckSendHealthData(void)
{
	u16_t steps,calorie,distance,light_sleep,deep_sleep;
	u8_t tmpbuf[20] = {0};
	u8_t databuf[128] = {0};
	static u32_t health_hour_count = 0;

	health_hour_count++;
	if(health_hour_count == global_settings.health_interval)
	{
		health_hour_count = 0;

		GetSportData(&steps, &calorie, &distance);
		GetSleepTimeData(&deep_sleep, &light_sleep);
		
		//steps
		sprintf(tmpbuf, "%d,", steps);
		strcpy(databuf, tmpbuf);
		
		//wrist
		if(is_wearing())
			strcat(databuf, "1,");
		else
			strcat(databuf, "0,");
		
		//sitdown time
		strcat(databuf, "0,");
		
		//activity time
		strcat(databuf, "0,");
		
		//light sleep time
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf, "%d,", light_sleep);
		strcat(databuf, tmpbuf);
		
		//deep sleep time
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf, "%d,", deep_sleep);
		strcat(databuf, tmpbuf);
		
		//move body
		strcat(databuf, "0,");
		
		//calorie
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf, "%d,", calorie);
		strcat(databuf, tmpbuf);
		
		//distance
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf, "%d,", distance);
		strcat(databuf, tmpbuf);
		
		//systolic
		strcat(databuf, "0,");
		
		//diastolic
		strcat(databuf, "0,");
		
		//heart rate
		strcat(databuf, "0,");
		
		//SPO2
		strcat(databuf, "0");
		
		NBSendHealthData(databuf, strlen(databuf));
	}
}

/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendLocationData
 * DESCRIPTION
 *  定时检测并上传定位数据包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void TimeCheckSendLocationData(void)
{
	static u32_t loc_hour_count = 0;
	
	loc_hour_count++;
	if(loc_hour_count == global_settings.dot_interval.time)
	{
		loc_hour_count = 0;
		location_wait_gps = true;
		APP_Ask_GPS_Data();
	}
}

/*****************************************************************************
 * FUNCTION
 *  StepCheckSendLocationData
 * DESCRIPTION
 *  计步检测并上传定位数据包
 * PARAMETERS
 *	steps			[IN]		当前累计的记步数
 * RETURNS
 *  Nothing
 *****************************************************************************/
void StepCheckSendLocationData(u16_t steps)
{
	static u32_t step_count = 0;
	
	if((steps - step_count) >= global_settings.dot_interval.steps)
	{
		step_count = steps;
		location_wait_gps = true;
		APP_Ask_GPS_Data();		
	}
}

/*****************************************************************************
 * FUNCTION
 *  SendPowerOnData
 * DESCRIPTION
 *  发送开机数据包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendPowerOnData(void)
{
	u8_t tmpbuf[10] = {0};
	u8_t databuf[128] = {0};
	
	//imsi
	strcpy(databuf, g_imsi);
	strcat(databuf, ",");
	
	//iccid
	strcat(databuf, g_iccid);
	strcat(databuf, ",");

	//nb rsrp
	sprintf(tmpbuf, "%d,", g_rsrp);
	strcat(databuf, tmpbuf);
	
	//time zone
	strcat(databuf, g_timezone);
	strcat(databuf, ",");
	
	//battery
	GetBatterySocString(tmpbuf);
	strcat(databuf, tmpbuf);
			
	NBSendPowerOnInfor(databuf, strlen(databuf));
}

/*****************************************************************************
 * FUNCTION
 *  SendPowerOffData
 * DESCRIPTION
 *  发送关机数据包
 * PARAMETERS
 *	pwroff_mode			[IN]		关机模式 1:低电关机 2:按键关机 3:重启关机 
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendPowerOffData(u8_t pwroff_mode)
{
	u8_t tmpbuf[10] = {0};
	u8_t databuf[128] = {0};
	
	//pwr off mode
	sprintf(databuf, "%d,", pwroff_mode);
	
	//nb rsrp
	sprintf(tmpbuf, "%d,", g_rsrp);
	strcat(databuf, tmpbuf);
	
	//battery
	GetBatterySocString(tmpbuf);
	strcat(databuf, tmpbuf);
			
	NBSendPowerOffInfor(databuf, strlen(databuf));
}

