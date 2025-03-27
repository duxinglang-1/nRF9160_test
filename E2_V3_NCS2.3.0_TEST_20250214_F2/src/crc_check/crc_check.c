/****************************************Copyright (c)************************************************
** File Name:			    crc_check.c
** Descriptions:			data crc check process source file
** Created By:				xie biao
** Created Date:			2021-12-27
** Modified Date:      		2021-12-27 
** Version:			    	V1.0
******************************************************************************************************/
#include <nrf9160.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "crc_check.h"
#include "logger.h"

//#define CRC_DEBUG

CRC_8 crc_8 = {0x07,0x00,0x00,false,false};
CRC_8 crc_8_ITU = {0x07,0x00,0x55,false,false};
CRC_8 crc_8_ROHC = {0x07,0xff,0x00,true,true};
CRC_8 crc_8_MAXIM = {0x31,0x00,0x00,true,true};

CRC_16 crc_16_IBM = {0x8005,0x0000,0x0000,true,true};
CRC_16 crc_16_MAXIM = {0x8005,0x0000,0xffff,true,true};
CRC_16 crc_16_USB = {0x8005,0xffff,0xffff,true,true};
CRC_16 crc_16_MODBUS = {0x8005,0xffff,0x0000,true,true};
CRC_16 crc_16_CCITT = {0x1021,0x0000,0x0000,true,true};
CRC_16 crc_16_CCITT_FALSE = {0x1021,0xffff,0x0000,false,false};
CRC_16 crc_16_X5 = {0x1021,0xffff,0xffff,true,true};
CRC_16 crc_16_XMODEM = {0x1021,0x0000,0x0000,false,false};
CRC_16 crc_16_DNP = {0x3d65,0x0000,0xffff,true,true};

CRC_32 crc_32 = {0x04c11db7,0xffffffff,0xffffffff,true,true};
CRC_32 crc_32_MPEG2 = {0x04c11db7,0xffffffff,0x00000000,false,false};

uint8_t crc8_cal(uint8_t *addr, int num, CRC_8 type);
uint16_t crc16_cal(uint8_t *addr, int num, CRC_16 type);
uint32_t crc32_cal(uint8_t *addr, int num, CRC_32 type);

/*****************************************************************************
*function name:reverse8
*function: �ֽڷ�ת����1100 0101 ��ת��Ϊ1010 0011
*input��1�ֽ�
*output:��ת���ֽ�
******************************************************************************/
uint8_t reverse8(uint8_t data)
{
    uint8_t i;
    uint8_t temp=0;
	
    for(i=0;i<8;i++)	//�ֽڷ�ת
        temp |= ((data>>i) & 0x01)<<(7-i);
    return temp;
}

/*****************************************************************************
*function name:reverse16
*function: ˫�ֽڷ�ת����1100 0101 1110 0101��ת��Ϊ1010 0111 1010 0011
*input��˫�ֽ�
*output:��ת��˫�ֽ�
******************************************************************************/
uint16_t reverse16(uint16_t data)
{
    uint8_t i;
    uint16_t temp=0;
	
    for(i=0;i<16;i++)		//��ת
        temp |= ((data>>i) & 0x0001)<<(15-i);
    return temp;
}

/*****************************************************************************
*function name:reverse32
*function: 32bit�ַ�ת
*input��32bit��
*output:��ת��32bit��
******************************************************************************/
uint32_t reverse32(uint32_t data)
{
    uint8_t i;
    uint32_t temp=0;
	
    for(i=0;i<32;i++)		//��ת
        temp |= ((data>>i) & 0x01)<<(31-i);
    return temp;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//1.���㷨�����ݵ�ǰѡ���CRC��׼����Ϊ�������룬��������ݸ��������𲽼���CRCֵ��
//  �ŵ�:ͨ���ԣ���������Ŀǰͨ�ñ�׼�Լ��ͻ��Զ����׼
//  ȱ��:ÿ����Ҫ���㣬�ȽϺ�ʱ���������������ϴ��ʱ��
//////////////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
*function name:crc8_cal
*function: CRCУ�飬У��ֵΪ8λ
*input:addr-�����׵�ַ��num-���ݳ��ȣ��ֽڣ���type-CRC8���㷨����
*output:8λУ��ֵ
******************************************************************************/
uint8_t crc8_cal(uint8_t *addr, int num, CRC_8 type)  
{  
    uint8_t data;
    uint8_t crc = type.InitValue;					//��ʼֵ
    int i;
	
    for(;num>0;num--)               
    {  
        data = *addr++;
        if(type.InputReverse == true)
        	data = reverse8(data);				//�ֽڷ�ת
        crc = crc^data;							//��crc��ʼֵ��� 
        for(i=0;i<8;i++)						//ѭ��8λ 
        {  
            if(crc&0x80)						//�����Ƴ���λΪ1�����ƺ������ʽ���
                crc = (crc<<1)^type.poly;    
            else								//����ֱ������
                crc <<= 1;                  
        }
    }

    if(type.OutputReverse == true)				//������������ת
        crc = reverse8(crc);
    crc = crc^type.xor;							//����������ֵ���
    return(crc);								//��������У��ֵ
}
 
/*****************************************************************************
*function name:crc16_cal
*function: CRCУ�飬У��ֵΪ16λ
*input:addr-�����׵�ַ��num-���ݳ��ȣ��ֽڣ���type-CRC16���㷨����
*output:16λУ��ֵ
******************************************************************************/
uint16_t crc16_cal(uint8_t *addr, int num, CRC_16 type)  
{  
    uint8_t data;
    uint16_t crc = type.InitValue;					//��ʼֵ
    int i; 
	
    for(;num>0;num--)               
    {  
        data = *addr++;
        if(type.InputReverse == true)
            data = reverse8(data);				//�ֽڷ�ת
        crc = crc^(data<<8);					//��crc��ʼֵ��8λ��� 
        for(i=0;i<8;i++)						//ѭ��8λ 
        {  
            if(crc&0x8000)						//�����Ƴ���λΪ1�����ƺ������ʽ���
                crc = (crc<<1)^type.poly;    
            else								//����ֱ������
                crc <<= 1;                  
        }
    }
    if(type.OutputReverse == true)				//������������ת
        crc = reverse16(crc);
    crc = crc^type.xor;							//����������ֵ���
    return(crc);								//��������У��ֵ
}

/*****************************************************************************
*function name:crc32_cal
*function: CRCУ�飬У��ֵΪ32λ
*input:addr-�����׵�ַ��num-���ݳ��ȣ��ֽڣ���type-CRC32���㷨����
*output:32λУ��ֵ
******************************************************************************/
uint32_t crc32_cal(uint8_t *addr, int num, CRC_32 type)  
{  
    uint8_t data;
    uint32_t crc = type.InitValue;					//��ʼֵ
    int i;
	
    for(;num>0;num--)               
    {  
        data = *addr++;
        if(type.InputReverse == true)
            data = reverse8(data);				//�ֽڷ�ת
        crc = crc^(data<<24);					//��crc��ʼֵ��8λ��� 
        for(i=0;i<8;i++)						//ѭ��8λ 
        {  
            if(crc&0x80000000)					//�����Ƴ���λΪ1�����ƺ������ʽ���
                crc = (crc<<1)^type.poly;    
            else								//����ֱ������
                crc <<= 1;                  
        }
    }
    if(type.OutputReverse == true)				//������������ת
        crc = reverse32(crc);
    crc = crc^type.xor;							//����������ֵ���
    return(crc);								//��������У��ֵ
}


//////////////////////////////////////////////////////////////////////////////////////////////
//2.�������֪������ѡ�õ�CRC�����׼�󣬿���ͨ�����ֱ�Ӳ�ѯCRCֵ���Լӿ����Ч��
//	�ŵ�:Ч�ʸߣ������Ǵ���������ʱ��
//	ȱ��:�Կռ任ʱ�䣬��Ҫ�Ƚ�CRC�������ڴ棬���Ҳ�ͬ�ı�׼��CRC��ͬ������ͨ��
//  ��Ҫע����ǣ�������õı���ݶ���ʽ�Ĳ�ͬ����ͬ���������ò�ͬ����ʽʱ��һ��Ҫ����
//  ���ɶ�Ӧ�ı����Զ���ʽΪ0x07��u8 crc8�����ò�����Ը�дΪ���£�
//////////////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
*function name:GenerateCrc8Table
*function: ����8λcrc��
*input:addr-�����׵�ַ��num-���ݳ��ȣ��ֽڣ���type-CRC32���㷨����
*output:32λУ��ֵ
******************************************************************************/
void GenerateCrc8Table(uint8_t *crc8Table)  
{  
    uint8_t crc=0;
    uint16_t i,j;

    for(j=0;j<256;j++)
    {
        if(!(j%16))						//16����Ϊ1��
        {
        #ifdef CRC_DEBUG
        	LOGD("\r\n");
		#endif
        }
 
        crc = (uint8_t)j;
        for(i=0;i<8;i++)             
        {  
            if(crc&0x80)				//���λΪ1
                crc = (crc<<1)^0x07;	//���ƺ������ʽ���
            else						//����ֱ������
                crc <<= 1;                    
        }
        crc8Table[j] = crc;//ȡ���ֽ�
	#ifdef CRC_DEBUG 
        LOGD("%2x ",crc);
	#endif
    }
#ifdef CRC_DEBUG	
    LOGD("\r\n");
#endif
}

/*****************************************************************************
*function name:crc8withTable
*function: CRCУ�飬У��ֵΪ8λ
*input:addr-�����׵�ַ��len-���ݳ��ȣ��ֽڣ���crc8Table-CRC8��
*output:8λУ��ֵ
******************************************************************************/
uint8_t crc8withTable(uint8_t *addr, int len, uint8_t *crc8Table)  
{  
    uint8_t data;
    uint8_t crc = 00;							//��ʼֵ
    int i;

    for(;len>0;len--)               
    {  
        data = *addr++;
        crc = crc^data;						//��crc��ʼֵ��� 
        crc = crc8Table[crc];				//�滻����forѭ��
		//for (i = 0; i < 8; i++)			//ѭ��8λ 
		//{  
		//	if (crc & 0x80)					//�����Ƴ���λΪ1�����ƺ������ʽ���
		//		crc = (crc << 1) ^ 0x07;    
		//	else							//����ֱ������
		//		crc <<= 1;                  
		//}
    }

    crc = crc^0x00;							//����������ֵ���
    return(crc);							//��������У��ֵ
}

//////////////////////////////////////////////////////////////////////////////////////////
//��3.3.3�е�CRC16����Ϊ������������һ�����˱�ÿ��Ԫ�ض���2�ֽڣ�һ��256��Ԫ�ء�������Ҫ
//��������ֳ����������и��ֽڷ���crcLowTable�����ֽڷ���crcHighTable����Ҳ�����Ϊʲô
//�����������ǰ��������ķ���ȷʵ��ʵ�ֲ���������ɱ�������£�
/*****************************************************************************
*function name:GenerateCrc16Table
*function: ����crc16����õı��
*input: crcHighTable��crcLowTable:256��С�����飬�����ɵı���׵�ַ 
*output:��
******************************************************************************/
void GenerateCrc16Table(uint8_t *crcHighTable, uint8_t *crcLowTable)  
{  
    uint16_t crc=0;
    uint16_t i,j;

    for(j=0;j<256;j++)
    {
        if(!(j%8))
		{
		#ifdef CRC_DEBUG
			LOGD("\r\n");
		#endif
        }
 
        crc = j;
        for(i=0;i<8;i++)             
        {  
            if(crc&0x0001)       				//����ǰ��ͺ���ʡȥ�˷�ת���������������ƣ�������ֵΪ����ʽ�ķ�תֵ
                crc = (crc>>1)^0xA001;			//���ƺ������ʽ��ת�����
            else                   				//����ֱ������
                crc >>= 1;                    
        }
        crcHighTable[j] = (uint8_t)(crc&0xff);		//ȡ���ֽ�
        crcLowTable[j] = (uint8_t)((crc>>8)&0xff);	//ȡ���ֽ�
	#ifdef CRC_DEBUG
		LOGD("%4x  ",crc);
	#endif
    }
#ifdef CRC_DEBUG	
    LOGD("\r\n");
#endif
}

/////////////////////////////////////////////////////////////////////////////////////
//���˱��󣬾Ϳ���ʵ�ֲ���ˣ�Ϊʲô��15-17�д�����Ҳ���������������漰���Ƶ���
//��ӭ��������������ָ�������������£�
/*****************************************************************************
*function name:Crc16withTable
*function: �ò������CRC
*input:  addr���ַ�����ʼ��ַ��len ���ַ������ȣ�table���õ��ı��
*output:��
******************************************************************************/
uint16_t Crc16withTable(uint8_t *addr, int len, uint8_t *crcHighTable, uint8_t *crcLowTable)  
{  
    uint8_t crcHi = 0x00;
    uint8_t crcLo = 0x00;
    uint8_t index;
    uint16_t crc;

    for(;len>0;len--)             
    {  
        index = crcLo^*(addr++);				//��8λ��򣬵õ��������ֵ
        crcLo = crcHi^crcHighTable[index];
        crcHi = crcLowTable[index];
    }
    crc = (uint16_t)(crcHi<<8 | crcLo);
    return(crc^0xffff);							//����У��ֵ
}

