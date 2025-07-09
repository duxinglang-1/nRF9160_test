/****************************************Copyright (c)************************************************
** File Name:			    max86176.c
** Descriptions:			PPG AFE process source file
** Created By:				xie biao
** Created Date:			2025-07-07
** Modified Date:      		2025-07-07 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "max86176.h"
#include "logger.h"

#define MAX86176_DEBUG

struct device *spi_ppg;
struct device *gpio_ppg;
static struct gpio_callback gpio_cb1,gpio_cb2;

static struct spi_buf_set tx_bufs,rx_bufs;
static struct spi_buf tx_buff,rx_buff;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

bool ppg_int1_flag = false;
bool ppg_int2_flag = false;

bool gUseSpi = true;
uint8_t gReadBuf[NUM_SAMPLES_PER_INT*NUM_BYTES_PER_SAMPLE*EXTRABUFFER];	// array to store register reads
bool gUseEcg=true, gUseEcgPpgTime=true;	// modify these as desired

static void PPG_Sleep_us(int us)
{
	k_sleep(K_MSEC(1));
}

static void PPG_Sleep_ms(int ms)
{
	k_sleep(K_MSEC(ms));
}

static void PPG_CS_LOW(void)
{
	gpio_pin_set(gpio_ppg, PPG_CS_PIN, 0);
}

static void PPG_CS_HIGH(void)
{
	gpio_pin_set(gpio_ppg, PPG_CS_PIN, 1);
}

void PPG_Int1_Event(void)
{
	ppg_int1_flag = true;
}

void PPG_Int2_Event(void)
{
	ppg_int2_flag = true;
}

static void PPG_gpio_Init(void)
{
	int err;
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_UP;
	
  	//¶Ë¿Ú³õÊ¼»¯
  	gpio_ppg = DEVICE_DT_GET(PPG_PORT);
	if(!gpio_ppg)
	{
		return;
	}

	gpio_pin_configure(gpio_ppg, PPG_CS_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ppg, PPG_CS_PIN, 1);
	
	//interrupt
	gpio_pin_configure(gpio_ppg, PPG_INT1_PIN, flag);
	gpio_pin_interrupt_configure(gpio_ppg, PPG_INT1_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb1, PPG_Int1_Event, BIT(PPG_INT1_PIN));
	gpio_add_callback(gpio_ppg, &gpio_cb1);
	gpio_pin_interrupt_configure(gpio_ppg, PPG_INT1_PIN, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);

	gpio_pin_configure(gpio_ppg, PPG_INT2_PIN, flag);
	gpio_pin_interrupt_configure(gpio_ppg, PPG_INT2_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb2, PPG_Int2_Event, BIT(PPG_INT2_PIN));
	gpio_add_callback(gpio_ppg, &gpio_cb2);
	gpio_pin_interrupt_configure(gpio_ppg, PPG_INT2_PIN, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);

}

static void PPG_SPI_Init(void)
{
	spi_ppg = DEVICE_DT_GET(PPG_DEV);
	if(!spi_ppg) 
	{
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	spi_cfg.frequency = 4000000;
	spi_cfg.slave = 0;
}

static void PPG_SPI_Transceive(uint8_t *txbuf, uint32_t txbuflen, uint8_t *rxbuf, uint32_t rxbuflen)
{
	int err;
	
	tx_buff.buf = txbuf;
	tx_buff.len = txbuflen;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	rx_buff.buf = rxbuf;
	rx_buff.len = rxbuflen;
	rx_bufs.buffers = &rx_buff;
	rx_bufs.count = 1;

	PPG_CS_LOW();
	err = spi_transceive(spi_ppg, &spi_cfg, &tx_bufs, &rx_bufs);
	PPG_CS_HIGH();

	if(err)
	{
	}
}

void Max86176_WriteReg(uint8_t regAddr, uint8_t val)
{
	uint8_t i = 0;
	uint8_t tx_buf[8];
	
	tx_buf[i++] = regAddr;
	tx_buf[i++] = 0x00;
	tx_buf[i++] = val;
	PPG_SPI_Transceive(tx_buf, i, NULL, 0);
}

void Max86176_ReadReg(uint8_t regAddr, uint8_t numBytes, uint8_t *readbuf)
{
	uint8_t i = 0;
	uint8_t tx_buf[8];
	uint8_t rx_buf[numBytes+2];
	
	tx_buf[i++] = regAddr;
	tx_buf[i++] = 0x80;
	PPG_SPI_Transceive(tx_buf, i, rx_buf, numBytes+2);
	memcpy(readbuf, rx_buf+2, numBytes);
}

static void Max86176_Init(void)	// call this on power up
{
	uint8_t part_id;

	Max86176_ReadReg(0xff, 1, &part_id);
#ifdef MAX86176_DEBUG
	LOGD("ID:%d", part_id);
#endif
	if(part_id != MAX86176_PART_ID)
		return;
	
	Max86176_WriteReg(0x10, 1);	// RESET
	while(1)
	{
		Max86176_ReadReg(0x10, 1, &gReadBuf[0]);
	#ifdef MAX86176_DEBUG
		LOGD("reset:%d", gReadBuf[0]);
	#endif	
		if((gReadBuf[0]&0x01) == 0x00)	//This bit then automatically becomes ¡®0¡¯ after thereset sequence is completed. 
			break;
	}
	
	Max86176_ReadReg(0x00, NUM_STATUS_REGS, gReadBuf);	// read and clear all status registers
	Max86176_WriteReg(0x0d, AFE_FIFO_SIZE-NUM_SAMPLES_PER_INT);	// FIFO_A_FULL; assert A_FULL on NUM_SAMPLES_PER_INT samples
	Max86176_WriteReg(0x12, ((gUseEcgPpgTime?1:0)<<2));	// PPG_TIMING_DATA; note that the initial PPG frames may not have an associated ECG sample if they come before the ECG samples have started
	Max86176_WriteReg(0x80, 0x80);	// A_FULL_EN; enable interrupt pin on A_FULL assertion; ensure to enable the MCU's interrupt pin

	//PLL (32.768kHz*(0x1f+289)=10.486MHz)
	Max86176_WriteReg(0x19, (1<<7)|0x1f);	// MDIV=0x1f, NDIV=0x13f
	Max86176_WriteReg(0x1a, 0x3f);	// NDIV
	Max86176_WriteReg(0x18, (1<<7)|1);	// PLL_LOCK_WNDW | PLL_EN
	gReadBuf[4] = 0;
	while((gReadBuf[2]&0x7)!=0x2)	// wait for PLL lock
	{
		PPG_Sleep_ms(500);
		Max86176_ReadReg(0x00, NUM_STATUS_REGS, gReadBuf);
	}
	
    //ECG (32.768kHz*(0x1f+289)/(0x13f+1)/64=512Sps)
    Max86176_WriteReg(0x90, ((gUseEcg?1:0)<<7)|2); // ECG_EN | ECG_DEC_RATE
	Max86176_WriteReg(0xa2, (0<<7)|(0<<6)); 	// OPEN_P | OPEN_N

	//PPG (32.768kHz/256=128Fps)
	uint32_t FrClkDiv = 0x100;
	Max86176_WriteReg(0x11, 0x07);	// MEAS1-3_EN; enable first NUM_MEAS_PER_FRAME measurements
	Max86176_WriteReg(0x1d, (FrClkDiv>>8)&0xff);	// FR_CLK_DIV
	Max86176_WriteReg(0x1e, (FrClkDiv>>0)&0xff);	// FR_CLK_DIV
	Max86176_WriteReg(0x25, 0x10);	// MEAS1_DRVA_PA; set current to non-0
	Max86176_WriteReg(0x2d, 0x10);	// MEAS2_DRVA_PA; set current to non-0
	Max86176_WriteReg(0x35, 0x10);	// MEAS3_DRVA_PA; set current to non-0
	// ACLOFF (ADC: ~10Sps, DAC: 32.768kHz*(0x1f+289)/(10*64*(2+1))=5461.3Hz)
	Max86176_WriteReg(0x93, (2<<5));	// EN_LOFF_DET
	Max86176_WriteReg(0x94, (1<<6)|(2<<4)|5);	// HI_CM_RES_EN (1 to use ECG common-mode input impedance) | LOFF_CG_MODE (2 when using RLD or lead bias) | LOFF_IMAG (200nA)
	Max86176_WriteReg(0x95, (1<<7)|2);	// AC_LOFF_IWAVE (1=sine) | AC_LOFF_FREQ_DIV
	Max86176_WriteReg(0x99, 0x10);	// AC_LOFF_THRESH
	Max86176_WriteReg(0x9a, (0<<6)|5);	// AC_LOFF_UTIL_PGA_GAIN | AC_LOFF_HPF
	// other relevant ACLOFF registers are at defaults: AC_LOFF_CONV=0 (~10Sps), AC_LOFF_CMP=2
	// RLD
	Max86176_WriteReg(0xa8, (1<<7)|(1<<6)|(1<<4)|(1<<3)|(1<<2)|3);	// RLD_EN | RLD_MODE | EN_RLD_OOR | ACTV_CM_P | ACTV_CM_N | RLD_GAIN
	Max86176_WriteReg(0xa9, (0<<7)|(1<<6)|(0<<4)|0);	// RLD_EXT_RES | SEL_VCM_IN | RLD_BW | BODY_BIAS_DAC
}

void Max86176_onAfeInt(void)	// call this on AFE interrupt
{
	static int32_t gEcgSampleCount=-1;
	static bool gEcgPpgTimeOccurred;	// static since the reference sample have come in the previous interrupt
	static uint8_t gPpgFrameItemCount;
	
	Max86176_ReadReg(0x00, NUM_STATUS_REGS, gReadBuf);	// read and clear all status registers
	if (!(gReadBuf[0]&0x80))	// check A_FULL bit
		return;
	
	Max86176_ReadReg(0x0a, 2, gReadBuf);	// read FIFO_DATA_COUNT
	uint32_t count = ((gReadBuf[0]&0x80)<<1)|gReadBuf[1];	// FIFO_DATA_COUNT will be >= NUM_SAMPLES_PER_INT
	Max86176_ReadReg(0x0c, count*NUM_BYTES_PER_SAMPLE, gReadBuf);	// read FIFO_DATA
	uint8_t readBufIx, sampleIx[NUM_ADC]={0}, tag;
	int32_t adcCountArr[NUM_ADC][NUM_SAMPLES_PER_INT*EXTRABUFFER];	// array to store ECG/ACLOFF/PPG ADC counts, time data
	for (readBufIx=0; readBufIx<count*NUM_BYTES_PER_SAMPLE; readBufIx+=NUM_BYTES_PER_SAMPLE)	// parse the FIFO data
	{
		tag = (gReadBuf[readBufIx]>>4)&0xf;
		if (tag<=TAG_PPG_MAX && 
			((gUseEcg && gUseEcgPpgTime && gEcgPpgTimeOccurred) ||
			!(gUseEcg && gUseEcgPpgTime)
			))
		// If time data and the ADC for the time data are enabled, only save samples that have associated time data. PPG samples may not have associated time data if they come before ECG samples, which may occur on start-up or PLL unlock.
		{
			if (++gPpgFrameItemCount == (NUM_MEAS_PER_FRAME*NUM_PPG_PER_MEAS))
			{
				gPpgFrameItemCount = 0;
				gEcgPpgTimeOccurred = false;
			}
			adcCountArr[IX_PPG][sampleIx[IX_PPG]] = ((gReadBuf[readBufIx+0]&0xf)<<16) + (gReadBuf[readBufIx+1]<<8) + gReadBuf[readBufIx+2];
			if (gReadBuf[readBufIx+0]&0x8)
				adcCountArr[IX_PPG][sampleIx[IX_PPG]]-=(1<<20);
			sampleIx[IX_PPG]++;
		}
		else if (tag==TAG_ECG)
		{
			gEcgSampleCount++;
			adcCountArr[IX_ECG][sampleIx[IX_ECG]++] = (gReadBuf[readBufIx+0]>>2)&1;	// Note that every other item is ECG fast recovery. Comment this out if not needed.
			adcCountArr[IX_ECG][sampleIx[IX_ECG]] = ((gReadBuf[readBufIx+0]&0x3)<<16) + (gReadBuf[readBufIx+1]<<8) + gReadBuf[readBufIx+2];
			if (gReadBuf[readBufIx+0]&0x2)
				adcCountArr[IX_ECG][sampleIx[IX_ECG]]-=(1<<18);
			sampleIx[IX_ECG]++;
		}
		else if (tag==TAG_LOFFUTIL)
		{
			tag = (gReadBuf[readBufIx+0]>>2)&1;	// this can also be used for the array index in this example
			adcCountArr[tag][sampleIx[tag]] = ((gReadBuf[readBufIx+1]&0xf)<<8) + gReadBuf[readBufIx+2];
			if (gReadBuf[readBufIx+0]&0x8)
				adcCountArr[tag][sampleIx[tag]]-=(1<<12);
			sampleIx[tag]++;
		}
		else if (tag==TAG_TIME)
		{			
			gEcgPpgTimeOccurred = true;
			adcCountArr[IX_TIME][sampleIx[IX_TIME]++] = gEcgSampleCount;	// the ECG sample number that this PPG_TIMING_DATA is associated with
			adcCountArr[IX_TIME][sampleIx[IX_TIME]++] = ((gReadBuf[readBufIx+1]&0x3)<<8) + gReadBuf[readBufIx+2];	
		}
	}
	// Process adcCountArr[][] here. sampleIx[] tells how many samples are in each adcCountArr[].
	// If sampleIx[IX_PPG]!=0 && gPpgFrameItemCount!=0, the complete frame has not been read (it will be complete on the next FIFO read), so account for that in the processing.
	// The shorter the delay between the HW interrupt and calling this function, and the more PPG measurements to be made, the higher the likelihood that the PPG frame may not be complete on this read.
}

void PPG_Sensor_Init(void)
{
	PPG_gpio_Init();
	PPG_SPI_Init();

	Max86176_Init();
}

