/****************************************Copyright (c)************************************************
** File Name:			    audio.c
** Descriptions:			audio process source file
** Created By:				xie biao
** Created Date:			2021-03-04
** Modified Date:      		2021-05-08 
** Version:			    	V1.1
******************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <nrfx.h>
#include <nrfx_i2s.h>
//#include "audio_wav.h"
//#include "external_flash.h"
#include "audio.h"
#include "logger.h"

#if 0
#define I2S_MCK		25
#define I2S_LRCK	13
#define I2S_BCK		14
#define I2S_DI		16
#define I2S_DO		17

#define I2S_DATA_BLOCK_WORDS    512
static u32_t m_buffer_rx[2][I2S_DATA_BLOCK_WORDS];
static u32_t m_buffer_tx[2][I2S_DATA_BLOCK_WORDS];

// Delay time between consecutive I2S transfers performed in the main loop
// (in milliseconds).
#define PAUSE_TIME          500
// Number of blocks of data to be contained in each transfer.
#define BLOCKS_TO_TRANSFER  20

static u8_t volatile m_blocks_transferred     = 0;
static u8_t          m_zero_samples_to_ignore = 0;
static u16_t         m_sample_value_to_send;
static u16_t         m_sample_value_expected;
static bool             m_error_encountered;

static u32_t       * volatile mp_block_to_fill  = NULL;
static u32_t const * volatile mp_block_to_check = NULL;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define AUDIO_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define AUDIO_PORT	""
#endif

#define WTN_DATA	13      //接 13脚
#define WTN_BUSY	14		//busy脚,音频播放之后由低变高

static bool audio_trige_flag = false;

static struct device *gpio_audio;
static struct gpio_callback gpio_cb;

//延时函数
void Delay_ms(unsigned int dly)
{
	k_sleep(dly);
}

void Delay_us(unsigned int dly)
{
	k_usleep(dly);
}

//发送一个字节数据
void Audio_Send_ByteData(u8_t data)
{
	u8_t j;
	
	gpio_pin_write(gpio_audio, WTN_DATA, 0);
	Delay_ms(5);
	
	for(j=0;j<8;j++)
	{
		if(data&0x01)
		{
			gpio_pin_write(gpio_audio, WTN_DATA, 1);
			Delay_us(600);
			gpio_pin_write(gpio_audio, WTN_DATA, 0);
			Delay_us(200);
		}
		else
		{
			gpio_pin_write(gpio_audio, WTN_DATA, 1);
			Delay_us(200);
			gpio_pin_write(gpio_audio, WTN_DATA, 0);
			Delay_us(600);
			
		}
		data >>= 1;
	}
	
	gpio_pin_write(gpio_audio, WTN_DATA, 1);
}

//控制音量
void Volume_Control(unsigned char vol)  //E0  ------  EF
{
	Audio_Send_ByteData(vol);
	Delay_us(400);
}

//播放语音
void Voice_Start(u8_t voice_addr)  //0  -----------  6
{
	Audio_Send_ByteData(voice_addr);
}

//停止播放
void Voice_Stop(void)
{
	Audio_Send_ByteData(0xFE);
}

//循环播放当前语音
void Voice_Loop(void)
{	
	Delay_us(400);
	Audio_Send_ByteData(0xF2);
}

//播放120报警声
void audio_play_alarm(void)
{
	Voice_Start(3);
}

//播放中文语音提示
void audio_play_chn_voice(void)
{
	Voice_Start(1);
}

//播放英文语音提示
void audio_play_en_voice(void)
{
	Voice_Start(2);
}

//停止播放
void audio_stop(void)
{
	Voice_Stop();
}

//SOS停止播放报警
void SOSStopAlarm(void)
{
	Voice_Stop();
}

//SOS播放报警
void SOSPlayAlarm(void)
{
	Voice_Start(3);
}

//摔倒停止播放报警
void FallStopAlarm(void)
{
	Voice_Stop();
}

//摔倒播放中文报警
void FallPlayAlarmCn(void)
{
	Voice_Start(1);
}

//摔倒播放英文报警
void FallPlayAlarmEn(void)
{
	Voice_Start(2);
}

void AudioInterruptHandle(void)
{
	LOGD("begin");
	audio_trige_flag = true;
}

//io口初始化 
void audio_init(void)
{
	int flag = GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE|GPIO_PUD_PULL_UP|GPIO_INT_ACTIVE_HIGH|GPIO_INT_DEBOUNCE;
	
	Set_Audio_Power_On();
	
	gpio_audio = DEVICE_DT_GET(AUDIO_PORT);
	
	gpio_pin_configure(gpio_audio, WTN_DATA, GPIO_DIR_OUT);
	gpio_pin_write(gpio_audio, WTN_DATA, 1);

	//busy interrupt
	gpio_pin_configure(gpio_audio, WTN_BUSY, flag);
	gpio_pin_disable_callback(gpio_audio, WTN_BUSY);
	gpio_init_callback(&gpio_cb, AudioInterruptHandle, BIT(WTN_BUSY));
	gpio_add_callback(gpio_audio, &gpio_cb);
	gpio_pin_enable_callback(gpio_audio, WTN_BUSY);

	Delay_ms(100);

	//Set_Audio_Power_Off();
}

void AudioMsgProcess(void)
{
	if(audio_trige_flag)
	{
		audio_trige_flag = false;
	}
}

#if 0
void audio_get_wav_info(u32_t aud_addr, wav_riff_chunk *info_riff, wav_fmt_chunk *info_fmt, wav_data_chunk *info_data)
{
	u32_t index;
	wav_riff_chunk riff_data = {0};
	wav_fmt_chunk fmt_data = {0};
	wav_data_chunk pcm_data = {0};

	index = aud_addr;
	SpiFlash_Read((u8_t*)&riff_data, index, (u32_t)sizeof(wav_riff_chunk));
	if(memcmp(riff_data.riff_mark, WAV_RIFF_ID, 4) != 0)
	{
		LOGD("get wav riff fail!");
		return;
	}

	index += sizeof(wav_riff_chunk);
	SpiFlash_Read((u8_t*)&fmt_data, index, (u32_t)sizeof(wav_fmt_chunk));

	index += (fmt_data.fmt_size+8);
	while(1)
	{
		SpiFlash_Read((u8_t*)&pcm_data, index, (u32_t)sizeof(wav_data_chunk));
		if(strcmp(pcm_data.data_mark, "data") == 0)
			break;
		else
			index += (pcm_data.data_size+8);
	}

	if(info_riff)
	{
		memcpy(info_riff, (void*)&riff_data, (u32_t)sizeof(wav_riff_chunk));
	}
	
	if(info_fmt)
	{
		memcpy(info_fmt, (void*)&fmt_data, (u32_t)sizeof(wav_fmt_chunk));
	}
	
	if(info_data)
	{
		memcpy(info_data, (void*)&pcm_data, (u32_t)sizeof(wav_data_chunk));
	}

	LOGD("get audio wav success");
}

void audio_get_wav_pcm_info(u32_t aud_addr, u32_t *pcmaddr, u32_t *pcmlen)
{
	u32_t index;
	wav_riff_chunk riff_data = {0};
	wav_fmt_chunk fmt_data = {0};
	wav_data_chunk pcm_data = {0};

	index = aud_addr;
	SpiFlash_Read((u8_t*)&riff_data, index, (u32_t)sizeof(wav_riff_chunk));
	if(memcmp(riff_data.riff_mark, WAV_RIFF_ID, 4) != 0)
	{
		LOGD("get wav riff fail");
		*pcmaddr = 0;
		return;
	}

	index += sizeof(wav_riff_chunk);
	SpiFlash_Read((u8_t*)&fmt_data, index, (u32_t)sizeof(wav_fmt_chunk));

	index += (fmt_data.fmt_size+8);

	while(1)
	{
		SpiFlash_Read((u8_t*)&pcm_data, index, (u32_t)sizeof(wav_data_chunk));
		if(strcmp(pcm_data.data_mark, "data") == 0)
			break;
		else
			index += (pcm_data.data_size+8);
	}

	*pcmaddr = (index-aud_addr)+8;
	*pcmlen = pcm_data.data_size;
}

u32_t audio_get_wav_pcm_data(u32_t aud_addr, u8_t *buffer, u32_t start, u32_t buflen)
{
	u32_t pcm_addr,pcm_len,read_len;
	
	audio_get_wav_pcm_info(aud_addr, &pcm_addr, &pcm_len);
		
	if(start >= pcm_len)
	{
		return 0;
	}
	
	read_len = buflen;
	if((start+read_len) > pcm_len)
		read_len = pcm_len-start;

	SpiFlash_Read(buffer, (aud_addr+pcm_addr+start), read_len);

	return read_len;
}

void audio_get_wav_size(u32_t aud_addr, u32_t *filelen)
{
	wav_riff_chunk riff_data = {0};
	
	audio_get_wav_info(aud_addr, &riff_data, NULL, NULL);

	if(riff_data.wav_size > 0)
		*filelen = (riff_data.wav_size+8);
	else
		*filelen = 0;
}

void audio_get_wav_time(u32_t aud_addr, u32_t *time)
{
	wav_fmt_chunk fmt_data = {0};
	wav_data_chunk pcm_data = {0};

	audio_get_wav_info(aud_addr, NULL, &fmt_data, &pcm_data);

	if(pcm_data.data_size > 0)
		*time = pcm_data.data_size/fmt_data.byte_freq;
	else
		*time = 0;
}

void test_audio_wav(void)
{
	u32_t pcmaddr,pcmlen,filelen,time;
	wav_riff_chunk riff_data = {0};
	wav_fmt_chunk fmt_data = {0};
	wav_data_chunk pcm_data = {0};

	audio_get_wav_info(AUDIO_FALL_ALARM_ADDR, &riff_data, &fmt_data, &pcm_data);
	audio_get_wav_pcm_info(AUDIO_FALL_ALARM_ADDR, &pcmaddr, &pcmlen);
	audio_get_wav_size(AUDIO_FALL_ALARM_ADDR, &filelen);
	audio_get_wav_time(AUDIO_FALL_ALARM_ADDR, &time);
	
	LOGD("get audio wav");
}

ISR_DIRECT_DECLARE(i2s_isr_handler)
{
	nrfx_i2s_irq_handler();
	ISR_DIRECT_PM();
	return 1;
}

static void prepare_tx_data(u32_t * p_block)
{
    // These variables will be both zero only at the very beginning of each
    // transfer, so we use them as the indication that the re-initialization
    // should be performed.
    if (m_blocks_transferred == 0 && m_zero_samples_to_ignore == 0)
    {
        // Number of initial samples (actually pairs of L/R samples) with zero
        // values that should be ignored - see the comment in 'check_samples'.
        m_zero_samples_to_ignore = 2;
        m_sample_value_to_send   = 0xCAFE;
        m_sample_value_expected  = 0xCAFE;
        m_error_encountered      = false;
    }

    // [each data word contains two 16-bit samples]
    uint16_t i;
    for (i = 0; i < I2S_DATA_BLOCK_WORDS; ++i)
    {
        uint16_t sample_l = m_sample_value_to_send - 1;
        uint16_t sample_r = m_sample_value_to_send + 1;
        ++m_sample_value_to_send;

        uint32_t * p_word = &p_block[i];
        ((uint16_t *)p_word)[0] = sample_l;
        ((uint16_t *)p_word)[1] = sample_r;
    }
}


static bool check_samples(uint32_t const * p_block)
{
    // [each data word contains two 16-bit samples]
    uint16_t i;
    for (i = 0; i < I2S_DATA_BLOCK_WORDS; ++i)
    {
        uint32_t const * p_word = &p_block[i];
        uint16_t actual_sample_l = ((uint16_t const *)p_word)[0];
        uint16_t actual_sample_r = ((uint16_t const *)p_word)[1];

        // Normally a couple of initial samples sent by the I2S peripheral
        // will have zero values, because it starts to output the clock
        // before the actual data is fetched by EasyDMA. As we are dealing
        // with streaming the initial zero samples can be simply ignored.
        if (m_zero_samples_to_ignore > 0 &&
            actual_sample_l == 0 &&
            actual_sample_r == 0)
        {
            --m_zero_samples_to_ignore;
        }
        else
        {
            m_zero_samples_to_ignore = 0;

            uint16_t expected_sample_l = m_sample_value_expected - 1;
            uint16_t expected_sample_r = m_sample_value_expected + 1;
            ++m_sample_value_expected;

            if (actual_sample_l != expected_sample_l ||
                actual_sample_r != expected_sample_r)
            {
                LOGD("%3u: %04x/%04x, expected: %04x/%04x (i: %u)",
                    m_blocks_transferred, actual_sample_l, actual_sample_r,
                    expected_sample_l, expected_sample_r, i);
                return false;
            }
        }
    }
    LOGD("%3u: OK", m_blocks_transferred);
    return true;
}


static void check_rx_data(uint32_t const * p_block)
{
    ++m_blocks_transferred;

    if (!m_error_encountered)
    {
        m_error_encountered = !check_samples(p_block);
    }

    if (m_error_encountered)
    {
        //bsp_board_led_off(LED_OK);
        //bsp_board_led_invert(LED_ERROR);
    }
    else
    {
        //bsp_board_led_off(LED_ERROR);
        //bsp_board_led_invert(LED_OK);
    }
}

static void data_handler(nrfx_i2s_buffers_t const * p_released,
                         uint32_t                      status)
{
	nrfx_err_t ret;
	
    // 'nrf_drv_i2s_next_buffers_set' is called directly from the handler
    // each time next buffers are requested, so data corruption is not
    // expected.
    __ASSERT(p_released != NULL, "p_released == NULL");

    // When the handler is called after the transfer has been stopped
    // (no next buffers are needed, only the used buffers are to be
    // released), there is nothing to do.
    if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED))
    {
        return;
    }

    // First call of this handler occurs right after the transfer is started.
    // No data has been transferred yet at this point, so there is nothing to
    // check. Only the buffers for the next part of the transfer should be
    // provided.
    if (!p_released->p_rx_buffer)
    {
        nrfx_i2s_buffers_t const next_buffers = {
            .p_rx_buffer = m_buffer_rx[1],
            .p_tx_buffer = m_buffer_tx[1],
        };
		
        ret = nrfx_i2s_next_buffers_set(&next_buffers);
		__ASSERT(ret == NRFX_SUCCESS, "ret != NRFX_SUCCESS");

        mp_block_to_fill = m_buffer_tx[1];
    }
    else
    {
        mp_block_to_check = p_released->p_rx_buffer;
        // The driver has just finished accessing the buffers pointed by
        // 'p_released'. They can be used for the next part of the transfer
        // that will be scheduled now.
        ret = nrfx_i2s_next_buffers_set(p_released);
		__ASSERT(ret == NRFX_SUCCESS, "ret != NRFX_SUCCESS");

        // The pointer needs to be typecasted here, so that it is possible to
        // modify the content it is pointing to (it is marked in the structure
        // as pointing to constant data because the driver is not supposed to
        // modify the provided data).
        mp_block_to_fill = (uint32_t *)p_released->p_tx_buffer;
    }
}

void test_i2s(void)
{
	uint32_t err_code = NRFX_SUCCESS;
	
	nrfx_i2s_config_t config = NRFX_I2S_DEFAULT_CONFIG(I2S_BCK,I2S_LRCK,I2S_MCK,I2S_DO,I2S_DI);
	// In Master mode the MCK frequency and the MCK/LRCK ratio should be
	// set properly in order to achieve desired audio sample rate (which
	// is equivalent to the LRCK frequency).
	// For the following settings we'll get the LRCK frequency equal to
	// 15873 Hz (the closest one to 16 kHz that is possible to achieve).
	config.mode         = NRF_I2S_MODE_MASTER;
	config.format       = NRF_I2S_FORMAT_I2S;
	config.alignment    = NRF_I2S_ALIGN_LEFT;
	config.sample_width = NRF_I2S_SWIDTH_16BIT;
	config.channels     = NRF_I2S_CHANNELS_LEFT;
	config.mck_setup    = NRF_I2S_MCK_32MDIV8;
	config.ratio        = NRF_I2S_RATIO_32X;

	err_code = nrfx_i2s_init(&config, data_handler);
	__ASSERT(err_code == NRFX_SUCCESS, "i2s init error");

	for (;;)
	{
		m_blocks_transferred = 0;
		mp_block_to_fill  = NULL;
		mp_block_to_check = NULL;

		prepare_tx_data(m_buffer_tx[0]);

		nrfx_i2s_buffers_t const initial_buffers = {
		    .p_tx_buffer = m_buffer_tx[0],
		    .p_rx_buffer = m_buffer_rx[0],
		};
		err_code = nrfx_i2s_start(&initial_buffers, I2S_DATA_BLOCK_WORDS, 0);
		__ASSERT(err_code == NRFX_SUCCESS, "i2s start error!");

		do {
		    // Wait for an event.
		    __WFE();
		    // Clear the event register.
		    __SEV();
		    __WFE();

		    if (mp_block_to_fill)
		    {
		        prepare_tx_data(mp_block_to_fill);
		        mp_block_to_fill = NULL;
		    }
		    if (mp_block_to_check)
		    {
		        check_rx_data(mp_block_to_check);
		        mp_block_to_check = NULL;
		    }
		} while (m_blocks_transferred < BLOCKS_TO_TRANSFER);

		nrfx_i2s_stop();

		k_sleep(K_MSEC(PAUSE_TIME));
    }
}
#endif
