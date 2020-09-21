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

#ifdef RT_CFG80211_P2P_SUPPORT
INT rtmp_wdev_idx_find_by_p2p_ifaddr(RTMP_ADAPTER *pAd, UCHAR *ifAddr)
{
        INT idx, type;
        ULONG flags;
	PNET_DEV if_dev = NULL;
	struct wifi_dev *wdev = NULL;

        if (!ifAddr)
                return -1;

        RTMP_INT_LOCK(&pAd->irq_lock, flags);
        for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
		wdev = pAd->wdev_list[idx];
		if (!wdev)
                        continue;

		if_dev = wdev->if_dev;
		if (!if_dev)
			continue;

		if (!if_dev->ieee80211_ptr)
			continue;

		type = if_dev->ieee80211_ptr->iftype;

		if ( RTMPEqualMemory(wdev->if_addr, ifAddr, MAC_ADDR_LEN) &&
		    ( type == RT_CMD_80211_IFTYPE_P2P_CLIENT || 
                      type == RT_CMD_80211_IFTYPE_P2P_GO ||
                      type == RT_CMD_80211_IFTYPE_P2P_DEVICE
		    ))	
		{
                        //MTWF_LOG(DBG_CAT_ALL, DBG_LVL_INFO,
                        //                ("find the  wdev(type:%d, idx:%d) from wdev_list\n",
                        //                pAd->wdev_list[idx]->wdev_type, pAd->wdev_list[idx]->wdev_idx));
                        break;
                }
        }

#if 0
        if (idx == WDEV_NUM_MAX) {
                MTWF_LOG(DBG_CAT_ALL, DBG_LVL_ERROR,
                                        ("Cannot found wdev ifAddr(%02X:%02X:%02X:%02X:%02X:%02X) in wdev_list\n",
                                        PRINT_MAC(ifAddr)));
                MTWF_LOG(DBG_CAT_ALL, DBG_LVL_OFF, ("Dump wdev_list:\n"));
                for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
                        MTWF_LOG(DBG_CAT_ALL, DBG_LVL_OFF, ("Idx %d: 0x%p\n", idx, pAd->wdev_list[idx]));
                }
        }
#endif
        RTMP_INT_UNLOCK(&pAd->irq_lock, flags);

	return ((idx < WDEV_NUM_MAX) ? idx : -1);
}
#endif /* RT_CFG80211_P2P_SUPPORT */

INT rtmp_wdev_idx_unreg(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	INT idx;
	ULONG flags;

	if (!wdev)
		return -1;

	RTMP_INT_LOCK(&pAd->irq_lock, flags);
	for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
		if (pAd->wdev_list[idx] == wdev) {
			DBGPRINT(RT_DEBUG_WARN, 
					("unregister wdev(type:%d, idx:%d) from wdev_list\n",
					wdev->wdev_type, wdev->wdev_idx));
			pAd->wdev_list[idx] = NULL;
			wdev->wdev_idx = WDEV_NUM_MAX;
			break;
		}
	}

	if (idx == WDEV_NUM_MAX) {
		DBGPRINT(RT_DEBUG_ERROR, 
					("Cannot found wdev(%p, type:%d, idx:%d) in wdev_list\n",
					wdev, wdev->wdev_type, wdev->wdev_idx));
		DBGPRINT(RT_DEBUG_OFF, ("Dump wdev_list:\n"));
		for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
			DBGPRINT(RT_DEBUG_OFF, ("Idx %d: 0x%p\n", idx, pAd->wdev_list[idx]));
		}
	}
	RTMP_INT_UNLOCK(&pAd->irq_lock, flags);

	return ((idx < WDEV_NUM_MAX) ? 0 : -1);

}


INT rtmp_wdev_idx_reg(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	CHAR idx;
	ULONG flags;

	if (!wdev)
		return -1;

	RTMP_INT_LOCK(&pAd->irq_lock, flags);
	for (idx = 0; idx < WDEV_NUM_MAX; idx++) {
		if (pAd->wdev_list[idx] == wdev) {
			DBGPRINT(RT_DEBUG_WARN, 
					("wdev(type:%d) already registered and idx(%d) %smatch\n",
					wdev->wdev_type, wdev->wdev_idx, 
					((idx != wdev->wdev_idx) ? "mis" : "")));
			break;
		}
		if (pAd->wdev_list[idx] == NULL) {
			pAd->wdev_list[idx] = wdev;
			break;
		}
	}

	wdev->wdev_idx = idx;
	if (idx < WDEV_NUM_MAX) {
		DBGPRINT(RT_DEBUG_TRACE, ("Assign wdev_idx=%d\n", idx));
	}
	RTMP_INT_UNLOCK(&pAd->irq_lock, flags);

	return ((idx < WDEV_NUM_MAX) ? idx : -1);
}


//#ifdef RTMP_MAC_PCI
INT wdev_bcn_buf_init(RTMP_ADAPTER *pAd, BCN_BUF_STRUC *bcn_info)
{
	//bcn_info->bBcnSntReq = FALSE;
	bcn_info->BcnBufIdx = HW_BEACON_MAX_NUM;
	bcn_info->cap_ie_pos = 0;

	if (!bcn_info->BeaconPkt) {
		RTMPAllocateNdisPacket(pAd, &bcn_info->BeaconPkt, NULL, 0, NULL, MAX_BEACON_SIZE);
	}
	else {
		DBGPRINT(RT_DEBUG_OFF, ("%s():BcnPkt is allocated!\n", __FUNCTION__));		
	}
	
#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT)
	{
//		ASSERT(bcn_info->bcn_state == BCN_TX_IDLE);
		bcn_info->bcn_state = BCN_TX_IDLE;
	}
#endif /* MT_MAC */

	return TRUE;
}


INT wdev_bcn_buf_deinit(RTMP_ADAPTER *pAd, BCN_BUF_STRUC *bcn_info)
{
#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT)
	{
		if (bcn_info->bcn_state != BCN_TX_IDLE) {
			DBGPRINT(RT_DEBUG_ERROR, ("%s(): Bcn not in idle(%d) when try to free it!\n",
						__FUNCTION__, bcn_info->bcn_state));
			return FALSE;
		}
		bcn_info->bcn_state = BCN_TX_STOP;
	}
#endif /* MT_MAC */

	if (bcn_info->BeaconPkt) {
		RTMPFreeNdisPacket(pAd, bcn_info->BeaconPkt);
		bcn_info->BeaconPkt = NULL;
	}

	return TRUE;
}
//#endif /* RTMP_MAC_PCI */

INT wdev_init(RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UINT wdev_type)
{
#ifdef CONFIG_AP_SUPPORT
	if (wdev_type == WDEV_TYPE_AP) {
		//BSS_STRUCT *pMbss;

		wdev->wdev_type = WDEV_TYPE_AP;

		wdev->tx_pkt_allowed = ApAllowToSendPacket;
		wdev->tx_pkt_handle = APSendPacket;
		wdev->wdev_hard_tx = APHardTransmit;
		
		wdev->rx_pkt_allowed = ap_rx_pkt_allow;
		wdev->rx_ps_handle = ap_rx_ps_handle;
		wdev->rx_pkt_foward = ap_rx_foward_handle;

		//pMbss = (BSS_STRUCT *)wdev->func_dev;

		return TRUE;
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (wdev_type == WDEV_TYPE_STA) {
		wdev->wdev_type = WDEV_TYPE_STA;
		wdev->tx_pkt_allowed = StaAllowToSendPacket; // StaAllowToSendPacket_new;
		wdev->tx_pkt_handle = STASendPacket_New; // STASendPacket;
		wdev->wdev_hard_tx = STAHardTransmit;
		wdev->rx_pkt_foward = sta_rx_fwd_hnd;
		wdev->rx_pkt_allowed = sta_rx_pkt_allow;
//#ifdef RTMP_MAC_PCI
		wdev_bcn_buf_init(pAd, &pAd->StaCfg.bcn_buf);
//#endif /* RTMP_MAC_PCI */
	}
#endif /* CONFIG_STA_SUPPORT */


	return FALSE;
}

