/****************************************Copyright (c)************************************************
** File Name:			    fota_mqtt.c
** Descriptions:			fota by mqtt protocal process source file
** Created By:				xie biao
** Created Date:			2021-08-10
** Modified Date:      		2021-08-10 
** Version:			    	V1.0
******************************************************************************************************/
#ifdef CONFIG_FOTA_DOWNLOAD

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#include <sys/reboot.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <net/fota_download.h>
#include <dfu/mcuboot.h>
#ifdef CONFIG_PPG_SUPPORT
#include "max32674.h"
#endif
#include "nb.h"
#include "external_flash.h"
#include "fota_mqtt.h"
#include "screen.h"
#include "logger.h"

//#define FOTA_DEBUG

#define TLS_SEC_TAG 42
#define FOTA_RESULT_NOTIFY_TIMEOUT	5

static bool fota_confirm_flag = false;
static bool fota_start_flag = false;
static bool fota_run_flag = false;
static bool fota_reboot_flag = false;
static bool fota_redraw_pro_flag = false;

uint8_t g_fota_progress = 0;

static struct device *gpiob;
static struct gpio_callback gpio_cb;
static struct k_work_q *app_work_q;
static struct k_delayed_work fota_work;
static FOTA_STATUS_ENUM fota_cur_status = FOTA_STATUS_ERROR;

static void fota_timer_handler(struct k_timer *timer_id);
K_TIMER_DEFINE(fota_timer, fota_timer_handler, NULL);

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
#ifdef FOTA_DEBUG
	LOGD("bsdlib recoverable error: %u\n", err);
#endif
}

static int modem_configure(void)
{
	int err;

	err = lte_lc_psm_req(false);
	if(err)
	{
	#ifdef FOTA_DEBUG
		LOGD("lte_lc_psm_req, error: %d", err);
	#endif
	}

	err = lte_lc_edrx_req(false);
	if(err)
	{
	#ifdef FOTA_DEBUG
		LOGD("lte_lc_edrx_req, error: %d", err);
	#endif
	}

	return err;
}

int cert_provision(void)
{
	static const char cert[] = {
		#include "../fota/cert/BaltimoreCyberTrustRoot"
	};
	BUILD_ASSERT_MSG(sizeof(cert) < KB(4), "Certificate too large");
	int err;
	bool exists;
	uint8_t unused;

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists);
	if(err)
	{
	#ifdef FOTA_DEBUG
		LOGD("Failed to check for certificates err %d", err);
	#endif
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if(err)
		{
		#ifdef FOTA_DEBUG
			LOGD("Failed to delete existing certificate, err %d", err);
		#endif
		}
	}

#ifdef FOTA_DEBUG
	LOGD("Provisioning certificate");
#endif
	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err)
	{
	#ifdef FOTA_DEBUG
		LOGD("Failed to provision certificate, err %d", err);
	#endif
		return err;
	}

	return 0;
}

/**@brief Start transfer of the file. */
static void app_dfu_transfer_start(struct k_work *unused)
{
	int retval;
	int sec_tag;
	uint8_t host[256] = {0};
	uint8_t file[128] = {0};

#ifdef FOTA_DEBUG
	LOGD("begin");
#endif

#ifndef CONFIG_USE_HTTPS
	sec_tag = -1;
#else
	sec_tag = TLS_SEC_TAG;
#endif

	if(strncmp(g_imsi, "460", strlen("460")) == 0)
		strcpy(host, CONFIG_FOTA_DOWNLOAD_HOST_CN);
	else
		strcpy(host, CONFIG_FOTA_DOWNLOAD_HOST_HK);

	strcpy(file, g_prj_dir);
	strcat(file, CONFIG_FOTA_DOWNLOAD_FILE);
	
	retval = fota_download_start(host, file, sec_tag, 0, 0);
	if(retval != 0)
	{
	#ifdef FOTA_DEBUG
		LOGD("fota_download_start() failed, err %d", retval);
	#endif
		fota_run_flag = false;
		fota_cur_status = FOTA_STATUS_ERROR;
		fota_redraw_pro_flag = true;
		k_timer_start(&fota_timer, K_SECONDS(FOTA_RESULT_NOTIFY_TIMEOUT), K_NO_WAIT);
	}
}

void fota_exit(void)
{
	fota_run_flag = false;
	fota_cur_status = FOTA_STATUS_MAX;
	LCD_Set_BL_Mode(LCD_BL_AUTO);
	ExitFOTAScreen();
}

static void fota_timer_handler(struct k_timer *timer)
{
	switch(fota_cur_status)
	{
	case FOTA_STATUS_PREPARE:
		break;
		
	case FOTA_STATUS_LINKING:
		break;
		
	case FOTA_STATUS_DOWNLOADING:
		break;
		
	case FOTA_STATUS_FINISHED:
		fota_cur_status = FOTA_STATUS_MAX;
		fota_reboot_flag = true;
		break;
		
	case FOTA_STATUS_ERROR:
		fota_run_flag = false;
		fota_cur_status = FOTA_STATUS_MAX;
		fota_exit();
		break;
		
	case FOTA_STATUS_MAX:
		break;
	}
}

void fota_work_init(struct k_work_q *work_q)
{
	app_work_q = work_q;

	k_delayed_work_init(&fota_work, app_dfu_transfer_start);
}

FOTA_STATUS_ENUM get_fota_status(void)
{
	return fota_cur_status;
}

bool fota_is_running(void)
{
	return fota_run_flag;
}

void fota_begin(void)
{
	fota_start_flag = true;
}

void fota_start(void)
{
	if(!fota_run_flag)
	{
		fota_run_flag = true;
		fota_cur_status = FOTA_STATUS_PREPARE;
		
		EnterFOTAScreen();		
	}
}

void fota_start_confirm(void)
{
	fota_cur_status = FOTA_STATUS_LINKING;
	fota_redraw_pro_flag = true;
	LCD_Set_BL_Mode(LCD_BL_ALWAYS_ON);
	//DisConnectMqttLink();
	modem_configure();
	k_delayed_work_submit_to_queue(app_work_q, &fota_work, K_SECONDS(2));
}

void fota_reboot_confirm(void)
{
	fota_reboot_flag = true;
}

void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch(evt->id)
	{
	case FOTA_DOWNLOAD_EVT_ERROR:
	#ifdef FOTA_DEBUG	
		LOGD("Received error");
	#endif
		fota_cur_status = FOTA_STATUS_ERROR;
		k_timer_start(&fota_timer, K_SECONDS(FOTA_RESULT_NOTIFY_TIMEOUT), K_NO_WAIT);
		break;

	case FOTA_DOWNLOAD_EVT_PROGRESS:
	#ifdef FOTA_DEBUG
		LOGD("Received progress:%d", evt->progress);
	#endif
		g_fota_progress = evt->progress;
		fota_cur_status = FOTA_STATUS_DOWNLOADING;
		break;
		
	case FOTA_DOWNLOAD_EVT_FINISHED:
	#ifdef FOTA_DEBUG
		LOGD("Received finished!");
	#endif
		fota_cur_status = FOTA_STATUS_FINISHED;
		k_timer_start(&fota_timer, K_SECONDS(FOTA_RESULT_NOTIFY_TIMEOUT), K_NO_WAIT);
		break;

	default:
		break;
	}

	fota_redraw_pro_flag = true;
}

static int application_init(void)
{
	int err;

	err = fota_download_init(fota_dl_handler);
	if(err != 0)
	{
		return err;
	}

	return 0;
}

void fota_init(void)
{
	int err;

#ifdef FOTA_DEBUG
	LOGD("begin");
#endif

#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
	err = nrf_modem_lib_init(NORMAL_MODE);
#else
	err = nrf_modem_lib_get_init_ret();
#endif
	switch(err)
	{
	case 0:
		/* Initialization successful, no action required. */
	#ifdef FOTA_DEBUG
		LOGD("Initialization successful");
	#endif		
		break;

	case MODEM_DFU_RESULT_OK:
	#ifdef FOTA_DEBUG	
		LOGD("Modem firmware update successful!");
		LOGD("Modem will run the new firmware after reboot");
	#endif
		k_thread_suspend(k_current_get());
		break;
		
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
	#ifdef FOTA_DEBUG
		LOGD("Modem firmware update failed");
		LOGD("Modem will run non-updated firmware on reboot.");
	#endif
		break;
		
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
	#ifdef FOTA_DEBUG
		LOGD("Modem firmware update failed");
		LOGD("Fatal error.");
	#endif
		break;
		return;
		
	default:
	#ifdef FOTA_DEBUG	
		LOGD("Could not initialize modem library, fatal error: %d", err);
	#endif
		break;
	}

	boot_write_img_confirmed();

	err = application_init();
	if(err != 0)
	{
		return;
	}

#ifdef FOTA_DEBUG
	LOGD("done");
#endif
}

void FOTARedrawProgress(void)
{
	if(screen_id == SCREEN_ID_FOTA)
	{
		scr_msg[screen_id].para |= SCREEN_EVENT_UPDATE_FOTA;
		scr_msg[screen_id].act = SCREEN_ACTION_UPDATE;
	}
}

void MenuStartFOTA(void)
{
	fota_confirm_flag = true;
}

void fota_excu(void)
{
	fota_confirm_flag = true;
}

void FotaMsgProc(void)
{
	if(fota_confirm_flag)
	{
		fota_confirm_flag = false;
		fota_start_confirm();
	}
	
	if(fota_start_flag)
	{
		fota_start_flag = false;
		fota_start();
	}

	if(fota_redraw_pro_flag)
	{
		fota_redraw_pro_flag = false;
		FOTARedrawProgress();
	}
	
	if(fota_reboot_flag)
	{
		fota_reboot_flag = false;
		
		if(strcmp(g_new_ui_ver,g_ui_ver) != 0)
		{
			dl_img_start();
		}
		else if(strcmp(g_new_font_ver,g_font_ver) != 0)
		{
			dl_font_start();
		}
	#ifdef CONFIG_PPG_SUPPORT
		else if(strcmp(g_new_ppg_ver,g_ppg_ver) != 0)
		{
			dl_ppg_start();
		}
	#endif
		else
		{
			LCD_Clear(BLACK);
			sys_reboot(1);
		}
	}

	if(fota_run_flag)
	{
		k_sleep(K_MSEC(50));
	}
}

#endif/*CONFIG_FOTA_DOWNLOAD*/

