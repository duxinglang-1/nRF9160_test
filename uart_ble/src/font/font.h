#ifndef __FONT_H__
#define __FONT_H__

#include <stdbool.h>
#include <stdint.h>
#include <nrf9160.h>
#include <zephyr.h>

#define FONT_16
#define FONT_24
//#define FONT_32

//#define FONTMAKER_MBCS_FONT		//fontmake根据RM提供的矢量字库转换成的mbcs编码点阵字库数据
//#define FONTMAKER_UNICODE_FONT	//fontmake根据RM提供的矢量字库转换成的unicode编码点阵字库数据

#ifdef FONTMAKER_MBCS_FONT
#define FONT_MBCS_HEAD_FLAG_0	0x4D
#define FONT_MBCS_HEAD_FLAG_1	0x46
#define FONT_MBCS_HEAD_FLAG_1	0x4C
#define FONT_MBCS_HEAD_FLAG_1	0x11
#define FONT_MBCS_HEAD_LEN		16
#endif

#ifdef FONTMAKER_UNICODE_FONT
#define FONT_UNI_HEAD_FLAG_0	0x55
#define FONT_UNI_HEAD_FLAG_1	0x46
#define FONT_UNI_FLAG_1			0x4C
#define FONT_UNI_HEAD_FLAG_1	0x11
#define FONT_UNI_HEAD_LEN		16
#define FONT_UNI_SECT_LEN		8
#define FONT_UNI_SECT_NUM_MAX	10
#endif

typedef enum
{
	FONT_SIZE_MIN,
#ifdef FONT_16
	FONT_SIZE_16 = 16,
#endif
#ifdef FONT_24
	FONT_SIZE_24 = 24,
#endif
#ifdef FONT_32
	FONT_SIZE_32 = 32,
#endif
	FONT_SIZE_MAX
}SYSTEM_FONT_SIZE;

#ifdef FONTMAKER_UNICODE_FONT
typedef struct
{
	u8_t id[4];
	u32_t filelen;
	u8_t sect_num;
	u8_t hight;
	u16_t codepage;
	u16_t charnum;
	u16_t reserved;
}font_uni_head;

typedef struct
{
	u16_t first_char;
	u16_t last_char;
	u32_t index_addr;
}font_uni_sect;

typedef struct
{
	u8_t width;
	u32_t font_addr;
}font_uni_index;

typedef struct
{
	font_uni_head head;
	font_uni_sect sect[FONT_UNI_SECT_NUM_MAX];
	font_uni_index index;
}font_uni_infor;
#endif

//英文字库
#ifdef FONT_16
extern unsigned char asc2_1608[96][16];
#ifdef FONTMAKER_MBCS_FONT
extern unsigned char asc2_16_rm[];
#endif
#endif
#ifdef FONT_24
extern unsigned char asc2_2412[96][48];
#endif
#ifdef FONT_32
extern unsigned char asc2_3216[96][64];
#endif
//中文字库
#ifdef FONT_16
//extern unsigned char chinese_1616_1[2726][32];
//extern unsigned char chinese_1616_2[2726][32];
//extern unsigned char chinese_1616_3[2726][32];
#ifdef FONTMAKER_MBCS_FONT
//extern unsigned char RM_JIS_16_1[72192];
//extern unsigned char RM_JIS_16_2[72192];
//extern unsigned char RM_JIS_16_3[72192];
//extern unsigned char RM_JIS_16_4[72192];
//extern unsigned char RM_JIS_16_5[72208];
#endif

#if 0	//def FONTMAKER_UNICODE_FONT
//extern unsigned char RM_UNI_16_1[88288];
//extern unsigned char RM_UNI_16_2[88288];
//extern unsigned char RM_UNI_16_3[88288];
//extern unsigned char RM_UNI_16_4[88288];
//extern unsigned char RM_UNI_16_5[88288];
//extern unsigned char RM_UNI_16_6[88288];
//extern unsigned char RM_UNI_16_7[88224];
#endif/*FONTMAKER_UNICODE_FONT*/
#endif

#ifdef FONT_24
//extern unsigned char chinese_2424_1[1200][72];
//extern unsigned char chinese_2424_2[1200][72];
//extern unsigned char chinese_2424_3[1200][72];
//extern unsigned char chinese_2424_4[1200][72];
//extern unsigned char chinese_2424_5[1200][72];
//extern unsigned char chinese_2424_6[1200][72];
//extern unsigned char chinese_2424_7[977][72];
#endif

#ifdef FONT_32
//extern unsigned char chinese_3232_1[700][128];
//extern unsigned char chinese_3232_2[700][128];
//extern unsigned char chinese_3232_3[700][128];
//extern unsigned char chinese_3232_4[700][128];
//extern unsigned char chinese_3232_5[700][128];
//extern unsigned char chinese_3232_6[700][128];
//extern unsigned char chinese_3232_7[700][128];
//extern unsigned char chinese_3232_8[700][128];
//extern unsigned char chinese_3232_9[700][128];
//extern unsigned char chinese_3232_10[700][128];
//extern unsigned char chinese_3232_11[700][128];
//extern unsigned char chinese_3232_12[478][128];
#endif

#endif/*__FONT_H__*/
