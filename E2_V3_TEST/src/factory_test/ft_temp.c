/****************************************Copyright (c)************************************************
** File Name:			    ft_temp.c
** Descriptions:			Factory test temp module source file
** Created By:				xie biao
** Created Date:			2023-02-17
** Modified Date:      		2023-02-17 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#ifdef CONFIG_TOUCH_SUPPORT
#include "CST816.h"
#endif
#ifdef CONFIG_TEMP_SUPPORT
#include "temp.h"
#endif/*CONFIG_TEMP_SUPPORT*/
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_temp.h"

#define FT_TEMP_TITLE_W				100
#define FT_TEMP_TITLE_H				40
#define FT_TEMP_TITLE_X				((LCD_WIDTH-FT_TEMP_TITLE_W)/2)
#define FT_TEMP_TITLE_Y				20
#define FT_TEMP_MENU_STR_W			150
#define FT_TEMP_MENU_STR_H			30
#define FT_TEMP_MENU_STR_X			((LCD_WIDTH-FT_TEMP_MENU_STR_W)/2)
#define FT_TEMP_MENU_STR_Y			80
#define FT_TEMP_MENU_STR_OFFSET_Y	5
#define FT_TEMP_SLE1_STR_W			70
#define FT_TEMP_SLE1_STR_H			30
#define FT_TEMP_SLE1_STR_X			40
#define FT_TEMP_SLE1_STR_Y			170
#define FT_TEMP_SLE2_STR_W			70
#define FT_TEMP_SLE2_STR_H			30
#define FT_TEMP_SLE2_STR_X			130
#define FT_TEMP_SLE2_STR_Y			170
#define FT_TEMP_RET_STR_W			120
#define FT_TEMP_RET_STR_H			60
#define FT_TEMP_RET_STR_X			((LCD_WIDTH-FT_TEMP_RET_STR_W)/2)
#define FT_TEMP_RET_STR_Y			((LCD_HEIGHT-FT_TEMP_RET_STR_H)/2)
#define FT_TEMP_NUM_W				150
#define FT_TEMP_NUM_H				40
#define FT_TEMP_NUM_X				((LCD_WIDTH-FT_TEMP_NUM_W)/2)
#define FT_TEMP_NUM_Y				100

#define FT_TEMP_TEST_TIMEROUT	30

static bool ft_temp_check_ok = false;
static bool ft_temp_checking = false;

static void TempTestTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(Temp_test_timer, TempTestTimerOutCallBack, NULL);

static void FTMenuTempDumpProc(void){}

const ft_menu_t FT_MENU_TEMP = 
{
	FT_TEMP,
	0,
	2,
	{
		{0x8BF7,0x6234,0x4E0A,0x624B,0x8868,0x0000},						//请戴上手表
		{0x6309,0x4EFB,0x610F,0x952E,0x542F,0x52A8,0x6D4B,0x91CF,0x0000},	//按任意键启动测量
	},
	{
		FTMenuTempDumpProc,
		FTMenuTempDumpProc,
	},
	{	
		//page proc func
		FTMenuTempDumpProc,
		FTMenuTempDumpProc,
		FTMenuTempDumpProc,
		FTMenuTempDumpProc,
	},
};

static void FTMenuTempSle1Hander(void)
{
	FTMainMenu6Proc();
}

static void FTMenuTempSle2Hander(void)
{
	ExitFTMenuTemp();
}

static void TempTestTimerOutCallBack(struct k_timer *timer_id)
{
	ft_temp_checking = false;
}

static void FTMenuTempStopTest(void)
{
	ft_temp_checking = false;
	k_timer_stop(&Temp_test_timer);
	FTStopTemp();
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuTempStartTest(void)
{
	ft_temp_checking = true;
	FTStartTemp();
	k_timer_start(&Temp_test_timer, K_SECONDS(FT_TEMP_TEST_TIMEROUT), K_NO_WAIT);
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

static void FTMenuTempUpdate(void)
{
	static uint8_t count = 0;
	static bool flag = false;
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[10] = {0x6E29,0x5EA6,0x6D4B,0x8BD5,0x0000};//温度测试
	uint16_t sle_str[2][5] = {
 								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
 								{0x9000,0x51FA,0x0000},//退出
 							 };
	uint16_t ret_str[2][5] = {
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
							  };

	if(ft_temp_checking)
	{
		uint8_t tmpbuf[256] = {0};
		
		if(!flag)
		{
			flag = true;
			LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
			LCD_SetFontSize(FONT_SIZE_36);
			//clear menu str
			LCD_Fill(FT_TEMP_MENU_STR_X, FT_TEMP_MENU_STR_Y+0*(FT_TEMP_MENU_STR_H+FT_TEMP_MENU_STR_OFFSET_Y), FT_TEMP_MENU_STR_W, FT_TEMP_MENU_STR_H, BLACK);
			LCD_Fill(FT_TEMP_MENU_STR_X, FT_TEMP_MENU_STR_Y+1*(FT_TEMP_MENU_STR_H+FT_TEMP_MENU_STR_OFFSET_Y), FT_TEMP_MENU_STR_W, FT_TEMP_MENU_STR_H, BLACK);
		}
		//temp num
		sprintf(tmpbuf, "%0.1f", g_temp_skin);
		LCD_MeasureString(tmpbuf,&w,&h);
		x = FT_TEMP_NUM_X+(FT_TEMP_NUM_W-w)/2;
		y = FT_TEMP_NUM_Y+(FT_TEMP_NUM_H-h)/2;
		LCD_Fill(FT_TEMP_NUM_X, FT_TEMP_NUM_Y, FT_TEMP_NUM_W, FT_TEMP_NUM_H, BLACK);
		LCD_ShowString(x,y,tmpbuf);
	}
	else
	{
		flag = false;

		LCD_Set_BL_Mode(LCD_BL_AUTO);
		LCD_SetFontSize(FONT_SIZE_36);

		//title
		LCD_MeasureUniString(title_str, &w, &h);
		LCD_ShowUniString(FT_TEMP_TITLE_X+(FT_TEMP_TITLE_W-w)/2, FT_TEMP_TITLE_Y, title_str);
		//temp date
		LCD_Fill(FT_TEMP_NUM_X, FT_TEMP_NUM_Y, FT_TEMP_NUM_W, FT_TEMP_NUM_H, BLACK);
		//pass or fail
		LCD_SetFontSize(FONT_SIZE_52);
		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);
		LCD_MeasureUniString(ret_str[ft_temp_check_ok], &w, &h);
		LCD_ShowUniString(FT_TEMP_RET_STR_X+(FT_TEMP_RET_STR_W-w)/2, FT_TEMP_RET_STR_Y+(FT_TEMP_RET_STR_H-h)/2, ret_str[ft_temp_check_ok]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();
		//leftsoft key and rightsoft key
		LCD_SetFontSize(FONT_SIZE_28);
		LCD_MeasureUniString(sle_str[0], &w, &h);
		x = FT_TEMP_SLE1_STR_X+(FT_TEMP_SLE1_STR_W-w)/2;
		y = FT_TEMP_SLE1_STR_Y+(FT_TEMP_SLE1_STR_H-h)/2;
		LCD_DrawRectangle(FT_TEMP_SLE1_STR_X, FT_TEMP_SLE1_STR_Y, FT_TEMP_SLE1_STR_W, FT_TEMP_SLE1_STR_H);
		LCD_ShowUniString(x, y, sle_str[0]);
		LCD_MeasureUniString(sle_str[1], &w, &h);
		x = FT_TEMP_SLE2_STR_X+(FT_TEMP_SLE2_STR_W-w)/2;
		y = FT_TEMP_SLE2_STR_Y+(FT_TEMP_SLE2_STR_H-h)/2;
		LCD_DrawRectangle(FT_TEMP_SLE2_STR_X, FT_TEMP_SLE2_STR_Y, FT_TEMP_SLE2_STR_W, FT_TEMP_SLE2_STR_H);
		LCD_ShowUniString(x, y, sle_str[1]);

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuTempSle1Hander);
		SetRightKeyUpHandler(FTMenuTempSle2Hander);
			
	#ifdef CONFIG_TOUCH_SUPPORT
		clear_all_touch_event_handle();
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TEMP_SLE1_STR_X, FT_TEMP_SLE1_STR_X+FT_TEMP_SLE1_STR_W, FT_TEMP_SLE1_STR_Y, FT_TEMP_SLE1_STR_Y+FT_TEMP_SLE1_STR_H, FTMenuTempSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TEMP_SLE2_STR_X, FT_TEMP_SLE2_STR_X+FT_TEMP_SLE2_STR_W, FT_TEMP_SLE2_STR_Y, FT_TEMP_SLE2_STR_Y+FT_TEMP_SLE2_STR_H, FTMenuTempSle2Hander);
	#endif	
	}
}

static void FTMenuTempShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x6E29,0x5EA6,0x6D4B,0x8BD5,0x0000};//温度测试
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
	LCD_ShowUniString(FT_TEMP_TITLE_X+(FT_TEMP_TITLE_W-w)/2, FT_TEMP_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_20);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.name[i], &w, &h);
		LCD_ShowUniString(FT_TEMP_MENU_STR_X+(FT_TEMP_MENU_STR_W-w)/2, FT_TEMP_MENU_STR_Y+i*(FT_TEMP_MENU_STR_H+FT_TEMP_MENU_STR_OFFSET_Y), ft_menu.name[i]);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_TEMP_MENU_STR_X, 
									FT_TEMP_MENU_STR_X+FT_TEMP_MENU_STR_W, 
									FT_TEMP_MENU_STR_Y+i*(FT_TEMP_MENU_STR_H+FT_TEMP_MENU_STR_OFFSET_Y), 
									FT_TEMP_MENU_STR_Y+i*(FT_TEMP_MENU_STR_H+FT_TEMP_MENU_STR_OFFSET_Y)+FT_TEMP_MENU_STR_H, 
									ft_menu.sel_handler[i]);
	#endif
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_TEMP_SLE1_STR_X+(FT_TEMP_SLE1_STR_W-w)/2;
	y = FT_TEMP_SLE1_STR_Y+(FT_TEMP_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_TEMP_SLE1_STR_X, FT_TEMP_SLE1_STR_Y, FT_TEMP_SLE1_STR_W, FT_TEMP_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_TEMP_SLE2_STR_X+(FT_TEMP_SLE2_STR_W-w)/2;
	y = FT_TEMP_SLE2_STR_Y+(FT_TEMP_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_TEMP_SLE2_STR_X, FT_TEMP_SLE2_STR_Y, FT_TEMP_SLE2_STR_W, FT_TEMP_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuTempStartTest);
	SetRightKeyUpHandler(FTMenuTempStartTest);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TEMP_SLE1_STR_X, FT_TEMP_SLE1_STR_X+FT_TEMP_SLE1_STR_W, FT_TEMP_SLE1_STR_Y, FT_TEMP_SLE1_STR_Y+FT_TEMP_SLE1_STR_H, FTMenuTempSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_TEMP_SLE2_STR_X, FT_TEMP_SLE2_STR_X+FT_TEMP_SLE2_STR_W, FT_TEMP_SLE2_STR_Y, FT_TEMP_SLE2_STR_Y+FT_TEMP_SLE2_STR_H, FTMenuTempSle2Hander);
#endif		
}

void FTMenuTempProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuTempShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuTempUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void FTTempStatusUpdate(void)
{
	static uint8_t count;

	count++;
	if(count > 5)
	{
		count = 0;
		ft_temp_check_ok = true;
		FTMenuTempStopTest();
	}
	
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_TEMP))
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

void ExitFTMenuTemp(void)
{
	k_timer_stop(&Temp_test_timer);
	ReturnFTMainMenu();
}

void EnterFTMenuTemp(void)
{
	memcpy(&ft_menu, &FT_MENU_TEMP, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

