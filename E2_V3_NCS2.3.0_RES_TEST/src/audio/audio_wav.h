/****************************************Copyright (c)************************************************
** File Name:			    audio_wav.h
** Descriptions:			Wave format audio file processing header file
** Created By:				xie biao
** Created Date:			2021-03-04
** Modified Date:      		2021-03-04 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __AUDIO_WAV_H__
#define __AUDIO_WAV_H__
	
#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

//wav�ļ������ݽṹ����������ͷ��44bytes
/********************************************************************************************************************
��ʼ��ַ    ռ�ÿռ�                ����ַ���ֵĺ���
00H	    	4byte	    			RIFF����Դ�����ļ���־��
04H	    	4byte	    			����һ����ַ��ʼ���ļ�β�����ֽ�������λ�ֽ��ں��棬�������001437ECH��
									����ʮ������1325036byte��������֮ǰ��8byte������1325044byte�ˡ�
08H	    	4byte	    			WAVE������wav�ļ���ʽ��
---------------------------------------------------------------------------------------------------------------------
0CH	    	4byte	    			FMT�����θ�ʽ��־
10H	    	4byte	    			FMT�鳤�ȡ�
14H	    	2byte	    			Ϊ1ʱ��ʾ����PCM���룬����1ʱ��ʾ��ѹ���ı��롣������0001H��
16H	    	2byte	    			1Ϊ��������2Ϊ˫������������0001H��
18H	    	4byte	    			����Ƶ�ʣ�������00002B11H��Ҳ����11025Hz��
1CH	    	4byte	    			Byte��=����Ƶ��*��Ƶͨ����*ÿ�β����õ�������λ��/8��00005622H��
									Ҳ����22050Byte/s=11025*1*16/2��
20H	    	2byte	    			�����=ͨ����*ÿ�β����õ�������λ��/8��0002H��Ҳ����2=1*16/8��
22H	    	2byte	    			��������λ����0010H��16��һ����������ռ2byte��
---------------------------------------------------------------------------------------------------------------------
24H			4byte					fact�����ݱ�־(���ֶο�ѡ)
28H			4byte					fact�鳤��
2CH			����					fact����Ϣ
---------------------------------------------------------------------------------------------------------------------
24H			4byte					LIST�����ݱ�־(���ֶο�ѡ)
28H			4byte					list�鳤��
2CH			����					list����Ϣ
---------------------------------------------------------------------------------------------------------------------
24H	    	4byte	    			data�����ݱ�־��
28H	    	4byte	    			Wav�ļ�ʵ����Ƶ������ռ�Ĵ�С��������001437C8H��1325000���ټ���2CH������
									��1325044�������ļ��Ĵ�С��
2CH	    	����	    			ʵ����Ƶ����
********************************************************************************************************************/

#define WAV_RIFF_ID		"RIFF"
#define WAV_WAVE_ID		"WAVE"
#define WAV_FMT_ID		"fmt "
#define WAV_FACT_ID		"fact"
#define WAV_LIST_ID		"LIST"
#define WAV_DATA_ID		"data"

typedef struct
{
	uint8_t riff_mark[4];	//riff id "RIFF"
	uint32_t wav_size;		//wav_size+8=file_size
	uint8_t wav_str[4];	//wave�ļ���ʽ "WAVE"
}wav_riff_chunk;

typedef struct
{
	uint8_t fmt_mark[4];			//fmt id "fmt "
	uint32_t fmt_size;				//fmt�鳤��
	uint16_t pcm_encode;			//�����ʽ��WAVE_FORMAT_PCM��ʽһ���õ��������
	uint16_t sound_channel;		//������ 1:������ 2:˫������
	uint32_t pcm_sample_freq;		//����Ƶ�� 
	uint32_t byte_freq;			//���� = ����Ƶ��*��Ƶͨ����*ÿ�β����õ�������λ��/8
	uint16_t block_alin;			//����� = ͨ����*ÿ�β����õ�������λ��/8
	uint16_t sample_bits;			//����������λ��
}wav_fmt_chunk;

//��ѡ��
typedef struct
{
	uint8_t fact_mark[4];			//fact id "fact"
	uint32_t fact_size;			//fact�鳤��
	uint32_t fact_data;			//fact������
}wav_fact_chunk;

//һ���ڸ�ʽת�������
typedef struct
{
	uint8_t list_mark[4];			//list id "LIST"
	uint32_t list_size;			//list�鳤��
	uint8_t *list_info;			//list������ָ��
}wav_list_chunk;

typedef struct
{
	uint8_t data_mark[4];			//data id "data"
	uint32_t data_size;			//data�鳤��
	uint8_t *data_pcm;				//data������ָ��
}wav_data_chunk;

typedef struct
{
	wav_riff_chunk riff;
	wav_fmt_chunk format;
	wav_fact_chunk fact;
	wav_data_chunk data;
} wav_head;



//extern char fall_alarm_1[69168];
//extern char fall_alarm_2[69168];
//extern char fall_alarm_3[69168];
//extern char fall_alarm_4[69108];

	
#endif/*__AUDIO_WAV_H__*/


