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
	rt2860.c

	Abstract:
	Specific funcitons and variables for RT3070

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
*/

#ifdef RT28xx

#include "rt_config.h"


VOID RT28xx_ch_tunning(RTMP_ADAPTER *pAd, INT bw)
{	
	if (pAd->MACVersion != 0x28600100)
		return;

	
	if (bw == BW_20)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x16);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x08);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x11);
	}
	else if (bw == BW_40)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x1A);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R70, 0x0A);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x16);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("!!!rt2860C !!! \n"));

}


VOID RT28xx_ChipSwitchChannel(
	IN PRTMP_ADAPTER 			pAd,
	IN UCHAR					Channel,
	IN BOOLEAN					bScan) 
{
	CHAR    TxPwer = 0, TxPwer2 = DEFAULT_RF_TX_POWER; /*Bbp94 = BBPR94_DEFAULT, TxPwer2 = DEFAULT_RF_TX_POWER;*/
	UCHAR	index;
	UINT32 	Value = 0; /*BbpReg, Value;*/
	UCHAR 	RFValue = 0;
	UINT32 i = 0;
	ULONG	R2 = 0, R3 = DEFAULT_RF_TX_POWER, R4 = 0;
	RTMP_RF_REGS *RFRegTable;
	CHAR lan_gain;


	/* Search Tx power value*/
	/*
		We can't use ChannelList to search channel, since some central channl's txpowr doesn't list 
		in ChannelList, so use TxPower array instead.
	*/
	for (index = 0; index < MAX_NUM_OF_CHANNELS; index++)
	{
		if (Channel == pAd->TxPower[index].Channel)
		{
			TxPwer = pAd->TxPower[index].Power;
			TxPwer2 = pAd->TxPower[index].Power2;
			break;
		}
	}


	if (index == MAX_NUM_OF_CHANNELS)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s(): Can't find the Channel#%d \n", __FUNCTION__, Channel));
	}

	RFRegTable = RF2850RegTable;

	switch (pAd->RfIcType)
	{
		case RFIC_2820:
		case RFIC_2850:
		case RFIC_2720:
		case RFIC_2750:
			for (index = 0; index < NUM_OF_2850_CHNL; index++)
			{
				if (Channel == RFRegTable[index].Channel)
				{
					R2 = RFRegTable[index].R2;
					if (pAd->Antenna.field.TxPath == 1)
					{
						R2 |= 0x4000;	/*If TXpath is 1, bit 14 = 1;*/
					}

					if ((pAd->Antenna.field.RxPath == 2)
#ifdef GREENAP_SUPPORT
						&& (pAd->ApCfg.bGreenAPActive == FALSE)
#endif /* GREENAP_SUPPORT */
						)
					{
						R2 |= 0x40;	/*write 1 to off Rxpath.*/
					}
					else if ((pAd->Antenna.field.RxPath == 1)
#ifdef GREENAP_SUPPORT
							|| (pAd->ApCfg.bGreenAPActive == TRUE)
#endif /* GREENAP_SUPPORT */
							)
					{
						R2 |= 0x20040;	/*write 1 to off RxPath*/
					}

					if (Channel > 14)
					{
						/* initialize R3, R4*/
						R3 = (RFRegTable[index].R3 & 0xffffc1ff);
						R4 = (RFRegTable[index].R4 & (~0x001f87c0)) | (pAd->RfFreqOffset << 15);

						/* 5G band power range: 0xF9~0X0F, TX0 Reg3 bit9/TX1 Reg4 bit6="0" means the TX power reduce 7dB*/
						/*R3*/
						if ((TxPwer >= -7) && (TxPwer < 0))
						{
							TxPwer = (7+TxPwer);

							/* TxPwer is not possible larger than 15 */

							R3 |= (TxPwer << 10);
							DBGPRINT(RT_DEBUG_TRACE, ("%s(): TxPwer=%d \n", __FUNCTION__, TxPwer));
						}
						else
						{
							TxPwer = (TxPwer > 0xF) ? (0xF) : (TxPwer);
							R3 |= (TxPwer << 10) | (1 << 9);
						}

						/* R4*/
						if ((TxPwer2 >= -7) && (TxPwer2 < 0))
						{
							TxPwer2 = (7+TxPwer2);
							
							R4 |= (TxPwer2 << 7);
							DBGPRINT(RT_DEBUG_TRACE, ("%s(): TxPwer2=%d \n", __FUNCTION__, TxPwer2));
						}
						else
						{
							TxPwer2 = (TxPwer2 > 0xF) ? (0xF) : (TxPwer2);
							R4 |= (TxPwer2 << 7) | (1 << 6);
						}                        
					}
					else
					{
						R3 = (RFRegTable[index].R3 & 0xffffc1ff) | (TxPwer << 9); /* set TX power0*/
						R4 = (RFRegTable[index].R4 & (~0x001f87c0)) | (pAd->RfFreqOffset << 15) | (TxPwer2 <<6);/* Set freq Offset & TxPwr1*/
					}

					/* Based on BBP current mode before changing RF channel.*/
					if (!bScan && (pAd->CommonCfg.BBPCurrentBW == BW_40)
#ifdef GREENAP_SUPPORT
						&& (pAd->ApCfg.bGreenAPActive == 0)
#endif /* GREENAP_SUPPORT */
						)
					{
						R4 |=0x200000;
					}

					/* Update variables*/
					pAd->LatchRfRegs.Channel = Channel;
					pAd->LatchRfRegs.R1 = RFRegTable[index].R1;
					pAd->LatchRfRegs.R2 = R2;
					pAd->LatchRfRegs.R3 = R3;
					pAd->LatchRfRegs.R4 = R4;

					/* Set RF value 1's set R3[bit2] = [0]*/
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
					RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

					RTMPusecDelay(200);

					/* Set RF value 2's set R3[bit2] = [1]*/
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
					RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 | 0x04));
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

					RTMPusecDelay(200);

					/* Set RF value 3's set R3[bit2] = [0]*/
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
					RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

					break;
				}
			}

			DBGPRINT(RT_DEBUG_TRACE, ("SwitchChannel#%d(RF=%d, Pwr0=%lu, Pwr1=%lu, %dT) to , R1=0x%08x, R2=0x%08x, R3=0x%08x, R4=0x%08x\n",
										Channel, 
										pAd->RfIcType, 
										(R3 & 0x00003e00) >> 9,
										(R4 & 0x000007c0) >> 6,
										pAd->Antenna.field.TxPath,
										pAd->LatchRfRegs.R1, 
										pAd->LatchRfRegs.R2, 
										pAd->LatchRfRegs.R3, 
										pAd->LatchRfRegs.R4));
			break;

		default:
			DBGPRINT(RT_DEBUG_TRACE, ("SwitchChannel#%d : unknown RFIC=%d\n", Channel, pAd->RfIcType));
			break;
	}	

	/* Change BBP setting during siwtch from a->g, g->a*/
	lan_gain = GET_LNA_GAIN(pAd);
	if (Channel <= 14)
	{
		ULONG	TxPinCfg = 0x00050F0A; /*Gary 2007/08/09 0x050A0A*/
		CHAR lan_gain = GET_LNA_GAIN(pAd);
			
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, (0x37 - lan_gain));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R63, (0x37 - lan_gain));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, (0x37 - lan_gain));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R86, 0);/*(0x44 - lan_gain));	According the Rory's suggestion to solve the middle range issue.*/

		/* Rx High power VGA offset for LNA select*/
		{
			if (pAd->NicConfig2.field.ExternalLNAForG)
			{
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R82, 0x62);
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R75, 0x46);
			}
			else
			{
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R82, 0x84);
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R75, 0x50);
			}
		}

		/* 5G band selection PIN, bit1 and bit2 are complement*/
		rtmp_mac_set_band(pAd, BAND_24G);

		/* Turn off unused PA or LNA when only 1T or 1R*/
		if (pAd->Antenna.field.TxPath == 1)
			TxPinCfg &= 0xFFFFFFF3;
		if (pAd->Antenna.field.RxPath == 1)
			TxPinCfg &= 0xFFFFF3FF;
		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, TxPinCfg);

		rtmp_bbp_set_filter_coefficient_ctrl(pAd, Channel);
	}
	else
	{
		ULONG	TxPinCfg = 0x00050F05;/*Gary 2007/8/9 0x050505*/
		UINT8	bbpValue;

		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, (0x37 - lan_gain));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R63, (0x37 - lan_gain));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, (0x37 - lan_gain));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R86, 0); /*(0x44 - lan_gain));   According the Rory's suggestion to solve the middle range issue.  */   

		/* Set the BBP_R82 value here */
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R82, 0xF2);


		/* Rx High power VGA offset for LNA select*/
		bbpValue = (pAd->NicConfig2.field.ExternalLNAForA) ? 0x46 : 0x50;
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R75, bbpValue);

		/* 5G band selection PIN, bit1 and bit2 are complement*/
		rtmp_mac_set_band(pAd, BAND_5G);

		/* Turn off unused PA or LNA when only 1T or 1R*/
		if (pAd->Antenna.field.TxPath == 1)
			TxPinCfg &= 0xFFFFFFF3;
		if (pAd->Antenna.field.RxPath == 1)
			TxPinCfg &= 0xFFFFF3FF;
		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, TxPinCfg);
	}

	/*
		On 11A, We should delay and wait RF/BBP to be stable
		and the appropriate time should be 1000 micro seconds 
		005/06/05 - On 11G, We also need this delay time. Otherwise it's difficult to pass the WHQL.

	*/
	RTMPusecDelay(1000);  
}

#endif /*RT28xx */

