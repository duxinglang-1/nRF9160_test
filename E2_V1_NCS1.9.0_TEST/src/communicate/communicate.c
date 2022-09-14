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
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif/*CONFIG_PPG_SUPPORT*/
#ifdef CONFIG_TEMP_SUPPORT
#include "temp.h"
#endif/*CONFIG_TEMP_SUPPORT*/
#include "communicate.h"
#include "logger.h"

extern uint16_t g_last_steps;

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
	uint8_t reply[256] = {0};
	uint32_t i,count=3;

	if(wifi_data.count > 0)
		count = wifi_data.count;
		
	strcat(reply, "3,");
	for(i=0;i<count;i++)
	{
		strcat(reply, wifi_data.node[i].mac);
		strcat(reply, "&");
		strcat(reply, wifi_data.node[i].rssi);
		strcat(reply, "&");
		if(i < (count-1))
			strcat(reply, "|");
	}

	NBSendLocationData(reply, strlen(reply));
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
void location_get_gps_data_reply(bool flag, struct nrf_modem_gnss_pvt_data_frame gps_data)
{
	uint8_t reply[128] = {0};
	uint8_t tmpbuf[8] = {0};
	uint32_t tmp1;
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

	tmp1 = (uint32_t)(gps_data.latitude);	//经度整数部分
	tmp2 = gps_data.latitude - tmp1;	//经度小数部分
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (uint32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/10000));
	strcat(reply, tmpbuf);
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/100));
	strcat(reply, tmpbuf);
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1));
	strcat(reply, tmpbuf);

	//semicolon
	strcat(reply, "|");
	
	//longitude
	if(gps_data.longitude < 0)
	{
		strcat(reply, "-");
		gps_data.longitude = -gps_data.longitude;
	}

	tmp1 = (uint32_t)(gps_data.longitude);	//经度整数部分
	tmp2 = gps_data.longitude - tmp1;	//经度小数部分
	//integer
	sprintf(tmpbuf, "%d", tmp1);
	strcat(reply, tmpbuf);
	//dot
	strcat(reply, ".");
	//decimal
	tmp1 = (uint32_t)(tmp2*1000000);
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/10000));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%10000;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1/100));
	strcat(reply, tmpbuf);	
	tmp1 = tmp1%100;
	sprintf(tmpbuf, "%02d", (uint8_t)(tmp1));
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
	uint16_t light_sleep=0,deep_sleep=0;
	uint8_t i,tmpbuf[20] = {0};
	uint8_t databuf[1024] = {0};
	uint8_t hr_data[24] = {0};
	uint8_t spo2_data[24] = {0};
#ifdef CONFIG_PPG_SUPPORT	
	bpt_data bp_data[24] = {0};
#endif
	uint16_t temp_data[24] = {0};
	uint16_t step_data[24] = {0};
	
	//wrist
	if(is_wearing())
		strcat(databuf, "1,");
	else
		strcat(databuf, "0,");
	
	//sitdown time
	strcat(databuf, "0,");
	
	//activity time
	strcat(databuf, "0,");

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_SLEEP_SUPPORT)
	GetSleepTimeData(&deep_sleep, &light_sleep);
#endif
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

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
	GetCurDayStepRecData(step_data);
#endif
	for(i=0;i<24;i++)
	{
		uint16_t calorie,distance;
		
		memset(tmpbuf,0,sizeof(tmpbuf));
		
		if(step_data[i] == 0xffff)
			step_data[i] = 0;

		distance = 0.7*step_data[i];
		calorie = (0.8214*60*distance)/1000;
		sprintf(tmpbuf, "%d&%d&%d", step_data[i], distance, calorie);

		if(i<23)
			strcat(tmpbuf,"|");
		else
			strcat(tmpbuf,",");
		
		strcat(databuf, tmpbuf);
	}

#ifdef CONFIG_PPG_SUPPORT
	//hr
	GetCurDayHrRecData(hr_data);
	//spo2
	GetCurDaySpo2RecData(spo2_data);
	//bpt
	GetCurDayBptRecData(bp_data);
#endif/*CONFIG_PPG_SUPPORT*/
	//hr
	for(i=0;i<24;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

		if(hr_data[i] == 0xff)
			hr_data[i] = 0;		
		sprintf(tmpbuf, "%d", hr_data[i]);

		if(i<23)
			strcat(tmpbuf,"|");
		else
			strcat(tmpbuf,",");
		strcat(databuf, tmpbuf);
	}
	//spo2
	for(i=0;i<24;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

		if(spo2_data[i] == 0xff)
			spo2_data[i] = 0;	
		sprintf(tmpbuf, "%d", spo2_data[i]);

		if(i<23)
			strcat(tmpbuf,"|");
		else
			strcat(tmpbuf,",");
		strcat(databuf, tmpbuf);
	}
	//bpt
	for(i=0;i<24;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

	#ifdef CONFIG_PPG_SUPPORT
		if(bp_data[i].systolic == 0xff)
			bp_data[i].systolic = 0;
		if(bp_data[i].diastolic == 0xff)
			bp_data[i].diastolic = 0;
		sprintf(tmpbuf, "%d&%d", bp_data[i].systolic,bp_data[i].diastolic);
	#else
		strcpy(tmpbuf, "0&0");
	#endif
	
		if(i<23)
			strcat(tmpbuf,"|");
		else
			strcat(tmpbuf,",");
		strcat(databuf, tmpbuf);
	}
	
#ifdef CONFIG_TEMP_SUPPORT
	//body temp
	GetCurDayTempRecData(temp_data);
#endif/*CONFIG_TEMP_SUPPORT*/
	for(i=0;i<24;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

		if(temp_data[i] == 0xffff)
			temp_data[i] = 0;
		sprintf(tmpbuf, "%0.1f", (float)temp_data[i]/10.0);

		if(i<23)
			strcat(tmpbuf,"|");
		strcat(databuf, tmpbuf);
	}

	NBSendTimelyHealthData(databuf, strlen(databuf));
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
	static uint32_t loc_hour_count = 0;
	bool flag = false;

	loc_hour_count++;
	if(date_time.hour >= 21 || date_time.hour < 9)
	{
		if(loc_hour_count == 360)
		{
			flag = true;
		}
	}
	else if(loc_hour_count == global_settings.dot_interval.time)
	{
		flag = true;
	}

	if(flag)
	{
		loc_hour_count = 0;
	#ifdef CONFIG_WIFI
		location_wait_wifi = true;
		APP_Ask_wifi_data();
	#else
		location_wait_gps = true;
		APP_Ask_GPS_Data();
	#endif
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
void StepCheckSendLocationData(uint16_t steps)
{
	static uint16_t step_count = 0;

	if(step_count == 0)
		step_count = g_last_steps;
	
	if((steps - step_count) >= global_settings.dot_interval.steps)
	{
		step_count = steps;

	#ifdef CONFIG_WIFI
		location_wait_wifi = true;
		APP_Ask_wifi_data();
	#else
		location_wait_gps = true;
		APP_Ask_GPS_Data();
	#endif		
	}
}

/*****************************************************************************
 * FUNCTION
 *  SyncSendHealthData
 * DESCRIPTION
 *  数据同步上传健康数据包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SyncSendHealthData(void)
{
	uint16_t steps=0,calorie=0,distance=0;
	uint16_t light_sleep=0,deep_sleep=0;
	uint8_t tmpbuf[20] = {0};
	uint8_t databuf[128] = {0};

#ifdef CONFIG_IMU_SUPPORT
  #ifdef CONFIG_STEP_SUPPORT
	GetSportData(&steps, &calorie, &distance);
  #endif
  #ifdef CONFIG_SLEEP_SUPPORT
	GetSleepTimeData(&deep_sleep, &light_sleep);
  #endif
#endif

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

#ifdef CONFIG_PPG_SUPPORT	
	//systolic
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_bpt.systolic);
	strcat(databuf, tmpbuf);
	
	//diastolic
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_bpt.diastolic); 	
	strcat(databuf, tmpbuf);
	
	//heart rate
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_hr);		
	strcat(databuf, tmpbuf);
	
	//SPO2
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_spo2); 	
	strcat(databuf, tmpbuf);
#else
	strcat(databuf, "0,0,0,0,");
#endif/*CONFIG_PPG_SUPPORT*/

#ifdef CONFIG_TEMP_SUPPORT
	//body temp
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%0.1f", g_temp_body); 	
	strcat(databuf, tmpbuf);
#else
	strcat(databuf, "0.0");
#endif/*CONFIG_TEMP_SUPPORT*/

	NBSendSingleHealthData(databuf, strlen(databuf));
}

/*****************************************************************************
 * FUNCTION
 *  SyncSendLocalData
 * DESCRIPTION
 *  数据同步上传定位数据包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SyncSendLocalData(void)
{
#ifdef CONFIG_WIFI
	location_wait_wifi = true;
	APP_Ask_wifi_data();
#else
	location_wait_gps = true;
	APP_Ask_GPS_Data();
#endif

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
	uint8_t tmpbuf[10] = {0};
	uint8_t databuf[128] = {0};
	
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
	strcat(databuf, ",");

	//mcu fw version
	strcat(databuf, g_fw_version);	

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
void SendPowerOffData(uint8_t pwroff_mode)
{
	uint8_t tmpbuf[10] = {0};
	uint8_t databuf[128] = {0};
	
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


/*****************************************************************************
 * FUNCTION
 *  SendSosAlarmData
 * DESCRIPTION
 *  发送SOS报警包(无地址信息)
 * PARAMETERS
 *	
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendSosAlarmData(void)
{
	uint8_t reply[256] = {0};
	uint32_t i,count=1;

	strcat(reply, "3,");
	for(i=0;i<count;i++)
	{
		strcat(reply, "");
		strcat(reply, "&");
		strcat(reply, "");
		strcat(reply, "&");
		if(i < (count-1))
			strcat(reply, "|");
	}

	NBSendSosWifiData(reply, strlen(reply));
}

