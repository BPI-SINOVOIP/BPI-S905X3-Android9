/*
 *************************************************************************** 
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2012, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	cfg80211_inf.c

	Abstract:

	Revision History:
	Who		When			What
	--------	----------		----------------------------------------------
	YF Luo		06-28-2012		Init version
*/
#define RTMP_MODULE_OS

#include "rt_config.h" 
#include "rtmp_comm.h"
#include "rt_os_util.h"
#include "rt_os_net.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
#ifdef RT_CFG80211_SUPPORT
BOOLEAN RTMP_CFG80211_VIF_ON(
	IN      VOID     *pAdSrc)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	return pAd->Cfg80211VifDevSet.isGoingOn;
}
	
BOOLEAN RTMP_CFG80211_VIF_P2P_GO_ON(
	IN      VOID     *pAdSrc)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	PNET_DEV pNetDev = NULL;

	if ((pAd->Cfg80211VifDevSet.vifDevList.size > 0) &&
		((pNetDev = RTMP_CFG80211_FindVifEntry_ByType(pAd, RT_CMD_80211_IFTYPE_P2P_GO)) != NULL))
	{
    		return TRUE;    
	}
	else	
	{
		return FALSE;
	}
}		

BOOLEAN RTMP_CFG80211_VIF_P2P_CLI_ON(
        IN      VOID     *pAdSrc)
{
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
        PNET_DEV pNetDev = NULL;

        if ((pAd->Cfg80211VifDevSet.vifDevList.size > 0) &&
            ((pNetDev = RTMP_CFG80211_FindVifEntry_ByType(pAd, RT_CMD_80211_IFTYPE_P2P_CLIENT)) != NULL))
        {

	//	if (pNetDev->ieee80211_ptr->sme_state == CFG80211_SME_CONNECTED)
                	return TRUE;
        }
        else
        {
                return FALSE;
        }

}

PCFG80211_VIF_DEV static RTMP_CFG80211_FindVifEntry_ByMac(
	IN      VOID     *pAdSrc,
	IN      PNET_DEV pNewNetDev)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	PLIST_HEADER  pCacheList = &pAd->Cfg80211VifDevSet.vifDevList;
	PCFG80211_VIF_DEV       	pDevEntry = NULL;
	PLIST_ENTRY		        pListEntry = NULL;

	pListEntry = pCacheList->pHead;
	pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	while (pDevEntry != NULL)
	{
		if (RTMPEqualMemory(pDevEntry->net_dev->dev_addr, pNewNetDev->dev_addr, MAC_ADDR_LEN))
			return pDevEntry;

		pListEntry = pListEntry->pNext;
		pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	}
	
	return NULL;	
}

PNET_DEV RTMP_CFG80211_FindVifEntry_ByType(
	IN      VOID     *pAdSrc,
	      UINT32    devType)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	PLIST_HEADER  pCacheList = &pAd->Cfg80211VifDevSet.vifDevList;
	PCFG80211_VIF_DEV       	pDevEntry = NULL;
	PLIST_ENTRY		        pListEntry = NULL;

	pListEntry = pCacheList->pHead;
	pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	while (pDevEntry != NULL)
	{
		if (pDevEntry->devType == devType)
			return pDevEntry->net_dev;
		
		pListEntry = pListEntry->pNext;
		pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	}
	
	return NULL;	
}

VOID RTMP_CFG80211_AddVifEntry(
	IN      VOID     *pAdSrc,
	IN      PNET_DEV pNewNetDev,
	IN      UINT32   DevType)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	PCFG80211_VIF_DEV pNewVifDev = NULL;
	
	os_alloc_mem(NULL, (UCHAR **)&pNewVifDev, sizeof(CFG80211_VIF_DEV));
	if (pNewVifDev)
	{
		NdisZeroMemory(pNewVifDev, sizeof(CFG80211_VIF_DEV));

		pNewVifDev->pNext = NULL;
		pNewVifDev->net_dev = pNewNetDev;
		pNewVifDev->devType = DevType;
		NdisZeroMemory(pNewVifDev->CUR_MAC, MAC_ADDR_LEN);
		NdisCopyMemory(pNewVifDev->CUR_MAC, pNewNetDev->dev_addr, MAC_ADDR_LEN);

		insertTailList(&pAd->Cfg80211VifDevSet.vifDevList, (PLIST_ENTRY)pNewVifDev);	
		DBGPRINT(RT_DEBUG_TRACE, ("Add CFG80211 VIF Device, Type: %d.\n", pNewVifDev->devType));
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Error in alloc mem in New CFG80211 VIF Function.\n"));
	}
}

VOID RTMP_CFG80211_RemoveVifEntry(
        IN      VOID                 *pAdSrc,
		IN      PNET_DEV pNewNetDev)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	PLIST_ENTRY     pListEntry = NULL;

	pListEntry = (PLIST_ENTRY)RTMP_CFG80211_FindVifEntry_ByMac(pAd, pNewNetDev);	
	
	if (pListEntry)
	{
		delEntryList(&pAd->Cfg80211VifDevSet.vifDevList, pListEntry);
		os_free_mem(NULL, pListEntry);	
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Error in RTMP_CFG80211_RemoveVifEntry.\n"));
	}
}

PNET_DEV RTMP_CFG80211_VirtualIF_Get(
	IN VOID                 *pAdSrc
)
{
	//PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	//return pAd->Cfg80211VifDevSet.Cfg80211VifDev[0].net_dev;
	return NULL;
}

VOID RTMP_CFG80211_VirtualIF_CancelP2pClient(
	IN VOID 		*pAdSrc)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
        PLIST_HEADER  pCacheList = &pAd->Cfg80211VifDevSet.vifDevList;
        PCFG80211_VIF_DEV               pDevEntry = NULL;
        PLIST_ENTRY                     pListEntry = NULL;

	DBGPRINT(RT_DEBUG_ERROR, ("==> RTMP_CFG80211_VirtualIF_CancelP2pClient.\n"));

        pListEntry = pCacheList->pHead;
        pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
        while (pDevEntry != NULL)
        {
                if (pDevEntry->devType == RT_CMD_80211_IFTYPE_P2P_CLIENT)
                {
			pDevEntry->devType = RT_CMD_80211_IFTYPE_P2P_GO;	
			 DBGPRINT(RT_DEBUG_ERROR, ("==> RTMP_CFG80211_VirtualIF_CancelP2pClient HIT.\n"));
			break;
                }

                pListEntry = pListEntry->pNext;
                pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
        }
	
	pAd->flg_apcli_init = FALSE;
	pAd->ApCfg.ApCliTab[MAIN_MBSSID].dev = NULL;
}


/*
========================================================================
Routine Description:
    Open a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: open successfully
    otherwise: open fail

Note:
========================================================================
*/

static INT CFG80211_VirtualIF_Open(
	IN PNET_DEV		dev_p)
{
	VOID *pAdSrc;
	PRTMP_ADAPTER pAd = NULL;
		
	printk("=======> Open\n");
	pAdSrc = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAdSrc);
	pAd = (PRTMP_ADAPTER)pAdSrc;
	
	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %d,%s\n", __FUNCTION__, dev_p->ifindex, RTMP_OS_NETDEV_GET_DEVNAME(dev_p)));

	//if (VIRTUAL_IF_UP(pAd) != 0)
	//	return -1;

	/* increase MODULE use count */
	RT_MOD_INC_USE_COUNT();

	if (dev_p->ieee80211_ptr->iftype == RT_CMD_80211_IFTYPE_P2P_CLIENT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("ApCli_Open\n"));
		pAd->flg_apcli_init = TRUE;
		ApCli_Open(pAd, dev_p);
		return 0;
	}
	
	RTMP_OS_NETDEV_START_QUEUE(dev_p);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: <=== %s\n", __FUNCTION__, RTMP_OS_NETDEV_GET_DEVNAME(dev_p)));

	return 0;
}

/*
========================================================================
Routine Description:
    Close a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: close successfully
    otherwise: close fail

Note:
========================================================================
*/
static INT CFG80211_VirtualIF_Close(
	IN	PNET_DEV	dev_p)
{
	VOID *pAdSrc;
	PRTMP_ADAPTER pAd = NULL;

	pAdSrc = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAdSrc);
	pAd = (PRTMP_ADAPTER)pAdSrc;
	
	if (dev_p->ieee80211_ptr->iftype == RT_CMD_80211_IFTYPE_P2P_CLIENT)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("ApCli_Close\n"));
		CFG80211OS_ScanEnd(pAd->pCfg80211_CB, TRUE);
		if (pAd->FlgCfg80211Scanning)
			pAd->FlgCfg80211Scanning = FALSE;
		RT_MOD_DEC_USE_COUNT();
		return ApCli_Close(pAd, dev_p);
	}
	
	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, RTMP_OS_NETDEV_GET_DEVNAME(dev_p)));

	RTMP_OS_NETDEV_STOP_QUEUE(dev_p);
	
	if (netif_carrier_ok(dev_p))
		netif_carrier_off(dev_p);

	if (INFRA_ON(pAd))
		AsicEnableBssSync(pAd);
	else if (ADHOC_ON(pAd))
		AsicEnableIbssSync(pAd);
	else
		AsicDisableSync(pAd);

	//VIRTUAL_IF_DOWN(pAd);

	RT_MOD_DEC_USE_COUNT();
	return 0;
} 
	
VOID RTMP_CFG80211_VirtualIF_Init(
	IN VOID 		*pAdSrc,
	IN CHAR			*pDevName,
	IN UINT32                DevType)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	RTMP_OS_NETDEV_OP_HOOK	netDevHook, *pNetDevOps;
	PNET_DEV	new_dev_p;
	APCLI_STRUCT	*pApCliEntry;
	CHAR preIfName[12];
	UINT devNameLen = strlen(pDevName);
	UINT preIfIndex = pDevName[devNameLen-1] - 48;
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
	struct wireless_dev *pWdev;
	UINT32 MC_RowID = 0, IoctlIF = 0, Inf = INT_P2P;

	DBGPRINT(RT_DEBUG_TRACE, ("%s ---> (%s, %d)\n", __FUNCTION__, pDevName, devNameLen));
	memset(preIfName, 0, sizeof(preIfName));
	NdisCopyMemory(preIfName, pDevName, devNameLen-1);

	pNetDevOps=&netDevHook;

	DBGPRINT(RT_DEBUG_TRACE, ("%s ---> (%s, %s, %d)\n", __FUNCTION__, pDevName, preIfName, preIfIndex));

	/* init operation functions and flags */
	NdisZeroMemory(&netDevHook, sizeof(netDevHook));
	netDevHook.open = CFG80211_VirtualIF_Open;	     /* device opem hook point */
	netDevHook.stop = CFG80211_VirtualIF_Close;	     /* device close hook point */
	netDevHook.xmit = CFG80211_VirtualIF_PacketSend; /* hard transmit hook point */
	netDevHook.ioctl = CFG80211_VirtualIF_Ioctl;	 /* ioctl hook point */

#ifdef CONFIG_APSTA_MIXED_SUPPORT
#if WIRELESS_EXT >= 12
	netDevHook.iw_handler = (void *)&rt28xx_ap_iw_handler_def;
#endif /* WIRELESS_EXT >= 12 */
#endif /* CONFIG_APSTA_MIXED_SUPPORT */
		
	//if (DevType == RT_CMD_80211_IFTYPE_P2P_CLIENT)
	//	Inf = INT_APCLI;
		
	new_dev_p = RtmpOSNetDevCreate(MC_RowID, &IoctlIF, Inf, preIfIndex, sizeof(PRTMP_ADAPTER), preIfName);
	
	if (new_dev_p == NULL)
	{
		/* allocation fail, exit */
		DBGPRINT(RT_DEBUG_ERROR, ("Allocate network device fail (CFG80211)...\n"));
		return;
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Register CFG80211 I/F (%s)\n", RTMP_OS_NETDEV_GET_DEVNAME(new_dev_p)));
	}	
	
	new_dev_p->destructor =  free_netdev;
	RTMP_OS_NETDEV_SET_PRIV(new_dev_p, pAd);
	pNetDevOps->needProtcted = TRUE;

	NdisMoveMemory(&pNetDevOps->devAddr[0], &pAd->CurrentAddress[0], MAC_ADDR_LEN);

	/* 	 
		Bit1 of MAC address Byte0 is local administration bit 
		and should be set to 1 in extended multiple BSSIDs'
		Bit3~ of MAC address Byte0 is extended multiple BSSID index.
	*/	
	if (pAd->chipCap.MBSSIDMode == MBSSID_MODE1)
		pNetDevOps->devAddr[0] += 2; /* NEW BSSID */
	else
	{
#ifdef P2P_ODD_MAC_ADJUST
		if (pNetDevOps->devAddr[5] & 0x01 == 0x01)
			pNetDevOps->devAddr[5] -= 1;
		else
#endif /* P2P_ODD_MAC_ADJUST */
		pNetDevOps->devAddr[5] += FIRST_MBSSID;
	}		

	/* Shared p2p mac address is not allow for original wpa_supplicant */
	pNetDevOps->devAddr[0] ^= 1 << 2;

	switch (DevType)
	{
		case RT_CMD_80211_IFTYPE_MONITOR:
			DBGPRINT(RT_DEBUG_TRACE, ("CFG80211 I/F Monitor Type\n"));
			//RTMP_OS_NETDEV_SET_TYPE_MONITOR(new_dev_p);	
			break;

		case RT_CMD_80211_IFTYPE_P2P_CLIENT:
			pNetDevOps->priv_flags = INT_APCLI;
			pAd->ApCfg.ApCliTab[MAIN_MBSSID].dev = NULL;
			pApCliEntry = &pAd->ApCfg.ApCliTab[MAIN_MBSSID];
			pApCliEntry->dev = new_dev_p;
			COPY_MAC_ADDR(pAd->P2PCurrentAddress, pNetDevOps->devAddr);
			COPY_MAC_ADDR(pApCliEntry->CurrentAddress, pAd->P2PCurrentAddress);
			break;

		case RT_CMD_80211_IFTYPE_P2P_GO:
			pNetDevOps->priv_flags = INT_P2P;
			pAd->ApCfg.MBSSID[MAIN_MBSSID].MSSIDDev = NULL;
			//The Behivaor in SetBeacon Ops	
			//pAd->ApCfg.MBSSID[MAIN_MBSSID].MSSIDDev = new_dev_p;
			pAd->VifNextMode = RT_CMD_80211_IFTYPE_AP;
			pAd->cfg80211_ctrl.isCfgInApMode = RT_CMD_80211_IFTYPE_AP;
			COPY_MAC_ADDR(pAd->P2PCurrentAddress, pNetDevOps->devAddr);
			break;

		default:
		DBGPRINT(RT_DEBUG_ERROR, ("Unknown CFG80211 I/F Type (%d)\n", DevType));
	}

	pWdev = kzalloc(sizeof(*pWdev), GFP_KERNEL);
	
	new_dev_p->ieee80211_ptr = pWdev;
	pWdev->wiphy = p80211CB->pCfg80211_Wdev->wiphy;
	SET_NETDEV_DEV(new_dev_p, wiphy_dev(pWdev->wiphy));	
	pWdev->netdev = new_dev_p;
	pWdev->iftype = DevType;
		
	RtmpOSNetDevAttach(pAd->OpMode, new_dev_p, pNetDevOps);

	AsicSetBssid(pAd, pAd->CurrentAddress); 
		
	//if (DevType == RT_CMD_80211_IFTYPE_P2P_CLIENT)
	//	pAd->flg_apcli_init = TRUE;

	/* Record the pNetDevice to Cfg80211VifDevList */
	RTMP_CFG80211_AddVifEntry(pAd, new_dev_p, DevType);

	DBGPRINT(RT_DEBUG_TRACE, ("%s <---\n", __FUNCTION__));
}

/*
========================================================================
Routine Description:
    Send a packet to WLAN.

Arguments:
    skb_p           points to our adapter
    dev_p           which WLAN network interface

Return Value:
    0: transmit successfully
    otherwise: transmit fail

Note:
========================================================================
*/
INT CFG80211_VirtualIF_PacketSend(
	IN PNDIS_PACKET 	skb_p, 
	IN PNET_DEV			dev_p)
{
	//struct sk_buff *pRxPkt = RTPKT_TO_OSPKT(skb_p);
	
	DBGPRINT(RT_DEBUG_INFO, ("%s ---> %d\n", __FUNCTION__, dev_p->ieee80211_ptr->iftype));
	
	MEM_DBG_PKT_ALLOC_INC(skb_p);

	if(!(RTMP_OS_NETDEV_STATE_RUNNING(dev_p)))
	{
		/* the interface is down */
		RELEASE_NDIS_PACKET(NULL, skb_p, NDIS_STATUS_FAILURE);
		return 0;
	}

	if (dev_p->ieee80211_ptr->iftype == RT_CMD_80211_IFTYPE_P2P_CLIENT)
	{
		return P2P_PacketSend(skb_p, dev_p, rt28xx_packet_xmit);
	}
	
	return CFG80211_PacketSend(skb_p, dev_p, rt28xx_packet_xmit);

} /* End of CFG80211_VirtualIF_PacketSend */


VOID RTMP_CFG80211_VirtualIF_Remove(
	IN  VOID 				 *pAdSrc,
	IN	PNET_DEV			  dev_p,
	IN  UINT32                DevType)
{

	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
    	BOOLEAN isGoOn = FALSE;
	BOOLEAN Cancelled;

	if (dev_p)
	{
		if (pAd->cfg80211_ctrl.Cfg80211RocTimerRunning == TRUE) 
		{
			CFG80211DBG(RT_DEBUG_OFF, ("%s : CANCEL Cfg80211RocTimer\n", __FUNCTION__));
			RTMPCancelTimer(&pAd->cfg80211_ctrl.Cfg80211RocTimer, &Cancelled);
			pAd->cfg80211_ctrl.Cfg80211RocTimerRunning = FALSE;
		}

		pAd->apcli_wfd_connect = FALSE;
		pAd->Cfg80211VifDevSet.isGoingOn = FALSE;
		isGoOn = RTMP_CFG80211_VIF_P2P_GO_ON(pAd);
		RTMP_CFG80211_RemoveVifEntry(pAd, dev_p);
		RTMP_OS_NETDEV_STOP_QUEUE(dev_p);
		
		kalCfg80211Disconnected(dev_p, 0, NULL, 0, TRUE, GFP_KERNEL);
		synchronize_rcu();
		if (isGoOn)
		{
			RtmpOSNetDevDetach(dev_p);
			pAd->ApCfg.MBSSID[MAIN_MBSSID].MSSIDDev = NULL;
		}
		else if (pAd->flg_apcli_init)
		{
			OPSTATUS_CLEAR_FLAG(pAd, fOP_AP_STATUS_MEDIA_STATE_CONNECTED);
			DBGPRINT(RT_DEBUG_TRACE, ("ApCli_Remove\n"));
                	
			RtmpOSNetDevDetach(dev_p);

			/* Clear it as NULL to prevent latter access error. */
			pAd->flg_apcli_init = FALSE;
			pAd->ApCfg.ApCliTab[MAIN_MBSSID].dev = NULL;
			
		}
		else /* Never Opened When New Netdevice on */
		{
			RtmpOSNetDevDetach(dev_p);
		}

		/* Restore p2p mac address */
		if (!RTMP_CFG80211_VIF_P2P_GO_ON(pAd) && !RTMP_CFG80211_VIF_P2P_CLI_ON(pAd)) {
			COPY_MAC_ADDR(pAd->cfg80211_ctrl.P2PCurrentAddress,
				pAd->dummy_p2p_net_dev->dev_addr);
			DBGPRINT(RT_DEBUG_TRACE, ("%s(): P2PCurrentAddress %X:%X:%X:%X:%X:%X\n",
				__func__, PRINT_MAC(pAd->cfg80211_ctrl.P2PCurrentAddress)));
		}

		if (dev_p->ieee80211_ptr)
		{
			kfree(dev_p->ieee80211_ptr);
			dev_p->ieee80211_ptr = NULL;
		}		
	}	
	
} /* End of CFG80211_VirtualIF_Remove */

/*
========================================================================
Routine Description:
    IOCTL to WLAN.

Arguments:
    dev_p           which WLAN network interface
    rq_p            command information
    cmd             command ID

Return Value:
    0: IOCTL successfully
    otherwise: IOCTL fail

Note:
    SIOCETHTOOL     8946    New drivers use this ETHTOOL interface to
                            report link failure activity.
========================================================================
*/
INT CFG80211_VirtualIF_Ioctl(
	IN PNET_DEV				dev_p, 
	IN OUT VOID 	*rq_p, 
	IN INT 					cmd)
{

	RTMP_ADAPTER *pAd;

	pAd = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAd);

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
		return -ENETDOWN;

	DBGPRINT(RT_DEBUG_TRACE, ("%s --->\n", __FUNCTION__));

	return rt28xx_ioctl(dev_p, rq_p, cmd);

} /* End of CFG80211_VirtualIF_Ioctl */

int CFG80211_PacketSend(
        IN      PNDIS_PACKET                            pPktSrc,
        IN      PNET_DEV                                        pDev,
        IN      RTMP_NET_PACKET_TRANSMIT        Func)
{
    PRTMP_ADAPTER pAd;
    INT minIdx;
    pAd = RTMP_OS_NETDEV_GET_PRIV(pDev);
    ASSERT(pAd);
	
    pAd->RalinkCounters.PendingNdisPacketCount ++;
    NdisZeroMemory((PUCHAR)(GET_OS_PKT_CB(pPktSrc) + CB_OFF), 15);
    RTMP_SET_PACKET_SOURCE(pPktSrc, PKTSRC_NDIS);
    RTMP_SET_PACKET_MOREDATA(pPktSrc, FALSE);


	/* To Indicate from Which VIF */
	switch (pDev->ieee80211_ptr->iftype)
	{
		case RT_CMD_80211_IFTYPE_AP:
			minIdx = MIN_NET_DEVICE_FOR_CFG80211_VIF_AP;
			RTMP_SET_PACKET_OPMODE(pPktSrc, OPMODE_AP);
			break;

		case RT_CMD_80211_IFTYPE_P2P_GO:;
			minIdx = MIN_NET_DEVICE_FOR_CFG80211_VIF_P2P_GO;
			RTMP_SET_PACKET_OPMODE(pPktSrc, OPMODE_AP);
			break;	

		case RT_CMD_80211_IFTYPE_P2P_CLIENT:
			minIdx = MIN_NET_DEVICE_FOR_CFG80211_VIF_P2P_CLI;
			break;				

		case RT_CMD_80211_IFTYPE_STATION:
			minIdx = MIN_NET_DEVICE_FOR_CFG80211_VIF_STA;
			break;	

		default:
			DBGPRINT(RT_DEBUG_TRACE, ("Unknown CFG80211 I/F Type (%d)\n", pDev->ieee80211_ptr->iftype));	
			RELEASE_NDIS_PACKET(pAd, pPktSrc, NDIS_STATUS_FAILURE);
			return 0;
	}	

	//DBGPRINT(RT_DEBUG_TRACE, ("CFG80211 Packet Type (%d)\n",  MAIN_MBSSID + minIdx));
	RTMP_SET_PACKET_NET_DEVICE_MBSSID(pPktSrc, (minIdx));
	SET_OS_PKT_NETDEV(pPktSrc, pAd->net_dev);
	
	return Func(RTPKT_TO_OSPKT(pPktSrc));

	RELEASE_NDIS_PACKET(pAd, pPktSrc, NDIS_STATUS_FAILURE);
	return 0;		
}

static INT CFG80211_DummyP2pIf_Open(
	IN PNET_DEV		dev_p)
{
	struct wireless_dev *wdev = dev_p->ieee80211_ptr;

	if (!wdev)
			return -EINVAL;	

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))			
	wdev->wiphy->interface_modes |= (BIT(NL80211_IFTYPE_P2P_CLIENT)
		| BIT(NL80211_IFTYPE_P2P_GO));	
#endif

	return 0;
}

static INT CFG80211_DummyP2pIf_Close(
	IN PNET_DEV		dev_p)
{
	struct wireless_dev *wdev = dev_p->ieee80211_ptr;

	if (!wdev)
			return -EINVAL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	wdev->wiphy->interface_modes = (wdev->wiphy->interface_modes)
									& (~(BIT(NL80211_IFTYPE_P2P_CLIENT)|
									BIT(NL80211_IFTYPE_P2P_GO)));
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
	{
		extern const struct ieee80211_iface_combination *p_ra_iface_combinations_ap_sta;
		extern const INT ra_iface_combinations_ap_sta_num;
		wdev->wiphy->iface_combinations = p_ra_iface_combinations_ap_sta;
		wdev->wiphy->n_iface_combinations = ra_iface_combinations_ap_sta_num; 
	}
#endif

	wdev->iftype = NL80211_IFTYPE_STATION;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0))
	wdev->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_P2P_DEVICE);
#endif /* LINUX_VERSION_CODE 3.7.0 */
	return 0;
}

static INT CFG80211_DummyP2pIf_Ioctl(
	IN PNET_DEV				dev_p, 
	IN OUT VOID 	*rq_p, 
	IN INT 					cmd)
{
	RTMP_ADAPTER *pAd;

	pAd = RTMP_OS_NETDEV_GET_PRIV(dev_p);
	ASSERT(pAd);

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
		return -ENETDOWN;

	DBGPRINT(RT_DEBUG_TRACE, ("%s --->\n", __FUNCTION__));

	return rt28xx_ioctl(dev_p, rq_p, cmd);

}

static INT CFG80211_DummyP2pIf_PacketSend(
	IN PNDIS_PACKET 	skb_p, 
	IN PNET_DEV			dev_p)
{
	return 0;
}

VOID RTMP_CFG80211_DummyP2pIf_Remove(
	IN VOID 		*pAdSrc)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	PNET_DEV dummy_p2p_dev = (PNET_DEV)pAd->dummy_p2p_net_dev;
	
	RtmpOSNetDevProtect(1);
	if (pAd->dummy_p2p_net_dev)
	{
		RTMP_OS_NETDEV_STOP_QUEUE(pAd->dummy_p2p_net_dev);
		RtmpOSNetDevDetach(pAd->dummy_p2p_net_dev);
		if (dummy_p2p_dev->ieee80211_ptr)
                {
                        kfree(dummy_p2p_dev->ieee80211_ptr);
                        dummy_p2p_dev->ieee80211_ptr = NULL;
                }
		//RtmpOSNetDevDetach(pAd->dummy_p2p_net_dev);
		RtmpOSNetDevProtect(0);
		
		RtmpOSNetDevFree(pAd->dummy_p2p_net_dev);	
		
		RtmpOSNetDevProtect(1);		
		pAd->flg_cfg_dummy_p2p_init = FALSE;
	}
	RtmpOSNetDevProtect(0);
}
	
VOID RTMP_CFG80211_DummyP2pIf_Init(
	IN VOID 		*pAdSrc)
{
#define INF_CFG80211_DUMMY_P2P_NAME "p2p"
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
	RTMP_OS_NETDEV_OP_HOOK	netDevHook, *pNetDevOps;
	PNET_DEV	new_dev_p;
	UINT32 MC_RowID = 0, IoctlIF = 0, Inf = INT_P2P;
	UINT preIfIndex = 0;
	struct wireless_dev *pWdev;

	if (pAd->flg_cfg_dummy_p2p_init != FALSE)
		return;
	
	pNetDevOps=&netDevHook;

	/* init operation functions and flags */
	NdisZeroMemory(&netDevHook, sizeof(netDevHook));
	netDevHook.open = CFG80211_DummyP2pIf_Open;	         /* device opem hook point */
	netDevHook.stop = CFG80211_DummyP2pIf_Close;	     /* device close hook point */
	netDevHook.xmit = CFG80211_DummyP2pIf_PacketSend;    /* hard transmit hook point */
	netDevHook.ioctl = CFG80211_DummyP2pIf_Ioctl;	     /* ioctl hook point */	
	
	new_dev_p = RtmpOSNetDevCreate(MC_RowID, &IoctlIF, Inf, preIfIndex, sizeof(PRTMP_ADAPTER), INF_CFG80211_DUMMY_P2P_NAME);

	if (new_dev_p == NULL)
	{
		/* allocation fail, exit */
		DBGPRINT(RT_DEBUG_ERROR, ("Allocate network device fail (CFG80211: Dummy P2P IF)...\n"));
		return;
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Register CFG80211 I/F (%s)\n", RTMP_OS_NETDEV_GET_DEVNAME(new_dev_p)));
	}

	RTMP_OS_NETDEV_SET_PRIV(new_dev_p, pAd);

	/* Set local administration bit for unique mac address of p2p0 */
	NdisMoveMemory(&pNetDevOps->devAddr[0], &pAd->CurrentAddress[0], MAC_ADDR_LEN);
	pNetDevOps->devAddr[0] += 2;
	COPY_MAC_ADDR(pAd->cfg80211_ctrl.P2PCurrentAddress, pNetDevOps->devAddr);
	AsicSetBssid(pAd, pAd->cfg80211_ctrl.P2PCurrentAddress);

	pNetDevOps->needProtcted = TRUE;
	
	pWdev = kzalloc(sizeof(*pWdev), GFP_KERNEL);
	if (unlikely(!pWdev)) 
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Could not allocate wireless device\n"));
		return;
	}

	new_dev_p->ieee80211_ptr = pWdev;
	pWdev->wiphy = p80211CB->pCfg80211_Wdev->wiphy;
	SET_NETDEV_DEV(new_dev_p, wiphy_dev(pWdev->wiphy));	
	pWdev->netdev = new_dev_p;
	/*
	pWdev->iftype = RT_CMD_80211_IFTYPE_STATION;		
	pWdev->wiphy->interface_modes = (pWdev->wiphy->interface_modes)
									& (~(BIT(NL80211_IFTYPE_P2P_CLIENT)|
									BIT(NL80211_IFTYPE_P2P_GO)));
	*/
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0))
	pWdev->iftype = RT_CMD_80211_IFTYPE_P2P_DEVICE;
#else
	pWdev->iftype = RT_CMD_80211_IFTYPE_P2P_CLIENT;
#endif /* LINUX_VERSION_CODE: 3.7.0 */
	/* interface_modes move from IF open to init */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0))
	pWdev->wiphy->interface_modes |= (BIT(NL80211_IFTYPE_P2P_CLIENT) |
								BIT(NL80211_IFTYPE_P2P_GO) );
	pWdev->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_P2P_DEVICE);
#else
	pWdev->wiphy->interface_modes &= (~(BIT(NL80211_IFTYPE_P2P_CLIENT)
	| BIT(NL80211_IFTYPE_P2P_GO)));
#endif /* LINUX_VERSION_CODE 3.7.0 */
#endif /* LINUX_VERSION_CODE 2.6.37 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
	{
	extern const struct ieee80211_iface_combination * p_ra_iface_combinations_p2p;
	extern const INT ra_iface_combinations_p2p_num;
	pWdev->wiphy->iface_combinations = p_ra_iface_combinations_p2p;
	pWdev->wiphy->n_iface_combinations = ra_iface_combinations_p2p_num;
	}
#endif /* KERNEL_VERSION >= 3.8.0 */
	RtmpOSNetDevAttach(pAd->OpMode, new_dev_p, pNetDevOps); 
	pAd->dummy_p2p_net_dev = new_dev_p;
	pAd->flg_cfg_dummy_p2p_init = TRUE;	
}

#endif /* RT_CFG80211_SUPPORT */
#endif /* LINUX_VERSION_CODE */

