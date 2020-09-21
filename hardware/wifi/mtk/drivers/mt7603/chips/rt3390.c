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
	rt3390.c

	Abstract:
	Specific funcitons and variables for RT3090

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
*/

#ifdef RT3390

#include "rt_config.h"


#ifndef RTMP_RF_RW_SUPPORT
#error "You Should Enable compile flag RTMP_RF_RW_SUPPORT for this chip"
#endif /* RTMP_RF_RW_SUPPORT */

#if 1
VOID NICInitRT3390RFRegisters(IN PRTMP_ADAPTER pAd)
{
	INT i;
	UINT8 RfReg;
	UINT32 data;
/*	CHAR bbpreg; */

	/* Driver must read EEPROM to get RfIcType before initial RF registers*/
	/* Initialize RF register to default value*/

	/* Init RF calibration*/
	/* Driver should toggle RF R30 bit7 before init RF registers*/

	RT30xxReadRFRegister(pAd, RF_R30, (PUCHAR)&RfReg);
	RfReg |= 0x80;
	RT30xxWriteRFRegister(pAd, RF_R30, (UCHAR)RfReg);
	RtmpusecDelay(1000);
	RfReg &= 0x7F;
	RT30xxWriteRFRegister(pAd, RF_R30, (UCHAR)RfReg);

	/* init R24, R31*/
/*	RT30xxWriteRFRegister(pAd, RF_R24, 0x0F);*/
/*	RT30xxWriteRFRegister(pAd, RF_R31, 0x0F);*/

#ifdef RELEASE_EXCLUDE
		/* RT3071 version E has fixed this issue*/
#endif /* RELEASE_EXCLUDE */
		if ((pAd->NicConfig2.field.DACTestBit == 1) && ((pAd->MACVersion & 0xffff) < 0x0211))
		{
#ifdef RELEASE_EXCLUDE
			/* patch tx EVM issue temporarily*/
#endif /* RELEASE_EXCLUDE */
			RTMP_IO_READ32(pAd, LDO_CFG0, &data);
			data = ((data & 0xE0FFFFFF) | 0x0D000000);
			RTMP_IO_WRITE32(pAd, LDO_CFG0, data);
		}
		else
		{
#ifdef RELEASE_EXCLUDE
			/* patch CCK ok, OFDM failed issue, just toggle and restore LDO_CFG0.*/
#endif /* RELEASE_EXCLUDE */
			RTMP_IO_READ32(pAd, LDO_CFG0, &data);
			data = ((data & 0xE0FFFFFF) | 0x0D000000);
			RTMP_IO_WRITE32(pAd, LDO_CFG0, data);

			RtmpusecDelay(1000);
			data = ((data & 0xE0FFFFFF) | 0x01000000);
			RTMP_IO_WRITE32(pAd, LDO_CFG0, data);
		}

	if (IS_RT3071(pAd) || IS_RT3390(pAd))
	{
#ifdef RELEASE_EXCLUDE
		/* patch LNA_PE_G1 failed issue*/
#endif /* RELEASE_EXCLUDE */
		RTMP_IO_READ32(pAd, GPIO_SWITCH, &data);
		data &= ~(0x20);
		RTMP_IO_WRITE32(pAd, GPIO_SWITCH, data);


		/* RF registers initialization*/
		for (i = 0; i < RT3390_NUM_RF_REG_PARMS; i++)
		{
			RT30xxWriteRFRegister(pAd, RT3390_RFRegTable[i].Register, RT3390_RFRegTable[i].Value);
		}

		/* Driver should set RF R6 bit6 on before calibration	*/
		RT30xxReadRFRegister(pAd, RF_R06, (PUCHAR)&RfReg);
		RfReg |= 0x40;
		RT30xxWriteRFRegister(pAd, RF_R06, (UCHAR)RfReg);

		if (IS_RT3390(pAd)) /* Disable RF filter calibration*/
		{      
			/* Disable RF filter calibration*/
			pAd->Mlme.CaliBW20RfR24 = BW20RFR24;
			pAd->Mlme.CaliBW40RfR24 = BW40RFR24;

			pAd->Mlme.CaliBW20RfR31 = BW20RFR31;
			pAd->Mlme.CaliBW40RfR31 = BW40RFR31;
		}
		else
		{
			/*For RF filter Calibration*/
			/*RT33xxFilterCalibration(pAd);*/
		}

		/* Initialize RF R27 register, set RF R27 must be behind RTMPFilterCalibration()*/
		if ((pAd->MACVersion & 0xffff) < 0x0211)
			RT30xxWriteRFRegister(pAd, RF_R27, 0x3);

		/* set led open drain enable*/
		RTMP_IO_READ32(pAd, OPT_14, &data);
		data |= 0x01;
		RTMP_IO_WRITE32(pAd, OPT_14, data);

		/* Initialize RT3090 serial MAc registers which is different from RT2860 serial*/
		RTMP_IO_WRITE32(pAd, TX_SW_CFG1, 0);

		/* RT3071 version E has fixed this issue*/
		if ((pAd->MACVersion & 0xffff) < 0x0211)
		{
			if (pAd->NicConfig2.field.DACTestBit == 1)
			{
			    RTMP_IO_WRITE32(pAd, TX_SW_CFG2, 0x1F);	/* To fix throughput drop drastically*/
			}
			else
			{
			    RTMP_IO_WRITE32(pAd, TX_SW_CFG2, 0x0F);	/* To fix throughput drop drastically*/
			}
		}
		else
		{
			RTMP_IO_WRITE32(pAd, TX_SW_CFG2, 0x0);
		}

		/* set default antenna as main*/
		if (pAd->RfIcType == RFIC_3320)
			AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt);

/*
		From RT3071 Power Sequence v1.1 document, the Normal Operation Setting Registers as follow :
		BBP_R138 / RF_R1 / RF_R15 / RF_R17 / RF_R20 / RF_R21.
 */
		/* RF power sequence setup, load RF normal operation-mode setup*/
		RT33xxLoadRFNormalModeSetup(pAd);
       }
}
#else
VOID NICInitRT3390RFRegisters(IN PRTMP_ADAPTER pAd)
{
		INT i;
	/* Driver must read EEPROM to get RfIcType before initial RF registers*/
	/* Initialize RF register to default value*/
	if (IS_RT3090(pAd)||IS_RT3390(pAd)||IS_RT3572(pAd))
	{
		/* Init RF calibration*/
		/* Driver should toggle RF R30 bit7 before init RF registers*/
		UINT32 RfReg = 0, data;
		
		RT30xxReadRFRegister(pAd, RF_R30, (PUCHAR)&RfReg);
		RfReg |= 0x80;
		RT30xxWriteRFRegister(pAd, RF_R30, (UCHAR)RfReg);
		RtmpusecDelay(1000);
		RfReg &= 0x7F;
		RT30xxWriteRFRegister(pAd, RF_R30, (UCHAR)RfReg);

		/* init R24, R31*/
		RT30xxWriteRFRegister(pAd, RF_R24, 0x0F);
		RT30xxWriteRFRegister(pAd, RF_R31, 0x0F);

		if (IS_RT3390(pAd))
		{
			/* patch LNA_PE_G1 failed issue*/
			RTMP_IO_READ32(pAd, GPIO_SWITCH, &data);
			data &= ~(0x20);
			RTMP_IO_WRITE32(pAd, GPIO_SWITCH, data);

			/* RF registers initialization*/
			for (i = 0; i < NUM_RF_REG_PARMS_OVER_RT3390; i++)
			{
				RT30xxWriteRFRegister(pAd, RT3390_RFRegTable[i].Register, RT3390_RFRegTable[i].Value);
			}
		}

		/* patch LNA_PE_G1 failed issue*/
		RTMP_IO_READ32(pAd, GPIO_SWITCH, &data);
		data &= ~(0x20);
		RTMP_IO_WRITE32(pAd, GPIO_SWITCH, data);

		/* Initialize RF register to default value*/
		for (i = 0; i < NUM_RF_REG_PARMS_OVER_RT3390; i++)
		{
			RT30xxWriteRFRegister(pAd, RT3390_RFRegTable[i].Register, RT3390_RFRegTable[i].Value);
		}

		/* Driver should set RF R6 bit6 on before calibration	*/
		RT30xxReadRFRegister(pAd, RF_R06, (PUCHAR)&RfReg);
		RfReg |= 0x40;
		RT30xxWriteRFRegister(pAd, RF_R06, (UCHAR)RfReg);

		/*For RF filter Calibration*/
		RTMPFilterCalibration(pAd);

		/* Initialize RF R27 register, set RF R27 must be behind RTMPFilterCalibration()*/
		if ((pAd->MACVersion & 0xffff) < 0x0211)
			RT30xxWriteRFRegister(pAd, RF_R27, 0x3);

		/* set led open drain enable*/
		RTMP_IO_READ32(pAd, OPT_14, &data);
		data |= 0x01;
		RTMP_IO_WRITE32(pAd, OPT_14, data);
		
		/* set default antenna as main*/
		if (pAd->RfIcType == RFIC_3020)
			AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt);

		/* add by johnli, RF power sequence setup, load RF normal operation-mode setup*/
		RT33xxLoadRFNormalModeSetup(pAd);
	}

}
#endif
#endif /* RT3390 */

