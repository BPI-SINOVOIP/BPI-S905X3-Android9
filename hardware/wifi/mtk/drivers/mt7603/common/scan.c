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

	Abstract:

	Revision History:
	Who 		When			What
	--------	----------		----------------------------------------------
*/

#include "rt_config.h"

#ifdef SCAN_SUPPORT
static INT scan_ch_restore(RTMP_ADAPTER *pAd, UCHAR OpMode)
{
#ifdef CONFIG_STA_SUPPORT
	USHORT Status;
#endif /* CONFIG_STA_SUPPORT */
	INT bw, ch;
		
#if defined(RT_CFG80211_SUPPORT) && defined(RT_CFG80211_P2P_CONCURRENT_DEVICE)
	BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[CFG_GO_BSSID_IDX];
	PAPCLI_STRUCT pApCliEntry = pApCliEntry = &pAd->ApCfg.ApCliTab[MAIN_MBSSID];
	struct wifi_dev *p2p_wdev = &pMbss->wdev;
	struct wifi_dev *wdev;

	if(RTMP_CFG80211_VIF_P2P_GO_ON(pAd) )
	{
		p2p_wdev = &pMbss->wdev;
	}
	else if(RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) )
	{
		p2p_wdev = &pApCliEntry->wdev;
	}

	if(INFRA_ON(pAd) && (!RTMP_CFG80211_VIF_P2P_GO_ON(pAd)))
	{
		//this should be resotre to infra sta!!
		wdev = &pAd->StaCfg.wdev;
	       bbp_set_bw(pAd, pAd->StaCfg.wdev.bw);
	}
	else		
#endif /* defined(RT_CFG80211_SUPPORT) && defined(RT_CFG80211_P2P_CONCURRENT_DEVICE) */

        if (pAd->CommonCfg.BBPCurrentBW != pAd->hw_cfg.bbp_bw)
                bbp_set_bw(pAd, pAd->hw_cfg.bbp_bw);

#ifdef DOT11_VHT_AC
	if (pAd->hw_cfg.bbp_bw == BW_80)
		ch = pAd->CommonCfg.vht_cent_ch;
	else 
#endif /* DOT11_VHT_AC */
	if (pAd->hw_cfg.bbp_bw == BW_40)
		ch = pAd->CommonCfg.CentralChannel;
        else
		ch = pAd->CommonCfg.Channel;

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
		
#if defined(RT_CFG80211_SUPPORT) && defined(RT_CFG80211_P2P_CONCURRENT_DEVICE)
        if (INFRA_ON(pAd))
        {
		bw = pAd->StaCfg.wdev.bw;
		bbp_set_bw(pAd, (UINT8)bw);

        }
 	else if (RTMP_CFG80211_VIF_P2P_GO_ON(pAd) && (ch != p2p_wdev->channel) && (p2p_wdev->CentralChannel != 0))
	{
		bw = p2p_wdev->bw;
		bbp_set_bw(pAd, (UINT8)bw);
	}
	else if (RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) && (ch != p2p_wdev->channel) && (p2p_wdev->CentralChannel != 0))
	{
		bw = p2p_wdev->bw;
		bbp_set_bw(pAd, (UINT8)bw);
	}
/*If GO start, we need to change to GO Channel*/
        if (INFRA_ON(pAd))
        {
                ch = pAd->StaCfg.wdev.CentralChannel;
        }
	else if((ch != p2p_wdev->CentralChannel) && (p2p_wdev->CentralChannel != 0))
		ch = p2p_wdev->CentralChannel;

#endif /* defined(RT_CFG80211_SUPPORT) && defined(RT_CFG80211_P2P_CONCURRENT_DEVICE) */

	ASSERT((ch != 0));
	AsicSwitchChannel(pAd, (UCHAR)ch, FALSE);
	AsicLockChannel(pAd, (UCHAR)ch);

	DBGPRINT(RT_DEBUG_TRACE,
		("SYNC - End of SCAN, restore to %dMHz channel %d, Total BSS[%02d]\n",
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
								pAd->CommonCfg.bAPSDForcePowerSave ? PWR_SAVE : pAd->StaCfg.PwrMgmt.Psm);
			DBGPRINT(RT_DEBUG_TRACE, ("%s -- Send null frame\n", __FUNCTION__));
		}

#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE                               
        if (pAd->ApCfg.ApCliTab[MAIN_MBSSID].Valid && RTMP_CFG80211_VIF_P2P_CLI_ON(pAd))
    	{
            	DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: PWR_ACTIVE SCAN_END\n"));
            	RT_CFG80211_P2P_CLI_SEND_NULL_FRAME(pAd, PWR_ACTIVE);
    	}
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */

		/* keep the latest scan channel, could be 0 for scan complete, or other channel*/
		pAd->StaCfg.LastScanChannel = pAd->ScanCtrl.Channel;

		pAd->StaCfg.ScanChannelCnt = 0;

		/* Suspend scanning and Resume TxData for Fast Scanning*/
		if ((pAd->ScanCtrl.Channel != 0) &&
		(pAd->StaCfg.bImprovedScan))	/* it is scan pending*/
		{
			MLME_SCAN_REQ_STRUCT       ScanReq;
			pAd->Mlme.SyncMachine.CurrState = SCAN_PENDING;
			Status = MLME_SUCCESS;
			DBGPRINT(RT_DEBUG_WARN, ("bFastRoamingScan ~~~ Get back to send data ~~~\n"));

			RTMPResumeMsduTransmission(pAd);
/* for SCC & MCC Case*/
			pAd->StaCfg.LastScanTime = pAd->Mlme.Now32;
			ScanParmFill(pAd, &ScanReq, pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen, BSS_ANY, pAd->MlmeAux.ScanType);
			MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ, sizeof(MLME_SCAN_REQ_STRUCT), &ScanReq, 0);
			DBGPRINT(RT_DEBUG_OFF, ("bImprovedScan ............. Resume for bImprovedScan, SCAN_PENDING .............. \n"));
			RTMP_MLME_HANDLER(pAd);

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
#ifdef APCLI_SUPPORT
#ifdef APCLI_AUTO_CONNECT_SUPPORT
			if (pAd->ApCfg.ApCliAutoConnectRunning == TRUE)
			{
				if (!ApCliAutoConnectExec(pAd))
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Error in  %s\n", __FUNCTION__));
				}
			}			
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
#endif /* APCLI_SUPPORT */
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

		if (!((pAd->CommonCfg.Channel > 14) && (pAd->CommonCfg.bIEEE80211H == TRUE) && (pAd->Dot11_H.RDMode != RD_NORMAL_MODE)))
			AsicEnableBssSync(pAd, pAd->CommonCfg.BeaconPeriod);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef MESH_SUPPORT
	MlmeEnqueue(pAd, MESH_CTRL_STATE_MACHINE, APMT2_MLME_SCAN_FINISH, 0, NULL, 0);
#endif /* MESH_SUPPORT */

	return TRUE;
}


#if 0
static INT scan_type_chk(RTMP_ADAPTER *pAd, UCHAR OpMode, UCHAR sc_type)
{
	return TRUE;
}
#endif

static INT scan_active(RTMP_ADAPTER *pAd, UCHAR OpMode, UCHAR ScanType)
{
	UCHAR *frm_buf = NULL;
	HEADER_802_11 Hdr80211;
	ULONG FrameLen = 0;
	UCHAR SsidLen = 0;
#ifdef CONFIG_STA_SUPPORT
	USHORT Status;
#endif /* CONFIG_STA_SUPPORT */
	UINT i;

	if (MlmeAllocateMemory(pAd, &frm_buf) != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s():allocate memory fail\n", __FUNCTION__));
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
#ifdef RT_CFG80211_P2P_SUPPORT
		|| (ScanType == SCAN_P2P)
#endif /* RT_CFG80211_P2P_SUPPORT */
		)
		SsidLen = pAd->ScanCtrl.SsidLen;

#ifdef P2P_SUPPORT
	if ((pAd->ScanCtrl.ScanType == SCAN_P2P) || (pAd->ScanCtrl.ScanType == SCAN_P2P_SEARCH)
#ifdef P2P_APCLI_SUPPORT
		 || ((pAd->ScanCtrl.ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_AP) && (P2P_CLI_ON(pAd)))
#endif /* P2P_APCLI_SUPPORT */
	)
	{
		PRT_P2P_CONFIG pP2PCtrl = &pAd->P2pCfg;
		UCHAR		SupRate[MAX_LEN_OF_SUPPORTED_RATES];
		UCHAR		SupRateLen = 0;

		SsidLen = WILDP2PSSIDLEN; /* Use Wildword SSID */
		SupRate[0]	= 0x8C;    /* 6 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[1]	= 0x12;    /* 9 mbps, in units of 0.5 Mbps */
		SupRate[2]	= 0x98;    /* 12 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[3]	= 0x24;    /* 18 mbps, in units of 0.5 Mbps */
		SupRate[4]	= 0xb0;    /* 24 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[5]	= 0x48;    /* 36 mbps, in units of 0.5 Mbps */
		SupRate[6]	= 0x60;    /* 48 mbps, in units of 0.5 Mbps */
		SupRate[7]	= 0x6c;    /* 54 mbps, in units of 0.5 Mbps */
		SupRateLen	= 8;
		/* P2P scan must use P2P mac address. */
		MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR,
							pP2PCtrl->CurrentAddress,
							BROADCAST_ADDR);

		MakeOutgoingFrame(frm_buf,				&FrameLen,
							sizeof(HEADER_802_11),	&Hdr80211,
							1,						&SsidIe,
							1,						&SsidLen,
							SsidLen,					&WILDP2PSSID[0],
							1,						&SupRateIe,
							1,						&SupRateLen,
							SupRateLen, 			SupRate, 
							END_OF_ARGS);
	}
	else
#endif /* P2P_SUPPORT */
#ifdef RT_CFG80211_P2P_SUPPORT
    if (ScanType == SCAN_P2P)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s(): this is a p2p scan from cfg80211 layer\n", __FUNCTION__));
#ifdef RT_CFG80211_P2P_STATIC_CONCURRENT_DEVICE
		MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR,
				      pAd->cfg80211_ctrl.P2PCurrentAddress, BROADCAST_ADDR);
#else
		MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR,
                                      pAd->CurrentAddress, BROADCAST_ADDR);
#endif /* RT_CFG80211_P2P_STATIC_CONCURRENT_DEVICE */
                MakeOutgoingFrame(frm_buf,               &FrameLen,
                                                  sizeof(HEADER_802_11),    &Hdr80211,
                                                  1,                        &SsidIe,
                                                  1,                        &SsidLen,
                                                  SsidLen,                  pAd->ScanCtrl.Ssid,
                                                  1,                        &SupRateIe,
                                                  1,                        &pAd->cfg80211_ctrl.P2pSupRateLen,
                                                  pAd->cfg80211_ctrl.P2pSupRateLen,  pAd->cfg80211_ctrl.P2pSupRate,
                                                  END_OF_ARGS);
	}
	else
#endif /* RT_CFG80211_P2P_SUPPORT */
	{
#ifdef CONFIG_AP_SUPPORT
		/*IF_DEV_CONFIG_OPMODE_ON_AP(pAd) */
		if (OpMode == OPMODE_AP)
		{
			MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR,
								pAd->ApCfg.MBSSID[0].wdev.bssid,
								BROADCAST_ADDR);
#if 0
			MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR, 
								pAd->ApCfg.MBSSID[0].Bssid,
								pAd->ApCfg.MBSSID[0].Bssid);
#endif
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		/*IF_DEV_CONFIG_OPMODE_ON_STA(pAd) */
		if (OpMode == OPMODE_STA)
		{
			MgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR, 
								pAd->CurrentAddress,
								BROADCAST_ADDR);
		}
#endif /* CONFIG_STA_SUPPORT */

		MakeOutgoingFrame(frm_buf,               &FrameLen,
						  sizeof(HEADER_802_11),    &Hdr80211,
						  1,                        &SsidIe,
						  1,                        &SsidLen,
						  SsidLen,			        pAd->ScanCtrl.Ssid,
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
#ifdef RT_BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
#endif

#ifdef CONFIG_STA_SUPPORT
		if ((pAd->bBroadComHT == TRUE) && (OpMode == OPMODE_STA))
		{
			UCHAR BROADCOM[4] = {0x0, 0x90, 0x4c, 0x33};

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
#endif /* CONFIG_STA_SUPPORT */
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
		if ((pAd->ScanCtrl.Channel <= 14) && (pAd->CommonCfg.bBssCoexEnable == TRUE))
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
		(pAd->ScanCtrl.Channel > 14)) {		
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
	if ((pAd->ScanCtrl.ScanType == SCAN_P2P) || (pAd->ScanCtrl.ScanType == SCAN_P2P_SEARCH) ||
		((pAd->ScanCtrl.ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_AP) && (P2P_CLI_ON(pAd))))
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

#ifdef WAC_SUPPORT
	WAC_AddDevAdvAttrIE(pAd, BSS0, frm_buf, &FrameLen);
#endif /* WAC_SUPPORT */
#endif /* WSC_STA_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
	if ((OpMode == OPMODE_STA) &&
		(pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE) &&
		(pAd->StaCfg.wpa_supplicant_info.WpsProbeReqIeLen != 0))
	{
		ULONG 		WpsTmpLen = 0;
		
		MakeOutgoingFrame(frm_buf + FrameLen,              &WpsTmpLen,
						pAd->StaCfg.wpa_supplicant_info.WpsProbeReqIeLen,
						pAd->StaCfg.wpa_supplicant_info.pWpsProbeReqIe,
						END_OF_ARGS);

		FrameLen += WpsTmpLen;
	}
#endif /* WPA_SUPPLICANT_SUPPORT */
#ifdef RT_CFG80211_SUPPORT
	if ((OpMode == OPMODE_STA) &&
		(pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE) &&
		CFG80211DRV_OpsScanRunning(pAd))
	{
		ULONG 		ExtraIeTmpLen = 0;
		
		MakeOutgoingFrame(frm_buf + FrameLen,              &ExtraIeTmpLen,
						pAd->cfg80211_ctrl.ExtraIeLen,	pAd->cfg80211_ctrl.pExtraIe,
						END_OF_ARGS);

		FrameLen += ExtraIeTmpLen;	
	}
#endif /* RT_CFG80211_SUPPORT */
#endif /*CONFIG_STA_SUPPORT*/

#ifdef P2P_SUPPORT
	if ((pAd->ScanCtrl.ScanType == SCAN_P2P) || (pAd->ScanCtrl.ScanType == SCAN_P2P_SEARCH)
#ifdef P2P_APCLI_SUPPORT
		|| ((pAd->ScanCtrl.ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_AP) && (P2P_CLI_ON(pAd)))
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
#ifdef WFD_SUPPORT
		ptr = frm_buf + FrameLen;
		WfdMakeWfdIE(pAd, SUBTYPE_PROBE_REQ, ptr, &P2PIeLen);
		FrameLen += P2PIeLen;
#endif /* WFD_SUPPORT */
	}

#ifdef WFA_WFD_SUPPORT
	if (pAd->P2pCfg.bWIDI)
	{
		if (pAd->pWfdIeInProbeReq && (pAd->WfdIeInProbeReqLen != 0))
		{
			ULONG WfdIeTmpLen = 0;
			MakeOutgoingFrame(frm_buf+FrameLen,	&WfdIeTmpLen,
						pAd->WfdIeInProbeReqLen,	pAd->pWfdIeInProbeReq,
						END_OF_ARGS);
			FrameLen += WfdIeTmpLen;
		}
	}
#endif /* WFA_WFD_SUPPORT */
#endif /* P2P_SUPPORT */
	for (i = 0; i < 3; i++)
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
			(pAd->CommonCfg.Channel == pAd->ScanCtrl.Channel))
		{

			RTMPSendNullFrame(pAd, 
						  pAd->CommonCfg.TxRate, 
						  (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
						  PWR_SAVE);
			DBGPRINT(RT_DEBUG_TRACE, ("%s():Send PWA NullData frame to notify the associated AP!\n", __FUNCTION__));
		}
	}
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
VOID ScanNextChannel(RTMP_ADAPTER *pAd, UCHAR OpMode)
{
	UCHAR ScanType = SCAN_TYPE_MAX;
	UINT ScanTimeIn5gChannel = SHORT_CHANNEL_TIME;
	BOOLEAN ScanPending = FALSE;
	RALINK_TIMER_STRUCT *sc_timer = NULL;
	UINT stay_time = 0;

#ifdef WIDI_SUPPORT	
	static int count = 0;
#endif /* WIDI_SUPPORT */


#ifdef CONFIG_ATE
	/* Nothing to do in ATE mode. */
	if (ATE_ON(pAd))
		return;
#endif /* CONFIG_ATE */

#ifdef WIDI_SUPPORT	
#ifdef CONFIG_STA_SUPPORT
	if ((pAd->StaCfg.bWIDI && (pAd->StaCfg.bSendingProbe == TRUE))
#ifdef P2P_SUPPORT
		|| (pAd->P2pCfg.bWIDI && (pAd->gP2pSendingProbeResponse == 1))
#endif /* P2P_SUPPORT */
		)
	{
		RTMPSetTimer(&pAd->MlmeAux.ScanTimer, MAX_CHANNEL_TIME);
		return;
	}
#endif /* CONFIG_STA_SUPPORT */
#endif /* WIDI_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (MONITOR_ON(pAd))
			return;
	}

	ScanPending = ((pAd->StaCfg.bImprovedScan) && (pAd->StaCfg.ScanChannelCnt>=7));
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	if (OpMode == OPMODE_AP)
		ScanType = pAd->ScanCtrl.ScanType;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (OpMode == OPMODE_STA)
		ScanType = pAd->MlmeAux.ScanType;
#endif /* CONFIG_STA_SUPPORT */
	if (ScanType == SCAN_TYPE_MAX) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s():Incorrect ScanType!\n", __FUNCTION__));
		return;
	}
#ifdef CONFIG_STA_SUPPORT
#ifdef RT_CFG80211_SUPPORT
	/* Since the Channel List is from Upper layer */
	if (CFG80211DRV_OpsScanRunning(pAd) && !ScanPending)
	{
#ifndef CCN3_TV_SUPPORT
		if (RTMP_CFG80211_VIF_P2P_GO_ON(pAd))
		{
			DBGPRINT(RT_DEBUG_OFF, ("%s():Scan Only Go Channel %d\n", __FUNCTION__, pAd->CommonCfg.Channel));
			while ((pAd->ScanCtrl.Channel =
						(UCHAR)CFG80211DRV_OpsScanGetNextChannel(pAd))
									&& pAd->CommonCfg.Channel != 0)
			{
				if(pAd->ScanCtrl.Channel == pAd->CommonCfg.Channel)
				{
					break;
				}
			}
		}
		else
		{
#endif /* ifndef CCN3_TV_SUPPORT */
			pAd->ScanCtrl.Channel = (UCHAR)CFG80211DRV_OpsScanGetNextChannel(pAd);
#ifndef CCN3_TV_SUPPORT
		}
#endif /* ifndef CCN3_TV_SUPPORT */
	}
#endif /* RT_CFG80211_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */ 
	if ((pAd->ScanCtrl.Channel == 0) || ScanPending) 
	{
#ifdef WIDI_SUPPORT
		count++;
		if (count > 10)
		{
			count = 0;
			if (ScanType != SCAN_PASSIVE)
				DBGPRINT(RT_DEBUG_ERROR, ("Driver is Alive; ScanType %d\n", ScanType));
		}
#endif /* WIDI_SUPPORT */
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
			if (pAd->StaCfg.PwrMgmt.Psm == PWR_SAVE)
				RTMP_SET_PSM_BIT(pAd, PWR_ACTIVE);
		}
#endif /* CONFIG_STA_SUPPORT */

#ifdef WIFI_REGION32_HIDDEN_SSID_SUPPORT
                if (((pAd->ScanCtrl.Channel == 12) || (pAd->ScanCtrl.Channel == 13)) &&
					((pAd->CommonCfg.CountryRegion & 0x7f) == REGION_32_BG_BAND))
                        CmdIdConfigInternalSetting(pAd, DPD_CONF, DPD_OFF);
                else
                        CmdIdConfigInternalSetting(pAd, DPD_CONF, DPD_ON);
#endif /* WIFI_REGION32_HIDDEN_SSID_SUPPORT */



		AsicSwitchChannel(pAd, pAd->ScanCtrl.Channel, TRUE);
		AsicLockChannel(pAd, pAd->ScanCtrl.Channel);

#ifdef CONFIG_STA_SUPPORT
		if (OpMode == OPMODE_STA)
		{
			BOOLEAN bScanPassive = FALSE;
			if (pAd->ScanCtrl.Channel > 14)
			{
#ifdef DPA_T
				/* All 5G channel, No matter DFS/80211H setting is y or n */
				bScanPassive = TRUE;
#else /* DPA_T */
				if ((pAd->CommonCfg.bIEEE80211H == 1)
					&& RadarChannelCheck(pAd, pAd->ScanCtrl.Channel))
					bScanPassive = TRUE;
#endif /* !DPA_T */
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
#ifdef P2P_CHANNEL_LIST_SEPARATE
		if ((pAd->ScanCtrl.ScanType == SCAN_P2P))
		{
			if (P2P_CHAN_PropertyCheck(pAd, pAd->ScanCtrl.Channel, CHANNEL_PASSIVE_SCAN) == TRUE)
				ScanType = SCAN_PASSIVE;
		}
		else
#endif /* P2P_CHANNEL_LIST_SEPARATE */
		if (CHAN_PropertyCheck(pAd, pAd->ScanCtrl.Channel, CHANNEL_PASSIVE_SCAN) == TRUE)
			ScanType = SCAN_PASSIVE;

#if defined(DPA_T) || defined(WIFI_REGION32_HIDDEN_SSID_SUPPORT)
		/* Ch 12~14 is passive scan, No matter DFS and 80211H setting is y or n */
		if ((pAd->ScanCtrl.Channel >= 12) && (pAd->ScanCtrl.Channel <= 14) 
			&& ((pAd->CommonCfg.CountryRegion & 0x7f) == REGION_32_BG_BAND))
			ScanType = SCAN_PASSIVE;
#endif /* DPA_T */

#ifdef CONFIG_AP_SUPPORT
		if (OpMode == OPMODE_AP)
			sc_timer = &pAd->ScanCtrl.APScanTimer;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		if (OpMode == OPMODE_STA)
			sc_timer = &pAd->MlmeAux.ScanTimer;
#endif /* CONFIG_STA_SUPPORT */
		if (!sc_timer) {
			DBGPRINT(RT_DEBUG_ERROR, ("%s():ScanTimer not assigned!\n", __FUNCTION__));
			return;
		}
			
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
				if (pAd->ScanCtrl.Channel > 14)
					stay_time = ScanTimeIn5gChannel;
				else
					stay_time = MIN_CHANNEL_TIME;
			}
			else
				stay_time = MAX_CHANNEL_TIME;
		}
		
		if (RTMP_CFG80211_VIF_P2P_CLI_ON(pAd)){
			stay_time = P2P_CLI_ON_SCAN_TIME;
		}
		
#ifdef CONFIG_STA_SUPPORT
#ifdef RT_CFG80211_SUPPORT
	//CFG_TODO: for testing.
	/* Since the Channel List is from Upper layer */
	if (CFG80211DRV_OpsScanRunning(pAd) && 
	    (pAd->cfg80211_ctrl.Cfg80211ChanListLen == 1))
		stay_time = 500;
#endif /* RT_CFG80211_SUPPORT */			
#endif /* CONFIG_STA_SUPPORT */
		
		RTMPSetTimer(sc_timer, stay_time);
			
		if (SCAN_MODE_ACT(ScanType))
		{
			if (pAd->MlmeAux.params.FlgScanThisSsid) {
				int i;
				for (i = 0; i < pAd->MlmeAux.params.num_ssids; ++i) {
				/* record desired BSS parameters */
				pAd->ScanCtrl.SsidLen =
					(UCHAR)pAd->MlmeAux.params.ssids[i].ssid_len;
				NdisMoveMemory(pAd->ScanCtrl.Ssid, pAd->MlmeAux.params.ssids[i].ssid, pAd->MlmeAux.params.ssids[i].ssid_len);
					if (scan_active(pAd, OpMode, ScanType) == FALSE)
						return;
				}
			} else if (scan_active(pAd, OpMode, ScanType) == FALSE)
				return;

#ifdef CONFIG_STA_SUPPORT
			if ((ScanType == SCAN_ACTIVE) &&
			   (pAd->ScanCtrl.SsidLen > 0))
			{
				/* Enhance Connectivity for Hidden Ssid Scanning */
				CHAR desiredSsid[MAX_LEN_OF_SSID], backSsid[MAX_LEN_OF_SSID];
				UCHAR desiredSsidLen, backSsidLen;

				/* For Boardcast SSID ProbeReq */
				desiredSsidLen= 0 ;
				NdisZeroMemory(desiredSsid, MAX_LEN_OF_SSID);

				//1. backup the original MlmeAux
				backSsidLen = pAd->ScanCtrl.SsidLen;
				NdisCopyMemory(backSsid, pAd->ScanCtrl.Ssid, backSsidLen);
					
				//2. fill the desried ssid into SM
				pAd->ScanCtrl.SsidLen = desiredSsidLen;
				NdisCopyMemory(pAd->ScanCtrl.Ssid, desiredSsid, desiredSsidLen);

				//3. scan action
				scan_active(pAd, OpMode, ScanType);
			
				//4. restore to ScanCtrl
				pAd->ScanCtrl.SsidLen  = backSsidLen;
				NdisCopyMemory(pAd->ScanCtrl.Ssid, backSsid, backSsidLen);
				
			}
#endif /* CONFIG_STA_SUPPORT */
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


BOOLEAN ScanRunning(RTMP_ADAPTER *pAd)
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

