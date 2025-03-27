/****************************************Copyright (c)************************************************
** File Name:			    crc_check.h
** Descriptions:			data crc check process head file
** Created By:				xie biao
** Created Date:			2021-12-27
** Modified Date:      		2021-12-27 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __CRC_CHECK_H__
#define __CRC_CHECK_H__

typedef struct
{
	uint8_t poly;				//����ʽ
	uint8_t InitValue;			//��ʼֵ
	uint8_t xor;				//������ֵ
	bool InputReverse;		//���뷴ת
	bool OutputReverse;		//�����ת
}CRC_8;
 
typedef struct
{
	uint16_t poly;				//����ʽ
	uint16_t InitValue;		//��ʼֵ
	uint16_t xor;				//������ֵ
	bool InputReverse;		//���뷴ת
	bool OutputReverse;		//�����ת
}CRC_16;
 
typedef struct
{
	uint32_t poly;				//����ʽ
	uint32_t InitValue;		//��ʼֵ
	uint32_t xor;				//������ֵ
	bool InputReverse;		//���뷴ת
	bool OutputReverse;		//�����ת
}CRC_32;


extern CRC_8 crc_8;
extern CRC_8 crc_8_ITU;
extern CRC_8 crc_8_ROHC;
extern CRC_8 crc_8_MAXIM;

extern CRC_16 crc_16_IBM;
extern CRC_16 crc_16_MAXIM;
extern CRC_16 crc_16_USB;
extern CRC_16 crc_16_MODBUS;
extern CRC_16 crc_16_CCITT;
extern CRC_16 crc_16_CCITT_FALSE;
extern CRC_16 crc_16_X5;
extern CRC_16 crc_16_XMODEM;
extern CRC_16 crc_16_DNP;

extern CRC_32 crc_32;
extern CRC_32 crc_32_MPEG2;


extern uint8_t crc8_cal(uint8_t *addr, int num, CRC_8 type);
extern uint16_t crc16_cal(uint8_t *addr, int num, CRC_16 type);
extern uint32_t crc32_cal(uint8_t *addr, int num, CRC_32 type);

#endif/*__CRC_CHECK_H__*/