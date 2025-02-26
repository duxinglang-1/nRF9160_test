/****************************************Copyright (c)************************************************
** File Name:			    ft_viabrate.c
** Descriptions:			Factory test viabrate module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "max20353.h"
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_vibrate.h"

#define FT_VIB_TITLE_W				100
#define FT_VIB_TITLE_H				40
#define FT_VIB_TITLE_X				((LCD_WIDTH-FT_VIB_TITLE_W)/2)
#define FT_VIB_TITLE_Y				20

#define FT_VIB_MENU_STR_W			150
#define FT_VIB_MENU_STR_H			30
#define FT_VIB_MENU_STR_X			((LCD_WIDTH-FT_VIB_MENU_STR_W)/2)
#define FT_VIB_MENU_STR_Y			80
#define FT_VIB_MENU_STR_OFFSET_Y	5

#define FT_VIB_SLE1_STR_W			70
#define FT_VIB_SLE1_STR_H			30
#define FT_VIB_SLE1_STR_X			40
#define FT_VIB_SLE1_STR_Y			170
#define FT_VIB_SLE2_STR_W			70
#define FT_VIB_SLE2_STR_H			30
#define FT_VIB_SLE2_STR_X			130
#define FT_VIB_SLE2_STR_Y			170

#define FT_VIB_PASS_STR_W			80
#define FT_VIB_PASS_STR_H			40
#define FT_VIB_PASS_STR_X			30
#define FT_VIB_PASS_STR_Y			((LCD_WIDTH-FT_VIB_PASS_STR_H)/2)
#define FT_VIB_FAIL_STR_W			80
#define FT_VIB_FAIL_STR_H			40
#define FT_VIB_FAIL_STR_X			130
#define FT_VIB_FAIL_STR_Y			((LCD_WIDTH-FT_VIB_FAIL_STR_H)/2)

#define FT_VIB_RET_STR_W			120
#define FT_VIB_RET_STR_H			60
#define FT_VIB_RET_STR_X			((LCD_WIDTH-FT_VIB_RET_STR_W)/2)
#define FT_VIB_RET_STR_Y			((LCD_HEIGHT-FT_VIB_RET_STR_H)/2)
#define FT_VIB_STATUS_STR_W			150
#define FT_VIB_STATUS_STR_H			40
#define FT_VIB_STATUS_STR_X			((LCD_WIDTH-FT_VIB_STATUS_STR_W)/2)
#define FT_VIB_STATUS_STR_Y			100	

static bool ft_vib_working = false;
static bool ft_vib_start_flag = false;
static bool ft_vib_testing = false;
static bool update_show_flag = false;

static void VibDelayStartCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(vib_test_timer, VibDelayStartCallBack, NULL);

static void FTMenuVibDumpProc(void){}

const ft_menu_t FT_MENU_VIB = 
{
	FT_VIBRATE,
	0,
	2,
	{
		//按任意键启动
		{
			{0x6309,0x4EFB,0x610F,0x952E,0x542F,0x52A8,0x6D4B,0x8BD5,0x0000},
			FTMenuVibDumpProc,
		},
		//测试时按任意键退出
		{
			{0x6D4B,0x8BD5,0x65F6,0x6309,0x4EFB,0x610F,0x952E,0x9000,0x51FA,0x0000},
			FTMenuVibDumpProc,
		},
	},
	{	
		//page proc func
		FTMenuVibDumpProc,
		FTMenuVibDumpProc,
		FTMenuVibDumpProc,
		FTMenuVibDumpProc,
	},
};

static void FTMenuVibPassHander(void)
{
	vibrate_off();
	
	ft_menu_checked[ft_main_menu_index] = true;
	switch(g_ft_status)
	{
	case FT_STATUS_SMT:
		ft_smt_results.vib_ret = 1;
		SaveFactoryTestResults(FT_STATUS_SMT, &ft_smt_results);
		FT_SMT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
		break;
		
	case FT_STATUS_ASSEM:
		ft_assem_results.vib_ret = 1;
		SaveFactoryTestResults(FT_STATUS_ASSEM, &ft_assem_results);
		FT_ASSEM_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
		break;
	}	
}

static void FTMenuVibFailHander(void)
{
	vibrate_off();
	
	ft_menu_checked[ft_main_menu_index] = false;
	switch(g_ft_status)
	{
	case FT_STATUS_SMT:
		ft_smt_results.vib_ret = 2;
		SaveFactoryTestResults(FT_STATUS_SMT, &ft_smt_results);
		FT_SMT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
		break;
		
	case FT_STATUS_ASSEM:
		ft_assem_results.vib_ret = 2;
		SaveFactoryTestResults(FT_STATUS_ASSEM, &ft_assem_results);
		FT_ASSEM_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
		break;
	}
	
}

static void FTMenuVibSle1Hander(void)
{
	vibrate_off();

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

static void FTMenuVibSle2Hander(void)
{
	ExitFTMenuVibrate();
}

static void FTMenuVibStopTest(void)
{
	ft_vib_testing = false;
	vibrate_off();
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuVibStartTest(void)
{
	ft_vib_testing = true;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	k_timer_start(&vib_test_timer, K_MSEC(1000), K_NO_WAIT);
}

static void VibDelayStartCallBack(struct k_timer *timer_id)
{
	ft_vib_start_flag = true;
}

static void FTMenuVibUpdate(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x9707,0x52A8,0x6D4B,0x8BD5,0x0000};//震动测试
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };	
	uint16_t status_str[2][5] = {
									{0x9707,0x52A8,0x505C,0x6B62,0x0000},//震动停止
									{0x9707,0x52A8,0x542F,0x52A8,0x0000},//震动启动
								};

	if(ft_vib_testing)
	{	
		if(!update_show_flag)
		{
			update_show_flag = true;
			
			LCD_SetFontSize(FONT_SIZE_28);

			LCD_Fill(FT_VIB_MENU_STR_X, FT_VIB_MENU_STR_Y, FT_VIB_MENU_STR_W, 2*(FT_VIB_MENU_STR_H+FT_VIB_MENU_STR_OFFSET_Y), BLACK);
			
			ClearAllKeyHandler();
			SetLeftKeyUpHandler(FTMenuVibStopTest);
			SetRightKeyUpHandler(FTMenuVibStopTest);
		}

		LCD_MeasureUniString(status_str[ft_vib_working], &w, &h);
		LCD_ShowUniString(FT_VIB_STATUS_STR_X+(FT_VIB_STATUS_STR_W-w)/2, FT_VIB_STATUS_STR_Y+(FT_VIB_STATUS_STR_H-h)/2, status_str[ft_vib_working]);
	}
	else
	{
		update_show_flag = false;
		
		LCD_SetFontSize(FONT_SIZE_28);

		LCD_Fill(FT_VIB_STATUS_STR_X, FT_VIB_STATUS_STR_Y, FT_VIB_STATUS_STR_W, FT_VIB_STATUS_STR_H, BLACK);

		LCD_MeasureUniString(ret_str[0], &w, &h);
		x = FT_VIB_PASS_STR_X+(FT_VIB_PASS_STR_W-w)/2;
		y = FT_VIB_PASS_STR_Y+(FT_VIB_PASS_STR_H-h)/2;
		LCD_DrawRectangle(FT_VIB_PASS_STR_X, FT_VIB_PASS_STR_Y, FT_VIB_PASS_STR_W, FT_VIB_PASS_STR_H);
		LCD_ShowUniString(x, y, ret_str[0]);
		LCD_MeasureUniString(ret_str[1], &w, &h);
		x = FT_VIB_FAIL_STR_X+(FT_VIB_FAIL_STR_W-w)/2;
		y = FT_VIB_FAIL_STR_Y+(FT_VIB_FAIL_STR_H-h)/2;
		LCD_DrawRectangle(FT_VIB_FAIL_STR_X, FT_VIB_FAIL_STR_Y, FT_VIB_FAIL_STR_W, FT_VIB_FAIL_STR_H);
		LCD_ShowUniString(x, y, ret_str[1]);

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuVibSle1Hander);
		SetRightKeyUpHandler(FTMenuVibSle2Hander);

	#ifdef CONFIG_TOUCH_SUPPORT
		clear_all_touch_event_handle();
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_VIB_SLE1_STR_X, FT_VIB_SLE1_STR_X+FT_VIB_SLE1_STR_W, FT_VIB_SLE1_STR_Y, FT_VIB_SLE1_STR_Y+FT_VIB_SLE1_STR_H, FTMenuVibSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_VIB_SLE2_STR_X, FT_VIB_SLE2_STR_X+FT_VIB_SLE2_STR_W, FT_VIB_SLE2_STR_Y, FT_VIB_SLE2_STR_Y+FT_VIB_SLE2_STR_H, FTMenuVibSle2Hander);	
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_VIB_PASS_STR_X, FT_VIB_PASS_STR_X+FT_VIB_PASS_STR_W, FT_VIB_PASS_STR_Y, FT_VIB_PASS_STR_Y+FT_VIB_PASS_STR_H, FTMenuVibPassHander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_VIB_FAIL_STR_X, FT_VIB_FAIL_STR_X+FT_VIB_FAIL_STR_W, FT_VIB_FAIL_STR_Y, FT_VIB_FAIL_STR_Y+FT_VIB_FAIL_STR_H, FTMenuVibFailHander);	
	#endif
	}
}

static void FTMenuVibShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x9707,0x52A8,0x6D4B,0x8BD5,0x0000};//震动测试
	uint16_t sle_str[2][5] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							 };
	uint16_t status_str[2][5] = {
									{0x9707,0x52A8,0x505C,0x6B62,0x0000},//震动停止
									{0x9707,0x52A8,0x542F,0x52A8,0x0000},//震动启动
								};

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_VIB_TITLE_X+(FT_VIB_TITLE_W-w)/2, FT_VIB_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_20);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.item[i].name, &w, &h);
		LCD_ShowUniString(FT_VIB_MENU_STR_X+(FT_VIB_MENU_STR_W-w)/2, FT_VIB_MENU_STR_Y+(FT_VIB_MENU_STR_H-h)/2+i*(FT_VIB_MENU_STR_H+FT_VIB_MENU_STR_OFFSET_Y), ft_menu.item[i].name);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_VIB_MENU_STR_X, 
									FT_VIB_MENU_STR_X+FT_VIB_MENU_STR_W, 
									FT_VIB_MENU_STR_Y+i*(FT_VIB_MENU_STR_H+FT_VIB_MENU_STR_OFFSET_Y), 
									FT_VIB_MENU_STR_Y+i*(FT_VIB_MENU_STR_H+FT_VIB_MENU_STR_OFFSET_Y)+FT_VIB_MENU_STR_H, 
									ft_menu.item[i].sel_handler);
	#endif
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_VIB_SLE1_STR_X+(FT_VIB_SLE1_STR_W-w)/2;
	y = FT_VIB_SLE1_STR_Y+(FT_VIB_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_VIB_SLE1_STR_X, FT_VIB_SLE1_STR_Y, FT_VIB_SLE1_STR_W, FT_VIB_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_VIB_SLE2_STR_X+(FT_VIB_SLE2_STR_W-w)/2;
	y = FT_VIB_SLE2_STR_Y+(FT_VIB_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_VIB_SLE2_STR_X, FT_VIB_SLE2_STR_Y, FT_VIB_SLE2_STR_W, FT_VIB_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuVibStartTest);
	SetRightKeyUpHandler(FTMenuVibStartTest);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_VIB_SLE1_STR_X, FT_VIB_SLE1_STR_X+FT_VIB_SLE1_STR_W, FT_VIB_SLE1_STR_Y, FT_VIB_SLE1_STR_Y+FT_VIB_SLE1_STR_H, FTMenuVibSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_VIB_SLE2_STR_X, FT_VIB_SLE2_STR_X+FT_VIB_SLE2_STR_W, FT_VIB_SLE2_STR_Y, FT_VIB_SLE2_STR_Y+FT_VIB_SLE2_STR_H, FTMenuVibSle2Hander);
#endif		
}

void FTMenuVibrateProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuVibShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuVibUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}

	if(ft_vib_start_flag)
	{
		vibrate_on(VIB_RHYTHMIC, 1000, 1000);
		ft_vib_start_flag = false;
	}
}

void FTVibrateStatusUpdate(bool flag)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_VIBRATE))
	{
		ft_vib_working = flag;
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}
}

void ExitFTMenuVibrate(void)
{
	vibrate_off();
	ReturnFTMainMenu();
}

void EnterFTMenuVibrate(void)
{
	update_show_flag = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_VIB, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

