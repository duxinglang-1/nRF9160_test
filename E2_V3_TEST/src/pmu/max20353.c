#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
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

#ifdef GPIO_ACT_I2C
#define PMU_SCL		0
#define PMU_SDA		1

#else/*GPIO_ACT_I2C*/

#define I2C1_NODE DT_NODELABEL(i2c1)
#if DT_NODE_HAS_STATUS(I2C1_NODE, okay)
#define PMU_DEV	DT_LABEL(I2C1_NODE)
#else
/* A build error here means your board does not have I2C enabled. */
#error "i2c1 devicetree node is disabled"
#define PMU_DEV	""
#endif

#define PMU_SCL			31
#define PMU_SDA			30

#endif/*GPIO_ACT_I2C*/

#define PMU_PORT 		"GPIO_0"
#define PMU_ALRTB		7
#define PMU_EINT		8

static bool pmu_check_ok = false;
static uint8_t PMICStatus[4], PMICInts[3];
static struct device *i2c_pmu;
static struct device *gpio_pmu;
static struct gpio_callback gpio_cb1,gpio_cb2;

static void test_soc_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(soc_timer, test_soc_timerout, NULL);
static void pmu_battery_low_shutdown_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(soc_pwroff, pmu_battery_low_shutdown_timerout, NULL);
static void sys_pwr_off_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(sys_pwroff, sys_pwr_off_timerout, NULL);
static void vibrate_start_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(vib_start_timer, vibrate_start_timerout, NULL);
static void vibrate_stop_timerout(struct k_timer *timer_id);
K_TIMER_DEFINE(vib_stop_timer, vibrate_stop_timerout, NULL);

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

uint8_t g_bat_soc = 0;

BAT_CHARGER_STATUS g_chg_status = BAT_CHARGING_NO;
BAT_LEVEL_STATUS g_bat_level = BAT_LEVEL_NORMAL;
vibrate_msg_t g_vib = {0};

maxdev_ctx_t pmu_dev_ctx;

extern bool key_pwroff_flag;

#ifdef SHOW_LOG_IN_SCREEN
static uint8_t tmpbuf[256] = {0};

static void show_infor1(uint8_t *strbuf)
{
	//LCD_Clear(BLACK);
	LCD_Fill(30,50,180,70,BLACK);
	LCD_ShowStringInRect(30,50,180,70,strbuf);
}

static void show_infor2(uint8_t *strbuf)
{
	//LCD_Clear(BLACK);
	LCD_Fill(30,130,180,70,BLACK);
	LCD_ShowStringInRect(30,130,180,70,strbuf);
}
#endif

#ifdef GPIO_ACT_I2C
void I2C_INIT(void)
{
	if(gpio_pmu == NULL)
		gpio_pmu = device_get_binding(PMU_PORT);

	gpio_pin_configure(gpio_pmu, PMU_SCL, GPIO_OUTPUT);
	gpio_pin_configure(gpio_pmu, PMU_SDA, GPIO_OUTPUT);
	gpio_pin_set(gpio_pmu, PMU_SCL, 1);
	gpio_pin_set(gpio_pmu, PMU_SDA, 1);
}

void I2C_SDA_OUT(void)
{
	gpio_pin_configure(gpio_pmu, PMU_SDA, GPIO_OUTPUT);
}

void I2C_SDA_IN(void)
{
	gpio_pin_configure(gpio_pmu, PMU_SDA, GPIO_INPUT);
}

void I2C_SDA_H(void)
{
	gpio_pin_set(gpio_pmu, PMU_SDA, 1);
}

void I2C_SDA_L(void)
{
	gpio_pin_set(gpio_pmu, PMU_SDA, 0);
}

void I2C_SCL_H(void)
{
	gpio_pin_set(gpio_pmu, PMU_SCL, 1);
}

void I2C_SCL_L(void)
{
	gpio_pin_set(gpio_pmu, PMU_SCL, 0);
}

void Delay_ms(unsigned int dly)
{
	k_sleep(K_MSEC(dly));
}

void Delay_us(unsigned int dly)
{
	k_usleep(dly);
}

//产生起始信号
void I2C_Start(void)
{
	I2C_SDA_OUT();

	I2C_SDA_H();
	I2C_SCL_H();
	//Delay_us(10);
	I2C_SDA_L();
	//Delay_us(10);
	I2C_SCL_L();
}

//产生停止信号
void I2C_Stop(void)
{
	I2C_SDA_OUT();

	I2C_SCL_L();
	I2C_SDA_L();
	I2C_SCL_H();
	//Delay_us(10);
	I2C_SDA_H();
	//Delay_us(10);
}

//主机产生应答信号ACK
void I2C_Ack(void)
{
	I2C_SDA_OUT();
	I2C_SCL_L();

	I2C_SDA_L();
	I2C_SCL_H();
	I2C_SCL_L();
}

//主机不产生应答信号NACK
void I2C_NAck(void)
{
	I2C_SDA_OUT();
	I2C_SCL_L();

	I2C_SDA_H();
	I2C_SCL_H();
	I2C_SCL_L();
}

//等待从机应答信号
//返回值：1 接收应答失败
//		  0 接收应答成功
uint8_t I2C_Wait_Ack(void)
{
	uint8_t val,tempTime=0;

	I2C_SDA_IN();
	I2C_SCL_H();

	while(1)
	{
		val = gpio_pin_get_raw(gpio_pmu, PMU_SDA);
		if(val == 0)
			break;
		
		tempTime++;
		if(tempTime>250)
		{
			I2C_Stop();
			return 1;
		}	 
	}

	I2C_SCL_L();
	return 0;
}

//I2C 发送一个字节
uint8_t I2C_Write_Byte(uint8_t txd)
{
	uint8_t i=0;

	I2C_SDA_OUT();
	I2C_SCL_L();//拉低时钟开始数据传输

	for(i=0;i<8;i++)
	{
		if((txd&0x80)>0) //0x80  1000 0000
			I2C_SDA_H();
		else
			I2C_SDA_L();

		txd<<=1;
		I2C_SCL_H();
		I2C_SCL_L();
	}

	return I2C_Wait_Ack();
}

//I2C 读取一个字节
void I2C_Read_Byte(bool ack, uint8_t *data)
{
   uint8_t i=0,receive=0,val=0;

   I2C_SDA_IN();
   for(i=0;i<8;i++)
   {
   		I2C_SCL_L();
		I2C_SCL_H();

		receive<<=1;
		val = gpio_pin_get_raw(gpio_pmu, PMU_SDA);
		if(val == 1)
		   receive++;
   }

   	if(ack == false)
	   	I2C_NAck();
	else
		I2C_Ack();

	*data = receive;
}

uint8_t I2C_write_data(uint8_t addr, uint8_t *databuf, uint16_t len)
{
	uint8_t i;

	addr = (addr<<1);

	I2C_Start();
	if(I2C_Write_Byte(addr))
		goto err;

	for(i=0;i<len;i++)
	{
		if(I2C_Write_Byte(databuf[i]))
			goto err;
	}

	I2C_Stop();
	return 0;
	
err:
	return -1;
}

uint8_t I2C_read_data(uint8_t addr, uint8_t *databuf, uint16_t len)
{
	uint8_t i;

	addr = (addr<<1)|1;

	I2C_Start();
	if(I2C_Write_Byte(addr))
		goto err;

	for(i=0;i<len;i++)
	{
		I2C_Read_Byte(false, &databuf[i]);
	}
	I2C_Stop();
	return 0;
	
err:
	return -1;
}
#endif

static bool init_i2c(void)
{
#ifdef GPIO_ACT_I2C
	I2C_INIT();
	return true;
#else
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
#endif	
}

static int32_t platform_write(struct device *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	uint32_t i=0;
	uint8_t data[len+1];
	uint32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
#ifdef GPIO_ACT_I2C
	rslt = I2C_write_data(MAX20353_I2C_ADDR, data, len+1);
#else
	rslt = i2c_write(handle, data, len+1, MAX20353_I2C_ADDR);
#endif
	return rslt;
}

static int32_t platform_read(struct device *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	uint32_t rslt = 0;

#ifdef GPIO_ACT_I2C
	rslt = I2C_write_data(MAX20353_I2C_ADDR, &reg, 1);
	if(rslt == 0)
	{
		rslt = I2C_read_data(MAX20353_I2C_ADDR, bufp, len);
	}
#else
	rslt = i2c_write(handle, &reg, 1, MAX20353_I2C_ADDR);
	if(rslt == 0)
	{
		rslt = i2c_read(handle, bufp, len, MAX20353_I2C_ADDR);
	}
#endif
	return rslt;
}

void PPG_Power_On(void)
{
	MAX20353_BoostConfig();
}

void PPG_Power_Off(void)
{
	MAX20353_BoostDisable();
}

void Set_Screen_Backlight_Level(BACKLIGHT_LEVEL level)
{
	int ret = 0;

	ret = MAX20353_LED0(0, (24*level)/BACKLIGHT_LEVEL_MAX, true);
	ret = MAX20353_LED1(0, (24*level)/BACKLIGHT_LEVEL_MAX, true);
}

void Set_Screen_Backlight_On(void)
{
	int ret = 0;

	ret = MAX20353_LED0(0, (24*global_settings.backlight_level)/BACKLIGHT_LEVEL_MAX, true);
	ret = MAX20353_LED1(0, (24*global_settings.backlight_level)/BACKLIGHT_LEVEL_MAX, true);
}

void Set_Screen_Backlight_Off(void)
{
	int ret = 0;

	ret = MAX20353_LED0(0, 0, false);
	ret = MAX20353_LED1(0, 0, false);
}

void sys_pwr_off_timerout(struct k_timer *timer_id)
{
	sys_pwr_off_flag = true;
}

void vibrate_start_timerout(struct k_timer *timer_id)
{
	vibrate_stop_flag = true;
}

void vibrate_stop_timerout(struct k_timer *timer_id)
{
	vibrate_start_flag = true;
}

void vibrate_off(void)
{
	memset(&g_vib, 0, sizeof(g_vib));
	
	vibrate_stop_flag = true;
}

void vibrate_on(VIBRATE_MODE mode, uint32_t mSec1, uint32_t mSec2)
{
	g_vib.work_mode = mode;
	g_vib.on_time = mSec1;
	g_vib.off_time = mSec2;
	
	switch(g_vib.work_mode)
	{
	case VIB_ONCE:
	case VIB_RHYTHMIC:
		k_timer_start(&vib_start_timer, K_MSEC(g_vib.on_time), K_NO_WAIT);
		break;

	case VIB_CONTINUITY:
		break;
	}

	vibrate_start_flag = true;
}

void system_power_off(uint8_t flag)
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
		k_sleep(K_MSEC(100));
		VibrateStop();

		k_timer_start(&sys_pwroff, K_MSEC(5*1000), K_MSEC(5*1000));
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
	k_timer_start(&soc_pwroff, K_MSEC(10*1000), K_NO_WAIT);
}

void pmu_battery_update(void)
{
	uint8_t tmpbuf[8] = {0};

	if(!pmu_check_ok)
		return;

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
			pmu_battery_low_shutdown();
		}
	}
	else if(g_bat_soc < 10)
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

	if(charger_is_connected)
	{
		g_bat_level = BAT_LEVEL_NORMAL;
	}

	if(g_chg_status == BAT_CHARGING_NO)
		pmu_redraw_bat_flag = true;
}

bool pmu_interrupt_proc(void)
{
	uint8_t i,val;
	uint8_t tmpbuf[128] = {0};
	notify_infor infor = {0};
	uint8_t int0,status0,status1;
	int ret;

	if(!pmu_check_ok)
		return true;
	
	ret = MAX20353_ReadReg(REG_INT0, &int0);
	if(ret == MAX20353_ERROR)
		return false;
	
	if((int0&0x40) == 0x40) //Charger status change INT  
	{
		ret = MAX20353_ReadReg(REG_STATUS0, &status0);
		if(ret == MAX20353_ERROR)
			return false;
		
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

			lcd_sleep_out = true;
			break;
		}

		pmu_redraw_bat_flag = true;
	}
	
	if((int0&0x08) == 0x08) //USB OK Int
	{
		ret = MAX20353_ReadReg(REG_STATUS1, &status1);
		if(ret == MAX20353_ERROR)
			return false;
		
		if((status1&0x08) == 0x08) //USB OK   
		{
		#ifdef PMU_DEBUG
			LOGD("charger push in!");
		#endif	
			pmu_battery_stop_shutdown();
			
			InitCharger();

			charger_is_connected = true;
			
			g_chg_status = BAT_CHARGING_PROGRESS;
			g_bat_level = BAT_LEVEL_NORMAL;
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

			ExitNotify();
			lcd_sleep_out = true;
		}

		pmu_redraw_bat_flag = true;
	}

	val = gpio_pin_get_raw(gpio_pmu, PMU_EINT);//xb add 20201202 防止多个中断同时触发，MCU没及时处理导致PMU中断脚一直拉低
	if(val == 0)
		return false;
	else
		return true;
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
	uint8_t i;
	notify_infor infor = {0};
	int ret;
	uint8_t MSB=0,LSB=0;

	if(!pmu_check_ok)
		return true;

#ifdef PMU_DEBUG
	LOGD("begin");
#endif

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
	#ifdef PMU_DEBUG
		LOGD("SOC change alert!");
	#endif

		pmu_battery_update();
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

	ret = MAX20353_SOCWriteReg(0x1A, MSB, LSB);
	if(ret == MAX20353_ERROR)
		return false;
	
	ret = MAX20353_SOCWriteReg(0x0C, 0x12, 0x5C);
	if(ret == MAX20353_ERROR)
		return false;

	return true;
#endif	
}

void PmuAlertHandle(void)
{
	pmu_alert_flag = true;
}

void MAX20353_InitData(void)
{
	uint8_t status0,status1;
	
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
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_UP;

#ifdef PMU_DEBUG
	LOGD("pmu_init");
#endif
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
	gpio_pin_interrupt_configure(gpio_pmu, PMU_EINT, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb1, PmuInterruptHandle, BIT(PMU_EINT));
	gpio_add_callback(gpio_pmu, &gpio_cb1);
	gpio_pin_interrupt_configure(gpio_pmu, PMU_EINT, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);

	//alert interrupt
	gpio_pin_configure(gpio_pmu, PMU_ALRTB, flag);
	gpio_pin_interrupt_configure(gpio_pmu, PMU_ALRTB, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb2, PmuAlertHandle, BIT(PMU_ALRTB));
	gpio_add_callback(gpio_pmu, &gpio_cb2);
	gpio_pin_interrupt_configure(gpio_pmu, PMU_ALRTB, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);

	rst = init_i2c();
	if(!rst)
		return;

	pmu_dev_ctx.write_reg = platform_write;
	pmu_dev_ctx.read_reg  = platform_read;
#ifdef GPIO_ACT_I2C
	pmu_dev_ctx.handle    = NULL;
#else
	pmu_dev_ctx.handle    = i2c_pmu;
#endif

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
	uint8_t MSB,LSB;
	uint8_t RCOMP,Status0,Status1,Status2,Status3;
	uint16_t VCell,SOC,CRate,MODE,Version,HIBRT,Config,Status,VALRT,VReset,CMD,OCV;
	uint8_t strbuf[512] = {0};
	
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
	if((screen_id == SCREEN_ID_IDLE)
		||(screen_id == SCREEN_ID_HR)
		||(screen_id == SCREEN_ID_SPO2)
		||(screen_id == SCREEN_ID_BP)
		||(screen_id == SCREEN_ID_TEMP)
		)
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

void GetBatterySocString(uint8_t *str_utc)
{
	if(str_utc == NULL)
		return;

	sprintf(str_utc, "%d", g_bat_soc);
}

void PMUMsgProcess(void)
{
	bool ret = false;
	uint8_t val;

	if(pmu_trige_flag)
	{
	#ifdef PMU_DEBUG
		LOGD("int");
	#endif	
		ret = pmu_interrupt_proc();
		if(ret)
		{
			pmu_trige_flag = false;
		}
	}
	
	if(pmu_alert_flag)
	{
	#ifdef PMU_DEBUG
		LOGD("alert");
	#endif
		ret = pmu_alert_proc();
		if(ret)
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

		if(g_vib.work_mode == VIB_RHYTHMIC)
		{
			k_timer_start(&vib_stop_timer, K_MSEC(g_vib.off_time), K_NO_WAIT);
		}
		else
		{
			memset(&g_vib, 0, sizeof(g_vib));
		}
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
	uint8_t Status0,Status1,Status2,Status3;
	
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
