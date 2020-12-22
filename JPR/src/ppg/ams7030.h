#include <nrf9160.h>
#include <zephyr.h>
#include <device.h>

/** @addtogroup  Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*amsdev_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*amsdev_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  amsdev_write_ptr  write_reg;
  amsdev_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
}amsdev_ctx_t;

amsdev_ctx_t ppg_dev_ctx;

extern u8_t g_heart_rate;

extern void PPG_init(void);
extern void PPGMsgProcess(void);

/*heart rate*/
extern void GetHeartRate(u8_t *HR);
