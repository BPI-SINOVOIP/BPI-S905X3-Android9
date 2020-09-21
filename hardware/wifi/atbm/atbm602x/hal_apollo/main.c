/*
 * mac80211 glue code for mac80211 altobeam APOLLO drivers
 * *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 * Based on apollo code
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * Based on:
 * Copyright (c) 2006, Michael Wu <flamingice@sourmilk.net>
 * Copyright (c) 2007-2009, Christian Lamparter <chunkeey@web.de>
 * Copyright 2008, Johannes Berg <johannes@sipsolutions.net>
 *
 * Based on:
 * - the islsm (softmac prism54) driver, which is:
 *   Copyright 2004-2006 Jean-Baptiste Note <jbnote@gmail.com>, et al.
 * - stlc45xx driver
 *   Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*Linux version 3.4.0 compilation*/
//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0))
#include<linux/module.h>
//#endif
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/etherdevice.h>
#include <linux/vmalloc.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <net/atbm_mac80211.h>
#include <linux/debugfs.h>

#include "apollo.h"
#include "txrx.h"
#include "sbus.h"
#include "fwio.h"
#include "hwio.h"
#include "bh.h"
#include "sta.h"
#include "ap.h"
#include "scan.h"
#include "debug.h"
#include "pm.h"
#ifdef ATBM_SUPPORT_SMARTCONFIG
#include "smartconfig.h"
#endif
#include "svn_version.h"

MODULE_AUTHOR("wifi_software <wifi_software@altobeam.com>");
MODULE_DESCRIPTION("Softmac altobeam apollo wifi common code");
MODULE_LICENSE("GPL");
MODULE_ALIAS("atbm_core");

static int ampdu = 0xff;
module_param(ampdu, int, 0644);
static int sgi = 1;
module_param(sgi, int, 0644);
int start_choff = 0xff;
module_param(start_choff, int, 0644);
int start_cca_check = 0;
module_param(start_cca_check, int, 0644);
static int stbc = 1;
module_param(stbc, int, 0644);
static int efuse = 0;
module_param(efuse, int, 0644);
static int driver_ver = DRIVER_VER;
module_param(driver_ver, int, 0644);
static int fw_ver = 0;
module_param(fw_ver, int, 0644);
/* Accept MAC address of the form macaddr=0x00,0x80,0xE1,0x30,0x40,0x50 */
static u8 atbm_mac_default[ATBM_WIFI_MAX_VIFS][ETH_ALEN] = {
	{0x00, 0x12, 0x34, 0x00, 0x00, 0x00},
	{0x00, 0x12, 0x34, 0x00, 0x00, 0x01},
};
module_param_array_named(macaddr, atbm_mac_default[0], byte, NULL, S_IRUGO);
module_param_array_named(macaddr2, atbm_mac_default[1], byte, NULL, S_IRUGO);
MODULE_PARM_DESC(macaddr, "First MAC address");
MODULE_PARM_DESC(macaddr2, "Second MAC address");
#define verson_str(_ver) #_ver
#define verson(ver) verson_str(ver)
const char* hmac_ver = "HMAC_VER:"verson(DRIVER_VER);
#ifdef CUSTOM_FEATURE_MAC /* To use macaddr and PS Mode of customers */
#define PATH_WIFI_MACADDR	"/data/.mac.info"
#define PATH_WIFI_PSM_INFO		"/data/.psm.info"
#define PATH_WIFI_EFUSE	    "/data/.efuse.info"
int access_file(char *path, char *buffer, int size, int isRead);
#ifdef CUSTOM_FEATURE_PSM/* To control ps mode */
static int savedpsm = 0;
#endif
#endif

#define DEFAULT_CCA_INTERVAL_MS		(5000)
#define DEFAULT_CCA_INTERVAL_US		(DEFAULT_CCA_INTERVAL_MS*1000)
#define DEFAULT_CCA_UTIL_US		(50) //cca register util : 50us
/* TODO: use rates and channels from the device */
#define RATETAB_ENT(_rate, _rateid, _flags)		\
	{						\
		.bitrate	= (_rate),		\
		.hw_value	= (_rateid),		\
		.flags		= (_flags),		\
	}

static struct ieee80211_rate atbm_rates[] = {
	RATETAB_ENT(10,  0,   0),
	RATETAB_ENT(20,  1,   0),
	RATETAB_ENT(55,  2,   0),
	RATETAB_ENT(110, 3,   0),
	RATETAB_ENT(60,  6,  0),
	RATETAB_ENT(90,  7,  0),
	RATETAB_ENT(120, 8,  0),
	RATETAB_ENT(180, 9,  0),
	RATETAB_ENT(240, 10, 0),
	RATETAB_ENT(360, 11, 0),
	RATETAB_ENT(480, 12, 0),
	RATETAB_ENT(540, 13, 0),
};

static struct ieee80211_rate atbm_mcs_rates[] = {
	RATETAB_ENT(65,  14, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(130, 15, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(195, 16, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(260, 17, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(390, 18, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(520, 19, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(585, 20, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(650, 21, IEEE80211_TX_RC_MCS),
	RATETAB_ENT(60 , 22, IEEE80211_TX_RC_MCS),
};

#define atbm_a_rates		(atbm_rates + 4)
#define atbm_a_rates_size	(ARRAY_SIZE(atbm_rates) - 4)
#define atbm_g_rates		(atbm_rates + 0)
#define atbm_g_rates_size	(ARRAY_SIZE(atbm_rates))
#define atbm_n_rates		(atbm_mcs_rates)
#define atbm_n_rates_size	(ARRAY_SIZE(atbm_mcs_rates))


#define CHAN2G(_channel, _freq, _flags) {			\
	.band			= IEEE80211_BAND_2GHZ,		\
	.center_freq		= (_freq),			\
	.hw_value		= (_channel),			\
	.flags			= (_flags),			\
	.max_antenna_gain	= 0,				\
	.max_power		= 30,				\
}

#define CHAN5G(_channel, _flags) {				\
	.band			= IEEE80211_BAND_5GHZ,		\
	.center_freq	= 5000 + (5 * (_channel)),		\
	.hw_value		= (_channel),			\
	.flags			= (_flags),			\
	.max_antenna_gain	= 0,				\
	.max_power		= 30,				\
}

static struct ieee80211_channel atbm_2ghz_chantable[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

#ifdef CONFIG_ATBM_APOLLO_5GHZ_SUPPORT
static struct ieee80211_channel atbm_5ghz_chantable[] = {
	CHAN5G(34, 0),		CHAN5G(36, 0),
	CHAN5G(38, 0),		CHAN5G(40, 0),
	CHAN5G(42, 0),		CHAN5G(44, 0),
	CHAN5G(46, 0),		CHAN5G(48, 0),
	CHAN5G(52, 0),		CHAN5G(56, 0),
	CHAN5G(60, 0),		CHAN5G(64, 0),
	CHAN5G(100, 0),		CHAN5G(104, 0),
	CHAN5G(108, 0),		CHAN5G(112, 0),
	CHAN5G(116, 0),		CHAN5G(120, 0),
	CHAN5G(124, 0),		CHAN5G(128, 0),
	CHAN5G(132, 0),		CHAN5G(136, 0),
	CHAN5G(140, 0),		CHAN5G(149, 0),
	CHAN5G(153, 0),		CHAN5G(157, 0),
	CHAN5G(161, 0),		CHAN5G(165, 0),
	CHAN5G(184, 0),		CHAN5G(188, 0),
	CHAN5G(192, 0),		CHAN5G(196, 0),
	CHAN5G(200, 0),		CHAN5G(204, 0),
	CHAN5G(208, 0),		CHAN5G(212, 0),
	CHAN5G(216, 0),
};
#endif /* CONFIG_ATBM_APOLLO_5GHZ_SUPPORT */
#ifdef ATBM_NOT_SUPPORT_40M_CHW
#pragma message("ATBM601x:not support 40M chw") 
#else
#pragma message("ATBM602x:support 40M chw")
#endif
static struct ieee80211_supported_band atbm_band_2ghz = {
	.channels = atbm_2ghz_chantable,
	.n_channels = ARRAY_SIZE(atbm_2ghz_chantable),
	.bitrates = atbm_g_rates,
	.n_bitrates = atbm_g_rates_size,
	.ht_cap = {
		.cap = IEEE80211_HT_CAP_GRN_FLD
				|
#ifndef ATBM_NOT_SUPPORT_40M_CHW
#ifdef ATBM_SUPPORT_WIDTH_40M
//#if 0
			IEEE80211_HT_CAP_SUP_WIDTH_20_40
				|
			IEEE80211_HT_CAP_DSSSCCK40
				|
#endif
#endif
#ifdef CONFIG_ATBM_APOLLO_SUPPORT_SGI
			IEEE80211_HT_CAP_SGI_20 |
#ifndef ATBM_NOT_SUPPORT_40M_CHW
#ifdef ATBM_SUPPORT_WIDTH_40M
//#if 0
			IEEE80211_HT_CAP_SGI_40
				|
#endif
#endif
#endif /* ATBM_APOLLO_SUPPORT_SGI */
			(1 << IEEE80211_HT_CAP_RX_STBC_SHIFT)
		,
		.ht_supported = 1,
		.ampdu_factor = IEEE80211_HT_MAX_AMPDU_32K,
		.ampdu_density = IEEE80211_HT_MPDU_DENSITY_NONE,
		.mcs = {
			.rx_mask[0] = 0xFF,
			.rx_highest = __cpu_to_le16(0),
			.tx_params = IEEE80211_HT_MCS_TX_DEFINED,
		},
	},
};

#ifdef CONFIG_ATBM_APOLLO_5GHZ_SUPPORT
static struct ieee80211_supported_band atbm_band_5ghz = {
	.channels = atbm_5ghz_chantable,
	.n_channels = ARRAY_SIZE(atbm_5ghz_chantable),
	.bitrates = atbm_a_rates,
	.n_bitrates = atbm_a_rates_size,
	.ht_cap = {
		.cap = IEEE80211_HT_CAP_GRN_FLD |
			(1 << IEEE80211_HT_CAP_RX_STBC_SHIFT),
		.ht_supported = 1,
		.ampdu_factor = IEEE80211_HT_MAX_AMPDU_8K,
		.ampdu_density = IEEE80211_HT_MPDU_DENSITY_NONE,
		.mcs = {
			.rx_mask[0] = 0xFF,
			.rx_highest = __cpu_to_le16(0),
			.tx_params = IEEE80211_HT_MCS_TX_DEFINED,
		},
	},
};
#endif /* CONFIG_ATBM_APOLLO_5GHZ_SUPPORT */

static const unsigned long atbm_ttl[] = {
	1 * HZ,	/* VO */
	2 * HZ,	/* VI */
	5 * HZ, /* BE */
	10 * HZ	/* BK */
};
void atbm_set_fw_ver(struct atbm_common *hw_priv)
{
	fw_ver = hw_priv->wsm_caps.firmwareVersion;
}
#ifdef ATBM_SUPPORT_WIDTH_40M
#define CHW_IS_40M			(1)
#define CHW_IS_20M			(0)
#define CHW_ACTION_TO_PEER(chw,action)				\
	do												\
	{												\
		if(atomic_read(&hw_priv->phy_chantype) == (!!!chw))	\
		{												\
			wsm_lock_tx_async(hw_priv);						\
			action = (notifyPeerAction_t)chw;				\
			printk("%s:%s----->%s\n",__func__,(chw?"20M":"40M"),(chw?"40M":"20M"));	\
			atomic_set(&hw_priv->phy_chantype,chw);								\
			wsm_unlock_tx(hw_priv);												\
		}																		\
	}while(0)
int atbm_req_lmc_to_send_action(struct atbm_vif *priv,notifyPeerAction_t is40M)
{
	struct wsm_req_chtype_change req_chtype_chg;
	static u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	if(priv->join_status<=ATBM_APOLLO_JOIN_STATUS_MONITOR)
	{
		return -1;
	}
	if(is40M>=notifyPeerMax)
	{
		return -1;
	}
	if(priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
	{
		memcpy(&req_chtype_chg.MacAddr[0],&broadcast_addr[0],6);
	}
	else if(priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA)
	{
		memcpy(&req_chtype_chg.MacAddr[0],priv->vif->bss_conf.bssid,6);
	}
	else
	{
		return -1;
	}
	printk("%s: join_status=%d, addr =%pM,is40M = %d\n",
			__func__,priv->join_status , &req_chtype_chg.MacAddr[0],is40M);
	if(is40M == notifyPeer20M)
		req_chtype_chg.flags = WSM_SEND_CHTYPE_CHG_REQUEST__FLAGS_SEN_20M;
	else if(is40M == notifyPeer40M)
		req_chtype_chg.flags = WSM_SEND_CHTYPE_CHG_REQUEST__FLAGS_SEN_40M;

	return wsm_req_chtype_change_func(priv->hw_priv,&req_chtype_chg,priv->if_id);
}
void atbm_start_detect_cca(struct atbm_vif *priv,u32 rx_phy_enable_num_req)
{
	#define CCA_INTERVAL_S		100
	struct wsm_get_cca_req get_cca;
	struct wsm_get_cca_resp res_cca;
	struct atbm_common *hw_priv = priv->hw_priv;
	enum nl80211_channel_type temp_channel_type = NL80211_CHAN_NO_HT;
	if(priv == NULL)
	{
		return;
	}
	if(rx_phy_enable_num_req<CCA_INTERVAL_S)
	{
		rx_phy_enable_num_req = CCA_INTERVAL_S;
	}
	spin_lock_bh(&hw_priv->spinlock_channel_type);
	temp_channel_type = hw_priv->channel_type; 
	spin_unlock_bh(&hw_priv->spinlock_channel_type);
	if(temp_channel_type<=NL80211_CHAN_HT20)
	{
		return;
	}
	memset(&get_cca,0,sizeof(struct wsm_get_cca_req));
	memset(&res_cca,0,sizeof(struct wsm_get_cca_resp));

	get_cca.rx_phy_enable_num_req = rx_phy_enable_num_req;
	get_cca.flags |= WSM_GET_CCA_FLAGS__SET_INIT;

	if(wsm_get_cca(hw_priv,&get_cca,&res_cca,0))
	{
		printk("%s:atbm_get_cca_work err\n",__func__);
	}
}
void atbm_channel_type_change_work(struct work_struct *work)
{
	struct atbm_vif *priv =  container_of(work, struct atbm_vif, chantype_change_work.work);
	struct atbm_common *hw_priv = priv->hw_priv;
	struct wsm_set_chantype set_channtype;
	enum nl80211_channel_type temp_channel_type;
	notifyPeerAction_t action=notifyPeerMax;
	if(atbm_bh_is_term(hw_priv)){
//		wsm_unlock_tx(hw_priv);
		return;
	}
	if(priv == NULL)
	{
//		wsm_unlock_tx(hw_priv);
		printk(KERN_ERR "%s:priv == NULL\n",__func__);
		goto type_change_work_err;
	}
	if(!atomic_read(&hw_priv->channel_chaging))
	{
//		wsm_unlock_tx(hw_priv);
		printk(KERN_ERR "%s:what happend.only if_id 0 can change channel type so return\n",__func__);
		goto type_change_work_err;
	}
	if(priv->join_status == ATBM_APOLLO_JOIN_STATUS_PASSIVE)
	{
//		wsm_unlock_tx(hw_priv);
		printk(KERN_ERR "%s:priv->join_status err\n",__func__);
		goto type_change_work_err;
	}
	printk(KERN_DEBUG "atbm_channel_type_change_work in+++++\n");
	if (unlikely(down_trylock(&hw_priv->scan.lock))) {
		/* Scan is already in progress. Requeue self. */
		//schedule();
		printk(KERN_ERR "%s: scan locked so requeue the work\n",__func__);
		if(atbm_hw_priv_queue_delayed_work(hw_priv, &priv->chantype_change_work,HZ/3) <= 0)
		{
			printk(KERN_ERR "%s: scan locked so requeue the work err\n",__func__);
//			wsm_unlock_tx(hw_priv);
		}
		return;
	}
	wsm_lock_tx_async(hw_priv);
	mutex_lock(&hw_priv->conf_mutex);
	mutex_lock(&hw_priv->chantype_mutex);
	
	spin_lock_bh(&hw_priv->spinlock_channel_type);
	temp_channel_type = hw_priv->channel_type; 
	spin_unlock_bh(&hw_priv->spinlock_channel_type);
	/*
	*only when change bit has been set ,then send cmd to lmc.
	*triger this work queue only when we recive action frame(Note:Spec 8.5.2.5 and 8.5.8.7,and special beacon)
	*/
	if(
		test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANNUM_CHANGE,&hw_priv->change_bit)
		||
		test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANTYPE_CHANGE,&hw_priv->change_bit)
	  )
	{
		/*
		*waitting the frame in the lmac  has been send successful,then we
		*can change channel ,channel type, or both. after set channel completed,we
		*must UNLOCK TX.
		*/
		//wsm_vif_lock_tx(priv);
		wsm_flush_tx(hw_priv);

		set_channtype.band = (hw_priv->channel->band == IEEE80211_BAND_5GHZ) ?
							 WSM_PHY_BAND_5G : WSM_PHY_BAND_2_4G;
		set_channtype.flag = 0;
		set_channtype.channelNumber = hw_priv->channel->hw_value;
		set_channtype.channelType = (u32)temp_channel_type;
		printk(KERN_ERR "%s:chatype(%d),channelNumber(%d)\n",__func__,
			set_channtype.channelType,set_channtype.channelNumber);
		clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANNUM_CHANGE,&hw_priv->change_bit);
		clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANTYPE_CHANGE,&hw_priv->change_bit);		
		if(wsm_set_chantype_func(hw_priv,&set_channtype,priv->if_id))
		{
			printk(KERN_DEBUG "%s:wsm_set_chantype_func err\n",__func__);
			/*
			*if we set channel type err to lmc,send 20 M is safe
			*/
			printk(KERN_DEBUG "%s:set channeltype err",__func__);
			atomic_set(&hw_priv->phy_chantype,0);
			goto type_change_work_exit;
		}
		printk("%s  %d\n", __FUNCTION__, __LINE__);
		if((temp_channel_type == NL80211_CHAN_HT40MINUS) || (temp_channel_type == NL80211_CHAN_HT40PLUS))
		{
			atomic_set(&hw_priv->tx_20M_lock,0);
			printk("%s  %d\n", __FUNCTION__, __LINE__);
		}
		else
		{
			atomic_set(&hw_priv->tx_20M_lock,1);
			printk("%s  %d\n", __FUNCTION__, __LINE__);
		}
	}
	/*
	*tx_20M_lock:must send 20M.
	*when we recieve an action frame(Note:Spec8.5.12.2),tell we 
	*must send 20M ,then set this bit.
	*/
	if(
		((temp_channel_type == NL80211_CHAN_HT40MINUS)
		||
		(temp_channel_type == NL80211_CHAN_HT40PLUS))
		&&
		(atomic_read(&hw_priv->tx_20M_lock) == 0)
	  )
	{
		if(!test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit))
		{
			if((atomic_read(&hw_priv->cca_detect_running) == 0)&&(start_cca_check == 1))
			{
				printk(KERN_DEBUG "%s:start get cca\n",__func__);
				atbm_start_detect_cca(priv,hw_priv->rx_phy_enable_num_req);
				atomic_set(&hw_priv->cca_detect_running,1);
				mod_timer(&hw_priv->chantype_timer,
				jiffies + (atomic_read(&hw_priv->cca_interval_ms)/1000)*HZ+HZ/1000);
			}
			printk(KERN_DEBUG "%s:we can send 40M later\n",__func__);
			/*
			*at here if hw_priv->phy_chantype is zero,we send action to
			*ap to notify that we only can receive 20M
			*/
			atomic_set(&hw_priv->phy_chantype,1);
		}
		else
		{
			action = notifyPeer40M;
			atomic_set(&hw_priv->phy_chantype,1);
			del_timer_sync(&hw_priv->chantype_timer);
			atomic_set(&hw_priv->cca_detect_running,0);
		}
	}
	else
	{
		if(atomic_read(&hw_priv->tx_20M_lock) == 1)
		{			
			del_timer_sync(&hw_priv->chantype_timer);
			atomic_set(&hw_priv->cca_detect_running,0);
			
			if(temp_channel_type>=NL80211_CHAN_HT40MINUS)
				CHW_ACTION_TO_PEER(CHW_IS_20M,action);
			
			if(test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit))
			{
				action = notifyPeer20M;
			}

			if(test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANGE_PHY_CHTYPE,&hw_priv->change_bit))
			{
				action = notifyPeerMax;
				clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANGE_PHY_CHTYPE,&hw_priv->change_bit);
			}
		}
	}
	if((action<notifyPeerMax)&&(start_cca_check == 1))
	{
		printk(KERN_ERR "%s:atbm_req_lmc_to_send_action %d\n",__func__,action);
		atbm_req_lmc_to_send_action(priv,action);
	}
type_change_work_exit:
	/*
	*wsm_unlock_tx enable tx.
	*/
	wsm_unlock_tx(hw_priv);
	if(atomic_sub_return(1, &hw_priv->channel_chaging) < 0)
			atomic_set(&hw_priv->channel_chaging, 0);
	mutex_unlock(&hw_priv->chantype_mutex);	
	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
	printk(KERN_DEBUG "atbm_channel_type_change_work out ------\n");
	return;
type_change_work_err:
	mutex_lock(&hw_priv->conf_mutex);
	mutex_lock(&hw_priv->chantype_mutex);
	clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANNUM_CHANGE,&hw_priv->change_bit);
	clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANTYPE_CHANGE,&hw_priv->change_bit);		
	clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANGE_PHY_CHTYPE,&hw_priv->change_bit);
	if(atomic_sub_return(1, &hw_priv->channel_chaging) < 0)
			atomic_set(&hw_priv->channel_chaging, 0);
	mutex_unlock(&hw_priv->chantype_mutex);	
	mutex_unlock(&hw_priv->conf_mutex);
	return;

}
static void atbm_chantype_timer(unsigned long arg)
{	
	struct atbm_common *hw_priv =
			(struct atbm_common *)arg;
	int can_get = 0;

	if(atbm_bh_is_term(hw_priv))
	{
		return;
	}
	spin_lock_bh(&hw_priv->spinlock_channel_type);
	can_get = hw_priv->channel_type>=NL80211_CHAN_HT40MINUS? 1:0;
	spin_unlock_bh(&hw_priv->spinlock_channel_type);
	
	if(atomic_read(&hw_priv->tx_20M_lock))
	{
		can_get = 0;
		printk("%s:tx_20M_lock is locked,we cannot get cca\n",__func__);
	}
	
	if(can_get == 0)
	{
		printk("%s:channel_type(%d),tx_20M_lock(%d)\n",__func__, hw_priv->channel_type,atomic_read(&hw_priv->tx_20M_lock));
		return;	
	}
	//printk("%s:can_get(%d)\n",__func__,can_get);
	if(start_cca_check == 1)
	{
		if(atbm_hw_priv_queue_work(hw_priv, &hw_priv->get_cca_work) <= 0)
		{
			printk("%s:cmd get cca err",__func__);
		}
	}

}
void atbm_get_cca_work(struct work_struct *work)
{
#define CCA_CHANGE_TO_40M_a  atomic_read(&hw_priv->chw_sw_40M_level)
#define CCA_CHANGE_TO_20M_a  atomic_read(&hw_priv->chw_sw_20M_level)
	struct atbm_common *hw_priv = container_of(work, struct atbm_common, get_cca_work);
	u8 can_get = 0;
	u32	flags = 0;
	struct wsm_get_cca_req get_cca;
	struct wsm_get_cca_resp res_cca;
	//u32 prev_cca;
	notifyPeerAction_t action=notifyPeerMax;
	struct atbm_vif * priv = __ABwifi_hwpriv_to_vifpriv(hw_priv,0);

	if((priv == NULL) || atbm_bh_is_term(hw_priv))
	{
		printk("%s:priv == NULL\n",__func__);
		goto cca_work_end;
	}
	
	mutex_lock(&hw_priv->chantype_mutex);
	spin_lock_bh(&hw_priv->spinlock_channel_type);
	can_get = hw_priv->channel_type>=NL80211_CHAN_HT40MINUS? 1:0;
	spin_unlock_bh(&hw_priv->spinlock_channel_type);
	if(atomic_read(&hw_priv->tx_20M_lock))	{
		can_get = 0;
		printk("%s:tx_20M_lock is locked,we cannot get cca\n",__func__);
	}
	/*
	*when the work pennding,hw_priv->channel_type may be changed;
	*/
	if(can_get == 0)	{
		flags |= WSM_GET_CCA_FLAGS__STOP_CCA;
	}
	else	{
		flags |= WSM_GET_CCA_FLAGS__START_CCA;

		if(test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CCA_LEVEL_CHANGE,&hw_priv->change_bit))	{
			printk("%s:cca_interval_change\n",__func__);
			clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CCA_LEVEL_CHANGE,&hw_priv->change_bit);
			//atbm_start_detect_cca(priv,atomic_read(&hw_priv->cca_interval_ms));
		}
	}
	get_cca.rx_phy_enable_num_req = hw_priv->rx_phy_enable_num_req;
	get_cca.flags = flags;
	if(wsm_get_cca(hw_priv,&get_cca,&res_cca,0))	{
		printk("%s:atbm_get_cca_work err\n",__func__);
		goto cca_work_end;
	}
	
	if(res_cca.status != WSM_STATUS_SUCCESS)	{
		printk("%s:res_cca.status err (%d)\n",__func__,res_cca.status);
		goto cca_work_end;
	}
	else{
		
		if(res_cca.rx_phy_enable_num_cnf < get_cca.rx_phy_enable_num_req){
			printk("%s:getcca not good rx_phy_enable_num_cnf (%d) < (%d)\n",__func__,res_cca.rx_phy_enable_num_cnf , get_cca.rx_phy_enable_num_req);
			goto cca_work_end;
		}
	}
	if(can_get == 1)	{

		//·ÀÖ¹Ô½½ç
		if(res_cca.pri_channel_idle_cnt > 0xfffff){
			res_cca.pri_snd_channel_idle_cnt = res_cca.pri_snd_channel_idle_cnt>>8; 
			res_cca.pri_channel_idle_cnt = res_cca.pri_channel_idle_cnt>>8; 
		}
		if((res_cca.pri_snd_channel_idle_cnt<<8) > (res_cca.pri_channel_idle_cnt*CCA_CHANGE_TO_40M_a)) {
			if(atomic_read(&hw_priv->tx_20M_lock) == 0)			{		
				CHW_ACTION_TO_PEER(CHW_IS_40M,action);
			}
			else			{
				CHW_ACTION_TO_PEER(CHW_IS_20M,action);
			}
		}
		else if((res_cca.pri_snd_channel_idle_cnt<<8) <( res_cca.pri_channel_idle_cnt*CCA_CHANGE_TO_20M_a))
		{
			CHW_ACTION_TO_PEER(CHW_IS_20M,action);
		
		}
	}

	if((action<notifyPeerMax)&&(start_cca_check == 1))
	{
		printk("%s:atbm_req_lmc_to_send_action %d\n",__func__,action);
		atbm_req_lmc_to_send_action(priv,action);
	}
cca_work_end:
	/*
	*restart timer
	*/
	if(can_get)
	{
		mod_timer(&hw_priv->chantype_timer,
		jiffies + (atomic_read(&hw_priv->cca_interval_ms)/1000)*HZ+HZ/1000);
	//	printk("%s:start chantype_timer\n",__func__);
	}
	else
	{
		printk("%s:stop chantype_timer\n",__func__);
	}
	mutex_unlock(&hw_priv->chantype_mutex);	
}
void atbm_clear_wpas_p2p_40M_ie(struct atbm_ieee80211_mgmt *mgmt,u32 pkg_len)
{
	u8 *ie = NULL;
	int ie_len = 0;
	struct ieee80211_ht_cap *htcap = NULL;
	u8 *htcap_ie = NULL;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0))
	typedef int (*mgmt_filter_handle)(__le16 fc);
	#else
	typedef bool (*mgmt_filter_handle)(__le16 fc);
	#endif
	struct mgmt_filter
	{
		mgmt_filter_handle	handle;
		u8*				variable;
	};

	struct mgmt_filter atbm_mgmt_filter_table[]=
	{
		{.handle = ieee80211_is_beacon,.variable=mgmt->u.beacon.variable,},
		{.handle = ieee80211_is_probe_req,.variable=mgmt->u.probe_req.variable,},
		{.handle = ieee80211_is_probe_resp,.variable=mgmt->u.probe_resp.variable,},
		{.handle = ieee80211_is_assoc_req,.variable= mgmt->u.assoc_req.variable,},
		{.handle = ieee80211_is_reassoc_req,.variable=mgmt->u.reassoc_req.variable,},
		{.handle = ieee80211_is_assoc_resp,.variable= mgmt->u.assoc_resp.variable,},
		{.handle = ieee80211_is_reassoc_resp,.variable=mgmt->u.assoc_resp.variable,},
		{.handle = NULL,.variable=NULL,},
	};
	u16 clear_bit = IEEE80211_HT_CAP_SUP_WIDTH_20_40 | 
		           IEEE80211_HT_CAP_DSSSCCK40 |
		           IEEE80211_HT_CAP_SGI_40;
	u8 index = 0;
	if((mgmt == NULL) || (pkg_len == 0))
	{
		printk(KERN_DEBUG "%s->err:mgmt(%p),pkg_len(%d)\n",__func__,mgmt,pkg_len);
		return;
	}
	
	for(index=0;atbm_mgmt_filter_table[index].handle != NULL;index++)
	{
		if(!atbm_mgmt_filter_table[index].handle(mgmt->frame_control))
		{
			if(atbm_mgmt_filter_table[index+1].handle == NULL)
			{
				printk(KERN_DEBUG "%s:is another mgmt\n",__func__);
			}
			continue;
		}

		ie =atbm_mgmt_filter_table[index].variable;
		printk(KERN_DEBUG "%s:filter index(%d)\n",__func__,index);
		break;
	}
	if(ie == NULL)
	{
		printk(KERN_DEBUG "%s:ie == NULL\n",__func__);
		return;
	}
	ie_len = pkg_len-(ie - (u8*)mgmt);
	if(ie_len <= 0)
	{
		return;
	}

	htcap_ie = (u8 *)cfg80211_find_ie( ATBM_WLAN_EID_HT_CAPABILITY, ie, ie_len);

	if(htcap_ie == NULL)
	{
		printk(KERN_DEBUG "%s:htcap_ie == NULL,len(%d),pkg_len(%d)\n",__func__,ie_len,pkg_len);
		return;
	}

	if(htcap_ie[1] < sizeof(struct ieee80211_ht_cap) )
	{
		printk(KERN_DEBUG "%s:ie len is err len(%d),pkg_len(%d)\n",__func__,htcap_ie[1],pkg_len);
		return;
	}
	htcap = (struct ieee80211_ht_cap *)(htcap_ie+2);
	htcap->cap_info = htcap->cap_info &(~clear_bit);
	
}
#endif
#ifdef ATBM_PKG_REORDER
#define TID_IS_SAFE(tid_index,action)	if((tid_index)>ATBM_RX_DATA_QUEUES)	action
#define SEQ_IS_SAFE(seq_index,action) if((seq_index)>=BUFF_STORED_LEN)	action
#define reorder_debug(debug_en,...)  if(debug_en)	printk(KERN_DEBUG __VA_ARGS__)
#define REORDER_DEBUG		(1)
#define THE_RETRY_PKG_INEDX	(0x800)
#define BUFF_INDEX_IS_SAFE(index)	((index)&(BUFF_STORED_LEN-1))
#define DEUG_SPINLOCK		(0)
#if DEUG_SPINLOCK
#define spinlock_debug(type)		printk("%s:tid_params_%s\n",__func__,#type)
#else
#define spinlock_debug(type)
#endif
#define tid_params_spin_lock(lock,type) \
	do									\
	{									\
		spinlock_debug(type);			\
		type(lock);						\
	}while(0)
#define tid_params_spin_unlock(lock,type)		\
	do										\
	{										\
		spinlock_debug(type);				\
		type(lock);							\
	}while(0)
static struct atbm_ba_tid_params *atbm_get_tid_params(struct atbm_reorder_queue_comm * atbm_reorder,u8 tid)
{
	struct atbm_ba_tid_params *tid_params = NULL;
	
	if(tid>=ATBM_RX_DATA_QUEUES)
	{
		return NULL;
	}

	mutex_lock(&atbm_reorder->reorder_mutex);
	tid_params = &atbm_reorder->atbm_rx_tid[tid];
	mutex_unlock(&atbm_reorder->reorder_mutex);

	return tid_params;
}
static void atbm_skb_buff_queue(struct atbm_ba_tid_params *tid_params,u8 index,struct sk_buff *skb)
{
	u8 start_index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	tid_params->skb_reorder_buff[index] = skb;

	//set time to the oldest time
	while(tid_params->frame_rx_time[index]==0){
		tid_params->frame_rx_time[index] = jiffies;
		//reorder_debug(REORDER_DEBUG,"enqueue index %x\n",index);
		if(start_index == index)
			break;
		index =BUFF_INDEX_IS_SAFE(index-1);
	}

	tid_params->skb_buffed++; 
	WARN_ON(tid_params->wind_size<tid_params->skb_buffed);
 	//__atbm_skb_queue_tail(&tid_params->header, skb);
}
static void atbm_skb_buff_dequeue(struct atbm_common *hw_priv,struct atbm_ba_tid_params *tid_params,u8 index,bool uplayer)
{
	struct sk_buff *skb;
	 
	skb = tid_params->skb_reorder_buff[index];
	//reorder_debug(REORDER_DEBUG,"dequeue index %x\n",index);
	if(skb == NULL){
		tid_params->frame_rx_time[index] = 0;
	}
	else {
		WARN_ON(tid_params->skb_buffed ==0);
		WARN_ON(tid_params->wind_size<tid_params->skb_buffed);
		tid_params->skb_buffed--;
		tid_params->frame_rx_time[index] = 0;
		tid_params->skb_reorder_buff[index] = NULL; 
		
		if(uplayer==NULL){
			ieee80211_rx_irqsafe(hw_priv->hw, skb);
		}
		else {			
			atbm_kfree_skb(skb);
		}
	}
	//__atbm_skb_dequeue(&tid_params->header)
}

int atbm_reorder_skb_forcedrop(struct atbm_vif *priv,struct atbm_ba_tid_params *tid_params,int need_free)
{
	u8 i = 0;
	u8 seq = 0;	

	//reorder_debug(REORDER_DEBUG,"%s:has_buffed(%d)\n",__func__,tid_params->skb_buffed);
	seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	for(i=0;i<need_free;i++)
	{
		atbm_skb_buff_dequeue(priv->hw_priv,tid_params,seq,1);
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	}		

	return 0;
}
int atbm_reorder_skb_forcefree(struct atbm_vif *priv,struct atbm_ba_tid_params *tid_params,int need_free)
{
	u8 i = 0;
	u8 seq = 0;	

	//reorder_debug(REORDER_DEBUG,"%s:has_buffed(%d)\n",__func__,tid_params->skb_buffed);
	seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	for(i=0;i<need_free;i++)
	{
		atbm_skb_buff_dequeue(priv->hw_priv,tid_params,seq,NULL);
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	}		

	return 0;
}

int atbm_reorder_skb_uplayer(struct atbm_vif *priv,struct atbm_ba_tid_params *tid_params)
{
	u16 seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);

	while(tid_params->skb_buffed)
	{
		if(tid_params->skb_reorder_buff[seq] == NULL)
		{
			//reorder_debug(REORDER_DEBUG,"%s,index(%d)\n",__func__,seq);
			break;
		}
		atbm_skb_buff_dequeue(priv->hw_priv,tid_params,seq,NULL);
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	}

	return 0;
}

static void atbm_reorder_pkg_timeout(unsigned long arg)
{
	struct atbm_ba_tid_params *tid_params= (struct atbm_ba_tid_params *)arg;
	struct atbm_vif *priv = (struct atbm_vif *)tid_params->reorder_priv;
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct sk_buff_head frames;
	u8 i = 0;
	u16 index = 0;
	if(!test_bit(BAR_TID_EN,&tid_params->tid_en))
	{
		return;
	}

	tid_params_spin_lock(&tid_params->skb_reorder_spinlock,spin_lock_bh);
	if(!tid_params->skb_buffed)
		goto exit;
	index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	while(i<tid_params->wind_size) 
	{
		if(tid_params->frame_rx_time[index] ==0)
			break;
		if(time_is_before_eq_jiffies(tid_params->frame_rx_time[index]+(5*HZ/1000))){
			
			reorder_debug(REORDER_DEBUG,"ReOrder:timeout %x>%x sn %x\n",tid_params->frame_rx_time[index],jiffies,tid_params->start_seq);
			atbm_skb_buff_dequeue(hw_priv,tid_params,index,NULL);
		}
		else {
			break;
		}
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
		i++;			
	}
	atbm_reorder_skb_uplayer(priv,tid_params);
	index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
 	if(tid_params->skb_buffed){
		mod_timer(&tid_params->overtime_timer,
				tid_params->frame_rx_time[index] + (tid_params->timeout*HZ)/1000);
 	}
	else {
	 	clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
	}
exit:
	tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,spin_unlock_bh);
}




int atbm_reorder_skb_queue(struct atbm_vif *priv,struct sk_buff *skb,u8 link_id)
{
	u16 index = 0;
	int res = 0;
	struct atbm_reorder_queue_comm * atbm_reorder = NULL;
	struct atbm_ba_tid_params *tid_params;
	struct ieee80211_hdr *frame = (struct ieee80211_hdr *)skb->data;
	u8 frag =  frame->seq_ctrl & IEEE80211_SCTL_FRAG;
	u8 more = ieee80211_has_morefrags(frame->frame_control);
	u8 *qc = ieee80211_get_qos_ctl(frame);
	int tid = 0;
	u16 frame_seq = 0;
	u16 start_seq = 0;
	u16 now_frame_id = 0;
	if(link_id>ATBM_APOLLO_LINK_ID_UNMAPPED)
	{
		WARN_ON(1);
		return 0;
	}
	/*
	*fragment frame cant not been buffedn in 
	*ampdu queue;
	*/
	if(frag|more)
	{
		reorder_debug(REORDER_DEBUG,"is a fregment frames\n");
		return 0;
	}
	tid = *qc & /*IEEE80211_QOS_CTL_TID_MASK(*/7;
	/*
	*if it is a fragment frame,we add it to the buff.
	*/
	atbm_reorder = &priv->atbm_reorder_link_id[link_id];
	tid_params =atbm_get_tid_params(atbm_reorder,tid);
	if(tid_params == NULL)
	{
		reorder_debug(REORDER_DEBUG,"tid_params == NULL");
		return 0;
	}
	if(!test_bit(BAR_TID_EN,&tid_params->tid_en))
	{
		//printk("tid(%d),is not enable\n",tid);
		return 0;
	}
	start_seq = tid_params->start_seq;
	frame_seq = (frame->seq_ctrl>>4)&SEQ_NUM_MASKER;
	now_frame_id = BUFF_INDEX_IS_SAFE(frame_seq);
	
	tid_params_spin_lock(&tid_params->skb_reorder_spinlock,spin_lock_bh);
	index = ((frame_seq)-(tid_params->start_seq))&SEQ_NUM_MASKER;
	if (
		//only hope that frame come,so directly queue
		((index == 0)&&(tid_params->skb_buffed == 0))	
	    )
	{
		tid_params->start_seq =(frame_seq+1)&SEQ_NUM_MASKER;
		res = 0;
		goto exit_reorder;
	}
	/*
	*the frame maybe has been lost,so
	*send to the mac80211.
	*/
	if(index>=THE_RETRY_PKG_INEDX)
	{
		reorder_debug(REORDER_DEBUG,"rx a prev frame,seq(%x),start(%x)\n",(frame->seq_ctrl&(IEEE80211_SCTL_SEQ))>>4,start_seq);
		res = 0;
		goto exit_reorder;
	}
	/*
	*skb buff has been overflowed,maybe same frames has bee buffed a long time,
	*so send some prev frames;
	*/
	if(index >= tid_params->wind_size)
	{
		u8 need_free = index-tid_params->wind_size+1;
		reorder_debug(REORDER_DEBUG,"overflow:need_free(%d),sn(%x)\n",need_free,tid_params->start_seq);
		if(need_free>=tid_params->wind_size)
		{
			reorder_debug(REORDER_DEBUG,"what a fuck seq,queue to mac80211 handles\n");
			res = 0;
			goto exit_reorder;;
		}
		atbm_reorder_skb_forcefree(priv,tid_params,need_free);	
	}
	else {
		/*
		*retry frames or fragment frames 
		*/
		if(tid_params->skb_reorder_buff[now_frame_id] != NULL)
		{
			/*
			*if it is a retry frame,we discare it;
			*/
			res = -1;
			reorder_debug(REORDER_DEBUG,"%s:ieee80211_has_retry\n",__func__);
			goto exit_reorder;
		}
		else
		{
			//reorder_debug(REORDER_DEBUG,"seq(%x),start(%x),index(%d)\n",frame_seq,start_seq,index);
		}
	}
	atbm_skb_buff_queue(tid_params,now_frame_id,skb);
	
maybe_send_order:
	/*
	*dequeue the sequential frames after the the "index" frame;
	*/
	atbm_reorder_skb_uplayer(priv,tid_params);
	
//		reorder_debug(REORDER_DEBUG,"header(%d),ampdu reordered,buffed(%d)---2\n",tid_params->index_hread,tid_params->skb_buffed);
	if((!test_bit(REORDER_TIMER_RUNING,&tid_params->timer_running)||(start_seq!= tid_params->start_seq))&&
		(tid_params->skb_buffed)&&
		(tid_params->timeout))
	{		
		index= BUFF_INDEX_IS_SAFE(tid_params->start_seq);

		//reorder_debug(REORDER_DEBUG,"mod_timer,skb_buffed(%d),start_seq(%x)\n",tid_params->skb_buffed,tid_params->start_seq);
		set_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
		WARN_ON(tid_params->frame_rx_time[index] == 0);
		mod_timer(&tid_params->overtime_timer,
			tid_params->frame_rx_time[index] + (tid_params->timeout*HZ)/1000);
	}
	else if(tid_params->skb_buffed==0)
	{
		clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
		del_timer_sync(&tid_params->overtime_timer);
	}
	res = -1;
exit_reorder:
	tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,spin_unlock_bh);
	return res;
}
void atbm_reorder_func_init(struct atbm_vif *priv)
{
	
	u8 i, j,k;
	struct atbm_reorder_queue_comm * atbm_reorder;
	struct atbm_ba_tid_params *tid_params;
	reorder_debug(REORDER_DEBUG,"%s\n",__func__);
	for(k=0;k<ATBM_APOLLO_LINK_ID_UNMAPPED;k++)
	{
		atbm_reorder = &priv->atbm_reorder_link_id[k];
		for(i=0;i<ATBM_RX_DATA_QUEUES;i++)
		{
			tid_params = &atbm_reorder->atbm_rx_tid[i];
			clear_bit(BAR_TID_EN,&tid_params->tid_en);
			spin_lock_init(&tid_params->skb_reorder_spinlock);
			tid_params->overtime_timer.data = (unsigned long)tid_params;
			tid_params->overtime_timer.function = atbm_reorder_pkg_timeout;
			init_timer(&tid_params->overtime_timer);
			//atbm_reorder->atbm_rx_tid[i].skb_reorder_buff = NULL;
			//atbm_reorder->atbm_rx_tid[i].frame_rx_time = NULL;
		}
		mutex_init(&atbm_reorder->reorder_mutex);
		atbm_reorder->link_id=k;
//		INIT_WORK(&atbm_reorder->reorder_work,atbm_reorder_pkg_work);

	}
	
}
void atbm_reorder_tid_buffed_clear(struct atbm_vif *priv,struct atbm_ba_tid_params *tid_params)
{
	u8 header= 0;
	if((priv == NULL)||(tid_params==NULL))
	{
		return;
	}
	if(!tid_params->tid_en)
	{
		reorder_debug(REORDER_DEBUG,"!tid_params->tid_en\n");
		return;
	}
	
	if(!tid_params->skb_buffed)
		return;

	atbm_reorder_skb_forcefree(priv,tid_params,tid_params->wind_size);
	del_timer_sync(&tid_params->overtime_timer);
	clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
}
void atbm_reorder_tid_reset(struct atbm_vif *priv,struct atbm_ba_tid_params *tid_params)
{
	struct sk_buff *clear_skb;
	if((priv == NULL)||(tid_params==NULL))
	{
		return;
	}
	if(!test_bit(BAR_TID_EN,&tid_params->tid_en))
	{
		reorder_debug(REORDER_DEBUG,"!tid_params->tid_en\n");
		return;
	}
	
	tid_params_spin_lock(&tid_params->skb_reorder_spinlock,spin_lock_bh);
	if(!tid_params->skb_buffed)
		goto exit_tid_reset;
	atbm_reorder_skb_forcedrop(priv,tid_params,tid_params->wind_size);

	tid_params->start_seq = 0;
	del_timer_sync(&tid_params->overtime_timer);
	clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
exit_tid_reset:
	tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,spin_unlock_bh);

}
void atbm_reorder_func_reset(struct atbm_vif *priv,u8 link_id)
{
	u8 link_start, link_end,tid;
	struct atbm_reorder_queue_comm * atbm_reorder;
	if(link_id == 0xff)
	{
		link_start = 0;
		link_end = ATBM_APOLLO_LINK_ID_UNMAPPED;
	}
	else
	{
		if(link_id>=ATBM_APOLLO_LINK_ID_UNMAPPED)
		{
			return;
		}
		link_start = link_id;
		link_end = link_start+1;
	}
	for(;link_start<link_end;link_start++)
	{	
		atbm_reorder =  &priv->atbm_reorder_link_id[link_start];
//		mutex_lock(&atbm_reorder->reorder_mutex);
		for(tid=0;tid<ATBM_RX_DATA_QUEUES;tid++)
		{
			if(!test_bit(BAR_TID_EN,&atbm_reorder->atbm_rx_tid[tid].tid_en))
			{
				continue;
			}
			atbm_reorder_tid_reset(priv,&atbm_reorder->atbm_rx_tid[tid]);
			tid_params_spin_lock(&atbm_reorder->atbm_rx_tid[tid].skb_reorder_spinlock,spin_lock_bh);
			clear_bit(BAR_TID_EN,&atbm_reorder->atbm_rx_tid[tid].tid_en);
			atbm_reorder->atbm_rx_tid[tid].ssn = 0;
			atbm_reorder->atbm_rx_tid[tid].wind_size = 0;
			atbm_reorder->atbm_rx_tid[tid].start_seq = 0;
			tid_params_spin_unlock(&atbm_reorder->atbm_rx_tid[tid].skb_reorder_spinlock,spin_unlock_bh);
		}
//		mutex_unlock(&atbm_reorder->reorder_mutex);
	}
}
void atbm_updata_ba_tid_params(struct atbm_vif *priv,struct atbm_ba_params *ba_params)
{
	u8 tid;
	u8 link_id;
	struct atbm_reorder_queue_comm * atbm_reorder =NULL;
	struct atbm_ba_tid_params *tid_params = NULL;
	u8 action = ba_params->action;
	int i=0;

	if(priv == NULL)
	{
		reorder_debug(REORDER_DEBUG,"err:%s:priv == NULL\n",__func__);
		return;
	}
	if(priv->join_status < ATBM_APOLLO_JOIN_STATUS_MONITOR)
	{
		reorder_debug(REORDER_DEBUG,"interface type(%d) err\n",priv->join_status);
		return;
	}
	if((priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)&&
		(ba_params->link_id == ATBM_APOLLO_LINK_ID_UNMAPPED))
	{
		return;
	}
	tid = ba_params->tid;
	link_id = ba_params->link_id;
	atbm_reorder = &priv->atbm_reorder_link_id[link_id-1];
	tid_params = &atbm_reorder->atbm_rx_tid[tid];
	switch(action)
	{
		case ATBM_BA__ACTION_RX_ADDBR:
		{
			reorder_debug(REORDER_DEBUG,"WSM_BA_FLAGS__RX_ADDBA_RE\n");
//			mutex_lock(&atbm_reorder->reorder_mutex);
			if(test_bit(BAR_TID_EN,&tid_params->tid_en))
			{
				printk("tid_params->tid_en\n");	
				goto exit_action_rx_addba;
			}
			tid_params_spin_lock(&tid_params->skb_reorder_spinlock,spin_lock_bh);
			tid_params->ssn = ba_params->ssn;
			tid_params->wind_size = ba_params->win_size;
			tid_params->timeout = ba_params->timeout;
			if(tid_params->timeout == 0)
				tid_params->timeout = tid_params->wind_size*PER_PKG_TIME_INTERVAL;
			tid_params->start_seq = (ba_params->ssn)&SEQ_NUM_MASKER;
			tid_params->index_tail = tid_params->wind_size-1;
			for(i=0;i<BUFF_STORED_LEN;i++){
				tid_params->skb_reorder_buff[i]=NULL;
				tid_params->frame_rx_time[i]=0;
				}
			
			//__atbm_skb_queue_head_init(&tid_params->header);
			tid_params->reorder_priv = priv;
			tid_params->skb_buffed = 0;
			set_bit(BAR_TID_EN,&tid_params->tid_en);
			tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,spin_unlock_bh);
exit_action_rx_addba:
//			mutex_unlock(&atbm_reorder->reorder_mutex);
			break;
		}
		case ATBM_BA__ACTION_RX_DELBA:
		{
			
			if(!test_bit(BAR_TID_EN,&tid_params->tid_en))
				goto exit_action_rx_delba;
			reorder_debug(REORDER_DEBUG,"WSM_BA_FLAGS__RX_DELBA_RE,link_id(%d)\n",link_id);
			atbm_reorder_tid_reset(priv,tid_params);
//			mutex_lock(&atbm_reorder->reorder_mutex);
			tid_params_spin_lock(&tid_params->skb_reorder_spinlock,spin_lock_bh);
			clear_bit(BAR_TID_EN,&atbm_reorder->atbm_rx_tid[tid].tid_en);
			tid_params->ssn = 0;
			tid_params->wind_size = 0;
			tid_params->start_seq = 0;
			tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,spin_unlock_bh);
exit_action_rx_delba:
//			mutex_unlock(&atbm_reorder->reorder_mutex);
			break;			
		}
		case ATBM_BA__ACTION_RX_BAR:
		{
			u16 pre_start_seq = 0;
			 
//			mutex_lock(&atbm_reorder->reorder_mutex);
			if(!test_bit(BAR_TID_EN,&tid_params->tid_en))
				goto exit_actin_bar;
			tid_params_spin_lock(&tid_params->skb_reorder_spinlock,spin_lock_bh);
			pre_start_seq = tid_params->start_seq;
			//reorder_debug(REORDER_DEBUG,"RX_BAR,BARsn(%x),Ssn(%x)\n",ba_params->ssn,pre_start_seq);
			if(((ba_params->ssn-pre_start_seq)&SEQ_NUM_MASKER)>THE_RETRY_PKG_INEDX)
				goto exit_actin_bar_spin_unlock;
			if((ba_params->ssn-pre_start_seq)==0){
				goto exit_actin_bar_spin_unlock;
			}
			reorder_debug(REORDER_DEBUG,"RX_BAR,BARsn(%x),Ssn(%x)\n",ba_params->ssn,pre_start_seq);
			/*
			*if pre_start_seq+1 <ba_params->ssn maybe we have missed
			*same package so clear the buff queue
			*/
			if((((ba_params->ssn-pre_start_seq)&SEQ_NUM_MASKER) <= tid_params->wind_size)
				&&tid_params->skb_buffed)
			{
				u8 need_free = ((ba_params->ssn - pre_start_seq)&SEQ_NUM_MASKER);
				
				reorder_debug(REORDER_DEBUG,"RX_BAR,BARsn(%x),Ssn(%x)\n",ba_params->ssn,pre_start_seq);
				reorder_debug(REORDER_DEBUG,"_RX_BAR case 3,ssn(%x),buffed(%d)need_free %d\n",tid_params->start_seq,tid_params->skb_buffed,need_free);
				
				/*
				*1. a)if need_free >tid_params->wind_size, we dequeue the skb_queue ,then tid_params->skb_buffed
				*	  must be equal to zero;
				*    b)if need_free <tid_params->wind_size,we dequeue the skb_queue,then pre_start_seq must be 
				*	 equal to ba_params->ssn;
				*/
				atbm_reorder_skb_forcefree(priv,tid_params,need_free);
				/*
				*2. if need_free<tid_params->wind_size,then pre_start_seq must equal to ba_params->ssn
				*/
				if(tid_params->skb_buffed&&(tid_params->start_seq != ba_params->ssn))
				{
					reorder_debug(REORDER_DEBUG,"WSM_BA_FLAGS__RX_BAR err\n");
				}
				/*
				*3.from the index_head ,the skb_buff has some buff that is not equal to NULL,we should dequeue it.
				*but must MAKE SURE THAT :pre_start_seq == ba_params->ssn
				*/				
				atbm_reorder_skb_uplayer(priv,tid_params);
				
			}
			else {
				if(tid_params->skb_buffed){
					reorder_debug(REORDER_DEBUG,"_RX_BAR free ALL case 4,(%x)=ssn(%x)-start_seq(%x),buffed(%d)\n",(ba_params->ssn-pre_start_seq),
					ba_params->ssn,tid_params->start_seq,tid_params->skb_buffed);
					atbm_reorder_skb_forcefree(priv,tid_params,tid_params->wind_size);
				}
				tid_params->start_seq = ba_params->ssn;
			}

			if((!test_bit(REORDER_TIMER_RUNING,&tid_params->timer_running)||(pre_start_seq!= tid_params->start_seq))&&
				(tid_params->skb_buffed)&&
				(tid_params->timeout))
			{		
				u16 index= BUFF_INDEX_IS_SAFE(tid_params->start_seq);

				//reorder_debug(REORDER_DEBUG,"mod_timer,skb_buffed(%d),start_seq(%x)\n",tid_params->skb_buffed,tid_params->start_seq);
				set_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
				WARN_ON(tid_params->frame_rx_time[index] == 0);
				mod_timer(&tid_params->overtime_timer,
					tid_params->frame_rx_time[index] + (tid_params->timeout*HZ)/1000);
			}
			else if(tid_params->skb_buffed==0)
			{
				clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
				del_timer_sync(&tid_params->overtime_timer);
			}
			tid_params->ssn = ba_params->ssn;
exit_actin_bar_spin_unlock:
			tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,spin_unlock_bh);
exit_actin_bar:
//			mutex_unlock(&atbm_reorder->reorder_mutex);
			break;
		}
		default:
		{
			printk("%s:action err\n",__func__);
			break;
		}
	}
}
#endif
static void atbm_set_p2p_vif_addr(struct atbm_common *hw_priv)
{
	memcpy(hw_priv->addresses[1].addr,hw_priv->addresses[0].addr,ETH_ALEN);
	#ifdef ATBM_P2P_ADDR_USE_LOCAL_BIT
	hw_priv->addresses[1].addr[0] ^= BIT(1);
	#else
	hw_priv->addresses[1].addr[5] += 1;
	#endif
}
int atbm_set_mac_addr2efuse(struct ieee80211_hw *hw, u8 *macAddr)
{
	struct atbm_common *hw_priv = hw->priv;
	u8 EfusemacAddr[6] = {0};


	if (wsm_get_mac_address(hw_priv, &EfusemacAddr[0]) == 0)
	{
		
		if (EfusemacAddr[0]| EfusemacAddr[1]|EfusemacAddr[2]|EfusemacAddr[3]|EfusemacAddr[4]|EfusemacAddr[5])
		{
			printk(KERN_ERR "Cannot set MAC addr, because MAC addr of efuse have been writed\n");
			return -1;
		}
	}else
	{
		printk(KERN_ERR "Cannot get MAC addr, because MAC addr of efuse have been disabled\n");
		return -1;
	}

	if (wsm_set_efuse_mac_address(hw_priv, &macAddr[0]) == 0)
	{
		if (macAddr[0]| macAddr[1]|macAddr[2]|macAddr[3]|macAddr[4]|macAddr[5])
		{
			memcpy(hw_priv->addresses[0].addr,macAddr,ETH_ALEN);		
			atbm_set_p2p_vif_addr(hw_priv);
		}
	}else
	{
		printk(KERN_ERR "Cannot set MAC addr, because MAC addr of efuse have been disabled\n");
		return -1;
	}

	
	if (hw_priv->addresses[0].addr[3] == 0 &&
		hw_priv->addresses[0].addr[4] == 0 &&
		hw_priv->addresses[0].addr[5] == 0) {
		get_random_bytes(&hw_priv->addresses[0].addr[3], 3);
		atbm_set_p2p_vif_addr(hw_priv);
	}
#ifdef P2P_MULTIVIF
	memcpy(hw_priv->addresses[2].addr, hw_priv->addresses[1].addr,
		  ETH_ALEN);
	hw_priv->addresses[2].addr[4] ^= 0x80;
#endif

	SET_IEEE80211_PERM_ADDR(hw_priv->hw, hw_priv->addresses[0].addr);

	return 0;
}
static const struct ieee80211_ops atbm_ops = {
	.start			= atbm_start,
	.stop			= atbm_stop,
	.add_interface		= atbm_add_interface,
	.remove_interface	= atbm_remove_interface,
	.change_interface	= atbm_change_interface,
	.tx			= atbm_tx,
	.hw_scan		= atbm_hw_scan,
	.cancel_hw_scan = atbm_cancel_hw_scan,
#ifdef ROAM_OFFLOAD
	.sched_scan_start	= atbm_hw_sched_scan_start,
	.sched_scan_stop	= atbm_hw_sched_scan_stop,
#endif /*ROAM_OFFLOAD*/
	.set_tim		= atbm_set_tim,
	.sta_notify		= atbm_sta_notify,
	.sta_add		= atbm_sta_add,
	.sta_remove		= atbm_sta_remove,
	.set_key		= atbm_set_key,
	.set_rts_threshold	= atbm_set_rts_threshold,
	.config			= atbm_config,
	.bss_info_changed	= atbm_bss_info_changed,
	.prepare_multicast	= atbm_prepare_multicast,
	.configure_filter	= atbm_configure_filter,
	.conf_tx		= atbm_conf_tx,
	.get_stats		= atbm_get_stats,
	.ampdu_action		= atbm_ampdu_action,
	.flush			= atbm_flush,
#ifdef CONFIG_PM
	.suspend		= atbm_wow_suspend,
	.resume			= atbm_wow_resume,
#endif /* CONFIG_PM */
	/* Intentionally not offloaded:					*/
	/*.channel_switch	= atbm_channel_switch,		*/
	.remain_on_channel	= atbm_remain_on_channel,
	.cancel_remain_on_channel = atbm_cancel_remain_on_channel,
#ifdef IPV6_FILTERING
	.set_data_filter        = atbm_set_data_filter,
#endif /*IPV6_FILTERING*/
#ifdef CONFIG_NL80211_TESTMODE
	.testmode_cmd		= atbm_altmtest_cmd,
#endif
	.set_mac_addr2efuse = atbm_set_mac_addr2efuse,
};
#ifdef CONFIG_PM
static const struct wiphy_wowlan_support atbm_wowlan_support = {
	/* Support only for limited wowlan functionalities */
	.flags = WIPHY_WOWLAN_ANY | WIPHY_WOWLAN_DISCONNECT,
};
#endif
void atbm_get_mac_address(struct atbm_common *hw_priv)
{
	int i;
	u8 macAddr[6] = {0};
#ifdef CUSTOM_FEATURE_MAC/*TO usr macaddr of customers*/
	char readmac[17+1]={0};
#endif
	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++) {
		memcpy(hw_priv->addresses[i].addr,
			   atbm_mac_default[i],
			   ETH_ALEN);
	}
	if (wsm_get_mac_address(hw_priv, &macAddr[0]) == 0)
	{
		if (macAddr[0]| macAddr[1]|macAddr[2]|macAddr[3]|macAddr[4]|macAddr[5])
		{
			memcpy(hw_priv->addresses[0].addr,macAddr,ETH_ALEN);
			atbm_set_p2p_vif_addr(hw_priv);
		}
	}
#ifdef CUSTOM_FEATURE_MAC/* To use macaddr of customers */
	if(access_file(PATH_WIFI_MACADDR,readmac,17,1) > 0) {
		sscanf(readmac,"%2X:%2X:%2X:%2X:%2X:%2X",
								(int *)&hw_priv->addresses[0].addr[0],
								(int *)&hw_priv->addresses[0].addr[1],
								(int *)&hw_priv->addresses[0].addr[2],
								(int *)&hw_priv->addresses[0].addr[3],
								(int *)&hw_priv->addresses[0].addr[4],
								(int *)&hw_priv->addresses[0].addr[5]);
		atbm_set_p2p_vif_addr(hw_priv);
	}
#endif
	
	if (hw_priv->addresses[0].addr[3] == 0 &&
		hw_priv->addresses[0].addr[4] == 0 &&
		hw_priv->addresses[0].addr[5] == 0) {
		get_random_bytes(&hw_priv->addresses[0].addr[3], 3);
		atbm_set_p2p_vif_addr(hw_priv);
	}
#ifdef P2P_MULTIVIF
	memcpy(hw_priv->addresses[2].addr, hw_priv->addresses[1].addr,
		  ETH_ALEN);
	hw_priv->addresses[2].addr[4] ^= 0x80;
#endif

	SET_IEEE80211_PERM_ADDR(hw_priv->hw, hw_priv->addresses[0].addr);

}
#ifdef USB_BUS
extern 	void atbm_tx_tasklet(unsigned long priv);
extern 	void atbm_rx_tasklet(unsigned long priv);
extern void atbm_tx_complete_work(struct work_struct *work);
extern void atbm_rx_complete_work(struct work_struct *work);

#endif
extern void atbm_etf_test_expire_timer(unsigned long arg);
struct ieee80211_hw *atbm_init_common(size_t hw_priv_data_len)
{
	int i;
	struct ieee80211_hw *hw;
	struct atbm_common *hw_priv;
	struct ieee80211_supported_band *sband;
	int band;

	hw = ieee80211_alloc_hw(hw_priv_data_len, &atbm_ops);
	if (!hw)
		return NULL;

	hw_priv = hw->priv;
	/* TODO:COMBO this debug message can be removed */
	printk(KERN_ERR "Allocated hw_priv @ %p\n", hw_priv);
	hw_priv->if_id_slot = 0;
	hw_priv->roc_if_id = -1;
	hw_priv->bStartTx = 0;
	hw_priv->bStartTxWantCancel = 0;
	atomic_set(&hw_priv->num_vifs, 0);


	hw_priv->hw = hw;
	hw_priv->rates = atbm_rates; /* TODO: fetch from FW */
	hw_priv->mcs_rates = atbm_n_rates;
#ifdef ROAM_OFFLOAD
	hw_priv->auto_scanning = 0;
	hw_priv->frame_rcvd = 0;
	hw_priv->num_scanchannels = 0;
	hw_priv->num_2g_channels = 0;
	hw_priv->num_5g_channels = 0;
#endif /*ROAM_OFFLOAD*/

	/* Enable block ACK for 4 TID (BE,VI,VI,VO). */
	/*due to HW limitations*/
	//hw_priv->ba_tid_mask = 0xB1;
	//hw_priv->ba_tid_rx_mask = 0xB1;
	//hw_priv->ba_tid_tx_mask = 0xB1;
	hw_priv->ba_tid_rx_mask = ampdu;
	hw_priv->ba_tid_tx_mask = ampdu;

	hw->flags = IEEE80211_HW_SIGNAL_DBM |
		    IEEE80211_HW_SUPPORTS_PS |
		    IEEE80211_HW_SUPPORTS_DYNAMIC_PS |
		    IEEE80211_HW_REPORTS_TX_ACK_STATUS |
		    IEEE80211_HW_SUPPORTS_UAPSD |
		    IEEE80211_HW_CONNECTION_MONITOR |
		    IEEE80211_HW_SUPPORTS_CQM_RSSI |
		    /* Aggregation is fully controlled by firmware.
		     * Do not need any support from the mac80211 stack */
		    /* IEEE80211_HW_AMPDU_AGGREGATION | */
		    IEEE80211_HW_SUPPORTS_P2P_PS |
		    IEEE80211_HW_SUPPORTS_CQM_BEACON_MISS |
		    IEEE80211_HW_SUPPORTS_CQM_TX_FAIL |
		    IEEE80211_HW_BEACON_FILTER |
		    IEEE80211_HW_MFP_CAPABLE/*11W need this*/;

	hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
					  BIT(NL80211_IFTYPE_ADHOC) |
					  BIT(NL80211_IFTYPE_AP) |
					  BIT(NL80211_IFTYPE_MESH_POINT) |
					  BIT(NL80211_IFTYPE_P2P_CLIENT) |
					  BIT(NL80211_IFTYPE_P2P_GO);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
	hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);
#endif

#ifndef P2P_MULTIVIF
	hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_P2P_CLIENT) |
					  BIT(NL80211_IFTYPE_P2P_GO);
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
	hw->wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	hw->wiphy->flags |= WIPHY_FLAG_OFFCHAN_TX 
	#ifdef ATBM_AP_SME
							   | WIPHY_FLAG_HAVE_AP_SME
	#endif
								;
#endif
#ifdef CONFIG_PM
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0))
	/* Support only for limited wowlan functionalities */
	hw->wiphy->wowlan.flags = WIPHY_WOWLAN_ANY |
					  WIPHY_WOWLAN_DISCONNECT;
	hw->wiphy->wowlan.n_patterns = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	hw->wiphy->wowlan.max_pkt_offset = 0,
#endif
#else
	hw->wiphy->wowlan = &atbm_wowlan_support;
#endif
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 0, 8))
	hw->wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
#endif
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0))
	hw->wiphy->flags |= WIPHY_FLAG_DISABLE_BEACON_HINTS;
	#endif
	hw->wiphy->n_addresses = ATBM_WIFI_MAX_VIFS;
	hw->wiphy->addresses = hw_priv->addresses;
	hw->wiphy->max_remain_on_channel_duration = 1000;
	hw->channel_change_time = 500;	/* TODO: find actual value */
	/* hw_priv->beacon_req_id = cpu_to_le32(0); */
#ifdef ATBM_WIFI_QUEUE_LOCK_BUG
#ifndef P2P_MULTIVIF
	hw->queues = 8;
#else
	hw->queues = 12;
#endif
#else
	hw->queues = 4;
#endif
	hw_priv->noise = -94;

	hw->max_rates = 8;
	hw->max_rate_tries = 15;
	hw->extra_tx_headroom = WSM_TX_EXTRA_HEADROOM + 64 +
		8  /* TKIP IV */ +
		12 /* TKIP ICV and MIC */;
	hw->extra_tx_headroom += IEEE80211_ATBM_SKB_HEAD_SIZE;
	hw->sta_data_size = sizeof(struct atbm_sta_priv);
	hw->vif_data_size = sizeof(struct atbm_vif);
	if (sgi)
	{
		atbm_band_2ghz.ht_cap.cap |=  IEEE80211_HT_CAP_SGI_20;
		if (atbm_band_2ghz.ht_cap.cap &  IEEE80211_HT_CAP_SUP_WIDTH_20_40)
			atbm_band_2ghz.ht_cap.cap |=  IEEE80211_HT_CAP_SGI_40;
		if (atbm_band_2ghz.ht_cap.cap &  IEEE80211_HT_CAP_SUP_WIDTH_20_40)
			atbm_band_2ghz.ht_cap.cap |=  IEEE80211_HT_CAP_SGI_40;
	}else
	{		
		atbm_band_2ghz.ht_cap.cap &=  ~(IEEE80211_HT_CAP_SGI_20|IEEE80211_HT_CAP_SGI_40);
	}
	if (stbc)
	{
		//disable MIMO RXSTBC
		atbm_band_2ghz.ht_cap.cap |= 0;//IEEE80211_HT_CAP_RX_STBC;
	}else
	{
		atbm_band_2ghz.ht_cap.cap &= ~(IEEE80211_HT_CAP_TX_STBC|IEEE80211_HT_CAP_RX_STBC);
	}
	hw->wiphy->bands[IEEE80211_BAND_2GHZ] = &atbm_band_2ghz;
#ifdef CONFIG_ATBM_APOLLO_5GHZ_SUPPORT
	hw->wiphy->bands[IEEE80211_BAND_5GHZ] = &atbm_band_5ghz;
#endif /* CONFIG_ATBM_APOLLO_5GHZ_SUPPORT */

	/* Channel params have to be cleared before registering wiphy again */
	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		sband = hw->wiphy->bands[band];
		if (!sband)
			continue;
		for (i = 0; i < sband->n_channels; i++) {
			sband->channels[i].flags = 0;
			sband->channels[i].max_antenna_gain = 0;
			sband->channels[i].max_power = 30;
		}
	}

	hw->wiphy->max_scan_ssids = WSM_SCAN_MAX_NUM_OF_SSIDS;
	hw->wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;

	SET_IEEE80211_PERM_ADDR(hw, hw_priv->addresses[0].addr);
	spin_lock_init(&hw_priv->tx_com_lock);
	spin_lock_init(&hw_priv->rx_com_lock);
#ifdef USB_BUS
#ifdef USB_USE_TASTLET_TXRX
	printk("atbmwifi USB_USE_TASTLET_TXRX enable\n");
	tasklet_init(&hw_priv->tx_cmp_tasklet, atbm_tx_tasklet, (unsigned long)hw_priv);
	tasklet_init(&hw_priv->rx_cmp_tasklet, atbm_rx_tasklet, (unsigned long)hw_priv);
#else	
	printk("atbmwifi INIT_WORK enable\n");
	INIT_WORK(&hw_priv->rx_complete_work, atbm_rx_complete_work);
	INIT_WORK(&hw_priv->tx_complete_work, atbm_tx_complete_work);
	hw_priv->tx_workqueue= create_singlethread_workqueue("tx_workqueue");
	hw_priv->rx_workqueue= create_singlethread_workqueue("rx_workqueue");
#endif
#endif //#ifdef USB_BUS

#ifdef ATBM_SDIO_TXRX_ENHANCE

	sema_init(&hw_priv->sdio_rx_process_lock, 1);
#endif

	spin_lock_init(&hw_priv->wakelock_spinlock);
	hw_priv->wakelock_hw_counter = 0;
	hw_priv->wakelock_bh_counter = 0;
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&hw_priv->hw_wake, WAKE_LOCK_SUSPEND, "wlan_hw_wake");
	wake_lock_init(&hw_priv->bh_wake, WAKE_LOCK_SUSPEND, "wlan_bh_wake");
#endif /* CONFIG_HAS_WAKELOCK */


	spin_lock_init(&hw_priv->vif_list_lock);
	mutex_init(&hw_priv->wsm_cmd_mux);
	mutex_init(&hw_priv->conf_mutex);
	#ifndef OPER_CLOCK_USE_SEM
	mutex_init(&hw_priv->wsm_oper_lock);
	#else
	sema_init(&hw_priv->wsm_oper_lock, 1);
	init_timer(&hw_priv->wsm_pm_timer);
	hw_priv->wsm_pm_timer.data = (unsigned long)hw_priv;
	hw_priv->wsm_pm_timer.function = atbm_pm_timer;
	spin_lock_init(&hw_priv->wsm_pm_spin_lock);
	atomic_set(&hw_priv->wsm_pm_running, 0);
	#endif
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	spin_lock_init(&hw_priv->tsm_lock);
#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
	hw_priv->workqueue = create_singlethread_workqueue("atbm_wq");
	sema_init(&hw_priv->scan.lock, 1);
	INIT_WORK(&hw_priv->scan.work, atbm_scan_work);
#ifdef ROAM_OFFLOAD
	INIT_WORK(&hw_priv->scan.swork, atbm_sched_scan_work);
#endif /*ROAM_OFFLOAD*/
	INIT_DELAYED_WORK(&hw_priv->scan.probe_work, atbm_probe_work);
	INIT_DELAYED_WORK(&hw_priv->scan.timeout, atbm_scan_timeout);
#ifdef CONFIG_WIRELESS_EXT
	INIT_WORK(&hw_priv->etf_tx_end_work, etf_scan_end_work);
	//init_timer(&hw_priv->etf_expire_timer);	
	//hw_priv->etf_expire_timer.expires = jiffies+1000*HZ;
	//hw_priv->etf_expire_timer.data = (unsigned long)hw_priv;
	//hw_priv->etf_expire_timer.function = atbm_etf_test_expire_timer;
#endif //#ifdef CONFIG_WIRELESS_EXT
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	INIT_DELAYED_WORK(&hw_priv->advance_scan_timeout,
		 atbm_advance_scan_timeout);
#endif
	INIT_DELAYED_WORK(&hw_priv->rem_chan_timeout, atbm_rem_chan_timeout);
	INIT_WORK(&hw_priv->tx_policy_upload_work, tx_policy_upload_work);
	spin_lock_init(&hw_priv->event_queue_lock);
	INIT_LIST_HEAD(&hw_priv->event_queue);
	INIT_WORK(&hw_priv->event_handler, atbm_event_handler);
	INIT_WORK(&hw_priv->ba_work, atbm_ba_work);
	spin_lock_init(&hw_priv->ba_lock);
	init_timer(&hw_priv->ba_timer);
	hw_priv->ba_timer.data = (unsigned long)hw_priv;
	hw_priv->ba_timer.function = atbm_ba_timer;
	atbm_skb_queue_head_init(&hw_priv->rx_frame_queue);
	atbm_skb_queue_head_init(&hw_priv->rx_frame_free);
#ifdef ATBM_SUPPORT_SMARTCONFIG
	INIT_WORK(&hw_priv->scan.smartwork, atbm_smart_scan_work);
	INIT_WORK(&hw_priv->scan.smartsetChanwork, atbm_smart_setchan_work);
	INIT_WORK(&hw_priv->scan.smartstopwork, atbm_smart_stop_work);
	init_timer(&hw_priv->smartconfig_expire_timer);	
	hw_priv->smartconfig_expire_timer.data = (unsigned long)hw_priv;
	hw_priv->smartconfig_expire_timer.function = atbm_smartconfig_expire_timer;
#endif
#ifdef SDIO_BUS
	init_waitqueue_head(&hw_priv->wsm_synchanl_done);
#ifdef ATBM_SDIO_PATCH
	spin_lock_init(&hw_priv->SeqBitMapLock);
	INIT_LIST_HEAD(&hw_priv->SeqBitMapList);
	INIT_WORK(&hw_priv->wsm_sync_channl, wsm_sync_channl);
#else
	INIT_WORK(&hw_priv->wsm_sync_channl, wsm_sync_channl_reset);
#endif
#endif
	if (unlikely(atbm_queue_stats_init(&hw_priv->tx_queue_stats,
			WLAN_LINK_ID_MAX,
			atbm_skb_dtor,
			hw_priv))) {
		ieee80211_free_hw(hw);
		return NULL;
	}

	for (i = 0; i < 4; ++i) {
		if (unlikely(atbm_queue_init(&hw_priv->tx_queue[i],
				&hw_priv->tx_queue_stats, i, ATBM_WIFI_MAX_QUEUE_SZ,
				atbm_ttl[i]))) {
			for (; i > 0; i--)
				atbm_queue_deinit(&hw_priv->tx_queue[i - 1]);
			atbm_queue_stats_deinit(&hw_priv->tx_queue_stats);
			ieee80211_free_hw(hw);
			return NULL;
		}
	}
#ifdef ATBM_SUPPORT_WIDTH_40M
	spin_lock_init(&hw_priv->spinlock_channel_type);
	hw_priv->channel_type = NL80211_CHAN_NO_HT;
	atomic_set(&hw_priv->channel_chaging, 0);
	atomic_set(&hw_priv->phy_chantype,0);
	atomic_set(&hw_priv->tx_20M_lock,1);
	atomic_set(&hw_priv->cca_detect_running,0);
	hw_priv->chanch_if_id = ATBM_WIFI_MAX_VIFS;
//	INIT_WORK(&hw_priv->chantype_change_work, atbm_channel_type_change_work);
	INIT_WORK(&hw_priv->get_cca_work,atbm_get_cca_work);
	mutex_init(&hw_priv->chantype_mutex);
	init_timer(&hw_priv->chantype_timer);
	hw_priv->chantype_timer.data = (unsigned long)hw_priv;
	hw_priv->chantype_timer.function = atbm_chantype_timer;
	hw_priv->rx_phy_enable_num_req = DEFAULT_CCA_INTERVAL_US/DEFAULT_CCA_UTIL_US/3*2; //
	atomic_set(&hw_priv->cca_interval_ms,DEFAULT_CCA_INTERVAL_MS);
	hw_priv->rx_40M_pkg_cnt=0;
	hw_priv->rx_20M_pkg_cnt=0;
	hw_priv->rx_40M_pkg_detect = 0;
	hw_priv->rx_20M_pkg_detect = 0;
	clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANNUM_CHANGE,&hw_priv->change_bit);
	clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANTYPE_CHANGE,&hw_priv->change_bit);	
	clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit);
	atomic_set(&hw_priv->chw_sw_20M_level,20);
	atomic_set(&hw_priv->chw_sw_40M_level,128);
#endif

#ifdef ATBM_P2P_CHANGE
	atomic_set(&hw_priv->go_bssid_set,0);
	atomic_set(&hw_priv->receive_go_resp,0);
	atomic_set(&hw_priv->p2p_oper_channel,0);
	atomic_set(&hw_priv->combination,0);	
	atomic_set(&hw_priv->operating_channel_combination,0);
#endif

#ifdef ATBM_SUPPORT_PKG_MONITOR
	hw_priv->monitor_if_id = -1;
#endif
	init_waitqueue_head(&hw_priv->channel_switch_done);
	init_waitqueue_head(&hw_priv->wsm_cmd_wq);
	init_waitqueue_head(&hw_priv->wsm_startup_done);
	init_waitqueue_head(&hw_priv->offchannel_wq);
	hw_priv->offchannel_done = 0;
	wsm_buf_init(&hw_priv->wsm_cmd_buf);
	spin_lock_init(&hw_priv->wsm_cmd.lock);
	tx_policy_init(hw_priv);
#if defined(CONFIG_ATBM_APOLLO_WSM_DUMPS_SHORT)
	hw_priv->wsm_dump_max_size = 20;
#endif /* CONFIG_ATBM_APOLLO_WSM_DUMPS_SHORT */
	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++)
		hw_priv->hw_bufs_used_vif[i] = 0;

#ifdef MCAST_FWDING
       for (i = 0; i < WSM_MAX_BUF; i++)
               wsm_init_release_buffer_request(hw_priv, i);
       hw_priv->buf_released = 0;
#endif
	hw_priv->vif0_throttle = ATBM_WIFI_HOST_VIF0_11BG_THROTTLE;
	hw_priv->vif1_throttle = ATBM_WIFI_HOST_VIF1_11BG_THROTTLE;
	return hw;
}
EXPORT_SYMBOL_GPL(atbm_init_common);

int atbm_register_common(struct ieee80211_hw *dev)
{
#ifdef CONFIG_ATBM_APOLLO_LEDS
	struct atbm_common *hw_priv = dev->priv;
#endif /* CONFIG_ATBM_APOLLO_LEDS */
	int err;
	err = ieee80211_register_hw(dev);
	if (err) {
		atbm_dbg(ATBM_APOLLO_DBG_ERROR, "Cannot register device (%d).\n",
				err);
		return err;
	}

#ifdef CONFIG_ATBM_APOLLO_LEDS
	err = atbm_init_leds(priv);
	if (err) {
		atbm_pm_deinit(&hw_priv->pm_state);
		ieee80211_unregister_hw(dev);
		return err;
	}
#endif /* CONFIG_ATBM_APOLLO_LEDS */


	atbm_dbg(ATBM_APOLLO_DBG_MSG, "is registered as '%s'\n",
			wiphy_name(dev->wiphy));
	return 0;
}
EXPORT_SYMBOL_GPL(atbm_register_common);

void atbm_free_common(struct ieee80211_hw *dev)
{
	/* struct atbm_common *hw_priv = dev->priv; */
	/* unsigned int i; */

	ieee80211_free_hw(dev);
}
EXPORT_SYMBOL_GPL(atbm_free_common);

void atbm_unregister_common(struct ieee80211_hw *dev)
{
	struct atbm_common *hw_priv = dev->priv;
	int i;
	atomic_set(&hw_priv->atbm_pluged,0);
	printk(KERN_ERR "atbm_unregister_common.++\n");
	ieee80211_unregister_hw(dev);
	
	hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);

	
#ifdef USB_BUS
#ifdef USB_USE_TASTLET_TXRX
	tasklet_kill(&hw_priv->tx_cmp_tasklet);
	tasklet_kill(&hw_priv->rx_cmp_tasklet);	
#else
	flush_workqueue(hw_priv->tx_workqueue);
	destroy_workqueue(hw_priv->tx_workqueue);
	hw_priv->tx_workqueue = NULL;	
	flush_workqueue(hw_priv->rx_workqueue);
	destroy_workqueue(hw_priv->rx_workqueue);
	hw_priv->rx_workqueue = NULL;	
#endif
#endif

	hw_priv->init_done = 0;
	atbm_unregister_bh(hw_priv);
	atbm_debug_release_common(hw_priv);
#ifdef OPER_CLOCK_USE_SEM
	del_timer_sync(&hw_priv->wsm_pm_timer);
	spin_lock_bh(&hw_priv->wsm_pm_spin_lock);
	if(atomic_read(&hw_priv->wsm_pm_running) == 1){
		atomic_set(&hw_priv->wsm_pm_running, 0);
		wsm_oper_unlock(hw_priv);
		atbm_release_suspend(hw_priv);
		printk(KERN_ERR "%s,up pm lock\n",__func__);
	}
	spin_unlock_bh(&hw_priv->wsm_pm_spin_lock);
#endif	
#ifdef CONFIG_ATBM_APOLLO_LEDS
	atbm_unregister_leds(hw_priv);
#endif /* CONFIG_ATBM_APOLLO_LEDS */

	mutex_destroy(&hw_priv->conf_mutex);
#ifdef MCAST_FWDING
	for (i = 0; i < WSM_MAX_BUF; i++)
		wsm_buf_deinit(&hw_priv->wsm_release_buf[i]);
#endif
	del_timer_sync(&hw_priv->ba_timer);
	flush_workqueue(hw_priv->workqueue);
	destroy_workqueue(hw_priv->workqueue);
	hw_priv->workqueue = NULL;

#ifdef CONFIG_HAS_WAKELOCK
	hw_priv->wakelock_hw_counter = 0;
	hw_priv->wakelock_bh_counter = 0;
	wake_lock_destroy(&hw_priv->hw_wake);
	wake_lock_destroy(&hw_priv->bh_wake);
#endif /* CONFIG_HAS_WAKELOCK */

	wsm_buf_deinit(&hw_priv->wsm_cmd_buf);
	if (hw_priv->skb_cache) {
		atbm_dev_kfree_skb(hw_priv->skb_cache);
		hw_priv->skb_cache = NULL;
	}

	if (hw_priv->sdd) {
		release_firmware(hw_priv->sdd);
		hw_priv->sdd = NULL;
	}

	atbm_free_event_queue(hw_priv);
	
	for (i = 0; i < 4; ++i)
		atbm_queue_deinit(&hw_priv->tx_queue[i]);
	atbm_queue_stats_deinit(&hw_priv->tx_queue_stats);
	#ifdef CONFIG_PM
	atbm_pm_deinit(&hw_priv->pm_state);
	#endif

	//for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++) {
	//	atbm_kfree(hw_priv->vif_list[i]);
	//	hw_priv->vif_list[i] = NULL;
	//}
	/*
	*only sdio chip reset cpu here
	*/
	if(hw_priv->sbus_ops->sbus_reset_chip)
		hw_priv->sbus_ops->sbus_reset_chip(hw_priv->sbus_priv);
	printk(KERN_ERR "atbm_unregister_common.--\n");
}
EXPORT_SYMBOL_GPL(atbm_unregister_common);

static void ABwifi_set_ifce_comb(struct atbm_common *hw_priv,
				 struct ieee80211_hw *hw)
{
#ifdef P2P_MULTIVIF
	hw_priv->if_limits1[0].max = 2;
#else
	hw_priv->if_limits1[0].max = 1;
#endif

	hw_priv->if_limits1[0].types = BIT(NL80211_IFTYPE_STATION);
	hw_priv->if_limits1[1].max = 1;
	hw_priv->if_limits1[1].types = BIT(NL80211_IFTYPE_AP);

#ifdef P2P_MULTIVIF
	hw_priv->if_limits2[0].max = 3;
#else
	hw_priv->if_limits2[0].max = 2;
#endif
	hw_priv->if_limits2[0].types = BIT(NL80211_IFTYPE_STATION);

#ifdef P2P_MULTIVIF
       hw_priv->if_limits3[0].max = 2;
#else
	hw_priv->if_limits3[0].max = 1;
#endif

	hw_priv->if_limits3[0].types = BIT(NL80211_IFTYPE_STATION);
	hw_priv->if_limits3[1].max = 1;
	#ifdef P2P_MULTIVIF
	hw_priv->if_limits3[1].types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
				      BIT(NL80211_IFTYPE_P2P_GO);
	#else
	hw_priv->if_limits3[1].types = BIT(NL80211_IFTYPE_AP);
	#endif

	/* TODO:COMBO: mac80211 doesn't yet support more than 1
	 * different channel */
	hw_priv->if_combs[0].num_different_channels = 1;
#ifdef P2P_MULTIVIF
        hw_priv->if_combs[0].max_interfaces = 3;
#else
	hw_priv->if_combs[0].max_interfaces = 2;
#endif
	hw_priv->if_combs[0].limits = hw_priv->if_limits1;
	hw_priv->if_combs[0].n_limits = 2;

	hw_priv->if_combs[1].num_different_channels = 1;

#ifdef P2P_MULTIVIF
        hw_priv->if_combs[1].max_interfaces = 3;
#else
	hw_priv->if_combs[1].max_interfaces = 2;
#endif
	hw_priv->if_combs[1].limits = hw_priv->if_limits2;
	hw_priv->if_combs[1].n_limits = 1;

	hw_priv->if_combs[2].num_different_channels = 1;
#ifdef P2P_MULTIVIF
        hw_priv->if_combs[2].max_interfaces = 3;
#else
	hw_priv->if_combs[2].max_interfaces = 2;
#endif
	hw_priv->if_combs[2].limits = hw_priv->if_limits3;
	hw_priv->if_combs[2].n_limits = 2;

	hw->wiphy->iface_combinations = &hw_priv->if_combs[0];
	hw->wiphy->n_iface_combinations = 3;

}

void atbm_monitor_pc(struct atbm_common *hw_priv);
//#ifdef RESET_CHANGE
struct atbm_common *g_hw_priv=0;
//#endif
int atbm_core_probe(const struct sbus_ops *sbus_ops,
		      struct sbus_priv *sbus,
		      struct device *pdev,
		      struct atbm_common **pself)
{
	int err = -ENOMEM;
	struct ieee80211_hw *dev;
	struct atbm_common *hw_priv=NULL;
	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};
	int if_id;
#ifdef CUSTOM_FEATURE_PSM/* To control ps mode */
	char buffer[2];
    savedpsm = mode.power_mode;
	FUNC_ENTER();
	if(access_file(PATH_WIFI_PSM_INFO,buffer,2,1) > 0) {
		if(buffer[0] == 0x30) {
			mode.power_mode = wsm_power_mode_active;
		}
		else
		{
			if(savedpsm)
				mode.power_mode = savedpsm;
			else /* Set default */
				mode.power_mode = wsm_power_mode_quiescent;
		}
		printk("apollo wifi : PSM changed to %d\n",mode.power_mode);
	}
	else {
		printk("apollo wifi : Using default PSM %d\n",mode.power_mode);
	}
#endif

	dev = atbm_init_common(sizeof(struct atbm_common));
	if (!dev)
		goto err;

	hw_priv = dev->priv;
#ifdef RESET_CHANGE
	atomic_set(&hw_priv->reset_flag, 0);
	atomic_set(&hw_priv->fw_reloading, 0);
	atomic_set(&hw_priv->reset_conter, 0);
	spin_lock_init(&hw_priv->send_deauthen_lock);
#endif
	atbm_hw_priv_assign_pointer(hw_priv);
	hw_priv->init_done = 0;
	atomic_set(&hw_priv->atbm_pluged,0);
	hw_priv->sbus_ops = sbus_ops;
	hw_priv->sbus_priv = sbus;
	hw_priv->pdev = pdev;
	SET_IEEE80211_DEV(hw_priv->hw, pdev);

	/* WSM callbacks. */
	hw_priv->wsm_cbc.scan_complete = atbm_scan_complete_cb;
	hw_priv->wsm_cbc.tx_confirm = atbm_tx_confirm_cb;
	hw_priv->wsm_cbc.rx = atbm_rx_cb;
	hw_priv->wsm_cbc.suspend_resume = atbm_suspend_resume;
	/* hw_priv->wsm_cbc.set_pm_complete = atbm_set_pm_complete_cb; */
	hw_priv->wsm_cbc.channel_switch = atbm_channel_switch_cb;
	*pself = dev->priv;
#ifdef CONFIG_PM
	err = atbm_pm_init(&hw_priv->pm_state, hw_priv);
	if (err) {
		atbm_dbg(ATBM_APOLLO_DBG_ERROR, "Cannot init PM. (%d).\n",
				err);
		goto err1;
	}
#endif
	err = atbm_register_bh(hw_priv);
	if (err)
		goto err1;
	
	atomic_set(&hw_priv->atbm_pluged,1);
reload_fw:
	err = atbm_load_firmware(hw_priv);
	if (err){
		printk("atbm_load_firmware ERROR!\n");
		goto err2;
	}

	ABwifi_set_ifce_comb(hw_priv, dev);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	WARN_ON(hw_priv->sbus_ops->set_block_size(hw_priv->sbus_priv,
		SDIO_BLOCK_SIZE));
	printk("set_block_size=%d\n", SDIO_BLOCK_SIZE);
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);

	hw_priv->init_done = 1;
#if 0
	printk("mdelay wait wsm_startup_done  !!\n");
	if (wait_event_interruptible_timeout(hw_priv->wsm_startup_done,
			hw_priv->wsm_caps.firmwareReady,3*HZ)<=0){
			atbm_monitor_pc(hw_priv);
			mdelay(10);
			atbm_monitor_pc(hw_priv);
			printk("wait wsm_startup_done timeout!!\n");
			if(hw_priv->sbus_ops->sbus_reset_chip){
				printk(KERN_ERR "reload fw\n");
				if(hw_priv->sbus_ops->irq_unsubscribe)
					hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);
				hw_priv->sbus_ops->sbus_reset_chip(hw_priv->sbus_priv);
				goto reload_fw;
			}else {
				err = -ETIMEDOUT;
				goto err3;
			}
	}
#else	
//fix mstar CVT suspend bug,in CVT mstar suspend wait_event_interruptible_timeout sometime will not delay
	{
	int loop =0;
	printk("mdelay wait wsm_startup_done  !!\n");
	while(hw_priv->wsm_caps.firmwareReady !=1){
		mdelay(10);
		if(loop++>300){
			printk("wait_event_interruptible_timeout wsm_startup_done timeout ERROR !!\n");
			
			atbm_monitor_pc(hw_priv);
			if(hw_priv->sbus_ops->sbus_reset_chip){
					printk(KERN_ERR "reload fw\n");
					if(hw_priv->sbus_ops->irq_unsubscribe)
						hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);
					hw_priv->sbus_ops->sbus_reset_chip(hw_priv->sbus_priv);
					goto reload_fw;
			}else {
				err = -ETIMEDOUT;
				goto err3;
			}
		}
	}
	}
#endif 
	atbm_firmware_init_check(hw_priv);
	for (if_id = 0; if_id < ABwifi_get_nr_hw_ifaces(hw_priv); if_id++) {
		/* Set low-power mode. */
		err = wsm_set_operational_mode(hw_priv, &mode, if_id);
		if (err) {
			WARN_ON(1);
			goto err3;
		}
		/* Enable multi-TX confirmation */
		err = wsm_use_multi_tx_conf(hw_priv, true, if_id);
		if (err) {
#ifndef CONFIG_TX_NO_CONFIRM
			WARN_ON(1);
			goto err3;
#endif //CONFIG_TX_NO_CONFIRM
		}
	}
	atbm_set_fw_ver(hw_priv);
	atbm_get_mac_address(hw_priv);
	if (efuse)
	{
		char buffer[15];
		if(access_file(PATH_WIFI_EFUSE,buffer,15,1) > 0) {
			err = wsm_set_efuse_data(hw_priv, buffer, 15);
			if (err) {
				WARN_ON(1);
				goto err3;
			}
			printk("apollo wifi : Set efuse data\n");
		}
	}
	{
		struct efuse_headr efuse_data;
		err = wsm_get_efuse_data(hw_priv, (void *)&efuse_data, sizeof(efuse_data));
		if (err) {
			WARN_ON(1);
			goto err3;
		}
		printk("efuse data is [0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x:0x%x:0x%x:0x%x:0x%x:0x%x]\n",
				efuse_data.version,efuse_data.dcxo_trim,efuse_data.delta_gain1,efuse_data.delta_gain2,efuse_data.delta_gain3,
				efuse_data.Tj_room,efuse_data.topref_ctrl_bias_res_trim,efuse_data.PowerSupplySel,efuse_data.mac[0],efuse_data.mac[1],
				efuse_data.mac[2],efuse_data.mac[3],efuse_data.mac[4],efuse_data.mac[5]);
	}
	err = atbm_register_common(dev);
	if (err) {
		goto err3;
	}
#ifdef CONFIG_PM
	atbm_pm_stay_awake(&hw_priv->pm_state, 6 * HZ);
#endif  //#ifdef CONFIG_PM
	*pself = dev->priv;
	return err;

err3:
	//sbus_ops->reset(sbus);
	hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);
err2:
	atomic_set(&hw_priv->atbm_pluged,0);
	atbm_unregister_bh(hw_priv);
err1:
	#ifdef CONFIG_PM
	atbm_pm_deinit(&hw_priv->pm_state);
	#endif
	atbm_free_common(dev);
err:
	*pself=NULL;
	if(hw_priv)
		hw_priv->init_done = 0;
	return err;
}
EXPORT_SYMBOL_GPL(atbm_core_probe);

void atbm_core_release(struct atbm_common *self)
{
	atbm_unregister_common(self->hw);
	atbm_free_common(self->hw);
	return;
}
EXPORT_SYMBOL_GPL(atbm_core_release);
#ifdef CUSTOM_FEATURE_MAC /* To use macaddr and ps mode of customers */
int access_file(char *path, char *buffer, int size, int isRead)
{
	int ret=0;
	struct file *fp;
	mm_segment_t old_fs = get_fs();

	if(isRead)
		fp = filp_open(path,O_RDONLY,S_IRUSR);
	else
		fp = filp_open(path,O_CREAT|O_WRONLY,S_IRUSR);

	if (IS_ERR(fp)) {
		printk("apollo wifi : can't open %s\n",path);
		return -1;
	}

	if(isRead)
	{
		if(fp->f_op->read == NULL) {
			printk("apollo wifi : Not allow to read\n");
			return -2;
		}
		else {
			fp->f_pos = 0;
			set_fs(KERNEL_DS);
			ret = vfs_read(fp,buffer,size,&fp->f_pos);
			set_fs(old_fs);
		}
	}
	else
	{
		if(fp->f_op->write == NULL) {
			printk("apollo wifi : Not allow to write\n");
			return -2;
		}
		else {
			fp->f_pos = 0;
			set_fs(KERNEL_DS);
			ret = vfs_write(fp,buffer,size,&fp->f_pos);
			set_fs(old_fs);
		}
	}
	filp_close(fp,NULL);

	printk("apollo wifi : access_file return code(%d)\n",ret);
	return ret;
}
#endif
#if defined(CONFIG_NL80211_TESTMODE) || defined(CONFIG_ATBM_IOCTRL)


void atbm_set_shortGI(u32 shortgi)
{
	printk("atbm_set_shortGI,%d\n", shortgi);
	if (shortgi)
	{
		atbm_band_2ghz.ht_cap.cap |=  IEEE80211_HT_CAP_SGI_20;
		if (atbm_band_2ghz.ht_cap.cap &  IEEE80211_HT_CAP_SUP_WIDTH_20_40)
			atbm_band_2ghz.ht_cap.cap |=  IEEE80211_HT_CAP_SGI_40;
	}else
	{
		atbm_band_2ghz.ht_cap.cap &=  ~(IEEE80211_HT_CAP_SGI_20|IEEE80211_HT_CAP_SGI_40);
	}

}
void atbm_set_40M(struct atbm_common *hw_priv,u32 params)
{
	#define ATBM_40M_FREE			((u32)(-1))
#ifdef ATBM_SUPPORT_WIDTH_40M
	u16 is40M = (u16)(params>>16);
	u16 chtype = (u16)(params&0xffff);
	static u8 saved_chw;
	static enum nl80211_channel_type saved_chtype;
	struct atbm_vif * priv = ABwifi_hwpriv_to_vifpriv(hw_priv,0);
	
	if(!priv)
	{
		return;
	}
	
	if(!test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit))
	{
		saved_chw = atomic_read(&hw_priv->tx_20M_lock);
		saved_chtype = hw_priv->channel_type;
		printk("%s:saved_chw(%d),saved_chtype(%d)\n",__func__,saved_chw,saved_chtype);
	}
	atbm_priv_vif_list_read_unlock(&priv->vif_lock);

	if((params == ATBM_40M_FREE)&&
		test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit))
	{
		atomic_set(&hw_priv->tx_20M_lock,saved_chw);
		is40M = saved_chw;
		chtype = saved_chtype;
		clear_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit);
		printk("%s: free the 40M params\n",__func__);
	}
	else if(params == ATBM_40M_FREE)
	{
		printk("the params is err params(%x)\n",params);
		return ;
	}
	else
	{
		set_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit);
	}
	
	printk("atbm_set_40M,chtype(%d),send_40M(%d)\n",chtype ,is40M);
	
	if(chtype>NL80211_CHAN_HT40PLUS)
	{
		chtype = hw_priv->channel_type;
	}
	
	if(chtype != hw_priv->channel_type)
	{
		spin_lock_bh(&hw_priv->spinlock_channel_type);
		hw_priv->channel_type = chtype;
		spin_unlock_bh(&hw_priv->spinlock_channel_type);
		set_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANTYPE_CHANGE,&hw_priv->change_bit);
		printk("%s:chtype(%d),channel_type(%d)\n",__func__,chtype,hw_priv->channel_type);
	}
	
	is40M = ((is40M)&&(chtype>=NL80211_CHAN_HT40MINUS));
	if(test_bit(WSM_SET_CHANTYPECHANGE_FLAGS__TOOL_SET_FORCE,&hw_priv->change_bit))
		atomic_set(&hw_priv->tx_20M_lock,!!!is40M);
	
	atomic_add_return(1, &hw_priv->channel_chaging);
	if(atbm_hw_priv_queue_delayed_work(hw_priv, &priv->chantype_change_work,0) <= 0)
	{
		printk("%s: queue chantype_change_work err\n",__func__);
		if(atomic_sub_return(1, &hw_priv->channel_chaging)<=0)
		{
			printk("%s: channel_chaging value err\n",__func__);
		}
	}
	
	if (chtype>=NL80211_CHAN_HT40MINUS)
	{
		atbm_band_2ghz.ht_cap.cap |=  IEEE80211_HT_CAP_SUP_WIDTH_20_40;
		if (atbm_band_2ghz.ht_cap.cap & IEEE80211_HT_CAP_SGI_20)
			atbm_band_2ghz.ht_cap.cap |=  IEEE80211_HT_CAP_SGI_40;
	}else
	{
		atbm_band_2ghz.ht_cap.cap &=  ~(IEEE80211_HT_CAP_SUP_WIDTH_20_40|IEEE80211_HT_CAP_SGI_40);
	}
#endif

}

void atbm_set_cca(struct atbm_common *hw_priv,u8 *buff)
{
#ifdef ATBM_SUPPORT_WIDTH_40M	

	#define ATBM_CCA_FREE			((u32)(-1))
	struct atbm_cca_val
	{
		u32 cca_20M_level;
		u32 cca_40M_level;
		u32 cca_interval;
	} *patbm_cca_val = NULL;

	struct atbm_cca_chage
	{
		u8 cca_20M_level_change;
		u8 cca_40M_level_change;
		u8 cca_interval_change;
	}cca_change = {.cca_interval_change = 0, .cca_20M_level_change=0,.cca_40M_level_change  = 0,};
	patbm_cca_val = (struct atbm_cca_val *)buff;
	printk(KERN_ERR "%s:cca_interval(%d),cca_20M_level(%d),cca_40M_level(%d)",__func__,patbm_cca_val->cca_interval,
		patbm_cca_val->cca_20M_level,patbm_cca_val->cca_40M_level);
	if(patbm_cca_val->cca_interval != ATBM_CCA_FREE) cca_change.cca_interval_change = 1;
	if(patbm_cca_val->cca_20M_level != ATBM_CCA_FREE)  cca_change.cca_20M_level_change = 1;
	if(patbm_cca_val->cca_40M_level != ATBM_CCA_FREE)  cca_change.cca_40M_level_change = 1;

	if(cca_change.cca_interval_change&&
		(atomic_read(&hw_priv->cca_interval_ms) != patbm_cca_val->cca_interval))
	{
		printk(KERN_ERR "%s:cca_interval_change(%d)\n",__func__,patbm_cca_val->cca_interval);
		atomic_set(&hw_priv->cca_interval_ms, patbm_cca_val->cca_interval) ;
		set_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CCA_LEVEL_CHANGE,&hw_priv->change_bit);
	}

	if(cca_change.cca_20M_level_change)
	{
		printk(KERN_ERR "%s:cca_20M_level_change(%d)\n",__func__,patbm_cca_val->cca_20M_level);
		atomic_set(&hw_priv->chw_sw_20M_level, patbm_cca_val->cca_20M_level) ;
	}
	if(cca_change.cca_40M_level_change)
	{
		printk(KERN_ERR "%s:cca_40M_level_change(%d)\n",__func__,patbm_cca_val->cca_40M_level);
		atomic_set(&hw_priv->chw_sw_40M_level, patbm_cca_val->cca_40M_level) ;
	}
	
#endif
	
}
#endif

