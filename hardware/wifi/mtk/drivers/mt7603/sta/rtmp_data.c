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
	rtmp_data.c

	Abstract:
	Data path subroutines

	Revision History:
	Who 		When			What
	--------	----------		----------------------------------------------
*/
#include "rt_config.h"

#if 0
#define RxWISNR1			RXWI_N.bbp_rxinfo[1]
#define RxWISNR2			RXWI_N.bbp_rxinfo[2]
#define RxWISNR1			RXWI_O.SNR1
#define RxWISNR2			RXWI_O.SNR2
#endif


#ifdef ANT_DIVERSITY_SUPPORT
#ifdef MT7601
#define ANT_DIV_PACKET_NUM	10
// TODO: shiang-usw, what the FXXX this function used for? RSSI? SNR? or?
VOID STARxCollectEvalRssi(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk, RXINFO_STRUC *pRxInfo)
{
	struct _RXWI_XMAC *rxwi_x = &pRxBlk->pRxWI->RXWI_X;
	CHAR eval_ant = pAd->RxAnt.Pair1SecondaryRxAnt;
	CHAR select_ant = (rxwi_x->SNR1 & 0x80) >> 7;
	UCHAR phymode;
	
	if (pAd->RxAnt.EvaluatePeriod == 0)
		eval_ant = pAd->RxAnt.Pair1PrimaryRxAnt;
	else
		eval_ant = pAd->RxAnt.Pair1SecondaryRxAnt;

	/* match the default ant */
	if (eval_ant == select_ant)
	{
		if (pAd->CommonCfg.bTriggerTimer)
		{
	
			if (pRxInfo->Crc)
				pAd->RxAnt.CRC_Cnt[eval_ant]++;
	
			/* calculate SNR */
			phymode = pRxBlk->rx_rate.field.MODE;
			
			if (phymode == MODE_CCK)
			{
				pAd->RxAnt.CCK_SNR[eval_ant] += (42 - ((rxwi_x->SNR0 * 3) >> 3));
				pAd->RxAnt.CCK_Cnt[eval_ant] ++;
			}
			else
			{
				pAd->RxAnt.OFDM_SNR[eval_ant] += ConvertToSnr(pAd, rxwi_x->SNR0);
				pAd->RxAnt.OFDM_Cnt[eval_ant] ++;
			}
		}

		STA_COLLECT_RX_ANTENNA_AVERAGE_RSSI(pAd, 
										ConvertToRssi(pAd, (struct raw_rssi_info *)(&pRxBlk->rx_signal.raw_rssi[0])),
										0);

		pAd->StaCfg.NumOfAvgRssiSample++;

		if (pAd->StaCfg.NumOfAvgRssiSample >= ANT_DIV_PACKET_NUM && pAd->CommonCfg.bTriggerTimer)
		{
			pAd->CommonCfg.bTriggerTimer = FALSE;
			RTMPModTimer(&pAd->Mlme.RxAntEvalTimer, 0);
		}
		
		pAd->CommonCfg.nPacket_Cnt++;
		
	}
	else
	{
		pAd->CommonCfg.nAntMiss_Cnt++;
	}
}
#endif /* MT7601 */
#endif /* ANT_DIVERSITY_SUPPORT */


/*
	========================================================================

	Routine Description:
		This subroutine will scan through releative ring descriptor to find
		out avaliable free ring descriptor and compare with request size.
		
	Arguments:
		pAd Pointer to our adapter
		QueIdx		Selected TX Ring
		
	Return Value:
		NDIS_STATUS_FAILURE 	Not enough free descriptor
		NDIS_STATUS_SUCCESS 	Enough free descriptor

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
#ifdef RTMP_MAC_PCI
NDIS_STATUS RTMPFreeTXDRequest(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR QueIdx,
	IN UCHAR NumberRequired,
	IN PUCHAR FreeNumberIs)
{
	ULONG FreeNumber = 0;
	NDIS_STATUS Status = NDIS_STATUS_FAILURE;

	switch (QueIdx) {
	case QID_AC_BK:
	case QID_AC_BE:
	case QID_AC_VI:
	case QID_AC_VO:
	case QID_HCCA:
		{
			RTMP_TX_RING *pTxRing;

			pTxRing = &pAd->TxRing[QueIdx];
			if (pTxRing->TxSwFreeIdx > pTxRing->TxCpuIdx)
				FreeNumber = pTxRing->TxSwFreeIdx - pTxRing->TxCpuIdx - 1;
			else
				FreeNumber = pTxRing->TxSwFreeIdx + TX_RING_SIZE - pTxRing->TxCpuIdx - 1;

			if (FreeNumber >= NumberRequired)
				Status = NDIS_STATUS_SUCCESS;
			break;
		}
	case QID_MGMT:
		if (pAd->MgmtRing.TxSwFreeIdx > pAd->MgmtRing.TxCpuIdx)
			FreeNumber = pAd->MgmtRing.TxSwFreeIdx - pAd->MgmtRing.TxCpuIdx - 1;
		else
			FreeNumber = pAd->MgmtRing.TxSwFreeIdx + MGMT_RING_SIZE - pAd->MgmtRing.TxCpuIdx - 1;

		if (FreeNumber >= NumberRequired)
			Status = NDIS_STATUS_SUCCESS;
		break;

#ifdef CONFIG_ANDES_SUPPORT
	case QID_CTRL:
		if (pAd->CtrlRing.TxSwFreeIdx > pAd->CtrlRing.TxCpuIdx)
			FreeNumber = pAd->CtrlRing.TxSwFreeIdx - pAd->CtrlRing.TxCpuIdx - 1;
		else
			FreeNumber = pAd->CtrlRing.TxSwFreeIdx + MGMT_RING_SIZE - pAd->CtrlRing.TxCpuIdx - 1;

		if (FreeNumber >= NumberRequired)
			Status = NDIS_STATUS_SUCCESS;
		break;
#endif /* CONFIG_ANDES_SUPPORT */

	default:
		DBGPRINT(RT_DEBUG_ERROR,
			 ("RTMPFreeTXDRequest::Invalid QueIdx(=%d)\n", QueIdx));
		break;
	}
	*FreeNumberIs = (UCHAR) FreeNumber;

	return (Status);
}
#endif /* RTMP_MAC_PCI */


#ifdef RTMP_MAC_USB
/*
	Actually, this function used to check if the TxHardware Queue still has frame need to send.
	If no frame need to send, go to sleep, else, still wake up.
*/
NDIS_STATUS RTMPFreeTXDRequest(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR QueIdx,
	IN UCHAR NumberRequired,
	IN PUCHAR FreeNumberIs)
{
	/*ULONG         FreeNumber = 0; */
	NDIS_STATUS Status = NDIS_STATUS_FAILURE;
	unsigned long IrqFlags = 0;
	HT_TX_CONTEXT *pHTTXContext;

	switch (QueIdx) {
	case QID_AC_BK:
	case QID_AC_BE:
	case QID_AC_VI:
	case QID_AC_VO:
	case QID_HCCA:
		{
			pHTTXContext = &pAd->TxContext[QueIdx];
			RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx],
				      IrqFlags);
			if ((pHTTXContext->CurWritePosition !=
			     pHTTXContext->ENextBulkOutPosition)
			    || (pHTTXContext->IRPPending == TRUE)) {
				Status = NDIS_STATUS_FAILURE;
			} else {
				Status = NDIS_STATUS_SUCCESS;
			}
			RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx],
					IrqFlags);
		}
		break;

	case QID_MGMT:
		if (pAd->MgmtRing.TxSwFreeIdx != MGMT_RING_SIZE)
			Status = NDIS_STATUS_FAILURE;
		else
			Status = NDIS_STATUS_SUCCESS;
		break;

	default:
		DBGPRINT(RT_DEBUG_ERROR,
			 ("RTMPFreeTXDRequest::Invalid QueIdx(=%d)\n", QueIdx));
		break;
	}

	return (Status);
}
#endif /* RTMP_MAC_USB */

#ifdef RTMP_MAC_SDIO
/*
	Actually, this function used to check if the TxHardware Queue still has frame need to send.
	If no frame need to send, go to sleep, else, still wake up.
*/
NDIS_STATUS RTMPFreeTXDRequest(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR QueIdx,
	IN UCHAR NumberRequired,
	IN PUCHAR FreeNumberIs)
{
	/*ULONG         FreeNumber = 0; */
	NDIS_STATUS Status = NDIS_STATUS_FAILURE;

	return (Status);
}
#endif /* RTMP_MAC_SDIO */


/*
========================================================================
Routine Description:
	This routine is used to do packet parsing and classification for Tx packet 
	to STA device, and it will en-queue packets to our TxSwQ depends on AC 
	class.
	
Arguments:
	pAd    		Pointer to our adapter
	pPacket 	Pointer to send packet

Return Value:
	NDIS_STATUS_SUCCESS		If succes to queue the packet into TxSwQ.
	NDIS_STATUS_FAILURE			If failed to do en-queue.

Note:
	You only can put OS-indepened & STA related code in here.
========================================================================
*/
INT STASendPacket_New(RTMP_ADAPTER *pAd, PNDIS_PACKET pPacket)
{
	PACKET_INFO PacketInfo;
	UCHAR *pSrcBufVA;
	UINT SrcBufLen, frag_sz, pkt_len;
	UCHAR NumberOfFrag;
	UCHAR QueIdx = 0;
	UCHAR UserPriority;
	UCHAR wcid;
#ifdef MESH_SUPPORT
	BOOLEAN bMeshPkt = FALSE;
#endif /* MESH_SUPPORT */
	MAC_TABLE_ENTRY *pMacEntry = NULL;
	STA_TR_ENTRY *tr_entry = NULL;
	struct wifi_dev *wdev;
	INT32 ret=0;

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>%s()\n", __FUNCTION__));
#endif /* RELEASE_EXCLUDE */

	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);
	if ((!pSrcBufVA) || (SrcBufLen <= 14)) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s():pkt error(%p, %d)\n",
					__FUNCTION__, pSrcBufVA, SrcBufLen));
		goto error_drop;
	}

	wcid = RTMP_GET_PACKET_WCID(pPacket);
#if 0
#ifdef MESH_SUPPORT	
	/* In HT rate adhoc mode, A-MPDU is often used. So need to lookup BA Table and MAC Entry. */
	/* Note multicast packets in adhoc also use BSSID_WCID index. */
	if (VALID_WCID(wcid)) {
		pMacEntry = &pAd->MacTab.Content[wcid];
		tr_entry = &pAd->MacTab.tr_entry[wcid];
		if ((pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
		    && (tr_entry->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
		    && (RTMP_GET_PACKET_EAPOL(pPacket) == FALSE)) {
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			return NDIS_STATUS_FAILURE;
		}

		RTMP_SET_PACKET_WCID(pPacket, wcid);
	} else
#endif /* MESH_SUPPORT */
	if (*pSrcBufVA & 0x01) {
		if (ADHOC_ON(pAd)) {
			wcid = MCAST_WCID;
		}
	}

#ifdef XLINK_SUPPORT
	if (ADHOC_ON(pAd) && (pAd->StaCfg.PSPXlink == TRUE)) {
		pMacEntry = &pAd->MacTab.Content[MCAST_WCID];
	}
#endif /* XLINK_SUPPORT */

	 if (ADHOC_ON(pAd)) {
		if (*pSrcBufVA & 0x01) {
			RTMP_SET_PACKET_WCID(pPacket, MCAST_WCID);
			pMacEntry = &pAd->MacTab.Content[MCAST_WCID];
		} else {
#ifdef XLINK_SUPPORT
			if (pAd->StaCfg.PSPXlink) {
				pMacEntry = &pAd->MacTab.Content[MCAST_WCID];
				pMacEntry->Aid = MCAST_WCID;
			} else
#endif /* XLINK_SUPPORT */
				pMacEntry = MacTableLookup(pAd, pSrcBufVA);

			if (pMacEntry)
				RTMP_SET_PACKET_WCID(pPacket, pMacEntry->wcid);
#ifdef ETH_CONVERT_SUPPORT
			else {
				RTMP_SET_PACKET_WCID(pPacket, MCAST_WCID);
			}
#endif /* ETH_CONVERT_SUPPORT */
		}
	}
#else
	/* In HT rate adhoc mode, A-MPDU is often used. So need to lookup BA Table and MAC Entry. */
	/* Note multicast packets in adhoc also use BSSID_WCID index. */
#ifdef MESH_SUPPORT
	if (VALID_WCID(wcid) && IS_ENTRY_MESH(&pAd->MacTab.Content[wcid])) {
		bMeshPkt = TRUE;
		pMacEntry = &pAd->MacTab.Content[wcid];
		tr_entry = &pAd->MacTab.tr_entry[wcid];
		if ((pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
		    && (tr_entry->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
		    && (RTMP_GET_PACKET_EAPOL(pPacket) == FALSE)) {
			goto error_drop;
		}
	}
	else
#endif /* MESH_SUPPORT */
	{
		if (pAd->StaCfg.BssType == BSS_INFRA) {
#if defined(QOS_DLS_SUPPORT) || defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
			if (VALID_WCID(wcid) &&
			    (IS_ENTRY_DLS(&pAd->MacTab.Content[wcid]) ||
			     IS_ENTRY_TDLS(&pAd->MacTab.Content[wcid]))) {			     
				pMacEntry = &pAd->MacTab.Content[wcid];
			} else
#endif /*defined(QOS_DLS_SUPPORT) || defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */
			{
				pMacEntry = &pAd->MacTab.Content[BSSID_WCID];
#if 0 /* packet wcid is already set in wdev_tx_pkts */
				RTMP_SET_PACKET_WCID(pPacket, BSSID_WCID);
#endif
			}
		} else if (ADHOC_ON(pAd)) {
			if (*pSrcBufVA & 0x01) {
#if 0 /* packet wcid is already set in wdev_tx_pkts */
				RTMP_SET_PACKET_WCID(pPacket, MCAST_WCID);
#endif
				pMacEntry = &pAd->MacTab.Content[MCAST_WCID];
			} else {
#ifdef XLINK_SUPPORT
				if (pAd->StaCfg.PSPXlink) {
					pMacEntry = &pAd->MacTab.Content[MCAST_WCID];
					pMacEntry->Aid = MCAST_WCID;
				} else
#endif /* XLINK_SUPPORT */
					pMacEntry = MacTableLookup(pAd, pSrcBufVA);

#if 0 /* packet wcid is already set in wdev_tx_pkts */
				if (pMacEntry)
					RTMP_SET_PACKET_WCID(pPacket, pMacEntry->wcid);
#ifdef ETH_CONVERT_SUPPORT
				else {
					RTMP_SET_PACKET_WCID(pPacket, MCAST_WCID);
				}
#endif /* ETH_CONVERT_SUPPORT */
#endif
			}
		}
	}
#endif

	if (!pMacEntry) {
		DBGPRINT(RT_DEBUG_ERROR,
			 ("%s():No such Addr(%2x:%2x:%2x:%2x:%2x:%2x) in MacTab\n",
			 __FUNCTION__, PRINT_MAC(pSrcBufVA)));
		goto error_drop;
	}

	wdev = &pAd->StaCfg.wdev;
#if 0
	tr_entry = &pAd->MacTab.tr_entry[pMacEntry->wcid];
#else
	tr_entry = &pAd->MacTab.tr_entry[wcid];
#endif

#if 0 /* packet wcid is already set in wdev_tx_pkts */
	if (ADHOC_ON(pAd)
#ifdef MESH_SUPPORT
	    && (bMeshPkt == FALSE)
#endif /* MESH_SUPPORT */
	    ) {
		RTMP_SET_PACKET_WCID(pPacket, (UCHAR) pMacEntry->wcid);
	}
#endif

	/* Check the Ethernet Frame type of this packet, and set the RTMP_SET_PACKET_SPECIFIC flags. */
	/*              Here we set the PACKET_SPECIFIC flags(LLC, VLAN, DHCP/ARP, EAPOL). */
	UserPriority = 0;
	QueIdx = QID_AC_BE;
	RTMPCheckEtherType(pAd, pPacket, tr_entry, wdev, &UserPriority, &QueIdx, &wcid);
	/*add hook point when enqueue*/
	RTMP_OS_TXRXHOOK_CALL(WLAN_TX_ENQUEUE,pPacket,QueIdx,pAd);
#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#ifdef UAPSD_SUPPORT
	/* the code must behind RTMPCheckEtherType() to get QueIdx */
	if ((pMacEntry != NULL) &&
		(pMacEntry->PsMode == PWR_SAVE) &&
		(pMacEntry->FlgPsModeIsWakeForAWhile == FALSE) &&
		(pMacEntry->bAPSDDeliverEnabledPerAC[QueIdx] == 0))
	{
		/* the packet will be sent to AP, not the peer directly */
		pMacEntry = &pAd->MacTab.Content[BSSID_WCID];
#if 0 /* packet wcid is already set in wdev_tx_pkts */
		/* for AP entry, pEntry->bAPSDDeliverEnabledPerAC[QueIdx] always is 0 */
		RTMP_SET_PACKET_WCID(pPacket, BSSID_WCID);
#endif
	}
#endif /* UAPSD_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */

#ifdef WAPI_SUPPORT
	/* Check the status of the controlled port and filter the outgoing frame for WAPI */
	if (((wdev->AuthMode == Ndis802_11AuthModeWAICERT) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWAIPSK))
	    && (wdev->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
	    && (RTMP_GET_PACKET_WAI(pPacket) == FALSE)
#ifdef MESH_SUPPORT
	    && bMeshPkt == FALSE
#endif /* MESH_SUPPORT */
	    ) {
		DBGPRINT(RT_DEBUG_TRACE,
			 ("%s():Drop packet before port secured! Line:%d\n", __FUNCTION__, __LINE__));
		goto error_drop;
	}
#endif /* WAPI_SUPPORT */

	/* WPA 802.1x secured port control - drop all non-802.1x frame before port secured */
	if (((wdev->AuthMode == Ndis802_11AuthModeWPA) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWPAPSK) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWPA2) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWPA2PSK)
#ifdef WPA_SUPPLICANT_SUPPORT
	     || (pAd->StaCfg.wdev.IEEE8021X == TRUE)
#endif /* WPA_SUPPLICANT_SUPPORT */
	    )
	    && ((wdev->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
		|| (pAd->StaCfg.MicErrCnt >= 2))
	    && (RTMP_GET_PACKET_EAPOL(pPacket) == FALSE)
#ifdef MESH_SUPPORT
	    && bMeshPkt == FALSE
#endif /* MESH_SUPPORT */
	    ) {
		DBGPRINT(RT_DEBUG_TRACE,
			 ("%s():Drop packet before port secured! Line:%d\n", __FUNCTION__, __LINE__));
		goto error_drop;
	}
#ifdef WSC_STA_SUPPORT
	if ((pAd->StaCfg.WscControl.WscConfMode != WSC_DISABLE) &&
	    (pAd->StaCfg.WscControl.bWscTrigger == TRUE) &&
	    (RTMP_GET_PACKET_EAPOL(pPacket) == FALSE)) {
		DBGPRINT(RT_DEBUG_TRACE,
			 ("%s():Drop packet before WPS process completed!\n", __FUNCTION__));
		goto error_drop;
	}
#endif /* WSC_STA_SUPPORT */

	/* 
		STEP 1. Decide number of fragments required to deliver this MSDU.
			The estimation here is not very accurate because difficult to
			take encryption overhead into consideration here. The result
			"NumberOfFrag" is then just used to pre-check if enough free
			TXD are available to hold this MSDU.

			The calculated "NumberOfFrag" is a rough estimation because of various
			encryption/encapsulation overhead not taken into consideration. This number is just
			used to make sure enough free TXD are available before fragmentation takes place.
			In case the actual required number of fragments of an NDIS packet
			excceeds "NumberOfFrag"caculated here and not enough free TXD available, the
			last fragment (i.e. last MPDU) will be dropped in RTMPHardTransmit() due to out of
			resource, and the NDIS packet will be indicated NDIS_STATUS_FAILURE. This should
			rarely happen and the penalty is just like a TX RETRY fail. Affordable.

		exception:
			a). fragmentation not allowed on multicast & broadcast
			b). Aggregation overwhelms fragmentation (fCLIENT_STATUS_AGGREGATION_CAPABLE)
			c). TSO/CSO not do fragmentation
	*/
	// TODO: shiang-usw. we need to modify the TxPktClassificatio to adjust the NumberOfFrag!
	pkt_len = PacketInfo.TotalPacketLength - LENGTH_802_3 + LENGTH_802_1_H;
	frag_sz = (pAd->CommonCfg.FragmentThreshold) - LENGTH_802_11 - LENGTH_CRC;
	if (*pSrcBufVA & 0x01)	/* fragmentation not allowed on multicast & broadcast */
		NumberOfFrag = 1;
	else if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE))
		NumberOfFrag = 1;	/* Aggregation overwhelms fragmentation */
	else if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AMSDU_INUSED))
		NumberOfFrag = 1;	/* Aggregation overwhelms fragmentation */
#ifdef DOT11_N_SUPPORT
	else if ((wdev->HTPhyMode.field.MODE == MODE_HTMIX)
		 || (wdev->HTPhyMode.field.MODE == MODE_HTGREENFIELD))
		NumberOfFrag = 1;	/* MIMO RATE overwhelms fragmentation */
#endif /* DOT11_N_SUPPORT */
	else
	{
		NumberOfFrag = (UCHAR)((pkt_len / frag_sz) + 1);
		/* To get accurate number of fragmentation, Minus 1 if the size just match to allowable fragment size */
		if ((pkt_len % frag_sz) == 0) {
			NumberOfFrag--;
		}
	}

	/* Save fragment number to Ndis packet reserved field */
	RTMP_SET_PACKET_FRAGMENTS(pPacket, NumberOfFrag);

#ifdef WMM_ACM_SUPPORT
{
	BOOLEAN RTSRequired;

	/*
		STEP 2. Check the requirement of RTS; decide packet TX rate
		If multiple fragment required, RTS is required only for the first fragment
		if the fragment size large than RTS threshold
	 */
	if (NumberOfFrag > 1)
		RTSRequired = (pAd->CommonCfg.FragmentThreshold > pAd->CommonCfg.RtsThreshold) ? 1 : 0;
	else
		RTSRequired = (PacketInfo.TotalPacketLength > pAd->CommonCfg.RtsThreshold) ? 1 : 0;
	RTMP_SET_PACKET_RTS(pPacket, RTSRequired);
}
#endif /* WMM_ACM_SUPPORT */

	RTMP_SET_PACKET_UP(pPacket, UserPriority);

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#ifdef UAPSD_SUPPORT
	if ((pMacEntry != NULL) &&
		(pMacEntry->PsMode == PWR_SAVE) &&
		(pMacEntry->FlgPsModeIsWakeForAWhile == FALSE) &&
		(pMacEntry->bAPSDDeliverEnabledPerAC[QueIdx] != 0))
	{
		/*
			only UAPSD of the peer for the queue is set;
			for non-UAPSD queue, we will send it to the AP.
		*/
		if (RtmpInsertPsQueue(pAd, pPacket, pMacEntry, QueIdx) != NDIS_STATUS_SUCCESS)
		{
			goto error_drop_nofree;
		}
	}
	else
		/* non-PS mode or send PS packet to AP */
#endif /* UAPSD_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */
	{
#if 0
			unsigned long IrqFlags;

		/* Make sure SendTxWait queue resource won't be used by other threads */
		RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
		if (pAd->TxSwQueue[QueIdx].Number >= pAd->TxSwQMaxLen) {
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#ifdef BLOCK_NET_IF
			StopNetIfQueue(pAd, QueIdx, pPacket);
#endif /* BLOCK_NET_IF */
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);

			return NDIS_STATUS_FAILURE;
		} else {
				InsertTailQueueAc(pAd, pMacEntry, &pAd->TxSwQueue[QueIdx], PACKET_TO_QUEUE_ENTRY(pPacket));
		}
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#else
#ifdef IP_ASSEMBLY
		if ( (ret = rtmp_IpAssembleHandle(pAd,tr_entry, pPacket,QueIdx,PacketInfo))!=NDIS_STATUS_INVALID_DATA) 
		{
			if(ret == NDIS_STATUS_FAILURE)
			{
				goto error_drop_nofree;
			}
			/*else if success do normal path means*/
			
		}else
#endif /* IP_ASSEMBLY */
			{
				if (rtmp_enq_req(pAd, pPacket, QueIdx, tr_entry, FALSE,NULL) == FALSE) {
					goto error_drop;
				}
			}
#if 0
			//dump_tr_entry(pAd, wcid, __FUNCTION__, __LINE__);
			DBGPRINT(RT_DEBUG_OFF, ("%s(%d): shiang-dbg EnQDone done\n", __FUNCTION__, __LINE__));
#endif
#endif
	}
#ifdef DOT11_N_SUPPORT
#if 0
	if (pMacEntry && (wcid < MAX_LEN_OF_TR_TABLE))
#else
	if (pMacEntry && (tr_entry->EntryType != ENTRY_CAT_MCAST) && (pMacEntry->wcid < MAX_LEN_OF_MAC_TABLE))
#endif
	{
#if 0
		STA_TR_ENTRY *tr_entry = &pAd->MacTab.tr_entry[pMacEntry->wcid];
#endif
		RTMP_BASetup(pAd, tr_entry, UserPriority);
	}
#endif /* DOT11_N_SUPPORT */

	pAd->RalinkCounters.OneSecOsTxCount[QueIdx]++;	/* TODO: for debug only. to be removed */
	return NDIS_STATUS_SUCCESS;

error_drop:
	RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
error_drop_nofree:
	/*add hook point when drop*/
	RTMP_OS_TXRXHOOK_CALL(WLAN_TX_DROP,pPacket,QueIdx,pAd);
	return NDIS_STATUS_FAILURE;
}


/*
--------------------------------------------------------
FIND ENCRYPT KEY AND DECIDE CIPHER ALGORITHM
	Find the WPA key, either Group or Pairwise Key
	LEAP + TKIP also use WPA key.
--------------------------------------------------------
	Decide WEP bit and cipher suite to be used.
	Same cipher suite should be used for whole fragment burst
	In Cisco CCX 2.0 Leap Authentication
	WepStatus is Ndis802_11WEPEnabled but the key will use PairwiseKey
	Instead of the SharedKey, SharedKey Length may be Zero.
*/
VOID STAFindCipherAlgorithm(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	CIPHER_KEY *pKey = NULL;
	UCHAR KeyIdx = 0xff, CipherAlg = CIPHER_NONE;
	MAC_TABLE_ENTRY *pMacEntry = pTxBlk->pMacEntry;
	NDIS_802_11_ENCRYPTION_STATUS Cipher;
	PUCHAR pSrcBufVA;
	struct wifi_dev *wdev = &pAd->StaCfg.wdev;

	pSrcBufVA = GET_OS_PKT_DATAPTR(pTxBlk->pPacket);

#ifdef MESH_SUPPORT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry)) {
		if (RTMP_GET_PACKET_EAPOL(pTxBlk->pPacket)) {
			/* These EAPoL frames must be clear before 4-way handshaking is completed. */
			if ((!(TX_BLK_TEST_FLAG(pTxBlk, fTX_bClearEAPFrame))) &&
			    (pMacEntry->PairwiseKey.CipherAlg) && (pMacEntry->PairwiseKey.KeyLen))
			{
				CipherAlg = pMacEntry->PairwiseKey.CipherAlg;
				pKey = &pMacEntry->PairwiseKey;
			} else {
				CipherAlg = CIPHER_NONE;
				pKey = NULL;
			}
		} else if (pMacEntry->WepStatus == Ndis802_11WEPEnabled) {
			CipherAlg = pAd->MeshTab.SharedKey.CipherAlg;
			pKey = &pAd->MeshTab.SharedKey;
		} else if (pMacEntry->WepStatus == Ndis802_11TKIPEnable ||
					pMacEntry->WepStatus == Ndis802_11AESEnable) {
			CipherAlg = pMacEntry->PairwiseKey.CipherAlg;
			pKey = &pMacEntry->PairwiseKey;
		} else {
			CipherAlg = CIPHER_NONE;
			pKey = NULL;
		}
	}
	else
#endif /* MESH_SUPPORT */
#ifdef WAPI_SUPPORT
	if (pMacEntry
		    && (pMacEntry->WepStatus == Ndis802_11EncryptionSMS4Enabled))
	{
		if (RTMP_GET_PACKET_WAI(pTxBlk->pPacket)) {
			/* WAI negotiation packet is always clear. */
			CipherAlg = CIPHER_NONE;
			pKey = NULL;
		} else {
			KeyIdx = pMacEntry->usk_id;	/* USK ID */
			CipherAlg = pMacEntry->PairwiseKey.CipherAlg;
			if (CipherAlg == CIPHER_SMS4) {
				pKey = &pMacEntry->PairwiseKey;
#ifdef SOFT_ENCRYPT
				if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_SOFTWARE_ENCRYPT)) {
					TX_BLK_SET_FLAG(pTxBlk, fTX_bSwEncrypt);
					/* TSC increment pre encryption transmittion */
					inc_iv_byte(pKey->TxTsc, LEN_WAPI_TSC, 2);
				}
#endif /* SOFT_ENCRYPT */
			}
		}
	}
	else
#endif /* WAPI_SUPPORT */
	{
		/* Select Cipher */
		if ((*pSrcBufVA & 0x01) && (ADHOC_ON(pAd)))
			Cipher = pAd->StaCfg.GroupCipher;	/* Cipher for Multicast or Broadcast */
		else
			Cipher = pAd->StaCfg.PairCipher;	/* Cipher for Unicast */

		if (RTMP_GET_PACKET_EAPOL(pTxBlk->pPacket)) {

			/* 4-way handshaking frame must be clear */
			if (!(TX_BLK_TEST_FLAG(pTxBlk, fTX_bClearEAPFrame)) && pMacEntry &&
			    (pMacEntry->PairwiseKey.CipherAlg) && 
			    (pMacEntry->PairwiseKey.KeyLen)) { 
				CipherAlg = pMacEntry->PairwiseKey.CipherAlg; 
				KeyIdx = 0;
			}
		} else if (Cipher == Ndis802_11WEPEnabled) {
			KeyIdx = wdev->DefaultKeyId;
		} else if ((Cipher == Ndis802_11TKIPEnable) ||
			   (Cipher == Ndis802_11AESEnable)) {
			if ((*pSrcBufVA & 0x01) && (ADHOC_ON(pAd)))	/* multicast */
				KeyIdx = wdev->DefaultKeyId;
			else if (ADHOC_ON(pAd) && pAd->SharedKey[BSS0][0].KeyLen)
				KeyIdx = 0;
			else if (pMacEntry && pMacEntry->PairwiseKey.KeyLen) 
				KeyIdx = 0;
			else
				KeyIdx = wdev->DefaultKeyId;
		}

		if ((KeyIdx >= 4) && (KeyIdx != 0xff)) {
			DBGPRINT(RT_DEBUG_TRACE,
					("%s():KeyIdx must be smaller than 4!\n", __func__));
			return;
		}

		if (KeyIdx == 0xff)
			CipherAlg = CIPHER_NONE;
#ifdef ADHOC_WPA2PSK_SUPPORT
		else if ((ADHOC_ON(pAd))
			 && (Cipher == Ndis802_11AESEnable)
			 && (wdev->AuthMode == Ndis802_11AuthModeWPA2PSK)
			 && (wdev->PortSecured == WPA_802_1X_PORT_SECURED)) {
			CipherAlg = CIPHER_AES;
			pKey = &pAd->SharedKey[BSS0][KeyIdx];
		}
#endif /* ADHOC_WPA2PSK_SUPPORT */
		else if ((Cipher == Ndis802_11WEPDisabled)
			 || (((Cipher == Ndis802_11WEPEnabled) && (pAd->SharedKey[BSS0][KeyIdx].KeyLen == 0))
			|| ((((Cipher == Ndis802_11TKIPEnable) || (Cipher == Ndis802_11AESEnable)
			|| (Cipher == Ndis802_11TKIPAESMix)) && pMacEntry && (pMacEntry->PairwiseKey.KeyLen == 0))
			&& (!ADHOC_ON(pAd)))) || (ADHOC_ON(pAd) && (pAd->SharedKey[BSS0][KeyIdx].KeyLen == 0)))
			CipherAlg = CIPHER_NONE;
#ifdef WPA_SUPPLICANT_SUPPORT
		else if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP &&
			 (Cipher == Ndis802_11WEPEnabled) &&
			 (wdev->IEEE8021X == TRUE) &&
			 (wdev->PortSecured == WPA_802_1X_PORT_NOT_SECURED))
			CipherAlg = CIPHER_NONE;
#endif /* WPA_SUPPLICANT_SUPPORT */
		else {
			if (ADHOC_ON(pAd)) {
			CipherAlg = pAd->SharedKey[BSS0][KeyIdx].CipherAlg;
			pKey = &pAd->SharedKey[BSS0][KeyIdx];
		}
			else if (((Cipher == Ndis802_11TKIPEnable) ||
                           (Cipher == Ndis802_11AESEnable) ||
			   (Cipher == Ndis802_11TKIPAESMix)) && pMacEntry) {
				CipherAlg = pMacEntry->PairwiseKey.CipherAlg;
				pKey = &pMacEntry->PairwiseKey;
			} else {
				CipherAlg = pAd->SharedKey[BSS0][KeyIdx].CipherAlg;
				pKey = &pAd->SharedKey[BSS0][KeyIdx];
			}
		}
	}

	pTxBlk->CipherAlg = CipherAlg;
	pTxBlk->pKey = pKey;
	pTxBlk->KeyIdx = KeyIdx;
}


#ifdef DOT11_N_SUPPORT
VOID STABuildCache802_11Header(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk, UCHAR *buf)
{
	STA_TR_ENTRY *tr_entry;
	HEADER_802_11 *wifi_hdr;

	wifi_hdr = (HEADER_802_11 *)buf;
	tr_entry = pTxBlk->tr_entry;

	/*
		Update the cached 802.11 HEADER
	*/

	/* normal wlan header size : 24 octets */
	pTxBlk->wifi_hdr_len = sizeof(HEADER_802_11);

	/* More Bit */
	wifi_hdr->FC.MoreData = TX_BLK_TEST_FLAG(pTxBlk, fTX_bMoreData);

	/* Sequence */
	wifi_hdr->Sequence = tr_entry->TxSeq[pTxBlk->UserPriority];
	tr_entry->TxSeq[pTxBlk->UserPriority] = (tr_entry->TxSeq[pTxBlk->UserPriority] + 1) & MAXSEQ;

#ifdef MESH_SUPPORT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry)) {
		PNDIS_PACKET pPacket = pTxBlk->pPacket;
		COPY_MAC_ADDR(wifi_hdr->Addr3, pTxBlk->pMeshDA);
		if (RTMP_GET_MESH_SOURCE(pPacket) == MESH_PROXY)
			COPY_MAC_ADDR(&wifi_hdr->Octet[0], pAd->MeshTab.wdev.if_addr);
		else
			COPY_MAC_ADDR(&wifi_hdr->Octet[0], pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);
		pTxBlk->wifi_hdr_len += MAC_ADDR_LEN;
	}
	else
#endif /* MESH_SUPPORT */
	{
		/* Check if the frame can be sent through DLS direct link interface
		   If packet can be sent through DLS, then force aggregation disable. (Hard to determine peer STA's capability) */
#ifdef QOS_DLS_SUPPORT
		BOOLEAN bDLSFrame = FALSE;
		INT DlsEntryIndex = 0;

		DlsEntryIndex = RTMPCheckDLSFrame(pAd, pTxBlk->pSrcBufHeader);
		if (DlsEntryIndex >= 0)
			bDLSFrame = TRUE;
		else
			bDLSFrame = FALSE;
#endif /* QOS_DLS_SUPPORT */

		/* The addr3 of normal packet send from DS is Dest Mac address. */
#ifdef QOS_DLS_SUPPORT
		if (bDLSFrame) {
			COPY_MAC_ADDR(wifi_hdr->Addr1, pTxBlk->pSrcBufHeader);
			COPY_MAC_ADDR(wifi_hdr->Addr3, pAd->CommonCfg.Bssid);
			wifi_hdr->FC.ToDs = 0;
		} else
#endif /* QOS_DLS_SUPPORT */
#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bTdlsEntry)) {
			COPY_MAC_ADDR(wifi_hdr->Addr1, pTxBlk->pSrcBufHeader);
			COPY_MAC_ADDR(wifi_hdr->Addr3, pAd->CommonCfg.Bssid);
			wifi_hdr->FC.ToDs = 0;
		} else
#endif /* DOT11Z_TDLS_SUPPORT */
		if (ADHOC_ON(pAd))
			COPY_MAC_ADDR(wifi_hdr->Addr3, pAd->CommonCfg.Bssid);
		else
		{
			COPY_MAC_ADDR(wifi_hdr->Addr3, pTxBlk->pSrcBufHeader);
#ifdef CLIENT_WDS
			if (!MAC_ADDR_EQUAL((pTxBlk->pSrcBufHeader + MAC_ADDR_LEN), pAd->CurrentAddress))
			{
				wifi_hdr->FC.FrDs = 1;
				COPY_MAC_ADDR(&wifi_hdr->Octet[0], pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);	/* ADDR4 = SA */
				pTxBlk->wifi_hdr_len += MAC_ADDR_LEN;
			}
#endif /* CLIENT_WDS */
		}
	}

	if (pAd->CommonCfg.bAPSDForcePowerSave)
		wifi_hdr->FC.PwrMgmt = PWR_SAVE;
	else
		wifi_hdr->FC.PwrMgmt = (RtmpPktPmBitCheck(pAd) == TRUE);

	pTxBlk->MpduHeaderLen = pTxBlk->wifi_hdr_len;
}
#endif /* DOT11_N_SUPPORT */


VOID STABuildCommon802_11Header(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	HEADER_802_11 *wifi_hdr;
	UINT8 tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;
	STA_TR_ENTRY *tr_entry = pTxBlk->tr_entry;

	/*
		MAKE A COMMON 802.11 HEADER
	*/

	ASSERT((tr_entry != NULL));

	/* Get the header start offset and normal wlan header size : 24 octets */
	pTxBlk->wifi_hdr = &pTxBlk->HeaderBuf[tx_hw_hdr_len];
	pTxBlk->wifi_hdr_len = sizeof(HEADER_802_11);

	wifi_hdr = (HEADER_802_11 *)pTxBlk->wifi_hdr;
	// TODO:shiang-usw, is it necessary to do this zero here?
	NdisZeroMemory(wifi_hdr, sizeof(HEADER_802_11));

	wifi_hdr->FC.FrDs = 0;
	wifi_hdr->FC.Type = FC_TYPE_DATA;
	wifi_hdr->FC.SubType = TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM) ? SUBTYPE_QDATA : SUBTYPE_DATA;

	// TODO: shiang-usw, for BCAST/MCAST, original it's sequence assigned by "pAd->Sequence", how about now?
	if (tr_entry) {
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM)) {
			wifi_hdr->Sequence = tr_entry->TxSeq[pTxBlk->UserPriority];
			tr_entry->TxSeq[pTxBlk->UserPriority] = (tr_entry->TxSeq[pTxBlk->UserPriority] + 1) & MAXSEQ;
		} else {
			wifi_hdr->Sequence = tr_entry->NonQosDataSeq;
			tr_entry->NonQosDataSeq = (tr_entry->NonQosDataSeq + 1) & MAXSEQ;
		}
	}
	else
	{
		wifi_hdr->Sequence = pAd->Sequence;
		pAd->Sequence = (pAd->Sequence + 1) & MAXSEQ; /* next sequence */
	}

	wifi_hdr->Frag = 0;
	wifi_hdr->FC.MoreData = TX_BLK_TEST_FLAG(pTxBlk, fTX_bMoreData);

#ifdef MESH_SUPPORT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry))
	{
		PNDIS_PACKET pPacket = pTxBlk->pPacket;
		wifi_hdr->FC.FrDs = 1;
		wifi_hdr->FC.ToDs = 1;
		COPY_MAC_ADDR(wifi_hdr->Addr1, pTxBlk->pMacEntry->Addr); /* to AP2 */
		COPY_MAC_ADDR(wifi_hdr->Addr2, pAd->MeshTab.wdev.if_addr); /* from AP1 */
		COPY_MAC_ADDR(wifi_hdr->Addr3, pTxBlk->pMeshDA);
		if (RTMP_GET_MESH_SOURCE(pTxBlk->pPacket) == MESH_PROXY)
			COPY_MAC_ADDR(&wifi_hdr->Octet[0], pAd->MeshTab.wdev.if_addr);
		else
			COPY_MAC_ADDR(&wifi_hdr->Octet[0], pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);	/* ADDR4 = SA */
		pTxBlk->wifi_hdr_len += MAC_ADDR_LEN;
	}
	else
#endif /* MESH_SUPPORT */
	{
		if (pAd->StaCfg.BssType == BSS_INFRA) {
#ifdef QOS_DLS_SUPPORT
			BOOLEAN bDLSFrame = FALSE;
#endif /* QOS_DLS_SUPPORT */

			COPY_MAC_ADDR(wifi_hdr->Addr1, tr_entry->Addr);
			COPY_MAC_ADDR(wifi_hdr->Addr2, tr_entry->wdev->if_addr);
#ifdef QOS_DLS_SUPPORT
			if (INFRA_ON(pAd)) {
				INT DlsEntryIndex = RTMPCheckDLSFrame(pAd, pTxBlk->pSrcBufHeader);

				/* Check if the frame can be sent through DLS direct link interface */
				/* If packet can be sent through DLS, then force aggregation disable. (Hard to determine peer STA's capability) */
				if (DlsEntryIndex >= 0)
					bDLSFrame = TRUE;
				else
					bDLSFrame = FALSE;
			}
#endif /* QOS_DLS_SUPPORT */

#ifdef QOS_DLS_SUPPORT
			if (bDLSFrame) {
				COPY_MAC_ADDR(wifi_hdr->Addr3, tr_entry->wdev->bssid);
				wifi_hdr->FC.ToDs = 0;
			} else
#endif /* QOS_DLS_SUPPORT */
#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
			if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bTdlsEntry)) {
				COPY_MAC_ADDR(wifi_hdr->Addr3, tr_entry->wdev->bssid);
				wifi_hdr->FC.ToDs = 0;
			} else
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */
			{
				COPY_MAC_ADDR(wifi_hdr->Addr3, pTxBlk->pSrcBufHeader);
				wifi_hdr->FC.ToDs = 1;
#ifdef CLIENT_WDS
				if (!MAC_ADDR_EQUAL((pTxBlk->pSrcBufHeader + MAC_ADDR_LEN), tr_entry->wdev->if_addr))
				{
					wifi_hdr->FC.FrDs = 1;
					COPY_MAC_ADDR(&wifi_hdr->Octet[0], pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);/* ADDR4 = SA */
					pTxBlk->wifi_hdr_len += MAC_ADDR_LEN;
				}
#endif /* CLIENT_WDS */
			}
		}
		else if (ADHOC_ON(pAd))
		{
			COPY_MAC_ADDR(wifi_hdr->Addr1, pTxBlk->pSrcBufHeader);
#ifdef XLINK_SUPPORT
			if (pAd->StaCfg.PSPXlink)
				/* copy the SA of ether frames to address 2 of 802.11 frame */
				COPY_MAC_ADDR(wifi_hdr->Addr2, pTxBlk->pSrcBufHeader + MAC_ADDR_LEN);
			else
#endif /* XLINK_SUPPORT */
				COPY_MAC_ADDR(wifi_hdr->Addr2, tr_entry->wdev->if_addr);
#if 0
			COPY_MAC_ADDR(wifi_hdr->Addr3, tr_entry->wdev->bssid);
#else
			COPY_MAC_ADDR(wifi_hdr->Addr3, pAd->CommonCfg.Bssid);
#endif
			wifi_hdr->FC.ToDs = 0;
		}
	}

	if (pTxBlk->CipherAlg != CIPHER_NONE)
		wifi_hdr->FC.Wep = 1;

	if (pAd->CommonCfg.bAPSDForcePowerSave)
		wifi_hdr->FC.PwrMgmt = PWR_SAVE;
	else
		wifi_hdr->FC.PwrMgmt = (RtmpPktPmBitCheck(pAd) == TRUE);

	/* build QOS Control bytes */
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bWMM)) {
		UCHAR *buf = (UCHAR *)(pTxBlk->wifi_hdr + pTxBlk->wifi_hdr_len);

#ifdef MULTI_WMM_SUPPORT		
		if (pTxBlk->QueIdx >= EDCA_WMM1_AC0_PIPE)
			*buf = ((pTxBlk->UserPriority & 0x0F) | (pAd->CommonCfg.AckPolicy[pTxBlk->QueIdx - EDCA_WMM1_AC0_PIPE] << 5));
		else	
#endif /* MULTI_WMM_SUPPORT */
		*buf = ((pTxBlk->UserPriority & 0x0F) | (pAd->CommonCfg.AckPolicy[pTxBlk->QueIdx] << 5));

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#ifdef UAPSD_SUPPORT
		UAPSD_MR_EOSP_SET(buf, pTxBlk);
#endif /* UAPSD_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */

		if (pTxBlk->TxFrameType == TX_AMSDU_FRAME)
			*buf |= 0x80;

		*(buf + 1) = 0;
		pTxBlk->wifi_hdr_len += 2;
	}
	
	pTxBlk->MpduHeaderLen = pTxBlk->wifi_hdr_len;
}


static inline PUCHAR STA_Build_ARalink_Frame_Header(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	UCHAR *pHeaderBufPtr;
	HEADER_802_11 *wifi_hdr;
	PNDIS_PACKET pNextPacket;
	UINT32 nextBufLen;
	PQUEUE_ENTRY pQEntry;
	//UINT8 tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;

	STAFindCipherAlgorithm(pAd, pTxBlk);
	STABuildCommon802_11Header(pAd, pTxBlk);

	pHeaderBufPtr = pTxBlk->wifi_hdr;

	wifi_hdr = (HEADER_802_11 *)pHeaderBufPtr;

	/* steal "order" bit to mark "aggregation" */
	wifi_hdr->FC.Order = 1;

	/* skip common header */
	pHeaderBufPtr += pTxBlk->wifi_hdr_len;

	/* padding at front of LLC header. LLC header should at 4-bytes aligment. */
	pTxBlk->HdrPadLen = (UCHAR)((ULONG)pHeaderBufPtr);
	pHeaderBufPtr = (UCHAR *)ROUND_UP(pHeaderBufPtr, 4);
	pTxBlk->HdrPadLen = (UCHAR)((ULONG)(pHeaderBufPtr - pTxBlk->HdrPadLen));


	/*
		For RA Aggregation, put the 2nd MSDU length(extra 2-byte field) after 
		QOS_CONTROL in little endian format
	*/
	pQEntry = pTxBlk->TxPacketList.Head;
	pNextPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
	nextBufLen = GET_OS_PKT_LEN(pNextPacket);
	if (RTMP_GET_PACKET_VLAN(pNextPacket))
		nextBufLen -= LENGTH_802_1Q;

	*pHeaderBufPtr = (UCHAR)nextBufLen & 0xff;
	*(pHeaderBufPtr + 1) = (UCHAR)(nextBufLen >> 8);

	pHeaderBufPtr += 2;
	pTxBlk->MpduHeaderLen += 2;
	pTxBlk->wifi_hdr_len += 2;

	return pHeaderBufPtr;
}


#ifdef DOT11_N_SUPPORT
static inline PUCHAR STA_Build_AMSDU_Frame_Header(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	UCHAR *buf_ptr;
	HEADER_802_11 *wifi_hdr;
	//UINT8 tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;

	STAFindCipherAlgorithm(pAd, pTxBlk);
	STABuildCommon802_11Header(pAd, pTxBlk);

	buf_ptr = pTxBlk->wifi_hdr;
	wifi_hdr = (HEADER_802_11 *)buf_ptr;

	/* skip common header */
	buf_ptr += pTxBlk->wifi_hdr_len;

	/*
	   padding at front of LLC header
	   LLC header should locate at 4-octets aligment
	   @@@ MpduHeaderLen excluding padding @@@
	 */
	pTxBlk->HdrPadLen = (UCHAR)((ULONG)buf_ptr);
	buf_ptr = (UCHAR *)(ROUND_UP(buf_ptr, 4));
	pTxBlk->HdrPadLen = (UCHAR)((ULONG)(buf_ptr - pTxBlk->HdrPadLen));

	return buf_ptr;

}


#ifdef TXBF_SUPPORT
static inline BOOLEAN tx_txbf_handle(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk, BOOLEAN bHTC, UCHAR *buf_ptr)
{
	MAC_TABLE_ENTRY *pEntry = pTxBlk->pMacEntry;

	ASSERT(pEntry);

	pTxBlk->TxSndgPkt = SNDG_TYPE_DISABLE;

	DBGPRINT(RT_DEBUG_TRACE, ("--Sounding in AMPDU: TxSndgType=%d, MCS=%d\n",
				pMacEntry->TxSndgType,
				pMacEntry->TxSndgType==SNDG_TYPE_NDP? pMacEntry->sndgMcs: pTxBlk->pTransmit->field.MCS));

	NdisAcquireSpinLock(&pEntry->TxSndgLock);
	if (pEntry->TxSndgType >= SNDG_TYPE_SOUNDING)
	{
		if (bHTC == FALSE)
		{
			NdisZeroMemory(buf_ptr, sizeof(HT_CONTROL));
			bHTC = TRUE;
		}

		if (pEntry->TxSndgType == SNDG_TYPE_SOUNDING)
		{
			/* Select compress if supported. Otherwise select noncompress */
			if ((pAd->CommonCfg.ETxBfNoncompress==0) &&
				(pEntry->HTCapability.TxBFCap.ExpComBF>0))
				((PHT_CONTROL)buf_ptr)->CSISTEERING = 3;
			else
				((PHT_CONTROL)buf_ptr)->CSISTEERING = 2;

		}
		else if (pEntry->TxSndgType == SNDG_TYPE_NDP)
		{
			/* Select compress if supported. Otherwise select noncompress */
			if ((pAd->CommonCfg.ETxBfNoncompress == 0) &&
				(pEntry->HTCapability.TxBFCap.ExpComBF>0) &&
				(pEntry->HTCapability.TxBFCap.ComSteerBFAntSup >= (pEntry->sndgMcs/8))
			)
				((PHT_CONTROL)buf_ptr)->CSISTEERING = 3;
			else
				((PHT_CONTROL)buf_ptr)->CSISTEERING = 2;

			/* Set NDP Announcement */
			((PHT_CONTROL)buf_ptr)->NDPAnnounce = 1;

			pTxBlk->TxNDPSndgBW = pEntry->sndgBW;
			pTxBlk->TxNDPSndgMcs = pEntry->sndgMcs;
		}

		pTxBlk->TxSndgPkt = pEntry->TxSndgType;
		pEntry->TxSndgType = SNDG_TYPE_DISABLE;
	}
	NdisReleaseSpinLock(&pEntry->TxSndgLock);
		
#ifdef MFB_SUPPORT
#if defined(MRQ_FORCE_TX)
	/* have to replace this by the correct condition!!! */
	pEntry->HTCapability.ExtHtCapInfo.MCSFeedback = MCSFBK_MRQ;
#endif

	/*
		Ignore sounding frame because the signal format of sounding frmae may 
		be different from normal data frame, which may result in different MFB 
	*/
	if ((pEntry->HTCapability.ExtHtCapInfo.MCSFeedback >=MCSFBK_MRQ) &&
		(pTxBlk->TxSndgPkt == SNDG_TYPE_DISABLE))
	{
		if (bHTC == FALSE)
		{
			NdisZeroMemory(buf_ptr, sizeof(HT_CONTROL));
			bHTC = TRUE;
		}
		MFB_PerPareMRQ(pAd, buf_ptr, pEntry);
	}

	if (pAd->CommonCfg.HtCapability.ExtHtCapInfo.MCSFeedback >=MCSFBK_MRQ && 
		pEntry->toTxMfb == 1)
	{
		if (bHTC == FALSE)
		{
			NdisZeroMemory(buf_ptr, sizeof(HT_CONTROL));
			bHTC = TRUE;
		}
		MFB_PerPareMFB(pAd, buf_ptr, pEntry);/* not complete yet!!! */
		pEntry->toTxMfb = 0;
	}
#endif /* MFB_SUPPORT */

	return bHTC;
}
#endif /* TXBF_SUPPORT */


VOID STA_AMPDU_Frame_Tx(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	HEADER_802_11 *wifi_hdr;
	UCHAR *pHeaderBufPtr;
	USHORT freeCnt = 0;
	BOOLEAN bVLANPkt;
	MAC_TABLE_ENTRY *pMacEntry;
	STA_TR_ENTRY *tr_entry;
	PQUEUE_ENTRY pQEntry;
	BOOLEAN bHTCPlus = FALSE;
	//UINT8 TXWISize = pAd->chipCap.TXWISize;
	UINT8 tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;

	ASSERT(pTxBlk);
	

	while (pTxBlk->TxPacketList.Head) {
		pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
		pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
		if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE) {
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			continue;
		}

		pMacEntry = pTxBlk->pMacEntry;
		tr_entry = pTxBlk->tr_entry;
		if ((tr_entry->isCached)
#ifdef TXBF_SUPPORT
			&& (pMacEntry->TxSndgType == SNDG_TYPE_DISABLE)
#endif /* TXBF_SUPPORT */
		)
		{
			/* NOTE: Please make sure the size of tr_entry->CachedBuf[] is smaller than pTxBlk->HeaderBuf[]!!!! */
#if defined(MT7603) || defined(MT7628)
#ifndef VENDOR_FEATURE1_SUPPORT
			NdisMoveMemory((UCHAR *)(&pTxBlk->HeaderBuf[0]), 
							(UCHAR *)(&tr_entry->CachedBuf[0]),
							tx_hw_hdr_len + sizeof(HEADER_802_11));
#else
			pTxBlk->HeaderBuf = (UCHAR *)(tr_entry->HeaderBuf);
#endif /* VENDOR_FEATURE1_SUPPORT */
#else
#ifndef VENDOR_FEATURE1_SUPPORT
			NdisMoveMemory((UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]),
							(UCHAR *)(&tr_entry->CachedBuf[0]),
							TXWISize + sizeof(HEADER_802_11));
#else
			pTxBlk->HeaderBuf = (UCHAR *)(tr_entry->HeaderBuf);
#endif /* VENDOR_FEATURE1_SUPPORT */
#endif /* MT7603 */

			pHeaderBufPtr = (UCHAR *)(&pTxBlk->HeaderBuf[tx_hw_hdr_len]);
			STABuildCache802_11Header(pAd, pTxBlk, pHeaderBufPtr);

#ifdef SOFT_ENCRYPT
			RTMPUpdateSwCacheCipherInfo(pAd, pTxBlk, pHeaderBufPtr);
#endif /* SOFT_ENCRYPT */
		}
		else
		{
			STAFindCipherAlgorithm(pAd, pTxBlk);
			STABuildCommon802_11Header(pAd, pTxBlk);

			pHeaderBufPtr = &pTxBlk->HeaderBuf[tx_hw_hdr_len];
		}

#ifdef SOFT_ENCRYPT
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) {
			if (RTMPExpandPacketForSwEncrypt(pAd, pTxBlk) == FALSE) {
				RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
				continue;
			}
		}
#endif /* SOFT_ENCRYPT */

		bVLANPkt = (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket) ? TRUE : FALSE);
		wifi_hdr = (HEADER_802_11 *)pHeaderBufPtr;

		/* skip common header */
		pHeaderBufPtr += pTxBlk->MpduHeaderLen;

#ifdef VENDOR_FEATURE1_SUPPORT
		if (tr_entry->isCached
			&& (tr_entry->Protocol == (RTMP_GET_PACKET_PROTOCOL(pTxBlk->pPacket)))
#ifdef SOFT_ENCRYPT
		    && !TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)
#endif /* SOFT_ENCRYPT */
#ifdef TXBF_SUPPORT
			&& (pMacEntry->TxSndgType == SNDG_TYPE_DISABLE)
#endif /* TXBF_SUPPORT */
			)
		{
			/* build QOS Control bytes */
			*pHeaderBufPtr = (pTxBlk->UserPriority & 0x0F);
#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#ifdef UAPSD_SUPPORT
			UAPSD_MR_EOSP_SET(pHeaderBufPtr, pTxBlk);
#endif /* UAPSD_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */
			pTxBlk->MpduHeaderLen = tr_entry->MpduHeaderLen;
			pTxBlk->wifi_hdr_len = tr_entry->wifi_hdr_len;
			pHeaderBufPtr = ((UCHAR *)wifi_hdr) + pTxBlk->MpduHeaderLen;

			pTxBlk->HdrPadLen = tr_entry->HdrPadLen;

			/* skip 802.3 header */
			pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
			pTxBlk->SrcBufLen -= LENGTH_802_3;

			/* skip vlan tag */
			if (bVLANPkt) {
				pTxBlk->pSrcBufData += LENGTH_802_1Q;
				pTxBlk->SrcBufLen -= LENGTH_802_1Q;
			}
		}
		else
#endif /* VENDOR_FEATURE1_SUPPORT */
		{
			/* build HTC control field after QoS field */
			bHTCPlus = FALSE;
			if ((pAd->CommonCfg.bRdg == TRUE)
			    && (CLIENT_STATUS_TEST_FLAG(pTxBlk->pMacEntry, fCLIENT_STATUS_RDG_CAPABLE))
#ifdef TXBF_SUPPORT
				&& (pMacEntry->TxSndgType != SNDG_TYPE_NDP)
#endif /* TXBF_SUPPORT */
			)
			{
				if (tr_entry->isCached == FALSE)
				{
					NdisZeroMemory(pHeaderBufPtr, sizeof(HT_CONTROL));
					((PHT_CONTROL)pHeaderBufPtr)->RDG = 1;
				}
				bHTCPlus = TRUE;
			}

#ifdef TXBF_SUPPORT
			if (pAd->chipCap.FlgHwTxBfCap)
				bHTCPlus = tx_txbf_handle(pAd, pTxBlk, bHTCPlus, pHeaderBufPtr);
#endif /* TXBF_SUPPORT */

			if (bHTCPlus == TRUE)
			{
				wifi_hdr->FC.Order = 1;
				pHeaderBufPtr += 4;
				pTxBlk->MpduHeaderLen += 4;
				pTxBlk->wifi_hdr_len += 4;
			}

			ASSERT(pTxBlk->MpduHeaderLen >= 24);

			/* skip 802.3 header */
			pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
			pTxBlk->SrcBufLen -= LENGTH_802_3;

			/* skip vlan tag */
			if (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket))
			{
				pTxBlk->pSrcBufData += LENGTH_802_1Q;
				pTxBlk->SrcBufLen -= LENGTH_802_1Q;
			}

			/*
			   padding at front of LLC header
			   LLC header should locate at 4-octets aligment

			   @@@ MpduHeaderLen excluding padding @@@
			 */
			pTxBlk->HdrPadLen = (UCHAR)((ULONG)pHeaderBufPtr);
			pHeaderBufPtr = (UCHAR *)ROUND_UP(pHeaderBufPtr, 4);
			pTxBlk->HdrPadLen = (UCHAR)((ULONG)(pHeaderBufPtr - pTxBlk->HdrPadLen));

#ifdef VENDOR_FEATURE1_SUPPORT
			tr_entry->HdrPadLen = pTxBlk->HdrPadLen;
#endif /* VENDOR_FEATURE1_SUPPORT */

#ifdef SOFT_ENCRYPT
			if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) {
				tx_sw_encrypt(pAd, pTxBlk, pHeaderBufPtr, wifi_hdr);
			}
			else
#endif /* SOFT_ENCRYPT */
			{
#ifdef MESH_SUPPORT
				if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry))
					InsertPktMeshHeader(pAd, pTxBlk, &pHeaderBufPtr);
#endif /* MESH_SUPPORT */

				/*
				   Insert LLC-SNAP encapsulation - 8 octets
					if original Ethernet frame contains no LLC/SNAP,
					then an extra LLC/SNAP encap is required
				 */
				EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(pTxBlk->pSrcBufData - 2, pTxBlk->pExtraLlcSnapEncap);
				if (pTxBlk->pExtraLlcSnapEncap) {
					NdisMoveMemory(pHeaderBufPtr, pTxBlk->pExtraLlcSnapEncap, 6);
					pHeaderBufPtr += 6;
					/* get 2 octets (TypeofLen) */
					NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufData - 2, 2);

					pHeaderBufPtr += 2;
					pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
				}
			}

#ifdef VENDOR_FEATURE1_SUPPORT
			tr_entry->Protocol = RTMP_GET_PACKET_PROTOCOL(pTxBlk->pPacket);
			tr_entry->MpduHeaderLen = pTxBlk->MpduHeaderLen;
			tr_entry->wifi_hdr_len = pTxBlk->wifi_hdr_len;
#endif /* VENDOR_FEATURE1_SUPPORT */
		}

#ifdef CONFIG_TSO_SUPPORT
		// TODO: shiang-6590, the pTxBlk->MpduHeaderLen is not correct for tso_info
		rlt_tso_info_write(pAd, (TSO_INFO *)(&pTxBlk->HeaderBuf[tx_hw_hdr_len - TSO_SIZE]), pTxBlk, pTxBlk->MpduHeaderLen);
#endif /* CONFIG_TSO_SUPPORT */

		if ((tr_entry->isCached)
#ifdef TXBF_SUPPORT
			&& (pTxBlk->TxSndgPkt == SNDG_TYPE_DISABLE)
#endif /* TXBF_SUPPORT */
		)
		{
			write_tmac_info_Cache(pAd, &pTxBlk->HeaderBuf[0], pTxBlk);
		}
		else
		{
			write_tmac_info_Data(pAd, &pTxBlk->HeaderBuf[0], pTxBlk);

		if (RTMP_GET_PACKET_LOWRATE(pTxBlk->pPacket))
			tr_entry->isCached = FALSE;

#if defined(MT7603) || defined(MT7628)
	{
		ULONG cache_sz = (pHeaderBufPtr - (UCHAR *)(&pTxBlk->HeaderBuf[0]));

		NdisZeroMemory((UCHAR *)(&tr_entry->CachedBuf[0]), sizeof(tr_entry->CachedBuf));
		NdisMoveMemory((UCHAR *)(&tr_entry->CachedBuf[0]), 
						(UCHAR *)(&pTxBlk->HeaderBuf[0]), 
						cache_sz);

#ifdef VENDOR_FEATURE1_SUPPORT
		/* use space to get performance enhancement */
		NdisZeroMemory((UCHAR *)(&tr_entry->HeaderBuf[0]), sizeof(tr_entry->HeaderBuf));
		NdisMoveMemory((UCHAR *)(&tr_entry->HeaderBuf[0]), 
						(UCHAR *)(&pTxBlk->HeaderBuf[0]), 
						cache_sz);
#endif /* VENDOR_FEATURE1_SUPPORT */
	}
#else
			NdisZeroMemory((UCHAR *)(&tr_entry->CachedBuf[0]), sizeof(tr_entry->CachedBuf));
			NdisMoveMemory((UCHAR *)(&tr_entry->CachedBuf[0]),
							(UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]),
							(pHeaderBufPtr - (UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE])));

#ifdef VENDOR_FEATURE1_SUPPORT
			/* use space to get performance enhancement */
			NdisZeroMemory((UCHAR *)(&tr_entry->HeaderBuf[0]), sizeof(tr_entry->HeaderBuf));
			NdisMoveMemory((UCHAR *)(&tr_entry->HeaderBuf[0]),
				       (UCHAR *)(&pTxBlk->HeaderBuf[0]),
				       (pHeaderBufPtr - (UCHAR *)(&pTxBlk->HeaderBuf[0])));
#endif /* VENDOR_FEATURE1_SUPPORT */
#endif /* MT7603 */

//+++Mark by shiang for test
//		tr_entry->isCached = TRUE;
//---Mark by shiang for test
		}

#ifdef TXBF_SUPPORT
		if (pTxBlk->TxSndgPkt != SNDG_TYPE_DISABLE)
			tr_entry->isCached = FALSE;
#endif /* TXBF_SUPPORT */

#ifdef STATS_COUNT_SUPPORT
		pAd->RalinkCounters.TransmittedMPDUsInAMPDUCount.u.LowPart++;
		pAd->RalinkCounters.TransmittedOctetsInAMPDUCount.QuadPart += pTxBlk->SrcBufLen;
#endif /* STATS_COUNT_SUPPORT */
        pAd->TxTotalByteCnt += pTxBlk->SrcBufLen;

		HAL_WriteTxResource(pAd, pTxBlk, TRUE, &freeCnt);

#ifdef SMART_ANTENNA
	if (pMacEntry)
		pMacEntry->saTxCnt++;
#endif /* SMART_ANTENNA */

#ifdef DBG_CTRL_SUPPORT
#ifdef INCLUDE_DEBUG_QUEUE
		if (pAd->CommonCfg.DebugFlags & DBF_DBQ_TXFRAME)
			dbQueueEnqueueTxFrame((UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), (UCHAR *)wifi_hdr);
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

		/*
			Kick out Tx
		*/
			HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);

		pAd->RalinkCounters.KickTxCount++;
		pAd->RalinkCounters.OneSecTxDoneCount++;
	}
}


VOID STA_AMSDU_Frame_Tx(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	UCHAR *pHeaderBufPtr, *subFrameHeader;
	USHORT freeCnt = 0;
	USHORT subFramePayloadLen = 0;	/* AMSDU Subframe length without AMSDU-Header / Padding */
	USHORT totalMPDUSize = 0;
	UCHAR padding = 0;
	USHORT FirstTx = 0, LastTxIdx = 0;
	UCHAR frameNum = 0;
	PQUEUE_ENTRY pQEntry;

	ASSERT((pTxBlk->TxPacketList.Number > 1));

	while (pTxBlk->TxPacketList.Head)
	{
		pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
		pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
		if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE) {
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			continue;
		}

		/* skip 802.3 header */
		pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
		pTxBlk->SrcBufLen -= LENGTH_802_3;

		/* skip vlan tag */
		if (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket)) {
			pTxBlk->pSrcBufData += LENGTH_802_1Q;
			pTxBlk->SrcBufLen -= LENGTH_802_1Q;
		}

		if (frameNum == 0)
		{
			pHeaderBufPtr = STA_Build_AMSDU_Frame_Header(pAd, pTxBlk);

			/* NOTE: TxWI->TxWIMPDUByteCnt will be updated after final frame was handled. */
			write_tmac_info_Data(pAd, &pTxBlk->HeaderBuf[0], pTxBlk);
		}
		else
		{
#if defined(MT7603) || defined(MT7628)
			pHeaderBufPtr = &pTxBlk->HeaderBuf[0];
#else
			// TODO: shiang-usw, check this, original code is use pTxBlk->HeaderBuf[0]
			pHeaderBufPtr = &pTxBlk->HeaderBuf[TXINFO_SIZE];
#endif /* MT7603 */
			padding = (UCHAR)(ROUND_UP(AMSDU_SUBHEAD_LEN + subFramePayloadLen, 4)
				- (AMSDU_SUBHEAD_LEN + subFramePayloadLen));
			NdisZeroMemory(pHeaderBufPtr, padding + AMSDU_SUBHEAD_LEN);
			pHeaderBufPtr += padding;
			pTxBlk->MpduHeaderLen = padding;
		}

		/*
		   A-MSDU subframe
		   DA(6)+SA(6)+Length(2) + LLC/SNAP Encap
		 */
		subFrameHeader = pHeaderBufPtr;
		subFramePayloadLen = (USHORT)pTxBlk->SrcBufLen;

		NdisMoveMemory(subFrameHeader, pTxBlk->pSrcBufHeader, 12);

#ifdef ETH_CONVERT_SUPPORT
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bDonglePkt))
			NdisMoveMemory(&subFrameHeader[6], pAd->CurrentAddress, 6);
#endif /* ETH_CONVERT_SUPPORT */

		pHeaderBufPtr += AMSDU_SUBHEAD_LEN;
		pTxBlk->MpduHeaderLen += AMSDU_SUBHEAD_LEN;

		/* Insert LLC-SNAP encapsulation - 8 octets */
		EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(pTxBlk->pSrcBufData - 2, pTxBlk->pExtraLlcSnapEncap);

		subFramePayloadLen = (USHORT)pTxBlk->SrcBufLen;

		if (pTxBlk->pExtraLlcSnapEncap) {
			NdisMoveMemory(pHeaderBufPtr, pTxBlk->pExtraLlcSnapEncap, 6);
			pHeaderBufPtr += 6;
			/* get 2 octets (TypeofLen) */
			NdisMoveMemory(pHeaderBufPtr, pTxBlk->pSrcBufData - 2, 2);
			pHeaderBufPtr += 2;
			pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
			subFramePayloadLen += LENGTH_802_1_H;
		}

		/* update subFrame Length field */
		subFrameHeader[12] = (UCHAR)((subFramePayloadLen & 0xFF00) >> 8);
		subFrameHeader[13] = subFramePayloadLen & 0xFF;

#if defined(MT7603) || defined(MT7628)
		if (frameNum == 0)
			totalMPDUSize += pAd->chipCap.tx_hw_hdr_len - pTxBlk->hw_rsv_len + pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen + pTxBlk->SrcBufLen;
		else
			totalMPDUSize += pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;
#else
		totalMPDUSize += pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;
#endif /* MT7603 */

		if (frameNum == 0)
			FirstTx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &freeCnt);
		else
			LastTxIdx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &freeCnt);

#ifdef DBG_CTRL_SUPPORT
#ifdef INCLUDE_DEBUG_QUEUE
		if (pAd->CommonCfg.DebugFlags & DBF_DBQ_TXFRAME)
			dbQueueEnqueueTxFrame((UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), NULL);
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

		frameNum++;

#ifdef SMART_ANTENNA
		if (pTxBlk->pMacEntry)
			pTxBlk->pMacEntry->saTxCnt++;
#endif /* SMART_ANTENNA */

		pAd->RalinkCounters.KickTxCount++;
		pAd->RalinkCounters.OneSecTxDoneCount++;
        pAd->TxTotalByteCnt += pTxBlk->SrcBufLen;

#ifdef STATS_COUNT_SUPPORT
		{
			/* calculate Transmitted AMSDU Count and ByteCount */
			pAd->RalinkCounters.TxAMSDUCount.u.LowPart++;
		}
#endif /* STATS_COUNT_SUPPORT */
	}

	HAL_FinalWriteTxResource(pAd, pTxBlk, totalMPDUSize, FirstTx);
	HAL_LastTxIdx(pAd, pTxBlk->QueIdx, LastTxIdx);

	/*
		Kick out Tx
	*/
		HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);
}
#endif /* DOT11_N_SUPPORT */


VOID STA_Legacy_Frame_Tx(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	HEADER_802_11 *wifi_hdr;
	UCHAR *buf_ptr;
	USHORT freeCnt = 0;
	BOOLEAN bVLANPkt;
	QUEUE_ENTRY *pQEntry;
#ifdef CONFIG_TSO_SUPPORT
	UINT8 TXWISize = pAd->chipCap.TXWISize;
#endif /* CONFIG_TSO_SUPPORT */

	ASSERT(pTxBlk);

	pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
	pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
	if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE) {
		RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
		return;
	}

#ifdef STATS_COUNT_SUPPORT
	if (pTxBlk->TxFrameType == TX_MCAST_FRAME) {
		INC_COUNTER64(pAd->WlanCounters.MulticastTransmittedFrameCount);
	}
#endif /* STATS_COUNT_SUPPORT */

	// TODO: shiang-usw, remove this to other place!
	if (pTxBlk->TxRate < pAd->CommonCfg.MinTxRate)
		pTxBlk->TxRate = pAd->CommonCfg.MinTxRate;

	STAFindCipherAlgorithm(pAd, pTxBlk);
	STABuildCommon802_11Header(pAd, pTxBlk);

#ifdef SOFT_ENCRYPT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) {
		if (RTMPExpandPacketForSwEncrypt(pAd, pTxBlk) == FALSE) {
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			return;
		}
	}
#endif /* SOFT_ENCRYPT */

	/* skip 802.3 header and VLAN tag if have */
	pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
	pTxBlk->SrcBufLen -= LENGTH_802_3;
	bVLANPkt = (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket) ? TRUE : FALSE);
	if (bVLANPkt) {
		pTxBlk->pSrcBufData += LENGTH_802_1Q;
		pTxBlk->SrcBufLen -= LENGTH_802_1Q;
	}

	wifi_hdr = (HEADER_802_11 *)pTxBlk->wifi_hdr;
	/* The remaining content of MPDU header should locate at 4-octets aligment */
	pTxBlk->HdrPadLen = (pTxBlk->wifi_hdr_len & 0x3);
	if (pTxBlk->HdrPadLen)
		pTxBlk->HdrPadLen = 4 - pTxBlk->HdrPadLen;
	pTxBlk->MpduHeaderLen = pTxBlk->wifi_hdr_len;

	buf_ptr = (UCHAR *)(pTxBlk->wifi_hdr + pTxBlk->wifi_hdr_len + pTxBlk->HdrPadLen);

#ifdef SOFT_ENCRYPT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) {
		tx_sw_encrypt(pAd, pTxBlk, buf_ptr, wifi_hdr);
	}
	else
#endif /* SOFT_ENCRYPT */
	{
#ifdef MESH_SUPPORT
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry))
			InsertPktMeshHeader(pAd, pTxBlk, &buf_ptr);
#endif /* MESH_SUPPORT */

		/*
		   Insert LLC-SNAP encapsulation - 8 octets
			if original Ethernet frame contains no LLC/SNAP,
			then an extra LLC/SNAP encap is required
		 */
		EXTRA_LLCSNAP_ENCAP_FROM_PKT_START(pTxBlk->pSrcBufHeader,
						   pTxBlk->pExtraLlcSnapEncap);
		if (pTxBlk->pExtraLlcSnapEncap) {
			UCHAR vlan_size;

			NdisMoveMemory(buf_ptr, pTxBlk->pExtraLlcSnapEncap, 6);
			buf_ptr += 6;
			/* skip vlan tag */
			vlan_size = (bVLANPkt) ? LENGTH_802_1Q : 0;
			/* get 2 octets (TypeofLen) */
			NdisMoveMemory(buf_ptr,
				       pTxBlk->pSrcBufHeader + 12 + vlan_size,
				       2);
			buf_ptr += 2;
			pTxBlk->MpduHeaderLen += LENGTH_802_1_H; /* 6 + 2 */
		}
	}

#ifdef ADHOC_WPA2PSK_SUPPORT
	if (ADHOC_ON(pAd)
	    && (pAd->StaCfg.wdev.AuthMode == Ndis802_11AuthModeWPA2PSK)
	    && (pAd->StaCfg.GroupCipher == Ndis802_11AESEnable)
	    && (!pTxBlk->pMacEntry)) {
		/* use Wcid as Hardware Key Index */
		GET_GroupKey_WCID(pAd, pTxBlk->Wcid, BSS0);
	}
#endif /* ADHOC_WPA2PSK_SUPPORT */

#ifdef CONFIG_TSO_SUPPORT
	// TODO: shiang-6590, the pTxBlk->MpduHeaderLen is not correct for tso_info
	rlt_tso_info_write(pAd, (TSO_INFO *)(&pTxBlk->HeaderBuf[TXINFO_SIZE + TXWISize]), pTxBlk, pTxBlk->MpduHeaderLen);
#endif /* CONFIG_TSO_SUPPORT */

	/*
		prepare for TXWI
		use Wcid as Hardware Key Index
	 */
	write_tmac_info_Data(pAd, &pTxBlk->HeaderBuf[0], pTxBlk);

	HAL_WriteTxResource(pAd, pTxBlk, TRUE, &freeCnt);

#ifdef SMART_ANTENNA
	if (pTxBlk->pMacEntry)
		pTxBlk->pMacEntry->saTxCnt++;
#endif /* SMART_ANTENNA */

#ifdef DBG_CTRL_SUPPORT
#ifdef INCLUDE_DEBUG_QUEUE
	if (pAd->CommonCfg.DebugFlags & DBF_DBQ_TXFRAME)
		dbQueueEnqueueTxFrame((UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), (UCHAR *)wifi_hdr);
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

	/*
	   Kick out Tx
	 */
	HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);

	pAd->RalinkCounters.KickTxCount++;
	pAd->RalinkCounters.OneSecTxDoneCount++;
    pAd->TxTotalByteCnt += pTxBlk->SrcBufLen;

//DBGPRINT(RT_DEBUG_TRACE, ("<--%s(%d):Success\n", __FUNCTION__, __LINE__));
}


VOID STA_Fragment_Frame_Tx(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	HEADER_802_11 *wifi_hdr;
	UCHAR *buf_ptr;
	USHORT freeCnt = 0;
	BOOLEAN bVLANPkt;
	QUEUE_ENTRY *pQEntry;
	PACKET_INFO PacketInfo;
#ifdef SOFT_ENCRYPT
	UCHAR *tmp_ptr = NULL;
	UINT32 buf_offset = 0;
#endif /* SOFT_ENCRYPT */
	HTTRANSMIT_SETTING *pTransmit;
	UCHAR fragNum = 0;
	USHORT EncryptionOverhead = 0;
	UINT32 FreeMpduSize, SrcRemainingBytes;
	USHORT AckDuration;
	UINT NextMpduSize;
	//UINT8 TXWISize = pAd->chipCap.TXWISize;
	//UINT8 tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;

	ASSERT(pTxBlk);
	ASSERT(TX_BLK_TEST_FLAG(pTxBlk, fTX_bAllowFrag));

	pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
	pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
	if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE) {
		RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
		return;
	}

	STAFindCipherAlgorithm(pAd, pTxBlk);
	STABuildCommon802_11Header(pAd, pTxBlk);

#ifdef SOFT_ENCRYPT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) {
		if (RTMPExpandPacketForSwEncrypt(pAd, pTxBlk) == FALSE) {
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			return;
		}
	}
#endif /* SOFT_ENCRYPT */

	if (pTxBlk->CipherAlg == CIPHER_TKIP) {
		pTxBlk->pPacket = duplicate_pkt_with_TKIP_MIC(pAd, pTxBlk->pPacket);
		if (pTxBlk->pPacket == NULL)
			return;
		RTMP_QueryPacketInfo(pTxBlk->pPacket, &PacketInfo, &pTxBlk->pSrcBufHeader, &pTxBlk->SrcBufLen);
	}

	/* skip 802.3 header */
	pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
	pTxBlk->SrcBufLen -= LENGTH_802_3;

	/* skip vlan tag */
	bVLANPkt = (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket) ? TRUE : FALSE);
	if (bVLANPkt) {
		pTxBlk->pSrcBufData += LENGTH_802_1Q;
		pTxBlk->SrcBufLen -= LENGTH_802_1Q;
	}

	buf_ptr = pTxBlk->wifi_hdr;
	wifi_hdr = (HEADER_802_11 *)buf_ptr;

	/* skip common header */
	buf_ptr += pTxBlk->wifi_hdr_len;

	/* The remaining content of MPDU header should locate at 4-octets aligment */
	pTxBlk->HdrPadLen = (UCHAR)((ULONG)buf_ptr);
	buf_ptr = (UCHAR *)ROUND_UP(buf_ptr, 4);
	pTxBlk->HdrPadLen = (UCHAR)((ULONG)(buf_ptr - pTxBlk->HdrPadLen));
	pTxBlk->MpduHeaderLen = pTxBlk->wifi_hdr_len;

#ifdef SOFT_ENCRYPT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) {
		UCHAR iv_offset = 0;

		/*
			If original Ethernet frame contains no LLC/SNAP,
			then an extra LLC/SNAP encap is required
		*/
		EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(pTxBlk->pSrcBufData - 2,
						    pTxBlk->pExtraLlcSnapEncap);

		/* Insert LLC-SNAP encapsulation (8 octets) to MPDU data buffer */
		if (pTxBlk->pExtraLlcSnapEncap) {
			/* Reserve the front 8 bytes of data for LLC header */
			pTxBlk->pSrcBufData -= LENGTH_802_1_H;
			pTxBlk->SrcBufLen += LENGTH_802_1_H;

			NdisMoveMemory(pTxBlk->pSrcBufData, pTxBlk->pExtraLlcSnapEncap, 6);
		}

		/* Construct and insert specific IV header to MPDU header */
		RTMPSoftConstructIVHdr(pTxBlk->CipherAlg,
				       pTxBlk->KeyIdx,
				       pTxBlk->pKey->TxTsc,
				       buf_ptr, &iv_offset);
		buf_ptr += iv_offset;
		pTxBlk->MpduHeaderLen += iv_offset;

	}
	else
#endif /* SOFT_ENCRYPT */
	{
#ifdef MESH_SUPPORT
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry))
			InsertPktMeshHeader(pAd, pTxBlk, &buf_ptr);
#endif /* MESH_SUPPORT */

		/*
		   Insert LLC-SNAP encapsulation - 8 octets
			if original Ethernet frame contains no LLC/SNAP,
			then an extra LLC/SNAP encap is required
		 */
		EXTRA_LLCSNAP_ENCAP_FROM_PKT_START(pTxBlk->pSrcBufHeader,
						   pTxBlk->pExtraLlcSnapEncap);
		if (pTxBlk->pExtraLlcSnapEncap) {
			UCHAR vlan_size;

			NdisMoveMemory(buf_ptr, pTxBlk->pExtraLlcSnapEncap, 6);
			buf_ptr += 6;
			/* skip vlan tag */
			vlan_size = (bVLANPkt) ? LENGTH_802_1Q : 0;
			/* get 2 octets (TypeofLen) */
			NdisMoveMemory(buf_ptr, pTxBlk->pSrcBufHeader + 12 + vlan_size, 2);
			buf_ptr += 2;
			pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
		}
	}

	/*  1. If TKIP is used and fragmentation is required. Driver has to
		   append TKIP MIC at tail of the scatter buffer
		2. When TXWI->FRAG is set as 1 in TKIP mode, 
		   MAC ASIC will only perform IV/EIV/ICV insertion but no TKIP MIC
	*/
	/*  TKIP appends the computed MIC to the MSDU data prior to fragmentation into MPDUs. */
	if (pTxBlk->CipherAlg == CIPHER_TKIP) {
#ifdef ETH_CONVERT_SUPPORT
		/*
		   When enable dongle mode for EthernetConverter, we cannot directly calculate the
		   MIC value base on original 802.3 packet, we need use our MAC address as the
		   src MAC of 802.3 packet to calculate the MIC, so we use the "bDonglePkt" to
		   indicate if the function should calculate this packet base on origianl paylaod
		   or need to change the srcMAC as our MAC address.
		 */
		RTMPCalculateMICValue(pAd, pTxBlk->pPacket,
				      pTxBlk->pExtraLlcSnapEncap, pTxBlk->pKey,
				      (TX_BLK_TEST_FLAG
				       (pTxBlk, fTX_bDonglePkt)));
#else
		RTMPCalculateMICValue(pAd, pTxBlk->pPacket, pTxBlk->pExtraLlcSnapEncap, pTxBlk->pKey, 0);
#endif /* ETH_CONVERT_SUPPORT */

		/*
		   NOTE: DON'T refer the skb->len directly after following copy. Becasue the length is not adjust
				to correct lenght, refer to pTxBlk->SrcBufLen for the packet length in following progress.
		 */
		NdisMoveMemory(pTxBlk->pSrcBufData + pTxBlk->SrcBufLen, &pAd->PrivateInfo.Tx.MIC[0], 8);
		pTxBlk->SrcBufLen += 8;
		pTxBlk->TotalFrameLen += 8;
	}

	/*
	   calcuate the overhead bytes that encryption algorithm may add. This
	   affects the calculate of "duration" field
	 */
	if ((pTxBlk->CipherAlg == CIPHER_WEP64) || (pTxBlk->CipherAlg == CIPHER_WEP128))
		EncryptionOverhead = 8; /* WEP: IV[4] + ICV[4]; */
	else if (pTxBlk->CipherAlg == CIPHER_TKIP)
		EncryptionOverhead = 12; /* TKIP: IV[4] + EIV[4] + ICV[4], MIC will be added to TotalPacketLength */
	else if (pTxBlk->CipherAlg == CIPHER_AES)
		EncryptionOverhead = 16;	/* AES: IV[4] + EIV[4] + MIC[8] */
#ifdef WAPI_SUPPORT
	else if (pTxBlk->CipherAlg == CIPHER_SMS4)
		EncryptionOverhead = 16;	/* SMS4: MIC[16] */
#endif /* WAPI_SUPPORT */
	else
		EncryptionOverhead = 0;

	pTransmit = pTxBlk->pTransmit;
	/* Decide the TX rate */
	if (pTransmit->field.MODE == MODE_CCK)
		pTxBlk->TxRate = (UCHAR)pTransmit->field.MCS;
	else if (pTransmit->field.MODE == MODE_OFDM)
		pTxBlk->TxRate = (UCHAR)(pTransmit->field.MCS + RATE_FIRST_OFDM_RATE);
	else
		pTxBlk->TxRate = RATE_6_5;

	/* decide how much time an ACK/CTS frame will consume in the air */
	if (pTxBlk->TxRate <= RATE_LAST_OFDM_RATE)
		AckDuration = RTMPCalcDuration(pAd, pAd->CommonCfg.ExpectedACKRate[pTxBlk->TxRate], 14);
	else
		AckDuration = RTMPCalcDuration(pAd, RATE_6_5, 14);
	/*DBGPRINT(RT_DEBUG_INFO, ("!!!Fragment AckDuration(%d), TxRate(%d)!!!\n", AckDuration, pTxBlk->TxRate)); */

#ifdef SOFT_ENCRYPT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) {
		/* store the outgoing frame for calculating MIC per fragmented frame */
		os_alloc_mem(pAd, (PUCHAR *)&tmp_ptr, pTxBlk->SrcBufLen);
		if (tmp_ptr == NULL) {
			DBGPRINT(RT_DEBUG_ERROR, ("%s():no memory for MIC calculation!\n",
				  __FUNCTION__));
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			return;
		}
		NdisMoveMemory(tmp_ptr, pTxBlk->pSrcBufData, pTxBlk->SrcBufLen);
	}
#endif /* SOFT_ENCRYPT */

	/* Init the total payload length of this frame. */
	SrcRemainingBytes = pTxBlk->SrcBufLen;
	pTxBlk->TotalFragNum = 0xff;

	do {
		FreeMpduSize = pAd->CommonCfg.FragmentThreshold - LENGTH_CRC;
		FreeMpduSize -= pTxBlk->MpduHeaderLen;

		if (SrcRemainingBytes <= FreeMpduSize)
		{
			/* This is the last or only fragment */
			pTxBlk->SrcBufLen = SrcRemainingBytes;

			wifi_hdr->FC.MoreFrag = 0;
			wifi_hdr->Duration = pAd->CommonCfg.Dsifs + AckDuration;

			/* Indicate the lower layer that this's the last fragment. */
			pTxBlk->TotalFragNum = fragNum;
#ifdef MT_MAC
			pTxBlk->FragIdx = ((fragNum == 0) ? TX_FRAG_ID_NO : TX_FRAG_ID_LAST);
#endif /* MT_MAC */
		}
		else
		{	/* more fragment is required */
			pTxBlk->SrcBufLen = FreeMpduSize;

			NextMpduSize = min(((UINT)SrcRemainingBytes - pTxBlk->SrcBufLen),
								((UINT)pAd->CommonCfg.FragmentThreshold));
			wifi_hdr->FC.MoreFrag = 1;
			wifi_hdr->Duration = (3 * pAd->CommonCfg.Dsifs) + (2 * AckDuration) + 
			    						RTMPCalcDuration(pAd, pTxBlk->TxRate, NextMpduSize + EncryptionOverhead);
#ifdef MT_MAC
			pTxBlk->FragIdx = ((fragNum == 0) ? TX_FRAG_ID_FIRST : TX_FRAG_ID_MIDDLE);
#endif /* MT_MAC */
		}

		SrcRemainingBytes -= pTxBlk->SrcBufLen;

		if (fragNum == 0)
			pTxBlk->FrameGap = IFS_HTTXOP;
		else
			pTxBlk->FrameGap = IFS_SIFS;

#ifdef SOFT_ENCRYPT
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt))
		{
			UCHAR ext_offset = 0;

			NdisMoveMemory(pTxBlk->pSrcBufData, tmp_ptr + buf_offset, pTxBlk->SrcBufLen);
			buf_offset += pTxBlk->SrcBufLen;

			/* Encrypt the MPDU data by software */
			RTMPSoftEncryptionAction(pAd,
						 pTxBlk->CipherAlg,
									(UCHAR *)wifi_hdr,
						 pTxBlk->pSrcBufData,
						 pTxBlk->SrcBufLen,
						 pTxBlk->KeyIdx,
									pTxBlk->pKey,
									&ext_offset);
			pTxBlk->SrcBufLen += ext_offset;
			pTxBlk->TotalFrameLen += ext_offset;
		}
#endif /* SOFT_ENCRYPT */

		write_tmac_info_Data(pAd, &pTxBlk->HeaderBuf[0], pTxBlk);
		HAL_WriteFragTxResource(pAd, pTxBlk, fragNum, &freeCnt);

#ifdef SMART_ANTENNA
		if (pTxBlk->pMacEntry)
			pTxBlk->pMacEntry->saTxCnt++;
#endif /* SMART_ANTENNA */

#ifdef DBG_CTRL_SUPPORT
#ifdef INCLUDE_DEBUG_QUEUE
		if (pAd->CommonCfg.DebugFlags & DBF_DBQ_TXFRAME)
			dbQueueEnqueueTxFrame((UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), (UCHAR *)wifi_hdr);
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

		pAd->RalinkCounters.KickTxCount++;
		pAd->RalinkCounters.OneSecTxDoneCount++;
        pAd->TxTotalByteCnt += pTxBlk->SrcBufLen;

#ifdef SOFT_ENCRYPT
		if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt))
		{
#ifdef WAPI_SUPPORT
			if (pTxBlk->CipherAlg == CIPHER_SMS4)
			{
				/* incease WPI IV for next MPDU */
				inc_iv_byte(pTxBlk->pKey->TxTsc, LEN_WAPI_TSC, 2);
				/* Construct and insert WPI-SMS4 IV header to MPDU header */
				RTMPConstructWPIIVHdr(pTxBlk->KeyIdx, pTxBlk->pKey->TxTsc,
						      buf_ptr - (LEN_WPI_IV_HDR));
			}
			else
#endif /* WAPI_SUPPORT */
			if ((pTxBlk->CipherAlg == CIPHER_WEP64) || (pTxBlk->CipherAlg == CIPHER_WEP128))
			{
				inc_iv_byte(pTxBlk->pKey->TxTsc, LEN_WEP_TSC, 1);
				/* Construct and insert 4-bytes WEP IV header to MPDU header */
				RTMPConstructWEPIVHdr(pTxBlk->KeyIdx, pTxBlk->pKey->TxTsc,
						      buf_ptr - (LEN_WEP_IV_HDR));
			}
			else if (pTxBlk->CipherAlg == CIPHER_TKIP)
				;
			else if (pTxBlk->CipherAlg == CIPHER_AES)
			{
				inc_iv_byte(pTxBlk->pKey->TxTsc, LEN_WPA_TSC, 1);
				/* Construct and insert 8-bytes CCMP header to MPDU header */
				RTMPConstructCCMPHdr(pTxBlk->KeyIdx, pTxBlk->pKey->TxTsc,
						     buf_ptr - (LEN_CCMP_HDR));
			}
		}
		else
#endif /* SOFT_ENCRYPT */
		{
			/* Update the frame number, remaining size of the NDIS packet payload. */
			if (fragNum == 0 && pTxBlk->pExtraLlcSnapEncap)
				pTxBlk->MpduHeaderLen -= LENGTH_802_1_H;	/* space for 802.11 header. */
		}

		fragNum++;
		/* SrcRemainingBytes -= pTxBlk->SrcBufLen; */
		pTxBlk->pSrcBufData += pTxBlk->SrcBufLen;

		wifi_hdr->Frag++;	/* increase Frag # */

	} while (SrcRemainingBytes > 0);

#ifdef SOFT_ENCRYPT
	if (tmp_ptr != NULL)
		os_free_mem(pAd, tmp_ptr);
#endif /* SOFT_ENCRYPT */

	/*
	   Kick out Tx
	 */
		HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);
}


VOID STA_ARalink_Frame_Tx(RTMP_ADAPTER *pAd, TX_BLK * pTxBlk)
{
	UCHAR *buf_ptr;
	USHORT freeCnt = 0;
	USHORT totalMPDUSize = 0;
	USHORT FirstTx, LastTxIdx;
	UCHAR frameNum = 0;
	BOOLEAN bVLANPkt;
	PQUEUE_ENTRY pQEntry;

	ASSERT(pTxBlk);
	ASSERT((pTxBlk->TxPacketList.Number == 2));

	FirstTx = LastTxIdx = 0;  /* Is it ok init they as 0? */
	while (pTxBlk->TxPacketList.Head)
	{
		pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
		pTxBlk->pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);

		if (RTMP_FillTxBlkInfo(pAd, pTxBlk) != TRUE) {
			RELEASE_NDIS_PACKET(pAd, pTxBlk->pPacket, NDIS_STATUS_FAILURE);
			continue;
		}

		/* skip 802.3 header */
		pTxBlk->pSrcBufData = pTxBlk->pSrcBufHeader + LENGTH_802_3;
		pTxBlk->SrcBufLen -= LENGTH_802_3;

		/* skip vlan tag */
		bVLANPkt = (RTMP_GET_PACKET_VLAN(pTxBlk->pPacket) ? TRUE : FALSE);
		if (bVLANPkt) {
			pTxBlk->pSrcBufData += LENGTH_802_1Q;
			pTxBlk->SrcBufLen -= LENGTH_802_1Q;
		}

		/*
			For first frame, we need to create:
				802.11 header + padding(optional) + RA-AGG-LEN + SNAP Header
			For second aggregated frame, we need create:
				the 802.3 header to headerBuf, because PCI will copy it to SDPtr0
		*/
		if (frameNum == 0)
		{
			buf_ptr = STA_Build_ARalink_Frame_Header(pAd, pTxBlk);

			/*
				It's ok write the TxWI here, because the TxWI->TxWIMPDUByteCnt
			   will be updated after final frame was handled.
			 */
			write_tmac_info_Data(pAd, &pTxBlk->HeaderBuf[0], pTxBlk);

#ifdef MESH_SUPPORT
			if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry))
				InsertPktMeshHeader(pAd, pTxBlk, &buf_ptr);
#endif /* MESH_SUPPORT */

			/* Insert LLC-SNAP encapsulation - 8 octets */
			EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(pTxBlk->pSrcBufData - 2,
							    pTxBlk->pExtraLlcSnapEncap);

			if (pTxBlk->pExtraLlcSnapEncap) {
				NdisMoveMemory(buf_ptr, pTxBlk->pExtraLlcSnapEncap, 6);
				buf_ptr += 6;
				
				/* get 2 octets (TypeofLen) */
				NdisMoveMemory(buf_ptr, pTxBlk->pSrcBufData - 2, 2);
				buf_ptr += 2;
				pTxBlk->MpduHeaderLen += LENGTH_802_1_H;
			}
		}
		else
		{
			buf_ptr = &pTxBlk->HeaderBuf[0];
			pTxBlk->MpduHeaderLen = 0;

			/* 
			   A-Ralink sub-sequent frame header is the same as 802.3 header.
			   DA(6)+SA(6)+FrameType(2)
			 */
			NdisMoveMemory(buf_ptr, pTxBlk->pSrcBufHeader, 12);
			buf_ptr += 12;
			/* get 2 octets (TypeofLen) */
			NdisMoveMemory(buf_ptr, pTxBlk->pSrcBufData - 2, 2);
			buf_ptr += 2;
			pTxBlk->MpduHeaderLen = ARALINK_SUBHEAD_LEN;
		}

#if defined(MT7603) || defined(MT7628)
		if (frameNum == 0)
			totalMPDUSize += pAd->chipCap.tx_hw_hdr_len - pTxBlk->hw_rsv_len + pTxBlk->MpduHeaderLen + pTxBlk->HdrPadLen + pTxBlk->SrcBufLen;
		else
		totalMPDUSize += pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;
#else
		totalMPDUSize += pTxBlk->MpduHeaderLen + pTxBlk->SrcBufLen;
#endif /* MT7603 */

		if (frameNum == 0)
			FirstTx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &freeCnt);
		else
			LastTxIdx = HAL_WriteMultiTxResource(pAd, pTxBlk, frameNum, &freeCnt);

#ifdef SMART_ANTENNA
		if (pTxBlk->pMacEntry)
			pTxBlk->pMacEntry->saTxCnt++;
#endif /* SMART_ANTENNA */

#ifdef DBG_CTRL_SUPPORT
#ifdef INCLUDE_DEBUG_QUEUE
		if (pAd->CommonCfg.DebugFlags & DBF_DBQ_TXFRAME)
			dbQueueEnqueueTxFrame((UCHAR *)(&pTxBlk->HeaderBuf[TXINFO_SIZE]), NULL);
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

		frameNum++;

		pAd->RalinkCounters.OneSecTxARalinkCnt++;
		pAd->RalinkCounters.KickTxCount++;
		pAd->RalinkCounters.OneSecTxDoneCount++;
        pAd->TxTotalByteCnt += pTxBlk->SrcBufLen;
	}

	HAL_FinalWriteTxResource(pAd, pTxBlk, totalMPDUSize, FirstTx);
	HAL_LastTxIdx(pAd, pTxBlk->QueIdx, LastTxIdx);

	/*
	   Kick out Tx
	 */
		HAL_KickOutTx(pAd, pTxBlk, pTxBlk->QueIdx);

}


#define RELEASE_FRAMES_OF_TXBLK(_pAd, _pTxBlk, _pQEntry, _Status) 										\
		while(_pTxBlk->TxPacketList.Head)														\
		{																						\
			_pQEntry = RemoveHeadQueue(&_pTxBlk->TxPacketList);									\
			RELEASE_NDIS_PACKET(_pAd, QUEUE_ENTRY_TO_PACKET(_pQEntry), _Status);	\
		}


#ifdef VHT_TXBF_SUPPORT
VOID STA_NDPA_Frame_Tx(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	UCHAR *buf;
	VHT_NDPA_FRAME *vht_ndpa;
	struct wifi_dev *wdev;
	UINT frm_len, sta_cnt;
	SNDING_STA_INFO *sta_info;
	MAC_TABLE_ENTRY *pMacEntry;
	
	pTxBlk->Wcid = RTMP_GET_PACKET_WCID(pTxBlk->pPacket);
	pTxBlk->pMacEntry = &pAd->MacTab.Content[pTxBlk->Wcid];
	pMacEntry = pTxBlk->pMacEntry;

	if (pMacEntry) 
	{
		wdev = pMacEntry->wdev;

		if (MlmeAllocateMemory(pAd, &buf) != NDIS_STATUS_SUCCESS)
			return;

		NdisZeroMemory(buf, MGMT_DMA_BUFFER_SIZE);

		vht_ndpa = (VHT_NDPA_FRAME *)buf;
		frm_len = sizeof(VHT_NDPA_FRAME);
		vht_ndpa->fc.Type = FC_TYPE_CNTL;
		vht_ndpa->fc.SubType = SUBTYPE_VHT_NDPA;
		COPY_MAC_ADDR(vht_ndpa->ra, pMacEntry->Addr);
		COPY_MAC_ADDR(vht_ndpa->ta, wdev->if_addr);

		/* Currnetly we only support 1 STA for a VHT DNPA */
		sta_info = vht_ndpa->sta_info;
		sta_info->aid12 = 0;
		sta_info->fb_type = SNDING_FB_SU;
		sta_info->nc_idx = 0;
		vht_ndpa->token.token_num = pMacEntry->snd_dialog_token;
		frm_len += sizeof(SNDING_STA_INFO);

		if (frm_len >= (MGMT_DMA_BUFFER_SIZE - sizeof(SNDING_STA_INFO))) {
			DBGPRINT(RT_DEBUG_ERROR, ("%s(): len(%d) too large!cnt=%d\n",
						__FUNCTION__, frm_len, sta_cnt));
		}

		if (pMacEntry->snd_dialog_token & 0xc0)
			pMacEntry->snd_dialog_token = 0;
		else
			pMacEntry->snd_dialog_token++;

#if 0 // Jason's suggestion
		vht_ndpa->duration = pAd->CommonCfg.Dsifs + 
							RTMPCalcDuration(pAd, pAd->CommonCfg.MlmeRate, frm_len);
#else
		vht_ndpa->duration = 100;
#endif

		//DBGPRINT(RT_DEBUG_OFF, ("Send VHT NDPA Frame to STA(%02x:%02x:%02x:%02x:%02x:%02x)\n",
		//						PRINT_MAC(pMacEntry->Addr)));
		//hex_dump("VHT NDPA Frame", buf, frm_len);

		// NDPA's BW needs to sync with Tx BW
		pAd->CommonCfg.MlmeTransmit.field.BW = pMacEntry->HTPhyMode.field.BW;

		pTxBlk->Flags = FALSE; // No Acq Request
		
		MiniportMMRequest(pAd, 0, buf, frm_len);
		MlmeFreeMemory(pAd, buf);
	}

	pMacEntry->TxSndgType = SNDG_TYPE_DISABLE;
}
#endif /* VHT_TXBF_SUPPORT */


/*
	========================================================================
	Routine Description:
		Copy frame from waiting queue into relative ring buffer and set 
	appropriate ASIC register to kick hardware encryption before really
	sent out to air.
		
	Arguments:
		pAd 	   		Pointer to our adapter
		pTxBlk			Pointer to outgoing TxBlk structure.
		QueIdx			Queue index for processing
		
	Return Value:
		None
	========================================================================
*/
NDIS_STATUS STAHardTransmit(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	PQUEUE_ENTRY pQEntry;
	PNDIS_PACKET pPacket;

	if (pTxBlk->TxPacketList.Head == NULL) {
		DBGPRINT(RT_DEBUG_ERROR,
			 ("pTxBlk->TotalFrameNum == %d!\n",
			  pTxBlk->TxPacketList.Number));
		return NDIS_STATUS_FAILURE;
	}

	pPacket = QUEUE_ENTRY_TO_PACKET(pTxBlk->TxPacketList.Head);

#ifdef STA_LP_PHASE_1_SUPPORT
	if (pAd->CountDowntoPsm < 12)
		pAd->CountDowntoPsm = 12;
#else
#ifdef RTMP_MAC_USB
	/* there's packet to be sent, keep awake for 1200ms */
	if (pAd->CountDowntoPsm < 12)
		pAd->CountDowntoPsm = 12;
#endif /* RTMP_MAC_USB */
#endif

	/*add hook point when dequeue*/
	RTMP_OS_TXRXHOOK_CALL(WLAN_TX_DEQUEUE,pPacket,pTxBlk->QueIdx,pAd);
	/* ------------------------------------------------------------------
	   STEP 1. WAKE UP PHY
	   outgoing frame always wakeup PHY to prevent frame lost and 
	   turn off PSM bit to improve performance
	   ------------------------------------------------------------------
	   not to change PSM bit, just send this frame out?
	 */
	/*In MT7603 legacy psp and want to tx packet, keep protocol in sleep mode and H/W in active mode*/
#ifdef STA_LP_PHASE_1_SUPPORT	
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
#else
	if ((pAd->StaCfg.PwrMgmt.Psm == PWR_SAVE) &&
		OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE) && 
		(pAd->StaCfg.WindowsPowerMode != Ndis802_11PowerModeLegacy_PSP))
#endif /* STA_LP_PHASE_1_SUPPORT */
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%s::AsicForceWakeup At HardTx\n", __FUNCTION__));
		RTEnqueueInternalCmd(pAd, CMDTHREAD_FORCE_WAKE_UP, NULL, 0);
	}
	
	/* It should not change PSM bit, when APSD turn on. */
	if ((!(pAd->StaCfg.wdev.UapsdInfo.bAPSDCapable && pAd->CommonCfg.APEdcaParm.bAPSDCapable)
			&& (pAd->CommonCfg.bAPSDForcePowerSave == FALSE))
	    || (RTMP_GET_PACKET_EAPOL(pTxBlk->pPacket))
	    || (RTMP_GET_PACKET_WAI(pTxBlk->pPacket)))
	{
		if ((RtmpPktPmBitCheck(pAd) == TRUE) && (pAd->StaCfg.WindowsPowerMode == Ndis802_11PowerModeFast_PSP))
		{
			RTMP_SET_PSM_BIT(pAd, PWR_ACTIVE);
		}
	}

#ifdef HDR_TRANS_TX_SUPPORT
#ifdef MESH_SUPPORT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bMeshEntry))
		pTxBlk->NeedTrans = FALSE;
	else
#endif /* MESH_SUPPORT */
#ifdef SOFT_ENCRYPT
	if (TX_BLK_TEST_FLAG(pTxBlk, fTX_bSwEncrypt)) /* need LLC, not yet generated */
		pTxBlk->NeedTrans = FALSE;
	else
#endif /* SOFT_ENCRYPT */
#ifdef CLIENT_WDS
	if (!MAC_ADDR_EQUAL((pTxBlk->pSrcBufHeader + MAC_ADDR_LEN), pAd->CurrentAddress))
		pTxBlk->NeedTrans = FALSE;
	else
#endif /* CLIENT_WDS */
#ifdef XLINK_SUPPORT
	if (pAd->StaCfg.PSPXlink ) /* Use SA as Address 2 */
		pTxBlk->NeedTrans = FALSE;
	else
#endif /* XLINK_SUPPORT */
	{
		pTxBlk->NeedTrans = TRUE;
#ifdef TXBF_SUPPORT
		pTxBlk->NeedTrans = FALSE;
#endif // TXBF_SUPPORT //
	}
#endif /* HDR_TRANS_TX_SUPPORT */

#ifdef PCIE_PS_SUPPORT
	if (RTMP_TEST_PSFLAG(pAd, fRTMP_PS_DISABLE_TX)) {
			DBGPRINT(RT_DEBUG_TRACE, ("drop pkt for PCIE-PS!!\n"));
			while (pTxBlk->TxPacketList.Head) {
				pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
				pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
				if (pPacket)
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
			return NDIS_STATUS_FAILURE;
	}
#endif /* PCIE_PS_SUPPORT */

#ifdef VHT_TXBF_SUPPORT			
	if ((pTxBlk->TxFrameType & TX_NDPA_FRAME) > 0)
	{
		UCHAR mlmeMCS, mlmeBW, mlmeMode;

		mlmeMCS  = pAd->CommonCfg.MlmeTransmit.field.MCS;
		mlmeBW   = pAd->CommonCfg.MlmeTransmit.field.BW;
		mlmeMode = pAd->CommonCfg.MlmeTransmit.field.MODE;
		
		pAd->NDPA_Request = TRUE;
	
		STA_NDPA_Frame_Tx(pAd, pTxBlk);
		
		pAd->NDPA_Request = FALSE;
		pTxBlk->TxFrameType &= ~TX_NDPA_FRAME;

		// Finish NDPA and then recover to mlme's own setting
		pAd->CommonCfg.MlmeTransmit.field.MCS  = mlmeMCS;
		pAd->CommonCfg.MlmeTransmit.field.BW   = mlmeBW;
		pAd->CommonCfg.MlmeTransmit.field.MODE = mlmeMode;
	}
#endif

#ifdef CFG_TDLS_SUPPORT

	if(IS_ENTRY_TDLS(pTxBlk->pMacEntry))
	{
		pTxBlk->Pid = PID_TDLS;
	}
#endif //CFG_TDLS_SUPPORT
#ifdef UAPSD_SUPPORT
	if(RTMP_GET_PACKET_UAPSD_Flag(pTxBlk->pPacket))
	{
		pTxBlk->Pid = PID_UAPSD;
	}
#endif
	switch (pTxBlk->TxFrameType)
	{
#ifdef DOT11_N_SUPPORT
	case TX_AMPDU_FRAME:
#ifdef HDR_TRANS_TX_SUPPORT
		if (pTxBlk->NeedTrans)
			STA_AMPDU_Frame_Tx_Hdr_Trns(pAd, pTxBlk);
		else
#endif /* HDR_TRANS_TX_SUPPORT */
		STA_AMPDU_Frame_Tx(pAd, pTxBlk);
		break;
#endif /* DOT11_N_SUPPORT */
	case TX_LEGACY_FRAME:
#ifdef HDR_TRANS_TX_SUPPORT
		if (pTxBlk->NeedTrans)
			STA_Legacy_Frame_Tx_Hdr_Trns(pAd, pTxBlk);
		else
#endif /* HDR_TRANS_TX_SUPPORT */
			STA_Legacy_Frame_Tx(pAd, pTxBlk);
		break;
	case TX_MCAST_FRAME:
#ifdef HDR_TRANS_TX_SUPPORT
		pTxBlk->NeedTrans = FALSE;
#endif /* HDR_TRANS_TX_SUPPORT */
		STA_Legacy_Frame_Tx(pAd, pTxBlk);
		break;
#ifdef DOT11_N_SUPPORT
	case TX_AMSDU_FRAME:
		STA_AMSDU_Frame_Tx(pAd, pTxBlk);
		break;
#endif /* DOT11_N_SUPPORT */
	case TX_RALINK_FRAME:
		STA_ARalink_Frame_Tx(pAd, pTxBlk);
		break;
	case TX_FRAG_FRAME:
		STA_Fragment_Frame_Tx(pAd, pTxBlk);
		break;
	default:
		{
			/* It should not happened! */
			DBGPRINT(RT_DEBUG_ERROR, ("Send a pacekt was not classified!!\n"));
			while (pTxBlk->TxPacketList.Head) {
				pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
				pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
				if (pPacket)
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
		}
		break;
	}

	return (NDIS_STATUS_SUCCESS);

}


/*
	========================================================================
	Routine Description:
	Arguments:
		pAd 	Pointer to our adapter

	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
VOID RTMPHandleTwakeupInterrupt(
	IN PRTMP_ADAPTER pAd)
{
#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("Twakeup Expired... !!!\n"));
#endif /* RELEASE_EXCLUDE */
	AsicForceWakeup(pAd, FALSE);
}


/*
========================================================================
Routine Description:
	This routine is used to do packet parsing and classification for Tx packet 
	to STA device, and it will en-queue packets to our TxSwQ depends on AC 
	class.
	
Arguments:
	pAd    		Pointer to our adapter
	pPacket 	Pointer to send packet

Return Value:
	NDIS_STATUS_SUCCESS		If succes to queue the packet into TxSwQ.
	NDIS_STATUS_FAILURE			If failed to do en-queue.

Note:
	You only can put OS-indepened & STA related code in here.
========================================================================
*/
INT STASendPacket(RTMP_ADAPTER *pAd, PNDIS_PACKET pPacket)
{
	PACKET_INFO PacketInfo;
	UCHAR *pSrcBufVA;
	UINT SrcBufLen, AllowFragSize;
	UCHAR NumberOfFrag;
	UCHAR QueIdx;
	UCHAR UserPriority;
	UCHAR Wcid;
	unsigned long IrqFlags = 0;
#ifdef MESH_SUPPORT
	BOOLEAN bMeshPkt = FALSE;
#endif /* MESH_SUPPORT */
	MAC_TABLE_ENTRY *pMacEntry = NULL;
	struct wifi_dev *wdev;
	STA_TR_ENTRY *tr_entry = NULL;
	
#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("==>%s()\n", __FUNCTION__));
#endif /* RELEASE_EXCLUDE */

	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);
	if ((pSrcBufVA == NULL) || (SrcBufLen <= 14))
	{
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		DBGPRINT(RT_DEBUG_ERROR, ("%s():pkt error(%p, %d)\n",
					__FUNCTION__, pSrcBufVA, SrcBufLen));
		return NDIS_STATUS_FAILURE;
	}

	Wcid = RTMP_GET_PACKET_WCID(pPacket);
	/* In HT rate adhoc mode, A-MPDU is often used. So need to lookup BA Table and MAC Entry. */
	/* Note multicast packets in adhoc also use BSSID_WCID index. */
#ifdef MESH_SUPPORT
	if (VALID_WCID(Wcid) && IS_ENTRY_MESH(&pAd->MacTab.Content[Wcid])) {
		bMeshPkt = TRUE;
		pMacEntry = &pAd->MacTab.Content[Wcid];
		tr_entry = &pAd->MacTab.tr_entry[Wcid];
		if ((pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
		    && (tr_entry->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
		    && (RTMP_GET_PACKET_EAPOL(pPacket) == FALSE)) {
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			return NDIS_STATUS_FAILURE;
		}
	}
	else
#endif /* MESH_SUPPORT */
	{
		if (pAd->StaCfg.BssType == BSS_INFRA) {
#if defined(QOS_DLS_SUPPORT) || defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
			if (VALID_WCID(Wcid) &&
			    (IS_ENTRY_DLS(&pAd->MacTab.Content[Wcid]) ||
			     IS_ENTRY_TDLS(&pAd->MacTab.Content[Wcid]))) {
				pMacEntry = &pAd->MacTab.Content[Wcid];
			} else
#endif /* defined(QOS_DLS_SUPPORT) || defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */
			{
				pMacEntry = &pAd->MacTab.Content[BSSID_WCID];
				RTMP_SET_PACKET_WCID(pPacket, BSSID_WCID);
			}
		} else if (ADHOC_ON(pAd)) {
			if (*pSrcBufVA & 0x01) {
				RTMP_SET_PACKET_WCID(pPacket, MCAST_WCID);
				pMacEntry = &pAd->MacTab.Content[MCAST_WCID];
			} else {
#ifdef XLINK_SUPPORT
				if (pAd->StaCfg.PSPXlink) {
					pMacEntry = &pAd->MacTab.Content[MCAST_WCID];
					pMacEntry->Aid = MCAST_WCID;
				} else
#endif /* XLINK_SUPPORT */
					pMacEntry = MacTableLookup(pAd, pSrcBufVA);

				if (pMacEntry)
					RTMP_SET_PACKET_WCID(pPacket, pMacEntry->wcid);
#ifdef ETH_CONVERT_SUPPORT
				else {
					RTMP_SET_PACKET_WCID(pPacket, MCAST_WCID);
				}
#endif /* ETH_CONVERT_SUPPORT */
			}
		}
	}

	if (!pMacEntry) {
		DBGPRINT(RT_DEBUG_ERROR,
			 ("%s():No such Addr(%2x:%2x:%2x:%2x:%2x:%2x) in MacTab\n",
			 __FUNCTION__, PRINT_MAC(pSrcBufVA)));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return NDIS_STATUS_FAILURE;
	}

	if (ADHOC_ON(pAd)
#ifdef MESH_SUPPORT
	    && (bMeshPkt == FALSE)
#endif /* MESH_SUPPORT */
	    ) {
		RTMP_SET_PACKET_WCID(pPacket, (UCHAR) pMacEntry->wcid);
	}

	wdev = &pAd->StaCfg.wdev;
DBGPRINT(RT_DEBUG_OFF, ("%s(): pMacEntry->wcid=%d\n", __FUNCTION__, pMacEntry->wcid));
	tr_entry = &pAd->MacTab.tr_entry[pMacEntry->wcid];
	
	/* Check the Ethernet Frame type of this packet, and set the RTMP_SET_PACKET_SPECIFIC flags. */
	/*              Here we set the PACKET_SPECIFIC flags(LLC, VLAN, DHCP/ARP, EAPOL). */
	UserPriority = 0;
	QueIdx = QID_AC_BE;
	RTMPCheckEtherType(pAd, pPacket, tr_entry, wdev, &UserPriority, &QueIdx, &pMacEntry->wcid);

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#ifdef UAPSD_SUPPORT
	/* the code must behind RTMPCheckEtherType() to get QueIdx */
	if ((pMacEntry != NULL) &&
		(pMacEntry->PsMode == PWR_SAVE) &&
		(pMacEntry->FlgPsModeIsWakeForAWhile == FALSE) &&
		(pMacEntry->bAPSDDeliverEnabledPerAC[QueIdx] == 0))
	{
		/* the packet will be sent to AP, not the peer directly */
		pMacEntry = &pAd->MacTab.Content[BSSID_WCID];

		/* for AP entry, pEntry->bAPSDDeliverEnabledPerAC[QueIdx] always is 0 */
		RTMP_SET_PACKET_WCID(pPacket, BSSID_WCID);
	}
#endif /* UAPSD_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */

#ifdef WAPI_SUPPORT
	/* Check the status of the controlled port and filter the outgoing frame for WAPI */
	if (((wdev->AuthMode == Ndis802_11AuthModeWAICERT) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWAIPSK))
	    && (wdev->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
	    && (RTMP_GET_PACKET_WAI(pPacket) == FALSE)
#ifdef MESH_SUPPORT
	    && bMeshPkt == FALSE
#endif /* MESH_SUPPORT */
	    ) {
		DBGPRINT(RT_DEBUG_TRACE,
			 ("%s():Drop packet before port secured!\n", __FUNCTION__));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);

		return NDIS_STATUS_FAILURE;
	}
#endif /* WAPI_SUPPORT */

	/* WPA 802.1x secured port control - drop all non-802.1x frame before port secured */
	if (((wdev->AuthMode == Ndis802_11AuthModeWPA) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWPAPSK) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWPA2) ||
	     (wdev->AuthMode == Ndis802_11AuthModeWPA2PSK)
#ifdef WPA_SUPPLICANT_SUPPORT
	     || (pAd->StaCfg.wdev.IEEE8021X == TRUE)
#endif /* WPA_SUPPLICANT_SUPPORT */
	    )
	    && ((wdev->PortSecured == WPA_802_1X_PORT_NOT_SECURED)
		|| (pAd->StaCfg.MicErrCnt >= 2))
	    && (RTMP_GET_PACKET_EAPOL(pPacket) == FALSE)
#ifdef MESH_SUPPORT
	    && bMeshPkt == FALSE
#endif /* MESH_SUPPORT */
	    ) {
		DBGPRINT(RT_DEBUG_TRACE,
			 ("%s():Drop packet before port secured!\n", __FUNCTION__));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);

		return NDIS_STATUS_FAILURE;
	}
#ifdef WSC_STA_SUPPORT
	if ((pAd->StaCfg.WscControl.WscConfMode != WSC_DISABLE) &&
	    (pAd->StaCfg.WscControl.bWscTrigger == TRUE) &&
	    (RTMP_GET_PACKET_EAPOL(pPacket) == FALSE)) {
		DBGPRINT(RT_DEBUG_TRACE,
			 ("%s():Drop packet before WPS process completed!\n", __FUNCTION__));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);

		return NDIS_STATUS_FAILURE;
	}
#endif /* WSC_STA_SUPPORT */

	/* 
		STEP 1. Decide number of fragments required to deliver this MSDU.
			The estimation here is not very accurate because difficult to
			take encryption overhead into consideration here. The result
			"NumberOfFrag" is then just used to pre-check if enough free
			TXD are available to hold this MSDU.
	*/
	if (*pSrcBufVA & 0x01)	/* fragmentation not allowed on multicast & broadcast */
		NumberOfFrag = 1;
	else if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE))
		NumberOfFrag = 1;	/* Aggregation overwhelms fragmentation */
	else if (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AMSDU_INUSED))
		NumberOfFrag = 1;	/* Aggregation overwhelms fragmentation */
#ifdef DOT11_N_SUPPORT
	else if ((wdev->HTPhyMode.field.MODE == MODE_HTMIX)
		 || (wdev->HTPhyMode.field.MODE == MODE_HTGREENFIELD))
		NumberOfFrag = 1;	/* MIMO RATE overwhelms fragmentation */
#endif /* DOT11_N_SUPPORT */
	else
	{
		/*
			The calculated "NumberOfFrag" is a rough estimation because of various
			encryption/encapsulation overhead not taken into consideration. This number is just
			used to make sure enough free TXD are available before fragmentation takes place.
			In case the actual required number of fragments of an NDIS packet
			excceeds "NumberOfFrag"caculated here and not enough free TXD available, the
			last fragment (i.e. last MPDU) will be dropped in RTMPHardTransmit() due to out of
			resource, and the NDIS packet will be indicated NDIS_STATUS_FAILURE. This should
			rarely happen and the penalty is just like a TX RETRY fail. Affordable.
		*/
		UINT32 Size;

		AllowFragSize = (pAd->CommonCfg.FragmentThreshold) - LENGTH_802_11 - LENGTH_CRC;
		Size = PacketInfo.TotalPacketLength - LENGTH_802_3 + LENGTH_802_1_H;
		NumberOfFrag = (UCHAR)((Size / AllowFragSize) + 1);
		/* To get accurate number of fragmentation, Minus 1 if the size just match to allowable fragment size */
		if ((Size % AllowFragSize) == 0) {
			NumberOfFrag--;
		}
	}
	/* Save fragment number to Ndis packet reserved field */
	RTMP_SET_PACKET_FRAGMENTS(pPacket, NumberOfFrag);

{
	BOOLEAN RTSRequired;

	/*
		STEP 2. Check the requirement of RTS; decide packet TX rate
		If multiple fragment required, RTS is required only for the first fragment
		if the fragment size large than RTS threshold
	 */
	if (NumberOfFrag > 1)
		RTSRequired = (pAd->CommonCfg.FragmentThreshold > pAd->CommonCfg.RtsThreshold) ? 1 : 0;
	else
		RTSRequired = (PacketInfo.TotalPacketLength > pAd->CommonCfg.RtsThreshold) ? 1 : 0;
	RTMP_SET_PACKET_RTS(pPacket, RTSRequired);
}

	RTMP_SET_PACKET_UP(pPacket, UserPriority);

#ifdef ETH_CONVERT_SUPPORT
	if (pAd->StaCfg.bFragFlag == TRUE) {
		static QUEUE_HEADER queue1[4], queue2[4];
		static BOOLEAN is_init = 0;
		static INT32 ID1[4] = { -1, -1, -1, -1 }, ID2[4] = {-1, -1, -1, -1};
		static INT32 ID1_FragSize[4] = { -1, -1, -1, -1 }, ID2_FragSize[4] = {-1, -1, -1, -1};
		static ULONG jiffies1[4], jiffies2[4];

		QUEUE_HEADER *pAC_Queue1, *pAC_Queue2;
		INT32 *pAC_ID1, *pAC_ID2, *pAC_ID1_FragSize, *pAC_ID2_FragSize;
		ULONG *pAC_Jiffies1, *pAC_Jiffies2;

		UINT32 i;
		typedef union ip_flags_frag_offset {
			struct {
#ifdef RT_BIG_ENDIAN
				USHORT flags_reserved:1;
				USHORT flags_may_frag:1;
				USHORT flags_more_frag:1;
				USHORT frag_offset:13;
#else
				USHORT frag_offset:13;
				USHORT flags_more_frag:1;
				USHORT flags_may_frag:1;
				USHORT flags_reserved:1;
#endif
			} field;
			USHORT word;
		} IP_FLAGS_FRAG_OFFSET;

		typedef struct ip_v4_hdr {
#ifdef RT_BIG_ENDIAN
			UCHAR version:4, ihl:4;
#else
			UCHAR ihl:4, version:4;
#endif
			UCHAR tos;
			USHORT tot_len;
			USHORT identifier;
		} IP_V4_HDR;

		IP_V4_HDR *pIpv4Hdr, Ipv4Hdr;
		IP_FLAGS_FRAG_OFFSET *pFlags_frag_offset, Flags_frag_offset;
		ULONG Now;

		if (is_init == 0) {
			for (i = 0; i < 4; i++) {
				InitializeQueueHeader(&queue1[i]);
				InitializeQueueHeader(&queue2[i]);
			}
			is_init = 1;
		}

		pAC_Queue1 = &queue1[QueIdx];
		pAC_Queue2 = &queue2[QueIdx];
		pAC_ID1 = &ID1[QueIdx];
		pAC_ID2 = &ID2[QueIdx];
		pAC_ID1_FragSize = &ID1_FragSize[QueIdx];
		pAC_ID2_FragSize = &ID2_FragSize[QueIdx];
		pAC_Jiffies1 = &jiffies1[QueIdx];
		pAC_Jiffies2 = &jiffies2[QueIdx];

		NdisGetSystemUpTime(&Now);
#if 0
		/*if ((pAC_Queue1->Number != 0) && (Now > (*pAC_Jiffies1)+1000)) */
#endif
		if ((pAC_Queue1->Number != 0)
		    && (RTMP_TIME_AFTER(Now, (*pAC_Jiffies1) + (1000 * OS_HZ))))
		{
			PQUEUE_ENTRY pBackupPktEntry;
			PNDIS_PACKET pBackupPkt;

			while (1) {
				pBackupPktEntry = RemoveHeadQueue(pAC_Queue1);
				if (pBackupPktEntry == NULL)
					break;
				pBackupPkt = QUEUE_ENTRY_TO_PACKET(pBackupPktEntry);
				RELEASE_NDIS_PACKET(pAd, pBackupPkt, NDIS_STATUS_FAILURE);
			}

			*pAC_ID1 = -1;
			*pAC_ID1_FragSize = -1;
			*pAC_Jiffies1 = 0;
		}

		NdisGetSystemUpTime(&Now);
		if ((pAC_Queue2->Number != 0)
		    && (RTMP_TIME_AFTER(Now, (*pAC_Jiffies2) + (1000 * OS_HZ))))
		{
			PQUEUE_ENTRY pBackupPktEntry;
			PNDIS_PACKET pBackupPkt;

			while (1) {
				pBackupPktEntry = RemoveHeadQueue(pAC_Queue2);
				if (pBackupPktEntry == NULL)
					break;
				pBackupPkt = QUEUE_ENTRY_TO_PACKET(pBackupPktEntry);
				RELEASE_NDIS_PACKET(pAd, pBackupPkt, NDIS_STATUS_FAILURE);
			}

			*pAC_ID2 = -1;
			*pAC_ID2_FragSize = -1;
			*pAC_Jiffies2 = 0;
		}

		if (RTMP_GET_PACKET_IPV4(pPacket) &&
		    ((pAd->TxSwQueue[QueIdx].Number >= (pAd->TxSwQMaxLen >> 1)) ||
				((pAC_Queue1->Number) || (pAC_Queue2->Number)))
		    ) {
			pIpv4Hdr = (IP_V4_HDR *) (PacketInfo.pFirstBuffer + LENGTH_802_3);
			pFlags_frag_offset =
			    (IP_FLAGS_FRAG_OFFSET *) (PacketInfo.pFirstBuffer + LENGTH_802_3 + 6);
			Flags_frag_offset.word = ntohs(pFlags_frag_offset->word);
			Ipv4Hdr.identifier = ntohs(pIpv4Hdr->identifier);
			Ipv4Hdr.tot_len = ntohs(pIpv4Hdr->tot_len);
			Ipv4Hdr.ihl = pIpv4Hdr->ihl;

			/* check if 1st fragment */
			if ((Flags_frag_offset.field.flags_more_frag == 1) &&
			    (Flags_frag_offset.field.frag_offset == 0)) {
				NdisGetSystemUpTime(&Now);
				if ((*pAC_ID1) == -1) {
					*pAC_ID1 = Ipv4Hdr.identifier;
					*pAC_Jiffies1 = Now;
					*pAC_ID1_FragSize = Ipv4Hdr.tot_len - (Ipv4Hdr.ihl * 4);
					InsertTailQueue(pAC_Queue1, PACKET_TO_QUEUE_ENTRY(pPacket));
				} else if ((*pAC_ID2) == -1) {
					*pAC_ID2 = Ipv4Hdr.identifier;
					*pAC_Jiffies2 = Now;
					*pAC_ID2_FragSize = Ipv4Hdr.tot_len - (Ipv4Hdr.ihl * 4);
					InsertTailQueue(pAC_Queue2, PACKET_TO_QUEUE_ENTRY(pPacket));
				} else {
					QUEUE_HEADER *pTempqueue;
					PQUEUE_ENTRY pBackupPktEntry;
					PNDIS_PACKET pBackupPkt;

					/* free backup fragments */
					if ((*pAC_Jiffies1) > (*pAC_Jiffies2)) {
						pTempqueue = pAC_Queue2;
						*pAC_ID2 = Ipv4Hdr.identifier;
					} else {
						pTempqueue = pAC_Queue1;
						*pAC_ID1 = Ipv4Hdr.identifier;
					}

					if (pTempqueue->Number != 0) {
						while (1) {
							pBackupPktEntry = RemoveHeadQueue(pTempqueue);
							if (pBackupPktEntry == NULL)
								break;
							pBackupPkt = QUEUE_ENTRY_TO_PACKET(pBackupPktEntry);
							RELEASE_NDIS_PACKET(pAd, pBackupPkt, NDIS_STATUS_FAILURE);
						}
					}
				}
			} else {
				/* check if last fragment */
				if ((Ipv4Hdr.identifier == (*pAC_ID1))
				    || (Ipv4Hdr.identifier == (*pAC_ID2))) {
					QUEUE_HEADER *pTempqueue;
					PQUEUE_ENTRY pBackupPktEntry;
					PNDIS_PACKET pBackupPkt;

					if (Ipv4Hdr.identifier == (*pAC_ID1)) {
						InsertTailQueue(pAC_Queue1, PACKET_TO_QUEUE_ENTRY(pPacket));
						pTempqueue = pAC_Queue1;
					} else {
						InsertTailQueue(pAC_Queue2, PACKET_TO_QUEUE_ENTRY(pPacket));
						pTempqueue = pAC_Queue2;
					}

					/* the last fragment */
					if ((Flags_frag_offset.field.flags_more_frag == 0)
					    && (Flags_frag_offset.field.frag_offset != 0)) {
						UINT32 fragment_count = 0;
						BOOLEAN bDrop = FALSE;
						if (Ipv4Hdr.identifier == (*pAC_ID1)) {
							fragment_count = ((Flags_frag_offset.field.frag_offset * 8) / (*pAC_ID1_FragSize)) + 1;
							if (pTempqueue->Number != fragment_count)
								bDrop = TRUE;
							*pAC_ID1 = -1;
						}

						if (Ipv4Hdr.identifier == (*pAC_ID2)) {
							fragment_count = ((Flags_frag_offset.field.frag_offset * 8) / (*pAC_ID2_FragSize)) + 1;
							if (pTempqueue->Number != fragment_count)
								bDrop = TRUE;
							*pAC_ID2 = -1;
						}

						/* if number does not equal coreect fragment count or no enough space, */
						/* free backup fragments to SwTxQueue[] */
						if ((bDrop == TRUE) ||
						    (pAd->TxSwQueue[QueIdx].Number > (pAd->TxSwQMaxLen - pTempqueue->Number))) {
							while (1) {
								pBackupPktEntry = RemoveHeadQueue(pTempqueue);
								if (pBackupPktEntry == NULL)
									break;
								pBackupPkt = QUEUE_ENTRY_TO_PACKET(pBackupPktEntry);
								RELEASE_NDIS_PACKET(pAd, pBackupPkt, NDIS_STATUS_FAILURE);
							}
							return
							    NDIS_STATUS_SUCCESS;
						}

						/* move backup fragments to SwTxQueue[] */
						NdisAcquireSpinLock(&pAd->TxSwQueueLock[QueIdx]);
						while (1) {
							pBackupPktEntry = RemoveHeadQueue(pTempqueue);
							if (pBackupPktEntry == NULL)
								break;
							InsertTailQueueAc(pAd, pMacEntry, &pAd->TxSwQueue[QueIdx], pBackupPktEntry);
						}
						NdisReleaseSpinLock(&pAd->TxSwQueueLock[QueIdx]);
					}
				} else {

					/*
					   bypass none-fist fragmented packets (we should not drop this packet)
					   when 
					   1. (pAd->TxSwQueue[QueIdx].Number >= (pAd->TxSwQMaxLen[QueIdx] >> 1)
					   and 
					   2. two fragmented buffer are empty 
					 */
					if (((*pAC_ID1) == -1)
					    && ((*pAC_ID2) == -1)) {
						goto normal_drop;
					}

					if ((Flags_frag_offset.field.flags_more_frag != 0)
					    && (Flags_frag_offset.field.frag_offset != 0)) {
						NdisAcquireSpinLock(&pAd->TxSwQueueLock[QueIdx]);
						RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
						NdisReleaseSpinLock(&pAd->TxSwQueueLock[QueIdx]);
						return NDIS_STATUS_SUCCESS;
					} else {
						goto normal_drop;
					}
				}
			}
		} else
			goto normal_drop;
	}
	else
	{
	      normal_drop:
#endif /* ETH_CONVERT_SUPPORT */

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#ifdef UAPSD_SUPPORT
	if ((pMacEntry != NULL) &&
		(pMacEntry->PsMode == PWR_SAVE) &&
		(pMacEntry->FlgPsModeIsWakeForAWhile == FALSE) &&
		(pMacEntry->bAPSDDeliverEnabledPerAC[QueIdx] != 0))
	{
		/*
			only UAPSD of the peer for the queue is set;
			for non-UAPSD queue, we will send it to the AP.
		*/
		if (RtmpInsertPsQueue(pAd, pPacket, pMacEntry, QueIdx) != NDIS_STATUS_SUCCESS)
			return NDIS_STATUS_FAILURE;
	}
	else
		/* non-PS mode or send PS packet to AP */
#endif /* UAPSD_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */
	{
		/* Make sure SendTxWait queue resource won't be used by other threads */
		RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
		if (pAd->TxSwQueue[QueIdx].Number >= pAd->TxSwQMaxLen) {
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#ifdef BLOCK_NET_IF
			StopNetIfQueue(pAd, QueIdx, pPacket);
#endif /* BLOCK_NET_IF */
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);

			return NDIS_STATUS_FAILURE;
		} else {
				InsertTailQueueAc(pAd, pMacEntry, &pAd->TxSwQueue[QueIdx], PACKET_TO_QUEUE_ENTRY(pPacket));
		}
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
	}
#ifdef ETH_CONVERT_SUPPORT
	}
#endif /* ETH_CONVERT_SUPPORT */

#ifdef DOT11_N_SUPPORT
	RTMP_BASetup(pAd, tr_entry, UserPriority);
#endif /* DOT11_N_SUPPORT */

	pAd->RalinkCounters.OneSecOsTxCount[QueIdx]++;	/* TODO: for debug only. to be removed */
	return NDIS_STATUS_SUCCESS;
}

