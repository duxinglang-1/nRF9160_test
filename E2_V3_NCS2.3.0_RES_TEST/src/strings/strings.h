/****************************************Copyright (c)************************************************
** File Name:			    strings.h
** Descriptions:			strings head file
** Created By:				xie biao
** Created Date:			2025-10-10
** Modified Date:      		2025-10-10 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/types.h>

//strlib.bin���ļ�
//���ļ��ṹ���ļ�ͷ+������
//	  �ļ�ͷ���ַ�������(2 bytes) + �����ܹ�����(2 bytes) + ��1���ַ���(��1���ַ����������������λ��(4 bytes)+�ַ����ĳ���(4 bytes)+...+��m���ַ����������������λ��(4 bytes)+�ַ����ĳ���(4 bytes)) + ... +  ��n���ַ���(��1���ַ����������������λ��(4 bytes)+�ַ����ĳ���(4 bytes)+...+��m���ַ����������������λ��(4 bytes)+�ַ����ĳ���(4 bytes))
//	  �����壺��1���ַ���(��1���ַ�����+...+��m���ַ�����) + ... + ��n���ַ���(��1���ַ�����+...+��m���ַ�����)
//��������Ϊ��Chinese��English��German��French��Italian��Spanish��Portuguese��Polish��Swedish��Japanese��Korea��Russian��Arabic

typedef struct
{
	uint32_t addr;
	uint32_t len;
}dataindex;

typedef struct
{
	uint16_t str_count;
	uint16_t lang_count;
	uint32_t index_len;
	dataindex *p_index;
}strlib_infor_t;

#endif/*__STRINGS_H__*/