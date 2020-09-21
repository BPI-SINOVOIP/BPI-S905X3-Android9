/****************************************************************************
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
 ****************************************************************************

    Module Name:
    assoc.c
 
    Abstract:
    Handle association related requests either from WSTA or from local MLME
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP
 */

#include "rt_config.h"

extern UCHAR	CISCO_OUI[];
extern UCHAR	WPA_OUI[];
extern UCHAR	RSN_OUI[];
extern UCHAR	WME_INFO_ELEM[];
extern UCHAR	WME_PARM_ELEM[];
extern UCHAR	RALINK_OUI[];
extern UCHAR BROADCOM_OUI[];
extern UCHAR WPS_OUI[];


static void ap_assoc_info_debugshow(
	IN RTMP_ADAPTER *pAd,
	IN BOOLEAN isReassoc,
	IN MAC_TABLE_ENTRY *pEntry,
	IN IE_LISTS *ie_list)
{
#ifdef DBG
	PUCHAR	sAssoc = isReassoc ? (PUCHAR)"ReASSOC" : (PUCHAR)"ASSOC";
#endif /* DBG */
	struct wifi_dev *wdev;

	wdev = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev;
	DBGPRINT(RT_DEBUG_TRACE, ("%s - \n\tAssign AID=%d to STA %02x:%02x:%02x:%02x:%02x:%02x\n",
		sAssoc, pEntry->Aid, PRINT_MAC(pEntry->Addr)));

#ifdef DOT11_N_SUPPORT
	if (ie_list->ht_cap_len && WMODE_CAP_N(pAd->CommonCfg.PhyMode))
	{
		assoc_ht_info_debugshow(pAd, pEntry, ie_list->ht_cap_len, &ie_list->HTCapability);

		DBGPRINT(RT_DEBUG_TRACE, ("\n%s - Update AP OperaionMode=%d, fAnyStationIsLegacy=%d, fAnyStation20Only=%d, fAnyStationNonGF=%d\n\n",
					sAssoc, 
					pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode, 
					pAd->MacTab.fAnyStationIsLegacy,
					pAd->MacTab.fAnyStation20Only, 
					pAd->MacTab.fAnyStationNonGF));

#ifdef DOT11_VHT_AC
		if ((ie_list->vht_cap_len) &&
			WMODE_CAP_N(pAd->CommonCfg.PhyMode) &&
			(pAd->CommonCfg.Channel > 14))
		{
			assoc_vht_info_debugshow(pAd, pEntry, &ie_list->vht_cap, NULL);
		}
#endif /* DOT11_VHT_AC */


		DBGPRINT(RT_DEBUG_TRACE, ("\tExt Cap Info: \n"));
#ifdef DOT11N_DRAFT3
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tBss2040CoexistMgmt=%d\n",
				pEntry->BSS2040CoexistenceMgmtSupport));
#endif /* DOT11N_DRAFT3 */
#ifdef DOT11_VHT_AC
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tOperatinModeNotification(%d)\n",
				pEntry->ext_cap.operating_mode_notification));
		if (pEntry->ext_cap.operating_mode_notification) {
			DBGPRINT(RT_DEBUG_TRACE, ("\t\t\tChannelWidth(%d), RxNss(%d), RxNssType(%d), ForceOpMode(%d)\n",
					pEntry->operating_mode.ch_width,
					pEntry->operating_mode.rx_nss,
					pEntry->operating_mode.rx_nss_type,
					pEntry->force_op_mode));
		}
#endif /* DOT11_VHT_AC */
	}
	else
#endif /* DOT11_N_SUPPORT */
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s - legacy STA\n", sAssoc));
		DBGPRINT(RT_DEBUG_TRACE, ("\n%s - MODE=%d, MCS=%d\n", sAssoc,
								pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.MCS));
	}

	DBGPRINT(RT_DEBUG_TRACE, ("\tAuthMode=%d, WepStatus=%d, WpaState=%d, GroupKeyWepStatus=%d\n",
		pEntry->AuthMode, pEntry->WepStatus, pEntry->WpaState, wdev->GroupKeyWepStatus));

	DBGPRINT(RT_DEBUG_TRACE, ("\tWMMCap=%d, RalinkAgg=%d, PiggyBack=%d, RDG=%d, TxAMSDU=%d, IdleTimeout=%d\n",
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE),
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE),
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE),
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_RDG_CAPABLE),
		CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_AMSDU_INUSED),
		pEntry->StaIdleTimeout));

}


static USHORT update_associated_mac_entry(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN IE_LISTS *ie_list,
	IN UCHAR MaxSupportedRate)
{
	struct wifi_dev *wdev;
#ifdef TXBF_SUPPORT
	BOOLEAN	 supportsETxBF = FALSE;
#endif // TXBF_SUPPORT //
	STA_TR_ENTRY *tr_entry;

	ASSERT((pEntry->func_tb_idx < pAd->ApCfg.BssidNum));
	wdev = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev;
	tr_entry = &pAd->MacTab.tr_entry[pEntry->wcid];

	/* Update auth, wep, legacy transmit rate setting . */
	pEntry->Sst = SST_ASSOC;
#if 0
#ifdef RELEASE_EXCLUDE
	/* 
		patch for Nintendo DS support rate bug - it only support tx rate 1 and 2
		For this client, AP always transmits low rate(1Mbps) packets.
		20090724: In RT28xx chips, it's done by Hardware autofallback from 11MB -> 5.5MB -> 2MB
	*/
#endif /* RELEASE_EXCLUDE */
	if ((((pEntry->Addr[0]==0x00) && (pEntry->Addr[1]==0x09) && (pEntry->Addr[2]==0xBF)) ||
		 ((pEntry->Addr[0]==0x00) && (pEntry->Addr[1]==0x16) && (pEntry->Addr[2]==0x56)))
			&& MaxSupportedRate == RATE_11)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("==>Assoc-Req from Nintendo DS client.\n"));
		MaxSupportedRate = RATE_1;
	}
#endif

	pEntry->MaxSupportedRate = min(pAd->CommonCfg.MaxTxRate, MaxSupportedRate);
	
	set_entry_phy_cfg(pAd, pEntry);

	pEntry->CapabilityInfo = ie_list->CapabilityInfo;

	if ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) || (pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
	{
		pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
		pEntry->WpaState = AS_INITPSK;
	}
#ifdef DOT1X_SUPPORT
	else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) ||
			(pEntry->AuthMode == Ndis802_11AuthModeWPA2) ||
			(wdev->IEEE8021X == TRUE))
	{
		pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
		pEntry->WpaState = AS_AUTHENTICATION;
	}
#endif /* DOT1X_SUPPORT */
#ifdef WAPI_SUPPORT
	else if ((pEntry->AuthMode == Ndis802_11AuthModeWAICERT) || 
			 (pEntry->AuthMode == Ndis802_11AuthModeWAIPSK))
	{
		pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
		pEntry->WpaState = AS_AUTHENTICATION2;
	}
#endif /* WAPI_SUPPORT */
    

	/* Ralink proprietary Piggyback and Aggregation support for legacy RT61 chip */
	set_sta_ra_cap(pAd, pEntry, ie_list->RalinkIe);

	/* In WPA or 802.1x mode, the port is not secured, otherwise is secued. */
	if ((pEntry->AuthMode >= Ndis802_11AuthModeWPA) 
#ifdef DOT1X_SUPPORT
		|| (wdev->IEEE8021X == TRUE)
#endif /* DOT1X_SUPPORT */
	)
		tr_entry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
	else
		tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;

#ifdef SOFT_ENCRYPT
	/* There are some situation to need to encryption by software 			   	
	   1. The Client support PMF. It shall ony support AES cipher.
	   2. The Client support WAPI.
	   If use RT3883 or later, HW can handle the above.	
	   */
#ifdef WAPI_SUPPORT
	if (!(IS_HW_WAPI_SUPPORT(pAd)) 
		&& (pEntry->WepStatus == Ndis802_11EncryptionSMS4Enabled))
	{
		CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SOFTWARE_ENCRYPT);
	}
#endif /* WAPI_SUPPORT */

#ifdef DOT11W_PMF_SUPPORT
	if ((pAd->chipCap.FlgPMFEncrtptMode == PMF_ENCRYPT_MODE_0)
		&& CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_PMF_CAPABLE))
	{
		CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SOFTWARE_ENCRYPT);
	}
#endif /* DOT11W_PMF_SUPPORT */
#endif /* SOFT_ENCRYPT */

#ifdef DOT11_N_SUPPORT
	/* 	
		WFA recommend to restrict the encryption type in 11n-HT mode.
	 	So, the WEP and TKIP are not allowed in HT rate.
	*/
	if (pAd->CommonCfg.HT_DisallowTKIP &&
		IS_INVALID_HT_SECURITY(pEntry->WepStatus))
	{
		/* Force to None-HT mode due to WiFi 11n policy */
		ie_list->ht_cap_len = 0;
#ifdef DOT11_VHT_AC
		ie_list->vht_cap_len = 0;
#endif /* DOT11_VHT_AC */
		DBGPRINT(RT_DEBUG_TRACE, ("%s : Force the STA as Non-HT mode\n", __FUNCTION__));
	}

	/* If this Entry supports 802.11n, upgrade to HT rate. */
	if ((ie_list->ht_cap_len != 0) && 
		(wdev->DesiredHtPhyInfo.bHtEnable) &&
		WMODE_CAP_N(pAd->CommonCfg.PhyMode))
	{
		ht_mode_adjust(pAd, pEntry, &ie_list->HTCapability, &pAd->CommonCfg.DesiredHtPhy);

#ifdef DOT11N_DRAFT3
		if (ie_list->ExtCapInfo.BssCoexistMgmtSupport)
			pEntry->BSS2040CoexistenceMgmtSupport = 1;
#endif /* DOT11N_DRAFT3 */
#ifdef CONFIG_HOTSPOT_R2
		if (ie_list->ExtCapInfo.qosmap)
			pEntry->QosMapSupport = 1;
#endif		
#ifdef DOT11V_WNM_SUPPORT
		if(IS_BSS_TRANSIT_MANMT_SUPPORT(pAd, pEntry->func_tb_idx))
		{
			if(ie_list->ExtCapInfo.BssTransitionManmt)
				pEntry->BssTransitionManmtSupport = 1;
		}
		if(IS_WNMDMS_SUPPORT(pAd, pEntry->func_tb_idx))
		{
			if(ie_list->ExtCapInfo.DMSSupport)
				pEntry->DMSSupport = 1;
		}
#endif /* DOT11V_WNM_SUPPORT */

		/* 40Mhz BSS Width Trigger events */
		if (ie_list->HTCapability.HtCapInfo.Forty_Mhz_Intolerant)
		{
#ifdef DOT11N_DRAFT3
			pEntry->bForty_Mhz_Intolerant = TRUE;
			pAd->MacTab.fAnyStaFortyIntolerant = TRUE;
			if(((pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_40) && 
				(pAd->CommonCfg.Channel <=14)) &&
			    ((pAd->CommonCfg.bBssCoexEnable == TRUE) &&
				(pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth != 0) && 
				(pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset != 0))
			)
			{
				pAd->CommonCfg.LastBSSCoexist2040.field.BSS20WidthReq = 1;
				pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 0;
				pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = 0;
				pAd->CommonCfg.Bss2040CoexistFlag |= BSS_2040_COEXIST_INFO_SYNC;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("pEntry set 40MHz Intolerant as 1\n"));
#endif /* DOT11N_DRAFT3 */
			Handle_BSS_Width_Trigger_Events(pAd);
		}
		
#ifdef TXBF_SUPPORT
#ifdef DOT11_VHT_AC
		if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
				(pAd->CommonCfg.Channel > 14) &&
				(ie_list->vht_cap_len))
			supportsETxBF = clientSupportsVHTETxBF(pAd, &ie_list->vht_cap.vht_cap);
		else		
#endif /* DOT11_VHT_AC */
		supportsETxBF = clientSupportsETxBF(pAd, &ie_list->HTCapability.TxBFCap);
#endif /* TXBF_SUPPORT */

		/* find max fixed rate */
		pEntry->MaxHTPhyMode.field.MCS = (USHORT)get_ht_max_mcs(pAd
			, &wdev->DesiredHtPhyInfo.MCSSet[0]
			, &ie_list->HTCapability.MCSSet[0]);
		 
		if (wdev->DesiredTransmitSetting.field.MCS != MCS_AUTO)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("@@@ IF-ra%d DesiredTransmitSetting.field.MCS = %d\n",
								pEntry->func_tb_idx,
								wdev->DesiredTransmitSetting.field.MCS));

			set_ht_fixed_mcs(pAd, pEntry, (UCHAR)wdev->DesiredTransmitSetting.field.MCS
							, (UCHAR)wdev->HTPhyMode.field.MCS);
		}

		pEntry->MaxHTPhyMode.field.STBC = (ie_list->HTCapability.HtCapInfo.RxSTBC & (pAd->CommonCfg.DesiredHtPhy.TxSTBC));
		// TODO: shiang-6590, check if this is necessary here, perforce didn't have this
		if (ie_list->HTCapability.HtCapParm.MpduDensity < 5)
			ie_list->HTCapability.HtCapParm.MpduDensity = 5;

		pEntry->MpduDensity = ie_list->HTCapability.HtCapParm.MpduDensity;
		pEntry->MaxRAmpduFactor = ie_list->HTCapability.HtCapParm.MaxRAmpduFactor;
		pEntry->MmpsMode = (UCHAR)ie_list->HTCapability.HtCapInfo.MimoPs;
		pEntry->AMsduSize = (UCHAR)ie_list->HTCapability.HtCapInfo.AMsduSize;

		if (pAd->CommonCfg.DesiredHtPhy.AmsduEnable && (pAd->CommonCfg.REGBACapability.field.AutoBA == FALSE))
			CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AMSDU_INUSED);
		
		if (pAd->CommonCfg.ht_ldpc && (pAd->chipCap.phy_caps & fPHY_CAP_LDPC)) {
			if (ie_list->HTCapability.HtCapInfo.ht_rx_ldpc)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_HT_RX_LDPC_CAPABLE);
		}

		set_sta_ht_cap(pAd, pEntry, &ie_list->HTCapability);

		/* Record the received capability from association request */
		NdisMoveMemory(&pEntry->HTCapability, &ie_list->HTCapability, sizeof(HT_CAPABILITY_IE));

#ifdef DOT11_VHT_AC
		if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
			(pAd->CommonCfg.Channel > 14) &&
			ie_list->vht_cap_len)
		{
			VHT_CAP_INFO *vht_cap_info = (VHT_CAP_INFO *)&ie_list->vht_cap;

			pEntry->MaxHTPhyMode.field.MODE = MODE_VHT;
			if ((pEntry->MaxHTPhyMode.field.BW== BW_40) && (wdev->DesiredHtPhyInfo.vht_bw == VHT_BW_80))
				pEntry->MaxHTPhyMode.field.BW = BW_80;

			/* TODO: implement get_vht_max_mcs to get peer max MCS */
			if (ie_list->vht_cap.mcs_set.rx_mcs_map.mcs_ss1 == VHT_MCS_CAP_9) {
				if ((pEntry->MaxHTPhyMode.field.BW == BW_20))
					pEntry->MaxHTPhyMode.field.MCS = 8;
				else
				pEntry->MaxHTPhyMode.field.MCS = 9;
			} else if (ie_list->vht_cap.mcs_set.rx_mcs_map.mcs_ss1 == VHT_MCS_CAP_8) {
				pEntry->MaxHTPhyMode.field.MCS = 8;
			} else if (ie_list->vht_cap.mcs_set.rx_mcs_map.mcs_ss1 == VHT_MCS_CAP_8) {
				pEntry->MaxHTPhyMode.field.MCS = 7;
			}
			
			if (ie_list->vht_cap.mcs_set.rx_mcs_map.mcs_ss2 == VHT_MCS_CAP_9) {
				if ((pEntry->MaxHTPhyMode.field.BW == BW_20))
					pEntry->MaxHTPhyMode.field.MCS = ((1 << 4) | 8);
				else
					pEntry->MaxHTPhyMode.field.MCS = ((1 << 4) | 9);
			} else if (ie_list->vht_cap.mcs_set.rx_mcs_map.mcs_ss2 == VHT_MCS_CAP_8) {
				pEntry->MaxHTPhyMode.field.MCS = ((1 << 4) | 8);
			} else if (ie_list->vht_cap.mcs_set.rx_mcs_map.mcs_ss2 == VHT_MCS_CAP_7) {
				pEntry->MaxHTPhyMode.field.MCS = ((1 << 4) | 7);
			}


DBGPRINT(RT_DEBUG_OFF, ("%s(): Peer's PhyCap=>Mode:%s, BW:%s\n", 
				__FUNCTION__,
				get_phymode_str(pEntry->MaxHTPhyMode.field.MODE),
				get_bw_str(pEntry->MaxHTPhyMode.field.BW)));

			if (pAd->CommonCfg.vht_ldpc && (pAd->chipCap.phy_caps & fPHY_CAP_LDPC)) {
			if (vht_cap_info->rx_ldpc)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_VHT_RX_LDPC_CAPABLE);
			}

			if (vht_cap_info->sgi_80M)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SGI80_CAPABLE);

			if (vht_cap_info->sgi_160M)
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_SGI160_CAPABLE);

			if (pAd->CommonCfg.vht_stbc)
			{
				if (vht_cap_info->tx_stbc)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_VHT_TXSTBC_CAPABLE);
				if (vht_cap_info->rx_stbc)
					CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_VHT_RXSTBC_CAPABLE);
			}
			NdisMoveMemory(&pEntry->vht_cap_ie, &ie_list->vht_cap, sizeof(VHT_CAP_IE));
		}

		if (ie_list->operating_mode_len == sizeof(OPERATING_MODE) &&
			ie_list->operating_mode.rx_nss_type == 0)
		{
			pEntry->operating_mode = ie_list->operating_mode;
			pEntry->force_op_mode = TRUE;
			DBGPRINT(RT_DEBUG_OFF, ("%s(): Peer's OperatingMode=>RxNssType: %d, RxNss: %d, ChBW: %d\n",
				__FUNCTION__, pEntry->operating_mode.rx_nss_type,
				pEntry->operating_mode.rx_nss,
				pEntry->operating_mode.ch_width));		
		}
		else
			pEntry->force_op_mode = FALSE;
#endif /* DOT11_VHT_AC */
	}
	else
	{
#ifdef CONFIG_HOTSPOT_R2
		if (ie_list->ExtCapInfo.qosmap)
			pEntry->QosMapSupport = 1;
#endif
		pAd->MacTab.fAnyStationIsLegacy = TRUE;
		NdisZeroMemory(&pEntry->HTCapability, sizeof(HT_CAPABILITY_IE));
#ifdef DOT11_VHT_AC
		// TODO: shiang-usw, it's ugly and need to revise it
		NdisZeroMemory(&pEntry->vht_cap_ie, sizeof(VHT_CAP_IE));
		NdisZeroMemory(&pEntry->SupportVHTMCS, sizeof(pEntry->SupportVHTMCS));
		pEntry->SupportRateMode &= (~SUPPORT_VHT_MODE);
#endif /* DOT11_VHT_AC */
	}
#endif /* DOT11_N_SUPPORT */

	pEntry->HTPhyMode.word = pEntry->MaxHTPhyMode.word;

#if 0
	/*
		If dynamic rate switching is enabled, starts from a more reliable rate to
		increase STA's DHCP succeed rate
	*/
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
		pEntry->CurrTxRate = min(pEntry->MaxSupportedRate, RATE_11);
	else
#endif
	pEntry->CurrTxRate = pEntry->MaxSupportedRate;

#ifdef MFB_SUPPORT
	pEntry->lastLegalMfb = 0;
	pEntry->isMfbChanged = FALSE;
	pEntry->fLastChangeAccordingMfb = FALSE;

	pEntry->toTxMrq = TRUE;
	pEntry->msiToTx = 0;/*has to increment whenever a mrq is sent */
	pEntry->mrqCnt = 0;

	pEntry->pendingMfsi = 0;

	pEntry->toTxMfb = FALSE;
	pEntry->mfbToTx = 0;
	pEntry->mfb0 = 0;
	pEntry->mfb1 = 0;
#endif	/* MFB_SUPPORT */

	pEntry->freqOffsetValid = FALSE;

#ifdef TXBF_SUPPORT
	if (pAd->chipCap.FlgHwTxBfCap) {
		TxBFInit(pAd, pEntry, supportsETxBF);
		if (supportsETxBF) {
			HT_BF_CAP *pTxBFCap = &ie_list->HTCapability.TxBFCap;
			UCHAR ndpSndgStreams = pAd->Antenna.field.TxPath;

			if ((pTxBFCap->ExpComBF > 0) && (!pAd->CommonCfg.ETxBfNoncompress))
			{
				if ((pTxBFCap->ComSteerBFAntSup + 1) < pAd->Antenna.field.TxPath)
					ndpSndgStreams = pTxBFCap->ComSteerBFAntSup + 1;
			}
			
			if ((pTxBFCap->ExpNoComBF > 0) && (pAd->CommonCfg.ETxBfNoncompress))
			{
				if ((pTxBFCap->NoComSteerBFAntSup + 1) < pAd->Antenna.field.TxPath)
					ndpSndgStreams = pTxBFCap->NoComSteerBFAntSup + 1;
			}
			pEntry->ndpSndgStreams = ndpSndgStreams;
		}
	}
#endif // TXBF_SUPPORT //

	// Initialize Rate Adaptation
	MlmeRAInit(pAd, pEntry);

	/* Set asic auto fall back */
	if (wdev->bAutoTxRateSwitch == TRUE)
	{
		UCHAR TableSize = 0;
		
		pEntry->bAutoTxRateSwitch = TRUE;

		MlmeSelectTxRateTable(pAd, pEntry, &pEntry->pTable, &TableSize, &pEntry->CurrTxRateIndex);

		MlmeNewTxRate(pAd, pEntry);

#ifdef NEW_RATE_ADAPT_SUPPORT
		if (! ADAPT_RATE_TABLE(pEntry->pTable))
#endif /* NEW_RATE_ADAPT_SUPPORT */
			pEntry->HTPhyMode.field.ShortGI = GI_800;
	}
	else
	{
		pEntry->HTPhyMode.field.MCS = wdev->HTPhyMode.field.MCS;
		pEntry->bAutoTxRateSwitch = FALSE;
#ifdef WFA_VHT_PF
		if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode)) {
			pEntry->HTPhyMode.field.MCS = wdev->DesiredTransmitSetting.field.MCS + 
										((pAd->CommonCfg.TxStream - 1) << 4);
		}
#endif /* WFA_VHT_PF */

		/* If the legacy mode is set, overwrite the transmit setting of this entry. */
		RTMPUpdateLegacyTxSetting((UCHAR)wdev->DesiredTransmitSetting.field.FixedTxMode, pEntry);

#ifdef MCS_LUT_SUPPORT
		if ( pAd->chipCap.hif_type == HIF_MT) 
		{
			asic_mcs_lut_update(pAd, pEntry);
			pEntry->LastTxRate = (USHORT) (pEntry->HTPhyMode.word);
		}
#endif /* MCS_LUT_SUPPORT */

		DBGPRINT(RT_DEBUG_TRACE, ("%s(SS=%d): pMbss(FixedTxMode=%d, MCS=%d), pEntry(Mode=%d, MCS=%d)\n",
					__FUNCTION__, pAd->CommonCfg.TxStream,
					wdev->DesiredTransmitSetting.field.FixedTxMode,
					wdev->DesiredTransmitSetting.field.MCS,
					pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.MCS));
	}

#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT)
	{
        AsicUpdateRxWCIDTable(pAd, pEntry->wcid, pEntry->Addr);
		AsicRcpiReset(pAd, pEntry->wcid);
	}
#endif

	if (pEntry->AuthMode < Ndis802_11AuthModeWPA)
		ApLogEvent(pAd, pEntry->Addr, EVENT_ASSOCIATED);

	APUpdateCapabilityAndErpIe(pAd);
#ifdef DOT11_N_SUPPORT
	APUpdateOperationMode(pAd);
#endif /* DOT11_N_SUPPORT */

	pEntry->ReTryCounter = PEER_MSG1_RETRY_TIMER_CTR;

#ifdef HOSTAPD_SUPPORT
	if((wdev->Hostapd == Hostapd_EXT) &&
	    ((wdev->AuthMode >= Ndis802_11AuthModeWPA) || wdev->IEEE8021X))
	{
#if 0 /* os abl move */
		/* 
			notify hostapd that new association has been built.

			Beacuse hostapd doesn't need to know sa_family now,
			use sa_family to inform hostapd this station is Privacy enabled or disabled.
			0xFFFD: Privacy is Disabled, 0xFFFC: Privacy is Enabled.
		*/
		union iwreq_data wrqu;

		NdisZeroMemory(&wrqu, sizeof(wrqu));
		wrqu.addr.sa_family = pEntry->func_tb_idx;/*(pEntry->CapabilityInfo & 0x0010) == 0 ? 0xFFFD : 0xFFFC; */
		NdisMoveMemory(wrqu.addr.sa_data, Addr, ETH_ALEN);
		DBGPRINT(RT_DEBUG_TRACE, ("adding %d bytes\n", wrqu.data.length));
	    	wireless_send_event(pAd->net_dev, IWEVREGISTERED, &wrqu, NULL);
#endif /* 0 */
		RtmpOSWrielessEventSendExt(pAd->net_dev, RT_WLAN_EVENT_EXPIRED,
					-1, pEntry->Addr, NULL, 0,
					((pEntry->CapabilityInfo & 0x0010) == 0 ? 0xFFFD : 0xFFFC));
	}
#endif /*HOSTAPD_SUPPORT*/

	return MLME_SUCCESS;
}


/*
    ==========================================================================
    Description:
       assign a new AID to the newly associated/re-associated STA and
       decide its MaxSupportedRate and CurrTxRate. Both rates should not
       exceed AP's capapbility
    Return:
       MLME_SUCCESS - association successfully built
       others - association failed due to resource issue
    ==========================================================================
 */
static USHORT APBuildAssociation(
    IN RTMP_ADAPTER *pAd,
    IN MAC_TABLE_ENTRY *pEntry,
    IN IE_LISTS *ie_list,
    IN UCHAR MaxSupportedRateIn500Kbps,
    OUT USHORT *pAid)
{
	USHORT StatusCode = MLME_SUCCESS;
	UCHAR MaxSupportedRate = RATE_11;
	struct wifi_dev *wdev;
#ifdef WSC_AP_SUPPORT
	WSC_CTRL *wsc_ctrl;
#endif /* WSC_AP_SUPPORT */
	STA_TR_ENTRY *tr_entry;

	MaxSupportedRate = dot11_2_ra_rate(MaxSupportedRateIn500Kbps);

	if ((WMODE_EQUAL(pAd->CommonCfg.PhyMode, WMODE_G)
#ifdef DOT11_N_SUPPORT
		|| WMODE_EQUAL(pAd->CommonCfg.PhyMode, (WMODE_G | WMODE_GN))
#endif /* DOT11_N_SUPPORT */
		) 
		&& (MaxSupportedRate < RATE_FIRST_OFDM_RATE)
	)
		return MLME_ASSOC_REJ_DATA_RATE;

#ifdef DOT11_N_SUPPORT
	/* 11n only */
	if (WMODE_HT_ONLY(pAd->CommonCfg.PhyMode)&& (ie_list->ht_cap_len == 0))
		return MLME_ASSOC_REJ_DATA_RATE;
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11_VHT_AC
	if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
		(pAd->CommonCfg.Channel > 14) &&
		(ie_list->vht_cap_len == 0) &&
		(pAd->CommonCfg.bNonVhtDisallow))
		return MLME_ASSOC_REJ_DATA_RATE;
#endif /* DOT11_VHT_AC */

	if (!pEntry)
		return MLME_UNSPECIFY_FAIL;

	if (pEntry && ((pEntry->Sst == SST_AUTH) || (pEntry->Sst == SST_ASSOC)))
	{
		/* TODO:
			should qualify other parameters, for example - 
			capablity, supported rates, listen interval, etc., to 
			decide the Status Code
		*/
		tr_entry = &pAd->MacTab.tr_entry[pEntry->wcid];
		if (tr_entry == NULL)
			DBGPRINT(RT_DEBUG_TRACE, ("tr_entry is NULL.\n"));
		
		*pAid = pEntry->Aid;
		pEntry->NoDataIdleCount = 0;
		// TODO: shiang-usw,  remove upper setting becasue we need to migrate to tr_entry!
		pAd->MacTab.tr_entry[pEntry->wcid].NoDataIdleCount = 0;

		pEntry->StaConnectTime = 0;
#ifdef CONFIG_HOTSPOT_R2
        if (!CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_OSEN_CAPABLE))
#endif       
        { 
#ifdef WSC_AP_SUPPORT
			if (pEntry->bWscCapable == FALSE)
#endif /* WSC_AP_SUPPORT */
			{
				/* check the validity of the received RSNIE */
				StatusCode = (USHORT)APValidateRSNIE(pAd
					, pEntry, &ie_list->RSN_IE[0]
					, ie_list->RSNIE_Len);
				if (StatusCode != MLME_SUCCESS)
					return StatusCode;
			}
        }
		NdisMoveMemory(pEntry->RSN_IE, &ie_list->RSN_IE[0], ie_list->RSNIE_Len);
		pEntry->RSNIE_Len = ie_list->RSNIE_Len;

#ifdef RELEASE_EXCLUDE
		DBGPRINT(RT_DEBUG_INFO, ("ASSOC - MaxSupportedRate = %x, pAd->CommonCfg.MaxTxRate = %x\n",
			 MaxSupportedRate, pAd->CommonCfg.MaxTxRate));
#endif /* RELEASE_EXCLUDE */

		wdev = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev;
		if (*pAid == 0)
			StatusCode = MLME_ASSOC_REJ_UNABLE_HANDLE_STA;
		else if ((pEntry->RSNIE_Len == 0) &&
				(wdev->AuthMode >= Ndis802_11AuthModeWPA) 
#ifdef HOSTAPD_SUPPORT
				&& (wdev->Hostapd == Hostapd_EXT)
#endif /* HOSTAPD_SUPPORT */
#ifdef RT_CFG80211_SUPPORT
				&& (wdev->Hostapd == Hostapd_CFG)
#endif /*RT_CFG80211_SUPPORT*/
		)
		{
#ifdef WSC_AP_SUPPORT
			wsc_ctrl = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl;
			if (((wsc_ctrl->WscConfMode != WSC_DISABLE) &&
				pEntry->bWscCapable
#ifdef WSC_V2_SUPPORT
				&& (wsc_ctrl->WscV2Info.bWpsEnable || 
				(wsc_ctrl->WscV2Info.bEnableWpsV2 == FALSE))
#endif /* WSC_V2_SUPPORT */
				)
#ifdef HOSTAPD_SUPPORT
				|| wdev->Hostapd == Hostapd_EXT
#endif /*HOSTAPD_SUPPORT*/
#ifdef RT_CFG80211_SUPPORT
				|| wdev->Hostapd == Hostapd_CFG
#endif /*RT_CFG80211_SUPPORT*/
			)
			{
				pEntry->Sst = SST_ASSOC;
				StatusCode = MLME_SUCCESS;
				/* In WPA or 802.1x mode, the port is not secured. */
				if ((pEntry->AuthMode >= Ndis802_11AuthModeWPA) 
#ifdef DOT1X_SUPPORT
				|| (wdev->IEEE8021X == TRUE)
#endif /* DOT1X_SUPPORT */					
				)
				{
					if(tr_entry)
						tr_entry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
				}
				else
				{
					if(tr_entry)
						tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;
				}
				if ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) ||
				(pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
				{
					pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
					pEntry->WpaState = AS_INITPSK;
				}
#ifdef DOT1X_SUPPORT				
				else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) ||
					(pEntry->AuthMode == Ndis802_11AuthModeWPA2) ||
					(wdev->IEEE8021X == TRUE))
				{
					pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
					pEntry->WpaState = AS_AUTHENTICATION;
				}
#endif /* DOT1X_SUPPORT */					
			}
			else
			{
				DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - WSC_STATE_MACHINE is OFF.<WscConfMode = %d, apidx =%d>\n",
				                     wsc_ctrl->WscConfMode, pEntry->func_tb_idx));
				StatusCode = MLME_ASSOC_DENY_OUT_SCOPE;
			}
#else  /* WSC_AP_SUPPORT */
#ifdef RT_CFG80211_P2P_SUPPORT
			//CFG_TODO: due to WPS_AP flag
			pEntry->Sst = SST_ASSOC;
			StatusCode = MLME_SUCCESS;
#else			
			StatusCode = MLME_ASSOC_DENY_OUT_SCOPE;
#endif /* RT_CFG80211_P2P_SUPPORT */			
#endif /* WSC_AP_SUPPORT */

#ifdef HOSTAPD_SUPPORT
			if(wdev->Hostapd == Hostapd_EXT
				&& (wdev->AuthMode >= Ndis802_11AuthModeWPA 
				|| wdev->IEEE8021X))
			{
#if 0 /* os abl move */
				union iwreq_data wrqu;
			
				/* notify hostapd that new association has been built. */
				NdisZeroMemory(&wrqu, sizeof(wrqu));

				/*
					Beacuse hostapd doesn't need to know sa_family now,
					use sa_family to inform hostapd this station is Privacy enabled or disabled.
					0xFFFD: Privacy is Disabled, 0xFFFC: Privacy is Enabled.
				*/
				/*wrqu.addr.sa_family = pEntry->func_tb_idx; */
				wrqu.addr.sa_family = (pEntry->CapabilityInfo & 0x0010) == 0 ? 0xFFFD : 0xFFFC;
				NdisMoveMemory(wrqu.addr.sa_data, Addr, ETH_ALEN);
				DBGPRINT(RT_DEBUG_TRACE, ("adding %d bytes\n", wrqu.data.length));
			    	wireless_send_event(pAd->net_dev, IWEVREGISTERED, &wrqu, NULL);
#endif /* 0 */

				RtmpOSWrielessEventSendExt(pAd->net_dev, RT_WLAN_EVENT_EXPIRED,
						-1, pEntry->Addr, NULL, 0,
						((pEntry->CapabilityInfo & 0x0010) == 0 ? 0xFFFD : 0xFFFC));
			}
#endif /*HOSTAPD_SUPPORT*/
		}
		else
		{
			StatusCode = update_associated_mac_entry(pAd, pEntry, ie_list, MaxSupportedRate);
		}
	}
	else /* CLASS 3 error should have been handled beforehand; here should be MAC table full */
		StatusCode = MLME_ASSOC_REJ_UNABLE_HANDLE_STA;

	if (StatusCode == MLME_SUCCESS)
	{
#ifdef DPA_S
#ifdef P2P_SUPPORT
		UCHAR p2pIndex = P2P_NOT_FOUND;
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
		if (ie_list->bWmmCapable)
		{
			CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
		}
		else
		{
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
		}
#ifdef DPA_S
#ifdef P2P_SUPPORT
		p2pIndex = P2pGroupTabSearch(pAd, pEntry->Addr);
		if (p2pIndex != P2P_NOT_FOUND)
		{
			PRT_P2P_CLIENT_ENTRY pP2pEntry = &pAd->P2pTable.Client[p2pIndex];
			P2pSendSamsungEvent(pAd, P2P_NOTIF_SOFTAP_STA_ASSOC, pEntry->Addr, pP2pEntry->DeviceName, pP2pEntry->DeviceNameLen);
		}
		else
			P2pSendSamsungEvent(pAd, P2P_NOTIF_SOFTAP_STA_ASSOC, pEntry->Addr, NULL, 0);
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
	}

    return StatusCode;
}


#ifdef IAPP_SUPPORT
/*
 ========================================================================
 Routine Description:
    Send Leyer 2 Update Frame to update forwarding table in Layer 2 devices.

 Arguments:
    *mac_p - the STATION MAC address pointer

 Return Value:
    TRUE - send successfully
    FAIL - send fail

 Note:
 ========================================================================
*/
static BOOLEAN IAPP_L2_Update_Frame_Send(RTMP_ADAPTER *pAd, UINT8 *mac, INT wdev_idx)
{
#if 0 /* os abl move */
/* Layer 2 Update frame to switch/bridge */
/* For any Layer2 devices, e.g., bridges, switches and other APs, the frame
   can update their forwarding tables with the correct port to reach the new
   location of the STA */
typedef struct GNU_PACKED _RT_IAPP_L2_UPDATE_FRAME {

    UCHAR   DA[ETH_ALEN]; /* broadcast MAC address */
    UCHAR   SA[ETH_ALEN]; /* the MAC address of the STA that has just associated
                             or reassociated */
    USHORT  Len;          /* 8 octets */
    UCHAR   DSAP;         /* null */
    UCHAR   SSAP;         /* null */
    UCHAR   Control;      /* reference to IEEE Std 802.2 */
    UCHAR   XIDInfo[3];   /* reference to IEEE Std 802.2 */
} RT_IAPP_L2_UPDATE_FRAME, *PRT_IAPP_L2_UPDATE_FRAME;

	RT_IAPP_L2_UPDATE_FRAME  frame_body;
	INT size = sizeof(RT_IAPP_L2_UPDATE_FRAME);
	PNDIS_PACKET	pNetBuf;

	
	pNetBuf = RtmpOSNetPktAlloc(pAd, size);
	if (!pNetBuf)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Error! Can't allocate a skb.\n"));
		return FALSE;
	}
	
	/* init the update frame body */
	NdisZeroMemory(&frame_body, size);

	memset(frame_body.DA, 0xFF, ETH_ALEN);
	memcpy(frame_body.SA, mac_p, ETH_ALEN);

	frame_body.Len      = OS_HTONS(ETH_ALEN);
	frame_body.DSAP     = 0;
	frame_body.SSAP     = 0x01;
	frame_body.Control  = 0xAF;

	frame_body.XIDInfo[0] = 0x81;
	frame_body.XIDInfo[1] = 1;
	frame_body.XIDInfo[2] = 1 << 1;

	SET_OS_PKT_NETDEV(pNetBuf, pAd->wdev_list[wdev_idx]->if_dev);
#ifdef UCOS
	net_pkt_put(pNetBuf, size);
	memcpy(pNetBuf->data, &frame_body, size);
#else
    skb_reserve(pNetBuf, 2);
    memcpy(skb_put(pNetBuf, size), &frame_body, size);
#endif
#endif /* 0 */

	NDIS_PACKET	*pNetBuf;

	pNetBuf = RtmpOsPktIappMakeUp(get_netdev_from_bssid(pAd, wdev_idx), mac);
	if (pNetBuf == NULL)
		return FALSE;

	/* UCOS: update the built-in bridge, too (don't use gmac.xmit()) */
	announce_802_3_packet(pAd, pNetBuf, OPMODE_AP);

	IAPP_L2_UpdatePostCtrl(pAd, mac, wdev_idx);

	return TRUE;
} /* End of IAPP_L2_Update_Frame_Send */
#endif /* IAPP_SUPPORT */


/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
#if 0
BOOLEAN PeerAssocReqCmmSanity(
    IN PRTMP_ADAPTER pAd, 
	IN BOOLEAN isReassoc,
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2,
    OUT USHORT *pCapabilityInfo, 
    OUT USHORT *pListenInterval, 
    OUT PUCHAR pApAddr,
    OUT UCHAR *pSsidLen,
    OUT char *Ssid,
    OUT UCHAR *pRatesLen,
    OUT UCHAR Rates[],
    OUT UCHAR *RSN,
    OUT UCHAR *pRSNLen,
    OUT BOOLEAN *pbWmmCapable,
#ifdef WSC_AP_SUPPORT
	OUT BOOLEAN *pWscCapable,
#endif /* WSC_AP_SUPPORT */
	OUT ULONG  *pRalinkIe,
	OUT EXT_CAP_INFO_ELEMENT *pExtCapInfo,
#ifdef DOT11R_FT_SUPPORT
	OUT PFT_INFO pFtInfo,
#endif /* DOT11R_FT_SUPPORT */
#ifdef DOT11K_RRM_SUPPORT
	OUT PRRM_EN_CAP_IE pRrmEnCap,
#endif /* DOT11K_RRM_SUPPORT */
#ifdef P2P_SUPPORT
	OUT ULONG *P2PSubelementLen, 
	OUT PUCHAR pP2pSubelement, 
#endif /* P2P_SUPPORT */
    OUT UCHAR		 *pHtCapabilityLen,
    OUT HT_CAPABILITY_IE *pHtCapability)
{
    CHAR			*Ptr;
    PFRAME_802_11	Fr = (PFRAME_802_11)Msg;
    PEID_STRUCT		eid_ptr;
    UCHAR			Sanity = 0;
    UCHAR			WPA1_OUI[4] = { 0x00, 0x50, 0xF2, 0x01 };
    UCHAR			WPA2_OUI[3] = { 0x00, 0x0F, 0xAC };
    MAC_TABLE_ENTRY *pEntry = (MAC_TABLE_ENTRY *)NULL;
#ifdef P2P_SUPPORT
	PRT_P2P_CONFIG	pP2PCtrl = &pAd->P2pCfg;
	UCHAR	P2POUIBYTE[4] = {0x50, 0x6f, 0x9a, 0x9};
#endif /* P2P_SUPPORT */

    /* to prevent caller from using garbage output value */
	*pSsidLen     = 0;
    *pRatesLen    = 0;
    *pRSNLen      = 0;
    *pbWmmCapable = FALSE;
    *pRalinkIe    = 0;
    *pHtCapabilityLen= 0;	

    COPY_MAC_ADDR(pAddr2, &Fr->Hdr.Addr2);

	pEntry = MacTableLookup(pAd, pAddr2);
	if (pEntry == NULL)
		return FALSE;


#ifdef DOT11R_FT_SUPPORT
    NdisZeroMemory(pFtInfo, sizeof(FT_INFO));
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
	NdisZeroMemory(pRrmEnCap, sizeof(RRM_EN_CAP_IE));
#endif /* DOT11K_RRM_SUPPORT */

    Ptr = (PCHAR)Fr->Octet;

    NdisMoveMemory(pCapabilityInfo, &Fr->Octet[0], 2);
    NdisMoveMemory(pListenInterval, &Fr->Octet[2], 2);

    if (isReassoc) 
	{
		NdisMoveMemory(pApAddr, &Fr->Octet[4], 6);
		eid_ptr = (PEID_STRUCT) &Fr->Octet[10];
	}
	else
	{
		eid_ptr = (PEID_STRUCT) &Fr->Octet[4];
	}


    /* get variable fields from payload and advance the pointer */
    while (((UCHAR *)eid_ptr + eid_ptr->Len + 1) < ((UCHAR *)Fr + MsgLen))
    {
        switch(eid_ptr->Eid)
        {
            case IE_SSID:
				if (((Sanity&0x1) == 1))
					break;

                if ((eid_ptr->Len <= MAX_LEN_OF_SSID))
                {
                    Sanity |= 0x01;
                    NdisMoveMemory(Ssid, eid_ptr->Octet, eid_ptr->Len);
                    *pSsidLen = eid_ptr->Len;
                    DBGPRINT(RT_DEBUG_TRACE,
						("PeerAssocReqSanity - SsidLen = %d  \n", *pSsidLen));
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE,
						("PeerAssocReqSanity - wrong IE_SSID\n"));
                    return FALSE;
                }
                break;

            case IE_SUPP_RATES:
                if ((eid_ptr->Len <= MAX_LEN_OF_SUPPORTED_RATES) &&
					(eid_ptr->Len > 0))
                {
                    Sanity |= 0x02;
                    NdisMoveMemory(Rates, eid_ptr->Octet, eid_ptr->Len);

                    DBGPRINT(RT_DEBUG_TRACE,
						("PeerAssocReqSanity - IE_SUPP_RATES., Len=%d. "
						"Rates[0]=%x\n", eid_ptr->Len, Rates[0]));
                    DBGPRINT(RT_DEBUG_TRACE,
						("Rates[1]=%x %x %x %x %x %x %x\n",
						Rates[1], Rates[2], Rates[3],
						Rates[4], Rates[5], Rates[6],
						Rates[7]));

                    *pRatesLen = eid_ptr->Len;
                }
                else
                {
					UCHAR RateDefault[8] = \
							{ 0x82, 0x84, 0x8b, 0x96, 0x12, 0x24, 0x48, 0x6c };

                	/* HT rate not ready yet. return true temporarily. rt2860c */
                    /*DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - wrong IE_SUPP_RATES\n")); */
                    Sanity |= 0x02;
                    *pRatesLen = 8;
					NdisMoveMemory(Rates, RateDefault, 8);

                    DBGPRINT(RT_DEBUG_TRACE,
						("PeerAssocReqSanity - wrong IE_SUPP_RATES., Len=%d\n",
						eid_ptr->Len));
                }
                break;

            case IE_EXT_SUPP_RATES:
                if (eid_ptr->Len + *pRatesLen <= MAX_LEN_OF_SUPPORTED_RATES)
                {
                    NdisMoveMemory(&Rates[*pRatesLen], eid_ptr->Octet,
									eid_ptr->Len);
                    *pRatesLen = (*pRatesLen) + eid_ptr->Len;
                }
                else
                {
                    NdisMoveMemory(&Rates[*pRatesLen], eid_ptr->Octet,
									MAX_LEN_OF_SUPPORTED_RATES - (*pRatesLen));
                    *pRatesLen = MAX_LEN_OF_SUPPORTED_RATES;
                }
                break;
                
            case IE_HT_CAP:
			if (eid_ptr->Len >= sizeof(HT_CAPABILITY_IE))
			{
				NdisMoveMemory(pHtCapability, eid_ptr->Octet, SIZE_HT_CAP_IE);

				*(USHORT *)(&pHtCapability->HtCapInfo) = \
							cpu2le16(*(USHORT *)(&pHtCapability->HtCapInfo));

#ifdef UNALIGNMENT_SUPPORT
				{
					EXT_HT_CAP_INFO extHtCapInfo;

					NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&pHtCapability->ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
					*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
					NdisMoveMemory((PUCHAR)(&pHtCapability->ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
				}
#else				
				*(USHORT *)(&pHtCapability->ExtHtCapInfo) = \
							cpu2le16(*(USHORT *)(&pHtCapability->ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

				*pHtCapabilityLen = SIZE_HT_CAP_IE;
				Sanity |= 0x10;
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - IE_HT_CAP\n"));
			}
			else
			{
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - wrong IE_HT_CAP.eid_ptr->Len = %d\n", eid_ptr->Len));
			}
				
		break;
		case IE_EXT_CAPABILITY:
			if (eid_ptr->Len >= sizeof(EXT_CAP_INFO_ELEMENT))
			{
				NdisMoveMemory(pExtCapInfo, eid_ptr->Octet, sizeof(EXT_CAP_INFO_ELEMENT));
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - IE_EXT_CAPABILITY!\n"));
			}

			break;

            case IE_WPA:    /* same as IE_VENDOR_SPECIFIC */
            case IE_WPA2:
#ifdef DOT11R_FT_SUPPORT
#ifdef WMM_ACM_SUPPORT
				if (ACM_WME_ELM_Check((UCHAR *)eid_ptr,
					ACM_WME_OUI_SUBTYPE_TSPEC) == ACM_RTN_OK)
				{
					if ((pFtInfo->RicInfo.RicIEsLen + eid_ptr->Len + 2) < MAX_RICIES_LEN)
					{
						NdisMoveMemory(&pFtInfo->RicInfo.RicIEs[pFtInfo->RicInfo.RicIEsLen],
										&eid_ptr->Eid, eid_ptr->Len + 2);
						pFtInfo->RicInfo.RicIEsLen += eid_ptr->Len + 2;
					}
				}
#endif /* WMM_ACM_SUPPORT */
#endif /* DOT11R_FT_SUPPORT */

#ifdef P2P_SUPPORT
				if (NdisEqualMemory(eid_ptr->Octet, P2POUIBYTE, sizeof(P2POUIBYTE)) && (eid_ptr->Len >= 4))
				{
					if (*P2PSubelementLen == 0)
					{
						RTMPMoveMemory(pP2pSubelement, &eid_ptr->Eid, (eid_ptr->Len+2));
						*P2PSubelementLen = (eid_ptr->Len+2);
					}
					else if (*P2PSubelementLen > 0)
					{
						if (((*P2PSubelementLen) + (eid_ptr->Len+2)) <= MAX_VIE_LEN)
						{
							RTMPMoveMemory(pP2pSubelement + *P2PSubelementLen, &eid_ptr->Eid, (eid_ptr->Len+2));
							*P2PSubelementLen += (eid_ptr->Len+2);
						}
						else
						{
							DBGPRINT(RT_DEBUG_ERROR, ("%s: ERROR!! 111 Sum of P2PSubelementLen= %lu, > MAX_VIE_LEN !!\n", __FUNCTION__ ,((*P2PSubelementLen) + (eid_ptr->Len+2))));
							return FALSE;
						}
					}

					DBGPRINT(RT_DEBUG_TRACE, (" ! ===>P2P - PeerAssocReqSanity  P2P IE Len becomes = %d.   %s\n", *P2PSubelementLen, decodeP2PState(pP2PCtrl->P2PConnectState)));
					break;
				}
#endif /* P2P_SUPPORT */
				if (NdisEqualMemory(eid_ptr->Octet, WPS_OUI, 4))
				{
#ifdef WSC_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
					if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.WscV2Info.bWpsEnable) ||
						(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.WscV2Info.bEnableWpsV2 == FALSE))
#endif /* WSC_V2_SUPPORT */
				    *pWscCapable = TRUE;
#endif /* WSC_AP_SUPPORT */
#ifdef EASY_CONFIG_SETUP
					AutoProvisionCheckEntry(pAd, pEntry, eid_ptr);
#endif /* EASY_CONFIG_SETUP */
				    break;
				}

				/* Handle Atheros and Broadcom draft 11n STAs */
				if (NdisEqualMemory(eid_ptr->Octet, BROADCOM_OUI, 3))
				{
					switch (eid_ptr->Octet[3])
					{
						case 0x33: 
							if ((eid_ptr->Len-4) == sizeof(HT_CAPABILITY_IE))
							{
								NdisMoveMemory(pHtCapability, &eid_ptr->Octet[4], SIZE_HT_CAP_IE);

								*(USHORT *)(&pHtCapability->HtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->HtCapInfo));
#ifdef UNALIGNMENT_SUPPORT
								{
									EXT_HT_CAP_INFO extHtCapInfo;

									NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&pHtCapability->ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
									*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
									NdisMoveMemory((PUCHAR)(&pHtCapability->ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
								}
#else				
								*(USHORT *)(&pHtCapability->ExtHtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

								*pHtCapabilityLen = SIZE_HT_CAP_IE;
							}
							break;
						
						default:
							/* ignore other cases */
							break;
					}
				}

                if (NdisEqualMemory(eid_ptr->Octet, RALINK_OUI, 3) && (eid_ptr->Len == 7))
                {
                    /**pRalinkIe = eid_ptr->Octet[3]; */
					if (eid_ptr->Octet[3] != 0)
                    	*pRalinkIe = eid_ptr->Octet[3];
        			else
        				*pRalinkIe = 0xf0000000; /* Set to non-zero value (can't set bit0-2) to represent this is Ralink Chip. So at linkup, we will set ralinkchip flag. */
                    break;
                }
                
                /* WMM_IE */
                if (NdisEqualMemory(eid_ptr->Octet, WME_INFO_ELEM, 6) && (eid_ptr->Len == 7))
                {
                    *pbWmmCapable = TRUE;

#ifdef UAPSD_SUPPORT
					if (pEntry)
					{
						UAPSD_AssocParse(pAd,
									pEntry, (UINT8 *)&eid_ptr->Octet[6],
									pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.UapsdInfo.bAPSDCapable);
					}
#endif /* UAPSD_SUPPORT */

                    break;
                }

                if (pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode < Ndis802_11AuthModeWPA)
                    break;
                
                /* 	If this IE did not begins with 00:0x50:0xf2:0x01,  
                	it would be proprietary. So we ignore it. */
                if (!NdisEqualMemory(eid_ptr->Octet, WPA1_OUI, sizeof(WPA1_OUI))
                    && !NdisEqualMemory(&eid_ptr->Octet[2], WPA2_OUI, sizeof(WPA2_OUI)))
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("Not RSN IE, maybe WMM IE!!!\n"));
                    break;                          
                }
                
                if (/*(eid_ptr->Len <= MAX_LEN_OF_RSNIE) &&*/ (eid_ptr->Len >= MIN_LEN_OF_RSNIE))
                {
					hex_dump("Received RSNIE in Assoc-Req", (UCHAR *)eid_ptr, eid_ptr->Len + 2);
                    
					/* Copy whole RSNIE context */
                    NdisMoveMemory(RSN, eid_ptr, eid_ptr->Len + 2);
					*pRSNLen=eid_ptr->Len + 2;

#ifdef DOT11R_FT_SUPPORT
					NdisMoveMemory(pFtInfo->RSN_IE, eid_ptr, eid_ptr->Len + 2);
					pFtInfo->RSNIE_Len = eid_ptr->Len + 2;
#endif /* DOT11R_FT_SUPPORT */					
                }
                else
                {
                    *pRSNLen=0;
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - missing IE_WPA(%d)\n",eid_ptr->Len));
                    return FALSE;
                }               
                break;

#ifdef WAPI_SUPPORT				
			case IE_WAPI:				
				if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode != Ndis802_11AuthModeWAICERT) &&
					(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode != Ndis802_11AuthModeWAIPSK))
                    break;				

				/* Sanity check the validity of WIE */
				/* Todo - AlbertY */
				
				/* Copy whole WAPI-IE context */
                NdisMoveMemory(RSN, eid_ptr, eid_ptr->Len + 2);
				*pRSNLen=eid_ptr->Len + 2;		
				DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - IE_WAPI(%d)\n",eid_ptr->Len));
				break;
#endif /* WAPI_SUPPORT */				

#ifdef DOT11R_FT_SUPPORT
			case IE_FT_MDIE:
				FT_FillMdIeInfo(eid_ptr, &pFtInfo->MdIeInfo);
				break;

			case IE_FT_FTIE:
				FT_FillFtIeInfo(eid_ptr, &pFtInfo->FtIeInfo);
				break;

			case IE_FT_RIC_DATA:
				/* record the pointer of first RDIE. */
				if (pFtInfo->RicInfo.pRicInfo == NULL)
				{
					pFtInfo->RicInfo.pRicInfo = &eid_ptr->Eid;
					pFtInfo->RicInfo.Len = ((UCHAR*)Fr + MsgLen)
											- (UCHAR*)eid_ptr + 1;
				}
			case IE_FT_RIC_DESCRIPTOR:
				if ((pFtInfo->RicInfo.RicIEsLen + eid_ptr->Len + 2) < MAX_RICIES_LEN)
				{
					NdisMoveMemory(&pFtInfo->RicInfo.RicIEs[pFtInfo->RicInfo.RicIEsLen],
									&eid_ptr->Eid, eid_ptr->Len + 2);
					pFtInfo->RicInfo.RicIEsLen += eid_ptr->Len + 2;
				}
				break;
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
			case IE_RRM_EN_CAP:
				{
					UINT64 value;
					NdisMoveMemory(&value, eid_ptr->Octet, sizeof(UINT64));
					pRrmEnCap->word = le2cpu64(value);
				}
				break;
#endif /* DOT11K_RRM_SUPPORT */

            default:
                break;
        }

        eid_ptr = (PEID_STRUCT)((UCHAR*)eid_ptr + 2 + eid_ptr->Len);        
    }

	if ((Sanity&0x3) != 0x03)	 
    {
        DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - missing mandatory field\n"));
        return FALSE;
    }
    else
    {
        DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - success\n"));
        return TRUE;
    }
}
#else
BOOLEAN PeerAssocReqCmmSanity(
	RTMP_ADAPTER *pAd, 
	BOOLEAN isReassoc,
	VOID *Msg,
	INT MsgLen,
	IE_LISTS *ie_lists)
{
    //CHAR *Ptr;
    PFRAME_802_11	Fr = (PFRAME_802_11)Msg;
    PEID_STRUCT eid_ptr;
    UCHAR Sanity = 0;
    UCHAR WPA1_OUI[4] = { 0x00, 0x50, 0xF2, 0x01 };
    UCHAR WPA2_OUI[3] = { 0x00, 0x0F, 0xAC };
#ifdef CONFIG_HOTSPOT_R2
	UCHAR HS2_OSEN_OUI[4] = { 0x50, 0x6f, 0x9a, 0x12 };
	UCHAR HS2OUIBYTE[4] = {0x50, 0x6f, 0x9a, 0x10};
#endif    
    MAC_TABLE_ENTRY *pEntry = (MAC_TABLE_ENTRY *)NULL;
#ifdef P2P_SUPPORT
	PRT_P2P_CONFIG	pP2PCtrl = &pAd->P2pCfg;
	UCHAR	P2POUIBYTE[4] = {0x50, 0x6f, 0x9a, 0x9};
#endif /* P2P_SUPPORT */
#ifdef DOT11R_FT_SUPPORT
	PFT_INFO *pFtInfo = &ie_lists->FtInfo;
#endif /* DOT11R_FT_SUPPORT */
#ifdef DOT11K_RRM_SUPPORT
	RRM_EN_CAP_IE *pRrmEnCap = &ie_lists->RrmEnCap;
#endif /* DOT11K_RRM_SUPPORT */
#ifdef P2P_SUPPORT
	UCHAR *pP2pSubelement = &ie_lists->P2pSubelement[0];
#endif /* P2P_SUPPORT */
	HT_CAPABILITY_IE *pHtCapability = &ie_lists->HTCapability;


	pEntry = MacTableLookup(pAd, &Fr->Hdr.Addr2[0]);
	if (pEntry == NULL)
		return FALSE;

	COPY_MAC_ADDR(&ie_lists->Addr2[0], &Fr->Hdr.Addr2[0]);
	
	//Ptr = (PCHAR)Fr->Octet;

	NdisMoveMemory(&ie_lists->CapabilityInfo, &Fr->Octet[0], 2);
	NdisMoveMemory(&ie_lists->ListenInterval, &Fr->Octet[2], 2);

	if (isReassoc) 
	{
		NdisMoveMemory(&ie_lists->ApAddr[0], &Fr->Octet[4], 6);
		eid_ptr = (PEID_STRUCT) &Fr->Octet[10];
	}
	else
	{
		eid_ptr = (PEID_STRUCT) &Fr->Octet[4];
	}


    /* get variable fields from payload and advance the pointer */
    while (((UCHAR *)eid_ptr + eid_ptr->Len + 1) < ((UCHAR *)Fr + MsgLen))
    {
        switch(eid_ptr->Eid)
        {
            case IE_SSID:
			if (((Sanity&0x1) == 1))
				break;

                if ((eid_ptr->Len <= MAX_LEN_OF_SSID))
                {
                    Sanity |= 0x01;
                    NdisMoveMemory(&ie_lists->Ssid[0], eid_ptr->Octet, eid_ptr->Len);
                    ie_lists->SsidLen = eid_ptr->Len;
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - SsidLen = %d  \n", ie_lists->SsidLen));
                }
                else
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - wrong IE_SSID\n"));
                    return FALSE;
                }
                break;

            case IE_SUPP_RATES:
                if ((eid_ptr->Len <= MAX_LEN_OF_SUPPORTED_RATES) &&
					(eid_ptr->Len > 0))
                {
                    Sanity |= 0x02;
                    NdisMoveMemory(&ie_lists->SupportedRates[0], eid_ptr->Octet, eid_ptr->Len);

                    DBGPRINT(RT_DEBUG_TRACE,
						("PeerAssocReqSanity - IE_SUPP_RATES., Len=%d. "
						"Rates[0]=%x\n", eid_ptr->Len, ie_lists->SupportedRates[0]));
                    DBGPRINT(RT_DEBUG_TRACE,
						("Rates[1]=%x %x %x %x %x %x %x\n",
						ie_lists->SupportedRates[1], ie_lists->SupportedRates[2],
						ie_lists->SupportedRates[3], ie_lists->SupportedRates[4],
						ie_lists->SupportedRates[5], ie_lists->SupportedRates[6],
						ie_lists->SupportedRates[7]));

                    ie_lists->SupportedRatesLen = eid_ptr->Len;
                }
                else
                {
					UCHAR RateDefault[8] = \
							{ 0x82, 0x84, 0x8b, 0x96, 0x12, 0x24, 0x48, 0x6c };

                	/* HT rate not ready yet. return true temporarily. rt2860c */
                    /*DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - wrong IE_SUPP_RATES\n")); */
                    Sanity |= 0x02;
                    ie_lists->SupportedRatesLen = 8;
					NdisMoveMemory(&ie_lists->SupportedRates[0], RateDefault, 8);

                    DBGPRINT(RT_DEBUG_TRACE,
						("PeerAssocReqSanity - wrong IE_SUPP_RATES., Len=%d\n",
						eid_ptr->Len));
                }
                break;

            case IE_EXT_SUPP_RATES:
                if (eid_ptr->Len + ie_lists->SupportedRatesLen <= MAX_LEN_OF_SUPPORTED_RATES)
                {
                    NdisMoveMemory(&ie_lists->SupportedRates[ie_lists->SupportedRatesLen], eid_ptr->Octet,
									eid_ptr->Len);
                    ie_lists->SupportedRatesLen += eid_ptr->Len;
                }
                else
                {
                    NdisMoveMemory(&ie_lists->SupportedRates[ie_lists->SupportedRatesLen], eid_ptr->Octet,
									MAX_LEN_OF_SUPPORTED_RATES - (ie_lists->SupportedRatesLen));
                    ie_lists->SupportedRatesLen = MAX_LEN_OF_SUPPORTED_RATES;
                }
                break;
                
            case IE_HT_CAP:
			if (eid_ptr->Len >= sizeof(HT_CAPABILITY_IE))
			{
				NdisMoveMemory(pHtCapability, eid_ptr->Octet, SIZE_HT_CAP_IE);

				*(USHORT *)(&pHtCapability->HtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->HtCapInfo));

#ifdef UNALIGNMENT_SUPPORT
				{
					EXT_HT_CAP_INFO extHtCapInfo;

					NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&pHtCapability->ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
					*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
					NdisMoveMemory((PUCHAR)(&pHtCapability->ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
				}
#else				
				*(USHORT *)(&pHtCapability->ExtHtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

				ie_lists->ht_cap_len = SIZE_HT_CAP_IE;
				Sanity |= 0x10;
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - IE_HT_CAP\n"));
			}
			else
			{
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - wrong IE_HT_CAP.eid_ptr->Len = %d\n", eid_ptr->Len));
			}
				
		break;
		case IE_EXT_CAPABILITY:
			if (eid_ptr->Len)
			{
				INT ext_len = eid_ptr->Len;

				ext_len = ext_len > sizeof(EXT_CAP_INFO_ELEMENT) ? sizeof(EXT_CAP_INFO_ELEMENT) : ext_len;
				NdisMoveMemory(&ie_lists->ExtCapInfo, eid_ptr->Octet, ext_len);
				DBGPRINT(RT_DEBUG_WARN, ("PeerAssocReqSanity - IE_EXT_CAPABILITY!\n"));
			}

			break;

            case IE_WPA:    /* same as IE_VENDOR_SPECIFIC */
            case IE_WPA2:
#ifdef DOT11R_FT_SUPPORT
#ifdef WMM_ACM_SUPPORT
				if (ACM_WME_ELM_Check((UCHAR *)eid_ptr,
					ACM_WME_OUI_SUBTYPE_TSPEC) == ACM_RTN_OK)
				{
					if ((pFtInfo->RicInfo.RicIEsLen + eid_ptr->Len + 2) < MAX_RICIES_LEN)
					{
						NdisMoveMemory(&pFtInfo->RicInfo.RicIEs[pFtInfo->RicInfo.RicIEsLen],
										&eid_ptr->Eid, eid_ptr->Len + 2);
						pFtInfo->RicInfo.RicIEsLen += eid_ptr->Len + 2;
					}
				}
#endif /* WMM_ACM_SUPPORT */
#endif /* DOT11R_FT_SUPPORT */
#ifdef CONFIG_HOTSPOT_R2
				if (NdisEqualMemory(eid_ptr->Octet, HS2OUIBYTE, sizeof(HS2OUIBYTE)) && (eid_ptr->Len >= 5))
				{
					//UCHAR tmp2 = 0x12;
					UCHAR *hs2_config = (UCHAR *)&eid_ptr->Octet[4];
					UCHAR ppomo_exist = ((*hs2_config) >> 1) & 0x01;
					UCHAR hs2_version = ((*hs2_config) >> 4) & 0x0f;
					//UCHAR *tmp3 = (UCHAR *)&pEntry->hs_info.ppsmo_id;
					//UCHAR tmp[2] = {0x12,0x34};

					pEntry->hs_info.version = hs2_version;
					pEntry->hs_info.ppsmo_exist = ppomo_exist;	
					if (pEntry->hs_info.ppsmo_exist)
					{
						NdisMoveMemory(&pEntry->hs_info.ppsmo_id, &eid_ptr->Octet[5], 2);
						//NdisMoveMemory(tmp3, tmp, 2);
					}				
					DBGPRINT(RT_DEBUG_OFF,
							("Assoc HS2 STA:version:%d,ppomo exist:%d, value:0x%x\n",
							pEntry->hs_info.version, pEntry->hs_info.ppsmo_exist,
							pEntry->hs_info.ppsmo_id));
					break;
				}
#endif /* CONFIG_HOTSPOT_R2 */
#ifdef P2P_SUPPORT
				if (NdisEqualMemory(eid_ptr->Octet, P2POUIBYTE, sizeof(P2POUIBYTE)) && (eid_ptr->Len >= 4))
				{
					if (ie_lists->P2PSubelementLen == 0)
					{
						RTMPMoveMemory(pP2pSubelement, &eid_ptr->Eid, (eid_ptr->Len+2));
						ie_lists->P2PSubelementLen = (eid_ptr->Len+2);
					}
					else if (ie_lists->P2PSubelementLen > 0)
					{
						if (((ie_lists->P2PSubelementLen) + (eid_ptr->Len+2)) <= MAX_VIE_LEN)
						{
						RTMPMoveMemory(pP2pSubelement + ie_lists->P2PSubelementLen, &eid_ptr->Eid, (eid_ptr->Len+2));
						ie_lists->P2PSubelementLen += (eid_ptr->Len+2);
						}
						else
						{
							DBGPRINT(RT_DEBUG_ERROR, ("%s: ERROR!! 111 Sum of P2PSubelementLen= %lu, > MAX_VIE_LEN !!\n", __FUNCTION__ ,(ie_lists->P2PSubelementLen + (eid_ptr->Len+2))));
							return FALSE;
						}
					}

					DBGPRINT(RT_DEBUG_TRACE, (" ! ===>P2P - PeerAssocReqSanity  P2P IE Len becomes = %d.   %s\n", ie_lists->P2PSubelementLen, decodeP2PState(pP2PCtrl->P2PConnectState)));
					break;
				}
#endif /* P2P_SUPPORT */

				if (NdisEqualMemory(eid_ptr->Octet, WPS_OUI, 4))
				{
#ifdef WSC_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
					if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.WscV2Info.bWpsEnable) ||
						(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.WscV2Info.bEnableWpsV2 == FALSE))
#endif /* WSC_V2_SUPPORT */
						ie_lists->bWscCapable = TRUE;
#endif /* WSC_AP_SUPPORT */
#ifdef EASY_CONFIG_SETUP
					AutoProvisionCheckEntry(pAd, pEntry, eid_ptr);
#endif /* EASY_CONFIG_SETUP */
					break;
				}

				/* Handle Atheros and Broadcom draft 11n STAs */
				if (NdisEqualMemory(eid_ptr->Octet, BROADCOM_OUI, 3))
				{
					switch (eid_ptr->Octet[3])
					{
						case 0x33: 
							if ((eid_ptr->Len-4) == sizeof(HT_CAPABILITY_IE))
							{
								NdisMoveMemory(pHtCapability, &eid_ptr->Octet[4], SIZE_HT_CAP_IE);

								*(USHORT *)(&pHtCapability->HtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->HtCapInfo));
#ifdef UNALIGNMENT_SUPPORT
								{
									EXT_HT_CAP_INFO extHtCapInfo;

									NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&pHtCapability->ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
									*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
									NdisMoveMemory((PUCHAR)(&pHtCapability->ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
								}
#else				
								*(USHORT *)(&pHtCapability->ExtHtCapInfo) = cpu2le16(*(USHORT *)(&pHtCapability->ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

								ie_lists->ht_cap_len = SIZE_HT_CAP_IE;
							}
							break;
						
						default:
							/* ignore other cases */
							break;
					}
				}

                if (NdisEqualMemory(eid_ptr->Octet, RALINK_OUI, 3) && (eid_ptr->Len == 7))
                {
			if (eid_ptr->Octet[3] != 0)
		       	ie_lists->RalinkIe = eid_ptr->Octet[3];
        		else
        			ie_lists->RalinkIe = 0xf0000000; /* Set to non-zero value (can't set bit0-2) to represent this is Ralink Chip. So at linkup, we will set ralinkchip flag. */
                    break;
                }
                
                /* WMM_IE */
                if (NdisEqualMemory(eid_ptr->Octet, WME_INFO_ELEM, 6) && (eid_ptr->Len == 7))
                {
                    ie_lists->bWmmCapable = TRUE;

#ifdef UAPSD_SUPPORT
			if (pEntry)
			{
				UAPSD_AssocParse(pAd,
							pEntry, (UINT8 *)&eid_ptr->Octet[6],
							pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.UapsdInfo.bAPSDCapable);
			}
#endif /* UAPSD_SUPPORT */

                    break;
                }

                if (pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.AuthMode < Ndis802_11AuthModeWPA)
                    break;
                
                /* 	If this IE did not begins with 00:0x50:0xf2:0x01,  
                	it would be proprietary. So we ignore it. */
                if (!NdisEqualMemory(eid_ptr->Octet, WPA1_OUI, sizeof(WPA1_OUI))
                    && !NdisEqualMemory(&eid_ptr->Octet[2], WPA2_OUI, sizeof(WPA2_OUI)))
                {
                    DBGPRINT(RT_DEBUG_TRACE, ("Not RSN IE, maybe WMM IE!!!\n"));
                    
#ifdef CONFIG_HOTSPOT_R2
			if (!NdisEqualMemory(eid_ptr->Octet, HS2_OSEN_OUI, sizeof(HS2_OSEN_OUI)))
			{
				unsigned char *tmp = (unsigned char *)eid_ptr->Octet;

				DBGPRINT(RT_DEBUG_OFF, ("!!!!!!not found OSEN IE,%x:%x:%x:%x\n", *tmp, *(tmp+1), *(tmp+2), *(tmp+3)));
				CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_OSEN_CAPABLE);
                    break;                          
                }
			else
			{
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_OSEN_CAPABLE);
				DBGPRINT(RT_DEBUG_OFF, ("!!!!!!found OSEN IE\n"));
			}
#else
                   	break;                          
#endif          
                }
                
                if (/*(eid_ptr->Len <= MAX_LEN_OF_RSNIE) &&*/ (eid_ptr->Len >= MIN_LEN_OF_RSNIE))
                {
					hex_dump("Received RSNIE in Assoc-Req", (UCHAR *)eid_ptr, eid_ptr->Len + 2);
                    
					/* Copy whole RSNIE context */
                    NdisMoveMemory(&ie_lists->RSN_IE[0], eid_ptr, eid_ptr->Len + 2);
					ie_lists->RSNIE_Len =eid_ptr->Len + 2;

#ifdef DOT11R_FT_SUPPORT
					NdisMoveMemory(pFtInfo->RSN_IE, eid_ptr, eid_ptr->Len + 2);
					pFtInfo->RSNIE_Len = eid_ptr->Len + 2;
#endif /* DOT11R_FT_SUPPORT */					
                }
                else
                {
                    ie_lists->RSNIE_Len = 0;
                    DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - missing IE_WPA(%d)\n",eid_ptr->Len));
                    return FALSE;
                }               
                break;

#ifdef WAPI_SUPPORT				
			case IE_WAPI:				
				if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.AuthMode != Ndis802_11AuthModeWAICERT) &&
					(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.AuthMode != Ndis802_11AuthModeWAIPSK))
                    break;				

				/* Sanity check the validity of WIE */
				/* Todo - AlbertY */
				
				/* Copy whole WAPI-IE context */
                NdisMoveMemory(&ie_lists->RSN_IE[0], eid_ptr, eid_ptr->Len + 2);
				ie_lists->RSNIE_Len =eid_ptr->Len + 2;		
				DBGPRINT(RT_DEBUG_TRACE, ("PeerAssocReqSanity - IE_WAPI(%d)\n",eid_ptr->Len));
				break;
#endif /* WAPI_SUPPORT */				

#ifdef DOT11R_FT_SUPPORT
			case IE_FT_MDIE:
				FT_FillMdIeInfo(eid_ptr, &pFtInfo->MdIeInfo);
				break;

			case IE_FT_FTIE:
				FT_FillFtIeInfo(eid_ptr, &pFtInfo->FtIeInfo);
				break;

			case IE_FT_RIC_DATA:
				/* record the pointer of first RDIE. */
				if (pFtInfo->RicInfo.pRicInfo == NULL)
				{
					pFtInfo->RicInfo.pRicInfo = &eid_ptr->Eid;
					pFtInfo->RicInfo.Len = ((UCHAR*)Fr + MsgLen) - (UCHAR*)eid_ptr + 1;
				}
			case IE_FT_RIC_DESCRIPTOR:
				if ((pFtInfo->RicInfo.RicIEsLen + eid_ptr->Len + 2) < MAX_RICIES_LEN)
				{
					NdisMoveMemory(&pFtInfo->RicInfo.RicIEs[pFtInfo->RicInfo.RicIEsLen],
									&eid_ptr->Eid, eid_ptr->Len + 2);
					pFtInfo->RicInfo.RicIEsLen += eid_ptr->Len + 2;
				}
				break;
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
			case IE_RRM_EN_CAP:
				{
					UINT64 value;
					NdisMoveMemory(&value, eid_ptr->Octet, sizeof(UINT64));
					pRrmEnCap->word = le2cpu64(value);
				}
				break;
#endif /* DOT11K_RRM_SUPPORT */

#ifdef DOT11_VHT_AC
		case IE_VHT_CAP:
			if (eid_ptr->Len == sizeof(VHT_CAP_IE))
			{
				NdisMoveMemory(&ie_lists->vht_cap, eid_ptr->Octet, sizeof(VHT_CAP_IE));
				ie_lists->vht_cap_len = eid_ptr->Len;
				DBGPRINT(RT_DEBUG_TRACE, ("%s():IE_VHT_CAP\n", __FUNCTION__));
			}
			else
			{
				DBGPRINT(RT_DEBUG_WARN, ("%s():wrong IE_VHT_CAP, eid->Len = %d\n",
							__FUNCTION__, eid_ptr->Len));
			}

		case IE_VHT_OP:
			if (eid_ptr->Len == sizeof(VHT_OP_IE))
			{
				NdisMoveMemory(&ie_lists->vht_op, eid_ptr->Octet, sizeof(VHT_OP_IE));
				ie_lists->vht_op_len = eid_ptr->Len;
				DBGPRINT(RT_DEBUG_TRACE, ("%s():IE_VHT_OP\n", __FUNCTION__));
			}

		case IE_OPERATING_MODE_NOTIFY:
			if (eid_ptr->Len == sizeof(OPERATING_MODE)) {
				ie_lists->operating_mode_len = sizeof(OPERATING_MODE);
				NdisMoveMemory(&ie_lists->operating_mode, &eid_ptr->Octet[0], sizeof(OPERATING_MODE));
				DBGPRINT(RT_DEBUG_TRACE, ("%s():IE_OPERATING_MODE_NOTIFY!\n", __FUNCTION__));
			}
			break;
#endif /* DOT11_VHT_AC */

            default:
                break;
        }

        eid_ptr = (PEID_STRUCT)((UCHAR*)eid_ptr + 2 + eid_ptr->Len);        
    }

	if ((Sanity&0x3) != 0x03)	 
	{
		DBGPRINT(RT_DEBUG_WARN, ("%s(): - missing mandatory field\n", __FUNCTION__));
		return FALSE;
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s() - success\n", __FUNCTION__));
		return TRUE;
	}
}
#endif


VOID ap_cmm_peer_assoc_req_action(
    IN PRTMP_ADAPTER pAd,
    IN MLME_QUEUE_ELEM *Elem,
	IN BOOLEAN isReassoc)
{
	struct wifi_dev *wdev;
	BSS_STRUCT *pMbss;
	BOOLEAN bAssocSkip = FALSE;
	CHAR rssi;
	IE_LISTS *ie_list = NULL;
	HEADER_802_11 AssocRspHdr;
	USHORT CapabilityInfoForAssocResp;
	USHORT StatusCode = MLME_SUCCESS;
	USHORT Aid;
	PUCHAR pOutBuffer = NULL;
	NDIS_STATUS NStatus;
	ULONG FrameLen = 0;
	UCHAR MaxSupportedRate = 0;
	UCHAR SupRateLen, PhyMode, FlgIs11bSta;
	UCHAR i;
	MAC_TABLE_ENTRY *pEntry = NULL;
	STA_TR_ENTRY *tr_entry = NULL;

#ifdef DBG
	UCHAR *sAssoc = isReassoc ? (PUCHAR)"ReASSOC" : (PUCHAR)"ASSOC";
#endif /* DBG */
	UCHAR SubType;
	BOOLEAN bACLReject = FALSE;
#ifdef DOT1X_SUPPORT
	PUINT8 pPmkid = NULL;
	UINT8 pmkid_count = 0;
#endif /* DOT1X_SUPPORT */
#ifdef DOT11R_FT_SUPPORT
	PFT_CFG pFtCfg = NULL;
	FT_INFO FtInfoBuf;
#endif /* DOT11R_FT_SUPPORT */
#ifdef WSC_AP_SUPPORT
	WSC_CTRL *wsc_ctrl;
#endif /* WSC_AP_SUPPORT */
#ifdef RT_BIG_ENDIAN
	UINT32 tmp_1;
	UINT64 tmp_2;
#endif /*RT_BIG_ENDIAN*/


	/* disallow new association */
	if (pAd->ApCfg.BANClass3Data == TRUE) {
		DBGPRINT(RT_DEBUG_TRACE, ("Disallow new Association\n"));
		return;
	}
		
	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&ie_list, sizeof(IE_LISTS));
	if (ie_list == NULL) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s(): mem alloc failed\n", __FUNCTION__));
		return;
	}
	NdisZeroMemory(ie_list, sizeof(IE_LISTS));
					
	if (!PeerAssocReqCmmSanity(pAd, isReassoc, Elem->Msg, Elem->MsgLen, ie_list))
		goto LabelOK;

	pEntry = MacTableLookup(pAd, ie_list->Addr2);
	if (!pEntry) {
		DBGPRINT(RT_DEBUG_ERROR, ("NoAuth MAC - %02x:%02x:%02x:%02x:%02x:%02x\n",
					PRINT_MAC(ie_list->Addr2)));
		goto LabelOK;
	}
	
	if (!VALID_MBSS(pAd, pEntry->func_tb_idx)) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s():pEntry bounding invalid wdev(apidx=%d)\n",
					__FUNCTION__, pEntry->func_tb_idx));
		goto LabelOK;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s():pEntry->func_tb_idx=%d\n", __FUNCTION__, pEntry->func_tb_idx));
	pMbss = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx];
	wdev = &pMbss->wdev;
	tr_entry = &pAd->MacTab.tr_entry[pEntry->wcid];

	if(tr_entry == NULL)
		DBGPRINT(RT_DEBUG_TRACE, ("tr_entry is NULL.\n"));

#ifdef WSC_AP_SUPPORT
	wsc_ctrl = &pMbss->WscControl;
#endif /* WSC_AP_SUPPORT */

	PhyMode = wdev->PhyMode;

	FlgIs11bSta = 1;
	for(i=0; i<ie_list->SupportedRatesLen; i++)
	{
		if (((ie_list->SupportedRates[i] & 0x7F) != 2) &&
			((ie_list->SupportedRates[i] & 0x7F) != 4) &&
			((ie_list->SupportedRates[i] & 0x7F) != 11) &&
			((ie_list->SupportedRates[i] & 0x7F) != 22))
		{
			FlgIs11bSta = 0;
			break;
		}
	}
    
#ifdef DOT11W_PMF_SUPPORT
        if ((tr_entry->PortSecured == WPA_802_1X_PORT_SECURED)
                && (CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_PMF_CAPABLE)))
        {
                StatusCode = MLME_ASSOC_REJ_TEMPORARILY;
                goto SendAssocResponse;
        }
#endif /* DOT11W_PMF_SUPPORT */
    
	/* clear the previous Pairwise key table */
    if(pEntry->Aid != 0 &&
	(pEntry->WepStatus >= Ndis802_11TKIPEnable 
#ifdef DOT1X_SUPPORT
	|| wdev->IEEE8021X
#endif /* DOT1X_SUPPORT */
	))
    {
		/* clear GTK state */
		pEntry->GTKState = REKEY_NEGOTIATING;
    	
		NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));

		/* clear this entry as no-security mode */
		AsicRemovePairwiseKeyEntry(pAd, pEntry->wcid);

#ifdef MT_MAC										
		if (pAd->chipCap.hif_type == HIF_MT)
			CmdProcAddRemoveKey(pAd, 1, pEntry->apidx, 0, pEntry->wcid, PAIRWISEKEYTABLE, &pEntry->PairwiseKey, pEntry->Addr);
#endif

#ifdef DOT1X_SUPPORT
		/* Notify 802.1x daemon to clear this sta info */
		if (pEntry->AuthMode == Ndis802_11AuthModeWPA || 
			pEntry->AuthMode == Ndis802_11AuthModeWPA2 ||
			wdev->IEEE8021X)
		DOT1X_InternalCmdAction(pAd, pEntry, DOT1X_DISCONNECT_ENTRY);
#endif /* DOT1X_SUPPORT */

#ifdef WAPI_SUPPORT
		WAPI_InternalCmdAction(pAd, 
							   pEntry->AuthMode, 
							   pEntry->func_tb_idx, 
							   pEntry->Addr, 
							   WAI_MLME_DISCONNECT);

		RTMPCancelWapiRekeyTimerAction(pAd, pEntry);
#endif /* WAPI_SUPPORT */
    }

#ifdef MT_PS
#ifdef MT_MAC										
#if 0
	if (pAd->chipCap.hif_type == HIF_MT)
	{
		if ((tr_entry->ps_state != APPS_RETRIEVE_IDLE) 
			&& (tr_entry->ps_state != APPS_RETRIEVE_DONE))
		{
			DBGPRINT(RT_DEBUG_WARN, ("%s(%d) switch ps status(%d) to waiting state and disable redirect mode.\n", 
				__FUNCTION__, __LINE__, tr_entry->ps_state));
			tr_entry->ps_state = APPS_RETRIEVE_WAIT_EVENT;
			RTEnqueueInternalCmd(pAd, CMDTHREAD_PS_CLEAR, pEntry, sizeof(MAC_TABLE_ENTRY));
		}
	}
#else

	tr_entry->ps_state = APPS_RETRIEVE_IDLE;

#endif
#endif /* MT_MAC */
#endif /* MT_PS */

#ifdef WSC_AP_SUPPORT
    /* since sta has been left, ap should receive EapolStart and EapRspId again. */
	pEntry->Receive_EapolStart_EapRspId = 0;
	pEntry->bWscCapable = ie_list->bWscCapable;
#ifdef WSC_V2_SUPPORT
	if ((wsc_ctrl->WscV2Info.bEnableWpsV2) &&
		(wsc_ctrl->WscV2Info.bWpsEnable == FALSE))
		;
	else
#endif /* WSC_V2_SUPPORT */
	{
	    if (pEntry->func_tb_idx < pAd->ApCfg.BssidNum)
	    {
	        if (MAC_ADDR_EQUAL(pEntry->Addr, wsc_ctrl->EntryAddr))
	        {
	            BOOLEAN Cancelled;
	            RTMPZeroMemory(wsc_ctrl->EntryAddr, MAC_ADDR_LEN);
	            RTMPCancelTimer(&wsc_ctrl->EapolTimer, &Cancelled);
	            wsc_ctrl->EapolTimerRunning = FALSE;
	        }
	    }
			
	    if ((ie_list->RSNIE_Len == 0) &&
		 (wdev->AuthMode >= Ndis802_11AuthModeWPA) &&
		 (wsc_ctrl->WscConfMode != WSC_DISABLE))
	        pEntry->bWscCapable = TRUE;
	}
#endif /* WSC_AP_SUPPORT */

#ifdef EASY_CONFIG_SETUP
	if (pEntry->bRaAutoWpsCapable)
		;
	else
#endif /* EASY_CONFIG_SETUP */
    /* for hidden SSID sake, SSID in AssociateRequest should be fully verified */
	if ((ie_list->SsidLen != pMbss->SsidLen) ||
		(NdisEqualMemory(ie_list->Ssid, pMbss->Ssid, ie_list->SsidLen)==0))
        goto LabelOK;
        
#ifdef WSC_V2_SUPPORT
	/* Do not check ACL when WPS V2 is enabled and ACL policy is positive. */
	if ((pEntry->bWscCapable) &&
		(wsc_ctrl->WscConfMode != WSC_DISABLE) &&
		(wsc_ctrl->WscV2Info.bEnableWpsV2) &&
		(wsc_ctrl->WscV2Info.bWpsEnable) &&
		(MAC_ADDR_EQUAL(wsc_ctrl->EntryAddr, ZERO_MAC_ADDR)))
		;
	else
#endif /* WSC_V2_SUPPORT */        
	/* set a flag for sending Assoc-Fail response to unwanted STA later. */
	if (!ApCheckAccessControlList(pAd, ie_list->Addr2, pEntry->func_tb_idx))
		bACLReject = TRUE;

	DBGPRINT(RT_DEBUG_OFF, ("%s - MBSS(%d), receive %s request from %02x:%02x:%02x:%02x:%02x:%02x\n",
				sAssoc, pEntry->func_tb_idx, sAssoc, PRINT_MAC(ie_list->Addr2)));
    
	/* supported rates array may not be sorted. sort it and find the maximum rate */
	for (i=0; i<ie_list->SupportedRatesLen; i++)
	{
		if (MaxSupportedRate < (ie_list->SupportedRates[i] & 0x7f))
			MaxSupportedRate = ie_list->SupportedRates[i] & 0x7f;
	}

	/*
		Assign RateLen here or we will select wrong rate table in
		APBuildAssociation() when 11N compile option is disabled.
	*/
	pEntry->RateLen = ie_list->SupportedRatesLen;

#ifdef DOT11_VHT_AC
#ifdef RT_BIG_ENDIAN
	// TODO: shiang-usw, change this endian swap mechanism!
	NdisCopyMemory(&tmp_1,&ie_list->vht_cap.vht_cap, 4);
	tmp_1=SWAP32(tmp_1);
	NdisCopyMemory(&ie_list->vht_cap.vht_cap,&tmp_1, 4);
	
	NdisCopyMemory(&tmp_2,&(ie_list->vht_cap.mcs_set), 8);
	tmp_2=SWAP64(tmp_2);
	NdisCopyMemory(&(ie_list->vht_cap.mcs_set),&tmp_2, 8);
	//SWAP32((UINT32)vht_cap_ie.vht_cap);
	//SWAP32((UINT32)vht_cap_ie.mcs_set);
#endif /* RT_BIG_ENDIAN */
#endif

	RTMPSetSupportMCS(pAd,
					OPMODE_AP,
					pEntry,
					ie_list->SupportedRates,
					ie_list->SupportedRatesLen,
					NULL,
					0,
#ifdef DOT11_VHT_AC
					ie_list->vht_cap_len,
					&ie_list->vht_cap,
#endif /* DOT11_VHT_AC */
					&ie_list->HTCapability,
					ie_list->ht_cap_len);

	/* 2. qualify this STA's auth_asoc status in the MAC table, decide StatusCode */
	StatusCode = APBuildAssociation(pAd, pEntry, ie_list, MaxSupportedRate, &Aid);

#ifdef DOT11R_FT_SUPPORT
	if (pEntry->func_tb_idx < pAd->ApCfg.BssidNum)
	{
		pFtCfg = &(wdev->FtCfg);
		if ((pFtCfg->FtCapFlag.Dot11rFtEnable)
			&& (StatusCode == MLME_SUCCESS))
			StatusCode = FT_AssocReqHandler(pAd, isReassoc, pFtCfg, pEntry,
							&ie_list->FtInfo, &FtInfoBuf);

		/* just silencely discard this frame */
		if (StatusCode == 0xFFFF)
			goto LabelOK;
	}
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
	if ((pEntry->func_tb_idx < pAd->ApCfg.BssidNum)
		&& IS_RRM_ENABLE(pAd, pEntry->func_tb_idx))
	{
		pEntry->RrmEnCap.word = ie_list->RrmEnCap.word;
	}
#endif /* DOT11K_RRM_SUPPORT */

#ifdef DOT11_VHT_AC
	if (ie_list->vht_cap_len)
	{
//+++Add by shiang for debug
		if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode)) {
			DBGPRINT(RT_DEBUG_TRACE, ("%s():Peer is VHT capable device!\n", __FUNCTION__));

			NdisMoveMemory(&pEntry->ext_cap, &ie_list->ExtCapInfo, sizeof(ie_list->ExtCapInfo));
			DBGPRINT(RT_DEBUG_TRACE, ("\tOperatingModeNotification=%d\n",
						pEntry->ext_cap.operating_mode_notification));
			//dump_vht_cap(pAd, &ie_list->vht_cap);
		}
//---Add by shiang for debug
	}
#endif /* DOT11_VHT_AC */

	if (StatusCode == MLME_ASSOC_REJ_DATA_RATE)
		RTMPSendWirelessEvent(pAd, IW_STA_MODE_EVENT_FLAG, pEntry->Addr, wdev->wdev_idx, 0);

#ifdef DOT11W_PMF_SUPPORT
SendAssocResponse:
#endif /* DOT11W_PMF_SUPPORT */
	/* 3. send Association Response */
	NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
	if (NStatus != NDIS_STATUS_SUCCESS)
		goto LabelOK;
        
	DBGPRINT(RT_DEBUG_OFF, ("%s - Send %s response (Status=%d)...\n", sAssoc, sAssoc, StatusCode));
	Aid |= 0xc000; /* 2 most significant bits should be ON */

	SubType = isReassoc ? SUBTYPE_REASSOC_RSP : SUBTYPE_ASSOC_RSP;

	CapabilityInfoForAssocResp = pMbss->CapabilityInfo; /*use AP's cability */
#ifdef WSC_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
	if ((wsc_ctrl->WscV2Info.bEnableWpsV2) &&
		(wsc_ctrl->WscV2Info.bWpsEnable == FALSE))
		;
	else
#endif /* WSC_V2_SUPPORT */
	{
		if ((wsc_ctrl->WscConfMode != WSC_DISABLE) &&
			(ie_list->CapabilityInfo & 0x0010))
		{
			CapabilityInfoForAssocResp |= 0x0010;
		}
	}
#endif /* WSC_AP_SUPPORT */
	
	/* fail in ACL checking => send an Assoc-Fail resp. */
	SupRateLen = pAd->CommonCfg.SupRateLen;

	/* TODO: need to check rate in support rate element, not number */
	if (FlgIs11bSta == 1)
		SupRateLen = 4;

	/* YF@20120419: Refuse the weak signal of AssocReq */
	rssi = RTMPMaxRssi(pAd,
						ConvertToRssi(pAd, &Elem->rssi_info, RSSI_IDX_0),
						ConvertToRssi(pAd, &Elem->rssi_info, RSSI_IDX_1),
						ConvertToRssi(pAd, &Elem->rssi_info, RSSI_IDX_2));
	DBGPRINT(RT_DEBUG_TRACE, ("ra[%d] ASSOC_REQ Threshold = %d, PktMaxRssi=%d\n",
				pEntry->func_tb_idx, pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AssocReqRssiThreshold,
				rssi));
	if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AssocReqRssiThreshold != 0) && (rssi < pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AssocReqRssiThreshold))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Reject this ASSOC_REQ due to Weak Signal.\n"));
		bAssocSkip = TRUE;
	}

	if (bACLReject == TRUE || bAssocSkip)
	{
		MgtMacHeaderInit(pAd, &AssocRspHdr, SubType, 0, ie_list->Addr2, 
							wdev->if_addr, wdev->bssid);
		StatusCode = MLME_UNSPECIFY_FAIL;
		MakeOutgoingFrame(pOutBuffer, &FrameLen,
			              sizeof(HEADER_802_11), &AssocRspHdr,
			              2,                        &CapabilityInfoForAssocResp,
			              2,                        &StatusCode,
			              2,                        &Aid,
			              1,                        &SupRateIe,
			              1,                        &SupRateLen,
			              SupRateLen, pAd->CommonCfg.SupRate,
			              END_OF_ARGS);
		MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
		MlmeFreeMemory(pAd, (PVOID) pOutBuffer);

		RTMPSendWirelessEvent(pAd, IW_MAC_FILTER_LIST_EVENT_FLAG, ie_list->Addr2, wdev->wdev_idx, 0);

#ifdef WSC_V2_SUPPORT
		/* If this STA exists, delete it. */
		if (pEntry)
			MacTableDeleteEntry(pAd, pEntry->wcid, pEntry->Addr);
#endif /* WSC_V2_SUPPORT */

		if (bAssocSkip == TRUE)
		{
			pEntry = MacTableLookup(pAd, ie_list->Addr2);
			if (pEntry)
				MacTableDeleteEntry(pAd, pEntry->wcid, pEntry->Addr);
		}

		goto LabelOK;
	}

	MgtMacHeaderInit(pAd, &AssocRspHdr, SubType, 0, ie_list->Addr2, 
						wdev->if_addr, wdev->bssid);
#ifdef RT_CFG80211_P2P_SUPPORT
        if (RTMP_CFG80211_VIF_P2P_GO_ON(pAd))
        {

	MakeOutgoingFrame(pOutBuffer, &FrameLen,
	                  sizeof(HEADER_802_11), &AssocRspHdr,
	                  2,                        &CapabilityInfoForAssocResp,
	                  2,                        &StatusCode,
	                  2,                        &Aid,
	                  1,                        &SupRateIe,
                          1,                        &pAd->cfg80211_ctrl.P2pSupRateLen,
                          pAd->cfg80211_ctrl.P2pSupRateLen, pAd->cfg80211_ctrl.P2pSupRate,
                          END_OF_ARGS);


        }
        else
#endif /* RT_CFG80211_P2P_SUPPORT */
        {
	MakeOutgoingFrame(pOutBuffer, &FrameLen,
	                  sizeof(HEADER_802_11), &AssocRspHdr,
	                  2,                        &CapabilityInfoForAssocResp,
	                  2,                        &StatusCode,
	                  2,                        &Aid,
	                  1,                        &SupRateIe,
	                  1,                        &SupRateLen,
	                  SupRateLen, pAd->CommonCfg.SupRate,
	                  END_OF_ARGS);
	}

	if ((pAd->CommonCfg.ExtRateLen) && (PhyMode != WMODE_B) && (FlgIs11bSta == 0))
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen,
		                  1,                        &ExtRateIe,
		                  1,                        &pAd->CommonCfg.ExtRateLen,
		                  pAd->CommonCfg.ExtRateLen,    pAd->CommonCfg.ExtRate,
		                  END_OF_ARGS);
		FrameLen += TmpLen;
	}

#ifdef DOT11R_FT_SUPPORT
	if ((pFtCfg != NULL) && (pFtCfg->FtCapFlag.Dot11rFtEnable))
	{
		PUINT8	mdie_ptr;
		UINT8	mdie_len;
		PUINT8	ftie_ptr = NULL;
		UINT8	ftie_len = 0;
		PUINT8  ricie_ptr = NULL;
		UINT8   ricie_len = 0;
				
		/* Insert RSNIE if necessary */
		if (FtInfoBuf.RSNIE_Len != 0)
		{ 
	        ULONG TmpLen;
	        MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen, 
							  FtInfoBuf.RSNIE_Len,		FtInfoBuf.RSN_IE,
	                          END_OF_ARGS);
	        FrameLen += TmpLen;
	    }	

		/* Insert MDIE. */
		mdie_ptr = pOutBuffer+FrameLen;
		mdie_len = 5;
		FT_InsertMdIE(pAd, pOutBuffer+FrameLen, &FrameLen,
				FtInfoBuf.MdIeInfo.MdId, FtInfoBuf.MdIeInfo.FtCapPlc);

		/* Insert FTIE. */
		if (FtInfoBuf.FtIeInfo.Len != 0)
		{
			ftie_ptr = pOutBuffer+FrameLen;
			ftie_len = (2 + FtInfoBuf.FtIeInfo.Len);
			FT_InsertFTIE(pAd, pOutBuffer+FrameLen, &FrameLen,
				FtInfoBuf.FtIeInfo.Len, FtInfoBuf.FtIeInfo.MICCtr,
				FtInfoBuf.FtIeInfo.MIC, FtInfoBuf.FtIeInfo.ANonce,
				FtInfoBuf.FtIeInfo.SNonce);
		}
		/* Insert R1KH IE into FTIE. */
		if (FtInfoBuf.FtIeInfo.R1khIdLen!= 0)
			FT_FTIE_InsertKhIdSubIE(pAd, pOutBuffer+FrameLen, &FrameLen,
					FT_R1KH_ID, FtInfoBuf.FtIeInfo.R1khId,
					FtInfoBuf.FtIeInfo.R1khIdLen);

		/* Insert GTK Key info into FTIE. */
		if (FtInfoBuf.FtIeInfo.GtkLen!= 0)
	 		FT_FTIE_InsertGTKSubIE(pAd, pOutBuffer+FrameLen, &FrameLen,
	 			FtInfoBuf.FtIeInfo.GtkSubIE, FtInfoBuf.FtIeInfo.GtkLen);

		/* Insert R0KH IE into FTIE. */
		if (FtInfoBuf.FtIeInfo.R0khIdLen!= 0)
			FT_FTIE_InsertKhIdSubIE(pAd, pOutBuffer+FrameLen, &FrameLen,
					FT_R0KH_ID, FtInfoBuf.FtIeInfo.R0khId,
					FtInfoBuf.FtIeInfo.R0khIdLen);

		/* Insert RIC. */
		if (ie_list->FtInfo.RicInfo.Len)
		{
			ULONG TempLen;

			FT_RIC_ResourceRequestHandle(pAd, pEntry,
						(PUCHAR)ie_list->FtInfo.RicInfo.pRicInfo,
						ie_list->FtInfo.RicInfo.Len,
						(PUCHAR)pOutBuffer+FrameLen,
						(PUINT32)&TempLen);
			ricie_ptr = (PUCHAR)(pOutBuffer+FrameLen);
			ricie_len = TempLen;
			FrameLen += TempLen;
		}

		/* Calculate the FT MIC for FT procedure */
		if (FtInfoBuf.FtIeInfo.MICCtr.field.IECnt)
		{
			UINT8	ft_mic[FT_MIC_LEN];
			PFT_FTIE	pFtIe;
		
			FT_CalculateMIC(pEntry->Addr, 
							wdev->Bssid, 
							pEntry->PTK, 
							6, 
							FtInfoBuf.RSN_IE, 
							FtInfoBuf.RSNIE_Len, 
							mdie_ptr, 
							mdie_len, 
							ftie_ptr, 
							ftie_len,
							ricie_ptr,
							ricie_len,
							ft_mic);

			/* Update the MIC field of FTIE */
			pFtIe = (PFT_FTIE)(ftie_ptr + 2);
			NdisMoveMemory(pFtIe->MIC, ft_mic, FT_MIC_LEN);

			/* Install pairwise key */
			WPAInstallPairwiseKey(pAd, pEntry->func_tb_idx, pEntry, TRUE);

			/* Update status and set Port as Secured */
			pEntry->WpaState = AS_PTKINITDONE;
			pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
			tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;
		}

		/* 	Record the MDIE & FTIE of (re)association response of
			Initial Mobility Domain Association. It's used in 
			FT 4-Way handshaking */
		if (pEntry->AuthMode >= Ndis802_11AuthModeWPA2 &&
			ie_list->FtInfo.FtIeInfo.Len == 0)
		{
			NdisMoveMemory(&pEntry->InitialMDIE, mdie_ptr, mdie_len);

			pEntry->InitialFTIE_Len = ftie_len;
			NdisMoveMemory(pEntry->InitialFTIE, ftie_ptr, ftie_len);					
		}
	}
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
	if (IS_RRM_ENABLE(pAd, pEntry->func_tb_idx))
		RRM_InsertRRMEnCapIE(pAd, pOutBuffer+FrameLen, &FrameLen, pEntry->func_tb_idx);
#endif /* DOT11K_RRM_SUPPORT */

	/* add WMM IE here */
	if (wdev->bWmmCapable && CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE))
	{
		ULONG TmpLen;
		UCHAR WmeParmIe[26] = {IE_VENDOR_SPECIFIC, 24, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x01, 0, 0};

		WmeParmIe[8] = pAd->ApCfg.BssEdcaParm.EdcaUpdateCount & 0x0f;
#ifdef UAPSD_SUPPORT
		UAPSD_MR_IE_FILL(WmeParmIe[8], &pMbss->wdev.UapsdInfo);
#endif /* UAPSD_SUPPORT */
		for (i = QID_AC_BE; i <= QID_AC_VO; i++)
		{
			WmeParmIe[10+ (i*4)] = (i << 5) +     /* b5-6 is ACI */
								((UCHAR)pAd->ApCfg.BssEdcaParm.bACM[i] << 4) + /* b4 is ACM */
								(pAd->ApCfg.BssEdcaParm.Aifsn[i] & 0x0f);              /* b0-3 is AIFSN */
			WmeParmIe[11+ (i*4)] = (pAd->ApCfg.BssEdcaParm.Cwmax[i] << 4) + /* b5-8 is CWMAX */
								(pAd->ApCfg.BssEdcaParm.Cwmin[i] & 0x0f);              /* b0-3 is CWMIN */
			WmeParmIe[12+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] & 0xff);        /* low byte of TXOP */
			WmeParmIe[13+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] >> 8);          /* high byte of TXOP */
		}

		MakeOutgoingFrame(pOutBuffer+FrameLen, &TmpLen,
							26, WmeParmIe,
							END_OF_ARGS);
		FrameLen += TmpLen;
	}

#ifdef DOT11W_PMF_SUPPORT
        if (StatusCode == MLME_ASSOC_REJ_TEMPORARILY) {
		ULONG TmpLen;
		UCHAR IEType = IE_TIMEOUT_INTERVAL; //IE:0x15
		UCHAR IELen = 5;
		UCHAR TIType = 3;
		UINT32 units = 1 << 10; //1 seconds, should be 0x3E8
		MakeOutgoingFrame(pOutBuffer+FrameLen, &TmpLen,
                                1, &IEType,
                                1, &IELen,
                                1, &TIType,
                                4, &units,
                                END_OF_ARGS);
                FrameLen += TmpLen;
	}
#endif /* DOT11W_PMF_SUPPORT */

#ifdef DOT11_N_SUPPORT
	/* HT capability in AssocRsp frame. */
	if ((ie_list->ht_cap_len > 0) && WMODE_CAP_N(pAd->CommonCfg.PhyMode))
	{
		ULONG TmpLen;
		UCHAR HtLen1 = sizeof(pAd->CommonCfg.AddHTInfo);
		HT_CAPABILITY_IE HtCapabilityRsp;
#ifdef RT_BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
		ADD_HT_INFO_IE	addHTInfoTmp;
#endif

		NdisMoveMemory(&HtCapabilityRsp, &pAd->CommonCfg.HtCapability, ie_list->ht_cap_len);

		/* add HT Capability IE */
#ifndef RT_BIG_ENDIAN
		MakeOutgoingFrame(pOutBuffer+FrameLen,			&TmpLen,
											1,			&HtCapIe,
											1,			&ie_list->ht_cap_len,
							ie_list->ht_cap_len,		&HtCapabilityRsp,
											1,			&AddHtInfoIe,
											1,			&HtLen1,
										HtLen1,			&pAd->CommonCfg.AddHTInfo,
						  END_OF_ARGS);
#else
		NdisMoveMemory(&HtCapabilityTmp, &HtCapabilityRsp, ie_list->ht_cap_len);
		*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
#ifdef UNALIGNMENT_SUPPORT
		{
			EXT_HT_CAP_INFO extHtCapInfo;

			NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
			*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
			NdisMoveMemory((PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
		}
#else
		*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

		NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, HtLen1);
		*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
		*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

		MakeOutgoingFrame(pOutBuffer + FrameLen,         &TmpLen,
							1,                           &HtCapIe,
							1,                           &ie_list->ht_cap_len,
							ie_list->ht_cap_len,            &HtCapabilityTmp,
							1,                           &AddHtInfoIe,
							1,                           &HtLen1,
							HtLen1,                      &addHTInfoTmp,
							END_OF_ARGS);
#endif
		FrameLen += TmpLen;

		if ((ie_list->RalinkIe) == 0 || (pAd->bBroadComHT == TRUE))
		{
			UCHAR epigram_ie_len;
			UCHAR BROADCOM_HTC[4] = {0x0, 0x90, 0x4c, 0x33};
			UCHAR BROADCOM_AHTINFO[4] = {0x0, 0x90, 0x4c, 0x34};


			epigram_ie_len = ie_list->ht_cap_len + 4;
#ifndef RT_BIG_ENDIAN
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_HTC[0],
						  ie_list->ht_cap_len,            		&HtCapabilityRsp,
						  END_OF_ARGS);
#else
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_HTC[0],
						  ie_list->ht_cap_len,            		&HtCapabilityTmp,
						  END_OF_ARGS);
#endif

			FrameLen += TmpLen;
			epigram_ie_len = HtLen1 + 4;
#ifndef RT_BIG_ENDIAN
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_AHTINFO[0],
						  HtLen1, 							&pAd->CommonCfg.AddHTInfo,
						  END_OF_ARGS);
#else
			MakeOutgoingFrame(pOutBuffer + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_AHTINFO[0],
						  HtLen1, 							&addHTInfoTmp,
						  END_OF_ARGS);
#endif
			FrameLen += TmpLen;
		}

#ifdef DOT11N_DRAFT3
	 	/* P802.11n_D3.03, 7.3.2.60 Overlapping BSS Scan Parameters IE */
	 	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode) &&
			(pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == 1))
	 	{
			OVERLAP_BSS_SCAN_IE OverlapScanParam;
			ULONG TmpLen;
			UCHAR OverlapScanIE, ScanIELen;

			OverlapScanIE = IE_OVERLAPBSS_SCAN_PARM;
			ScanIELen = 14;
			OverlapScanParam.ScanPassiveDwell = cpu2le16(pAd->CommonCfg.Dot11OBssScanPassiveDwell);
			OverlapScanParam.ScanActiveDwell = cpu2le16(pAd->CommonCfg.Dot11OBssScanActiveDwell);
			OverlapScanParam.TriggerScanInt = cpu2le16(pAd->CommonCfg.Dot11BssWidthTriggerScanInt);
			OverlapScanParam.PassiveTalPerChannel = cpu2le16(pAd->CommonCfg.Dot11OBssScanPassiveTotalPerChannel);
			OverlapScanParam.ActiveTalPerChannel = cpu2le16(pAd->CommonCfg.Dot11OBssScanActiveTotalPerChannel);
			OverlapScanParam.DelayFactor = cpu2le16(pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor);
			OverlapScanParam.ScanActThre = cpu2le16(pAd->CommonCfg.Dot11OBssScanActivityThre);
			
			MakeOutgoingFrame(pOutBuffer + FrameLen, &TmpLen,
								1,			&OverlapScanIE,
								1,			&ScanIELen,
								ScanIELen,	&OverlapScanParam,
								END_OF_ARGS);
			
			FrameLen += TmpLen;
	 	}
#endif /* DOT11N_DRAFT3 */

#ifdef DOT11_VHT_AC
		if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
			(pAd->CommonCfg.Channel > 14) &&
			(ie_list->vht_cap_len))
		{
			FrameLen += build_vht_ies(pAd, pOutBuffer + FrameLen, SUBTYPE_ASSOC_RSP);
		}
#endif /* DOT11_VHT_AC */
	}
#endif /* DOT11_N_SUPPORT */

#ifdef CONFIG_HOTSPOT_R2
	/* qosmap IE */
	DBGPRINT(RT_DEBUG_OFF, ("entry=%d\n", pEntry->QosMapSupport));
	if (pEntry->QosMapSupport)
	{
		ULONG	TmpLen;
		UCHAR	QosMapIE, ielen = 0, i = 0, explen = 0;
		PHOTSPOT_CTRL pHSCtrl =  &pAd->ApCfg.MBSSID[pEntry->apidx].HotSpotCtrl;

		if (pHSCtrl->QosMapEnable)
		{
			QosMapIE = IE_QOS_MAP_SET;
		
			for (i=0;i<21;i++)
 			{
 				if (pHSCtrl->DscpException[i] == 0xffff)
		 			break;
 				else
 					explen += 2;
 			}
 	
 			pEntry->DscpExceptionCount = explen;
 			memcpy((UCHAR *)pEntry->DscpRange, (UCHAR *)pHSCtrl->DscpRange, 16);
 			memcpy((UCHAR *)pEntry->DscpException, (UCHAR *)pHSCtrl->DscpException, 42);
 		
 			ielen = explen+16;
		
			MakeOutgoingFrame(pOutBuffer+ FrameLen, &TmpLen,
					1,			&QosMapIE,
					1,			&ielen,
					explen,		pEntry->DscpException,
					16,			pEntry->DscpRange,
					END_OF_ARGS);
					
			FrameLen += TmpLen;			
		}
	}
#endif /* CONFIG_HOTSPOT_R2 */

		/* 7.3.2.27 Extended Capabilities IE */
		{
		ULONG TmpLen, infoPos;
		PUCHAR pInfo;
		UCHAR extInfoLen;
		BOOLEAN bNeedAppendExtIE = FALSE;
		EXT_CAP_INFO_ELEMENT extCapInfo;

		
		extInfoLen = sizeof(EXT_CAP_INFO_ELEMENT);
		NdisZeroMemory(&extCapInfo, extInfoLen);

#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
		/* P802.11n_D1.10, HT Information Exchange Support */
		if (WMODE_CAP_N(pAd->CommonCfg.PhyMode)
			&& (pAd->CommonCfg.Channel <= 14) 
			&& (pAd->CommonCfg.bBssCoexEnable == TRUE)
		)
		{
			extCapInfo.BssCoexistMgmtSupport = 1;
		}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
		if (IS_BSS_TRANSIT_MANMT_SUPPORT(pAd, pEntry->func_tb_idx))
		{
			if( ie_list->ExtCapInfo.BssTransitionManmt == 1)
			{
				extCapInfo.BssTransitionManmt = 1;
				pEntry->bBSSMantSTASupport = TRUE;
			}			
		}
		if (IS_WNMDMS_SUPPORT(pAd, pEntry->func_tb_idx))
		{
			if ( ie_list->ExtCapInfo.DMSSupport == 1)
			{
				extCapInfo.DMSSupport = 1;
				pEntry->bDMSSTASupport = TRUE;
			}
		}
#endif /* DOT11V_WNM_SUPPORT */

#ifdef DOT11_VHT_AC
		if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
			(pAd->CommonCfg.Channel > 14))
			extCapInfo.operating_mode_notification = 1;
#endif /* DOT11_VHT_AC */

		pInfo = (UCHAR *)(&extCapInfo);
		for (infoPos = 0; infoPos < extInfoLen; infoPos++)
		{
			if (pInfo[infoPos] != 0)
			{
				bNeedAppendExtIE = TRUE;
				break;
			}
		}

		if (bNeedAppendExtIE == TRUE)
		{
			MakeOutgoingFrame(pOutBuffer+FrameLen, &TmpLen,
							1,			&ExtCapIe,
							1,			&extInfoLen,
							extInfoLen,	&extCapInfo,
							  END_OF_ARGS);
			FrameLen += TmpLen;
		}
	}
	
	/* add Ralink-specific IE here - Byte0.b0=1 for aggregation, Byte0.b1=1 for piggy-back */
{
	ULONG TmpLen;
	UCHAR RalinkSpecificIe[9] = {IE_VENDOR_SPECIFIC, 7, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00};

	if (pAd->CommonCfg.bAggregationCapable)
		RalinkSpecificIe[5] |= 0x1;
	if (pAd->CommonCfg.bPiggyBackCapable)
		RalinkSpecificIe[5] |= 0x2;
#ifdef DOT11_N_SUPPORT
	if (pAd->CommonCfg.bRdg)
		RalinkSpecificIe[5] |= 0x4;
#endif /* DOT11_N_SUPPORT */
	MakeOutgoingFrame(pOutBuffer+FrameLen, &TmpLen,
						9, RalinkSpecificIe,
						END_OF_ARGS);
	FrameLen += TmpLen;

#ifdef RT3883
	if (IS_RT3883(pAd))
		FrameLen += RT3883_ext_pkt_len(pOutBuffer, FrameLen, RalinkSpecificIe, 9);
#endif // RT3883 //
}
#ifdef RT_CFG80211_SUPPORT
    /* Append extra IEs provided by wpa_supplicant */
	if (pAd->ApCfg.MBSSID[pEntry->apidx].AssocRespExtraIeLen)
    {
        ULONG TmpLen = 0;
	INT32 IesLen = pAd->ApCfg.MBSSID[pEntry->apidx].AssocRespExtraIeLen;
	UCHAR *Ies = pAd->ApCfg.MBSSID[pEntry->apidx].AssocRespExtraIe;

        //if (RTMPIsValidIEs(Ies, IesLen))
        {
            DBGPRINT(RT_DEBUG_TRACE, ("AssocRespExtraIE Added (Len=%d)\n", IesLen));
            MakeOutgoingFrame(pOutBuffer+FrameLen, 
                              &TmpLen, 
                              IesLen, 
                              Ies, 
                              END_OF_ARGS);
            FrameLen += TmpLen;
        }
        //else
        //    DBGPRINT(RT_DEBUG_ERROR, ("AssocRespExtraIE len incorrect!\n"));
    }
#endif /* RT_CFG80211_SUPPORT */
  
#ifdef EASY_CONFIG_SETUP
	if (pEntry->bRaAutoWpsCapable)
		AutoProvisionBuildAssocRspIE(pAd, Elem, pEntry, pOutBuffer, &FrameLen);
#endif /* EASY_CONFIG_SETUP */
#ifdef WSC_AP_SUPPORT
	if (pEntry->bWscCapable)
	{
		UCHAR *pWscBuf = NULL, WscIeLen = 0;
		ULONG WscTmpLen = 0;

		os_alloc_mem(NULL, (UCHAR **)&pWscBuf, 512);
		if(pWscBuf)
		{
			NdisZeroMemory(pWscBuf, 512);
			WscBuildAssocRespIE(pAd, pEntry->func_tb_idx, 0, pWscBuf, &WscIeLen);
			MakeOutgoingFrame(pOutBuffer + FrameLen, &WscTmpLen,
								  WscIeLen, pWscBuf,
								  END_OF_ARGS);

			FrameLen += WscTmpLen;
			os_free_mem(NULL, pWscBuf);
		}
	}
#endif /* WSC_AP_SUPPORT */
  
#ifdef P2P_SUPPORT
	if (ie_list->P2PSubelementLen > 0)
	{
		ULONG 	TmpLen;
		UCHAR 	P2pIdx = P2P_NOT_FOUND;
		UCHAR	GroupCap = 0xff, DeviceCap = 0xff, DevAddr[6] = {0}, DeviceType[8], DeviceName[32], DeviceNameLen = 0;
		PUCHAR 	pData;
		USHORT	Dpid, ConfigMethod;

		pEntry->bP2pClient = TRUE;
		pEntry->P2pInfo.P2pClientState = P2PSTATE_CLIENT_ASSOC;
		P2pParseSubElmt(pAd, (PVOID)ie_list->P2pSubelement, ie_list->P2PSubelementLen, FALSE, &Dpid, &GroupCap, 
			&DeviceCap, DeviceName, &DeviceNameLen, DevAddr, NULL, NULL, NULL, NULL, &ConfigMethod, 
			&ConfigMethod, DeviceType, NULL, NULL, NULL, NULL, &StatusCode, NULL,
#ifdef WFD_SUPPORT
			NULL, NULL,
#endif /* WFD_SUPPORT */
			NULL);

		P2pIdx = P2pGroupTabSearch(pAd, DevAddr);
		if (P2pIdx == P2P_NOT_FOUND)
			P2pIdx = P2pGroupTabInsert(pAd, DevAddr, P2PSTATE_DISCOVERY_CLIENT, NULL, 0, 0, 0);

		if (P2pIdx != P2P_NOT_FOUND)
		{
			pEntry->P2pInfo.p2pIndex = P2pIdx;
		DBGPRINT(RT_DEBUG_TRACE, ("ASSOC RSP - Insert P2P IE to %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pEntry->Addr)));
		DBGPRINT(RT_DEBUG_TRACE, (" %d. DevAddr = %02x:%02x:%02x:%02x:%02x:%02x\n", P2pIdx, PRINT_MAC(DevAddr)));
		DBGPRINT(RT_DEBUG_TRACE, ("DeviceNameLen = %d, DeviceName = %c %c %c %c %c %c %c %c\n", DeviceNameLen , 
			DeviceName[0], DeviceName[1], DeviceName[2], DeviceName[3],
			DeviceName[4], DeviceName[5], DeviceName[6], DeviceName[7]));

		/* update P2P Interface Address */
		RTMPMoveMemory(pAd->P2pTable.Client[P2pIdx].InterfaceAddr, pEntry->Addr, MAC_ADDR_LEN);

		pData = pOutBuffer + FrameLen;
		P2pMakeP2pIE(pAd, SUBTYPE_ASSOC_RSP, pData, &TmpLen);
		FrameLen += TmpLen;
	}
	}
	else
		pEntry->bP2pClient = FALSE;

#ifdef WFD_SUPPORT
	{
		PUCHAR	pData;
		ULONG	WfdIeLen = 0;

		pData = pOutBuffer + FrameLen;
		WfdMakeWfdIE(pAd, SUBTYPE_ASSOC_RSP, pData, &WfdIeLen);
		FrameLen += WfdIeLen;
	}
#endif /* WFD_SUPPORT */

#ifdef WFA_WFD_SUPPORT
	if (pAd->P2pCfg.bWIDI)
	{
		if (pAd->pWfdIeInAssocRsp && (pAd->WfdIeInAssocRspLen != 0))
		{
			ULONG WfdIeTmpLen = 0;

			MakeOutgoingFrame(pOutBuffer+FrameLen,	&WfdIeTmpLen,
				  pAd->WfdIeInAssocRspLen,	pAd->pWfdIeInAssocRsp,
				  END_OF_ARGS);
			FrameLen += WfdIeTmpLen;
		}
	}
#endif /* WFA_WFD_SUPPORT */
#endif /* P2P_SUPPORT */
	MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, (PVOID) pOutBuffer);

#ifdef DOT11W_PMF_SUPPORT
	if (StatusCode == MLME_ASSOC_REJ_TEMPORARILY)
	{
		PMF_MlmeSAQueryReq(pAd, pEntry);
	}
#endif /* DOT11W_PMF_SUPPORT */

	/* set up BA session */
	if (StatusCode == MLME_SUCCESS)
	{
#ifdef RELEASE_EXCLUDE
		/*
			For PM bit for management frame, in IEEE802.11 spec.,
			11.2.1.1 STA Power Management modes, it says that.

			"The Power Managment bit shall not be set in any management frame,
			except an Action frame."

			Because PM bit shall not be set for management frame, it means that
			when station sends the assoc or reassociation frame with PM = 0,
			the station wakes up.

			So when STA connected to AP and enter power save, then assoc or
			reassoc need to clear power save mode.
		*/
#endif /* RELEASE_EXCLUDE */
		pEntry->PsMode = PWR_ACTIVE;
		// TODO: shiang-usw, we need to rmove upper setting and migrate to tr_entry->PsMode
		pAd->MacTab.tr_entry[pEntry->wcid].PsMode = PWR_ACTIVE;

		wdev->allow_data_tx = TRUE;
		
#ifdef IAPP_SUPPORT
		/*PFRAME_802_11 Fr = (PFRAME_802_11)Elem->Msg; */
/*		POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie; */

		/* send association ok message to IAPPD */
		IAPP_L2_Update_Frame_Send(pAd, pEntry->Addr, pEntry->wdev->wdev_idx);
		DBGPRINT(RT_DEBUG_TRACE, ("####### Send L2 Frame Mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
								  PRINT_MAC(pEntry->Addr)));

/*		SendSingalToDaemon(SIGUSR2, pObj->IappPid); */

#ifdef DOT11R_FT_SUPPORT		
		/*
			Do not do any check here.
			We need to send MOVE-Req frame to AP1 even open mode.
		*/
/*		if (IS_FT_RSN_STA(pEntry) && (FtInfo.FtIeInfo.Len != 0)) */
		if (isReassoc == 1)
		{					
			/* only for reassociation frame */
			FT_KDP_EVT_REASSOC EvtReAssoc;

			EvtReAssoc.SeqNum = 0;
			NdisMoveMemory(EvtReAssoc.MacAddr, pEntry->Addr, MAC_ADDR_LEN);
			NdisMoveMemory(EvtReAssoc.OldApMacAddr, ie_list->ApAddr, MAC_ADDR_LEN);

			FT_KDP_EVENT_INFORM(pAd, pEntry->func_tb_idx, FT_KDP_SIG_FT_REASSOCIATION,
								&EvtReAssoc, sizeof(EvtReAssoc), NULL);
		}
#endif /* DOT11R_FT_SUPPORT */
		
#endif /* IAPP_SUPPORT */

		ap_assoc_info_debugshow(pAd, isReassoc, pEntry, ie_list);

		/* send wireless event - for association */
		RTMPSendWirelessEvent(pAd, IW_ASSOC_EVENT_FLAG, pEntry->Addr, 0, 0);
    	
		/* This is a reassociation procedure */
		pEntry->IsReassocSta = isReassoc;
		
#ifdef DOT11_N_SUPPORT
		/* clear txBA bitmap */
		pEntry->TXBAbitmap = 0;
		if (pEntry->MaxHTPhyMode.field.MODE >= MODE_HTMIX)
		{
			CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE);
			if ((pAd->CommonCfg.Channel <=14) &&
				pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset &&
				(ie_list->HTCapability.HtCapInfo.ChannelWidth==BW_40))
			{
				SendBeaconRequest(pAd, pEntry->wcid);
			}
			/*BAOriSessionSetUp(pAd, pEntry, 0, 0, 3000, FALSE); */
		}
#endif /* DOT11_N_SUPPORT */

#ifdef WMM_ACM_SUPPORT
		if (isReassoc == 0)
		{
#ifdef RELEASE_EXCLUDE
			/* That association not be attempted by the STA if it is currently
				associated to an AP, ex:

				IEEE802.11 Annex C Mlme_request_2c(3) (informative):
				Associate request rejected as invalid while associated.

				So we dont need to care about that the station sends association
				request to the associated AP. Here, we still clear TSPEC. */

            /* Consider this scenario:  A STA loses contact with its associated
                AP while dynamic U-APSD settings are in place.  The STA is reset
                (perhaps by the user, or perhaps as part of the connection
                recovery).  It then Associates to the same AP, but it has no
                knowledge of the prior association with the AP.
                The STA is now going to be very confused if the U-APSD settings
                are still in place. */
#endif /* RELEASE_EXCLUDE */

			/* delete all TSPEC for association request */
			ACMP_StationDelete(pAd, pEntry);
		}
		else
		{
#ifdef RELEASE_EXCLUDE
			/* That reassociation not be attempted by the STA unless it is
				associated to an AP in the ESS, ex:

				IEEE802.11 7.3.1.9:
				Status code 11: Reassociation denied due to inability to confirm
				that association exists

				IEEE802.11 11.3:
				Reassociation frames shall only be sent if the sending STA is
				already associated in the same ESS.
			*/

			/* 	case 1: (11r or IAPP)
				. sta1 associates to AP1
				. sta1 builds TSPEC to AP1
				. AP1 actives TSPEC
				. sta1 builds TSPEC through AP1 to AP2
				. AP2 builds TSPEC
				. sta1 disassociates to AP1
				. AP1 clears TSPEC
				. sta1 reassociates to AP2
				. AP2 actives TSPEC

				case 2: (same AP)
				. sta1 associates to AP1
				. sta1 builds TSPEC to AP1
				. AP1 actives TSPEC
				. sta1 reassociates to AP1
			*/
#endif /* RELEASE_EXCLUDE */

			/* keep all TSPEC for reassociation request */

			/* static UAPSD state = UAPSD state of reassociation request */
			/* keep UAPSD state to UAPSD of TSPEC TS, if TSPEC TS is deleted,
				recover to the UAPSD state of reassociation request, not
				the UAPSD state of association request */

			ACMP_UAPSD_StateUpdate(pAd, pEntry);
		}
#endif /* WMM_ACM_SUPPORT */

#ifdef DOT11R_FT_SUPPORT
		/* 	If the length of FTIE field of the (re)association-request frame
			is larger than zero, it shall indicate the Fast-BSS transition is in progress. */
		if (ie_list->FtInfo.FtIeInfo.Len > 0)
		{
			;
		}
		else
#endif /* DOT11R_FT_SUPPORT */	
#ifdef RT_CFG80211_SUPPORT
		if (TRUE) /*CFG_TODO*/
        {
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
			PNET_DEV pNetDev = NULL;
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */

			hex_dump("ASSOC_REQ", Elem->Msg, Elem->MsgLen);

#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE			
			if ((pAd->cfg80211_ctrl.Cfg80211VifDevSet.vifDevList.size > 0) &&
			    ((pNetDev = RTMP_CFG80211_FindVifEntry_ByType(pAd, RT_CMD_80211_IFTYPE_P2P_GO)) != NULL))
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CONCURRENT_DEVICE CFG : GO NOITFY THE CLIENT ASSOCIATED\n"));
				CFG80211OS_NewSta(pNetDev, ie_list->Addr2, (PUCHAR)Elem->Msg, Elem->MsgLen);
			}
			else
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
			{
				DBGPRINT(RT_DEBUG_TRACE, ("SINGLE_DEVICE CFG : GO NOITFY THE CLIENT ASSOCIATED\n"));
				CFG80211OS_NewSta(pAd->net_dev, ie_list->Addr2, (PUCHAR)Elem->Msg, Elem->MsgLen);
				if (pEntry->WepStatus == Ndis802_11WEPEnabled)
				{
					/* Set WEP key to ASIC */
					UCHAR KeyIdx = 0;
					UCHAR CipherAlg = 0;

					KeyIdx = wdev->DefaultKeyId;					
					CipherAlg = pAd->SharedKey[pEntry->func_tb_idx][KeyIdx].CipherAlg;

					/*
						If WEP is used, set pair-wise cipherAlg into WCID
						attribute table for this entry.
					*/
					RTMP_SET_WCID_SEC_INFO(pAd, 
											pEntry->func_tb_idx, 
											KeyIdx, 
											CipherAlg, 
											pEntry->wcid, 
											SHAREDKEYTABLE);
#ifdef MT_MAC										
					if (pAd->chipCap.hif_type == HIF_MT)
						CmdProcAddRemoveKey(pAd, 0, pEntry->func_tb_idx, KeyIdx, pEntry->wcid, PAIRWISEKEYTABLE, &pAd->SharedKey[pEntry->func_tb_idx][KeyIdx], pEntry->Addr);
#endif										
				}						
			}
        }
		else
#endif	
		/* enqueue a EAPOL_START message to trigger EAP state machine doing the authentication */
	    if ((pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) || 
			(pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK))
    	{
#ifdef WSC_AP_SUPPORT
            /*
                In WPA-PSK mode,
                If Association Request of station has RSN/SSN, WPS AP Must Not send EAP-Request/Identity to station
                no matter WPS AP does receive EAPoL-Start from STA or not.
                Marvell WPS test bed(v2.1.1.5) will send AssocReq with WPS IE and RSN/SSN IE.
            */
            if (pEntry->bWscCapable || (ie_list->RSNIE_Len == 0))
            {
			DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - IF(ra%d) This is a WPS Client.\n\n", pEntry->func_tb_idx));
			goto LabelOK;
            }
            else
            {
                pEntry->bWscCapable = FALSE;
                pEntry->Receive_EapolStart_EapRspId = (WSC_ENTRY_GET_EAPOL_START | WSC_ENTRY_GET_EAP_RSP_ID);
                /* This STA is not a WPS STA */
                NdisZeroMemory(wsc_ctrl->EntryAddr, 6);
            }
#endif /* WSC_AP_SUPPORT */

			/* Enqueue a EAPOL-start message with the pEntry for WPAPSK State Machine */
			if ((pEntry->EnqueueEapolStartTimerRunning == EAPOL_START_DISABLE
#ifdef HOSTAPD_SUPPORT			
				&& wdev->Hostapd == Hostapd_Diable
#endif/*HOSTAPD_SUPPORT*/
				)
#ifdef WSC_AP_SUPPORT
				&& !pEntry->bWscCapable
#endif /* WSC_AP_SUPPORT */
				)
			{
        		/* Enqueue a EAPOL-start message with the pEntry */
        		pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_PSK;
        		RTMPSetTimer(&pEntry->EnqueueStartForPSKTimer, ENQUEUE_EAPOL_START_TIMER);
			}
    	}
#ifdef DOT1X_SUPPORT
		/*else if (isReassoc && */
		else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2) && 
				((pPmkid = WPA_ExtractSuiteFromRSNIE(ie_list->RSN_IE, ie_list->RSNIE_Len, PMKID_LIST, &pmkid_count)) != NULL))
		{	/* Key cache */
			INT	CacheIdx;

			if (((CacheIdx = RTMPSearchPMKIDCache(pAd, pEntry->func_tb_idx, pEntry->Addr)) != -1)
				&& (RTMPEqualMemory(pPmkid, &pMbss->PMKIDCache.BSSIDInfo[CacheIdx].PMKID, LEN_PMKID)))
			{
				/* Enqueue a EAPOL-start message with the pEntry for WPAPSK State Machine */
				if ((pEntry->EnqueueEapolStartTimerRunning == EAPOL_START_DISABLE
#ifdef HOSTAPD_SUPPORT
					&& wdev->Hostapd == Hostapd_Diable
#endif /*HOSTAPD_SUPPORT*/
				)
#ifdef WSC_AP_SUPPORT
					&& !pEntry->bWscCapable
#endif /* WSC_AP_SUPPORT */
				)
				{
					/* Enqueue a EAPOL-start message with the pEntry */
					pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_PSK;
					RTMPSetTimer(&pEntry->EnqueueStartForPSKTimer, ENQUEUE_EAPOL_START_TIMER);
				}

				pEntry->PMKID_CacheIdx = CacheIdx;
				DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - 2.PMKID matched and start key cache algorithm\n"));
			}
			else
			{
				pEntry->PMKID_CacheIdx = ENTRY_NOT_FOUND;
				DBGPRINT(RT_DEBUG_ERROR, ("ASSOC - 2.PMKID not found \n"));
			}
		}
		else if ((pEntry->AuthMode == Ndis802_11AuthModeWPA) ||
				 (pEntry->AuthMode == Ndis802_11AuthModeWPA2) ||
				 ((wdev->IEEE8021X)
#ifdef WSC_AP_SUPPORT
					&& (!pEntry->bWscCapable)
#endif /* WSC_AP_SUPPORT */
				)
		)
		{
			/* Enqueue a EAPOL-start message to trigger EAP SM */
			if (pEntry->EnqueueEapolStartTimerRunning == EAPOL_START_DISABLE
#ifdef HOSTAPD_SUPPORT
				&& wdev->Hostapd == Hostapd_Diable
#endif/*HOSTAPD_SUPPORT*/
			)
			{
      	  			pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_1X;
       	 		RTMPSetTimer(&pEntry->EnqueueStartForPSKTimer, ENQUEUE_EAPOL_START_TIMER);
			}
		}
#endif /* DOT1X_SUPPORT */
#ifdef WAPI_SUPPORT
		else if (pEntry->AuthMode == Ndis802_11AuthModeWAICERT)
		{
			WAPI_InternalCmdAction(pAd, 
								   pEntry->AuthMode, 
								   pEntry->func_tb_idx, 
								   pEntry->Addr, 
								   WAI_MLME_CERT_AUTH_START);	

			RTMPInitWapiRekeyTimerAction(pAd, pEntry);
		}
		else if (pEntry->AuthMode == Ndis802_11AuthModeWAIPSK)
		{
			WAPI_InternalCmdAction(pAd, 
								   pEntry->AuthMode, 
								   pEntry->func_tb_idx, 
								   pEntry->Addr, 
								   WAI_MLME_KEY_HS_START);
			RTMPInitWapiRekeyTimerAction(pAd, pEntry);
		}
#endif /* WAPI_SUPPORT */
        else
        {
			if (pEntry->WepStatus == Ndis802_11WEPEnabled)
			{
				/* Set WEP key to ASIC */
				UCHAR KeyIdx = 0;
				UCHAR CipherAlg = 0;

				KeyIdx = wdev->DefaultKeyId;					
				CipherAlg = pAd->SharedKey[pEntry->func_tb_idx][KeyIdx].CipherAlg;

				/*
					If WEP is used, set pair-wise cipherAlg into WCID
					attribute table for this entry.
				*/
				RTMP_SET_WCID_SEC_INFO(pAd, 
										pEntry->func_tb_idx, 
										KeyIdx, 
										CipherAlg, 
										pEntry->wcid, 
										SHAREDKEYTABLE);
#ifdef MT_MAC										
				if (pAd->chipCap.hif_type == HIF_MT)
					CmdProcAddRemoveKey(pAd, 0, pEntry->func_tb_idx, KeyIdx, pEntry->wcid, PAIRWISEKEYTABLE, &pAd->SharedKey[pEntry->func_tb_idx][KeyIdx], pEntry->Addr);
#endif										
			}	
        }

#ifdef SMART_ANTENNA
		{
			unsigned long irqflags;
			// Check if need to reset the sa training procedures to init stage!
			RTMP_IRQ_LOCK(&pAd->smartAntLock, irqflags);
			if (RTMP_SA_WORK_ON(pAd))
			{
				//sa_add_train_entry(pAd, &pEntry->Addr[0], FALSE);
				pAd->pSAParam->bStaChange = TRUE;
				DBGPRINT(RT_DEBUG_TRACE, 
							("%s():sta(%02x:%02x:%02x:%02x:%02x:%02x) add!\n", 
							__FUNCTION__, PRINT_MAC(pEntry->Addr)));
			}
			RTMP_IRQ_UNLOCK(&pAd->smartAntLock, irqflags);
		}
#endif // SMART_ANTENNA //
	}

LabelOK:
#ifdef RT_CFG80211_P2P_SUPPORT 
	if (StatusCode != MLME_SUCCESS)
		CFG80211_ApStaDelSendEvent(pAd, pEntry->Addr);
#endif /* RT_CFG80211_P2P_SUPPORT */
	
	if (ie_list != NULL)
		os_free_mem(NULL, ie_list);

	return;	
}



/*
    ==========================================================================
    Description:
        peer assoc req handling procedure
    Parameters:
        Adapter - Adapter pointer
        Elem - MLME Queue Element
    Pre:
        the station has been authenticated and the following information is stored
    Post  :
        -# An association response frame is generated and sent to the air
    ==========================================================================
 */
VOID APPeerAssocReqAction(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	ap_cmm_peer_assoc_req_action(pAd, Elem, 0);
}

/*
    ==========================================================================
    Description:
        mlme reassoc req handling procedure
    Parameters:
        Elem -
    Pre:
        -# SSID  (Adapter->ApCfg.ssid[])
        -# BSSID (AP address, Adapter->ApCfg.bssid)
        -# Supported rates (Adapter->ApCfg.supported_rates[])
        -# Supported rates length (Adapter->ApCfg.supported_rates_len)
        -# Tx power (Adapter->ApCfg.tx_power)
    ==========================================================================
 */
VOID APPeerReassocReqAction(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	ap_cmm_peer_assoc_req_action(pAd, Elem, 1);
}


/* 
    ==========================================================================
    Description:
        MLME message sanity check
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
 */
BOOLEAN PeerDisassocReqSanity(
    IN PRTMP_ADAPTER pAd, 
    IN VOID *Msg, 
    IN ULONG MsgLen, 
    OUT PUCHAR pAddr2, 
    OUT	UINT16	*SeqNum,
    OUT USHORT *Reason) 
{
    PFRAME_802_11 Fr = (PFRAME_802_11)Msg;

    COPY_MAC_ADDR(pAddr2, &Fr->Hdr.Addr2);
	*SeqNum = Fr->Hdr.Sequence;
    NdisMoveMemory(Reason, &Fr->Octet[0], 2);

    return TRUE;
}


/*
    ==========================================================================
    Description:
        left part of IEEE 802.11/1999 p.374
    Parameters:
        Elem - MLME message containing the received frame
    ==========================================================================
 */
VOID APPeerDisassocReqAction(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	UCHAR Addr2[MAC_ADDR_LEN];
	USHORT Reason;
	UINT16 SeqNum;		
	MAC_TABLE_ENTRY *pEntry;
	
#ifdef DOT1X_SUPPORT	
	struct wifi_dev *wdev;
#endif /* DOT1X_SUPPORT */

	DBGPRINT(RT_DEBUG_OFF, ("ASSOC - 1 receive DIS-ASSOC request \n"));
	if (! PeerDisassocReqSanity(pAd, Elem->Msg, Elem->MsgLen, Addr2, &SeqNum, &Reason))
		return;

	DBGPRINT(RT_DEBUG_OFF, ("ASSOC - receive DIS-ASSOC(seq-%d) request from %02x:%02x:%02x:%02x:%02x:%02x, reason=%d\n", 
				SeqNum, PRINT_MAC(Addr2), Reason));
    
	pEntry = MacTableLookup(pAd, Addr2);
	if (pEntry == NULL)
		return;

	if (pEntry->wcid < MAX_LEN_OF_MAC_TABLE)
    {
#ifdef DPA_S
#ifdef P2P_SUPPORT
		UCHAR p2pIndex = P2P_NOT_FOUND;
#endif /* P2P_SUPPORT */
#endif /* DPA_S */

#ifdef DOT1X_SUPPORT
		wdev = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev;
		/* Notify 802.1x daemon to clear this sta info */
		if (pEntry->AuthMode == Ndis802_11AuthModeWPA || 
			pEntry->AuthMode == Ndis802_11AuthModeWPA2 ||
			wdev->IEEE8021X)
			DOT1X_InternalCmdAction(pAd, pEntry, DOT1X_DISCONNECT_ENTRY);
#endif /* DOT1X_SUPPORT */
	
#ifdef WAPI_SUPPORT
		WAPI_InternalCmdAction(pAd, 
							   pEntry->AuthMode, 
							   pEntry->func_tb_idx, 
							   pEntry->Addr, 
							   WAI_MLME_DISCONNECT);		
#endif /* WAPI_SUPPORT */
	
		/* send wireless event - for disassociation */
		RTMPSendWirelessEvent(pAd, IW_DISASSOC_EVENT_FLAG, Addr2, 0, 0);
        ApLogEvent(pAd, Addr2, EVENT_DISASSOCIATED);

#ifdef DPA_S
#ifdef P2P_SUPPORT
		p2pIndex = P2pGroupTabSearch(pAd, Addr2);
		if (p2pIndex != P2P_NOT_FOUND)
		{
			PRT_P2P_CLIENT_ENTRY pP2pEntry = &pAd->P2pTable.Client[p2pIndex];
			P2pSendSamsungEvent(pAd, P2P_NOTIF_SOFTAP_STA_DISASSOC, Addr2, pP2pEntry->DeviceName, pP2pEntry->DeviceNameLen);
		}
		else
			P2pSendSamsungEvent(pAd, P2P_NOTIF_SOFTAP_STA_DISASSOC, Addr2, NULL, 0);
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
		MacTableDeleteEntry(pAd, pEntry->wcid, Addr2);

#ifdef MAC_REPEATER_SUPPORT
		if (pAd->ApCfg.bMACRepeaterEn == TRUE)
		{
			UCHAR apCliIdx, CliIdx;
			REPEATER_CLIENT_ENTRY *pReptEntry = NULL;

			pReptEntry = RTMPLookupRepeaterCliEntry(pAd, TRUE, Addr2);
			if (pReptEntry && (pReptEntry->CliConnectState != 0))
			{
				apCliIdx = pReptEntry->MatchApCliIdx;
				CliIdx = pReptEntry->MatchLinkIdx;
				MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_DISCONNECT_REQ, 0, NULL,
								(64 + MAX_EXT_MAC_ADDR_SIZE*apCliIdx + CliIdx));
						RTMP_MLME_HANDLER(pAd);
						RTMPRemoveRepeaterEntry(pAd, apCliIdx, CliIdx);
			}
		}
#endif /* MAC_REPEATER_SUPPORT */
    }
}


/*
    ==========================================================================
    Description:
        delete it from STA and disassoc s STA
    Parameters:
        Elem -
    ==========================================================================
 */
VOID MbssKickOutStas(RTMP_ADAPTER *pAd, INT apidx, USHORT Reason)
{
	INT i;
	PMAC_TABLE_ENTRY pEntry;

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &pAd->MacTab.Content[i];
		if (pEntry && IS_ENTRY_CLIENT(pEntry) && pEntry->func_tb_idx == apidx)
			APMlmeKickOutSta(pAd, pEntry->Addr, pEntry->wcid, Reason);
	}

	return;
}


/*
    ==========================================================================
    Description:
        delete it from STA and disassoc s STA
    Parameters:
        Elem -
    ==========================================================================
 */
VOID APMlmeKickOutSta(RTMP_ADAPTER *pAd, UCHAR *pStaAddr, UCHAR Wcid, USHORT Reason)
{
	HEADER_802_11 DisassocHdr;
	PUCHAR pOutBuffer = NULL;
	ULONG FrameLen = 0;
	NDIS_STATUS NStatus;
	MAC_TABLE_ENTRY *pEntry;
	UCHAR Aid;
	UCHAR ApIdx;

	pEntry = MacTableLookup(pAd, pStaAddr);

	if (pEntry == NULL)
	{
		return;
	}
	Aid = (UCHAR)pEntry->Aid;
	ApIdx = pEntry->func_tb_idx;

	ASSERT(Aid == Wcid);

	if (ApIdx >= pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid Apidx=%d\n",
			__FUNCTION__, ApIdx));
		return;
	}

	if (Aid < MAX_LEN_OF_MAC_TABLE)
	{
		/* send wireless event - for disassocation */
		RTMPSendWirelessEvent(pAd, IW_DISASSOC_EVENT_FLAG, pStaAddr, 0, 0);
        ApLogEvent(pAd, pStaAddr, EVENT_DISASSOCIATED);

	    /* 2. send out a DISASSOC request frame */
	    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
	    if (NStatus != NDIS_STATUS_SUCCESS)
	        return;
	    
	    DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - MLME disassociates %02x:%02x:%02x:%02x:%02x:%02x; Send DISASSOC request\n",
	        		PRINT_MAC(pStaAddr)));
	    MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pStaAddr, 
							pAd->ApCfg.MBSSID[ApIdx].wdev.if_addr,
							pAd->ApCfg.MBSSID[ApIdx].wdev.bssid);
	    MakeOutgoingFrame(pOutBuffer,            &FrameLen,
	                      sizeof(HEADER_802_11), &DisassocHdr,
	                      2,                     &Reason,
	                      END_OF_ARGS);
	    MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	    MlmeFreeMemory(pAd, pOutBuffer);
		MacTableDeleteEntry(pAd, Aid, pStaAddr);
    }
}


#ifdef DOT11W_PMF_SUPPORT
VOID APMlmeKickOutAllSta(RTMP_ADAPTER *pAd, UCHAR apidx, USHORT Reason)
{
    HEADER_802_11 DisassocHdr;
    PUCHAR pOutBuffer = NULL;
    ULONG FrameLen = 0;
    NDIS_STATUS     NStatus;
    UCHAR           BROADCAST_ADDR[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    PPMF_CFG        pPmfCfg = NULL;

    pPmfCfg = &pAd->ApCfg.MBSSID[apidx].PmfCfg;
    if ((apidx < pAd->ApCfg.BssidNum) && (pPmfCfg))
    {
        /* Send out a Deauthentication request frame */
        NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
        if (NStatus != NDIS_STATUS_SUCCESS)
            return;
        DBGPRINT(RT_DEBUG_ERROR, ("Send DISASSOC Broadcast frame(%d) with ra%d \n", Reason, apidx));

        /* 802.11 Header */
        NdisZeroMemory(&DisassocHdr, sizeof(HEADER_802_11));
        DisassocHdr.FC.Type = FC_TYPE_MGMT;
        DisassocHdr.FC.SubType = SUBTYPE_DISASSOC;
        DisassocHdr.FC.ToDs = 0;
        DisassocHdr.FC.Wep = 0;
        COPY_MAC_ADDR(DisassocHdr.Addr1, BROADCAST_ADDR);
        COPY_MAC_ADDR(DisassocHdr.Addr2, pAd->ApCfg.MBSSID[apidx].wdev.bssid);
        COPY_MAC_ADDR(DisassocHdr.Addr3, pAd->ApCfg.MBSSID[apidx].wdev.bssid);
        MakeOutgoingFrame(pOutBuffer, &FrameLen,
            sizeof(HEADER_802_11), &DisassocHdr,
            2, &Reason,
            END_OF_ARGS);

        if (pPmfCfg->MFPC == TRUE)
        {
            ULONG TmpLen;
            UCHAR res_buf[LEN_PMF_MMIE];
            USHORT EID, ELen;
            
            EID = IE_PMF_MMIE;
            ELen = LEN_PMF_MMIE;
            MakeOutgoingFrame(pOutBuffer + FrameLen, &TmpLen,
                1, &EID,
                1, &ELen,
                LEN_PMF_MMIE, res_buf,
                END_OF_ARGS);
            FrameLen += TmpLen;
            DBGPRINT(RT_DEBUG_WARN, ("[PMF]: This is a Broadcast Robust management frame, Add 0x4C(76) EID\n"));
        }

        MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
        MlmeFreeMemory(pAd, pOutBuffer);
    }
}
#endif /* DOT11W_PMF_PLUGFEST */


/*
    ==========================================================================
    Description:
        Upper layer orders to disassoc s STA
    Parameters:
        Elem -
    ==========================================================================
 */
VOID APMlmeDisassocReqAction(RTMP_ADAPTER *pAd, MLME_QUEUE_ELEM *Elem)
{
	MLME_DISASSOC_REQ_STRUCT *DisassocReq = (MLME_DISASSOC_REQ_STRUCT *)(Elem->Msg);

	APMlmeKickOutSta(pAd, DisassocReq->Addr, Elem->Wcid, DisassocReq->Reason);
}


/*
    ==========================================================================
    Description:
        right part of IEEE 802.11/1999 page 374
    Note:
        This event should never cause ASSOC state machine perform state
        transition, and has no relationship with CNTL machine. So we separate
        this routine as a service outside of ASSOC state transition table.
    ==========================================================================
 */
VOID APCls3errAction(RTMP_ADAPTER *pAd, ULONG Wcid, HEADER_802_11 *pHeader)
{
    HEADER_802_11         DisassocHdr;
    PUCHAR                pOutBuffer = NULL;
    ULONG                 FrameLen = 0;
    NDIS_STATUS           NStatus;
    USHORT                Reason = REASON_CLS3ERR;
    MAC_TABLE_ENTRY       *pEntry = NULL;

#if 0
    pEntry = MacTableLookup(pAd, pHeader->Addr2);
#else
	if (Wcid < MAX_LEN_OF_MAC_TABLE)
		pEntry = &(pAd->MacTab.Content[Wcid]);
#endif

    if (pEntry)
    {
        /*ApLogEvent(pAd, pAddr, EVENT_DISASSOCIATED); */
		MacTableDeleteEntry(pAd, pEntry->wcid, pHeader->Addr2);
    }
    
    /* 2. send out a DISASSOC request frame */
    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    if (NStatus != NDIS_STATUS_SUCCESS)
        return;
    
    DBGPRINT(RT_DEBUG_TRACE, ("ASSOC - Class 3 Error, Send DISASSOC frame to %02x:%02x:%02x:%02x:%02x:%02x\n",
    				PRINT_MAC(pHeader->Addr2)));
    MgtMacHeaderInit(pAd, &DisassocHdr, SUBTYPE_DISASSOC, 0, pHeader->Addr2, 
						pHeader->Addr1,
						pHeader->Addr1);
    MakeOutgoingFrame(pOutBuffer,            &FrameLen,
                      sizeof(HEADER_802_11), &DisassocHdr,
                      2,                     &Reason,
                      END_OF_ARGS);
    MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
    MlmeFreeMemory(pAd, pOutBuffer);
}


/*
    ==========================================================================
    Description:
        association state machine init, including state transition and timer init
    Parameters:
        S - pointer to the association state machine
    Note:
        The state machine looks like the following
        
                                    AP_ASSOC_IDLE
        APMT2_MLME_DISASSOC_REQ    mlme_disassoc_req_action
        APMT2_PEER_DISASSOC_REQ    peer_disassoc_action
        APMT2_PEER_ASSOC_REQ       drop
        APMT2_PEER_REASSOC_REQ     drop
        APMT2_CLS3ERR              cls3err_action
    ==========================================================================
 */
VOID APAssocStateMachineInit(
    IN RTMP_ADAPTER *pAd,
    IN STATE_MACHINE *S,
    OUT STATE_MACHINE_FUNC Trans[])
{
    StateMachineInit(S, (STATE_MACHINE_FUNC*)Trans, AP_MAX_ASSOC_STATE, AP_MAX_ASSOC_MSG, (STATE_MACHINE_FUNC)Drop, AP_ASSOC_IDLE, AP_ASSOC_MACHINE_BASE);

    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_MLME_DISASSOC_REQ, (STATE_MACHINE_FUNC)APMlmeDisassocReqAction);
    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_PEER_DISASSOC_REQ, (STATE_MACHINE_FUNC)APPeerDisassocReqAction);
    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_PEER_ASSOC_REQ,    (STATE_MACHINE_FUNC)APPeerAssocReqAction);
    StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_PEER_REASSOC_REQ,  (STATE_MACHINE_FUNC)APPeerReassocReqAction);
/*  StateMachineSetAction(S, AP_ASSOC_IDLE, APMT2_CLS3ERR,           APCls3errAction); */
}

