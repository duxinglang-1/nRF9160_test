/****************************************Copyright (c)************************************************
** File Name:			    lps22df.h
** Descriptions:			lps22df interface process head file
** Created By:				xie biao
** Created Date:			2024-06-24
** Modified Date:      		
** Version:			    	V1.0
******************************************************************************************************/
#ifndef __LPS22DF_H__
#define __LPS22DF_H__

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define PRESSURE_DEV DT_NODELABEL(i2c1)
#else
#error "i2c1 devicetree node is disabled"
#define PRESSURE_DEV	""
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
#define PRESSURE_PORT DT_NODELABEL(gpio0)
#else
#error "gpio0 devicetree node is disabled"
#define PRESSURE_PORT	""
#endif

#define GPIO_ACT_I2C

#define PRESSURE_EINT		13
#define PRESSURE_SCL		14
#define PRESSURE_SDA		16
#define PRESSURE_POW_EN		17

#define LPS22DF_CHIP_ID		0xB4
#define LPS22DF_I2C_ADDRESS	0x5C

#define LPS22DF_NO_ERROR   	0
#define LPS22DF_ERROR     	-1

typedef enum
{
	//RegisterName    			Addr. 	  	bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0
	REG_INTERRUPT_CFG 			= 0x0B, //AUTOREFP | RESET_ARP | AUTOZERO | RESET_AZ | - | LIR  | PLE | PHE
	REG_THS_P_L 	  			= 0x0C, //THS[7:0]
	REG_THS_P_H 	  			= 0x0D, //THS[14:8]
	REG_IF_CTRL		  			= 0x0E, //INT_EN_I3C | I2C_I3C_DIS | SIM | SDA_PU_EN | SDO_PU_EN | INT_PD_DIS | CS_PU_DIS
	REG_WHO_AM_I 	  			= 0x0F, //1 | 0 | 1 | 1 | 0 | 1 | 0 | 0 	(B4h)
	REG_CTRL_REG1	  			= 0x10, //0 | ODR[3:0] | AVG[2:0]
	REG_CTRL_REG2  	  			= 0x11,	//BOOT | 0 | LFPF_CFG | EN_LPFP | BDU | SWRESET | - | ONESHOT
	REG_CTRL_REG3 	  			= 0x12,	//0 | 0 | 0 | 0 | INT_H_L | 0 | PP_OD | IF_ADD_INC
	REG_CTRL_REG4	  			= 0x13,	//0 | DRDY_PLS | DRDY | INT_EN | - | INT_F_FULL | INT_F_WTM | INT_F_OVR
	REG_FIFO_CTRL	  			= 0x14,	//0 | 0 | 0 | 0 | STOP_ON_WTM | TRIG_MODES | F_MODE1 | F_MODE0
	REG_FIFO_WTM	  			= 0x15,	//0 | WTM[6:0]
	REG_REF_P_L		  			= 0x16,	//REFL[7:0]
	REG_REF_P_H		  			= 0x17,	//REFL[15:8]
	REG_I3C_IF_CTRL	  			= 0x19,	//1 | 0 | ASF_ON | 0 | 0 | 0 | I3C_Bus_Avb_Sel[1:0]
	REG_RPDS_L		  			= 0x1A,	//RPDS[7:0]
	REG_RPDS_H		  			= 0x1B,	//RPDS[15:8]
	REG_INT_SOURCE	  			= 0x24,	//BOOT_ON | 0 | 0 | 0 | 0 | IA | PL | PH
	REG_FIFO_STATUS1  			= 0x25,	//FSS[7:0]
	REG_FIFO_STATUS2  			= 0x26,	//FIFO_WTM_IA | FIFO_OVR_IA | FIFO_FULL_IA | - | - | - | - | -
	REG_STATUS		  			= 0x27,	//- | - | T_OR | P_OR | - | - | T_DA | P_DA
	REG_PRESSURE_OUT_XL 		= 0x28,	//POUT[7:0]
 	REG_PRESSURE_OUT_L  		= 0x29,	//POUT[15:8]
	REG_PRESSURE_OUT_H  		= 0x2A, //POUT[23:16]
	REG_TEMP_OUT_L				= 0x2B,	//TOUT[7:0]
	REG_TEMP_OUT_H				= 0x2C,	//TOUT[15:8]
	REG_FIFO_DATA_OUT_PRESS_XL 	= 0x78,	//FIFO_P[7:0]
	REG_FIFO_DATA_OUT_PRESS_L  	= 0x79,	//FIFO_P[15:8]
	REG_FIFO_DATA_OUT_PRESS_H  	= 0x7A, //FIFO_P[23:16]
}LPS22DF_REG;

typedef enum
{
	MEAS_ONE_SHOT,
	MEAS_CONTINOUS,
	MEAS_MAX
}LPS22DF_MEAS_CTRL;

typedef struct
{
	uint8_t phe			:1;
	uint8_t ple			:1;
	uint8_t lir			:1;
	uint8_t na			:1;
	uint8_t reset_az	:1;
	uint8_t autozero	:1;
	uint8_t reset_arp	:1;
	uint8_t autorefp	:1;
}lps22df_int_t;

typedef struct
{
	uint8_t na			:1;
	uint8_t cs_pu_dis	:1;
	uint8_t int_pd_dis	:1;
	uint8_t sdo_pu_en	:1;
	uint8_t sda_pu_en	:1;
	uint8_t sim			:1;
	uint8_t	i2c_i3c_dis	:1;
	uint8_t int_en_i3c	:1;
}lps22df_if_t;

typedef struct
{
	uint8_t avg	:3;
	uint8_t odr	:4;
	uint8_t	na	:1;
}lps22df_ctrl_reg_1_t;

typedef struct
{
	uint8_t oneshot	:1;
	uint8_t na		:1;
	uint8_t swreset	:1;
	uint8_t bdu		:1;
	uint8_t en_lpfp	:1;
	uint8_t lfpf_cfg:1;
	uint8_t zero	:1;
	uint8_t boot	:1;
}lps22df_ctrl_reg_2_t;

typedef struct
{
	uint8_t if_add_inc	:1;
	uint8_t pp_od		:1;
	uint8_t zero_1		:1;
	uint8_t int_h_l		:1;
	uint8_t zero_2		:4;
}lps22df_ctrl_reg_3_t;

typedef struct
{
	uint8_t int_f_ovr	:1;
	uint8_t int_f_wtm	:1;
	uint8_t int_f_full	:1;
	uint8_t na			:1;
	uint8_t int_en		:1;
	uint8_t drdy		:1;
	uint8_t drdy_pls	:1;
	uint8_t	zero		:1;
}lps22df_ctrl_reg_4_t;

typedef struct
{
	uint8_t f_mode		:2;
	uint8_t trig_modes	:1;
	uint8_t stop_on_wtm	:1;
	uint8_t zero		:4;
}lps22df_fifo_ctrl_t;

typedef struct
{
	uint8_t i3c_bus_avb_sel	:2;
	uint8_t zero_1			:3;
	uint8_t asf_on			:1;
	uint8_t zero_2			:1;
	uint8_t one				:1;
}lps22df_i3c_if_ctrl_t;

typedef struct
{
	lps22df_int_t int_ctrl;
	uint16_t ths_p;
	lps22df_if_t if_ctrl;
	lps22df_ctrl_reg_1_t reg1_ctrl;
	lps22df_ctrl_reg_2_t reg2_ctrl;
	lps22df_ctrl_reg_3_t reg3_ctrl;
	lps22df_ctrl_reg_4_t reg4_ctrl;
	lps22df_fifo_ctrl_t fifo_ctrl;
	uint8_t fifo_wtm;
	uint16_t ref_p;
	lps22df_i3c_if_ctrl_t i3c_if_ctrl;
	uint16_t rpds;
	LPS22DF_MEAS_CTRL meas_ctrl;
}lps22df_settings_t;

extern lps22df_settings_t lps22df_settings;

extern bool LPS22DF_Init(void);
extern void LPS22DF_Start(LPS22DF_MEAS_CTRL meas_type);
extern void LPS22DF_Stop(void);
extern void pressure_start(void);
extern void pressure_stop(void);
extern bool GetPressure(float *prs);

#endif/*__LPS22DF_H__*/