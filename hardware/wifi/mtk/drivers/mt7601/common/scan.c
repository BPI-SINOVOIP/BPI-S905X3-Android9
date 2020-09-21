/*

*/

#include "rt_config.h"

#ifdef P2P_SUPPORT
extern UCHAR	WILDP2PSSID[];
extern UCHAR	WILDP2PSSIDLEN;
#endif /* P2P_SUPPORT */

#ifdef SCAN_SUPPORT
static INT scan_ch_restore(RTMP_ADAPTER *pAd, UCHAR OpMode)
{
#ifdef CONFIG_STA_SUPPORT
	USHORT Status;
#endif /* CONFIG_STA_SUPPORT */
	INT bw, ch;
		
	if (pAd->CommonCfg.BBPCurrentBW != pAd->hw_cfg.bbp_bw)
	{	
		rtmp_bbp_set_bw(pAd, pAd->hw_cfg.bbp_bw);

		AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
		AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);

		ch = pAd->CommonCfg.CentralChannel;
	}
	else
	{
		AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
		AsicLockChannel(pAd, pAd->CommonCfg.Channel);
		ch = pAd->CommonCfg.Channel;

	}

	switch(pAd->CommonCfg.BBPCurrentBW)
	{
		case BW_80:
			bw = 80;
			break;
		case BW_40:
			bw = 40;
			break;
		case BW_10:
			bw = 10;
			break;
		case BW_20:
		default:
			bw =20;
			break;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("SYNC - End of SCAN, restore to %dMHz channel %d, Total BSS[%02d]\n",
				bw, ch, pAd->ScanTab.BssNr));
		
#ifdef CONFIG_STA_SUPPORT
	if (OpMode == OPMODE_STA)
	{
		/*
		If all peer Ad-hoc clients leave, driver would do LinkDown and LinkUp.
		In LinkUp, CommonCfg.Ssid would copy SSID from MlmeAux. 
		To prevent SSID is zero or wrong in Beacon, need to recover MlmeAux.SSID here.
		*/
		if (ADHOC_ON(pAd))
		{
			NdisZeroMemory(pAd->MlmeAux.Ssid, MAX_LEN_OF_SSID);
			pAd->MlmeAux.SsidLen = pAd->CommonCfg.SsidLen;
			NdisMoveMemory(pAd->MlmeAux.Ssid, pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen);
		}


		/*
		To prevent data lost.
		Send an NULL data with turned PSM bit on to current associated AP before SCAN progress.
		Now, we need to send an NULL data with turned PSM bit off to AP, when scan progress done 
		*/
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED) && (INFRA_ON(pAd)))
		{
			RTMPSendNullFrame(pAd, 
								pAd->CommonCfg.TxRate, 
								(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
								pAd->CommonCfg.bAPSDForcePowerSave ? PWR_SAVE : pAd->StaCfg.Psm);
			DBGPRINT(RT_DEBUG_TRACE, ("%s -- Send null frame\n", __FUNCTION__));
		}

#ifdef RT_CFG80211_SUPPORT                                      
        if (pAd->ApCfg.ApCliTab[MAIN_MBSSID].Valid && RTMP_CFG80211_VIF_P2P_CLI_ON(pAd))
		{
			OS_WAIT(20);
			DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: PWR_ACTIVE SCAN_END\n"));
			RT_CFG80211_P2P_CLI_SEND_NULL_FRAME(pAd, PWR_ACTIVE);
		}

#endif /* RT_CFG80211_SUPPORT */
		

		/* keep the latest scan channel, could be 0 for scan complete, or other channel*/
		pAd->StaCfg.LastScanChannel = pAd->MlmeAux.Channel;

		pAd->StaCfg.ScanChannelCnt = 0;

		/* Suspend scanning and Resume TxData for Fast Scanning*/
		if ((pAd->MlmeAux.Channel != 0) &&
		(pAd->StaCfg.bImprovedScan))	/* it is scan pending*/
		{
			pAd->Mlme.SyncMachine.CurrState = SCAN_PENDING;
			Status = MLME_SUCCESS;
			DBGPRINT(RT_DEBUG_WARN, ("bFastRoamingScan ~~~ Get back to send data ~~~\n"));

			RTMPResumeMsduTransmission(pAd);
		}
		else
		{
			pAd->StaCfg.BssNr = pAd->ScanTab.BssNr;
			pAd->StaCfg.bImprovedScan = FALSE;

			pAd->Mlme.SyncMachine.CurrState = SYNC_IDLE;
			Status = MLME_SUCCESS;
			MlmeEnqueue(pAd, MLME_CNTL_STATE_MACHINE, MT2_SCAN_CONF, 2, &Status, 0);
			RTMP_MLME_HANDLER(pAd);
		}

	}
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	if (OpMode == OPMODE_AP)
	{
#ifdef P2P_APCLI_SUPPORT
		/* P2P CLIENT in WSC Scan or Re-Connect scanning. */
		if (P2P_CLI_ON(pAd) && (ApScanRunning(pAd) == TRUE))
		{
			/*MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_JOIN_REQ_TIMEOUT, 0, NULL, 0);*/
			DBGPRINT(RT_DEBUG_INFO, ("%s::  Scan Done! reset APCLI CTRL State Machine!\n", __FUNCTION__));
			pAd->ApCfg.ApCliTab[0].CtrlCurrState = APCLI_CTRL_DISCONNECTED;
		}
#endif /* P2P_APCLI_SUPPORT */

		pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;
		RTMPResumeMsduTransmission(pAd);

		/* iwpriv set auto channel selection*/
		/* scanned all channels*/
		if (pAd->ApCfg.bAutoChannelAtBootup==TRUE)
		{
			pAd->CommonCfg.Channel = SelectBestChannel(pAd, pAd->ApCfg.AutoChannelAlg);
			pAd->ApCfg.bAutoChannelAtBootup = FALSE;
#ifdef DOT11_N_SUPPORT
			N_ChannelCheck(pAd);
#endif /* DOT11_N_SUPPORT */
			APStop(pAd);
			APStartUp(pAd);
		}

		if (!((pAd->CommonCfg.Channel > 14) && (pAd->CommonCfg.bIEEE80211H == TRUE)))
			AsicEnableBssSync(pAd);
	}
#endif /* CONFIG_AP_SUPPORT */


	return TRUE;
}



static INT scan_active(RTMP_ADAPTER *pAd, UCHAR OpMode, UCHAR ScanType)
{
	UCHAR *frm_buf = NULL;
	HEADER_802_11 Hdr80211;
	ULONG FrameLen = 0;
	UCHAR SsidLen = 0;
#ifdef CONFIG_STA_SUPPORT
	USHORT Status;
#endif /* CONFIG_STA_SUPPORT */
#ifdef P2P_SUPPORT
	UCHAR SupRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR SupRateLen = 0;
#endif /* P2P_SUPPORT */


	if (MlmeAllocateMemory(pAd, &frm_buf) != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("SYNC - ScanNextChannel() allocate memory fail\n"));
#ifdef CONFIG_STA_SUPPORT
		if (OpMode == OPMODE_STA)
		{
			pAd->Mlme.SyncMachine.CurrState = SYNC_IDLE;
			Status = MLME_FAIL_NO_RESOURCE;
			MlmeEnqueue(pAd, MLME_CNTL_STATE_MACHINE, MT2_SCAN_CONF, 2, &Status, 0);
		}
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
		if (OpMode == OPMODE_AP)
			pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;
#endif /* CONFIG_AP_SUPPORT */
		return FALSE;
	}

#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
	if (ScanType == SCAN_2040_BSS_COEXIST)
	{
		DBGPRINT(RT_DEBUG_INFO, ("SYNC - SCAN_2040_BSS_COEXIST !! Prepare to send Probe Request\n"));
	}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
	
	/* There is no need to send broadcast probe request if active scan is in effect.*/
	SsidLen = 0;
	if ((ScanType == SCAN_ACTIVE) || (ScanType == FAST_SCAN_ACTIVE)
#ifdef WSC_STA_SUPPORT
		|| ((ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_STA))
#endif /* WSC_STA_SUPPORT */
#ifdef P2P_SUPPORT
		|| (ScanType == SCAN_P2P)
#endif /* P2P_SUPPORT */
		)
		SsidLen = pAd->MlmeAux.SsidLen;

#ifdef P2P_SUPPORT
	if (ScanType == SCAN_P2P) {
		DBGPRINT(RT_DEBUG_INFO,
			("%s: p2p scan from cfg80211, %X:%X:%X:%X:%X:%X for probe req\n",
			__func__, PRINT_MAC(pAd->cfg80211_ctrl.P2PCurrentAddress)));
		MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR,
			pAd->cfg80211_ctrl.P2PCurrentAddress, BROADCAST_ADDR);

		SupRate[0]	= 0x8C;    /* 6 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[1]	= 0x12;    /* 9 mbps, in units of 0.5 Mbps */
		SupRate[2]	= 0x98;    /* 12 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[3]	= 0x24;    /* 18 mbps, in units of 0.5 Mbps */
		SupRate[4]	= 0xb0;    /* 24 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[5]	= 0x48;    /* 36 mbps, in units of 0.5 Mbps */
		SupRate[6]	= 0x60;    /* 48 mbps, in units of 0.5 Mbps */
		SupRate[7]	= 0x6c;    /* 54 mbps, in units of 0.5 Mbps */
		SupRateLen	= 8;

		MakeOutgoingFrame(frm_buf,	&FrameLen,
			sizeof(HEADER_802_11),	&Hdr80211,
			1,						&SsidIe,
			1,						&SsidLen,
			SsidLen,				pAd->MlmeAux.Ssid,
			1,						&SupRateIe,
			1,						&SupRateLen,
			SupRateLen,				SupRate,
			END_OF_ARGS);
	} else
#endif /* P2P_SUPPORT */
	{
#ifdef CONFIG_AP_SUPPORT
		/*IF_DEV_CONFIG_OPMODE_ON_AP(pAd) */
		if (OpMode == OPMODE_AP)
		{
			MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR, 
#ifdef P2P_SUPPORT
								pAd->ApCfg.MBSSID[0].Bssid,
#endif /* P2P_SUPPORT */
								pAd->ApCfg.MBSSID[0].Bssid);
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		/*IF_DEV_CONFIG_OPMODE_ON_STA(pAd) */
		if (OpMode == OPMODE_STA)
		{
			MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR, 
#ifdef P2P_SUPPORT
								pAd->CurrentAddress,
#endif /* P2P_SUPPORT */
								BROADCAST_ADDR);
		}
#endif /* CONFIG_STA_SUPPORT */

		MakeOutgoingFrame(frm_buf,               &FrameLen,
						  sizeof(HEADER_802_11),    &Hdr80211,
						  1,                        &SsidIe,
						  1,                        &SsidLen,
						  SsidLen,			        pAd->MlmeAux.Ssid,
						  1,                        &SupRateIe,
						  1,                        &pAd->CommonCfg.SupRateLen,
						  pAd->CommonCfg.SupRateLen,  pAd->CommonCfg.SupRate, 
						  END_OF_ARGS);

		if (pAd->CommonCfg.ExtRateLen)
		{
			ULONG Tmp;
			MakeOutgoingFrame(frm_buf + FrameLen,            &Tmp,
							  1,                                &ExtRateIe,
							  1,                                &pAd->CommonCfg.ExtRateLen,
							  pAd->CommonCfg.ExtRateLen,          pAd->CommonCfg.ExtRate, 
							  END_OF_ARGS);
			FrameLen += Tmp;
		}
	}
#ifdef DOT11_N_SUPPORT
	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
	{
		ULONG	Tmp;
		UCHAR	HtLen;
		UCHAR	BROADCOM[4] = {0x0, 0x90, 0x4c, 0x33};
#ifdef RT_BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
#endif
		if (pAd->bBroadComHT == TRUE)
		{
			HtLen = pAd->MlmeAux.HtCapabilityLen + 4;
#ifdef RT_BIG_ENDIAN
			NdisMoveMemory(&HtCapabilityTmp, &pAd->MlmeAux.HtCapability, SIZE_HT_CAP_IE);
			*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
#ifdef UNALIGNMENT_SUPPORT
			{
				EXT_HT_CAP_INFO extHtCapInfo;

				NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
				*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
				NdisMoveMemory((PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
			}
#else				
			*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = cpu2le16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &WpaIe,
							1,                                &HtLen,
							4,                                &BROADCOM[0],
							pAd->MlmeAux.HtCapabilityLen,     &HtCapabilityTmp, 
							END_OF_ARGS);
#else
			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &WpaIe,
							1,                                &HtLen,
							4,                                &BROADCOM[0],
							pAd->MlmeAux.HtCapabilityLen,     &pAd->MlmeAux.HtCapability, 
							END_OF_ARGS);
#endif /* RT_BIG_ENDIAN */
		}
		else				
		{
			HtLen = sizeof(HT_CAPABILITY_IE);
#ifdef RT_BIG_ENDIAN
			NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, SIZE_HT_CAP_IE);
			*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
#ifdef UNALIGNMENT_SUPPORT
			{
				EXT_HT_CAP_INFO extHtCapInfo;

				NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
				*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
				NdisMoveMemory((PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
			}
#else				
			*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = cpu2le16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &HtCapIe,
							1,                                &HtLen,
							HtLen,                            &HtCapabilityTmp, 
							END_OF_ARGS);
#else
			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &HtCapIe,
							1,                                &HtLen,
							HtLen,                            &pAd->CommonCfg.HtCapability, 
							END_OF_ARGS);
#endif /* RT_BIG_ENDIAN */
		}
		FrameLen += Tmp;

#ifdef DOT11N_DRAFT3
		if ((pAd->MlmeAux.Channel <= 14) && (pAd->CommonCfg.bBssCoexEnable == TRUE))
		{
			ULONG Tmp;
			HtLen = 1;
			MakeOutgoingFrame(frm_buf + FrameLen,            &Tmp,
							  1,					&ExtHtCapIe,
							  1,					&HtLen,
							  1,          			&pAd->CommonCfg.BSSCoexist2040.word, 
							  END_OF_ARGS);

			FrameLen += Tmp;
		}
#endif /* DOT11N_DRAFT3 */
	}
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11_VHT_AC
	if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
		(pAd->MlmeAux.Channel > 14)) {		
		FrameLen += build_vht_ies(pAd, (UCHAR *)(frm_buf + FrameLen), SUBTYPE_PROBE_REQ);
	}
#endif /* DOT11_VHT_AC */

#ifdef WSC_STA_SUPPORT
	if (OpMode == OPMODE_STA)
	{
		BOOLEAN bHasWscIe = FALSE;
		/* 
			Append WSC information in probe request if WSC state is running
		*/
		if ((pAd->StaCfg.WscControl.WscEnProbeReqIE) && 
			(pAd->StaCfg.WscControl.WscConfMode != WSC_DISABLE) &&
			(pAd->StaCfg.WscControl.bWscTrigger == TRUE))
			bHasWscIe = TRUE;
#ifdef WSC_V2_SUPPORT
		else if ((pAd->StaCfg.WscControl.WscEnProbeReqIE) && 
			(pAd->StaCfg.WscControl.WscV2Info.bEnableWpsV2))
			bHasWscIe = TRUE;
#endif /* WSC_V2_SUPPORT */

#ifdef P2P_SUPPORT
	/* 
		P2pMakeProbeReqIE will build WSC IE for P2P, 
		it is not good to append normal WSC IE into P2P probe request frame here.
	*/
	if ((pAd->MlmeAux.ScanType == SCAN_P2P) || (pAd->MlmeAux.ScanType == SCAN_P2P_SEARCH) ||
		((pAd->MlmeAux.ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_AP) && (P2P_CLI_ON(pAd))))
		bHasWscIe = FALSE;
#endif /* P2P_SUPPORT */

		if (bHasWscIe)
		{
			UCHAR *pWscBuf = NULL, WscIeLen = 0;
			ULONG WscTmpLen = 0;

			os_alloc_mem(NULL, (UCHAR **)&pWscBuf, 512);
			if (pWscBuf != NULL)
			{
				NdisZeroMemory(pWscBuf, 512);
				WscBuildProbeReqIE(pAd, STA_MODE, pWscBuf, &WscIeLen);

				MakeOutgoingFrame(frm_buf + FrameLen,              &WscTmpLen,
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

#ifdef CONFIG_STA_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	if ((OpMode == OPMODE_STA) &&
		(pAd->StaCfg.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE) &&
		(pAd->StaCfg.WpsProbeReqIeLen != 0))
	{
		ULONG 		WpsTmpLen = 0;
		
		MakeOutgoingFrame(frm_buf + FrameLen,              &WpsTmpLen,
						pAd->StaCfg.WpsProbeReqIeLen,	pAd->StaCfg.pWpsProbeReqIe,
						END_OF_ARGS);

		FrameLen += WpsTmpLen;
	}
#endif /* WPA_SUPPLICANT_SUPPORT */
#endif /*CONFIG_STA_SUPPORT*/

#ifdef P2P_SUPPORT
	if ((pAd->MlmeAux.ScanType == SCAN_P2P) || (pAd->MlmeAux.ScanType == SCAN_P2P_SEARCH)
#ifdef P2P_APCLI_SUPPORT
		|| ((pAd->MlmeAux.ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_AP) && (P2P_CLI_ON(pAd)))
#endif /* P2P_APCLI_SUPPORT */
	)
	{
		ULONG P2PIeLen;
		UCHAR tmp_len;
		PUCHAR ptr;
		ptr = frm_buf + FrameLen;
		P2pMakeProbeReqIE(pAd, ptr, &tmp_len);
		FrameLen += tmp_len;
		
		/* Put P2P IE to the last. */
		ptr = frm_buf + FrameLen;
		P2pMakeP2pIE(pAd, SUBTYPE_PROBE_REQ, ptr, &P2PIeLen);

		FrameLen += P2PIeLen;
	}

#endif /* P2P_SUPPORT */

	MiniportMMRequest(pAd, 0, frm_buf, FrameLen);

#ifdef CONFIG_STA_SUPPORT
	if (OpMode == OPMODE_STA)
	{
		/*
			To prevent data lost.
			Send an NULL data with turned PSM bit on to current associated AP when SCAN in the channel where
			associated AP located.
		*/
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED) && 
			(INFRA_ON(pAd)) &&
			(pAd->CommonCfg.Channel == pAd->MlmeAux.Channel))
		{
			RTMPSendNullFrame(pAd, 
						  pAd->CommonCfg.TxRate, 
						  (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
						  PWR_SAVE);
			DBGPRINT(RT_DEBUG_TRACE, ("ScanNextChannel():Send PWA NullData frame to notify the associated AP!\n"));
		}
	}

#if 0//def RT_CFG80211_SUPPORT                                      
                if (pAd->ApCfg.ApCliTab[MAIN_MBSSID].Valid && RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) 
			&& (pAd->CommonCfg.Channel == pAd->MlmeAux.Channel))
		{
                
			DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: PWR_SAVE SCANNING\n"));
			RT_CFG80211_P2P_CLI_SEND_NULL_FRAME(pAd, PWR_SAVE);
		}
#endif /* RT_CFG80211_SUPPORT */

#endif /* CONFIG_STA_SUPPORT */

	MlmeFreeMemory(pAd, frm_buf);

	return TRUE;
}


/*
	==========================================================================
	Description:
		Scan next channel
	==========================================================================
 */
VOID ScanNextChannel(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR OpMode) 
{
	UCHAR ScanType = pAd->MlmeAux.ScanType;
	UINT ScanTimeIn5gChannel = SHORT_CHANNEL_TIME;
	BOOLEAN ScanPending = FALSE;
	RALINK_TIMER_STRUCT *sc_timer;
	UINT stay_time = 0;
			

#ifdef RALINK_ATE
	/* Nothing to do in ATE mode. */
	if (ATE_ON(pAd))
		return;
#endif /* RALINK_ATE */


#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (MONITOR_ON(pAd))
			return;
	}

	ScanPending = ((pAd->StaCfg.bImprovedScan) && (pAd->StaCfg.ScanChannelCnt>=7));
#endif /* CONFIG_STA_SUPPORT */

#ifdef RT_CFG80211_SUPPORT
#ifdef CONFIG_STA_SUPPORT
                if ( (pAd->pCfg80211ChanList != NULL) && (pAd->MlmeAux.Channel != 0))
                {
                
                        UINT32 ChanId;
                        for ( ChanId = 0 ; ChanId <pAd->Cfg80211ChanListLan ; ChanId++ )
                        {
                        
                        		if ((RTMP_CFG80211_VIF_P2P_GO_ON(pAd) || 
									 (RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) && pAd->apcli_wfd_connect == TRUE)) &&
									  pAd->CommonCfg.Channel)
                        		{
                        			if(pAd->MlmeAux.Channel <= pAd->CommonCfg.Channel && pAd->MlmeAux.Channel >0 )
                        			{
	                        			if(pAd->pCfg80211ChanList[ChanId] == pAd->CommonCfg.Channel)
	                        			{
	                        				pAd->MlmeAux.Channel = pAd->pCfg80211ChanList[ChanId];
											DBGPRINT(RT_DEBUG_OFF, ("ScanNextChannel():Scan Only Go or wfd Channel %d!\n", pAd->MlmeAux.Channel));
											break;
	                        			}
                        			}
									else
									{
										if(pAd->MlmeAux.Channel != 0)
										{											
											DBGPRINT(RT_DEBUG_TRACE, ("ScanNextChannel():Scan Go or wfd Channel Done, clear the channel list. pAd->MlmeAux.Channel=%d\n", pAd->MlmeAux.Channel));
											os_free_mem(NULL, pAd->pCfg80211ChanList);
                       						pAd->pCfg80211ChanList = NULL;
                        					pAd->Cfg80211ChanListLan = 0;
                        					pAd->MlmeAux.Channel = 0;
										}
										DBGPRINT(RT_DEBUG_TRACE, ("ScanNextChannel():Scan Go or wfd Channel Done. pAd->MlmeAux.Channel=%d\n", pAd->MlmeAux.Channel));
										break;
									}
                        		}
                                else if ( pAd->pCfg80211ChanList[ChanId] >= pAd->MlmeAux.Channel )
                                {
                                        pAd->MlmeAux.Channel = pAd->pCfg80211ChanList[ChanId];
                                        break;
                                }
                        }

                }

#endif /* CONFIG_STA_SUPPORT */
#endif /* RT_CFG80211_SUPPORT */

	if ((pAd->MlmeAux.Channel == 0) || ScanPending) 
	{
		scan_ch_restore(pAd, OpMode);
	} 
#ifdef RTMP_MAC_USB
#ifdef CONFIG_STA_SUPPORT
	else if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST) &&
		(OpMode == OPMODE_STA))
	{
		pAd->Mlme.SyncMachine.CurrState = SYNC_IDLE;
		MlmeCntlConfirm(pAd, MT2_SCAN_CONF, MLME_FAIL_NO_RESOURCE);
	}	
#endif /* CONFIG_STA_SUPPORT */
#endif /* RTMP_MAC_USB */
	else 
	{
#ifdef CONFIG_STA_SUPPORT
		if (OpMode == OPMODE_STA)
		{
			/* BBP and RF are not accessible in PS mode, we has to wake them up first*/
			if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
				AsicForceWakeup(pAd, TRUE);

			/* leave PSM during scanning. otherwise we may lost ProbeRsp & BEACON*/
			if (pAd->StaCfg.Psm == PWR_SAVE)
				RTMP_SET_PSM_BIT(pAd, PWR_ACTIVE);
		}
#endif /* CONFIG_STA_SUPPORT */

		AsicSwitchChannel(pAd, pAd->MlmeAux.Channel, TRUE);
		AsicLockChannel(pAd, pAd->MlmeAux.Channel);

#ifdef CONFIG_STA_SUPPORT
		if (OpMode == OPMODE_STA)
		{
			BOOLEAN bScanPassive = FALSE;
			if (pAd->MlmeAux.Channel > 14)
			{
				if ((pAd->CommonCfg.bIEEE80211H == 1)
					&& RadarChannelCheck(pAd, pAd->MlmeAux.Channel))
					bScanPassive = TRUE;
			}
#ifdef CARRIER_DETECTION_SUPPORT
			if (pAd->CommonCfg.CarrierDetect.Enable == TRUE)
				bScanPassive = TRUE;
#endif /* CARRIER_DETECTION_SUPPORT */ 

			if (bScanPassive)
			{
				ScanType = SCAN_PASSIVE;
				ScanTimeIn5gChannel = MIN_CHANNEL_TIME;
			}
		}

#endif /* CONFIG_STA_SUPPORT */

		/* Check if channel if passive scan under current regulatory domain */
		if (CHAN_PropertyCheck(pAd, pAd->MlmeAux.Channel, CHANNEL_PASSIVE_SCAN) == TRUE)
			ScanType = SCAN_PASSIVE;


		if (OpMode == OPMODE_AP)
			sc_timer = &pAd->MlmeAux.APScanTimer;
		else
			sc_timer = &pAd->MlmeAux.ScanTimer;
			
		/* We need to shorten active scan time in order for WZC connect issue */
		/* Chnage the channel scan time for CISCO stuff based on its IAPP announcement */
		if (ScanType == FAST_SCAN_ACTIVE)
			stay_time = FAST_ACTIVE_SCAN_TIME;
		else /* must be SCAN_PASSIVE or SCAN_ACTIVE*/
		{
#ifdef CONFIG_STA_SUPPORT
			pAd->StaCfg.ScanChannelCnt++;
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
			if ((OpMode == OPMODE_AP) && (pAd->ApCfg.bAutoChannelAtBootup))
				stay_time = AUTO_CHANNEL_SEL_TIMEOUT;
			else
#endif /* CONFIG_AP_SUPPORT */
			if (WMODE_CAP_2G(pAd->CommonCfg.PhyMode) &&
				WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
			{
				if (pAd->MlmeAux.Channel > 14)
					stay_time = ScanTimeIn5gChannel;
				else
					stay_time = MIN_CHANNEL_TIME;
			}
			else
				stay_time = MAX_CHANNEL_TIME;
		}
				
		RTMPSetTimer(sc_timer, stay_time);
			
		if (SCAN_MODE_ACT(ScanType))
		{
			if (scan_active(pAd, OpMode, ScanType) == FALSE)
				return;
		}

		/* For SCAN_CISCO_PASSIVE, do nothing and silently wait for beacon or other probe reponse*/
		
#ifdef CONFIG_STA_SUPPORT
		if (OpMode == OPMODE_STA)
			pAd->Mlme.SyncMachine.CurrState = SCAN_LISTEN;
#endif /* CONFIG_STA_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
		if (OpMode == OPMODE_AP)
			pAd->Mlme.ApSyncMachine.CurrState = AP_SCAN_LISTEN;
#endif /* CONFIG_AP_SUPPORT */
	}
}


BOOLEAN ScanRunning(
		IN PRTMP_ADAPTER pAd)
{
	BOOLEAN	rv = FALSE;

#ifdef CONFIG_STA_SUPPORT
#ifdef P2P_SUPPORT
		rv = ((pAd->Mlme.ApSyncMachine.CurrState == AP_SCAN_LISTEN) ? TRUE : FALSE);

		if (rv == FALSE)
		{
			if ((pAd->Mlme.SyncMachine.CurrState == SCAN_LISTEN) || (pAd->Mlme.SyncMachine.CurrState == SCAN_PENDING))
				rv = TRUE;
		}
#else
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			if ((pAd->Mlme.SyncMachine.CurrState == SCAN_LISTEN) || (pAd->Mlme.SyncMachine.CurrState == SCAN_PENDING))
				rv = TRUE;
		}
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
#ifdef AP_SCAN_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			rv = ((pAd->Mlme.ApSyncMachine.CurrState == AP_SCAN_LISTEN) ? TRUE : FALSE);
#endif /* AP_SCAN_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	return rv;
}

#endif /* SCAN_SUPPORT */

