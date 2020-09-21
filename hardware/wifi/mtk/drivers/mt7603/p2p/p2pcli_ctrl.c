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
	apcli_ctrl.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Fonchi		2006-06-23      modified for rt61-APClinent
*/
#ifdef P2P_SUPPORT

#include "rt_config.h"


static VOID ApCliCtrlJoinReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlJoinReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlProbeRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlAuthRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlAuth2RspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlAuthReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlAuth2ReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlAssocRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlDeAssocRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlAssocReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlDisconnectReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlPeerDeAssocReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlDeAssocAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

static VOID ApCliCtrlDeAuthAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

/*
    ==========================================================================
    Description:
        The apcli ctrl state machine, 
    Parameters:
        Sm - pointer to the state machine
    Note:
        the state machine looks like the following
    ==========================================================================
 */
VOID ApCliCtrlStateMachineInit(
	IN PRTMP_ADAPTER pAd,
	IN STATE_MACHINE *Sm,
	OUT STATE_MACHINE_FUNC Trans[])
{
	UCHAR i;

	StateMachineInit(Sm, (STATE_MACHINE_FUNC*)Trans,
		APCLI_MAX_CTRL_STATE, APCLI_MAX_CTRL_MSG,
		(STATE_MACHINE_FUNC)Drop, APCLI_CTRL_DISCONNECTED,
		APCLI_CTRL_MACHINE_BASE);

	/* disconnected state */
	StateMachineSetAction(Sm, APCLI_CTRL_DISCONNECTED, APCLI_CTRL_JOIN_REQ, (STATE_MACHINE_FUNC)ApCliCtrlJoinReqAction);

	/* probe state */
	StateMachineSetAction(Sm, APCLI_CTRL_PROBE, APCLI_CTRL_PROBE_RSP, (STATE_MACHINE_FUNC)ApCliCtrlProbeRspAction);
	StateMachineSetAction(Sm, APCLI_CTRL_PROBE, APCLI_CTRL_JOIN_REQ_TIMEOUT, (STATE_MACHINE_FUNC)ApCliCtrlJoinReqTimeoutAction);
	StateMachineSetAction(Sm, APCLI_CTRL_PROBE, APCLI_CTRL_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlDisconnectReqAction);

	/* auth state */
	StateMachineSetAction(Sm, APCLI_CTRL_AUTH, APCLI_CTRL_AUTH_RSP, (STATE_MACHINE_FUNC)ApCliCtrlAuthRspAction);
	StateMachineSetAction(Sm, APCLI_CTRL_AUTH, APCLI_CTRL_AUTH_REQ_TIMEOUT, (STATE_MACHINE_FUNC)ApCliCtrlAuthReqTimeoutAction);
	StateMachineSetAction(Sm, APCLI_CTRL_AUTH, APCLI_CTRL_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlDisconnectReqAction);
 	StateMachineSetAction(Sm, APCLI_CTRL_AUTH, APCLI_CTRL_PEER_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlPeerDeAssocReqAction);

	/* auth2 state */
	StateMachineSetAction(Sm, APCLI_CTRL_AUTH_2, APCLI_CTRL_AUTH_RSP, (STATE_MACHINE_FUNC)ApCliCtrlAuth2RspAction);
	StateMachineSetAction(Sm, APCLI_CTRL_AUTH_2, APCLI_CTRL_AUTH_REQ_TIMEOUT, (STATE_MACHINE_FUNC)ApCliCtrlAuth2ReqTimeoutAction);
	StateMachineSetAction(Sm, APCLI_CTRL_AUTH_2, APCLI_CTRL_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlDisconnectReqAction);
 	StateMachineSetAction(Sm, APCLI_CTRL_AUTH_2, APCLI_CTRL_PEER_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlPeerDeAssocReqAction);

	/* assoc state */
	StateMachineSetAction(Sm, APCLI_CTRL_ASSOC, APCLI_CTRL_ASSOC_RSP, (STATE_MACHINE_FUNC)ApCliCtrlAssocRspAction);
	StateMachineSetAction(Sm, APCLI_CTRL_ASSOC, APCLI_CTRL_ASSOC_REQ_TIMEOUT, (STATE_MACHINE_FUNC)ApCliCtrlAssocReqTimeoutAction);
	StateMachineSetAction(Sm, APCLI_CTRL_ASSOC, APCLI_CTRL_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlDeAssocAction);
 	StateMachineSetAction(Sm, APCLI_CTRL_ASSOC, APCLI_CTRL_PEER_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlPeerDeAssocReqAction);

	/* deassoc state */
	StateMachineSetAction(Sm, APCLI_CTRL_DEASSOC, APCLI_CTRL_DEASSOC_RSP, (STATE_MACHINE_FUNC)ApCliCtrlDeAssocRspAction);

	/* connected state */
	StateMachineSetAction(Sm, APCLI_CTRL_CONNECTED, APCLI_CTRL_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlDeAuthAction);
 	StateMachineSetAction(Sm, APCLI_CTRL_CONNECTED, APCLI_CTRL_PEER_DISCONNECT_REQ, (STATE_MACHINE_FUNC)ApCliCtrlPeerDeAssocReqAction);

	for (i = 0; i < MAX_APCLI_NUM; i++)
		pAd->ApCfg.ApCliTab[i].CtrlCurrState = APCLI_CTRL_DISCONNECTED;

	return;
}


/* 
    ==========================================================================
    Description:
        APCLI MLME JOIN req state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlJoinReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	APCLI_MLME_JOIN_REQ_STRUCT JoinReq;
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;
#ifdef WSC_AP_SUPPORT
	PWSC_CTRL	pWpsCtrl = &pAd->ApCfg.ApCliTab[ifIndex].WscControl;
#endif /* WSC_AP_SUPPORT */
#ifdef CONFIG_MULTI_CHANNEL
	BOOLEAN bChangeInitBW = FALSE;
	UCHAR BBPValue;
#endif /*CONFIG_MULTI_CHANNEL*/

	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Start Probe Req.\n", __FUNCTION__));
	if (ifIndex >= MAX_APCLI_NUM)
		return;

	if (ApScanRunning(pAd) == TRUE)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	NdisZeroMemory(&JoinReq, sizeof(APCLI_MLME_JOIN_REQ_STRUCT));

	if (!MAC_ADDR_EQUAL(pApCliEntry->CfgApCliBssid, ZERO_MAC_ADDR))
	{
		COPY_MAC_ADDR(JoinReq.Bssid, pApCliEntry->CfgApCliBssid);
	}

#ifdef WSC_AP_SUPPORT
    if ((pWpsCtrl->WscConfMode != WSC_DISABLE) &&
		(pWpsCtrl->bWscTrigger == TRUE))
    {
    	ULONG bss_idx = 0;
        NdisZeroMemory(JoinReq.Ssid, MAX_LEN_OF_SSID);
        JoinReq.SsidLen = pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscSsid.SsidLength;
		NdisMoveMemory(JoinReq.Ssid, pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscSsid.Ssid, JoinReq.SsidLen);
#if 0
		if (pWpsCtrl->WscMode == 1) /* PIN */
#endif
		{
#if 0
			bss_idx = BssSsidTableSearchBySSID(&pAd->ScanTab, (PCHAR)JoinReq.Ssid, JoinReq.SsidLen);
#endif
			bss_idx = BssSsidTableSearch(&pAd->ScanTab, pAd->P2pCfg.Bssid, (PCHAR)JoinReq.Ssid, JoinReq.SsidLen, pAd->CommonCfg.Channel);
			if (bss_idx == BSS_NOT_FOUND)
			{
				*pCurrState = APCLI_CTRL_PROBE;
				ApSiteSurvey(pAd, NULL, SCAN_WSC_ACTIVE, FALSE);
				return;
			}
			else
			{
				BOOLEAN Cancelled;
				INT old_conf_mode = pWpsCtrl->WscConfMode, old_wsc_mode = pWpsCtrl->WscMode;
				ADD_HTINFO	RootApHtInfo, ApHtInfo;
				UCHAR channel = pAd->CommonCfg.Channel, RootApChannel = pAd->ScanTab.BssEntry[bss_idx].Channel;
				UCHAR RootApCentralChannel = pAd->ScanTab.BssEntry[bss_idx].CentralChannel;
				ApHtInfo = pAd->CommonCfg.AddHTInfo.AddHtInfo;
				RootApHtInfo = pAd->ScanTab.BssEntry[bss_idx].AddHtInfo.AddHtInfo;
				
#if 0
				/* Stop Scan and resume */
				RTMPCancelTimer(&pAd->MlmeAux.ScanTimer, &Cancelled);
				pAd->MlmeAux.Channel = 0;
				ScanNextChannel(pAd, OPMODE_AP);
#endif
				if (INFRA_ON(pAd) && (pAd->StaActive.SupportedHtPhy.ExtChanOffset != RootApHtInfo.ExtChanOffset) 
					&& (pAd->StaActive.SupportedHtPhy.ChannelWidth == BW_40)
#ifdef CONFIG_MULTI_CHANNEL
 && (pAd->Multi_Channel_Enable == FALSE)
#endif /* CONFIG_MULTI_CHANNEL */
)
				{
					DBGPRINT(RT_DEBUG_OFF, ("%s :: Channel offset = %d not match INFRA Channel offset %d .\n",
								__FUNCTION__, pAd->MlmeAux.CentralChannel, RootApCentralChannel));
					P2pLinkDown(pAd, P2P_CONNECT_FAIL);
					return;
				}

#ifdef CONFIG_MULTI_CHANNEL
					if (pAd->Multi_Channel_Enable == TRUE)
					{
							pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
							pAd->ApCfg.ApCliTab[0].Channel = RootApChannel;
							pAd->ApCfg.ApCliTab[0].CentralChannel= RootApCentralChannel;
							
							if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED) && (INFRA_ON(pAd)) 
								&&(pAd->CommonCfg.Channel == pAd->LatchRfRegs.Channel) )
							{
								RTMPSendNullFrame(pAd, 
												  pAd->CommonCfg.TxRate, 
												  (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
												  PWR_SAVE);

								DBGPRINT(RT_DEBUG_TRACE, ("ApCliCtrlJoinReqAction -- Send PSM Data frame\n"));
								OS_WAIT(200);
							}

							{

								/* switch channel and waiting for beacon timer */
								AsicSwitchChannel(pAd, pAd->ApCfg.ApCliTab[0].Channel, FALSE);
								AsicLockChannel(pAd, pAd->ApCfg.ApCliTab[0].Channel);
								bbp_set_bw(pAd, BW_20);
							}
					}
#endif /*CONFIG_MULTI_CHANNEL*/	
/* Bug fix: Sometimes p2p connection will send probe request with incorrect channel. */
#if 0
				if ((RootApChannel != channel) ||
					((RootApCentralChannel != RootApChannel) &&
					 (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40) && 
					 (ApHtInfo.ExtChanOffset != RootApHtInfo.ExtChanOffset)))
				{
					RTMP_STRING ChStr[5] = {0};
					if (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth  == BW_40)
					{
						if (RootApHtInfo.ExtChanOffset == EXTCHA_ABOVE)
							Set_HtExtcha_Proc(pAd, "1");
						else
							Set_HtExtcha_Proc(pAd, "0");
					}
					sprintf(ChStr, "%d", pAd->ScanTab.BssEntry[bss_idx].Channel);
					Set_Channel_Proc(pAd, ChStr);
					/*
						ApStop will call WscStop, we need to reset WscConfMode, WscMode & bWscTrigger here.
					*/

					pWpsCtrl->WscState = WSC_STATE_START;
					pWpsCtrl->WscStatus = STATUS_WSC_START_ASSOC;
					pWpsCtrl->WscMode = old_wsc_mode;
					pWpsCtrl->WscConfMode = old_conf_mode;
					pWpsCtrl->bWscTrigger = TRUE;
					return;
				}				
				else
				{
					pAd->CommonCfg.Channel = pAd->ScanTab.BssEntry[bss_idx].Channel;
					AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
					AsicLockChannel(pAd, pAd->CommonCfg.Channel);
				}
#endif
			}
		}
    }
    else
#endif /* WSC_AP_SUPPORT */
	if (pApCliEntry->CfgSsidLen != 0)
	{
		ULONG bss_idx = 0;
#ifdef CONFIG_MULTI_CHANNEL
			if (pAd->Multi_Channel_Enable == TRUE)
			{
					UCHAR ApChannel = 0;
					JoinReq.SsidLen = pApCliEntry->CfgSsidLen;
					NdisMoveMemory(&(JoinReq.Ssid), pApCliEntry->CfgSsid, JoinReq.SsidLen);

					bss_idx = BssSsidTableSearch(&pAd->ScanTab, JoinReq.Bssid, (PCHAR)(JoinReq.Ssid), JoinReq.SsidLen, pAd->CommonCfg.Channel);
						if (bss_idx == BSS_NOT_FOUND)
						{
							DBGPRINT(RT_DEBUG_TRACE, ("2 Can't find %s !\n",(PCHAR)pApCliEntry->CfgSsid));
							*pCurrState = APCLI_CTRL_PROBE;
							ApSiteSurvey(pAd, NULL, SCAN_WSC_ACTIVE, FALSE);
							return;
						}
						else
						{
							
							pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
							pAd->ApCfg.ApCliTab[0].Channel = pAd->ScanTab.BssEntry[bss_idx].Channel;
							pAd->ApCfg.ApCliTab[0].CentralChannel=pAd->ScanTab.BssEntry[bss_idx].CentralChannel;
							ApChannel = pAd->ScanTab.BssEntry[bss_idx].Channel;

							if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED) && (INFRA_ON(pAd)) 
								&&(pAd->CommonCfg.Channel == pAd->LatchRfRegs.Channel) )
							{
								RTMPSendNullFrame(pAd, 
												  pAd->CommonCfg.TxRate, 
												  (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
												  PWR_SAVE);

								DBGPRINT(RT_DEBUG_TRACE, ("ApCliCtrlJoinReqAction !bWscTrigger -- Send PSM Data frame\n"));
									OS_WAIT(200);
							}
							AsicSwitchChannel(pAd, ApChannel, FALSE);
							AsicLockChannel(pAd, ApChannel);
							bbp_set_bw(pAd, BW_20);
					}
			}
#endif /*CONFIG_MULTI_CHANNEL*/

/* Bug fix: Sometimes p2p connection will send probe request with incorrect channel. */
#if 0
		bss_idx = BssSsidTableSearch(&pAd->ScanTab, JoinReq.Bssid, (PCHAR)pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen, pAd->CommonCfg.Channel);
		if (bss_idx == BSS_NOT_FOUND)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s::  can't find SSID[%s] in ScanTab.\n", __FUNCTION__, pApCliEntry->CfgSsid));
			*pCurrState = APCLI_CTRL_PROBE;
			ApSiteSurvey(pAd, NULL, SCAN_ACTIVE, FALSE);
			return;
		}

		DBGPRINT(RT_DEBUG_TRACE, ("%s::  find SSID[%ld][%s] channel[%d-%d] in ScanTab.\n", __FUNCTION__, bss_idx, pApCliEntry->CfgSsid, pAd->ScanTab.BssEntry[bss_idx].Channel, pAd->ScanTab.BssEntry[bss_idx].CentralChannel));
		/* BssSearch Table has found the pEntry, send Prob Req. directly */
		pAd->CommonCfg.Channel = pAd->ScanTab.BssEntry[bss_idx].Channel;
		AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
		AsicLockChannel(pAd, pAd->CommonCfg.Channel);
#endif

		JoinReq.SsidLen = pApCliEntry->CfgSsidLen;
		NdisMoveMemory(&(JoinReq.Ssid), pApCliEntry->CfgSsid, JoinReq.SsidLen);
	}

	if (pApCliEntry->CfgSsidLen == 0)
	{
			DBGPRINT(RT_DEBUG_TRACE, ("%s::  ApCliEntry SSID Len is 0 !!!.\n", __FUNCTION__));
			*pCurrState = APCLI_CTRL_PROBE;
			ApSiteSurvey(pAd, NULL, SCAN_WSC_ACTIVE, FALSE);
			return;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Probe Ssid=%s, Bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
		__FUNCTION__, JoinReq.Ssid, JoinReq.Bssid[0], JoinReq.Bssid[1], JoinReq.Bssid[2],
		JoinReq.Bssid[3], JoinReq.Bssid[4], JoinReq.Bssid[5]));

	*pCurrState = APCLI_CTRL_PROBE;

	MlmeEnqueue(pAd, APCLI_SYNC_STATE_MACHINE, APCLI_MT2_MLME_PROBE_REQ,
		sizeof(APCLI_MLME_JOIN_REQ_STRUCT), &JoinReq, ifIndex);

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME JOIN req timeout state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlJoinReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	APCLI_MLME_JOIN_REQ_STRUCT JoinReq;
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Probe Req Timeout.\n", __FUNCTION__));

	if (ifIndex >= MAX_APCLI_NUM)
		return;

	if (ApScanRunning(pAd) == TRUE)
	{
		*pCurrState = APCLI_CTRL_DISCONNECTED;
		return;
	}

	/* stay in same state. */
	*pCurrState = APCLI_CTRL_PROBE;

	/* retry Probe Req. */
	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Retry Probe Req.\n", __FUNCTION__));

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	NdisZeroMemory(&JoinReq, sizeof(APCLI_MLME_JOIN_REQ_STRUCT));

	if (!MAC_ADDR_EQUAL(pApCliEntry->CfgApCliBssid, ZERO_MAC_ADDR))
	{
		COPY_MAC_ADDR(JoinReq.Bssid, pApCliEntry->CfgApCliBssid);
	}

#ifdef WSC_AP_SUPPORT
    if ((pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscConfMode != WSC_DISABLE) &&
		(pAd->ApCfg.ApCliTab[ifIndex].WscControl.bWscTrigger == TRUE))
    {
	    ULONG bss_idx = 0;
		bss_idx = BssSsidTableSearchBySSID(&pAd->ScanTab, (PCHAR)JoinReq.Ssid, JoinReq.SsidLen);
		if (bss_idx == BSS_NOT_FOUND)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s::  WSC exec. start SiteSurvey!\n", __FUNCTION__));
			*pCurrState = APCLI_CTRL_PROBE;
			ApSiteSurvey(pAd, NULL, SCAN_WSC_ACTIVE, FALSE);
			return;
		}
        NdisZeroMemory(JoinReq.Ssid, MAX_LEN_OF_SSID);
        JoinReq.SsidLen = pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscSsid.SsidLength;
		NdisMoveMemory(JoinReq.Ssid, pAd->ApCfg.ApCliTab[ifIndex].WscControl.WscSsid.Ssid, JoinReq.SsidLen);
    }
    else
#endif /* WSC_AP_SUPPORT */
	if (pApCliEntry->CfgSsidLen != 0)
	{
		ULONG bss_idx = 0;

		bss_idx = BssSsidTableSearchBySSID(&pAd->ScanTab, (PCHAR)pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen);
		if (bss_idx == BSS_NOT_FOUND)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s::  can't find SSID[%s] in ScanTab.\n", __FUNCTION__, pApCliEntry->CfgSsid));
			*pCurrState = APCLI_CTRL_PROBE;
			ApSiteSurvey(pAd, NULL, SCAN_ACTIVE, FALSE);
			return;
		}

			DBGPRINT(RT_DEBUG_TRACE, ("%s::  find SSID[%ld][%s] channel[%d-%d] in ScanTab.\n", __FUNCTION__, bss_idx, pApCliEntry->CfgSsid, pAd->ScanTab.BssEntry[bss_idx].Channel, pAd->ScanTab.BssEntry[bss_idx].CentralChannel));
			/* BssSearch Table has found the pEntry, send Prob Req. directly */
			pAd->CommonCfg.Channel = pAd->ScanTab.BssEntry[bss_idx].Channel;
			AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
			AsicLockChannel(pAd, pAd->CommonCfg.Channel);

		JoinReq.SsidLen = pApCliEntry->CfgSsidLen;
		NdisMoveMemory(&(JoinReq.Ssid), pApCliEntry->CfgSsid, JoinReq.SsidLen);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Probe Ssid=%s, Bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
		__FUNCTION__, JoinReq.Ssid, JoinReq.Bssid[0], JoinReq.Bssid[1], JoinReq.Bssid[2],
		JoinReq.Bssid[3], JoinReq.Bssid[4], JoinReq.Bssid[5]));
	MlmeEnqueue(pAd, APCLI_SYNC_STATE_MACHINE, APCLI_MT2_MLME_PROBE_REQ,
		sizeof(APCLI_MLME_JOIN_REQ_STRUCT), &JoinReq, ifIndex);

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME Probe Rsp state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlProbeRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	APCLI_CTRL_MSG_STRUCT *Info = (APCLI_CTRL_MSG_STRUCT *)(Elem->Msg);
	USHORT Status = Info->Status;
	PAPCLI_STRUCT pApCliEntry;
	MLME_AUTH_REQ_STRUCT AuthReq;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;
	struct wifi_dev *wdev;


	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	wdev = &pApCliEntry->wdev;


	if (Status == MLME_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Probe respond success.\n", __FUNCTION__));
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Apcli-Interface Ssid=%s.\n", __FUNCTION__, pApCliEntry->Ssid));
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Apcli-Interface Bssid=%02x:%02x:%02x:%02x:%02x:%02x.\n", __FUNCTION__,
			pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid[0],
			pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid[1],
			pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid[2],
			pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid[3],
			pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid[4],
			pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid[5]));

		*pCurrState = APCLI_CTRL_AUTH;

		pApCliEntry->AuthReqCnt = 0;

		COPY_MAC_ADDR(AuthReq.Addr, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid);

		/* start Authentication Req.	 */	
		/* If AuthMode is Auto, try shared key first */
		if ((wdev->AuthMode == Ndis802_11AuthModeShared) ||
				(wdev->AuthMode == Ndis802_11AuthModeAutoSwitch))
		{		
			AuthReq.Alg = Ndis802_11AuthModeShared;
		}
		else
		{
			AuthReq.Alg = Ndis802_11AuthModeOpen;
		}

		AuthReq.Timeout = AUTH_TIMEOUT;
		MlmeEnqueue(pAd, APCLI_AUTH_STATE_MACHINE, APCLI_MT2_MLME_AUTH_REQ,
			sizeof(MLME_AUTH_REQ_STRUCT), &AuthReq, ifIndex);
	} else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Probe respond fail.\n", __FUNCTION__));
		*pCurrState = APCLI_CTRL_DISCONNECTED;
	}

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME AUTH Rsp state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlAuthRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	APCLI_CTRL_MSG_STRUCT *Info = (APCLI_CTRL_MSG_STRUCT *)(Elem->Msg);
	USHORT Status = Info->Status;
	MLME_ASSOC_REQ_STRUCT  AssocReq;
	MLME_AUTH_REQ_STRUCT AuthReq;
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;
	struct wifi_dev *wdev;


	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	wdev = &pApCliEntry->wdev;	
	if(Status == MLME_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Auth Rsp Success.\n", __FUNCTION__));
		*pCurrState = APCLI_CTRL_ASSOC;

		pApCliEntry->AssocReqCnt = 0;

		AssocParmFill(pAd, &AssocReq, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, pAd->ApCfg.ApCliTab[0].MlmeAux.CapabilityInfo,
			ASSOC_TIMEOUT, /*pAd->PortCfg.DefaultListenCount*/5);
		MlmeEnqueue(pAd, APCLI_ASSOC_STATE_MACHINE, APCLI_MT2_MLME_ASSOC_REQ,
			sizeof(MLME_ASSOC_REQ_STRUCT), &AssocReq, ifIndex);
	} 
	else
	{
		if (wdev->AuthMode == Ndis802_11AuthModeAutoSwitch)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("(%s) Auth Rsp Failure.\n", __FUNCTION__));

			*pCurrState = APCLI_CTRL_AUTH_2;

			/* start Second Authentication Req. */
			DBGPRINT(RT_DEBUG_TRACE, ("(%s) Start Second Auth Rep.\n", __FUNCTION__));
			COPY_MAC_ADDR(AuthReq.Addr, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid);
			AuthReq.Alg = Ndis802_11AuthModeOpen;
			AuthReq.Timeout = AUTH_TIMEOUT;
			MlmeEnqueue(pAd, APCLI_AUTH_STATE_MACHINE, APCLI_MT2_MLME_AUTH_REQ,
			sizeof(MLME_AUTH_REQ_STRUCT), &AuthReq, ifIndex);
		} else
		{
			NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, MAC_ADDR_LEN);
			NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid, MAX_LEN_OF_SSID);
			pApCliEntry->AuthReqCnt = 0;
			*pCurrState = APCLI_CTRL_DISCONNECTED;
		}
	}

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME AUTH2 Rsp state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlAuth2RspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	APCLI_CTRL_MSG_STRUCT *Info = (APCLI_CTRL_MSG_STRUCT *)(Elem->Msg);
	USHORT Status = Info->Status;
	MLME_ASSOC_REQ_STRUCT  AssocReq;
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	if(Status == MLME_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Auth2 Rsp Success.\n", __FUNCTION__));
		*pCurrState = APCLI_CTRL_ASSOC;

		pApCliEntry->AssocReqCnt = 0;

		AssocParmFill(pAd, &AssocReq, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, pAd->ApCfg.ApCliTab[0].MlmeAux.CapabilityInfo,
			ASSOC_TIMEOUT, /*pAd->PortCfg.DefaultListenCount*/5);
		MlmeEnqueue(pAd, APCLI_ASSOC_STATE_MACHINE, APCLI_MT2_MLME_ASSOC_REQ, 
			sizeof(MLME_ASSOC_REQ_STRUCT), &AssocReq, ifIndex);
	} else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Sta Auth Rsp Failure.\n", __FUNCTION__));

		*pCurrState = APCLI_CTRL_DISCONNECTED;
	}

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME Auth Req timeout state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlAuthReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	MLME_AUTH_REQ_STRUCT AuthReq;
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Auth Req Timeout.\n", __FUNCTION__));

	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->AuthReqCnt++;

	if (pApCliEntry->AuthReqCnt > 5)
	{
		*pCurrState = APCLI_CTRL_DISCONNECTED;
		NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, MAC_ADDR_LEN);
		NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid, MAX_LEN_OF_SSID);
		pApCliEntry->AuthReqCnt = 0;
		return;
	}

	/* stay in same state. */
	*pCurrState = APCLI_CTRL_AUTH;

	/* retry Authentication. */
	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Retry Auth Req.\n", __FUNCTION__));
	COPY_MAC_ADDR(AuthReq.Addr, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid);
	AuthReq.Alg = pAd->ApCfg.ApCliTab[0].MlmeAux.Alg; /* Ndis802_11AuthModeOpen; */
	AuthReq.Timeout = AUTH_TIMEOUT;
	MlmeEnqueue(pAd, APCLI_AUTH_STATE_MACHINE, APCLI_MT2_MLME_AUTH_REQ,
		sizeof(MLME_AUTH_REQ_STRUCT), &AuthReq, ifIndex);

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME Auth2 Req timeout state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlAuth2ReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME ASSOC RSP state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlAssocRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	PAPCLI_STRUCT pApCliEntry;
	APCLI_CTRL_MSG_STRUCT *Info = (APCLI_CTRL_MSG_STRUCT *)(Elem->Msg);
	USHORT Status = Info->Status;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	if(Status == MLME_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) apCliIf = %d, Receive Assoc Rsp Success.\n", __FUNCTION__, ifIndex));

		if (ApCliLinkUp(pAd, ifIndex))
		{
			*pCurrState = APCLI_CTRL_CONNECTED;
		}
		else
		{
			DBGPRINT(RT_DEBUG_TRACE, ("(%s) apCliIf = %d, Insert Remote AP to MacTable failed.\n", __FUNCTION__,  ifIndex));
			/* Reset the apcli interface as disconnected and Invalid. */
			*pCurrState = APCLI_CTRL_DISCONNECTED;
			pApCliEntry->Valid = FALSE;
		}
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) apCliIf = %d, Receive Assoc Rsp Failure.\n", __FUNCTION__,  ifIndex));

		*pCurrState = APCLI_CTRL_DISCONNECTED;

		/* set the apcli interface be valid. */
		pApCliEntry->Valid = FALSE;
	}

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME DeASSOC RSP state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlDeAssocRspAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	PAPCLI_STRUCT pApCliEntry;
	APCLI_CTRL_MSG_STRUCT *Info = (APCLI_CTRL_MSG_STRUCT *)(Elem->Msg);
	USHORT Status = Info->Status;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	if (Status == MLME_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Receive DeAssoc Rsp Success.\n", __FUNCTION__));
	} else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("(%s) Receive DeAssoc Rsp Failure.\n", __FUNCTION__));
	}

	if (pApCliEntry->Valid)
		ApCliLinkDown(pAd, ifIndex);
	
	*pCurrState = APCLI_CTRL_DISCONNECTED;

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME Assoc Req timeout state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlAssocReqTimeoutAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	MLME_ASSOC_REQ_STRUCT  AssocReq;
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Assoc Req Timeout.\n", __FUNCTION__));

	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	/* give up to retry authentication req after retry it 5 times. */
	pApCliEntry->AssocReqCnt++;
	if (pApCliEntry->AssocReqCnt > 5)
	{
		*pCurrState = APCLI_CTRL_DISCONNECTED;
		NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, MAC_ADDR_LEN);
		NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid, MAX_LEN_OF_SSID);
		pApCliEntry->AuthReqCnt = 0;
		return;
	}

	/* stay in same state. */
	*pCurrState = APCLI_CTRL_ASSOC;

	/* retry Association Req. */
	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Retry Association Req.\n", __FUNCTION__));
	AssocParmFill(pAd, &AssocReq, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, pAd->ApCfg.ApCliTab[0].MlmeAux.CapabilityInfo,
		ASSOC_TIMEOUT, /*pAd->PortCfg.DefaultListenCount*/5);
	MlmeEnqueue(pAd, APCLI_ASSOC_STATE_MACHINE, APCLI_MT2_MLME_ASSOC_REQ, 
		sizeof(MLME_ASSOC_REQ_STRUCT), &AssocReq, ifIndex);

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME Disconnect Rsp state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlDisconnectReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("(%s) MLME Request disconnect.\n", __FUNCTION__));

	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	if (pApCliEntry->Valid)
		ApCliLinkDown(pAd, ifIndex);

	/* set the apcli interface be invalid. */
	pApCliEntry->Valid = FALSE;

	/* clear MlmeAux.Ssid and Bssid. */
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, MAC_ADDR_LEN);
	pAd->ApCfg.ApCliTab[0].MlmeAux.SsidLen = 0;
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid, MAX_LEN_OF_SSID);
	pAd->ApCfg.ApCliTab[0].MlmeAux.Rssi = 0;

	*pCurrState = APCLI_CTRL_DISCONNECTED;

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME Peer DeAssoc Req state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlPeerDeAssocReqAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	PAPCLI_STRUCT pApCliEntry;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;
#ifdef CONFIG_MULTI_CHANNEL
	MAC_TABLE_ENTRY *pEntry;
#endif /*CONFIG_MULTI_CHANNEL*/


	DBGPRINT(RT_DEBUG_TRACE, ("(%s) Peer DeAssoc Req.\n", __FUNCTION__));

	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	if (pApCliEntry->Valid)
		ApCliLinkDown(pAd, ifIndex);

	/* set the apcli interface be invalid. */
	pApCliEntry->Valid = FALSE;

	/* clear MlmeAux.Ssid and Bssid. */
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, MAC_ADDR_LEN);
	pAd->ApCfg.ApCliTab[0].MlmeAux.SsidLen = 0;
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid, MAX_LEN_OF_SSID);
	pAd->ApCfg.ApCliTab[0].MlmeAux.Rssi = 0;

	*pCurrState = APCLI_CTRL_DISCONNECTED;

	return;
}

/* 
    ==========================================================================
    Description:
        APCLI MLME Disconnect Req state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlDeAssocAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	PAPCLI_STRUCT pApCliEntry;
	MLME_DISASSOC_REQ_STRUCT DisassocReq;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("(%s) MLME Request Disconnect.\n", __FUNCTION__));

	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	DisassocParmFill(pAd, &DisassocReq, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, REASON_DISASSOC_STA_LEAVING);
	MlmeEnqueue(pAd, APCLI_ASSOC_STATE_MACHINE, APCLI_MT2_MLME_DISASSOC_REQ,
		sizeof(MLME_DISASSOC_REQ_STRUCT), &DisassocReq, ifIndex);

	if (pApCliEntry->Valid)
		ApCliLinkDown(pAd, ifIndex);

	/* set the apcli interface be invalid. */
	pApCliEntry->Valid = FALSE;

	/* clear MlmeAux.Ssid and Bssid. */
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, MAC_ADDR_LEN);
	pAd->ApCfg.ApCliTab[0].MlmeAux.SsidLen = 0;
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid, MAX_LEN_OF_SSID);
	pAd->ApCfg.ApCliTab[0].MlmeAux.Rssi = 0;

	*pCurrState = APCLI_CTRL_DEASSOC;

	return;
}


/* 
    ==========================================================================
    Description:
        APCLI MLME Disconnect Req state machine procedure
    ==========================================================================
 */
static VOID ApCliCtrlDeAuthAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	PAPCLI_STRUCT pApCliEntry;
	MLME_DEAUTH_REQ_STRUCT	DeAuthFrame;
	USHORT ifIndex = (USHORT)(Elem->Priv);
	PULONG pCurrState = &pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState;


	DBGPRINT(RT_DEBUG_TRACE, ("(%s) MLME Request Disconnect.\n", __FUNCTION__));

	if (ifIndex >= MAX_APCLI_NUM)
		return;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	/* Fill in the related information */
	DeAuthFrame.Reason = (USHORT)REASON_DEAUTH_STA_LEAVING;
	COPY_MAC_ADDR(DeAuthFrame.Addr, pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid);
	
	MlmeEnqueue(pAd, 
				  APCLI_AUTH_STATE_MACHINE, 
				  APCLI_MT2_MLME_DEAUTH_REQ, 
				  sizeof(MLME_DEAUTH_REQ_STRUCT),
				  &DeAuthFrame, 
				  ifIndex);

	if (pApCliEntry->Valid)
		ApCliLinkDown(pAd, ifIndex);

	/* set the apcli interface be invalid. */
	pApCliEntry->Valid = FALSE;

	/* clear MlmeAux.Ssid and Bssid. */
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Bssid, MAC_ADDR_LEN);
	pAd->ApCfg.ApCliTab[0].MlmeAux.SsidLen = 0;
	NdisZeroMemory(pAd->ApCfg.ApCliTab[0].MlmeAux.Ssid, MAX_LEN_OF_SSID);
	pAd->ApCfg.ApCliTab[0].MlmeAux.Rssi = 0;

	*pCurrState = APCLI_CTRL_DISCONNECTED;

	return;
}

#endif /* P2P_SUPPORT */

