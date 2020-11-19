#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include "max20353_reg.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(max20353, CONFIG_LOG_DEFAULT_LEVEL);

static struct device *i2c_pmu;
static struct device *gpio_pmu;
static struct gpio_callback gpio_cb1,gpio_cb2;

bool pmu_trige_flag = false;
bool pmu_alert_flag = false;

maxdev_ctx_t pmu_dev_ctx;

static bool init_i2c(void)
{
	i2c_pmu = device_get_binding(PMU_DEV);
	if(!i2c_pmu)
	{
		printf("ERROR SETTING UP I2C\r\n");
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
	MAX20353_PowerOffConfig();
}

void ReInitCharger(void)
{
	MAX20353_ChargerCfg();
	MAX20353_ChargerCtrl();
}

void pmu_interrupt_proc(void)
{
	int ret = 0;
	uint8_t Int0, Status0,Status1;

	LOG_INF("pmu_interrupt_proc\n");

	ret |= MAX20353_ReadReg(REG_INT0, &Int0);
	if(Int0 & 0x08)
	{
		ret |= MAX20353_ReadReg(REG_STATUS0, &Status0);
		ret |= MAX20353_ReadReg(REG_STATUS1, &Status1);
		printf("Status0=0x%02X, Status1=0x%02X,", Status0, Status1); 

		if(((Status1&0x08)==0x08) && ((Status0&0x07)==0x00))
			printf("Charging Finished!\n");
		if(((Status1&0x08)==0x08) && ((Status0&0x07)==0x07))
			printf("Charger Fault!\n");

		if(Status1&0x08)
		{ 
			ReInitCharger();
			printf("USB Power Good!\n");
		}
		else
		{
			printf("USB Power Off!\n");
		}
	}
}

void PmuInterruptHandle(void)
{
	pmu_trige_flag = true;
}

void pmu_alert_proc(void)
{
	float bat_soc;

	LOG_INF("pmu_alert_proc\n");

	bat_soc = MAX20353_CalculateSOC();
	LOG_INF("bat_soc:%f\n", bat_soc);
}

void PmuAlertHandle(void)
{
	pmu_alert_flag = true;
}

void pmu_init(void)
{
	bool rst;
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_LOW|GPIO_INT_DEBOUNCE;

	LOG_INF("pmu_init\n");

  	//¶Ë¿Ú³õÊ¼»¯
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
}

void test_pmu(void)
{
    pmu_init();
}

