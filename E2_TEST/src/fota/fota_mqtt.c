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
#include <bsd.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/bsdlib.h>
#include <modem/modem_key_mgmt.h>
#include <net/fota_download.h>
#include <dfu/mcuboot.h>
#include "nb.h"
#include "fota_mqtt.h"
#include "screen.h"
#include "logger.h"

#define TLS_SEC_TAG 42
#define FOTA_RESULT_NOTIFY_TIMEOUT	5

static bool fota_confirm_flag = false;
static bool fota_start_flag = false;
static bool fota_run_flag = false;
static bool fota_reboot_flag = false;
static bool fota_redraw_pro_flag = false;

u8_t g_fota_progress = 0;

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
	LOGD("bsdlib recoverable error: %u\n", err);
}

static int modem_configure(void)
{
	int err;

	err = lte_lc_psm_req(false);
	if(err)
	{
		LOGD("lte_lc_psm_req, error: %d", err);
	}

	err = lte_lc_edrx_req(false);
	if(err)
	{
		LOGD("lte_lc_edrx_req, error: %d", err);
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
	u8_t unused;

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists, &unused);
	if (err) {
		LOGD("Failed to check for certificates err %d", err);
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			LOGD("Failed to delete existing certificate, err %d",
			       err);
		}
	}

	LOGD("Provisioning certificate");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err)
	{
		LOGD("Failed to provision certificate, err %d", err);
		return err;
	}

	return 0;
}

/**@brief Start transfer of the file. */
static void app_dfu_transfer_start(struct k_work *unused)
{
	int retval;
	int sec_tag;
	u8_t host[256] = {0};

	LOGD("begin");

#ifndef CONFIG_USE_HTTPS
	sec_tag = -1;
#else
	sec_tag = TLS_SEC_TAG;
#endif

	if(strncmp(g_imsi, "460", strlen("460")) == 0)
		strcpy(host, CONFIG_FOTA_DOWNLOAD_HOST_CN);
	else
		strcpy(host, CONFIG_FOTA_DOWNLOAD_HOST_HK);

	retval = fota_download_start(host,
				     CONFIG_FOTA_DOWNLOAD_FILE,
				     sec_tag);
	if (retval != 0)
	{
		LOGD("fota_download_start() failed, err %d", retval);
		fota_run_flag = false;
		fota_cur_status = FOTA_STATUS_ERROR;
		fota_redraw_pro_flag = true;
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
	DisConnectMqttLink();
	
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
		LOGD("Received error");
		fota_cur_status = FOTA_STATUS_ERROR;
		break;

	case FOTA_DOWNLOAD_EVT_PROGRESS:
		LOGD("Received progress:%d", evt->progress);
		g_fota_progress = evt->progress;
		fota_cur_status = FOTA_STATUS_DOWNLOADING;
		break;
		
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOGD("Received finished!");
		fota_cur_status = FOTA_STATUS_FINISHED;
		k_timer_start(&fota_timer, K_SECONDS(FOTA_RESULT_NOTIFY_TIMEOUT), NULL);
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

	LOGD("begin");

#if !defined(CONFIG_BSD_LIBRARY_SYS_INIT)
	err = bsdlib_init();
#else
	/* If bsdlib is initialized on post-kernel we should
	 * fetch the returned error code instead of bsdlib_init
	 */
	err = bsdlib_get_init_ret();
#endif
	switch(err)
	{
	case MODEM_DFU_RESULT_OK:
		LOGD("Modem firmware update successful!");
		LOGD("Modem will run the new firmware after reboot");
		k_thread_suspend(k_current_get());
		break;
		
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOGD("Modem firmware update failed");
		LOGD("Modem will run non-updated firmware on reboot.");
		break;
		
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOGD("Modem firmware update failed");
		LOGD("Fatal error.");
		break;
		
	case -1:
		LOGD("Could not initialize bsdlib.");
		LOGD("Fatal error.");
		return;
		
	default:
		break;
	}
	LOGD("Initialized bsdlib");

#if !defined(CONFIG_BSD_LIBRARY_SYS_INIT)
	/* Initialize AT only if bsdlib_init() is manually
	 * called by the main application
	 */
	err = at_notif_init();
	__ASSERT(err == 0, "AT Notify could not be initialized.");
	err = at_cmd_init();
	__ASSERT(err == 0, "AT CMD could not be established.");
#endif

	boot_write_img_confirmed();

	err = application_init();
	if(err != 0)
	{
		return;
	}

	LOGD("done");
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
		LOGD("fota_reboot!");
		
		fota_reboot_flag = false;
		sys_reboot(0);
	}

	if(fota_run_flag)
	{
		k_sleep(K_MSEC(50));
	}
}

#endif/*CONFIG_FOTA_DOWNLOAD*/

