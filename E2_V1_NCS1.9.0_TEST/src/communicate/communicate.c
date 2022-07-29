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
//#include "gps.h"
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
 *  ��λЭ�����ȡWiFi����֮����ϴ����ݰ�����
 * PARAMETERS
 *  wifi_data       [IN]       wifi���ݽṹ��
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
 *  ��λЭ�����ȡGPS����֮����ϴ����ݰ�����
 * PARAMETERS
 *	flag			[IN]		GPS���ݻ�ȡ��� ture:�ɹ� false:ʧ��
 *  gps_data       	[IN]		GPS���ݽṹ��
 * RETURNS
 *  Nothing
 *****************************************************************************/
/*void location_get_gps_data_reply(bool flag, struct gps_pvt gps_data)
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

	tmp1 = (uint32_t)(gps_data.latitude);	//������������
	tmp2 = gps_data.latitude - tmp1;	//����С������
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

	tmp1 = (uint32_t)(gps_data.longitude);	//������������
	tmp2 = gps_data.longitude - tmp1;	//����С������
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
}*/

/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendHealthData
 * DESCRIPTION
 *  ��ʱ��Ⲣ�ϴ��������ݰ�
 * PARAMETERS
 *	Nothing
 * RETURNS
 *  Nothing
 *****************************************************************************/
void TimeCheckSendHealthData(void)
{
	uint16_t steps=0,calorie=0,distance=0;
	uint16_t light_sleep=0,deep_sleep=0;
	uint8_t tmpbuf[20] = {0};
	uint8_t databuf[128] = {0};

#ifdef CONFIG_IMU_SUPPORT
	GetSportData(&steps, &calorie, &distance);
	GetSleepTimeData(&deep_sleep, &light_sleep);
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
	sprintf(tmpbuf, "%d,", g_bp_systolic);
	strcat(databuf, tmpbuf);
	
	//diastolic
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_bp_diastolic); 	
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

	NBSendHealthData(databuf, strlen(databuf));
}

/*****************************************************************************
 * FUNCTION
 *  TimeCheckSendLocationData
 * DESCRIPTION
 *  ��ʱ��Ⲣ�ϴ���λ���ݰ�
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
		//location_wait_gps = true;
		//APP_Ask_GPS_Data();
	#endif
	}
}

/*****************************************************************************
 * FUNCTION
 *  StepCheckSendLocationData
 * DESCRIPTION
 *  �Ʋ���Ⲣ�ϴ���λ���ݰ�
 * PARAMETERS
 *	steps			[IN]		��ǰ�ۼƵļǲ���
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
		//location_wait_gps = true;
		//APP_Ask_GPS_Data();
	#endif		
	}
}

/*****************************************************************************
 * FUNCTION
 *  SyncSendHealthData
 * DESCRIPTION
 *  ����ͬ���ϴ��������ݰ�
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
	GetSportData(&steps, &calorie, &distance);
	GetSleepTimeData(&deep_sleep, &light_sleep);
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
	sprintf(tmpbuf, "%d,", g_bp_systolic);
	strcat(databuf, tmpbuf);
	
	//diastolic
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf, "%d,", g_bp_diastolic); 	
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

	NBSendHealthData(databuf, strlen(databuf));
}

/*****************************************************************************
 * FUNCTION
 *  SyncSendLocalData
 * DESCRIPTION
 *  ����ͬ���ϴ���λ���ݰ�
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
	//location_wait_gps = true;
	//APP_Ask_GPS_Data();
#endif

}

/*****************************************************************************
 * FUNCTION
 *  SendPowerOnData
 * DESCRIPTION
 *  ���Ϳ������ݰ�
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
			
	NBSendPowerOnInfor(databuf, strlen(databuf));
}

/*****************************************************************************
 * FUNCTION
 *  SendPowerOffData
 * DESCRIPTION
 *  ���͹ػ����ݰ�
 * PARAMETERS
 *	pwroff_mode			[IN]		�ػ�ģʽ 1:�͵�ػ�2:�����ػ� 3:�����ػ� 
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
 *  ����SOS������(�޵�ַ��Ϣ)
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

