/****************************************Copyright (c)************************************************
** File Name:			    ft_sim.c
** Descriptions:			Factory test sim card source file
** Created By:				xie biao
** Created Date:			2023-03-13
** Modified Date:      		2023-03-13 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "nb.h"
#include "ft_main.h"
#include "ft_sim.h"
	
#define FT_SIM_TITLE_W				150
#define FT_SIM_TITLE_H				40
#define FT_SIM_TITLE_X				((LCD_WIDTH-FT_SIM_TITLE_W)/2)
#define FT_SIM_TITLE_Y				20

#define FT_SIM_MENU_STR_W			150
#define FT_SIM_MENU_STR_H			25
#define FT_SIM_MENU_STR_X			((LCD_WIDTH-FT_SIM_MENU_STR_W)/2)
#define FT_SIM_MENU_STR_Y			60
#define FT_SIM_MENU_STR_OFFSET_Y	5

#define FT_SIM_SLE1_STR_W			70
#define FT_SIM_SLE1_STR_H			30
#define FT_SIM_SLE1_STR_X			40
#define FT_SIM_SLE1_STR_Y			170
#define FT_SIM_SLE2_STR_W			70
#define FT_SIM_SLE2_STR_H			30
#define FT_SIM_SLE2_STR_X			130
#define FT_SIM_SLE2_STR_Y			170

#define FT_SIM_RET_STR_W			120
#define FT_SIM_RET_STR_H			60
#define FT_SIM_RET_STR_X			((LCD_WIDTH-FT_SIM_RET_STR_W)/2)
#define FT_SIM_RET_STR_Y			((LCD_HEIGHT-FT_SIM_RET_STR_H)/2)

#define FT_SIM_NOTIFY_W				200
#define FT_SIM_NOTIFY_H				40
#define FT_SIM_NOTIFY_X				((LCD_WIDTH-FT_SIM_NOTIFY_W)/2)
#define FT_SIM_NOTIFY_Y				100
	
#define FT_SIM_TEST_TIMEROUT	1

static uint8_t ft_sim_status = 0;//1:turn on modem 2:get the imsi/iccic 3:got the imsi/iccid
static bool ft_sim_check_ok = false;
static bool ft_sim_checking = false;

static void SIMTestTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(sim_test_timer, SIMTestTimerOutCallBack, NULL);

static void FTMenuSIMDumpProc(void){}

const ft_menu_t FT_MENU_SIM = 
{
	FT_SIM,
	0,
	0,
	{
		{
			{0x0000},
			FTMenuSIMDumpProc,
		},
	},
	{	
		//page proc func
		FTMenuSIMDumpProc,
		FTMenuSIMDumpProc,
		FTMenuSIMDumpProc,
		FTMenuSIMDumpProc,
	},
};

static void FTMenuSIMSle1Hander(void)
{
	FT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
}

static void FTMenuSIMSle2Hander(void)
{
	ExitFTMenuSIM();
}

static void SIMTestTimerOutCallBack(struct k_timer *timer_id)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_SIM))
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuSIMUpdate(void)
{
	uint16_t w,h;
	uint16_t imsi_str[6] = {0x0049,0x004D,0x0053,0x0049,0x003A,0x0000};			//IMSI:
	uint16_t iccid_str[8] = {0x0049,0x0043,0x0043,0x0049,0x0044,0x003A,0x0000};	//ICCID:
	uint16_t ret_str[2][5] = {
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
							  };

	if(ft_sim_checking)
	{
		uint8_t tmpbuf[256] = {0};
		
		switch(ft_sim_status)
		{
		case 0:
			break;
		case 1:
			ft_sim_status = 2;
			GetModemInfor();
			k_timer_start(&sim_test_timer, K_SECONDS(FT_SIM_TEST_TIMEROUT), K_NO_WAIT);
			break;
		case 2:
			ft_sim_status = 3;
			k_timer_start(&sim_test_timer, K_SECONDS(FT_SIM_TEST_TIMEROUT), K_NO_WAIT);
			break;
		case 3:
			ft_sim_checking = false;
			SetModemTurnOff();
			LCD_SetFontSize(FONT_SIZE_28);

			LCD_Fill(FT_SIM_NOTIFY_X, FT_SIM_NOTIFY_Y, FT_SIM_NOTIFY_W, FT_SIM_NOTIFY_H, BLACK);
			LCD_MeasureUniString(imsi_str, &w, &h);
			LCD_ShowUniString(FT_SIM_MENU_STR_X, FT_SIM_MENU_STR_Y+(FT_SIM_MENU_STR_H-h)/2+0*FT_SIM_MENU_STR_H, imsi_str);
			LCD_MeasureUniString(iccid_str, &w, &h);
			LCD_ShowUniString(FT_SIM_MENU_STR_X, FT_SIM_MENU_STR_Y+(FT_SIM_MENU_STR_H-h)/2+2*FT_SIM_MENU_STR_H, iccid_str);

			if((strlen(g_imsi) > 0)&&(strlen(g_iccid) > 0))
			{
				ft_sim_check_ok = true;
				ft_menu_checked[ft_main_menu_index] = true;
				
				LCD_SetFontSize(FONT_SIZE_20);
				mmi_asc_to_ucs2(tmpbuf, g_imsi);
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(FT_SIM_MENU_STR_X, FT_SIM_MENU_STR_Y+(FT_SIM_MENU_STR_H-h)/2+1*FT_SIM_MENU_STR_H, tmpbuf);
				mmi_asc_to_ucs2(tmpbuf, g_iccid);
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(FT_SIM_MENU_STR_X, FT_SIM_MENU_STR_Y+(FT_SIM_MENU_STR_H-h)/2+3*FT_SIM_MENU_STR_H, tmpbuf);
			}
			else
			{
				ft_sim_check_ok = false;
			}

			k_timer_start(&sim_test_timer, K_SECONDS(FT_SIM_TEST_TIMEROUT), K_NO_WAIT);
			break;
		}
	}
	else
	{
		LCD_Set_BL_Mode(LCD_BL_AUTO);

		//pass or fail
		LCD_SetFontSize(FONT_SIZE_52);
		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);
		LCD_MeasureUniString(ret_str[ft_sim_check_ok], &w, &h);
		LCD_ShowUniString(FT_SIM_RET_STR_X+(FT_SIM_RET_STR_W-w)/2, FT_SIM_RET_STR_Y+(FT_SIM_RET_STR_H-h)/2, ret_str[ft_sim_check_ok]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuSIMSle1Hander);
		SetRightKeyUpHandler(FTMenuSIMSle2Hander);
			
	#ifdef CONFIG_TOUCH_SUPPORT
		clear_all_touch_event_handle();
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_SIM_SLE1_STR_X, FT_SIM_SLE1_STR_X+FT_SIM_SLE1_STR_W, FT_SIM_SLE1_STR_Y, FT_SIM_SLE1_STR_Y+FT_SIM_SLE1_STR_H, FTMenuSIMSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_SIM_SLE2_STR_X, FT_SIM_SLE2_STR_X+FT_SIM_SLE2_STR_W, FT_SIM_SLE2_STR_Y, FT_SIM_SLE2_STR_Y+FT_SIM_SLE2_STR_H, FTMenuSIMSle2Hander);
	#endif	
	}
}

static void FTMenuSIMShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[8] = {0x0053,0x0049,0x004D,0x5361,0x6D4B,0x8BD5,0x0000};//SIM卡测试
	uint16_t sle_str[2][5] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							  };
	uint16_t notify_str[9] = {0x6B63,0x5728,0x83B7,0x53D6,0x4FE1,0x606F,0x2026,0x0000};//正在获取信息…

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif

	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_SIM_TITLE_X+(FT_SIM_TITLE_W-w)/2, FT_SIM_TITLE_Y, title_str);
	LCD_MeasureUniString(notify_str, &w, &h);
	LCD_ShowUniString(FT_SIM_NOTIFY_X+(FT_SIM_NOTIFY_W-w)/2, FT_SIM_NOTIFY_Y+(FT_SIM_NOTIFY_H-h)/2, notify_str);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_SIM_SLE1_STR_X+(FT_SIM_SLE1_STR_W-w)/2;
	y = FT_SIM_SLE1_STR_Y+(FT_SIM_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_SIM_SLE1_STR_X, FT_SIM_SLE1_STR_Y, FT_SIM_SLE1_STR_W, FT_SIM_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_SIM_SLE2_STR_X+(FT_SIM_SLE2_STR_W-w)/2;
	y = FT_SIM_SLE2_STR_Y+(FT_SIM_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_SIM_SLE2_STR_X, FT_SIM_SLE2_STR_Y, FT_SIM_SLE2_STR_W, FT_SIM_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuSIMSle1Hander);
	SetRightKeyUpHandler(FTMenuSIMSle2Hander);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_SIM_SLE1_STR_X, FT_SIM_SLE1_STR_X+FT_SIM_SLE1_STR_W, FT_SIM_SLE1_STR_Y, FT_SIM_SLE1_STR_Y+FT_SIM_SLE1_STR_H, FTMenuSIMSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_SIM_SLE2_STR_X, FT_SIM_SLE2_STR_X+FT_SIM_SLE2_STR_W, FT_SIM_SLE2_STR_Y, FT_SIM_SLE2_STR_Y+FT_SIM_SLE2_STR_H, FTMenuSIMSle2Hander);
#endif		
}

void FTMenuSIMProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuSIMShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuSIMUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void ExitFTMenuSIM(void)
{
	ft_sim_status = 0;
	ft_sim_checking = false;
	ft_sim_check_ok = false;
	k_timer_stop(&sim_test_timer);
	SetModemTurnOff();
	ReturnFTMainMenu();
}

void EnterFTMenuSIM(void)
{
	ft_sim_status = 0;
	ft_sim_check_ok = false;
	ft_sim_checking = true;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_SIM, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;

	if((strlen(g_imsi) == 0)||(strlen(g_iccid) == 0))
	{
		ft_sim_status = 1;
		SetModemTurnOn();
	}
	else
	{
		ft_sim_status = 3;
	}
	
	k_timer_start(&sim_test_timer, K_SECONDS(FT_SIM_TEST_TIMEROUT), K_NO_WAIT);
}

