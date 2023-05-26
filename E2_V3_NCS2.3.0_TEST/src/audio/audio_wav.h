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

//wav文件的数据结构，其中数据头共44bytes
/********************************************************************************************************************
起始地址    占用空间                本地址数字的含义
00H	    	4byte	    			RIFF，资源交换文件标志。
04H	    	4byte	    			从下一个地址开始到文件尾的总字节数。高位字节在后面，这里就是001437ECH，
									换成十进制是1325036byte，算上这之前的8byte就正好1325044byte了。
08H	    	4byte	    			WAVE，代表wav文件格式。
---------------------------------------------------------------------------------------------------------------------
0CH	    	4byte	    			FMT，波形格式标志
10H	    	4byte	    			FMT块长度。
14H	    	2byte	    			为1时表示线性PCM编码，大于1时表示有压缩的编码。这里是0001H。
16H	    	2byte	    			1为单声道，2为双声道，这里是0001H。
18H	    	4byte	    			采样频率，这里是00002B11H，也就是11025Hz。
1CH	    	4byte	    			Byte率=采样频率*音频通道数*每次采样得到的样本位数/8，00005622H，
									也就是22050Byte/s=11025*1*16/2。
20H	    	2byte	    			块对齐=通道数*每次采样得到的样本位数/8，0002H，也就是2=1*16/8。
22H	    	2byte	    			样本数据位数，0010H即16，一个量化样本占2byte。
---------------------------------------------------------------------------------------------------------------------
24H			4byte					fact，数据标志(该字段可选)
28H			4byte					fact块长度
2CH			不定					fact块信息
---------------------------------------------------------------------------------------------------------------------
24H			4byte					LIST，数据标志(该字段可选)
28H			4byte					list块长度
2CH			不定					list块信息
---------------------------------------------------------------------------------------------------------------------
24H	    	4byte	    			data，数据标志。
28H	    	4byte	    			Wav文件实际音频数据所占的大小，这里是001437C8H即1325000，再加上2CH就正好
									是1325044，整个文件的大小。
2CH	    	不定	    			实际音频数据
********************************************************************************************************************/

#define WAV_RIFF_ID		"RIFF"
#define WAV_WAVE_ID		"WAVE"
#define WAV_FMT_ID		"fmt "
#define WAV_FACT_ID		"fact"
#define WAV_LIST_ID		"LIST"
#define WAV_DATA_ID		"data"

typedef struct
{
	u8_t riff_mark[4];	//riff id "RIFF"
	u32_t wav_size;		//wav_size+8=file_size
	u8_t wav_str[4];	//wave文件格式 "WAVE"
}wav_riff_chunk;

typedef struct
{
	u8_t fmt_mark[4];			//fmt id "fmt "
	u32_t fmt_size;				//fmt块长度
	u16_t pcm_encode;			//编码格式（WAVE_FORMAT_PCM格式一般用的是这个）
	u16_t sound_channel;		//声道数 1:单声道 2:双声道。
	u32_t pcm_sample_freq;		//采样频率 
	u32_t byte_freq;			//码率 = 采样频率*音频通道数*每次采样得到的样本位数/8
	u16_t block_alin;			//块对齐 = 通道数*每次采样得到的样本位数/8
	u16_t sample_bits;			//样本的数据位数
}wav_fmt_chunk;

//可选项
typedef struct
{
	u8_t fact_mark[4];			//fact id "fact"
	u32_t fact_size;			//fact块长度
	u32_t fact_data;			//fact块数据
}wav_fact_chunk;

//一般在格式转换后出现
typedef struct
{
	u8_t list_mark[4];			//list id "LIST"
	u32_t list_size;			//list块长度
	u8_t *list_info;			//list块数据指针
}wav_list_chunk;

typedef struct
{
	u8_t data_mark[4];			//data id "data"
	u32_t data_size;			//data块长度
	u8_t *data_pcm;				//data块数据指针
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


