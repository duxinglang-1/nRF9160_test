/****************************************Copyright (c)************************************************
** File Name:			    ft_lcd.c
** Descriptions:			Factory test lcd module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-03-08 
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
#include "ft_lcd.h"
	
#define FT_LCD_TITLE_W				100
#define FT_LCD_TITLE_H				40
#define FT_LCD_TITLE_X				((LCD_WIDTH-FT_LCD_TITLE_W)/2)
#define FT_LCD_TITLE_Y				20
#define FT_LCD_MENU_STR_W			150
#define FT_LCD_MENU_STR_H			30
#define FT_LCD_MENU_STR_X			((LCD_WIDTH-FT_LCD_MENU_STR_W)/2)
#define FT_LCD_MENU_STR_Y			80
#define FT_LCD_MENU_STR_OFFSET_Y	5
#define FT_LCD_SLE1_STR_W			70
#define FT_LCD_SLE1_STR_H			30
#define FT_LCD_SLE1_STR_X			40
#define FT_LCD_SLE1_STR_Y			170
#define FT_LCD_SLE2_STR_W			70
#define FT_LCD_SLE2_STR_H			30
#define FT_LCD_SLE2_STR_X			130
#define FT_LCD_SLE2_STR_Y			170
#define FT_LCD_RET_STR_W			120
#define FT_LCD_RET_STR_H			60
#define FT_LCD_RET_STR_X			((LCD_WIDTH-FT_LCD_RET_STR_W)/2)
#define FT_LCD_RET_STR_Y			((LCD_HEIGHT-FT_LCD_RET_STR_H)/2)

#define FT_LCD_COLOR_CHANGE_INTERVEIW	1000

static bool ft_lcd_color_change = false;
static FT_LCD_COLOR ft_color;

static void LcdTestTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(Lcd_test_timer, LcdTestTimerOutCallBack, NULL);

static void FTMenuLcdDumpProc(void){}

const ft_menu_t FT_MENU_LCD = 
{
	FT_LCD,
	0,
	2,
	{
		{0x6309,0x4EFB,0x610F,0x952E,0x542F,0x52A8,0x6D4B,0x8BD5,0x0000},		//按任意键启动
		{0x6D4B,0x8BD5,0x65F6,0x6309,0x4EFB,0x610F,0x952E,0x9000,0x51FA,0x0000},//测试时按任意键退出
	},
	{
		FTMenuLcdDumpProc,
		FTMenuLcdDumpProc,
	},
	{	
		//page proc func
		FTMenuLcdDumpProc,
		FTMenuLcdDumpProc,
		FTMenuLcdDumpProc,
		FTMenuLcdDumpProc,
	},
};

static void FTMenuLcdSle1Hander(void)
{
	FTMainMenu4Proc();
}

static void FTMenuLcdSle2Hander(void)
{
	ExitFTMenuLcd();
}

static void LcdTestTimerOutCallBack(struct k_timer *timer_id)
{
	ft_color++;
	if(ft_color >= LCD_COLOR_MAX)
		ft_color = LCD_COLOR_BEGIN;
	
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuLcdStopTest(void)
{
	ft_lcd_color_change = false;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	k_timer_stop(&Lcd_test_timer);
}

static void FTMenuLcdStartTest(void)
{
	ft_lcd_color_change = true;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	k_timer_start(&Lcd_test_timer, K_MSEC(FT_LCD_COLOR_CHANGE_INTERVEIW), K_MSEC(FT_LCD_COLOR_CHANGE_INTERVEIW));
}

static void FTMenuLcdUpdate(void)
{
	static bool flag = false;
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[10] = {0x5C4F,0x5E55,0x6D4B,0x8BD5,0x0000};//屏幕测试
	uint16_t sle_str[2][5] = {
 								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
 								{0x9000,0x51FA,0x0000},//退出
 							 };
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };

	LCD_Clear(BLACK);
	
	if(ft_lcd_color_change)
	{
		if(!flag)
		{
			flag = true;
			LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
			
			ClearAllKeyHandler();
			SetLeftKeyUpHandler(FTMenuLcdStopTest);
			SetRightKeyUpHandler(FTMenuLcdStopTest);
		}
		
		switch(ft_color)
		{
		case LCD_COLOR_BLACK:
			LCD_Clear(BLACK);
			break;
		case LCD_COLOR_WHITE:
			LCD_Clear(WHITE);
			break;
		case LCD_COLOR_RED:
			LCD_Clear(RED);
			break;
		case LCD_COLOR_GREEN:
			LCD_Clear(GREEN);
			break;
		case LCD_COLOR_BLUE:
			LCD_Clear(BLUE);
			break;
		}
	}
	else
	{
		flag = false;
		
		LCD_Set_BL_Mode(LCD_BL_AUTO);
		LCD_SetFontSize(FONT_SIZE_36);

		//title
		LCD_MeasureUniString(title_str, &w, &h);
		LCD_ShowUniString(FT_LCD_TITLE_X+(FT_LCD_TITLE_W-w)/2, FT_LCD_TITLE_Y, title_str);
		//pass
		LCD_SetFontSize(FONT_SIZE_52);
		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);
		LCD_MeasureUniString(ret_str[0], &w, &h);
		LCD_ShowUniString(FT_LCD_RET_STR_X+(FT_LCD_RET_STR_W-w)/2, FT_LCD_RET_STR_Y+(FT_LCD_RET_STR_H-h)/2, ret_str[0]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();
		//leftsoft key and rightsoft key
		LCD_SetFontSize(FONT_SIZE_28);
		LCD_MeasureUniString(sle_str[0], &w, &h);
		x = FT_LCD_SLE1_STR_X+(FT_LCD_SLE1_STR_W-w)/2;
		y = FT_LCD_SLE1_STR_Y+(FT_LCD_SLE1_STR_H-h)/2;
		LCD_DrawRectangle(FT_LCD_SLE1_STR_X, FT_LCD_SLE1_STR_Y, FT_LCD_SLE1_STR_W, FT_LCD_SLE1_STR_H);
		LCD_ShowUniString(x, y, sle_str[0]);
		LCD_MeasureUniString(sle_str[1], &w, &h);
		x = FT_LCD_SLE2_STR_X+(FT_LCD_SLE2_STR_W-w)/2;
		y = FT_LCD_SLE2_STR_Y+(FT_LCD_SLE2_STR_H-h)/2;
		LCD_DrawRectangle(FT_LCD_SLE2_STR_X, FT_LCD_SLE2_STR_Y, FT_LCD_SLE2_STR_W, FT_LCD_SLE2_STR_H);
		LCD_ShowUniString(x, y, sle_str[1]);

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuLcdSle1Hander);
		SetRightKeyUpHandler(FTMenuLcdSle2Hander);
			
	#ifdef CONFIG_TOUCH_SUPPORT
		clear_all_touch_event_handle();
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_LCD_SLE1_STR_X, FT_LCD_SLE1_STR_X+FT_LCD_SLE1_STR_W, FT_LCD_SLE1_STR_Y, FT_LCD_SLE1_STR_Y+FT_LCD_SLE1_STR_H, FTMenuLcdSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_LCD_SLE2_STR_X, FT_LCD_SLE2_STR_X+FT_LCD_SLE2_STR_W, FT_LCD_SLE2_STR_Y, FT_LCD_SLE2_STR_Y+FT_LCD_SLE2_STR_H, FTMenuLcdSle2Hander);
	#endif		
	}
}

static void FTMenuLcdShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x5C4F,0x5E55,0x6D4B,0x8BD5,0x0000};//屏幕测试
	uint16_t sle_str[2][5] = {
 								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
 								{0x9000,0x51FA,0x0000},//退出
 							  };

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);
	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_LCD_TITLE_X+(FT_LCD_TITLE_W-w)/2, FT_LCD_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_20);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.name[i], &w, &h);
		LCD_ShowUniString(FT_LCD_MENU_STR_X+(FT_LCD_MENU_STR_W-w)/2, FT_LCD_MENU_STR_Y+(FT_LCD_MENU_STR_H-h)/2+i*(FT_LCD_MENU_STR_H+FT_LCD_MENU_STR_OFFSET_Y), ft_menu.name[i]);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_LCD_MENU_STR_X, 
									FT_LCD_MENU_STR_X+FT_LCD_MENU_STR_W, 
									FT_LCD_MENU_STR_Y+i*(FT_LCD_MENU_STR_H+FT_LCD_MENU_STR_OFFSET_Y), 
									FT_LCD_MENU_STR_Y+i*(FT_LCD_MENU_STR_H+FT_LCD_MENU_STR_OFFSET_Y)+FT_LCD_MENU_STR_H, 
									ft_menu.sel_handler[i]);
	#endif
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_LCD_SLE1_STR_X+(FT_LCD_SLE1_STR_W-w)/2;
	y = FT_LCD_SLE1_STR_Y+(FT_LCD_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_LCD_SLE1_STR_X, FT_LCD_SLE1_STR_Y, FT_LCD_SLE1_STR_W, FT_LCD_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_LCD_SLE2_STR_X+(FT_LCD_SLE2_STR_W-w)/2;
	y = FT_LCD_SLE2_STR_Y+(FT_LCD_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_LCD_SLE2_STR_X, FT_LCD_SLE2_STR_Y, FT_LCD_SLE2_STR_W, FT_LCD_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuLcdStartTest);
	SetRightKeyUpHandler(FTMenuLcdStartTest);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_LCD_SLE1_STR_X, FT_LCD_SLE1_STR_X+FT_LCD_SLE1_STR_W, FT_LCD_SLE1_STR_Y, FT_LCD_SLE1_STR_Y+FT_LCD_SLE1_STR_H, FTMenuLcdSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_LCD_SLE2_STR_X, FT_LCD_SLE2_STR_X+FT_LCD_SLE2_STR_W, FT_LCD_SLE2_STR_Y, FT_LCD_SLE2_STR_Y+FT_LCD_SLE2_STR_H, FTMenuLcdSle2Hander);
#endif		
}

void FTMenuLcdProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuLcdShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuLcdUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void ExitFTMenuLcd(void)
{
	ReturnFTMainMenu();
}

void EnterFTMenuLcd(void)
{
	ft_color = LCD_COLOR_BEGIN;
	memcpy(&ft_menu, &FT_MENU_LCD, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}
