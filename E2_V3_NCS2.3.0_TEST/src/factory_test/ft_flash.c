/****************************************Copyright (c)************************************************
** File Name:			    ft_flash.c
** Descriptions:			Factory test flash module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <drivers/flash.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_flash.h"
			
#define FT_FLASH_TITLE_W			100
#define FT_FLASH_TITLE_H			40
#define FT_FLASH_TITLE_X			((LCD_WIDTH-FT_FLASH_TITLE_W)/2)
#define FT_FLASH_TITLE_Y			20

#define FT_FLASH_MENU_STR_W			150
#define FT_FLASH_MENU_STR_H			30
#define FT_FLASH_MENU_STR_X			((LCD_WIDTH-FT_FLASH_MENU_STR_W)/2)
#define FT_FLASH_MENU_STR_Y			80
#define FT_FLASH_MENU_STR_OFFSET_Y	5

#define FT_FLASH_SLE1_STR_W			70
#define FT_FLASH_SLE1_STR_H			30
#define FT_FLASH_SLE1_STR_X			40
#define FT_FLASH_SLE1_STR_Y			170
#define FT_FLASH_SLE2_STR_W			70
#define FT_FLASH_SLE2_STR_H			30
#define FT_FLASH_SLE2_STR_X			130
#define FT_FLASH_SLE2_STR_Y			170

#define FT_FLASH_RET_STR_W			120
#define FT_FLASH_RET_STR_H			60
#define FT_FLASH_RET_STR_X			((LCD_WIDTH-FT_FLASH_RET_STR_W)/2)
#define FT_FLASH_RET_STR_Y			((LCD_HEIGHT-FT_FLASH_RET_STR_H)/2)

#define FT_FLASH_STR_W				LCD_WIDTH
#define FT_FLASH_STR_H				40
#define FT_FLASH_STR_X				((LCD_WIDTH-FT_FLASH_STR_W)/2)
#define FT_FLASH_STR_Y				100

#define FT_FLASH_DELAY_CHECK	1000

static bool ft_flash_start_check = false;
static bool ft_flash_checked = false;

static void FlashDelayTestCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(flash_test_timer, FlashDelayTestCallBack, NULL);

static void FTMenuFlashDumpProc(void){}

const ft_menu_t FT_MENU_FLASH = 
{
	FT_FLASH,
	0,
	0,
	{
		{0x0000},
	},
	{
		FTMenuFlashDumpProc,
	},
	{	
		//page proc func
		FTMenuFlashDumpProc,
		FTMenuFlashDumpProc,
		FTMenuFlashDumpProc,
		FTMenuFlashDumpProc,
	},
};

static void FTMenuFlashSle1Hander(void)
{
	FTMainMenu9Proc();
}

static void FTMenuFlashSle2Hander(void)
{
	ExitFTMenuFlash();
}

void FTFlashStatusUpdate(void)
{
	uint16_t flash_id;
	uint8_t ui_version[16],font_version[16],ppg_version[16];
	
	flash_id = SpiFlash_ReadID();
	if(flash_id == W25Q64_ID)
	{
		SPIFlash_Read_DataVer(&ui_version, &font_version, &ppg_version);
		if((ui_version[0] != 0xff && ui_version[0] != 0x00)
			&&(font_version[0] != 0xff && font_version[0] != 0x00)
			&&(ppg_version[0] != 0xff && ppg_version[0] != 0x00))
		{
			ft_flash_checked = true;
			ft_menu_checked[ft_main_menu_index] = true;
		}
	}
	
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_FLASH))
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FlashDelayTestCallBack(struct k_timer *timer_id)
{
	ft_flash_start_check = true;
}

static void FTMenuFlashUpdate(void)
{
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
							  };

	LCD_Set_BL_Mode(LCD_BL_AUTO);

	LCD_Fill(FT_FLASH_STR_X, FT_FLASH_STR_Y, FT_FLASH_STR_W, FT_FLASH_STR_H, BLACK);
	
	LCD_SetFontSize(FONT_SIZE_52);
	LCD_SetFontColor(BRRED);
	LCD_SetFontBgColor(GREEN);
	LCD_MeasureUniString(ret_str[ft_flash_checked], &w, &h);
	LCD_ShowUniString(FT_FLASH_RET_STR_X+(FT_FLASH_RET_STR_W-w)/2, FT_FLASH_RET_STR_Y+(FT_FLASH_RET_STR_H-h)/2, ret_str[ft_flash_checked]);
	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();
}

static void FTMenuFlashShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[8] = {0x0046,0x004C,0x0041,0x0053,0x0048,0x6D4B,0x8BD5,0x0000};//FLASH测试
	uint16_t sle_str[2][4] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							};
	uint16_t notify_str[8] = {0x6B63,0x5728,0x68C0,0x6D4B,0x2026,0x0000};//正在检测…

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif
	
	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_FLASH_TITLE_X+(FT_FLASH_TITLE_W-w)/2, FT_FLASH_TITLE_Y, title_str);

	LCD_MeasureUniString(notify_str, &w, &h);
	LCD_ShowUniString(FT_FLASH_STR_X+(FT_FLASH_STR_W-w)/2, FT_FLASH_STR_Y+(FT_FLASH_STR_H-h)/2, notify_str);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_FLASH_SLE1_STR_X+(FT_FLASH_SLE1_STR_W-w)/2;
	y = FT_FLASH_SLE1_STR_Y+(FT_FLASH_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_FLASH_SLE1_STR_X, FT_FLASH_SLE1_STR_Y, FT_FLASH_SLE1_STR_W, FT_FLASH_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_FLASH_SLE2_STR_X+(FT_FLASH_SLE2_STR_W-w)/2;
	y = FT_FLASH_SLE2_STR_Y+(FT_FLASH_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_FLASH_SLE2_STR_X, FT_FLASH_SLE2_STR_Y, FT_FLASH_SLE2_STR_W, FT_FLASH_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuFlashSle1Hander);
	SetRightKeyUpHandler(FTMenuFlashSle2Hander);
		
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_FLASH_SLE1_STR_X, FT_FLASH_SLE1_STR_X+FT_FLASH_SLE1_STR_W, FT_FLASH_SLE1_STR_Y, FT_FLASH_SLE1_STR_Y+FT_FLASH_SLE1_STR_H, FTMenuFlashSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_FLASH_SLE2_STR_X, FT_FLASH_SLE2_STR_X+FT_FLASH_SLE2_STR_W, FT_FLASH_SLE2_STR_Y, FT_FLASH_SLE2_STR_Y+FT_FLASH_SLE2_STR_H, FTMenuFlashSle2Hander);
#endif	
}

void FTMenuFlashProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuFlashShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuFlashUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}

	if(ft_flash_start_check)
	{
		FTFlashStatusUpdate();
		ft_flash_start_check = false;
	}
}

void ExitFTMenuFlash(void)
{
	k_timer_stop(&flash_test_timer);
	ReturnFTMainMenu();
}

void EnterFTMenuFlash(void)
{
	ft_flash_start_check = false;
	ft_flash_checked = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_FLASH, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;

	k_timer_start(&flash_test_timer, K_MSEC(FT_FLASH_DELAY_CHECK), K_NO_WAIT);
}
