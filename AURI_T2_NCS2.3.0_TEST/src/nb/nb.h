/****************************************Copyright (c)************************************************
** File name:			Nb.h
** Last modified Date:          
** Last Version:		V1.0   
** Created by:			谢彪
** Created date:		2020-06-30
** Version:			    1.0
** Descriptions:		NB-IoT头文件
******************************************************************************************************/
#define CMD_GET_IMEI	"AT+CGSN"
#define CMD_GET_IMSI	"AT+CIMI"
#define CMD_GET_ICCID	"AT%%XICCID"
#define CMD_GET_CRSM	"AT+CRSM=176,12258,0,0,10"
#define CMD_GET_MODEM_V "AT+CGMR"
#define CMD_GET_CESQ	"AT+CESQ"
#define CMD_GET_SNR		"AT%%XSNRSQ?"
#define CMD_GET_APN		"AT+CGDCONT?"
#define CMD_GET_CSQ		"AT+CSQ"
#define CMD_GET_MODEM_PARA	"AT%%XMONITOR"
#define CMD_GET_CUR_BAND	"AT%%XCBAND"
#define CMD_GET_SUPPORT_BAND	"AT%%XCBAND=?"
#define CMD_GET_LOCKED_BAND		"AT%%XBANDLOCK?"
#define CMD_GET_REG_STATUS	"AT+CEREG?"
#define CMD_SET_CREG	 	"AT+CEREG=5"
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
#define CMD_SET_EPCO_MODE	"AT%%XEPCO=0"
#endif

#define CMD_SET_NW_MODE_NB			"AT%%XSYSTEMMODE=0,1,0,0"	//Preferred network mode: Narrowband-IoT
#define CMD_SET_NW_MODE_NB_GPS		"AT%%XSYSTEMMODE=0,1,1,0"	//Preferred network mode: Narrowband-IoT and GPS
#define CMD_SET_NW_MODE_LTE_M		"AT%%XSYSTEMMODE=1,0,0,0"	//Preferred network mode: LTE-M
#define CMD_SET_NW_MODE_LTE_M_GPS	"AT%%XSYSTEMMODE=1,0,1,0"	//Preferred network mode: LTE-M and GPS
#define CMD_SET_NW_MODE_GPS			"AT%%XSYSTEMMODE=0,0,1,0"	//Preferred network mode: GPS

#define CMD_SET_FUN_MODE 	"AT+CFUN=1"						//Set the modem to Normal mode
#define CMD_SET_RAI		 	"AT%%XRAI=3"					//Set the modem rai parament

#define IMEI_MAX_LEN	(15)
#define IMSI_MAX_LEN	(15)
#define ICCID_MAX_LEN	(20)
#define MODEM_MAX_LEN   (17)

#define APN_MAX_LEN			(100)
#define	PLMN_MAX_LEN        (6)

//#define NB_SIGNAL_TEST		//xb add 2022-01-14 需要测试NB和GPS天线的时候打开这里

typedef enum
{
	NB_SIG_LEVEL_NO,
	NB_SIG_LEVEL_0 = NB_SIG_LEVEL_NO,
	NB_SIG_LEVEL_1,
	NB_SIG_LEVEL_2,
	NB_SIG_LEVEL_3,
	NB_SIG_LEVEL_4,
	NB_SIG_LEVEL_MAX
}NB_SIGNL_LEVEL;

typedef struct
{
	uint8_t plmn[PLMN_MAX_LEN];
	uint8_t apn[APN_MAX_LEN];
}NB_APN_PARAMENT;


extern NB_SIGNL_LEVEL g_nb_sig;

extern bool get_modem_info_flag;
extern bool test_nb_flag;

extern uint8_t nb_test_info[256];
extern uint8_t g_imsi[IMSI_MAX_LEN+1];
extern uint8_t g_imei[IMEI_MAX_LEN+1];
extern uint8_t g_iccid[ICCID_MAX_LEN+1];
extern uint8_t g_modem[MODEM_MAX_LEN+1];

extern uint8_t g_new_fw_ver[64];
extern uint8_t g_new_modem_ver[64];
extern uint8_t g_new_ble_ver[64];
extern uint8_t g_new_wifi_ver[64];
extern uint8_t g_new_ui_ver[16];
extern uint8_t g_new_font_ver[16];
extern uint8_t g_new_ppg_ver[16];
extern uint8_t g_timezone[5];
extern uint8_t g_prj_dir[128];
extern uint8_t g_rsrp;

extern void NB_init(struct k_work_q *work_q);
extern void NBMsgProcess(void);
extern void NBSendSosData(uint8_t *data, uint32_t datalen);
extern void SetModemTurnOff(void);
extern void SetModemTurnOn(void);
extern void SetModemGps(void);
