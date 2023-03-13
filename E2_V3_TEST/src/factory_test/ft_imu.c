/****************************************Copyright (c)************************************************
** File Name:			    ft_imu.c
** Descriptions:			Factory test imu module source file
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
#include "external_flash.h"
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "ft_main.h"
#include "ft_imu.h"
		
#define FT_IMU_TITLE_W				100
#define FT_IMU_TITLE_H				40
#define FT_IMU_TITLE_X				((LCD_WIDTH-FT_IMU_TITLE_W)/2)
#define FT_IMU_TITLE_Y				20
#define FT_IMU_MENU_STR_W			150
#define FT_IMU_MENU_STR_H			30
#define FT_IMU_MENU_STR_X			((LCD_WIDTH-FT_IMU_MENU_STR_W)/2)
#define FT_IMU_MENU_STR_Y			80
#define FT_IMU_MENU_STR_OFFSET_Y	5
#define FT_IMU_SLE1_STR_W			70
#define FT_IMU_SLE1_STR_H			30
#define FT_IMU_SLE1_STR_X			40
#define FT_IMU_SLE1_STR_Y			170
#define FT_IMU_SLE2_STR_W			70
#define FT_IMU_SLE2_STR_H			30
#define FT_IMU_SLE2_STR_X			130
#define FT_IMU_SLE2_STR_Y			170
#define FT_IMU_RET_STR_W			120
#define FT_IMU_RET_STR_H			60
#define FT_IMU_RET_STR_X			((LCD_WIDTH-FT_IMU_RET_STR_W)/2)
#define FT_IMU_RET_STR_Y			((LCD_HEIGHT-FT_IMU_RET_STR_H)/2)
#define FT_IMU_STR_W				LCD_WIDTH
#define FT_IMU_STR_H				40
#define FT_IMU_STR_X				((LCD_WIDTH-FT_IMU_STR_W)/2)
#define FT_IMU_STR_Y				100

static bool ft_imu_checked = false;
static void FTMenuIMUDumpProc(void){}

const ft_menu_t FT_MENU_IMU = 
{
	FT_IMU,
	0,
	0,
	{
		{0x0000},
	},
	{
		FTMenuIMUDumpProc,
	},
	{	
		//page proc func
		FTMenuIMUDumpProc,
		FTMenuIMUDumpProc,
		FTMenuIMUDumpProc,
		FTMenuIMUDumpProc,
	},
};

static void FTMenuIMUSle1Hander(void)
{
	FTMainMenu8Proc();
}

static void FTMenuIMUSle2Hander(void)
{
	ExitFTMenuIMU();
}

static void FTMenuIMUUpdate(void)
{
	uint16_t x,y,w,h;
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };

	if(!ft_imu_checked)
	{
		ft_imu_checked = true;
		
		LCD_Set_BL_Mode(LCD_BL_AUTO);
		LCD_Fill(FT_IMU_STR_X, FT_IMU_STR_Y, FT_IMU_STR_W, FT_IMU_STR_H, BLACK);
		
		LCD_SetFontSize(FONT_SIZE_52);
		LCD_SetFontColor(BRRED);
		LCD_SetFontBgColor(GREEN);
		LCD_MeasureUniString(ret_str[0], &w, &h);
		LCD_ShowUniString(FT_IMU_RET_STR_X+(FT_IMU_RET_STR_W-w)/2, FT_IMU_RET_STR_Y+(FT_IMU_RET_STR_H-h)/2, ret_str[0]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();
	}
}

static void FTMenuIMUShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[6] = {0x0049,0x004D,0x0055,0x6D4B,0x8BD5,0x0000};//IMU测试
	uint16_t sle_str[2][4] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							};
	uint16_t notify_str[8] = {0x8BF7,0x5927,0x529B,0x6447,0x6643,0x8BBE,0x5907,0x0000};//请大力摇晃设备

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif
	
	LCD_Clear(BLACK);
	LCD_SetFontSize(FONT_SIZE_36);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_IMU_TITLE_X+(FT_IMU_TITLE_W-w)/2, FT_IMU_TITLE_Y, title_str);

	LCD_MeasureUniString(notify_str, &w, &h);
	LCD_ShowUniString(FT_IMU_STR_X+(FT_IMU_STR_W-w)/2, FT_IMU_STR_Y+(FT_IMU_STR_H-h)/2, notify_str);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_IMU_SLE1_STR_X+(FT_IMU_SLE1_STR_W-w)/2;
	y = FT_IMU_SLE1_STR_Y+(FT_IMU_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_IMU_SLE1_STR_X, FT_IMU_SLE1_STR_Y, FT_IMU_SLE1_STR_W, FT_IMU_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_IMU_SLE2_STR_X+(FT_IMU_SLE2_STR_W-w)/2;
	y = FT_IMU_SLE2_STR_Y+(FT_IMU_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_IMU_SLE2_STR_X, FT_IMU_SLE2_STR_Y, FT_IMU_SLE2_STR_W, FT_IMU_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuIMUSle1Hander);
	SetRightKeyUpHandler(FTMenuIMUSle2Hander);
		
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_IMU_SLE1_STR_X, FT_IMU_SLE1_STR_X+FT_IMU_SLE1_STR_W, FT_IMU_SLE1_STR_Y, FT_IMU_SLE1_STR_Y+FT_IMU_SLE1_STR_H, FTMenuIMUSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_IMU_SLE2_STR_X, FT_IMU_SLE2_STR_X+FT_IMU_SLE2_STR_W, FT_IMU_SLE2_STR_Y, FT_IMU_SLE2_STR_Y+FT_IMU_SLE2_STR_H, FTMenuIMUSle2Hander);
#endif	
}

void FTMenuIMUProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuIMUShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuIMUUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void FTIMUStatusUpdate(void)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_IMU))
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
}

void ExitFTMenuIMU(void)
{
	ReturnFTMainMenu();
}

void EnterFTMenuIMU(void)
{
	ft_imu_checked = false;
	memcpy(&ft_menu, &FT_MENU_IMU, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}
