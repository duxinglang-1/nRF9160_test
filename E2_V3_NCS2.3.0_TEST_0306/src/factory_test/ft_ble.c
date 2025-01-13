/****************************************Copyright (c)************************************************
** File Name:			    ft_ble.c
** Descriptions:			Factory test ble module source file
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
#include "screen.h"
#include "settings.h"
#include "key.h"
#include "uart_ble.h"
#include "ft_main.h"
#include "ft_ble.h"
		
#define FT_BLE_TITLE_W				150
#define FT_BLE_TITLE_H				40
#define FT_BLE_TITLE_X				((LCD_WIDTH-FT_BLE_TITLE_W)/2)
#define FT_BLE_TITLE_Y				20

#define FT_BLE_MENU_STR_W			150
#define FT_BLE_MENU_STR_H			25
#define FT_BLE_MENU_STR_X			((LCD_WIDTH-FT_BLE_MENU_STR_W)/2)
#define FT_BLE_MENU_STR_Y			60
#define FT_BLE_MENU_STR_OFFSET_Y	5

#define FT_BLE_SLE1_STR_W			70
#define FT_BLE_SLE1_STR_H			30
#define FT_BLE_SLE1_STR_X			40
#define FT_BLE_SLE1_STR_Y			170
#define FT_BLE_SLE2_STR_W			70
#define FT_BLE_SLE2_STR_H			30
#define FT_BLE_SLE2_STR_X			130
#define FT_BLE_SLE2_STR_Y			170

#define FT_BLE_RET_STR_W			120
#define FT_BLE_RET_STR_H			60
#define FT_BLE_RET_STR_X			((LCD_WIDTH-FT_BLE_RET_STR_W)/2)
#define FT_BLE_RET_STR_Y			((LCD_HEIGHT-FT_BLE_RET_STR_H)/2)

#define FT_BLE_NOTIFY_W				200
#define FT_BLE_NOTIFY_H				40
#define FT_BLE_NOTIFY_X				((LCD_WIDTH-FT_BLE_NOTIFY_W)/2)
#define FT_BLE_NOTIFY_Y				100
		
#define FT_BLE_TEST_TIMEROUT	1
	
static uint8_t ft_ble_status = 0;//1:ready get the imsi/iccic 2:getting the imsi/iccid 3:got the imsi/iccid
static bool ft_ble_check_ok = false;
static bool ft_ble_checking = false;

static void BleTestTimerOutCallBack(struct k_timer *timer_id);
K_TIMER_DEFINE(ble_test_timer, BleTestTimerOutCallBack, NULL);

static void FTMenuBleDumpProc(void){}

const ft_menu_t FT_MENU_BLE = 
{
	FT_BLE,
	0,
	0,
	{
		{
			{0x0000},
			FTMenuBleDumpProc,
		},
	},
	{	
		//page proc func
		FTMenuBleDumpProc,
		FTMenuBleDumpProc,
		FTMenuBleDumpProc,
		FTMenuBleDumpProc,
	},
};

static void FTMenuBleSle1Hander(void)
{
	FT_MENU_MAIN.item[ft_main_menu_index+1].sel_handler();
}

static void FTMenuBleSle2Hander(void)
{
	ExitFTMenuBle();
}

static void BleTestTimerOutCallBack(struct k_timer *timer_id)
{
	if((screen_id == SCREEN_ID_FACTORY_TEST)&&(ft_menu.id == FT_BLE))
	{
		switch(ft_ble_status)
		{
		case 1:
			ft_ble_checking = true;
			break;
			
		case 2:
			ft_ble_checking = false;
			if((strlen(g_ble_mac_addr) > 0)&&(strlen(g_nrf52810_ver) > 0))
			{
				ft_ble_check_ok = true;
				ft_menu_checked[ft_main_menu_index] = true;
			}
			break;
			
		case 3:
			ft_ble_checking = false;
			ft_ble_check_ok = true;
			ft_menu_checked[ft_main_menu_index] = true;
			break;
		}
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_UPDATE;
	}
}

static void FTMenuBleUpdate(void)
{
	uint16_t w,h;
	uint16_t addr_str[8] = {0x004D,0x0061,0x0063,0x5730,0x5740,0x003A,0x0000}; 	//Mac地址:
	uint16_t ver_str[8] = {0x56FA,0x4EF6,0x7248,0x672C,0x003A,0x0000}; 			//固件版本:
	uint16_t ret_str[2][5] = {
								{0x0046,0x0041,0x0049,0x004C,0x0000},//FAIL
								{0x0050,0x0041,0x0053,0x0053,0x0000},//PASS
							  };

	if(ft_ble_checking)
	{
		uint8_t tmpbuf[256] = {0};
		
		switch(ft_ble_status)
		{
		case 0:
			break;
		case 1:
			ft_ble_status = 2;
			MCU_get_nrf52810_ver();
			MCU_get_ble_mac_address();
			k_timer_start(&ble_test_timer, K_SECONDS(FT_BLE_TEST_TIMEROUT), K_NO_WAIT);
			break;
		case 2:
			ft_ble_checking = false;
			LCD_SetFontSize(FONT_SIZE_28);

			LCD_Fill(FT_BLE_NOTIFY_X, FT_BLE_NOTIFY_Y, FT_BLE_NOTIFY_W, FT_BLE_NOTIFY_H, BLACK);
			LCD_MeasureUniString(addr_str, &w, &h);
			LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+0*FT_BLE_MENU_STR_H, addr_str);
			LCD_MeasureUniString(ver_str, &w, &h);
			LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+2*FT_BLE_MENU_STR_H, ver_str);

			if((strlen(g_ble_mac_addr) > 0)&&(strlen(g_nrf52810_ver) > 0))
			{
				ft_ble_check_ok = true;
				ft_menu_checked[ft_main_menu_index] = true;

				LCD_SetFontSize(FONT_SIZE_20);
				mmi_asc_to_ucs2(tmpbuf, g_ble_mac_addr);
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+1*FT_BLE_MENU_STR_H, tmpbuf);
				mmi_asc_to_ucs2(tmpbuf, &g_nrf52810_ver[15]);
				LCD_MeasureUniString(tmpbuf, &w, &h);
				LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+3*FT_BLE_MENU_STR_H, tmpbuf);
			}
			else
			{
				ft_ble_check_ok = false;
			}

			k_timer_start(&ble_test_timer, K_SECONDS(FT_BLE_TEST_TIMEROUT), K_NO_WAIT);
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
		LCD_MeasureUniString(ret_str[ft_ble_check_ok], &w, &h);
		LCD_ShowUniString(FT_BLE_RET_STR_X+(FT_BLE_RET_STR_W-w)/2, FT_BLE_RET_STR_Y+(FT_BLE_RET_STR_H-h)/2, ret_str[ft_ble_check_ok]);
		LCD_ReSetFontBgColor();
		LCD_ReSetFontColor();

		ClearAllKeyHandler();
		SetLeftKeyUpHandler(FTMenuBleSle1Hander);
		SetRightKeyUpHandler(FTMenuBleSle2Hander);
			
	#ifdef CONFIG_TOUCH_SUPPORT
		clear_all_touch_event_handle();
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_BLE_SLE1_STR_X, FT_BLE_SLE1_STR_X+FT_BLE_SLE1_STR_W, FT_BLE_SLE1_STR_Y, FT_BLE_SLE1_STR_Y+FT_BLE_SLE1_STR_H, FTMenuBleSle1Hander);
		register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_BLE_SLE2_STR_X, FT_BLE_SLE2_STR_X+FT_BLE_SLE2_STR_W, FT_BLE_SLE2_STR_Y, FT_BLE_SLE2_STR_Y+FT_BLE_SLE2_STR_H, FTMenuBleSle2Hander);
	#endif	
	}
}

static void FTMenuBleShow(void)
{
	uint8_t i;
	uint16_t x,y,w,h;
	uint16_t title_str[8] = {0x0042,0x004C,0x0045,0x6D4B,0x8BD5,0x0000};//BLE测试
	uint16_t addr_str[8] = {0x004D,0x0061,0x0063,0x5730,0x5740,0x003A,0x0000}; 	//Mac地址:
	uint16_t ver_str[8] = {0x56FA,0x4EF6,0x7248,0x672C,0x003A,0x0000}; 			//固件版本:
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
	LCD_ShowUniString(FT_BLE_TITLE_X+(FT_BLE_TITLE_W-w)/2, FT_BLE_TITLE_Y, title_str);

	if((strlen(g_ble_mac_addr) > 0)&&(strlen(g_nrf52810_ver) > 0))
	{
		uint8_t tmpbuf[256] = {0};

		LCD_SetFontSize(FONT_SIZE_28);
		LCD_MeasureUniString(addr_str, &w, &h);
		LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+0*FT_BLE_MENU_STR_H, addr_str);
		LCD_MeasureUniString(ver_str, &w, &h);
		LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+2*FT_BLE_MENU_STR_H, ver_str);

		LCD_SetFontSize(FONT_SIZE_20);
		mmi_asc_to_ucs2(tmpbuf, g_ble_mac_addr);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+1*FT_BLE_MENU_STR_H, tmpbuf);
		mmi_asc_to_ucs2(tmpbuf, &g_nrf52810_ver[15]);
		LCD_MeasureUniString(tmpbuf, &w, &h);
		LCD_ShowUniString(FT_BLE_MENU_STR_X, FT_BLE_MENU_STR_Y+(FT_BLE_MENU_STR_H-h)/2+3*FT_BLE_MENU_STR_H, tmpbuf);
	}
	else
	{
		LCD_MeasureUniString(notify_str, &w, &h);
		LCD_ShowUniString(FT_BLE_NOTIFY_X+(FT_BLE_NOTIFY_W-w)/2, FT_BLE_NOTIFY_Y+(FT_BLE_NOTIFY_H-h)/2, notify_str);
	}

	LCD_SetFontSize(FONT_SIZE_28);
	LCD_MeasureUniString(sle_str[0], &w, &h);
	x = FT_BLE_SLE1_STR_X+(FT_BLE_SLE1_STR_W-w)/2;
	y = FT_BLE_SLE1_STR_Y+(FT_BLE_SLE1_STR_H-h)/2;
	LCD_DrawRectangle(FT_BLE_SLE1_STR_X, FT_BLE_SLE1_STR_Y, FT_BLE_SLE1_STR_W, FT_BLE_SLE1_STR_H);
	LCD_ShowUniString(x, y, sle_str[0]);
	LCD_MeasureUniString(sle_str[1], &w, &h);
	x = FT_BLE_SLE2_STR_X+(FT_BLE_SLE2_STR_W-w)/2;
	y = FT_BLE_SLE2_STR_Y+(FT_BLE_SLE2_STR_H-h)/2;
	LCD_DrawRectangle(FT_BLE_SLE2_STR_X, FT_BLE_SLE2_STR_Y, FT_BLE_SLE2_STR_W, FT_BLE_SLE2_STR_H);
	LCD_ShowUniString(x, y, sle_str[1]);

	ClearAllKeyHandler();
	SetLeftKeyUpHandler(FTMenuBleSle1Hander);
	SetRightKeyUpHandler(FTMenuBleSle2Hander);
	
#ifdef CONFIG_TOUCH_SUPPORT
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_BLE_SLE1_STR_X, FT_BLE_SLE1_STR_X+FT_BLE_SLE1_STR_W, FT_BLE_SLE1_STR_Y, FT_BLE_SLE1_STR_Y+FT_BLE_SLE1_STR_H, FTMenuBleSle1Hander);
	register_touch_event_handle(TP_EVENT_SINGLE_CLICK, FT_BLE_SLE2_STR_X, FT_BLE_SLE2_STR_X+FT_BLE_SLE2_STR_W, FT_BLE_SLE2_STR_Y, FT_BLE_SLE2_STR_Y+FT_BLE_SLE2_STR_H, FTMenuBleSle2Hander);
#endif		
}

void FTMenuBleProcess(void)
{
	if(scr_msg[SCREEN_ID_FACTORY_TEST].act != SCREEN_ACTION_NO)
	{
		if(scr_msg[SCREEN_ID_FACTORY_TEST].status != SCREEN_STATUS_CREATED)
			scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;

		switch(scr_msg[SCREEN_ID_FACTORY_TEST].act)
		{
		case SCREEN_ACTION_ENTER:
			scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATED;
			FTMenuBleShow();
			break;
			
		case SCREEN_ACTION_UPDATE:
			FTMenuBleUpdate();
			break;
		}
	
		scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_NO;
	}
}

void ExitFTMenuBle(void)
{
	ft_ble_status = 0;
	ft_ble_checking = false;
	ft_ble_check_ok = false;
	k_timer_stop(&ble_test_timer);
	ReturnFTMainMenu();
}

void EnterFTMenuBle(void)
{
	ft_ble_status = 0;
	ft_ble_check_ok = false;
	ft_ble_checking = true;
	ft_menu_checked[ft_main_menu_index] = false;
	memcpy(&ft_menu, &FT_MENU_BLE, sizeof(ft_menu_t));
	
	history_screen_id = screen_id;
	scr_msg[history_screen_id].act = SCREEN_ACTION_NO;
	scr_msg[history_screen_id].status = SCREEN_STATUS_NO;

	screen_id = SCREEN_ID_FACTORY_TEST; 
	scr_msg[SCREEN_ID_FACTORY_TEST].act = SCREEN_ACTION_ENTER;
	scr_msg[SCREEN_ID_FACTORY_TEST].status = SCREEN_STATUS_CREATING;

	if((strlen(g_ble_mac_addr) == 0)||(strlen(g_nrf52810_ver) == 0))
	{
		ft_ble_status = 1;
	}
	else
	{
		ft_ble_status = 3;
	}
	
	k_timer_start(&ble_test_timer, K_SECONDS(FT_BLE_TEST_TIMEROUT), K_NO_WAIT);
}

