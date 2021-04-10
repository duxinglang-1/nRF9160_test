/****************************************Copyright (c)************************************************
** File name:			Nb.h
** Last modified Date:          
** Last Version:		V1.0   
** Created by:			Ð»±ë
** Created date:		2020-06-30
** Version:			    1.0
** Descriptions:		NB-IoTÍ·ÎÄ¼þ
******************************************************************************************************/
#define CMD_GET_IMEI	"AT+CGSN"
#define CMD_GET_IMSI	"AT+CIMI"
#define CMD_GET_RSRP	"AT+CESQ"

#define CMD_SET_CREG	 	"AT+CEREG=5"
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
#define CMD_SET_EPCO_MODE	"AT%XEPCO=0"
#endif
#define CMD_SET_NW_MODE		"AT%XSYSTEMMODE=0,1,1,0"	//Preferred network mode: Narrowband-IoT and GPS
#define CMD_SET_FUN_MODE 	"AT+CFUN=1"					//Set the modem to Normal mode

#define IMEI_MAX_LEN	(15+1)
#define IMSI_MAX_LEN	(15+1)


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

extern NB_SIGNL_LEVEL g_nb_sig;

extern void NB_init(struct k_work_q *work_q);
extern void NBMsgProcess(void);
