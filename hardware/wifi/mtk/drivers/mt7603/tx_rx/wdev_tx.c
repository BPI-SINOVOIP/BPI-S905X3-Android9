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


/*
========================================================================
Routine Description:
    Early checking and OS-depened parsing for Tx packet to AP device.

Arguments:
    NDIS_HANDLE 	MiniportAdapterContext	Pointer refer to the device handle, i.e., the pAd.
	PPNDIS_PACKET	ppPacketArray			The packet array need to do transmission.
	UINT			NumberOfPackets			Number of packet in packet array.
	
Return Value:
	NONE					

Note:
	This function do early checking and classification for send-out packet.
	You only can put OS-depened & AP related code in here.
========================================================================
*/
INT wdev_tx_pkts(NDIS_HANDLE dev_hnd, PPNDIS_PACKET pkt_list, UINT pkt_cnt, struct wifi_dev *wdev)
{
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)dev_hnd;
	PNDIS_PACKET pPacket;
	INT allowToSend;
	UCHAR wcid = MAX_LEN_OF_MAC_TABLE;
	UINT Index;
#ifdef CONFIG_FPGA_MODE
	BOOLEAN force_tx;
#endif /* CONFIG_FPGA_MODE */

#ifdef RELEASE_EXCLUDE
	DBGPRINT(RT_DEBUG_INFO, ("====>%s()\n", __FUNCTION__));
#endif /* RELEASE_EXCLUDE */
	
	for (Index = 0; Index < pkt_cnt; Index++)
	{
		pPacket = pkt_list[Index];

   		if (RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_RESET_IN_PROGRESS |
								fRTMP_ADAPTER_HALT_IN_PROGRESS |
								fRTMP_ADAPTER_RADIO_OFF /*|
								fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS*/)))
		{
			/* Drop send request since hardware is in reset state */
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			continue;
		}

#ifdef CONFIG_FPGA_MODE
		force_tx = FALSE;
		if (pAd->fpga_ctl.fpga_on & 0x1) {
			if (pAd->fpga_ctl.tx_kick_cnt > 0) {
				if (pAd->fpga_ctl.tx_kick_cnt < 0xffff)  {
					pAd->fpga_ctl.tx_kick_cnt--;
				}
				force_tx = TRUE;
			}
		}
#endif /* CONFIG_FPGA_MODE */

	if ((pAd->cfg80211_ctrl.isCfgInApMode == RT_CMD_80211_IFTYPE_AP && (wdev->Hostapd != Hostapd_CFG))
#ifdef RT_CFG80211_P2P_SUPPORT
		&& (!RTMP_CFG80211_VIF_P2P_GO_ON(pAd))
#endif //RT_CFG80211_P2P_SUPPORT
		)
	{
		/*Drop send request since hardware is in reset state*/
		RELEASE_NDIS_PACKET(pAd,pPacket,NDIS_STATUS_FAILURE);
		//printk("%s: RELEASE_NDIS_PACKET packet, lk added\n", __FUNCTION__);
		continue;
	}

		if (((wdev->allow_data_tx == TRUE) 
#ifdef CONFIG_FPGA_MODE
			|| (force_tx == TRUE)
#endif /* CONFIG_FPGA_MODE */
			) 
			&& (wdev->tx_pkt_allowed)) {
			allowToSend = wdev->tx_pkt_allowed(pAd, wdev, pPacket, &wcid);
		}
		else {
			allowToSend = FALSE;
			//RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		}

#ifdef CONFIG_FPGA_MODE
		if (force_tx == TRUE && allowToSend == TRUE) {
			//int dump_len = GET_OS_PKT_LEN(pPacket);
			
			DBGPRINT(RT_DEBUG_INFO, ("%s():send Packet!wcid=%d, wdev_idx=%d, pktLen=%d\n",
						__FUNCTION__, wcid, wdev->wdev_idx, GET_OS_PKT_LEN(pPacket)));
			//hex_dump("wdev_tx_pkts():802.3 packet", GET_OS_PKT_DATAPTR(pPacket), dump_len > 255 ? 255 : dump_len);
		}
#endif /* CONFIG_FPGA_MODE */

		if (allowToSend == TRUE)
		{
			RTMP_SET_PACKET_WCID(pPacket, wcid);
			RTMP_SET_PACKET_WDEV(pPacket, wdev->wdev_idx);
			NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
			pAd->RalinkCounters.PendingNdisPacketCount++;

			if (wdev->tx_pkt_handle)
				wdev->tx_pkt_handle(pAd, pPacket);
			else
			{
				DBGPRINT(RT_DEBUG_ERROR, ("%s():tx_pkt_handle not assigned!\n", __FUNCTION__));
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
		}
		else
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
	}

	RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, WCID_ALL, MAX_TX_PROCESS);

	return 0;
}

