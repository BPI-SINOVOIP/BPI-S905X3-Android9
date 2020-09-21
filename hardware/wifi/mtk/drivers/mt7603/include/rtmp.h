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
    rtmp.h

    Abstract:
    Miniport generic portion header file

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------

*/
#ifndef __RTMP_H__
#define __RTMP_H__

#include "link_list.h"
#include "spectrum_def.h"

#include "rtmp_dot11.h"
#include "wpa_cmm.h"

#ifdef CONFIG_AP_SUPPORT
#include "ap_autoChSel_cmm.h"
#endif /* CONFIG_AP_SUPPORT */

#include "wsc.h"
#ifdef MAT_SUPPORT
#include "mat.h"
#endif /* MAT_SUPPORT */

#ifdef MESH_SUPPORT
#include "mesh_def.h"
#endif /* MESH_SUPPORT */

#ifdef WAPI_SUPPORT
#include "wapi_def.h"
#endif /* WAPI_SUPPORT */

#include "rtmp_chip.h"

#ifdef DOT11R_FT_SUPPORT
#include "ft_cmm.h"
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
#include "rrm_cmm.h"
#endif /* DOT11K_RRM_SUPPORT */

#ifdef DOT11W_PMF_SUPPORT
#include "pmf_cmm.h"
#endif /* DOT11W_PMF_SUPPORT */

#ifdef DOT11Z_TDLS_SUPPORT
#include "tdls_cmm.h"
#endif /* DOT11Z_TDLS_SUPPORT */

#ifdef CLIENT_WDS
#include "client_wds_cmm.h"
#endif /* CLIENT_WDS */

#ifdef DOT11V_WNM_SUPPORT
#include "wnm_cmm.h"
#endif /* DOT11V_WNM_SUPPORT */

#ifdef BT_COEXISTENCE_SUPPORT
#include	"misc_cmm.h"
#endif /* BT_COEXISTENCE_SUPPORT */

#ifdef P2P_SUPPORT
#include "p2p_cmm.h"
#endif /* P2P_SUPPORT */

#ifdef RT_CFG80211_SUPPORT
#include "cfg80211_cmm.h"
#endif /* RT_CFG80211_SUPPORT */

#ifdef WFD_SUPPORT
#include "wfd_cmm.h"
#endif /* WFD_SUPPORT */

#include "drs_extr.h"

struct _RTMP_RA_LEGACY_TB;

typedef struct _RTMP_ADAPTER RTMP_ADAPTER;
typedef struct _RTMP_ADAPTER *PRTMP_ADAPTER;

typedef struct _RTMP_CHIP_OP RTMP_CHIP_OP;
typedef struct _RTMP_CHIP_CAP RTMP_CHIP_CAP;

#ifdef BB_SOC
#include "os/bb_soc.h"
#endif

#include "mcu/mcu.h"

#ifdef CONFIG_M8051_SUPPORT
#include "mcu/mcu_51.h"
#endif

#ifdef CONFIG_ANDES_SUPPORT
#include "mcu/andes_core.h"
#endif

#include "radar.h"

#ifdef CARRIER_DETECTION_SUPPORT
#include "cs.h"
#endif /* CARRIER_DETECTION_SUPPORT */

#ifdef DFS_SUPPORT
#include "dfs.h"
#endif /* DFS_SUPPORT */

#ifdef LED_CONTROL_SUPPORT
#include "rt_led.h"
#endif /* LED_CONTROL_SUPPORT */

#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
#include "frq_cal.h"
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */

#ifdef CONFIG_ATE
#include "ate.h"
#endif

#ifdef CONFIG_DOT11U_INTERWORKING
#include "dot11u_interworking.h"
#include "gas.h"
#endif

#ifdef CONFIG_DOT11V_WNM
#include "dot11v_wnm.h"
#include "wnm.h"
#endif

#ifdef CONFIG_HOTSPOT
#include "hotspot.h"
#endif

#ifdef CONFIG_SNIFFER_SUPPORT
#include "sniffer/sniffer.h"
#endif

#ifdef MT76XX_BTCOEX_SUPPORT
#include "mcu/btcoex.h"
#endif /*MT76XX_BTCOEX_SUPPORT*/

#ifdef MT_MAC
#include "cmm_asic_mt.h"
#endif /* MT_MAC */

// TODO: shiang-6590, remove it after ATE fully re-organized! copy from rtmp_bbp.h
#ifndef MAX_BBP_ID
#ifdef RTMP_RBUS_SUPPORT
/* TODO: for this definition, need to modify it!! */
	/*#define MAX_BBP_ID	255 */
	#define MAX_BBP_ID	200

#if defined(RT3352) || defined(RT5350)
	#undef MAX_BBP_ID
	#define MAX_BBP_ID	255
#endif /* RT3352 */
#else
#if defined(RT5370) || defined(RT5390) || defined(RT3290) //for hw antenna diversity (PPAD)
	#define MAX_BBP_ID	255
#elif defined(RT30xx)
	/* edit by johnli, RF power sequence setup, add BBP R138 for ADC dynamic on/off control */
#ifdef RT5592
	#define MAX_BBP_ID	255
#else
	#define MAX_BBP_ID	185
#endif
#elif defined(RT2883)
	#define MAX_BBP_ID	180
#else
	#define MAX_BBP_ID	136
#endif /* RT30xx */

#endif /* RTMP_RBUS_SUPPORT */


#if defined(RT5572) || defined(RT6352)
#undef MAX_BBP_ID
#define MAX_BBP_ID	248
#undef MAX_BBP_MSG_SIZE
#define MAX_BBP_MSG_SIZE 4096
#endif /* defined(RT5572) || defined(RT6352) */
#endif
// TODO: ---End


#ifdef WIDI_SUPPORT
/* This defines after how many channels you want to go into listen channel.*/
#define TOGGLE_TO_WIFIDIRECT_LISTEN_CHANNEL 15
#endif /* WIDI_SUPPORT */

/*+++Used for merge MiniportMMRequest() and MiniportDataMMRequest() into one function */
#define MGMT_USE_QUEUE_FLAG	0x80
#define MGMT_USE_PS_FLAG	0x40
/*---Used for merge MiniportMMRequest() and MiniportDataMMRequest() into one function */
/* The number of channels for per-channel Tx power offset */


#define	MAXSEQ		(0xFFF)

#ifdef DOT11N_SS3_SUPPORT
#define MAX_MCS_SET 24		/* From MCS 0 ~ MCS 23 */
#else
#define MAX_MCS_SET 16		/* From MCS 0 ~ MCS 15 */
#endif /* DOT11N_SS3_SUPPORT */


#define MAX_TXPOWER_ARRAY_SIZE	5
#ifdef RT2883
#undef MAX_TXPOWER_ARRAY_SIZE
#define MAX_TXPOWER_ARRAY_SIZE	7
#endif /* RT2883 */
#ifdef RT3883
#undef MAX_TXPOWER_ARRAY_SIZE
#define MAX_TXPOWER_ARRAY_SIZE	10
#endif /* RT3883 */

#define MAX_EEPROM_BUFFER_SIZE	1024
#define PS_RETRIEVE_TOKEN        0x76

extern unsigned char CISCO_OUI[];
extern UCHAR BaSizeArray[4];

extern UCHAR BROADCAST_ADDR[MAC_ADDR_LEN];
extern UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN];
extern char *CipherName[];
extern UCHAR SNAP_802_1H[6];
extern UCHAR SNAP_BRIDGE_TUNNEL[6];
extern UCHAR EAPOL[2];
extern UCHAR IPX[2];
extern UCHAR TPID[];
extern UCHAR APPLE_TALK[2];
#ifdef DOT11Z_TDLS_SUPPORT
extern UCHAR TDLS_LLC_SNAP_WITH_CATEGORY[10];
#ifdef WFD_SUPPORT
extern UCHAR TDLS_LLC_SNAP_WITH_WFD_CATEGORY[10];
#endif /* WFD_SUPPORT */
extern UCHAR TDLS_ETHERTYPE[2];
#endif /* DOT11Z_TDLS_SUPPORT */
extern UCHAR OfdmRateToRxwiMCS[];
extern UCHAR WMM_UP2AC_MAP[8];

extern unsigned char RateIdToMbps[];
extern USHORT RateIdTo500Kbps[];

extern UCHAR CipherSuiteWpaNoneTkip[];
extern UCHAR CipherSuiteWpaNoneTkipLen;

extern UCHAR CipherSuiteWpaNoneAes[];
extern UCHAR CipherSuiteWpaNoneAesLen;

extern UCHAR SsidIe;
extern UCHAR SupRateIe;
extern UCHAR ExtRateIe;

#ifdef DOT11_N_SUPPORT
extern UCHAR HtCapIe;
extern UCHAR AddHtInfoIe;
extern UCHAR NewExtChanIe;
extern UCHAR BssCoexistIe;
extern UCHAR ExtHtCapIe;
#endif /* DOT11_N_SUPPORT */
extern UCHAR ExtCapIe;

extern UCHAR ErpIe;
extern UCHAR DsIe;
extern UCHAR TimIe;
extern UCHAR WpaIe;
extern UCHAR Wpa2Ie;
extern UCHAR IbssIe;
extern UCHAR WapiIe;

extern UCHAR WPA_OUI[];
extern UCHAR RSN_OUI[];
extern UCHAR WAPI_OUI[];
extern UCHAR WME_INFO_ELEM[];
extern UCHAR WME_PARM_ELEM[];
extern UCHAR RALINK_OUI[];
extern UCHAR PowerConstraintIE[];

struct _RX_BLK;
struct raw_rssi_info;

typedef struct _UAPSD_INFO {
	BOOLEAN bAPSDCapable;
} UAPSD_INFO;


typedef union _CAPTURE_MODE_PACKET_BUFFER {
	struct
	{
		UINT32       BYTE0:8;
		UINT32       BYTE1:8;
		UINT32       BYTE2:8;
		UINT32       BYTE3:8;
	} field;
	UINT32                   Value;
}CAPTURE_MODE_PACKET_BUFFER;


/*
	Queue structure and macros
*/
#define InitializeQueueHeader(QueueHeader)              \
{                                                       \
	(QueueHeader)->Head = (QueueHeader)->Tail = NULL;   \
	(QueueHeader)->Number = 0;                          \
}

#define RemoveHeadQueue(QueueHeader)                \
(QueueHeader)->Head;                                \
{                                                   \
	PQUEUE_ENTRY pNext;                             \
	if ((QueueHeader)->Head != NULL)				\
	{												\
		pNext = (QueueHeader)->Head->Next;          \
		(QueueHeader)->Head->Next = NULL;		\
		(QueueHeader)->Head = pNext;                \
		if (pNext == NULL)                          \
			(QueueHeader)->Tail = NULL;             \
		(QueueHeader)->Number--;                    \
	}												\
}

#define RemoveTailQueue(QueueHeader)       \
(QueueHeader)->Tail;                       \
{                                          \
	PQUEUE_ENTRY pNext;                     \
	if ((QueueHeader)->Head != NULL)			 \
	{                                       \
      pNext = (QueueHeader)->Head;         \
      if (pNext->Next == NULL) {           \
			(QueueHeader)->Head = NULL;       \
			(QueueHeader)->Tail = NULL;       \
      } else {                             \
         while (pNext->Next != (QueueHeader)->Tail) \
         {                                 \
            pNext = pNext->Next;           \
         }                                 \
         (QueueHeader)->Tail = pNext;      \
         pNext->Next = NULL;               \
      }                                    \
      (QueueHeader)->Number--;              \
   }                                        \
}


#define InsertHeadQueue(QueueHeader, QueueEntry)            \
{                                                           \
		((PQUEUE_ENTRY)QueueEntry)->Next = (QueueHeader)->Head; \
		(QueueHeader)->Head = (PQUEUE_ENTRY)(QueueEntry);       \
		if ((QueueHeader)->Tail == NULL)                        \
			(QueueHeader)->Tail = (PQUEUE_ENTRY)(QueueEntry);   \
		(QueueHeader)->Number++;                                \
}

#define InsertTailQueue(QueueHeader, QueueEntry)				\
{                                                               \
	((PQUEUE_ENTRY)QueueEntry)->Next = NULL;                    \
	if ((QueueHeader)->Tail)                                    \
		(QueueHeader)->Tail->Next = (PQUEUE_ENTRY)(QueueEntry); \
	else                                                        \
		(QueueHeader)->Head = (PQUEUE_ENTRY)(QueueEntry);       \
	(QueueHeader)->Tail = (PQUEUE_ENTRY)(QueueEntry);           \
	(QueueHeader)->Number++;                                    \
}

#define InsertTailQueueAc(pAd, pEntry, QueueHeader, QueueEntry)			\
{																		\
	((PQUEUE_ENTRY)QueueEntry)->Next = NULL;							\
	if ((QueueHeader)->Tail)											\
		(QueueHeader)->Tail->Next = (PQUEUE_ENTRY)(QueueEntry);			\
	else																\
		(QueueHeader)->Head = (PQUEUE_ENTRY)(QueueEntry);				\
	(QueueHeader)->Tail = (PQUEUE_ENTRY)(QueueEntry);					\
	(QueueHeader)->Number++;											\
}

#ifdef WMM_ACM_SUPPORT

#undef InsertTailQueueAc

#ifdef RELEASE_EXCLUDE
/*
	From Hamilton, Mark [Mark.Hamilton@Polycom.com], he says that
	The point of admission control is that the AP is "guaranteeing" that the
	stream will be given the time it requested, if it tries to use it - at
	least in the sense of that stream being protected from unadmitted and
	other admitted streams.  (That is, other forces, like medium contention
	problems, retry rates, interference, overlapping BSS, etc. can all cause
	a stream to not get its allocated bandwidth.  But, to fail to get its
	allocation simply because the AP over-allocated is not correct behavior.)
*/
#endif /* RELEASE_EXCLUDE */

/* we need to check if the packet can be put to the queue based on ACM */
#error "shiang-usw, Fix me because pEntry is not revise yet!!"
#define InsertTailQueueAc(pAd, pEntry, QueueHeader, QueueEntry)				\
{																			\
	PQUEUE_HEADER __QueueHeaderNew;											\
	ACM_FUNC_STATUS __Status = ACM_RTN_NO_ACM;								\
	UINT32 __QueueType;														\
	BOOLEAN __FlgIsPktFreed = FALSE;										\
																			\
	ACMR_PKT_MARK_MIN_PHY_MODE(QUEUE_ENTRY_TO_PACKET(QueueEntry), ACMR_PHY_NONE);\
	RTMP_SET_PACKET_TX_TIME(QUEUE_ENTRY_TO_PACKET(QueueEntry), 0);			\
		if (ACMP_IsNeedToDoAcm(pAd, pEntry, RTMP_GET_PACKET_UP(				\
								QUEUE_ENTRY_TO_PACKET(QueueEntry))) == TRUE)\
		{																	\
			ACMR_PKT_QOS_TYPE_SET(QUEUE_ENTRY_TO_PACKET(QueueEntry), 0);	\
			__Status = ACMP_DataPacketQueue(pAd, pEntry, 					\
					QUEUE_ENTRY_TO_PACKET(QueueEntry), 0, &__QueueType);	\
																			\
			if ((__Status == ACM_RTN_FAIL) ||									\
				((__Status == ACM_RTN_OK) &&									\
				(pAd->TxSwQueue[__QueueType].Number >= pAd->TxSwQMaxLen)))	\
			{																	\
				__FlgIsPktFreed = TRUE;										\
																			\
				if ((__Status == ACM_RTN_OK) &&								\
					(__QueueType == ACMR_QID_AC_BE)	&&						\
					(ACMP_BE_QueueFullHandle(pAd,							\
											pEntry,							\
						QUEUE_ENTRY_TO_PACKET(QueueEntry)) == ACM_RTN_OK))	\
				{															\
					__FlgIsPktFreed = FALSE;								\
				}															\
																			\
				if (__FlgIsPktFreed == TRUE)								\
				{															\
					RELEASE_NDIS_PACKET(pAd,								\
										QUEUE_ENTRY_TO_PACKET(QueueEntry),	\
										NDIS_STATUS_FAILURE);				\
				}															\
			}																\
		}																	\
	if (__FlgIsPktFreed == FALSE)											\
	{																		\
		if (__Status == ACM_RTN_NO_ACM)										\
			__QueueHeaderNew = QueueHeader;									\
		else																\
			__QueueHeaderNew = &pAd->TxSwQueue[__QueueType];				\
		((PQUEUE_ENTRY)QueueEntry)->Next = NULL;							\
		if ((__QueueHeaderNew)->Tail)										\
			(__QueueHeaderNew)->Tail->Next = (PQUEUE_ENTRY)(QueueEntry);	\
		else																\
			(__QueueHeaderNew)->Head = (PQUEUE_ENTRY)(QueueEntry);			\
		(__QueueHeaderNew)->Tail = (PQUEUE_ENTRY)(QueueEntry);				\
		(__QueueHeaderNew)->Number++;										\
	}																		\
}
#endif /* WMM_ACM_SUPPORT */

/* */
/*  Macros for flag and ref count operations */
/* */
#define RTMP_SET_FLAG(_M, _F)       ((_M)->Flags |= (_F))
#define RTMP_CLEAR_FLAG(_M, _F)     ((_M)->Flags &= ~(_F))
#define RTMP_CLEAR_FLAGS(_M)        ((_M)->Flags = 0)
#define RTMP_TEST_FLAG(_M, _F)      (((_M)->Flags & (_F)) != 0)
#define RTMP_TEST_FLAGS(_M, _F)     (((_M)->Flags & (_F)) == (_F))
/* Macro for power save flag. */
#define RTMP_SET_PSFLAG(_M, _F)       ((_M)->PSFlags |= (_F))
#define RTMP_CLEAR_PSFLAG(_M, _F)     ((_M)->PSFlags &= ~(_F))
#define RTMP_CLEAR_PSFLAGS(_M)        ((_M)->PSFlags = 0)
#define RTMP_TEST_PSFLAG(_M, _F)      (((_M)->PSFlags & (_F)) != 0)
#define RTMP_TEST_PSFLAGS(_M, _F)     (((_M)->PSFlags & (_F)) == (_F))

#define OPSTATUS_SET_FLAG(_pAd, _F)     ((_pAd)->CommonCfg.OpStatusFlags |= (_F))
#define OPSTATUS_CLEAR_FLAG(_pAd, _F)   ((_pAd)->CommonCfg.OpStatusFlags &= ~(_F))
#define OPSTATUS_TEST_FLAG(_pAd, _F)    (((_pAd)->CommonCfg.OpStatusFlags & (_F)) != 0)

#define WIFI_TEST_SET_FLAG(_pAd, _F)     ((_pAd)->CommonCfg.WiFiTestFlags |= (_F))
#define WIFI_TEST_CLEAR_FLAG(_pAd, _F)   ((_pAd)->CommonCfg.WiFiTestFlags &= ~(_F))
#define WIFI_TEST_CHECK_FLAG(_pAd, _F)    (((_pAd)->CommonCfg.WiFiTestFlags & (_F)) != 0)

#define CLIENT_STATUS_SET_FLAG(_pEntry,_F)      ((_pEntry)->ClientStatusFlags |= (_F))
#define CLIENT_STATUS_CLEAR_FLAG(_pEntry,_F)    ((_pEntry)->ClientStatusFlags &= ~(_F))
#define CLIENT_STATUS_TEST_FLAG(_pEntry,_F)     (((_pEntry)->ClientStatusFlags & (_F)) != 0)

#define CLIENT_CAP_SET_FLAG(_pEntry,_F)      ((_pEntry)->cli_cap_flags |= (_F))
#define CLIENT_CAP_CLEAR_FLAG(_pEntry,_F)    ((_pEntry)->cli_cap_flags &= ~(_F))
#define CLIENT_CAP_TEST_FLAG(_pEntry,_F)     (((_pEntry)->cli_cap_flags & (_F)) != 0)


#define RX_FILTER_SET_FLAG(_pAd, _F)    ((_pAd)->CommonCfg.PacketFilter |= (_F))
#define RX_FILTER_CLEAR_FLAG(_pAd, _F)  ((_pAd)->CommonCfg.PacketFilter &= ~(_F))
#define RX_FILTER_TEST_FLAG(_pAd, _F)   (((_pAd)->CommonCfg.PacketFilter & (_F)) != 0)

#define RTMP_SET_MORE_FLAG(_M, _F)       ((_M)->MoreFlags |= (_F))
#define RTMP_TEST_MORE_FLAG(_M, _F)      (((_M)->MoreFlags & (_F)) != 0)
#define RTMP_CLEAR_MORE_FLAG(_M, _F)     ((_M)->MoreFlags &= ~(_F))

#define SET_ASIC_CAP(_pAd, _caps)		((_pAd)->chipCap.asic_caps |= (_caps))
#define IS_ASIC_CAP(_pAd, _caps)			(((_pAd)->chipCap.asic_caps & (_caps)) != 0)
#define CLR_ASIC_CAP(_pAd, _caps)		((_pAd)->chipCap.asic_caps &= ~(_caps))

#define TX_FLAG_STOP_DEQUEUE	(fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS |\
								fRTMP_ADAPTER_RADIO_OFF |\
								fRTMP_ADAPTER_RESET_IN_PROGRESS |\
								fRTMP_ADAPTER_HALT_IN_PROGRESS |\
								fRTMP_ADAPTER_NIC_NOT_EXIST |\
								fRTMP_ADAPTER_DISABLE_DEQUEUEPACKET)

#define INC_RING_INDEX(_idx, _RingSize)    \
{                                          \
    (_idx) = (_idx+1) % (_RingSize);       \
}

#define TR_ENQ_COUNT_INC(tr) \
{								\
	tr->enqCount++;				\
}

#define TR_ENQ_COUNT_DEC(tr) \
{								\
	tr->enqCount--;				\
}

#define TR_TOKEN_COUNT_INC(tr, qid) \
{								\
	tr->TokenCount[qid]++;				\
}

#define TR_TOKEN_COUNT_DEC(tr, qid) \
{								\
	tr->TokenCount[qid]--;				\
}


#define SQ_ENQ_PS_MAX 		512
#define SQ_ENQ_NORMAL_MAX 	512
#define SQ_ENQ_RESERVE_PERAC 	(SQ_ENQ_NORMAL_MAX/2)

#ifdef USB_BULK_BUF_ALIGMENT
#define CUR_WRITE_IDX_INC(_idx, _RingSize)    \
{                                          \
    (_idx) = (_idx+1) % (_RingSize);       \
}
#endif /* USB_BULK_BUF_ALIGMENT */


/*
	Common fragment list structure -  Identical to the scatter gather frag list structure
*/
#define NIC_MAX_PHYS_BUF_COUNT              8

typedef struct _RTMP_SCATTER_GATHER_ELEMENT {
	PVOID Address;
	ULONG Length;
	PULONG Reserved;
} RTMP_SCATTER_GATHER_ELEMENT, *PRTMP_SCATTER_GATHER_ELEMENT;

typedef struct _RTMP_SCATTER_GATHER_LIST {
	ULONG NumberOfElements;
	PULONG Reserved;
	RTMP_SCATTER_GATHER_ELEMENT Elements[NIC_MAX_PHYS_BUF_COUNT];
} RTMP_SCATTER_GATHER_LIST, *PRTMP_SCATTER_GATHER_LIST;


/*
	Some utility macros
*/
#define GET_LNA_GAIN(_pAd)	((_pAd->LatchRfRegs.Channel <= 14) ? (_pAd->BLNAGain) : ((_pAd->LatchRfRegs.Channel <= 64) ? (_pAd->ALNAGain0) : ((_pAd->LatchRfRegs.Channel <= 128) ? (_pAd->ALNAGain1) : (_pAd->ALNAGain2))))

#define INC_COUNTER64(Val)          (Val.QuadPart++)

#define INFRA_ON(_p)                (OPSTATUS_TEST_FLAG(_p, fOP_STATUS_INFRA_ON))
#define ADHOC_ON(_p)                (OPSTATUS_TEST_FLAG(_p, fOP_STATUS_ADHOC_ON))
#ifdef CONFIG_STA_SUPPORT
#define MONITOR_ON(_p)              (((_p)->StaCfg.BssType) == BSS_MONITOR)
#else
#define MONITOR_ON(_p)              (((_p)->ApCfg.BssType) == BSS_MONITOR)
#endif

#define IDLE_ON(_p)                 (!INFRA_ON(_p) && !ADHOC_ON(_p))

/* Check LEAP & CCKM flags */
#define LEAP_ON(_p)                 (((_p)->StaCfg.LeapAuthMode) == CISCO_AuthModeLEAP)
#define LEAP_CCKM_ON(_p)            ((((_p)->StaCfg.LeapAuthMode) == CISCO_AuthModeLEAP) && ((_p)->StaCfg.LeapAuthInfo.CCKM == TRUE))

/* if orginal Ethernet frame contains no LLC/SNAP, then an extra LLC/SNAP encap is required */
#define EXTRA_LLCSNAP_ENCAP_FROM_PKT_START(_pBufVA, _pExtraLlcSnapEncap)		\
{																\
	if (((*(_pBufVA + 12) << 8) + *(_pBufVA + 13)) > 1500)		\
	{															\
		_pExtraLlcSnapEncap = SNAP_802_1H;						\
		if (NdisEqualMemory(IPX, _pBufVA + 12, 2) || 			\
			NdisEqualMemory(APPLE_TALK, _pBufVA + 12, 2))		\
		{														\
			_pExtraLlcSnapEncap = SNAP_BRIDGE_TUNNEL;			\
		}														\
	}															\
	else														\
	{															\
		_pExtraLlcSnapEncap = NULL;								\
	}															\
}

/* New Define for new Tx Path. */
#define EXTRA_LLCSNAP_ENCAP_FROM_PKT_OFFSET(_pBufVA, _pExtraLlcSnapEncap)	\
{																\
	if (((*(_pBufVA) << 8) + *(_pBufVA + 1)) > 1500)			\
	{															\
		_pExtraLlcSnapEncap = SNAP_802_1H;						\
		if (NdisEqualMemory(IPX, _pBufVA, 2) || 				\
			NdisEqualMemory(APPLE_TALK, _pBufVA, 2))			\
		{														\
			_pExtraLlcSnapEncap = SNAP_BRIDGE_TUNNEL;			\
		}														\
	}															\
	else														\
	{															\
		_pExtraLlcSnapEncap = NULL;								\
	}															\
}

#define MAKE_802_3_HEADER(_buf, _pMac1, _pMac2, _pType)                   \
{                                                                       \
    NdisMoveMemory(_buf, _pMac1, MAC_ADDR_LEN);                           \
    NdisMoveMemory((_buf + MAC_ADDR_LEN), _pMac2, MAC_ADDR_LEN);          \
    NdisMoveMemory((_buf + MAC_ADDR_LEN * 2), _pType, LENGTH_802_3_TYPE); \
}

/*
	if pData has no LLC/SNAP (neither RFC1042 nor Bridge tunnel),
		keep it that way.
	else if the received frame is LLC/SNAP-encaped IPX or APPLETALK,
		preserve the LLC/SNAP field
	else remove the LLC/SNAP field from the result Ethernet frame

	Patch for WHQL only, which did not turn on Netbios but use IPX within its payload
	Note:
		_pData & _DataSize may be altered (remove 8-byte LLC/SNAP) by this MACRO
		_pRemovedLLCSNAP: pointer to removed LLC/SNAP; NULL is not removed
*/
#define CONVERT_TO_802_3(_p8023hdr, _pDA, _pSA, _pData, _DataSize, _pRemovedLLCSNAP)      \
{                                                                       \
    char LLC_Len[2];                                                    \
                                                                        \
    _pRemovedLLCSNAP = NULL;                                            \
    if (NdisEqualMemory(SNAP_802_1H, _pData, 6)  ||                     \
        NdisEqualMemory(SNAP_BRIDGE_TUNNEL, _pData, 6))                 \
    {                                                                   \
        PUCHAR pProto = _pData + 6;                                     \
                                                                        \
        if ((NdisEqualMemory(IPX, pProto, 2) || NdisEqualMemory(APPLE_TALK, pProto, 2)) &&  \
            NdisEqualMemory(SNAP_802_1H, _pData, 6))                    \
        {                                                               \
            LLC_Len[0] = (UCHAR)(_DataSize >> 8);                       \
            LLC_Len[1] = (UCHAR)(_DataSize & (256 - 1));                \
            MAKE_802_3_HEADER(_p8023hdr, _pDA, _pSA, LLC_Len);          \
        }                                                               \
        else                                                            \
        {                                                               \
            MAKE_802_3_HEADER(_p8023hdr, _pDA, _pSA, pProto);           \
            _pRemovedLLCSNAP = _pData;                                  \
            _DataSize -= LENGTH_802_1_H;                                \
            _pData += LENGTH_802_1_H;                                   \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        LLC_Len[0] = (UCHAR)(_DataSize >> 8);                           \
        LLC_Len[1] = (UCHAR)(_DataSize & (256 - 1));                    \
        MAKE_802_3_HEADER(_p8023hdr, _pDA, _pSA, LLC_Len);              \
    }                                                                   \
}

/*
	Enqueue this frame to MLME engine
	We need to enqueue the whole frame because MLME need to pass data type
	information from 802.11 header
*/
#define REPORT_MGMT_FRAME_TO_MLME(_pAd, Wcid, _pFrame, _FrameSize, _Rssi0, _Rssi1, _Rssi2, _MinSNR, _OpMode)        \
do{                                                                                       \
	struct raw_rssi_info _rssi_info;\
	_rssi_info.raw_rssi[0] = _Rssi0;\
	_rssi_info.raw_rssi[1] = _Rssi1;\
	_rssi_info.raw_rssi[2] = _Rssi2;\
	_rssi_info.raw_snr = _MinSNR;\
	MlmeEnqueueForRecv(_pAd, Wcid, &_rssi_info, _FrameSize, _pFrame, _OpMode);   \
}while(0)


#define IPV4_ADDR_EQUAL(pAddr1, pAddr2)         RTMPEqualMemory((PVOID)(pAddr1), (PVOID)(pAddr2), 4)
#define IPV6_ADDR_EQUAL(pAddr1, pAddr2)         RTMPEqualMemory((PVOID)(pAddr1), (PVOID)(pAddr2), 16)
#define MAC_ADDR_EQUAL(pAddr1,pAddr2)           RTMPEqualMemory((PVOID)(pAddr1), (PVOID)(pAddr2), MAC_ADDR_LEN)
#define SSID_EQUAL(ssid1, len1, ssid2, len2)    ((len1==len2) && (RTMPEqualMemory(ssid1, ssid2, len1)))


/*
	Data buffer for DMA operation, the buffer must be contiguous physical memory
	Both DMA to / from CPU use the same structure.
*/
typedef struct _RTMP_DMABUF {
	ULONG AllocSize;
	PVOID AllocVa;		/* TxBuf virtual address */
	NDIS_PHYSICAL_ADDRESS AllocPa;	/* TxBuf physical address */
} RTMP_DMABUF, *PRTMP_DMABUF;


/*
	Control block (Descriptor) for all ring descriptor DMA operation, buffer must be
	contiguous physical memory. NDIS_PACKET stored the binding Rx packet descriptor
	which won't be released, driver has to wait until upper layer return the packet
	before giveing up this rx ring descriptor to ASIC. NDIS_BUFFER is assocaited pair
	to describe the packet buffer. For Tx, NDIS_PACKET stored the tx packet descriptor
	which driver should ACK upper layer when the tx is physically done or failed.
*/
typedef struct _RTMP_DMACB {
	ULONG AllocSize;	/* Control block size */
	PVOID AllocVa;		/* Control block virtual address */
	NDIS_PHYSICAL_ADDRESS AllocPa;	/* Control block physical address */
	PNDIS_PACKET pNdisPacket;
	PNDIS_PACKET pNextNdisPacket;
	NDIS_PHYSICAL_ADDRESS PacketPa;
	RTMP_DMABUF DmaBuf;	/* Associated DMA buffer structure */
#ifdef CACHE_LINE_32B
	RXD_STRUC LastBDInfo;
#endif /* CACHE_LINE_32B */
} RTMP_DMACB, *PRTMP_DMACB;

BOOLEAN MonitorRxRing(struct _RTMP_ADAPTER *pAd);
BOOLEAN MonitorTxRing(struct _RTMP_ADAPTER *pAd);
BOOLEAN MonitorRxPse(struct _RTMP_ADAPTER *pAd);

typedef struct _RTMP_TX_RING {
	RTMP_DMACB Cell[TX_RING_SIZE];
	UINT32 TxCpuIdx;
	UINT32 TxDmaIdx;
	UINT32 TxSwFreeIdx;	/* software next free tx index */
	UINT32 hw_desc_base;
	UINT32 hw_cidx_addr;
	UINT32 hw_didx_addr;
	UINT32 hw_cnt_addr;
} RTMP_TX_RING;

typedef struct _RTMP_RX_RING {
	RTMP_DMACB Cell[RX_RING_SIZE];
	UINT32 RxCpuIdx;
	UINT32 RxDmaIdx;
	INT32 RxSwReadIdx;	/* software next read index */
	UINT32 hw_desc_base;
	UINT32 hw_cidx_addr;
	UINT32 hw_didx_addr;
	UINT32 hw_cnt_addr;
} RTMP_RX_RING;

typedef struct _RTMP_MGMT_RING {
	RTMP_DMACB Cell[MGMT_RING_SIZE];
	UINT32 TxCpuIdx;
	UINT32 TxDmaIdx;
	UINT32 TxSwFreeIdx;	/* software next free tx index */
	UINT32 hw_desc_base;
	UINT32 hw_cidx_addr;
	UINT32 hw_didx_addr;
	UINT32 hw_cnt_addr;
} RTMP_MGMT_RING;

typedef struct _RTMP_CTRL_RING {
	RTMP_DMACB Cell[MGMT_RING_SIZE];
	UINT32 TxCpuIdx;
	UINT32 TxDmaIdx;
	UINT32 TxSwFreeIdx;	/* software next free tx index */
	UINT32 hw_desc_base;
	UINT32 hw_cidx_addr;
	UINT32 hw_didx_addr;
	UINT32 hw_cnt_addr;
} RTMP_CTRL_RING;

#ifdef MT_MAC
typedef struct _RTMP_BCN_RING {
	RTMP_DMACB Cell[BCN_RING_SIZE];
	UINT32 TxCpuIdx;
	UINT32 TxDmaIdx;
	UINT32 TxSwFreeIdx;	/* software next free tx index */
	UINT32 hw_desc_base;
	UINT32 hw_cidx_addr;
	UINT32 hw_didx_addr;
	UINT32 hw_cnt_addr;
} RTMP_BCN_RING;
#endif /* MT_MAC */

/*
	Statistic counter structure
*/
typedef struct _COUNTER_802_3 {
	/* General Stats */
	ULONG GoodTransmits;
	ULONG GoodReceives;
	ULONG TxErrors;
	ULONG RxErrors;
	ULONG RxNoBuffer;
} COUNTER_802_3, *PCOUNTER_802_3;

typedef struct _COUNTER_802_11 {
	ULONG Length;
/*	LARGE_INTEGER   LastTransmittedFragmentCount; */
	LARGE_INTEGER TransmittedFragmentCount;
	LARGE_INTEGER MulticastTransmittedFrameCount;
	LARGE_INTEGER FailedCount;
	LARGE_INTEGER RetryCount;
	LARGE_INTEGER MultipleRetryCount;
	LARGE_INTEGER RTSSuccessCount;
	LARGE_INTEGER RTSFailureCount;
	LARGE_INTEGER ACKFailureCount;
	LARGE_INTEGER FrameDuplicateCount;
	LARGE_INTEGER ReceivedFragmentCount;
	LARGE_INTEGER MulticastReceivedFrameCount;
	LARGE_INTEGER FCSErrorCount;
	LARGE_INTEGER TransmittedFrameCount;
	LARGE_INTEGER WEPUndecryptableCount;
	LARGE_INTEGER TransmitCountFrmOs;
#ifdef MT_MAC
	LARGE_INTEGER TxAggRange1Count;
	LARGE_INTEGER TxAggRange2Count;
	LARGE_INTEGER TxAggRange3Count;
	LARGE_INTEGER TxAggRange4Count;

	/* for PER debug */
	LARGE_INTEGER AmpduFailCount;
	LARGE_INTEGER AmpduSuccessCount;
	/* for PER debug */

#endif /* MT_MAC */
} COUNTER_802_11, *PCOUNTER_802_11;


typedef struct _ASIC_UPDATE_PROTECT
{
	USHORT OperationMode;
	UCHAR SetMask;
	BOOLEAN bDisableBGProtect;
	BOOLEAN bNonGFExist;
} ASIC_UPDATE_PROTECT, *PASIC_UPDATE_PROTECT;

// Wtbl2RateTableUpdate
typedef struct _WTBL2_RATE_TABLE_UPDATE
{
	UCHAR ucWcid;
	UINT32 u4Wtbl2D9;
	UINT32 rate[8];
} WTBL2_RATE_TABLE_UPDATE, *PWTBL2_RATE_TABLE_UPDATE;


#ifdef RT3290
typedef struct _ANT_DIVERSITY
{
	BOOLEAN		RateUp;
	ULONG		TrainCounter;
	ULONG		AntennaDiversityState;	// 0->Stable state 1->training state
	ULONG		AntennaDiversityPER[2];  // 0 ->main 1->aux
	ULONG		AntennaDiversityTxPacketCount[2];  // 0 ->main 1->aux
	ULONG		AntennaDiversityRxPacketCount[2];
	CHAR		Rssi[2];
	ULONG		AntennaDiversityCount;
	ULONG		AntennaDiversityTrigger;
}ANT_DIVERSITY, *PANT_DIVERSITY;
#endif /* RT3290 */

typedef struct _COUNTER_RALINK {
	UINT32 OneSecStart;	/* for one sec count clear use */
	UINT32 OneSecBeaconSentCnt;
	UINT32 OneSecFalseCCACnt;	/* CCA error count, for debug purpose, might move to global counter */
	UINT32 OneSecCCKFalseCCACnt;
	UINT32 OneSecOFDMFalseCCACnt;
	UINT32 OneSecRxFcsErrCnt;	/* CRC error */
	UINT32 OneSecRxOkCnt;	/* RX without error */
	UINT32 OneSecTxFailCount;
	UINT32 OneSecTxNoRetryOkCount;
	UINT32 OneSecTxRetryOkCount;
	UINT32 OneSecRxOkDataCnt;	/* unicast-to-me DATA frame count */
	UINT32 OneSecTransmittedByteCount;	/* both successful and failure, used to calculate TX throughput */

	ULONG OneSecOsTxCount[NUM_OF_TX_RING];
	ULONG OneSecDmaDoneCount[NUM_OF_TX_RING];
	UINT32 OneSecTxDoneCount;
	ULONG OneSecRxCount;
	UINT32 OneSecReceivedByteCount;
	UINT32 OneSecTxARalinkCnt;	/* Tx Ralink Aggregation frame cnt */
	UINT32 OneSecRxARalinkCnt;	/* Rx Ralink Aggregation frame cnt */
	UINT32 OneSecEnd;	/* for one sec count clear use */

	ULONG TransmittedByteCount;	/* both successful and failure, used to calculate TX throughput */
	ULONG ReceivedByteCount;	/* both CRC okay and CRC error, used to calculate RX throughput */
#if defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) || defined(RT5592)
	ULONG LastReceivedByteCount;
#endif /* defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) || defined(RT5592) */
#ifdef RT3290
	// TODO: shiang, check the purpose of following parameter
	ULONG OneSecRxOkCnt2; /* RX without error */
#endif /* RT3290 */
	ULONG BadCQIAutoRecoveryCount;
	ULONG PoorCQIRoamingCount;
	ULONG MgmtRingFullCount;
	ULONG RxCountSinceLastNULL;
	ULONG RxCount;
	ULONG KickTxCount;
	LARGE_INTEGER RealFcsErrCount;
	ULONG PendingNdisPacketCount;
	ULONG FalseCCACnt;                    /* CCA error count */

	UINT32 LastOneSecTotalTxCount;	/* OneSecTxNoRetryOkCount + OneSecTxRetryOkCount + OneSecTxFailCount */
	UINT32 LastOneSecRxOkDataCnt;	/* OneSecRxOkDataCnt */
	ULONG DuplicateRcv;
	ULONG TxAggCount;
	ULONG TxNonAggCount;
	ULONG TxAgg1MPDUCount;
	ULONG TxAgg2MPDUCount;
	ULONG TxAgg3MPDUCount;
	ULONG TxAgg4MPDUCount;
	ULONG TxAgg5MPDUCount;
	ULONG TxAgg6MPDUCount;
	ULONG TxAgg7MPDUCount;
	ULONG TxAgg8MPDUCount;
	ULONG TxAgg9MPDUCount;
	ULONG TxAgg10MPDUCount;
	ULONG TxAgg11MPDUCount;
	ULONG TxAgg12MPDUCount;
	ULONG TxAgg13MPDUCount;
	ULONG TxAgg14MPDUCount;
	ULONG TxAgg15MPDUCount;
	ULONG TxAgg16MPDUCount;

	LARGE_INTEGER TxAMSDUCount;
	LARGE_INTEGER RxAMSDUCount;
	LARGE_INTEGER TransmittedAMPDUCount;
	LARGE_INTEGER TransmittedMPDUsInAMPDUCount;
	LARGE_INTEGER TransmittedOctetsInAMPDUCount;
	LARGE_INTEGER MPDUInReceivedAMPDUCount;

	ULONG PhyErrCnt;
	ULONG PlcpErrCnt;
} COUNTER_RALINK, *PCOUNTER_RALINK;

typedef struct _COUNTER_DRS {
	/* to record the each TX rate's quality. 0 is best, the bigger the worse. */
	USHORT TxQuality[MAX_TX_RATE_INDEX+1];
	UCHAR PER[MAX_TX_RATE_INDEX+1];
	UCHAR TxRateUpPenalty;	/* extra # of second penalty due to last unstable condition */
	/*BOOLEAN         fNoisyEnvironment; */
	BOOLEAN fLastSecAccordingRSSI;
	UCHAR LastSecTxRateChangeAction;	/* 0: no change, 1:rate UP, 2:rate down */
	UCHAR LastTimeTxRateChangeAction;	/*Keep last time value of LastSecTxRateChangeAction */
	ULONG LastTxOkCount;
} COUNTER_DRS, *PCOUNTER_DRS;


#ifdef DOT11_N_SUPPORT
#ifdef TXBF_SUPPORT
typedef struct _COUNTER_TXBF{
	ULONG TxSuccessCount;
	ULONG TxRetryCount;
	ULONG TxFailCount;
	ULONG ETxSuccessCount;
	ULONG ETxRetryCount;
	ULONG ETxFailCount;
	ULONG ITxSuccessCount;
	ULONG ITxRetryCount;
	ULONG ITxFailCount;
} COUNTER_TXBF;
#endif /* TXBF_SUPPORT */
#endif /* DOT11_N_SUPPORT */


#ifdef STREAM_MODE_SUPPORT
typedef struct _STREAM_MODE_ENTRY_{
#define STREAM_MODE_STATIC		1
	USHORT flag;
	UCHAR macAddr[MAC_ADDR_LEN];
}STREAM_MODE_ENTRY;
#endif /* STREAM_MODE_SUPPORT */

/* for Microwave oven */
#ifdef MICROWAVE_OVEN_SUPPORT
typedef struct _MO_CFG_STRUCT {
	BOOLEAN		bEnable;
	UINT8  		nPeriod_Cnt; 	/* measurement period 100ms, mitigate the interference period 900 ms */
	UINT16 		nFalseCCACnt;
	UINT16		nFalseCCATh;	/* default is 100 */
#ifdef MT7601
	UINT32		Stored_BBP_R65;
	UCHAR		Stored_RF_B5_R6;
	UCHAR		Stored_RF_B5_R7;
#endif /* MT7601 */
#ifdef RT6352
	UCHAR		Stored_RF_B7_R6;
	UCHAR		Stored_RF_B7_R7;
#endif /* RT6352 */
} MO_CFG_STRUCT, *PMO_CFG_STRUCT;
#endif /* MICROWAVE_OVEN_SUPPORT */

/* TODO: need to integrate with MICROWAVE_OVEN_SUPPORT */
#ifdef DYNAMIC_VGA_SUPPORT
/* for dynamic vga */
typedef struct _LNA_VGA_CTL_STRUCT {
	BOOLEAN		bEnable;
	BOOLEAN		bDyncVgaEnable;
	UINT8  		nPeriod_Cnt; 	/* measurement period 100ms, mitigate the interference period 900 ms */
	UINT16 		nFalseCCACnt;
	UINT16		nFalseCCATh;	/* default is 100 */
	UINT16		nLowFalseCCATh;
	UCHAR		agc_vga_init_0;
	UINT16		agc_0_vga_set1_2;
	UCHAR 		agc_vga_init_1;
	UINT16		agc_1_vga_set1_2;
} LNA_VGA_CTL_STRUCT, *PLNA_VGA_CTL_STRUCT;
#endif /* DYNAMIC_VGA_SUPPORT */

/***************************************************************************
  *	security key related data structure
  **************************************************************************/

/* structure to define WPA Group Key Rekey Interval */
typedef struct GNU_PACKED _RT_802_11_WPA_REKEY {
	ULONG ReKeyMethod;	/* mechanism for rekeying: 0:disable, 1: time-based, 2: packet-based */
	ULONG ReKeyInterval;	/* time-based: seconds, packet-based: kilo-packets */
} RT_WPA_REKEY,*PRT_WPA_REKEY, RT_802_11_WPA_REKEY, *PRT_802_11_WPA_REKEY;


#ifdef RTMP_MAC_USB
/***************************************************************************
  *	RTUSB I/O related data structure
  **************************************************************************/

/* for USB interface, avoid in interrupt when write key */
typedef struct RT_ADD_PAIRWISE_KEY_ENTRY {
	UCHAR MacAddr[6];
	USHORT MacTabMatchWCID;	/* ASIC */
	CIPHER_KEY CipherKey;
} RT_ADD_PAIRWISE_KEY_ENTRY,*PRT_ADD_PAIRWISE_KEY_ENTRY;


/* Cipher suite type for mixed mode group cipher, P802.11i-2004 */
typedef enum _RT_802_11_CIPHER_SUITE_TYPE {
	Cipher_Type_NONE,
	Cipher_Type_WEP40,
	Cipher_Type_TKIP,
	Cipher_Type_RSVD,
	Cipher_Type_CCMP,
	Cipher_Type_WEP104
} RT_802_11_CIPHER_SUITE_TYPE, *PRT_802_11_CIPHER_SUITE_TYPE;
#endif /* RTMP_MAC_USB */

typedef struct {
	UCHAR Addr[MAC_ADDR_LEN];
	UCHAR ErrorCode[2];	/*00 01-Invalid authentication type */
	/*00 02-Authentication timeout */
	/*00 03-Challenge from AP failed */
	/*00 04-Challenge to AP failed */
	BOOLEAN Reported;
} ROGUEAP_ENTRY, *PROGUEAP_ENTRY;

typedef struct {
	UCHAR RogueApNr;
	ROGUEAP_ENTRY RogueApEntry[MAX_LEN_OF_BSS_TABLE];
} ROGUEAP_TABLE, *PROGUEAP_TABLE;

/*
  *	Fragment Frame structure
  */
typedef struct _FRAGMENT_FRAME {
	PNDIS_PACKET pFragPacket;
	ULONG RxSize;
	USHORT Sequence;
	USHORT LastFrag;
	ULONG Flags;		/* Some extra frame information. bit 0: LLC presented */
} FRAGMENT_FRAME, *PFRAGMENT_FRAME;


/*
	Tkip Key structure which RC4 key & MIC calculation
*/
typedef struct _TKIP_KEY_INFO {
	UINT nBytesInM;		/* # bytes in M for MICKEY */
	ULONG IV16;
	ULONG IV32;
	ULONG K0;		/* for MICKEY Low */
	ULONG K1;		/* for MICKEY Hig */
	ULONG L;		/* Current state for MICKEY */
	ULONG R;		/* Current state for MICKEY */
	ULONG M;		/* Message accumulator for MICKEY */
	UCHAR RC4KEY[16];
	UCHAR MIC[8];
} TKIP_KEY_INFO, *PTKIP_KEY_INFO;


/*
	Private / Misc data, counters for driver internal use
*/
typedef struct __PRIVATE_STRUC {
	/* Tx ring full occurrance number */
	UINT TxRingFullCnt;
	/* Tkip stuff */
	TKIP_KEY_INFO Tx;
	TKIP_KEY_INFO Rx;
} PRIVATE_STRUC, *PPRIVATE_STRUC;


/***************************************************************************
  *	Channel and BBP related data structures
  **************************************************************************/
/* structure to tune BBP R66 (BBP TUNING) */
typedef struct _BBP_R66_TUNING {
	BOOLEAN bEnable;
	USHORT FalseCcaLowerThreshold;	/* default 100 */
	USHORT FalseCcaUpperThreshold;	/* default 512 */
	UCHAR R66Delta;
	UCHAR R66CurrentValue;
	BOOLEAN R66LowerUpperSelect;	/*Before LinkUp, Used LowerBound or UpperBound as R66 value. */
} BBP_R66_TUNING, *PBBP_R66_TUNING;


#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
#define EFFECTED_CH_SECONDARY 0x1
#define EFFECTED_CH_PRIMARY	0x2
#define EFFECTED_CH_LEGACY		0x4
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

/* structure to store channel TX power */
typedef struct _CHANNEL_TX_POWER {
	USHORT RemainingTimeForUse;	/*unit: sec */
	UCHAR Channel;
#ifdef DOT11N_DRAFT3
	BOOLEAN bEffectedChannel;	/* For BW 40 operating in 2.4GHz , the "effected channel" is the channel that is covered in 40Mhz. */
#endif /* DOT11N_DRAFT3 */
	CHAR Power;
	CHAR Power2;
#ifdef DOT11N_SS3_SUPPORT
	CHAR Power3;
#endif /* DOT11N_SS3_SUPPORT */
	UCHAR MaxTxPwr;
	UCHAR DfsReq;
	UCHAR RegulatoryDomain;

/*
	Channel property:

	CHANNEL_DISABLED: The channel is disabled.
	CHANNEL_PASSIVE_SCAN: Only passive scanning is allowed.
	CHANNEL_NO_IBSS: IBSS is not allowed.
	CHANNEL_RADAR: Radar detection is required.
	CHANNEL_NO_FAT_ABOVE: Extension channel above this channel is not allowed.
	CHANNEL_NO_FAT_BELOW: Extension channel below this channel is not allowed.
	CHANNEL_40M_CAP: 40 BW channel group
	CHANNEL_80M_CAP: 800 BW channel group
 */
#define CHANNEL_DEFAULT_PROP	0x00
#define CHANNEL_DISABLED		0x01	/* no use */
#define CHANNEL_PASSIVE_SCAN	0x02
#define CHANNEL_NO_IBSS			0x04
#define CHANNEL_RADAR			0x08
#define CHANNEL_NO_FAT_ABOVE	0x10
#define CHANNEL_NO_FAT_BELOW	0x20
#define CHANNEL_40M_CAP			0x40
#define CHANNEL_80M_CAP			0x80

	UCHAR Flags;

#if defined(RT30xx) || defined(RT5390) || defined(RT5392)
	UCHAR Tx0FinePowerCtrl;	/* Tx0 fine power control in 0.1dB step */
	UCHAR Tx1FinePowerCtrl;	/* Tx1 fine power control in 0.1dB step */
	UCHAR Tx2FinePowerCtrl;	/* Tx2 fine power control in 0.1dB step */
#endif /* defined(RT30xx) || defined(RT5390) || defined(RT5392) */
} CHANNEL_TX_POWER, *PCHANNEL_TX_POWER;

/* Channel list subset */
typedef struct _CHANNEL_LIST_SUB {
	UCHAR	Channel;
	UCHAR	IdxMap; /* Index mapping to original channel list */
} CHANNEL_LIST_SUB, *PCHANNEL_LIST_SUB;


typedef struct _SOFT_RX_ANT_DIVERSITY_STRUCT {
	UCHAR EvaluatePeriod;	/* 0:not evalute status, 1: evaluate status, 2: switching status */
	UCHAR EvaluateStableCnt;
	UCHAR Pair1PrimaryRxAnt;	/* 0:Ant-E1, 1:Ant-E2 */
	UCHAR Pair1SecondaryRxAnt;	/* 0:Ant-E1, 1:Ant-E2 */
#ifdef CONFIG_STA_SUPPORT
	SHORT Pair1AvgRssi[2];	/* AvgRssi[0]:E1, AvgRssi[1]:E2 */
	SHORT Pair2AvgRssi[2];	/* AvgRssi[0]:E3, AvgRssi[1]:E4 */
#endif /* CONFIG_STA_SUPPORT */
	SHORT Pair1LastAvgRssi;	/* */
	SHORT Pair2LastAvgRssi;	/* */
	ULONG RcvPktNumWhenEvaluate;
	BOOLEAN FirstPktArrivedWhenEvaluate;
#ifdef CONFIG_AP_SUPPORT
	LONG Pair1AvgRssiGroup1[2];
	LONG Pair1AvgRssiGroup2[2];
	ULONG RcvPktNum[2];
#endif /* CONFIG_AP_SUPPORT */
} SOFT_RX_ANT_DIVERSITY, *PSOFT_RX_ANT_DIVERSITY;

typedef enum _ABGBAND_STATE_ {
	UNKNOWN_BAND,
	BG_BAND,
	A_BAND,
} ABGBAND_STATE;

#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_MAC_PCI
/* Power save method control */
typedef union _PS_CONTROL {
	struct {
		ULONG EnablePSinIdle:1;	/* Enable radio off when not connect to AP. radio on only when sitesurvey, */
		ULONG EnableNewPS:1;	/* Enable new  Chip power save fucntion . New method can only be applied in chip version after 2872. and PCIe. */
		ULONG rt30xxPowerMode:2;	/* Power Level Mode for rt30xx chip */
		ULONG rt30xxFollowHostASPM:1;	/* Card Follows Host's setting for rt30xx chip. */
		ULONG rt30xxForceASPMTest:1;	/* Force enable L1 for rt30xx chip. This has higher priority than rt30xxFollowHostASPM Mode. */
/*		ULONG		rsv:26;			// Radio Measurement Enable */
		ULONG AMDNewPSOn:1;	/* Enable for AMD L1 (toggle) */
		ULONG LedMode:2;	/* 0: Blink normal.  1: Slow blink not normal. */
		ULONG rt30xxForceL0:1;	/* Force only use L0 for rt30xx */

		/* PCIe config space [Completion TimeOut Disable], compatible issue with Intel HM55 */
		ULONG CTO:1;	/* 0: default, update the CTO bit to disable; 1: Keep BIOS config value */
		ULONG PM4PCIeCLKOn:1;	/* 0: default, turn off PCIE CLk at PM4; 1: FW MCU cmd arg1 as "0x5a" which will not turn off PCIE CLK */

		ULONG rsv:20;	/* Rsvd */
	} field;
	ULONG word;
} PS_CONTROL, *PPS_CONTROL;
#endif /* RTMP_MAC_PCI */
#endif /* CONFIG_STA_SUPPORT */


/***************************************************************************
  *	structure for MLME state machine
  **************************************************************************/
typedef struct _MLME_STRUCT {
#ifdef CONFIG_STA_SUPPORT
	/* STA state machines */
	STATE_MACHINE CntlMachine;
	STATE_MACHINE AssocMachine;
	STATE_MACHINE AuthMachine;
	STATE_MACHINE AuthRspMachine;
	STATE_MACHINE SyncMachine;
	STATE_MACHINE WpaPskMachine;
	STATE_MACHINE LeapMachine;
	STATE_MACHINE_FUNC AssocFunc[ASSOC_FUNC_SIZE];
	STATE_MACHINE_FUNC AuthFunc[AUTH_FUNC_SIZE];
	STATE_MACHINE_FUNC AuthRspFunc[AUTH_RSP_FUNC_SIZE];
	STATE_MACHINE_FUNC SyncFunc[SYNC_FUNC_SIZE];
#ifdef DOT11R_FT_SUPPORT
	STATE_MACHINE FtOtaAuthMachine;
	STATE_MACHINE_FUNC FtOtaAuthFunc[FT_OTA_AUTH_FUNC_SIZE];
	STATE_MACHINE FtOtdActMachine;
	STATE_MACHINE_FUNC FtOtdActFunc[FT_OTD_FUNC_SIZE];
#endif /* DOT11R_FT_SUPPORT */

#ifdef STA_EASY_CONFIG_SETUP
	STATE_MACHINE AutoProvisionMachine;
	STATE_MACHINE_FUNC AutoProvisionFunc[AUTO_PROVISION_FUNC_SIZE];
#endif /* STA_EASY_CONFIG_SETUP */
#endif /* CONFIG_STA_SUPPORT */
	STATE_MACHINE_FUNC ActFunc[ACT_FUNC_SIZE];
	/* Action */
	STATE_MACHINE ActMachine;

#ifdef WSC_INCLUDED
	STATE_MACHINE WscMachine;
	STATE_MACHINE_FUNC WscFunc[WSC_FUNC_SIZE];

#ifdef IWSC_SUPPORT
	STATE_MACHINE			IWscMachine;
	STATE_MACHINE_FUNC		IWscFunc[IWSC_FUNC_SIZE];
#endif /* IWSC_SUPPORT */
#endif /* WSC_INCLUDED */

#ifdef QOS_DLS_SUPPORT
	STATE_MACHINE DlsMachine;
	STATE_MACHINE_FUNC DlsFunc[DLS_FUNC_SIZE];
#endif /* QOS_DLS_SUPPORT */

#ifdef DOT11Z_TDLS_SUPPORT
	STATE_MACHINE TdlsMachine;
	STATE_MACHINE_FUNC TdlsFunc[TDLS_FUNC_SIZE];
	STATE_MACHINE TdlsChSwMachine;
	STATE_MACHINE_FUNC TdlsChSwFunc[TDLS_CHSW_FUNC_SIZE];
#endif /* DOT11Z_TDLS_SUPPORT */

#ifdef CONFIG_HOTSPOT
	STATE_MACHINE HSCtrlMachine;
	STATE_MACHINE_FUNC HSCtrlFunc[GAS_FUNC_SIZE];
#endif

#ifdef CONFIG_DOT11U_INTERWORKING
	STATE_MACHINE GASMachine;
	STATE_MACHINE_FUNC GASFunc[GAS_FUNC_SIZE];
#endif

#ifdef CONFIG_DOT11V_WNM
	STATE_MACHINE BTMMachine;
	STATE_MACHINE_FUNC BTMFunc[BTM_FUNC_SIZE];
	STATE_MACHINE WNMNotifyMachine;
	STATE_MACHINE_FUNC WNMNotifyFunc[WNM_NOTIFY_FUNC_SIZE];
#endif

#ifdef CONFIG_AP_SUPPORT
	/* AP state machines */
	STATE_MACHINE ApAssocMachine;
	STATE_MACHINE ApAuthMachine;
	STATE_MACHINE ApSyncMachine;
	STATE_MACHINE_FUNC ApAssocFunc[AP_ASSOC_FUNC_SIZE];
/*	STATE_MACHINE_FUNC		ApDlsFunc[DLS_FUNC_SIZE]; */
	STATE_MACHINE_FUNC ApAuthFunc[AP_AUTH_FUNC_SIZE];
	STATE_MACHINE_FUNC ApSyncFunc[AP_SYNC_FUNC_SIZE];
#ifdef APCLI_SUPPORT
	STATE_MACHINE ApCliAuthMachine;
	STATE_MACHINE ApCliAssocMachine;
	STATE_MACHINE ApCliCtrlMachine;
	STATE_MACHINE ApCliSyncMachine;
	STATE_MACHINE ApCliWpaPskMachine;

	STATE_MACHINE_FUNC ApCliAuthFunc[APCLI_AUTH_FUNC_SIZE];
	STATE_MACHINE_FUNC ApCliAssocFunc[APCLI_ASSOC_FUNC_SIZE];
	STATE_MACHINE_FUNC ApCliCtrlFunc[APCLI_CTRL_FUNC_SIZE];
	STATE_MACHINE_FUNC ApCliSyncFunc[APCLI_SYNC_FUNC_SIZE];
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	/* common WPA state machine */
	STATE_MACHINE WpaMachine;
	STATE_MACHINE_FUNC WpaFunc[WPA_FUNC_SIZE];

#ifdef MESH_SUPPORT
	STATE_MACHINE MeshCtrlMachine;
	STATE_MACHINE MeshLinkMngMachine;

	STATE_MACHINE_FUNC MeshCtrlFunc[MESH_CTRL_FUNC_SIZE];
	STATE_MACHINE_FUNC MeshLinkMngFunc[MESH_LINK_MNG_FUNC_SIZE];
#endif /* MESH_SUPPORT */

	ULONG ChannelQuality;	/* 0..100, Channel Quality Indication for Roaming */
	ULONG Now32;		/* latch the value of NdisGetSystemUpTime() */
	ULONG LastSendNULLpsmTime;

	BOOLEAN bRunning;
	NDIS_SPIN_LOCK TaskLock;
	MLME_QUEUE Queue;

	UINT ShiftReg;

#ifdef BT_COEXISTENCE_SUPPORT
	RALINK_TIMER_STRUCT MiscDetectTimer;
#endif /* BT_COEXISTENCE_SUPPORT */
	RALINK_TIMER_STRUCT PeriodicTimer;
	RALINK_TIMER_STRUCT APSDPeriodicTimer;
	RALINK_TIMER_STRUCT LinkDownTimer;
	RALINK_TIMER_STRUCT LinkUpTimer;
#ifdef RTMP_MAC_PCI
	UCHAR bPsPollTimerRunning;
	RALINK_TIMER_STRUCT PsPollTimer;
	RALINK_TIMER_STRUCT RadioOnOffTimer;
#endif /* RTMP_MAC_PCI */
	ULONG PeriodicRound;
	ULONG GPIORound;
	ULONG OneSecPeriodicRound;

	UCHAR RealRxPath;
	BOOLEAN bLowThroughput;
	BOOLEAN bEnableAutoAntennaCheck;
	RALINK_TIMER_STRUCT RxAntEvalTimer;
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	BOOLEAN bStartScc;
#endif /*RT_CFG80211_P2P_CONCURRENT_DEVICE */

#if defined(RT30xx) || defined(RT5390) || defined(RT5392)
	UCHAR CaliBW40RfR24;
	UCHAR CaliBW20RfR24;
	UCHAR CaliBW20RfR31;
	UCHAR CaliBW40RfR31;
#endif /* defined(RT30xx) || defined(RT5390) || defined(RT5392) */

#ifdef RTMP_MAC_USB
	RALINK_TIMER_STRUCT AutoWakeupTimer;
	BOOLEAN AutoWakeupTimerRunning;
#endif /* RTMP_MAC_USB */

#ifdef MT7601
#ifdef CONFIG_STA_SUPPORT
	// TODO: shiang-usw, what's the purpose of this parameter?
	BOOLEAN bSendNullFrameAfterWareUp;
#endif /* CONFIG_STA_SUPPORT */
#endif /* MT7601 */
} MLME_STRUCT, *PMLME_STRUCT;

#ifdef DOT11_N_SUPPORT
/***************************************************************************
  *	802.11 N related data structures
  **************************************************************************/
struct reordering_mpdu {
	struct reordering_mpdu *next;
	PNDIS_PACKET pPacket;	/* coverted to 802.3 frame */
	int Sequence;		/* sequence number of MPDU */
	BOOLEAN bAMSDU;
	UCHAR					OpMode;
};

struct reordering_list {
	struct reordering_mpdu *next;
	int qlen;
};

struct reordering_mpdu_pool {
	PVOID mem;
	NDIS_SPIN_LOCK lock;
	struct reordering_list freelist;
};

typedef enum _BA_SESSION_TYPE{
	BA_SESSION_INV = 0,
	BA_SESSION_ORI = 1,
	BA_SESSION_RECP = 2,
}BA_SESSION_TYPE;

typedef enum _REC_BLOCKACK_STATUS {
	Recipient_NONE = 0,
	Recipient_USED,
	Recipient_HandleRes,
	Recipient_Accept
} REC_BLOCKACK_STATUS, *PREC_BLOCKACK_STATUS;

typedef enum _ORI_BLOCKACK_STATUS {
	Originator_NONE = 0,
	Originator_USED,
	Originator_WaitRes,
	Originator_Done
} ORI_BLOCKACK_STATUS, *PORI_BLOCKACK_STATUS;

typedef struct _BA_ORI_ENTRY {
	UCHAR Wcid;
	UCHAR TID;
	UCHAR BAWinSize;
	UCHAR Token;
	UCHAR amsdu_cap;
/* Sequence is to fill every outgoing QoS DATA frame's sequence field in 802.11 header. */
	USHORT Sequence;
	USHORT TimeOutValue;
	ORI_BLOCKACK_STATUS ORI_BA_Status;
	RALINK_TIMER_STRUCT ORIBATimer;
	PVOID pAdapter;
} BA_ORI_ENTRY, *PBA_ORI_ENTRY;

typedef struct _BA_REC_ENTRY {
	UCHAR Wcid;
	UCHAR TID;
	UCHAR BAWinSize;	/* 7.3.1.14. each buffer is capable of holding a max AMSDU or MSDU. */
	/*UCHAR NumOfRxPkt; */
	/*UCHAR    Curindidx; // the head in the RX reordering buffer */
	USHORT LastIndSeq;
/*	USHORT		LastIndSeqAtTimer; */
	USHORT TimeOutValue;
	RALINK_TIMER_STRUCT RECBATimer;
	ULONG LastIndSeqAtTimer;
	ULONG nDropPacket;
	ULONG rcvSeq;
	REC_BLOCKACK_STATUS REC_BA_Status;
/*	UCHAR	RxBufIdxUsed; */
	/* corresponding virtual address for RX reordering packet storage. */
	/*RTMP_REORDERDMABUF MAP_RXBuf[MAX_RX_REORDERBUF]; */
	NDIS_SPIN_LOCK RxReRingLock;	/* Rx Ring spinlock */
/*	struct _BA_REC_ENTRY *pNext; */
	PVOID pAdapter;
	struct reordering_list list;
} BA_REC_ENTRY, *PBA_REC_ENTRY;


typedef struct {
	ULONG numAsRecipient;	/* I am recipient of numAsRecipient clients. These client are in the BARecEntry[] */
	ULONG numAsOriginator;	/* I am originator of   numAsOriginator clients. These clients are in the BAOriEntry[] */
	ULONG numDoneOriginator;	/* count Done Originator sessions */
	BA_ORI_ENTRY BAOriEntry[MAX_LEN_OF_BA_ORI_TABLE];
	BA_REC_ENTRY BARecEntry[MAX_LEN_OF_BA_REC_TABLE];
} BA_TABLE, *PBA_TABLE;

/*For QureyBATableOID use; */
typedef struct GNU_PACKED _OID_BA_REC_ENTRY {
	UCHAR MACAddr[MAC_ADDR_LEN];
	UCHAR BaBitmap;		/* if (BaBitmap&(1<<TID)), this session with{MACAddr, TID}exists, so read BufSize[TID] for BufferSize */
	UCHAR rsv;
	UCHAR BufSize[8];
	REC_BLOCKACK_STATUS REC_BA_Status[8];
} OID_BA_REC_ENTRY, *POID_BA_REC_ENTRY;

/*For QureyBATableOID use; */
typedef struct GNU_PACKED _OID_BA_ORI_ENTRY {
	UCHAR MACAddr[MAC_ADDR_LEN];
	UCHAR BaBitmap;		/* if (BaBitmap&(1<<TID)), this session with{MACAddr, TID}exists, so read BufSize[TID] for BufferSize, read ORI_BA_Status[TID] for status */
	UCHAR rsv;
	UCHAR BufSize[8];
	ORI_BLOCKACK_STATUS ORI_BA_Status[8];
} OID_BA_ORI_ENTRY, *POID_BA_ORI_ENTRY;

typedef struct _QUERYBA_TABLE {
	OID_BA_ORI_ENTRY BAOriEntry[MAX_LEN_OF_MAC_TABLE];
	OID_BA_REC_ENTRY BARecEntry[MAX_LEN_OF_MAC_TABLE];
	UCHAR OriNum;		/* Number of below BAOriEntry */
	UCHAR RecNum;		/* Number of below BARecEntry */
} QUERYBA_TABLE, *PQUERYBA_TABLE;

typedef union _BACAP_STRUC {
#ifdef RT_BIG_ENDIAN
	struct {
		UINT32:4;
		UINT32 b2040CoexistScanSup:1;	/*As Sta, support do 2040 coexistence scan for AP. As Ap, support monitor trigger event to check if can use BW 40MHz. */
		UINT32 bHtAdhoc:1;	/* adhoc can use ht rate. */
		UINT32 MMPSmode:2;	/* MIMO power save more, 0:static, 1:dynamic, 2:rsv, 3:mimo enable */
		UINT32 AmsduSize:1;	/* 0:3839, 1:7935 bytes. UINT  MSDUSizeToBytes[]        = { 3839, 7935}; */
		UINT32 AmsduEnable:1;	/*Enable AMSDU transmisstion */
		UINT32 MpduDensity:3;
		UINT32 Policy:2;	/* 0: DELAY_BA 1:IMMED_BA  (//BA Policy subfiled value in ADDBA frame)   2:BA-not use */
		UINT32 AutoBA:1;	/* automatically BA */
		UINT32 TxBAWinLimit:8;
		UINT32 RxBAWinLimit:8;
	} field;
#else
	struct {
		UINT32 RxBAWinLimit:8;
		UINT32 TxBAWinLimit:8;
		UINT32 AutoBA:1;	/* automatically BA */
		UINT32 Policy:2;	/* 0: DELAY_BA 1:IMMED_BA  (//BA Policy subfiled value in ADDBA frame)   2:BA-not use */
		UINT32 MpduDensity:3;
		UINT32 AmsduEnable:1;	/*Enable AMSDU transmisstion */
		UINT32 AmsduSize:1;	/* 0:3839, 1:7935 bytes. UINT  MSDUSizeToBytes[]        = { 3839, 7935}; */
		UINT32 MMPSmode:2;	/* MIMO power save more, 0:static, 1:dynamic, 2:rsv, 3:mimo enable */
		UINT32 bHtAdhoc:1;	/* adhoc can use ht rate. */
		UINT32 b2040CoexistScanSup:1;	/*As Sta, support do 2040 coexistence scan for AP. As Ap, support monitor trigger event to check if can use BW 40MHz. */
		 UINT32:4;
	} field;
#endif
	UINT32 word;
} BACAP_STRUC, *PBACAP_STRUC;

typedef struct {
	BOOLEAN IsRecipient;
	UCHAR MACAddr[MAC_ADDR_LEN];
	UCHAR TID;
	UCHAR nMSDU;
	USHORT TimeOut;
	BOOLEAN bAllTid;	/* If True, delete all TID for BA sessions with this MACaddr. */
} OID_ADD_BA_ENTRY, *POID_ADD_BA_ENTRY;

#ifdef DOT11N_DRAFT3
typedef enum _BSS2040COEXIST_FLAG {
	BSS_2040_COEXIST_DISABLE = 0,
	BSS_2040_COEXIST_TIMER_FIRED = 1,
	BSS_2040_COEXIST_INFO_SYNC = 2,
	BSS_2040_COEXIST_INFO_NOTIFY = 4,
	BSS_2040_COEXIST_BW_SYNC = 8,
} BSS2040COEXIST_FLAG;

typedef struct _BssCoexChRange_ {
	UCHAR primaryCh;
	UCHAR secondaryCh;
	UCHAR effectChStart;
	UCHAR effectChEnd;
} BSS_COEX_CH_RANGE;
#endif /* DOT11N_DRAFT3 */

#define IS_VHT_STA(_pMacEntry)	(_pMacEntry->MaxHTPhyMode.field.MODE == MODE_VHT)
#define IS_HT_STA(_pMacEntry)	\
	(_pMacEntry->MaxHTPhyMode.field.MODE >= MODE_HTMIX)

#define IS_HT_RATE(_pMacEntry)	\
	(_pMacEntry->HTPhyMode.field.MODE >= MODE_HTMIX)

#ifdef DOT11_VHT_AC
#define IS_VHT_RATE(_pMacEntry)	\
	(_pMacEntry->HTPhyMode.field.MODE == MODE_VHT)
#endif
#define PEER_IS_HT_RATE(_pMacEntry)	\
	(_pMacEntry->HTPhyMode.field.MODE >= MODE_HTMIX)

#endif /* DOT11_N_SUPPORT */

/*This structure is for all 802.11n card InterOptibilityTest action. Reset all Num every n second.  (Details see MLMEPeriodic) */
typedef struct _IOT_STRUC {
	BOOLEAN bRTSLongProtOn;
#ifdef CONFIG_STA_SUPPORT
	BOOLEAN bLastAtheros;
	//BOOLEAN bCurrentAtheros;
	//BOOLEAN bNowAtherosBurstOn;
	//BOOLEAN bNextDisableRxBA;
	BOOLEAN bToggle;
#endif /* CONFIG_STA_SUPPORT */
} IOT_STRUC;

/* This is the registry setting for 802.11n transmit setting.  Used in advanced page. */
typedef union _REG_TRANSMIT_SETTING {
#ifdef RT_BIG_ENDIAN
	struct {
		UINT32 rsv:13;
		UINT32 EXTCHA:2;
		UINT32 HTMODE:1;
		UINT32 TRANSNO:2;
		UINT32 STBC:1;	/*SPACE */
		UINT32 ShortGI:1;
		UINT32 BW:1;	/*channel bandwidth 20MHz or 40 MHz */
		UINT32 TxBF:1;	/* 3*3 */
		UINT32 ITxBfEn:1;
		UINT32 rsv0:9;
		/*UINT32  MCS:7;                 // MCS */
		/*UINT32  PhyMode:4; */
	} field;
#else
	struct {
		/*UINT32  PhyMode:4; */
		/*UINT32  MCS:7;                 // MCS */
		UINT32 rsv0:9;
		UINT32 ITxBfEn:1;
		UINT32 TxBF:1;
		UINT32 BW:1;	/*channel bandwidth 20MHz or 40 MHz */
		UINT32 ShortGI:1;
		UINT32 STBC:1;	/*SPACE */
		UINT32 TRANSNO:2;
		UINT32 HTMODE:1;
		UINT32 EXTCHA:2;
		UINT32 rsv:13;
	} field;
#endif
	UINT32 word;
} REG_TRANSMIT_SETTING;


typedef union _DESIRED_TRANSMIT_SETTING {
#ifdef RT_BIG_ENDIAN
	struct {
		USHORT rsv:2;
		USHORT FixedTxMode:3;	/* If MCS isn't AUTO, fix rate in CCK, OFDM, HT or VHT mode. */
		USHORT PhyMode:4;
		USHORT MCS:7;	/* MCS */
	} field;
#else
	struct {
		USHORT MCS:7;
		USHORT PhyMode:4;
		USHORT FixedTxMode:3;
		USHORT rsv:2;
	} field;
#endif
	USHORT word;
 } DESIRED_TRANSMIT_SETTING;


struct hw_setting{
	UCHAR prim_ch;
	UCHAR cent_ch;
	UINT8 bbp_bw;
	UCHAR rf_band;
	UCHAR cur_ch_pwr[3];
	CHAR lan_gain;
};


#define WDEV_TYPE_AP		0x01
#define WDEV_TYPE_STA		0x02
#define WDEV_TYPE_ADHOC	0x04
#define WDEV_TYPE_WDS		0x08
#define WDEV_TYPE_MESH	0x10

struct wifi_dev{
	PNET_DEV if_dev;
	VOID *func_dev;
	VOID *sys_handle;

	CHAR wdev_idx;	/* index refer from pAd->wdev_list[] */
	CHAR func_idx; /* index refer to func_dev which pointer to */
	UCHAR tr_tb_idx; /* index refer to BSS which this device belong */
	UCHAR wdev_type;
	UCHAR PhyMode;
	UCHAR channel;
	UCHAR CentralChannel;
	UCHAR extcha;
	UCHAR bw;
	UCHAR if_addr[MAC_ADDR_LEN];
	UCHAR bssid[MAC_ADDR_LEN];
	UCHAR hw_bssid_idx;

	/* security segment */
	NDIS_802_11_AUTHENTICATION_MODE AuthMode;
	NDIS_802_11_WEP_STATUS WepStatus;
	NDIS_802_11_WEP_STATUS GroupKeyWepStatus;
	WPA_MIX_PAIR_CIPHER WpaMixPairCipher;
	UCHAR DefaultKeyId;
	UCHAR PortSecured;
#if defined(DOT1X_SUPPORT) || defined(WPA_SUPPLICANT_SUPPORT)
	BOOLEAN IEEE8021X; /* Only indicate if we are running in dynamic WEP mode (WEP+802.1x) */
#endif /* DOT1X_SUPPORT */

	/* transmit segment */
	BOOLEAN allow_data_tx;
	BOOLEAN IgmpSnoopEnable; /* Only enabled for AP/WDS mode */
	RT_PHY_INFO DesiredHtPhyInfo;
	DESIRED_TRANSMIT_SETTING DesiredTransmitSetting;	/* Desired transmit setting. this is for reading registry setting only. not useful. */
	BOOLEAN bAutoTxRateSwitch;
	HTTRANSMIT_SETTING HTPhyMode, MaxHTPhyMode, MinHTPhyMode;	/* For transmit phy setting in TXWI. */

	/* 802.11 protocol related characters */
	BOOLEAN bWmmCapable;	/* 0:disable WMM, 1:enable WMM */
	/* UAPSD information: such as enable or disable, do not remove */
	UAPSD_INFO UapsdInfo;


	/* VLAN related */
	BOOLEAN bVLAN_Tag;
	USHORT VLAN_VID;
	USHORT VLAN_Priority;

	/* operations */
	INT (*tx_pkt_allowed)(struct _RTMP_ADAPTER *pAd, struct wifi_dev *wdev, PNDIS_PACKET pPacket, UCHAR *pWcid);
	INT (*tx_pkt_handle)(struct _RTMP_ADAPTER *pAd, PNDIS_PACKET pPacket);
	NDIS_STATUS (*wdev_hard_tx)(struct _RTMP_ADAPTER *pAd, struct _TX_BLK *pTxBlk);
	INT (*rx_pkt_allowed)(struct _RTMP_ADAPTER *pAd, struct _RX_BLK *pRxBlk);
	INT (*rx_pkt_hdr_chk)(struct _RTMP_ADAPTER *pAd, struct _RX_BLK *pRxBlk);
	INT (*rx_ps_handle)(struct _RTMP_ADAPTER *pAd, struct _RX_BLK *pRxBlk);
	INT (*rx_pkt_foward)(struct _RTMP_ADAPTER *pAd, struct wifi_dev *wdev, PNDIS_PACKET pPacket);

	/* last received packet's SNR for each antenna */
	UCHAR LastSNR0;
	UCHAR LastSNR1;
#ifdef DOT11N_SS3_SUPPORT
	UCHAR LastSNR2;
	INT32 BF_SNR[3];	/* Last RXWI BF SNR. Units=0.25 dB */
#endif /* DOT11N_SS3_SUPPORT */
	RSSI_SAMPLE RssiSample;
	ULONG NumOfAvgRssiSample;

#if defined(RT_CFG80211_SUPPORT) || defined(HOSTAPD_SUPPORT)
	NDIS_HOSTAPD_STATUS Hostapd;
#endif	
};


typedef struct _PWR_MGMT_STRUCT_ 
{
	USHORT		Psm;		/* power management mode   (PWR_ACTIVE|PWR_SAVE), Please use this value to replace  pAd->StaCfg.Psm in the future*/
	BOOLEAN	bBeaconLost;
	BOOLEAN	bTriggerRoaming;
	BOOLEAN	bEnterPsmNull;
	UINT8		ucDtimPeriod;
	UINT8		ucBeaconPeriod;
	UINT8		ucOwnMacIdx;
} PWR_MGMT_STRUCT, *PPWR_MGMT_STRUCT;


#ifdef RTMP_MAC_USB
/***************************************************************************
  *	USB-based chip Beacon related data structures
  **************************************************************************/
#define BEACON_BITMAP_MASK		0xff
typedef struct _BEACON_SYNC_STRUCT_ {
	UCHAR BeaconBuf[HW_BEACON_MAX_NUM][HW_BEACON_OFFSET];
	UCHAR *BeaconTxWI[HW_BEACON_MAX_NUM];
	ULONG TimIELocationInBeacon[HW_BEACON_MAX_NUM];
	ULONG CapabilityInfoLocationInBeacon[HW_BEACON_MAX_NUM];
	BOOLEAN EnableBeacon;	/* trigger to enable beacon transmission. */
	UCHAR BeaconBitMap;	/* NOTE: If the MAX_MBSSID_NUM is larger than 8, this parameter need to change. */
	UCHAR DtimBitOn;	/* NOTE: If the MAX_MBSSID_NUM is larger than 8, this parameter need to change. */
} BEACON_SYNC_STRUCT;
#endif /* RTMP_MAC_USB */

/***************************************************************************
  *	Multiple SSID related data structures
  **************************************************************************/
#define WLAN_MAX_NUM_OF_TIM			((MAX_LEN_OF_MAC_TABLE >> 3) + 1)	/* /8 + 1 */
#define WLAN_CT_TIM_BCMC_OFFSET		0	/* unit: 32B */

/* clear bcmc TIM bit */
#define WLAN_MR_TIM_BCMC_CLEAR(apidx) \
	pAd->ApCfg.MBSSID[apidx].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET] &= ~NUM_BIT8[0];

/* set bcmc TIM bit */
#define WLAN_MR_TIM_BCMC_SET(apidx) \
	pAd->ApCfg.MBSSID[apidx].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET] |= NUM_BIT8[0];

#define WLAN_MR_TIM_BCMC_GET(apidx) \
	(pAd->ApCfg.MBSSID[apidx].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET] & NUM_BIT8[0])

/* clear a station PS TIM bit */
#define WLAN_MR_TIM_BIT_CLEAR(ad_p, apidx, _aid) \
	{	UCHAR tim_offset = _aid >> 3; \
		UCHAR bit_offset = _aid & 0x7; \
		ad_p->ApCfg.MBSSID[apidx].TimBitmaps[tim_offset] &= (~NUM_BIT8[bit_offset]); }

/* set a station PS TIM bit */
#define WLAN_MR_TIM_BIT_SET(ad_p, apidx, _aid) \
	{	UCHAR tim_offset = _aid >> 3; \
		UCHAR bit_offset = _aid & 0x7; \
		ad_p->ApCfg.MBSSID[apidx].TimBitmaps[tim_offset] |= NUM_BIT8[bit_offset]; }

#ifdef EASY_CONFIG_SETUP
#ifdef WAC_SUPPORT
typedef struct _WAC_REQUEST {
	BOOLEAN	bReqType; /* TRUE: set, FALSE: get */
	UCHAR	Command; /* 0: add, 1: del */
	UCHAR	FrameType;
	BOOLEAN	RV;	/* TRUE: success, FALSE: fail */
	USHORT	CurrentStatus;
	UCHAR	VendorInfoLen;
	PUCHAR	pVendorInfo;
} WAC_REQUEST, *PWAC_REQUEST;
#endif /* WAC_SUPPORT */


typedef struct _EASY_CONFIG_INFO {
#ifdef CONFIG_STA_SUPPORT
	RALINK_TIMER_STRUCT ScanTimer;
	RALINK_TIMER_STRUCT InfraConnTimer;
	RALINK_TIMER_STRUCT AdhocConnTimer;
	UCHAR ModuleType;
	BOOLEAN bRaAutoWpsAp;
	BOOLEAN bChangeMode;
	UCHAR WpsEncryptPIN[12];
#ifdef WAC_SUPPORT
	BOOLEAN	bWACAP;
	UCHAR	DoAutoWAC; /* 0x00: Do WAC by driver, 0x01: Do WAC by UI */
	BOOLEAN	bAutoWAC; /* TRUE: Do WAC by driver, FALSE: Do WAC by UI */
	PUCHAR			pVendorInfoForProbeReq;
	UCHAR			VendorInfoForProbeReqLen;
#endif /* WAC_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
	BOOLEAN bEnable;
	BOOLEAN bDoAutoWps;
	CHAR RssiThreshold;
	UCHAR WpsPTK[64];	/* Ralink proprietary PTK */
	UCHAR WpsPinCode[8];
#ifdef WAC_SUPPORT
	BOOLEAN			bEnableWAC;
	NDIS_SPIN_LOCK 	WAC_PeerListSemLock;
	LIST_HEADER		WAC_PeerList;
#ifdef WAC_QOS_PRIORITY
	NDIS_SPIN_LOCK 	WAC_ForcePriorityListSemLock;
	LIST_HEADER		WAC_ForcePriorityList;
#endif /* WAC_QOS_PRIORITY */
	USHORT 			CurrentStatus;
#ifdef CONFIG_AP_SUPPORT
	PUCHAR			pVendorInfoForProbeRsp;
	UCHAR			VendorInfoForProbeRspLen;
	PUCHAR			pVendorInfoForBeacon;
	UCHAR			VendorInfoForBeaconLen;
#endif /* CONFIG_AP_SUPPORT */
#endif /* WAC_SUPPORT */
} EASY_CONFIG_INFO, *PEASY_CONFIG_INFO;
#endif /* EASY_CONFIG_SETUP */


enum BCN_TX_STATE{
    BCN_TX_UNINIT = 0,
	BCN_TX_IDLE = 1,
    BCN_TX_WRITE_TO_DMA = 2,
    BCN_TX_DMA_DONE = 3,
    BCN_TX_TXS_DONE = 4,
	BCN_TX_STOP = 5
};

typedef struct _BCN_BUF_STRUCT {
	BOOLEAN bBcnSntReq;	/* used in if beacon send or stop */
	UCHAR BcnBufIdx;
	enum BCN_TX_STATE bcn_state;	/* Make sure if no packet pending in the Hardware */
//	NDIS_SPIN_LOCK bcn_lock;

#if 0
	CHAR BeaconBuf[MAX_BEACON_SIZE];	/* NOTE: BeaconBuf should be 4-byte aligned */
#else
	PNDIS_PACKET BeaconPkt;
#endif
	UCHAR cap_ie_pos;
}BCN_BUF_STRUC;

#ifdef MT_MAC
typedef struct _TIM_BUF_STRUCT {
	BOOLEAN bTimSntReq;	/* used in if beacon send or stop */
	UCHAR TimBufIdx;
	PNDIS_PACKET TimPkt;
//	PNDIS_PACKET TimPkt2;
} TIM_BUF_STRUC;

INT wdev_tim_buf_init(RTMP_ADAPTER *pAd, TIM_BUF_STRUC *tim_info);
#endif


#ifdef CONFIG_AP_SUPPORT
#ifdef DBG
#ifdef MT_MAC
#define MAX_TIME_RECORD 5
#endif
#endif

typedef struct _BSS_STRUCT {
	struct wifi_dev wdev;

	INT mbss_idx;

#ifdef HOSTAPD_SUPPORT
	NDIS_HOSTAPD_STATUS Hostapd;
	BOOLEAN HostapdWPS;
#endif

	CHAR Ssid[MAX_LEN_OF_SSID];
	UCHAR SsidLen;
	BOOLEAN bHideSsid;

	USHORT CapabilityInfo;

	UCHAR MaxStaNum;	/* Limit the STA connection number per BSS */
	UCHAR StaCount;
	UINT16 StationKeepAliveTime;	/* unit: second */

	/*
		Security segment
	*/
	UCHAR RSNIE_Len[2];
	UCHAR RSN_IE[2][MAX_LEN_OF_RSNIE];

	/* WPA */
	UCHAR GMK[32];
	UCHAR PMK[32];
	UCHAR GTK[32];
	UCHAR GNonce[32];
	NDIS_802_11_PRIVACY_FILTER PrivacyFilter;

	/* for Group Rekey, AP ONLY */
	RT_WPA_REKEY WPAREKEY;
	ULONG REKEYCOUNTER;
	RALINK_TIMER_STRUCT REKEYTimer;
	UCHAR REKEYTimerRunning;
	UINT8 RekeyCountDown;

	/* For PMK Cache using, AP ONLY */
	ULONG PMKCachePeriod;	/* unit : jiffies */
	NDIS_AP_802_11_PMKID PMKIDCache;

#ifdef DOT1X_SUPPORT
	BOOLEAN PreAuth;

	/* For 802.1x daemon setting per BSS */
	UINT8 radius_srv_num;
	RADIUS_SRV_INFO radius_srv_info[MAX_RADIUS_SRV_NUM];
	UINT8 NasId[IFNAMSIZ];
	UINT8 NasIdLen;
#endif /* DOT1X_SUPPORT */

#ifdef WAPI_SUPPORT
	UCHAR WAPIPassPhrase[64];	/* WAPI PSK pass phrase */
	UINT WAPIPassPhraseLen;	/* the length of WAPI PSK pass phrase */
	UINT WapiPskType;	/* 0 - Hex, 1 - ASCII */
	UCHAR WAPI_BK[16];	/* WAPI base key */

	UCHAR NMK[LEN_WAPI_NMK];
	UCHAR key_announce_flag[LEN_WAPI_TSC];
	BOOLEAN sw_wpi_encrypt;	/* WPI data encrypt by SW */
#endif /* WAPI_SUPPORT */


	/*
		Transmitting segment
	*/
	UCHAR TxRate; /* RATE_1, RATE_2, RATE_5_5, RATE_11, ... */
	UCHAR DesiredRates[MAX_LEN_OF_SUPPORTED_RATES];	/* OID_802_11_DESIRED_RATES */
	UCHAR DesiredRatesIndex;
	UCHAR MaxTxRate; /* RATE_1, RATE_2, RATE_5_5, RATE_11 */


	/*
		Statistics segment
	*/
	/*MBSS_STATISTICS MbssStat;*/
	ULONG TxCount;
	ULONG RxCount;
	ULONG ReceivedByteCount;
	ULONG TransmittedByteCount;
	ULONG RxErrorCount;
	ULONG RxDropCount;

	ULONG TxErrorCount;
	ULONG TxDropCount;
	ULONG ucPktsTx;
	ULONG ucPktsRx;
	ULONG mcPktsTx;
	ULONG mcPktsRx;
	ULONG bcPktsTx;
	ULONG bcPktsRx;

	UCHAR BANClass3Data;
	ULONG IsolateInterStaTraffic;

	/* outgoing BEACON frame buffer and corresponding TXWI */
	BCN_BUF_STRUC bcn_buf;
	UCHAR TimBitmaps[WLAN_MAX_NUM_OF_TIM];
	UCHAR TimIELocationInBeacon;

#ifdef MT_MAC
	TIM_BUF_STRUC tim_buf;
	UCHAR TimIELocationInTim;
#endif /* MT_MAC */

#ifdef DOT11V_WNM_SUPPORT
	UCHAR DMSEntrycount;
	UCHAR totalDMScount;
#endif /* DOT11V_WNM_SUPPORT */

	RT_802_11_ACL AccessControlList;

	/* EDCA QoS */
	/*BOOLEAN bWmmCapable;*/	/* 0:disable WMM, 1:enable WMM */
	BOOLEAN bDLSCapable;	/* 0:disable DLS, 1:enable DLS */
#ifdef QOS_DLS_SUPPORT
	UCHAR DlsPTK[64];	/* Due to windows dirver count on meetinghouse to handle 4-way shake */
#endif /* QOS_DLS_SUPPORT */

	/*
	   Why need the parameter: 2009/09/22

	   1. iwpriv ra0 set WmmCapable=0
	   2. iwpriv ra0 set WirelessMode=9
	   3. iwpriv ra0 set WirelessMode=0
	   4. iwpriv ra0 set SSID=SampleAP

	   After the 4 commands, WMM is still enabled.
	   So we need the parameter to recover WMM Capable flag.

	   No the problem in station mode.
	 */
	BOOLEAN bWmmCapableOrg;	/* origin Wmm Capable in non-11n mode */


	/*
`		WPS segment
	*/
	WSC_LV_INFO WscIEBeacon;
	WSC_LV_INFO WscIEProbeResp;
#ifdef WSC_AP_SUPPORT
	WSC_CTRL WscControl;
	WSC_SECURITY_MODE WscSecurityMode;
#endif /* WSC_AP_SUPPORT */

#ifdef IDS_SUPPORT
	UINT32 RcvdConflictSsidCount;
	UINT32 RcvdSpoofedAssocRespCount;
	UINT32 RcvdSpoofedReassocRespCount;
	UINT32 RcvdSpoofedProbeRespCount;
	UINT32 RcvdSpoofedBeaconCount;
	UINT32 RcvdSpoofedDisassocCount;
	UINT32 RcvdSpoofedAuthCount;
	UINT32 RcvdSpoofedDeauthCount;
	UINT32 RcvdSpoofedUnknownMgmtCount;
	UINT32 RcvdReplayAttackCount;

	CHAR RssiOfRcvdConflictSsid;
	CHAR RssiOfRcvdSpoofedAssocResp;
	CHAR RssiOfRcvdSpoofedReassocResp;
	CHAR RssiOfRcvdSpoofedProbeResp;
	CHAR RssiOfRcvdSpoofedBeacon;
	CHAR RssiOfRcvdSpoofedDisassoc;
	CHAR RssiOfRcvdSpoofedAuth;
	CHAR RssiOfRcvdSpoofedDeauth;
	CHAR RssiOfRcvdSpoofedUnknownMgmt;
	CHAR RssiOfRcvdReplayAttack;
#endif /* IDS_SUPPORT */

#ifdef DOT11R_FT_SUPPORT
	FT_CFG FtCfg;
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
	WNM_CONFIG WnmCfg;
#endif /* DOT11V_WNM_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
	RRM_CONFIG RrmCfg;
#endif /* DOT11K_RRM_SUPPORT */

#ifdef DOT11W_PMF_SUPPORT
	PMF_CFG PmfCfg;
#endif /* DOT11W_PMF_SUPPORT */

#ifdef EASY_CONFIG_SETUP
	EASY_CONFIG_INFO EasyConfigInfo;
#endif /* EASY_CONFIG_SETUP */

	/* YF@20120417: Avoid connecting to AP in Poor Env, value 0 fOr disable. */
	CHAR AssocReqRssiThreshold;
	CHAR RssiLowForStaKickOut;

#ifdef CONFIG_DOT11U_INTERWORKING
	GAS_CTRL GASCtrl;
#endif

#ifdef CONFIG_HOTSPOT
	HOTSPOT_CTRL HotSpotCtrl;
#endif

#ifdef CONFIG_DOT11V_WNM
	WNM_CTRL WNMCtrl;
#endif

#ifdef SPECIFIC_TX_POWER_SUPPORT
	CHAR TxPwrAdj;
#endif /* SPECIFIC_TX_POWER_SUPPORT */

#ifdef DBG
#ifdef MT_MAC
    ULONG WriteBcnDoneTime[MAX_TIME_RECORD];
    ULONG BcnDmaDoneTime[MAX_TIME_RECORD];
    UCHAR bcn_not_idle_time;
    UINT32 bcn_recovery_num;
    ULONG TXS_TSF[MAX_TIME_RECORD];
    ULONG TXS_SN[MAX_TIME_RECORD];
    UCHAR timer_loop;
#endif /* MT_MAC */
#endif

#ifdef RT_CFG80211_SUPPORT
    /* Extra IEs for (Re)Association Response provided by wpa_supplicant. E.g, WPS & P2P & WFD...etc */
    UCHAR AssocRespExtraIe[512];
    UINT32 AssocRespExtraIeLen;
#endif /* RT_CFG80211_SUPPORT */

#ifdef BCN_OFFLOAD_SUPPORT
    BOOLEAN updateEventIsTriggered;
#endif /*BCN_OFFLOAD_SUPPORT*/
} BSS_STRUCT;

#endif /* CONFIG_AP_SUPPORT */

#if 0
#ifdef MT76XX_BTCOEX_SUPPORT
typedef struct PROTECTION_FRAME_STRUCT {
	BOOLEAN     Occupied;
       ULONG         Triggernumber;
       ULONG         Valid;
       ULONG         NodeType;
       ULONG         BssHashID;
       ULONG         FrameType;
} PROTECTION_FRAME_STRUCT, *PPROTECTION_FRAME_STRUCT;

typedef struct _COEX_RX_MANUAL_AGC_GAIN {
	BOOLEAN     bEnable;
       BOOLEAN     bStopAGC;
	CHAR      LMthreshold;
	CHAR      MHthreshold;
} COEX_RX_MANUAL_AGC_GAIN, *PCOEX_RX_MANUAL_AGC_GAIN;

typedef struct _COEX_MODE_STRUCTURE {
	BOOLEAN     bForce;
       BOOLEAN  FDDRequest;
       BOOLEAN  TDDRequest;
       ULONG      TDD_Power;
       ULONG      FDD_Power;
	ULONG      DefaultMode;
	ULONG      CurrentMode;
       ULONG      UpdateMode;
       CHAR        CoexTDDRSSITreshold;
       CHAR        CoexFDDRSSITreshold;
} COEX_MODE_STRUCTURE, *PCOEX_MODE_STRUCTURE;
#endif /*MT76XX_BTCOEX_SUPPORT*/
#endif


typedef struct _PROTECTION_STRUCT {

    USHORT  OperationMode;
    UCHAR   SetMask; 
    BOOLEAN bDisableBGProtect;    
    BOOLEAN bNonGFExist;

} PROTECTION_STRUCT;


/* configuration common to OPMODE_AP as well as OPMODE_STA */
typedef struct _COMMON_CONFIG {
	BOOLEAN bCountryFlag;
	UCHAR CountryCode[4];
#ifdef EXT_BUILD_CHANNEL_LIST
	UCHAR Geography;
	UCHAR DfsType;
	PUCHAR pChDesp;
#endif /* EXT_BUILD_CHANNEL_LIST */
	UCHAR CountryRegion;	/* Enum of country region, 0:FCC, 1:IC, 2:ETSI, 3:SPAIN, 4:France, 5:MKK, 6:MKK1, 7:Israel */
	UCHAR CountryRegionForABand;	/* Enum of country region for A band */
	UCHAR PhyMode;
	UCHAR cfg_wmode;
	UCHAR SavedPhyMode;
	USHORT Dsifs;		/* in units of usec */
	ULONG PacketFilter;	/* Packet filter for receiving */
	UINT8 RegulatoryClass[MAX_NUM_OF_REGULATORY_CLASS];

	CHAR Ssid[MAX_LEN_OF_SSID];	/* NOT NULL-terminated */
	UCHAR SsidLen;		/* the actual ssid length in used */
	UCHAR LastSsidLen;	/* the actual ssid length in used */
	CHAR LastSsid[MAX_LEN_OF_SSID];	/* NOT NULL-terminated */
	UCHAR LastBssid[MAC_ADDR_LEN];

	UCHAR Bssid[MAC_ADDR_LEN];
	USHORT BeaconPeriod;
	UCHAR Channel;
	UCHAR CentralChannel;	/* Central Channel when using 40MHz is indicating. not real channel. */

	UCHAR SupRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR SupRateLen;
	UCHAR ExtRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR ExtRateLen;
	UCHAR DesireRate[MAX_LEN_OF_SUPPORTED_RATES];	/* OID_802_11_DESIRED_RATES */
	UCHAR MaxDesiredRate;
	UCHAR ExpectedACKRate[MAX_LEN_OF_SUPPORTED_RATES];

	ULONG BasicRateBitmap;	/* backup basic ratebitmap */
	ULONG BasicRateBitmapOld;	/* backup basic ratebitmap */

	BOOLEAN bInServicePeriod;

#if 0 /* mbss uapsd */
	BOOLEAN bAPSDCapable;
#endif /* 0 */

	BOOLEAN bAPSDAC_BE;
	BOOLEAN bAPSDAC_BK;
	BOOLEAN bAPSDAC_VI;
	BOOLEAN bAPSDAC_VO;

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
	BOOLEAN	 TDLS_bAPSDAC_BE;
	BOOLEAN	 TDLS_bAPSDAC_BK;
	BOOLEAN	 TDLS_bAPSDAC_VI;
	BOOLEAN	 TDLS_bAPSDAC_VO;
	UCHAR TDLS_MaxSPLength;
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */

	/* because TSPEC can modify the APSD flag, we need to keep the APSD flag
	   requested in association stage from the station;
	   we need to recover the APSD flag after the TSPEC is deleted. */
	BOOLEAN bACMAPSDBackup[4];	/* for delivery-enabled & trigger-enabled both */
	BOOLEAN bACMAPSDTr[4];	/* no use */
	UCHAR MaxSPLength;

	BOOLEAN bNeedSendTriggerFrame;
	BOOLEAN bAPSDForcePowerSave;	/* Force power save mode, should only use in APSD-STAUT */
	ULONG TriggerTimerCount;
	UCHAR BBPCurrentBW;	/* BW_10, BW_20, BW_40, BW_80 */
	REG_TRANSMIT_SETTING RegTransmitSetting;	/*registry transmit setting. this is for reading registry setting only. not useful. */
	UCHAR TxRate;		/* Same value to fill in TXD. TxRate is 6-bit */
	UCHAR MaxTxRate;	/* RATE_1, RATE_2, RATE_5_5, RATE_11 */
	UCHAR TxRateIndex;	/* Tx rate index in Rate Switch Table */
	UCHAR MinTxRate;	/* RATE_1, RATE_2, RATE_5_5, RATE_11 */
	UCHAR RtsRate;		/* RATE_xxx */
	HTTRANSMIT_SETTING MlmeTransmit;	/* MGMT frame PHY rate setting when operatin at Ht rate. */
	UCHAR MlmeRate;		/* RATE_xxx, used to send MLME frames */
	UCHAR BasicMlmeRate;	/* Default Rate for sending MLME frames */

	USHORT RtsThreshold;	/* in unit of BYTE */
	USHORT FragmentThreshold;	/* in unit of BYTE */

	UCHAR TxPower;		/* in unit of mW */
	ULONG TxPowerPercentage;	/* 0~100 % */
	ULONG TxPowerDefault;	/* keep for TxPowerPercentage */
	UINT8 PwrConstraint;

#ifdef DOT11_N_SUPPORT
	BACAP_STRUC BACapability;	/*   NO USE = 0XFF  ;  IMMED_BA =1  ;  DELAY_BA=0 */
	BACAP_STRUC REGBACapability;	/*   NO USE = 0XFF  ;  IMMED_BA =1  ;  DELAY_BA=0 */
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11_VHT_AC
	BOOLEAN force_vht;
	UCHAR vht_bw;
	UCHAR vht_sgi_80;
	UCHAR vht_stbc;
	UCHAR vht_bw_signal;
	UCHAR vht_cent_ch;
	UCHAR vht_cent_ch2;
	UCHAR vht_mcs_cap;
	UCHAR vht_nss_cap;
	USHORT vht_tx_hrate;
	USHORT vht_rx_hrate;
	BOOLEAN ht20_forbid;
	BOOLEAN vht_ldpc;
#endif /* DOT11_VHT_AC */

	IOT_STRUC IOTestParm;	/* 802.11n InterOpbility Test Parameter; */
	ULONG TxPreamble;	/* Rt802_11PreambleLong, Rt802_11PreambleShort, Rt802_11PreambleAuto */
	BOOLEAN bUseZeroToDisableFragment;	/* Microsoft use 0 as disable */
	ULONG UseBGProtection;	/* 0: auto, 1: always use, 2: always not use */
	BOOLEAN bUseShortSlotTime;	/* 0: disable, 1 - use short slot (9us) */
	BOOLEAN bEnableTxBurst;	/* 1: enble TX PACKET BURST (when BA is established or AP is not a legacy WMM AP), 0: disable TX PACKET BURST */
	BOOLEAN bAggregationCapable;	/* 1: enable TX aggregation when the peer supports it */
	BOOLEAN bPiggyBackCapable;	/* 1: enable TX piggy-back according MAC's version */
	BOOLEAN bIEEE80211H;	/* 1: enable IEEE802.11h spec. */
	UCHAR RDDurRegion; /* Region of radar detection */
	ULONG DisableOLBCDetect;	/* 0: enable OLBC detect; 1 disable OLBC detect */

#ifdef DOT11_N_SUPPORT
	BOOLEAN bRdg;
#endif /* DOT11_N_SUPPORT */
	BOOLEAN bWmmCapable;	/* 0:disable WMM, 1:enable WMM */
	QOS_CAPABILITY_PARM APQosCapability;	/* QOS capability of the current associated AP */
	EDCA_PARM APEdcaParm;	/* EDCA parameters of the current associated AP */
	QBSS_LOAD_PARM APQbssLoad;	/* QBSS load of the current associated AP */
	UCHAR AckPolicy[4];	/* ACK policy of the specified AC. see ACK_xxx */
#ifdef CONFIG_STA_SUPPORT
	BOOLEAN bDLSCapable;	/* 0:disable DLS, 1:enable DLS */
#endif /* CONFIG_STA_SUPPORT */
	/* a bitmap of BOOLEAN flags. each bit represent an operation status of a particular */
	/* BOOLEAN control, either ON or OFF. These flags should always be accessed via */
	/* OPSTATUS_TEST_FLAG(), OPSTATUS_SET_FLAG(), OP_STATUS_CLEAR_FLAG() macros. */
	/* see fOP_STATUS_xxx in RTMP_DEF.C for detail bit definition */
	ULONG OpStatusFlags;

	BOOLEAN NdisRadioStateOff;	/*For HCT 12.0, set this flag to TRUE instead of called MlmeRadioOff. */
	ABGBAND_STATE BandState;        /* For setting BBP used on B/G or A mode. */

#ifdef ANT_DIVERSITY_SUPPORT
	UCHAR RxAntDiversityCfg;	/* 0:disable, 1:enable Software Rx Antenna Diversity, 2: Main, 3: Aux  */
	BOOLEAN bSWRxAntDiversity;	/* 0:disable, 1:enable Rx Antenna Diversity. */
	BOOLEAN bHWRxAntDiversity;	/* 0:disable, 1:enable Rx Antenna Diversity. */
	CHAR nAntEval_Threshold;	/* HW PPAD --> ANT evaluation threshold, -55dBm ? */
	UINT16 nAntMiss_Threshold;	/* HW PPAD antenna selection mismatch default antenna, 30% ? */
	UINT16 nAntMiss_Cnt;		/* HW PPAD antenna selection mismatch default antenna, 30% ? */
	BOOLEAN bAntEvalEnable;		/* need to evaluate another antenna */
	BOOLEAN bTriggerTimer;		/* need to early escape */
#endif /* ANT_DIVERSITY_SUPPORT */

#ifdef DFS_SUPPORT
	/* IEEE802.11H--DFS. */
	RADAR_DETECT_STRUCT RadarDetect;
#endif /* DFS_SUPPORT */
#ifdef CARRIER_DETECTION_SUPPORT
	CARRIER_DETECTION_STRUCT CarrierDetect;
#endif /* CARRIER_DETECTION_SUPPORT */

#ifdef DOT11_N_SUPPORT
	/* HT */
	RT_HT_CAPABILITY DesiredHtPhy;
	HT_CAPABILITY_IE HtCapability;
	ADD_HT_INFO_IE AddHTInfo;	/* Useful as AP. */
	/*This IE is used with channel switch announcement element when changing to a new 40MHz. */
	/*This IE is included in channel switch ammouncement frames 7.4.1.5, beacons, probe Rsp. */
	NEW_EXT_CHAN_IE NewExtChanOffset;	/*7.3.2.20A, 1 if extension channel is above the control channel, 3 if below, 0 if not present */

	EXT_CAP_INFO_ELEMENT ExtCapIE;	/* this is the extened capibility IE appreed in MGMT frames. Doesn't need to update once set in Init. */

#ifdef DOT11N_DRAFT3
	BOOLEAN bBssCoexEnable;
	/*
	   Following two paramters now only used for the initial scan operation. the AP only do
	   bandwidth fallback when BssCoexApCnt > BssCoexApCntThr
	   By default, the "BssCoexApCntThr" is set as 0 in "UserCfgInit()".
	 */
	UCHAR BssCoexApCntThr;
	UCHAR BssCoexApCnt;

	UCHAR Bss2040CoexistFlag;	/* bit 0: bBssCoexistTimerRunning, bit 1: NeedSyncAddHtInfo. */
	RALINK_TIMER_STRUCT Bss2040CoexistTimer;
	UCHAR Bss2040NeedFallBack; 	/* 1: Need Fall back to 20MHz */

	/*This IE is used for 20/40 BSS Coexistence. */
	BSS_2040_COEXIST_IE BSS2040CoexistInfo;

	USHORT Dot11OBssScanPassiveDwell;	/* Unit : TU. 5~1000 */
	USHORT Dot11OBssScanActiveDwell;	/* Unit : TU. 10~1000 */
	USHORT Dot11BssWidthTriggerScanInt;	/* Unit : Second */
	USHORT Dot11OBssScanPassiveTotalPerChannel;	/* Unit : TU. 200~10000 */
	USHORT Dot11OBssScanActiveTotalPerChannel;	/* Unit : TU. 20~10000 */
	USHORT Dot11BssWidthChanTranDelayFactor;
	USHORT Dot11OBssScanActivityThre;	/* Unit : percentage */

	ULONG Dot11BssWidthChanTranDelay;	/* multiple of (Dot11BssWidthTriggerScanInt * Dot11BssWidthChanTranDelayFactor) */
	ULONG CountDownCtr;	/* CountDown Counter from (Dot11BssWidthTriggerScanInt * Dot11BssWidthChanTranDelayFactor) */

	BSS_2040_COEXIST_IE LastBSSCoexist2040;
	BSS_2040_COEXIST_IE BSSCoexist2040;
	TRIGGER_EVENT_TAB TriggerEventTab;
	UCHAR ChannelListIdx;

	BOOLEAN bOverlapScanning;
	BOOLEAN bBssCoexNotify;
#endif /* DOT11N_DRAFT3 */

	BOOLEAN bHTProtect;
	BOOLEAN bMIMOPSEnable;
	BOOLEAN bBADecline;
	BOOLEAN bDisableReordering;
	BOOLEAN bForty_Mhz_Intolerant;
	BOOLEAN bExtChannelSwitchAnnouncement;
	BOOLEAN bRcvBSSWidthTriggerEvents;
	ULONG LastRcvBSSWidthTriggerEventsTime;

	UCHAR TxBASize;

	BOOLEAN bRalinkBurstMode;
	UINT32 RestoreBurstMode;
	BOOLEAN ht_ldpc;
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11_VHT_AC
	UINT32 cfg_vht;
	VHT_CAP_INFO vht_info;
	VHT_CAP_IE vht_cap_ie;
	BOOLEAN bNonVhtDisallow; /* Disallow non-VHT connection */
#endif /* DOT11_VHT_AC */

#ifdef SYSTEM_LOG_SUPPORT
	/* Enable wireless event */
	BOOLEAN bWirelessEvent;
#endif /* SYSTEM_LOG_SUPPORT */

	BOOLEAN bWiFiTest;	/* Enable this parameter for WiFi test */

	/* Tx & Rx Stream number selection */
	UCHAR TxStream;
	UCHAR RxStream;

	/* transmit phy mode, trasmit rate for Multicast. */
#ifdef MCAST_RATE_SPECIFIC
	UCHAR McastTransmitMcs;
	UCHAR McastTransmitPhyMode;
#endif /* MCAST_RATE_SPECIFIC */

	BOOLEAN bHardwareRadio;	/* Hardware controlled Radio enabled */

#ifdef RTMP_MAC_USB
	BOOLEAN bMultipleIRP;	/* Multiple Bulk IN flag */
	UCHAR NumOfBulkInIRP;	/* if bMultipleIRP == TRUE, NumOfBulkInIRP will be 4 otherwise be 1 */
	RT_HT_CAPABILITY SupportedHtPhy;
	ULONG MaxPktOneTxBulk;
	UCHAR TxBulkFactor;
	UCHAR RxBulkFactor;

	BOOLEAN IsUpdateBeacon;
	BEACON_SYNC_STRUCT *pBeaconSync;
	RALINK_TIMER_STRUCT BeaconUpdateTimer;
	UINT32 BeaconAdjust;
	UINT32 BeaconFactor;
	UINT32 BeaconRemain;
#endif /* RTMP_MAC_USB */

#ifdef WSC_INCLUDED
	/* WSC hardware push button function 0811 */
	UINT8 WscHdrPshBtnCheckCount;
#endif /* WSC_INCLUDED */

#ifdef __ECOS
#ifdef PLATFORM_BUTTON_SUPPORT
        /* Hardware Restore button in APSOC */
        ULONG RestoreHdrBtnTimestamp;
	BOOLEAN RestoreHdrBtnFlag;	/* 0:disable, 1: enable */
#endif // PLATFORM_BUTTON_SUPPORT //
#endif /* __ECOS */

	NDIS_SPIN_LOCK MeasureReqTabLock;
	PMEASURE_REQ_TAB pMeasureReqTab;

	NDIS_SPIN_LOCK TpcReqTabLock;
	PTPC_REQ_TAB pTpcReqTab;

	/* transmit phy mode, trasmit rate for Multicast. */
#ifdef MCAST_RATE_SPECIFIC
	HTTRANSMIT_SETTING MCastPhyMode;
#endif /* MCAST_RATE_SPECIFIC */

#ifdef SINGLE_SKU
	UINT16 DefineMaxTxPwr;
	BOOLEAN bSKUMode;
	UINT16 AntGain;
	UINT16 BandedgeDelta;
	UINT16 ModuleTxpower;
#endif /* SINGLE_SKU */

#ifdef SINGLE_SKU_V2
#ifdef RT6352
	UCHAR SkuChannel;
#endif /* RT6352 */
#endif /* SINGLE_SKU_V2 */

#ifdef WAPI_SUPPORT
	COMMON_WAPI_INFO comm_wapi_info;

	/* rekey related parameter */
	/* USK update parameter */
	UINT8 wapi_usk_rekey_method;	/* 0:disable , 1:time, 2:packet */
	UINT32 wapi_usk_rekey_threshold;	/* the rekey threshold */

	/* MSK update parameter */
	UINT8 wapi_msk_rekey_method;	/* 0:disable , 1:time, 2:packet */
	UINT32 wapi_msk_rekey_threshold;	/* the rekey threshold */

	UINT32 wapi_msk_rekey_cnt;
	RALINK_TIMER_STRUCT WapiMskRekeyTimer;
	UCHAR WapiMskRekeyTimerRunning;
#endif /* WAPI_SUPPORT */
#ifdef RTMP_RBUS_SUPPORT
	ULONG CID;
	ULONG CN;
#ifdef RT6352
	UCHAR PKG_ID;
	UCHAR Chip_VerID;
	UCHAR Chip_E_Number;
#endif /* RT6352 */
#endif /* RTMP_RBUS_SUPPORT */

#if defined(RT305x)||defined(RT30xx)
	/* request by Gary, for High Power issue */
	UCHAR HighPowerPatchDisabled;
#endif /* defined(RT305x)|| defined(RT30xx) */

	BOOLEAN HT_DisallowTKIP;	/* Restrict the encryption type in 11n HT mode */
#ifdef DOT11K_RRM_SUPPORT
	BOOLEAN VoPwrConstraintTest;
#endif /* DOT11K_RRM_SUPPORT */

	BOOLEAN HT_Disable;	/* 1: disable HT function; 0: enable HT function */

#if defined(RT2883) || defined(RT3883) || defined(RT3593)
	ULONG PhyRateLimit;	/* PHY Rate limit in Mbps */
	INT		FixedRate;	/* Fix the rate during Rate Adaptation. FixedRate is ItemNo index into RateSwitch Table. -1 => disabled. */
	BOOLEAN FineAGC;	/* Fine AGC enabled */
#endif /* defined(RT2883) || defined(RT3883) || defined(RT3593) */

#ifdef PRE_ANT_SWITCH
	BOOLEAN PreAntSwitch;	/* Preamble Antenna Switch */
	SHORT PreAntSwitchRSSI;	/* Preamble Antenna Switch RSSI threshold */
	SHORT PreAntSwitchTimeout; /* Preamble Antenna Switch timeout in seconds */
#endif /* PRE_ANT_SWITCH */

#ifdef CFO_TRACK
	SHORT	CFOTrack;	/* CFO Tracking. 0=>use default, 1=>track, 2-7=> track 8-n times, 8=>done tracking */
#endif /* CFO_TRACK */

#ifdef NEW_RATE_ADAPT_SUPPORT
	USHORT	lowTrafficThrd;		/* Threshold for reverting to default MCS when traffic is low */
	SHORT	TrainUpRuleRSSI;	/* If TrainUpRule=2 then use Hybrid rule when RSSI < TrainUpRuleRSSI */
	USHORT	TrainUpLowThrd;		/* QuickDRS Hybrid train up low threshold */
	USHORT	TrainUpHighThrd;	/* QuickDRS Hybrid train up high threshold */
	BOOLEAN	TrainUpRule;		/* QuickDRS train up criterion: 0=>Throughput, 1=>PER, 2=> Throughput & PER */
#endif /* NEW_RATE_ADAPT_SUPPORT */

#ifdef STREAM_MODE_SUPPORT
#define		STREAM_MODE_STA_NUM		4

	UCHAR	StreamMode; /* 0=disabled, 1=enable for 1SS, 2=enable for 2SS, 3=enable for 1,2SS */
	UCHAR	StreamModeMac[STREAM_MODE_STA_NUM][MAC_ADDR_LEN];
	UINT16	StreamModeMCS;	/* Bit map for enabling Stream Mode based on MCS */
#endif /* STREAM_MODE_SUPPORT */

#ifdef DOT11_N_SUPPORT
#ifdef TXBF_SUPPORT
	ULONG ITxBfTimeout;
	ULONG ETxBfTimeout;
	ULONG	ETxBfEnCond;		/* Enable sending of sounding and beamforming */
	BOOLEAN	ETxBfNoncompress;	/* Force non-compressed Sounding Response */
	BOOLEAN	ETxBfIncapable;		/* Report Incapable of BF in TX BF Capabilities */
#endif /* TXBF_SUPPORT */
#endif /* DOT11_N_SUPPORT */

#ifdef DBG_CTRL_SUPPORT
	ULONG DebugFlags;	/* Temporary debug flags */
#endif /* DBG_CTRL_SUPPORT */

#ifdef RTMP_MAC_PCI
	BOOLEAN bPCIeBus;	/* The adapter runs over PCIe bus */
#endif /* RTMP_MAC_PCI */

#ifdef WSC_INCLUDED
	BOOLEAN WscPBCOverlap;
	WSC_STA_PBC_PROBE_INFO WscStaPbcProbeInfo;
#endif /* WSC_INCLUDED */

#ifdef CONFIG_ZTE_RADIO_ONOFF
	BOOLEAN bRadioEnable;
#endif /* CONFIG_ZTE_RADIO_ONOFF */

#ifdef MICROWAVE_OVEN_SUPPORT
	MO_CFG_STRUCT MO_Cfg;	/* data structure for mitigating microwave interference */
#endif /* MICROWAVE_OVEN_SUPPORT */

#ifdef RT6352
	BOOLEAN bEnTemperatureTrack;
#endif /* RT6352 */
#ifdef MT76XX_BTCOEX_SUPPORT
        //For COEX
        UCHAR 					AntMode; // Antenna mode (ANT_MODE_XXX)
        ULONG                            CoexDefaultMode;
        UCHAR 					CoexRXAGCLMTreshold;
        UCHAR 					CoexRXAGCMHTreshold;
        UCHAR                            CoexTDDRSSITreshold;
        UCHAR                            CoexFDDRSSITreshold;
        ULONG                            CoexWLANPrivilegeTime;
#endif /* MT76XX_BTCOEX_SUPPORT */

/* TODO: need to integrate with MICROWAVE_OVEN_SUPPORT */
#ifdef DYNAMIC_VGA_SUPPORT
	LNA_VGA_CTL_STRUCT lna_vga_ctl;
#endif /* DYNAMIC_VGA_SUPPORT */

	BOOLEAN bStopReadTemperature; /* avoid race condition between FW/driver */
    BOOLEAN bTXRX_RXV_ON;
    BOOLEAN ManualTxop;
    ULONG ManualTxopThreshold;
    UCHAR ManualTxopUpBound;
    UCHAR ManualTxopLowBound;

    PROTECTION_STRUCT RestoreProtection;

} COMMON_CONFIG, *PCOMMON_CONFIG;

#ifdef DBG_CTRL_SUPPORT
/* DebugFlag definitions */
#define DBF_NO_BF_AWARE_RA		0x0001	/* Revert to older Rate Adaptation that is not BF aware */
#define DBF_SHOW_BF_STATS		0x0002	/* Display BF statistics in AP "iwpriv stat" display */
#define DBF_NO_TXBF_3SS			0x0004	/* Disable TXBF for MCS > 20 */
#define DBF_UNUSED0008			0x0008	/* Unused */
#define DBF_DBQ_RA_LOG			0x0010	/* Log RA information in DBQ */
#define DBF_INIT_MCS_MARGIN		0x0020	/* Use 6 dB margin when selecting initial MCS */
#define DBF_INIT_MCS_DIS1		0x0040	/* Disable highest MCSs when selecting initial MCS */
#define DBF_FORCE_QUICK_DRS		0x0080	/* Force Quick DRS even if rate didn't change */
#define DBF_FORCE_SGI			0x0100	/* Force Short GI */
#define DBF_DBQ_NO_BCN			0x0200	/* Disable logging of RX Beacon frames */
#define DBF_LOG_VCO_CAL			0x0400	/* Log VCO cal */
#define DBF_DISABLE_CAL			0x0800	/* Disable Divider Calibration at channel change */
#ifdef INCLUDE_DEBUG_QUEUE
#define DBF_DBQ_TXFIFO			0x1000	/* Enable logging of TX information from FIFO */
#define DBF_DBQ_TXFRAME			0x2000	/* Enable logging of Frames queued for TX */
#define DBF_DBQ_RXWI_FULL		0x4000	/* Enable logging of full RXWI */
#define DBF_DBQ_RXWI			0x8000	/* Enable logging of partial RXWI */
#endif /* INCLUDE_DEBUG_QUEUE */

#define DBF_SHOW_RA_LOG			0x010000	/* Display concise Rate Adaptation information */
#define DBF_SHOW_ZERO_RA_LOG	0x020000	/* Include RA Log entries when TxCount is 0 */
#define DBF_FORCE_20MHZ			0x040000	/* Force 20 MHz TX */
#define DBF_FORCE_40MHZ 		0x080000	/* Force 40 MHz Tx */
#define DBF_DISABLE_CCK			0x100000	/* Disable CCK */
#define DBF_UNUSED200000		0x200000	/* Unused */
#define DBF_ENABLE_HT_DUP		0x400000	/* Allow HT Duplicate mode in TX rate table */
#define DBF_ENABLE_CCK_5G		0x800000	/* Enable CCK rates in 5G band */
#define DBF_UNUSED0100000		0x0100000	/* Unused */
#define DBF_ENABLE_20MHZ_MCS8	0x02000000	/* Substitute 20MHz MCS8 for 40MHz MCS8 */
#define DBF_DISABLE_20MHZ_MCS0	0x04000000	/* Disable substitution of 20MHz MCS0 for 40MHz MCS32 */
#define DBF_DISABLE_20MHZ_MCS1	0x08000000	/* Disable substitution of 20MHz MCS1 for 40MHz MCS0 */
#endif /* DBG_CTRL_SUPPORT */

#ifdef CFG_TDLS_SUPPORT
/* CFG TDLS */
typedef struct _CFG_TDLS_STRUCT
{
		/* For TPK handshake */
	UCHAR			ANonce[32];	/* Generated in Message 1, random variable */
	UCHAR			SNonce[32];	/* Generated in Message 2, random variable */
	ULONG			KeyLifetime;	/*  Use type= 'Key Lifetime Interval' unit: Seconds, min lifetime = 300 seconds */
	BOOLEAN			bCfgTDLSCapable; /* 0:disable TDLS, 1:enable TDLS  ; using supplicant sm */
	BOOLEAN			TdlsChSwitchSupp;
	BOOLEAN  		TdlsPsmSupp;
	
	UINT8 			TdlsLinkCount;
	UINT8 			TdlsDialogToken;
	CFG_TDLS_ENTRY		TDLSEntry[MAX_NUM_OF_CFG_TDLS_ENTRY];
}CFG_TDLS_STRUCT,*PCFG_TDLS_STRUCT;
#endif

#ifdef WPA_SUPPLICANT_SUPPORT
typedef struct _WPA_SUPPLICANT_INFO{
	/*
		802.1x WEP + MD5 will set key to driver before assoc, but we need to apply the key to
		ASIC after get EAPOL-Success frame, so we use this flag to indicate that
	*/
	BOOLEAN IEEE8021x_required_keys;
	CIPHER_KEY DesireSharedKey[4];	/* Record user desired WEP keys */
	UCHAR DesireSharedKeyId;

	/* 0x00: driver ignores wpa_supplicant */
	/* 0x01: wpa_supplicant initiates scanning and AP selection */
	/* 0x02: driver takes care of scanning, AP selection, and IEEE 802.11 association parameters */
	/* 0x80: wpa_supplicant trigger driver to do WPS */
	UCHAR WpaSupplicantUP;
	UCHAR WpaSupplicantScanCount;
	BOOLEAN bRSN_IE_FromWpaSupplicant;
	BOOLEAN bLostAp;
	UCHAR *pWpsProbeReqIe;
	UINT WpsProbeReqIeLen;
	UCHAR *pWpaAssocIe;
	UINT WpaAssocIeLen;
#ifdef CFG_TDLS_SUPPORT
	CFG_TDLS_STRUCT CFG_Tdls_info;
#endif
}WPA_SUPPLICANT_INFO;
#endif /* WPA_SUPPLICANT_SUPPORT */


#ifdef CONFIG_STA_SUPPORT

#ifdef CREDENTIAL_STORE
typedef struct _STA_CONNECT_INFO {
	BOOLEAN Changeable;
	BOOLEAN IEEE8021X;
	CHAR Ssid[MAX_LEN_OF_SSID]; // NOT NULL-terminated
	UCHAR SsidLen; // the actual ssid length in used
	NDIS_802_11_AUTHENTICATION_MODE AuthMode; // This should match to whatever microsoft defined
	NDIS_802_11_WEP_STATUS WepStatus;
	UCHAR DefaultKeyId;
	UCHAR PMK[LEN_PMK]; // WPA PSK mode PMK
	UCHAR WpaPassPhrase[64]; // WPA PSK pass phrase
	UINT WpaPassPhraseLen; // the length of WPA PSK pass phrase
	UINT8 WpaState;
	CIPHER_KEY SharedKey[1][4]; // STA always use SharedKey[BSS0][0..3]
	NDIS_SPIN_LOCK Lock;
} STA_CONNECT_INFO, *P_STA_CONNECT_INFO;
#endif /* CREDENTIAL_STORE */

#ifdef DOT11Z_TDLS_SUPPORT
typedef struct _TDLS_STRUCT
{
	BOOLEAN bTDLSCapable;	/* 0:disable TDLS, 1:enable TDLS */
	BOOLEAN bAcceptWeakSecurity;
	BOOLEAN	TdlsChSwitchSupp;
	BOOLEAN	TdlsPsmSupp;
	UINT8 TdlsDialogToken;
	UINT32 TdlsKeyLifeTime;
	UINT8 TdlsLinkSize;		/* record how much links establish already. */
	RT_802_11_TDLS TDLSEntry[MAX_NUM_OF_TDLS_ENTRY];
	NDIS_SPIN_LOCK TDLSEntryLock;
	NDIS_SPIN_LOCK TDLSUapsdLock;
#ifdef TDLS_AUTOLINK_SUPPORT
	BOOLEAN	TdlsAutoLink;
	LIST_HEADER TdlsDiscovPeerList;
	NDIS_SPIN_LOCK TdlsDiscovPeerListSemLock;
	LIST_HEADER TdlsBlackList;
	NDIS_SPIN_LOCK TdlsBlackListSemLock;

	CHAR TdlsAutoSetupRssiThreshold;
	CHAR TdlsAutoTeardownRssiThreshold;
	USHORT TdlsRssiMeasurementPeriod;
	USHORT TdlsDisabledPeriodByTeardown;
	USHORT TdlsAutoDiscoveryPeriod;
#endif /* TDLS_AUTOLINK_SUPPORT */

	/* Channel Switch */
	NDIS_SPIN_LOCK TdlsChSwLock;
	NDIS_SPIN_LOCK TdlsInitChannelLock;
	RALINK_TIMER_STRUCT TdlsPeriodGoBackBaseChTimer;
	RALINK_TIMER_STRUCT TdlsPeriodGoOffChTimer;
	RALINK_TIMER_STRUCT TdlsDisableChannelSwitchTimer;

	BOOLEAN TdlsForcePowerSaveWithAP;
	BOOLEAN bDoingPeriodChannelSwitch;
	BOOLEAN bChannelSwitchInitiator;
	BOOLEAN bChannelSwitchWaitSuccess;
	UCHAR TdlsCurrentTargetChannel;
	UCHAR TdlsDesireChannel;
	UCHAR TdlsDesireChannelBW;
	UCHAR TdlsDesireChSwMacAddr[MAC_ADDR_LEN];
	UCHAR TdlsAsicOperateChannel;
	UCHAR TdlsAsicOperateChannelBW;

	UCHAR TdlsChannelSwitchPairCount;
	UCHAR TdlsChannelSwitchRetryCount;
	ULONG TdlsGoBackStartTime;
	ULONG TdlsChSwSilenceTime;
	ULONG TdlsActiveSwitchTime;
	ULONG TdlsActiveSwitchTimeOut;
	UCHAR TdlsDtimCount;

	/* record old power save mode */
#define TDLS_POWER_SAVE_ACTIVE_COUNT_DOWN_NUM		(5*1000/MLME_TASK_EXEC_INTV)
	BOOLEAN TdlsFlgIsKeepingActiveCountDown; /* keep active until 0 */
	UINT8 TdlsPowerSaveActiveCountDown;
}TDLS_STRUCT, *PTDLS_STRUCT;
#endif // DOT11Z_TDLS_SUPPORT //




/* Modified by Wu Xi-Kun 4/21/2006 */
/* STA configuration and status */
typedef struct _STA_ADMIN_CONFIG {
	struct wifi_dev wdev;

	/*
		GROUP 1 -
		User configuration loaded from Registry, E2PROM or OID_xxx. These settings describe
		the user intended configuration, but not necessary fully equal to the final
		settings in ACTIVE BSS after negotiation/compromize with the BSS holder (either
		AP or IBSS holder).
		Once initialized, user configuration can only be changed via OID_xxx
	*/
	UCHAR BssType;		/* BSS_INFRA or BSS_ADHOC */

#ifdef MONITOR_FLAG_11N_SNIFFER_SUPPORT
#define MONITOR_FLAG_11N_SNIFFER		0x01
	UCHAR BssMonitorFlag;	/* Specific flag for monitor */
#endif /* MONITOR_FLAG_11N_SNIFFER_SUPPORT */

	USHORT AtimWin;		/* used when starting a new IBSS */

	/*
		GROUP 2 -
		User configuration loaded from Registry, E2PROM or OID_xxx. These settings describe
		the user intended configuration, and should be always applied to the final
		settings in ACTIVE BSS without compromising with the BSS holder.
		Once initialized, user configuration can only be changed via OID_xxx
	*/
	USHORT DefaultListenCount;	/* default listen count; */
	USHORT ThisTbttNumToNextWakeUp;
	ULONG WindowsPowerMode;	/* Power mode for AC power */
	ULONG WindowsBatteryPowerMode;	/* Power mode for battery if exists */
	BOOLEAN bWindowsACCAMEnable;	/* Enable CAM power mode when AC on */
	BOOLEAN bAutoReconnect;	/* Set to TRUE when setting OID_802_11_SSID with no matching BSSID */
	UCHAR RssiTrigger;
	UCHAR RssiTriggerMode;	/* RSSI_TRIGGERED_UPON_BELOW_THRESHOLD or RSSI_TRIGGERED_UPON_EXCCEED_THRESHOLD */

	ULONG WindowsPowerProfile;	/* Windows power profile, for NDIS5.1 PnP */

	BOOLEAN	 FlgPsmCanNotSleep; /* TRUE: can not switch ASIC to sleep */
	/* MIB:ieee802dot11.dot11smt(1).dot11StationConfigTable(1) */
	USHORT Psm;		/* power management mode   (PWR_ACTIVE|PWR_SAVE) */
	USHORT DisassocReason;
	UCHAR DisassocSta[MAC_ADDR_LEN];
	USHORT DeauthReason;
	UCHAR DeauthSta[MAC_ADDR_LEN];
	USHORT AuthFailReason;
	UCHAR AuthFailSta[MAC_ADDR_LEN];

	/*
		Security segment
	*/
	NDIS_802_11_PRIVACY_FILTER PrivacyFilter;	/* PrivacyFilter enum for 802.1X */

	/* Add to support different cipher suite for WPA2/WPA mode */
	NDIS_802_11_ENCRYPTION_STATUS GroupCipher;	/* Multicast cipher suite */
	NDIS_802_11_ENCRYPTION_STATUS PairCipher;	/* Unicast cipher suite */
	BOOLEAN bMixCipher;	/* Indicate current Pair & Group use different cipher suites */
	USHORT RsnCapability;

	UCHAR WpaPassPhrase[64];	/* WPA PSK pass phrase */
	UINT WpaPassPhraseLen;	/* the length of WPA PSK pass phrase */
	UCHAR PMK[LEN_PMK];	/* WPA PSK mode PMK */
	UCHAR PTK[LEN_PTK];	/* WPA PSK mode PTK */
	UCHAR GMK[LEN_GMK];	/* WPA PSK mode GMK */
	UCHAR GTK[MAX_LEN_GTK];	/* GTK from authenticator */
	UCHAR GNonce[32];	/* GNonce for WPA2PSK from authenticator */
	CIPHER_KEY TxGTK;
	BSSID_INFO SavedPMK[PMKID_NO];
	UINT SavedPMKNum;	/* Saved PMKID number */

#ifdef WAPI_SUPPORT
	UCHAR WAPIPassPhrase[64];	/* WAPI PSK pass phrase */
	UINT WAPIPassPhraseLen;	/* the length of WAPI PSK pass phrase */
	UINT WapiPskType;	/* 0 - Hex, 1 - ASCII */
	UCHAR WAPI_BK[16];	/* WAPI base key */

	UCHAR NMK[16];		/* WAPI notify master key */
#endif /* WAPI_SUPPORT */

	/* For WPA countermeasures */
	ULONG LastMicErrorTime;	/* record last MIC error time */
	ULONG MicErrCnt;	/* Should be 0, 1, 2, then reset to zero (after disassoiciation). */
	BOOLEAN bBlockAssoc;	/* Block associate attempt for 60 seconds after counter measure occurred. */
	/* For WPA-PSK supplicant state */
	UINT8 WpaState;		/* Default is SS_NOTUSE and handled by microsoft 802.1x */
	UCHAR ReplayCounter[8];
	UCHAR ANonce[32];	/* ANonce for WPA-PSK from aurhenticator */
	UCHAR SNonce[32];	/* SNonce for WPA-PSK */

	RSSI_SAMPLE RssiSample;
	ULONG NumOfAvgRssiSample;

	ULONG LastBeaconRxTime;	/* OS's timestamp of the last BEACON RX time */

	ULONG LastScanTime;	/* Record last scan time for issue BSSID_SCAN_LIST */
	BOOLEAN bNotFirstScan;	/* Sam add for ADHOC flag to do first scan when do initialization */
	BOOLEAN bSwRadio;	/* Software controlled Radio On/Off, TRUE: On */
	BOOLEAN bHwRadio;	/* Hardware controlled Radio On/Off, TRUE: On */
	BOOLEAN bRadio;		/* Radio state, And of Sw & Hw radio state */
	BOOLEAN bHardwareRadio;	/* Hardware controlled Radio enabled */
	BOOLEAN bShowHiddenSSID;	/* Show all known SSID in SSID list get operation */

	/* New for WPA, windows want us to to keep association information and */
	/* Fixed IEs from last association response */
	NDIS_802_11_ASSOCIATION_INFORMATION AssocInfo;
	USHORT ReqVarIELen;	/* Length of next VIE include EID & Length */
	UCHAR ReqVarIEs[MAX_VIE_LEN];	/* The content saved here should be little-endian format. */
	USHORT ResVarIELen;	/* Length of next VIE include EID & Length */
	UCHAR ResVarIEs[MAX_VIE_LEN];

	UCHAR RSNIE_Len;
	UCHAR RSN_IE[MAX_LEN_OF_RSNIE];	/* The content saved here should be little-endian format. */

	//ULONG CLBusyBytes;	/* Save the total bytes received durning channel load scan time */
	USHORT RPIDensity[8];	/* Array for RPI density collection */

	UCHAR RMReqCnt;		/* Number of measurement request saved. */
	UCHAR CurrentRMReqIdx;	/* Number of measurement request saved. */
	BOOLEAN ParallelReq;	/* Parallel measurement, only one request performed, */
	/* It must be the same channel with maximum duration */
	USHORT ParallelDuration;	/* Maximum duration for parallel measurement */
	UCHAR ParallelChannel;	/* Only one channel with parallel measurement */
	USHORT IAPPToken;	/* IAPP dialog token */
	/* Hack for channel load and noise histogram parameters */
	UCHAR NHFactor;		/* Parameter for Noise histogram */
	UCHAR CLFactor;		/* Parameter for channel load */

	RALINK_TIMER_STRUCT StaQuickResponeForRateUpTimer;
	BOOLEAN StaQuickResponeForRateUpTimerRunning;

	UCHAR DtimCount;	/* 0.. DtimPeriod-1 */
	UCHAR DtimPeriod;	/* default = 3 */

	BCN_BUF_STRUC bcn_buf;

#ifdef QOS_DLS_SUPPORT
	RT_802_11_DLS DLSEntry[MAX_NUM_OF_DLS_ENTRY];
	UCHAR DlsReplayCounter[8];
#endif				/* QOS_DLS_SUPPORT */

#ifdef DOT11Z_TDLS_SUPPORT
	TDLS_STRUCT TdlsInfo;
#endif /* DOT11Z_TDLS_SUPPORT */

	RALINK_TIMER_STRUCT WpaDisassocAndBlockAssocTimer;
	/* Fast Roaming */
	BOOLEAN bAutoRoaming;	/* 0:disable auto roaming by RSSI, 1:enable auto roaming by RSSI */
	CHAR dBmToRoam;		/* the condition to roam when receiving Rssi less than this value. It's negative value. */

#ifdef WPA_SUPPLICANT_SUPPORT
	WPA_SUPPLICANT_INFO wpa_supplicant_info;
#endif /* WPA_SUPPLICANT_SUPPORT */

#ifdef STA_EASY_CONFIG_SETUP
	BOOLEAN bScanReqIsFromWebUI;
#endif /* STA_EASY_CONFIG_SETUP */

#ifdef WSC_STA_SUPPORT
	WSC_LV_INFO WpsIEBeacon;
	WSC_LV_INFO WpsIEProbeResp;
	WSC_CTRL WscControl;
#ifdef DPA_T
	BOOLEAN bPriorityCtrl;
	INT	WscPbcExtraScanCount;
	INT	WscPbcExtraRemainScanCount;
#endif /* DPA_T */
#ifdef IWSC_SUPPORT
	IWSC_INFO IWscInfo;
#endif /* IWSC_SUPPORT */
#endif /* WSC_STA_SUPPORT */
	CHAR dev_name[16];
	USHORT OriDevType;

	BOOLEAN bTGnWifiTest;
	BOOLEAN bSkipAutoScanConn;

#ifdef RTMP_MAC_PCI
	UCHAR BBPR3;

	/* PS Control has 2 meanings for advanced power save function. */
	/* 1. EnablePSinIdle : When no connection, always radio off except need to do site survey. */
	/* 2. EnableNewPS  : will save more current in sleep or radio off mode. */
	PS_CONTROL PSControl;
#endif /* RTMP_MAC_PCI */

#ifdef EXT_BUILD_CHANNEL_LIST
	UCHAR IEEE80211dClientMode;
	UCHAR StaOriCountryCode[3];
	UCHAR StaOriGeography;
#endif /* EXT_BUILD_CHANNEL_LIST */

#ifdef IP_ASSEMBLY
	BOOLEAN bFragFlag;
#endif /* IP_ASSEMBLY */

#ifdef DOT11R_FT_SUPPORT
	DOT11R_CMN_STRUC Dot11RCommInfo;
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
	WNM_CONFIG WnmCfg;
	BOOLEAN bBSSMantAPSupport;
	BOOLEAN bDMSAPSupport;
	BOOLEAN BTMQuerySend;
	BOOLEAN bDMSSetUp;
	UCHAR BTMQueryDialogToken;
	UCHAR DMSREQDialogToken;
#endif				/* DOT11V_WNM_SUPPORT */

#ifdef DOT11W_PMF_SUPPORT
	PMF_CFG PmfCfg;
#endif /* DOT11W_PMF_SUPPORT */

	BOOLEAN bAutoConnectByBssid;
	ULONG BeaconLostTime;	/* seconds */
	BOOLEAN bForceTxBurst;	/* 1: force enble TX PACKET BURST, 0: disable */
#ifdef XLINK_SUPPORT
	BOOLEAN PSPXlink;	/* 0: Disable. 1: Enable */
#endif /* XLINK_SUPPORT */
	BOOLEAN bAutoConnectIfNoSSID;
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
	UCHAR RegClass;		/*IE_SUPP_REG_CLASS: 2009 PF#3: For 20/40 Intolerant Channel Report */
#endif /* DOT11N_DRAFT3 */
	BOOLEAN bAdhocN;
#endif /* DOT11_N_SUPPORT */
	BOOLEAN bAdhocCreator;	/*TRUE indicates divice is Creator. */
#ifdef HE_BD_CFG80211_SUPPORT
	BOOLEAN bHt40Test; /* 1: recover original bit setting about BSS_COEX and HT40_SHORT GI, 0:disalbe */
#endif /* HE_BD_CFG80211_SUPPORT */

#ifdef THREADX
#ifdef SIGMATEL_SDK
	SY_CONTROL SYControl;
#endif /* SIGMATEL_SDK */
#endif /* THREADX */

	/*
	   Enhancement Scanning Mechanism
	   To decrease the possibility of ping loss
	 */
	BOOLEAN bImprovedScan;
	BOOLEAN BssNr;
	UCHAR ScanChannelCnt;	/* 0 at the beginning of scan, stop at 7 */
	UCHAR LastScanChannel;
	/************************************/

	BOOLEAN bFastConnect;

/*connectinfo  for tmp store connect info from UI*/
	BOOLEAN Connectinfoflag;
	UCHAR   ConnectinfoBssid[MAC_ADDR_LEN];
	UCHAR   ConnectinfoChannel;
	UCHAR   ConnectinfoSsidLen;
	CHAR    ConnectinfoSsid[MAX_LEN_OF_SSID];
	UCHAR ConnectinfoBssType;

#ifdef EASY_CONFIG_SETUP
	EASY_CONFIG_INFO EasyConfigInfo;
#endif /* EASY_CONFIG_SETUP */

#ifdef WIDI_SUPPORT
	BOOLEAN bWIDI;
	BOOLEAN bSendingProbe;
#endif /* WIDI_SUPPORT */

#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
	BOOLEAN AdaptiveFreq;  /* Todo: iwpriv and profile support. */
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */

#ifdef RT3290
	// TODO: shiang, check the purpose of following parameters
	ULONG AntS;
	ULONG AntSAuxDelta;
	ULONG AntSRssiFactor;
	ULONG AntSPERFactor;
	CHAR Rssi[2];
#endif /* RT3290 */

#ifdef CONFIG_DOT11U_INTERWORKING
	GAS_CTRL GASCtrl;
#endif

#ifdef CONFIG_HOTSPOT
	HOTSPOT_CTRL HotSpotCtrl;
#endif

#ifdef CONFIG_DOT11V_WNM
	WNM_CTRL WNMCtrl;
#endif

#ifdef WFD_SUPPORT
	RT_WFD_CONFIG WfdCfg;
#endif /* WFD_SUPPORT */

	PWR_MGMT_STRUCT	PwrMgmt;
} STA_ADMIN_CONFIG, *PSTA_ADMIN_CONFIG;


/*
	This data structure keep the current active BSS/IBSS's configuration that
	this STA had agreed upon joining the network. Which means these parameters
	are usually decided by the BSS/IBSS creator instead of user configuration.
	Data in this data structurre is valid only when either ADHOC_ON()/INFRA_ON()
	is TRUE. Normally, after SCAN or failed roaming attempts, we need to
	recover back to the current active settings
*/
typedef struct _STA_ACTIVE_CONFIG {
	USHORT Aid;
	USHORT AtimWin;		/* in kusec; IBSS parameter set element */
	USHORT CapabilityInfo;
	EXT_CAP_INFO_ELEMENT ExtCapInfo;
	USHORT CfpMaxDuration;
	USHORT CfpPeriod;

	/* Copy supported rate from desired AP's beacon. We are trying to match */
	/* AP's supported and extended rate settings. */
	UCHAR SupRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR ExtRate[MAX_LEN_OF_SUPPORTED_RATES];
	UCHAR SupRateLen;
	UCHAR ExtRateLen;
	/* Copy supported ht from desired AP's beacon. We are trying to match */
	RT_PHY_INFO SupportedPhyInfo;
	RT_HT_CAPABILITY SupportedHtPhy;
#ifdef DOT11_VHT_AC
	RT_VHT_CAP	SupVhtCap;
#endif /* DOT11_VHT_AC */
} STA_ACTIVE_CONFIG;


#ifdef ETH_CONVERT_SUPPORT
#define ETH_CONVERT_NODE_MAX 256

/* Ethernet Convertor operation mode definitions. */
typedef enum {
	ETH_CONVERT_MODE_DISABLE = 0,
	ETH_CONVERT_MODE_DONGLE = 1,	/* Multiple client support, dispatch to AP by device mac address. */
	ETH_CONVERT_MODE_CLONE = 2,	/* Single client support, dispatch to AP by client's mac address. */
	ETH_CONVERT_MODE_HYBRID = 3,	/* Multiple client supprot, dispatch to AP by client's mac address. */
} ETH_CONVERT_MODE;

typedef struct _ETH_CONVERT_STRUCT_ {
	UCHAR EthCloneMac[MAC_ADDR_LEN];	/*Only meanful when ECMode = Clone/Hybrid mode. */
	UCHAR ECMode;		/* 0 = Disable, 1 = Dongle mode, 2 = Clone mode, 3 = Hybrid mode. */
	BOOLEAN CloneMacVaild;	/* 1 if the CloneMac is valid for connection. 0 if not valid. */
/*	UINT32		nodeCount;					// the number of nodes which connect to Internet via us. */
	UCHAR SSIDStr[MAX_LEN_OF_SSID];
	UCHAR SSIDStrLen;
	BOOLEAN macAutoLearn;	/*0: disabled, 1: enabled. */
} ETH_CONVERT_STRUCT;
#endif /* ETH_CONVERT_SUPPORT */

#endif /* CONFIG_STA_SUPPORT */

#ifdef RTMP_RBUS_SUPPORT
#ifdef VIDEO_TURBINE_SUPPORT
/* Video Mode related configuration */
typedef struct _AP_VIDEO_CONFIG {
	BOOLEAN Enable;
	BOOLEAN ClassifierEnable;
	BOOLEAN HighTxMode;
	UCHAR TxPwr;
	BOOLEAN VideoMCSEnable;
	UCHAR VideoMCS;
	UCHAR TxBASize;
	BOOLEAN TxLifeTimeMode;	/* 1: Packet Life Time mode, 0: Retry Limit mode */
	UCHAR TxLifeTime;
	UINT16 TxRetryLimit;
} AP_VIDEO_STRUCT, *PAP_VIDEO_STRUCT;

#endif /* VIDEO_TURBINE_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
/***************************************************************************
  *	AP related data structures
  **************************************************************************/
/* AUTH-RSP State Machine Aux data structure */
typedef struct _AP_MLME_AUX {
	UCHAR Addr[MAC_ADDR_LEN];
	USHORT Alg;
	CHAR Challenge[CIPHER_TEXT_LEN];
} AP_MLME_AUX, *PAP_MLME_AUX;
#endif /* CONFIG_AP_SUPPORT */

#ifdef ADHOC_WPA2PSK_SUPPORT
typedef struct _FOUR_WAY_HANDSHAKE_PROFILE {
	UCHAR ANonce[LEN_KEY_DESC_NONCE];
	UCHAR SNonce[LEN_KEY_DESC_NONCE];
	UCHAR ReplayCounter[LEN_KEY_DESC_REPLAY];
	UCHAR PTK[64];
	UINT8 WpaState;
	UCHAR MsgType;
	RALINK_TIMER_STRUCT MsgRetryTimer;
	UCHAR MsgRetryCounter;
} FOUR_WAY_HANDSHAKE_PROFILE, *PFOUR_WAY_HANDSHAKE_PROFILE;
#endif /* ADHOC_WPA2PSK_SUPPORT */


#if 0
typedef struct _STA_MGMT_ENTRY {
#ifdef CONFIG_AP_SUPPORT
	BSS_STRUCT *pMbss;
#endif /* CONFIG_AP_SUPPORT */

#ifdef P2P_SUPPORT
	UINT32 P2PEntryType; /* 0:Invalid, 1:P2P_GO, 2:P2P_CLI */
	P2P_ENTRY_PARM P2pInfo;
	BOOLEAN bP2pClient;
	UCHAR	DeviceName[P2P_DEVICE_NAME_LEN];
#endif /* P2P_SUPPORT */
#ifdef WFD_SUPPORT
	BOOLEAN bWfdClient;
#endif /* WFD_SUPPORT */


	/* WPA/WPA2 4-way database */
	UCHAR EnqueueEapolStartTimerRunning;	/* Enqueue EAPoL-Start for triggering EAP SM */
	RALINK_TIMER_STRUCT EnqueueStartForPSKTimer;	/* A timer which enqueue EAPoL-Start for triggering PSK SM */
#ifdef ADHOC_WPA2PSK_SUPPORT
	FOUR_WAY_HANDSHAKE_PROFILE WPA_Supplicant;
	FOUR_WAY_HANDSHAKE_PROFILE WPA_Authenticator;
	CIPHER_KEY RxGTK;
	BOOLEAN bPeerHigherMAC;
#ifdef IWSC_SUPPORT
	BOOLEAN bUpdateInfoFromPeerBeacon;
#endif /* IWSC_SUPPORT */
#endif /* ADHOC_WPA2PSK_SUPPORT */

#ifdef IWSC_SUPPORT
	BOOLEAN bIWscSmpbcAccept;
#endif /* IWSC_SUPPORT */

	/* record which entry revoke MIC Failure , if it leaves the BSS itself, AP won't update aMICFailTime MIB */
	UCHAR CMTimerRunning;
	UCHAR RSNIE_Len;
	UCHAR RSN_IE[MAX_LEN_OF_RSNIE];
	UCHAR ANonce[LEN_KEY_DESC_NONCE];
	UCHAR SNonce[LEN_KEY_DESC_NONCE];
	UCHAR R_Counter[LEN_KEY_DESC_REPLAY];
	UCHAR PTK[64];
	UCHAR ReTryCounter;
	RALINK_TIMER_STRUCT RetryTimer;
	NDIS_802_11_AUTHENTICATION_MODE AuthMode;	/* This should match to whatever microsoft defined */
	NDIS_802_11_WEP_STATUS WepStatus;
	NDIS_802_11_WEP_STATUS GroupKeyWepStatus;
	UINT8 WpaState;
	UINT8 GTKState;
	NDIS_802_11_PRIVACY_FILTER PrivacyFilter;	/* PrivacyFilter enum for 802.1X */
	CIPHER_KEY PairwiseKey;
	INT PMKID_CacheIdx;
	UCHAR PMKID[LEN_PMKID];
	UCHAR NegotiatedAKM[LEN_OUI_SUITE];	/* It indicate the negotiated AKM suite */

#ifdef WAPI_SUPPORT
	UCHAR usk_id;		/* unicast key index for WPI */
#endif /* WAPI_SUPPORT */

	BOOLEAN IsReassocSta;	/* Indicate whether this is a reassociation procedure */
	ULONG NoDataIdleCount;
	ULONG AssocDeadLine;
	UINT16 StationKeepAliveCount;	/* unit: second */
	USHORT CapabilityInfo;


	UCHAR VirtualTimeout; /* peer power save virtual timeout */
	ULONG PsQIdleCount;

#ifdef QOS_DLS_SUPPORT
#ifdef CONFIG_STA_SUPPORT
	UCHAR MatchDlsEntryIdx;	/* indicate the index in pAd->StaCfg.DLSEntry */
#endif /* CONFIG_STA_SUPPORT */
	BOOLEAN bDlsInit;
#endif /* QOS_DLS_SUPPORT */

	UINT32 StaConnectTime;	/* the live time of this station since associated with AP */
	UINT32 StaIdleTimeout;	/* idle timeout per entry */

#ifdef UAPSD_SUPPORT
	/* these UAPSD states are used on the fly */
	/* 0:AC_BK, 1:AC_BE, 2:AC_VI, 3:AC_VO */
	BOOLEAN bAPSDCapablePerAC[4];	/* for trigger-enabled */
	BOOLEAN bAPSDDeliverEnabledPerAC[4];	/* for delivery-enabled */

#ifdef WMM_ACM_SUPPORT
	/* because TSPEC can modify the UAPSD flag, we need to keep the APSD flag
	   requested in (re)association stage from the station;
	   we need to recover the APSD flag after the TSPEC is deleted. */
	BOOLEAN bACMAPSDBackup[4];
	BOOLEAN bACMAPSDBackupDeliverEnabled[4];
#endif /* WMM_ACM_SUPPORT */

	UCHAR MaxSPLength;

	BOOLEAN bAPSDAllAC;	/* 1: all AC are delivery-enabled U-APSD */

	QUEUE_HEADER UAPSDQueue[WMM_NUM_OF_AC];	/* queue for each U-APSD */
	USHORT UAPSDQIdleCount;	/* U-APSD queue timeout */

	PQUEUE_ENTRY pUAPSDEOSPFrame;	/* the last U-APSD frame */
	USHORT UAPSDTxNum;	/* total U-APSD frame number */
	BOOLEAN bAPSDFlagEOSPOK;	/* 1: EOSP frame is tx by ASIC */
	BOOLEAN bAPSDFlagSPStart;	/* 1: SP is started */

	/* need to use unsigned long, because time parameters in OS is defined as
	   unsigned long */
	unsigned long UAPSDTimeStampLast;	/* unit: 1000000/OS_HZ */
	BOOLEAN bAPSDFlagSpRoughUse;	/* 1: use rough SP (default: accurate) */

	/* we will set the flag when PS-poll frame is received and
	   clear it when statistics handle.
	   if the flag is set when PS-poll frame is received then calling
	   statistics handler to clear it. */
	BOOLEAN bAPSDFlagLegacySent;	/* 1: Legacy PS sent but yet statistics handle */

#ifdef RTMP_MAC_USB
	UINT32 UAPSDTagOffset[WMM_NUM_OF_AC];
#endif /* RTMP_MAC_USB */
#endif /* UAPSD_SUPPORT */

#ifdef STREAM_MODE_SUPPORT
	UINT32 StreamModeMACReg;	/* MAC reg used to control stream mode for this client. 0=>No stream mode */
#endif /* STREAM_MODE_SUPPORT */


	UINT FIFOCount;
	UINT DebugFIFOCount;
	UINT DebugTxCount;

/* ==================================================== */
	EXT_CAP_INFO_ELEMENT ext_cap;

#ifdef DOT11_N_SUPPORT
	HT_CAPABILITY_IE HTCapability;

#ifdef DOT11N_DRAFT3
	UCHAR BSS2040CoexistenceMgmtSupport;
	BOOLEAN bForty_Mhz_Intolerant;
#endif /* DOT11N_DRAFT3 */

#ifdef DOT11_VHT_AC
	VHT_CAP_IE vht_cap_ie;

	/* only take effect if ext_cap.operating_mode_notification = 1 */
	BOOLEAN force_op_mode;
	OPERATING_MODE operating_mode;
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */


/* ==================================================== */
	enum RATE_ADAPT_ALG rateAlg;
	UCHAR RateLen;
	UCHAR MaxSupportedRate;
	BOOLEAN bAutoTxRateSwitch;
	UCHAR CurrTxRate;
	UCHAR CurrTxRateIndex;
	UCHAR lastRateIdx;
	UCHAR *pTable;	/* Pointer to this entry's Tx Rate Table */

#ifdef NEW_RATE_ADAPT_SUPPORT
	UCHAR lowTrafficCount;
	UCHAR fewPktsCnt;
	BOOLEAN perThrdAdj;
	UCHAR mcsGroup;/* the mcs group to be tried */
#endif /* NEW_RATE_ADAPT_SUPPORT */

#ifdef AGS_SUPPORT
	AGS_CONTROL AGSCtrl;	/* AGS control */
#endif /* AGS_SUPPORT */

	/* to record the each TX rate's quality. 0 is best, the bigger the worse. */
	USHORT TxQuality[MAX_TX_RATE_INDEX + 1];
	BOOLEAN fLastSecAccordingRSSI;
	UCHAR LastSecTxRateChangeAction;	/* 0: no change, 1:rate UP, 2:rate down */
	CHAR LastTimeTxRateChangeAction;	/* Keep last time value of LastSecTxRateChangeAction */
	ULONG LastTxOkCount; /* TxSuccess count in last Rate Adaptation interval */
	UCHAR LastTxPER;	/* Tx PER in last Rate Adaptation interval */
	UCHAR PER[MAX_TX_RATE_INDEX + 1];
	UINT32 CurrTxRateStableTime;	/* # of second in current TX rate */
	UCHAR TxRateUpPenalty;	/* extra # of second penalty due to last unstable condition */

	UCHAR SupportRateMode; /* 1: CCK 2:OFDM 4: HT, 8:VHT */
	BOOLEAN SupportCCKMCS[MAX_LEN_OF_CCK_RATES];
	BOOLEAN SupportOFDMMCS[MAX_LEN_OF_OFDM_RATES];
#ifdef DOT11_N_SUPPORT
	BOOLEAN SupportHTMCS[MAX_LEN_OF_HT_RATES];
#ifdef DOT11_VHT_AC
	BOOLEAN SupportVHTMCS[MAX_LEN_OF_VHT_RATES];
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */

#ifdef MFB_SUPPORT
	UCHAR lastLegalMfb;	/* last legal mfb which is used to set rate */
	BOOLEAN isMfbChanged;	/* purpose: true when mfb has changed but the new mfb is not adopted for Tx */
	struct _RTMP_RA_LEGACY_TB *LegalMfbRS;
	BOOLEAN fLastChangeAccordingMfb;
	NDIS_SPIN_LOCK fLastChangeAccordingMfbLock;
/* Tx MRQ */
	BOOLEAN toTxMrq;
	UCHAR msiToTx, mrqCnt;	/*mrqCnt is used to count down the inverted-BF mrq to be sent */
/* Rx mfb */
	UCHAR pendingMfsi;
/* Tx MFB */
	BOOLEAN toTxMfb;
	UCHAR	mfbToTx;
	UCHAR	mfb0, mfb1;
#endif	/* MFB_SUPPORT */

	UINT32 OneSecTxNoRetryOkCount;
	UINT32 OneSecTxRetryOkCount;
	UINT32 OneSecTxFailCount;
	UINT32 OneSecRxLGICount;		/* unicast-to-me Long GI count */
	UINT32 OneSecRxSGICount;      	/* unicast-to-me Short GI count */

#ifdef FIFO_EXT_SUPPORT
	UINT32 fifoTxSucCnt;
	UINT32 fifoTxRtyCnt;
#endif /* FIFO_EXT_SUPPORT */

	BOOLEAN bWscCapable;
	UCHAR Receive_EapolStart_EapRspId;
#ifdef EASY_CONFIG_SETUP
	BOOLEAN bRaAutoWpsCapable;
#endif /* EASY_CONFIG_SETUP */
#ifdef WAC_SUPPORT
	BOOLEAN bSamsungAutoWpsCapable;
#endif /* WAC_SUPPORT */

	UINT32 TXMCSExpected[MAX_MCS_SET];
	UINT32 TXMCSSuccessful[MAX_MCS_SET];
	UINT32 TXMCSFailed[MAX_MCS_SET];
	UINT32 TXMCSAutoFallBack[MAX_MCS_SET][MAX_MCS_SET];


#ifdef WMM_ACM_SUPPORT
/* reference to acm_extr.h */
/* 8 pointers for IN TSPEC and 8 pointers for OUT TSEPC */
#define WMM_STA_ACM_INFO_SIZE	(sizeof(UINT8 *)*16)
	UCHAR ACM_Info[WMM_STA_ACM_INFO_SIZE];

	UCHAR ACM_NumOfTspecIn;
	UCHAR ACM_NumOfTspecOut;

	UCHAR ACM_NumOfOutTspecInAc[4];
	UCHAR ACM_NumOfInTspecInAc[4];
#endif /* WMM_ACM_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	ULONG LastBeaconRxTime;
#endif /* CONFIG_STA_SUPPORT */

#ifdef WAPI_SUPPORT
	BOOLEAN WapiUskRekeyTimerRunning;
	RALINK_TIMER_STRUCT WapiUskRekeyTimer;
	UINT32 wapi_usk_rekey_cnt;
#endif /* WAPI_SUPPORT */


#ifdef DOT11R_FT_SUPPORT
	FT_MDIE_INFO MdIeInfo;
	FT_FTIE_INFO FtIeInfo;

	UINT8 InitialMDIE[5];
	UINT8 InitialFTIE[256];
	UINT InitialFTIE_Len;

	UCHAR FT_PMK_R0[32];
	UCHAR FT_PMK_R0_NAME[16];
	UCHAR FT_PMK_R1[32];
	UCHAR FT_PMK_R1_NAME[16];
	UCHAR PTK_NAME[16];

	UCHAR FT_UCipher[4];
	UCHAR FT_Akm[4];
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
	RRM_EN_CAP_IE RrmEnCap;
#endif /* DOT11K_RRM_SUPPORT */

#ifdef DOT11W_PMF_SUPPORT
	UCHAR PmfTxTsc[LEN_WPA_TSC];
	UCHAR PmfRxTsc[LEN_WPA_TSC];
        RALINK_TIMER_STRUCT SAQueryTimer;
        RALINK_TIMER_STRUCT SAQueryConfirmTimer;
	UCHAR SAQueryStatus;
        USHORT TransactionID;
#endif /* DOT11W_PMF_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
#ifdef CONFIG_AP_SUPPORT
	BOOLEAN bBSSMantSTASupport;
	BOOLEAN bDMSSTASupport;
	UCHAR BTMQueryDialogToken;
	BOOLEAN DisassociationImminent;
	BOOLEAN BSSTerminationIncluded;
	RALINK_TIMER_STRUCT DisassocTimer;
#endif /* CONFIG_AP_SUPPORT */
#endif /* DOT11V_WNM_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
	UCHAR BssTransitionManmtSupport;
	UCHAR DMSSupport;
	BOOLEAN Beclone;
#endif /* DOT11V_WNM_SUPPORT */


#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
	UINT16			TdlsTxFailCount;
	UINT32			TdlsKeyLifeTimeCount;
	UCHAR			MatchTdlsEntryIdx; // indicate the index in pAd->StaCfg.DLSEntry
#endif // DOT11Z_TDLS_SUPPORT //


#ifdef SMART_ANTENNA
	UINT32		saLstTxNoRtyCnt;
	UINT32		saLstTxRtyCnt;
	UINT32		saLstTxFailCnt;
	UCHAR		*pRateTable;
	UCHAR		orgTxRateCol;
	CHAR		baseAvgRSSI[3];
	INT32		curRSSI[3];
	INT32		cntRSSI[3];
	INT32		rssi_zero_cnt[3];
	INT32 		sumSNR[3];
	INT32		cntSNR[3];
	INT32		sumPreSNR[3];
	INT32		cntPreSNR[3];
	CHAR		avgRssi[3];
	CHAR		prevAvgRssi[3];
	CHAR		avgSNR[3];
	CHAR		avgPreSNR[3];
	CHAR		curAvgRSSI[3];

	UINT32		hwTxSucCnt;
	UINT32		hwTxRtyCnt;
	ULONG		calcTime;

	UINT32		saTxCnt;
	UINT32		saRxCnt;
	ULONG		mcsUsage[33];
	ULONG		curMcsApplyTime;
	CHAR		mcsInUse;

	VOID		*pTrainEntry;
#endif /* SMART_ANTENNA */

	ULONG ChannelQuality;	/* 0..100, Channel Quality Indication for Roaming */

	VOID *pAd;
	//struct _MAC_TABLE_ENTRY *pNext;
}STA_MGMT_ENTRY;
#endif


enum {
	PS_TOKEN_STAT_IDLE=0,   		/*00:no ps packets for this sta*/
	PS_TOKEN_STAT_ERR = 1,             /*01:not use this case */
	PS_TOKEN_STAT_PKT=2,		/*02:wcid not in queue but still ps packtes in packet queue*/
	PS_TOKEN_STAT_WCID_PKT=3,	/*03:wcid in queue and still ps packtet in packte queue*/
};

typedef struct _DUPLICATED_FRAME{
	INT prev_mgmt_frame_sn;
	UCHAR prev_mgmt_src_addr[6];
	}DUPLICATED_FRAME;

typedef struct _STA_TR_ENTRY{
	UINT32 EntryType;
	struct wifi_dev *wdev;

	UCHAR wcid;
	/*
		func_tb_idx used to indicate following index:
			in ApCfg.ApCliTab
			in pAd->MeshTab
			in WdsTab.MacTab
	*/
	// TODO:shiang-usw, need to integrate this parameter to wdev_idx or something else!!
	UCHAR func_tb_idx;
	UCHAR Addr[MAC_ADDR_LEN];

	/*
		Tx Info
	*/
	USHORT NonQosDataSeq;
	USHORT TxSeq[NUM_OF_TID];

	QUEUE_HEADER tx_queue[WMM_QUE_NUM];
	QUEUE_HEADER ps_queue;
	UINT	enqCount;
	UINT	TokenCount[WMM_QUE_NUM];
#ifdef MT_PS
#if 0
	QUEUE_HEADER mt_ps_queue[WMM_QUE_NUM];
#endif
	BOOLEAN bEospNullSnd;
	UCHAR EospNullUp;
#endif /* MT_PS */
	INT		ps_qbitmap;
	UCHAR		ps_state;
	UCHAR		retrieve_start_state;
	UCHAR		token_enq_all_fail;

	BOOLEAN tx_pend_for_agg[WMM_QUE_NUM];
	NDIS_SPIN_LOCK txq_lock[WMM_QUE_NUM];
	NDIS_SPIN_LOCK ps_queue_lock;
	UINT deq_cnt;
	UINT deq_bytes;
	ULONG PsQIdleCount;

	BOOLEAN enq_cap;
	BOOLEAN deq_cap;
	UCHAR PsTokenFlag;

	/*
		STA status
	*/

	UCHAR bssid[MAC_ADDR_LEN];
	BOOLEAN bIAmBadAtheros;	/* Flag if this is Atheros chip that has IOT problem.  We need to turn on RTS/CTS protection. */
	BOOLEAN isCached;
	UCHAR PortSecured;
	UCHAR PsMode;
	UCHAR FlgPsModeIsWakeForAWhile; /* wake up for a while until a condition */
#if 1 // def WDS_SUPPORT
	BOOLEAN LockEntryTx;	/* TRUE = block to WDS Entry traffic, FALSE = not. */
#endif /* WDS_SUPPORT */

#ifdef WAC_SUPPORT
#ifdef WAC_QOS_PRIORITY
	BOOLEAN bSamsungForcePriority;
#endif /* WAC_QOS_PRIORITY */
#endif /* WAC_SUPPORT */

	UCHAR CurrTxRate;

#ifdef VENDOR_FEATURE1_SUPPORT
	/* total 128B, use UINT32 to avoid alignment problem */
	UINT32 HeaderBuf[32];	/* (total 128B) TempBuffer for TX_INFO + TX_WI + 802.11 Header + padding + AMSDU SubHeader + LLC/SNAP */
	UCHAR HdrPadLen;	/* recording Header Padding Length; */
	UCHAR MpduHeaderLen;
	UCHAR wifi_hdr_len;
	UINT16 Protocol;
#endif /* VENDOR_FEATURE1_SUPPORT */

#ifdef DOT11_N_SUPPORT
	UINT32 CachedBuf[16];	/* UINT (4 bytes) for alignment */

	USHORT RXBAbitmap;	/* fill to on-chip  RXWI_BA_BITMASK in 8.1.3RX attribute entry format */
	USHORT TXBAbitmap;	/* This bitmap as originator, only keep in software used to mark AMPDU bit in TXWI */
	USHORT TXAutoBAbitmap;
	USHORT BADeclineBitmap;
	USHORT BARecWcidArray[NUM_OF_TID];	/* The mapping wcid of recipient session. if RXBAbitmap bit is masked */
	USHORT BAOriWcidArray[NUM_OF_TID];	/* The mapping wcid of originator session. if TXBAbitmap bit is masked */
	USHORT BAOriSequence[NUM_OF_TID];	/* The mapping wcid of originator session. if TXBAbitmap bit is masked */

	UCHAR MpduDensity;
	UCHAR MaxRAmpduFactor;
	UCHAR AMsduSize;
	UCHAR MmpsMode;		/* MIMO power save mode. */
#endif /* DOT11_N_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
#ifdef MAC_REPEATER_SUPPORT
	BOOLEAN bReptCli;
	BOOLEAN bReptEthCli;
	UCHAR MatchReptCliIdx;
	UCHAR ReptCliAddr[MAC_ADDR_LEN];
	ULONG ReptCliIdleCount;
#endif /* MAC_REPEATER_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */


#ifdef TXBF_SUPPORT
	UCHAR			TxSndgType;
	NDIS_SPIN_LOCK	TxSndgLock;

/* ETxBF */
	UCHAR		bfState;
	UCHAR		sndgMcs;
	UCHAR		sndgBW;
	UCHAR		sndg0Mcs;
	INT			sndg0Snr0, sndg0Snr1, sndg0Snr2;

#ifdef ETXBF_EN_COND3_SUPPORT
	UCHAR		bestMethod;
	UCHAR		sndgRateIdx;
	UCHAR		bf0Mcs, sndg0RateIdx, bf0RateIdx;
	UCHAR		sndg1Mcs, bf1Mcs, sndg1RateIdx, bf1RateIdx;
	INT			sndg1Snr0, sndg1Snr1, sndg1Snr2;
#endif /* ETXBF_EN_COND3_SUPPORT */
	UCHAR		noSndgCnt;
	UCHAR		eTxBfEnCond;
	UCHAR		noSndgCntThrd, ndpSndgStreams;
	UCHAR		iTxBfEn;
	RALINK_TIMER_STRUCT eTxBfProbeTimer;

	BOOLEAN		phyETxBf;			/* True=>Set ETxBF bit in PHY rate */
	BOOLEAN		phyITxBf;			/* True=>Set ITxBF bit in PHY rate */
	UCHAR		lastNonBfRate;		/* Last good non-BF rate */
	BOOLEAN		lastRatePhyTxBf;	/* For Quick Check. True if last rate was BF */
	USHORT      BfTxQuality[MAX_TX_RATE_INDEX + 1];	/* Beamformed TX Quality */

	COUNTER_TXBF TxBFCounters;		/* TxBF Statistics */
	UINT LastETxCount;		/* Used to compute %BF statistics */
	UINT LastITxCount;
	UINT LastTxCount;
#endif /* TXBF_SUPPORT */

#ifdef VHT_TXBF_SUPPORT
	UINT8 snd_dialog_token;
#ifdef SOFT_SOUNDING
	BOOLEAN snd_reqired;
	HTTRANSMIT_SETTING snd_rate;
#endif /* SOFT_SOUNDING */
#endif /* VHT_TXBF_SUPPORT */


	/*
		Statistics related parameters
	*/
	UINT32 ContinueTxFailCnt;
	ULONG TimeStamp_toTxRing;
	ULONG NoDataIdleCount;

#ifdef CONFIG_AP_SUPPORT
	LARGE_INTEGER TxPackets;
	LARGE_INTEGER RxPackets;
	ULONG TxBytes;
	ULONG RxBytes;
#endif /* CONFIG_AP_SUPPORT */
 /* 
 	Used to ignore consecutive PS poll.
 	set: when we get a PS poll.
 	clear: when a PS data is sent or two period passed.
 */
	UINT8 PsDeQWaitCnt;
	INT cacheSn[NUM_OF_UP];
}STA_TR_ENTRY;


typedef struct _MAC_TABLE_ENTRY {
	UINT32 EntryType;
	struct wifi_dev *wdev;
	PVOID pAd;
	struct _MAC_TABLE_ENTRY *pNext;

	/*
		A bitmap of BOOLEAN flags. each bit represent an operation status of a particular
		BOOLEAN control, either ON or OFF. These flags should always be accessed via
		CLIENT_STATUS_TEST_FLAG(), CLIENT_STATUS_SET_FLAG(), CLIENT_STATUS_CLEAR_FLAG() macros.
		see fOP_STATUS_xxx in RTMP_DEF.C for detail bit definition. fCLIENT_STATUS_AMSDU_INUSED
	*/
	ULONG ClientStatusFlags;
	ULONG cli_cap_flags;

	HTTRANSMIT_SETTING HTPhyMode, MaxHTPhyMode;	/* For transmit phy setting in TXWI. */
	HTTRANSMIT_SETTING MinHTPhyMode;

/*
	wcid:

	tr_tb_idx:

	func_tb_idx used to indicate following index:
		in ApCfg.ApCliTab
		in pAd->MeshTab
		in WdsTab.MacTab

	apidx: should remove this
*/
	UCHAR wcid;
	UCHAR tr_tb_idx;
	UCHAR func_tb_idx;
	UCHAR apidx;		/* MBSS number */

	BOOLEAN isRalink;
	BOOLEAN bIAmBadAtheros;	/* Flag if this is Atheros chip that has IOT problem.  We need to turn on RTS/CTS protection. */

	UCHAR Addr[MAC_ADDR_LEN];
#ifdef CONFIG_AP_SUPPORT
	BSS_STRUCT *pMbss;
#endif /* CONFIG_AP_SUPPORT */

	/*
		STATE MACHINE Status
	*/
	USHORT Aid;	/* in range 1~2007, with bit 14~15 = b'11, e.g., 0xc001~0xc7d7 */
	SST Sst;
	AUTH_STATE AuthState;	/* for SHARED KEY authentication state machine used only */

#ifdef P2P_SUPPORT
	UINT32 P2PEntryType; /* 0:Invalid, 1:P2P_GO, 2:P2P_CLI */
	P2P_ENTRY_PARM P2pInfo;
	BOOLEAN bP2pClient;
	UCHAR DeviceName[P2P_DEVICE_NAME_LEN];
#else 
#ifdef RT_CFG80211_P2P_SUPPORT
	UINT32 P2PEntryType; /* 0:Invalid, 1:P2P_GO, 2:P2P_CLI */
#endif /* RT_CFG80211_P2P_SUPPORT */
#endif /* P2P_SUPPORT */
#ifdef WFD_SUPPORT
	BOOLEAN bWfdClient;
#endif /* WFD_SUPPORT */

	/* Rx status related parameters */
	RSSI_SAMPLE RssiSample;
	UINT32 LastTxRate;
	UINT32 LastRxRate;
	SHORT freqOffset;		/* Last RXWI FOFFSET */
	SHORT freqOffsetValid;	/* Set when freqOffset field has been updated */

#if defined(RT2883) || defined(RT3883) || defined(RT3593)
	INT32 BF_SNR[3];	/* Last RXWI BF SNR. Units=0.25 dB */
#endif /* defined(RT2883) || defined(RT3883) || defined(RT3593) */

#ifdef WAPI_SUPPORT
	UCHAR usk_id;		/* unicast key index for WPI */
#endif /* WAPI_SUPPORT */

	/* WPA/WPA2 4-way database */
	UCHAR EnqueueEapolStartTimerRunning;	/* Enqueue EAPoL-Start for triggering EAP SM */
	RALINK_TIMER_STRUCT EnqueueStartForPSKTimer;	/* A timer which enqueue EAPoL-Start for triggering PSK SM */
#ifdef ADHOC_WPA2PSK_SUPPORT
	FOUR_WAY_HANDSHAKE_PROFILE WPA_Supplicant;
	FOUR_WAY_HANDSHAKE_PROFILE WPA_Authenticator;
	CIPHER_KEY RxGTK;
	BOOLEAN bPeerHigherMAC;
#ifdef IWSC_SUPPORT
	BOOLEAN bUpdateInfoFromPeerBeacon;
#endif /* IWSC_SUPPORT */
#endif /* ADHOC_WPA2PSK_SUPPORT */

#ifdef IWSC_SUPPORT
	BOOLEAN bIWscSmpbcAccept;
#endif /* IWSC_SUPPORT */

	/* record which entry revoke MIC Failure , if it leaves the BSS itself, AP won't update aMICFailTime MIB */
	UCHAR CMTimerRunning;
	UCHAR RSNIE_Len;
	UCHAR RSN_IE[MAX_LEN_OF_RSNIE];
	UCHAR ANonce[LEN_KEY_DESC_NONCE];
	UCHAR SNonce[LEN_KEY_DESC_NONCE];
	UCHAR R_Counter[LEN_KEY_DESC_REPLAY];
	UCHAR PTK[64];
	UCHAR ReTryCounter;
	RALINK_TIMER_STRUCT RetryTimer;
	NDIS_802_11_AUTHENTICATION_MODE AuthMode;	/* This should match to whatever microsoft defined */
	NDIS_802_11_WEP_STATUS WepStatus;
	NDIS_802_11_WEP_STATUS GroupKeyWepStatus;
	UINT8 WpaState;
	UINT8 GTKState;
	NDIS_802_11_PRIVACY_FILTER PrivacyFilter;	/* PrivacyFilter enum for 802.1X */
	CIPHER_KEY PairwiseKey;
	INT PMKID_CacheIdx;
	UCHAR PMKID[LEN_PMKID];
	UCHAR NegotiatedAKM[LEN_OUI_SUITE];	/* It indicate the negotiated AKM suite */

	UCHAR bssid[MAC_ADDR_LEN];
	BOOLEAN IsReassocSta;	/* Indicate whether this is a reassociation procedure */
	ULONG NoDataIdleCount;
	ULONG AssocDeadLine;
	UINT16 StationKeepAliveCount;	/* unit: second */
	USHORT CapabilityInfo;
	UCHAR PsMode;
	UCHAR FlgPsModeIsWakeForAWhile; /* wake up for a while until a condition */
	UCHAR VirtualTimeout; /* peer power save virtual timeout */

#ifdef WDS_SUPPORT
	BOOLEAN LockEntryTx;	/* TRUE = block to WDS Entry traffic, FALSE = not. */
#endif /* WDS_SUPPORT */

#ifdef QOS_DLS_SUPPORT
#ifdef CONFIG_STA_SUPPORT
	UCHAR MatchDlsEntryIdx;	/* indicate the index in pAd->StaCfg.DLSEntry */
#endif /* CONFIG_STA_SUPPORT */
	BOOLEAN bDlsInit;
#endif /* QOS_DLS_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
#ifdef MAC_REPEATER_SUPPORT
	BOOLEAN bReptCli;
	BOOLEAN bReptEthCli;
	UCHAR MatchReptCliIdx;
	UCHAR ReptCliAddr[MAC_ADDR_LEN];
	ULONG ReptCliIdleCount;
#endif /* MAC_REPEATER_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	UINT32 StaConnectTime;	/* the live time of this station since associated with AP */
	UINT32 StaIdleTimeout;	/* idle timeout per entry */

#ifdef UAPSD_SUPPORT
	/* these UAPSD states are used on the fly */
	/* 0:AC_BK, 1:AC_BE, 2:AC_VI, 3:AC_VO */
	BOOLEAN bAPSDCapablePerAC[4];	/* for trigger-enabled */
	BOOLEAN bAPSDDeliverEnabledPerAC[4];	/* for delivery-enabled */

#ifdef WMM_ACM_SUPPORT
	/* because TSPEC can modify the UAPSD flag, we need to keep the APSD flag
	   requested in (re)association stage from the station;
	   we need to recover the APSD flag after the TSPEC is deleted. */
	BOOLEAN bACMAPSDBackup[4];
	BOOLEAN bACMAPSDBackupDeliverEnabled[4];
#endif /* WMM_ACM_SUPPORT */

	UCHAR MaxSPLength;

	BOOLEAN bAPSDAllAC;	/* 1: all AC are delivery-enabled U-APSD */

	QUEUE_HEADER UAPSDQueue[WMM_NUM_OF_AC];	/* queue for each U-APSD */
	USHORT UAPSDQIdleCount;	/* U-APSD queue timeout */

	PQUEUE_ENTRY pUAPSDEOSPFrame;	/* the last U-APSD frame */
	USHORT UAPSDTxNum;	/* total U-APSD frame number */
	BOOLEAN bAPSDFlagEOSPOK;	/* 1: EOSP frame is tx by ASIC */
	BOOLEAN bAPSDFlagSPStart;	/* 1: SP is started */

	/* need to use unsigned long, because time parameters in OS is defined as
	   unsigned long */
	unsigned long UAPSDTimeStampLast;	/* unit: 1000000/OS_HZ */
	BOOLEAN bAPSDFlagSpRoughUse;	/* 1: use rough SP (default: accurate) */

	/* we will set the flag when PS-poll frame is received and
	   clear it when statistics handle.
	   if the flag is set when PS-poll frame is received then calling
	   statistics handler to clear it. */
	BOOLEAN bAPSDFlagLegacySent;	/* 1: Legacy PS sent but yet statistics handle */

#ifdef RTMP_MAC_USB
	UINT32 UAPSDTagOffset[WMM_NUM_OF_AC];
#endif /* RTMP_MAC_USB */
#endif /* UAPSD_SUPPORT */

#ifdef STREAM_MODE_SUPPORT
	UINT32 StreamModeMACReg;	/* MAC reg used to control stream mode for this client. 0=>No stream mode */
#endif /* STREAM_MODE_SUPPORT */

	UINT FIFOCount;
	UINT DebugFIFOCount;
	UINT DebugTxCount;

#ifdef DOT11_N_SUPPORT
	USHORT NoBADataCountDown;
#endif /* DOT11_N_SUPPORT */

/* ==================================================== */
	enum RATE_ADAPT_ALG rateAlg;
	// TODO: shiang-usw, use following parameters to replace "RateLen/MaxSupportedRate"
#if 0
	UINT8 rate_cap_cck;	/* 1/2/5.5/11 */
	UINT8 rate_cap_ofdm;	/* 6/9/12/18/24/36/48/54 */
#ifdef DOT11_N_SUPPORT
	UINT8 rate_cap_ht[5];	/* MCS0~7, 8~15, 16~23, 24~31, 32 */
#endif /* DOT11_N_SUPPORT */
#ifdef DOT11_VHT_AC
	UINT16 rate_cap_vht[2];	/* 1SS MCS0~9, 2SS MCS0~9 */
#endif /* DOT11_VHT_AC */
#else
	UCHAR RateLen;
	UCHAR MaxSupportedRate;
#endif

	BOOLEAN bAutoTxRateSwitch;
	UCHAR CurrTxRate;
	UCHAR CurrTxRateIndex;
	UCHAR lastRateIdx;
	UCHAR *pTable;	/* Pointer to this entry's Tx Rate Table */

	UCHAR ucMaxTxRetryCnt;

	BOOLEAN fgDisableCCK;
#ifdef NEW_RATE_ADAPT_SUPPORT
	UCHAR lowTrafficCount;
	UCHAR fewPktsCnt;
	BOOLEAN perThrdAdj;
	UCHAR mcsGroup;/* the mcs group to be tried */
#endif /* NEW_RATE_ADAPT_SUPPORT */

#ifdef AGS_SUPPORT
	AGS_CONTROL AGSCtrl;	/* AGS control */
#endif /* AGS_SUPPORT */

	/* to record the each TX rate's quality. 0 is best, the bigger the worse. */
	USHORT TxQuality[MAX_TX_RATE_INDEX + 1];
	BOOLEAN fLastSecAccordingRSSI;
	UCHAR LastSecTxRateChangeAction;	/* 0: no change, 1:rate UP, 2:rate down */
	CHAR LastTimeTxRateChangeAction;	/* Keep last time value of LastSecTxRateChangeAction */
	ULONG LastTxOkCount; /* TxSuccess count in last Rate Adaptation interval */
	UCHAR LastTxPER;	/* Tx PER in last Rate Adaptation interval */
	UCHAR PER[MAX_TX_RATE_INDEX + 1];
	UINT32 CurrTxRateStableTime;	/* # of second in current TX rate */
	UCHAR TxRateUpPenalty;	/* extra # of second penalty due to last unstable condition */

	UCHAR SupportRateMode; /* 1: CCK 2:OFDM 4: HT, 8:VHT */
	BOOLEAN SupportCCKMCS[MAX_LEN_OF_CCK_RATES];
	BOOLEAN SupportOFDMMCS[MAX_LEN_OF_OFDM_RATES];
#ifdef DOT11_N_SUPPORT
	BOOLEAN SupportHTMCS[MAX_LEN_OF_HT_RATES];
#ifdef DOT11_VHT_AC
	BOOLEAN SupportVHTMCS[MAX_LEN_OF_VHT_RATES];
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */

#ifdef PEER_DELBA_TX_ADAPT
	BOOLEAN bPeerDelBaTxAdaptEn;
#endif /* PEER_DELBA_TX_ADAPT */

#ifdef MFB_SUPPORT
	UCHAR lastLegalMfb;	/* last legal mfb which is used to set rate */
	BOOLEAN isMfbChanged;	/* purpose: true when mfb has changed but the new mfb is not adopted for Tx */
	struct _RTMP_RA_LEGACY_TB *LegalMfbRS;
	BOOLEAN fLastChangeAccordingMfb;
	NDIS_SPIN_LOCK fLastChangeAccordingMfbLock;
/* Tx MRQ */
	BOOLEAN toTxMrq;
	UCHAR msiToTx, mrqCnt;	/*mrqCnt is used to count down the inverted-BF mrq to be sent */
/* Rx mfb */
	UCHAR pendingMfsi;
/* Tx MFB */
	BOOLEAN toTxMfb;
	UCHAR	mfbToTx;
	UCHAR	mfb0, mfb1;
#endif	/* MFB_SUPPORT */
#ifdef TXBF_SUPPORT
	UCHAR			TxSndgType;
	NDIS_SPIN_LOCK	TxSndgLock;

/* ETxBF */
	UCHAR		bfState;
	UCHAR		sndgMcs;
	UCHAR		sndgBW;
	UCHAR		sndg0Mcs;
	INT			sndg0Snr0, sndg0Snr1, sndg0Snr2;

#ifdef ETXBF_EN_COND3_SUPPORT
	UCHAR		bestMethod;
	UCHAR		sndgRateIdx;
	UCHAR		bf0Mcs, sndg0RateIdx, bf0RateIdx;
	UCHAR		sndg1Mcs, bf1Mcs, sndg1RateIdx, bf1RateIdx;
	INT			sndg1Snr0, sndg1Snr1, sndg1Snr2;
#endif /* ETXBF_EN_COND3_SUPPORT */
	UCHAR		noSndgCnt;
	UCHAR		eTxBfEnCond;
	UCHAR		noSndgCntThrd, ndpSndgStreams;
	UCHAR		iTxBfEn;
	RALINK_TIMER_STRUCT eTxBfProbeTimer;

	BOOLEAN		phyETxBf;			/* True=>Set ETxBF bit in PHY rate */
	BOOLEAN		phyITxBf;			/* True=>Set ITxBF bit in PHY rate */
	UCHAR		lastNonBfRate;		/* Last good non-BF rate */
	BOOLEAN		lastRatePhyTxBf;	/* For Quick Check. True if last rate was BF */
	USHORT      BfTxQuality[MAX_TX_RATE_INDEX + 1];	/* Beamformed TX Quality */

	COUNTER_TXBF TxBFCounters;		/* TxBF Statistics */
	UINT LastETxCount;		/* Used to compute %BF statistics */
	UINT LastITxCount;
	UINT LastTxCount;
#endif /* TXBF_SUPPORT */

#ifdef VHT_TXBF_SUPPORT
	UINT8 snd_dialog_token;
#ifdef SOFT_SOUNDING
	BOOLEAN snd_reqired;
	HTTRANSMIT_SETTING snd_rate;
#endif /* SOFT_SOUNDING */
#endif /* VHT_TXBF_SUPPORT */

	UINT32 OneSecTxNoRetryOkCount;
	UINT32 OneSecTxRetryOkCount;
	UINT32 OneSecTxFailCount;
	UINT32 OneSecRxLGICount;		/* unicast-to-me Long GI count */
	UINT32 OneSecRxSGICount;      	/* unicast-to-me Short GI count */

#ifdef FIFO_EXT_SUPPORT
	UINT32 fifoTxSucCnt;
	UINT32 fifoTxRtyCnt;
#endif /* FIFO_EXT_SUPPORT */

	UINT32 ContinueTxFailCnt;
	ULONG TimeStamp_toTxRing;

/*==================================================== */
	EXT_CAP_INFO_ELEMENT ext_cap;

#ifdef DOT11_N_SUPPORT
	HT_CAPABILITY_IE HTCapability;

	USHORT RXBAbitmap;	/* fill to on-chip  RXWI_BA_BITMASK in 8.1.3RX attribute entry format */
	USHORT TXBAbitmap;	/* This bitmap as originator, only keep in software used to mark AMPDU bit in TXWI */
	USHORT TXAutoBAbitmap;
	USHORT BADeclineBitmap;
	USHORT BARecWcidArray[NUM_OF_TID];	/* The mapping wcid of recipient session. if RXBAbitmap bit is masked */
	USHORT BAOriWcidArray[NUM_OF_TID];	/* The mapping wcid of originator session. if TXBAbitmap bit is masked */
	USHORT BAOriSequence[NUM_OF_TID];	/* The mapping wcid of originator session. if TXBAbitmap bit is masked */

	UCHAR MpduDensity;
	UCHAR MaxRAmpduFactor;
	UCHAR AMsduSize;
	UCHAR MmpsMode;		/* MIMO power save mode. */

#ifdef DOT11N_DRAFT3
	UCHAR BSS2040CoexistenceMgmtSupport;
	BOOLEAN bForty_Mhz_Intolerant;
#endif /* DOT11N_DRAFT3 */

#ifdef DOT11_VHT_AC
	VHT_CAP_IE vht_cap_ie;

	/* only take effect if ext_cap.operating_mode_notification = 1 */
	BOOLEAN force_op_mode;
	OPERATING_MODE operating_mode;
#endif /* DOT11_VHT_AC */
#endif /* DOT11_N_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
	UCHAR BssTransitionManmtSupport;
	UCHAR DMSSupport;
	BOOLEAN Beclone;
#endif /* DOT11V_WNM_SUPPORT */

	BOOLEAN bWscCapable;
	UCHAR Receive_EapolStart_EapRspId;
#ifdef EASY_CONFIG_SETUP
	BOOLEAN bRaAutoWpsCapable;
#endif /* EASY_CONFIG_SETUP */
#ifdef WAC_SUPPORT
	BOOLEAN bSamsungAutoWpsCapable;
#endif /* WAC_SUPPORT */

	UINT32 TXMCSExpected[MAX_MCS_SET];
	UINT32 TXMCSSuccessful[MAX_MCS_SET];
	UINT32 TXMCSFailed[MAX_MCS_SET];
	UINT32 TXMCSAutoFallBack[MAX_MCS_SET][MAX_MCS_SET];

#ifdef CONFIG_STA_SUPPORT
	ULONG LastBeaconRxTime;
#endif /* CONFIG_STA_SUPPORT */

#ifdef WAPI_SUPPORT
	BOOLEAN WapiUskRekeyTimerRunning;
	RALINK_TIMER_STRUCT WapiUskRekeyTimer;
	UINT32 wapi_usk_rekey_cnt;
#endif /* WAPI_SUPPORT */

#ifdef WMM_ACM_SUPPORT
/* reference to acm_extr.h */
/* 8 pointers for IN TSPEC and 8 pointers for OUT TSEPC */
#define WMM_STA_ACM_INFO_SIZE	(sizeof(UINT8 *)*16)
	UCHAR ACM_Info[WMM_STA_ACM_INFO_SIZE];

	UCHAR ACM_NumOfTspecIn;
	UCHAR ACM_NumOfTspecOut;

	UCHAR ACM_NumOfOutTspecInAc[4];
	UCHAR ACM_NumOfInTspecInAc[4];
#endif /* WMM_ACM_SUPPORT */

#ifdef DOT11R_FT_SUPPORT
	FT_MDIE_INFO MdIeInfo;
	FT_FTIE_INFO FtIeInfo;

	UINT8 InitialMDIE[5];
	UINT8 InitialFTIE[256];
	UINT InitialFTIE_Len;

	UCHAR FT_PMK_R0[32];
	UCHAR FT_PMK_R0_NAME[16];
	UCHAR FT_PMK_R1[32];
	UCHAR FT_PMK_R1_NAME[16];
	UCHAR PTK_NAME[16];

	UCHAR FT_UCipher[4];
	UCHAR FT_Akm[4];
#endif /* DOT11R_FT_SUPPORT */

#ifdef DOT11K_RRM_SUPPORT
	RRM_EN_CAP_IE RrmEnCap;
#endif /* DOT11K_RRM_SUPPORT */

#ifdef DOT11W_PMF_SUPPORT
	UCHAR PmfTxTsc[LEN_WPA_TSC];
	UCHAR PmfRxTsc[LEN_WPA_TSC];
        RALINK_TIMER_STRUCT SAQueryTimer;
        RALINK_TIMER_STRUCT SAQueryConfirmTimer;
	UCHAR SAQueryStatus;
        USHORT TransactionID;
#endif /* DOT11W_PMF_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
#ifdef CONFIG_AP_SUPPORT
	BOOLEAN bBSSMantSTASupport;
	BOOLEAN bDMSSTASupport;
	UCHAR BTMQueryDialogToken;
	BOOLEAN DisassociationImminent;
	BOOLEAN BSSTerminationIncluded;
	RALINK_TIMER_STRUCT DisassocTimer;
#endif /* CONFIG_AP_SUPPORT */
#endif /* DOT11V_WNM_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	LARGE_INTEGER TxPackets;
	LARGE_INTEGER RxPackets;
	ULONG TxBytes;
	ULONG RxBytes;
#endif /* CONFIG_AP_SUPPORT */

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
	UINT16			TdlsTxFailCount;
	UINT32			TdlsKeyLifeTimeCount;
	UCHAR			MatchTdlsEntryIdx; // indicate the index in pAd->StaCfg.DLSEntry
#endif // DOT11Z_TDLS_SUPPORT //

#ifdef SMART_ANTENNA
	UINT32		saLstTxNoRtyCnt;
	UINT32		saLstTxRtyCnt;
	UINT32		saLstTxFailCnt;
	UCHAR		*pRateTable;
	UCHAR		orgTxRateCol;
	CHAR		baseAvgRSSI[3];
	INT32		curRSSI[3];
	INT32		cntRSSI[3];
	INT32		rssi_zero_cnt[3];
	INT32 		sumSNR[3];
	INT32		cntSNR[3];
	INT32		sumPreSNR[3];
	INT32		cntPreSNR[3];
	CHAR		avgRssi[3];
	CHAR		prevAvgRssi[3];
	CHAR		avgSNR[3];
	CHAR		avgPreSNR[3];
	CHAR		curAvgRSSI[3];

	UINT32		hwTxSucCnt;
	UINT32		hwTxRtyCnt;
	ULONG		calcTime;

	UINT32		saTxCnt;
	UINT32		saRxCnt;
	ULONG		mcsUsage[33];
	ULONG		curMcsApplyTime;
	CHAR		mcsInUse;

	VOID		*pTrainEntry;
#endif /* SMART_ANTENNA */

	ULONG ChannelQuality;	/* 0..100, Channel Quality Indication for Roaming */
#ifdef CONFIG_HOTSPOT_R2
	UINT16				BTMDisassocCount;
	UCHAR				IsWNMReqValid;
	UCHAR				IsBTMReqValid;
	UCHAR				QosMapSupport;
	UCHAR				DscpExceptionCount;
	USHORT 				DscpRange[8];
	USHORT 				DscpException[21];	
	struct wnm_req_data	*ReqData;
	struct btm_req_data	*ReqbtmData;
	struct _sta_hs_info hs_info;
	UCHAR				IsKeep;
#endif /* CONFIG_HOTSPOT_R2 */
#ifdef MT_PS
	UCHAR i_psm; /* 0: disable, 1: enable */
#endif /* MT_PS */
} MAC_TABLE_ENTRY, *PMAC_TABLE_ENTRY;


typedef enum _MAC_ENT_STATUS_{
	/* fAnyStationInPsm */
	MAC_TB_ANY_PSM = 0x1,
	/*
		fAnyStationBadAtheros
		Check if any Station is atheros 802.11n Chip.  We need to use RTS/CTS with Atheros 802,.11n chip.
	*/
	MAC_TB_ANY_ATH = 0x2,
	/*
		fAnyTxOPForceDisable
		Check if it is necessary to disable BE TxOP
	*/
	MAC_TB_FORCE_TxOP = 0x4,
	/*
		fAllStationAsRalink
		Check if all stations are ralink-chipset
	*/
	MAC_TB_ALL_RALINK = 0x8,
	/*
		fAnyStationIsLegacy
		Check if I use legacy rate to transmit to my BSS Station
	*/
	MAC_TB_ANY_LEGACY = 0x10,
	/*
		fAnyStationNonGF
		Check if any Station can't support GF
	*/
	MAC_TB_ANY_NON_GF = 0x20,
	/* fAnyStation20Only */
	MAC_TB_ANY_HT20 = 0x40,
	/*
		fAnyStationMIMOPSDynamic
		Check if any Station is MIMO Dynamic
	*/
	MAC_TB_ANY_MIMO_DYNAMIC = 0x80,
	/*
		fAnyBASession
		Check if there is BA session.  Force turn on RTS/CTS
	*/
	MAC_TB_ANY_BA = 0x100,
	/* fAnyStaFortyIntolerant */
	MAC_TB_ANY_40_INTOlERANT = 0x200,
	/*
		fAllStationGainGoodMCS
		Check if all stations more than MCS threshold
	*/
	MAC_TB_ALL_GOOD_MCS = 0x400,
	/*
		fAnyStationIsHT
		Check if still has any station set the Intolerant bit on!
	*/
	MAC_TB_ANY_HT = 0x800,
	/* fAnyWapiStation */
	MAC_TB_ANY_WAPI = 0x1000,
}MAC_ENT_STATUS;

typedef struct _MAC_TABLE {
	MAC_TABLE_ENTRY *Hash[HASH_TABLE_SIZE];
	MAC_TABLE_ENTRY Content[MAX_LEN_OF_MAC_TABLE];
	UINT16 Size;
	STA_TR_ENTRY tr_entry[MAX_LEN_OF_TR_TABLE];
	QUEUE_HEADER McastPsQueue;
	ULONG PsQIdleCount;
	MAC_ENT_STATUS sta_status;

	BOOLEAN fAnyStationInPsm;
	BOOLEAN fAnyStationBadAtheros;	/* Check if any Station is atheros 802.11n Chip.  We need to use RTS/CTS with Atheros 802,.11n chip. */
	BOOLEAN fAnyTxOPForceDisable;	/* Check if it is necessary to disable BE TxOP */
	BOOLEAN fAllStationAsRalink;	/* Check if all stations are ralink-chipset */
#ifdef DOT11_N_SUPPORT
	BOOLEAN fAnyStationIsLegacy;	/* Check if I use legacy rate to transmit to my BSS Station/ */
	BOOLEAN fAnyStationNonGF;	/* Check if any Station can't support GF. */
	BOOLEAN fAnyStation20Only;	/* Check if any Station can't support GF. */
	BOOLEAN fAnyStationMIMOPSDynamic;	/* Check if any Station is MIMO Dynamic */
	BOOLEAN fAnyBASession;	/* Check if there is BA session.  Force turn on RTS/CTS */
	BOOLEAN fAnyStaFortyIntolerant;	/* Check if still has any station set the Intolerant bit on! */
	BOOLEAN fAllStationGainGoodMCS; /* Check if all stations more than MCS threshold */

#ifdef CONFIG_AP_SUPPORT
	BOOLEAN fAnyStationIsHT;	/* Check if there is 11n STA.  Force turn off AP MIMO PS */
#endif /* CONFIG_AP_SUPPORT */
#endif /* DOT11_N_SUPPORT */

#ifdef WAPI_SUPPORT
	BOOLEAN fAnyWapiStation;
#endif /* WAPI_SUPPORT */

	USHORT MsduLifeTime; /* life time for PS packet */
} MAC_TABLE, *PMAC_TABLE;
#ifdef CONFIG_SNIFFER_SUPPORT
#define MONITOR_MODE_OFF  0
#define MONITOR_MODE_REGULAR_RX  1
#define MONITOR_MODE_FULL 2


typedef struct _MONITOR_STRUCT
{
	struct wifi_dev monitor_wdev;
	INT current_monitor_mode;
	BOOLEAN	monitor_initiated;
}MONITOR_STRUCT;
#endif /*CONFIG_SNIFFER_SUPPORT*/


#ifdef CONFIG_AP_SUPPORT
/***************************************************************************
  *	AP WDS related data structures
  **************************************************************************/
#ifdef WDS_SUPPORT
typedef struct _WDS_COUNTER {
	LARGE_INTEGER ReceivedFragmentCount;
	LARGE_INTEGER TransmittedFragmentCount;
	ULONG ReceivedByteCount;
	ULONG TransmittedByteCount;
	ULONG RxErrorCount;
	ULONG TxErrors;
	LARGE_INTEGER MulticastReceivedFrameCount;
	ULONG RxNoBuffer;
} WDS_COUNTER, *PWDS_COUNTER;

typedef struct _WDS_ENTRY {
	BOOLEAN Valid;
	UCHAR Addr[MAC_ADDR_LEN];
	ULONG NoDataIdleCount;
	struct _WDS_ENTRY *pNext;
} WDS_ENTRY, *PWDS_ENTRY;

typedef struct _WDS_TABLE_ENTRY {
	USHORT Size;
	UCHAR WdsAddr[MAC_ADDR_LEN];
	WDS_ENTRY *Hash[HASH_TABLE_SIZE];
	WDS_ENTRY Content[MAX_LEN_OF_MAC_TABLE];
	UCHAR MaxSupportedRate;
	UCHAR CurrTxRate;
	USHORT TxQuality[MAX_LEN_OF_SUPPORTED_RATES];
	USHORT OneSecTxOkCount;
	USHORT OneSecTxRetryOkCount;
	USHORT OneSecTxFailCount;
	ULONG CurrTxRateStableTime;	/* # of second in current TX rate */
	UCHAR TxRateUpPenalty;	/* extra # of second penalty due to last unstable condition */
} WDS_TABLE_ENTRY, *PWDS_TABLE_ENTRY;


typedef struct _RT_802_11_WDS_ENTRY {
	struct wifi_dev wdev;
	UCHAR Valid;
	UCHAR PeerWdsAddr[MAC_ADDR_LEN];
	UCHAR MacTabMatchWCID;	/* ASIC */
	UCHAR KeyIdx;
	CIPHER_KEY WdsKey;

	WDS_COUNTER WdsCounter;
} RT_802_11_WDS_ENTRY, *PRT_802_11_WDS_ENTRY;

typedef struct _WDS_TABLE {
	UCHAR Mode;
	UINT Size;
	RT_802_11_WDS_ENTRY WdsEntry[MAX_WDS_ENTRY];
} WDS_TABLE, *PWDS_TABLE;
#endif /* WDS_SUPPORT */

#ifdef MAC_REPEATER_SUPPORT
typedef struct _REPEATER_CLIENT_ENTRY {
	//BOOLEAN bValid;
	BOOLEAN CliEnable;
	BOOLEAN CliValid;
	BOOLEAN bEthCli;
	UCHAR MatchApCliIdx;
	UCHAR MatchLinkIdx;
	UCHAR MacTabWCID;
	UCHAR CliConnectState; /* 0: disconnect 1: connecting 2: connected */

	ULONG CtrlCurrState;
	ULONG SyncCurrState;
	ULONG AuthCurrState;
	ULONG AssocCurrState;

	RALINK_TIMER_STRUCT ApCliAssocTimer, ApCliAuthTimer;
	RALINK_TIMER_STRUCT ReptCliResetTimer;

	USHORT AuthReqCnt;
	USHORT AssocReqCnt;
	ULONG CliTriggerTime;

	/* For WPA countermeasures */
	ULONG LastMicErrorTime; /* record last MIC error time */
	BOOLEAN bBlockAssoc;	/* Block associate attempt for 60 seconds after counter measure occurred. */

	UCHAR OriginalAddress[MAC_ADDR_LEN];
	UCHAR CurrentAddress[MAC_ADDR_LEN];
	PVOID pAd;
	struct _REPEATER_CLIENT_ENTRY *pNext;
} REPEATER_CLIENT_ENTRY, *PREPEATER_CLIENT_ENTRY;

typedef struct _REPEATER_CLIENT_ENTRY_MAP {
	PREPEATER_CLIENT_ENTRY pReptCliEntry;
	struct _REPEATER_CLIENT_ENTRY_MAP *pNext;
} REPEATER_CLIENT_ENTRY_MAP, *PREPEATER_CLIENT_ENTRY_MAP;

typedef struct _INVAILD_TRIGGER_MAC_ENTRY {
	UCHAR MacAddr[MAC_ADDR_LEN];
	BOOLEAN bInsert;
	struct _INVAILD_TRIGGER_MAC_ENTRY *pNext;
} INVAILD_TRIGGER_MAC_ENTRY, *PINVAILD_TRIGGER_MAC_ENTRY;

typedef struct _REPEATER_CTRL_STRUCT {
	INVAILD_TRIGGER_MAC_ENTRY RepeaterInvaildEntry[32];
	INVAILD_TRIGGER_MAC_ENTRY *ReptInvaildHash[HASH_TABLE_SIZE];
	UCHAR ReptInVaildMacSize;
} REPEATER_CTRL_STRUCT, *PREPEATER_CTRL_STRUCT;
#endif /* MAC_REPEATER_SUPPORT */

/***************************************************************************
  *	AP APCLI related data structures
  **************************************************************************/
typedef struct _APCLI_STRUCT {
	struct wifi_dev wdev;

	BOOLEAN Enable;		/* Set it as 1 if the apcli interface was configured to "1"  or by iwpriv cmd "ApCliEnable" */
	BOOLEAN Valid;		/* Set it as 1 if the apcli interface associated success to remote AP. */

	MLME_AUX MlmeAux;			/* temporary settings used during MLME state machine */

	UCHAR MacTabWCID;	/*WCID value, which point to the entry of ASIC Mac table. */
	UCHAR SsidLen;
	CHAR Ssid[MAX_LEN_OF_SSID];

	UCHAR CfgSsidLen;
	CHAR CfgSsid[MAX_LEN_OF_SSID];
	UCHAR CfgApCliBssid[MAC_ADDR_LEN];

	ULONG ApCliRcvBeaconTime;
	ULONG ApCliLinkUpTime;
	USHORT ApCliBeaconPeriod;

	ULONG CtrlCurrState;
	ULONG SyncCurrState;
	ULONG AuthCurrState;
	ULONG AssocCurrState;
	ULONG WpaPskCurrState;

#ifdef APCLI_AUTO_CONNECT_SUPPORT
		USHORT	ProbeReqCnt;
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
	USHORT AuthReqCnt;
	USHORT AssocReqCnt;

	UCHAR MpduDensity;


	/*
		Security segment
	*/
	/* Add to support different cipher suite for WPA2/WPA mode */
	NDIS_802_11_ENCRYPTION_STATUS GroupCipher;	/* Multicast cipher suite */
	NDIS_802_11_ENCRYPTION_STATUS PairCipher;	/* Unicast cipher suite */
	BOOLEAN bMixCipher;	/* Indicate current Pair & Group use different cipher suites */
	USHORT RsnCapability;

	UCHAR PSK[100];		/* reserve PSK key material */
	UCHAR PSKLen;
	UCHAR PMK[32];		/* WPA PSK mode PMK */
	UCHAR GTK[32];		/* GTK from authenticator */

	/*CIPHER_KEY            PairwiseKey; */
	CIPHER_KEY SharedKey[SHARE_KEY_NUM];

	/* store RSN_IE built by driver */
	UCHAR RSN_IE[MAX_LEN_OF_RSNIE];	/* The content saved here should be convert to little-endian format. */
	UCHAR RSNIE_Len;

	/* For WPA countermeasures */
	ULONG LastMicErrorTime;	/* record last MIC error time */
	BOOLEAN bBlockAssoc;	/* Block associate attempt for 60 seconds after counter measure occurred. */

	/* For WPA-PSK supplicant state */
	UCHAR SNonce[32];	/* SNonce for WPA-PSK */
	UCHAR GNonce[32];	/* GNonce for WPA-PSK from authenticator */

#ifdef WPA_SUPPLICANT_SUPPORT
	WPA_SUPPLICANT_INFO wpa_supplicant_info;

	BOOLEAN	 bScanReqIsFromWebUI;
	BSSID_INFO SavedPMK[PMKID_NO];
	UINT SavedPMKNum; /* Saved PMKID number */
	BOOLEAN bConfigChanged;
	NDIS_802_11_ASSOCIATION_INFORMATION AssocInfo;
	USHORT ReqVarIELen; /* Length of next VIE include EID & Length */
	UCHAR ReqVarIEs[MAX_VIE_LEN]; /* The content saved here should be little-endian format. */
	USHORT ResVarIELen; /* Length of next VIE include EID & Length */
	UCHAR ResVarIEs[MAX_VIE_LEN];
	UCHAR LastSsidLen;               /* the actual ssid length in used */
	CHAR LastSsid[MAX_LEN_OF_SSID]; /* NOT NULL-terminated */
	UCHAR LastBssid[MAC_ADDR_LEN];
	UINT16 LastDeauthReason;
#endif /* WPA_SUPPLICANT_SUPPORT */


	/*
		WPS segment
	*/
#ifdef WSC_AP_SUPPORT
	WSC_CTRL WscControl;
#endif /* WSC_AP_SUPPORT */

	/*
		Transmitting segment
	*/
	UCHAR RxMcsSet[16];

#ifdef P2P_SUPPORT
	BOOLEAN bP2pClient;
#endif /* P2P_SUPPORT */

	PSPOLL_FRAME PsPollFrame;
	HEADER_802_11 NullFrame;

#ifdef MAC_REPEATER_SUPPORT
	REPEATER_CLIENT_ENTRY RepeaterCli[MAX_EXT_MAC_ADDR_SIZE];
	REPEATER_CLIENT_ENTRY_MAP RepeaterCliMap[MAX_EXT_MAC_ADDR_SIZE];
#endif /* MAC_REPEATER_SUPPORT */
} APCLI_STRUCT, *PAPCLI_STRUCT;


typedef struct _AP_ADMIN_CONFIG {
	USHORT CapabilityInfo;
	/* Multiple SSID */
	UCHAR BssidNum;
	UCHAR MacMask;
	BSS_STRUCT MBSSID[HW_BEACON_MAX_NUM];
	ULONG IsolateInterStaTrafficBTNBSSID;
#ifdef CONFIG_SNIFFER_SUPPORT
	UCHAR BssType; /* Infra mode or monitor mode */ 
#endif /*CONFIG_SNIFFER_SUPPORT*/

#ifdef APCLI_SUPPORT
	UCHAR ApCliInfRunned;	/* Number of  ApClient interface which was running. value from 0 to MAX_APCLI_INTERFACE */
	UINT8 ApCliNum;
	BOOLEAN FlgApCliIsUapsdInfoUpdated;
	APCLI_STRUCT ApCliTab[MAX_APCLI_NUM];	/*AP-client */
#ifdef APCLI_AUTO_CONNECT_SUPPORT
	BOOLEAN		ApCliAutoConnectRunning;
	BOOLEAN		ApCliAutoConnectChannelSwitching;
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
#endif /* APCLI_SUPPORT */

#ifdef MAC_REPEATER_SUPPORT
	NDIS_SPIN_LOCK ReptCliEntryLock;
	REPEATER_CLIENT_ENTRY *ReptCliHash[HASH_TABLE_SIZE];
	REPEATER_CLIENT_ENTRY_MAP *ReptMapHash[HASH_TABLE_SIZE];
	UCHAR BridgeAddress[MAC_ADDR_LEN];
	REPEATER_CTRL_STRUCT ReptControl;
#endif /* MAC_REPEATER_SUPPORT */

	/* for wpa */
	RALINK_TIMER_STRUCT CounterMeasureTimer;

	UCHAR CMTimerRunning;
	UCHAR BANClass3Data;
	LARGE_INTEGER aMICFailTime;
	LARGE_INTEGER PrevaMICFailTime;
	ULONG MICFailureCounter;

	RSSI_SAMPLE RssiSample;
	ULONG NumOfAvgRssiSample;

	BOOLEAN bAutoChannelAtBootup;	/* 0: disable, 1: enable */
#ifdef SUPPORT_ACS_ALL_CHANNEL_RANK
    BOOLEAN bAutoChannelScaned;
#endif    
	ChannelSel_Alg AutoChannelAlg;	/* Alg for selecting Channel */
#ifdef AP_SCAN_SUPPORT
	UINT32  ACSCheckTime;           /* Periodic timer to trigger Auto Channel Selection (unit: second) */
	UINT32  ACSCheckCount;          /* if  ACSCheckCount > ACSCheckTime, then do ACS check */
#endif /* AP_SCAN_SUPPORT */
	BOOLEAN bAvoidDfsChannel;	/* 0: disable, 1: enable */
	BOOLEAN bIsolateInterStaTraffic;
	BOOLEAN bHideSsid;

	/* temporary latch for Auto channel selection */
	ULONG ApCnt;		/* max RSSI during Auto Channel Selection period */
	UCHAR AutoChannel_Channel;	/* channel number during Auto Channel Selection */
	UCHAR current_channel_index;	/* current index of channel list */
	UCHAR AutoChannelSkipListNum;	/* number of rejected channel list */
	UCHAR AutoChannelSkipList[10];
	UCHAR DtimCount;	/* 0.. DtimPeriod-1 */
	UCHAR DtimPeriod;	/* default = 3 */
	UCHAR ErpIeContent;
	ULONG LastOLBCDetectTime;
	ULONG LastNoneHTOLBCDetectTime;
	ULONG LastScanTime;	/* Record last scan time for issue BSSID_SCAN_LIST */

#ifdef DOT1X_SUPPORT
	/* dot1x related parameter */
	UINT32 own_ip_addr;
	UINT32 retry_interval;
	UINT32 session_timeout_interval;
	UINT32 quiet_interval;
	UCHAR EAPifname[HW_BEACON_MAX_NUM][IFNAMSIZ];	/* indicate as the binding interface for EAP negotiation. */
	UCHAR EAPifname_len[HW_BEACON_MAX_NUM];
	UCHAR PreAuthifname[HW_BEACON_MAX_NUM][IFNAMSIZ];	/* indicate as the binding interface for WPA2 Pre-authentication. */
	UCHAR PreAuthifname_len[HW_BEACON_MAX_NUM];
#endif /* DOT1X_SUPPORT */

	/* EDCA parameters to be announced to its local BSS */
	EDCA_PARM BssEdcaParm;

	RALINK_TIMER_STRUCT ApQuickResponeForRateUpTimer;
	BOOLEAN ApQuickResponeForRateUpTimerRunning;

#ifdef IDS_SUPPORT
	/* intrusion detection parameter */
	BOOLEAN IdsEnable;
	UINT32 AuthFloodThreshold;	/* Authentication frame flood threshold */
	UINT32 AssocReqFloodThreshold;	/* Association request frame flood threshold */
	UINT32 ReassocReqFloodThreshold;	/* Re-association request frame flood threshold */
	UINT32 ProbeReqFloodThreshold;	/* Probe request frame flood threshold */
	UINT32 DisassocFloodThreshold;	/* Disassociation frame flood threshold */
	UINT32 DeauthFloodThreshold;	/* Deauthentication frame flood threshold */
	UINT32 EapReqFloodThreshold;	/* EAP request frame flood threshold */
	UINT32 DataFloodThreshold;		/* Malicious data frame flood threshold */

	UINT32 RcvdAuthCount;
	UINT32 RcvdAssocReqCount;
	UINT32 RcvdReassocReqCount;
	UINT32 RcvdProbeReqCount;
	UINT32 RcvdDisassocCount;
	UINT32 RcvdDeauthCount;
	UINT32 RcvdEapReqCount;
	UINT32 RcvdMaliciousDataCount;	/* Data Frame DDOS */

	RALINK_TIMER_STRUCT IDSTimer;
	BOOLEAN IDSTimerRunning;
#endif /* IDS_SUPPORT */

	/* Indicate the maximum idle timeout */
	UINT32 StaIdleTimeout;

	ULONG EntryLifeCheck;

#ifdef IGMP_SNOOP_SUPPORT
	BOOLEAN IgmpSnoopEnable;	/* 0: disable, 1: enable. */
#endif /* IGMP_SNOOP_SUPPORT */

#ifdef DOT11R_FT_SUPPORT
	FT_TAB FtTab;
#endif /* DOT11R_FT_SUPPORT */

#ifdef CLIENT_WDS
	NDIS_SPIN_LOCK CliWdsTabLock;
	PCLIWDS_PROXY_ENTRY pCliWdsEntryPool;
	LIST_HEADER CliWdsEntryFreeList;
	LIST_HEADER CliWdsProxyTb[CLIWDS_HASH_TAB_SIZE];
#endif /* CLIENT_WDS */

#ifdef DOT11_N_SUPPORT
#ifdef GREENAP_SUPPORT
	UCHAR GreenAPLevel;
	BOOLEAN bGreenAPEnable;
	BOOLEAN bGreenAPActive;
#endif /* GREENAP_SUPPORT */

#ifdef RT3593
	ULONG MAX_PSDU_LEN;	/* Maximum PSDU length */
#endif /* RT3593 */
#endif /* DOT11_N_SUPPORT */

	UCHAR EntryClientCount;

#ifdef SUPPORT_ACS_ALL_CHANNEL_RANK
    UCHAR ACS_ChannelRankList[MAX_NUM_OF_CHANNELS];
    INT32 ACS_ChannelRankCount;
#endif /* SUPPORT_ACS_ALL_CHANNEL_RANK */

#ifdef MAC_REPEATER_SUPPORT
	BOOLEAN bMACRepeaterEn;
	UCHAR MACRepeaterOuiMode;
	UINT8 EthApCliIdx;
	UCHAR RepeaterCliSize;
#endif /* MAC_REPEATER_SUPPORT */
} AP_ADMIN_CONFIG;

#ifdef IGMP_SNOOP_SUPPORT
typedef enum _IGMP_GROUP_TYPE {
	MODE_IS_INCLUDE = 1,
	MODE_IS_EXCLUDE,
	CHANGE_TO_INCLUDE_MODE,
	CHANGE_TO_EXCLUDE_MODE,
	ALLOW_NEW_SOURCES,
	BLOCK_OLD_SOURCES
} IgmpGroupType;

typedef enum _MULTICAST_FILTER_ENTRY_TYPE {
	MCAT_FILTER_STATIC = 0,
	MCAT_FILTER_DYNAMIC,
} MulticastFilterEntryType;

typedef struct _MEMBER_ENTRY {
	struct _MEMBER_ENTRY *pNext;
	UCHAR Addr[MAC_ADDR_LEN];
/*	USHORT Aid; */
} MEMBER_ENTRY, *PMEMBER_ENTRY;

typedef struct _MULTICAST_FILTER_TABLE_ENTRY {
	BOOLEAN Valid;
	MulticastFilterEntryType type;	/* 0: static, 1: dynamic. */
	UINT lastTime;
	PNET_DEV net_dev;
	UCHAR Addr[MAC_ADDR_LEN];
	LIST_HEADER MemberList;
	struct _MULTICAST_FILTER_TABLE_ENTRY *pNext;
} MULTICAST_FILTER_TABLE_ENTRY, *PMULTICAST_FILTER_TABLE_ENTRY;

typedef struct _MULTICAST_FILTER_TABLE {
	UCHAR Size;
	PMULTICAST_FILTER_TABLE_ENTRY
	  Hash[MAX_LEN_OF_MULTICAST_FILTER_HASH_TABLE];
	MULTICAST_FILTER_TABLE_ENTRY Content[MAX_LEN_OF_MULTICAST_FILTER_TABLE];
	NDIS_SPIN_LOCK MulticastFilterTabLock;
	NDIS_SPIN_LOCK FreeMemberPoolTabLock;
	MEMBER_ENTRY freeMemberPool[FREE_MEMBER_POOL_SIZE];
	LIST_HEADER freeEntryList;
} MULTICAST_FILTER_TABLE, *PMULTICAST_FILTER_TABLE;
#endif /* IGMP_SNOOP_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT

typedef struct _DMSID_TABLE {
	struct _DMSID_TABLE *pNext;
	UCHAR DMSIDs;
	UINT32 DMSID_DestIP;
} DMSID_TABLE, PDMSID_TABLE;
typedef struct _DMS_ENTRY {
	struct _DMS_ENTRY *pNext;
	LIST_HEADER DMSIDList;
	PMAC_TABLE_ENTRY pEntry;
	UCHAR apidx;
} DMS_ENTRY, *PDMS_ENTRY;


#endif /* DOT11V_WNM_SUPPORT */

#ifdef NINTENDO_AP
typedef struct _NINDO_CTRL_BLOCK {

	RT_NINTENDO_TABLE DS_TABLE;

#ifdef CHIP25XX
	spinlock_t NINTENDO_TABLE_Lock;
#else
	NDIS_SPIN_LOCK NINTENDO_TABLE_Lock;
#endif				/* CHIP25XX */

	UCHAR NINTENDO_UP_BUFFER[512];
	UCHAR Local_KeyIdx;
	CIPHER_KEY Local_SharedKey;
	UCHAR Local_bHideSsid;
	UCHAR Local_AuthMode;
	UCHAR Local_WepStatus;
	USHORT Local_CapabilityInfo;
} NINDO_CTRL_BLOCK;
#endif /* NINTENDO_AP */
#ifdef DOT11_N_SUPPORT
#ifdef GREENAP_SUPPORT
typedef enum _RT_GREEN_AP_LEVEL {
	GREENAP_11BGN_STAS = 0,
	GREENAP_ONLY_11BG_STAS,
	GREENAP_WITHOUT_ANY_STAS_CONNECT
} RT_GREEN_AP_LEVEL;
#endif /* DOT11_N_SUPPORT */
#endif /* GREENAP_SUPPORT */

/* ----------- end of AP ---------------------------- */
#endif /* CONFIG_AP_SUPPORT */

#ifdef BLOCK_NET_IF
typedef struct _BLOCK_QUEUE_ENTRY {
	BOOLEAN SwTxQueueBlockFlag;
	LIST_HEADER NetIfList;
} BLOCK_QUEUE_ENTRY, *PBLOCK_QUEUE_ENTRY;
#endif /* BLOCK_NET_IF */

#ifdef MESH_SUPPORT
typedef struct _MESH_STRUCT {
	struct wifi_dev wdev;

	UCHAR HostName[MAX_HOST_NAME_LEN];
	UCHAR OpMode;
	UINT8 TTL;
	BOOLEAN MeshAutoLink;
	BOOLEAN UCGEnable;
	UCHAR MeshChannel;
#ifdef DOT11_N_SUPPORT
	UINT8 ExtChOffset;
#endif				/* DOT11_N_SUPPORT */
	ULONG MeshMaxTxRate;
	ULONG MeshMultiCastAgeOut;

	UCHAR MeshId[MAX_MESH_ID_LEN];
	UCHAR MeshIdLen;

	/*UCHAR                 MKDDID[MAC_ADDR_LEN]; */
	MESH_SECURITY_CAPABILITY_IE LocalMSCIE;

	UINT32 MeshSeq;
	UINT32 MeshPreqID;
	UINT32 MeshOriginatorDsn;

	UINT8 PathProtocolId;	/* 0: mean HWMP. */
	UINT8 PathMetricId;	/* 0: airtime. */
	UINT8 ContgesionCtrlId;
	MESH_CAPABILITY MeshCapability;
	UINT32 CPI;		/* channel precedence indicator. */
	UINT32 NewCPI;		/* channel precedence indicator. */
	PMESH_NEIGHBOR_TAB pMeshNeighborTab;	/* all neighbor MPs list. */
	MESH_LINK MeshLink[MAX_MESH_LINKS];	/* Current established peer link list. */
	UINT8 LinkSize;		/* record how much links establish already. */

	NDIS_SPIN_LOCK MeshEntryTabLock;
	PMESH_ENTRY_TABLE pMeshEntryTab;

	NDIS_SPIN_LOCK MeshRouteTabLock;
	/*MESH_ROUTING_TABLE    MeshRouteTab; */
	PMESH_ROUTING_TABLE pMeshRouteTab;

	UCHAR bMeshPortal;
	NDIS_SPIN_LOCK MeshProxyTabLock;
	PMESH_PROXY_ENTRY_TABLE pMeshProxyTab;

	NDIS_SPIN_LOCK MeshBMPktTabLock;
	PMESH_BMPKTSIG_TAB pBMPktSigTab;

	NDIS_SPIN_LOCK MultipathTabLock;
	PMESH_MULTIPATH_ENTRY pMultipathEntryPool;
	LIST_HEADER MultipathEntryFreeList;

	USHORT CapabilityInfo;

	BOOLEAN EasyMeshSecurity;
	UCHAR WPAPassPhraseKey[64];
	UCHAR WPAPassPhraseKeyLen;
	UCHAR DesiredWepKey[26];
	UCHAR DesiredWepKeyLen;
	UCHAR PMK[32];
	CIPHER_KEY SharedKey;

	/* store RSN_IE built by driver */
	UCHAR RSN_IE[MAX_LEN_OF_RSNIE];	/* The content saved here should be convert to little-endian format. */
	UCHAR RSNIE_Len;
	UCHAR PMKID[LEN_PMKID];	/*PMK-MAName(sender), the identifier of the currently-valid PMK-MA belonging to the key */
	/*hierarchy created by the local MP during a prior Initial MSA Authentiction. */
	UCHAR PMKID_Len;

	UCHAR LocalMpInitialMsaLink;	/* 0 indicates this MP is MKD or this MP doesn't complete its Initial MSA */
	/* 1~4 indicates which MESHLINK is its Initial MSA link. */

	BOOLEAN bInitialMsaDone;	/* indicate that the MP had completed the Initial MSA Authentication */
	BOOLEAN bKeyholderDone;	/* indicate that the MP had completed key holder handshake and became MA */
	BOOLEAN bConnectedToMKD;	/* indicate that the MP has a valid path to the MKD */
	/* and a current security association with the MKD */
	BOOLEAN MeshOnly;	/* 1: means mesh only mode. decline scan request from rausb0. */

	MESH_CTRL_STATE CtrlCurrentState;
	RALINK_TIMER_STRUCT PldTimer, McsTimer;

	BCN_BUF_STRUC bcn_buf;
	UCHAR TimIELocationInBeacon;
} MESH_STRUCT;
#endif /* MESH_SUPPORT */


struct wificonf {
	BOOLEAN bShortGI;
	BOOLEAN bGreenField;
};

typedef struct _RTMP_DEV_INFO_ {
	UCHAR chipName[16];
	RTMP_INF_TYPE infType;
} RTMP_DEV_INFO;

#ifdef DBG_DIAGNOSE
#define MAX_VHT_MCS_SET 	20 /* for 1ss~ 2ss with MCS0~9 */

#define DIAGNOSE_TIME	10	/* 10 sec */

struct dbg_diag_info{
	USHORT TxDataCnt[WMM_NUM_OF_AC];	/* Tx total data count */
	USHORT TxFailCnt;
	USHORT RxDataCnt;	/* Rx Total Data count. */
	USHORT RxCrcErrCnt;

#ifdef DBG_TXQ_DEPTH
	/* TxSwQ length in scale of 0, 1, 2, 3, 4, 5, 6, 7, >=8 */
	USHORT TxSWQueCnt[WMM_NUM_OF_AC][9];
	UINT32 enq_fall_cnt[WMM_NUM_OF_AC];
	UINT32 deq_fail_no_resource_cnt[WMM_NUM_OF_AC];
	UINT32 deq_called;
	UINT32 deq_round;
	UINT32 deq_cnt[9];
#endif /* DBG_TXQ_DEPTH */

#ifdef DBG_TX_RING_DEPTH
	/* TxDesc queue length in scale of 0~14, >=15 */
	USHORT TxDescCnt[WMM_NUM_OF_AC][24];
#endif /* DBG_TX_RING_DEPTH */

#ifdef MT_MAC
#ifdef DBG_PSE_DEPTH
	/* PSE Page buffer depth */
	USHORT pse_pg_cnt[50];
#endif /* DBG_PSE_DEPTH */
#endif /* MT_MAC */

#ifdef DBG_TX_AGG_CNT
	USHORT TxAggCnt;
	USHORT TxNonAggCnt;
	/* TxDMA APMDU Aggregation count in range from 0 to 15, in setp of 1. */
	USHORT TxAMPDUCnt[16];
#endif /* DBG_TX_AGG_CNT */

#ifdef DBG_TX_MCS
	/* TxDate MCS Count in range from 0 to 15, step in 1 */
	USHORT TxMcsCnt_HT[MAX_MCS_SET];
#ifdef DOT11_VHT_AC
	UINT TxMcsCnt_VHT[MAX_VHT_MCS_SET];
#endif /* DOT11_VHT_AC */
#endif /* DBG_TX_MCS */

#ifdef DBG_RX_MCS
	/* Rx HT MCS Count in range from 0 to 15, step in 1 */
	USHORT RxMcsCnt_HT[MAX_MCS_SET];
#ifdef DOT11_VHT_AC
	/* for VHT80MHz only, not calcuate 20/40 MHz packets */
	UINT RxMcsCnt_VHT[MAX_VHT_MCS_SET];
#endif /* DOT11_VHT_AC */
#endif /* DBG_RX_MCS */
};

typedef enum{
    DIAG_COND_ALL = 0,
    DIAG_COND_PSE_DEPTH = 1,
    DIAG_COND_TX_RING_DEPTH = 2,
    DIAG_COND_TXQ_DEPTH = 4,
} DIAG_COND_STATUS;

typedef struct _RtmpDiagStrcut_ {	/* Diagnosis Related element */
	BOOLEAN inited;
	UCHAR wcid;
	UCHAR qIdx;
	UCHAR ArrayStartIdx;
	UCHAR ArrayCurIdx;
    UINT32 diag_cond;

	struct dbg_diag_info diag_info[DIAGNOSE_TIME];
} RtmpDiagStruct;
#endif /* DBG_DIAGNOSE */

#if defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION)
/*
	The number of channels for per-channel Tx power offset
*/
#define NUM_OF_CH_FOR_PER_CH_TX_PWR_OFFSET	14

/* The Tx power control using the internal ALC */
#ifdef RT8592
// TODO: shiang-6590, fix me for this ugly definition!
#define LOOKUP_TB_SIZE		45
#else
#define LOOKUP_TB_SIZE		33
#endif /* RT8592 */

typedef struct _TX_POWER_CONTROL {
	BOOLEAN bInternalTxALC; /* Internal Tx ALC */
	BOOLEAN bExtendedTssiMode; /* The extended TSSI mode (each channel has different Tx power if needed) */
	CHAR PerChTxPwrOffset[NUM_OF_CH_FOR_PER_CH_TX_PWR_OFFSET + 1]; /* Per-channel Tx power offset */
	CHAR idxTxPowerTable; /* The index of the Tx power table for ant0 */
	CHAR idxTxPowerTable2; /* The index of the Tx power table for ant1 */
	CHAR RF_TX_ALC; /* 3390: RF R12[4:0]: Tx0 ALC, 3352: RF R47[4:0]: Tx0 ALC, 5390: RF R49[5:0]: Tx0 ALC */
	CHAR MAC_PowerDelta; /* Tx power control over MAC 0x1314~0x1324 */
	CHAR MAC_PowerDelta2; /* Tx power control for Tx1 */
	CHAR TotalDeltaPower2; /* Tx power control for Tx1 */
#ifdef RTMP_TEMPERATURE_COMPENSATION
	INT LookupTable[IEEE80211_BAND_NUMS][LOOKUP_TB_SIZE];
	INT RefTemp[IEEE80211_BAND_NUMS];
	UCHAR TssiGain[IEEE80211_BAND_NUMS];
	/* Index offset, -7....25. */
	INT LookupTableIndex;
#endif /* RTMP_TEMPERATURE_COMPENSATION */

} TX_POWER_CONTROL, *PTX_POWER_CONTROL;
#endif /* RTMP_INTERNAL_TX_ALC || RTMP_TEMPERATURE_COMPENSATION */

/* */
/* The entry of transmit power control over MAC */
/* */
typedef struct _TX_POWER_CONTROL_OVER_MAC_ENTRY {
	USHORT MACRegisterOffset;	/* MAC register offset */
	ULONG RegisterValue;	/* Register value */
} TX_POWER_CONTROL_OVER_MAC_ENTRY, *PTX_POWER_CONTROL_OVER_MAC_ENTRY;

/* */
/* The maximum registers of transmit power control */
/* */
#define MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS 5
#ifdef RT3593
#undef MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS
#define MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS	15
#endif /* RT3593 */

#ifdef RT2883
#undef MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS
#define MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS	12
#endif /* RT2883 */

#ifdef RT3883
#undef MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS
#define MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS	15
#endif /* RT3883 */

#ifdef MT76x0
#undef MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS
#define MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS	6
#endif /* MT76x0 */

/* */
/* The configuration of the transmit power control over MAC */
/* */
typedef struct _CONFIGURATION_OF_TX_POWER_CONTROL_OVER_MAC {
	UCHAR NumOfEntries;	/* Number of entries */
	TX_POWER_CONTROL_OVER_MAC_ENTRY TxPwrCtrlOverMAC[MAX_TX_PWR_CONTROL_OVER_MAC_REGISTERS];
} CONFIGURATION_OF_TX_POWER_CONTROL_OVER_MAC, *PCONFIGURATION_OF_TX_POWER_CONTROL_OVER_MAC;

/* */
/* The extension of the transmit power control over MAC */
/* */
typedef struct _TX_POWER_CONTROL_EXT_OVER_MAC {
	struct {
		ULONG TxPwrCfg0;	/* MAC 0x1314 */
		ULONG TxPwrCfg0Ext;	/* MAC 0x1390 */
		ULONG TxPwrCfg1;	/* MAC 0x1318 */
		ULONG TxPwrCfg1Ext;	/* MAC 0x1394 */
		ULONG TxPwrCfg2;	/* MAC 0x131C */
		ULONG TxPwrCfg2Ext;	/* MAC 0x1398 */
		ULONG TxPwrCfg3;	/* MAC 0x1320 */
		ULONG TxPwrCfg3Ext;	/* MAC 0x139C */
		ULONG TxPwrCfg4;	/* MAC 0x1324 */
		ULONG TxPwrCfg4Ext;	/* MAC 0x13A0 */
		ULONG TxPwrCfg5;	/* MAC 0x1384 */
		ULONG TxPwrCfg6;	/* MAC 0x1388 */
		ULONG TxPwrCfg7;	/* MAC 0x13D4 */
		ULONG TxPwrCfg8;	/* MAC 0x13D8 */
		ULONG TxPwrCfg9;	/* MAC 0x13DC */
	} BW20Over2Dot4G;

	struct {
		ULONG TxPwrCfg0;	/* MAC 0x1314 */
		ULONG TxPwrCfg0Ext;	/* MAC 0x1390 */
		ULONG TxPwrCfg1;	/* MAC 0x1318 */
		ULONG TxPwrCfg1Ext;	/* MAC 0x1394 */
		ULONG TxPwrCfg2;	/* MAC 0x131C */
		ULONG TxPwrCfg2Ext;	/* MAC 0x1398 */
		ULONG TxPwrCfg3;	/* MAC 0x1320 */
		ULONG TxPwrCfg3Ext;	/* MAC 0x139C */
		ULONG TxPwrCfg4;	/* MAC 0x1324 */
		ULONG TxPwrCfg4Ext;	/* MAC 0x13A0 */
		ULONG TxPwrCfg5;	/* MAC 0x1384 */
		ULONG TxPwrCfg6;	/* MAC 0x1388 */
		ULONG TxPwrCfg7;	/* MAC 0x13D4 */
		ULONG TxPwrCfg8;	/* MAC 0x13D8 */
		ULONG TxPwrCfg9;	/* MAC 0x13DC */
	} BW40Over2Dot4G;

	struct {
		ULONG TxPwrCfg0;	/* MAC 0x1314 */
		ULONG TxPwrCfg0Ext;	/* MAC 0x1390 */
		ULONG TxPwrCfg1;	/* MAC 0x1318 */
		ULONG TxPwrCfg1Ext;	/* MAC 0x1394 */
		ULONG TxPwrCfg2;	/* MAC 0x131C */
		ULONG TxPwrCfg2Ext;	/* MAC 0x1398 */
		ULONG TxPwrCfg3;	/* MAC 0x1320 */
		ULONG TxPwrCfg3Ext;	/* MAC 0x139C */
		ULONG TxPwrCfg4;	/* MAC 0x1324 */
		ULONG TxPwrCfg4Ext;	/* MAC 0x13A0 */
		ULONG TxPwrCfg5;	/* MAC 0x1384 */
		ULONG TxPwrCfg6;	/* MAC 0x1388 */
		ULONG TxPwrCfg7;	/* MAC 0x13D4 */
		ULONG TxPwrCfg8;	/* MAC 0x13D8 */
		ULONG TxPwrCfg9;	/* MAC 0x13DC */
	} BW20Over5G;

	struct {
		ULONG TxPwrCfg0;	/* MAC 0x1314 */
		ULONG TxPwrCfg0Ext;	/* MAC 0x1390 */
		ULONG TxPwrCfg1;	/* MAC 0x1318 */
		ULONG TxPwrCfg1Ext;	/* MAC 0x1394 */
		ULONG TxPwrCfg2;	/* MAC 0x131C */
		ULONG TxPwrCfg2Ext;	/* MAC 0x1398 */
		ULONG TxPwrCfg3;	/* MAC 0x1320 */
		ULONG TxPwrCfg3Ext;	/* MAC 0x139C */
		ULONG TxPwrCfg4;	/* MAC 0x1324 */
		ULONG TxPwrCfg4Ext;	/* MAC 0x13A0 */
		ULONG TxPwrCfg5;	/* MAC 0x1384 */
		ULONG TxPwrCfg6;	/* MAC 0x1388 */
		ULONG TxPwrCfg7;	/* MAC 0x13D4 */
		ULONG TxPwrCfg8;	/* MAC 0x13D8 */
		ULONG TxPwrCfg9;	/* MAC 0x13DC */
	} BW40Over5G;
} TX_POWER_CONTROL_EXT_OVER_MAC, *PTX_POWER_CONTROL_EXT_OVER_MAC;

/* For Wake on Wireless LAN */
#if (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB)) || defined(MT_WOW_SUPPORT)
typedef struct _WOW_CFG_STRUCT {
	BOOLEAN			bEnable;		/* Enable WOW function*/
	BOOLEAN			bWOWFirmware;	/* Enable WOW function, trigger to reload WOW-support firmware */
	BOOLEAN			bInBand;		/* use in-band signal to wakeup system */
	UINT8			nSelectedGPIO;	/* Side band signal to wake up system */
	UINT8			nDelay;			/* Delay number is multiple of 3 secs, and it used to postpone the WOW function */
	UINT32          nHoldTime;      /* GPIO pulse hold time, unit: 1us, 0 means hold forever.*/
	BOOLEAN			bWoWRunning;	/* WOW function is working */
	UINT8			nWakeupInterface; /* PCI:0 USB:1 GPIO:2 */ 
	UINT8			bGPIOHighLow;	/* 0: low to high, 1: high to low */
	UINT8			nKeepAlivePeriod; /* 0:NOT configured, 1~15:wow keep alive */
#ifdef RT_CFG80211_SUPPORT
		UCHAR			PTK[LEN_PTK];		/* Store the PTK for rekey */
		UCHAR			ReplayCounter[LEN_KEY_DESC_REPLAY]; /* Store the replay counter for rekey */
#endif /* RT_CFG80211_SUPPORT */

} WOW_CFG_STRUCT, *PWOW_CFG_STRUCT;

typedef enum {
	WOW_GPIO_LOW_TO_HIGH,
	WOW_GPIO_HIGH_TO_LOW
} WOW_GPIO_HIGH_LOW_T;

typedef enum {
	WOW_PKT_TO_HOST,
	WOW_PKT_TO_ANDES
} WOW_PKT_FLOW_T;

typedef enum {
	WOW_WAKEUP_BY_PCIE,
	WOW_WAKEUP_BY_USB,
	WOW_WAKEUP_BY_GPIO
} WOW_WAKEUP_METHOD_T;

typedef enum {
	WOW_ENABLE = 1,
	WOW_TRAFFIC = 3,
	WOW_WAKEUP = 4
} WOW_FEATURE_T;

typedef enum {
	WOW_MASK_CFG = 1,
	WOW_SEC_CFG,
	WOW_INFRA_CFG,
	WOW_P2P_CFG,
} WOW_CONFIG_T;

enum {
	WOW_MAGIC_PKT,
	WOW_BITMAP,
	WOW_IPV4_TCP_SYNC,
	WOW_IPV6_TCP_SYNC
};

typedef struct NEW_WOW_MASK_CFG_STRUCT {
	UINT32 	Config_Type;
	UINT32 	Function_Enable;
	UINT32 	Detect_Mask;
	UINT32 	Event_Mask;
} NEW_WOW_MASK_CFG_STRUCT, PNEW_WOW_MASK_CFG_STRUCT;

typedef struct NEW_WOW_SEC_CFG_STRUCT {
	UINT32 	Config_Type;
	UINT32 	WPA_Ver;
	UCHAR 	PTK[64];
	UCHAR 	R_COUNTER[8];
	UCHAR 	Key_Id;
	UCHAR 	Cipher_Alg;
	UCHAR 	WCID;
	UCHAR 	Group_Cipher;
} NEW_WOW_SEC_CFG_STRUCT, PNEW_WOW_SEC_CFG_STRUCT;

typedef struct NEW_WOW_INFRA_CFG_STRUCT {
	UINT32 	Config_Type;
	UCHAR 	STA_MAC[6];
	UCHAR 	AP_MAC[6];
	UINT32 	AP_Status;
} NEW_WOW_INFRA_CFG_STRUCT, PNEW_WOW_INFRA_CFG_STRUCT;

typedef struct _NEW_WOW_P2P_CFG_STRUCT {
	UINT32 	Config_Type;
	UCHAR 	GO_MAC[6];
	UCHAR 	CLI_MAC[6];
	UINT32 	P2P_Status;
} NEW_WOW_P2P_CFG_STRUCT, *PNEW_WOW_P2P_CFG_STRUCT;

typedef struct _NEW_WOW_PARAM_STRUCT {
	UINT32 	Parameter;
	UINT32 	Value;
} NEW_WOW_PARAM_STRUCT, *PNEW_WOW_PARAM_STRUCT;
#endif /* (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB)) || defined(MT_WOW_SUPPORT) */

/*
	Packet drop reason code
*/
typedef enum{
	PKT_ATE_ON = 1 << 8,
	PKT_RADAR_ON = 2 << 8,
	PKT_RRM_QUIET = 3 << 8,
	PKT_TX_STOP = 4 <<8,
	PKT_TX_JAM = 5 << 8,

	PKT_NETDEV_DOWN = 6 < 8,
	PKT_NETDEV_NO_MATCH = 7 << 8,
	PKT_NOT_ALLOW_SEND = 8 << 8,

	PKT_INVALID_DST = 9<< 8,
	PKT_INVALID_SRC = 10 << 8,
	PKT_INVALID_PKT_DATA = 11 << 8,
	PKT_INVALID_PKT_LEN = 12 << 8,
	PKT_INVALID_ETH_TYPE = 13 << 8,
	PKT_INVALID_TXBLK_INFO = 14 << 8,
	PKT_INVALID_SW_ENCRYPT = 15 << 8,
	PKT_INVALID_PKT_TYPE = 16 << 8,
	PKT_INVALID_PKT_MIC = 17 << 8,

	PKT_PORT_NOT_SECURE = 18 << 8,
	PKT_TSPEC_NO_MATCH  = 19 << 8,
	PKT_NO_ASSOCED_STA = 20 << 8,
	PKT_INVALID_MAC_ENTRY = 21 << 8,

	PKT_TX_QUE_FULL = 22 << 8,
	PKT_TX_QUE_ADJUST = 23<<8,

	PKT_PS_QUE_TIMEOUT = 24 <<8,
	PKT_PS_QUE_CLEAN = 25 << 8,
	PKT_MCAST_PS_QUE_FULL = 26 << 8,
	PKT_UCAST_PS_QUE_FULL = 27 << 8,

	PKT_RX_EAPOL_SANITY_FAIL = 28 <<8,
	PKT_RX_NOT_TO_KERNEL = 29 << 8,
	PKT_RX_MESH_SIG_FAIL = 30 << 8,
	PKT_APCLI_FAIL = 31 << 8,
	PKT_ZERO_DATA = 32 <<8,
	PKT_SW_DECRYPT_FAIL = 33 << 8,
	PKT_TX_SW_ENC_FAIL = 34 << 8,

	PKT_ACM_FAIL = 35 << 8,
	PKT_IGMP_GRP_FAIL = 36 << 8,
	PKT_MGMT_FAIL = 37 << 8,
	PKT_AMPDU_OUT_ORDER = 38 << 8,
	PKT_UAPSD_EOSP = 39 << 8,
	PKT_UAPSD_Q_FULL = 40 << 8,

	PKT_DRO_REASON_MAX = 41,
}PKT_DROP_REASON;

/* Packet drop Direction code */
typedef enum{
	PKT_TX = 0,
	PKT_RX = 1 << 31,
}PKT_DROP_DIECTION;


#ifdef SMART_ANTENNA
/*
	1. The combination of all possible training patterns =
		txNss : the maxima antennas used for transmission,
					e.g., for 3*3 device, the txNss = 3
		When user try to select the antenna, they shall use following format to do the selection
				iwpriv ra0 set sa_ant=1,5
					=> it means user try to use PinHeader 1 and pinHeader 5 as the tx antenna

	2. The Training sequence used for smart antenna traing
		pTrainSeq: is a sequence used to do antenna selection.

	the time of one round for the antenna selection training:
		totalTime = maxAntTry
*/

#define SA_TX_NSS_MAX_NUM	3
#define SA_DBG_LOG_MAX_CNT	10
#define SA_ENTRY_MAX_NUM 1

#define SA_DEFAULT_TX_CNT	100
#define SA_DEFAULT_TX_NSS		2
#define SA_DEFAULT_MCS_BOUND	23
#define SA_DEFAULT_MSC			5
#define SA_DEFAULT_CHK_PERIOD	200	// 400
#define SA_DEFAULT_ANT_TRIAL	1
#define SA_DEFAULT_RSSI_VAR		8 // 5
#define SA_DEFAULT_RSSI_THRESHOLD	-80

typedef enum _RTMP_SA_OP_MODE_{
	SA_MODE_NONE = 0,
	SA_MODE_MANUAL = 1,
	SA_MODE_ONESHOT = 2,
	SA_MODE_AUTO = 3,
}RTMP_SA_OP_MODE;

typedef enum _RTMP_SA_STA_RULE_{
	SA_STA_BY_DEFAULT = 0, // first in or static assign
	SA_STA_BY_RSSI = 1,
}RTMP_SA_STA_RULE;

typedef struct _RTMP_SA_AGSP_MAP_ {
	UINT8 hdrPin;
	UINT32 regOffset;/* The GPIO address */
	UINT32 gpioBit;	/* the bit field in the regOffset */
}RTMP_SA_AGSP_MAP;


typedef struct _RTMP_SA_TRAIN_LOG_ELEMENT_{
	UINT32 antPattern;
	UINT32 patternOffset;
	UCHAR antWeight;
	UCHAR candWeight;
	ULONG srtTime;
	ULONG endTime;
	UINT32 txMcs;
	UINT32 txCnt;			// txNoRtyCnt + RtyOkCnt + FailCnt
	UINT32 txNoRtyCnt;
	UINT32 txRtyCnt;		// RtyOkCnt + FailCnt
	UINT32 txRtyFailCnt;	// FailCnt
	UINT32 PER;				// (txRtyCnt * 100) / txCnt
	UINT32 rxCnt;
	INT32  sumRSSI[SA_TX_NSS_MAX_NUM];
	UINT32 cntRSSI[SA_TX_NSS_MAX_NUM];
	INT32  avgRSSI[SA_TX_NSS_MAX_NUM];
	INT32  sumSNR[SA_TX_NSS_MAX_NUM];
	UINT32 cntSNR[SA_TX_NSS_MAX_NUM];
	INT32  avgSNR[SA_TX_NSS_MAX_NUM];
	INT32  sumPreSNR[SA_TX_NSS_MAX_NUM];
	UINT32 cntPreSNR[SA_TX_NSS_MAX_NUM];

#ifdef SA_DBG
	UINT32 rssiDist[SA_TX_NSS_MAX_NUM][33];
	UINT32 SNRDist[SA_TX_NSS_MAX_NUM][33];
	UINT32 preSNRDist[SA_TX_NSS_MAX_NUM][33];
	UINT32 txMcsDist[MAX_MCS_SET];			// the txMcs distruction cnt;
	UINT32 rxMcsDist[MAX_MCS_SET];			// the rxMcs distruction cnt;
#endif // SA_DBG //
}RTMP_SA_TRAIN_LOG_ELEMENT;


typedef struct _RTMP_SA_TRAIN_LOG_{
	UINT32 antPattern;
	int lastRnd;
	int firstRnd;
	RTMP_SA_TRAIN_LOG_ELEMENT record[SA_DBG_LOG_MAX_CNT];
}RTMP_SA_TRAIN_LOG;


typedef struct _RTMP_SA_TRAIN_SEQ_{
	UINT32 antPattern;
	RTMP_SA_AGSP_MAP *pAgspEntry;
}RTMP_SA_TRAIN_SEQ;


/*
	For SmartAntenna auto training mode, it's a three-stages loop state machine
	1. Initial stage (first time)
		do fully scan for all antenna patterns
	2. Confirm stage (second time)
		do fully scan for all antenna patterns

		if Initial stage and confirm stage results are the same
			=> goto Monitor stage
		else
			=> goto confirm stage
	3. Monitor stage (third and following)
		Only do RSSI monitoring.

		if | CurrentRSSI - previousRSSI| <= 5
			=> stay in Monitor stage
		else
			=> go to initial stage
*/
typedef enum{
	SA_INVALID_STAGE = 0,
	SA_INIT_STAGE = 1,
	SA_CONFIRM_STAGE = 2,
	SA_MONITOR_STAGE = 3,
}RTMP_SA_TRAIN_STAGE;

#define ANT_WEIGHT_SCAN_ALL 0xf0
#define ANT_WEIGHT_SCAN_AVG 0x70
#define ANT_WEIGHT_SCAN_ONE	0x10


#define ANT_WEIGHT_CAND_HIGH	0x01
#define ANT_WEIGHT_CAND_AVG		0x02
#define ANT_WEIGHT_CAND_LOW		0x03
#define ANT_WEIGHT_CAND_INIT	0xff

#define ANT_SELECT_FIRST		0x01
#define ANT_SELECT_IGNORE_BASE	0x02
#define ANT_SELECT_BASE			0x04


typedef struct _RTMP_SA_TRAINING_PARAM_{
	RTMP_SA_TRAIN_LOG_ELEMENT *pTrainInfo;

	UCHAR macAddr[MAC_ADDR_LEN];
	BOOLEAN bStatic;		// Indicate if this entry assigned by user or by driver itself

	MAC_TABLE_ENTRY *pMacEntry;

	/* Indicate the info */
	RTMP_SA_TRAIN_LOG_ELEMENT antBaseInfo;

	UINT32 curAntPattern;	// indicate current antenna pattern used for transmission
	UINT32 patternOffset;	// Indicate the offset of the antPattern compare to the pSAParam->pTrainSeq;
	RTMP_SA_TRAIN_LOG_ELEMENT *pCurTrainInfo;

	UINT32 canAntPattern;	// The best candidate until now!
	RTMP_SA_TRAIN_LOG_ELEMENT *pCanTrainInfo;


	RTMP_SA_TRAIN_STAGE	trainStage;
	UINT32 ant_init_stage;
	UINT32 ant_confirm_stage;
	ULONG time_to_start;	// in units of system Clk

	UCHAR mcsStableCnt;
	UCHAR trainWeight;
	BOOLEAN bTraining;		// set as TRUE when trianing procedure is on-going
	BOOLEAN bLastRnd;

#ifdef SA_TRAIN_SBS
	BOOLEAN bRoundDone;
#endif // SA_TRAIN_SBS //

#ifdef SA_LUMP_SUM
	UINT32 sumTxCnt;
	UINT32 sumTxRtyCnt;
	UINT32 sumTxFailCnt;
#endif // SA_LUMP_SUM //
}RTMP_SA_TRAINING_PARAM;


typedef struct _SMART_ANTENNA_STRUCT_{
	RTMP_SA_OP_MODE saMode;		// 1 = manually, 2= one shot , 3 =auto learn

	/* Number of antennas used for the transmission in the same time. */
	UCHAR txNss;

	/* Mcs Stable Count, define the requirements for trigger the
		Antenna switch algorithm,
			0~254: valid count.
			255: reserved value
	*/
	UCHAR saMsc;

	/*
		MCS upper bound for SmartAntenna adaptive tunning.
	*/
	UCHAR saMcsBound;


	/* The maximum number of antenna pattern in one testing period */
	UCHAR maxAntTry;

	/* condition for sa training */
	UINT32 trainCond;	// used for method 3

	/* Method for Antenna candidate selection */
	UINT32 candMethod;

	/* the delay time(in seconds) before do the antenna switching training procedures */
	UINT32 trainDelay;

	/* Time period of simple the data info for a specific antenna period. */
	INT32 chkPeriod;

	/* RSSI variance threshod for training procedures */
	UCHAR rssiVar;

	/* threshold value used for select target training entry */
	CHAR rssiThreshold;

	/* condition to check if need to skip the confirm stage when run in auto mode */
	BOOLEAN bSkipConfStage;

	/* indicate if any station associate/disassociate to us and need to re-do the training procedure */
	BOOLEAN bStaChange;

	/* indicate if the Rssi variance is larger than threshold for sa training */
	BOOLEAN bRssiChange;

	/* the antenna header pin and gpio mapping */
	RTMP_SA_AGSP_MAP *agsp;
	INT32 agspCnt;

	/* Training sequence of antenna pattern used to do simpling.
		0x0: indicate the end of the sequence.
	*/
	UINT32 *pTrainSeq;
	RTMP_SA_TRAIN_LOG_ELEMENT *pTrainMem;
	/* Total length of the training sequence, not include the terminator. */
	int trainSeqCnt;

	RALINK_TIMER_STRUCT     saSwitchTimer;

	/* Rule used to select the target Training entry */
	UINT32	candStaRule;

	RTMP_SA_TRAINING_PARAM trainEntry[SA_ENTRY_MAX_NUM];

}SMART_ANTENNA_STRUCT;
#endif // SMART_ANTENNA //


typedef struct _BBP_RESET_CTL
{
#define BBP_RECORD_NUM	49
	REG_PAIR BBPRegDB[BBP_RECORD_NUM];
	BOOLEAN	AsicCheckEn;
} BBP_RESET_CTL, *PBBP_RESET_CTL;


typedef struct _SCAN_CTRL_{
	UCHAR ScanType;
	UCHAR BssType;
	UCHAR Channel;
	UCHAR SsidLen;
	CHAR Ssid[MAX_LEN_OF_SSID];
	UCHAR Bssid[MAC_ADDR_LEN];

#ifdef CONFIG_AP_SUPPORT
	RALINK_TIMER_STRUCT APScanTimer;
#endif /* CONFIG_AP_SUPPORT */
}SCAN_CTRL;


#define TX_SWQ_FIFO_LEN	512
typedef struct tx_swq_fifo{
	UCHAR swq[TX_SWQ_FIFO_LEN]; // value 0 is used to indicate free to insert, value 1~127 used to incidate the WCID entry
	UINT enqIdx;
	UINT deqIdx;
	NDIS_SPIN_LOCK swq_lock;	/* spinlock for swq */
}TX_SWQ_FIFO;

#ifdef RT_CFG80211_SUPPORT
typedef struct _CFG80211_VIF_DEV
{
	struct _CFG80211_VIF_DEV *pNext;
	BOOLEAN isMainDev;
	UINT32 devType;
	PNET_DEV net_dev;
	UCHAR CUR_MAC[MAC_ADDR_LEN];

	/* ProbeReq Frame */
	BOOLEAN Cfg80211RegisterProbeReqFrame;
	CHAR Cfg80211ProbeReqCount;

	/* Action Frame */
	BOOLEAN Cfg80211RegisterActionFrame;
	CHAR Cfg80211ActionCount;
} CFG80211_VIF_DEV, *PCFG80211_VIF_DEV;

typedef struct _CFG80211_VIF_DEV_SET
{
#define MAX_CFG80211_VIF_DEV_NUM  2

	BOOLEAN inUsed;
	UINT32 vifDevNum;
	LIST_HEADER vifDevList;
	BOOLEAN isGoingOn; /* To check any vif in list */
} CFG80211_VIF_DEV_SET;

/* TxMmgt Related */
typedef struct _CFG80211_TX_PACKET
{
	struct _CFG80211_TX_PACKET *pNext;
	UINT32 TxStatusSeq;			  /* TxMgmt Packet ID from sequence */
	UCHAR *pTxStatusBuf;		  /* TxMgmt Packet buffer content */
	UINT32 TxStatusBufLen;		  /* TxMgmt Packet buffer Length */

} CFG80211_TX_PACKET, *PCFG80211_TX_PACKET;

/* CFG80211 Total CTRL Point */
typedef struct _CFG80211_CONTROL
{
	BOOLEAN FlgCfg8021Disable2040Scan; 
	BOOLEAN FlgCfg80211Scanning;   /* Record it When scanReq from wpa_supplicant */
	BOOLEAN FlgCfg80211Connecting; /* Record it When ConnectReq from wpa_supplicant*/
	/* Scan Related */
    UINT32 *pCfg80211ChanList;    /* the channel list from from wpa_supplicant */
    UCHAR Cfg80211ChanListLen;    /* channel list length */
	UCHAR Cfg80211CurChanIndex;   /* current index in channel list when driver in scanning */

	UCHAR *pExtraIe;  /* Carry on Scan action from supplicant */
	UINT   ExtraIeLen;

	UCHAR *pAssocRspIe;  /* Buffer to keep association response IE from AP. Used in informing Linux kernel. */
	UINT32 assocRspIeLen; /* length of the association response IE */

	UCHAR Cfg_pending_Ssid[MAX_LEN_OF_SSID+1]; /* Record the ssid, When ScanTable Full */
   	UCHAR Cfg_pending_SsidLen;

	/* ROC Related */
	RALINK_TIMER_STRUCT Cfg80211RocTimer;
	CMD_RTPRIV_IOCTL_80211_CHAN Cfg80211ChanInfo;
	BOOLEAN Cfg80211RocTimerInit;
	BOOLEAN Cfg80211RocTimerRunning;

	/* Tx_Mmgt Related */
	UINT32 TxStatusSeq;			  /* TxMgmt Packet ID from sequence */
	UCHAR *pTxStatusBuf;		  /* TxMgmt Packet buffer content */
	UINT32 TxStatusBufLen;		  /* TxMgmt Packet buffer Length */
	BOOLEAN TxStatusInUsed;
	BOOLEAN IsNeedTxStatus;
	BOOLEAN TxSAcked;
	RTMP_OS_COMPLETION fw_event_done;
	PNET_DEV TxNdev;
	LIST_HEADER cfg80211TxPacketList;

	/* P2P Releated*/
	UCHAR P2PCurrentAddress[MAC_ADDR_LEN];	  /* User changed MAC address */
	BOOLEAN isCfgDeviceInP2p; 				  /* For BaseRate 6 */
	BOOLEAN IsInListenProgress;				  /* Flag to indicate if is in listen progress*/

	/* MainDevice Info. */
	CFG80211_VIF_DEV cfg80211MainDev;
	
#if defined(RT_CFG80211_P2P_CONCURRENT_DEVICE) || defined(CFG80211_MULTI_STA)
	/* For add_virtual_intf */
	CFG80211_VIF_DEV_SET Cfg80211VifDevSet;
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE || CFG80211_MULTI_STA */

#ifdef CFG80211_MULTI_STA
    PNET_DEV multi_sta_net_dev;
    BOOLEAN flg_cfg_multi_sta_init;
#endif /* CFG80211_MULTI_STA */

#ifdef RT_CFG80211_P2P_SUPPORT 
	BOOLEAN bP2pCliPmEnable;

	BOOLEAN bPreKeepSlient;
	BOOLEAN	bKeepSlient;

	UCHAR MyGOwcid;	
	UCHAR NoAIndex;
	UCHAR CTWindows; /* CTWindows and OppPS parameter field */

	P2PCLIENT_NOA_SCHEDULE GONoASchedule;
	RALINK_TIMER_STRUCT P2pCTWindowTimer;
	RALINK_TIMER_STRUCT P2pSwNoATimer;
	RALINK_TIMER_STRUCT P2pPreAbsenTimer; 

	UCHAR P2pSupRate[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR P2pSupRateLen;
    UCHAR P2pExtRate[MAX_LEN_OF_SUPPORTED_RATES];
    UCHAR P2pExtRateLen;
		
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	/* Dummy P2P Device for ANDROID JB */
	PNET_DEV dummy_p2p_net_dev;
	struct wifi_dev dummy_p2p_wdev;
	BOOLEAN flg_cfg_dummy_p2p_init;
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
	
#ifdef RT_CFG80211_P2P_SINGLE_DEVICE
	ULONG P2POpStatusFlags; /* P2P_CLI_UP / P2P_GO_UP*/
#endif /* RT_CFG80211_P2P_SINGLE_DEVICE */
#endif /* RT_CFG80211_P2P_SUPPORT */	

	/* In AP Mode */
	UINT8 isCfgInApMode;    /* Is any one Device in AP Mode */
	UCHAR *beacon_tail_buf; /* Beacon buf from upper layer */
	UINT32 beacon_tail_len;

	UCHAR *pCfg80211ExtraIeAssocRsp;
	UINT32 Cfg80211ExtraIeAssocRspLen;

	/* for AdHoc Mode */
	UCHAR *BeaconExtraIe;
	UINT  BeaconExtraIeLen;

	/* Mcc Part */
	//BOOLEAN isMccOn;

	/* TODO: need fix it */
	UCHAR Cfg80211_Alpha2[2];
#ifdef P2P_CHANNEL_LIST_SEPARATE	
	CHANNEL_TX_POWER ChannelList[MAX_NUM_OF_CHANNELS];
	UINT	ChannelListNum;
	UINT	CountryRegion;
	UINT	CountryRegionForABand;
#endif /* P2P_CHANNEL_LIST_SEPARATE */ 


} CFG80211_CTRL, *PCFG80211_CTRL;
#endif /* RT_CFG80211_SUPPORT */


typedef struct rtmp_mac_ctrl {
#ifdef MT_MAC
	UINT8 wtbl_entry_cnt[4];
	UINT8 wtbl_entry_size[4];
	UINT32 wtbl_base_addr[4]; // base address for WTBL2/3/4
	UINT32 wtbl_base_fid[4];
	UINT32 page_size;
#endif /* MT_MAC */
}RTMP_MAC_CTRL;

typedef struct rtmp_phy_ctrl{
	UINT8 rf_band_cap;

#ifdef CONFIG_AP_SUPPORT
#ifdef AP_QLOAD_SUPPORT
	UINT8 FlgQloadEnable;	/* 1: any BSS WMM is enabled */
	ULONG QloadUpTimeLast;	/* last up time */
	UINT8 QloadChanUtil;	/* last QBSS Load, unit: us */
	UINT32 QloadChanUtilTotal;	/* current QBSS Load Total */
	UINT8 QloadChanUtilBeaconCnt;	/* 1~100, default: 50 */
	UINT8 QloadChanUtilBeaconInt;	/* 1~100, default: 50 */
	UINT32 QloadLatestChannelBusyTimePri;
	UINT32 QloadLatestChannelBusyTimeSec;

	/*
	   ex: For 100ms beacon interval,
	   if the busy time in last TBTT is smaller than 5ms, QloadBusyCount[0] ++;
	   if the busy time in last TBTT is between 5 and 10ms, QloadBusyCount[1] ++;
	   ......
	   if the busy time in last TBTT is larger than 95ms, QloadBusyCount[19] ++;

	   Command: "iwpriv ra0 qload show".
	 */

/* provide busy time statistics for every TBTT */
#define QLOAD_FUNC_BUSY_TIME_STATS

/* provide busy time alarm mechanism */
/* use the function to avoid to locate in some noise environments */
#define QLOAD_FUNC_BUSY_TIME_ALARM

#ifdef QLOAD_FUNC_BUSY_TIME_STATS
#define QLOAD_BUSY_INTERVALS	20	/* partition TBTT to QLOAD_BUSY_INTERVALS */
	/* for primary channel & secondary channel */
	UINT32 QloadBusyCountPri[QLOAD_BUSY_INTERVALS];
	UINT32 QloadBusyCountSec[QLOAD_BUSY_INTERVALS];
#endif /* QLOAD_FUNC_BUSY_TIME_STATS */

#ifdef QLOAD_FUNC_BUSY_TIME_ALARM
#define QLOAD_DOES_ALARM_OCCUR(pAd)	(pAd->phy_ctrl.FlgQloadAlarmIsSuspended == TRUE)
#define QLOAD_ALARM_EVER_OCCUR(pAd) (pAd->phy_ctrl.QloadAlarmNumber > 0)
	BOOLEAN FlgQloadAlarmIsSuspended;	/* 1: suspend */

	UINT8 QloadAlarmBusyTimeThreshold;	/* unit: 1/100 */
	UINT8 QloadAlarmBusyNumThreshold;	/* unit: 1 */
	UINT8 QloadAlarmBusyNum;
	UINT8 QloadAlarmDuration;	/* unit: TBTT */

	UINT32 QloadAlarmNumber;	/* total alarm times */
	BOOLEAN FlgQloadAlarm;	/* 1: alarm occurs */

	/* speed up use */
	UINT32 QloadTimePeriodLast;
	UINT32 QloadBusyTimeThreshold;
#else

#define QLOAD_DOES_ALARM_OCCUR(pAd)	0
#endif /* QLOAD_FUNC_BUSY_TIME_ALARM */

#endif /* AP_QLOAD_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
}RTMP_PHY_CTRL;


#ifdef RTMP_SDIO_SUPPORT
/* Tc Resource index */
typedef enum _ENUM_TRAFFIC_CLASS_INDEX_T {
    /*First HW queue*/
    TC0_INDEX = 0,   /* HIF TX: AC0 packets */
    TC1_INDEX,       /* HIF TX: AC1 packets */
    TC2_INDEX,       /* HIF TX: AC2 packets */
    TC3_INDEX,       /* HIF TX: AC3 packets */
    TC4_INDEX,       /* HIF TX: CPU packets */
    TC5_INDEX,       /* HIF TX: AC4 packets */
    /* Second HW queue */
    TC6_INDEX,       /* HIF TX: AC10 packets */
    TC7_INDEX,       /* HIF TX: AC11 packets */
    TC8_INDEX,       /* HIF TX: AC12 packets */
    TC9_INDEX,       /* HIF TX: AC13 packets */
    TC10_INDEX,      /* HIF TX: AC14 packets */
    TC_NUM           /* Maximum number of Traffic Classes. */
} ENUM_TRAFFIC_CLASS_INDEX_T;

typedef struct _TX_TCQ_STATUS_T {
    UINT16                  au2TxDonePageCount[TC_NUM];
    UINT16                  u2AvailiablePageCount;
    UINT8                   ucNextTcIdx;
    UINT16                  au2FreePageCount[TC_NUM];
    UINT16                  au2MaxNumOfPage[TC_NUM];
    UINT16                  au2FreeBufferCount[TC_NUM];
    UINT16                  au2MaxNumOfBuffer[TC_NUM];
} TX_TCQ_STATUS_T, *P_TX_TCQ_STATUS_T;


typedef struct _TX_CTRL_T {
    UINT32                 u4PageSize;
    UINT32                 u4TotalPageNum;
    UINT32                 u4TotalTxRsvPageNum;
/* Elements below is classified according to TC (Traffic Class) value. */
    TX_TCQ_STATUS_T         rTc;
    PUINT8                 pucTxCoalescingBufPtr;
} TX_CTRL_T, *P_TX_CTRL_T;
#endif 

#ifdef SMART_CARRIER_SENSE_SUPPORT
typedef struct _SMART_CARRIER_SENSE_CTRL{
	BOOLEAN		SCSEnable;	/* 0:Disable, 1:Enable */
	UINT8		SCSStatus; /* 0: Normal, 1:Low_gain */
	CHAR		SCSMinRssi;
	UINT32		CR_AGC_0_default;
	UINT32		CR_AGC_3_default;
	UINT8		EDCCA_Status;
	UINT32			PdCount;
	UINT32			MdrdyCount;
	UINT32			FalseCCA;
	CHAR			CurrSensitivity;
	CHAR			AdjustSensitivity;
	UINT32			RtsCount;
	UINT32			RtsRtyCount;
	CHAR			RSSIBoundary;
	UINT8			SCSMinRssiTolerance;
	UINT32			SCSTrafficThreshold;
	UINT16			FalseCcaUpBond;
	UINT16			FalseCcaLowBond;
	CHAR			FixedRssiBond;
	BOOLEAN			ForceMode; /*This mode only consider False CCA not care RTS PER.*/

} SMART_CARRIER_SENSE_CTRL;
#endif /* SMART_CARRIER_SENSE_SUPPORT */
/*
	The miniport adapter structure
*/
struct _RTMP_ADAPTER {
	VOID *OS_Cookie;	/* save specific structure relative to OS */
	PNET_DEV net_dev;

#ifdef RTMP_MAC_PCI
/*****************************************************************************************/
/*      PCI related parameters                                                           								  */
/*****************************************************************************************/
#if 0 /*We should use pciConfig if we really start to merege the code into RT2860 source tree. */
	INF_PCI_CONFIG pciConfig;
#else
	PUCHAR CSRBaseAddress;	/* PCI MMIO Base Address, all access will use */
	unsigned int irq_num;
#endif

#ifdef PCI_MSI_SUPPORT
	BOOLEAN HaveMsi;
#endif /* PCI_MSI_SUPPORT */

	USHORT RLnkCtrlConfiguration;
	USHORT RLnkCtrlOffset;
	USHORT HostLnkCtrlConfiguration;
	USHORT HostLnkCtrlOffset;
	USHORT PCIePowerSaveLevel;
#ifdef CONFIG_STA_SUPPORT
	USHORT LnkCtrlBitMask;
	ULONG Rt3xxHostLinkCtrl;	/* USed for 3090F chip */
	ULONG Rt3xxRalinkLinkCtrl;	/* USed for 3090F chip */
	ULONG HostVendor;
#endif /* CONFIG_STA_SUPPORT */
	USHORT DeviceID;	/* Read from PCI config */
	ULONG AccessBBPFailCount;
	BOOLEAN bPCIclkOff;	/* flag that indicate if the PICE power status in Configuration SPace.. */
	BOOLEAN bPCIclkOffDisableTx;

	BOOLEAN brt30xxBanMcuCmd;	/* when = 0xff means all commands are ok to set . */
	BOOLEAN b3090ESpecialChip;	/* 3090E special chip that write EEPROM 0x24=0x9280. */
	/* ULONG CheckDmaBusyCount;  // Check Interrupt Status Register Count. */

	UINT32 int_enable_reg;
	UINT32 int_disable_mask;
	UINT32 int_pending;

	RTMP_DMABUF TxBufSpace[NUM_OF_TX_RING];	/* Shared memory of all 1st pre-allocated TxBuf associated with each TXD */
	RTMP_DMABUF RxDescRing[2];	/* Shared memory for RX descriptors */
	RTMP_DMABUF TxDescRing[NUM_OF_TX_RING];	/* Shared memory for Tx descriptors */

	RTMP_RX_RING RxRing[NUM_OF_RX_RING];
	NDIS_SPIN_LOCK RxRingLock[NUM_OF_RX_RING];	/* Rx Ring spinlock */
	NDIS_SPIN_LOCK LockInterrupt;
	NDIS_SPIN_LOCK tssi_lock;
#endif /* RTMP_MAC_PCI */

	RTMP_TX_RING TxRing[NUM_OF_TX_RING];	/* AC0~3 + HCCA */

	NDIS_SPIN_LOCK irq_lock;
	RTMP_OS_SEM e2p_read_lock;

	/*======Cmd Thread in PCI/RBUS/USB */
	CmdQ CmdQ;
	NDIS_SPIN_LOCK CmdQLock;	/* CmdQLock spinlock */
	RTMP_OS_TASK cmdQTask;
    RTMP_OS_SEM AutoRateLock;

#ifdef DOT11_N_SUPPORT
	BA_TABLE BATable;
	NDIS_SPIN_LOCK BATabLock;
	RALINK_TIMER_STRUCT RECBATimer;
#endif /* DOT11_N_SUPPORT */

#ifdef RTMP_MAC_USB
/**********************************************************/
/*      USB related parameters                                                         */
/**********************************************************/
#if 0 /*We should use usbConfig if we really start to merege the code into RT2860 source tree. */
	INF_USB_CONFIG usbConfig;
#else
/*	struct usb_config_descriptor *config; */
	VOID *config;

	UINT NumberOfPipes;
	USHORT BulkOutMaxPacketSize;
	USHORT BulkInMaxPacketSize;
	UINT8 BulkOutEpAddr[6];
	UINT8 BulkInEpAddr[2];
#endif /*0*/

	/*======Control Flags */
	ULONG BulkFlags;
	BOOLEAN bUsbTxBulkAggre;	/* Flags for bulk out data priority */

	/*======Semaphores (event) */
	RTMP_OS_SEM UsbVendorReq_semaphore;
	RTMP_OS_SEM reg_atomic;
	RTMP_OS_SEM hw_atomic;
	RTMP_OS_SEM cal_atomic;
	RTMP_OS_SEM tssi_lock;
#if defined(CONFIG_MULTI_CHANNEL) || defined(DOT11Z_TDLS_SUPPORT)
	RTMP_OS_SEM MultiChannelLock;
#endif /* defined(CONFIG_MULTI_CHANNEL) || defined(DOT11Z_TDLS_SUPPORT) */
	PVOID UsbVendorReqBuf;
/*	wait_queue_head_t	 *wait; */
	VOID *wait;

#ifdef CONFIG_ATE
	/* lock for ATE */
	NDIS_SPIN_LOCK GenericLock;	/* ATE Tx/Rx generic spinlock */
#endif /* CONFIG_ATE */

#endif /* RTMP_MAC_USB */

#ifdef RTMP_SDIO_SUPPORT
	UINT32 BlockSize;
	NDIS_SPIN_LOCK SdioWorkTaskLock;
	DL_LIST SdioWorkTaskList;
	NDIS_SPIN_LOCK IntStatusLock;
	NDIS_SPIN_LOCK TcCountLock;
	UINT32 IntStatus;
	PNDIS_PACKET SDIORxPacket;
	NDIS_SPIN_LOCK SDIOTxPacketListLock[NUM_OF_TX_RING];
	DL_LIST SDIOTxPacketList[NUM_OF_TX_RING];
    TX_CTRL_T rTxCtrl;
#endif

#if 0
/*****************************************************************************************/
/*      RBUS related parameters                                                           								  */
/*****************************************************************************************/
#ifdef RTMP_RBUS_SUPPORT
	/* resource for software backlog queues */
	NDIS_SPIN_LOCK page_lock;	/* for nat speedup by bruce */
#endif /* RTMP_RBUS_SUPPORT */
#else
	/* resource for software backlog queues */
	NDIS_SPIN_LOCK page_lock;	/* for nat speedup by bruce */
#endif

/*********************************************************/
/*      Both PCI/USB related parameters                                         */
/*********************************************************/
	/*RTMP_DEV_INFO                 chipInfo; */
	RTMP_INF_TYPE infType;
	UCHAR			AntMode;

/*********************************************************/
/*      Driver Mgmt related parameters                                            */
/*********************************************************/
	/* OP mode: either AP or STA */
	UCHAR OpMode;		/* OPMODE_STA, OPMODE_AP */

	struct wifi_dev *wdev_list[WDEV_NUM_MAX];

	RTMP_OS_TASK mlmeTask;
#ifdef RTMP_TIMER_TASK_SUPPORT
	/* If you want use timer task to handle the timer related jobs, enable this. */
	RTMP_TIMER_TASK_QUEUE TimerQ;
	NDIS_SPIN_LOCK TimerQLock;
	RTMP_OS_TASK timerTask;
#endif /* RTMP_TIMER_TASK_SUPPORT */

/*********************************************************/
/*      Tx related parameters                                                           */
/*********************************************************/
	BOOLEAN DeQueueRunning[NUM_OF_TX_RING];	/* for ensuring RTUSBDeQueuePacket get call once */
	NDIS_SPIN_LOCK DeQueueLock[NUM_OF_TX_RING];

#ifdef RTMP_MAC_USB
#ifdef MT_MAC
	NDIS_SPIN_LOCK BulkOutWmm1Lock[NUM_OF_WMM1_TX_RING];	/* BulkOut spinlock for WMM1 ACs */
	HT_TX_CONTEXT TxContextWmm1[NUM_OF_WMM1_TX_RING];
	NDIS_SPIN_LOCK TxContextQueueWmm1Lock[NUM_OF_WMM1_TX_RING];	/* TxContextQueue WMM1 spinlock */
	UCHAR NextBulkOutWmm1Index[NUM_OF_WMM1_TX_RING];	/* only used for WMM1 EDCA bulkout pipe */
	BOOLEAN BulkOutWmm1Pending[NUM_OF_WMM1_TX_RING];	/* used for total WMM1 bulkout pipe */
	ULONG watchDogWmm1TxPendingCnt[NUM_OF_WMM1_TX_RING];
#endif /* MT_MAC */

	/* Data related context and AC specified, 4 AC supported */
	NDIS_SPIN_LOCK BulkOutLock[6];	/* BulkOut spinlock for 4 ACs */
	NDIS_SPIN_LOCK MLMEBulkOutLock;	/* MLME BulkOut lock */

	HT_TX_CONTEXT TxContext[NUM_OF_TX_RING];
#ifdef USB_IOT_WORKAROUND	
	HT_TX_CONTEXT TxContext2[NUM_OF_TX_RING];		
#endif

	NDIS_SPIN_LOCK TxContextQueueLock[NUM_OF_TX_RING];	/* TxContextQueue spinlock */

	/* 4 sets of Bulk Out index and pending flag */
	/*
	   array size of NextBulkOutIndex must be larger than or equal to 5;
	   Or BulkOutPending[0] will be overwrited in NICInitTransmit().
	 */
	UCHAR NextBulkOutIndex[NUM_OF_TX_RING];	/* only used for 4 EDCA bulkout pipe */

	BOOLEAN BulkOutPending[6];	/* used for total 6 bulkout pipe */
	UCHAR bulkResetPipeid;
	BOOLEAN MgmtBulkPending;
	ULONG bulkResetReq[6];
#ifdef INF_AMAZON_SE
	ULONG BulkOutDataSizeCount[NUM_OF_TX_RING];
	BOOLEAN BulkOutDataFlag[NUM_OF_TX_RING];
	ULONG BulkOutDataSizeLimit[NUM_OF_TX_RING];
	UCHAR RunningQueueNoCount;
	UCHAR LastRunningQueueNo;
#endif /* #ifdef INF_AMAZON_SE */
#endif /* RTMP_MAC_USB */

#ifdef CONFIG_STA_SUPPORT
	USHORT CountDowntoPsm;
#endif /* CONFIG_STA_SUPPORT */

	/* resource for software backlog queues */
	QUEUE_HEADER TxSwQueue[NUM_OF_TX_RING];	/* 4 AC + 1 HCCA */
	NDIS_SPIN_LOCK TxSwQueueLock[NUM_OF_TX_RING];	/* TxSwQueue spinlock */
#if defined(MT_MAC) && defined(IP_ASSEMBLY)
	DL_LIST assebQueue[NUM_OF_TX_RING];
#endif

	/* Maximum allowed tx software Queue length */
	UINT TxSwQMaxLen;
	struct tx_swq_fifo tx_swq[NUM_OF_TX_RING];
	/*psTokenQueue is for PS station can keep packets status and update to tx_swq	*/
	DL_LIST  psTokenQueue;

#ifdef MT_PS
	struct tx_swq_fifo apps_cr_q;
	USHORT apps_last_wcid;
#endif 

	RTMP_DMABUF MgmtDescRing;	/* Shared memory for MGMT descriptors */
	RTMP_MGMT_RING MgmtRing;
	NDIS_SPIN_LOCK MgmtRingLock;	/* Prio Ring spinlock */

#ifdef MT_MAC
	RTMP_DMABUF BcnDescRing;	/* Shared memory for Beacon descriptors */
	RTMP_BCN_RING BcnRing;
	NDIS_SPIN_LOCK BcnRingLock;	/* Beacon Ring spinlock */
#endif

#if defined(RTMP_PCI_SUPPORT) || defined(RTMP_RBUS_SUPPORT)
#ifdef CONFIG_ANDES_SUPPORT
	RTMP_DMABUF CtrlDescRing;	/* Shared memory for CTRL descriptors */
	RTMP_CTRL_RING CtrlRing;
	NDIS_SPIN_LOCK CtrlRingLock;	/* Ctrl Ring spinlock */
#endif /* CONFIG_ANDES_SUPPORT */
	//NDIS_SPIN_LOCK BcnRingLock;	/* Beacon Ring spinlock */

#ifdef MT_MAC
	RTMP_DMABUF TxBmcBufSpace;	/* Shared memory of all 1st pre-allocated TxBuf associated with each TXD */
	RTMP_DMABUF TxBmcDescRing;	/* Shared memory for Tx descriptors */
	RTMP_TX_RING TxBmcRing;		/* BMC */
#endif /* MT_MAC */

#endif /* RTMP_PCI_SUPPORT */

	UCHAR LastMCUCmd;

/*********************************************************/
/*      Rx related parameters                                                          */
/*********************************************************/

#ifdef RTMP_MAC_PCI
	// TODO: shiang, check the purpose of following lock "McuCmdLock"
	NDIS_SPIN_LOCK McuCmdLock;	/*MCU Command Queue spinlock for RT3090/3592/3390/3593/5390/5392/5592/3290 */
#endif /* RTMP_MAC_PCI */

#ifdef RTMP_MAC_USB
	RX_CONTEXT RxContext[RX_RING_SIZE];	/* 1 for redundant multiple IRP bulk in. */
	UINT8 RxBulkInReadIndex;
	NDIS_SPIN_LOCK BulkInLock;	/* BulkIn spinlock for 4 ACs */
	NDIS_SPIN_LOCK CmdRspLock;
	NDIS_SPIN_LOCK RxFreeQLock;
	DL_LIST RxFreeQ;
	NDIS_SPIN_LOCK RxProcessingQLock;
	DL_LIST RxProcessingQ;
	NDIS_SPIN_LOCK RxBulkInQLock;
	DL_LIST RxBulkInQ;

	ULONG TransferBufferLength;	/* current length of the packet buffer */

	CMD_RSP_CONTEXT CmdRspEventContext;
#endif /* RTMP_MAC_USB */

	/* RX re-assembly buffer for fragmentation */
	FRAGMENT_FRAME FragFrame;	/* Frame storage for fragment frame */

#ifdef MT_MAC
	TXS_CTL TxSCtl;
#endif

/***********************************************************/
/*      ASIC related parameters                                                          */
/***********************************************************/
	RTMP_CHIP_OP chipOps;
	RTMP_CHIP_CAP chipCap;
	struct phy_ops *phy_op;
	struct hw_setting hw_cfg;

	UINT32 MACVersion;	/* MAC version. Record rt2860C(0x28600100) or rt2860D (0x28600101).. */
	UINT32 ChipID;
	UINT32 HWVersion;
	UINT32 FWVersion;
	UINT16 ee_chipId; 		/* Chip version. Read from EEPROM 0x00 to identify RT5390H */
	INT dev_idx;

#ifdef MT_MAC
	struct rtmp_mac_ctrl mac_ctrl;
	USHORT rx_pspoll_filter;
#endif /* MT_MAC */
	BOOLEAN iwpriv_command;

	/* --------------------------- */
	/* E2PROM                                     */
	/* --------------------------- */
	enum EEPROM_STORAGE_TYPE eeprom_type;

	ULONG EepromVersion;	/* byte 0: version, byte 1: revision, byte 2~3: unused */
	ULONG FirmwareVersion;	/* byte 0: Minor version, byte 1: Major version, otherwise unused. */
	USHORT EEPROMDefaultValue[NUM_EEPROM_BBP_PARMS];
	UCHAR EEPROMAddressNum;	/* 93c46=6  93c66=8 */
	BOOLEAN EepromAccess;
	UCHAR EFuseTag;

#ifdef RTMP_EFUSE_SUPPORT
	BOOLEAN bUseEfuse;
#endif /* RTMP_EFUSE_SUPPORT */

	UCHAR EEPROMImage[MAX_EEPROM_BUFFER_SIZE];
	UCHAR E2pAccessMode; /* Used to identify flash, efuse, eeprom or bin from start-up */
	UCHAR e2pCurMode;

#ifdef RTMP_FLASH_SUPPORT
	UCHAR *eebuf;
	UINT32 flash_offset;
#endif /* RTMP_FLASH_SUPPORT */

	EEPROM_ANTENNA_STRUC Antenna;	/* Since ANtenna definition is different for a & g. We need to save it for future reference. */
	EEPROM_NIC_CONFIG2_STRUC NicConfig2;
#if defined(BT_COEXISTENCE_SUPPORT) || defined(RT3290) || defined(RT8592)
	EEPROM_NIC_CONFIG3_STRUC NicConfig3;
#endif /* defined(BT_COEXISTENCE_SUPPORT) || defined(RT3290) || defined(RT8592) */

	/* --------------------------- */
	/* BBP Control                               */
	/* --------------------------- */
#if defined(RTMP_BBP) || defined(CONFIG_ATE)
	// TODO: shiang-6590, remove "defined(CONFIG_ATE)" after ATE has fully re-organized!
	UCHAR BbpWriteLatch[MAX_BBP_ID + 1];	/* record last BBP register value written via BBP_IO_WRITE/BBP_IO_WRITE_VY_REG_ID */
	UCHAR Bbp94;
#endif /* defined(RTMP_BBP) || defined(CONFIG_ATE) */

	CHAR BbpRssiToDbmDelta;	/* change from UCHAR to CHAR for high power */
	BBP_R66_TUNING BbpTuning;

	/* ---------------------------- */
	/* RFIC control                                 */
	/* ---------------------------- */
	struct rtmp_phy_ctrl phy_ctrl;

	UCHAR RfIcType;		/* RFIC_xxx */
	UCHAR RfFreqOffset;	/* Frequency offset for channel switching */

	RTMP_RF_REGS LatchRfRegs;	/* latch th latest RF programming value since RF IC doesn't support READ */

#ifdef RTMP_RBUS_SUPPORT
	UCHAR RfFreqDelta;	/* Frequency Delta */
#endif /* RTMP_RBUS_SUPPORT */
#ifdef RTMP_MAC_PCI
	UCHAR ShowRf;		/* Show RF register write for 2880 */
#endif /* RTMP_MAC_PCI */

	/* This soft Rx Antenna Diversity mechanism is used only when user set */
	/* RX Antenna = DIVERSITY ON */
	SOFT_RX_ANT_DIVERSITY RxAnt;

	/* ---------------------------- */
	/* TxPower control                           */
	/* ---------------------------- */
	CHANNEL_TX_POWER TxPower[MAX_NUM_OF_CHANNELS];	/* Store Tx power value for all channels. */
	CHANNEL_TX_POWER ChannelList[MAX_NUM_OF_CHANNELS];	/* list all supported channels for site survey */

#ifdef RT3883
	BOOLEAN bTxPwrRangeExt;
#endif /* RT3883 */

#if 0
	CHANNEL_11J_TX_POWER TxPower11J[MAX_NUM_OF_11JCHANNELS];	/* 802.11j channel and bw */
#endif /* Unused */

	UCHAR ChannelListNum;	/* number of channel in ChannelList[] */
	BOOLEAN BbpForCCK;
	ULONG Tx20MPwrCfgABand[MAX_TXPOWER_ARRAY_SIZE];
	ULONG Tx20MPwrCfgGBand[MAX_TXPOWER_ARRAY_SIZE];
	ULONG Tx40MPwrCfgABand[MAX_TXPOWER_ARRAY_SIZE];
	ULONG Tx40MPwrCfgGBand[MAX_TXPOWER_ARRAY_SIZE];
#ifdef DOT11_VHT_AC
	ULONG Tx80MPwrCfgABand[MAX_TXPOWER_ARRAY_SIZE]; // Per-rate Tx power control for VHT BW80 (5GHz only)
	BOOLEAN force_vht_op_mode;
#endif /* DOT11_VHT_AC */

#ifdef RT3593
	TX_POWER_CONTROL_EXT_OVER_MAC TxPwrCtrlExtOverMAC;	/* The extension of the transmit power control over MAC */
#endif /* RT3593 */

#ifdef MT7601
	UINT32 TxCCKPwrCfg;
#endif /* MT7601 */

	BOOLEAN bAutoTxAgcA;	/* Enable driver auto Tx Agc control */
	UCHAR TssiRefA;		/* Store Tssi reference value as 25 temperature. */
	UCHAR TssiPlusBoundaryA[2][8];	/* Tssi boundary for increase Tx power to compensate. [Group][Boundary Index]*/
	UCHAR TssiMinusBoundaryA[2][8];	/* Tssi boundary for decrease Tx power to compensate. [Group][Boundary Index]*/
	UCHAR TxAgcStepA;	/* Store Tx TSSI delta increment / decrement value */
	CHAR TxAgcCompensateA;	/* Store the compensation (TxAgcStep * (idx-1)) */
	CHAR TssiCalibratedOffset;	/* reference temperature(e2p[D1h]) */
	UCHAR ChBndryIdx;			/* 5G Channel Group Boundary Index for Temperature Compensation */

	BOOLEAN bAutoTxAgcG;	/* Enable driver auto Tx Agc control */
	UCHAR TssiRefG;		/* Store Tssi reference value as 25 temperature. */
	UCHAR TssiPlusBoundaryG[8];	/* Tssi boundary for increase Tx power to compensate. */
	UCHAR TssiMinusBoundaryG[8];	/* Tssi boundary for decrease Tx power to compensate. */
	UCHAR TxAgcStepG;	/* Store Tx TSSI delta increment / decrement value */
	CHAR TxAgcCompensateG;	/* Store the compensation (TxAgcStep * (idx-1)) */
	CHAR mp_delta_pwr;			 /* calculated by MP ATE temperature */
#if defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION)
	TX_POWER_CONTROL TxPowerCtrl;	/* The Tx power control using the internal ALC */
	CHAR curr_temp;
	CHAR rx_temp_base[2];	/* initial VGA value for chain 0/1,  used for base of rx temp compensation power base */
/*	CHAR CurrTemperature; */
	CHAR DeltaPwrBeforeTempComp;
	CHAR LastTempCompDeltaPwr;
#endif /* RTMP_INTERNAL_TX_ALC || RTMP_TEMPERATURE_COMPENSATION */

#ifdef RT6352
	UCHAR rf_pa_mode_over_cck[4];
	UCHAR rf_pa_mode_over_ofdm[8];
	UCHAR rf_pa_mode_over_ht[16];
#endif /* RT6352 */

#ifdef THERMAL_PROTECT_SUPPORT
	BOOLEAN switch_tx_stream; /* determine switch action */
	BOOLEAN force_one_tx_stream; /* for rate adaptation */
	BOOLEAN fgThermalProtectToggle;
	INT32 last_thermal_pro_temp;
	INT32 thermal_pro_high_criteria;
	BOOLEAN thermal_pro_high_en;
	INT32 thermal_pro_low_criteria;
	BOOLEAN thermal_pro_low_en;
#ifdef MT76x0
	UINT32 default_edca_ac0;
#endif /* MT76x0 */
#endif /* THERMAL_PROTECT_SUPPORT */

#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
	FREQUENCY_CALIBRATION_CONTROL FreqCalibrationCtrl;	/* The frequency calibration control */
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */

	signed char BGRssiOffset[3]; /* Store B/G RSSI #0/1/2 Offset value on EEPROM 0x46h */
	signed char ARssiOffset[3]; /* Store A RSSI 0/1/2 Offset value on EEPROM 0x4Ah */

	CHAR BLNAGain;		/* Store B/G external LNA#0 value on EEPROM 0x44h */
	CHAR ALNAGain0;		/* Store A external LNA#0 value for ch36~64 */
	CHAR ALNAGain1;		/* Store A external LNA#1 value for ch100~128 */
	CHAR ALNAGain2;		/* Store A external LNA#2 value for ch132~165 */
#if defined(RT35xx) || defined(RT3593)
	/* for 3572 */
	UCHAR Bbp25;
	UCHAR Bbp26;
#endif /* defined(RT35xx) || defined(RT3593) */
	UCHAR TxMixerGain24G;	/* Tx mixer gain value from EEPROM to improve Tx EVM / Tx DAC, 2.4G */
	UCHAR TxMixerGain5G;

#if defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) || defined(RT5592)
	USHORT	SameRxByteCount;
	UCHAR	BbpResetCount;
	UCHAR				BbpResetFlag;
	UCHAR				BbpResetFlagCount;
	UCHAR				BbpResetFlagCountVale;
	BBP_RESET_CTL 		BBPResetCtl;
#endif /* defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) || defined(RT5592) */

#ifdef LED_CONTROL_SUPPORT
	/* LED control */
	LED_CONTROL LedCntl;
#endif /* LED_CONTROL_SUPPORT */

	/* ---------------------------- */
	/* MAC control                                 */
	/* ---------------------------- */
#ifdef SPECIFIC_BCN_BUF_SUPPORT
	UCHAR ShrMSel;
	NDIS_SPIN_LOCK ShrMemLock;
#ifdef RTMP_MAC_USB
	RTMP_OS_SEM  ShrMemSemaphore;
#endif /* RTMP_MAC_USB */
#endif /* SPECIFIC_BCN_BUF_SUPPORT */

	UCHAR wmm_cw_min; /* CW_MIN_IN_BITS, actual CwMin = 2^CW_MIN_IN_BITS - 1 */
	UCHAR wmm_cw_max; /* CW_MAX_IN_BITS, actual CwMax = 2^CW_MAX_IN_BITS - 1 */

#ifdef RT8592
	CHAR bw_cal[3];	// bw cal value for RF_R32, 0:20M, 1:40M, 2:80M
// TODO: shiang-6590, temporary get from windows and need to revise it!!
	/* IQ Calibration */
	UCHAR IQGainTx[3][4];
	UCHAR IQPhaseTx[3][4];
	USHORT IQControl;
#endif /* RT8592 */

#if defined(RT3290) || defined(RLT_MAC)
#ifdef RTMP_MAC_PCI
	NDIS_SPIN_LOCK  WlanEnLock;
#endif

#ifdef RTMP_MAC_USB
	RTMP_OS_SEM WlanEnLock;
#endif

	WLAN_FUN_CTRL_STRUC WlanFunCtrl;
#endif /* defined(RT3290) || defined(RLT_MAC) */
#ifdef RT3290
	// TODO: shiang, check about the purpose of this parameter
	CMB_CTRL_3290_STRUC CmbCtrl;
	WLAN_FUN_INFO_STRUC WlanFunInfo;
	BT_FUN_CTRL_STRUC BtFunCtrl;
	WLAN_BT_COEX_SETTING WlanBTCoexInfo;
	BOOLEAN RateUp;
	ULONG AntennaDiversityState;/* 0->Stable state 1->training state */
	ULONG AntennaDiversityPER[2];  // 0 ->main 1->aux
	ULONG AntennaDiversityTxPacketCount[2];  // 0 ->main 1->aux
	ULONG AntennaDiversityRxPacketCount[2];
	ULONG AntennaDiversityTrigger;
	ULONG AntennaDiversityCount;
	ULONG TrainCounter;
	ANT_DIVERSITY AntennaDiversityInfo;
#endif /* RT3290 */

/*****************************************************************************************/
/*      802.11 related parameters                                                        */
/*****************************************************************************************/
	/* outgoing BEACON frame buffer and corresponding TXD */
#if !(defined(MT7603) || defined(MT7628))
	TXWI_STRUC BeaconTxWI;
	PUCHAR BeaconBuf;
#endif /* MT7603 */
	USHORT BeaconOffset[HW_BEACON_MAX_NUM];

	/* pre-build PS-POLL and NULL frame upon link up. for efficiency purpose. */
#ifdef CONFIG_STA_SUPPORT
	PSPOLL_FRAME PsPollFrame;
#endif /* CONFIG_STA_SUPPORT */
	HEADER_802_11 NullFrame;

#ifdef RTMP_MAC_USB
	TX_CONTEXT NullContext;
	TX_CONTEXT PsPollContext;
#endif /* RTMP_MAC_USB */

#ifdef MESH_SUPPORT
	BOOLEAN flg_mesh_init;
#endif /* MESH_SUPPORT */

#ifdef UAPSD_SUPPORT
	NDIS_SPIN_LOCK UAPSDEOSPLock;	/* EOSP frame access lock use */
	BOOLEAN bAPSDFlagSPSuspend;	/* 1: SP is suspended; 0: SP is not */
#endif /* UAPSD_SUPPORT */
#ifdef CONFIG_SNIFFER_SUPPORT
MONITOR_STRUCT monitor_ctrl;
#endif /* CONFIG_SNIFFER_SUPPORT */

/*=========AP=========== */
#ifdef CONFIG_AP_SUPPORT
	/* ----------------------------------------------- */
	/* AP specific configuration & operation status */
	/* used only when pAd->OpMode == OPMODE_AP */
	/* ----------------------------------------------- */
	AP_ADMIN_CONFIG ApCfg;	/* user configuration when in AP mode */
	AP_MLME_AUX ApMlmeAux;

#ifdef WDS_SUPPORT
	WDS_TABLE WdsTab;	/* WDS table when working as an AP */
	NDIS_SPIN_LOCK WdsTabLock;
#endif /* WDS_SUPPORT */

#ifdef MBSS_SUPPORT
	BOOLEAN FlgMbssInit;
#endif /* MBSS_SUPPORT */

#ifdef WDS_SUPPORT
	BOOLEAN flg_wds_init;
#endif /* WDS_SUPPORT */

#ifdef APCLI_SUPPORT
	BOOLEAN flg_apcli_init;
#endif /* APCLI_SUPPORT */

/*#ifdef AUTO_CH_SELECT_ENHANCE */
	PBSSINFO pBssInfoTab;
	PCHANNELINFO pChannelInfo;
/*#endif // AUTO_CH_SELECT_ENHANCE */

#ifdef NINTENDO_AP
	UCHAR NINTENDO_WEP_SEED[NINTENDO_SSID_NICKNAME_LN];
	UCHAR NINTENDO_WEP_KEY[16];
	BOOLEAN bNintendoCapable;
#endif /* NINTENDO_AP */

#endif /* CONFIG_AP_SUPPORT */

/*=======STA=========== */
#ifdef CONFIG_STA_SUPPORT
	/* ----------------------------------------------- */
	/* STA specific configuration & operation status */
	/* used only when pAd->OpMode == OPMODE_STA */
	/* ----------------------------------------------- */
	STA_ADMIN_CONFIG StaCfg;	/* user desired settings */
	STA_ACTIVE_CONFIG StaActive;	/* valid only when ADHOC_ON(pAd) || INFRA_ON(pAd) */
	CHAR nickname[IW_ESSID_MAX_SIZE + 1];	/* nickname, only used in the iwconfig i/f */
	NDIS_MEDIA_STATE PreMediaState;
#endif /* CONFIG_STA_SUPPORT */

/*=======Common=========== */
	enum RATE_ADAPT_ALG rateAlg;		/* Rate adaptation algorithm */

	NDIS_MEDIA_STATE IndicateMediaState;	/* Base on Indication state, default is NdisMediaStateDisConnected */

#ifdef PROFILE_STORE
	RTMP_OS_TASK 	WriteDatTask;
	BOOLEAN			bWriteDat;
#endif /* PROFILE_STORE */

#ifdef CREDENTIAL_STORE
	STA_CONNECT_INFO StaCtIf;
#endif /* CREDENTIAL_STORE */

#ifdef WSC_INCLUDED
	RTMP_OS_TASK wscTask;
	UCHAR WriteWscCfgToDatFile;
	BOOLEAN WriteWscCfgToAr9DatFile;
	NDIS_SPIN_LOCK WscElmeLock;
	MLME_QUEUE_ELEM *pWscElme;

	/* WSC hardware push button function 0811 */
	BOOLEAN WscHdrPshBtnFlag;	/* 1: support, read from EEPROM */
#ifdef CONFIG_AP_SUPPORT
	BOOLEAN bWscDriverAutoUpdateCfg;
#endif /* CONFIG_AP_SUPPORT */
#endif /* WSC_INCLUDED */

#ifdef RTMP_RBUS_SUPPORT
#ifdef RT3XXX_ANTENNA_DIVERSITY_SUPPORT
	RTMP_OS_TASK ADTask;
#endif /* RT3XXX_ANTENNA_DIVERSITY_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

	/* MAT related parameters */
#ifdef MAT_SUPPORT
	MAT_STRUCT MatCfg;
#endif /* MAT_SUPPORT */

#ifdef RTMP_RBUS_SUPPORT
#ifdef VIDEO_TURBINE_SUPPORT
	AP_VIDEO_STRUCT VideoTurbine;
#endif /* VIDEO_TURBINE_SUPPORT */
#endif /* RTMP_RBUS_SUPPORT */

	/*
		Frequency setting for rate adaptation
			@ra_interval: 		for baseline time interval
			@ra_fast_interval:	for quick response time interval
	*/
	UINT32 ra_interval;
	UINT32 ra_fast_interval;

	/* configuration: read from Registry & E2PROM */
	BOOLEAN bLocalAdminMAC;	/* Use user changed MAC */
	UCHAR PermanentAddress[MAC_ADDR_LEN];	/* Factory default MAC address */
	UCHAR CurrentAddress[MAC_ADDR_LEN];	/* User changed MAC address */

	/* ------------------------------------------------------ */
	/* common configuration to both OPMODE_STA and OPMODE_AP */
	/* ------------------------------------------------------ */
	UINT VirtualIfCnt;

	COMMON_CONFIG CommonCfg;
#ifdef MT76XX_BTCOEX_SUPPORT
	ULONG					BTCOMBOFlags;
	ULONG                          BTStatusFlags;
       PROTECTION_FRAME_STRUCT          NullFrameSpace[NULLFRAMESPACE];
	UCHAR                          HASH_BSSID[10][MAC_ADDR_LEN + 1];
       COEX_RX_MANUAL_AGC_GAIN            coexRXManualAGCGain;
	COEX_MODE_STRUCTURE                CoexMode;
#endif /*MT76XX_BTCOEX_SUPPORT*/
	MLME_STRUCT Mlme;

	/* AP needs those vaiables for site survey feature. */
#ifdef CONFIG_STA_SUPPORT
	MLME_AUX MlmeAux;	/* temporary settings used during MLME state machine */
#endif /* CONFIG_STA_SUPPORT */
#if defined(AP_SCAN_SUPPORT) || defined(CONFIG_STA_SUPPORT)
	SCAN_CTRL ScanCtrl;
	BSS_TABLE ScanTab;	/* store the latest SCAN result */
#endif /* defined(AP_SCAN_SUPPORT) || defined(CONFIG_STA_SUPPORT) */

	/*About MacTab, the sta driver will use #0 and #1 for multicast and AP. */
	MAC_TABLE MacTab;	/* ASIC on-chip WCID entry table.  At TX, ASIC always use key according to this on-chip table. */
	NDIS_SPIN_LOCK MacTabLock;

	EXT_CAP_INFO_ELEMENT ExtCapInfo;

	/* DOT11_H */
	DOT11_H Dot11_H;

	/* encryption/decryption KEY tables */
	CIPHER_KEY SharedKey[HW_BEACON_MAX_NUM + MAX_P2P_NUM][4];	/* STA always use SharedKey[BSS0][0..3] */

	/* various Counters */
	COUNTER_802_3 Counters8023;	/* 802.3 counters */
	COUNTER_802_11 WlanCounters;	/* 802.11 MIB counters */
	COUNTER_RALINK RalinkCounters;	/* Ralink propriety counters */
	/* COUNTER_DRS DrsCounters;	*/ /* counters for Dynamic TX Rate Switching */
	PRIVATE_STRUC PrivateInfo;	/* Private information & counters */

	/* flags, see fRTMP_ADAPTER_xxx flags */
	ULONG Flags;		/* Represent current device status */
	ULONG PSFlags;		/* Power Save operation flag. */
	ULONG MoreFlags;	/* Represent specific requirement */

	/* current TX sequence # */
	USHORT Sequence;

	/* Control disconnect / connect event generation */
	/*+++Didn't used anymore */
	ULONG LinkDownTime;
	/*--- */
	ULONG LastRxRate;
	ULONG LastTxRate;
	/*+++Used only for Station */
	BOOLEAN bConfigChanged;	/* Config Change flag for the same SSID setting */
	BOOLEAN bDisableRtsProtect;
	/*--- */

	ULONG ExtraInfo;	/* Extra information for displaying status of UI */
	ULONG SystemErrorBitmap;	/* b0: E2PROM version error */
	UCHAR SME;
#ifdef SYSTEM_LOG_SUPPORT
	/* --------------------------- */
	/* System event log                       */
	/* --------------------------- */
	RT_802_11_EVENT_TABLE EventTab;
#endif /* SYSTEM_LOG_SUPPORT */

#ifdef HOSTAPD_SUPPORT
	UINT32 IoctlIF;
#endif /* HOSTAPD_SUPPORT */
#ifdef INF_PPA_SUPPORT
	UINT32 g_if_id;
	BOOLEAN PPAEnable;
	PPA_DIRECTPATH_CB *pDirectpathCb;
#endif /* INF_PPA_SUPPORT */

	/**********************************************************/
	/*      Statistic related parameters                                                    */
	/**********************************************************/
#ifdef RTMP_MAC_USB
	ULONG BulkOutDataOneSecCount;
	ULONG BulkInDataOneSecCount;
	ULONG BulkLastOneSecCount;	/* BulkOutDataOneSecCount + BulkInDataOneSecCount */
	ULONG watchDogRxCnt;
	ULONG watchDogRxOverFlowCnt;
	ULONG watchDogTxPendingCnt[NUM_OF_TX_RING];
#endif /* RTMP_MAC_USB */

	BOOLEAN bUpdateBcnCntDone;

#ifdef RELEASE_EXCLUDE
	/* A watchdog to prevent MAC/BBP into deadlock condition */
#endif /* RELEASE_EXCLUDE */
	ULONG macwd;
#ifdef RT6352
	BOOLEAN bEnableMacWD;
#endif /* RT6352 */
	/* ---------------------------- */
	/* DEBUG paramerts */
	/* ---------------------------- */

	BOOLEAN bLinkAdapt;
	BOOLEAN bForcePrintTX;
	BOOLEAN bForcePrintRX;
	BOOLEAN bStaFifoTest;
	BOOLEAN bProtectionTest;
	BOOLEAN bHCCATest;
	BOOLEAN bGenOneHCCA;
	BOOLEAN bBroadComHT;
	ULONG BulkOutReq;
	ULONG BulkOutComplete;
	ULONG BulkOutCompleteOther;
	ULONG BulkOutCompleteCancel;	/* seems not use now? */
#ifdef MT76x2
	INT32 rts_tx_retry_num;
#endif /* MT76x2 */
	struct wificonf WIFItestbed;

	UCHAR TssiGain;
#if 0
#ifdef CONFIG_ATE
	ATE_INFO ate;
#endif /* CONFIG_ATE */
#endif

#ifdef RTMP_MAC_USB
	BOOLEAN ContinBulkOut;	/*ATE bulk out control */
	BOOLEAN ContinBulkIn;	/*ATE bulk in control */
	RTMP_OS_ATOMIC BulkOutRemained;
	RTMP_OS_ATOMIC BulkInRemained;
#endif /* RTMP_MAC_USB */

#ifdef CONFIG_ATE
	ATE_CTRL ATECtrl;
#endif

#ifdef DOT11_N_SUPPORT
	struct reordering_mpdu_pool mpdu_blk_pool;
#endif /* DOT11_N_SUPPORT */

	/* statistics count */
#if 0 /* os abl move */
#ifdef LINUX
#if WIRELESS_EXT >= 12
	struct iw_statistics iw_stats;
#endif

	struct net_device_stats stats;
#endif /* LINUX */
#else

	VOID *iw_stats;
	VOID *stats;
#endif

#ifdef BLOCK_NET_IF
	BLOCK_QUEUE_ENTRY blockQueueTab[NUM_OF_TX_RING];
#endif /* BLOCK_NET_IF */

#ifdef CONFIG_AP_SUPPORT
#ifdef IGMP_SNOOP_SUPPORT
	PMULTICAST_FILTER_TABLE pMulticastFilterTable;
	UCHAR IgmpGroupTxRate;
#endif /* IGMP_SNOOP_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef ETH_CONVERT_SUPPORT
	ETH_CONVERT_STRUCT EthConvert;
#endif /* ETH_CONVERT_SUPPORT */

#ifdef MULTIPLE_CARD_SUPPORT
	INT32 MC_RowID;
	RTMP_STRING MC_FileName[256];
	UINT32 E2P_OFFSET_IN_FLASH[MAX_NUM_OF_MULTIPLE_CARD];
#endif /* MULTIPLE_CARD_SUPPORT */

	ULONG TbttTickCount;	/* beacon timestamp work-around */

#ifdef CONFIG_AP_SUPPORT
	RALINK_TIMER_STRUCT PeriodicTimer;
#endif /* CONFIG_AP_SUPPORT */

	/* for detect_wmm_traffic() BE TXOP use */
	ULONG OneSecondnonBEpackets;	/* record non BE packets per second */
	UCHAR is_on;

	/* for detect_wmm_traffic() BE/BK TXOP use */
#define TIME_BASE			(1000000/OS_HZ)
#define TIME_ONE_SECOND		(1000000/TIME_BASE)
	UCHAR flg_be_adjust;
	ULONG be_adjust_last_time;

#ifdef NINTENDO_AP
	NINDO_CTRL_BLOCK nindo_ctrl_block;
#endif /* NINTENDO_AP */

#ifdef WSC_INCLUDED
	/* for multiple card */
	UCHAR *pHmacData;
#endif /* WSC_INCLUDED */

#ifdef IKANOS_VX_1X0
	struct IKANOS_TX_INFO IkanosTxInfo;
	struct IKANOS_TX_INFO IkanosRxInfo[HW_BEACON_MAX_NUM + MAX_WDS_ENTRY +
					   MAX_APCLI_NUM + MAX_MESH_NUM];
#endif /* IKANOS_VX_1X0 */

#ifdef MESH_SUPPORT
	NDIS_SPIN_LOCK MeshTabLock;
	MESH_STRUCT MeshTab;
#endif /* MESH_SUPPORT */

#ifdef WMM_ACM_SUPPORT
	UINT8 *pACM_Ctrl_BK;
	UINT16 AcmAvalCap;	/* unit: 32us */

	NDIS_SPIN_LOCK AcmTspecSemLock;
	NDIS_SPIN_LOCK AcmTspecIrqLock;
#endif /* WMM_ACM_SUPPORT */

	UINT8 FlgCtsEnabled;
	UINT8 PM_FlgSuspend;

#ifdef CONFIG_STA_SUPPORT
#ifdef DOT11R_FT_SUPPORT
	/* RIC */
#define FT_RIC_CB		((FT_RIC_CTRL_BLOCK *)(pAd->pFT_RIC_Ctrl_BK))
	VOID *pFT_RIC_Ctrl_BK;
	NDIS_SPIN_LOCK FT_RicLock;
#endif /* DOT11R_FT_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
	LIST_HEADER DMSEntryList;
	LIST_HEADER FMSEntryList;
#endif /* DOT11V_WNM_SUPPORT */

	UCHAR FifoUpdateDone, FifoUpdateRx;

#ifdef LINUX
#ifdef RT_CFG80211_SUPPORT
	CFG80211_CTRL cfg80211_ctrl;
	VOID *pCfg80211_CB;
#endif /* RT_CFG80211_SUPPORT */
#endif /* LINUX */

#ifdef OS_ABL_SUPPORT
#ifdef MAT_SUPPORT
	/* used in OS_ABL */
	BOOLEAN (*MATPktRxNeedConvert) (RTMP_ADAPTER *pAd, PNET_DEV net_dev);

	PUCHAR (*MATEngineRxHandle)(RTMP_ADAPTER *pAd, PNDIS_PACKET pPkt, UINT infIdx);
#endif /* MAT_SUPPORT */
#endif /* OS_ABL_SUPPORT */

	UINT32 ContinueMemAllocFailCount;

	struct {
		INT IeLen;
		UCHAR *pIe;
	} ProbeRespIE[MAX_LEN_OF_BSS_TABLE];
#ifdef HW_COEXISTENCE_SUPPORT
	BOOLEAN bHWCoexistenceInit;
	BOOLEAN bWiMaxCoexistenceOn;
#endif /* HW_COEXISTENCE_SUPPORT */
#ifdef BT_COEXISTENCE_SUPPORT
#ifdef RELEASE_EXCLUDE
	/* BT Coexistence Data Structure */
#endif /* RELEASE_EXCLUDE */
	BOOLEAN bMiscOn;
#ifdef RTMP_USB_SUPPORT
	BOOLEAN bCoexMethod;
#endif /* RTMP_USB_SUPPORT */
#ifdef DOT11_N_SUPPORT
	BOOLEAN bPermitRecBaDown;
	BOOLEAN bPermitMcsDown;
	BOOLEAN bPermitTxBaSizeDown;
	BOOLEAN bPermitTxBaDensityDown;
#endif /* DOT11_N_SUPPORT */
	BOOLEAN bPermitTxPowerDown;
	BOOLEAN bPermitLnaGainDown;
#ifdef RELEASE_EXCLUDE
	/* bluetooth configureation */
#endif /* RELEASE_EXCLUDE */
	ULONG ulConfiguration;
	ULONG ulActiveCountPastPeriod;	/* Record Buletooth activities in the past period */
	BUSY_DEGREE BusyDegree;	/* BT Device Status */
#endif /* BT_COEXISTENCE_SUPPORT */

	/* purpose: We free all kernel resources when module is removed */
	LIST_HEADER RscTimerMemList;	/* resource timers memory */
	LIST_HEADER RscTaskMemList;	/* resource tasks memory */
	LIST_HEADER RscLockMemList;	/* resource locks memory */
	LIST_HEADER RscTaskletMemList;	/* resource tasklets memory */
	LIST_HEADER RscSemMemList;	/* resource semaphore memory */
	LIST_HEADER RscAtomicMemList;	/* resource atomic memory */

	/* purpose: Cancel all timers when module is removed */
	LIST_HEADER RscTimerCreateList;	/* timers list */

#ifdef OS_ABL_SUPPORT
#ifdef RTMP_PCI_SUPPORT
	RTMP_PCI_CONFIG PciConfig;
#endif /* RTMP_PCI_SUPPORT */
#endif /* OS_ABL_SUPPORT */

#ifdef P2P_SUPPORT
	RT_P2P_CONFIG			P2pCfg;
	NDIS_SPIN_LOCK			P2pTableSemLock;
	RT_P2P_TABLE			P2pTable;
	ULONG					GOBeaconBufNoALen;
	CHAR					GoBeaconBuf[512]; /* NOTE: BeaconBuf should be 4-byte aligned */
	ULONG					BeaconBufLen;
	ULONG					GoBeaconBufLen;
	BOOLEAN 				bIsClearScanTab;   /* TURE, we need to force Scan */
	BOOLEAN 				flg_p2p_init;
	ULONG					flg_p2p_OpStatusFlags;
	UCHAR					P2PChannel;
#ifdef DOT11_N_SUPPORT
	UINT8					P2PExtChOffset;
#endif /* DOT11_N_SUPPORT */
	UCHAR					P2PCurrentAddress[MAC_ADDR_LEN];	  /* User changed MAC address */
	PNET_DEV				p2p_dev;
#endif /* P2P_SUPPORT */

#ifdef RT3883
	UINT32 FlgCWC;
#endif /* RT3883 */

#ifdef SMART_ANTENNA
	int smartAntEnable;	/* trigger to enable the smartAntenna function */
	int smartAntDbgOn;
	SMART_ANTENNA_STRUCT *pSAParam;
	NDIS_SPIN_LOCK smartAntLock;/* lock used to handle synchronization of smart antenna algorithms. */
#endif /* SMART_ANTENNA */

#ifdef WIDI_SUPPORT
	USHORT mfg_code;
	USHORT prod_code;
	UINT serial_num;
	UCHAR cap_flag;
	UCHAR hori_size;
	UCHAR vert_size;
	/* Scan toggle for L2SD and P2P Listen Channel*/
	ULONG p2p_l2sd_scan_toggle;
	/* Flag to check whether you are sending a probe response. If set , stay on channel for dwell time*/
	INT gP2pSendingProbeResponse;
	/* p2p device Name*/
	UCHAR device_name[32];

#ifdef WFA_WFD_SUPPORT
	PUCHAR pWfdIeInBeacon;
	UCHAR WfdIeInBeaconLen;
	PUCHAR pWfdIeInProbeReq;
	UCHAR WfdIeInProbeReqLen;
	PUCHAR pWfdIeInProbeRsp;
	UCHAR WfdIeInProbeRspLen;
	PUCHAR pWfdIeInActionPkt;
	UCHAR WfdIeInActionPktLen;

	PUCHAR pWfdIeInAssocReq;
	UCHAR WfdIeInAssocReqLen;
	PUCHAR pWfdIeInAssocRsp;
	UCHAR WfdIeInAssocRspLen;
#endif /* WFA_WFD_SUPPORT */
	UCHAR EnterpriseEnabled; // EnterpriseEnabled 1: enable 0: disable
	UCHAR ToggleEnabled;       //tmcourie: ToggleEnabled 1: enable 0: disable
#endif /* WIDI_SUPPORT */
#ifdef CONFIG_MULTI_CHANNEL
	CHAR NullFrBuf[256];
	UINT32 MultiChannelFlowCtl;
	RTMP_OS_TASK MultiChannelTask;
	UCHAR MultiChannelAction;
#endif /* CONFIG_MULTI_CHANNEL */

#if (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB)) || defined(MT_WOW_SUPPORT)
	WOW_CFG_STRUCT WOW_Cfg; /* data structure for wake on wireless */
#endif /* (defined(WOW_SUPPORT) && defined(RTMP_MAC_USB)) || defined(MT_WOW_SUPPORT) */

	TMAC_TXD_L NullTxWI;
	USHORT NullBufOffset[2];

#ifdef MULTI_MAC_ADDR_EXT_SUPPORT
	BOOLEAN bUseMultiMacAddrExt;
#endif /* MULTI_MAC_ADDR_EXT_SUPPORT */

#ifdef HW_TX_RATE_LOOKUP_SUPPORT
	BOOLEAN bUseHwTxLURate;
#endif /* HW_TX_RATE_LOOKUP_SUPPORT */

#ifdef RT6352
	UCHAR RfBank;
	UCHAR VGA_Gain0_idx;
	UCHAR VGA_Gain1_idx;
	BOOLEAN bCalibrationDone;
	CHAR tx_bw_cal[2];	// bw cal value for RF_R32, 0:20M, 1:40M
	CHAR rx_bw_cal[2];	// bw cal value for RF_R32, 0:20M, 1:40M
	UCHAR Tx0_DPD_ALC_tag0;
	UCHAR Tx0_DPD_ALC_tag1;
	UCHAR Tx1_DPD_ALC_tag0;
	UCHAR Tx1_DPD_ALC_tag1;

	UCHAR Tx0_DPD_ALC_tag0_flag;
	UCHAR Tx0_DPD_ALC_tag1_flag;
	UCHAR Tx1_DPD_ALC_tag0_flag;
	UCHAR Tx1_DPD_ALC_tag1_flag;
	INT32 DoDPDCurrTemperature;
	INT32 DoCalibrationTemperature;
	UINT16 E2p_D0_Value;
	INT32 CurrTemperature;
	CHAR TemperatureRef25C;
	BOOLEAN bRef25CVaild;
	BOOLEAN bLowTemperatureTrigger;
	CHAR BW_Power_Delta;
	BOOLEAN bExtPA;
#endif /* RT6352 */

#ifdef CONFIG_ANDES_SUPPORT
	struct MCU_CTRL MCUCtrl;
#endif /* CONFIG_ANDES_SUPPORT */

#ifdef WLAN_SKB_RECYCLE
    struct sk_buff_head rx0_recycle;
#endif /* WLAN_SKB_RECYCLE */

#ifdef CONFIG_MULTI_CHANNEL
	USHORT NullBufOffset[2];
	CHAR NullFrBuf[100];
	UINT32 NullFrLen;
	/*
	UINT32 MultiChannelFlowCtl;
	RTMP_OS_TASK MultiChannelTask;
	UCHAR MultiChannelAction;
	*/
#endif /* CONFIG_MULTI_CHANNEL */

#ifdef SINGLE_SKU_V2
	BOOLEAN SKUEn;
	DL_LIST SingleSkuPwrList;
	UCHAR DefaultTargetPwr;
	CHAR SingleSkuRatePwrDiff[19];
	BOOLEAN bOpenFileSuccess;
#endif /* SINGLE_SKU_V2 */


#if defined(WFA_VHT_PF) || defined(MT7603_FPGA) || defined(MT7628_FPGA)
	BOOLEAN force_amsdu;
#endif
#ifdef WFA_VHT_PF
	BOOLEAN force_noack;
	BOOLEAN vht_force_sgi;
	BOOLEAN vht_force_tx_stbc;

	BOOLEAN force_vht_op_mode;
	UCHAR vht_pf_op_ss;
	UCHAR vht_pf_op_bw;
#endif /* WFA_VHT_PF */

#ifdef CONFIG_FPGA_MODE
	struct fpga_ctrl fpga_ctl;
#endif /* CONFIG_FPGA_MODE */

#ifdef DBG_DIAGNOSE
	RtmpDiagStruct DiagStruct;
#endif /* DBG_DIAGNOSE */

#ifdef CONFIG_SNIFFER_SUPPORT
	struct sniffer_control sniffer_ctl;
#endif

#ifdef RTMP_USB_SUPPORT
	struct usb_control usb_ctl;
#endif

#ifdef MT76x2
#ifdef CONFIG_CALIBRATION_COLLECTION
	struct mt76x2_calibration_info calibration_info[MAX_MT76x2_CALIBRATION_ID];
#endif
#endif /* MT76x2 */

#ifdef VHT_TXBF_SUPPORT
	BOOLEAN NDPA_Request;
#endif

    BOOLEAN        bPS_Retrieve;

    UINT32 rxv2_cyc3[10];

#ifdef DBG
#ifdef MT_MAC
    UCHAR BcnCnt; //Carter debug
    ULONG HandleInterruptTime;
    ULONG HandlePreInterruptTime;
#endif
#endif
#ifdef RTMP_PCI_SUPPORT
	UINT32 DropInvalidPacket;
	BOOLEAN RxRest;
	BOOLEAN PDMAWatchDogEn;
	BOOLEAN PDMAWatchDogDbg;
	UINT8 TxDMACheckTimes;
	UINT8 RxDMACheckTimes;
	ULONG TxDMAResetCount;
	ULONG RxDMAResetCount;
	ULONG PDMAResetFailCount;
	NDIS_SPIN_LOCK IndirectUpdateLock;
#endif
	BOOLEAN PSEWatchDogEn;
	UINT8 RxPseCheckTimes;
	ULONG PSEResetCount;
	ULONG PSETriggerType1Count;
	ULONG PSETriggerType2Count;
	ULONG PSEResetFailCount;
#ifdef RTMP_USB_SUPPORT
	RTMP_OS_SEM IndirectUpdateLock;
#endif

	ULONG SkipTxRCount;
	UINT shortretry;
    BOOLEAN bDisableBGProtect;
    BOOLEAN bNonGFExist;

	ULONG   TxTotalByteCnt;
    ULONG   RxTotalByteCnt;
    TX_AC_PARAM_T   CurrEdcaParam[CMD_EDCA_AC_MAX];

#ifdef LOAD_FW_ONE_TIME
	BOOLEAN  FWLoad;
#endif /* LOAD_FW_ONE_TIME */

#ifdef USB_IOT_WORKAROUND2
	BOOLEAN  bUSBIOTReady;
#endif
#ifdef SMART_CARRIER_SENSE_SUPPORT
	SMART_CARRIER_SENSE_CTRL	SCSCtrl;
#endif /* SMART_CARRIER_SENSE_SUPPORT */
	BOOLEAN bLink11b;
};

#if defined(RTMP_INTERNAL_TX_ALC) || defined(RTMP_TEMPERATURE_COMPENSATION)
/* The offset of the Tx power tuning entry (zero-based array) */
#define TX_POWER_TUNING_ENTRY_OFFSET			30

/* The lower-bound of the Tx power tuning entry */
#define LOWERBOUND_TX_POWER_TUNING_ENTRY		-30

/* The upper-bound of the Tx power tuning entry in G band */
#define UPPERBOUND_TX_POWER_TUNING_ENTRY(__pAd)		((__pAd)->chipCap.TxAlcTxPowerUpperBound_2G)

#ifdef A_BAND_SUPPORT
/* The upper-bound of the Tx power tuning entry in A band */
#define UPPERBOUND_TX_POWER_TUNING_ENTRY_5G(__pAd)		((__pAd)->chipCap.TxAlcTxPowerUpperBound_5G)
#endif /* A_BAND_SUPPORT */

/* Temperature compensation lookup table */

#define TEMPERATURE_COMPENSATION_LOOKUP_TABLE_OFFSET	7

/* The lower/upper power delta index for the TSSI rate table */

#define LOWER_POWER_DELTA_INDEX		0
#define UPPER_POWER_DELTA_INDEX		24

/* The offset of the TSSI rate table */

#define TSSI_RATIO_TABLE_OFFSET	12


/* Get the power delta bound */

#define GET_TSSI_RATE_TABLE_INDEX(x) (((x) > UPPER_POWER_DELTA_INDEX) ? (UPPER_POWER_DELTA_INDEX) : (((x) < LOWER_POWER_DELTA_INDEX) ? (LOWER_POWER_DELTA_INDEX) : ((x))))

/* 802.11b CCK TSSI information */

typedef union _CCK_TSSI_INFO
{
#ifdef RT_BIG_ENDIAN
	struct
	{
		UCHAR	Reserved:1;
		UCHAR	ShortPreamble:1;
		UCHAR	Rate:2;
		UCHAR	Tx40MSel:2;
		UCHAR	TxType:2;
	} field;
#else
	struct
	{
		UCHAR	TxType:2;
		UCHAR	Tx40MSel:2;
		UCHAR	Rate:2;
		UCHAR	ShortPreamble:1;
		UCHAR	Reserved:1;
	} field;
#endif /* RT_BIG_ENDIAN */

	UCHAR	value;
} CCK_TSSI_INFO, *PCCK_TSSI_INFO;


/* 802.11a/g OFDM TSSI information */

typedef union _OFDM_TSSI_INFO
{
#ifdef RT_BIG_ENDIAN
	struct
	{
		UCHAR	Rate:4;
		UCHAR	Tx40MSel:2;
		UCHAR	TxType:2;
	} field;
#else
	struct
	{
		UCHAR	TxType:2;
		UCHAR	Tx40MSel:2;
		UCHAR	Rate:4;
	} field;
#endif /* RT_BIG_ENDIAN */

	UCHAR	value;
} OFDM_TSSI_INFO, *POFDM_TSSI_INFO;


/* 802.11n HT TSSI information */

typedef struct _HT_TSSI_INFO {
	union {
#ifdef RT_BIG_ENDIAN
		struct {
			UCHAR SGI:1;
			UCHAR STBC:2;
			UCHAR Aggregation:1;
			UCHAR Tx40MSel:2;
			UCHAR TxType:2;
		} field;
#else
		struct {
			UCHAR TxType:2;
			UCHAR Tx40MSel:2;
			UCHAR Aggregation:1;
			UCHAR STBC:2;
			UCHAR SGI:1;
		} field;
#endif /* RT_BIG_ENDIAN */

		UCHAR value;
	} PartA;

	union {
#ifdef RT_BIG_ENDIAN
		struct {
			UCHAR BW:1;
			UCHAR MCS:7;
		} field;
#else
		struct {
			UCHAR MCS:7;
			UCHAR BW:1;
		} field;
#endif /* RT_BIG_ENDIAN */
		UCHAR	value;
	} PartB;
} HT_TSSI_INFO, *PHT_TSSI_INFO;

typedef struct _TSSI_INFO_{
	UCHAR tssi_info_0;
	union {
		CCK_TSSI_INFO cck_tssi_info;
		OFDM_TSSI_INFO ofdm_tssi_info;
		HT_TSSI_INFO ht_tssi_info_1;
		UCHAR byte;
	}tssi_info_1;
	HT_TSSI_INFO ht_tssi_info_2;
}TSSI_INFO;

#endif /* RTMP_INTERNAL_TX_ALC || RTMP_TEMPERATURE_COMPENSATION */


typedef struct _PEER_PROBE_REQ_PARAM {
	UCHAR Addr2[MAC_ADDR_LEN];
	CHAR Ssid[MAX_LEN_OF_SSID];
	UCHAR SsidLen;
	BOOLEAN bRequestRssi;
#ifdef CONFIG_HOTSPOT
	BOOLEAN IsIWIE;
	BOOLEAN IsIWCapability;
	UCHAR Hessid[MAC_ADDR_LEN];
	BOOLEAN IsHessid;
	UINT8 AccessNetWorkType;
#endif /* CONFIG_HOTSPOT */
} PEER_PROBE_REQ_PARAM, *PPEER_PROBE_REQ_PARAM;


/***************************************************************************
  *	Rx Path software control block related data structures
  **************************************************************************/
typedef enum RX_BLK_FLAGS{
	fRX_AMPDU = (1 << 0),
	fRX_AMSDU = (1 << 1),
	fRX_ARALINK = (1 << 2),
	fRX_HTC = (1 << 3),
	fRX_PAD = (1 << 4),
	fRX_QOS = (1 << 5),
	fRX_EAP = (1 << 6),
	fRX_WPI = (1 << 7),
	fRX_AP = (1 << 8),			// Packet received from AP
	fRX_STA = (1 << 9),			// Packet received from Client(Infra mode)
	fRX_ADHOC = (1 << 10),		// packet received from AdHoc peer
	fRX_WDS = (1 << 11),		// Packet received from WDS
	fRX_MESH = (1 << 12),		// Packet received from MESH
	fRX_DLS = (1 << 13),		// Packet received from DLS peer
	fRX_TDLS = (1 << 14),		// Packet received from TDLS peer
	fRX_RETRIEVE = (1 << 15), 	// Packet received from mcu
	fRX_CMD_RSP = (1 << 16),	// Pakket received from mcu command response
	fRX_TXRX_RXV = (1 << 17),   // RxV received from Rx Ring1
}RX_BLK_FLAGS;


typedef struct _RX_BLK
{
	UCHAR hw_rx_info[RXD_SIZE]; /* include "RXD_STRUC RxD" and "RXINFO_STRUC rx_info " */
	RXINFO_STRUC *pRxInfo; /* for RLT, in head of frame buffer, for RTMP, in hw_rx_info[RXINFO_OFFSET] */
#ifdef RLT_MAC
	RXFCE_INFO *pRxFceInfo; /* for RLT, in in hw_rx_info[RXINFO_OFFSET], for RTMP, no such field */
#endif /* RLT_MAC */
	RXWI_STRUC *pRxWI; /*in frame buffer and after "rx_info" fields */
	UCHAR *rmac_info;
	HEADER_802_11 *pHeader; /* poiter of 802.11 header, pointer to frame buffer and shall not shift this pointer */
	PNDIS_PACKET pRxPacket; /* os_packet pointer, shall not change */
	UCHAR *pData; /* init to pRxPacket->data, refer to frame buffer, may changed depends on processing */
	USHORT DataSize; /* init to  RXWI->MPDUtotalByteCnt, and may changes depends on processing */
	RX_BLK_FLAGS Flags;

	/* Mirror info of partial fields of RxWI and RxInfo */
	USHORT MPDUtotalByteCnt; /* Refer to RXWI->MPDUtotalByteCnt */
	UCHAR UserPriority;	/* for calculate TKIP MIC using */
	UCHAR OpMode;	/* 0:OPMODE_STA 1:OPMODE_AP */
	UCHAR wcid;		/* copy of pRxWI->wcid */
	UCHAR U2M;
	UCHAR key_idx;
	UCHAR bss_idx;
	UCHAR TID;
	UINT32 TimeStamp;
	struct rx_signal_info rx_signal;
	CHAR ldpc_ex_sym;
	HTTRANSMIT_SETTING rx_rate;
    UINT32 rxv2_cyc1;
    UINT32 rxv2_cyc2;
    UINT32 rxv2_cyc3;
#ifdef HDR_TRANS_SUPPORT
	BOOLEAN	bHdrRxTrans;	/* this packet's header is translated to 802.3 by HW  */
	BOOLEAN bHdrVlanTaged;	/* VLAN tag is added to this header */
	UCHAR *pTransData;
	USHORT TransDataSize;
#endif /* HDR_TRANS_SUPPORT */
} RX_BLK;


#define RX_BLK_SET_FLAG(_pRxBlk, _flag)		(_pRxBlk->Flags |= _flag)
#define RX_BLK_TEST_FLAG(_pRxBlk, _flag)		(_pRxBlk->Flags & _flag)
#define RX_BLK_CLEAR_FLAG(_pRxBlk, _flag)	(_pRxBlk->Flags &= ~(_flag))


#define AMSDU_SUBHEAD_LEN	14
#define ARALINK_SUBHEAD_LEN	14
#define ARALINK_HEADER_LEN	 2


/***************************************************************************
  *	Tx Path software control block related data structures
  **************************************************************************/
#define TX_UNKOWN_FRAME		0x00
#define TX_MCAST_FRAME			0x01
#define TX_LEGACY_FRAME		0x02
#define TX_AMPDU_FRAME		0x04
#define TX_AMSDU_FRAME		0x08
#define TX_RALINK_FRAME		0x10
#define TX_FRAG_FRAME			0x20
// TODO: shiang-usw, revise for TX_NDPA_FRAME!
#define TX_NDPA_FRAME		0x40

#define TX_FRAG_ID_NO			0x0
#define TX_FRAG_ID_FIRST		0x1
#define TX_FRAG_ID_MIDDLE		0x2
#define TX_FRAG_ID_LAST			0x3

/* Currently the sizeof(TX_BLK) is 148 bytes. */
typedef struct _TX_BLK {
	UCHAR				QueIdx;
	UCHAR				TxFrameType;				/* Indicate the Transmission type of the all frames in one batch */
	UCHAR				TotalFrameNum;				/* Total frame number want to send-out in one batch */
#ifdef MT_MAC
	UCHAR				FragIdx;					/* refer to TX_FRAG_ID_xxxx */
	UINT8				Pid;
	UINT8				TxS2Host;
	UINT8				TxS2Mcu;
	UINT8				TxSFormat;
#endif /* MT_MAC */
	USHORT				TotalFragNum;				/* Total frame fragments required in one batch */
	USHORT				TotalFrameLen;				/* Total length of all frames want to send-out in one batch */

	QUEUE_HEADER		TxPacketList;
	MAC_TABLE_ENTRY	*pMacEntry;					/* NULL: packet with 802.11 RA field is multicast/broadcast address */
	STA_TR_ENTRY *tr_entry;
	HTTRANSMIT_SETTING	*pTransmit;

	/* Following structure used for the characteristics of a specific packet. */
	PNDIS_PACKET		pPacket;
	UCHAR				*pSrcBufHeader;				/* Reference to the head of sk_buff->data */
	UCHAR				*pSrcBufData;				/* Reference to the sk_buff->data, will changed depends on hanlding progresss */
	UINT				SrcBufLen;					/* Length of packet payload which not including Layer 2 header */
	UCHAR				*pExtraLlcSnapEncap;			/* NULL means no extra LLC/SNAP is required */
	UCHAR				hw_rsv_len;

#ifndef VENDOR_FEATURE1_SUPPORT
	/*
		Note: Can not insert any other new parameters
		between pExtraLlcSnapEncap & HeaderBuf; Or
		the start address of HeaderBuf will not be aligned by 4.

		But we can not change HeaderBuf[128] to HeaderBuf[32] because
		many codes use HeaderBuf[index].
	*/
	UCHAR				HeaderBuf[128];				/* TempBuffer for TX_INFO + TX_WI + TSO_INFO + 802.11 Header + padding + AMSDU SubHeader + LLC/SNAP */
#else
	UINT32				HeaderBuffer[32];			/* total 128B, use UINT32 to avoid alignment problem */
	UCHAR				*HeaderBuf;
#endif /* VENDOR_FEATURE1_SUPPORT */
	UCHAR				*wifi_hdr;

	UCHAR				MpduHeaderLen;				/* 802.11 header length NOT including the padding */
	UCHAR				first_buf_len;
	UCHAR				wifi_hdr_len;
	UCHAR				HdrPadLen;					/* recording Header Padding Length; */
	UCHAR				UserPriority;				/* priority class of packet */
	UCHAR				FrameGap;					/* what kind of IFS this packet use */
	UCHAR				MpduReqNum;					/* number of fragments of this frame */

// TODO: shiang-6590, potential to remove
	UCHAR				TxRate;						/* TODO: Obsoleted? Should change to MCS? */
	UCHAR				CipherAlg;					/* cipher alogrithm */
	PCIPHER_KEY			pKey;
	UCHAR				KeyIdx;						/* Indicate the transmit key index */
	UCHAR				OpMode;
	UCHAR				Wcid;						/* The MAC entry associated to this packet */
	/*UCHAR				apidx;*/						/* The interface associated to this packet */
	UCHAR				wdev_idx;				// Used to replace apidx

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	UINT				ApCliIfidx;
	PAPCLI_STRUCT		pApCliEntry;
#endif /* APCLI_SUPPORT */

	BSS_STRUCT *pMbss;

#endif /* CONFIG_AP_SUPPORT */
// TODO: ---End

	UINT32				Flags;						/*See following definitions for detail. */

	/*YOU SHOULD NOT TOUCH IT! Following parameters are used for hardware-depended layer. */
	ULONG				Priv;						/* Hardware specific value saved in here. */
#ifdef MESH_SUPPORT
	PUCHAR				pMeshDA;					/* used to record Mesh DA. */
#endif /* MESH_SUPPORT */

#ifdef TXBF_SUPPORT
	UCHAR				TxSndgPkt; /* 1: sounding 2: NDP sounding */
	UCHAR				TxNDPSndgBW;
	UCHAR				TxNDPSndgMcs;
#endif /* TXBF_SUPPORT */

#if defined(CONFIG_CSO_SUPPORT) || defined(CONFIG_TSO_SUPPORT)
	UCHAR				naf_type;
#endif /* defined(CONFIG_CSO_SUPPORT) || defined(CONFIG_TSO_SUPPORT) */
#ifdef CONFIG_TSO_SUPPORT
	TSO_INFO			tso_info;
#endif /* CONFIG_TSO_SUPPORT */
#ifdef TX_PKT_SG
	PACKET_INFO pkt_info;
#endif /* TX_PKT_SG */

#ifdef HDR_TRANS_SUPPORT
	BOOLEAN				NeedTrans;	/* indicate the packet needs to do hw header translate */
#endif /* HDR_TRANS_SUPPORT */
	UINT32 TxSPriv;
	struct wifi_dev *wdev;
} TX_BLK;


#define fTX_bRtsRequired			0x0001	/* Indicate if need send RTS frame for protection. Not used in RT2860/RT2870. */
#define fTX_bAckRequired			0x0002	/* the packet need ack response */
#define fTX_bPiggyBack			0x0004	/* Legacy device use Piggback or not */
#define fTX_bHTRate				0x0008	/* allow to use HT rate */
#define fTX_bWMM				0x0010	/* QOS Data */
#define fTX_bAllowFrag			0x0020	/* allow to fragment the packet, A-MPDU, A-MSDU, A-Ralink is not allowed to fragment */
#define fTX_bMoreData			0x0040	/* there are more data packets in PowerSave Queue */
#define fTX_bClearEAPFrame		0x0100

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
#define fTX_bApCliPacket			0x0200
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#define fTX_bSwEncrypt			0x0400	/* this packet need to be encrypted by software before TX */

#ifdef UAPSD_SUPPORT
#define fTX_bWMM_UAPSD_EOSP	0x0800	/* Used when UAPSD_SUPPORT */
#endif /* UAPSD_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
#ifdef WDS_SUPPORT
#define fTX_bWDSEntry			0x1000	/* Used when WDS_SUPPORT */
#endif /* WDS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
#ifdef ETH_CONVERT_SUPPORT
#define fTX_bDonglePkt			0x2000	/* Used when ETH_CONVERT_SUPPORT */
#endif /* ETH_CONVERT_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef MESH_SUPPORT
#define fTX_bMeshEntry			0x4000	/* Used when MESH_SUPPORT */
#endif /* MESH_SUPPORT */

#ifdef WAPI_SUPPORT
#define fTX_bWPIDataFrame		0x8000	/* indicate this packet is an WPI data frame, it need to be encrypted by software */
#endif /* WAPI_SUPPORT */

#ifdef CLIENT_WDS
#define fTX_bClientWDSFrame		0x10000
#endif /* CLIENT_WDS */

#if defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT)
#define fTX_bTdlsEntry				0x20000	/* Used when DOT11Z_TDLS_SUPPORT */
#endif /* defined(DOT11Z_TDLS_SUPPORT) || defined(CFG_TDLS_SUPPORT) */

#ifdef WFA_VHT_PF
#define fTX_AmsduInAmpdu		0x40000
#endif /* WFA_VHT_PF */

#define fTX_ForceRate				0x80000

#define TX_BLK_SET_FLAG(_pTxBlk, _flag)		(_pTxBlk->Flags |= _flag)
#define TX_BLK_TEST_FLAG(_pTxBlk, _flag)	(((_pTxBlk->Flags & _flag) == _flag) ? 1 : 0)
#define TX_BLK_CLEAR_FLAG(_pTxBlk, _flag)	(_pTxBlk->Flags &= ~(_flag))


#ifdef DBG_DEQUE
struct deq_log_struct{
	UCHAR que_depth[WMM_NUM_OF_AC];
	UCHAR deq_cnt[WMM_NUM_OF_AC];
	UCHAR deq_round;
};
#endif /* DBG_DEQUE */


typedef struct dequeue_info{
	BOOLEAN inited;
	UCHAR start_q;
	UCHAR end_q;
	CHAR cur_q;
	UCHAR target_wcid;
	UCHAR target_que;
	UCHAR cur_wcid;
	UCHAR q_max_cnt[WMM_QUE_NUM];
	INT pkt_bytes;
	INT pkt_cnt;
	INT deq_pkt_bytes;
	INT deq_pkt_cnt;
	INT status;
	BOOLEAN full_qid[WMM_QUE_NUM];
#ifdef DBG_DEQUE
	deq_log_struct deq_log;
#endif /* DBG_DEQUE */
}DEQUE_INFO;


#ifdef RT_BIG_ENDIAN
/***************************************************************************
  *	Endian conversion related functions
  **************************************************************************/

#ifdef MT_MAC
static inline VOID mt_rmac_d0_endian_change(UINT32 *rxinfo)
{
	(*rxinfo) = SWAP32(*rxinfo);
}


static inline VOID mt_rmac_base_info_endian_change(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *rxinfo)
{
	int i;

	for(i=1; i < 4; i++) {
		*(((UINT32 *)rxinfo) +i) = SWAP32(*(((UINT32 *)rxinfo)+i));
	}
}
#endif /* MT_MAC */


/*
	========================================================================

	Routine Description:
		Endian conversion of Tx/Rx descriptor .

	Arguments:
		pAd 	Pointer to our adapter
		pData			Pointer to Tx/Rx descriptor
		DescriptorType	Direction of the frame

	Return Value:
		None

	Note:
		Call this function when read or update descriptor
	========================================================================
*/
static inline VOID RTMPWIEndianChange(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *pData,
	IN ULONG DescriptorType)
{
	int size;
	int i;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	UINT8 RXWISize = pAd->chipCap.RXWISize;

	size = ((DescriptorType == TYPE_TXWI) ? TXWISize : RXWISize);

	if(DescriptorType == TYPE_TXWI)
	{
		*((UINT32 *)(pData)) = SWAP32(*((UINT32 *)(pData)));		/* Byte 0~3 */
		*((UINT32 *)(pData + 4)) = SWAP32(*((UINT32 *)(pData+4)));	/* Byte 4~7 */
		if (size > 16)
			*((UINT32 *)(pData + 16)) = SWAP32(*((UINT32 *)(pData + 16)));	/* Byte 16~19 */
	}
	else
	{
		for(i=0; i < size/4 ; i++)
			*(((UINT32 *)pData) +i) = SWAP32(*(((UINT32 *)pData)+i));
	}
}

#ifdef MT_MAC
/*
	========================================================================

	Routine Description:
		Endian conversion of MacTxInfo/MacRxInfo descriptor .

	Arguments:
		pAd 	Pointer to our adapter
		pData			Pointer to Tx/Rx descriptor
		DescriptorType	Direction of the frame
		Length 	 	Length of MacTxInfo/MacRxInfo

	Return Value:
		None

	Note:
		Call this function when read or update descriptor
	========================================================================
*/
static inline VOID MTMacInfoEndianChange(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *pData,
	IN ULONG DescriptorType,
	IN UINT16 Length)
{
	int i;
	if(DescriptorType == TYPE_TMACINFO)
	{
		for(i=0; i < Length/4 ; i++)
			*(((UINT32 *)pData) +i) = SWAP32(*(((UINT32 *)pData)+i));
	}
	else /* TYPE_RMACINFO */
	{
		for(i=1; i < Length/4 ; i++) /* i from 1, due to 1st DW had endia change already, so skip it here. */
			*(((UINT32 *)pData) +i) = SWAP32(*(((UINT32 *)pData)+i));
	}
}
#endif /* MT_MAC */

#ifdef RTMP_MAC_PCI
static inline VOID WriteBackToDescriptor(
	IN UCHAR *Dest,
 	IN UCHAR *Src,
	IN BOOLEAN DoEncrypt,
	IN ULONG DescriptorType)
{
	UINT32 *p1, *p2;

	p1 = ((UINT32 *)Dest);
	p2 = ((UINT32 *)Src);

	*p1 = *p2;
	*(p1+2) = *(p2+2);
	*(p1+3) = *(p2+3);
	*(p1+1) = *(p2+1); /* Word 1; this must be written back last */
}
#endif /* RTMP_MAC_PCI */


/*
	========================================================================

	Routine Description:
		Endian conversion of Tx/Rx descriptor .

	Arguments:
		pAd 	Pointer to our adapter
		pData			Pointer to Tx/Rx descriptor
		DescriptorType	Direction of the frame

	Return Value:
		None

	Note:
		Call this function when read or update descriptor
	========================================================================
*/
#ifdef RTMP_MAC_PCI
static inline VOID RTMPDescriptorEndianChange(UCHAR *pData, ULONG DescType)
{
	*((UINT32 *)(pData)) = SWAP32(*((UINT32 *)(pData)));			/* Byte 0~3 */
	*((UINT32 *)(pData + 8)) = SWAP32(*((UINT32 *)(pData+8)));		/* Byte 8~11 */
	*((UINT32 *)(pData +12)) = SWAP32(*((UINT32 *)(pData + 12)));	/* Byte 12~15 */
	*((UINT32 *)(pData + 4)) = SWAP32(*((UINT32 *)(pData + 4)));		/* Byte 4~7, this must be swapped last */
}
#endif /* RTMP_MAC_PCI */

#ifdef RTMP_MAC_USB
static inline VOID RTMPDescriptorEndianChange(UCHAR *pData, ULONG DescType)
{
	*((UINT32 *)(pData)) = SWAP32(*((UINT32 *)(pData)));
}
#endif /* RTMP_MAC_USB */
/*
	========================================================================

	Routine Description:
		Endian conversion of all kinds of 802.11 frames .

	Arguments:
		pAd 	Pointer to our adapter
		pData			Pointer to the 802.11 frame structure
		Dir 			Direction of the frame
		FromRxDoneInt	Caller is from RxDone interrupt

	Return Value:
		None

	Note:
		Call this function when read or update buffer data
	========================================================================
*/
static inline VOID	RTMPFrameEndianChange(
	IN	RTMP_ADAPTER *pAd,
	IN	PUCHAR			pData,
	IN	ULONG			Dir,
	IN	BOOLEAN 		FromRxDoneInt)
{
	PHEADER_802_11 pFrame;
	PUCHAR	pMacHdr;

	/* swab 16 bit fields - Frame Control field */
	if(Dir == DIR_READ)
	{
		*(USHORT *)pData = SWAP16(*(USHORT *)pData);
	}

	pFrame = (PHEADER_802_11) pData;
	pMacHdr = (PUCHAR) pFrame;

	/* swab 16 bit fields - Duration/ID field */
	*(USHORT *)(pMacHdr + 2) = SWAP16(*(USHORT *)(pMacHdr + 2));

	if (pFrame->FC.Type != FC_TYPE_CNTL)
	{
		/* swab 16 bit fields - Sequence Control field */
		*(USHORT *)(pMacHdr + 22) = SWAP16(*(USHORT *)(pMacHdr + 22));
	}

	if(pFrame->FC.Type == FC_TYPE_MGMT)
	{
		switch(pFrame->FC.SubType)
		{
			case SUBTYPE_ASSOC_REQ:
			case SUBTYPE_REASSOC_REQ:
				/* swab 16 bit fields - CapabilityInfo field */
				pMacHdr += sizeof(HEADER_802_11);
				*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

				/* swab 16 bit fields - Listen Interval field */
				pMacHdr += 2;
				*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
				break;

			case SUBTYPE_ASSOC_RSP:
			case SUBTYPE_REASSOC_RSP:
				/* swab 16 bit fields - CapabilityInfo field */
				pMacHdr += sizeof(HEADER_802_11);
				*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

				/* swab 16 bit fields - Status Code field */
				pMacHdr += 2;
				*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

				/* swab 16 bit fields - AID field */
				pMacHdr += 2;
				*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
				break;

			case SUBTYPE_AUTH:
				 /* When the WEP bit is on, don't do the conversion here.
					This is only shared WEP can hit this condition.
					For AP, it shall do conversion after decryption.
					For STA, it shall do conversion before encryption. */
				if (pFrame->FC.Wep == 1)
					break;
				else
				{
					/* swab 16 bit fields - Auth Alg No. field */
					pMacHdr += sizeof(HEADER_802_11);
					*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

					/* swab 16 bit fields - Auth Seq No. field */
					pMacHdr += 2;
					*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

					/* swab 16 bit fields - Status Code field */
					pMacHdr += 2;
					*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
				}
				break;

			case SUBTYPE_BEACON:
			case SUBTYPE_PROBE_RSP:
				/* swab 16 bit fields - BeaconInterval field */
				pMacHdr += (sizeof(HEADER_802_11) + TIMESTAMP_LEN);
				*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);

				/* swab 16 bit fields - CapabilityInfo field */
				pMacHdr += sizeof(USHORT);
				*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
				break;

			case SUBTYPE_DEAUTH:
			case SUBTYPE_DISASSOC:
				/* If the PMF is negotiated, those frames shall be encrypted */
				if(!FromRxDoneInt && pFrame->FC.Wep == 1)
					break;
				else
				{
					/* swab 16 bit fields - Reason code field */
					pMacHdr += sizeof(HEADER_802_11);
					*(USHORT *)pMacHdr = SWAP16(*(USHORT *)pMacHdr);
				}
				break;
		}
	}
	else if( pFrame->FC.Type == FC_TYPE_DATA )
	{
	}
	else if(pFrame->FC.Type == FC_TYPE_CNTL)
	{
		switch(pFrame->FC.SubType)
		{
			case SUBTYPE_BLOCK_ACK_REQ:
				{
					PFRAME_BA_REQ pBAReq = (PFRAME_BA_REQ)pFrame;
					*(USHORT *)(&pBAReq->BARControl) = SWAP16(*(USHORT *)(&pBAReq->BARControl));
					pBAReq->BAStartingSeq.word = SWAP16(pBAReq->BAStartingSeq.word);
				}
				break;
			case SUBTYPE_BLOCK_ACK:
				/* For Block Ack packet, the HT_CONTROL field is in the same offset with Addr3 */
				*(UINT32 *)(&pFrame->Addr3[0]) = SWAP32(*(UINT32 *)(&pFrame->Addr3[0]));
				break;

			case SUBTYPE_ACK:
				/*For ACK packet, the HT_CONTROL field is in the same offset with Addr2 */
				*(UINT32 *)(&pFrame->Addr2[0])=	SWAP32(*(UINT32 *)(&pFrame->Addr2[0]));
				break;
		}
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,("Invalid Frame Type!!!\n"));
	}

	/* swab 16 bit fields - Frame Control */
	if(Dir == DIR_WRITE)
	{
		*(USHORT *)pData = SWAP16(*(USHORT *)pData);
	}
}
#endif /* RT_BIG_ENDIAN */


/***************************************************************************
  *	Other static inline function definitions
  **************************************************************************/
static inline VOID ConvertMulticastIP2MAC(
	IN PUCHAR pIpAddr,
	IN PUCHAR *ppMacAddr,
	IN UINT16 ProtoType)
{
	if (pIpAddr == NULL)
		return;

	if (ppMacAddr == NULL || *ppMacAddr == NULL)
		return;

	switch (ProtoType)
	{
		case ETH_P_IPV6:
/*			memset(*ppMacAddr, 0, MAC_ADDR_LEN); */
			*(*ppMacAddr) = 0x33;
			*(*ppMacAddr + 1) = 0x33;
			*(*ppMacAddr + 2) = pIpAddr[12];
			*(*ppMacAddr + 3) = pIpAddr[13];
			*(*ppMacAddr + 4) = pIpAddr[14];
			*(*ppMacAddr + 5) = pIpAddr[15];
			break;

		case ETH_P_IP:
		default:
/*			memset(*ppMacAddr, 0, MAC_ADDR_LEN); */
			*(*ppMacAddr) = 0x01;
			*(*ppMacAddr + 1) = 0x00;
			*(*ppMacAddr + 2) = 0x5e;
			*(*ppMacAddr + 3) = pIpAddr[1] & 0x7f;
			*(*ppMacAddr + 4) = pIpAddr[2];
			*(*ppMacAddr + 5) = pIpAddr[3];
			break;
	}

	return;
}


char *get_phymode_str(int phy_mode);
char *get_bw_str(int bandwidth);


NDIS_STATUS RTMPAllocTxRxRingMemory(RTMP_ADAPTER *pAd);

#ifdef RESOURCE_PRE_ALLOC
NDIS_STATUS RTMPInitTxRxRingMemory(RTMP_ADAPTER *pAd);
#endif /* RESOURCE_PRE_ALLOC */

INT get_dev_config_idx(RTMP_ADAPTER *pAd);
UCHAR *get_dev_name_prefix(RTMP_ADAPTER *pAd, INT dev_type);

void RTMPPreReadParametersHook(RTMP_ADAPTER *pAd);
NDIS_STATUS	RTMPReadParametersHook(RTMP_ADAPTER *pAd);
NDIS_STATUS	RTMPSetProfileParameters(RTMP_ADAPTER *pAd, RTMP_STRING *pBuffer);

INT RTMPGetKeyParameter(
    IN RTMP_STRING *key,
    OUT RTMP_STRING *dest,
    IN INT destsize,
    IN RTMP_STRING *buffer,
    IN BOOLEAN bTrimSpace);

#ifdef WSC_INCLUDED
VOID rtmp_read_wsc_user_parms_from_file(
	IN	RTMP_ADAPTER *pAd,
	IN 	char *tmpbuf,
	IN 	char *buffer);
#endif/*WSC_INCLUDED*/

#ifdef SINGLE_SKU_V2
NDIS_STATUS	RTMPSetSingleSKUParameters(RTMP_ADAPTER *pAd);

VOID InitSkuRateDiffTable(RTMP_ADAPTER *pAd );
UCHAR GetSkuChannelBasePwr(RTMP_ADAPTER *pAd, UCHAR channel);

VOID UpdateSkuRatePwr(RTMP_ADAPTER *pAd, UCHAR ch, UCHAR bw, CHAR base_pwr);
#endif /* SINGLE_SKU_V2 */


#ifdef RTMP_RF_RW_SUPPORT
VOID RTMP_ReadRF(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			RegID,
	OUT	PUCHAR			pValue1,
	OUT PUCHAR			pValue2,
	IN	UCHAR			BitMask);

VOID RTMP_WriteRF(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			RegID,
	IN	UCHAR			Value,
	IN	UCHAR			BitMask);

NDIS_STATUS	RT30xxWriteRFRegister(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			regID,
	IN	UCHAR			value);

NDIS_STATUS	RT30xxReadRFRegister(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			regID,
	IN	PUCHAR			pValue);

NDIS_STATUS	RT635xWriteRFRegister(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			bank,
	IN	UCHAR			regID,
	IN	UCHAR			value);

NDIS_STATUS	RT635xReadRFRegister(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			bank,
	IN	UCHAR			regID,
	IN	PUCHAR			pValue);

BOOLEAN RTMPAdjustFrequencyOffset(
	IN RTMP_ADAPTER *pAd,
	INOUT PUCHAR pRefFreqOffset);
#endif /* RTMP_RF_RW_SUPPORT */

BOOLEAN RTMPCheckPhyMode(
	IN RTMP_ADAPTER *pAd,
	IN UINT8 BandSupported,
	INOUT UCHAR *pPhyMode);

#ifdef RLT_RF
NDIS_STATUS rlt_rf_write(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR bank,
	IN UCHAR regID,
	IN UCHAR value);

NDIS_STATUS rlt_rf_read(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR bank,
	IN UCHAR regID,
	IN UCHAR *pValue);
#endif /* RLT_RF */


#ifdef RTMP_MAC_PCI
INT NICInitPwrPinCfg(RTMP_ADAPTER *pAd);
#endif /* RTMP_MAC_PCI */

VOID NICInitAsicFromEEPROM(RTMP_ADAPTER *pAd);

NDIS_STATUS NICInitializeAdapter(RTMP_ADAPTER *pAd, BOOLEAN bHardReset);
NDIS_STATUS NICInitializeAsic(RTMP_ADAPTER *pAd, BOOLEAN bHardReset);
INT hif_sys_init(RTMP_ADAPTER *pAd, BOOLEAN bHardReset);
INT hif_sys_exit(RTMP_ADAPTER *pAd);


VOID NICResetFromError(RTMP_ADAPTER *pAd);
VOID NicUpdatFalseCCACounters(RTMP_ADAPTER *pAd);


VOID RTMPRingCleanUp(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR           RingType);

VOID UserCfgExit(RTMP_ADAPTER *pAd);
VOID UserCfgInit(RTMP_ADAPTER *pAd);

int load_patch(RTMP_ADAPTER *ad);
NDIS_STATUS NICLoadFirmware(RTMP_ADAPTER *pAd);
VOID NICEraseFirmware(RTMP_ADAPTER *pAd);

VOID NICUpdateFifoStaCounters(RTMP_ADAPTER *pAd);
NTSTATUS MtCmdNICUpdateRawCounters(RTMP_ADAPTER *pAd, PCmdQElmt CMDQelmt);
VOID NICUpdateRawCounters(RTMP_ADAPTER *pAd);
NTSTATUS MtCmdWtbl2RateTableUpdate(RTMP_ADAPTER *pAd, PCmdQElmt CMDQelmt);

#ifdef CONFIG_STA_SUPPORT
NTSTATUS SetPSMBitHdlr(IN PRTMP_ADAPTER pAd, IN PCmdQElmt CMDQelmt);
NTSTATUS ForceWakeUpHdlr(IN PRTMP_ADAPTER pAd, IN PCmdQElmt CMDQelmt);
NTSTATUS ForceSleepAutoWakeupHdlr(IN PRTMP_ADAPTER pAd, IN PCmdQElmt CMDQelmt);
#endif /* CONFIG_STA_SUPPORT */

UINT32 AsicGetCrcErrCnt(RTMP_ADAPTER *pAd);
UINT32 AsicGetCCACnt(RTMP_ADAPTER *pAd);
UINT32 AsicGetChBusyCnt(RTMP_ADAPTER *pAd, UCHAR ch_idx);

#if defined(RTMP_MAC) || defined(RLT_MAC)
#ifdef FIFO_EXT_SUPPORT
BOOLEAN NicGetMacFifoTxCnt(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
VOID AsicFifoExtSet(RTMP_ADAPTER *pAd);
VOID AsicFifoExtEntryClean(RTMP_ADAPTER * pAd, MAC_TABLE_ENTRY *pEntry);
#endif /* FIFO_EXT_SUPPORT */
#endif /* defined(RTMP_MAC) || defined(RLT_MAC) */

VOID NicResetRawCounters(RTMP_ADAPTER *pAd);
VOID NicGetTxRawCounters(
	IN RTMP_ADAPTER *pAd,
	IN TX_STA_CNT0_STRUC *pStaTxCnt0,
	IN TX_STA_CNT1_STRUC *pStaTxCnt1);

VOID RTMPZeroMemory(VOID *pSrc, ULONG Length);
ULONG RTMPCompareMemory(VOID *pSrc1, VOID *pSrc2, ULONG Length);
VOID RTMPMoveMemory(VOID *pDest, VOID *pSrc, ULONG Length);

VOID AtoH(RTMP_STRING *src, UCHAR *dest, int destlen);
UCHAR BtoH(char ch);

VOID RTMP_TimerListAdd(RTMP_ADAPTER *pAd, VOID *pRsc);
VOID RTMP_TimerListRelease(RTMP_ADAPTER *pAd, VOID *pRsc);
VOID RTMP_AllTimerListRelease(RTMP_ADAPTER *pAd);

VOID RTMPInitTimer(
	IN RTMP_ADAPTER *pAd,
	IN RALINK_TIMER_STRUCT *pTimer,
	IN VOID *pTimerFunc,
	IN VOID *pData,
	IN BOOLEAN Repeat);

VOID RTMPSetTimer(RALINK_TIMER_STRUCT *pTimer, ULONG Value);
VOID RTMPModTimer(RALINK_TIMER_STRUCT *pTimer, ULONG Value);
VOID RTMPCancelTimer(RALINK_TIMER_STRUCT *pTimer, BOOLEAN *pCancelled);
VOID RTMPReleaseTimer(RALINK_TIMER_STRUCT *pTimer, BOOLEAN *pCancelled);

VOID RTMPEnableRxTx(RTMP_ADAPTER *pAd);

VOID AntCfgInit(RTMP_ADAPTER *pAd);

/* */
/* prototype in action.c */
/* */
VOID ActHeaderInit(
    IN RTMP_ADAPTER *pAd,
    IN OUT HEADER_802_11 *pHdr80211,
    IN UCHAR *da,
    IN UCHAR *sa,
    IN UCHAR *bssid);

VOID ActionStateMachineInit(
    IN	RTMP_ADAPTER *pAd,
    IN  STATE_MACHINE *S,
    OUT STATE_MACHINE_FUNC Trans[]);

VOID MlmeADDBAAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM *Elem);

VOID MlmeDELBAAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM *Elem);

VOID SendSMPSAction(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR Wcid,
	IN UCHAR smps);

#ifdef CONFIG_AP_SUPPORT
VOID SendBeaconRequest(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR				Wcid);
#endif /* CONFIG_AP_SUPPORT */


#ifdef CONFIG_STA_SUPPORT
#ifdef QOS_DLS_SUPPORT
VOID DlsParmFill(
	IN RTMP_ADAPTER *pAd,
	IN OUT MLME_DLS_REQ_STRUCT *pDlsReq,
	IN PRT_802_11_DLS pDls,
	IN USHORT reason);
#endif /* QOS_DLS_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef DOT11_N_SUPPORT
VOID RECBATimerTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);

VOID ORIBATimerTimeout(
	IN	RTMP_ADAPTER *pAd);

VOID SendRefreshBAR(
	IN	RTMP_ADAPTER *pAd,
	IN	MAC_TABLE_ENTRY	*pEntry);

#ifdef DOT11N_DRAFT3
VOID RTMP_11N_D3_TimerInit(
	IN RTMP_ADAPTER *pAd);

VOID SendBSS2040CoexistMgmtAction(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR	Wcid,
	IN	UCHAR	apidx,
	IN	UCHAR	InfoReq);

VOID SendNotifyBWActionFrame(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR  Wcid,
	IN UCHAR apidx);

BOOLEAN ChannelSwitchSanityCheck(
	IN	RTMP_ADAPTER *pAd,
	IN    UCHAR  Wcid,
	IN    UCHAR  NewChannel,
	IN    UCHAR  Secondary);

VOID ChannelSwitchAction(
	IN	RTMP_ADAPTER *pAd,
	IN    UCHAR  Wcid,
	IN    UCHAR  Channel,
	IN    UCHAR  Secondary);

ULONG BuildIntolerantChannelRep(
	IN	RTMP_ADAPTER *pAd,
	IN    PUCHAR  pDest);

VOID Update2040CoexistFrameAndNotify(
	IN	RTMP_ADAPTER *pAd,
	IN    UCHAR  Wcid,
	IN	BOOLEAN	bAddIntolerantCha);

VOID Send2040CoexistAction(
	IN	RTMP_ADAPTER *pAd,
	IN    UCHAR  Wcid,
	IN	BOOLEAN	bAddIntolerantCha);

VOID UpdateBssScanParm(
	IN RTMP_ADAPTER *pAd,
	IN OVERLAP_BSS_SCAN_IE APBssScan);
#endif /* DOT11N_DRAFT3 */

INT AsicSetRalinkBurstMode(RTMP_ADAPTER *pAd, BOOLEAN enable);
#endif /* DOT11_N_SUPPORT */

VOID AsicSetTxPreamble(RTMP_ADAPTER *pAd, USHORT TxPreamble);

VOID BarHeaderInit(
	IN	RTMP_ADAPTER *pAd,
	IN OUT PFRAME_BAR pCntlBar,
	IN PUCHAR pDA,
	IN PUCHAR pSA);

VOID InsertActField(
	IN RTMP_ADAPTER *pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 Category,
	IN UINT8 ActCode);

BOOLEAN QosBADataParse(
	IN RTMP_ADAPTER *pAd,
	IN BOOLEAN bAMSDU,
	IN PUCHAR p8023Header,
	IN UCHAR	WCID,
	IN UCHAR	TID,
	IN USHORT Sequence,
	IN UCHAR DataOffset,
	IN USHORT Datasize,
	IN UINT   CurRxIndex);

#ifdef DOT11_N_SUPPORT
BOOLEAN CntlEnqueueForRecv(
    IN	RTMP_ADAPTER *pAd,
	IN ULONG Wcid,
    IN ULONG MsgLen,
	IN PFRAME_BA_REQ pMsg);

VOID BaAutoManSwitch(
	IN	RTMP_ADAPTER *pAd);
#endif /* DOT11_N_SUPPORT */

VOID HTIOTCheck(
	IN	RTMP_ADAPTER *pAd,
	IN    UCHAR     BatRecIdx);


INT rtmp_wdev_idx_find_by_p2p_ifaddr(RTMP_ADAPTER *pAd, UCHAR *ifAddr);
INT rtmp_wdev_idx_reg(RTMP_ADAPTER *pAd, struct wifi_dev *wdev);
INT rtmp_wdev_idx_unreg(RTMP_ADAPTER *pAd, struct wifi_dev *wdev);
INT wdev_init(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UINT wdev_type);
INT wdev_tx_pkts(NDIS_HANDLE dev_hnd, PPNDIS_PACKET pkt_list, UINT pkt_cnt, struct wifi_dev *wdev);



VOID rtmp_ps_init(RTMP_ADAPTER *pAd);
INT rtmp_psDeq_report(RTMP_ADAPTER *pAd,struct dequeue_info *info);
INT rtmp_psDeq_req(RTMP_ADAPTER *pAd);
INT rtmp_ps_enq(RTMP_ADAPTER *pAd, STA_TR_ENTRY *tr_entry);


#ifdef IP_ASSEMBLY

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


typedef struct ip_assemble_data {	
	DL_LIST list;
	QUEUE_HEADER queue;
	INT32 identify;
	INT32 fragSize;
	ULONG createTime;
}IP_ASSEMBLE_DATA;

INT rtmp_IpAssembleHandle(RTMP_ADAPTER *pAd,STA_TR_ENTRY *pTrEntry , PNDIS_PACKET pPacket,UCHAR queIdx,PACKET_INFO packetInfo);
#endif


#ifdef DOT11_N_SUPPORT
VOID RTMP_BASetup(
	IN RTMP_ADAPTER *pAd,
	IN STA_TR_ENTRY *tr_entry,
	IN UINT8 UserPriority);
#endif /* DOT11_N_SUPPORT */

VOID RTMPDeQueuePacket(
	IN RTMP_ADAPTER *pAd,
   	IN BOOLEAN bIntContext,
	IN UCHAR QueIdx,
	IN INT wcid,
	IN INT Max_Tx_Packets);

INT rtmp_tx_swq_init(RTMP_ADAPTER *pAd);
INT rtmp_tx_swq_exit(RTMP_ADAPTER *pAd, UCHAR wcid);

INT rtmp_enq_req(RTMP_ADAPTER *pAd, PNDIS_PACKET pkt, UCHAR qidx, STA_TR_ENTRY *tr_entry, BOOLEAN FlgIsLocked,QUEUE_HEADER *pPktQueue);
INT rtmp_deq_req(RTMP_ADAPTER *pAd, INT tx_cnt, struct dequeue_info *info);
VOID rtmp_tx_swq_dump(RTMP_ADAPTER *pAd, INT qidx);
VOID rtmp_sta_txq_dump(RTMP_ADAPTER *pAd, STA_TR_ENTRY *tr_entry, INT qidx);

INT rtmp_tx_burst_set(RTMP_ADAPTER *pAd);


NDIS_STATUS RTMPFreeTXDRequest(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR           RingType,
	IN  UCHAR           NumberRequired,
	IN 	PUCHAR          FreeNumberIs);

NDIS_STATUS MlmeHardTransmit(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR	QueIdx,
	IN  PNDIS_PACKET    pPacket,
	IN	BOOLEAN			FlgDataQForce,
	IN	BOOLEAN			FlgIsLocked,
	IN	BOOLEAN			FlgIsCheckPS);

NDIS_STATUS MlmeHardTransmitMgmtRing(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR	QueIdx,
	IN  PNDIS_PACKET    pPacket);

#ifdef RTMP_MAC_PCI
NDIS_STATUS MlmeHardTransmitTxRing(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR QueIdx,
	IN PNDIS_PACKET pPacket);

NDIS_STATUS MlmeDataHardTransmit(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR QueIdx,
	IN PNDIS_PACKET pPacket);

VOID ral_write_txd(
	IN RTMP_ADAPTER *pAd,
	IN TXD_STRUC *pTxD,
	IN TX_BLK *txblk,
	IN BOOLEAN bWIV,
	IN UCHAR QSEL);
#endif /* RTMP_MAC_PCI */

USHORT RTMPCalcDuration(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR Rate,
	IN ULONG Size);

VOID write_tmac_info(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *tmac_info,
	IN MAC_TX_INFO *info,
	IN HTTRANSMIT_SETTING *pTransmit);


VOID write_tmac_info_Data(RTMP_ADAPTER *pAd, UCHAR *buf, TX_BLK *pTxBlk);
VOID write_tmac_info_Cache(RTMP_ADAPTER *pAd, UCHAR *buf, TX_BLK *pTxBlk);

VOID RTMPSuspendMsduTransmission(
	IN RTMP_ADAPTER *pAd);

VOID RTMPResumeMsduTransmission(
	IN RTMP_ADAPTER *pAd);

NDIS_STATUS MiniportMMRequest(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR QueIdx,
	IN UCHAR *pData,
	IN UINT Length);

VOID RTMPSendNullFrame(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR TxRate,
	IN BOOLEAN bQosNull,
	IN USHORT PwrMgmt);

INT32 RTMPSendNullFrameAndWaitStatus(
	IN PRTMP_ADAPTER pAd,
	IN USHORT PwrMgmt);

VOID RTMPOffloadPm(RTMP_ADAPTER *pAd, UINT8 ucWlanIdx, UINT8 ucPmNumber, UINT8 ucPmState);

#ifdef __ECOS
NDIS_STATUS RTMP_AllocateNdisPacket_AppandMbuf(
	IN	RTMP_ADAPTER *pAd,
	OUT PNDIS_PACKET   *ppPacket,
	IN	struct mbuf    *pMBuf);

VOID Restore_button_CheckHandler(
	IN	RTMP_ADAPTER *pAd);
#endif /* __ECOS */

#if 0 /* os abl move to rt_os_util.h */
NDIS_STATUS RTMPAllocateNdisPacket(
	IN  RTMP_ADAPTER *pAd,
	IN  PNDIS_PACKET    *pPacket,
	IN  PUCHAR          pHeader,
	IN  UINT            HeaderLen,
	IN  PUCHAR          pData,
	IN  UINT            DataLen);

VOID RTMPFreeNdisPacket(
	IN  RTMP_ADAPTER *pAd,
	IN  PNDIS_PACKET    pPacket);
#endif /* 0 */

BOOLEAN RTMPFreeTXDUponTxDmaDone(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR            QueIdx);

BOOLEAN RTMPCheckEtherType(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET	pPacket,
	IN STA_TR_ENTRY *tr_entry,
	IN struct wifi_dev *wdev,
	OUT PUCHAR pUserPriority,
	OUT PUCHAR pQueIdx,
	OUT PUCHAR pWcid);

BOOLEAN CheckICMPPacket(RTMP_ADAPTER *pAd, UCHAR *pSrcBuf, UINT8 Direction);

VOID RTMPCckBbpTuning(
	IN	RTMP_ADAPTER *pAd,
	IN	UINT			TxRate);

#ifdef RT6352
VOID InitRfPaModeTable(
	IN struct _RTMP_ADAPTER 	*pAd);
#endif /* RT6352 */

/*
	MLME routines
*/

/* Asic/RF/BBP related functions */
VOID AsicGetTxPowerOffset(
	IN PRTMP_ADAPTER 			pAd,
	IN PULONG					TxPwr);

VOID AsicGetAutoAgcOffsetForExternalTxAlc(
	IN PRTMP_ADAPTER 		pAd,
	IN PCHAR 				pDeltaPwr,
	IN PCHAR 				pTotalDeltaPwr,
	IN PCHAR 				pAgcCompensate,
	IN PCHAR 				pDeltaPowerByBbpR1);

VOID AsicExtraPowerOverMAC(RTMP_ADAPTER *pAd);

#ifdef RTMP_TEMPERATURE_COMPENSATION
VOID AsicGetAutoAgcOffsetForTemperatureSensor(
	IN PRTMP_ADAPTER 		pAd,
	IN PCHAR				pDeltaPwr,
	IN PCHAR				pTotalDeltaPwr,
	IN PCHAR				pAgcCompensate,
	IN PCHAR 				pDeltaPowerByBbpR1);

BOOLEAN LoadTempCompTableFromEEPROM(
		IN	struct _RTMP_ADAPTER	*pAd,
		IN	const USHORT		E2P_OFFSET_START,
		IN	const USHORT		E2P_OFFSET_END,
		OUT	PUCHAR			TssiTable,
		IN	const INT			StartIndex,
		IN	const UINT32		TABLE_SIZE);
#endif /* RTMP_TEMPERATURE_COMPENSATION */

#ifdef SINGLE_SKU
VOID GetSingleSkuDeltaPower(
	IN 		RTMP_ADAPTER *pAd,
	IN 		PCHAR 			pTotalDeltaPower,
	INOUT 	PULONG			pSingleSKUTotalDeltaPwr,
	INOUT  	PUCHAR              	pSingleSKUBbpR1Offset);
#endif /* SINGLE_SKU*/

VOID AsicPercentageDeltaPower(
	IN 		PRTMP_ADAPTER 		pAd,
	IN		CHAR				Rssi,
	INOUT	PCHAR				pDeltaPwr,
	INOUT	PCHAR				pDeltaPowerByBbpR1);

VOID AsicCompensatePowerViaBBP(
	IN RTMP_ADAPTER *pAd,
	INOUT CHAR *pTotalDeltaPower);

VOID AsicAdjustTxPower(RTMP_ADAPTER *pAd);

VOID AsicUpdateProtect(
	IN		RTMP_ADAPTER *pAd,
	IN 		USHORT			OperaionMode,
	IN 		UCHAR			SetMask,
	IN		BOOLEAN			bDisableBGProtect,
	IN		BOOLEAN			bNonGFExist);

NTSTATUS MtCmdAsicUpdateProtect(RTMP_ADAPTER *pAd, PCmdQElmt CMDQelmt);

#if defined(RTMP_BBP) && defined(RLT_BBP)
INT AsicRltSetTxStream(RTMP_ADAPTER *pAd, UCHAR opmode, BOOLEAN up);
#define ASIC_RLT_SET_TX_STREAM(_pAd, _opmode, _up) AsicRltSetTxStream(_pAd, _opmode, _up);
#else
#define ASIC_RLT_SET_TX_STREAM(_pAd, _opmode, _up)
#endif
INT AsicSetTxStream(RTMP_ADAPTER *pAd, UINT32 StreamNums);
INT AsicSetRxStream(RTMP_ADAPTER *pAd, UINT32 StreamNums);
INT AsicSetBW(RTMP_ADAPTER *pAd, INT bw);

VOID AsicAntennaSelect(RTMP_ADAPTER *pAd, UCHAR Channel);

VOID AsicResetBBPAgent(RTMP_ADAPTER *pAd);
VOID AsicBBPAdjust(RTMP_ADAPTER *pAd);

VOID AsicSwitchChannel(RTMP_ADAPTER *pAd, UCHAR ch, BOOLEAN bScan);
VOID AsicLockChannel(RTMP_ADAPTER *pAd, UCHAR Channel);

INT AsicSetChannel(RTMP_ADAPTER *pAd, UCHAR ch, UINT8 bw, UINT8 ext_ch, BOOLEAN bScan);

INT AsicSetRxPath(RTMP_ADAPTER *pAd, UINT32 RxPathSel);

#ifdef CONFIG_ATE
INT AsicSetTxTonePower(RTMP_ADAPTER *pAd, INT dec0, INT dec1);
INT AsicSetRfFreqOffset(RTMP_ADAPTER *pAd, UINT32 FreqOffset);
INT AsicSetTSSI(RTMP_ADAPTER *pAd, UINT32 bOnOff, UCHAR WFSelect);
INT AsicSetDPD(RTMP_ADAPTER *pAd, UINT32 bOnOff, UCHAR WFSelect);
#ifdef CONFIG_QA
UINT32 AsicGetRxStat(RTMP_ADAPTER *pAd, UINT type);
#endif /* CONFIG_QA */
INT AsicSetTxToneTest(RTMP_ADAPTER *pAd, UINT32 bOnOff, UCHAR Type);
INT AsicStartContinousTx(RTMP_ADAPTER *pAd, UINT32 PhyMode, UINT32 BW, UINT32 PriCh, UINT32 Mcs, UINT32 WFSel);
INT AsicStopContinousTx(RTMP_ADAPTER *pAd);
#endif /* CONFIG_ATE */


#ifdef CONFIG_STA_SUPPORT
VOID AsicSleepThenAutoWakeup(RTMP_ADAPTER *pAd, USHORT TbttNumToNextWakeUp);
VOID AsicForceWakeup(RTMP_ADAPTER *pAd, BOOLEAN bFromTx);
#endif /* CONFIG_STA_SUPPORT */

INT AsicSetDevMac(RTMP_ADAPTER *pAd, UCHAR *addr, UCHAR omac_idx);
VOID AsicSetBssid(RTMP_ADAPTER *pAd, UCHAR *pBssid, UCHAR curr_bssid_idx);

VOID AsicDelWcidTab(RTMP_ADAPTER *pAd, UCHAR Wcid);

#ifdef MAC_APCLI_SUPPORT
VOID AsicSetApCliBssid(RTMP_ADAPTER *pAd, UCHAR *pBssid, UCHAR index);
#endif /* MAC_APCLI_SUPPORT */

INT AsicSetRxFilter(RTMP_ADAPTER *pAd);

#ifdef DOT11_N_SUPPORT
INT AsicSetRDG(RTMP_ADAPTER *pAd, BOOLEAN bEnable);
#ifdef MT_MAC
INT AsicWtblSetRDG(RTMP_ADAPTER *pAd, BOOLEAN bEnable);
INT AsicUpdateTxOP(RTMP_ADAPTER *pAd, UINT32 ac_num, UINT32 txop_val);
#endif /* MT_MAC */
#endif /* DOT11_N_SUPPORT */

INT AsicSetPreTbtt(RTMP_ADAPTER *pAd, BOOLEAN enable);
INT AsicSetGPTimer(RTMP_ADAPTER *pAd, BOOLEAN enable, UINT32 timeout);
INT AsicSetChBusyStat(RTMP_ADAPTER *pAd, BOOLEAN enable);

INT AsicGetTsfTime(RTMP_ADAPTER *pAd, UINT32 *high_part, UINT32 *low_part);

VOID AsicDisableSync(RTMP_ADAPTER *pAd);
VOID AsicEnableBssSync(RTMP_ADAPTER *pAd, USHORT BeaconPeriod);
VOID AsicEnableApBssSync(RTMP_ADAPTER *pAd, USHORT BeaconPeriod);
VOID AsicEnableIbssSync(RTMP_ADAPTER *pAd);

#define WMM_PARAM_TXOP	0
#define WMM_PARAM_AIFSN	1
#define WMM_PARAM_CWMIN	2
#define WMM_PARAM_CWMAX	3
#define WMM_PARAM_ALL		4

#define WMM_PARAM_AC_0		0
#define WMM_PARAM_AC_1		1
#define WMM_PARAM_AC_2		2
#define WMM_PARAM_AC_3		3
UINT32 AsicGetWmmParam(RTMP_ADAPTER *pAd, UINT32 ac, UINT32 type);
INT AsicSetWmmParam(RTMP_ADAPTER *pAd, UINT ac, UINT type, UINT val);

VOID AsicSetEdcaParm(RTMP_ADAPTER *pAd, PEDCA_PARM pEdcaParm);

#define TX_RTY_CFG_RTY_LIMIT_SHORT	0x1
#define TX_RTY_CFG_RTY_LIMIT_LONG	0x2
INT AsicSetRetryLimit(RTMP_ADAPTER *pAd, UINT32 type, UINT32 limit);

UINT32 AsicGetRetryLimit(RTMP_ADAPTER *pAd, UINT32 type);

VOID AsicSetSlotTime(RTMP_ADAPTER *pAd, BOOLEAN bUseShortSlotTime, UCHAR channel);
INT AsicSetMacMaxLen(RTMP_ADAPTER *pAd);

VOID AsicAddSharedKeyEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR BssIdx,
	IN UCHAR KeyIdx,
	IN PCIPHER_KEY pCipherKey);

VOID AsicRemoveSharedKeyEntry(RTMP_ADAPTER *pAd, UCHAR BssIdx, UCHAR KeyIdx);

VOID AsicUpdateWCIDIVEIV(RTMP_ADAPTER *pAd, USHORT WCID, ULONG uIV, ULONG uEIV);
VOID AsicUpdateRxWCIDTable(RTMP_ADAPTER *pAd, USHORT WCID, UCHAR *pAddr);
VOID AsicUpdateBASession(RTMP_ADAPTER *pAd, UCHAR wcid, UCHAR tid, UINT16 sn, UCHAR basize, BOOLEAN isAdd, INT ses_type);
UINT16 AsicGetTidSn(RTMP_ADAPTER *pAd, UCHAR Wcid, UCHAR Tid);

VOID AsicUpdateWcidAttributeEntry(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR BssIdx,
	IN 	UCHAR KeyIdx,
	IN 	UCHAR CipherAlg,
	IN	UINT8 Wcid,
	IN	UINT8 KeyTabFlag);

VOID AsicAddPairwiseKeyEntry(RTMP_ADAPTER *pAd, UCHAR WCID, PCIPHER_KEY pCipherKey);
VOID AsicRemovePairwiseKeyEntry(RTMP_ADAPTER *pAd, UCHAR Wcid);

#ifdef MT_MAC
VOID CmdProcAddRemoveKey(
	IN  PRTMP_ADAPTER 	pAd,
	IN	UCHAR			AddRemove,
	IN  UCHAR			BssIdx,
	IN	UCHAR			key_idx,
	IN 	UCHAR			Wcid,
	IN	UCHAR			KeyTabFlag,
	IN	PCIPHER_KEY		pCipherKey,
	IN	PUCHAR			PeerAddr);
#endif

#ifdef MT_MAC
VOID AsicSetSMPS(RTMP_ADAPTER *pAd, UCHAR wcid, UCHAR smps);
#endif /* MT_MAC */

#ifdef MCS_LUT_SUPPORT
VOID asic_mcs_lut_update(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
#endif /* MCS_LUT_SUPPORT */

#ifdef MT_MAC
VOID AsicRssiUpdate(RTMP_ADAPTER *pAd);
VOID AsicRcpiReset(RTMP_ADAPTER *pAd, UCHAR ucWcid);

VOID AsicTxCntUpdate(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry, MT_TX_COUNTER *pTxInfo);
#endif /* MT_MAC */

#ifdef CONFIG_AP_SUPPORT
VOID AsicSetMbssMode(RTMP_ADAPTER *pAd, UCHAR NumOfBcns);
#endif /* CONFIG_AP_SUPPORT */

BOOLEAN AsicSendCommandToMcu(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR         Command,
	IN UCHAR         Token,
	IN UCHAR         Arg0,
	IN UCHAR         Arg1,
	IN BOOLEAN in_atomic);

BOOLEAN AsicSendCmdToMcuAndWait(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR Command,
	IN UCHAR Token,
	IN UCHAR Arg0,
	IN UCHAR Arg1,
	IN BOOLEAN in_atomic);

BOOLEAN AsicSendCommandToMcuBBP(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR		 Command,
	IN UCHAR		 Token,
	IN UCHAR		 Arg0,
	IN UCHAR		 Arg1,
	IN BOOLEAN		FlgIsNeedLocked);

#ifdef RTMP_RBUS_SUPPORT
int RtmpAsicSendCommandToSwMcu(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR Command,
	IN UCHAR Token,
	IN UCHAR Arg0,
	IN UCHAR Arg1,
	IN BOOLEAN FlgIsNeedLocked);
#endif /* RTMP_RBUS_SUPPORT */

#ifdef RTMP_MAC_PCI
BOOLEAN AsicCheckCommanOk(RTMP_ADAPTER *pAd, UCHAR Command);
#endif /* RTMP_MAC_PCI */

#ifdef WAPI_SUPPORT
VOID AsicUpdateWAPIPN(
	IN RTMP_ADAPTER *pAd,
	IN USHORT		 WCID,
	IN ULONG         pn_low,
	IN ULONG         pn_high);
#endif /* WAPI_SUPPORT */

#ifdef VCORECAL_SUPPORT
VOID AsicVCORecalibration(RTMP_ADAPTER *pAd);
#endif /* VCORECAL_SUPPORT */

#ifdef STREAM_MODE_SUPPORT
UINT32 StreamModeRegVal(
	IN RTMP_ADAPTER *pAd);

VOID AsicSetStreamMode(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR pMacAddr,
	IN INT chainIdx,
	IN BOOLEAN bEnabled);

VOID RtmpStreamModeInit(
	IN RTMP_ADAPTER *pAd);

/*
	Update the Tx chain address
	Parameters
		pAd: The adapter data structure
		pMacAddress: The MAC address of the peer STA

	Return Value:
		None
*/
VOID AsicUpdateTxChainAddress(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR pMacAddress);

INT Set_StreamMode_Proc(
    IN  RTMP_ADAPTER *pAd,
    IN  RTMP_STRING *arg);

INT Set_StreamModeMac_Proc(
    IN  RTMP_ADAPTER *pAd,
    IN  RTMP_STRING *arg);

INT Set_StreamModeMCS_Proc(
    IN  RTMP_ADAPTER *pAd,
    IN  RTMP_STRING *arg);
#endif /* STREAM_MODE_SUPPORT */

#ifdef WOW_SUPPORT
#ifdef RTMP_MAC_USB
/* For WOW, 8051 MUC send full frame */
VOID AsicWOWSendNullFrame(
    IN RTMP_ADAPTER *pAd,
    IN UCHAR TxRate,
    IN BOOLEAN bQosNull);

VOID AsicLoadWOWFirmware(
    IN RTMP_ADAPTER *pAd,
    IN BOOLEAN WOW);
#endif /* RTMP_MAC_USB */
#endif /* WOW_SUPPORT */

VOID MacAddrRandomBssid(
	IN  RTMP_ADAPTER *pAd,
	OUT PUCHAR pAddr);

VOID MgtMacHeaderInit(
	IN  RTMP_ADAPTER *pAd,
	INOUT HEADER_802_11 *pHdr80211,
	IN UCHAR SubType,
	IN UCHAR ToDs,
	IN UCHAR *pDA,
	IN UCHAR *pSA,
	IN UCHAR *pBssid);

VOID MgtMacHeaderInitExt(
    IN  RTMP_ADAPTER *pAd,
    IN OUT PHEADER_802_11 pHdr80211,
    IN UCHAR SubType,
    IN UCHAR ToDs,
    IN PUCHAR pDA,
    IN PUCHAR pSA,
    IN PUCHAR pBssid);

VOID MlmeRadioOff(
	IN RTMP_ADAPTER *pAd);

VOID MlmeRadioOn(
	IN RTMP_ADAPTER *pAd);


VOID BssTableInit(
	IN BSS_TABLE *Tab);

#ifdef DOT11_N_SUPPORT
VOID BATableInit(
	IN RTMP_ADAPTER *pAd,
    IN BA_TABLE *Tab);

VOID BATableExit(
	IN RTMP_ADAPTER *pAd);
#endif /* DOT11_N_SUPPORT */

ULONG BssTableSearch(
	IN BSS_TABLE *Tab,
	IN PUCHAR pBssid,
	IN UCHAR Channel);

ULONG BssSsidTableSearch(
	IN BSS_TABLE *Tab,
	IN PUCHAR    pBssid,
	IN PUCHAR    pSsid,
	IN UCHAR     SsidLen,
	IN UCHAR     Channel);

ULONG BssTableSearchWithSSID(
	IN BSS_TABLE *Tab,
	IN PUCHAR    Bssid,
	IN PUCHAR    pSsid,
	IN UCHAR     SsidLen,
	IN UCHAR     Channel);

ULONG BssSsidTableSearchBySSID(
	IN BSS_TABLE *Tab,
	IN PUCHAR	 pSsid,
	IN UCHAR	 SsidLen);

VOID BssTableDeleteEntry(
	IN OUT  PBSS_TABLE pTab,
	IN      PUCHAR pBssid,
	IN      UCHAR Channel);

#if 0
ULONG  BssTableSetEntry_Old(
	IN  RTMP_ADAPTER *pAd,
	OUT PBSS_TABLE pTab,
	IN PUCHAR pBssid,
	IN CHAR Ssid[],
	IN UCHAR SsidLen,
	IN UCHAR BssType,
	IN USHORT BeaconPeriod,
	IN CF_PARM *CfParm,
	IN USHORT AtimWin,
	IN USHORT CapabilityInfo,
	IN UCHAR SupRate[],
	IN UCHAR SupRateLen,
	IN UCHAR ExtRate[],
	IN UCHAR ExtRateLen,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN ADD_HT_INFO_IE *pAddHtInfo,	/* AP might use this additional ht info IE */
	IN UCHAR HtCapabilityLen,
	IN UCHAR AddHtInfoLen,
	IN UCHAR NewExtChanOffset,
	IN UCHAR Channel,
	IN CHAR Rssi,
	IN LARGE_INTEGER TimeStamp,
	IN UCHAR CkipFlag,
	IN PEDCA_PARM pEdcaParm,
	IN PQOS_CAPABILITY_PARM pQosCapability,
	IN PQBSS_LOAD_PARM pQbssLoad,
	IN USHORT LengthVIE,
	IN PNDIS_802_11_VARIABLE_IEs pVIE);
#else
ULONG BssTableSetEntry(
	IN RTMP_ADAPTER *pAd,
	OUT BSS_TABLE *Tab,
	IN BCN_IE_LIST *ie_list,
	IN CHAR Rssi,
	IN USHORT LengthVIE,
	IN PNDIS_802_11_VARIABLE_IEs pVIE);
#endif


#ifdef DOT11_N_SUPPORT
VOID BATableInsertEntry(
    IN	RTMP_ADAPTER *pAd,
	IN USHORT Aid,
    IN USHORT		TimeOutValue,
	IN USHORT		StartingSeq,
    IN UCHAR TID,
	IN UCHAR BAWinSize,
	IN UCHAR OriginatorStatus,
    IN BOOLEAN IsRecipient);

#ifdef DOT11N_DRAFT3
VOID Bss2040CoexistTimeOut(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);


VOID  TriEventInit(
	IN	RTMP_ADAPTER *pAd);

INT TriEventTableSetEntry(
	IN	RTMP_ADAPTER *pAd,
	OUT TRIGGER_EVENT_TAB *Tab,
	IN PUCHAR pBssid,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN UCHAR			HtCapabilityLen,
	IN UCHAR			RegClass,
	IN UCHAR ChannelNo);

#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

VOID BssTableSsidSort(
	IN  RTMP_ADAPTER *pAd,
	OUT BSS_TABLE *OutTab,
	IN  CHAR Ssid[],
	IN  UCHAR SsidLen);

VOID  BssTableSortByRssi(
	IN OUT BSS_TABLE *OutTab,
	IN BOOLEAN isInverseOrder);

VOID BssCipherParse(BSS_ENTRY *pBss);

NDIS_STATUS  MlmeQueueInit(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE *Queue);

VOID  MlmeQueueDestroy(
	IN MLME_QUEUE *Queue);

BOOLEAN MlmeEnqueue(
	IN RTMP_ADAPTER *pAd,
	IN ULONG Machine,
	IN ULONG MsgType,
	IN ULONG MsgLen,
	IN VOID *Msg,
	IN ULONG Priv);

BOOLEAN MlmeEnqueueForRecv(
	IN  RTMP_ADAPTER *pAd,
	IN ULONG Wcid,
	IN struct raw_rssi_info *rssi_info,
	IN ULONG MsgLen,
	IN PVOID Msg,
	IN UCHAR OpMode);

#ifdef WSC_INCLUDED
BOOLEAN MlmeEnqueueForWsc(
	IN RTMP_ADAPTER *pAd,
	IN ULONG eventID,
	IN LONG senderID,
	IN ULONG Machine,
	IN ULONG MsgType,
	IN ULONG MsgLen,
	IN VOID *Msg);
#endif /* WSC_INCLUDED */

BOOLEAN MlmeDequeue(
	IN MLME_QUEUE *Queue,
	OUT MLME_QUEUE_ELEM **Elem);

VOID    MlmeRestartStateMachine(
	IN  RTMP_ADAPTER *pAd);

BOOLEAN  MlmeQueueEmpty(
	IN MLME_QUEUE *Queue);

BOOLEAN  MlmeQueueFull(
	IN MLME_QUEUE *Queue,
	IN UCHAR SendId);

BOOLEAN  MsgTypeSubst(
	IN RTMP_ADAPTER *pAd,
	IN PFRAME_802_11 pFrame,
	OUT INT *Machine,
	OUT INT *MsgType);

VOID StateMachineInit(
	IN STATE_MACHINE *Sm,
	IN STATE_MACHINE_FUNC Trans[],
	IN ULONG StNr,
	IN ULONG MsgNr,
	IN STATE_MACHINE_FUNC DefFunc,
	IN ULONG InitState,
	IN ULONG Base);

VOID StateMachineSetAction(
	IN STATE_MACHINE *S,
	IN ULONG St,
	ULONG Msg,
	IN STATE_MACHINE_FUNC F);

VOID StateMachinePerformAction(
	IN  RTMP_ADAPTER *pAd,
	IN STATE_MACHINE *S,
	IN MLME_QUEUE_ELEM *Elem,
	IN ULONG CurrState);

VOID Drop(
	IN  RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID AssocStateMachineInit(
	IN  RTMP_ADAPTER *pAd,
	IN  STATE_MACHINE *Sm,
	OUT STATE_MACHINE_FUNC Trans[]);

VOID ReassocTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID AssocTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID DisassocTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

/*---------------------------------------------- */
VOID MlmeDisassocReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID MlmeAssocReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID MlmeReassocReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID MlmeDisassocReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerAssocRspAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerReassocRspAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerDisassocAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID DisassocTimeoutAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID AssocTimeoutAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID  ReassocTimeoutAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID  Cls3errAction(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR pAddr);

VOID  InvalidStateWhenAssoc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID  InvalidStateWhenReassoc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID InvalidStateWhenDisassociate(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

#ifdef RTMP_MAC_USB
VOID MlmeCntlConfirm(
	IN RTMP_ADAPTER *pAd,
	IN ULONG MsgType,
	IN USHORT Msg);
#endif /* RTMP_MAC_USB */

VOID  ComposePsPoll(
	IN  RTMP_ADAPTER *pAd);

VOID  ComposeNullFrame(
	IN  RTMP_ADAPTER *pAd);

VOID  AssocPostProc(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR pAddr2,
	IN  USHORT CapabilityInfo,
	IN  USHORT Aid,
	IN  UCHAR SupRate[],
	IN  UCHAR SupRateLen,
	IN  UCHAR ExtRate[],
	IN  UCHAR ExtRateLen,
	IN PEDCA_PARM pEdcaParm,
	IN IE_LISTS *ie_list,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN  UCHAR HtCapabilityLen,
	IN ADD_HT_INFO_IE *pAddHtInfo);

VOID AuthStateMachineInit(
	IN  RTMP_ADAPTER *pAd,
	IN PSTATE_MACHINE sm,
	OUT STATE_MACHINE_FUNC Trans[]);

VOID AuthTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID MlmeAuthReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerAuthRspAtSeq2Action(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerAuthRspAtSeq4Action(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID AuthTimeoutAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID Cls2errAction(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR pAddr);

VOID MlmeDeauthReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID InvalidStateWhenAuth(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

/*============================================= */

VOID AuthRspStateMachineInit(
	IN  RTMP_ADAPTER *pAd,
	IN  PSTATE_MACHINE Sm,
	IN  STATE_MACHINE_FUNC Trans[]);

VOID PeerDeauthAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID PeerAuthSimpleRspGenAndSend(
	IN  RTMP_ADAPTER *pAd,
	IN  PHEADER_802_11  pHdr80211,
	IN  USHORT Alg,
	IN  USHORT Seq,
	IN  USHORT Reason,
	IN  USHORT Status);

/* */
/* Private routines in dls.c */
/* */
#ifdef CONFIG_AP_SUPPORT
#ifdef QOS_DLS_SUPPORT
VOID APDLSStateMachineInit(
    IN	RTMP_ADAPTER *pAd,
    IN  STATE_MACHINE		*S,
    OUT STATE_MACHINE_FUNC	Trans[]);

VOID APPeerDlsReqAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM	*Elem);

VOID APPeerDlsRspAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM	*Elem);

VOID APPeerDlsTearDownAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM	*Elem);
#endif /* QOS_DLS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
#ifdef QOS_DLS_SUPPORT
void DlsStateMachineInit(
    IN RTMP_ADAPTER *pAd,
    IN STATE_MACHINE *Sm,
    OUT STATE_MACHINE_FUNC Trans[]);

VOID MlmeDlsReqAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM *Elem);

VOID PeerDlsReqAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM	*Elem);

VOID PeerDlsRspAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM	*Elem);

VOID MlmeDlsTearDownAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM *Elem);

VOID PeerDlsTearDownAction(
    IN RTMP_ADAPTER *pAd,
    IN MLME_QUEUE_ELEM	*Elem);

VOID RTMPCheckDLSTimeOut(
	IN RTMP_ADAPTER *pAd);

BOOLEAN RTMPRcvFrameDLSCheck(
	IN RTMP_ADAPTER *pAd,
	IN PHEADER_802_11 pHeader,
	IN ULONG Len,
	IN RXINFO_STRUC *pRxInfo);

INT	RTMPCheckDLSFrame(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *pDA);

VOID RTMPSendDLSTearDownFrame(
	IN	RTMP_ADAPTER *pAd,
	IN  PUCHAR          pDA);

NDIS_STATUS RTMPSendSTAKeyRequest(
	IN	RTMP_ADAPTER *pAd,
	IN	PUCHAR			pDA);

NDIS_STATUS RTMPSendSTAKeyHandShake(
	IN	RTMP_ADAPTER *pAd,
	IN	PUCHAR			pDA);

VOID DlsTimeoutAction(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

BOOLEAN MlmeDlsReqSanity(
	IN RTMP_ADAPTER *pAd,
    IN VOID *Msg,
    IN ULONG MsgLen,
    OUT PRT_802_11_DLS *pDLS,
    OUT PUSHORT pReason);

INT Set_DlsEntryInfo_Display_Proc(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR arg);

MAC_TABLE_ENTRY *MacTableInsertDlsEntry(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR	pAddr,
	IN  UINT	DlsEntryIdx);

BOOLEAN MacTableDeleteDlsEntry(
	IN RTMP_ADAPTER *pAd,
	IN USHORT wcid,
	IN PUCHAR pAddr);

MAC_TABLE_ENTRY *DlsEntryTableLookup(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR	pAddr,
	IN BOOLEAN	bResetIdelCount);

MAC_TABLE_ENTRY *DlsEntryTableLookupByWcid(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR	wcid,
	IN PUCHAR	pAddr,
	IN BOOLEAN	bResetIdelCount);

INT	Set_DlsAddEntry_Proc(
	IN	RTMP_ADAPTER *pAd,
	IN	RTMP_STRING *arg);

INT	Set_DlsTearDownEntry_Proc(
	IN	RTMP_ADAPTER *pAd,
	IN	RTMP_STRING *arg);
#endif /* QOS_DLS_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef QOS_DLS_SUPPORT
BOOLEAN PeerDlsReqSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *Msg,
    IN ULONG MsgLen,
    OUT PUCHAR pDA,
    OUT PUCHAR pSA,
    OUT USHORT *pCapabilityInfo,
    OUT USHORT *pDlsTimeout,
    OUT UCHAR *pRatesLen,
    OUT UCHAR Rates[],
    OUT UCHAR *pHtCapabilityLen,
    OUT HT_CAPABILITY_IE *pHtCapability);

BOOLEAN PeerDlsRspSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *Msg,
    IN ULONG MsgLen,
    OUT PUCHAR pDA,
    OUT PUCHAR pSA,
    OUT USHORT *pCapabilityInfo,
    OUT USHORT *pStatus,
    OUT UCHAR *pRatesLen,
    OUT UCHAR Rates[],
    OUT UCHAR *pHtCapabilityLen,
    OUT HT_CAPABILITY_IE *pHtCapability);

BOOLEAN PeerDlsTearDownSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *Msg,
    IN ULONG MsgLen,
    OUT PUCHAR pDA,
    OUT PUCHAR pSA,
    OUT USHORT *pReason);
#endif /* QOS_DLS_SUPPORT */

BOOLEAN PeerProbeReqSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *Msg,
    IN ULONG MsgLen,
    OUT PEER_PROBE_REQ_PARAM *Param);

/*======================================== */

VOID SyncStateMachineInit(
	IN  RTMP_ADAPTER *pAd,
	IN  STATE_MACHINE *Sm,
	OUT STATE_MACHINE_FUNC Trans[]);

VOID BeaconTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID ScanTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID MlmeScanReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID InvalidStateWhenScan(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID InvalidStateWhenJoin(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID InvalidStateWhenStart(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerBeacon(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID EnqueueProbeRequest(
	IN RTMP_ADAPTER *pAd);

BOOLEAN ScanRunning(
		IN RTMP_ADAPTER *pAd);
/*========================================= */

VOID MlmeCntlInit(
	IN  RTMP_ADAPTER *pAd,
	IN  STATE_MACHINE *S,
	OUT STATE_MACHINE_FUNC Trans[]);

VOID MlmeCntlMachinePerformAction(
	IN  RTMP_ADAPTER *pAd,
	IN  STATE_MACHINE *S,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlIdleProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlOidScanProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlOidSsidProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM * Elem);

VOID CntlOidRTBssidProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM * Elem);

VOID CntlMlmeRoamingProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM * Elem);

VOID CntlWaitDisassocProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlWaitJoinProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlWaitReassocProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlWaitStartProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlWaitAuthProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlWaitAuthProc2(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID CntlWaitAssocProc(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

#ifdef QOS_DLS_SUPPORT
VOID CntlOidDLSSetupProc(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);
#endif /* QOS_DLS_SUPPORT */

#ifdef DOT11V_WNM_SUPPORT
#ifdef CONFIG_AP_SUPPORT
VOID APPeerWNMBTMQueryAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID APPeerWNMBTMRspAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID APPeerWNMDMSReqAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
VOID MlmeWNMBTMQueryAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID MlmeWNMDMSRspAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID PeerWNMBTMReqAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

#endif /* CONFIG_STA_SUPPORT */
#endif /* DOT11V_WNM_SUPPORT */

VOID LinkUp(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR BssType);

VOID LinkDown(
	IN  RTMP_ADAPTER *pAd,
	IN  BOOLEAN         IsReqFromAP);

VOID IterateOnBssTab(
	IN  RTMP_ADAPTER *pAd);

VOID IterateOnBssTab2(
	IN  RTMP_ADAPTER *pAd);;

VOID JoinParmFill(
	IN  RTMP_ADAPTER *pAd,
	IN  OUT MLME_JOIN_REQ_STRUCT *JoinReq,
	IN  ULONG BssIdx);

VOID AssocParmFill(
	IN  RTMP_ADAPTER *pAd,
	IN OUT MLME_ASSOC_REQ_STRUCT *AssocReq,
	IN  PUCHAR pAddr,
	IN  USHORT CapabilityInfo,
	IN  ULONG Timeout,
	IN  USHORT ListenIntv);

VOID ScanParmFill(
	IN  RTMP_ADAPTER *pAd,
	IN  OUT MLME_SCAN_REQ_STRUCT *ScanReq,
	IN  RTMP_STRING Ssid[],
	IN  UCHAR SsidLen,
	IN  UCHAR BssType,
	IN  UCHAR ScanType);

VOID DisassocParmFill(
	IN  RTMP_ADAPTER *pAd,
	IN  OUT MLME_DISASSOC_REQ_STRUCT *DisassocReq,
	IN  PUCHAR pAddr,
	IN  USHORT Reason);

VOID StartParmFill(
	IN  RTMP_ADAPTER *pAd,
	IN  OUT MLME_START_REQ_STRUCT *StartReq,
	IN  CHAR Ssid[],
	IN  UCHAR SsidLen);

VOID AuthParmFill(
	IN  RTMP_ADAPTER *pAd,
	IN  OUT MLME_AUTH_REQ_STRUCT *AuthReq,
	IN  PUCHAR pAddr,
	IN  USHORT Alg);

VOID EnqueuePsPoll(
	IN  RTMP_ADAPTER *pAd);

VOID EnqueueBeaconFrame(
	IN  RTMP_ADAPTER *pAd);

VOID MlmeJoinReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID MlmeScanReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID MlmeStartReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID MlmeForceJoinReqAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID MlmeForceScanReqAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

VOID ScanTimeoutAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID BeaconTimeoutAtJoinAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerBeaconAtScanAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerBeaconAtJoinAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerBeacon(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID PeerProbeReqAction(
	IN  RTMP_ADAPTER *pAd,
	IN  MLME_QUEUE_ELEM *Elem);

VOID ScanNextChannel(
	IN  RTMP_ADAPTER *pAd,
	IN	UCHAR	OpMode);

#ifdef WIDI_SUPPORT
#ifdef CONFIG_STA_SUPPORT
VOID WidiSendProbeRequest(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR * destMac,
	IN UCHAR ssidLen,
	IN CHAR * ssidStr,
	IN UCHAR * deviceName,
	IN UCHAR * primaryDeviceType,
	IN UCHAR * vendExt,
	IN USHORT vendExtLen,
	IN UCHAR channel);
#endif /* CONFIG_STA_SUPPORT */
#endif /* WIDI_SUPPORT */

BOOLEAN MlmeScanReqSanity(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	OUT UCHAR *BssType,
	OUT CHAR ssid[],
	OUT UCHAR *SsidLen,
	OUT UCHAR *ScanType);


BOOLEAN PeerBeaconAndProbeRspSanity_Old(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	IN  UCHAR MsgChannel,
	OUT PUCHAR pAddr2,
	OUT PUCHAR pBssid,
	OUT CHAR Ssid[],
	OUT UCHAR *pSsidLen,
	OUT UCHAR *pBssType,
	OUT USHORT *pBeaconPeriod,
	OUT UCHAR *pChannel,
	OUT UCHAR *pNewChannel,
	OUT LARGE_INTEGER *pTimestamp,
	OUT CF_PARM *pCfParm,
	OUT USHORT *pAtimWin,
	OUT USHORT *pCapabilityInfo,
	OUT UCHAR *pErp,
	OUT UCHAR *pDtimCount,
	OUT UCHAR *pDtimPeriod,
	OUT UCHAR *pBcastFlag,
	OUT UCHAR *pMessageToMe,
	OUT UCHAR SupRate[],
	OUT UCHAR *pSupRateLen,
	OUT UCHAR ExtRate[],
	OUT UCHAR *pExtRateLen,
	OUT	UCHAR *pCkipFlag,
	OUT	UCHAR *pAironetCellPowerLimit,
	OUT PEDCA_PARM       pEdcaParm,
	OUT PQBSS_LOAD_PARM  pQbssLoad,
	OUT PQOS_CAPABILITY_PARM pQosCapability,
	OUT ULONG *pRalinkIe,
	OUT UCHAR		 *pHtCapabilityLen,
#ifdef CONFIG_STA_SUPPORT
	OUT UCHAR		 *pPreNHtCapabilityLen,
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
    OUT UCHAR 	*pSelReg,
#endif /* NATIVE_WPA_SUPPLICANT_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
	OUT HT_CAPABILITY_IE *pHtCapability,
	OUT EXT_CAP_INFO_ELEMENT *pExtCapInfo,
	OUT UCHAR		 *AddHtInfoLen,
	OUT ADD_HT_INFO_IE *AddHtInfo,
	OUT UCHAR *NewExtChannel,
	OUT USHORT *LengthVIE,
	OUT PNDIS_802_11_VARIABLE_IEs pVIE);


BOOLEAN PeerBeaconAndProbeRspSanity(
	IN RTMP_ADAPTER *pAd,
	IN VOID *Msg,
	IN ULONG MsgLen,
	IN UCHAR  MsgChannel,
	OUT BCN_IE_LIST *ie_list,
	OUT USHORT *LengthVIE,
	OUT PNDIS_802_11_VARIABLE_IEs pVIE,
	IN BOOLEAN bGetDtim);


#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
BOOLEAN PeerBeaconAndProbeRspSanity2(
	IN RTMP_ADAPTER *pAd,
	IN VOID *Msg,
	IN ULONG MsgLen,
	IN OVERLAP_BSS_SCAN_IE *BssScan,
	OUT UCHAR 	*RegClass);
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

BOOLEAN PeerAddBAReqActionSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *pMsg,
    IN ULONG MsgLen,
	OUT PUCHAR pAddr2);

BOOLEAN PeerAddBARspActionSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *pMsg,
    IN ULONG MsgLen);

BOOLEAN PeerDelBAActionSanity(
    IN RTMP_ADAPTER *pAd,
    IN UCHAR Wcid,
    IN VOID *pMsg,
    IN ULONG MsgLen);

BOOLEAN MlmeAssocReqSanity(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	OUT PUCHAR pApAddr,
	OUT USHORT *CapabilityInfo,
	OUT ULONG *Timeout,
	OUT USHORT *ListenIntv);

BOOLEAN MlmeAuthReqSanity(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	OUT PUCHAR pAddr,
	OUT ULONG *Timeout,
	OUT USHORT *Alg);

BOOLEAN MlmeStartReqSanity(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	OUT CHAR Ssid[],
	OUT UCHAR *Ssidlen);

BOOLEAN PeerAuthSanity(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	OUT PUCHAR pAddr,
	OUT USHORT *Alg,
	OUT USHORT *Seq,
	OUT USHORT *Status,
	OUT CHAR ChlgText[]);

BOOLEAN PeerAssocRspSanity(
	IN  RTMP_ADAPTER *pAd,
    IN VOID *pMsg,
	IN  ULONG MsgLen,
	OUT PUCHAR pAddr2,
	OUT USHORT *pCapabilityInfo,
	OUT USHORT *pStatus,
	OUT USHORT *pAid,
	OUT UCHAR SupRate[],
	OUT UCHAR *pSupRateLen,
	OUT UCHAR ExtRate[],
	OUT UCHAR *pExtRateLen,
    OUT HT_CAPABILITY_IE		*pHtCapability,
    OUT ADD_HT_INFO_IE		*pAddHtInfo,	/* AP might use this additional ht info IE */
    OUT UCHAR			*pHtCapabilityLen,
    OUT UCHAR			*pAddHtInfoLen,
    OUT UCHAR			*pNewExtChannelOffset,
	OUT PEDCA_PARM pEdcaParm,
	OUT EXT_CAP_INFO_ELEMENT *pExtCapInfo,
	OUT UCHAR *pCkipFlag,
	OUT IE_LISTS *ie_list);

BOOLEAN PeerDisassocSanity(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	OUT PUCHAR pAddr2,
	OUT USHORT *Reason);

BOOLEAN PeerDeauthSanity(
	IN  RTMP_ADAPTER *pAd,
	IN  VOID *Msg,
	IN  ULONG MsgLen,
	OUT PUCHAR pAddr1,
	OUT PUCHAR pAddr2,
	OUT PUCHAR pAddr3,
	OUT USHORT *Reason);

BOOLEAN GetTimBit(
	IN  CHAR *Ptr,
	IN  USHORT Aid,
	OUT UCHAR *TimLen,
	OUT UCHAR *BcastFlag,
	OUT UCHAR *DtimCount,
	OUT UCHAR *DtimPeriod,
	OUT UCHAR *MessageToMe);

UCHAR ChannelSanity(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR channel);

NDIS_802_11_NETWORK_TYPE NetworkTypeInUseSanity(
	IN BSS_ENTRY *pBss);

BOOLEAN MlmeDelBAReqSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *Msg,
    IN ULONG MsgLen);

BOOLEAN MlmeAddBAReqSanity(
    IN RTMP_ADAPTER *pAd,
    IN VOID *Msg,
    IN ULONG MsgLen,
    OUT PUCHAR pAddr2);

ULONG MakeOutgoingFrame(
	OUT UCHAR *Buffer,
	OUT ULONG *Length, ...);

UCHAR RandomByte(
	IN  RTMP_ADAPTER *pAd);

UCHAR RandomByte2(
	IN  RTMP_ADAPTER *pAd);

VOID AsicUpdateAutoFallBackTable(RTMP_ADAPTER *pAd, UCHAR *pTxRate);
INT AsicSetAutoFallBack(RTMP_ADAPTER *pAd, BOOLEAN enable);
INT AsicAutoFallbackInit(RTMP_ADAPTER *pAd);


#if defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) || defined(RT5592)
VOID AsicCheckForHwRecovery(
	IN RTMP_ADAPTER *pAd) ;
#endif /* defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) || defined(RT5592) */

VOID  MlmePeriodicExecTimer(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

NTSTATUS  MlmePeriodicExec(IN PRTMP_ADAPTER pAd, IN PCmdQElmt CMDQelmt);

VOID LinkDownExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID LinkUpExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID STAMlmePeriodicExec(
	RTMP_ADAPTER *pAd);

VOID MlmeAutoScan(
	IN RTMP_ADAPTER *pAd);

VOID MlmeAutoReconnectLastSSID(
	IN RTMP_ADAPTER *pAd);

BOOLEAN MlmeValidateSSID(
	IN PUCHAR pSsid,
	IN UCHAR  SsidLen);

VOID MlmeCheckForRoaming(
	IN RTMP_ADAPTER *pAd,
	IN ULONG    Now32);

BOOLEAN MlmeCheckForFastRoaming(
	IN  RTMP_ADAPTER *pAd);

#ifdef TXBF_SUPPORT
BOOLEAN MlmeTxBfAllowed(
	IN PRTMP_ADAPTER 		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN struct _RTMP_RA_LEGACY_TB *pTxRate);
#endif /* TXBF_SUPPORT */

VOID MlmeCalculateChannelQuality(
	IN RTMP_ADAPTER *pAd,
	IN PMAC_TABLE_ENTRY pMacEntry,
	IN ULONG Now);

VOID MlmeCheckPsmChange(
	IN RTMP_ADAPTER *pAd,
	IN ULONG    Now32);

VOID MlmeSetPsmBit(
	IN RTMP_ADAPTER *pAd,
	IN USHORT psm);

VOID RTMPSetEnterPsmNullBit(IN PPWR_MGMT_STRUCT pPwrMgmt);

VOID RTMPClearEnterPsmNullBit(IN PPWR_MGMT_STRUCT pPwrMgmt);

BOOLEAN RTMPEnterPsmNullBitStatus(IN PPWR_MGMT_STRUCT pPwrMgmt);


VOID MlmeSetTxPreamble(
	IN RTMP_ADAPTER *pAd,
	IN USHORT TxPreamble);

VOID UpdateBasicRateBitmap(
	IN	RTMP_ADAPTER *pAd);

VOID MlmeUpdateTxRates(
	IN RTMP_ADAPTER *pAd,
	IN 	BOOLEAN		 	bLinkUp,
	IN	UCHAR			apidx);

#ifdef DOT11_N_SUPPORT
VOID MlmeUpdateHtTxRates(
	IN PRTMP_ADAPTER 		pAd,
	IN	UCHAR				apidx);
#endif /* DOT11_N_SUPPORT */

VOID    RTMPCheckRates(
	IN      RTMP_ADAPTER *pAd,
	IN OUT  UCHAR           SupRate[],
	IN OUT  UCHAR           *SupRateLen);

BOOLEAN RTMPCheckHt(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR Wcid,
	INOUT HT_CAPABILITY_IE *pHtCapability,
	INOUT ADD_HT_INFO_IE *pAddHtInfo);

#ifdef DOT11_VHT_AC
BOOLEAN RTMPCheckVht(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR Wcid,
	IN VHT_CAP_IE *vht_cap,
	IN VHT_OP_IE *vht_op);
#endif /* DOT11_VHT_AC */

VOID RTMPUpdateMlmeRate(
	IN RTMP_ADAPTER *pAd);

CHAR RTMPMaxNoise(
	IN RTMP_ADAPTER	* pAd,
	IN CHAR		Noise0,
	IN CHAR		Noise1,
	IN CHAR		Noise2);

CHAR RTMPMaxRssi(
	IN RTMP_ADAPTER *pAd,
	IN CHAR				Rssi0,
	IN CHAR				Rssi1,
	IN CHAR				Rssi2);
CHAR RTMPMinRssi(
	IN RTMP_ADAPTER *pAd,
	IN CHAR Rssi0,
	IN CHAR Rssi1,
	IN CHAR Rssi2);
CHAR RTMPAvgRssi(
        IN RTMP_ADAPTER *pAd,
        IN RSSI_SAMPLE		*pRssi);


CHAR RTMPMinSnr(
	IN RTMP_ADAPTER *pAd,
	IN CHAR				Snr0,
	IN CHAR				Snr1);

VOID AsicSetRxAnt(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR			Ant);

#ifdef MICROWAVE_OVEN_SUPPORT
INT Set_MO_FalseCCATh_Proc(
	IN	RTMP_ADAPTER *pAd,
	IN	RTMP_STRING *arg);

VOID AsicMeasureFalseCCA(
	IN RTMP_ADAPTER *pAd
);

VOID AsicMitigateMicrowave(
	IN RTMP_ADAPTER *pAd
);
#endif /* MICROWAVE_OVEN_SUPPORT */

#ifdef RTMP_EFUSE_SUPPORT
INT set_eFuseGetFreeBlockCount_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_eFusedump_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_eFuseLoadFromBin_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

UCHAR eFuseReadRegisters(RTMP_ADAPTER *pAd, USHORT Offset, UINT16 Length, UINT16* pData);
VOID EfusePhysicalReadRegisters(RTMP_ADAPTER *pAd, USHORT Offset, USHORT Length, USHORT* pData);

int RtmpEfuseSupportCheck(RTMP_ADAPTER *pAd);

#ifdef CONFIG_ATE
INT Set_LoadEepromBufferFromEfuse_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_eFuseBufferModeWriteBack_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_BinModeWriteBack_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* CONFIG_ATE */
#endif /* RTMP_EFUSE_SUPPORT */

#ifdef RT33xx
VOID RT33xxLoadRFNormalModeSetup(RTMP_ADAPTER *pAd);
VOID RT33xxLoadRFSleepModeSetup(RTMP_ADAPTER *pAd);
VOID RT33xxReverseRFSleepModeSetup(RTMP_ADAPTER *pAd, BOOLEAN FlgIsInitState);

#ifdef RT3370
VOID NICInitRT3370RFRegisters(RTMP_ADAPTER *pAd);
#endif /* RT3070 */

#ifdef RT3390
VOID NICInitRT3390RFRegisters(RTMP_ADAPTER *pAd);
#endif /* RT3090 */

VOID RT33xxHaltAction(RTMP_ADAPTER *pAd);
VOID RT33xxSetRxAnt(RTMP_ADAPTER *pAd, UCHAR Ant);

#endif /* RT33xx */

#if defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392)
VOID NICInitRT5390RFRegisters(RTMP_ADAPTER *pAd);

#if 0 /* Move to chipOps.AsicBbpInit hook function (NICInitRT5390BbpRegisters) */
 VOID RT5390_PostBBPInitialization(
	IN RTMP_ADAPTER *pAd);
#endif

VOID RT5390SetRxAnt(RTMP_ADAPTER *pAd, UCHAR Ant);
#endif /* defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) */

#ifdef RT305x
VOID NICInitRT305xRFRegisters(RTMP_ADAPTER *pAd);
#endif /* RT305x */

VOID AsicEvaluateRxAnt(RTMP_ADAPTER *pAd);

VOID AsicRxAntEvalTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID APSDPeriodicExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID RTMPSetPiggyBack(RTMP_ADAPTER *pAd, BOOLEAN bPiggyBack);

BOOLEAN RTMPCheckEntryEnableAutoRateSwitch(
	IN RTMP_ADAPTER *pAd,
	IN PMAC_TABLE_ENTRY	pEntry);

UCHAR RTMPStaFixedTxMode(
	IN RTMP_ADAPTER *pAd,
	IN PMAC_TABLE_ENTRY	pEntry);

VOID RTMPUpdateLegacyTxSetting(
	IN UCHAR fixed_tx_mode,
	IN PMAC_TABLE_ENTRY	pEntry);

BOOLEAN RTMPAutoRateSwitchCheck(RTMP_ADAPTER *pAd);

#ifdef RT5390
VOID RTMPBtWiFiSwitch(RTMP_ADAPTER *pAd, UCHAR BTWiFiSwitchMode);
VOID RTMPBtWiFiDiversity(RTMP_ADAPTER *pAd, UCHAR BTMode);
#endif /* RT5390 */

#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_FREQ_CALIBRATION_SUPPORT
VOID InitFrequencyCalibration(RTMP_ADAPTER *pAd);
VOID StopFrequencyCalibration(RTMP_ADAPTER *pAd);
VOID FrequencyCalibration(RTMP_ADAPTER *pAd);
CHAR GetFrequencyOffset(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk);
#endif /* RTMP_FREQ_CALIBRATION_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef RTMP_TEMPERATURE_COMPENSATION
VOID InitLookupTable(RTMP_ADAPTER *pAd);
#endif /* RTMP_TEMPERATURE_COMPENSATION */

VOID MlmeHalt(RTMP_ADAPTER *pAd);
NDIS_STATUS MlmeInit(RTMP_ADAPTER *pAd);

VOID MlmeResetRalinkCounters(RTMP_ADAPTER *pAd);

VOID BuildChannelList(RTMP_ADAPTER *pAd);
UCHAR FirstChannel(RTMP_ADAPTER *pAd);
UCHAR NextChannel(RTMP_ADAPTER *pAd, UCHAR channel);

VOID ChangeToCellPowerLimit(RTMP_ADAPTER *pAd, UCHAR AironetCellPowerLimit);


VOID    RTMPInitMICEngine(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR          pKey,
	IN  PUCHAR          pDA,
	IN  PUCHAR          pSA,
	IN  UCHAR           UserPriority,
	IN  PUCHAR          pMICKey);

BOOLEAN RTMPTkipCompareMICValue(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR          pSrc,
	IN  PUCHAR          pDA,
	IN  PUCHAR          pSA,
	IN  PUCHAR          pMICKey,
	IN	UCHAR			UserPriority,
	IN  UINT            Len);

VOID    RTMPCalculateMICValue(
	IN  RTMP_ADAPTER *pAd,
	IN  PNDIS_PACKET    pPacket,
	IN  PUCHAR pEncap,
	IN  PCIPHER_KEY pKey,
	IN	UCHAR apidx);

VOID    RTMPTkipAppendByte( TKIP_KEY_INFO *pTkip, UCHAR uChar);
VOID    RTMPTkipAppend(TKIP_KEY_INFO *pTkip, UCHAR *pSrc, UINT nBytes);
VOID RTMPTkipGetMIC(TKIP_KEY_INFO *pTkip);


INT RT_CfgSetCountryRegion(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg,
	IN INT band);

INT RT_CfgSetWirelessMode(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

RT_802_11_PHY_MODE wmode_2_cfgmode(UCHAR wmode);
UCHAR cfgmode_2_wmode(UCHAR cfg_mode);
UCHAR *wmode_2_str(UCHAR wmode);

#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
INT RT_CfgSetMbssWirelessMode(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

INT RT_CfgSetShortSlot(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	RT_CfgSetWepKey(
	IN	RTMP_ADAPTER *pAd,
	IN	RTMP_STRING *keyString,
	IN	CIPHER_KEY		*pSharedKey,
	IN	INT				keyIdx);

INT RT_CfgSetWPAPSKKey(
	IN RTMP_ADAPTER	*pAd,
	IN RTMP_STRING *keyString,
	IN INT			keyStringLen,
	IN UCHAR		*pHashStr,
	IN INT			hashStrLen,
	OUT PUCHAR		pPMKBuf);

INT	RT_CfgSetFixedTxPhyMode(RTMP_STRING *arg);
INT	RT_CfgSetMacAddress(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	RT_CfgSetTxMCSProc(RTMP_STRING *arg, BOOLEAN *pAutoRate);
INT	RT_CfgSetAutoFallBack(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef WSC_INCLUDED
INT	RT_CfgSetWscPinCode(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *pPinCodeStr,
	OUT PWSC_CTRL   pWscControl);
#endif /* WSC_INCLUDED */

INT	Set_Antenna_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef RT5350
VOID RT5350SetRxAnt(RTMP_ADAPTER *pAd, UCHAR Ant);

INT Set_Hw_Antenna_Div_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* RT5350 */

#ifdef RT6352
INT Set_RfBankSel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef RTMP_TEMPERATURE_CALIBRATION
INT Set_TemperatureCAL_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* RTMP_TEMPERATURE_CALIBRATION */
#endif /* RT6352 */

#ifdef HW_TX_RATE_LOOKUP_SUPPORT
INT Set_HwTxRateLookUp_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* HW_TX_RATE_LOOKUP_SUPPORT */

#ifdef MULTI_MAC_ADDR_EXT_SUPPORT
INT Set_EnMultiMacAddrExt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_MultiMacAddrExt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* MULTI_MAC_ADDR_EXT_SUPPORT */

INT set_tssi_enable(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef CONFIG_WIFI_TEST
INT set_pbf_loopback(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_pbf_rx_drop(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif

#ifdef MT_MAC
INT set_get_fid(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_fw_log(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT SetManualTxOP(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_themal_sensor(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_rx_pspoll_filter_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT SetManualTxOPThreshold(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT SetManualTxOPUpBound(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT SetManualTxOPLowBound(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif

#ifdef RTMP_MAC_PCI
INT Set_PDMAWatchDog_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif
INT SetPSEWatchDog_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

NDIS_STATUS RTMPWPARemoveKeyProc(
	IN  RTMP_ADAPTER *pAd,
	IN  PVOID           pBuf);

VOID RTMPWPARemoveAllKeys(
	IN  RTMP_ADAPTER *pAd);

BOOLEAN RTMPCheckStrPrintAble(
    IN  CHAR *pInPutStr,
    IN  UCHAR strLen);

VOID RTMPSetPhyMode(
	IN  RTMP_ADAPTER *pAd,
	IN  ULONG phymode);

VOID RTMPUpdateHTIE(
	IN	RT_HT_CAPABILITY	*pRtHt,
	IN		UCHAR				*pMcsSet,
	OUT		HT_CAPABILITY_IE *pHtCapability,
	OUT		ADD_HT_INFO_IE		*pAddHtInfo);

VOID RTMPAddWcidAttributeEntry(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			BssIdx,
	IN 	UCHAR		 	KeyIdx,
	IN 	UCHAR		 	CipherAlg,
	IN 	MAC_TABLE_ENTRY *pEntry);

RTMP_STRING *GetEncryptType(CHAR enc);
RTMP_STRING *GetAuthMode(CHAR auth);

VOID set_sta_ra_cap(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *ent, ULONG ra_ie);

#ifdef DOT11_N_SUPPORT
VOID set_sta_ht_cap(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *ent, HT_CAPABILITY_IE *ht_ie);

VOID RTMPSetHT(RTMP_ADAPTER *pAd, OID_SET_HT_PHYMODE *pHTPhyMode);
VOID RTMPSetIndividualHT(RTMP_ADAPTER *pAd, UCHAR apidx);

UCHAR get_cent_ch_by_htinfo(
	RTMP_ADAPTER *pAd,
	ADD_HT_INFO_IE *ht_op,
	HT_CAPABILITY_IE *ht_cap);

UCHAR RTMP_GetPrimaryCh(
	RTMP_ADAPTER *pAd, 
	UCHAR ch);

INT get_ht_cent_ch(RTMP_ADAPTER *pAd, UINT8 *rf_bw, UINT8 *ext_ch);
INT ht_mode_adjust(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry, HT_CAPABILITY_IE *peer, RT_HT_CAPABILITY *my);
INT set_ht_fixed_mcs(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry, UCHAR fixed_mcs, UCHAR mcs_bound);
INT get_ht_max_mcs(RTMP_ADAPTER *pAd, UCHAR *desire_mcs, UCHAR *cap_mcs);
#endif /* DOT11_N_SUPPORT */

VOID RTMPDisableDesiredHtInfo(
	IN	RTMP_ADAPTER *pAd);

#ifdef SYSTEM_LOG_SUPPORT
VOID RtmpDrvSendWirelessEvent(
	IN	VOID			*pAdSrc,
	IN	USHORT			Event_flag,
	IN	PUCHAR 			pAddr,
	IN  UCHAR			wdev_idx,
	IN	CHAR			Rssi);
#else
#define RtmpDrvSendWirelessEvent(_pAd, _Event_flag, _pAddr, wdev_idx, _Rssi)
#endif /* SYSTEM_LOG_SUPPORT */

CHAR ConvertToRssi(
	IN RTMP_ADAPTER *pAd,
	IN struct raw_rssi_info *rssi_info,
	IN UCHAR rssi_idx);

CHAR ConvertToSnr(RTMP_ADAPTER *pAd, UCHAR Snr);

#ifdef DOT11N_DRAFT3
VOID BuildEffectedChannelList(
	IN RTMP_ADAPTER *pAd);

#if 0
VOID AddToRepositList(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR pBssid,
	IN CHAR Ssid[],
	IN UCHAR SsidLen,
	IN UCHAR BssType,
	IN USHORT BeaconPeriod,
	IN USHORT AtimWin,
	IN USHORT CapabilityInfo,
	IN UCHAR SupRate[],
	IN UCHAR SupRateLen,
	IN UCHAR ExtRate[],
	IN UCHAR ExtRateLen,
	IN UCHAR ChannelNo,
	IN CHAR Rssi,
	IN LARGE_INTEGER TimeStamp,
	IN USHORT LengthVIE,
	IN PNDIS_802_11_VARIABLE_IEs pVIE);
#endif

VOID DeleteEffectedChannelList(RTMP_ADAPTER *pAd);

VOID CntlChannelWidth(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR			PrimaryChannel,
	IN UCHAR			CentralChannel,
	IN UCHAR			ChannelWidth,
	IN UCHAR			SecondaryChannelOffset);

#endif /* DOT11N_DRAFT3 */


VOID APAsicEvaluateRxAnt(
	IN RTMP_ADAPTER *pAd);

#ifdef ANT_DIVERSITY_SUPPORT
VOID	APAsicAntennaAvg(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR	              AntSelect,
	IN	SHORT	              *RssiAvg);
#endif /* ANT_DIVERSITY_SUPPORT */

VOID APAsicRxAntEvalTimeout(RTMP_ADAPTER *pAd);


VOID RTMPGetTxTscFromAsic(RTMP_ADAPTER *pAd, UCHAR apidx, UCHAR *pTxTsc);

MAC_TABLE_ENTRY *PACInquiry(RTMP_ADAPTER *pAd, UCHAR Wcid);

UINT APValidateRSNIE(
	IN RTMP_ADAPTER *pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN PUCHAR			pRsnIe,
	IN UCHAR			rsnie_len);

VOID HandleCounterMeasure(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY  *pEntry);

VOID WPAStart4WayHS(
	IN  RTMP_ADAPTER *pAd,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN	ULONG			TimeInterval);

VOID WPAStart2WayGroupHS(
	IN  RTMP_ADAPTER *pAd,
	IN  MAC_TABLE_ENTRY *pEntry);

VOID PeerPairMsg1Action(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY  *pEntry,
	IN MLME_QUEUE_ELEM *Elem);

VOID PeerPairMsg2Action(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY  *pEntry,
	IN MLME_QUEUE_ELEM *Elem);

VOID PeerPairMsg3Action(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY  *pEntry,
	IN MLME_QUEUE_ELEM *Elem);

VOID PeerPairMsg4Action(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY  *pEntry,
	IN MLME_QUEUE_ELEM *Elem);

VOID WPAPairMsg3Retry(
    IN PRTMP_ADAPTER    pAd, 
    IN MAC_TABLE_ENTRY  *pEntry,
    IN ULONG			TimeInterval
);

VOID PeerGroupMsg1Action(
	IN  RTMP_ADAPTER *pAd,
	IN  PMAC_TABLE_ENTRY pEntry,
    IN  MLME_QUEUE_ELEM  *Elem);

VOID PeerGroupMsg2Action(
	IN  RTMP_ADAPTER *pAd,
	IN  PMAC_TABLE_ENTRY pEntry,
	IN  VOID             *Msg,
	IN  UINT             MsgLen);

VOID CMTimerExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID WPARetryExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

#ifdef TXBF_SUPPORT
VOID eTxBfProbeTimerExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);
#endif /* TXBF_SUPPORT */

VOID EnqueueStartForPSKExec(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);

VOID RTMPHandleSTAKey(
    IN RTMP_ADAPTER *pAd,
    IN MAC_TABLE_ENTRY  *pEntry,
    IN MLME_QUEUE_ELEM  *Elem);

VOID MlmeDeAuthAction(
	IN  RTMP_ADAPTER *pAd,
	IN  PMAC_TABLE_ENTRY pEntry,
	IN  USHORT           Reason,
	IN  BOOLEAN          bDataFrameFirst);

#ifdef DOT11W_PMF_SUPPORT
VOID PMF_SAQueryTimeOut(
        IN PVOID SystemSpecific1,
        IN PVOID FunctionContext,
        IN PVOID SystemSpecific2,
        IN PVOID SystemSpecific3);

VOID PMF_SAQueryConfirmTimeOut(
        IN PVOID SystemSpecific1,
        IN PVOID FunctionContext,
        IN PVOID SystemSpecific2,
        IN PVOID SystemSpecific3);
#endif /* DOT11W_PMF_SUPPORT */

VOID GREKEYPeriodicExec(
	IN  PVOID   SystemSpecific1,
	IN  PVOID   FunctionContext,
	IN  PVOID   SystemSpecific2,
	IN  PVOID   SystemSpecific3);

VOID AES_128_CMAC(
	IN	PUCHAR	key,
	IN	PUCHAR	input,
	IN	INT		len,
	OUT	PUCHAR	mac);

#ifdef DOT1X_SUPPORT
VOID    WpaSend(
    IN  RTMP_ADAPTER *pAd,
    IN  PUCHAR          pPacket,
    IN  ULONG           Len);

VOID RTMPAddPMKIDCache(
	IN  RTMP_ADAPTER *pAd,
	IN	INT						apidx,
	IN	PUCHAR				pAddr,
	IN	UCHAR					*PMKID,
	IN	UCHAR					*PMK);

INT RTMPSearchPMKIDCache(
	IN  RTMP_ADAPTER *pAd,
	IN	INT				apidx,
	IN	PUCHAR		pAddr);

VOID RTMPDeletePMKIDCache(
	IN  RTMP_ADAPTER *pAd,
	IN	INT				apidx,
	IN  INT				idx);

VOID RTMPMaintainPMKIDCache(
	IN  RTMP_ADAPTER *pAd);
#else
#define RTMPMaintainPMKIDCache(_pAd)
#endif /* DOT1X_SUPPORT */

#ifdef RESOURCE_PRE_ALLOC
VOID RTMPResetTxRxRingMemory(
	IN  RTMP_ADAPTER   *pAd);
#endif /* RESOURCE_PRE_ALLOC */

VOID RTMPFreeTxRxRingMemory(
    IN  RTMP_ADAPTER *pAd);

BOOLEAN RTMP_FillTxBlkInfo(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk);

 void announce_802_3_packet(
	IN	VOID			*pAdSrc,
	IN	PNDIS_PACKET	pPacket,
	IN	UCHAR			OpMode);

#ifdef DOT11_N_SUPPORT
UINT BA_Reorder_AMSDU_Annnounce(
	IN	RTMP_ADAPTER *pAd,
	IN	PNDIS_PACKET	pPacket,
	IN	UCHAR			OpMode);
#endif /* DOT11_N_SUPPORT */

PNET_DEV get_netdev_from_bssid(RTMP_ADAPTER *pAd, UCHAR FromWhichBSSID);

#ifdef DOT11_N_SUPPORT
void ba_flush_reordering_timeout_mpdus(
	IN RTMP_ADAPTER *pAd,
	IN PBA_REC_ENTRY	pBAEntry,
	IN ULONG			Now32);


VOID BAOriSessionSetUp(
			IN RTMP_ADAPTER *pAd,
			IN MAC_TABLE_ENTRY	*pEntry,
			IN UCHAR			TID,
			IN USHORT			TimeOut,
			IN ULONG			DelayTime,
			IN BOOLEAN		isForced);

VOID BASessionTearDownALL(
	IN OUT	RTMP_ADAPTER *pAd,
	IN		UCHAR Wcid);

VOID BAOriSessionTearDown(
	IN OUT	RTMP_ADAPTER *pAd,
	IN		UCHAR			Wcid,
	IN		UCHAR			TID,
	IN		BOOLEAN			bPassive,
	IN		BOOLEAN			bForceSend);

VOID BARecSessionTearDown(
	IN OUT	RTMP_ADAPTER *pAd,
	IN		UCHAR			Wcid,
	IN		UCHAR			TID,
	IN		BOOLEAN			bPassive);
#endif /* DOT11_N_SUPPORT */

BOOLEAN ba_reordering_resource_init(RTMP_ADAPTER *pAd, int num);
void ba_reordering_resource_release(RTMP_ADAPTER *pAd);

INT ComputeChecksum(
	IN UINT PIN);

UINT GenerateWpsPinCode(
	IN	RTMP_ADAPTER *pAd,
    IN  BOOLEAN         bFromApcli,
	IN	UCHAR	apidx);

#ifdef WSC_INCLUDED
INT	Set_WscGenPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef BB_SOC
INT	Set_WscResetPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif

INT Set_WscVendorPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef WSC_AP_SUPPORT
VOID RTMPIoctlSetWSCOOB(IN RTMP_ADAPTER *pAd);
#endif

/* */
/* prototype in wsc.c */
/* */
BOOLEAN	WscMsgTypeSubst(
	IN	UCHAR	EAPType,
	IN	UCHAR	EAPCode,
	OUT	INT	    *MsgType);

VOID    WscStateMachineInit(
	IN	RTMP_ADAPTER *pAd,
	IN	STATE_MACHINE		*S,
	OUT STATE_MACHINE_FUNC Trans[]);

#ifdef IWSC_SUPPORT
void	IWSC_StateMachineInit(
    IN  RTMP_ADAPTER *pAd,
    IN  STATE_MACHINE *S,
    OUT STATE_MACHINE_FUNC Trans[]);

VOID	IWSC_Init(
	IN  IN RTMP_ADAPTER *pAd);
#endif // IWSC_SUPPORT //

VOID    WscEAPOLStartAction(
    IN  RTMP_ADAPTER *pAd,
    IN  MLME_QUEUE_ELEM  *Elem);

VOID    WscEAPAction(
	IN	RTMP_ADAPTER *pAd,
	IN	MLME_QUEUE_ELEM *Elem);

VOID    WscEapEnrolleeAction(
	IN	RTMP_ADAPTER *pAd,
	IN	MLME_QUEUE_ELEM	*Elem,
	IN  UCHAR	        MsgType,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl);

#ifdef CONFIG_AP_SUPPORT
VOID    WscEapApProxyAction(
	IN	RTMP_ADAPTER *pAd,
	IN	MLME_QUEUE_ELEM	*Elem,
	IN  UCHAR	        MsgType,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl);
#endif /* CONFIG_AP_SUPPORT */

VOID    WscEapRegistrarAction(
	IN	RTMP_ADAPTER *pAd,
	IN	MLME_QUEUE_ELEM	*Elem,
	IN  UCHAR	        MsgType,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl);

VOID    WscEAPOLTimeOutAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

VOID    Wsc2MinsTimeOutAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

UCHAR	WscRxMsgType(
	IN	RTMP_ADAPTER *pAd,
	IN	PMLME_QUEUE_ELEM	pElem);

VOID	WscInitRegistrarPair(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	IN  UCHAR				apidx);

VOID	WscSendEapReqId(
	IN	RTMP_ADAPTER *pAd,
	IN	PMAC_TABLE_ENTRY	pEntry,
	IN  UCHAR				CurOpMode);

VOID    WscSendEapolStart(
	IN	RTMP_ADAPTER *pAd,
	IN  PUCHAR          pBssid,
	IN  UCHAR			CurOpMode);

VOID	WscSendEapRspId(
	IN	RTMP_ADAPTER *pAd,
	IN  PMAC_TABLE_ENTRY    pEntry,
	IN  PWSC_CTRL           pWscControl);

VOID	WscMacHeaderInit(
	IN	RTMP_ADAPTER *pAd,
	IN OUT	PHEADER_802_11	Hdr,
	IN	PUCHAR 			pAddr1,
	IN  PUCHAR          pBSSID,
	IN  BOOLEAN         bFromApCli);

VOID	WscSendMessage(
	IN	RTMP_ADAPTER *pAd,
	IN  UCHAR               OpCode,
	IN  PUCHAR				pData,
	IN  INT					Len,
	IN  PWSC_CTRL           pWscControl,
    IN  UCHAR               OpMode,	/* 0: AP Mode, 1: AP Client Mode, 2: STA Mode */
    IN  UCHAR               EapType);

VOID	WscSendEapReqAck(
	IN	RTMP_ADAPTER *pAd,
	IN	PMAC_TABLE_ENTRY	pEntry);

VOID	WscSendEapReqDone(
	IN	RTMP_ADAPTER *pAd,
	IN	PMLME_QUEUE_ELEM	pElem);

VOID	WscSendEapFail(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	IN  BOOLEAN				bSendDeAuth);

VOID WscM2DTimeOutAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

VOID WscUPnPMsgTimeOutAction(
	IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

int WscSendUPnPConfReqMsg(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR apIdx,
	IN PUCHAR ssidStr,
	IN PUCHAR macAddr,
	IN INT	  Status,
	IN UINT   eventID,
	IN UCHAR  CurOpMode);


int WscSendUPnPMessage(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR				apIdx,
	IN	USHORT				msgType,
	IN	USHORT				msgSubType,
	IN	PUCHAR				pData,
	IN	INT					dataLen,
	IN	UINT				eventID,
	IN	UINT				toIPAddr,
	IN	PUCHAR				pMACAddr,
	IN  UCHAR				CurOpMode);

VOID WscUPnPErrHandle(
	IN RTMP_ADAPTER *pAd,
	IN PWSC_CTRL		pWscControl,
	IN UINT 			eventID);

VOID    WscBuildBeaconIE(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR b_configured,
	IN	BOOLEAN b_selRegistrar,
	IN	USHORT devPwdId,
	IN	USHORT selRegCfgMethods,
	IN  UCHAR apidx,
	IN  UCHAR *pAuthorizedMACs,
	IN  UCHAR  	AuthorizedMACsLen,
	IN  UCHAR	CurOpMode);

VOID    WscBuildProbeRespIE(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR respType,
	IN	UCHAR scState,
	IN	BOOLEAN b_selRegistrar,
	IN	USHORT devPwdId,
	IN	USHORT selRegCfgMethods,
	IN  UCHAR apidx,
	IN  UCHAR *pAuthorizedMACs,
	IN  INT   AuthorizedMACsLen,
	IN  UCHAR	CurOpMode);

#ifdef WIDI_SUPPORT
#ifdef CONFIG_STA_SUPPORT
VOID WscMakeProbeReqIEWithVendorExt(
	IN	RTMP_ADAPTER *pAd,
	IN	PUCHAR			pDeviceName,
	IN	PUCHAR			pPrimaryDeviceType,
	IN	PUCHAR			pVendExt,
	IN	USHORT			vendExtLen,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pIeLen);
#endif /* CONFIG_STA_SUPPORT */
#endif /* WIDI_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
VOID WscBuildAssocRespIE(
	IN	RTMP_ADAPTER *pAd,
	IN  UCHAR 			ApIdx,
	IN  UCHAR			Reason,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pIeLen);

VOID	WscSelectedRegistrar(
	IN	RTMP_ADAPTER *pAd,
	IN	PUCHAR	RegInfo,
	IN	UINT	length,
	IN  UCHAR 	apidx);

VOID    WscInformFromWPA(
    IN  PMAC_TABLE_ENTRY    pEntry);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
VOID WscBuildProbeReqIE(
	IN	RTMP_ADAPTER *pAd,
	IN  UCHAR			CurOpMode,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pIeLen);

VOID WscBuildAssocReqIE(
	IN  PWSC_CTRL		pWscControl,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pIeLen);

#ifdef IWSC_SUPPORT
VOID IWSC_Stop(RTMP_ADAPTER *pAd, BOOLEAN bSendNotification);

VOID IWSC_T1TimerAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

VOID IWSC_T2TimerAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

VOID IWSC_EntryTimerAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

VOID IWSC_DevQueryAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

BOOLEAN	IWSC_PeerEapolStart(
    IN  RTMP_ADAPTER *pAd,
    IN  PMAC_TABLE_ENTRY 	pEntry);

VOID IWSC_AddSmpbcEnrollee(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR			pPeerAddr);

BOOLEAN IWSC_IpContentForCredential(
	IN  RTMP_ADAPTER *pAd);
#endif /* IWSC_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

VOID    WscProfileRetryTimeout(
	IN  PVOID SystemSpecific1,
	IN  PVOID FunctionContext,
	IN  PVOID SystemSpecific2,
	IN  PVOID SystemSpecific3);

VOID    WscPBCTimeOutAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);

VOID    WscScanTimeOutAction(
    IN  PVOID SystemSpecific1,
    IN  PVOID FunctionContext,
    IN  PVOID SystemSpecific2,
    IN  PVOID SystemSpecific3);


INT WscGenerateUUID(
	RTMP_ADAPTER	*pAd,
	UCHAR 			*uuidHexStr,
	UCHAR 			*uuidAscStr,
	int 			apIdx,
	BOOLEAN			bUseCurrentTime);

VOID WscStop(
	IN	RTMP_ADAPTER *pAd,
#ifdef CONFIG_AP_SUPPORT
    IN  BOOLEAN         bFromApcli,
#endif /* CONFIG_AP_SUPPORT */
	IN  PWSC_CTRL       pWscControl);

VOID WscInit(
	IN	RTMP_ADAPTER *pAd,
    IN  BOOLEAN         bFromApcli,
	IN  UCHAR       	BssIndex);

BOOLEAN	ValidateChecksum(UINT PIN);

UINT WscRandomGen4digitPinCode(RTMP_ADAPTER *pAd);

UINT WscRandomGeneratePinCode(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR	apidx);

int BuildMessageM1(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM2(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM2D(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM3(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM4(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM5(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM6(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM7(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageM8(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageDONE(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageACK(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int BuildMessageNACK(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	OUT	VOID *pbuf);

int ProcessMessageM1(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	IN	VOID *precv,
	IN	INT Length,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM2(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL		pWscControl,
	IN	VOID *precv,
	IN	INT Length,
	IN  UCHAR			apidx,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM2D(
	IN	RTMP_ADAPTER *pAd,
	IN	VOID *precv,
	IN	INT Length,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM3(
	IN	RTMP_ADAPTER *pAd,
	IN	VOID *precv,
	IN	INT Length,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM4(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	IN	VOID *precv,
	IN	INT Length,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM5(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	IN	VOID *precv,
	IN	INT Length,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM6(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	IN	VOID *precv,
	IN	INT Length,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM7(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL           pWscControl,
	IN	VOID *precv,
	IN	INT Length,
	OUT	PWSC_REG_DATA pReg);

int ProcessMessageM8(
	IN	RTMP_ADAPTER *pAd,
	IN	VOID *precv,
	IN	INT Length,
	IN  PWSC_CTRL       pWscControl);

USHORT  WscGetAuthType(
    IN  NDIS_802_11_AUTHENTICATION_MODE authType);

USHORT  WscGetEncryType(
    IN  NDIS_802_11_WEP_STATUS encryType);

NDIS_STATUS WscThreadInit(RTMP_ADAPTER *pAd);

BOOLEAN WscThreadExit(RTMP_ADAPTER *pAd);

int     AppendWSCTLV(
    IN  USHORT index,
    OUT UCHAR * obuf,
    IN  UCHAR * ibuf,
    IN  USHORT varlen);

VOID    WscGetRegDataPIN(
    IN  RTMP_ADAPTER *pAd,
    IN  UINT            PinCode,
    IN  PWSC_CTRL       pWscControl);

VOID    WscPushPBCAction(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL   	pWscControl);

VOID    WscScanExec(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL   	pWscControl);

BOOLEAN WscPBCExec(
	IN	RTMP_ADAPTER *pAd,
	IN  BOOLEAN			bFromM2,
	IN  PWSC_CTRL       pWscControl);

VOID    WscPBCBssTableSort(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL       pWscControl);

VOID	WscGenRandomKey(
	IN  	RTMP_ADAPTER *pAd,
	IN  	PWSC_CTRL       pWscControl,
	INOUT	PUCHAR			pKey,
	INOUT	PUSHORT			pKeyLen);

VOID	WscCreateProfileFromCfg(
	IN	RTMP_ADAPTER *pAd,
	IN  UCHAR               OpMode,         /* 0: AP Mode, 1: AP Client Mode, 2: STA Mode */
	IN  PWSC_CTRL           pWscControl,
	OUT PWSC_PROFILE        pWscProfile);

void    WscWriteConfToPortCfg(
    IN  RTMP_ADAPTER *pAd,
    IN  PWSC_CTRL       pWscControl,
    IN  PWSC_CREDENTIAL pCredential,
    IN  BOOLEAN         bEnrollee);

#ifdef APCLI_SUPPORT
void    WscWriteConfToApCliCfg(
    IN  RTMP_ADAPTER *pAd,
    IN  PWSC_CTRL       pWscControl,
    IN  PWSC_CREDENTIAL pCredential,
    IN  BOOLEAN         bEnrollee);
#endif /* APCLI_SUPPORT */

VOID   WpsSmProcess(
    IN PRTMP_ADAPTER        pAd,
    IN MLME_QUEUE_ELEM 	   *Elem);

VOID WscPBCSessionOverlapCheck(
	IN	RTMP_ADAPTER *pAd);

VOID WscPBC_DPID_FromSTA(
	IN	RTMP_ADAPTER *pAd,
	IN	PUCHAR				pMacAddr);

#ifdef CONFIG_AP_SUPPORT
INT	WscGetConfWithoutTrigger(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL       pWscControl,
	IN  BOOLEAN         bFromUPnP);

BOOLEAN	WscReadProfileFromUfdFile(
	IN	RTMP_ADAPTER *pAd,
	IN  UCHAR               ApIdx,
	IN  RTMP_STRING *pUfdFileName);

BOOLEAN	WscWriteProfileToUfdFile(
	IN	RTMP_ADAPTER *pAd,
	IN  UCHAR               ApIdx,
	IN  RTMP_STRING *pUfdFileName);
#endif /* CONFIG_AP_SUPPORT */

VOID WscCheckWpsIeFromWpsAP(
    IN  RTMP_ADAPTER *pAd,
    IN  PEID_STRUCT		pEid,
#ifdef STA_EASY_CONFIG_SETUP
	OUT PUCHAR			pWpsAp,
#endif /* STA_EASY_CONFIG_SETUP */
    OUT PUSHORT			pDPIDFromAP);

#ifdef CONFIG_STA_SUPPORT
VOID PeerProbeRespAction(
	IN RTMP_ADAPTER *pAd,
	IN MLME_QUEUE_ELEM *Elem);

ULONG WscSearchWpsApBySSID(
	IN  RTMP_ADAPTER *pAd,
	IN PUCHAR	 		pSsid,
	IN UCHAR	 		SsidLen,
	IN INT		 		WscMode);
#endif /* CONFIG_STA_SUPPORT */

/* WSC hardware push button function 0811 */
VOID WSC_HDR_BTN_Init(RTMP_ADAPTER *pAd);
VOID WSC_HDR_BTN_Stop(RTMP_ADAPTER *pAd);
VOID WSC_HDR_BTN_CheckHandler(RTMP_ADAPTER *pAd);
#ifdef WSC_LED_SUPPORT
BOOLEAN WscSupportWPSLEDMode(RTMP_ADAPTER *pAd);
BOOLEAN WscSupportWPSLEDMode10(RTMP_ADAPTER *pAd);

BOOLEAN WscAPHasSecuritySetting(RTMP_ADAPTER *pAd,PWSC_CTRL pWscControl);

VOID WscLEDTimer(
	IN PVOID	SystemSpecific1,
	IN PVOID	FunctionContext,
	IN PVOID	SystemSpecific2,
	IN PVOID	SystemSpecific3);

VOID WscSkipTurnOffLEDTimer(
	IN PVOID	SystemSpecific1,
	IN PVOID	FunctionContext,
	IN PVOID	SystemSpecific2,
	IN PVOID	SystemSpecific3);
#endif /* WSC_LED_SUPPORT */

#ifdef DPA_T
VOID WscSortProfile(RTMP_ADAPTER *pAd, PWSC_CTRL pWscControl);
#endif /* DPA_T */

#ifdef WIDI_SUPPORT
void WidiWscNotify(
	IN ULONG pAd,
	IN ULONG pWidiMsg,
	IN ULONG MsgSize,
	IN ULONG MsgType);

VOID WidiUpdateStateToDaemon(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR BssIdx,
	IN UCHAR WidiType,
	IN PUCHAR pPeerMac,
	IN PUCHAR pSsid,
	IN UCHAR  SsidLen,
	IN UCHAR WidiAssocStat);

VOID WidiNotifyVendorExtToDaemon(
	IN RTMP_ADAPTER *pAd,
	IN PWIDI_VENDOR_EXT pWiDiVendorExtList,
	IN PUCHAR pSrcMac,
	IN UCHAR channel,
	IN PCHAR Ssid,
	IN UCHAR SsidLen);

UINT WscSpecialRandomGeneratePinCode(RTMP_ADAPTER *pAd);

VOID WidiNotifyP2pIPToDaemon(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR pWpsIe,
	IN UCHAR WpsIeLen,
	IN PUCHAR pSrcMac,
	IN UCHAR channel,
	IN PCHAR Ssid,
	IN UCHAR SsidLen);

BOOLEAN  WidiNotifyP2pProbeIeToDaemon(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR pWpsIe,
	IN UCHAR WpsIeLen,
	IN PUCHAR pSrcMac,
	IN UINT8 channel,
	IN UCHAR *Ssid,
	IN USHORT SsidLen);

#ifdef WFA_WFD_SUPPORT
VOID WidiNotifyWfdIEToDaemon(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR pData,
	IN UINT32 DataLen,
	IN PUCHAR pSrcMac,
	IN UINT8 FrameType);

VOID P2pGetWfdIE(
	IN RTMP_ADAPTER *pAd,
	IN VOID				*Msg,
	IN ULONG			MsgLen,
	OUT UCHAR			*pWfdIe,
	OUT UINT32			*pWfdIeLen);
#endif /* WFA_WFD_SUPPORT */
#endif /* WIDI_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
VOID WscUpdatePortCfgTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);
#endif /* CONFIG_AP_SUPPORT */

VOID WscCheckPeerDPID(
	IN  RTMP_ADAPTER *pAd,
	IN  PFRAME_802_11 	Fr,
	IN  PUCHAR			eid_data,
	IN  INT				eid_len);

VOID WscClearPeerList(PLIST_HEADER pWscEnList);

PWSC_PEER_ENTRY WscFindPeerEntry(PLIST_HEADER	pWscEnList, UCHAR *pMacAddr);
VOID WscDelListEntryByMAC(PLIST_HEADER pWscEnList, UCHAR *pMacAddr);
VOID WscInsertPeerEntryByMAC(PLIST_HEADER	pWscEnList, UCHAR *pMacAddr);

#ifdef CONFIG_AP_SUPPORT
INT WscApShowPeerList(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
INT WscStaShowPeerList(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* CONFIG_STA_SUPPORT */

VOID WscMaintainPeerList(RTMP_ADAPTER *pAd, PWSC_CTRL pWpsCtrl);

VOID WscAssignEntryMAC(RTMP_ADAPTER *pAd, PWSC_CTRL pWpsCtrl);

#ifdef WSC_V2_SUPPORT
#ifdef CONFIG_AP_SUPPORT
VOID WscOnOff(RTMP_ADAPTER *pAd, INT ApIdx, BOOLEAN bOff);

VOID WscAddEntryToAclList(RTMP_ADAPTER *pAd, INT ApIdx, UCHAR *pMacAddr);

VOID WscSetupLockTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID WscCheckPinAttackCount(RTMP_ADAPTER *pAd, PWSC_CTRL pWscControl);
#endif /* CONFIG_AP_SUPPORT */

BOOLEAN	WscGenV2Msg(
	IN  PWSC_CTRL		pWpsCtrl,
	IN  BOOLEAN			bSelRegistrar,
	IN	PUCHAR			pAuthorizedMACs,
	IN  INT   			AuthorizedMACsLen,
	OUT	UCHAR			**pOutBuf,
	OUT	INT				*pOutBufLen);

BOOLEAN	WscParseV2SubItem(
	IN	UCHAR			SubID,
	IN	PUCHAR			pData,
	IN	USHORT			DataLen,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pOutBufLen);

VOID	WscSendEapFragAck(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL			pWscControl,
	IN	PMAC_TABLE_ENTRY	pEntry);

VOID	WscSendEapFragData(
	IN	RTMP_ADAPTER *pAd,
	IN  PWSC_CTRL			pWscControl,
	IN	PMAC_TABLE_ENTRY	pEntry);
#endif /* WSC_V2_SUPPORT */

BOOLEAN WscGetDataFromPeerByTag(
    IN  RTMP_ADAPTER *pAd,
    IN  PUCHAR			pIeData,
    IN  INT				IeDataLen,
    IN  USHORT			WscTag,
    OUT PUCHAR			pWscBuf,
    OUT PUSHORT			pWscBufLen);

#endif /* WSC_INCLUDED */

INT32 ShowRFInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT32 ShowBBPInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT32 show_redirect_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef EASY_CONFIG_SETUP
#ifdef CONFIG_AP_SUPPORT
VOID 	AutoProvisionBuildAssocRspIE(
	IN  RTMP_ADAPTER *pAd,
	IN  PMLME_QUEUE_ELEM	Elem,
	IN  PMAC_TABLE_ENTRY	pEntry,
	OUT PUCHAR				pOutBuffer,
	OUT PULONG				pFrameLen);

VOID	AutoProvisionCheckEntry(
	IN  RTMP_ADAPTER *pAd,
	IN  PMAC_TABLE_ENTRY	pEntry,
	IN  PEID_STRUCT			eid_ptr);

VOID	AutoProvisionAssignSSID(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR				eid_data,
	IN  INT					eid_len,
	IN  PUCHAR				pAddr2,
	IN  UCHAR				apidx,
	OUT PUCHAR				SsidLen,
	OUT PUCHAR				Ssid);
#endif /* CONFIG_AP_SUPPORT */

VOID AutoProvisionGenWpsPTK(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR			ApIdx);

VOID AutoProvisionDecodeExtData(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR 			ApIdx,
	IN  PUCHAR			pEntryAddr,
	IN  PUCHAR				pData);

BOOLEAN WpsMICIntegrity(
    IN  RTMP_ADAPTER *pAd,
    IN  PUCHAR 			pData,
    IN  INT 			DataLen,
    IN	PUCHAR			pMacAddr,
    IN	UCHAR			ApIdx);

VOID	WpsCalculateMIC(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR			pBuf,
	IN	INT				BufLen,
	IN	PEASY_CONFIG_INFO		pEasyConfig,
	IN  PUCHAR			pMacAddr,
	OUT	PUCHAR			pMIC);
#endif /* EASY_CONFIG_SETUP */

#ifdef WAC_SUPPORT
VOID WAC_GenDeviceAdvertiseAttr(
	IN  BOOLEAN			bEnableWAC,
	OUT	PUCHAR			pData,
	OUT PUCHAR			pDataLen);

BOOLEAN WAC_ParseAttr(
	IN UCHAR AttrID,
	IN UCHAR *pInData,
	IN UCHAR InDataLen,
	OUT UCHAR *pOutData,
	IN UCHAR OutDataLen);

VOID WAC_AddDevAdvAttrIE(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR apidx,
	INOUT UCHAR *pBeaconBuf,
	INOUT ULONG *pFrameLen);

INT WAC_IoctlReq(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR ApIdx,
	IN  RTMP_IOCTL_INPUT_STRUCT	*wrq);

#ifdef CONFIG_AP_SUPPORT
VOID WAC_GenStatusAttr(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR ApIdx,
	IN CHAR PeerRssi,
	IN BOOLEAN bInvalidData,
	IN CHAR ThresholdOffset1,
	OUT UCHAR *pData,
	OUT UCHAR *pDataLen);

VOID WAC_MaintainPeerList(
	IN  PLIST_HEADER	pWscEnList);

VOID WAC_CheckWACEntry(
	IN  RTMP_ADAPTER *pAd,
	IN  MAC_TABLE_ENTRY *pEntry);

VOID WAC_PeerProbeReq(
	IN  RTMP_ADAPTER *pAd,
	IN  UCHAR 			ApIdx,
	IN  MLME_QUEUE_ELEM *Elem,
	IN  UCHAR			*Addr2,
	INOUT UCHAR			*pBuf,
	INOUT ULONG			*pFrameLen);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
VOID WAC_GenUicastProbeReqAttr(
	IN  BOOLEAN			bEnableWAC,
	IN  PUCHAR			pPIN,
	OUT	PUCHAR			pData,
	OUT PUCHAR			pDataLen);

UCHAR WAC_SearchSamsungWACAP(RTMP_ADAPTER *pAd);

BOOLEAN WAC_FindWACAP(RTMP_ADAPTER *pAd);
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
#ifdef WAC_QOS_PRIORITY
VOID WAC_InsertForcePriorityEntryByMAC(
	IN  PLIST_HEADER	pWscEnList,
	IN UCHAR *pMacAddr,
	IN UCHAR DeviceType);
#endif /* WAC_QOS_PRIORITY */
#endif /* CONFIG_AP_SUPPORT */
#endif /* WAC_SUPPORT */

#ifdef NINTENDO_AP
VOID InitNINTENDO_TABLE(RTMP_ADAPTER *pAd);

UCHAR CheckNINTENDO_TABLE(RTMP_ADAPTER *pAd, CHAR *ds_Ssid, UCHAR DS_SsidLen, UCHAR *dsAddr);
UCHAR DelNINTENDO_ENTRY(RTMP_ADAPTER *pAd, UCHAR *pDS_Addr);

VOID RTMPIoctlNintendoCapable(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

VOID RTMPIoctlNintendoGetTable(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);

VOID RTMPIoctlNintendoSetTable(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq);
#endif /* NINTENDO_AP */

BOOLEAN rtstrmactohex(RTMP_STRING *s1, RTMP_STRING *s2);
BOOLEAN rtstrcasecmp(RTMP_STRING *s1, RTMP_STRING *s2);
RTMP_STRING *rtstrstruncasecmp(RTMP_STRING *s1, RTMP_STRING *s2);

RTMP_STRING *rtstrstr( const RTMP_STRING *s1, const RTMP_STRING *s2);
RTMP_STRING *rstrtok( RTMP_STRING *s, const RTMP_STRING *ct);
int rtinet_aton(const RTMP_STRING *cp, unsigned int *addr);

/*//////// common ioctl functions ////////*/
INT Set_DriverVersion_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_CountryRegion_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_CountryRegionABand_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_WirelessMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#ifdef RT_CFG80211_SUPPORT
INT Set_DisableCfg2040Scan_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif	
INT Set_MBSS_WirelessMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_Channel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#if defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392)
INT Set_BbpResetFlagCountVale(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* defined(RT5370) || defined(RT5372) || defined(RT5390) || defined(RT5392) */
INT	Set_ShortSlot_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_TxPower_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_BGProtection_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_TxPreamble_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_RTSThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_FragThreshold_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_TxBurst_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef RTMP_MAC_PCI
INT Set_ShowRF_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* RTMP_MAC_PCI */

#ifdef AGGREGATION_SUPPORT
INT	Set_PktAggregate_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* AGGREGATION_SUPPORT */

#ifdef INF_PPA_SUPPORT
INT	Set_INF_AMAZON_SE_PPA_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT ifx_ra_start_xmit (
	IN	struct net_device *rx_dev,
	IN	struct net_device *tx_dev,
	IN	struct sk_buff *skb,
	IN	int len);
#endif /* INF_PPA_SUPPORT */

INT	Set_IEEE80211H_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef EXT_BUILD_CHANNEL_LIST
INT Set_ExtCountryCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ExtDfsType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ChannelListAdd_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ChannelListShow_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ChannelListDel_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* EXT_BUILD_CHANNEL_LIST */

#ifdef DBG
INT	Set_Debug_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_DebugFunc_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

VOID RTMPIoctlMAC(RTMP_ADAPTER *pAd, RTMP_IOCTL_INPUT_STRUCT *wrq);
#endif

#ifdef THERMAL_PROTECT_SUPPORT	
INT set_thermal_protection_criteria_proc(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_STRING *arg);
#endif /* THERMAL_PROTECT_SUPPORT */

#ifdef TXBF_SUPPORT
INT	Set_ReadITxBf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_ReadETxBf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WriteITxBf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WriteETxBf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_StatITxBf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_StatETxBf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_TxBfTag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ITxBfTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ETxBfTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_InvTxBfTag_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_ITxBfCal_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#ifdef MT76x2
INT	mt76x2_Set_ITxBfCal_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif
INT	Set_ITxBfDivCal_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_ITxBfLnaCal_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_ETxBfEnCond_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ETxBfCodebook_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ETxBfCoefficient_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ETxBfGrouping_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ETxBfNoncompress_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ETxBfIncapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_NoSndgCntThrd_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_NdpSndgStreams_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_Trigger_Sounding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_ITxBfEn_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef MT76x2
INT Set_TxBfProfileTag_Help(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTagValid(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTagRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTagWrite(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileDataRead(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileDataWrite(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileDataWriteAll(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileDataReadAll(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTag_TimeOut(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTag_Matrix(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTag_SNR(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTag_TxScale(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTag_MAC(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_TxBfProfileTag_Flg(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif

#endif /* TXBF_SUPPORT */

#ifdef VHT_TXBF_SUPPORT
INT Set_VhtNDPA_Sounding_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* VHT_TXBF_SUPPORT */


INT Set_RateAdaptInterval(RTMP_ADAPTER *pAd, RTMP_STRING *arg);


#ifdef PRE_ANT_SWITCH
INT Set_PreAntSwitch_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_PreAntSwitchRSSI_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_PreAntSwitchTimeout_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#if defined (RT2883) || defined (RT3883)
#ifdef CONFIG_STA_SUPPORT
VOID STASelectPktDetAntenna(RTMP_ADAPTER *pAd);
#endif /* CONFIG_STA_SUPPORT*/

#ifdef CONFIG_AP_SUPPORT
INT rtmp_pre_ant_switch(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
#endif /* CONFIG_AP_SUPPORT */
#endif /* defined (RT2883) || defined (RT3883) */
#endif /* PRE_ANT_SWITCH */


#if defined(RT2883) || defined(RT3883) || defined(RT3593)
INT Set_PhyRateLimit_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef DBG
INT Set_FixedRate_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DBG */
#endif /* defined(RT2883) || defined(RT3883) || defined(RT3593) */

#ifdef CFO_TRACK
INT Set_CFOTrack_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef CFO_TRACK
#ifdef CONFIG_AP_SUPPORT
INT rtmp_cfo_track(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry, INT lastClient);
#endif /* CONFIG_AP_SUPPORT */
#endif /* CFO_TRACK */

#endif // CFO_TRACK //

#ifdef DBG_CTRL_SUPPORT
INT Set_DebugFlags_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef INCLUDE_DEBUG_QUEUE
INT Set_DebugQueue_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
void dbQueueEnqueue(UCHAR type, UCHAR *data);
void dbQueueEnqueueTxFrame(UCHAR *pTxWI, UCHAR *pDot11Hdr);
void dbQueueEnqueueRxFrame(UCHAR *pRxWI, UCHAR *pDot11Hdr ULONG flags);
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

INT Show_DescInfo_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Show_MacTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef DOT11_N_SUPPORT
INT Show_BaTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11_N_SUPPORT */

#ifdef MT_MAC
INT Show_PSTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_wtbl_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_temp_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_mib_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT32 ShowTmacInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT32 ShowAggInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT ShowManualTxOP(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT32 ShowPseInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT32 ShowPseData(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#if defined(RTMP_PCI_SUPPORT) || defined(RTMP_USB_SUPPORT)
INT32 ShowDMASchInfo(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif
#endif /* MT_MAC */
INT Show_sta_tr_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_stainfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_devinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_sysinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_trinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT show_txqinfo_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_ResetStatCounter_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef DOT11_N_SUPPORT
INT	Set_BASetup_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_BADecline_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_BAOriTearDown_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_BARecTearDown_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtBw_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtMcs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtGi_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtOpMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtStbc_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtExtcha_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtMpduDensity_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtBaWinSize_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtRdg_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtLinkAdapt_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtAmsdu_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtAutoBa_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtProtect_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtMimoPs_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef DOT11N_DRAFT3
INT Set_HT_BssCoex_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_HT_BssCoexApCntThr_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11N_DRAFT3 */


#ifdef CONFIG_AP_SUPPORT
INT	Set_HtTxStream_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtRxStream_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#ifdef DOT11_N_SUPPORT
#ifdef GREENAP_SUPPORT
INT	Set_GreenAP_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* GREENAP_SUPPORT */
#endif /* DOT11_N_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

INT	SetCommonHT(RTMP_ADAPTER *pAd);

INT	Set_ForceShortGI_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_ForceGF_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_SendSMPSAction_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

void convert_reordering_packet_to_preAMSDU_or_802_3_packet(
	IN RTMP_ADAPTER *pAd,
	IN RX_BLK *pRxBlk,
	IN UCHAR wdev_idx);

INT	Set_HtMIMOPSmode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtTxBASize_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_HtDisallowTKIP_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_BurstMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11_N_SUPPORT */


#ifdef DOT11_VHT_AC
INT Set_VhtBw_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_VhtStbc_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_VhtBwSignal_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_VhtDisallowNonVHT_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11_VHT_AC */


#ifdef APCLI_SUPPORT
INT RTMPIoctlConnStatus(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /*APCLI_SUPPORT*/

#ifdef ETH_CONVERT_SUPPORT
INT Set_EthConvertMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_EthCloneMac_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* ETH_CONVERT_SUPPORT */


#ifdef RELEASE_EXCLUDE
#ifdef RTMP_MAC_USB
INT	Show_RxBulk_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Show_TxBulk_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* RTMP_MAC_USB */
#endif /* RELEASE_EXCLUDE */

#ifdef CONFIG_STA_SUPPORT
VOID RTMPSendDLSTearDownFrame(RTMP_ADAPTER *pAd, UCHAR *pDA);
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
#ifdef RTMP_MAC_USB
#ifdef DOT11_N_SUPPORT
INT sta_wmm_adjust(RTMP_ADAPTER *pAd);
#endif /* DOT11_N_SUPPORT */
#endif /* RTMP_MAC_USB */
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
VOID detect_wmm_traffic(RTMP_ADAPTER *pAd, UCHAR up, UCHAR bOutput);
VOID dynamic_tune_be_tx_op(RTMP_ADAPTER *pAd, ULONG nonBEpackets);
#endif /* CONFIG_AP_SUPPORT */


#ifdef DOT11_N_SUPPORT
VOID Handle_BSS_Width_Trigger_Events(RTMP_ADAPTER *pAd);

void build_ext_channel_switch_ie(
	IN RTMP_ADAPTER *pAd,
	IN HT_EXT_CHANNEL_SWITCH_ANNOUNCEMENT_IE *pIE);

void assoc_ht_info_debugshow(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN UCHAR ht_cap_len,
	IN HT_CAPABILITY_IE *pHTCapability);
#endif /* DOT11_N_SUPPORT */

BOOLEAN rtmp_rx_done_handle(RTMP_ADAPTER *pAd);

#ifdef RTMP_PCI_SUPPORT
#ifdef CONFIG_ANDES_SUPPORT
BOOLEAN RxRing1DoneInterruptHandle(RTMP_ADAPTER *pAd);
VOID RTMPHandleTxRing8DmaDoneInterrupt(RTMP_ADAPTER *pAd);
#endif /* CONFIG_ANDES_SUPPORT */

#ifdef MT_MAC
VOID RTMPHandleBcnDmaDoneInterrupt(RTMP_ADAPTER *pAd);
#endif /* MT_MAC */
#endif /* RTMP_PCI_SUPPORT */

INT wdev_bcn_buf_deinit(RTMP_ADAPTER *pAd, BCN_BUF_STRUC *bcn_info);
INT wdev_bcn_buf_init(RTMP_ADAPTER *pAd, BCN_BUF_STRUC *bcn_info);

#ifdef DOT11_N_SUPPORT
VOID Indicate_AMPDU_Packet(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk, UCHAR wdev_idx);
VOID Indicate_AMSDU_Packet(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk, UCHAR wdev_idx);

VOID BaReOrderingBufferMaintain(RTMP_ADAPTER *pAd);
#endif /* DOT11_N_SUPPORT */

/* Normal legacy Rx packet indication */
VOID Indicate_Legacy_Packet(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk, UCHAR wdev_idx);
VOID Indicate_EAPOL_Packet(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk, UCHAR wdev_idx);

UINT deaggregate_AMSDU_announce(
	IN	RTMP_ADAPTER *pAd,
	PNDIS_PACKET		pPacket,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize,
	IN	UCHAR			OpMode);

#ifdef TXBF_SUPPORT
BOOLEAN clientSupportsETxBF(RTMP_ADAPTER *pAd, HT_BF_CAP *pTxBFCap);
void setETxBFCap(RTMP_ADAPTER *pAd, HT_BF_CAP *pTxBFCap);
#ifdef VHT_TXBF_SUPPORT
BOOLEAN clientSupportsVHTETxBF(RTMP_ADAPTER *pAd, VHT_CAP_INFO *pTxBFCapInfo);
void setVHTETxBFCap(RTMP_ADAPTER *pAd, VHT_CAP_INFO *pTxBFCap);
#endif /* VHT_TXBF_SUPPORT */

#ifdef ETXBF_EN_COND3_SUPPORT
VOID txSndgSameMcs(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY * pEnt, UCHAR smoothMfb);
VOID txSndgOtherGroup(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
VOID txMrqInvTxBF(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
VOID chooseBestMethod(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry, UCHAR mfb);
VOID rxBestSndg(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
#endif /* ETXBF_EN_COND3_SUPPORT */

VOID handleBfFb(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk);

VOID TxBFInit(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry, BOOLEAN supETxBF);

VOID eTxBFProbing(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);

VOID Trigger_Sounding_Packet(
	IN	RTMP_ADAPTER *pAd,
	IN	UCHAR			SndgType,
	IN	UCHAR			SndgBW,
	IN	UCHAR			SndgMcs,
	IN  MAC_TABLE_ENTRY *pEntry);

VOID rtmp_asic_set_bf(RTMP_ADAPTER *pAd);
BOOLEAN rtmp_chk_itxbf_calibration(RTMP_ADAPTER *pAd);

#endif /* TXBF_SUPPORT */

/* remove LLC and get 802_3 Header */
#define  RTMP_802_11_REMOVE_LLC_AND_CONVERT_TO_802_3(_pRxBlk, _pHeader802_3)	\
{																						\
	PUCHAR _pRemovedLLCSNAP = NULL, _pDA, _pSA;                                 						\
																				\
	if (RX_BLK_TEST_FLAG(_pRxBlk, fRX_WDS) || RX_BLK_TEST_FLAG(_pRxBlk, fRX_MESH)) 		\
	{                                                                           											\
		_pDA = _pRxBlk->pHeader->Addr3;                                         							\
		_pSA = (PUCHAR)_pRxBlk->pHeader + sizeof(HEADER_802_11);                					\
	}                                                                           											\
	else if (RX_BLK_TEST_FLAG(_pRxBlk, fRX_AP))										\
	{																					\
		_pDA = _pRxBlk->pHeader->Addr1; 													\
		if (RX_BLK_TEST_FLAG(_pRxBlk, fRX_DLS))									\
			_pSA = _pRxBlk->pHeader->Addr2;										\
		else																	\
			_pSA = _pRxBlk->pHeader->Addr3;		\
	} \
	else if (RX_BLK_TEST_FLAG(_pRxBlk, fRX_STA))                              	\
	{                                                                       \
		_pDA = _pRxBlk->pHeader->Addr3;                                     \
		_pSA = _pRxBlk->pHeader->Addr2;                                     \
	}                                                                       \
	else if (RX_BLK_TEST_FLAG(_pRxBlk, fRX_ADHOC))                              \
	{                                                                       \
		_pDA = _pRxBlk->pHeader->Addr1;                                     \
		_pSA = _pRxBlk->pHeader->Addr2;                                     \
	} \
	else                                                                        										\
	{    /* TODO: shiang-usw, where shall we go here?? */ \
		DBGPRINT(RT_DEBUG_ERROR, ("%s():Un-assigned Peer's Role!\n", __FUNCTION__));\
		_pDA = _pRxBlk->pHeader->Addr3;                                         							\
		_pSA = _pRxBlk->pHeader->Addr2;                                         							\
	}                                                                           											\
																				\
	CONVERT_TO_802_3(_pHeader802_3, _pDA, _pSA, _pRxBlk->pData, 		\
		_pRxBlk->DataSize, _pRemovedLLCSNAP);	                                  \
	if(_pRemovedLLCSNAP != NULL) 										\
	{																		\
		DBGPRINT(RT_DEBUG_INFO, ("%s():LLC/SNAP field found!\n", __FUNCTION__));           \
	}                                                      \
}


VOID Announce_or_Forward_802_3_Packet(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET pPacket,
	IN UCHAR wdev_idx,
	IN UCHAR op_mode);

VOID Indicate_ARalink_Packet(
	IN RTMP_ADAPTER *pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN RX_BLK *pRxBlk,
	IN UCHAR wdev_idx);

VOID Update_Rssi_Sample(
	IN RTMP_ADAPTER *pAd,
	IN RSSI_SAMPLE *pRssi,
	IN struct rx_signal_info *signal,
	IN USHORT phymode,
	IN USHORT bw);

PNDIS_PACKET GetPacketFromRxRing(
	RTMP_ADAPTER *pAd,
	RX_BLK *pRxBlk,
	BOOLEAN	*pbReschedule,
	UINT32 *pRxPending,
	UCHAR RxRingNo);

PNDIS_PACKET RTMPDeFragmentDataFrame(
	IN RTMP_ADAPTER *pAd,
	IN RX_BLK *pRxBlk);

/*////////////////////////////////////*/

#if defined (AP_SCAN_SUPPORT) || defined (CONFIG_STA_SUPPORT)
VOID RTMPIoctlGetSiteSurvey(
	IN	RTMP_ADAPTER *pAd,
	IN	RTMP_IOCTL_INPUT_STRUCT *wrq);
#endif

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
INT Set_ApCli_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Enable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ApCli_Ssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */


#ifdef MAT_SUPPORT

VOID getIPv6MacTbInfo(MAT_STRUCT *, char *, ULONG);
VOID getIPMacTbInfo(MAT_STRUCT *pMatCfg, char *pOutBuf, ULONG BufLen);

NDIS_STATUS MATEngineInit(RTMP_ADAPTER *pAd);
NDIS_STATUS MATEngineExit(RTMP_ADAPTER *pAd);

PUCHAR MATEngineRxHandle(RTMP_ADAPTER *pAd, PNDIS_PACKET pPkt, UINT infIdx);
PUCHAR MATEngineTxHandle(RTMP_ADAPTER *pAd, PNDIS_PACKET pPkt, UINT infIdx, UCHAR OpMode);

BOOLEAN MATPktRxNeedConvert(RTMP_ADAPTER *pAd, PNET_DEV net_dev);

#endif /* MAT_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
typedef struct CountryCodeToCountryRegion {
	USHORT		CountryNum;
	UCHAR		IsoName[3];
	/*UCHAR		CountryName[40]; */
	RTMP_STRING *pCountryName;
	BOOLEAN		SupportABand;
	/*ULONG		RegDomainNum11A; */
	UCHAR		RegDomainNum11A;
	BOOLEAN  	SupportGBand;
	/*ULONG		RegDomainNum11G; */
	UCHAR		RegDomainNum11G;
} COUNTRY_CODE_TO_COUNTRY_REGION;
#endif /* CONFIG_AP_SUPPORT */

#ifdef SNMP_SUPPORT
/*for snmp */
typedef struct _DefaultKeyIdxValue
{
	UCHAR	KeyIdx;
	UCHAR	Value[16];
} DefaultKeyIdxValue, *PDefaultKeyIdxValue;
#endif

typedef struct _ASIC_TX_CNT_UPDATE
{
	MAC_TABLE_ENTRY *pEntry;
	MT_TX_COUNTER *pTxInfo;	
} ASIC_TX_CNT_UPDATE, *PASIC_TX_CNT_UPDATE;


void STA_MonPktSend(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk);

#ifdef CONFIG_STA_SUPPORT
VOID RTMPSetDesiredRates(RTMP_ADAPTER *pAd, LONG Rates);

#ifdef XLINK_SUPPORT
INT Set_XlinkMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* XLINK_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

INT	Set_FixedTxMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef CONFIG_APSTA_MIXED_SUPPORT
INT	Set_OpMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* CONFIG_APSTA_MIXED_SUPPORT */

INT Set_LongRetryLimit_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ShortRetryLimit_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_AutoFallBack_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
VOID RT28XXDMADisable(RTMP_ADAPTER *pAd);
VOID RT28XXDMAEnable(RTMP_ADAPTER *pAd);

#ifdef RTMP_MAC_PCI
VOID PDMAWatchDog(RTMP_ADAPTER *pAd);
VOID PDMAResetAndRecovery(RTMP_ADAPTER *pAd);
VOID DumpPseInfo(RTMP_ADAPTER *pAd);
#endif
VOID PSEWatchDog(RTMP_ADAPTER *pAd);
VOID PSEResetAndRecovery(RTMP_ADAPTER *pAd);

VOID RT28xx_UpdateBeaconToAsic(
	IN RTMP_ADAPTER * pAd,
	IN INT apidx,
	IN ULONG BeaconLen,
	IN ULONG UpdatePos);

void CfgInitHook(RTMP_ADAPTER *pAd);


NDIS_STATUS RtmpNetTaskInit(RTMP_ADAPTER *pAd);
NDIS_STATUS RtmpMgmtTaskInit(RTMP_ADAPTER *pAd);
VOID RtmpNetTaskExit(RTMP_ADAPTER *pAd);
VOID RtmpMgmtTaskExit(RTMP_ADAPTER *pAd);

void tbtt_tasklet(unsigned long data);

#ifdef MT_MAC
void mt_mac_int_0_tasklet(unsigned long data);
void mt_mac_int_1_tasklet(unsigned long data);
void mt_mac_int_2_tasklet(unsigned long data);
void mt_mac_int_3_tasklet(unsigned long data);
void mt_mac_int_4_tasklet(unsigned long data);
#endif /* MT_MAC */

#ifdef RTMP_MAC_PCI
BOOLEAN RT28xxPciAsicRadioOff(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR            Level,
	IN USHORT           TbttNumToNextWakeUp);

BOOLEAN RT28xxPciAsicRadioOn(RTMP_ADAPTER *pAd, UCHAR Level);
VOID RTMPInitPCIeDevice(RT_CMD_PCIE_INIT *pConfig, VOID *pAd);

#ifdef CONFIG_STA_SUPPORT
#ifdef PCIE_PS_SUPPORT
VOID RTMPInitPCIeLinkCtrlValue(RTMP_ADAPTER *pAd);
VOID RTMPPCIeLinkCtrlValueRestore(RTMP_ADAPTER *pAd, UCHAR Level);
VOID RTMPPCIeLinkCtrlSetting(RTMP_ADAPTER *pAd, USHORT Max);
VOID RTMPrt3xSetPCIePowerLinkCtrl(RTMP_ADAPTER *pAd);

VOID PsPollWakeExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID  RadioOnExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);
#endif /* PCIE_PS_SUPPORT */

VOID RT28xxPciStaAsicForceWakeup(RTMP_ADAPTER *pAd, BOOLEAN bFromTx);

VOID RT28xxPciStaAsicSleepThenAutoWakeup(
	IN RTMP_ADAPTER *pAd,
	IN USHORT TbttNumToNextWakeUp);

#endif /* CONFIG_STA_SUPPORT */

VOID PciMlmeRadioOn(RTMP_ADAPTER *pAd);
VOID PciMlmeRadioOFF(RTMP_ADAPTER *pAd);

ra_dma_addr_t RtmpDrvPciMapSingle(
	IN RTMP_ADAPTER *pAd,
	IN VOID *ptr,
	IN size_t size,
	IN INT sd_idx,
	IN INT direction);

INT rtmp_irq_init(RTMP_ADAPTER *pAd);
#endif /* RTMP_MAC_PCI */

#ifdef CONFIG_STA_SUPPORT
#ifdef CREDENTIAL_STORE
NDIS_STATUS RecoverConnectInfo(RTMP_ADAPTER *pAd);
NDIS_STATUS StoreConnectInfo(RTMP_ADAPTER *pAd);
#endif /* CREDENTIAL_STORE */
#endif /* CONFIG_STA_SUPPORT */

VOID AsicTurnOffRFClk(RTMP_ADAPTER *pAd, UCHAR Channel);

#ifdef RT305x
INT Set_HiPower_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#if defined(RT3352) || defined(RT5350)
#ifdef RTMP_INTERNAL_TX_ALC
INT Set_TSSIMaxRange_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* RTMP_INTERNAL_TX_ALC */
#endif /* defined(RT3352) || defined(RT5350) */
#endif /* RT305x */

#ifdef MT_WOW_SUPPORT
VOID MT76xxAndesWOWEnable(RTMP_ADAPTER *pAd);
VOID MT76xxAndesWOWDisable(RTMP_ADAPTER *pAd);
VOID MT76xxAndesWOWInit(RTMP_ADAPTER *pAd);
#endif

/*////////////////////////////////////*/


/*///////////////////////////////////*/
INT RTMPShowCfgValue(RTMP_ADAPTER *pAd, RTMP_STRING *name, RTMP_STRING *buf, UINT32 MaxLen);
RTMP_STRING *RTMPGetRalinkAuthModeStr(NDIS_802_11_AUTHENTICATION_MODE authMode);
RTMP_STRING *RTMPGetRalinkEncryModeStr(USHORT encryMode);
/*//////////////////////////////////*/

#ifdef CONFIG_STA_SUPPORT
VOID AsicStaBbpTuning(RTMP_ADAPTER *pAd);

BOOLEAN StaUpdateMacTableEntry(
	IN  RTMP_ADAPTER *pAd,
	IN  PMAC_TABLE_ENTRY	pEntry,
	IN  UCHAR				MaxSupportedRateIn500Kbps,
	IN  HT_CAPABILITY_IE	*pHtCapability,
	IN  UCHAR				HtCapabilityLen,
	IN  ADD_HT_INFO_IE		*pAddHtInfo,
	IN  UCHAR				AddHtInfoLen,
	IN IE_LISTS *ie_list,
	IN  USHORT        		CapabilityInfo);

#ifdef DOT11R_FT_SUPPORT
VOID FT_OTA_AuthTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);

VOID FT_OTD_TimeoutAction(
	IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);
#endif /* DOT11R_FT_SUPPORT */

BOOLEAN	AUTH_ReqSend(
	IN  PRTMP_ADAPTER 		pAd,
	IN  PMLME_QUEUE_ELEM	pElem,
	IN  PRALINK_TIMER_STRUCT pAuthTimer,
	IN  RTMP_STRING *pSMName,
	IN  USHORT				SeqNo,
	IN  PUCHAR				pNewElement,
	IN  ULONG				ElementLen);
#endif /* CONFIG_STA_SUPPORT */

VOID RTMPSetAGCInitValue(RTMP_ADAPTER *pAd, UCHAR BandWidth);

#ifdef TXBF_SUPPORT
VOID handleHtcField(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk);
#endif /* TXBF_SUPPORT */

#ifdef MFB_SUPPORT
VOID MFB_PerPareMRQ(RTMP_ADAPTER *pAd, VOID* pBuf, MAC_TABLE_ENTRY *pEntry);
VOID MFB_PerPareMFB(RTMP_ADAPTER *pAd, VOID *pBuf, MAC_TABLE_ENTRY *pEntry);
#endif /* MFB_SUPPORT */

#define VIRTUAL_IF_INC(__pAd) ((__pAd)->VirtualIfCnt++)
#define VIRTUAL_IF_DEC(__pAd) ((__pAd)->VirtualIfCnt--)
#define VIRTUAL_IF_NUM(__pAd) ((__pAd)->VirtualIfCnt)


#ifdef RTMP_USB_SUPPORT
#ifdef INF_AMAZON_SE
VOID SoftwareFlowControl(RTMP_ADAPTER *pAd) ;
#endif /* INF_AMAZON_SE */


VOID RTUSBInitTxDesc(
	IN	RTMP_ADAPTER *pAd,
	IN	PTX_CONTEXT		pTxContext,
	IN	UCHAR			BulkOutPipeId,
	IN	usb_complete_t	Func);

VOID RTUSBInitHTTxDesc(
	IN	RTMP_ADAPTER *pAd,
	IN	PHT_TX_CONTEXT	pTxContext,
	IN	UCHAR			BulkOutPipeId,
	IN	ULONG			BulkOutSize,
	IN	usb_complete_t	Func);

VOID RTUSBInitRxDesc(RTMP_ADAPTER *pAd, RX_CONTEXT *pRxContext);
VOID RTUSBCleanUpDataBulkOutQueue(RTMP_ADAPTER *pAd);
VOID RTUSBCancelPendingBulkOutIRP(RTMP_ADAPTER *pAd);

VOID RTUSBBulkOutDataPacket(RTMP_ADAPTER *pAd, UCHAR BulkOutPipeId, UCHAR Index);
VOID RTUSBBulkOutNullFrame(RTMP_ADAPTER *pAd);
VOID RTUSBBulkOutRTSFrame(RTMP_ADAPTER *pAd);

VOID RTUSBCancelPendingBulkInIRP(RTMP_ADAPTER *pAd);
VOID RTUSBCancelPendingIRPs(RTMP_ADAPTER *pAd);

VOID RTUSBBulkOutMLMEPacket(RTMP_ADAPTER *pAd, UCHAR Index);
VOID RTUSBBulkOutPsPoll(RTMP_ADAPTER *pAd);
VOID RTUSBCleanUpMLMEBulkOutQueue(RTMP_ADAPTER *pAd);

VOID RTUSBKickBulkOut(RTMP_ADAPTER *pAd);
VOID RTUSBBulkReceive(RTMP_ADAPTER *pAd);

VOID RTUSBBulkCmdRspEventReceive(RTMP_ADAPTER *pAd);

VOID DoBulkIn(RTMP_ADAPTER *pAd);

VOID RTUSBInitRxDesc(RTMP_ADAPTER *pAd, RX_CONTEXT *pRxContext);
VOID RTUSBBulkRxHandle(ULONG data);
VOID InitUSBDevice(RT_CMD_USB_INIT *pConfig, VOID *pAd);
#endif /* RTMP_USB_SUPPORT */


#ifdef SOFT_ENCRYPT
BOOLEAN RTMPExpandPacketForSwEncrypt(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk);
VOID RTMPUpdateSwCacheCipherInfo(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk, UCHAR *pHdr);
INT tx_sw_encrypt(RTMP_ADAPTER *pAd, TX_BLK *pTxBlk, UCHAR *buf_ptr, HEADER_802_11 *wifi_hdr);
#endif /* SOFT_ENCRYPT */


/*
	OS Related funciton prototype definitions.
	TODO: Maybe we need to move these function prototypes to other proper place.
*/
VOID RTInitializeCmdQ(PCmdQ cmdq);

INT RTPCICmdThread(ULONG Context);

VOID CMDHandler(RTMP_ADAPTER *pAd);

VOID RTThreadDequeueCmd(PCmdQ cmdq, PCmdQElmt *pcmdqelmt);

NDIS_STATUS RTEnqueueInternalCmd(
	IN RTMP_ADAPTER *pAd,
	IN NDIS_OID			Oid,
	IN PVOID			pInformationBuffer,
	IN UINT32			InformationBufferLength);

#ifdef HOSTAPD_SUPPORT
VOID ieee80211_notify_michael_failure(
	IN	RTMP_ADAPTER *pAd,
	IN	PHEADER_802_11   pHeader,
	IN	UINT            keyix,
	IN	INT              report);

const CHAR* ether_sprintf(const UINT8 *mac);
#endif/*HOSTAPD_SUPPORT*/


#ifdef VENDOR_FEATURE3_SUPPORT
VOID RTMP_IO_WRITE32(RTMP_ADAPTER *pAd, UINT32 Offset, UINT32 Value);
#endif /* VENDOR_FEATURE3_SUPPORT */


BOOLEAN CHAN_PropertyCheck(RTMP_ADAPTER *pAd, UINT32 ChanNum, UCHAR Property);

#ifdef SINGLE_SKU_V2
INT SetSKUEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* SINGLE_SKU_V2 */
INT Set_ed_chk_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef CONFIG_STA_SUPPORT
#ifdef STA_EASY_CONFIG_SETUP
void AutoProvisionStateMachineInit(
    IN RTMP_ADAPTER *pAd,
    IN STATE_MACHINE *Sm,
    OUT STATE_MACHINE_FUNC Trans[]);

VOID AutoProvisionScanTimeOutAction(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);

VOID InfraConnectionTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);

VOID AdhocConnectionTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);
#endif /* STA_EASY_CONFIG_SETUP */

/* command */
INT Set_SSID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT	Set_WmmCapable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_NetworkType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_AuthMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_EncrypType_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT Set_DefaultKeyID_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_Wep_Key_Proc(RTMP_ADAPTER  *pAd, RTMP_STRING *Key, INT KeyLen, INT KeyId);
INT Set_Key1_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_Key2_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_Key3_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_Key4_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_WPAPSK_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_PSMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef WPA_SUPPLICANT_SUPPORT
INT Set_Wpa_Support(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WPA_SUPPLICANT_SUPPORT */

#ifdef DBG
VOID RTMPIoctlE2PROM(RTMP_ADAPTER *pAd, RTMP_IOCTL_INPUT_STRUCT *wrq);
#endif /* DBG */

#ifdef WSC_STA_SUPPORT
VOID CntlWscIterate(RTMP_ADAPTER *pAd);

USHORT WscGetAuthTypeFromStr(RTMP_STRING * arg);
USHORT WscGetEncrypTypeFromStr(RTMP_STRING *arg);

INT	Set_WscConfMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WscConfStatus_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WscSsid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_WscBssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WscMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WscUUIDE_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WscGetConf_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_WscStop_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_WscPinCode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef WSC_V2_SUPPORT
INT Set_WscForceSetAP_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WSC_V2_SUPPORT */
#endif /* WSC_STA_SUPPORT */

NDIS_STATUS RTMPWPANoneAddKeyProc(RTMP_ADAPTER *pAd, VOID *pBuf);

INT Set_FragTest_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef DOT11_N_SUPPORT
INT Set_TGnWifiTest_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11_N_SUPPORT */

INT Set_LongRetryLimit_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ShortRetryLimit_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef EXT_BUILD_CHANNEL_LIST
INT Set_Ieee80211dClientMode_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* EXT_BUILD_CHANNEL_LIST */

INT	Show_Adhoc_MacTable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *extra, UINT32 size);

#ifdef RTMP_RF_RW_SUPPORT
VOID RTMPIoctlRF(
	IN	RTMP_ADAPTER *pAd,
	IN	RTMP_IOCTL_INPUT_STRUCT *wrq);
#endif /* RTMP_RF_RW_SUPPORT */

#ifdef DOT11R_FT_SUPPORT
INT Set_FtApBssid_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_FtSupport_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* DOT11R_FT_SUPPORT */

INT Set_BeaconLostTime_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_AutoRoaming_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_SiteSurvey_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT Set_ForceTxBurst_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

VOID RTMPAddKey(RTMP_ADAPTER *pAd, PNDIS_802_11_KEY pKey);
VOID StaSiteSurvey(RTMP_ADAPTER *pAd, PNDIS_802_11_SSID pSsid, UCHAR ScanType);

VOID MaintainBssTable(
	IN  RTMP_ADAPTER *pAd,
	IN OUT	BSS_TABLE *Tab,
	IN  ULONG	MaxRxTimeDiff,
	IN  UCHAR	MaxSameRxTimeCount);
#endif /* CONFIG_STA_SUPPORT */

void  getRate(HTTRANSMIT_SETTING HTSetting, ULONG* fLastTxRxRate);


#ifdef APCLI_SUPPORT
#ifdef WPA_SUPPLICANT_SUPPORT
VOID ApcliSendAssocIEsToWpaSupplicant(
    IN  RTMP_ADAPTER *pAd,
    IN UINT ifIndex);

VOID ApcliWpaSendEapolStart(
	IN	RTMP_ADAPTER *pAd,
	IN  PUCHAR          pBssid,
	IN  PMAC_TABLE_ENTRY pMacEntry,
	IN	PAPCLI_STRUCT pApCliEntry);
#endif/*WPA_SUPPLICANT_SUPPORT*/

VOID ApCliRTMPSendNullFrame(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR			TxRate,
	IN BOOLEAN 		bQosNull,
	IN MAC_TABLE_ENTRY *pMacEntry,
	IN      USHORT          PwrMgmt);

#endif/*APCLI_SUPPORT*/


void RTMP_IndicateMediaState(
	IN	RTMP_ADAPTER *pAd,
	IN  NDIS_MEDIA_STATE	media_state);

#if defined(RT3350) || defined(RT33xx)
VOID RTMP_TxEvmCalibration(
	IN RTMP_ADAPTER *pAd);
#endif /* defined(RT3350) || defined(RT33xx) */

INT RTMPSetInformation(
    IN RTMP_ADAPTER *pAd,
    IN OUT RTMP_IOCTL_INPUT_STRUCT *rq,
    IN INT cmd);

INT RTMPQueryInformation(
    IN RTMP_ADAPTER *pAd,
    INOUT RTMP_IOCTL_INPUT_STRUCT *rq,
    IN INT cmd);

VOID RTMPIoctlShow(
	IN RTMP_ADAPTER *pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *rq,
	IN UINT32 subcmd,
	IN VOID *pData,
	IN ULONG Data);

INT RTMP_COM_IoctlHandle(
	IN VOID *pAdSrc,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq,
	IN INT cmd,
	IN USHORT subcmd,
	IN VOID *pData,
	IN ULONG Data);

#ifdef CONFIG_AP_SUPPORT
INT RTMP_AP_IoctlPrepare(RTMP_ADAPTER *pAd, VOID *pCB);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_SNIFFER_SUPPORT
INT Set_MonitorMode_Proc(RTMP_ADAPTER	*pAd, RTMP_STRING *arg);
#endif /* CONFIG_SNIFFER_SUPPORT */

INT Set_VcoPeriod_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT	Set_RateAlg_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef SINGLE_SKU
INT Set_ModuleTxpower_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* SINGLE_SKU */

NDIS_STATUS RtmpEnqueueTokenFrame(
    IN RTMP_ADAPTER *pAd,
    IN UCHAR *pAddr,
    IN UCHAR TxRate,
    IN UCHAR AID,
    IN UCHAR apidx,
    IN UCHAR OldUP);

VOID RtmpEnqueueNullFrame(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *pAddr,
	IN UCHAR TxRate,
	IN UCHAR AID,
	IN UCHAR apidx,
	IN BOOLEAN bQosNull,
	IN BOOLEAN bEOSP,
	IN UCHAR OldUP);

VOID RtmpCleanupPsQueue(
	IN  RTMP_ADAPTER *pAd,
	IN  PQUEUE_HEADER   pQueue);

NDIS_STATUS RtmpInsertPsQueue(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET pPacket,
	IN MAC_TABLE_ENTRY *pMacEntry,
	IN UCHAR QueIdx);

VOID RtmpHandleRxPsPoll(RTMP_ADAPTER *pAd, UCHAR *pAddr, USHORT Aid, BOOLEAN isActive);
BOOLEAN RtmpPsIndicate(RTMP_ADAPTER *pAd, UCHAR *pAddr, UCHAR wcid, UCHAR Psm);

#ifdef MT_MAC
#if defined(MT7603) && defined(RTMP_PCI_SUPPORT)
INT  MtPSDummyCR(RTMP_ADAPTER *pAd);
BOOLEAN MtStartPSRetrieve(RTMP_ADAPTER *pAd, USHORT wcid);
#endif
VOID MtHandleRxPsPoll(RTMP_ADAPTER *pAd, UCHAR *pAddr, USHORT wcid, BOOLEAN isActive);
BOOLEAN MtPsIndicate(RTMP_ADAPTER *pAd, UCHAR *pAddr, UCHAR wcid, UCHAR Psm);
VOID MtPsRedirectDisableCheck(RTMP_ADAPTER *pAd, UCHAR wcid);
VOID MtPsSendToken(RTMP_ADAPTER *pAd, UINT32 WlanIdx);
VOID MtSetIgnorePsm(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry, UCHAR value);
VOID CheckSkipTX(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
VOID MtEnqTxSwqFromPsQueue(RTMP_ADAPTER *pAd, UCHAR qidx, STA_TR_ENTRY *tr_entry);
#endif /* MT_MAC */

#if defined(RTMP_MAC) || defined(RLT_MAC)
VOID RalHandleRxPsPoll(RTMP_ADAPTER *pAd, UCHAR *pAddr, USHORT wcid, BOOLEAN isActive);
BOOLEAN RalPsIndicate(RTMP_ADAPTER *pAd, UCHAR *pAddr, UCHAR wcid, UCHAR Psm);
#endif /* RTMP_MAC || RLT_MAC */

#ifdef CONFIG_MULTI_CHANNEL
VOID RtmpEnqueueLastNullFrame(
	IN RTMP_ADAPTER *pAd,
	IN PUCHAR pAddr,
	IN UCHAR TxRate,
	IN UCHAR PID,
	IN UCHAR apidx,
    IN BOOLEAN bQosNull,
    IN BOOLEAN bEOSP,
    IN UCHAR OldUP,
    IN UCHAR PwrMgmt,
	IN UCHAR OpMode);

VOID EnableMACTxPacket(
	IN RTMP_ADAPTER *pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN UCHAR PwrMgmt,
	IN BOOLEAN bTxNullFramei,
	IN UCHAR QSel);

VOID DisableMACTxPacket(
	IN RTMP_ADAPTER *pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN UCHAR PwrMgmt,
	IN BOOLEAN bWaitACK,
	IN UCHAR QSel);

VOID InitMultiChannelRelatedValue(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR Channel,
	IN UCHAR CentralChannel);

VOID EDCA_ActionTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

VOID HCCA_ActionTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);

NDIS_STATUS MultiChannelThreadInit(RTMP_ADAPTER *pAd);
BOOLEAN MultiChannelThreadExit(RTMP_ADAPTER *pAd);
VOID MultiChannelTimerStop(RTMP_ADAPTER *pAd);
VOID MultiChannelTimerStart(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY  *pEntry);
#endif /* CONFIG_MULTI_CHANNEL */

VOID RtmpPrepareHwNullFrame(
	IN RTMP_ADAPTER *pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN BOOLEAN bQosNull,
	IN BOOLEAN bEOSP,
	IN UCHAR OldUP,
	IN UCHAR OpMode,
	IN UCHAR PwrMgmt,
	IN BOOLEAN bWaitACK,
	IN CHAR Index);


VOID dev_rx_mgmt_frm(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk);
VOID dev_rx_ctrl_frm(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk);

#ifdef CONFIG_STA_SUPPORT
BOOLEAN RtmpPktPmBitCheck(RTMP_ADAPTER *pAd);
VOID RtmpPsActiveExtendCheck(RTMP_ADAPTER *pAd);
VOID RtmpPsModeChange(RTMP_ADAPTER *pAd, UINT32 PsMode);
#endif /* CONFIG_STA_SUPPORT */

#ifdef DOT11_N_SUPPORT
void DisplayTxAgg (RTMP_ADAPTER *pAd);
#endif /* DOT11_N_SUPPORT */

VOID set_default_ap_edca_param(RTMP_ADAPTER *pAd);
VOID set_default_sta_edca_param(RTMP_ADAPTER *pAd);

UCHAR dot11_2_ra_rate(UCHAR MaxSupportedRateIn500Kbps);
UCHAR dot11_max_sup_rate(INT SupRateLen, UCHAR *SupRate, INT ExtRateLen, UCHAR *ExtRate);

VOID tr_tb_reset_entry(RTMP_ADAPTER *pAd, UCHAR tr_tb_idx);
VOID tr_tb_set_entry(RTMP_ADAPTER *pAd, UCHAR tr_tb_idx, MAC_TABLE_ENTRY *pEntry);
VOID tr_tb_set_mcast_entry(RTMP_ADAPTER *pAd, UCHAR tr_tb_idx, struct wifi_dev *wdev);
VOID dump_tr_entry(RTMP_ADAPTER *pAd, INT tr_idx, RTMP_STRING *caller, INT line);

VOID mgmt_tb_set_mcast_entry(RTMP_ADAPTER *pAd, UCHAR wcid);
VOID set_entry_phy_cfg(RTMP_ADAPTER *pAd, MAC_TABLE_ENTRY *pEntry);
VOID MacTableReset(RTMP_ADAPTER *pAd, INT startWcid);
MAC_TABLE_ENTRY *MacTableLookup(RTMP_ADAPTER *pAd, const UCHAR *pAddr);
BOOLEAN MacTableDeleteEntry(RTMP_ADAPTER *pAd, USHORT wcid, UCHAR *pAddr);
MAC_TABLE_ENTRY *MacTableInsertEntry(
    IN RTMP_ADAPTER *pAd,
    IN UCHAR *pAddr,
    IN struct wifi_dev *wdev,
	IN UINT32 ent_type,
	IN UCHAR OpMode,
	IN BOOLEAN CleanAll);

#ifdef MAC_REPEATER_SUPPORT
REPEATER_CLIENT_ENTRY *RTMPLookupRepeaterCliEntry(
	IN RTMP_ADAPTER *pAd,
	IN BOOLEAN bRealMAC,
	IN PUCHAR pAddr);

VOID RTMPInsertRepeaterAsicEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR CliIdx,
	IN PUCHAR pAddr);

VOID RTMPRemoveRepeaterAsicEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR CliIdx);

VOID RTMPInsertRepeaterEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR apidx,
	IN PUCHAR pAddr);

VOID RTMPRemoveRepeaterEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR func_tb_idx,
	IN UCHAR CliIdx);

VOID RTMPRepeaterReconnectionCheck(
	IN RTMP_ADAPTER *pAd);

MAC_TABLE_ENTRY *RTMPInsertRepeaterMacEntry(
	IN  RTMP_ADAPTER *pAd,
	IN  PUCHAR pAddr,
	IN  struct wifi_dev *wdev,
	IN  UCHAR apidx,
	IN  UCHAR cliIdx,
	IN BOOLEAN CleanAll);

BOOLEAN RTMPRepeaterVaildMacEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *pAddr);

INVAILD_TRIGGER_MAC_ENTRY *RepeaterInvaildMacLookup(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *pAddr);

VOID RTMPRepeaterInsertInvaildMacEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR *pAddr);

BOOLEAN RTMPRepeaterRemoveInvaildMacEntry(
	IN RTMP_ADAPTER *pAd,
	IN UCHAR idx,
	IN UCHAR *pAddr);

INT	Show_Repeater_Cli_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef MT_MAC
void insert_repeater_root_entry(
	IN PRTMP_ADAPTER pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN  UCHAR *pAddr,
	IN UCHAR ReptCliIdx);
#endif /* MT_MAC */
#endif /* MAC_REPEATER_SUPPORT */

VOID dump_rxblk(RTMP_ADAPTER *pAd, RX_BLK *pRxBlk);

#if defined(CONFIG_CSO_SUPPORT) || defined(CONFIG_TSO_SUPPORT)
#ifdef RT65xx
VOID dump_tsoinfo(TSO_INFO *tinfo);
#endif /* RT65xx */
#endif /* defined(CONFIG_CSO_SUPPORT) || defined(CONFIG_TSO_SUPPORT) */

#ifdef RTMP_MAC_PCI
VOID dump_rxd(RTMP_ADAPTER *pAd, RXD_STRUC *pRxD);
#endif /* RTMP_MAC_PCI */

#ifdef MT_MAC
UINT32 parse_rx_packet_type(RTMP_ADAPTER *, RX_BLK *, VOID *);
#endif /* MT_MAC */

#ifdef CONFIG_FPGA_MODE
INT set_vco_cal(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_tr_stop(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_tx_kickcnt(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_data_phy_mode(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_data_bw(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_data_ldpc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_data_mcs(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_data_gi(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_data_basize(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_fpga_mode(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_loopback_mode(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

#ifdef MT_MAC
INT set_txs_report_type(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_no_bcn(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* MT_MAC */

#ifdef CONFIG_STA_SUPPORT
INT set_con_bssid_proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* CONFIG_STA_SUPPORT */

#ifdef CAPTURE_MODE
VOID cap_mode_init(RTMP_ADAPTER *pAd);
VOID cap_mode_deinit(RTMP_ADAPTER *pAd);
INT set_cap_start(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_cap_trigger(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT asic_cap_start(RTMP_ADAPTER *pAd);
INT asic_cap_stop(RTMP_ADAPTER *pAd);
INT cap_status_chk_and_get(RTMP_ADAPTER *pAd);
INT set_cap_dump(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
VOID cap_dump(RTMP_ADAPTER *pAd, CHAR *bank1, CHAR *bank2, INT len);
#endif /* CAPTURE_MODE */
#endif /* CONFIG_FPGA_MODE */

#if defined(WFA_VHT_PF) || defined(MT7603_FPGA) || defined(MT7628_FPGA)
INT set_force_amsdu(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* defined(WFA_VHT_PF) || defined(MT7603_FPGA) */

#ifdef WFA_VHT_PF
/* for SIGMA */
INT set_vht_nss_mcs_cap(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_vht_nss_mcs_opt(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_vht_opmode_notify_ie(RTMP_ADAPTER *pAd, RTMP_STRING *arg);

INT set_force_operating_mode(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_force_noack(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_force_vht_sgi(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_force_vht_tx_stbc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT set_force_ext_cca(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif /* WFA_VHT_PF */


#if defined(CONFIG_CSO_SUPPORT) || defined(CONFIG_TSO_SUPPORT)
INT rlt_net_acc_init(RTMP_ADAPTER *pAd);
INT rlt_tso_info_write(RTMP_ADAPTER *pAd, TSO_INFO *tinfo, TX_BLK *pTxBlk, int hdr_len);
#endif /* defined(CONFIG_CSO_SUPPORT) || defined(CONFIG_TSO_SUPPORT) */

INT SetRF(RTMP_ADAPTER *pAd, RTMP_STRING *Arg);
int write_reg(RTMP_ADAPTER *ad, UINT32 base, UINT16 offset, UINT32 value);
int read_reg(struct _RTMP_ADAPTER *ad, UINT32 base, UINT16 offset, UINT32 *value);
INT show_pwr_info(RTMP_ADAPTER *ad, RTMP_STRING *arg);

#ifdef MT76x2
#ifdef CONFIG_CALIBRATION_COLLECTION
INT Show_Cal_Info(struct _RTMP_ADAPTER *pAd, RTMP_STRING *arg);
#endif
#endif /* MT76x2 */

#ifdef WSC_INCLUDED
#ifdef WSC_NFC_SUPPORT
INT RtmpOSNotifyRawData(
	IN PNET_DEV pNetDev,
	IN PUCHAR buff,
	IN INT len,
	IN ULONG type,
	IN USHORT protocol);

#endif /* WSC_NFC_SUPPORT */
BOOLEAN	WscProcessCredential(
	IN	PRTMP_ADAPTER		pAdapter,
	IN	PUCHAR				pPlainData,
	IN	INT					PlainLength,
	IN  PWSC_CTRL           pWscCtrl);
#endif /* WSC_INCLUDED */

#ifdef MT_MAC
VOID StatRateToString(RTMP_ADAPTER *pAd, CHAR *Output, UCHAR TxRx, UINT32 RawData);
#endif /* MT_MAC */

#ifdef P2P_CHANNEL_LIST_SEPARATE
UCHAR P2PChannelSanity(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR channel);

VOID P2PBuildChannelList(
	IN PRTMP_ADAPTER pAd);

BOOLEAN P2P_CHAN_PropertyCheck(
	IN PRTMP_ADAPTER	pAd,
	IN UINT32			ChanNum,
	IN UCHAR			Property);
#endif /* P2P_CHANNEL_LIST_SEPARATE */


VOID  rtmp_ee_load_from_efuse(RTMP_ADAPTER *pAd);
#ifdef CONFIG_SNIFFER_SUPPORT
VOID Monitor_Init(RTMP_ADAPTER *pAd, RTMP_OS_NETDEV_OP_HOOK *pNetDevOps);
VOID Monitor_Remove(RTMP_ADAPTER *pAd);
BOOLEAN Monitor_Open(RTMP_ADAPTER *pAd, PNET_DEV dev_p);
BOOLEAN Monitor_Close(RTMP_ADAPTER *pAd, PNET_DEV dev_p);
#endif /* CONFIG_SNIFFER_SUPPORT */

#ifdef SMART_CARRIER_SENSE_SUPPORT
INT SetSCSEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT SetSCSDbgLogEnable_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT SetSCSCfg_Proc(RTMP_ADAPTER *pAd, RTMP_STRING *arg);
INT32 ShowSCSInfo(RTMP_ADAPTER *pAd, RTMP_STRING *Arg);
#endif /* SMART_CARRIER_SENSE_SUPPORT */

#endif  /* __RTMP_H__ */

