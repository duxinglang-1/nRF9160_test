#ifndef __FONT_H__
#define __FONT_H__

#define FONT_16
//#define FONT_24
//#define FONT_32

//#define FONTMAKER_FONT		//fontmake根据RM提供的矢量字库转换成的点阵字库数据

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

//英文字库
#ifdef FONT_16
extern unsigned char asc2_1608[96][16];
#ifdef FONTMAKER_FONT
extern unsigned char asc2_16_rm[];
#endif
#endif
#if 0	//def FONT_24
extern unsigned char asc2_2412[96][48];
#endif
#if	0	//def FONT_32
extern unsigned char asc2_3216[96][64];
#endif
//中文字库
#ifdef FONT_16
//extern unsigned char chinese_1616_1[2726][32];
//extern unsigned char chinese_1616_2[2726][32];
//extern unsigned char chinese_1616_3[2726][32];
#ifdef FONTMAKER_FONT
//extern unsigned char RM_JIS_16_1[85952];
//extern unsigned char RM_JIS_16_2[85952];
//extern unsigned char RM_JIS_16_3[85952];
//extern unsigned char RM_JIS_16_4[85936];

//extern unsigned char RM_GBK_16_1[65280];
//extern unsigned char RM_GBK_16_2[65280];
//extern unsigned char RM_GBK_16_3[65280];
//extern unsigned char RM_GBK_16_4[65264];
#endif
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
