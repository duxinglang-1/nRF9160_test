#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include "max20353.h"
#include "max20353_reg.h"
#include "Lcd.h"
#include "datetime.h"
#include "settings.h"
#include "screen.h"
#include "external_flash.h"
#include "logger.h"

//#define SHOW_LOG_IN_SCREEN
//#define PMU_DEBUG

static bool pmu_check_ok = false;
static u8_t PMICStatus[4], PMICInts[3];
static struct device *i2c_pmu;
static struct device *gpio_pmu;
static struct gpio_callback gpio_cb1,gpio_cb2;

static void test_soc_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(soc_timer, test_soc_timerout, NULL);
static void pmu_battery_low_shutdown_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(soc_pwroff, pmu_battery_low_shutdown_timerout, NULL);
static void sys_pwr_off_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(sys_pwroff, sys_pwr_off_timerout, NULL);
static void vibrate_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(vib_timer, vibrate_timerout, NULL);

bool vibrate_start_flag = false;
bool vibrate_stop_flag = false;
bool pmu_trige_flag = false;
bool pmu_alert_flag = false;
bool pmu_bat_flag = false;
bool pmu_check_temp_flag = false;
bool pmu_redraw_bat_flag = true;
bool lowbat_pwr_off_flag = false;
bool sys_pwr_off_flag = false;
bool read_soc_status = false;
bool charger_is_connected = false;
bool pmu_bat_has_notify = false;
bool sys_shutdown_is_running = false;

u8_t g_bat_soc = 0;

BAT_CHARGER_STATUS g_chg_status = BAT_CHARGING_NO;
BAT_LEVEL_STATUS g_bat_level = BAT_LEVEL_NORMAL;

maxdev_ctx_t pmu_dev_ctx;

extern bool key_pwroff_flag;

#ifdef SHOW_LOG_IN_SCREEN
static u8_t tmpbuf[256] = {0};

static void show_infor1(u8_t *strbuf)
{
	//LCD_Clear(BLACK);
	LCD_Fill(30,50,180,70,BLACK);
	LCD_ShowStringInRect(30,50,180,70,strbuf);
}

static void show_infor2(u8_t *strbuf)
{
	//LCD_Clear(BLACK);
	LCD_Fill(30,130,180,70,BLACK);
	LCD_ShowStringInRect(30,130,180,70,strbuf);
}

#endif

static bool init_i2c(void)
{
	i2c_pmu = device_get_binding(PMU_DEV);
	if(!i2c_pmu)
	{
	#ifdef PMU_DEBUG
		LOGD("ERROR SETTING UP I2C");
	#endif
		return false;
	} 
	else
	{
		i2c_configure(i2c_pmu, I2C_SPEED_SET(I2C_SPEED_FAST));
		return true;
	}
}

static s32_t platform_write(struct device *handle, u8_t reg, u8_t *bufp, u16_t len)
{
	u32_t i=0;
	u8_t data[len+1];
	u32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(handle, data, len+1, MAX20353_I2C_ADDR);

	return rslt;
}

static s32_t platform_read(struct device *handle, u8_t reg, u8_t *bufp, u16_t len)
{
	u32_t rslt = 0;

	rslt = i2c_write(handle, &reg, 1, MAX20353_I2C_ADDR);
	if(rslt == 0)
	{
		rslt = i2c_read(handle, bufp, len, MAX20353_I2C_ADDR);
	}

	return rslt;
}

void Set_PPG_Power_On(void)
{
	MAX20353_LDO1Config();
}

void Set_PPG_Power_Off(void)
{
	MAX20353_LDO1Disable();
}

void Set_Screen_Backlight_Level(BACKLIGHT_LEVEL level)
{
	int ret = 0;

	ret = MAX20353_LED0(2, (31*level)/BACKLIGHT_LEVEL_MAX, true);
	ret = MAX20353_LED1(2, (31*level)/BACKLIGHT_LEVEL_MAX, true);
}

void Set_Screen_Backlight_On(void)
{
	int ret = 0;

	ret = MAX20353_LED0(2, (31*global_settings.backlight_level)/BACKLIGHT_LEVEL_MAX, true);
	ret = MAX20353_LED1(2, (31*global_settings.backlight_level)/BACKLIGHT_LEVEL_MAX, true);
	MAX20353_BuckBoostConfig();
}

void Set_Screen_Backlight_Off(void)
{
	int ret = 0;

	MAX20353_BuckBoostDisable();
	ret = MAX20353_LED0(2, 0, false);
	ret = MAX20353_LED1(2, 0, false);
}

void sys_pwr_off_timerout(struct k_timer *timer_id)
{
	sys_pwr_off_flag = true;
}

void vibrate_timerout(struct k_timer *timer_id)
{
	vibrate_stop_flag = true;
}

void system_power_off(u8_t flag)
{
	if(!sys_shutdown_is_running)
	{
		sys_shutdown_is_running = true;
		
		SaveSystemDateTime();
		if(nb_is_connected())
		{
			SendPowerOffData(flag);
		}

		VibrateStart();
		k_timer_start(&vib_timer, K_MSEC(100), NULL);
		k_timer_start(&sys_pwroff, K_MSEC(5*1000), NULL);
	}
}

void SystemShutDown(void)
{	
#ifdef PMU_DEBUG
	LOGD("begin");
#endif
	MAX20353_PowerOffConfig();
}

void pmu_battery_low_shutdown_timerout(struct k_timer *timer_id)
{
	lowbat_pwr_off_flag = true;
}

void pmu_battery_stop_shutdown(void)
{
	if(k_timer_remaining_get(&soc_pwroff) > 0)
		k_timer_stop(&soc_pwroff);
}

void pmu_battery_low_shutdown(void)
{
	k_timer_start(&soc_pwroff, K_MSEC(10*1000), NULL);
}

void pmu_reg_proc(void)
{
	u8_t i;
	u8_t tmpbuf[128] = {0};
	notify_infor infor = {0};
	u8_t int0,int1,int2;
	u8_t status0,status1,status2,status3;

	MAX20353_ReadReg(REG_INT0, &int0);
#ifdef PMU_DEBUG	
	MAX20353_ReadReg(REG_INT1, &int1);
	MAX20353_ReadReg(REG_INT2, &int2);
	LOGD("INT:%02X, %02X, %02X", int0,int1,int2);
#endif	

#ifdef PMU_DEBUG
	MAX20353_ReadReg(REG_STATUS0, &status0);
	MAX20353_ReadReg(REG_STATUS1, &status1);
	MAX20353_ReadReg(REG_STATUS2, &status2);
	MAX20353_ReadReg(REG_STATUS3, &status3);
	LOGD("status:%02X, %02X, %02X, %02X", status0,status1,status2,status3);
#endif
	if((int0&0x40) == 0x40) //Charger status change INT  
	{
		MAX20353_ReadReg(REG_STATUS0, &status0);		
		switch((status0&0x07))
		{
		case 0x00://Charger off
		case 0x01://Charging suspended due to temperature (see battery charger state diagram)
		case 0x07://Charger fault condition (see battery charger state diagram)	
			g_chg_status = BAT_CHARGING_NO;
			break;
			
		case 0x02://Pre-charge in progress
		case 0x03://Fast-charge constant current mode in progress
		case 0x04://Fast-charge constant voltage mode in progress
		case 0x05://Maintain charge in progress
			g_chg_status = BAT_CHARGING_PROGRESS;
			break;
			
		case 0x06://Maintain charger timer done
			g_chg_status = BAT_CHARGING_FINISHED;
		#ifdef PMU_DEBUG
			LOGD("charging finished!");
		#endif
			
		#ifdef BATTERY_SOC_GAUGE	
			g_bat_soc = MAX20353_CalculateSOC();
		#ifdef PMU_DEBUG
			LOGD("g_bat_soc:%d", g_bat_soc);
		#endif
			if(g_bat_soc >= 95)
				g_bat_soc = 100;
		#endif

			if(screen_id == SCREEN_ID_NOTIFY)
			{
				sprintf(tmpbuf, "%d%%", g_bat_soc);
				mmi_asc_to_ucs2(notify_msg.text, tmpbuf);
				notify_msg.img[0] = IMG_BAT_CHRING_ANI_5_ADDR;
				notify_msg.img_count = 1;
				scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
				scr_msg[screen_id].para = SCREEN_EVENT_UPDATE_POP_STR|SCREEN_EVENT_UPDATE_POP_IMG;
			}
			
			lcd_sleep_out = true;
			break;
		}

		pmu_redraw_bat_flag = true;
	}
	
	if((int0&0x08) == 0x08) //USB OK Int
	{
		MAX20353_ReadReg(REG_STATUS1, &status1);
		
		if((status1&0x08) == 0x08) //USB OK   
		{
			u32_t bat_img[5] = {IMG_BAT_CHRING_ANI_1_ADDR,IMG_BAT_CHRING_ANI_2_ADDR,IMG_BAT_CHRING_ANI_3_ADDR,IMG_BAT_CHRING_ANI_4_ADDR,IMG_BAT_CHRING_ANI_5_ADDR};

		#ifdef PMU_DEBUG
			LOGD("charger push in!");
		#endif	
			pmu_battery_stop_shutdown();
			
			InitCharger();

			charger_is_connected = true;
			
			g_chg_status = BAT_CHARGING_PROGRESS;
			g_bat_level = BAT_LEVEL_NORMAL;

			infor.x = 0;
			infor.y = 0;
			infor.w = LCD_WIDTH;
			infor.h = LCD_HEIGHT;
			infor.align = NOTIFY_ALIGN_CENTER;
			infor.type = NOTIFY_TYPE_NOTIFY;
			sprintf(tmpbuf, "%d%%", g_bat_soc);
			mmi_asc_to_ucs2(infor.text, tmpbuf);
			for(i=0;i<ARRAY_SIZE(bat_img);i++)
				infor.img[i] = bat_img[i];
			infor.img_count = ARRAY_SIZE(bat_img);
			DisplayPopUp(infor);
			
			lcd_sleep_out = true;
		}
		else
		{		
		#ifdef PMU_DEBUG
			LOGD("charger push out!");
		#endif
			charger_is_connected = false;
			
			g_chg_status = BAT_CHARGING_NO;

		#ifdef BATTERY_SOC_GAUGE	
			g_bat_soc = MAX20353_CalculateSOC();
			if(g_bat_soc>100)
				g_bat_soc = 100;
			
			if(g_bat_soc < 5)
			{
				g_bat_level = BAT_LEVEL_VERY_LOW;
				pmu_battery_low_shutdown();
			}
			else if(g_bat_soc < 20)
			{
				g_bat_level = BAT_LEVEL_LOW;
			}
			else if(g_bat_soc < 80)
			{
				g_bat_level = BAT_LEVEL_NORMAL;
			}
			else
			{
				g_bat_level = BAT_LEVEL_GOOD;
			}
		#endif

			ExitNotifyScreen();
			lcd_sleep_out = true;
		}

		pmu_redraw_bat_flag = true;
	}	
}

void pmu_interrupt_proc(void)
{
	u8_t val;
	
	pmu_reg_proc();
}

void PmuInterruptHandle(void)
{
	pmu_trige_flag = true;
}

//An alert can indicate many different conditions. The
//STATUS register identifies which alert condition was met.
//Clear the corresponding bit after servicing the alert
bool pmu_alert_proc(void)
{
	u8_t i;
	u8_t tmpbuf[128] = {0};
	notify_infor infor = {0};
	int ret;
	u8_t MSB,LSB;

#ifdef BATTERY_SOC_GAUGE
	ret = MAX20353_SOCReadReg(0x1A, &MSB, &LSB);
	if(ret == MAX20353_ERROR)
		return false;
	
#ifdef PMU_DEBUG
	LOGD("status:%02X", MSB);
#endif
	if(MSB&0x40)
	{
		//EnVr (enable voltage reset alert)
		MSB = MSB&0xBF;
	#ifdef PMU_DEBUG
		LOGD("voltage reset alert!");
	#endif
	}
	if(MSB&0x20)
	{
		//SC (1% SOC change) is set when SOC changes by at least 1% if CONFIG.ALSC is set
		MSB = MSB&0xDF;

		g_bat_soc = MAX20353_CalculateSOC();
	#ifdef PMU_DEBUG
		LOGD("SOC:%d", g_bat_soc);
	#endif	
		if(g_bat_soc>100)
			g_bat_soc = 100;
		
		if(g_bat_soc < 5)
		{
			g_bat_level = BAT_LEVEL_VERY_LOW;
			if(!charger_is_connected)
			{
				infor.x = 0;
				infor.y = 0;
				infor.w = LCD_WIDTH;
				infor.h = LCD_HEIGHT;
				infor.align = NOTIFY_ALIGN_CENTER;
				infor.type = NOTIFY_TYPE_POPUP;
				sprintf(tmpbuf, "%d%%", g_bat_soc);
				mmi_asc_to_ucs2(infor.text, tmpbuf);
				infor.img[0] = IMG_BAT_LOW_ICON_ADDR;
				infor.img_count = 1;
				DisplayPopUp(infor);
				
				pmu_battery_low_shutdown();
			}
		}
		else if(g_bat_soc < 10)
		{
			g_bat_level = BAT_LEVEL_LOW;
			if(!charger_is_connected)
			{
				infor.x = 0;
				infor.y = 0;
				infor.w = LCD_WIDTH;
				infor.h = LCD_HEIGHT;
				infor.align = NOTIFY_ALIGN_CENTER;
				infor.type = NOTIFY_TYPE_POPUP;
				sprintf(tmpbuf, "%d%%", g_bat_soc);
				mmi_asc_to_ucs2(infor.text, tmpbuf);
				infor.img[0] = IMG_BAT_LOW_ICON_ADDR;
				infor.img_count = 1;
				DisplayPopUp(infor);
			}
		}
		else if(g_bat_soc < 80)
		{
			g_bat_level = BAT_LEVEL_NORMAL;
		}
		else
		{
			g_bat_level = BAT_LEVEL_GOOD;
		}

		if(charger_is_connected)
		{
			g_bat_level = BAT_LEVEL_NORMAL;
			if(screen_id == SCREEN_ID_NOTIFY)
			{
				sprintf(tmpbuf, "%d%%", g_bat_soc);
				mmi_asc_to_ucs2(notify_msg.text, tmpbuf);
				
				scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
				scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_POP_STR;
			}
		}

		if(g_chg_status == BAT_CHARGING_NO)
			pmu_redraw_bat_flag = true;
	}
	if(MSB&0x10)
	{
		//HD (SOC low) is set when SOC crosses the value in CONFIG.ATHD
		MSB = MSB&0xEF;
	#ifdef PMU_DEBUG
		LOGD("SOC low alert!");
	#endif
	}
	if(MSB&0x08)
	{
		//VR (voltage reset) is set after the device has been reset if EnVr is set.
		MSB = MSB&0xF7;
	#ifdef PMU_DEBUG
		LOGD("voltage reset alert!");
	#endif
	}
	if(MSB&0x04)
	{
		//VL (voltage low) is set when VCELL has been below ALRT.VALRTMIN
		MSB = MSB&0xFB;
	#ifdef PMU_DEBUG
		LOGD("voltage low alert!");
	#endif
	}
	if(MSB&0x02)
	{
		//VH (voltage high) is set when VCELL has been above ALRT.VALRTMAX
		MSB = MSB&0xFD;
	#ifdef PMU_DEBUG
		LOGD("voltage high alert!");
	#endif
	}
	if(MSB&0x01)
	{
		//RI (reset indicator) is set when the device powers up.
		//Any time this bit is set, the IC is not configured, so the
		//model should be loaded and the bit should be cleared
		MSB = MSB&0xFE;
	#ifdef PMU_DEBUG
		LOGD("reset indicator alert!");
	#endif

		MAX20353_QuickStart();
	}

	MAX20353_SOCWriteReg(0x1A, MSB, LSB);
	MAX20353_SOCWriteReg(0x0C, 0x12, 0x5C);

	return true;
#endif	
}

void PmuAlertHandle(void)
{
	pmu_alert_flag = true;
}

void MAX20353_InitData(void)
{
	u8_t status0,status1;
	
	MAX20353_ReadReg(REG_STATUS0, &status0);
	switch((status0&0x07))
	{
	case 0x00://Charger off
	case 0x01://Charging suspended due to temperature (see battery charger state diagram)
	case 0x07://Charger fault condition (see battery charger state diagram)
		g_chg_status = BAT_CHARGING_NO;
		break;
		
	case 0x02://Pre-charge in progress
	case 0x03://Fast-charge constant current mode in progress
	case 0x04://Fast-charge constant voltage mode in progress
	case 0x05://Maintain charge in progress
		g_chg_status = BAT_CHARGING_PROGRESS;
		break;
		
	case 0x06://Maintain charger timer done
		g_chg_status = BAT_CHARGING_FINISHED;
		break;
	}
	
	MAX20353_ReadReg(REG_STATUS1, &status1);
	if((status1&0x08) == 0x08) //USB OK   
	{
		pmu_battery_stop_shutdown();
		InitCharger();

		charger_is_connected = true;
		g_chg_status = BAT_CHARGING_PROGRESS;
	
	#ifdef BATTERY_SOC_GAUGE	
		g_bat_soc = MAX20353_CalculateSOC();
		if(g_bat_soc>100)
			g_bat_soc = 100;
		
		if(g_bat_soc < 5)
		{
			g_bat_level = BAT_LEVEL_VERY_LOW;
			pmu_battery_low_shutdown();
		}
		else if(g_bat_soc < 20)
		{
			g_bat_level = BAT_LEVEL_LOW;
		}
		else if(g_bat_soc < 80)
		{
			g_bat_level = BAT_LEVEL_NORMAL;
		}
		else
		{
			g_bat_level = BAT_LEVEL_GOOD;
		}
	#endif
	}
	else
	{			
		charger_is_connected = false;
		g_chg_status = BAT_CHARGING_NO;
		
	#ifdef BATTERY_SOC_GAUGE	
		g_bat_soc = MAX20353_CalculateSOC();
		if(g_bat_soc>100)
			g_bat_soc = 100;
		
		if(g_bat_soc < 5)
		{
			g_bat_level = BAT_LEVEL_VERY_LOW;
			pmu_battery_low_shutdown();
		}
		else if(g_bat_soc < 20)
		{
			g_bat_level = BAT_LEVEL_LOW;
		}
		else if(g_bat_soc < 80)
		{
			g_bat_level = BAT_LEVEL_NORMAL;
		}
		else
		{
			g_bat_level = BAT_LEVEL_GOOD;
		}
	#endif
	}
}

void pmu_init(void)
{
	bool rst;
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_LOW|GPIO_INT_DEBOUNCE;

#ifdef PMU_DEBUG
	LOGD("pmu_init");
#endif
  	//端口初始化
  	gpio_pmu = device_get_binding(PMU_PORT);
	if(!gpio_pmu)
	{
	#ifdef PMU_DEBUG
		LOGD("Cannot bind gpio device");
	#endif
		return;
	}

	//charger interrupt
	gpio_pin_configure(gpio_pmu, PMU_EINT, flag);
	gpio_pin_disable_callback(gpio_pmu, PMU_EINT);
	gpio_init_callback(&gpio_cb1, PmuInterruptHandle, BIT(PMU_EINT));
	gpio_add_callback(gpio_pmu, &gpio_cb1);
	gpio_pin_enable_callback(gpio_pmu, PMU_EINT);

	//alert interrupt
	gpio_pin_configure(gpio_pmu, PMU_ALRTB, flag);
	gpio_pin_disable_callback(gpio_pmu, PMU_ALRTB);
	gpio_init_callback(&gpio_cb2, PmuAlertHandle, BIT(PMU_ALRTB));
	gpio_add_callback(gpio_pmu, &gpio_cb2);
	gpio_pin_enable_callback(gpio_pmu, PMU_ALRTB);

	rst = init_i2c();
	if(!rst)
		return;

	pmu_dev_ctx.write_reg = platform_write;
	pmu_dev_ctx.read_reg  = platform_read;
	pmu_dev_ctx.handle    = i2c_pmu;

	pmu_check_ok = MAX20353_Init();
	if(!pmu_check_ok)
		return;
	
	MAX20353_InitData();

	VibrateStart();
	k_sleep(K_MSEC(100));
	VibrateStop();

#ifdef PMU_DEBUG
	LOGD("pmu_init done!");
#endif
}

void test_pmu(void)
{
    pmu_init();
}

//******************************************************************************
int MAX20303_CheckPMICStatusRegisters(unsigned char buf_results[5])
{ 
	int ret;

	ret  = MAX20353_ReadReg(REG_STATUS0, &buf_results[0]);
	ret |= MAX20353_ReadReg(REG_STATUS1, &buf_results[1]);
	ret |= MAX20353_ReadReg(REG_STATUS2, &buf_results[2]);
	ret |= MAX20353_ReadReg(REG_STATUS3, &buf_results[3]);
	ret |= MAX20353_ReadReg(REG_SYSTEM_ERROR, &buf_results[4]);
	return ret;
}

#ifdef BATTERY_SOC_GAUGE
void test_soc_status(void)
{
	u8_t MSB,LSB;
	u8_t RCOMP,Status0,Status1,Status2,Status3;
	u16_t VCell,SOC,CRate,MODE,Version,HIBRT,Config,Status,VALRT,VReset,CMD,OCV;
	u8_t strbuf[512] = {0};
	
	MAX20353_SOCReadReg(0x02, &MSB, &LSB);//vcell
	VCell = ((MSB<<8)+LSB);
	VCell = VCell*625/8/1000;
	
	MAX20353_SOCReadReg(0x04, &MSB, &LSB);//soc
	SOC = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x0C, &MSB, &LSB);//Config RCOMP(MSB)
	RCOMP = MSB;
	Config = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x16, &MSB, &LSB);//CRate
	CRate = ((MSB<<8)+LSB);
	if(CRate&0x8000==0x8000)
		CRate |= 0xFFFF0000;
	CRate = CRate*208;
	
	MAX20353_SOCReadReg(0x06, &MSB, &LSB);//MODE
	MODE = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x08, &MSB, &LSB);//Version
	Version = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x0A, &MSB, &LSB);//HIBRT
	HIBRT = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x1A, &MSB, &LSB);//Status
	Status = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x14, &MSB, &LSB);//VALRT
	VALRT = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x18, &MSB, &LSB);//VReset
	VReset = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0xFE, &MSB, &LSB);//CMD
	CMD = ((MSB<<8)+LSB);
	
	MAX20353_SOCReadReg(0x0E, &MSB, &LSB);//OCV
	OCV = ((MSB<<8)+LSB);

	MAX20353_ReadReg(REG_STATUS0, &Status0);
	Status0 = Status0&0x07;

#ifdef PMU_DEBUG	
	sprintf(strbuf, "%02d/%02d/%04d-%02d:%02d:%02d %2.3f,%3.8f,0x%02X,%1.5f,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,%d\n", 
				date_time.day,date_time.month,date_time.year,date_time.hour,date_time.minute,date_time.second,
				(float)VCell/1000, (float)SOC/256.0,
				RCOMP,
				(float)CRate/1000/100,
				MODE, Version, HIBRT, Config, Status, VALRT, VReset, CMD, OCV, Status0);

	LOGD("%s", strbuf);
#endif
}

void test_soc_timerout(struct k_timer *timer_id)
{
	read_soc_status = true;
}

void test_soc(void)
{
	k_timer_start(&soc_timer, K_MSEC(10*1000), K_MSEC(15*1000));
}
#endif/*BATTERY_SOC_GAUGE*/

void PMURedrawBatStatus(void)
{
	if(screen_id == SCREEN_ID_IDLE)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_BAT;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

#ifdef BATTERT_NTC_CHECK
void PMUUpdateTempForSOC(void)
{
	MAX20353_UpdateTemper();
}
#endif

void GetBatterySocString(u8_t *str_utc)
{
	if(str_utc == NULL)
		return;

	sprintf(str_utc, "%d", g_bat_soc);
}

void PMUMsgProcess(void)
{
	u8_t val;
#ifdef PMU_DEBUG	
	static u32_t i=0;
#endif

	if(pmu_trige_flag)
	{
	#ifdef PMU_DEBUG
		LOGD("int");
	#endif	
		if(pmu_check_ok)
			pmu_interrupt_proc();
		
		pmu_trige_flag = false;
	}

	if(gpio_pin_read(gpio_pmu, PMU_EINT, &val) == 0)	//xb add 20201202 防止多个中断同时触发，MCU没及时处理导致PMU中断脚一直拉低
	{
		if(val == 0)
		{
		#ifdef PMU_DEBUG
			i++;
			LOGD("count:%d", i);
		#endif	
			pmu_reg_proc();
		}
	#ifdef PMU_DEBUG
		else
		{
			i = 0;
		}
	#endif	
	}
	
	if(pmu_alert_flag)
	{
	#ifdef PMU_DEBUG
		LOGD("alert");
	#endif
		if(pmu_check_ok)
		{
			bool ret;
			
			ret = pmu_alert_proc();
			if(ret)
			{
				pmu_alert_flag = false;
			}
		}
		else
		{
			pmu_alert_flag = false;
		}
	}

	if(lowbat_pwr_off_flag)
	{
		system_power_off(1);
		lowbat_pwr_off_flag = false;
	}
	
	if(key_pwroff_flag)
	{
		system_power_off(2);
		key_pwroff_flag = false;
	}
	
	if(sys_pwr_off_flag)
	{
		if(pmu_check_ok)
			SystemShutDown();
		
		sys_pwr_off_flag = false;
	}
	
	if(vibrate_start_flag)
	{
		if(pmu_check_ok)
			VibrateStart();
		
		vibrate_start_flag = false;
	}
	
	if(vibrate_stop_flag)
	{
		if(pmu_check_ok)
			VibrateStop();
		
		vibrate_stop_flag = false;
	}

#ifdef BATTERY_SOC_GAUGE
	if(read_soc_status)
	{
		if(pmu_check_ok)
			test_soc_status();
		
		read_soc_status = false;
	}
#endif

	if(pmu_redraw_bat_flag)
	{
		if(pmu_check_ok)
			PMURedrawBatStatus();
		
		pmu_redraw_bat_flag = false;
	}

#ifdef BATTERT_NTC_CHECK
	if(pmu_check_temp_flag)
	{
		if(pmu_check_ok)
			PMUUpdateTempForSOC();
		
		pmu_check_temp_flag = false;
	}
#endif
}

void MAX20353_ReadStatus(void)
{
	u8_t Status0,Status1,Status2,Status3;
	
	MAX20353_ReadReg(REG_STATUS0, &Status0);
	MAX20353_ReadReg(REG_STATUS1, &Status1);
	MAX20353_ReadReg(REG_STATUS2, &Status2);
	MAX20353_ReadReg(REG_STATUS3, &Status3);
#ifdef PMU_DEBUG
	LOGD("Status0=0x%02X,Status1=0x%02X,Status2=0x%02X,Status3=0x%02X", Status0, Status1, Status2, Status3); 
#endif
}

void test_bat_soc(void)
{
#ifdef SHOW_LOG_IN_SCREEN
	sprintf(tmpbuf, "SOC:%d\n", g_bat_soc);
	show_infor1(tmpbuf);
#endif
}
