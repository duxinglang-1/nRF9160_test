/****************************************Copyright (c)************************************************
** File Name:			    ft_pmu.c
** Descriptions:			Factory test pmu module source file
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
#include "ft_pmu.h"
			
#define FT_PMU_TITLE_W				100
#define FT_PMU_TITLE_H				40
#define FT_PMU_TITLE_X				((LCD_WIDTH-FT_PMU_TITLE_W)/2)
#define FT_PMU_TITLE_Y				20

#define FT_PMU_MENU_STR_W			150
#define FT_PMU_MENU_STR_H			30
#define FT_PMU_MENU_STR_X			((LCD_WIDTH-FT_PMU_MENU_STR_W)/2)
#define FT_PMU_MENU_STR_Y			80
#define FT_PMU_MENU_STR_OFFSET_Y	5

#define FT_PMU_SLE1_STR_W			70
#define FT_PMU_SLE1_STR_H			30
#define FT_PMU_SLE1_STR_X			40
#define FT_PMU_SLE1_STR_Y			170
#define FT_PMU_SLE2_STR_W			70
#define FT_PMU_SLE2_STR_H			30
#define FT_PMU_SLE2_STR_X			130
#define FT_PMU_SLE2_STR_Y			170

#define FT_PMU_RET_STR_W			120
#define FT_PMU_RET_STR_H			60
#define FT_PMU_RET_STR_X			((LCD_WIDTH-FT_PMU_RET_STR_W)/2)
#define FT_PMU_RET_STR_Y			((LCD_HEIGHT-FT_PMU_RET_STR_H)/2)

#define FT_PMU_STR_W				LCD_WIDTH
#define FT_PMU_STR_H				40
#define FT_PMU_STR_X				((LCD_WIDTH-FT_PMU_STR_W)/2)
#define FT_PMU_STR_Y				100

static uint8_t ft_pmu_change_flag = 0;
static uint8_t ft_pmu_checked = false;

static void FTMenuPMUDumpProc(void){}

const ft_menu_t FT_MENU_PMU = 
{
	FT_PMU,
	0,
	0,
	{
		{
			{0x0000},
			FTMenuPMUDumpProc,
		},
	},
	{	
		//page proc func
		FTMenuPMUDumpProc,
		FTMenuPMUDumpProc,
		FTMenuPMUDumpProc,
		FTMenuPMUDumpProc,
	},
};

static void FTMenuPMUSle1Hander(void)
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

static void FTMenuPMUSle2Hander(void)
{
	ExitFTMenuPMU();
}

static void FTMenuPMUUpdate(void)
{
	static uint8_t check_count = 0;
	uint16_t x,y,w,h;
	uint8_t tmpbuf[16] = {0};
	uint16_t unibuf[16] = {0};
	uint16_t soc_str[10] = {0x5F53,0x524D,0x7535,0x91CF,0x003A,0x0000};//当前电量:
	uint16_t charg_link_str[8] = {0x5145,0x7535,0x5668,0x5DF2,0x8FDE,0x63A5,0x0000};//充电器已连接
	uint16_t charg_unlink_str[8] = {0x5145,0x7535,0x5668,0x672A,0x8FDE,0x63A5,0x0000};//充电器未连接
	uint16_t ret_str[2][5] = {
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
							  };

	if(!ft_pmu_checked)
	{
		LCD_SetFontSize(FONT_SIZE_28);

		//soc
		if(ft_pmu_change_flag == 1)
		{
			LCD_Fill(FT_PMU_MENU_STR_X, FT_PMU_MENU_STR_Y+0*(FT_PMU_MENU_STR_H+FT_PMU_MENU_STR_OFFSET_Y), FT_PMU_MENU_STR_W, FT_PMU_MENU_STR_H, BLACK);
			sprintf(tmpbuf, "%d", g_bat_soc);
			mmi_asc_to_ucs2((uint8_t*)unibuf, tmpbuf);
			mmi_ucs2cat((uint8_t*)soc_str, (uint8_t*)unibuf);
			LCD_MeasureUniString(soc_str, &w, &h);
			LCD_ShowUniString(FT_PMU_MENU_STR_X+(FT_PMU_MENU_STR_W-w)/2, FT_PMU_MENU_STR_Y+(FT_PMU_MENU_STR_H-h)/2+0*(FT_PMU_MENU_STR_H+FT_PMU_MENU_STR_OFFSET_Y), soc_str);
		}

		//charge status
		if(ft_pmu_change_flag == 2)
		{
			check_count++;
			if(check_count > 1)
			{
				check_count = 0;
				ft_pmu_checked = true;
				ft_menu_checked[ft_main_menu_index] = true;
				switch(g_ft_status)
				{
				case FT_STATUS_SMT:
					ft_smt_results.pmu_ret = 1;
					SaveFactoryTestResults(FT_STATUS_SMT, &ft_smt_results);
					break;
					
				case FT_STATUS_ASSEM:
					ft_assem_results.pmu_ret = 1;
					SaveFactoryTestResults(FT_STATUS_ASSEM, &ft_assem_results);
					break;
				}

				LCD_SetFontSize(FONT_SIZE_52);
				LCD_SetFontColor(BRRED);
				LCD_SetFontBgColor(GREEN);
				LCD_MeasureUniString(ret_str[0], &w, &h);
				LCD_ShowUniString(FT_PMU_RET_STR_X+(FT_PMU_RET_STR_W-w)/2, FT_PMU_RET_STR_Y+(FT_PMU_RET_STR_H-h)/2, ret_str[0]);
				LCD_ReSetFontBgColor();
				LCD_ReSetFontColor();
			}
			else
			{
				memset(unibuf, 0x0000, sizeof(unibuf));
				if(charger_is_connected)
					mmi_ucs2cat((uint8_t*)unibuf, (uint8_t*)charg_link_str);
				else
					mmi_ucs2cat((uint8_t*)unibuf, (uint8_t*)charg_unlink_str);
				LCD_MeasureUniString(unibuf, &w, &h);
				LCD_ShowUniString(FT_PMU_MENU_STR_X+(FT_PMU_MENU_STR_W-w)/2, FT_PMU_MENU_STR_Y+(FT_PMU_MENU_STR_H-h)/2+1*(FT_PMU_MENU_STR_H+FT_PMU_MENU_STR_OFFSET_Y), unibuf);
				}
		}

		ft_pmu_change_flag = 0;
	}
}

static void FTMenuPMUShow(void)
{
	uint8_t i;
	uint8_t tmpbuf[16] = {0};
	uint16_t unibuf[16] = {0};
	uint16_t x,y,w,h;
	uint16_t title_str[6] = {0x5145,0x7535,0x6D4B,0x8BD5,0x0000};//充电测试
	uint16_t soc_str[10] = {0x5F53,0x524D,0x7535,0x91CF,0x003A,0x0000};//当前电量:
	uint16_t charg_link_str[8] = {0x5145,0x7535,0x5668,0x5DF2,0x8FDE,0x63A5,0x0000};//充电器已连接
	uint16_t charg_unlink_str[8] = {0x5145,0x7535,0x5668,0x672A,0x8FDE,0x63A5,0x0000};//充电器未连接
	uint16_t sle_str[2][4] = {
								{0x4E0B,0x4E00,0x9879,0x0000},//下一项
								{0x9000,0x51FA,0x0000},//退出
							 };

#ifdef CONFIG_TOUCH_SUPPORT
	clear_all_touch_event_handle();
#endif
	
	LCD_Clear(BLACK);
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(title_str, &w, &h);
	LCD_ShowUniString(FT_PMU_TITLE_X+(FT_PMU_TITLE_W-w)/2, FT_PMU_TITLE_Y, title_str);
	
	//soc
	sprintf(tmpbuf, "%d", g_bat_soc);
	mmi_asc_to_ucs2((uint8_t*)unibuf, tmpbuf);
	mmi_ucs2cat((uint8_t*)soc_str, (uint8_t*)unibuf);
	LCD_MeasureUniString(soc_str, &w, &h);
	LCD_ShowUniString(FT_PMU_MENU_STR_X+(FT_PMU_MENU_STR_W-w)/2, FT_PMU_MENU_STR_Y+(FT_PMU_MENU_STR_H-h)/2+0*(FT_PMU_MENU_STR_H+FT_PMU_MENU_STR_OFFSET_Y), soc_str);

	//charge status
	memset(unibuf, 0x0000, sizeof(unibuf));
	if(charger_is_connected)
		mmi_ucs2cat((uint8_t*)unibuf, (uint8_t*)charg_link_str);
	else
		mmi_ucs2cat((uint8_t*)unibuf, (uint8_t*)charg_unlink_str);
	LCD_MeasureUniString(unibuf, &w, &h);
	LCD_ShowUniString(FT_PMU_MENU_STR_X+(FT_PMU_MENU_STR_W-w)/2, FT_PMU_MENU_STR_Y+(FT_PMU_MENU_STR_H-h)/2+1*(FT_PMU_MENU_STR_H+FT_PMU_MENU_STR_OFFSET_Y), unibuf);

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_PMU_SLE1_STR_X+(FT_PMU_SLE1_STR_W-w)/2;
	y = FT_PMU_SLE1_STR_Y+(FT_PMU_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_PMU_SLE1_STR_X, FT_PMU_SLE1_STR_Y, FT_PMU_SLE1_STR_W, FT_PMU_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_PMU_SLE2_STR_X+(FT_PMU_SLE2_STR_W-w)/2;
	y = FT_PMU_SLE2_STR_Y+(FT_PMU_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_PMU_SLE2_STR_X, FT_PMU_SLE2_STR_Y, FT_PMU_SLE2_STR_W, FT_PMU_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuPMUSle1Hander);
	SetRightKeyUpHandler(FTMenuPMUSle2Hander);
		
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_PMU_SLE1_STR_X, FT_PMU_SLE1_STR_X+FT_PMU_SLE1_STR_W, FT_PMU_SLE1_STR_Y, FT_PMU_SLE1_STR_Y+FT_PMU_SLE1_STR_H, FTMenuPMUSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_PMU_SLE2_STR_X, FT_PMU_SLE2_STR_X+FT_PMU_SLE2_STR_W, FT_PMU_SLE2_STR_Y, FT_PMU_SLE2_STR_Y+FT_PMU_SLE2_STR_H, FTMenuPMUSle2Hander);
#endif	
}

void FTMenuPMUProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuPMUShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuPMUUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void FTPMUStatusUpdate(uint8_t flag)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_PMU))
	{
		ft_pmu_change_flag = flag;//1:soc change 2:charge insert or outsert
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}
}

void ExitFTMenuPMU(void)
{
	ReturnFTMainMenu();
}

void EnterFTMenuPMU(void)
{
	ft_pmu_checked = false;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_PMU, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;
}
