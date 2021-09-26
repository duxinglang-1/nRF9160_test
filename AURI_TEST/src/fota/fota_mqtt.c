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
#include "fota_mqtt.h"
#include "screen.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(fota_mqtt, CONFIG_LOG_DEFAULT_LEVEL);

#define TLS_SEC_TAG 42

static bool fota_start_flag = false;
static bool fota_confirm_flag = false;
static bool fota_run_flag = false;
static bool fota_reboot_flag = false;
static bool fota_redraw_pro_flag = false;

u8_t g_fota_progress = 0;

static struct device *gpiob;
static struct gpio_callback gpio_cb;
static struct k_work_q *app_work_q;
static struct k_work fota_work;
static FOTA_STATUS_ENUM fota_cur_status = FOTA_STATUS_DOWNLOADING;

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	LOG_INF("bsdlib recoverable error: %u\n", err);
}

#if 0	//xb test 2021-08-12
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
			"This sample does not support auto init and connect");
	int err;
#if !defined(CONFIG_BSD_LIBRARY_SYS_INIT)
	/* Initialize AT only if bsdlib_init() is manually
	 * called by the main application
	 */
	err = at_notif_init();
	__ASSERT(err == 0, "AT Notify could not be initialized.");
	err = at_cmd_init();
	__ASSERT(err == 0, "AT CMD could not be established.");
#if defined(CONFIG_USE_HTTPS)
	err = cert_provision();
	__ASSERT(err == 0, "Could not provision root CA to %d", TLS_SEC_TAG);
#endif
#endif
	printk("LTE Link Connecting ...\n");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	printk("LTE Link Connected!\n");
#endif
}

#else

static int modem_configure(void)
{
	int err;

	err = lte_lc_psm_req(false);
	if(err)
	{
		LOG_INF("[%s] lte_lc_psm_req, error: %d\n", __func__, err);
	}

	err = lte_lc_edrx_req(false);
	if(err)
	{
		LOG_INF("[%s] lte_lc_edrx_req, error: %d\n", __func__, err);
	}

	return err;
}
#endif

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
		LOG_INF("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			LOG_INF("Failed to delete existing certificate, err %d\n",
			       err);
		}
	}

	LOG_INF("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err)
	{
		LOG_INF("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/**@brief Start transfer of the file. */
static void app_dfu_transfer_start(struct k_work *unused)
{
	int retval;
	int sec_tag;

	LOG_INF("[%s]\n", __func__);

#ifndef CONFIG_USE_HTTPS
	sec_tag = -1;
#else
	sec_tag = TLS_SEC_TAG;
#endif

	retval = fota_download_start(CONFIG_FOTA_DOWNLOAD_HOST,
				     CONFIG_FOTA_DOWNLOAD_FILE,
				     sec_tag);
	if (retval != 0)
	{
		LOG_INF("fota_download_start() failed, err %d\n", retval);
		fota_run_flag = false;
		fota_cur_status = FOTA_STATUS_ERROR;
		fota_redraw_pro_flag = true;
	}
}

void fota_exit(void)
{
	fota_run_flag = false;
	fota_cur_status = FOTA_STATUS_MAX;
	
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
	fota_cur_status = FOTA_STATUS_DOWNLOADING;
	fota_redraw_pro_flag = true;
	
	DisconnectAppMqttLink();
	
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
		LOG_INF("[%s] Received error\n", __func__);
		fota_cur_status = FOTA_STATUS_ERROR;
		break;

	case FOTA_DOWNLOAD_EVT_PROGRESS:
		LOG_INF("[%s] Received progress:%d\n", __func__, evt->progress);
		g_fota_progress = evt->progress;
		fota_cur_status = FOTA_STATUS_DOWNLOADING;
		break;
		
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("[%s] Received finished!\n", __func__);
		fota_cur_status = FOTA_STATUS_FINISHED;
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

	LOG_INF("[%s] begin\n", __func__);

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
		LOG_INF("Modem firmware update successful!\n");
		LOG_INF("Modem will run the new firmware after reboot\n");
		k_thread_suspend(k_current_get());
		break;
		
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_INF("Modem firmware update failed\n");
		LOG_INF("Modem will run non-updated firmware on reboot.\n");
		break;
		
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_INF("Modem firmware update failed\n");
		LOG_INF("Fatal error.\n");
		break;
		
	case -1:
		LOG_INF("Could not initialize bsdlib.\n");
		LOG_INF("Fatal error.\n");
		return;
		
	default:
		break;
	}
	LOG_INF("Initialized bsdlib\n");

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

	LOG_INF("[%s] done\n", __func__);
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

void FotaMsgProc(void)
{
	if(fota_start_flag)
	{
		fota_start_flag = false;
		fota_start();
	}

	if(fota_confirm_flag)
	{
		fota_confirm_flag = false;
		fota_start_confirm();
	}
	
	if(fota_redraw_pro_flag)
	{
		fota_redraw_pro_flag = false;
		FOTARedrawProgress();
	}
	
	if(fota_reboot_flag)
	{
		LOG_INF("[%s] fota_reboot!\n", __func__);
		
		fota_reboot_flag = false;
		sys_reboot(0);
	}

	if(fota_run_flag)
	{
		k_sleep(K_MSEC(50));
	}
}

#endif/*CONFIG_FOTA_DOWNLOAD*/

