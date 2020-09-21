/*
 * Interface handling (except master interface)
 *
 * Copyright 2002-2005, Instant802 Networks, Inc.
 * Copyright 2005-2006, Devicescape Software, Inc.
 * Copyright (c) 2006 Jiri Benc <jbenc@suse.cz>
 * Copyright 2008, Johannes Berg <johannes@sipsolutions.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <net/atbm_mac80211.h>
#include <net/ieee80211_radiotap.h>
#ifdef CONFIG_WIRELESS_EXT 
#include <net/iw_handler.h>
#endif
#include "ieee80211_i.h"
#include "sta_info.h"
#include "debugfs_netdev.h"
#include "mesh.h"
#include "led.h"
#include "driver-ops.h"
#include "wme.h"
#include "rate.h"

/**
 * DOC: Interface list locking
 *
 * The interface list in each struct ieee80211_local is protected
 * three-fold:
 *
 * (1) modifications may only be done under the RTNL
 * (2) modifications and readers are protected against each other by
 *     the iflist_mtx.
 * (3) modifications are done in an RCU manner so atomic readers
 *     can traverse the list in RCU-safe blocks.
 *
 * As a consequence, reads (traversals) of the list can be protected
 * by either the RTNL, the iflist_mtx or RCU.
 */


static int ieee80211_change_mtu(struct net_device *dev, int new_mtu)
{
	int meshhdrlen;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	meshhdrlen = (sdata->vif.type == NL80211_IFTYPE_MESH_POINT) ? 5 : 0;

	/* FIX: what would be proper limits for MTU?
	 * This interface uses 802.3 frames. */
	if (new_mtu < 256 ||
	    new_mtu > IEEE80211_MAX_DATA_LEN - 24 - 6 - meshhdrlen) {
		return -EINVAL;
	}

#ifdef CONFIG_MAC80211_ATBM_VERBOSE_DEBUG
	printk(KERN_DEBUG "%s: setting MTU %d\n", dev->name, new_mtu);
#endif /* CONFIG_MAC80211_ATBM_VERBOSE_DEBUG */
	dev->mtu = new_mtu;
	return 0;
}

static int ieee80211_change_mac(struct net_device *dev, void *addr)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct sockaddr *sa = addr;
	int ret;

	if (ieee80211_sdata_running(sdata))
		return -EBUSY;

	ret = eth_mac_addr(dev, sa);

	if (ret == 0)
		memcpy(sdata->vif.addr, sa->sa_data, ETH_ALEN);

	return ret;
}

static inline int identical_mac_addr_allowed(int type1, int type2)
{
	return type1 == NL80211_IFTYPE_MONITOR ||
		type2 == NL80211_IFTYPE_MONITOR ||
		(type1 == NL80211_IFTYPE_AP && type2 == NL80211_IFTYPE_WDS) ||
		(type1 == NL80211_IFTYPE_WDS &&
			(type2 == NL80211_IFTYPE_WDS ||
			 type2 == NL80211_IFTYPE_AP)) ||
		(type1 == NL80211_IFTYPE_AP && type2 == NL80211_IFTYPE_AP_VLAN) ||
		(type1 == NL80211_IFTYPE_AP_VLAN &&
			(type2 == NL80211_IFTYPE_AP ||
			 type2 == NL80211_IFTYPE_AP_VLAN));
}

static int ieee80211_check_concurrent_iface(struct ieee80211_sub_if_data *sdata,
					    enum nl80211_iftype iftype)
{
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_sub_if_data *nsdata;
	struct net_device *dev = sdata->dev;

	ASSERT_RTNL();

	/* we hold the RTNL here so can safely walk the list */
	list_for_each_entry(nsdata, &local->interfaces, list) {
		struct net_device *ndev = nsdata->dev;

		if (ndev != dev && ieee80211_sdata_running(nsdata)) {
			/*
			 * Allow only a single IBSS interface to be up at any
			 * time. This is restricted because beacon distribution
			 * cannot work properly if both are in the same IBSS.
			 *
			 * To remove this restriction we'd have to disallow them
			 * from setting the same SSID on different IBSS interfaces
			 * belonging to the same hardware. Then, however, we're
			 * faced with having to adopt two different TSF timers...
			 */
			if (iftype == NL80211_IFTYPE_ADHOC &&
			    nsdata->vif.type == NL80211_IFTYPE_ADHOC)
				return -EBUSY;

			/*
			 * The remaining checks are only performed for interfaces
			 * with the same MAC address.
			 */
			if (atbm_compare_ether_addr(dev->dev_addr, ndev->dev_addr))
				continue;

			/*
			 * check whether it may have the same address
			 */
			if (!identical_mac_addr_allowed(iftype,
							nsdata->vif.type))
				return -ENOTUNIQ;

			/*
			 * can only add VLANs to enabled APs
			 */
			if (iftype == NL80211_IFTYPE_AP_VLAN &&
			    nsdata->vif.type == NL80211_IFTYPE_AP)
				sdata->bss = &nsdata->u.ap;
		}
	}

	return 0;
}

static int ieee80211_check_queues(struct ieee80211_sub_if_data *sdata)
{
	int n_queues = sdata->local->hw.queues;
	int i;

	for (i = 0; i < IEEE80211_NUM_ACS; i++) {
		if (WARN_ON_ONCE(sdata->vif.hw_queue[i] ==
				 IEEE80211_INVAL_HW_QUEUE))
			return -EINVAL;
		//printk("(%p)sdata->vif.hw_queue[%d]=%d ",sdata,i,sdata->vif.hw_queue[i]);
		if (WARN_ON_ONCE(sdata->vif.hw_queue[i] >=
				 n_queues))
			return -EINVAL;
	}

	if ((sdata->vif.type != NL80211_IFTYPE_AP) ||
	    !(sdata->local->hw.flags & IEEE80211_HW_QUEUE_CONTROL)) {
		sdata->vif.cab_queue = IEEE80211_INVAL_HW_QUEUE;
		return 0;
	}

	if (WARN_ON_ONCE(sdata->vif.cab_queue == IEEE80211_INVAL_HW_QUEUE))
		return -EINVAL;

	if (WARN_ON_ONCE(sdata->vif.cab_queue >= n_queues))
		return -EINVAL;

	return 0;
}

void ieee80211_adjust_monitor_flags(struct ieee80211_sub_if_data *sdata,
				    const int offset)
{
	u32 flags = sdata->u.mntr_flags, req_flags = 0;

#define ADJUST(_f, _s) do {\
	if (flags & MONITOR_FLAG_##_f)\
		req_flags |= _s;\
	} while (0)

	ADJUST(PLCPFAIL, FIF_PLCPFAIL);
	ADJUST(CONTROL, FIF_PSPOLL);
	ADJUST(CONTROL, FIF_CONTROL);
	ADJUST(FCSFAIL, FIF_FCSFAIL);
	ADJUST(OTHER_BSS, FIF_OTHER_BSS);
	if (offset > 0)
		sdata->req_filt_flags |= req_flags;
	else
		sdata->req_filt_flags &= ~req_flags;

#undef ADJUST
}

static void ieee80211_set_default_queues(struct ieee80211_sub_if_data *sdata)
{
	struct ieee80211_local *local = sdata->local;
	int i;

	for (i = 0; i < IEEE80211_NUM_ACS; i++) {
		if (local->hw.flags & IEEE80211_HW_QUEUE_CONTROL)
			sdata->vif.hw_queue[i] = IEEE80211_INVAL_HW_QUEUE;
		else
			sdata->vif.hw_queue[i] = i;
		//printk("%s (%p)sdata->vif.hw_queue[%d]=%d\n",__func__,sdata,i,sdata->vif.hw_queue[i]);
	}
	sdata->vif.cab_queue = IEEE80211_INVAL_HW_QUEUE;
	
}

/*
 * NOTE: Be very careful when changing this function, it must NOT return
 * an error on interface type changes that have been pre-checked, so most
 * checks should be in ieee80211_check_concurrent_iface.
 */
static int ieee80211_do_open(struct net_device *dev, bool coming_up)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct sta_info *sta;
	u32 changed = 0;
	int res;
	u32 hw_reconf_flags = 0;

	sdata->vif.bss_conf.chan_conf = &sdata->chan_state.conf;
	
	////printk("%s (%p)sdata->vif.hw_queue[0]=%d\n",__func__,sdata,sdata->vif.hw_queue[0]);

	switch (sdata->vif.type) {
	case NL80211_IFTYPE_WDS:
		if (!is_valid_ether_addr(sdata->u.wds.remote_addr))
			return -ENOLINK;
		break;
	case NL80211_IFTYPE_AP_VLAN:
		if (!sdata->bss)
			return -ENOLINK;
		list_add(&sdata->u.vlan.list, &sdata->bss->vlans);
		break;
	case NL80211_IFTYPE_AP:
		sdata->bss = &sdata->u.ap;
		break;
	case NL80211_IFTYPE_MESH_POINT:
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_MONITOR:
	case NL80211_IFTYPE_ADHOC:
		/* no special treatment */
		break;
	case NL80211_IFTYPE_UNSPECIFIED:
	case NUM_NL80211_IFTYPES:
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
		/* cannot happen */
		WARN_ON(1);
		break;
	default:
		WARN_ON(1);
		break;
	}

	if (local->open_count == 0) {
		res = drv_start(local);
		if (res)
			goto err_del_bss;
		#ifdef IEEE80211_SUPPORT_NAPI
		if (local->ops->napi_poll)
			napi_enable(&local->napi);
		#endif
		/* we're brought up, everything changes */
		hw_reconf_flags = ~0;
		ieee80211_led_radio(local, true);
		ieee80211_mod_tpt_led_trig(local,
					   IEEE80211_TPT_LEDTRIG_FL_RADIO, 0);
	}

	/*
	 * Copy the hopefully now-present MAC address to
	 * this interface, if it has the special null one.
	 */
	if (is_zero_ether_addr(dev->dev_addr)) {
		memcpy(dev->dev_addr,
		       local->hw.wiphy->perm_addr,
		       ETH_ALEN);
		memcpy(dev->perm_addr, dev->dev_addr, ETH_ALEN);

		if (!is_valid_ether_addr(dev->dev_addr)) {
			if (!local->open_count)
				drv_stop(local);
			return -EADDRNOTAVAIL;
		}
	}

	switch (sdata->vif.type) {
	case NL80211_IFTYPE_AP_VLAN:
		/* no need to tell driver */
		break;
	case NL80211_IFTYPE_MONITOR:
		if (sdata->u.mntr_flags & MONITOR_FLAG_COOK_FRAMES) {
			local->cooked_mntrs++;
			break;
		}

		/* must be before the call to ieee80211_configure_filter */
		local->monitors++;
		if (local->monitors == 1) {
			local->hw.conf.flags |= IEEE80211_CONF_MONITOR;
			hw_reconf_flags |= IEEE80211_CONF_CHANGE_MONITOR;
		}

		ieee80211_adjust_monitor_flags(sdata, 1);
		ieee80211_configure_filter(sdata);
#ifdef ATBM_SUPPORT_PKG_MONITOR
		if(strstr(sdata->name,"mon") != NULL){
			netif_carrier_on(dev);
			break;
		}
		printk(KERN_ERR "%s:[%s] change to monitor mode(%d)\n",__func__,sdata->name,coming_up);
		local->only_monitors++;
		local->monitor_sdata=sdata;
#else
		netif_carrier_on(dev);
		break;
#endif
	default:
		if (coming_up) {
			res = drv_add_interface(local, &sdata->vif);
			if (res)
				goto err_stop;
			//printk("%s <%d> ,(%p)sdata->vif.hw_queue[0]=%d\n",__func__,__LINE__,sdata,sdata->vif.hw_queue[0]);
			res = ieee80211_check_queues(sdata);
			if (res)
				goto err_del_interface;
		}
		//printk("%s <%d> ,(%p)sdata->vif.hw_queue[0]=%d\n",__func__,__LINE__,sdata,sdata->vif.hw_queue[0]);

		if (sdata->vif.type == NL80211_IFTYPE_AP) {
			ieee80211_configure_filter(sdata);
		}

		changed |= ieee80211_reset_erp_info(sdata);
		ieee80211_bss_info_change_notify(sdata, changed);

		if (sdata->vif.type == NL80211_IFTYPE_STATION)
			netif_carrier_off(dev);
		else
			netif_carrier_on(dev);

#ifdef 	CONFIG_MAC80211_BRIDGE
		//add by wp for ipc bridge
		dev->priv_flags &= ~IFF_DONT_BRIDGE;
#endif //	CONFIG_MAC80211_BRIDGE
	}

	set_bit(SDATA_STATE_RUNNING, &sdata->state);

	if (sdata->vif.type == NL80211_IFTYPE_WDS) {
		/* Create STA entry for the WDS peer */
		sta = sta_info_alloc(sdata, sdata->u.wds.remote_addr,
				     GFP_KERNEL);
		if (!sta) {
			res = -ENOMEM;
			goto err_del_interface;
		}

		/* no atomic bitop required since STA is not live yet */
		set_sta_flag(sta, WLAN_STA_AUTHORIZED);

		res = sta_info_insert(sta);
		if (res) {
			/* STA has been freed */
			goto err_del_interface;
		}

		rate_control_rate_init(sta);
	}

	mutex_lock(&local->mtx);
	hw_reconf_flags |= __ieee80211_recalc_idle(local);
	mutex_unlock(&local->mtx);

	if (coming_up)
		local->open_count++;
#ifdef CONFIG_MAC80211_BRIDGE
	br0_netdev_open(dev);
#endif	// CONFIG_MAC80211_BRIDGE

	if (hw_reconf_flags) {
		ieee80211_hw_config(local, hw_reconf_flags);
		/*
		 * set default queue parameters so drivers don't
		 * need to initialise the hardware if the hardware
		 * doesn't start up with sane defaults
		 */
		ieee80211_set_wmm_default(sdata);
	}

	ieee80211_recalc_ps(local, -1);

	netif_tx_start_all_queues(dev);

	return 0;
 err_del_interface:
	drv_remove_interface(local, &sdata->vif);
 err_stop:
	if (!local->open_count)
		drv_stop(local);
 err_del_bss:
	sdata->bss = NULL;
	if (sdata->vif.type == NL80211_IFTYPE_AP_VLAN)
		list_del(&sdata->u.vlan.list);
	clear_bit(SDATA_STATE_RUNNING, &sdata->state);
	return res;
}

static int ieee80211_open(struct net_device *dev)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	int err;

	/* fail early if user set an invalid address */
	if (!is_valid_ether_addr(dev->dev_addr))
		return -EADDRNOTAVAIL;
	//printk("%s (%p)sdata->vif.hw_queue[0]=%d\n",__func__,sdata,sdata->vif.hw_queue[0]);

	err = ieee80211_check_concurrent_iface(sdata, sdata->vif.type);
	if (err)
		return err;

	return ieee80211_do_open(dev, true);
}

static void ieee80211_do_stop(struct ieee80211_sub_if_data *sdata,
			      bool going_down)
{
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(local, sdata);
	unsigned long flags;
	struct sk_buff *skb, *tmp;
	u32 hw_reconf_flags = 0;
	int i;
	enum nl80211_channel_type orig_ct;
	
	clear_bit(SDATA_STATE_RUNNING, &sdata->state);

	if (local->scan_sdata == sdata)
		ieee80211_scan_cancel(local);
	else if(local->pending_scan_sdata&&(local->pending_scan_sdata == sdata))
	{
		printk("cancle pendding scan request\n");
		local->scan_sdata = local->pending_scan_sdata;
		local->scan_req = local->pending_scan_req;
		local->pending_scan_sdata = NULL;
		local->pending_scan_req = NULL;
		ieee80211_scan_cancel(local);
	}
		

	/*
	 * Stop TX on this interface first.
	 */
	netif_tx_stop_all_queues(sdata->dev);

	ieee80211_roc_purge(sdata);
	/*
	 * Purge work for this interface.
	 */
	ieee80211_work_purge(sdata);

#ifdef ATBM_AP_SME
	ieee80211_ap_sme_mlme_purge(sdata);
	ieee80211_ap_sme_event_purge(sdata);
#endif
	/*
	 * Remove all stations associated with this interface.
	 *
	 * This must be done before calling ops->remove_interface()
	 * because otherwise we can later invoke ops->sta_notify()
	 * whenever the STAs are removed, and that invalidates driver
	 * assumptions about always getting a vif pointer that is valid
	 * (because if we remove a STA after ops->remove_interface()
	 * the driver will have removed the vif info already!)
	 *
	 * This is relevant only in AP, WDS and mesh modes, since in
	 * all other modes we've already removed all stations when
	 * disconnecting etc.
	 */
	sta_info_flush(local, sdata);

	netif_addr_lock_bh(sdata->dev);
	spin_lock_bh(&local->filter_lock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
	__hw_addr_unsync(&sdata->mc_list, &sdata->dev->mc,
			 sdata->dev->addr_len);
#else
	__dev_addr_unsync(&sdata->mc_list, &sdata->mc_count,
			  &sdata->dev->mc_list, &sdata->dev->mc_count);
#endif
	spin_unlock_bh(&local->filter_lock);
	netif_addr_unlock_bh(sdata->dev);

	ieee80211_configure_filter(sdata);

	del_timer_sync(&sdata->dynamic_ps_timer);
	cancel_work_sync(&sdata->dynamic_ps_enable_work);

	/* APs need special treatment */
	if (sdata->vif.type == NL80211_IFTYPE_AP) {
		struct ieee80211_sub_if_data *vlan, *tmpsdata;
		struct beacon_data *old_beacon =
			rtnl_dereference(sdata->u.ap.beacon);

		/* sdata_running will return false, so this will disable */
		ieee80211_bss_info_change_notify(sdata,
						 BSS_CHANGED_BEACON_ENABLED);

		/* remove beacon */
		RCU_INIT_POINTER(sdata->u.ap.beacon, NULL);
		synchronize_rcu();
		atbm_kfree(old_beacon);

		/* down all dependent devices, that is VLANs */
		list_for_each_entry_safe(vlan, tmpsdata, &sdata->u.ap.vlans,
					 u.vlan.list)
			dev_close(vlan->dev);
		WARN_ON(!list_empty(&sdata->u.ap.vlans));

		/* free all potentially still buffered bcast frames */
		local->total_ps_buffered -= atbm_skb_queue_len(&sdata->u.ap.ps_bc_buf);
		atbm_skb_queue_purge(&sdata->u.ap.ps_bc_buf);
	}

	if (going_down)
		local->open_count--;

	switch (sdata->vif.type) {
	case NL80211_IFTYPE_AP_VLAN:
		list_del(&sdata->u.vlan.list);
		/* no need to tell driver */
		break;
	case NL80211_IFTYPE_MONITOR:
		if (sdata->u.mntr_flags & MONITOR_FLAG_COOK_FRAMES) {
			local->cooked_mntrs--;
			break;
		}

		local->monitors--;
		if (local->monitors == 0) {
			local->hw.conf.flags &= ~IEEE80211_CONF_MONITOR;
			hw_reconf_flags |= IEEE80211_CONF_CHANGE_MONITOR;
		}

		ieee80211_adjust_monitor_flags(sdata, -1);
		ieee80211_configure_filter(sdata);
		#ifdef ATBM_SUPPORT_PKG_MONITOR
		if(strstr(sdata->name,"mon") != NULL){
			break;
		}
		local->only_monitors--;
		local->monitor_sdata = NULL;
		#else
		break;
		#endif
	default:
		flush_work(&sdata->work);
		/*
		 * When we get here, the interface is marked down.
		 * Call synchronize_rcu() to wait for the RX path
		 * should it be using the interface and enqueuing
		 * frames at this very time on another CPU.
		 */
		synchronize_rcu();
		atbm_skb_queue_purge(&sdata->skb_queue);
		#ifdef ATBM_AP_SME
		ieee80211_ap_sme_mlme_purge(sdata);
		ieee80211_ap_sme_event_purge(sdata);
		#endif
		/*
		 * Disable beaconing here for mesh only, AP and IBSS
		 * are already taken care of.
		 */
		if (sdata->vif.type == NL80211_IFTYPE_MESH_POINT)
			ieee80211_bss_info_change_notify(sdata,
				BSS_CHANGED_BEACON_ENABLED);

		/*
		 * Free all remaining keys, there shouldn't be any,
		 * except maybe group keys in AP more or WDS?
		 */
		ieee80211_free_keys(sdata);

		if (going_down)
			drv_remove_interface(local, &sdata->vif);
	}

	sdata->bss = NULL;
#ifdef CONFIG_MAC80211_BRIDGE
	//void ieee80211_brigde_flush(_adapter *priv);
	ieee80211_brigde_flush(sdata);
#endif	// CONFIG_MAC80211_BRIDGE

	mutex_lock(&local->mtx);
	hw_reconf_flags |= __ieee80211_recalc_idle(local);
	mutex_unlock(&local->mtx);

	ieee80211_recalc_ps(local, -1);

	if (local->open_count == 0) {
		#ifdef IEEE80211_SUPPORT_NAPI
		if (local->ops->napi_poll)
			napi_disable(&local->napi);
		#endif
		ieee80211_clear_tx_pending(local);
		ieee80211_stop_device(local);

		/* no reconfiguring after stop! */
		hw_reconf_flags = 0;
	}

	/* Re-calculate channel-type, in case there are multiple vifs
	 * on different channel types.
	 */
	orig_ct = chan_state->_oper_channel_type;
	if (local->hw.flags & IEEE80211_HW_SUPPORTS_MULTI_CHANNEL)
		ieee80211_set_channel_type(local, sdata, NL80211_CHAN_NO_HT);
	else	
		ieee80211_set_channel_type(local, NULL, NL80211_CHAN_NO_HT);

	/* do after stop to avoid reconfiguring when we stop anyway */
	if (hw_reconf_flags || (orig_ct != chan_state->_oper_channel_type))
		ieee80211_hw_config(local, hw_reconf_flags);

	spin_lock_irqsave(&local->queue_stop_reason_lock, flags);
	for (i = 0; i < IEEE80211_MAX_QUEUES; i++) {
		atbm_skb_queue_walk_safe(&local->pending[i], skb, tmp) {
			struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
			if (info->control.vif == &sdata->vif) {
				__atbm_skb_unlink(skb, &local->pending[i]);
				atbm_dev_kfree_skb_irq(skb);
			}
		}
	}
	spin_unlock_irqrestore(&local->queue_stop_reason_lock, flags);

}

static int ieee80211_stop(struct net_device *dev)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	ieee80211_do_stop(sdata, true);

	return 0;
}

static void ieee80211_set_multicast_list(struct net_device *dev)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	int allmulti, promisc, sdata_allmulti, sdata_promisc;

#if 0
	if (!(SDATA_STATE_RUNNING & sdata->state))
		return;
#endif

	allmulti = !!(dev->flags & IFF_ALLMULTI);
	promisc = !!(dev->flags & IFF_PROMISC);
	sdata_allmulti = !!(sdata->flags & IEEE80211_SDATA_ALLMULTI);
	sdata_promisc = !!(sdata->flags & IEEE80211_SDATA_PROMISC);

	if (allmulti != sdata_allmulti)
		sdata->flags ^= IEEE80211_SDATA_ALLMULTI;

	if (promisc != sdata_promisc)
		sdata->flags ^= IEEE80211_SDATA_PROMISC;

	spin_lock_bh(&local->filter_lock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
	__hw_addr_sync(&sdata->mc_list, &dev->mc, dev->addr_len);
#else
	__dev_addr_sync(&sdata->mc_list, &sdata->mc_count,
			&dev->mc_list, &dev->mc_count);
#endif
	spin_unlock_bh(&local->filter_lock);
	ieee80211_queue_work(&local->hw, &sdata->reconfig_filter);
}

/*
 * Called when the netdev is removed or, by the code below, before
 * the interface type changes.
 */
static void ieee80211_teardown_sdata(struct net_device *dev)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	int flushed;
	int i;

	/* free extra data */
	ieee80211_free_keys(sdata);

	ieee80211_debugfs_remove_netdev(sdata);

	for (i = 0; i < IEEE80211_FRAGMENT_MAX; i++)
		__atbm_skb_queue_purge(&sdata->fragments[i].skb_list);
	sdata->fragment_next = 0;

	if (ieee80211_vif_is_mesh(&sdata->vif))
		mesh_rmc_free(sdata);

	flushed = sta_info_flush(local, sdata);
	WARN_ON(flushed);
}

static u16 ieee80211_netdev_select_queue(struct net_device *dev,
                                         struct sk_buff *skb
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
					,
                                         void *accel_priv,
                                         select_queue_fallback_t fallback
#endif
					 )

{
	return ieee80211_select_queue(IEEE80211_DEV_TO_SUB_IF(dev), skb);
}
/*
*CONFIG_ATBM_IOCTRL : use net ioctrl to process some cmd
*/
#if defined(CONFIG_ATBM_IOCTRL)

typedef struct android_wifi_priv_cmd_s {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

struct altm_msg{
	unsigned int type;
	unsigned int value;
	unsigned int externData[30];
};

enum ANDROID_WIFI_CMD {
	ANDROID_WIFI_CMD_START,				
	ANDROID_WIFI_CMD_STOP,			
	ANDROID_WIFI_CMD_SCAN_ACTIVE,
	ANDROID_WIFI_CMD_SCAN_PASSIVE,		
	ANDROID_WIFI_CMD_RSSI,	
	ANDROID_WIFI_CMD_LINKSPEED,
	ANDROID_WIFI_CMD_RXFILTER_START,
	ANDROID_WIFI_CMD_RXFILTER_STOP,	
	ANDROID_WIFI_CMD_RXFILTER_ADD,	
	ANDROID_WIFI_CMD_RXFILTER_REMOVE,
	ANDROID_WIFI_CMD_BTCOEXSCAN_START,
	ANDROID_WIFI_CMD_BTCOEXSCAN_STOP,
	ANDROID_WIFI_CMD_BTCOEXMODE,
	ANDROID_WIFI_CMD_SETSUSPENDOPT,
	ANDROID_WIFI_CMD_P2P_DEV_ADDR,	
	ANDROID_WIFI_CMD_SETFWPATH,		
	ANDROID_WIFI_CMD_SETBAND,		
	ANDROID_WIFI_CMD_GETBAND,			
	ANDROID_WIFI_CMD_COUNTRY,			
	ANDROID_WIFI_CMD_P2P_SET_NOA,
	ANDROID_WIFI_CMD_P2P_GET_NOA,	
	ANDROID_WIFI_CMD_P2P_SET_PS,	
	ANDROID_WIFI_CMD_SET_AP_WPS_P2P_IE,
#ifdef CONFIG_PNO_SUPPORT
	ANDROID_WIFI_CMD_PNOSSIDCLR_SET,
	ANDROID_WIFI_CMD_PNOSETUP_SET,
	ANDROID_WIFI_CMD_PNOENABLE_SET,
	ANDROID_WIFI_CMD_PNODEBUG_SET,
#endif

	ANDROID_WIFI_CMD_MACADDR,

	ANDROID_WIFI_CMD_BLOCK,

	ANDROID_WIFI_CMD_WFD_ENABLE,
	ANDROID_WIFI_CMD_WFD_DISABLE,
	
	ANDROID_WIFI_CMD_WFD_SET_TCPPORT,
	ANDROID_WIFI_CMD_WFD_SET_MAX_TPUT,
	ANDROID_WIFI_CMD_WFD_SET_DEVTYPE,
	ANDROID_WIFI_CMD_CHANGE_DTIM,
	ANDROID_WIFI_CMD_HOSTAPD_SET_MACADDR_ACL,
	ANDROID_WIFI_CMD_HOSTAPD_ACL_ADD_STA,
	ANDROID_WIFI_CMD_HOSTAPD_ACL_REMOVE_STA,
#ifdef CONFIG_GTK_OL
	ANDROID_WIFI_CMD_GTK_REKEY_OFFLOAD,
#endif //CONFIG_GTK_OL
	ANDROID_WIFI_CMD_P2P_DISABLE,
	ANDROID_WIFI_CMD_MAX
};

static const char *android_wifi_cmd_str[ANDROID_WIFI_CMD_MAX] = {
	"START",
	"STOP",
	"SCAN-ACTIVE",
	"SCAN-PASSIVE",
	"RSSI",
	"LINKSPEED",
	"RXFILTER-START",
	"RXFILTER-STOP",
	"RXFILTER-ADD",
	"RXFILTER-REMOVE",
	"BTCOEXSCAN-START",
	"BTCOEXSCAN-STOP",
	"BTCOEXMODE",
	"SETSUSPENDOPT",
	"P2P_DEV_ADDR",
	"SETFWPATH",
	"SETBAND",
	"GETBAND",
	"COUNTRY",
	"P2P_SET_NOA",
	"P2P_GET_NOA",
	"P2P_SET_PS",
	"SET_AP_WPS_P2P_IE",
#ifdef CONFIG_PNO_SUPPORT
	"PNOSSIDCLR",
	"PNOSETUP",
	"PNOFORCE",
	"PNODEBUG",
#endif

	"MACADDR",

	"BLOCK",
	"WFD-ENABLE",
	"WFD-DISABLE",
	"WFD-SET-TCPPORT",
	"WFD-SET-MAXTPUT",
	"WFD-SET-DEVTYPE",
	"SET_DTIM",
	"HOSTAPD_SET_MACADDR_ACL",
	"HOSTAPD_ACL_ADD_STA",
	"HOSTAPD_ACL_REMOVE_STA",
#ifdef CONFIG_GTK_OL
	"GTK_REKEY_OFFLOAD",
#endif //CONFIG_GTK_OL
/*	Private command for	P2P disable*/
	"P2P_DISABLE"
};

#define IEEE80211_CMD_GET(cmdname,index) (cmdname##_buff[index] == NULL ? \
	cmdname##_default:cmdname##_buff[index])
typedef int (*android_cmd_handle)(struct ieee80211_sub_if_data *sdata,char *cmd,u32 cmd_len,u8 cmd_index);

int ieee80211_android_cmd_handle_default(struct ieee80211_sub_if_data *sdata, 
	char *cmd,u32 cmd_len,u8 cmd_index)
{
	int ret = 0;
	printk(KERN_DEBUG "%s:android cmd-->(%s) unhandle\n",__func__,cmd);

	return ret;
}

static const android_cmd_handle ieee80211_android_cmd_handle_buff[] = {
	/*ANDROID_WIFI_CMD_START*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_STOP*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_SCAN_ACTIVE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_SCAN_PASSIVE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_RSSI*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_LINKSPEED*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_RXFILTER_START*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_RXFILTER_STOP*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_RXFILTER_ADD*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_RXFILTER_REMOVE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_BTCOEXSCAN_START*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_BTCOEXSCAN_STOP*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_BTCOEXMODE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_SETSUSPENDOPT*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_P2P_DEV_ADDR*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_SETFWPATH*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_SETBAND*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_GETBAND*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_COUNTRY*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_P2P_SET_NOA*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_P2P_GET_NOA*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_P2P_SET_PS*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_SET_AP_WPS_P2P_IE*/
	ieee80211_android_cmd_handle_default,
#ifdef CONFIG_PNO_SUPPORT
	/*ANDROID_WIFI_CMD_PNOSSIDCLR_SET*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_PNOSETUP_SET*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_PNOENABLE_SET*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_PNODEBUG_SET*/
	ieee80211_android_cmd_handle_default,
#endif

	/*ANDROID_WIFI_CMD_MACADDR*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_BLOCK*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_WFD_ENABLE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_WFD_DISABLE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_WFD_SET_TCPPORT*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_WFD_SET_MAX_TPUT*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_WFD_SET_DEVTYPE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_CHANGE_DTIM*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_HOSTAPD_SET_MACADDR_ACL*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_HOSTAPD_ACL_ADD_STA*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_HOSTAPD_ACL_REMOVE_STA*/
	ieee80211_android_cmd_handle_default,
#ifdef CONFIG_GTK_OL
	/*ANDROID_WIFI_CMD_GTK_REKEY_OFFLOAD*/
	ieee80211_android_cmd_handle_default,
#endif //CONFIG_GTK_OL
	/*ANDROID_WIFI_CMD_P2P_DISABLE*/
	ieee80211_android_cmd_handle_default,
	/*ANDROID_WIFI_CMD_MAX*/
	ieee80211_android_cmd_handle_default

 };

#define IEEE80211_NETDEV_BASE_CMD			SIOCDEVPRIVATE
#define IEEE80211_NETDEV_WEXT_CMD			SIOCDEVPRIVATE
#define IEEE80211_NETDEV_ANDROID_CMD		(SIOCDEVPRIVATE+1)
#define IEEE80211_NETDEV_VENDOR_CMD			(SIOCDEVPRIVATE+2)

int ieee80211_netdev_cmd_handle_default(struct net_device *dev, 
	struct ifreq *rq)
{
	int ret = 0;
	printk(KERN_ERR "%s\n",__func__);
	return ret;
}
int ieee80211_android_cmdstr_to_num(char *cmdstr)
{
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	#define strnicmp	strncasecmp
	#endif /* Linux kernel >= 4.0.0 */
	
	int cmd_num;
	for(cmd_num=0 ; cmd_num<ANDROID_WIFI_CMD_MAX; cmd_num++)
		if(0 == strnicmp(cmdstr , android_wifi_cmd_str[cmd_num], 
			strlen(android_wifi_cmd_str[cmd_num])) )
			break;

	return cmd_num;
}
/*
* process wext cmd ,but now........
*/
int ieee80211_netdev_process_wext_cmd(struct net_device *dev, struct ifreq *rq)
{
	int ret = 0;
	printk(KERN_DEBUG "%s\n",__func__);
	return ret;
}
/*
*process android cmd,only retrun success
*/
int ieee80211_netdev_process_android_cmd(struct net_device *dev, struct ifreq *rq)
{	
#if 0
	char *android_cmd = NULL;
	int cmd_num = ANDROID_WIFI_CMD_MAX;
	android_cmd_handle call_fn = NULL;
	int ret=0;
	android_wifi_priv_cmd android_cmd_struct;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	
	if (!rq->ifr_data) 
	{
		printk(KERN_ERR "ifr->ifr_data == NULL\n");
		ret = -EINVAL;
		goto exit;
	}

	memset(&android_cmd_struct,0,sizeof(android_wifi_priv_cmd));
	
	if (copy_from_user(&android_cmd_struct, rq->ifr_data, sizeof(android_wifi_priv_cmd))) 
	{
		printk(KERN_ERR "copy_from_user err\n");
		ret = -EFAULT;
		goto exit;
	}
	if(android_cmd_struct.total_len<=0)
	{
		printk(KERN_ERR "android_cmd_struct.total_len<=0\n");
		ret = -EINVAL;
		goto exit;
	}
	android_cmd = atbm_kmalloc(android_cmd_struct.total_len,GFP_KERNEL);

	if(android_cmd == NULL)
	{
		printk(KERN_ERR "atbm_kmalloc err\n");
		ret = -EFAULT;
		goto exit;
	}

	memset(android_cmd,0,android_cmd_struct.total_len);

	if (copy_from_user(android_cmd, (void *)android_cmd_struct.buf, 
		android_cmd_struct.total_len)) 
	{
		printk(KERN_ERR "copy_from_user android_cmd err\n");
		ret = -EFAULT;
		goto exit;
	}
	if(android_cmd == NULL)
	{
		printk(KERN_ERR "android_cmd == NULL\n");
		ret = -EFAULT;
		goto exit;
	}
	printk(KERN_ERR "%s\n",__func__);
	cmd_num = ieee80211_android_cmdstr_to_num(android_cmd);

	if(cmd_num >= ANDROID_WIFI_CMD_MAX)
	{
		printk(KERN_ERR "no cmd found\n");
		ret = -EFAULT;
		goto exit;
	}

	call_fn = IEEE80211_CMD_GET(ieee80211_android_cmd_handle,cmd_num);
	
	ret = call_fn(sdata,android_cmd,android_cmd_struct.total_len,cmd_num);
exit:
	if(android_cmd)
		atbm_kfree(android_cmd);
	return ret;
#else

	return 0;
#endif
}
extern int rate_altm_control_test(struct wiphy *wiphy, void *data, int len);
extern int atbm_altmtest_cmd(struct ieee80211_hw *hw, void *data, int len);
/*
*vendor cmd ----> atbm_tool
*/
int ieee80211_netdev_process_vendor_cmd(struct net_device *dev, struct ifreq *rq)
{
	struct altm_msg vendor_msg;
	int ret=0;
	int i;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	int len = 0;
	
	if (!rq->ifr_data) 
	{
		printk(KERN_ERR "%s:ifr->ifr_data == NULL\n",__func__);
		ret = -EINVAL;
		goto exit;
	}
	
	memset(&vendor_msg,0,sizeof(struct altm_msg));
	
	if (copy_from_user(&vendor_msg, rq->ifr_data, sizeof(struct altm_msg))) 
	{
		printk(KERN_ERR "copy_from_user err\n");
		ret = -EFAULT;
		goto exit;
	}
	len = sizeof(struct altm_msg);
	printk("type = %d, value = %d\n", vendor_msg.type, vendor_msg.value);
	for(i=0;i<30;i++)
		printk("%d ", vendor_msg.externData[i]);
	sdata->local->hw.vendcmd_nl80211 = 1;
	ret = rate_altm_control_test(sdata->local->hw.wiphy,&vendor_msg,len);
	ret |= atbm_altmtest_cmd(&sdata->local->hw, &vendor_msg, len); 
	
	if((ret > 0)&&(ret<=len))
	{
		if (copy_to_user(rq->ifr_data,(u8*)(&sdata->local->hw.vendreturn),sizeof(struct response)))
		{
			printk(KERN_ERR "copy_to_user err\n");
			ret = -EFAULT;
		}
		else
			ret = 0;
		goto exit;
	}
exit:
	sdata->local->hw.vendcmd_nl80211 = 0;
	return ret;	
}

typedef int (* ieee80211_netdev_cmd_handle)(struct net_device *dev, struct ifreq *rq);
ieee80211_netdev_cmd_handle ieee80211_netdev_cmd_handle_buff[]={
	ieee80211_netdev_process_wext_cmd,
	ieee80211_netdev_process_android_cmd,
	ieee80211_netdev_process_vendor_cmd
};

int ieee80211_netdev_ioctrl(struct net_device *dev, struct ifreq *rq, int cmd)
{	
	int ret = 0;
	ieee80211_netdev_cmd_handle call_func = NULL;

	if((cmd > IEEE80211_NETDEV_VENDOR_CMD) 
		||
		(cmd < IEEE80211_NETDEV_BASE_CMD) )
	{
		printk(KERN_ERR "%s:cmd err\n",__func__);
		ret = -EINVAL;

		goto exit;
	}
	call_func = IEEE80211_CMD_GET(ieee80211_netdev_cmd_handle,cmd - IEEE80211_NETDEV_BASE_CMD);
	ret = call_func(dev,rq);
	
exit:
	return ret;
}
#endif
static const struct net_device_ops ieee80211_dataif_ops = {
	.ndo_open		= ieee80211_open,
	.ndo_stop		= ieee80211_stop,
	.ndo_uninit		= ieee80211_teardown_sdata,
	.ndo_start_xmit		= ieee80211_subif_start_xmit,
	.ndo_set_rx_mode	= ieee80211_set_multicast_list,
	.ndo_change_mtu 	= ieee80211_change_mtu,
	.ndo_set_mac_address 	= ieee80211_change_mac,
	.ndo_select_queue	= ieee80211_netdev_select_queue,
#if defined(CONFIG_ATBM_IOCTRL)
	.ndo_do_ioctl = ieee80211_netdev_ioctrl,
#endif
};

static u16 ieee80211_monitor_select_queue(struct net_device *dev,
                                          struct sk_buff *skb
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
                                          ,
                                          void *accel_priv,
                                          select_queue_fallback_t fallback
#endif
)

{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hdr *hdr;
	struct ieee80211_radiotap_header *rtap = (void *)skb->data;
	u8 *p;

	if (local->hw.queues < IEEE80211_NUM_ACS)
		return 0;

	if (skb->len < 4 ||
	    skb->len < le16_to_cpu(rtap->it_len) + 2 /* frame control */)
		return 0; /* doesn't matter, frame will be dropped */

	hdr = (void *)((u8 *)skb->data + le16_to_cpu(rtap->it_len));

	if (!ieee80211_is_data(hdr->frame_control)) {
		skb->priority = 7;
		return ieee802_1d_to_ac[skb->priority];
	}
	if (!ieee80211_is_data_qos(hdr->frame_control)) {
		skb->priority = 0;
		return ieee802_1d_to_ac[skb->priority];
	}

	p = ieee80211_get_qos_ctl(hdr);
	skb->priority = *p & IEEE80211_QOS_CTL_TAG1D_MASK;

	return ieee80211_downgrade_queue(local, skb);
}

static const struct net_device_ops ieee80211_monitorif_ops = {
	.ndo_open		= ieee80211_open,
	.ndo_stop		= ieee80211_stop,
	.ndo_uninit		= ieee80211_teardown_sdata,
	.ndo_start_xmit		= ieee80211_monitor_start_xmit,
	.ndo_set_rx_mode	= ieee80211_set_multicast_list,
	.ndo_change_mtu 	= ieee80211_change_mtu,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_select_queue	= ieee80211_monitor_select_queue,
};

static void ieee80211_if_setup(struct net_device *dev)
{
	ether_setup(dev);
	dev->priv_flags &= ~IFF_TX_SKB_SHARING;
	netdev_attach_ops(dev, &ieee80211_dataif_ops);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29))
	/* Do we need this ? */
	/* we will validate the address ourselves in ->open */
	dev->validate_addr = NULL;
#endif
	dev->destructor = free_netdev;
}

static void ieee80211_iface_work(struct work_struct *work)
{
	struct ieee80211_sub_if_data *sdata =
		container_of(work, struct ieee80211_sub_if_data, work);
	struct ieee80211_local *local = sdata->local;
	struct sk_buff *skb;
	struct sta_info *sta;
	struct ieee80211_ra_tid *ra_tid;

	if (!ieee80211_sdata_running(sdata))
		return;

	if (local->scanning)
		return;

	/*
	 * ieee80211_queue_work() should have picked up most cases,
	 * here we'll pick the rest.
	 */
	if (WARN(local->suspended,
		 "interface work scheduled while going to suspend\n"))
		return;

	/* first process frames */
	while ((skb = atbm_skb_dequeue(&sdata->skb_queue))) {
		struct atbm_ieee80211_mgmt *mgmt = (void *)skb->data;

		if (skb->pkt_type == IEEE80211_SDATA_QUEUE_AGG_START) {
			ra_tid = (void *)&skb->cb;
			ieee80211_start_tx_ba_cb(&sdata->vif, ra_tid->ra,
						 ra_tid->tid);
		} else if (skb->pkt_type == IEEE80211_SDATA_QUEUE_AGG_STOP) {
			ra_tid = (void *)&skb->cb;
			ieee80211_stop_tx_ba_cb(&sdata->vif, ra_tid->ra,
						ra_tid->tid);
		} else if (ieee80211_is_action(mgmt->frame_control) &&
			   mgmt->u.action.category == WLAN_CATEGORY_BACK) {
			int len = skb->len;

			mutex_lock(&local->sta_mtx);
			sta = sta_info_get_bss(sdata, mgmt->sa);
			if (sta) {
				switch (mgmt->u.action.u.addba_req.action_code) {
				case WLAN_ACTION_ADDBA_REQ:
					ieee80211_process_addba_request(
							local, sta, mgmt, len);
					break;
				case WLAN_ACTION_ADDBA_RESP:
					ieee80211_process_addba_resp(local, sta,
								     mgmt, len);
					break;
				case WLAN_ACTION_DELBA:
					ieee80211_process_delba(sdata, sta,
								mgmt, len);
					break;
				default:
					WARN_ON(1);
					break;
				}
			}
			mutex_unlock(&local->sta_mtx);
		} else if (ieee80211_is_data_qos(mgmt->frame_control)) {
			struct ieee80211_hdr *hdr = (void *)mgmt;
			/*
			 * So the frame isn't mgmt, but frame_control
			 * is at the right place anyway, of course, so
			 * the if statement is correct.
			 *
			 * Warn if we have other data frame types here,
			 * they must not get here.
			 */
			WARN_ON(hdr->frame_control &
					cpu_to_le16(IEEE80211_STYPE_NULLFUNC));
			WARN_ON(!(hdr->seq_ctrl &
					cpu_to_le16(IEEE80211_SCTL_FRAG)));
			/*
			 * This was a fragment of a frame, received while
			 * a block-ack session was active. That cannot be
			 * right, so terminate the session.
			 */
			mutex_lock(&local->sta_mtx);
			sta = sta_info_get_bss(sdata, mgmt->sa);
			if (sta) {
				u16 tid = *ieee80211_get_qos_ctl(hdr) &
						IEEE80211_QOS_CTL_TID_MASK;

				__ieee80211_stop_rx_ba_session(
					sta, tid, WLAN_BACK_RECIPIENT,
					WLAN_REASON_QSTA_REQUIRE_SETUP,
					true);
			}
			mutex_unlock(&local->sta_mtx);
		} else {
			switch (sdata->vif.type) {
			case NL80211_IFTYPE_AP:
			case NL80211_IFTYPE_P2P_GO:
				if (ieee80211_is_beacon(mgmt->frame_control)) {
					struct ieee80211_bss_conf *bss_conf =
						&sdata->vif.bss_conf;
					u32 bss_info_changed = 0, erp = 0;
					struct ieee80211_channel *channel =
						ieee80211_get_channel(local->hw.wiphy,
								IEEE80211_SKB_RXCB(skb)->freq);
					if(cfg80211_inform_bss_frame(local->hw.wiphy, channel,
							(struct ieee80211_mgmt *)mgmt, skb->len,
							IEEE80211_SKB_RXCB(skb)->signal, GFP_ATOMIC))
					#if 0
					
					erp = cfg80211_get_local_erp(local->hw.wiphy,
							IEEE80211_SKB_RXCB(skb)->freq);
					#else
					{
						const u8 *p;
						int tmp = 0;
						p = cfg80211_find_ie(ATBM_WLAN_EID_SUPP_RATES,
						mgmt->u.beacon.variable,skb->len-
						offsetof(struct atbm_ieee80211_mgmt,u.probe_resp.variable));
						if (p) {
							/* Check for pure 11b access poin */
							if (p[1] == 4)
								erp |= WLAN_ERP_USE_PROTECTION;
						}

						p = cfg80211_find_ie(ATBM_WLAN_EID_ERP_INFO,
								mgmt->u.beacon.variable, skb->len-
						offsetof(struct atbm_ieee80211_mgmt,u.probe_resp.variable));
						if (!p)
							continue;
						tmp = p[ERP_INFO_BYTE_OFFSET];
						erp |= tmp;
					}

					#endif
					
					if (!!bss_conf->use_cts_prot !=
							!!(erp & WLAN_ERP_USE_PROTECTION)) {
						bss_conf->use_cts_prot =
							!!(erp & WLAN_ERP_USE_PROTECTION);
						bss_info_changed |= BSS_CHANGED_ERP_CTS_PROT;
					}
					ieee80211_bss_info_change_notify(sdata,
							bss_info_changed);
				};
				break;
			case NL80211_IFTYPE_STATION:
				ieee80211_sta_rx_queued_mgmt(sdata, skb);
				break;
			case NL80211_IFTYPE_ADHOC:
				ieee80211_ibss_rx_queued_mgmt(sdata, skb);
				break;
			case NL80211_IFTYPE_MESH_POINT:
				if (!ieee80211_vif_is_mesh(&sdata->vif))
					break;
				ieee80211_mesh_rx_queued_mgmt(sdata, skb);
				break;
			default:
				WARN(1, "frame for unexpected interface type");
				break;
			}

		}
		atbm_kfree_skb(skb);
	}

	/* then other type-dependent work */
	switch (sdata->vif.type) {
	case NL80211_IFTYPE_STATION:
		ieee80211_sta_work(sdata);
		break;
	case NL80211_IFTYPE_ADHOC:
		ieee80211_ibss_work(sdata);
		break;
	case NL80211_IFTYPE_MESH_POINT:
		if (!ieee80211_vif_is_mesh(&sdata->vif))
			break;
		ieee80211_mesh_work(sdata);
		break;
	default:
		break;
	}
}

static void ieee80211_reconfig_filter(struct work_struct *work)
{
	struct ieee80211_sub_if_data *sdata =
		container_of(work, struct ieee80211_sub_if_data,
			     reconfig_filter);

	ieee80211_configure_filter(sdata);
}

/*
 * Helper function to initialise an interface to a specific type.
 */
static void ieee80211_setup_sdata(struct ieee80211_sub_if_data *sdata,
				  enum nl80211_iftype type)
{
	/* clear type-dependent union */
	memset(&sdata->u, 0, sizeof(sdata->u));
	atomic_set(&sdata->connectting,IEEE80211_ATBM_CONNECT_DONE);
#ifdef CONFIG_MAC80211_ATBM_ROAMING_CHANGES
	sdata->queues_locked = 0;
#endif
	/* and set some type-dependent values */
	sdata->vif.type = type;
	sdata->vif.p2p = false;
	netdev_attach_ops(sdata->dev, &ieee80211_dataif_ops);
	sdata->wdev.iftype = type;

	sdata->control_port_protocol = cpu_to_be16(ETH_P_PAE);
	sdata->control_port_no_encrypt = false;

	/* only monitor differs */
	sdata->dev->type = ARPHRD_ETHER;

	atbm_skb_queue_head_init(&sdata->skb_queue);
	INIT_WORK(&sdata->work, ieee80211_iface_work);
	INIT_WORK(&sdata->reconfig_filter, ieee80211_reconfig_filter);
#ifdef ATBM_AP_SME
	ieee80211_ap_sme_queue_mgmt_init(sdata);
	ieee80211_ap_sme_event_init(sdata);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))

	__hw_addr_init(&sdata->mc_list);

#endif


	/*
	 * Initialize wiphy parameters to IEEE 802.11 MIB default values.
	 * RTS threshold is disabled by default with the special -1 value.
	 */
	sdata->vif.bss_conf.retry_short = sdata->wdev.wiphy->retry_short = 7;
	sdata->vif.bss_conf.retry_long =  sdata->wdev.wiphy->retry_long = 4;
	sdata->vif.bss_conf.rts_threshold =  sdata->wdev.wiphy->rts_threshold = (u32) -1;

	switch (type) {
	case NL80211_IFTYPE_P2P_GO:
		type = NL80211_IFTYPE_AP;
		sdata->vif.type = type;
		sdata->vif.p2p = true;
		/* fall through */
	case NL80211_IFTYPE_AP:
		atbm_skb_queue_head_init(&sdata->u.ap.ps_bc_buf);
		INIT_LIST_HEAD(&sdata->u.ap.vlans);
		sdata->local->uapsd_queues = IEEE80211_DEFAULT_UAPSD_QUEUES;
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
		type = NL80211_IFTYPE_STATION;
		sdata->vif.type = type;
		sdata->vif.p2p = true;
		/* fall through */
	case NL80211_IFTYPE_STATION:
		ieee80211_sta_setup_sdata(sdata);
		break;
	case NL80211_IFTYPE_ADHOC:
		ieee80211_ibss_setup_sdata(sdata);
		break;
	case NL80211_IFTYPE_MESH_POINT:
		if (ieee80211_vif_is_mesh(&sdata->vif))
			ieee80211_mesh_init_sdata(sdata);
		break;
	case NL80211_IFTYPE_MONITOR:
		{
			#ifdef	ATBM_WIFI_QUEUE_LOCK_BUG
			struct ieee80211_sub_if_data *attach_sdata = NULL;
			
			list_for_each_entry(attach_sdata, &sdata->local->interfaces, list){
				if(attach_sdata == sdata)
					continue;
				if(strstr(sdata->name,attach_sdata->name) == NULL)
					continue;
				printk(KERN_ERR "%s:add[%s],attach[%s]\n",__func__,sdata->name,attach_sdata->name);
				memcpy(sdata->vif.hw_queue,attach_sdata->vif.hw_queue,IEEE80211_NUM_ACS);
			}
			#endif
			
			sdata->dev->type = ARPHRD_IEEE80211_RADIOTAP;
			netdev_attach_ops(sdata->dev, &ieee80211_monitorif_ops);
			sdata->u.mntr_flags = MONITOR_FLAG_CONTROL |
					      MONITOR_FLAG_OTHER_BSS;
		}
		break;
	case NL80211_IFTYPE_WDS:
	case NL80211_IFTYPE_AP_VLAN:
		break;
	case NL80211_IFTYPE_UNSPECIFIED:
	case NUM_NL80211_IFTYPES:
		BUG();
		break;
	default:
		break;
	}

	ieee80211_debugfs_add_netdev(sdata);

	drv_bss_info_changed(sdata->local, sdata, &sdata->vif.bss_conf,
			     BSS_CHANGED_RETRY_LIMITS);
}

/*
 * Helper function to initialise an interface to a specific type.
 */
static void ieee80211_delete_sdata(struct ieee80211_sub_if_data *sdata)
{
	switch (sdata->vif.type) {
	case NL80211_IFTYPE_P2P_GO:
	case NL80211_IFTYPE_AP:
		break;
		
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_STATION:
		{
		struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;		
		
		del_timer_sync(&ifmgd->bcn_mon_timer);
		del_timer_sync(&ifmgd->conn_mon_timer);
		del_timer_sync(&ifmgd->chswitch_timer);
		del_timer_sync(&ifmgd->timer);
		}
		break;
	default:
		break;
	}
}

static int ieee80211_runtime_change_iftype(struct ieee80211_sub_if_data *sdata,
					   enum nl80211_iftype type)
{
	struct ieee80211_local *local = sdata->local;
	int ret, err;
	enum nl80211_iftype internal_type = type;
	bool p2p = false;

	ASSERT_RTNL();

	if (!local->ops->change_interface)
		return -EBUSY;

	switch (sdata->vif.type) {
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_ADHOC:
		/*
		 * Could maybe also all others here?
		 * Just not sure how that interacts
		 * with the RX/config path e.g. for
		 * mesh.
		 */
		break;
	#ifdef	ATBM_SUPPORT_PKG_MONITOR
	case NL80211_IFTYPE_MONITOR:
		if(local->only_monitors)
			break;
		else 
			return -EBUSY;
	#endif
	default:
		return -EBUSY;
	}

	switch (type) {
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_ADHOC:
		/*
		 * Could probably support everything
		 * but WDS here (WDS do_open can fail
		 * under memory pressure, which this
		 * code isn't prepared to handle).
		 */
		break;
	#ifdef	ATBM_SUPPORT_PKG_MONITOR
	case NL80211_IFTYPE_MONITOR:
		if(local->only_monitors)
			break;
		else 
			return -EBUSY;
	#endif
	case NL80211_IFTYPE_P2P_CLIENT:
		p2p = true;
		internal_type = NL80211_IFTYPE_STATION;
		break;
	case NL80211_IFTYPE_P2P_GO:
		p2p = true;
		internal_type = NL80211_IFTYPE_AP;
		break;
	default:
		return -EBUSY;
	}

	ret = ieee80211_check_concurrent_iface(sdata, internal_type);
	if (ret)
		return ret;

	ieee80211_do_stop(sdata, false);

	ieee80211_teardown_sdata(sdata->dev);

	ret = drv_change_interface(local, sdata, internal_type, p2p);
	if (ret)
		type = sdata->vif.type;

	/*
	 * Ignore return value here, there's not much we can do since
	 * the driver changed the interface type internally already.
	 * The warnings will hopefully make driver authors fix it :-)
	 */
	ieee80211_check_queues(sdata);

	ieee80211_setup_sdata(sdata, type);

	err = ieee80211_do_open(sdata->dev, false);
	WARN(err, "type change: do_open returned %d", err);

	return ret;
}

int ieee80211_if_change_type(struct ieee80211_sub_if_data *sdata,
			     enum nl80211_iftype type)
{
	struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(sdata->local, sdata);
	int ret;

	ASSERT_RTNL();

	if (type == ieee80211_vif_type_p2p(&sdata->vif))
		return 0;

	/* Setting ad-hoc mode on non-IBSS channel is not supported. */
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
	if (chan_state->oper_channel->flags & IEEE80211_CHAN_NO_IBSS &&
	    type == NL80211_IFTYPE_ADHOC)
		return -EOPNOTSUPP;
	#endif
	#ifdef	ATBM_SUPPORT_PKG_MONITOR
	if(sdata->local->only_monitors){
		if(sdata->local->monitor_sdata){
			printk(KERN_ERR "%s:[%s] in monitor mode,change to other mode\n",__func__,sdata->local->monitor_sdata->name);
		}
	}
	else if((type == NL80211_IFTYPE_MONITOR)&&(strstr(sdata->name,"mon")==NULL)){
		struct ieee80211_sub_if_data *other_sdata;
		list_for_each_entry(other_sdata, &sdata->local->interfaces, list){
			
			 if ((other_sdata->vif.type != NL80211_IFTYPE_MONITOR)&&
			 	ieee80211_sdata_running(other_sdata)){
			 	printk(KERN_ERR "[%s] is running,so please close [%s]\n",other_sdata->name,other_sdata->name);
				printk(KERN_ERR "and try again later to set[%s] to monitor mode\n",sdata->name);
				return -EOPNOTSUPP;
			 }
		}
	}
	#endif
	if (ieee80211_sdata_running(sdata)) {
		ret = ieee80211_runtime_change_iftype(sdata, type);
		if (ret)
			return ret;
	} else {
		/* Purge and reset type-dependent state. */
		ieee80211_teardown_sdata(sdata->dev);
		ieee80211_setup_sdata(sdata, type);
	}

	/* reset some values that shouldn't be kept across type changes */
	sdata->vif.bss_conf.basic_rates =
		ieee80211_atbm_mandatory_rates(sdata->local,
			chan_state->conf.channel->band);
	sdata->drop_unencrypted = 0;
	if (type == NL80211_IFTYPE_STATION)
		sdata->u.mgd.use_4addr = false;

	return 0;
}

static void ieee80211_assign_perm_addr(struct ieee80211_local *local,
				       struct net_device *dev,
				       enum nl80211_iftype type)
{
	struct ieee80211_sub_if_data *sdata;
	u64 mask, start, addr, val, inc;
	u8 *m;
	u8 tmp_addr[ETH_ALEN];
	int i;

	/* default ... something at least */
	memcpy(dev->perm_addr, local->hw.wiphy->perm_addr, ETH_ALEN);

	if (is_zero_ether_addr(local->hw.wiphy->addr_mask) &&
	    local->hw.wiphy->n_addresses <= 1)
		return;


	mutex_lock(&local->iflist_mtx);

	switch (type) {
	case NL80211_IFTYPE_MONITOR:
		/* doesn't matter */
		break;
	case NL80211_IFTYPE_WDS:
	case NL80211_IFTYPE_AP_VLAN:
		/* match up with an AP interface */
		list_for_each_entry(sdata, &local->interfaces, list) {
			if (sdata->vif.type != NL80211_IFTYPE_AP)
				continue;
			memcpy(dev->perm_addr, sdata->vif.addr, ETH_ALEN);
			break;
		}
		/* keep default if no AP interface present */
		break;
	default:
		/* assign a new address if possible -- try n_addresses first */
		for (i = 0; i < local->hw.wiphy->n_addresses; i++) {
			bool used = false;

			list_for_each_entry(sdata, &local->interfaces, list) {
				if (memcmp(local->hw.wiphy->addresses[i].addr,
					   sdata->vif.addr, ETH_ALEN) == 0) {
					used = true;
					break;
				}
			}

			if (!used) {
				memcpy(dev->perm_addr,
				       local->hw.wiphy->addresses[i].addr,
				       ETH_ALEN);
				break;
			}
		}

		/* try mask if available */
		if (is_zero_ether_addr(local->hw.wiphy->addr_mask))
			break;

		m = local->hw.wiphy->addr_mask;
		mask =	((u64)m[0] << 5*8) | ((u64)m[1] << 4*8) |
			((u64)m[2] << 3*8) | ((u64)m[3] << 2*8) |
			((u64)m[4] << 1*8) | ((u64)m[5] << 0*8);

		if (__ffs64(mask) + hweight64(mask) != fls64(mask)) {
			/* not a contiguous mask ... not handled now! */
			printk(KERN_DEBUG "not contiguous\n");
			break;
		}

		m = local->hw.wiphy->perm_addr;
		start = ((u64)m[0] << 5*8) | ((u64)m[1] << 4*8) |
			((u64)m[2] << 3*8) | ((u64)m[3] << 2*8) |
			((u64)m[4] << 1*8) | ((u64)m[5] << 0*8);

		inc = 1ULL<<__ffs64(mask);
		val = (start & mask);
		addr = (start & ~mask) | (val & mask);
		do {
			bool used = false;

			tmp_addr[5] = addr >> 0*8;
			tmp_addr[4] = addr >> 1*8;
			tmp_addr[3] = addr >> 2*8;
			tmp_addr[2] = addr >> 3*8;
			tmp_addr[1] = addr >> 4*8;
			tmp_addr[0] = addr >> 5*8;

			val += inc;

			list_for_each_entry(sdata, &local->interfaces, list) {
				if (memcmp(tmp_addr, sdata->vif.addr,
							ETH_ALEN) == 0) {
					used = true;
					break;
				}
			}

			if (!used) {
				memcpy(dev->perm_addr, tmp_addr, ETH_ALEN);
				break;
			}
			addr = (start & ~mask) | (val & mask);
		} while (addr != start);

		break;
	}

	mutex_unlock(&local->iflist_mtx);
}
#ifdef CONFIG_WIRELESS_EXT
extern struct iw_handler_def atbm_handlers_def;
#endif
int ieee80211_if_add(struct ieee80211_local *local, const char *name,
		     struct net_device **new_dev, enum nl80211_iftype type,
		     struct vif_params *params)
{
	struct net_device *ndev;
	struct ieee80211_sub_if_data *sdata = NULL;
	int ret, i;
	int txqs = 1;

	ASSERT_RTNL();

	if (local->hw.queues >= IEEE80211_NUM_ACS)
		txqs = IEEE80211_NUM_ACS;
//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,25))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,00))
	ndev = alloc_netdev_mqs(sizeof(*sdata) + local->hw.vif_data_size,
				name,NET_NAME_UNKNOWN, ieee80211_if_setup, txqs, 1);
#else	
	ndev = alloc_netdev_mqs(sizeof(*sdata) + local->hw.vif_data_size,
				name, ieee80211_if_setup, txqs, 1);
#endif
	if (!ndev)
		return -ENOMEM;
	dev_net_set(ndev, wiphy_net(local->hw.wiphy));
#ifdef CHKSUM_HW_SUPPORT
	ndev->hw_features = (NETIF_F_RXCSUM|NETIF_F_IP_CSUM);
	ndev->features |= ndev->hw_features;
	printk("+++++++++++++++++++++++++++++++hw checksum open ++++++++++++++++++++\n");
#endif

/* This is an optimization, just ignore for older kernels */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	ndev->needed_headroom = local->tx_headroom +
				4*6 /* four MAC addresses */
				+ 2 + 2 + 2 + 2 /* ctl, dur, seq, qos */
				+ 6 /* mesh */
				+ 8 /* rfc1042/bridge tunnel */
				- ETH_HLEN /* ethernet hard_header_len */
				+ IEEE80211_ENCRYPT_HEADROOM;
	ndev->needed_tailroom = IEEE80211_ENCRYPT_TAILROOM;
#endif

	ret = dev_alloc_name(ndev, ndev->name);
	if (ret < 0)
		goto fail;

	ieee80211_assign_perm_addr(local, ndev, type);
	memcpy(ndev->dev_addr, ndev->perm_addr, ETH_ALEN);
	SET_NETDEV_DEV(ndev, wiphy_dev(local->hw.wiphy));

	/* don't use IEEE80211_DEV_TO_SUB_IF because it checks too much */
	sdata = netdev_priv(ndev);
	ndev->ieee80211_ptr = &sdata->wdev;
	memcpy(sdata->vif.addr, ndev->dev_addr, ETH_ALEN);
	memcpy(sdata->name, ndev->name, IFNAMSIZ);

	/* initialise type-independent data */
	sdata->wdev.wiphy = local->hw.wiphy;
	sdata->local = local;
	sdata->dev = ndev;
#ifdef CONFIG_INET
	sdata->arp_filter_state = true;
#ifdef IPV6_FILTERING
	sdata->ndp_filter_state = true;
#endif /*IPV6_FILTERING*/
#endif

	for (i = 0; i < IEEE80211_FRAGMENT_MAX; i++)
		atbm_skb_queue_head_init(&sdata->fragments[i].skb_list);

	INIT_LIST_HEAD(&sdata->key_list);

	for (i = 0; i < IEEE80211_NUM_BANDS; i++) {
		struct ieee80211_supported_band *sband;
		sband = local->hw.wiphy->bands[i];
		sdata->rc_rateidx_mask[i] =
			sband ? (1 << sband->n_bitrates) - 1 : 0;

		if (!sdata->chan_state.oper_channel) {
			/* init channel we're on */
			/* soumik: set default channel to non social channel */
			sdata->chan_state.conf.channel =
			/* sdata->chan_state.oper_channel = &sband->channels[0]; */
			sdata->chan_state.oper_channel = &sband->channels[2];
			sdata->chan_state.conf.channel_type = NL80211_CHAN_NO_HT;
		}
	}

	sdata->dynamic_ps_forced_timeout = -1;

	INIT_WORK(&sdata->dynamic_ps_enable_work,
		  ieee80211_dynamic_ps_enable_work);
	INIT_WORK(&sdata->dynamic_ps_disable_work,
		  ieee80211_dynamic_ps_disable_work);
	setup_timer(&sdata->dynamic_ps_timer,
		    ieee80211_dynamic_ps_timer, (unsigned long) sdata);

	sdata->vif.bss_conf.listen_interval = local->hw.max_listen_interval;

	ieee80211_set_default_queues(sdata);

	/* setup type-dependent data */
	ieee80211_setup_sdata(sdata, type);

	if (params) {
		ndev->ieee80211_ptr->use_4addr = params->use_4addr;
		if (type == NL80211_IFTYPE_STATION)
			sdata->u.mgd.use_4addr = params->use_4addr;
	}
	#ifdef CONFIG_WIRELESS_EXT
    ndev->wireless_handlers = &atbm_handlers_def;
    #endif

	ret = register_netdevice(ndev);
	if (ret)
		goto fail;

	mutex_lock(&local->iflist_mtx);
	list_add_tail_rcu(&sdata->list, &local->interfaces);
	mutex_unlock(&local->iflist_mtx);
#ifdef CONFIG_MAC80211_BRIDGE
	br0_attach(sdata);
#endif //CONFIG_MAC80211_BRIDGE
	if (new_dev)
		*new_dev = ndev;

	return 0;

 fail:
	free_netdev(ndev);
	return ret;
}

void ieee80211_if_remove(struct ieee80211_sub_if_data *sdata)
{
	ASSERT_RTNL();

	cancel_work_sync(&sdata->reconfig_filter);
	cancel_work_sync(&sdata->work);
	
#ifdef CONFIG_MAC80211_BRIDGE
	br0_detach(sdata);
#endif //CONFIG_MAC80211_BRIDGE

	mutex_lock(&sdata->local->iflist_mtx);
	list_del_rcu(&sdata->list);
	mutex_unlock(&sdata->local->iflist_mtx);
	ieee80211_delete_sdata(sdata);

	if (ieee80211_vif_is_mesh(&sdata->vif))
		mesh_path_flush_by_iface(sdata);

	synchronize_rcu();
	unregister_netdevice(sdata->dev);
}

/*
 * Remove all interfaces, may only be called at hardware unregistration
 * time because it doesn't do RCU-safe list removals.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,65) || (ATBM_WIFI_PLATFORM == 10))
 void ieee80211_remove_interfaces(struct ieee80211_local *local)
 {
	 struct ieee80211_sub_if_data *sdata, *tmp;
	 LIST_HEAD(unreg_list);
	 LIST_HEAD(wdev_list);
 
	 ASSERT_RTNL();
 
	 /*
	  * Close all AP_VLAN interfaces first, as otherwise they
	  * might be closed while the AP interface they belong to
	  * is closed, causing unregister_netdevice_many() to crash.
	  */
	 list_for_each_entry(sdata, &local->interfaces, list)
		 if (sdata->vif.type == NL80211_IFTYPE_AP_VLAN)
			 dev_close(sdata->dev);
 
	 /*
	  * Close all AP_VLAN interfaces first, as otherwise they
	  * might be closed while the AP interface they belong to
	  * is closed, causing unregister_netdevice_many() to crash.
	  */
	 list_for_each_entry(sdata, &local->interfaces, list)
		 if (sdata->vif.type == NL80211_IFTYPE_AP_VLAN)
			 dev_close(sdata->dev);
	 mutex_lock(&local->iflist_mtx);
	 list_for_each_entry_safe(sdata, tmp, &local->interfaces, list) {
		 list_del(&sdata->list);
 
		 if (sdata->dev)
			 unregister_netdevice_queue(sdata->dev, &unreg_list);
		 else
			 list_add(&sdata->list, &wdev_list);
	 }
	 mutex_unlock(&local->iflist_mtx);
	 unregister_netdevice_many(&unreg_list);
	 list_for_each_entry_safe(sdata, tmp, &wdev_list, list) {
		 list_del(&sdata->list);
		 cfg80211_unregister_wdev(&sdata->wdev);
		 atbm_kfree(sdata);
	 }
 }

#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)&& LINUX_VERSION_CODE < KERNEL_VERSION(3,10,65))
void ieee80211_remove_interfaces(struct ieee80211_local *local)
{
	struct ieee80211_sub_if_data *sdata, *tmp;
	LIST_HEAD(unreg_list);

	ASSERT_RTNL();

	mutex_lock(&local->iflist_mtx);
	list_for_each_entry_safe(sdata, tmp, &local->interfaces, list) {
		list_del(&sdata->list);

		if (ieee80211_vif_is_mesh(&sdata->vif))
			mesh_path_flush_by_iface(sdata);

		unregister_netdevice_queue(sdata->dev, &unreg_list);
	}
	mutex_unlock(&local->iflist_mtx);
	unregister_netdevice_many(&unreg_list);
	list_del(&unreg_list);
}
#else
void ieee80211_remove_interfaces(struct ieee80211_local *local)
{
	struct ieee80211_sub_if_data *sdata, *tmp;

	ASSERT_RTNL();

	list_for_each_entry_safe(sdata, tmp, &local->interfaces, list) {
		mutex_lock(&local->iflist_mtx);
		list_del(&sdata->list);
		mutex_unlock(&local->iflist_mtx);

		unregister_netdevice(sdata->dev);
	}
}
#endif

static u32 ieee80211_idle_off(struct ieee80211_local *local,
			      const char *reason)
{
	if (!(local->hw.conf.flags & IEEE80211_CONF_IDLE))
		return 0;

#ifdef CONFIG_MAC80211_ATBM_VERBOSE_DEBUG
	wiphy_debug(local->hw.wiphy, "device no longer idle - %s\n", reason);
#endif

	local->hw.conf.flags &= ~IEEE80211_CONF_IDLE;
	return IEEE80211_CONF_CHANGE_IDLE;
}

static u32 ieee80211_idle_on(struct ieee80211_local *local)
{
	struct ieee80211_sub_if_data *sdata;
	if (local->hw.conf.flags & IEEE80211_CONF_IDLE)
		return 0;

#ifdef CONFIG_MAC80211_ATBM_VERBOSE_DEBUG
	wiphy_debug(local->hw.wiphy, "device now idle\n");
#endif
	list_for_each_entry(sdata, &local->interfaces, list)
		drv_flush(local, sdata, false);

	local->hw.conf.flags |= IEEE80211_CONF_IDLE;
	return IEEE80211_CONF_CHANGE_IDLE;
}

u32 __ieee80211_recalc_idle(struct ieee80211_local *local)
{
	struct ieee80211_sub_if_data *sdata;
	int count = 0;
	bool working = false, scanning = false, hw_roc = false;
	struct ieee80211_work *wk;
	unsigned int led_trig_start = 0, led_trig_stop = 0;
	struct ieee80211_roc_work *roc;

#ifdef CONFIG_PROVE_LOCKING
	WARN_ON(debug_locks && !lockdep_rtnl_is_held() &&
		!lockdep_is_held(&local->iflist_mtx));
#endif
	lockdep_assert_held(&local->mtx);

	list_for_each_entry(sdata, &local->interfaces, list) {
		if (!ieee80211_sdata_running(sdata)) {
			sdata->vif.bss_conf.idle = true;
			continue;
		}

		sdata->old_idle = sdata->vif.bss_conf.idle;

		/* do not count disabled managed interfaces */
		if (sdata->vif.type == NL80211_IFTYPE_STATION &&
		    !sdata->u.mgd.associated) {
			sdata->vif.bss_conf.idle = true;
			continue;
		}
		/* do not count unused IBSS interfaces */
		if (sdata->vif.type == NL80211_IFTYPE_ADHOC &&
		    !sdata->u.ibss.ssid_len) {
			sdata->vif.bss_conf.idle = true;
			continue;
		}
		/* count everything else */
		count++;
	}

	if (!local->ops->remain_on_channel) {
		list_for_each_entry(roc, &local->roc_list, list) {
			working = true;
			roc->sdata->vif.bss_conf.idle = false;
		}
	}


	list_for_each_entry(wk, &local->work_list, list) {
		working = true;
		wk->sdata->vif.bss_conf.idle = false;
	}

	if (local->scan_sdata) {
		scanning = true;
		local->scan_sdata->vif.bss_conf.idle = false;
	}

	if (local->hw_roc_channel)
		hw_roc = true;

	list_for_each_entry(sdata, &local->interfaces, list) {
		if (sdata->old_idle == sdata->vif.bss_conf.idle)
			continue;
		if (!ieee80211_sdata_running(sdata))
			continue;
		ieee80211_bss_info_change_notify(sdata, BSS_CHANGED_IDLE);
	}

	if (working || scanning || hw_roc)
		led_trig_start |= IEEE80211_TPT_LEDTRIG_FL_WORK;
	else
		led_trig_stop |= IEEE80211_TPT_LEDTRIG_FL_WORK;

	if (count)
		led_trig_start |= IEEE80211_TPT_LEDTRIG_FL_CONNECTED;
	else
		led_trig_stop |= IEEE80211_TPT_LEDTRIG_FL_CONNECTED;

	ieee80211_mod_tpt_led_trig(local, led_trig_start, led_trig_stop);

	if (hw_roc)
		return ieee80211_idle_off(local, "hw remain-on-channel");
	if (working)
		return ieee80211_idle_off(local, "working");
	if (scanning)
		return ieee80211_idle_off(local, "scanning");
	if (!count)
		return ieee80211_idle_on(local);
	else
		return ieee80211_idle_off(local, "in use");

	return 0;
}

void ieee80211_recalc_idle(struct ieee80211_local *local)
{
	u32 chg;

	mutex_lock(&local->iflist_mtx);
	chg = __ieee80211_recalc_idle(local);
	mutex_unlock(&local->iflist_mtx);
	if (chg)
		ieee80211_hw_config(local, chg);
}

static int netdev_notify(struct notifier_block *nb,
			 unsigned long state,
			 void *ndev)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
	struct net_device *dev = ndev;
#else
	struct net_device *dev = netdev_notifier_info_to_dev(ndev);
#endif
	struct ieee80211_sub_if_data *sdata;

	if (state != NETDEV_CHANGENAME)
		return 0;

	if (!dev->ieee80211_ptr || !dev->ieee80211_ptr->wiphy)
		return 0;

	if (dev->ieee80211_ptr->wiphy->privid != mac80211_wiphy_privid)
		return 0;

	sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	memcpy(sdata->name, dev->name, IFNAMSIZ);

	ieee80211_debugfs_rename_netdev(sdata);
	return 0;
}

static struct notifier_block mac80211_netdev_notifier = {
	.notifier_call = netdev_notify,
};

int ieee80211_iface_init(void)
{
	return register_netdevice_notifier(&mac80211_netdev_notifier);
}

void ieee80211_iface_exit(void)
{
	unregister_netdevice_notifier(&mac80211_netdev_notifier);
}
