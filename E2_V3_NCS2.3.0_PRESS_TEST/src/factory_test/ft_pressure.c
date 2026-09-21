/****************************************Copyright (c)************************************************
** File Name:			    ft_pressure.c
** Descriptions:			Pressure test flash module source file
** Created By:				xie biao
** Created Date:			2026-09-21
** Modified Date:      		2026-09-21 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "pressure.h"
#include "ft_main.h"
#include "ft_pressure.h"
			
#define FT_PRESSURE_TITLE_W			100
#define FT_PRESSURE_TITLE_H			40
#define FT_PRESSURE_TITLE_X			((LCD_WIDTH-FT_PRESSURE_TITLE_W)/2)
#define FT_PRESSURE_TITLE_Y			20

#define FT_PRESSURE_MENU_STR_W			150
#define FT_PRESSURE_MENU_STR_H			30
#define FT_PRESSURE_MENU_STR_X			((LCD_WIDTH-FT_PRESSURE_MENU_STR_W)/2)
#define FT_PRESSURE_MENU_STR_Y			80
#define FT_PRESSURE_MENU_STR_OFFSET_Y	5

#define FT_PRESSURE_SLE1_STR_W			70
#define FT_PRESSURE_SLE1_STR_H			30
#define FT_PRESSURE_SLE1_STR_X			40
#define FT_PRESSURE_SLE1_STR_Y			170
#define FT_PRESSURE_SLE2_STR_W			70
#define FT_PRESSURE_SLE2_STR_H			30
#define FT_PRESSURE_SLE2_STR_X			130
#define FT_PRESSURE_SLE2_STR_Y			170

#define FT_PRESSURE_RET_STR_W			120
#define FT_PRESSURE_RET_STR_H			60
#define FT_PRESSURE_RET_STR_X			((LCD_WIDTH-FT_PRESSURE_RET_STR_W)/2)
#define FT_PRESSURE_RET_STR_Y			((LCD_HEIGHT-FT_PRESSURE_RET_STR_H)/2)

#define FT_PRESSURE_STR_W				LCD_WIDTH
#define FT_PRESSURE_STR_H				40
#define FT_PRESSURE_STR_X				((LCD_WIDTH-FT_PRESSURE_STR_W)/2)
#define FT_PRESSURE_STR_Y				100

#define FT_PRESSURE_DELAY_CHECK	1000

static bool ft_pressure_start_check = false;
static bool ft_pressure_checked = false;

static void PressureDelayTestCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(pressure_test_timer, PressureDelayTestCallBack, NULL);

static void FTMenuPressureDumpProc(void){}

const ft_menu_t FT_MENU_PRESSURE = 
{
	FT_PRESSURE,
	0,
	0,
	{
		{
			{0x0000},
			FTMenuPressureDumpProc,
		},
	},
	{	
		//page proc func
		FTMenuPressureDumpProc,
		FTMenuPressureDumpProc,
		FTMenuPressureDumpProc,
		FTMenuPressureDumpProc,
	},
};

static void FTMenuPressureSle1Hander(void)
{
	switch(g_ft_status)
	{
	case FT_STATUS_SMT:
		FT_SMT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
		break;
		
	case FT_STATUS_ASSEM:
		FT_ASSEM_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
		break;
	}
}

static void FTMenuPressureSle2Hander(void)
{
	ExitFTMenuPressure();
}

void FTPressureStatusUpdate(void)
{
	uint16_t pressure_id;
	
	pressure_id = Pressure_ReadID();
	if((pressure_id == LPS22DF_CHIP_ID) 
		|| (pressure_id == DPS368_CHIP_ID)
		)
	{
		ft_pressure_checked = true;
		ft_menu_checked[ft_main_menu_index] = true;
	}
	else
	{
		ft_pressure_checked = false;
		ft_menu_checked[ft_main_menu_index] = false;
	}
	
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_PRESSURE))
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void PressureDelayTestCallBack(struct k_timer *timer_id)
{
	ft_pressure_start_check = true;
}

static void FTMenuPressureUpdate(void)
{
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
							  };

	LCD_Fill(FT_PRESSURE_STR_X, FT_PRESSURE_STR_Y, FT_PRESSURE_STR_W, FT_PRESSURE_STR_H, BLACK);
	
	LCD_SetFontSize(FONT_SIZE_52);
	LCD_SetFontColor(BRRED);
	LCD_SetFontBgColor(GREEN);
	LCD_MeasureUniString(ret_str[ft_pressure_checked], &w, &h);
	LCD_ShowUniString(FT_PRESSURE_RET_STR_X+(FT_PRESSURE_RET_STR_W-w)/2, FT_PRESSURE_RET_STR_Y+(FT_PRESSURE_RET_STR_H-h)/2, ret_str[ft_pressure_checked]);
	LCD_ReSetFontBgColor();
	LCD_ReSetFontColor();

	switch(g_ft_status)
	{
	case FT_STATUS_SMT:
		if(ft_pressure_checked)
			ft_smt_results.pressure_ret = 1;
		else
			ft_smt_results.pressure_ret = 2;
		
		SaveFactoryTestResults(FT_STATUS_SMT, &ft_smt_results);
		break;
		
	case FT_STATUS_ASSEM:
		if(ft_pressure_checked)
			ft_assem_results.pressure_ret = 1;
		else
			ft_assem_results.pressure_ret = 2;
		
		SaveFactoryTestResults(FT_STATUS_ASSEM, &ft_assem_results);
		break;
	}
}

static void FTMenuPressureShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[8] = {0x6C14,0x538B,0x6D4B,0x8BD5,0x0000};//气压测试
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

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_PRESSURE_TITLE_X+(FT_PRESSURE_TITLE_W-w)/2, FT_PRESSURE_TITLE_Y, title_str);

	LCD_MeasureUniString(notify_str, &w, &h);
	LCD_ShowUniString(FT_PRESSURE_STR_X+(FT_PRESSURE_STR_W-w)/2, FT_PRESSURE_STR_Y+(FT_PRESSURE_STR_H-h)/2, notify_str);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_PRESSURE_SLE1_STR_X+(FT_PRESSURE_SLE1_STR_W-w)/2;
	y = FT_PRESSURE_SLE1_STR_Y+(FT_PRESSURE_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_PRESSURE_SLE1_STR_X, FT_PRESSURE_SLE1_STR_Y, FT_PRESSURE_SLE1_STR_W, FT_PRESSURE_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_PRESSURE_SLE2_STR_X+(FT_PRESSURE_SLE2_STR_W-w)/2;
	y = FT_PRESSURE_SLE2_STR_Y+(FT_PRESSURE_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_PRESSURE_SLE2_STR_X, FT_PRESSURE_SLE2_STR_Y, FT_PRESSURE_SLE2_STR_W, FT_PRESSURE_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuPressureSle1Hander);
	SetRightKeyUpHandler(FTMenuPressureSle2Hander);
		
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_PRESSURE_SLE1_STR_X, FT_PRESSURE_SLE1_STR_X+FT_PRESSURE_SLE1_STR_W, FT_PRESSURE_SLE1_STR_Y, FT_PRESSURE_SLE1_STR_Y+FT_PRESSURE_SLE1_STR_H, FTMenuPressureSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_PRESSURE_SLE2_STR_X, FT_PRESSURE_SLE2_STR_X+FT_PRESSURE_SLE2_STR_W, FT_PRESSURE_SLE2_STR_Y, FT_PRESSURE_SLE2_STR_Y+FT_PRESSURE_SLE2_STR_H, FTMenuPressureSle2Hander);
#endif	
}

void FTMenuPressureProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuPressureShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuPressureUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}

	if(ft_pressure_start_check)
	{
		FTPressureStatusUpdate();
		ft_pressure_start_check = false;
	}
}

void ExitFTMenuPressure(void)
{
	k_timer_stop(&pressure_test_timer);
	ReturnFTMainMenu();
}

void EnterFTMenuPressure(void)
{
	ft_pressure_start_check = false;
	ft_pressure_checked = false;
	memcpy(&ft_menu, &FT_MENU_PRESSURE, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;

	k_timer_start(&pressure_test_timer, K_MSEC(FT_PRESSURE_DELAY_CHECK), K_NO_WAIT);
}
