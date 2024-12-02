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
#include "uart_ble.h"
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

void TimeCheckSendWristOffData(void)
{
	uint8_t reply[8] = {0};

	if(CheckSCC())
		strcpy(reply, "1");
	else
		strcpy(reply, "0");
	
	NBSendTimelyWristOffData(reply, strlen(reply));
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
	uint16_t step_data[24] = {0};
	sleep_data sleep[24] = {0};
	uint8_t reply[512] = {0};

	memset(&reply, 0x00, sizeof(reply));
	
	//wrist
	if(ppg_skin_contacted_flag)
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

#ifdef CONFIG_PPG_SUPPORT
/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendHrData
 * DESCRIPTION
 *  定时检测并上传健康数据心率包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
uint8_t hr_data[sizeof(ppg_hr_rec2_data)] = {0};
void TimeCheckSendHrData(void)
{
	uint16_t i,len;
	uint8_t tmpbuf[32] = {0};
	uint8_t reply[2048] = {0};
	hr_rec2_nod *p_hr;

	memset(&hr_data, 0x00, sizeof(hr_data));
	GetCurDayHrRecData(&hr_data);
	p_hr = (hr_rec2_nod*)hr_data;

	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

		if((p_hr->year == 0xffff || p_hr->year == 0x0000)
			||(p_hr->month == 0xff || p_hr->month == 0x00)
			||(p_hr->day == 0xff || p_hr->day == 0x00)
			||(p_hr->hour == 0xff || p_hr->min == 0xff)
			)
		{
			break;
		}
		
		sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_hr->year, p_hr->month, p_hr->day, p_hr->hour, p_hr->min);
		strcat(reply, tmpbuf);
		sprintf(tmpbuf, "%d|", p_hr->hr);
		strcat(reply, tmpbuf);

		p_hr++;
	}

	len = strlen(reply);
	if(len > 0)
		reply[len-1] = ',';
	else
		reply[len] = ',';
	NBSendTimelyHrData(reply, strlen(reply));
}

/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendSpo2Data
 * DESCRIPTION
 *  定时检测并上传健康数据血氧包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
uint8_t spo2_data[sizeof(ppg_spo2_rec2_data)] = {0};
void TimeCheckSendSpo2Data(void)
{
	uint16_t i,len;
	uint8_t tmpbuf[32] = {0};
	uint8_t reply[2048] = {0};
	spo2_rec2_nod *p_spo2;

	memset(&spo2_data, 0x00, sizeof(spo2_data));
	GetCurDaySpo2RecData(&spo2_data);
	p_spo2 = (spo2_rec2_nod*)spo2_data;

	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

		if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
			||(p_spo2->month == 0xff || p_spo2->month == 0x00)
			||(p_spo2->day == 0xff || p_spo2->day == 0x00)
			||(p_spo2->hour == 0xff || p_spo2->min == 0xff)
			)
		{
			break;
		}
		
		sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_spo2->year, p_spo2->month, p_spo2->day, p_spo2->hour, p_spo2->min);
		strcat(reply, tmpbuf);
		sprintf(tmpbuf, "%d|", p_spo2->spo2);
		strcat(reply, tmpbuf);

		p_spo2++;
	}

	len = strlen(reply);
	if(len > 0)
		reply[len-1] = ',';
	else
		reply[len] = ',';
	NBSendTimelySpo2Data(reply, strlen(reply));
}

/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendBptData
 * DESCRIPTION
 *  定时检测并上传健康数据血压包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
uint8_t bp_data[sizeof(ppg_bpt_rec2_data)] = {0};
void TimeCheckSendBptData(void)
{
	uint16_t i,len;
	uint8_t tmpbuf[32] = {0};
	uint8_t reply[2048] = {0};
	bpt_rec2_nod *p_bpt;

	memset(&bp_data, 0x00, sizeof(bp_data));
	GetCurDayBptRecData(&bp_data);
	p_bpt = (bpt_rec2_nod*)bp_data;

	for(i=0;i<PPG_REC2_MAX_DAILY;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

		if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
			||(p_bpt->month == 0xff || p_bpt->month == 0x00)
			||(p_bpt->day == 0xff || p_bpt->day == 0x00)
			||(p_bpt->hour == 0xff || p_bpt->min == 0xff)
			)
		{
			break;
		}
		
		sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_bpt->year, p_bpt->month, p_bpt->day, p_bpt->hour, p_bpt->min);
		strcat(reply, tmpbuf);
		sprintf(tmpbuf, "%d&%d|", p_bpt->bpt.systolic, p_bpt->bpt.diastolic);
		strcat(reply, tmpbuf);

		p_bpt++;
	}

	len = strlen(reply);
	if(len > 0)
		reply[len-1] = ',';
	else
		reply[len] = ',';
	NBSendTimelyBptData(reply, strlen(reply));
}
#endif

#ifdef CONFIG_TEMP_SUPPORT
/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendTempData
 * DESCRIPTION
 *  定时检测并上传健康数据体温包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
uint8_t temp_data[sizeof(temp_rec2_data)] = {0};
void TimeCheckSendTempData(void)
{
	uint16_t i,len;
	uint8_t tmpbuf[32] = {0};
	uint8_t reply[2048] = {0};
	temp_rec2_nod *p_temp;

	memset(&temp_data, 0x00, sizeof(temp_data));
	GetCurDayTempRecData(&temp_data);
	p_temp = (temp_rec2_nod*)temp_data;
	
	for(i=0;i<TEMP_REC2_MAX_DAILY;i++)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));

		if((p_temp->year == 0xffff || p_temp->year == 0x0000)
			||(p_temp->month == 0xff || p_temp->month == 0x00)
			||(p_temp->day == 0xff || p_temp->day == 0x00)
			||(p_temp->hour == 0xff || p_temp->min == 0xff)
			)
		{
			break;
		}
		
		sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_temp->year, p_temp->month, p_temp->day, p_temp->hour, p_temp->min);
		strcat(reply, tmpbuf);
		sprintf(tmpbuf, "%0.1f|", (float)p_temp->deca_temp/10.0);
		strcat(reply, tmpbuf);

		p_temp++;
	}

	len = strlen(reply);
	if(len > 0)
		reply[len-1] = ',';
	else
		reply[len] = ',';
	NBSendTimelyTempData(reply, strlen(reply));
}
#endif

#if defined(CONFIG_PPG_SUPPORT)||defined( CONFIG_TEMP_SUPPORT)
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
#if 1	//xb add 2024-01-17 修改定时健康数据分开上传
	if(1
	#ifdef CONFIG_PPG_SUPPORT
		&& CheckSCC()
	#endif
		)
	{
	#ifdef CONFIG_PPG_SUPPORT
		TimeCheckSendHrData();
		TimeCheckSendSpo2Data();
		TimeCheckSendBptData();
	#endif
	#ifdef CONFIG_TEMP_SUPPORT
		TimeCheckSendTempData();
	#endif
	}
	else
	{
		TimeCheckSendWristOffData();
	}
#else
	uint8_t i,tmpbuf[20] = {0};
	uint8_t hr_data[24] = {0};
	uint8_t spo2_data[24] = {0};
#ifdef CONFIG_PPG_SUPPORT	
	bpt_data bp_data[24] = {0};
#endif
	uint16_t temp_data[24] = {0};

	memset(&reply, 0x00, sizeof(reply));
	
	//wrist
	if(ppg_skin_contacted_flag)
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
#endif
}
#endif

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
		uint8_t reply[2048] = {0};
		uint16_t step_data[24] = {0};
		uint8_t step_time[15] = {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00};
		sleep_data sleep[24] = {0};
		uint8_t sleep_time[15] = {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00};
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
			sprintf(step_time, "%04d%02d%02d230000", step_rec2.year,step_rec2.month,step_rec2.day);
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
			sprintf(sleep_time, "%04d%02d%02d230000", sleep_rec2.year,sleep_rec2.month,sleep_rec2.day);
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
		strcat(reply, step_time);
		strcat(reply, ",");
		
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
			else
				strcat(tmpbuf,",");
			strcat(reply, tmpbuf);
		}
		strcat(reply, sleep_time);

		NBSendMissSportData(reply, strlen(reply));
	}
}
#endif

#if defined(CONFIG_PPG_SUPPORT)||defined(CONFIG_TEMP_SUPPORT)
static uint8_t databuf[PPG_REC2_MAX_DAILY*sizeof(bpt_rec2_nod)] = {0};
/*****************************************************************************
 * FUNCTION
 *  SendMissingHrData
 * DESCRIPTION
 *  补发漏传的健康数据心率包(不补发当天的数据，防止固定的23点的时间戳造成当天数据混乱)
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendMissingHrData(void)
{
	uint16_t i,j,len;
	uint8_t tmpbuf[32] = {0};
	hr_rec2_nod *p_hr;
	sys_date_timer_t temp_date = {0};

	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	DateDecrease(&temp_date, 6);
	
	for(i=0;i<7;i++)
	{
		bool flag = false;
		uint8_t hr_time[15] = {0x30};
		uint8_t reply[2048] = {0};

		memset(&databuf, 0x00, sizeof(databuf));
		GetGivenDayHrRecData(temp_date, &databuf);
		p_hr = (hr_rec2_nod*)databuf;
		for(j=0;j<PPG_REC2_MAX_DAILY;j++)
		{
		  	if((p_hr->year == 0xffff || p_hr->year == 0x0000)
				||(p_hr->month == 0xff || p_hr->month == 0x00)
				||(p_hr->day == 0xff || p_hr->day == 0x00)
				||(p_hr->hour == 0xff || p_hr->min == 0xff)
				)
			{
				break;
			}

			if(!flag)
			{
				flag = true;
				sprintf(hr_time, "%04d%02d%02d230000", p_hr->year, p_hr->month, p_hr->day);
			}

			memset(tmpbuf,0,sizeof(tmpbuf));
			sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_hr->year, p_hr->month, p_hr->day, p_hr->hour, p_hr->min);
			strcat(reply, tmpbuf);
			sprintf(tmpbuf, "%d|", p_hr->hr);
			strcat(reply, tmpbuf);
			
			p_hr++;
		}

		len = strlen(reply);
		if(len > 0)
		{
			reply[len-1] = ',';
			strcat(reply, hr_time);
			NBSendMissHrData(reply, strlen(reply));
		}

		DateIncrease(&temp_date, 1);
	}
}

/*****************************************************************************
 * FUNCTION
 *  SendMissingSpo2Data
 * DESCRIPTION
 *  补发漏传的健康数据血氧包(不补发当天的数据，防止固定的23点的时间戳造成当天数据混乱)
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendMissingSpo2Data(void)
{
	uint16_t i,j,len;
	uint8_t tmpbuf[32] = {0};
	spo2_rec2_nod *p_spo2;
	sys_date_timer_t temp_date = {0};

	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	DateDecrease(&temp_date, 6);
	
	for(i=0;i<7;i++)
	{
		bool flag = false;
		uint8_t spo2_time[15] = {0x30};
		uint8_t reply[2048] = {0};

		memset(&databuf, 0x00, sizeof(databuf));
		GetGivenDaySpo2RecData(temp_date, &databuf);
		p_spo2 = (spo2_rec2_nod*)databuf;
		for(j=0;j<PPG_REC2_MAX_DAILY;j++)
		{
			if((p_spo2->year == 0xffff || p_spo2->year == 0x0000)
				||(p_spo2->month == 0xff || p_spo2->month == 0x00)
				||(p_spo2->day == 0xff || p_spo2->day == 0x00)
				||(p_spo2->hour == 0xff || p_spo2->min == 0xff)
				)
			{
				break;
			}

			if(!flag)
			{
				flag = true;
				sprintf(spo2_time, "%04d%02d%02d230000", p_spo2->year, p_spo2->month, p_spo2->day);
			}

			memset(tmpbuf,0,sizeof(tmpbuf));
			sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_spo2->year, p_spo2->month, p_spo2->day, p_spo2->hour, p_spo2->min);
			strcat(reply, tmpbuf);
			sprintf(tmpbuf, "%d|", p_spo2->spo2);
			strcat(reply, tmpbuf);
			
			p_spo2++;
		}

		len = strlen(reply);
		if(len > 0)
		{
			reply[len-1] = ',';
			strcat(reply, spo2_time);
			NBSendMissSpo2Data(reply, strlen(reply));
		}

		DateIncrease(&temp_date, 1);
	}
}

/*****************************************************************************
 * FUNCTION
 *  SendMissingBptData
 * DESCRIPTION
 *  补发漏传的健康数据血压包(不补发当天的数据，防止固定的23点的时间戳造成当天数据混乱)
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendMissingBptData(void)
{
	uint16_t i,j,len;
	uint8_t tmpbuf[32] = {0};
	bpt_rec2_nod *p_bpt;
	sys_date_timer_t temp_date = {0};

	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	DateDecrease(&temp_date, 6);

	for(i=0;i<7;i++)
	{
		bool flag = false;
		uint8_t bpt_time[15] = {0x30};
		uint8_t reply[2048] = {0};

		memset(&databuf, 0x00, sizeof(databuf));
		GetGivenDayBptRecData(temp_date, &databuf);
		p_bpt = (bpt_rec2_nod*)databuf;
		for(j=0;j<PPG_REC2_MAX_DAILY;j++)
		{
			if((p_bpt->year == 0xffff || p_bpt->year == 0x0000)
				||(p_bpt->month == 0xff || p_bpt->month == 0x00)
				||(p_bpt->day == 0xff || p_bpt->day == 0x00)
				||(p_bpt->hour == 0xff || p_bpt->min == 0xff)
				)
			{
				break;
			}

			if(!flag)
			{
				flag = true;
				sprintf(bpt_time, "%04d%02d%02d230000", p_bpt->year, p_bpt->month, p_bpt->day);
			}

			memset(tmpbuf,0,sizeof(tmpbuf));
			sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_bpt->year, p_bpt->month, p_bpt->day, p_bpt->hour, p_bpt->min);
			strcat(reply, tmpbuf);
			sprintf(tmpbuf, "%d&%d|", p_bpt->bpt.systolic, p_bpt->bpt.diastolic);
			strcat(reply, tmpbuf);
			
			p_bpt++;
		}

		len = strlen(reply);
		if(len > 0)
		{
			reply[len-1] = ',';
			strcat(reply, bpt_time);
			NBSendMissBptData(reply, strlen(reply));
		}

		DateIncrease(&temp_date, 1);
	}
}


/*****************************************************************************
 * FUNCTION
 *  SendMissingTempData
 * DESCRIPTION
 *  补发漏传的健康数据体温包(不补发当天的数据，防止固定的23点的时间戳造成当天数据混乱)
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendMissingTempData(void)
{
	uint16_t i,j,len;
	uint8_t tmpbuf[32] = {0};
	temp_rec2_nod *p_temp;
	sys_date_timer_t temp_date = {0};

	memcpy(&temp_date, &date_time, sizeof(sys_date_timer_t));
	DateDecrease(&temp_date, 6);	

	for(i=0;i<7;i++)
	{
		bool flag = false;
		uint8_t temp_time[15] = {0x30};
		uint8_t reply[2048] = {0};

		memset(&databuf, 0x00, sizeof(databuf));
		GetGivenDayTempRecData(temp_date, &databuf);
		p_temp = (temp_rec2_nod*)databuf;
		for(j=0;j<PPG_REC2_MAX_DAILY;j++)
		{
			if((p_temp->year == 0xffff || p_temp->year == 0x0000)
				||(p_temp->month == 0xff || p_temp->month == 0x00)
				||(p_temp->day == 0xff || p_temp->day == 0x00)
				||(p_temp->hour == 0xff || p_temp->min == 0xff)
				)
			{
				break;
			}

			if(!flag)
			{
				flag = true;
				sprintf(temp_time, "%04d%02d%02d230000", p_temp->year, p_temp->month, p_temp->day);
			}

			memset(tmpbuf,0,sizeof(tmpbuf));
			sprintf(tmpbuf, "%04d%02d%02d%02d%02d;", p_temp->year, p_temp->month, p_temp->day, p_temp->hour, p_temp->min);
			strcat(reply, tmpbuf);
			sprintf(tmpbuf, "%0.1f|", (float)p_temp->deca_temp/10.0);
			strcat(reply, tmpbuf);
			
			p_temp++;
		}

		len = strlen(reply);
		if(len > 0)
		{
			reply[len-1] = ',';
			strcat(reply, temp_time);
			NBSendMissTempData(reply, strlen(reply));
		}

		DateIncrease(&temp_date, 1);
	}
}

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
#if 1	//xb add 2024-01-17 修改补传数据单独上传
	SendMissingHrData();
	SendMissingSpo2Data();
	SendMissingBptData();
	SendMissingTempData();
#else
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
		uint8_t hr_data[24] = {0};
		uint8_t hr_time[15] = {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00};
		uint8_t spo2_data[24] = {0};
		uint8_t spo2_time[15] = {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00};
	#ifdef CONFIG_PPG_SUPPORT
		bpt_data bp_data[24] = {0};
	#endif
		uint8_t bp_time[15] = {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00};
		uint16_t temp_data[24] = {0};
		uint8_t temp_time[15] = {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00};
	#ifdef CONFIG_PPG_SUPPORT
		ppg_hr_rec2_data hr_rec2 = {0};
		ppg_spo2_rec2_data spo2_rec2 = {0};
		ppg_bpt_rec2_data bpt_rec2 = {0};
	#endif
	#ifdef CONFIG_TEMP_SUPPORT
		temp_rec2_data temp_rec2 = {0};
	#endif

		memset(&reply, 0x00, sizeof(reply));
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
			sprintf(hr_time, "%04d%02d%02d230000", hr_rec2.year,hr_rec2.month,hr_rec2.day);
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
			sprintf(spo2_time, "%04d%02d%02d230000", spo2_rec2.year,spo2_rec2.month,spo2_rec2.day);
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
			sprintf(bp_time, "%04d%02d%02d230000", bpt_rec2.year,bpt_rec2.month,bpt_rec2.day);
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
			sprintf(temp_time, "%04d%02d%02d230000", temp_rec2.year,temp_rec2.month,temp_rec2.day);
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
		strcat(reply, hr_time);
		strcat(reply, ",");
		
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
		strcat(reply, spo2_time);
		strcat(reply, ",");
			
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
		strcat(reply, bp_time);
		strcat(reply, ",");

		//temp
		for(j=0;j<24;j++)
		{
			memset(tmpbuf,0,sizeof(tmpbuf));

			if(temp_data[j] == 0xffff)
				temp_data[j] = 0;
			sprintf(tmpbuf, "%0.1f", (float)temp_data[j]/10.0);

			if(j<23)
				strcat(tmpbuf,"|");
			else
				strcat(tmpbuf,",");
			strcat(reply, tmpbuf);
		}
		strcat(reply, temp_time);
		
		NBSendMissHealthData(reply, strlen(reply));
	}
#endif	
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
	if(1
	#ifdef CONFIG_PPG_SUPPORT	
		&& CheckSCC()
	#endif	
		)		
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
	uint8_t reply[256] = {0};

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
	strcat(reply, ",");

	//modem fw version
	strcat(reply, &g_modem[12]);	
	strcat(reply, ",");

	//ppg algo
#ifdef CONFIG_PPG_SUPPORT	
	strcat(reply, g_ppg_ver);
#endif
	strcat(reply, ",");

	//wifi version
#ifdef CONFIG_WIFI_SUPPORT
	strcat(reply, g_wifi_ver);	
#endif
	strcat(reply, ",");

	//wifi mac
#ifdef CONFIG_WIFI_SUPPORT	
	strcat(reply, g_wifi_mac_addr);	
#endif
	strcat(reply, ",");

	//ble version
	strcat(reply, &g_nrf52810_ver[15]);	
	strcat(reply, ",");

	//ble mac
	strcat(reply, g_ble_mac_addr);	

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
 *  SendSettingsData
 * DESCRIPTION
 *  发送终端设置项数据包
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendSettingsData(void)
{
	uint8_t tmpbuf[10] = {0};
	uint8_t reply[128] = {0};

	//temp uint
	sprintf(reply, "%d,", global_settings.temp_unit);
	
	//language
	switch(global_settings.language)
	{
#ifndef FW_FOR_CN
	case LANGUAGE_EN:	//English
		strcat(reply, "en");
		break;
	case LANGUAGE_DE:	//Deutsch
		strcat(reply, "de");
		break;
	case LANGUAGE_FR:	//French
		strcat(reply, "fr");
		break;
	case LANGUAGE_ITA:	//Italian
		strcat(reply, "it");
		break;
	case LANGUAGE_ES:	//Spanish
		strcat(reply, "es");
		break;
	case LANGUAGE_PT:	//Portuguese
		strcat(reply, "pt");
		break;
#else
	case LANGUAGE_CHN:	//Chinese
		strcat(reply, "zh");
		break;
	case LANGUAGE_EN:	//English
		strcat(reply, "en");
		break;
#endif	
	}
	
	NBSendSettingsData(reply, strlen(reply));
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

/*****************************************************************************
 * FUNCTION
 *  SendVerCheckAskData
 * DESCRIPTION
 *  发送FOTA版本信息请求包
 * PARAMETERS
 *	
 * RETURNS
 *  Nothing
 *****************************************************************************/
void SendVerCheckAskData(void)
{
	uint8_t reply[8] = {0};
	uint32_t i,count=1;

	strcpy(reply, "1");
	NBSendVerCheckData(reply, strlen(reply));
}

