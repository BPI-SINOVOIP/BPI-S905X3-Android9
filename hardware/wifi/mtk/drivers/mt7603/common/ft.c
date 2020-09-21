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
    ft.c
 
    Abstract:
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Fonchi Wu   12-19-2008    
 */
#ifdef DOT11R_FT_SUPPORT

#include "rt_config.h"
#include "ft.h"

#ifdef CONFIG_AP_SUPPORT

static VOID FT_RrbEnqueue(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pDA,
	IN PFT_ACTION pFtAction,
	IN UINT16 FtActLen,
	IN UINT32 ApIdx);

static VOID FT_ReqActionParse(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 Len,
	IN PUCHAR Ptr,
	OUT PFT_INFO pFtInfo);

VOID FT_ConstructGTKSubIe(
	IN	PRTMP_ADAPTER 		pAd,
	IN 	PMAC_TABLE_ENTRY 	pEntry,
	OUT	PFT_FTIE_INFO		pFtInfo);

UINT16	FT_AuthReqRsnValidation(
	IN	PRTMP_ADAPTER 		pAd,
	IN 	PMAC_TABLE_ENTRY 	pEntry,
	IN	PFT_CFG 			pFtCfg,
	IN 	PFT_INFO 			pFtInfo_in,
	OUT PFT_INFO 			pFtInfo_out);

UINT16	FT_AuthConfirmRsnValidation(
    IN 	PRTMP_ADAPTER 		pAd,
    IN 	PMAC_TABLE_ENTRY 	pEntry,
	IN 	PFT_INFO			pFtInfo_in);

UINT16	FT_AssocReqRsnValidation(
    IN 	PRTMP_ADAPTER 		pAd,
    IN 	PMAC_TABLE_ENTRY 	pEntry,
	IN 	PFT_INFO			pFtInfo_in,
	OUT PFT_INFO			pFtInfo_out);

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_EnqueueAuthReply(
    IN PRTMP_ADAPTER pAd, 
    IN PHEADER_802_11 pHdr, 
    IN USHORT Alg, 
    IN USHORT Seq, 
    IN USHORT StatusCode,
    IN PFT_MDIE_INFO pMdIeInfo,
	IN PFT_FTIE_INFO pFtIeInfo,
	IN PFT_RIC_INFO pRicInfo,
	IN PUCHAR pRsnIe,
	IN UCHAR RsnIeLen)
{
    HEADER_802_11     AuthHdr;
    ULONG             FrameLen = 0;
    PUCHAR            pOutBuffer = NULL;
    NDIS_STATUS       NStatus;
	PUINT8			ftie_ptr = NULL;
	UINT8			ftie_len = 0;
	PUINT8			mdie_ptr = NULL;
	UINT8			mdie_len = 0;

    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    if (NStatus != NDIS_STATUS_SUCCESS) 
        return;

	DBGPRINT(RT_DEBUG_TRACE, ("AUTH_RSP - Send FT-AUTH response (%d)...\n", StatusCode));
	MgtMacHeaderInit(pAd, &AuthHdr, SUBTYPE_AUTH, 0, pHdr->Addr2, 
						pHdr->Addr1,
						pHdr->Addr1);
	MakeOutgoingFrame(pOutBuffer,            &FrameLen, 
						sizeof(HEADER_802_11), &AuthHdr, 
						2,                     &Alg, 
						2,                     &Seq, 
						2,                     &StatusCode, 
						END_OF_ARGS);
	
	/* Insert MDIE. */
	if((pMdIeInfo != NULL) && (pMdIeInfo->Len > 0))
	{
		mdie_ptr = pOutBuffer + FrameLen;
		mdie_len = 5;
		FT_InsertMdIE(pAd, pOutBuffer + FrameLen, &FrameLen,
						pMdIeInfo->MdId, pMdIeInfo->FtCapPlc);
	}

	/* Insert FTIE. */
	if ((pFtIeInfo != NULL) && (pFtIeInfo->Len != 0))
	{
		ftie_ptr = pOutBuffer+FrameLen;
		ftie_len = (2 + pFtIeInfo->Len);
		FT_InsertFTIE(pAd, pOutBuffer+FrameLen, &FrameLen,
			pFtIeInfo->Len, pFtIeInfo->MICCtr,
			pFtIeInfo->MIC, pFtIeInfo->ANonce,
			pFtIeInfo->SNonce);
		
		/* Insert R1KH IE into FTIE. */
		if (pFtIeInfo->R1khIdLen!= 0)
			FT_FTIE_InsertKhIdSubIE(pAd, pOutBuffer+FrameLen, &FrameLen,
					FT_R1KH_ID, pFtIeInfo->R1khId,
					pFtIeInfo->R1khIdLen);
		
		/* Insert R0KH IE into FTIE. */
		if (pFtIeInfo->R0khIdLen!= 0)
			FT_FTIE_InsertKhIdSubIE(pAd, pOutBuffer+FrameLen, &FrameLen,
					FT_R0KH_ID, pFtIeInfo->R0khId,
					pFtIeInfo->R0khIdLen);
						
	}

	/* Insert RSNIE. */
	if ((RsnIeLen != 0) && (pRsnIe != NULL))
	{
        ULONG TmpLen;
        MakeOutgoingFrame(pOutBuffer+FrameLen,      &TmpLen, 
						  RsnIeLen,					pRsnIe,
                          END_OF_ARGS);
        FrameLen += TmpLen;
	}

	/* Insert RIC. */
	if ((pRicInfo != NULL) && (pRicInfo->Len != 0))
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pOutBuffer+FrameLen,	&TmpLen, 
				pRicInfo->Len,		pRicInfo->pRicInfo,
				END_OF_ARGS);
		FrameLen += TmpLen;
	}

	/* Calculate MIC in authentication-ACK frame */	
	if (pFtIeInfo->MICCtr.field.IECnt)
	{
		PMAC_TABLE_ENTRY pEntry;
		
		if ((pEntry = MacTableLookup(pAd, pHdr->Addr2)) != NULL)
		{
			UINT8	ft_mic[16];
			PFT_FTIE	pFtIe;
				
			FT_CalculateMIC(pHdr->Addr2, 
							pHdr->Addr1, 
							pEntry->PTK, 
							4, 
							pRsnIe, 
							RsnIeLen, 
							mdie_ptr, 
							mdie_len, 
							ftie_ptr, 
							ftie_len,
							pRicInfo->pRicInfo,
							pRicInfo->Len,
							ft_mic);

			/* Update the MIC field of FTIE */
			pFtIe = (PFT_FTIE)(ftie_ptr + 2);
			NdisMoveMemory(pFtIe->MIC, ft_mic, FT_MIC_LEN);
		}
	}

	MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, pOutBuffer);
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
static VOID FT_ReqActionParse(
	IN PRTMP_ADAPTER pAd,
	IN UINT16 Len,
	IN PUCHAR Ptr,
	OUT PFT_INFO pFtInfo)
{
	PEID_STRUCT eid_ptr;
	UCHAR WPA1_OUI[4]={0x00,0x50,0xF2,0x01};
	UCHAR WPA2_OUI[3]={0x00,0x0F,0xAC};

	eid_ptr = (PEID_STRUCT) Ptr;

	NdisZeroMemory(pFtInfo, sizeof(FT_INFO));
	
    /* get variable fields from payload and advance the pointer */
	while(((UCHAR*)eid_ptr + eid_ptr->Len + 1) < ((PUCHAR)Ptr + Len))
	{
		switch(eid_ptr->Eid)
		{
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
					pFtInfo->RicInfo.Len = ((UCHAR*)Ptr + Len)
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

			case IE_RSN:
			case IE_WPA:
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

				if (NdisEqualMemory(eid_ptr->Octet, WPA1_OUI, sizeof(WPA1_OUI))
					|| NdisEqualMemory(&eid_ptr->Octet[2], WPA2_OUI, sizeof(WPA2_OUI)))
				{
					NdisMoveMemory(pFtInfo->RSN_IE, eid_ptr, eid_ptr->Len + 2);
					pFtInfo->RSNIE_Len = eid_ptr->Len + 2;
				}
				break;

			default:
				break;
		}
        eid_ptr = (PEID_STRUCT)((UCHAR*)eid_ptr + 2 + eid_ptr->Len);        
	}
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_MakeFtActFrame(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 ActType,
	IN PUCHAR pStaMac,
	IN PUCHAR pTargetApMac,
	IN UINT16 StatusCode,
	IN PFT_INFO pFtInfo)
{
	ULONG TmpLen = 0;
	UCHAR Category = FT_CATEGORY_BSS_TRANSITION;

	if (pFrameBuf == NULL)
		return;
		
	/* Build basic frame first */
	MakeOutgoingFrame((pFrameBuf + *pFrameLen),	&TmpLen,
					1,							&Category,
					1,							&ActType,
					6,							pStaMac,
					6,							pTargetApMac,
					END_OF_ARGS);

	*pFrameLen += TmpLen;

	if ((ActType == FT_ACTION_BT_RSP)
		|| (ActType == FT_ACTION_BT_ACK))
	{
		UINT16 StatusCodeBuf;

			TmpLen = 0;
			StatusCodeBuf = cpu2le16(StatusCode);
		MakeOutgoingFrame((pFrameBuf + *pFrameLen),	&TmpLen,
							2,						&StatusCodeBuf,
							END_OF_ARGS);
		*pFrameLen += TmpLen;
	}

	/* Insert MD IE into packet. */
	if (pFtInfo->MdIeInfo.Len != 0)
		FT_InsertMdIE(pAd, (pFrameBuf + *pFrameLen), pFrameLen,
			pFtInfo->MdIeInfo.MdId, pFtInfo->MdIeInfo.FtCapPlc);

	/* Insert FT IE into packet. */
	if (pFtInfo->FtIeInfo.Len != 0)
		FT_InsertFTIE(pAd, (pFrameBuf + *pFrameLen), pFrameLen,
			pFtInfo->FtIeInfo.Len, pFtInfo->FtIeInfo.MICCtr,
			pFtInfo->FtIeInfo.MIC, pFtInfo->FtIeInfo.ANonce,
			pFtInfo->FtIeInfo.SNonce);

	if (pFtInfo->FtIeInfo.R0khIdLen!= 0)
		FT_FTIE_InsertKhIdSubIE(pAd, (pFrameBuf + *pFrameLen),
			pFrameLen, FT_R0KH_ID, pFtInfo->FtIeInfo.R0khId,
			pFtInfo->FtIeInfo.R0khIdLen);

	if (pFtInfo->FtIeInfo.R1khIdLen!= 0)
		FT_FTIE_InsertKhIdSubIE(pAd, (pFrameBuf + *pFrameLen),
			pFrameLen, FT_R1KH_ID, pFtInfo->FtIeInfo.R1khId,
			pFtInfo->FtIeInfo.R1khIdLen);

	if (pFtInfo->FtIeInfo.GtkLen!= 0)
 		FT_FTIE_InsertGTKSubIE(pAd, (pFrameBuf + *pFrameLen),
 			pFrameLen, pFtInfo->FtIeInfo.GtkSubIE,
 			pFtInfo->FtIeInfo.GtkLen);


	/* Insert Ric IE into packet .*/
	if ((ActType == FT_ACTION_BT_CONFIRM)
		|| (ActType == FT_ACTION_BT_ACK))
	{
		TmpLen = 0;
		MakeOutgoingFrame((pFrameBuf + *pFrameLen),	&TmpLen,
				pFtInfo->RicInfo.Len, (PUCHAR)pFtInfo->RicInfo.pRicInfo,
					END_OF_ARGS);
		*pFrameLen += TmpLen;
	}

	return;
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
 VOID FT_CfgInitial(
	IN PRTMP_ADAPTER pAd)
{
	INT apidx;
	PFT_CFG pFtCfg;
	RTMP_STRING R0khIdBuf[50];

	NdisZeroMemory(R0khIdBuf, 50);
	for (apidx = 0; apidx < MAX_MBSSID_NUM(pAd); apidx++)
	{
		pFtCfg = &pAd->ApCfg.MBSSID[apidx].FtCfg;
		pFtCfg->FtCapFlag.Dot11rFtEnable = TRUE;
		pFtCfg->FtCapFlag.FtOverDs = TRUE;
		pFtCfg->FtCapFlag.RsrReqCap = TRUE;

		FT_SET_MDID(pFtCfg->FtMdId, FT_DEFAULT_MDID);

		snprintf(R0khIdBuf, sizeof(R0khIdBuf), "Ralink:%02x:%02x:%02x:%02x:%02x:%02x",
			RandomByte(pAd), RandomByte(pAd), RandomByte(pAd),
			RandomByte(pAd), RandomByte(pAd), RandomByte(pAd));
		NdisZeroMemory(pFtCfg->FtR0khId, sizeof(pFtCfg->FtR0khId));
		NdisMoveMemory(pFtCfg->FtR0khId, R0khIdBuf, strlen(R0khIdBuf));
		pFtCfg->FtR0khIdLen = strlen(R0khIdBuf);
	}
}

VOID FT_Init(
	IN PRTMP_ADAPTER pAd)
{
	FT_KDP_Init(pAd);
	FT_RIC_Init(pAd);
	FT_R1khEntryTabInit(pAd);
}

VOID FT_Release(
	IN PRTMP_ADAPTER pAd)
{
	FT_KDP_Release(pAd);
	FT_RIC_Release(pAd);
	FT_R1khEntryTabDestroy(pAd);
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
USHORT FT_AuthReqHandler(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN PFT_INFO pFtInfo,
	OUT PFT_INFO pFtInfoBuf)
{
	USHORT result = MLME_SUCCESS;
	UCHAR ApIdx = pEntry->func_tb_idx;
	PFT_CFG pFtCfg;
	FT_CAP_AND_POLICY FtCapPlc;

	DBGPRINT(RT_DEBUG_TRACE, ("---> %s \n", __FUNCTION__));

	if (ApIdx >= pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: invalid apidx (%d)\n", \
			__FUNCTION__, ApIdx));
		return MLME_UNSPECIFY_FAIL;
	}

	pFtCfg = &pAd->ApCfg.MBSSID[ApIdx].FtCfg;
	NdisZeroMemory(pFtInfoBuf, sizeof(FT_INFO));

	do 
	{
		if ((pFtInfo->MdIeInfo.Len == 0)
			|| (!FT_MDID_EQU(pFtInfo->MdIeInfo.MdId, pFtCfg->FtMdId)))
		{
			DBGPRINT_ERR(("%s : The MDIE is invalid\n", __FUNCTION__));
			result = FT_STATUS_CODE_INVALID_MDIE;
			break;
		}

		/* FT auth-req in an RSN */
		if (pFtInfo->RSNIE_Len != 0)
		{
			/* Sanity check */
			result = FT_AuthReqRsnValidation(pAd, 
											   pEntry, 
											   pFtCfg, 
											   pFtInfo, 
											   pFtInfoBuf);
			if (result != MLME_SUCCESS)
				break;
		}
		else
		{
			/*	FT auth-req with no RSN Ie (OPEN mode).
				reply auth-rsp with success. */
			;
		}
		NdisMoveMemory(&pEntry->MdIeInfo, &pFtInfo->MdIeInfo,
				pFtInfo->MdIeInfo.Len);

		/* prepare Ft IEs for association response. */
		FT_SET_MDID(pFtInfoBuf->MdIeInfo.MdId, pFtCfg->FtMdId);
		FtCapPlc.field.FtOverDs = pFtCfg->FtCapFlag.FtOverDs;
		FtCapPlc.field.RsrReqCap = pFtCfg->FtCapFlag.RsrReqCap;
		pFtInfoBuf->MdIeInfo.FtCapPlc.word =
			pFtInfo->MdIeInfo.FtCapPlc.word & FtCapPlc.word;
		pFtInfoBuf->MdIeInfo.Len = 3;

		result = MLME_SUCCESS;
		break;
	} while(0);

	DBGPRINT(RT_DEBUG_TRACE, ("<--- %s done. StatusCode(%d)\n", __FUNCTION__, result));

	return result;
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
USHORT FT_AuthConfirmHandler(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN PFT_INFO pFtInfo,
	OUT PFT_INFO pFtInfoBuf)
{
	USHORT result = MLME_SUCCESS;
	UCHAR ApIdx = pEntry->func_tb_idx;
	PFT_CFG pFtCfg;
	FT_CAP_AND_POLICY FtCapPlc;

	DBGPRINT(RT_DEBUG_TRACE, ("%s:\n", __FUNCTION__));

	if (ApIdx >= pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: invalid apidx (%d)\n", __FUNCTION__, ApIdx));
		return MLME_UNSPECIFY_FAIL;
	}

	pFtCfg = &pAd->ApCfg.MBSSID[ApIdx].FtCfg;
	NdisZeroMemory(pFtInfoBuf, sizeof(FT_INFO));

	do 
	{
		if ((pFtInfo->MdIeInfo.Len == 0)
			|| (!FT_MDID_EQU(pFtInfo->MdIeInfo.MdId, pFtCfg->FtMdId)))
		{
			/* invalid MDID. reject it. */
			result = FT_STATUS_CODE_INVALID_MDIE;
			break;
		}

		if (pFtInfo->RSNIE_Len != 0)
		{
			UINT16 	result;
			UINT8 	rsnie_len = 0;
    		PUINT8  rsnie_ptr = NULL;
			UINT8	ft_len = 0;
						
			DBGPRINT(RT_DEBUG_TRACE, ("%s: Fast BSS transition in a RSN \n", __FUNCTION__));
			result = FT_AuthConfirmRsnValidation(pAd, 
												pEntry, 
												pFtInfo);
			if (result != MLME_SUCCESS)
			break;

			/* Prepare RSNIE for outgoing frame */
			if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK) ||
				(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode == Ndis802_11AuthModeWPA1WPA2)) 
		    {
		        rsnie_len = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSNIE_Len[1];
		        rsnie_ptr = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSN_IE[1][0];
			}
		    else
		    {
		        rsnie_len = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSNIE_Len[0];
		        rsnie_ptr = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSN_IE[0][0];
		    }

			pFtInfoBuf->RSNIE_Len = 0;
			RTMPInsertRSNIE(pFtInfoBuf->RSN_IE, 
							(PULONG)&pFtInfoBuf->RSNIE_Len,
							rsnie_ptr, 
							rsnie_len, 
							pEntry->FT_PMK_R1_NAME, 
							LEN_PMK_NAME);

			ft_len = sizeof(FT_FTIE);
	
			/* Prepare MIC-control and MIC field of FTIE for outgoing frame. */										
			pFtInfoBuf->FtIeInfo.MICCtr.field.IECnt = 3;
			NdisZeroMemory(pFtInfoBuf->FtIeInfo.MIC, 16);

			/* Prepare ANonce and Snonce field of FTIE for outgoing frame */
			NdisMoveMemory(pFtInfoBuf->FtIeInfo.ANonce, pEntry->ANonce, LEN_NONCE);
			NdisMoveMemory(pFtInfoBuf->FtIeInfo.SNonce, pEntry->SNonce, LEN_NONCE);

			/* Prepare in the R0KHID and its length */
			pFtInfoBuf->FtIeInfo.R0khIdLen = pFtCfg->FtR0khIdLen;
			NdisMoveMemory(pFtInfoBuf->FtIeInfo.R0khId, 
							pFtCfg->FtR0khId, pFtCfg->FtR0khIdLen);
			ft_len += (2 + pFtInfoBuf->FtIeInfo.R0khIdLen);

			/* Prepare in the R1KHID and its length */
			pFtInfoBuf->FtIeInfo.R1khIdLen = MAC_ADDR_LEN;
			NdisMoveMemory(pFtInfoBuf->FtIeInfo.R1khId, 
					   pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, MAC_ADDR_LEN);
			ft_len += (2 + MAC_ADDR_LEN);

			/* Update the length of FTIE */
			pFtInfoBuf->FtIeInfo.Len = ft_len;
			
		}

		/*
			FT auth-req with no RSN Ie (OPEN mode).
			reply auth-rsp with success.
		*/
		FT_RIC_ResourceRequestHandle(pAd, pEntry,
				(PUCHAR)pFtInfo->RicInfo.pRicInfo,
				pFtInfo->RicInfo.Len,
				(PUCHAR)pFtInfoBuf->RicInfo.pRicInfo,
				(PUINT32)&pFtInfoBuf->RicInfo.Len);

		/* 	In an RSN, The IE count need to include RIC for 
			MIC calculation */
		if (pFtInfoBuf->FtIeInfo.MICCtr.field.IECnt > 0 &&
			pFtInfoBuf->RicInfo.Len > 0)
			pFtInfoBuf->FtIeInfo.MICCtr.field.IECnt += 1;

		/* prepare Ft IEs for association response. */
		FT_SET_MDID(pFtInfoBuf->MdIeInfo.MdId, pFtCfg->FtMdId);
		FtCapPlc.field.FtOverDs = pFtCfg->FtCapFlag.FtOverDs;
		FtCapPlc.field.RsrReqCap = pFtCfg->FtCapFlag.RsrReqCap;
		pFtInfoBuf->MdIeInfo.FtCapPlc.word =
			pFtInfo->MdIeInfo.FtCapPlc.word & FtCapPlc.word;
		pFtInfoBuf->MdIeInfo.Len = 3;

		result = MLME_SUCCESS;

		break;
	} while(0);

	return result;
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
USHORT FT_AssocReqHandler(
    IN PRTMP_ADAPTER pAd, 
	IN BOOLEAN isReassoc,
	IN PFT_CFG pFtCfg,
    IN PMAC_TABLE_ENTRY pEntry,
	IN PFT_INFO			pPeer_FtInfo,
	OUT PFT_INFO	pFtInfoBuf)
{
	USHORT statusCode = MLME_SUCCESS;
	FT_CAP_AND_POLICY FtCapPlc;

	DBGPRINT(RT_DEBUG_TRACE, ("%s:\n", __FUNCTION__));

	NdisZeroMemory(pFtInfoBuf, sizeof(FT_INFO));

	if ((pFtCfg->FtCapFlag.Dot11rFtEnable)
		&& (pPeer_FtInfo != NULL) && (pPeer_FtInfo->MdIeInfo.Len != 0))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: it's FT client \n", __FUNCTION__));

		/* If the contents of the MDIE received by the AP do not match the
		** contents advertised in the Beacon and Probe Response frames, the
		** AP shall reject the (Re)association Request with status code 54
		** ("Invalid MDIE"). */
		if (!FT_MDID_EQU(pPeer_FtInfo->MdIeInfo.MdId, pFtCfg->FtMdId))
			statusCode = FT_STATUS_CODE_INVALID_MDIE;
		else
		{
			UINT8	ft_len = 0;
			
			ft_len = sizeof(FT_FTIE);

			/* Indicate this is a FT Initial Mobility Domain Association procedure */	
			if (!IS_FT_STA(pEntry))
			{
				NdisMoveMemory(&pEntry->MdIeInfo, &pPeer_FtInfo->MdIeInfo,
						pPeer_FtInfo->MdIeInfo.Len);
			}
					
			if (pPeer_FtInfo->RSNIE_Len != 0)
			{

				/* This is Fast BSS transition procedure with RSN */ 
				if (pPeer_FtInfo->FtIeInfo.Len > 0)
				{
					UINT16 result;

					DBGPRINT(RT_DEBUG_TRACE, ("%s: Fast BSS transition in a RSN \n", __FUNCTION__));
					result = FT_AssocReqRsnValidation(pAd, 
														pEntry, 
														pPeer_FtInfo, 
														pFtInfoBuf);
					if (result != MLME_SUCCESS)
						return result;					

					/* Update the length of GTK in FTIE*/
					if (pFtInfoBuf->FtIeInfo.GtkLen)
						ft_len += (2 + pFtInfoBuf->FtIeInfo.GtkLen);
				}
			
				/* Prepare in the R0KHID and its length */
				if (isReassoc)
				{
					pFtInfoBuf->FtIeInfo.R0khIdLen = pPeer_FtInfo->FtIeInfo.R0khIdLen;
					NdisMoveMemory(pFtInfoBuf->FtIeInfo.R0khId, 
									pPeer_FtInfo->FtIeInfo.R0khId, pFtInfoBuf->FtIeInfo.R0khIdLen);
				}
				else
				{
					pFtInfoBuf->FtIeInfo.R0khIdLen = pFtCfg->FtR0khIdLen;
					NdisMoveMemory(pFtInfoBuf->FtIeInfo.R0khId, 
									pFtCfg->FtR0khId, pFtCfg->FtR0khIdLen);
				}
				ft_len += (2 + pFtInfoBuf->FtIeInfo.R0khIdLen);

				/* Prepare in the R1KHID and its length */
				pFtInfoBuf->FtIeInfo.R1khIdLen = MAC_ADDR_LEN;
				NdisMoveMemory(pFtInfoBuf->FtIeInfo.R1khId, 
						   pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, MAC_ADDR_LEN);
				ft_len += (2 + MAC_ADDR_LEN);

				/* Update the length of FTIE */
				pFtInfoBuf->FtIeInfo.Len = ft_len;
			}

			/* prepare MDIE for association response. */
			FT_SET_MDID(pFtInfoBuf->MdIeInfo.MdId, pFtCfg->FtMdId);
			FtCapPlc.field.FtOverDs = pFtCfg->FtCapFlag.FtOverDs;
			FtCapPlc.field.RsrReqCap = pFtCfg->FtCapFlag.RsrReqCap;
			pFtInfoBuf->MdIeInfo.FtCapPlc.word =
				pPeer_FtInfo->MdIeInfo.FtCapPlc.word & FtCapPlc.word;
			pFtInfoBuf->MdIeInfo.Len = 3;
			
		}
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: it isn't FT client \n", __FUNCTION__));
	}

	return statusCode;
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_R1khEntryTabInit(
	IN PRTMP_ADAPTER pAd)
{
	INT idx;

	/* init spin lock */
	NdisAllocateSpinLock(pAd, &(pAd->ApCfg.FtTab.FT_R1khEntryTabLock));

	pAd->ApCfg.FtTab.FT_R1khEntryTabSize = 0;
	for (idx = 0; idx < FT_R1KH_ENTRY_HASH_TABLE_SIZE; idx++)
	{
		/* init event list */
		initList(&(pAd->ApCfg.FtTab.FT_R1khEntryTab[idx]));
	}
	pAd->ApCfg.FtTab.FT_R1khEntryTabReady = TRUE;
} /* End of FT_R1khEntryTabInit */

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
INT FT_R1khEntryInsert(
	IN PRTMP_ADAPTER pAd,
	IN PUINT8 pPmkR0Name,
	IN PUINT8 pPmkR1Name,
	IN PUINT8 pPmkR1Key,
	IN PUINT8 pPairwisChipher,
	IN PUINT8 pAkmSuite,
	IN UINT32 KeyLifeTime,
	IN UINT32 RassocDeadline,
	IN PUINT8 pR0khId,
	IN UINT8 R0khIdLen,
	IN PUINT8 pStaMac)
{
	UINT8 HashId;
	PFT_R1HK_ENTRY pEntry;

	if (pAd->ApCfg.FtTab.FT_R1khEntryTabSize >= FT_R1KH_ENTRY_TABLE_SIZE)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: FT_R1khEntryTab full.\n", __FUNCTION__));
		return -1;
	}

	if(os_alloc_mem(pAd, (PUCHAR *)&pEntry, sizeof(FT_R1HK_ENTRY)) ==
						NDIS_STATUS_FAILURE)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Unable to alloc memory.\n", __FUNCTION__));
		return -1;
	}

	pEntry->pNext = NULL;
	if (pStaMac != NULL)
		NdisMoveMemory(pEntry->StaMac, pStaMac, MAC_ADDR_LEN);
	if (pR0khId != NULL && R0khIdLen > 0)
	{
		pEntry->R0khIdLen = R0khIdLen;
		NdisMoveMemory(pEntry->R0khId, pR0khId, R0khIdLen);
	}
	if (pPairwisChipher != NULL)
		NdisMoveMemory(pEntry->PairwisChipher, pPairwisChipher, 4);
	if (pAkmSuite != NULL)
		NdisMoveMemory(pEntry->AkmSuite, pAkmSuite, 4);
	if (pPmkR0Name != NULL)
		NdisMoveMemory(pEntry->PmkR0Name, pPmkR0Name, 16);
	if (pPmkR1Name != NULL)
		NdisMoveMemory(pEntry->PmkR1Name, pPmkR1Name, 16);
	if (pPmkR1Key != NULL)
		NdisMoveMemory(pEntry->PmkR1Key, pPmkR1Key, 32);
	pEntry->KeyLifeTime = KeyLifeTime;
	pEntry->RassocDeadline = RassocDeadline;
	HashId = FT_R1KH_HASH_INDEX(pEntry->PmkR1Name);

	RTMP_SEM_LOCK(&(pAd->ApCfg.FtTab.FT_R1khEntryTabLock));

	insertTailList(&pAd->ApCfg.FtTab.FT_R1khEntryTab[HashId],
					(RT_LIST_ENTRY *)pEntry);

	pAd->ApCfg.FtTab.FT_R1khEntryTabSize++;
	RTMP_SEM_UNLOCK(&(pAd->ApCfg.FtTab.FT_R1khEntryTabLock));

	return 0;
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_R1khEntryDelete(
	IN PRTMP_ADAPTER pAd,
	IN PFT_R1HK_ENTRY pEntry)
{
	UINT8 HashId;
	PFT_TAB pFtTab;

	pFtTab = &pAd->ApCfg.FtTab;
	HashId = FT_R1KH_HASH_INDEX(pEntry->PmkR1Name);

	RTMP_SEM_LOCK(&(pFtTab->FT_R1khEntryTabLock));

	delEntryList(&pFtTab->FT_R1khEntryTab[HashId],
					(RT_LIST_ENTRY *)pEntry);

	os_free_mem(pAd, pEntry);

	pFtTab->FT_R1khEntryTabSize--;
	RTMP_SEM_UNLOCK(&(pFtTab->FT_R1khEntryTabLock));
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_R1khEntryTabDestroy(
	IN PRTMP_ADAPTER pAd)
{
	INT idx;
	PFT_R1HK_ENTRY pEntry;
	PFT_TAB pFtTab;

	pFtTab = &pAd->ApCfg.FtTab;
	pFtTab->FT_R1khEntryTabReady = FALSE;

	RTMP_SEM_LOCK(&(pFtTab->FT_R1khEntryTabLock));

	for (idx = 0; idx < FT_R1KH_ENTRY_HASH_TABLE_SIZE; idx ++)
	{
		do
		{
			pEntry = (PFT_R1HK_ENTRY)removeHeadList(
						&(pFtTab->FT_R1khEntryTab[idx]));
			if (pEntry != NULL)
				os_free_mem(pAd, (PUCHAR)pEntry);
		} while (pEntry != NULL);
	}

	RTMP_SEM_UNLOCK(&(pFtTab->FT_R1khEntryTabLock));
	NdisFreeSpinLock(&(pFtTab->FT_R1khEntryTabLock));
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
PFT_R1HK_ENTRY FT_R1khEntryTabLookup(
	IN PRTMP_ADAPTER pAd,
	IN PUINT8 pPMKR1Name)
{
	UINT8 HashId;
	PFT_R1HK_ENTRY pEntry;
	
	HashId = FT_R1KH_HASH_INDEX(pPMKR1Name);
	RTMP_SEM_LOCK(&(pAd->ApCfg.FtTab.FT_R1khEntryTabLock));

	pEntry = (PFT_R1HK_ENTRY)pAd->ApCfg.FtTab.FT_R1khEntryTab[HashId].pHead;
	while(pEntry != NULL)
	{
		if (RTMPEqualMemory(pPMKR1Name, pEntry->PmkR1Name, FT_KDP_WPA_NAME_MAX_SIZE))
			break;
		pEntry = pEntry->pNext;
	}

	RTMP_SEM_UNLOCK(&(pAd->ApCfg.FtTab.FT_R1khEntryTabLock));

	return pEntry;
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_FtAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	PMAC_TABLE_ENTRY pEntry;
	PHEADER_802_11 pHdr;
	PFT_ACTION pFtAction;
	UINT16 FtActLen;
	BOOLEAN IsTerminate;
	FT_INFO FtInfo;
	FT_INFO FtInfoBuf;
	USHORT result;
	NDIS_STATUS NStatus;
	PUCHAR pFtActFrame = NULL;
	ULONG FtActFrameLen = 0;
	INT apidx;
	PFT_CFG pFtCfg;
	
	pHdr = (PHEADER_802_11)Elem->Msg;
	pFtAction = (PFT_ACTION)(&Elem->Msg[LENGTH_802_11]);
	FtActLen = (Elem->MsgLen - LENGTH_802_11);

	/* Find which MBSSID to be authenticate */
	apidx = get_apidx_by_addr(pAd, pHdr->Addr1);
	if (apidx >= pAd->ApCfg.BssidNum)
		return;

	pFtCfg = &pAd->ApCfg.MBSSID[apidx].FtCfg;
	
	/* decide self is terminate or not. */
	IsTerminate = (MAC_ADDR_EQUAL(pFtAction->TargetApAddr,
					pAd->ApCfg.MBSSID[apidx].Bssid)) ? TRUE : FALSE;

	switch(pFtAction->Action)
	{
		case FT_ACTION_BT_REQ:

			DBGPRINT(RT_DEBUG_TRACE,
				("Get FT_ACTION_BT_REQ IsTerminate=%d\n", IsTerminate));

			if (IsTerminate)
			{
				NStatus = MlmeAllocateMemory(pAd, &pFtActFrame);
				if (NStatus != NDIS_STATUS_SUCCESS)
				{
					DBGPRINT(RT_DEBUG_ERROR,("%s: allocate memory failed.\n",
												__FUNCTION__));
					return;
				}

				if (!pFtCfg->FtCapFlag.Dot11rFtEnable)
					return;

				pEntry = MacTableLookup(pAd, pHdr->Addr2);
				if (!pEntry)
					pEntry = MacTableInsertEntry(pAd, pHdr->Addr2,
									wdev, ENTRY_CLIENT, TRUE);

				/* Parse FT-Request action frame. */
				FT_ReqActionParse(pAd, (FtActLen - sizeof(PFT_ACTION)),
					pFtAction->Oct, &FtInfo);

				/* FT-Request frame Handler. */
				NdisZeroMemory(&FtInfoBuf, sizeof(FT_INFO));
				result = FT_AuthReqHandler(pAd, pEntry, &FtInfo, &FtInfoBuf);

				if (result == MLME_SUCCESS)
				{
					NdisMoveMemory(&pEntry->MdIeInfo, &FtInfo.MdIeInfo, sizeof(FT_MDIE_INFO));

					pEntry->AuthState = AS_AUTH_OPEN;
					pEntry->Sst = SST_AUTH;
				}

				/* Build Ft-Rsp action frame. */
				FtActFrameLen = 0;
				FT_MakeFtActFrame(pAd, pFtActFrame, &FtActFrameLen,
					FT_ACTION_BT_RSP, pEntry->Addr, pFtAction->TargetApAddr,
					result, &FtInfoBuf);

				/* send FT-Rsp action frame to corresponding STA. */
				FT_RrbEnqueue(pAd, pHdr->Addr3, (PFT_ACTION)pFtActFrame,
								FtActFrameLen, pEntry->func_tb_idx);

				MlmeFreeMemory(pAd, pFtActFrame);
			}
			else
			{
				FT_RrbEnqueue(pAd, pFtAction->TargetApAddr,
							(PFT_ACTION)pFtAction, FtActLen, apidx);
			}
				break;

		case FT_ACTION_BT_CONFIRM:

			DBGPRINT(RT_DEBUG_TRACE,
				("Get FT_ACTION_BT_CONFIRM IsTerminate=%d\n", IsTerminate));

			if (IsTerminate)
			{
				NDIS_STATUS NStatus = MlmeAllocateMemory(pAd, &pFtActFrame);

				if (NStatus != NDIS_STATUS_SUCCESS) 
				{
					DBGPRINT(RT_DEBUG_ERROR,("%s: allocate memory failed.\n",
							__FUNCTION__));
					return;
				}

				if (Elem->Wcid < MAX_LEN_OF_MAC_TABLE)
					pEntry = &pAd->MacTab.Content[Elem->Wcid];
				else
				{
					DBGPRINT(RT_DEBUG_OFF, ("%s: invalid STA.\n", __FUNCTION__));
					return;
				}

				if (pEntry->func_tb_idx > pAd->ApCfg.BssidNum)
					return;

				if (!pFtCfg->FtCapFlag.Dot11rFtEnable)
					return;

				/* Parse FT-Request action frame. */
				FT_ReqActionParse(pAd, (FtActLen - sizeof(PFT_ACTION)),
					pFtAction->Oct, &FtInfo);

				/* FT-Request frame Handler. */
				NdisZeroMemory(&FtInfoBuf, sizeof(FT_INFO));
				os_alloc_mem(pAd, (UCHAR **)&(FtInfoBuf.RicInfo.pRicInfo), 512);
				if (FtInfoBuf.RicInfo.pRicInfo != NULL)
				{
					result = FT_AuthConfirmHandler(pAd, pEntry, &FtInfo, &FtInfoBuf);

					/* Build Ft-Ack action frame. */
					FtActFrameLen = 0;
					FT_MakeFtActFrame(pAd, pFtActFrame, &FtActFrameLen,
						FT_ACTION_BT_ACK, pEntry->Addr, pHdr->Addr3, result,
						&FtInfoBuf);

					/* reply FT-Ack action frame to corresponding STA. */
					FT_RrbEnqueue(pAd, pHdr->Addr3, (PFT_ACTION)pFtActFrame, 
									FtActFrameLen, pEntry->func_tb_idx);
					os_free_mem(NULL, FtInfoBuf.RicInfo.pRicInfo);
				}
				else
				{
					return;
				}
			}
			else
				FT_RrbEnqueue(pAd, pFtAction->TargetApAddr,
					pFtAction, FtActLen, apidx);
			break;

		case FT_ACTION_BT_RSP:
		case FT_ACTION_BT_ACK:

			DBGPRINT(RT_DEBUG_TRACE,
				("Get FT_ACTION_BT_RSP or FT_ACTION_BT_ACK \
					IsTerminate=%d\n", IsTerminate));

			/* forward it to corrspondign STA. */
			NStatus = MlmeAllocateMemory(pAd, &pFtActFrame);
			if (NStatus != NDIS_STATUS_SUCCESS) 
			{
				DBGPRINT(RT_DEBUG_ERROR,("%s: allocate memory failed.\n",
											__FUNCTION__));
				return;
			}

			COPY_MAC_ADDR(pHdr->Addr1, pFtAction->StaMac);
			COPY_MAC_ADDR(pHdr->Addr2, pAd->ApCfg.MBSSID[apidx].Bssid);
			COPY_MAC_ADDR(pHdr->Addr3, pAd->ApCfg.MBSSID[apidx].Bssid);
			pHdr->FC.ToDs = 0;
			pHdr->FC.FrDs = 1;

			FtActFrameLen = 0;
			MakeOutgoingFrame(pFtActFrame,  &FtActFrameLen,
			      Elem->MsgLen,				pHdr,
			      END_OF_ARGS);

			MiniportMMRequest(pAd, 0, pFtActFrame, FtActFrameLen);
			MlmeFreeMemory(pAd, pFtActFrame);
			break;

		default:
			DBGPRINT(RT_DEBUG_ERROR, ("%s: unknow action type (%d).\n", \
				__FUNCTION__, pFtAction->Action));
			break;
	}

}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
static VOID FT_RrbEnqueue(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pDA,
	IN PFT_ACTION pFtAction,
	IN UINT16 FtActLen,
	IN UINT32 ApIdx)
{
#if 1
	FT_KDP_EVT_ACTION FtKdpEvtAction;

	COPY_MAC_ADDR(FtKdpEvtAction.MacDa, pDA);

	switch(pFtAction->Action)
	{
		case FT_ACTION_BT_REQ:
		case FT_ACTION_BT_CONFIRM:
			FtKdpEvtAction.RequestType = 0;
			COPY_MAC_ADDR(FtKdpEvtAction.MacSa, pFtAction->StaMac);
			COPY_MAC_ADDR(FtKdpEvtAction.MacAp,
							pAd->ApCfg.MBSSID[ApIdx].Bssid);
			break;

		case FT_ACTION_BT_RSP:
		case FT_ACTION_BT_ACK:
			FtKdpEvtAction.RequestType = 1;
			COPY_MAC_ADDR(FtKdpEvtAction.MacSa, pFtAction->TargetApAddr);
			COPY_MAC_ADDR(FtKdpEvtAction.MacAp,
							pAd->ApCfg.MBSSID[ApIdx].Bssid);
			break;
	}

	FT_KDP_EVENT_INFORM(pAd, ApIdx, FT_KDP_SIG_ACTION, pFtAction,
			FtActLen, &FtKdpEvtAction);

#else
	UCHAR FtRrb[] = {0x89, 0x0d};
	UCHAR Header802_3[14];
	NDIS_PACKET *pPktComm;
	PUCHAR pSA = ZERO_MAC_ADDR;

	/* allocate a rx packet */
#ifdef FT_OS_LINUX
/*	pPktComm = (NDIS_PACKET *)dev_alloc_skb(sizeof(RT_SIGNAL_STRUC)+LENGTH_802_3); */
	DEV_ALLOC_SKB(pAd, pPktComm, sizeof(RT_SIGNAL_STRUC)+LENGTH_802_3);
#endif
#ifdef FT_OS_VXWORKS
	pPktComm = RtmpOSNetPktAlloc(pAd, sizeof(RT_SIGNAL_STRUC)+LENGTH_802_3);
#endif

	if (pPktComm != NULL)
	{
		/*
			The Remote Frame Type for FT Remote request/response
			messages shall be set to 1.
		*/
		UINT8 RemoteFrameType = 1;
		/*
			The FT Packet Type field shall be set to 0 
			for Remote Request, and to 1 for Remote Response.
		*/
		UINT8 FtPacketType = 0;

		pSA = pFtAction->StaMac;
		if ((pFtAction->Action == FT_ACTION_BT_RSP)
			|| (pFtAction->Action == FT_ACTION_BT_ACK))
			FtPacketType = 1;

		MAKE_802_3_HEADER(Header802_3, pDA, pSA, FtRrb);
		memcpy(skb_put(RTPKT_TO_OSPKT(pPktComm), LENGTH_802_3),
						&Header802_3, LENGTH_802_3);
		memcpy(skb_put(RTPKT_TO_OSPKT(pPktComm), sizeof(UINT8)),
						&RemoteFrameType, sizeof(UINT8));
		memcpy(skb_put(RTPKT_TO_OSPKT(pPktComm), sizeof(UINT8)),
						&FtPacketType, sizeof(UINT8));
		memcpy(skb_put(RTPKT_TO_OSPKT(pPktComm), sizeof(UINT16)),
						&FtActLen, sizeof(UINT16));
		memcpy(skb_put(RTPKT_TO_OSPKT(pPktComm), MAC_ADDR_LEN),
						&pFtAction->TargetApAddr, MAC_ADDR_LEN);
		memcpy(skb_put(RTPKT_TO_OSPKT(pPktComm), FtActLen),
						pFtAction, FtActLen);

		/* convert 802.11 to 802.3 packet */
#ifdef FT_OS_LINUX
		GET_OS_PKT_NETDEV(pPktComm) = get_netdev_from_bssid(pAd, BSS0);
#endif
#ifdef FT_OS_VXWORKS
		SET_OS_PKT_NETDEV(pPktComm, get_netdev_from_bssid(pAd, BSS0));
#endif

		/* pass this packet to upper layer */
		announce_802_3_packet(pAd, pPktComm);
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,
			("%s: Allocate net buffer fail!\n", __FUNCTION__));
	} /* End of if */
#endif
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_RrbHandler(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pPktSrc,
	IN INT32 PktLen)
{
	PMAC_TABLE_ENTRY pEntry;
	NDIS_STATUS Status;
	PUCHAR pOutBuffer = NULL;
	ULONG FrameLen;
	HEADER_802_11 Hdr;
	PFT_RRB pRrb = (PFT_RRB)(pPktSrc + LENGTH_802_3);
	PUCHAR pDA;
	PUCHAR pSA;
	PFT_ACTION pFtAction;
	ULONG Wcid;


	Status = MlmeAllocateMemory(pAd, &pOutBuffer);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%s: allocate auth buffer fail!\n",
					__FUNCTION__));
		return;
	} /* End of if */

	pFtAction = (PFT_ACTION)pRrb->Oct;

	pDA = pPktSrc;
	pSA = pFtAction->StaMac;

	pEntry = MacTableLookup(pAd, pSA);
	if (pEntry)
		Wcid = pEntry->wcid;
	else
		Wcid = RESERVED_WCID;

	/* Make 802.11 header. */
	ActHeaderInit(pAd, &Hdr, pDA, pSA, pRrb->APAdr);

	/* Make ft action frame. */
	MakeOutgoingFrame(pOutBuffer,				&FrameLen,
					sizeof(HEADER_802_11),		&Hdr,
					pRrb->FTActLen,				(PUCHAR)pRrb->Oct,
					END_OF_ARGS);

	/* enqueue it into FT action state machine. */
	REPORT_MGMT_FRAME_TO_MLME(pAd, Wcid, pOutBuffer, FrameLen, 0, 0, 0, 0, 0);

	if (pOutBuffer)
		os_free_mem(pAd, pOutBuffer);
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_R1KHInfoMaintenance(
	IN PRTMP_ADAPTER pAd)
{
	INT HashIdx;
	PFT_R1HK_ENTRY pEntry;
	PMAC_TABLE_ENTRY pMacEntry;
	PFT_TAB pFtTab;

	pFtTab = &pAd->ApCfg.FtTab;
	if (pFtTab->FT_R1khEntryTabReady != TRUE)
		return;

	RTMP_SEM_LOCK(&pFtTab->FT_R1khEntryTabLock);
	for (HashIdx = 0; HashIdx < FT_R1KH_ENTRY_HASH_TABLE_SIZE; HashIdx++)
	{
		pEntry = (PFT_R1HK_ENTRY)\
						(pFtTab->FT_R1khEntryTab[HashIdx].pHead);

		while (pEntry != NULL)
		{
			if((pEntry->KeyLifeTime--) == 0)
			{
				PFT_R1HK_ENTRY pEntryTmp;
				MLME_DISASSOC_REQ_STRUCT DisassocReq;

				/*
					Kick out the station.
					and Info KDP daemon to delete the key.
				*/
				pMacEntry = MacTableLookup(pAd, pEntry->StaMac);

				if (pMacEntry)
				{
					DisassocParmFill(pAd, &DisassocReq, pEntry->StaMac,
						MLME_UNSPECIFY_FAIL);
					MlmeEnqueue(pAd, AP_ASSOC_STATE_MACHINE, APMT2_MLME_DISASSOC_REQ,
						sizeof(MLME_DISASSOC_REQ_STRUCT), (PVOID)&DisassocReq, 0);
				}

				/*
					Indicate IAPP daemon to delete R0KH-SA
					relative to the STA
				*/
				FT_KDP_EVENT_INFORM(pAd, 0, FT_KDP_SIG_KEY_TIMEOUT,
										pEntry->StaMac, 6, NULL);

				pEntryTmp = pEntry->pNext;
				delEntryList(&pFtTab->FT_R1khEntryTab[HashIdx],
								(RT_LIST_ENTRY *)pEntry);
				os_free_mem(pAd, pEntry);
				pFtTab->FT_R1khEntryTabSize--;
				pEntry = pEntryTmp;
			}
			else
				pEntry = pEntry->pNext;
		}
	}
	RTMP_SEM_UNLOCK(&pFtTab->FT_R1khEntryTabLock);
}

VOID FT_ConstructGTKSubIe(
	IN	PRTMP_ADAPTER 		pAd,
	IN 	PMAC_TABLE_ENTRY 	pEntry,
	OUT	PFT_FTIE_INFO		pFtInfo)
{
	UINT8	gtk_len = 0;
	UINT8	pad_len = 0;	
	UINT8	key_buf[32];
	UINT8	e_key_buf[40];	
	UINT8	key_len;
	UINT	e_key_len;
	UCHAR	apidx;
	UCHAR	key_idx;
	UCHAR	cipher_alg;
	PUINT8	gtk;
	ULONG	TmpLen = 0;
	UINT8	remainder;
	FT_GTK_KEY_INFO KeyInfo;
	UCHAR 			rsc[8];

	apidx = pEntry->func_tb_idx;	
	gtk = pAd->ApCfg.MBSSID[apidx].GTK;	
	key_idx = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].DefaultKeyId;
	cipher_alg = pAd->SharedKey[pEntry->func_tb_idx][key_idx].CipherAlg;

	DBGPRINT(RT_DEBUG_TRACE, ("%s : key idx(%d) \n", __FUNCTION__, key_idx));
	
	switch (cipher_alg)
	{
		case CIPHER_WEP64:
			gtk_len = 5;
			break;
		case CIPHER_WEP128:
			gtk_len = 13;
			break;	
		case CIPHER_TKIP:
			gtk_len = 32;
			break;
		case CIPHER_AES:
			gtk_len = 16;
			break;		
	}

	/*  The Key field shall be padded before encrypting if the key length 
		is less than 16 octets or if it is not a multiple of 8. */
	NdisMoveMemory(key_buf, gtk, gtk_len);
	key_len = gtk_len;
	if ((remainder = gtk_len & 0x07) != 0)
	{
		INT	i;
	
		pad_len = (8 - remainder);
		key_buf[gtk_len] = 0xDD;
		for (i = 1; i < pad_len; i++)
			key_buf[gtk_len + i] = 0;

		key_len += pad_len;
	}
	if (key_len < 16)
	{
		INT	i;
		
		pad_len = 16 - key_len;
		for (i = 0; i < pad_len; i++)
			key_buf[key_len + i] = 0;

		key_len += pad_len;	
	}
			
	NdisZeroMemory(&KeyInfo, sizeof(FT_GTK_KEY_INFO));
	KeyInfo.field.KeyId = key_idx;	
	KeyInfo.word = cpu2le16(KeyInfo.word);

	/* Get Group RSC form Asic */
	NdisZeroMemory(rsc, 8);
	RTMPGetTxTscFromAsic(pAd, apidx, rsc);

	e_key_len = key_len;
	AES_Key_Wrap(key_buf, key_len, 
				 &pEntry->PTK[LEN_PTK_KCK], LEN_PTK_KEK, 
				 e_key_buf, &e_key_len);

	/* Construct FT GTK-IE*/
	
    MakeOutgoingFrame(pFtInfo->GtkSubIE,            &TmpLen,
                      sizeof(FT_GTK_KEY_INFO),  	&KeyInfo,
                      1,  							&gtk_len,
                      8,							rsc,
                      e_key_len,					e_key_buf,
                      END_OF_ARGS);
								
	pFtInfo->GtkLen = TmpLen;

}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
BOOLEAN FT_QueryKeyInfoForKDP(
	IN 	PRTMP_ADAPTER 	pAd,
	IN	UINT32			ApIdx,
	OUT FT_KDP_EVT_KEY_ELM	*pEvtKeyReq)
{
	INT		CacheIdx;
	ULONG 	Now;
	UINT32	alive_tick;
	INT		remain_time = 0;
	PAP_BSSID_INFO pkeyInfo;
	PFT_R1HK_ENTRY pR1khEntry;	
	UCHAR OriPMKR1Name[FT_KDP_WPA_NAME_MAX_SIZE];

	/* Search PMK Cache */
	CacheIdx = RTMPSearchPMKIDCache(pAd, ApIdx, pEvtKeyReq->MacAddr);

	if (CacheIdx == -1)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s : The PMKR0 doesn't exist for %02x:%02x:%02x:%02x:%02x:%02x\n", 
							__FUNCTION__, PRINT_MAC(pEvtKeyReq->MacAddr)));
		return FALSE;
	}

	pkeyInfo = &pAd->ApCfg.MBSSID[ApIdx].PMKIDCache.BSSIDInfo[CacheIdx];

	/* Derive the PMK-R1 and PMK-R1-Name for this R1KH */
	FT_DerivePMKR1(pkeyInfo->PMK, 
				   pkeyInfo->PMKID, 
				   /*pEvtKeyReq->KeyInfo.R1KHID, */ /* peer R1KH-ID */
				   pAd->ApCfg.MBSSID[ApIdx].Bssid,
				   pEvtKeyReq->MacAddr, 
				   pEvtKeyReq->PMKR1, 
				   OriPMKR1Name);

	pR1khEntry = FT_R1khEntryTabLookup(pAd, OriPMKR1Name);

	if (pR1khEntry == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s : No initial association information 2 for %02x:%02x:%02x:%02x:%02x:%02x\n", 
							__FUNCTION__, PRINT_MAC(pEvtKeyReq->MacAddr)));
		return FALSE;
	}

	FT_DerivePMKR1(pkeyInfo->PMK, 
				   pkeyInfo->PMKID, 
				   pEvtKeyReq->KeyInfo.R1KHID, /* peer R1KH-ID */
				   pEvtKeyReq->MacAddr, 
				   pEvtKeyReq->PMKR1, 
				   pEvtKeyReq->PMKR1Name);

	/* calculate the remaine time of PMKR0-Key */
	NdisGetSystemUpTime(&Now);
	alive_tick = Now - pkeyInfo->RefreshTime;
	if (alive_tick < pAd->ApCfg.MBSSID[ApIdx].PMKCachePeriod)
		remain_time = (pAd->ApCfg.MBSSID[ApIdx].PMKCachePeriod - alive_tick)/OS_HZ;

	/* Assign KeyLifeTime and Reassociation Deadline */
	pEvtKeyReq->KeyLifeTime = remain_time;
	pEvtKeyReq->ReassocDeadline = FT_REASSOC_DEADLINE;

	/* Assign R0KH-ID */
	pEvtKeyReq->KeyInfo.R0KHIDLen = pAd->ApCfg.MBSSID[ApIdx].FtCfg.FtR0khIdLen;
	NdisMoveMemory(pEvtKeyReq->KeyInfo.R0KHID, 
				   pAd->ApCfg.MBSSID[ApIdx].FtCfg.FtR0khId, 
				   pAd->ApCfg.MBSSID[ApIdx].FtCfg.FtR0khIdLen);

	/* Assign PMK-R0-Name */
	NdisMoveMemory(pEvtKeyReq->KeyInfo.PMKR0Name, pkeyInfo->PMKID, LEN_PMK_NAME);

	/* Assign cipher and AKM */
	NdisMoveMemory(pEvtKeyReq->PairwisChipher, pR1khEntry->PairwisChipher, 4);
	NdisMoveMemory(pEvtKeyReq->AkmSuite, pR1khEntry->AkmSuite, 4);

	/* Assign R0KH MAC */
	NdisMoveMemory(pEvtKeyReq->R0KH_MAC, pEvtKeyReq->MacAddr, 6);
	
	return TRUE;

}

UINT16	FT_AuthReqRsnValidation(
	IN	PRTMP_ADAPTER 		pAd,
	IN 	PMAC_TABLE_ENTRY 	pEntry,
	IN	PFT_CFG 			pFtCfg,
	IN 	PFT_INFO 			pFtInfo_in,
	OUT PFT_INFO 			pFtInfo_out)
{
	UINT8 count = 0;
	PUINT8 pAkmSuite = NULL;
	PUINT8 pPmkR0Name = NULL;
	PUINT8 pCipher = NULL;
	PFT_R1HK_ENTRY pR1hkEntry = NULL;
	UINT8	ft_len = 0;
	UINT8	ptk_len;
	UINT8 	rsnie_len = 0;
    PUINT8  rsnie_ptr = NULL;
	UINT16	result = MLME_SUCCESS;
	
	/* Check the validity of the received RSNIE */
	if ((result = APValidateRSNIE(pAd, pEntry, pFtInfo_in->RSN_IE, pFtInfo_in->RSNIE_Len)) != MLME_SUCCESS)	
	{				
		return result;
	}

	/* Extract the PMK-R0-Name from the received RSNIE */
	pPmkR0Name = WPA_ExtractSuiteFromRSNIE(pFtInfo_in->RSN_IE, pFtInfo_in->RSNIE_Len, PMKID_LIST, &count);
	if (pPmkR0Name)
	{	
#ifdef FT_RSN_DEBUG
		hex_dump("FT PMK-R0-NAME", pPmkR0Name, count * LEN_PMK_NAME);
#endif /* FT_RSN_DEBUG */

		/*	The R1KH of the target AP uses the value of PMKR0Name and other 
			information in the frame to calculate PMKR1Name. */	
		FT_DerivePMKR1Name(pPmkR0Name, 
						   pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, 
						   pEntry->Addr, 
						   pEntry->FT_PMK_R1_NAME);

		hex_dump("pPmkR0Name=", pPmkR0Name, LEN_PMK_NAME);
		hex_dump("pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid=", pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, 6);
		hex_dump("pEntry->Addr=", pEntry->Addr, 6);
		hex_dump("pEntry->FT_PMK_R1_NAME=", pEntry->FT_PMK_R1_NAME, sizeof(pEntry->FT_PMK_R1_NAME));

		/* Look up the R1KH Table */
		pR1hkEntry = FT_R1khEntryTabLookup(pAd, pEntry->FT_PMK_R1_NAME);

		/* 	If the target AP does not have the key identified by PMKR1Name, 
	 		it may retrieve that key from the R0KH identified by the STA. */
		if ((pR1hkEntry == NULL) ||
			(RTMPEqualMemory(pEntry->FT_PMK_R1_NAME, pR1hkEntry->PmkR1Name, LEN_PMK_NAME) == FALSE))
		{
#if 0		
			FT_KDP_EVT_KEY_ELM EvtKeyReq;
				
			NdisZeroMemory(&EvtKeyReq, sizeof(FT_KDP_EVT_KEY_ELM));

			/* make up request content */
			EvtKeyReq.ElmId = FT_KDP_ELM_ID_PRI;
			EvtKeyReq.ElmLen = FT_KDP_ELM_PRI_LEN;
			EvtKeyReq.OUI[0] = FT_KDP_ELM_PRI_OUI_0;
			EvtKeyReq.OUI[1] = FT_KDP_ELM_PRI_OUI_1;
			EvtKeyReq.OUI[2] = FT_KDP_ELM_PRI_OUI_2;

			NdisMoveMemory(EvtKeyReq.MacAddr, pEntry->Addr, ETH_ALEN);
			NdisMoveMemory(EvtKeyReq.KeyInfo.S1KHID, pEntry->Addr, FT_KDP_S1KHID_MAX_SIZE);
			NdisMoveMemory(EvtKeyReq.KeyInfo.R1KHID,
							pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid,
							FT_KDP_R1KHID_MAX_SIZE);
			
			/* Issue an event to query PMKR1 information from R0KH */				
			FT_KDP_EVENT_INFORM(pAd, pEntry->func_tb_idx, FT_KDP_SIG_KEY_REQ_AUTO,
								&EvtKeyReq, sizeof(FT_KDP_EVT_KEY_ELM), NULL);
#endif

			/*	If the requested R0KH is not reachable, the AP shall respond 
				to the Authentication Request with status code 28 ("R0KH unreachable"). */
#if 1
			{											
				return FT_STATUS_CODE_R0KH_UNREACHABLE;
			}
#endif 					 
		}

		/*  If the RSNIE in the Authentication Request frame contains an invalid 
			PMKR0Name, and the AP has determined that it is an invalid PMKR0Name, 
			the AP shall reject the Authentication Request with status code 53 
			("Invalid PMKID").  */
		if ((pR1hkEntry == NULL) ||
			(RTMPEqualMemory(pPmkR0Name, pR1hkEntry->PmkR0Name, LEN_PMK_NAME) == FALSE))
		{
			DBGPRINT_ERR(("%s : The PMKID is invalid\n", __FUNCTION__));
			hex_dump("Peer PMKR0Name", pPmkR0Name, LEN_PMK_NAME);
			hex_dump("Own PMKR0Name", pR1hkEntry->PmkR0Name, LEN_PMK_NAME);
			return FT_STATUS_CODE_INVALID_PMKID;
		}																
	}
	else
	{
		/* reject the Authentication Request with status code 53 ("Invalid PMKID") */
		DBGPRINT_ERR(("%s : The peer PMKID is emtpy\n", __FUNCTION__));				
		return FT_STATUS_CODE_INVALID_PMKID;
	}

	if (pR1hkEntry == NULL)
	{
		DBGPRINT_ERR(("%s : The R1KH table doesn't exist\n", __FUNCTION__));				
		return FT_STATUS_CODE_R0KH_UNREACHABLE;
	}

	/* Update the Reassocation Deadline */
	pEntry->AssocDeadLine = pR1hkEntry->RassocDeadline;
		
	/* Extract the AKM suite from the received RSNIE */
	pAkmSuite = WPA_ExtractSuiteFromRSNIE(pFtInfo_in->RSN_IE, pFtInfo_in->RSNIE_Len, AKM_SUITE, &count);
	if ((pAkmSuite == NULL) ||
		(RTMPEqualMemory(pAkmSuite, pR1hkEntry->AkmSuite, 4) == FALSE))
	{
		/*	It doesn't a negotiated AKM of Fast BSS Transition, the AP 
			shall reject the Authentication Request with status code 43 ("Invalid AKMP"). */
		DBGPRINT_ERR(("%s : The AKM is invalid\n", __FUNCTION__));
		return MLME_INVALID_AKMP;
	}

	/* Extract the pairwise cipher suite from the received RSNIE */
	pCipher = WPA_ExtractSuiteFromRSNIE(pFtInfo_in->RSN_IE, pFtInfo_in->RSNIE_Len, PAIRWISE_SUITE, &count);
	if ((pCipher == NULL) ||
		(RTMPEqualMemory(pCipher, pR1hkEntry->PairwisChipher, 4) == FALSE))
	{
		/* 	If the non-AP STA selects a pairwise cipher suite in the RSNIE
		 	that is different than the ones used in the Initial Mobility 
		 	Domain association, then the AP shall reject the Authentication 
		 	Request with status code 19 ("Invalid Pair-wise Cipher"). */
		DBGPRINT_ERR(("%s : The pairwise-cipher is invalid\n", __FUNCTION__));				
		return MLME_INVALID_PAIRWISE_CIPHER;
	}

	/* Check the validity of R0KHID */			
	if ((pFtInfo_in->FtIeInfo.R0khIdLen != pR1hkEntry->R0khIdLen) ||
		(RTMPEqualMemory(pFtInfo_in->FtIeInfo.R0khId, 
						 pR1hkEntry->R0khId, 
						 pR1hkEntry->R0khIdLen) == FALSE))
	{								
		/*	If the FTIE in the FT Request frame contains an invalid R0KH-ID, 
		 	the AP shall reject the FT Request with status code 55 ("Invalid FTIE"). */
		DBGPRINT_ERR(("%s : The FTIE is invalid\n", __FUNCTION__));				
		return FT_STATUS_CODE_INVALID_FTIE;
	}
		
	/* Get PMK-R1 from R1KH Table */
	NdisMoveMemory(pEntry->FT_PMK_R1, pR1hkEntry->PmkR1Key, LEN_PMK);

	/* Get SNonce from Auth-req */
	NdisMoveMemory(pEntry->SNonce, pFtInfo_in->FtIeInfo.SNonce, LEN_NONCE);

	/* Generate ANonce randomly */
	GenRandom(pAd, pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, pEntry->ANonce);

	hex_dump("anonce", pEntry->ANonce, 32);
	hex_dump("snonce", pEntry->SNonce, 32);

	if (pEntry->WepStatus == Ndis802_11TKIPEnable)
		ptk_len = 32+32;
	else
		ptk_len = 32+16;

	/* Derive FT PTK and PTK-NAME */
	FT_DerivePTK(pEntry->FT_PMK_R1, 
			   	pEntry->FT_PMK_R1_NAME, 
			   	pEntry->ANonce, 
			   	pEntry->SNonce, 
		   		pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, 	
		   		pEntry->Addr, 							
			   	ptk_len, 			 
			   	pEntry->PTK, 
			   	pEntry->PTK_NAME);
	
	ft_len = sizeof(FT_FTIE); 

	/* Prepare some information for authentication response using */
	NdisMoveMemory(pFtInfo_out->FtIeInfo.ANonce, pEntry->ANonce, LEN_NONCE);
	NdisMoveMemory(pFtInfo_out->FtIeInfo.SNonce, pEntry->SNonce, LEN_NONCE);

	pFtInfo_out->FtIeInfo.R0khIdLen = pR1hkEntry->R0khIdLen;
	NdisMoveMemory(pFtInfo_out->FtIeInfo.R0khId, 
		 		   pR1hkEntry->R0khId, 
		 		   pR1hkEntry->R0khIdLen);
	ft_len += (2 + pR1hkEntry->R0khIdLen);
	
	pFtInfo_out->FtIeInfo.R1khIdLen = MAC_ADDR_LEN;
	NdisMoveMemory(pFtInfo_out->FtIeInfo.R1khId, 
				   pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, 
				   MAC_ADDR_LEN);
	ft_len += (2 + MAC_ADDR_LEN);

	/* Update the total length for FTIE */
	pFtInfo_out->FtIeInfo.Len = ft_len;

	/* Prepare RSNIE for authentication response */	        
    if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK) ||
		(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode == Ndis802_11AuthModeWPA1WPA2)) 
    {
        rsnie_len = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSNIE_Len[1];
        rsnie_ptr = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSN_IE[1][0];
	}
    else
    {
        rsnie_len = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSNIE_Len[0];
        rsnie_ptr = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSN_IE[0][0];
    }

	pFtInfo_out->RSNIE_Len = 0;
	RTMPInsertRSNIE(pFtInfo_out->RSN_IE, 
					(PULONG)&pFtInfo_out->RSNIE_Len,
					rsnie_ptr, 
					rsnie_len, 
					pPmkR0Name, 
					LEN_PMK_NAME);

	return MLME_SUCCESS;

}

UINT16	FT_AuthConfirmRsnValidation(
    IN 	PRTMP_ADAPTER 		pAd,
    IN 	PMAC_TABLE_ENTRY 	pEntry,
	IN 	PFT_INFO			pFtInfo_in)
{																								
	PFT_FTIE_INFO  pPeerFtIe;
	PUINT8 	pmkid_ptr = NULL;
	UINT8	pmkid_count = 0;
	UINT8	ft_mic[16];
	PFT_R1HK_ENTRY pR1hkEntry = NULL;
	PUINT8 	pAkmSuite = NULL;
	UINT8 	count = 0;
	
	/*	The R1KH of the target AP verifies the MIC in the FTIE in 
		the Reassociation Request frame, and shall discard the 
		request if the MIC is incorrect. */	 
	FT_CalculateMIC(pEntry->Addr, 
					pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, 
					pEntry->PTK, 
					3, 
					pFtInfo_in->RSN_IE, 
					pFtInfo_in->RSNIE_Len, 
					pFtInfo_in->MdIeInfo.pMdIe, 
					pFtInfo_in->MdIeInfo.Len + 2, 
					pFtInfo_in->FtIeInfo.pFtIe, 
					pFtInfo_in->FtIeInfo.Len + 2, 
					pFtInfo_in->RicInfo.RicIEs, 
					pFtInfo_in->RicInfo.RicIEsLen, 
					ft_mic);
	
	if (!RTMPEqualMemory(ft_mic, pFtInfo_in->FtIeInfo.MIC, FT_MIC_LEN))
	{
		DBGPRINT_ERR(("%s : MIC is different\n", __FUNCTION__));
		hex_dump("received MIC", pFtInfo_in->FtIeInfo.MIC, FT_MIC_LEN);
		hex_dump("desired  MIC", ft_mic, FT_MIC_LEN);						
		return 0xFFFF;
	}

	pPeerFtIe = &pFtInfo_in->FtIeInfo;

	/* Look up the R1KH Table */
	pR1hkEntry = FT_R1khEntryTabLookup(pAd, pEntry->FT_PMK_R1_NAME);
	if (pR1hkEntry == NULL)
	{
		DBGPRINT_ERR(("%s : Invalid R1KH table in target AP\n", __FUNCTION__));
		return FT_STATUS_CODE_RESERVED;						
	}

	/* Extract the AKM suite from the received RSNIE */
	pAkmSuite = WPA_ExtractSuiteFromRSNIE(pFtInfo_in->RSN_IE, 
								  pFtInfo_in->RSNIE_Len, 
								  AKM_SUITE, &count);
	
	if ((pAkmSuite == NULL) ||
		(RTMPEqualMemory(pAkmSuite, pR1hkEntry->AkmSuite, 4) == FALSE))
	{
		/* 	It doesn't a negotiated AKM of Fast BSS Transition, the AP 
			shall reject the Authentication Request with status code 43 
			("Invalid AKMP"). */
		DBGPRINT_ERR(("%s : Invalid AKMP\n", __FUNCTION__));
		return MLME_INVALID_AKMP;
	}
						
	/*  If the FTIE in the Reassociation Request frame contains 
		a different R0KH-ID, R1KH-ID, ANonce, or SNonce, the AP 
		shall reject the Reassociation Request with status code
		55 ("Invalid FTIE").  */
	if ((RTMPEqualMemory(pPeerFtIe->R0khId, pR1hkEntry->R0khId, pR1hkEntry->R0khIdLen) == FALSE) ||
		(RTMPEqualMemory(pPeerFtIe->R1khId, pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, MAC_ADDR_LEN) == FALSE) ||
		(RTMPEqualMemory(pPeerFtIe->ANonce, pEntry->ANonce, 32) == FALSE) ||
		(RTMPEqualMemory(pPeerFtIe->SNonce, pEntry->SNonce, 32) == FALSE))
	{
		DBGPRINT_ERR(("%s : Invalid FTIE\n", __FUNCTION__));
		return FT_STATUS_CODE_INVALID_FTIE;						
	}

	/*	If the RSNIE in the Reassociation Request frame contains an invalid 
		PMKR1Name, the AP shall reject the Reassociation Request with status 
		code 53 ("Invalid PMKID"). */
	pmkid_ptr = WPA_ExtractSuiteFromRSNIE(pFtInfo_in->RSN_IE, pFtInfo_in->RSNIE_Len, PMKID_LIST, &pmkid_count);
	if ((pmkid_ptr == NULL) || 
		(RTMPEqualMemory(pmkid_ptr, pEntry->FT_PMK_R1_NAME, LEN_PMK_NAME) == FALSE))
	{
		DBGPRINT_ERR(("%s : Invalid PMKID\n", __FUNCTION__));
		return FT_STATUS_CODE_INVALID_PMKID;						
	}
					
	return MLME_SUCCESS;

}

UINT16	FT_AssocReqRsnValidation(
    IN 	PRTMP_ADAPTER 		pAd,
    IN 	PMAC_TABLE_ENTRY 	pEntry,
	IN 	PFT_INFO			pFtInfo_in,
	OUT PFT_INFO			pFtInfo_out)
{																								
	PFT_FTIE_INFO  pPeerFtIe;
	PUINT8 	pmkid_ptr = NULL;
	UINT8	pmkid_count = 0;
	UINT8	ft_mic[16];
	UINT8 	rsnie_len = 0;
    PUINT8  rsnie_ptr = NULL;
	PFT_R1HK_ENTRY pR1hkEntry = NULL;
	PUINT8 	pAkmSuite = NULL;
	UINT8 	count = 0;


	/*	The R1KH of the target AP verifies the MIC in the FTIE in 
		the Reassociation Request frame, and shall discard the 
		request if the MIC is incorrect. */
	FT_CalculateMIC(pEntry->Addr, 
					pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, 
					pEntry->PTK, 
					5, 
					pFtInfo_in->RSN_IE, 
					pFtInfo_in->RSNIE_Len, 
					pFtInfo_in->MdIeInfo.pMdIe, 
					pFtInfo_in->MdIeInfo.Len + 2, 
					pFtInfo_in->FtIeInfo.pFtIe, 
					pFtInfo_in->FtIeInfo.Len + 2, 
					pFtInfo_in->RicInfo.RicIEs,
					pFtInfo_in->RicInfo.RicIEsLen,
					ft_mic);

	if (!RTMPEqualMemory(ft_mic, pFtInfo_in->FtIeInfo.MIC, 16))
	{
		DBGPRINT_ERR(("%s : MIC is different\n", __FUNCTION__));
		hex_dump("received MIC", pFtInfo_in->FtIeInfo.MIC, 16);
		hex_dump("desired  MIC", ft_mic, 16);						
		return 0xFFFF;
	}

	pPeerFtIe = &pFtInfo_in->FtIeInfo;

	/* Look up the R1KH Table */
	pR1hkEntry = FT_R1khEntryTabLookup(pAd, pEntry->FT_PMK_R1_NAME);
	if (pR1hkEntry == NULL)
	{
		DBGPRINT_ERR(("%s : Invalid R1KH table in target AP\n", __FUNCTION__));
		return FT_STATUS_CODE_RESERVED;						
	}

	/* Extract the AKM suite from the received RSNIE */
	pAkmSuite = WPA_ExtractSuiteFromRSNIE(pFtInfo_in->RSN_IE, 
								  pFtInfo_in->RSNIE_Len, 
								  AKM_SUITE, &count);
	
	if ((pAkmSuite == NULL) ||
		(RTMPEqualMemory(pAkmSuite, pR1hkEntry->AkmSuite, 4) == FALSE))
	{
		/* 	It doesn't a negotiated AKM of Fast BSS Transition, the AP 
			shall reject the Authentication Request with status code 43 
			("Invalid AKMP"). */
		DBGPRINT_ERR(("%s : Invalid AKMP\n", __FUNCTION__));
		return MLME_INVALID_AKMP;
	}
						
	/*  If the FTIE in the Reassociation Request frame contains 
		a different R0KH-ID, R1KH-ID, ANonce, or SNonce, the AP 
		shall reject the Reassociation Request with status code
		55 ("Invalid FTIE").  */
	if ((RTMPEqualMemory(pPeerFtIe->R0khId, pR1hkEntry->R0khId, pR1hkEntry->R0khIdLen) == FALSE) ||
		(RTMPEqualMemory(pPeerFtIe->R1khId, pAd->ApCfg.MBSSID[pEntry->func_tb_idx].Bssid, MAC_ADDR_LEN) == FALSE) ||
		(RTMPEqualMemory(pPeerFtIe->ANonce, pEntry->ANonce, 32) == FALSE) ||
		(RTMPEqualMemory(pPeerFtIe->SNonce, pEntry->SNonce, 32) == FALSE))
	{
		DBGPRINT_ERR(("%s : Invalid FTIE\n", __FUNCTION__));
		return FT_STATUS_CODE_INVALID_FTIE;						
	}

	/*	If the RSNIE in the Reassociation Request frame contains an invalid 
		PMKR1Name, the AP shall reject the Reassociation Request with status 
		code 53 ("Invalid PMKID"). */
	pmkid_ptr = WPA_ExtractSuiteFromRSNIE(pFtInfo_in->RSN_IE, pFtInfo_in->RSNIE_Len, PMKID_LIST, &pmkid_count);
	if ((pmkid_ptr == NULL) || 
		(RTMPEqualMemory(pmkid_ptr, pEntry->FT_PMK_R1_NAME, LEN_PMK_NAME) == FALSE))
	{
		DBGPRINT_ERR(("%s : Invalid PMKID\n", __FUNCTION__));
		return FT_STATUS_CODE_INVALID_PMKID;						
	}

	/* Prepare RSNIE for outgoing frame */
	if ((pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK) ||
		(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].AuthMode == Ndis802_11AuthModeWPA1WPA2)) 
    {
        rsnie_len = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSNIE_Len[1];
        rsnie_ptr = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSN_IE[1][0];
	}
    else
    {
        rsnie_len = pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSNIE_Len[0];
        rsnie_ptr = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].RSN_IE[0][0];
    }

	pFtInfo_out->RSNIE_Len = 0;
	RTMPInsertRSNIE(pFtInfo_out->RSN_IE, 
					(PULONG)&pFtInfo_out->RSNIE_Len,
					rsnie_ptr, 
					rsnie_len, 
					pEntry->FT_PMK_R1_NAME, 
					LEN_PMK_NAME);
	
	/* Prepare MIC-control and MIC field of FTIE for outgoing frame. */										
	pFtInfo_out->FtIeInfo.MICCtr.field.IECnt = 3;
	NdisZeroMemory(pFtInfo_out->FtIeInfo.MIC, 16);

	/* Prepare ANonce and Snonce field of FTIE for outgoing frame */
	NdisMoveMemory(pFtInfo_out->FtIeInfo.ANonce, pEntry->ANonce, LEN_NONCE);
	NdisMoveMemory(pFtInfo_out->FtIeInfo.SNonce, pEntry->SNonce, LEN_NONCE);
	
	/* Prepare GTK related information of FTIE for outgoing frame */
	FT_ConstructGTKSubIe(pAd, pEntry, &pFtInfo_out->FtIeInfo);															
	/*ft_len += (2 + pFtInfo_out->FtIeInfo.GtkLen); */
				
	/* Prepare RIC-Response */
#if 0
	if (pFtInfo_in->RicInfo.Len)
	{
		pFtInfo_out->FtIeInfo.MICCtr.field.IECnt += 1;
	}
#endif				
					
	return MLME_SUCCESS;

}
#endif /* CONFIG_AP_SUPPORT */


/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_FillMdIeInfo(
	PEID_STRUCT eid_ptr,
	PFT_MDIE_INFO pMdIeInfo)
{
	PFT_MDIE pMdIe;

	pMdIeInfo->Len = 3;
	pMdIeInfo->pMdIe = eid_ptr;	/* store the pointer of the original MD-IE for MIC calculating */ 

	pMdIe = (PFT_MDIE)(eid_ptr->Octet);
	FT_SET_MDID(pMdIeInfo->MdId, pMdIe->MdId);

	NdisMoveMemory(&(pMdIeInfo->FtCapPlc.word), &pMdIe->FtCapPlc.word,
		sizeof(FT_CAP_AND_POLICY));
}

/*
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_FillFtIeInfo(
	PEID_STRUCT eid_ptr,
	PFT_FTIE_INFO pFtIeInfo)
{
	PFT_FTIE pFtIe;
	PFT_OPTION_FIELD subEidPtr;
	UINT16 MicCtrBuf;
	INT RemainLen;
	PUINT8	ptr;

	RemainLen = eid_ptr->Len;
	pFtIeInfo->Len = eid_ptr->Len;
	pFtIeInfo->pFtIe = eid_ptr;		/* store the pointer of the original FT-IE for MIC calculating */ 

	pFtIe = (PFT_FTIE)eid_ptr->Octet;
	NdisMoveMemory(&MicCtrBuf, &(pFtIe->MICCtr.word),
					sizeof(FT_MIC_CTR_FIELD));
	pFtIeInfo->MICCtr.word = le2cpu16(MicCtrBuf);
	RemainLen -= 2;

	NdisMoveMemory(pFtIeInfo->MIC, pFtIe->MIC, 16);
	RemainLen -= 16;

	NdisMoveMemory(pFtIeInfo->ANonce, pFtIe->ANonce, 32);
	RemainLen -= 32;

	NdisMoveMemory(pFtIeInfo->SNonce, pFtIe->SNonce, 32);
	RemainLen -= 32;
	
	/* Pare sub-element field. */
	/*subEidPtr = (PFT_OPTION_FIELD)(pFtIe->Option); */
	ptr = pFtIe->Option;
	while (RemainLen > 0)
	{
		subEidPtr = (PFT_OPTION_FIELD)ptr;
	
		switch(subEidPtr->SubElementId)
		{
			case FT_R0KH_ID:
				if ((subEidPtr->Len > 0) && (subEidPtr->Len <=FT_ROKH_ID_LEN))
				{
					pFtIeInfo->R0khIdLen = subEidPtr->Len;
					NdisMoveMemory(pFtIeInfo->R0khId, subEidPtr->Oct,
						pFtIeInfo->R0khIdLen);
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("%s Invalid R0KHID Length (%d)\n",
						__FUNCTION__, subEidPtr->Len));
				break;

			case FT_R1KH_ID:
				if (subEidPtr->Len == FT_R1KH_ID_LEN)
				{
					pFtIeInfo->R1khIdLen = subEidPtr->Len;
					NdisMoveMemory(pFtIeInfo->R1khId, subEidPtr->Oct,
						pFtIeInfo->R1khIdLen);
				}
				else
					DBGPRINT(RT_DEBUG_ERROR, ("%s Invalid R1KHID Length (%d)\n",
						__FUNCTION__, subEidPtr->Len));
				break;

			case FT_GTK:
				if (subEidPtr->Len > 0)
				{
					pFtIeInfo->GtkLen = subEidPtr->Len;
					NdisMoveMemory(pFtIeInfo->GtkSubIE, &subEidPtr->Oct[0], subEidPtr->Len);
				}
				break;

			default:
				break;
		}

		ptr += (subEidPtr->Len + 2);
		RemainLen -= (subEidPtr->Len + 2);

		/* avoid infinite loop. */
		if (subEidPtr->Len == 0)
			break;
	}
}

#ifdef CONFIG_STA_SUPPORT
VOID FT_FTIeParse(
	IN		UINT8		FtIeLen,
	IN		PFT_FTIE	pFtIe,
	OUT		PUCHAR		pR1KH_Id,
	OUT		UCHAR		*GTKLen,
	OUT		PUCHAR		pGTK,
	OUT		UCHAR		*R0KH_IdLen,
	OUT		PUCHAR		pR0KH_Id)
{
	UCHAR	*ptr;
	UINT8	RemainLen;
	PFT_OPTION_FIELD subEidPtr;

	*GTKLen = 0;
	*R0KH_IdLen = 0;

	ptr = (PUCHAR)&pFtIe->Option[0];
	RemainLen = FtIeLen - sizeof(FT_FTIE);
	DBGPRINT(RT_DEBUG_TRACE, ("FT_TEMP- FtIeParse (  FtIeLen = %d )\n", FtIeLen));
	DBGPRINT(RT_DEBUG_TRACE, ("FT_TEMP- FtIeParse ( Len that doesn't include subelement = %d )\n", RemainLen));

	while (RemainLen > 0)
	{
		subEidPtr = (PFT_OPTION_FIELD)ptr;
	
		if (subEidPtr->SubElementId == FT_R1KH_ID)
		{
			RTMPMoveMemory(pR1KH_Id, subEidPtr->Oct, subEidPtr->Len);	
			DBGPRINT(RT_DEBUG_TRACE, ("%s : R1KHID length(%d)\n", __FUNCTION__, subEidPtr->Len));
		}
		else if (subEidPtr->SubElementId == FT_GTK)
		{
			*GTKLen = subEidPtr->Len;
			DBGPRINT(RT_DEBUG_TRACE, ("FT_TEMP- FtIeParse ( *GTKLen = %d )\n", *GTKLen));
			if ((*GTKLen >= 15) && (*GTKLen <= 64))
			{
				RTMPMoveMemory(pGTK, subEidPtr->Oct, subEidPtr->Len);
			}
			else
			{
				*GTKLen = 0;
				DBGPRINT(RT_DEBUG_ERROR, ("FT- FtIeParse ( Invalid  GTKLen  = %d)\n", *GTKLen));
			}
			
		}
		else if (subEidPtr->SubElementId == FT_R0KH_ID)
		{
			*R0KH_IdLen = subEidPtr->Len;
			DBGPRINT(RT_DEBUG_TRACE, ("FT_TEMP- FtIeParse ( *R0KH_IdLen = %d )\n", *R0KH_IdLen));
			if ((*R0KH_IdLen >= 1) && (*R0KH_IdLen <= 48))
			{
				RTMPMoveMemory(pR0KH_Id, subEidPtr->Oct, subEidPtr->Len);
			}
			else
			{
				DBGPRINT(RT_DEBUG_ERROR, ("FT- FtIeParse ( Invalid  R0KH_IdLen = %d )\n",*R0KH_IdLen));
				*R0KH_IdLen = 0;
			}
			
		}
		
		ptr += (subEidPtr->Len + 2);
		RemainLen -= (subEidPtr->Len + 2);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s done\n", __FUNCTION__));

}


/*
	==========================================================================
	Description:
		

	IRQL = DISPATCH_LEVEL

	Output:
	==========================================================================
 */
BOOLEAN FT_CheckForRoaming(
	IN	PRTMP_ADAPTER	pAd)
{
	USHORT		i;
	BSS_TABLE	*pRoamTab = &pAd->MlmeAux.RoamTab;
	BSS_ENTRY	*pBss;
	CHAR max_rssi;
	
	DBGPRINT(RT_DEBUG_TRACE, ("==> FT_CheckForRoaming\n"));
	/* put all roaming candidates into RoamTab, and sort in RSSI order */
	BssTableInit(pRoamTab);
	for (i = 0; i < pAd->ScanTab.BssNr; i++)
	{
		pBss = &pAd->ScanTab.BssEntry[i];

		if (pBss->bHasMDIE == FALSE)
			continue;	/* skip legacy AP */

		if (MAC_ADDR_EQUAL(pBss->Bssid, pAd->CommonCfg.Bssid))
			continue;	 /* skip current AP */
		if (!FT_MDID_EQU(pBss->FT_MDIE.MdId, pAd->StaCfg.Dot11RCommInfo.MdIeInfo.MdId))
			continue;	 /* skip different MDID */

		if ((pBss->Rssi <= -85) && (pBss->Channel == pAd->CommonCfg.Channel))
			continue;	/* skip RSSI too weak at the same channel */

		if ((pBss->Channel != pAd->CommonCfg.Channel) &&
			(pBss->FT_MDIE.FtCapPlc.field.FtOverDs == FALSE))
			continue;	/* skip AP in different channel without supporting FtOverDs */

		max_rssi = RTMPMaxRssi(pAd, pAd->StaCfg.RssiSample.LastRssi[0],
								pAd->StaCfg.RssiSample.LastRssi[1],
								pAd->StaCfg.RssiSample.LastRssi[2]);
		if (pBss->Rssi < (max_rssi + RSSI_DELTA))
			continue;	/* skip AP without better RSSI */

		if ((pBss->AuthMode != pAd->StaCfg.AuthMode) ||
			(pBss->WepStatus != pAd->StaCfg.WepStatus))
			continue;	 /* skip different Security Setting */

		DBGPRINT(RT_DEBUG_TRACE, ("max_rssi = %d, pBss->Rssi = %d\n", max_rssi, pBss->Rssi));
		/* AP passing all above rules is put into roaming candidate table */
		NdisMoveMemory(&pRoamTab->BssEntry[pRoamTab->BssNr], pBss, sizeof(BSS_ENTRY));
		pRoamTab->BssNr += 1;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("<== FT_CheckForRoaming (BssNr=%d)\n", pRoamTab->BssNr));
	if (pRoamTab->BssNr > 0)
	{
		/* check CntlMachine.CurrState to avoid collision with NDIS SetOID request */
		if (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE)
		{
			pAd->RalinkCounters.PoorCQIRoamingCount ++;
			DBGPRINT(RT_DEBUG_TRACE, ("MMCHK - Roaming attempt #%ld\n", pAd->RalinkCounters.PoorCQIRoamingCount));
			MlmeEnqueue(pAd, MLME_CNTL_STATE_MACHINE, MT2_MLME_ROAMING_REQ, 0, NULL, 0);
			RTMP_MLME_HANDLER(pAd);
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN	FT_GetMDIE(
	IN  PNDIS_802_11_VARIABLE_IEs	pVIE,
	IN  USHORT						LengthVIE,
	OUT FT_MDIE_INFO				*pMdIeInfo)
{
	PEID_STRUCT     pEid;
	USHORT          Length = 0;

	pEid = (PEID_STRUCT) pVIE;
	pMdIeInfo->Len = 0;
	while ((Length + 2 + (USHORT)pEid->Len) <= LengthVIE)    
	{
		switch(pEid->Eid)
		{
			case IE_FT_MDIE:
				if (pEid->Len == sizeof(FT_MDIE))
				{
					NdisMoveMemory(&pMdIeInfo->MdId[0], &pEid->Octet[0], FT_MDID_LEN);
					pMdIeInfo->FtCapPlc.word = pEid->Octet[FT_MDID_LEN];
					pMdIeInfo->Len = pEid->Len;
				}
				return TRUE;
		}
		Length = Length + 2 + pEid->Len;  /* Eid[1] + Len[1]+ content[Len] */
        pEid = (PEID_STRUCT)((UCHAR*)pEid + 2 + pEid->Len);   
	}
	return FALSE;
}


BOOLEAN FT_ExtractGTKSubIe(
	IN	PRTMP_ADAPTER 		pAd,
	IN 	PMAC_TABLE_ENTRY 	pEntry,
	IN	PFT_FTIE_INFO		pFtInfo)
{
	PFT_GTK_KEY_INFO 	pKeyInfo;	
	UCHAR				gtk_len;
	UINT				unwrap_len;
	PUINT8				pData;
	UINT8				data_offset = 0;
	UINT8				key_p[64];

	if (pFtInfo->GtkLen < 11)
	{
		DBGPRINT_ERR(("%s : The length is invalid\n", __FUNCTION__));
		return FALSE;
	}

	pData = pFtInfo->GtkSubIE;

	/* Extract the Key Info field */
	pKeyInfo = (PFT_GTK_KEY_INFO)pData;
	pKeyInfo->word = cpu2le16(pKeyInfo->word);
	pAd->StaCfg.DefaultKeyId = pKeyInfo->field.KeyId;
	data_offset += sizeof(FT_GTK_KEY_INFO);

	DBGPRINT(RT_DEBUG_TRACE, ("%s : key idx(%d) \n", __FUNCTION__, pAd->StaCfg.DefaultKeyId));

	/* Extract the Key Length field */
	gtk_len = *(pData + data_offset);
	data_offset += 1;

	/* Extract the RSC field */
	data_offset += 8;

	/* Decrypt the Key field by AES Key UNWRAP */
	AES_Key_Unwrap(pData + data_offset, pFtInfo->GtkLen - data_offset, 
				   &pEntry->PTK[LEN_PTK_KCK], LEN_PTK_KEK, 
				   key_p, &unwrap_len);

	/* Compare the GTK length */
	if (unwrap_len != gtk_len)
	{
		DBGPRINT_ERR(("%s : The GTK length is unmatched\n", __FUNCTION__));
		return FALSE;
	}	
	
	/* set key material, TxMic and RxMic */
	NdisZeroMemory(pAd->StaCfg.GTK, MAX_LEN_GTK);
	NdisMoveMemory(pAd->StaCfg.GTK, key_p, gtk_len);
	
	return TRUE;
}

/*	
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
VOID FT_ConstructAuthReqInRsn(
	IN 	PRTMP_ADAPTER 	pAd,
	IN 	PUCHAR 			pFrameBuf,
	OUT PULONG 			pFrameLen)
{
	UINT8 	FtIeLen = 0;
	FT_MIC_CTR_FIELD FtMicCtr;
	UINT8	ft_mic[16];
	UINT8	anonce[32];

	/* Insert RSNIE[PMKR0Name] */
	RTMPInsertRSNIE(pFrameBuf + (*pFrameLen), 
					pFrameLen, 
					pAd->StaCfg.RSN_IE, 
					pAd->StaCfg.RSNIE_Len, 
					pAd->StaCfg.Dot11RCommInfo.PMKR0Name, 
					LEN_PMK_NAME);

	/* 	Insert FTIE[SNonce, R0KH-ID] 
		R0KH-ID: Optional parameter - Sub-EID(1 byte)+Len(1 byte)+Data(variable bytes) */
	FtIeLen = sizeof(FT_FTIE) + 2 + pAd->StaCfg.Dot11RCommInfo.R0khIdLen;
	FtMicCtr.word = 0;
	GenRandom(pAd, pAd->CurrentAddress, pAd->MlmeAux.FtIeInfo.SNonce);
	NdisZeroMemory(ft_mic, 16);
	NdisZeroMemory(anonce, 32);
	
	FT_InsertFTIE(pAd, 
				  pFrameBuf + (*pFrameLen), 
				  pFrameLen, 
				  FtIeLen, 
				  FtMicCtr, 
				  ft_mic, 
				  anonce, 
				  &pAd->MlmeAux.FtIeInfo.SNonce[0]);
	
	FT_FTIE_InsertKhIdSubIE(pAd, 
							pFrameBuf + (*pFrameLen), 
							pFrameLen, 
							FT_R0KH_ID, 
							&pAd->StaCfg.Dot11RCommInfo.R0khId[0], 
							pAd->StaCfg.Dot11RCommInfo.R0khIdLen);
	

}

#endif /* CONFIG_STA_SUPPORT */

/*
	========================================================================
	
	Routine Description:
		It is used to derive the first level FT Key Hierarchy key, PMK-R0, 
		and its identifier PMKR0Name.
		(IEEE 802.11r/D9.0, 8.5.1.5.3)

	Arguments:

	Return Value:

	Note:
		R0-Key-Data = 
			KDF-384(XXKey, "FT-R0", 
					SSIDlength || SSID || MDID || 
					R0KHlength || R0KH-ID || S0KH-ID)
		PMK-R0 = L(R0-Key-Data, 0, 256)
		PMK-R0Name-Salt = L(R0-Key-Data, 256, 128)		

		PMKR0Name = Truncate-128(SHA-256("FT-R0N" || PMK-R0Name-Salt))

	========================================================================
*/
VOID FT_DerivePMKR0(
	IN	PUINT8	xxkey,
	IN	INT		xxkey_len,
	IN	PUINT8	ssid,
	IN	INT		ssid_len,
	IN	PUINT8	mdid,
	IN	PUINT8 r0khid,
	IN	INT		r0khid_len,
	IN	PUINT8	s0khid,
	OUT	PUINT8	pmkr0,
	OUT	PUINT8	pmkr0_name)
{	
	const char label_name[] = "FT-R0N";
	UCHAR	R0KeyData[96];
	UCHAR	PmkR0NameSalt[20];
	UCHAR	temp_result[64];
	UCHAR   context[128];
	UINT    c_len=0; 

	/* =========================== */
	/* 		PMK-R0 derivation */
	/* =========================== */

	/* construct the concatenated context for PMK-R0 */
	/* SSIDlength(1 byte) */
	/* SSID(0~32 bytes) */
	/* MDID(2 bytes) */
	/* R0KHlength(1 byte) */
	/* R0KH-ID(5~48 bytes) */
	/* S0KH-ID(6 bytes) */

	hex_dump("xxkey", (PUCHAR)xxkey, xxkey_len);
	hex_dump("label", (PUCHAR)label_name, sizeof(label_name));
	hex_dump("ssid", (PUCHAR)ssid, ssid_len);
	hex_dump("mdis", (PUCHAR)mdid, 2);
	hex_dump("r0khid", (PUCHAR)r0khid, r0khid_len);
	hex_dump("s0khid", (PUCHAR)s0khid, MAC_ADDR_LEN);

	/* Initial the related context */
	NdisZeroMemory(temp_result, 64);
	NdisZeroMemory(context, 128);
	c_len = 0;

	/* concatenate the SSIDlength with a single octet */
	context[0] = ssid_len;
	c_len += 1;

	/* concatenate the SSID with its length */
	NdisMoveMemory(&context[c_len], ssid, ssid_len);
	c_len += ssid_len;

	/* concatenate the MDID with 2-octets */
	NdisMoveMemory(&context[c_len], mdid, 2);
	c_len += 2;

	/* concatenate the R0KHlength with a single octet */
	context[c_len] = r0khid_len;
	c_len += 1;

	/* concatenate the R0KH-ID with its length */
	NdisMoveMemory(&context[c_len], r0khid, r0khid_len);
	c_len += r0khid_len;

	/* concatenate the S0KH-ID with its length */
	NdisMoveMemory(&context[c_len], s0khid, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* Calculate a 48-bytes key material through FT-KDF */
	KDF(xxkey, xxkey_len, (PUINT8)"FT-R0", 5, context, c_len, R0KeyData, 48);

	/* PMK-R0 key shall be computed as the first 256 bits (bits 0-255) */
	/* of the R0-Key-Data. The latter 128 bits of R0-Key-Data shall */
	/* be used as the PMK-R0Name-Salt to generate the PMKR0Name. */
	NdisMoveMemory(pmkr0, R0KeyData, LEN_PMK);
	NdisMoveMemory(PmkR0NameSalt, &R0KeyData[32], LEN_PMK_NAME);

	/* =============================== */
	/* 		PMK-R0-Name derivation */
	/* =============================== */
	
	/* Initial the related parameter for PMK-R0-Name derivation */
	NdisZeroMemory(temp_result, 64);
	NdisZeroMemory(context, 128);
	c_len = 0;

	/* concatenate the label with its length */
	NdisMoveMemory(context, label_name, strlen(label_name));
	c_len += strlen(label_name);

	/* concatenate the PMK-R0Name-Salt with its length */
	NdisMoveMemory(&context[c_len], PmkR0NameSalt, LEN_PMK_NAME);
	c_len += LEN_PMK_NAME;

	RT_SHA256(context, c_len, temp_result);
	NdisMoveMemory(pmkr0_name, temp_result, LEN_PMK_NAME);

	hex_dump("PMK-R0", (UCHAR *)pmkr0, LEN_PMK);
	hex_dump("PMK-R0-Name", (UCHAR *)pmkr0_name, LEN_PMK_NAME);
	
}

/*
	========================================================================
	
	Routine Description:
		It is used to derive the second level FT Key Hierarchy key 
		identifier, PMK-R1-NAME.
		(IEEE 802.11r/D9.0, 8.5.1.5.4)

	Arguments:

	Return Value:

	Note:		
		PMKR1Name = Truncate-128(SHA-256("FT-R1N" || PMKR0Name || R1KH-ID || S1KH-ID))

	========================================================================
*/
VOID FT_DerivePMKR1Name(
	IN	PUINT8	pmkr0_name,
	IN	PUINT8 	r1khid,
	IN	PUINT8	s1khid,
	OUT	PUINT8	pmkr1_name)
{	
	const char label_name[] = "FT-R1N";
	UCHAR	temp_result[64];
	UCHAR   context[128];
	UINT    c_len = 0; 

	/* =============================== */
	/* 		PMK-R1-Name derivation */
	/* =============================== */

	/* Initial the related parameter for PMK-R1-Name derivation */
	NdisZeroMemory(temp_result, 64);
	NdisZeroMemory(context, 128);
	c_len = 0;

	/* concatenate the label with its length */
	NdisMoveMemory(context, label_name, strlen(label_name));
	c_len += strlen(label_name);

	/* concatenate the PMK-R1-Name with 16-octets */
	NdisMoveMemory(&context[c_len], pmkr0_name, LEN_PMK_NAME);
	c_len += LEN_PMK_NAME;

	/* concatenate the R1KH-ID with 6-octets */
	NdisMoveMemory(&context[c_len], r1khid, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* concatenate the S1KH-ID with 6-octets */
	NdisMoveMemory(&context[c_len], s1khid, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* derive PMK-R1-Name */
	RT_SHA256(context, c_len, temp_result);
	NdisMoveMemory(pmkr1_name, temp_result, LEN_PMK_NAME);
	
	hex_dump("PMK-R1-Name", (UCHAR *)pmkr1_name, LEN_PMK_NAME);
	
}

/*
	========================================================================
	
	Routine Description:
		It is used to derive the second level FT Key Hierarchy key, PMK-R1,
		and its identifier PMKR1Name.
		(IEEE 802.11r/D9.0, 8.5.1.5.4)

	Arguments:

	Return Value:

	Note:
		PMK-R1 = KDF-256(PMK-R0, "FT-R1", R1KH-ID || S1KH-ID)
		PMKR1Name = Truncate-128(SHA-256("FT-R1N" || PMKR0Name || R1KH-ID || S1KH-ID))

	========================================================================
*/
VOID FT_DerivePMKR1(
	IN	PUINT8	pmkr0,
	IN	PUINT8	pmkr0_name,
	IN	PUINT8 r1khid,
	IN	PUINT8	s1khid,
	OUT	PUINT8	pmkr1,
	OUT	PUINT8	pmkr1_name)
{	
	/*const char label_name[] = "FT-R1N"; */
	UCHAR	temp_result[64];
	UCHAR   context[128];
	UINT    c_len=0; 

	/* =========================== */
	/* 		PMK-R1 derivation */
	/* =========================== */

	DBGPRINT(RT_DEBUG_TRACE, ("%s:\n", __FUNCTION__));
	hex_dump("pmkr0", pmkr0, 32);
	hex_dump("pmkr0_name", pmkr0_name, LEN_PMK_NAME);
	hex_dump("r1khid", r1khid, MAC_ADDR_LEN);
	hex_dump("s1khid", s1khid, MAC_ADDR_LEN);
	hex_dump("pmkr1", pmkr1, 32);
	hex_dump("pmkr1_name", pmkr1_name, 16);

	/* construct the concatenated context for PMK-R1 */
	/* R1KH-ID(6 bytes) */
	/* S1KH-ID(6 bytes) */

	/* Initial the related parameter */
	NdisZeroMemory(temp_result, 64);
	NdisZeroMemory(context, 128);
	c_len = 0;
	
	/* concatenate the R1KH-ID with 6-octets */
	NdisMoveMemory(&context[c_len], r1khid, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* concatenate the S1KH-ID with 6-octets */
	NdisMoveMemory(&context[c_len], s1khid, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* Calculate a 32-bytes key material through FT-KDF */
	KDF(pmkr0, LEN_PMK, (PUINT8)"FT-R1", 5, context, c_len, temp_result, 32);
	NdisMoveMemory(pmkr1, temp_result, LEN_PMK);

	/* =============================== */
	/* 		PMK-R1-Name derivation */
	/* =============================== */
	FT_DerivePMKR1Name(pmkr0_name, 
					   r1khid, 
					   s1khid, 
					   pmkr1_name);

	hex_dump("PMK-R1", (UCHAR *)pmkr1, LEN_PMK);
	hex_dump("PMK-R1-Name", (UCHAR *)pmkr1_name, LEN_PMK_NAME);
	
}

/*
	========================================================================
	
	Routine Description:
		It is used to derive the third level FT Key Hierarchy key, PTK,
		and its identifier PTKName.
		(IEEE 802.11r/D9.0, 8.5.1.5.4)

	Arguments:

	Return Value:

	Note:
		PTK = KDF-PTKLen(PMK-R1, "FT-PTK", SNonce || ANonce || BSSID || STA-ADDR)
		PTKName = 
			Truncate-128(SHA-256(PMKR1Name || "FT-PTKN" || SNonce || ANonce || BSSID || 
							STA-ADDR))

	========================================================================
*/
VOID FT_DerivePTK(
	IN	PUINT8	pmkr1,
	IN	PUINT8	pmkr1_name,
	IN	PUINT8 	a_nonce,
	IN	PUINT8 	s_nonce,
	IN	PUINT8 	bssid,
	IN	PUINT8 	sta_mac,
	IN	UINT	key_len,
	OUT	PUINT8	ptk,
	OUT	PUINT8	ptk_name)
{	
	const char label_name[] = "FT-PTKN";
	UCHAR	temp_result[64];
	UCHAR   context[128];
	UINT    c_len=0; 

	/* =============================== */
	/* 		PTK derivation */
	/* =============================== */

	/* construct the concatenated context for PTK */
	/* SNonce (32 bytes) */
	/* ANonce (32 bytes) */
	/* BSSID (6 bytes) */
	/* STA-ADDR (6-bytes) */

	/* Initial the related parameter */
	NdisZeroMemory(temp_result, 64);
	NdisZeroMemory(context, 128);
	c_len = 0;
	
	/* concatenate the SNonce with 32-octets */
	NdisMoveMemory(&context[c_len], s_nonce, LEN_NONCE);
	c_len += LEN_NONCE;

	/* concatenate the ANonce with 32-octets */
	NdisMoveMemory(&context[c_len], a_nonce, LEN_NONCE);
	c_len += LEN_NONCE;

	/* concatenate the BSSID with 6-octets */
	NdisMoveMemory(&context[c_len], bssid, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* concatenate the STA-ADDR with 6-octets */
	NdisMoveMemory(&context[c_len], sta_mac, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* Calculate a key material through FT-KDF */
	KDF(pmkr1, 
			LEN_PMK, 
			(PUINT8)"FT-PTK", 
			6, 
			context, 
			c_len, 
			temp_result, 
			key_len);
	NdisMoveMemory(ptk, temp_result, key_len);

	/* =============================== */
	/* 		PTK-Name derivation */
	/* =============================== */

	/* Initial the related parameter for PTK-Name derivation */
	NdisZeroMemory(temp_result, 64);
	NdisZeroMemory(context, 128);
	c_len = 0;

	/* concatenate the PMK-R1-Name with 16-octets */
	NdisMoveMemory(&context[c_len], pmkr1_name, LEN_PMK_NAME);
	c_len += LEN_PMK_NAME;

	/* concatenate the label with its length */
	NdisMoveMemory(&context[c_len], label_name, strlen(label_name));
	c_len += strlen(label_name);

	/* concatenate the SNonce with 32-octets */
	NdisMoveMemory(&context[c_len], s_nonce, LEN_NONCE);
	c_len += LEN_NONCE;

	/* concatenate the ANonce with 32-octets */
	NdisMoveMemory(&context[c_len], a_nonce, LEN_NONCE);
	c_len += LEN_NONCE;

	/* concatenate the BSSID with 6-octets */
	NdisMoveMemory(&context[c_len], bssid, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* concatenate the STA-ADDR with 6-octets */
	NdisMoveMemory(&context[c_len], sta_mac, MAC_ADDR_LEN);
	c_len += MAC_ADDR_LEN;

	/* Derive PTKName */
	RT_SHA256(context, c_len, temp_result);
	NdisMoveMemory(ptk_name, temp_result, LEN_PMK_NAME);

	/*hex_dump("ANonce", (UCHAR *)a_nonce, LEN_NONCE); */
	/*hex_dump("SNonce", (UCHAR *)s_nonce, LEN_NONCE); */

	hex_dump("PTK", (UCHAR *)ptk, key_len);
	hex_dump("PTK-Name", (UCHAR *)ptk_name, LEN_PMK_NAME);
	
}

#if 0
VOID FT_TEST(int input)
{
	char	key_tmp[] = "abc";
	char	data_tmp[] = "data";
	char 	hash[32];
	char 	xxkey[32] = {
			0x9C, 0x1E, 0x49, 0x18, 0x59, 0x8B, 0x67, 0xB2, 
			0x53, 0x5D, 0xB9, 0x7D, 0xF6, 0x5D, 0x70, 0x78, 
			0x26, 0x2D, 0xC8, 0x81, 0xFB, 0x55, 0x5B, 0x31, 
			0xB4, 0xF3, 0xDB, 0x3A, 0x8A, 0xF2, 0xD7, 0x67
		};
	char anonce[32] = {
			0x31, 0xB4, 0xB5, 0xA6, 0xFE, 0xBE, 0xB3, 0xCB, 
			0xCA, 0xCA, 0xCB, 0xCE, 0xAE, 0x72, 0xE2, 0xC7, 
			0x12, 0xB7, 0x34, 0x7B, 0x3D, 0xE3, 0xBE, 0x29, 
			0x72, 0xE7, 0xAF, 0x1B, 0x7A, 0xBE, 0xE0, 0xA3
		};	
	char snonce[32] = {
			0x21, 0xF7, 0x7F, 0xF1, 0xF6, 0x2B, 0x78, 0xE2, 
			0xDD, 0xC0, 0x3F, 0x24, 0x7B, 0xBC, 0xDF, 0x10,
			0x63, 0xA3, 0x25, 0xFB, 0x29, 0x61, 0x44, 0x04, 
			0x7B, 0xD8, 0x0F, 0x3C, 0x0D, 0xD7, 0xB2, 0x2C
		};	
	char ssid[11] = {
			0x52, 0x61, 0x6c, 0x69, 0x6e, 0x6b, 0x57, 0x69, 
			0x46, 0x69, 0x31
		};
	char mdid[2] = {0x36, 0x34};
	char r0kh_id[24] = {
			0x30, 0x30, 0x30, 0x43, 0x34, 0x33, 0x31, 0x30, 
			0x31, 0x32, 0x35, 0x30, 0x00, 0x65, 0x35, 0x3a, 
			0x39, 0x61, 0x3a, 0x34, 0x38, 0x3a, 0x63, 0x32
		};
	char sta_addr[6] = {0x00, 0x0F, 0x86, 0x56, 0xA2, 0xB3};
	char bssid[6] = {0x00, 0x0c, 0x43, 0x10, 0x12, 0x50};
	char s0kh_id[6];
	char r1kh_id[6];
	char s1kh_id[6];	
	char pmk_r0[32];
	char pmk_r0_name[16];
	char pmk_r1[32]; /*= {
			0xBE, 0x3F, 0x65, 0xF9, 0x9F, 0x19, 0x51, 0x82, 
			0xA7, 0x88, 0x53, 0x72, 0xFA, 0x50, 0xE4, 0xBF, 
			0x52, 0xD9, 0x49, 0x95, 0xD1, 0x90, 0x94, 0xA4, 
			0xBB, 0x45, 0x7D, 0x78, 0x71, 0x8D, 0x57, 0xCD
		};*/
	char pmk_r1_name[16]; /* = {
			0xEE, 0xF9, 0xCB, 0x73, 0x04, 0x48, 0xC3, 0xD3, 
			0xB4, 0xDF, 0xB8, 0x07, 0x17, 0x69, 0x04, 0x35
		};*/
	char ptk[48];
	char ptk_name[16];
 
	/*KDF(key_tmp, 3, "KDF test", 8, (RTMP_STRING *)data_tmp, 4, hash, sizeof(hash)); */
	/*hex_dump("KDF", hash, sizeof(hash)); */
	NdisMoveMemory(s0kh_id, sta_addr, sizeof(sta_addr));
	NdisMoveMemory(s1kh_id, sta_addr, sizeof(sta_addr));
	NdisMoveMemory(r1kh_id, bssid, sizeof(bssid));
	
	FT_DerivePMKR0(xxkey, sizeof(xxkey), ssid, sizeof(ssid), 
				   mdid, r0kh_id, sizeof(r0kh_id), s0kh_id, pmk_r0, pmk_r0_name);
	FT_DerivePMKR1(pmk_r0, pmk_r0_name, r1kh_id, s1kh_id, pmk_r1, pmk_r1_name);
	FT_DerivePTK(pmk_r1, pmk_r1_name, anonce, snonce, bssid, sta_addr, 48, ptk, ptk_name);
	
}
#endif 

/*
	========================================================================
	
	Routine Description:
		Calcaulate FT MIC. It is used during Fast BSS transition.

	Arguments:

	Return Value:

	Note:
		It's defined in IEEE 802.11r D9.0 11A.8.4/8.5
		The MIC shall be calculated using the KCK and the AES-128-CMAC 
		algorithm. The output of the AES-128-CMAC shall be 128 bits.

		The MIC shall be calculated on the concatenation, in the 
		following order, of:
		-  non-AP STA MAC address (6 octets)
		-  Target AP MAC address (6 octets)
		-  Transaction sequence number (1 octet)
		-  Contents of the RSN information element.
		-  Contents of the MDIE.
		-  Contents of the FTIE, with the MIC field of the FTIE set to 0.
		-  Contents of the RIC-Request (if present)
		
	========================================================================
*/
VOID	FT_CalculateMIC(
	IN	PUINT8		sta_addr,	
	IN	PUINT8		ap_addr,
	IN	PUINT8		kck,
	IN	UINT8		seq,
	IN  PUINT8		rsnie,
	IN	UINT8		rsnie_len,
	IN	PUINT8		mdie,
	IN	UINT8		mdie_len,
	IN	PUINT8		ftie,
	IN	UINT8		ftie_len,
	IN	PUINT8		ric,
	IN	UINT8		ric_len,
	OUT PUINT8		mic)
{
    UCHAR   *OutBuffer;
	ULONG	FrameLen = 0;
	ULONG	TmpLen = 0;
	UINT	mlen = AES_KEY128_LENGTH;

	DBGPRINT(RT_DEBUG_TRACE, ("%s\n", __FUNCTION__));
	
	NdisZeroMemory(mic, sizeof(mic));
	
	/* allocate memory for MIC calculation */
	os_alloc_mem(NULL, (PUCHAR *)&OutBuffer, 512);
    if (OutBuffer == NULL)
    {
		DBGPRINT(RT_DEBUG_ERROR, ("!!!FT_CalculateMIC: no memory!!!\n"));
		return;
    }
		
	/* make a header frame for calculating MIC. */
    MakeOutgoingFrame(OutBuffer,            		&TmpLen,
                      MAC_ADDR_LEN,  				sta_addr,
                      MAC_ADDR_LEN,  				ap_addr,
                      1,							&seq,
                      END_OF_ARGS);
	FrameLen += TmpLen;					

	/* concatenate RSNIE */
	if (rsnie_len != 0)
	{				
	    MakeOutgoingFrame(OutBuffer + FrameLen,		&TmpLen,
	                      rsnie_len,	  			rsnie,
	                      END_OF_ARGS);
		FrameLen += TmpLen;					
	}
	
	/* concatenate MDIE */
	if (mdie_len != 0)
	{		
	    MakeOutgoingFrame(OutBuffer + FrameLen,		&TmpLen,
	                      mdie_len,	  				mdie,
	                      END_OF_ARGS);
		FrameLen += TmpLen;					
	}
	
	/* concatenate FTIE */
	if (ftie != 0)
	{
		/* The MIC field of the FTIE set to 0 */		
		NdisZeroMemory(ftie + 4, 16);
		
	    MakeOutgoingFrame(OutBuffer + FrameLen,		&TmpLen,
	                      ftie_len,	  				ftie,
	                      END_OF_ARGS);
		FrameLen += TmpLen;					
	}	
	
	/* concatenate RIC-Request/Response if present */
	if (ric != 0)
	{
		MakeOutgoingFrame(OutBuffer + FrameLen,     &TmpLen,
						  ric_len,					ric,
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
#if 0	
	hex_dump("OutBuffer", OutBuffer, FrameLen);
#endif
	
	/* Calculate MIC */				
	AES_CMAC(OutBuffer, FrameLen, kck, LEN_PTK_KCK, mic, &mlen);

	os_free_mem(NULL, OutBuffer);
}

#ifdef CONFIG_AP_SUPPORT
/*	
========================================================================
Routine Description:

Arguments:

Return Value:

Note:

========================================================================
*/
void FT_rtmp_read_parameters_from_file(
	IN PRTMP_ADAPTER pAd,
	RTMP_STRING *tmpbuf,
	RTMP_STRING *pBuffer)
{
	INT Loop;

	/* FtSupport */
	if(RTMPGetKeyParameter("FtSupport", tmpbuf, 32, pBuffer, TRUE))
	{
		RTMP_STRING *macptr;
	    for (Loop = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), Loop++)
	    {
			if (Loop >= MAX_MBSSID_NUM(pAd))
				break;

			if(simple_strtol(macptr, 0, 10) != 0)  /*Enable */
				pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.Dot11rFtEnable = TRUE;
			else /*Disable */
				pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.Dot11rFtEnable = FALSE;

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Dot11rFtEnable=%d\n",
				Loop, pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.Dot11rFtEnable));
		}
	}

	/* FtRic */
	if(RTMPGetKeyParameter("FtRic", tmpbuf, 32, pBuffer, TRUE))
	{
		RTMP_STRING *macptr;
	    for (Loop = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), Loop++)
	    {
			if (Loop >= MAX_MBSSID_NUM(pAd))
				break;

			if(simple_strtol(macptr, 0, 10) != 0)  /*Enable */
				pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.RsrReqCap = TRUE;
			else /*Disable */
				pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.RsrReqCap = FALSE;

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Dot11rFtRic=%d\n",
				Loop, pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.RsrReqCap));
	    }
	}

	/* FtOtd */
	if(RTMPGetKeyParameter("FtOtd", tmpbuf, 32, pBuffer, TRUE))
	{
		RTMP_STRING *macptr;
	    for (Loop = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), Loop++)
	    {
			if (Loop >= MAX_MBSSID_NUM(pAd))
				break;

			if(simple_strtol(macptr, 0, 10) != 0)  /*Enable */
				pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.FtOverDs = TRUE;
			else /*Disable */
				pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.FtOverDs = FALSE;

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Dot11rFtOtd=%d\n",
				Loop, pAd->ApCfg.MBSSID[Loop].FtCfg.FtCapFlag.FtOverDs));
	    }
	}

	for (Loop = 0; Loop < MAX_MBSSID_NUM(pAd); Loop++)
	{
		RTMP_STRING tok_str[16];

		/*
			FtMdId:
			FtMdId shall be a value of two octets.
		*/
		NdisZeroMemory(tok_str, sizeof(tok_str));
		snprintf(tok_str, sizeof(tok_str), "FtMdId%d", Loop + 1);
		if(RTMPGetKeyParameter(tok_str, tmpbuf, FT_MDID_LEN + 1, pBuffer, FALSE))
		{
			if (strlen(tmpbuf) == FT_MDID_LEN)
			{
				NdisMoveMemory(pAd->ApCfg.MBSSID[Loop].FtCfg.FtMdId, tmpbuf, FT_MDID_LEN);
				DBGPRINT(RT_DEBUG_TRACE, ("%s::FtMdid(%d)=%c%c\n", __FUNCTION__, Loop,
											pAd->ApCfg.MBSSID[Loop].FtCfg.FtMdId[0],
											pAd->ApCfg.MBSSID[Loop].FtCfg.FtMdId[1]));
			}
			else
			{
				DBGPRINT(RT_DEBUG_TRACE, ("%s: Invalid MdId=%s\n", __FUNCTION__,tmpbuf));
			}
		}

		/*
			FtR0khId:
			FtR0khId shall be in string of 1 ~ 48 octets.
		*/
		NdisZeroMemory(tok_str, sizeof(tok_str));
		snprintf(tok_str, sizeof(tok_str), "FtR0khId%d", Loop + 1);
		if(RTMPGetKeyParameter(tok_str, tmpbuf, FT_ROKH_ID_LEN + 1, pBuffer, FALSE))
		{
			if (strlen(tmpbuf) <= FT_ROKH_ID_LEN)
			{
				NdisMoveMemory(pAd->ApCfg.MBSSID[Loop].FtCfg.FtR0khId, tmpbuf, strlen(tmpbuf));
				pAd->ApCfg.MBSSID[Loop].FtCfg.FtR0khId[strlen(tmpbuf)] = '\0';
				DBGPRINT(RT_DEBUG_TRACE, ("%s::FtR0khId(%d)=%s\n", __FUNCTION__, Loop,
											pAd->ApCfg.MBSSID[Loop].FtCfg.FtR0khId));
			}
			else
			{
				DBGPRINT(RT_DEBUG_TRACE, ("%s: Invalid R0khId(%d)=%s Len=%d\n",
									__FUNCTION__, Loop, tmpbuf, strlen(tmpbuf)));
			}
		}
	}
}

INT Set_FT_Enable(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	INT apidx = pObj->ioctl_if;
	PFT_CFG pFtCfg;
	ULONG Value;

	Value = (UINT) simple_strtol(arg, 0, 10);

	if (apidx > pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid interface number (%d).\n",
			__FUNCTION__, apidx));
		return TRUE;
	}


	pFtCfg = &pAd->ApCfg.MBSSID[pObj->ioctl_if].FtCfg;
	pFtCfg->FtCapFlag.Dot11rFtEnable = (Value == 0 ? FALSE : TRUE);

	return TRUE;
}

INT Set_FT_Mdid(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	INT apidx = pObj->ioctl_if;
	PFT_CFG pFtCfg;

	if (apidx > pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid interface number (%d).\n",
			__FUNCTION__, apidx));
		return TRUE;
	}

	pFtCfg = &pAd->ApCfg.MBSSID[pObj->ioctl_if].FtCfg;
	NdisMoveMemory(pFtCfg->FtMdId, arg, FT_MDID_LEN);

	return TRUE;
}

INT Set_FT_R0khid(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	INT apidx = pObj->ioctl_if;
	PFT_CFG pFtCfg;

	if (apidx > pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid interface number (%d).\n",
			__FUNCTION__, apidx));
		return TRUE;
	}

	if (strlen(arg) > FT_ROKH_ID_LEN)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid R0KHID Length (%d).\n",
			__FUNCTION__, strlen(arg)));
		DBGPRINT(RT_DEBUG_ERROR, ("%s: The length shall be in range from 1 to 48 octects.\n",
			__FUNCTION__));
		return TRUE;
	}		

	pFtCfg = &pAd->ApCfg.MBSSID[pObj->ioctl_if].FtCfg;
	NdisMoveMemory(pFtCfg->FtR0khId, arg, strlen(arg));
	pFtCfg->FtR0khIdLen = strlen(arg);

	return TRUE;
}

INT Set_FT_RIC(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	INT apidx = pObj->ioctl_if;
	PFT_CFG pFtCfg;
	ULONG Value;

	Value = (UINT) simple_strtol(arg, 0, 10);

	if (apidx > pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid interface number (%d).\n",
			__FUNCTION__, apidx));
		return TRUE;
	}

	pFtCfg = &pAd->ApCfg.MBSSID[pObj->ioctl_if].FtCfg;
	pFtCfg->FtCapFlag.RsrReqCap = (Value == 0 ? FALSE : TRUE);

	return TRUE;
}

INT Set_FT_OTD(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	INT apidx = pObj->ioctl_if;
	PFT_CFG pFtCfg;
	ULONG Value;

	Value = (UINT) simple_strtol(arg, 0, 10);

	if (apidx > pAd->ApCfg.BssidNum)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Invalid interface number (%d).\n",
			__FUNCTION__, apidx));
		return TRUE;
	}

	pFtCfg = &pAd->ApCfg.MBSSID[pObj->ioctl_if].FtCfg;
	pFtCfg->FtCapFlag.FtOverDs = (Value == 0 ? FALSE : TRUE);

	return TRUE;
}

INT	Show_FTConfig_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;
	INT apidx = pObj->ioctl_if;
	PFT_CFG pFtCfg;

	if (apidx >= pAd->ApCfg.BssidNum)
		return -1;

	pFtCfg = &pAd->ApCfg.MBSSID[pObj->ioctl_if].FtCfg;

	DBGPRINT(RT_DEBUG_OFF, ("MDID=%c%c\n", pFtCfg->FtMdId[0], pFtCfg->FtMdId[1]));
	DBGPRINT(RT_DEBUG_OFF, ("R0KHID=%s, Len=%d\n", pFtCfg->FtR0khId, pFtCfg->FtR0khIdLen));
	DBGPRINT(RT_DEBUG_OFF, ("FT Enable=%d\n", pFtCfg->FtCapFlag.Dot11rFtEnable));
	DBGPRINT(RT_DEBUG_OFF, ("FT RIC=%d\n", pFtCfg->FtCapFlag.RsrReqCap));
	DBGPRINT(RT_DEBUG_OFF, ("FT OTD=%d\n", pFtCfg->FtCapFlag.FtOverDs));

	return TRUE;
}
#endif /* CONFIG_AP_SUPPORT */

#endif /* DOT11R_FT_SUPPORT */

