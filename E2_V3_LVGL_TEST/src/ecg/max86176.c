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
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include "logger.h"

#define MAX86176_DEBUG

short CoeffBuf_40Hz_LowPass[FILTERORDER] = {
	1, -6, 5, 2, -12, 11, 8, -26, 18, 20,
	-49, 25, 43, -82, 28, 83, -126, 22, 146, -182,
	-2, 240, -247, -56, 377, -317, -159, 573, -387, -343,
	862, -450, -679, 1341, -501, -1407, 2406, -535, -4188, 9276,
	21299, 9276, -4188, -535, 2406, -1407, -501, 1341, -679, -450,
	862, -343, -387, 573, -159, -317, 377, -56, -247, 240,
	-2, -182, 146, 22, -126, 83, 28, -82, 43, 25,
	-49, 20, 18, -26, 8, 11, -12, 2, 5, -6,
	1};

// short CoeffBuf_40Hz_LowPass[FILTERORDER] = {
// 	0, 0, 0, 0, 0, 0, 1, -1, 0, 2,
// 	-4, 2, 4 - 9, 7, 4, -17, 19, 0, -28,
// 	40, -15, -37, 73, -48, -37, 116, -108, -12, 162,
// 	-204, 57, 194, -336, 195, 181, -499, 431, 79, -677,
// 	810, -192, -845, 1446, -832, -977, 2858 - 2932, -1050, 18562,
// 	18562, -1050, -2932, 2858, -977, -832, 1446, -845, -192, 810,
// 	-677, 79, 431, -499, 181, 195, -336, 194, 57, -204,
// 	162, -12, -108, 116, -37, -48, 73, -37, -15, 40,
// 	-28, 0, 19, -17, 4, 7, -9, 4, 2, -4,
// 	2, 0, -1, 1, 0, 0, 0, 0, 0, 0};

// short CoeffBuf_40Hz_LowPass[FILTERORDER] = {

// 	-33, 19, 48, 44, 9, -49, -118, -179, -217,
// 	-222, -191, -131, -57, 13, 61, 74, 48, -12,
// 	-92, -173, -233, -257, -237, -178, -92, -1, 73,
// 	110, 99, 40, -52, -156, -246, -299, -299, -244,
// 	-145, -26, 83, 155, 170, 119, 14, -123, -257,
// 	-355, -389, -347, -234, -75, 91, 224, 286, 256,
// 	135, -53, -266, -449, -555, -547, -417, -186, 97,
// 	365, 547, 584, 447, 145, -271, -711, -1067, -1230,
// 	-1111, -664, 100, 1114, 2259, 3386, 4334, 4967, 5189,
// 	4967, 4334, 3386, 2259, 1114, 100, -664, -1111, -1230,
// 	-1067, -711, -271, 145, 447, 584, 547, 365, 97,
// 	-186, -417, -547, -555, -449, -266, -53, 135, 256,
// 	286, 224, 91, -75, -234, -347, -389, -355, -257,
// 	-123, 14, 119, 170, 155, 83, -26, -145, -244,
// 	-299, -299, -246, -156, -52, 40, 99, 110, 73,
// 	-1, -92, -178, -237, -257, -233, -173, -92, -12,
// 	48, 74, 61, 13, -57, -131, -191, -222, -217,
// 	-179, -118, -49, 9, 44, 48, 19, -33};
short ECG_WorkingBuff[2 * FILTERORDER];

#define ecgsize 30 * 128 // 根据需要调整大小
int EcgSampleCount = 0;
long ecgdatas[ecgsize];

// 定义日志队列大小：每条日志最大64字节，缓存1024条（可根据需求调整）
#define LOG_QUEUE_ITEM_SIZE 16
#define LOG_QUEUE_ITEM_COUNT 128 * 30 // 30秒ECG数据量缓存
static uint8_t log_queue_buf[LOG_QUEUE_ITEM_SIZE * LOG_QUEUE_ITEM_COUNT];
static struct ring_buf log_ring_buf;

// 定义日志模块
LOG_MODULE_REGISTER(MAX86176, CONFIG_LOG_DEFAULT_LEVEL);

struct device *spi_ecg;
struct device *gpio_ecg;
static struct gpio_callback gpio_cb;

static struct spi_buf_set tx_bufs, rx_bufs;
static struct spi_buf tx_buff, rx_buff;

static struct spi_config spi_cfg;
static struct spi_cs_control spi_cs_ctr;
bool ecg_int_flag = false;

bool gUseSpi = true;
uint8_t gReadBuf[NUM_SAMPLES_PER_INT * NUM_BYTES_PER_SAMPLE * EXTRABUFFER] = {0}; // array to store register reads
bool gUseEcg = true, gUseEcgPpgTime = true;									// modify these as desired

int QRS_Second_Prev_Sample = 0;
int QRS_Prev_Sample = 0;
int QRS_Current_Sample = 0;
int QRS_Next_Sample = 0;
int QRS_Second_Next_Sample = 0;

static unsigned short QRS_B4_Buffer_ptr = 0;

short QRS_Threshold_Old = 0;
short QRS_Threshold_New = 0;
unsigned char first_peak_detect = false;
unsigned short Respiration_Rate = 0;

// Savitzky-Golay滤波器结构体和系数
typedef struct {
    short buffer[9];        // 9点缓冲区
    int index;              // 当前写入位置
    int is_buffer_full;     // 缓冲区是否已填满
} sg_filter_t;

static const double sg_coeffs[9] = {
    -0.0202, 0.2828, 0.3716, 0.3117, 0.1688, 0.0087, -0.1032, -0.1010, 0.0808
};

static sg_filter_t sg_filter = {0};

static void ECG_Sleep_us(int us)
{
	k_sleep(K_MSEC(1));
}

static void ECG_Sleep_ms(int ms)
{
	k_sleep(K_MSEC(ms));
}

static void ECG_CS_LOW(void)
{
	gpio_pin_set(gpio_ecg, ECG_CS_PIN, 0);
}

static void ECG_CS_HIGH(void)
{
	gpio_pin_set(gpio_ecg, ECG_CS_PIN, 1);
}

void ECG_Int_Event(void)
{
	LOG_INF("INT Event");
	ecg_int_flag = true;
}

static void ECG_gpio_Init(void)
{
	int err;
	gpio_flags_t flag = GPIO_INPUT | GPIO_PULL_UP;

	// 端口初始化
	gpio_ecg = DEVICE_DT_GET(ECG_PORT);
	if (!gpio_ecg)
	{
		return;
	}
	// 设置为高，模式为测量ecg
	gpio_pin_configure(gpio_ecg, ECG_PWR_H_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ecg, ECG_PWR_H_PIN, 1);

	gpio_pin_configure(gpio_ecg, ECG_PWR_L_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ecg, ECG_PWR_L_PIN, 0);

	gpio_pin_configure(gpio_ecg, ECG_SW_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ecg, ECG_SW_PIN, 1);

	gpio_pin_configure(gpio_ecg, ECG_CS_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio_ecg, ECG_CS_PIN, 1);

	// interrupt
	gpio_pin_configure(gpio_ecg, ECG_INT_PIN, flag);
	gpio_pin_interrupt_configure(gpio_ecg, ECG_INT_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb, ECG_Int_Event, BIT(ECG_INT_PIN));
	gpio_add_callback(gpio_ecg, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_ecg, ECG_INT_PIN, GPIO_INT_ENABLE | GPIO_INT_EDGE_FALLING);
}

static void ECG_SPI_Init(void)
{
	spi_ecg = DEVICE_DT_GET(ECG_DEV);
	if (!spi_ecg)
	{
		return;
	}

	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	spi_cfg.frequency = 4000000;
	spi_cfg.slave = 0;
}

static void ECG_SPI_Transceive(uint8_t *txbuf, uint32_t txbuflen, uint8_t *rxbuf, uint32_t rxbuflen)
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

	ECG_CS_LOW();
	err = spi_transceive(spi_ecg, &spi_cfg, &tx_bufs, &rx_bufs);
	ECG_CS_HIGH();

	if (err)
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
	ECG_SPI_Transceive(tx_buf, i, NULL, 0);
}

void Max86176_ReadReg(uint8_t regAddr, uint8_t numBytes, uint8_t *readbuf)
{
	uint8_t i = 0;
	uint8_t tx_buf[8];
	uint8_t rx_buf[numBytes + 2];

	tx_buf[i++] = regAddr;
	tx_buf[i++] = 0x80;
	ECG_SPI_Transceive(tx_buf, i, rx_buf, numBytes + 2);
	memcpy(readbuf, rx_buf + 2, numBytes);
}

static void Max86176_Init(void) // call this on power up
{
	uint8_t part_id;

	Max86176_ReadReg(0xff, 1, &part_id);
#ifdef MAX86176_DEBUG
	LOG_INF("ID:%d", part_id);
#endif
	if (part_id != MAX86176_PART_ID)
		return;

	Max86176_WriteReg(0x10, 1); // RESET  ,并且使用过了内部时钟
	while (1)
	{
		Max86176_ReadReg(0x10, 1, &gReadBuf[0]);
	#ifdef MAX86176_DEBUG
		LOG_INF("reset:%d", gReadBuf[0]);
	#endif
		if((gReadBuf[0] & 0x01) == 0x00) // This bit then automatically becomes ‘0’ after thereset sequence is completed.
			break;
	}

	uint8_t readbuf[6];					// 存储6个寄存器的数据
	Max86176_ReadReg(0x00, 6, readbuf); // 从0x00开始读6字节

	LOG_INF("Reg0 0x%02X, 0x%02X,0x%02X, 0x%02X,0x%02X, 0x%02X\n", readbuf[0], readbuf[1], readbuf[2], readbuf[3], readbuf[4], readbuf[5]);

	Max86176_WriteReg(0x10, 0);
	LOG_INF("Max86176_Init");
	// Max86176_ReadReg(0x00, NUM_STATUS_REGS, gReadBuf);			  // 连续读6个状态寄存器的值， read and clear all status registers
	Max86176_WriteReg(0x0d, AFE_FIFO_SIZE - NUM_SAMPLES_PER_INT); // 设置FIFO数量，FIFO_A_FULL; assert A_FULL on NUM_SAMPLES_PER_INT samples
	Max86176_WriteReg(0x12, ((gUseEcgPpgTime ? 1 : 0) << 2));	  // 配置PPG的时序数据到FIFO中，PPG_TIMING_DATA; note that the initial PPG frames may not have an associated ECG sample if they come before the ECG samples have started
	Max86176_WriteReg(0x80, 0x80);								  // 中断使能 A_FULL_EN; enable interrupt pin on A_FULL assertion; ensure to enable the MCU's interrupt pin
	// Max86176_WriteReg(0x80, 0xE0);
	Max86176_WriteReg(0x1C, 0X20); // CLK_SEL; use internal 32.768kHz clock

	// PLL (32.768kHz*(0x1f+289)=10.486MHz)
	// PLL_CLK = FCLK * (MDIV + 288) / (NDIV + 1)”
	Max86176_WriteReg(0x19, 0x9F);
	uint8_t s19 = 0;
	Max86176_ReadReg(0x19, 1, &s19); // 读取Status 5寄存器(0x04)
	LOG_INF("s19  : 0x%02X", s19);

	Max86176_WriteReg(0x1A, 0x3F);
	uint8_t s1a = 0;
	Max86176_ReadReg(0x1A, 1, &s1a); // 读取Status 5寄存器(0x04)
	LOG_INF("s1a  : 0x%02X", s1a);
	Max86176_WriteReg(0x18, 0x1); // PLL_LOCK_WNDW | PLL_EN

	uint8_t pll_status = 0;
	Max86176_ReadReg(0x04, 1, &pll_status); // 读取Status 5寄存器(0x04)

	LOG_INF("read pll begin");
	while (!(pll_status & 0x02))
	{
		LOG_INF("pll_status : 0x%02X", pll_status);
		ECG_Sleep_ms(100);
		Max86176_ReadReg(0x04, 1, &pll_status); // 读取Status 5寄存器(0x04)
	}

	LOG_INF("read pll end");
	// ECG (32.768kHz*(0x1f+289)/(0x13f+1)/64=512Sps)
	// Max86176_WriteReg(0x90, ((gUseEcg ? 1 : 0) << 7) | 2); // 打开ECG，并且设置ECG的采样率 ECG_EN | ECG_DEC_RATE 采样频率为512sps
	Max86176_WriteReg(0x90, ((gUseEcg ? 1 : 0) << 7) | 4); // 修改采样率为128sps

	uint8_t ecg_config2 = (3 << 4) | (0); // PGA=8x (011b), INA=20x (00b)
	Max86176_WriteReg(0x91, ecg_config2); // 设置ECG增益

	Max86176_WriteReg(0xa2, (0 << 7) | (0 << 6)); // ECG_P正极输入端和ECG_N负极输入端连接到ECG通道 OPEN_P | OPEN_N

	// Max86176_WriteReg(0x85, 0x08);// 使能导联脱落中断，设置中断正极还是负极触发
	//  PPG (32.768kHz/256=128Fps)
	uint32_t FrClkDiv = 0x100;
	Max86176_WriteReg(0x11, 0x07);					 // PPG测试通道配置槽 MEAS1-3_EN; enable first NUM_MEAS_PER_FRAME measurements
	Max86176_WriteReg(0x1d, (FrClkDiv >> 8) & 0xff); // PPG时钟分频器FR_CLK_DIV
	Max86176_WriteReg(0x1e, (FrClkDiv >> 0) & 0xff); // FR_CLK_DIV
	Max86176_WriteReg(0x25, 0x10);					 // 设置LED驱动脉冲电流幅度 MEAS1_DRVA_PA; set current to non-0
	Max86176_WriteReg(0x2d, 0x10);					 // MEAS2_DRVA_PA; set current to non-0
	Max86176_WriteReg(0x35, 0x10);					 // MEAS3_DRVA_PA; set current to non-0

	// ACLOFF (ADC: ~10Sps, DAC: 32.768kHz*(0x1f+289)/(10*64*(2+1))=5461.3Hz)
	Max86176_WriteReg(0x93, (2 << 5));				  // AC交流导联脱落使能 EN_LOFF_DET
	Max86176_WriteReg(0x94, (1 << 6) | (2 << 4) | 5); // 使用 ECG 共模输入电阻，提高 AC-LOFF 精度 HI_CM_RES_EN (1 to use ECG common-mode input impedance) | LOFF_CG_MODE (2 when using RLD or lead bias) | LOFF_IMAG (200nA)
	Max86176_WriteReg(0x95, (1 << 7) | 2);			  // AC_LOFF_IWAVE (1=sine) | AC_LOFF_FREQ_DIV
	Max86176_WriteReg(0x99, 0x10);					  // AC 导联脱落比较器阈值 AC_LOFF_THRESH
	Max86176_WriteReg(0x9a, (0 << 6) | 5);			  // 用来滤除直流和低频干扰 AC_LOFF_UTIL_PGA_GAIN | AC_LOFF_HPF
	// other relevant ACLOFF registers are at defaults: AC_LOFF_CONV=0 (~10Sps), AC_LOFF_CMP=2
	// RLD
	Max86176_WriteReg(0xa8, (1 << 7) | (1 << 6) | (1 << 4) | (1 << 3) | (1 << 2) | 3); // RLD_EN | RLD_MODE | EN_RLD_OOR | ACTV_CM_P | ACTV_CM_N | RLD_GAIN
	Max86176_WriteReg(0xa9, (0 << 7) | (1 << 6) | (0 << 4) | 0);					   // RLD_EXT_RES | SEL_VCM_IN | RLD_BW | BODY_BIAS_DAC
}

#define NOTCH_B0 0.9910186348f
#define NOTCH_B1 1.5321355283f
#define NOTCH_B2 0.9910186348f
#define NOTCH_A1 1.5321355283f
#define NOTCH_A2 0.9820372695f

float ECG_NotchFilter(float input)
{
	static float x1 = 0, x2 = 0; // 过去两个输入
	static float y1 = 0, y2 = 0; // 过去两个输出
	float y;

	y = NOTCH_B0 * input + NOTCH_B1 * x1 + NOTCH_B2 * x2 - NOTCH_A1 * y1 - NOTCH_A2 * y2;

	// 更新状态
	x2 = x1;
	x1 = input;
	y2 = y1;
	y1 = y;

	return y;
}

/*
FIR滤波有限脉冲响应
*/
void ECG_FilterProcess(short *WorkingBuff, short *CoeffBuf, short *FilterOut)
{
	int i;
	int acc = 0;
	for (i = 0; i < FILTERORDER; i++)
	{
		acc += (*CoeffBuf++) * (*WorkingBuff--);
	}

	*FilterOut = (acc >> 15); // 根据滤波器精度调整右移位数
}

void ECG_IIR_FIR_Filter(short CurrAqsSample, short *FilteredOut)
{
	static unsigned short ECG_bufStart = 0, ECG_bufCur = FILTERORDER - 1, ECGFirstFlag = 1;
	static short ECG_Pvev_DC_Sample, ECG_Pvev_Sample;
	short *CoeffBuf;
	short temp1, temp2, ECGData;
	unsigned short Cur_Chan;
	short FiltOut;
	CoeffBuf = CoeffBuf_40Hz_LowPass;
	if (ECGFirstFlag)
	{
		for (Cur_Chan = 0; Cur_Chan < FILTERORDER; Cur_Chan++)
		{
			ECG_WorkingBuff[Cur_Chan] = 0;
		}
		ECG_Pvev_DC_Sample = 0;
		ECG_Pvev_Sample = 0;
		ECGFirstFlag = 0;
	}
	temp1 = NRCOEFF * ECG_Pvev_DC_Sample; // First order IIR
	ECG_Pvev_DC_Sample = (CurrAqsSample - ECG_Pvev_Sample) + temp1;
	ECG_Pvev_Sample = CurrAqsSample;
	temp2 = ECG_Pvev_DC_Sample >> 2;
	ECGData = (long)temp2;
	// LOGD(" IIR_ECGDATA: %ld", ECGData);
	ECG_WorkingBuff[ECG_bufCur] = ECGData;
	ECG_FilterProcess(&ECG_WorkingBuff[ECG_bufCur], CoeffBuf, (short *)&FiltOut);
	ECG_WorkingBuff[ECG_bufStart] = ECGData;
	FilteredOut[0] = FiltOut;
	ECG_bufCur++;
	ECG_bufStart++;
	if (ECG_bufStart == (FILTERORDER - 1))
	{
		ECG_bufStart = 0;
		ECG_bufCur = FILTERORDER - 1;
	}

	return;
}

#define SAMPLING_RATE 128

#define TWO_SEC_SAMPLES 2 * SAMPLING_RATE
#define MAXIMA_SEARCH_WINDOW 40
#define MINIMUM_SKIP_WINDOW 50

// #define MAXIMA_SEARCH_MS 80
// #define MINIMUM_SKIP_MS 100

// #define MAXIMA_SEARCH_WINDOW ((MAXIMA_SEARCH_MS * SAMPLING_RATE) / 1000)
// #define MINIMUM_SKIP_WINDOW   ((MINIMUM_SKIP_MS * SAMPLING_RATE) / 1000)

#define MAX_PEAK_TO_SEARCH 5
unsigned int sample_count = 0;
unsigned short QRS_Heart_Rate = 0;
unsigned char HR_flag;
unsigned char Start_Sample_Count_Flag = 0;
unsigned int sample_index[MAX_PEAK_TO_SEARCH + 2];
static void QRS_check_sample_crossing_threshold(unsigned short scaled_result)
{
	/* array to hold the sample indexes S1,S2,S3 etc */

	static unsigned short s_array_index = 0;
	static unsigned short m_array_index = 0;

	static unsigned char threshold_crossed = false;
	static unsigned short maxima_search = 0;
	static unsigned char peak_detected = false;
	static unsigned short skip_window = 0;
	static long maxima_sum = 0;
	static unsigned int peak = 0;
	static unsigned int sample_sum = 0;
	static unsigned int nopeak = 0;
	unsigned short max = 0;
	unsigned short HRAvg;

	if (true == threshold_crossed)
	{
		/*
		Once the sample value crosses the threshold check for the
		maxima value till MAXIMA_SEARCH_WINDOW samples are received
		*/
		sample_count++;
		maxima_search++;

		if (scaled_result > peak)
		{
			peak = scaled_result;
		}

		if (maxima_search >= MAXIMA_SEARCH_WINDOW)
		{
			// Store the maxima values for each peak
			maxima_sum += peak;
			maxima_search = 0;

			threshold_crossed = false;
			peak_detected = true;
		}
	}
	else if (true == peak_detected)
	{
		/*
		Once the sample value goes below the threshold
		skip the samples untill the SKIP WINDOW criteria is meet
		*/
		sample_count++;
		skip_window++;

		if (skip_window >= MINIMUM_SKIP_WINDOW)
		{
			skip_window = 0;
			peak_detected = false;
		}

		if (m_array_index == MAX_PEAK_TO_SEARCH)
		{
			sample_sum = sample_sum / (MAX_PEAK_TO_SEARCH - 1);
			HRAvg = (unsigned short)sample_sum;
#if 0
			if((LeadStatus & 0x0005)== 0x0000)
			{
				
			QRS_Heart_Rate = (unsigned short) 60 *  SAMPLING_RATE;
			QRS_Heart_Rate =  QRS_Heart_Rate/ HRAvg ;
				if(QRS_Heart_Rate > 250)
					QRS_Heart_Rate = 250 ;
			}
			else
			{
				QRS_Heart_Rate = 0;
			}
#else
			// Compute HR without checking LeadOffStatus
			QRS_Heart_Rate = (unsigned short)60 * SAMPLING_RATE;
			QRS_Heart_Rate = QRS_Heart_Rate / HRAvg;
			if (QRS_Heart_Rate > 250)
				QRS_Heart_Rate = 250;
#endif

			/* Setting the Current HR value in the ECG_Info structure*/

			HR_flag = 1;

			maxima_sum = maxima_sum / MAX_PEAK_TO_SEARCH;
			max = (short)maxima_sum;
			/*  calculating the new QRS_Threshold based on the maxima obtained in 4 peaks */
			maxima_sum = max * 7;
			maxima_sum = maxima_sum / 10;
			QRS_Threshold_New = (short)maxima_sum;

			/* Limiting the QRS Threshold to be in the permissible range*/
			if (QRS_Threshold_New > (4 * QRS_Threshold_Old))
			{
				QRS_Threshold_New = QRS_Threshold_Old;
			}

			sample_count = 0;
			s_array_index = 0;
			m_array_index = 0;
			maxima_sum = 0;
			sample_index[0] = 0;
			sample_index[1] = 0;
			sample_index[2] = 0;
			sample_index[3] = 0;
			Start_Sample_Count_Flag = 0;

			sample_sum = 0;
		}
	}
	else if (scaled_result > QRS_Threshold_New)
	{
		/*
			If the sample value crosses the threshold then store the sample index
		*/
		Start_Sample_Count_Flag = 1;
		sample_count++;
		m_array_index++;
		threshold_crossed = true;
		peak = scaled_result;
		nopeak = 0;

		/*	storing sample index*/
		sample_index[s_array_index] = sample_count;
		if (s_array_index >= 1)
		{
			sample_sum += sample_index[s_array_index] - sample_index[s_array_index - 1];
		}
		s_array_index++;
	}

	else if ((scaled_result < QRS_Threshold_New) && (Start_Sample_Count_Flag == 1))
	{
		sample_count++;
		nopeak++;
		if (nopeak > (3 * SAMPLING_RATE))
		{
			sample_count = 0;
			s_array_index = 0;
			m_array_index = 0;
			maxima_sum = 0;
			sample_index[0] = 0;
			sample_index[1] = 0;
			sample_index[2] = 0;
			sample_index[3] = 0;
			Start_Sample_Count_Flag = 0;
			peak_detected = false;
			sample_sum = 0;

			first_peak_detect = false;
			nopeak = 0;

			QRS_Heart_Rate = 0;
			HR_flag = 1;
		}
	}
	else
	{
		nopeak++;
		if (nopeak > (3 * SAMPLING_RATE))
		{
			/* Reset heart rate computation sate variable in case of no peak found in 3 seconds */
			sample_count = 0;
			s_array_index = 0;
			m_array_index = 0;
			maxima_sum = 0;
			sample_index[0] = 0;
			sample_index[1] = 0;
			sample_index[2] = 0;
			sample_index[3] = 0;
			Start_Sample_Count_Flag = 0;
			peak_detected = false;
			sample_sum = 0;
			first_peak_detect = false;
			nopeak = 0;
			QRS_Heart_Rate = 0;
			HR_flag = 1;
		}
	}
}

static void QRS_process_buffer(void)
{

	short first_derivative = 0; // 一阶导数的值
	short scaled_result = 0;	// 缩放后的结果

	static short max = 0;

	/* calculating first derivative*/
	first_derivative = QRS_Next_Sample - QRS_Prev_Sample;

	// 获取一阶导数的绝对值
	if (first_derivative < 0)
	{
		first_derivative = -(first_derivative);
	}

	scaled_result = first_derivative;

	if (scaled_result > max)
	{
		max = scaled_result;
	}

	QRS_B4_Buffer_ptr++;
	if (QRS_B4_Buffer_ptr == TWO_SEC_SAMPLES)
	{
		QRS_Threshold_Old = ((max * 7) / 10);
		QRS_Threshold_New = QRS_Threshold_Old;
		if (QRS_Threshold_New < 10)
    QRS_Threshold_New = 10;
	
		// 如果max大于70，则设置first_peak_detect为true,表示检测到第一个峰值
		if (max > 20)
			first_peak_detect = true;
		max = 0;
		QRS_B4_Buffer_ptr = 0;
	}

	if (true == first_peak_detect)
	{
		QRS_check_sample_crossing_threshold(scaled_result);
	}
}

void QRS_Algorithm_Interface(short CurrSample)
{
	static short prev_data[32] = {0};
	long Mac = 0; // 累加值
	prev_data[0] = CurrSample;
	for (int i = 31; i > 0; i--)
	{
		Mac += prev_data[i];
		prev_data[i] = prev_data[i - 1];
	}
	Mac += CurrSample;
	Mac = Mac >> 2;
	CurrSample = (short)Mac; // 将累加的值右移2位然后转化为短整形
	QRS_Second_Prev_Sample = QRS_Prev_Sample;
	QRS_Prev_Sample = QRS_Current_Sample;
	QRS_Current_Sample = QRS_Next_Sample;
	QRS_Next_Sample = QRS_Second_Next_Sample;
	QRS_Second_Next_Sample = CurrSample;
	QRS_process_buffer();
}


short ECG_SavitzkyGolay_Filter(short new_sample) {
    // 将新样本存入缓冲区
    sg_filter.buffer[sg_filter.index] = new_sample;
    sg_filter.index++;
    
    // 循环缓冲区
    if (sg_filter.index >= 9) {
        sg_filter.index = 0;
        sg_filter.is_buffer_full = 1;
    }
    
    // 只有当缓冲区填满后才进行滤波
    if (!sg_filter.is_buffer_full) {
        return new_sample; // 缓冲区未满时返回原始值
    }
    
    // 计算卷积（滤波）
    double sum = 0.0;
    int coeff_index = 0;
    
    // 从当前索引向前遍历缓冲区（循环）
    for (int i = sg_filter.index, count = 0; count < 9; count++) {
        sum += sg_filter.buffer[i] * sg_coeffs[coeff_index++];
        
        i++;
        if (i >= 9) {
            i = 0; // 循环到缓冲区开头
        }
    }
    
    // 将结果转换回short类型，注意数据范围
    int16_t result = (int16_t)(sum + 0.5); // 四舍五入
    
    return result;
}


// 日志线程入口函数（优先级设为最低，避免抢占主线程）
#define LOG_THREAD_STACK_SIZE 1024
#define LOG_THREAD_PRIORITY 5
short ECGFilteredData[4];
static void log_consumer_thread(void *p1, void *p2, void *p3)
{
	uint8_t log_str[4];
	size_t read_len;
	int32_t ecg_value;
	double dc3, ecg_bp, y, hrBeforeKalman, hrFiltered;
	double ecgInputValue;
	short curr_sample;
	float notch_out;
	while (1)
	{
		// 从队列读取日志（队列为空时阻塞，不占用CPU）
		read_len = ring_buf_get(&log_ring_buf, log_str, sizeof(log_str));
		if (read_len == 0)
		{
			k_sleep(K_MSEC(20)); // 防止空转占CPU
			continue;
		}
		if (read_len == 4)
		{
			ecg_value = *(int32_t *)log_str; // 还原32位整数
			ecgInputValue = (double)ecg_value;
			// 如果最高位是 1，则负数补码
			if (ecg_value & 0x20000)
			{
				ecg_value -= (1 << 18);
			}

			// LOG_INF("ECG RAW DATA %ld", ecg_value);
			//   转换为short类型传入滤波函数
			curr_sample = (short)ecg_value;

			ECG_IIR_FIR_Filter(curr_sample, &ECGFilteredData[1]);
			ECGFilteredData[1] = ECG_SavitzkyGolay_Filter(ECGFilteredData[1]);
			 QRS_Algorithm_Interface(ECGFilteredData[1]);
			LOG_INF(" HEART_ECGDATA: %ld", QRS_Heart_Rate);
			//LOG_INF("TI_FILTER_ECGDATA: %d", ECGFilteredData[1]);

			// baseline_filter((double)ecgInputValue, 50.0, 40.0, 1.0, 1.0, 128.0,
			//  				&dc3, &ecg_bp, &y, &hrBeforeKalman, &hrFiltered);
			//  LOG_INF("MAX_FILTER_ECGDATA: %d", y);
		}


		// LOG_INF("%s", log_str);
		k_sleep(K_MSEC(5)); // 保持打印节奏
	}
}
K_THREAD_STACK_DEFINE(log_stack_area, LOG_THREAD_STACK_SIZE);
static struct k_thread log_thread_data;

// 初始化日志线程（在 ECG_Sensor_Init 中调用）
static void log_thread_init(void)
{

	k_thread_create(&log_thread_data, log_stack_area, K_THREAD_STACK_SIZEOF(log_stack_area),
					log_consumer_thread, NULL, NULL, NULL,
					LOG_THREAD_PRIORITY, 0, K_NO_WAIT);
}

// 初始化队列
static void log_queue_init(void)
{
	ring_buf_init(&log_ring_buf, LOG_QUEUE_ITEM_SIZE * LOG_QUEUE_ITEM_COUNT, log_queue_buf);
}

//array to store ECG/ACLOFF/PPG ADC counts, time data
int32_t adcCountArr[NUM_ADC][NUM_SAMPLES_PER_INT * EXTRABUFFER];								 // array to store ECG/ACLOFF/PPG ADC counts, time data

void Max86176_onAfeInt(void) // call this on AFE interrupt
{
	static int32_t gEcgSampleCount = -1;
	static bool gEcgPpgTimeOccurred; // static since the reference sample have come in the previous interrupt
	static uint8_t gPpgFrameItemCount;
	int32_t ecg_value;
	uint8_t data0, data1, data2, sampleIx[NUM_ADC] = {0}, tag;;
	uint16_t readBufIx = 0; 

	LOGD("begin");
	
	Max86176_ReadReg(0x00, NUM_STATUS_REGS, gReadBuf); // read and clear all status registers
	// LOG_INF("gReadBuf[0]:%d", gReadBuf[0]);
	if (!(gReadBuf[0] & 0x80)) // FIFO满的标志位  check A_FULL bit
		return;
	// LOG_INF("zhongduan22222");
	Max86176_ReadReg(0x0a, 2, gReadBuf);						// read FIFO_DATA_COUNT
	uint32_t count = ((gReadBuf[0] & 0x80) << 1) | gReadBuf[1]; // FIFO_DATA_COUNT will be >= NUM_SAMPLES_PER_INT
	LOGD("count:%d", count);

	Max86176_ReadReg(0x0c, count * NUM_BYTES_PER_SAMPLE, gReadBuf); // read FIFO_DATA
	for(readBufIx = 0;readBufIx < (count*NUM_BYTES_PER_SAMPLE);readBufIx += NUM_BYTES_PER_SAMPLE) // parse the FIFO data
	{
		tag = (gReadBuf[readBufIx] >> 4) & 0xf;
		LOGD("readBufIx:%d, tag:%d", readBufIx, tag);
	#if 0	//xb test 2025-11-28	
		if(0)//((tag <= TAG_PPG_MAX) && ((gUseEcg && gUseEcgPpgTime && gEcgPpgTimeOccurred) || !(gUseEcg && gUseEcgPpgTime)))
		{
			//If time data and the ADC for the time data are enabled, only save samples that have associated time data. 
			//PPG samples may not have associated time data if they come before ECG samples, which may occur on start-up or PLL unlock.
			if(++gPpgFrameItemCount == (NUM_MEAS_PER_FRAME * NUM_PPG_PER_MEAS))
			{
				gPpgFrameItemCount = 0;
				gEcgPpgTimeOccurred = false;
			}
			adcCountArr[IX_PPG][sampleIx[IX_PPG]] = ((gReadBuf[readBufIx + 0] & 0xf) << 16) + (gReadBuf[readBufIx + 1] << 8) + gReadBuf[readBufIx + 2];
			if (gReadBuf[readBufIx + 0] & 0x8)
				adcCountArr[IX_PPG][sampleIx[IX_PPG]] -= (1 << 20);
			sampleIx[IX_PPG]++;
		}
		else if(tag == TAG_ECG)
		{
			data0 = gReadBuf[readBufIx];
			data1 = gReadBuf[readBufIx + 1];
			data2 = gReadBuf[readBufIx + 2];
			// ECG 数据是 18-bit 二补码
			ecg_value = ((data0 & 0x03) << 16) | (data1 << 8) | data2;

			uint8_t *data_ptr = (uint8_t *)&ecg_value;
			uint32_t data_len = sizeof(ecg_value);

			// 尝试写入队列，队列满时可选择丢弃（避免阻塞主线程）
			ring_buf_put(&log_ring_buf, data_ptr, data_len);
			// LOG_INF(" ECG_RAWDATA: %ld,", ecg_value);
			//  ecgdatas[EcgSampleCount++ % ecgsize] = ecg_value;

			gEcgSampleCount++;
			adcCountArr[IX_ECG][sampleIx[IX_ECG]++] = (gReadBuf[readBufIx + 0] >> 2) & 1; // Note that every other item is ECG fast recovery. Comment this out if not needed.
			adcCountArr[IX_ECG][sampleIx[IX_ECG]] = ((gReadBuf[readBufIx + 0] & 0x3) << 16) + (gReadBuf[readBufIx + 1] << 8) + gReadBuf[readBufIx + 2];
			if (gReadBuf[readBufIx + 0] & 0x2)
				adcCountArr[IX_ECG][sampleIx[IX_ECG]] -= (1 << 18);
			sampleIx[IX_ECG]++;
		}
		else if(tag == TAG_LOFFUTIL)
		{
			// LOG_INF("LOFFUTIL RAW DATS");
			tag = (gReadBuf[readBufIx + 0] >> 2) & 1; // this can also be used for the array index in this example
			adcCountArr[tag][sampleIx[tag]] = ((gReadBuf[readBufIx + 1] & 0xf) << 8) + gReadBuf[readBufIx + 2];
			if (gReadBuf[readBufIx + 0] & 0x8)
				adcCountArr[tag][sampleIx[tag]] -= (1 << 12);
			sampleIx[tag]++;
			if (sampleIx[tag] >= NUM_SAMPLES_PER_INT * EXTRABUFFER)
			{
				sampleIx[tag] = 0;
			}
		}
		else if(tag == TAG_TIME)
		{
			// LOG_INF("TIME RAW DATS");
			gEcgPpgTimeOccurred = true;
			adcCountArr[IX_TIME][sampleIx[IX_TIME]++] = gEcgSampleCount; // the ECG sample number that this PPG_TIMING_DATA is associated with
			adcCountArr[IX_TIME][sampleIx[IX_TIME]++] = ((gReadBuf[readBufIx + 1] & 0x3) << 8) + gReadBuf[readBufIx + 2];
		}
	#endif	
	}
	
	LOGD("end");
	// Process adcCountArr[][] here. sampleIx[] tells how many samples are in each adcCountArr[].
	// If sampleIx[IX_PPG]!=0 && gPpgFrameItemCount!=0, the complete frame has not been read (it will be complete on the next FIFO read), so account for that in the processing.
	// The shorter the delay between the HW interrupt and calling this function, and the more PPG measurements to be made, the higher the likelihood that the PPG frame may not be complete on this read.
}

void ECG_Disable_int(void)
{
	gpio_pin_interrupt_configure(gpio_ecg, ECG_INT_PIN, GPIO_INT_DISABLE);
}

void ECG_Enable_int(void)
{
	gpio_pin_configure(gpio_ecg, ECG_INT_PIN, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_interrupt_configure(gpio_ecg, ECG_INT_PIN, GPIO_INT_DISABLE);
	gpio_init_callback(&gpio_cb, ECG_Int_Event, BIT(ECG_INT_PIN));
	gpio_add_callback(gpio_ecg, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_ecg, ECG_INT_PIN, GPIO_INT_ENABLE | GPIO_INT_EDGE_FALLING);
}

void ECG_init(void)
{
	MAX20353_BoostDisable();
	
	ECG_gpio_Init();
	ECG_SPI_Init();

	// 初始化日志队列和日志线程
	log_queue_init();
	log_thread_init();

	k_sleep(K_MSEC(50));
	
	Max86176_Init();
}

void ECGMsgProcess(void)
{
	if(ecg_int_flag)
	{
		LOGD("ecg int!");
		//ECG_Disable_int();
		ecg_int_flag = false;
		//Max86176_onAfeInt();
		//ECG_Enable_int();
	}
}
