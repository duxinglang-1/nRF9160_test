/****************************************Copyright (c)************************************************
** File Name:			    dl_target.h
** Descriptions:			data to target process head file
** Created By:				xie biao
** Created Date:			2021-12-29
** Modified Date:      		2021-12-29 
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __DL_TARGET_H__
#define __DL_TARGET_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DL_TARGET_IMAGE_TYPE_UI 1
#define DL_TARGET_IMAGE_TYPE_FONT 2

enum dl_target_evt_id
{
	DL_TARGET_EVT_TIMEOUT,
	DL_TARGET_EVT_ERASE_DONE
};

typedef void (*dl_target_callback_t)(enum dl_target_evt_id evt_id);

/** @brief Functions which needs to be supported by all DL targets.
 */
struct dl_target {
	int (*init)(size_t file_size, dl_target_callback_t cb);
	int (*offset_get)(size_t *offset);
	int (*write)(const void *const buf, size_t len);
	int (*done)(bool successful);
};

/**
 * @brief Find the image type for the buffer of bytes recived. Used to determine
 *	  what dl target to initialize.
 *
 * @param[in] buf A buffer of bytes which are the start of an binary firmware
 *		  image.
 * @param[in] len The length of the provided buffer.
 *
 * @return Positive identifier for a supported image type or a negative error
 *	   code identicating reason of failure.
 **/
int dl_target_img_type(const void *const buf, size_t len);

/**
 * @brief Initialize the resources needed for the specific image type DL
 *	  target.
 *
 *	  If a target update is in progress, and the same target is
 *	  given as input, then calling the 'init()' function of that target is
 *	  skipped.
 *
 *	  To allow continuation of an aborted DL procedure, call the
 *	  'dfu_target_offset_get' function after invoking this function.
 *
 * @param[in] img_type Image type identifier.
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] cb Callback function in case the DL operation requires additional
 *		 proceedures to be called.
 *
 * @return 0 for a supported image type or a negative error
 *	   code identicating reason of failure.
 *
 **/
int dl_target_init(int img_type, size_t file_size, dl_target_callback_t cb);

/**
 * @brief Get offset of the firmware upgrade
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int dl_target_offset_get(size_t *offset);

/**
 * @brief Write the given buffer to the initialized DL target.
 *
 * @param[in] buf A buffer of bytes which contains part of an binary firmware
 *		  image.
 * @param[in] len The length of the provided buffer.
 *
 * @return Positive identifier for a supported image type or a negative error
 *	   code identicating reason of failure.
 **/
int dl_target_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize the resources that were needed for the current DL
 *	  target.
 *
 * @param[in] successful Indicate whether the process completed successfully or
 *			 was aborted.
 *
 * @return 0 for an successful deinitialization or a negative error
 *	   code identicating reason of failure.
 **/
int dl_target_done(bool successful);
	
#ifdef __cplusplus
	}
#endif
	
#endif/*__DL_TARGET_H__*/

