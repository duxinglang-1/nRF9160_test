#ifndef __FONT_H__
#define __FONT_H__

typedef enum
{
	FONT_SIZE_MIN,
	FONT_SIZE_16 = 16,
	FONT_SIZE_24 = 24,
	FONT_SIZE_32 = 32,
	FONT_SIZE_MAX
}system_font_size;

//Ó¢ÎÄ×Ö¿â
extern const unsigned char asc2_1608[95][16];
extern const unsigned char asc2_2412[95][36];
extern const unsigned char asc2_3216[95][64];
//ÖÐÎÄ×Ö¿â
extern const unsigned char chinese_1616[8178][32];
extern const unsigned char chinese_2424[8178][72];
extern const unsigned char chinese_3232[8178][128];

#endif/*__FONT_H__*/
