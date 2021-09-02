/****************************************Copyright (c)************************************************
** File Name:			    nau88c10.c
** Descriptions:			audio codec process source file
** Created By:				xie biao
** Created Date:			2021-03-02
** Modified Date:      		2021-03-02 
** Version:			    	V1.0
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>

#include "nau88c10.h"
#include "Lcd.h"
#include "datetime.h"
#include "settings.h"
#include "screen.h"
	
#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(nau88c10, CONFIG_LOG_DEFAULT_LEVEL);

static u8_t whoamI, rst;
static u8_t HardwareID;
static u8_t FirmwareID;

static struct device *i2c_audio;
static struct device *gpio_audio;

dev_ctx_t audio_dev_ctx;

static s32_t platform_write(struct device *handle, u8_t reg, u8_t *bufp, u16_t len)
{
	u8_t i=0;
	u8_t data[len+1];
	u32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(handle, data, len+1, NAU88C10_I2C_ADDR);

	return rslt;
}

static s32_t platform_read(struct device *handle, u8_t reg, u8_t *bufp, u16_t len)
{
	u32_t rslt = 0;

	rslt = i2c_write(handle, &reg, 1, NAU88C10_I2C_ADDR);
	if(rslt == 0)
	{
		rslt = i2c_read(handle, bufp, len, NAU88C10_I2C_ADDR);
	}

	return rslt;
}

static bool init_i2c(void)
{
	i2c_audio = device_get_binding(AUDIO_DEV);
	if(!i2c_audio)
	{
		LOG_INF("ERROR SETTING UP I2C\r\n");
		return false;
	} 
	else
	{
		i2c_configure(i2c_audio, I2C_SPEED_SET(I2C_SPEED_FAST));
		return true;
	}
}

static void init_gpio(void)
{
	gpio_audio = device_get_binding(AUDIO_PORT);
	gpio_pin_configure(gpio_audio, AUDIO_LDO_EN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_audio, AUDIO_LDO_EN, 1);

	Delay(5);//上电延迟一会
}

s32_t NAU88C10_WriteReg(MAU88C10_REG reg, u16_t value)
{ 
    s32_t ret;
	u8_t data[2] = {0};
	
	data[0] = value>>8;
	data[1] = (u8_t)value&0x00ff;
	
	reg = reg<<1;
	if(data[0] != 0)
		reg |= 0x01;

	ret = audio_dev_ctx.write_reg(audio_dev_ctx.handle, reg, &data[1], 1);
	if(ret != 0)
	{
		ret = NAU88C10_ERROR;  
	}
	else
	{
		ret = NAU88C10_NO_ERROR; 
	}

	return ret;
}

s32_t NAU88C10_ReadReg(MAU88C10_REG reg, u16_t *value)
{
    s32_t ret;
	u8_t data[2] = {0};

	reg = reg<<1;
	
	ret = audio_dev_ctx.read_reg(audio_dev_ctx.handle, reg, data, 2);
    if(ret != 0)
    {
        ret = NAU88C10_ERROR;
    }
    else
    {
        ret = NAU88C10_NO_ERROR;
    }

	*value = (data[0]<<8) | data[1];
	
    return ret;
}

void NAU88C10_GetDeviceID(u16_t *Device_ID)
{
	s32_t ret;
	
	ret = NAU88C10_ReadReg(REG_DEVICE_REVISION, Device_ID);
	if(ret == 0)
	{
		LOG_INF("get id success! %03x\n", *Device_ID);
	}
	else
	{
		LOG_INF("get id fail!\n");
	}
}

void NAU88C10_Init(void)
{
	u8_t i;
	u16_t value;

	NAU88C10_WriteReg(REG_SW_RESET, 0x112);
	// Add 150ms delay .	Karin 2016/8/24
	Delay(150);
	/*
	**[NAU8810_USB_AD_DA/Power Management]
	**0x1 = 0x1fd
	**0x2 = 0x1bf
	**0x3 = 0x1ef
	*/
	NAU88C10_WriteReg(REG_POWER_MG_1, 0x1fd);
	NAU88C10_WriteReg(REG_POWER_MG_2, 0x1bf);
	NAU88C10_WriteReg(REG_POWER_MG_3, 0x1ef);

	/*
	**[NAU8810_USB_AD_DA/Audio Control] 
	**0x4 = 0x119
	**0x5 = 0x0
	**0x5 = 0x20	16k  48k  8k
	**0x6 = 0x148 slave
	**0x6 = 0x149   master
	**0x6 = 0x1b9 master 16k
	**0x6 = 0x159 master 48k
	**0x6 = 0x1ed master 8k
	**0x7 = 0x0
	**0x8 = 0x0
	**0x9 = 0x0
	**0xa = 0xc
	**0xb = 0x1ff
	**0xc = 0x1ff
	**0xd = 0x0
	**0xe = 0x108
	**0xf = 0x1ff
	*/
	NAU88C10_WriteReg(REG_AI_FORMAT,  	0x119);
	NAU88C10_WriteReg(REG_COMPANDING, 	0x020);
	//NAU88C10_WriteReg(REG_CLOCK_CTRL_1, 0x1ed);   // Karin 2016/8/24
	NAU88C10_WriteReg(REG_CLOCK_CTRL_2, 0x000);
	//NAU88C10_WriteReg(0x000, 0x08);
	//NAU88C10_WriteReg(0x000, 0x09);
	NAU88C10_WriteReg(REG_DAC_CTRL, 	0x00c);
	NAU88C10_WriteReg(REG_DAC_VOL,  	0x1ff);
	//NAU88C10_WriteReg(0x1ff, 0x0c); 	  // Karin 2016/8/24
	//NAU88C10_WriteReg(0x000, 0x0d); 	  // Karin 2016/8/24
	NAU88C10_WriteReg(REG_ADC_CTRL, 	0x1f8);		  // 0x108 --> x1f8 . Karin 2016/8/24
	NAU88C10_WriteReg(REG_ADC_VOL,  	0x1ff);

	/*
	**[NAU8810_USB_AD_DA/Equalizer] 
	**0x12 = 0x12c
	**0x13 = 0x2c
	**0x14 = 0x2c
	**0x15 = 0x2c
	**0x16 = 0x2c
	*/
	NAU88C10_WriteReg(REG_EQ1_LOW_CUTOFF, 	0x12c);
	NAU88C10_WriteReg(REG_EQ2_PEAK_1, 		0x02c);
	NAU88C10_WriteReg(REG_EQ3_PEAK_2, 		0x02c);
	NAU88C10_WriteReg(REG_EQ4_PEAK_3, 		0x02c);
	NAU88C10_WriteReg(REG_EQ5_HIGI_CUTOFF, 	0x02c);

	/*
	**[NAU8810_USB_AD_DA/DAC Limiter] 
	**0x18 = 0x32
	**0x19 = 0x0
	*/
	NAU88C10_WriteReg(REG_DAC_LIMIT_1, 0x032);
	NAU88C10_WriteReg(REG_DAC_LIMIT_2, 0x000);

	/*
	**[NAU8810_USB_AD_DA/Notch Filter] 
	**0x1b = 0x0
	**0x1c = 0x0
	**0x1d = 0x0
	**0x1e = 0x0
	*/
	NAU88C10_WriteReg(REG_NFC_0_HIGI, 0x000);
	NAU88C10_WriteReg(REG_NFC_0_LOW,  0x000);
	NAU88C10_WriteReg(REG_NFC_1_HIGI, 0x000);
	NAU88C10_WriteReg(REG_NFC_1_LOW,  0x000);

	/*
	**[NAU8810_USB_AD_DA/ALC Control] 
	**0x20 = 0x38
	**0x21 = 0xb
	**0x22 = 0x32
	**0x23 = 0x10
	*/
	NAU88C10_WriteReg(REG_ALC_CTRL_1, 0x038);
	NAU88C10_WriteReg(REG_ALC_CTRL_2, 0x00b);
	NAU88C10_WriteReg(REG_ALC_CTRL_3, 0x032);
	NAU88C10_WriteReg(REG_NOISE_GATE, 0x010);

	/*
	**[NAU8810_USB_AD_DA/PLL Control] 
	**0x24 = 0x8
	**0x24 = 0x006	  16k  48k
	**0x25 = 0xc
	**0x25 = 0x009	  16k  48k
	**0x26 = 0x93
	**0x26 = 0x06e	16k   48k
	**0x27 = 0xe9
	**0x27 = 0x12f	16k    48k
	*/
	NAU88C10_WriteReg(REG_PLL_N_CTRL,	0x006);
	NAU88C10_WriteReg(REG_PLL_K_1,		0x009);
	NAU88C10_WriteReg(REG_PLL_K_2,		0x06e);
	NAU88C10_WriteReg(REG_PLL_K_3,		0x12f);
	NAU88C10_WriteReg(REG_CLOCK_CTRL_1,	0x1ed);	  // Karin 2016/8/24
	/*
	**[NAU8810_USB_AD_DA/BYP Control] 
	**0x28 = 0xe9
	*/
	//NAU88C10_WriteReg(REG_ATT_CTRL, 0x0e9); 	  // Karin 2016/8/24

	/*
	**[NAU8810_USB_AD_DA/Input Output Mixer] 
	**0x2c = 0x33
	**0x2d = 0x11e
	**0x2e = 0x150
	**0x2f = 0x100
	**0x30 = 0x100
	**0x31 = 0x43
	**0x32 = 0x1
	**0x33 = 0x0
	**0x34 = 0x139
	**0x35 = 0x139
	**0x36 = 0x139
	**0x37 = 0x139
	**0x38 = 0x1
	**0x39 = 0x40
	**0x3a = 0x0
	*/
	NAU88C10_WriteReg(REG_INPUT_CTRL, 0x003);	  // Karin 2016/8/24
	NAU88C10_WriteReg(REG_PGA_GAIN, 0x11e);
	//NAU88C10_WriteReg(0x150, 0x2e);   // Karin 2016/8/24
	NAU88C10_WriteReg(REG_ADC_BOOST, 0x100);
	//NAU88C10_WriteReg(0x100, 0x30);   // Karin 2016/8/24
	NAU88C10_WriteReg(REG_OUTPUT_CTRL, 0x002);	  // Karin 2016/8/24
	//NAU88C10_WriteReg(REG_MIXER_CTRL, 0x001);   // Karin 2016/8/24
	//NAU88C10_WriteReg(0x000, 0x33);   // Karin 2016/8/24
	//NAU88C10_WriteReg(0x139, 0x34);   // Karin 2016/8/24
	//NAU88C10_WriteReg(0x139, 0x35);   // Karin 2016/8/24
	NAU88C10_WriteReg(REG_SPKOUT_VOL, 0x139);
	//NAU88C10_WriteReg(0x139, 0x37);   // Karin 2016/8/24
	//NAU88C10_WriteReg(REG_MONO_MIXER_CTRL, 0x001);   // Karin 2016/8/24
	//NAU88C10_WriteReg(0x040, 0x39);   // Karin 2016/8/24
	//NAU88C10_WriteReg(REG_POWER_MG_4, 0x000);   // Karin 2016/8/24
}

void Audio_device_init(void)
{
	LOG_INF("Audio_init\n");

	init_gpio();
	
	if(!init_i2c())
		return;

	audio_dev_ctx.write_reg = platform_write;
	audio_dev_ctx.read_reg = platform_read;
	audio_dev_ctx.handle = i2c_audio;

	NAU88C10_Init();
	
	LOG_INF("Audio_init done!\n");
}

void AudioMsgProcess(void)
{

}

