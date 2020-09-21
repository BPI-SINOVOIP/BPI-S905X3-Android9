/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2012, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    ap_repeater.c

    Abstract:
    Support MAC Repeater function.

    Revision History:
    Who             		When              What
    --------------  ----------      ----------------------------------------------
    Arvin				11-16-2012      created
*/

#ifdef MAC_REPEATER_SUPPORT

#include "rt_config.h"


REPEATER_CLIENT_ENTRY *RTMPLookupRepeaterCliEntry(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN bRealMAC,
	IN PUCHAR pAddr)
{
	ULONG HashIdx;
	UCHAR tempMAC[6];
	REPEATER_CLIENT_ENTRY *pEntry = NULL;
	REPEATER_CLIENT_ENTRY_MAP *pMapEntry = NULL;

	NdisAcquireSpinLock(&pAd->ApCfg.ReptCliEntryLock);
	COPY_MAC_ADDR(tempMAC, pAddr);
	HashIdx = MAC_ADDR_HASH_INDEX(tempMAC);

	if (bRealMAC == TRUE)
	{
		pMapEntry = pAd->ApCfg.ReptMapHash[HashIdx];
		while (pMapEntry)
		{
			pEntry = pMapEntry->pReptCliEntry;

			if (pEntry->CliValid && MAC_ADDR_EQUAL(pEntry->OriginalAddress, tempMAC))
				break;
			else
			{
				pEntry = NULL;
				pMapEntry = pMapEntry->pNext;
			}
		}
	}
	else
	{
		pEntry = pAd->ApCfg.ReptCliHash[HashIdx];
		while (pEntry)
		{
			if (pEntry->CliValid && MAC_ADDR_EQUAL(pEntry->CurrentAddress, tempMAC))
				break;
			else
				pEntry = pEntry->pNext;
		}
	}
	NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);

	return pEntry;
}

#if 0
VOID RTMPInsertRepeaterAsicEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR CliIdx,
	IN PUCHAR pAddr)
{
	ULONG offset, Addr;
	UCHAR tempMAC[MAC_ADDR_LEN];

	DBGPRINT(RT_DEBUG_WARN, (" %s.\n", __FUNCTION__));

	COPY_MAC_ADDR(tempMAC, pAddr);
	
	offset = 0x1480 + (HW_WCID_ENTRY_SIZE * CliIdx);	
	Addr = tempMAC[0] + (tempMAC[1] << 8) +(tempMAC[2] << 16) +(tempMAC[3] << 24);
	RTMP_IO_WRITE32(pAd, offset, Addr);
	Addr = tempMAC[4] + (tempMAC[5] << 8);
	RTMP_IO_WRITE32(pAd, offset + 4, Addr); 

	DBGPRINT(RT_DEBUG_ERROR, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n", 
							PRINT_MAC(tempMAC), CliIdx));

}

VOID RTMPRemoveRepeaterAsicEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR CliIdx)
{
	ULONG offset, Addr;

	DBGPRINT(RT_DEBUG_WARN, (" %s.\n", __FUNCTION__));

	offset = 0x1480 + (HW_WCID_ENTRY_SIZE * CliIdx);
	Addr = 0;
	RTMP_IO_WRITE32(pAd, offset, Addr);
	RTMP_IO_WRITE32(pAd, offset + 4, Addr);
}
#endif

VOID RTMPInsertRepeaterEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR apidx,
	IN PUCHAR pAddr)
{
	INT CliIdx, idx;
	UCHAR HashIdx;
	BOOLEAN Cancelled;
	UCHAR tempMAC[MAC_ADDR_LEN];
	APCLI_CTRL_MSG_STRUCT ApCliCtrlMsg;
	PREPEATER_CLIENT_ENTRY pReptCliEntry = NULL, pCurrEntry = NULL;
	PREPEATER_CLIENT_ENTRY_MAP pReptCliMap;
	UCHAR SPEC_ADDR[6][3] = {{0x02, 0x0F, 0xB5}, {0x02, 0x09, 0x5B},
								{0x02, 0x14, 0x6C}, {0x02, 0x18, 0x4D},
								{0x02, 0x1B, 0x2F}, {0x02, 0x1E, 0x2A}};

	DBGPRINT(RT_DEBUG_TRACE, (" %s.\n", __FUNCTION__));

	NdisAcquireSpinLock(&pAd->ApCfg.ReptCliEntryLock);

	if (pAd->ApCfg.RepeaterCliSize >= MAX_EXT_MAC_ADDR_SIZE)
	{
		DBGPRINT(RT_DEBUG_ERROR, (" Repeater Client Full !!!\n"));
		NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);
		return ;
	}

	if (pAd->ApCfg.ApCliTab[apidx].Enable == FALSE)
	{
		DBGPRINT(RT_DEBUG_ERROR, (" ApCli Interface is Down !!!\n"));
		NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);
		return ;
	}

	for (CliIdx = 0; CliIdx < MAX_EXT_MAC_ADDR_SIZE; CliIdx++)
	{
		pReptCliEntry = &pAd->ApCfg.ApCliTab[apidx].RepeaterCli[CliIdx];

		if ((pReptCliEntry->CliEnable) && 
			(MAC_ADDR_EQUAL(pReptCliEntry->OriginalAddress, pAddr) || MAC_ADDR_EQUAL(pReptCliEntry->CurrentAddress, pAddr)))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("\n  receive mac :%02x:%02x:%02x:%02x:%02x:%02x !!!\n", 
						pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5]));
			DBGPRINT(RT_DEBUG_ERROR, (" duplicate Insert !!!\n"));
			NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);
			return ;
		}

		if (pReptCliEntry->CliEnable == FALSE)
			break;
	}

	if (CliIdx >= MAX_EXT_MAC_ADDR_SIZE)
	{
		DBGPRINT(RT_DEBUG_ERROR, (" Repeater Client Full !!!\n"));
		NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);
		return ;
	}

	pReptCliEntry = &pAd->ApCfg.ApCliTab[apidx].RepeaterCli[CliIdx];
	pReptCliMap = &pAd->ApCfg.ApCliTab[apidx].RepeaterCliMap[CliIdx];

	/* ENTRY PREEMPTION: initialize the entry */
	RTMPCancelTimer(&pReptCliEntry->ApCliAuthTimer, &Cancelled);
	RTMPCancelTimer(&pReptCliEntry->ApCliAssocTimer, &Cancelled);
	pReptCliEntry->CtrlCurrState = APCLI_CTRL_DISCONNECTED;
	pReptCliEntry->AuthCurrState = APCLI_AUTH_REQ_IDLE;
	pReptCliEntry->AssocCurrState = APCLI_ASSOC_IDLE;
	pReptCliEntry->CliConnectState = 0;
	pReptCliEntry->CliValid = FALSE;
	pReptCliEntry->bEthCli = FALSE;
	pReptCliEntry->MacTabWCID = 0xFF;
	pReptCliEntry->AuthReqCnt = 0;
	pReptCliEntry->AssocReqCnt = 0;
	pReptCliEntry->CliTriggerTime = 0;
	pReptCliEntry->pNext = NULL;
	pReptCliMap->pReptCliEntry = pReptCliEntry;
	pReptCliMap->pNext = NULL;

	COPY_MAC_ADDR(pReptCliEntry->OriginalAddress, pAddr);
	COPY_MAC_ADDR(tempMAC, pAddr);

	if (pAd->ApCfg.MACRepeaterOuiMode == 1)
	{
		DBGPRINT(RT_DEBUG_ERROR, (" todo !!!\n"));
	}
	else if (pAd->ApCfg.MACRepeaterOuiMode == 2)
	{
		INT IdxToUse;
		
		for (idx = 0; idx < 6; idx++)
		{
			if (RTMPEqualMemory(SPEC_ADDR[idx], pAddr, 3))
				break;
		}

		/* If there is a matched one, use the next one; otherwise, use the first one. */
		if (idx >= 0  && idx < 5)
			IdxToUse = idx + 1;
		else 
			IdxToUse = 0;
		NdisCopyMemory(tempMAC, SPEC_ADDR[IdxToUse], 3);
	}
	else
	{
		NdisCopyMemory(tempMAC, pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, 3);
	}

	COPY_MAC_ADDR(pReptCliEntry->CurrentAddress, tempMAC);
	pReptCliEntry->CliEnable = TRUE;
	pReptCliEntry->CliConnectState = 1;
	pReptCliEntry->pNext = NULL;
	NdisGetSystemUpTime(&pReptCliEntry->CliTriggerTime);

	RTMPInsertRepeaterAsicEntry(pAd, CliIdx, tempMAC);
		
	HashIdx = MAC_ADDR_HASH_INDEX(tempMAC);
	if (pAd->ApCfg.ReptCliHash[HashIdx] == NULL)
	{
		pAd->ApCfg.ReptCliHash[HashIdx] = pReptCliEntry;
	}
	else
	{
		pCurrEntry = pAd->ApCfg.ReptCliHash[HashIdx];
#if 0
		if (pCurrEntry == pReptCliEntry)			
		{
			NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);
			return;
		}			
#endif
		while (pCurrEntry->pNext != NULL)
			pCurrEntry = pCurrEntry->pNext;
		pCurrEntry->pNext = pReptCliEntry;
	}

	HashIdx = MAC_ADDR_HASH_INDEX(pReptCliEntry->OriginalAddress);
	if (pAd->ApCfg.ReptMapHash[HashIdx] == NULL)
		pAd->ApCfg.ReptMapHash[HashIdx] = pReptCliMap;
	else
	{
		PREPEATER_CLIENT_ENTRY_MAP pCurrMapEntry;
	
		pCurrMapEntry = pAd->ApCfg.ReptMapHash[HashIdx];

		while (pCurrMapEntry->pNext != NULL)
			pCurrMapEntry = pCurrMapEntry->pNext;
		pCurrMapEntry->pNext = pReptCliMap;
	}

	pAd->ApCfg.RepeaterCliSize++;
	NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);

	NdisZeroMemory(&ApCliCtrlMsg, sizeof(APCLI_CTRL_MSG_STRUCT));
	ApCliCtrlMsg.Status = MLME_SUCCESS;
	COPY_MAC_ADDR(&ApCliCtrlMsg.SrcAddr[0], tempMAC);
	ApCliCtrlMsg.BssIdx = apidx;
	ApCliCtrlMsg.CliIdx = CliIdx;

	MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_MT2_AUTH_REQ,
			sizeof(APCLI_CTRL_MSG_STRUCT), &ApCliCtrlMsg, apidx);

}

VOID RTMPRemoveRepeaterEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR func_tb_idx,
	IN UCHAR CliIdx)
{
	USHORT HashIdx;
	REPEATER_CLIENT_ENTRY *pEntry, *pPrevEntry, *pProbeEntry;
	REPEATER_CLIENT_ENTRY_MAP *pMapEntry, *pPrevMapEntry, *pProbeMapEntry;
	BOOLEAN bVaild;

	DBGPRINT(RT_DEBUG_ERROR, (" %s.\n", __FUNCTION__));

	RTMPRemoveRepeaterAsicEntry(pAd, CliIdx);

	NdisAcquireSpinLock(&pAd->ApCfg.ReptCliEntryLock);
	pEntry = &pAd->ApCfg.ApCliTab[func_tb_idx].RepeaterCli[CliIdx];

	bVaild = TRUE;

	HashIdx = MAC_ADDR_HASH_INDEX(pEntry->CurrentAddress);

	pPrevEntry = NULL;
	pProbeEntry = pAd->ApCfg.ReptCliHash[HashIdx];

	ASSERT(pProbeEntry);

	if (pProbeEntry == NULL)
	{
		bVaild = FALSE;
		goto done;
	}

	if (pProbeEntry != NULL)
	{
		/* update Hash list*/
		do
		{
			if (pProbeEntry == pEntry)
			{
				if (pPrevEntry == NULL)
				{
					pAd->ApCfg.ReptCliHash[HashIdx] = pEntry->pNext;
				}
				else
				{
					pPrevEntry->pNext = pEntry->pNext;
				}
				break;
			}

			pPrevEntry = pProbeEntry;
			pProbeEntry = pProbeEntry->pNext;
		} while (pProbeEntry);
	}

	/* not found !!!*/
	ASSERT(pProbeEntry != NULL);

	if (pProbeEntry == NULL)
	{
		bVaild = FALSE;
		goto done;
	}

	pMapEntry = &pAd->ApCfg.ApCliTab[func_tb_idx].RepeaterCliMap[CliIdx];

	HashIdx = MAC_ADDR_HASH_INDEX(pEntry->OriginalAddress);

	pPrevMapEntry = NULL;
	pProbeMapEntry = pAd->ApCfg.ReptMapHash[HashIdx];
	ASSERT(pProbeMapEntry);
	if (pProbeMapEntry != NULL)
	{
		/* update Hash list*/
		do
		{
			if (pProbeMapEntry == pMapEntry)
			{
				if (pPrevMapEntry == NULL)
				{
					pAd->ApCfg.ReptMapHash[HashIdx] = pMapEntry->pNext;
				}
				else
				{
					pPrevMapEntry->pNext = pMapEntry->pNext;
				}
				break;
			}

			pPrevMapEntry = pProbeMapEntry;
			pProbeMapEntry = pProbeMapEntry->pNext;
		} while (pProbeMapEntry);
	}
	/* not found !!!*/
	ASSERT(pProbeMapEntry != NULL);

done:

	pAd->ApCfg.ApCliTab[func_tb_idx].RepeaterCli[CliIdx].CliConnectState = 0;
	NdisZeroMemory(pAd->ApCfg.ApCliTab[func_tb_idx].RepeaterCli[CliIdx].OriginalAddress, MAC_ADDR_LEN);

	if (bVaild == TRUE)
		pAd->ApCfg.RepeaterCliSize--;

	NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);

	return;
}

MAC_TABLE_ENTRY *RTMPInsertRepeaterMacEntry(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR *pAddr,
	IN struct wifi_dev *wdev,
	IN  UCHAR apIdx,
	IN  UCHAR cliIdx,
	IN BOOLEAN CleanAll)
{
	UCHAR HashIdx;
	int i;
	MAC_TABLE_ENTRY *pEntry = NULL, *pCurrEntry;
	BOOLEAN Cancelled;

	if (pAd->MacTab.Size >= MAX_LEN_OF_MAC_TABLE)
		return NULL;

	/* allocate one MAC entry*/
	NdisAcquireSpinLock(&pAd->MacTabLock);

	i = (MAX_NUMBER_OF_MAC + ((MAX_EXT_MAC_ADDR_SIZE + 1) * (apIdx - MIN_NET_DEVICE_FOR_APCLI)));

	if (cliIdx != 0xFF)
		i  = i + cliIdx + 1;

	/* pick up the first available vacancy*/
	if (IS_ENTRY_NONE(&pAd->MacTab.Content[i]))
	{
		pEntry = &pAd->MacTab.Content[i];

		/* ENTRY PREEMPTION: initialize the entry */
		if (pEntry->RetryTimer.Valid)
			RTMPCancelTimer(&pEntry->RetryTimer, &Cancelled);
		if (pEntry->EnqueueStartForPSKTimer.Valid)
			RTMPCancelTimer(&pEntry->EnqueueStartForPSKTimer, &Cancelled);
#ifdef DOT11W_PMF_SUPPORT
		RTMPCancelTimer(&pEntry->SAQueryTimer, &Cancelled);
		RTMPCancelTimer(&pEntry->SAQueryConfirmTimer, &Cancelled);
#endif /* DOT11W_PMF_SUPPORT */

		NdisZeroMemory(pEntry, sizeof(MAC_TABLE_ENTRY));

		if (CleanAll == TRUE)
		{
			pEntry->MaxSupportedRate = RATE_11;
			pEntry->CurrTxRate = RATE_11;
			NdisZeroMemory(pEntry, sizeof(MAC_TABLE_ENTRY));
			pEntry->PairwiseKey.KeyLen = 0;
			pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
		}

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
		if (apIdx >= MIN_NET_DEVICE_FOR_APCLI)
		{
			SET_ENTRY_APCLI(pEntry);
		}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

		pEntry->wdev = wdev;
		pEntry->wcid = i;
		//SET_ENTRY_AP(pEntry);//Carter, why set Apcli Entry then set to AP entry?
		pAd->MacTab.tr_entry[i].isCached = FALSE;
		//tr_entry->isCached = FALSE;
		pEntry->bIAmBadAtheros = FALSE;

		RTMPInitTimer(pAd, &pEntry->EnqueueStartForPSKTimer, GET_TIMER_FUNCTION(EnqueueStartForPSKExec), pEntry, FALSE);

#ifdef APCLI_SUPPORT
		if (IS_ENTRY_APCLI(pEntry))
		{
			RTMPInitTimer(pAd, &pEntry->RetryTimer, GET_TIMER_FUNCTION(WPARetryExec), pEntry, FALSE);
		}
#endif /* APCLI_SUPPORT */
		

#ifdef TXBF_SUPPORT
		if (pAd->chipCap.FlgHwTxBfCap)
			RTMPInitTimer(pAd, &pEntry->eTxBfProbeTimer, GET_TIMER_FUNCTION(eTxBfProbeTimerExec), pEntry, FALSE);
#endif /* TXBF_SUPPORT */

		pEntry->pAd = pAd;
		pEntry->CMTimerRunning = FALSE;
		pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_DISABLE;
		pEntry->RSNIE_Len = 0;
		NdisZeroMemory(pEntry->R_Counter, sizeof(pEntry->R_Counter));
		pEntry->ReTryCounter = PEER_MSG1_RETRY_TIMER_CTR;
		pEntry->func_tb_idx = (apIdx - MIN_NET_DEVICE_FOR_APCLI);

		if (IS_ENTRY_APCLI(pEntry))
			pEntry->apidx = (apIdx - MIN_NET_DEVICE_FOR_APCLI);

		pEntry->pMbss = NULL;

#ifdef APCLI_SUPPORT
		if (IS_ENTRY_APCLI(pEntry))
		{
			pEntry->AuthMode = pAd->ApCfg.ApCliTab[pEntry->func_tb_idx].wdev.AuthMode;
			pEntry->WepStatus = pAd->ApCfg.ApCliTab[pEntry->func_tb_idx].wdev.WepStatus;

			if (pEntry->AuthMode < Ndis802_11AuthModeWPA)
			{
				pEntry->WpaState = AS_NOTUSE;
				pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
			}
			else
			{
				pEntry->WpaState = AS_PTKSTART;
				pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
			}
		}
#endif /* APCLI_SUPPORT */	

		pEntry->GTKState = REKEY_NEGOTIATING;
		pEntry->PairwiseKey.KeyLen = 0;
		pEntry->PairwiseKey.CipherAlg = CIPHER_NONE;
		pAd->MacTab.tr_entry[i].PortSecured = WPA_802_1X_PORT_NOT_SECURED;
		//pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;

		pEntry->PMKID_CacheIdx = ENTRY_NOT_FOUND;
		COPY_MAC_ADDR(pEntry->Addr, pAddr);

#ifdef APCLI_SUPPORT
		if (IS_ENTRY_APCLI(pEntry))
		{
			COPY_MAC_ADDR(pEntry->bssid, pAddr);
		}
#endif // APCLI_SUPPORT //

		pEntry->Sst = SST_NOT_AUTH;
		pEntry->AuthState = AS_NOT_AUTH;
		pEntry->Aid = (USHORT)i;
		pEntry->CapabilityInfo = 0;
		pEntry->PsMode = PWR_ACTIVE;
		pAd->MacTab.tr_entry[i].PsQIdleCount = 0;
		pAd->MacTab.tr_entry[i].PsTokenFlag = 0;
		//pEntry->PsQIdleCount = 0;
		pEntry->NoDataIdleCount = 0;
		pEntry->AssocDeadLine = MAC_TABLE_ASSOC_TIMEOUT;
		pEntry->ContinueTxFailCnt = 0;
		pEntry->TimeStamp_toTxRing = 0;
		// TODO: shiang-usw,  remove upper setting becasue we need to migrate to tr_entry!
		pAd->MacTab.tr_entry[i].PsMode = PWR_ACTIVE;
		pAd->MacTab.tr_entry[i].NoDataIdleCount = 0;
		pAd->MacTab.tr_entry[i].ContinueTxFailCnt = 0;
		pAd->MacTab.tr_entry[i].LockEntryTx = FALSE;
		pAd->MacTab.tr_entry[i].TimeStamp_toTxRing = 0;

		pAd->MacTab.Size ++;

		/* Set the security mode of this entry as OPEN-NONE in ASIC */
		RTMP_REMOVE_PAIRWISE_KEY_ENTRY(pAd, (UCHAR)i);

		/* Add this entry into ASIC RX WCID search table */
		RTMP_STA_ENTRY_ADD(pAd, pEntry);

#ifdef TXBF_SUPPORT
		if (pAd->chipCap.FlgHwTxBfCap)
			NdisAllocateSpinLock(pAd, &pEntry->TxSndgLock);
#endif /* TXBF_SUPPORT */

		DBGPRINT(RT_DEBUG_TRACE, ("%s - allocate entry #%d, Aid = %d, Total= %d\n",__FUNCTION__, i, pEntry->Aid, pAd->MacTab.Size));

	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s - exist entry #%d, Aid = %d, Total= %d\n", __FUNCTION__, i, pEntry->Aid, pAd->MacTab.Size));
		NdisReleaseSpinLock(&pAd->MacTabLock);
		return pEntry;
	}

	/* add this MAC entry into HASH table */
	if (pEntry)
	{
		HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
		if (pAd->MacTab.Hash[HashIdx] == NULL)
		{
			pAd->MacTab.Hash[HashIdx] = pEntry;
		}
		else
		{
			pCurrEntry = pAd->MacTab.Hash[HashIdx];
			while (pCurrEntry->pNext != NULL)
				pCurrEntry = pCurrEntry->pNext;
			pCurrEntry->pNext = pEntry;
		}

#ifdef SMART_ANTENNA
		pEntry->mcsInUse = -1;
#endif /* SMART_ANTENNA */
	}

	NdisReleaseSpinLock(&pAd->MacTabLock);
	/*update tx burst, must after unlock pAd->MacTabLock*/
	rtmp_tx_burst_set(pAd);
	
	return pEntry;
}

VOID RTMPRepeaterReconnectionCheck(
	IN PRTMP_ADAPTER pAd)
{
#ifdef APCLI_AUTO_CONNECT_SUPPORT
	INT i;
	PCHAR	pApCliSsid, pApCliCfgSsid;
	UCHAR	CfgSsidLen;
	NDIS_802_11_SSID Ssid;
	
	if (pAd->ApCfg.bMACRepeaterEn &&
		pAd->ApCfg.MACRepeaterOuiMode == 2 &&
		pAd->ApCfg.ApCliAutoConnectRunning == FALSE)
	{
		for (i = 0; i < MAX_APCLI_NUM; i++)
		{
			pApCliSsid = pAd->ApCfg.ApCliTab[i].Ssid;
			pApCliCfgSsid = pAd->ApCfg.ApCliTab[i].CfgSsid;
			CfgSsidLen = pAd->ApCfg.ApCliTab[i].CfgSsidLen;
			if ((pAd->ApCfg.ApCliTab[i].CtrlCurrState < APCLI_CTRL_AUTH ||
				!NdisEqualMemory(pApCliSsid, pApCliCfgSsid, CfgSsidLen)) &&
				pAd->ApCfg.ApCliTab[i].CfgSsidLen > 0 && 
				pAd->Mlme.OneSecPeriodicRound % 23 == 0)
			{
				DBGPRINT(RT_DEBUG_TRACE, (" %s(): Scan channels for AP (%s)\n", 
							__FUNCTION__, pApCliCfgSsid));
				pAd->ApCfg.ApCliAutoConnectRunning = TRUE;
				Ssid.SsidLength = CfgSsidLen;
				NdisCopyMemory(Ssid.Ssid, pApCliCfgSsid, CfgSsidLen);
				ApSiteSurvey(pAd, &Ssid, SCAN_ACTIVE, FALSE);
			}	
		}
	}
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
}

BOOLEAN RTMPRepeaterVaildMacEntry(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr)
{
	INVAILD_TRIGGER_MAC_ENTRY *pEntry = NULL;

	if (pAd->ApCfg.RepeaterCliSize >= MAX_EXT_MAC_ADDR_SIZE)
		return FALSE;

	if(IS_MULTICAST_MAC_ADDR(pAddr))
		return FALSE;

	if(IS_BROADCAST_MAC_ADDR(pAddr))
		return FALSE;

	pEntry = RepeaterInvaildMacLookup(pAd, pAddr);

	if (pEntry)
		return FALSE;
	else
		return TRUE;
}

INVAILD_TRIGGER_MAC_ENTRY *RepeaterInvaildMacLookup(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr)
{
	ULONG HashIdx;
	INVAILD_TRIGGER_MAC_ENTRY *pEntry = NULL;
	
	HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
	pEntry = pAd->ApCfg.ReptControl.ReptInvaildHash[HashIdx];

	while (pEntry)
	{
		if (MAC_ADDR_EQUAL(pEntry->MacAddr, pAddr))
		{
			break;
		}
		else
			pEntry = pEntry->pNext;
	}

	if (pEntry && pEntry->bInsert)
		return pEntry;
	else
		return NULL;
}

VOID RTMPRepeaterInsertInvaildMacEntry(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr)
{
	UCHAR HashIdx, idx = 0;
	INVAILD_TRIGGER_MAC_ENTRY *pEntry = NULL;
	INVAILD_TRIGGER_MAC_ENTRY *pCurrEntry = NULL;

	if (pAd->ApCfg.ReptControl.ReptInVaildMacSize >= 32)
		return;

	if (MAC_ADDR_EQUAL(pAddr, ZERO_MAC_ADDR))
		return;

	NdisAcquireSpinLock(&pAd->ApCfg.ReptCliEntryLock);
	for (idx = 0; idx< 32; idx++)
	{
		pEntry = &pAd->ApCfg.ReptControl.RepeaterInvaildEntry[idx];

		if (MAC_ADDR_EQUAL(pEntry->MacAddr, pAddr))
		{
			if (pEntry->bInsert)
			{
				NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);
				return;
			}
		}

		/* pick up the first available vacancy*/
		if (pEntry->bInsert == FALSE)
		{
			NdisZeroMemory(pEntry->MacAddr, MAC_ADDR_LEN);
			COPY_MAC_ADDR(pEntry->MacAddr, pAddr);
			pEntry->bInsert = TRUE;
			break;
		}
	}

	/* add this entry into HASH table */
	if (pEntry)
	{
		HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
		pEntry->pNext = NULL;
		if (pAd->ApCfg.ReptControl.ReptInvaildHash[HashIdx] == NULL)
		{
			pAd->ApCfg.ReptControl.ReptInvaildHash[HashIdx] = pEntry;
		}
		else
		{
			pCurrEntry = pAd->ApCfg.ReptControl.ReptInvaildHash[HashIdx];
			while (pCurrEntry->pNext != NULL)
				pCurrEntry = pCurrEntry->pNext;
			pCurrEntry->pNext = pEntry;
		}
	}

	DBGPRINT(RT_DEBUG_ERROR, (" Store Invaild MacAddr = %02x:%02x:%02x:%02x:%02x:%02x. !!!\n",
				PRINT_MAC(pEntry->MacAddr)));

	pAd->ApCfg.ReptControl.ReptInVaildMacSize++;
	NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);

	return;
}

BOOLEAN RTMPRepeaterRemoveInvaildMacEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR idx,
	IN PUCHAR pAddr)
{
	USHORT HashIdx;
	INVAILD_TRIGGER_MAC_ENTRY *pEntry = NULL;
	INVAILD_TRIGGER_MAC_ENTRY *pPrevEntry, *pProbeEntry;

	NdisAcquireSpinLock(&pAd->ApCfg.ReptCliEntryLock);

	HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
	pEntry = &pAd->ApCfg.ReptControl.RepeaterInvaildEntry[idx];

	if (pEntry && pEntry->bInsert)
	{
		pPrevEntry = NULL;
		pProbeEntry = pAd->ApCfg.ReptControl.ReptInvaildHash[HashIdx];
		ASSERT(pProbeEntry);
		if (pProbeEntry != NULL)
		{
			/* update Hash list*/
			do
			{
				if (pProbeEntry == pEntry)
				{
					if (pPrevEntry == NULL)
					{
						pAd->ApCfg.ReptControl.ReptInvaildHash[HashIdx] = pEntry->pNext;
					}
					else
					{
						pPrevEntry->pNext = pEntry->pNext;
					}
					break;
				}
		
				pPrevEntry = pProbeEntry;
				pProbeEntry = pProbeEntry->pNext;
			} while (pProbeEntry);
		}
		/* not found !!!*/
		ASSERT(pProbeEntry != NULL);

		pAd->ApCfg.ReptControl.ReptInVaildMacSize--;
	}

	NdisZeroMemory(pEntry->MacAddr, MAC_ADDR_LEN);
	pEntry->bInsert = FALSE;

	NdisReleaseSpinLock(&pAd->ApCfg.ReptCliEntryLock);

	return TRUE;
}

INT Show_Repeater_Cli_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	INT i;
    //UINT32 RegValue;
	ULONG DataRate=0;

	if (!pAd->ApCfg.bMACRepeaterEn)
		return TRUE;

	DBGPRINT(RT_DEBUG_OFF, ("\n"));

#ifdef DOT11_N_SUPPORT
	DBGPRINT(RT_DEBUG_OFF, ("HT Operating Mode : %d\n",
	pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode));
	DBGPRINT(RT_DEBUG_OFF, ("\n"));
#endif /* DOT11_N_SUPPORT */
	
	DBGPRINT(RT_DEBUG_OFF, ("\n%-19s%-4s%-4s%-4s%-4s%-8s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s%-7s%-7s\n",
		   "MAC", "AID", "BSS", "PSM", "WMM", "MIMOPS", "RSSI0", "RSSI1", 
		   "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC", "Idle", "Rate"));

	for (i = 0; i < MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (pEntry && IS_ENTRY_APCLI(pEntry)&& (pEntry->Sst == SST_ASSOC) && (pEntry->bReptCli))
		{
			DataRate=0;
			getRate(pEntry->HTPhyMode, &DataRate);

			DBGPRINT(RT_DEBUG_OFF, ("%02X:%02X:%02X:%02X:%02X:%02X  ",
					pEntry->ReptCliAddr[0], pEntry->ReptCliAddr[1], pEntry->ReptCliAddr[2],
					pEntry->ReptCliAddr[3], pEntry->ReptCliAddr[4], pEntry->ReptCliAddr[5]));

			DBGPRINT(RT_DEBUG_OFF, ("%-4d", (int)pEntry->Aid));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d-%d", (int)pEntry->apidx, pEntry->func_tb_idx));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d", (int)pEntry->PsMode));
			DBGPRINT(RT_DEBUG_OFF, ("%-4d",
					(int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE)));
#ifdef DOT11_N_SUPPORT
			DBGPRINT(RT_DEBUG_OFF, ("%-8d", (int)pEntry->MmpsMode));
#endif /* DOT11_N_SUPPORT */)
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pEntry->RssiSample.AvgRssi[0]));
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pEntry->RssiSample.AvgRssi[1]));
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", pEntry->RssiSample.AvgRssi[2]));
			DBGPRINT(RT_DEBUG_OFF, ("%-10s", get_phymode_str(pEntry->HTPhyMode.field.MODE)));
			DBGPRINT(RT_DEBUG_OFF, ("%-6s", get_bw_str(pEntry->HTPhyMode.field.BW)));
			DBGPRINT(RT_DEBUG_OFF, ("%-6d", pEntry->HTPhyMode.field.MCS));
			DBGPRINT(RT_DEBUG_OFF, ("%-6d", pEntry->HTPhyMode.field.ShortGI));
			DBGPRINT(RT_DEBUG_OFF, ("%-6d", pEntry->HTPhyMode.field.STBC));
			DBGPRINT(RT_DEBUG_OFF, ("%-7d",
					(int)(pEntry->StaIdleTimeout - pEntry->NoDataIdleCount)));
			DBGPRINT(RT_DEBUG_OFF, ("%-7d", (int)DataRate));
			DBGPRINT(RT_DEBUG_OFF, ("%-10d, %d, %d%%\n", pEntry->DebugFIFOCount,
					pEntry->DebugTxCount, (pEntry->DebugTxCount) ?
					((pEntry->DebugTxCount-pEntry->DebugFIFOCount)*100/pEntry->DebugTxCount) : 0));
			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	} 

	return TRUE;
}
#endif /* MAC_REPEATER_SUPPORT */

