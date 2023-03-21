/****************************************Copyright (c)************************************************
** File Name:			    data_download.c
** Descriptions:			data download by mqtt protocal process source file
** Created By:				xie biao
** Created Date:			2021-12-28
** Modified Date:      		2021-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#include <modem/lte_lc.h>
#include <net/download_client.h>
#include "external_flash.h"
#include "data_download.h"
#include "dl_target.h"
#include "screen.h"
#include "nb.h"
#include "logger.h"

#define TLS_SEC_TAG 42
#define DL_SOCKET_RETRIES 2
#define DL_BUF_LEN	4096

static bool dl_start_flag = false;
static bool dl_run_flag = false;
static bool dl_reboot_flag = false;
static bool dl_redraw_pro_flag = false;

static uint8_t databuf[DL_BUF_LEN] = {0};
static uint32_t datalen = 0;

static dl_callback_t callback;
static struct download_client   dlc;
static struct k_delayed_work    dlc_with_offset_work;
static int socket_retries_left;

static struct device *gpiob;
static struct gpio_callback gpio_cb;
static struct k_work_q *app_work_q;
static struct k_delayed_work dl_work;
static DL_STATUS_ENUM dl_cur_status = DL_STATUS_ERROR;

uint8_t g_dl_progress = 0;
DL_DATA_TYPE g_dl_data_type = DL_DATA_IMG;

static int modem_configure(void)
{
	int err;

	err = lte_lc_psm_req(false);
	if(err)
	{
	#ifdef DL_DEBUG
		LOGD("lte_lc_psm_req, error: %d", err);
	#endif
	}

	err = lte_lc_edrx_req(false);
	if(err)
	{
	#ifdef DL_DEBUG
		LOGD("lte_lc_edrx_req, error: %d", err);
	#endif
	}

	return err;
}


static void send_evt(enum download_evt_id id)
{
	__ASSERT(id != DOWNLOAD_EVT_PROGRESS, "use send_progress");
	const struct download_evt evt = {
		.id = id
	};
	callback(&evt);
}

static void send_progress(int progress)
{
	const struct download_evt evt = { .id = DOWNLOAD_EVT_PROGRESS,
					       .progress = progress };
	callback(&evt);
}

static uint32_t rec_len = 0;

static void dl_target_callback_handler(enum dl_target_evt_id evt)
{
	switch(evt)
	{
	case DL_TARGET_EVT_TIMEOUT:
		send_evt(DOWNLOAD_EVT_ERASE_PENDING);
		break;
	case DL_TARGET_EVT_ERASE_DONE:
		send_evt(DOWNLOAD_EVT_ERASE_DONE);
		break;
	default:
		send_evt(DOWNLOAD_EVT_ERROR);
	}
}


static int download_client_callback(const struct download_client_evt *event)
{
	static bool first_fragment = true;
	static size_t file_size;
	size_t offset;
	int err;

	if (event == NULL)
	{
		return -EINVAL;
	}

	switch (event->id)
	{
	case DOWNLOAD_CLIENT_EVT_FRAGMENT:
		if(first_fragment)
		{
			datalen = 0;
			err = download_client_file_size_get(&dlc, &file_size);
			if(err != 0)
			{
			#ifdef DL_DEBUG
				LOGD("download_client_file_size_get err: %d", err);
			#endif
				send_evt(DOWNLOAD_EVT_ERROR);
				return err;
			}
			
			first_fragment = false;
			err = dl_target_init(g_dl_data_type, file_size, dl_target_callback_handler);
			if((err < 0) && (err != -EBUSY))
			{
			#ifdef DL_DEBUG
				LOGD("dfu_target_init error %d", err);
			#endif
				send_evt(DOWNLOAD_EVT_ERROR);
				return err;
			}

			err = dl_target_offset_get(&offset);
			if(err != 0)
			{
			#ifdef DL_DEBUG
				LOGD("unable to get dfu target offset err: %d", err);
			#endif
				send_evt(DOWNLOAD_EVT_ERROR);
			}

			if(offset != 0)
			{
				/* Abort current download procedure, and
				 * schedule new download from offset.
				 */
				k_delayed_work_submit(&dlc_with_offset_work, K_SECONDS(1));
			#ifdef DL_DEBUG
				LOGD("Refuse fragment, restart with offset");
			#endif

				return -1;
			}
		}

		memcpy(&databuf[datalen], event->fragment.buf, event->fragment.len);
		datalen += event->fragment.len;
	#ifdef DL_DEBUG
		LOGD("FRAGMENT datalen:%d", datalen);
	#endif
		if(datalen >= DL_BUF_LEN)
		{
			err = dl_target_write(databuf, datalen);
			datalen = 0;
			
			if(err != 0)
			{
			#ifdef DL_DEBUG
				LOGD("dl_target_write error %d", err);
			#endif
				(void) download_client_disconnect(&dlc);
				send_evt(DOWNLOAD_EVT_ERROR);
				return err;
			}
		}
		
		if(!first_fragment)
		{
			err = dl_target_offset_get(&offset);
			if(err != 0)
			{
			#ifdef DL_DEBUG
				LOGD("unable to get dl target offset err: %d", err);
			#endif
				send_evt(DOWNLOAD_EVT_ERROR);
				return err;
			}

			if(file_size == 0)
			{
			#ifdef DL_DEBUG
				LOGD("invalid file size: %d", file_size);
			#endif
				send_evt(DOWNLOAD_EVT_ERROR);
				return err;
			}

			send_progress((offset * 100) / file_size);
		#ifdef DL_DEBUG
			LOGD("Progress: %d/%d", offset, file_size);
		#endif
		}
		break;

	case DOWNLOAD_CLIENT_EVT_DONE:
	#ifdef DL_DEBUG
		LOGD("DONE datalen:%d", datalen);

	#endif
		if(datalen > 0)
		{
			err = dl_target_write(databuf, datalen);
			datalen = 0;
			
			if(err != 0)
			{
			#ifdef DL_DEBUG
				LOGD("dl_target_write error %d", err);
			#endif
				(void) download_client_disconnect(&dlc);
				send_evt(DOWNLOAD_EVT_ERROR);
				return err;
			}
		}
		
		err = dl_target_done(true);
		if(err != 0)
		{
		#ifdef DL_DEBUG
			LOGD("dfu_target_done error: %d", err);
		#endif
			send_evt(DOWNLOAD_EVT_ERROR);
			return err;
		}

		err = download_client_disconnect(&dlc);
		if(err != 0)
		{
			send_evt(DOWNLOAD_EVT_ERROR);
			return err;
		}
		send_evt(DOWNLOAD_EVT_FINISHED);
		first_fragment = true;
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR:
		/* In case of socket errors we can return 0 to retry/continue,
		 * or non-zero to stop
		 */
		if((socket_retries_left) && ((event->error == -ENOTCONN) ||
					      (event->error == -ECONNRESET)))
		{
		#ifdef DL_DEBUG
			LOGD("Download socket error. %d retries left...", socket_retries_left);
		#endif
			socket_retries_left--;
			/* Fall through and return 0 below to tell
			 * download_client to retry
			 */
		}
		else
		{
			download_client_disconnect(&dlc);
		#ifdef DL_DEBUG
			LOGD("Download client error");
		#endif
			first_fragment = true;
			send_evt(DOWNLOAD_EVT_ERROR);
			/* Return non-zero to tell download_client to stop */
			return event->error;
		}
		break;

	default:
		break;
	}

	return 0;
}

static void download_with_offset(struct k_work *unused)
{
	int offset;
	int err = dl_target_offset_get(&offset);

	err = download_client_start(&dlc, dlc.file, offset);
#ifdef DL_DEBUG
	LOGD("Downloading from offset: 0x%x", offset);
#endif
	if (err != 0)
	{
	#ifdef DL_DEBUG
		LOGD("%s failed with error %d", __func__, err);
	#endif
	}
}

int dl_download_start(const char *host, const char *file, int sec_tag)
{
	int err = -1;

	struct download_client_cfg config = {
		.sec_tag = sec_tag,
	};

	if(host == NULL || file == NULL || callback == NULL)
	{
		return -EINVAL;
	}

	socket_retries_left = DL_SOCKET_RETRIES;

	err = download_client_connect(&dlc, host, &config);
	if(err != 0)
	{
		return err;
	}

	err = download_client_start(&dlc, file, 0);
	if(err != 0)
	{
		download_client_disconnect(&dlc);
		return err;
	}

	return 0;
}

/**@brief Start transfer of the file. */
static void dl_transfer_start(struct k_work *unused)
{
	uint8_t dl_host[256] = {0};
	uint8_t dl_file[256] = {0};
	int retval;
	int sec_tag;

#ifdef DL_DEBUG
	LOGD("begin");
#endif

#ifndef CONFIG_USE_HTTPS
	sec_tag = -1;
#else
	sec_tag = TLS_SEC_TAG;
#endif

	switch(g_dl_data_type)
	{
	#ifdef CONFIG_IMG_DATA_UPDATE	
	case DL_DATA_IMG:
		if(strncmp(g_imsi, "460", strlen("460")) == 0)
			strcpy(dl_host, CONFIG_DATA_DOWNLOAD_HOST_CN);
		else
			strcpy(dl_host, CONFIG_DATA_DOWNLOAD_HOST_HK);
		strcpy(dl_file, g_prj_dir);
		strcat(dl_file, CONFIG_IMG_DATA_DOWNLOAD_FILE);
		break;
	#endif

	#ifdef CONFIG_FONT_DATA_UPDATE
	case DL_DATA_FONT:
		if(strncmp(g_imsi, "460", strlen("460")) == 0)
			strcpy(dl_host, CONFIG_DATA_DOWNLOAD_HOST_CN);
		else
			strcpy(dl_host, CONFIG_DATA_DOWNLOAD_HOST_HK);
		strcpy(dl_file, g_prj_dir);
		strcat(dl_file, CONFIG_FONT_DATA_DOWNLOAD_FILE);		
		break;
	#endif

	#if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
	case DL_DATA_PPG:
		if(strncmp(g_imsi, "460", strlen("460")) == 0)
			strcpy(dl_host, CONFIG_DATA_DOWNLOAD_HOST_CN);
		else
			strcpy(dl_host, CONFIG_DATA_DOWNLOAD_HOST_HK);
		strcpy(dl_file, g_prj_dir);
		strcat(dl_file, CONFIG_PPG_DATA_DOWNLOAD_FILE);
		break;
	#endif		
	}
	
	retval = dl_download_start(dl_host, dl_file, sec_tag);
	if(retval != 0)
	{
	#ifdef DL_DEBUG
		LOGD("download_start() failed, err %d", retval);
	#endif
		dl_run_flag = false;
		dl_cur_status = DL_STATUS_ERROR;
		dl_redraw_pro_flag = true;
	}
}

static void dl_timer_handler(struct k_timer *timer)
{
	switch(dl_cur_status)
	{
	case DL_STATUS_PREPARE:
		break;
		
	case DL_STATUS_LINKING:
		break;
		
	case DL_STATUS_DOWNLOADING:
		break;
		
	case DL_STATUS_FINISHED:
		dl_cur_status = DL_STATUS_MAX;
		dl_reboot_flag = true;
		break;
		
	case DL_STATUS_ERROR:
		dl_run_flag = false;
		dl_cur_status = DL_STATUS_MAX;
		dl_exit();
		break;
		
	case DL_STATUS_MAX:
		break;
	}
}

void dl_work_init(struct k_work_q *work_q)
{
	app_work_q = work_q;

	k_delayed_work_init(&dl_work, dl_transfer_start);
}

DL_STATUS_ENUM get_dl_status(void)
{
	return dl_cur_status;
}

bool dl_is_running(void)
{
	return dl_run_flag;
}

#ifdef CONFIG_IMG_DATA_UPDATE
void dl_img_prev(void)
{
	dl_run_flag = false;
	dl_cur_status = DL_STATUS_MAX;
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	PrevDlImgScreen();
}

void dl_img_exit(void)
{
	dl_run_flag = false;
	dl_cur_status = DL_STATUS_MAX;
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	ExitDlImgScreen();
}

void dl_img_start(void)
{
	dl_run_flag = true;
	g_dl_data_type = DL_DATA_IMG;
	dl_cur_status = DL_STATUS_PREPARE;
	
	EnterDlScreen();
}
#endif
#ifdef CONFIG_FONT_DATA_UPDATE
void dl_font_prev(void)
{
	dl_run_flag = false;
	dl_cur_status = DL_STATUS_MAX;
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	PrevDlFontScreen();
}

void dl_font_exit(void)
{
	dl_run_flag = false;
	dl_cur_status = DL_STATUS_MAX;
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	ExitDlFontScreen();
}

void dl_font_start(void)
{
	dl_run_flag = true;
	g_dl_data_type = DL_DATA_FONT;
	dl_cur_status = DL_STATUS_PREPARE;
	
	EnterDlScreen();
}
#endif

#if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
void dl_ppg_prev(void)
{
	dl_run_flag = false;
	dl_cur_status = DL_STATUS_MAX;
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	PrevDlPpgScreen();
}

void dl_ppg_exit(void)
{
	dl_run_flag = false;
	dl_cur_status = DL_STATUS_MAX;
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	ExitDlPpgScreen();
}

void dl_ppg_start(void)
{
	dl_run_flag = true;
	g_dl_data_type = DL_DATA_PPG;
	dl_cur_status = DL_STATUS_PREPARE;
	
	EnterDlScreen();
}
#endif

void dl_prev(void)
{
	switch(g_dl_data_type)
	{
	#ifdef CONFIG_IMG_DATA_UPDATE
	case DL_DATA_IMG:
		dl_img_prev();
		break;
	#endif

	#ifdef CONFIG_FONT_DATA_UPDATE
	case DL_DATA_FONT:
		dl_font_prev();
		break;
	#endif

	#if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
	case DL_DATA_PPG:
		dl_ppg_prev();
		break;
	#endif 
	}
}

void dl_exit(void)
{
	switch(g_dl_data_type)
	{
	#ifdef CONFIG_IMG_DATA_UPDATE
	case DL_DATA_IMG:
		dl_img_exit();
		break;
	#endif

	#ifdef CONFIG_FONT_DATA_UPDATE
	case DL_DATA_FONT:
		dl_font_exit();
		break;
	#endif

	#if defined(CONFIG_PPG_DATA_UPDATE)&&defined(CONFIG_PPG_SUPPORT)
	case DL_DATA_PPG:
		dl_ppg_exit();
		break;
	#endif
	}
}

void dl_start(void)
{
	dl_start_flag = true;
}

void dl_start_confirm(void)
{
#ifdef CONFIG_ANIMATION_SUPPORT	
	AnimaStopShow();
#endif
#ifdef CONFIG_TEMP_SUPPORT
	if(TempIsWorking()&&!TempIsWorkingTiming())
		MenuStopTemp();
#endif
#ifdef CONFIG_PPG_SUPPORT
	if(IsInPPGScreen()&&!PPGIsWorkingTiming())
		MenuStopPPG();
#endif
#ifdef CONFIG_WIFI_SUPPORT
	if(wifi_is_working())
		MenuStopWifi();
#endif

	dl_cur_status = DL_STATUS_LINKING;
	dl_redraw_pro_flag = true;

	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	//DisConnectMqttLink();
	modem_configure();
	k_delayed_work_submit_to_queue(app_work_q, &dl_work, K_SECONDS(2));
}

void dl_reboot_confirm(void)
{
	dl_reboot_flag = true;
}

void dl_handler(const struct download_evt *evt)
{
	switch(evt->id)
	{
	case DOWNLOAD_EVT_ERROR:
	#ifdef DL_DEBUG
		LOGD("Received error");
	#endif
		dl_cur_status = DL_STATUS_ERROR;
		break;

	case DOWNLOAD_EVT_PROGRESS:
	#ifdef DL_DEBUG
		LOGD("Received progress:%d", evt->progress);
	#endif
		g_dl_progress = evt->progress;
		dl_cur_status = DL_STATUS_DOWNLOADING;
		break;
		
	case DOWNLOAD_EVT_FINISHED:
	#ifdef DL_DEBUG
		LOGD("Received finished!");
	#endif
		switch(g_dl_data_type)
		{
		case DL_DATA_IMG:
			SpiFlash_Write(g_new_ui_ver, IMG_VER_ADDR, 16);
			strcpy(g_ui_ver, g_new_ui_ver);
			break;
		case DL_DATA_FONT:
			SpiFlash_Write(g_new_font_ver, FONT_VER_ADDR, 16);
			strcpy(g_font_ver, g_new_font_ver);		
			break;
		case DL_DATA_PPG:
			SpiFlash_Write(g_new_ppg_ver, PPG_ALGO_VER_ADDR, 16);
			strcpy(g_ppg_algo_ver, g_new_ppg_ver);				
			break;
		}
		dl_cur_status = DL_STATUS_FINISHED;
		break;

	default:
		break;
	}

	dl_redraw_pro_flag = true;
}

void dl_application(dl_callback_t client_callback)
{
	if (client_callback == NULL)
	{
		return -EINVAL;
	}

	callback = client_callback;

	k_delayed_work_init(&dlc_with_offset_work, download_with_offset);

	int err = download_client_init(&dlc, download_client_callback);

	if(err != 0)
	{
		return;
	}
}

void dl_init(void)
{
	dl_application(dl_handler);
}

void DlRedrawProgress(void)
{
	if(screen_id == SCREEN_ID_DL)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_DL;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void DlMsgProc(void)
{
	if(dl_start_flag)
	{
		dl_start_confirm();
		dl_start_flag = false;
	}
	
	if(dl_redraw_pro_flag)
	{
		dl_redraw_pro_flag = false;
		DlRedrawProgress();
	}
	
	if(dl_reboot_flag)
	{
		dl_reboot_flag = false;
		LCD_Clear(BLACK);
		sys_reboot(0);
	}

	if(dl_run_flag)
	{
		k_sleep(K_MSEC(50));
	}
}
