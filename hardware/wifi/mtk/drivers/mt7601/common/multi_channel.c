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
	multi_channel.c
 
    Abstract:
 
    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
 */

 
#include "rt_config.h"

#ifdef CONFIG_MULTI_CHANNEL


VOID RtmpPrepareHwNullFrame(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN BOOLEAN bQosNull,
	IN BOOLEAN bEOSP,
	IN UCHAR OldUP,
	IN UCHAR OpMode,
	IN UCHAR PwrMgmt,
	IN BOOLEAN bWaitACK,
	IN CHAR Index)
{
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	TXWI_STRUC *pTxWI;
	TXINFO_STRUC *pTxInfo;
	PUCHAR pNullFrame;
	PHEADER_802_11 pNullFr;
	UINT32 frameLen;
	UINT32 totalLen;
	UCHAR *ptr;
	UINT i;
	UINT32 longValue;
	UCHAR MlmeRate;
#ifdef P2P_SUPPORT
	PAPCLI_STRUCT pApCliEntry = NULL;
#endif /* P2P_SUPPORT */

#ifdef RT_BIG_ENDIAN
	NDIS_STATUS    NState;
	PUCHAR pNullFrBuf;
#endif /* RT_BIG_ENDIAN */


	NdisZeroMemory(pAd->NullFrBuf, sizeof(pAd->NullFrBuf));
	pTxWI = (TXWI_STRUC *)&pAd->NullFrBuf[0];
	pNullFrame = &pAd->NullFrBuf[TXWISize];

	pNullFr = (PHEADER_802_11) pNullFrame;
	frameLen = sizeof(HEADER_802_11);
	
	pNullFr->FC.Type = BTYPE_DATA;
	pNullFr->FC.SubType = SUBTYPE_NULL_FUNC;
	pNullFr->FC.ToDs = 1;
	pNullFr->FC.FrDs = 0;

	COPY_MAC_ADDR(pNullFr->Addr1, pEntry->Addr);
#ifdef P2P_SUPPORT
	if (IS_ENTRY_APCLI(pEntry))
	{
		pApCliEntry = &pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx];
		COPY_MAC_ADDR(pNullFr->Addr2, pApCliEntry->CurrentAddress);
		COPY_MAC_ADDR(pNullFr->Addr3, pApCliEntry->CfgApCliBssid);
	}
	else
#endif /* P2P_SUPPORT */
	{
		COPY_MAC_ADDR(pNullFr->Addr2, pAd->CurrentAddress);
		COPY_MAC_ADDR(pNullFr->Addr3, pAd->CommonCfg.Bssid);
	}

	pNullFr->FC.PwrMgmt = PwrMgmt;

	pNullFr->Duration = pAd->CommonCfg.Dsifs + RTMPCalcDuration(pAd, pAd->CommonCfg.TxRate, 14);

	/* sequence is increased in MlmeHardTx */
	pNullFr->Sequence = pAd->Sequence;
	pAd->Sequence = (pAd->Sequence+1) & MAXSEQ; /* next sequence  */

	if (bQosNull)
	{
		UCHAR *qos_p = ((UCHAR *)pNullFr) + frameLen;

		pNullFr->FC.SubType = SUBTYPE_QOS_NULL;

		/* copy QOS control bytes */
		qos_p[0] = ((bEOSP) ? (1 << 4) : 0) | OldUP;
		qos_p[1] = 0;
		frameLen += 2;
	} /* End of if */

	RTMPWriteTxWI(pAd, pTxWI,  FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, 0, pEntry->Aid, frameLen,
		0, 0, (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_HTTXOP, FALSE, &pAd->CommonCfg.MlmeTransmit);

	dumpTxWI(pAd, pTxWI);

	if (bWaitACK)
		pTxWI->TxWITXRPT = 1;

	hex_dump("RtmpPrepareHwNullFrame", pAd->NullFrBuf, TXWISize + frameLen);

	totalLen = TXWISize + frameLen;
	pAd->NullFrLen = totalLen;
	ptr = pAd->NullFrBuf;

#ifdef RT_BIG_ENDIAN
	NState = os_alloc_mem(pAd, (PUCHAR *) &pNullFrBuf, 100);
	if ( NState == NDIS_STATUS_FAILURE )
		return;

	NdisZeroMemory(pNullFrame, 100);
	NdisMoveMemory(pNullFrBuf, pAd->NullFrBuf, totalLen);
	RTMPWIEndianChange(pAd, pNullFrBuf, TYPE_TXWI);
	RTMPFrameEndianChange(pAd, (PUCHAR)pNullFrBuf + TXWISize, DIR_WRITE, FALSE);

	ptr = pNullFrBuf;
#endif /* RT_BIG_ENDIAN */


	for (i= 0; i< totalLen; i+=4)
	{
		longValue =  *ptr + (*(ptr + 1) << 8) + (*(ptr + 2) << 16) + (*(ptr + 3) << 24);
		//hex_dump("null frame before",&longValue, 4);

		if (Index == 0)
			RTMP_IO_WRITE32(pAd, pAd->NullBufOffset[0] + i, longValue);
		else if (Index == 1)
			RTMP_IO_WRITE32(pAd, pAd->NullBufOffset[1] + i, longValue);

		//RTMP_IO_WRITE32(pAd, 0xB700 + i, longValue);
		//RTMP_IO_WRITE32(pAd, 0xB780 + i, longValue);

		ptr += 4;
	}



}


VOID RTMPHwSendNullFrame(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR TxRate,
	IN BOOLEAN bQosNull,
	IN USHORT PwrMgmt,
	IN CHAR Index)
{

	UINT8 TXWISize = pAd->chipCap.TXWISize;
	NDIS_STATUS    NState;
	PHEADER_802_11 pNullFr;
	UCHAR *ptr;
	UINT32 longValue;
#ifdef RT_BIG_ENDIAN
	PUCHAR pNullFrame;
#endif /* RT_BIG_ENDIAN */


	DBGPRINT(RT_DEBUG_TRACE, ("%s - Send NULL Frame @%d Mbps...\n", __FUNCTION__, RateIdToMbps[pAd->CommonCfg.TxRate]));

	pNullFr = (PHEADER_802_11)((&pAd->NullFrBuf[0]) +TXWISize);

	pNullFr->FC.PwrMgmt = PwrMgmt;

	pNullFr->Duration = pAd->CommonCfg.Dsifs + RTMPCalcDuration(pAd, TxRate, 14);

	/* sequence is increased in MlmeHardTx */
	pNullFr->Sequence = pAd->Sequence;
	pAd->Sequence = (pAd->Sequence+1) & MAXSEQ; /* next sequence  */

	//hex_dump("RtmpPrepareHwNullFrame", pAd->NullFrBuf,  pAd->NullFrLen);

	if (Index == 0)
	{
		ptr = pAd->NullFrBuf + TXWISize;

#ifdef RT_BIG_ENDIAN
		longValue =  (*ptr << 8) + *(ptr + 1) + (*(ptr + 2) << 16) + (*(ptr + 3) << 24);
#else
		longValue =  *ptr + (*(ptr + 1) << 8) + (*(ptr + 2) << 16) + (*(ptr + 3) << 24);
#endif /* RT_BIG_ENDIAN */
		RTMP_IO_WRITE32(pAd, pAd->NullBufOffset[0] + TXWISize, longValue);

		ptr = pAd->NullFrBuf + TXWISize + 20;	// update Sequence
		longValue =  *ptr + (*(ptr + 1) << 8) + (*(ptr + 2) << 16) + (*(ptr + 3) << 24);
		RTMP_IO_WRITE32(pAd, pAd->NullBufOffset[0] + TXWISize + 20, longValue);
	}
	else if (Index == 1)
	{
		ptr = pAd->NullFrBuf + TXWISize;
#ifdef RT_BIG_ENDIAN
		longValue =  (*ptr << 8) + *(ptr + 1) + (*(ptr + 2) << 16) + (*(ptr + 3) << 24);
#else
		longValue =  *ptr + (*(ptr + 1) << 8) + (*(ptr + 2) << 16) + (*(ptr + 3) << 24);
#endif /* RT_BIG_ENDIAN */
		RTMP_IO_WRITE32(pAd, pAd->NullBufOffset[1] + TXWISize, longValue);

		ptr = pAd->NullFrBuf + TXWISize + 20;	// update Sequence
		longValue =  *ptr + (*(ptr + 1) << 8) + (*(ptr + 2) << 16) + (*(ptr + 3) << 24);
		RTMP_IO_WRITE32(pAd, pAd->NullBufOffset[1] + TXWISize + 20, longValue);
	}

	RTMP_IO_WRITE32(pAd, PBF_CTRL, 0x04);

}


#endif /* CONFIG_MULTI_CHANNEL */

