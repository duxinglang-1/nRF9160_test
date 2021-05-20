#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <dk_buttons_and_leds.h>
#include "lcd.h"
#include "imei.h"

#define GET_IMEI	"AT+CGSN=1"
#define RES_IMEI_HEAD	"+CGSN: "	//+CGSN: "352656100158792"

#define BAR_CODE_SATRT_A	"bbsbssssbss"
#define BAR_CODE_SATRT_B	"bbsbssbssss"
#define BAR_CODE_STOP		"bbsssbbbsbsbb"

#define BAR_CODE_HEIGHT		60
#define BAR_CODE_WIDTH		1
#define BAR_CODE_X			0
#define BAR_CODE_Y			(LCD_HEIGHT-BAR_CODE_HEIGHT)/2


u8_t imei_buf[IMEI_LEN+1] = {0};

static bar_code128_t code128[95] = 
{
	' ', "bbsbbssbbss",
	'!', "bbssbbsbbss",
	'"', "bbssbbssbbs",
	'#', "bssbssbbsss",
	'$', "bssbsssbbss",
	'%', "bsssbssbbss",
	'&', "bssbbssbsss",
	'\'', "bssbbsssbss",
	'(', "bsssbbssbss",
	')', "bbssbssbsss",
	'*', "bbssbsssbss",
	'+', "bbsssbssbss",
	',', "bsbbssbbbss",
	'-', "bssbbsbbbss",
	'.', "bssbbssbbbs",
	'/', "bsbbbssbbss",
	'0', "bssbbbsbbss",
	'1', "bssbbbssbbs",
	'2', "bbssbbbssbs",
	'3', "bbssbsbbbss",
	'4', "bbssbssbbbs",
	'5', "bbsbbbssbss",
	'6', "bbssbbbsbss",
	'7', "bbbsbbsbbbs",
	'8', "bbbsbssbbss",
	'9', "bbbssbsbbss",
	':', "bbbssbssbbs",
	';', "bbbsbbssbss",
	'<', "bbbssbbsbss",
	'=', "bbbssbbssbs",	
	'>', "bbsbbsbbsss",
	'?', "bbsbbsssbbs",
	'@', "bbsssbbsbbs",
	'A', "bsbsssbbsss",
	'B', "bsssbsbbsss",
	'C', "bsssbsssbbs",
	'D', "bsbbsssbsss",
	'E', "bsssbbsbsss",
	'F', "bsssbbsssbs",
	'G', "bbsbsssbsss",
	'H', "bbsssbsbsss",
	'I', "bbsssbsssbs",
	'J', "bsbbsbbbsss",
	'K', "bsbbsssbbbs",
	'L', "bsssbbsbbbs",
	'M', "bsbbbsbbsss",
	'N', "bsbbbsssbbs",
	'O', "bsssbbbsbbs",
	'P', "bbbsbbbsbbs",
	'Q', "bbsbsssbbbs",
	'R', "bbsssbsbbbs",
	'S', "bbsbbbsbsss",
	'T', "bbsbbbsssbs",
	'U', "bbsbbbsbbbs",
	'V', "bbbsbsbbsss",
	'W', "bbbsbsssbbs",
	'X', "bbbsssbsbbs",
	'Y', "bbbsbbsbsss",
	'Z', "bbbsbbsssbs",
	'[', "bbbsssbbsbs",
	'\\', "bbbsbbbbsbs",
	']', "bbssbssssbs",
	'^', "bbbbsssbsbs",
	'_', "bsbssbbssss",
	'`', "bsbssssbbss",
	'a', "bssbsbbssss",
	'b', "bssbssssbbs",
	'c', "bssssbsbbss",
	'd', "bssssbssbbs",
	'e', "bsbbssbssss",
	'f', "bsbbssssbss",
	'g', "bssbbsbssss",
	'h', "bssbbssssbs",
	'i', "bssssbbsbss",
	'j', "bssssbbssbs",
	'k', "bbssssbssbs",
	'I', "bbssbsbssss",
	'm', "bbbbsbbbsbs",
	'n', "bbssssbsbss",
	'o', "bsssbbbbsbs",
	'p', "bsbssbbbbss",
	'q', "bssbsbbbbss",
	'r', "bssbssbbbbs",
	's', "bsbbbbssbss",
	't', "bssbbbbsbss",
	'u', "bssbbbbssbs",
	'v', "bbbbsbssbss",
	'w', "bbbbssbsbss",
	'x', "bbbbssbssbs",
	'y', "bbsbbsbbbbs",
	'z', "bbsbbbbsbbs",
	'{', "bbbbsbbsbbs",
	'|', "bsbsbbbbsss",
	'}', "bsbsssbbbbs",
	'~', "bsssbsbbbbs",
};

bool get_imei(u8_t len, u8_t *str_imei)
{
	int err;
	enum at_cmd_state at_state;

	err = at_cmd_write(GET_IMEI, str_imei, len, &at_state);
	if(err) 
	{
		printk("get imei error, err:%d, at_state:%d", err,at_state);
		return false;
	}

	return true;
}

void show_QR_code(u32_t datalen, u8_t *data)
{

}

void show_bar_code(u32_t datalen, u8_t *data)
{
	u8_t str_error[128] = "The data is invalid!";
	u8_t str_bar[1024] = {0};
	u16_t w,h;
	u32_t i,j,x,y;
	u32_t check;
	
	if(datalen == 0 || data == NULL)
	{
		LCD_MeasureString(str_error, &w, &h);
		LCD_ShowString((LCD_WIDTH-w)/2,(LCD_HEIGHT-h)/2,str_error);
		return;
	}

	strcpy(str_bar, BAR_CODE_SATRT_B);
	check = 103;//开始位ID为103
	
	for(i=0;i<datalen;i++)
	{
		for(j=0;j<95;j++)
		{
			if(data[i] == code128[j].byte)
			{
				strcat(str_bar, code128[j].str);
				break;
			}
		}

		check += j*(i+1);
	}

	check = 0x00ff&(check%103);
	strcat(str_bar, code128[check].str);
	
	strcat(str_bar, BAR_CODE_STOP);
	
	LCD_Fill(0,(BAR_CODE_Y-20),LCD_WIDTH,(BAR_CODE_HEIGHT+2*20),GRAY);
	
	j = strlen(str_bar);
	x = LCD_WIDTH>(BAR_CODE_WIDTH*j) ? (LCD_WIDTH-BAR_CODE_WIDTH*j)/2 : 0;
	
	for(i=0;i<j;i++)
	{
		if(str_bar[i] == 'b')
			LCD_Fill(x+i*BAR_CODE_WIDTH,BAR_CODE_Y,BAR_CODE_WIDTH,BAR_CODE_HEIGHT,BLACK);
		//else
		//	LCD_Fill(x+i*BAR_CODE_WIDTH,BAR_CODE_Y,BAR_CODE_WIDTH,BAR_CODE_HEIGHT,WHITE);
	}
}

void test_imei(void)
{
	int err;
	u8_t tmpbuf[128] = {0};
	
	printk("test_imei\n");

	LCD_ShowString(0,0,"IMEI:");
	if(get_imei(128, tmpbuf))
	{
		memcpy(imei_buf, &tmpbuf[1+strlen(RES_IMEI_HEAD)], IMEI_LEN);
		LCD_ShowString(0,20,imei_buf);

		show_bar_code(15,imei_buf);
	}
}