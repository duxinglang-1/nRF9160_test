/****************************************Copyright (c)************************************************
** File Name:			    ft_current.c
** Descriptions:			Factory test PCBA standby current source file
** Created By:				xie biao
** Created Date:			2023-03-13
** Modified Date:      		2023-03-13 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_current.h"
		
#define FT_CUR_TITLE_W				100
#define FT_CUR_TITLE_H				40
#define FT_CUR_TITLE_X				((LCD_WIDTH-FT_CUR_TITLE_W)/2)
#define FT_CUR_TITLE_Y				20

#define FT_CUR_MENU_STR_W			200
#define FT_CUR_MENU_STR_H			30
#define FT_CUR_MENU_STR_X			((LCD_WIDTH-FT_CUR_MENU_STR_W)/2)
#define FT_CUR_MENU_STR_Y			80
#define FT_CUR_MENU_STR_OFFSET_Y	5

#define FT_CUR_SLE1_STR_W			70
#define FT_CUR_SLE1_STR_H			30
#define FT_CUR_SLE1_STR_X			40
#define FT_CUR_SLE1_STR_Y			170
#define FT_CUR_SLE2_STR_W			70
#define FT_CUR_SLE2_STR_H			30
#define FT_CUR_SLE2_STR_X			130
#define FT_CUR_SLE2_STR_Y			170

#define FT_CUR_PASS_STR_W			80
#define FT_CUR_PASS_STR_H			40
#define FT_CUR_PASS_STR_X			30
#define FT_CUR_PASS_STR_Y			((LCD_HEIGHT-FT_CUR_PASS_STR_H)/2)
#define FT_CUR_FAIL_STR_W			80
#define FT_CUR_FAIL_STR_H			40
#define FT_CUR_FAIL_STR_X			130
#define FT_CUR_FAIL_STR_Y			((LCD_HEIGHT-FT_CUR_FAIL_STR_H)/2)

#define FT_CUR_RET_STR_W			120
#define FT_CUR_RET_STR_H			60
#define FT_CUR_RET_STR_X			((LCD_WIDTH-FT_CUR_RET_STR_W)/2)
#define FT_CUR_RET_STR_Y			((LCD_HEIGHT-FT_CUR_RET_STR_H)/2)

static bool ft_cur_checking = false;

static void FTMenuCurDumpProc(void){}

const ft_menu_t FT_MENU_CURRENT = 
{
	FT_CURRENT,
	0,
	2,
	{
		{0x6309,0x4EFB,0x610F,0x952E,0x8FDB,0x5165,0x4F11,0x7720,0x72B6,0x6001,0x0000},		//按任意键进入休眠状态
		{0x4F11,0x7720,0x540E,0x6309,0x4EFB,0x610F,0x952E,0x5524,0x9192,0x0000},			//休眠后按任意键唤醒
	},
	{
		FTMenuCurDumpProc,
		FTMenuCurDumpProc,
	},
	{	
		//page proc func
		FTMenuCurDumpProc,
		FTMenuCurDumpProc,
		FTMenuCurDumpProc,
		FTMenuCurDumpProc,
	},
};

static void FTMenuCurPassHander(void)
{
	ft_menu_checked[ft_main_menu_index] = true;
	FTMainMenu2Proc();
}

static void FTMenuCurFailHander(void)
{
	ft_menu_checked[ft_main_menu_index] = false;
	FTMainMenu2Proc();
}

static void FTMenuCurSle1Hander(void)
{
	FTMainMenu2Proc();
}

static void FTMenuCurSle2Hander(void)
{
	ExitFTMenuCur();
}

static void FTMenuCurStopTest(void)
{
	ft_cur_checking = false;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuCurStartTest(void)
{
	ft_cur_checking = true;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuCurUpdate(void)
{
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };

	if(ft_cur_checking)
	{
		LCD_Set_BL_Mode(LCD_BL_OFF);

		LCD_Fill(FT_CUR_MENU_STR_X, FT_CUR_MENU_STR_Y+0*(FT_CUR_MENU_STR_H+FT_CUR_MENU_STR_OFFSET_Y), FT_CUR_MENU_STR_W, FT_CUR_MENU_STR_H, BLACK);
		LCD_Fill(FT_CUR_MENU_STR_X, FT_CUR_MENU_STR_Y+1*(FT_CUR_MENU_STR_H+FT_CUR_MENU_STR_OFFSET_Y), FT_CUR_MENU_STR_W, FT_CUR_MENU_STR_H, BLACK);
		
		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuCurStopTest);
		SetRightKeyUpHandler(FTMenuCurStopTest);
	}
	else
	{
		LCD_Set_BL_Mode(LCD_BL_AUTO);
		
		LCD_SetFontSize(FONT_SIZE_36);
		LCD_MeasureUniString(ret_str[0], &w, &h);
		x = FT_CUR_PASS_STR_X+(FT_CUR_PASS_STR_W-w)/2;
		y = FT_CUR_PASS_STR_Y+(FT_CUR_PASS_STR_H-h)/2;
		LCD_DrawRectangle(FT_CUR_PASS_STR_X, FT_CUR_PASS_STR_Y, FT_CUR_PASS_STR_W, FT_CUR_PASS_STR_H);
		LCD_ShowUniString(x, y, ret_str[0]);
		LCD_MeasureUniString(ret_str[1], &w, &h);
		x = FT_CUR_FAIL_STR_X+(FT_CUR_FAIL_STR_W-w)/2;
		y = FT_CUR_FAIL_STR_Y+(FT_CUR_FAIL_STR_H-h)/2;
		LCD_DrawRectangle(FT_CUR_FAIL_STR_X, FT_CUR_FAIL_STR_Y, FT_CUR_FAIL_STR_W, FT_CUR_FAIL_STR_H);
		LCD_ShowUniString(x, y, ret_str[1]);

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuCurSle1Hander);
		SetRightKeyUpHandler(FTMenuCurSle2Hander);
			
	#ifdef CONFIG_TOUCH_SUPPORT
		clear_all_touch_event_handle();
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_CUR_SLE1_STR_X, FT_CUR_SLE1_STR_X+FT_CUR_SLE1_STR_W, FT_CUR_SLE1_STR_Y, FT_CUR_SLE1_STR_Y+FT_CUR_SLE1_STR_H, FTMenuCurSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_CUR_SLE2_STR_X, FT_CUR_SLE2_STR_X+FT_CUR_SLE2_STR_W, FT_CUR_SLE2_STR_Y, FT_CUR_SLE2_STR_Y+FT_CUR_SLE2_STR_H, FTMenuCurSle2Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_CUR_PASS_STR_X, FT_CUR_PASS_STR_X+FT_CUR_PASS_STR_W, FT_CUR_PASS_STR_Y, FT_CUR_PASS_STR_Y+FT_CUR_PASS_STR_H, FTMenuCurPassHander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_CUR_FAIL_STR_X, FT_CUR_FAIL_STR_X+FT_CUR_FAIL_STR_W, FT_CUR_FAIL_STR_Y, FT_CUR_FAIL_STR_Y+FT_CUR_FAIL_STR_H, FTMenuCurFailHander);
	#endif		
	}
}

static void FTMenuCurShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x7535,0x6D41,0x6D4B,0x8BD5,0x0000};//电流测试
	uint16_t sle_str[2][5] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							  };

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_CUR_TITLE_X+(FT_CUR_TITLE_W-w)/2, FT_CUR_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_20);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.name[i], &w, &h);
		LCD_ShowUniString(FT_CUR_MENU_STR_X+(FT_CUR_MENU_STR_W-w)/2, FT_CUR_MENU_STR_Y+(FT_CUR_MENU_STR_H-h)/2+i*(FT_CUR_MENU_STR_H+FT_CUR_MENU_STR_OFFSET_Y), ft_menu.name[i]);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_CUR_MENU_STR_X, 
									FT_CUR_MENU_STR_X+FT_CUR_MENU_STR_W, 
									FT_CUR_MENU_STR_Y+i*(FT_CUR_MENU_STR_H+FT_CUR_MENU_STR_OFFSET_Y), 
									FT_CUR_MENU_STR_Y+i*(FT_CUR_MENU_STR_H+FT_CUR_MENU_STR_OFFSET_Y)+FT_CUR_MENU_STR_H, 
									ft_menu.sel_handler[i]);
	#endif
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_CUR_SLE1_STR_X+(FT_CUR_SLE1_STR_W-w)/2;
	y = FT_CUR_SLE1_STR_Y+(FT_CUR_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_CUR_SLE1_STR_X, FT_CUR_SLE1_STR_Y, FT_CUR_SLE1_STR_W, FT_CUR_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_CUR_SLE2_STR_X+(FT_CUR_SLE2_STR_W-w)/2;
	y = FT_CUR_SLE2_STR_Y+(FT_CUR_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_CUR_SLE2_STR_X, FT_CUR_SLE2_STR_Y, FT_CUR_SLE2_STR_W, FT_CUR_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuCurStartTest);
	SetRightKeyUpHandler(FTMenuCurStartTest);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_CUR_SLE1_STR_X, FT_CUR_SLE1_STR_X+FT_CUR_SLE1_STR_W, FT_CUR_SLE1_STR_Y, FT_CUR_SLE1_STR_Y+FT_CUR_SLE1_STR_H, FTMenuCurSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_CUR_SLE2_STR_X, FT_CUR_SLE2_STR_X+FT_CUR_SLE2_STR_W, FT_CUR_SLE2_STR_Y, FT_CUR_SLE2_STR_Y+FT_CUR_SLE2_STR_H, FTMenuCurSle2Hander);
#endif		
}

void FTMenuCurProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuCurShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuCurUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

bool IsFTCurrentTest(void)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_CURRENT))
		return true;
	else
		return false;
}

void ExitFTMenuCur(void)
{
	ReturnFTMainMenu();
}

void EnterFTMenuCur(void)
{
	ft_cur_checking = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_CURRENT, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

