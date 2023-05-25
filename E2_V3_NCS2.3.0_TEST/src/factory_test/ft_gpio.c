/****************************************Copyright (c)************************************************
** File Name:			    ft_gpio.c
** Descriptions:			Factory test gpio(key&wrist) module source file
** Created By:				xie biao
** Created Date:			2023-02-20
** Modified Date:      		2023-02-20 
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
#include "ft_main.h"
#include "ft_gpio.h"

#define FT_KEY_TITLE_W				100
#define FT_KEY_TITLE_H				40
#define FT_KEY_TITLE_X				((LCD_WIDTH-FT_KEY_TITLE_W)/2)
#define FT_KEY_TITLE_Y				20

#define FT_KEY_MENU_STR_W			150
#define FT_KEY_MENU_STR_H			30
#define FT_KEY_MENU_STR_X			((LCD_WIDTH-FT_KEY_MENU_STR_W)/2)
#define FT_KEY_MENU_STR_Y			80
#define FT_KEY_MENU_STR_OFFSET_Y	5

#define FT_KEY_SLE1_STR_W			70
#define FT_KEY_SLE1_STR_H			30
#define FT_KEY_SLE1_STR_X			40
#define FT_KEY_SLE1_STR_Y			170
#define FT_KEY_SLE2_STR_W			70
#define FT_KEY_SLE2_STR_H			30
#define FT_KEY_SLE2_STR_X			130
#define FT_KEY_SLE2_STR_Y			170

#define FT_KEY_RET_STR_W			120
#define FT_KEY_RET_STR_H			60
#define FT_KEY_RET_STR_X			((LCD_WIDTH-FT_KEY_RET_STR_W)/2)
#define FT_KEY_RET_STR_Y			((LCD_HEIGHT-FT_KEY_RET_STR_H)/2)

ft_key_t ft_key[KEY_MAX] = {0};

const ft_key_t FT_KEY_INF[KEY_MAX] = 
{
	{KEY_POWER, FT_KEY_NONE},
	{KEY_SOS,	FT_KEY_NONE},
};

static void FTMenuKeyDumpProc(void){}

const ft_menu_t FT_MENU_KEY = 
{
	FT_KEY,
	0,
	2,
	{
		{0x0050,0x004F,0x0057,0x0045,0x0052,0x952E,0x0000},//POWER键
		{0x0053,0x004F,0x0053,0x952E,0x0000},//SOS键
	},	
	{
		FTMenuKeyDumpProc,
		FTMenuKeyDumpProc,
	},
	{	
		//page proc func
		FTMenuKeyDumpProc,
		FTMenuKeyDumpProc,
		FTMenuKeyDumpProc,
		FTMenuKeyDumpProc,
	},
};

static void FTMenuKeySle1Hander(void)
{
	FTMainMenu3Proc();
}

static void FTMenuKeySle2Hander(void)
{
	ExitFTMenuKey();
}

static void FTMenuKeyPowerDownProc(void)
{
	uint8_t i;
	
	for(i=0;i<ARRAY_SIZE(ft_key);i++)
	{
		if((ft_key[i].id == KEY_POWER) && (ft_key[i].status == FT_KEY_NONE))
		{
			ft_key[i].status = FT_KEY_DOWN;
			break;
		}	
	}
}

static void FTMenuKeyPowerUpProc(void)
{
	uint8_t i;
	
	for(i=0;i<ARRAY_SIZE(ft_key);i++)
	{
		if((ft_key[i].id == KEY_POWER) && (ft_key[i].status == FT_KEY_DOWN))
		{
			ft_key[i].status = FT_KEY_UP;
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
			break;
		}	
	}
}

static void FTMenuKeySosDownProc(void)
{
	uint8_t i;
	
	for(i=0;i<ARRAY_SIZE(ft_key);i++)
	{
		if((ft_key[i].id == KEY_SOS) && (ft_key[i].status == FT_KEY_NONE))
		{
			ft_key[i].status = FT_KEY_DOWN;
			break;
		}	
	}
}

static void FTMenuKeySosUpProc(void)
{
	uint8_t i;
	
	for(i=0;i<ARRAY_SIZE(ft_key);i++)
	{
		if((ft_key[i].id == KEY_SOS) && (ft_key[i].status == FT_KEY_DOWN))
		{
			ft_key[i].status = FT_KEY_UP;
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
			break;
		}	
	}
}

static void FTMenukeyUpdate(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };

	for(i=0;i<ARRAY_SIZE(ft_key);i++)
	{
		if(ft_key[i].status == FT_KEY_UP)
		{
			ft_key[i].status = FT_KEY_DONE;
			LCD_Fill(FT_KEY_MENU_STR_X, FT_KEY_MENU_STR_Y+i*(FT_KEY_MENU_STR_H+FT_KEY_MENU_STR_OFFSET_Y), FT_KEY_MENU_STR_W, FT_KEY_MENU_STR_H, BLACK);
			break;
		}	
	}

	for(i=0;i<ARRAY_SIZE(ft_key);i++)
	{
		if(ft_key[i].status != FT_KEY_DONE)
			break;
	}

	if(i == ARRAY_SIZE(ft_key))
	{
		LCD_Set_BL_Mode(LCD_BL_AUTO);
		
		LCD_Fill(FT_KEY_RET_STR_X-2, FT_KEY_RET_STR_Y-2, FT_KEY_RET_STR_W+4, FT_KEY_RET_STR_H+4, GRAY);
		LCD_Fill(FT_KEY_RET_STR_X, FT_KEY_RET_STR_Y, FT_KEY_RET_STR_W, FT_KEY_RET_STR_H, GREEN);

		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);

		LCD_SetFontSize(FONT_SIZE_52);
		LCD_MeasureUniString(ret_str[0], &w, &h);
		LCD_ShowUniString(FT_KEY_RET_STR_X+(FT_KEY_RET_STR_W-w)/2, FT_KEY_RET_STR_Y+(FT_KEY_RET_STR_H-h)/2, ret_str[0]);

		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuKeySle1Hander);
		SetRightKeyUpHandler(FTMenuKeySle2Hander);

		ft_menu_checked[ft_main_menu_index] = true;
	}
}

static void FTMenuKeyShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x6309,0x952E,0x6D4B,0x8BD5,0x0000};//按键测试
	uint16_t sle_str[2][4] = {
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
	LCD_ShowUniString(FT_KEY_TITLE_X+(FT_KEY_TITLE_W-w)/2, FT_KEY_TITLE_Y, title_str);

	LCD_SetFontSize(FONT_SIZE_28);
	for(i=0;i<ft_menu.count;i++)
	{
		LCD_MeasureUniString(ft_menu.name[i], &w, &h);
		LCD_ShowUniString(FT_KEY_MENU_STR_X+(FT_KEY_MENU_STR_W-w)/2, FT_KEY_MENU_STR_Y+(FT_KEY_MENU_STR_H-h)/2+i*(FT_KEY_MENU_STR_H+FT_KEY_MENU_STR_OFFSET_Y), ft_menu.name[i]);

	#ifdef CONFIG_TOUCH_SUPPORT
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, 
									FT_KEY_MENU_STR_X, 
									FT_KEY_MENU_STR_X+FT_KEY_MENU_STR_W, 
									FT_KEY_MENU_STR_Y+i*(FT_KEY_MENU_STR_H+FT_KEY_MENU_STR_OFFSET_Y), 
									FT_KEY_MENU_STR_Y+i*(FT_KEY_MENU_STR_H+FT_KEY_MENU_STR_OFFSET_Y)+FT_KEY_MENU_STR_H, 
									ft_menu.sel_handler[i]);
	#endif
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_KEY_SLE1_STR_X+(FT_KEY_SLE1_STR_W-w)/2;
	y = FT_KEY_SLE1_STR_Y+(FT_KEY_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_KEY_SLE1_STR_X, FT_KEY_SLE1_STR_Y, FT_KEY_SLE1_STR_W, FT_KEY_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_KEY_SLE2_STR_X+(FT_KEY_SLE2_STR_W-w)/2;
	y = FT_KEY_SLE2_STR_Y+(FT_KEY_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_KEY_SLE2_STR_X, FT_KEY_SLE2_STR_Y, FT_KEY_SLE2_STR_W, FT_KEY_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetKeyHandler(FTMenuKeyPowerDownProc, KEY_POWER, KEY_EVENT_DOWN);
	SetKeyHandler(FTMenuKeyPowerUpProc, KEY_POWER, KEY_EVENT_UP);
	SetKeyHandler(FTMenuKeySosDownProc, KEY_SOS, KEY_EVENT_DOWN);
	SetKeyHandler(FTMenuKeySosUpProc, KEY_SOS, KEY_EVENT_UP);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_KEY_SLE1_STR_X, FT_KEY_SLE1_STR_X+FT_KEY_SLE1_STR_W, FT_KEY_SLE1_STR_Y, FT_KEY_SLE1_STR_Y+FT_KEY_SLE1_STR_H, FTMenuKeySle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_KEY_SLE2_STR_X, FT_KEY_SLE2_STR_X+FT_KEY_SLE2_STR_W, FT_KEY_SLE2_STR_Y, FT_KEY_SLE2_STR_Y+FT_KEY_SLE2_STR_H, FTMenuKeySle2Hander);
#endif		
}

void FTMenuKeyProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuKeyShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenukeyUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void ExitFTMenuKey(void)
{
	ReturnFTMainMenu();
}

void EnterFTMenuKey(void)
{
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_KEY, sizeof(ft_menu_t));
	memcpy(&ft_key, &FT_KEY_INF, sizeof(FT_KEY_INF));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST;	
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

#ifdef CONFIG_WRIST_CHECK_SUPPORT
#define FT_WRIST_STATUS_STR_W			150
#define FT_WRIST_STATUS_STR_H			40
#define FT_WRIST_STATUS_STR_X			((LCD_WIDTH-FT_KEY_MENU_STR_W)/2)
#define FT_WRIST_STATUS_STR_Y			100

#define FT_WRIST_RET_STR_W				120
#define FT_WRIST_RET_STR_H				60
#define FT_WRIST_RET_STR_X				((LCD_WIDTH-FT_KEY_RET_STR_W)/2)
#define FT_WRIST_RET_STR_Y				((LCD_HEIGHT-FT_KEY_RET_STR_H)/2)

static uint8_t ft_wrist_checked = false;

static void FTMenuWristDumpProc(void){}

const ft_menu_t FT_MENU_WRIST = 
{
	FT_WRIST,
	0,
	0,
	{
		{0x0000},
	},	
	{
		FTMenuWristDumpProc,
	},
	{	
		//page proc func
		FTMenuWristDumpProc,
		FTMenuWristDumpProc,
		FTMenuWristDumpProc,
		FTMenuWristDumpProc,
	},
};

static void FTMenuWristSle1Hander(void)
{
	FTMainMenu7Proc();
}

static void FTMenuWristSle2Hander(void)
{
	ExitFTMenuKey();
}

static void FTMenuWristUpdate(void)
{
	static uint8_t check_count = 0;
	uint16_t x,y,w,h;
	uint16_t status_str[2][4] = {
									{0x8131,0x4E0B,0x0000},//脱下
									{0x6234,0x4E0A,0x0000},//戴上
								};
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };	
	if(!ft_wrist_checked)
	{
		LCD_SetFontSize(FONT_SIZE_36);

		check_count++;
		if(check_count > 2)
		{
			check_count = 0;
			ft_wrist_checked = true;
			ft_menu_checked[ft_main_menu_index] = true;
			
			LCD_Set_BL_Mode(LCD_BL_AUTO);
			LCD_SetFontSize(FONT_SIZE_52);
			LCD_SetFontColor(BRRED);
			LCD_SetFontBgColor(GREEN);
			LCD_MeasureUniString(ret_str[0], &w, &h);
			LCD_ShowUniString(FT_WRIST_RET_STR_X+(FT_WRIST_RET_STR_W-w)/2, FT_WRIST_RET_STR_Y+(FT_WRIST_RET_STR_H-h)/2, ret_str[0]);
			LCD_ReSetFontBgColor();
			LCD_ReSetFontColor();
		}
		else
		{
			LCD_MeasureUniString(status_str[touch_flag], &w, &h);
			LCD_ShowUniString(FT_WRIST_STATUS_STR_X+(FT_WRIST_STATUS_STR_W-w)/2, FT_WRIST_STATUS_STR_Y+(FT_WRIST_STATUS_STR_H-h)/2, status_str[touch_flag]);
		}
	}
}

static void FTMenuWristShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[5] = {0x8131,0x8155,0x6D4B,0x8BD5,0x0000};//脱腕测试
	uint16_t sle_str[2][4] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							};
	uint16_t status_str[2][4] = {
									{0x8131,0x4E0B,0x0000},//脱下
									{0x6234,0x4E0A,0x0000},//戴上
								};

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif
	
	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_KEY_TITLE_X+(FT_KEY_TITLE_W-w)/2, FT_KEY_TITLE_Y, title_str);

	LCD_MeasureUniString(status_str[touch_flag], &w, &h);
	LCD_ShowUniString(FT_WRIST_STATUS_STR_X+(FT_WRIST_STATUS_STR_W-w)/2, FT_WRIST_STATUS_STR_Y+(FT_WRIST_STATUS_STR_H-h)/2, status_str[touch_flag]);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_KEY_SLE1_STR_X+(FT_KEY_SLE1_STR_W-w)/2;
	y = FT_KEY_SLE1_STR_Y+(FT_KEY_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_KEY_SLE1_STR_X, FT_KEY_SLE1_STR_Y, FT_KEY_SLE1_STR_W, FT_KEY_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_KEY_SLE2_STR_X+(FT_KEY_SLE2_STR_W-w)/2;
	y = FT_KEY_SLE2_STR_Y+(FT_KEY_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_KEY_SLE2_STR_X, FT_KEY_SLE2_STR_Y, FT_KEY_SLE2_STR_W, FT_KEY_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuWristSle1Hander);
	SetRightKeyUpHandler(FTMenuWristSle2Hander);
		
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_KEY_SLE1_STR_X, FT_KEY_SLE1_STR_X+FT_KEY_SLE1_STR_W, FT_KEY_SLE1_STR_Y, FT_KEY_SLE1_STR_Y+FT_KEY_SLE1_STR_H, FTMenuWristSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_KEY_SLE2_STR_X, FT_KEY_SLE2_STR_X+FT_KEY_SLE2_STR_W, FT_KEY_SLE2_STR_Y, FT_KEY_SLE2_STR_Y+FT_KEY_SLE2_STR_H, FTMenuWristSle2Hander);
#endif		
}

void FTMenuWristProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuWristShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuWristUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void FTWristStatusUpdate(void)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_WRIST))
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

void ExitFTMenuWrist(void)
{
	ReturnFTMainMenu();
}

void EnterFTMenuWrist(void)
{
	ft_wrist_checked = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_WRIST, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST;	
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}

#endif/*CONFIG_WRIST_CHECK_SUPPORT*/
