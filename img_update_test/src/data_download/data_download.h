/****************************************Copyright (c)************************************************
** File Name:			    data_download.h
** Descriptions:			data download by mqtt protocal process head file
** Created By:				xie biao
** Created Date:			2021-12-28
** Modified Date:      		2021-12-28 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __DATA_DOWNLOAD_H__
#define __DATA_DOWNLOAD_H__

#include "lcd.h"

#define DL_NOTIFY_OFFSET_W		10
#define DL_NOTIFY_OFFSET_H		10

#define DL_NOTIFY_RECT_W		200
#define DL_NOTIFY_RECT_H		200
#define DL_NOTIFY_RECT_X		((LCD_WIDTH-DL_NOTIFY_RECT_W)/2)
#define DL_NOTIFY_RECT_Y		((LCD_HEIGHT-DL_NOTIFY_RECT_H)/2)

#define DL_NOTIFY_STRING_W		(DL_NOTIFY_RECT_W-10)
#define	DL_NOTIFY_STRING_H		(90)
#define	DL_NOTIFY_STRING_X		((LCD_WIDTH-DL_NOTIFY_STRING_W)/2)
#define	DL_NOTIFY_STRING_Y		(DL_NOTIFY_RECT_Y+(DL_NOTIFY_RECT_H-DL_NOTIFY_STRING_H)/2)

#define DL_NOTIFY_PRO_W			(DL_NOTIFY_RECT_W-20)
#define DL_NOTIFY_PRO_H			20
#define DL_NOTIFY_PRO_X			(DL_NOTIFY_RECT_X+(DL_NOTIFY_RECT_W-DL_NOTIFY_PRO_W)/2)
#define DL_NOTIFY_PRO_Y			(DL_NOTIFY_STRING_Y+40)

#define DL_NOTIFY_YES_W			60
#define DL_NOTIFY_YES_H			40
#define DL_NOTIFY_YES_X			(DL_NOTIFY_RECT_X+DL_NOTIFY_OFFSET_W)
#define DL_NOTIFY_YES_Y			(DL_NOTIFY_RECT_Y+DL_NOTIFY_RECT_H-DL_NOTIFY_YES_H-DL_NOTIFY_OFFSET_H)

#define DL_NOTIFY_NO_W			60
#define DL_NOTIFY_NO_H			40
#define DL_NOTIFY_NO_X			(DL_NOTIFY_RECT_X+(DL_NOTIFY_RECT_W-DL_NOTIFY_NO_W-DL_NOTIFY_OFFSET_W))
#define DL_NOTIFY_NO_Y			(DL_NOTIFY_RECT_Y+DL_NOTIFY_RECT_H-DL_NOTIFY_YES_H-DL_NOTIFY_OFFSET_H)

typedef enum
{
	DL_STATUS_PREPARE,
	DL_STATUS_LINKING,
	DL_STATUS_DOWNLOADING,
	DL_STATUS_FINISHED,
	DL_STATUS_ERROR,
	DL_STATUS_MAX
}DL_STATUS_ENUM;

enum download_evt_id
{
	/** download progress report. */
	DOWNLOAD_EVT_PROGRESS,
	/** download finished. */
	DOWNLOAD_EVT_FINISHED,
	/** download erase pending. */
	DOWNLOAD_EVT_ERASE_PENDING,
	/** download erase done. */
	DOWNLOAD_EVT_ERASE_DONE,
	/** download error. */
	DOWNLOAD_EVT_ERROR,
};

struct download_evt
{
	enum download_evt_id id;
	/** Download progress % */
	int progress;
};

typedef void (*dl_callback_t)(const struct download_evt *evt);

extern u8_t g_dl_progress;

extern void dl_work_init(struct k_work_q *work_q);
extern void dl_init(void);
extern void dl_start(void);
extern void dl_start_confirm(void);
extern void dl_reboot_confirm(void);
extern void dl_exit(void);
extern bool dl_is_running(void);

extern DL_STATUS_ENUM get_dl_status(void);

#endif/*__DATA_DOWNLOAD_H__*/
