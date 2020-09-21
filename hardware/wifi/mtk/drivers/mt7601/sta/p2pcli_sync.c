/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	sta_sync.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Fonchi		2006-06-23      modified for rt61-APClinent
*/

#ifdef P2P_SUPPORT

#include "rt_config.h"

#define OBSS_BEACON_RSSI_THRESHOLD		(-85)

static VOID ApCliProbeTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3);

static VOID ApCliMlmeProbeReqAction(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliPeerProbeRspAtJoinAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliProbeTimeoutAtJoinAction(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliInvalidStateWhenJoin(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliEnqueueProbeRequest(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR SsidLen,
	OUT PCHAR Ssid,
	IN USHORT ifIndex);

static VOID ApCliPeerProbeRspAction(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem);

DECLARE_TIMER_FUNCTION(ApCliProbeTimeout);
BUILD_TIMER_FUNCTION(ApCliProbeTimeout);

/*
    ==========================================================================
    Description:
        The sync state machine, 
    Parameters:
        Sm - pointer to the state machine
    Note:
        the state machine looks like the following
    ==========================================================================
 */
VOID ApCliSyncStateMachineInit(
	IN PRTMP_ADAPTER pAd,
	IN STATE_MACHINE *Sm,
	OUT STATE_MACHINE_FUNC Trans[])
{
	UCHAR i;

	StateMachineInit(Sm, (STATE_MACHINE_FUNC*)Trans,
		APCLI_MAX_SYNC_STATE, APCLI_MAX_SYNC_MSG,
		(STATE_MACHINE_FUNC)Drop, APCLI_SYNC_IDLE,
		APCLI_SYNC_MACHINE_BASE);

	/* column 1 */
	StateMachineSetAction(Sm, APCLI_SYNC_IDLE, APCLI_MT2_MLME_PROBE_REQ, (STATE_MACHINE_FUNC)ApCliMlmeProbeReqAction);
	StateMachineSetAction(Sm, APCLI_SYNC_IDLE, APCLI_MT2_PEER_PROBE_RSP, (STATE_MACHINE_FUNC)ApCliPeerProbeRspAction);

	//column 2 */
	StateMachineSetAction(Sm, APCLI_JOIN_WAIT_PROBE_RSP, APCLI_MT2_MLME_PROBE_REQ, (STATE_MACHINE_FUNC)ApCliInvalidStateWhenJoin);
	StateMachineSetAction(Sm, APCLI_JOIN_WAIT_PROBE_RSP, APCLI_MT2_PEER_PROBE_RSP, (STATE_MACHINE_FUNC)ApCliPeerProbeRspAtJoinAction);
	StateMachineSetAction(Sm, APCLI_JOIN_WAIT_PROBE_RSP, APCLI_MT2_PEER_BEACON, (STATE_MACHINE_FUNC)ApCliPeerProbeRspAtJoinAction);
	StateMachineSetAction(Sm, APCLI_JOIN_WAIT_PROBE_RSP, APCLI_MT2_PROBE_TIMEOUT, (STATE_MACHINE_FUNC)ApCliProbeTimeoutAtJoinAction);

	/* timer init */
	RTMPInitTimer(pAd, &pAd->ApCliMlmeAux.ProbeTimer, GET_TIMER_FUNCTION(ApCliProbeTimeout), pAd, FALSE);

	for (i = 0; i < MAX_APCLI_NUM; i++)
		pAd->ApCfg.ApCliTab[i].SyncCurrState = APCLI_SYNC_IDLE;

	return;
}

/* 
    ==========================================================================
    Description:
        Becaon timeout handler, executed in timer thread
    ==========================================================================
 */
static VOID ApCliProbeTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3)
{
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;

	DBGPRINT(RT_DEBUG_TRACE, ("ApCli_SYNC - ProbeReqTimeout\n"));

	MlmeEnqueue(pAd, APCLI_SYNC_STATE_MACHINE, APCLI_MT2_PROBE_TIMEOUT, 0, NULL, 0);
	RTMP_MLME_HANDLER(pAd);

	return;
}

/* 
    ==========================================================================
    Description:
        MLME PROBE req state machine procedure
    ==========================================================================
 */
static VOID ApCliMlmeProbeReqAction(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	BOOLEAN Cancelled;
	APCLI_MLME_JOIN_REQ_STRUCT *Info = (APCLI_MLME_JOIN_REQ_STRUCT *)(Elem->Msg);
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].SyncCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("ApCli SYNC - ApCliMlmeProbeReqAction(Ssid %s)\n", Info->Ssid));

	/* reset all the timers */
	RTMPCancelTimer(&pAd->ApCliMlmeAux.ProbeTimer, &Cancelled);

	pAd->ApCliMlmeAux.Rssi = -9999;
	pAd->ApCliMlmeAux.Channel = pAd->CommonCfg.Channel;
	pAd->ApCliMlmeAux.SupRateLen = pAd->CommonCfg.SupRateLen;
	NdisMoveMemory(pAd->ApCliMlmeAux.SupRate, pAd->CommonCfg.SupRate, pAd->CommonCfg.SupRateLen);

	/* Prepare the default value for extended rate */
	pAd->ApCliMlmeAux.ExtRateLen = pAd->CommonCfg.ExtRateLen;
	NdisMoveMemory(pAd->ApCliMlmeAux.ExtRate, pAd->CommonCfg.ExtRate, pAd->CommonCfg.ExtRateLen);

	RTMPSetTimer(&pAd->ApCliMlmeAux.ProbeTimer, PROBE_TIMEOUT);

	ApCliEnqueueProbeRequest(pAd, Info->SsidLen, (PCHAR) Info->Ssid, ifIndex);

	DBGPRINT(RT_DEBUG_TRACE, ("ApCli SYNC - Start Probe the SSID %s on channel =%d\n", pAd->ApCliMlmeAux.Ssid, pAd->ApCliMlmeAux.Channel));

	*pCurrState = APCLI_JOIN_WAIT_PROBE_RSP;

	return;
}

/* 
    ==========================================================================
    Description:
        When waiting joining the (I)BSS, beacon received from external
    ==========================================================================
 */
static VOID ApCliPeerProbeRspAtJoinAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	USHORT LenVIE;
	UCHAR *VarIE = NULL;
	NDIS_802_11_VARIABLE_IEs *pVIE = NULL;
	APCLI_CTRL_MSG_STRUCT ApCliCtrlMsg;
	PAPCLI_STRUCT pApCliEntry = NULL;
#ifdef DOT11_N_SUPPORT
        UCHAR CentralChannel = 0;
#endif /* DOT11_N_SUPPORT */

	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].SyncCurrState;

	BCN_IE_LIST *ie_list = NULL;

	CHAR	RealRssi = -127;
	/* Init Variable IE structure */
	os_alloc_mem(NULL, (UCHAR **)&VarIE, MAX_VIE_LEN);
	if (VarIE == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		goto LabelErr;
	}
	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;
	
	os_alloc_mem(NULL, (UCHAR **)&ie_list, sizeof(BCN_IE_LIST));
	if (ie_list == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate ie_list fail!!!\n", __FUNCTION__));
		goto LabelErr;
	}
	NdisZeroMemory(ie_list, sizeof(BCN_IE_LIST));


	

	if (PeerBeaconAndProbeRspSanity(pAd, 
								Elem->Msg, 
								Elem->MsgLen, 
								Elem->Channel,
								ie_list,
								&LenVIE,
								pVIE))
	{
		/*
			BEACON from desired BSS/IBSS found. We should be able to decide most
			BSS parameters here.
			Q. But what happen if this JOIN doesn't conclude a successful ASSOCIATEION?
				Do we need to receover back all parameters belonging to previous BSS?
			A. Should be not. There's no back-door recover to previous AP. It still need
				a new JOIN-AUTH-ASSOC sequence.
		*/
		INT ssidEqualFlag = FALSE;
		INT ssidEmptyFlag = FALSE;
		INT bssidEqualFlag = FALSE;
		INT bssidEmptyFlag = FALSE;
		INT matchFlag = FALSE;
		ULONG   Bssidx;

#ifdef P2P_SUPPORT
		MlmeEnqueue(pAd, P2P_DISC_STATE_MACHINE, P2P_DISC_PEER_PROB_RSP, Elem->MsgLen, Elem->Msg, ie_list->Channel);
#endif /* P2P_SUPPORT */
		RealRssi = RTMPMaxRssi(pAd, ConvertToRssi(pAd, Elem->Rssi0, RSSI_0,Elem->AntSel, BW_20),
					ConvertToRssi(pAd, Elem->Rssi1,RSSI_1, Elem->AntSel, BW_20), 
					ConvertToRssi(pAd, Elem->Rssi2, RSSI_2, Elem->AntSel, BW_20));


				/*
					Update ScanTab
				*/
				Bssidx = BssTableSearch(&pAd->ScanTab, ie_list->Bssid, ie_list->Channel);
				if (Bssidx == BSS_NOT_FOUND)
				{
					/* discover new AP of this network, create BSS entry */
					Bssidx = BssTableSetEntry(pAd, &pAd->ScanTab, ie_list, -127, LenVIE, pVIE);
					
					if (Bssidx == BSS_NOT_FOUND) /* return if BSS table full */
					{
						printk("ERROR: Driver ScanTable Full In Apcli ProbeRsp Join\n");
						goto LabelErr;
					}

					NdisMoveMemory(pAd->ScanTab.BssEntry[Bssidx].PTSF, &Elem->Msg[24], 4);
					NdisMoveMemory(&pAd->ScanTab.BssEntry[Bssidx].TTSF[0], &Elem->TimeStamp.u.LowPart, 4);
					NdisMoveMemory(&pAd->ScanTab.BssEntry[Bssidx].TTSF[4], &Elem->TimeStamp.u.LowPart, 4);
					pAd->ScanTab.BssEntry[Bssidx].MinSNR = Elem->Signal % 10;
					if (pAd->ScanTab.BssEntry[Bssidx].MinSNR == 0)
						pAd->ScanTab.BssEntry[Bssidx].MinSNR = -5;
					
					NdisMoveMemory(pAd->ScanTab.BssEntry[Bssidx].MacAddr, ie_list->Addr2, MAC_ADDR_LEN);
				}

#ifdef RT_CFG80211_SUPPORT
                                        printk("Info: Update the SSID %s in Kernel Table\n", ie_list->Ssid);
                                        RT_CFG80211_SCANNING_INFORM(pAd, Bssidx, ie_list->Channel, (UCHAR *)Elem->Msg, Elem->MsgLen, RealRssi);
#endif /* RT_CFG80211_SUPPORT */

		pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

		/* Check the Probe-Rsp's Bssid. */
		if(!MAC_ADDR_EQUAL(pApCliEntry->CfgApCliBssid, ZERO_MAC_ADDR))
			bssidEqualFlag = MAC_ADDR_EQUAL(pApCliEntry->CfgApCliBssid, ie_list->Bssid);
		else
			bssidEmptyFlag = TRUE;
		/* Check the Probe-Rsp's Ssid. */
		if(pApCliEntry->CfgSsidLen != 0)
			ssidEqualFlag = SSID_EQUAL(pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen, ie_list->Ssid, ie_list->SsidLen);
		else
			ssidEmptyFlag = TRUE;


		/* bssid and ssid, Both match. */
		if (bssidEqualFlag && ssidEqualFlag)
			matchFlag = TRUE;

		/* ssid match but bssid doesn't be indicate. */
		else if(ssidEqualFlag && bssidEmptyFlag)
			matchFlag = TRUE;

		/* user doesn't indicate any bssid or ssid. AP-Clinet will auto pick a AP to join by most strong siganl strength. */
		else if (bssidEmptyFlag && ssidEmptyFlag)
			matchFlag = TRUE;


		DBGPRINT(RT_DEBUG_TRACE, ("SYNC - bssidEqualFlag=%d, ssidEqualFlag=%d, matchFlag=%d\n", bssidEqualFlag, ssidEqualFlag, matchFlag));
		if (matchFlag)
		{
			/* Validate RSN IE if necessary, then copy store this information */
			if ((LenVIE > 0) 
#ifdef WSC_AP_SUPPORT
                && ((pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscConfMode == WSC_DISABLE) || 
                	(pAd->ApCfg.ApCliTab[ifIndex].WscControl.bWscTrigger == FALSE))
#endif /* WSC_AP_SUPPORT */
#ifdef RT_CFG80211_SUPPORT
				/* When using CFG80211 and trigger WPS, do not check security. */
				&& ! (pAd->ApCfg.ApCliTab[MAIN_MBSSID].WpaSupplicantUP & WPA_SUPPLICANT_ENABLE_WPS)
#endif /* RT_CFG80211_SUPPORT */
                )
			{
				if (ApCliValidateRSNIE(pAd, (PEID_STRUCT)pVIE, LenVIE, ifIndex))
				{
					pAd->ApCliMlmeAux.VarIELen = LenVIE;
					NdisMoveMemory(pAd->ApCliMlmeAux.VarIEs, pVIE, pAd->ApCliMlmeAux.VarIELen);
				}
				else
				{
					/* ignore this response */
					pAd->ApCliMlmeAux.VarIELen = 0;
					DBGPRINT(RT_DEBUG_ERROR, ("ERROR: The RSN IE of this received Probe-resp is dis-match !!!!!!!!!! \n"));
					goto LabelErr;
				}
			}
			else
			{
				if (pApCliEntry->AuthMode >= Ndis802_11AuthModeWPA
#ifdef WSC_AP_SUPPORT
                    && ((pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscConfMode == WSC_DISABLE) || 
                		(pAd->ApCfg.ApCliTab[ifIndex].WscControl.bWscTrigger == FALSE))
#endif /* WSC_AP_SUPPORT */
                    )
				{
					/* ignore this response */
					DBGPRINT(RT_DEBUG_ERROR, ("ERROR: The received Probe-resp has empty RSN IE !!!!!!!!!! \n"));
					goto LabelErr;
				}	
				
				pAd->ApCliMlmeAux.VarIELen = 0;
			}

			DBGPRINT(RT_DEBUG_TRACE, ("SYNC - receive desired PROBE_RSP at JoinWaitProbeRsp... Channel = %d\n", ie_list->Channel));

			/* if the Bssid doesn't be indicated then you need to decide which AP to connect by most strong Rssi signal strength. */
			if (bssidEqualFlag == FALSE)
			{
				/* caculate real rssi value. */
				CHAR Rssi0 = ConvertToRssi(pAd, Elem->Rssi0, RSSI_0, Elem->AntSel, BW_20);
				CHAR Rssi1 = ConvertToRssi(pAd, Elem->Rssi1, RSSI_1, Elem->AntSel, BW_20);
				CHAR Rssi2 = ConvertToRssi(pAd, Elem->Rssi2, RSSI_2, Elem->AntSel, BW_20);
				LONG RealRssi = (LONG)(RTMPMaxRssi(pAd, Rssi0, Rssi1, Rssi2));

				DBGPRINT(RT_DEBUG_TRACE, ("SYNC - previous Rssi = %ld current Rssi=%ld\n", pAd->ApCliMlmeAux.Rssi, (LONG)RealRssi));
				if (pAd->ApCliMlmeAux.Rssi > (LONG)RealRssi)
					goto LabelErr;
				else
					pAd->ApCliMlmeAux.Rssi = RealRssi;
			}
			else
			{
				BOOLEAN Cancelled;
				RTMPCancelTimer(&pAd->ApCliMlmeAux.ProbeTimer, &Cancelled);

				if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
				{
				/* Stop Scan and resume */
				RTMPCancelTimer(&pAd->MlmeAux.ScanTimer, &Cancelled);
				pAd->MlmeAux.Channel = 0;
				ScanNextChannel(pAd, OPMODE_AP);
				}

				DBGPRINT(RT_DEBUG_TRACE, ("%s::  Swich Channel = %d. and STOP Scanning!!\n", __FUNCTION__, ie_list->Channel));
			}

			NdisMoveMemory(pAd->ApCliMlmeAux.Ssid, ie_list->Ssid, ie_list->SsidLen);
			pAd->ApCliMlmeAux.SsidLen = ie_list->SsidLen;

			NdisMoveMemory(pAd->ApCliMlmeAux.Bssid, ie_list->Bssid, MAC_ADDR_LEN);			
			pAd->ApCliMlmeAux.CapabilityInfo = ie_list->CapabilityInfo & SUPPORTED_CAPABILITY_INFO;
			pAd->ApCliMlmeAux.BssType = ie_list->BssType;
			pAd->ApCliMlmeAux.BeaconPeriod = ie_list->BeaconPeriod;
			pAd->ApCliMlmeAux.Channel = ie_list->Channel;
			pAd->ApCliMlmeAux.AtimWin = ie_list->AtimWin;
			pAd->ApCliMlmeAux.CfpPeriod = ie_list->CfParm.CfpPeriod;
			pAd->ApCliMlmeAux.CfpMaxDuration = ie_list->CfParm.CfpMaxDuration;
			pAd->ApCliMlmeAux.APRalinkIe = ie_list->RalinkIe;

			/* Copy AP's supported rate to MlmeAux for creating assoication request */
			/* Also filter out not supported rate */
			pAd->ApCliMlmeAux.SupRateLen = ie_list->SupRateLen;
			NdisMoveMemory(pAd->ApCliMlmeAux.SupRate, ie_list->SupRate, ie_list->SupRateLen);
			RTMPCheckRates(pAd, pAd->ApCliMlmeAux.SupRate, &pAd->ApCliMlmeAux.SupRateLen);
			pAd->ApCliMlmeAux.ExtRateLen = ie_list->ExtRateLen;
			NdisMoveMemory(pAd->ApCliMlmeAux.ExtRate, ie_list->ExtRate, ie_list->ExtRateLen);
			RTMPCheckRates(pAd, pAd->ApCliMlmeAux.ExtRate, &pAd->ApCliMlmeAux.ExtRateLen);

#ifdef DOT11_N_SUPPORT
			NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].RxMcsSet,sizeof(pAd->ApCfg.ApCliTab[ifIndex].RxMcsSet));
			/* filter out un-supported ht rates */
			if ((ie_list->HtCapabilityLen > 0) && 
				(pApCliEntry->DesiredHtPhyInfo.bHtEnable) &&
				WMODE_CAP_N(pAd->CommonCfg.PhyMode))
			{
#ifdef P2P_SUPPORT				
				BOOLEAN P2PGroup_BW;				
				UCHAR BwFallBack = 0;
#endif /* P2P_SUPPORT */

				RTMPZeroMemory(&pAd->ApCliMlmeAux.HtCapability, SIZE_HT_CAP_IE);
				pAd->ApCliMlmeAux.NewExtChannelOffset = ie_list->NewExtChannelOffset;
				pAd->ApCliMlmeAux.HtCapabilityLen = ie_list->HtCapabilityLen;
				ApCliCheckHt(pAd, ifIndex, &ie_list->HtCapability, &ie_list->AddHtInfo);
				RTMPMoveMemory(&pAd->ApCliMlmeAux.AddHtInfo, &ie_list->AddHtInfo, SIZE_ADD_HT_INFO_IE);

				if (ie_list->AddHtInfoLen > 0)
				{
					CentralChannel = ie_list->AddHtInfo.ControlChan;
		 			/* Check again the Bandwidth capability of this AP. */
					CentralChannel = get_cent_ch_by_htinfo(pAd, &ie_list->AddHtInfo, &ie_list->HtCapability);
		 			DBGPRINT(RT_DEBUG_TRACE, ("PeerBeaconAtJoinAction HT===>Central Channel = %d, Control Channel = %d,  .\n", CentralChannel, ie_list->AddHtInfo.ControlChan));
				}
#ifdef P2P_SUPPORT
				if (INFRA_ON(pAd) && (pAd->CommonCfg.CentralChannel != CentralChannel) && (pAd->StaActive.SupportedHtPhy.ChannelWidth == BW_40))
				{
					DBGPRINT(RT_DEBUG_OFF, ("PeerBeaconAtJoinAction HT===> Channel offset = %d not match INFRA Channel offset %d .\n",
								pAd->MlmeAux.CentralChannel, CentralChannel));
					//goto LabelErr;
				}

				/*P2PChannelInit(pAd, MAIN_MBSSID); */
				pAd->ApCliMlmeAux.CentralChannel = CentralChannel;
				//P2PInitChannelRelatedValue(pAd);
				if (pAd->ApCliMlmeAux.HtCapability.HtCapInfo.ChannelWidth == BW_40)
					P2PGroup_BW = TRUE;
				else
					P2PGroup_BW = FALSE;

				AdjustChannelRelatedValue(pAd,
											&BwFallBack,
											ifIndex,
											P2PGroup_BW,
											pAd->ApCliMlmeAux.Channel,
											pAd->ApCliMlmeAux.CentralChannel);

				if (BwFallBack == 1)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("Infra STA connection to 40MHz AP, but Infra extra and P2P Group extra is different!!!\n"));
					pAd->ApCliMlmeAux.HtCapability.HtCapInfo.ChannelWidth = BW_20;
					pAd->ApCliMlmeAux.CentralChannel = pAd->ApCliMlmeAux.Channel;
					pAd->ApCliMlmeAux.bBwFallBack = TRUE;
				}
				else
				{
					pAd->ApCliMlmeAux.bBwFallBack = FALSE;
				}

				pAd->ApCliMlmeAux.ConCurrentCentralChannel = pAd->CommonCfg.CentralChannel;
#endif /* P2P_SUPPORT */
			}
			else
#endif /* DOT11_N_SUPPORT */
			{
				RTMPZeroMemory(&pAd->ApCliMlmeAux.HtCapability, SIZE_HT_CAP_IE);
				RTMPZeroMemory(&pAd->ApCliMlmeAux.AddHtInfo, SIZE_ADD_HT_INFO_IE);
				pAd->ApCliMlmeAux.HtCapabilityLen = 0;
			}
#ifdef P2P_SUPPORT
			P2PUpdateMlmeRate(pAd);
#else
			RTMPUpdateMlmeRate(pAd);
#endif /* P2P_SUPPORT */

#ifdef DOT11_N_SUPPORT
			/* copy QOS related information */
			if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
			{
				NdisMoveMemory(&pAd->ApCliMlmeAux.APEdcaParm, &ie_list->EdcaParm, sizeof(EDCA_PARM));
				NdisMoveMemory(&pAd->ApCliMlmeAux.APQbssLoad, &ie_list->QbssLoad, sizeof(QBSS_LOAD_PARM));
				NdisMoveMemory(&pAd->ApCliMlmeAux.APQosCapability, &ie_list->QosCapability, sizeof(QOS_CAPABILITY_PARM));
			}
			else
#endif /* DOT11_N_SUPPORT */
			{
				NdisZeroMemory(&pAd->ApCliMlmeAux.APEdcaParm, sizeof(EDCA_PARM));
				NdisZeroMemory(&pAd->ApCliMlmeAux.APQbssLoad, sizeof(QBSS_LOAD_PARM));
				NdisZeroMemory(&pAd->ApCliMlmeAux.APQosCapability, sizeof(QOS_CAPABILITY_PARM));
			}

			DBGPRINT(RT_DEBUG_TRACE, ("APCLI SYNC - after JOIN, SupRateLen=%d, ExtRateLen=%d\n", 
				pAd->ApCliMlmeAux.SupRateLen, pAd->ApCliMlmeAux.ExtRateLen));

			if (ie_list->AironetCellPowerLimit != 0xFF)
			{
				/*We need to change our TxPower for CCX 2.0 AP Control of Client Transmit Power */
				ChangeToCellPowerLimit(pAd, ie_list->AironetCellPowerLimit);
			}
			else  /*Used the default TX Power Percentage. */
				pAd->CommonCfg.TxPowerPercentage = pAd->CommonCfg.TxPowerDefault;
			if(bssidEqualFlag == TRUE)
			{
				*pCurrState = APCLI_SYNC_IDLE;

				ApCliCtrlMsg.Status = MLME_SUCCESS;
				MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_PROBE_RSP,
					sizeof(APCLI_CTRL_MSG_STRUCT), &ApCliCtrlMsg, ifIndex);
			}
		}
		/* not to me BEACON, ignored */
	}
	/* sanity check fail, ignore this frame */

LabelErr:
	if (VarIE != NULL)
		os_free_mem(NULL, VarIE);
	if (ie_list != NULL)
		os_free_mem(NULL, ie_list);

	return;
}

static VOID ApCliProbeTimeoutAtJoinAction(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem) 
{
	APCLI_CTRL_MSG_STRUCT ApCliCtrlMsg;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].SyncCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("APCLI_SYNC - ProbeTimeoutAtJoinAction\n"));
	*pCurrState = SYNC_IDLE;

	DBGPRINT(RT_DEBUG_TRACE, ("APCLI_SYNC - ApCliMlmeAux.Bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
				pAd->ApCliMlmeAux.Bssid[0],
				pAd->ApCliMlmeAux.Bssid[1],
				pAd->ApCliMlmeAux.Bssid[2],
				pAd->ApCliMlmeAux.Bssid[3],
				pAd->ApCliMlmeAux.Bssid[4],
				pAd->ApCliMlmeAux.Bssid[5]));

	if(!MAC_ADDR_EQUAL(pAd->ApCliMlmeAux.Bssid, ZERO_MAC_ADDR))
	{
		ApCliCtrlMsg.Status = MLME_SUCCESS;
		MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_PROBE_RSP,
			sizeof(APCLI_CTRL_MSG_STRUCT), &ApCliCtrlMsg, ifIndex);
	} else
	{
		MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_JOIN_REQ_TIMEOUT, 0, NULL, ifIndex);
	}

	return;
}

/* 
    ==========================================================================
    Description:
    ==========================================================================
 */
static VOID ApCliInvalidStateWhenJoin(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	APCLI_CTRL_MSG_STRUCT ApCliCtrlMsg;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].SyncCurrState;

	*pCurrState = APCLI_SYNC_IDLE;
	ApCliCtrlMsg.Status = MLME_STATE_MACHINE_REJECT;
	MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_PROBE_RSP,
		sizeof(APCLI_CTRL_MSG_STRUCT), &ApCliCtrlMsg, ifIndex);

	DBGPRINT(RT_DEBUG_TRACE, ("APCLI_AYNC - ApCliInvalidStateWhenJoin(state=%ld). Reset SYNC machine\n", *pCurrState));

	return;
}

/* 
	==========================================================================
	Description:
	==========================================================================
 */
static VOID ApCliEnqueueProbeRequest(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR SsidLen,
	OUT PCHAR Ssid,
	IN USHORT ifIndex)
{
	NDIS_STATUS     NState;
	PUCHAR          pOutBuffer;
	ULONG           FrameLen = 0;
	HEADER_802_11   Hdr80211;
	UCHAR           SsidIe    = IE_SSID;
	UCHAR           SupRateIe = IE_SUPP_RATES;
	UCHAR ssidLen;
	CHAR ssid[MAX_LEN_OF_SSID];
#if !defined(RT_CFG80211_SUPPORT)
	PUCHAR	ptr;
	ULONG			TmpLen;
#endif


	DBGPRINT(RT_DEBUG_TRACE, ("force out a ProbeRequest ...\n"));

	
	NState = MlmeAllocateMemory(pAd, &pOutBuffer);  //Get an unused nonpaged memory */
	if(NState != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("EnqueueProbeRequest() allocate memory fail\n"));
		return;
	}
	else
	{
		if(MAC_ADDR_EQUAL(pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid, ZERO_MAC_ADDR))
			ApCliMgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0,
				BROADCAST_ADDR, BROADCAST_ADDR, ifIndex);
		else
			ApCliMgtMacHeaderInit(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0,
				pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid, pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid, ifIndex);

		ssidLen = SsidLen;
		NdisZeroMemory(ssid, MAX_LEN_OF_SSID);
		NdisMoveMemory(ssid, Ssid, ssidLen);

		/* this ProbeRequest explicitly specify SSID to reduce unwanted ProbeResponse */
		MakeOutgoingFrame(pOutBuffer,		&FrameLen,
			sizeof(HEADER_802_11),			&Hdr80211,
			1,								&SsidIe,
			1,								&ssidLen,
			ssidLen,						ssid,
			1,								&SupRateIe,
			1,								&pAd->ApCliMlmeAux.SupRateLen,
			pAd->ApCliMlmeAux.SupRateLen,		pAd->ApCliMlmeAux.SupRate,
			END_OF_ARGS);

		/* Add the extended rate IE */
		if (pAd->ApCliMlmeAux.ExtRateLen != 0)
		{
			ULONG            tmp;
		
			MakeOutgoingFrame(pOutBuffer + FrameLen,    &tmp,
				1,                        &ExtRateIe,
				1,                        &pAd->ApCliMlmeAux.ExtRateLen,
				pAd->ApCliMlmeAux.ExtRateLen,  pAd->ApCliMlmeAux.ExtRate,                           
				END_OF_ARGS);
			FrameLen += tmp;
		}

//fix to carry supplicant's extra IE like P2P & WFD 
#ifdef RT_CFG80211_SUPPORT
		if ((pAd->StaCfg.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE) &&
			(pAd->StaCfg.WpsProbeReqIeLen != 0))
		{
			ULONG 		WpsTmpLen = 0;
			
			MakeOutgoingFrame(pOutBuffer + FrameLen,              &WpsTmpLen,
							pAd->StaCfg.WpsProbeReqIeLen,	pAd->StaCfg.pWpsProbeReqIe,
							END_OF_ARGS);
			FrameLen += WpsTmpLen;
		}
#else /* RT_CFG80211_SUPPORT */
		P2pMakeProbeRspWSCIE(pAd, pOutBuffer + FrameLen, &TmpLen);
		FrameLen += TmpLen;
	
		ptr = pOutBuffer + FrameLen;
		P2pMakeP2pIE(pAd, SUBTYPE_PROBE_REQ, ptr, &TmpLen);
		FrameLen += TmpLen;
#endif /* !RT_CFG80211_SUPPORT */

		MiniportMMRequest(pAd, QID_AC_BE, pOutBuffer, FrameLen);
		MlmeFreeMemory(pAd, pOutBuffer);
	}

	return;
}

BOOLEAN ApCliWaitProbRsp(
	IN PRTMP_ADAPTER pAd,
	IN USHORT ifIndex)
{
	if (ifIndex >= MAX_APCLI_NUM)
		return FALSE;

	return (pAd->ApCfg.ApCliTab[ifIndex].SyncCurrState == APCLI_JOIN_WAIT_PROBE_RSP) ?
		TRUE : FALSE;
}

/*
    ==========================================================================
    Description:
        peer sends beacon back when scanning
    ==========================================================================
 */
VOID ApCliPeerProbeRspAction(
	IN PRTMP_ADAPTER pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
    PFRAME_802_11   pFrame;

	UCHAR			*VarIE = NULL;
    USHORT          LenVIE;
	NDIS_802_11_VARIABLE_IEs	*pVIE = NULL;

	CHAR			RealRssi = -127;

	BCN_IE_LIST *ie_list = NULL;


	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&VarIE, MAX_VIE_LEN);
	if (VarIE == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		goto LabelErr;
	}
	/* Init Variable IE structure */
	pVIE = (PNDIS_802_11_VARIABLE_IEs) VarIE;
	pVIE->Length = 0;

	os_alloc_mem(NULL, (UCHAR **)&ie_list, sizeof(BCN_IE_LIST));
	if (ie_list == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate ie_list fail!!!\n", __FUNCTION__));
		goto LabelErr;
	}
	NdisZeroMemory(ie_list, sizeof(BCN_IE_LIST));

	pFrame = (PFRAME_802_11) Elem->Msg;

	if (PeerBeaconAndProbeRspSanity(pAd, 
								Elem->Msg, 
								Elem->MsgLen, 
								Elem->Channel,
								ie_list,
								&LenVIE,
								pVIE))
    {
		ULONG Idx;
		CHAR  Rssi = -127;

		RealRssi = RTMPMaxRssi(pAd, ConvertToRssi(pAd, Elem->Rssi0, RSSI_0, Elem->AntSel, BW_20), ConvertToRssi(pAd, Elem->Rssi1, RSSI_1, Elem->AntSel, BW_20), ConvertToRssi(pAd, Elem->Rssi2, RSSI_2, Elem->AntSel, BW_20));

#ifdef P2P_SUPPORT
		MlmeEnqueue(pAd, P2P_DISC_STATE_MACHINE, P2P_DISC_PEER_PROB_RSP, Elem->MsgLen, Elem->Msg, ie_list->Channel);
#endif /* P2P_SUPPORT */

		
		/* ignore BEACON not in this channel */
		if (ie_list->Channel != pAd->MlmeAux.Channel
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
			&& (pAd->CommonCfg.bOverlapScanning == FALSE)
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
		   )
		{
			goto __End_Of_APPeerBeaconAtScanAction;
		}

#ifdef DOT11_N_SUPPORT
   		if ((RealRssi > OBSS_BEACON_RSSI_THRESHOLD) && (ie_list->HtCapability.HtCapInfo.Forty_Mhz_Intolerant)) /* || (HtCapabilityLen == 0))) */
		{
			Handle_BSS_Width_Trigger_Events(pAd);
		}
#endif /* DOT11_N_SUPPORT */

#ifdef IDS_SUPPORT
		/* Conflict SSID detection */
		if (ie_list->Channel == pAd->CommonCfg.Channel)
			RTMPConflictSsidDetection(pAd, ie_list->Ssid, ie_list->SsidLen, Elem->Rssi0, Elem->Rssi1, Elem->Rssi2, Elem->AntSel);
#endif /* IDS_SUPPORT */

		/*
			This correct im-proper RSSI indication during SITE SURVEY issue.
			Always report bigger RSSI during SCANNING when receiving multiple BEACONs from the same AP.
			This case happens because BEACONs come from adjacent channels, so RSSI become weaker as we
			switch to more far away channels.
		*/
        Idx = BssTableSearch(&pAd->ScanTab, ie_list->Bssid, ie_list->Channel);
		if (Idx != BSS_NOT_FOUND)
		{
			if (Idx >= MAX_LEN_OF_BSS_TABLE)
			{
				ASSERT(0);
				goto __End_Of_APPeerBeaconAtScanAction;
			}
            Rssi = pAd->ScanTab.BssEntry[Idx].Rssi;
		}
		

        /* TODO: 2005-03-04 dirty patch. we should change all RSSI related variables to SIGNED SHORT for easy/efficient reading and calaulation */
		RealRssi = RTMPMaxRssi(pAd, ConvertToRssi(pAd, Elem->Rssi0, RSSI_0, Elem->AntSel, BW_20), ConvertToRssi(pAd, Elem->Rssi1, RSSI_1, Elem->AntSel, BW_20), ConvertToRssi(pAd, Elem->Rssi2, RSSI_2, Elem->AntSel, BW_20));
        if ((RealRssi + pAd->BbpRssiToDbmDelta) > Rssi)
            Rssi = RealRssi + pAd->BbpRssiToDbmDelta;

		Idx = BssTableSetEntry(pAd, &pAd->ScanTab, ie_list, -Rssi, LenVIE, pVIE);
		
		if (Idx != BSS_NOT_FOUND)
		{
			NdisMoveMemory(pAd->ScanTab.BssEntry[Idx].PTSF, &Elem->Msg[24], 4);
			NdisMoveMemory(&pAd->ScanTab.BssEntry[Idx].TTSF[0], &Elem->TimeStamp.u.LowPart, 4);
			NdisMoveMemory(&pAd->ScanTab.BssEntry[Idx].TTSF[4], &Elem->TimeStamp.u.LowPart, 4);
		}
	}

	/* sanity check fail, ignored */
__End_Of_APPeerBeaconAtScanAction:
	/*scan beacon in pastive */
#ifdef CONFIG_AP_SUPPORT
IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
{
	if (ie_list->Channel == pAd->ApCfg.AutoChannel_Channel)
	{
		if (AutoChBssSearchWithSSID(pAd, ie_list->Bssid, (PUCHAR)ie_list->Ssid, ie_list->SsidLen, ie_list->Channel) == BSS_NOT_FOUND)
			pAd->pChannelInfo->ApCnt[pAd->ApCfg.current_channel_index]++;

		AutoChBssInsertEntry(pAd, ie_list->Bssid, (CHAR *)ie_list->Ssid, ie_list->SsidLen, ie_list->Channel, ie_list->NewExtChannelOffset, RealRssi);   
	}
}
#endif /* CONFIG_AP_SUPPORT */
LabelErr:
	if (VarIE != NULL)
		os_free_mem(NULL, VarIE);
	if (ie_list != NULL)
		os_free_mem(NULL, ie_list);
}

#endif /* P2P_SUPPORT */

