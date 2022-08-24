/****************************************Copyright (c)************************************************
** File Name:			    crc_check.c
** Descriptions:			data crc check process source file
** Created By:				xie biao
** Created Date:			2021-12-27
** Modified Date:      		2021-12-27 
** Version:			    	V1.0
******************************************************************************************************/
#include <nrf9160.h>
#include <kernel_structs.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include "crc_check.h"
#include "logger.h"

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
*function: 字节反转，如1100 0101 反转后为1010 0011
*input：1字节
*output:反转后字节
******************************************************************************/
uint8_t reverse8(uint8_t data)
{
    uint8_t i;
    uint8_t temp=0;
	
    for(i=0;i<8;i++)	//字节反转
        temp |= ((data>>i) & 0x01)<<(7-i);
    return temp;
}

/*****************************************************************************
*function name:reverse16
*function: 双字节反转，如1100 0101 1110 0101反转后为1010 0111 1010 0011
*input：双字节
*output:反转后双字节
******************************************************************************/
uint16_t reverse16(uint16_t data)
{
    uint8_t i;
    uint16_t temp=0;
	
    for(i=0;i<16;i++)		//反转
        temp |= ((data>>i) & 0x0001)<<(15-i);
    return temp;
}

/*****************************************************************************
*function name:reverse32
*function: 32bit字反转
*input：32bit字
*output:反转后32bit字
******************************************************************************/
uint32_t reverse32(uint32_t data)
{
    uint8_t i;
    uint32_t temp=0;
	
    for(i=0;i<32;i++)		//反转
        temp |= ((data>>i) & 0x01)<<(31-i);
    return temp;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//1.计算法，根据当前选择的CRC标准，作为参数传入，函数会根据各类条件逐步计算CRC值。
//  优点:通用性，可以适用目前通用标准以及客户自定义标准
//  缺点:每步都要计算，比较耗时，尤其在数据量较大的时候
//////////////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
*function name:crc8_cal
*function: CRC校验，校验值为8位
*input:addr-数据首地址；num-数据长度（字节）；type-CRC8的算法类型
*output:8位校验值
******************************************************************************/
uint8_t crc8_cal(uint8_t *addr, int num, CRC_8 type)  
{  
    uint8_t data;
    uint8_t crc = type.InitValue;					//初始值
    int i;
	
    for(;num>0;num--)               
    {  
        data = *addr++;
        if(type.InputReverse == true)
        	data = reverse8(data);				//字节反转
        crc = crc^data;							//与crc初始值异或 
        for(i=0;i<8;i++)						//循环8位 
        {  
            if(crc&0x80)						//左移移出的位为1，左移后与多项式异或
                crc = (crc<<1)^type.poly;    
            else								//否则直接左移
                crc <<= 1;                  
        }
    }

    if(type.OutputReverse == true)				//满足条件，反转
        crc = reverse8(crc);
    crc = crc^type.xor;							//最后返与结果异或值异或
    return(crc);								//返回最终校验值
}
 
/*****************************************************************************
*function name:crc16_cal
*function: CRC校验，校验值为16位
*input:addr-数据首地址；num-数据长度（字节）；type-CRC16的算法类型
*output:16位校验值
******************************************************************************/
uint16_t crc16_cal(uint8_t *addr, int num, CRC_16 type)  
{  
    uint8_t data;
    uint16_t crc = type.InitValue;					//初始值
    int i; 
	
    for(;num>0;num--)               
    {  
        data = *addr++;
        if(type.InputReverse == true)
            data = reverse8(data);				//字节反转
        crc = crc^(data<<8);					//与crc初始值高8位异或 
        for(i=0;i<8;i++)						//循环8位 
        {  
            if(crc&0x8000)						//左移移出的位为1，左移后与多项式异或
                crc = (crc<<1)^type.poly;    
            else								//否则直接左移
                crc <<= 1;                  
        }
    }
    if(type.OutputReverse == true)				//满足条件，反转
        crc = reverse16(crc);
    crc = crc^type.xor;							//最后返与结果异或值异或
    return(crc);								//返回最终校验值
}

/*****************************************************************************
*function name:crc32_cal
*function: CRC校验，校验值为32位
*input:addr-数据首地址；num-数据长度（字节）；type-CRC32的算法类型
*output:32位校验值
******************************************************************************/
uint32_t crc32_cal(uint8_t *addr, int num, CRC_32 type)  
{  
    uint8_t data;
    uint32_t crc = type.InitValue;					//初始值
    int i;
	
    for(;num>0;num--)               
    {  
        data = *addr++;
        if(type.InputReverse == true)
            data = reverse8(data);				//字节反转
        crc = crc^(data<<24);					//与crc初始值高8位异或 
        for(i=0;i<8;i++)						//循环8位 
        {  
            if(crc&0x80000000)					//左移移出的位为1，左移后与多项式异或
                crc = (crc<<1)^type.poly;    
            else								//否则直接左移
                crc <<= 1;                  
        }
    }
    if(type.OutputReverse == true)				//满足条件，反转
        crc = reverse32(crc);
    crc = crc^type.xor;							//最后返与结果异或值异或
    return(crc);								//返回最终校验值
}


//////////////////////////////////////////////////////////////////////////////////////////////
//2.查表法，当知道数据选用的CRC编码标准后，可以通过查表法直接查询CRC值，以加快计算效率
//	优点:效率高，尤其是大数据量的时候
//	缺点:以空间换时间，需要先将CRC表置入内存，而且不同的标准下CRC表不同，不可通用
//  需要注意的是，查表法所用的表根据多项式的不同而不同，所以再用不同多项式时，一定要重新
//  生成对应的表。所以多项式为0x07的u8 crc8函数用查表法可以改写为如下：
//////////////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
*function name:GenerateCrc8Table
*function: 生成8位crc表
*input:addr-数据首地址；num-数据长度（字节）；type-CRC32的算法类型
*output:32位校验值
******************************************************************************/
void GenerateCrc8Table(uint8_t *crc8Table)  
{  
    uint8_t crc=0;
    uint16_t i,j;

    for(j=0;j<256;j++)
    {
        if(!(j%16))						//16个数为1行
            LOGD("\r\n");
 
        crc = (uint8_t)j;
        for(i=0;i<8;i++)             
        {  
            if(crc&0x80)				//最高位为1
                crc = (crc<<1)^0x07;	//左移后与多项式异或
            else						//否则直接左移
                crc <<= 1;                    
        }
        crc8Table[j] = crc;//取低字节
        LOGD("%2x ",crc);
    }
    LOGD("\r\n");
}

/*****************************************************************************
*function name:crc8withTable
*function: CRC校验，校验值为8位
*input:addr-数据首地址；len-数据长度（字节）；crc8Table-CRC8表
*output:8位校验值
******************************************************************************/
uint8_t crc8withTable(uint8_t *addr, int len, uint8_t *crc8Table)  
{  
    uint8_t data;
    uint8_t crc = 00;							//初始值
    int i;

    for(;len>0;len--)               
    {  
        data = *addr++;
        crc = crc^data;						//与crc初始值异或 
        crc = crc8Table[crc];				//替换下面for循环
		//for (i = 0; i < 8; i++)			//循环8位 
		//{  
		//	if (crc & 0x80)					//左移移出的位为1，左移后与多项式异或
		//		crc = (crc << 1) ^ 0x07;    
		//	else							//否则直接左移
		//		crc <<= 1;                  
		//}
    }

    crc = crc^0x00;							//最后返与结果异或值异或
    return(crc);							//返回最终校验值
}

//////////////////////////////////////////////////////////////////////////////////////////
//以3.3.3中的CRC16代码为例，首先生成一个表，此表每个元素都是2字节，一共256个元素。但是需要
//将这个表拆分成两个表，其中高字节放在crcLowTable，低字节放在crcHighTable（我也不理解为什么
//这样做，但是按照这样的方法确实能实现查表法）。生成表代码如下：
/*****************************************************************************
*function name:GenerateCrc16Table
*function: 生成crc16查表法用的表格
*input: crcHighTable，crcLowTable:256大小的数组，即生成的表格首地址 
*output:无
******************************************************************************/
void GenerateCrc16Table(uint8_t *crcHighTable, uint8_t *crcLowTable)  
{  
    uint16_t crc=0;
    uint16_t i,j;

    for(j=0;j<256;j++)
    {
        if(!(j%8))
            LOGD("\r\n");
 
        crc = j;
        for(i=0;i<8;i++)             
        {  
            if(crc&0x0001)       				//由于前面和后面省去了反转，所以这里是右移，且异或的值为多项式的反转值
                crc = (crc>>1)^0xA001;			//右移后与多项式反转后异或
            else                   				//否则直接右移
                crc >>= 1;                    
        }
        crcHighTable[j] = (uint8_t)(crc&0xff);		//取低字节
        crcLowTable[j] = (uint8_t)((crc>>8)&0xff);	//取高字节
        LOGD("%4x  ",crc);		
    }
    LOGD("\r\n");
}

/////////////////////////////////////////////////////////////////////////////////////
//有了表格后，就可以实现查表法了（为什么用15-17行代码我也不清楚，这里可能涉及到推倒，
//欢迎大神们在评论区指导），代码如下：
/*****************************************************************************
*function name:Crc16withTable
*function: 用查表法计算CRC
*input:  addr：字符串起始地址；len ：字符串长度；table：用到的表格
*output:无
******************************************************************************/
uint16_t Crc16withTable(uint8_t *addr, int len, uint8_t *crcHighTable, uint8_t *crcLowTable)  
{  
    uint8_t crcHi = 0x00;
    uint8_t crcLo = 0x00;
    uint8_t index;
    uint16_t crc;

    for(;len>0;len--)             
    {  
        index = crcLo^*(addr++);				//低8位异或，得到表格索引值
        crcLo = crcHi^crcHighTable[index];
        crcHi = crcLowTable[index];
    }
    crc = (uint16_t)(crcHi<<8 | crcLo);
    return(crc^0xffff);							//返回校验值
}

