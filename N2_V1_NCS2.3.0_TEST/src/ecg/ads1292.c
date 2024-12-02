/****************************************Copyright (c)************************************************
** File Name:			    ads1292.c
** Descriptions:			Sensor source file for ADS1292
** Created By:				xie biao
** Created Date:			2024-04-11
** Modified Date:      		2024-04-11
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "ads1292.h"
#include "logger.h"

#define ADS_DEBUG

struct device *spi_ecg;
struct device *gpio_ecg;
static struct gpio_callback gpio_cb;

static struct spi_buf_set tx_bufs,rx_bufs;
static struct spi_buf tx_buff,rx_buff;
static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;

static uint8_t spi_tx_buf[8] = {0};  
static uint8_t spi_rx_buf[8] = {0};  

static bool ecg_trige_flag = false;

uint8_t ADC_Read_data[16] = {0};
uint8_t ADS129x_SPI_cmd_Flag=0, ADS129x_SPI_data_Flag=0,  SPI_Send_count=0, SPI_Tx_Count = 0;
uint8_t SPI_Rx_Data_Flag = 0,  SPI_Rx_Count=0, SPI_Rx_exp_Count=0 ;
uint8_t SPI_Tx_buf[10] = {0};
uint8_t SPI_Rx_buf[12] = {0};
uint8_t ECGRecorder_data_Buf[80] = {0};
uint8_t Recorder_head;
uint8_t Recorder_tail;

long ADS1x9x_ECG_Data_buf[6] = {0};

ADS1x9x_state_t ECG_Recoder_state = {0};

uint8_t ADS1x9xRegVal[ADS_REG_MAX] = 
{
	//Device ID read Ony
	0x00,
	//CONFIG1
	0x02,
	//CONFIG2
	0xE0,
	//LOFF
	0xF0,
	//CH1SET (PGA gain = 6)
	0x00,
	//CH2SET (PGA gain = 6)
	0x00,
	//RLD_SENS (default)
	0x2C,
	//LOFF_SENS (default)
	0x0F,    
	//LOFF_STAT
	0x00,
	//RESP1
	0xEA,
	//RESP2
	0x03,
	//GPIO
	0x0C 
};		

uint8_t ADS1x9xR_Default_Reg_Settings[ADS_REG_MAX] = 
{
	//Device ID read Ony
	0x00,
	//CONFIG1
	0x02,
	//CONFIG2
	0xE0,
	//LOFF
	0xF0,
	//CH1SET (PGA gain = 6)
	0x00,
	//CH2SET (PGA gain = 6)
	0x00,
	//RLD_SENS (default)
	0x2C,
	//LOFF_SENS (default)
	0x0F,    
	//LOFF_STAT
	0x00,
	//RESP1
	0xEA,
	//RESP2
	0x03,
	//GPIO
	0x0C 
};		

uint8_t ADS1x9x_Default_Reg_Settings[ADS_REG_MAX] = 
{
	//Device ID read Ony
	0x00,
	//CONFIG1
	0x02,
	//CONFIG2
	0xE0,
	//LOFF
	0xF0,
	//CH1SET (PGA gain = 6)
	0x00,
	//CH2SET (PGA gain = 6)
	0x00,
	//RLD_SENS (default)
	0x2C,
	//LOFF_SENS (default)
	0x0F,    
	//LOFF_STAT
	0x00,
	//RESP1
	0x02,
	//RESP2
	0x03,
	//GPIO
	0x0C 
};		

void EcgInterruptHandle(void)
{
	ecg_trige_flag = true;
}

void ECG_CS_LOW(void)
{
	gpio_pin_set(gpio_ecg, ECG_CS_PIN, 0);
}

void ECG_CS_HIGH(void)
{
	gpio_pin_set(gpio_ecg, ECG_CS_PIN, 1);
}

void ECG_START_LOW(void)
{
	gpio_pin_set(gpio_ecg, ECG_START_PIN, 0);
}

void ECG_START_HIGH(void)
{
	gpio_pin_set(gpio_ecg, ECG_START_PIN, 1);
}

void ECG_RESET_LOW(void)
{
	gpio_pin_set(gpio_ecg, ECG_RESET_PIN, 0);
}

void ECG_RESET_HIGH(void)
{
	gpio_pin_set(gpio_ecg, ECG_RESET_PIN, 1);
}

void ECG_SPI_Init(void)
{
	spi_ecg = DEVICE_DT_GET(ECG_DEVICE);
	if(!spi_ecg) 
	{
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8);
	spi_cfg.frequency = 1000000;
	spi_cfg.slave = 0;
}

void ADS_Sys_Set(ADS_SYS_COMMAND sys_command)
{
	int err;
	
	spi_tx_buf[0] = sys_command;

	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	ECG_CS_LOW();
	
	err = spi_transceive(spi_ecg, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}

	ECG_CS_HIGH();
}

void ADS_Data_Write(ADS_DATA_COMMAND data_command)
{
	int err;
		
	spi_tx_buf[0] = data_command;

	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	ECG_CS_LOW();
	
	err = spi_transceive(spi_ecg, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error: %d", err);
	#endif
	}

	ECG_CS_HIGH();
}

void ADS_Data_Read(ADS_DATA_COMMAND data_command, uint8_t *rxbuf, uint32_t len)
{
	int err;
	
	spi_tx_buf[0] = data_command;

	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	ECG_CS_LOW();
	
	err = spi_transceive(spi_ecg, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error 001: %d", err);
	#endif
	}

	rx_buff.buf = rxbuf;
	rx_buff.len = len;
	rx_bufs.buffers = &rx_buff;
	rx_bufs.count = 1;

	err = spi_transceive(spi_ecg, &spi_cfg, NULL, &rx_bufs);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error 002: %d", err);
	#endif
	}

	ECG_CS_HIGH();
}

void ADS_Reg_Write(ADS_RED_ADDR addr, uint8_t data)
{
	int err;

	switch(addr)
  	{
	case ADS_REG_CFG1://This register configures each ADC channel sample rate.
		data = data&0x87;
		break;
	case ADS_REG_CFG2://This register configures the test signal, clock, reference, and LOFF buffer.
		data = data&0xFB;
		data |= 0x80;
		break;
	case ADS_REG_LOFF://This register configures the lead-off detection operation.
		data = data&0xFD;
		data |= 0x10;
		break;
	case ADS_REG_CH1SET://This register configures the power mode, PGA gain, and multiplexer settings channels(PD1).
	case ADS_REG_CH2SET://This register configures the power mode, PGA gain, and multiplexer settings channels(PD2).
	case ADS_REG_RLD_SENS://This register controls the selection of the positive and negative signals from each channel for right leg drive derivation.
		break;
	case ADS_REG_LOFF_SENS://This register selects the positive and negative side from each channel for lead-off detection.
		data = data&0x3F;
		break;
	case ADS_REG_LOFF_STAT://This register stores the status of whether the positive or negative electrode on each channel is on or off.
		data = data&0x5F;
		break;
	case ADS_REG_RESP1://This register controls the respiration functionality. This register applies to the ADS1292R version only.
		data |= 0x02;
		break;
	case ADS_REG_RESP2://This register controls the respiration and calibration functionality.
		data = data&0x87;
		data |= 0x01;
		break;
	case ADS_REG_GPIO://This register controls the GPIO pins.
		data = data&0x0F;
		break;
	default:
		break;
  	}
	
	spi_tx_buf[0] = ADS_RED_COM_WREG|(addr&0x1f);
	spi_tx_buf[1] = 0x00;

	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 2;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	ECG_CS_LOW();
	
	err = spi_transceive(spi_ecg, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error 001: %d", err);
	#endif
	}

	tx_buff.buf = &data;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	err = spi_transceive(spi_ecg, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error 002: %d", err);
	#endif
	}

	ECG_CS_HIGH();
}

void ADS_Reg_Read(ADS_RED_ADDR addr, uint8_t *data)
{
	int err;
	
	spi_tx_buf[0] = ADS_REG_COM_RREG|(addr&0x1f);
	spi_tx_buf[1] = 0x00;

	tx_buff.buf = spi_tx_buf;
	tx_buff.len = 2;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;

	ECG_CS_LOW();
	
	err = spi_transceive(spi_ecg, &spi_cfg, &tx_bufs, NULL);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error 001: %d", err);
	#endif
	}

	rx_buff.buf = data;
	rx_buff.len = 1;
	rx_bufs.buffers = &rx_buff;
	rx_bufs.count = 1;

	err = spi_transceive(spi_ecg, &spi_cfg, NULL, &rx_bufs);
	if(err)
	{
	#ifdef ADS_DEBUG
		LOGD("SPI error 002: %d", err);
	#endif
	}

	ECG_CS_HIGH();
}

void ADS1x9x_Reset(void)
{
	ECG_RESET_HIGH();
	k_sleep(K_MSEC(50));
	ECG_RESET_LOW();
	k_sleep(K_MSEC(50));
	ECG_RESET_HIGH();
	k_sleep(K_MSEC(50));
}
  
void ADS1x9x_Disable_Start(void)
{
	ECG_START_LOW();
    k_sleep(K_MSEC(50));
}

void ADS1x9x_Enable_Start(void)
{
	ECG_START_HIGH();
    k_sleep(K_MSEC(50));
}

void ADS1x9x_PowerDown_Enable(void)
{
	ECG_RESET_LOW();
    k_sleep(K_MSEC(50));
}

void ADS1x9x_PowerDown_Disable(void)
{
	ECG_RESET_HIGH();
    k_sleep(K_MSEC(50));
}

void Soft_Start_ReStart_ADS1x9x (void)
{
	ADS_Sys_Set(ADS_SYS_COM_START);
    ECG_CS_HIGH();                                                      
}

void Hard_Start_ReStart_ADS1x9x(void)
{
	ECG_START_HIGH();	// Set Start pin to High
}

void Soft_Start_ADS1x9x (void)
{
    ADS_Sys_Set(ADS_SYS_COM_START);	// Send 0x08 to the ADS1x9x
}

void Soft_Stop_ADS1x9x (void)
{
    ADS_Sys_Set(ADS_SYS_COM_STOP);	// Send 0x0A to the ADS1x9x
}

void Hard_Stop_ADS1x9x (void)
{
  	ECG_START_LOW();	// Set Start pin to Low
   	k_sleep(K_MSEC(50));
}

void Stop_Read_Data_Continuous (void)
{
	ADS_Data_Write(ADS_DATA_COM_SDATAC);	// Send 0x11 to the ADS1x9x
}

void Start_Read_Data_Continuous (void)
{
    ADS_Data_Write(ADS_DATA_COM_RDATAC);	// Send 0x10 to the ADS1x9x
}

void Start_Data_Conv_Command (void)
{
    ADS_Sys_Set(ADS_SYS_COM_START);			// Send 0x08 to the ADS1x9x
}

void ADS1191_Parse_Data_Packet(void)
{
	uint8_t ECG_Chan_num;

	switch(ECG_Recoder_state.state)
	{
	case ECG_STATE_IDLE:
		break;

	case ECG_STATE_DATA_STREAMING:
		for(ECG_Chan_num=0;ECG_Chan_num<2;ECG_Chan_num++)
		{
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = (signed long)SPI_Rx_buf[2*ECG_Chan_num]; 	// Get MSB 8 bits 
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = ADS1x9x_ECG_Data_buf[ECG_Chan_num] << 8;
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] |= SPI_Rx_buf[2*ECG_Chan_num+1];				// Get LSB 8 bits
		}
		
		ADS1x9x_ECG_Data_buf[0] = ADS1x9x_ECG_Data_buf[0] << 8;								// to make compatable with 24 bit devices
		break;
		
	case ECG_STATE_ACQUIRE_DATA:
	case ECG_STATE_RECORDING:
		{
   			uint8_t *ptr;
			ptr = &ECGRecorder_data_Buf[Recorder_head << 3]; // Point to Circular buffer at head*8;
			*ptr++ = SPI_Rx_buf[0];				// Store status 
			*ptr++ = SPI_Rx_buf[1];				// Store status 
//			if ((SPI_Rx_buf[2] & 0x80 ) == 0x80)// CH0[15-8] = MSB ( 16Bit device)
//			*ptr++ = 0xFF;						// CH0[23-16] = 0xFF ( 16Bit device) sign
//			else
//			*ptr++ = 0;							// CH0[23-16] = 0 ( 16Bit device)
			*ptr++ = SPI_Rx_buf[2];				// CH0[15-8] = MSB ( 16Bit device)
			*ptr++ = SPI_Rx_buf[3];				// CH0[7-0] = LSB ( 16Bit device)
			*ptr++ = 0;
//			if ((SPI_Rx_buf[2] & 0x80 ) == 0x80)// CH0[15-8] = MSB ( 16Bit device)
//			*ptr++ = 0xFF;						// CH0[23-16] = 0xFF ( 16Bit device) sign
//			else
//			*ptr++ = 0;							// CH0[23-16] = 0 ( 16Bit device)
			*ptr++ = SPI_Rx_buf[2];				// CH0[15-8] = MSB ( 16Bit device)
			*ptr++ = SPI_Rx_buf[3];				// CH0[7-0] = LSB ( 16Bit device)
			*ptr++ = 0;
			Recorder_head ++;					// Increment Circuler buffer pointer
			
			if(Recorder_head == 32)				// Check for Circuler buffer depth.
				Recorder_head = 0;				// Rest once it reach to MAX
		}
		break;
       
	default:
		break;
	}
}

void ADS1192_Parse_Data_Packet(void)
{
	uint8_t ECG_Chan_num;
	
	switch(ECG_Recoder_state.state)
	{
	case ECG_STATE_IDLE:
		break;
		
	case ECG_STATE_DATA_STREAMING:
		for(ECG_Chan_num=0;ECG_Chan_num<3;ECG_Chan_num++)
		{
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = (signed long)SPI_Rx_buf[2*ECG_Chan_num];	// Get MSB Bits15-bits8
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = ADS1x9x_ECG_Data_buf[ECG_Chan_num] << 8;
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] |= SPI_Rx_buf[2*ECG_Chan_num+1];				// Get LSB Bits7-bits0
		}
		ADS1x9x_ECG_Data_buf[0] = ADS1x9x_ECG_Data_buf[0] << 8;				// to make compatable with 24 bit devices
		break;

	case ECG_STATE_ACQUIRE_DATA:
	case ECG_STATE_RECORDING:
		{
			uint8_t *ptr;

			ptr = &ECGRecorder_data_Buf[Recorder_head << 3]; // Point to Circular buffer at head*8;
			*ptr++ = SPI_Rx_buf[0];				// Store status 
			*ptr++ = SPI_Rx_buf[1];				// Store status 

			//			if ((SPI_Rx_buf[2] & 0x80 ) == 0x80)// CH0[15-8] = MSB ( 16Bit device)
			//			*ptr++ = 0xFF;						// CH0[23-16] = 0xFF ( 16Bit device) sign
			//			else
			//			*ptr++ = 0;							// CH0[23-16] = 0 ( 16Bit device)
			*ptr++ = SPI_Rx_buf[2];				// CH0[15-8] = MSB ( 16Bit device)
			*ptr++ = SPI_Rx_buf[3];				// CH0[7-0] = LSB ( 16Bit device)
			*ptr++ = 0;

			//			if ((SPI_Rx_buf[4] & 0x80 ) == 0x80)// CH1[15-8] = MSB ( 16Bit device)
			//			*ptr++ = 0xFF;						// CH1[23-16] = 0xFF ( 16Bit device) sign
			//			else
			//			*ptr++ = 0;							// CH1[23-16] = 0 ( 16Bit device)
			*ptr++ = SPI_Rx_buf[4];				// CH1[15-8] = MSB ( 16Bit device)
			*ptr++ = SPI_Rx_buf[5];				// CH1[7-0] = LSB ( 16Bit device)
			*ptr++ = 0;
			Recorder_head ++;					// Increment Circuler buffer pointer

			if(Recorder_head == 32)				// Check for Circuler buffer depth.
				Recorder_head = 0;				// Rest once it reach to MAX
		}
		break;
       
	default:
		break;
	}
}

void ADS1291_Parse_Data_Packet(void)
{
	uint8_t ECG_Chan_num;

	switch(ECG_Recoder_state.state)
	{		
	case ECG_STATE_DATA_STREAMING:
		for(ECG_Chan_num=0;ECG_Chan_num<2;ECG_Chan_num++)
		{
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = (signed long)SPI_Rx_buf[3*ECG_Chan_num];	// Get Bits23-bits16

			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = ADS1x9x_ECG_Data_buf[ECG_Chan_num] << 8;
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] |= SPI_Rx_buf[3*ECG_Chan_num+1];				// Get Bits15-bits8

			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = ADS1x9x_ECG_Data_buf[ECG_Chan_num] << 8;
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] |= SPI_Rx_buf[3*ECG_Chan_num+2];				// Get Bits7-bits0
		}
		break;

   case ECG_STATE_ACQUIRE_DATA:
   case ECG_STATE_RECORDING:
		{
			uint8_t *ptr;
			
			ptr = &ECGRecorder_data_Buf[Recorder_head<<3]; // Point to Circular buffer at head*8;

			*ptr++ = SPI_Rx_buf[0];				// Store status 
			*ptr++ = SPI_Rx_buf[1];				// Store status 
			//SPI_Rx_buf[2] is always 0x00 so it is discarded

			*ptr++ = SPI_Rx_buf[3];				// CH0[23-16] = MSB ( 24 Bit device)
			*ptr++ = SPI_Rx_buf[4];				// CH0[15-8] = MID ( 24 Bit device)
			*ptr++ = SPI_Rx_buf[5];				// CH0[7-0] = LSB ( 24 Bit device)

			*ptr++ = SPI_Rx_buf[3];				// CH1[23-16] = Ch0 to mentain uniformality
			*ptr++ = SPI_Rx_buf[4];				// CH1[15-8] =  Ch0 to mentain uniformality
			*ptr++ = SPI_Rx_buf[5];				// CH1[7-0] =  Ch0 to mentain uniformality

			Recorder_head++;					// Increment Circuler buffer pointer
			if(Recorder_head == 32)				// Check for Circuler buffer depth.
				Recorder_head = 0;				// Rest once it reach to MAX
		}
		break;
       
	default:
		break;
	}
}

void ADS1292x_Parse_Data_Packet(void)
{
	uint8_t ECG_Chan_num;
	
	switch(ECG_Recoder_state.state)
	{		
	case ECG_STATE_DATA_STREAMING:
		for(ECG_Chan_num=0;ECG_Chan_num<3;ECG_Chan_num++)
		{
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = (signed long)SPI_Rx_buf[3*ECG_Chan_num];

			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = ADS1x9x_ECG_Data_buf[ECG_Chan_num] << 8;
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] |= SPI_Rx_buf[3*ECG_Chan_num+1];

			ADS1x9x_ECG_Data_buf[ECG_Chan_num] = ADS1x9x_ECG_Data_buf[ECG_Chan_num] << 8;
			ADS1x9x_ECG_Data_buf[ECG_Chan_num] |= SPI_Rx_buf[3*ECG_Chan_num+2];
		}
		break;

	case ECG_STATE_ACQUIRE_DATA:
	case ECG_STATE_RECORDING:
		{
			uint8_t *ptr;

			ptr = &ECGRecorder_data_Buf[Recorder_head << 3]; // Point to Circular buffer at head*8;
			*ptr++ = SPI_Rx_buf[0];				// Store status 
			*ptr++ = SPI_Rx_buf[1];				// Store status 
			//SPI_Rx_buf[2] is always 0x00 so it is discarded

			*ptr++ = SPI_Rx_buf[3];				// CH0[23-16] = MSB ( 24 Bit device)
			*ptr++ = SPI_Rx_buf[4];				// CH0[15-8] = MID ( 24 Bit device)
			*ptr++ = SPI_Rx_buf[5];				// CH0[7-0] = LSB ( 24 Bit device)

			*ptr++ = SPI_Rx_buf[6];				// CH1[23-16] = MSB ( 24 Bit device)
			*ptr++ = SPI_Rx_buf[7];				// CH1[15-8] = MID ( 24 Bit device)
			*ptr++ = SPI_Rx_buf[8];				// CH1[7-0] = LSB ( 24 Bit device)

			Recorder_head ++;					// Increment Circuler buffer pointer
			if(Recorder_head == 32)				// Check for circuler buffer depth.
				Recorder_head = 0;				// Rest once it reach to MAX
		}
		break;

	default:
		break;
	}
}

void ADS1x9x_Parse_Data_Packet(void)
{
	switch(ADS1x9xRegVal[0]&0x03)
	{
	case ADS1191_16BIT:
		ADS1191_Parse_Data_Packet();
		break;
	
	case ADS1192_16BIT:
		ADS1192_Parse_Data_Packet();
		break;
	
	case ADS1291_24BIT:
		ADS1291_Parse_Data_Packet();
		break;
	
	case ADS1292_24BIT:
		ADS1292x_Parse_Data_Packet();
		break;
	}
	
	//ECG_Data_rdy = 1;
}

void ADS1x9x_Default_Reg_Init(void)
{
	uint8_t Reg_Init_i;
	
	if((ADS1x9xRegVal[0]&0X20) == 0x20)
	{
		for(Reg_Init_i=1;Reg_Init_i<12; Reg_Init_i++)
		{
			ADS_Reg_Write(Reg_Init_i, ADS1x9xR_Default_Reg_Settings[Reg_Init_i]);
		}
	}
	else
	{
		for(Reg_Init_i=1;Reg_Init_i<12; Reg_Init_i++)
		{
			ADS_Reg_Write(Reg_Init_i, ADS1x9x_Default_Reg_Settings[Reg_Init_i]);
		}
	}
}

void ADS1x9x_Read_All_Regs(uint8_t *ADS1x9xReg_buf)
{
	uint8_t Regs_i;

	for(Regs_i=ADS_REG_ID;Regs_i<ADS_REG_MAX;Regs_i++)
	{
		ADS_Reg_Read(Regs_i, &ADS1x9xReg_buf[Regs_i]);
	#ifdef ADS_DEBUG
		LOGD("reg_%d:%x", Regs_i, ADS1x9xReg_buf[Regs_i]);
	#endif
	}
}

void ADS1x9x_Sensor_Init(void)
{
	uint8_t device_id;

	ADS_Data_Write(ADS_DATA_COM_SDATAC);
	ADS_Sys_Set(ADS_SYS_COM_STOP);
	
	ADS_Reg_Read(ADS_REG_ID, &device_id);
#ifdef ADS_DEBUG	
	LOGD("id:%x", device_id);
#endif
	switch((device_id&0xE0)>>5)
	{
	case 0b000:
	#ifdef ADS_DEBUG	
		LOGD("Reserved 000");
	#endif
		break;
	case 0b001:
	#ifdef ADS_DEBUG	
		LOGD("Reserved 001");
	#endif
		break;
	case 0b010:
	#ifdef ADS_DEBUG
		LOGD("ADS1x9x device");
	#endif
		break;
	case 0b011:
	#ifdef ADS_DEBUG
		LOGD("ADS1292R device");
	#endif
		break;
	case 0b100:
	#ifdef ADS_DEBUG
		LOGD("Reserved 100");
	#endif
		break;
	case 0b101:
	#ifdef ADS_DEBUG
		LOGD("Reserved 101");
	#endif
		break;
	case 0b110:
	#ifdef ADS_DEBUG
		LOGD("Reserved 110");
	#endif
		break;
	case 0b111:
	#ifdef ADS_DEBUG
		LOGD("Reserved 111");
	#endif
		break;
	}

	switch(device_id&0x03)
	{
	case 0b00:
	#ifdef ADS_DEBUG
		LOGD("ADS1191");
	#endif
		break;
	case 0b01:
	#ifdef ADS_DEBUG
		LOGD("ADS1192");
	#endif
		break;
	case 0b10:
	#ifdef ADS_DEBUG
		LOGD("ADS1291");
	#endif
		break;
	case 0b11:
	#ifdef ADS_DEBUG
		LOGD("ADS1292 and ADS1292R");
	#endif
		break;
	}

	ADS1x9x_Read_All_Regs(ADS1x9xRegVal);
	ADS1x9x_Default_Reg_Init();
	ADS1x9x_Read_All_Regs(ADS1x9xRegVal);
	
	ADS1x9x_PowerDown_Enable();
}

void ADS1x9x_Interrupt_Enable(void)
{
	gpio_pin_interrupt_configure(gpio_ecg, ECG_DRDY_PIN, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);
}

void ADS1x9x_Interrupt_Disable(void)
{
	gpio_pin_interrupt_configure(gpio_ecg, ECG_DRDY_PIN, GPIO_INT_DISABLE);
}

bool ADS1x9x_IsLeadOn(void)
{
	uint8_t data;
	
	ADS_Reg_Read(ADS_REG_LOFF_STAT, &data);
	//if(data&0x1c)
	//{
	//}
}

void ADS1x9x_Init(void)
{
	int err;
	gpio_flags_t flag = GPIO_INPUT|GPIO_PULL_UP;

  	//¶Ë¿Ú³õÊ¼»¯
  	gpio_ecg = DEVICE_DT_GET(ECG_PORT);
	if(!gpio_ecg)
	{
		return;
	}

	gpio_pin_configure(gpio_ecg, ECG_RESET_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ecg, ECG_RESET_PIN, 0);
	k_sleep(K_MSEC(50));
	gpio_pin_set(gpio_ecg, ECG_RESET_PIN, 1);
	
	gpio_pin_configure(gpio_ecg, ECG_START_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ecg, ECG_CS_PIN, 0);
	
	gpio_pin_configure(gpio_ecg, ECG_CS_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ecg, ECG_CS_PIN, 1);
	
	//ecg ready interrupt
	gpio_pin_configure(gpio_ecg, ECG_DRDY_PIN, flag);
	gpio_pin_interrupt_configure(gpio_ecg, ECG_DRDY_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb, EcgInterruptHandle, BIT(ECG_DRDY_PIN));
	gpio_add_callback(gpio_ecg, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_ecg, ECG_DRDY_PIN, GPIO_INT_ENABLE|GPIO_INT_EDGE_FALLING);
	
	ECG_SPI_Init();

	ADS1x9x_Sensor_Init();
}

void ADS1x9x_Msg_Process(void)
{
	if(ecg_trige_flag)
	{
		ecg_trige_flag = false;
	}
}
