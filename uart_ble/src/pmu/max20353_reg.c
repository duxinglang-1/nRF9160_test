/**************************************************************************
 *Name        : Max20353_reg.c
 *Author      : xie biao
 *Version     : V1.0
 *Create      : 2020-09-14
 *Copyright   : August
**************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/i2c.h>
#include "max20353_reg.h"
#include "max20353.h"
#include "NTC_table.h"
#include "lcd.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(max20353_reg, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef BATTERY_SOC_GAUGE
#define VERIFY_AND_FIX 1
#define LOAD_MODEL !(VERIFY_AND_FIX)
#define EMPTY_ADJUSTMENT		0
#define FULL_ADJUSTMENT			100
#define RCOMP0					22
#define TEMP_COUP				(-0.71875)
#define TEMP_CODOWN				(-3.875)
#define TEMP_CODOWNN10			(-3.90625)
#define OCVTEST					57680
#define SOCCHECKA				114
#define SOCCHECKB				116
#define BITS					18
#define RCOMPSEG				0x0400
#define INI_OCVTEST_HIGH_BYTE 	(OCVTEST>>8)
#define INI_OCVTEST_LOW_BYTE	(OCVTEST&0x00ff)

static u8_t SOC_1, SOC_2;
static u8_t original_OCV_1, original_OCV_2;
static u16_t SOC;
static u8_t SOC_percent;

static struct k_timer ntc_check_timer;

void prepare_to_load_model(void);
void load_model(void);
bool verify_model_correct(void);
void cleanup_model_load(void);
int ReadWord(u8_t reg, u8_t *MSB, u8_t *LSB);
int WriteWord(u8_t reg, u8_t MSB, u8_t LSB);
int WriteMulti(u8_t *data, u8_t len);
#endif

static u8_t HardwareID;
static u8_t FirmwareID;

static u8_t PMICIntMasks[3];

u8_t i2cbuffer_[16];
u8_t appdatainoutbuffer_[8];
u8_t appcmdoutvalue_;

static unsigned char Pattern[256] = 
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

extern maxdev_ctx_t pmu_dev_ctx;

void delay_ms(uint32_t period)
{
    k_sleep(K_MSEC(period));
}

#ifdef MOTOR_TYPE_ERM
int MAX20303_HapticConfigDCMotor(void)
{
	int32_t i,j,ret=0;

	//Config 0
	appcmdoutvalue_ = 0xA0;
	appdatainoutbuffer_[0] = 0x0A; //0x0F; // EmfEn(resonance detection)/HptSel(ERM)/ALC/ZeroCrossHysteresis
	appdatainoutbuffer_[1] = 0xDA; //0x9F; // Initial guess of Back EMF frequency = 25.6M/64/IniGss =235/205; IniGss = 0x6A6/0x79F
	appdatainoutbuffer_[2] = 0x16; //0x87; // ZccSlowEn=0/FltrCntrEn=0
	appdatainoutbuffer_[3] = 0x00; //0x00; // Skip periods before BEMF measuring
	appdatainoutbuffer_[4] = 0x07; //0x05; // Wide Window for BEMF zero crossing
	appdatainoutbuffer_[5] = 0x02; //0x01; // Narrow Window for BEMF zero crossing
	ret |= MAX20353_AppWrite(6);

	//Config 1
	appcmdoutvalue_ = 0xA2;
	appdatainoutbuffer_[0] = 0xF0; //0x01; // EmfSkipCyc
	appdatainoutbuffer_[1] = 0x88; //0x00; // BlankWdw, zero corssing comparator blanking time after enable(1/25.6MHz)
	appdatainoutbuffer_[2] = 0x00; //0x02; // BlankWdw, zero corssing comparator blanking time after enable(1/25.6MHz)
	appdatainoutbuffer_[3] = 0xFF; //0x5D; // Vpp_Sine_max = 5.65V, Vpp_Square_max = 4V, VFS= Vpp/2 = 2V, Max_VFS = 5.5V, set VFS = 2/5.5*255 = 92 = 0x5C(square), Sine: VFS=2.8/5.5*255=0x82
	appdatainoutbuffer_[4] = 0xE6; //0xE6; // ETRGOdAmp, Overdrive amplitude, LSB = 0.78%VFS, 98%
	appdatainoutbuffer_[5] = 0x10; //0x10; // ETRGOdDur, Overdrive period, LSB = 5ms, 80ms
	ret |= MAX20353_AppWrite(6);

	//Config 2
	appcmdoutvalue_ = 0xA4;
	appdatainoutbuffer_[0] = 0xC0; //0xC0; // ETRGActAmp, normal amplitude, , LSB = 0.78%VFS, don't care
	appdatainoutbuffer_[1] = 0xFF; //0xFF; // ETRGActDur, normal period, LSB = 10ms, don't care
	appdatainoutbuffer_[2] = 0xCD; //0xCD; // ETRGActAmp, braking amplitude, , LSB = 0.78%VFS, 80*VFS
	appdatainoutbuffer_[3] = 0x14; //0x14; // ETRGActDur, breaking period, LSB = 5ms, 60ms
	appdatainoutbuffer_[4] = 0x23; //0x15; // narrow window gain, wide window gain = 1
	appdatainoutbuffer_[5] = 0x06; //0x00; // periods from wide to narrow
	ret |= MAX20353_AppWrite(6);

	//==============================================
	// Sys UVLO threshold = 1.6V
	appcmdoutvalue_ = 0xA6;
	appdatainoutbuffer_[0] = 0x4A; //Sys UVLO threshold = 1.6V
	ret |= MAX20353_AppWrite(1);
	if(ret != 0)
		return MAX20353_ERROR;

	ret |= MAX20353_WriteReg( REG_HPT_DIRECT1,  0x00); //hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x12, disable HptDrvEn
	if(ret != 0)
		return MAX20353_ERROR;
	
	if(ret != 0)
		return MAX20353_ERROR;

	return MAX20353_NO_ERROR;
}
#endif

#ifdef MOTOR_TYPE_LRA
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

#if 0
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
#endif
	if(ret != 0)
		return MAX20353_ERROR;

	return MAX20353_NO_ERROR;
}
#endif

void VibrateStart(void)
{
#ifdef MOTOR_TYPE_ERM
	MAX20353_WriteReg( REG_HPT_RTI2CAMP,  0x3F);	//振幅控制，从0x00到0x7f
	MAX20353_WriteReg( REG_HPT_DIRECT1,  0x26); //hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x06
#else defined(MOTOR_TYPE_LRA)
	appcmdoutvalue_ = 0xAC;
	MAX20353_WriteReg( REG_AP_CMDOUT,  appcmdoutvalue_);//0x17	//Pattern RAM Address
#endif
}

void VibrateStop(void)
{
#ifdef MOTOR_TYPE_ERM
	MAX20353_WriteReg( REG_HPT_DIRECT1,  0x00); //hptExtTrig=1, HptRamEn=1, HptDrvEn=1, HptDrvMode=0x12
#else defined(MOTOR_TYPE_LRA)
	MAX20353_WriteReg( REG_HPT_DIRECT1,  0x00);
#endif
}

int MAX20353_Buck1Config(void) 
{
    int32_t ret = 0;
	
    appcmdoutvalue_ = 0x35;
    appdatainoutbuffer_[0] = 0x00;  	//
    appdatainoutbuffer_[1] = 0x2C;  	//0x28    0.7+(0.025V * number)    0x48*0.025 =1.8v     //0.7V to 2.275V, Linear Scale, 25mV increments
    appdatainoutbuffer_[2] = 0x1F;  	//0x2F  	01 = 20mA, Use for 1V < Buck1VSet < 1.8V
    appdatainoutbuffer_[3] = 0x01;  	// Enable
    ret = MAX20353_AppWrite(4);
	
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
    appdatainoutbuffer_[0] = 0x05;     //0x01  0.5V to 1.95V, Linear Scale, 25mV increments,使能   LDO1  
    appdatainoutbuffer_[1] = 0x34;     //0x28  0.5V + (0.025V * number)   =  1.95V   1.8
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
    appdatainoutbuffer_[1] = 0x03;	// 00 : 6.5V, 01: 5V
    ret = MAX20353_AppWrite(2);

    return ret;
}

/// @brief BuckBoost to 5.0V output rail **/
//******************************************************************************
int MAX20353_BuckBoostDisable(void) 
{
    int32_t ret = 0;
    appcmdoutvalue_ = 0x70;
    appdatainoutbuffer_[0] = 0x00;
    appdatainoutbuffer_[1] = 0x00;
    appdatainoutbuffer_[2] = 0x00;
    appdatainoutbuffer_[3] = 0x00;     
    ret = MAX20353_AppWrite(4);

    return ret;
}

/// @brief BuckBoost to 5.0V output rail **/
//******************************************************************************
int MAX20353_BuckBoostConfig(void) 
{
    int32_t ret = 0;
    appcmdoutvalue_ = 0x70;
    appdatainoutbuffer_[0] = 0x00;
    appdatainoutbuffer_[1] = 0x04;
    appdatainoutbuffer_[2] = 0x19;		// 2.5V + (0.1V * number) = 5.0V
    appdatainoutbuffer_[3] = 0x41;     
    ret = MAX20353_AppWrite(4);

    return ret;
}

int MAX20353_PowerOffConfig(void) 
{
    int32_t ret = 0;
	
    appcmdoutvalue_ = 0x80;
    appdatainoutbuffer_[0] = 0xB2;
    ret = MAX20353_AppWrite(1);

    return ret;
}

int MAX20353_SoftResetConfig(void) 
{
    int32_t ret = 0;
	
    appcmdoutvalue_ = 0x81;
    appdatainoutbuffer_[0] = 0xB3;
    ret = MAX20353_AppWrite(1);

    return ret;
}

int MAX20353_HardResetConfig(void) 
{
    int32_t ret = 0;
	
    appcmdoutvalue_ = 0x82;
    appdatainoutbuffer_[0] = 0xB4;
    ret = MAX20353_AppWrite(1);

    return ret;
}

/**************************************************
*Name:	MAX20353_LED0
*Function: LED0 控制
*Parameter:
*			flag:true-on,false-off
*			IStep:0-2
*			Amplitude:0-31
*return: 
***************************************************/
int MAX20353_LED0(int IStep, int Amplitude, bool flag)
{ 
	int ret = 0;

	//Bit7: LED2Open, Bit6: LED1Open, Bit5: LED0Open, Bit1-0: LEDIStep: 0:0.6mA, 1:1.0mA, 2:1.2mA, 3:Reserved 
	ret |= MAX20353_WriteReg(REG_LED_STEP_DIRECT,  IStep&0x03);

	//Bit7-5: 1:LED1On, Bit4-0: Amplitude, LED current = IStep*Amplitude 
	if(flag)
		ret |= MAX20353_WriteReg(REG_LED0_DIRECT,  0x20|(Amplitude&0x1F)); 
	else
		ret |= MAX20353_WriteReg(REG_LED0_DIRECT,  0x00); 
}

/**************************************************
*Name:	MAX20353_LED1
*Function: LED1 控制
*Parameter:
*			flag:true-on,false-off
*			IStep:0-2
*			Amplitude:0-31
*return: 
***************************************************/
int MAX20353_LED1(int IStep, int Amplitude, bool flag)
{ 
	int ret = 0;

	//Bit7: LED2Open, Bit6: LED1Open, Bit5: LED0Open, Bit1-0: LEDIStep: 0:0.6mA, 1:1.0mA, 2:1.2mA, 3:Reserved 
	ret |= MAX20353_WriteReg(REG_LED_STEP_DIRECT,  IStep&0x03);

	//Bit7-5: 1:LED1On, Bit4-0: Amplitude, LED current = IStep*Amplitude 
	if(flag)
		ret |= MAX20353_WriteReg(REG_LED1_DIRECT,  0x20|(Amplitude&0x1F)); 
	else
		ret |= MAX20353_WriteReg(REG_LED1_DIRECT,  0x00); 
}

/**************************************************
*Name:	MAX20353_LED2
*Function: LED2 控制
*Parameter:
*			flag:true-on,false-off
*			IStep:0-2
*			Amplitude:0-31
*return: 
***************************************************/
int MAX20353_LED2(int IStep, int Amplitude, bool flag)
{ 
	int ret = 0;

	//Bit7: LED2Open, Bit6: LED1Open, Bit5: LED0Open, Bit1-0: LEDIStep: 0:0.6mA, 1:1.0mA, 2:1.2mA, 3:Reserved 
	ret |= MAX20353_WriteReg(REG_LED_STEP_DIRECT,  IStep&0x03);

	//Bit7-5: 1:LED1On, Bit4-0: Amplitude, LED current = IStep*Amplitude 
	if(flag)
		ret |= MAX20353_WriteReg(REG_LED2_DIRECT,  0x20|(Amplitude&0x1F)); 
	else
		ret |= MAX20353_WriteReg(REG_LED2_DIRECT,  0x00); 
}

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
	
	return ret;
}

int MAX20353_AppWrite(uint8_t dataoutlen)
{
	static int ret;

	ret  = MAX20353_WriteRegMulti(REG_AP_DATOUT0, appdatainoutbuffer_, dataoutlen); ///0x0F

	ret |= MAX20353_WriteReg(REG_AP_CMDOUT, appcmdoutvalue_); //0x17	

	k_sleep(K_MSEC(10));
	ret |= MAX20353_ReadReg(REG_AP_RESPONSE, &appcmdoutvalue_);//0x18
	if(ret != 0)
		ret = MAX20353_ERROR;
	else
		ret = MAX20353_NO_ERROR;
	
	return ret;
}

int MAX20353_AppRead(uint8_t datainlen)
{
	int ret;

	ret = MAX20353_WriteReg(REG_AP_CMDOUT, appcmdoutvalue_);

	k_sleep(K_MSEC(10));
	
	ret |= MAX20353_ReadRegMulti(REG_AP_RESPONSE, i2cbuffer_, datainlen);
	if(ret != 0)
		return MAX20353_ERROR;

	return MAX20353_NO_ERROR;
}

int MAX20353_WriteRegMulti(max20353_reg_t reg, uint8_t *value, uint8_t len)
{
	s32_t ret;

	ret = pmu_dev_ctx.write_reg(pmu_dev_ctx.handle, reg, value, len);
	if(ret != 0)
	{
		ret = MAX20353_ERROR; 
	}
	else
	{ 
		ret = MAX20353_NO_ERROR;
	}
	
	return ret;
}

int MAX20353_WriteReg(max20353_reg_t reg, uint8_t value)
{ 
    s32_t ret;

	ret = pmu_dev_ctx.write_reg(pmu_dev_ctx.handle, reg, &value, 1);
	if(ret != 0)
	{
		ret = MAX20353_ERROR;  
	}
	else
	{
		ret = MAX20353_NO_ERROR; 
	}

	return ret;
}

/******************************************************************************
读取一个字节
*/
int MAX20353_ReadReg(max20353_reg_t reg, uint8_t *value)
{
    s32_t ret;

	ret = pmu_dev_ctx.read_reg(pmu_dev_ctx.handle, reg, value, 1);
    if(ret != 0)
    {
        ret = MAX20353_ERROR;
    }
    else
    {
        ret = MAX20353_NO_ERROR;
    }
	
    return ret;
}

/******************************************************************************
读取多个字节
*/
int MAX20353_ReadRegMulti(max20353_reg_t reg, uint8_t *value, uint8_t len)
{
    s32_t ret;

	ret = pmu_dev_ctx.read_reg(pmu_dev_ctx.handle, reg, value, len);
    if(ret != 0)
        ret = MAX20353_ERROR;
    else
        ret = MAX20353_NO_ERROR;
	
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
		return MAX20353_ERROR;

	return MAX20353_NO_ERROR;
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

// Read status0-3
int MAX20353_CheckPMICStatus(unsigned char buf_results[4])
{
	int ret;

	ret  = MAX20353_ReadReg(REG_STATUS0, &buf_results[0]);
	ret |= MAX20353_ReadReg(REG_STATUS1, &buf_results[1]);
	ret |= MAX20353_ReadReg(REG_STATUS2, &buf_results[2]);
	ret |= MAX20353_ReadReg(REG_STATUS3, &buf_results[3]);
	return ret;
}

// Read Int0-2
int MAX20353_CheckPMICIntRegisters(unsigned char buf_results[3])
{
	int ret;
	ret  = MAX20353_ReadReg(REG_INT0, &buf_results[0]);
	ret |= MAX20353_ReadReg(REG_INT1, &buf_results[1]);
	ret |= MAX20353_ReadReg(REG_INT2, &buf_results[2]);
	return ret;
}

// Write Int Mask0-2
int MAX20353_EnablePMICIntMaskRegisters(unsigned char buf_results[3])
{
	int ret;
	ret  = MAX20353_WriteReg(REG_INT_MASK0, buf_results[0]);
	ret |= MAX20353_WriteReg(REG_INT_MASK1, buf_results[1]);
	ret |= MAX20353_WriteReg(REG_INT_MASK2, buf_results[2]);
	return ret;
}

//充电配置    
int MAX20353_ChargerCfg(void)
{
	int32_t ret = 0;

	appcmdoutvalue_ = 0x14; 
	appdatainoutbuffer_[0] = 0x04; // Maintain charge b00:0min, FastCharge b00:150min, for 1C charging, PreCharge b00: 30min for dead battery 
	appdatainoutbuffer_[1] = 0x61; // Precharge to b110:3.0V, b00:0.05IFChg for dead battery, ChgDone b01: 0.1IFChg 
	appdatainoutbuffer_[2] = 0xD6; // Auto Stop, Auto ReStart, ReChg Threshold b01:120mV, Bat Volt b0011: 4.2V, b0110:4.35 
	appdatainoutbuffer_[3] = 0x07; // System min volt = 4.3V 
	ret |= MAX20353_AppWrite(4);

	return ret;
}

//充电使能
int MAX20353_ChargerCtrl(void)
{
	int32_t ret = 0;

	appcmdoutvalue_ = 0x1A; 
	appdatainoutbuffer_[0] = 0x01; // Thermal EN, Charger EN 
	ret |= MAX20353_AppWrite(1); 

	return ret;
}

int MAX20353_InputCurCfg(void)
{
	int32_t ret = 0;

	appcmdoutvalue_ = 0x10;
	appdatainoutbuffer_[0] = 0x1E;  //最大输入电流500ma(充电电流+系统工作电流,不是充电电流，充电电流由F3的电阻决定，目前设置为160ma),截断时间10ms
	ret = MAX20353_AppWrite(1);

	return ret;
}

void MAX20353_ChargerInit(void)
{
	//charger config
	MAX20353_ChargerCfg();
	MAX20353_ChargerCtrl();
	MAX20353_InputCurCfg();

	//Enable Int Mask 
	//Enable Charger Status Int and USB OK Int 
	PMICIntMasks[0] = 0x48;
	PMICIntMasks[1] = 0x00;
	PMICIntMasks[2] = 0x00;
	MAX20353_EnablePMICIntMaskRegisters(PMICIntMasks);
}

int InitCharger(void)
{ 
	int32_t ret = 0; 

	appcmdoutvalue_ = 0x14; 
	appdatainoutbuffer_[0] = 0x04; // Maintain charge b00:0min, FastCharge b00:150min, for 1C charging, PreCharge b00: 30min for dead battery 
	appdatainoutbuffer_[1] = 0x61; // Precharge to b110:3.0V, b00:0.05IFChg for dead battery, ChgDone b01: 0.1IFChg 
	appdatainoutbuffer_[2] = 0xD6; // Auto Stop, Auto ReStart, ReChg Threshold b01:120mV, Bat Volt b0011: 4.2V, b0110:4.35 
	appdatainoutbuffer_[3] = 0x07; // System min volt = 4.3V 
	ret |= MAX20353_AppWrite(4);
	
	appcmdoutvalue_ = 0x1A; 
	appdatainoutbuffer_[0] = 0x01; // Thermal EN, Charger EN 
	ret |= MAX20353_AppWrite(1); 
	return ret;
}

//马达的震动     
int MAX20353_MotorCtrl(u32_t time_num, u32_t count)
{
	s32_t ret = 0;

	//Config 0
	appcmdoutvalue_ = 0xA0;
	appdatainoutbuffer_[0] = 0x0A;	//0x0F;	0x0e // EmfEn(resonance detection)/HptSel(LRA)/ALC/ZeroCrossHysteresis
	appdatainoutbuffer_[1] = 0xDA;	//0x9F;	// Initial guess of Back EMF frequency = 25.6M/64/IniGss =235/205; IniGss = 0x6A6/0x79F
	appdatainoutbuffer_[2] = 0x16;	//0x87;	// ZccSlowEn=0/FltrCntrEn=0
	appdatainoutbuffer_[3] = 0x00;	//0x00;	// Skip periods before BEMF measuring
	appdatainoutbuffer_[4] = 0x07;	//0x05;	// Wide Window for BEMF zero crossing
	appdatainoutbuffer_[5] = 0x02;	//0x01;	// Narrow Window for BEMF zero crossing
	ret |= MAX20353_AppWrite(6);
 
	do
	{
		appcmdoutvalue_ = 0xAC;
		ret |= MAX20353_WriteReg( REG_AP_CMDOUT,  appcmdoutvalue_);//0x17	//Pattern RAM Address

		k_sleep(K_MSEC(time_num));
		MAX20353_ReadRegMulti(REG_AP_RESPONSE, appdatainoutbuffer_, 4);//0x18
		count--;          
	}while((appdatainoutbuffer_[1]!=0x00)&&(count>0));	//freg = 400,000/(APDataIn2 | (APDataIn3<<8))
 
	if(ret != 0)
		return MAX20353_ERROR;

	return MAX20353_NO_ERROR;
}

void MAX20353_GetDeviceID(u8_t *Device_ID)
{
	MAX20353_ReadReg(REG_HARDWARE_ID, Device_ID);
}

bool MAX20353_Init(void)
{
	MAX20353_GetDeviceID(&HardwareID);
	if(HardwareID != MAX20353_HARDWARE_ID)
		return false;

	//MAX20353_SoftResetConfig();
	
	//供电电压及电流配置
	MAX20353_Buck1Config();		//1.8v  350mA
	MAX20353_Buck2Config(); 	//3.3V  350mA
	MAX20353_LDO1Config();	//1.8v 50mA switch mode
	MAX20353_LDO2Config();	//2.8V 100mA
	MAX20353_BoostConfig(); //5V 只有buck2的3.3V关闭，即PPG才会亮

	//电荷泵及BUCK/BOOST配置
	MAX20353_ChargePumpConfig();

	//MAX20353_BuckBoostDisable();
	MAX20353_BuckBoostConfig();

	//马达驱动
#ifdef MOTOR_TYPE_ERM
	MAX20303_HapticConfigDCMotor();
#elif defined(MOTOR_TYPE_LRA)
	MAX20353_HapticConfig();	//线性马达驱动参数进行初始化  
	//MAX20353_InitRAM();  		//马达振动模式
#endif

	//电量计
#ifdef BATTERY_SOC_GAUGE
	MAX20353_SOCInit();
#endif

	//充电配置
	MAX20353_ChargerInit();

	return true;
}

#ifdef BATTERY_SOC_GAUGE	//xb add 2020-11-05 增加有关电量计的代码
//******************************************************************************
int ReadWord(u8_t reg, u8_t *MSB, u8_t *LSB)
{
	u32_t ret;
	u8_t data = reg;
	u8_t value[2];

	ret = i2c_write(pmu_dev_ctx.handle, &data, sizeof(data), MAX20353_I2C_ADDR_FUEL_GAUGE);
	if(ret != 0)
	{
		return MAX20353_ERROR;
	}

	ret = i2c_read(pmu_dev_ctx.handle, value, sizeof(value), MAX20353_I2C_ADDR_FUEL_GAUGE);
	if (ret != 0)
	{
		return MAX20353_ERROR;
	}
	
	*MSB = value[0];
	*LSB = value[1];

	return MAX20353_NO_ERROR;
}

//******************************************************************************
int WriteWord(u8_t reg, u8_t MSB, u8_t LSB)
{
	u8_t cmdData[3] = {reg, MSB, LSB};
	u32_t rslt = 0;

	rslt = i2c_write(pmu_dev_ctx.handle, cmdData, sizeof(cmdData), MAX20353_I2C_ADDR_FUEL_GAUGE);
	if (rslt != 0)
		return MAX20353_ERROR;
	return MAX20353_NO_ERROR;
}

int WriteMulti(u8_t *data, u8_t len)
{
	u32_t ret;

	ret = i2c_write(pmu_dev_ctx.handle, data, len, MAX20353_I2C_ADDR_FUEL_GAUGE);
	if (ret != 0)
		return MAX20353_ERROR;
	return MAX20353_NO_ERROR;
}

#ifdef BATTERT_NTC_CHECK
u8_t MAX20353_ReadTHM(void)
{
	int ret;

	// Read THM
	appcmdoutvalue_ = 0x53;
	appdatainoutbuffer_[0] = 0x22; ////4 average, THM
	ret |= MAX20353_AppWrite(1);

	appcmdoutvalue_ = 0x53;
	ret |= MAX20353_AppRead(5);
	//LOG_INF("%02X, %02X, %02X, %02X, %02X\n", i2cbuffer_[0], i2cbuffer_[1], i2cbuffer_[2], i2cbuffer_[3], i2cbuffer_[4]);

	return i2cbuffer_[4];
}

int MAX20353_UpdateRCOMP(int temp)
{
	int RCOMP; 
	// RCOMP value at 20 degrees C 
	int INI_RCOMP = RCOMP0; 
	// RCOMP change per degree for every degree above 20 degrees C 
	float TempCoUp = TEMP_COUP; 
	// RCOMP change per degree for every degree below 20 degrees C 
	float TempCoDown = TEMP_CODOWN; 
	// RCOMP change per degree for every degree below 0 degrees C 
	float TempCoDownN10 = TEMP_CODOWNN10; 
	//float temp = 25; // battery temperature degrees C 
	//float used_tempco = temp> 20 ? TempCoUp : TempCoDown; 
	//int result = INI_RCOMP + (temp - 20) * used_tempco;  

	float used_tempco; 
	int result; 

	if(temp>20) // (20, ...) 
	{
		used_tempco = TempCoUp;
		result = INI_RCOMP + (temp - 20) * used_tempco;
	}
	else if(temp>0) // {0, 20)
	{
		used_tempco = TempCoDown;
		result = INI_RCOMP + (temp - 20) * used_tempco;
	}
	else //(-10, 0)
	{
		// calculate result to 0 degree
		used_tempco = TempCoDown;
		int result_0 = INI_RCOMP + (0 - 20) * used_tempco;
		// calculate result < 0 degree
		used_tempco = TempCoDownN10;
		result = result_0 + (temp - 0) * used_tempco;
	}

	RCOMP = (result >= 0xff ? 0xff : (result <= 0 ?  0 : result));
	//Set RCOMP, SOC 1% change alert, Empty threshold 4%
	WriteWord(0x0C, RCOMP, 0x5C);
	return RCOMP;
}

void MAX20353_UpdateTemper(void)
{
	u8_t thm;
	s8_t begin,end,tmp;
	float resistance;
	s16_t temper=25;
	u8_t tmpbuf[128] = {0};
	static u8_t pre_thm=0;
	static u16_t pre_temper=25;	//初始化默认25度
	
	thm = MAX20353_ReadTHM();
	if(thm == pre_thm)
		return;

	pre_thm = thm;
	resistance = (float)10/(255.00/thm-1);

	sprintf(tmpbuf, "resistance:%.4f", resistance);
	//LOG_INF("%s\n", tmpbuf);

	begin = 0;
	end = TEMPER_NUM_MAX-1;
	while(begin <= end)
	{
		tmp = (begin+end)/2;
		
		if(ntc_table[tmp].impedance == resistance)
			break;

		if(ntc_table[tmp].impedance > resistance)
		{
			begin = tmp+1;
		}
		else
		{
			end = tmp-1;
		}
	}

	if(begin <= end)	//find success!
	{
		//LOG_INF("find success!\n");
		
		temper = ntc_table[tmp].temperature;
	}
	else			//select closeet
	{
		float com1,com2;

		//LOG_INF("select closeet!\n");
		
		if(begin == tmp+1)
		{
			com1 = fabs(ntc_table[tmp].impedance-resistance);
			com2 = fabs(ntc_table[tmp+1].impedance-resistance);
			sprintf(tmpbuf, "001 com1:%.4f, com2:%04f", com1, com2);
			//LOG_INF("%s\n", tmpbuf);
			if(com1 > com2)
			{
				temper = ntc_table[tmp+1].temperature;
			}
			else
			{
				temper = ntc_table[tmp].temperature;
			}
		}
		else if(end == tmp-1)
		{
			com1 = fabs(ntc_table[tmp].impedance-resistance);
			com2 = fabs(ntc_table[tmp-1].impedance-resistance);
			sprintf(tmpbuf, "002 com1:%.4f, com2:%04f", com1, com2);
			//LOG_INF("%s\n", tmpbuf);
			if(com1 > com2)
			{
				temper = ntc_table[tmp-1].temperature;
			}
			else
			{
				temper = ntc_table[tmp].temperature;
			}			
		}
	}

	//LOG_INF("temper:%d\n", temper);

	if(temper != pre_temper)
	{
		pre_temper = temper;
		MAX20353_UpdateRCOMP(temper);	
	}
}

void MAX20353_CheckTemper(void)
{
	pmu_check_temp_flag = true;
}

void MAX20353_StartCheckTemper(void)
{
	k_timer_init(&ntc_check_timer, MAX20353_CheckTemper, NULL);
	k_timer_start(&ntc_check_timer, K_MSEC(30*1000), K_MSEC(60*1000));
}

#endif/*BATTERT_NTC_CHECK*/

u8_t MAX20353_CalculateSOC(void)
{
	u16_t tmp;
	
	ReadWord(0x04, &SOC_1, &SOC_2);
	SOC = (SOC_1 << 8) + SOC_2;
	
	if(BITS == 18)
		SOC_percent = SOC/256;
	else
		SOC_percent = SOC/512;

	//SOC_percent = 26.5;
	
	return SOC_percent;
}

void MAX20353_QuickStart(void)
{
	// Use with caution!
	// Execute only when the battery is relaxed, meaning charge==0.000mA and
	// discharge==0.000mA for >30 minutes
	WriteWord(0x06, 0x40, 0x00);
}

void MAX20353_NewInitVoltage(void)
{
	u8_t Volt_MSB, Volt_LSB;
	u16_t VCell1, VCell2, OCV,Desired_OCV;
	
	/******************************************************************************
	Step 1. Read First VCELL Sample
	*/
	ReadWord(0x02, &Volt_MSB, &Volt_LSB);
	VCell1 = ((Volt_MSB << 8) + Volt_LSB);
	
	/******************************************************************************
	Step 2. Delay 500ms
	Delay at least 500ms to ensure a new reading in the VCELL register.
	*/
	delay_ms(500);
	
	/******************************************************************************
	Step 3. Read Second VCELL Sample
	*/
	ReadWord(0x02, &Volt_MSB, &Volt_LSB);
	VCell2 = ((Volt_MSB << 8) + Volt_LSB);
	
	/******************************************************************************
	Step 4. Unlock Model Access
	To unlock access to the model the host software must write 0x4Ah to memory
	location 0x3E and write 0x57 to memory location 0x3F.
	Model Access must be unlocked to read and write the OCV register.
	*/
	WriteWord(0x3E, 0x4A, 0x57);
	
	/******************************************************************************
	Step 5. Read OCV
	*/
	ReadWord(0x0E, &Volt_MSB, &Volt_LSB);
	OCV = ((Volt_MSB << 8) + Volt_LSB);
	
	/******************************************************************************
	Step 6. Determine maximum value of VCell1, VCell2, and OCV
	*/
	if((VCell1 > VCell2) && (VCell1 > OCV))
	{
		Desired_OCV = VCell1;
	}
	else if((VCell2 > VCell1) && (VCell2 > OCV))
	{
		Desired_OCV = VCell2;
	}
	else
	{
		Desired_OCV = OCV;
	}
	
	/******************************************************************************
	Step 7. Write OCV
	*/
	WriteWord(0x0E, Desired_OCV >> 8, Desired_OCV & 0xFF);
	
	/******************************************************************************
	Step 8. Lock Model Access
	*/
	WriteWord(0x3E, 0x00, 0x00);
	
	/******************************************************************************
	Step 9. Delay 125ms
	This delay must be at least 150mS before reading the SOC Register to allow
	the correct value to be calculated by the device.
	*/
	delay_ms(250);
}

void prepare_to_load_model(void)
{
	u8_t unlock_test_OCV_1, unlock_test_OCV_2;

step_1:	
	/******************************************************************************
	Step 1. Unlock Model Access
	This enables access to the OCV and table registers
	*/
	WriteWord(0x3E, 0x4A, 0x57);
	
	/******************************************************************************
	Step 2. Read OCV
	The OCV Register will be modified during the process of loading the custom
	model. Read and store this value so that it can be written back to the
	device after the model has been loaded.
	*/
	ReadWord(0x0E, &original_OCV_1, &original_OCV_2);
	
	/******************************************************************************
	Step 2.5. Verify Model Access Unlocked
	If Model Access was correctly unlocked in Step 1, then the OCV bytes read
	in Step 2 will not be 0xFF. If the values of both bytes are 0xFF,
	that indicates that Model Access was not correctly unlocked and the
	sequence should be repeated from Step 1.
	*/
	if((original_OCV_1 == 0xFF) && (original_OCV_2 == 0xFF))
	{
		goto step_1;
	}
	
	/******************************************************************************
	Step 2.5.1 MAX17058/59 version 0x0011 Only
	To ensure good RCOMPSeg values in MAX17058/59, a POR signal has to be sent to
	MAX17058 for version 0x0011. The OCV must be saved before the POR signal is sent
	for best Fuel Gauge
	performance. Step 2.5.1 is not required for version 0x0012.
	*/
	//do
	//{
	//	WriteWord(0xFE, 0x54, 0x00); //Send POR command
	//	WriteWord(0x3E, 0x4A, 0x57); //Send Unlock command
	//	ReadWord(0x0E, &unlock_test_OCV_1, &unlock_test_OCV_2);
	//	//Test Model access unlock similar to step 2.
	//} while((unlock_test_OCV_1 == 0xFF) && (unlock_test_OCV_2 == 0xFF));
	
	/******************************************************************************
	Step 3. Write OCV (MAX17040/1/3/4 only)
	Find OCVTest_High_Byte and OCVTest_Low_Byte values in INI file
	*/
	//WriteWord(0x0E, INI_OCVTEST_HIGH_BYTE, INI_OCVTEST_LOW_BYTE);
	
	/******************************************************************************
	Step 4. Write RCOMP to its Maximum Value (MAX17040/1/3/4 only)
	Make the fuel-gauge respond as slowly as possible (MSB = 0xFF), and disable
	alerts during model loading (LSB = 0x00)
	*/
	//WriteWord(0x0C, 0xFF, 0x00);
}

void load_model(void)
{
	int k;
	u8_t databuf[10] = {0};
	u8_t addr_mem;
	u32_t RCOMPSeg = RCOMPSEG;
	unsigned char RCOMPSeg_MSB = (RCOMPSeg >> 8) & 0xFF;
	unsigned char RCOMPSeg_LSB = RCOMPSeg & 0xFF;	
	u8_t model_data[65] = 
	{
		// Fill in your model data here from the INI file
		// 0x##, 0x##, ..., 0x##
		0x40,
		0xA3,0x90,0xB5,0x40,0xB7,0xC0,0xB8,0xA0,0xBA,0xE0,
		0xBC,0x30,0xBD,0x80,0xBE,0xC0,0xC0,0xA0,0xC3,0x00,
		0xC5,0x70,0xC7,0xA0,0xCB,0xF0,0xCE,0x70,0xD2,0x10,
		0xD7,0x50,0x00,0x80,0x08,0xE0,0x12,0x80,0x0F,0xE0,
		0x15,0xC0,0x12,0xC0,0x19,0xE0,0x12,0xF0,0x0C,0xF0,
		0x09,0xE0,0x09,0x30,0x08,0x80,0x07,0xE0,0x09,0x00,
		0x06,0x10,0x06,0x10
	};
	u8_t RCOMP_data[33] = // 1+16*2, first byte is the memory address
	{
		// Fill in RCOMP data 0x0080
		// 0x##, 0x##, ..., 0x##
		0x80,
		0x04,0x00,0x04,0x00,0x04,0x00,0x04,0x00,0x04,0x00,
		0x04,0x00,0x04,0x00,0x04,0x00,0x04,0x00,0x04,0x00,
		0x04,0x00,0x04,0x00,0x04,0x00,0x04,0x00,0x04,0x00,
		0x04,0x00
	};

	/******************************************************************************
	Step 5. Write the Model
	Once the model is unlocked, the host software must write the 64 byte model
	to the device. The model is located between memory 0x40 and 0x7F.
	The model is available in the INI file provided with your performance
	report. See the end of this document for an explanation of the INI file.
	Note that the table registers are write-only and will always read
	0xFF. Step 9 will confirm the values were written correctly.
	*/

	/* Perform these I2C tasks:
	START
	Send Slave Address (0x6C)
	Send Memory Location (0x40)
	for(k=0;k<0x40;k++)
	{
		Send Data Byte (model_data[k])
	}
	*/
	//addr_mem = 0x40;
	/*
	for(k=0;k<0x40;k++)
	{
		pmu_dev_ctx.write_reg(pmu_dev_ctx.handle, addr_mem, &model_data[k], 1);
		addr_mem++;
	}
	*/
	WriteMulti(model_data, sizeof(model_data));
	
	/******************************************************************************
	Step 5.1 Write RCOMPSeg (MAX17048/MAX17049 only)
	MAX17048 and MAX17049 have expanded RCOMP range to battery with high
	resistance due to low temperature or chemistry design. RCOMPSeg can be
	used to expand the range of RCOMP. For these devices, RCOMPSeg should be
	written along with the default model. For INI files without RCOMPSeg
	specified, use RCOMPSeg = 0x0080.
	*/

	/* Perform I2C:
	Send Memory Location (0x80)
	*/
	//addr_mem = 0x80;
	//databuf[0] = RCOMPSeg_MSB;
	//databuf[1] = RCOMPSeg_LSB;
	/*
	for(k=0;k<0x10;k++)
	{
		// Send Data Byte (RCOMPSeg_MSB)
		// Send Data Byte (RCOMPSeg_LSB)
		pmu_dev_ctx.write_reg(pmu_dev_ctx.handle, addr_mem, databuf, 2);
		addr_mem+2;
	}
	*/
	/* I2C STOP */
	WriteMulti(RCOMP_data, sizeof(RCOMP_data));
}

bool verify_model_is_correct(void)
{
	/******************************************************************************
	Step 6. Delay at least 150ms (MAX17040/1/3/4 only)
	This delay must be at least 150mS, but the upper limit is not critical
	in this step.
	*/
	//delay_ms(150);
	
	/******************************************************************************
	Step 7. Write OCV
	This OCV should produce the SOC_Check values in Step 9
	*/
	WriteWord(0x0E, INI_OCVTEST_HIGH_BYTE, INI_OCVTEST_LOW_BYTE);
	
	/******************************************************************************
	Step 7.1 Disable Hibernate (MAX17048/49 only)
	The IC updates SOC less frequently in hibernate mode, so make sure it
	is not hibernating
	*/
	WriteWord(0x0A, 0, 0);
	
	/******************************************************************************
	Step 7.2. Lock Model Access (MAX17048/49/58/59 only)
	To allow the ModelGauge algorithm to run in MAX17048/49/58/59 only, the model
	must
	be locked. This is harmless but unnecessary for MAX17040/1/3/4
	*/
	WriteWord(0x3E, 0, 0);
	
	/******************************************************************************
	Step 8. Delay between 150ms and 600ms
	This delay must be between 150ms and 600ms. Delaying beyond 600ms could
	cause the verification to fail.
	*/
	delay_ms(300);
	
	/******************************************************************************
	Step 9. Read SOC Register and compare to expected result
	There will be some variation in the SOC register due to the ongoing
	activity of the ModelGauge algorithm. Therefore, the upper byte of the SOC
	register is verified to be within a specified range to verify that the
	model was loaded correctly. This value is not an indication of the state of
	the actual battery. Please note that INI_SOCCheckA and INI_SOCCheckB has a
	fixed LSB of 1/256% for both 18 and 19 bit models.
	*/
	ReadWord(0x04, &SOC_1, &SOC_2);
	if(SOC_1 >= SOCCHECKA && SOC_1 <= SOCCHECKB)
	{
		// model was loaded successfully
		return true;
	}
	else
	{
		// model was NOT loaded successfully
		return false;
	}
}

void cleanup_model_load(void)
{
	/******************************************************************************
	Step 9.1. Unlock Model Access (MAX17048/49/58/59 only)
	To write OCV, MAX17048/49/58/59 requires model access to be unlocked.
	*/
	WriteWord(0x3E, 0x4A, 0x57);
	
	/******************************************************************************
	Step 10. Restore CONFIG and OCV
	It is up to the application how to configure the LSB of the CONFIG
	register; any byte value is valid.
	*/
	WriteWord(0x0C, RCOMP0, 0x5C);
	WriteWord(0x0E, original_OCV_1, original_OCV_2);
	
	/******************************************************************************
	Step 10.1 Restore Hibernate (MAX17048/49 only)
	Remember to restore your desired Hibernate configuration after the
	model was verified.
	*/
	// Restore your desired value of HIBRT
	WriteWord(0x0A, 0x80, 0x30);
	
	/******************************************************************************
	Step 11. Lock Model Access
	*/
	WriteWord(0x3E, 0x00, 0x00);
	
	/******************************************************************************
	Step 12. Wait 150ms Minimum
	This delay must be at least 150mS before reading the SOC Register to allow
	the correct value to be calculated by the device.
	*/
	delay_ms(300);
	
	/******************************************************************************
	Step 13. Final SOC check (MAX17040/41/43/44 only)
	Check for normal update of SOC after OCV restore
	*/
	//if(SOC < SOCCHECKA)
	//	model was loaded successfully
	//else
	//	goto step C1 and reload model
}

/* If you know the model is not loaded, call:
handle_model(LOAD_MODEL);
If you want to verify the model, and correct errors, call:
handle_model(VERIFY_AND_FIX);
*/
void handle_model(int load_or_verify)
{
	bool model_load_ok = false;
	u8_t retry = 3;
	
	do
	{
		// Steps 1-4
		prepare_to_load_model();
		if(load_or_verify == LOAD_MODEL)
		{
			//Step 5
			load_model();
		}
		
		//Steps 6-9
		model_load_ok = verify_model_is_correct();
		if(!model_load_ok)
		{
			load_or_verify = LOAD_MODEL;
		}

		retry--;
	}while((!model_load_ok)&&(retry>0));
	
	// Steps 10-11
	cleanup_model_load();
}

int MAX20353_SOCReadReg(u8_t reg, u8_t *MSB, u8_t *LSB)
{
	u32_t ret;
	u8_t data = reg;
	u8_t value[2];

	ret = i2c_write(pmu_dev_ctx.handle, &data, sizeof(data), MAX20353_I2C_ADDR_FUEL_GAUGE);
	if(ret != 0)
	{
		return MAX20353_ERROR;
	}

	ret = i2c_read(pmu_dev_ctx.handle, value, sizeof(value), MAX20353_I2C_ADDR_FUEL_GAUGE);
	if (ret != 0)
	{
		return MAX20353_ERROR;
	}
	
	*MSB = value[0];
	*LSB = value[1];

	return MAX20353_NO_ERROR;

}

int MAX20353_SOCWriteReg(u8_t reg, u8_t MSB, u8_t LSB)
{
	u8_t cmdData[3] = {reg, MSB, LSB};
	u32_t rslt = 0;

	rslt = i2c_write(pmu_dev_ctx.handle, cmdData, sizeof(cmdData), MAX20353_I2C_ADDR_FUEL_GAUGE);
	if (rslt != 0)
		return MAX20353_ERROR;
	return MAX20353_NO_ERROR;
}

void MAX20353_SOCInit(void)
{
	u8_t MSB,LSB;

	MAX20353_SOCReadReg(0x1A, &MSB, &LSB);
	if(MSB&0x01)
	{
		//RI (reset indicator) is set when the device powers up.
		//Any time this bit is set, the IC is not configured, so the
		//model should be loaded and the bit should be cleared
		MSB = MSB&0xFE;
		MAX20353_SOCWriteReg(0x1A, MSB, LSB);
		MAX20353_QuickStart();
		delay_ms(150);
		
		handle_model(LOAD_MODEL);
	}
	
	//设置默认温度25度，SOC变化1%报警，电量小于4%报警
	WriteWord(0x0C, 0x12, 0x5C);

#ifdef BATTERT_NTC_CHECK
	MAX20353_StartCheckTemper();
#endif
}
#endif/*BATTERY_SOC_GAUGE*/
