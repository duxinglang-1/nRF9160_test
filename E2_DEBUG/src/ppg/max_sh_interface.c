/****************************************Copyright (c)************************************************
** File Name:			    max_sh_interface.c
** Descriptions:			max sensor hub interface source file
** Created By:				xie biao
** Created Date:			2021-06-21
** Modified Date:      		2021-06-21 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include "max_sh_interface.h"

#include "max_sh_api.h"
#include "external_flash.h"
#include "settings.h"
#include "lcd.h"
#include "font.h"
#include "logger.h"

//#define MAX_DEBUG

#define MAX630FTHR       1
#define ME15_DEV_MRD103  0
#define ME15_DEV_OB7     0

#if MAX630FTHR
#define SH_RST_PORT                	"GPIO_0"
#define SH_RST_PIN            		16
#define SH_MFIO_PORT           	    "GPIO_0"
#define SH_MFIO_PIN            		14
#elif ME15_DEV_MRD103
#define SH_RST_PORT                 "GPIO_0"
#define SH_RST_PIN            		16
#define SH_MFIO_PORT           	    "GPIO_0"
#define SH_MFIO_PIN            		14
#elif ME15_DEV_OB7
#define SH_RST_PORT                 "GPIO_0"
#define SH_RST_PIN            		16
#define SH_MFIO_PORT           	    "GPIO_0"
#define SH_MFIO_PIN            		14
#endif

#define PPG_DEV 	"I2C_1"
#define PPG_PORT 	"GPIO_0"
	
#define PPG_SDA_PIN		11
#define PPG_SCL_PIN		12
#define PPG_INT_PIN		13
#define PPG_MFIO_PIN	14
#define PPG_RST_PIN		16
#define PPG_EN_PIN		17
#define PPG_I2C_EN_PIN	3

#define MAX32674_I2C_ADD     0x55
	
#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PULL_UP DT_ALIAS_SW0_GPIOS_FLAGS
#else
#define PULL_UP 0
#endif
#define EDGE (GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE)

#define MFIO_LOW_DURATION        1500
#define SS_DEFAULT_RETRIES       ((int) (5))
#define SH_BPT_CAL_COUNT_MAX	(5)

//max size is used for bootloader page loading
#define SS_TX_BUF_SIZE		(BL_MAX_PAGE_SIZE+BL_AES_AUTH_SIZE+BL_FLASH_CMD_LEN)

static struct device *i2c_ppg;
static struct device *gpio_ppg;
static struct gpio_callback gpio_cb;

u8_t sh_write_buf[SS_TX_BUF_SIZE]={0};
u8_t sh_bpt_cal[CAL_RESULT_SIZE]={0};

extern bool ppg_int_event;
extern u8_t g_ppg_bpt_status;
extern u8_t g_ppg_ver[64];

void wait_us(int us)
{
	k_sleep(K_MSEC(1));
}

void wait_ms(int ms)
{
	k_sleep(K_MSEC(ms));
}

void PPG_i2c_on(void)
{
	gpio_pin_configure(gpio_ppg, PPG_I2C_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_I2C_EN_PIN, 1);
}

void PPG_i2c_off(void)
{
	gpio_pin_configure(gpio_ppg, PPG_I2C_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_I2C_EN_PIN, 0);
}

void PPG_Enable(void)
{
	gpio_pin_configure(gpio_ppg, PPG_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_EN_PIN, 1);
}

void PPG_Disable(void)
{
	gpio_pin_configure(gpio_ppg, PPG_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_EN_PIN, 0);
}

static void sh_init_i2c(void)
{
	i2c_ppg = device_get_binding(PPG_DEV);
	if(!i2c_ppg)
	{
	#ifdef MAX_DEBUG
		LOGD("ERROR SETTING UP I2C");
	#endif
	}
	else
	{
		i2c_configure(i2c_ppg, I2C_SPEED_SET(I2C_SPEED_FAST));
	}
}

static void interrupt_event(struct device *interrupt, struct gpio_callback *cb, u32_t pins)
{
	ppg_int_event = true; 
}

static void sh_init_gpio(void)
{
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_DOWN|GPIO_INT_ACTIVE_HIGH|GPIO_INT_DEBOUNCE;

	gpio_ppg = device_get_binding(PPG_PORT);
	
	//interrupt
	gpio_pin_configure(gpio_ppg, PPG_INT_PIN, flag);
	gpio_pin_disable_callback(gpio_ppg, PPG_INT_PIN);
	gpio_init_callback(&gpio_cb, interrupt_event, BIT(PPG_INT_PIN));
	gpio_add_callback(gpio_ppg, &gpio_cb);
	gpio_pin_enable_callback(gpio_ppg, PPG_INT_PIN);

	gpio_pin_configure(gpio_ppg, PPG_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_EN_PIN, 1);

	gpio_pin_configure(gpio_ppg, PPG_I2C_EN_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_ppg, PPG_I2C_EN_PIN, 1);
	
	//PPGÄ£Ê½Ñ¡Ôñ(bootload\application)
	gpio_pin_configure(gpio_ppg, PPG_RST_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_ppg, PPG_MFIO_PIN, GPIO_DIR_OUT);
}

void SH_mfio_to_low_and_keep(int waitDurationInUs)
{
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 0);
	wait_us(waitDurationInUs);
}

void SH_pull_mfio_to_high (void)
{
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 1);
}

void SH_rst_to_BL_mode(void)
{
	//set all high
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 1);
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 1);
	k_sleep(K_MSEC(10));

	//set mfio low
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 0);
	k_sleep(K_MSEC(10));

	//reset sensor hub
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 0);
	k_sleep(K_MSEC(100));
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 1);
	
	//enter bootloader mode
	int s32_status = sh_put_in_bootloader();
	if(s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("set bl mode fail, %x", s32_status);
	#endif
		return;
	}

	k_sleep(K_MSEC(50));
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 1);

#ifdef MAX_DEBUG
	LOGD("set bl mode success!");
#endif
}

void SH_rst_to_APP_mode(void)
{
	//set rst low and mfio high
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 0);
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 1);
	wait_ms(10);

	//set rst high
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 1);
	wait_ms(50);
	
	//enter application mode end delay for initialization finishes
	wait_ms(2000);

#ifdef MAX_DEBUG	
	LOGD("set app mode success!");
#endif
}

void SH_to_APP_from_BL_timing_out(void)
{
	//set rst and mfio low
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 0);
	gpio_pin_write(gpio_ppg, PPG_MFIO_PIN, 0);
	k_sleep(K_MSEC(10));

	//set rst high
	gpio_pin_write(gpio_ppg, PPG_RST_PIN, 1);
	k_sleep(K_MSEC(50));

	//If no I2C commands are sent to the AlgoHub within the next 1s, 
	//then the AlgoHub will automatically switch to application mode.
}

/**
 * @brief	   Read data from BPT sensor
 * @retval	   SS_SUCCESS  if everything is successful, others	communication error
 */
int sh_read_cmd(u8_t *cmd_bytes,
				int cmd_bytes_len,
				u8_t *data,
				int data_len,
				u8_t *rxbuf,
				int rxbuf_sz,
				int sleep_ms )
{

	int retries = SS_DEFAULT_RETRIES;

	SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
	int ret = i2c_write(i2c_ppg, cmd_bytes, cmd_bytes_len, MAX32674_I2C_ADD);
	SH_pull_mfio_to_high();

	while((ret != 0) && (retries-- > 0))
	{
		WAIT_MS(1);

		SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
		ret = i2c_write(i2c_ppg, cmd_bytes, cmd_bytes_len, MAX32674_I2C_ADD);
		SH_pull_mfio_to_high();
	}

	if(retries == 0)
		return SS_ERR_UNAVAILABLE;


	WAIT_MS(sleep_ms);

	SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
	ret = i2c_read(i2c_ppg, rxbuf, rxbuf_sz, MAX32674_I2C_ADD);
	SH_pull_mfio_to_high();

	bool try_again = (rxbuf[0] == SS_ERR_TRY_AGAIN);

	while((ret != 0 || (try_again)) && (retries-- > 0))
	{
		WAIT_MS(sleep_ms);

		SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
		ret = i2c_read(i2c_ppg, rxbuf, rxbuf_sz, MAX32674_I2C_ADD);
		SH_pull_mfio_to_high();

		try_again = (rxbuf[0] == SS_ERR_TRY_AGAIN);
	}

	if(ret != 0 || (rxbuf[0] == SS_ERR_TRY_AGAIN))
		return SS_ERR_UNAVAILABLE;

	if(rxbuf[0] == 0xAA)
	{
		rxbuf[0] = SS_SUCCESS;
	}

	return (int)((SS_STATUS)rxbuf[0]);
}

int sh_write_cmd(u8_t *tx_buf,
				 int tx_len,
				 int sleep_ms)
{
	int retries = SS_DEFAULT_RETRIES;

	SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
	int ret = i2c_write(i2c_ppg, tx_buf, tx_len, MAX32674_I2C_ADD);
	SH_pull_mfio_to_high();

	while((ret != 0) && (retries--) > 0)
	{
		WAIT_MS(1);
		SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
		ret = i2c_write(i2c_ppg, tx_buf, tx_len, MAX32674_I2C_ADD);
		SH_pull_mfio_to_high();
	}

	if((ret != 0) && (retries == 0))
	{
	   return SS_ERR_UNAVAILABLE;
	}

	WAIT_MS(sleep_ms);

	u8_t status_byte;
	SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
	ret = i2c_read(i2c_ppg, (u8_t*)&status_byte, 1, MAX32674_I2C_ADD);
	SH_pull_mfio_to_high();

	bool try_again = (status_byte == SS_ERR_TRY_AGAIN);

	while((ret != 0 || (try_again)) && (retries--) > 0)
	{
		WAIT_MS(sleep_ms);

		SH_mfio_to_low_and_keep(MFIO_LOW_DURATION);
		ret = i2c_read(i2c_ppg, (u8_t*)&status_byte, 1, MAX32674_I2C_ADD);
		SH_pull_mfio_to_high();

		try_again = (status_byte == SS_ERR_TRY_AGAIN);
	}

	if (ret != 0 || try_again)
	{
		return SS_ERR_UNAVAILABLE;
	}

	if (status_byte == 0xAA || status_byte == 0xAB)
	{
		status_byte = SS_SUCCESS;
	}

	return (int)((SS_STATUS)status_byte);
}

int sh_write_cmd_with_data(u8_t *cmd_bytes,
						   int cmd_bytes_len,
						   u8_t *data,
						   int data_len,
						   int cmd_delay_ms)
{
	memcpy(sh_write_buf, cmd_bytes, cmd_bytes_len);
	memcpy(sh_write_buf+cmd_bytes_len, data, data_len);
	int status = sh_write_cmd(sh_write_buf, cmd_bytes_len+data_len, cmd_delay_ms);
	return status;
}

s32_t sh_enter_int_mode(void)
{
	u8_t ByteSeq[] =  {0xB8,0x01};
	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), SS_DEFAULT_CMD_SLEEP_MS);
    return status;
}

s32_t sh_enter_polling_mode(void)
{
	u8_t ByteSeq[] =  {0xB8,0x00};
	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), SS_DEFAULT_CMD_SLEEP_MS);
    return status;
}

s32_t sh_get_sensorhub_operating_mode(u8_t *hubMode)
{

	u8_t ByteSeq[] = {0x02,0x00};
	u8_t rxbuf[2]  = { 0 };

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
			                    0, 0,
			                    &rxbuf[0], sizeof(rxbuf),
								SS_DEFAULT_CMD_SLEEP_MS);

	*hubMode = rxbuf[1];
	return status;
}


s32_t sh_get_hub_fw_version(u8_t* fw_version)
{
	u8_t u8_work_mode;
	u8_t cmd_bytes[2];
	u8_t rxbuf[4];
	u32_t status;

	sh_get_sensorhub_operating_mode(&u8_work_mode);

	if(u8_work_mode > 0)
	{
		cmd_bytes[0] = SS_FAM_R_BOOTLOADER;
		cmd_bytes[1] = SS_CMDIDX_BOOTFWVERSION;
	}
	else if(u8_work_mode == 0)
	{
		cmd_bytes[0] = SS_FAM_R_IDENTITY;
		cmd_bytes[1] = SS_CMDIDX_FWVERSION;
	}
	else
	{
		return status;
	}

	status = sh_read_cmd(&cmd_bytes[0], sizeof(cmd_bytes),
							0, 0,
							&rxbuf[0], sizeof(rxbuf),
							SS_DEFAULT_CMD_SLEEP_MS );

	memcpy(fw_version, &rxbuf[1],sizeof(rxbuf) - 1);
	return status;
}

int sh_set_sensorhub_operating_mode(u8_t hubMode)
{
	u8_t ByteSeq[] = {0x01,0x00,hubMode};
	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), SS_DEFAULT_CMD_SLEEP_MS);
    return status;
}

int sh_get_sensorhub_status(u8_t *hubStatus)
{
	u8_t ByteSeq[] = {0x00,0x00};
	u8_t rxbuf[2]  = { 0 };

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
			                    0, 0,
			                    &rxbuf[0], sizeof(rxbuf),
								SS_DEFAULT_CMD_SLEEP_MS);

	*hubStatus = rxbuf[1];
	return status;
}

int sh_set_data_type(int data_type_, bool sc_en_)
{

	u8_t cmd_bytes[] = { 0x10, 0x00 };
	u8_t data_bytes[] = { (u8_t)((sc_en_ ? SS_MASK_OUTPUTMODE_SC_EN : 0) |
							((data_type_ << SS_SHIFT_OUTPUTMODE_DATATYPE) & SS_MASK_OUTPUTMODE_DATATYPE)) };

	int status = sh_write_cmd_with_data(&cmd_bytes[0], sizeof(cmd_bytes),
								&data_bytes[0], sizeof(data_bytes),
								SS_DEFAULT_CMD_SLEEP_MS);
	return status;
}

int sh_get_data_type(int *data_type_, bool *sc_en_)
{
	u8_t ByteSeq[] = {0x11,0x00};
	u8_t rxbuf[2]  = {0};

	int status = sh_read_cmd( &ByteSeq[0], sizeof(ByteSeq),
							  0, 0,
							  &rxbuf[0], sizeof(rxbuf),
							  SS_DEFAULT_CMD_SLEEP_MS);
	if (status == 0x00 /*SS_SUCCESS*/) {
		*data_type_ =
			(rxbuf[1] & SS_MASK_OUTPUTMODE_DATATYPE) >> SS_SHIFT_OUTPUTMODE_DATATYPE;
		*sc_en_ =
			(bool)((rxbuf[1] & SS_MASK_OUTPUTMODE_SC_EN) >> SS_SHIFT_OUTPUTMODE_SC_EN);

	}

	return status;
}

int sh_set_fifo_thresh(int threshold)
{
	u8_t cmd_bytes[]  = { 0x10 , 0x01 };
	u8_t data_bytes[] = { (u8_t)threshold };

	int status = sh_write_cmd_with_data(&cmd_bytes[0], sizeof(cmd_bytes),
								&data_bytes[0], sizeof(data_bytes),
								SS_DEFAULT_CMD_SLEEP_MS
	                            );
	return status;
}

int sh_get_fifo_thresh(int *thresh)
{
	u8_t ByteSeq[] = {0x11,0x01};
	u8_t rxbuf[2]  = {0};

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
							 0, 0,
							 &rxbuf[0], sizeof(rxbuf),
							 SS_DEFAULT_CMD_SLEEP_MS);

	*thresh = (int) rxbuf[1];
	return status;
}

int sh_ss_comm_check(void)
{
	u8_t ByteSeq[] = {0xFF, 0x00};
	u8_t rxbuf[2];

	int status = sh_read_cmd( &ByteSeq[0], sizeof(ByteSeq),
							  0, 0,
							  &rxbuf[0], sizeof(rxbuf),
							  SS_DEFAULT_CMD_SLEEP_MS );

	int tries = 4;
	while(status == SS_ERR_TRY_AGAIN && tries--)
	{
		WAIT_MS(1000);
		status = sh_read_cmd( &ByteSeq[0], sizeof(ByteSeq),
									  0, 0,
									  &rxbuf[0], sizeof(rxbuf),
									  SS_DEFAULT_CMD_SLEEP_MS);

	}

	return status;
}

int sh_num_avail_samples(int *numSamples)
{

	 u8_t ByteSeq[] = {0x12,0x00};
	 u8_t rxbuf[2]  = {0};

	 int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
							  0, 0,
							  &rxbuf[0], sizeof(rxbuf),
							  1);

	 *numSamples = (int) rxbuf[1];

	 return status;
}

int sh_read_fifo_data(int numSamples, int sampleSize, u8_t* databuf, int databufSz)
{
	int bytes_to_read = numSamples * sampleSize + 1; //+1 for status byte
	u8_t ByteSeq[] = {0x12,0x01};

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
							 0, 0,
							 databuf, bytes_to_read,
							 2);

	return status;
}

int sh_set_reg(int idx, u8_t addr, u32_t val, int regSz)
{
	u8_t ByteSeq[] = { 0x40 , ((u8_t)idx) , addr};
	u8_t data_bytes[4];

	for(int i = 0; i < regSz; i++)
	{
		data_bytes[i] = (val >> (8 * (regSz - 1)) & 0xFF);
	}
	
	int status = sh_write_cmd_with_data( &ByteSeq[0], sizeof(ByteSeq),
							             &data_bytes[0], (u8_t) regSz,
										 SS_DEFAULT_CMD_SLEEP_MS);

    return status;
}

int sh_get_reg(int idx, u8_t addr, u32_t *val)
{
	u32_t i32tmp;
	u8_t ByteSeq[] = { 0x42, ((u8_t) idx)};
	u8_t rxbuf[3]  = {0};

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
								0, 0,
							 &rxbuf[0], sizeof(rxbuf),
							 SS_DEFAULT_CMD_SLEEP_MS);

    if(status == 0x00 /* SS_SUCCESS */)
    {
    	int reg_width = rxbuf[1];
    	u8_t ByteSeq2[] = { 0x41, ((u8_t)idx) , addr} ;
    	u8_t rxbuf2[5]  = {0};

    	status = sh_read_cmd(&ByteSeq2[0], sizeof(ByteSeq2),
    						0, 0,
    						&rxbuf2[0], reg_width + 1,
							SS_DEFAULT_CMD_SLEEP_MS);

    	if(status == 0x00  /* SS_SUCCESS */)
		{
    		i32tmp = 0;
    		for(int i = 0;i < reg_width;i++)
    		{
    			i32tmp = (i32tmp << 8) | rxbuf2[i + 1];
    		}
            *val = i32tmp;
    	}
    }
    else
    {
    #ifdef MAX_DEBUG
    	LOGD("read register wideth fail");
    #endif
    }

    return status;
}

int sh_spi_release()
{
	uint8_t ByteSeq[] =  {SS_FAM_W_SPI_SELECT, SS_CMDIDX_SPI_RELASE};
	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), SS_DEFAULT_CMD_SLEEP_MS);
    return status;
}


int sh_spi_use()
{
	uint8_t ByteSeq[] =  {SS_FAM_W_SPI_SELECT, SS_CMDIDX_SPI_USE};
	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), SS_DEFAULT_CMD_SLEEP_MS);
    return status;
}


int sh_spi_status(uint8_t * spi_status)
{
	uint8_t ByteSeq[] =  {SS_FAM_R_SPI_SELECT};

	uint8_t rxbuf[2]  = { 0 };

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
			                    0, 0,
			                    &rxbuf[0], sizeof(rxbuf),
								SS_DEFAULT_CMD_SLEEP_MS);

	*spi_status = rxbuf[1];
	return status;
}

int sh_sensor_enable_(int idx, int mode, u8_t ext_mode)
{
	u8_t ByteSeq[] = {0x44, (u8_t)idx, (u8_t)mode, ext_mode};

	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), 5 * SS_ENABLE_SENSOR_SLEEP_MS);
    return status;
}

int sh_sensor_disable(int idx)
{
	u8_t ByteSeq[] = {0x44, ((u8_t) idx), 0x00};

	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), SS_ENABLE_SENSOR_SLEEP_MS);
	return status;
}

int sh_get_input_fifo_size(int *fifo_size)
{
	u8_t ByteSeq[] = {0x13,0x01};
	u8_t rxbuf[3]; /* status + fifo size */

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
							  0, 0,
							  rxbuf, sizeof(rxbuf), 2*SS_DEFAULT_CMD_SLEEP_MS);

	*fifo_size = rxbuf[1] << 8 | rxbuf[2];
	return status;
}

int sensorhub_set_algo_operation_mode( const uint8_t algo_op_mode )
{
	uint8_t Temp[1] = { algo_op_mode };
	int status = sh_set_algo_cfg(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, SS_CFGIDX_WHRM_WSPO2_BPT_OPERATION_MODE, &Temp[0], 1);

	return status;
}

int sensorhub_set_algo_submode( const uint8_t algo_op_mode , const uint8_t algo_op_submode )
{
	int status;
	uint8_t Temp[1] = { algo_op_submode };

	if(algo_op_mode == SH_OPERATION_WHRM_MODE)
	{
		status = sh_set_algo_cfg(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, SS_CFGIDX_WHRM_WSPO2_SUITE_ALGO_MODE, &Temp[0], 1);
	}
	else if(algo_op_mode == SH_OPERATION_WHRM_BPT_MODE ||  algo_op_mode == SH_OPERATION_BPT_MODE)
	{
		status = sh_set_algo_cfg(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, CFG_CFGIDX_BPT_ALGO_SUBMODE, &Temp[0], 1);  // DAVID : check here!
	}

	return status;
}

int sensorhub_enable_sensors()
{
	int ret = 0;

	ret = sh_sensor_enable_(SH_SENSORIDX_ACCEL, 1, SH_INPUT_DATA_DIRECT_SENSOR);
	g_algo_sensor_stat.accel_enabled = 1;
	if(0 == ret)
	{
		/* Enabling OS6X with host supplies data */
		ret = sh_sensor_enable_(SH_SENSORIDX_MAX86176, 1, SH_INPUT_DATA_DIRECT_SENSOR);
		g_algo_sensor_stat.max86176_enabled =1;
	}

	return ret;
}

int sensorhub_disable_sensor()
{
	int ret = 0;

	ret = sh_sensor_disable(SH_SENSORIDX_ACCEL);
	g_algo_sensor_stat.accel_enabled = 0;
	if(0 == ret)
	{
		/* Enabling OS6X with host supplies data */
		ret = sh_sensor_disable(SH_SENSORIDX_MAX86176);
		g_algo_sensor_stat.max86176_enabled = 0;
	}

	return ret;
}

int sensorhub_enable_algo(sensorhub_report_mode_t mode , int sh_operation_mode , int sh_algo_submode)
{
	int ret = 0;

	//MYG: setting algo mode will determine which is enabled
	g_algo_sensor_stat.bpt_algo_enabled = 0; //MYG: for MWA test

    //SH_OPERATION_WHRM_MODE here.
	ret = sensorhub_set_algo_operation_mode(sh_operation_mode);
#ifdef MAX_DEBUG	
	LOGD("set algo oper mode Ret:%d", ret);
#endif
	//if(ret < 0)
	//	return -1;  MYG: not define in regular sensorhub release

	ret = sensorhub_set_algo_submode(sh_operation_mode, sh_algo_submode);
#ifdef MAX_DEBUG
	LOGD("set algo submode Ret:%d", ret);
#endif

	ret = sh_enable_algo_(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, (int)mode);
#ifdef MAX_DEBUG
	LOGD("set algo enable Ret:%d", ret);
#endif
	if(ret < 0)
    	return -1;

	if(sh_operation_mode == SH_OPERATION_WHRM_MODE)
	{
		if(SENSORHUB_MODE_BASIC == mode)
		{
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 1;
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2 = 0;
		}
		else
		{
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2 = 1;
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 0;
		}

		g_algo_sensor_stat.bpt_algo_enabled = 0;
		g_algo_sensor_stat.algo_raw_enabled = 0;

	}
	else if(sh_operation_mode == SH_OPERATION_WHRM_BPT_MODE)
	{
		if(SENSORHUB_MODE_BASIC == mode)
		{
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 1;
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2 = 0;
		}
		else
		{
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2 = 1;
			g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 0;
		}

		g_algo_sensor_stat.bpt_algo_enabled = 1;
		g_algo_sensor_stat.algo_raw_enabled = 0;

	}
	else if(sh_operation_mode == SH_OPERATION_RAW_MODE)
	{
		g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 0;
		g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2 = 0;
		g_algo_sensor_stat.bpt_algo_enabled = 0;
		g_algo_sensor_stat.algo_raw_enabled = 1;
	}

	return ret;
}

int sensorhub_disable_algo(void)
{
	int ret = 0;

	ret = sh_disable_algo(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X);

	g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 0;
	g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode2 = 0;
	g_algo_sensor_stat.algo_raw_enabled = 0;
	g_algo_sensor_stat.bpt_algo_enabled = 0;

	return ret;
}

int sh_enable_algo_(int idx, int mode)
{
    u8_t cmd_bytes[] = { 0x52, (u8_t)idx, (u8_t)mode };

	int status = sh_write_cmd_with_data(&cmd_bytes[0], sizeof(cmd_bytes), 0, 0, 50 * SS_ENABLE_SENSOR_SLEEP_MS);

	return status;
}

int sh_disable_algo(int idx)
{
	u8_t ByteSeq[] = { 0x52, ((u8_t) idx) , 0x00};

	int status = sh_write_cmd( &ByteSeq[0],sizeof(ByteSeq), SS_ENABLE_SENSOR_SLEEP_MS );

    return status;
}

int sh_set_algo_cfg(int algo_idx, int cfg_idx, uint8_t *cfg, int cfg_sz)
{
	u8_t ByteSeq[] = { 0x50 , ((u8_t) algo_idx) , ((u8_t) cfg_idx) };

	int status = sh_write_cmd_with_data( &ByteSeq[0], sizeof(ByteSeq),
			                             cfg, cfg_sz,
										 SS_CMD_WAIT_PULLTRANS_MS);
	return status;
}

int sh_get_algo_cfg(int algo_idx, int cfg_idx, u8_t *cfg, int cfg_sz)
{
	u8_t ByteSeq[] = { 0x51 , ((u8_t) algo_idx) , ((u8_t) cfg_idx) };

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
						     0, 0,
							 cfg, cfg_sz,
							 SS_DEFAULT_CMD_SLEEP_MS);
	return status;
}

int sh_feed_to_input_fifo(u8_t *tx_buf, int tx_buf_sz, int *nb_written)
{
	int status;

	u8_t rxbuf[3];
	tx_buf[0] = 0x14;
	tx_buf[1] = 0x00;

	status= sh_read_cmd(tx_buf, tx_buf_sz,
			            0, 0,
			            rxbuf, sizeof(rxbuf), SS_FEEDFIFO_CMD_SLEEP_MS);

	*nb_written = rxbuf[1] * 256 + rxbuf[2];

	return status;
}


int sh_get_num_bytes_in_input_fifo(int *fifo_size)
{
    u8_t ByteSeq[] = {0x13,0x04};
	u8_t rxbuf[3]; /* status + fifo size */

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
							 0, 0,
							 rxbuf, sizeof(rxbuf),
							 2*SS_DEFAULT_CMD_SLEEP_MS);

	*fifo_size = rxbuf[1] << 8 | rxbuf[2];

	return status;
}

int sh_set_report_period(u8_t period)
{
	u8_t cmd_bytes[]  = { SS_FAM_W_COMMCHAN, SS_CMDIDX_REPORTPERIOD };
	u8_t data_bytes[] = { (u8_t)period };

	int status = sh_write_cmd_with_data(&cmd_bytes[0], sizeof(cmd_bytes),
								              &data_bytes[0], sizeof(data_bytes), SS_DEFAULT_CMD_SLEEP_MS );
	return status;
}


int sh_set_sensor_cfg(int sensor_idx, int cfg_idx, u8_t *cfg, int cfg_sz, int sleep_ms)
{
	u8_t cmd_bytes[] = { SS_FAM_W_SENSOR_CONFIG, (u8_t)sensor_idx, (u8_t)cfg_idx };
	int status = sh_write_cmd_with_data(&cmd_bytes[0], sizeof(cmd_bytes),
								 	 	 	 	  cfg, cfg_sz, sleep_ms   );
	return status;
}

int sh_disable_sensor_list(void)
{
	u8_t cmd_bytes[] = { SS_FAM_W_SENSORMODE, 0xFF, 2, SH_SENSORIDX_ACCEL, 0, SH_INPUT_DATA_DIRECT_SENSOR, SH_SENSORIDX_MAX8614X, 0, SH_INPUT_DATA_DIRECT_SENSOR };

	int status = sh_write_cmd_with_data(&cmd_bytes[0], sizeof(cmd_bytes), 0, 0, 5 * SS_ENABLE_SENSOR_SLEEP_MS);

	return status;
}


/*
 * BOOTLOADER RELATED FUNCTIONS
 *
 *
 * */

s32_t sh_exit_from_bootloader(void)
{
	return sh_set_sensorhub_operating_mode(0x00);
}

s32_t sh_put_in_bootloader(void)
{
	return sh_set_sensorhub_operating_mode(0x08);
}

s32_t sh_checkif_bootldr_mode(void)
{
	u8_t hubMode;
	int status = sh_get_sensorhub_operating_mode(&hubMode);
	return (status != SS_SUCCESS)? -1:(hubMode & SS_MASK_MODE_BOOTLDR);
}

s32_t sh_get_bootloader_MCU_tye(u8_t *type)
{
	u8_t ByteSeq[]= { 0xFF, 0x00 };
    u8_t rxbuf[2];

    int status = sh_read_cmd( &ByteSeq[0], sizeof(ByteSeq),
                          0, 0,
                          &rxbuf[0], sizeof(rxbuf),
						  SS_DEFAULT_CMD_SLEEP_MS);
    if (status == 0x00)
    {
    	*type = rxbuf[1];
    }
    else
    {
    	*type = -1;
    }

    return status;
}

s32_t sh_get_bootloader_pagesz(u16_t *pagesz)
{
	u8_t ByteSeq[]= { 0x81, 0x01 };
	u8_t rxbuf[3];
	int sz = 0;

	int status = sh_read_cmd(&ByteSeq[0], sizeof(ByteSeq),
								0, 0,
								&rxbuf[0], sizeof(rxbuf),
								SS_DEFAULT_CMD_SLEEP_MS);
	if(status == 0x00)
	{
		sz = (256*(int)rxbuf[1]) + rxbuf[2];
		if(sz > BL_MAX_PAGE_SIZE )
		{
			sz = -2;
		}
	}

	*pagesz = sz;

	return status;
}

s32_t sh_set_bootloader_numberofpages(u8_t pageCount)
{
    u8_t ByteSeq[] = { 0x80, 0x02 };

    u8_t data_bytes[] = { (u8_t)((pageCount >> 8) & 0xFF), (u8_t)(pageCount & 0xFF) };

    int status = sh_write_cmd_with_data(&ByteSeq[0], sizeof(ByteSeq),
								        &data_bytes[0], sizeof(data_bytes),
										SS_DEFAULT_CMD_SLEEP_MS );

    return status;
}

s32_t sh_set_bootloader_iv(u8_t* iv_bytes)
{
	 u8_t ByteSeq[] = { 0x80, 0x00 };

	 int status = sh_write_cmd_with_data( &ByteSeq[0], sizeof(ByteSeq),
			                              &iv_bytes[0], BL_AES_NONCE_SIZE ,
										  SS_DEFAULT_CMD_SLEEP_MS
										  );

     return status;
}


s32_t sh_set_bootloader_auth(u8_t* auth_bytes)
{
	 u8_t ByteSeq[] = { 0x80, 0x01 };

	 int status = sh_write_cmd_with_data( &ByteSeq[0], sizeof(ByteSeq),
			                              &auth_bytes[0], BL_AES_AUTH_SIZE,
										  SS_DEFAULT_CMD_SLEEP_MS
										  );

     return status;
}


s32_t sh_set_bootloader_partial_write_size(u32_t partial_size)
{
    u8_t ByteSeq[] = { 0x80, 0x06 };
    u8_t data_bytes[] = { (u8_t)((partial_size >> 8) & 0xFF), (u8_t)(partial_size & 0xFF) };

    int status = sh_write_cmd_with_data(&ByteSeq[0], sizeof(ByteSeq),
								        &data_bytes[0], sizeof(data_bytes),
										SS_DEFAULT_CMD_SLEEP_MS );

    return status;
}


s32_t sh_set_bootloader_erase(void)
{
	u8_t ByteSeq[] = { 0x80, 0x03 };

    int status = sh_write_cmd_with_data(&ByteSeq[0], sizeof(ByteSeq),
                                        0, 0,
										BL_ERASE_DELAY);

    return status;
}


#ifdef SH_OTA_DATA_STORE_IN_FLASH
u8_t Fw_data[BL_FLASH_PARTIAL_SIZE];
s32_t sh_set_bootloader_flashpages(u32_t FwData_addr, u8_t u8_pageSize)
{
	s32_t status = -1;
    u8_t ByteSeq[] = { 0x80, 0x04};
    u32_t u32_dataIdx = 0;
    u32_t u32_pageDataLen = (BL_MAX_PAGE_SIZE+BL_AES_AUTH_SIZE);

	for(u32_t i = 0; i < u8_pageSize; i++)
	{
		u32_dataIdx = BL_ST_PAGE_IDEX + i * u32_pageDataLen;

		for(u32_t j = 0; j < (1+(8000/BL_FLASH_PARTIAL_SIZE)); j++)
		{
			u32_t part_index,part_len;

			part_index = BL_FLASH_PARTIAL_SIZE*j;
			part_len = BL_FLASH_PARTIAL_SIZE;
			if(j == (8000/BL_FLASH_PARTIAL_SIZE))
				part_len = 208;
			
			SpiFlash_Read(Fw_data, FwData_addr+u32_dataIdx+part_index, part_len);
			status = sh_write_cmd_with_data(&ByteSeq[0], sizeof(ByteSeq),
										  Fw_data, part_len,
										  BL_PAGE_W_DLY_TIME
										  );

			if(status != SS_SUCCESS)
			{
			#ifdef MAX_DEBUG
				LOGD("Write page %d part %d data FW fail: %x", i, j, status);
			#endif
				return status;
			}
		}
	#ifdef MAX_DEBUG
		LOGD("write page %d data done!", i);
	#endif
	}
	return status;
}

#else

s32_t sh_set_bootloader_flashpages(u8_t *u8p_FwData , u8_t u8_pageSize)
{
	s32_t status = -1;
    u8_t ByteSeq[] = { 0x80, 0x04 };
    u32_t u32_dataIdx = 0;
    u32_t u32_pageDataLen = (BL_MAX_PAGE_SIZE+BL_AES_AUTH_SIZE);

	for(u32_t i = 0; i < u8_pageSize; i++)
	{
		u32_dataIdx = BL_ST_PAGE_IDEX + i * u32_pageDataLen;

		status = sh_write_cmd_with_data( &ByteSeq[0], sizeof(ByteSeq),
										  &u8p_FwData[u32_dataIdx], u32_pageDataLen,
										  BL_PAGE_W_DLY_TIME
										  );

		if (status != SS_SUCCESS)
		{
		#ifdef MAX_DEBUG
			LOGD("Write page %d data FW fail: %x", i,  status);
		#endif
			return status;
		}
	#ifdef MAX_DEBUG
		LOGD("write  page %d data done", i);
	#endif
	}
	return status;
}
#endif

/* **********************************************************************************************
 * 																							   	*
 *   					   COMMAND INTERFACE for MAX86141    								 	*
 *																								*
 * **********************************************************************************************/
int SH_Max8614x_set_ppgreg(const u8_t addr, const u32_t val)
{
	int status = sh_set_reg(SH_SENSORIDX_MAX8614X, addr, val, 1);
	return status;
}


int SH_Max8614x_get_ppgreg(const u8_t addr , u32_t *regVal)
{
    int status = sh_get_reg(SH_SENSORIDX_MAX8614X, (u8_t) addr, regVal);
	return status;
}

int sh_set_cfg_wearablesuite_aecenable(const u8_t isAecEnable )
{
	u8_t Temp[1] = { isAecEnable };
    int status = sh_set_algo_cfg(SS_ALGOIDX_WHRM_WSPO2_SUITE, SS_CFGIDX_WHRM_WSPO2_SUITE_AEC_ENABLE, &Temp[0], 1);
	return status;

}

int sh_get_cfg_wearablesuite_aecenable( u8_t *isAecEnable )
{
	u8_t rxBuff[1+1]; // first byte is status
	int status = sh_set_algo_cfg(SS_ALGOIDX_WHRM_WSPO2_SUITE, SS_CFGIDX_WHRM_WSPO2_SUITE_AEC_ENABLE, &rxBuff[0], sizeof(rxBuff) );
	*isAecEnable =  rxBuff[1];

	return status;
}

/*
 *  Sensorhub Authentication Related Functions
 *
 *
 */
int sh_get_dhparams( u8_t *response, int response_sz)
{
	u8_t cmd_bytes[] = { SS_FAM_R_INITPARAMS , (u8_t) 0x00 };

	int status = sh_read_cmd(&cmd_bytes[0], sizeof(cmd_bytes),
								0, 0,
								response, response_sz, SS_DEFAULT_CMD_SLEEP_MS);

	return status;
}

int sh_set_dhlocalpublic(  u8_t *response , int response_sz)
{
	u8_t cmd_bytes[] = { SS_FAM_W_DHPUBLICKEY, (u8_t) 0x00 };
	int status = sh_write_cmd_with_data(&cmd_bytes[0], sizeof(cmd_bytes),
								        response, response_sz, SS_DEFAULT_CMD_SLEEP_MS);
	return status;
}


int sh_get_dhremotepublic( u8_t *response, int response_sz)
{
	u8_t cmd_bytes[] = { SS_FAM_R_DHPUBLICKEY , (u8_t) 0x00 };

	int status = sh_read_cmd(&cmd_bytes[0], sizeof(cmd_bytes),
											0, 0,
											response, response_sz, SS_DEFAULT_CMD_SLEEP_MS);

	return status;
}

int sh_get_authentication( u8_t *response, int response_sz)
{
//	const int auth_cfg_sz = 32; // fixed to 32 bytes

	u8_t cmd_bytes[] = { SS_FAM_R_AUTHSEQUENCE , (u8_t) 0x00 };

	int status = sh_read_cmd(&cmd_bytes[0], sizeof(cmd_bytes),
								   0, 0,
								   response, response_sz, SS_DEFAULT_CMD_SLEEP_MS);

	return status;
}

bool sh_init_interface(void)
{
	s32_t s32_status;
	u8_t u8_rxbuf[3] = {0};
	u8_t mcu_type;

	sh_init_i2c();
	sh_init_gpio();

	SH_rst_to_APP_mode();
	//SH_rst_to_BL_mode();

	//check MCU type
	s32_status = sh_get_bootloader_MCU_tye(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("Read MCU type fail, %x", s32_status);
	#endif

		PPG_i2c_off();
		PPG_Disable();
		PPG_Power_Off();
		return false;
	}
#ifdef MAX_DEBUG	
	LOGD("MCU type = %d", u8_rxbuf[0]);
#endif
	mcu_type = u8_rxbuf[0];
	
	s32_status = sh_get_hub_fw_version(u8_rxbuf);
	if (s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("read FW version fail %x", s32_status);
	#endif
	
		PPG_i2c_off();
		PPG_Disable();
		PPG_Power_Off();
		return false;
	}
	else
	{
		sprintf(g_ppg_ver, "%d.%d.%d", u8_rxbuf[0], u8_rxbuf[1], u8_rxbuf[2]);
	#ifdef MAX_DEBUG
		LOGD("FW version is:%s", g_ppg_ver);
	#endif
	}

	if((mcu_type != 1) || (u8_rxbuf[1] != 4))
	{
	#ifdef FONTMAKER_UNICODE_FONT
		LCD_SetFontSize(FONT_SIZE_20);
	#else	
		LCD_SetFontSize(FONT_SIZE_16);
	#endif
		NotifyShowStrings((LCD_WIDTH-180)/2, (LCD_HEIGHT-120)/2, 180, 120, NULL, 0, "PPG is upgrading firmware, please wait a few minutes!");
		SH_OTA_upgrade_process();
		LCD_SleepOut();
	}

	PPG_i2c_off();
	PPG_Disable();
	PPG_Power_Off();	
	return true;
}


void sh_get_BL_version(void)
{
	s32_t s32_status;
	u8_t u8_rxbuf[3] = {0,0,0};

	SH_rst_to_BL_mode();

	//check MCU type
	s32_status = sh_get_bootloader_MCU_tye(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("Read MCU type fail, %x", s32_status);
	#endif
	}
#ifdef MAX_DEBUG	
	LOGD("MCU type = %d", u8_rxbuf[0]);
#endif

	s32_status = sh_get_hub_fw_version(u8_rxbuf);
	if (s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("read FW version fail %x", s32_status);
	#endif
	}
	else
	{
	#ifdef MAX_DEBUG
		LOGD("FW version is %d.%d.%d", u8_rxbuf[0], u8_rxbuf[1], u8_rxbuf[2]);
	#endif
	}

	u32_t u32_sensorID = 0;
	s32_status = sh_get_reg(0x0, 0xFF, &u32_sensorID);
	if(s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("read MAX86141 ID fail, %x", s32_status);
	#endif
	}
	else
	{
	#ifdef MAX_DEBUG
		LOGD("sensor MAX86141 ID = %x", u32_sensorID);
	#endif
	}

	u32_t u32_accSensorId = 0;
	s32_status = sh_get_reg(SH_SENSORIDX_ACCEL, 0xF, &u32_accSensorId);
	if (s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("read acc sensor ID fail %x", s32_status);
	#endif
	}
	else
	{
	#ifdef MAX_DEBUG
		LOGD("acc sensor ID is %x", u32_accSensorId);
	#endif
	}
}

void sh_get_APP_version(void)
{
	s32_t s32_status;
	u8_t u8_rxbuf[3];

	SH_rst_to_APP_mode();

	//check MCU type
	s32_status = sh_get_bootloader_MCU_tye(u8_rxbuf);
	if(s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("Read MCU type fail, %x", s32_status);
	#endif
	}
#ifdef MAX_DEBUG
	LOGD("MCU type = %d", u8_rxbuf[0]);
#endif

	s32_status = sh_get_hub_fw_version(u8_rxbuf);
	if (s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("read FW version fail %x", s32_status);
	#endif
	}
	else
	{
	#ifdef MAX_DEBUG
		LOGD("FW version is %d.%d.%d", u8_rxbuf[0], u8_rxbuf[1], u8_rxbuf[2]);
	#endif
	}

	u32_t u32_sensorID = 0;
	s32_status = sh_get_reg(0x0, 0xFF, &u32_sensorID);
	if(s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("read MAX86141 ID fail, %x", s32_status);
	#endif
	}
	else
	{
	#ifdef MAX_DEBUG
		LOGD("sensor MAX86141 ID = %x", u32_sensorID);
	#endif
	}

	u32_t u32_accSensorId = 0;
	s32_status = sh_get_reg(SH_SENSORIDX_ACCEL, 0xF, &u32_accSensorId);
	if (s32_status != SS_SUCCESS)
	{
	#ifdef MAX_DEBUG
		LOGD("read acc sensor ID fail %x", s32_status);
	#endif
	}
	else
	{
	#ifdef MAX_DEBUG
		LOGD("acc sensor ID is %x", u32_accSensorId);
	#endif
	}
}

bool sh_check_bpt_cal_data(void)
{
	SpiFlash_Read(sh_bpt_cal, PPG_BPT_CAL_DATA_ADDR, PPG_BPT_CAL_DATA_SIZE);

	if((sh_bpt_cal[0] == 0x4f)&&(sh_bpt_cal[1] == 0x3c)&&(sh_bpt_cal[2] == 0x34)&&(sh_bpt_cal[3] == 0x01))
		return true;
	else
		return false;
}

void sh_get_bpt_cal_data(void)
{
	int status;
	
	status = sh_get_cfg_bpt_cal_result(&sh_bpt_cal);
	SpiFlash_Write(sh_bpt_cal, PPG_BPT_CAL_DATA_ADDR, PPG_BPT_CAL_DATA_SIZE);
}

void sh_req_bpt_cal_data(void)
{
	int status;

	//Enable AEC
	status = sh_set_cfg_wearablesuite_afeenable(1);
#ifdef MAX_DEBUG
	LOGD("enabel aec ret:%d", status);
#endif
	//Enable automatic calculation of target PD current
	status = sh_set_cfg_wearablesuite_autopdcurrentenable(1);
#ifdef MAX_DEBUG
	LOGD("enabel auto pd cur ret:%d", status);
#endif
	//Set minimum PD current to 12.5uA
	status = sh_set_cfg_wearablesuite_minpdcurrent(125);
#ifdef MAX_DEBUG
	LOGD("set min pd cur ret:%d", status);
#endif
	//Set initial PD current to 31.2uA
	status = sh_set_cfg_wearablesuite_initialpdcurrent(312);
#ifdef MAX_DEBUG
	LOGD("set init pd cur ret:%d", status);
#endif
	//Set target PD current to 31.2uA
	status = sh_set_cfg_wearablesuite_targetpdcurrent(312);
#ifdef MAX_DEBUG
	LOGD("set target pd cur ret:%d", status);
#endif
	//Enable SCD
	status = sh_set_cfg_wearablesuite_scdenable(1);
#ifdef MAX_DEBUG
	LOGD("enable scd ret:%d", status);
#endif
	//Set the output format to Sample Counter byte, Sensor Data and Algorithm
	status = sh_set_data_type(SS_DATATYPE_BOTH, true);
#ifdef MAX_DEBUG
	LOGD("set output format ret:%d", status);
#endif
	//set fifo thresh
	status = sh_set_fifo_thresh(1);
#ifdef MAX_DEBUG
	LOGD("set fifo thresh ret:%d", status);	
#endif
	//Set the samples report period to 40ms(minimum is 32ms for BPT).
	status = sh_set_report_period(25);
#ifdef MAX_DEBUG
	LOGD("set samples period ret:%d", status);	
#endif
	//Enable the sensor.
	status = sensorhub_enable_sensors();
#ifdef MAX_DEBUG
	LOGD("enabel sensor ret:%d", status);
#endif
	//set algo mode
	status = sh_set_cfg_wearablesuite_algomode(0x0);
#ifdef MAX_DEBUG
	LOGD("set algo mode Ret:%d", status);
#endif
	//set algo oper mode
	status = sensorhub_set_algo_operation_mode(SH_OPERATION_WHRM_BPT_MODE);
	g_algo_sensor_stat.bpt_algo_enabled = 1;
#ifdef MAX_DEBUG
	LOGD("set algo oper mode Ret:%d", status);
#endif
	//Set the cal_index, reference systolic, reference diastolic
	status = sh_set_cfg_bpt_sys_dia(0, global_settings.bp_calibra.systolic, global_settings.bp_calibra.diastolic);
#ifdef MAX_DEBUG
	LOGD("set cal_index sys&dia ret:%d", status);
#endif
	//set algo submode
	status = sensorhub_set_algo_submode(SH_OPERATION_WHRM_BPT_MODE, SH_BPT_MODE_CALIBCATION);
#ifdef MAX_DEBUG
	LOGD("set algo submode Ret:%d", status);
#endif
	//enable algo
	status = sh_enable_algo_(SS_ALGOIDX_WHRM_WSPO2_SUITE_OS6X, SENSORHUB_MODE_BASIC);
	g_algo_sensor_stat.whrm_wspo2_suite_enabled_mode1 = 1;
#ifdef MAX_DEBUG
	LOGD("set algo enable Ret:%d", status);
#endif
}

void sh_set_bpt_cal_data(void)
{
	int status;

	//Set the the cal_index to 0.
	status = sh_set_cfg_bpt_cal_index(0);
#ifdef MAX_DEBUG
	LOGD("Set the the cal_index to 0 ret:%d", status);
#endif
	//Set the BPT user calibration vector (using cal_index 0).
	status = sh_set_cfg_bpt_cal_result(&sh_bpt_cal[0*CAL_RESULT_SIZE]);
#ifdef MAX_DEBUG
	LOGD("Set the BPT user calibration vector ret:%d", status);
#endif

#if 0
	//Set the the cal_index to 1.
	status = sh_set_cfg_bpt_cal_index(1);
#ifdef MAX_DEBUG
	LOGD("Set the the cal_index to 1 ret:%d", status);
#endif
	//Set the BPT user calibration vector (using cal_index 1).
	status = sh_set_cfg_bpt_cal_result(&sh_bpt_cal[0*CAL_RESULT_SIZE]);
#ifdef MAX_DEBUG
	LOGD("Set the BPT user calibration vector ret:%d", status);
#endif
	//Set the the cal_index to 2.
	status = sh_set_cfg_bpt_cal_index(2);
#ifdef MAX_DEBUG
	LOGD("Set the the cal_index to 2 ret:%d", status);
#endif
	//Set the BPT user calibration vector (using cal_index 2).
	status = sh_set_cfg_bpt_cal_result(&sh_bpt_cal[0*CAL_RESULT_SIZE]);
#ifdef MAX_DEBUG
	LOGD("Set the BPT user calibration vector ret:%d", status);
#endif
	//Set the the cal_index to 3.
	status = sh_set_cfg_bpt_cal_index(3);
#ifdef MAX_DEBUG
	LOGD("Set the the cal_index to 3 ret:%d", status);
#endif
	//Set the BPT user calibration vector (using cal_index 3).
	status = sh_set_cfg_bpt_cal_result(&sh_bpt_cal[0*CAL_RESULT_SIZE]);
#ifdef MAX_DEBUG
	LOGD("Set the BPT user calibration vector ret:%d", status);
#endif
	//Set the the cal_index to 4.
	status = sh_set_cfg_bpt_cal_index(4);
#ifdef MAX_DEBUG
	LOGD("Set the the cal_index to 4 ret:%d", status);
#endif
	//Set the BPT user calibration vector (using cal_index 4).
	status = sh_set_cfg_bpt_cal_result(&sh_bpt_cal[0*CAL_RESULT_SIZE]);	
#ifdef MAX_DEBUG
	LOGD("Set the BPT user calibration vector ret:%d", status);
#endif
#endif	
}
