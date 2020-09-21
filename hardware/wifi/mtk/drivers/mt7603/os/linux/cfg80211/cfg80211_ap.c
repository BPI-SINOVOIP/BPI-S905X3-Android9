/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2013, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

/****************************************************************************

	Abstract:

	All related CFG80211 function body.

	History:

***************************************************************************/
#define RTMP_MODULE_OS

#if defined (HE_BD_CFG80211_SUPPORT) && defined (BD_KERNEL_VER)
#undef  LINUX_VERSION_CODE
#define LINUX_VERSION_CODE KERNEL_VERSION(2,6,39)
#endif /* HE_BD_CFG80211_SUPPORT && BD_KERNEL_VER */

#ifdef RT_CFG80211_SUPPORT
#ifdef CONFIG_AP_SUPPORT

#include "rt_config.h"

#ifdef MT_MAC
VOID write_tmac_info_beacon(RTMP_ADAPTER *pAd, INT apidx, UCHAR *tmac_buf, HTTRANSMIT_SETTING *BeaconTransmit, ULONG frmLen);
#endif /* MT_MAC */

static INT CFG80211DRV_UpdateTimIE(PRTMP_ADAPTER pAd, UINT mbss_idx, PUCHAR pBeaconFrame, UINT32 tim_ie_pos)
{
	UCHAR  ID_1B, TimFirst, TimLast, *pTim, *ptr, New_Tim_Len;
	UINT  i;
		
	ptr = pBeaconFrame + tim_ie_pos; /* TIM LOCATION */
	*ptr = IE_TIM;
	*(ptr + 2) = pAd->ApCfg.DtimCount;
	*(ptr + 3) = pAd->ApCfg.DtimPeriod;	
	
	TimFirst = 0; /* record first TIM byte != 0x00 */
	TimLast = 0;  /* record last  TIM byte != 0x00 */
	
	pTim = pAd->ApCfg.MBSSID[mbss_idx].TimBitmaps;	
	
	for(ID_1B=0; ID_1B < WLAN_MAX_NUM_OF_TIM; ID_1B++)
	{
		/* get the TIM indicating PS packets for 8 stations */
		UCHAR tim_1B = pTim[ID_1B];

		if (ID_1B == 0)
			tim_1B &= 0xfe; /* skip bit0 bc/mc */

		if (tim_1B == 0)
			continue; /* find next 1B */

		if (TimFirst == 0)
			TimFirst = ID_1B;
			
		TimLast = ID_1B;
	} 
	
	/* fill TIM content to beacon buffer */
	if (TimFirst & 0x01)
		TimFirst --; /* find the even offset byte */		

	*(ptr + 1) = 3 + (TimLast - TimFirst + 1); /* TIM IE length */
	*(ptr + 4) = TimFirst;

	for(i=TimFirst; i <= TimLast; i++)
		*(ptr + 5 + i - TimFirst) = pTim[i];

	/* bit0 means backlogged mcast/bcast */
    if (pAd->ApCfg.DtimCount == 0)
		*(ptr + 4) |= (pAd->ApCfg.MBSSID[mbss_idx].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET] & 0x01); 

	/* adjust BEACON length according to the new TIM */
	New_Tim_Len = (2 + *(ptr+1));
	
	return New_Tim_Len;
}

static UCHAR *CFG80211_find_ie(UCHAR eid, UCHAR *ies, int len)
{
	while (len > 2 && ies[0] != eid) {
		len -= ies[1] + 2;
		ies += ies[1] + 2;
	}
	if (len < 2)
		return NULL;
	if (len < 2 + ies[1])
		return NULL;
	return ies;
}

static INT CFG80211DRV_UpdateApSettingFromBeacon(PRTMP_ADAPTER pAd, UINT mbss_idx, CMD_RTPRIV_IOCTL_80211_BEACON *pBeacon)
{
	BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[mbss_idx];
	struct wifi_dev *wdev = &pMbss->wdev;
	
	const UCHAR *ssid_ie = NULL, *wpa_ie = NULL, *rsn_ie = NULL;
	//const UINT WFA_OUI = 0x0050F2;
	//const UCHAR WMM_OUI_TYPE = 0x2;
	//UCHAR *wmm_ie = NULL;
	
	const UCHAR *supp_rates_ie = NULL;
	const UCHAR *ext_supp_rates_ie = NULL, *ht_info = NULL, *ht_cap = NULL;
#ifdef DOT11_N_SUPPORT
	UCHAR *ht_cap_set = NULL; 
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
	const UCHAR CFG_HT_OP_EID = WLAN_EID_HT_OPERATION; 
#else
	const UCHAR CFG_HT_OP_EID = WLAN_EID_HT_INFORMATION;
#endif /* LINUX_VERSION_CODE: 3.5.0 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)) 
	const UCHAR CFG_WPA_EID = WLAN_EID_VENDOR_SPECIFIC;
#else
	const UCHAR CFG_WPA_EID = WLAN_EID_WPA;
#endif /* LINUX_VERSION_CODE: 3.8.0 */
	
	ssid_ie = cfg80211_find_ie(WLAN_EID_SSID, pBeacon->beacon_head+36, pBeacon->beacon_head_len-36);
	supp_rates_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, pBeacon->beacon_head+36, pBeacon->beacon_head_len-36);
	/* if it doesn't find WPA_IE in tail first 30 bytes. treat it as is not found */
	wpa_ie = cfg80211_find_ie(CFG_WPA_EID, pBeacon->beacon_tail, pBeacon->beacon_tail_len); 
	rsn_ie = cfg80211_find_ie(WLAN_EID_RSN, pBeacon->beacon_tail, pBeacon->beacon_tail_len);//wpa2 case.
	ext_supp_rates_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	ht_cap = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	ht_info = cfg80211_find_ie(CFG_HT_OP_EID, pBeacon->beacon_tail, pBeacon->beacon_tail_len);

/* Forced to add Short-GI support in SoftAP mode, it will improve RX T.P. performance with about 10%*/
#ifdef DOT11_N_SUPPORT
	if(ht_cap != NULL)
	{
		ht_cap_set = CFG80211_find_ie(WLAN_EID_HT_CAPABILITY, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
		if(pAd->CommonCfg.RegTransmitSetting.field.ShortGI == GI_400)
		{
			if(ht_cap_set)
			{
				*(ht_cap_set+2)|=(1<<5);//Set Short_GI bit to be 1
				hex_dump("ht_cap",ht_cap_set,3*(sizeof(UCHAR)));//For debug. Dump the Short-GI bit
			}
		}
	}
#endif

#if 0
	/* Not Yet use */
	const UCHAR *supp_rates_ie = NULL, *ext_supp_rates_ie = NULL, *ht_cap = NULL, *ht_info = NULL;
	supp_rates_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, pBeacon->beacon_head+36, pBeacon->beacon_head_len-36);
	ext_supp_rates_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	ht_cap = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	ht_info = cfg80211_find_ie(WLAN_EID_HT_INFORMATION, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
#endif


	/* SSID */
	
	if (ssid_ie == NULL) 
	{
		NdisMoveMemory(pMbss->Ssid, "CFG_Linux_GO", 12);
		pMbss->SsidLen = 12;
		DBGPRINT(RT_DEBUG_ERROR,("CFG: SSID Not Found In Packet\n"));
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0))
	else if (pBeacon->ssid_len != 0)
	{
		NdisZeroMemory(pMbss->Ssid, pMbss->SsidLen);
		pMbss->SsidLen = (UCHAR)pBeacon->ssid_len;
		NdisCopyMemory(pMbss->Ssid, ssid_ie+2, pMbss->SsidLen);		
		DBGPRINT(RT_DEBUG_ERROR,("\nCFG : SSID: %s, %d\n", pMbss->Ssid, pMbss->SsidLen));
	}
#endif
	else 
	{
		pMbss->SsidLen = ssid_ie[1];
		NdisCopyMemory(pMbss->Ssid, ssid_ie+2, pMbss->SsidLen);
		DBGPRINT(RT_DEBUG_TRACE,("CFG : SSID: %s, %d\n", pMbss->Ssid, pMbss->SsidLen));
	}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0))
	if (pBeacon->hidden_ssid > 0 && pBeacon->hidden_ssid < 3) 
	{
		pMbss->bHideSsid = TRUE;
		if ((pBeacon->ssid_len != 0) 
			 && (pBeacon->ssid_len <= MAX_LEN_OF_SSID))
		{
			pMbss->SsidLen = (UCHAR)pBeacon->ssid_len;
			NdisCopyMemory(pMbss->Ssid, pBeacon->ssid, pMbss->SsidLen);
			DBGPRINT(RT_DEBUG_ERROR,("80211> [Hidden] SSID: %s, %d\n", pMbss->Ssid, pMbss->SsidLen));
		}
	}
	else
		pMbss->bHideSsid = FALSE;
	
#endif /* LINUX_VERSION_CODE 3.4.0 */

	/* WMM EDCA Paramter */ 
	CFG80211_SyncPacketWmmIe(pAd, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	
	/* Security */
	CFG80211_ParseBeaconIE(pAd, pMbss, wdev, (UCHAR *)wpa_ie, (UCHAR *)rsn_ie);
	
	
	pMbss->CapabilityInfo =	CAP_GENERATE(1, 0, (wdev->WepStatus != Ndis802_11EncryptionDisabled), 
			 (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong ? 0 : 1), pAd->CommonCfg.bUseShortSlotTime, /*SpectrumMgmt*/FALSE);
			 
	/* Disable Driver-Internal Rekey */
	pMbss->WPAREKEY.ReKeyInterval = 0;
	pMbss->WPAREKEY.ReKeyMethod = DISABLE_REKEY;	
	
	if (pBeacon->interval != 0)
	{
		DBGPRINT(RT_DEBUG_TRACE,("CFG_TIM New BI %d\n", pBeacon->interval));
		pAd->CommonCfg.BeaconPeriod = (USHORT)pBeacon->interval;
	}
	
	if (pBeacon->dtim_period != 0)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG_TIM New DP %d\n", pBeacon->dtim_period));
		pAd->ApCfg.DtimPeriod = (UCHAR)pBeacon->dtim_period;
	}


	return TRUE;		
}

VOID CFG80211DRV_DisableApInterface(PRTMP_ADAPTER pAd)
{
#ifdef RT_CFG80211_P2P_SUPPORT
	UINT apidx = CFG_GO_BSSID_IDX;
#else
	UINT apidx = MAIN_MBSSID;
#endif /* RT_CFG80211_P2P_SUPPORT */
	INT startWcid = 1;
	
	/*CFG_TODO: IT Should be set fRTMP_ADAPTER_HALT_IN_PROGRESS */
	pAd->ApCfg.MBSSID[apidx].bcn_buf.bBcnSntReq = FALSE;

  	/* For AP - STA switch */
	if ((pAd->CommonCfg.BBPCurrentBW != BW_20) && (!INFRA_ON(pAd)))
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> %s, switch to BW_20\n", __FUNCTION__));
		bbp_set_bw(pAd, BW_20);
   	}

    /* Disable pre-TBTT interrupt */
    AsicSetPreTbtt(pAd, FALSE);

    if (!INFRA_ON(pAd))
    {
		/* Disable piggyback */
		RTMPSetPiggyBack(pAd, FALSE);
		AsicUpdateProtect(pAd, 0,  (ALLN_SETPROTECT|CCKSETPROTECT|OFDMSETPROTECT), TRUE, FALSE);
    }

    if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
    {
        AsicDisableSync(pAd);
    }

#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE 
	if (INFRA_ON(pAd))
		startWcid = 2;
#endif /* #ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE */

	MacTableReset(pAd, startWcid);

#ifdef RTMP_MAC_USB
    	/* For RT2870, we need to clear the beacon sync buffer. */
    	RTUSBBssBeaconExit(pAd);
#endif /* RTMP_MAC_USB */

#ifdef BCN_OFFLOAD_SUPPORT
	/* Disable pre-TBTT event */
	RT28xx_UpdateBeaconToMcu(pAd, apidx, 0, FALSE, 0, 0);
	DBGPRINT(RT_DEBUG_OFF, ("stop HW beacon PRE-TBTT event\n"));
#endif
	OPSTATUS_CLEAR_FLAG(pAd, fOP_AP_STATUS_MEDIA_STATE_CONNECTED);
	RTMP_IndicateMediaState(pAd, NdisMediaStateDisconnected);
#ifdef CONFIG_STA_SUPPORT
	/* re-assoc to STA's wdev */
	RTMP_OS_NETDEV_SET_WDEV(pAd->net_dev, &pAd->StaCfg.wdev);
#endif /*CONFIG_STA_SUPPORT*/
}

VOID CFG80211_UpdateBeacon(
	VOID                                            *pAdOrg,
	UCHAR 										    *beacon_head_buf,
	UINT32											beacon_head_len,
	UCHAR 										    *beacon_tail_buf,
	UINT32											beacon_tail_len,
	BOOLEAN											isAllUpdate)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	PCFG80211_CTRL pCfg80211_ctrl = &pAd->cfg80211_ctrl;
	HTTRANSMIT_SETTING BeaconTransmit;   /* MGMT frame PHY rate setting when operatin at Ht rate. */
	PUCHAR pBeaconFrame;	
	UCHAR *tmac_info, New_Tim_Len = 0;
	UINT32 beacon_len = 0;
	BSS_STRUCT *pMbss;
	struct wifi_dev *wdev;
	COMMON_CONFIG *pComCfg;
#ifdef RT_CFG80211_P2P_SUPPORT
	UINT apidx = CFG_GO_BSSID_IDX;
#else
	UINT apidx = MAIN_MBSSID;
#endif /*RT_CFG80211_P2P_SUPPORT*/

	UCHAR tx_hw_hdr_len = pAd->chipCap.tx_hw_hdr_len;
    UINT8 TXWISize = pAd->chipCap.TXWISize;

	pComCfg = &pAd->CommonCfg;
    pMbss = &pAd->ApCfg.MBSSID[apidx];
    wdev = &pMbss->wdev;


	if (!pMbss || !pMbss->bcn_buf.BeaconPkt)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("CFG80211 Beacon: BCN BUF NULL return!!\n"));
		return;
	}
	tmac_info = (UCHAR *)GET_OS_PKT_DATAPTR(pMbss->bcn_buf.BeaconPkt);

#ifdef MT_MAC
        if (pAd->chipCap.hif_type == HIF_MT)
        {
                pBeaconFrame = (UCHAR *)(tmac_info + tx_hw_hdr_len);
        }
        else
#endif /* MT_MAC */
        {
                pBeaconFrame = (UCHAR *)(tmac_info + TXWISize);
        }
	
	if (isAllUpdate) /* Invoke From CFG80211 OPS For setting Beacon buffer */
	{
		/* 1. Update the Buf before TIM IE */
		NdisCopyMemory(pBeaconFrame, beacon_head_buf, beacon_head_len);
		
		/* 2. Update the Location of TIM IE */
		pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon = (UCHAR)beacon_head_len;
		
		/* 3. Store the Tail Part For appending later */
		if (pCfg80211_ctrl->beacon_tail_buf != NULL)
			 os_free_mem(NULL, pCfg80211_ctrl->beacon_tail_buf);
		
		os_alloc_mem(NULL, (UCHAR **)&pCfg80211_ctrl->beacon_tail_buf, beacon_tail_len);
		if (pCfg80211_ctrl->beacon_tail_buf != NULL)
		{
			NdisCopyMemory(pCfg80211_ctrl->beacon_tail_buf, beacon_tail_buf, beacon_tail_len);
			pCfg80211_ctrl->beacon_tail_len = beacon_tail_len;
		}		
		else
		{
			pCfg80211_ctrl->beacon_tail_len = 0;
			DBGPRINT(RT_DEBUG_ERROR, ("CFG80211 Beacon: MEM ALLOC ERROR\n"));
		}
		DBGPRINT(RT_DEBUG_ERROR, ("%s: %d isAllUpdate return!\n",__FUNCTION__, __LINE__)); 

		return;  	
	}
	else /* Invoke From Beacon Timer */
	{		
		if (pAd->ApCfg.DtimCount == 0)
			pAd->ApCfg.DtimCount = pAd->ApCfg.DtimPeriod - 1;
		else
			pAd->ApCfg.DtimCount -= 1;
	}

#ifdef MT_MAC
	if (pAd->chipCap.hif_type == HIF_MT) 
	{
		//printk("bcnBuf State =====> %d \n", pMbss->bcn_buf.bcn_state);
		BOOLEAN is_pretbtt_int = FALSE;

#ifdef RTMP_PCI_SUPPORT
        USHORT FreeNum = GET_BCNRING_FREENO(pAd);
		if (FreeNum < 0) {
	    		DBGPRINT(RT_DEBUG_ERROR, ("%s()=>BSS0:BcnRing FreeNum is not enough!\n",
	                                        __FUNCTION__));
	    		return;
		}
#endif /* RTMP_PCI_SUPPORT */

        if (pMbss->bcn_buf.bcn_state != BCN_TX_IDLE) {
            DBGPRINT(RT_DEBUG_INFO, ("%s()=>BSS0:BcnPkt not idle(%d)!\n",
                                    __FUNCTION__, pMbss->bcn_buf.bcn_state));
        	APCheckBcnQHandler(pAd, apidx, &is_pretbtt_int);
            if (is_pretbtt_int == FALSE)
			{
				DBGPRINT(RT_DEBUG_INFO, ("==============> pretbtt_int not init \n"));
                return;
			}
		}
}
#endif /* MT_MAC */
	
	/* 4. Update the TIM IE */
	New_Tim_Len = (UCHAR)CFG80211DRV_UpdateTimIE(pAd, apidx, pBeaconFrame,
				pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon);

	/* 5. Update the Buffer AFTER TIM IE */
	if (pCfg80211_ctrl->beacon_tail_buf != NULL)
	{
		NdisCopyMemory(pBeaconFrame + pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon + New_Tim_Len, 
			       pCfg80211_ctrl->beacon_tail_buf, pCfg80211_ctrl->beacon_tail_len);
		
		beacon_len = pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon + pCfg80211_ctrl->beacon_tail_len 
			     + New_Tim_Len;
	}
	else
	{
		 DBGPRINT(RT_DEBUG_TRACE, ("BEACON ====> CFG80211_UpdateBeacon OOPS\n"));
		 return;
	}	 

#if 0 //For Debug
	printk("TimIELocationInBeacon ===> %d\n", pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon);
	printk("New_Tim_Len =============> %d\n", New_Tim_Len);
	printk("beacon_tail_len =========> %d\n", pCfg80211_ctrl->beacon_tail_len);
	printk("Total_Beacon_Len =========> %d\n", beacon_len);
 #endif
 
    BeaconTransmit.word = 0;
	/* Should be Find the P2P IE Then Set Basic Rate to 6M */	
#ifdef RT_CFG80211_P2P_SUPPORT	
	if (RTMP_CFG80211_VIF_P2P_GO_ON(pAd)) 
	BeaconTransmit.field.MODE = MODE_OFDM; /* Use 6Mbps */
	else
#endif /*RT_CFG80211_P2P_SUPPORT*/		
		BeaconTransmit.field.MODE = MODE_CCK;
	
	BeaconTransmit.field.MCS = MCS_RATE_6;

	write_tmac_info_beacon(pAd, apidx, tmac_info, &BeaconTransmit, beacon_len);

	/* CFG_TODO */		
	RT28xx_UpdateBeaconToAsic(pAd, apidx, beacon_len, 
			pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon);		
	
}

BOOLEAN CFG80211DRV_OpsBeaconSet(VOID *pAdOrg, VOID *pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_BEACON *pBeacon = (CMD_RTPRIV_IOCTL_80211_BEACON *)pData;
	UINT apidx;
#ifdef RT_CFG80211_P2P_SUPPORT
	apidx = CFG_GO_BSSID_IDX;
#else
	apidx = MAIN_MBSSID;
#endif /*RT_CFG80211_P2P_SUPPORT*/


	CFG80211DRV_UpdateApSettingFromBeacon(pAd, apidx, pBeacon);
	CFG80211_UpdateBeacon(pAd, pBeacon->beacon_head, pBeacon->beacon_head_len,
			  	   pBeacon->beacon_tail, pBeacon->beacon_tail_len, TRUE);

	return TRUE;
}
	
BOOLEAN CFG80211DRV_OpsBeaconAdd(VOID *pAdOrg, VOID *pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_BEACON *pBeacon;
#ifdef RTMP_MAC_USB
	UCHAR num_idx;
#endif /* RTMP_MAC_USB */	
#ifdef CONFIG_AP_SUPPORT
#ifdef RTMP_MAC_USB
	BOOLEAN Cancelled;
#endif /* RTMP_MAC_USB */	
#endif /* CONFIG_AP_SUPPORT */	
	UINT i = 0;
#ifdef RT_CFG80211_P2P_SUPPORT
	UINT apidx = CFG_GO_BSSID_IDX;
#else
	UINT apidx = MAIN_MBSSID;
#endif /*RT_CFG80211_P2P_SUPPORT*/

#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	PNET_DEV pNetDev = NULL;
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */	

	BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[apidx];
	struct wifi_dev *wdev = &pMbss->wdev;
	CHAR tr_tb_idx = MAX_LEN_OF_MAC_TABLE + apidx;

	/* for Concurrent, AP/P2P GO use HW_BSSID 1 */
	//wdev->hw_bssid_idx = CFG_GO_BSSID_IDX;
	wdev->hw_bssid_idx = (UCHAR)apidx;

#ifdef RT_CFG80211_SUPPORT
#ifdef RT_CFG80211_P2P_SUPPORT
	if (!RTMP_CFG80211_VIF_P2P_GO_ON(pAd)) 
#endif /* RT_CFG80211_P2P_SUPPORT */		
		wdev->Hostapd=Hostapd_CFG;
#endif /* RT_CFG80211_SUPPORT */

	CFG80211DBG(RT_DEBUG_OFF, ("80211> %s ==>\n", __FUNCTION__));

#ifdef CONFIG_AP_SUPPORT
#ifdef RTMP_MAC_USB
	RTMPCancelTimer(&pAd->CommonCfg.BeaconUpdateTimer, &Cancelled);
#endif /* RTMP_MAC_USB */	
#endif /* CONFIG_AP_SUPPORT */	

	pBeacon = (CMD_RTPRIV_IOCTL_80211_BEACON *)pData;

#ifdef UAPSD_SUPPORT
        wdev->UapsdInfo.bAPSDCapable = TRUE;
        pMbss->CapabilityInfo |= 0x0800;
#endif /* UAPSD_SUPPORT */

	CFG80211DRV_UpdateApSettingFromBeacon(pAd, apidx, pBeacon);
	
	AsicSetRxFilter(pAd);
	
	/* Start from 0 & MT_MAC using HW_BSSID 1, TODO */
#ifdef RT_CFG80211_P2P_SUPPORT
	pAd->ApCfg.BssidNum = (CFG_GO_BSSID_IDX + 1); 
#else
	pAd->ApCfg.BssidNum = (MAIN_MBSSID + 1); 
#endif /*RT_CFG80211_P2P_SUPPORT*/
	pAd->MacTab.MsduLifeTime = 20; /* pEntry's UAPSD Q Idle Threshold */
	/* CFG_TODO */
	pAd->ApCfg.MBSSID[apidx].bcn_buf.BcnBufIdx = 0 ;
	for(i = 0; i < WLAN_MAX_NUM_OF_TIM; i++)
                pAd->ApCfg.MBSSID[apidx].TimBitmaps[i] = 0;

	pMbss->bcn_buf.bBcnSntReq = TRUE;

	/* For GO Timeout */
	pAd->ApCfg.StaIdleTimeout = 300;
	pMbss->StationKeepAliveTime = 0;
	
	AsicDisableSync(pAd);

	if (pAd->CommonCfg.Channel > 14)
		pAd->CommonCfg.PhyMode = (WMODE_A | WMODE_AN);
	else
		pAd->CommonCfg.PhyMode = (WMODE_B | WMODE_G |WMODE_GN);

	/* cfg_todo */
	wdev->bWmmCapable = TRUE;

	wdev->wdev_type = WDEV_TYPE_AP;
	wdev->func_dev = (void *)&pAd->ApCfg.MBSSID[apidx];
	wdev->sys_handle = (void *)pAd;
	wdev->func_idx = (CHAR)apidx;
	

#if 0
	wdev->tx_pkt_allowed = ApAllowToSendPacket;
	wdev->wdev_hard_tx = APHardTransmit;
	wdev->allow_data_tx = TRUE;


	AsicSetBssid(pAd, wdev->if_addr, 0x1);
	AsicSetDevMac(pAd, wdev->if_addr, 0x1);
#endif

#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	/* Using netDev ptr from VifList if VifDevList Exist */
	if ((pAd->cfg80211_ctrl.Cfg80211VifDevSet.vifDevList.size > 0) &&
	   ((pNetDev = RTMP_CFG80211_FindVifEntry_ByType(pAd, RT_CMD_80211_IFTYPE_P2P_GO)) != NULL))	
	{
		wdev->if_dev = pNetDev;
		COPY_MAC_ADDR(wdev->bssid, pNetDev->dev_addr);
		COPY_MAC_ADDR(wdev->if_addr, pNetDev->dev_addr);	
		
		RTMP_OS_NETDEV_SET_WDEV(pNetDev, wdev);
		RTMP_OS_NETDEV_SET_PRIV(pNetDev, pAd);
	}
	else
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */			
	{	
		wdev->if_dev = pAd->net_dev;
		COPY_MAC_ADDR(wdev->bssid, pAd->CurrentAddress);
		COPY_MAC_ADDR(wdev->if_addr, pAd->CurrentAddress);		

		/* assoc to MBSSID's wdev */
		RTMP_OS_NETDEV_SET_WDEV(pAd->net_dev, wdev);
		RTMP_OS_NETDEV_SET_PRIV(pAd->net_dev, pAd);
		//reset to INT_MAIN , because SET_PRIV would clear priv_flag
		RT_DEV_PRIV_FLAGS_SET(pAd->net_dev,INT_MAIN);
	}

	/* BC/MC Handling */
    wdev->tr_tb_idx = tr_tb_idx;
    tr_tb_set_mcast_entry(pAd, tr_tb_idx, wdev);

	/* TX */
    wdev->tx_pkt_allowed = ApAllowToSendPacket;
    wdev->wdev_hard_tx = APHardTransmit;
    wdev->tx_pkt_handle = APSendPacket;

	/* RX */
    wdev->rx_pkt_allowed = ap_rx_pkt_allow;
    wdev->rx_pkt_foward = ap_rx_foward_handle;
    wdev->rx_ps_handle = ap_rx_ps_handle;

#ifdef RT_CFG80211_P2P_SUPPORT
	wdev_bcn_buf_init(pAd, &pAd->ApCfg.MBSSID[apidx].bcn_buf);
#endif /*RT_CFG80211_P2P_SUPPORT*/	
	if (rtmp_wdev_idx_reg(pAd, wdev) < 0)
    {
		DBGPRINT(RT_DEBUG_TRACE, ("%s(): register wdev fail\n", __FUNCTION__));
	}                
	wdev->allow_data_tx = TRUE;

    AsicSetBssid(pAd, wdev->if_addr, 0x1);
    AsicSetDevMac(pAd, wdev->if_addr, 0x1);

	DBGPRINT(RT_DEBUG_TRACE, ("New AP BSSID %02x:%02x:%02x:%02x:%02x:%02x (%d)\n", 
		PRINT_MAC(wdev->bssid), pAd->CommonCfg.PhyMode));

	RTMPSetPhyMode(pAd, pAd->CommonCfg.PhyMode);

#ifdef DOT11_N_SUPPORT
	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode) && (pAd->Antenna.field.TxPath == 2))
		bbp_set_txdac(pAd, 2);
	else
#endif /* DOT11_N_SUPPORT */
		bbp_set_txdac(pAd, 0);
	
	/* Receiver Antenna selection */
	bbp_set_rxpath(pAd, pAd->Antenna.field.RxPath);

	if(!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		if (WMODE_CAP_N(pAd->CommonCfg.PhyMode) || wdev->bWmmCapable)
		{
			/* EDCA parameters used for AP's own transmission */
			if (pAd->CommonCfg.APEdcaParm.bValid == FALSE)
				set_default_ap_edca_param(pAd);

			/* EDCA parameters to be annouced in outgoing BEACON, used by WMM STA */
			if (pAd->ApCfg.BssEdcaParm.bValid == FALSE)
				set_default_sta_edca_param(pAd);

			AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);
		}
		else
			AsicSetEdcaParm(pAd, NULL);
	}

#ifdef DOT11_N_SUPPORT
#if 0
	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
		pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth = BW_20; /* Patch UI */
#endif
	AsicSetRDG(pAd, pAd->CommonCfg.bRdg);

	AsicSetRalinkBurstMode(pAd, pAd->CommonCfg.bRalinkBurstMode);
#endif /* DOT11_N_SUPPORT */

	//AsicSetBssid(pAd, pAd->CurrentAddress, 0x0); 
		
	DBGPRINT(RT_DEBUG_TRACE, ("%s():Reset WCID Table\n", __FUNCTION__));
#ifdef MT7601U
	if (IS_MT7601U(pAd))
	{
		UINT32 MACValue[128 * 2];
		UINT32 Index;

		for (Index = 0; Index < 128 * 2; Index+=2)
		{
			MACValue[Index] = 0;
			MACValue[Index + 1] = 0;
		}

		if (pAd->chipOps.BurstWrite)
			pAd->chipOps.BurstWrite(pAd, MAC_WCID_BASE, MACValue, 128 * 2);
	}
	else
#endif /* MT7601U */
	AsicDelWcidTab(pAd, WCID_ALL);
	
	pAd->MacTab.Content[0].Addr[0] = 0x01;
	pAd->MacTab.Content[0].HTPhyMode.field.MODE = MODE_OFDM;
	pAd->MacTab.Content[0].HTPhyMode.field.MCS = 3;
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE

/* for SCC */
#ifdef DOT11_N_SUPPORT
	SetCommonHT(pAd);
#endif /* DOT11_N_SUPPORT */

	/*In MCC  & p2p GO not support VHT now, */
	/*change here for support P2P GO 40 BW*/
	/*	pAd->CommonCfg.vht_bw = 0;*/
	if(pAd->CommonCfg.RegTransmitSetting.field.EXTCHA == EXTCHA_BELOW)
		pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel - 2;
	else if (pAd->CommonCfg.RegTransmitSetting.field.EXTCHA == EXTCHA_ABOVE)
		pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel + 2;
	else
#endif /*RT_CFG80211_P2P_CONCURRENT_DEVICE */

	pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel;
	
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	if (INFRA_ON(pAd))
	{
		pAd->CommonCfg.CentralChannel = pAd->StaCfg.wdev.CentralChannel;
		pAd->CommonCfg.Channel = pAd->StaCfg.wdev.channel;
		wdev->bw = pAd->StaCfg.wdev.bw;
	}
	AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel,FALSE); 
       AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);
	bbp_set_bw(pAd, wdev->bw);
#endif /*RT_CFG80211_P2P_CONCURRENT_DEVICE */
	
	AsicBBPAdjust(pAd);
	//MlmeSetTxPreamble(pAd, (USHORT)pAd->CommonCfg.TxPreamble);	
	//MlmeUpdateTxRates(pAd, FALSE, MIN_NET_DEVICE_FOR_CFG80211_VIF_P2P_GO + apidx);
#ifdef RT_CFG80211_P2P_SUPPORT
	MlmeUpdateTxRates(pAd, FALSE, (UCHAR)(MIN_NET_DEVICE_FOR_CFG80211_VIF_P2P_GO + apidx));
#else
	MlmeUpdateTxRates(pAd, FALSE, apidx);
#endif /*RT_CFG80211_P2P_SUPPORT*/
		
#ifdef DOT11_N_SUPPORT 
	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))		
		MlmeUpdateHtTxRates(pAd, MIN_NET_DEVICE_FOR_MBSSID);
#endif /* DOT11_N_SUPPORT */

	/* Disable Protection first. */		
	if (!INFRA_ON(pAd))		
		AsicUpdateProtect(pAd, 0, (ALLN_SETPROTECT|CCKSETPROTECT|OFDMSETPROTECT), TRUE, FALSE);		

	APUpdateCapabilityAndErpIe(pAd);		
#ifdef DOT11_N_SUPPORT	
	APUpdateOperationMode(pAd);
#endif /* DOT11_N_SUPPORT */		
	CFG80211_UpdateBeacon(pAd, pBeacon->beacon_head, pBeacon->beacon_head_len,
	                       pBeacon->beacon_tail, pBeacon->beacon_tail_len, TRUE);
		
#ifdef RTMP_MAC_USB
	RTUSBBssBeaconInit(pAd);
#endif /* RTMP_MAC_USB */
							   
#ifdef RTMP_MAC_USB
#ifdef CONFIG_STA_SUPPORT
    RTMPInitTimer(pAd, &pAd->CommonCfg.BeaconUpdateTimer, GET_TIMER_FUNCTION(BeaconUpdateExec), pAd, TRUE);
#endif /*CONFIG_STA_SUPPORT*/
	RTUSBBssBeaconStart(pAd);
#endif /* RTMP_MAC_USB */

	/* Enable AP BSS Sync */
	AsicEnableApBssSync(pAd, pAd->CommonCfg.BeaconPeriod);
	//pAd->P2pCfg.bSentProbeRSP = TRUE;

	AsicSetPreTbtt(pAd, TRUE);

#ifdef BCN_OFFLOAD_SUPPORT
        /*trigger pre-TBTT event here, then HW will trigger MCU event when
        HW pre-TBTT int happen, and host can send becacon or BMC to MCU*/
	RT28xx_UpdateBeaconToMcu(pAd, apidx, 0, TRUE, 0, 0);
	DBGPRINT(RT_DEBUG_OFF, ("trigger HW beacon PRE-TBTT event\n"));
#endif

#ifdef RTMP_MAC_USB
	/*
	 * Support multiple BulkIn IRP,
	 * the value on pAd->CommonCfg.NumOfBulkInIRP may be large than 1.
	 */
	for(num_idx=0; num_idx < pAd->CommonCfg.NumOfBulkInIRP; num_idx++)
	{
		RTUSBBulkReceive(pAd);
	}
#endif /* RTMP_MAC_USB */

	OPSTATUS_SET_FLAG(pAd, fOP_AP_STATUS_MEDIA_STATE_CONNECTED);	
	RTMP_IndicateMediaState(pAd, NdisMediaStateConnected);

	return TRUE;
}

BOOLEAN CFG80211DRV_ApKeyDel(
	VOID                                            *pAdOrg,
	VOID                                            *pData)
{
    PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
    CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
	MAC_TABLE_ENTRY *pEntry;
#ifdef RT_CFG80211_P2P_SUPPORT
	UINT apidx = CFG_GO_BSSID_IDX;
#else
	UINT apidx = MAIN_MBSSID;
#endif /*RT_CFG80211_P2P_SUPPORT*/

	
	pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        if (pKeyInfo->bPairwise == FALSE )
#else
        if (pKeyInfo->KeyId > 0)
#endif
	{
		DBGPRINT(RT_DEBUG_TRACE,("CFG: AsicRemoveSharedKeyEntry %d\n", pKeyInfo->KeyId));	
		AsicRemoveSharedKeyEntry(pAd, (UCHAR)apidx, pKeyInfo->KeyId);

#ifdef MT_MAC
		if (pAd->chipCap.hif_type == HIF_MT)
			CmdProcAddRemoveKey(pAd, 1, (UCHAR)apidx,
				pKeyInfo->KeyId, BSS0, SHAREDKEYTABLE,
				&pAd->SharedKey[apidx][pKeyInfo->KeyId], BROADCAST_ADDR);
#endif /* MT_MAC */
	}
	else
	{
		pEntry = MacTableLookup(pAd, pKeyInfo->MAC);
		
		if (pEntry && (pEntry->Aid != 0))
		{
			NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));
			AsicRemovePairwiseKeyEntry(pAd, (UCHAR)pEntry->wcid);

#ifdef MT_MAC
			if (pAd->chipCap.hif_type == HIF_MT)
            	CmdProcAddRemoveKey(pAd, 1, pEntry->apidx, 0, pEntry->wcid, PAIRWISEKEYTABLE, 
									&pEntry->PairwiseKey, pEntry->Addr);
#endif /* MT_MAC */

		}
	}
	
	return TRUE;
}

VOID CFG80211DRV_RtsThresholdAdd(
	VOID                                            *pAdOrg,
	UINT                                            threshold)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	
		if((threshold > 0) && (threshold <= MAX_RTS_THRESHOLD))
			pAd->CommonCfg.RtsThreshold  = (USHORT)threshold;
#ifdef CONFIG_STA_SUPPORT
		else if (threshold== 0)
			pAd->CommonCfg.RtsThreshold = MAX_RTS_THRESHOLD;
#endif /* CONFIG_STA_SUPPORT */
}


VOID CFG80211DRV_FragThresholdAdd(
	VOID                                            *pAdOrg,
	UINT                                            threshold)
{
		PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
		if (threshold > MAX_FRAG_THRESHOLD || threshold < MIN_FRAG_THRESHOLD)
		{ 
			/*Illegal FragThresh so we set it to default*/
			pAd->CommonCfg.FragmentThreshold = MAX_FRAG_THRESHOLD;
		}
		else if (threshold % 2 == 1)
		{
			/*
				The length of each fragment shall always be an even number of octets, 
				except for the last fragment of an MSDU or MMPDU, which may be either 
				an even or an odd number of octets.
			*/
			pAd->CommonCfg.FragmentThreshold = (USHORT)(threshold - 1);
		}
		else
		{
			pAd->CommonCfg.FragmentThreshold = (USHORT)threshold;
		}

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			if (pAd->CommonCfg.FragmentThreshold == MAX_FRAG_THRESHOLD)
				pAd->CommonCfg.bUseZeroToDisableFragment = TRUE;
			else
				pAd->CommonCfg.bUseZeroToDisableFragment = FALSE;
		}
#endif /* CONFIG_STA_SUPPORT */		
}

BOOLEAN CFG80211DRV_ApKeyAdd(
	VOID                                            *pAdOrg,
	VOID                                            *pData)
{
#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
	MAC_TABLE_ENTRY *pEntry = NULL;
	UCHAR Wcid = 0;
#ifdef RT_CFG80211_P2P_SUPPORT
	UCHAR apidx = CFG_GO_BSSID_IDX;
#else
	UCHAR apidx = MAIN_MBSSID;
#endif /*RT_CFG80211_P2P_SUPPORT*/

	BSS_STRUCT *pMbss = &pAd->ApCfg.MBSSID[apidx];
    struct wifi_dev *pWdev = &pMbss->wdev;
	
	DBGPRINT(RT_DEBUG_TRACE,("%s =====> \n", __FUNCTION__));
	pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;
	
	if (pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP40 || pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP104)
	{
		pWdev->WepStatus = Ndis802_11WEPEnabled;
		{
				//UCHAR CipherAlg;
				CIPHER_KEY	*pSharedKey;
				POS_COOKIE pObj;
				
				pObj = (POS_COOKIE) pAd->OS_Cookie;
				
				pSharedKey = &pAd->SharedKey[apidx][pKeyInfo->KeyId];
				NdisMoveMemory(pSharedKey->Key, pKeyInfo->KeyBuf, pKeyInfo->KeyLen);


				if (pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP40)
					pAd->SharedKey[apidx][pKeyInfo->KeyId].CipherAlg = CIPHER_WEP64;
				else
					pAd->SharedKey[apidx][pKeyInfo->KeyId].CipherAlg = CIPHER_WEP128;

			AsicAddSharedKeyEntry(pAd, (UCHAR)apidx,
				pKeyInfo->KeyId, pSharedKey);
		}		
	}
	else if(pKeyInfo->KeyType == RT_CMD_80211_KEY_WPA)
	{

	if (pKeyInfo->cipher == Ndis802_11AESEnable)
	{
		/* AES */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        if (pKeyInfo->bPairwise == FALSE )
#else
        if (pKeyInfo->KeyId > 0)
#endif	/* LINUX_VERSION_CODE 2.6.37 */	
		{
			if (pWdev->GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (GROUP) %d\n", pKeyInfo->KeyLen));
				pAd->SharedKey[apidx][pKeyInfo->KeyId].KeyLen= LEN_TK;
				NdisMoveMemory(pAd->SharedKey[apidx][pKeyInfo->KeyId].Key, pKeyInfo->KeyBuf, pKeyInfo->KeyLen);
				
				pAd->SharedKey[apidx][pKeyInfo->KeyId].CipherAlg = CIPHER_AES;

				AsicAddSharedKeyEntry(pAd, apidx, pKeyInfo->KeyId, 
						&pAd->SharedKey[apidx][pKeyInfo->KeyId]);

				GET_GroupKey_WCID(pAd, Wcid, apidx);
				RTMPSetWcidSecurityInfo(pAd, apidx, (UINT8)(pKeyInfo->KeyId), 
				pAd->SharedKey[apidx][pKeyInfo->KeyId].CipherAlg, Wcid, SHAREDKEYTABLE);

#ifdef MT_MAC
                if (pAd->chipCap.hif_type == HIF_MT)
                        CmdProcAddRemoveKey(pAd, 0, apidx, pKeyInfo->KeyId, Wcid, SHAREDKEYTABLE,
                                        	&pAd->SharedKey[apidx][pKeyInfo->KeyId], BROADCAST_ADDR);
#endif /* MT_MAC */
				
			}
		}
		else
		{
			if (pKeyInfo->MAC)
				pEntry = MacTableLookup(pAd, pKeyInfo->MAC);
				
			if(pEntry)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (PAIRWISE) %d\n", pKeyInfo->KeyLen));
				pEntry->PairwiseKey.KeyLen = LEN_TK;
				NdisCopyMemory(&pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyBuf, OFFSET_OF_PTK_TK);
				NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyLen);
				pEntry->PairwiseKey.CipherAlg = CIPHER_AES;
				
				AsicAddPairwiseKeyEntry(pAd, (UCHAR)pEntry->Aid, &pEntry->PairwiseKey);
				RTMPSetWcidSecurityInfo(pAd, pEntry->apidx,
					(UINT8)(pKeyInfo->KeyId & 0x0fff),
					pEntry->PairwiseKey.CipherAlg, (UINT8)pEntry->Aid,
					PAIRWISEKEYTABLE);

#ifdef MT_MAC
                if (pAd->chipCap.hif_type == HIF_MT)
                    	CmdProcAddRemoveKey(pAd, 0, apidx, pKeyInfo->KeyId, pEntry->wcid, PAIRWISEKEYTABLE,
                                            &pEntry->PairwiseKey, pEntry->Addr);
#endif /* MT_MAC */
				
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
				if((pAd->StaCfg.wdev.bw != pWdev->bw) && ((pAd->StaCfg.wdev.channel == pWdev->channel)))
				{
					pAd->Mlme.bStartScc = TRUE;
				}
#endif /*RT_CFG80211_P2P_CONCURRENT_DEVICE */
				
			}
			else	
			{
				DBGPRINT(RT_DEBUG_ERROR,("CFG: Set AES Security Set. (PAIRWISE) But pEntry NULL\n"));
			}
		
		}
		}else if (pKeyInfo->cipher == Ndis802_11TKIPEnable){
		/* TKIP */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        if (pKeyInfo->bPairwise == FALSE )
#else
        if (pKeyInfo->KeyId > 0)
#endif	/* LINUX_VERSION_CODE 2.6.37 */	
		{
			if (pWdev->GroupKeyWepStatus == Ndis802_11Encryption2Enabled)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set TKIP Security Set. (GROUP) %d\n", pKeyInfo->KeyLen));
				pAd->SharedKey[apidx][pKeyInfo->KeyId].KeyLen= LEN_TK;
				NdisMoveMemory(pAd->SharedKey[apidx][pKeyInfo->KeyId].Key, pKeyInfo->KeyBuf, pKeyInfo->KeyLen);
				
				pAd->SharedKey[apidx][pKeyInfo->KeyId].CipherAlg = CIPHER_TKIP;

				AsicAddSharedKeyEntry(pAd, apidx, pKeyInfo->KeyId, 
						&pAd->SharedKey[apidx][pKeyInfo->KeyId]);

				GET_GroupKey_WCID(pAd, Wcid, apidx);
				RTMPSetWcidSecurityInfo(pAd, apidx, (UINT8)(pKeyInfo->KeyId), 
				pAd->SharedKey[apidx][pKeyInfo->KeyId].CipherAlg, Wcid, SHAREDKEYTABLE);

#ifdef MT_MAC
                if (pAd->chipCap.hif_type == HIF_MT)
                        CmdProcAddRemoveKey(pAd, 0, apidx, pKeyInfo->KeyId, Wcid, SHAREDKEYTABLE,
                                        	&pAd->SharedKey[apidx][pKeyInfo->KeyId], BROADCAST_ADDR);
#endif /* MT_MAC */
				
			}
		}
		else
		{
			if (pKeyInfo->MAC)
				pEntry = MacTableLookup(pAd, pKeyInfo->MAC);
				
			if(pEntry)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set TKIP Security Set. (PAIRWISE) %d\n", pKeyInfo->KeyLen));
				pEntry->PairwiseKey.KeyLen = LEN_TK;
				NdisCopyMemory(&pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyBuf, OFFSET_OF_PTK_TK);
				NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyLen);
				pEntry->PairwiseKey.CipherAlg = CIPHER_TKIP;
				
				AsicAddPairwiseKeyEntry(pAd, (UCHAR)pEntry->Aid, &pEntry->PairwiseKey);
				RTMPSetWcidSecurityInfo(pAd, pEntry->apidx,
					(UINT8)(pKeyInfo->KeyId & 0x0fff),
					pEntry->PairwiseKey.CipherAlg, (UINT8)pEntry->Aid,
					PAIRWISEKEYTABLE);

#ifdef MT_MAC
                if (pAd->chipCap.hif_type == HIF_MT)
                    	CmdProcAddRemoveKey(pAd, 0, apidx, pKeyInfo->KeyId, pEntry->wcid, PAIRWISEKEYTABLE,
                                            &pEntry->PairwiseKey, pEntry->Addr);
#endif /* MT_MAC */
				
			}
			else	
			{
				DBGPRINT(RT_DEBUG_ERROR,("CFG: Set TKIP Security Set. (PAIRWISE) But pEntry NULL\n"));
			}
		
		}
	}
	}
#endif /* CONFIG_AP_SUPPORT */
	return TRUE;

}

INT CFG80211_StaPortSecured(
	IN VOID                                         *pAdCB,
	IN const UCHAR				* pMac,
	IN UINT						flag)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	MAC_TABLE_ENTRY *pEntry;
	STA_TR_ENTRY *tr_entry;

	pEntry = MacTableLookup(pAd, pMac);
	if (!pEntry)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Can't find pEntry in CFG80211_StaPortSecured\n"));
	}
	else
	{
		tr_entry = &pAd->MacTab.tr_entry[pEntry->wcid];
		if (flag)
		{
			pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
			pEntry->WpaState = AS_PTKINITDONE;
			tr_entry->PortSecured = WPA_802_1X_PORT_SECURED;	
			DBGPRINT(RT_DEBUG_TRACE,("AID:%d, PortSecured\n", pEntry->Aid));
		}
		else
		{
			pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
			tr_entry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;	
			DBGPRINT(RT_DEBUG_TRACE,("AID:%d, PortNotSecured\n", pEntry->Aid));
		}	
	}
	
	return 0;
}

INT CFG80211_ApStaDel(
	IN VOID                                         *pAdCB,
	IN const u8                                     *pMac)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	MAC_TABLE_ENTRY *pEntry;
	INT startWcid = 1;
	
	DBGPRINT(RT_DEBUG_TRACE, ("%s =====> \n", __FUNCTION__));
	if (pMac == NULL)
	{
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE	
		/* From WCID=2 */
		if (INFRA_ON(pAd))
			startWcid = 2;
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */			

		MacTableReset(pAd, startWcid);	
	}
	else
	{
		pEntry = MacTableLookup(pAd, pMac);
		if (pEntry)
		{
			MlmeDeAuthAction(pAd, pEntry, REASON_NO_LONGER_VALID, FALSE);
		}
		else
			DBGPRINT(RT_DEBUG_ERROR, ("Can't find pEntry(%02x:%02x:%02x:%02x:%02x:%02x) in ApStaDel\n",
						 PRINT_MAC(pMac)));
	}
	return 0;
}

INT CFG80211_setApDefaultKey(
	IN VOID                    *pAdCB,
	IN UINT 					Data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;

	DBGPRINT(RT_DEBUG_TRACE, ("Set Ap Default Key: %d\n", Data));
#ifdef RT_CFG80211_P2P_SUPPORT
	pAd->ApCfg.MBSSID[CFG_GO_BSSID_IDX].wdev.DefaultKeyId = (UCHAR)Data;
#else
	pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev.DefaultKeyId = UCHAR)Data;
#endif /*RT_CFG80211_P2P_SUPPORT*/
	
	return 0;	
}

INT CFG80211_ApStaDelSendEvent(PRTMP_ADAPTER pAd, const PUCHAR mac_addr)
{
#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
	PNET_DEV pNetDev = NULL;
	if ((pAd->cfg80211_ctrl.Cfg80211VifDevSet.vifDevList.size > 0) &&
		((pNetDev = RTMP_CFG80211_FindVifEntry_ByType(pAd, RT_CMD_80211_IFTYPE_P2P_GO)) != NULL))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CONCURRENT_DEVICE CFG : GO NOITFY THE CLIENT Disconnected\n"));
		CFG80211OS_DelSta(pNetDev, mac_addr);
	}
	else
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */				
	{
		DBGPRINT(RT_DEBUG_TRACE, ("SINGLE_DEVICE CFG : GO NOITFY THE CLIENT Disconnected\n"));
		CFG80211OS_DelSta(pAd->net_dev, mac_addr);
	}	
	return 0;
}

VOID CFG80211_setApAssocRspExtraIe(VOID *pAdOrg, UCHAR *assocresp_ies, 
        UINT32 assocresp_ies_len)
{
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        PUCHAR pAssocRespBuf = (PUCHAR)pAd->ApCfg.MBSSID[MAIN_MBSSID].AssocRespExtraIe;

    	DBGPRINT(RT_DEBUG_TRACE, ("%s: IE len = %d\n", __FUNCTION__, assocresp_ies_len));
    	if (assocresp_ies_len > sizeof(pAd->ApCfg.MBSSID[MAIN_MBSSID].AssocRespExtraIe))
    	{
        	DBGPRINT(RT_DEBUG_ERROR, ("%s: AssocResp buf size not enough\n", __FUNCTION__));
        	return;
    	}

    	NdisCopyMemory(pAssocRespBuf, assocresp_ies, assocresp_ies_len);
    	pAd->ApCfg.MBSSID[MAIN_MBSSID].AssocRespExtraIeLen = assocresp_ies_len;
}

#endif /* CONFIG_AP_SUPPORT */
#endif /* RT_CFG80211_SUPPORT */
