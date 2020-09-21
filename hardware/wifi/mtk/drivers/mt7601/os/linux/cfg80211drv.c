/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
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

#ifdef RT_CFG80211_SUPPORT

#include "rt_config.h"

#define RT_CFG80211_DEBUG /* debug use */
#define CFG80211CB			(pAd->pCfg80211_CB)

#ifdef RT_CFG80211_DEBUG
#define CFG80211DBG(__Flg, __pMsg)		DBGPRINT(__Flg, __pMsg)
#else
#define CFG80211DBG(__Flg, __pMsg)
#endif /* RT_CFG80211_DEBUG */
extern INT RtmpIoctl_rt_ioctl_siwauth(
	IN      RTMP_ADAPTER                    *pAd,
	IN      VOID                            *pData,
	IN      ULONG                            Data);

extern UCHAR P2POUIBYTE[];
BUILD_TIMER_FUNCTION(RemainOnChannelTimeout);

VOID RemainOnChannelTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
#define RESTORE_COM_CH_TIME 100

	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *) FunctionContext;
	
	DBGPRINT(RT_DEBUG_INFO, ("CFG80211_ROC: RemainOnChannelTimeout\n"));
	
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS | fRTMP_ADAPTER_NIC_NOT_EXIST)) 
	{
		CFG80211DBG(RT_DEBUG_OFF, ("RemainOnChannelTimeout Abort! pAd->Flags = %lu \n", pAd->Flags));
		return ;
	}
	
	
	if ( pAd->ApCfg.ApCliTab[MAIN_MBSSID].Valid && 
	     RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) && 
            (pAd->LatchRfRegs.Channel != pAd->ApCliMlmeAux.Channel))
	{
		/* Extend the ROC_TIME for Common Channel When P2P_CLI on */
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_ROC: ROC_Timeout APCLI_ON Channel: %d\n", pAd->ApCliMlmeAux.Channel));
		rtmp_bbp_set_bw(pAd,BW_20);
        AsicSwitchChannel(pAd, pAd->ApCliMlmeAux.Channel, FALSE);
        AsicLockChannel(pAd, pAd->ApCliMlmeAux.Channel);

		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: P2P_CLI PWR_ACTIVE ROC_END\n"));
		CFG80211_P2pClientSendNullFrame(pAd, PWR_ACTIVE);
#ifdef CONFIG_STA_SUPPORT
		if (INFRA_ON(pAd))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: CONCURRENT STA PWR_ACTIVE ROC_END\n"));
			RTMPSendNullFrame(pAd, pAd->CommonCfg.TxRate, 
								(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
								pAd->CommonCfg.bAPSDForcePowerSave ? PWR_SAVE : pAd->StaCfg.Psm);			
		}
#endif /*CONFIG_STA_SUPPORT*/
		RTMPSetTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, RESTORE_COM_CH_TIME);
	}
	else if (INFRA_ON(pAd)	&&
		(pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.BW == BW_20 || 
                pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_20) &&
		(pAd->LatchRfRegs.Channel != pAd->CommonCfg.Channel))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_ROC: ROC_Timeout INFRA_ON Channel(BW_20): %d\n", pAd->CommonCfg.Channel));
		rtmp_bbp_set_bw(pAd,BW_20);
        AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
        AsicLockChannel(pAd, pAd->CommonCfg.Channel);
#ifdef CONFIG_STA_SUPPORT
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: INFRA_ON PWR_ACTIVE ROC_END\n"));
		RTMPSendNullFrame(pAd, pAd->CommonCfg.TxRate, 
							(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
							pAd->CommonCfg.bAPSDForcePowerSave ? PWR_SAVE : pAd->StaCfg.Psm);
#endif /*CONFIG_STA_SUPPORT*/
		RTMPSetTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, RESTORE_COM_CH_TIME);		    	 
	}	
	else if (INFRA_ON(pAd)	&&
		(pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.BW == BW_40 || 
                pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_40)  && 
		(pAd->LatchRfRegs.Channel != pAd->CommonCfg.CentralChannel))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_ROC: ROC_Timeout INFRA_ON Channel(BW_40): %d\n", pAd->CommonCfg.CentralChannel));
		rtmp_bbp_set_bw(pAd,BW_40);
        AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
        AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);
#ifdef CONFIG_STA_SUPPORT
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: INFRA_ON PWR_ACTIVE ROC_END\n"));
		RTMPSendNullFrame(pAd, pAd->CommonCfg.TxRate, 
							(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
							pAd->CommonCfg.bAPSDForcePowerSave ? PWR_SAVE : pAd->StaCfg.Psm);
#endif /*CONFIG_STA_SUPPORT*/
		RTMPSetTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, RESTORE_COM_CH_TIME);		    	 
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_ROC: RemainOnChannelTimeout -- FINISH\n"));
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
     if(pAd->flg_cfg_dummy_p2p_init)
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,7,2))
	cfg80211_remain_on_channel_expired(pAd->dummy_p2p_net_dev->ieee80211_ptr, pAd->CfgChanInfo.cookie,  pAd->CfgChanInfo.chan,  GFP_KERNEL);
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(3,6,0))
        	cfg80211_remain_on_channel_expired((struct wireless_dev *)(pAd->dummy_p2p_net_dev), pAd->CfgChanInfo.cookie, 
                  	                           pAd->CfgChanInfo.chan, pAd->CfgChanInfo.ChanType, GFP_KERNEL);
#else
		cfg80211_remain_on_channel_expired(pAd->dummy_p2p_net_dev, pAd->CfgChanInfo.cookie, 
                  	                           pAd->CfgChanInfo.chan, pAd->CfgChanInfo.ChanType, GFP_KERNEL);
#endif
#endif
		pAd->Cfg80211RocTimerRunning = FALSE;
	}
		
}

VOID CFG80211_P2pClientSendNullFrame(
    IN VOID                                     *pAdCB,
    IN INT                                       PwrMgmt)
{
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	MAC_TABLE_ENTRY *pEntry;

        pEntry = MacTableLookup(pAd, pAd->ApCliMlmeAux.Bssid);
        if (pEntry == NULL)
        {
        	DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_ROC: Can't Find In Table: %02x:%02x:%02x:%02x:%02x:%02x\n",
                	                           PRINT_MAC(pAd->ApCliMlmeAux.Bssid)));
        }
        else
        {
                ApCliRTMPSendNullFrame(pAd,
                                       RATE_6,
                                       (CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE)) ? TRUE:FALSE,
                                       pEntry, PwrMgmt);
                OS_WAIT(20);
        }
}

INT CFG80211DRV_IoctlHandle(
	IN	VOID					*pAdSrc,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq,
	IN	INT						cmd,
	IN	USHORT					subcmd,
	IN	VOID					*pData,
	IN	ULONG					Data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
#ifdef CONFIG_AP_SUPPORT
	struct ieee80211_mgmt *mgmt;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))	
	INT offset = sizeof(HEADER_802_11) + 12;
#endif
#endif
	UINT32 *pChanList = (UINT32 *) pData;

	switch(cmd)
	{
		case CMD_RTPRIV_IOCTL_80211_START:
		case CMD_RTPRIV_IOCTL_80211_END:
			/* nothing to do */
			break;

		case CMD_RTPRIV_IOCTL_80211_CB_GET:
			*(VOID **)pData = (VOID *)(pAd->pCfg80211_CB);
			break;

		case CMD_RTPRIV_IOCTL_80211_CB_SET:
			pAd->pCfg80211_CB = pData;
			break;

		case CMD_RTPRIV_IOCTL_80211_CHAN_SET:
			if (CFG80211DRV_OpsSetChannel(pAd, pData) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_80211_VIF_CHG:
			if (CFG80211DRV_OpsChgVirtualInf(pAd, pData, Data) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_80211_SCAN:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
			if (CFG80211DRV_OpsScan(pAd) != TRUE)
#else
			if (CFG80211DRV_OpsScanCheckStatus(pAd, Data) != TRUE) 
#endif
			{
				return NDIS_STATUS_FAILURE;
			}
			break;

#if 1 //patch : cfg80211_scan_done() crash issue!
		case CMD_RTPRIV_IOCTL_80211_SCAN_STATUS_LOCK_INIT:
			{
				CFG80211_CB *pCfg80211_CB  = (CFG80211_CB *)pAd->pCfg80211_CB;
				
				if(Data)
				{
					NdisAllocateSpinLock(pAd, &pCfg80211_CB->scan_notify_lock);
				}
				else
				{
					NdisFreeSpinLock(&pCfg80211_CB->scan_notify_lock);
				}
			}
			break;
#endif
		case CMD_RTPRIV_IOCTL_80211_IBSS_JOIN:
			CFG80211DRV_OpsJoinIbss(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_STA_LEAVE:
			CFG80211DRV_OpsLeave(pAd, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_STA_GET:
			if (CFG80211DRV_StaGet(pAd, pData) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_80211_STA_KEY_ADD:
			CFG80211DRV_StaKeyAdd(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_AP_KEY_ADD:
			CFG80211DRV_ApKeyAdd(pAd, pData);
			break;
        case CMD_RTPRIV_IOCTL_80211_RTS_THRESHOLD_ADD:
            CFG80211DRV_RtsThresholdAdd(pAd, Data);
            break;

       case CMD_RTPRIV_IOCTL_80211_FRAG_THRESHOLD_ADD:
            CFG80211DRV_FragThresholdAdd(pAd, Data);
            break;

		case CMD_RTPRIV_IOCTL_80211_P2P_CLIENT_KEY_ADD:
			CFG80211DRV_P2pClientKeyAdd(pAd, pData);
			break;
			
		case CMD_RTPRIV_IOCTL_80211_AP_KEY_DEL:
			CFG80211DRV_ApKeyDel(pAd, pData);
			break;
			
		case CMD_RTPRIV_IOCTL_80211_AP_KEY_DEFAULT_SET:
			CFG80211_setApDefaultKey(pAd, Data);
				
#ifdef CONFIG_STA_SUPPORT
		case CMD_RTPRIV_IOCTL_80211_STA_KEY_DEFAULT_SET:
			CFG80211_setStaDefaultKey(pAd, Data);
			break;
#endif /*CONFIG_STA_SUPPORT*/

		case CMD_RTPRIV_IOCTL_80211_CONNECT_TO:
			if (Data == RT_CMD_80211_IFTYPE_P2P_CLIENT)
				CFG80211DRV_P2pClientConnect(pAd, pData);
			else
				CFG80211DRV_Connect(pAd, pData);
			break;

#ifdef RFKILL_HW_SUPPORT
		case CMD_RTPRIV_IOCTL_80211_RFKILL:
		{
			UINT32 data = 0;
			BOOLEAN active;

			/* Read GPIO pin2 as Hardware controlled radio state */
			RTMP_IO_READ32(pAd, GPIO_CTRL_CFG, &data);
			active = !!(data & 0x04);

			if (!active)
			{
				RTMPSetLED(pAd, LED_RADIO_OFF);
				*(UINT8 *)pData = 0;
			}
			else
				*(UINT8 *)pData = 1;
		}
			break;
#endif /* RFKILL_HW_SUPPORT */

		case CMD_RTPRIV_IOCTL_80211_REG_NOTIFY_TO:
			CFG80211DRV_RegNotify(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_UNREGISTER:
			CFG80211_UnRegister(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_BANDINFO_GET:
		{
			CFG80211_BAND *pBandInfo = (CFG80211_BAND *)pData;
			CFG80211_BANDINFO_FILL(pAd, pBandInfo);
		}
			break;

		case CMD_RTPRIV_IOCTL_80211_SURVEY_GET:
			CFG80211DRV_SurveyGet(pAd, pData);
			break;
			
		case CMD_RTPRIV_IOCTL_80211_EXTRA_IES_SET:
			CFG80211DRV_OpsExtraIesSet(pAd);	
			break;

		case CMD_RTPRIV_IOCTL_80211_REMAIN_ON_CHAN_SET:			
			CFG80211DRV_OpsRemainOnChannel(pAd, pData, Data);			 		
			break;				
		case CMD_RTPRIV_IOCTL_80211_CANCEL_REMAIN_ON_CHAN_SET:
			CFG80211DRV_OpsCancelRemainOnChannel(pAd, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_MGMT_FRAME_REG:
			CFG80211DRV_OpsMgmtFrameProbeRegister(pAd, pData, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_ACTION_FRAME_REG:
			CFG80211DRV_OpsMgmtFrameActionRegister(pAd, pData, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_CHANNEL_LOCK:
			if (pAd->LatchRfRegs.Channel != Data)
			{
				DBGPRINT(RT_DEBUG_INFO, ("CFG80211_PKT: TX CHANNEL_LOCK %lu\n", Data));	
				AsicSwitchChannel(pAd, Data, FALSE);
				AsicLockChannel(pAd, Data);
				
				DBGPRINT(RT_DEBUG_INFO, ("OFF-Channel Send Packet:  %lu-%d\n", Data, pAd->LatchRfRegs.Channel));
			}
			else
				DBGPRINT(RT_DEBUG_INFO, ("OFF-Channel Channel Equal:  %lu-%d\n", Data, pAd->LatchRfRegs.Channel));
			break;
			
		case CMD_RTPRIV_IOCTL_80211_CHANNEL_RESTORE:
			if (pAd->Cfg80211RocTimerRunning == TRUE)
			{ 
				if (pAd->LatchRfRegs.Channel != pAd->CfgChanInfo.ChanId)
				{
					AsicSwitchChannel(pAd, pAd->CfgChanInfo.ChanId, FALSE);
					AsicLockChannel(pAd, pAd->CfgChanInfo.ChanId);
					DBGPRINT(RT_DEBUG_TRACE, ("After OFF-Channel Send, restore to ROC: %d\n", pAd->CfgChanInfo.ChanId));
				}
				else
					DBGPRINT(RT_DEBUG_TRACE, ("After OFF-Channel Send, Keep in ROC: %d\n", pAd->CfgChanInfo.ChanId));	
			}
			else
			{
				if (pAd->LatchRfRegs.Channel != pAd->CommonCfg.Channel)
				{			
					AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
					AsicLockChannel(pAd, pAd->CommonCfg.Channel);
					DBGPRINT(RT_DEBUG_TRACE, ("After OFF-Channel Send, restore to Common: %d\n", pAd->CommonCfg.Channel));
				}
					DBGPRINT(RT_DEBUG_TRACE, ("After OFF-Channel Send, Keep in Common: %d\n", pAd->CommonCfg.Channel));	
			}	
			break;

		case CMD_RTPRIV_IOCTL_80211_MGMT_FRAME_SEND:
			/* send a managment frame */
			pAd->TxStatusInUsed = TRUE;
			pAd->TxStatusSeq = pAd->Sequence;
			if (pData != NULL) 
			{
#ifdef WFD_SUPPORT
				if (pAd->StaCfg.WfdCfg.bSuppInsertWfdIe)
				{
					PP2P_PUBLIC_FRAME	pFrame = (PP2P_PUBLIC_FRAME)pData;
					ULONG	WfdIeLen = 0, WfdIeBitmap = 0;
				
					switch (pFrame->p80211Header.FC.SubType) 
					{
						case SUBTYPE_BEACON:
						case SUBTYPE_PROBE_REQ:
						case SUBTYPE_ASSOC_REQ:
						case SUBTYPE_REASSOC_REQ:
							WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
								(0x1 << SUBID_WFD_COUPLED_SINK_INFO);
							break;	

						case SUBTYPE_ASSOC_RSP:
						case SUBTYPE_REASSOC_RSP:
							WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
								(0x1 << SUBID_WFD_COUPLED_SINK_INFO) | (0x1 << SUBID_WFD_SESSION_INFO);
							break;	

						case SUBTYPE_PROBE_RSP:
							WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
								(0x1 << SUBID_WFD_COUPLED_SINK_INFO) | (0x1 << SUBID_WFD_SESSION_INFO);
							break;	

						case SUBTYPE_ACTION:
							if ((pFrame->Category == CATEGORY_PUBLIC) &&
								(pFrame->Action == ACTION_WIFI_DIRECT))
							{
								switch (pFrame->Subtype) 
								{
									case GO_NEGOCIATION_REQ:
									case GO_NEGOCIATION_RSP:
									case GO_NEGOCIATION_CONFIRM:
									case P2P_PROVISION_REQ:
										WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
											(0x1 << SUBID_WFD_COUPLED_SINK_INFO);
										break;
				
									case P2P_INVITE_REQ:
									case P2P_INVITE_RSP:
									case P2P_PROVISION_RSP:
										WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
											(0x1 << SUBID_WFD_COUPLED_SINK_INFO) | (0x1 << SUBID_WFD_SESSION_INFO);
										break;
								}
							}
							break;
					}
				
					if (WfdIeBitmap > 0)
					{
						PUCHAR		pOutBuffer;
						NDIS_STATUS   NStatus;
						
						NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);  /* Get an unused nonpaged memory */
						if (NStatus != NDIS_STATUS_SUCCESS)
							DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
						else
						{
							memcpy(pOutBuffer, pData, Data);
							WfdMakeWfdIE(pAd, WfdIeBitmap, pOutBuffer + Data, &WfdIeLen);
							Data += WfdIeLen;
							
							if (pAd->pTxStatusBuf != NULL)
								os_free_mem(NULL, pAd->pTxStatusBuf);
							
							os_alloc_mem(NULL, (UCHAR **)&pAd->pTxStatusBuf, Data);
							if (pAd->pTxStatusBuf != NULL)
							{
								NdisCopyMemory(pAd->pTxStatusBuf, pOutBuffer, Data);
								pAd->TxStatusBufLen = Data;
							}
							else
							{
								DBGPRINT(RT_DEBUG_ERROR, ("CFG_TX_STATUS: MEM ALLOC ERROR\n"));
								MlmeFreeMemory(pAd, pOutBuffer);
								return NDIS_STATUS_FAILURE;
							}
							MiniportMMRequest(pAd, 0, pOutBuffer, Data);
						}
					}
				}
				else
#endif /* WFD_SUPPORT */
				{
					if (pAd->pTxStatusBuf != NULL)
					{
						os_free_mem(NULL, pAd->pTxStatusBuf);
						pAd->pTxStatusBuf = NULL;
					}

					os_alloc_mem(NULL, (UCHAR **)&pAd->pTxStatusBuf, Data);
					if (pAd->pTxStatusBuf != NULL)
					{
						NdisCopyMemory(pAd->pTxStatusBuf, pData, Data);
						pAd->TxStatusBufLen = Data;
					}
					else
					{
						DBGPRINT(RT_DEBUG_ERROR, ("CFG_TX_STATUS: MEM ALLOC ERROR\n"));
						return NDIS_STATUS_FAILURE;
					}

					CFG80211DRV_PrintFrameType(pAd, "TX", pData, Data);
#ifdef CONFIG_AP_SUPPORT
					//struct ieee80211_mgmt *mgmt;
					mgmt = (struct ieee80211_mgmt *)pData;
					if (ieee80211_is_probe_resp(mgmt->frame_control))
					{
						//BOOLEAN res;
						//INT offset = sizeof(HEADER_802_11) + 12;
						#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))	
								CFG80211_SyncPacketWmmIe(pAd, pData + offset , Data - offset);
						#endif
							
						//hex_dump("probe_rsp:", pData, Data);
					
					 } /* end of ieee80211_is_probe_resp */
#endif /* CONFIG_AP_SUPPORT */
					MiniportMMRequest(pAd, 0, pData, Data);
				}
			}
			break;

		case CMD_RTPRIV_IOCTL_80211_REMAIN_ON_CHAN_DUR_TIMER_INIT:
			DBGPRINT(RT_DEBUG_TRACE, ("ROC TIMER INIT\n"));
			RTMPInitTimer(pAd, &pAd->Cfg80211RemainOnChannelDurationTimer, GET_TIMER_FUNCTION(RemainOnChannelTimeout), pAd, FALSE);
			break;

		case CMD_RTPRIV_IOCTL_80211_CHANNEL_LIST_SET:
			DBGPRINT(RT_DEBUG_INFO, ("CMD_RTPRIV_IOCTL_80211_CHANNEL_LIST_SET: %lu\n", Data));
			//UINT32 *pChanList = (UINT32 *) pData;

			if (pChanList != NULL) 
			{
			
				if (pAd->pCfg80211ChanList != NULL)
					os_free_mem(NULL, pAd->pCfg80211ChanList);

				os_alloc_mem(NULL, (UCHAR **)(&pAd->pCfg80211ChanList), sizeof(UINT32 *) * Data);
				if (pAd->pCfg80211ChanList != NULL)
				{
					NdisCopyMemory(pAd->pCfg80211ChanList, pChanList, sizeof(UINT32 *) * Data);
					pAd->Cfg80211ChanListLan = Data;
				}
				else
				{
					return NDIS_STATUS_FAILURE;
				}
			}	
			break;

		case CMD_RTPRIV_IOCTL_80211_BEACON_SET:
			CFG80211DRV_OpsBeaconSet(pAd, pData, 0);			
			break;
		
		case CMD_RTPRIV_IOCTL_80211_BEACON_ADD:
			CFG80211DRV_OpsBeaconSet(pAd, pData, 1);
			break;
			
		case CMD_RTPRIV_IOCTL_80211_BEACON_DEL:
#ifdef WFD_SUPPORT
			pAd->StaCfg.WfdCfg.bSuppGoOn = FALSE;
#endif /* WFD_SUPPORT */
			break;

		case CMD_RTPRIV_IOCTL_80211_CHANGE_BSS_PARM:
			CFG80211DRV_OpsChangeBssParm(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_AP_PROBE_RSP:
#if 0		
			if (pData != NULL)
			{
				if (pAd->pCfg80211RrobeRsp != NULL)
					os_free_mem(NULL, pAd->pCfg80211RrobeRsp);

				os_alloc_mem(NULL, (UCHAR **)&pAd->pCfg80211RrobeRsp, Data);
				if (pAd->pCfg80211RrobeRsp != NULL)
				{
					NdisCopyMemory(pAd->pCfg80211RrobeRsp, pData, Data);
					pAd->Cfg80211AssocRspLen = Data;
				}
				else
				{
					DBGPRINT(RT_DEBUG_TRACE, ("YF_AP: MEM ALLOC ERROR\n"));
					return NDIS_STATUS_FAILURE;
				}
				
			}
			else
				return NDIS_STATUS_FAILURE;
#endif				
			break;

		case CMD_RTPRIV_IOCTL_80211_PORT_SECURED:
			CFG80211_StaPortSecured(pAd, pData, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_AP_STA_DEL:
			CFG80211_ApStaDel(pAd, pData);
			break;
		case CMD_RTPRIV_IOCTL_80211_BITRATE_SET:
		//	pAd->CommonCfg.PhyMode = PHY_11AN_MIXED;
		//	RTMPSetPhyMode(pAd,  pAd->CommonCfg.PhyMode);
			//Set_WirelessMode_Proc(pAd, PHY_11AGN_MIXED);
			break;

		case CMD_RTPRIV_IOCTL_80211_VIF_ADD:
			if (CFG80211DRV_OpsVifAdd(pAd, pData) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

        case CMD_RTPRIV_IOCTL_80211_VIF_GET:
            //*(VOID *)pData = (VOID *)pAd->Cfg80211VifDevSet.Cfg80211VifDev[0].net_dev;
            break;

        case CMD_RTPRIV_IOCTL_80211_RESET:
            CFG80211_reSetToDefault(pAd);
            break;
		
		case CMD_RTPRIV_IOCTL_80211_P2PCLI_ASSSOC_IE_SET:
			CFG80211DRV_SetP2pCliAssocIe(pAd, pData, Data);
			break;	
			
		case CMD_RTPRIV_IOCTL_80211_CANCEL_P2P_CLIENT:
			RTMP_CFG80211_VirtualIF_CancelP2pClient(pAd);
			break;	
			
		case CMD_RTPRIV_IOCTL_80211_ANDROID_PRIV_CMD:
			//rt_android_private_command_entry(pAd, );
			break;	
#ifdef RT_P2P_SPECIFIC_WIRELESS_EVENT
		case CMD_RTPRIV_IOCTL_80211_SEND_WIRELESS_EVENT:
			CFG80211_SendWirelessEvent(pAd, pData);
			break;
#endif /* RT_P2P_SPECIFIC_WIRELESS_EVENT */
		default:
			return NDIS_STATUS_FAILURE;
	}

	return NDIS_STATUS_SUCCESS;
}

BOOLEAN CFG80211DRV_PrintFrameType(
        IN  VOID                                        *pAdOrg,
		IN	PUCHAR										 preStr,
		IN	PUCHAR										 pData,
		IN	UINT                              			 Length)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	BOOLEAN isP2pFrame = FALSE;
	struct ieee80211_mgmt *mgmt;
	mgmt = (struct ieee80211_mgmt *)pData;
	if (ieee80211_is_mgmt(mgmt->frame_control)) 
	{
		if (ieee80211_is_probe_resp(mgmt->frame_control)) 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_PKT: %s ProbeRsp Frame %d\n", preStr, pAd->LatchRfRegs.Channel));
		        if (!mgmt->u.probe_resp.timestamp)
        		{
                		struct timeval tv;
                		do_gettimeofday(&tv);
                		mgmt->u.probe_resp.timestamp = ((UINT64) tv.tv_sec * 1000000) + tv.tv_usec;
        		}
		}
		else if (ieee80211_is_disassoc(mgmt->frame_control))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s DISASSOC Frame\n", preStr));
		}
		else if (ieee80211_is_deauth(mgmt->frame_control))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s Deauth Frame\n", preStr));
		}
		else if (ieee80211_is_action(mgmt->frame_control)) 
		{
			PP2P_PUBLIC_FRAME pFrame = (PP2P_PUBLIC_FRAME)pData;
			if ((pFrame->p80211Header.FC.SubType == SUBTYPE_ACTION) &&
			    (pFrame->Category == CATEGORY_PUBLIC) &&
			    (pFrame->Action == ACTION_WIFI_DIRECT))
			{
				isP2pFrame = TRUE;
				switch (pFrame->Subtype)
				{
					case GO_NEGOCIATION_REQ:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s GO_NEGOCIACTION_REQ %d\n", 
									preStr, pAd->LatchRfRegs.Channel));
						break;	

					case GO_NEGOCIATION_RSP:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s GO_NEGOCIACTION_RSP %d\n", 
									preStr, pAd->LatchRfRegs.Channel));
						break;

					case GO_NEGOCIATION_CONFIRM:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s GO_NEGOCIACTION_CONFIRM %d\n", 
									preStr,  pAd->LatchRfRegs.Channel));
						break;

					case P2P_PROVISION_REQ:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2P_PROVISION_REQ %d\n", 
									preStr, pAd->LatchRfRegs.Channel));
						break;

					case P2P_PROVISION_RSP:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2P_PROVISION_RSP %d\n", 
									preStr, pAd->LatchRfRegs.Channel));
						break;

					case P2P_INVITE_REQ:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2P_INVITE_REQ %d\n", 
									preStr, pAd->LatchRfRegs.Channel));
						break;

					case P2P_INVITE_RSP:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2P_INVITE_RSP %d\n", 
									preStr, pAd->LatchRfRegs.Channel));
						break;
					case P2P_DEV_DIS_REQ:
                                                DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2P_DEV_DIS_REQ %d\n",
                                                                        preStr, pAd->LatchRfRegs.Channel));
						break;						
					case P2P_DEV_DIS_RSP:
                                                DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2P_DEV_DIS_RSP %d\n",
                                                                        preStr, pAd->LatchRfRegs.Channel));
                                                break;
				}
			}else if ((pFrame->p80211Header.FC.SubType == SUBTYPE_ACTION) &&
                            (pFrame->Category == CATEGORY_PUBLIC) &&
                            ((pFrame->Action == ACTION_GAS_INITIAL_REQ)  || 
				(pFrame->Action == ACTION_GAS_INITIAL_RSP )   || 
				(pFrame->Action == ACTION_GAS_COMEBACK_REQ ) || 
				(pFrame->Action == ACTION_GAS_COMEBACK_RSP))) {
                                isP2pFrame = TRUE;
			} else if (pFrame->Category == CATEGORY_VENDOR) {
				PP2P_ACTION_FRAME pP2PActFrame = (PP2P_ACTION_FRAME) pData;

				if (RTMPEqualMemory(&pP2PActFrame->Octet[2], P2POUIBYTE, 4)) {
					isP2pFrame = TRUE;
					switch (pP2PActFrame->Subtype) {
					case P2PACT_NOA:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2PACT_NOA %d\n",
							preStr, pAd->LatchRfRegs.Channel));
						break;
					case P2PACT_PERSENCE_REQ:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2PACT_PERSENCE_REQ %d\n",
							preStr, pAd->LatchRfRegs.Channel));
						break;
					case P2PACT_PERSENCE_RSP:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2PACT_PERSENCE_RSP %d\n",
							preStr, pAd->LatchRfRegs.Channel));
						break;
					case P2PACT_GO_DISCOVER_REQ:
						DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s P2PACT_GO_DISCOVER_REQ %d\n",
							preStr, pAd->LatchRfRegs.Channel));
						break;
					default:
						DBGPRINT(RT_DEBUG_ERROR,
							("[%s] unknown P2P action frame subtype (%d)\n",
							__func__, pP2PActFrame->Subtype));
						break;
					}
				}
			}
			else
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_PKT: %s ACTION Frame %d\n", preStr, pAd->LatchRfRegs.Channel));
			}
		}	
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG80211_PKT: %s UNKOWN MGMT FRAME TYPE\n", preStr));
		}
	}
	else
	{
		DBGPRINT(RT_DEBUG_INFO, ("CFG80211_PKT: %s UNKOWN FRAME TYPE\n", preStr));
	}

	return isP2pFrame;
	
}

VOID CFG80211DRV_DisableApInterface(
        VOID                                            *pAdOrg)
{
#ifdef CONFIG_AP_SUPPORT
	UINT32 Value = 0;
	UCHAR ByteValue = 0;

	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;

	pAd->ApCfg.MBSSID[MAIN_MBSSID].bBcnSntReq = FALSE;
	pAd->P2pCfg.bSentProbeRSP = FALSE;
  
	if (pAd->CommonCfg.BBPCurrentBW == BW_40)
	{
   		pAd->CommonCfg.BBPCurrentBW = BW_20; // skynien follow YF's suggestion add here , for GO/p2p switch 
   		ByteValue = 0;	
   		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &ByteValue);
   		ByteValue &= (~0x18);
   		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, ByteValue);
   	}

        /* Disable pre-tbtt interrupt */
        RTMP_IO_READ32(pAd, INT_TIMER_EN, &Value);
        Value &=0xe;
        RTMP_IO_WRITE32(pAd, INT_TIMER_EN, Value);

        if (!INFRA_ON(pAd))
        {
                /* Disable piggyback */
                RTMPSetPiggyBack(pAd, FALSE);
                AsicUpdateProtect(pAd, 0,  (ALLN_SETPROTECT|CCKSETPROTECT|OFDMSETPROTECT), TRUE, FALSE);

        }

        if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
        {
                /*RTMP_ASIC_INTERRUPT_DISABLE(pAd); */
                AsicDisableSync(pAd);

#ifdef LED_CONTROL_SUPPORT
                /* Set LED */
                RTMPSetLED(pAd, LED_LINK_DOWN);
#endif /* LED_CONTROL_SUPPORT */
        }

#ifdef RTMP_MAC_USB
        /* For RT2870, we need to clear the beacon sync buffer. */
        RTUSBBssBeaconExit(pAd);
#endif /* RTMP_MAC_USB */
#endif /* CONFIG_AP_SUPPORT */
}

VOID CFG80211DRV_OpsMgmtFrameProbeRegister(
        VOID                                            *pAdOrg,
        VOID                                            *pData,
		BOOLEAN                                          isReg)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) pAdOrg;
	PNET_DEV pNewNetDev = (PNET_DEV) pData;
	PLIST_HEADER  pCacheList = &pAd->Cfg80211VifDevSet.vifDevList;
	PCFG80211_VIF_DEV       pDevEntry = NULL;
	PLIST_ENTRY		        pListEntry = NULL;

	/* Search the CFG80211 VIF List First */
	pListEntry = pCacheList->pHead;
	pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	while (pDevEntry != NULL)
	{
		if (RTMPEqualMemory(pDevEntry->net_dev->dev_addr, pNewNetDev->dev_addr, MAC_ADDR_LEN))
			break;

		pListEntry = pListEntry->pNext;
		pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	}
		
	/* Check The Registration is for VIF Device */	
	if ((pAd->Cfg80211VifDevSet.vifDevList.size > 0) &&
		(pDevEntry != NULL))
	{
		if (isReg)
		{
			if (pDevEntry->Cfg80211ProbeReqCount < 255)
				pDevEntry->Cfg80211ProbeReqCount++;
		}
		else 
		{
			if (pDevEntry->Cfg80211ProbeReqCount > 0)
				pDevEntry->Cfg80211ProbeReqCount--;	
		}

		if (pDevEntry->Cfg80211ProbeReqCount > 0)
			pDevEntry->Cfg80211RegisterProbeReqFrame = TRUE;
		else 
			pDevEntry->Cfg80211RegisterProbeReqFrame = FALSE;			
			
		return;
	}
	
	/* IF Not Exist on VIF List, the device must be MAIN_DEV */
	if (isReg)
	{
		if (pAd->Cfg80211ProbeReqCount < 255)
			pAd->Cfg80211ProbeReqCount++;
	}
	else 
	{
		if (pAd->Cfg80211ProbeReqCount > 0)
			pAd->Cfg80211ProbeReqCount--;	
	}
	if (pAd->Cfg80211ProbeReqCount > 0)
		pAd->Cfg80211RegisterProbeReqFrame = TRUE;
	else 
	{
		pAd->Cfg80211RegisterProbeReqFrame = FALSE;
		pAd->Cfg80211ProbeReqCount = 0;
	}	
	
	DBGPRINT(RT_DEBUG_INFO, ("pAd->Cfg80211RegisterProbeReqFrame=%d[%d]\n",pAd->Cfg80211RegisterProbeReqFrame, pAd->Cfg80211ProbeReqCount));
}

VOID CFG80211DRV_OpsMgmtFrameActionRegister(
        VOID                                            *pAdOrg,
        VOID                                            *pData,
		BOOLEAN                                          isReg)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER) pAdOrg;
	PNET_DEV pNewNetDev = (PNET_DEV) pData;
	PLIST_HEADER  pCacheList = &pAd->Cfg80211VifDevSet.vifDevList;
	PCFG80211_VIF_DEV       pDevEntry = NULL;
	PLIST_ENTRY		        pListEntry = NULL;

	/* Search the CFG80211 VIF List First */
	pListEntry = pCacheList->pHead;
	pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	while (pDevEntry != NULL)
	{
		if (RTMPEqualMemory(pDevEntry->net_dev->dev_addr, pNewNetDev->dev_addr, MAC_ADDR_LEN))
			break;

		pListEntry = pListEntry->pNext;
		pDevEntry = (PCFG80211_VIF_DEV)pListEntry;
	}
		
	/* Check The Registration is for VIF Device */	
	if ((pAd->Cfg80211VifDevSet.vifDevList.size > 0) &&
		(pDevEntry != NULL))
	{
		if (isReg)
		{
			if (pDevEntry->Cfg80211ActionCount < 255)
				pDevEntry->Cfg80211ActionCount++;
		}
		else 
		{
			if (pDevEntry->Cfg80211ActionCount > 0)
				pDevEntry->Cfg80211ActionCount--;	
		}

		if (pDevEntry->Cfg80211ActionCount > 0)
			pDevEntry->Cfg80211RegisterActionFrame = TRUE;
		else 
			pDevEntry->Cfg80211RegisterActionFrame = FALSE;			
		
		DBGPRINT(RT_DEBUG_INFO, ("TYPE pDevEntry->Cfg80211RegisterActionFrame=%d[%d]\n",pDevEntry->Cfg80211RegisterActionFrame, pDevEntry->Cfg80211ActionCount));
		
		return;
	}
	
	/* IF Not Exist on VIF List, the device must be MAIN_DEV */
	if (isReg)
	{
		if (pAd->Cfg80211ActionCount < 255)
			pAd->Cfg80211ActionCount++;
	}
	else 
	{
		if (pAd->Cfg80211ActionCount > 0)
			pAd->Cfg80211ActionCount--;	
	}
	if (pAd->Cfg80211ActionCount > 0)
		pAd->Cfg80211RegisterActionFrame = TRUE;
	else 
	{
		pAd->Cfg80211RegisterActionFrame = FALSE;
		pAd->Cfg80211ActionCount = 0;
	}	
	
	DBGPRINT(RT_DEBUG_INFO, ("TYPE pAd->Cfg80211RegisterActionFrame=%d[%d]\n",pAd->Cfg80211RegisterActionFrame, pAd->Cfg80211ActionCount));
}
		
BOOLEAN CFG80211DRV_OpsVifAdd(
        VOID                                            *pAdOrg,
        VOID                                            *pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_VIF_SET *pVifInfo;	
	pVifInfo = (CMD_RTPRIV_IOCTL_80211_VIF_SET *)pData;
	
	if (pAd->Cfg80211VifDevSet.isGoingOn)
		return FALSE;

#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
	if (RTMPEqualMemory(pVifInfo->vifName, "p2p-ap0-0", 9))
		pAd->p2pForceGOAsSoftAP = TRUE;
	else
		pAd->p2pForceGOAsSoftAP = FALSE;
#endif
	pAd->Cfg80211VifDevSet.isGoingOn = TRUE;
	RTMP_CFG80211_VirtualIF_Init(pAd, pVifInfo->vifName, pVifInfo->vifType);
	return TRUE;
}

VOID CFG80211DRV_OpsChangeBssParm(
        VOID                                            *pAdOrg,
        VOID                                            *pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_BSS_PARM *pBssInfo;
	BOOLEAN TxPreamble;

	CFG80211DBG(RT_DEBUG_TRACE, ("%s\n", __FUNCTION__));

	pBssInfo = (CMD_RTPRIV_IOCTL_80211_BSS_PARM *)pData;	

	/* Short Preamble */
	if (pBssInfo->use_short_preamble != -1)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("%s: ShortPreamble %d\n", __FUNCTION__, pBssInfo->use_short_preamble));
        	pAd->CommonCfg.TxPreamble = (pBssInfo->use_short_preamble == 0 ? Rt802_11PreambleLong : Rt802_11PreambleShort);	
		TxPreamble = (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong ? 0 : 1);
		MlmeSetTxPreamble(pAd, (USHORT)pAd->CommonCfg.TxPreamble);			
	}
	
	/* CTS Protection */
	if (pBssInfo->use_cts_prot != -1)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("%s: CTS Protection %d\n", __FUNCTION__, pBssInfo->use_cts_prot));
	}
	
	/* Short Slot */
	if (pBssInfo->use_short_slot_time != -1)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("%s: Short Slot %d\n", __FUNCTION__, pBssInfo->use_short_slot_time));
	}
}

BOOLEAN CFG80211DRV_OpsSetChannel(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_CHAN *pChan;
	UINT8 ChanId;
	UINT8 IfType;
	UINT8 ChannelType;
	STRING ChStr[5] = { 0 };
	UCHAR NewBW = BW_20;
#ifdef DOT11_N_SUPPORT
	BOOLEAN FlgIsChanged;
#endif /* DOT11_N_SUPPORT */

/*
 *  enum nl80211_channel_type {
 *	NL80211_CHAN_NO_HT,
 *	NL80211_CHAN_HT20,
 *	NL80211_CHAN_HT40MINUS,
 *	NL80211_CHAN_HT40PLUS
 *  };
 */
	/* init */
	pChan = (CMD_RTPRIV_IOCTL_80211_CHAN *)pData;
	ChanId = pChan->ChanId;
	IfType = pChan->IfType;
	ChannelType = pChan->ChanType;

#ifdef DOT11_N_SUPPORT
	if (IfType != RT_CMD_80211_IFTYPE_MONITOR)
	{
		/* get channel BW */
		FlgIsChanged = TRUE;
	
		/* set to new channel BW */
		if (ChannelType == RT_CMD_80211_CHANTYPE_HT20)
		{
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_20;
			pAd->CommonCfg.HT_Disable = 0;
		} else if (ChannelType == RT_CMD_80211_CHANTYPE_HT40MINUS) {
			NewBW = BW_40;
			pAd->CommonCfg.Channel = ChanId;
			pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_BELOW;
			pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel - 2;
			CFG80211DBG(RT_DEBUG_OFF,
				("80211> EXTCHA_BELOW Central %d , channel %d\n",
				pAd->CommonCfg.CentralChannel, pAd->CommonCfg.Channel));
			/* not support NL80211_CHAN_HT40MINUS or NL80211_CHAN_HT40PLUS */
			/* i.e. primary channel = 36, secondary channel must be 40 */
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_40;
			pAd->CommonCfg.HT_Disable = 0;
		} else if (ChannelType == RT_CMD_80211_CHANTYPE_HT40PLUS) {
			NewBW = BW_40;
			pAd->CommonCfg.Channel = ChanId;
			pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_ABOVE;
			pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel + 2;
			CFG80211DBG(RT_DEBUG_OFF,
				("80211> EXTCHA_ABOVE Central %d , channel %d\n",
				pAd->CommonCfg.CentralChannel, pAd->CommonCfg.Channel));
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_40;
			pAd->CommonCfg.HT_Disable = 0;
		} else if  (ChannelType == RT_CMD_80211_CHANTYPE_NOHT) {
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_20;
			pAd->CommonCfg.HT_Disable = 1;	
		}
		
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> New BW = %d\n", pAd->CommonCfg.RegTransmitSetting.field.BW));
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> HT Disable = %d\n", pAd->CommonCfg.HT_Disable));	
	}
	else
	{
		/* for monitor mode */
		FlgIsChanged = TRUE;
		pAd->CommonCfg.HT_Disable = 0;
		pAd->CommonCfg.RegTransmitSetting.field.BW = BW_20;//BW_40;
		if (ChannelType == RT_CMD_80211_CHANTYPE_HT40MINUS) {
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_40;
			pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_BELOW;
			CFG80211DBG(RT_DEBUG_OFF,
				("pAd->CommonCfg.RegTransmitSetting.field.BW = %d\n,"
				"pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = %d\n",
				pAd->CommonCfg.RegTransmitSetting.field.BW,
				pAd->CommonCfg.RegTransmitSetting.field.EXTCHA));
		}

		if (ChannelType == RT_CMD_80211_CHANTYPE_HT40PLUS) {
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_40;
			pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_ABOVE;
			CFG80211DBG(RT_DEBUG_OFF,
				("pAd->CommonCfg.RegTransmitSetting.field.BW = %d\n,"
				"pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = %d\n",
				pAd->CommonCfg.RegTransmitSetting.field.BW,
				pAd->CommonCfg.RegTransmitSetting.field.EXTCHA));
			}
	} /* End of if */

	if (FlgIsChanged == TRUE)
		SetCommonHT(pAd);
	/* End of if */
#endif /* DOT11_N_SUPPORT */

	/* switch to the channel with Common Channel */

	sprintf(ChStr, "%d", ChanId);
	if (Set_Channel_Proc(pAd, ChStr) == FALSE)
	{
		CFG80211DBG(RT_DEBUG_ERROR, ("80211> Change channel fail!\n"));
	} /* End of if */
	else
	{
		//if (pAd->LatchRfRegs.Channel != pAd->CommonCfg.Channel)
		if (!MONITOR_ON(pAd))
		{	
			if (NewBW == BW_40) {
				rtmp_bbp_set_bw(pAd, NewBW);
				CFG80211DBG(RT_DEBUG_OFF,
					("80211> BBPCurrentBW %d\n", pAd->CommonCfg.BBPCurrentBW));
				AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
				AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);
			} else {
				CFG80211DBG(RT_DEBUG_OFF,
					("80211> BBPCurrentBW %d\n", pAd->CommonCfg.BBPCurrentBW));
				AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
				AsicLockChannel(pAd, pAd->CommonCfg.Channel);
			}
		}	
	}

	/*CFG80211DBG(RT_DEBUG_TRACE, ("80211> New CH = %d, New BW = %d with Ext[%d]\n", 
	pAd->CommonCfg.CentralChannel, pAd->CommonCfg.RegTransmitSetting.field.BW,
	pAd->CommonCfg.RegTransmitSetting.field.EXTCHA));*/
	
	if(IfType == RT_CMD_80211_IFTYPE_AP ||
	   IfType == RT_CMD_80211_IFTYPE_P2P_GO)
	{
		//p80211CB->pCfg80211_Wdev->iftype = RT_CMD_80211_IFTYPE_AP;
		//pAd->OpMode = OPMODE_AP;
		CFG80211DBG(RT_DEBUG_ERROR, ("80211> Set the channel in AP Mode\n"));
		return TRUE;
	}
#ifdef CONFIG_STA_SUPPORT
#ifdef DOT11_N_SUPPORT
	if ((IfType == RT_CMD_80211_IFTYPE_STATION) && (FlgIsChanged == TRUE))
	{
		/*
			1. Station mode;
			2. New BW settings is 20MHz but current BW is not 20MHz;
			3. New BW settings is 40MHz but current BW is 20MHz;

			Re-connect to the AP due to BW 20/40 or HT/non-HT change.
		*/
		// Set_SSID_Proc(pAd, (PSTRING)pAd->CommonCfg.Ssid);
	} 
#endif /* DOT11_N_SUPPORT */

	if (IfType == RT_CMD_80211_IFTYPE_ADHOC)
	{
		/* update IBSS beacon */
		MlmeUpdateTxRates(pAd, FALSE, 0);
		MakeIbssBeacon(pAd);
		AsicEnableIbssSync(pAd);

		Set_SSID_Proc(pAd, (PSTRING)pAd->CommonCfg.Ssid);
	} 

	if (IfType == RT_CMD_80211_IFTYPE_MONITOR)
	{
		/* reset monitor mode in the new channel */
		Set_NetworkType_Proc(pAd, "Monitor");
		RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, pChan->MonFilterFlag);
	} 
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}


BOOLEAN CFG80211DRV_OpsChgVirtualInf(
	VOID						*pAdOrg,
	VOID						*pFlgFilter,
	UINT8						IfType)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	UINT32 FlgFilter = *(UINT32 *)pFlgFilter;

    CFG80211_CB *p80211CB = pAd->pCfg80211_CB;

#ifdef CONFIG_STA_SUPPORT
	/* change type */
	if (IfType == RT_CMD_80211_IFTYPE_ADHOC)
		Set_NetworkType_Proc(pAd, "Adhoc");
	else if (IfType == RT_CMD_80211_IFTYPE_STATION ||
			(IfType == RT_CMD_80211_IFTYPE_P2P_CLIENT))
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> Change the Interface to STA Mode\n"));
		Set_NetworkType_Proc(pAd, "Infra");
#ifdef CONFIG_AP_SUPPORT
		if (pAd->VifNextMode == RT_CMD_80211_IFTYPE_AP)
			CFG80211DRV_DisableApInterface(pAd);
#endif /*CONFIG_AP_SUPPORT	*/
		pAd->VifNextMode = RT_CMD_80211_IFTYPE_STATION;
		p80211CB->pCfg80211_Wdev->iftype = RT_CMD_80211_IFTYPE_STATION;
		pAd->OpMode = OPMODE_STA;
		pAd->cfg80211_ctrl.isCfgInApMode = RT_CMD_80211_IFTYPE_STATION;
	}
	else if (IfType == RT_CMD_80211_IFTYPE_MONITOR)
	{
		/* set packet filter */
		Set_NetworkType_Proc(pAd, "Monitor");

		if (FlgFilter != 0)
		{
			UINT32 Filter;

			RTMP_IO_READ32(pAd, RX_FILTR_CFG, &Filter);

			if ((FlgFilter & RT_CMD_80211_FILTER_FCSFAIL) == \
												RT_CMD_80211_FILTER_FCSFAIL)
			{
				Filter = Filter & (~0x01);
			}
			else
				Filter = Filter | 0x01;
			/* End of if */
	
			if ((FlgFilter & RT_CMD_80211_FILTER_PLCPFAIL) == \
												RT_CMD_80211_FILTER_PLCPFAIL)
			{
				Filter = Filter & (~0x02);
			}
			else
				Filter = Filter | 0x02;
			/* End of if */
	
			if ((FlgFilter & RT_CMD_80211_FILTER_CONTROL) == \
												RT_CMD_80211_FILTER_CONTROL)
			{
				Filter = Filter & (~0xFF00);
			}
			else
				Filter = Filter | 0xFF00;
			/* End of if */
	
			if ((FlgFilter & RT_CMD_80211_FILTER_OTHER_BSS) == \
												RT_CMD_80211_FILTER_OTHER_BSS)
			{
				Filter = Filter & (~0x08);
			}
			else
				Filter = Filter | 0x08;
			/* End of if */

			RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, Filter);
			*(UINT32 *)pFlgFilter = Filter;
		} /* End of if */

		return TRUE; /* not need to set SSID */
	} /* End of if */
	else
#endif /*CONFIG_STA_SUPPORT*/
		if (IfType == RT_CMD_80211_IFTYPE_AP ||
			(IfType == RT_CMD_80211_IFTYPE_P2P_GO))	
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> Change the Interface to AP Mode\n"));		
		pAd->VifNextMode = RT_CMD_80211_IFTYPE_AP;
		pAd->cfg80211_ctrl.isCfgInApMode = RT_CMD_80211_IFTYPE_AP;
                //p80211CB->pCfg80211_Wdev->iftype = RT_CMD_80211_IFTYPE_AP;
                //pAd->OpMode = OPMODE_AP;

		return TRUE;	
	}/* End of if */

	return TRUE;

}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
BOOLEAN CFG80211DRV_OpsScan(
	VOID						*pAdOrg)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        CFG80211DBG(RT_DEBUG_TRACE, ("80211> CFG80211DRV_OpsScan ==> \n")); 

	if (pAd->FlgCfg80211Scanning == TRUE || 
		RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
	 {
		CFG80211DBG(RT_DEBUG_ERROR, ("pAd->FlgCfg80211Scanning == %d\n", 
				pAd->FlgCfg80211Scanning)); 	
		return FALSE; /* scanning */
	}	

	/* do scan */
	pAd->FlgCfg80211Scanning = TRUE;
	return TRUE;
}
#else
BOOLEAN CFG80211DRV_OpsScanCheckStatus(
	VOID						*pAdOrg,
	UINT8						 IfType)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        CFG80211DBG(RT_DEBUG_TRACE, ("80211> CFG80211DRV_OpsScan ==> \n")); 

	if (pAd->FlgCfg80211Scanning == TRUE || 
		RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
	 {
		CFG80211DBG(RT_DEBUG_ERROR, ("pAd->FlgCfg80211Scanning == %d\n", 
				pAd->FlgCfg80211Scanning)); 	
		return FALSE; /* scanning */
	}

	if ((OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)) &&
		((pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPA) || 
			(pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPAPSK) ||
			(pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPA2) ||
			(pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPA2PSK)) &&	
			(pAd->StaCfg.PortSecured == WPA_802_1X_PORT_NOT_SECURED))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("!!! Link UP, Port Not Secured! ignore this set::OID_802_11_BSSID_LIST_SCAN\n"));
		return FALSE; 
	}
			
	if (RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) &&
	    pAd->FlgCfg80211Connecting == TRUE &&
            IfType == RT_CMD_80211_IFTYPE_STATION)
	{
		DBGPRINT(RT_DEBUG_ERROR,("WARN: P2P_CLIENT In Connecting & Canncel Scan with Infra Side\n"));
		return FALSE;
	}	
#ifdef RT_CFG80211_SUPPORT	
	if (pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan == TRUE &&
        (IfType == RT_CMD_80211_IFTYPE_AP))
	{
		DBGPRINT(RT_DEBUG_ERROR,("Disable 20/40 scan!!\n"));
		return FALSE;
	}	
#endif /* RT_CFG80211_SUPPORT */

	/* do scan */
	pAd->FlgCfg80211Scanning = TRUE;
#endif /*CONFIG_STA_SUPPORT*/
	return TRUE;
}
#endif

static int CFG80211DRV_UpdateTimIE(
	VOID                                            *pAdOrg,
	PUCHAR 											pBeaconFrame,
	UINT32											tim_ie_pos)
{
#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	UCHAR  ID_1B, TimFirst, TimLast, *pTim, *ptr, New_Tim_Len;
	UINT  i;
		
	ptr = pBeaconFrame + tim_ie_pos; /* TIM LOCATION */
	*ptr = IE_TIM;
	*(ptr + 2) = pAd->ApCfg.DtimCount;
	*(ptr + 3) = pAd->ApCfg.DtimPeriod;	
	
	TimFirst = 0; /* record first TIM byte != 0x00 */
	TimLast = 0;  /* record last  TIM byte != 0x00 */
	
	pTim = pAd->ApCfg.MBSSID[MAIN_MBSSID].TimBitmaps;	
	
	for(ID_1B=0; ID_1B<WLAN_MAX_NUM_OF_TIM; ID_1B++)
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

	for(i=TimFirst; i<=TimLast; i++)
		*(ptr + 5 + i - TimFirst) = pTim[i];

	/* bit0 means backlogged mcast/bcast */
    	if (pAd->ApCfg.DtimCount == 0)
		*(ptr + 4) |= (pAd->ApCfg.MBSSID[MAIN_MBSSID].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET] & 0x01); 

	/* adjust BEACON length according to the new TIM */
	New_Tim_Len = (2 + *(ptr+1));
	
	return New_Tim_Len;
#else
	return 0;
#endif
}	

VOID CFG80211_UpdateBeacon(
	VOID                                            *pAdOrg,
	UCHAR 										    *beacon_head_buf,
	UINT32											beacon_head_len,
	UCHAR 										    *beacon_tail_buf,
	UINT32											beacon_tail_len,
	BOOLEAN											isAllUpdate)
{
#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	HTTRANSMIT_SETTING BeaconTransmit;   /* MGMT frame PHY rate setting when operatin at Ht rate. */
	TXWI_STRUC *pTxWI = &pAd->BeaconTxWI;
	//UINT8 TXWISize = pAd->chipCap.TXWISize;
	//UCHAR  *ptr, New_Tim_Len;
	UCHAR  New_Tim_Len;
	//UINT  i;
	//UINT32 longValue, beacon_len;
	UINT32 beacon_len;
	PUCHAR pBeaconFrame = (PUCHAR)pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf;

	if (isAllUpdate) /* Invoke From CFG80211 OPS For setting Beacon buffer */
	{
		/* 1. Update the Before TIM IE */
		NdisCopyMemory(pBeaconFrame, beacon_head_buf, beacon_head_len);
		
		/* 2. Update the TIM IE */
		pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon = beacon_head_len;
		
		/* 3. Store the Tail Part For appending later */
		if (pAd->beacon_tail_buf != NULL)
			 os_free_mem(NULL, pAd->beacon_tail_buf);
		
		os_alloc_mem(NULL, (UCHAR **)&pAd->beacon_tail_buf, beacon_tail_len);
		if (pAd->beacon_tail_buf != NULL)
		{
			NdisCopyMemory(pAd->beacon_tail_buf, beacon_tail_buf, beacon_tail_len);
			pAd->beacon_tail_len = beacon_tail_len;
		}		
		else
		{
			pAd->beacon_tail_len = 0;
			DBGPRINT(RT_DEBUG_ERROR, ("CFG80211 Beacon: MEM ALLOC ERROR\n"));
		}

		return;  	
	}
	else /* Invoke From Beacon Timer */
	{		
		if (pAd->ApCfg.DtimCount == 0)
			pAd->ApCfg.DtimCount = pAd->ApCfg.DtimPeriod - 1;
		else
			pAd->ApCfg.DtimCount -= 1;

		/* 1. Update the TIM IE */
		New_Tim_Len = CFG80211DRV_UpdateTimIE(pAd, pBeaconFrame, pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon);
		
		/* 2. Update the AFTER TIM IE */
		if (pAd->beacon_tail_buf != NULL)
		{
			NdisCopyMemory(pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf + pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon + New_Tim_Len, 
							pAd->beacon_tail_buf, pAd->beacon_tail_len);
		
			beacon_len = pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon + pAd->beacon_tail_len + New_Tim_Len;
		}
		else
		{
			 printk("BEACON ====> CFG80211_UpdateBeacon OOPS\n");
			 return;
		}
	}
	
    	BeaconTransmit.word = 0;
	/* Should be Find the P2P IE Then Set Basic Rate to 6M */	
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP		
	if (pAd->p2pForceGOAsSoftAP)
	{
		BeaconTransmit.field.MODE = MODE_CCK; /* Use 6Mbps */
		BeaconTransmit.field.MCS = 0;		
	}
	else{
#endif	
	if(RTMP_CFG80211_VIF_P2P_GO_ON(pAd))
		BeaconTransmit.field.MODE = MODE_OFDM; /* Use 6Mbps */
	else
		BeaconTransmit.field.MODE = MODE_CCK;

	BeaconTransmit.field.MCS = MCS_RATE_6;
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
	}
#endif

    	RTMPWriteTxWI(pAd, pTxWI, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, 0, BSS0Mcast_WCID,
                beacon_len, PID_MGMT, 0, 0, IFS_HTTXOP, FALSE, &BeaconTransmit);

			
	RT28xx_UpdateBeaconToAsic(pAd, MAIN_MBSSID, beacon_len, pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon);		
#if 0
    ptr = (PUCHAR)&pAd->BeaconTxWI;
#ifdef RT_BIG_ENDIAN
    RTMPWIEndianChange(ptr, TYPE_TXWI);
#endif

	for (i=0; i<TXWISize; i+=4)  /* 16-byte TXWI field */
	{
		longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[0] + i, longValue);
		ptr += 4;
	}

    /* update BEACON frame content. start right after the 16-byte TXWI field. */
    ptr = pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf;
#ifdef RT_BIG_ENDIAN
    RTMPFrameEndianChange(pAd, ptr, DIR_WRITE, FALSE);
#endif

	for (i= 0; i< beacon_len; i+=4)
	{
		longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[0] + TXWISize + i, longValue);
		ptr += 4;
	}

	BEACON_SYNC_STRUCT	*pBeaconSync = pAd->CommonCfg.pBeaconSync;
	
	if (pBeaconSync == NULL)
	{
		CFG80211DBG(RT_DEBUG_ERROR, ("80211> UpdateBeacon - pBeaconSync == NULL\n"));
		return;
	}
	
	ptr = (PUCHAR) (pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf + pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon);
	if ((*(ptr + 4)) & 0x01)
		pBeaconSync->DtimBitOn |= (1 << MAIN_MBSSID);
	else
		pBeaconSync->DtimBitOn &= ~(1 << MAIN_MBSSID);
#endif
#endif/*CONFIG_AP_SUPPORT*/

}

VOID CFG80211_UpdateAssocRespExtraIe(
	VOID                                            *pAdOrg,
	UCHAR 										    *assocresp_ies,
	UINT32											assocresp_ies_len)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	PUCHAR pAssocRespBuf = (PUCHAR)pAd->ApCfg.MBSSID[MAIN_MBSSID].AssocRespExtraIe;
	//PUCHAR UpdatePos;

    DBGPRINT(RT_DEBUG_TRACE, ("%s: IE len = %d\n", __FUNCTION__, assocresp_ies_len));
    if (assocresp_ies_len > sizeof(pAd->ApCfg.MBSSID[MAIN_MBSSID].AssocRespExtraIe))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("%s: AssocResp buf size not enough\n", __FUNCTION__));
        return;
    }
    NdisCopyMemory(pAssocRespBuf, assocresp_ies, assocresp_ies_len);
    pAd->ApCfg.MBSSID[MAIN_MBSSID].AssocRespExtraIeLen = assocresp_ies_len;
}

const UCHAR *cfg80211_parsing_ie(UCHAR eid, const UCHAR *ies, UINT32 len)
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

/* REF: ap_connect.c ApMakeBssBeacon */
BOOLEAN CFG80211DRV_OpsBeaconSet(
    VOID                                            *pAdOrg,
    VOID                                            *pData,
	BOOLEAN                                          isAdd)
{
#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_BEACON *pBeacon;
	UINT32 rx_filter_flag;
	BOOLEAN TxPreamble = FALSE, SpectrumMgmt = FALSE;
	BOOLEAN	bWmmCapable = FALSE;
	UCHAR	BBPR1 = 0, BBPR3 = 0;
	//UINT  i;
	INT idx;
	ULONG offset;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))		
	const UINT WFA_OUI = 0x0050F2;
	const UCHAR WMM_OUI_TYPE = 0x2;
#else
	const UCHAR *ssid_ie = NULL;
#endif
	PMULTISSID_STRUCT pMbss = &pAd->ApCfg.MBSSID[MAIN_MBSSID];
	const UCHAR *wpa_ie = NULL;
	const UCHAR *rsn_ie = NULL;
	const UCHAR *supp_rates_ie = NULL;
	//const UCHAR *ext_supp_rates_ie = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
#ifdef DOT11_N_SUPPORT
	UCHAR *ht_cap = NULL;
#endif
	//const UCHAR *ht_info = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))		
	const UINT8 *wmm_ie = NULL;
#endif
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
	const UCHAR *vendor_spec = NULL;
#endif
#if 0
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
	const UCHAR CFG_HT_OP_EID = WLAN_EID_HT_OPERATION; 
#else
	const UCHAR CFG_HT_OP_EID = WLAN_EID_HT_INFORMATION;
#endif /* LINUX_VERSION_CODE: 3.5.0 */
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)) 
	const UCHAR CFG_WPA_EID = WLAN_EID_VENDOR_SPECIFIC;
#else
	const UCHAR CFG_WPA_EID = WLAN_EID_WPA;
#endif /* LINUX_VERSION_CODE: 3.8.0 */
#endif
	PNET_DEV pNetDev = NULL;
#ifdef RTMP_MAC_USB
	UCHAR num_idx;
#endif

	CFG80211DBG(RT_DEBUG_TRACE, ("80211> CFG80211DRV_OpsBeaconSet ==> %d\n", isAdd));
	pBeacon = (CMD_RTPRIV_IOCTL_80211_BEACON *)pData;

#ifdef WFD_SUPPORT
	if (pAd->StaCfg.WfdCfg.bSuppInsertWfdIe)
	{
		ULONG TmpLen, WfdIeBitmap;

		ptr = pBeacon->beacon_tail + pBeacon->beacon_tail_len;
		WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
			(0x1 << SUBID_WFD_COUPLED_SINK_INFO);
		WfdMakeWfdIE(pAd, WfdIeBitmap, ptr, &TmpLen);
		pBeacon->beacon_tail_len += TmpLen;
	}
#endif /* WFD_SUPPORT */

	if (isAdd)
	{
		rx_filter_flag = APNORMAL;
		RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, rx_filter_flag);     /* enable RX of DMA block */
	
		pAd->ApCfg.BssidNum = 1;
		pAd->MacTab.MsduLifeTime = 20; /* default 5 seconds */
		pAd->ApCfg.MBSSID[MAIN_MBSSID].bBcnSntReq = TRUE;

		/* For GO Timeout */
		pAd->ApCfg.StaIdleTimeout = 30;
		pAd->ApCfg.MBSSID[MAIN_MBSSID].StationKeepAliveTime = 10;

		AsicDisableSync(pAd);

		if (pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED)
		{
			if (pAd->CommonCfg.Channel > 14)
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11AN_MIXED;
			else
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11BGN_MIXED;
		}
		else
		{
			if (pAd->CommonCfg.Channel > 14)
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11A;
			else
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11BG_MIXED;
		}

		TxPreamble = (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong ? 0 : 1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
		if (pBeacon->ssid_len <= IEEE80211_MAX_SSID_LEN) {
			pMbss->SsidLen = pBeacon->ssid_len;
			NdisCopyMemory(pMbss->Ssid, pBeacon->ssid, pMbss->SsidLen);
		} else
			DBGPRINT(RT_DEBUG_ERROR, ("CFG: Invalid SSID len %d\n",
				(INT)pBeacon->ssid_len));

		DBGPRINT(RT_DEBUG_TRACE, ("CFG : SSID: %s, %d\n", pMbss->Ssid, pMbss->SsidLen));

		pMbss->hidden_ssid = pBeacon->hidden_ssid;
#else
		ssid_ie = cfg80211_parsing_ie(WLAN_EID_SSID, pBeacon->beacon_head+36,
								pBeacon->beacon_head_len-36);

		NdisZeroMemory(pMbss->Ssid, sizeof(pMbss->Ssid));
		if (ssid_ie == NULL) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG: SSID Not Found In Packet\n"));
			NdisMoveMemory(pMbss->Ssid, "P2P_Linux_AP", 12);
			pMbss->SsidLen = 12;
		} else {
			pMbss->SsidLen = ssid_ie[1];
			NdisCopyMemory(pMbss->Ssid, ssid_ie+2, pMbss->SsidLen);
			DBGPRINT(RT_DEBUG_TRACE, ("CFG : SSID: %s, %d\n", pMbss->Ssid,
					pMbss->SsidLen));
		}
		pMbss->hidden_ssid = 0;
#endif /* LINUX_VERSION_CODE >= NKERNEL_VERSION(3, 2, 0) */
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
	supp_rates_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, pBeacon->beacon_head+36, pBeacon->beacon_head_len-36);

	wpa_ie = cfg80211_find_ie(CFG_WPA_EID, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	rsn_ie = cfg80211_find_ie(WLAN_EID_RSN, pBeacon->beacon_tail, pBeacon->beacon_tail_len);//wpa2 case.
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))		
        wmm_ie = cfg80211_find_vendor_ie(WFA_OUI, WMM_OUI_TYPE, pBeacon->beacon_tail, pBeacon->beacon_tail_len);	
#endif
#if 0
	ext_supp_rates_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	ht_cap = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))
	ht_info = cfg80211_find_ie(WLAN_EID_HT_OPERATION, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
#else
	ht_info = cfg80211_find_ie(WLAN_EID_HT_INFORMATION, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
#endif
#endif

#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
	vendor_spec = cfg80211_find_ie(WLAN_EID_VENDOR_SPECIFIC, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
#endif
#ifdef DOT11_N_SUPPORT /*forced to enable short-GI*/
	ht_cap = (UCHAR *)cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	if(ht_cap != NULL)
	{
		if(pAd->CommonCfg.RegTransmitSetting.field.ShortGI == GI_400)
		{
			*(ht_cap+2)|=((1<<5)|(1<<6));//Set Short_GI bit to be 1
		}
	}
#endif
#else
	supp_rates_ie = cfg80211_parsing_ie(WLAN_EID_SUPP_RATES, pBeacon->beacon_head+36, pBeacon->beacon_head_len-36);
	wpa_ie = cfg80211_parsing_ie(WLAN_EID_WPA, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
	rsn_ie = cfg80211_parsing_ie(WLAN_EID_RSN, pBeacon->beacon_tail, pBeacon->beacon_tail_len);/*wpa2 case.*/
#endif


	/* WMM EDCA Paramter */ 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))	
	CFG80211_SyncPacketWmmIe(pAd, pBeacon->beacon_tail, pBeacon->beacon_tail_len);
#endif

	
	
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP

	if (pAd->p2pForceGOAsSoftAP)
	{
		DBGPRINT(RT_DEBUG_TRACE,("Remove P2P IE enter, lk added\n"));
		
		hex_dump("vendor_spec:", vendor_spec, 83);
		//Remove p2p IE
		if (vendor_spec)
		{			
			DBGPRINT(RT_DEBUG_TRACE,("Remove P2P IE done, lk added\n"));
			PUCHAR pVendorIE = NULL, pPreVendorIE = NULL;
			UINT VendorIELen;
			//Remove p2p capability
			//pDest = strstr(vendor_spec, WIFIDIRECT_OUI);

			pVendorIE = vendor_spec;
			while (pVendorIE < pBeacon->beacon_tail + pBeacon->beacon_tail_len)
			{		
				
				if (RTMPEqualMemory(pVendorIE + 2/*EID + Length*/, P2POUIBYTE, 4))
				{		
					VendorIELen = *(pVendorIE + 1);
					hex_dump("pDest p2p IE 2:", pVendorIE, VendorIELen);
					RTMPZeroMemory(pVendorIE + 2, VendorIELen);				
					pPreVendorIE = pVendorIE;
					pVendorIE += *(pVendorIE + 1) + 2;
					
					//Change IE length
					*(pPreVendorIE + 1) = 9;
					VendorIELen = *(pPreVendorIE + 1);	
					if (pVendorIE < pBeacon->beacon_tail + pBeacon->beacon_tail_len)
					{					
						RTMPMoveMemory(pPreVendorIE, pVendorIE + 2, pBeacon->beacon_tail + pBeacon->beacon_tail_len - pVendorIE);						
					}

					//Reset beacon length	
					pBeacon->beacon_tail_len -= VendorIELen;
					break;
				}
				else 
				{
					pVendorIE += *(pVendorIE + 1) + 2;
				}				
			}
			
			DBGPRINT(RT_DEBUG_TRACE,("Remove P2P IE RTMPZeroMemory done, lk added\n"));

		}
	}
#endif
	if (isAdd)
	{
		//if (pMbss->bWmmCapable)
		//{
        	bWmmCapable = FALSE;
			pMbss->bWmmCapable = FALSE;
		//}
		
		/* Using netDev ptr from VifList if VifDevList Exist */
		if ((pAd->Cfg80211VifDevSet.vifDevList.size > 0) &&
		   ((pNetDev = RTMP_CFG80211_FindVifEntry_ByType(pAd, RT_CMD_80211_IFTYPE_P2P_GO)) != NULL))	
		{
			pMbss->MSSIDDev = pNetDev;
			COPY_MAC_ADDR(pMbss->Bssid, pNetDev->dev_addr);
		}
		else
		{	
			pMbss->MSSIDDev = pAd->net_dev;
			COPY_MAC_ADDR(pMbss->Bssid, pAd->CurrentAddress);
		}
		DBGPRINT(RT_DEBUG_TRACE, ("New AP BSSID %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pMbss->Bssid)));
		/*Security*/
		CFG80211_ParseBeaconIE(pAd,pMbss,wpa_ie,rsn_ie);
		//printk("WAP_IE:%s RSN_IE:%s\n",wpa_ie?"YES":"NULL",rsn_ie?"YES":"NULL");
#if 0
		if ((wpa_ie == NULL) && (rsn_ie == NULL))//open/case
		{
			DBGPRINT(RT_DEBUG_TRACE,("%s:: Open/None case\n", __func__));
			pMbss->AuthMode = Ndis802_11AuthModeOpen;
 			pMbss->WepStatus = Ndis802_11WEPDisabled;
			pMbss->WpaMixPairCipher = MIX_CIPHER_NOTUSE;
		}
		else if ((wpa_ie != NULL) && (rsn_ie == NULL)) //wpapsk/tkipaes case
		{
			DBGPRINT(RT_DEBUG_TRACE,("%s:: WPAPSK/TKIPAES case\n", __func__));
			pMbss->AuthMode = Ndis802_11AuthModeWPAPSK;
 			pMbss->WepStatus = Ndis802_11Encryption4Enabled;
			pMbss->GroupKeyWepStatus = pMbss->WepStatus;
			pMbss->WscSecurityMode = WPAPSKAES;
			pMbss->WpaMixPairCipher = WPA_TKIPAES_WPA2_NONE;
			pMbss->RSNIE_Len[0] = wpa_ie[1];
			NdisMoveMemory(pMbss->RSN_IE[0], wpa_ie+2, wpa_ie[1]);//copy rsn ie
		}
		else if ((wpa_ie == NULL) && (rsn_ie != NULL))
		{
			DBGPRINT(RT_DEBUG_TRACE,("%s:: WPA2PSK/AES case\n", __func__));
			pMbss->AuthMode = Ndis802_11AuthModeWPA2PSK;
 			pMbss->WepStatus = Ndis802_11Encryption3Enabled;
			pMbss->WscSecurityMode = WPA2PSKAES;
			pMbss->GroupKeyWepStatus = pMbss->WepStatus;
			pMbss->WpaMixPairCipher = MIX_CIPHER_NOTUSE;
			pMbss->RSNIE_Len[0] = rsn_ie[1];
			NdisMoveMemory(pMbss->RSN_IE[0], rsn_ie+2, rsn_ie[1]);//copy rsn ie
		}
#endif
		pMbss->CapabilityInfo =
			CAP_GENERATE(1, 0, (pMbss->WepStatus != Ndis802_11EncryptionDisabled), 
				     TxPreamble, pAd->CommonCfg.bUseShortSlotTime, SpectrumMgmt);

		//RTMPMakeRSNIE(pAd, Ndis802_11AuthModeWPA2PSK, Ndis802_11Encryption3Enabled, MIN_NET_DEVICE_FOR_P2P_GO);

		/* Disable Driver-Internal Rekey */
		pMbss->WPAREKEY.ReKeyInterval = 0;
		pMbss->WPAREKEY.ReKeyMethod = DISABLE_REKEY;

#ifdef DOT11_N_SUPPORT
		RTMPSetPhyMode(pAd,  pAd->CommonCfg.PhyMode);
		SetCommonHT(pAd);

		if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED) && (pAd->Antenna.field.TxPath == 2))
		{
			RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1, &BBPR1);
			BBPR1 &= (~0x18);
			BBPR1 |= 0x10;
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R1, BBPR1);
		}
		else
#endif /* DOT11_N_SUPPORT */
		{
			RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1, &BBPR1);
			BBPR1 &= (~0x18);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R1, BBPR1);
		}
	
		/* Receiver Antenna selection, write to BBP R3(bit4:3) */
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBPR3);
		BBPR3 &= (~0x18);
		if(pAd->Antenna.field.RxPath == 3)
		{
			BBPR3 |= (0x10);
		}
		else if(pAd->Antenna.field.RxPath == 2)
		{
			BBPR3 |= (0x8);
		}
		else if(pAd->Antenna.field.RxPath == 1)
		{
			BBPR3 |= (0x0);
		}
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBPR3);

		if(!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
		{
			if ((pAd->CommonCfg.PhyMode > PHY_11G) || bWmmCapable)
			{
				/* EDCA parameters used for AP's own transmission */
				pAd->CommonCfg.APEdcaParm.bValid = TRUE;
				pAd->CommonCfg.APEdcaParm.Aifsn[0] = 3;
				pAd->CommonCfg.APEdcaParm.Aifsn[1] = 7;
				pAd->CommonCfg.APEdcaParm.Aifsn[2] = 1;
				pAd->CommonCfg.APEdcaParm.Aifsn[3] = 1;

				pAd->CommonCfg.APEdcaParm.Cwmin[0] = 4;
				pAd->CommonCfg.APEdcaParm.Cwmin[1] = 4;
				pAd->CommonCfg.APEdcaParm.Cwmin[2] = 3;
				pAd->CommonCfg.APEdcaParm.Cwmin[3] = 2;

				pAd->CommonCfg.APEdcaParm.Cwmax[0] = 6;
				pAd->CommonCfg.APEdcaParm.Cwmax[1] = 10;
				pAd->CommonCfg.APEdcaParm.Cwmax[2] = 4;
				pAd->CommonCfg.APEdcaParm.Cwmax[3] = 3;

				pAd->CommonCfg.APEdcaParm.Txop[0]  = 0;
				pAd->CommonCfg.APEdcaParm.Txop[1]  = 0;
				pAd->CommonCfg.APEdcaParm.Txop[2]  = 94;	/*96; */
				pAd->CommonCfg.APEdcaParm.Txop[3]  = 47;	/*48; */
				AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);

				/* EDCA parameters to be annouced in outgoing BEACON, used by WMM STA */
				pAd->ApCfg.BssEdcaParm.bValid = TRUE;
				pAd->ApCfg.BssEdcaParm.Aifsn[0] = 3;
				pAd->ApCfg.BssEdcaParm.Aifsn[1] = 7;
				pAd->ApCfg.BssEdcaParm.Aifsn[2] = 2;
				pAd->ApCfg.BssEdcaParm.Aifsn[3] = 2;

				pAd->ApCfg.BssEdcaParm.Cwmin[0] = 4;
				pAd->ApCfg.BssEdcaParm.Cwmin[1] = 4;
				pAd->ApCfg.BssEdcaParm.Cwmin[2] = 3;
				pAd->ApCfg.BssEdcaParm.Cwmin[3] = 2;

				pAd->ApCfg.BssEdcaParm.Cwmax[0] = 10;
				pAd->ApCfg.BssEdcaParm.Cwmax[1] = 10;
				pAd->ApCfg.BssEdcaParm.Cwmax[2] = 4;
				pAd->ApCfg.BssEdcaParm.Cwmax[3] = 3;
	
				pAd->ApCfg.BssEdcaParm.Txop[0]  = 0;
				pAd->ApCfg.BssEdcaParm.Txop[1]  = 0;
				pAd->ApCfg.BssEdcaParm.Txop[2]  = 94;	/*96; */
				pAd->ApCfg.BssEdcaParm.Txop[3]  = 47;	/*48; */
			}
			else
			{
				AsicSetEdcaParm(pAd, NULL);
			}
		}

#ifdef DOT11_N_SUPPORT
		if (pAd->CommonCfg.PhyMode < PHY_11ABGN_MIXED)
		{
			/* Patch UI */
			pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth = BW_20;
		}
		
		/* init */
		if (pAd->CommonCfg.bRdg)
		{	
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
			AsicEnableRDG(pAd);
		}
		else	
		{
			RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
			AsicDisableRDG(pAd);
		}			
#endif /* DOT11_N_SUPPORT */

		AsicSetBssid(pAd, pAd->CurrentAddress); 
		AsicSetMcastWC(pAd);
		
		/* In AP mode,  First WCID Table in ASIC will never be used. To prevent it's 0xff-ff-ff-ff-ff-ff, Write 0 here. */
		/* p.s ASIC use all 0xff as termination of WCID table search. */
		RTMP_IO_WRITE32(pAd, MAC_WCID_BASE, 0x00);
		RTMP_IO_WRITE32(pAd, MAC_WCID_BASE + 4, 0x0);

		/* reset WCID table */
		for (idx=2; idx<255; idx++)
		{
			offset = MAC_WCID_BASE + (idx * HW_WCID_ENTRY_SIZE);	
			RTMP_IO_WRITE32(pAd, offset, 0x0);
			RTMP_IO_WRITE32(pAd, offset+4, 0x0);
		}

		pAd->MacTab.Content[0].Addr[0] = 0x01;		
	
		pAd->MacTab.Content[0].HTPhyMode.field.MODE = MODE_OFDM;
		pAd->MacTab.Content[0].HTPhyMode.field.MCS = 3;

		AsicBBPAdjust(pAd);
		//MlmeSetTxPreamble(pAd, (USHORT)pAd->CommonCfg.TxPreamble);	
		MlmeUpdateTxRates(pAd, FALSE, MIN_NET_DEVICE_FOR_P2P_GO);
#ifdef DOT11_N_SUPPORT 
		if (pAd->CommonCfg.PhyMode > PHY_11G)		
		MlmeUpdateHtTxRates(pAd, MIN_NET_DEVICE_FOR_P2P_GO);
#endif /* DOT11_N_SUPPORT */

		/* Disable Protection first. */		
		if (!INFRA_ON(pAd))		
			AsicUpdateProtect(pAd, 0, (ALLN_SETPROTECT|CCKSETPROTECT|OFDMSETPROTECT), TRUE, FALSE);		

		APUpdateCapabilityAndErpIe(pAd);		
#ifdef DOT11_N_SUPPORT	
		APUpdateOperationMode(pAd);
#endif /* DOT11_N_SUPPORT */		
		
#ifdef RTMP_MAC_USB
        RTUSBBssBeaconInit(pAd);
#endif /* RTMP_MAC_USB */
	}
	
	//pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11BGN_MIXED;
	if (pBeacon->interval != 0)
	{
		DBGPRINT(RT_DEBUG_TRACE,("CFG_TIM New BI %d\n", pBeacon->interval));
		pAd->CommonCfg.BeaconPeriod = pBeacon->interval;
	}
	
	if (pBeacon->dtim_period != 0)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG_TIM New DP %d\n", pBeacon->dtim_period));
#ifdef RT_CFG80211_SUPPORT
		pAd->ApCfg.DtimPeriod = 1;
#else
		pAd->ApCfg.DtimPeriod = pBeacon->dtim_period;	
#endif
	}
	
	CFG80211_UpdateBeacon(pAd, pBeacon->beacon_head, pBeacon->beacon_head_len,
			  				   pBeacon->beacon_tail, pBeacon->beacon_tail_len,
							   TRUE);
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0))
	/* Update assoc resp extra ie */
	if (pBeacon->assocresp_ies_len && pBeacon->assocresp_ies)
	{
		CFG80211_UpdateAssocRespExtraIe(pAd, pBeacon->assocresp_ies, pBeacon->assocresp_ies_len);
	}
#endif

	if (isAdd)
	{	
#ifdef RTMP_MAC_USB
#ifdef CONFIG_STA_SUPPORT
			RTMPInitTimer(pAd, &pAd->CommonCfg.BeaconUpdateTimer, GET_TIMER_FUNCTION(BeaconUpdateExec), pAd, TRUE);
#endif /*CONFIG_STA_SUPPORT*/
			RTUSBBssBeaconStart(pAd);
#endif /* RTMP_MAC_USB */


		/* Enable BSS Sync*/
		DBGPRINT(RT_DEBUG_TRACE, ("CFG_TIM Apply BI %d\n", pAd->CommonCfg.BeaconPeriod));
		AsicEnableP2PGoSync(pAd);
		pAd->P2pCfg.bSentProbeRSP = TRUE;

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

#if 0
		{
			BOOLEAN     Cancelled;
			RTMPCancelTimer(&pAd->CommonCfg.BeaconUpdateTimer, & Cancelled);
		}

#ifdef RTMP_MAC_USB
		RTMPInitTimer(pAd, &pAd->CommonCfg.BeaconUpdateTimer, GET_TIMER_FUNCTION(BeaconUpdateExec), pAd, TRUE);
#endif /* RTMP_MAC_USB */
	
		{
			RTMPSetTimer(&pAd->CommonCfg.BeaconUpdateTimer, 10 /*pAd->CommonCfg.BeaconPeriod*/);
		}

#endif
		OPSTATUS_SET_FLAG(pAd, fOP_AP_STATUS_MEDIA_STATE_CONNECTED);
		RTMP_IndicateMediaState(pAd, NdisMediaStateConnected);

	}


#ifdef WFD_SUPPORT
	pAd->StaCfg.WfdCfg.bSuppGoOn = TRUE;
#endif /* WFD_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */	
	return TRUE;
}

BOOLEAN CFG80211DRV_OpsExtraIesSet(
	VOID						*pAdOrg)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	
	CFG80211_CB *pCfg80211_CB = pAd->pCfg80211_CB;
	UINT ie_len = 0;

	if (pCfg80211_CB->pCfg80211_ScanReq)
		ie_len = pCfg80211_CB->pCfg80211_ScanReq->ie_len;

    	CFG80211DBG(RT_DEBUG_INFO, ("80211> CFG80211DRV_OpsExtraIesSet ==> %d\n", ie_len)); 
#ifdef CONFIG_STA_SUPPORT
	CFG80211DBG(RT_DEBUG_INFO, ("80211> is_wpa_supplicant_up ==> %d\n", 
									pAd->StaCfg.WpaSupplicantUP)); 
#endif /*CONFIG_STA_SUPPORT*/	
	
	if (ie_len == 0)
		return FALSE;
	
	if (pAd->StaCfg.pWpsProbeReqIe)
	{	
		os_free_mem(NULL, pAd->StaCfg.pWpsProbeReqIe);
		pAd->StaCfg.pWpsProbeReqIe = NULL;
	}

	pAd->StaCfg.WpsProbeReqIeLen = 0;

	CFG80211DBG(RT_DEBUG_INFO, ("80211> is_wpa_supplicant_up ==> %d\n", pAd->StaCfg.WpaSupplicantUP)); 
	os_alloc_mem(pAd, (UCHAR **)&(pAd->StaCfg.pWpsProbeReqIe), ie_len);
	if (pAd->StaCfg.pWpsProbeReqIe)
	{
		memcpy(pAd->StaCfg.pWpsProbeReqIe, pCfg80211_CB->pCfg80211_ScanReq->ie, ie_len);
		pAd->StaCfg.WpsProbeReqIeLen = ie_len;
		//hex_dump("WpsProbeReqIe", pAd->StaCfg.pWpsProbeReqIe, pAd->StaCfg.WpsProbeReqIeLen);
		DBGPRINT(RT_DEBUG_INFO, ("Set::RT_OID_WPS_PROBE_REQ_IE, WpsProbeReqIeLen = %d!!\n",
					pAd->StaCfg.WpsProbeReqIeLen));
	}
	else
	{
		CFG80211DBG(RT_DEBUG_ERROR, ("80211> CFG80211DRV_OpsExtraIesSet ==> allocate fail. \n")); 
		return FALSE;
	}
	return TRUE;
}


BOOLEAN CFG80211DRV_OpsJoinIbss(
	VOID						*pAdOrg,
	VOID						*pData)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_IBSS *pIbssInfo;


	pIbssInfo = (CMD_RTPRIV_IOCTL_80211_IBSS *)pData;
	pAd->StaCfg.bAutoReconnect = TRUE;

	pAd->CommonCfg.BeaconPeriod = pIbssInfo->BeaconInterval;
	Set_SSID_Proc(pAd, (PSTRING)pIbssInfo->pSsid);
#endif /* CONFIG_STA_SUPPORT */
	return TRUE;
}


BOOLEAN CFG80211DRV_OpsLeave(
	VOID						*pAdOrg,
	UINT8						IfType)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;

    MLME_DEAUTH_REQ_STRUCT   DeAuthReq;
    MLME_QUEUE_ELEM *pMsgElem = NULL;
	

	pAd->StaCfg.bAutoReconnect = FALSE;
	pAd->FlgCfg80211Connecting = FALSE;

        pAd->MlmeAux.AutoReconnectSsidLen= 32;
        NdisZeroMemory(pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen);

    os_alloc_mem(pAd, (UCHAR **)&pMsgElem, sizeof(MLME_QUEUE_ELEM));
	
	if (IfType == RT_CMD_80211_IFTYPE_P2P_CLIENT)
		COPY_MAC_ADDR(DeAuthReq.Addr, pAd->ApCliMlmeAux.Bssid);
	else
		COPY_MAC_ADDR(DeAuthReq.Addr, pAd->CommonCfg.Bssid);
		
    DeAuthReq.Reason = REASON_DEAUTH_STA_LEAVING;
    pMsgElem->MsgLen = sizeof(MLME_DEAUTH_REQ_STRUCT);
    NdisMoveMemory(pMsgElem->Msg, &DeAuthReq, sizeof(MLME_DEAUTH_REQ_STRUCT));
    MlmeDeauthReqAction(pAd, pMsgElem);
    os_free_mem(NULL, pMsgElem);
	 
	if (IfType == RT_CMD_80211_IFTYPE_P2P_CLIENT)
		ApCliLinkDown(pAd, 0);
	else
		LinkDown(pAd, FALSE);
#endif /* CONFIG_STA_SUPPORT */
	return TRUE;
}


BOOLEAN CFG80211DRV_StaGet(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_STA *pIbssInfo;


	pIbssInfo = (CMD_RTPRIV_IOCTL_80211_STA *)pData;

#ifdef CONFIG_AP_SUPPORT
{
	MAC_TABLE_ENTRY *pEntry;
	ULONG DataRate = 0;
	UINT32 RSSI;


	pEntry = MacTableLookup(pAd, pIbssInfo->MAC);
	if (pEntry == NULL)
		return FALSE;
	/* End of if */

	/* fill tx rate */
	getRate(pEntry->HTPhyMode, &DataRate);

	if ((pEntry->HTPhyMode.field.MODE == MODE_HTMIX) ||
		(pEntry->HTPhyMode.field.MODE == MODE_HTGREENFIELD))
	{
		if (pEntry->HTPhyMode.field.BW)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_BW_40;
		/* End of if */
		if (pEntry->HTPhyMode.field.ShortGI)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_SHORT_GI;
		/* End of if */

		pIbssInfo->TxRateMCS = pEntry->HTPhyMode.field.MCS;
	}
	else
	{
		pIbssInfo->TxRateFlags = RT_CMD_80211_TXRATE_LEGACY;
		pIbssInfo->TxRateMCS = DataRate*1000; /* unit: 100kbps */
	} /* End of if */

	/* fill signal */
	RSSI = RTMPAvgRssi(pAd, &pEntry->RssiSample);
	pIbssInfo->Signal = RSSI;

	/* fill tx count */
	pIbssInfo->TxPacketCnt = pEntry->OneSecTxNoRetryOkCount + 
						pEntry->OneSecTxRetryOkCount + 
						pEntry->OneSecTxFailCount;

	/* fill inactive time */
	pIbssInfo->InactiveTime = pEntry->NoDataIdleCount * 1000; /* unit: ms */
	pIbssInfo->InactiveTime *= MLME_TASK_EXEC_MULTIPLE;
	pIbssInfo->InactiveTime /= 20;
}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
{
	HTTRANSMIT_SETTING PhyInfo;
	ULONG DataRate = 0;
	UINT32 RSSI;


	/* fill tx rate */
    if ((!WMODE_CAP_N(pAd->CommonCfg.PhyMode)) ||
	 (pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.MODE <= MODE_OFDM))
	{
		PhyInfo.word = pAd->StaCfg.HTPhyMode.word;
	}
    else
		PhyInfo.word = pAd->MacTab.Content[BSSID_WCID].HTPhyMode.word;
	/* End of if */

	getRate(PhyInfo, &DataRate);

	if ((PhyInfo.field.MODE == MODE_HTMIX) ||
		(PhyInfo.field.MODE == MODE_HTGREENFIELD))
	{
		if (PhyInfo.field.BW)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_BW_40;
		/* End of if */
		if (PhyInfo.field.ShortGI)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_SHORT_GI;
		/* End of if */

		pIbssInfo->TxRateMCS = PhyInfo.field.MCS;
	}
	else
	{
		pIbssInfo->TxRateFlags = RT_CMD_80211_TXRATE_LEGACY;
		pIbssInfo->TxRateMCS = DataRate*10; /* unit: 100kbps */
	} /* End of if */

    /* fill tx/rx packet count */
    pIbssInfo->tx_packets = pAd->WlanCounters.TransmittedFragmentCount.u.LowPart;
    pIbssInfo->tx_retries = pAd->WlanCounters.RetryCount.u.LowPart;
    pIbssInfo->tx_failed = pAd->WlanCounters.FailedCount.u.LowPart;
    pIbssInfo->rx_packets = pAd->WlanCounters.ReceivedFragmentCount.QuadPart;

	/* fill signal */
	RSSI = RTMPAvgRssi(pAd, &pAd->StaCfg.RssiSample);
	pIbssInfo->Signal = RSSI;
}
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}

BOOLEAN CFG80211DRV_ApKeyDel(
        VOID                                            *pAdOrg,
        VOID                                            *pData)
{
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
	MAC_TABLE_ENTRY *pEntry;
	
	pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        if (pKeyInfo->bPairwise == FALSE )
#else
        if (pKeyInfo->KeyId > 0)
#endif
	{
		DBGPRINT(RT_DEBUG_TRACE,("CFG: AsicRemoveSharedKeyEntry %d\n", pKeyInfo->KeyId));	
		AsicRemoveSharedKeyEntry(pAd, MAIN_MBSSID, pKeyInfo->KeyId);
	}
	else
	{
		pEntry = MacTableLookup(pAd, pKeyInfo->MAC);
		
		if (pEntry && (pEntry->Aid != 0))
		{
			NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));
			AsicRemovePairwiseKeyEntry(pAd, (UCHAR)pEntry->Aid);
		}
	}
	
	return TRUE;

}

BOOLEAN CFG80211DRV_ApKeyAdd(
        VOID                                            *pAdOrg,
        VOID                                            *pData)
{
#ifdef CONFIG_AP_SUPPORT
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
		MAC_TABLE_ENTRY *pEntry =NULL;
		UINT8 Wcid;
#ifdef RT_CFG80211_SUPPORT
	UINT apidx = MAIN_MBSSID;
#endif /*RT_CFG80211_SUPPORT*/

        DBGPRINT(RT_DEBUG_TRACE,("CFG: CFG80211DRV_ApKeyAdd\n"));
        pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;

	if (pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP40 || pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP104)
	{
		pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus = Ndis802_11WEPEnabled;
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

			AsicAddSharedKeyEntry(pAd, apidx, pKeyInfo->KeyId, pSharedKey);
		}		
	}
	else if(pKeyInfo->KeyType == RT_CMD_80211_KEY_WPA)
	{
		if (pKeyInfo->cipher == Ndis802_11AESEnable)
			{
		/* AES */
//#ifndef CONFIG_P2P_AUTO_GO_AS_SOFTAP
		//pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus = Ndis802_11Encryption3Enabled;
		//pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus = Ndis802_11Encryption3Enabled;
//#endif		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        if (pKeyInfo->bPairwise == FALSE )
#else
        if (pKeyInfo->KeyId > 0)
#endif		
		{
			//PMULTISSID_STRUCT pMbss = &pAd->ApCfg.MBSSID[MAIN_MBSSID];
			//pMbss->DefaultKeyId = pKeyInfo->KeyId;

			if (pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus == Ndis802_11Encryption3Enabled
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
			|| pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus == Ndis802_11Encryption4Enabled
#endif
			)
			{
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
				if (pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus == Ndis802_11Encryption4Enabled)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set TKIP Security Set. (GROUP) %d\n", pKeyInfo->KeyLen));
				}
				else {
					DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (GROUP) %d\n", pKeyInfo->KeyLen));
				}
#else
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (GROUP) %d\n", pKeyInfo->KeyLen));
#endif
				
				pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].KeyLen= LEN_TK;
				NdisMoveMemory(pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].Key, pKeyInfo->KeyBuf, pKeyInfo->KeyLen);
				//NdisMoveMemory(pAd->SharedKey[MAIN_MBSSID][pMbss->DefaultKeyId].RxMic, (Key.ik_keydata+16+8), 8);
				//NdisMoveMemory(pAd->SharedKey[MAIN_MBSSID][pMbss->DefaultKeyId].TxMic, (Key.ik_keydata+16), 8);				
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
				if (pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus == Ndis802_11Encryption4Enabled)
					pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].CipherAlg = CIPHER_TKIP;
				else if (pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
#endif
				pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].CipherAlg = CIPHER_AES;

				AsicAddSharedKeyEntry(pAd, MAIN_MBSSID, pKeyInfo->KeyId, 
						&pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId]);

				GET_GroupKey_WCID(pAd, Wcid, MAIN_MBSSID);
				RTMPSetWcidSecurityInfo(pAd, MAIN_MBSSID, (UINT8)(pKeyInfo->KeyId), 
						pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].CipherAlg, Wcid, SHAREDKEYTABLE);
				
			}
		}
		else
		{
			if (pKeyInfo->MAC)
				pEntry = MacTableLookup(pAd, pKeyInfo->MAC);
			if(pEntry)
			{
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
				if (pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus == Ndis802_11Encryption4Enabled)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set TKIP Security Set. (PAIRWISE) %d\n", pKeyInfo->KeyLen));
				}
				else{
					DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (PAIRWISE) %d\n", pKeyInfo->KeyLen));
				}	
				#else
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (PAIRWISE) %d\n", pKeyInfo->KeyLen));
#endif
				pEntry->PairwiseKey.KeyLen = LEN_TK;
				NdisCopyMemory(&pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyBuf, OFFSET_OF_PTK_TK);
				NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyLen);
#ifdef CONFIG_P2P_AUTO_GO_AS_SOFTAP
				if (pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus == Ndis802_11Encryption4Enabled)
					pEntry->PairwiseKey.CipherAlg = CIPHER_TKIP;
				else if (pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus == Ndis802_11Encryption3Enabled)
#endif	
				pEntry->PairwiseKey.CipherAlg = CIPHER_AES;
				
				AsicAddPairwiseKeyEntry(pAd, (UCHAR)pEntry->Aid, &pEntry->PairwiseKey);
				RTMPSetWcidSecurityInfo(pAd, pEntry->apidx, (UINT8)(pKeyInfo->KeyId & 0x0fff),
					pEntry->PairwiseKey.CipherAlg, pEntry->Aid, PAIRWISEKEYTABLE);
			}
			else	
			{
				DBGPRINT(RT_DEBUG_ERROR,("CFG: Set AES Security Set. (PAIRWISE) But pEntry NULL\n"));
			}
		
			}
			
		}else if (pKeyInfo->cipher == Ndis802_11TKIPEnable) {
		/* TKIP */
				//pWdev->WepStatus = Ndis802_11Encryption2Enabled;
				//pWdev->GroupKeyWepStatus = Ndis802_11Encryption2Enabled;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
		        if (pKeyInfo->bPairwise == FALSE )
#else
		        if (pKeyInfo->KeyId > 0)
#endif	/* LINUX_VERSION_CODE 2.6.37 */	
				{
					if (pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus == Ndis802_11Encryption2Enabled)
					{
						DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set TKIP Security Set. (GROUP) %d\n", pKeyInfo->KeyLen));
						pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].KeyLen= LEN_TK;
						NdisMoveMemory(pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].Key, pKeyInfo->KeyBuf, pKeyInfo->KeyLen);
						
						pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].CipherAlg = CIPHER_TKIP;

						AsicAddSharedKeyEntry(pAd, MAIN_MBSSID, pKeyInfo->KeyId, 
								&pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId]);

						GET_GroupKey_WCID(pAd, Wcid, MAIN_MBSSID);
						RTMPSetWcidSecurityInfo(pAd, MAIN_MBSSID, (UINT8)(pKeyInfo->KeyId), 
								pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].CipherAlg, Wcid, SHAREDKEYTABLE);
						
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
						RTMPSetWcidSecurityInfo(pAd, pEntry->apidx, (UINT8)(pKeyInfo->KeyId & 0x0fff),
							pEntry->PairwiseKey.CipherAlg, pEntry->Aid, PAIRWISEKEYTABLE);
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


VOID CFG80211DRV_P2pClientKeyAdd(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
	
    DBGPRINT(RT_DEBUG_TRACE, ("CFG Debug: CFG80211DRV_P2pClientKeyAdd\n"));
	if(pAd == NULL)
	{
		ASSERT(0);
		return;
	}
    pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;
	
	if (pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP40 || pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP104)
		;
	else
	{
		INT 	BssIdx;
		PAPCLI_STRUCT pApCliEntry;
		MAC_TABLE_ENTRY	*pMacEntry=(MAC_TABLE_ENTRY *)NULL;
	
		BssIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + MAIN_MBSSID;
		pApCliEntry = &pAd->ApCfg.ApCliTab[MAIN_MBSSID];
		pMacEntry = &pAd->MacTab.Content[pApCliEntry->MacTabWCID]; 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        if (pKeyInfo->bPairwise == FALSE )
#else
        if (pKeyInfo->KeyId > 0)
#endif		
		{
			//pApCliEntry->DefaultKeyId = pKeyInfo->KeyId;
			
			if (pApCliEntry->WepStatus == Ndis802_11Encryption3Enabled)
			{
				printk("APCLI: Set AES Security Set. [%d] (GROUP) %d\n", BssIdx, pKeyInfo->KeyLen);
				if (!memcmp(pApCliEntry->SharedKey[pKeyInfo->KeyId].Key,
					pKeyInfo->KeyBuf, LEN_TK)) {
					DBGPRINT(RT_DEBUG_OFF, ("%s skip reinstall gtk\n",
						 __func__));
					goto skip_reinstall_gtk;
				}
				pMacEntry->rx_ccmp_pn_bmc[pKeyInfo->KeyId] = 0;
				pMacEntry->rx_ccmp_pn_bmc_zero[pKeyInfo->KeyId] = TRUE;
				NdisZeroMemory(&pApCliEntry->SharedKey[pKeyInfo->KeyId], sizeof(CIPHER_KEY));  
				pApCliEntry->SharedKey[pKeyInfo->KeyId].KeyLen = LEN_TK;
				NdisMoveMemory(pApCliEntry->SharedKey[pKeyInfo->KeyId].Key, pKeyInfo->KeyBuf, pKeyInfo->KeyLen);
				
				pApCliEntry->SharedKey[pKeyInfo->KeyId].CipherAlg = CIPHER_AES;

				AsicAddSharedKeyEntry(pAd, BssIdx, pKeyInfo->KeyId, 
									&pApCliEntry->SharedKey[pKeyInfo->KeyId]);
						
				RTMPAddWcidAttributeEntry(pAd, 
										  BssIdx, 
										  pKeyInfo->KeyId, 
										  pApCliEntry->SharedKey[pKeyInfo->KeyId].CipherAlg, 
										  NULL);				
skip_reinstall_gtk:
				if (pMacEntry->AuthMode >= Ndis802_11AuthModeWPA)
				{
					/* set 802.1x port control */
					pMacEntry->PortSecured = WPA_802_1X_PORT_SECURED;
					pMacEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
				}						  
			}
		}	
		else
		{	
			if(pMacEntry)
			{
				printk("APCLI: Set AES Security Set. [%d] (PAIRWISE) %d\n", BssIdx, pKeyInfo->KeyLen);
				if (!memcmp(pMacEntry->PairwiseKey.Key,
					pKeyInfo->KeyBuf, LEN_TK)) {
					DBGPRINT(RT_DEBUG_OFF, ("%s skip reinstall ptk\n",
						 __func__));
					goto skip_reinstall_ptk;
				}
				pMacEntry->rx_ccmp_pn_uc = 0;
				NdisZeroMemory(&pMacEntry->PairwiseKey, sizeof(CIPHER_KEY));  
				pMacEntry->PairwiseKey.KeyLen = LEN_TK;
				
				NdisCopyMemory(&pMacEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyBuf, OFFSET_OF_PTK_TK);
				NdisMoveMemory(pMacEntry->PairwiseKey.Key, &pMacEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyLen);
				
				pMacEntry->PairwiseKey.CipherAlg = CIPHER_AES;
				
				AsicAddPairwiseKeyEntry(pAd, (UCHAR)pMacEntry->Aid, &pMacEntry->PairwiseKey);
				RTMPSetWcidSecurityInfo(pAd, BssIdx, 0, pMacEntry->PairwiseKey.CipherAlg, pMacEntry->Aid, PAIRWISEKEYTABLE);
skip_reinstall_ptk:
				;
			}
			else	
			{
				printk("APCLI: Set AES Security Set. (PAIRWISE) But pMacEntry NULL\n");
			}			
		}		
	}
}
	
BOOLEAN CFG80211DRV_StaKeyAdd(
	VOID						*pAdOrg,
	VOID						*pData)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
	RT_CMD_STA_IOCTL_SECURITY IoctlSec;

	IoctlSec.Alg = 0;
	pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;

	if (pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP40 || pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP104)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("RT_CMD_80211_KEY_WEP\n"));
#if 0
		switch(pKeyInfo->KeyId)
		{
			case 1:
			default:
				Set_Key1_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;

			case 2:
				Set_Key2_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;

			case 3:
				Set_Key3_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;

			case 4:
				Set_Key4_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;
		} /* End of switch */
#endif
	} 
	else 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set_WPAPSK_Proc ==> %d, %d, %zu...\n", pKeyInfo->KeyId, pKeyInfo->KeyType, strlen(pKeyInfo->KeyBuf)));
		
		IoctlSec.KeyIdx = pKeyInfo->KeyId;
		IoctlSec.pData = pKeyInfo->KeyBuf;
		IoctlSec.length = pKeyInfo->KeyLen;
		
		/* YF@20120327: Due to WepStatus will be set in the cfg connect function.*/
		if (pAd->StaCfg.WepStatus == Ndis802_11Encryption2Enabled)
			IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP;
		else if (pAd->StaCfg.WepStatus == Ndis802_11Encryption3Enabled)
			IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP;
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
		if (pKeyInfo->bPairwise == FALSE )
#else
		if (pKeyInfo->KeyId > 0)
#endif	
		{
			if (pAd->StaCfg.GroupCipher == Ndis802_11Encryption2Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP;
			else if (pAd->StaCfg.GroupCipher == Ndis802_11Encryption3Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP;
				
			DBGPRINT(RT_DEBUG_TRACE, ("Install GTK: %d\n", IoctlSec.Alg));
			IoctlSec.ext_flags = RT_CMD_STA_IOCTL_SECURTIY_EXT_GROUP_KEY;
		}	
		else
		{
			if (pAd->StaCfg.PairCipher == Ndis802_11Encryption2Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP;
			else if (pAd->StaCfg.PairCipher == Ndis802_11Encryption3Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP;
				
			DBGPRINT(RT_DEBUG_TRACE, ("Install PTK: %d\n", IoctlSec.Alg));
			IoctlSec.ext_flags = RT_CMD_STA_IOCTL_SECURTIY_EXT_SET_TX_KEY;
		}
		
		Set_GroupKey_Proc(pAd, (RTMP_IOCTL_INPUT_STRUCT *)(&IoctlSec));	
	}


#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}

VOID CFG80211DRV_SetP2pCliAssocIe(
	VOID						*pAdOrg,
	VOID						*pData,
	UINT                         ie_len)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	hex_dump("P2PCLI=", pData, ie_len);
	
	if (ie_len > 0)	
	{
		if (pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe)
		{
			os_free_mem(NULL, pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe);
			pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe = NULL;
		}

		os_alloc_mem(NULL, (UCHAR **)&pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe, ie_len);
		if (pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe)
		{
			pAd->ApCfg.ApCliTab[MAIN_MBSSID].WpaAssocIeLen = ie_len;
			NdisMoveMemory(pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe, pData, pAd->ApCfg.ApCliTab[MAIN_MBSSID].WpaAssocIeLen);
		}
		else
			pAd->ApCfg.ApCliTab[MAIN_MBSSID].WpaAssocIeLen=0;
	}
	else
	{
		if (pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe)
		{
			os_free_mem(NULL, pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe);
			pAd->ApCfg.ApCliTab[MAIN_MBSSID].pWpaAssocIe = NULL;
		}
		pAd->ApCfg.ApCliTab[MAIN_MBSSID].WpaAssocIeLen=0;	
	}
}

/* For P2P_CLIENT Connection Setting in AP_CLI SM */
BOOLEAN CFG80211DRV_P2pClientConnect(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_CONNECT *pConnInfo;
	UCHAR Connect_SSID[NDIS_802_11_LENGTH_SSID];
	UINT32 Connect_SSIDLen;
	
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	pObj->ioctl_if_type = INT_APCLI;
	
	pConnInfo = (CMD_RTPRIV_IOCTL_80211_CONNECT *)pData;
	DBGPRINT(RT_DEBUG_TRACE, ("APCLI Connection onGoing.....\n"));

	Connect_SSIDLen = pConnInfo->SsidLen;
	if (Connect_SSIDLen > NDIS_802_11_LENGTH_SSID)
		Connect_SSIDLen = NDIS_802_11_LENGTH_SSID;
	
	memset(&Connect_SSID, 0, sizeof(Connect_SSID));
	memcpy(Connect_SSID, pConnInfo->pSsid, Connect_SSIDLen);

	pAd->ApCfg.ApCliTab[MAIN_MBSSID].WpaSupplicantUP = WPA_SUPPLICANT_ENABLE; 
	/* Check the connection is WPS or not */
	if (pConnInfo->bWpsConnection) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("AP_CLI WPS Connection onGoing.....\n"));
		pAd->ApCfg.ApCliTab[MAIN_MBSSID].WpaSupplicantUP |= WPA_SUPPLICANT_ENABLE_WPS;
	}		

	/* Set authentication mode */
	if (pConnInfo->WpaVer == 2)
	{
		if (!(pConnInfo->FlgIs8021x == TRUE)) 
		{
			DBGPRINT(RT_DEBUG_TRACE,("APCLI WPA2PSK\n"));
			Set_ApCli_AuthMode_Proc(pAd, "WPA2PSK");
		}
	}
	else if (pConnInfo->WpaVer == 1)
	{
		if (!pConnInfo->FlgIs8021x) 
		{
			DBGPRINT(RT_DEBUG_TRACE,("APCLI WPAPSK\n"));
			Set_ApCli_AuthMode_Proc(pAd, "WPAPSK");
		}
	}
	else
		Set_ApCli_AuthMode_Proc(pAd, "OPEN");	

	/* Set PTK Encryption Mode */
	if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_CCMP) {
		DBGPRINT(RT_DEBUG_TRACE,("AES\n"));
		Set_ApCli_EncrypType_Proc(pAd, "AES");
	}
	else if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_TKIP) {
		DBGPRINT(RT_DEBUG_TRACE,("TKIP\n"));
		Set_ApCli_EncrypType_Proc(pAd, "TKIP");
	}
	else if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_WEP)
	{
		DBGPRINT(RT_DEBUG_TRACE,("WEP\n"));
		Set_ApCli_EncrypType_Proc(pAd, "WEP");
	}
	
	/* Groupwise Key Information Setting */
	
	if (pConnInfo->pBssid != NULL)
	{
		NdisZeroMemory(pAd->ApCfg.ApCliTab[0].CfgApCliBssid, MAC_ADDR_LEN);
		NdisCopyMemory(pAd->ApCfg.ApCliTab[0].CfgApCliBssid, pConnInfo->pBssid, MAC_ADDR_LEN);
		NdisCopyMemory(pAd->P2pCfg.Bssid, pConnInfo->pBssid, MAC_ADDR_LEN);
	}
	
	OPSTATUS_SET_FLAG(pAd, fOP_AP_STATUS_MEDIA_STATE_CONNECTED);

	pAd->FlgCfg80211Connecting = TRUE;
	Set_ApCli_Ssid_Proc(pAd, (PSTRING)Connect_SSID);
	Set_ApCli_Enable_Proc(pAd, "1");
	CFG80211DBG(RT_DEBUG_ERROR, ("80211> APCLI CONNECTING SSID = %s\n", Connect_SSID));

	return TRUE;	
}

extern INT RtmpIoctl_rt_ioctl_siwauth(
	IN      RTMP_ADAPTER                    *pAd,
	IN      VOID                            *pData,
	IN      ULONG                            Data);

BOOLEAN CFG80211DRV_Connect(
	VOID						*pAdOrg,
	VOID						*pData)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_CONNECT *pConnInfo;
	UCHAR SSID[NDIS_802_11_LENGTH_SSID+1];
	UINT32 SSIDLen;
	RT_CMD_STA_IOCTL_SECURITY_ADV IoctlWpa;

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_INFRA_ON) && 
            OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211: Connected, disconnect first !\n"));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211: No Connection\n"));
	}

	pConnInfo = (CMD_RTPRIV_IOCTL_80211_CONNECT *)pData;

	/* change to infrastructure mode if we are in ADHOC mode */
	Set_NetworkType_Proc(pAd, "Infra");

	SSIDLen = pConnInfo->SsidLen;
	if (SSIDLen > NDIS_802_11_LENGTH_SSID+1)
	{
		SSIDLen = NDIS_802_11_LENGTH_SSID+1;
	}
	
	memset(&SSID, 0, sizeof(SSID));
	memcpy(SSID, pConnInfo->pSsid, SSIDLen);

	if (pConnInfo->bWpsConnection) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS Connection onGoing.....\n"));
		/* YF@20120327: Trigger Driver to Enable WPS function. */	
		pAd->StaCfg.WpaSupplicantUP |= WPA_SUPPLICANT_ENABLE_WPS;  /* Set_Wpa_Support(pAd, "3") */
		Set_AuthMode_Proc(pAd, "OPEN");
		Set_EncrypType_Proc(pAd, "NONE");
		Set_SSID_Proc(pAd, (PSTRING)SSID);
		
		return TRUE;
	}
	else
	{
		pAd->StaCfg.WpaSupplicantUP = WPA_SUPPLICANT_ENABLE; /* Set_Wpa_Support(pAd, "1")*/
	}	
	
	/* set authentication mode */
	if (pConnInfo->WpaVer == 2)
	{
		if (pConnInfo->FlgIs8021x == TRUE) {
			DBGPRINT(RT_DEBUG_TRACE, ("WPA2\n"));
			Set_AuthMode_Proc(pAd, "WPA2");
		}
		else 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("WPA2PSK\n"));
			Set_AuthMode_Proc(pAd, "WPA2PSK");
		}
		/* End of if */
	}
	else if (pConnInfo->WpaVer == 1)
	{
		if (pConnInfo->FlgIs8021x == TRUE) {
			DBGPRINT(RT_DEBUG_TRACE, ("WPA\n"));
			Set_AuthMode_Proc(pAd, "WPA");
		}
		else 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("WPAPSK\n"));
			Set_AuthMode_Proc(pAd, "WPAPSK");
		}
		/* End of if */
	}
	else if (pConnInfo->AuthType == Ndis802_11AuthModeAutoSwitch)
		Set_AuthMode_Proc(pAd, "WEPAUTO");
    else if (pConnInfo->AuthType == Ndis802_11AuthModeShared)
		Set_AuthMode_Proc(pAd, "SHARED");
	else
		Set_AuthMode_Proc(pAd, "OPEN");
	/* End of if */

	CFG80211DBG(RT_DEBUG_TRACE,
				("80211> AuthMode = %d\n", pAd->StaCfg.AuthMode));

	/* set encryption mode */
	if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_CCMP) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("AES\n"));
		Set_EncrypType_Proc(pAd, "AES");
	}
	else if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_TKIP) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("TKIP\n"));
		Set_EncrypType_Proc(pAd, "TKIP");
	}
	else if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_WEP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WEP\n"));
		Set_EncrypType_Proc(pAd, "WEP");
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("NONE\n"));
		Set_EncrypType_Proc(pAd, "NONE");		
	}
	
	/* Groupwise Key Information Setting */
	IoctlWpa.flags = RT_CMD_STA_IOCTL_WPA_GROUP;    
	if (pConnInfo->GroupwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_CCMP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("GTK AES\n"));
		IoctlWpa.value = RT_CMD_STA_IOCTL_WPA_GROUP_CCMP;
		RtmpIoctl_rt_ioctl_siwauth(pAd, &IoctlWpa, 0);
	}
	else if (pConnInfo->GroupwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_TKIP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("GTK TKIP\n"));
		IoctlWpa.value = RT_CMD_STA_IOCTL_WPA_GROUP_TKIP;
		RtmpIoctl_rt_ioctl_siwauth(pAd, &IoctlWpa, 0);
	} 

	CFG80211DBG(RT_DEBUG_TRACE,
				("80211> EncrypType = %d\n", pAd->StaCfg.WepStatus));

	CFG80211DBG(RT_DEBUG_TRACE,
				("80211> Key = %p\n", pConnInfo->pKey));

	/* set channel: STATION will auto-scan */

	/* set WEP key */
	if (pConnInfo->pKey &&
		((pConnInfo->GroupwiseEncrypType | pConnInfo->PairwiseEncrypType) &
												RT_CMD_80211_CONN_ENCRYPT_WEP))
	{
		UCHAR KeyBuf[50];

		/* reset AuthMode and EncrypType */
		Set_EncrypType_Proc(pAd, "WEP");

		/* reset key */
#ifdef RT_CFG80211_DEBUG
		hex_dump("KeyBuf=", (UINT8 *)pConnInfo->pKey, pConnInfo->KeyLen);
#endif /* RT_CFG80211_DEBUG */

		pAd->StaCfg.DefaultKeyId = pConnInfo->KeyIdx; /* base 0 */
		if (pConnInfo->KeyLen >= sizeof(KeyBuf))
			return FALSE;
		/* End of if */
		memcpy(KeyBuf, pConnInfo->pKey, pConnInfo->KeyLen);
		KeyBuf[pConnInfo->KeyLen] = 0x00;

		CFG80211DBG(RT_DEBUG_ERROR,
					("80211> pAd->StaCfg.DefaultKeyId = %d\n",
					pAd->StaCfg.DefaultKeyId));

		Set_Wep_Key_Proc(pAd, (PSTRING)KeyBuf, (INT)pConnInfo->KeyLen, (INT)pConnInfo->KeyIdx);

	} /* End of if */

	/* TODO: We need to provide a command to set BSSID to associate a AP */

	/* re-set SSID */
	pAd->FlgCfg80211Connecting = TRUE;

	Set_SSID_Proc(pAd, (PSTRING)SSID);
	CFG80211DBG(RT_DEBUG_TRACE, ("80211> Connecting SSID = %s\n", SSID));
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}


VOID CFG80211DRV_RegNotify(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_REG_NOTIFY *pRegInfo;


	pRegInfo = (CMD_RTPRIV_IOCTL_80211_REG_NOTIFY *)pData;

	/* keep Alpha2 and we can re-call the function when interface is up */
	pAd->Cfg80211_Alpha2[0] = pRegInfo->Alpha2[0];
	pAd->Cfg80211_Alpha2[1] = pRegInfo->Alpha2[1];

	/* apply the new regulatory rule */
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
	{
		/* interface is up */
		CFG80211_RegRuleApply(pAd, pRegInfo->pWiphy, (UCHAR *)pRegInfo->Alpha2);
	}
	else
	{
		CFG80211DBG(RT_DEBUG_ERROR, ("crda> interface is down!\n"));
	} /* End of if */
}


VOID CFG80211DRV_SurveyGet(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_SURVEY *pSurveyInfo;


	pSurveyInfo = (CMD_RTPRIV_IOCTL_80211_SURVEY *)pData;

	pSurveyInfo->pCfg80211 = pAd->pCfg80211_CB;

#ifdef AP_QLOAD_SUPPORT
	pSurveyInfo->ChannelTimeBusy = pAd->QloadLatestChannelBusyTimePri;
	pSurveyInfo->ChannelTimeExtBusy = pAd->QloadLatestChannelBusyTimeSec;
#endif /* AP_QLOAD_SUPPORT */
}


VOID CFG80211_UnRegister(
	IN VOID						*pAdOrg,
	IN VOID						*pNetDev)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;


	/* sanity check */
	if (pAd->pCfg80211_CB == NULL)
		return;
	

	CFG80211OS_UnRegister(pAd->pCfg80211_CB, pNetDev);

#if 1 //patch : cfg80211_scan_done() crash issue!
	RTMP_DRIVER_80211_SCAN_STATUS_LOCK_INIT(pAd, FALSE);
#endif

	/* Reset CFG80211 Global Setting Here */
	DBGPRINT(RT_DEBUG_TRACE, ("==========> TYPE Reset CFG80211 Global Setting Here <==========\n"));
	pAd->Cfg80211RegisterActionFrame = FALSE, 
	pAd->Cfg80211ActionCount = 0;
	
	pAd->Cfg80211RegisterProbeReqFrame = FALSE;
	pAd->Cfg80211ProbeReqCount = 0;	
	
	pAd->pCfg80211_CB = NULL;
	pAd->CommonCfg.HT_Disable = 0;

#if 1
        /* It should be free when ScanEnd, 
           But Hit close the device in Scanning */
        if (pAd->pCfg80211ChanList != NULL)
        {
                os_free_mem(NULL, pAd->pCfg80211ChanList);
                pAd->pCfg80211ChanList = NULL;
                pAd->Cfg80211ChanListLan = 0;
        }

        if (pAd->pTxStatusBuf != NULL)
        {
                os_free_mem(NULL, pAd->pTxStatusBuf);
                pAd->pTxStatusBuf = NULL;
        }

        if (pAd->beacon_tail_buf != NULL)
        {
                os_free_mem(NULL, pAd->beacon_tail_buf);
                pAd->beacon_tail_buf = NULL;
        }
#endif

}


/*
========================================================================
Routine Description:
	Parse and handle country region in beacon from associated AP.

Arguments:
	pAdCB			- WLAN control block pointer
	pVIE			- Beacon elements
	LenVIE			- Total length of Beacon elements

Return Value:
	NONE

Note:
========================================================================
*/
VOID CFG80211_BeaconCountryRegionParse(
	IN VOID						*pAdCB,
	IN NDIS_802_11_VARIABLE_IEs	*pVIE,
	IN UINT16					LenVIE)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	UCHAR *pElement = (UCHAR *)pVIE;
	UINT32 LenEmt;


	while(LenVIE > 0)
	{
		pVIE = (NDIS_802_11_VARIABLE_IEs *)pElement;

		if (pVIE->ElementID == IE_COUNTRY)
		{
			/* send command to do regulation hint only when associated */
			RTEnqueueInternalCmd(pAd, CMDTHREAD_REG_HINT_11D,
								pVIE->data, pVIE->Length);
			break;
		} /* End of if */

		LenEmt = pVIE->Length + 2;

		if (LenVIE <= LenEmt)
			break; /* length is not enough */
		/* End of if */

		pElement += LenEmt;
		LenVIE -= LenEmt;
	} /* End of while */
} /* End of CFG80211_BeaconCountryRegionParse */

VOID CFG80211_LostGoInform(
    IN VOID 					*pAdCB)
{

	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	//CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
	PNET_DEV pNetDev = NULL;
	
	DBGPRINT(RT_DEBUG_TRACE, ("80211> CFG80211_LostGoInform ==> \n"));
	pAd->FlgCfg80211Connecting = FALSE;

	if ((pAd->Cfg80211VifDevSet.vifDevList.size > 0) &&        
	((pNetDev = RTMP_CFG80211_FindVifEntry_ByType(pAd, RT_CMD_80211_IFTYPE_P2P_CLIENT)) != NULL))
	{
		//cfg80211_disconnected(pNetDev, 0, NULL, 0, GFP_KERNEL);
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
	        if (pNetDev->ieee80211_ptr->sme_state == CFG80211_SME_CONNECTING)
       	 	{
                   cfg80211_connect_result(pNetDev, NULL, NULL, 0, NULL, 0,
                                                                   WLAN_STATUS_UNSPECIFIED_FAILURE, GFP_KERNEL);
        	}
        	else if (pNetDev->ieee80211_ptr->sme_state == CFG80211_SME_CONNECTED)
        	{
        	   //Solve with Xiaomi and act as GC, disconnect slow issue
                   //cfg80211_disconnected(pNetDev, 0, NULL, 0, GFP_KERNEL);
				kalCfg80211Disconnected
					(pNetDev, WLAN_REASON_DEAUTH_LEAVING,
						NULL, 0, FALSE, GFP_KERNEL);
        	}
		#else
			kalCfg80211Disconnected(pNetDev, 0, NULL, 0, FALSE, GFP_KERNEL);
		#endif
	}
	else
		DBGPRINT(RT_DEBUG_ERROR, ("80211> BUG CFG80211_LostGoInform, BUT NetDevice not exist.\n"));
		
	Set_ApCli_Enable_Proc(pAd, "0");	
	
}
/*
========================================================================
Routine Description:
	Re-Initialize wireless channel/PHY in 2.4GHZ and 5GHZ.

Arguments:
	pAdCB			- WLAN control block pointer

Return Value:
	NONE

Note:
	CFG80211_SupBandInit() is called in xx_probe().
========================================================================
*/
#ifdef CONFIG_STA_SUPPORT
VOID CFG80211_LostApInform(
    IN VOID 					*pAdCB)
{

	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
#if KERNEL_VERSION(3, 11, 0) > LINUX_VERSION_CODE
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
#endif

	pAd->StaCfg.bAutoReconnect = FALSE;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
	DBGPRINT(RT_DEBUG_TRACE, ("80211> CFG80211_LostApInform ==> %d\n", 
					p80211CB->pCfg80211_Wdev->sme_state));

	//cfg80211_disconnected(pAd->net_dev, 0, NULL, 0, GFP_KERNEL);
	if (p80211CB->pCfg80211_Wdev->sme_state == CFG80211_SME_CONNECTING)
	{
		   cfg80211_connect_result(pAd->net_dev, NULL, NULL, 0, NULL, 0,
								   WLAN_STATUS_UNSPECIFIED_FAILURE, GFP_KERNEL);
	}
	else if (p80211CB->pCfg80211_Wdev->sme_state == CFG80211_SME_CONNECTED)
	{
		   kalCfg80211Disconnected(pAd->net_dev, 0, NULL, 0, FALSE, GFP_KERNEL);
	} 
#else
	kalCfg80211Disconnected(pAd->net_dev, 0, NULL, 0, FALSE, GFP_KERNEL);
#endif

}
#endif /*CONFIG_STA_SUPPORT*/

BOOLEAN CFG80211DRV_OpsCancelRemainOnChannel(
	VOID                                            *pAdOrg,
	UINT32                                          cookie)
{	
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	BOOLEAN Cancelled;
	CFG80211DBG(RT_DEBUG_TRACE, ("%s\n", __FUNCTION__));

	if (pAd->Cfg80211RocTimerRunning == TRUE)
	{
			CFG80211DBG(RT_DEBUG_TRACE, ("CFG_ROC : CANCEL Cfg80211RemainOnChannelDurationTimer\n"));
			RTMPCancelTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, &Cancelled);
			pAd->Cfg80211RocTimerRunning = FALSE;
	}

//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
//	cfg80211_remain_on_channel_expired(pAd->dummy_p2p_net_dev, cookie, pAd->CfgChanInfo.chan, 
//				pAd->CfgChanInfo.ChanType, GFP_KERNEL);
//#endif

	return TRUE;	
}

/* Set a given time on specific channel to listen action Frame */
BOOLEAN CFG80211DRV_OpsRemainOnChannel(	
	VOID						*pAdOrg,	
	VOID						*pData,	
	UINT32 						duration)
{	
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_CHAN *pChanInfo;
	//STRING ChStr[5] = { 0 };
	BOOLEAN     Cancelled;

	CFG80211DBG(RT_DEBUG_INFO, ("%s\n", __FUNCTION__));
	pChanInfo = (CMD_RTPRIV_IOCTL_80211_CHAN *) pData;

	/* Will be Exit the ApCli Connected Channel so send Null frame on current */
	if ( pAd->ApCfg.ApCliTab[MAIN_MBSSID].Valid && 
	     RTMP_CFG80211_VIF_P2P_CLI_ON(pAd) &&
	    (pAd->ApCliMlmeAux.Channel != pChanInfo->ChanId) &&
           (pAd->ApCliMlmeAux.Channel == pAd->LatchRfRegs.Channel))	
	{
                DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: APCLI PWR_SAVE ROC_START\n"));
                CFG80211_P2pClientSendNullFrame(pAd, PWR_SAVE);
	}

	if (INFRA_ON(pAd) &&
	    ((pAd->CommonCfg.Channel != pChanInfo->ChanId) || (pAd->CommonCfg.CentralChannel != pChanInfo->ChanId)) &&
        ((pAd->CommonCfg.Channel == pAd->LatchRfRegs.Channel) || (pAd->CommonCfg.CentralChannel== pAd->LatchRfRegs.Channel)))	
	{
                DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_NULL: STA PWR_SAVE ROC_START\n"));
		  RTMPSendNullFrame(pAd, pAd->CommonCfg.TxRate, 
							  (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED) ? TRUE:FALSE),
							  PWR_SAVE);				
	}	

	/* Channel Switch Case:
	 * 1. P2P_FIND:    [SOCIAL_CH]->[COM_CH]->[ROC_CH]--N_TUs->[ROC_TIMEOUT]
	 *                 Set COM_CH to ROC_CH for merge COM_CH & ROC_CH dwell section.
     	 *	 
	 * 2. OFF_CH_WAIT: [ROC_CH]--200ms-->[ROC_TIMEOUT]->[COM_CH]
	 *                 Most in GO case.
	 * 
	 */

	if (pAd->LatchRfRegs.Channel != pChanInfo->ChanId) 
	{
		if (INFRA_ON(pAd) && pAd->CommonCfg.BBPCurrentBW == BW_40 && 
		    pAd->LatchRfRegs.Channel == pAd->CommonCfg.CentralChannel && 
           	    pChanInfo->ChanId == pAd->CommonCfg.Channel)
        {
           		DBGPRINT(RT_DEBUG_TRACE, ("ROC -> Do not change ch\n"));
        }	
        else
	    {
		    	DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_PKT: ROC CHANNEL_LOCK %d\n", pChanInfo->ChanId));
	    	    rtmp_bbp_set_bw(pAd,BW_20);      
				AsicSwitchChannel(pAd, pChanInfo->ChanId, FALSE);
				AsicLockChannel(pAd, pChanInfo->ChanId);

        }	
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_PKT: ROC CHANNEL_LOCK %d\n", pChanInfo->ChanId));
		//AsicSwitchChannel(pAd, pChanInfo->ChanId, FALSE);
		//AsicLockChannel(pAd, pChanInfo->ChanId);
		
	}
	else
	{
		CFG80211DBG(RT_DEBUG_INFO, ("80211> ComCH == ROC_CH \n"));
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,7,2))
	cfg80211_ready_on_channel(pAd->dummy_p2p_net_dev->ieee80211_ptr,  pChanInfo->cookie, pChanInfo->chan,duration, GFP_KERNEL);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
	cfg80211_ready_on_channel((struct wireless_dev *)(pAd->dummy_p2p_net_dev),  pChanInfo->cookie, pChanInfo->chan, pChanInfo->ChanType, duration, GFP_KERNEL);
#else
	cfg80211_ready_on_channel(pAd->dummy_p2p_net_dev,  pChanInfo->cookie, pChanInfo->chan, pChanInfo->ChanType, duration, GFP_KERNEL);
#endif
#endif
	NdisCopyMemory(&pAd->CfgChanInfo, pChanInfo, sizeof(CMD_RTPRIV_IOCTL_80211_CHAN));

	if (pAd->Cfg80211RocTimerInit == FALSE)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("CFG80211_ROC : INIT Cfg80211RemainOnChannelDurationTimer\n"));
		RTMPInitTimer(pAd, &pAd->Cfg80211RemainOnChannelDurationTimer, GET_TIMER_FUNCTION(RemainOnChannelTimeout), pAd, FALSE);
		pAd->Cfg80211RocTimerInit = TRUE;
	}
	
	if (pAd->Cfg80211RocTimerRunning == TRUE)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("CFG80211_ROC : CANCEL Cfg80211RemainOnChannelDurationTimer\n"));
		RTMPCancelTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, &Cancelled);
		pAd->Cfg80211RocTimerRunning = FALSE;
	}

	RTMPSetTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, duration + 20);
	pAd->Cfg80211RocTimerRunning = TRUE;

	return TRUE;	
}


/*
========================================================================
Routine Description:
	Hint to the wireless core a regulatory domain from driver.

Arguments:
	pAd				- WLAN control block pointer
	pCountryIe		- pointer to the country IE
	CountryIeLen	- length of the country IE

Return Value:
	NONE

Note:
	Must call the function in kernel thread.
========================================================================
*/
VOID CFG80211_RegHint(
	IN VOID						*pAdCB,
	IN UCHAR					*pCountryIe,
	IN ULONG					CountryIeLen)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;


	CFG80211OS_RegHint(CFG80211CB, pCountryIe, CountryIeLen);
} /* End of CFG80211_RegHint */


/*
========================================================================
Routine Description:
	Hint to the wireless core a regulatory domain from country element.

Arguments:
	pAdCB			- WLAN control block pointer
	pCountryIe		- pointer to the country IE
	CountryIeLen	- length of the country IE

Return Value:
	NONE

Note:
	Must call the function in kernel thread.
========================================================================
*/
VOID CFG80211_RegHint11D(
	IN VOID						*pAdCB,
	IN UCHAR					*pCountryIe,
	IN ULONG					CountryIeLen)
{
	/* no regulatory_hint_11d() in 2.6.32 */
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;


	CFG80211OS_RegHint11D(CFG80211CB, pCountryIe, CountryIeLen);
} /* End of CFG80211_RegHint11D */


/*
========================================================================
Routine Description:
	Apply new regulatory rule.

Arguments:
	pAdCB			- WLAN control block pointer
	pWiphy			- Wireless hardware description
	pAlpha2			- Regulation domain (2B)

Return Value:
	NONE

Note:
	Can only be called when interface is up.

	For general mac80211 device, it will be set to new power by Ops->config()
	In rt2x00/, the settings is done in rt2x00lib_config().
========================================================================
*/
VOID CFG80211_RegRuleApply(
	IN VOID						*pAdCB,
	IN VOID						*pWiphy,
	IN UCHAR					*pAlpha2)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	VOID *pBand24G, *pBand5G;
	UINT32 IdBand, IdChan, IdPwr;
	UINT32 ChanNum, ChanId, Power, RecId, DfsType;
	BOOLEAN FlgIsRadar;
	ULONG IrqFlags;
#ifdef DFS_SUPPORT	
	RADAR_DETECT_STRUCT	*pRadarDetect;
#endif /* DFS_SUPPORT */


	CFG80211DBG(RT_DEBUG_TRACE, ("crda> CFG80211_RegRuleApply ==>\n"));

	/* init */
	pBand24G = NULL;
	pBand5G = NULL;

	if (pAd == NULL)
		return;

	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);

	/* zero first */
	NdisZeroMemory(pAd->ChannelList,
					MAX_NUM_OF_CHANNELS * sizeof(CHANNEL_TX_POWER));

	/* 2.4GHZ & 5GHz */
	RecId = 0;
#ifdef DFS_SUPPORT	
	pRadarDetect = &pAd->CommonCfg.RadarDetect;
#endif /* DFS_SUPPORT */

	/* find the DfsType */
	DfsType = CE;

	pBand24G = NULL;
	pBand5G = NULL;

	if (CFG80211OS_BandInfoGet(CFG80211CB, pWiphy, &pBand24G, &pBand5G) == FALSE)
		return;

#ifdef AUTO_CH_SELECT_ENHANCE
#ifdef EXT_BUILD_CHANNEL_LIST
	if ((pAlpha2[0] != '0') && (pAlpha2[1] != '0'))
	{
		UINT32 IdReg;

		if (pBand5G != NULL)
		{
			for(IdReg=0; ; IdReg++)
			{
				if (ChRegion[IdReg].CountReg[0] == 0x00)
					break;
				/* End of if */
	
				if ((pAlpha2[0] == ChRegion[IdReg].CountReg[0]) &&
					(pAlpha2[1] == ChRegion[IdReg].CountReg[1]))
				{
					if (pAd->CommonCfg.DfsType != MAX_RD_REGION)
						DfsType = pAd->CommonCfg.DfsType;
					else
						DfsType = ChRegion[IdReg].DfsType;
	
					CFG80211DBG(RT_DEBUG_TRACE,
								("crda> find region %c%c, DFS Type %d\n",
								pAlpha2[0], pAlpha2[1], DfsType));
					break;
				} /* End of if */
			} /* End of for */
		} /* End of if */
	} /* End of if */
#endif /* EXT_BUILD_CHANNEL_LIST */
#endif /* AUTO_CH_SELECT_ENHANCE */

	for(IdBand=0; IdBand<2; IdBand++)
	{
		if (((IdBand == 0) && (pBand24G == NULL)) ||
			((IdBand == 1) && (pBand5G == NULL)))
		{
			continue;
		} /* End of if */

		if (IdBand == 0)
		{
			CFG80211DBG(RT_DEBUG_TRACE, ("crda> reset chan/power for 2.4GHz\n"));
		}
		else
		{
			CFG80211DBG(RT_DEBUG_TRACE, ("crda> reset chan/power for 5GHz\n"));
		} /* End of if */

		ChanNum = CFG80211OS_ChanNumGet(CFG80211CB, pWiphy, IdBand);

		for(IdChan=0; IdChan<ChanNum; IdChan++)
		{
			if (CFG80211OS_ChanInfoGet(CFG80211CB, pWiphy, IdBand, IdChan,
									&ChanId, &Power, &FlgIsRadar) == FALSE)
			{
				/* the channel is not allowed in the regulatory domain */
				/* get next channel information */
				continue;
			} /* End of if */

			if ((pAd->CommonCfg.PhyMode == PHY_11A) ||
				(pAd->CommonCfg.PhyMode == PHY_11AN_MIXED))
			{
				/* 5G-only mode */
				if (ChanId <= CFG80211_NUM_OF_CHAN_2GHZ)
					continue; /* check next */
				/* End of if */
			} /* End of if */

			if ((pAd->CommonCfg.PhyMode != PHY_11A) &&
				(pAd->CommonCfg.PhyMode != PHY_11ABG_MIXED) &&
				(pAd->CommonCfg.PhyMode != PHY_11AN_MIXED) &&
				(pAd->CommonCfg.PhyMode != PHY_11ABGN_MIXED) &&
				(pAd->CommonCfg.PhyMode != PHY_11AGN_MIXED))
			{
				/* 2.5G-only mode */
				if (ChanId > CFG80211_NUM_OF_CHAN_2GHZ)
					continue; /* check next */
				/* End of if */
			} /* End of if */

			for(IdPwr=0; IdPwr<MAX_NUM_OF_CHANNELS; IdPwr++)
			{
				if (ChanId == pAd->TxPower[IdPwr].Channel)
				{
					/* init the channel info. */
					NdisMoveMemory(&pAd->ChannelList[RecId],
									&pAd->TxPower[IdPwr],
									sizeof(CHANNEL_TX_POWER));

					/* keep channel number */
					pAd->ChannelList[RecId].Channel = ChanId;

					/* keep maximum tranmission power */
					pAd->ChannelList[RecId].MaxTxPwr = Power;

					/* keep DFS flag */
					if (FlgIsRadar == TRUE)
						pAd->ChannelList[RecId].DfsReq = TRUE;
					else
						pAd->ChannelList[RecId].DfsReq = FALSE;
					/* End of if */

					/* keep DFS type */
					pAd->ChannelList[RecId].RegulatoryDomain = DfsType;

					/* re-set DFS info. */
					pAd->CommonCfg.RDDurRegion = DfsType;

					CFG80211DBG(RT_DEBUG_TRACE,
								("Chan %03d:\tpower %d dBm, "
								"DFS %d, DFS Type %d\n",
								ChanId, Power,
								((FlgIsRadar == TRUE)?1:0),
								DfsType));

					/* change to record next channel info. */
					RecId ++;
					break;
				} /* End of if */
			} /* End of for */
		} /* End of for */
	} /* End of for */

	pAd->ChannelListNum = RecId;
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

	CFG80211DBG(RT_DEBUG_TRACE, ("crda> Number of channels = %d\n", RecId));
} /* End of CFG80211_RegRuleApply */


/*
========================================================================
Routine Description:
	Inform us that a scan is got.

Arguments:
	pAdCB				- WLAN control block pointer

Return Value:
	NONE

Note:
	Call RT_CFG80211_SCANNING_INFORM, not CFG80211_Scaning
========================================================================
*/
VOID CFG80211_Scaning(
	IN VOID							*pAdCB,
	IN UINT32						BssIdx,
	IN UINT32						ChanId,
	IN UCHAR						*pFrame,
	IN UINT32						FrameLen,
	IN INT32						RSSI)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	VOID *pCfg80211_CB = pAd->pCfg80211_CB;
	BOOLEAN FlgIsNMode;
	UINT8 BW;


	//CFG80211DBG(RT_DEBUG_ERROR, ("80211> CFG80211_Scaning ==>\n"));

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("80211> Network is down!\n"));
		return;
	} /* End of if */

	/*
		In connect function, we also need to report BSS information to cfg80211;
		Not only scan function.
	*/
	if ((pAd->FlgCfg80211Scanning == FALSE) &&
		(pAd->FlgCfg80211Connecting == FALSE))
	{
		
		//CFG80211DBG(RT_DEBUG_ERROR, ("YF DEBUG: FlgCfg80211Scanning & FlgCfg80211Connecting ALL False\n"));
		return; /* no scan is running */
	} /* End of if */

	/* init */
	/* Note: Can not use local variable to do pChan */
	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
		FlgIsNMode = TRUE;
	else
		FlgIsNMode = FALSE;

	if (pAd->CommonCfg.RegTransmitSetting.field.BW == BW_20)
		BW = 0;
	else
		BW = 1;

	CFG80211OS_Scaning(pCfg80211_CB,
						ChanId,
						pFrame,
						FrameLen,
						RSSI,
						FlgIsNMode,
						BW);
#endif /* CONFIG_STA_SUPPORT */
} /* End of CFG80211_Scaning */

static void CFG80211_UpdateBssAvgRssi(
	IN      PBSS_ENTRY                              pBssEntry)
{
        BOOLEAN bInitial = FALSE;

        if (!(pBssEntry->AvgRssiX8 | pBssEntry->AvgRssi))
        {
                bInitial = TRUE;
        }

        if (bInitial)
        {
                pBssEntry->AvgRssiX8 = pBssEntry->Rssi << 3;
                pBssEntry->AvgRssi  = pBssEntry->Rssi;
        }
        else
        {
                pBssEntry->AvgRssiX8 = (pBssEntry->AvgRssiX8 - pBssEntry->AvgRssi) + pBssEntry->Rssi;
        }

        pBssEntry->AvgRssi = pBssEntry->AvgRssiX8 >> 3;

}

/*
========================================================================
Routine Description:
	Inform us that scan ends.

Arguments:
	pAdCB			- WLAN control block pointer
	FlgIsAborted	- 1: scan is aborted

Return Value:
	NONE

Note:
========================================================================
*/
VOID CFG80211_ScanEnd(
	IN VOID						*pAdCB,
	IN BOOLEAN					FlgIsAborted)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	UINT32 index;
	PBSS_ENTRY pBssEntry;
	CFG80211_CB *pCfg80211_CB  = (CFG80211_CB *)pAd->pCfg80211_CB;
	struct wiphy *pWiphy = pCfg80211_CB->pCfg80211_Wdev->wiphy;
	struct ieee80211_channel *chan;
	UINT32 CenFreq;
	struct cfg80211_bss *bss;

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("80211> Network is down!\n"));
		return;
	} /* End of if */

	if (pAd->FlgCfg80211Scanning == FALSE)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("80211> No scan is running!\n"));
		return; /* no scan is running */
	} /* End of if */

	if (FlgIsAborted == TRUE)
		FlgIsAborted = 1;
	else
	{
		FlgIsAborted = 0;
		for (index = 0; index < pAd->ScanTab.BssNr; index++) 
		{
			pBssEntry = &pAd->ScanTab.BssEntry[index];
			
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39)) 
			if (pAd->ScanTab.BssEntry[index].Channel > 14) 
				CenFreq = ieee80211_channel_to_frequency
					(pAd->ScanTab.BssEntry[index].Channel, KAL_BAND_5GHZ);
			else 
				CenFreq = ieee80211_channel_to_frequency
					(pAd->ScanTab.BssEntry[index].Channel, KAL_BAND_2GHZ);
#else
			CenFreq = ieee80211_channel_to_frequency(pAd->ScanTab.BssEntry[index].Channel);
#endif

			chan = ieee80211_get_channel(pWiphy, CenFreq);			
			bss = cfg80211_get_bss(pWiphy, chan, pBssEntry->Bssid, pBssEntry->Ssid, pBssEntry->SsidLen, 
						WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
			if (bss == NULL)
			{
				/* ScanTable Entry not exist in kernel buffer */
			}
			else
			{
				/* HIt */
				CFG80211_UpdateBssAvgRssi(pBssEntry);
				bss->signal = pBssEntry->AvgRssi * 100; //UNIT: MdBm
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
				cfg80211_put_bss(pWiphy, bss);
#else
				cfg80211_put_bss(bss);
#endif
			}
		}
	}
 
	CFG80211OS_ScanEnd(CFG80211CB, FlgIsAborted);

	pAd->FlgCfg80211Scanning = FALSE;
#endif /* CONFIG_STA_SUPPORT */
} /* End of CFG80211_ScanEnd */


/*
========================================================================
Routine Description:
	Inform CFG80211 about association status.

Arguments:
	pAdCB			- WLAN control block pointer
	pBSSID			- the BSSID of the AP
	pReqIe			- the element list in the association request frame
	ReqIeLen		- the request element length
	pRspIe			- the element list in the association response frame
	RspIeLen		- the response element length
	FlgIsSuccess	- 1: success; otherwise: fail

Return Value:
	None

Note:
========================================================================
*/
VOID CFG80211_ConnectResultInform(
	IN VOID						*pAdCB,
	IN UCHAR					*pBSSID,
	IN UCHAR					*pReqIe,
	IN UINT32					ReqIeLen,
	IN UCHAR					*pRspIe,
	IN UINT32					RspIeLen,
	IN UCHAR					FlgIsSuccess)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;


	CFG80211DBG(RT_DEBUG_TRACE, ("80211> CFG80211_ConnectResultInform ==>\n"));

	CFG80211OS_ConnectResultInform(CFG80211CB,
								pBSSID,
								pReqIe,
								ReqIeLen,
								pRspIe,
								RspIeLen,
								FlgIsSuccess);

	pAd->FlgCfg80211Connecting = FALSE;
} /* End of CFG80211_ConnectResultInform */

VOID CFG80211_P2pClientConnectResultInform(
	IN VOID                                         *pAdCB,
        IN UCHAR                                        *pBSSID,
        IN UCHAR                                        *pReqIe,
        IN UINT32                                       ReqIeLen,
        IN UCHAR                                        *pRspIe,
        IN UINT32                                       RspIeLen,
        IN UCHAR                                        FlgIsSuccess)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;

	CFG80211OS_P2pClientConnectResultInform(pAd->ApCfg.ApCliTab[MAIN_MBSSID].dev, pBSSID, 
					pReqIe, ReqIeLen, pRspIe, RspIeLen, FlgIsSuccess);

	pAd->FlgCfg80211Connecting = FALSE;
}


/*
========================================================================
Routine Description:
	Re-Initialize wireless channel/PHY in 2.4GHZ and 5GHZ.

Arguments:
	pAdCB			- WLAN control block pointer

Return Value:
	TRUE			- re-init successfully
	FALSE			- re-init fail

Note:
	CFG80211_SupBandInit() is called in xx_probe().
	But we do not have complete chip information in xx_probe() so we
	need to re-init bands in xx_open().
========================================================================
*/
BOOLEAN CFG80211_SupBandReInit(
	IN VOID						*pAdCB)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	CFG80211_BAND BandInfo;


	CFG80211DBG(RT_DEBUG_TRACE, ("80211> re-init bands...\n"));

	/* re-init bands */
	NdisZeroMemory(&BandInfo, sizeof(BandInfo));
	CFG80211_BANDINFO_FILL(pAd, &BandInfo);

	return CFG80211OS_SupBandReInit(CFG80211CB, &BandInfo);
} /* End of CFG80211_SupBandReInit */


INT CFG80211_StaPortSecured(
	IN VOID                                         *pAdCB,
	IN UCHAR 					*pMac,
	IN UINT						flag)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	MAC_TABLE_ENTRY *pEntry;

	pEntry = MacTableLookup(pAd, pMac);
	if (!pEntry)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Can't find pEntry in CFG80211_StaPortSecured\n"));
	}
	else
	{
		if (flag)
		{
			printk("AID:%d, PortSecured\n", pEntry->Aid);
			pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
			pEntry->WpaState = AS_PTKINITDONE;
			pEntry->PortSecured = WPA_802_1X_PORT_SECURED;	
		}
		else
		{
			printk("AID:%d, PortNotSecured\n", pEntry->Aid);
			pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
			pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;	
		}	
	}
	
	return 0;
}

VOID CFG80211_ApStaDel(
	IN VOID                                         *pAdCB,
	IN UCHAR                                        *pMac)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	MAC_TABLE_ENTRY *pEntry;
	UCHAR pMacF[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //wpa_supplicant will set pMac=FF:FF:FF:FF:FF:FF when remove p2p group
	if (pMac == NULL || MAC_ADDR_EQUAL(pMacF, pMac))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Deauthenticate all stations!\n"));
		/* From WCID=2 */
		if (INFRA_ON(pAd))
			P2PMacTableReset(pAd);
		else
			MacTableReset(pAd);	
	}
	else
	{
		pEntry = MacTableLookup(pAd, pMac);
		if (pEntry)
		{
			MlmeDeAuthAction(pAd, pEntry, 2, FALSE);
		}
		else
			DBGPRINT(RT_DEBUG_ERROR, ("Can't find pEntry in ApStaDel\n"));
	}
}

#ifdef CONFIG_STA_SUPPORT
//CMD_RTPRIV_IOCTL_80211_KEY_DEFAULT_SET:
INT CFG80211_setStaDefaultKey(
	IN VOID                                         *pAdCB,
	IN UINT 					Data
)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;

	DBGPRINT(RT_DEBUG_TRACE, ("Set Sta Default Key: %d\n", Data));
    pAd->StaCfg.DefaultKeyId = Data; /* base 0 */
	return 0;	
}
#endif /*CONFIG_STA_SUPPORT*/

INT CFG80211_setApDefaultKey(
	IN VOID                                         *pAdCB,
	IN UINT 					Data
)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set Ap Default Key: %d\n", Data));
    pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId = Data;
	return 0;	
}


INT CFG80211_reSetToDefault(
	IN VOID                                         *pAdCB)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	DBGPRINT(RT_DEBUG_TRACE, (" %s\n", __FUNCTION__));

#ifdef RT_CFG80211_SUPPORT
         if (pAd->FlgCfg80211Scanning == TRUE)
         {
                         DBGPRINT(RT_DEBUG_ERROR, ("WARN: close the scan cmd in device close phase\n"));
                         CFG80211OS_ScanEnd(pAd->pCfg80211_CB, TRUE);
				OS_WAIT(1000);
         }
#endif /* RT_CFG80211_SUPPORT */

	
	pAd->Cfg80211RegisterProbeReqFrame = FALSE;
	pAd->Cfg80211RegisterActionFrame = FALSE;
	pAd->Cfg80211ProbeReqCount = 0;
	pAd->Cfg80211ActionCount = 0;

	pAd->Cfg80211RocTimerInit = FALSE;
	pAd->Cfg80211RocTimerRunning = FALSE;

	pAd->FlgCfg80211Scanning = FALSE;

	return TRUE;
}

VOID CFG80211_IndicateScanFail(
	IN VOID                                         *pAdCB)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
	
	CFG80211OS_ScanEnd(p80211CB, TRUE);
}

BOOLEAN CFG80211_checkScanResInKernelCache(
        IN VOID                                         *pAdCB,
        IN UCHAR                                        *pBSSID,
	IN UCHAR					*pSsid,
	IN INT       					ssidLen)
{
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
        CFG80211_CB *pCfg80211_CB  = (CFG80211_CB *)pAd->pCfg80211_CB;
        struct wiphy *pWiphy = pCfg80211_CB->pCfg80211_Wdev->wiphy;
        //ULONG bss_idx = BSS_NOT_FOUND;
        struct cfg80211_bss *bss;
	
	bss = cfg80211_get_bss(pWiphy, NULL, pBSSID,
                               pSsid, ssidLen,
                               WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
	if (bss)
        {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
		cfg80211_put_bss(pWiphy, bss);
#else
		cfg80211_put_bss(bss);
#endif
                return TRUE;
        }

	return FALSE;
}

BOOLEAN CFG80211_checkScanTable(
        IN VOID                                         *pAdCB)
{
    PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
    CFG80211_CB *pCfg80211_CB  = (CFG80211_CB *)pAd->pCfg80211_CB;
    struct wiphy *pWiphy = pCfg80211_CB->pCfg80211_Wdev->wiphy;
	ULONG bss_idx = BSS_NOT_FOUND;
	struct cfg80211_bss *bss;
	struct ieee80211_channel *chan;
	UINT32 CenFreq;
	UINT64 timestamp;
	struct timeval tv;
	UCHAR *ie, ieLen = 0;
	BOOLEAN isOk = FALSE;	
	PBSS_ENTRY pBssEntry;

        if (MAC_ADDR_EQUAL(pAd->ApCliMlmeAux.Bssid, ZERO_MAC_ADDR))
        {
		CFG80211DBG(RT_DEBUG_ERROR, ("pAd->ApCliMlmeAux.Bssid ==> ZERO_MAC_ADDR\n"));
		//ToDo: pAd->ApCfg.ApCliTab[0].CfgApCliBssid
                return FALSE;
        }

	/* Fake TSF */
	do_gettimeofday(&tv);
	timestamp = ((UINT64)tv.tv_sec * 1000000) + tv.tv_usec;

	bss = cfg80211_get_bss(pWiphy, NULL, pAd->ApCliMlmeAux.Bssid,
			       pAd->ApCliMlmeAux.Ssid, pAd->ApCliMlmeAux.SsidLen, 
			       WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
	if (bss)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Found %s in Kernel_ScanTable with CH[%d]\n", pAd->ApCliMlmeAux.Ssid, bss->channel->center_freq));
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
		cfg80211_put_bss(pWiphy, bss);
#else		
		bss->tsf = timestamp;
	cfg80211_put_bss(bss);
#endif		
		return TRUE;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Can't Found %s in Kernel_ScanTable & Try Fake it\n", pAd->ApCliMlmeAux.Ssid));
	}

	bss_idx = BssSsidTableSearchBySSID(&pAd->ScanTab, pAd->ApCliMlmeAux.Ssid, pAd->ApCliMlmeAux.SsidLen);

	if (bss_idx != BSS_NOT_FOUND)
	{
		/* Since the cfg80211 kernel scanTable not exist this Entry,
		 * Build an Entry for this connect inform event.  
         	 */
		if (bss_idx>=MAX_LEN_OF_BSS_TABLE)
		{
			ASSERT(0);
			return FALSE;
		}
		pBssEntry = &pAd->ScanTab.BssEntry[bss_idx];

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39))
		if (pAd->ScanTab.BssEntry[bss_idx].Channel > 14)
			CenFreq = ieee80211_channel_to_frequency(pBssEntry->Channel, KAL_BAND_5GHZ);
		else
			CenFreq = ieee80211_channel_to_frequency(pBssEntry->Channel, KAL_BAND_2GHZ);
#else
            CenFreq = ieee80211_channel_to_frequency(pBssEntry->Channel);
#endif
        	chan = ieee80211_get_channel(pWiphy, CenFreq);		

		ieLen = 2 + pAd->ApCliMlmeAux.SsidLen + pBssEntry->VarIeFromProbeRspLen;

		os_alloc_mem(NULL, (UCHAR **)&ie, ieLen);
		if (!ie)
		{
			CFG80211DBG(RT_DEBUG_ERROR, ("Memory Allocate Fail in CFG80211_checkScanTable\n"));
			return FALSE;
		}

		ie[0] = WLAN_EID_SSID;
		ie[1] = pAd->ApCliMlmeAux.SsidLen;
		NdisCopyMemory(ie + 2, pAd->ApCliMlmeAux.Ssid, pAd->ApCliMlmeAux.SsidLen);
		NdisCopyMemory(ie + 2 + pAd->ApCliMlmeAux.SsidLen, pBssEntry->pVarIeFromProbRsp, 
				pBssEntry->VarIeFromProbeRspLen);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))	
		bss = cfg80211_inform_bss(pWiphy, chan,CFG80211_BSS_FTYPE_UNKNOWN,
					  pAd->ApCliMlmeAux.Bssid, timestamp, WLAN_CAPABILITY_ESS, pAd->ApCliMlmeAux.BeaconPeriod,
					  ie, ieLen,
					  (pBssEntry->AvgRssi * 100), GFP_KERNEL);
#else
		bss = cfg80211_inform_bss(pWiphy, chan,
					  pAd->ApCliMlmeAux.Bssid, timestamp, WLAN_CAPABILITY_ESS, pAd->ApCliMlmeAux.BeaconPeriod,
					  ie, ieLen,
					  (pBssEntry->AvgRssi * 100), GFP_KERNEL);	
#endif
		if (bss)
		{
			printk("Fake New %s(%02x:%02x:%02x:%02x:%02x:%02x) in Kernel_ScanTable with CH[%d][%d] BI:%d len:%d\n", 
					pAd->ApCliMlmeAux.Ssid, 
					PRINT_MAC(pAd->ApCliMlmeAux.Bssid),bss->channel->center_freq, pBssEntry->Channel,
					pAd->ApCliMlmeAux.BeaconPeriod, pBssEntry->VarIeFromProbeRspLen);	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
			cfg80211_put_bss(pWiphy, bss);
#else
			cfg80211_put_bss(bss);
#endif		
			isOk = TRUE;
		}
		
		if (ie != NULL)
			os_free_mem(NULL, ie);

		if (isOk)
			return TRUE;
	}
	else
		printk("%s Not In Driver Scan Table\n", pAd->ApCliMlmeAux.Ssid);

	return FALSE;
}
VOID CFG80211_ParseBeaconIE(VOID *pAdOrg, VOID *pData,const UCHAR *wpa_ie,  const UCHAR *rsn_ie)
{
	//PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	PMULTISSID_STRUCT pMbss = (PMULTISSID_STRUCT)pData;
	PEID_STRUCT 		 pEid;
	PUCHAR				pTmp;
	NDIS_802_11_ENCRYPTION_STATUS	TmpCipher;
	NDIS_802_11_ENCRYPTION_STATUS	PairCipher;		/* Unicast cipher 1, this one has more secured cipher suite */
	NDIS_802_11_ENCRYPTION_STATUS	PairCipherAux;	/* Unicast cipher 2 if AP announce two unicast cipher suite */
	//NDIS_802_11_ENCRYPTION_STATUS	GroupCipher;	/* Group cipher */
	PAKM_SUITE_STRUCT				pAKM;
	USHORT							Count;
	BOOLEAN bWPA = FALSE;
	BOOLEAN bWPA2 = FALSE;
	BOOLEAN bMix = FALSE;

		/* Security */
	PairCipher	 = Ndis802_11WEPDisabled;
	PairCipherAux = Ndis802_11WEPDisabled;
	
	if ((wpa_ie == NULL) && (rsn_ie == NULL)) //open case
	{
		DBGPRINT(RT_DEBUG_TRACE,("%s:: Open/None case\n", __FUNCTION__));
		pMbss->AuthMode = Ndis802_11AuthModeOpen;
		pMbss->WepStatus = Ndis802_11WEPDisabled;
		pMbss->WpaMixPairCipher = MIX_CIPHER_NOTUSE;
	}
	
	 if ((wpa_ie != NULL)) //wpapsk/tkipaes case
	{
		pEid = (PEID_STRUCT)wpa_ie;
		pTmp =(PUCHAR)pEid;
		if (NdisEqualMemory(pEid->Octet, WPA_OUI, 4))
		{
			pMbss->AuthMode = Ndis802_11AuthModeOpen;
			DBGPRINT(RT_DEBUG_TRACE,("%s:: WPA case\n", __FUNCTION__));
			bWPA = TRUE;
			pTmp   += 11;
				switch (*pTmp)
				{
					case 1:
						DBGPRINT(RT_DEBUG_TRACE,("Group Ndis802_11GroupWEP40Enabled\n"));
						pMbss->GroupKeyWepStatus  = Ndis802_11GroupWEP40Enabled;
						break;
					case 5:
						DBGPRINT(RT_DEBUG_TRACE,("Group Ndis802_11GroupWEP104Enabled\n"));
						pMbss->GroupKeyWepStatus  = Ndis802_11GroupWEP104Enabled;
						break;
					case 2:
						DBGPRINT(RT_DEBUG_TRACE,("Group Ndis802_11TKIPEnable\n"));
						pMbss->GroupKeyWepStatus  = Ndis802_11TKIPEnable;
						break;
					case 4:
						DBGPRINT(RT_DEBUG_TRACE,(" Group Ndis802_11AESEnable\n"));
						pMbss->GroupKeyWepStatus  = Ndis802_11AESEnable;
						break;
					default:
						break;
				}
				/* number of unicast suite*/
				pTmp   += 1;

				/* skip all unicast cipher suites*/
				/*Count = *(PUSHORT) pTmp;				*/
				Count = (pTmp[1]<<8) + pTmp[0];
				pTmp   += sizeof(USHORT);

				/* Parsing all unicast cipher suite*/
				while (Count > 0)
				{
					/* Skip OUI*/
					pTmp += 3;
					TmpCipher = Ndis802_11WEPDisabled;
					switch (*pTmp)
					{
						case 1:
						case 5: /* Although WEP is not allowed in WPA related auth mode, we parse it anyway*/
							TmpCipher = Ndis802_11WEPEnabled;
							break;
						case 2:
							TmpCipher = Ndis802_11TKIPEnable;
							break;
						case 4:
							TmpCipher = Ndis802_11AESEnable;
							break;
						default:
							break;
					}
					if (TmpCipher > PairCipher)
					{
						/* Move the lower cipher suite to PairCipherAux*/
						PairCipherAux = PairCipher;
						PairCipher	= TmpCipher;
					}
					else
					{
						PairCipherAux = TmpCipher;
					}
					pTmp++;
					Count--;
				}
				switch (*pTmp)
				{
					case 1:
						/* Set AP support WPA-enterprise mode*/
							pMbss->AuthMode = Ndis802_11AuthModeWPA;
						break;
					case 2:
						/* Set AP support WPA-PSK mode*/
							pMbss->AuthMode = Ndis802_11AuthModeWPAPSK;
						break;
					default:
						break;
				}
				pTmp   += 1;

					DBGPRINT(RT_DEBUG_TRACE,("AuthMode = %s\n",GetAuthMode(pMbss->AuthMode)));
					if (pMbss->GroupKeyWepStatus == PairCipher)
					{
						pMbss->WpaMixPairCipher = MIX_CIPHER_NOTUSE;
						pMbss->WepStatus=pMbss->GroupKeyWepStatus;
					}
					else
					{
						DBGPRINT(RT_DEBUG_TRACE,("WPA Mix TKIPAES\n"));

						bMix = TRUE;
					}
				pMbss->RSNIE_Len[0] = wpa_ie[1];
				NdisMoveMemory(pMbss->RSN_IE[0], wpa_ie+2, wpa_ie[1]);//copy rsn ie			
		}
		else {
			DBGPRINT(RT_DEBUG_TRACE,("%s:: Open/None case\n", __FUNCTION__));
			pMbss->AuthMode = Ndis802_11AuthModeOpen;		
		}	
	}
	 
	if (rsn_ie != NULL)
	{
		PRSN_IE_HEADER_STRUCT			pRsnHeader;
		PCIPHER_SUITE_STRUCT			pCipher;

		pEid =(PEID_STRUCT)rsn_ie;
		pTmp = (PUCHAR)pEid;
		pRsnHeader = (PRSN_IE_HEADER_STRUCT) pTmp;
				
				/* 0. Version must be 1*/
		if (le2cpu16(pRsnHeader->Version) == 1)
		{
			pTmp   += sizeof(RSN_IE_HEADER_STRUCT);

			/* 1. Check group cipher*/
			pCipher = (PCIPHER_SUITE_STRUCT) pTmp;		

			if (NdisEqualMemory(pTmp, RSN_OUI, 3))
			{	
				DBGPRINT(RT_DEBUG_TRACE,("%s:: WPA2 case\n", __FUNCTION__));
				bWPA2 = TRUE;
				pMbss->AuthMode = Ndis802_11AuthModeOpen;
					switch (pCipher->Type)
					{
						case 1:
							DBGPRINT(RT_DEBUG_TRACE,("Ndis802_11GroupWEP40Enabled\n"));
							pMbss->GroupKeyWepStatus = Ndis802_11GroupWEP40Enabled;
							break;
						case 5:
							DBGPRINT(RT_DEBUG_TRACE,("Ndis802_11GroupWEP104Enabled\n"));
							pMbss->GroupKeyWepStatus = Ndis802_11GroupWEP104Enabled;
							break;
						case 2:
							DBGPRINT(RT_DEBUG_TRACE,("Ndis802_11TKIPEnable\n"));
							pMbss->GroupKeyWepStatus = Ndis802_11TKIPEnable;
							break;
						case 4:
							DBGPRINT(RT_DEBUG_TRACE,("Ndis802_11AESEnable\n"));
							pMbss->GroupKeyWepStatus = Ndis802_11AESEnable;
							break;
						default:
							break;
					}

					/* set to correct offset for next parsing*/
					pTmp   += sizeof(CIPHER_SUITE_STRUCT);

					/* 2. Get pairwise cipher counts*/
					/*Count = *(PUSHORT) pTmp;*/
					Count = (pTmp[1]<<8) + pTmp[0];
					pTmp   += sizeof(USHORT);			

					/* 3. Get pairwise cipher*/
					/* Parsing all unicast cipher suite*/
					while (Count > 0)
					{
						/* Skip OUI*/
						pCipher = (PCIPHER_SUITE_STRUCT) pTmp;
						TmpCipher = Ndis802_11WEPDisabled;
						switch (pCipher->Type)
						{
							case 1:
							case 5: /* Although WEP is not allowed in WPA related auth mode, we parse it anyway*/
								TmpCipher = Ndis802_11WEPEnabled;
								break;
							case 2:
								TmpCipher = Ndis802_11TKIPEnable;
								break;
							case 4:
								TmpCipher = Ndis802_11AESEnable;
								break;
							default:
								break;
						}

						//pMbss->wdev.WepStatus = TmpCipher;
						if (TmpCipher > PairCipher)
						{
							/* Move the lower cipher suite to PairCipherAux*/
							PairCipherAux = PairCipher;
							PairCipher	 = TmpCipher;
						}
						else
						{
							PairCipherAux = TmpCipher;
						}
						pTmp += sizeof(CIPHER_SUITE_STRUCT);
						Count--;
					}

					/* 4. get AKM suite counts*/
					/*Count	= *(PUSHORT) pTmp;*/
					Count = (pTmp[1]<<8) + pTmp[0];
					pTmp   += sizeof(USHORT);

					/* 5. Get AKM ciphers*/
					/* Parsing all AKM ciphers*/
					while (Count > 0)
					{
						pAKM = (PAKM_SUITE_STRUCT) pTmp;
						if (!RTMPEqualMemory(pTmp, RSN_OUI, 3))
							break;

						switch (pAKM->Type)
						{
							case 0:
									pMbss->AuthMode = Ndis802_11AuthModeWPANone;
								break;                                                        
							case 1:
								/* Set AP support WPA-enterprise mode*/
									pMbss->AuthMode = Ndis802_11AuthModeWPA2;
								break;
							case 2:                                                      
								/* Set AP support WPA-PSK mode*/
									pMbss->AuthMode = Ndis802_11AuthModeWPA2PSK;
								break;
							default:
									pMbss->AuthMode = Ndis802_11AuthModeMax;
								break;
						}
						pTmp   += sizeof(AKM_SUITE_STRUCT);
						Count--;
					}		
					DBGPRINT(RT_DEBUG_TRACE,("AuthMode = %s\n",GetAuthMode(pMbss->AuthMode)));
					if (pMbss->GroupKeyWepStatus == PairCipher)
					{
						pMbss->WpaMixPairCipher = MIX_CIPHER_NOTUSE;
						pMbss->WepStatus=pMbss->GroupKeyWepStatus;
					}
					else
					{
						DBGPRINT(RT_DEBUG_TRACE,("WPA2 Mix TKIPAES\n"));
						bMix= TRUE;
					}
					pMbss->RSNIE_Len[0] = rsn_ie[1];
					NdisMoveMemory(pMbss->RSN_IE[0], rsn_ie+2, rsn_ie[1]);//copy rsn ie			
			}
			else {
				DBGPRINT(RT_DEBUG_TRACE,("%s:: Open/None case\n", __FUNCTION__));
				pMbss->AuthMode = Ndis802_11AuthModeOpen;			
			}
		}		
	}

	if (bWPA2 && bWPA)
		{
			if (bMix)
			{
				pMbss->WpaMixPairCipher = WPA_TKIPAES_WPA2_TKIPAES;
				pMbss->WepStatus = Ndis802_11TKIPAESMix;
			}
		} else if (bWPA2) {
			if (bMix)
			{
				pMbss->WpaMixPairCipher = WPA_NONE_WPA2_TKIPAES;
				pMbss->WepStatus = Ndis802_11TKIPAESMix;
			}		
		} else if (bWPA) {
			if (bMix)
			{
				pMbss->WpaMixPairCipher = WPA_TKIPAES_WPA2_NONE;
				pMbss->WepStatus = Ndis802_11TKIPAESMix;
			}
		}	
}

#ifdef CONFIG_AP_SUPPORT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))	
BOOLEAN CFG80211_SyncPacketWmmIe(VOID *pAdOrg, VOID *pData, ULONG dataLen)

{
	//PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	//const UINT WFA_OUI = 0x0050F2;
	//const UCHAR WMM_OUI_TYPE = 0x2;
	//UCHAR *wmm_ie = NULL;
	//UINT i = QID_AC_BE;
	return TRUE;//????
#if 0
	//hex_dump("probe_rsp_in:", pData, dataLen);
	//pTemp = (UINT8 *)pData;
	wmm_ie = cfg80211_find_vendor_ie(WFA_OUI, WMM_OUI_TYPE, pData, dataLen);



	if (wmm_ie != NULL)

        {

#ifdef UAPSD_SUPPORT

                if (pAd->ApCfg.MBSSID[0].UapsdInfo.bAPSDCapable == TRUE)

                {

                        wmm_ie[8] |= 0x80;

                }

#endif /* UAPSD_SUPPORT */


                /* WMM: sync from driver's EDCA paramter */

                for (i = QID_AC_BE; i <= QID_AC_VO; i++)

                {



                        wmm_ie[10+ (i*4)] = (i << 5) +                                     /* b5-6 is ACI */

                                            ((UCHAR)pAd->ApCfg.BssEdcaParm.bACM[i] << 4) + /* b4 is ACM */

                                            (pAd->ApCfg.BssEdcaParm.Aifsn[i] & 0x0f);      /* b0-3 is AIFSN */



                        wmm_ie[11+ (i*4)] = (pAd->ApCfg.BssEdcaParm.Cwmax[i] << 4) +       /* b5-8 is CWMAX */

                                            (pAd->ApCfg.BssEdcaParm.Cwmin[i] & 0x0f);      /* b0-3 is CWMIN */

                        wmm_ie[12+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] & 0xff);/* low byte of TXOP */

                        wmm_ie[13+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] >> 8);  /* high byte of TXOP */

                }



		return TRUE;

        }



	return FALSE;	
#endif
}
#endif
#endif /* CONFIG_AP_SUPPORT */


#endif /* RT_CFG80211_SUPPORT */

/* End of cfg80211drv.c */
