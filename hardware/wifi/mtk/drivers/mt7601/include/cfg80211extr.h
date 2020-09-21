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

	All MAC80211/CFG80211 Function Prototype.

***************************************************************************/

#ifdef RT_CFG80211_SUPPORT

#define RT_CFG80211_REGISTER(__pDev, __pNetDev)								\
	CFG80211_Register(__pDev, __pNetDev);

#define RT_CFG80211_BEACON_CR_PARSE(__pAd, __pVIE, __LenVIE)				\
	CFG80211_BeaconCountryRegionParse((VOID *)__pAd, __pVIE, __LenVIE);

#define RT_CFG80211_CRDA_REG_HINT(__pAd, __pCountryIe, __CountryIeLen)		\
	CFG80211_RegHint((VOID *)__pAd, __pCountryIe, __CountryIeLen);

#define RT_CFG80211_CRDA_REG_HINT11D(__pAd, __pCountryIe, __CountryIeLen)	\
	CFG80211_RegHint11D((VOID *)__pAd, __pCountryIe, __CountryIeLen);

#define RT_CFG80211_CRDA_REG_RULE_APPLY(__pAd)								\
	CFG80211_RegRuleApply((VOID *)__pAd, NULL, __pAd->Cfg80211_Alpha2);

#define RT_CFG80211_SCANNING_INFORM(__pAd, __BssIdx, __ChanId, __pFrame,	\
			__FrameLen, __RSSI)									\
	CFG80211_Scaning((VOID *)__pAd, __BssIdx, __ChanId, __pFrame,			\
						__FrameLen, __RSSI);
//YF
#define RT_CFG80211_BEACON_TIM_UPDATE(__pAd)                                \
        CFG80211_UpdateBeacon((VOID *)__pAd, NULL, 0, NULL, 0, FALSE);
		
#define RT_CFG80211_LOST_AP_INFORM(__pAd) 									\
	CFG80211_LostApInform((VOID *)__pAd);	

#define RT_CFG80211_LOST_GO_INFORM(__pAd) 									\
	CFG80211_LostGoInform((VOID *)__pAd);	
	
#define RT_CFG80211_SCAN_END(__pAd, __FlgIsAborted)							\
	CFG80211_ScanEnd((VOID *)__pAd, __FlgIsAborted);

#define RT_CFG80211_REINIT(__pAd)											\
	CFG80211_SupBandReInit((VOID *)__pAd);									\

#define RT_CFG80211_CONN_RESULT_INFORM(__pAd, __pBSSID, __pReqIe, __ReqIeLen,\
			__pRspIe, __RspIeLen, __FlgIsSuccess)							\
	CFG80211_ConnectResultInform((VOID *)__pAd, __pBSSID,					\
			__pReqIe, __ReqIeLen, __pRspIe, __RspIeLen, __FlgIsSuccess);

#define RT_CFG80211_P2P_CLI_CONN_RESULT_INFORM(__pAd, __pBSSID, __pReqIe, __ReqIeLen,\
			__pRspIe, __RspIeLen, __FlgIsSuccess)							            \
	CFG80211_P2pClientConnectResultInform(__pAd, __pBSSID,					        \
			__pReqIe, __ReqIeLen, __pRspIe, __RspIeLen, __FlgIsSuccess);

#define RT_CFG80211_P2P_CLI_SEND_NULL_FRAME(_pAd, _PwrMgmt) \
	CFG80211_P2pClientSendNullFrame(_pAd, _PwrMgmt);
			
#define RT_CFG80211_RFKILL_STATUS_UPDATE(_pAd, _active) \
	CFG80211_RFKillStatusUpdate(_pAd, _active);

#ifdef SINGLE_SKU
#define CFG80211_BANDINFO_FILL(__pAd, __pBandInfo)							\
{																			\
	(__pBandInfo)->RFICType = __pAd->RFICType;								\
	(__pBandInfo)->MpduDensity = __pAd->CommonCfg.BACapability.field.MpduDensity;\
	(__pBandInfo)->TxStream = __pAd->CommonCfg.TxStream;					\
	(__pBandInfo)->RxStream = __pAd->CommonCfg.RxStream;					\
	(__pBandInfo)->MaxTxPwr = __pAd->CommonCfg.DefineMaxTxPwr;				\
	if (WMODE_EQUAL(__pAd->CommonCfg.PhyMode, WMODE_B))				\
		(__pBandInfo)->FlgIsBMode = TRUE;									\
	else																	\
		(__pBandInfo)->FlgIsBMode = FALSE;									\
	(__pBandInfo)->MaxBssTable = MAX_LEN_OF_BSS_TABLE;						\
	(__pBandInfo)->RtsThreshold = pAd->CommonCfg.RtsThreshold;				\
	(__pBandInfo)->FragmentThreshold = pAd->CommonCfg.FragmentThreshold;	\
	(__pBandInfo)->RetryMaxCnt = 0;											\
	RTMP_IO_READ32(__pAd, TX_RTY_CFG, &((__pBandInfo)->RetryMaxCnt));		\
}
#else
#define CFG80211_BANDINFO_FILL(__pAd, __pBandInfo)							\
{																			\
	(__pBandInfo)->RFICType = __pAd->RFICType;								\
	(__pBandInfo)->MpduDensity = __pAd->CommonCfg.BACapability.field.MpduDensity;\
	(__pBandInfo)->TxStream = __pAd->CommonCfg.TxStream;					\
	(__pBandInfo)->RxStream = __pAd->CommonCfg.RxStream;					\
	(__pBandInfo)->MaxTxPwr = 0;											\
	if (WMODE_EQUAL(__pAd->CommonCfg.PhyMode, WMODE_B))				\
		(__pBandInfo)->FlgIsBMode = TRUE;									\
	else																	\
		(__pBandInfo)->FlgIsBMode = FALSE;									\
	(__pBandInfo)->MaxBssTable = MAX_LEN_OF_BSS_TABLE;						\
	(__pBandInfo)->RtsThreshold = pAd->CommonCfg.RtsThreshold;				\
	(__pBandInfo)->FragmentThreshold = pAd->CommonCfg.FragmentThreshold;	\
	(__pBandInfo)->RetryMaxCnt = 0;											\
	RTMP_IO_READ32(__pAd, TX_RTY_CFG, &((__pBandInfo)->RetryMaxCnt));		\
}
#endif /* SINGLE_SKU */


/* utilities used in DRV module */
INT CFG80211DRV_IoctlHandle(
	IN	VOID					*pAdSrc,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq,
	IN	INT						cmd,
	IN	USHORT					subcmd,
	IN	VOID					*pData,
	IN	ULONG					Data);

BOOLEAN CFG80211DRV_OpsSetChannel(
	VOID						*pAdOrg,
	VOID						*pData);

VOID CFG80211DRV_OpsChangeBssParm(
        VOID                                            *pAdOrg,
        VOID                                            *pData);

BOOLEAN CFG80211DRV_OpsChgVirtualInf(
	VOID						*pAdOrg,
	VOID						*pFlgFilter,
	UINT8						IfType);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
BOOLEAN CFG80211DRV_OpsScan(
        VOID                                            *pAdOrg);
#else
BOOLEAN CFG80211DRV_OpsScanCheckStatus(
	VOID						*pAdOrg,
	UINT8						IfType);
#endif
BOOLEAN CFG80211DRV_OpsJoinIbss(
	VOID						*pAdOrg,
	VOID						*pData);

BOOLEAN CFG80211DRV_OpsLeave(
	VOID						*pAdOrg,
	UINT8						IfType);

BOOLEAN CFG80211DRV_StaGet(
	VOID						*pAdOrg,
	VOID						*pData);

BOOLEAN CFG80211DRV_Connect(
	VOID						*pAdOrg,
	VOID						*pData);

BOOLEAN CFG80211DRV_StaKeyAdd(
	VOID						*pAdOrg,
	VOID						*pData);

BOOLEAN CFG80211DRV_ApKeyAdd(
        VOID                                            *pAdOrg,
        VOID                                            *pData);

VOID CFG80211DRV_RtsThresholdAdd(
	VOID                                            *pAdOrg,
	UINT                                            threshold);

VOID CFG80211DRV_FragThresholdAdd(
	VOID                                            *pAdOrg,
	UINT                                            threshold);


BOOLEAN CFG80211DRV_ApKeyDel(
	VOID						*pAdOrg,
	VOID						*pData);

VOID CFG80211DRV_RegNotify(
	VOID						*pAdOrg,
	VOID						*pData);

VOID CFG80211DRV_SurveyGet(
	VOID						*pAdOrg,
	VOID						*pData);

VOID CFG80211DRV_PmkidConfig(
	VOID						*pAdOrg,
	VOID						*pData);

VOID CFG80211_RegHint(
	IN VOID						*pAdCB,
	IN UCHAR					*pCountryIe,
	IN ULONG					CountryIeLen);

VOID CFG80211_RegHint11D(
	IN VOID						*pAdCB,
	IN UCHAR					*pCountryIe,
	IN ULONG					CountryIeLen);

VOID CFG80211_ScanEnd(
	IN VOID						*pAdCB,
	IN BOOLEAN					FlgIsAborted);

VOID CFG80211_P2pClientConnectResultInform(
        IN VOID                                         *pAdCB,
        IN UCHAR                                        *pBSSID,
        IN UCHAR                                        *pReqIe,
        IN UINT32                                       ReqIeLen,
        IN UCHAR                                        *pRspIe,
        IN UINT32                                       RspIeLen,
        IN UCHAR                                        FlgIsSuccess);

VOID CFG80211_ConnectResultInform(
	IN VOID						*pAdCB,
	IN UCHAR					*pBSSID,
	IN UCHAR					*pReqIe,
	IN UINT32					ReqIeLen,
	IN UCHAR					*pRspIe,
	IN UINT32					RspIeLen,
	IN UCHAR					FlgIsSuccess);

BOOLEAN CFG80211_SupBandReInit(
	IN VOID						*pAdCB);

VOID CFG80211_RegRuleApply(
	IN VOID						*pAdCB,
	IN VOID						*pWiphy,
	IN UCHAR					*pAlpha2);

VOID CFG80211_Scaning(
	IN VOID						*pAdCB,
	IN UINT32					BssIdx,
	IN UINT32					ChanId,
	IN UCHAR					*pFrame,
	IN UINT32					FrameLen,
	IN INT32					RSSI);

#ifdef RFKILL_HW_SUPPORT
VOID CFG80211_RFKillStatusUpdate(
	IN PVOID					pAd,
	IN BOOLEAN					active);
#endif /* RFKILL_HW_SUPPORT */

VOID CFG80211_UnRegister(
	IN VOID						*pAdOrg,
	IN VOID						*pNetDev);

VOID CFG80211_LostApInform(
	IN VOID 					*pAdCB);

VOID CFG80211_LostGoInform(
    IN VOID 					*pAdCB);	

BOOLEAN CFG80211DRV_OpsExtraIesSet(
	IN VOID						*pAdOrg);

BOOLEAN CFG80211DRV_OpsRemainOnChannel(	
	VOID						*pAdOrg,	
	VOID						*pData,	
	UINT32 						duration);

BOOLEAN CFG80211DRV_OpsCancelRemainOnChannel(
        VOID                                            *pAdOrg,
        UINT32                                          cookie);

VOID CFG80211_ParseBeaconIE(
	VOID *pAdOrg, 
	VOID *pData, 
	const UCHAR *wpa_ie,
	const UCHAR *rsn_ie);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))	
BOOLEAN CFG80211_SyncPacketWmmIe(
	VOID *pAdOrg,
	VOID *pData,
	ULONG dataLen);
#endif

VOID CFG80211DRV_DisableApInterface(
		VOID 									 *pAdOrg);


BOOLEAN CFG80211DRV_OpsBeaconSet(
        VOID                                            *pAdOrg,
        VOID                                            *pData,
        BOOLEAN                                          isAdd);

INT CFG80211_StaPortSecured(
        IN VOID                                         *pAdCB,
        IN UCHAR                                        *pMac,
        IN UINT                                         flag);

VOID CFG80211_ApStaDel(
        IN VOID                                         *pAdCB,
        IN UCHAR                                        *pMac);

INT CFG80211_setApDefaultKey(
        IN VOID                                         *pAdCB,
        IN UINT                                         Data);

#ifdef CONFIG_STA_SUPPORT
INT CFG80211_setStaDefaultKey(
        IN VOID                                         *pAdCB,
        IN UINT                                         Data);
#endif /*CONFIG_STA_SUPPORT*/

BOOLEAN CFG80211DRV_OpsVifAdd(
        VOID                                            *pAdOrg,
        VOID                                            *pData);

VOID CFG80211DRV_OpsMgmtFrameProbeRegister(
        VOID                                            *pAdOrg,
        VOID                                            *pData,
		BOOLEAN                                          isReg);

VOID CFG80211DRV_OpsMgmtFrameActionRegister(
        VOID                                            *pAdOrg,
        VOID                                            *pData,
		BOOLEAN                                          isReg);	

INT CFG80211_reSetToDefault(
	IN VOID                                         *pAdCB);

VOID CFG80211DRV_SetP2pCliAssocIe(
	VOID						*pAdOrg,
	VOID						*pData,
	UINT                         ie_len);	

VOID CFG80211DRV_P2pClientKeyAdd(
	VOID						*pAdOrg,
	VOID						*pData);
	
VOID RemainOnChannelTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3);
	
VOID CFG80211_UpdateBeacon(
        VOID                                            *pAdOrg,
        UCHAR                                                                               *beacon_head_buf,
        UINT32                                                                                  beacon_head_len,
        UCHAR                                                                               *beacon_tail_buf,
        UINT32                                                                                  beacon_tail_len,
        BOOLEAN                                                                                 isAllUpdate);

VOID CFG80211_IndicateScanFail(
	IN VOID                                         *pAdCB);
	
BOOLEAN CFG80211DRV_P2pClientConnect(
	VOID						*pAdOrg,
	VOID						*pData);
	

BOOLEAN CFG80211DRV_PrintFrameType(
        IN  VOID                                        *pAdOrg,
		IN	PUCHAR										 preStr,
		IN	PUCHAR										 pData,
		IN	UINT                              			 Length);

BOOLEAN CFG80211_checkScanTable(
        IN VOID                                         *pAdCB);

VOID CFG80211_P2pClientSendNullFrame(
    IN VOID                                     *pAdCB,
    IN INT                                       PwrMgmt);

BOOLEAN CFG80211_insertScanTableFromKernelCache(
        IN VOID                                         *pAdCB,
        IN UCHAR                                        *pBSSID);

BOOLEAN CFG80211_checkScanResInKernelCache(
        IN VOID                                         *pAdCB,
        IN UCHAR                                        *pBSSID,
        IN UCHAR                                        *pSsid,
        IN INT                                          ssidLen);

VOID CFG80211_UpdateAssocRespExtraIe(
        VOID                                            *pAdOrg,
        UCHAR                                           *assocresp_ies,
        UINT32                                          assocresp_ies_len);

#endif /* RT_CFG80211_SUPPORT */

/* End of cfg80211extr.h */
