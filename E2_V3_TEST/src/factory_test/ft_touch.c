/****************************************Copyright (c)************************************************
** File Name:			    ft_touch.c
** Descriptions:			Factory test touch module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#include "external_flash.h"
#include "lcd.h"
#if defined(LCD_ORCZ010903C_GC9A01)
#include "LCD_ORCZ010903C_GC9A01.h"
#elif defined(LCD_R108101_GC9307)
#include "LCD_R108101_GC9307.h"
#elif defined(LCD_ORCT012210N_ST7789V2)
#include "LCD_ORCT012210N_ST7789V2.h"
#elif defined(LCD_R154101_ST7796S)
#include "LCD_R154101_ST7796S.h"
#elif defined(LCD_LH096TIG11G_ST7735SV)
#include "LCD_LH096TIG11G_ST7735SV.h"
#elif defined(LCD_VGM068A4W01_SH1106G)
#include "LCD_VGM068A4W01_SH1106G.h"
#elif defined(LCD_VGM096064A6W01_SP5090)
#include "LCD_VGM096064A6W01_SP5090.h"
#endif 
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_touch.h"

#define FT_TP_TITLE_W				100
#define FT_TP_TITLE_H				40
#define FT_TP_TITLE_X				((COL-FT_TP_TITLE_W)/2)
#define FT_TP_TITLE_Y				20

#define FT_TP_MENU_STR_W			150
#define FT_TP_MENU_STR_H			30
#define FT_TP_MENU_STR_X			((COL-FT_TP_MENU_STR_W)/2)
#define FT_TP_MENU_STR_Y			80
#define FT_TP_MENU_STR_OFFSET_Y		5

#define FT_TP_CLICK_CIRCLE_R		15
#define FT_TP_CLICK_OFFSET_X		40
#define FT_TP_CLICK_OFFSET_Y		40
#define FT_TP_CIRCLE_1_X			(FT_TP_CLICK_OFFSET_X+FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_1_Y			(FT_TP_CLICK_OFFSET_Y+FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_2_X			(COL-FT_TP_CLICK_OFFSET_X-FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_2_Y			(FT_TP_CLICK_OFFSET_Y+FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_3_X			(FT_TP_CLICK_OFFSET_X+FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_3_Y			(ROW-FT_TP_CLICK_OFFSET_Y-FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_4_X			(COL-FT_TP_CLICK_OFFSET_X-FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_4_Y			(ROW-FT_TP_CLICK_OFFSET_Y-FT_TP_CLICK_CIRCLE_R)
#define FT_TP_CIRCLE_5_X			(COL/2)
#define FT_TP_CIRCLE_5_Y			(ROW/2)

#define FT_TP_SLE1_STR_W			70
#define FT_TP_SLE1_STR_H			30
#define FT_TP_SLE1_STR_X			40
#define FT_TP_SLE1_STR_Y			170
#define FT_TP_SLE2_STR_W			70
#define FT_TP_SLE2_STR_H			30
#define FT_TP_SLE2_STR_X			130
#define FT_TP_SLE2_STR_Y			170

#define FT_TP_RET_STR_W				120
#define FT_TP_RET_STR_H				60
#define FT_TP_RET_STR_X				((COL-FT_TP_RET_STR_W)/2)
#define FT_TP_RET_STR_Y				((ROW-FT_TP_RET_STR_H)/2)

static bool ft_tp_testing = false;
static bool update_show_flag = false;

ft_touch_t ft_tp = {0};

const ft_touch_t FT_TP_INF[FT_TP_MAX] = 
{
	{FT_TP_SINGLE_CLICK,	5,	0,	FT_TP_NONE},
	{FT_TP_MOVING_UP, 		1,	0,	FT_TP_NONE},
	{FT_TP_MOVING_DOWN, 	1,	0,	FT_TP_NONE},
	{FT_TP_MOVING_LEFT,		1,	0,	FT_TP_NONE},
	{FT_TP_MOVING_RIGHT,	1,	0,	FT_TP_NONE},
};

static void FTMenuTouchDumpProc(void){}
static void FTMenuSingleClickHander(void);

const ft_menu_t FT_MENU_TOUCH = 
{
	FT_TOUCH,
	0,
	2,
	{
		{0x6309,0x4EFB,0x610F,0x952E,0x542F,0x52A8,0x6D4B,0x8BD5,0x0000},		//按任意键启动测试
		{0x6D4B,0x8BD5,0x65F6,0x6309,0x4EFB,0x610F,0x952E,0x9000,0x51FA,0x0000},//测试时按任意键退出
	},	
	{
		FTMenuTouchDumpProc,
		FTMenuTouchDumpProc,
	},
	{	
		//page proc func
		FTMenuTouchDumpProc,
		FTMenuTouchDumpProc,
		FTMenuTouchDumpProc,
		FTMenuTouchDumpProc,
	},
};

const ft_touch_click_t FT_TOUCH_CLICK[5] = 
{
	{FT_TP_CIRCLE_5_X, FT_TP_CIRCLE_5_Y, FT_TP_CLICK_CIRCLE_R, (COL-120)/2, 			FT_TP_CIRCLE_5_Y+FT_TP_CLICK_CIRCLE_R+10, 120, 30, FTMenuSingleClickHander},
	{FT_TP_CIRCLE_1_X, FT_TP_CIRCLE_1_Y, FT_TP_CLICK_CIRCLE_R, FT_TP_CIRCLE_1_X-120/2,	FT_TP_CIRCLE_1_Y+FT_TP_CLICK_CIRCLE_R+10, 120, 30, FTMenuSingleClickHander},
	{FT_TP_CIRCLE_2_X, FT_TP_CIRCLE_2_Y, FT_TP_CLICK_CIRCLE_R, FT_TP_CIRCLE_2_X-120/2,	FT_TP_CIRCLE_2_Y+FT_TP_CLICK_CIRCLE_R+10, 120, 30, FTMenuSingleClickHander},
	{FT_TP_CIRCLE_3_X, FT_TP_CIRCLE_3_Y, FT_TP_CLICK_CIRCLE_R, FT_TP_CIRCLE_3_X-120/2,	FT_TP_CIRCLE_3_Y-FT_TP_CLICK_CIRCLE_R-40, 120, 30, FTMenuSingleClickHander},
	{FT_TP_CIRCLE_4_X, FT_TP_CIRCLE_4_Y, FT_TP_CLICK_CIRCLE_R, FT_TP_CIRCLE_4_X-120/2,	FT_TP_CIRCLE_4_Y-FT_TP_CLICK_CIRCLE_R-40, 120, 30, FTMenuSingleClickHander},
};

static void FTMenuTouchStopTest(void)
{
	ft_tp_testing = false;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuTouchStartTest(void)
{
	ft_tp_testing = true;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuTouchSle1Hander(void)
{
	FTMainMenu5Proc();
}

static void FTMenuTouchSle2Hander(void)
{
	ExitFTMenuTouch();
}

static void FTMenuMoveHander(void)
{
	ft_tp.check_index++;
	if(ft_tp.check_index == ft_tp.check_count)
	{
		ft_tp.check_item++;
		if(ft_tp.check_item == FT_TP_MAX)
		{
			ft_menu_checked[ft_main_menu_index] = true;
			FTMenuTouchStopTest();
		}
		else
		{
			memcpy(&ft_tp, &FT_TP_INF[ft_tp.check_item], sizeof(FT_TP_INF[ft_tp.check_item]));
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
		}
	}
	else
	{
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}
}

static void FTMenuSingleClickHander(void)
{
	ft_tp.check_index++;
	if(ft_tp.check_index == ft_tp.check_count)
	{
		ft_tp.check_item++;
		if(ft_tp.check_item == FT_TP_MAX)
		{
			ft_menu_checked[ft_main_menu_index] = true;
			FTMenuTouchStopTest();
		}
		else
		{
			memcpy(&ft_tp, &FT_TP_INF[ft_tp.check_item], sizeof(FT_TP_INF[ft_tp.check_item]));
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
		}
	}
	else
	{
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}
}

static void FTMenuTouchDrawTarget(uint16_t x, uint16_t y, uint16_t radius)
{
	LCD_Draw_Circle(x,y,radius);
	LCD_DrawLine(x-radius-2, y, x+radius+2, y);
	LCD_DrawLine(x, y-radius-2, x, y+radius+2);
}

static void FTMenuTouchUpdate(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[] = {0x89E6,0x6478,0x6D4B,0x8BD5,0x0000};//触摸测试
	uint16_t sle_str[2][4] = {
								{0x4E0B,0x4E00,0x9879,0x0000},	//下一项
								{0x9000,0x51FA,0x0000},			//退出
							 };
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };
	uint16_t item_str[5][5] = {
								{0x5355,0x51FB,0x9776,0x5FC3,0x0000},//单击靶心
								{0x5411,0x4E0A,0x6ED1,0x52A8,0x0000},//向上滑动
	 							{0x5411,0x4E0B,0x6ED1,0x52A8,0x0000},//向下滑动
								{0x5411,0x5DE6,0x6ED1,0x52A8,0x0000},//向左滑动
								{0x5411,0x53F3,0x6ED1,0x52A8,0x0000},//向右滑动
							 };
	uint8_t item_tp_event[5] = {
									TP_EVENT_SINGLE_CLICK,
									TP_EVENT_MOVING_UP,
									TP_EVENT_MOVING_DOWN,
									TP_EVENT_MOVING_LEFT,
									TP_EVENT_MOVING_RIGHT
								};

	LCD_Clear(BLACK);

	clear_all_touch_event_handle();

	if(ft_tp_testing)
	{
		if(!update_show_flag)
		{
			update_show_flag = true;
			LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
			
			ClearAllKeyHandler();
			SetLeftKeyUpHandler(FTMenuTouchStopTest);
			SetRightKeyUpHandler(FTMenuTouchStopTest);
		}

		LCD_SetFontSize(FONT_SIZE_28);
		switch(ft_tp.check_item)
		{
		case FT_TP_MOVING_UP:
		case FT_TP_MOVING_DOWN:
		case FT_TP_MOVING_LEFT:
		case FT_TP_MOVING_RIGHT:
			LCD_MeasureUniString(item_str[ft_tp.check_item], &w, &h);
			x = (LCD_WIDTH-w)/2;
			y = (LCD_HEIGHT-h)/2;
			LCD_ShowUniString(x, y, item_str[ft_tp.check_item]);
			register_touch_event_handle(item_tp_event[ft_tp.check_item], 0, LCD_WIDTH, 0, LCD_HEIGHT, FTMenuMoveHander);
			break;
			
		case FT_TP_SINGLE_CLICK:
			FTMenuTouchDrawTarget(FT_TOUCH_CLICK[ft_tp.check_index].circle_x, FT_TOUCH_CLICK[ft_tp.check_index].circle_y, FT_TP_CLICK_CIRCLE_R);
			LCD_MeasureUniString(item_str[ft_tp.check_item], &w, &h);
			x = FT_TOUCH_CLICK[ft_tp.check_index].str_x+(FT_TOUCH_CLICK[ft_tp.check_index].str_w-w)/2;
			y = FT_TOUCH_CLICK[ft_tp.check_index].str_y+(FT_TOUCH_CLICK[ft_tp.check_index].str_h-h)/2;
			LCD_ShowUniString(x, y, item_str[ft_tp.check_item]);
			register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TOUCH_CLICK[ft_tp.check_index].circle_x-FT_TP_CLICK_CIRCLE_R, FT_TOUCH_CLICK[ft_tp.check_index].circle_x+FT_TP_CLICK_CIRCLE_R, FT_TOUCH_CLICK[ft_tp.check_index].circle_y-FT_TP_CLICK_CIRCLE_R, FT_TOUCH_CLICK[ft_tp.check_index].circle_y+FT_TP_CLICK_CIRCLE_R, FT_TOUCH_CLICK[ft_tp.check_index].func);
			break;
		}
	}
	else
	{
		uint8_t result;
		
		update_show_flag = false;
		
		LCD_Set_BL_Mode(LCD_BL_AUTO);
		LCD_SetFontSize(FONT_SIZE_36);

		//title
		LCD_MeasureUniString(title_str, &w, &h);
		LCD_ShowUniString(FT_TP_TITLE_X+(FT_TP_TITLE_W-w)/2, FT_TP_TITLE_Y, title_str);
		//pass or fail
		LCD_SetFontSize(FONT_SIZE_52);
		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);
		if(ft_tp.check_item == FT_TP_MAX)
			result = 0;
		else
			result = 1;
		LCD_MeasureUniString(ret_str[result], &w, &h);
		LCD_ShowUniString(FT_TP_RET_STR_X+(FT_TP_RET_STR_W-w)/2, FT_TP_RET_STR_Y+(FT_TP_RET_STR_H-h)/2, ret_str[result]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();
		//leftsoft key and rightsoft key
		LCD_SetFontSize(FONT_SIZE_28);
		LCD_MeasureUniString(sle_str[0], &w, &h);
		x = FT_TP_SLE1_STR_X+(FT_TP_SLE1_STR_W-w)/2;
		y = FT_TP_SLE1_STR_Y+(FT_TP_SLE1_STR_H-h)/2;
		LCD_DrawRectangle(FT_TP_SLE1_STR_X, FT_TP_SLE1_STR_Y, FT_TP_SLE1_STR_W, FT_TP_SLE1_STR_H);
		LCD_ShowUniString(x, y, sle_str[0]);
		LCD_MeasureUniString(sle_str[1], &w, &h);
		x = FT_TP_SLE2_STR_X+(FT_TP_SLE2_STR_W-w)/2;
		y = FT_TP_SLE2_STR_Y+(FT_TP_SLE2_STR_H-h)/2;
		LCD_DrawRectangle(FT_TP_SLE2_STR_X, FT_TP_SLE2_STR_Y, FT_TP_SLE2_STR_W, FT_TP_SLE2_STR_H);
		LCD_ShowUniString(x, y, sle_str[1]);

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuTouchSle1Hander);
		SetRightKeyUpHandler(FTMenuTouchSle2Hander);

		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TP_SLE1_STR_X, FT_TP_SLE1_STR_X+FT_TP_SLE1_STR_W, FT_TP_SLE1_STR_Y, FT_TP_SLE1_STR_Y+FT_TP_SLE1_STR_H, FTMenuTouchSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TP_SLE2_STR_X, FT_TP_SLE2_STR_X+FT_TP_SLE2_STR_W, FT_TP_SLE2_STR_Y, FT_TP_SLE2_STR_Y+FT_TP_SLE2_STR_H, FTMenuTouchSle2Hander);
	}
}

static void FTMenuTouchShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[] = {0x89E6,0x6478,0x6D4B,0x8BD5,0x0000};//触摸测试
	uint16_t sle_str[2][4] = {
								{0x4E0B,0x4E00,0x9879,0x0000},	//下一项
								{0x9000,0x51FA,0x0000},			//退出
							 };
	
	clear_all_touch_event_handle();
	
	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	
	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_TP_TITLE_X+(FT_TP_TITLE_W-w)/2, FT_TP_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_20);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.name[i], &w, &h);
		LCD_ShowUniString(FT_TP_MENU_STR_X+(FT_TP_MENU_STR_W-w)/2, FT_TP_MENU_STR_Y+(FT_TP_MENU_STR_H-h)/2+i*(FT_TP_MENU_STR_H+FT_TP_MENU_STR_OFFSET_Y), ft_menu.name[i]);

		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_TP_MENU_STR_X, 
									FT_TP_MENU_STR_X+FT_TP_MENU_STR_W, 
									FT_TP_MENU_STR_Y+i*(FT_TP_MENU_STR_H+FT_TP_MENU_STR_OFFSET_Y), 
									FT_TP_MENU_STR_Y+i*(FT_TP_MENU_STR_H+FT_TP_MENU_STR_OFFSET_Y)+FT_TP_MENU_STR_H, 
									ft_menu.sel_handler[i]);
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_TP_SLE1_STR_X+(FT_TP_SLE1_STR_W-w)/2;
	y = FT_TP_SLE1_STR_Y+(FT_TP_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_TP_SLE1_STR_X, FT_TP_SLE1_STR_Y, FT_TP_SLE1_STR_W, FT_TP_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_TP_SLE2_STR_X+(FT_TP_SLE2_STR_W-w)/2;
	y = FT_TP_SLE2_STR_Y+(FT_TP_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_TP_SLE2_STR_X, FT_TP_SLE2_STR_Y, FT_TP_SLE2_STR_W, FT_TP_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuTouchStartTest);
	SetRightKeyUpHandler(FTMenuTouchStartTest);

	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TP_SLE1_STR_X, FT_TP_SLE1_STR_X+FT_TP_SLE1_STR_W, FT_TP_SLE1_STR_Y, FT_TP_SLE1_STR_Y+FT_TP_SLE1_STR_H, FTMenuTouchSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TP_SLE2_STR_X, FT_TP_SLE2_STR_X+FT_TP_SLE2_STR_W, FT_TP_SLE2_STR_Y, FT_TP_SLE2_STR_Y+FT_TP_SLE2_STR_H, FTMenuTouchSle2Hander);
}

void FTMenuTouchProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuTouchShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuTouchUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void ExitFTMenuTouch(void)
{
	ReturnFTMainMenu();
}

void EnterFTMenuTouch(void)
{
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_TOUCH, sizeof(ft_menu_t));
	memcpy(&ft_tp, &FT_TP_INF[0], sizeof(FT_TP_INF[0]));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST;	
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

