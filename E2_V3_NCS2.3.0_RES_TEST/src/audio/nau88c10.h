/****************************************Copyright (c)************************************************
** File Name:			    nau88c10.c
** Descriptions:			audio codec process head file
** Created By:				xie biao
** Created Date:			2021-03-02
** Modified Date:      		2021-03-02 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __NAU88C10_H__
#define __NAU88C10_H__

#include <nrf9160.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

extern struct device *nau88c10_I2C;

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define AUDIO_DEV DT_NODELABEL(i2c1)
#else
#error "spi3 devicetree node is disabled"
#define AUDIO_DEV	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define AUDIO_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define AUDIO_PORT	""
#endif
	
#define AUDIO_SCL	31
#define AUDIO_SDA	30
#define AUDIO_LDO_EN	0

#define NAU88C10_DEVICE_ID	0x0EE
	
#define NAU88C10_I2C_ADDR	(0x34 >> 1)
	
#define NAU88C10_NO_ERROR   0
#define NAU88C10_ERROR      -1

typedef enum
{
	//Software Reset
	REG_SW_RESET		= 0x00,

	//Power Management
	REG_POWER_MG_1		= 0x01,	 	//= 0x17d //ʹ����˷�ƫ�õ�·	
	REG_POWER_MG_2		= 0x02,		//= 0x15	//ʹ��PGA��ѹ�� 
	REG_POWER_MG_3		= 0x03,		//= 0xfd	//ʹ��DACģ�� 

	//Audio Control,����ƽ̨���ʼ��I2S�ӿ�Ϊ��1�����豸��2��PCMA��ʽ��3���ֿ�ȣ�16�ֽڣ��������Ƶ��48KHZ��
	//����ƽ̨�Ǳ߶�Ӧ��Ƶ��Ӧ��Ϊ��BCLK - 1.536MHZ��Fs - 48KHZ�� 
	REG_AI_FORMAT		= 0x04,	 	//= 0x18	//I2S�ӿ����ã� PCMA��ʽ���ֳ��ȣ�WLEN����16�ֽ�    
	REG_COMPANDING		= 0x05,		//= 0x0  //0x0���ر�ADC ֱ��ͨ��DAC��0x1����ADCֱ��ͨ��DAC-���ڲ���ADC�Ĺ����Ƿ�����
	REG_CLOCK_CTRL_1 	= 0x06,		//= 0x148  //I2S�ӿ����ã�0-I2S�����ڴ�ģʽ��,��ʱ������˷�������ź�ֱ��ADC��ͨ��I2S�ӿ����������ƽ̨��1-I2S��������ģʽ��
	REG_CLOCK_CTRL_2	= 0x07,		//= 0x0    //ADC�Ĳ���Ƶ�ʣ�����Ϊ48KHZ
	//0x8 = 0x0
	//0x9 = 0x0
	REG_DAC_CTRL		= 0x0A,		//= 0x08    //����Ͼ������ƼĴ���
	REG_DAC_VOL			= 0x0B,		//= 0x1ff  //DAC������ƼĴ���
	//0xc = 0x0    
	//0xd = 0x0
	REG_ADC_CTRL		= 0x0E,		//= 0x108   //ADC����Ƶ�ʿ��ƼĴ���
	REG_ADC_VOL			= 0x0F,		//= 0x1ff   //ADC������ƼĴ���

	//5-BAND EQUALIZER CONTROL REGISTERS
	REG_EQ1_LOW_CUTOFF	= 0x12,		//= 0x12c
	REG_EQ2_PEAK_1		= 0x13,		//= 0x2c
	REG_EQ3_PEAK_2		= 0x14,		//= 0x2c
	REG_EQ4_PEAK_3		= 0x15,		//= 0x2c
	REG_EQ5_HIGI_CUTOFF	= 0x16,		//= 0x2c

	//DAC LIMITER REGISTERS
	REG_DAC_LIMIT_1		= 0x18,		//= 0x32
	REG_DAC_LIMIT_2		= 0x19,		//= 0x0

	//Notch Filter
	REG_NFC_0_HIGI		= 0x1B,		//= 0x0
	REG_NFC_0_LOW		= 0x1C,		//= 0x0
	REG_NFC_1_HIGI		= 0x1D,		//= 0x0
	REG_NFC_1_LOW		= 0x1E,		//= 0x0

	//ALC Control
	REG_ALC_CTRL_1		= 0x20,		//= 0x38
	REG_ALC_CTRL_2		= 0x21,		//= 0xb
	REG_ALC_CTRL_3		= 0x22,		//= 0x32
	REG_NOISE_GATE		= 0x23,		//= 0x0

	//PLL Control
	REG_PLL_N_CTRL		= 0x24,		//= 0x8
	REG_PLL_K_1			= 0x25,		//= 0xc
	REG_PLL_K_2			= 0x26,		//= 0x93
	REG_PLL_K_3			= 0x27,		//= 0xe9

	//INPUT, OUTPUT & MIXER CONTROL
	REG_ATT_CTRL		= 0x28,		//= 0x0
	REG_INPUT_CTRL		= 0x2C,		//= 0x3
	REG_PGA_GAIN		= 0x2D,		//= 0x2a
	REG_ADC_BOOST		= 0x2F,		//= 0x100
	REG_OUTPUT_CTRL		= 0x31,		//= 0x2
	REG_MIXER_CTRL		= 0x32,		//= 0x1
	REG_SPKOUT_VOL		= 0x36,		// = 0xb9
	REG_MONO_MIXER_CTRL	= 0x38,		//= 0x1

	//LOW POWER CONTROL
	REG_POWER_MG_4		= 0x3A,

	//PCM TIME SLOT & ADCOUT IMPEDANCE OPTION CONTROL
	REG_TIME_SLOT		= 0x3B,
	REG_ADCOUT_DRIVE	= 0x3C,

	REG_DEVICE_REVISION	= 0x3E,
	REG_2_WIRE_ID		= 0x3F,
	REG_ADDITIONAL_ID	= 0x40,
	REG_RESERVED		= 0x41,

	REG_HIGI_VOLTAGE_CTRL	= 0x45,
	REG_ALC_ENHANCE_1		= 0x46,
	REG_ALC_ENHANCE_2		= 0x47,
	REG_ADDITIONAL_IF_CTRL	= 0x49,
	REG_POWER_TIE_OFF_CTRL	= 0x4B,
	REG_AGC_P2P_DET			= 0x4C,
	REG_AGC_PEAK_DET		= 0x4D,
	REG_CTRL_AND_STATUS		= 0x4E,
	REG_OUTPUT_TIE_OFF_CTEL	= 0x4F
}MAU88C10_REG;

/** @addtogroup  Interfaces_Functions
  * @brief		 This section provide a set of functions used to read and
  * 			 write a generic register of the device.
  * 			 MANDATORY: return 0 -> no Error.
  * @{
  *
  *
**/
typedef int32_t (*dev_write_ptr)(struct device *handle, u8_t reg, u8_t *bufp, u16_t len);
typedef int32_t (*dev_read_ptr)(struct device *handle, u8_t reg, u8_t *bufp, u16_t len);

typedef struct {
  /** Component mandatory fields **/
  dev_write_ptr	write_reg;
  dev_read_ptr	read_reg;
  /** Customizable optional pointer **/
  struct device *handle;
} dev_ctx_t;


extern void Audio_init(void);
extern void AudioMsgProcess(void);

#endif/*__NAU88C10_H__*/
