/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	cmm_sync.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	John Chang	2004-09-01      modified for rt2561/2661
*/
#include "rt_config.h"

/*BaSizeArray follows the 802.11n definition as MaxRxFactor.  2^(13+factor) bytes. When factor =0, it's about Ba buffer size =8.*/
UCHAR BaSizeArray[4] = {8,16,32,64};

extern COUNTRY_REGION_CH_DESC Country_Region_ChDesc_2GHZ[];
extern UINT16 const Country_Region_GroupNum_2GHZ;
extern COUNTRY_REGION_CH_DESC Country_Region_ChDesc_5GHZ[];
extern UINT16 const Country_Region_GroupNum_5GHZ;

/*
	==========================================================================
	Description:
		Update StaCfg->ChannelList[] according to 1) Country Region 2) RF IC type,
		and 3) PHY-mode user selected.
		The outcome is used by driver when doing site survey.

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
VOID BuildChannelList(RTMP_ADAPTER *pAd)
{
	UCHAR i, j;
	UINT16 index = 0, num = 0;
	PCH_DESC pChDesc = NULL;
	BOOLEAN bRegionFound = FALSE;
	PUCHAR pChannelList = NULL;
	PUCHAR pChannelListFlag = NULL;
#ifdef CFG80211_BUILD_CHANNEL_LIST
        PCH_DESC pChDesc2G = NULL, pChDesc5G = NULL;
#endif /* CFG80211_BUILD_CHANNEL_LIST */



	NdisZeroMemory(pAd->ChannelList, MAX_NUM_OF_CHANNELS * sizeof(CHANNEL_TX_POWER));

	/* if not 11a-only mode, channel list starts from 2.4Ghz band*/
	if (WMODE_CAP_2G(pAd->CommonCfg.PhyMode))
	{
		for (i = 0; i < Country_Region_GroupNum_2GHZ; i++)
		{
			if ((pAd->CommonCfg.CountryRegion & 0x7f) ==
				Country_Region_ChDesc_2GHZ[i].RegionIndex)
			{
				pChDesc = Country_Region_ChDesc_2GHZ[i].pChDesc;
				num = TotalChNum(pChDesc);
				bRegionFound = TRUE;
#ifdef CFG80211_BUILD_CHANNEL_LIST
                                pChDesc2G = pChDesc;
#endif /* CFG80211_BUILD_CHANNEL_LIST */

				break;
			}
		}

		if (!bRegionFound)
		{
			DBGPRINT(RT_DEBUG_ERROR,("CountryRegion=%d not support", pAd->CommonCfg.CountryRegion));
			return;
		}

		if (num > 0)
		{
			os_alloc_mem(NULL, (UCHAR **)&pChannelList, num * sizeof(UCHAR));

			if (!pChannelList)
			{
				DBGPRINT(RT_DEBUG_ERROR,("%s:Allocate memory for ChannelList failed\n", __FUNCTION__));
				return;
			}

			os_alloc_mem(NULL, (UCHAR **)&pChannelListFlag, num * sizeof(UCHAR));

			if (!pChannelListFlag)
			{
				DBGPRINT(RT_DEBUG_ERROR,("%s:Allocate memory for ChannelListFlag failed\n", __FUNCTION__));
				os_free_mem(NULL, pChannelList);
				return;
			}

			for (i = 0; i < num; i++)
			{
				pChannelList[i] = GetChannel_2GHZ(pChDesc, i);
				pChannelListFlag[i] = GetChannelFlag(pChDesc, i);
			}

			for (i = 0; i < num; i++)
			{
				for (j = 0; j < MAX_NUM_OF_CHANNELS; j++)
				{
					if (pChannelList[i] == pAd->TxPower[j].Channel)
						NdisMoveMemory(&pAd->ChannelList[index+i], &pAd->TxPower[j], sizeof(CHANNEL_TX_POWER));
						pAd->ChannelList[index + i].Flags = pChannelListFlag[i];
					// TODO: shiang-7603, NdisMoveMemory may replace the pAd->ChannelList[index+i].Channel as other values!
					pAd->ChannelList[index+i].Channel = pChannelList[i];
				}

#ifdef DOT11_N_SUPPORT
						if (N_ChannelGroupCheck(pAd, pAd->ChannelList[index + i].Channel))
							pAd->ChannelList[index + i].Flags |= CHANNEL_40M_CAP;
#ifdef DOT11_VHT_AC
						if (vht80_channel_group(pAd, pAd->ChannelList[index + i].Channel))
							pAd->ChannelList[index + i].Flags |= CHANNEL_80M_CAP;
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */

				pAd->ChannelList[index+i].MaxTxPwr = 20;
			}

			index += num;

			os_free_mem(NULL, pChannelList);
			os_free_mem(NULL, pChannelListFlag);
		}
		bRegionFound = FALSE;
		num = 0;
	}

	if (WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
	{
		for (i = 0; i < Country_Region_GroupNum_5GHZ; i++)
		{
			if ((pAd->CommonCfg.CountryRegionForABand & 0x7f) ==
				Country_Region_ChDesc_5GHZ[i].RegionIndex)
			{
				pChDesc = Country_Region_ChDesc_5GHZ[i].pChDesc;
				num = TotalChNum(pChDesc);
				bRegionFound = TRUE;
#ifdef CFG80211_BUILD_CHANNEL_LIST
                                pChDesc5G = pChDesc;
#endif /* CFG80211_BUILD_CHANNEL_LIST */

				break;
			}
		}

		if (!bRegionFound)
		{
			DBGPRINT(RT_DEBUG_ERROR,("CountryRegionABand=%d not support", pAd->CommonCfg.CountryRegionForABand));
			return;
		}

		if (num > 0)
		{
			UCHAR RadarCh[15]={52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140};
#ifdef CONFIG_AP_SUPPORT
			UCHAR q=0;
#endif /* CONFIG_AP_SUPPORT */
			os_alloc_mem(NULL, (UCHAR **)&pChannelList, num * sizeof(UCHAR));

			if (!pChannelList)
			{
				DBGPRINT(RT_DEBUG_ERROR,("%s:Allocate memory for ChannelList failed\n", __FUNCTION__));
				return;
			}

			os_alloc_mem(NULL, (UCHAR **)&pChannelListFlag, num * sizeof(UCHAR));

			if (!pChannelListFlag)
			{
				DBGPRINT(RT_DEBUG_ERROR,("%s:Allocate memory for ChannelListFlag failed\n", __FUNCTION__));
				os_free_mem(NULL, pChannelList);
				return;
			}

			for (i = 0; i < num; i++)
			{
				pChannelList[i] = GetChannel_5GHZ(pChDesc, i);
				pChannelListFlag[i] = GetChannelFlag(pChDesc, i);
			}

#ifdef CONFIG_AP_SUPPORT
			for (i = 0; i < num; i++)
			{
				if((pAd->CommonCfg.bIEEE80211H == 0)|| ((pAd->CommonCfg.bIEEE80211H == 1) && (pAd->CommonCfg.RDDurRegion != FCC)))
				{
					pChannelList[q] = GetChannel_5GHZ(pChDesc, i);
					pChannelListFlag[q] = GetChannelFlag(pChDesc, i);
					q++;
				}
				/*Based on the requiremnt of FCC, some channles could not be used anymore when test DFS function.*/
				else if ((pAd->CommonCfg.bIEEE80211H == 1) &&
						(pAd->CommonCfg.RDDurRegion == FCC) &&
						(pAd->Dot11_H.bDFSIndoor == 1))
				{
					if((GetChannel_5GHZ(pChDesc, i) < 116) || (GetChannel_5GHZ(pChDesc, i) > 128))
					{
						pChannelList[q] = GetChannel_5GHZ(pChDesc, i);
						pChannelListFlag[q] = GetChannelFlag(pChDesc, i);
						q++;
					}
				}
				else if ((pAd->CommonCfg.bIEEE80211H == 1) &&
						(pAd->CommonCfg.RDDurRegion == FCC) &&
						(pAd->Dot11_H.bDFSIndoor == 0))
				{
					if((GetChannel_5GHZ(pChDesc, i) < 100) || (GetChannel_5GHZ(pChDesc, i) > 140) )
					{
						pChannelList[q] = GetChannel_5GHZ(pChDesc, i);
						pChannelListFlag[q] = GetChannelFlag(pChDesc, i);
						q++;
					}
				}

			}
			num = q;
#endif /* CONFIG_AP_SUPPORT */

			for (i=0; i<num; i++)
			{
				for (j=0; j<MAX_NUM_OF_CHANNELS; j++)
				{
					if (pChannelList[i] == pAd->TxPower[j].Channel)
						NdisMoveMemory(&pAd->ChannelList[index+i], &pAd->TxPower[j], sizeof(CHANNEL_TX_POWER));
						pAd->ChannelList[index + i].Flags = pChannelListFlag[i];
					// TODO: shiang-7603, NdisMoveMemory may replace the pAd->ChannelList[index+i].Channel as other values!
					pAd->ChannelList[index+i].Channel = pChannelList[i];
				}

#ifdef DOT11_N_SUPPORT
				if (N_ChannelGroupCheck(pAd, pAd->ChannelList[index + i].Channel))
					pAd->ChannelList[index + i].Flags |= CHANNEL_40M_CAP;
#ifdef DOT11_VHT_AC
				if (vht80_channel_group(pAd, pAd->ChannelList[index + i].Channel))
					pAd->ChannelList[index + i].Flags |= CHANNEL_80M_CAP;
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */

				for (j=0; j<15; j++)
				{
					if (pChannelList[i] == RadarCh[j])
						pAd->ChannelList[index+i].DfsReq = TRUE;
				}
				pAd->ChannelList[index+i].MaxTxPwr = 20;
			}
			index += num;

			os_free_mem(NULL, pChannelList);
			os_free_mem(NULL, pChannelListFlag);
		}
	}

	pAd->ChannelListNum = (UCHAR)index;
	DBGPRINT(RT_DEBUG_TRACE,("CountryCode(2.4G/5G)=%d/%d, RFIC=%d, PHY mode=%d, support %d channels\n",
		pAd->CommonCfg.CountryRegion, pAd->CommonCfg.CountryRegionForABand, pAd->RfIcType, pAd->CommonCfg.PhyMode, pAd->ChannelListNum));

#ifdef RT_CFG80211_SUPPORT
	for (i=0; i<pAd->ChannelListNum; i++)
	{
		CFG80211OS_ChanInfoInit(
					pAd->pCfg80211_CB,
					i,
					pAd->ChannelList[i].Channel,
					pAd->ChannelList[i].MaxTxPwr,
					WMODE_CAP_N(pAd->CommonCfg.PhyMode),
					(pAd->CommonCfg.RegTransmitSetting.field.BW == BW_20));
	}

#ifdef CFG80211_BUILD_CHANNEL_LIST

   if (CFG80211OS_UpdateRegRuleByRegionIdx(
                    pAd->pCfg80211_CB,
                    pChDesc2G,
                    pChDesc5G
                    ) != 0)
    {
        DBGPRINT(RT_DEBUG_ERROR,("Update RegRule failed!\n"));
    }
#endif /* CFG80211_BUILD_CHANNEL_LIST */
#endif /* RT_CFG80211_SUPPORT */

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("SupportedChannelList:\n"));
	for (i=0; i<pAd->ChannelListNum; i++)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE,("\tChannel # %d: Pwr0/1 = %d/%d, Flags = %x\n ",
									 pAd->ChannelList[i].Channel,
									 pAd->ChannelList[i].Power,
									 pAd->ChannelList[i].Power2,
									 pAd->ChannelList[i].Flags));
	}
#endif
}


#ifdef P2P_CHANNEL_LIST_SEPARATE
VOID P2PBuildChannelList(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR i, index=0, num=0;
	PCH_DESC pChDesc = NULL;
	BOOLEAN bRegionFound = FALSE;
	PCFG80211_CTRL pCfg80211_Ctrl = &pAd->cfg80211_ctrl;

	NdisZeroMemory(pCfg80211_Ctrl->ChannelList, MAX_NUM_OF_CHANNELS * sizeof(CHANNEL_TX_POWER));

	/* if not 11a-only mode, channel list starts from 2.4Ghz band*/
	if (WMODE_CAP_2G(pAd->CommonCfg.PhyMode))
	{
		for (i = 0; i < Country_Region_GroupNum_2GHZ; i++)
		{
			if ((pCfg80211_Ctrl->CountryRegion & 0x7f) ==
				Country_Region_ChDesc_2GHZ[i].RegionIndex)
			{
				pChDesc = Country_Region_ChDesc_2GHZ[i].pChDesc;
				num = TotalChNum(pChDesc);
				bRegionFound = TRUE;
				break;
			}
		}

		if (!bRegionFound)
		{
			DBGPRINT(RT_DEBUG_ERROR,("%s::P2PCountryRegion=%d not support", __FUNCTION__, pCfg80211_Ctrl->CountryRegion));
			return;		
		}

		if (num > 0)
		{
			for (i = 0; i < num; i++)
			{
				pCfg80211_Ctrl->ChannelList[i].Channel = GetChannel_2GHZ(pChDesc, i);
				pCfg80211_Ctrl->ChannelList[i].Flags = GetChannelFlag(pChDesc, i);
			}

			index += num;
		}
		num = 0;
	}

	if (WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
	{
		for (i = 0; i < Country_Region_GroupNum_5GHZ; i++)
		{
			if ((pCfg80211_Ctrl->CountryRegionForABand & 0x7f) ==
				Country_Region_ChDesc_5GHZ[i].RegionIndex)
			{
				pChDesc = Country_Region_ChDesc_5GHZ[i].pChDesc;
				num = TotalChNum(pChDesc);
				bRegionFound = TRUE;
				break;
			}
		}

		if (!bRegionFound)
		{
			DBGPRINT(RT_DEBUG_ERROR,("%s::P2PCountryRegionForABand=%d not support", __FUNCTION__, pCfg80211_Ctrl->CountryRegionForABand));
			return;		
		}

		if (num > 0)
		{
			for (i = 0; i < num; i++)
			{
				pCfg80211_Ctrl->ChannelList[index + i].Channel = GetChannel_5GHZ(pChDesc, i);
				pCfg80211_Ctrl->ChannelList[index + i].Flags = GetChannelFlag(pChDesc, i);
			}

			index += num;
		}
	}

	pCfg80211_Ctrl->ChannelListNum = index;	

	DBGPRINT(RT_DEBUG_TRACE, ("%s::Channel list nubmer = %d\n", __FUNCTION__, pCfg80211_Ctrl->ChannelListNum));
}
#endif /* P2P_CHANNEL_LIST_SEPARATE */



/*
	==========================================================================
	Description:
		This routine return the first channel number according to the country
		code selection and RF IC selection (signal band or dual band). It is called
		whenever driver need to start a site survey of all supported channels.
	Return:
		ch - the first channel number of current country code setting

	IRQL = PASSIVE_LEVEL

	==========================================================================
 */
UCHAR FirstChannel(RTMP_ADAPTER *pAd)
{
	return pAd->ChannelList[0].Channel;
}


/*
	==========================================================================
	Description:
		This routine returns the next channel number. This routine is called
		during driver need to start a site survey of all supported channels.
	Return:
		next_channel - the next channel number valid in current country code setting.
	Note:
		return 0 if no more next channel
	==========================================================================
 */
UCHAR NextChannel(RTMP_ADAPTER *pAd, UCHAR channel)
{
	int i;
	UCHAR next_channel = 0;
#ifdef P2P_SUPPORT
	UCHAR	CurrentChannel = channel;

	if (pAd->ScanCtrl.ScanType == SCAN_P2P_SEARCH)
	{
		if (IS_P2P_LISTEN(pAd))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("Error !! P2P Discovery state machine has change to Listen state during scanning !\n"));
			return next_channel;
		}

		for (i = 0; i < (pAd->P2pCfg.P2pProprietary.ListenChanelCount - 1); i++)
		{
			if (CurrentChannel == pAd->P2pCfg.P2pProprietary.ListenChanel[i])
				next_channel = pAd->P2pCfg.P2pProprietary.ListenChanel[i+1];
		}
		P2P_INC_CHA_INDEX(pAd->P2pCfg.P2pProprietary.ListenChanelIndex, pAd->P2pCfg.P2pProprietary.ListenChanelCount);
		if (next_channel == CurrentChannel)
		{
			DBGPRINT(RT_DEBUG_INFO, ("SYNC -  next_channel equals to CurrentChannel= %d\n", next_channel));
			DBGPRINT(RT_DEBUG_INFO, ("SYNC -  ListenChannel List : %d  %d  %d\n",
						pAd->P2pCfg.P2pProprietary.ListenChanel[0],
						pAd->P2pCfg.P2pProprietary.ListenChanel[1],
						pAd->P2pCfg.P2pProprietary.ListenChanel[2]));

			next_channel = 0;
		}

#if 0
		if (!INFRA_ON(pAd) && (next_channel == 0))
			pAd->CommonCfg.Channel = pAd->P2pCfg.ListenChannel;
#endif
		DBGPRINT(RT_DEBUG_INFO, ("SYNC - P2P Scan return channel = %d. Listen Channel = %d.\n",
					next_channel, pAd->CommonCfg.Channel));

		return next_channel;
	}
#endif /* P2P_SUPPORT */

#ifdef P2P_CHANNEL_LIST_SEPARATE
	PCFG80211_CTRL pCfg80211_Ctrl = &pAd->cfg80211_ctrl;
#endif /* P2P_CHANNEL_LIST_SEPARATE */

#ifdef P2P_CHANNEL_LIST_SEPARATE
	if ((pAd->ScanCtrl.ScanType == SCAN_P2P))
	{
		for (i = 0; i < (pCfg80211_Ctrl->ChannelListNum - 1); i++)
		{
			if (channel == pCfg80211_Ctrl->ChannelList[i].Channel)
			{
				next_channel = pCfg80211_Ctrl->ChannelList[i+1].Channel;
				return next_channel;
			}
		}
	}
#endif /* P2P_CHANNEL_LIST_SEPARATE */




	for (i = 0; i < (pAd->ChannelListNum - 1); i++)
	{
		if (channel == pAd->ChannelList[i].Channel)
		{
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
			/* Only scan effected channel if this is a SCAN_2040_BSS_COEXIST*/
			/* 2009 PF#2: Nee to handle the second channel of AP fall into affected channel range.*/
			if ((pAd->ScanCtrl.ScanType == SCAN_2040_BSS_COEXIST) && (pAd->ChannelList[i+1].Channel >14))
			{
				channel = pAd->ChannelList[i+1].Channel;
				continue;
			}
			else
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
			{
				/* Record this channel's idx in ChannelList array.*/
			next_channel = pAd->ChannelList[i+1].Channel;
			break;
	}
		}
	}
	return next_channel;
}


/*
	==========================================================================
	Description:
		This routine is for Cisco Compatible Extensions 2.X
		Spec31. AP Control of Client Transmit Power
	Return:
		None
	Note:
	   Required by Aironet dBm(mW)
		   0dBm(1mW),   1dBm(5mW), 13dBm(20mW), 15dBm(30mW),
		  17dBm(50mw), 20dBm(100mW)

	   We supported
		   3dBm(Lowest), 6dBm(10%), 9dBm(25%), 12dBm(50%),
		  14dBm(75%),   15dBm(100%)

		The client station's actual transmit power shall be within +/- 5dB of
		the minimum value or next lower value.
	==========================================================================
 */
VOID ChangeToCellPowerLimit(RTMP_ADAPTER *pAd, UCHAR AironetCellPowerLimit)
{
	/*
		valud 0xFF means that hasn't found power limit information
		from the AP's Beacon/Probe response
	*/
	if (AironetCellPowerLimit == 0xFF)
		return;

	if (AironetCellPowerLimit < 6) /*Used Lowest Power Percentage.*/
		pAd->CommonCfg.TxPowerPercentage = 6;
	else if (AironetCellPowerLimit < 9)
		pAd->CommonCfg.TxPowerPercentage = 10;
	else if (AironetCellPowerLimit < 12)
		pAd->CommonCfg.TxPowerPercentage = 25;
	else if (AironetCellPowerLimit < 14)
		pAd->CommonCfg.TxPowerPercentage = 50;
	else if (AironetCellPowerLimit < 15)
		pAd->CommonCfg.TxPowerPercentage = 75;
	else
		pAd->CommonCfg.TxPowerPercentage = 100; /*else used maximum*/

	if (pAd->CommonCfg.TxPowerPercentage > pAd->CommonCfg.TxPowerDefault)
		pAd->CommonCfg.TxPowerPercentage = pAd->CommonCfg.TxPowerDefault;

}


CHAR ConvertToRssi(RTMP_ADAPTER *pAd, struct raw_rssi_info *rssi_info, UCHAR rssi_idx)
{
	UCHAR RssiOffset, LNAGain;
	CHAR BaseVal;
	CHAR rssi;

	/* Rssi equals to zero or rssi_idx larger than 3 should be an invalid value*/
	if (rssi_idx >= 3)
		return -99;

	rssi = rssi_info->raw_rssi[rssi_idx];

	if (rssi == 0)
		return -99;

	LNAGain = pAd->hw_cfg.lan_gain;

	if (pAd->LatchRfRegs.Channel > 14)
		RssiOffset = pAd->ARssiOffset[rssi_idx];
	else
		RssiOffset = pAd->BGRssiOffset[rssi_idx];

	BaseVal = -12;
#ifdef RT6352
	if (IS_RT6352(pAd))
		BaseVal = -2;
#endif /* RT6352 */

#if defined (MT7603) || defined (MT7628)
	if (IS_MT7603(pAd) || IS_MT7628(pAd))
		return (rssi + (CHAR)RssiOffset - (CHAR)LNAGain);
#endif

#ifdef RT65xx
	/*
		Recommended by CSD team about MT76x0:
		SW/QA owners should read the "external-LNA gain" and "RSSI OFFSET" content in EEPROM as "SIGNED".
		2.4G : RSSI_report = RSSI_bpp + EEPROM_0x46[15:8 or 7:0] - EEPROM_0x44[7:0]
		5G : RSSI_report = RSSI_bbp + EEPROM_0x4A[15:8 or 7:0] - EEPROM_0x44 or 0x48 or 0x4c[15:8]
	*/
	if (IS_MT76x0(pAd))
		return (rssi + (CHAR)RssiOffset - (CHAR)LNAGain);

	if (IS_MT76x2(pAd)) {
		if (pAd->LatchRfRegs.Channel > 14)
			return (rssi + pAd->ARssiOffset[rssi_idx] - (CHAR)LNAGain);
		else
			return (rssi + pAd->BGRssiOffset[rssi_idx] - (CHAR)LNAGain);
	}

	if (IS_RT8592(pAd))
		return (rssi - LNAGain - RssiOffset);
#endif /* RT65xx */

#ifdef MT7601
	// TODO: shiang-usw, fix me for AntSel and BW for MT7601!!!
	if (IS_MT7601(pAd))
	{
		CHAR LNA, RSSI;
		CHAR *LNATable;
		UCHAR AntSel = rssi_info->raw_rssi[1]; /* Antenna ID */
		UCHAR BW = rssi_info->raw_rssi[2]; /* BW */

/*
		CHAR MainBW40LNA[] = { 1, 18, 35 };
		CHAR MainBW20LNA[] = { 1, 18, 36 };
		CHAR AuxBW40LNA[] = { 1, 23, 42 };
		CHAR AuxBW20LNA[] = { 1, 23, 42 };
*/
		CHAR MainBW40LNA[] = { 0, 16, 34 };
		CHAR MainBW20LNA[] = { -2, 15, 33 };
		CHAR AuxBW40LNA[] = { -2, 16, 34 };
		CHAR AuxBW20LNA[] = { -2, 15, 33 };

		LNA = (rssi >> 6) & 0x3;
		RSSI = rssi & 0x3F;

		if ( (AntSel >> 7) == 0 )
		{
			if (BW == BW_40)
				LNATable = MainBW40LNA;
			else
				LNATable = MainBW20LNA;
		}
		else
		{
			if (BW == BW_40)
				LNATable = AuxBW40LNA;
			else
				LNATable = AuxBW20LNA;
		}

		if ( LNA == 3 )
			LNA = LNATable[2];
		else if ( LNA == 2 )
			LNA = LNATable[1];
		else
			LNA = LNATable[0];

		return ( 8 - LNA - RSSI - LNAGain - RssiOffset );
	}
	else
#endif /* MT7601 */
		return (BaseVal - RssiOffset - LNAGain - rssi);
}


CHAR ConvertToSnr(RTMP_ADAPTER *pAd, UCHAR Snr)
{
	if (pAd->chipCap.SnrFormula == SNR_FORMULA2)
		return (Snr * 3 + 8) >> 4;
	else if (pAd->chipCap.SnrFormula == SNR_FORMULA3)
		return (Snr * 3 / 16 ); /* * 0.1881 */
	else
		return (Snr - 16);
		/*return ((0xeb	- Snr) * 3) /	16 ; */
}


#ifdef WIDI_SUPPORT
#ifdef CONFIG_STA_SUPPORT
/*
	==========================================================================
	Description:
		SendProbeRequest
	==========================================================================
 */
VOID WidiSendProbeRequest(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR * destMac,
	IN UCHAR ssidLen,
	IN CHAR * ssidStr,
	IN UCHAR * deviceName,
	IN UCHAR * primaryDeviceType,
	IN UCHAR * vendExt,
	IN USHORT vendExtLen,
	IN UCHAR channel)
{
	HEADER_802_11   Hdr80211;
	PUCHAR          pOutBuffer = NULL;
	NDIS_STATUS     NStatus;
	ULONG           FrameLen = 0;
	UCHAR           SsidLen = 0, ScanType = pAd->MlmeAux.ScanType;
	UINT			ScanTimeIn5gChannel = SHORT_CHANNEL_TIME;

	pAd->StaCfg.bSendingProbe = 1;

	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (MONITOR_ON(pAd))
			return;
	}

	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		/* BBP and RF are not accessible in PS mode, we has to wake them up first */
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
			AsicForceWakeup(pAd, TRUE);

		/* leave PSM during scanning. otherwise we may lost ProbeRsp & BEACON */
		if (pAd->StaCfg.Psm == PWR_SAVE)
			RTMP_SET_PSM_BIT(pAd, PWR_ACTIVE);
	}

	AsicSwitchChannel(pAd, channel, FALSE);
	AsicLockChannel(pAd, channel);

	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (pAd->MlmeAux.Channel > 14)
		{
			if ((pAd->CommonCfg.bIEEE80211H == 1) &&
				RadarChannelCheck(pAd, pAd->MlmeAux.Channel))
			{
				ScanType = SCAN_PASSIVE;
				ScanTimeIn5gChannel = MIN_CHANNEL_TIME;
			}
		}

#ifdef CARRIER_DETECTION_SUPPORT /* Roger sync Carrier */
		/* carrier detection */
		if (pAd->CommonCfg.CarrierDetect.Enable == TRUE)
		{
			ScanType = SCAN_PASSIVE;
			ScanTimeIn5gChannel = MIN_CHANNEL_TIME;
		}
#endif /* CARRIER_DETECTION_SUPPORT */
	}

	/* Global country domain(ch1-11:active scan, ch12-14 passive scan) */
	if (((pAd->MlmeAux.Channel <= 14) &&
		(pAd->MlmeAux.Channel >= 12) &&
		((pAd->CommonCfg.CountryRegion & 0x7f) == REGION_31_BG_BAND)) ||
		(CHAN_PropertyCheck(pAd, pAd->MlmeAux.Channel, CHANNEL_PASSIVE_SCAN) == TRUE))
	{
		ScanType = SCAN_PASSIVE;
	}

	NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);  /* Get an unused nonpaged memory */
	if (NStatus != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s():allocate memory fail\n", __FUNCTION__));
		pAd->StaCfg.bSendingProbe = 0;
		return;
	}

	MgtMacHeaderInit(pAd, &Hdr80211,
					SUBTYPE_PROBE_REQ, 0,
					destMac,
					pAd->CurrentAddress,
					destMac);

	MakeOutgoingFrame(pOutBuffer,               &FrameLen,
					  sizeof(HEADER_802_11),    &Hdr80211,
					  1,                        &SsidIe,
					  1,                        &SsidLen,
					  ssidLen,			        ssidStr,
					  1,                        &SupRateIe,
					  1,                        &pAd->CommonCfg.SupRateLen,
					  pAd->CommonCfg.SupRateLen,  pAd->CommonCfg.SupRate,
					  END_OF_ARGS);

	if (pAd->CommonCfg.ExtRateLen)
	{
		ULONG Tmp;
		MakeOutgoingFrame(pOutBuffer + FrameLen,            &Tmp,
						  1,                                &ExtRateIe,
						  1,                                &pAd->CommonCfg.ExtRateLen,
						  pAd->CommonCfg.ExtRateLen,          pAd->CommonCfg.ExtRate,
						  END_OF_ARGS);
		FrameLen += Tmp;
	}

#ifdef WSC_STA_SUPPORT
	if (pAd->OpMode == OPMODE_STA)
	{
		/* Append WSC information in probe request if WSC state is running */
		if ((pAd->StaCfg.WscControl.WscEnProbeReqIE) &&
			(pAd->StaCfg.WscControl.WscConfMode != WSC_DISABLE))
			/* && (pAd->StaCfg.WscControl.bWscTrigger == TRUE)) */
		{
			UCHAR		*pWscBuf = NULL, WscIeLen = 0;
			ULONG 		WscTmpLen = 0;

			if (os_alloc_mem(pAd, (UCHAR **)&pWscBuf, 512) == NDIS_STATUS_SUCCESS)
			{
				NdisZeroMemory(pWscBuf, 512);


				WscMakeProbeReqIEWithVendorExt(pAd, deviceName, primaryDeviceType,
									vendExt, vendExtLen, pWscBuf, &WscIeLen);
				/* DBGPRINT(RT_DEBUG_ERROR, ("Created WPS IE for Probe Request; Len = %d\n", WscIeLen)); */

				MakeOutgoingFrame(pOutBuffer + FrameLen,              &WscTmpLen,
								WscIeLen,                             pWscBuf,
								END_OF_ARGS);

				FrameLen += WscTmpLen;
				os_free_mem(NULL, pWscBuf);
			}
			else
				DBGPRINT(RT_DEBUG_WARN, ("%s:: WscBuf Allocate failed!\n", __FUNCTION__));
		}
	}
#endif /* WSC_STA_SUPPORT */

	/* DBGPRINT(RT_DEBUG_ERROR, ("Really Sending out Probe Request\n")); */
	MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);

	MlmeFreeMemory(pAd, pOutBuffer);

#if 0 //fix toggle scan
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		pAd->Mlme.SyncMachine.CurrState = SCAN_LISTEN;
#endif

	pAd->StaCfg.bSendingProbe = 0;
}
#endif /* CONFIG_STA_SUPPORT */
#endif /* WIDI_SUPPORT */


#ifdef CONFIG_AP_SUPPORT
#ifdef DOT11_N_SUPPORT
extern int DetectOverlappingPeriodicRound;

VOID Handle_BSS_Width_Trigger_Events(RTMP_ADAPTER *pAd)
{
	ULONG Now32;

#ifdef DOT11N_DRAFT3
	if (pAd->CommonCfg.bBssCoexEnable == FALSE)
		return;
#endif /* DOT11N_DRAFT3 */

	if ((pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_40) &&
		(pAd->CommonCfg.Channel <=14))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Rcv BSS Width Trigger Event: 40Mhz --> 20Mhz \n"));
        NdisGetSystemUpTime(&Now32);
		pAd->CommonCfg.LastRcvBSSWidthTriggerEventsTime = Now32;
		pAd->CommonCfg.bRcvBSSWidthTriggerEvents = TRUE;
		pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 0;
		pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = 0;
        DetectOverlappingPeriodicRound = 31;
	}
}
#endif /* DOT11_N_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

