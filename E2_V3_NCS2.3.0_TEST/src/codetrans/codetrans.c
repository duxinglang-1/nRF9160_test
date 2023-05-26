/****************************************Copyright (c)************************************************
** File Name:			    codetrans.c
** Descriptions:			Key message process source file
** Created By:				xie biao
** Created Date:			2020-12-28
** Modified Date:      		2020-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <logging/log.h>
#include <nrfx.h>
#include "codetrans.h"
#include "CharsetTable.h"
#include "logger.h"

/* Handling UCS2 string not aligned with 2 bytes. (Obigo adopts UTF8 by default) */
#define MMI_CHSET_DBG_ASSERT(_expr)
// #define MMI_CHSET_UTF8_FAULT_TOLERANT
#define MMI_CHSET_UTF8_ALLOW_BOM
#define CAST_PU8(RAW)   ((uint8_t*)(RAW))
#define CAST_CPU8(RAW)  ((const uint8_t*)(RAW))
#define GET_U8_BYTE_TO_U16(RAW, i)  ((uint16_t)(CAST_CPU8(RAW)[i]))
#define STR_SIZE(len) ((len)<<1)
#define STR_AT(RAW, n) ((void*)(CAST_PU8(RAW)+STR_SIZE(n)))
#define CHR_AT(RAW, n) ((uint16_t)(GET_U8_BYTE_TO_U16(RAW, STR_SIZE(n))+(GET_U8_BYTE_TO_U16(RAW, STR_SIZE(n)+1)<<8)))
#define SET_CHR_AT(RAW, n, value)   do {                          \
                              uint8_t *_p = STR_AT(RAW, n);     \
                              uint16_t v= (uint16_t) (value); \
                              _p[0] = (uint8_t) (v & 0xff);     \
                              _p[1] = (uint8_t) (v >> 8);       \
                           } while (0)
#define CHR(x)      ((uint16_t)(x))

#define INVERSE_HIGH_LOW_BYTE(x) (((x) >> 8) | (((x) & 0xff)<<8))

#define MERGE_TO_WCHAR(high, low)  (high << 8) + ((uint8_t)low)
#define UTF16_HIGH_START      0xD800
#define UTF16_HIGH_END        0xDBFF
#define UTF16_LOW_START       0xDC00
#define UTF16_LOW_END         0xDFFF

static bool g_chset_tbl_is_init = false;
static mmi_chset_info_struct* g_chset_tbl[CHSET_PAIR_TOTAL] = {0};


#ifdef __MMI_CHSET_IDEOGRAM_SUPPORT__
const key_index_t *g_chset_ucs2_to_encode_key_LSB_index[NO_OF_TEXT_ENCODING] = 
{
#ifdef __MMI_CHSET_BIG5__
    g_chset_ucs2_to_big5_key_LSB_index,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_GB2312__
    g_chset_ucs2_to_gb2312_key_LSB_index,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_HKSCS__
    g_chset_ucs2_to_hkscs_key_LSB_index
#else 
    NULL
#endif 
};

const key_index_t *g_chset_encode_to_ucs2_key_LSB_index[NO_OF_TEXT_ENCODING] = 
{
#ifdef __MMI_CHSET_BIG5__
    g_chset_big5_to_ucs2_key_LSB_index,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_GB2312__
    g_chset_gb2312_to_ucs2_key_LSB_index,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_HKSCS__
    g_chset_hkscs_to_ucs2_key_LSB_index
#else 
    NULL
#endif 
};

const uint8_t *g_chset_ucs2_to_encode_key_MSB[NO_OF_TEXT_ENCODING] = 
{
#ifdef __MMI_CHSET_BIG5__
    g_chset_ucs2_to_big5_key_MSB,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_GB2312__
    g_chset_ucs2_to_gb2312_key_MSB,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_HKSCS__
    g_chset_ucs2_to_hkscs_key_MSB
#else 
    NULL
#endif 
};

const uint8_t *g_chset_encode_to_ucs2_key_MSB[NO_OF_TEXT_ENCODING] = 
{
#ifdef __MMI_CHSET_BIG5__
    g_chset_big5_to_ucs2_key_MSB,
#else 
    NULL,
#endif 
    NULL, //new struct for gb2312 MSB table
#ifdef __MMI_CHSET_HKSCS__
    g_chset_hkscs_to_ucs2_key_MSB
#else 
    NULL
#endif 
};

const uint16_t *g_chset_ucs2_to_encode_table[NO_OF_TEXT_ENCODING] = 
{
#ifdef __MMI_CHSET_BIG5__
    g_chset_ucs2_to_big5_table,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_GB2312__
    g_chset_ucs2_to_gb2312_table,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_HKSCS__
    g_chset_ucs2_to_hkscs_table
#else 
    NULL
#endif 
};

const uint16_t *g_chset_encode_to_ucs2_table[NO_OF_TEXT_ENCODING] = 
{
#ifdef __MMI_CHSET_BIG5__
    g_chset_big5_to_ucs2_table,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_GB2312__
    g_chset_gb2312_to_ucs2_table,
#else 
    NULL,
#endif 
#ifdef __MMI_CHSET_HKSCS__
    g_chset_hkscs_to_ucs2_table
#else 
    NULL
#endif 
};

const uint16_t g_chset_unknown_encode_char_no_space[] = 
{
    0xFFFF,
    0xFFFF,
    0xFFFF
};

const uint16_t g_chset_unknown_encode_char_space[] = 
{
    0x20,
    0x20,
    0x20
};

const uint16_t *g_chset_unknown_encode_char = g_chset_unknown_encode_char_no_space;
#endif /* __MMI_CHSET_IDEOGRAM_SUPPORT__ */ 

static const uint8_t g_cheset_utf8_bytes_per_char[16] = 
{
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 2, 2, 3, 4  /* we don't support UCS4 */
};

const uint8_t g_chset_state[] = 
{
    0,
    1,  /* MMI_CHSET_ASCII */

#ifdef __MMI_CHSET_WESTERN_WIN__
    1,
#else 
    0,
#endif 

#ifdef __MMI_CHSET_BIG5__
    1,
#else 
    0,
#endif 

#ifdef __MMI_CHSET_GB2312__
    1,
#else 
    0,
#endif 

#ifdef __MMI_CHSET_HKSCS__
    1,
#else 
    0,
#endif 

#ifdef __MMI_CHSET_SJIS__
    1,
#else 
    0,
#endif 

#ifdef __MMI_CHSET_GB18030__
    1,
#else 
    0,
#endif 

#ifdef __MMI_CHSET_UTF7__
    1,
#else
    0,
#endif

    /* PLace all encodings using algo to above this one */
    1,  /* MMI_CHSET_UTF16LE, */
    1,  /* MMI_CHSET_UTF16BE, */
    1,  /* MMI_CHSET_UTF8, */
    1,  /* MMI_CHSET_UCS2, */
    1   /* MMI_CHSET_TOTAL */
};


#ifdef MMI_CHSET_UTF8_ALLOW_BOM
/*****************************************************************************
 * FUNCTION
 *  mmi_chset_is_utf8_BOM
 * DESCRIPTION
 *  
 * PARAMETERS
 *  str     [IN]        
 * RETURNS
 *  
 *****************************************************************************/
static bool mmi_chset_is_utf8_BOM(const uint8_t *str)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (str[0] == 0xEF && str[1] == 0xBB && str[2] == 0xBF)
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif /* MMI_CHSET_UTF8_ALLOW_BOM */ 

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_get_utf_byte_order
 * DESCRIPTION
 *  The function is used for get the byte order via the byte order mark .
 * PARAMETERS
 *  str_bom     [IN]        The byte order mark.
 * RETURNS
 *  returns MMI_CHSET_UTF16LE if the stream is little-endian
 *  returns MMI_CHSET_UTF16BE if the stream is big-endian
 *  returns MMI_CHSET_UTF8 if the input BOM is a UTF-8's BOM
 *  Otherwise returns -1
 *****************************************************************************/
int8_t mmi_chset_get_utf_byte_order(const int8_t *str_bom)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint16_t wc;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    wc = MERGE_TO_WCHAR(str_bom[0], str_bom[1]);

    switch (wc)
    {
    case 0xFFFE:
        return MMI_CHSET_UTF16LE;

    case 0xFEFF:
        return MMI_CHSET_UTF16BE;

    case 0xEFBB:
        if((int8_t)0xBF == str_bom[2])
            return MMI_CHSET_UTF8;
        else
            return -1;

    default:
        return -1;
    }
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_binary_search
 * DESCRIPTION
 *  Binary search MSB index in the mapping table
 * PARAMETERS
 *  key                 [IN]        value to find
 *  lookup_table        [IN]        lookup table
 *  start               [IN]        start index
 *  end                 [IN]        end index
 * RETURNS
 *  The MSB index in the lookup table
 *****************************************************************************/
static int16_t mmi_chset_binary_search(
                    const uint8_t key,
                    const uint8_t *lookup_table,
                    int16_t start,
                    int16_t end)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (start <= end)
    {
        int16_t mid = (start + end) / 2;

        if (key > lookup_table[mid])
        {
            start = mid + 1;
        }
        else if (key < lookup_table[mid])
        {
            end = mid - 1;
        }
        else    /* key == target */
        {
            return mid;
        }
    }

    return -1;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_utf8_len
 * DESCRIPTION
 *  
 * PARAMETERS
 *  ucs2        [IN]        
 * RETURNS
 *  
 *****************************************************************************/
static int mmi_chset_ucs2_to_utf8_len(uint16_t ucs2)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (ucs2 < (uint16_t)0x80)
    {
        return 1;
    }
    else if (ucs2 < (uint16_t)0x800)
    {
        return 2;
    }
    else
    {
        return 3;
    }
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_copy_to_dest
 * DESCRIPTION
 *  When the source charset type equals to dest charset type, copy source to dest directly
 * PARAMETERS
 *  src_type        [IN]        Charset type of source
 *  src_buff        [IN]        Buffer stores source string
 *  dest_buff       [OUT]       Buffer stores destination string
 *  dest_size       [IN]        Size of destination buffer (bytes)
 *  src_end_pos     [OUT]       
 * RETURNS
 *  Bytes copied
 *****************************************************************************/
int32_t mmi_chset_copy_to_dest(
            ENUM_MMI_CHSET src_type,
            uint8_t *src_buff,
            uint8_t *dest_buff,
            int32_t dest_size,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t WordBytes = 0;
    ENUM_MMI_CHSET_PAIR Scheme;
    int32_t count = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if(src_type == MMI_CHSET_UCS2)
    {
        WordBytes = CHSET_BIT_WORD_16;
    }
    else
    {
        Scheme = (ENUM_MMI_CHSET_PAIR) (BASE_ENCODING_SCHEME + (src_type - MMI_CHSET_BASE) * 2 - 1);
		__ASSERT(Scheme < CHSET_PAIR_TOTAL, "Scheme < CHSET_PAIR_TOTAL");
        WordBytes = g_chset_tbl[Scheme]->Input_word_bits;
    }

    if(WordBytes == CHSET_BIT_WORD_16)
    {
        for (count = 0; src_buff[count] != 0 || src_buff[count + 1] != 0; count = count + 2)
        {
            if (count >= dest_size - 2)
            {
                count = (uint16_t)dest_size - 2;
                break;
            }
            dest_buff[count] = src_buff[count];
            dest_buff[count + 1] = src_buff[count + 1];
        }
        *src_end_pos = (uint32_t)(src_buff + count);
        dest_buff[count] = 0;
        dest_buff[++count] = 0;
        return count + 1;
    }
    else if(WordBytes == CHSET_BIT_WORD_8)
    {
        for (count = 0; src_buff[count] != 0; count++)
        {
            if (count >= dest_size - 1)
            {
                count = (uint16_t)dest_size - 1;
                break;
            }
            dest_buff[count] = src_buff[count];
        }
        *src_end_pos = (uint32_t)(src_buff + count);
        dest_buff[count] = 0;
        return count + 1;
    }
    else
    {
        return 0;
    }
}

static int32_t mmi_chset_simple_convert(
			 uint8_t *dest,
			 int32_t dest_size,
			 uint8_t *src,
			 uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	int32_t bytecount = 0;
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/

    while ((*src != 0) && (bytecount+2 < dest_size))
    {
        if (*src < 0x7F)
        {
            *dest++ = *src++;
            *dest++ = 0x00;
        }
        else
        {
            *dest++ = 0x3F;
            *dest++ = 0x00;
        }
        bytecount += 2;
    }

    *src_end_pos = (uint32_t)src;

    *dest++ = 0x00;
    *dest = 0x00;
    return bytecount;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_ascii
 * DESCRIPTION
 *  
 * PARAMETERS
 *  encode_char     [IN]        
 *  encoding        [IN]        
 * RETURNS
 *  
 *****************************************************************************/
static uint16_t mmi_chset_ucs2_to_ascii(uint8_t *pOutBuffer, uint8_t *pInBuffer, int32_t dest_size, uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint16_t count = 0;
    *src_end_pos = (uint32_t)pInBuffer;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (!((*pInBuffer == 0) && (*(pInBuffer + 1) == 0)) && (dest_size > count+1))
    {      
        if (((uint16_t)(*pInBuffer) <= 0x7F) && ((uint16_t)(*(pInBuffer+1)) == 0))
        {
            *pOutBuffer = *(pInBuffer);
        }
        else
        {
            *pOutBuffer = (char)0xFF;            
        }
        pInBuffer += 2;
        pOutBuffer++;
        count++;
    }

    *pOutBuffer = 0;
    *src_end_pos = (uint32_t)pInBuffer;
	
    return count+1;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ascii_to_ucs2
 * DESCRIPTION
 *
 * PARAMETERS
 *  pOutBuffer      [OUT]  UCS2 destination string   
 *  pInBuffer       [IN]   ASCII source string  
 * RETURNS
 *  Return the bytes to convert.
 *****************************************************************************/
static uint16_t mmi_chset_ascii_to_ucs2(uint8_t *pOutBuffer, uint8_t *pInBuffer, int32_t dest_size, uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t count = 0;
    *src_end_pos = (uint32_t)pInBuffer;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while(*pInBuffer != '\0' && dest_size >= (count+4))
    {
        if(*pInBuffer > (char)0x7F)
        {
            pOutBuffer[count++] = (char)0xFF;
            pOutBuffer[count++] = (char)0xFF;
        }
        else
        {
            pOutBuffer[count++] = *pInBuffer;
            pOutBuffer[count++] = '\0';
        }
	 pInBuffer++;
    }
    pOutBuffer[count++] = '\0';
    pOutBuffer[count++] = '\0';
    *src_end_pos = (uint32_t)pInBuffer;

    return (uint16_t) count;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_utf8
 * DESCRIPTION
 *  
 * PARAMETERS
 *  utf8        [OUT]       
 *  ucs2        [IN]        
 * RETURNS
 *  
 *****************************************************************************/
int32_t mmi_chset_ucs2_to_utf8(uint8_t *utf8, uint16_t ucs2)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int count;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if(ucs2 < (uint16_t)0x80)
    {
        count = 1;
    }
    else if(ucs2 < (uint16_t)0x800)
    {
        count = 2;
    }
    else
    {
        count = 3;
    }
	
    switch(count)
    {
    case 3:
        utf8[2] = 0x80 | (ucs2 & 0x3f);
        ucs2 = ucs2 >> 6;
        ucs2 |= 0x800;
    case 2:
        utf8[1] = 0x80 | (ucs2 & 0x3f);
        ucs2 = ucs2 >> 6;
        ucs2 |= 0xc0;
    case 1:
        utf8[0] = (uint8_t)ucs2;
    }
    return count;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_utf8_string_ex
 * DESCRIPTION
 *  
 * PARAMETERS
 *  dest            [OUT]       
 *  dest_size       [IN]        
 *  src             [IN]        
 *  src_end_pos     [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
int32_t mmi_chset_ucs2_to_utf8_string_ex(
            uint8_t *dest,
            int32_t dest_size,
            uint8_t *src,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int pos = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while(src[0] || src[1])
    {
        if (pos + mmi_chset_ucs2_to_utf8_len(CHR_AT(src, 0)) >= dest_size)      /* leave space of '\0' */
        {
            MMI_CHSET_DBG_ASSERT(0);
            break;
        }

        pos += mmi_chset_ucs2_to_utf8(dest + pos, CHR_AT(src, 0));
        src += 2;
        if (pos >= dest_size - 1)
        {
            break;
        }
    }
    if (pos >= dest_size)
    {
        MMI_CHSET_DBG_ASSERT(0);
        dest[dest_size - 1] = 0;
    }
    else
    {
        dest[pos] = 0;
    }
    *src_end_pos = (uint32_t)src;
    return pos + 1;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_utf8_to_ucs2
 * DESCRIPTION
 *  
 * PARAMETERS
 *  dest        [OUT]       
 *  utf8        [IN]        
 * RETURNS
 *  
 *****************************************************************************/
int32_t mmi_chset_utf8_to_ucs2(uint8_t *dest, uint8_t *utf8)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t c = utf8[0];
    int count = g_cheset_utf8_bytes_per_char[c >> 4];
    uint16_t ucs2 = 0xFFFF;   /* invalid character */

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    switch (count)
    {
    case 1:
        ucs2 = (uint16_t)c;
        break;
    case 2:
        if (utf8[1])
        {
            ucs2 = ((uint16_t) (c & 0x1f) << 6) | (uint16_t) (utf8[1] ^ 0x80);
        }
        break;
    case 3:
        if (utf8[1] && utf8[2])
        {
            ucs2 = ((uint16_t) (c & 0x0f) << 12)
                | ((uint16_t) (utf8[1] ^ 0x80) << 6) | (uint16_t) (utf8[2] ^ 0x80);
        }
        break;
    case 4:
        break;
    default:
        count = 1;   /* the character cannot be converted, return 1 means always consume 1 byte */
        break;
    }

    SET_CHR_AT(dest, 0, ucs2);

    return count;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_utf8_to_ucs2_string_ex
 * DESCRIPTION
 *  
 * PARAMETERS
 *  dest            [OUT]       
 *  dest_size       [IN]        
 *  src             [IN]        
 *  src_end_pos     [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
int32_t mmi_chset_utf8_to_ucs2_string_ex(
            uint8_t *dest,
            int32_t dest_size,
            uint8_t *src,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int pos = 0;
    int cnt;
    int src_len = strlen((const char*)src);

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
#ifdef MMI_CHSET_UTF8_ALLOW_BOM
    if (src_len >= 3 && mmi_chset_is_utf8_BOM(src))
    {
        src += 3;
        src_len -= 3;
    }
#endif /* MMI_CHSET_UTF8_ALLOW_BOM */ 

    MMI_CHSET_DBG_ASSERT((dest_size % 2) == 0);
    dest_size -= (dest_size % 2);
    *src_end_pos = (uint32_t)src; /* set src_end_pos first */

    if(dest_size < 2)
    {
        MMI_CHSET_DBG_ASSERT(0);    /* dest wont be NULL-terminated */
        return 0;
    }

    while (*src && pos < dest_size - 2)
    {
        cnt = mmi_chset_utf8_to_ucs2(dest + pos, src);
         
        if(
             (int32_t)(src - (*src_end_pos)) >= (int32_t)(src_len - cnt) &&
             (*(dest + pos) == 0xFF && *(dest + pos + 1) == 0xFF) &&
            !(*src == 0xEF && *(src+1) == 0xBF && *(src+2) == 0xBF)
            )
        {
            /* 
             * If src isn't 0xEF, 0xBF, 0xBF and its remain length is not enough but dest is 0xFFFF, we will abort the process.
             * dest data is invalid character because src data is not enough to convert 
             */
            break;
        }
        if (cnt == 0)   /* abnormal case */
        {
        #ifdef MMI_CHSET_UTF8_FAULT_TOLERANT
            src++;
        #else 
            break;
        #endif 
        }        
        else    /* normal case */
        {
            src += cnt;
            pos += 2;
        }
    }
    *src_end_pos = (uint32_t) src;
    dest[pos] = 0;
    dest[pos + 1] = 0;
    return pos + 2;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_utf16_string
 * DESCRIPTION
 *  Converts Unicode text into UTF-16 encoding.
 * PARAMETERS
 *  dest_utf16      [OUT]       Destination buffer. On return, contains the Unicode encoded output string.
 *  dest_size       [IN]        The size of Destination buffer.
 *  utf16_type      [IN]        The type of input UTF-16 encoded string. MMI_CHSET_UTF16LE or MMI_CHSET_UTF16BE
 *  include_bom     [IN]        Should include the byte order mark in the UTF-16 encoded output string.
 *  src_ucs2        [IN]        The UTF-16 encoded input string.
 *  src_end_pos     [OUT]       The point to the end of converted sub-string at the input string.
 * RETURNS
 *  The number of converted bytes, including the final null wide-characters.
 *****************************************************************************/
static int32_t mmi_chset_ucs2_to_utf16_string(
                    uint8_t *dest_utf16,
                    int32_t dest_size,
                    ENUM_MMI_CHSET utf16_type,
                    bool include_bom,
                    uint8_t *src_ucs2,
                    uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t count = 0;
    uint16_t wc;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	__ASSERT(dest_utf16 && src_ucs2 && src_end_pos, "dest_utf16 && src_ucs2 && src_end_pos");
	__ASSERT((MMI_CHSET_UTF16BE == utf16_type) || (MMI_CHSET_UTF16LE == utf16_type), "(MMI_CHSET_UTF16BE == utf16_type) || (MMI_CHSET_UTF16LE == utf16_type)");

    if (include_bom)
    {
        if (MMI_CHSET_UTF16BE == utf16_type)
        {
            dest_utf16[0] = (uint8_t)0xFF;
            dest_utf16[1] = (uint8_t)0xFE;
        }
        else
        {
            dest_utf16[0] = (uint8_t)0xFE;
            dest_utf16[1] = (uint8_t)0xFF;
        }
        dest_utf16 += 2;
        dest_size -= 2;
        count += 2;
    }

    while((src_ucs2[0] || src_ucs2[1]) && (count < (dest_size - 2)))
    {
        wc = MERGE_TO_WCHAR(src_ucs2[1], src_ucs2[0]);

        /* UTF-16 surrogate values are illegal in UCS2; 
           0xffff or 0xfffe are both reserved values */
        if (!(wc >= UTF16_HIGH_START && wc <= UTF16_LOW_END))
        {
            if (MMI_CHSET_UTF16LE == utf16_type)
            {
                dest_utf16[0] = src_ucs2[0];
                dest_utf16[1] = src_ucs2[1];
            }
            else
            {
                dest_utf16[0] = src_ucs2[1];
                dest_utf16[1] = src_ucs2[0];
            }
            dest_utf16 += 2;
            count += 2;
        }
        src_ucs2 += 2;
    }

    dest_utf16[0] = '\0';
    dest_utf16[1] = '\0';

    *src_end_pos = (uint32_t) src_ucs2;

    return (count + 2);
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_utf16_to_ucs2_string
 * DESCRIPTION
 *  Converts text encoded using the Unicode transformation format UTF-16
 *  into the Unicode UCS-2 character set.
 * PARAMETERS
 *  dest_ucs2       [OUT]       Destination buffer. On return, contains the Unicode encoded output string.
 *  dest_size       [IN]        The size of Destination buffer.
 *  src_utf16       [IN]        The UTF-16 encoded input string.
 *  utf16_type      [IN]        The type of input UTF-16 encoded string. MMI_CHSET_UTF16LE or MMI_CHSET_UTF16BE
 *  src_end_pos     [OUT]       The point to the end of converted sub-string at the input string.
 * RETURNS
 *  The number of converted bytes, including the final null wide-characters.
 *****************************************************************************/
static int32_t mmi_chset_utf16_to_ucs2_string(
                    uint8_t *dest_ucs2,
                    int32_t dest_size,
                    uint8_t *src_utf16,
                    ENUM_MMI_CHSET utf16_type,
                    uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int8_t utf16_bo;
    int32_t count = 0;
    uint16_t wc1;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	__ASSERT(dest_ucs2 && src_utf16 && src_end_pos, "dest_ucs2 && src_utf16 && src_end_pos");
	__ASSERT((MMI_CHSET_UTF16BE == utf16_type) || (MMI_CHSET_UTF16LE == utf16_type), "(MMI_CHSET_UTF16BE == utf16_type) || (MMI_CHSET_UTF16LE == utf16_type)");

    utf16_bo = mmi_chset_get_utf_byte_order((int8_t *)src_utf16);

    if (-1 != utf16_bo)
    {
        src_utf16 += 2;
        /* needn't convert the beginning BOM */
    }
    else
    {
        utf16_bo = utf16_type;
    }

    while ((src_utf16[0] || src_utf16[1]) && (count < (dest_size - 2)))
    {
        wc1 = ((MMI_CHSET_UTF16BE == utf16_bo) ?
               MERGE_TO_WCHAR(src_utf16[0], src_utf16[1]) : MERGE_TO_WCHAR(src_utf16[1], src_utf16[0]));

        /* we ignore surrogate pair */
        if (((wc1 >= UTF16_HIGH_START) && (wc1 <= UTF16_HIGH_END)) ||
            ((wc1 >= UTF16_LOW_START) && (wc1 <= UTF16_LOW_END)))
        {
            src_utf16 += 2;
        }
        else
        {
            /* normal case */
            dest_ucs2[0] = wc1 & 0xFF;
            dest_ucs2[1] = wc1 >> 8;
            src_utf16 += 2;
            dest_ucs2 += 2;
            count += 2;
        }
    }

    dest_ucs2[0] = '\0';
    dest_ucs2[1] = '\0';

    *src_end_pos = (uint32_t)src_utf16;
	
    return (count + 2);
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_text_to_ucs2_str_ex
 * DESCRIPTION
 *  
 * PARAMETERS
 *  dest            [OUT]       
 *  dest_size       [IN]        
 *  src             [IN]        
 *  encoding        [IN]        
 *  src_end_pos     [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
uint16_t mmi_chset_text_to_ucs2_str_ex(
            uint8_t *dest,
            int32_t dest_size,
            uint8_t *src,
            ENUM_ENCODING_TYPE encoding,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    const key_index_t *key_LSB_index_table = g_chset_encode_to_ucs2_key_LSB_index[encoding];
    const uint8_t *key_MSB_table = g_chset_encode_to_ucs2_key_MSB[encoding];
#ifdef __MMI_CHSET_GB2312__
    const MSB_Mapping_Struct *gb2312_key_MSB_table = g_chset_gb2312_to_ucs2_key_MSB;
#endif
    const uint16_t *ucs2_table = g_chset_encode_to_ucs2_table[encoding];

    uint8_t key_LSB;
    uint8_t key_MSB;
    int16_t start;
    int16_t end;
    int16_t pos = 0;
    int16_t index;
    uint8_t *src_end = src + strlen((char*)src);

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (key_LSB_index_table == NULL || ucs2_table == NULL || (key_MSB_table == NULL
	#ifdef __MMI_CHSET_GB2312__
		 && gb2312_key_MSB_table == NULL
	#endif
		))
    {
        return 0;
    }
    
    while (*(src) != 0) /* I do not know how we assume that this is end of data.. Currently we are taking only chinese encodings for
                           the optimized algo so we can take it as 2 bytes */
    {
        if (*(src) <= 0x7f)
        {
            *(dest + pos) = *src;
            pos++;
            *(dest + pos) = 0;
            pos++;
            src += 1;
        }
        else
        {
			if (src_end - src <= 1)
            {
                /* can't encoding the character. The data may be segmented. */
                break;
            }
            key_LSB = (uint8_t)(*(src));
            start = key_LSB_index_table[key_LSB].start;

            if (start < 0)
            {

                dest[pos] = (uint8_t)g_chset_unknown_encode_char[encoding];
                dest[pos + 1] = (uint8_t)(g_chset_unknown_encode_char[encoding] >> 8);
                src += 1;
            }
            else
            {
                key_MSB = (uint8_t) (*(src + 1));
                end = key_LSB_index_table[key_LSB].end;
			#ifdef __MMI_CHSET_GB2312__
                if( GB2312_ENCODING_TYPE == encoding)
                {
                    index = mmi_chset_binary_search_MSB_mapping_table(key_MSB, gb2312_key_MSB_table, mmi_chset_gb2312_to_ucs2_MSB_size(), start, end);
                }
                else
			#endif
                {
                    index = mmi_chset_binary_search(key_MSB, key_MSB_table, start, end);
                }

                if (index < 0)  /* key MSB not found */
                {
                    dest[pos] = (uint8_t) g_chset_unknown_encode_char[encoding];
                    dest[pos + 1] = (uint8_t) (g_chset_unknown_encode_char[encoding] >> 8);
                }
                else
                {
                    dest[pos] = (uint8_t) ucs2_table[index];
                    dest[pos + 1] = (uint8_t) (ucs2_table[index] >> 8);
                }
                src += 2;
            }
            pos += 2;
        }
        if (pos >= dest_size - 2)
        {
            break;
        }
    }
    *src_end_pos = (uint32_t) src;
    dest[pos] = 0;
    dest[pos + 1] = 0;
    return pos + 2;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_text_str_ex
 * DESCRIPTION
 *  
 * PARAMETERS
 *  dest            [OUT]       
 *  dest_size       [IN]        
 *  src             [IN]        
 *  encoding        [IN]        
 *  src_end_pos     [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
uint16_t mmi_chset_ucs2_to_text_str_ex(
            uint8_t *dest,
            int32_t dest_size,
            uint8_t *src,
            ENUM_ENCODING_TYPE encoding,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    const key_index_t *key_LSB_index_table = g_chset_ucs2_to_encode_key_LSB_index[encoding];
    const uint8_t *key_MSB_table = g_chset_ucs2_to_encode_key_MSB[encoding];
    const uint16_t *encode_table = g_chset_ucs2_to_encode_table[encoding];
    uint8_t key_LSB;
    uint8_t key_MSB;
    int16_t start;
    int16_t end;
    int16_t index;
    int16_t pos = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (key_LSB_index_table == NULL || key_MSB_table == NULL || encode_table == NULL)
    {
        return 0;
    }
    
    while (*src || *(src + 1))
    {
        if (*src <= 0x7f && *(src + 1) == 0)
        {
            *(dest + pos) = *(src);
            pos++;
        }
        else
        {
            /*
             * The original conversion tables of mmi_chset_ucs2_to_text_str() seem to assume the src is big-endian,
             * but the src should be little-endian. We need to change the byte order.
             */
            key_LSB = (uint8_t) (*(src + 1));
            start = key_LSB_index_table[key_LSB].start;
            if (start < 0)
            {
                dest[pos] = (uint8_t) (g_chset_unknown_encode_char[encoding] >> 8);
                dest[pos + 1] = (uint8_t) g_chset_unknown_encode_char[encoding];
            }
            else
            {
                key_MSB = (uint8_t) (*(src));
                end = key_LSB_index_table[key_LSB].end;
                if ((index = mmi_chset_binary_search(key_MSB, key_MSB_table, start, end)) < 0)  /* key MSB not found */
                {
                    dest[pos] = (uint8_t) (g_chset_unknown_encode_char[encoding] >> 8);
                    dest[pos + 1] = (uint8_t) g_chset_unknown_encode_char[encoding];
                }
                else
                {
                    dest[pos] = (uint8_t) (encode_table[index] >> 8);
                    dest[pos + 1] = (uint8_t) encode_table[index];
                }
            }
            pos += 2;
        }
        src += 2;

        if (pos >= dest_size - 1)
        {
            break;
        }
    }
    *src_end_pos = (uint32_t) src;
    dest[pos] = 0;
    return pos + 1;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_encode_decode_algo2
 * DESCRIPTION
 *  
 * PARAMETERS
 *  Scheme          [IN]        
 *  pOutBuffer      [OUT]       
 *  pInBuffer       [IN]        
 *  nBuffsize       [IN]        
 *  src_end_pos     [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
static uint16_t mmi_chset_encode_decode_algo2(
                    ENUM_MMI_CHSET_PAIR Scheme,
                    uint8_t *pOutBuffer,
                    uint8_t *pInBuffer,
                    int32_t nBuffsize,
                    uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    switch (Scheme)
    {
    #ifdef __MMI_CHSET_GB2312__
        case GB2312_TO_UCS2:
            return mmi_chset_text_to_ucs2_str_ex(
                    (uint8_t*) pOutBuffer,
                    nBuffsize,
                    (uint8_t*) pInBuffer,
                    GB2312_ENCODING_TYPE,
                    src_end_pos);
        case UCS2_TO_GB2312:
            return mmi_chset_ucs2_to_text_str_ex(
                    (uint8_t*) pOutBuffer,
                    nBuffsize,
                    (uint8_t*) pInBuffer,
                    GB2312_ENCODING_TYPE,
                    src_end_pos);
	#endif
	#ifdef __MMI_CHSET_BIG5__
        case BIG5_TO_UCS2:
            return mmi_chset_text_to_ucs2_str_ex(
                    (uint8_t*) pOutBuffer,
                    nBuffsize,
                    (uint8_t*) pInBuffer,
                    BIG_5_ENCODING_TYPE,
                    src_end_pos);
        case UCS2_TO_BIG5:
            return mmi_chset_ucs2_to_text_str_ex(
                    (uint8_t*) pOutBuffer,
                    nBuffsize,
                    (uint8_t*) pInBuffer,
                    BIG_5_ENCODING_TYPE,
                    src_end_pos);
	#endif
	#ifdef __MMI_CHSET_HKSCS__
        case HKSCS_TO_UCS2:
            return mmi_chset_text_to_ucs2_str_ex(
                    (uint8_t*) pOutBuffer,
                    nBuffsize,
                    (uint8_t*) pInBuffer,
                    HKSCS_ENCODING_TYPE,
                    src_end_pos);
        case UCS2_TO_HKSCS:
            return mmi_chset_ucs2_to_text_str_ex(
                    (uint8_t*) pOutBuffer,
                    nBuffsize,
                    (uint8_t*) pInBuffer,
                    HKSCS_ENCODING_TYPE,
                    src_end_pos);
	#endif
        default:
            return 0;
    }
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_utf7_str
 * DESCRIPTION
 *  Encodes the MMI UCS Buffer to UTF7 Buffer for sending a mail
 * PARAMETERS
 *  g_encoded_buf       [OUT]       Encoded Data
 *  g_data_buf          [IN]        MMI Editor Data
 *  data_size           [IN]        
 * RETURNS
 *  Nothing
 *****************************************************************************/
int mmi_chset_ucs2_to_utf7_str(uint8_t *g_encoded_buf, uint8_t *g_data_buf, int data_size, uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t temp_char[3];
    unsigned short *current_char, prev_char = 0;
    int output_len = 0, ucs2_chars_count = 0, ctr = 0, encode_flag = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    data_size -= 2;
    prev_char = 0;

    while(!(g_data_buf[ctr] == NULL && g_data_buf[ctr + 1] == NULL))   /* ctr < len * 2 ) */
    {
        temp_char[0] = g_data_buf[ctr];
        temp_char[1] = g_data_buf[ctr + 1];
        temp_char[2] = 0;

        current_char = (unsigned short*)temp_char;

        if(temp_char[0] == '+')
        {
            if(encode_flag == 1)
            {
                encode_flag = 0;
                if(ucs2_chars_count)
                {
                    mmi_ucs2_utf_chars(
                        g_encoded_buf,
                        &output_len,
                        *current_char,
                        prev_char,
                        ucs2_chars_count,
                        encode_flag,
                        data_size);
                }
                g_encoded_buf[output_len++] = '-';
            }

            if (output_len < data_size - 1)
            {
                g_encoded_buf[output_len++] = '+';
                g_encoded_buf[output_len++] = '-';
            }
            else
            {
                break;
            }
            ucs2_chars_count = 0;
        }
        else if(direct_chars_ucs2_to_utf(*current_char))
        {
            /*
             * If Unicode value is less than 0x7F, 
             * leaving "Modified Base 64" encoding and encode char directly.
             */
            if (encode_flag == 1)
            {
                encode_flag = 0;
                if (ucs2_chars_count)
                {
                    mmi_ucs2_utf_chars(
                        g_encoded_buf,
                        &output_len,
                        *current_char,
                        prev_char,
                        ucs2_chars_count,
                        encode_flag,
                        data_size);
                }
                g_encoded_buf[output_len++] = '-';
            }
            if (output_len < data_size)
            {
                g_encoded_buf[output_len++] = (uint8_t)(*current_char);
            }
            else
            {
                break;
            }

            ucs2_chars_count = 0;
        }
        else
        {
            /*
             * If Unicode value is larger than 0x7F, 
             * entering "Modified Base 64" encoding.
             */
            if (encode_flag == 0)
            {
                g_encoded_buf[output_len++] = '+';
                encode_flag = 1;
            }

            /*
             * Encode UTF7 chars from Unicode
             * Every Unicode is 16 bits (2 bytes)
             * Every UTF7 char would be transfered from 6 bits according "Modified Base 64"
             * One Unicode will encode three UTF7 chars in worst case (padding bits in 0).
             * There are three cases:
             * Case 1: (No previous Unicode)
             *     1 Unicode encodes 2 UTF7 chars, and remains 4 bits
             *     - ucs2_chars_count: 0
             * Case 2: (Previous Unicode remains 4 bits)
             *     Previous Unicode(4 bits) and 1 Unicode encode 3 UTF7 chars, and remain 2 bits
             *     - ucs2_chars_count: 1
             * Case 3: (Previous Unicode remains 2 bits)
             *     Previous Unicode(2 bits) and 1 Unicode encode 3 UTF7 chars, and no remained bits
             *     - ucs2_chars_count: 2
             */
            mmi_ucs2_utf_chars(
                g_encoded_buf,
                &output_len,
                *current_char,
                prev_char,
                ucs2_chars_count,
                encode_flag,
                data_size);
            ucs2_chars_count++;
            if (ucs2_chars_count == 3)
            {
                ucs2_chars_count = 0;
            }
        }
        ctr += 2;
        prev_char = *current_char;
    }

    *src_end_pos = (uint32_t)(&g_data_buf[ctr]);

    g_encoded_buf[output_len] = 0;
    return output_len + 1;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_utf7_to_ucs2_str
 * DESCRIPTION
 *  decodes UTF7 Buffer to MMI UCS Buffer for after recieving a mail.
 *  refer RFC 2152 to get UTF7 format
 * PARAMETERS
 *  g_unicode_decoded_buf       [OUT]       Decoded Data
 *  g_encoded_buf               [IN]        This is UTF7 encoded buffer to be converted
 *  data_size                   [IN]        Size of destination buffer (bytes)
 * RETURNS
 *  length of decoded buffer
 *****************************************************************************/
int mmi_chset_utf7_to_ucs2_str(uint8_t *g_unicode_decoded_buf, uint8_t *g_encoded_buf, int data_size, uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int ctr = 0, output_len = 0, temp_ctr = 0, no_decoded_combinations = 0;
    uint8_t current_encoded_buf[3] = {0}, prev_last_char = 0, ch;
    int g_plus_indicator_flag = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while(g_encoded_buf[ctr] != 0 && output_len < data_size - 2)
    {
        ch = g_encoded_buf[ctr];
        /*
         * When the char is '+', begin to decode UTF7 according "Modified Base 64" (REC 2045).
         * When the char is '-', end of decoding UTF7 according "Modified Base 64".
         */
        if ((ch != '+') && (g_plus_indicator_flag == 0))
        {
            /* the octet value should be less than 0x7F */
            g_unicode_decoded_buf[output_len++] = ch;
            g_unicode_decoded_buf[output_len++] = 0;
        }
        else
        {
            char temp_buff = 0; /* For taking care for the unicode + character */

            if ((ch != '-') && (g_plus_indicator_flag == 1))
            {
                current_encoded_buf[temp_ctr++] = ch;
            }

            if ((temp_ctr == 3 && no_decoded_combinations != 2) || (temp_ctr == 2 && no_decoded_combinations == 2))
            {
                /*
                 * Every UTF7 char would be transfered to 6 bits according "Modified Base 64"
                 * Every Unicode is 16 bits (2 bytes)
                 * Three UTF7 chars will encode one Unicode in worst case.
                 * There are three cases:
                 * Case 1: (No previous char)
                 *     Need 3 UTF7 chars encode 1 Unicode, and remain 2 bits
                 *     - no_decoded_combinations: 0
                 * Case 2: (Previous char remains 2 bits, and still need 3 UTF7 chars)
                 *     Need more 3 UTF7 chars to encode 1 Unicode, and remain 4 bits
                 *     - no_decoded_combinations: 1
                 * Case 3: (Previous char remains 4 bits, and only need 2 UTF7 chars)
                 *     Need more 2 UTF7 chars to encode 1 Unicode. (no remained bits)
                 *     - no_decoded_combinations: 2
                 */
                mmi_utf_ucs2_chars(
                    g_unicode_decoded_buf,
                    current_encoded_buf,
                    prev_last_char,
                    no_decoded_combinations,
                    &output_len);
                no_decoded_combinations++;
                prev_last_char = (no_decoded_combinations) ? current_encoded_buf[2] : NULL;
                if (no_decoded_combinations == 3)
                {
                    no_decoded_combinations = 0;
                }
                current_encoded_buf[0] = 0;
                current_encoded_buf[1] = 0;
                current_encoded_buf[2] = 0;
                temp_ctr = 0;
            }

            g_plus_indicator_flag = 1;

            if ((ch == '-') && (temp_ctr != 0))
            {
                mmi_utf_ucs2_chars(
                    g_unicode_decoded_buf,
                    current_encoded_buf,
                    prev_last_char,
                    no_decoded_combinations % 3,
                    &output_len);
                /* handle this condition in decoder where chars less then 3 */
                current_encoded_buf[0] = 0;
                current_encoded_buf[1] = 0;
                current_encoded_buf[2] = 0;

                prev_last_char = 0;
                no_decoded_combinations = 0;
                g_plus_indicator_flag = 0;
                temp_ctr = 0;
            }
            else if ((ch == '-') && (temp_ctr == 0))
            {
                current_encoded_buf[0] = 0;
                current_encoded_buf[1] = 0;
                current_encoded_buf[2] = 0;

                prev_last_char = 0;
                no_decoded_combinations = 0;
                g_plus_indicator_flag = 0;
                temp_ctr = 0;
            }
            temp_buff = g_encoded_buf[ctr + 1];
            if (ch == '+' && temp_buff == '-')  /* Fot taking care for the unicode '+' character */
            {
                g_unicode_decoded_buf[output_len++] = ch;
                g_unicode_decoded_buf[output_len++] = 0;
            }
        }
        ctr++;
    }

    *src_end_pos = (uint32_t)(&g_encoded_buf[ctr]);

    g_unicode_decoded_buf[output_len] = 0;
    g_unicode_decoded_buf[output_len + 1] = 0;
    return (2 * (output_len + 1));

}

#ifdef __MMI_CHSET_GB18030__
/*****************************************************************************
 * FUNCTION
 *  mmi_chset_gb18030_to_ucs2_2_byte_search
 * DESCRIPTION
 *  
 * PARAMETERS
 *  src         [IN]        
 *  dest        [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
static bool mmi_chset_gb18030_to_ucs2_2_byte_search(uint16_t src, uint16_t *dest)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t start, end, mid;
    uint16_t result;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    start = 0;
    end = mmi_chset_gb18030_ucs2_2byte_size() - 1;
    while (start <= end)
    {
        if ((end - start) == 1) /* to prevent missed-comaprison */
        {
            mid = end;
        }
        else
        {
            mid = (start + end) / 2;
        }
        result = g_chset_gb18030_to_ucs2_2_byte_tbl[mid].gb_code;
        if (src > result)
        {
            start = mid + 1;
        }
        else if (src < result)
        {
            end = mid - 1;
        }
        else    /* found */
        {
            *dest = g_chset_gb18030_to_ucs2_2_byte_tbl[mid].ucs2_code;
            return true;
        }
    }

    return false;
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_gb18030_to_ucs2_4_byte_search
 * DESCRIPTION
 *  
 * PARAMETERS
 *  src         [IN]        
 *  dest        [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
static bool mmi_chset_gb18030_to_ucs2_4_byte_search(uint32_t src, uint16_t *dest)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t start, end, mid;
    uint32_t result;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    start = 0;
    end = mmi_chset_gb18030_ucs2_4byte_size() - 1;
    while (start <= end)
    {
        if ((end - start) == 1) /* to prevent missed-comaprison */
        {
            mid = end;
        }
        else
        {
            mid = (start + end) / 2;
        }
        result =
            (uint32_t)((g_chset_gb18030_to_ucs2_4_byte_tbl[mid].gb_code_high << 16) +
                          (g_chset_gb18030_to_ucs2_4_byte_tbl[mid].gb_code_low));
        if (src > result)
        {
            start = mid + 1;
        }
        else if (src < result)
        {
            end = mid - 1;
        }
        else    /* found */
        {
            *dest = g_chset_gb18030_to_ucs2_4_byte_tbl[mid].ucs2_code;
            return true;
        }
    }

    return false;
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_gb18030_to_ucs2
 * DESCRIPTION
 *  
 * PARAMETERS
 *  pOutBuffer      [OUT]       
 *  pInBuffer       [IN]        
 *  inLen           [IN]        
 * RETURNS
 *  
 *****************************************************************************/
uint16_t mmi_chset_gb18030_to_ucs2(uint8_t *pOutBuffer, uint8_t *pInBuffer, int32_t inLen)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t c1, c2, c3, c4;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    /* pre-insert unknown character */
    pOutBuffer[0] = 0xff;
    pOutBuffer[1] = 0xff;

    c1 = pInBuffer[0];
    if (c1 < 0x80)  /* 1 byte */
    {
        pOutBuffer[0] = pInBuffer[0];
        pOutBuffer[1] = 0x00;
        return 1;   /* consume 1 byte */
    }

    if (inLen >= 2)
    {
        c2 = pInBuffer[1];
    }
    else
    {
        return 1;   /* consume 1 byte */
    }

    /* 2 bytes */
    if ((c1 >= 0x81 && c1 <= 0xfe) && ((c2 >= 0x40 && c2 <= 0x7e) || (c2 >= 0x80 && c2 <= 0xfe)))
    {
        uint16_t src, dest;

        src = (((c1 << 8) & 0xff00) + c2);

        /* binary search 2 bytes table */
        if (mmi_chset_gb18030_to_ucs2_2_byte_search(src, &dest))
        {
            pOutBuffer[0] = (uint8_t)(dest & 0xff);
            pOutBuffer[1] = (uint8_t)(dest >> 8);
            return 2;
        }
        else
        {
            return 1;   /* consume 1 byte */
        }
    }

    if (inLen >= 4) /* 4 bytes */
    {
        c3 = pInBuffer[2];
        c4 = pInBuffer[3];

        if (c1 >= 0x81 && c2 <= 0xfe)
            if (c2 >= 0x30 && c2 <= 0x39)
                if (c3 >= 0x81 && c3 <= 0xfe)
                    if (c4 >= 0x30 && c4 <= 0x39)
                    {
                        uint32_t src;
                        uint16_t dest;

                        /* binary search 4 bytes table */
                        src = (uint32_t) ((c1 << 24) + (c2 << 16) + (c3 << 8) + c4);

                        if (mmi_chset_gb18030_to_ucs2_4_byte_search(src, &dest))
                        {
                            pOutBuffer[0] = (uint8_t)(dest & 0xff);
                            pOutBuffer[1] = (uint8_t)(dest >> 8);
                            return 4;
                        }
                        else
                        {
                            return 1;   /* consume 1 byte */
                        }
                    }
    }

    return 1;
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_gb18030_to_ucs2_str
 * DESCRIPTION
 *  
 * PARAMETERS
 *  pOutBuffer      [OUT]       
 *  pInBuffer       [IN]        
 *  dest_size       [IN]        
 *  src_end_pos     [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
static uint16_t mmi_chset_gb18030_to_ucs2_str(
                    uint8_t *pOutBuffer,
                    uint8_t *pInBuffer,
                    int32_t dest_size,
                    uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t *in_cursor = pInBuffer;
    uint8_t *out_cursor = pOutBuffer;
    uint8_t tmp_result[2];

    uint16_t diget_len;
    int32_t in_len = strlen((char*)pInBuffer);
    int32_t out_len = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (in_len && dest_size > (out_len + 2))
    {
        diget_len = mmi_chset_gb18030_to_ucs2((uint8_t*)&tmp_result, (uint8_t*)in_cursor, in_len);
        if (tmp_result[0] == 0xFF && tmp_result[1] == 0xFF && in_len == diget_len)
        {
            /* the last remained data of pInBuffer may be segmented */
            break;
        }
        in_len -= diget_len;
        in_cursor += diget_len;

        out_cursor[0] = tmp_result[0];
        out_cursor[1] = tmp_result[1];
        out_cursor += 2;
        out_len += 2;
    }
    *src_end_pos = (uint32_t) in_cursor;

    pOutBuffer[out_len] = 0;
    pOutBuffer[++out_len] = 0;
    return (out_len + 1);
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_gb18030_to_ucs2_str_with_src_length
 * DESCRIPTION
 *  
 * PARAMETERS
 *  pOutBuffer      [OUT]       
 *  pInBuffer       [IN]        
 *  dest_size       [IN]        
 *  in_len          [IN]        
 * RETURNS
 *  
 *****************************************************************************/
static uint16_t mmi_chset_gb18030_to_ucs2_str_with_src_length(
                    uint8_t *pOutBuffer,
                    uint8_t *pInBuffer,
                    int32_t dest_size,
                    int32_t in_len)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t *in_cursor = pInBuffer;
    uint8_t *out_cursor = pOutBuffer;
    uint8_t tmp_result[2];

    uint16_t diget_len;
    int32_t out_len = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (in_len && dest_size > (out_len + 2))
    {
        diget_len = mmi_chset_gb18030_to_ucs2((uint8_t*)&tmp_result, (uint8_t*)in_cursor, in_len);
        in_len -= diget_len;
        in_cursor += diget_len;

        out_cursor[0] = tmp_result[0];
        out_cursor[1] = tmp_result[1];
        out_cursor += 2;
        out_len += 2;
    }

    return (out_len);
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_gb18030_2_byte_search
 * DESCRIPTION
 *  
 * PARAMETERS
 *  src         [IN]        
 *  dest        [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
static bool mmi_chset_ucs2_to_gb18030_2_byte_search(uint16_t src, uint32_t *dest)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t start, end, mid;
    uint16_t result;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    start = 0;
    end = mmi_chset_ucs2_gb18030_2byte_size() - 1;
    while (start <= end)
    {
        if ((end - start) == 1) /* to prevent missed-comaprison */
        {
            mid = end;
        }
        else
        {
            mid = (start + end) / 2;
        }
        result = g_chset_ucs2_to_gb18030_2_byte_tbl[mid].ucs2_code;
        if (src > result)
        {
            start = mid + 1;
        }
        else if (src < result)
        {
            end = mid - 1;
        }
        else    /* found */
        {
            *dest = g_chset_ucs2_to_gb18030_2_byte_tbl[mid].gb_code;
            return true;
        }
    }

    return false;

}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_gb18030_4_byte_search
 * DESCRIPTION
 *  
 * PARAMETERS
 *  src         [IN]        
 *  dest        [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
static bool mmi_chset_ucs2_to_gb18030_4_byte_search(uint16_t src, uint32_t *dest)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t start, end, mid;
    uint16_t result;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    start = 0;
    end = mmi_chset_gb18030_ucs2_4byte_size() - 1;
    while (start <= end)
    {
        if ((end - start) == 1) /* to prevent missed-comaprison */
        {
            mid = end;
        }
        else
        {
            mid = (start + end) / 2;
        }
        result = g_chset_gb18030_to_ucs2_4_byte_tbl[mid].ucs2_code;
        if (src > result)
        {
            start = mid + 1;
        }
        else if (src < result)
        {
            end = mid - 1;
        }
        else    /* found */
        {
            *dest =
                (uint32_t)((g_chset_gb18030_to_ucs2_4_byte_tbl[mid].gb_code_high << 16) +
                              g_chset_gb18030_to_ucs2_4_byte_tbl[mid].gb_code_low);
            return true;
        }
    }

    return false;
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_gb18030
 * DESCRIPTION
 *  
 * PARAMETERS
 *  pOutBuffer      [OUT]       
 *  pInBuffer       [IN]        
 * RETURNS
 *  
 *****************************************************************************/
uint16_t mmi_chset_ucs2_to_gb18030(uint8_t *pOutBuffer, uint8_t *pInBuffer)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint16_t ucs2_char;
    uint32_t gb_char;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    /* single byte */
    if (pInBuffer[1] == 0 && pInBuffer[0] < 0x80)
    {
        pOutBuffer[0] = pInBuffer[0];
        return 1;
    }

    ucs2_char = ((pInBuffer[1] << 8) + pInBuffer[0]);
    /* search 2 bytes table */
    if (mmi_chset_ucs2_to_gb18030_2_byte_search(ucs2_char, &gb_char))
    {
        pOutBuffer[0] = (uint8_t)((gb_char >> 8) & 0xff);
        pOutBuffer[1] = (uint8_t)(gb_char & 0xff);
        return 2;
    }

    /* search 4 bytes table */
    if (mmi_chset_ucs2_to_gb18030_4_byte_search(ucs2_char, &gb_char))
    {
        pOutBuffer[0] = (uint8_t)((gb_char >> 24) & 0xff);
        pOutBuffer[1] = (uint8_t)((gb_char >> 16) & 0xff);
        pOutBuffer[2] = (uint8_t)((gb_char >> 8) & 0xff);
        pOutBuffer[3] = (uint8_t)(gb_char & 0xff);
        return 4;
    }

    /* unknown character */
    pOutBuffer[0] = 0xff;
    return 1;
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_gb18030_str
 * DESCRIPTION
 *  
 * PARAMETERS
 *  pOutBuffer      [OUT]       
 *  pInBuffer       [IN]        
 *  dest_size       [IN]        
 *  src_end_pos     [OUT]       
 * RETURNS
 *  
 *****************************************************************************/
static uint16_t mmi_chset_ucs2_to_gb18030_str(
                    uint8_t *pOutBuffer,
                    uint8_t *pInBuffer,
                    int32_t dest_size,
                    uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t *in_cursor = pInBuffer;
    uint8_t *out_cursor = pOutBuffer;
    uint8_t tmp_result[4];

    int32_t out_len = 0;
    int32_t i;
    uint16_t result_len;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while ((in_cursor[0] != 0) || (in_cursor[1] != 0) && (dest_size > (out_len + 2)))
    {
        result_len = mmi_chset_ucs2_to_gb18030((unsigned char*)&tmp_result, (unsigned char*)in_cursor);

        for (i = (result_len - 1); i >= 0; i--)
        {
            out_cursor[i] = tmp_result[i];
        }

        in_cursor += 2;
        out_cursor += result_len;
        out_len += result_len;
    }
    *src_end_pos = (uint32_t) in_cursor;

    pOutBuffer[out_len] = 0;
    pOutBuffer[++out_len] = 0;
    return (out_len + 1);
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_gb18030_str_with_src_length
 * DESCRIPTION
 *  
 * PARAMETERS
 *  pOutBuffer      [OUT]       
 *  pInBuffer       [IN]        
 *  dest_size       [IN]        
 *  src_size        [IN]        
 * RETURNS
 *  
 *****************************************************************************/
static uint16_t mmi_chset_ucs2_to_gb18030_str_with_src_length(
                    uint8_t *pOutBuffer,
                    uint8_t *pInBuffer,
                    int32_t dest_size,
                    int32_t src_size)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t *in_cursor = pInBuffer;
    uint8_t *out_cursor = pOutBuffer;
    uint8_t tmp_result[4];

    int32_t out_len = 0;
    int32_t i;
    uint16_t result_len;
    int32_t src_index = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (src_index < src_size && (dest_size > (out_len + 2)))
    {
        result_len = mmi_chset_ucs2_to_gb18030((unsigned char*)&tmp_result, (unsigned char*)in_cursor);

        for (i = (result_len - 1); i >= 0; i--)
        {
            out_cursor[i] = tmp_result[i];
        }

        in_cursor += 2;
        out_cursor += result_len;
        out_len += result_len;
        src_index += 2;
    }

    return (out_len);
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_convert_with_src_length
 * DESCRIPTION
 *  
 * PARAMETERS
 *  src_type        [IN]        
 *  dest_type       [IN]        
 *  src_buff        [IN]        
 *  src_size        [IN]        
 *  dest_buff       [OUT]       
 *  dest_size       [IN]        
 * RETURNS
 *  
 *****************************************************************************/
int32_t mmi_chset_convert_with_src_length(
            ENUM_MMI_CHSET src_type,
            ENUM_MMI_CHSET dest_type,
            char *src_buff,
            int32_t src_size,
            char *dest_buff,
            int32_t dest_size)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (src_type == MMI_CHSET_GB18030 && dest_type == MMI_CHSET_UCS2)
    {
        return mmi_chset_gb18030_to_ucs2_str_with_src_length((uint8_t*)dest_buff, (uint8_t*)src_buff, dest_size, src_size);
    }
    else if (src_type == MMI_CHSET_UCS2 && dest_type == MMI_CHSET_GB18030)
    {
        return mmi_chset_ucs2_to_gb18030_str_with_src_length((uint8_t*)dest_buff, (uint8_t*)src_buff, dest_size, src_size);
    }
    /* only support GB18030 for now */
    return 0;

}
#endif /* __MMI_CHSET_GB18030__ */ 


#ifdef __MMI_CHSET_SJIS__
/*****************************************************************************
 * FUNCTION
 *  mmi_chset_sjis_to_ucs2
 * DESCRIPTION
 *  Convert the Shift JIS string to UCS2
 * PARAMETERS
 *  dest            [OUT]       dest buffer
 *  dest_size       [IN]        dest buffer size
 *  src             [IN]        source buffer
 *  src_end_pos     [OUT]       source buffer end position
 * RETURNS
 *  Converted byte number
 *****************************************************************************/
uint32_t mmi_chset_sjis_to_ucs2(
            uint8_t *dest,
            int32_t dest_size,
            uint8_t *src,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    const key_index_t *key_LSB_index_table = g_chset_sjis_to_ucs2_key_LSB_index;
    const uint8_t *key_MSB_table = g_chset_sjis_to_ucs2_key_MSB;
    const uint16_t *ucs2_table = g_chset_sjis_to_ucs2_table;

    uint8_t key_LSB;
    uint8_t key_MSB;
    int16_t start;
    int16_t end;
    uint32_t pos = 0;
    int16_t index;
    uint8_t *src_end = src + strlen((char*)src);

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (key_LSB_index_table == NULL || key_MSB_table == NULL || ucs2_table == NULL)
    {
        return 0;
    }

    while (*(src) != 0)
    {
        if (*(src) <= 0x7f)
        {
            *(dest + pos) = *src;
            pos++;
            *(dest + pos) = 0;
            pos++;
            src += 1;
        }
        else if ((*(src) >= 0xA1) && (*(src) <= 0xDF))
        {
            *(dest + pos) = 0x61 + (*(src) - 0xA1);
            pos++;
            *(dest + pos) = 0xFF;
            pos++;
            src += 1;
        }
        else
        {
            key_LSB = (uint8_t)(*(src + 1));
            start = key_LSB_index_table[key_LSB].start;

            if (start < 0)
            {
                if (src_end - src <= 1)
                {
                    /* can't encoding the character. The data may be segmented. */
                    break;
                }
                else
                {
                    dest[pos] = (uint8_t)0xFF;
                    dest[pos + 1] = (uint8_t)0xFF;
                }
            }
            else
            {
                key_MSB = (uint8_t) (*(src));
                end = key_LSB_index_table[key_LSB].end;
                if ((index = mmi_chset_binary_search(key_MSB, key_MSB_table, start, end)) < 0)  /* key MSB not found */
                {
                    if (src_end - src <= 1)
                    {
                        /* can't encoding the character. The data may be segmented. */
                        break;
                    }
                    else
                    {
                        dest[pos] = (uint8_t)0xFF;
                        dest[pos + 1] = (uint8_t)0xFF;
                    }
                }
                else
                {
                    dest[pos] = (uint8_t) ucs2_table[index];
                    dest[pos + 1] = (uint8_t) (ucs2_table[index] >> 8);

                }
            }
            src += 2;
            pos += 2;
        }
        if (pos >= dest_size - 2)
        {
            break;
        }
    }
    *src_end_pos = (uint32_t)src;
    dest[pos] = 0;
    dest[pos + 1] = 0;
    return pos + 2;
}


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_ucs2_to_sjis
 * DESCRIPTION
 *  Convert the UCS2 string to Shift JIS
 * PARAMETERS
 *  dest            [OUT]       dest buffer
 *  dest_size       [IN]        dest buffer size
 *  src             [IN]        source buffer
 *  src_end_pos     [OUT]       source buffer end position
 * RETURNS
 *  Converted byte number
 *****************************************************************************/
uint32_t mmi_chset_ucs2_to_sjis(
            uint8_t *dest,
            int32_t dest_size,
            uint8_t *src,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    const key_index_t *key_LSB_index_table = g_chset_ucs2_to_sjis_key_LSB_index;
    const uint8_t *key_MSB_table = g_chset_ucs2_to_sjis_key_MSB;
    const uint16_t *encode_table = g_chset_ucs2_to_sjis_table;
    uint8_t key_LSB;
    uint8_t key_MSB;
    int16_t start;
    int16_t end;
    int16_t index;
    uint32_t pos = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (key_LSB_index_table == NULL || key_MSB_table == NULL || encode_table == NULL)
    {
        return 0;
    }

    while (*src || *(src + 1))
    {
        if (*src <= 0x7f && *(src + 1) == 0)
        {
            *(dest + pos) = *(src);
            pos++;
        }
        else if ((*(src + 1) == 0xFF) && ((*src >= 0x61) && (*src <= 0x9F)))
        {
            *(dest + pos) = 0xA1 + (*(src) - 0x61);
            pos++;
        }
        else
        {
            key_LSB = (uint8_t)(*(src));
            start = key_LSB_index_table[key_LSB].start;
            if (start < 0)
            {
                dest[pos] = (uint8_t)0xFF;
                dest[pos + 1] = (uint8_t)0xFF;
            }
            else
            {
                key_MSB = (uint8_t)(*(src + 1));
                end = key_LSB_index_table[key_LSB].end;
                if ((index = mmi_chset_binary_search(key_MSB, key_MSB_table, start, end)) < 0)  /* key MSB not found */
                {
                    dest[pos] = (uint8_t)0xFF;
                    dest[pos + 1] = (uint8_t)0xFF;
                }
                else
                {
                    dest[pos] = (uint8_t) (encode_table[index] >> 8);
                    dest[pos + 1] = (uint8_t) encode_table[index];
                }

            }
            pos += 2;
        }
        src += 2;

        if (pos >= dest_size - 2)
        {
            break;
        }
    }
    *src_end_pos = (uint32_t)src;
    dest[pos] = 0;
    dest[pos + 1] = 0;
    return pos + 2;
}
#endif /* __MMI_CHSET_SJIS__ */


/*****************************************************************************
 * FUNCTION
 *  mmi_chset_convert_to_native_ucs2
 * DESCRIPTION
 *  Convert string to given character set from native (LE) UCS2 . (will add the terminate character)
 * PARAMETERS
 *  dest_type       [IN]        Charset type of destination
 *  src_buff        [IN]        Buffer stores source string
 *  dest_buff       [OUT]       Buffer stores destination string
 *  dest_size       [IN]        Size of destination buffer (bytes)
 *  src_end_pos     [OUT]       
 * RETURNS
 *  Length of destination string, including null terminator. (bytes)
 *****************************************************************************/
static int32_t mmi_chset_convert_from_native_ucs2(
            ENUM_MMI_CHSET dest_type,
            uint8_t *src_buff,
            uint8_t *dest_buff,
            int32_t dest_size,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t result = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    switch(dest_type)
    {
    case MMI_CHSET_ASCII:            
        result = mmi_chset_ucs2_to_ascii(dest_buff, src_buff, dest_size, src_end_pos);                          
        break;
    case MMI_CHSET_UTF8: 
        result = mmi_chset_ucs2_to_utf8_string_ex(
            (uint8_t*) dest_buff,
            dest_size,
            (uint8_t*) src_buff,
            src_end_pos);
        break;	
    case MMI_CHSET_UTF16LE:
        result = mmi_chset_ucs2_to_utf16_string(
            dest_buff,
            dest_size,
            MMI_CHSET_UTF16LE,
            false,
            src_buff,
            src_end_pos);
        break;
    case MMI_CHSET_UTF16BE:
        result = mmi_chset_ucs2_to_utf16_string(
            dest_buff,
            dest_size,
            MMI_CHSET_UTF16BE,
            false,
            src_buff,
            src_end_pos);
        break;       

#ifdef __MMI_CHSET_BIG5__ 
    case MMI_CHSET_BIG5:
        result = mmi_chset_encode_decode_algo2(UCS2_TO_BIG5, dest_buff, src_buff, dest_size, src_end_pos);    /* Length is hardcode for now to maximum */
        break;
#endif

#ifdef __MMI_CHSET_GB2312__
    case MMI_CHSET_GB2312:            
        result = mmi_chset_encode_decode_algo2(UCS2_TO_GB2312, dest_buff, src_buff, dest_size, src_end_pos);    /* Length is hardcode for now to maximum */
        break;
#endif

#ifdef __MMI_CHSET_HKSCS__
    case MMI_CHSET_HKSCS:            
        result = mmi_chset_encode_decode_algo2(UCS2_TO_HKSCS, dest_buff, src_buff, dest_size, src_end_pos);    /* Length is hardcode for now to maximum */
        break;
#endif

#ifdef __MMI_CHSET_UTF7__
    case MMI_CHSET_UTF7:
        result = mmi_chset_ucs2_to_utf7_str(dest_buff, src_buff, dest_size, src_end_pos);
        break;       
#endif

#ifdef __MMI_CHSET_GB18030__
    case MMI_CHSET_GB18030:
        result = mmi_chset_ucs2_to_gb18030_str(dest_buff, src_buff, dest_size, src_end_pos);           
        break;          
#endif

#ifdef __MMI_CHSET_SJIS__
    case MMI_CHSET_SJIS:
        result = mmi_chset_ucs2_to_sjis((uint8_t *)dest_buff, dest_size, (uint8_t *)src_buff, src_end_pos);           
        break;       
#endif

    default:            
        result = 0;
        break;
    }
	
    return result;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_convert_to_native_ucs2
 * DESCRIPTION
 *  Convert string from 1 character set to native (LE) UCS2 . (will add the terminate character)
 * PARAMETERS
 *  src_type        [IN]        Charset type of source
 *  src_buff        [IN]        Buffer stores source string
 *  dest_buff       [OUT]       Buffer stores destination string
 *  dest_size       [IN]        Size of destination buffer (bytes)
 *  src_end_pos     [OUT]       
 * RETURNS
 *  Length of destination string, including null terminator. (bytes)
 *****************************************************************************/
static int32_t mmi_chset_convert_to_native_ucs2(
            ENUM_MMI_CHSET src_type,
            uint8_t *src_buff,
            uint8_t *dest_buff,
            int32_t dest_size,
            uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int32_t result;
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    switch(src_type)
    {

    case MMI_CHSET_ASCII:
        result = mmi_chset_ascii_to_ucs2(dest_buff, src_buff, dest_size, src_end_pos);
        break;

    case MMI_CHSET_UTF8:            
        result = mmi_chset_utf8_to_ucs2_string_ex(
            dest_buff,
            dest_size,
            src_buff,
            src_end_pos);	
        break;

    case MMI_CHSET_UTF16LE:
        result = mmi_chset_utf16_to_ucs2_string(
            dest_buff,
            dest_size,
            src_buff,
            MMI_CHSET_UTF16LE,
            src_end_pos);
        break;

    case MMI_CHSET_UTF16BE:
        result = mmi_chset_utf16_to_ucs2_string(
            dest_buff,
            dest_size,
            src_buff,
            MMI_CHSET_UTF16BE,
            src_end_pos);
        break;

#ifdef __MMI_CHSET_BIG5__            
	case MMI_CHSET_BIG5:
		result = mmi_chset_encode_decode_algo2(BIG5_TO_UCS2, dest_buff, src_buff, dest_size, src_end_pos);	  /* Length is hardcode for now to maximum */
		break;
#endif 

#ifdef __MMI_CHSET_GB2312__
    case MMI_CHSET_GB2312:            
        result = mmi_chset_encode_decode_algo2(GB2312_TO_UCS2, dest_buff, src_buff, dest_size, src_end_pos);    /* Length is hardcode for now to maximum */
        break;
#endif

#ifdef __MMI_CHSET_HKSCS__            
    case MMI_CHSET_HKSCS:            
        result = mmi_chset_encode_decode_algo2(HKSCS_TO_UCS2, dest_buff, src_buff, dest_size, src_end_pos);    /* Length is hardcode for now to maximum */
        break;
#endif

#ifdef __MMI_CHSET_UTF7__
    case MMI_CHSET_UTF7:
        result = mmi_chset_utf7_to_ucs2_str(dest_buff, src_buff, dest_size, src_end_pos);
        break;       
#endif

#ifdef __MMI_CHSET_GB18030__
    case MMI_CHSET_GB18030:
        result = mmi_chset_gb18030_to_ucs2_str(dest_buff, src_buff, dest_size, src_end_pos);
        break;          
#endif

#ifdef __MMI_CHSET_SJIS__
    case MMI_CHSET_SJIS:
        result = mmi_chset_sjis_to_ucs2((uint8_t *)dest_buff, dest_size, (uint8_t *)src_buff, src_end_pos);
        break;       
#endif

        /* For all rest of the encodings. */                
    default:            
        result = 0;
		break;
    }
	
    return result;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_convert_ex
 * DESCRIPTION
 *  Convert string between 2 character sets. (will add the terminate character)
 * PARAMETERS
 *  src_type        [IN]        Charset type of source
 *  dest_type       [IN]        Charset type of destination
 *  src_buff        [IN]        Buffer stores source string
 *  dest_buff       [OUT]       Buffer stores destination string
 *  dest_size       [IN]        Size of destination buffer (bytes)
 *  src_end_pos     [OUT]       
 * RETURNS
 *  Length of destination string, including null terminator. (bytes)
 *****************************************************************************/
int32_t mmi_chset_convert_ex(
                               ENUM_MMI_CHSET src_type,
                               ENUM_MMI_CHSET dest_type,
                               char *src_buff,
                               char *dest_buff,
                               int32_t dest_size,
                               uint32_t *src_end_pos)
{
    /*----------------------------------------------------------------*/
    /* Local Variables												  */
    /*----------------------------------------------------------------*/
    /* typecast buffer to safer types */
    uint8_t *src_buff_int = (uint8_t *)src_buff; 
    uint8_t *dest_buff_int = (uint8_t *)dest_buff;
    /*----------------------------------------------------------------*/
    /* Code Body													  */
    /*----------------------------------------------------------------*/

    *src_end_pos = (uint32_t)src_buff_int;

    /* unsupported charset */
    if(src_type >= MMI_CHSET_TOTAL || dest_type >= MMI_CHSET_TOTAL)
    {
        return 0;
    }

    if(src_type == dest_type)
    {
        return mmi_chset_copy_to_dest(src_type, src_buff_int, dest_buff_int, dest_size, src_end_pos);
    }

    if(g_chset_state[src_type] == 0) /* If source type is disabled, but is 8 bit based simple code page*/
    {
        if (src_type <= MMI_CHSET_WESTERN_WIN)
        {
            return mmi_chset_simple_convert(dest_buff_int, dest_size, src_buff_int, src_end_pos); 		
        }
        else
        {
            return 0;
        }
    }

    if (g_chset_state[dest_type] == 0)
    {
        return 0;
    }

    if(src_type == MMI_CHSET_UCS2 )
    {
        return mmi_chset_convert_from_native_ucs2(dest_type, src_buff_int, dest_buff_int, dest_size, src_end_pos );
    }
    else if(dest_type == MMI_CHSET_UCS2 )
    {
        return mmi_chset_convert_to_native_ucs2(src_type, src_buff_int, dest_buff_int, dest_size, src_end_pos );									 
    }					
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_convert
 * DESCRIPTION
 *  Convert string between 2 character sets. (will add the terminate character)
 * PARAMETERS
 *  src_type        [IN]        Charset type of source
 *  dest_type       [IN]        Charset type of destination
 *  src_buff        [IN]        Buffer stores source string
 *  dest_buff       [OUT]       Buffer stores destination string
 *  dest_size       [IN]        Size of destination buffer (bytes)
 * RETURNS
 *  Length of destination string, including null terminator. (bytes)
 *****************************************************************************/
int32_t mmi_chset_convert(
            ENUM_MMI_CHSET src_type,
            ENUM_MMI_CHSET dest_type,
            char *src_buff,
            char *dest_buff,
            int32_t dest_size)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint32_t src_end_pos = (uint32_t)src_buff;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    return mmi_chset_convert_ex(src_type, dest_type, src_buff, dest_buff, dest_size, &src_end_pos);
}

/*****************************************************************************
 * FUNCTION
 *  mmi_chset_init
 * DESCRIPTION
 *  Routine for initializing the related data structures of the various encoding types
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void mmi_chset_init(void)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
#ifdef __MMI_CHSET_BIG5__
	{
		static mmi_chset_info_struct big5_ucs2_tbl = {0};
		static mmi_chset_info_struct ucs2_big5_tbl = {0};
		
		big5_ucs2_tbl.Input_word_bits = CHSET_BIT_WORD_16;
		big5_ucs2_tbl.Output_word_bits = CHSET_BIT_WORD_16;
		big5_ucs2_tbl.UndefChar = 0xFF1F;
		g_chset_tbl[BIG5_TO_UCS2] = &big5_ucs2_tbl;
		
		ucs2_big5_tbl.Input_word_bits = CHSET_BIT_WORD_16;
		ucs2_big5_tbl.Output_word_bits = CHSET_BIT_WORD_16;
		ucs2_big5_tbl.UndefChar = 0xA148;
		g_chset_tbl[UCS2_TO_BIG5] = &ucs2_big5_tbl;
	}
#endif /* __MMI_CHSET_BIG5__ */ 

#if defined(__MMI_CHSET_GB2312__) || defined(__MMI_CHSET_HKSCS__)
	{
		static mmi_chset_info_struct chinese_common_ucs2_tbl = {0};
		static mmi_chset_info_struct ucs2_chinese_common_tbl = {0};
		
		chinese_common_ucs2_tbl.Input_word_bits = CHSET_BIT_WORD_16;
		chinese_common_ucs2_tbl.Output_word_bits = CHSET_BIT_WORD_16;
		chinese_common_ucs2_tbl.UndefChar = 0xFF1F;		
		
		ucs2_chinese_common_tbl.Input_word_bits = CHSET_BIT_WORD_16;
		ucs2_chinese_common_tbl.Output_word_bits = CHSET_BIT_WORD_16;
		ucs2_chinese_common_tbl.UndefChar = 0x233F;

#ifdef __MMI_CHSET_GB2312__
		g_chset_tbl[GB2312_TO_UCS2] = &chinese_common_ucs2_tbl;
		g_chset_tbl[UCS2_TO_GB2312] = &ucs2_chinese_common_tbl;
#endif
#ifdef __MMI_CHSET_HKSCS__
        g_chset_tbl[HKSCS_TO_UCS2] = &chinese_common_ucs2_tbl;
        g_chset_tbl[UCS2_TO_HKSCS] = &ucs2_chinese_common_tbl;
#endif
	}
#endif /* __MMI_CHSET_GB2312__ || __MMI_CHSET_HKSCS__ */ 

#ifdef __MMI_CHSET_WESTERN_WIN__
	{
	    static mmi_chset_info_struct western_win_ucs2_tbl;
		static mmi_chset_info_struct ucs2_western_win_tbl;

		western_win_ucs2_tbl.pConversnTable = NULL;		
	    western_win_ucs2_tbl.pMappingTable = &g_chset_map_western_win_ucs2[0];
	    western_win_ucs2_tbl.TotalMapTbIndex = mmi_chset_map_western_win_ucs2_size();
		western_win_ucs2_tbl.Input_word_bits = CHSET_BIT_WORD_8;
		western_win_ucs2_tbl.Output_word_bits = CHSET_BIT_WORD_16;
		western_win_ucs2_tbl.UndefChar = 0x003F;
	    g_chset_tbl[WESTERN_WINDOWS_TO_UCS2] = &western_win_ucs2_tbl;


		ucs2_western_win_tbl.pConversnTable = NULL; 
	    ucs2_western_win_tbl.pMappingTable = &g_chset_map_ucs2_western_win[0];
	    ucs2_western_win_tbl.TotalMapTbIndex = mmi_chset_map_ucs2_western_win_size();
		ucs2_western_win_tbl.Input_word_bits = CHSET_BIT_WORD_16;
		ucs2_western_win_tbl.Output_word_bits = CHSET_BIT_WORD_8;
		ucs2_western_win_tbl.UndefChar = 0x3F;
		g_chset_tbl[UCS2_TO_WESTERN_WINDOWS] = &ucs2_western_win_tbl;
	}
#endif /* __MMI_CHSET_WESTERN_WIN__ */ 


    g_chset_tbl_is_init = true;
}

