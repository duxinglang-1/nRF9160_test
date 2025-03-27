#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define ATECC608A_I2C_ADD	0x50

/** Device Identification (Who am I) **/
#define ATECC608A_ID                           0x02

/** @addtogroup  Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*maxdev_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*maxdev_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  maxdev_write_ptr  write_reg;
  maxdev_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} mcdev_ctx_t;

#define ATECC608A_WHO_AM_I                     0x00

