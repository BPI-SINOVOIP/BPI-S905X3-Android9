/*
 * mac80211 STA and AP API for mac80211 altobeam APOLLO drivers
 *
 *
 * Copyright (c) 2016, altobeam
 * Author: 
 *
 * Based on 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "apollo.h"
#include "sta.h"
#include "ap.h"
#include "bh.h"
#include "net/atbm_mac80211.h"
#include "mac80211/ieee80211_i.h"


#if defined(CONFIG_ATBM_APOLLO_STA_DEBUG)
#define ap_printk(...) printk(__VA_ARGS__)
#else
#define ap_printk(...)
#endif
extern int start_choff;
#define HT_INFO_OFFSET 4
#define HT_INFO_MASK 0x0011
#define HT_INFO_IE_LEN 22
#define ATBM_APOLLO_LINK_ID_GC_TIMEOUT ((unsigned long)(10 * HZ))

#define ATBM_APOLLO_ENABLE_ARP_FILTER_OFFLOAD	3
/*For Samsung, it is defined as 4*/
#define ATBM_APOLLO_KEEP_ALIVE_PERIOD	(4)

#ifndef ERP_INFO_BYTE_OFFSET
#define ERP_INFO_BYTE_OFFSET 2
#endif

#ifdef IPV6_FILTERING
#define ATBM_APOLLO_ENABLE_NDP_FILTER_OFFLOAD	3
#endif /*IPV6_FILTERING*/

static int atbm_upload_beacon(struct atbm_vif *priv);
#ifdef ATBM_PROBE_RESP_EXTRA_IE
static int atbm_upload_proberesp(struct atbm_vif *priv);
#endif
static int atbm_upload_pspoll(struct atbm_vif *priv);
static int atbm_upload_null(struct atbm_vif *priv);
static int atbm_upload_qosnull(struct atbm_vif *priv);
static int atbm_start_ap(struct atbm_vif *priv);
static int atbm_update_beaconing(struct atbm_vif *priv);
/*
static int atbm_enable_beaconing(struct atbm_vif *priv,
				   bool enable);
*/
static void __atbm_sta_notify(struct atbm_vif *priv,
				enum sta_notify_cmd notify_cmd,
				int link_id);

/* ******************************************************************** */
/* AP API								*/

int atbm_sta_add(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta)
{
	struct atbm_sta_priv *sta_priv =
			(struct atbm_sta_priv *)&sta->drv_priv;
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);
	struct atbm_link_entry *entry;
	struct sk_buff *skb;
	struct atbm_common *hw_priv = hw->priv;

	if(atbm_bh_is_term(hw_priv)){
		return 0;
	}
#ifdef P2P_MULTIVIF
	WARN_ON(priv->if_id == ATBM_WIFI_GENERIC_IF_ID);
#endif

	if (priv->mode != NL80211_IFTYPE_AP)
		return 0;
	printk(KERN_ERR "%s:sta(%pM)\n",__func__,sta->addr);
	sta_priv->priv = priv;
	sta_priv->link_id = atbm_find_link_id(priv, sta->addr);
	if (WARN_ON(!sta_priv->link_id)) {
		/* Impossible error */
		wiphy_info(priv->hw->wiphy,
			"[AP] No more link IDs available.\n");
		return -ENOENT;
	}

	entry = &priv->link_id_db[sta_priv->link_id - 1];
	spin_lock_bh(&priv->ps_state_lock);
	if ((sta->uapsd_queues & IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK) ==
					IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK)
		priv->sta_asleep_mask |= BIT(sta_priv->link_id);
	entry->status = ATBM_APOLLO_LINK_HARD;
	while ((skb = atbm_skb_dequeue(&entry->rx_queue)))
		ieee80211_rx_irqsafe(priv->hw, skb);
	spin_unlock_bh(&priv->ps_state_lock);
	hw_priv->connected_sta_cnt++;
	if(hw_priv->connected_sta_cnt>1) {
			wsm_lock_tx(hw_priv);
			WARN_ON(wsm_set_block_ack_policy(hw_priv,
					ATBM_APOLLO_TX_BLOCK_ACK_DISABLED_FOR_ALL_TID,
					ATBM_APOLLO_RX_BLOCK_ACK_DISABLED_FOR_ALL_TID,
					priv->if_id));
			wsm_unlock_tx(hw_priv);
	}
	return 0;
}

int atbm_sta_remove(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		      struct ieee80211_sta *sta)
{
	struct atbm_common *hw_priv = hw->priv;
	struct atbm_sta_priv *sta_priv =
			(struct atbm_sta_priv *)&sta->drv_priv;
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);
	struct atbm_link_entry *entry;

	if(atbm_bh_is_term(hw_priv)){
		return 0;
	}
#ifdef P2P_MULTIVIF
	WARN_ON(priv->if_id == ATBM_WIFI_GENERIC_IF_ID);
#endif

	if (priv->mode != NL80211_IFTYPE_AP || !sta_priv->link_id)
		return 0;
	printk(KERN_ERR "%s:sta(%pM)\n",__func__,sta->addr);
	entry = &priv->link_id_db[sta_priv->link_id - 1];
#ifdef 	ATBM_PKG_REORDER
	atbm_reorder_func_reset(priv,sta_priv->link_id - 1);
#endif
	spin_lock_bh(&priv->ps_state_lock);
	entry->status = ATBM_APOLLO_LINK_RESERVE;
	entry->timestamp = jiffies;
	wsm_lock_tx_async(hw_priv);
	if (atbm_hw_priv_queue_work(hw_priv, &priv->link_id_work) <= 0)
		wsm_unlock_tx(hw_priv);
	spin_unlock_bh(&priv->ps_state_lock);
	flush_workqueue(hw_priv->workqueue);
	hw_priv->connected_sta_cnt--;
	if(hw_priv->connected_sta_cnt <= 1) {
		if ((priv->if_id != 1) ||
			((priv->if_id == 1) 
			#ifdef P2P_MULTIVIF
			&& hw_priv->is_go_thru_go_neg
			#endif
			)
			) {
			wsm_lock_tx(hw_priv);
			WARN_ON(wsm_set_block_ack_policy(hw_priv,
						ATBM_APOLLO_TX_BLOCK_ACK_ENABLED_FOR_ALL_TID,
						ATBM_APOLLO_RX_BLOCK_ACK_ENABLED_FOR_ALL_TID,
						priv->if_id));
			wsm_unlock_tx(hw_priv);
		}
	}
	return 0;
}

static void __atbm_sta_notify(struct atbm_vif *priv,
				enum sta_notify_cmd notify_cmd,
				int link_id)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	u32 bit, prev;

	/* Zero link id means "for all link IDs" */
	if (link_id)
		bit = BIT(link_id);
	else if (WARN_ON_ONCE(notify_cmd != STA_NOTIFY_AWAKE))
		bit = 0;
	else
		bit = priv->link_id_map;
	prev = priv->sta_asleep_mask & bit;

	switch (notify_cmd) {
	case STA_NOTIFY_SLEEP:
		if (!prev) {
			if (priv->buffered_multicasts &&
					!priv->sta_asleep_mask)
				atbm_hw_priv_queue_work(hw_priv,
					&priv->multicast_start_work);
			priv->sta_asleep_mask |= bit;
		}
		break;
	case STA_NOTIFY_AWAKE:
		if (prev) {
			priv->sta_asleep_mask &= ~bit;
			priv->pspoll_mask &= ~bit;
			if (priv->tx_multicast && link_id &&
					!priv->sta_asleep_mask)
				atbm_hw_priv_queue_work(hw_priv,
					&priv->multicast_stop_work);
			atbm_bh_wakeup(hw_priv);
		}
		break;
	}
}

void atbm_sta_notify(struct ieee80211_hw *dev,
		       struct ieee80211_vif *vif,
		       enum sta_notify_cmd notify_cmd,
		       struct ieee80211_sta *sta)
{
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);
	struct atbm_sta_priv *sta_priv =
		(struct atbm_sta_priv *)&sta->drv_priv;

	if(atomic_read(&priv->enabled)==0){
		return;
	}
	if(atbm_bh_is_term(priv->hw_priv)){
		return;
	}
#ifdef P2P_MULTIVIF
	WARN_ON(priv->if_id == ATBM_WIFI_GENERIC_IF_ID);
#endif
	spin_lock_bh(&priv->ps_state_lock);
	__atbm_sta_notify(priv, notify_cmd, sta_priv->link_id);
	spin_unlock_bh(&priv->ps_state_lock);
}

static void atbm_ps_notify(struct atbm_vif *priv,
		      int link_id, bool ps)
{
	if (link_id > ATBMWIFI_MAX_STA_IN_AP_MODE)
		return;

	txrx_printk(KERN_DEBUG "%s for LinkId: %d. STAs asleep: %.8X\n",
			ps ? "Stop" : "Start",
			link_id, priv->sta_asleep_mask);

	/* TODO:COMBO: __atbm_sta_notify changed. */
	__atbm_sta_notify(priv,
		ps ? STA_NOTIFY_SLEEP : STA_NOTIFY_AWAKE, link_id);
}

static int atbm_set_tim_impl(struct atbm_vif *priv, bool aid0_bit_set)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct sk_buff *skb;
	struct wsm_update_ie update_ie = {
		.what = WSM_UPDATE_IE_BEACON,
		.count = 1,
	};
	u16 tim_offset, tim_length;
	
#if 0
	ap_printk(KERN_DEBUG "[AP] %s mcast: %s.\n",
		__func__, aid0_bit_set ? "ena" : "dis");
#endif
#ifdef RESET_CHANGE
	if (atomic_read(&hw_priv->fw_reloading))
	{
		return 0;
		//cmd can not transmit,return now
	}
#endif
	skb = ieee80211_beacon_get_tim(priv->hw, priv->vif,
			&tim_offset, &tim_length);
	if (!skb) {
		if (!__atbm_flush(hw_priv, false, priv->if_id))
			wsm_unlock_tx(hw_priv);
		return -ENOENT;
	}

	if (tim_offset && tim_length >= 6) {
		/* Ignore DTIM count from mac80211:
		 * firmware handles DTIM internally. */
		skb->data[tim_offset + 2] = 0;

		/* Set/reset aid0 bit */
		if (aid0_bit_set)
			skb->data[tim_offset + 4] |= 1;
		else
			skb->data[tim_offset + 4] &= ~1;
	}

	update_ie.ies = &skb->data[tim_offset];
	update_ie.length = tim_length;
	WARN_ON(wsm_update_ie(hw_priv, &update_ie, priv->if_id));

	atbm_dev_kfree_skb(skb);

	return 0;
}

void atbm_set_tim_work(struct work_struct *work)
{
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, set_tim_work);
	if(atbm_bh_is_term(priv->hw_priv)){
		return;
	}
	(void)atbm_set_tim_impl(priv, priv->aid0_bit_set);
}

int atbm_set_tim(struct ieee80211_hw *dev, struct ieee80211_sta *sta,
		   bool set)
{
	struct atbm_sta_priv *sta_priv =
		(struct atbm_sta_priv *)&sta->drv_priv;
	struct atbm_vif *priv = sta_priv->priv;

	if(atomic_read(&priv->enabled)==0){
		return 0;
	}
	if(atbm_bh_is_term(priv->hw_priv)){
		return 0;
	}
#ifdef P2P_MULTIVIF
	WARN_ON(priv->if_id == ATBM_WIFI_GENERIC_IF_ID);
#endif
	WARN_ON(priv->mode != NL80211_IFTYPE_AP);
	atbm_hw_priv_queue_work(priv->hw_priv, &priv->set_tim_work);
	return 0;
}

void atbm_set_cts_work(struct work_struct *work)
{
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, set_cts_work.work);
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);

	u8 erp_ie[3] = {ATBM_WLAN_EID_ERP_INFO, 0x1, 0};
	struct wsm_update_ie update_ie = {
		.what = WSM_UPDATE_IE_BEACON,
		.count = 1,
		.ies = erp_ie,
		.length = 3,
	};
	u32 erp_info;
	__le32 use_cts_prot;
	if(atbm_bh_is_term(hw_priv)){
		return;
	}
	mutex_lock(&hw_priv->conf_mutex);
	erp_info = priv->erp_info;
	mutex_unlock(&hw_priv->conf_mutex);
	use_cts_prot =
		erp_info & WLAN_ERP_USE_PROTECTION ?
		__cpu_to_le32(1) : 0;

	erp_ie[ERP_INFO_BYTE_OFFSET] = erp_info;

	ap_printk(KERN_DEBUG "[STA] ERP information 0x%x\n", erp_info);

	/* TODO:COMBO: If 2 interfaces are on the same channel they share
	the same ERP values */
	WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_NON_ERP_PROTECTION,
				&use_cts_prot, sizeof(use_cts_prot),
				priv->if_id));
#if defined(CONFIG_NL80211_TESTMODE) || defined(CONFIG_ATBM_IOCTRL)
	{
		extern int atbm_tool_use_cts_prot;
		atbm_tool_use_cts_prot = use_cts_prot;
	}
#endif
	/* If STA Mode update_ie is not required */
	if (priv->mode != NL80211_IFTYPE_STATION) {
		WARN_ON(wsm_update_ie(hw_priv, &update_ie, priv->if_id));
	}

	return;
}

static int atbm_set_btcoexinfo(struct atbm_vif *priv)
{
	struct wsm_override_internal_txrate arg;
	int ret = 0;

	if (priv->mode == NL80211_IFTYPE_STATION) {
		/* Plumb PSPOLL and NULL template */
		WARN_ON(atbm_upload_pspoll(priv));
		WARN_ON(atbm_upload_null(priv));
	} else {
		return 0;
	}

	memset(&arg, 0, sizeof(struct wsm_override_internal_txrate));

	if (!priv->vif->p2p) {
		/* STATION mode */
		if (priv->bss_params.operationalRateSet & ~0xF) {
			ap_printk(KERN_DEBUG "[STA] STA has ERP rates\n");
			/* G or BG mode */
			arg.internalTxRate = (__ffs(
			priv->bss_params.operationalRateSet & ~0xF));
		} else {
			ap_printk(KERN_DEBUG "[STA] STA has non ERP rates\n");
			/* B only mode */
			arg.internalTxRate = (__ffs(
			priv->association_mode.basicRateSet));
		}
		arg.nonErpInternalTxRate = (__ffs(
			priv->association_mode.basicRateSet));
	} else {
		/* P2P mode */
		arg.internalTxRate = (__ffs(
			priv->bss_params.operationalRateSet & ~0xF));
		arg.nonErpInternalTxRate = (__ffs(
			priv->bss_params.operationalRateSet & ~0xF));
	}

	ap_printk(KERN_DEBUG "[STA] BTCOEX_INFO"
		"MODE %d, internalTxRate : %x, nonErpInternalTxRate: %x\n",
		priv->mode,
		arg.internalTxRate,
		arg.nonErpInternalTxRate);

	ret = WARN_ON(wsm_write_mib(ABwifi_vifpriv_to_hwpriv(priv),
		WSM_MIB_ID_OVERRIDE_INTERNAL_TX_RATE,
		&arg, sizeof(arg), priv->if_id));

	return ret;
}

void atbm_ibss_join_work(struct atbm_vif *priv)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	const u8 *bssid;
	struct cfg80211_bss *bss;
	struct wsm_protected_mgmt_policy mgmt_policy;
	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};
	struct wsm_join join = {
                .mode = WSM_JOIN_MODE_IBSS,
                .preambleType = WSM_JOIN_PREAMBLE_SHORT,
                .probeForJoin = 1,
                /* dtimPeriod will be updated after association */
                .dtimPeriod = 1,
//                .beaconInterval = bss->beacon_interval,
        };



	if (unlikely(priv->join_status)) {
		wsm_lock_tx(hw_priv);
		mutex_unlock(&hw_priv->conf_mutex);
		atbm_unjoin_work(&priv->unjoin_work);
		mutex_lock(&hw_priv->conf_mutex);
	}

	bssid = priv->vif->bss_conf.bssid;
	bss = ieee80211_atbm_get_bss(hw_priv->hw->wiphy, hw_priv->channel,
			bssid, NULL, 0, 0, 0);
	if (!bss) {
		wsm_unlock_tx(hw_priv);
		return;
	}
	join.beaconInterval = bss->beacon_interval;
	if (priv->if_id)
		join.flags |= WSM_FLAG_MAC_INSTANCE_1;
	else
		join.flags &= ~WSM_FLAG_MAC_INSTANCE_1;

	if(priv->tmpframe_probereq_set==0){
		join.probeForJoin = 0;
		//hw_priv->channel->hw_value = 6;
		//join.channelNumber = 0;
		printk("<atbm_WIFI>because not scan before join, probeForJoin =0 hw_value %d\n",hw_priv->channel->hw_value);
	}
	/* BT Coex related changes */
	if (hw_priv->is_BT_Present) {
		if (((hw_priv->conf_listen_interval * 100) %
				bss->beacon_interval) == 0)
			priv->listen_interval =
				((hw_priv->conf_listen_interval * 100) /
				bss->beacon_interval);
		else
			priv->listen_interval =
				((hw_priv->conf_listen_interval * 100) /
				bss->beacon_interval + 1);
	}

	if (priv->hw->conf.ps_dtim_period) {
		priv->join_dtim_period = priv->hw->conf.ps_dtim_period;
	}
	join.dtimPeriod = priv->join_dtim_period;
	priv->beacon_int = bss->beacon_interval;
	ap_printk(KERN_DEBUG "[STA] Join DTIM: %d, interval: %d\n",
			join.dtimPeriod, priv->beacon_int);

	hw_priv->is_go_thru_go_neg = false;
	join.channelNumber = hw_priv->channel->hw_value;

	/* basicRateSet will be updated after association.
	Currently these values are hardcoded */
	if (hw_priv->channel->band == IEEE80211_BAND_5GHZ) {
		join.band = WSM_PHY_BAND_5G;
		join.basicRateSet = 64; /*6 mbps*/
	}else{
		join.band = WSM_PHY_BAND_2_4G;
		join.basicRateSet = 7; /*1, 2, 5.5 mbps*/
	}
	memcpy(&join.bssid[0], bssid, sizeof(join.bssid));
	memcpy(&priv->join_bssid[0], bssid, sizeof(priv->join_bssid));

	if (priv->vif->p2p) {
		join.flags |= WSM_JOIN_FLAGS_P2P_GO;
#ifdef P2P_MULTIVIF
		join.flags |= (1 << 6);
#endif
		join.basicRateSet =
			atbm_rate_mask_to_wsm(hw_priv, 0xFF0);
	}
#ifdef ATBM_SUPPORT_WIDTH_40M		
		if(priv->if_id)
		{
			join.channel_type = (u32)(hw_priv->channel_type>NL80211_CHAN_NO_HT ? 
									  NL80211_CHAN_HT20 : NL80211_CHAN_NO_HT);
		}
		else
		{
			spin_lock_bh(&hw_priv->spinlock_channel_type);
			join.channel_type = (u32)hw_priv->channel_type;
			spin_unlock_bh(&hw_priv->spinlock_channel_type);
		}
#endif
	wsm_flush_tx(hw_priv);

	/* Queue unjoin if not associated in 3 sec. */
	atbm_hw_priv_queue_delayed_work(hw_priv,
		&priv->join_timeout, 3 * HZ);
	#ifdef CONFIG_PM
	/*Stay Awake for Join Timeout*/
	atbm_pm_stay_awake(&hw_priv->pm_state, 3 * HZ);
	#endif

	atbm_disable_listening(priv);

	WARN_ON(wsm_set_operational_mode(hw_priv, &mode, priv->if_id));
	WARN_ON(wsm_set_block_ack_policy(hw_priv,
		/*hw_priv->ba_tid_mask*/0, hw_priv->ba_tid_rx_mask, priv->if_id));
	spin_lock_bh(&hw_priv->ba_lock);
	hw_priv->ba_ena = false;
	hw_priv->ba_cnt = 0;
	hw_priv->ba_acc = 0;
	hw_priv->ba_hist = 0;
	hw_priv->ba_cnt_rx = 0;
	hw_priv->ba_acc_rx = 0;
	spin_unlock_bh(&hw_priv->ba_lock);

	mgmt_policy.protectedMgmtEnable = 0;
	mgmt_policy.unprotectedMgmtFramesAllowed = 1;
	mgmt_policy.encryptionForAuthFrame = 1;
	wsm_set_protected_mgmt_policy(hw_priv, &mgmt_policy,
				      priv->if_id);

	if (wsm_join(hw_priv, &join, priv->if_id)) {
		memset(&priv->join_bssid[0],
			0, sizeof(priv->join_bssid));

		atbm_cancle_delayed_work(&priv->join_timeout,true);
	} else {
		/* Upload keys */
		priv->join_status = ATBM_APOLLO_JOIN_STATUS_IBSS;
		atbm_upload_keys(priv);

		/* Due to beacon filtering it is possible that the
		 * AP's beacon is not known for the mac80211 stack.
		 * Disable filtering temporary to make sure the stack
		 * receives at least one */
		priv->disable_beacon_filter = true;

	}
	atbm_update_filtering(priv);
	ieee80211_atbm_put_bss(hw_priv->hw->wiphy, bss);
	wsm_unlock_tx(hw_priv);
}
void atbm_bss_info_changed(struct ieee80211_hw *dev,
			     struct ieee80211_vif *vif,
			     struct ieee80211_bss_conf *info,
			     u32 changed)
{
	struct atbm_common *hw_priv = dev->priv;
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);
	bool do_ibss_join = false;

#ifdef P2P_MULTIVIF
	if (priv->if_id == ATBM_WIFI_GENERIC_IF_ID)
		return;
#endif

#ifdef RESET_CHANGE
	if(atomic_read(&hw_priv->fw_reloading))
	{
		printk("%s:hw_priv->reset_flag lock \n",__func__);
		return;
	}
#endif
	if(atbm_bh_is_term(hw_priv)){
		return;
	}
	mutex_lock(&hw_priv->conf_mutex);
	if (changed & BSS_CHANGED_BSSID) {
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
		spin_lock_bh(&hw_priv->tsm_lock);
		if (hw_priv->tsm_info.sta_associated) {
			unsigned now = jiffies;
			hw_priv->tsm_info.sta_roamed = 1;
			if ((now - hw_priv->tsm_info.txconf_timestamp_vo) >
			    (now - hw_priv->tsm_info.rx_timestamp_vo))
				hw_priv->tsm_info.use_rx_roaming = 1;
		} else {
			hw_priv->tsm_info.sta_associated = 1;
		}
		spin_unlock_bh(&hw_priv->tsm_lock);
#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
		memcpy(priv->bssid, info->bssid, ETH_ALEN);
		atbm_setup_mac_pvif(priv);
		if (info->ibss_joined)
			do_ibss_join = true;
	}

	/* TODO: BSS_CHANGED_IBSS */

	if (changed & BSS_CHANGED_ARP_FILTER) {
		struct wsm_arp_ipv4_filter filter = {0};
		int i;
		ap_printk(KERN_DEBUG "[STA] BSS_CHANGED_ARP_FILTER "
				     "enabled: %d, cnt: %d\n",
				     info->arp_filter_enabled,
				     info->arp_addr_cnt);

		if (info->arp_filter_enabled){
                        if (vif->type == NL80211_IFTYPE_STATION)
                                filter.enable = (u32)ATBM_APOLLO_ENABLE_ARP_FILTER_OFFLOAD;
                        else if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                                filter.enable = (u32)(1<<1);
                        else
                                filter.enable = 0;
                }

		/* Currently only one IP address is supported by firmware.
		 * In case of more IPs arp filtering will be disabled. */
		if (info->arp_addr_cnt > 0 &&
		    info->arp_addr_cnt <= WSM_MAX_ARP_IP_ADDRTABLE_ENTRIES) {
			for (i = 0; i < info->arp_addr_cnt; i++) {
				filter.ipv4Address[i] = info->arp_addr_list[i];
				ap_printk(KERN_DEBUG "[STA] addr[%d]: 0x%X\n",
					  i, filter.ipv4Address[i]);
			}
		} else
			filter.enable = 0;
		ap_printk(KERN_DEBUG "[STA] arp ip filter enable: %d\n",
			  __le32_to_cpu(filter.enable));
		printk("[STA] arp ip filter enable: %d\n",
			  __le32_to_cpu(filter.enable));

		if (filter.enable){
			LOCAL_SET_CONNECT_STOP(hw_to_local(hw_priv->hw));
            atbm_set_arpreply(dev, vif);
		}

                priv->filter4.enable = filter.enable;
#ifdef ATBM_PKG_REORDER
		priv->filter4.enable = 0;
#endif
		if (wsm_set_arp_ipv4_filter(hw_priv, &filter, priv->if_id))
			WARN_ON(1);

		if (filter.enable &&
			(priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA)) {
			/* Firmware requires that value for this 1-byte field must
			 * be specified in units of 500us. Values above the 128ms
			 * threshold are not supported. */
			if (info->dynamic_ps_timeout >= 0x80)
				priv->powersave_mode.fastPsmIdlePeriod = 0xFF;
			else
				priv->powersave_mode.fastPsmIdlePeriod =
					info->dynamic_ps_timeout << 1;

			if (priv->setbssparams_done) {
				struct wsm_set_pm pm = priv->powersave_mode;
				int ret = 0;
				if (priv->user_power_set_true)
					 priv->powersave_mode.pmMode = priv->user_pm_mode;
				else if ((priv->power_set_true &&
					((priv->powersave_mode.pmMode == WSM_PSM_ACTIVE) ||
                                        (priv->powersave_mode.pmMode == WSM_PSM_PS))) ||
					!priv->power_set_true)
						priv->powersave_mode.pmMode = WSM_PSM_FAST_PS;
				
				#ifdef ATBM_WIFI_NO_P2P_PS
				if(priv->vif->p2p)
					priv->powersave_mode.pmMode = WSM_PSM_ACTIVE;
				#endif
				
				ret = atbm_set_pm (priv, &priv->powersave_mode);
				if(ret)
					priv->powersave_mode = pm;
			} else
				priv->powersave_mode.pmMode = WSM_PSM_FAST_PS;
			priv->power_set_true = 0;
			priv->user_power_set_true = 0;
		}

	}

#ifdef IPV6_FILTERING
	if (changed & BSS_CHANGED_NDP_FILTER) {
		struct wsm_ndp_ipv6_filter filter = {0};
		int i;
		u16 *ipv6addr = NULL;

		ap_printk(KERN_DEBUG "[STA] BSS_CHANGED_NDP_FILTER "
				     "enabled: %d, cnt: %d\n",
				     info->ndp_filter_enabled,
				     info->ndp_addr_cnt);

		if (info->ndp_filter_enabled) {
			if (vif->type == NL80211_IFTYPE_STATION)
				filter.enable = (u32)ATBM_APOLLO_ENABLE_NDP_FILTER_OFFLOAD;
			else if ((vif->type == NL80211_IFTYPE_AP))
				filter.enable = (u32)(1<<1);
			else
				filter.enable = 0;
		}

		/* Currently only one IP address is supported by firmware.
		 * In case of more IPs ndp filtering will be disabled. */
		if (info->ndp_addr_cnt > 0 &&
		    info->ndp_addr_cnt <= WSM_MAX_NDP_IP_ADDRTABLE_ENTRIES) {
			for (i = 0; i < info->ndp_addr_cnt; i++) {
				priv->filter6.ipv6Address[i] = filter.ipv6Address[i] = info->ndp_addr_list[i];
				ipv6addr = (u16 *)(&filter.ipv6Address[i]);
				ap_printk(KERN_DEBUG "[STA] ipv6 addr[%d]: %x:%x:%x:%x:%x:%x:%x:%x\n", \
									i, cpu_to_be16(*(ipv6addr + 0)), cpu_to_be16(*(ipv6addr + 1)), \
									cpu_to_be16(*(ipv6addr + 2)), cpu_to_be16(*(ipv6addr + 3)), \
									cpu_to_be16(*(ipv6addr + 4)), cpu_to_be16(*(ipv6addr + 5)), \
									cpu_to_be16(*(ipv6addr + 6)), cpu_to_be16(*(ipv6addr + 7)));
			}
		} else {
			filter.enable = 0;
			for (i = 0; i < info->ndp_addr_cnt; i++) {
				ipv6addr = (u16 *)(&info->ndp_addr_list[i]);
				ap_printk(KERN_DEBUG "[STA] ipv6 addr[%d]: %x:%x:%x:%x:%x:%x:%x:%x\n", \
									i, cpu_to_be16(*(ipv6addr + 0)), cpu_to_be16(*(ipv6addr + 1)), \
									cpu_to_be16(*(ipv6addr + 2)), cpu_to_be16(*(ipv6addr + 3)), \
									cpu_to_be16(*(ipv6addr + 4)), cpu_to_be16(*(ipv6addr + 5)), \
									cpu_to_be16(*(ipv6addr + 6)), cpu_to_be16(*(ipv6addr + 7)));
			}
		}

		ap_printk(KERN_DEBUG "[STA] ndp ip filter enable: %d\n",
			  __le32_to_cpu(filter.enable));

		if (filter.enable)
			atbm_set_na(dev, vif);

		priv->filter6.enable = filter.enable;

		if (wsm_set_ndp_ipv6_filter(hw_priv, &filter, priv->if_id))
			WARN_ON(1);
#if 0 /*Commented out to disable Power Save in IPv6*/
		if (filter.enable && (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA) && (priv->vif->p2p) &&
				!(priv->firmware_ps_mode.pmMode & WSM_PSM_FAST_PS)) {
			if(priv->setbssparams_done) {
				struct wsm_set_pm pm = priv->powersave_mode;
				int ret = 0;

				priv->powersave_mode.pmMode = WSM_PSM_FAST_PS;
				ret = atbm_set_pm(priv, &priv->powersave_mode);
				if(ret) {
					priv->powersave_mode = pm;
				}
			} else {
				priv->powersave_mode.pmMode = WSM_PSM_FAST_PS;
			}
		}
#endif
	}
#endif /*IPV6_FILTERING*/

	if (changed & BSS_CHANGED_BEACON) {
		ap_printk(KERN_DEBUG "BSS_CHANGED_BEACON\n");
#ifdef HIDDEN_SSID
		if(priv->join_status != ATBM_APOLLO_JOIN_STATUS_AP)
		{
      priv->hidden_ssid = info->hidden_ssid;
      priv->ssid_length = info->ssid_len;
      memcpy(priv->ssid, info->ssid, info->ssid_len);
		}
		else
      ap_printk(KERN_DEBUG "priv->join_status=%d\n",priv->join_status);
#endif
		WARN_ON(atbm_upload_beacon(priv));
		WARN_ON(atbm_update_beaconing(priv));
	}

	if (changed & BSS_CHANGED_IBSS)
	{
		priv->beacon_int = info->beacon_int;
		atbm_upload_beacon(priv);
		WARN_ON(atbm_update_beaconing(priv));
	}
	if (changed & BSS_CHANGED_BEACON_ENABLED) {
		ap_printk(KERN_DEBUG "BSS_CHANGED_BEACON_ENABLED dummy\n");
		if (priv->enable_beacon != info->enable_beacon) {
		priv->enable_beacon = info->enable_beacon;
		}
	}

	if (changed & BSS_CHANGED_BEACON_INT) {
		ap_printk(KERN_DEBUG "CHANGED_BEACON_INT\n");
		/* Restart AP only when connected */
		if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
		{
			WARN_ON(atbm_update_beaconing(priv));
		}
		else if (info->ibss_joined)
		{
			do_ibss_join = true;
		}
	}

	if (changed & BSS_CHANGED_ASSOC) {
		wsm_lock_tx(hw_priv);
		priv->wep_default_key_id = -1;
		wsm_unlock_tx(hw_priv);

		if (!info->assoc /* && !info->ibss_joined */) {
			priv->cqm_link_loss_count = 40;
			priv->cqm_beacon_loss_count = 20;
			priv->cqm_tx_failure_thold = 0;
		}
		priv->cqm_tx_failure_count = 0;
	}
	/* TODO: BSS_CHANGED_IBSS */
	if (do_ibss_join)
	{
		wsm_lock_tx(hw_priv);
		atbm_ibss_join_work(priv);
	}
	if (changed & BSS_CHANGED_IBSS)
	{
		if (!info->assoc) {
			wsm_lock_tx(hw_priv);
			atbm_ibss_join_work(priv);
		}
	}

	if (changed &
	    (BSS_CHANGED_ASSOC |
	     BSS_CHANGED_BASIC_RATES |
	     BSS_CHANGED_ERP_PREAMBLE |
	     BSS_CHANGED_HT |
	     BSS_CHANGED_ERP_SLOT |
	     BSS_CHANGED_IBSS)) {
		int is_combo = 0;
		int i;
		struct atbm_vif *tmp_priv;
		printk(KERN_DEBUG "BSS_CHANGED_ASSOC.changed(%x)\n",changed);
		if (info->assoc || info->ibss_joined) { /* TODO: ibss_joined */
			struct ieee80211_sta *sta = NULL;
			if (info->dtim_period)
				priv->join_dtim_period = info->dtim_period;
			priv->beacon_int = info->beacon_int;

			/* Associated: kill join timeout */
			mutex_unlock(&hw_priv->conf_mutex);
			atbm_cancle_delayed_work(&priv->join_timeout,false);
			mutex_lock(&hw_priv->conf_mutex);

			rcu_read_lock();
			if (info->bssid && !info->ibss_joined)
				sta = ieee80211_find_sta(vif, info->bssid);
			if (sta) {
				/* TODO:COMBO:Change this once
				* mac80211 changes are available */
				//printk("%s:channel_type(%d),changed(%x)\n",__func__,info->channel_type,changed);
				BUG_ON(!hw_priv->channel);
				hw_priv->ht_info.ht_cap = sta->ht_cap;
				priv->bss_params.operationalRateSet =
					__cpu_to_le32(
					atbm_rate_mask_to_wsm(hw_priv,
					sta->supp_rates[
						hw_priv->channel->band]));
				hw_priv->ht_info.channel_type =
						info->channel_type;
				hw_priv->ht_info.operation_mode =
						info->ht_operation_mode;
			} else {
				memset(&hw_priv->ht_info, 0,
						sizeof(hw_priv->ht_info));
				priv->bss_params.operationalRateSet = -1;
			}
			rcu_read_unlock();
			priv->htcap = (sta && atbm_is_ht(&hw_priv->ht_info));
			atbm_for_each_vif(hw_priv, tmp_priv, i) {
#ifdef P2P_MULTIVIF
				if ((i == (ATBM_WIFI_MAX_VIFS - 1)) || !tmp_priv)
#else
				if (!tmp_priv)
#endif
					continue;
				if (tmp_priv->join_status >= ATBM_APOLLO_JOIN_STATUS_STA)
					is_combo++;
			}

			if (is_combo > 1) {
				hw_priv->vif0_throttle = ATBM_WIFI_HOST_VIF0_11BG_THROTTLE;
				hw_priv->vif1_throttle = ATBM_WIFI_HOST_VIF1_11BG_THROTTLE;
				printk(KERN_ERR "ASSOC is_combo %d\n",hw_priv->vif0_throttle);
			} else if ((priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA) &&
					priv->htcap) {
				hw_priv->vif0_throttle = ATBM_WIFI_HOST_VIF0_11N_THROTTLE;
				hw_priv->vif1_throttle = ATBM_WIFI_HOST_VIF1_11N_THROTTLE;
				//printk(KERN_ERR "ASSOC HTCAP 11N %d\n",hw_priv->vif0_throttle);
			} else {
				hw_priv->vif0_throttle = ATBM_WIFI_HOST_VIF0_11BG_THROTTLE;
				hw_priv->vif1_throttle = ATBM_WIFI_HOST_VIF1_11BG_THROTTLE;
				printk(KERN_ERR "ASSOC not_combo 11BG %d\n",hw_priv->vif0_throttle);
			}

			if (sta) {
				__le32 val = 0;
				if (hw_priv->ht_info.operation_mode &
				IEEE80211_HT_OP_MODE_NON_GF_STA_PRSNT) {
					ap_printk(KERN_DEBUG"[STA]"
						" Non-GF STA present\n");
					/* Non Green field capable STA */
					val = __cpu_to_le32(BIT(1));
				}
#if 0
				wsm_write_mib(hw_priv,WSM_MIB_ID_SET_HT_PROTECTION,
					&val, sizeof(val), priv->if_id);
#endif
			}

			priv->association_mode.greenfieldMode =
				atbm_ht_greenfield(&hw_priv->ht_info);
			priv->association_mode.flags =
				WSM_ASSOCIATION_MODE_SNOOP_ASSOC_FRAMES |
				WSM_ASSOCIATION_MODE_USE_PREAMBLE_TYPE |
				WSM_ASSOCIATION_MODE_USE_HT_MODE |
				WSM_ASSOCIATION_MODE_USE_BASIC_RATE_SET |
				WSM_ASSOCIATION_MODE_USE_MPDU_START_SPACING;
			priv->association_mode.preambleType =
				info->use_short_preamble ?
				WSM_JOIN_PREAMBLE_SHORT :
				WSM_JOIN_PREAMBLE_LONG;
#if defined(CONFIG_NL80211_TESTMODE) || defined(CONFIG_ATBM_IOCTRL)
			{
				extern int atbm_tool_use_short_preamble;
				atbm_tool_use_short_preamble = priv->association_mode.preambleType;
			}
#endif
			priv->association_mode.basicRateSet = __cpu_to_le32(
				atbm_rate_mask_to_wsm(hw_priv,
				info->basic_rates));
			priv->association_mode.mpduStartSpacing =
				atbm_ht_ampdu_density(&hw_priv->ht_info);

			priv->cqm_beacon_loss_count =
					info->cqm_beacon_miss_thold;
			priv->cqm_tx_failure_thold =
					info->cqm_tx_fail_thold;
			priv->cqm_tx_failure_count = 0;
			atbm_cancle_delayed_work(&priv->bss_loss_work,false);
			atbm_cancle_delayed_work(&priv->connection_loss_work,false);
			atbm_cancle_delayed_work(&priv->dhcp_retry_work,false);

			priv->bss_params.beaconLostCount =
					priv->cqm_beacon_loss_count ?
					priv->cqm_beacon_loss_count :
					priv->cqm_link_loss_count;

			priv->bss_params.aid = info->aid;

			if (priv->join_dtim_period < 1)
				priv->join_dtim_period = 1;

			ap_printk(KERN_DEBUG "[STA] DTIM %d, interval: %d\n",
				priv->join_dtim_period, priv->beacon_int);
			ap_printk("[STA] Preamble: %d, " \
				"Greenfield: %d, Aid: %d, " \
				"Rates: 0x%.8X, Basic: 0x%.8X,mpduStartSpacing %d\n",
				priv->association_mode.preambleType,
				priv->association_mode.greenfieldMode,
				priv->bss_params.aid,
				priv->bss_params.operationalRateSet,
				priv->association_mode.basicRateSet,
				priv->association_mode.mpduStartSpacing);
			WARN_ON(wsm_set_association_mode(hw_priv,
				&priv->association_mode, priv->if_id));
			if (!info->ibss_joined)
			{
			WARN_ON(wsm_keep_alive_period(hw_priv,
				ATBM_APOLLO_KEEP_ALIVE_PERIOD /* sec */,
				priv->if_id));
			WARN_ON(wsm_set_bss_params(hw_priv, &priv->bss_params,
				priv->if_id));
			priv->setbssparams_done = true;
			WARN_ON(wsm_set_beacon_wakeup_period(hw_priv,
				    priv->beacon_int * priv->join_dtim_period >
				    MAX_BEACON_SKIP_TIME_MS ? 1 :
				    priv->join_dtim_period, 0, priv->if_id));
			printk("%s:priv->htcap(%d)\n",__func__,priv->htcap);
			if (priv->htcap) {
				wsm_lock_tx(hw_priv);
				/* Statically enabling block ack for TX/RX */
				WARN_ON(wsm_set_block_ack_policy(hw_priv,
					hw_priv->ba_tid_tx_mask, hw_priv->ba_tid_rx_mask,
						priv->if_id));
				wsm_unlock_tx(hw_priv);
			}

			atbm_set_pm(priv, &priv->powersave_mode);
			}
			if (priv->vif->p2p) {
				ap_printk(KERN_DEBUG
					"[STA] Setting p2p powersave "
					"configuration.\n");
				WARN_ON(wsm_set_p2p_ps_modeinfo(hw_priv,
					&priv->p2p_ps_modeinfo, priv->if_id));
				atbm_notify_noa(priv, ATBM_APOLLO_NOA_NOTIFICATION_DELAY);
			}

			if (priv->mode == NL80211_IFTYPE_STATION)
				WARN_ON(atbm_upload_qosnull(priv));

			if (hw_priv->is_BT_Present)
				WARN_ON(atbm_set_btcoexinfo(priv));
		} else {
			memset(&priv->association_mode, 0,
				sizeof(priv->association_mode));
			memset(&priv->bss_params, 0, sizeof(priv->bss_params));
		}
	}
	if (changed & (BSS_CHANGED_ASSOC | BSS_CHANGED_ERP_CTS_PROT)) {
		u32 prev_erp_info = priv->erp_info;

		if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP) {
			if (info->use_cts_prot)
				priv->erp_info |= WLAN_ERP_USE_PROTECTION;
			else if (!(prev_erp_info & WLAN_ERP_NON_ERP_PRESENT))
				priv->erp_info &= ~WLAN_ERP_USE_PROTECTION;

			if (prev_erp_info != priv->erp_info)
				atbm_hw_priv_queue_delayed_work(hw_priv,
						&priv->set_cts_work, 0*HZ);
		}
	}

	if (changed & (BSS_CHANGED_ASSOC | BSS_CHANGED_ERP_SLOT)) {
		__le32 slot_time = info->use_short_slot ?
			__cpu_to_le32(9) : __cpu_to_le32(20);
		ap_printk(KERN_DEBUG "[STA] Slot time :%d us.\n",
			__le32_to_cpu(slot_time));

		//add by wp fix PHICOMM AP4  long slot  bug
		if(priv->htcap){
			 slot_time = 	__cpu_to_le32(9);
		}
		printk( "[STA] Slot time :%d us.\n",__le32_to_cpu(slot_time));
		WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_SLOT_TIME,
			&slot_time, sizeof(slot_time), priv->if_id));
		
#if defined(CONFIG_NL80211_TESTMODE) || defined(CONFIG_ATBM_IOCTRL)
		{
			extern int atbm_tool_use_short_slot;
			atbm_tool_use_short_slot = (slot_time == __cpu_to_le32(9))? 1:0;
		}
#endif
	}
	if (changed & (BSS_CHANGED_ASSOC | BSS_CHANGED_CQM)) {
		struct wsm_rcpi_rssi_threshold threshold = {
			.rollingAverageCount = 8,
		};

		ap_printk(KERN_DEBUG "[CQM] RSSI threshold "
			"subscribe: %d +- %d\n",
			info->cqm_rssi_thold, info->cqm_rssi_hyst);
		ap_printk(KERN_DEBUG "[CQM] Beacon loss subscribe: %d\n",
			info->cqm_beacon_miss_thold);
		ap_printk(KERN_DEBUG "[CQM] TX failure subscribe: %d\n",
			info->cqm_tx_fail_thold);
		priv->cqm_rssi_thold = info->cqm_rssi_thold;
		priv->cqm_rssi_hyst = info->cqm_rssi_hyst;
		if (info->cqm_rssi_thold || info->cqm_rssi_hyst) {
			/* RSSI subscription enabled */
			/* TODO: It's not a correct way of setting threshold.
			 * Upper and lower must be set equal here and adjusted
			 * in callback. However current implementation is much
			 * more relaible and stable. */
			if (priv->cqm_use_rssi) {
				threshold.upperThreshold =
					info->cqm_rssi_thold +
					info->cqm_rssi_hyst;
				threshold.lowerThreshold =
					info->cqm_rssi_thold;
			} else {
				/* convert RSSI to RCPI
				 * RCPI = (RSSI + 110) * 2 */
				threshold.upperThreshold =
					(info->cqm_rssi_thold +
					 info->cqm_rssi_hyst + 110) * 2;
				threshold.lowerThreshold =
					(info->cqm_rssi_thold + 110) * 2;
			}
			threshold.rssiRcpiMode |=
				WSM_RCPI_RSSI_THRESHOLD_ENABLE;
		} else {
			/* There is a bug in FW, see sta.c. We have to enable
			 * dummy subscription to get correct RSSI values. */
			threshold.rssiRcpiMode |=
				WSM_RCPI_RSSI_THRESHOLD_ENABLE |
				WSM_RCPI_RSSI_DONT_USE_UPPER |
				WSM_RCPI_RSSI_DONT_USE_LOWER;
		}
		WARN_ON(wsm_set_rcpi_rssi_threshold(hw_priv, &threshold,
					priv->if_id));

		priv->cqm_tx_failure_thold = info->cqm_tx_fail_thold;
		priv->cqm_tx_failure_count = 0;

		if (priv->cqm_beacon_loss_count !=
				info->cqm_beacon_miss_thold) {
			priv->cqm_beacon_loss_count =
				info->cqm_beacon_miss_thold;
			priv->bss_params.beaconLostCount =
				priv->cqm_beacon_loss_count ?
				priv->cqm_beacon_loss_count :
				priv->cqm_link_loss_count;
			/* Make sure we are associated before sending
			 * set_bss_params to firmware */
			if (priv->bss_params.aid) {
				WARN_ON(wsm_set_bss_params(hw_priv,
							   &priv->bss_params,
							   priv->if_id));
				priv->setbssparams_done = true;
			}
		}
	}

	if (changed & BSS_CHANGED_PS) {
		if (info->ps_enabled == false)
			priv->powersave_mode.pmMode = WSM_PSM_ACTIVE;
		else if (info->dynamic_ps_timeout <= 0)
			priv->powersave_mode.pmMode = WSM_PSM_PS;
		else
			priv->powersave_mode.pmMode = WSM_PSM_FAST_PS;

		ap_printk(KERN_DEBUG "[STA] Aid: %d, Joined: %s, Powersave: %s\n",
			priv->bss_params.aid,
			priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA ? "yes" : "no",
			priv->powersave_mode.pmMode == WSM_PSM_ACTIVE ? "WSM_PSM_ACTIVE" :
			priv->powersave_mode.pmMode == WSM_PSM_PS ? "WSM_PSM_PS" :
			priv->powersave_mode.pmMode == WSM_PSM_FAST_PS ? "WSM_PSM_FAST_PS" :
			"UNKNOWN");

		/* Firmware requires that value for this 1-byte field must
		 * be specified in units of 500us. Values above the 128ms
		 * threshold are not supported. */
		if (info->dynamic_ps_timeout >= 0x80)
			priv->powersave_mode.fastPsmIdlePeriod = 0xFF;
		else
			priv->powersave_mode.fastPsmIdlePeriod =
					info->dynamic_ps_timeout << 1;

		#ifdef ATBM_WIFI_NO_P2P_PS
		if(priv->vif->p2p)
			priv->powersave_mode.pmMode = WSM_PSM_ACTIVE;
		#endif
		
		if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA &&
				priv->bss_params.aid &&
				priv->setbssparams_done &&
				priv->filter4.enable)
			atbm_set_pm(priv, &priv->powersave_mode);
		else
			priv->power_set_true = 1;
	}

	if (changed & BSS_CHANGED_RETRY_LIMITS) {
		ap_printk(KERN_DEBUG "[AP] Retry limits: %d (long), " \
			"%d (short).\n", info->retry_long, info->retry_short);
		spin_lock_bh(&hw_priv->tx_policy_cache.lock);
		/*TODO:COMBO: for now it's still handled per hw and kept
		 * in atbm_common */
		hw_priv->long_frame_max_tx_count = info->retry_long;
		hw_priv->short_frame_max_tx_count =
			(info->retry_short < 0x0F) ?
			info->retry_short : 0x0F;
		hw_priv->hw->max_rate_tries = hw_priv->short_frame_max_tx_count;
		spin_unlock_bh(&hw_priv->tx_policy_cache.lock);
		/* TBD: I think we don't need tx_policy_force_upload().
		 * Outdated policies will leave cache in a normal way. */
		/* WARN_ON(tx_policy_force_upload(priv)); */
	}
	if (changed & BSS_CHANGED_P2P_PS) {
		struct wsm_p2p_ps_modeinfo *modeinfo;
		modeinfo = &priv->p2p_ps_modeinfo;
		ap_printk(KERN_DEBUG "[AP] BSS_CHANGED_P2P_PS\n");
		ap_printk(KERN_DEBUG "[AP] Legacy PS: %d for AID %d "
			"in %d mode.\n", info->p2p_ps.legacy_ps,
			priv->bss_params.aid, priv->join_status);

		if (info->p2p_ps.legacy_ps >= 0) {
			if (info->p2p_ps.legacy_ps > 0)
				priv->powersave_mode.pmMode = WSM_PSM_PS;
			else
				priv->powersave_mode.pmMode = WSM_PSM_ACTIVE;

			if(info->p2p_ps.ctwindow && info->p2p_ps.opp_ps)
				priv->powersave_mode.pmMode = WSM_PSM_PS;
			if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA)
				atbm_set_pm(priv, &priv->powersave_mode);
		}

		printk(KERN_DEBUG "[AP] CTWindow: %d\n",
			info->p2p_ps.ctwindow);
		if (info->p2p_ps.ctwindow >= 128)
			modeinfo->oppPsCTWindow = 127;
		else if (info->p2p_ps.ctwindow >= 0)
			modeinfo->oppPsCTWindow = info->p2p_ps.ctwindow;

		printk(KERN_DEBUG "[AP] Opportunistic: %d\n",
			info->p2p_ps.opp_ps);
		switch (info->p2p_ps.opp_ps) {
		case 0:
			modeinfo->oppPsCTWindow &= ~(BIT(7));
			break;
		case 1:
			modeinfo->oppPsCTWindow |= BIT(7);
			break;
		default:
			break;
		}

		printk(KERN_DEBUG "[AP] NOA: %d, %d, %d, %d\n",
			info->p2p_ps.count,
			info->p2p_ps.start,
			info->p2p_ps.duration,
			info->p2p_ps.interval);
		/* Notice of Absence */
		modeinfo->count = info->p2p_ps.count;

		if (info->p2p_ps.count) {
			/* In case P2P_GO we need some extra time to be sure
			 * we will update beacon/probe_resp IEs correctly */
#define NOA_DELAY_START_MS	300
			if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
				modeinfo->startTime =
					__cpu_to_le32(info->p2p_ps.start +
						      NOA_DELAY_START_MS);
			else
				modeinfo->startTime =
					__cpu_to_le32(info->p2p_ps.start);
			modeinfo->duration =
				__cpu_to_le32(info->p2p_ps.duration);
			modeinfo->interval =
				 __cpu_to_le32(info->p2p_ps.interval);
			modeinfo->dtimCount = 1;
			modeinfo->reserved = 0;
		} else {
			modeinfo->dtimCount = 0;
			modeinfo->startTime = 0;
			modeinfo->reserved = 0;
			modeinfo->duration = 0;
			modeinfo->interval = 0;
		}

		if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA ||
		    priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP) {
			WARN_ON(wsm_set_p2p_ps_modeinfo(hw_priv, modeinfo,
				priv->if_id));
		}

		/* Temporary solution while firmware don't support NOA change
		 * notification yet */
		atbm_notify_noa(priv, 10);
	}

	mutex_unlock(&hw_priv->conf_mutex);

}

void atbm_multicast_start_work(struct work_struct *work)
{
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, multicast_start_work);
	long tmo = priv->join_dtim_period *
			(priv->beacon_int + 20) * HZ / 1024;

	atbm_cancle_queue_work(&priv->multicast_stop_work,true);
	if(atbm_bh_is_term(priv->hw_priv)){
		return;
	}
	if (!priv->aid0_bit_set) {
		wsm_lock_tx(priv->hw_priv);
		atbm_set_tim_impl(priv, true);
		priv->aid0_bit_set = true;
		mod_timer(&priv->mcast_timeout, jiffies + tmo);
		wsm_unlock_tx(priv->hw_priv);
	}
}

void atbm_multicast_stop_work(struct work_struct *work)
{
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, multicast_stop_work);
	if(atbm_bh_is_term(priv->hw_priv)){
		return;
	}
	if (priv->aid0_bit_set) {
		del_timer_sync(&priv->mcast_timeout);
		wsm_lock_tx(priv->hw_priv);
		priv->aid0_bit_set = false;
		atbm_set_tim_impl(priv, false);
		wsm_unlock_tx(priv->hw_priv);
	}
}

void atbm_mcast_timeout(unsigned long arg)
{
	struct atbm_vif *priv =
		(struct atbm_vif *)arg;

	wiphy_warn(priv->hw->wiphy,
		"Multicast delivery timeout.\n");
	spin_lock_bh(&priv->ps_state_lock);
	priv->tx_multicast = priv->aid0_bit_set &&
			priv->buffered_multicasts;
	if (priv->tx_multicast)
		atbm_bh_wakeup(ABwifi_vifpriv_to_hwpriv(priv));
	spin_unlock_bh(&priv->ps_state_lock);
}

int atbm_ampdu_action(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif,
			enum ieee80211_ampdu_mlme_action action,
			struct ieee80211_sta *sta, u16 tid, u16 *ssn,
			u8 buf_size)
{
	/* Aggregation is implemented fully in firmware,
	 * including block ack negotiation.
	 * In case of AMPDU aggregation in RX direction
	 * re-ordering of packets takes place on host. mac80211
	 * needs the ADDBA Request to setup reodering.mac80211 also
	 * sends ADDBA Response which is discarded in the driver as
	 * FW generates the ADDBA Response on its own.*/
	int ret;

	switch (action) {
	case IEEE80211_AMPDU_RX_START:
	case IEEE80211_AMPDU_RX_STOP:
		/* Just return OK to mac80211 */
		ret = 0;
		break;
	default:
		ret = -ENOTSUPP;
	}
	return ret;
}

/* ******************************************************************** */
/* WSM callback								*/
void atbm_suspend_resume(struct atbm_vif *priv,
			   struct wsm_suspend_resume *arg)
{
	struct atbm_common *hw_priv =
		ABwifi_vifpriv_to_hwpriv(priv);
#if 0
	ap_printk(KERN_DEBUG "[AP] %s: %s\n",
			arg->stop ? "stop" : "start",
			arg->multicast ? "broadcast" : "unicast");
#endif
	if (arg->multicast) {
		bool cancel_tmo = false;
		spin_lock_bh(&priv->ps_state_lock);
		if (arg->stop) {
			priv->tx_multicast = false;
		} else {
			/* Firmware sends this indication every DTIM if there
			 * is a STA in powersave connected. There is no reason
			 * to suspend, following wakeup will consume much more
			 * power than it could be saved. */
			#ifdef CONFIG_PM
			atbm_pm_stay_awake(&hw_priv->pm_state,
					priv->join_dtim_period *
					(priv->beacon_int + 20) * HZ / 1024);
			#endif
			priv->tx_multicast = priv->aid0_bit_set &&
					priv->buffered_multicasts;
			if (priv->tx_multicast) {
				cancel_tmo = true;
				atbm_bh_wakeup(hw_priv);
			}
		}
		spin_unlock_bh(&priv->ps_state_lock);
		if (cancel_tmo)
			del_timer_sync(&priv->mcast_timeout);
	} else {
		spin_lock_bh(&priv->ps_state_lock);
		atbm_ps_notify(priv, arg->link_id, arg->stop);
		spin_unlock_bh(&priv->ps_state_lock);
		if (!arg->stop)
			atbm_bh_wakeup(hw_priv);
	}
	return;
}

/* ******************************************************************** */
/* AP privates								*/

static int atbm_upload_beacon(struct atbm_vif *priv)
{
	int ret = 0;
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_BEACON,
	};
	struct atbm_ieee80211_mgmt *mgmt;
	u8 *erp_inf, *ies, *ht_info;
	u32 ies_len;
	if (priv->vif->p2p ||
			hw_priv->channel->band == IEEE80211_BAND_5GHZ)
		frame.rate = WSM_TRANSMIT_RATE_6;

	frame.skb = ieee80211_beacon_get(priv->hw, priv->vif);
	if (!frame.skb)
	{
		printk(KERN_DEBUG "%s: ieee80211_beacon_get return NULL\n", __func__);
		return -ENOMEM;
	}
#ifdef ATBM_PRIVATE_IE
	frame.skb=atbm_mgmt_add_private_ie(frame.skb);
#endif
	mgmt = (void *)frame.skb->data;
	ies = mgmt->u.beacon.variable;
	ies_len = frame.skb->len - (u32)(ies - (u8 *)mgmt);
#ifdef ATBM_SUPPORT_WIDTH_40M
//#ifdef P2P_MULTIVIF
	printk(KERN_DEBUG "%s:atbm_clear_wpas_p2p_40M_ie\n",__func__);
	if(priv->if_id != 0)
		atbm_clear_wpas_p2p_40M_ie(mgmt,frame.skb->len);
//#endif
#endif
	ht_info = (u8 *)atbm_ieee80211_find_ie( ATBM_WLAN_EID_HT_INFORMATION, ies, ies_len);
        if (ht_info) {
		/* Enable RIFS*/
		ht_info[3] |= 8;
        }
	erp_inf = (u8 *)atbm_ieee80211_find_ie(ATBM_WLAN_EID_ERP_INFO, ies, ies_len);
	if (erp_inf) {
		if (erp_inf[ERP_INFO_BYTE_OFFSET]
				& WLAN_ERP_BARKER_PREAMBLE)
			priv->erp_info |= WLAN_ERP_BARKER_PREAMBLE;
		else
			priv->erp_info &= ~WLAN_ERP_BARKER_PREAMBLE;

		if (erp_inf[ERP_INFO_BYTE_OFFSET]
				& WLAN_ERP_NON_ERP_PRESENT) {
			priv->erp_info |= WLAN_ERP_USE_PROTECTION;
			priv->erp_info |= WLAN_ERP_NON_ERP_PRESENT;
		} else {
			priv->erp_info &= ~WLAN_ERP_USE_PROTECTION;
			priv->erp_info &= ~WLAN_ERP_NON_ERP_PRESENT;
		}
	}
#ifdef HIDDEN_SSID
	if (priv->hidden_ssid) {
		u8 *ssid_ie;
		u8 ssid_len;

		printk(KERN_DEBUG "%s: hidden_ssid set\n", __func__);
		ssid_ie = (u8 *)atbm_ieee80211_find_ie(ATBM_WLAN_EID_SSID, ies, ies_len);
		WARN_ON(!ssid_ie);
		ssid_len = ssid_ie[1];
		if (ssid_len) {
			printk(KERN_DEBUG "%s: hidden_ssid with zero content "
					"ssid\n", __func__);
			ssid_ie[1] = 0;
			memmove(ssid_ie + 2, ssid_ie + 2 + ssid_len,
					(ies + ies_len -
					 (ssid_ie + 2 + ssid_len)));
			frame.skb->len -= ssid_len;
		} else {
			printk(KERN_ERR "%s: hidden ssid with ssid len 0"
					" not supported", __func__);
			atbm_dev_kfree_skb(frame.skb);
			return -1;
		}
	}
#endif
	ret = wsm_set_template_frame(hw_priv, &frame, priv->if_id);
	if (!ret) {
#ifdef ATBM_PROBE_RESP_EXTRA_IE
		if(priv->mode == NL80211_IFTYPE_AP) {
		ret = atbm_upload_proberesp(priv);
			goto __exit_up_beacon;
		}
#endif
		/* TODO: Distille probe resp; remove TIM
		 * and other beacon-specific IEs */
		*(__le16 *)frame.skb->data =
			__cpu_to_le16(IEEE80211_FTYPE_MGMT |
				      IEEE80211_STYPE_PROBE_RESP);
		frame.frame_type = WSM_FRAME_TYPE_PROBE_RESPONSE;
		/* TODO: Ideally probe response template should separately
		   configured by supplicant through openmac. This is a
		   temporary work-around known to fail p2p group info
		   attribute related tests
		   */
			ret = wsm_set_template_frame(hw_priv, &frame,
					priv->if_id);
			WARN_ON(wsm_set_probe_responder(priv, false));
	}
__exit_up_beacon:
	atbm_dev_kfree_skb(frame.skb);

	return ret;
}

#ifdef ATBM_PROBE_RESP_EXTRA_IE
static int atbm_upload_proberesp(struct atbm_vif *priv)
{
	int ret = 0;
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_PROBE_RESPONSE,
	};
#ifdef HIDDEN_SSID
	u8 *ssid_ie;
#endif
	if (priv->vif->p2p || hw_priv->channel->band == IEEE80211_BAND_5GHZ)
		frame.rate = WSM_TRANSMIT_RATE_6;

	frame.skb = ieee80211_proberesp_get(priv->hw, priv->vif);
	if (WARN_ON(!frame.skb))
		return -ENOMEM;
#ifdef ATBM_PRIVATE_IE
	frame.skb=atbm_mgmt_add_private_ie(frame.skb);
#endif

#ifdef HIDDEN_SSID
	if (priv->hidden_ssid) {
		int offset;
		u8 ssid_len;
		/* we are assuming beacon from upper layer will always contain
		   zero filled ssid for hidden ap. The beacon shall never have
		   ssid len = 0.
		  */

		offset = offsetof(struct atbm_ieee80211_mgmt, u.probe_resp.variable);
		ssid_ie = (u8*)atbm_ieee80211_find_ie(ATBM_WLAN_EID_SSID,
				frame.skb->data + offset,
				frame.skb->len - offset);
		ssid_len = ssid_ie[1];
		if (ssid_len && (ssid_len == priv->ssid_length)) {
			memcpy(ssid_ie + 2, priv->ssid, ssid_len);
		} else {
			printk(KERN_ERR "%s: hidden ssid with mismatched ssid_len %d\n",
					__func__, ssid_len);
			atbm_dev_kfree_skb(frame.skb);
			return -1;
		}
	}
#endif
	ret = wsm_set_template_frame(hw_priv, &frame,  priv->if_id);
	WARN_ON(wsm_set_probe_responder(priv, false));

	atbm_dev_kfree_skb(frame.skb);

	return ret;
}
#endif

int atbm_upload_beacon_private(struct atbm_vif *priv)
{
	printk("%s, priv %p\n", __func__, priv);
	return atbm_upload_beacon(priv);
}
int atbm_upload_proberesp_private(struct atbm_vif *priv)
{
	printk("%s, priv %p\n", __func__, priv);
	return atbm_upload_proberesp(priv);
}

static int atbm_upload_pspoll(struct atbm_vif *priv)
{
	int ret = 0;
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_PS_POLL,
		.rate = 0xFF,
	};


	frame.skb = ieee80211_pspoll_get(priv->hw, priv->vif);
	if (WARN_ON(!frame.skb))
		return -ENOMEM;

	ret = wsm_set_template_frame(ABwifi_vifpriv_to_hwpriv(priv), &frame,
				priv->if_id);

	atbm_dev_kfree_skb(frame.skb);

	return ret;
}

static int atbm_upload_null(struct atbm_vif *priv)
{
	int ret = 0;
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_NULL,
		.rate = 0xFF,
	};


	frame.skb = ieee80211_nullfunc_get(priv->hw, priv->vif);
	if (WARN_ON(!frame.skb))
		return -ENOMEM;

	ret = wsm_set_template_frame(ABwifi_vifpriv_to_hwpriv(priv),
				&frame, priv->if_id);

	atbm_dev_kfree_skb(frame.skb);

	return ret;
}

static int atbm_upload_qosnull(struct atbm_vif *priv)
{
	int ret = 0;
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_QOS_NULL,
		.rate = 0xFF,
	};


	frame.skb = ieee80211_qosnullfunc_get(priv->hw, priv->vif);
	if (WARN_ON(!frame.skb))
		return -ENOMEM;

	ret = wsm_set_template_frame(hw_priv, &frame, priv->if_id);

	atbm_dev_kfree_skb(frame.skb);

	return ret;
}


static int atbm_start_ap(struct atbm_vif *priv)
{
	int ret;
#ifndef HIDDEN_SSID
	const u8 *ssidie;
	struct sk_buff *skb;
	int offset;
#endif
	struct ieee80211_bss_conf *conf = &priv->vif->bss_conf;
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct wsm_start start = {
		.mode = priv->vif->p2p ?
				WSM_START_MODE_P2P_GO : WSM_START_MODE_AP,
		/* TODO:COMBO:Change once mac80211 support is available */
		.band = (hw_priv->channel->band == IEEE80211_BAND_5GHZ) ?
				WSM_PHY_BAND_5G : WSM_PHY_BAND_2_4G,
		.channelNumber = hw_priv->channel->hw_value,
		.beaconInterval = conf->beacon_int,
		.DTIMPeriod = conf->dtim_period,
		.preambleType = conf->use_short_preamble ?
				WSM_JOIN_PREAMBLE_SHORT :
				WSM_JOIN_PREAMBLE_LONG,
		.probeDelay = 100,
		.basicRateSet = atbm_rate_mask_to_wsm(hw_priv,
				conf->basic_rates),
#ifdef P2P_MULTIVIF
		.CTWindow = priv->vif->p2p ? 0xFFFFFFFF : 0,
#endif
	};

	struct wsm_inactivity inactivity = {
		.min_inactivity = 50,
		.max_inactivity = 10,
	};

	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};

	if (priv->if_id)
		start.mode |= WSM_FLAG_MAC_INSTANCE_1;
	else
		start.mode &= ~WSM_FLAG_MAC_INSTANCE_1;
	
	if(priv->vif->p2p){
		inactivity.min_inactivity = 15;
		inactivity.max_inactivity = 5;
	}
#ifdef RESET_CHANGE
	if(!atomic_read(&hw_priv->ap_restart))
#endif
		hw_priv->connected_sta_cnt = 0;
#ifdef RESET_CHANGE
	else
		printk("%s:ap restart!!!!!\n",__func__);
#endif

#ifndef HIDDEN_SSID
	/* Get SSID */
	skb = ieee80211_beacon_get(priv->hw, priv->vif);
	if (WARN_ON(!skb))
		return -ENOMEM;

	offset = offsetof(struct atbm_ieee80211_mgmt, u.beacon.variable);
	ssidie = atbm_ieee80211_find_ie(ATBM_WLAN_EID_SSID, skb->data + offset,
				  skb->len - offset);

	memset(priv->ssid, 0, sizeof(priv->ssid));
	if (ssidie) {
		priv->ssid_length = ssidie[1];
		if (WARN_ON(priv->ssid_length > sizeof(priv->ssid)))
			priv->ssid_length = sizeof(priv->ssid);
		memcpy(priv->ssid, &ssidie[2], priv->ssid_length);
	} else {
		priv->ssid_length = 0;
	}
	atbm_dev_kfree_skb(skb);
#endif

	priv->beacon_int = conf->beacon_int;
	priv->join_dtim_period = conf->dtim_period;

	start.ssidLength = priv->ssid_length;
	memcpy(&start.ssid[0], priv->ssid, start.ssidLength);

	memset(&priv->link_id_db, 0, sizeof(priv->link_id_db));
	
#ifdef ATBM_SUPPORT_WIDTH_40M

	start.channel_type = (u32)(hw_priv->channel_type);
	if(start_choff<=NL80211_CHAN_HT40PLUS)
	{
		printk("%s:fix chatype(%d)\n",__func__,start_choff);
		start.channel_type = start_choff;
	}
	
	if(priv->if_id != 0)
		start.channel_type = NL80211_CHAN_HT20;
	
	printk(KERN_ERR "%s:start.channel_type,(%d),channelNumber(%d)\n",__func__,start.channel_type,start.channelNumber);
	
	if(priv->if_id == 0)
	{
		if((hw_priv->channel_type == NL80211_CHAN_HT40MINUS) || (hw_priv->channel_type == NL80211_CHAN_HT40PLUS))
		{
			atomic_set(&hw_priv->tx_20M_lock,0);
		}
		else
		{
			atomic_set(&hw_priv->tx_20M_lock,1);
		}

		atomic_add_return(1, &hw_priv->channel_chaging);
		if(atbm_hw_priv_queue_delayed_work(hw_priv, &priv->chantype_change_work,0) <= 0)
		{
			printk("%s: queue chantype_change_work err\n",__func__);
			if(atomic_sub_return(1, &hw_priv->channel_chaging)<=0)
			{
				printk("%s: channel_chaging value err\n",__func__);
			}
		}
	}
#endif

	ap_printk(KERN_DEBUG "[AP] ch: %d(%d), bcn: %d(%d), "
		"brt: 0x%.8X, ssid: %.*s.\n",
		start.channelNumber, start.band,
		start.beaconInterval, start.DTIMPeriod,
		start.basicRateSet,
		start.ssidLength, start.ssid);
	ret = WARN_ON(wsm_start(hw_priv, &start, priv->if_id));

	if (!ret && priv->vif->p2p) {
		ap_printk(KERN_DEBUG
			"[AP] Setting p2p powersave "
			"configuration.\n");
		WARN_ON(wsm_set_p2p_ps_modeinfo(hw_priv,
			&priv->p2p_ps_modeinfo, priv->if_id));
		atbm_notify_noa(priv, ATBM_APOLLO_NOA_NOTIFICATION_DELAY);
	}
#ifdef ATBM_P2P_CHANGE
	if(priv->if_id == 0){
		atomic_set(&hw_priv->combination,1);
	}
#endif
	/*Set Inactivity time*/
	 if(!(strstr(&start.ssid[0], "6.1.12"))) {
		wsm_set_inactivity(hw_priv, &inactivity, priv->if_id);
	    }
	if (!ret) {
		#ifdef P2P_MULTIVIF
		if ((priv->if_id ==1) && !hw_priv->is_go_thru_go_neg)
			WARN_ON(wsm_set_block_ack_policy(hw_priv,
				ATBM_APOLLO_TX_BLOCK_ACK_DISABLED_FOR_ALL_TID,
				ATBM_APOLLO_RX_BLOCK_ACK_DISABLED_FOR_ALL_TID,
				priv->if_id));
		else
		#endif
			WARN_ON(wsm_set_block_ack_policy(hw_priv,
				hw_priv->ba_tid_tx_mask,
				hw_priv->ba_tid_rx_mask,
				priv->if_id));

		priv->join_status = ATBM_APOLLO_JOIN_STATUS_AP;
		/* atbm_update_filtering(priv); */
	}
	WARN_ON(wsm_set_operational_mode(hw_priv, &mode, priv->if_id));
	hw_priv->vif0_throttle = ATBM_WIFI_HOST_VIF0_11N_THROTTLE;
	hw_priv->vif1_throttle = ATBM_WIFI_HOST_VIF0_11N_THROTTLE;
	//use when wep share key,need firmware not enc auth frame
	if((priv->cipherType == WLAN_CIPHER_SUITE_WEP40)
		|| 	(priv->cipherType == WLAN_CIPHER_SUITE_WEP104)){
		struct wsm_protected_mgmt_policy mgmt_policy;
		mgmt_policy.protectedMgmtEnable = 0;
		mgmt_policy.unprotectedMgmtFramesAllowed = 1;
		mgmt_policy.encryptionForAuthFrame = 1;//not enc for hw
		wsm_set_protected_mgmt_policy(hw_priv, &mgmt_policy,
						  priv->if_id);
	}
	#ifdef	ATBM_WIFI_QUEUE_LOCK_BUG
	atbm_set_priv_queue_cap(priv);
	#endif
	printk(KERN_ERR "AP/GO mode BG THROTTLE %d,cipherType %x\n", hw_priv->vif0_throttle,priv->cipherType);
	return ret;
}

static int atbm_update_beaconing(struct atbm_vif *priv)
{
	struct ieee80211_bss_conf *conf = &priv->vif->bss_conf;
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct wsm_reset reset = {
		.link_id = 0,
		.reset_statistics = true,
	};

	if (priv->mode == NL80211_IFTYPE_AP) {
		/* TODO: check if changed channel, band */
		if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_AP ||
		    priv->beacon_int != conf->beacon_int) {
			ap_printk(KERN_DEBUG "ap restarting\n");
			wsm_lock_tx(hw_priv);
			if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_PASSIVE)
				WARN_ON(wsm_reset(hw_priv, &reset,
						  priv->if_id));
			priv->join_status = ATBM_APOLLO_JOIN_STATUS_PASSIVE;
			WARN_ON(atbm_start_ap(priv));
			wsm_unlock_tx(hw_priv);
		} else
			ap_printk(KERN_DEBUG "ap started join_status: %d\n",
				  priv->join_status);
	}
	return 0;
}

int atbm_find_link_id(struct atbm_vif *priv, const u8 *mac)
{
	int i, ret = 0;
	spin_lock_bh(&priv->ps_state_lock);

	for (i = 0; i < ATBMWIFI_MAX_STA_IN_AP_MODE; ++i) {
		if (!memcmp(mac, priv->link_id_db[i].mac, ETH_ALEN) &&
				priv->link_id_db[i].status) {
			priv->link_id_db[i].timestamp = jiffies;
			ret = i + 1;
			break;
		}
	}
	spin_unlock_bh(&priv->ps_state_lock);
	return ret;
}

int atbm_alloc_link_id(struct atbm_vif *priv, const u8 *mac)
{
	int i, ret = 0,if_id;
	unsigned long max_inactivity = 0;
	unsigned long now = jiffies;
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);

	spin_lock_bh(&priv->ps_state_lock);
	for (if_id = 0; if_id < ATBM_WIFI_MAX_VIFS; if_id++) {
		for (i = 0; i < ATBMWIFI_MAX_STA_IN_AP_MODE; ++i) {
			if (!priv->link_id_db[i].status) {
				ret = i + 1;
				break;
			} else if (priv->link_id_db[i].status != ATBM_APOLLO_LINK_HARD &&
				!hw_priv->tx_queue_stats.link_map_cache[if_id][i]) {
				unsigned long inactivity =
						now - priv->link_id_db[i].timestamp;
				if (inactivity < max_inactivity)
					continue;
				max_inactivity = inactivity;
				ret = i + 1;
			}
		}
	}
	if (ret) {
		struct atbm_link_entry *entry = &priv->link_id_db[ret - 1];
		ap_printk(KERN_DEBUG "[AP] STA added, link_id: %d\n",
			ret);
		entry->status = ATBM_APOLLO_LINK_RESERVE;
		memcpy(&entry->mac, mac, ETH_ALEN);
		memset(&entry->buffered, 0, ATBM_APOLLO_MAX_TID);
		atbm_skb_queue_head_init(&entry->rx_queue);
		wsm_lock_tx_async(hw_priv);
		if (atbm_hw_priv_queue_work(hw_priv, &priv->link_id_work) <= 0)
			wsm_unlock_tx(hw_priv);
	} else {
		wiphy_info(priv->hw->wiphy,
			"[AP] Early: no more link IDs available.\n");
	}

	spin_unlock_bh(&priv->ps_state_lock);
	return ret;
}

void atbm_link_id_work(struct work_struct *work)
{
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, link_id_work);
	struct atbm_common *hw_priv = priv->hw_priv;
	if(atbm_bh_is_term(hw_priv)){
		wsm_unlock_tx(hw_priv);
		return;
	}
	wsm_flush_tx(hw_priv);
	atbm_link_id_gc_work(&priv->link_id_gc_work.work);
	wsm_unlock_tx(hw_priv);
}

void atbm_link_id_gc_work(struct work_struct *work)
{
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, link_id_gc_work.work);
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct wsm_map_link map_link = {
		.link_id = 0,
	};
	unsigned long now = jiffies;
	unsigned long next_gc = -1;
	long ttl;
	bool need_reset;
	u32 mask;
	int i;

	if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_AP)
		return;
	if(atbm_bh_is_term(hw_priv)){
		return;
	}
	wsm_lock_tx(hw_priv);
	spin_lock_bh(&priv->ps_state_lock);
	for (i = 0; i < ATBMWIFI_MAX_STA_IN_AP_MODE; ++i) {
		need_reset = false;
		mask = BIT(i + 1);
		if (priv->link_id_db[i].status == ATBM_APOLLO_LINK_RESERVE ||
			(priv->link_id_db[i].status == ATBM_APOLLO_LINK_HARD &&
			 !(priv->link_id_map & mask))) {
			if (priv->link_id_map & mask) {
				priv->sta_asleep_mask &= ~mask;
				priv->pspoll_mask &= ~mask;
				need_reset = true;
			}
			priv->link_id_map |= mask;
			if (priv->link_id_db[i].status != ATBM_APOLLO_LINK_HARD)
				priv->link_id_db[i].status = ATBM_APOLLO_LINK_SOFT;
			memcpy(map_link.mac_addr, priv->link_id_db[i].mac,
					ETH_ALEN);
			spin_unlock_bh(&priv->ps_state_lock);
			if (need_reset) {
				WARN_ON(ABwifi_unmap_link(priv, i + 1));
			}
			map_link.link_id = i + 1;
			WARN_ON(wsm_map_link(hw_priv, &map_link, priv->if_id));
			next_gc = min(next_gc, ATBM_APOLLO_LINK_ID_GC_TIMEOUT);
			spin_lock_bh(&priv->ps_state_lock);
		} else if (priv->link_id_db[i].status == ATBM_APOLLO_LINK_SOFT) {
			ttl = priv->link_id_db[i].timestamp - now +
					ATBM_APOLLO_LINK_ID_GC_TIMEOUT;
			if (ttl <= 0) {
				need_reset = true;
				priv->link_id_db[i].status = ATBM_APOLLO_LINK_OFF;
				priv->link_id_map &= ~mask;
				priv->sta_asleep_mask &= ~mask;
				priv->pspoll_mask &= ~mask;
				memset(map_link.mac_addr, 0, ETH_ALEN);
				spin_unlock_bh(&priv->ps_state_lock);
				WARN_ON(ABwifi_unmap_link(priv, i + 1));
				spin_lock_bh(&priv->ps_state_lock);
			} else {
				next_gc = min_t(unsigned long, next_gc, ttl);
			}
		} else if (priv->link_id_db[i].status == ATBM_APOLLO_LINK_RESET ||
				priv->link_id_db[i].status ==
				ATBM_APOLLO_LINK_RESET_REMAP) {
			int status = priv->link_id_db[i].status;
			priv->link_id_db[i].status = ATBM_APOLLO_LINK_OFF;
			priv->link_id_db[i].timestamp = now;
			spin_unlock_bh(&priv->ps_state_lock);
			WARN_ON(ABwifi_unmap_link(priv, i + 1));
			if (status == ATBM_APOLLO_LINK_RESET_REMAP) {
				memcpy(map_link.mac_addr,
						priv->link_id_db[i].mac,
						ETH_ALEN);
				map_link.link_id = i + 1;
				WARN_ON(wsm_map_link(hw_priv, &map_link,
						     priv->if_id));
				next_gc = min(next_gc,
					      ATBM_APOLLO_LINK_ID_GC_TIMEOUT);
				priv->link_id_db[i].status =
						priv->link_id_db[i].prev_status;
			}
			spin_lock_bh(&priv->ps_state_lock);
		}
		if (need_reset) {
			atbm_skb_queue_purge(&priv->link_id_db[i].rx_queue);
			ap_printk(KERN_DEBUG "[AP] STA removed, link_id: %d\n",
					i + 1);
		}
	}
	spin_unlock_bh(&priv->ps_state_lock);
	if (next_gc != -1)
		atbm_hw_priv_queue_delayed_work(hw_priv,
				&priv->link_id_gc_work, next_gc);
	wsm_unlock_tx(hw_priv);
}

void atbm_notify_noa(struct atbm_vif *priv, int delay)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
//	struct cfg80211_p2p_ps p2p_ps = {0};
	struct wsm_p2p_ps_modeinfo *modeinfo;
	modeinfo = &priv->p2p_ps_modeinfo;

	ap_printk(KERN_DEBUG "[AP]: %s called\n", __func__);
	/*
	*if ctwindows and disable is disabled,we should keep
	*the all of the interface awake;
	*/
	if((!!(modeinfo->oppPsCTWindow & BIT(7)) == 0)&&
		( (modeinfo->oppPsCTWindow & (~BIT(7))) == 0))
	{
		struct atbm_vif *priv_wlan;
		/*
		*BUG!!!!!! at go mode, the firmware can enter ps mode if the wlan0 interface
		*has connect ap.
		*so to fixed that bug, i wanted to change wlan0 ps mode to active mode.
		*!!!!!!!!due to that wsm_set_pm function must lock wsm_oper_lock and unlock wsm_oper_lock 
		*must wait until the firmware ps mode has change at ap,so there is some dangerous
		*that the hmac cannot receive the ps commplete indication,that make the wsm_oper_lock
		*unlock.
		*/
		if((priv_wlan=__ABwifi_hwpriv_to_vifpriv(hw_priv,0))!= NULL)
		{
			if(priv_wlan->powersave_mode.pmMode != WSM_PSM_ACTIVE)
				memcpy(&hw_priv->roc_wlan_pm,&priv_wlan->powersave_mode,sizeof(struct wsm_set_pm));
			
			priv_wlan->powersave_mode.pmMode = WSM_PSM_ACTIVE;
			atbm_set_pm(priv_wlan,&priv_wlan->powersave_mode);
		}
	}
	if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_AP)
		return;

	if (delay)
		msleep(delay);

	if (!WARN_ON(wsm_get_p2p_ps_modeinfo(hw_priv, modeinfo,priv->if_id))) {
#if defined(CONFIG_ATBM_APOLLO_STA_DEBUG)
		print_hex_dump_bytes("[AP] p2p_get_ps_modeinfo: ",
				     DUMP_PREFIX_NONE,
				     (u8 *)modeinfo,
				     sizeof(*modeinfo));
#endif /* CONFIG_ATBM_APOLLO_STA_DEBUG */
#if 0
		p2p_ps.opp_ps = !!(modeinfo->oppPsCTWindow & BIT(7));
		p2p_ps.ctwindow = modeinfo->oppPsCTWindow & (~BIT(7));
		p2p_ps.count = modeinfo->count;
		p2p_ps.start = __le32_to_cpu(modeinfo->startTime);
		p2p_ps.duration = __le32_to_cpu(modeinfo->duration);
		p2p_ps.interval = __le32_to_cpu(modeinfo->interval);
		p2p_ps.index = modeinfo->reserved;

		ieee80211_p2p_noa_notify(priv->vif,
					 &p2p_ps,
					 GFP_KERNEL);
#endif
	}

}

int ABwifi_unmap_link(struct atbm_vif *priv, int link_id)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	//int ret = 0;
	//struct wsm_operational_mode mode = {
	//	.power_mode = wsm_power_mode_quiescent,
	//	.disableMoreFlagUsage = true,
	//};


	struct wsm_map_link maplink = {
		.link_id = link_id,
		.unmap = true,
	};
	if (link_id)
		memcpy(&maplink.mac_addr[0],
			priv->link_id_db[link_id - 1].mac, ETH_ALEN);
	return wsm_map_link(hw_priv, &maplink, priv->if_id);

}
void atbm_ht_info_update_work(struct work_struct *work)
{
    struct sk_buff *skb;
    struct atbm_ieee80211_mgmt *mgmt;
    u8 *ht_info, *ies;
    u32 ies_len;
    struct atbm_vif *priv =
            container_of(work, struct atbm_vif, ht_info_update_work);
    struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
    struct wsm_update_ie update_ie = {
            .what = WSM_UPDATE_IE_BEACON,
            .count = 1,
    };
	if(atbm_bh_is_term(hw_priv)){
		return;
	}
    skb = ieee80211_beacon_get(priv->hw, priv->vif);
	if (WARN_ON(!skb))
		return;

    mgmt = (void *)skb->data;
    ies = mgmt->u.beacon.variable;
    ies_len = skb->len - (u32)(ies - (u8 *)mgmt);
    ht_info= (u8 *)atbm_ieee80211_find_ie( ATBM_WLAN_EID_HT_INFORMATION, ies, ies_len);
    if(ht_info && priv->ht_info == HT_INFO_MASK) {

    ht_info[HT_INFO_OFFSET] |= 0x11;
    update_ie.ies = ht_info;
    update_ie.length = HT_INFO_IE_LEN;

    WARN_ON(wsm_update_ie(hw_priv, &update_ie, priv->if_id));


    }
	atbm_dev_kfree_skb(skb);
}
#ifdef ATBM_SUPPORT_PKG_MONITOR
int atbm_start_monitor_mode(struct atbm_vif *priv,
				struct ieee80211_channel *chan)
{
	struct atbm_common *hw_priv = priv->hw_priv;
	int ret = 0;
	struct wsm_start start = {
		.mode = WSM_START_MODE_MONITOR_DEV | (priv->if_id << 4),
		.band = (chan->band == IEEE80211_BAND_5GHZ) ?
				WSM_PHY_BAND_5G : WSM_PHY_BAND_2_4G,
		.channelNumber = chan->hw_value,
		.beaconInterval = 100,
		.DTIMPeriod = 1,
		.probeDelay = 0,
		.basicRateSet = 0x0F,
	};
	if(priv->join_status != ATBM_APOLLO_JOIN_STATUS_SIMPLE_MONITOR){
		printk(KERN_ERR "%s:only simple monitor can enter,(%d)\n",__func__,priv->join_status);
		return -1;
	}
	/*
	*when one interface in monitor mode ,othe interface can not process scan
	*or remain on channel;
	*/
	lockdep_assert_held(&hw_priv->conf_mutex);
	hw_priv->monitor_if_id = priv->if_id;
	/*
	*must make sure monitor_if_id has been set to priv->if_id,before send cmd to lmc;
	*/
	smp_mb();
	/*
	* clear the pkg 
	*/
	ret = WARN_ON(__atbm_flush(hw_priv, false,priv->if_id));
	if(!ret){
		wsm_unlock_tx(hw_priv);
	}

#ifdef ATBM_SUPPORT_WIDTH_40M
	if((chan->hw_value>=9)&&(chan->hw_value<=14))
		start.channel_type = (u32)(NL80211_CHAN_HT40MINUS);
	else
		start.channel_type = (u32)(NL80211_CHAN_HT40PLUS);
#endif
	printk(KERN_ERR "%s:if_id(%d),channel(%d)\n",__func__,priv->if_id,chan->hw_value);
	wsm_start(hw_priv, &start, ATBM_WIFI_GENERIC_IF_ID);

	return ret;
}

int atbm_stop_monitor_mode(struct atbm_vif *priv)
{
	struct atbm_common *hw_priv = priv->hw_priv;
	int ret = 0;
	struct wsm_reset reset = {
		.reset_statistics = true,
	};
	lockdep_assert_held(&hw_priv->conf_mutex);
	ret = WARN_ON(__atbm_flush(hw_priv, false, priv->if_id));
	if (!ret) {
		wsm_unlock_tx(hw_priv);
	}
	
	ret = wsm_reset(priv->hw_priv, &reset, ATBM_WIFI_GENERIC_IF_ID);
	smp_mb();
	hw_priv->monitor_if_id = -1;	
	printk(KERN_ERR "%s:if_id(%d)\n",__func__,priv->if_id);
	return ret;
}
#endif
