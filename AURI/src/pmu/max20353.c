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

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(max20353, CONFIG_LOG_DEFAULT_LEVEL);

//#define SHOW_LOG_IN_SCREEN

static u8_t PMICStatus[4], PMICInts[3];
static struct device *i2c_pmu;
static struct device *gpio_pmu;
static struct gpio_callback gpio_cb1,gpio_cb2;
static struct k_timer soc_timer,soc_pwroff;

bool sys_pwr_off = false;
bool vibrate_start_flag = false;
bool vibrate_stop_flag = false;
bool pmu_trige_flag = false;
bool pmu_alert_flag = false;
bool pmu_bat_flag = false;
bool pmu_check_temp_flag = false;
bool pmu_redraw_bat_flag = true;

bool read_soc_status = false;
bool charger_is_connected = false;
bool pmu_bat_has_notify = false;

u8_t g_bat_soc = 0;
BAT_CHARGER_STATUS g_chg_status = BAT_CHARGING_NO;
BAT_LEVEL_STATUS g_bat_level = BAT_LEVEL_0;

maxdev_ctx_t pmu_dev_ctx;

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
		LOG_INF("ERROR SETTING UP I2C\r\n");
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
	u8_t i=0;
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

void Set_Screen_Backlight_On(void)
{
	int ret = 0;

	ret = MAX20353_LED1(2, 31, true);
}

void Set_Screen_Backlight_Off(void)
{
	int ret = 0;

	ret = MAX20353_LED1(2, 0, false);
}

void SystemShutDown(void)
{
	SaveSystemDateTime();
	
	MAX20353_PowerOffConfig();
}

void pmu_battery_low_shutdown_timerout(void)
{
	sys_pwr_off = true;
}

void pmu_battery_stop_shutdown(void)
{
	if(k_timer_remaining_get(&soc_pwroff) > 0)
		k_timer_stop(&soc_pwroff);
}

void pmu_battery_low_shutdown(void)
{
	k_timer_init(&soc_pwroff, pmu_battery_low_shutdown_timerout, NULL);
	k_timer_start(&soc_pwroff, K_MSEC(10*1000), NULL);
}

void pmu_charge_complete(void)
{
	MAX20353_LED0(2,10,true);//green led on
	MAX20353_LED2(2,10,false);//red led off
}

void pmu_charge_connected(void)
{
	MAX20353_LED0(2,10,false);//green led off
	MAX20353_LED2(2,10,true);//red led on
}

void pmu_charge_disconnected(void)
{
	MAX20353_LED0(2,10,false);//green led off
	MAX20353_LED2(2,10,false);//red led off
}

void pmu_interrupt_proc(void)
{
	u8_t int0,status0,status1;
	u8_t val;
	
	do
	{
		MAX20353_ReadReg(REG_INT0, &int0);
		//LOG_INF("pmu_interrupt_proc REG_INT0:%02X\n", int0);

		if((int0&0x40) == 0x40) //Charger status change INT  
		{
			MAX20353_ReadReg(REG_STATUS0, &status0);
			//LOG_INF("REG_STATUS0:%02X\n", status0);
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
				pmu_charge_complete();
				
				g_chg_status = BAT_CHARGING_FINISHED;
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
				pmu_battery_stop_shutdown();
				
				InitCharger();

				pmu_charge_connected();
				
				charger_is_connected = true;
				
				g_chg_status = BAT_CHARGING_PROGRESS;

				lcd_sleep_out = true;
			}
			else
			{	
				pmu_charge_disconnected();
				
				charger_is_connected = false;
				
				g_chg_status = BAT_CHARGING_NO;
				g_bat_soc = MAX20353_CalculateSOC();
				if(g_bat_soc>100)
					g_bat_soc = 100;
				
				if(g_bat_soc < 5)
				{
					g_bat_level = BAT_LEVEL_0;
					pmu_battery_low_shutdown();
				}
				else if(g_bat_soc < 10)
				{
					g_bat_level = BAT_LEVEL_0;
				}
				else if(g_bat_soc < 20)
				{
					g_bat_level = BAT_LEVEL_1;
				}
				else if(g_bat_soc < 40)
				{
					g_bat_level = BAT_LEVEL_2;
				}
				else if(g_bat_soc < 60)
				{
					g_bat_level = BAT_LEVEL_3;
				}
				else if(g_bat_soc < 80)
				{
					g_bat_level = BAT_LEVEL_4;
				}
				else
				{
					g_bat_level = BAT_LEVEL_5;
				}
				
				lcd_sleep_out = true;
			}

			pmu_redraw_bat_flag = true;
		}

		if(gpio_pin_read(gpio_pmu, PMU_EINT, &val))	//xb add 20201202 防止多个中断同时触发，MCU没及时处理导致PMU中断脚一直拉低
		{
			//LOG_INF("Cannot get pin");
			break;
		}
	}while(!val);
}

void PmuInterruptHandle(void)
{
	pmu_trige_flag = true;
}

//An alert can indicate many different conditions. The
//STATUS register identifies which alert condition was met.
//Clear the corresponding bit after servicing the alert
void pmu_alert_proc(void)
{
	u8_t buff[128] = {0};
	u8_t MSB,LSB;

	MAX20353_SOCReadReg(0x1A, &MSB, &LSB);
	//LOG_INF("pmu_alert_proc status:%02X\n", MSB);
	if(MSB&0x40)
	{
		//EnVr (enable voltage reset alert)
		MSB = MSB&0xBF;

		LOG_INF("voltage reset alert!\n");
	}
	if(MSB&0x20)
	{
		//SC (1% SOC change) is set when SOC changes by at least 1% if CONFIG.ALSC is set
		MSB = MSB&0xDF;

		g_bat_soc = MAX20353_CalculateSOC();
		if(g_bat_soc>100)
			g_bat_soc = 100;

		LOG_INF("SOC:%d\n", g_bat_soc);
		if(g_bat_soc < 5)
		{
			g_bat_level = BAT_LEVEL_0;
			if(!charger_is_connected)
			{
				//DisplayPopUp("Battery voltage is very low, the system will shut down in a few seconds!");
				pmu_battery_low_shutdown();
			}
		}
		else if(g_bat_soc < 10)
		{
			g_bat_level = BAT_LEVEL_0;
		}
		else if(g_bat_soc < 20)
		{
			g_bat_level = BAT_LEVEL_1;
			if(!charger_is_connected)
			{
				//DisplayPopUp("Battery voltage is low, please charge in time!");
			}
		}
		else if(g_bat_soc < 40)
		{
			g_bat_level = BAT_LEVEL_2;
		}
		else if(g_bat_soc < 60)
		{
			g_bat_level = BAT_LEVEL_3;
		}
		else if(g_bat_soc < 80)
		{
			g_bat_level = BAT_LEVEL_4;
		}
		else
		{
			g_bat_level = BAT_LEVEL_5;
		}

		pmu_redraw_bat_flag = true;
	}
	if(MSB&0x10)
	{
		//HD (SOC low) is set when SOC crosses the value in CONFIG.ATHD
		MSB = MSB&0xEF;

		LOG_INF("SOC low alert!\n");
	}
	if(MSB&0x08)
	{
		//VR (voltage reset) is set after the device has been reset if EnVr is set.
		MSB = MSB&0xF7;

		LOG_INF("voltage reset alert!\n");
	}
	if(MSB&0x04)
	{
		//VL (voltage low) is set when VCELL has been below ALRT.VALRTMIN
		MSB = MSB&0xFB;

		LOG_INF("voltage low alert!\n");
	}
	if(MSB&0x02)
	{
		//VH (voltage high) is set when VCELL has been above ALRT.VALRTMAX
		MSB = MSB&0xFD;

		LOG_INF("voltage high alert!\n");
	}
	if(MSB&0x01)
	{
		//RI (reset indicator) is set when the device powers up.
		//Any time this bit is set, the IC is not configured, so the
		//model should be loaded and the bit should be cleared
		MSB = MSB&0xFE;

		LOG_INF("reset indicator alert!\n");

		MAX20353_QuickStart();
	}

	MAX20353_SOCWriteReg(0x1A, MSB, LSB);
	MAX20353_SOCWriteReg(0x0C, 0x12, 0x5C);
}

void PmuAlertHandle(void)
{
	pmu_alert_flag = true;
}

void MAX20353_InitData(void)
{
	pmu_interrupt_proc();
	
	g_bat_soc = MAX20353_CalculateSOC();
	if(g_bat_soc>100)
		g_bat_soc = 100;

	if(g_bat_soc < 10)
	{
		g_bat_level = BAT_LEVEL_0;
	}
	else if(g_bat_soc < 20)
	{
		g_bat_level = BAT_LEVEL_1;
	}
	else if(g_bat_soc < 40)
	{
		g_bat_level = BAT_LEVEL_2;
	}
	else if(g_bat_soc < 60)
	{
		g_bat_level = BAT_LEVEL_3;
	}
	else if(g_bat_soc < 80)
	{
		g_bat_level = BAT_LEVEL_4;
	}
	else
	{
		g_bat_level = BAT_LEVEL_5;
	}

	//test_soc();
}

void pmu_init(void)
{
	bool rst;
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_LOW|GPIO_INT_DEBOUNCE;

	LOG_INF("pmu_init\n");

  	//端口初始化
  	gpio_pmu = device_get_binding(PMU_PORT);
	if(!gpio_pmu)
	{
		LOG_INF("Cannot bind gpio device\n");
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

	MAX20353_Init();
	MAX20353_InitData();
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
	
	sprintf(strbuf, "%02d/%02d/%04d-%02d:%02d:%02d %2.3f,%3.8f,0x%02X,%1.5f,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,0x%04X,%d\n", 
				date_time.day,date_time.month,date_time.year,date_time.hour,date_time.minute,date_time.second,
				(float)VCell/1000, (float)SOC/256.0,
				RCOMP,
				(float)CRate/1000/100,
				MODE, Version, HIBRT, Config, Status, VALRT, VReset, CMD, OCV, Status0);

	LOG_INF("%s", strbuf);
}

void test_soc_timerout(void)
{
	read_soc_status = true;
}

void test_soc(void)
{
	k_timer_init(&soc_timer, test_soc_timerout, NULL);
	k_timer_start(&soc_timer, K_MSEC(10*1000), K_MSEC(15*1000));
}

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

void PMUMsgProcess(void)
{
	if(pmu_trige_flag)
	{
		pmu_interrupt_proc();
		pmu_trige_flag = false;
	}
	
	if(pmu_alert_flag)
	{
		pmu_alert_proc();
		pmu_alert_flag = false;
	}
	
	if(sys_pwr_off)
	{
		SystemShutDown();
		sys_pwr_off = false;		
	}
	
	if(vibrate_start_flag)
	{
		VibrateStart();
		vibrate_start_flag = false;
	}
	
	if(vibrate_stop_flag)
	{
		VibrateStop();
		vibrate_stop_flag = false;
	}

	if(read_soc_status)
	{
		test_soc_status();
		read_soc_status = false;
	}

	if(pmu_redraw_bat_flag)
	{
		PMURedrawBatStatus();
		pmu_redraw_bat_flag = false;
	}

#ifdef BATTERT_NTC_CHECK
	if(pmu_check_temp_flag)
	{
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
	
	LOG_INF("Status0=0x%02X,Status1=0x%02X,Status2=0x%02X,Status3=0x%02X\n", Status0, Status1, Status2, Status3); 
}

void test_bat_soc(void)
{
#ifdef SHOW_LOG_IN_SCREEN
	sprintf(tmpbuf, "SOC:%d\n", g_bat_soc);
	show_infor1(tmpbuf);
#endif
}
