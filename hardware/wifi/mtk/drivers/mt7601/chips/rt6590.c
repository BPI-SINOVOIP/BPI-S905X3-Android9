/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rt6592.c

	Abstract:
	Specific funcitons and configurations for RT6590

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
*/

#ifdef RT65xx

#include	"rt_config.h"


UCHAR RT6590_EeBuffer[EEPROM_SIZE] = {
	0x83, 0x38, 0x01, 0x00, 0x00, 0x0c, 0x43, 0x28, 0x83, 0x00, 0x83, 0x28, 0x14, 0x18, 0xff, 0xff,
	0xff, 0xff, 0x83, 0x28, 0x14, 0x18, 0x00, 0x00, 0x01, 0x00, 0x6a, 0xff, 0x00, 0x02, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0c, 0x43, 0x28, 0x83, 0x01, 0x00, 0x0c,
	0x43, 0x28, 0x83, 0x02, 0x33, 0x0a, 0xec, 0x00, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x20, 0x01, 0x55, 0x77, 0xa8, 0xaa, 0x8c, 0x88, 0xff, 0xff, 0x0a, 0x08, 0x08, 0x06,
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x66, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa,
	0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xaa, 0xaa,
	0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xcc, 0xaa,
	0x88, 0x66, 0xcc, 0xaa, 0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xaa, 0xaa,
	0x88, 0x66, 0xaa, 0xaa, 0x88, 0x66, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	} ;


#define MT7650_EFUSE_CTRL	0x0024
static RTMP_REG_PAIR	RT6590_MACRegTable[] = {
	{PBF_SYS_CTRL,		0x80c00},
	{PBF_CFG,			0x77723c1f},
	{FCE_CTRL,			0x1},
	{AMPDU_MAX_LEN_20M1S,	0xAA842211},
	{TX_SW_CFG0,			0x604},	/* rising time gap */
// TODO: following register patch from windows
	{TX_SW_CFG1,		0x0},
	{TX_SW_CFG2,		0x0},
// TODO: ----patch end
// TODO: shiang-6590, check what tx report will send to us when following default value set as 2
	{0xa44,					0x0}, /* disable Tx info report */
// TODO: shiang-6590, following regiser used to patch for bbp hang issue, will make the RIFS rx failed!
	{TXOP_HLDR_ET,			0x3},
#ifdef HDR_TRANS_SUPPORT
	//{HEADER_TRANS_CTRL_REG, 0x1},
	{HEADER_TRANS_CTRL_REG, 0x3},
	{TSO_CTRL, 				0x07058},		// must set when enable HDR_TRANS_SUPPORT
#endif /* HDR_TRANS_SUPPORT */

	/*
		TC6008_TOP_MAC_reg_initial_value_20120426.xlsx 
	*/
	{AUX_CLK_CFG, 0x0},  /* Disable aux_clk_en */
	{BB_PA_MODE_CFG0, 0x010055FF},
	{BB_PA_MODE_CFG1, 0x00550055},
	{RF_PA_MODE_CFG0, 0x010055FF},
	{RF_PA_MODE_CFG1, 0x00550055},

	{TX_ALC_CFG_0, 0x5},
	{TX0_RF_GAIN_CORR, 0x00190000}, /* +2.5dB */
	{TX0_BB_GAIN_ATTEN, 0x00000000}, /* set BBP atten gain = 0 */
	{TX_ALC_VGA3, 0x0005000C}, /*  whenALC.RF[3:2] = 3, +3dB */
	{TX_PWR_CFG_0, 0x30303030},
	{TX_PWR_CFG_1, 0x30303030},
	{TX_PWR_CFG_2, 0x30303030},
	{TX_PWR_CFG_3, 0x30303030},
	{TX_PWR_CFG_4, 0x00003030},
	{TX_PWR_CFG_7, 0x00300030},
	{TX_PWR_CFG_8, 0x30},
	{TX_PWR_CFG_9, 0x30},

	{MT7650_EFUSE_CTRL, 0xD000}
};

static UCHAR RT6590_NUM_MAC_REG_PARMS = (sizeof(RT6590_MACRegTable) / sizeof(RTMP_REG_PAIR));

static RTMP_REG_PAIR RT6590_BBP_InitRegTb_GBand[] = {
	{CORE_R1,	0x00000058}, // BW 80
	{CORE_R4,	0x00000000},
	{CORE_R24,	0x00000000}, // Normal Tx mode
	{CORE_R32,	0x4003000a},
	{CORE_R42,	0x00000000}, // Disable noise histogram measurement
	{CORE_R44,	0x00000000}, // No Rxer noise floor measurement report

/*
	IBI - IN BAND REGISTER
	Initial values of these registers will be written by MAC. Driver doesn't need to write.	
*/

	{AGC1_R0,	0x00023400}, // BW 40
	{AGC1_R1,	0x00000001}, // Rx PE delay
	{AGC1_R2,	0x003A6464}, // CCA ED thresholds
	{AGC1_R4,	0x1FF6A652}, // Gain setting0 for Rx0

	{AGC1_R8,	0x102C40C0}, // Gain setting1 for Rx0

	{AGC1_R12,	0x703028F9}, // Gain control settings
	{AGC1_R13,	0x3D120412},
	{AGC1_R14,	0x00003202},
/*
	AGC1_R18, AGC1_R19, AGC1_R20, AGC1_R21: R only
*/
	{AGC1_R22,	0x00001E21},
	{AGC1_R23,	0x0000272C},
	{AGC1_R24,	0x00002F3A},
	{AGC1_R25,	0x8000005A}, // Fine sync timing delay setting
	{AGC1_R26,	0x00602004},
	{AGC1_R27,	0x000000C8}, // Frequency offset setting
	{AGC1_R28,	0x00060806}, // FOE auto correlation timing
	{AGC1_R30,	0x00000006},
	{AGC1_R31,	0x00000F03},
	{AGC1_R32,	0x00002828},
	{AGC1_R33,	0x00003218},
	{AGC1_R34,	0x000A0008},
	{AGC1_R35,	0x11111116},
	{AGC1_R37,	0x2121262C},
	{AGC1_R39,	0x2A2A3036},
	{AGC1_R41,	0x38383E45},
	{AGC1_R43,	0x27273438},
	{AGC1_R45,	0x26263034},
	{AGC1_R47,	0x26263438},
	{AGC1_R49,	0x3333393E},
	{AGC1_R51,	0x17171C1C},
	{AGC1_R53,	0x26262A2F},
	{AGC1_R55,	0x40404E58},
	{AGC1_R57,	0x00001010},
	{AGC1_R58,	0x00001010},
	{AGC1_R59,	0x004B28FF},
	{AGC1_R60,	0x0000FFFF},
	{AGC1_R61,	0x404C362C},
	{AGC1_R62,	0x00362E2C},
	{AGC1_R63,	0x00006E30},

	{TXC_R0, 	0x00280403},
	{TXC_R1, 	0x00000000}, // Normal Tx sin wave control

	{RXC_R1,	0x00000012}, // Bit range mapping and 11b preamble threshold
	{RXC_R2,	0x00000011}, // 11b tracking loop control
	{RXC_R3,	0x00000005}, // 11b adaptive filter control and high-pass filter
	{RXC_R4,	0x00000000}, // 11b timing control and EQ control

	{TXO_R0,	0x00000081},
	{TXO_R1,	0x0000010B},
	{TXO_R2,	0x00000000},
	{TXO_R3,	0x00040008},
	{TXO_R4,	0x00000006},
	{TXO_R5,	0x00C003F4},
	{TXO_R6,	0x00000000},
	{TXO_R7,	0x000E0000},
	{TXO_R8,	0x00000000},

	{TXBE_R0,	0x00000000},
	{TXBE_R1,	0x00101010},
	{TXBE_R2,	0x00000000},
	{TXBE_R3,	0x00000000},
	{TXBE_R4,	0x0000000C},
	{TXBE_R5,	0x00000000},
	{TXBE_R6,	0x00000000},
	{TXBE_R8,	0x00000014},
	{TXBE_R9,	0x20000000},
	{TXBE_R10,	0x00000000},
	{TXBE_R12,	0x00000000},
	{TXBE_R13,	0x00000000},
	{TXBE_R14,	0x00000000},
	{TXBE_R15,	0x00000000},
	{TXBE_R16,	0x00000000},
	{TXBE_R17,	0x00000000},

	{RXFE_R0,	0x006000E0},
	{RXFE_R3,	0x00000000},
	{RXFE_R4,	0x00000000},

	{RXO_R13,	0x00000012},
	{RXO_R14,	0x00060612},
	{RXO_R15,	0xC8321918},
	{RXO_R16,	0x0000001E},
	{RXO_R17,	0x00000000},
	{RXO_R21,	0x00000001}, // FSD d scaling (result from FEQ)
	{RXO_R24,	0x00000006}, // FSD d scaling (result from FEQ)
	{RXO_R28,	0x0000003F}, // RF loopback test control and BIST EVM report

	{CAL_R0, 0x00000000}, 
	{CAL_R1, 0x00000000}, 
	{CAL_R2, 0x00000000}, 
	{CAL_R3, 0x00000000}, 
	{CAL_R4, 0x000000FF},  // Enable all compensation (bbp and RF)
	{CAL_R5, 0x00000000}, 
	{CAL_R6, 0x00000000}, 
	{CAL_R7, 0x00000000}, 

	{CAL_R9, 0x00000000}, 
	{CAL_R10, 0x00000000}, 

	{CAL_R14, 0x00000000}, 
	{CAL_R15, 0x00000000}, 

	{CAL_R18, 0x00000000}, 
	{CAL_R19, 0x00000000}, 

	{CAL_R23, 0x00000000}, 
	{CAL_R24, 0x00000000}, 
	{CAL_R25, 0x00000000}, 

	{CAL_R28, 0x00000000}, 
	{CAL_R29, 0x00000000}, 

	{CAL_R36, 0x00000000}, 
	{CAL_R37, 0x00000000}, 

	{CAL_R40, 0x00000000}, 
	{CAL_R41, 0x00000000}, 


	{CAL_R58, 0x00000000}, 
	{CAL_R59, 0x00000000}, 
	{CAL_R60, 0x00000000}, 
	{CAL_R61, 0x00000000}, 
};
static UCHAR RT6590_BBP_InitRegTb_GBand_Tab_Size = (sizeof(RT6590_BBP_InitRegTb_GBand) / sizeof(RTMP_REG_PAIR));

static RTMP_REG_PAIR RT6590_BBP_InitRegTb_ABand[] = {
	{CORE_R1,	0x00000058}, /* BW 80 */
	{CORE_R4,	0x00000000},
	{CORE_R24,	0x00000000}, /* Normal Tx mode */
	{CORE_R32,	0x4003000a},
	{CORE_R42,	0x00000000}, // Disable noise histogram measurement
	{CORE_R44,	0x00000000}, // No Rxer noise floor measurement report

/*
	IBI - IN BAND REGISTER
	Initial values of these registers will be written by MAC. Driver doesn't need to write.	
*/

	{AGC1_R0,	0x00027400}, /* BW 80, 1Rx mode[4:3]=0, Rx select[1:0]=0 for ADC0 */
	{AGC1_R1,	0x00000001}, // Rx PE delay
	{AGC1_R2,	0x003A6464}, // CCA ED thresholds
	{AGC1_R4,	0x1FECA04F}, // 55nm LNA default A-band gain: 2012/04/30

	{AGC1_R8,	0x132C4CC0}, // 55nm LNA default A-band gain: 2012/04/30

	{AGC1_R12,	0x703028F9}, // VGA/LNA settle time: 2012/05/09
	{AGC1_R13,	0x2C3A0412}, // refine CCK AGC gain threshold: 2012/05/09
	{AGC1_R14,	0x00002A3B}, // 55nm 9-ADC OFDM A-band target level: 2012/04/30
/*
	AGC1_R18, AGC1_R19, AGC1_R20, AGC1_R21: R only
*/
	{AGC1_R22,	0x00001E21},
	{AGC1_R23,	0x0000272C},
	{AGC1_R24,	0x00002F3A},
	{AGC1_R25,	0x8000005A}, // Fine sync timing delay setting
	{AGC1_R26,	0x005A2004},
	{AGC1_R27,	0x000000C8}, // Frequency offset setting
	{AGC1_R28,	0x00060806}, // FOE auto correlation timing
	{AGC1_R30,	0x00000000}, // CCK-MRC
	{AGC1_R31,	0x00000F13}, // turn off DC estimation for GLRT: 2012/05/09
	{AGC1_R32,	0x0000181C},
	{AGC1_R33,	0x00003218},
	{AGC1_R34,	0x000A0008},
	{AGC1_R35,	0x11111F16}, // OFDM-TH0A
	{AGC1_R37,	0x2121262C}, // OFDM-TH0B
	{AGC1_R39,	0x2A2A3036},
	{AGC1_R41,	0x38383E45},
	{AGC1_R43,	0x27272B30},
	{AGC1_R45,	0x26262C31},
	{AGC1_R47,	0x26262B30},
	{AGC1_R49,	0x3333393E},
	{AGC1_R51,	0xFFFFFFFF},
	{AGC1_R53,	0xFFFFFFFF}, // higher CCK PD threshold to avoid false trigger: 2012/05/09
	{AGC1_R55,	0xFFFFFFFF}, // higher CCK PD threshold to avoid false trigger: 2012/05/09
	{AGC1_R57,	0x00001010},
	{AGC1_R58,	0x00000000},
	{AGC1_R59,	0x004B287D},
	{AGC1_R60,	0x00003214},
	{AGC1_R61,	0x404C362C},
	{AGC1_R62,	0x00362E2C},
	{AGC1_R63,	0x00006E30},

	{TXC_R0, 	0x00280403},
	{TXC_R1, 	0x00000000}, // Normal Tx sin wave control

	{RXC_R1,	0x00000012}, // Bit range mapping and 11b preamble threshold
	{RXC_R2,	0x00000011}, // 11b tracking loop control
	{RXC_R3,	0x00000005}, // 11b adaptive filter control and high-pass filter
	{RXC_R4,	0x00000000}, // 11b timing control and EQ control

	{TXO_R0,	0x00000082}, // BW 80
	{TXO_R1,	0x0000010B},
	{TXO_R2,	0x00000000},
	{TXO_R3,	0x00040008},
	{TXO_R4,	0x00000006},
	{TXO_R5,	0x00C003F4},
	{TXO_R6,	0x00000000},
	{TXO_R7,	0x000E0000},
	{TXO_R8,	0x00000000},

	{TXBE_R0,	0x00000000},
	{TXBE_R1,	0x00101010},
	{TXBE_R2,	0x00000000},
	{TXBE_R3,	0x00000000},
	{TXBE_R4,	0x00000004},
	{TXBE_R5,	0x00000000},
	{TXBE_R6,	0x00000000},
	{TXBE_R8,	0x00000014},
	{TXBE_R9,	0x20000000},
	{TXBE_R10,	0x00000000},
	{TXBE_R12,	0x00000000},
	{TXBE_R13,	0x00000000},
	{TXBE_R14,	0x00000000},
	{TXBE_R15,	0x00000000},
	{TXBE_R16,	0x00000000},
	{TXBE_R17,	0x00000000},

	{RXFE_R0,	0x006000E0},
	{RXFE_R3,	0x00000000},
	{RXFE_R4,	0x00000000},

	{RXO_R13,	0x00000012},
	{RXO_R14,	0x00060612},
	{RXO_R15,	0xC8321B18},
	{RXO_R16,	0x0000001E},
	{RXO_R17,	0x00000000},
	{RXO_R21,	0x00000001}, // FSD d scaling (result from FEQ)
	{RXO_R24,	0x00000006}, // FSD d scaling (result from FEQ)
	{RXO_R28,	0x0000003F}, // RF loopback test control and BIST EVM report

	{CAL_R0, 0x00000000}, 
	{CAL_R1, 0x00000000}, 
	{CAL_R2, 0x00000000}, 
	{CAL_R3, 0x00000000}, 
	{CAL_R4, 0x000000FF},  // Enable all compensation (bbp and RF)
	{CAL_R5, 0x00000000}, 
	{CAL_R6, 0x00000000}, 
	{CAL_R7, 0x00000000}, 

	{CAL_R9, 0x00000000}, 
	{CAL_R10, 0x00000000}, 

	{CAL_R14, 0x00000000}, 
	{CAL_R15, 0x00000000}, 

	{CAL_R18, 0x00000000}, 
	{CAL_R19, 0x00000000}, 

	{CAL_R23, 0x00000000}, 
	{CAL_R24, 0x00000000}, 
	{CAL_R25, 0x00000000}, 

	{CAL_R28, 0x00000000}, 
	{CAL_R29, 0x00000000}, 

	{CAL_R36, 0x00000000}, 
	{CAL_R37, 0x00000000}, 

	{CAL_R40, 0x00000000}, 
	{CAL_R41, 0x00000000}, 

	{CAL_R58, 0x00000000}, 
	{CAL_R59, 0x00000000}, 
	{CAL_R60, 0x00000000}, 
	{CAL_R61, 0x00000000}, 
};
static UCHAR RT6590_BBP_InitRegTb_ABand_Tab_Size = (sizeof(RT6590_BBP_InitRegTb_ABand) / sizeof(RTMP_REG_PAIR));

static RT6590_DOCO_Table RT6590_DCOC_Tab[] = {
	{RF_G_BAND | RF_A_BAND,	{CAL_R47, 0x00000200}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R48, 0x00001010}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R49, 0x00001008}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R50, 0x00000040}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R51, 0x00000404}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R52, 0x00080803}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R53, 0x00000303}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R54, 0x00008080}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R55, 0x00004040}},
	{RF_G_BAND | RF_A_BAND,	{CAL_R46, 0x00006464}},
};
static UCHAR RT6590_DCOC_Tab_Size = (sizeof(RT6590_DCOC_Tab) / sizeof(RT6590_DOCO_Table));

/* Bank	Register Value(Hex) */
static BANK_RF_REG_PAIR RT6590_RF_Central_RegTb[] = {
/*
	Bank 0 - For central blocks: BG, PLL, XTAL, LO, ADC/DAC
*/
	{RF_BANK0,	RF_R01, 0x01},
	{RF_BANK0,	RF_R02, 0x11},

	/*
		R3 ~ R7: VCO Cal.
	*/	
	{RF_BANK0,	RF_R03, 0x80}, /* Disable VCO cal by pass mode */
	{RF_BANK0,	RF_R04, 0x00}, /* R4 b<7>=1, VCO cal */
	{RF_BANK0,	RF_R05, 0x00},
	{RF_BANK0,	RF_R06, 0x41}, /* Set the open loop amplitude to middle since bypassing amplitude calibration */
	{RF_BANK0,	RF_R07, 0x00},

	/*
		XO
	*/
	{RF_BANK0,	RF_R08, 0x80}, 
	{RF_BANK0,	RF_R09, 0x00},
	{RF_BANK0,	RF_R10, 0x0C},
	{RF_BANK0,	RF_R11, 0x00},
	{RF_BANK0,	RF_R12, 0x00}, /* by EEPROM b<5:0> Frequency offset */

	/*
		BG
	*/
	{RF_BANK0,	RF_R13, 0x00},
	{RF_BANK0,	RF_R14, 0x00},
	{RF_BANK0,	RF_R15, 0x00},

	/*
		LDO
	*/
	{RF_BANK0,	RF_R16, 0x20},
	{RF_BANK0,	RF_R17, 0x00},
	{RF_BANK0,	RF_R18, 0x00},
	{RF_BANK0,	RF_R19, 0x00},

	/*
		XO
	*/
	{RF_BANK0,	RF_R20, 0x22},
	{RF_BANK0,	RF_R21, 0x02},
	{RF_BANK0,	RF_R22, 0x29/*0x3F*/}, /* XTAL Freq offset, varies */ // TODO: read freq offset value from EEPROM
	//{RF_BANK0,	RF_R22, 0x3F}, /* XTAL Freq offset, varies */ // TODO: read freq offset value from EEPROM
	{RF_BANK0,	RF_R23, 0x00},
	{RF_BANK0,	RF_R24, 0x00}, // XTAL is the same as PLL frequency ref. No need to divide
	{RF_BANK0,	RF_R25, 0x00},

	/*
		PLL
	*/
	{RF_BANK0,	RF_R26, 0x00},
	{RF_BANK0,	RF_R27, 0x00},
	{RF_BANK0,	RF_R28, 0x20},
	{RF_BANK0,	RF_R30, 0x02}, /* Use Frac N accumulator mod, bit 0 following PLL N table */
	{RF_BANK0,	RF_R32, 0x00},
	{RF_BANK0,	RF_R35, 0x00}, /* Change setting to reduce spurs */
	{RF_BANK0,	RF_R36, 0x3F},
	{RF_BANK0,	RF_R37, 0x00},	

	/*
		LO Buffer
	*/
	{RF_BANK0,	RF_R38, 0x25},
	{RF_BANK0,	RF_R39, 0x30}, /* Adjust LO buffer tank */
	
	/*
		Test Ports
	*/
	{RF_BANK0,	RF_R64, 0x00},
	{RF_BANK0,	RF_R65, 0x00},
	{RF_BANK0,	RF_R66, 0x00},
	{RF_BANK0,	RF_R67, 0x00},

	/*
		ADC/DAC
	*/
	{RF_BANK0,	RF_R68, 0x00},
	{RF_BANK0,	RF_R69, 0x08},
	{RF_BANK0,	RF_R70, 0x08},
	{RF_BANK0,	RF_R71, 0x40},
	{RF_BANK0,	RF_R72, 0xD0},
	{RF_BANK0,	RF_R73, 0x93},
};
static UINT32 RT6590_RF_Central_RegTb_Size = (sizeof(RT6590_RF_Central_RegTb) / sizeof(BANK_RF_REG_PAIR));

static BANK_RF_REG_PAIR RT6590_RF_2G_Channel_0_RegTb[] = {
/*
	Bank 5 - Channel 0 2G RF registers	
*/
	/*
		RX logic operation
	*/
	{RF_BANK5,	RF_R00, 0x00}, /* Change in SelectBand6590 */
	{RF_BANK5,	RF_R01, 0x00},
	{RF_BANK5,	RF_R02, 0x0C}, /* WiFi RX0 only (Ext SW diverity): 0x0B, WiFi RX1 only (Ext SW diverity): 0x0C */
	{RF_BANK5,	RF_R03, 0x00},

	/*
		TX logic operation
	*/
	{RF_BANK5,	RF_R04, 0x00},
	{RF_BANK5,	RF_R05, 0x80},
	{RF_BANK5,	RF_R06, 0x02},

	/*
		LDO
	*/
	{RF_BANK5,	RF_R07, 0x00},
	{RF_BANK5,	RF_R08, 0x00},
	{RF_BANK5,	RF_R09, 0x00},

	/*
		RX
	*/
	{RF_BANK5,	RF_R10, 0x51},
	{RF_BANK5,	RF_R11, 0x22},
	{RF_BANK5,	RF_R12, 0x22},
	{RF_BANK5,	RF_R13, 0x0F},
	{RF_BANK5,	RF_R14, 0x47}, /* Increase mixer current for more gain */
	{RF_BANK5,	RF_R15, 0x25},
	{RF_BANK5,	RF_R16, 0xC7}, /* Tune LNA2 tank */
	{RF_BANK5,	RF_R17, 0x00},
	{RF_BANK5,	RF_R18, 0x00},
	{RF_BANK5,	RF_R19, 0x30}, /* Improve max Pin */
	{RF_BANK5,	RF_R20, 0x33},
	{RF_BANK5,	RF_R21, 0x02},
	{RF_BANK5,	RF_R22, 0x32}, /* Tune LNA1 tank */
	{RF_BANK5,	RF_R23, 0x00},
	{RF_BANK5,	RF_R24, 0x25},
	{RF_BANK5,	RF_R25, 0x13}, /* Tune LNA0 tank */
	{RF_BANK5,	RF_R26, 0x00},
	{RF_BANK5,	RF_R27, 0x12},
	{RF_BANK5,	RF_R28, 0x08},
	{RF_BANK5,	RF_R29, 0x00},

	/*
		LOGEN
	*/
	{RF_BANK5,	RF_R30, 0x41}, /* Tune LOGEN tank */
	{RF_BANK5,	RF_R31, 0x5D}, /* Improve 3.2GHz spur */
	{RF_BANK5,	RF_R32, 0x31},
	{RF_BANK5,	RF_R33, 0x31},
	{RF_BANK5,	RF_R34, 0x34},
	{RF_BANK5,	RF_R35, 0x00},
	{RF_BANK5,	RF_R36, 0x00},

	/*
		TX
	*/
	{RF_BANK5,	RF_R37, 0xDD}, /* Improve 3.2GHz spur */
	{RF_BANK5,	RF_R38, 0xB3},
	{RF_BANK5,	RF_R39, 0x33},
	{RF_BANK5,	RF_R40, 0xB3},
	{RF_BANK5,	RF_R41, 0x31},
	{RF_BANK5,	RF_R42, 0xF2},
	{RF_BANK5,	RF_R43, 0x41}, /* Tune mixer tank */
	{RF_BANK5,	RF_R44, 0x17},
	{RF_BANK5,	RF_R45, 0x02},
	{RF_BANK5,	RF_R46, 0x15},
	{RF_BANK5,	RF_R47, 0x00},
	{RF_BANK5,	RF_R48, 0x53},
	{RF_BANK5,	RF_R49, 0x03},
	{RF_BANK5,	RF_R50, 0x6F}, /* Adjust PA1 bias for better EVM at RF ALC switching */
	{RF_BANK5,	RF_R51, 0x67},
	{RF_BANK5,	RF_R52, 0x67},
	{RF_BANK5,	RF_R53, 0x67},
	{RF_BANK5,	RF_R54, 0xF0},
	{RF_BANK5,	RF_R55, 0xF0},
	{RF_BANK5,	RF_R56, 0xFF},
	{RF_BANK5,	RF_R57, 0xFF},
	{RF_BANK5,	RF_R58, 0x36},
	{RF_BANK5,	RF_R59, 0x36},
	{RF_BANK5,	RF_R60, 0x36},
	{RF_BANK5,	RF_R61, 0x36},
	{RF_BANK5,	RF_R62, 0x4F},
	{RF_BANK5,	RF_R63, 0x4F},
	{RF_BANK5,	RF_R64, 0x5F},
	{RF_BANK5, 	RF_R65, 0x5F},
	{RF_BANK5,	RF_R66, 0xF4},
	{RF_BANK5,	RF_R67, 0xF4},
	{RF_BANK5,	RF_R68, 0xE7},
	{RF_BANK5,	RF_R69, 0xE7},
	
	{RF_BANK5,	RF_R127, 0x04},
};
static UINT32 RT6590_RF_2G_Channel_0_RegTb_Size = (sizeof(RT6590_RF_2G_Channel_0_RegTb) / sizeof(BANK_RF_REG_PAIR));

static BANK_RF_REG_PAIR RT6590_RF_5G_Channel_0_RegTb[] = {
/*
	Bank 6 - Channel 0 5G RF registers	
*/
	/*
		RX logic operation
	*/
	{RF_BANK6,	RF_R00, 0x41}, // Change in SelectBand6590
	{RF_BANK6,	RF_R01, 0x00},
	{RF_BANK6,	RF_R02, 0x09}, // Int SW diverity , RX0: 0x0C, RX1: 0x09
	{RF_BANK6,	RF_R03, 0x00},

	/*
		TX logic operation
	*/
	{RF_BANK6,	RF_R04, 0x00},
	{RF_BANK6,	RF_R05, 0x80},
	{RF_BANK6,	RF_R06, 0x02},

	/*
		LDO
	*/
	{RF_BANK6,	RF_R07, 0x00},
	{RF_BANK6,	RF_R08, 0x00},
	{RF_BANK6,	RF_R09, 0x00},

	/*
		RX
	*/
	{RF_BANK6,	RF_R10, 0x00},
	{RF_BANK6,	RF_R11, 0x01},
	{RF_BANK6,	RF_R12, 0x33},
	{RF_BANK6,	RF_R13, 0x23}, /* LB:0x23, MB:0x26, HB:0x26 */
	{RF_BANK6,	RF_R14, 0x00},
	{RF_BANK6,	RF_R15, 0x04},
	{RF_BANK6,	RF_R16, 0x04},
	{RF_BANK6,	RF_R17, 0x03}, /* LB:0x03, MB:0x00, HB:0x00 */
	{RF_BANK6,	RF_R18, 0x08},
	{RF_BANK6,	RF_R19, 0x00},
	{RF_BANK6,	RF_R20, 0x00},
	{RF_BANK6,	RF_R21, 0x22},

	/*
		LOGEN5G
	*/
	{RF_BANK6,	RF_R24, 0xF1}, /* LB:0xF1, MB:0xC1, HB:0x81 */
	{RF_BANK6,	RF_R25, 0x76},
	{RF_BANK6,	RF_R26, 0x24},
	{RF_BANK6,	RF_R27, 0x04},

	/*
		TX
	*/
	{RF_BANK6,	RF_R37, 0xBB},
	{RF_BANK6,	RF_R38, 0xB3},
	{RF_BANK6,	RF_R39, 0x36}, /* LB:0x36, MB:0x34, HB:0x32 */
	{RF_BANK6,	RF_R40, 0x33},
	{RF_BANK6,	RF_R41, 0x33},
	{RF_BANK6,	RF_R42, 0x83},
	{RF_BANK6,	RF_R43, 0x1B},
	{RF_BANK6,	RF_R44, 0xB3},
	{RF_BANK6,	RF_R45, 0x5C}, /* LB:0x5C, MB:0x2C, HB:0x0C */
	{RF_BANK6,	RF_R46, 0x17},
	{RF_BANK6,	RF_R47, 0x02},
	{RF_BANK6,	RF_R48, 0x15},
	{RF_BANK6,	RF_R49, 0x07},
	{RF_BANK6,	RF_R50, 0x14},
	{RF_BANK6,	RF_R51, 0x14},
	{RF_BANK6,	RF_R52, 0x0B},
	{RF_BANK6,	RF_R53, 0x0B},
	{RF_BANK6, 	RF_R54, 0x04},
	{RF_BANK6,	RF_R55, 0x04},
	{RF_BANK6,	RF_R56, 0x1F},
	{RF_BANK6,	RF_R57, 0x1F},
	{RF_BANK6,	RF_R58, 0x04}, /* LB:0x04, MB:0x03, HB:0x02 */
	{RF_BANK6,	RF_R59, 0x04}, /* LB:0x04, MB:0x03, HB:0x02 */
	{RF_BANK6,	RF_R60, 0x00},
	{RF_BANK6,	RF_R61, 0x00},
	{RF_BANK6,	RF_R62, 0x00},
	{RF_BANK6,	RF_R63, 0x00},
	{RF_BANK6,	RF_R64, 0xF0},
	{RF_BANK6,	RF_R65, 0x0F},
	{RF_BANK6, 	RF_R66, 0x02},
	{RF_BANK6,	RF_R67, 0x02},
	{RF_BANK6,	RF_R68, 0x00},
	{RF_BANK6,	RF_R69, 0x00},
	
	{RF_BANK6,	RF_R127, 0x04},
};
static UINT32 RT6590_RF_5G_Channel_0_RegTb_Size = (sizeof(RT6590_RF_5G_Channel_0_RegTb) / sizeof(BANK_RF_REG_PAIR));

static BANK_RF_REG_PAIR RT6590_RF_VGA_Channel_0_RegTb[] = {
/*
	Bank 7 - Channel 0 VGA RF registers	
*/
	{RF_BANK7,	RF_R00, 0x7F}, /* Allow BBP/MAC to do calibration */
	{RF_BANK7,	RF_R01, 0x00},
	{RF_BANK7,	RF_R02, 0x00},
	{RF_BANK7,	RF_R03, 0x00},
	{RF_BANK7,	RF_R04, 0x00},
	{RF_BANK7,	RF_R05, 0x20}, // No DPD

	{RF_BANK7,	RF_R06, 0x0B}, // RX Filter: RX BQ1 Tuning, 20M:0x40, 40M:0x18, 80M:0x0B
	{RF_BANK7,	RF_R07, 0x0B}, // RX Filter: RX BQ2 Tuning, 20M:0x40, 40M:0x18, 80M:0x0B
	{RF_BANK7,	RF_R08, 0x00}, // AFC setting, 20M:0x03, 40M:0x01, 80M:0x00

	{RF_BANK7,	RF_R09, 0x60}, /* RX0: 0x40, RX1: 0x60 */
	{RF_BANK7,	RF_R10, 0x07},
	{RF_BANK7,	RF_R11, 0x01},
	{RF_BANK7,	RF_R12, 0x0A}, /* For DCOC */
	{RF_BANK7,	RF_R13, 0x12}, /* For DCOC */
	{RF_BANK7,	RF_R14, 0x1A}, /* For DCOC */
	{RF_BANK7,	RF_R15, 0x10}, /* For DCOC */
	{RF_BANK7,	RF_R16, 0x42}, /* For DCOC */
	{RF_BANK7,	RF_R17, 0x00},
	{RF_BANK7,	RF_R18, 0x00},
	{RF_BANK7,	RF_R19, 0x00},
	{RF_BANK7,	RF_R20, 0x00},
	{RF_BANK7,	RF_R21, 0xF1},
	{RF_BANK7,	RF_R22, 0x11},
	{RF_BANK7,	RF_R23, 0xC2},
	{RF_BANK7,	RF_R24, 0x41},
	{RF_BANK7,	RF_R25, 0x20},
	{RF_BANK7,	RF_R26, 0x40},
	{RF_BANK7,	RF_R27, 0xD7},
	{RF_BANK7,	RF_R28, 0xA2},
	{RF_BANK7,	RF_R29, 0x60},
	{RF_BANK7,	RF_R30, 0x49},
	{RF_BANK7,	RF_R31, 0x20},
	{RF_BANK7,	RF_R32, 0x44},
	{RF_BANK7,	RF_R33, 0xC1},
	{RF_BANK7,	RF_R34, 0x60},
	{RF_BANK7,	RF_R35, 0xC0},
	{RF_BANK7,	RF_R36, 0x00},
	{RF_BANK7,	RF_R37, 0x00},
	{RF_BANK7,	RF_R38, 0x00},
	{RF_BANK7,	RF_R39, 0x00},
	{RF_BANK7,	RF_R40, 0x00},
	{RF_BANK7,	RF_R41, 0x00},
	{RF_BANK7,	RF_R42, 0x00},
	{RF_BANK7,	RF_R43, 0x00},
	{RF_BANK7,	RF_R44, 0x00},
	{RF_BANK7,	RF_R45, 0x00},
	{RF_BANK7,	RF_R46, 0x00},
	{RF_BANK7,	RF_R47, 0x00},
	{RF_BANK7,	RF_R48, 0x00},
	{RF_BANK7,	RF_R49, 0x00},
	{RF_BANK7,	RF_R50, 0x00},
	{RF_BANK7,	RF_R51, 0x00},
	{RF_BANK7,	RF_R52, 0x00},
	{RF_BANK7,	RF_R53, 0x00},
	{RF_BANK7,	RF_R54, 0x00},
	{RF_BANK7,	RF_R55, 0x00},
	{RF_BANK7,	RF_R56, 0x00},
	{RF_BANK7,	RF_R57, 0x00},

	{RF_BANK7,	RF_R58, 0x10}, /* TX Filter 1 tuning, 20M:0x40, 40M:0x20, 80M:0x10 */
	{RF_BANK7,	RF_R59, 0x10}, /* TX Filter 2 tuning, 20M:0x40, 40M:0x20, 80M:0x10 */

	{RF_BANK7,	RF_R60, 0x8A}, /* Set Vcm of TX mixer=0.4 */
	{RF_BANK7,	RF_R61, 0x01},
	{RF_BANK7,	RF_R62, 0x00},
	{RF_BANK7,	RF_R63, 0x00},
	{RF_BANK7,	RF_R64, 0x00},
	{RF_BANK7,	RF_R65, 0x00},
	{RF_BANK7,	RF_R66, 0x00},
	{RF_BANK7,	RF_R67, 0x00},
	{RF_BANK7,	RF_R68, 0x00},
	{RF_BANK7,	RF_R69, 0x00},
	{RF_BANK7,	RF_R70, 0x69}, /* TZA resistor - 2G RX:0x00, 5G RX:0x69 */
	{RF_BANK7,	RF_R71, 0xB0}, /* TZA resistor - 2G RX:0x00, 5G RX:0xB0 */
	{RF_BANK7,	RF_R72, 0x00},
	{RF_BANK7,	RF_R73, 0x00},
	{RF_BANK7,	RF_R74, 0x00},
	{RF_BANK7,	RF_R75, 0x00},
	{RF_BANK7,	RF_R126, 0x00},
	{RF_BANK7,	RF_R127, 0x00},
};
static UINT32 RT6590_RF_VGA_Channel_0_RegTb_Size = (sizeof(RT6590_RF_VGA_Channel_0_RegTb) / sizeof(BANK_RF_REG_PAIR));

static const RT6590_FREQ_ITEM RT6590_Frequency_Plan[] =
{

	/* Channel, Band,		PLL_N,	Pll_k, 	Rdiv,	Non-Sigma, 	Den,	LFC_R33,	LFC_R34,	Pll_idiv 	Pll_LDO */
		{1,		RF_G_BAND,	0x28, 	0x02, 	0x03, 	0x40, 		0x02,		0x14,		0x5D,		0x28,	0x00},
		{2, 	RF_G_BAND,	0xA1, 	0x02, 	0x01, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{3, 	RF_G_BAND,	0x50, 	0x0B, 	0x00, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{4, 	RF_G_BAND,	0x50, 	0x09, 	0x00, 	0x40, 		0x02,		0x14,		0x5D,		0x28,	0x00},
		{5, 	RF_G_BAND,	0xA2, 	0x02, 	0x01, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{6, 	RF_G_BAND,	0xA2, 	0x07, 	0x01, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{7, 	RF_G_BAND,	0x28, 	0x07, 	0x03, 	0x40, 		0x02,		0x14,		0x5D,		0x28,	0x00},
		{8, 	RF_G_BAND,	0xA3, 	0x02, 	0x01, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{9, 	RF_G_BAND,	0x28, 	0x0D, 	0x03, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{10, 	RF_G_BAND,	0x51, 	0x09, 	0x00, 	0x40, 		0x02,		0x14,		0x5D,		0x28,	0x00},
		{11, 	RF_G_BAND,	0xA4, 	0x02, 	0x01, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{12, 	RF_G_BAND,	0xA4, 	0x07, 	0x01, 	0x40, 		0x07,		0x14,		0x5D,		0x3C,	0x00},
		{13, 	RF_G_BAND,	0x29, 	0x02, 	0x03, 	0x40, 		0x02,		0x14,		0x5D,		0x28,	0x00},
		{14, 	RF_G_BAND,	0x29, 	0x04, 	0x03, 	0x40, 		0x02,		0x14,		0x5D,		0x28,	0x00},

	/* Channel, Band,						PLL_N, 	PLL_K,	Rdiv,	Non-Sigma, 	Den,	LFC_R33,	LFC_R34,	Pll_idiv 	Pll_LDO*/
		{36, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x02, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{38, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x03, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{40, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x04, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{42, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x05, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{44, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x06, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{46, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x07, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{48, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x08, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{50, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x09, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{52, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x0A, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{54, 	(RF_A_BAND | RF_A_BAND_LB),	0x2B, 	0x0B, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{56, 	(RF_A_BAND | RF_A_BAND_LB),	0x2C, 	0x00, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{58, 	(RF_A_BAND | RF_A_BAND_LB),	0x2C, 	0x01, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{60, 	(RF_A_BAND | RF_A_BAND_LB),	0x2C, 	0x02, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{62, 	(RF_A_BAND | RF_A_BAND_LB),	0x2C, 	0x03, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{64, 	(RF_A_BAND | RF_A_BAND_LB),	0x2C, 	0x04, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{100, 	(RF_A_BAND | RF_A_BAND_MB),	0x2D, 	0x0A, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{102, 	(RF_A_BAND | RF_A_BAND_MB),	0x2D, 	0x0B, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{104, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x00, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{106, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x01, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{108, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x02, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{110, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x03, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{112, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x04, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{114, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x05, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{116, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x06, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{118, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x07, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{120, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x08, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{122, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x09, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{124, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x0A, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{126, 	(RF_A_BAND | RF_A_BAND_MB),	0x2E, 	0x0B, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{128, 	(RF_A_BAND | RF_A_BAND_MB),	0x2F, 	0x00, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{130, 	(RF_A_BAND | RF_A_BAND_MB),	0x2F, 	0x01, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{132, 	(RF_A_BAND | RF_A_BAND_MB),	0x2F, 	0x02, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{134, 	(RF_A_BAND | RF_A_BAND_MB),	0x2F, 	0x03, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{136, 	(RF_A_BAND | RF_A_BAND_MB),	0x2F, 	0x04, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{138, 	(RF_A_BAND | RF_A_BAND_MB),	0x2F, 	0x05, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{140, 	(RF_A_BAND | RF_A_BAND_MB),	0x2F, 	0x06, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{142, 	(RF_A_BAND | RF_A_BAND_HB),	0x2F, 	0x07, 	0x03,	0x40, 		0x04,		0x03,		0xDD,		0x30,	0x20},
		{144, 	(RF_A_BAND | RF_A_BAND_HB),	0x2F, 	0x08, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{146, 	(RF_A_BAND | RF_A_BAND_HB),	0x2F, 	0x09, 	0x03,	0x40, 		0x04,		0x12,		0xDD,		0x30,	0x20},
		{149, 	(RF_A_BAND | RF_A_BAND_HB),	0x2F, 	0x15, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{151, 	(RF_A_BAND | RF_A_BAND_HB),	0x2F, 	0x17, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{153, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x01, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{155, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x03, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{157, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x05, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{159, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x07, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{161, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x09, 	0x03,	0x40, 		0x10,		0x12,		0xDD,		0x60,	0x20},
		{163, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x0B, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{165, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x0D, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{167, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x0F, 	0x03,	0x40, 		0x10,		0x12,		0xDD,		0x60,	0x20},
		{169, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x11, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{171, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x13, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
		{173, 	(RF_A_BAND | RF_A_BAND_HB),	0x30, 	0x15, 	0x03,	0x40, 		0x10,		0x03,		0xDD,		0x60,	0x20},
};
UCHAR NUM_OF_6590_CHNL = (sizeof(RT6590_Frequency_Plan) / sizeof(RT6590_FREQ_ITEM));

static const RT6590_RF_SWITCH_ITEM RT6590_RF_BW_Switch[] =
{
	/*   Bank, 		Register,	Aband,	BW, 	Value */
		{RF_BANK7,	RF_R06,		0xFF,	BW_20,	0x40},
		{RF_BANK7,	RF_R06,		0xFF,	BW_40,	0x18},
		{RF_BANK7,	RF_R06,		0xFF,	BW_80,	0x0B},

		{RF_BANK7,	RF_R07,		0xFF,	BW_20,	0x40},
		{RF_BANK7,	RF_R07,		0xFF,	BW_40,	0x18},
		{RF_BANK7,	RF_R07,		0xFF,	BW_80,	0x0B},

		{RF_BANK7,	RF_R08,		0xFF,	BW_20,	0x03},
		{RF_BANK7,	RF_R08,		0xFF,	BW_40,	0x01},
		{RF_BANK7,	RF_R08,		0xFF,	BW_80,	0x00},

		{RF_BANK7,	RF_R58,		0xFF,	BW_20,	0x40},
		{RF_BANK7,	RF_R58,		0xFF,	BW_40,	0x20},
		{RF_BANK7,	RF_R58,		0xFF,	BW_80,	0x10},

		{RF_BANK7,	RF_R59,		0xFF,	BW_20,	0x40},
		{RF_BANK7,	RF_R59,		0xFF,	BW_40,	0x20},
		{RF_BANK7,	RF_R59,		0xFF,	BW_80,	0x10},
};
UCHAR RT6590_RF_BW_Switch_Size = (sizeof(RT6590_RF_BW_Switch) / sizeof(RT6590_RF_SWITCH_ITEM));

static const RT6590_RF_SWITCH_ITEM RT6590_RF_Band_Switch[] =
{
	/*   Bank, 		Register,	Band,			BW, 	Value */
		{RF_BANK0,	RF_R16,		RF_G_BAND,		0xFF,	0x00},
		{RF_BANK0,	RF_R16,		RF_A_BAND,		0xFF,	0x20},
		
		{RF_BANK0,	RF_R39,		RF_G_BAND,		0xFF,	0x34},
		{RF_BANK0,	RF_R39,		RF_A_BAND_LB,	0xFF,	0x31},
		{RF_BANK0,	RF_R39,		RF_A_BAND_MB,	0xFF,	0x30},
		{RF_BANK0,	RF_R39,		RF_A_BAND_HB,	0xFF,	0x30},
		
		{RF_BANK6,	RF_R13,		RF_A_BAND_LB,	0xFF,	0x23},
		{RF_BANK6,	RF_R13,		RF_A_BAND_MB,	0xFF,	0x26},
		{RF_BANK6,	RF_R13,		RF_A_BAND_HB,	0xFF,	0x26},

		{RF_BANK6,	RF_R15,		RF_A_BAND_LB,	0xFF,	0x04},
		{RF_BANK6,	RF_R15,		RF_A_BAND_MB,	0xFF,	0x04},
		{RF_BANK6,	RF_R15,		RF_A_BAND_HB,	0xFF,	0x04},

		{RF_BANK6,	RF_R16,		RF_A_BAND_LB,	0xFF,	0x04},
		{RF_BANK6,	RF_R16,		RF_A_BAND_MB,	0xFF,	0x04},
		{RF_BANK6,	RF_R16,		RF_A_BAND_HB,	0xFF,	0x05},

		{RF_BANK6,	RF_R17,		RF_A_BAND_LB,	0xFF,	0x03},
		{RF_BANK6,	RF_R17,		RF_A_BAND_MB,	0xFF,	0x00},
		{RF_BANK6,	RF_R17,		RF_A_BAND_HB,	0xFF,	0x00},

		{RF_BANK6,	RF_R24,		RF_A_BAND_LB,	0xFF,	0xF1},
		{RF_BANK6,	RF_R24,		RF_A_BAND_MB,	0xFF,	0xC1},
		{RF_BANK6,	RF_R24,		RF_A_BAND_HB,	0xFF,	0x81},

		{RF_BANK6,	RF_R39,		RF_A_BAND_LB,	0xFF,	0x36},
		{RF_BANK6,	RF_R39,		RF_A_BAND_MB,	0xFF,	0x34},
		{RF_BANK6,	RF_R39,		RF_A_BAND_HB,	0xFF,	0x32},

		{RF_BANK6,	RF_R45,		RF_A_BAND_LB,	0xFF,	0x5C},
		{RF_BANK6,	RF_R45,		RF_A_BAND_MB,	0xFF,	0x2C},
		{RF_BANK6,	RF_R45,		RF_A_BAND_HB,	0xFF,	0x0C},

		{RF_BANK6,	RF_R58,		RF_A_BAND_LB,	0xFF,	0x04},
		{RF_BANK6,	RF_R58,		RF_A_BAND_MB,	0xFF,	0x03},
		{RF_BANK6,	RF_R58,		RF_A_BAND_HB,	0xFF,	0x02},

		{RF_BANK6,	RF_R59,		RF_A_BAND_LB,	0xFF,	0x04},
		{RF_BANK6,	RF_R59,		RF_A_BAND_MB,	0xFF,	0x03},
		{RF_BANK6,	RF_R59,		RF_A_BAND_HB,	0xFF,	0x02},

		{RF_BANK7,	RF_R05,		RF_G_BAND,		0xFF,	0x20},
		{RF_BANK7,	RF_R05,		RF_A_BAND,		0xFF,	0x20},
		
		{RF_BANK7,	RF_R70,		RF_G_BAND,		0xFF,	0x00},
		{RF_BANK7,	RF_R70,		RF_A_BAND,		0xFF,	0x4B},

		{RF_BANK7,	RF_R71,		RF_G_BAND,		0xFF,	0x00},
		{RF_BANK7,	RF_R71,		RF_A_BAND,		0xFF,	0x20},
};
UCHAR RT6590_RF_Band_Switch_Size = (sizeof(RT6590_RF_Band_Switch) / sizeof(RT6590_RF_SWITCH_ITEM));


//2 REMOVE: For test chip only
//
// Initialize TC6008 configurations
//
VOID InitTC6008Config(
	IN PRTMP_ADAPTER pAd)
{
	ULONG MacReg = 0;
	ULONG BbValue = 0;
	
	DBGPRINT(RT_DEBUG_TRACE, ("%s: -->\n", __FUNCTION__));

	/*
		For A1 version: RF_MISC[0] = 0
		For A2 and A3 version: RF_MISC[0] = 1
		20120327 - MT7650 PCIe EVB is A2.
	*/
	RTMP_IO_READ32(pAd, RF_MISC, &MacReg);
	MacReg |= (0x1);
	RTMP_IO_WRITE32(pAd, RF_MISC, MacReg);

	//
	// Disable COEX_EN
	//
	//RTMP_IO_READ32(pAd, COEXCFG0, &MacReg);
	RTMP_IO_READ32(pAd, 0x40, &MacReg);
	MacReg &= 0xFFFFFFFE;
	RTMP_IO_WRITE32(pAd, 0x40, MacReg);
	//RTMP_IO_WRITE32(pAd, COEXCFG0, MacReg);

	RTMP_IO_WRITE32(pAd, CMB_CTRL, 0x0001D7FF);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: <--\n", __FUNCTION__));
}


//
// Initialize FCE
//
VOID InitFce(
	PRTMP_ADAPTER pAd)
{
	L2_STUFFING_STRUC L2Stuffing = {0};

	DBGPRINT(RT_DEBUG_TRACE, ("%s: -->\n", __FUNCTION__));

	RTMP_IO_READ32(pAd, FCE_L2_STUFF, &L2Stuffing.word);
	L2Stuffing.field.FS_WR_MPDU_LEN_EN = 0;
	RTMP_IO_WRITE32(pAd, FCE_L2_STUFF, L2Stuffing.word);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: <--\n", __FUNCTION__));
}


/*
	Select 2.4/5GHz band
*/
VOID SelectBand6590(
	IN PRTMP_ADAPTER pAd, 
	IN UCHAR Channel)
{
	UCHAR RfValue = 0;
	UINT32 MacValue = 0, IdReg = 0;

	if (!IS_RT6590(pAd))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Incorrect NIC\n", __FUNCTION__));
		
		return;
	}
	
	DBGPRINT(RT_DEBUG_INFO, ("%s: -->\n", __FUNCTION__));

	if (Channel <= 14) 
	{

		/*
			Select 2.4GHz 
		*/
		RFRandomWritePair(pAd, RT6590_RF_2G_Channel_0_RegTb, RT6590_RF_2G_Channel_0_RegTb_Size);

		rlt_rf_read(pAd, RF_BANK5, RF_R00, &RfValue);
		RfValue &= 0xFE; /* clear 0 = 0 */
		RfValue |= 0x41; /* set bit0 = 1:: 2.4GHz Enable */
		rlt_rf_write(pAd, RF_BANK5, RF_R00, RfValue);

		rlt_rf_read(pAd, RF_BANK6, RF_R00, &RfValue);
		RfValue &= 0xFE; /* clear 0 = 0:: 5GHz Disable */
		rlt_rf_write(pAd, RF_BANK6, RF_R00, RfValue);

		RandomWritePair(pAd, RT6590_BBP_InitRegTb_GBand, RT6590_BBP_InitRegTb_GBand_Tab_Size); 

		rtmp_mac_set_band(pAd, BAND_24G);

		/* 
			TRSW_EN, RF_TR, LNA_PE_G0 and PA_PE_G0
		*/
		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, 0x00050202); 

	}
	else
	{
		/*
			Select 5GHz 
		*/
		RFRandomWritePair(pAd, RT6590_RF_5G_Channel_0_RegTb, RT6590_RF_5G_Channel_0_RegTb_Size);
		
		rlt_rf_read(pAd, RF_BANK5, RF_R00, &RfValue);
		RfValue &= 0xFE; /* clear 0 = 0:: 2.4GHz Disable */
		rlt_rf_write(pAd, RF_BANK5, RF_R00, RfValue);

		rlt_rf_read(pAd, RF_BANK6, RF_R00, &RfValue);
		RfValue &= 0xFE; /* clear 0 = 0 */
		RfValue |= 0x41; /* set bit0 = 1:: 5GHz Enable */
		rlt_rf_write(pAd, RF_BANK6, RF_R00, RfValue);
		
		RandomWritePair(pAd, RT6590_BBP_InitRegTb_ABand, RT6590_BBP_InitRegTb_ABand_Tab_Size);

		rtmp_mac_set_band(pAd, BAND_5G);

		/*
			TRSW_EN, RF_TR, LNA_PE_A0 and PA_PE_A0
		*/
		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, 0x00050101);
	}

	for(IdReg = 0; IdReg < RT6590_RF_BW_Switch_Size; IdReg++)
	{
		if (pAd->CommonCfg.BBPCurrentBW == RT6590_RF_BW_Switch[IdReg].BW)
		{
			rlt_rf_write(pAd, 
						RT6590_RF_BW_Switch[IdReg].Bank,
						RT6590_RF_BW_Switch[IdReg].Register,
						RT6590_RF_BW_Switch[IdReg].Value);
		}
	}


	DBGPRINT(RT_DEBUG_INFO, ("%s: <--\n", __FUNCTION__));
}

/*
	Set RF channel frequency parameters:	
	Rdiv: R24[1:0]
	N: R29[7:0], R30[0]
	Nominator: R31[4:0]
	Non-Sigma: !SDM R31[7:5]
	Den: (Denomina - 8) R32[4:0]
	Loop Filter Config: R33, R34
	Pll_idiv: frac comp R35[6:0]
*/
VOID SetRfChFreqParameters6590(
	IN PRTMP_ADAPTER pAd, 
	IN UCHAR Channel)
{
	ULONG i = 0;
	UCHAR RFValue = 0, RfBand = 0;

	if (!IS_RT6590(pAd))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Incorrect NIC\n", __FUNCTION__));		
		return;
	}
	
	DBGPRINT(RT_DEBUG_INFO, ("%s: -->\n", __FUNCTION__));

	for (i = 0; i < NUM_OF_6590_CHNL; i++)
	{
		if (Channel == RT6590_Frequency_Plan[i].Channel)
		{
			RfBand = RT6590_Frequency_Plan[i].Band;			

			if (RT6590_Frequency_Plan[i].Band & RF_A_BAND)
			{
				/*
					5G only
					Pll_LDO: R16 [6:4] = <010>
				*/
				rlt_rf_read(pAd, RF_BANK0, RF_R16, &RFValue);
				RFValue &= ~(0x70);
				RFValue |= RT6590_Frequency_Plan[i].Pll_LDO;
				rlt_rf_write(pAd, RF_BANK0, RF_R16, RFValue);

				rlt_rf_read(pAd, RF_BANK0, RF_R16, &RFValue);
			}

			/*
				PLL_N: RF register bank 0
				R29[7:0]
				R30[0]				
			*/
			RFValue = RT6590_Frequency_Plan[i].PLL_N & 0x00FF;
			rlt_rf_write(pAd, RF_BANK0, RF_R29, RFValue);

			rlt_rf_read(pAd, RF_BANK0, RF_R30, &RFValue);
			RFValue &= ~(0x1);
			RFValue |= ((RT6590_Frequency_Plan[i].PLL_N >> 8) & 0x0001);
			rlt_rf_write(pAd, RF_BANK0, RF_R30, RFValue);

			/*
				PLL_K (Nominator): 
				R31[4:0] (5-bits)
			*/
			rlt_rf_read(pAd, RF_BANK0, RF_R31, &RFValue);
			RFValue &= ~(0x1F);
			RFValue |= RT6590_Frequency_Plan[i].PLL_K;
			rlt_rf_write(pAd, RF_BANK0, RF_R31, RFValue);
			
			/*
				Rdiv: RF register bank 0
				R24[1:0]
			*/
			rlt_rf_read(pAd, RF_BANK0, RF_R24, &RFValue);
			RFValue &= ~(0x3);
			RFValue |= RT6590_Frequency_Plan[i].Rdiv;
			rlt_rf_write(pAd, RF_BANK0, RF_R24, RFValue);

			/*
				Non-Sigma: !SDM 
				R31[7:5] (3-bits)
			*/
			rlt_rf_read(pAd, RF_BANK0, RF_R31, &RFValue);
			RFValue &= ~(0xE0);
			RFValue |= RT6590_Frequency_Plan[i].NonSigma;
			rlt_rf_write(pAd, RF_BANK0, RF_R31, RFValue);

			/*
				Den: (Denomina - 8) 
				R32[4:0]  (5-bits)
			*/
			rlt_rf_read(pAd, RF_BANK0, RF_R32, &RFValue);
			RFValue &= ~(0x1F);
			RFValue |= RT6590_Frequency_Plan[i].Den;
			rlt_rf_write(pAd, RF_BANK0, RF_R32, RFValue);

			/*
				Loop Filter Config: R33, R34
			*/
			rlt_rf_write(pAd, RF_BANK0, RF_R33, RT6590_Frequency_Plan[i].LFC_R33);
			rlt_rf_write(pAd, RF_BANK0, RF_R34, RT6590_Frequency_Plan[i].LFC_R34);
			
			/*
				Pll_idiv: frac comp 
				R35[6:0]  (7-bits)
			*/
			rlt_rf_read(pAd, RF_BANK0, RF_R35, &RFValue);
			RFValue &= ~(0x7F);
			RFValue |= RT6590_Frequency_Plan[i].Pll_idiv;
			rlt_rf_write(pAd, RF_BANK0, RF_R35, RFValue);			
			
			pAd->LatchRfRegs.Channel = Channel; /* Channel latch */

			DBGPRINT(RT_DEBUG_TRACE,
				("%s: SwitchChannel#%d(Band = 0x%02X, RF=%d, %dT), "
				"PLL_N=0x%02X, PLL_K=0x%02X, Rdiv=0x%02X, NonSigma=0x%02X, Den=0x%02X, R33=0x%02X, R34=0x%02X, Pll_idiv=0x%02X, Pll_LDO=0x%02X\n",
				__FUNCTION__,
				Channel,
				RfBand,
				pAd->RfIcType,
				pAd->Antenna.field.TxPath,
				RT6590_Frequency_Plan[i].PLL_N,
				RT6590_Frequency_Plan[i].PLL_K,
				RT6590_Frequency_Plan[i].Rdiv,
				RT6590_Frequency_Plan[i].NonSigma,
				RT6590_Frequency_Plan[i].Den,
				RT6590_Frequency_Plan[i].LFC_R33,
				RT6590_Frequency_Plan[i].LFC_R34,
				RT6590_Frequency_Plan[i].Pll_idiv,
				RT6590_Frequency_Plan[i].Pll_LDO));
			
			break;
		}
	}	

	for(i = 0; i < RT6590_RF_Band_Switch_Size; i++)
	{
		if (RT6590_RF_Band_Switch[i].Band & RfBand)
		{
			rlt_rf_write(pAd, 
						RT6590_RF_Band_Switch[i].Bank,
						RT6590_RF_Band_Switch[i].Register,
						RT6590_RF_Band_Switch[i].Value);
		}
	}
	
	if (RfBand & RF_A_BAND_LB)
	{
		RTMP_IO_WRITE32(pAd, TX0_RF_GAIN_CORR, 0x00000003);
		RTMP_IO_WRITE32(pAd, TX0_BB_GAIN_ATTEN, 0x00000000);
		RTMP_IO_WRITE32(pAd, TX_ALC_VGA3, 0x00050006);
	}
	else if (RfBand & RF_A_BAND_MB)
	{
		RTMP_IO_WRITE32(pAd, TX0_RF_GAIN_CORR, 0x02000005);
		RTMP_IO_WRITE32(pAd, TX0_BB_GAIN_ATTEN, 0x00000000);
		RTMP_IO_WRITE32(pAd, TX_ALC_VGA3, 0x00050006);
	}
	else
	{
		RTMP_IO_WRITE32(pAd, TX0_RF_GAIN_CORR, 0x003D0005);
		RTMP_IO_WRITE32(pAd, TX0_BB_GAIN_ATTEN, 0x00000000);
		RTMP_IO_WRITE32(pAd, TX_ALC_VGA3, 0x00050006);
	}

	DBGPRINT(RT_DEBUG_INFO, ("%s: <--\n", __FUNCTION__));
}

static VOID NICInitRT6590RFRegisters(RTMP_ADAPTER *pAd)
{

	UINT32 IdReg;
	UCHAR RFValue;

	printk("%s\n", __FUNCTION__);

	RFRandomWritePair(pAd, RT6590_RF_Central_RegTb, RT6590_RF_Central_RegTb_Size);

	RFRandomWritePair(pAd, RT6590_RF_2G_Channel_0_RegTb, RT6590_RF_2G_Channel_0_RegTb_Size);

	RFRandomWritePair(pAd, RT6590_RF_5G_Channel_0_RegTb, RT6590_RF_5G_Channel_0_RegTb_Size);

	RFRandomWritePair(pAd, RT6590_RF_VGA_Channel_0_RegTb, RT6590_RF_VGA_Channel_0_RegTb_Size);

	/* 
		vcocal_en (initiate VCO calibration (reset after completion)) - It should be at the end of RF configuration. 
	*/
	rlt_rf_read(pAd, RF_BANK0, RF_R04, &RFValue);
	RFValue = ((RFValue & ~0x80) | 0x80); 
	rlt_rf_write(pAd, RF_BANK0, RF_R04, RFValue);
	return;
}


/*
========================================================================
Routine Description:
	Initialize specific MAC registers.

Arguments:
	pAd					- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
static VOID NICInitRT6590MacRegisters(RTMP_ADAPTER *pAd)
{
	UINT32 MacReg = 0;

	/*
		Enable PBF and MAC clock
		SYS_CTRL[11:10] = 0x3
	*/

	printk("%s\n", __FUNCTION__);	

	RandomWritePair(pAd, RT6590_MACRegTable, RT6590_NUM_MAC_REG_PARMS);

	/*
		Release BBP and MAC reset
		MAC_SYS_CTRL[1:0] = 0x0
	*/
	RTMP_IO_READ32(pAd, MAC_SYS_CTRL, &MacReg);
	MacReg &= ~(0x3);
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, MacReg);

	InitTC6008Config(pAd); //2 REMOVE: for test chip only

	InitFce(pAd);
}


/*
========================================================================
Routine Description:
	Initialize specific BBP registers.

Arguments:
	pAd					- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
static VOID NICInitRT6590BbpRegisters(
	IN	PRTMP_ADAPTER pAd)
{

	printk("%s\n", __FUNCTION__);

	RandomWritePair(pAd, RT6590_BBP_InitRegTb_ABand, RT6590_BBP_InitRegTb_ABand_Tab_Size);		
	
	return;
}


static VOID RT6590_AsicAntennaDefaultReset(
	IN struct _RTMP_ADAPTER	*pAd,
	IN EEPROM_ANTENNA_STRUC *pAntenna)
{
	pAntenna->word = 0;
	pAntenna->field.RfIcType = 0xf;
	pAntenna->field.TxPath = 2;
	pAntenna->field.RxPath = 2;
}


static VOID RT6590_ChipBBPAdjust(RTMP_ADAPTER *pAd)
{
	static char *ext_str[]={"extNone", "extAbove", "", "extBelow"};
	UCHAR rf_bw, ext_ch;


#ifdef DOT11_N_SUPPORT
	if (get_ht_cent_ch(pAd, &rf_bw, &ext_ch) == FALSE)
#endif /* DOT11_N_SUPPORT */
	{
		rf_bw = BW_20;
		ext_ch = EXTCHA_NONE;
		pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel;
	}

#ifdef DOT11_VHT_AC
	if (WMODE_CAP(pAd->CommonCfg.PhyMode, WMODE_AC) &&
		(pAd->CommonCfg.Channel > 14) &&
		(rf_bw == BW_40) &&
		(pAd->CommonCfg.vht_bw == VHT_BW_80) &&
		(pAd->CommonCfg.vht_cent_ch != pAd->CommonCfg.CentralChannel))
	{
		rf_bw = BW_80;
		pAd->CommonCfg.vht_cent_ch = vht_cent_ch_freq(pAd, pAd->CommonCfg.Channel);
	}

//+++Add by shiang for debug
	DBGPRINT(RT_DEBUG_OFF, ("%s():rf_bw=%d, ext_ch=%d, PrimCh=%d, HT-CentCh=%d, VHT-CentCh=%d\n",
				__FUNCTION__, rf_bw, ext_ch, pAd->CommonCfg.Channel,
				pAd->CommonCfg.CentralChannel, pAd->CommonCfg.vht_cent_ch));
//---Add by shiang for debug
#endif /* DOT11_VHT_AC */

	rtmp_bbp_set_bw(pAd, rf_bw);

	/* TX/Rx : control channel setting */
	rtmp_mac_set_ctrlch(pAd, ext_ch);
	rtmp_bbp_set_ctrlch(pAd, ext_ch);
		
#ifdef DOT11_N_SUPPORT
	DBGPRINT(RT_DEBUG_TRACE, ("%s() : %s, ChannelWidth=%d, Channel=%d, ExtChanOffset=%d(%d) \n",
					__FUNCTION__, ext_str[ext_ch],
					pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth,
					pAd->CommonCfg.Channel,
					pAd->CommonCfg.RegTransmitSetting.field.EXTCHA,
					pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset));
#endif /* DOT11_N_SUPPORT */
}

static VOID RT6590_ChipSwitchChannel(
	struct _RTMP_ADAPTER *pAd,
	UCHAR Channel,
	BOOLEAN	 bScan)
{
	CHAR TxPwer = 0, TxPwer2 = DEFAULT_RF_TX_POWER; /* Bbp94 = BBPR94_DEFAULT, TxPwer2 = DEFAULT_RF_TX_POWER; */
	UINT IdReg;
	UCHAR OldRFValue = 0, RFValue = 0;
	UCHAR PreRFValue;
	UINT32 RegValue = 0;
	UCHAR Index;
	UINT32 Value = 0;

#ifdef CONFIG_AP_SUPPORT
#ifdef AP_QLOAD_SUPPORT
	/* clear all statistics count for QBSS Load */
	QBSS_LoadStatusClear(pAd);
#endif /* AP_QLOAD_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	/*
		Configure 2.4/5GHz before accessing other MAC/BB/RF registers
	*/
	SelectBand6590(pAd, Channel);

	/*
		Set RF channel frequency parameters (Rdiv, N, K, D and Ksd)
	*/
	SetRfChFreqParameters6590(pAd, Channel);

	// TODO: shall we re-write DOCO table here?
	for(IdReg=0; IdReg < RT6590_DCOC_Tab_Size; IdReg++)
	{
		RTMP_BBP_IO_WRITE32(pAd, RT6590_DCOC_Tab[IdReg].RegDate.Register,
				RT6590_DCOC_Tab[IdReg].RegDate.Value);
	}


	/* 
		vcocal_en (initiate VCO calibration (reset after completion)) - It should be at the end of RF configuration. 
	*/
	rlt_rf_read(pAd, RF_BANK0, RF_R04, &RFValue);
	RFValue = ((RFValue & ~0x80) | 0x80); 
	rlt_rf_write(pAd, RF_BANK0, RF_R04, RFValue);

	rtmp_bbp_set_bw(pAd, pAd->CommonCfg.BBPCurrentBW);


	for (Index = 0; Index < MAX_NUM_OF_CHANNELS; Index++)
	{
		if (Channel == pAd->TxPower[Index].Channel)
		{
			TxPwer = pAd->TxPower[Index].Power;
			break;
		}
	}

	RTMP_IO_READ32(pAd, TX_ALC_CFG_0, &Value);
	Value = Value & (~0x3F3F);
	Value |= TxPwer;
	Value |= (0x2F << 16);
	Value |= (0x2F << 24);
	RTMP_IO_WRITE32(pAd, TX_ALC_CFG_0, Value);

	/*
		IQ compensation
	*/
	RTMP_BBP_IO_WRITE32(pAd, CAL_R4, 0x000000FF);

	{	
		//UINT32 Data[5] = {0xaabbccdd, 0x88991122, 0x77886655, 0x33992288, 0xaa112233};
		UINT32 Data[100];
		//UINT32 i;
		//BurstWrite(pAd, 0x00411314, Data, 5);
		//BurstRead(pAd, 0x1030, 100, Data);


	}
	/*
	  On 11A, We should delay and wait RF/BBP to be stable
	  and the appropriate time should be 1000 micro seconds
	  2005/06/05 - On 11G, We also need this delay time. Otherwise it's difficult to pass the WHQL.
	*/
	RTMPusecDelay(1000);	
	return;
}


#ifdef CONFIG_STA_SUPPORT
static VOID RT6590_NetDevNickNameInit(RTMP_ADAPTER *pAd)
{
	snprintf((PSTRING) pAd->nickname, sizeof(pAd->nickname), "RT6590STA");
}
#endif /* CONFIG_STA_SUPPORT */

VOID RT6590_NICInitAsicFromEEPROM(
	IN PRTMP_ADAPTER		pAd)
{
	// TODO: wait TC6008 EEPROM format
}

/*
	NOTE: MAX_NUM_OF_CHANNELS shall  equal sizeof(txpwr_chlist))
*/
static UCHAR rt6590_txpwr_chlist[] = {
	1, 2,3,4,5,6,7,8,9,10,11,12,13,14,
	36,38,40,44,46,48,52,54,56,60,62,64,
	100,102,104,108,110,112,116,118,120,124,126,128,132,134,136,140,
	149,151,153,157,159,161,165,167,169,171,173,
	42, 58, 106, 122, 155,
};

INT RT6590_ReadChannelPwr(RTMP_ADAPTER *pAd)
{
	UINT32 i, choffset, idx, ss_idx, ss_offset_g, ss_num;
	EEPROM_TX_PWR_STRUC Power;
	CHAR tx_pwr1, tx_pwr2;

	/* Read Tx power value for all channels*/
	/* Value from 1 - 0x7f. Default value is 24.*/
	/* Power value : 2.4G 0x00 (0) ~ 0x1F (31)*/
	/*             : 5.5G 0xF9 (-7) ~ 0x0F (15)*/

	DBGPRINT(RT_DEBUG_TRACE, ("%s()--->\n", __FUNCTION__));
	
	choffset = 0;
	ss_num = 1;

	for (i = 0; i < sizeof(rt6590_txpwr_chlist); i++)
	{
		pAd->TxPower[i].Channel = rt6590_txpwr_chlist[i];
		pAd->TxPower[i].Power = DEFAULT_RF_TX_POWER;	
	}


	/* 0. 11b/g, ch1 - ch 14, 1SS */
	ss_offset_g = EEPROM_G_TX_PWR_OFFSET;
	for (i = 0; i < 7; i++)
	{
		idx = i * 2;
		RT28xx_EEPROM_READ16(pAd, ss_offset_g + idx, Power.word);

		tx_pwr1 = tx_pwr2 = DEFAULT_RF_TX_POWER;

		if ((Power.field.Byte0 <= 0x27) && (Power.field.Byte0 >= 0))
			tx_pwr1 = Power.field.Byte0;

		if ((Power.field.Byte1 <= 0x27) || (Power.field.Byte1 >= 0))
			tx_pwr2 = Power.field.Byte1;

		pAd->TxPower[idx].Power = tx_pwr1;
		pAd->TxPower[idx + 1].Power = tx_pwr2;
		choffset++;
	}



	{
		/* 1. U-NII lower/middle band: 36, 38, 40; 44, 46, 48; 52, 54, 56; 60, 62, 64 (including central frequency in BW 40MHz)*/
		ASSERT((pAd->TxPower[choffset].Channel == 36));
		choffset = 14;
		ASSERT((pAd->TxPower[choffset].Channel == 36));
		for (i = 0; i < 6; i++)
		{
			idx = i * 2;
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + idx, Power.word);

			if ((Power.field.Byte0 < 0x20) && (Power.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power = Power.field.Byte0;

			if ((Power.field.Byte1 < 0x20) && (Power.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power = Power.field.Byte1;
		}


		/* 2. HipperLAN 2 100, 102 ,104; 108, 110, 112; 116, 118, 120; 124, 126, 128; 132, 134, 136; 140 (including central frequency in BW 40MHz)*/
		choffset = 14 + 12;
		ASSERT((pAd->TxPower[choffset].Channel == 100));
		for (i = 0; i < 8; i++)
		{

			idx = i * 2;
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + idx, Power.word);
			
			if ((Power.field.Byte0 < 0x20) && (Power.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power = Power.field.Byte0;

			if ((Power.field.Byte1 < 0x20) && (Power.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power = Power.field.Byte1;
		}


		/* 3. U-NII upper band: 149, 151, 153; 157, 159, 161; 165, 167, 169; 171, 173 (including central frequency in BW 40MHz)*/
		choffset = 14 + 12 + 16;
		ASSERT((pAd->TxPower[choffset].Channel == 149));
		for (i = 0; i < 6; i++)
		{
			idx = i * 2;
			RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + idx, Power.word);

			if ((Power.field.Byte0 < 0x20) && (Power.field.Byte0 >= 0))
				pAd->TxPower[idx + choffset + 0].Power = Power.field.Byte0;

			if ((Power.field.Byte1 < 0x20) && (Power.field.Byte1 >= 0))
				pAd->TxPower[idx + choffset + 1].Power = Power.field.Byte1;
		}


		/* choffset = 14 + 12 + 16 + 7; */
		choffset = 14 + 12 + 16 + 11;

#ifdef DOT11_VHT_AC
		ASSERT((pAd->TxPower[choffset].Channel == 42));

		/* For VHT80MHz, we need assign tx power for central channel 42, 58, 106, 122, and 155 */
		DBGPRINT(RT_DEBUG_TRACE, ("%s: Update Tx power control of the central channel (42, 58, 106, 122 and 155) for VHT BW80\n", __FUNCTION__));
		
		NdisMoveMemory(&pAd->TxPower[53], &pAd->TxPower[16], sizeof(CHANNEL_TX_POWER)); // channel 42 = channel 40
		NdisMoveMemory(&pAd->TxPower[54], &pAd->TxPower[22], sizeof(CHANNEL_TX_POWER)); // channel 58 = channel 56
		NdisMoveMemory(&pAd->TxPower[55], &pAd->TxPower[28], sizeof(CHANNEL_TX_POWER)); // channel 106 = channel 104
		NdisMoveMemory(&pAd->TxPower[56], &pAd->TxPower[34], sizeof(CHANNEL_TX_POWER)); // channel 122 = channel 120
		NdisMoveMemory(&pAd->TxPower[57], &pAd->TxPower[44], sizeof(CHANNEL_TX_POWER)); // channel 155 = channel 153

		pAd->TxPower[choffset].Channel = 42;
		pAd->TxPower[choffset+1].Channel = 58;
		pAd->TxPower[choffset+2].Channel = 106;
		pAd->TxPower[choffset+3].Channel = 122;
		pAd->TxPower[choffset+4].Channel = 155;
		
		choffset += 5;		/* the central channel of VHT80 */
		
		choffset = (MAX_NUM_OF_CHANNELS - 1);
#endif /* DOT11_VHT_AC */


		/* 4. Print and Debug*/
		for (i = 0; i < choffset; i++)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("E2PROM: TxPower[%03d], Channel=%d, Power[Tx:%d]\n",
						i, pAd->TxPower[i].Channel, pAd->TxPower[i].Power));
		}
	}

	return TRUE;
}


VOID RT6590_AsicExtraPowerOverMAC(
	IN	PRTMP_ADAPTER 		pAd)
{
	ULONG	ExtraPwrOverMAC = 0;
	ULONG	ExtraPwrOverTxPwrCfg7 = 0, ExtraPwrOverTxPwrCfg8 = 0, ExtraPwrOverTxPwrCfg9 = 0;

	/* For OFDM_54 and HT_MCS_7, extra fill the corresponding register value into MAC 0x13D4 */
	RTMP_IO_READ32(pAd, 0x1318, &ExtraPwrOverMAC);  
	ExtraPwrOverTxPwrCfg7 |= (ExtraPwrOverMAC & 0x0000FF00) >> 8; /* Get Tx power for OFDM 54 */
	RTMP_IO_READ32(pAd, 0x131C, &ExtraPwrOverMAC);  
	ExtraPwrOverTxPwrCfg7 |= (ExtraPwrOverMAC & 0x0000FF00) << 8; /* Get Tx power for HT MCS 7 */			
	RTMP_IO_WRITE32(pAd, TX_PWR_CFG_7, ExtraPwrOverTxPwrCfg7);

		
	DBGPRINT(RT_DEBUG_INFO, ("Offset =0x13D8, TxPwr = 0x%08X, ", (UINT)ExtraPwrOverTxPwrCfg8));
	
	DBGPRINT(RT_DEBUG_INFO, ("Offset = 0x13D4, TxPwr = 0x%08X, Offset = 0x13DC, TxPwr = 0x%08X\n", 
		(UINT)ExtraPwrOverTxPwrCfg7, 
		(UINT)ExtraPwrOverTxPwrCfg9));
}

//
// VHT BW80 delta power control (+4~-4dBm) for per-rate Tx power control
//
#define EEPROM_VHT_BW80_TX_POWER_DELTA	(0x11E)

//
// Read per-rate Tx power
//
VOID RT6590ReadTxPwrPerRate(
	IN PRTMP_ADAPTER pAd)
{
	UINT32 data, DataBw40ABand, DataBw80ABand, DataBw40GBand;
	USHORT i, value, value2;
	INT TxPwrBw40ABand, TxPwrBw80ABand, TxPwrBw40GBand;
	UCHAR t1,t2,t3,t4;
	BOOLEAN bMinusBw40ABand = FALSE, bMinusBw80ABand = FALSE,bMinusBw40GBand = FALSE;

    DBGPRINT(RT_DEBUG_TRACE, ("%s() -->\n", __FUNCTION__));
	
	//
	// Get power delta for BW40
	//
	RT28xx_EEPROM_READ16(pAd, EEPROM_TXPOWER_DELTA, value2);
	TxPwrBw40ABand = 0;
	TxPwrBw40GBand = 0;

	if ((value2 & 0xFF) != 0xFF)
	{
		if (value2 & 0x80)
			TxPwrBw40GBand = (value2 & 0xF);
	
		if (value2 & 0x40)
			bMinusBw40GBand = FALSE;
		else
			bMinusBw40GBand = TRUE;
	}
	
	if ((value2 & 0xFF00) != 0xFF00)
	{
		if (value2 & 0x8000)
			TxPwrBw40ABand = ((value2&0x0F00) >> 8);

		if (value2 & 0x4000)
			bMinusBw40ABand = FALSE;
		else
			bMinusBw40ABand = TRUE;
	}

	//
	// Get power delta for BW80
	//
	RT28xx_EEPROM_READ16(pAd, EEPROM_VHT_BW80_TX_POWER_DELTA, value2);
	TxPwrBw80ABand = 0;

	if ((value2 & 0xFF) != 0xFF)
	{
		if (value2 & 0x80)
			TxPwrBw80ABand = (value2 & 0xF);
	
		if (value2 & 0x40)
			bMinusBw80ABand = FALSE;
		else
			bMinusBw80ABand = TRUE;
	}
	
	DBGPRINT(RT_DEBUG_TRACE, ("%s: bMinusBw40ABand = %d, bMinusBw80ABand = %d, bMinusBw40GBand = %d\n", 
		__FUNCTION__, 
		bMinusBw40ABand, 
		bMinusBw80ABand, 
		bMinusBw40GBand));

	DBGPRINT(RT_DEBUG_TRACE, ("%s: TxPwrBw40ABand = %d, TxPwrBw80ABand = %d, TxPwrBw40GBand = %d\n", 
		__FUNCTION__, 
		TxPwrBw40ABand, 
		TxPwrBw80ABand, 
		TxPwrBw40GBand));

	for (i = 0; i < 5; i++)
	{
		RT28xx_EEPROM_READ16(pAd, (EEPROM_TXPOWER_BYRATE_20MHZ_2_4G + (i * 4)), value);
		data = value;
		
		if (bMinusBw40ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40ABand);
			if (t1 > 0xF)
				t1 = 0xF;
			
			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40ABand);
			if (t2 > 0xF)
				t2 = 0xF;
			
			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40ABand);
			if (t3 > 0xF)
				t3 = 0xF;
			
			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40ABand)
				t1 = (value & 0xF) - (TxPwrBw40ABand);
			else
				t1 = 0;
			
			if (((value & 0xF0) >> 4) > TxPwrBw40ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40ABand);
			else
				t2 = 0;
			
			if (((value & 0xF00) >> 8) > TxPwrBw40ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40ABand);
			else
				t3 = 0;
			
			if (((value & 0xF000) >> 12) > TxPwrBw40ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40ABand);
			else
				t4 = 0;
		}
		
		DataBw40ABand = t1 + (t2 << 4) + (t3 << 8) + (t4 << 12);

		if (bMinusBw80ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw80ABand);
			if (t1 > 0xF)
				t1 = 0xF;
			
			t2 = ((value & 0xF0) >> 4) + (TxPwrBw80ABand);
			if (t2 > 0xF)
				t2 = 0xF;
			
			t3 = ((value & 0xF00) >> 8) + (TxPwrBw80ABand);
			if (t3 > 0xF)
				t3 = 0xF;
			
			t4 = ((value & 0xF000) >> 12) + (TxPwrBw80ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw80ABand)
				t1 = (value & 0xF) - (TxPwrBw80ABand);
			else
				t1 = 0;
			
			if (((value & 0xF0) >> 4) > TxPwrBw80ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw80ABand);
			else
				t2 = 0;
			
			if (((value & 0xF00) >> 8) > TxPwrBw80ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw80ABand);
			else
				t3 = 0;
			
			if (((value & 0xF000) >> 12) > TxPwrBw80ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw80ABand);
			else
				t4 = 0;
		}
		
		DataBw80ABand = t1 + (t2 << 4) + (t3 << 8) + (t4 << 12);
		
		if (bMinusBw40GBand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40GBand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40GBand);
			if (t2 > 0xF)
				t2 = 0xF;

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40GBand);
			if (t3 > 0xF)
				t3 = 0xF;

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40GBand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40GBand)
				t1 = (value & 0xF) - (TxPwrBw40GBand);
			else
				t1 = 0;
			
			if (((value & 0xF0) >> 4) > TxPwrBw40GBand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40GBand);
			else
				t2 = 0;

			if (((value & 0xF00) >> 8) > TxPwrBw40GBand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40GBand);
			else
				t3 = 0;

			if (((value & 0xF000) >> 12) > TxPwrBw40GBand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40GBand);
			else
				t4 = 0;
		}
		
		DataBw40GBand = t1 + (t2 << 4) + (t3 << 8) + (t4 << 12);
	
		RT28xx_EEPROM_READ16(pAd, (EEPROM_TXPOWER_BYRATE_20MHZ_2_4G + (i * 4) + 2), value);
		if (bMinusBw40ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40ABand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40ABand);
			if (t2 > 0xF)
				t2 = 0xF;

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40ABand);
			if (t3 > 0xF)
				t3 = 0xF;

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40ABand)
				t1 = (value & 0xF) - (TxPwrBw40ABand);
			else
				t1 = 0;

			if (((value & 0xF0) >> 4) > TxPwrBw40ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40ABand);
			else
				t2 = 0;

			if (((value & 0xF00) >> 8) > TxPwrBw40ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40ABand);
			else
				t3 = 0;

			if (((value & 0xF000) >> 12) > TxPwrBw40ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40ABand);
			else
				t4 = 0;
		}
		
		DataBw40ABand |= ((t1 << 16) + (t2 << 20) + (t3 << 24) + (t4 << 28));

		if (bMinusBw80ABand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw80ABand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw80ABand);
			if (t2 > 0xF)
				t2 = 0xF;

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw80ABand);
			if (t3 > 0xF)
				t3 = 0xF;

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw80ABand);
			if (t4 > 0xF)
				t4 = 0xF;
		}
		else
		{
			if ((value & 0xF) > TxPwrBw80ABand)
				t1 = (value & 0xF) - (TxPwrBw80ABand);
			else
				t1 = 0;

			if (((value & 0xF0) >> 4) > TxPwrBw80ABand)
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw80ABand);
			else
				t2 = 0;

			if (((value & 0xF00) >> 8) > TxPwrBw80ABand)
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw80ABand);
			else
				t3 = 0;

			if (((value & 0xF000) >> 12) > TxPwrBw80ABand)
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw80ABand);
			else
				t4 = 0;
		}
		
		DataBw80ABand |= ((t1 << 16) + (t2 << 20) + (t3 << 24) + (t4 << 28));
		
		if (bMinusBw40GBand == FALSE)
		{
			t1 = (value & 0xF) + (TxPwrBw40GBand);
			if (t1 > 0xF)
				t1 = 0xF;

			t2 = ((value & 0xF0) >> 4) + (TxPwrBw40GBand);
			if (t2 > 0xF)
			{
				t2 = 0xF;
			}

			t3 = ((value & 0xF00) >> 8) + (TxPwrBw40GBand);
			if (t3 > 0xF)
			{
				t3 = 0xF;
			}

			t4 = ((value & 0xF000) >> 12) + (TxPwrBw40GBand);
			if (t4 > 0xF)
			{
				t4 = 0xF;
			}
		}
		else
		{
			if ((value & 0xF) > TxPwrBw40GBand)
			{
				t1 = (value & 0xF) - (TxPwrBw40GBand);
			}
			else
			{
				t1 = 0;
			}

			if (((value & 0xF0) >> 4) > TxPwrBw40GBand)
			{
				t2 = ((value & 0xF0) >> 4) - (TxPwrBw40GBand);
			}
			else
			{
				t2 = 0;
			}

			if (((value & 0xF00) >> 8) > TxPwrBw40GBand)
			{
				t3 = ((value & 0xF00) >> 8) - (TxPwrBw40GBand);
			}
			else
			{
				t3 = 0;
			}

			if (((value & 0xF000) >> 12) > TxPwrBw40GBand)
			{
				t4 = ((value & 0xF000) >> 12) - (TxPwrBw40GBand);
			}
			else
			{
				t4 = 0;
			}
		}
		
		DataBw40GBand |= ((t1 << 16) + (t2 << 20) + (t3 << 24) + (t4 << 28));
		
		data |= (value << 16);
	
		pAd->Tx20MPwrCfgABand[i] = data;
		pAd->Tx20MPwrCfgGBand[i] = data;
		
		pAd->Tx40MPwrCfgABand[i] = DataBw40ABand;
		pAd->Tx80MPwrCfgABand[i] = DataBw80ABand;
		pAd->Tx40MPwrCfgGBand[i] = DataBw40GBand;
	
		if (data != 0xFFFFFFFF)
		{
			RTMP_IO_WRITE32(pAd, (TX_PWR_CFG_0 + (i * 4)), data);
		}
		
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%s: data = 0x%X,  DataBw40ABand = 0x%X,  DataBw80ABand = 0x%X, DataBw40GBand = 0x%x\n", 
			__FUNCTION__, data, DataBw40ABand, DataBw80ABand, DataBw40GBand));
	}

    DBGPRINT(RT_DEBUG_TRACE, ("%s: <--\n", __FUNCTION__));
}

/*
========================================================================
Routine Description:
	Initialize RT6590.

Arguments:
	pAd					- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
VOID RT6590_Init(RTMP_ADAPTER *pAd)
{
	RTMP_CHIP_OP *pChipOps = &pAd->chipOps;
	RTMP_CHIP_CAP *pChipCap = &pAd->chipCap;
	int idx;


	DBGPRINT(RT_DEBUG_TRACE, ("-->%s():\n", __FUNCTION__));
	
	pAd->RfIcType = RFIC_UNKNOWN;
	/* 
		Init chip capabilities
	*/
	pChipCap->MaxNss = 1;
	pChipCap->TXWISize = 20;
	pChipCap->RXWISize = 28;
	pChipCap->WPDMABurstSIZE = 3;
	
	// TODO: ++++++following parameters are not finialized yet!!
#ifdef RTMP_FLASH_SUPPORT
	pChipCap->eebuf = RT6590_EeBuffer;
#endif /* RTMP_FLASH_SUPPORT */

	pChipCap->SnrFormula = SNR_FORMULA2;
	pChipCap->FlgIsHwWapiSup = TRUE;
	pChipCap->VcoPeriod = 10;
	pChipCap->FlgIsVcoReCalMode = VCO_CAL_MODE_3;
	pChipCap->FlgIsHwAntennaDiversitySup = FALSE;
#ifdef STREAM_MODE_SUPPORT
	pChipCap->FlgHwStreamMode = FALSE;
#endif /* STREAM_MODE_SUPPORT */
#ifdef TXBF_SUPPORT
	pChipCap->FlgHwTxBfCap = FALSE;
#endif /* TXBF_SUPPORT */
#ifdef FIFO_EXT_SUPPORT
	pChipCap->FlgHwFifoExtCap = TRUE;
#endif /* FIFO_EXT_SUPPORT */


	pChipCap->asic_caps |= (fASIC_CAP_PMF_ENC);

	pChipCap->phy_caps = (fPHY_CAP_24G | fPHY_CAP_5G);
	pChipCap->phy_caps |= (fPHY_CAP_HT | fPHY_CAP_VHT);

	pChipCap->RfReg17WtMethod = RF_REG_WT_METHOD_STEP_ON;
		
	pChipCap->MaxNumOfRfId = MAX_RF_ID;
	pChipCap->pRFRegTable = NULL;

	pChipCap->MaxNumOfBbpId = 200;	
	pChipCap->pBBPRegTable = NULL;
	pChipCap->bbpRegTbSize = 0;

#ifdef NEW_MBSSID_MODE
	pChipCap->MBSSIDMode = MBSSID_MODE1;
#else
	pChipCap->MBSSIDMode = MBSSID_MODE0;
#endif /* NEW_MBSSID_MODE */


#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
	// TODO: shiang-6590, fix me for FrqCal in STA mode!!
	/* Frequence Calibration */
	pChipCap->FreqCalibrationSupport = FALSE;
	pChipCap->FreqCalInitMode = 0;
	pChipCap->FreqCalMode = 0;
	pChipCap->RxWIFrqOffset = 0;
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef RTMP_EFUSE_SUPPORT
	pChipCap->EFUSE_USAGE_MAP_START = 0x1e0;
	pChipCap->EFUSE_USAGE_MAP_END = 0x1fd;      
	pChipCap->EFUSE_USAGE_MAP_SIZE = 30;
#endif /* RTMP_EFUSE_SUPPORT */

	pChipCap->WlanMemmapOffset = 0x410000;
	pChipCap->InbandPacketMaxLen = 192;
	//pChipCap->InbandPacketMaxLen = 12;

	RTMP_DRS_ALG_INIT(pAd, RATE_ALG_GRP);
		
	/*
		Following function configure beacon related parameters
		in pChipCap
			FlgIsSupSpecBcnBuf / BcnMaxHwNum / 
			WcidHwRsvNum / BcnMaxHwSize / BcnBase[]
	*/
	rlt_bcn_buf_init(pAd);

	/*
		init operator
	*/

	/* BBP adjust */
	pChipOps->ChipBBPAdjust = RT6590_ChipBBPAdjust;
	
#ifdef CONFIG_STA_SUPPORT
	pChipOps->ChipAGCAdjust = NULL;
#endif /* CONFIG_STA_SUPPORT */

	/* Channel */
	pChipOps->ChipSwitchChannel = RT6590_ChipSwitchChannel;
	pChipOps->ChipAGCInit = NULL;

	pChipOps->AsicMacInit = NICInitRT6590MacRegisters;
	pChipOps->AsicBbpInit = NICInitRT6590BbpRegisters;
	pChipOps->AsicRfInit = NICInitRT6590RFRegisters;
	pChipOps->AsicRfTurnOn = NULL;

	pChipOps->AsicHaltAction = NULL;
	pChipOps->AsicRfTurnOff = NULL;
	pChipOps->AsicReverseRfFromSleepMode = NULL;
	pChipOps->AsicResetBbpAgent = NULL;
	
	/* MAC */

	/* EEPROM */
	pChipOps->NICInitAsicFromEEPROM = RT6590_NICInitAsicFromEEPROM;
	
	/* Antenna */
	pChipOps->AsicAntennaDefaultReset = RT6590_AsicAntennaDefaultReset;

	/* TX ALC */
	pChipOps->InitDesiredTSSITable = NULL;
 	pChipOps->ATETssiCalibration = NULL;
	pChipOps->ATETssiCalibrationExtend = NULL;
	pChipOps->AsicTxAlcGetAutoAgcOffset = NULL;
	pChipOps->ATEReadExternalTSSI = NULL;
	pChipOps->TSSIRatio = NULL;
	
	/* Others */
#ifdef CONFIG_STA_SUPPORT
	pChipOps->NetDevNickNameInit = RT6590_NetDevNickNameInit;
#endif /* CONFIG_STA_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
	pAd->chipCap.carrier_func = TONE_RADAR_V2;
	pChipOps->ToneRadarProgram = ToneRadarProgram_v2;
#endif /* CARRIER_DETECTION_SUPPORT */

	/* Chip tuning */
	pChipOps->RxSensitivityTuning = NULL;
	pChipOps->AsicTxAlcGetAutoAgcOffset = NULL;
	pChipOps->AsicGetTxPowerOffset = AsicGetTxPowerOffset;
	pChipOps->AsicExtraPowerOverMAC = RT6590_AsicExtraPowerOverMAC;

	/* MCU related callback functions */

#ifdef RTMP_USB_SUPPORT
	pChipOps->loadFirmware = USBLoadFirmwareToAndes;
#endif

	pChipOps->sendCommandToMcu = AsicSendCmdToAndes;

/* 
	Following callback functions already initiailized in RtmpChipOpsHook() 
	1. Power save related
*/
#ifdef GREENAP_SUPPORT
	pChipOps->EnableAPMIMOPS = NULL;
	pChipOps->DisableAPMIMOPS = NULL;
#endif /* GREENAP_SUPPORT */
	// TODO: ------Upper parameters are not finialized yet!!
}
#endif /* RT65xx */

