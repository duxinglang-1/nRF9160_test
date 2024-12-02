typedef union{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

#define LSM6DSO_INT1_PIN	2
#define LSM6DSO_INT2_PIN	3
#define LSM6DSO_SDA_PIN		0
#define LSM6DSO_SCL_PIN		1

#define TWI_INSTANCE_ID               0
#define LSM6DSO_I2C_ADD               LSM6DSO_I2C_ADD_L >> 1 //need to shift 1 bit to the right.
#define ACC_GYRO_FIFO_BUF_LEN         200
#define VERIFY_DATA_BUF_LEN           200
#define PATTERN_LEN                   2*ACC_GYRO_FIFO_BUF_LEN
#define TWI_BUFFER_SIZE               14
#define TWI_TIMEOUT                   1000
#define INT_PIN                       10

//define low, medium, high memership
#define LOW_MS                        1
#define MEDIUM_MS                     2
#define HIGH_MS                       3

//define the memberships of Angle input
#define LOW_ANGLE                     15
#define MEDIUM_ANGLE                  40
#define HIGH_ANGLE                    80

//define the memberships of Gyroscope's Magnitude input
#define LOW_GYRO_MAGNITUDE            80
#define MEDIUM_GYRO_MAGNITUDE         180
#define HIGH_GYRO_MAGNITUDE           300

//define weight value
#define	WEIGHT_VALUE_10               10
#define	WEIGHT_VALUE_30               30
#define	WEIGHT_VALUE_50               50

//define default thresholds
#define ACC_MAGN_TRIGGER_THRES_DEF    9.0f 	 // for acc trigger
#define FUZZY_OUT_THRES_DEF           45.0f	 // for fuzzy output
#define STD_SVMG_SECOND_STAGE         12.0f
#define STD_VARIANCE_THRES_DEF        0.13f  	 // for Standard Deviation
#define PEAKS_NO_THRES                8       //number of peaks threshold
#define PEAK_THRES                    11.0f
#define MAX_GYROSCOPE_THRESHOLD       200  //max gyroscope threshold
#define MAX_ANGLE_THRESHOLD           40
#define	get_acc_magn(x)               x   //do nothing
#define	get_gyro_magn(x)              x   //do noting


//define minimum function
#define min(a,b) ((a)<(b)?(a):(b))

stmdev_ctx_t imu_dev_ctx;
lsm6dso_pin_int1_route_t int1_route;
lsm6dso_pin_int2_route_t int2_route;
lsm6dso_all_sources_t all_source;

lsm6dso_emb_fsm_enable_t fsm_enable;
uint16_t fsm_addr;

volatile bool hist_buff_flag      = false;
volatile bool curr_vrif_buff_flag = false;

bool int1_event = false;
bool int2_event = false;
bool fall_result = false;

static axis3bit16_t data_raw_acceleration;
static axis3bit16_t data_raw_angular_rate;
static float acceleration_mg[3];
static float angular_rate_mdps[3];
static float acceleration_g[3];
static float angular_rate_dps[3];
static uint8_t whoamI, rst;
float verify_acc_magn[VERIFY_DATA_BUF_LEN*2] = {0};
float std_devi=0;
float acc_magn_square=0, cur_angle=0, cur_max_gyro_magn=0, cur_fuzzy_output=0;

//static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);
static volatile bool m_xfer_done = false;


//uint8_t twi_tx_buffer[TWI_BUFFER_SIZE];

float acc_x_hist_buffer[PATTERN_LEN]  = {0}, acc_y_hist_buffer[PATTERN_LEN]   = {0}, acc_z_hist_buffer[PATTERN_LEN]   = {0};
float gyro_x_hist_buffer[PATTERN_LEN] = {0}, gyro_y_hist_buffer[PATTERN_LEN]  = {0}, gyro_z_hist_buffer[PATTERN_LEN]  = {0};

float accel_tempX[ACC_GYRO_FIFO_BUF_LEN] = {0}, accel_tempY[ACC_GYRO_FIFO_BUF_LEN] = {0}, accel_tempZ[ACC_GYRO_FIFO_BUF_LEN] = {0};
float gyro_tempX[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_tempY[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_tempZ[ACC_GYRO_FIFO_BUF_LEN]  = {0};

float acc_x_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, acc_y_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0}, acc_z_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0};
float gyro_x_cur_buffer[ACC_GYRO_FIFO_BUF_LEN] = {0}, gyro_y_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_cur_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0};

float acc_x_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, acc_y_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0}, acc_z_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]   = {0};
float gyro_x_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN] = {0}, gyro_y_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_vrif_buffer[ACC_GYRO_FIFO_BUF_LEN]  = {0};

float acc_x_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]  = {0}, acc_y_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]   = {0}, acc_z_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]   = {0};
float gyro_x_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN] = {0}, gyro_y_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]  = {0}, gyro_z_vrif_buffer_1[ACC_GYRO_FIFO_BUF_LEN]  = {0};

volatile uint8_t suspicion_rules[9][3] =
{
  LOW_MS     , LOW_MS     , LOW_MS    ,
  LOW_MS     , MEDIUM_MS  , LOW_MS    ,
  LOW_MS     , HIGH_MS    , LOW_MS    ,
  MEDIUM_MS  , LOW_MS     , LOW_MS    ,
  MEDIUM_MS  , MEDIUM_MS  , MEDIUM_MS ,
  MEDIUM_MS  , HIGH_MS    , HIGH_MS   ,
  HIGH_MS    , LOW_MS     , LOW_MS    ,
  HIGH_MS    , MEDIUM_MS  , MEDIUM_MS ,
  HIGH_MS    , HIGH_MS    , HIGH_MS
};

/*wrist tilt detection FSM*/

// Old version.
/*const uint8_t lsm6so_prg_wrist_tilt[] = {
      0x52, 0x00, 0x14, 0x00, 0x0D, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x0D, 0x06, 0x23, 0x00, 0x53,
      0x33, 0x74, 0x44, 0x22,
    };
*/
// new version from ucf file. check wrist_tilt.ucf file.
const uint8_t lsm6so_prg_wrist_tilt[] = {
  0x52, 0x00, 0x14, 0x00, 0x0D, 0x00, 0x8E, 0x31, 0x20, 0x00, 
  0x00, 0x0D, 0x06, 0x23, 0x00, 0x53, 0x33, 0x74, 0x44, 0x22,
};

// new version from Unico-GUI. Check picture.
/*const uint8_t lsm6so_prg_wrist_tilt[] = {
  0x52, 0x00, 0x14, 0x20, 0x0D, 0x0F, 0x8E, 0x31, 0x20, 0x00, 
  0x00, 0x0D, 0x06, 0x23, 0x00, 0x53, 0x33, 0x74, 0x44, 0x22,
};*/

/*fall + tap trigger FSM*/
const uint8_t falltrigger[] = {
      0x91, 0x00, 0x18, 0x00, 0x0E, 0x00, 0xCD, 0x3C,
      0x66, 0x36, 0xA8, 0x00, 0x00, 0x06, 0x23, 0X00,
      0x33, 0x63, 0x33, 0xA5, 0x57, 0x44, 0x22, 0X00,
     };