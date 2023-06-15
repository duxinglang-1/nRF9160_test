/****************************************Copyright (c)************************************************
** File Name:			    codetrans.h
** Descriptions:			Key message process head file
** Created By:				xie biao
** Created Date:			2020-12-28
** Modified Date:      		2020-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __CODETRANS_H__
#define __CODETRANS_H__

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __MMI_CHSET_IDEOGRAM_SUPPORT__

//#define __MMI_CHSET_SJIS__
//#define __MMI_CHSET_GB18030__

#define CHSET_MAX_COUNT             25
#define CHSET_BIT_WORD_16           16
#define CHSET_BIT_WORD_8            8
#define CHSET_TMP_BUFF_SIZE         2048        /* the size of the temp buffer in mmi_chset_convert() */

/* Enum of all supported charsets*/
typedef enum
{
    MMI_CHSET_BASE,
    MMI_CHSET_ASCII, 	// ASCII
    MMI_CHSET_WESTERN_WIN,
    /* Add new (8-bit) encodings above this line */
    MMI_CHSET_BIG5,  	// Big5 (Traditional chinese)
    MMI_CHSET_GB2312, 	// GB2312 (Simplified chinese)
    MMI_CHSET_HKSCS,  	// HKSCS 2004 (Hong Kong chinese)
    MMI_CHSET_SJIS,  	// SJIS (Japanese)
    MMI_CHSET_GB18030, 	// GB18030 (Simplified chinese-extended)
    MMI_CHSET_UTF7,  	// UTF-7 
    /* Place all CJK encodings above this one */
    MMI_CHSET_UTF16LE, 	// UTF-16LE
    MMI_CHSET_UTF16BE, 	// UTF-16BE
    MMI_CHSET_UTF8,  	// UTF-8
    MMI_CHSET_UCS2,  	// UCS2
    MMI_CHSET_TOTAL 
}ENUM_MMI_CHSET;

typedef enum
{
    BASE_ENCODING_SCHEME,

    ASCII_TO_UCS2,
    UCS2_TO_ASCII,   
    
    WESTERN_WINDOWS_TO_UCS2,
    UCS2_TO_WESTERN_WINDOWS,
    /* Add new (8-bit) encodings above this line */
	
    BIG5_TO_UCS2,
    UCS2_TO_BIG5,

    GB2312_TO_UCS2,
    UCS2_TO_GB2312,

    HKSCS_TO_UCS2,
    UCS2_TO_HKSCS,

    SJIS_TO_UCS2,
    UCS2_TO_SJIS,

    GB18030_TO_UCS2,
    UCS2_TO_GB18030,

    UTF7_TO_UCS2,
    UCS2_TO_UTF7,
    /* Place all CJK encodings above this one */
	
    UTF16LE_TO_UCS2,
    UCS2_TO_UTF16LE,
    
    UTF16BE_TO_UCS2,
    UCS2_TO_UTF16BE,
    
    UTF8_TO_UCS2,
    UCS2_TO_UTF8,

    CHSET_PAIR_TOTAL
}ENUM_MMI_CHSET_PAIR;

typedef enum
{
    BIG_5_ENCODING_TYPE,
    GB2312_ENCODING_TYPE,
    HKSCS_ENCODING_TYPE,
    NO_OF_TEXT_ENCODING
} ENUM_ENCODING_TYPE;

typedef struct
{
    short start;
    short end;
} key_index_t;

typedef struct
{
    unsigned short start_value;
    unsigned short end_value;
    unsigned short index;
} Mapping_Struct;

typedef struct
{
    const Mapping_Struct *pMappingTable;
    const uint16_t *pConversnTable;
    uint16_t TotalMapTbIndex;
    uint8_t Input_word_bits;
    uint8_t Output_word_bits;
    uint16_t UndefChar;   /* currently taken undefchar as quesmark */
} mmi_chset_info_struct;



extern void mmi_chset_init(void);
extern int32_t mmi_chset_convert(ENUM_MMI_CHSET src_type, ENUM_MMI_CHSET dest_type, char *src_buff, char *dest_buff, int32_t dest_size);


#ifdef __cplusplus
}
#endif

#endif/*__CODETRANS_H__*/
