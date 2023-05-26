/****************************************Copyright (c)************************************************
** File Name:			    ucs2.c
** Descriptions:			Operations related to Unicode coded strings source file
** Created By:				xie biao
** Created Date:			2020-12-29
** Modified Date:      		2020-12-29 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <nrfx.h>
#include "logger.h"

/*****************************************************************************
 * FUNCTION
 *  mmi_ucs2strlen
 * DESCRIPTION
 *  Gives the length of UCS2 encoded string
 * PARAMETERS
 *  arrOut      [IN]        array containing  UCS2 encoded characters
 * RETURNS
 *  S32-> Status
 *****************************************************************************/
int32_t mmi_ucs2strlen(const uint8_t *arrOut)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/
	int32_t nCount = 0;
	int32_t nLength = 0;

	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	/* Check for NULL character only at the odd no. of bytes 
	   assuming forst byte start from zero */
	if(arrOut)
	{
		while (arrOut[nCount] != 0 || arrOut[nCount + 1] != 0)
		{
			++nLength;
			nCount += 2;
		}
	}
	return nLength; /* One is added to count 0th byte */
}

/*****************************************************************************
 * FUNCTION
 *  mmi_ucs2cpy
 * DESCRIPTION
 *  copies the one UCS2 encoded string to other
 * PARAMETERS
 *  strDestination      [OUT]       StrDest-> Destination array
 *  strSource           [IN]        
 * RETURNS
 *  u8_t * -> pointer to destination string or NULL
 *****************************************************************************/
uint8_t *mmi_ucs2cpy(uint8_t *strDestination, const uint8_t *strSource)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint16_t count = 1;
    uint8_t *temp = strDestination;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (strSource == NULL)
    {
        if (strDestination)
        {
            *(strDestination + count - 1) = '\0';
            *(strDestination + count) = '\0';
        }
        return temp;

    }

    if (strDestination == NULL || strSource == NULL)
    {
        return NULL;
    }
    while (!((*(strSource + count) == 0) && (*(strSource + count - 1) == 0)))
    {

        *(strDestination + count - 1) = *(strSource + count - 1);
        *(strDestination + count) = *(strSource + count);
        count += 2;
    }

    *(strDestination + count - 1) = '\0';
    *(strDestination + count) = '\0';

    return temp;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_ucs2cat
 * DESCRIPTION
 *  
 *  
 *  User has to ensure that enough space is
 *  available in destination
 * PARAMETERS
 *  strDestination      [OUT]         
 *  strSource           [IN]        
 * RETURNS
 *  u8_t *
 *****************************************************************************/
uint8_t *mmi_ucs2cat(uint8_t *strDestination, const uint8_t *strSource)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/

	int8_t *dest = strDestination;

	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	dest = dest + mmi_ucs2strlen(strDestination) * 2;

	mmi_ucs2cpy(dest, strSource);
	return strDestination;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_ucs2cmp
 * DESCRIPTION
 *  compares two strings
 * PARAMETERS
 *  string1     [IN]        > first String
 *  string2     [OUT]       > Second String
 * RETURNS
 *  S32
 *****************************************************************************/
int32_t mmi_ucs2cmp(const uint8_t *string1, const uint8_t *string2)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/
	uint16_t nStr1;
	uint16_t nStr2;

	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	while ((*string1 == *string2) && (*(string1 + 1) == *(string2 + 1)))
	{

		if ((*string1 == 0) && (*(string1 + 1) == 0))
		{
			return 0;
		}

		string1 += 2;
		string2 += 2;

	}	/* End of while */

	nStr1 = (string1[1] << 8) | (uint8_t)string1[0];
	nStr2 = (string2[1] << 8) | (uint8_t)string2[0];

	return (int32_t) (nStr1 - nStr2);

}

/*****************************************************************************
 * FUNCTION
 *  mmi_ucs2chr
 * DESCRIPTION
 *  Searches a UCS2 encoded string for a given wide-character,
 *  which may be the null character L'\0'.
 * PARAMETERS
 *  strSrc        [IN]  UCS2 encoded string to search in.       
 *  c             [IN]  UCS2 encoded wide-character to search for.      
 * RETURNS
 *  returns pointer to the first occurrence of ch in string
 *  returns NULL if ch does not occur in string
 *****************************************************************************/
uint8_t *mmi_ucs2chr(const uint8_t *strSrc, uint16_t c)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/
	int8_t *chr = (int8_t*) strSrc;

	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	do
	{
		if ((*(chr+1)<<8|(uint8_t)(*chr)) == c)
		{
			return chr;
		}
		chr += 2;
	} while (*chr || *(chr+1));

	return NULL;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_ucs2str
 * DESCRIPTION
 *  Finds the first occurrence of string2 in string1
 * PARAMETERS
 *  str1        [IN]  string to search in.       
 *  str2        [IN]  string to search for.      
 * RETURNS
 *  returns a pointer to the first occurrence of string2 in
 *  string1, or NULL if string2 does not occur in string1
 *****************************************************************************/
uint8_t *mmi_ucs2str(const uint8_t *str1, const uint8_t *str2)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/
	uint8_t *cp = (uint8_t *)str1;
	uint8_t *s1, *s2;
	
	if (!(*str2 || *(str2+1)))
	{	 
		return((uint8_t *)str1);
	}
	
	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	while (*cp || *(cp+1))
	{
		s1 = cp;
		s2 = (uint8_t *) str2;
		
		while ((*s1 || *(s1+1)) 
			   && (*s2 || *(s2+1)) 
			   && !((*s1-*s2) || (*(s1+1)-*(s2+1))))
		{
			s1 += 2;
			s2 += 2;
		}
		
		if (!(*s2 || *(s2+1)))
		{
			return(cp);
		}
		
		cp += 2;
	}
	
	return NULL;	
}

/*****************************************************************************
 * FUNCTION
 *  unicode_to_ucs2encoding
 * DESCRIPTION
 *  convert unicode to UCS2 encoding
 * PARAMETERS
 *  unicode         [IN]        Value to be encoded
 *  charLength      [OUT]         
 *  arrOut          [OUT]         
 *  array           [OUT]       Of u8_t
 * RETURNS
 *  u8_t -> Status
 *****************************************************************************/
uint8_t unicode_to_ucs2encoding(uint16_t unicode, uint8_t *charLength, uint8_t *arrOut)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint8_t status = 1;
    uint8_t index = 0;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if(arrOut != 0)
    {
        if (unicode < 256)
        {
            arrOut[index++] = *((uint8_t*) (&unicode));
            arrOut[index] = 0;

        }
        else
        {
            arrOut[index++] = *((uint8_t*) (&unicode));
            arrOut[index] = *(((uint8_t*) (&unicode)) + 1);

        }
        *charLength = 2;
    }
    else
    {
        status = 0;
    }

    return status;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_asc_to_ucs2
 * DESCRIPTION
 *  Converts Ansii encode string to unicode
 *  
 *  Caller has to ensure that pOutBuffer
 *  should be as large
 * PARAMETERS
 *  pOutBuffer      [OUT]     
 *  pInBuffer       [IN]     
 * RETURNS
 *  u16_t
 *****************************************************************************/
uint16_t mmi_asc_to_ucs2(uint8_t *pOutBuffer, uint8_t *pInBuffer)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/
	int16_t count = -1;
	uint8_t charLen = 0;
	uint8_t arrOut[2];

	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	while (*pInBuffer != '\0')
	{

		unicode_to_ucs2encoding((uint16_t)(*((uint8_t *)pInBuffer)), &charLen, arrOut);

		// #ifdef MMI_ON_WIN32
		pOutBuffer[++count] = arrOut[0];
		pOutBuffer[++count] = arrOut[1];
		pInBuffer++;
		// #endif
	}

	pOutBuffer[++count] = '\0';
	pOutBuffer[++count] = '\0';
	return count + 1;
}

/*****************************************************************************
 * FUNCTION
 *  mmi_ucs2_to_asc
 * DESCRIPTION
 *  Converts Unicode encode string to Ascii
 *  
 *  Caller has to ensure that pOutBuffer
 *  should be  large enough
 * PARAMETERS
 *  pOutBuffer      [OUT]     
 *  pInBuffer       [IN]     
 * RETURNS
 *  u16_t
 *****************************************************************************/
uint16_t mmi_ucs2_to_asc(uint8_t *pOutBuffer, uint8_t *pInBuffer)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/
	uint16_t count = 0;

	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	while(!((*pInBuffer == 0) && (*(pInBuffer + 1) == 0)))
	{
		*pOutBuffer = *(pInBuffer);
		pInBuffer += 2;
		pOutBuffer++;
		count++;
	}

	*pOutBuffer = 0;
	return count;
}

uint16_t *ASCTOUCS2(uint8_t *input)
{
	/*----------------------------------------------------------------*/
	/* Local Variables												  */
	/*----------------------------------------------------------------*/
	uint16_t len=0,tmpbuf[512]={0};

	/*----------------------------------------------------------------*/
	/* Code Body													  */
	/*----------------------------------------------------------------*/
	while(!((*input == 0) && (*(input + 1) == 0)) && (len < sizeof(tmpbuf)-1))
	{
		tmpbuf[len++] = *(input);
		input += 2;
	}

	return tmpbuf;
}

