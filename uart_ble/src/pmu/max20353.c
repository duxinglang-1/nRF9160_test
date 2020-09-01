#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include <sys/printk.h>
#include <dk_buttons_and_leds.h>
#include "max20353.h"

static u8_t HardwareID;
static u8_t FirmwareID;

u8_t i2cbuffer_[16];
u8_t appdatainoutbuffer_[8];
u8_t appcmdoutvalue_;

static struct device *i2c_pmu;
static struct device *gpio_pmu;
static struct gpio_callback gpio_cb;

bool pmu_trige_flag = false;

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
		printf("I2C CONFIGURED\r\n");
		return true;
	}
}

static s32_t platform_write(u8_t reg, u8_t *bufp, u16_t len)
{
	u8_t i=0;
	u8_t data[len+1];
	u32_t rslt = 0;

	data[0] = reg;
	memcpy(&data[1], bufp, len);
	rslt = i2c_write(i2c_pmu, data, len+1, MAX20303_I2C_ADDR);

	return rslt;
}

static s32_t platform_read(u8_t reg, u8_t *bufp, u16_t len)
{
	u32_t rslt = 0;

	rslt = i2c_write(i2c_pmu, &reg, 1, MAX20303_I2C_ADDR);
	if(rslt == 0)
	{
		rslt = i2c_read(i2c_pmu, bufp, len, MAX20303_I2C_ADDR);
	}
	
	return rslt;
}

void delay_ms(uint32_t period)
{
    k_sleep(K_MSEC(period));
}

int MAX20353_HapticConfig(void)
{
	int32_t i,  ret = 0;

	// Config 0
	appcmdoutvalue_ = 0xA0;
	appdatainoutbuffer_[0] = 0x0E;	//0x0F;	// EmfEn(resonance detection)/HptSel(LRA)/ALC/ZeroCrossHysteresis
	appdatainoutbuffer_[1] = 0xDA;	//0x9F;	// Initial guess of Back EMF frequency = 25.6M/64/IniGss =235/205; IniGss = 0x6A6/0x79F
	appdatainoutbuffer_[2] = 0x16;	//0x87;	// ZccSlowEn=0/FltrCntrEn=0
	appdatainoutbuffer_[3] = 0x00;	//0x00;	// Skip periods before BEMF measuring
	appdatainoutbuffer_[4] = 0x07;	//0x05;	// Wide Window for BEMF zero crossing
	appdatainoutbuffer_[5] = 0x02;	//0x01;	// Narrow Window for BEMF zero crossing
	ret |= MAX20353_AppWrite(6);


	// Config 1
	appcmdoutvalue_ = 0xA2;
	appdatainoutbuffer_[0] = 0xF0;	//0x01;	// EmfSkipCyc
	appdatainoutbuffer_[1] = 0x88;	//0x00;	// BlankWdw, zero corssing comparator blanking time after enable(1/25.6MHz)
	appdatainoutbuffer_[2] = 0x00;	//0x02;	// BlankWdw, zero corssing comparator blanking time after enable(1/25.6MHz)
	appdatainoutbuffer_[3] = 0x82;	//0x5D;	// Vpp_Sine_max = 5.65V, Vpp_Square_max = 4V, VFS= Vpp/2 = 2V, Max_VFS = 5.5V, set VFS = 2/5.5*255 = 92 = 0x5C(square), Sine: VFS=2.8/5.5*255=0x82
	appdatainoutbuffer_[4] = 0xE6;	//0xE6;	// ETRGOdAmp, Overdrive amplitude, LSB = 0.78%VFS, 98%
	appdatainoutbuffer_[5] = 0x10;	//0x10;	// ETRGOdDur, Overdrive period, LSB = 5ms, 80ms
	ret |= MAX20353_AppWrite(6);

	// Config 2
	appcmdoutvalue_ = 0xA4;
	appdatainoutbuffer_[0] = 0xC0;	//0xC0;	// ETRGActAmp, normal amplitude, , LSB = 0.78%VFS, don't care
	appdatainoutbuffer_[1] = 0xFF;	//0xFF;	// ETRGActDur, normal period, LSB = 10ms, don't care
	appdatainoutbuffer_[2] = 0xCD;	//0xCD;	// ETRGActAmp, braking amplitude, , LSB = 0.78%VFS, 80*VFS
	appdatainoutbuffer_[3] = 0x14;	//0x14;	// ETRGActDur, breaking period, LSB = 5ms, 60ms
	appdatainoutbuffer_[4] = 0x23;	//0x15;	// narrow window gain, wide window gain = 1
	appdatainoutbuffer_[5] = 0x06;	//0x00;	// periods from wide to narrow
	ret |= MAX20353_AppWrite(6);


	// Sys UVLO threshold = 1.6V
	appcmdoutvalue_ = 0xA6;
	appdatainoutbuffer_[0] = 0x4A;	//Sys UVLO threshold = 1.6V
	ret |= MAX20353_AppWrite(1);

	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x00);	//hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x12, disable HptDrvEn

	//Send Auto-Tune Command
	i = 0;
	do
	{
		appcmdoutvalue_ = 0xAC;
		ret |= MAX20353_WriteReg( REG_AP_CMDOUT,  appcmdoutvalue_);//0x17	//Pattern RAM Address

		k_sleep(K_MSEC(2500));	
		MAX20353_ReadRegMulti(REG_AP_RESPONSE, appdatainoutbuffer_, 4);//0x18
		i++;       
		printf("%02X %02X %02X %02X\n", appdatainoutbuffer_[0], appdatainoutbuffer_[1], appdatainoutbuffer_[2], appdatainoutbuffer_[3]);
	} while((appdatainoutbuffer_[1]!=0x00) && (i<10));	//freg = 400,000/(APDataIn2 | (APDataIn3<<8))

	if(ret != 0)
		return MAX20303_ERROR;

	return MAX20303_NO_ERROR;
}

int MAX20353_Buck1Config(void) 
{
    int32_t ret = 0;
	
    appcmdoutvalue_ = 0x35;
    appdatainoutbuffer_[0] = 0x00;  	//
    appdatainoutbuffer_[1] = 0x2C;  	//0x28    0.7+(0.025V * number)    0x48*0.025 =1.8v     //0.7V to 2.275V, Linear Scale, 25mV increments
    appdatainoutbuffer_[2] = 0x3F;  	//0x2F  	01 = 20mA, Use for 1V < Buck1VSet < 1.8V
    appdatainoutbuffer_[3] = 0x01;  	// Enable
    ret = MAX20353_AppWrite(4);
	
    printf(" MAX20353_Buck1Config \r\n" );
    return ret;
}

int MAX20353_Buck2Config(void) 
{
    int32_t ret = 0;
	
    appcmdoutvalue_ = 0x3A;
    appdatainoutbuffer_[0] = 0x01;      //
    appdatainoutbuffer_[1] = 0x32;     	//0x32    0.7V + (0.05V * number) = 3.3V;
    appdatainoutbuffer_[2] = 0x3F;		//  0x3F 375mA  01 = 20mA, Use for 1V < Buck2VSet < 1.8V
    appdatainoutbuffer_[3] = 0x01;		// Enable
    ret = MAX20353_AppWrite(4);

    return ret;
}

int MAX20353_LDO1Config(void)
{
    int32_t ret = 0;

    appcmdoutvalue_ = 0x40;
    appdatainoutbuffer_[0] = 0x01;     //0x01  0.5V to 1.95V, Linear Scale, 25mV increments,使能   LDO1  
    appdatainoutbuffer_[1] = 0x28;     //0x28  0.5V + (0.025V * number)   =  1.95V   1.8
    ret = MAX20353_AppWrite(2);
    
    return ret;
}

int MAX20353_LDO2Config(void)
{
    int32_t ret = 0;
	
    appcmdoutvalue_ = 0x42;
    appdatainoutbuffer_[0] = 0x01;
    appdatainoutbuffer_[1] = 0x13;     // 0.9V + (0.1V * number)   =  2.8V 
    ret = MAX20353_AppWrite(2);

    return ret;
}

int MAX20353_BoostConfig(void) 
{
	int32_t ret = 0;
	
	appcmdoutvalue_ = 0x30;
	appdatainoutbuffer_[0] = 0x01;
	appdatainoutbuffer_[1] = 0x00;
	appdatainoutbuffer_[2] = 0x00;
	appdatainoutbuffer_[3] = 0x00;     // 5V + (0.25V * number); 0x00:5V, 0x3B:20V; EVKIT's cap can only be upto 6.3V
	ret = MAX20353_AppWrite(4);

	return ret;
}

int MAX20353_ChargePumpConfig(void)
{
    int32_t ret = 0;
    appcmdoutvalue_ = 0x46;
    appdatainoutbuffer_[0] = 0x01;	// Boost Enabled
    appdatainoutbuffer_[1] = 0x01;	// 00 : 6.5V, 01: 5V
    ret = MAX20353_AppWrite(2);

    return ret;
}

/// @brief BuckBoost to 4.5V output rail **/
//******************************************************************************
int MAX20353_BuckBoostConfig(void) 
{
    int32_t ret = 0;
    appcmdoutvalue_ = 0x70;
    appdatainoutbuffer_[0] = 0x00;
    appdatainoutbuffer_[1] = 0x04;
    appdatainoutbuffer_[2] = 0x14;		// 2.5V + (0.1V * number) = 4.5V
    appdatainoutbuffer_[3] = 0x41;     
    ret = MAX20353_AppWrite(4);

    return ret;
}

/**************************************************
*Name:	MAX20353_LED1
*Function: LED 控制
*Parameter:
*			IStep = 0-2
*			Amplitude:0-31
*return: 
***************************************************/
int MAX20353_LED1(int IStep, int Amplitude)
{ 
	int ret = 0;
	
	ret |= MAX20353_WriteReg(REG_LED_STEP_DIRECT,  IStep&0x03);
	//Bit7: LED2Open, Bit6: LED1Open, Bit6: LED0Open, Bit1-0: LEDIStep: 0:0.6mA, 1:1.0mA, 2:1.2mA, 3:Reserved 
	ret |= MAX20353_WriteReg(REG_LED1_DIRECT,  0x20|(Amplitude&0x1F)); 
	//Bit7-5: 1:LED1On, Bit4-0: Amplitude, LED current = IStep*Amplitude 
}

void Set_Screen_Backlight_On(void)
{
	int ret = 0;
	
	ret |= MAX20353_WriteReg(REG_LED_STEP_DIRECT,  2&0x03);
	//Bit7: LED2Open, Bit6: LED1Open, Bit5: LED0Open, Bit1-0: LEDIStep: 0:0.6mA, 1:1.0mA, 2:1.2mA, 3:Reserved 
	ret |= MAX20353_WriteReg(REG_LED1_DIRECT,  0x20|(31&0x1F)); 
	//Bit7-5: 1:LED1On, Bit4-0: Amplitude, LED current = IStep*Amplitude 

}

void Set_Screen_Backlight_Off(void)
{
	int ret = 0;
	
	ret |= MAX20353_WriteReg(REG_LED_STEP_DIRECT,  2&0x03);
	//Bit7: LED2Open, Bit6: LED1Open, Bit6: LED0Open, Bit1-0: LEDIStep: 0:0.6mA, 1:1.0mA, 2:1.2mA, 3:Reserved 
	ret |= MAX20353_WriteReg(REG_LED1_DIRECT,  0x00); 
	//Bit7-5: 1:LED1On, Bit4-0: Amplitude, LED current = IStep*Amplitude 

}

unsigned char Pattern[256] = 
{	
	//0: 128 bytes[0-0x7F], no repeat, total 640ms; 3 sine wave; Address 0x00
	0x00,0x0A,0x14,0x1E,0x27,0x31,0x3A,0x43,0x4B,0x53,0x5A,0x61,0x67,0x6D,0x71,0x76,
	0x79,0x7C,0x7D,0x7E,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7E,0x7E,0x7C,0x7A,0x77,
	0x74,0x6F,0x6A,0x64,0x5E,0x56,0x4F,0x47,0x3E,0x35,0x2C,0x22,0x18,0x0E,0x04,0x00,

	//1: 64 bytes[0-0x3F], repeat 1 to 2 times, last byte delay 80ms, total (320ms+80ms)*(1 or 2)=400 or 800ms; soft start/stop/short; Address 0x80
	0x00,0x0E,0x1C,0x29,0x35,0x41,0x4C,0x57,0x62,0x6C,0x75,0x7E,0x7E,0x7E,0x7E,0x7E,
	0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
	0x7F,0x7F,0x73,0x68,0x5E,0x54,0x4A,0x41,0x39,0x30,0x28,0x21,0x1A,0x13,0x0C,0x00,

	//2: 32 bytes[0-0x1F], no repeat, total 160ms: soft start/stop/1 time very short; Address 0xC0
	0x00,0x0E,0x1C,0x29,0x35,0x41,0x4C,0x57,0x62,0x6C,0x75,0x7E,0x7E,0x7E,0x7E,0x7E,
	0x73,0x68,0x5D,0x53,0x4A,0x41,0x38,0x30,0x28,0x20,0x19,0x12,0x0C,0x05,0x00,0x00,

	//3: 32 bytes[0-0x0F], total 100ms*6=600ms; Address 0xE0
	0x00,0x4E,0x7B,0x7F,0x7F,0x7B,0x4E,0x00,0x00,0x4E,0x7B,0x7F,0x7F,0x7B,0x4E,0x00,
	0x00,0x4E,0x7B,0x7F,0x7F,0x7B,0x4E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

///InitRAM  
//******************************************************************************
int MAX20353_InitRAM(void)
{
	int ret;
	//	unsigned char nLSx = 0x01;	// bit7-6: 01: not the last sample
	//	unsigned char Dur  = 0x01;	// bit4-0: 00001 = 5ms
	//	unsigned char Wait = 0x00;	// bit4-0: 00000 = 0ms
	//	unsigned char RPTx = 0x00;	// bit3-0: 0000

	int i;
	ret = 0;

	// Enable Pattern RAM
	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x40);//0x310 = RAM disabled.												// 1 = RAM enabled	//hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptrvMode=0x12

	for(i=0; i<256; i++)
	{
		appdatainoutbuffer_[0] = i;
		appdatainoutbuffer_[1] = 0x40|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample); Amp[7:2]
		appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x02;	// Amp[1:0];Dur[4:0]=1(5ms);Wait[4]=0;
		appdatainoutbuffer_[3] = 0x00;					// Wait[3:0]=0;RPTx[3:0]=0;
		ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);//0x28
		k_sleep(K_MSEC(10));
	}

	// Pattern 0 ending
	//0: 128 bytes[0-0x7F], no repeat, total 640ms; soft start/stop/long; Address 0x00
	i = 0x7F;	// Last byte, dur = 5ms, no repeat
	appdatainoutbuffer_[0] = i;
	appdatainoutbuffer_[1] = 0x00|(Pattern[i]>>2);	// nLSx[1:0]=00(last sample);11(Last and RPTx; Amp[7:2]
	appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x02;	// Amp[1:0];Dur[4:0]=0x01(5ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=0;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));

	// Pattern 1 ending
	//1: 64 bytes[0-0x3F], repeat 1 to 2 times, last byte delay 80ms, total (320ms+80ms)*(1 or 2)=400 or 800ms; soft start/stop/short; Address 0x80
	i = 0xBF; // Last byte, dur = 85ms, extra 80ms, repeat 2 times
	appdatainoutbuffer_[0] = i;
	appdatainoutbuffer_[1] = 0xC0|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x02;									// Wait[3:0]=0;RPTx[3:0]=2;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));

	// Pattern 2 ending
	//2: 32 bytes[0-0x1F], repeat 1 to 2 times, last byte delay 40ms, total (160ms+40: soft start/stop/1 time very short; Address 0xC0
	i = 0xDF; // Last byte, dur = 45ms, extra 40ms, no repeat
	appdatainoutbuffer_[0] = i;
	appdatainoutbuffer_[1] = 0x00|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x18;	// Amp[1:0];Dur[4:0]=0x09(45ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=2;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));

	// Pattern 3 ending
	//3: 32 bytes[0-0x0F], total 100ms*6=600ms; Address 0xE0
	i = 0xE4; // delay 85ms
	appdatainoutbuffer_[0] = i;
	appdatainoutbuffer_[1] = 0x40|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=15;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));
	i = 0xE8; // delay 85ms
	appdatainoutbuffer_[0] = i;
	//appdatainoutbuffer_[1] = 0x40|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	//appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=15;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));
	i = 0xEC; // delay 85ms

	appdatainoutbuffer_[0] = i;
	appdatainoutbuffer_[1] = 0x40|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=15;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));
	i = 0xF0; // delay 85ms
	appdatainoutbuffer_[0] = i;
	//appdatainoutbuffer_[1] = 0x40|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	//appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=15;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));
	i = 0xF4; // delay 85ms
	appdatainoutbuffer_[0] = i;
	appdatainoutbuffer_[1] = 0x40|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=15;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));
	i = 0xF8; // delay 85ms, end
	appdatainoutbuffer_[0] = i;
	appdatainoutbuffer_[1] = 0x00|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
	appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
	appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=15;
	ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
	k_sleep(K_MSEC(10));
	k_sleep(K_MSEC(10));	

	return ret;

}

int MAX20353_AppWrite(uint8_t dataoutlen)
{
	static int ret;

	ret  = MAX20353_WriteRegMulti(REG_AP_DATOUT0, appdatainoutbuffer_, dataoutlen); ///0x0F

	ret |= MAX20353_WriteReg(REG_AP_CMDOUT, appcmdoutvalue_); //0x17	

	k_sleep(K_MSEC(10));
	ret |= MAX20353_ReadReg(REG_AP_RESPONSE, &appcmdoutvalue_);//0x18
	printf(" appcmdoutvalue:%0x \r\n",appcmdoutvalue_ );

	if(ret != 0)
		ret = MAX20303_ERROR;
	else
		ret = MAX20303_NO_ERROR;
	
	return ret;
}

int MAX20353_WriteRegMulti(max20353_reg_t reg, uint8_t *value, uint8_t len)
{
	s32_t ret;
	
	ret = platform_write(reg, value, len);
	if(ret != 0)
	{
		ret = MAX20303_ERROR; 
	}
	else
	{ 
		ret = MAX20303_NO_ERROR;
	}
	
	return ret;
}

int MAX20353_WriteReg(max20353_reg_t reg, uint8_t value)
{ 
    s32_t ret;

	ret = platform_write(reg, &value, 1);
	if(ret != 0)
	{
		ret = MAX20303_ERROR;  
	}
	else
	{
		ret = MAX20303_NO_ERROR; 
	}

	return ret;
}

/******************************************************************************
读取一个字节
*/
int MAX20353_ReadReg(max20353_reg_t reg, uint8_t *value)
{
    s32_t ret;

	ret = platform_read(reg, value, 1);
    if(ret != 0)
    {
        ret = MAX20303_ERROR;
    }
    else
    {
        ret = MAX20303_NO_ERROR;
    }
	
    return ret;
}

/******************************************************************************
读取多个字节
*/
int MAX20353_ReadRegMulti(max20353_reg_t reg, uint8_t *value, uint8_t len)
{
    s32_t ret;

 	ret = platform_read(reg, value, len);
    if(ret != 0)
        ret = MAX20303_ERROR;
    else
        ret = MAX20303_NO_ERROR;
	
    return ret;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
int MAX20353_RAMPatStart(unsigned char RAMPatAddr)
{
	int ret = 0;

	ret |= MAX20353_WriteReg( REG_HPT_PATRAMADDR,  RAMPatAddr);// 0x31 Pattern RAM Address
	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0xF2);	//0x31  hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x12
	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x72);	//0x33 hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x12
	if (ret != 0)
		return MAX20303_ERROR;

	return MAX20303_NO_ERROR;
}

int MAX20353_Pattern1(int Mode)
{
	int i, ret = 0;

	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x40);	// 0x31, hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptrvMode=0x12

	// Pattern 1 ending
	if(Mode == 0)	// one times
	{
		i = 0xBF; // Last byte, dur = 85ms
		appdatainoutbuffer_[0] = i;
		appdatainoutbuffer_[1] = 0x00|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
		appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
		appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=2;
		ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
		k_sleep(K_MSEC(10));
	}
	else if(Mode == 1)	// two times
	{
		i = 0xBF; // Last byte, dur = 85ms, extra 80ms, repeat 2 times
		appdatainoutbuffer_[0] = i;
		appdatainoutbuffer_[1] = 0xC0|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
		appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x22;	// Amp[1:0];Dur[4:0]=0x11(85ms);Wait[4]=0;
		appdatainoutbuffer_[3] = 0x02;									// Wait[3:0]=0;RPTx[3:0]=2;
		ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
		k_sleep(K_MSEC(10));	
	}
	
	return ret;     
}


 int MAX20353_Pattern2(int Mode)
{
	int i, ret = 0;

	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x40);//hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptrvMode=0x12 0x40

	// Pattern 1 ending
	if(Mode == 0)	// one times
	{
		i = 0xDF; // Last byte, dur = 45ms, extra 40ms, no repeat
		appdatainoutbuffer_[0] = i;
		appdatainoutbuffer_[1] = 0x00|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
		appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x18;	// Amp[1:0];Dur[4:0]=0x09(45ms);Wait[4]=0;
		appdatainoutbuffer_[3] = 0x00;									// Wait[3:0]=0;RPTx[3:0]=2;
		ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
		k_sleep(K_MSEC(10));	
	}
	else if(Mode == 1)	// two times
	{
		i = 0xDF; // Last byte, dur = 45ms, extra 40ms, no repeat
		appdatainoutbuffer_[0] = i;
		appdatainoutbuffer_[1] = 0xC0|(Pattern[i]>>2);	// nLSx[1:0]=01(not last sample);11(Last and RPTx; Amp[7:2]
		appdatainoutbuffer_[2] = (Pattern[i]<<6)|0x18;	// Amp[1:0];Dur[4:0]=0x09(45ms);Wait[4]=0;
		appdatainoutbuffer_[3] = 0x02;									// Wait[3:0]=0;RPTx[3:0]=2;
		ret |= MAX20353_WriteRegMulti(REG_HPT_RAMADDR, appdatainoutbuffer_, 4);
		k_sleep(K_MSEC(10));	
	}		
    return ret;
}

int MAX20353_DirectWrite(int Amplitude, int time)
{
	int ret = 0;

	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x00);	//hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x06

	ret |= MAX20353_WriteReg( REG_HPT_RTI2CAMP,  Amplitude);
	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x26);	//hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x06
	k_sleep(K_MSEC(time));

	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x00);	//hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x12
	return ret;
}

/****************************************************************************
使用读取多个字节 和 写入多个字节 的函数进行测试
i2c_reg_write_byte(max20353_I2C,MAX20303_SLAVE_ADDR,reg,value);
*/
void test_multi_read_write(void)
{
    u8_t buf[10];
	
    memset(buf,0,sizeof(buf));
    buf[0]=0x01;
    buf[1]=0x02;
    buf[2]=0x03;
    buf[3]=0x03;
    buf[4]=0x05;

	platform_write(0x10, buf, 6);
	platform_read(0x10, buf, 5);

    printf("  buf buf0:%0x \r\n",buf[0] );
    printf("  buf buf1:%0x \r\n",buf[1] );
    printf("  buf buf2:%0x \r\n",buf[2] );
    printf("  buf buf3:%0x \r\n",buf[3] );
    printf("  buf buf4:%0x \r\n",buf[4] );

}

void MAX20353_Init(void)
{
	MAX20353_Buck1Config();
	MAX20353_Buck2Config();

	MAX20353_LDO1Config();
	MAX20353_LDO2Config();

	MAX20353_BoostConfig(); 

	MAX20353_ChargePumpConfig();
	MAX20353_BuckBoostConfig();

	//MAX20353_LED1(2, 0);
	Set_Screen_Backlight_On();
	//Set_Screen_Backlight_Off();
}

void pmu_interrupt_proc(void)
{
	printk("pmu_interrupt_proc\n");
}

void PmuInterruptHandle(void)
{
	pmu_trige_flag = true;
}

void pmu_init(void)
{
	bool rst;
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_LOW|GPIO_INT_DEBOUNCE;

  	//端口初始化
  	gpio_pmu = device_get_binding(PMU_PORT);
	if(!gpio_pmu)
	{
		printk("Cannot bind gpio device\n");
		return;
	}

	gpio_pin_configure(gpio_pmu, PMU_EINT, flag);
	gpio_pin_disable_callback(gpio_pmu, PMU_EINT);
	gpio_init_callback(&gpio_cb, PmuInterruptHandle, BIT(PMU_EINT));
	gpio_add_callback(gpio_pmu, &gpio_cb);
	gpio_pin_enable_callback(gpio_pmu, PMU_EINT);
	
	rst = init_i2c();
	if(!rst)
	{
		return;
	}

	platform_read(REG_HARDWARE_ID, &HardwareID, 1);
	if(HardwareID != MAX20353_HARDWARE_ID)
	{
		return;
	}
	
	platform_read(REG_FIRMWARE_REV, &FirmwareID, 1);
	if(FirmwareID != MAX20353_FIRMWARE_ID)
	{
		return;
	}
	
	MAX20353_Init();
	
#if 0 
    //MAX20353_HapticConfig();
    MAX20353_InitRAM();
#endif

    while(1)
    {  
		k_sleep(K_MSEC(1000)); 
		//printf("  counter_time:%d\r\n", counter_time++);
       
	#if 0
		k_sleep(K_MSEC(1000)); 
        
        i2c_burst_read(max20353_I2C,MAX20303_SLAVE_ADDR,0x00,buf,2);
        printf("HardwareID:%0x FirmwareID:%0x  counter_time:%d\r\n", buf[0], buf[1],counter_time++);
	#endif

       
	#if 0
		MAX20353_RAMPatStart(0x00);k_sleep(K_MSEC(1000));
		MAX20353_Pattern1(0);
		MAX20353_RAMPatStart(0x80);k_sleep(K_MSEC(1000));	

		MAX20353_Pattern1(1);
		MAX20353_RAMPatStart(0x80);k_sleep(K_MSEC(3000));	

		MAX20353_Pattern2(0);
		MAX20353_RAMPatStart(0xC0);k_sleep(K_MSEC(3000));	

		MAX20353_Pattern2(1);
		MAX20353_RAMPatStart(0xC0);k_sleep(K_MSEC(3000));         

		MAX20353_RAMPatStart(0xE0);k_sleep(K_MSEC(1000));        

		MAX20353_DirectWrite(0x7F, 1000);k_sleep(K_MSEC(1000));  

		MAX20353_DirectWrite(0xFF, 1000);k_sleep(K_MSEC(5000)); 
	#endif
    }	
}

void test_pmu(void)
{
    pmu_init();
}

