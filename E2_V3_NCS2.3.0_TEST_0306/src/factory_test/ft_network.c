/****************************************Copyright (c)************************************************
** File Name:			    ft_network.c
** Descriptions:			Factory test network module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "nb.h"
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_network.h"
				
#define FT_NET_TITLE_W				150
#define FT_NET_TITLE_H				40
#define FT_NET_TITLE_X				((LCD_WIDTH-FT_NET_TITLE_W)/2)
#define FT_NET_TITLE_Y				20

#define FT_NET_MENU_STR_W			150
#define FT_NET_MENU_STR_H			30
#define FT_NET_MENU_STR_X			((LCD_WIDTH-FT_NET_MENU_STR_W)/2)
#define FT_NET_MENU_STR_Y			80
#define FT_NET_MENU_STR_OFFSET_Y	5

#define FT_NET_SLE1_STR_W			70
#define FT_NET_SLE1_STR_H			30
#define FT_NET_SLE1_STR_X			40
#define FT_NET_SLE1_STR_Y			170
#define FT_NET_SLE2_STR_W			70
#define FT_NET_SLE2_STR_H			30
#define FT_NET_SLE2_STR_X			130
#define FT_NET_SLE2_STR_Y			170

#define FT_NET_PASS_STR_W			80
#define FT_NET_PASS_STR_H			40
#define FT_NET_PASS_STR_X			30
#define FT_NET_PASS_STR_Y			((LCD_WIDTH-FT_NET_PASS_STR_H)/2)
#define FT_NET_FAIL_STR_W			80
#define FT_NET_FAIL_STR_H			40
#define FT_NET_FAIL_STR_X			130
#define FT_NET_FAIL_STR_Y			((LCD_HEIGHT-FT_NET_FAIL_STR_H)/2)

#define FT_NET_RET_STR_W			120
#define FT_NET_RET_STR_H			60
#define FT_NET_RET_STR_X			((LCD_WIDTH-FT_NET_RET_STR_W)/2)
#define FT_NET_RET_STR_Y			((LCD_HEIGHT-FT_NET_RET_STR_H)/2)

#define FT_NET_NOTIFY_W				200
#define FT_NET_NOTIFY_H				40
#define FT_NET_NOTIFY_X				((LCD_WIDTH-FT_NET_NOTIFY_W)/2)
#define FT_NET_NOTIFY_Y				100

#if CONFIG_LTE_NETWORK_USE_FALLBACK
#define FT_NET_TEST_TIMEROUT	(CONFIG_LTE_NETWORK_TIMEOUT*2)
#else
#define FT_NET_TEST_TIMEROUT	(CONFIG_LTE_NETWORK_TIMEOUT*1)
#endif

static bool ft_net_check_ok = false;
static bool ft_net_checking = false;
static bool update_show_flag = false;

static void NetTestTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(net_test_timer, NetTestTimerOutCallBack, NULL);

static void FTMenuNetDumpProc(void){}

const ft_menu_t FT_MENU_NET = 
{
	FT_NET,
	0,
	2,
	{
		//按任意键启动
		{
			{0x6309,0x4EFB,0x610F,0x952E,0x542F,0x52A8,0x6D4B,0x8BD5,0x0000},
			FTMenuNetDumpProc,
		},
		//测试时按任意键退出
		{
			{0x6D4B,0x8BD5,0x65F6,0x6309,0x4EFB,0x610F,0x952E,0x9000,0x51FA,0x0000},
			FTMenuNetDumpProc,
		},
	},
	{	
		//page proc func
		FTMenuNetDumpProc,
		FTMenuNetDumpProc,
		FTMenuNetDumpProc,
		FTMenuNetDumpProc,
	},
};

static void FTMenuNetStopTest(void)
{
	ft_net_checking = false;
	k_timer_stop(&net_test_timer);
	FTStopNet();
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuNetStartTest(void)
{
	ft_net_checking = true;
	FTStartNet();
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;

	if(1)//(nb_is_chinese_sim())
		k_timer_start(&net_test_timer, K_SECONDS(FT_NET_TEST_TIMEROUT), K_NO_WAIT);
}

static void FTMenuNetPassHander(void)
{
	ft_menu_checked[ft_main_menu_index] = true;
	ft_results.net_ret = 1;
	SaveFactoryTestResults(ft_results);

	FT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
}

static void FTMenuNetFailHander(void)
{
	ft_menu_checked[ft_main_menu_index] = false;
	ft_results.net_ret = 2;
	SaveFactoryTestResults(ft_results);

	FT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
}

static void FTMenuNetSle1Hander(void)
{
	FTMenuNetStopTest();

	FT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
}

static void FTMenuNetSle2Hander(void)
{
	ExitFTMenuNet();
}

static void NetTestTimerOutCallBack(struct k_timer *timer_id)
{
	FTMenuNetStopTest();
}

static void FTMenuNetUpdate(void)
{
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };
	uint16_t notify_str[9] = {0x6B63,0x5728,0x83B7,0x53D6,0x4FE1,0x53F7,0x2026,0x0000};//正在获取信号…

	if(ft_net_checking)
	{
		uint8_t tmpbuf[512] = {0};
		
		if(!update_show_flag)
		{
			update_show_flag = true;
			
			LCD_Fill(FT_NET_MENU_STR_X, FT_NET_MENU_STR_Y, FT_NET_MENU_STR_W, 2*(FT_NET_MENU_STR_H+FT_NET_MENU_STR_OFFSET_Y), BLACK);
			
			LCD_SetFontSize(FONT_SIZE_28);
			LCD_MeasureUniString(notify_str, &w, &h);
			LCD_ShowUniString(FT_NET_NOTIFY_X+(FT_NET_NOTIFY_W-w)/2, FT_NET_NOTIFY_Y+(FT_NET_NOTIFY_H-h)/2, notify_str);

			ClearAllKeyHandler();
			SetLeftKeyUpHandler(FTMenuNetStopTest);
			SetRightKeyUpHandler(FTMenuNetStopTest);
		}
		else
		{
			LCD_Fill(FT_NET_NOTIFY_X, FT_NET_NOTIFY_Y, FT_NET_NOTIFY_W, FT_NET_NOTIFY_H, BLACK);
			
			LCD_SetFontSize(FONT_SIZE_20);
			LCD_Fill((LCD_WIDTH-160)/2, 60, 160, 100, BLACK);
			mmi_asc_to_ucs2(tmpbuf, nb_test_info);
			LCD_ShowUniStringInRect((LCD_WIDTH-160)/2, 60, 160, 100, (uint16_t*)tmpbuf);
		}
	}
	else
	{
		update_show_flag = false;
		LCD_Set_BL_Mode(LCD_BL_AUTO);

		//pass and fail
		LCD_SetFontSize(FONT_SIZE_28);
		LCD_MeasureUniString(ret_str[0], &w, &h);
		x = FT_NET_PASS_STR_X+(FT_NET_PASS_STR_W-w)/2;
		y = FT_NET_PASS_STR_Y+(FT_NET_PASS_STR_H-h)/2;
		LCD_Fill(FT_NET_PASS_STR_X, FT_NET_PASS_STR_Y, FT_NET_PASS_STR_W, FT_NET_PASS_STR_H, BLACK);
		LCD_DrawRectangle(FT_NET_PASS_STR_X, FT_NET_PASS_STR_Y, FT_NET_PASS_STR_W, FT_NET_PASS_STR_H);
		LCD_ShowUniString(x, y, ret_str[0]);
		
		LCD_MeasureUniString(ret_str[1], &w, &h);
		x = FT_NET_FAIL_STR_X+(FT_NET_FAIL_STR_W-w)/2;
		y = FT_NET_FAIL_STR_Y+(FT_NET_FAIL_STR_H-h)/2;
		LCD_Fill(FT_NET_FAIL_STR_X, FT_NET_FAIL_STR_Y, FT_NET_FAIL_STR_W, FT_NET_FAIL_STR_H, BLACK);
		LCD_DrawRectangle(FT_NET_FAIL_STR_X, FT_NET_FAIL_STR_Y, FT_NET_FAIL_STR_W, FT_NET_FAIL_STR_H);
		LCD_ShowUniString(x, y, ret_str[1]);

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuNetSle1Hander);
		SetRightKeyUpHandler(FTMenuNetSle2Hander);
			
	#ifdef CONFIG_TOUCH_SUPPORT
		clear_all_touch_event_handle();
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_NET_SLE1_STR_X, FT_NET_SLE1_STR_X+FT_NET_SLE1_STR_W, FT_NET_SLE1_STR_Y, FT_NET_SLE1_STR_Y+FT_NET_SLE1_STR_H, FTMenuNetSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_NET_SLE2_STR_X, FT_NET_SLE2_STR_X+FT_NET_SLE2_STR_W, FT_NET_SLE2_STR_Y, FT_NET_SLE2_STR_Y+FT_NET_SLE2_STR_H, FTMenuNetSle2Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_NET_PASS_STR_X, FT_NET_PASS_STR_X+FT_NET_PASS_STR_W, FT_NET_PASS_STR_Y, FT_NET_PASS_STR_Y+FT_NET_PASS_STR_H, FTMenuNetPassHander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_NET_FAIL_STR_X, FT_NET_FAIL_STR_X+FT_NET_FAIL_STR_W, FT_NET_FAIL_STR_Y, FT_NET_FAIL_STR_Y+FT_NET_FAIL_STR_H, FTMenuNetFailHander);		
	#endif	
	}
}

static void FTMenuNetShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[8] = {0x7F51,0x7EDC,0x6D4B,0x8BD5,0x0000};//网络测试
	uint16_t sle_str[2][5] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							  };
	uint16_t notify_str[9] = {0x6B63,0x5728,0x83B7,0x53D6,0x4FE1,0x53F7,0x2026,0x0000};//正在获取信号…

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_NET_TITLE_X+(FT_NET_TITLE_W-w)/2, FT_NET_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_20);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.item[i].name, &w, &h);
		LCD_ShowUniString(FT_NET_MENU_STR_X+(FT_NET_MENU_STR_W-w)/2, FT_NET_MENU_STR_Y+(FT_NET_MENU_STR_H-h)/2+i*(FT_NET_MENU_STR_H+FT_NET_MENU_STR_OFFSET_Y), ft_menu.item[i].name);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_NET_MENU_STR_X, 
									FT_NET_MENU_STR_X+FT_NET_MENU_STR_W, 
									FT_NET_MENU_STR_Y+i*(FT_NET_MENU_STR_H+FT_NET_MENU_STR_OFFSET_Y), 
									FT_NET_MENU_STR_Y+i*(FT_NET_MENU_STR_H+FT_NET_MENU_STR_OFFSET_Y)+FT_NET_MENU_STR_H, 
									ft_menu.item[i].sel_handler);
	#endif
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_NET_SLE1_STR_X+(FT_NET_SLE1_STR_W-w)/2;
	y = FT_NET_SLE1_STR_Y+(FT_NET_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_NET_SLE1_STR_X, FT_NET_SLE1_STR_Y, FT_NET_SLE1_STR_W, FT_NET_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_NET_SLE2_STR_X+(FT_NET_SLE2_STR_W-w)/2;
	y = FT_NET_SLE2_STR_Y+(FT_NET_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_NET_SLE2_STR_X, FT_NET_SLE2_STR_Y, FT_NET_SLE2_STR_W, FT_NET_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuNetStartTest);
	SetRightKeyUpHandler(FTMenuNetStartTest);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_NET_SLE1_STR_X, FT_NET_SLE1_STR_X+FT_NET_SLE1_STR_W, FT_NET_SLE1_STR_Y, FT_NET_SLE1_STR_Y+FT_NET_SLE1_STR_H, FTMenuNetSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_NET_SLE2_STR_X, FT_NET_SLE2_STR_X+FT_NET_SLE2_STR_W, FT_NET_SLE2_STR_Y, FT_NET_SLE2_STR_Y+FT_NET_SLE2_STR_H, FTMenuNetSle2Hander);
#endif		
}

void FTMenuNetProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuNetShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuNetUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void FTNetStatusUpdate(uint8_t rssp)
{
	static uint8_t count = 0;

	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_NET))
	{
	#if 0	//xb add 2023.07.25 手动检验是否合格
		if((rssp > 0)&&(rssp < 255))
		{
			count++;
			if(count > 3)
			{
				count = 0;
				ft_net_check_ok = true;
				ft_menu_checked[ft_main_menu_index] = true;
				FTMenuNetStopTest();
			}
		}
	#endif
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}
}

void IsFTNetTesting(void)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_NET))
		return true;
	else
		return false;
}

void ExitFTMenuNet(void)
{
	FTMenuNetStopTest();

	ft_net_checking = false;
	ft_net_check_ok = false;
	k_timer_stop(&net_test_timer);
	ReturnFTMainMenu();
}

void EnterFTMenuNet(void)
{
	ft_net_check_ok = false;
	update_show_flag = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_NET, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST;
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

