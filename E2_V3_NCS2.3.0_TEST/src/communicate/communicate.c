/****************************************Copyright (c)************************************************
** File Name:			    communicate.c
** Descriptions:			communicate source file
** Created By:				xie biao
** Created Date:			2021-04-28
** Modified Date:      		2021-04-28 
** Version:			    	V1.0
******************************************************************************************************/
#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "settings.h"
#include "nb.h"
#include "gps.h"
#include "external_flash.h"
#ifdef CONFIG_WIFI_SUPPORT
#include "esp8266.h"
#endif
#include "datetime.h"
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif
#ifdef CONFIG_TEMP_SUPPORT
#include "temp.h"
#endif
#ifdef CONFIG_IMU_SUPPORT
#include "Lsm6dso.h"
#endif
#include "communicate.h"
#include "logger.h"

extern uint16_t g_last_steps;

#ifdef CONFIG_WIFI_SUPPORT
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
	#ifdef CONFIG_WIFI_SUPPORT
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

#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendSportData
 * DESCRIPTION
 *  定时检测并上传运动数据包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void TimeCheckSendSportData(void)
{
	uint8_t i,tmpbuf[20] = {0};
	uint8_t reply[1024] = {0};
	uint16_t step_data[24] = {0};
	sleep_data sleep[24] = {0};

	//wrist
	if(is_wearing())
		strcpy(reply, "1,");
	else
		strcpy(reply, "0,");
	
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
		
		strcat(reply, tmpbuf);
	}

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_SLEEP_SUPPORT)
	GetCurDaySleepRecData((uint8_t*)&sleep);
#endif
	for(i=0;i<24;i++)
	{
		uint16_t total_sleep;
		
		memset(tmpbuf,0,sizeof(tmpbuf));
		
		if(sleep[i].deep == 0xffff)
			sleep[i].deep = 0;
		if(sleep[i].light == 0xffff)
			sleep[i].light = 0;

		total_sleep = sleep[i].deep+sleep[i].light;
		sprintf(tmpbuf, "%d&%d&%d", total_sleep, sleep[i].light, sleep[i].deep);

		if(i<23)
			strcat(tmpbuf,"|");
		strcat(reply, tmpbuf);
	}

	NBSendTimelySportData(reply, strlen(reply));
}
#endif

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
	uint8_t i,tmpbuf[20] = {0};
	uint8_t reply[1024] = {0};
	uint8_t hr_data[24] = {0};
	uint8_t spo2_data[24] = {0};
#ifdef CONFIG_PPG_SUPPORT	
	bpt_data bp_data[24] = {0};
#endif
	uint16_t temp_data[24] = {0};

	//wrist
	if(is_wearing())
		strcpy(reply, "1,");
	else
		strcpy(reply, "0,");
	
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
		strcat(reply, tmpbuf);
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
		strcat(reply, tmpbuf);
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
		strcat(reply, tmpbuf);
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
		strcat(reply, tmpbuf);
	}

	NBSendTimelyHealthData(reply, strlen(reply));
}

#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
/*****************************************************************************
 * FUNCTION
 *  SendMissingSportData
 * DESCRIPTION
 *  补发漏传的运动数据包(不补发当天的数据，防止固定的23点的时间戳造成当天数据混乱)
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendMissingSportData(void)
{
	uint8_t i,j,tmpbuf[20] = {0};
	uint8_t stepbuf[STEP_REC2_DATA_SIZE] = {0};
	uint8_t sleepbuf[SLEEP_REC2_DATA_SIZE] = {0};	

#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
	SpiFlash_Read(stepbuf, STEP_REC2_DATA_ADDR, STEP_REC2_DATA_SIZE);
#endif
#if defined(CONFIG_IMU_SUPPORT)&&defined(CONFIG_SLEEP_SUPPORT)
	SpiFlash_Read(sleepbuf, SLEEP_REC2_DATA_ADDR, SLEEP_REC2_DATA_SIZE);
#endif

	for(i=0;i<7;i++)
	{
		bool flag = false;
		uint8_t reply[1024] = {0};
		uint8_t timemap[16] = {0};
		uint16_t step_data[24] = {0};
		sleep_data sleep[24] = {0};
		step_rec2_data step_rec2 = {0};
		sleep_rec2_data	sleep_rec2 = {0};

	#if defined(CONFIG_IMU_SUPPORT)&&(defined(CONFIG_STEP_SUPPORT)||defined(CONFIG_SLEEP_SUPPORT))
	  #if defined(CONFIG_STEP_SUPPORT)&&defined(CONFIG_STEP_SUPPORT)
	  	//step
		memcpy(&step_rec2, &stepbuf[i*sizeof(step_rec2_data)], sizeof(step_rec2_data));
		if((step_rec2.year != 0xffff && step_rec2.year != 0x0000)
			&&(step_rec2.month != 0xff && step_rec2.month != 0x00)
			&&(step_rec2.day != 0xff && step_rec2.day != 0x00)
			)
		{
			flag = true;
			memcpy(step_data, step_rec2.steps, sizeof(step_rec2.steps));
			sprintf(timemap, "%04d%02d%02d230000", step_rec2.year,step_rec2.month,step_rec2.day);
		}
	  #endif
	  #if defined(CONFIG_SLEEP_SUPPORT)&&defined(CONFIG_SLEEP_SUPPORT)
		//sleep
		memcpy(&sleep_rec2, &sleepbuf[i*sizeof(sleep_rec2_data)], sizeof(sleep_rec2_data));
	  	if((sleep_rec2.year != 0xffff && sleep_rec2.year != 0x0000)
			&&(sleep_rec2.month != 0xff && sleep_rec2.month != 0x00)
			&&(sleep_rec2.day != 0xff && sleep_rec2.day != 0x00)
			)
		{
			flag = true;
			memcpy(sleep, sleep_rec2.sleep, sizeof(sleep_rec2.sleep));
			if(strlen(timemap) == 0)
				sprintf(timemap, "%04d%02d%02d230000", sleep_rec2.year,sleep_rec2.month,sleep_rec2.day);
		}
	  #endif
	#endif
	
		if(!flag)
			continue;
		
		//step
		for(j=0;j<24;j++)
		{
			uint16_t calorie,distance;
			
			memset(tmpbuf,0,sizeof(tmpbuf));
			
			if(step_data[j] == 0xffff)
				step_data[j] = 0;

			distance = 0.7*step_data[j];
			calorie = (0.8214*60*distance)/1000;
			sprintf(tmpbuf, "%d&%d&%d", step_data[j], distance, calorie);

			if(j<23)
				strcat(tmpbuf,"|");
			else
				strcat(tmpbuf,",");
			
			strcat(reply, tmpbuf);
		}
		//sleep
		for(j=0;j<24;j++)
		{
			uint16_t total_sleep;
			
			memset(tmpbuf,0,sizeof(tmpbuf));
			
			if(sleep[j].deep == 0xffff)
				sleep[j].deep = 0;
			if(sleep[j].light == 0xffff)
				sleep[j].light = 0;

			total_sleep = sleep[j].deep+sleep[j].light;
			sprintf(tmpbuf, "%d&%d&%d", total_sleep, sleep[j].light, sleep[j].deep);

			if(j<23)
				strcat(tmpbuf,"|");
			strcat(reply, tmpbuf);
		}

		NBSendMissSportData(reply, strlen(reply), timemap);
	}
}
#endif

#if defined(CONFIG_PPG_SUPPORT)||defined(CONFIG_TEMP_SUPPORT)
/*****************************************************************************
 * FUNCTION
 *  SendMissingHealthData
 * DESCRIPTION
 *  补发漏传的健康数据包(不补发当天的数据，防止固定的23点的时间戳造成当天数据混乱)
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendMissingHealthData(void)
{
	uint8_t i,j,tmpbuf[20] = {0};
	uint8_t hrbuf[PPG_HR_REC2_DATA_SIZE] = {0};
	uint8_t spo2buf[PPG_SPO2_REC2_DATA_SIZE] = {0};
	uint8_t bptbuf[PPG_BPT_REC2_DATA_SIZE] = {0};
	uint8_t tempbuf[TEMP_REC2_DATA_SIZE] = {0};	

#ifdef CONFIG_PPG_SUPPORT
	SpiFlash_Read(hrbuf, PPG_HR_REC2_DATA_ADDR, PPG_HR_REC2_DATA_SIZE);
	SpiFlash_Read(spo2buf, PPG_SPO2_REC2_DATA_ADDR, PPG_SPO2_REC2_DATA_SIZE);
	SpiFlash_Read(bptbuf, PPG_BPT_REC2_DATA_ADDR, PPG_BPT_REC2_DATA_SIZE);
#endif
#ifdef CONFIG_TEMP_SUPPORT
	SpiFlash_Read(tempbuf, TEMP_REC2_DATA_ADDR, TEMP_REC2_DATA_SIZE);
#endif

	for(i=0;i<7;i++)
	{
		bool flag = false;
		uint8_t reply[1024] = {0};
		uint8_t timemap[16] = {0};
		uint8_t hr_data[24] = {0};
		uint8_t spo2_data[24] = {0};
	#ifdef CONFIG_PPG_SUPPORT	
		bpt_data bp_data[24] = {0};
	#endif
		uint16_t temp_data[24] = {0};
	#ifdef CONFIG_PPG_SUPPORT
		ppg_hr_rec2_data hr_rec2 = {0};
		ppg_spo2_rec2_data spo2_rec2 = {0};
		ppg_bpt_rec2_data bpt_rec2 = {0};
	#endif	
	#ifdef CONFIG_TEMP_SUPPORT	
		temp_rec2_data temp_rec2 = {0};
	#endif
	
	#if defined(CONFIG_PPG_SUPPORT)||defined(CONFIG_TEMP_SUPPORT)
	  #ifdef CONFIG_PPG_SUPPORT
		//hr
		memcpy(&hr_rec2, &hrbuf[i*sizeof(ppg_hr_rec2_data)], sizeof(ppg_hr_rec2_data));
	  	if((hr_rec2.year != 0xffff && hr_rec2.year != 0x0000)
			&&(hr_rec2.month != 0xff && hr_rec2.month != 0x00)
			&&(hr_rec2.day != 0xff && hr_rec2.day != 0x00)
			)
		{
			flag = true;
			memcpy(hr_data, hr_rec2.hr, sizeof(hr_rec2.hr));
			sprintf(timemap, "%04d%02d%02d230000", hr_rec2.year,hr_rec2.month,hr_rec2.day);
		}
		
		//spo2
		memcpy(&spo2_rec2, &spo2buf[i*sizeof(ppg_spo2_rec2_data)], sizeof(ppg_spo2_rec2_data));
		if((spo2_rec2.year != 0xffff && spo2_rec2.year != 0x0000)
			&&(spo2_rec2.month != 0xff && spo2_rec2.month != 0x00)
			&&(spo2_rec2.day != 0xff && spo2_rec2.day != 0x00)
			)
		{
			flag = true;
			memcpy(spo2_data, spo2_rec2.spo2, sizeof(spo2_rec2.spo2));
			if(strlen(timemap) == 0)
				sprintf(timemap, "%04d%02d%02d230000", spo2_rec2.year,spo2_rec2.month,spo2_rec2.day);
		}
		
		//bpt
		memcpy(&bpt_rec2, &bptbuf[i*sizeof(ppg_bpt_rec2_data)], sizeof(ppg_bpt_rec2_data));
		if((bpt_rec2.year != 0xffff && bpt_rec2.year != 0x0000)
			&&(bpt_rec2.month != 0xff && bpt_rec2.month != 0x00)
			&&(bpt_rec2.day != 0xff && bpt_rec2.day != 0x00)
			)
		{
			flag = true;
			memcpy(bp_data, bpt_rec2.bpt, sizeof(bpt_rec2.bpt));
			if(strlen(timemap) == 0)
				sprintf(timemap, "%04d%02d%02d230000", bpt_rec2.year,bpt_rec2.month,bpt_rec2.day);
		}
	  #endif
	  
	  #ifdef CONFIG_TEMP_SUPPORT
		//body temp
		memcpy(&temp_rec2, &tempbuf[i*sizeof(temp_rec2_data)], sizeof(temp_rec2_data));
	  	if((temp_rec2.year != 0xffff && temp_rec2.year != 0x0000)
			&&(temp_rec2.month != 0xff && temp_rec2.month != 0x00)
			&&(temp_rec2.day != 0xff || temp_rec2.day != 0x00)
			)
		{
			flag = true;
			memcpy(temp_data, temp_rec2.deca_temp, sizeof(temp_rec2.deca_temp));
			if(strlen(timemap) == 0)
				sprintf(timemap, "%04d%02d%02d230000", temp_rec2.year,temp_rec2.month,temp_rec2.day);
		}
	  #endif
	#endif

		if(!flag)
			continue;
		
		//hr
		for(j=0;j<24;j++)
		{
			memset(tmpbuf,0,sizeof(tmpbuf));

			if(hr_data[j] == 0xff)
				hr_data[j] = 0;		
			sprintf(tmpbuf, "%d", hr_data[j]);

			if(j<23)
				strcat(tmpbuf,"|");
			else
				strcat(tmpbuf,",");
			strcat(reply, tmpbuf);
		}
		//spo2
		for(j=0;j<24;j++)
		{
			memset(tmpbuf,0,sizeof(tmpbuf));

			if(spo2_data[j] == 0xff)
				spo2_data[j] = 0;	
			sprintf(tmpbuf, "%d", spo2_data[j]);

			if(j<23)
				strcat(tmpbuf,"|");
			else
				strcat(tmpbuf,",");
			strcat(reply, tmpbuf);
		}
		//bpt
		for(j=0;j<24;j++)
		{
			memset(tmpbuf,0,sizeof(tmpbuf));

		#ifdef CONFIG_PPG_SUPPORT
			if(bp_data[j].systolic == 0xff)
				bp_data[j].systolic = 0;
			if(bp_data[j].diastolic == 0xff)
				bp_data[j].diastolic = 0;
			sprintf(tmpbuf, "%d&%d", bp_data[j].systolic,bp_data[j].diastolic);
		#else
			strcpy(tmpbuf, "0&0");
		#endif
		
			if(j<23)
				strcat(tmpbuf,"|");
			else
				strcat(tmpbuf,",");
			strcat(reply, tmpbuf);
		}
		
		for(j=0;j<24;j++)
		{
			memset(tmpbuf,0,sizeof(tmpbuf));

			if(temp_data[j] == 0xffff)
				temp_data[j] = 0;
			sprintf(tmpbuf, "%0.1f", (float)temp_data[j]/10.0);

			if(j<23)
				strcat(tmpbuf,"|");
			strcat(reply, tmpbuf);
		}
		
		NBSendMissHealthData(reply, strlen(reply), timemap);
	}
}
#endif

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
	#ifdef CONFIG_WIFI_SUPPORT
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

	#ifdef CONFIG_WIFI_SUPPORT
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
	uint8_t reply[1024] = {0};

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
	strcpy(reply, tmpbuf);
	
	//wrist
	if(is_wearing())
		strcat(reply, "1,");
	else
		strcat(reply, "0,");
	
	//sitdown time
	strcat(reply, "0,");
	
	//activity time
	strcat(reply, "0,");
	
	//light sleep time
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", light_sleep);
	strcat(reply, tmpbuf);
	
	//deep sleep time
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", deep_sleep);
	strcat(reply, tmpbuf);
	
	//move body
	strcat(reply, "0,");
	
	//calorie
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", calorie);
	strcat(reply, tmpbuf);
	
	//distance
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", distance);
	strcat(reply, tmpbuf);

#ifdef CONFIG_PPG_SUPPORT	
	//systolic
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_bpt_menu.systolic);
	strcat(reply, tmpbuf);
	
	//diastolic
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_bpt_menu.diastolic); 	
	strcat(reply, tmpbuf);
	
	//heart rate
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_hr_menu);		
	strcat(reply, tmpbuf);
	
	//SPO2
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_spo2_menu); 	
	strcat(reply, tmpbuf);
#else
	strcat(reply, "0,0,0,0,");
#endif/*CONFIG_PPG_SUPPORT*/

#ifdef CONFIG_TEMP_SUPPORT
	//body temp
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%0.1f", g_temp_menu); 	
	strcat(reply, tmpbuf);
#else
	strcat(reply, "0.0");
#endif/*CONFIG_TEMP_SUPPORT*/

	NBSendSingleHealthData(reply, strlen(reply));
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
#ifdef CONFIG_WIFI_SUPPORT
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
	uint8_t reply[128] = {0};

	//imsi
	strcpy(reply, g_imsi);
	strcat(reply, ",");
	
	//iccid
	strcat(reply, g_iccid);
	strcat(reply, ",");

	//nb rsrp
	sprintf(tmpbuf, "%d,", g_rsrp);
	strcat(reply, tmpbuf);
	
	//time zone
	strcat(reply, g_timezone);
	strcat(reply, ",");
	
	//battery
	GetBatterySocString(tmpbuf);
	strcat(reply, tmpbuf);
	strcat(reply, ",");

	//mcu fw version
	strcat(reply, g_fw_version);	

	NBSendPowerOnInfor(reply, strlen(reply));
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
	uint8_t reply[128] = {0};

	//pwr off mode
	sprintf(reply, "%d,", pwroff_mode);
	
	//nb rsrp
	sprintf(tmpbuf, "%d,", g_rsrp);
	strcat(reply, tmpbuf);
	
	//battery
	GetBatterySocString(tmpbuf);
	strcat(reply, tmpbuf);
			
	NBSendPowerOffInfor(reply, strlen(reply));
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
	uint8_t reply[8] = {0};
	uint32_t i,count=1;

	strcpy(reply, "1");
	NBSendAlarmData(reply, strlen(reply));
}

/*****************************************************************************
 * FUNCTION
 *  SendFallAlarmData
 * DESCRIPTION
 *  发送Fall报警包(无地址信息)
 * PARAMETERS
 *	
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendFallAlarmData(void)
{
	uint8_t reply[8] = {0};
	uint32_t i,count=1;

	strcpy(reply, "2");
	NBSendAlarmData(reply, strlen(reply));
}

