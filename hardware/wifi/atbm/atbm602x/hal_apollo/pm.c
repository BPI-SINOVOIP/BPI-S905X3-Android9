/*
 * Mac80211 power management API for altobeam APOLLO drivers
 * *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 * Based on apollo code
 * Copyright (c) 2011, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/if_ether.h>
#include "apollo.h"
#include "pm.h"
#include "sta.h"
#include "bh.h"
#include "sbus.h"

#define ATBM_APOLLO_BEACON_SKIPPING_MULTIPLIER 3

struct atbm_udp_port_filter {
	struct wsm_udp_port_filter_hdr hdr;
	struct wsm_udp_port_filter dhcp;
	struct wsm_udp_port_filter upnp;
} __packed;

struct atbm_ether_type_filter {
	struct wsm_ether_type_filter_hdr hdr;
	struct wsm_ether_type_filter ip;
	struct wsm_ether_type_filter pae;
	struct wsm_ether_type_filter wapi;
} __packed;

static struct atbm_udp_port_filter atbm_udp_port_filter_on = {
	.hdr.nrFilters = 2,
	.dhcp = {
		.filterAction = WSM_FILTER_ACTION_FILTER_OUT,
		.portType = WSM_FILTER_PORT_TYPE_DST,
		.udpPort = __cpu_to_le16(67),
	},
	.upnp = {
		.filterAction = WSM_FILTER_ACTION_FILTER_OUT,
		.portType = WSM_FILTER_PORT_TYPE_DST,
		.udpPort = __cpu_to_le16(1900),
	},
	/* Please add other known ports to be filtered out here and
	 * update nrFilters field in the header.
	 * Up to 4 filters are allowed. */
};

static struct wsm_udp_port_filter_hdr atbm_udp_port_filter_off = {
	.nrFilters = 0,
};

#ifndef ETH_P_WAPI
#define ETH_P_WAPI     0x88B4
#endif

static struct atbm_ether_type_filter atbm_ether_type_filter_on = {
	.hdr.nrFilters = 2,
/*	.ip = {
		.filterAction = WSM_FILTER_ACTION_FILTER_IN,
		.etherType = __cpu_to_le16(ETH_P_IP),
	},*/
	.pae = {
		.filterAction = WSM_FILTER_ACTION_FILTER_IN,
		.etherType = __cpu_to_le16(ETH_P_PAE),
	},
	.wapi = {
		.filterAction = WSM_FILTER_ACTION_FILTER_IN,
		.etherType = __cpu_to_le16(ETH_P_WAPI),
	},
	/* Please add other known ether types to be filtered out here and
	 * update nrFilters field in the header.
	 * Up to 4 filters are allowed. */
};

static struct wsm_ether_type_filter_hdr atbm_ether_type_filter_off = {
	.nrFilters = 0,
};

static int atbm_suspend_late(struct device *dev);
static void atbm_pm_release(struct device *dev);
static int atbm_pm_probe(struct platform_device *pdev);
static int __atbm_wow_suspend(struct atbm_vif *priv,
				struct cfg80211_wowlan *wowlan);
static int __atbm_wow_resume(struct atbm_vif *priv);


/* private */
struct atbm_suspend_state {
	unsigned long bss_loss_tmo;
	unsigned long connection_loss_tmo;
	unsigned long join_tmo;
	unsigned long direct_probe;
	unsigned long link_id_gc;
	bool beacon_skipping;
};

static const struct dev_pm_ops atbm_pm_ops = {
	.suspend_noirq = atbm_suspend_late,
};

static struct platform_driver atbm_power_driver = {
	.probe = atbm_pm_probe,
	.driver = {
		.name = "atbm_power",
		.pm = &atbm_pm_ops,
	},
};

static int atbm_pm_init_common(struct atbm_pm_state *pm,
				  struct atbm_common *hw_priv)
{
	int ret;

	spin_lock_init(&pm->lock);
	ret = platform_driver_register(&atbm_power_driver);
	if (ret)
		return ret;
	pm->pm_dev = platform_device_alloc("atbm_power", 0);
	if (!pm->pm_dev) {
		platform_driver_unregister(&atbm_power_driver);
		return -ENOMEM;
	}

	pm->pm_dev->dev.platform_data = hw_priv;
	ret = platform_device_add(pm->pm_dev);
	if (ret) {
		atbm_kfree(pm->pm_dev);
		pm->pm_dev = NULL;
	}

	return ret;
}
static int atbm_pm_init_stayawake_lock(struct atbm_pm_stayawake_lock *stayawake_lock)
{
	unsigned long flags;
	spin_lock_init(&stayawake_lock->stayawak_spinlock);
	#ifdef CONFIG_PM
	#ifdef CONFIG_WAKELOCK
	wake_lock_init(&stayawake_lock->stayawak_lock,WAKE_LOCK_SUSPEND, "atbm_wlan_stayawake");
	#endif
	#endif
	spin_lock_irqsave(&stayawake_lock->stayawak_spinlock,flags);
	stayawake_lock->stayawak_cnt = 0;
	spin_unlock_irqrestore(&stayawake_lock->stayawak_spinlock,flags);
	
	return 0;
}
static int atbm_pm_deinit_stayawake_lock(struct atbm_pm_stayawake_lock *stayawake_lock)
{
	#ifdef CONFIG_PM
	#ifdef CONFIG_WAKELOCK
	unsigned long flags;
	spin_lock_irqsave(&stayawake_lock->stayawak_spinlock,flags);
	if (wake_lock_active(&stayawake_lock->stayawak_lock))
		wake_unlock(&stayawake_lock->stayawak_lock);
	stayawake_lock->stayawak_cnt = 0;
	spin_unlock_irqrestore(&stayawake_lock->stayawak_spinlock,flags);
	wake_lock_destroy(&stayawake_lock->stayawak_lock);
	#endif
	#endif
	
	return 0;
}
static void atbm_pm_deinit_common(struct atbm_pm_state *pm)
{
	platform_driver_unregister(&atbm_power_driver);
	if (pm->pm_dev) {
		pm->pm_dev->dev.platform_data = NULL;
		platform_device_unregister(pm->pm_dev);
		pm->pm_dev = NULL;
	}
}
#ifdef CONFIG_PM
#ifdef CONFIG_WAKELOCK

int atbm_pm_init(struct atbm_pm_state *pm,
		   struct atbm_common *hw_priv)
{
	int ret = atbm_pm_init_common(pm, hw_priv);
	if (!ret)
		wake_lock_init(&pm->wakelock,
			WAKE_LOCK_SUSPEND, "atbm_wlan");
	atbm_pm_init_stayawake_lock(&pm->stayawake_lock);
	smp_mb();
	pm->b_init = 1;
	return ret;
}

void atbm_pm_deinit(struct atbm_pm_state *pm)
{
	if(pm->b_init==0)
		return ;
	pm->b_init =0;
	smp_mb();
	if (wake_lock_active(&pm->wakelock))
		wake_unlock(&pm->wakelock);
	wake_lock_destroy(&pm->wakelock);
	atbm_pm_deinit_common(pm);
	atbm_pm_deinit_stayawake_lock(&pm->stayawake_lock);
}

void atbm_pm_stay_awake(struct atbm_pm_state *pm,
			  unsigned long tmo)
{
	long cur_tmo;
	
	if(pm->b_init==0)
		return ;
	spin_lock_bh(&pm->lock);
#if (LINUX_VERSION_CODE >=KERNEL_VERSION(3,4,0))
	cur_tmo = pm->wakelock.ws.timer_expires - jiffies;
#else
 	cur_tmo = pm->wakelock.expires - jiffies;
#endif
	if (!wake_lock_active(&pm->wakelock) ||
			cur_tmo < (long)tmo)
		wake_lock_timeout(&pm->wakelock, tmo);
	spin_unlock_bh(&pm->lock);
}

#else /* CONFIG_WAKELOCK */

static void atbm_pm_stay_awake_tmo(unsigned long arg)
{
}

int atbm_pm_init(struct atbm_pm_state *pm,
		   struct atbm_common *hw_priv)
{
	int ret = atbm_pm_init_common(pm, hw_priv);
	if (!ret) {
		init_timer(&pm->stay_awake);
		pm->stay_awake.data = (unsigned long)pm;
		pm->stay_awake.function = atbm_pm_stay_awake_tmo;
		atbm_pm_init_stayawake_lock(&pm->stayawake_lock);
	}
	return ret;
}

void atbm_pm_deinit(struct atbm_pm_state *pm)
{
	del_timer_sync(&pm->stay_awake);
	atbm_pm_deinit_common(pm);
	atbm_pm_deinit_stayawake_lock(&pm->stayawake_lock);
}

void atbm_pm_stay_awake(struct atbm_pm_state *pm,
			  unsigned long tmo)
{
	long cur_tmo;
	spin_lock_bh(&pm->lock);
	cur_tmo = pm->stay_awake.expires - jiffies;
	if (!timer_pending(&pm->stay_awake) ||
			cur_tmo < (long)tmo)
		mod_timer(&pm->stay_awake, jiffies + tmo);
	spin_unlock_bh(&pm->lock);
}

#endif /* CONFIG_WAKELOCK */

void atbm_pm_stay_awake_lock(struct atbm_pm_state *pm)
{
	#ifdef CONFIG_WAKELOCK
	unsigned long flags;
	struct atbm_pm_stayawake_lock *stayawake_lock = &pm->stayawake_lock;
	if(pm->b_init==0)
		return ;
	spin_lock_irqsave(&stayawake_lock->stayawak_spinlock,flags);
	if(stayawake_lock->stayawak_cnt++ == 0){
		wake_lock(&stayawake_lock->stayawak_lock);
	}
	spin_unlock_irqrestore(&stayawake_lock->stayawak_spinlock,flags);
	#else
	pm = pm;
	#endif
}

void atbm_pm_stay_awake_unlock(struct atbm_pm_state *pm)
{
	#ifdef CONFIG_WAKELOCK
	unsigned long flags;
	struct atbm_pm_stayawake_lock *stayawake_lock = &pm->stayawake_lock;
	if(pm->b_init==0)
		return ;

	spin_lock_irqsave(&stayawake_lock->stayawak_spinlock,flags);
	BUG_ON(stayawake_lock->stayawak_cnt == 0);
	if(--stayawake_lock->stayawak_cnt == 0){
		wake_unlock(&stayawake_lock->stayawak_lock);
	}
	spin_unlock_irqrestore(&stayawake_lock->stayawak_spinlock,flags);
	//bm_pm_stay_awake(pm,HZ);
	#else
	pm = pm;
	#endif
}

#endif
static long atbm_suspend_work(struct delayed_work *work)
{
	int ret = cancel_delayed_work(work);
	long tmo;
	if (ret > 0) {
		/* Timer is pending */
		tmo = work->timer.expires - jiffies;
		if (tmo < 0)
			tmo = 0;
	} else {
		tmo = -1;
	}
	return tmo;
}

static int atbm_resume_work(struct atbm_common *hw_priv,
			       struct delayed_work *work,
			       unsigned long tmo)
{
	if ((long)tmo < 0)
		return 1;

	return atbm_hw_priv_queue_delayed_work(hw_priv, work, tmo);
}

static int atbm_suspend_late(struct device *dev)
{
	struct atbm_common *hw_priv = dev->platform_data;
	if (atomic_read(&hw_priv->bh_rx)) {
		wiphy_dbg(hw_priv->hw->wiphy,
			"%s: Suspend interrupted.\n",
			__func__);
		return -EAGAIN;
	}
	return 0;
}

static void atbm_pm_release(struct device *dev)
{
}

static int atbm_pm_probe(struct platform_device *pdev)
{
	pdev->dev.release = atbm_pm_release;
	return 0;
}

int _atbm_wow_suspend(struct ieee80211_hw *hw, struct cfg80211_wowlan *wowlan)
{
	struct atbm_common *hw_priv = hw->priv;
	struct atbm_vif *priv;
	int i, ret = 0;

	WARN_ON(!atomic_read(&hw_priv->num_vifs));

#ifdef ROAM_OFFLOAD
	atbm_for_each_vif(hw_priv, priv, i) {
#ifdef P2P_MULTIVIF
		if ((i == (ATBM_WIFI_MAX_VIFS - 1)) || !priv)
#else
		if (!priv)
#endif
			continue;
		if((priv->vif->type == NL80211_IFTYPE_STATION)
		&& (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA)) {
			down(&hw_priv->scan.lock);
			hw_priv->scan.if_id = priv->if_id;
			atbm_sched_scan_work(&hw_priv->scan.swork);
		}
	}
#endif /*ROAM_OFFLOAD*/

	/* Do not suspend when datapath is not idle */
	if (hw_priv->tx_queue_stats.num_queued[0]
			+ hw_priv->tx_queue_stats.num_queued[1])
		return -EBUSY;

	/* Make sure there is no configuration requests in progress. */
	if (!mutex_trylock(&hw_priv->conf_mutex))
		return -EBUSY;


	/* Do not suspend when scanning or ROC*/
	if (down_trylock(&hw_priv->scan.lock))
		goto revert1;

	if (delayed_work_pending(&hw_priv->scan.probe_work))
		goto revert2;

	/* Lock TX. */
	wsm_lock_tx_async(hw_priv);

	/* Wait to avoid possible race with bh code.
	 * But do not wait too long... */
	if (atbm_wait_event_timeout_stay_awake(hw_priv,hw_priv->bh_evt_wq,
			!(hw_priv->hw_bufs_used), HZ / 10,false) <= 0)
		goto revert3;

	atbm_for_each_vif(hw_priv, priv, i) {
#ifdef P2P_MULTIVIF
		if ((i == (ATBM_WIFI_MAX_VIFS - 1)) || !priv)
#else
		if (!priv)
#endif
			continue;

		ret = __atbm_wow_suspend(priv,
						wowlan);
		if (ret) {
			for (; i >= 0; i--) {
				if (!hw_priv->vif_list[i])
					continue;
				priv = (struct atbm_vif *)
					hw_priv->vif_list[i]->drv_priv;
				__atbm_wow_resume(priv);
			}
			goto revert3;
		}
	}

	/* Stop serving thread */
	if (atbm_bh_suspend(hw_priv)) {
		printk(KERN_DEBUG "%s: atbm_bh_suspend failed\n",
				__func__);
		atbm_wow_resume(hw);
		return -EBUSY;
	}

	/* Enable IRQ wake */
	ret = hw_priv->sbus_ops->power_mgmt(hw_priv->sbus_priv, true);
	if (ret) {
		printk(KERN_DEBUG "%s: PM request failed: %d. WoW is disabled.\n",
			__func__, ret);
		atbm_wow_resume(hw);
		return -EBUSY;
	}

	/* Force resume if event is coming from the device. */
	if (atomic_read(&hw_priv->bh_rx)) {
		printk(KERN_DEBUG "%s: incoming event present - resume\n",
				__func__);
		atbm_wow_resume(hw);
		return -EAGAIN;
	}
	return 0;
revert3:
	wsm_unlock_tx(hw_priv);
revert2:
	up(&hw_priv->scan.lock);
revert1:
	mutex_unlock(&hw_priv->conf_mutex);
	return -EBUSY;
}
int atbm_wow_suspend(struct ieee80211_hw *hw, struct cfg80211_wowlan *wowlan)
{
	struct atbm_common *hw_priv = hw->priv;
	struct atbm_vif *priv;
	int i, ret = 0;
	if(wowlan){
		printk(KERN_ERR "%s:wow_suspend\n",__func__);
		return _atbm_wow_suspend(hw,wowlan);
	}
	/*
	*wait scan or other work complete
	*/
	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);
	atbm_for_each_vif(hw_priv, priv, i) {
#ifdef P2P_MULTIVIF
		if ((i == (ATBM_WIFI_MAX_VIFS - 1)) || !priv)
#else
		if (!priv)
#endif
			continue;
		if((priv->vif->type == NL80211_IFTYPE_STATION)
		&& (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA)) {
			wsm_lock_tx(hw_priv);
			printk(KERN_ERR "[%s] line:%d unjoin_work\n", __func__, __LINE__);
			mutex_unlock(&hw_priv->conf_mutex);
			up(&hw_priv->scan.lock);
			atbm_unjoin_work(&priv->unjoin_work);
			down(&hw_priv->scan.lock);
			mutex_lock(&hw_priv->conf_mutex);
		}else if(priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP){
			printk(KERN_ERR "[%s] line:%d should stop ap\n", __func__, __LINE__);
		}
	}
	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
	/*
	*flush all work before susupend
	*/
	flush_workqueue(hw_priv->workqueue);
	return ret;
}
static int __atbm_wow_suspend(struct atbm_vif *priv,
				struct cfg80211_wowlan *wowlan)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct atbm_pm_state_vif *pm_state_vif = &priv->pm_state_vif;
	struct atbm_suspend_state *state;
	int ret;

#ifdef MCAST_FWDING
        struct wsm_forwarding_offload fwdoffload = {
                .fwenable = 0x1,
                .flags = 0x1,
        };
#endif
	/* Do not suspend when join work is scheduled */
	if (work_pending(&priv->join_work))
		goto revert1;

	/* Set UDP filter */
	wsm_set_udp_port_filter(hw_priv, &atbm_udp_port_filter_on.hdr,
				priv->if_id);

	/* Set ethernet frame type filter */
	wsm_set_ether_type_filter(hw_priv, &atbm_ether_type_filter_on.hdr,
				  priv->if_id);

        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                WARN_ON(wsm_set_keepalive_filter(priv, true));

#ifdef ATBM_APOLLO_SUSPEND_RESUME_FILTER_ENABLE
       /* Set Multicast Address Filter */
       if (priv->multicast_filter.numOfAddresses) {
               priv->multicast_filter.enable = 1;
               wsm_set_multicast_filter(hw_priv, &priv->multicast_filter, priv->if_id);
       }

       /* Set Enable Broadcast Address Filter */

       priv->broadcast_filter.action_mode = 1;
       if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                priv->broadcast_filter.address_mode = 3;

       atbm_set_macaddrfilter(hw_priv, priv, (u8 *)&priv->broadcast_filter);

#endif

#ifdef MCAST_FWDING
        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                WARN_ON(wsm_set_forwarding_offlad(hw_priv,
				&fwdoffload,priv->if_id));
#endif

	/* Allocate state */
	state = atbm_kzalloc(sizeof(struct atbm_suspend_state), GFP_KERNEL);
	if (!state)
		goto revert2;

	/* Store delayed work states. */
	state->bss_loss_tmo =
		atbm_suspend_work(&priv->bss_loss_work);
	state->connection_loss_tmo =
		atbm_suspend_work(&priv->connection_loss_work);
	state->join_tmo =
		atbm_suspend_work(&priv->join_timeout);
	state->link_id_gc =
		atbm_suspend_work(&priv->link_id_gc_work);

	/* Enable beacon skipping */
	if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA
			&& priv->join_dtim_period
			&& !priv->has_multicast_subscription) {
		state->beacon_skipping = true;
		wsm_set_beacon_wakeup_period(hw_priv,
					     priv->join_dtim_period,
					     ATBM_APOLLO_BEACON_SKIPPING_MULTIPLIER
					     * priv->join_dtim_period,
					     priv->if_id);
	}

	ret = timer_pending(&priv->mcast_timeout);
	if (ret)
		goto revert3;

	/* Store suspend state */
	pm_state_vif->suspend_state = state;

	return 0;

revert3:
	atbm_resume_work(hw_priv, &priv->bss_loss_work,
			state->bss_loss_tmo);
	atbm_resume_work(hw_priv, &priv->connection_loss_work,
			state->connection_loss_tmo);
	atbm_resume_work(hw_priv, &priv->join_timeout,
			state->join_tmo);
	atbm_resume_work(hw_priv, &priv->link_id_gc_work,
			state->link_id_gc);
	atbm_kfree(state);
revert2:
	wsm_set_udp_port_filter(hw_priv, &atbm_udp_port_filter_off,
				priv->if_id);
	wsm_set_ether_type_filter(hw_priv, &atbm_ether_type_filter_off,
				  priv->if_id);

        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                WARN_ON(wsm_set_keepalive_filter(priv, false));

#ifdef ATBM_APOLLO_SUSPEND_RESUME_FILTER_ENABLE
       /* Set Multicast Address Filter */
       if (priv->multicast_filter.numOfAddresses) {
               priv->multicast_filter.enable = 0;
               wsm_set_multicast_filter(hw_priv, &priv->multicast_filter, priv->if_id);
       }

       /* Set Enable Broadcast Address Filter */

       priv->broadcast_filter.action_mode = 0;
        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                priv->broadcast_filter.address_mode = 0;
       atbm_set_macaddrfilter(hw_priv, priv, (u8 *)&priv->broadcast_filter);

#endif

#ifdef MCAST_FWDING
	fwdoffload.flags = 0x0;	
        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                WARN_ON(wsm_set_forwarding_offlad(hw_priv, &fwdoffload,priv->if_id));
#endif
revert1:
	mutex_unlock(&hw_priv->conf_mutex);
	return -EBUSY;
}

int atbm_wow_resume(struct ieee80211_hw *hw)
{

	struct atbm_common *hw_priv = hw->priv;
	struct atbm_vif *priv;
	int i, ret = 0;

	WARN_ON(!atomic_read(&hw_priv->num_vifs));

	/* Disable IRQ wake */
	hw_priv->sbus_ops->power_mgmt(hw_priv->sbus_priv, false);

	up(&hw_priv->scan.lock);

	/* Resume BH thread */
	WARN_ON(atbm_bh_resume(hw_priv));

	atbm_for_each_vif(hw_priv, priv, i) {
#ifdef P2P_MULTIVIF
		if ((i == (ATBM_WIFI_MAX_VIFS - 1)) || !priv)
#else
		if (!priv)
#endif
			continue;
		ret = __atbm_wow_resume(priv);
		if (ret)
			break;
	}

	wsm_unlock_tx(hw_priv);
	/* Unlock configuration mutex */
	mutex_unlock(&hw_priv->conf_mutex);

	return ret;
}

static int __atbm_wow_resume(struct atbm_vif *priv)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct atbm_pm_state_vif *pm_state_vif = &priv->pm_state_vif;
	struct atbm_suspend_state *state;

	
#ifdef MCAST_FWDING
        struct wsm_forwarding_offload fwdoffload = {
                .fwenable = 0x1,
                .flags = 0x0,
        };
#endif
	state = pm_state_vif->suspend_state;
	pm_state_vif->suspend_state = NULL;

#ifdef ROAM_OFFLOAD
	if((priv->vif->type == NL80211_IFTYPE_STATION)
	&& (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA))
		atbm_hw_sched_scan_stop(hw_priv);
#endif /*ROAM_OFFLOAD*/

	if (state->beacon_skipping) {
		wsm_set_beacon_wakeup_period(hw_priv, priv->beacon_int *
				priv->join_dtim_period >
				MAX_BEACON_SKIP_TIME_MS ? 1 :
				priv->join_dtim_period, 0, priv->if_id);
		state->beacon_skipping = false;
	}

        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                WARN_ON(wsm_set_keepalive_filter(priv, false));

#ifdef ATBM_APOLLO_SUSPEND_RESUME_FILTER_ENABLE
       /* Set Multicast Address Filter */
       if (priv->multicast_filter.numOfAddresses) {
               priv->multicast_filter.enable = 0;
               wsm_set_multicast_filter(hw_priv, &priv->multicast_filter, priv->if_id);
       }

       /* Set Enable Broadcast Address Filter */

       priv->broadcast_filter.action_mode = 0;
        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                priv->broadcast_filter.address_mode = 0;

       atbm_set_macaddrfilter(hw_priv, priv, (u8 *)&priv->broadcast_filter);

#endif

#ifdef MCAST_FWDING
        if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
                WARN_ON(wsm_set_forwarding_offlad(hw_priv, &fwdoffload,priv->if_id));
#endif

	/* Resume delayed work */
	atbm_resume_work(hw_priv, &priv->bss_loss_work,
			state->bss_loss_tmo);
	atbm_resume_work(hw_priv, &priv->connection_loss_work,
			state->connection_loss_tmo);
	atbm_resume_work(hw_priv, &priv->join_timeout,
			state->join_tmo);
	atbm_resume_work(hw_priv, &priv->link_id_gc_work,
			state->link_id_gc);

	/* Remove UDP port filter */
	wsm_set_udp_port_filter(hw_priv, &atbm_udp_port_filter_off,
				priv->if_id);

	/* Remove ethernet frame type filter */
	wsm_set_ether_type_filter(hw_priv, &atbm_ether_type_filter_off,
				  priv->if_id);
	/* Free memory */
	atbm_kfree(state);

	return 0;
}
