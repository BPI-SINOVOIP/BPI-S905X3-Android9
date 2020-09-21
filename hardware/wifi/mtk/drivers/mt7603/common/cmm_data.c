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
	cmm_data.c

    Abstract:

    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
 */


#include "rt_config.h"


UCHAR	SNAP_802_1H[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
UCHAR	SNAP_BRIDGE_TUNNEL[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8};
UCHAR	EAPOL[] = {0x88, 0x8e};
UCHAR   TPID[] = {0x81, 0x00}; /* VLAN related */

UCHAR	IPX[] = {0x81, 0x37};
UCHAR	APPLE_TALK[] = {0x80, 0xf3};


//  UserPriority To AccessCategory mapping
UCHAR WMM_UP2AC_MAP[8] = {QID_AC_BE, QID_AC_BK,
							QID_AC_BK, QID_AC_BE,
							QID_AC_VI, QID_AC_VI,
							QID_AC_VO, QID_AC_VO};

struct WCID_TABLE{
	DL_LIST list;
	UCHAR wcid;
};
static struct WCID_TABLE wcid_table[MAX_LEN_OF_TR_TABLE];



#ifdef DBG_DIAGNOSE
VOID dbg_diag_deque_log(RTMP_ADAPTER *pAd)
{
	struct dbg_diag_info *diag_info;
	UCHAR QueIdx = 0;

	diag_info = &pAd->DiagStruct.diag_info[pAd->DiagStruct.ArrayCurIdx];

#ifdef RTMP_MAC_PCI
#ifdef DBG_TX_RING_DEPTH
    if ((pAd->DiagStruct.diag_cond & DIAG_COND_TX_RING_DEPTH) != DIAG_COND_TX_RING_DEPTH)
    {
		for (QueIdx = 0; QueIdx < 4 /* 4 */; QueIdx++)
		{
			UINT free_cnt, desc_num;
			UINT32 dma_id, cpu_id, hw_cnt;

			free_cnt = GET_TXRING_FREENO(pAd, QueIdx);
#if 0
			desc_num = (TX_RING_SIZE - free_cnt); /* Number of occupied hw desc.*/
			desc_num = ((desc_num <=15) ? desc_num : 15);
			diag_info->TxDescCnt[desc_num]++;
#else
			RTMP_IO_READ32(pAd, pAd->TxRing[QueIdx].hw_didx_addr, &dma_id);
			RTMP_IO_READ32(pAd, pAd->TxRing[QueIdx].hw_cidx_addr, &cpu_id);
			if (dma_id > cpu_id)
				hw_cnt = TX_RING_SIZE - dma_id + cpu_id;
			else if (cpu_id > dma_id)
				hw_cnt  = cpu_id - dma_id;
			else
				hw_cnt = ((free_cnt > 0) ? 0 : TX_RING_SIZE);

			hw_cnt = ((hw_cnt <=15) ? hw_cnt : 15);
			diag_info->TxDescCnt[QueIdx][hw_cnt]++;
#endif
		}
    }
#endif /* DBG_TX_RING_DEPTH */

#ifdef MT_MAC
#ifdef DBG_PSE_DEPTH
    if ((pAd->DiagStruct.diag_cond & DIAG_COND_PSE_DEPTH) != DIAG_COND_PSE_DEPTH)
	{
		UINT32 mac_val;
		RTMP_IO_READ32(pAd, 0x8148, &mac_val);
		mac_val = ((mac_val & 0xfff) / 13);
		if (mac_val > 48)
			mac_val = 49;
		diag_info->pse_pg_cnt[mac_val]++;
	}
#endif /* DBG_PSE_DEPTH */
#endif /* MT_MAC */
#endif /* RTMP_MAC_PCI */

#ifdef DBG_TXQ_DEPTH
    if ((pAd->DiagStruct.diag_cond & DIAG_COND_TXQ_DEPTH) != DIAG_COND_TXQ_DEPTH)
    {
		if ((pAd->DiagStruct.wcid > 0) &&  (pAd->DiagStruct.wcid < MAX_LEN_OF_TR_TABLE)) {
			STA_TR_ENTRY *tr_entry = &pAd->MacTab.tr_entry[pAd->DiagStruct.wcid];
			UCHAR swq_cnt;

			for (QueIdx = 0; QueIdx < 4; QueIdx++)
			{
				if (tr_entry->tx_queue[QueIdx].Number <= 7)
					swq_cnt = tr_entry->tx_queue[QueIdx].Number;
				else
					swq_cnt = 8;
				diag_info->TxSWQueCnt[QueIdx][swq_cnt]++;
			}
		}
    }
#endif /* DBG_TXQ_DEPTH */
}
#endif /* DBG_DIAGNOSE */

/*Nobody uses it currently*/
#if 0
static VOID dump_txblk(TX_BLK *pTxBlk)
{
	NDIS_PACKET *pPacket;
	int i, frameNum;
	PQUEUE_ENTRY	pQEntry;

	DBGPRINT(RT_DEBUG_TRACE,("Dump TX_BLK Structure:\n"));
	DBGPRINT(RT_DEBUG_TRACE,("\tTxFrameType=%d!\n", pTxBlk->TxFrameType));
	DBGPRINT(RT_DEBUG_TRACE,("\tTotalFrameLen=%d\n", pTxBlk->TotalFrameLen));
	DBGPRINT(RT_DEBUG_TRACE,("\tTotalFrameNum=%d!\n", pTxBlk->TxPacketList.Number));
	DBGPRINT(RT_DEBUG_TRACE,("\tTotalFragNum=%d!\n", pTxBlk->TotalFragNum));
	DBGPRINT(RT_DEBUG_TRACE,("\tpPacketList=\n"));

	frameNum = pTxBlk->TxPacketList.Number;

	for(i=0; i < frameNum; i++)
	{	int j;
		UCHAR	*pBuf;

		pQEntry = RemoveHeadQueue(&pTxBlk->TxPacketList);
		pPacket = QUEUE_ENTRY_TO_PACKET(pQEntry);
		if (pPacket)
		{
			pBuf = GET_OS_PKT_DATAPTR(pPacket);
			DBGPRINT(RT_DEBUG_TRACE,("\t\t[%d]:ptr=0x%x, Len=%d!\n", i, (UINT32)(GET_OS_PKT_DATAPTR(pPacket)), GET_OS_PKT_LEN(pPacket)));
			DBGPRINT(RT_DEBUG_TRACE,("\t\t"));
			for (j =0 ; j < GET_OS_PKT_LEN(pPacket); j++)
			{
				DBGPRINT(RT_DEBUG_TRACE,("%02x ", (pBuf[j] & 0xff)));
				if (j == 16)
					break;
			}
			InsertTailQueue(&pTxBlk->TxPacketList, PACKET_TO_QUEUE_ENTRY(pPacket));
		}
	}
	DBGPRINT(RT_DEBUG_TRACE,("\tWcid=%d!\n", pTxBlk->Wcid));
	DBGPRINT(RT_DEBUG_TRACE,("\twdev_idx=%d!\n", pTxBlk->wdev_idx));
	DBGPRINT(RT_DEBUG_TRACE,("----EndOfDump\n"));

}
#endif

VOID dump_rxblk(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk)
{
	DBGPRINT(RT_DEBUG_TRACE,("Dump RX_BLK Structure:\n"));

	DBGPRINT(RT_DEBUG_TRACE,("\tHW rx info:\n"));
	hex_dump("RawData", &pRxBlk->hw_rx_info[0], RXD_SIZE);

	DBGPRINT(RT_DEBUG_TRACE,("\tData Pointer info:\n"));

#if defined(RTMP_MAC) || defined(RLT_MAC)
	if (pAd->chipCap.hif_type == HIF_RTMP || pAd->chipCap.hif_type == HIF_RLT) {
		DBGPRINT(RT_DEBUG_TRACE,("\t\tpRxInfo=0x%p\n", pRxBlk->pRxInfo));
		dump_rxinfo(pAd, pRxBlk->pRxInfo);
#ifdef RLT_MAC
		if (pAd->chipCap.hif_type == HIF_RLT) {
			DBGPRINT(RT_DEBUG_TRACE,("\t\tpRxFceInfo=0x%p\n", pRxBlk->pRxFceInfo));
			dumpRxFCEInfo(pAd, pRxBlk->pRxFceInfo);
		}
#endif /* RLT_MAC */
		DBGPRINT(RT_DEBUG_TRACE,("\t\tpRxWI=0x%p\n", pRxBlk->pRxWI));
		dump_rmac_info(pAd, (UCHAR *)pRxBlk->pRxWI);
	}
#endif /* defined(RTMP_MAC) || defined(RLT_MAC) */

#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT) {
		DBGPRINT(RT_DEBUG_TRACE,("\t\trmac_info=0x%p\n", pRxBlk->rmac_info));
		dump_rmac_info(pAd, pRxBlk->rmac_info);
	}
#endif /* MT_MAC */

	DBGPRINT(RT_DEBUG_TRACE,("\t\tpRxPacket=0x%p, MPDUtotalByteCnt=%d\n", pRxBlk->pRxPacket, pRxBlk->MPDUtotalByteCnt));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tpHeader=0x%p\n", pRxBlk->pHeader));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tpData=0x%p\n", pRxBlk->pData));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tDataSize=%d\n", pRxBlk->DataSize));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tFlags=0x%x\n", pRxBlk->Flags));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tUserPriority=%d\n", pRxBlk->UserPriority));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tOpMode=%d\n", pRxBlk->OpMode));

	DBGPRINT(RT_DEBUG_TRACE,("\tMirror Info from RMAC Info:\n"));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tWCID=%d\n", pRxBlk->wcid));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tTID=%d\n", pRxBlk->TID));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tKey_idx=%d\n", pRxBlk->key_idx));
	DBGPRINT(RT_DEBUG_TRACE,("\t\tBSS_IDX=%d\n", pRxBlk->bss_idx));
#if defined(RLT_MAC) || defined(RTMP_MAC)
	if (pAd->chipCap.hif_type == HIF_RTMP || pAd->chipCap.hif_type == HIF_RLT) {
		DBGPRINT(RT_DEBUG_TRACE,("\t\tPhyMode=%d(%s)\n",
					pRxBlk->rx_rate.field.MODE,
					get_phymode_str(pRxBlk->rx_rate.field.MODE)));
		DBGPRINT(RT_DEBUG_TRACE,("\t\tMCS=%d\n", pRxBlk->rx_rate.field.MCS));
		DBGPRINT(RT_DEBUG_TRACE,("\t\tBW=%d\n", pRxBlk->rx_rate.field.BW));
		DBGPRINT(RT_DEBUG_TRACE,("\t\tSGI=%d\n", pRxBlk->rx_rate.field.ShortGI));
		DBGPRINT(RT_DEBUG_TRACE,("\t\tSTBC=%d\n", pRxBlk->rx_rate.field.STBC));
#ifdef RLT_MAC
		if (pAd->chipCap.hif_type == HIF_RLT)
			DBGPRINT(RT_DEBUG_TRACE,("\t\tLDPC=%d\n", pRxBlk->rx_rate.field.ldpc));
#endif /* RLT_MAC */
		DBGPRINT(RT_DEBUG_TRACE,("\t\tRSSI=%d:%d:%d\n",
						pRxBlk->rx_signal.raw_rssi[0], pRxBlk->rx_signal.raw_rssi[1],
						pRxBlk->rx_signal.raw_rssi[2]));
		DBGPRINT(RT_DEBUG_TRACE,("\t\tSNR=%d:%d:%d\n",
						pRxBlk->rx_signal.raw_snr[0], pRxBlk->rx_signal.raw_snr[1],
						pRxBlk->rx_signal.raw_snr[2]));
		DBGPRINT(RT_DEBUG_TRACE,("\t\tFreqOffset=%d\n",
						pRxBlk->rx_signal.freq_offset));
	}
#endif /* defined(RLT_MAC) || defined(RTMP_MAC) */

	//hex_dump("Dump RxPacket in dump_rxblk", (UCHAR *)pRxBlk->pHeader, pRxBlk->MPDUtotalByteCnt > 512 ? 512 : pRxBlk->MPDUtotalByteCnt);
}


#ifdef MT_MAC
/*
    1'b0: the related GROUP is not present
    1'b1: the related GROUP is present

    bit[0]: indicates GROUP1 (DW8~DW11)
    bit[1]: indicates GROUP2 (DW12~DW13)
    bit[2]: indicates GROUP3 (DW14~DW19)
    bit[3]: indicates GROUP4 (DW4~DW7)
*/
//#define RMAC_INFO_GRP_1_SIZE    16
//#define RMAC_INFO_GRP_2_SIZE    8
//#define RMAC_INFO_GRP_3_SIZE    24
//#define RMAC_INFO_GRP_4_SIZE    16

static INT32 RMACInfoGrpToLen[]={
        /* 0: base only */
        RMAC_INFO_BASE_SIZE,
        /* 1: [bit 0] base + group 1 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE,
        /* 2: [bit 1] base + group 2 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_2_SIZE,
        /* 3: [bit 0 + bit 1] base + group 1 + group 2 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE + RMAC_INFO_GRP_2_SIZE,
        /* 4: [bit 2] base + group 3 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_3_SIZE,
        /* 5: [bit 0 + bit 2] base + group 1 + group 3 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE + RMAC_INFO_GRP_3_SIZE,
        /* 6: [bit 1 + bit 2] base + group 2 + group 3 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_2_SIZE + RMAC_INFO_GRP_3_SIZE,
        /* 7: [bit 0 + bit 1 + bit 2] base + group 1 + group 2 + group 3 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE + RMAC_INFO_GRP_2_SIZE + RMAC_INFO_GRP_3_SIZE,
        /* 8: [bit 3 ] base + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_4_SIZE,
        /* 9: [bit 0 + bit 3 ] base + group 1 + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE + RMAC_INFO_GRP_4_SIZE,
        /* 10: [bit 1 + bit 3 ] base + group 2 + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_2_SIZE + RMAC_INFO_GRP_4_SIZE,
        /* 11: [bit 0 + bit 1 + bit 3 ] base + group 1 + group 2 + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE + RMAC_INFO_GRP_2_SIZE + RMAC_INFO_GRP_4_SIZE,
        /* 12: [bit 2 + bit 3 ] base + group 3 + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_3_SIZE + RMAC_INFO_GRP_4_SIZE,
        /* 13: [bit 0 + bit 2 + bit 3 ] base + group 1 + group 3 + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE + RMAC_INFO_GRP_3_SIZE + RMAC_INFO_GRP_4_SIZE,
        /* 14: [bit 1 + bit 2 + bit 3 ] base + group 2 + group 3 + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_2_SIZE + RMAC_INFO_GRP_3_SIZE + RMAC_INFO_GRP_4_SIZE,
        /* 15: [bit 0 + bit 1 + bit 2 + bit 3 ] base + group 1 + group 2 + group 3 + group 4 */
        RMAC_INFO_BASE_SIZE + RMAC_INFO_GRP_1_SIZE + RMAC_INFO_GRP_2_SIZE + RMAC_INFO_GRP_3_SIZE + RMAC_INFO_GRP_4_SIZE,
};


#ifdef MT_MAC
VOID ParseRxVPacket(RTMP_ADAPTER *pAd, UINT32 Type, RX_BLK *RxBlk, UCHAR *Data)
{
	//RXV_DWORD0 *DW0 = NULL;
	RXV_DWORD1 *DW1 = NULL;
	RX_VECTOR1_1ST_CYCLE *RXV1_1ST_CYCLE = NULL;
	//RX_VECTOR1_2ND_CYCLE *RXV1_2ND_CYCLE = NULL;
#ifdef CONFIG_QA
	RX_VECTOR1_3TH_CYCLE *RXV1_3TH_CYCLE = NULL;
#endif  /* CONFIG_QA */
	RX_VECTOR1_4TH_CYCLE *RXV1_4TH_CYCLE = NULL;
	RX_VECTOR1_5TH_CYCLE *RXV1_5TH_CYCLE = NULL;
	RX_VECTOR1_6TH_CYCLE *RXV1_6TH_CYCLE = NULL;
	RX_VECTOR2_1ST_CYCLE *RXV2_1ST_CYCLE = NULL;
	RX_VECTOR2_2ND_CYCLE *RXV2_2ND_CYCLE = NULL;
	RX_VECTOR2_3TH_CYCLE *RXV2_3TH_CYCLE = NULL;

	if (Type == RMAC_RX_PKT_TYPE_RX_NORMAL)
	{
		RXV1_1ST_CYCLE = (RX_VECTOR1_1ST_CYCLE *)Data;
		//RXV1_2ND_CYCLE = (RX_VECTOR1_2ND_CYCLE *)(Data + 4);
#ifdef CONFIG_QA
		RXV1_3TH_CYCLE = (RX_VECTOR1_3TH_CYCLE *)(Data + 8);
#endif  /* CONFIG_QA */
		RXV1_4TH_CYCLE = (RX_VECTOR1_4TH_CYCLE *)(Data + 12);
		RXV1_5TH_CYCLE = (RX_VECTOR1_5TH_CYCLE *)(Data + 16);
		RXV1_6TH_CYCLE = (RX_VECTOR1_6TH_CYCLE *)(Data + 20);
		RXV2_2ND_CYCLE = (RX_VECTOR2_2ND_CYCLE *)(Data + 28);
	}
	else if (Type == RMAC_RX_PKT_TYPE_RX_TXRXV)
	{
		//DW0 = (RXV_DWORD0 *)Data;
		DW1 = (RXV_DWORD1 *)(Data + 4);
		RXV1_1ST_CYCLE = (RX_VECTOR1_1ST_CYCLE *)(Data + 8);
		//RXV1_2ND_CYCLE = (RX_VECTOR1_2ND_CYCLE *)(Data + 12);
#ifdef CONFIG_QA
		RXV1_3TH_CYCLE = (RX_VECTOR1_3TH_CYCLE *)(Data + 16);
#endif  /* CONFIG_QA */
		RXV1_4TH_CYCLE = (RX_VECTOR1_4TH_CYCLE *)(Data + 20);
		RXV1_5TH_CYCLE = (RX_VECTOR1_5TH_CYCLE *)(Data + 24);

		RXV1_6TH_CYCLE = (RX_VECTOR1_6TH_CYCLE *)(Data + 28);
		RXV2_1ST_CYCLE = (RX_VECTOR2_1ST_CYCLE *)(Data + 32);
		RXV2_2ND_CYCLE = (RX_VECTOR2_2ND_CYCLE *)(Data + 36);
		RXV2_3TH_CYCLE = (RX_VECTOR2_3TH_CYCLE *)(Data + 40);

        RxBlk->rxv2_cyc1 = *(UINT32 *)RXV2_1ST_CYCLE;
        RxBlk->rxv2_cyc2 = *(UINT32 *)RXV2_2ND_CYCLE;
        RxBlk->rxv2_cyc3 = *(UINT32 *)RXV2_3TH_CYCLE;
        pAd->rxv2_cyc3[(DW1->RxvSn % 10)] = RxBlk->rxv2_cyc3;
#ifdef CONFIG_QA
        pAd->ATECtrl.RCPI0 = RXV1_3TH_CYCLE->Rcpi0;
        pAd->ATECtrl.RCPI1 = RXV1_3TH_CYCLE->Rcpi1;
		if (RXV1_5TH_CYCLE) {
			pAd->ATECtrl.FreqOffsetFromRx = RXV1_5TH_CYCLE->FoE;
			pAd->ATECtrl.SNR0 = RXV1_5TH_CYCLE->LTF_SNR0;
		}
		if (RXV2_2ND_CYCLE)
			pAd->ATECtrl.SNR1 = RXV2_2ND_CYCLE->OfdmLtfSNR1;
	pAd->ATECtrl.RSSI0 = RXV1_3TH_CYCLE->Rcpi0/2 - 110;
	pAd->ATECtrl.RSSI1 = RXV1_3TH_CYCLE->Rcpi1/2 - 110;
#endif /* CONFIG_QA */
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,("%s(): invalid Type %u\n",__FUNCTION__,Type));
		return; /* return here to avoid dereferencing NULL pointer below */
	}

#ifdef RELEASE_EXCLUDE
    /*
        MT7603, MT7628, MT7636 E1 has hardware bug.
        BBP not report BW20 CCK RCPI.
        so,driver must use inband RSSI.
    */
#endif /* RELEASE_EXCLUDE */
    //RxBlk->rx_signal.raw_rssi[0] = (RXV1_3TH_CYCLE->Rcpi0 - 220) / 2;
    //RxBlk->rx_signal.raw_rssi[1] = (RXV1_3TH_CYCLE->Rcpi1 - 220) / 2;
    RxBlk->rx_signal.raw_rssi[0] = (CHAR)RXV1_4TH_CYCLE->IBRssi0;
    RxBlk->rx_signal.raw_rssi[1] = (CHAR)RXV1_4TH_CYCLE->IBRssi1;
	if (RXV1_5TH_CYCLE)
		RxBlk->rx_signal.raw_snr[0] = RXV1_5TH_CYCLE->LTF_SNR0;
	if (RXV2_2ND_CYCLE)
		RxBlk->rx_signal.raw_snr[1] = RXV2_2ND_CYCLE->OfdmLtfSNR1;

	if (RXV1_6TH_CYCLE) {
#ifdef CONFIG_AP_SUPPORT
		pAd->ApCfg.RssiSample.LastNoiseLevel[0] = ((RXV1_6TH_CYCLE->Nf0 - 254)/2);
		pAd->ApCfg.RssiSample.LastNoiseLevel[1] = ((RXV1_6TH_CYCLE->Nf1 - 254)/2);
#endif
#ifdef CONFIG_STA_SUPPORT
		pAd->StaCfg.RssiSample.LastNoiseLevel[0] = ((RXV1_6TH_CYCLE->Nf0 - 254)/2);
		pAd->StaCfg.RssiSample.LastNoiseLevel[1] = ((RXV1_6TH_CYCLE->Nf1 - 254)/2);
#endif
	}

    //RxBlk->rx_signal.raw_snr[0] = rxwi_n->bbp_rxinfo[0];
    //RxBlk->rx_signal.raw_snr[1] = rxwi_n->bbp_rxinfo[1];
    //RxBlk->rx_signal.freq_offset = rxwi_n->bbp_rxinfo[4];

	RxBlk->rx_rate.field.MODE = (USHORT)RXV1_1ST_CYCLE->TxMode;
	RxBlk->rx_rate.field.MCS = (USHORT)RXV1_1ST_CYCLE->TxRate;
	RxBlk->rx_rate.field.ldpc = (USHORT)RXV1_1ST_CYCLE->HtAdCode;
	RxBlk->rx_rate.field.BW = (USHORT)RXV1_1ST_CYCLE->FrMode;
	RxBlk->rx_rate.field.STBC = (USHORT)RXV1_1ST_CYCLE->HtStbc;
	RxBlk->rx_rate.field.ShortGI = (USHORT)RXV1_1ST_CYCLE->HtShortGi;
}


static inline INT32 mt_rx_info_2_blk(
								RTMP_ADAPTER *pAd,
								RX_BLK *pRxBlk,
								PNDIS_PACKET pRxPacket,
								INT32 pkt_type)
{
    UCHAR *RMACInfo, *Pos;
    INT32 RMACInfoLen;
    struct rxd_base_struc *rx_base;
	//RXD_GRP4_STRUCT *RxdGrp4 = NULL;
	//RXD_GRP1_STRUCT *RxdGrp1 = NULL;
	RXD_GRP2_STRUCT *RxdGrp2 = NULL;
	RXD_GRP3_STRUCT *RxdGrp3 = NULL;

    pRxBlk->pRxInfo = (RXINFO_STRUC *)(&pRxBlk->hw_rx_info[RXINFO_OFFSET]);
	RMACInfo = (UCHAR *)(GET_OS_PKT_DATAPTR(pRxPacket));
	Pos = RMACInfo;
    pRxBlk->rmac_info = RMACInfo;
    rx_base = (struct rxd_base_struc *)RMACInfo;

	Pos += RMAC_INFO_BASE_SIZE;

	if (rx_base->rxd_0.grp_vld & RXS_GROUP4)
	{
		//RxdGrp4 = (RXD_GRP4_STRUCT *)Pos;
		Pos += RMAC_INFO_GRP_4_SIZE;
	}

	if (rx_base->rxd_0.grp_vld & RXS_GROUP1)
	{
		//RxdGrp1 = (RXD_GRP1_STRUCT *)Pos;
		Pos += RMAC_INFO_GRP_1_SIZE;
	}

	if (rx_base->rxd_0.grp_vld & RXS_GROUP2)
	{
		RxdGrp2 = (RXD_GRP2_STRUCT *)Pos;
		Pos += RMAC_INFO_GRP_2_SIZE;
	}

	if (rx_base->rxd_0.grp_vld & RXS_GROUP3)
	{
		RxdGrp3 = (RXD_GRP3_STRUCT *)Pos;
		Pos += RMAC_INFO_GRP_3_SIZE;
	}

	if(NULL == RxdGrp2)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Error RxdGrp2 is null pointer!!!!\n", __FUNCTION__));
		return INVALID_RX_PACKET_LEN;
	}

    RMACInfoLen = RMACInfoGrpToLen[rx_base->rxd_0.grp_vld];

#ifdef RT_BIG_ENDIAN
	if ((RMACInfoLen - 4) > 0)
	{
		MTMacInfoEndianChange(pAd, RMACInfo, TYPE_RMACINFO, RMACInfoLen);
    }
#endif /* RT_BIG_ENDIAN */

    //dump_rmac_info(pAd, RMACInfo);

	pRxBlk->MPDUtotalByteCnt = (USHORT)(rx_base->rxd_0.rx_byte_cnt - RMACInfoLen);

	if (rx_base->rxd_1.hdr_offset == 1) {
        pRxBlk->MPDUtotalByteCnt -= 2;
        RMACInfoLen += 2;
    }

    pRxBlk->DataSize = pRxBlk->MPDUtotalByteCnt;
	pRxBlk->wcid = (UCHAR)rx_base->rxd_2.wlan_idx;
	pRxBlk->bss_idx = (UCHAR)rx_base->rxd_1.bssid;
	pRxBlk->key_idx = (UCHAR)rx_base->rxd_1.key_id;
	pRxBlk->TID = (UCHAR)rx_base->rxd_2.tid;
    pRxBlk->TimeStamp = RxdGrp2->timestamp;

    pRxBlk->pRxInfo->U2M = rx_base->rxd_1.u2m;
    pRxBlk->pRxInfo->Mcast = rx_base->rxd_1.mcast;
    pRxBlk->pRxInfo->Bcast = rx_base->rxd_1.bcast;
    pRxBlk->pRxInfo->FRAG = rx_base->rxd_2.frag;
    pRxBlk->pRxInfo->NULLDATA = rx_base->rxd_2.null_frm;
    pRxBlk->pRxInfo->DATA = !(rx_base->rxd_2.ndata);
    pRxBlk->pRxInfo->HTC = rx_base->rxd_1.htc_vld;
    pRxBlk->pRxInfo->AMPDU = !(rx_base->rxd_2.non_ampdu);
    pRxBlk->pRxInfo->L2PAD = 0;
    pRxBlk->pRxInfo->AMSDU = 0; // TODO:
    pRxBlk->pRxInfo->CipherErr = rx_base->rxd_2.icv_err | (rx_base->rxd_2.tkip_mic_err << 1);/* 0: decryption okay, 1:ICV error, 2:MIC error, 3:KEY not valid */
    pRxBlk->pRxInfo->Crc = rx_base->rxd_2.fcs_err;
    pRxBlk->pRxInfo->MyBss = ((rx_base->rxd_1.bssid == 0xf) ? 0 : 1);
    pRxBlk->pRxInfo->BA = !(rx_base->rxd_2.non_ampdu);
    pRxBlk->pRxInfo->Decrypted = 0; // TODO:

	if (rx_base->rxd_0.grp_vld & RXS_GROUP3)
		ParseRxVPacket(pAd, RMAC_RX_PKT_TYPE_RX_NORMAL, pRxBlk, (UCHAR *)RxdGrp3);

    SET_OS_PKT_DATAPTR(pRxPacket, GET_OS_PKT_DATAPTR(pRxPacket) + RMACInfoLen);
    SET_OS_PKT_LEN(pRxPacket, pRxBlk->MPDUtotalByteCnt);

//+++Add by shiang for work-around
#if 0
    // TODO: shiang-MT7603, remove me after hw issue fixed!
    pRxBlk->MPDUtotalByteCnt -= 4;
    pRxBlk->DataSize = pRxBlk->MPDUtotalByteCnt;
    SET_OS_PKT_DATAPTR(pRxPacket, GET_OS_PKT_DATAPTR(pRxPacket) + 4);
    SET_OS_PKT_LEN(pRxPacket, pRxBlk->MPDUtotalByteCnt);
DBGPRINT(RT_DEBUG_OFF, ("%s(): After work-around, DataPtr=%p, DataLen=%d\n",
            __FUNCTION__, GET_OS_PKT_DATAPTR(pRxPacket), GET_OS_PKT_LEN(pRxPacket)));
#endif
//---Add by shiang for work-around

    return RMACInfoLen;
}
#endif /* MT_MAC */


#define NUM_TYPE_STRING 8
UCHAR *rx_pkt_type_string [NUM_TYPE_STRING] = {
    "RMAC_RX_PKT_TYPE_RX_TXS", "RMAC_RX_PKT_TYPE_RX_TXRXV", "RMAC_RX_PKT_TYPE_RX_NORMAL",
    "RMAC_RX_PKT_TYPE_RX_DUP_RFB", "RMAC_RX_PKT_TYPE_RX_TMR", "Undefine Type 0x5",
    "Undefine Type 0x6", "RMAC_RX_PKT_TYPE_RX_EVENT"
};


UINT32 parse_rx_packet_type(RTMP_ADAPTER *ad, RX_BLK *rx_blk, VOID *rx_packet)
{
    UINT32 rx_hw_hdr_len = 0;
    UINT32 rx_pkt_type;
    union rmac_rxd_0 *rxd_0;

    rxd_0 = (union rmac_rxd_0 *)(GET_OS_PKT_DATAPTR(rx_packet));

#ifdef RT_BIG_ENDIAN
    mt_rmac_d0_endian_change(&rxd_0->word);
#endif /* RT_BIG_ENDIAN */

#ifdef RTMP_PCI_SUPPORT
	if (RMAC_RX_PKT_RX_BYTE_COUNT(rxd_0->word) > RX_BUFFER_AGGRESIZE)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("drop rx length = 0x%x packet\n", 
				RMAC_RX_PKT_RX_BYTE_COUNT(rxd_0->word)));
		return 0;
	}
#endif
	
	rx_pkt_type = RMAC_RX_PKT_TYPE(rxd_0->word);
    
	switch (rx_pkt_type) {
        case RMAC_RX_PKT_TYPE_RX_NORMAL:
        case RMAC_RX_PKT_TYPE_RX_DUP_RFB:
#ifdef MT_MAC
            rx_hw_hdr_len = mt_rx_info_2_blk(ad, rx_blk, rx_packet, rx_pkt_type);
#endif /* MT_MAC */
            break;

        case RMAC_RX_PKT_TYPE_RX_TXRXV:
            {
                RMAC_RXD_0_TXRXV *rxv = (RMAC_RXD_0_TXRXV *)(GET_OS_PKT_DATAPTR(rx_packet));
                UCHAR *ptr;
                INT idx;

                DBGPRINT(RT_DEBUG_LOUD, ("RxV Report: Number=%d, ByteCnt=%d\n",
                            rxv->rxv_cnt, rxv->rx_byte_cnt));

                if (rxv->rx_byte_cnt != (rxv->rxv_cnt * 44 + 4))
                {
                    DBGPRINT(RT_DEBUG_ERROR, ("ReceivedByteCnt not equal rxv_entry required!\n"));
                }
                else
                {
                    ptr = (UCHAR *)(GET_OS_PKT_DATAPTR(rx_packet) + 4);

#ifdef RT_BIG_ENDIAN
					if ((rxv->rx_byte_cnt - 4) > 0)
					{
						MTMacInfoEndianChange(ad, ptr,
							TYPE_RMACINFO, rxv->rx_byte_cnt);
    				}
#endif /* RT_BIG_ENDIAN */

                    for (idx = 0; idx < rxv->rxv_cnt; idx++)
                    {
						ParseRxVPacket(ad, RMAC_RX_PKT_TYPE_RX_TXRXV, rx_blk, ptr);
                        ptr += 44;
                    }
                }
            }
            break;

        case RMAC_RX_PKT_TYPE_RX_TXS:
            {
                RMAC_RXD_0_TXS *txs = (RMAC_RXD_0_TXS *)(GET_OS_PKT_DATAPTR(rx_packet));
                UCHAR *ptr;
                INT idx;

              	DBGPRINT(RT_DEBUG_INFO, ("TxS Report: Number=%d, ByteCnt=%d\n",
                            txs->txs_cnt, txs->rx_byte_cnt));
                if (txs->rx_byte_cnt != (txs->txs_cnt * 20 + 4))
                {
                    DBGPRINT(RT_DEBUG_ERROR, ("ReceivedByteCnt not equal txs_entry required!\n"));
                }
                else
                {
                    ptr = (UCHAR *)(GET_OS_PKT_DATAPTR(rx_packet) + 4);
                    for (idx = 0; idx < txs->txs_cnt; idx++)
                    {
                        TXS_STRUC *txs_entry = (TXS_STRUC *)ptr;
                        TXS_D_0 *txs_d0 = &txs_entry->txs_d0;
#ifdef RT_BIG_ENDIAN
                        TXS_D_1 *txs_d1 = &txs_entry->txs_d1;
                        TXS_D_2 *txs_d2 = &txs_entry->txs_d2;
                        TXS_D_3 *txs_d3 = &txs_entry->txs_d3;
#endif
                        TXS_D_4 *txs_d4 = &txs_entry->txs_d4;
#ifdef RT_BIG_ENDIAN
						*(((UINT32 *)txs_d0)) = SWAP32(*(((UINT32 *)txs_d0)));
						*(((UINT32 *)txs_d1)) = SWAP32(*(((UINT32 *)txs_d1)));
						*(((UINT32 *)txs_d2)) = SWAP32(*(((UINT32 *)txs_d2)));
						*(((UINT32 *)txs_d3)) = SWAP32(*(((UINT32 *)txs_d3)));
						*(((UINT32 *)txs_d4)) = SWAP32(*(((UINT32 *)txs_d4)));
#endif
			ParseTxSPacket(ad, txs_d4->pid, (UINT8)txs_d0->txsfm, ptr);

                        ptr += 20;
                    }
                }
            }
            break;

        case RMAC_RX_PKT_TYPE_RX_EVENT:
         {
            if (ad->chipOps.rx_event_handler != NULL)
                ad->chipOps.rx_event_handler(ad, GET_OS_PKT_DATAPTR(rx_packet));

			if (rx_blk)
				RX_BLK_SET_FLAG(rx_blk, fRX_CMD_RSP);
         }
            break;
#ifdef MT_PS
		case RMAC_RX_PKT_TYPE_RETRIEVE:
			{
			struct rxd_base_struc *rx_base;
			TMAC_TXD_L *txd_l = (TMAC_TXD_L *)(GET_OS_PKT_DATAPTR(rx_packet));
            TMAC_TXD_1 *txd_1 = &txd_l->txd_1;
#ifdef RT_BIG_ENDIAN
		*(((UINT32 *)txd_1)) = SWAP32(*(((UINT32 *)txd_1)));
#endif /* RT_BIG_ENDIAN */

            UCHAR padding = txd_1->hdr_pad & 0x03;
            UCHAR hdr_info = txd_1->hdr_info*2;
            UCHAR *da, *sa;
            STA_TR_ENTRY *tr_entry;
			MAC_TABLE_ENTRY *pEntry;
         	unsigned long IrqFlags;
            UINT32 q_idx = QID_AC_BE;
            HEADER_802_11 *pWifi_hdr;
            UCHAR *qos_p;
#ifdef UAPSD_SUPPORT
			UCHAR ac_idx = 0;
#endif /* UAPSD_SUPPORT */

            tr_entry = &ad->MacTab.tr_entry[txd_1->wlan_idx];
			pEntry = &ad->MacTab.Content[txd_1->wlan_idx];
            rx_base = (struct rxd_base_struc *)rxd_0;
            pWifi_hdr = (HEADER_802_11 *)(GET_OS_PKT_DATAPTR(rx_packet) + sizeof(TMAC_TXD_L));

#ifdef RT_BIG_ENDIAN
			RTMPFrameEndianChange(ad, (PUCHAR)pWifi_hdr, DIR_READ, FALSE);
#endif

            DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_PS, ("parse_rx_packet_type wlan_idx=%d,%d,%d,0x%x\n", txd_1->wlan_idx,hdr_info,padding,tr_entry->EntryType));
            
			if ((pWifi_hdr->FC.Type == FC_TYPE_CNTL) || 
				(pWifi_hdr->FC.Type == FC_TYPE_MGMT))
			{
				return 0;
			}

			da = pWifi_hdr->Addr1;
            sa = pWifi_hdr->Addr2;
            qos_p = ((UCHAR *)pWifi_hdr) + sizeof(HEADER_802_11);

#ifdef UAPSD_SUPPORT
			/*
				Sanity Check for UAPSD condition for correct QoS index.
			*/
			if (qos_p[0] >= 8)
				qos_p[0] = 1; /* shout not be here */
			
			/* get the AC ID of incoming packet */
			ac_idx = WMM_UP2AC_MAP[qos_p[0]];
#endif /* UAPSD_SUPPORT */

			RX_BLK_SET_FLAG(rx_blk, fRX_RETRIEVE);
			if ((tr_entry->ps_state == APPS_RETRIEVE_GOING) 
				|| (tr_entry->ps_state == APPS_RETRIEVE_START_PS))
			{
				if (qos_p[1] == PS_RETRIEVE_TOKEN) //retrive_token : 0x76
				{
					rx_blk->DataSize = 0;
					rx_blk->MPDUtotalByteCnt = rx_base->rxd_0.rx_byte_cnt;
					tr_entry->ps_qbitmap &= (~(1 << qos_p[0])) ;
					//DBGPRINT(RT_DEBUG_ERROR, ("txd_1->wlan_idx=%d, got token:%d, tr_entry->ps_qbitmap=%x\n", txd_1->wlan_idx, qos_p[0], tr_entry->ps_qbitmap));                  
					if (tr_entry->ps_qbitmap == 0x0)
					{
						//QUEUE_ENTRY *pQEntry;
						UINT32 wlan_idx = 0;
						int for_qid;
						
						if (tr_entry->ps_queue.Number)
						{
							DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_PS, ("(wcid=%d) put ps_queue packets(Number=%d) to tx_queue.\n", txd_1->wlan_idx, tr_entry->ps_queue.Number));
							MtEnqTxSwqFromPsQueue(ad, q_idx, tr_entry);
						}

						for (for_qid = 0; for_qid < WMM_QUE_NUM; for_qid++)
                                                {
							tr_entry->TokenCount[for_qid] = tr_entry->tx_queue[for_qid].Number;
						}
						tr_entry->ps_state = APPS_RETRIEVE_WAIT_EVENT;
						DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_PS, ("(ps_state = %d)Receives all RMAC_RX_PKT_TYPE_RETRIEVE packets and send CMDTHREAD_PS_CLEAR cmd.\n", 
							tr_entry->ps_state));
						wlan_idx = pEntry->wcid;
						RTEnqueueInternalCmd(ad, CMDTHREAD_PS_CLEAR, (VOID *)&wlan_idx, sizeof(UINT32));
					}
				} 
				else 
				{
					INT packet_length = 0;

					if ((pWifi_hdr->FC.SubType == SUBTYPE_DATA_NULL) || (pWifi_hdr->FC.SubType == SUBTYPE_QOS_NULL))
						return 0;
				
					DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_PS, ("da=(%02x:%02x:%02x:%02x:%02x:%02x), sa=(%02x:%02x:%02x:%02x:%02x:%02x), qos_p->ac%d, qos_p->value=%d, Sequence=%d txd_1->wlan_idx=%d\n"
						, PRINT_MAC(da), PRINT_MAC(sa), qos_p[0], qos_p[1], pWifi_hdr->Sequence,txd_1->wlan_idx));

					packet_length = rx_base->rxd_0.rx_byte_cnt-sizeof(TMAC_TXD_L) - padding - hdr_info - sizeof(SNAP_802_1H);
					if (packet_length <= 0)
						return 0;
						
					rx_hw_hdr_len = sizeof(TMAC_TXD_L);
					SET_OS_PKT_DATAPTR(rx_packet, GET_OS_PKT_DATAPTR(rx_packet) + sizeof(TMAC_TXD_L) + padding + hdr_info + sizeof(SNAP_802_1H));
					SET_OS_PKT_LEN(rx_packet, packet_length);

					OS_PKT_HEAD_BUF_EXTEND(rx_packet, MAC_ADDR_LEN + MAC_ADDR_LEN);
					NdisCopyMemory(GET_OS_PKT_DATAPTR(rx_packet) + MAC_ADDR_LEN, sa, MAC_ADDR_LEN);
					NdisCopyMemory(GET_OS_PKT_DATAPTR(rx_packet), da, MAC_ADDR_LEN);

					RTMP_SET_PACKET_WCID(rx_packet, txd_1->wlan_idx);

					DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_PS, ("parse_rx_packet_type  not token txd_1->wlan_idx: %x, rx_packet addr: %x rx_wcid: %x\n",txd_1->wlan_idx,(u32)rx_packet,RTMP_GET_PACKET_WCID(rx_packet)));
					RTMP_SET_PACKET_WDEV(rx_packet, tr_entry->wdev->wdev_idx);

					RTMP_IRQ_LOCK(&ad->irq_lock, IrqFlags);
					if (tr_entry->ps_queue.Number >= MAX_PACKETS_IN_PS_QUEUE)
					{
						// drop the ps retrive pks due to limit ps queue max length
						RELEASE_NDIS_PACKET(ad, rx_packet, NDIS_STATUS_FAILURE);
					}
					else
					{
						InsertTailQueue(&tr_entry->ps_queue, PACKET_TO_QUEUE_ENTRY(rx_packet));
					}
					RTMP_IRQ_UNLOCK(&ad->irq_lock, IrqFlags);

#ifdef UAPSD_SUPPORT
					if (UAPSD_MR_IS_NOT_TIM_BIT_NEEDED_HANDLED(&ad->MacTab.Content[tr_entry->wcid], ac_idx))
					{
						/*
							1. the station is UAPSD station;
							2. one of AC is non-UAPSD (legacy) AC;
							3. the destinated AC of the packet is UAPSD AC. 
						*/
						/* So we can not set TIM bit due to one of AC is legacy AC */
					}
					else
#endif /* UAPSD_SUPPORT */
					{
						WLAN_MR_TIM_BIT_SET(ad, tr_entry->func_tb_idx, tr_entry->wcid);
					}
				}
			}
			else
			{
				DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_PS, ("da=(%02x:%02x:%02x:%02x:%02x:%02x), sa=(%02x:%02x:%02x:%02x:%02x:%02x), qos_p->ac%d, qos_p->value=%d, Sequence=%d txd_1->wlan_idx=%d, ps_state = %d\n"
					, PRINT_MAC(da), PRINT_MAC(sa), qos_p[0], qos_p[1], pWifi_hdr->Sequence,txd_1->wlan_idx, tr_entry->ps_state));
			}
			break;
		}
#endif /* MT_PS */

        case RMAC_RX_PKT_TYPE_RX_TMR:
            {
                TMR_FRM_STRUC *tmr = (TMR_FRM_STRUC *)(GET_OS_PKT_DATAPTR(rx_packet));
                struct rmac_rxd_0_tmr *ptmr_d0 = &tmr->tmr_d0;
                //TMR_D_1 *ptmr_d1 = &tmr->tmr_d1;
                UINT32 *ptod_0 = &tmr->tod_0;
                UINT32 *ptoa_0 = &tmr->toa_0;
                TMR_D_6 *tmr_d6 = &tmr->tmr_d6;

                hex_dump("TMR", (UCHAR *)tmr, 20);
				
				return 0;

                hex_dump("TMR", (UCHAR *)tmr, ptmr_d0->rx_byte_cnt);
                DBGPRINT(RT_DEBUG_OFF, ("TMR Report: ir = %d\n", ptmr_d0->ir));
                DBGPRINT(RT_DEBUG_OFF, ("DWORD_0: ByteCnt=%d, NC=%d, TMF=%d, "
                                        "toa_vld=%d, tod_vld=%d\n",
                            ptmr_d0->rx_byte_cnt, ptmr_d0->nc, ptmr_d0->tmf,
                            ptmr_d0->toa_vld, ptmr_d0->tod_vld));

                if (ptmr_d0->ir == 1) //responder case!
                {
                    TMR_D_2 *ptmr_d2 = &tmr->tmr_d2;
                    UINT32 *pta_16 = &tmr->ta_16;

                    DBGPRINT(RT_DEBUG_OFF, ("TYPE=%x,SUB_TYPE=%x\n",
                            ptmr_d0->type, ptmr_d0->sub_type));
                    DBGPRINT(RT_DEBUG_OFF, ("DWORD_2: TA[0:15]=%x, SN=%x\n", ptmr_d2->field.ta_0, ptmr_d2->field.sn));
                    DBGPRINT(RT_DEBUG_OFF, ("DWORD_3: TA[16:47]=%x\n", *pta_16));
                }

                DBGPRINT(RT_DEBUG_OFF, ("DWORD_4: TOD[0:31]=0x%x\n", *ptod_0));
                DBGPRINT(RT_DEBUG_OFF, ("DWORD_6: TOD[32:47]=0x%x\n", tmr_d6->field.tod_32));

                DBGPRINT(RT_DEBUG_OFF, ("DWORD_5: TOA[0:31]=0x%x\n", *ptoa_0));
                DBGPRINT(RT_DEBUG_OFF, ("DWORD_6: TOA[32:47]=0x%x\n", tmr_d6->field.toa_32));
            }
            break;

        default:
            DBGPRINT(RT_DEBUG_OFF, ("%s():Invalid PktType:%d\n", __FUNCTION__, rx_pkt_type));
            //rx_hw_hdr_len = pRxD->SDL0;
            //rx_blk->MPDUtotalByteCnt = 0;
            //rx_blk->DataSize = 0;
            break;
    }

    return rx_hw_hdr_len;
}
#endif /* MT_MAC */

#ifdef DOT11_N_SUPPORT
#ifdef RELEASE_EXCLUDE
/*
	For IOT compatibility, BA session can be build only when
	1. It is Ralink chip or
	2. It is OPEN or AES mode
*/
#endif /* RELEASE_EXCLUDE */
VOID RTMP_BASetup(RTMP_ADAPTER *pAd, STA_TR_ENTRY *tr_entry, UINT8 UPriority)
{
	MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[tr_entry->wcid];

	if (pAd->CommonCfg.BACapability.field.AutoBA == FALSE)
		return;

	// TODO: shiang-usw, fix me for pEntry, we should replace this paramter as tr_entry!
	if ((tr_entry && tr_entry->EntryType != ENTRY_CAT_MCAST && tr_entry->wcid < MAX_LEN_OF_MAC_TABLE) &&
		(pEntry->NoBADataCountDown == 0) && IS_HT_STA(pEntry))
	{
		BOOLEAN isRalink = CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);


		if (((pEntry->TXBAbitmap & (1<<UPriority)) == 0)
			/* && ((pEntry->BADeclineBitmap & (1 << UPriority)) == 0) */
			&& (tr_entry->PortSecured == WPA_802_1X_PORT_SECURED)
			&& (!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))
#ifdef RT3290
			&& (!(IS_RT3290(pAd) && pAd->WlanBTCoexInfo.ampduOff == TRUE))
#endif /* RT3290 */
			&& ((isRalink || IS_ENTRY_MESH(pEntry) || IS_ENTRY_WDS(pEntry))
				|| (pEntry->WepStatus == Ndis802_11WEPDisabled || pEntry->WepStatus == Ndis802_11AESEnable
#ifdef WAPI_SUPPORT
					|| pEntry->WepStatus == Ndis802_11EncryptionSMS4Enabled
#endif /* WAPI_SUPPORT */
				))
			)
		{
			BAOriSessionSetUp(pAd, pEntry, UPriority, 0, 10, FALSE);
		}
	}
}
#endif /* DOT11_N_SUPPORT */


/*
	========================================================================

	Routine Description:
		API for MLME to transmit management frame to AP (BSS Mode)
	or station (IBSS Mode)

	Arguments:
		pAd Pointer to our adapter
		pData		Pointer to the outgoing 802.11 frame
		Length		Size of outgoing management frame

	Return Value:
		NDIS_STATUS_FAILURE
		NDIS_STATUS_PENDING
		NDIS_STATUS_SUCCESS

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
#define MAX_DATAMM_RETRY	3

NDIS_STATUS MiniportMMRequest(RTMP_ADAPTER *pAd, UCHAR QueIdx, UCHAR *pData, UINT Length)
{
	PNDIS_PACKET pPacket;
	NDIS_STATUS Status = NDIS_STATUS_FAILURE;
	ULONG FreeNum;
#ifdef RTMP_MAC_PCI
	unsigned long	IrqFlags = 0;
	BOOLEAN bUseDataQ = FALSE;
#endif /* RTMP_MAC_PCI */
	BOOLEAN FlgDataQForce = FALSE, FlgIsLocked = FALSE;
	int retryCnt = 0;
	BOOLEAN			FlgIsCheckPS = FALSE;

	ASSERT(Length <= MGMT_DMA_BUFFER_SIZE);

	if ((QueIdx & MGMT_USE_QUEUE_FLAG) == MGMT_USE_QUEUE_FLAG)
	{
#ifdef RTMP_MAC_PCI
		bUseDataQ = TRUE;
#endif /* RTMP_MAC_PCI */
		QueIdx &= (~MGMT_USE_QUEUE_FLAG);
	}

#ifndef MT_PS
#ifdef CONFIG_FPGA_MODE
	if (pAd->fpga_ctl.fpga_on & 0x1) {
		if (pAd->fpga_ctl.tx_kick_cnt > 0) {
			if (pAd->fpga_ctl.tx_kick_cnt < 0xffff)
				pAd->fpga_ctl.tx_kick_cnt--;
		}
		else
			return NDIS_STATUS_FAILURE;

		QueIdx = 0;
		//bUseDataQ = TRUE;
	}
#endif /* CONFIG_FPGA_MODE */
#endif /* MT_PS */

	if ((QueIdx & MGMT_USE_PS_FLAG) == MGMT_USE_PS_FLAG)
	{
		FlgIsCheckPS = TRUE;
		QueIdx &= (~MGMT_USE_PS_FLAG);
	}
#ifdef RTMP_MAC_PCI
	if (pAd->MACVersion == 0x28600100)
	{
		/* do not care about the version */
		QueIdx = (bUseDataQ ==TRUE ? QueIdx : 3);
		bUseDataQ = TRUE;
	}

#if 0 //defined(MT7603) || defined(MT7628)
	// TODO: shiang-7603, now only verifiy the AC0 when DMA scheduler is not ready!
	if (IS_MT7603(pAd) || IS_MT7628(pAd)) {
		bUseDataQ = TRUE;
	}
#endif /* MT7603 */

	if (bUseDataQ)
	{
		RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
		FlgIsLocked = TRUE;
		retryCnt = MAX_DATAMM_RETRY;
	}
#endif /* RTMP_MAC_PCI */

	do
	{
		/* Reset is in progress, stop immediately*/
		if (RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_RESET_IN_PROGRESS |
								fRTMP_ADAPTER_HALT_IN_PROGRESS |
								fRTMP_ADAPTER_NIC_NOT_EXIST |
								fRTMP_ADAPTER_RADIO_OFF)) ||
			 !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP)
#ifdef P2P_SUPPORT
			|| IS_P2P_ABSENCE(pAd)
#endif /* P2P_SUPPORT */
		)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_MAC_USB
		if(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_IDLE_RADIO_OFF)) {
#ifdef RLT_MAC
			if (pAd->chipCap.hif_type == HIF_RLT) {
#ifdef MT7601
				if ( IS_MT7601(pAd) )
					ASIC_RADIO_ON(pAd, DOT11_RADIO_ON);
				else
#endif /* MT7601 */
					ASIC_RADIO_ON(pAd, MLME_RADIO_ON);
			}
#endif /* RLT_MAC */

#ifdef RTMP_MAC
			if (pAd->chipCap.hif_type == HIF_RTMP) {
				RT28xxUsbAsicRadioOn(pAd);
			}
#endif /* RTMP_MAC */
		}
#endif /* RTMP_MAC_USB */
#endif /* CONFIG_STA_SUPPORT */

		/* Check Free priority queue*/
		/* Since we use PBF Queue2 for management frame.  Its corresponding DMA ring should be using TxRing.*/
#ifdef RTMP_MAC_PCI
		if (bUseDataQ)
		{
#ifndef __ECOS /* Eddy temporary remove on ECOS */
			/* free Tx(QueIdx) resources*/
			RTMPFreeTXDUponTxDmaDone(pAd, QueIdx);
#endif /* __ECOS */
			FreeNum = GET_TXRING_FREENO(pAd, QueIdx);
		}
		else
#endif /* RTMP_MAC_PCI */
		{
			FreeNum = GET_MGMTRING_FREENO(pAd);
#ifdef __ECOS /* keep the management ring has more space */
			if (FreeNum < (MGMT_RING_SIZE/2)) {
				RTMPHandleMgmtRingDmaDoneInterrupt(pAd);
				FreeNum = GET_MGMTRING_FREENO(pAd);
			}
#endif /* __ECOS */
		}

		if ((FreeNum > 0))
		{
			INT hw_len;
			UCHAR rtmpHwHdr[40];

			// TODO: shiang-7603
			hw_len = pAd->chipCap.tx_hw_hdr_len;
			ASSERT((sizeof(rtmpHwHdr) > hw_len));

			/* We need to reserve space for rtmp hardware header. i.e., TxWI for RT2860 and TxInfo+TxWI for RT2870*/
			NdisZeroMemory(&rtmpHwHdr, hw_len);
			Status = RTMPAllocateNdisPacket(pAd, &pPacket, (UCHAR *)&rtmpHwHdr[0], hw_len, pData, Length);
			if (Status != NDIS_STATUS_SUCCESS)
			{
				DBGPRINT(RT_DEBUG_WARN, ("MiniportMMRequest (error:: can't allocate NDIS PACKET)\n"));
				break;
			}

#ifdef DOT11W_PMF_SUPPORT
#ifdef CONFIG_STA_SUPPORT
			if (INFRA_ON(pAd))
#endif /* CONFIG_STA_SUPPORT */
				if (pAd->chipCap.hif_type != HIF_MT)
				PMF_PerformTxFrameAction(pAd, pPacket);
#endif /* DOT11W_PMF_SUPPORT */

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#ifdef UAPSD_SUPPORT
			UAPSD_MR_QOS_NULL_HANDLE(pAd, pData, pPacket);
#endif /* UAPSD_SUPPORT */
#else
#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_SUPPORT
#ifdef P2P_SUPPORT
			if (P2P_GO_ON(pAd))
#else
#ifdef RT_CFG80211_P2P_SUPPORT
	        if (RTMP_CFG80211_VIF_P2P_GO_ON(pAd))
#else
        	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
#endif /* RT_CFG80211_P2P_SUPPORT */
#endif /* P2P_SUPPORT */
			{
				UAPSD_MR_QOS_NULL_HANDLE(pAd, pData, pPacket);
			}
#endif /* UAPSD_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */

#ifdef RTMP_MAC_PCI
			if (bUseDataQ)
			{
				FlgDataQForce = TRUE;
				retryCnt--;
			}
#endif /* RTMP_MAC_PCI */

			Status = MlmeHardTransmit(pAd, QueIdx, pPacket, FlgDataQForce, FlgIsLocked, FlgIsCheckPS);
			if (Status == NDIS_STATUS_SUCCESS)
				retryCnt = 0;
			else
				RELEASE_NDIS_PACKET(pAd, pPacket, Status);
		}
		else
		{
			pAd->RalinkCounters.MgmtRingFullCount++;
#ifdef RTMP_MAC_PCI
			if (bUseDataQ)
			{
				retryCnt--;
				DBGPRINT(RT_DEBUG_TRACE, ("retryCnt %d\n", retryCnt));
				if (retryCnt == 0)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Qidx(%d), not enough space in DataRing, MgmtRingFullCount=%ld!\n",
											QueIdx, pAd->RalinkCounters.MgmtRingFullCount));
				}
			}
			else
#endif /* RTMP_MAC_PCI */
			{
				DBGPRINT(RT_DEBUG_ERROR, ("Qidx(%d), not enough space in MgmtRing, MgmtRingFullCount=%ld!\n",
										QueIdx, pAd->RalinkCounters.MgmtRingFullCount));
			}

#ifdef RELEASE_EXCLUDE
#ifdef RTMP_MAC_USB
			/* TODO: need to remove it when do formal release, it's used for debug.*/
			Show_TxBulk_Proc(pAd, NULL);
#endif /* RTMP_MAC_USB */
#endif /* RELEASE_EXCLUDE */
		}
	} while (retryCnt > 0);


#ifdef RTMP_MAC_PCI
	if (bUseDataQ)
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
#endif /* RTMP_MAC_PCI */

	return Status;
}


#ifdef CONFIG_AP_SUPPORT
/*
	========================================================================

	Routine Description:
		Copy frame from waiting queue into relative ring buffer and set
	appropriate ASIC register to kick hardware transmit function

	Arguments:
		pAd Pointer to our adapter
		pBuffer 	Pointer to	memory of outgoing frame
		Length		Size of outgoing management frame
		FlgIsDeltsFrame 1: the frame is a DELTS frame

	Return Value:
		NDIS_STATUS_FAILURE
		NDIS_STATUS_PENDING
		NDIS_STATUS_SUCCESS

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
void AP_QueuePsActionPacket(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY *pMacEntry,
	IN PNDIS_PACKET pPacket,
	IN BOOLEAN FlgIsDeltsFrame,
	IN BOOLEAN FlgIsLocked,
	IN UCHAR MgmtQid)
{
#ifdef UAPSD_SUPPORT
#ifdef UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY
	PNDIS_PACKET DuplicatePkt = NULL;
#endif /* UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY */
#endif /* UAPSD_SUPPORT */
	STA_TR_ENTRY *tr_entry = &pAd->MacTab.tr_entry[pMacEntry->wcid];

	/* Note: for original mode of 4 AC are UAPSD, if station want to change
			the mode of a AC to legacy PS, we dont know where to put the
			response;
			1. send the response;
			2. but the station is in ps mode, so queue the response;
			3. we should queue the reponse to UAPSD queue because the station
				is not yet change its mode to legacy ps AC;
			4. so AP should change its mode to legacy ps AC only when the station
				sends a trigger frame and we send out the reponse;
			5. the mechanism is too complicate; */

#ifdef UAPSD_SUPPORT
	/*
		If the frame is action frame and the VO is UAPSD, we can not send the
		frame to VO queue, we need to send to legacy PS queue; or the frame
		maybe not got from QSTA.
	*/
/*    if ((pMacEntry->bAPSDDeliverEnabledPerAC[MgmtQid]) &&*/
/*		(FlgIsDeltsFrame == 0))*/
	if (pMacEntry->bAPSDDeliverEnabledPerAC[MgmtQid])
	{
		/* queue the management frame to VO queue if VO is deliver-enabled */
		DBGPRINT(RT_DEBUG_TRACE, ("ps> mgmt to UAPSD queue %d ... (IsDelts: %d)\n",
				MgmtQid, FlgIsDeltsFrame));

#ifdef UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY
		if (!pMacEntry->bAPSDAllAC)
		{
			/* duplicate one packet to legacy PS queue */
			RTMP_SET_PACKET_UAPSD(pPacket, 0, MgmtQid);
			DuplicatePkt = DuplicatePacket(wdev->if_dev, pPacket);
		}
		else
#endif /* UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY */
		{
			RTMP_SET_PACKET_UAPSD(pPacket, 1, MgmtQid);
		}

#if 1
		UAPSD_PacketEnqueue(pAd, pMacEntry, pPacket, MgmtQid, FALSE);
#else
		rtmp_enq_req(pAd, pPacket, MgmtQid, tr_entry, FlgIsLocked,NULL);
#endif

		if (pMacEntry->bAPSDAllAC)
		{
			/* mark corresponding TIM bit in outgoing BEACON frame*/
			WLAN_MR_TIM_BIT_SET(pAd, pMacEntry->func_tb_idx, pMacEntry->Aid);
		}
		else
		{
#ifdef UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY
			/* duplicate one packet to legacy PS queue */

			/*
				Sometimes AP will send DELTS frame to STA but STA will not
				send any trigger frame to get the DELTS frame.
				We must force to send it so put another one in legacy PS
				queue.
			*/
			if (DuplicatePkt != NULL)
			{
				pPacket = DuplicatePkt;
				goto Label_Legacy_PS;
			}
#endif /* UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY */
		}
	}
	else
#endif /* UAPSD_SUPPORT */
	{
#ifdef UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY
Label_Legacy_PS:
#endif /* UAPSD_CC_FUNC_PS_MGMT_TO_LEGACY */
		DBGPRINT(RT_DEBUG_TRACE,
					("ps> mgmt to legacy ps queue... (%d)\n", FlgIsDeltsFrame));

		if (tr_entry->ps_queue.Number >= MAX_PACKETS_IN_PS_QUEUE ||
			rtmp_enq_req(pAd, pPacket, MgmtQid, tr_entry, FlgIsLocked, NULL) == FALSE)
		{
			DBGPRINT(RT_DEBUG_TRACE,
					("%s(%d): WLAN_TX_DROP, pPacket=%p, QueIdx=%d, ps_queue_num=%d, wcid=%d\n",
					__FUNCTION__, __LINE__, pPacket, MgmtQid, tr_entry->ps_queue.Number, tr_entry->wcid));
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_RESOURCES);
			return;
		}
		else
		{
			DBGPRINT(RT_DEBUG_TRACE, ("ps> mgmt to legacy ps queue... (%d)\n", FlgIsDeltsFrame));
			/* mark corresponding TIM bit in outgoing BEACON frame*/
			WLAN_MR_TIM_BIT_SET(pAd, pMacEntry->func_tb_idx, pMacEntry->Aid);
		}

	}
}
#endif /* CONFIG_AP_SUPPORT */


/*
	========================================================================

	Routine Description:
		Copy frame from waiting queue into relative ring buffer and set
	appropriate ASIC register to kick hardware transmit function

	Arguments:
		pAd Pointer to our adapter
		pBuffer 	Pointer to	memory of outgoing frame
		Length		Size of outgoing management frame

	Return Value:
		NDIS_STATUS_FAILURE
		NDIS_STATUS_PENDING
		NDIS_STATUS_SUCCESS

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
NDIS_STATUS MlmeHardTransmit(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR QueIdx,
	IN PNDIS_PACKET pPacket,
	IN BOOLEAN FlgDataQForce,
	IN	BOOLEAN			FlgIsLocked,
	IN	BOOLEAN			FlgIsCheckPS)
{
#ifdef CONFIG_AP_SUPPORT
	MAC_TABLE_ENTRY *pEntry = NULL;
	HEADER_802_11 *pHeader_802_11;
	//UINT8 TXWISize = pAd->chipCap.TXWISize;
	UINT8 tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;
#endif /* CONFIG_AP_SUPPORT */
	PACKET_INFO PacketInfo;
	UCHAR *pSrcBufVA;
	UINT SrcBufLen;

	if ((pAd->Dot11_H.RDMode != RD_NORMAL_MODE)
#ifdef CARRIER_DETECTION_SUPPORT
#ifdef CONFIG_AP_SUPPORT
		||(isCarrierDetectExist(pAd) == TRUE)
#endif /* CONFIG_AP_SUPPORT */
#endif /* CARRIER_DETECTION_SUPPORT */
		)
	{
		return NDIS_STATUS_FAILURE;
	}

	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);
	if (pSrcBufVA == NULL)
		return NDIS_STATUS_FAILURE;

#ifdef CONFIG_AP_SUPPORT
	// TODO: shiang-7603
	pHeader_802_11 = (HEADER_802_11 *) (pSrcBufVA + tx_hw_hdr_len);

	/*
		Section 11.2.1.1 STA Power Management modes of IEEE802.11-2007:
		The Power Managment bit shall not be set in any management frame,
		except an Action frame.

		So in the 'baseline' test plan
		(Wi-Fi 802.11 WPA2, WPA, WEP Interoperability Test Plan),
		Section 2.2.6, the following Requirement:
        APs shall ignore the power save bit in any received Authenticate and
		(Re) Associate, and shall assume that the station is awake for the
		response.
	*/

	/*
		IEEE802.11, 11.2.1.4 AP operation during the contention period f)
		A single buffered MSDU or management frame for a STA in the PS mode shall
		be forwarded to the STA after a PS-Poll has been received from that STA.
		The More Data field shall be set to indicate the presence of further
		buffered MSDUs or "management frames" for the polling STA.
	*/

	/*
		IEEE802.11e, 11.2.1.4 Power management with APSD,
		An unscheduled SP ends after the QAP has attempted to transmit at least
		one MSDU or MMPDU associated with a delivery-enabled AC and destined for
		the non-AP QSTA, but no more than the number indicated in the Max SP
		Length field if the field has a nonzero value.
	*/

	if ((pHeader_802_11->FC.Type == FC_TYPE_DATA) ||
		(pHeader_802_11->FC.Type == FC_TYPE_MGMT))
	{
		if ((pHeader_802_11->FC.Type == FC_TYPE_MGMT) || (pHeader_802_11->FC.SubType != SUBTYPE_QOS_NULL))
			pEntry = MacTableLookup(pAd, pHeader_802_11->Addr1);
	}

#ifdef DOT11K_RRM_SUPPORT
#ifdef QUIET_SUPPORT
	if ((pEntry != NULL)
		&& (pEntry->func_tb_idx < pAd->ApCfg.BssidNum)
		&& IS_RRM_QUIET(pAd, pEntry->func_tb_idx))
	{
		return NDIS_STATUS_FAILURE;
	}
#endif /* QUIET_SUPPORT */
#endif /* DOT11K_RRM_SUPPORT */

#ifdef WMM_ACM_SUPPORT
	if (ACMR_IS_AP_MODE(pAd))
	{
		ACMR_PKT_QOS_TYPE_SET(pPacket, 0);

		/* check if the action frame is ADDTS Response */
		if ((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
			(pHeader_802_11->FC.SubType == SUBTYPE_ACTION))
		{
			UINT8 FlgIsRspFrame;

			FlgIsRspFrame = ACMP_IsResponseFrame(\
								((UCHAR *)pHeader_802_11)+sizeof(HEADER_802_11));

			if (FlgIsRspFrame == TRUE)
			{
				ACMR_PKT_QOS_TYPE_SET(pPacket, ACM_QOS_TYPE_ADDTS_RSP);
			}
		}
	}
#endif /* WMM_ACM_SUPPORT */

#ifdef CONFIG_FPGA_MODE
	if (pAd->fpga_ctl.fpga_on & 0x1) {
		if (pAd->fpga_ctl.tx_kick_cnt > 0) {
			if (pAd->fpga_ctl.tx_kick_cnt < 0xffff)  {
				pAd->fpga_ctl.tx_kick_cnt--;
			}
		} else {
			return NDIS_STATUS_FAILURE;
		}
	}
#endif /* CONFIG_FPGA_MODE */

	if ((pEntry != NULL) &&
		(pEntry->PsMode == PWR_SAVE) &&
		(((pHeader_802_11->FC.Type == FC_TYPE_DATA) &&
			(pHeader_802_11->FC.SubType != SUBTYPE_DATA_NULL) &&
			(pHeader_802_11->FC.SubType != SUBTYPE_QOS_NULL)) ||
		((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
			(pHeader_802_11->FC.SubType == SUBTYPE_ACTION)) ||
		((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
			(pHeader_802_11->FC.SubType == SUBTYPE_ACTION_NO_ACK)) ||
			(FlgIsCheckPS == 1)))
	{
		/* the peer is in PS mode, we need to queue the management frame */
		UINT8 FlgIsDeltsFrame = 0, MgmtQid = QID_AC_VO;

		/*
			1. Data & Not QoS Null, or
			2. Management & Action, or
			3. Management & Action No ACK;
		*/
		DBGPRINT(RT_DEBUG_TRACE, ("STA in ps mode, queue the mgmt frame\n"));
		RTMP_SET_PACKET_WCID(pPacket, pEntry->wcid);
		RTMP_SET_PACKET_MGMT_PKT(pPacket, 1); /* is management frame */
		RTMP_SET_PACKET_MGMT_PKT_DATA_QUE(pPacket, 0); /* default to management queue */

#ifdef P2P_SUPPORT
		if (IS_P2P_GO_ENTRY(pEntry))
		{
			RTMP_SET_PACKET_NET_DEVICE_P2P(pPacket,MAIN_MBSSID);
			RTMP_SET_PACKET_OPMODE(pPacket, OPMODE_AP);
		}
#endif /* P2P_SUPPORT */

#ifdef RT_CFG80211_P2P_SUPPORT
		if(pEntry->wdev->wdev_type == WDEV_TYPE_AP)
		{
			//RTMP_SET_PACKET_NET_DEVICE_MBSSID(pPacket, MAIN_MBSSID);
			RTMP_SET_PACKET_OPMODE(pPacket, OPMODE_AP);
		}
#endif /* RT_CFG80211_P2P_SUPPORT */
		
		if (FlgDataQForce == TRUE)
			RTMP_SET_PACKET_MGMT_PKT_DATA_QUE(pPacket, 1); /* force to data queue */

		if ((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
			(pHeader_802_11->FC.SubType == SUBTYPE_ACTION))
		{
			FRAME_ADDBA_REQ *pFrameBa = (FRAME_ADDBA_REQ *)pHeader_802_11;
			if (pFrameBa->Category == CATEGORY_BA)
				MgmtQid = QueIdx;
		}

#ifdef WMM_ACM_SUPPORT
		if (ACMR_IS_AP_MODE(pAd))
		{
			/* check if the packet is the DELTS frame */
			FlgIsDeltsFrame = ACMP_IsDeltsFrame(\
								((UCHAR *)pHeader_802_11)+sizeof(HEADER_802_11));
			if (FlgIsDeltsFrame == TRUE)
			{
				ACMR_PKT_QOS_TYPE_SET(pPacket, ACM_QOS_TYPE_DELTS);
			}
		}
#endif /* WMM_ACM_SUPPORT */

#ifdef CONFIG_HOTSPOT_R2
		if (((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
            (pHeader_802_11->FC.SubType == SUBTYPE_DISASSOC)) || 
			((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
            (pHeader_802_11->FC.SubType == SUBTYPE_DEAUTH)))
        {
			RTMP_SET_PACKET_DISASSOC(pPacket, 1);
			pEntry->IsKeep = 1;
		}
#endif /* CONFIG_HOTSPOT_R2 */

		AP_QueuePsActionPacket(pAd, pEntry, pPacket, FlgIsDeltsFrame,
								FlgIsLocked, MgmtQid);
		return NDIS_STATUS_SUCCESS;
	}
    else
#endif /* CONFIG_AP_SUPPORT */
    {
#ifdef RTMP_MAC_PCI
    	if (FlgDataQForce == TRUE)
    		return MlmeHardTransmitTxRing(pAd,QueIdx,pPacket);
    	else
#endif /* RTMP_MAC_PCI */
    		return MlmeHardTransmitMgmtRing(pAd,QueIdx,pPacket);
    }
}


NDIS_STATUS MlmeHardTransmitMgmtRing(RTMP_ADAPTER *pAd, UCHAR QueIdx, PNDIS_PACKET pPacket)
{
	PACKET_INFO PacketInfo;
	UCHAR *pSrcBufVA, *tmac_info;
	UINT SrcBufLen;
	HEADER_802_11 *pHeader_802_11;
	BOOLEAN bAckRequired, bInsertTimestamp;
    PFRAME_BAR pBar = NULL;
	UCHAR MlmeRate;
	MAC_TABLE_ENTRY *pMacEntry = NULL;
	UCHAR PID, wcid, tx_rate;
    HTTRANSMIT_SETTING *transmit = NULL, TransmitSetting;
	//UINT8 TXWISize = pAd->chipCap.TXWISize;
	UINT8 tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;
	MAC_TX_INFO mac_info;
#ifdef CONFIG_AP_SUPPORT
#ifdef SPECIFIC_TX_POWER_SUPPORT
	UCHAR TxPwrAdj = 0;
#endif /* SPECIFIC_TX_POWER_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#ifdef RT_CFG80211_P2P_SUPPORT
	struct ieee80211_mgmt *mgmt;
	BOOLEAN is_P2P_action_frame = FALSE;
#endif /* RT_CFG80211_P2P_SUPPORT */

	UCHAR prot = 0;
	UCHAR apidx = 0;
	ULONG Flags = 0;
	PCFG80211_CTRL pCfg80211_ctrl = &pAd->cfg80211_ctrl;

	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);

	/* Make sure MGMT ring resource won't be used by other threads*/
	RTMP_IRQ_LOCK(&pAd->MgmtRingLock, Flags);	

	NdisZeroMemory((UCHAR *)&mac_info, sizeof(mac_info));
	
	if (pSrcBufVA == NULL)
	{
		/* The buffer shouldn't be NULL*/
		RTMP_IRQ_UNLOCK(&pAd->MgmtRingLock, Flags);
		return NDIS_STATUS_FAILURE;
	}

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		/* outgoing frame always wakeup PHY to prevent frame lost */
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
		{
			DBGPRINT_RAW(RT_DEBUG_TRACE, ("%s::Wake up H/W\n", __FUNCTION__));
			AsicForceWakeup(pAd, TRUE);
		}
	}
#endif /* CONFIG_STA_SUPPORT */

#if defined(MT7603) || defined(MT7628)
	// TODO: shiang-7603
	tmac_info = pSrcBufVA;
#else
	tmac_info = pSrcBufVA + TXINFO_SIZE;
#endif /* MT7603 */
	pHeader_802_11 = (HEADER_802_11 *) (pSrcBufVA + tx_hw_hdr_len);

//DBGPRINT(RT_DEBUG_OFF, ("%s(): pSrcBufVA=0x%p, pHeader_802_11=0x%p, tmac_info=%p, tx_hw_hdr_len=%d\n",
//			__FUNCTION__, pSrcBufVA, pHeader_802_11, tmac_info, tx_hw_hdr_len));
	if (pHeader_802_11->Addr1[0] & 0x01)
		MlmeRate = pAd->CommonCfg.BasicMlmeRate;
	else
		MlmeRate = pAd->CommonCfg.MlmeRate;

	/* Verify Mlme rate for a / g bands.*/
	if ((pAd->LatchRfRegs.Channel > 14) && (MlmeRate < RATE_6)) /* 11A band*/
		MlmeRate = RATE_6;

	if (((pHeader_802_11->FC.Type == FC_TYPE_DATA) && (pHeader_802_11->FC.SubType == SUBTYPE_QOS_NULL)) || 
		((pHeader_802_11->FC.Type == FC_TYPE_CNTL) && (pHeader_802_11->FC.SubType == SUBTYPE_BLOCK_ACK_REQ)))
	{
		pMacEntry = MacTableLookup(pAd, pHeader_802_11->Addr1);
	}

#ifdef DOT11W_PMF_SUPPORT
	if (pAd->chipCap.hif_type == HIF_MT)
	{
		UINT32 ret = 0;
		MAC_TABLE_ENTRY *pEntry = NULL;

		pEntry = MacTableLookup(pAd, pHeader_802_11->Addr1);
		ret = PMF_RobustFrameClassify(
					(PHEADER_802_11)pHeader_802_11,
					(PUCHAR)(((PUCHAR)pHeader_802_11)+LENGTH_802_11),
					(SrcBufLen - LENGTH_802_11 - tx_hw_hdr_len),
					(PUCHAR) pEntry,
					FALSE);

		if (pEntry)
			apidx = pEntry->func_tb_idx;

		if (ret == UNICAST_ROBUST_FRAME)
		{
			prot = 1;
			pHeader_802_11->FC.Wep = 1;
		}
		else if (ret == GROUP_ROBUST_FRAME)
		{
			ret = PMF_EncapBIPAction(pAd,
						(PUCHAR)pHeader_802_11,
						(SrcBufLen - tx_hw_hdr_len));
			if (ret == PMF_STATUS_SUCCESS)
				prot = 2;
			else
				DBGPRINT(RT_DEBUG_OFF, ("%s, PMF GROUP ROBUST Encap fail, ret=%d\n",
					__FUNCTION__, ret));
		}
	}
#endif

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		/* Fixed W52 with Activity scan issue in ABG_MIXED and ABGN_MIXED mode.*/
		// TODO: shiang-6590, why we need this condition check here?
		if (WMODE_EQUAL(pAd->CommonCfg.PhyMode, WMODE_A | WMODE_B | WMODE_G)
#ifdef DOT11_N_SUPPORT
			|| WMODE_EQUAL(pAd->CommonCfg.PhyMode, WMODE_A | WMODE_B | WMODE_G | WMODE_AN | WMODE_GN)
#endif /* DOT11_N_SUPPORT */
#ifdef DOT11_VHT_AC
			|| WMODE_CAP(pAd->CommonCfg.PhyMode, WMODE_AC)
#endif /* DOT11_VHT_AC*/
		)
		{
			if (pAd->LatchRfRegs.Channel > 14)
			{
				pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
				pAd->CommonCfg.MlmeTransmit.field.MCS = MCS_RATE_6;
			}
			else
			{
				pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_CCK;
				pAd->CommonCfg.MlmeTransmit.field.MCS = MCS_0;
			}
		}
	}
#endif /* CONFIG_STA_SUPPORT */

#ifdef P2P_SUPPORT
	if (P2P_INF_ON(pAd))
	{
		if (MAC_ADDR_EQUAL(pHeader_802_11->Addr2, pAd->P2pCfg.CurrentAddress) ||
			(pAd->LatchRfRegs.Channel > 14))
		{
			pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
			pAd->CommonCfg.MlmeTransmit.field.MCS = MCS_RATE_6;
		}
		else
		{
			pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_CCK;
			pAd->CommonCfg.MlmeTransmit.field.MCS = MCS_0;
		}
	}
#endif /* P2P_SUPPORT */

#ifdef RT_CFG80211_P2P_SUPPORT
	/* make sure that p2p does not use cck rate */
	mgmt = (struct ieee80211_mgmt *)pHeader_802_11;
	if (ieee80211_is_action(mgmt->frame_control)) {
		PP2P_PUBLIC_FRAME pP2PFrame = (PP2P_PUBLIC_FRAME) pHeader_802_11;

		if ((pP2PFrame->p80211Header.FC.SubType == SUBTYPE_ACTION) &&
			(pP2PFrame->Category == CATEGORY_PUBLIC) &&
			(pP2PFrame->Action == ACTION_WIFI_DIRECT))
			is_P2P_action_frame = TRUE;
	}

	if ((pAd->CommonCfg.MlmeTransmit.field.MODE == MODE_CCK) &&
		(pAd->ScanCtrl.ScanType == SCAN_P2P ||
		(is_P2P_action_frame))) {
		pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
		pAd->CommonCfg.MlmeTransmit.field.MCS = MCS_RATE_6;
	}
#endif /* RT_CFG80211_SUPPORT */

	/*
		Should not be hard code to set PwrMgmt to 0 (PWR_ACTIVE)
		Snice it's been set to 0 while on MgtMacHeaderInit
		By the way this will cause frame to be send on PWR_SAVE failed.
	*/

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
		{
			/* We are in scan progress, just let the PwrMgmt bit keep as it orginally should be.*/
		}
		else
#endif /* CONFIG_STA_SUPPORT */
			pHeader_802_11->FC.PwrMgmt = PWR_ACTIVE;
#ifdef CONFIG_STA_SUPPORT
	}
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	/*
		In WMM-UAPSD, mlme frame should be set psm as power saving but probe
		request frame, Data-Null packets alse pass through MMRequest in RT2860,
		however, we hope control the psm bit to pass APSD
	*/
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if ((pHeader_802_11->FC.SubType == SUBTYPE_ACTION) ||
			(pHeader_802_11->FC.SubType == SUBTYPE_PS_POLL) ||
			((pHeader_802_11->FC.Type == FC_TYPE_DATA) &&
			((pHeader_802_11->FC.SubType == SUBTYPE_QOS_NULL) ||
			(pHeader_802_11->FC.SubType == SUBTYPE_DATA_NULL))))
		{
			if (RtmpPktPmBitCheck(pAd) == TRUE)
				pHeader_802_11->FC.PwrMgmt = PWR_SAVE;
			else if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED) &&
					INFRA_ON(pAd) &&
					RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
			{
				/* We are in scan progress, just let the PwrMgmt bit keep as it orginally should be */
			}
			else
			{
				pHeader_802_11->FC.PwrMgmt = pAd->CommonCfg.bAPSDForcePowerSave;
			}
#ifdef MT_MAC
			if (pAd->chipCap.hif_type == HIF_MT)
			{
#if defined(CONFIG_STA_SUPPORT) && defined(CONFIG_PM_BIT_HW_MODE)
				/* For  MT STA LP control, use H/W control mode for PM bit */
				mac_info.PsmBySw = 0;
#else
				mac_info.PsmBySw = 1;
#endif /* CONFIG_STA_SUPPORT && CONFIG_PM_BIT_HW_MODE */
			}
#endif /* MT_MAC */
		}
	}
#endif /* CONFIG_STA_SUPPORT */


#ifdef CONFIG_AP_SUPPORT
	pHeader_802_11->FC.MoreData = RTMP_GET_PACKET_MOREDATA(pPacket);
#endif /* CONFIG_AP_SUPPORT */


	bInsertTimestamp = FALSE;
	if (pHeader_802_11->FC.Type == FC_TYPE_CNTL) /* must be PS-POLL*/
	{
#ifdef CONFIG_STA_SUPPORT
		/*Set PM bit in ps-poll, to fix WLK 1.2  PowerSaveMode_ext failure issue.*/
		if ((pAd->OpMode == OPMODE_STA) && (pHeader_802_11->FC.SubType == SUBTYPE_PS_POLL))
		{
			pHeader_802_11->FC.PwrMgmt = PWR_SAVE;
		}
#endif /* CONFIG_STA_SUPPORT */
		bAckRequired = FALSE;

        if (pHeader_802_11->FC.SubType == SUBTYPE_BLOCK_ACK_REQ)
        {
            pBar = (PFRAME_BAR)(pSrcBufVA + tx_hw_hdr_len);
            bAckRequired = TRUE;
        }

#ifdef VHT_TXBF_SUPPORT
		if (pHeader_802_11->FC.SubType == SUBTYPE_VHT_NDPA)
		{
#if 0 // HsuChu's suggestion
			pHeader_802_11->Duration = RTMPCalcDuration(pAd, MlmeRate, SrcBufLen);
#else
			pHeader_802_11->Duration = 100;
#endif
			//DBGPRINT(RT_DEBUG_OFF, ("%s(): VHT_NDPA frame, rate=%d, len=%d, duration=%d\n",
			//			__FUNCTION__, MlmeRate, SrcBufLen, pHeader_802_11->Duration));
			//hex_dump("VHT_NDPA after update Duration", (UCHAR *)pHeader_802_11, SrcBufLen);
		}
#endif /* VHT_TXBF_SUPPORT */
	}
	else /* FC_TYPE_MGMT or FC_TYPE_DATA(must be NULL frame)*/
	{
		if (pHeader_802_11->Addr1[0] & 0x01) /* MULTICAST, BROADCAST*/
		{
			bAckRequired = FALSE;
			pHeader_802_11->Duration = 0;
		}
		else
		{
#ifdef SOFT_SOUNDING
			if (((pHeader_802_11->FC.Type == FC_TYPE_DATA) && (pHeader_802_11->FC.SubType == SUBTYPE_QOS_NULL))
				&& pMacEntry && (pMacEntry->snd_reqired == TRUE))
			{
				bAckRequired = FALSE;
				pHeader_802_11->Duration = 0;
			}
			else
#endif /* SOFT_SOUNDING */
			{
				bAckRequired = TRUE;
				pHeader_802_11->Duration = RTMPCalcDuration(pAd, MlmeRate, 14);
				if ((pHeader_802_11->FC.SubType == SUBTYPE_PROBE_RSP) && (pHeader_802_11->FC.Type == FC_TYPE_MGMT))
				{
					bInsertTimestamp = TRUE;
					bAckRequired = FALSE; /* Disable ACK to prevent retry 0x1f for Probe Response*/
#ifdef CONFIG_AP_SUPPORT
#ifdef SPECIFIC_TX_POWER_SUPPORT
					/* Find which MBSSID to be send this probeRsp */
					UINT32 apidx = get_apidx_by_addr(pAd, pHeader_802_11->Addr2);

					if ( !(apidx >= pAd->ApCfg.BssidNum) &&
					     (pAd->ApCfg.MBSSID[apidx].TxPwrAdj != -1) &&
					     (pAd->CommonCfg.MlmeTransmit.field.MODE == MODE_CCK) &&
					     (pAd->CommonCfg.MlmeTransmit.field.MCS == RATE_1))
					{
						TxPwrAdj = pAd->ApCfg.MBSSID[apidx].TxPwrAdj;
					}
#endif /* SPECIFIC_TX_POWER_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
				}
				else if ((pHeader_802_11->FC.SubType == SUBTYPE_PROBE_REQ) && (pHeader_802_11->FC.Type == FC_TYPE_MGMT))
				{
					bAckRequired = FALSE; /* Disable ACK to prevent retry 0x1f for Probe Request*/
				}
				else if ((pHeader_802_11->FC.SubType == SUBTYPE_DEAUTH) &&
						 (MacTableLookup(pAd, pHeader_802_11->Addr1) == NULL))
				{
					bAckRequired = FALSE; /* Disable ACK to prevent retry 0x1f for Deauth */
				}
			}
		}
	}

	pHeader_802_11->Sequence = pAd->Sequence++;
	if (pAd->Sequence >0xfff)
		pAd->Sequence = 0;

	/*
		Before radar detection done, mgmt frame can not be sent but probe req
		Because we need to use probe req to trigger driver to send probe req in passive scan
	*/
	if ((pHeader_802_11->FC.SubType != SUBTYPE_PROBE_REQ)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& (pAd->Dot11_H.RDMode != RD_NORMAL_MODE))
	{
		RTMP_IRQ_UNLOCK(&pAd->MgmtRingLock, Flags);
		return NDIS_STATUS_FAILURE;
	}

	/*
		fill scatter-and-gather buffer list into TXD. Internally created NDIS PACKET
		should always has only one physical buffer, and the whole frame size equals
		to the first scatter buffer size

		Initialize TX Descriptor
		For inter-frame gap, the number is for this frame and next frame
		For MLME rate, we will fix as 2Mb to match other vendor's implement

		management frame doesn't need encryption.
		so use RESERVED_WCID no matter u are sending to specific wcid or not
	*/
	PID = PID_MGMT;

#ifdef WMM_ACM_SUPPORT
	if (ACMR_PKT_QOS_TYPE_GET(pPacket) == ACM_QOS_TYPE_ADDTS_RSP)
	{
		/* no any empty field can be used in TxWI, nothing to do */
	}
#endif /* WMM_ACM_SUPPORT */

	if (pMacEntry == NULL)
	{
		wcid = RESERVED_WCID;
		// TODO: shiang-7603
		if (IS_MT7603(pAd) || IS_MT7628(pAd))
		{
			wcid = 0;
#if 1
			if(prot)
			{
				MAC_TABLE_ENTRY *pEntry = NULL;
				
				pEntry = MacTableLookup(pAd, pHeader_802_11->Addr1);

				if (pEntry)
					wcid = (UCHAR)pEntry->Aid;
			}
#endif
		}
		tx_rate = (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS;
		transmit = &pAd->CommonCfg.MlmeTransmit;
#ifdef VHT_TXBF_SUPPORT
		if (pAd->NDPA_Request)
		{
			transmit->field.MODE = MODE_VHT;
			transmit->field.MCS = MCS_RATE_6;
		}
#endif
	}
	else
	{
#if defined(P2P_SUPPORT) || defined(RT_CFG80211_P2P_SUPPORT)
		/* P2P Test Case 6.1.12, only OFDM rate can be captured by sniffer */
		if(
#ifdef P2P_SUPPORT
		   (pAd->P2pCfg.bLowRateQoSNULL == TRUE) && 
#endif /* P2P_SUPPORT */
#ifdef RT_CFG80211_P2P_SUPPORT
		   (CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_APSD_CAPABLE)) &&	
#endif /* RT_CFG80211_P2P_SUPPORT */
		   ((pHeader_802_11->FC.Type == FC_TYPE_DATA) &&
		   (pHeader_802_11->FC.SubType == SUBTYPE_QOS_NULL)))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s:: Using Low Rate to send QOS NULL!!\n", __FUNCTION__));
			pMacEntry->MaxHTPhyMode.field.MODE = 1;
			pMacEntry->MaxHTPhyMode.field.MCS = MCS_RATE_6;
		}
#endif /* P2P_SUPPORT || RT_CFG80211_SUPPORT */

		wcid = pMacEntry->wcid;
		tx_rate = (UCHAR)pMacEntry->MaxHTPhyMode.field.MCS;
		transmit = &pMacEntry->MaxHTPhyMode;
		/* dont use low rate to send QoS Null data frame */
	}

	if(prot)
		mac_info.prot = prot;

	if (prot == 2)
		mac_info.bss_idx = apidx;

	mac_info.FRAG = FALSE;

	mac_info.CFACK = FALSE;
	mac_info.InsTimestamp = bInsertTimestamp;
	mac_info.AMPDU = FALSE;

	mac_info.Ack = bAckRequired;
	mac_info.BM = IS_BM_MAC_ADDR(pHeader_802_11->Addr1);
	mac_info.NSeq = FALSE;
	mac_info.BASize = 0;

	mac_info.WCID = wcid;
	mac_info.TID = 0;
#if defined(MT7603) || defined(MT7628)
	// TODO: shiang-MT7603
	mac_info.Length = (SrcBufLen - tx_hw_hdr_len);
	if (pHeader_802_11->FC.Type == FC_TYPE_MGMT) {
		mac_info.hdr_len = 24;
		if (pHeader_802_11->FC.Order == 1)
			mac_info.hdr_len += 4;
	} else if (pHeader_802_11->FC.Type == FC_TYPE_DATA) {
		switch (pHeader_802_11->FC.SubType) {
			case SUBTYPE_DATA_NULL:
				mac_info.hdr_len = 24;
#ifdef RELEASE_EXCLUDE
        /*
        *   HEC# WHQA_0017099
        *   When ERP protection enable, it will use CCK to protect and send out 20Mhz pkt.
        *   if the pkt is use 40Mhz and Fixed BW, it will stuck in PSE and cannot be sendout.
        *   the below patch will be removed after MT7603E2
        */
#endif /* RELEASE_EXCLUDE */
				tx_rate = (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS;
				transmit = &pAd->CommonCfg.MlmeTransmit;
				break;
			case SUBTYPE_QOS_NULL:
				mac_info.hdr_len = 26;
#ifdef RELEASE_EXCLUDE
        /*
        *   HEC# WHQA_0017099
        *   When ERP protection enable, it will use CCK to protect and send out 20Mhz pkt.
        *   if the pkt is use 40Mhz and Fixed BW, it will stuck in PSE and cannot be sendout.
        *   the below patch will be removed after MT7603E2
        */
#endif /* RELEASE_EXCLUDE */
				tx_rate = (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS;
				transmit = &pAd->CommonCfg.MlmeTransmit;
				break;
			default:
				{
					DBGPRINT(RT_DEBUG_ERROR, ("%s(): FIXME!!!Unexpected frame(Type=%d, SubType=%d) send to MgmtRing, need to assign the length!\n",
						__FUNCTION__, pHeader_802_11->FC.Type, pHeader_802_11->FC.SubType));
					hex_dump("DataFrame", (char *)pHeader_802_11, 24);
				}
				break;
		}

		if (pMacEntry && pAd->MacTab.tr_entry[wcid].PsDeQWaitCnt)
			PID = PID_PS_DATA;

		mac_info.WCID = wcid;
	} 
    else if (pHeader_802_11->FC.Type == FC_TYPE_CNTL) 
    {
        switch (pHeader_802_11->FC.SubType) 
        {
            case SUBTYPE_BLOCK_ACK_REQ:
                PID = PID_CTL_BAR;
                mac_info.hdr_len = 16;
                mac_info.SpeEn = 0; 
		mac_info.TID = (UCHAR)pBar->BarControl.TID;
                if (pAd->CommonCfg.Channel > 14)
                { /* 2.4G */
                    TransmitSetting.field.MODE = MODE_OFDM;
                }    
                else 
                { /* 5G */
                    TransmitSetting.field.MODE = MODE_CCK;
                }    
                TransmitSetting.field.BW = BW_20;
                TransmitSetting.field.STBC = 0; 
                TransmitSetting.field.ShortGI = 0; 
                TransmitSetting.field.MCS = 0; 
                TransmitSetting.field.ldpc = 0; 
                transmit = &TransmitSetting;
                break;
		case SUBTYPE_PS_POLL:
			mac_info.hdr_len = sizeof (PSPOLL_FRAME);
#ifdef RELEASE_EXCLUDE
        /*
        *   HEC# WHQA_0017099
        *   When ERP protection enable, it will use CCK to protect and send out 20Mhz pkt.
        *   if the pkt is use 40Mhz and Fixed BW, it will stuck in PSE and cannot be sendout.
        *   the below patch will be removed after MT7603E2
        */
#endif /* RELEASE_EXCLUDE */
			tx_rate = (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS;
			transmit = &pAd->CommonCfg.MlmeTransmit;			
		break;

            default:
                DBGPRINT(RT_DEBUG_ERROR, ("%s(): FIXME!!!Unexpected frame(Type=%d, SubType=%d) send to MgmtRing, need to assign the length!\n",
                            __FUNCTION__, pHeader_802_11->FC.Type, pHeader_802_11->FC.SubType));
                break;
        }
    }
    else {
		DBGPRINT(RT_DEBUG_ERROR, ("%s(): FIXME!!!Unexpected frame send to MgmtRing, need to assign the length!\n",
					__FUNCTION__));
	}
#else
	mac_info.Length = (SrcBufLen - TXINFO_SIZE - pAd->chipCap.TXWISize - TSO_SIZE);
#endif /* MT7603 */
	mac_info.PID = PID;
	mac_info.TxRate = tx_rate;
	mac_info.SpeEn = 1;
	mac_info.Preamble = LONG_PREAMBLE;

#ifdef MT_MAC
	// TODO: shiang-MT7603
	mac_info.q_idx = Q_IDX_AC4;
#endif /* MT_MAC */

	/* PCI use Miniport to send NULL frame and need to add NULL frame TxS control here to enter PSM */
#ifdef CONFIG_STA_SUPPORT
#if defined(MT_MAC) && (defined(STA_LP_PHASE_1_SUPPORT) || defined(STA_LP_PHASE_2_SUPPORT))
		if ((pHeader_802_11->FC.SubType == SUBTYPE_DATA_NULL) || (pHeader_802_11->FC.SubType == SUBTYPE_QOS_NULL))
		{
			/* PCI use Miniport to send NULL frame and need to add NULL frame TxS control here to enter PSM */
			mac_info.PID = (pHeader_802_11->FC.PwrMgmt == PWR_ACTIVE) ? PID_NULL_FRAME_PWR_ACTIVE : PID_NULL_FRAME_PWR_SAVE;
			
			if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) && RTMPEnterPsmNullBitStatus(&pAd->StaCfg.PwrMgmt))
			{
				DBGPRINT(RT_DEBUG_TRACE, ("%s(line=%d)\n", __FUNCTION__, __LINE__));
				RTMPClearEnterPsmNullBit(&pAd->StaCfg.PwrMgmt);
				TxSTypeCtlPerPkt(pAd, mac_info.PID, TXS_FORMAT0, FALSE, TRUE, FALSE, TXS_DUMP_REPEAT); 
			}
			else
			{
				DBGPRINT(RT_DEBUG_TRACE, ("%s(line=%d)\n", __FUNCTION__, __LINE__));
				TxSTypeCtlPerPkt(pAd, mac_info.PID, TXS_FORMAT0, FALSE, FALSE, FALSE, TXS_DUMP_REPEAT); 
			}
		}
#endif /* MT_MAC */
#endif /* CONFIG_STA_SUPPORT */
	DBGPRINT(RT_DEBUG_TRACE, ("%s(line=%d), mac_info.PsmBySw(%d)\n", __FUNCTION__, __LINE__, mac_info.PsmBySw));
	if(pCfg80211_ctrl->TxStatusInUsed && pCfg80211_ctrl->IsNeedTxStatus)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s %d, PID (%d)\n", __FUNCTION__, __LINE__, mac_info.PID));
		TxSTypeCtlPerPkt(pAd, mac_info.PID, TXS_FORMAT0, FALSE, TRUE, FALSE, TXS_DUMP_REPEAT);
	}
	else
		TxSTypeCtlPerPkt(pAd, mac_info.PID, TXS_FORMAT0, FALSE, FALSE, FALSE, TXS_DUMP_REPEAT);

#ifdef SOFT_SOUNDING
	if (((pHeader_802_11->FC.Type == FC_TYPE_DATA) && (pHeader_802_11->FC.SubType == SUBTYPE_QOS_NULL))
		&& pMacEntry && (pMacEntry->snd_reqired == TRUE))
	{
		wcid = RESERVED_WCID;
		tx_rate = (UCHAR)pMacEntry->snd_rate.field.MCS;
		transmit = &pMacEntry->snd_rate;

		mac_info.Txopmode = IFS_PIFS;

		write_tmac_info(pAd, tmac_info, &mac_info, transmit);

		pMacEntry->snd_reqired = FALSE;
		DBGPRINT(RT_DEBUG_OFF, ("%s():Kick Sounding to %02x:%02x:%02x:%02x:%02x:%02x, dataRate(PhyMode:%s, BW:%sHz, %dSS, MCS%d)\n",
					__FUNCTION__, PRINT_MAC(pMacEntry->Addr),
					get_phymode_str(transmit->field.MODE),
					get_bw_str(transmit->field.BW),
					(transmit->field.MCS>>4) + 1, (transmit->field.MCS & 0xf)));
	}
	else
#endif /* SOFT_SOUNDING */
	{
		mac_info.Txopmode = IFS_BACKOFF;

        write_tmac_info(pAd, tmac_info, &mac_info, transmit);

#ifdef SPECIFIC_TX_POWER_SUPPORT
#if defined(RTMP_MAC) || defined(RLT_MAC)
		if (pMacEntry == NULL) {
			TXWI_STRUC *pFirstTxWI = (TXWI_STRUC *)tmac_info;

#ifdef RTMP_MAC
			if (pAd->chipCap.hif_type == HIF_RTMP)
	        		pFirstTxWI->TXWI_O.TxPwrAdj = TxPwrAdj;
#endif /* RTMP_MAC */
#ifdef RLT_MAC
			if (pAd->chipCap.hif_type == HIF_RLT)
				pFirstTxWI->TXWI_N.TxPwrAdj = TxPwrAdj;
#endif /* RLT_MAC */
		}
#endif /* defined(RTMP_MAC) || defined(RLT_MAC) */
#endif /* SPECIFIC_TX_POWER_SUPPORT */
	}

//+++Add by shiang for debug
#if 0 //defined(MT7603) || defined(MT7628)
// TODO: shiang-7603
	hex_dump("MgmtFrame", pSrcBufVA, SrcBufLen);
	hex_dump("TMAC_Info", tmac_info, sizeof(TMAC_TXD_L));
#endif /* MT7603 */

//---Add by shiang for debug
//+++Add by shiang for debug
if ((pHeader_802_11->FC.Type == FC_TYPE_CNTL) && (pHeader_802_11->FC.SubType == SUBTYPE_VHT_NDPA))
{
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): Send VhtNDPA to peer(wcid=%d, pMacEntry=%p) with Mode=%d, txRate=%d, BW=%d\n",
				__FUNCTION__, wcid, pMacEntry, transmit->field.MODE, tx_rate, transmit->field.BW));
	hex_dump("VHT_NDPA raw data", pSrcBufVA, SrcBufLen);
	dump_tmac_info(pAd, tmac_info);
}
//---Add by shiang for debug

#ifdef RT_BIG_ENDIAN
	RTMPFrameEndianChange(pAd, (PUCHAR)pHeader_802_11, DIR_WRITE, FALSE);
#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT)
	{
		MTMacInfoEndianChange(pAd, tmac_info, TYPE_TMACINFO, sizeof(TMAC_TXD_L));
	}
#endif
#if defined(RTMP_MAC) || defined(RLT_MAC)
	if (pAd->chipCap.hif_type == HIF_RTMP || pAd->chipCap.hif_type == HIF_RLT)
	{
		RTMPWIEndianChange(pAd, tmac_info, TYPE_TXWI);
	}
#endif
#endif

#ifdef CONFIG_TSO_SUPPORT
	// TODO: shiang-6590, for mgmt frame, we don't need to fill any fields of TSO_INFO
	//rlt_tso_info_write(pAd, tinfo, TX_BLK * pTxBlk);
#endif /* CONFIG_TSO_SUPPORT */

	/* Now do hardware-depened kick out.*/
    HAL_KickOutMgmtTx(pAd, mac_info.q_idx, pPacket, pSrcBufVA, SrcBufLen);

#ifdef CONFIG_HOTSPOT_R2
	if (RTMP_GET_PACKET_DISASSOC(pPacket))
	{
		if (((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
	        (pHeader_802_11->FC.SubType == SUBTYPE_DISASSOC)) ||
			((pHeader_802_11->FC.Type == FC_TYPE_MGMT) &&
            (pHeader_802_11->FC.SubType == SUBTYPE_DEAUTH)))
	    {
    	    pMacEntry = MacTableLookup(pAd, pHeader_802_11->Addr1);
    	}
		if ((pMacEntry) && (pMacEntry->IsKeep == 1))
			MacTableDeleteEntry(pAd, pMacEntry->Aid, pMacEntry->Addr);	
	}
#endif /* CONFIG_HOTSPOT_R2 */

	/* Make sure to release MGMT ring resource*/
/*	if (!IrqState)*/
	RTMP_IRQ_UNLOCK(&pAd->MgmtRingLock, Flags);
	return NDIS_STATUS_SUCCESS;
}


/********************************************************************************

	New DeQueue Procedures.

 ********************************************************************************/
#define DEQUEUE_LOCK(lock, bIntContext, IrqFlags) 				\
			do{													\
				if (bIntContext == FALSE)						\
				RTMP_IRQ_LOCK((lock), IrqFlags);		\
			}while(0)

#define DEQUEUE_UNLOCK(lock, bIntContext, IrqFlags)				\
			do{													\
				if (bIntContext == FALSE)						\
					RTMP_IRQ_UNLOCK((lock), IrqFlags);	\
			}while(0)


/*
	========================================================================
	Tx Path design algorithm:
		Basically, we divide the packets into four types, Broadcast/Multicast, 11N Rate(AMPDU, AMSDU, Normal), B/G Rate(ARALINK, Normal),
		Specific Packet Type. Following show the classification rule and policy for each kinds of packets.
				Classification Rule=>
					Multicast: (*addr1 & 0x01) == 0x01
					Specific : bDHCPFrame, bARPFrame, bEAPOLFrame, etc.
					11N Rate : If peer support HT
								(1).AMPDU  -- If TXBA is negotiated.
								(2).AMSDU  -- If AMSDU is capable for both peer and ourself.
											*). AMSDU can embedded in a AMPDU, but now we didn't support it.
								(3).Normal -- Other packets which send as 11n rate.

					B/G Rate : If peer is b/g only.
								(1).ARALINK-- If both of peer/us supprot Ralink proprietary Aggregation and the TxRate is large than RATE_6
								(2).Normal -- Other packets which send as b/g rate.
					Fragment:
								The packet must be unicast, NOT A-RALINK, NOT A-MSDU, NOT 11n, then can consider about fragment.

				Classified Packet Handle Rule=>
					Multicast:
								No ACK, 		pTxBlk->bAckRequired = FALSE;
								No WMM, 		pTxBlk->bWMM = FALSE;
								No piggyback,   pTxBlk->bPiggyBack = FALSE;
								Force LowRate,  pTxBlk->bForceLowRate = TRUE;
					Specific :	Basically, for specific packet, we should handle it specifically, but now all specific packets are use
									the same policy to handle it.
								Force LowRate,  pTxBlk->bForceLowRate = TRUE;

					11N Rate :
								No piggyback,	pTxBlk->bPiggyBack = FALSE;

								(1).AMSDU
									pTxBlk->bWMM = TRUE;
								(2).AMPDU
									pTxBlk->bWMM = TRUE;
								(3).Normal

					B/G Rate :
								(1).ARALINK

								(2).Normal
	========================================================================
*/
static UCHAR TxPktClassification(RTMP_ADAPTER *pAd, PNDIS_PACKET  pPacket, TX_BLK *pTxBlk)
{
	UCHAR TxFrameType = TX_LEGACY_FRAME;
	UCHAR Wcid;
	MAC_TABLE_ENTRY *pMacEntry = NULL;
	STA_TR_ENTRY *tr_entry = NULL;

	if (RTMP_GET_PACKET_TXTYPE(pPacket) == TX_MCAST_FRAME)
		return TX_MCAST_FRAME;

	/* Handle for unicast packets */
	Wcid = RTMP_GET_PACKET_WCID(pPacket);
	tr_entry = &pAd->MacTab.tr_entry[Wcid];
	if (Wcid < MAX_LEN_OF_MAC_TABLE)
		pMacEntry = &pAd->MacTab.Content[Wcid];
	else
		pMacEntry = &pAd->MacTab.Content[MCAST_WCID];

	pTxBlk->wdev = tr_entry->wdev;

	/* It's a specific packet need to force low rate, i.e., bDHCPFrame, bEAPOLFrame, bWAIFrame*/
	if ((RTMP_GET_PACKET_TXTYPE(pPacket) == TX_LEGACY_FRAME) || (pMacEntry->PsMode == PWR_SAVE))
		TxFrameType = TX_LEGACY_FRAME;
#ifdef DOT11_N_SUPPORT
	else if (IS_HT_RATE(pMacEntry))
	{

#ifdef VHT_TXBF_SUPPORT
		// TODO: shiang-usw, we should use cb here instead of mark the data type!
		if (pMacEntry->TxSndgType == SNDG_TYPE_NDP && IS_VHT_RATE(pMacEntry))
		{
#error
			TxFrameType = TX_NDPA_FRAME;
			DBGPRINT(RT_DEBUG_OFF, ("%s():Err!! fix me for this!!\n", __FUNCTION__));
		}
#endif

		if (RTMP_GET_PACKET_MOREDATA(pPacket) || (pMacEntry->PsMode == PWR_SAVE))
			TxFrameType = TX_LEGACY_FRAME;
		else
#ifdef UAPSD_SUPPORT
		if (RTMP_GET_PACKET_EOSP(pPacket))
			TxFrameType = TX_LEGACY_FRAME;
		else
#endif /* UAPSD_SUPPORT */
#ifdef WFA_VHT_PF
		if (pAd->force_amsdu == TRUE)
			return TX_AMSDU_FRAME;
		else
#endif /* WFA_VHT_PF */
		if ((pMacEntry->TXBAbitmap & (1<<(RTMP_GET_PACKET_UP(pPacket)))) != 0)
			return TX_AMPDU_FRAME;
		else if(CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AMSDU_INUSED)
#ifdef CONFIG_TSO_SUPPORT
			&& (!(RTMP_GET_PKT_CSO(pPacket) || RTMP_GET_PKT_TSO(pPacket)))
#endif /* CONFIG_TSO_SUPPORT */
		)
			return TX_AMSDU_FRAME;
	}
#endif /* DOT11_N_SUPPORT */
	else
	{	/* it's a legacy b/g packet.*/
		if ((CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE) && pAd->CommonCfg.bAggregationCapable) &&
			(RTMP_GET_PACKET_TXRATE(pPacket) >= RATE_6) &&
			(!(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE)))
#ifdef CONFIG_TSO_SUPPORT
			&& (!(RTMP_GET_PKT_CSO(pPacket) || RTMP_GET_PKT_TSO(pPacket)))
#endif /* CONFIG_TSO_SUPPORT */
		)
		{
			TxFrameType = TX_RALINK_FRAME;
		}
	}

#ifdef RELEASE_EXCLUDE
		/* 	IOT issue with Intel WiFi-Link 5100/5300 (v.12.2.0.11 - 2008/11/17) :
				When AP set fragment threshold as 256 in HT-mode, the intel STA can't
				complete EAP-certification with RADIUS server.

			[solution] :
				After the TX BA is built, the frame shall not be fragmented even if none-AMPDU frame.
				Add new condition to fix this issue
					((pMacEntry->TXBAbitmap & (1<<(RTMP_GET_PACKET_UP(pPacket)))) == 0)

			Currently, our fragment only support when a unicast packet send as:
				NOT-ARALINK, NOT-AMSDU and NOT-AMPDU
		 */
#endif /* RELEASE_EXCLUDE */
	if ((RTMP_GET_PACKET_FRAGMENTS(pPacket) > 1)
		 && (TxFrameType == TX_LEGACY_FRAME)
#ifdef DOT11_N_SUPPORT
		&& ((pMacEntry->TXBAbitmap & (1<<(RTMP_GET_PACKET_UP(pPacket)))) == 0)
#endif /* DOT11_N_SUPPORT */
		)
		TxFrameType = TX_FRAG_FRAME;

	return TxFrameType;
}


BOOLEAN RTMP_FillTxBlkInfo(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk)
{
	PACKET_INFO PacketInfo;
	PNDIS_PACKET pPacket;
	MAC_TABLE_ENTRY *pMacEntry = NULL;

	pPacket = pTxBlk->pPacket;
	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pTxBlk->pSrcBufHeader, &pTxBlk->SrcBufLen);
#ifdef TX_PKT_SG
	NdisMoveMemory( &pTxBlk->pkt_info, &PacketInfo, sizeof(PacketInfo));
#endif /* TX_PKT_SG */
	pTxBlk->Wcid = RTMP_GET_PACKET_WCID(pPacket);
	pTxBlk->wdev_idx = RTMP_GET_PACKET_WDEV(pPacket);

	pTxBlk->UserPriority = RTMP_GET_PACKET_UP(pPacket);
	pTxBlk->FrameGap = IFS_HTTXOP;
#ifdef CONFIG_AP_SUPPORT
	pTxBlk->pMbss = NULL;
#endif /* CONFIG_AP_SUPPORT */

	if (RTMP_GET_PACKET_CLEAR_EAP_FRAME(pTxBlk->pPacket))
		TX_BLK_SET_FLAG(pTxBlk, fTX_bClearEAPFrame);
	else
		TX_BLK_CLEAR_FLAG(pTxBlk, fTX_bClearEAPFrame);

#ifdef WAPI_SUPPORT
	/* Check if this is an WPI data frame*/
	if ((RTMPIsWapiCipher(pAd, pTxBlk->wdev_idx) == TRUE) &&
		 (RTMP_GET_PACKET_WAI(pTxBlk->pPacket) == FALSE))
		TX_BLK_SET_FLAG(pTxBlk, fTX_bWPIDataFrame);
	else
		TX_BLK_CLEAR_FLAG(pTxBlk, fTX_bWPIDataFrame);
#endif /* WAPI_SUPPORT */

	if (pTxBlk->tr_entry->EntryType == ENTRY_CAT_MCAST)
	{
		pTxBlk->pMacEntry = NULL;
		TX_BLK_SET_FLAG(pTxBlk, fTX_ForceRate);
		{
#ifdef MCAST_RATE_SPECIFIC
			PUCHAR pDA = GET_OS_PKT_DATAPTR(pPacket);
			if (((*pDA & 0x01) == 0x01) && (*pDA != 0xff))
				pTxBlk->pTransmit = &pAd->CommonCfg.MCastPhyMode;
			else
#endif /* MCAST_RATE_SPECIFIC */
				pTxBlk->pTransmit = &pAd->MacTab.Content[MCAST_WCID].HTPhyMode;
		}

		/* AckRequired = FALSE, when broadcast packet in Adhoc mode.*/
		TX_BLK_CLEAR_FLAG(pTxBlk, (fTX_bAckRequired | fTX_bAllowFrag | fTX_bWMM));
		if (RTMP_GET_PACKET_MOREDATA(pPacket))
		{
			TX_BLK_SET_FLAG(pTxBlk, fTX_bMoreData);
		}
#ifdef ETH_CONVERT_SUPPORT
		if (ADHOC_ON(pAd))
		{
			/* If we are running as Ethernet Converter, update MAT DataBase and duplicant the packet if necessary.*/
			if (pAd->EthConvert.ECMode & ETH_CONVERT_MODE_DONGLE)
			{
				PNDIS_PACKET donglePkt;

				if(!NdisEqualMemory(pAd->CurrentAddress, (GET_OS_PKT_DATAPTR(pPacket) + 6), MAC_ADDR_LEN))
				{
					TX_BLK_SET_FLAG(pTxBlk, fTX_bDonglePkt);
#ifdef RELEASE_EXCLUDE
					DBGPRINT(RT_DEBUG_INFO, ("@@ %s: Dongle Packet)\n", __FUNCTION__));
#endif /* RELEASE_EXCLUDE */
				}

				/* For each tx packet, update our MAT convert engine databases.*/
				donglePkt = (PNDIS_PACKET)MATEngineTxHandle(pAd, pPacket, 0, pTxBlk->OpMode);
				if(donglePkt)
				{
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
					pPacket = donglePkt;
					RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pTxBlk->pSrcBufHeader, &pTxBlk->SrcBufLen);
					pTxBlk->pPacket = donglePkt;
				}
			}
		}
#endif /* ETH_CONVERT_SUPPORT */
	}
	else
	{
		pTxBlk->pMacEntry = &pAd->MacTab.Content[pTxBlk->Wcid];
		pTxBlk->pTransmit = &pTxBlk->pMacEntry->HTPhyMode;

		pMacEntry = pTxBlk->pMacEntry;
#ifdef CONFIG_AP_SUPPORT
		pTxBlk->pMbss = pMacEntry->pMbss;
#endif /* CONFIG_AP_SUPPORT */

//YF MCC
#ifdef MULTI_WMM_SUPPORT
    	if (IS_ENTRY_APCLI(pMacEntry))
    	{
			pTxBlk->QueIdx = EDCA_WMM1_AC0_PIPE;
			printk("%s(): QIdx %d\n", __FUNCTION__, pTxBlk->QueIdx);
        }
#endif /* MULTI_WMM_SUPPORT */

		/* For all unicast packets, need Ack unless the Ack Policy is not set as NORMAL_ACK.*/
#ifdef MULTI_WMM_SUPPORT
		if (pTxBlk->QueIdx >= EDCA_WMM1_AC0_PIPE)
		{
			if (pAd->CommonCfg.AckPolicy[pTxBlk->QueIdx - EDCA_WMM1_AC0_PIPE] != NORMAL_ACK)
				TX_BLK_CLEAR_FLAG(pTxBlk, fTX_bAckRequired);
			else
				TX_BLK_SET_FLAG(pTxBlk, fTX_bAckRequired);
		}
		else
#endif /* MULTI_WMM_SUPPORT */
        {
		if (pAd->CommonCfg.AckPolicy[pTxBlk->QueIdx] != NORMAL_ACK)
			TX_BLK_CLEAR_FLAG(pTxBlk, fTX_bAckRequired);
		else
			TX_BLK_SET_FLAG(pTxBlk, fTX_bAckRequired);
		}
#ifdef CONFIG_STA_SUPPORT
#ifdef XLINK_SUPPORT
		if ((pAd->OpMode == OPMODE_STA) &&
			(ADHOC_ON(pAd)) /*&&
			(RX_FILTER_TEST_FLAG(pAd, fRX_FILTER_ACCEPT_PROMISCUOUS))*/)
		{
			if (pAd->StaCfg.PSPXlink)
				TX_BLK_CLEAR_FLAG(pTxBlk, fTX_bAckRequired);
		}
#endif /* XLINK_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef MESH_SUPPORT
		if (IS_ENTRY_MESH(pMacEntry))
		{
			PUCHAR pSrcBufVA = GET_OS_PKT_DATAPTR(pPacket);

			TX_BLK_SET_FLAG(pTxBlk, fTX_bMeshEntry);

			if (RTMP_GET_MESH_SOURCE(pPacket) == MESH_PROXY)
			{
				UINT8 MeshRouteId = RTMP_GET_MESH_ROUTE_ID(pPacket);
				/* B/Mcast packet.*/
				if ((MeshRouteId == BMCAST_ROUTE_ID) && (*pSrcBufVA & 0x01))
					pTxBlk->pMeshDA = pSrcBufVA;	/* Mesh DA*/
				else
				{
					PUCHAR pMeshDA = PathRouteAddrSearch(pAd, MeshRouteId);
					if (pMeshDA)
						pTxBlk->pMeshDA = pMeshDA;	/* Mesh DA*/
					else
						return FALSE;
				}
			}
			else /* MESH_ORIGIN, MESH_FORWARD packet.*/
				pTxBlk->pMeshDA = pSrcBufVA;		/* Mesh DA*/

			/* If both of peer and us support WMM, enable it.*/
			if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))
				TX_BLK_SET_FLAG(pTxBlk, fTX_bWMM);
		}
		else
#endif /* MESH_SUPPORT */
		{
#ifdef CONFIG_AP_SUPPORT
#if defined(P2P_SUPPORT) || defined(RT_CFG80211_P2P_SUPPORT) || defined(CFG80211_MULTI_STA)
			if (pTxBlk->OpMode == OPMODE_AP)
#else
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
#endif /* P2P_SUPPORT || RT_CFG80211_P2P_SUPPORT || CFG80211_MULTI_STA */
			{
#ifdef WDS_SUPPORT
				if(IS_ENTRY_WDS(pMacEntry))
				{
					TX_BLK_SET_FLAG(pTxBlk, fTX_bWDSEntry);
				}
				else
#endif /* WDS_SUPPORT */
#ifdef APCLI_SUPPORT
				if(IS_ENTRY_APCLI(pMacEntry))
				{
#ifdef MAT_SUPPORT
					PNDIS_PACKET apCliPkt = NULL;

#ifdef MAC_REPEATER_SUPPORT
					if ((pMacEntry->bReptCli) && (pAd->ApCfg.bMACRepeaterEn))
					{
						UCHAR tmpIdx;

						pAd->MatCfg.bMACRepeaterEn = pAd->ApCfg.bMACRepeaterEn;
						if(pAd->ApCfg.MACRepeaterOuiMode != 1)
						{
							tmpIdx = (64 + (MAX_EXT_MAC_ADDR_SIZE * pMacEntry->func_tb_idx) + pMacEntry->MatchReptCliIdx);
							// TODO: shiang-lock, fix ME!
							apCliPkt = (PNDIS_PACKET)MATEngineTxHandle(pAd, pPacket, tmpIdx, pTxBlk->OpMode);
						}
					}
					else
#endif /* MAC_REPEATER_SUPPORT */
					{
						/* For each tx packet, update our MAT convert engine databases.*/
						/* CFG_TODO */
						apCliPkt = (PNDIS_PACKET)MATEngineTxHandle(pAd, pPacket, pMacEntry->func_tb_idx, pTxBlk->OpMode);
					}

					if(apCliPkt)
					{
						RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
						pPacket = apCliPkt;
						RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pTxBlk->pSrcBufHeader, &pTxBlk->SrcBufLen);
						pTxBlk->pPacket = apCliPkt;
					}
#endif /* MAT_SUPPORT */
					pTxBlk->pApCliEntry = &pAd->ApCfg.ApCliTab[pMacEntry->func_tb_idx];
					TX_BLK_SET_FLAG(pTxBlk, fTX_bApCliPacket);

#ifdef RELEASE_EXCLUDE
					DBGPRINT(RT_DEBUG_INFO, ("RTMP_FillTxBlkInfo: ApClient - Rxwcid(%d), wdev_idx(%d))\n",pTxBlk->Wcid, pTxBlk->wdev_idx));
#endif /* RELEASE_EXCLUDE */
				}
				else
#endif /* APCLI_SUPPORT */
#ifdef CLIENT_WDS
				if (IS_ENTRY_CLIWDS(pMacEntry))
				{
					PUCHAR pDA = GET_OS_PKT_DATAPTR(pPacket);
					PUCHAR pSA = GET_OS_PKT_DATAPTR(pPacket) + MAC_ADDR_LEN;
					if (((pMacEntry->func_tb_idx < MAX_MBSSID_NUM(pAd))
						&& !MAC_ADDR_EQUAL(pSA, pAd->ApCfg.MBSSID[pMacEntry->func_tb_idx].Bssid))
						|| !MAC_ADDR_EQUAL(pDA, pMacEntry->Addr)
						)
					{
						TX_BLK_SET_FLAG(pTxBlk, fTX_bClientWDSFrame);
					}
				}
				else
#endif /* CLIENT_WDS */
				if (IS_ENTRY_CLIENT(pMacEntry))
				{ }
				else
					return FALSE;

				/* If both of peer and us support WMM, enable it.*/
				if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))
					TX_BLK_SET_FLAG(pTxBlk, fTX_bWMM);
			}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
#if defined(P2P_SUPPORT) || defined(RT_CFG80211_P2P_SUPPORT) || defined(CFG80211_MULTI_STA)
			if (pTxBlk->OpMode == OPMODE_STA)
#else
			IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
#endif /* P2P_SUPPORT || RT_CFG80211_P2P_SUPPORT || CFG80211_MULTI_STA */
			{
#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
				if(IS_ENTRY_TDLS(pMacEntry))
				{
					TX_BLK_SET_FLAG(pTxBlk, fTX_bTdlsEntry);
				}
#endif /* DOT11Z_TDLS_SUPPORT */

#ifdef ETH_CONVERT_SUPPORT
				/* If we are running as Ethernet Converter, update MAT DataBase and duplicant the packet if necessary.*/
				if (pAd->EthConvert.ECMode & ETH_CONVERT_MODE_DONGLE)
				{
					PNDIS_PACKET donglePkt;

					if(!NdisEqualMemory(pAd->CurrentAddress, (GET_OS_PKT_DATAPTR(pPacket) + 6), MAC_ADDR_LEN))
					{
						TX_BLK_SET_FLAG(pTxBlk, fTX_bDonglePkt);
#ifdef RELEASE_EXCLUDE
						DBGPRINT(RT_DEBUG_INFO, ("%s(): Dongle Packet)\n", __FUNCTION__));
#endif /* RELEASE_EXCLUDE */
					}

					/* For each tx packet, update our MAT convert engine databases.*/
					donglePkt = (PNDIS_PACKET)MATEngineTxHandle(pAd, pPacket, 0, pTxBlk->OpMode);
					if(donglePkt)
					{
						RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
						pPacket = donglePkt;
						RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pTxBlk->pSrcBufHeader, &pTxBlk->SrcBufLen);
						pTxBlk->pPacket = donglePkt;
					}
				}
#endif /* ETH_CONVERT_SUPPORT */

				/* If support WMM, enable it.*/
				if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) &&
					CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE))
					TX_BLK_SET_FLAG(pTxBlk, fTX_bWMM);
			}
#endif /* CONFIG_STA_SUPPORT */
		}

		if (pTxBlk->TxFrameType == TX_LEGACY_FRAME)
		{
			if ( ((RTMP_GET_PACKET_LOWRATE(pPacket))
#ifdef UAPSD_SUPPORT
				&& (!(pMacEntry && (pMacEntry->bAPSDFlagSPStart)))
#endif /* UAPSD_SUPPORT */
				) ||
				((pAd->OpMode == OPMODE_AP) && (pMacEntry->MaxHTPhyMode.field.MODE == MODE_CCK) && (pMacEntry->MaxHTPhyMode.field.MCS == RATE_1))
			)
			{	/* Specific packet, i.e., bDHCPFrame, bEAPOLFrame, bWAIFrame, need force low rate. */
				pTxBlk->pTransmit = &pAd->MacTab.Content[MCAST_WCID].HTPhyMode;
				TX_BLK_SET_FLAG(pTxBlk, fTX_ForceRate);

#ifdef WAPI_SUPPORT
				/* 	According to WAPIA certification description, WAI packets can not
					include QoS header */
				if (RTMP_GET_PACKET_WAI(pTxBlk->pPacket))
				{
					TX_BLK_CLEAR_FLAG(pTxBlk, fTX_bWMM);
				}
#endif /* WAPI_SUPPORT */
#ifdef DOT11_N_SUPPORT
				/* Modify the WMM bit for ICV issue. If we have a packet with EOSP field need to set as 1, how to handle it? */
				if (IS_HT_STA(pTxBlk->pMacEntry) &&
					(CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RALINK_CHIPSET)) &&
					((pAd->CommonCfg.bRdg == TRUE) && CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_RDG_CAPABLE)))
				{
					TX_BLK_CLEAR_FLAG(pTxBlk, fTX_bWMM);
				}
#endif /* DOT11_N_SUPPORT */
			}

#ifdef DOT11_N_SUPPORT
			if ( (IS_HT_RATE(pMacEntry) == FALSE) &&
				(CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE)))
			{	/* Currently piggy-back only support when peer is operate in b/g mode.*/
				TX_BLK_SET_FLAG(pTxBlk, fTX_bPiggyBack);
			}
#endif /* DOT11_N_SUPPORT */

			if (RTMP_GET_PACKET_MOREDATA(pPacket))
			{
				TX_BLK_SET_FLAG(pTxBlk, fTX_bMoreData);
			}
#ifdef UAPSD_SUPPORT
			if (RTMP_GET_PACKET_EOSP(pPacket))
			{
				TX_BLK_SET_FLAG(pTxBlk, fTX_bWMM_UAPSD_EOSP);
			}
#endif /* UAPSD_SUPPORT */
		}
		else if (pTxBlk->TxFrameType == TX_FRAG_FRAME)
		{
			TX_BLK_SET_FLAG(pTxBlk, fTX_bAllowFrag);
		}

		pMacEntry->DebugTxCount++;
	}

#ifdef CONFIG_TSO_SUPPORT
	if (RTMP_GET_PKT_CSO(pPacket) || RTMP_GET_PKT_TSO(pPacket))
	{
		TSO_INFO *info = &pTxBlk->tso_info;

		if (RTMP_GET_PKT_TSO(pPacket)) {
			DBGPRINT(RT_DEBUG_TRACE, ("%s(): Transmit a TSO/CSO frame:\n", __FUNCTION__));
			DBGPRINT(RT_DEBUG_TRACE, ("\tcso/tso=%d:%d, is_tcp=%d, ips=%d, tups=%d, mss=%d!\n",
					RTMP_GET_PKT_CSO(pPacket),
					RTMP_GET_PKT_TSO(pPacket),
					RTMP_GET_PKT_TCP(pPacket),
					RTMP_GET_PKT_IPS(pPacket),
					RTMP_GET_PKT_TUPS(pPacket),
					RTMP_GET_PKT_MSS(pPacket)));
		}
		pTxBlk->naf_type = RTMP_GET_PKT_NAF_TYPE(pPacket);
		info->ips = RTMP_GET_PKT_IPS(pPacket);
		info->tups = RTMP_GET_PKT_TUPS(pPacket);
		info->mss = RTMP_GET_PKT_MSS(pPacket);
		if (RTMP_GET_PKT_TCP(pPacket))
			info->tcp = 1;
	}
#endif /* CONFIG_TSO_SUPPORT */

	pAd->LastTxRate = (USHORT)pTxBlk->pTransmit->word;

	return TRUE;
}


BOOLEAN CanDoAggregateTransmit(RTMP_ADAPTER *pAd, NDIS_PACKET *pPacket, TX_BLK *pTxBlk)
{
	int minLen = LENGTH_802_3;

	/*DBGPRINT(RT_DEBUG_TRACE, ("Check if can do aggregation! TxFrameType=%d!\n", pTxBlk->TxFrameType));*/

	if (RTMP_GET_PACKET_WCID(pPacket) == MCAST_WCID)
		return FALSE;

	if (RTMP_GET_PACKET_DHCP(pPacket) ||
		RTMP_GET_PACKET_EAPOL(pPacket) ||
		RTMP_GET_PACKET_WAI(pPacket)
#ifdef CONFIG_TSO_SUPPORT
		|| (RTMP_GET_PKT_CSO(pPacket) || RTMP_GET_PKT_TSO(pPacket))
#endif /* CONFIG_TSO_SUPPORT */
	)
		return FALSE;

	/* Make sure the first packet has non-zero-length data payload */
	if (RTMP_GET_PACKET_VLAN(pPacket))
		minLen += LENGTH_802_1Q; /* VLAN tag */
	else if (RTMP_GET_PACKET_LLCSNAP(pPacket))
		minLen += 8; /* SNAP hdr Len*/
	if (minLen >= GET_OS_PKT_LEN(pPacket))
		return FALSE;

	if ((pTxBlk->TxFrameType == TX_AMSDU_FRAME) &&
		((pTxBlk->TotalFrameLen + GET_OS_PKT_LEN(pPacket))> (RX_BUFFER_AGGRESIZE - 100)))
	{	/* For AMSDU, allow the packets with total length < max-amsdu size*/
		return FALSE;
	}

	if ((pTxBlk->TxFrameType == TX_RALINK_FRAME) &&
		(pTxBlk->TxPacketList.Number == 2))
	{	/* For RALINK-Aggregation, allow two frames in one batch.*/
		return FALSE;
	}

#ifdef CONFIG_STA_SUPPORT
	if ((INFRA_ON(pAd)) && (pAd->OpMode == OPMODE_STA)) /* must be unicast to AP*/
		return TRUE;
	else
#endif /* CONFIG_STA_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
	/* CFG_TODO */
	if ((MAC_ADDR_EQUAL(GET_OS_PKT_DATAPTR(pTxBlk->pPacket), GET_OS_PKT_DATAPTR(pPacket))) 
	    && (pAd->OpMode == OPMODE_AP)) /* unicast to same STA*/
		return TRUE;
	else
#endif /* CONFIG_AP_SUPPORT */
		return FALSE;

}


VOID rtmp_sta_txq_dump(RTMP_ADAPTER *pAd, STA_TR_ENTRY *tr_entry, INT qidx)
{
	ULONG IrqFlags = 0;
	QUEUE_ENTRY *entry;

	RTMP_IRQ_LOCK(&pAd->irq_lock /*&tr_entry->txq_lock[qidx]*/, IrqFlags);

	entry = tr_entry->tx_queue[qidx].Head;
	DBGPRINT(RT_DEBUG_OFF, ("\nDump TxQ[%d] of TR_ENTRY(ID:%d, MAC:%02x:%02x:%02x:%02x:%02x:%02x): %s, enq_cap=%d\n",
				qidx, tr_entry->wcid, PRINT_MAC(tr_entry->Addr),
				entry == NULL ? "Empty" : "HasEntry", tr_entry->enq_cap));
	while(entry != NULL) {
		DBGPRINT(RT_DEBUG_OFF, (" 0x%p ", entry));
		entry = entry->Next;
		if (entry == NULL) {
			DBGPRINT(RT_DEBUG_OFF, ("\n"));
		}
	};

	RTMP_IRQ_UNLOCK(&pAd->irq_lock /* &tr_entry->txq_lock[qidx]*/, IrqFlags);
}


VOID rtmp_tx_swq_dump(RTMP_ADAPTER *pAd, INT qidx)
{
	ULONG IrqFlags = 0;
	INT deq_id, enq_id;

	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
	deq_id = pAd->tx_swq[qidx].deqIdx;
	enq_id = pAd->tx_swq[qidx].enqIdx;
	DBGPRINT(RT_DEBUG_OFF, ("\nDump TxSwQ[%d]: DeqIdx=%d, EnqIdx=%d, %s\n",
							qidx, deq_id, enq_id,
							(pAd->tx_swq[qidx].swq[deq_id] == 0 ? "Empty" : "HasEntry")));
	for (; deq_id != enq_id; (deq_id =  (deq_id == 511 ? 0 : deq_id+1)))
	{
		DBGPRINT(RT_DEBUG_OFF, (" %d ", pAd->tx_swq[qidx].swq[deq_id]));
	}
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
	DBGPRINT(RT_DEBUG_OFF, ("\n"));
}



VOID rtmp_ps_init(RTMP_ADAPTER *pAd)
{
	UCHAR i = 0;
	/*initial PS Token Queue*/	
	DlListInit(&pAd->psTokenQueue);

	for(i=0;i<MAX_LEN_OF_TR_TABLE;i++)
	{
		wcid_table[i].wcid = i;
	}

}

VOID rtmp_ps_listforAll(RTMP_ADAPTER *pAd)
{
	struct WCID_TABLE *pEntry= NULL;

	DBGPRINT(RT_DEBUG_OFF,("List:\t"));
	DlListForEach(pEntry,&pAd->psTokenQueue,struct WCID_TABLE,list)
	{
		DBGPRINT(RT_DEBUG_LOUD,("%d\t",pEntry->wcid));
	}
	DBGPRINT(RT_DEBUG_LOUD,("\n"));
}

INT rtmp_ps_enq(RTMP_ADAPTER *pAd, STA_TR_ENTRY *tr_entry)
{
	struct WCID_TABLE *pEntry;

	if(tr_entry->wcid  >= MAX_LEN_OF_TR_TABLE )
	{
		return TRUE;
	}

	pEntry = &wcid_table[tr_entry->wcid];
	
	if( tr_entry->PsTokenFlag != PS_TOKEN_STAT_WCID_PKT)
	{
		 tr_entry->PsTokenFlag  = PS_TOKEN_STAT_WCID_PKT;
		DlListAdd(&pAd->psTokenQueue,&pEntry->list);		
		DBGPRINT(RT_DEBUG_LOUD, ("enqueue PS fifo token to Token Queue, WCID: %d,%d.\n", tr_entry->wcid,pEntry->wcid));
		
	}else
	{
		DBGPRINT(RT_DEBUG_LOUD, ("not need enqueue WCID: %d.\n", tr_entry->wcid));
	}
	/*rtmp_ps_listforAll(pAd);*/
	return TRUE;
}

INT rtmp_psDeq_req(RTMP_ADAPTER *pAd)
{
	
	STA_TR_ENTRY *tr_entry = NULL;
	struct WCID_TABLE *psEntry = NULL;
	struct tx_swq_fifo *fifo_swq;
	
	INT32 cnt = 0, i = 0;
	INT32 capCount=0;
	/*remove first psToken now, should check WCID requeue when report*/
	do {
		psEntry = DlListFirst(&pAd->psTokenQueue,struct WCID_TABLE,list);
		
		if(psEntry==NULL)
		{
			break;
		}
		DlListDel(&psEntry->list);
		if( psEntry->wcid >=  MAX_LEN_OF_TR_TABLE)
		{
			DBGPRINT(RT_DEBUG_ERROR,("%s(): wcid >MAX_LEN_OF_TR_TABLE \n",__FUNCTION__));
			continue;
		}
		tr_entry = &pAd->MacTab.tr_entry[psEntry->wcid];
		
		if(!tr_entry)
		{
			DBGPRINT(RT_DEBUG_ERROR,("%s(): tr_entry is NULL \n",__FUNCTION__));
			continue;
		}

		if(tr_entry->enqCount <=0)
		{	
			tr_entry->PsTokenFlag = PS_TOKEN_STAT_IDLE;			
			DBGPRINT(RT_DEBUG_LOUD,("%s(): ps token flag =%d,wcid =%d\n",__FUNCTION__,tr_entry->PsTokenFlag,psEntry->wcid));
			continue;
		}		
		tr_entry->PsTokenFlag = PS_TOKEN_STAT_PKT;
		
		/*enqueue wcid to sw fifo queue*/
		fifo_swq = &pAd->tx_swq[QID_AC_BE];
		cnt = tr_entry->tx_queue[QID_AC_BE].Number ;		
		capCount = (fifo_swq->enqIdx >=fifo_swq->deqIdx) ? (TX_SWQ_FIFO_LEN-fifo_swq->enqIdx+fifo_swq->deqIdx) : (fifo_swq->deqIdx-fifo_swq->enqIdx); 		
		cnt = cnt > capCount ? capCount : cnt;
		for(i=0;i<cnt;i++)
		{
			/*check fifo is not full*/
			if ((fifo_swq->swq[fifo_swq->enqIdx] != 0)  || !(tr_entry->enq_cap))
			{			
				rtmp_ps_enq(pAd,tr_entry);
				DBGPRINT(RT_DEBUG_LOUD,("%s(): fifo is full,flag:%d,enqId:%d,deqId:%d, cap: %d \n",__FUNCTION__,tr_entry->PsTokenFlag,fifo_swq->enqIdx,fifo_swq->deqIdx,tr_entry->enq_cap));
				break;
			}

			fifo_swq->swq[fifo_swq->enqIdx] = tr_entry->wcid;
			INC_RING_INDEX(fifo_swq->enqIdx, TX_SWQ_FIFO_LEN);	
		}
	
		DBGPRINT(RT_DEBUG_LOUD,("%s(): deq  for PS retrieve and PSM change. WCID: %d,stat: %d\n",__FUNCTION__,tr_entry->wcid,tr_entry->PsTokenFlag));
		return TRUE;
	}while(TRUE);

	return TRUE;
}

INT rtmp_psDeq_report(RTMP_ADAPTER *pAd,struct dequeue_info *info)
{
	STA_TR_ENTRY *tr_entry = NULL;
	
	tr_entry = &pAd->MacTab.tr_entry[info->target_wcid];
	if(tr_entry->enqCount > 0  && tr_entry->PsTokenFlag == PS_TOKEN_STAT_PKT)
	{
		rtmp_ps_enq(pAd,tr_entry);
		DBGPRINT(RT_DEBUG_LOUD,("%s(): After DeqReport, re-enq PsToken, WCID: %d,STAT: %d\n",__FUNCTION__,info->target_wcid,tr_entry->PsTokenFlag));
	}else
	if(tr_entry->enqCount <= 0 && tr_entry->PsTokenFlag != PS_TOKEN_STAT_IDLE)
	{		
		tr_entry->PsTokenFlag = PS_TOKEN_STAT_IDLE;		
		DBGPRINT(RT_DEBUG_LOUD,("%s(): After DeqReport, Abort PsToken, WCID: %d,STAT: %d\n",__FUNCTION__,info->target_wcid,tr_entry->PsTokenFlag));
	}
	return TRUE;
}



INT rtmp_tx_swq_init(RTMP_ADAPTER *pAd)
{
	ULONG IrqFlags = 0;

	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
	NdisZeroMemory(pAd->tx_swq, sizeof(pAd->tx_swq));
	rtmp_ps_init(pAd);
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

	return TRUE;
}


INT rtmp_tx_swq_exit(RTMP_ADAPTER *pAd, UCHAR wcid)
{
	UCHAR ring_idx, wcid_start, wcid_end;
	STA_TR_ENTRY *tr_entry;
	ULONG IrqFlags = 0;
	PNDIS_PACKET pPacket;
	QUEUE_ENTRY *pEntry;
	QUEUE_HEADER *pQueue;

	if (wcid == WCID_ALL) {
		wcid_start = 0;
		wcid_end = MAX_LEN_OF_TR_TABLE;
	} else {
		if (wcid < MAX_LEN_OF_TR_TABLE)
			wcid_start = wcid_end = wcid;
		else
		{
			DBGPRINT(RT_DEBUG_OFF, ("%s():Invalid WCID[%d]\n",
						__FUNCTION__, wcid));
			return FALSE;
		}
	}

	for (wcid = wcid_start; wcid <= wcid_end; wcid++) {
		tr_entry = &pAd->MacTab.tr_entry[wcid];

		if (IS_ENTRY_NONE(tr_entry))
			continue;

		// TODO: shiang-usw, protect "tr_entry->enq_cap" here !!
		for (ring_idx = 0; ring_idx < WMM_QUE_NUM; ring_idx++)
		{
			RTMP_IRQ_LOCK(&pAd->irq_lock /* &tr_entry->txq_lock[ring_idx] */, IrqFlags);
			pQueue = &tr_entry->tx_queue[ring_idx];
			while (pQueue->Head)
			{
				pEntry = RemoveHeadQueue(pQueue);
				TR_ENQ_COUNT_DEC(tr_entry);
				pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
				if (pPacket)
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
#if 0 //def MT_PS
			pQueue = &tr_entry->mt_ps_queue[ring_idx];
			while (pQueue->Head)
			{
				pEntry = RemoveHeadQueue(pQueue);
				pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
				if (pPacket)
					RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
#endif /* MT_PS */
			RTMP_IRQ_UNLOCK(&pAd->irq_lock /* &tr_entry->txq_lock[ring_idx] */, IrqFlags);
		}

		RTMP_IRQ_LOCK(&pAd->irq_lock /* &tr_entry->ps_queue_lock */, IrqFlags);
		pQueue = &tr_entry->ps_queue;
		while (pQueue->Head)
		{
			pEntry = RemoveHeadQueue(pQueue);
			pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
			if (pPacket)
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		}
		RTMP_IRQ_UNLOCK(&pAd->irq_lock /*&tr_entry->ps_queue_lock*/, IrqFlags);
	}

	return TRUE;
}


INT rtmp_enq_req(RTMP_ADAPTER *pAd, PNDIS_PACKET pkt, UCHAR qidx, STA_TR_ENTRY *tr_entry, BOOLEAN FlgIsLocked,QUEUE_HEADER *pPktQueue)
{
	ULONG IrqFlags = 0;
	BOOLEAN enq_done = FALSE;
	INT enq_idx = 0;
	struct tx_swq_fifo *fifo_swq;
	UCHAR occupied_wcid = 0;
	QUEUE_ENTRY *pEntry;
	UINT capCount = 0;
	PNDIS_PACKET tmpPkt;

	ASSERT(qidx < WMM_QUE_NUM);
	ASSERT((tr_entry->wcid != 0));

	fifo_swq = &pAd->tx_swq[qidx];
	if (FlgIsLocked == FALSE)
		RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);

	if ((tr_entry->enqCount > SQ_ENQ_NORMAL_MAX)
		&&(tr_entry->tx_queue[qidx].Number > SQ_ENQ_RESERVE_PERAC))
	{
		occupied_wcid = fifo_swq->swq[enq_idx];
		enq_done = FALSE;
		goto enq_end;
	}

	if(pkt)
	{
	enq_idx = fifo_swq->enqIdx;
		if ((fifo_swq->swq[enq_idx] == 0) && (tr_entry->enq_cap))
		{
		InsertTailQueueAc(pAd, tr_entry, &tr_entry->tx_queue[qidx],
							PACKET_TO_QUEUE_ENTRY(pkt));
		fifo_swq->swq[enq_idx] = tr_entry->wcid;
		INC_RING_INDEX(fifo_swq->enqIdx, TX_SWQ_FIFO_LEN);
			TR_ENQ_COUNT_INC(tr_entry);
		enq_done = TRUE;
		} else
	        {
		occupied_wcid = fifo_swq->swq[enq_idx];
		enq_done = FALSE;
			goto enq_end;
		}
	}
	/*check soft queue is enough*/
	capCount = (fifo_swq->enqIdx >=fifo_swq->deqIdx) ? (TX_SWQ_FIFO_LEN-fifo_swq->enqIdx+fifo_swq->deqIdx) : (fifo_swq->deqIdx-fifo_swq->enqIdx); 
	/*insert full queue to soft queue*/
	if(pPktQueue && pPktQueue->Number <= capCount)
	{
		while(pPktQueue->Head)
		{
			pEntry = RemoveHeadQueue(pPktQueue);
			tmpPkt =  QUEUE_ENTRY_TO_PACKET(pEntry);
			
			enq_idx = fifo_swq->enqIdx;
			if ((fifo_swq->swq[enq_idx] == 0) && (tr_entry->enq_cap)) {
				InsertTailQueueAc(pAd, tr_entry, &tr_entry->tx_queue[qidx],tmpPkt);
				fifo_swq->swq[enq_idx] = tr_entry->wcid;
				INC_RING_INDEX(fifo_swq->enqIdx, TX_SWQ_FIFO_LEN);
				TR_ENQ_COUNT_INC(tr_entry);
				enq_done = TRUE;
			} else {
				occupied_wcid = fifo_swq->swq[enq_idx];
				enq_done = FALSE;
				goto enq_end;
			}
		}
	}
enq_end:
	if (FlgIsLocked == FALSE)
		RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

	DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
				("%s():EnqPkt(%p) for WCID(%d) to tx_swq[%d].swq[%d] %s\n",
				__FUNCTION__, pkt, tr_entry->wcid, qidx, enq_idx,
				(enq_done ? "success" : "fail")));
	if (enq_done == FALSE) {
#ifdef DBG_DIAGNOSE
		if ((pAd->DiagStruct.inited) && (pAd->DiagStruct.wcid == tr_entry->wcid))
			pAd->DiagStruct.diag_info[pAd->DiagStruct.ArrayCurIdx].enq_fall_cnt[qidx]++;
#endif /* DBG_DIAGNOSE */

		DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
					("\t FailedCause =>OccupiedWCID:%d,EnqCap:%d\n",
					occupied_wcid, tr_entry->enq_cap));
	}

	if (0 /*(RTDebugFunc & DBG_FUNC_TXQ) == DBG_FUNC_TXQ*/) {
		DBGPRINT(RT_DEBUG_OFF, ("%s():EnqSuccess!\n", __FUNCTION__));
		rtmp_tx_swq_dump(pAd, qidx);
		rtmp_sta_txq_dump(pAd, tr_entry, qidx);
	}
        /*add hook point for enqueue progress*/
	RTMP_OS_TXRXHOOK_CALL(WLAN_TX_ENQUEUE_PROGRESS,NULL,qidx,fifo_swq);

	return enq_done;
}

INT rtmp_deq_req(RTMP_ADAPTER *pAd, INT cnt, struct dequeue_info *info)
{
	CHAR deq_qid = 0, start_q, end_q;
	UCHAR deq_wcid;
	struct tx_swq_fifo *fifo_swq;
	STA_TR_ENTRY *tr_entry = NULL;

	if (!info->inited) {
		if (info->target_que < WMM_QUE_NUM) {
			info->start_q = info->target_que;
			info->end_q = info->target_que;
		}
		else
		{
			info->start_q = (WMM_QUE_NUM - 1);
			info->end_q = 0;
		}
		info->cur_q = info->start_q;

		if (info->target_wcid < MAX_LEN_OF_TR_TABLE) {
			info->pkt_cnt = cnt;
			info->full_qid[0] = FALSE;
			info->full_qid[1] = FALSE;
			info->full_qid[2] = FALSE;
			info->full_qid[3] = FALSE;
		} else {
			info->q_max_cnt[0] = (UCHAR)cnt;
			info->q_max_cnt[1] = (UCHAR)cnt;
			info->q_max_cnt[2] = (UCHAR)cnt;
			info->q_max_cnt[3] = (UCHAR)cnt;
		}
		info->inited = 1;
	}

	start_q = info->cur_q;
	end_q = info->end_q;

	/* wcid first */
	if (info->target_wcid < MAX_LEN_OF_TR_TABLE) {
		if (info->pkt_cnt <= 0) {
			info->status = NDIS_STATUS_FAILURE;
			goto done;
		}

		deq_wcid = info->target_wcid;
		if (info->target_que >= WMM_QUE_NUM) {
			tr_entry = &pAd->MacTab.tr_entry[deq_wcid];
			for (deq_qid = start_q; deq_qid >= end_q; deq_qid--) {
				if (info->full_qid[deq_qid] == FALSE && tr_entry->tx_queue[deq_qid].Number)
					break;
			}
		} else if (info->full_qid[info->target_que] == FALSE) {
			deq_qid = info->target_que;
		} else {
			info->status = NDIS_STATUS_FAILURE;
			goto done;
		}

		if (deq_qid >= 0) {
			info->cur_q = deq_qid;
			info->cur_wcid = deq_wcid;
		} else {
			info->status = NDIS_STATUS_FAILURE;
		}

		goto done;
	}

	/*
		Start from high queue,
		Don't need get "pAd->irq_lock" here becuse already get it by caller
	*/
	for (deq_qid = start_q; deq_qid >= end_q; deq_qid--)
	{
		fifo_swq = &pAd->tx_swq[deq_qid];
		deq_wcid = fifo_swq->swq[fifo_swq->deqIdx];

		if (deq_wcid == 0) {
			DBGPRINT(RT_DEBUG_LOUD | DBG_FUNC_TXQ,
						("%s():tx_swq[%d] emtpy!\n", __FUNCTION__, deq_qid));
			info->q_max_cnt[deq_qid] = 0;
			continue;
		}

		tr_entry = &pAd->MacTab.tr_entry[deq_wcid];

		/* If any stations are in Psm, AP will skip this swq and increase deqIdx */
		if (tr_entry->EntryType == ENTRY_CAT_MCAST)
		{	/* Broadcast/Multicast */
			if (pAd->MacTab.fAnyStationInPsm)
			{
				DBGPRINT(RT_DEBUG_LOUD | DBG_FUNC_TXQ,
							("!!!B/MCast entry and AP in PSM\n"));

				fifo_swq->swq[fifo_swq->deqIdx] = 0;
				INC_RING_INDEX(fifo_swq->deqIdx, TX_SWQ_FIFO_LEN);
				continue;
			}
		}
		else
		{	/* unicast */
			if ((tr_entry->PsMode == PWR_SAVE)
#ifdef MT_PS
			||((tr_entry->ps_state != APPS_RETRIEVE_IDLE) && (tr_entry->ps_state != APPS_RETRIEVE_DONE))
#endif /* MT_PS */
			)
			{
				DBGPRINT(RT_DEBUG_LOUD | DBG_FUNC_TXQ, ("!!!STA in PSM\n"));

				fifo_swq->swq[fifo_swq->deqIdx] = 0;
				INC_RING_INDEX(fifo_swq->deqIdx, TX_SWQ_FIFO_LEN);
				if (tr_entry->tx_queue[deq_qid].Number > 0)
					TR_TOKEN_COUNT_INC(tr_entry, deq_qid);
				continue;
			}
		}

		if (info->q_max_cnt[deq_qid] > 0) {
			info->cur_q = deq_qid;
			info->cur_wcid = deq_wcid;
			info->pkt_cnt = info->q_max_cnt[deq_qid];
			break;
		} else {
			/* Queue Quota full, go next! */
		}
	}

	if (deq_qid < end_q) {
		info->cur_q = deq_qid;
		info->status = NDIS_STATUS_FAILURE;
	}

done:
	rtmp_psDeq_req(pAd);
	DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
				("%s(): DeqReq %s, Start/End/Cur Queue=%d/%d/%d\n",
				__FUNCTION__,
				(info->status == NDIS_STATUS_SUCCESS ? "success" : "fail"),
				info->start_q, info->end_q, info->cur_q));

	if (info->status == NDIS_STATUS_SUCCESS) {
		tr_entry = &pAd->MacTab.tr_entry[info->cur_wcid];

		DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
					("\tdeq_info=>wcid:%d, qidx:%d, pkt_cnt:%d, q_max_cnt=%d, QueuedNum=%d\n",
					info->cur_wcid, info->cur_q, info->pkt_cnt, info->q_max_cnt[deq_qid],
					tr_entry->tx_queue[info->cur_q].Number));
	} else {
		DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
					("\tdeq_info=>wcid:%d, qidx:%d, pkt_cnt:%d\n",
					info->cur_wcid, info->cur_q, info->pkt_cnt));
	}

#ifdef DBG
	if (0){ // RTDebugFunc & DBG_FUNC_TXQ) {
		if (deq_qid >= 0) {
			// cannot call this here because we already get lock but this function will try lock again!
			rtmp_tx_swq_dump(pAd, deq_qid);

			if (deq_wcid < MAX_LEN_OF_TR_TABLE)
				rtmp_sta_txq_dump(pAd, &pAd->MacTab.tr_entry[deq_wcid], deq_qid);
		}
	}
#endif /* DBG */

	return TRUE;
}


INT rtmp_deq_report(RTMP_ADAPTER *pAd, struct dequeue_info *info)
{
	UINT tx_cnt = info->deq_pkt_cnt, qidx = info->cur_q;
	struct tx_swq_fifo *fifo_swq;

	DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
				("%s():Success DeQ(QId=%d) for WCID(%d), PktCnt=%d, TxSWQDeQ/EnQ ID=%d/%d\n",
				__FUNCTION__, info->cur_q, info->cur_wcid, info->deq_pkt_cnt,
				pAd->tx_swq[qidx].deqIdx, pAd->tx_swq[qidx].enqIdx));

	if ((qidx < 4) && (tx_cnt > 0)) {
		fifo_swq = &pAd->tx_swq[qidx];
		do {
			if (fifo_swq->swq[fifo_swq->deqIdx]  == info->cur_wcid) {
				fifo_swq->swq[fifo_swq->deqIdx] = 0;
				INC_RING_INDEX(fifo_swq->deqIdx, TX_SWQ_FIFO_LEN);
				tx_cnt--;
			} else
				break;
		}while (tx_cnt != 0);

		if (info->q_max_cnt[qidx] > 0)
			info->q_max_cnt[qidx] -= info->deq_pkt_cnt;

		if (info->target_wcid < MAX_LEN_OF_TR_TABLE)
			info->pkt_cnt -= info->deq_pkt_cnt;

		//rtmp_tx_swq_dump(pAd, qidx);
		//rtmp_sta_txq_dump(pAd, &pAd->MacTab.tr_entry[info->wcid], qidx);
	}

	rtmp_psDeq_report(pAd,info);


	DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
				("After DeqReport, tx_swq D/EQIdx=%d/%d, deq_info.q_max_cnt/pkt_cnt=%d/%d\n",
				pAd->tx_swq[qidx].deqIdx, pAd->tx_swq[qidx].enqIdx,
				info->q_max_cnt[qidx] , info->pkt_cnt));

	return TRUE;
}


#ifdef CONFIG_AP_SUPPORT
INT deq_mgmt_frame(RTMP_ADAPTER *pAd, PNDIS_PACKET pkt, UCHAR qIdx, BOOLEAN bLocked)
{
	NDIS_STATUS Status;
	//unsigned long IrqFlags;

#ifdef RTMP_MAC_PCI
	if (RTMP_GET_PACKET_MGMT_PKT_DATA_QUE(pkt) == 1)
	{
		//DEQUEUE_LOCK(&pAd->irq_lock, bLocked, IrqFlags);
		Status = MlmeHardTransmitTxRing(pAd,qIdx,pkt);
		//DEQUEUE_UNLOCK(&pAd->irq_lock, bLocked, IrqFlags);
	}
	else
#endif /* RTMP_MAC_PCI */
		Status = MlmeHardTransmitMgmtRing(pAd,qIdx,pkt);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("tx queued mgmt frame error!\n"));
		RELEASE_NDIS_PACKET(pAd, pkt, NDIS_STATUS_FAILURE);
	}

	return TRUE;
}
#endif /* CONFIG_AP_SUPPORT */


#define TX_FLAG_STOP_DEQUEUE	(fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS |\
								fRTMP_ADAPTER_RADIO_OFF |\
								fRTMP_ADAPTER_RESET_IN_PROGRESS |\
								fRTMP_ADAPTER_HALT_IN_PROGRESS |\
								fRTMP_ADAPTER_NIC_NOT_EXIST |\
								fRTMP_ADAPTER_DISABLE_DEQUEUEPACKET)

#ifdef RELEASE_EXCLUDE
/*
	Traffic Jam checking:
	WDS:
		WDS need to check the peer's absent by tx few (2 ~3) pkts to peer.
		When WDS Jam happen by peer's absent, drop pkts to hw for 1 min.

	non-WDS interface:
		Sample Lin, 20100412
		we also need to tx pkt to detect the link periodically;
		Or when pMacEntry->ContinueTxFailCnt >= pAd->ApCfg.EntryLifeCheck,
		no any chance to clear pMacEntry->ContinueTxFailCnt.

	EX: When pMacEntry->ContinueTxFailCnt >= pAd->ApCfg.EntryLifeCheck,
		the condition will not be removed and we will drop all packets for the
		pEntry. But maybe the signal becomes better. So we try to send a packet
		periodically and we will get the tx status in tx done interrupt.

		If the tx status is success, pMacEntry->ContinueTxFailCnt will be cleared to 0.
*/
#endif /* RELEASE_EXCLUDE */
#define ENTRY_RETRY_INTERVAL	(100 * OS_HZ / 1000)
static inline BOOLEAN traffic_jam_chk(RTMP_ADAPTER *pAd, STA_TR_ENTRY *tr_entry)
{
	BOOLEAN drop_it = FALSE;

	if (!IS_ENTRY_NONE(tr_entry))
	{
		ULONG Now32;

		NdisGetSystemUpTime(&Now32);

#ifdef CONFIG_AP_SUPPORT
#ifdef WDS_SUPPORT
		if (IS_ENTRY_WDS(tr_entry))
		{
			if (tr_entry->LockEntryTx &&
				RTMP_TIME_BEFORE(Now32, tr_entry->TimeStamp_toTxRing + WDS_ENTRY_RETRY_INTERVAL))
				drop_it = TRUE;
		}
		else
#endif /* WDS_SUPPORT */
		if (tr_entry->ContinueTxFailCnt >= pAd->ApCfg.EntryLifeCheck)
		{
			if(RTMP_TIME_BEFORE(Now32, tr_entry->TimeStamp_toTxRing + ENTRY_RETRY_INTERVAL))
				drop_it = TRUE;
		}
		else
#endif /* CONFIG_AP_SUPPORT */
		{
			tr_entry->TimeStamp_toTxRing = Now32;
		}
	}

	return drop_it;
}


INT deq_packet_gatter(RTMP_ADAPTER *pAd, struct dequeue_info *deq_info, TX_BLK *pTxBlk, BOOLEAN in_hwIRQ)
{
	STA_TR_ENTRY *tr_entry;
	MAC_TABLE_ENTRY *pMacEntry = NULL;
#ifdef WMM_ACM_SUPPORT
	BOOLEAN hasTxDesc = FALSE;
#endif /* WMM_ACM_SUPPORT */
	ULONG hwd_cnt = 0;
	PQUEUE_ENTRY qEntry = NULL;
	PNDIS_PACKET pPacket;
	PQUEUE_HEADER pQueue;
	//unsigned long IrqFlags;
	UCHAR QueIdx = deq_info->cur_q;
	UCHAR wcid = deq_info->cur_wcid;

	tr_entry = &pAd->MacTab.tr_entry[wcid];
	if (wcid < MAX_LEN_OF_MAC_TABLE)
		pMacEntry = &pAd->MacTab.Content[wcid];
	else
		pMacEntry = &pAd->MacTab.Content[MCAST_WCID];

	DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("-->%s(): deq_info->wcid=%d, qidx=%d! hwd_cnt=%lu!\n",
				__FUNCTION__, wcid, QueIdx, hwd_cnt));

	deq_info->deq_pkt_cnt = 0;

	do {
		hwd_cnt = GET_TXRING_FREENO(pAd, QueIdx);
		pQueue = &tr_entry->tx_queue[QueIdx];

dequeue:
		if ((qEntry = pQueue->Head) != NULL)
		{
			qEntry = RemoveHeadQueue(pQueue);
			TR_ENQ_COUNT_DEC(tr_entry);
			pPacket = QUEUE_ENTRY_TO_PACKET(qEntry);

			ASSERT(RTMP_GET_PACKET_WCID(pPacket) == wcid);

			DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
						("-->%s(): GetPacket, wcid=%d, deq_pkt_cnt=%d, TotalFrameNum=%d\n",
						__FUNCTION__, wcid, deq_info->deq_pkt_cnt, pTxBlk->TotalFrameNum));

#ifdef CONFIG_AP_SUPPORT
			// TODO: shiang-usw, for mgmt frame enqueue to a dataQ but still go through mgmtRing here, is that good??
			if (RTMP_GET_PACKET_MGMT_PKT(pPacket)) {
				DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("%s(): Call deq_mgmt_frame()!\n", __FUNCTION__));
				deq_mgmt_frame(pAd, pPacket, QueIdx, in_hwIRQ);
				deq_info->deq_pkt_cnt++;

				goto dequeue;
			}
#endif /* CONFIG_AP_SUPPORT */

			// TODO: shiang-usw, remove this check to other place!!
			if (traffic_jam_chk(pAd, tr_entry) == TRUE) {
				DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("%s(): traffic jam detected! free pkt!\n", __FUNCTION__));
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);

				deq_info->deq_pkt_cnt++;

				goto dequeue;
			}

			pTxBlk->TxFrameType = TxPktClassification(pAd, pPacket, pTxBlk);
			if (pTxBlk->TxFrameType & (TX_RALINK_FRAME | TX_AMSDU_FRAME)) {
				if (pTxBlk->TotalFrameNum == 0) {
					struct tx_swq_fifo *tx_swq = &pAd->tx_swq[QueIdx];
					BOOLEAN tx_swq_empty = FALSE;
					UINT diff = 0;

					if (tx_swq->enqIdx > tx_swq->deqIdx)
						diff = tx_swq->enqIdx - tx_swq->deqIdx;
					else if (tx_swq->enqIdx < tx_swq->deqIdx)
						diff = TX_SWQ_FIFO_LEN - tx_swq->deqIdx + tx_swq->enqIdx;

					if (diff == 1)
						tx_swq_empty = TRUE;

					if ((tx_swq_empty == TRUE) &&
						(NEED_QUEUE_BACK_FOR_AGG(pAd, QueIdx,
									hwd_cnt, pTxBlk->TxFrameType)))
					{
						InsertHeadQueue(pQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
						TR_ENQ_COUNT_INC(tr_entry);
						deq_info->q_max_cnt[QueIdx] = 0;
						break;
					}
				}
				else
				{
					if (CanDoAggregateTransmit(pAd, pPacket, pTxBlk) == FALSE) {
						InsertHeadQueue(pQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
						TR_ENQ_COUNT_INC(tr_entry);
						goto start_kick;
					}
				}
			}

#ifdef WMM_ACM_SUPPORT
#ifdef ACM_CC_FUNC_QUE_TX_CTRL
			/* init ACM related parameters for the packet */
			if (RTMP_GET_PACKET_TX_TIME(pPacket)) {
				UINT32 QueTypeTx;

				/*
					when any ACM of one AC is enabled, check if the packet can
					be transmitted

					Null frame is tx in management queue, not here
					but QoS null belongs to data frame so the method violates spec.
				*/
				ACMR_PKT_QOS_TYPE_SET(pPacket, 0);
				QueTypeTx = ACMP_MsduClassify(pAd, pMacEntry, pPacket,
											QueIdx, 0,
											pTxBlk->TxFrameType,
											pTxBlk->TxPacketList.Number + 1);

				if (pTxBlk->TotalFrameNum == 0) {
					if ((QueTypeTx == QID_AC_BE) || (QueTypeTx == QID_AC_BK) ||
						(QueTypeTx == QID_AC_VI) ||(QueTypeTx == QID_AC_VO))
					{
						QueIdx = QueTypeTx;
					}
					else
					{
						RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
						deq_info->deq_pkt_cnt++;
						continue;
					}
				} else {
					if (QueTypeTx != QueIdx)
						hasTxDesc = FALSE; /* Can not do AMSDU for the packet */
				}
			}
#endif /* ACM_CC_FUNC_QUE_TX_CTRL */
#endif /* WMM_ACM_SUPPORT */

			/*Need located here, since RTMP_HAS_ENOUGH_FREE_DESC will use it when USB mode*/
			pTxBlk->QueIdx = QueIdx;
#ifdef MT_MAC
#ifdef USE_BMC
			if ((pAd->chipCap.hif_type == HIF_MT) && (pAd->MacTab.fAnyStationInPsm == 1) && (tr_entry->EntryType == ENTRY_CAT_MCAST))
				pTxBlk->QueIdx = QID_BMC;
#endif /* USE_BMC */
#endif

			/* Early check to make sure we have enoguh Tx Resource. */
			if (!RTMP_HAS_ENOUGH_FREE_DESC(pAd, pTxBlk, hwd_cnt, pPacket))
			{
				pAd->PrivateInfo.TxRingFullCnt++;
				InsertHeadQueue(pQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
				TR_ENQ_COUNT_INC(tr_entry);

				if (deq_info->target_wcid < MAX_LEN_OF_TR_TABLE) {
					deq_info->full_qid[QueIdx] = TRUE;
				} else {
				deq_info->q_max_cnt[QueIdx] = 0;
				}
#ifdef DBG_DIAGNOSE
				if (pAd->DiagStruct.inited && pAd->DiagStruct.wcid == pTxBlk->Wcid) {
					struct dbg_diag_info *diag_info;

					diag_info = &pAd->DiagStruct.diag_info[pAd->DiagStruct.ArrayCurIdx];
					diag_info->deq_fail_no_resource_cnt[QueIdx]++;
				}
#endif /* DBG_DIAGNOSE */

#ifdef RELEASE_EXCLUDE
				DBGPRINT(RT_DEBUG_LOUD, ("TxRing(%d) full, PktFragCnt=%d\n",
								QueIdx, RTMP_GET_PACKET_FRAGMENTS(pPacket)));
#ifdef RTMP_MAC_PCI
				DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("Deq:NoFreeTxD[%d](CIDX=%d,TxSwFreeIdx=%d)\n",
							QueIdx, pAd->TxRing[QueIdx].TxCpuIdx,
							pAd->TxRing[QueIdx].TxSwFreeIdx));
#endif /* RTMP_MAC_PCI */
#endif /* RELEASE_EXCLUDE */
#ifdef RTMP_MAC_PCI
				DBGPRINT(RT_DEBUG_INFO, ("%s(): No free TxD(hwd_cnt=%ld, SWIDx=%d, CIDX=%d)\n",
							__FUNCTION__, hwd_cnt, pAd->TxRing[QueIdx].TxSwFreeIdx,
							pAd->TxRing[QueIdx].TxCpuIdx));
#else
                		DBGPRINT(RT_DEBUG_INFO, ("%s(): deque break beacuse no free txResource\n", __FUNCTION__));
#endif /* RTMP_MAC_PCI */

				break;
			}

			pTxBlk->TotalFrameNum++;
			/* The real fragment number maybe vary*/
			pTxBlk->TotalFragNum += RTMP_GET_PACKET_FRAGMENTS(pPacket);
			pTxBlk->TotalFrameLen += GET_OS_PKT_LEN(pPacket);

			if (pTxBlk->TotalFrameNum == 1) {

				pTxBlk->pPacket = pPacket;
				pTxBlk->wdev = tr_entry->wdev;
				pTxBlk->tr_entry = tr_entry;
				pTxBlk->pMacEntry = pMacEntry;
#ifdef VENDOR_FEATURE1_SUPPORT
				pTxBlk->HeaderBuf = (UCHAR *)pTxBlk->HeaderBuffer;
#endif /* VENDOR_FEATURE1_SUPPORT */
#if defined(P2P_SUPPORT) || defined(RT_CFG80211_P2P_SUPPORT) || defined(CFG80211_MULTI_STA)
				pTxBlk->OpMode = RTMP_GET_PACKET_OPMODE(pPacket);
#endif /* P2P_SUPPORT || RT_CFG80211_P2P_SUPPORT */
			}

			InsertTailQueue(&pTxBlk->TxPacketList, PACKET_TO_QUEUE_ENTRY(pPacket));
		}
		else
		{
			if (pTxBlk->TxPacketList.Number == 0)
			{
				deq_info->deq_pkt_cnt++;
				DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("<--%s():Try deQ a empty Q. pTxBlk.TxPktList.Num=%d, deq_info.pkt_cnt=%d\n",
					__FUNCTION__, pTxBlk->TxPacketList.Number, deq_info->pkt_cnt));
				break;
			}
		}

		if ((pTxBlk->TxFrameType & (TX_RALINK_FRAME | TX_AMSDU_FRAME)) &&
			(pTxBlk->TotalFrameNum == 1) && (pQueue->Head != NULL)) {
			DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
						("%s(): ForAMSDU/ARalink, try dequeue more pkt!\n",
						__FUNCTION__));
			goto dequeue;
		}

start_kick:
		if (pTxBlk->TxFrameType & (TX_RALINK_FRAME | TX_AMSDU_FRAME))
		{
			if (pTxBlk->TxPacketList.Number == 1)
				pTxBlk->TxFrameType = TX_LEGACY_FRAME;
		}

		DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("<--%s():pTxBlk.TxPktList.Num=%d, deq_info.pkt_cnt=%d\n",
					__FUNCTION__, pTxBlk->TxPacketList.Number, deq_info->pkt_cnt));
		break;
	} while(pTxBlk->TxPacketList.Number < deq_info->pkt_cnt);

	if (pTxBlk->TxPacketList.Number > 0) {
		deq_info->deq_pkt_cnt += pTxBlk->TxPacketList.Number;
		return TRUE;
	}
	else
		return FALSE;
}


/*
	========================================================================

	Routine Description:
		To do the enqueue operation and extract the first item of waiting
		list. If a number of available shared memory segments could meet
		the request of extracted item, the extracted item will be fragmented
		into shared memory segments.

	Arguments:
		pAd Pointer to our adapter
		pQueue		Pointer to Waiting Queue

	Return Value:
		None

	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
VOID RTMPDeQueuePacket(
	IN RTMP_ADAPTER *pAd,
	IN BOOLEAN in_hwIRQ,
	IN UCHAR QIdx,
	IN INT wcid,
	IN INT max_cnt)
{
	//NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	INT Count = 0, round  = 0;
	TX_BLK TxBlk, *pTxBlk = &TxBlk;
	UCHAR QueIdx = 0;
	unsigned long	IrqFlags = 0;
	struct dequeue_info deq_info = {0};

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO,("RTMPDeQueuePacket (Tx:%d)--> \n", max_cnt));
#endif /* RELEASE_EXCLUDE */

#ifdef DBG_DIAGNOSE
	if (pAd->DiagStruct.inited)
		dbg_diag_deque_log(pAd);
#endif /* DBG_DIAGNOSE */

	//NdisZeroMemory((UCHAR *)&deq_info, sizeof(deq_info));

	deq_info.target_wcid = (UCHAR)((wcid == WCID_ALL) ? MAX_LEN_OF_TR_TABLE : wcid);
	deq_info.target_que = QIdx;

	do
	{
		// TODO: shiang-usw, for another two options "IS_P2P_ABSENCE(pAd)" and RTMP_TEST_MORE_FLAG(pAd, fRTMP_ADAPTER_DISABLE_DEQUEUE)
		// TODO: we need to take care that for per-entry based control "tr_entry->deq_cap"
		if (RTMP_TEST_FLAG(pAd, TX_FLAG_STOP_DEQUEUE))
		{
			DBGPRINT(RT_DEBUG_TRACE,("%s:: SW Tx Dequeue was stoped, pAd->Flags=0x%lx\n", __FUNCTION__, pAd->Flags));
			break;
		}

		round++;

		DEQUEUE_LOCK(&pAd->irq_lock, in_hwIRQ, IrqFlags);

		rtmp_deq_req(pAd, max_cnt, &deq_info);
		if (deq_info.status == NDIS_STATUS_FAILURE) {
			DEQUEUE_UNLOCK(&pAd->irq_lock, in_hwIRQ, IrqFlags);
			break;
		}

		QueIdx = deq_info.cur_q;

		DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("%s(): deq_info:cur_wcid=%d, cur_qidx=%d, pkt_cnt=%d, pkt_bytes=%d\n",
			__FUNCTION__, deq_info.cur_wcid, QueIdx, deq_info.pkt_cnt, deq_info.pkt_bytes));
		
		NdisZeroMemory((UCHAR *)pTxBlk, sizeof(TX_BLK));

		RTMP_START_DEQUEUE(pAd, QueIdx, IrqFlags);

		deq_packet_gatter(pAd, &deq_info, pTxBlk, in_hwIRQ);
		if (pTxBlk->TotalFrameNum) {
			ASSERT(pTxBlk->wdev);
			if (pTxBlk->wdev)
				ASSERT(pTxBlk->wdev->wdev_hard_tx);

			if (pTxBlk->wdev && pTxBlk->wdev->wdev_hard_tx) {
				pTxBlk->wdev->wdev_hard_tx(pAd, pTxBlk);
			}
			else
			{
				DBGPRINT(RT_DEBUG_ERROR, ("%s():ERR! pTxBlk->wdev=%p, wdev_hard_tx=%p\n",
							__FUNCTION__, pTxBlk->wdev,
							pTxBlk->wdev ? pTxBlk->wdev->wdev_hard_tx : NULL));

#ifdef CONFIG_AP_SUPPORT
				IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					/*Status =*/ APHardTransmit(pAd, pTxBlk);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
				IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
					/*Status =*/ STAHardTransmit(pAd, pTxBlk);
#endif /* CONFIG_STA_SUPPORT */
			}

			Count += pTxBlk->TotalFrameNum;
		}

		RTMP_STOP_DEQUEUE(pAd, QueIdx, IrqFlags);

		rtmp_deq_report(pAd, &deq_info);

		DEQUEUE_UNLOCK(&pAd->irq_lock, in_hwIRQ, IrqFlags);

		DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ, ("%s(): deq_packet_gatter %s, TotalFrmNum=%d\n",
					__FUNCTION__, (pTxBlk->TotalFrameNum > 0 ? "success" : "fail"),
					pTxBlk->TotalFrameNum));

#ifdef RTMP_MAC_PCI
		if (++pAd->FifoUpdateDone >= FIFO_STAT_READ_PERIOD)
		{
			// TODO: shiang-usw, check this because of REG access here!!
			NICUpdateFifoStaCounters(pAd);
			pAd->FifoUpdateDone = 0;
		}
#endif /* RTMP_MAC_PCI */

#ifdef DBG
		if (round >= 1024) {
			DBGPRINT(RT_DEBUG_OFF, ("%s():ForceToBreak!!Buggy here?\n", __FUNCTION__));
			break;
		}
#endif /* DBG */

	}while(1);

#ifdef DBG_DEQUE
	DBGPRINT(RT_DEBUG_INFO | DBG_FUNC_TXQ,
				("--->%s():DeQueueRule:WCID[%d], Que[%d]\n",
				__FUNCTION__, deq_info.target_wcid, deq_info.target_que));
#endif /* DBG_DEQUE */

#ifdef DBG_DIAGNOSE
	if (pAd->DiagStruct.inited) {
		struct dbg_diag_info *diag_info;

		diag_info = &pAd->DiagStruct.diag_info[pAd->DiagStruct.ArrayCurIdx];
		diag_info->deq_called++;
		diag_info->deq_round += round;
		if (Count < 8)
			diag_info->deq_cnt[Count]++;
		else
			diag_info->deq_cnt[8]++;
	}
#endif /* DBG_DIAGNOSE */

#ifdef WMM_ACM_SUPPORT
#ifdef RTMP_MAC_USB
	if (!hasTxDesc)
		RTUSBKickBulkOut(pAd);
#endif /* RTMP_MAC_USB */
#endif /* WMM_ACM_SUPPORT */

#ifdef BLOCK_NET_IF
	ASSERT((QueIdx < NUM_OF_TX_RING));
	if ((pAd->blockQueueTab[QueIdx].SwTxQueueBlockFlag == TRUE)
		&& /* (pAd->TxSwQueue[QueIdx].Number < 1)*/
		(pAd->tx_swq[QueIdx].enqIdx == pAd->tx_swq[QueIdx].deqIdx)
	)
	{
		releaseNetIf(&pAd->blockQueueTab[QueIdx]);
	}
#endif /* BLOCK_NET_IF */

}


/*
	========================================================================

	Routine Description:
		Calculates the duration which is required to transmit out frames
	with given size and specified rate.

	Arguments:
		pAd 	Pointer to our adapter
		Rate			Transmit rate
		Size			Frame size in units of byte

	Return Value:
		Duration number in units of usec

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
USHORT RTMPCalcDuration(RTMP_ADAPTER *pAd, UCHAR Rate, ULONG Size)
{
	ULONG	Duration = 0;

	if (Rate < RATE_FIRST_OFDM_RATE) /* CCK*/
	{
		if ((Rate > RATE_1) && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED))
			Duration = 96;	/* 72+24 preamble+plcp*/
		else
			Duration = 192; /* 144+48 preamble+plcp*/

		Duration += (USHORT)((Size << 4) / RateIdTo500Kbps[Rate]);
		if ((Size << 4) % RateIdTo500Kbps[Rate])
			Duration ++;
	}
	else if (Rate <= RATE_LAST_OFDM_RATE)/* OFDM rates*/
	{
		Duration = 20 + 6;		/* 16+4 preamble+plcp + Signal Extension*/
		Duration += 4 * (USHORT)((11 + Size * 4) / RateIdTo500Kbps[Rate]);
		if ((11 + Size * 4) % RateIdTo500Kbps[Rate])
			Duration += 4;
	}
	else	/*mimo rate*/
	{
		Duration = 20 + 6;		/* 16+4 preamble+plcp + Signal Extension*/
	}

	return (USHORT)Duration;
}


/*
	NOTE: we do have an assumption here, that Byte0 and Byte1
		always reasid at the same scatter gather buffer
 */
static inline VOID Sniff2BytesFromNdisBuffer(
	IN PNDIS_BUFFER buf,
	IN UCHAR offset,
	OUT UCHAR *p0,
	OUT UCHAR *p1)
{
	UCHAR *ptr = (UCHAR *)(buf + offset);
	*p0 = *ptr;
	*p1 = *(ptr + 1);
}


#define ETH_TYPE_VLAN	0x8100
#define ETH_TYPE_IPv4	0x0800
#define ETH_TYPE_IPv6	0x86dd
#define ETH_TYPE_ARP	0x0806
#define ETH_TYPE_EAPOL	0x888e
#define ETH_TYPE_WAI	0x88b4

#define IP_VER_CODE_V4	0x40
#define IP_VER_CODE_V6	0x60
#define IP_PROTO_UDP	0x11
#define IP_HDR_LEN		20
#define ETH_HDR_LEN		14

#if defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2)
VOID CheckQosMapUP(PMAC_TABLE_ENTRY pEntry, UCHAR DSCP, PUCHAR pUserPriority)
{				
	UCHAR i = 0, find_up = 0, dscpL = 0, dscpH = 0;

	for (i=0;i<(pEntry->DscpExceptionCount/2);i++) {
		if ((pEntry->DscpException[i] & 0xff) == DSCP) {
			*pUserPriority = (pEntry->DscpException[i]>>8) & 0xff;
			find_up = 1;
			break;
		}
	}
				
	if (!find_up) {
		for (i=0;i<8;i++) {
			dscpL = pEntry->DscpRange[i] & 0xff;
			dscpH = (pEntry->DscpRange[i]>>8) & 0xff;	
			if ((DSCP <= dscpH) && (DSCP >= dscpL)) {
				*pUserPriority = i;
				break;
			}
		}
	}
}
#endif /* defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2) */

/*
	Check the Ethernet Frame type, and set RTMP_SET_PACKET_SPECIFIC flags
	Here we set the PACKET_SPECIFIC flags(LLC, VLAN, DHCP/ARP, EAPOL).
*/
BOOLEAN RTMPCheckEtherType(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET	pPacket,
	IN STA_TR_ENTRY *tr_entry,
	IN struct wifi_dev *wdev,
	OUT UCHAR *pUserPriority,
	OUT UCHAR *pQueIdx,
	OUT UCHAR *pWcid)
{
	UINT16 TypeLen;
	UCHAR Byte0, Byte1, *pSrcBuf, up = 0;
#if defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2)
	BSS_STRUCT *pMbss;
	MAC_TABLE_ENTRY *pMacEntry = &pAd->MacTab.Content[tr_entry->wcid];
	if (IS_ENTRY_CLIENT(tr_entry)) {
		BSS_STRUCT *pMbss = (BSS_STRUCT *) wdev->func_dev;
	}
#endif /* CONFIG_AP_SUPPORT && CONFIG_HOTSPOT_R2 */
	pSrcBuf = GET_OS_PKT_DATAPTR(pPacket);
	ASSERT(pSrcBuf);

	RTMP_SET_PACKET_SPECIFIC(pPacket, 0);

	/* get Ethernet protocol field and skip the Ethernet Header */
	TypeLen = (pSrcBuf[12] << 8) | pSrcBuf[13];
	pSrcBuf += LENGTH_802_3;
	if (TypeLen <= 1500)
	{
		/*
			802.3, 802.3 LLC:
				DestMAC(6) + SrcMAC(6) + Lenght(2) +
				DSAP(1) + SSAP(1) + Control(1) +
			if the DSAP = 0xAA, SSAP=0xAA, Contorl = 0x03, it has a 5-bytes SNAP header.
				=> + SNAP (5, OriginationID(3) + etherType(2))
			else
				=> It just has 3-byte LLC header, maybe a legacy ether type frame. we didn't handle it
		*/
		if (pSrcBuf[0] == 0xAA && pSrcBuf[1] == 0xAA && pSrcBuf[2] == 0x03)
		{
			Sniff2BytesFromNdisBuffer((PNDIS_BUFFER)pSrcBuf, 6, &Byte0, &Byte1);
			RTMP_SET_PACKET_LLCSNAP(pPacket, 1);
			TypeLen = (USHORT)((Byte0 << 8) + Byte1);
			pSrcBuf += LENGTH_802_1_H; /* Skip this LLC/SNAP header*/
		} else {
			return FALSE;
		}
	}

	/* If it's a VLAN packet, get the real Type/Length field.*/
	if (TypeLen == ETH_TYPE_VLAN)
	{
#ifdef CONFIG_AP_SUPPORT
		/*
			802.3 VLAN packets format:

			DstMAC(6B) + SrcMAC(6B)
			+ 802.1Q Tag Type (2B = 0x8100) + Tag Control Information (2-bytes)
			+ Length/Type(2B)
			+ data payload (42-1500 bytes)
			+ FCS(4B)

			VLAN tag: 3-bit UP + 1-bit CFI + 12-bit VLAN ID
		*/

		/* No matter unicast or multicast, discard it if not my VLAN packet. */
		if (wdev->VLAN_VID != 0)
		{
			USHORT vlan_id = *(USHORT *)pSrcBuf;

			vlan_id = cpu2be16(vlan_id);
			vlan_id = vlan_id & 0x0FFF; /* 12 bit */
			if (vlan_id != wdev->VLAN_VID) {
DBGPRINT(RT_DEBUG_OFF, ("%s():failed for VLAN_ID(vlan_id=%d, VLAN_VID=%d)\n",
					__FUNCTION__, vlan_id, wdev->VLAN_VID));
				return FALSE;
			}
		}
#endif /* CONFIG_AP_SUPPORT */

		RTMP_SET_PACKET_VLAN(pPacket, 1);
		Sniff2BytesFromNdisBuffer((PNDIS_BUFFER)pSrcBuf, 2, &Byte0, &Byte1);
		TypeLen = (USHORT)((Byte0 << 8) + Byte1);

		/* only use VLAN tag as UserPriority setting */
		up = (*pSrcBuf & 0xe0) >> 5;

#if defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2)
		if (pMacEntry && pMacEntry->QosMapSupport && pMbss->HotSpotCtrl.QosMapEnable) {
			CheckQosMapUP(pMacEntry, (*pSrcBuf & 0xfc) >> 2, pUserPriority);
		}
#endif /* defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2) */

		pSrcBuf += LENGTH_802_1Q; /* Skip the VLAN Header.*/
	}
	else if (TypeLen == ETH_TYPE_IPv4)
	{
		/*
			0       4       8          14  15                      31(Bits)
			+---+----+-----+----+---------------+
			|Ver |  IHL |DSCP |ECN |    TotalLen           |
			+---+----+-----+----+---------------+
			Ver    - 4bit Internet Protocol version number.
			IHL    - 4bit Internet Header Length
			DSCP - 6bit Differentiated Services Code Point(TOS)
			ECN   - 2bit Explicit Congestion Notification
			TotalLen - 16bit IP entire packet length(include IP hdr)
		*/
		up = (*(pSrcBuf + 1) & 0xe0) >> 5;
#if defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2)
		if (pMacEntry && pMacEntry->QosMapSupport && pMbss->HotSpotCtrl.QosMapEnable) {
			CheckQosMapUP(pMacEntry, (*(pSrcBuf+1) & 0xfc) >> 2, pUserPriority);
		}
#endif			
	}
	else if (TypeLen == ETH_TYPE_IPv6)
	{
		/*
			0       4       8        12     16                      31(Bits)
			+---+----+----+----+---------------+
			|Ver | TrafficClas |  Flow Label                   |
			+---+----+----+--------------------+
			Ver           - 4bit Internet Protocol version number.
			TrafficClas - 8bit traffic class field, the 6 most-significant bits used for DSCP
		*/
		up = ((*pSrcBuf) & 0x0e) >> 1;
				
#if defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2)
		if (pMacEntry && pMacEntry->QosMapSupport && pMbss->HotSpotCtrl.QosMapEnable) {
			CheckQosMapUP(pMacEntry, ((*pSrcBuf & 0x0f) << 2)|((*(pSrcBuf+1)) & 0xc0) >> 6, pUserPriority);
		}
#endif /* defined(CONFIG_AP_SUPPORT) && defined(CONFIG_HOTSPOT_R2) */
	}

#ifdef RTMP_RBUS_SUPPORT
#ifdef VIDEO_TURBINE_SUPPORT
	if (pAd->VideoTurbine.Enable)
	{
		/* RVT Out-band QoS */
		struct sk_buff *pSkbPkt = RTPKT_TO_OSPKT(pPacket);

		if(pSkbPkt->cb[40]==0x0E)
			up = (pSkbPkt->cb[41] & 0xe0) >> 5;
	}
#endif /* VIDEO_TURBINE_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

	switch (TypeLen)
	{
		case ETH_TYPE_IPv4:
			{
				UINT32 pktLen = GET_OS_PKT_LEN(pPacket);

				ASSERT((pktLen > (ETH_HDR_LEN + IP_HDR_LEN)));	/* 14 for ethernet header, 20 for IP header*/
				RTMP_SET_PACKET_IPV4(pPacket, 1);
				if (*(pSrcBuf + 9) == IP_PROTO_UDP)
				{
					UINT16 srcPort, dstPort;

					pSrcBuf += IP_HDR_LEN;
					srcPort = OS_NTOHS(get_unaligned((PUINT16)(pSrcBuf)));
					dstPort = OS_NTOHS(get_unaligned((PUINT16)(pSrcBuf+2)));

					if ((srcPort==0x44 && dstPort==0x43) || (srcPort==0x43 && dstPort==0x44))
					{	/*It's a BOOTP/DHCP packet*/
						RTMP_SET_PACKET_DHCP(pPacket, 1);
						RTMP_SET_PACKET_TXTYPE(pPacket, TX_LEGACY_FRAME);
					}

#ifdef CONFIG_AP_SUPPORT
#ifdef CONFIG_DOT11V_WNM
					WNMIPv4ProxyARPCheck(pAd, pPacket, srcPort, dstPort, pSrcBuf);
#endif
#ifdef CONFIG_HOTSPOT
					{
						UCHAR Wcid = RTMP_GET_PACKET_WCID(pPacket);
						if (!HSIPv4Check(pAd, &Wcid, pPacket, pSrcBuf, srcPort, dstPort))
						return FALSE;
					}
#endif
#endif
				}
			}
			break;
		case ETH_TYPE_ARP:
			{
#ifdef CONFIG_AP_SUPPORT
#ifdef CONFIG_DOT11V_WNM
				BSS_STRUCT *pMbss = (BSS_STRUCT *)wdev->func_dev;
				if (pMbss->WNMCtrl.ProxyARPEnable)
				{
					/* Check if IPv4 Proxy ARP Candidate from DS */
					if (IsIPv4ProxyARPCandidate(pAd, pSrcBuf - 2))
					{
						BOOLEAN FoundProxyARPEntry;
						FoundProxyARPEntry = IPv4ProxyARP(pAd, pMbss, pSrcBuf - 2, TRUE);
						if (!FoundProxyARPEntry)
							DBGPRINT(RT_DEBUG_TRACE, ("Can not find proxy entry\n"));

						return FALSE;
					}
				}
#endif
#ifdef CONFIG_HOTSPOT
				if (pMbss->HotSpotCtrl.HotSpotEnable)
				{
					if (!pMbss->HotSpotCtrl.DGAFDisable)
					{
						if (IsGratuitousARP(pAd, pSrcBuf - 2, pSrcBuf-14, pMbss))
							return FALSE;
					}
				}

#endif
#endif
				RTMP_SET_PACKET_DHCP(pPacket, 1);
				RTMP_SET_PACKET_TXTYPE(pPacket, TX_LEGACY_FRAME);
			}
			break;
		case ETH_P_IPV6:
			{
#ifdef CONFIG_AP_SUPPORT
#ifdef CONFIG_DOT11V_WNM
				BSS_STRUCT *pMbss = (BSS_STRUCT *)wdev->func_dev;
				WNMIPv6ProxyARPCheck(pAd, pPacket, pSrcBuf);
				if (pMbss->WNMCtrl.ProxyARPEnable)
				{
					/* Check if IPv6 Proxy ARP Candidate from DS */
					if (IsIPv6ProxyARPCandidate(pAd, pSrcBuf - 2))
					{
						BOOLEAN FoundProxyARPEntry;
						FoundProxyARPEntry = IPv6ProxyARP(pAd, pMbss, pSrcBuf - 2, TRUE);
						if (!FoundProxyARPEntry)
							DBGPRINT(RT_DEBUG_TRACE, ("Can not find IPv6 proxy entry\n"));

						return FALSE;
					}
				}
#endif
#ifdef CONFIG_HOTSPOT
				if (pMbss->HotSpotCtrl.HotSpotEnable)
				{
					if (!pMbss->HotSpotCtrl.DGAFDisable)
					{
						if (IsUnsolicitedNeighborAdver(pAd, pSrcBuf - 2))
							return FALSE;
					}
				}
#endif
				/*
					Check if DHCPv6 Packet, and Convert group-address DHCP
					packets to individually-addressed 802.11 frames
 				 */
#endif
				/* return AC_BE if packet is not IPv6 */
				if ((*pSrcBuf & 0xf0) != 0x60)
					up = 0;
			}
			break;
		case ETH_TYPE_EAPOL:
			RTMP_SET_PACKET_EAPOL(pPacket, 1);
			RTMP_SET_PACKET_TXTYPE(pPacket, TX_LEGACY_FRAME);
			break;
#ifdef WAPI_SUPPORT
		case ETH_TYPE_WAI:
			RTMP_SET_PACKET_WAI(pPacket, 1);
			RTMP_SET_PACKET_TXTYPE(pPacket, TX_LEGACY_FRAME);
			break;
#endif /* WAPI_SUPPORT */

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
		case 0x890d:
			{
				RTMP_SET_PACKET_TDLS_MMPDU(pPacket, 1);
				RTMP_SET_PACKET_TXTYPE(pPacket, TX_LEGACY_FRAME);
				up = 5;
			}
			break;
#endif /* DOT11Z_TDLS_SUPPORT */

		default:
			break;
	}

#ifdef VENDOR_FEATURE1_SUPPORT
	RTMP_SET_PACKET_PROTOCOL(pPacket, TypeLen);
#endif /* VENDOR_FEATURE1_SUPPORT */

	/* have to check ACM bit. downgrade UP & QueIdx before passing ACM*/
	/* NOTE: AP doesn't have to negotiate TSPEC. ACM is controlled purely via user setup, not protocol handshaking*/
	/*
		Under WMM ACM control, we dont need to check the bit;
		Or when a TSPEC is built for VO but we will change priority to
		BE here and when we issue a BA session, the BA session will
		be BE session, not VO session.
	*/
#ifndef WMM_ACM_SUPPORT
	if (pAd->CommonCfg.APEdcaParm.bACM[WMM_UP2AC_MAP[up]])
		up = 0;
#endif /* WMM_ACM_SUPPORT */

#ifdef WAC_SUPPORT
#ifdef WAC_QOS_PRIORITY
	if (tr_entry->bSamsungForcePriority)
		up = 7;
#endif /* WAC_QOS_PRIORITY */
#endif /* WAC_SUPPORT */

	/*
		Set WMM when
		1. wdev->bWmmCapable == TRUE
		2. Receiver's capability
			a). bc/mc packets
				->Need to get UP for IGMP use
			b). unicast packets
				-> CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_WMM_CAPABLE)
		3. has VLAN tag or DSCP fields in IPv4/IPv6 hdr
	*/
	if ((wdev->bWmmCapable == TRUE) && (up <= 7))
	{
		*pUserPriority = up;
		*pQueIdx = WMM_UP2AC_MAP[up];
	}

	return TRUE;
}


BOOLEAN CheckICMPPacket(RTMP_ADAPTER *pAd, UCHAR *pSrcBuf, UINT8 Direction)
{
	UINT16 TypeLen;
	UCHAR Byte0, Byte1;
	
	if (Direction == 0) /* TX */
	{
		TypeLen = (pSrcBuf[12] << 8) | pSrcBuf[13];
		pSrcBuf += LENGTH_802_3;

		if (TypeLen <= 1500)
		{
			if (pSrcBuf[0] == 0xAA && pSrcBuf[1] == 0xAA && pSrcBuf[2] == 0x03)
			{
				Sniff2BytesFromNdisBuffer((PNDIS_BUFFER)pSrcBuf, 6, &Byte0, &Byte1);
				TypeLen = (USHORT)((Byte0 << 8) + Byte1);
				pSrcBuf += LENGTH_802_1_H; /* Skip this LLC/SNAP header*/
			} else {
				return FALSE;
			}
		}
	
		if (TypeLen == ETH_TYPE_IPv4)
		{
			pSrcBuf += 9;

			if (*pSrcBuf == 0x01)
			{
				pSrcBuf += 11;
	
				if (*pSrcBuf == 0x00)
				{
					pSrcBuf++;
					if (*pSrcBuf == 0x00)
					{
						DBGPRINT(RT_DEBUG_INFO, ("ping reply packet\n"));
						return TRUE;
					}
				} 
				else if (*pSrcBuf == 0x08)
				{
					pSrcBuf++;
					if (*pSrcBuf == 0x00)
					{
						DBGPRINT(RT_DEBUG_INFO, ("ping req packet\n"));
						return TRUE;
					}
				}
			}
		}
	}
	else if (Direction == 1) /* RX */
	{
		pSrcBuf = NdisEqualMemory(SNAP_802_1H, pSrcBuf, 6) ? (pSrcBuf + 6) : pSrcBuf;
		
		TypeLen = (*pSrcBuf << 8) + (*(pSrcBuf + 1));
		
		if (TypeLen == ETH_TYPE_IPv4)
		{
			pSrcBuf += 9;

			if (*pSrcBuf == 0x01)
			{
				pSrcBuf += 11;
				
				if (*pSrcBuf == 0x00)
				{
					pSrcBuf++;
					if (*pSrcBuf == 0x00)
					{
						DBGPRINT(RT_DEBUG_INFO, ("ping reply packet\n"));
						return TRUE;
					}
				} 
				else if (*pSrcBuf == 0x08)
				{
					pSrcBuf++;
					if (*pSrcBuf == 0x00)
					{
						DBGPRINT(RT_DEBUG_INFO, ("ping req packet\n"));
						return TRUE;
					}
				}


			}
		}
	}

	return FALSE;
}


#ifdef SOFT_ENCRYPT
BOOLEAN RTMPExpandPacketForSwEncrypt(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk)
{
	PACKET_INFO PacketInfo;
	UINT32	ex_head = 0, ex_tail = 0;
	UCHAR 	NumberOfFrag = RTMP_GET_PACKET_FRAGMENTS(pTxBlk->pPacket);

#ifdef WAPI_SUPPORT
	if (pTxBlk->CipherAlg == CIPHER_SMS4)
		ex_tail = LEN_WPI_MIC;
	else
#endif /* WAPI_SUPPORT */
	if (pTxBlk->CipherAlg == CIPHER_AES)
		ex_tail = LEN_CCMP_MIC;

	ex_tail = (NumberOfFrag * ex_tail);

	pTxBlk->pPacket = ExpandPacket(pAd, pTxBlk->pPacket, ex_head, ex_tail);
	if (pTxBlk->pPacket == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: out of resource.\n", __FUNCTION__));
		return FALSE;
	}
	RTMP_QueryPacketInfo(pTxBlk->pPacket, &PacketInfo, &pTxBlk->pSrcBufHeader, &pTxBlk->SrcBufLen);

	return TRUE;
}


VOID RTMPUpdateSwCacheCipherInfo(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	IN UCHAR *pHdr)
{
	HEADER_802_11 *pHeader_802_11;
	MAC_TABLE_ENTRY *pMacEntry;

	pHeader_802_11 = (HEADER_802_11 *) pHdr;
	pMacEntry = pTxBlk->pMacEntry;

	if (pMacEntry && pHeader_802_11->FC.Wep &&
		CLIENT_STATUS_TEST_FLAG(pMacEntry, fCLIENT_STATUS_SOFTWARE_ENCRYPT))
	{
		PCIPHER_KEY pKey = &pMacEntry->PairwiseKey;

		TX_BLK_SET_FLAG(pTxBlk, fTX_bSwEncrypt);

		pTxBlk->CipherAlg = pKey->CipherAlg;
		pTxBlk->pKey = pKey;
#ifdef WAPI_SUPPORT
		pTxBlk->KeyIdx = pMacEntry->usk_id;

		/* TSC increment pre encryption transmittion */
		if (pKey->CipherAlg == CIPHER_SMS4)
			inc_iv_byte(pKey->TxTsc, LEN_WAPI_TSC, 2);
		else
#endif /* WAPI_SUPPORT */
		if ((pKey->CipherAlg == CIPHER_WEP64) || (pKey->CipherAlg == CIPHER_WEP128))
			inc_iv_byte(pKey->TxTsc, LEN_WEP_TSC, 1);
		else if ((pKey->CipherAlg == CIPHER_TKIP) || (pKey->CipherAlg == CIPHER_AES))
			inc_iv_byte(pKey->TxTsc, LEN_WPA_TSC, 1);

	}

}


INT tx_sw_encrypt(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk, UCHAR *pHeaderBufPtr, HEADER_802_11 *wifi_hdr)
{
	UCHAR iv_offset = 0, ext_offset = 0;

#ifdef CONFIG_TSO_SUPPORT
	if (pTxBlk->naf_type)
		DBGPRINT(RT_DEBUG_ERROR, ("%s():Error, try to encrypt a pkt marked to use NAF(%d)!\n",
						__FUNCTION__, pTxBlk->naf_type));
#endif /* CONFIG_TSO_SUPPORT */
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
						   pHeaderBufPtr, &iv_offset);
	pHeaderBufPtr += iv_offset;
	// TODO: shiang-MT7603, for header Len, shall we take care that??
	pTxBlk->MpduHeaderLen += iv_offset;

	/* Encrypt the MPDU data by software */
	RTMPSoftEncryptionAction(pAd,
							 pTxBlk->CipherAlg,
							 (UCHAR *)wifi_hdr,
							pTxBlk->pSrcBufData,
							pTxBlk->SrcBufLen,
							pTxBlk->KeyIdx,
							pTxBlk->pKey, &ext_offset);
	pTxBlk->SrcBufLen += ext_offset;
	pTxBlk->TotalFrameLen += ext_offset;

	return TRUE;
}
#endif /* SOFT_ENCRYPT */

#ifdef IP_ASSEMBLY

/*for cache usage to improve throughput*/
static IP_ASSEMBLE_DATA *pCurIpAsmData[NUM_OF_TX_RING];

 static INT rtmp_IpAssembleDataCreate(RTMP_ADAPTER *pAd,UCHAR queId,IP_ASSEMBLE_DATA **ppIpAsmbData,UINT id,UINT fragSize)
{	
	ULONG now = 0;
	IP_ASSEMBLE_DATA *pIpAsmbData = NULL;	
	DL_LIST *pAssHead = &pAd->assebQueue[queId];
	
	os_alloc_mem(NULL,(UCHAR**)&pIpAsmbData,sizeof(IP_ASSEMBLE_DATA));

	*ppIpAsmbData = pIpAsmbData;

	if(pIpAsmbData==NULL)
	{
		return NDIS_STATUS_FAILURE;
	}
	
	InitializeQueueHeader(&pIpAsmbData->queue);	
	NdisGetSystemUpTime(&now);
	pIpAsmbData->identify = id;
	pIpAsmbData->fragSize = fragSize;
	pIpAsmbData->createTime = now;
	
	DlListAdd(pAssHead,&pIpAsmbData->list);
	
	return NDIS_STATUS_SUCCESS;
}


static VOID rtmp_IpAssembleDataDestory(RTMP_ADAPTER *pAd,IP_ASSEMBLE_DATA *pIpAsmbData)
{
	PQUEUE_ENTRY pPktEntry;	
	PNDIS_PACKET pPkt;
		
	/*free queue packet*/
	while (1)
	{
		pPktEntry = RemoveHeadQueue(&pIpAsmbData->queue);
		if (pPktEntry == NULL)
		{
			break;
		}
		pPkt = QUEUE_ENTRY_TO_PACKET(pPktEntry);
		RELEASE_NDIS_PACKET(pAd, pPkt, NDIS_STATUS_FAILURE);
	}
	/*remove from list*/
	DlListDel(&pIpAsmbData->list);
	/*free data*/
	os_free_mem(NULL,pIpAsmbData);	
}


static IP_ASSEMBLE_DATA* rtmp_IpAssembleDataSearch(RTMP_ADAPTER *pAd, UCHAR queIdx, UINT identify)
{
	DL_LIST *pAssHead = &pAd->assebQueue[queIdx];
	IP_ASSEMBLE_DATA *pAssData = NULL;

	DlListForEach(pAssData,pAssHead,struct ip_assemble_data,list)
	{
		if(pAssData->identify == identify)
		{			
			return pAssData;
		}
	}	
	return NULL;
}


static VOID rtmp_IpAssembleDataUpdate(RTMP_ADAPTER *pAd)
{
	DL_LIST *pAssHead = NULL;
	IP_ASSEMBLE_DATA *pAssData = NULL,*pNextAssData=NULL;
	INT i=0;	
	ULONG now = 0;	
	QUEUE_HEADER *pAcQueue = NULL;
	
	NdisGetSystemUpTime(&now);
	for(i=0;i<NUM_OF_TX_RING;i++)
	{
		pAssHead = &pAd->assebQueue[i];
		DlListForEachSafe(pAssData,pNextAssData,pAssHead,struct ip_assemble_data,list)
		{
			pAcQueue = &pAssData->queue;
			if ((pAcQueue->Number != 0) && (RTMP_TIME_AFTER(now, (pAssData->createTime) + (1000 * OS_HZ))))
			{
				if(pCurIpAsmData[i] == pAssData)
				{
					pCurIpAsmData[i] = NULL;
				}
				rtmp_IpAssembleDataDestory(pAd,pAssData);
			}
		}
	}
}


INT rtmp_IpAssembleHandle(RTMP_ADAPTER *pAd,STA_TR_ENTRY *pTrEntry , PNDIS_PACKET pPacket,UCHAR queIdx,PACKET_INFO packetInfo)
{
	IP_ASSEMBLE_DATA *pIpAsmData = NULL;	
	/*define local variable*/
	IP_V4_HDR *pIpv4Hdr, Ipv4Hdr;
	IP_FLAGS_FRAG_OFFSET *pFlagsFragOffset, flagsFragOffset;	
	UINT fragSize = 0;
	QUEUE_HEADER *pAcQueue = NULL;
	UINT32 fragCount = 0;


	/*check all timer of assemble for ageout */
	rtmp_IpAssembleDataUpdate(pAd);

	/*is not ipv4 packet*/
	if (!RTMP_GET_PACKET_IPV4(pPacket))
	{
		/*continue to do normal path*/
		return NDIS_STATUS_INVALID_DATA;
	}

	pFlagsFragOffset = (IP_FLAGS_FRAG_OFFSET *) (packetInfo.pFirstBuffer + LENGTH_802_3 + 6);
	flagsFragOffset.word = ntohs(pFlagsFragOffset->word);
	

	/*is not fragment packet*/
	if(flagsFragOffset.field.flags_more_frag == 0 && flagsFragOffset.field.frag_offset == 0)
	{		
		/*continue to do normal path*/		
		return NDIS_STATUS_INVALID_DATA;
	}

	/*get ipv4 */
	pIpv4Hdr = (IP_V4_HDR *) (packetInfo.pFirstBuffer + LENGTH_802_3);
	Ipv4Hdr.identifier = ntohs(pIpv4Hdr->identifier);
	Ipv4Hdr.tot_len = ntohs(pIpv4Hdr->tot_len);
	Ipv4Hdr.ihl = pIpv4Hdr->ihl;
	fragSize = Ipv4Hdr.tot_len - (Ipv4Hdr.ihl * 4);

	/* check if 1st fragment */
	if ((flagsFragOffset.field.flags_more_frag == 1) && (flagsFragOffset.field.frag_offset == 0))
	{
		/*check current queue is exist this id packet or not*/
		pIpAsmData = rtmp_IpAssembleDataSearch(pAd,queIdx,Ipv4Hdr.identifier);
		/*if not exist, create it*/
		if(!pIpAsmData)
		{
			rtmp_IpAssembleDataCreate(pAd,queIdx,&pIpAsmData,Ipv4Hdr.identifier,fragSize);
			if(!pIpAsmData)
			{
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
				return NDIS_STATUS_FAILURE;
			}
		}

		/*store to  cache */
		pCurIpAsmData[queIdx] = pIpAsmData;
		/*insert packet*/
		pAcQueue = &pIpAsmData->queue;
		InsertTailQueue(pAcQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
			
	} else	
	{
		/*search assemble data from identify and cache first*/
		if(pCurIpAsmData[queIdx] && (pCurIpAsmData[queIdx]->identify == Ipv4Hdr.identifier))
		{
			pIpAsmData = pCurIpAsmData[queIdx];
		}else
		{
			pIpAsmData = rtmp_IpAssembleDataSearch(pAd,queIdx,Ipv4Hdr.identifier);

			/*not create assemble, should drop*/
			if(!pIpAsmData)
			{
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
				return NDIS_STATUS_FAILURE;
			}
			/*update cache*/
			pCurIpAsmData[queIdx] = pIpAsmData;
		}
		pAcQueue = &pIpAsmData->queue;
		InsertTailQueue(pAcQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
		/* check if last fragment */
		if (pIpAsmData && (flagsFragOffset.field.flags_more_frag == 0) && (flagsFragOffset.field.frag_offset != 0))
		{
			/*fragment packets gatter and check*/
			fragCount = ((flagsFragOffset.field.frag_offset * 8) / (pIpAsmData->fragSize)) + 1;
			
			if (pAcQueue->Number != fragCount)
			{
				rtmp_IpAssembleDataDestory(pAd,pIpAsmData);
				pCurIpAsmData[queIdx] = NULL;
				return NDIS_STATUS_FAILURE;
			}

			/* move backup fragments to software queue */
			if (rtmp_enq_req(pAd, NULL, queIdx, pTrEntry,FALSE,pAcQueue) == FALSE) 
			{
				rtmp_IpAssembleDataDestory(pAd,pIpAsmData);
				pCurIpAsmData[queIdx] = NULL;
				return NDIS_STATUS_FAILURE;
			}			
			rtmp_IpAssembleDataDestory(pAd,pIpAsmData);
			pCurIpAsmData[queIdx] = NULL;
		}
		
	}
	return NDIS_STATUS_SUCCESS;		
}
#endif


INT rtmp_tx_burst_set(RTMP_ADAPTER *pAd)
{
	
#ifdef DOT11_N_SUPPORT
#ifdef MT_MAC
			
#ifdef RTMP_SDIO_SUPPORT
	if(pAd->MCUCtrl.Stage == FW_RUN_TIME){
#endif
	if (pAd->chipCap.hif_type == HIF_MT)
	{
		/* Not RDG, update the TxOP else keep the default RDG's TxOP */
		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE) == FALSE)
		{
			if ((pAd->MacTab.Size == 1) && (pAd->CommonCfg.bEnableTxBurst))
			{ /* Only update TxOP for racing */
				AsicUpdateTxOP(pAd, WMM_PARAM_AC_1, 0x60);
			}
			else
			{
				AsicUpdateTxOP(pAd, WMM_PARAM_AC_1, 0x0);
			}
		}
	}
#ifdef RTMP_SDIO_SUPPORT
	}
#endif
			
#endif /* MT_MAC */
#endif /* DOT11_N_SUPPORT */

	return NDIS_STATUS_SUCCESS;
}

