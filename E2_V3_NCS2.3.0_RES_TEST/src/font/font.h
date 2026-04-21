#ifndef __FONT_H__
#define __FONT_H__

#include <stdbool.h>
#include <stdint.h>
#include <nrf9160.h>
#include <zephyr/kernel.h>

#define FONT_16
#define FONT_20
#define FONT_24
#define FONT_28
#define FONT_32
#define FONT_36
#define FONT_48
#define FONT_52
#define FONT_64
#define FONT_68

//#define FONTMAKER_MBCS_FONT		//fontmake根据RM提供的矢量字库转换成的mbcs编码点阵字库数据
#define FONTMAKER_UNICODE_FONT	//fontmake根据RM提供的矢量字库转换成的unicode编码点阵字库数据

#ifdef FONTMAKER_MBCS_FONT
#define FONT_MBCS_HEAD_FLAG_0	0x4D
#define FONT_MBCS_HEAD_FLAG_1	0x46
#define FONT_MBCS_HEAD_FLAG_2	0x4C
#define FONT_MBCS_HEAD_VER		0x11
#define FONT_MBCS_HEAD_LEN		16
#endif

#ifdef FONTMAKER_UNICODE_FONT
#define FONT_UNI_HEAD_FLAG_0			0x55
#define FONT_UNI_HEAD_FLAG_1			0x46
#define FONT_UNI_HEAD_FLAG_2			0x4C
#define FONT_UNI_HEAD_FIXED_LEN			16
#define FONT_UNI_HEAD_NOT_FIXED_LEN		40
#define FONT_UNI_SECT_LEN				8
#define FONT_UNI_SECT_NUM_MAX			10
#endif

typedef enum
{
	FONT_SIZE_MIN,
#ifdef FONT_16
	FONT_SIZE_16 = 16,
#endif
#ifdef FONT_20
	FONT_SIZE_20 = 20,
#endif
#ifdef FONT_24
	FONT_SIZE_24 = 24,
#endif
#ifdef FONT_28
	FONT_SIZE_28 = 28,
#endif
#ifdef FONT_32
	FONT_SIZE_32 = 32,
#endif
#ifdef FONT_36
	FONT_SIZE_36 = 36,
#endif
#ifdef FONT_48
	FONT_SIZE_48 = 48,
#endif
#ifdef FONT_52
	FONT_SIZE_52 = 52,
#endif
#ifdef FONT_64
	FONT_SIZE_64 = 64,
#endif
#ifdef FONT_68
	FONT_SIZE_68 = 68,
#endif
	FONT_SIZE_MAX
}SYSTEM_FONT_SIZE;

typedef enum
{
	FONT_HEIGHT_FIXED 	= 0b00,
	FONT_NOT_FIXED 		= 0b10,
	FONT_NOT_FIXED_EXT 	= 0b11
}FONT_FIXED_FORMAT;

#ifdef FONTMAKER_UNICODE_FONT

#pragma pack(push, 1)

typedef struct
{
	uint8_t width;		//Effective pixel width
	uint8_t height;		//Effective pixel height
	int8_t x_offset;	//x is offset from the origin coordinate
	int8_t y_offset;	//y is offset from the origin coordinate
}font_bbx_t;

typedef struct
{
	uint8_t id[4];			//'U'('S', 'M'), 'F', 'L', X---Unicode(Simple or MBCS) Font Library, X: Version (def: b7 == 1). 
	uint32_t file_size;		//file size
	uint32_t head_size;		//file header size
	uint8_t ScanMode;		//In scanning mode, it is always 0. It can be ignored.
	uint8_t sect_num;		//Section number
	uint16_t wCpFlag;		//Codepage flag: bit0~bit13
	uint32_t bpp;			//Bits per pixel.
	font_bbx_t bbx;			//Font bounding box.
	int32_t font_ascent;	//Font ascent.
	int32_t font_descent;	//Font descent.
	int32_t glyphs_size;	//character count
	int32_t glyphs_used;	//valid character count
}font_uni_head_not_fixed;

typedef struct
{
	uint8_t id[4];
	uint32_t filelen;
	uint8_t sect_num;
	uint8_t hight;
	uint16_t codepage;
	uint16_t charnum;
	uint16_t reserved;
}font_uni_head_fixed;

typedef struct
{
	font_bbx_t bbx;			//The current character's effective pixel width, height, as well as x and y offsets.
	uint8_t dwidth;			//device width, if it is 0, it indicates (overlapping) composite symbols.
	uint16_t bytes;			//Bytes (length of bitmap information)
	uint8_t *bitmap;		//bitmap data
}sbn_glyph_t;

#pragma pack(pop)


typedef union
{
	font_uni_head_not_fixed not_fixed;
	font_uni_head_fixed fixed;
}font_uni_head;

typedef struct
{
	uint16_t first_char;	//The first character
	uint16_t last_char;		//The last character
	uint32_t index_addr;	//The first character points to the offset address of the search table
}font_uni_sect;

typedef struct
{
	uint8_t width;
	uint32_t font_addr;
}font_uni_index;

typedef struct
{
	font_uni_head head;
	font_uni_sect sect[FONT_UNI_SECT_NUM_MAX];
	font_uni_index index;
}font_uni_infor;
#endif

typedef struct
{
	uint16_t isolated;
	uint16_t initial;
	uint16_t medial;
	uint16_t final;
}font_arabic_forms;

typedef struct
{
	uint16_t front;
	uint16_t rear;
	uint16_t deform;
}font_arabic_forms_spec;


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
#ifdef FONT_48
extern unsigned char asc2_4824[96][144];
#endif
#ifdef FONT_64
extern unsigned char asc2_6432[96][256];
#endif

//中文字库
#ifdef FONT_16
//extern unsigned char chinese_1616[8178][32];
#ifdef FONTMAKER_MBCS_FONT
//extern unsigned char RM_JIS_16_1[72192];
//extern unsigned char RM_JIS_16_2[72192];
//extern unsigned char RM_JIS_16_3[72192];
//extern unsigned char RM_JIS_16_4[72192];
//extern unsigned char RM_JIS_16_5[72208];
#endif
#ifdef FONTMAKER_UNICODE_FONT
//extern unsigned char RM_UNI_16_1[88288];
//extern unsigned char RM_UNI_16_2[88288];
//extern unsigned char RM_UNI_16_3[88288];
//extern unsigned char RM_UNI_16_4[88288];
//extern unsigned char RM_UNI_16_5[88288];
//extern unsigned char RM_UNI_16_6[88288];
//extern unsigned char RM_UNI_16_7[88224];
#endif/*FONTMAKER_UNICODE_FONT*/
#endif/*FONT_16*/

#ifdef FONT_24
//extern unsigned char chinese_2424[8178][72];
#ifdef FONTMAKER_UNICODE_FONT
//extern unsigned char RM_UNI_24_1[89600];
//extern unsigned char RM_UNI_24_2[89600];
//extern unsigned char RM_UNI_24_3[89600];
//extern unsigned char RM_UNI_24_4[89600];
//extern unsigned char RM_UNI_24_5[89600];
//extern unsigned char RM_UNI_24_6[89600];
//extern unsigned char RM_UNI_24_7[89600];
//extern unsigned char RM_UNI_24_8[89600];
//extern unsigned char RM_UNI_24_9[89600];
//extern unsigned char RM_UNI_24_10[89600];
//extern unsigned char RM_UNI_24_11[80388];
#endif/*def FONTMAKER_UNICODE_FONT*/

#endif/*FONT_24*/

#ifdef FONT_32
//extern unsigned char chinese_3232[8178][128];
#endif/*FONT_32*/

extern font_arabic_forms ara_froms[35];
extern font_arabic_forms_spec ara_froms_spec[3];
#endif/*__FONT_H__*/
