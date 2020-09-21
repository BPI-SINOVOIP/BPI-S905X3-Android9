/*
 * Device handling thread implementation for mac80211 altobeam APOLLO drivers
 *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 * Based on:
 * Atbm UMAC CW1200 driver, which is
 * Copyright (c) 2010, ST-Ericsson
 * Author: Ajitpal Singh <ajitpal.singh@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#undef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
#include <net/atbm_mac80211.h>
#include <linux/kthread.h>

#include "apollo.h"
#include "bh.h"
#include "hwio.h"
#include "wsm.h"
#include "sbus.h"
#include "debug.h"
#include "apollo_plat.h"
#include "sta.h"
#include "ap.h"
#include "scan.h"
#if defined(CONFIG_ATBM_APOLLO_BH_DEBUG)
#define bh_printk  printk
#else
#define bh_printk(...)
#endif
#define IS_SMALL_MSG(wsm_id) (wsm_id & 0x1000)
static int atbm_bh(void *arg);
extern void atbm_monitor_pc(struct atbm_common *hw_priv);
#ifdef RESET_CHANGE
extern void tx_policy_put_reset(struct atbm_common *hw_priv);
#endif
#ifdef ATBM_SDIO_TXRX_ENHANCE

static int atbm_bh_read_ctrl_reg(struct atbm_common *hw_priv,
					  u16 *ctrl_reg);
static struct sk_buff *atbm_get_skb(struct atbm_common *hw_priv, u32 len);
enum atbm_bh_read_action {
	ATBM_BH_READ_ACT__NO_DATA,
	ATBM_BH_READ_ACT__HAS_DATA,
	ATBM_BH_READ_ACT__READ_ERR,
	ATBM_BH_READ_ACT__NONE,
};
typedef int (*reed_ctrl_handler_t)(struct atbm_common *hw_priv,u16 *ctrl_reg);
typedef int (*reed_data_handler_t)(struct atbm_common *hw_priv, void *buf, u32 buf_len);
#endif
/* TODO: Verify these numbers with WSM specification. */
#define DOWNLOAD_BLOCK_SIZE_WR	(0x2000 - 4)
/* an SPI message cannot be bigger than (2"12-1)*2 bytes
 * "*2" to cvt to bytes */
#define MAX_SZ_RD_WR_BUFFERS	(DOWNLOAD_BLOCK_SIZE_WR*2)
#define PIGGYBACK_CTRL_REG	(2)
#define EFFECTIVE_BUF_SIZE	(MAX_SZ_RD_WR_BUFFERS - PIGGYBACK_CTRL_REG)
#define SDIO_TX_MAXLEN 16384
static u8 *  AggrBuffer=NULL;

typedef int (*atbm_wsm_handler)(struct atbm_common *hw_priv,
	u8 *data, size_t size);

#ifdef MCAST_FWDING
int wsm_release_buffer_to_fw(struct atbm_vif *priv, int count);
#endif

#define ATBM_OS_WAKE_LOCK(priv)			atbm_os_wake_lock(priv)
#define ATBM_OS_WAKE_UNLOCK(priv)		atbm_os_wake_unlock(priv)
int atbm_os_wake_lock(struct atbm_common *hw_priv)
{
	unsigned long flags;
	int ret = 0;

	if (hw_priv) {
		spin_lock_irqsave(&hw_priv->wakelock_spinlock, flags);

		if (hw_priv->wakelock_hw_counter == 0) {
#ifdef CONFIG_PM
#ifdef CONFIG_HAS_WAKELOCK
			wake_lock(&hw_priv->hw_wake);
#elif defined(SDIO_BUS) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
			atbm_pm_stay_awake(&hw_priv->pm_state,HZ/2);
#endif
#endif
		}
		hw_priv->wakelock_hw_counter++;
		ret = hw_priv->wakelock_hw_counter;
		spin_unlock_irqrestore(&hw_priv->wakelock_spinlock, flags);
	}
	return ret;
}

int atbm_os_wake_unlock(struct atbm_common *hw_priv)
{
	unsigned long flags;
	int ret = 0;

	if (hw_priv) {
		spin_lock_irqsave(&hw_priv->wakelock_spinlock, flags);
		if (hw_priv->wakelock_hw_counter > 0) {
			hw_priv->wakelock_hw_counter--;
			if (hw_priv->wakelock_hw_counter == 0) {
#ifdef CONFIG_HAS_WAKELOCK
				wake_unlock(&hw_priv->hw_wake);
#elif defined(SDIO_BUS) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
				//atbm_bus_dev_pm_relax(pub);
#endif
			}
			ret = hw_priv->wakelock_hw_counter;
		}
		spin_unlock_irqrestore(&hw_priv->wakelock_spinlock, flags);
	}
	return ret;
}
void atbm_bh_wake_lock(struct atbm_common *hw_priv)
{
	unsigned long flags;

	if (hw_priv) {
		spin_lock_irqsave(&hw_priv->wakelock_spinlock, flags);

		if (hw_priv->wakelock_bh_counter == 0) {
#ifdef CONFIG_PM
#ifdef CONFIG_HAS_WAKELOCK
			wake_lock(&hw_priv->bh_wake);
#elif defined(SDIO_BUS) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
			atbm_pm_stay_awake(&hw_priv->pm_state,HZ/5);
#endif	
#endif		
			hw_priv->wakelock_bh_counter++;
		}
		spin_unlock_irqrestore(&hw_priv->wakelock_spinlock, flags);
	}
}

void atbm_bh_wake_unlock(struct atbm_common *hw_priv)
{

	unsigned long flags;

	if (hw_priv) {
		spin_lock_irqsave(&hw_priv->wakelock_spinlock, flags);
		if (hw_priv->wakelock_bh_counter > 0) {
			hw_priv->wakelock_bh_counter = 0;
#ifdef CONFIG_HAS_WAKELOCK
				wake_unlock(&hw_priv->bh_wake);
#elif defined(SDIO_BUS) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
			//atbm_bus_dev_pm_relax(pub);
#endif
		}
		spin_unlock_irqrestore(&hw_priv->wakelock_spinlock, flags);
	}

}

int atbm_register_bh(struct atbm_common *hw_priv)
{
	int err = 0;
	struct sched_param param = { .sched_priority = 1 };
	printk(KERN_DEBUG "[BH] register.\n");
	BUG_ON(hw_priv->bh_thread);
	atomic_set(&hw_priv->bh_rx, 0);
	atomic_set(&hw_priv->bh_tx, 0);
	atomic_set(&hw_priv->bh_term, 0);
	atomic_set(&hw_priv->bh_suspend, ATBM_APOLLO_BH_RESUMED);
	hw_priv->buf_id_tx = 0;
	hw_priv->buf_id_rx = 0;
	hw_priv->syncChanl_done=1;
	init_waitqueue_head(&hw_priv->bh_wq);
	init_waitqueue_head(&hw_priv->bh_evt_wq);
	hw_priv->bh_thread = kthread_create(&atbm_bh, hw_priv, "atbm_bh");
	if (IS_ERR(hw_priv->bh_thread)) {
		err = PTR_ERR(hw_priv->bh_thread);
		hw_priv->bh_thread = NULL;
	} else {
		WARN_ON(sched_setscheduler(hw_priv->bh_thread,
			SCHED_FIFO, &param));
#ifdef HAS_PUT_TASK_STRUCT
		get_task_struct(hw_priv->bh_thread);
#endif
		wake_up_process(hw_priv->bh_thread);
	}
	return err;
}
static void atbm_hw_buff_reset(struct atbm_common *hw_priv)
{
	int i;
	hw_priv->wsm_tx_seq = 0;
	hw_priv->buf_id_tx = 0;
	hw_priv->wsm_rx_seq = 0;
	hw_priv->hw_bufs_used = 0;
	hw_priv->save_buf = NULL;
	hw_priv->save_buf_len = 0;
	hw_priv->save_buf_vif_selected = -1;
	hw_priv->buf_id_rx = 0;
	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++)
		hw_priv->hw_bufs_used_vif[i] = 0;
}
int atbm_reset_lmc_cpu(struct atbm_common *hw_priv)
{
	u32 val32;
	int ret=0;
	int retry=0;
	if(hw_priv == NULL)
	{
		return -1;
	}
	while (retry <= MAX_RETRY) {
		ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
		if(!ret){
			retry=0;
			break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			mdelay(1);
			printk(KERN_ERR
				"%s:%d: enable_irq: can't read " \
				"config register.\n", __func__,__LINE__);
		}
	}
	val32 |= ATBM_HIFREG_CONFIG_CPU_RESET_BIT_2;
	val32 |= ATBM_HIFREG_CONFIG_CPU_RESET_BIT;	
	
	while (retry <= MAX_RETRY) {
		ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
		if(!ret){
		    retry=0;
			break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			mdelay(1);
			printk(KERN_ERR
				"%s:%d: enable_irq: can't write " \
				"config register.\n", __func__,__LINE__);
		}
	}
	while (retry <= MAX_RETRY) {
		ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
		if(!ret){
			retry=0;
			break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			mdelay(1);
			printk(KERN_ERR
				"%s:%d: enable_irq: can't read " \
				"config register.\n", __func__,__LINE__);
		}
	}
	val32 &= ~ATBM_HIFREG_CONFIG_CPU_RESET_BIT_2;

	while (retry <= MAX_RETRY) {
		ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
		if(!ret){
			retry=0;
			break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			mdelay(1);
			printk(KERN_ERR
				"%s:%d: enable_irq: can't write " \
				"config register.\n", __func__,__LINE__);
		}
	}

	while (retry <= MAX_RETRY) {
		ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
		if(!ret){
			retry=0;
			break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			mdelay(1);
			printk(KERN_DEBUG "%s:%d: set_mode: can't read config register.\n",__func__,__LINE__);
		}
	}
	val32 |= ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT;

	while (retry <= MAX_RETRY) {
		ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
		if(!ret){
			retry=0;
			break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			mdelay(1);
			printk(KERN_DEBUG "%s:%d: set_mode: can't write config register.\n",__func__,__LINE__);
		}
	}
	return ret;
}

#ifdef RESET_CHANGE

struct atbm_mode_mask
{
	u16 sta_mask;
	u16 ap_mask;
};
struct atbm_deauthen_addr
{
	u8 *da;
	const u8 *sa;
	const u8 *bssid;
};
bool atbm_rest_send2peer(struct atbm_vif *priv,struct sk_buff *skb)
{
	return true;
}
bool atbm_rest_send2self(struct atbm_vif *priv,struct sk_buff *skb)
{
	ieee80211_rx_irqsafe(priv->hw, skb);

	return true;
}
bool atbm_reset_send_deauthen(struct atbm_vif *priv,const struct atbm_deauthen_addr *deauthen_addr,bool toself)
{
	struct sk_buff *skb = NULL;
	struct atbm_ieee80211_mgmt *deauth = NULL;

	skb = atbm_dev_alloc_skb(sizeof(struct atbm_ieee80211_mgmt) + 64);

	if(NULL == skb||NULL == deauthen_addr)
	{
		return false;
	}

	atbm_skb_reserve(skb, 64);
	deauth = (struct atbm_ieee80211_mgmt *)atbm_skb_put(skb, sizeof(struct atbm_ieee80211_mgmt));
	WARN_ON(!deauth);
	deauth->duration = 0;
	memcpy(deauth->da, deauthen_addr->da, ETH_ALEN);
	memcpy(deauth->sa, deauthen_addr->sa, ETH_ALEN);
	memcpy(deauth->bssid, deauthen_addr->bssid, ETH_ALEN);
	deauth->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
										IEEE80211_STYPE_DEAUTH);
	deauth->u.deauth.reason_code = WLAN_REASON_DEAUTH_LEAVING;
	deauth->seq_ctrl = 0;

	return (toself ? atbm_rest_send2self(priv,skb) : atbm_rest_send2peer(priv,skb));

}
void atbm_reset_sta_send_deauthen(struct atbm_common *hw_priv,u8 mask)
{
	struct atbm_vif *priv ;
	int i=0;
	struct sk_buff_head frames;
	struct sk_buff *deauth_skb = NULL;
	__atbm_skb_queue_head_init(&frames);
	atbm_for_each_vif(hw_priv, priv, i) {
		struct sk_buff *skb;
		struct atbm_ieee80211_mgmt *deauth;
		struct ieee80211_rx_status *status;
		struct atbm_deauthen_addr deauthen_addr;
		memset(&deauthen_addr,0,sizeof(struct atbm_deauthen_addr));
		if (priv == NULL)
			continue;
		if(!(mask&(BIT(priv->if_id))))
		{
			continue;
		}
		/*
		* do unjoin work
		*/
		wsm_lock_tx(hw_priv);
		atbm_unjoin_work(&priv->unjoin_work);
		if(!priv->vif->bss_conf.assoc)
		{
			continue;
		}
		deauthen_addr.da = priv->vif->addr;
		deauthen_addr.sa = priv->vif->bss_conf.bssid;
		deauthen_addr.bssid = priv->vif->bss_conf.bssid;
		printk(KERN_DEBUG "band(%d),freq(%d),channelNum(%d)\n",hw_priv->channel->band,
													hw_priv->channel->center_freq,
													hw_priv->channel->hw_value);
		skb = atbm_dev_alloc_skb(sizeof(struct atbm_ieee80211_mgmt) + 64);
		status = IEEE80211_SKB_RXCB(skb);
		status->band = hw_priv->channel->band;
		status->freq = hw_priv->channel->center_freq;
		status->flag = 0;
		status->antenna = 0;
		atbm_skb_reserve(skb, 64);
		deauth = (struct atbm_ieee80211_mgmt *)atbm_skb_put(skb, sizeof(struct atbm_ieee80211_mgmt));
		WARN_ON(!deauth);
		deauth->duration = 0;
		memcpy(deauth->da, priv->vif->addr, ETH_ALEN);
		memcpy(deauth->sa, priv->vif->bss_conf.bssid, ETH_ALEN);
		memcpy(deauth->bssid, priv->vif->bss_conf.bssid, ETH_ALEN);
		deauth->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
											IEEE80211_STYPE_DEAUTH);
		deauth->u.deauth.reason_code = WLAN_REASON_DEAUTH_LEAVING;
		deauth->seq_ctrl = 0;
		__atbm_skb_queue_tail(&frames,skb);
	}
	spin_lock_bh(&hw_priv->send_deauthen_lock);
	while ((deauth_skb = __atbm_skb_dequeue(&frames)))
	{
		ieee80211_rx_irqsafe(hw_priv->hw, deauth_skb);
	}
	spin_unlock_bh(&hw_priv->send_deauthen_lock);
}
#define SEND_DEAUTHEN_TO_HOSTAPD_FOR_RESET			(1)
void atbm_reset_ap(struct atbm_common *hw_priv,u8 mask)
{
	struct atbm_vif *priv ;
	int i=0;
	u8 p2p_if = 0;
	struct sk_buff_head frames;
	struct sk_buff *deauth_skb;
	__atbm_skb_queue_head_init(&frames);
	atbm_for_each_vif(hw_priv, priv, i) {
		if (priv == NULL)
			continue;
		if(!(mask&(1<<priv->if_id)))
		{
			continue;
		}

		if(priv->vif->p2p!=0 )
		{
			p2p_if = 1;
			printk(KERN_DEBUG "ap interface:p2p-p2p0 go interface\n");
		}
		else
		{
			p2p_if = 0;
			printk(KERN_DEBUG "ap interface:wlan0 interface\n");
		}
		printk(KERN_DEBUG  "connected_sta_cnt(%d)\n",hw_priv->connected_sta_cnt);
		if(hw_priv->connected_sta_cnt != 0)
		{
			u8	link_index = 0;
			struct sk_buff *skb;
			struct atbm_ieee80211_mgmt *deauth;
#if !SEND_DEAUTHEN_TO_HOSTAPD_FOR_RESET
			struct ieee80211_sta * sta;
#endif
			for(link_index = 0;link_index<ATBMWIFI_MAX_STA_IN_AP_MODE;link_index++)
			{
				if(priv->link_id_db[link_index].status != ATBM_APOLLO_LINK_HARD)
				{
					continue;
				}
#if SEND_DEAUTHEN_TO_HOSTAPD_FOR_RESET
				skb = atbm_dev_alloc_skb(sizeof(struct atbm_ieee80211_mgmt) + 64);
				atbm_skb_reserve(skb, 64);
				deauth = (struct atbm_ieee80211_mgmt *)atbm_skb_put(skb, sizeof(struct atbm_ieee80211_mgmt));
				WARN_ON(!deauth);
				deauth->duration = 0;
				printk(KERN_DEBUG"%s:mac[%pM]\n",__func__,priv->link_id_db[link_index].mac);
				memcpy(deauth->da, priv->vif->addr, ETH_ALEN);
				memcpy(deauth->sa, &priv->link_id_db[link_index].mac, ETH_ALEN);
				memcpy(deauth->bssid, priv->vif->addr, ETH_ALEN);
				deauth->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
												IEEE80211_STYPE_DEAUTH);
				deauth->u.deauth.reason_code = WLAN_REASON_DEAUTH_LEAVING;
				deauth->seq_ctrl = 0;
				__atbm_skb_queue_tail(&frames,skb);
#else
				/*
				*hope that way run successfully , we must enable disassoc_low_ack function in hostapd.conf.
				*see hostapd_event_sta_low_ack function in file src/ap/drv_callbacks.c;
				*/
				sta = ieee80211_find_sta(priv->vif, priv->link_id_db[link_index].mac);
				printk("%s:mac[%pM]\n",__func__,priv->link_id_db[link_index].mac);
				ieee80211_report_low_ack(sta,50);
#endif
			}
		}
		/*
		*start ap : set beacon infor for lmac
		*/
#if SEND_DEAUTHEN_TO_HOSTAPD_FOR_RESET
		spin_lock_bh(&hw_priv->send_deauthen_lock);
		while ((deauth_skb = __atbm_skb_dequeue(&frames)))
		{
			ieee80211_rx_irqsafe(priv->hw, deauth_skb);
		}
		spin_unlock_bh(&hw_priv->send_deauthen_lock);
#endif

		WARN_ON((atomic_add_return(1, &hw_priv->ap_restart) != 1));
		priv->join_status = ATBM_APOLLO_JOIN_STATUS_PASSIVE;
		atbm_bss_info_changed(hw_priv->hw,priv->vif,&priv->vif->bss_conf,BSS_CHANGED_BEACON_ENABLED|
						BSS_CHANGED_BEACON|
						BSS_CHANGED_SSID);
		atbm_bss_info_changed(hw_priv->hw,priv->vif,&priv->vif->bss_conf,BSS_CHANGED_ARP_FILTER);
		#ifdef IPV6_FILTERING
		atbm_bss_info_changed(hw_priv->hw,priv->vif,&priv->vif->bss_conf,BSS_CHANGED_NDP_FILTER);
		#endif /*IPV6_FILTERING*/
		WARN_ON((atomic_xchg(&hw_priv->ap_restart, 0) != 1));

	}
}
int atbm_reset_chip(struct atbm_common *hw_priv)
{
	int ret;
	printk("atbm_reset_chip!!!\n");
	ret = atbm_reg_write_32(hw_priv,(u16)0x16100074,1);
	mdelay(5);
	return ret;
}
int atbm_reload_fw(struct atbm_common *hw_priv)
{
	int err = 0;
	struct atbm_vif *priv ;
	int if_id;
	int i;
	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};

	hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);

	tx_policy_put_reset(hw_priv);

	//2..load firmware
	hw_priv->init_done = 0;
	hw_priv->wsm_caps.firmwareReady = 0;
	err = atbm_load_firmware(hw_priv);
	if (err)
	{
		err = -1;
		goto re_err;
	}
	hw_priv->init_done = 1;
	wake_up(&hw_priv->bh_wq);
	if (wait_event_interruptible_timeout(hw_priv->wsm_startup_done,
			hw_priv->wsm_caps.firmwareReady,3*HZ)<=0){
			printk("wait wsm_startup_done timeout!!\n");
			goto re_err;
	}
	atbm_firmware_init_check(hw_priv);
	WARN_ON((atomic_xchg(&hw_priv->fw_reloading, 0) != 1));
	printk( "reset wsm_startup_done ++ !!\n");
	/*
	*in reset mode,the lmac can not rx pkg.
	*/
#ifdef RESET_NOTIFY_TO_LMAC
	wsm_notify_fw_reset(hw_priv,WSM_RESET_FLAGS__RESET_FW);
#endif
	for (if_id = 0; if_id < ABwifi_get_nr_hw_ifaces(hw_priv); if_id++) {
		err = wsm_set_operational_mode(hw_priv, &mode, if_id);
		if (err) {
			printk("wsm_set_operational_mode failed\n");
			goto re_err;
		}
		err = wsm_use_multi_tx_conf(hw_priv, true, if_id);
		if (err) {
			printk("wsm_use_multi_tx_conf failed\n");
			goto re_err;
		}
	}
	/*
	*set perm_addr to lmac.
	*NOTIFY that:only can set perm_addr to lmac.
	*/
	memcpy(hw_priv->mac_addr, hw_priv->hw->wiphy->perm_addr, ETH_ALEN);
	if((err = atbm_setup_mac(hw_priv))!=0)
	{
		goto re_err;
	}
	atbm_for_each_vif(hw_priv, priv, i) {
		if (priv == NULL)
			continue;
		if(priv->if_id<2)
		{
			memcpy(hw_priv->mac_addr, priv->vif->addr, ETH_ALEN);
		    atbm_vif_setup(priv);
		}

		err = WARN_ON(atbm_setup_mac_pvif(priv));
		atomic_set(&priv->enabled, 1);
	}
re_err:
	return err;

}

int atbm_mac_reset(struct atbm_common *hw_priv,struct atbm_mode_mask *mask)
{
	struct atbm_vif *priv ;
	int i;

	/*
	*reset sdio or usb buff params.
	*/
	atbm_hw_buff_reset(hw_priv);
	/* clean up vif  data for reset*/
	atbm_for_each_vif(hw_priv, priv, i) {
		if (priv == NULL)
			continue;
		atomic_set(&priv->enabled, 0);
		down(&hw_priv->scan.lock);
		mutex_lock(&hw_priv->conf_mutex);
		atbm_tx_queues_lock(hw_priv);
		wsm_lock_tx(hw_priv);
		switch (priv->join_status) {
		case ATBM_APOLLO_JOIN_STATUS_IBSS:
			printk("%s %d\n", __func__, __LINE__);
		case ATBM_APOLLO_JOIN_STATUS_STA:
			mask->sta_mask |= (1<<priv->if_id);

			break;
		case ATBM_APOLLO_JOIN_STATUS_AP:
			mask->ap_mask |= (1<<priv->if_id);
			break;
		case ATBM_APOLLO_JOIN_STATUS_MONITOR:
			atbm_disable_listening(priv);
			printk("%s %d\n", __func__, __LINE__);
			break;
		default:
			break;
		}
		/*
		*see wsm_handle_tx_data function,at some situations,pkg is
		*pended,an lock tx_lock.so when reset,we must unlock the 
		*lock to allow send pkg at latter.
		*/
		if(work_pending(&priv->wep_key_work))
		{
			printk(KERN_DEBUG "work_pending(&priv->scan.probe_work)\n");
			atbm_cancle_queue_work(&priv->wep_key_work,false);
			wsm_unlock_tx(hw_priv);
		}
		if(work_pending(&priv->join_work))
		{
			printk("%s:cancle priv->join_work\n",__func__);
			atbm_cancle_queue_work(&priv->join_work,false);
			wsm_unlock_tx(hw_priv);
		}
		/*
		*when pending unjoin_work,the wsm_lock_tx is lock,we
		*must unlock the lock to enable tx pkg in later.
		*unjoin_work will be called in sta or p2p sta reset.
		*/
		if(work_pending(&priv->unjoin_work))
		{
			printk(KERN_DEBUG "work_pending(&priv->unjoin_work)\n");
			atbm_cancle_queue_work(&priv->unjoin_work,false);
			wsm_unlock_tx(hw_priv);
		}
		
		/*
		*when pending offchannel_work,the wsm_lock_tx is lock,so
		*we must unlock the lock.and call the offchannel_work function.
		*/
		if (work_pending(&priv->offchannel_work))
		{
			printk(KERN_DEBUG "work_pending(&priv->offchannel_work)\n");
			atbm_cancle_queue_work(&priv->offchannel_work,false);
			wsm_unlock_tx(hw_priv);
			wsm_lock_tx(hw_priv);
			atbm_offchannel_work(&priv->offchannel_work);
		}
		if (work_pending(&priv->link_id_work))
		{
			printk("%s:cancle &priv->link_id_work\n",__func__);
			atbm_cancle_queue_work(&priv->link_id_work,false);
			wsm_unlock_tx(hw_priv);
		}
		/* TODO:COMBO: Change Queue Module */
		if (!__atbm_flush(hw_priv, true, priv->if_id))
		{
			printk("%s:__atbm_flush err \n",__func__);
			wsm_unlock_tx(hw_priv);
		}

//		cancel_delayed_work_sync(&priv->bss_loss_work);
//		cancel_delayed_work_sync(&priv->connection_loss_work);
//		cancel_delayed_work_sync(&priv->link_id_gc_work);
//		cancel_delayed_work_sync(&priv->join_timeout);
//		cancel_delayed_work_sync(&priv->set_cts_work);
//		cancel_delayed_work_sync(&priv->pending_offchanneltx_work);

//		del_timer_sync(&priv->mcast_timeout);
		/* TODO:COMBO: May be reset of these variables "delayed_link_loss and
		 * join_status to default can be removed as dev_priv will be freed by
		 * mac80211 */
//		priv->delayed_link_loss = 0;
		//priv->join_status = ATBM_APOLLO_JOIN_STATUS_PASSIVE;
		wsm_unlock_tx(hw_priv);

		if ((priv->if_id ==1) && (priv->mode == NL80211_IFTYPE_AP
			|| priv->mode == NL80211_IFTYPE_P2P_GO)) {
			hw_priv->is_go_thru_go_neg = false;
		}
		priv->listening = false;

		atbm_tx_queues_unlock(hw_priv);
		mutex_unlock(&hw_priv->conf_mutex);
		up(&hw_priv->scan.lock);
	}
	return 0;
}
int atbm_reset_driver(struct atbm_common *hw_priv)
{
	int err = 0;
	struct atbm_mode_mask mode_mask =
	{
		.sta_mask=0,
		.ap_mask=0,
	};
	printk(KERN_DEBUG "%s start\n", __func__);
	if (hw_priv == NULL)
		goto err2;
	/*
	*when bh_err,hw_priv->bh_error has been set,so wsm_unlock_tx
	*can not sub the hw_priv->tx_lock.
	*/
	if(atomic_add_return(0, &hw_priv->tx_lock))
	{
		printk(KERN_ERR "%s:release hw_priv->tx_lock (%d)\n",__func__,atomic_read(&hw_priv->tx_lock));
	}
	/*
	*enable pending cmd
	*/
	{
		/*
		*if we can not get wsm_cmd_mux lock,it indicate that a cmd has
		*been send to lmac successfully,but can not receive response from
		*lmc.In that case ,we must send cmd response to ourself(hmac).let
		*the cmd process unlock the wsm_cmd.lock to make sure that hmac
		*can send cmd to lmc in later.
		*/
		if(!mutex_trylock(&hw_priv->wsm_cmd_mux))
		{
			spin_lock_bh(&hw_priv->wsm_cmd.lock);
			if(hw_priv->wsm_cmd.cmd != 0xFFFF)
			{
				hw_priv->wsm_cmd.ret = -1;
				hw_priv->wsm_cmd.done = 1;
				spin_unlock_bh(&hw_priv->wsm_cmd.lock);
				printk(KERN_DEBUG "cancle current pendding cmd,release wsm_cmd.lock\n");
				wake_up(&hw_priv->wsm_cmd_wq);
				msleep(2);
				spin_lock_bh(&hw_priv->wsm_cmd.lock);
			}
			spin_unlock_bh(&hw_priv->wsm_cmd.lock);
		}
		else
		{
			mutex_unlock(&hw_priv->wsm_cmd_mux);
		}
	}
	if(work_pending(&hw_priv->tx_policy_upload_work))
	{
		printk("cancel hw_priv->tx_policy_upload_work\n");
		atbm_cancle_queue_work(&hw_priv->tx_policy_upload_work,true);
		atbm_tx_queues_unlock(hw_priv);
		wsm_unlock_tx(hw_priv);
	}
	if(atbm_cancle_delayed_work(&hw_priv->scan.probe_work,true))
	{
		printk(KERN_DEBUG "work_pending(&hw_priv->scan.probe_work)\n");
		wsm_unlock_tx(hw_priv);
	}
	/*
	*cancle the current scanning process
	*/
	mutex_lock(&hw_priv->conf_mutex);
	if(atomic_read(&hw_priv->scan.in_progress))
	{
		struct atbm_vif *scan_priv = ABwifi_hwpriv_to_vifpriv(hw_priv,
					hw_priv->scan.if_id);
		bool scanto_running = false;
		atbm_priv_vif_list_read_unlock(&scan_priv->vif_lock);
		mutex_unlock(&hw_priv->conf_mutex);
		scanto_running = atbm_cancle_delayed_work(&hw_priv->scan.timeout,true);
		mutex_lock(&hw_priv->conf_mutex);
		if(scanto_running>0)
		{
			hw_priv->scan.curr = hw_priv->scan.end;

			mutex_unlock(&hw_priv->conf_mutex);
			atbm_hw_priv_queue_delayed_work(hw_priv,
			&hw_priv->scan.timeout, 0);
			msleep(2);
			mutex_lock(&hw_priv->conf_mutex);
		}
	}
	mutex_unlock(&hw_priv->conf_mutex);
	/*
	*cancle remain on channel process,but without testing...........
	*/
	{
		mutex_lock(&hw_priv->conf_mutex);
		if((atomic_read(&hw_priv->remain_on_channel))||(hw_priv->roc_if_id != -1))
		{
			mutex_unlock(&hw_priv->conf_mutex);
			printk(KERN_DEBUG "cancle remain on channel\n");
			atbm_cancle_delayed_work(&hw_priv->rem_chan_timeout,true);
			atbm_rem_chan_timeout(&hw_priv->rem_chan_timeout.work);
		}
		else
		{
			mutex_unlock(&hw_priv->conf_mutex);
		}
	}
	/*
	*when hmac enters in joining ,scaning ,seting power saving state,the 
	*wsm_oper_lock is still locked untill receiving completation
	*from lmac.so when hmac asserts at the station above, we must ensure
	*that wsm_oper_lock is unlocked.
	*/
	#if 0
	#ifndef OPER_CLOCK_USE_SEM
	if(!mutex_trylock(&hw_priv->wsm_oper_lock)){
		mutex_unlock(&hw_priv->wsm_oper_lock);
	}
	else {		
		mutex_unlock(&hw_priv->wsm_oper_lock);
	}
	#else
	if(down_trylock(&hw_priv->wsm_oper_lock)){
		up(&hw_priv->wsm_oper_lock);		
	}
	else {
		up(&hw_priv->wsm_oper_lock);
	}
	#endif
	#endif
	/*
	*flush all work in workqueue
	*/
	flush_workqueue(hw_priv->workqueue);
	/*
	* reset cpu for reloading fw
	*
	*/
	if (atbm_reset_lmc_cpu(hw_priv)<0)
	{
		goto err2;
	}
	/*
	*reset hmac and mask the sta and ap interfaces
	*/
	if(atbm_mac_reset(hw_priv,&mode_mask))
	{
		goto err2;
	}
	/*
	*reload fw
	*/
	if((err = atbm_reload_fw(hw_priv)) != 0)
	{
		printk("%s:reload fw err\n",__func__);
		/*
		*reload fw err,we can reload again
		*/
		if(err == -1)
			goto err2;
		/*
		*wait setup indication err,send err to bh
		*and then reload
		*/
		else if (err == -2)
			goto err3;
		else
			goto out;
	}
	
	if(mode_mask.sta_mask)
		atbm_reset_sta_send_deauthen(hw_priv,mode_mask.sta_mask);
	if(mode_mask.ap_mask)
		atbm_reset_ap(hw_priv,mode_mask.ap_mask);
	atomic_xchg(&hw_priv->reset_conter, 0);
	/*
	*reset ok. send msg to lmac.
	*/
#ifdef RESET_NOTIFY_TO_LMAC
	wsm_notify_fw_reset(hw_priv,0);
#else
	WARN_ON((atomic_xchg(&hw_priv->reset_flag, 0) != 1));	
#endif
out:
	return 0;
err3:
	printk(KERN_DEBUG "err3 %s %d\n", __func__, __LINE__);
	hw_priv->sbus_ops->reset(hw_priv->sbus_priv);
	hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);
	hw_priv->bh_error=1;
	wake_up(&hw_priv->bh_wq);
	err = -2;
	return err;
err2:
	printk(KERN_DEBUG "err2 %s %d\n", __func__, __LINE__);
	if(atomic_read(&hw_priv->reset_flag)==1)
	{
		//bh has been process hmac reset,so return wihout wake bh
		err = -1;
		atomic_add_return(1,&hw_priv->reset_flag);
	}
	else
	{
		hw_priv->bh_error=1;
		wake_up(&hw_priv->bh_wq);
		err = -2;
	}
	return err;
}
#endif
void atbm_unregister_bh(struct atbm_common *hw_priv)
{
	struct task_struct *thread = hw_priv->bh_thread;
	if (WARN_ON(!thread))
		return;

	hw_priv->bh_thread = NULL;
	bh_printk(KERN_DEBUG "[BH] unregister.\n");
	atomic_add(1, &hw_priv->bh_term);
	wake_up(&hw_priv->bh_wq);
	kthread_stop(thread);
#ifdef HAS_PUT_TASK_STRUCT
	put_task_struct(thread);
#endif
}


#ifdef ATBM_SDIO_TXRX_ENHANCE
int atbm_sdio_process_read_data(struct atbm_common *hw_priv,reed_ctrl_handler_t read_ctrl_func,
	reed_data_handler_t read_data_func)
{
	#define LMAC_MAX_RX_BUFF	(24)
	u32 read_len_lsb = 0;
	u32 read_len_msb = 0;
	u32 read_len;
	struct sk_buff *skb_rx = NULL;
	u16 ctrl_reg = 0;
	u32 alloc_len;
	u8 *data;
	u8 rx_continue_cnt = 0;
	ATBM_OS_WAKE_LOCK(hw_priv);
	if (WARN_ON(read_ctrl_func(
			hw_priv, &ctrl_reg))){
			printk(KERN_ERR"%s:read_ctrl_func err\n",__func__);
			goto err;
	}
rx_continue:
	read_len_lsb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK)*2;
	read_len_msb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK)*2;
	read_len=((read_len_msb>>2)+read_len_lsb);
	if (!read_len) {
		ATBM_OS_WAKE_UNLOCK(hw_priv);
		return 0;
	}

	if (WARN_ON((read_len < sizeof(struct wsm_hdr)) ||
			(read_len > EFFECTIVE_BUF_SIZE))) {
			printk(KERN_ERR "Invalid read len: %d,read_cnt(%d)\n",
				read_len,rx_continue_cnt);
		goto err;
	}
	/* Add SIZE of PIGGYBACK reg (CONTROL Reg)
	 * to the NEXT Message length + 2 Bytes for SKB */
	read_len = read_len + 2;
	alloc_len = read_len;
	if (alloc_len % SDIO_BLOCK_SIZE ) {
		alloc_len -= (alloc_len % SDIO_BLOCK_SIZE );
		alloc_len += SDIO_BLOCK_SIZE;
	}
	/* Check if not exceeding CW1200 capabilities */
	if (WARN_ON_ONCE(alloc_len > EFFECTIVE_BUF_SIZE)) {
		printk(KERN_DEBUG "Read aligned len: %d\n",
			alloc_len);
	}
	skb_rx = atbm_get_skb(hw_priv, alloc_len);
	if (WARN_ON(!skb_rx)){
		printk(KERN_ERR "%s:can not get skb,rx_continue_cnt(%d)\n",__func__,rx_continue_cnt);
		goto err;
	}

	atbm_skb_trim(skb_rx, 0);
	atbm_skb_put(skb_rx, read_len);
	data = skb_rx->data;
	if (WARN_ON(!data)){
		goto err;
	}
	if (WARN_ON(read_data_func(hw_priv, data, alloc_len))){
		printk(KERN_ERR "%s:read_data_func err,rx_continue_cnt(%d)\n",__func__,rx_continue_cnt);
		goto err;
	}
	/* Piggyback */
	ctrl_reg = __le16_to_cpu(
		((__le16 *)data)[alloc_len / 2 - 1]);
	
	if(atbm_bh_is_term(hw_priv) || atomic_read(&hw_priv->bh_term)){
		goto err;
	}
	else {
		atbm_skb_queue_tail(&hw_priv->rx_frame_queue, skb_rx);
	}
	read_len_lsb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK)*2;
	read_len_msb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK)*2;
	read_len=((read_len_msb>>2)+read_len_lsb);
	rx_continue_cnt++;
	if (atomic_add_return(1, &hw_priv->bh_rx) == 1){
		wake_up(&hw_priv->bh_wq);
	}
	if(read_len)
		goto rx_continue;
	ATBM_OS_WAKE_UNLOCK(hw_priv);
	return 0;
err:
	ATBM_OS_WAKE_UNLOCK(hw_priv);
	if(skb_rx)
		atbm_dev_kfree_skb(skb_rx);
	return -1;
}

enum atbm_bh_read_action atbm_bh_read_miss_data(struct atbm_common *hw_priv)
{
	enum atbm_bh_read_action action = ATBM_BH_READ_ACT__NO_DATA;
	if(!down_trylock(&hw_priv->sdio_rx_process_lock)){
		int ret = 0;
		ret = atbm_sdio_process_read_data(hw_priv,atbm_bh_read_ctrl_reg,atbm_data_read);
		if(ret == 0)
			action = atomic_xchg(&hw_priv->bh_rx, 0) ? ATBM_BH_READ_ACT__HAS_DATA : ATBM_BH_READ_ACT__NO_DATA;
		else
			action = ATBM_BH_READ_ACT__READ_ERR;
		up(&hw_priv->sdio_rx_process_lock);
	}else {
		action = ATBM_BH_READ_ACT__HAS_DATA;
	}
	return action;
}
int atbm_rx_bh_flush(struct atbm_common *hw_priv)
{
	struct sk_buff *skb ;

	while ((skb = atbm_skb_dequeue(&hw_priv->rx_frame_queue)) != NULL) {
		//printk("test=====>atbm_kfree skb %p \n",skb);
		ATBM_OS_WAKE_UNLOCK(hw_priv);
		atbm_dev_kfree_skb(skb);
	}
	return 0;
}
#define ATBM_BH_CHECK_MISS_DATA(_hw_priv,_rx,rx_lable,err_lable)	\
	do													\
	{													\
		switch(atbm_bh_read_miss_data(_hw_priv))		\
		{												\
			case ATBM_BH_READ_ACT__NO_DATA:				\
				break;									\
			case ATBM_BH_READ_ACT__HAS_DATA:			\
				_rx = 1;								\
				goto rx_lable;							\
			case ATBM_BH_READ_ACT__READ_ERR:			\
				_hw_priv->bh_error = 1;					\
				goto err_lable;							\
			default:									\
				break;									\
		}												\
	}while(0)											
#endif
void atbm_irq_handler(struct atbm_common *hw_priv)
{
#ifdef ATBM_SDIO_TXRX_ENHANCE
	reed_ctrl_handler_t read_ctrl_func;
	reed_data_handler_t read_data_func;
	#ifndef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
	read_ctrl_func = atbm_bh_read_ctrl_reg;
	read_data_func = atbm_data_read;
	#else
	read_ctrl_func = atbm_bh_read_ctrl_reg;
	read_data_func = atbm_data_read;
	#endif
	//printk( "[BH] irq.\n");
	/* To force the device to be always-on, the host sets WLAN_UP to 1 */
	if(!hw_priv->init_done){
		printk(KERN_DEBUG "[BH] irq. init_done =0 drop\n");
		return;
	}

	if (atbm_bh_is_term(hw_priv))
		return;

	
	down(&hw_priv->sdio_rx_process_lock);
	if (atbm_sdio_process_read_data(hw_priv,read_ctrl_func,read_data_func) != 0){
		goto rx_err;
	}
	up(&hw_priv->sdio_rx_process_lock);
	return;
rx_err:
	up(&hw_priv->sdio_rx_process_lock);
	if (atbm_bh_is_term(hw_priv) || atomic_read(&hw_priv->bh_term)){
		hw_priv->bh_error = 1;
		wake_up(&hw_priv->bh_wq);
	}
	return;
#else //ATBM_SDIO_TXRX_ENHANCE

#if ((ATBM_WIFI_PLATFORM == PLATFORM_XUNWEI)||(ATBM_WIFI_PLATFORM == PLATFORM_AMLOGIC_905))
#ifndef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
	/*Disable interrupt*/
	/*NOTE :lock already held*/
	//hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	__atbm_irq_enable(hw_priv,0);
	//hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
#endif //#ifndef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
#endif

	//printk( "[BH] irq.\n");
	/* To force the device to be always-on, the host sets WLAN_UP to 1 */
	if(!hw_priv->init_done){
		printk(KERN_DEBUG "[BH] irq. init_done =0 drop\n");
		return;
	}

	if (/* WARN_ON */(hw_priv->bh_error))
		return;


	ATBM_OS_WAKE_LOCK(hw_priv);

	if (atomic_add_return(1, &hw_priv->bh_rx) == 1)
		wake_up(&hw_priv->bh_wq);
#endif
}

int atbm_bh_suspend(struct atbm_common *hw_priv)
{
	int i =0;
	struct atbm_vif *priv = NULL;

	printk(KERN_ERR "[BH] try suspend.\n");
	if (hw_priv->bh_error) {
		wiphy_warn(hw_priv->hw->wiphy, "BH error -- can't suspend\n");
		return -EINVAL;
	}
#ifdef MCAST_FWDING

 	atbm_for_each_vif(hw_priv, priv, i) {
		if (!priv)
			continue;
		if ( (priv->multicast_filter.enable)
			&& (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP) ) {
			wsm_release_buffer_to_fw(priv,
				(hw_priv->wsm_caps.numInpChBufs - 1));
			break;
		}
	}
#endif

	atomic_set(&hw_priv->bh_suspend, ATBM_APOLLO_BH_SUSPEND);
	wake_up(&hw_priv->bh_wq);
	return atbm_wait_event_timeout_stay_awake(hw_priv,hw_priv->bh_evt_wq, hw_priv->bh_error ||
		(ATBM_APOLLO_BH_SUSPENDED == atomic_read(&hw_priv->bh_suspend)),
		 100 * HZ,false) ? 0 : -ETIMEDOUT;
}

int atbm_bh_resume(struct atbm_common *hw_priv)
{
int i =0;
#ifdef MCAST_FWDING
	int ret;
	struct atbm_vif *priv =NULL;
#endif

	printk(KERN_ERR "[BH] try resume.\n");
	if (hw_priv->bh_error) {
		wiphy_warn(hw_priv->hw->wiphy, "BH error -- can't resume\n");
		return -EINVAL;
	}

	atomic_set(&hw_priv->bh_suspend, ATBM_APOLLO_BH_RESUME);
	wake_up(&hw_priv->bh_wq);
        printk("wakeup bh,wait evt_wq\n");
#ifdef MCAST_FWDING
	ret = atbm_wait_event_timeout_stay_awake(hw_priv,hw_priv->bh_evt_wq, hw_priv->bh_error ||
				(ATBM_APOLLO_BH_RESUMED == atomic_read(&hw_priv->bh_suspend)),
				1 * HZ,false) ? 0 : -ETIMEDOUT;

	atbm_for_each_vif(hw_priv, priv, i) {
		if (!priv)
			continue;
		if ((priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
				&& (priv->multicast_filter.enable)) {
			u8 count = 0;
			WARN_ON(wsm_request_buffer_request(priv, &count));
			bh_printk(KERN_DEBUG
				"[BH] BH resume. Reclaim Buff %d \n",count);
			break;
		}
	}

	return ret;
#else
	return atbm_wait_event_timeout_stay_awake(hw_priv,hw_priv->bh_evt_wq,hw_priv->bh_error ||
		(ATBM_APOLLO_BH_RESUMED == atomic_read(&hw_priv->bh_suspend)),
		100 * HZ,false) ? 0 : -ETIMEDOUT;
#endif

}

void wsm_alloc_tx_buffer(struct atbm_common *hw_priv)
{
	spin_lock_bh(&hw_priv->tx_com_lock);
	++hw_priv->hw_bufs_used;
	spin_unlock_bh(&hw_priv->tx_com_lock);
}

int wsm_release_tx_buffer(struct atbm_common *hw_priv, int count)
{
	int ret = 0;
	int hw_bufs_used;
	spin_lock_bh(&hw_priv->tx_com_lock);
	hw_bufs_used = hw_priv->hw_bufs_used;
	hw_priv->hw_bufs_used -= count;

	if (WARN_ON(hw_priv->hw_bufs_used < 0))
		//ret = -1;
		hw_priv->hw_bufs_used=0;
	/* Tx data patch stops when all but one hw buffers are used.
	   So, re-start tx path in case we find hw_bufs_used equals
	   numInputChBufs - 1.
	 */
	else if (hw_bufs_used >= (hw_priv->wsm_caps.numInpChBufs - 1))
		ret = 1;
	spin_unlock_bh(&hw_priv->tx_com_lock);
	if (!(hw_priv->hw_bufs_used))
		wake_up(&hw_priv->bh_evt_wq);
	return ret;
}

#ifdef MCAST_FWDING
#ifndef USB_BUS

//just for sdio
int wsm_release_buffer_to_fw(struct atbm_vif *priv, int count)
{
	int i;
	u8 flags;
	struct wsm_buf *buf;
	u32 buf_len;
	struct wsm_hdr_tx *wsm;
	struct atbm_common *hw_priv = priv->hw_priv;


	if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_AP) {
		return 0;
	}

	bh_printk(KERN_DEBUG "Rel buffer to FW %d, %d\n", count, hw_priv->hw_bufs_used);

	for (i = 0; i < count; i++) {
		if ((hw_priv->hw_bufs_used + 1) < hw_priv->wsm_caps.numInpChBufs) {
			flags = i ? 0: 0x1;

			wsm_alloc_tx_buffer(hw_priv);

			buf = &hw_priv->wsm_release_buf[i];
			buf_len = buf->data - buf->begin;

			/* Add sequence number */
			wsm = (struct wsm_hdr_tx *)buf->begin;
			BUG_ON(buf_len < sizeof(*wsm));

			wsm->id &= __cpu_to_le32(
				~WSM_TX_SEQ(WSM_TX_SEQ_MAX));
			wsm->id |= cpu_to_le32(
				WSM_TX_SEQ(hw_priv->wsm_tx_seq));

			printk("REL %d\n", hw_priv->wsm_tx_seq);

			if (WARN_ON(atbm_data_write(hw_priv,
				buf->begin, buf_len))) {
				break;
			}


			hw_priv->buf_released = 1;
			hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq + 1) & WSM_TX_SEQ_MAX;
		} else
			break;
	}

	if (i == count) {
		return 0;
	}

	/* Should not be here */
	printk(KERN_ERR"[BH] Less HW buf %d,%d.\n", hw_priv->hw_bufs_used,
			hw_priv->wsm_caps.numInpChBufs);
	WARN_ON(1);

	return -1;
}
#endif //USB_BUS
#endif
static struct sk_buff *atbm_get_skb(struct atbm_common *hw_priv, u32 len)
{
	struct sk_buff *skb;
	u32 alloc_len = (len > SDIO_BLOCK_SIZE) ? len : SDIO_BLOCK_SIZE;

	if (len > SDIO_BLOCK_SIZE || !hw_priv->skb_cache) {
		skb = __atbm_dev_alloc_skb(alloc_len
				+ WSM_TX_EXTRA_HEADROOM
				+ 8  /* TKIP IV */
				+ 12 /* TKIP ICV + MIC */
				- 2  /* Piggyback */,GFP_KERNEL);
		BUG_ON(skb==NULL);
		/* In AP mode RXed SKB can be looped back as a broadcast.
		 * Here we reserve enough space for headers. */
		atbm_skb_reserve(skb, WSM_TX_EXTRA_HEADROOM
				+ 8 /* TKIP IV */
				- WSM_RX_EXTRA_HEADROOM);
	} else {
		skb = hw_priv->skb_cache;
		hw_priv->skb_cache = NULL;
	}
	return skb;
}


void atbm_put_skb(struct atbm_common *hw_priv, struct sk_buff *skb)
{
	if (hw_priv->skb_cache){
		//printk("%s atbm_kfree_skb skb=%p\n",__func__,skb);
		atbm_dev_kfree_skb(skb);
	}
	else
		hw_priv->skb_cache = skb;
}

static int atbm_bh_read_ctrl_reg(struct atbm_common *hw_priv,
					  u16 *ctrl_reg)
{
	int ret=0,retry=0;
	while (retry <= MAX_RETRY) {
		ret = atbm_reg_read_16(hw_priv,
				ATBM_HIFREG_CONTROL_REG_ID, ctrl_reg);
		if(!ret){
				break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			mdelay(retry);
			printk(KERN_ERR
				"[BH] Failed to read control register.ret=%x\n",ret);
		}
	}
	return ret;
}
#if (PROJ_TYPE>=ARES_B)
//just ARESB have this function
//used this function to clear sdio rtl bug register
// if not do this sdio direct mode (wr/read reigster) will not work
// this function is the same to atbm_data_force_write (used queue mode clear bit to clear)
// 
int atbm_powerave_sdio_sync(struct atbm_common *hw_priv)
{
	int ret=0;
	int retry=0;
	u32 config_reg;
	ret = atbm_reg_read_unlock_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &config_reg);
	if (ret < 0) {
		printk("%s: enable_irq: can't read config register.\n", __func__);
	}

	if(config_reg & ATBM_HIFREG_PS_SYNC_SDIO_FLAG)
	{
		printk("%s:%d\n",__func__,__LINE__);
		//atbm_hw_buff_reset(hw_priv);
		config_reg |= ATBM_HIFREG_PS_SYNC_SDIO_CLEAN;
		atbm_reg_write_unlock_32(hw_priv,ATBM_HIFREG_CONFIG_REG_ID,config_reg);
	}
	return ret;
}
#endif  //(PROJ_TYPE>=ARES_B)
int atbm_device_wakeup(struct atbm_common *hw_priv)
{
	u16 ctrl_reg;
	int ret=0;
	int loop = 1;

#ifdef PS_SETUP

	/* To force the device to be always-on, the host sets WLAN_UP to 1 */
	ret = atbm_reg_write_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID,
			ATBM_HIFREG_CONT_WUP_BIT);
	if (WARN_ON(ret))
		return ret;
#endif
	while(1){
		mdelay(2);
		ret = atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
		if (WARN_ON(ret)){
		}
		/* If the device returns WLAN_RDY as 1, the device is active and will
		 * remain active. */
		printk("Rdy =%x\n",ctrl_reg);
		if (ctrl_reg & ATBM_HIFREG_CONT_RDY_BIT) {
			printk(KERN_DEBUG "[BH] Device awake.<%d>\n",loop);
			ret= 1;
			break;
		}
	}
	return ret;
}
int rxMutiCnt[17]={0};
int rxCnt=0;
int rxMutiCnt_Num;
unsigned long g_printf_count = 0;
int atbm_rx_tasklet(struct atbm_common *hw_priv, int id,
		  struct wsm_hdr *wsm, struct sk_buff **skb_p)
{
	struct sk_buff *skb = *skb_p;
	struct sk_buff *atbm_skb_copy;
	//struct wsm_hdr *wsm;
	u32 wsm_len;
	int wsm_id;
	int data_len;
#define RX_ALLOC_BUFF_OFFLOAD (  (40+16)/*RX_DESC_OVERHEAD*/ -16 /*WSM_HI_RX_IND*/)

		wsm_len = __le32_to_cpu(wsm->len);
		wsm_id	= __le32_to_cpu(wsm->id) & 0xFFF;
		if(wsm_id == WSM_MULTI_RECEIVE_INDICATION_ID){
			struct wsm_multi_rx *  multi_rx = (struct wsm_multi_rx *)skb->data;			
			int RxFrameNum = multi_rx->RxFrameNum;
			
			data_len = wsm_len ;
			data_len -= sizeof(struct wsm_multi_rx);
			
			rxMutiCnt[ALIGN(wsm_len,1024)/1024]++;
			rxMutiCnt_Num+=RxFrameNum;
			
			wsm = (struct wsm_hdr *)(multi_rx+1);
			wsm_len = __le32_to_cpu(wsm->len);
			wsm_id	= __le32_to_cpu(wsm->id) & 0xFFF;
			
			//frame_hexdump("dump sdio wsm rx ->",wsm,32);
			do {

				if(data_len < wsm_len){
					printk("skb->len %x,wsm_len %x data_len %x\n",skb->len,wsm_len,data_len);
					//frame_hexdump("dump sdio wsm rx ->",skb->data,64);
					break;
				}
				BUG_ON((wsm_id  & (~WSM_TX_LINK_ID(WSM_TX_LINK_ID_MAX))) !=  WSM_RECEIVE_INDICATION_ID);
				atbm_skb_copy = __atbm_dev_alloc_skb(wsm_len + 16,GFP_KERNEL);
				BUG_ON(atbm_skb_copy == NULL);
				atbm_skb_reserve(atbm_skb_copy,  (8 - (((unsigned long)atbm_skb_copy->data)&7))/*ALIGN 8*/);
				
				memmove(atbm_skb_copy->data, wsm, wsm_len);
				atbm_skb_put(atbm_skb_copy,wsm_len);
				wsm_handle_rx(hw_priv,wsm_id,wsm,&atbm_skb_copy);
				data_len -= ALIGN(wsm_len + RX_ALLOC_BUFF_OFFLOAD,4);
				RxFrameNum--;

				wsm = (struct wsm_hdr *)((u8 *)wsm +ALIGN(( wsm_len + RX_ALLOC_BUFF_OFFLOAD),4));
				wsm_len = __le32_to_cpu(wsm->len);
				wsm_id	= __le32_to_cpu(wsm->id) & 0xFFF;
				
				if(atbm_skb_copy != NULL){
					atbm_dev_kfree_skb(atbm_skb_copy);
				}
			}while((RxFrameNum>0) && (data_len > 32));
			BUG_ON(RxFrameNum != 0);
			
		}
		else {
			//rxMutiCnt[ALIGN(wsm_len,1024)/1024]++;
			rxCnt++;
			return wsm_handle_rx(hw_priv,id,wsm,skb_p);
		}
		return 0;

}
//#define DEBUG_SDIO
#ifdef ATBM_SDIO_PATCH
#define CHECKSUM_LEN 4
static u32 GloId_array[64]={0};
u16 atbm_CalCheckSum(const u8 *data,u16 len)
{
  u16 t;
  const u8 *dataptr;
  const u8 *last_byte;
  u16 sum = 0;
  dataptr = data;
  last_byte = data + len - 1;
  while(dataptr < last_byte) {	/* At least two more bytes */
    t = (dataptr[0] << 8) + dataptr[1];
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
    dataptr += 2;
  }
  
  if(dataptr == last_byte) {
    t = (dataptr[0] << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
  }
  sum=(~sum)&0xffff;

  /* Return sum in host byte order. */
  return sum;
}
void atbm_packetId_to_seq(struct atbm_common *hw_priv,u32 packetId)
{
	GloId_array[hw_priv->SdioSeq]=packetId;
}
int atbm_seq_to_packetId(struct atbm_common *hw_priv,u32 seq)
{
	u32 packetId;
	packetId=GloId_array[seq];
	GloId_array[seq]=0;
	return packetId;
}
#endif //ATBM_SDIO_PATCH

#ifndef ATBM_SDIO_TXRX_ENHANCE
static int atbm_bh(void *arg)
{
	struct atbm_common *hw_priv = arg;
	struct atbm_vif *priv = NULL;
	struct sk_buff *skb_rx = NULL;
	u32 read_len;
	u32 read_len_lsb = 0;
	u32 read_len_msb = 0;
	int rx, tx=0, term, suspend;
#ifdef ATBM_SDIO_PATCH
	struct wsm_tx *wsm_txData;
	struct atbm_seq_bit_map *bitmap;
#endif
	struct wsm_hdr_tx *wsm_tx;
	struct wsm_hdr *wsm;
	u32 wsm_len;
	int wsm_id;
	u8 wsm_seq;
	int rx_resync = 1;
	u16 ctrl_reg = 0;
	int tx_allowed;
	int pending_tx = 0;
	int tx_burst;
	int rx_burst = 0;
	long status;
	bool powersave_enabled;
	int i;
	int vif_selected;
	unsigned long g_printf_time = 0;
	int ret_flush;				
	g_printf_time= jiffies;
	
	AggrBuffer= atbm_kmalloc(SDIO_TX_MAXLEN,GFP_KERNEL | GFP_DMA);
	if (!AggrBuffer) {
		atbm_dbg(ATBM_APOLLO_DBG_ERROR,
			"%s: can't allocate bootloader buffer.\n", __func__);
		return 0;
	}
	atbm_bh_wake_lock(hw_priv);
#define __ALL_HW_BUFS_USED (hw_priv->hw_bufs_used)
	while (1) {
#ifdef RESET_CHANGE
reset:
#endif
		powersave_enabled = 1;
		atbm_hw_vif_read_lock(&hw_priv->vif_list_lock);
		atbm_for_each_vif_safe(hw_priv, priv, i) 
		{
			if (!priv)
				continue;
			powersave_enabled &= !!priv->powersave_enabled;
		}
		atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
		if (!__ALL_HW_BUFS_USED
				&& powersave_enabled
				&& !hw_priv->device_can_sleep
				&& !atomic_read(&hw_priv->recent_scan)) {
			status = HZ/8;
			bh_printk(KERN_DEBUG "[BH] No Device wakedown.\n");
#ifdef PS_SETUP
			WARN_ON(atbm_reg_write_16(hw_priv,
						ATBM_HIFREG_CONTROL_REG_ID, 0));
			hw_priv->device_can_sleep = true;
#endif
		} else if (__ALL_HW_BUFS_USED)
			/* Interrupt loss detection */
			status = HZ;
		else
			status = HZ/20;
		/* If a packet has already been txed to the device then read the
		   control register for a probable interrupt miss before going
		   further to wait for interrupt; if the read length is non-zero
		   then it means there is some data to be received */
#if (ATBM_WIFI_PLATFORM != PLATFORM_AMLOGIC_905)
		if (__ALL_HW_BUFS_USED)
		{
			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			if(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)
			{
				rx = 1;
				goto txrx;
			}
		}
#endif		
		atbm_bh_wake_unlock(hw_priv);
#ifdef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
	   if(hw_priv->init_done){
			atbm_oob_intr_set(hw_priv->sbus_priv, 1);
	   }
#endif
		status = wait_event_interruptible_timeout(hw_priv->bh_wq, ({
				rx = atomic_xchg(&hw_priv->bh_rx, 0);
				tx = atomic_xchg(&hw_priv->bh_tx, 0);
				term = atomic_xchg(&hw_priv->bh_term, 0);
				suspend = pending_tx ?
					0 : atomic_read(&hw_priv->bh_suspend);
				(rx || tx || term || suspend || hw_priv->bh_error);
			}), status);
		atbm_bh_wake_lock(hw_priv);
		if (status < 0 || term || hw_priv->bh_error){
			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			//printk(" ++ctrl_reg= %x,\n",ctrl_reg);
			printk("%s BH thread break %ld %d %d ctrl_reg=%x\n",__func__,status,term,hw_priv->bh_error,ctrl_reg);
			break;
		}

		if (!status) {
			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			if(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)
			{
				int ret;
				ret=atbm_direct_write_reg_32(hw_priv,0xac8029c,0x7ff);
				if(ret){
					printk("0xac8029c Error\n");
				}
				printk(KERN_ERR "MISS 1\n");

#if (ATBM_WIFI_PLATFORM == PLATFORM_AMLOGIC_905)
				printk(KERN_ERR "bh_rx=%d\n", atomic_read(&hw_priv->bh_rx));
#ifndef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
				hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
				__atbm_irq_dbgPrint(hw_priv);
				hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);				
#endif
#endif
				rx = 1;
				goto txrx;
			}
		}
		if (!status && __ALL_HW_BUFS_USED)
		{
			unsigned long timestamp = jiffies;
			long timeout;
			bool pending = false;
			int i;

			wiphy_warn(hw_priv->hw->wiphy, "Missed interrupt Status =%d, buffused=%d\n",(int)status,(int)__ALL_HW_BUFS_USED);
			rx = 1;
			printk("[bh] next wsm_rx_seq %d wsm_tx_seq %d\n",hw_priv->wsm_rx_seq,hw_priv->wsm_tx_seq);
			printk("[bh] wsm_hiftx_cmd_num %d wsm_hif_cmd_conf_num %d\n",hw_priv->wsm_hiftx_num,hw_priv->wsm_hifconfirm_num);
			printk("[bh] wsm_txframe_num %d wsm_txconfirm_num %d\n",hw_priv->wsm_txframe_num,hw_priv->wsm_txconfirm_num);
			printk("[bh] num_pending[0]=%d num_pending[1]=%d pending[2]=%d pending[3]=%d\n",
															hw_priv->tx_queue[0].num_pending,
															hw_priv->tx_queue[1].num_pending,
															hw_priv->tx_queue[2].num_pending,
															hw_priv->tx_queue[3].num_pending);
			//atbm_monitor_pc(hw_priv);

			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			printk(KERN_DEBUG" ++ctrl_reg= %x,\n",ctrl_reg);

			/* Get a timestamp of "oldest" frame */
			for (i = 0; i < 4; ++i)
				pending |= atbm_queue_get_xmit_timestamp(
						&hw_priv->tx_queue[i],
						&timestamp, -1,
						hw_priv->pending_frame_id);

			/* Check if frame transmission is timed out.
			 * Add an extra second with respect to possible
			 * interrupt loss. */
			timeout = timestamp +
					WSM_CMD_LAST_CHANCE_TIMEOUT +
					1 * HZ  -
					jiffies;

			/* And terminate BH tread if the frame is "stuck" */
			if (pending && timeout < 0) {
				ret_flush=wsm_sync_channle_process(hw_priv,IN_BH);
				if(ret_flush==RECOVERY_ERR){
					printk(KERN_ERR "RESET CHANN ERR %d\n",__LINE__);
					break;
				}else{
					ctrl_reg=0;
					continue;
				}
			}
		} else if (!status) {
			if (!hw_priv->device_can_sleep
					&& !atomic_read(&hw_priv->recent_scan)) {
			        bh_printk(KERN_DEBUG "[BH] Device wakedown. Timeout.\n");
#ifdef PS_SETUP
				WARN_ON(atbm_reg_write_16(hw_priv,
						ATBM_HIFREG_CONTROL_REG_ID, 0));
				hw_priv->device_can_sleep = true;
#endif//#ifdef PS_SETUP
			}
			continue;
		} else if (suspend) {
			printk(KERN_ERR "[BH] Device suspend.\n");
			powersave_enabled = 1;
			//if in recovery clear reg
			if(hw_priv->syncChanl_done==0){
			     ctrl_reg=0;
			}
			atbm_hw_vif_read_lock(&hw_priv->vif_list_lock);
			atbm_for_each_vif_safe(hw_priv, priv, i) {
				if (!priv)
					continue;
				powersave_enabled &= !!priv->powersave_enabled;
			}
			atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
			if (powersave_enabled) {
				bh_printk(KERN_DEBUG "[BH] No Device wakedown. Suspend.\n");
			}

			atomic_set(&hw_priv->bh_suspend, ATBM_APOLLO_BH_SUSPENDED);
			wake_up(&hw_priv->bh_evt_wq);
			printk(KERN_ERR "[BH] wait resume..\n");
			status = wait_event_interruptible(hw_priv->bh_wq,
					ATBM_APOLLO_BH_RESUME == atomic_read(
						&hw_priv->bh_suspend));
			if (status < 0) {
				wiphy_err(hw_priv->hw->wiphy,
					"%s: Failed to wait for resume: %ld.\n",
					__func__, status);
				break;
			}
			printk(KERN_ERR "[BH] Device resume.\n");
			atomic_set(&hw_priv->bh_suspend, ATBM_APOLLO_BH_RESUMED);
			wake_up(&hw_priv->bh_evt_wq);
			atomic_add(1, &hw_priv->bh_rx);
			continue;
		}
txrx:
		tx += pending_tx;
		pending_tx = 0;

		if (rx) {
			u32 alloc_len;
			u8 *data;

			if(!(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)){
				if (WARN_ON(atbm_bh_read_ctrl_reg(
						hw_priv, &ctrl_reg)))
						break;
			}
rx:

			read_len_lsb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK)*2;
			read_len_msb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK)*2;
			read_len=((read_len_msb>>2)+read_len_lsb);
			if (!read_len) {
				rx_burst = 0;
				ctrl_reg = 0;
				goto __rxend;
			}
#ifdef PS_SETUP
			if (hw_priv->device_can_sleep) {
				//atbm_powerave_sdio_sync(hw_priv);
			}
#endif	
			if (WARN_ON((read_len < sizeof(struct wsm_hdr)) ||
					(read_len > EFFECTIVE_BUF_SIZE))) {
					printk(KERN_DEBUG "Invalid read len: %d",
						read_len);
				break;
			}
			/* Add SIZE of PIGGYBACK reg (CONTROL Reg)
			 * to the NEXT Message length + 2 Bytes for SKB */
			read_len = read_len + 2;
			alloc_len = read_len;
			if (alloc_len % SDIO_BLOCK_SIZE ) {
				alloc_len -= (alloc_len % SDIO_BLOCK_SIZE );
				alloc_len += SDIO_BLOCK_SIZE;
			}
			/* Check if not exceeding CW1200 capabilities */
			if (WARN_ON_ONCE(alloc_len > EFFECTIVE_BUF_SIZE)) {
				printk(KERN_DEBUG "Read aligned len: %d\n",
					alloc_len);
			}
			skb_rx = atbm_get_skb(hw_priv, alloc_len);
			if (WARN_ON(!skb_rx))
				break;

			atbm_skb_trim(skb_rx, 0);
			atbm_skb_put(skb_rx, read_len);
			data = skb_rx->data;
			if (WARN_ON(!data))
				break;
			if (WARN_ON(atbm_data_read(hw_priv, data, alloc_len))){
				break;
			}
			/* Piggyback */
			ctrl_reg = __le16_to_cpu(
				((__le16 *)data)[alloc_len / 2 - 1]);
			wsm = (struct wsm_hdr *)data;
			//frame_hexdump("dump sdio wsm rx ->",wsm,32);
			wsm_len = __le32_to_cpu(wsm->len);
			if (WARN_ON(wsm_len > read_len)){
				printk("<WARNING>  wsm_len =%d,read_len =%d\n",wsm_len,read_len);
				ret_flush=wsm_sync_channle_process(hw_priv,IN_BH);
				if(ret_flush==RECOVERY_ERR){
					printk(KERN_ERR "RESET CHANN ERR %d\n",__LINE__);
					break;
				}else{
					ctrl_reg=0;
					//free skb
					dev_kfree_skb(skb_rx);
					skb_rx=NULL;
					continue;
				}
			}
			wsm_id  = __le32_to_cpu(wsm->id) & 0xFFF;
			wsm_seq = (__le32_to_cpu(wsm->id) >> 13) & WSM_RX_SEQ_MAX;
			atbm_skb_trim(skb_rx, wsm_len);
			if (unlikely(wsm_id == 0x0800)) {
				int recovery_status =0;
				wsm_handle_exception(hw_priv,
					 &data[sizeof(*wsm)],
					wsm_len - sizeof(*wsm));
				ret_flush=wsm_sync_channle_process(hw_priv,IN_BH);
				if(ret_flush==RECOVERY_ERR){
					printk(KERN_ERR "RESET CHANN ERR %d\n",__LINE__);
					hw_priv->bh_error = 1;
					break;
				}else{
					ctrl_reg=0;
					//free skb
					dev_kfree_skb(skb_rx);
					skb_rx=NULL;
					//goto txrx;
 					continue;
				}
			} else if (unlikely(!rx_resync)) {
				if (WARN_ON(wsm_seq != hw_priv->wsm_rx_seq)) {
					ret_flush=wsm_sync_channle_process(hw_priv,IN_BH);
					if(ret_flush==RECOVERY_ERR){
						printk(KERN_ERR "RESET CHANN ERR %d\n",__LINE__);
						hw_priv->bh_error = 1;
						break;
					}else{
						ctrl_reg=0;
						//free skb
						dev_kfree_skb(skb_rx);
						skb_rx=NULL;
						continue;
					}
					break;
				}
			}
			bh_printk("wsm_rx_seq,wsm_seq=%d:wsm_seq=%d,wsm_id=%x\n",hw_priv->wsm_rx_seq,wsm_seq,wsm_id);
			hw_priv->wsm_rx_seq = (wsm_seq + 1) & WSM_RX_SEQ_MAX;
			rx_resync = 0;

			if (wsm_id & 0x0400) {
				int rc = wsm_release_tx_buffer(hw_priv, 1);
				if (WARN_ON(rc < 0))
					break;
				else if (rc > 0)
					tx = 1;
			}
			/* atbm_wsm_rx takes care on SKB livetime */
			if (WARN_ON(atbm_rx_tasklet(hw_priv, wsm_id, wsm,
						  &skb_rx))){
				ret_flush=wsm_sync_channle_process(hw_priv,IN_BH);
                                 if(ret_flush==RECOVERY_ERR){
                                                printk(KERN_ERR "RESET CHANN ERR %d\n",__LINE__);
                                                hw_priv->bh_error = 1;
                                                break;
                                  }else{
                                                ctrl_reg=0;
                                                //free skb
                                                dev_kfree_skb(skb_rx);
                                                skb_rx=NULL;
                                                continue;
                                  }
 				break;
			}
#ifdef 	DEBUG_SDIO
			if(time_is_before_jiffies(g_printf_time+5*HZ)){ 
				
				int i;
				g_printf_count++;
				printk("WSM:id=0x%x,txlen %d,putLen %d wsm_tx->flag %d txMutiFrameCount %d Seconds %d\n",wsm_tx->id,tx_len,putLen,wsm_tx->flag,txMutiFrameCount,(g_printf_count*10));
				printk("rxMutiCnt_Num=%d,rxCnt=%d,alloc_len=%d\n",rxMutiCnt_Num,rxCnt,alloc_len);
				for ( i=1;i<17;i++){
					printk("rxMutiCnt[%d]K=%d\n",i,rxMutiCnt[i]);
				}
				printk("packetNum=%d,blockNum=%d\n",packetNum,blockNum);
				 g_printf_time=jiffies;
				rxCnt=0;rxMutiCnt_Num=0;
				memset(rxMutiCnt,0,sizeof(rxMutiCnt));
			}			
#endif
			if (skb_rx) {
				atbm_put_skb(hw_priv, skb_rx);
				skb_rx = NULL;
			}
__rxend:
			read_len = 0;

#if ((ATBM_WIFI_PLATFORM == PLATFORM_XUNWEI) || (ATBM_WIFI_PLATFORM == PLATFORM_AMLOGIC_905))
#ifndef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
#if (ATBM_WIFI_PLATFORM == PLATFORM_AMLOGIC_905)
			if ((ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK) == 0)
#endif
			{
				hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
				__atbm_irq_enable(hw_priv,1);
				hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
			}
#endif
#endif
#if 1
			if (rx_burst) {
				atbm_debug_rx_burst(hw_priv);
				--rx_burst;
				goto rx;
			}
#endif
#ifdef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ

			atbm_oob_intr_set(hw_priv->sbus_priv, 1);
#endif

			ATBM_OS_WAKE_UNLOCK(hw_priv);
		}

tx:
		//printk(KERN_ERR "sdio BH %d\n",tx_burst);

		BUG_ON(hw_priv->hw_bufs_used > hw_priv->wsm_caps.numInpChBufs);
		tx_burst = (hw_priv->wsm_caps.numInpChBufs - hw_priv->hw_bufs_used);

		tx_allowed = tx_burst > 0;
		if (tx && 
			tx_allowed &&
			hw_priv->syncChanl_done
		) {
		#ifdef  ATBM_WSM_SDIO_TX_MULT
		#define WSM_SDIO_TX_MULT_BLOCK_SIZE	(6*SDIO_BLOCK_SIZE)
		#else
		#define WSM_SDIO_TX_MULT_BLOCK_SIZE	(SDIO_BLOCK_SIZE)
		#endif
			u32 tx_len=0;
			u32 putLen=0;
			#ifdef ATBM_SDIO_PATCH
			u32 offLen;
			#endif
			u8 *data;
			int ret=0;
			int txMutiFrameCount=0;
#if (PROJ_TYPE>=ARES_A)
			u32 wsm_flag_u32 = 0;
			u16 wsm_len_u16[2];
			u16 wsm_len_sum;
#endif
			if (hw_priv->device_can_sleep) {
				ret = atbm_device_wakeup(hw_priv);
				if (WARN_ON(ret < 0))
					break;
				else if (ret)
					hw_priv->device_can_sleep = false;
				else {
					/* Wait for "awake" interrupt */
					pending_tx = tx;
					continue;
				}
			}
			memset(&AggrBuffer[0],0,SDIO_TX_MAXLEN);
			do {
				
				tx_burst = (hw_priv->wsm_caps.numInpChBufs - hw_priv->hw_bufs_used); 

				if(tx_burst==0){
					if(txMutiFrameCount >0)
						break;
					else
						goto tx_end;
						
				}
				ret = wsm_get_tx(hw_priv, &data, &tx_len, &tx_burst,&vif_selected);
				//printk(KERN_ERR "tx_burst1 %d\n",tx_burst);
				if (ret <= 0) {
					//tx_burst=0;
					//printk(KERN_ERR "tx_burst2 %d\n",tx_burst);
					if(txMutiFrameCount >0)
						break;
					else
						goto tx_end;
				}
				else	{
						txMutiFrameCount++;
						wsm_tx = (struct wsm_hdr_tx *)data;
						BUG_ON(tx_len < sizeof(*wsm_tx));
						BUG_ON(__le32_to_cpu(wsm_tx->len) != tx_len);
#if (PROJ_TYPE>=ARES_A)
						wsm_flag_u32 = (tx_len) & 0xffff;
						wsm_len_u16[0] = wsm_flag_u32 & 0xff;
						wsm_len_u16[1] = (wsm_flag_u32 >> 8)& 0xff;
						wsm_len_sum = wsm_len_u16[0] + wsm_len_u16[1];
						if (wsm_len_sum & BIT(8))
						{
							wsm_flag_u32 |= ((wsm_len_sum + 1) & 0xff) << 24;
						}else
						{
							wsm_flag_u32 |= (wsm_len_sum & 0xff) << 24;
						}
						wsm_tx->flag=__cpu_to_le32(wsm_flag_u32);
						//printk("wsm_tx2,flag=0x%x\n", wsm_tx->flag);
#endif
						if (tx_len <= 8)
							tx_len = 16;

						if (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) ) {
							tx_len -= (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) );
							tx_len += WSM_SDIO_TX_MULT_BLOCK_SIZE;
						}

						/* Check if not exceeding CW1200
						capabilities */
						if (WARN_ON_ONCE(tx_len > EFFECTIVE_BUF_SIZE)) {
							printk(KERN_DEBUG "Write aligned len:"
							" %d\n", tx_len);
						}
#if (PROJ_TYPE<ARES_A)					
						wsm_tx->flag=(((tx_len/SDIO_BLOCK_SIZE)&0xff)-1);
#endif //(PROJ_TYPE!=ARES_A)
						wsm_tx->id &= __cpu_to_le32(~WSM_TX_SEQ(WSM_TX_SEQ_MAX));
						wsm_tx->id |= cpu_to_le32(WSM_TX_SEQ(hw_priv->wsm_tx_seq));
#ifdef  ATBM_WSM_SDIO_TX_MULT
						wsm_tx->tx_len = tx_len;
						wsm_tx->tx_id = wsm_tx->id;
#endif //ATBM_WSM_SDIO_TX_MULT
						bh_printk(KERN_DEBUG "tx wsm %x,hw_bufs_used %d\n",wsm_tx->id,hw_priv->hw_bufs_used);
						#ifdef ATBM_SDIO_PATCH
						bitmap= (struct atbm_seq_bit_map*)atbm_kmalloc(sizeof(struct atbm_seq_bit_map),GFP_ATOMIC);
						if(!bitmap){
							atbm_dbg(ATBM_APOLLO_DBG_ERROR,
								"%s: can't Alloc bitmap buffer.\n", __func__);
							return 0;
						}
						memset(bitmap,0,sizeof(struct atbm_seq_bit_map));
						//Add sdioSeq Num
						wsm_tx->sdioSeq=cpu_to_le32(hw_priv->SdioSeq);
						//do wsm cmd process
						if (data == hw_priv->wsm_cmd.ptr) { 
							//do cmd header checksum
							wsm_tx->headCsm=0;
							//wsm_tx->len+=4;
							wsm_tx->headCsm=(atbm_CalCheckSum((u8*)(wsm_tx),16)|0x88880000);//Only protect 16 byte
							offLen=wsm_tx->len;
							if(offLen%4){
								offLen-=(offLen%4);
								offLen+=4;
							}
							//do cmd body checksum
							*(u32*)((u8*)wsm_tx+offLen-CHECKSUM_LEN)=atbm_CalCheckSum((u8*)wsm_tx+sizeof(*wsm_tx),offLen-sizeof(*wsm_tx)-CHECKSUM_LEN);
							bitmap->bitm.DataFlag=IS_CMD;
						}else{/*do data proess*/
							//do data header checksum
							wsm_tx->headCsm=0;
							wsm_tx->headCsm=(atbm_CalCheckSum((u8*)(wsm_tx),16));//Only protect 16 byte
							wsm_txData=(struct wsm_tx*)data;
							bitmap->bitm.packetId=wsm_txData->packetID;
							if(!(wsm_txData->htTxParameters & WSM_NEED_TX_CONFIRM)){
								bitmap->bitm.DataFlag=IS_DATA;
							}else{
								atbm_packetId_to_seq(hw_priv,wsm_txData->packetID);
								bitmap->bitm.DataFlag=IS_MGMT;
							}
						}
						bitmap->bitm.HmacSsn=hw_priv->SdioSeq;
						spin_lock_bh(&hw_priv->SeqBitMapLock);
						list_add_tail(&bitmap->link, &hw_priv->SeqBitMapList);
						spin_unlock_bh(&hw_priv->SeqBitMapLock);
						#endif//ATBM_SDIO_PATCH
						wsm_alloc_tx_buffer(hw_priv);
						memcpy(&AggrBuffer[putLen],data,wsm_tx->len);

						putLen+=tx_len;
						hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq + 1) & WSM_TX_SEQ_MAX;
#ifdef ATBM_SDIO_PATCH
						hw_priv->SdioSeq=(hw_priv->SdioSeq+1)&WSM_TX_SDIO_SEQ_MAX;
#endif

						if (vif_selected != -1) {
							hw_priv->hw_bufs_used_vif[vif_selected]++;
						}
						if(wsm_txed(hw_priv, data)){
							hw_priv->wsm_hiftx_num++;
							break;
						}else{
							hw_priv->wsm_txframe_num++;
						}
						if (putLen+hw_priv->wsm_caps.sizeInpChBuf>SDIO_TX_MAXLEN)
						{
							break;
						}
					}
				}while(1);
				hw_priv->buf_id_offset=txMutiFrameCount;
				atomic_add(1, &hw_priv->bh_tx);

				if (WARN_ON(atbm_data_write(hw_priv,&AggrBuffer[0], putLen))) {
					wsm_release_tx_buffer(hw_priv,txMutiFrameCount);
					break;
				}
				if (tx_burst > 1) {
					atbm_debug_tx_burst(hw_priv);
					++rx_burst;
					goto tx;
				}
			}
tx_end:
		if (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)
			goto rx;
	}
	atbm_bh_wake_unlock(hw_priv);

	if (skb_rx) {
		atbm_put_skb(hw_priv, skb_rx);
		skb_rx = NULL;
	}
	if (!term) {
		int loop = 3;
		atbm_dbg(ATBM_APOLLO_DBG_ERROR, "[BH] Fatal error, exitting.\n");
		hw_priv->bh_error = 1;
		while(loop-->0){
			atbm_monitor_pc(hw_priv);
			msleep(10);
		}
#ifdef RESET_CHANGE
atbm_rest:
		WARN_ON((atomic_add_return(1, &hw_priv->reset_flag) != 1));
		WARN_ON((atomic_add_return(1, &hw_priv->fw_reloading) != 1));
		atomic_xchg(&hw_priv->bh_rx, 0);
		atomic_xchg(&hw_priv->bh_tx, 0);
		term = atomic_xchg(&hw_priv->bh_term, 0);
		atomic_xchg(&hw_priv->bh_suspend, 0);
		atomic_add_return(1,&hw_priv->reset_conter);
		/*set reset flag = 1,  not allow send packet to lmac*/
		hw_priv->init_done = 0;
		hw_priv->bh_error = 0;
		rx =0;
		tx=0;
		suspend =0;
		if(atbm_wtd_term(hw_priv))
		{
			atomic_set(&hw_priv->bh_term,term);
			goto bh_stop;
		}
		term=0;
		if(atomic_read(&hw_priv->reset_conter)>3)
		{
			goto bh_stop;
		}
		hw_priv->sbus_ops->wtd_wakeup(hw_priv->sbus_priv);
		printk("wtd_wakeup--\n");
wait_fw_loaded:
		status = wait_event_interruptible_timeout(hw_priv->bh_wq, hw_priv->init_done|hw_priv->bh_error,
	       50*HZ);

		if((!status)||(hw_priv->bh_error))
		{
			printk("fw loaded err\n");
			goto atbm_rest;
		}
		else if(status&&hw_priv->init_done)
		{
			printk("fw has been loaded,reset\n");
			goto reset;
		}

		goto wait_fw_loaded;
		
bh_stop:
		printk("sorry!!,we can not reload the fw !!!\n");
#else
		hw_priv->sbus_ops->wtd_wakeup(hw_priv->sbus_priv);
#endif
		atbm_hw_vif_read_lock(&hw_priv->vif_list_lock);
		atbm_for_each_vif_safe(hw_priv, priv, i) {
			if (!priv)
				continue;
//			ieee80211_driver_hang_notify(priv->vif, GFP_KERNEL);
		}
		atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
		#ifdef CONFIG_PM
		atbm_pm_stay_awake(&hw_priv->pm_state, 3*HZ);
		#endif
		/* TODO: schedule_work(recovery) */
#ifndef HAS_PUT_TASK_STRUCT
		/* The only reason of having this stupid code here is
		 * that __put_task_struct is not exported by kernel. */
		for (;;) {
			int status = wait_event_interruptible(hw_priv->bh_wq, ({
				term = atomic_xchg(&hw_priv->bh_term, 0);
				(term);
				}));

			if (status || term)
				break;
		}
#endif
	}
	printk("atbm_wifi_BH_thread stop ++\n");
	atbm_kfree(AggrBuffer);
	/*
	add this code just because 'linux kernel' need kthread not exit ,
	before kthread_stop func call,
	*/
	if(term)
	{
		//clear pendding cmd
		if(!mutex_trylock(&hw_priv->wsm_cmd_mux))
		{
			spin_lock_bh(&hw_priv->wsm_cmd.lock);
			if(hw_priv->wsm_cmd.cmd != 0xFFFF)
			{
				hw_priv->wsm_cmd.ret = -1;
				hw_priv->wsm_cmd.done = 1;
				spin_unlock_bh(&hw_priv->wsm_cmd.lock);
				printk(KERN_DEBUG "cancle current pendding cmd,release wsm_cmd.lock\n");
				wake_up(&hw_priv->wsm_cmd_wq);
				msleep(2);
				spin_lock_bh(&hw_priv->wsm_cmd.lock);
			}
			spin_unlock_bh(&hw_priv->wsm_cmd.lock);
		}
		else
		{
			mutex_unlock(&hw_priv->wsm_cmd_mux);
		}

		/*
		*cancle the current scanning process
		*/
		mutex_lock(&hw_priv->conf_mutex);
		if(atomic_read(&hw_priv->scan.in_progress))
		{
			struct atbm_vif *scan_priv = ABwifi_hwpriv_to_vifpriv(hw_priv,
						hw_priv->scan.if_id);
			bool scanto_running = false;
			atbm_priv_vif_list_read_unlock(&scan_priv->vif_lock);
			mutex_unlock(&hw_priv->conf_mutex);
			scanto_running = atbm_cancle_delayed_work(&hw_priv->scan.timeout,true);
			mutex_lock(&hw_priv->conf_mutex);
			if(scanto_running>0)
			{
				hw_priv->scan.curr = hw_priv->scan.end;
				mutex_unlock(&hw_priv->conf_mutex);
				atbm_scan_timeout(&hw_priv->scan.timeout.work);
				mutex_lock(&hw_priv->conf_mutex);
			}
		}
		mutex_unlock(&hw_priv->conf_mutex);
		{
			u8 i = 0;
			//cancel pendding work
			#define ATBM_CANCEL_PENDDING_WORK(work,work_func)			\
				do{														\
					if(atbm_cancle_queue_work(work,true)==true)			\
					{													\
						work_func(work);								\
					}													\
				}														\
				while(0)
					
			if(atbm_cancle_delayed_work(&hw_priv->scan.probe_work,true))
				atbm_probe_work(&hw_priv->scan.probe_work.work);
			if(atbm_cancle_delayed_work(&hw_priv->rem_chan_timeout,true))
				atbm_rem_chan_timeout(&hw_priv->rem_chan_timeout.work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.work,atbm_scan_work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->event_handler,atbm_event_handler);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->ba_work,atbm_ba_work);
			#ifdef ATBM_SUPPORT_WIDTH_40M
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->get_cca_work,atbm_get_cca_work);
			del_timer_sync(&hw_priv->chantype_timer);
			#endif
			#ifdef ATBM_SUPPORT_SMARTCONFIG
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.smartwork,atbm_smart_scan_work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.smartsetChanwork,atbm_smart_setchan_work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.smartstopwork,atbm_smart_stop_work);
			del_timer_sync(&hw_priv->smartconfig_expire_timer);
			#endif
			del_timer_sync(&hw_priv->ba_timer);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->tx_policy_upload_work,tx_policy_upload_work);
			atbm_for_each_vif(hw_priv, priv, i) {
				if(priv == NULL)
					continue;
				ATBM_CANCEL_PENDDING_WORK(&priv->wep_key_work,atbm_wep_key_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->join_work,atbm_join_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->unjoin_work,atbm_unjoin_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->offchannel_work,atbm_offchannel_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->link_id_work,atbm_link_id_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->tx_failure_work,atbm_tx_failure_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->set_tim_work, atbm_set_tim_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->multicast_start_work,atbm_multicast_start_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->multicast_stop_work, atbm_multicast_stop_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->linkid_reset_work, atbm_link_id_reset);
				ATBM_CANCEL_PENDDING_WORK(&priv->update_filtering_work, atbm_update_filtering_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->set_beacon_wakeup_period_work,
											atbm_set_beacon_wakeup_period_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->ht_info_update_work, atbm_ht_info_update_work);
				#ifdef ATBM_SUPPORT_WIDTH_40M
				//ATBM_CANCEL_PENDDING_WORK(&priv->chantype_change_work, atbm_channel_type_change_work);
				if(atbm_cancle_delayed_work(&priv->chantype_change_work,true))
					atbm_channel_type_change_work(&priv->chantype_change_work.work);
				#endif
				
				atbm_cancle_delayed_work(&priv->dhcp_retry_work,true);					
				if(atbm_cancle_delayed_work(&priv->bss_loss_work,true))
					atbm_bss_loss_work(&priv->bss_loss_work.work);
				if(atbm_cancle_delayed_work(&priv->connection_loss_work,true))
					atbm_connection_loss_work(&priv->connection_loss_work.work);
				if(atbm_cancle_delayed_work(&priv->set_cts_work,true))
					atbm_set_cts_work(&priv->set_cts_work.work);
				if(atbm_cancle_delayed_work(&priv->link_id_gc_work,true))
					atbm_link_id_gc_work(&priv->link_id_gc_work.work);
				if(atbm_cancle_delayed_work(&priv->pending_offchanneltx_work,true))
					atbm_pending_offchanneltx_work(&priv->pending_offchanneltx_work.work);
				if(atbm_cancle_delayed_work(&priv->join_timeout,true))
					atbm_join_timeout(&priv->join_timeout.work);	
				del_timer_sync(&priv->mcast_timeout);
				
			}
		}
	}
	while(term){
		if(kthread_should_stop()){
			break;
		}
		schedule_timeout_uninterruptible(msecs_to_jiffies(100));
	}
	printk("atbm_wifi_BH_thread stop --\n");
	return 0;
}

#else

static int atbm_bh(void *arg)
{
	struct atbm_common *hw_priv = arg;
	struct atbm_vif *priv = NULL;
	struct sk_buff *skb_rx = NULL;
	int rx, tx=0, term, suspend;
	struct wsm_hdr_tx *wsm_tx;
	struct wsm_hdr *wsm;
	u32 wsm_len;
	int wsm_id;
	u8 wsm_seq;
	int rx_resync = 1;
	u16 ctrl_reg = 0;
	int tx_allowed;
	int pending_tx = 0;
	int tx_burst;
	int rx_burst = 0;
	long status;
	bool powersave_enabled;
	int i;
	int vif_selected;
	unsigned long g_printf_time = 0;
	
	g_printf_time= jiffies;
	AggrBuffer= atbm_kmalloc(SDIO_TX_MAXLEN,GFP_KERNEL | GFP_DMA);
	if (!AggrBuffer) {
		atbm_dbg(ATBM_APOLLO_DBG_ERROR,
			"%s: can't allocate bootloader buffer.\n", __func__);
		return 0;
	}
	atbm_bh_wake_lock(hw_priv);
#define __ALL_HW_BUFS_USED (hw_priv->hw_bufs_used)
	while (1) {
#ifdef RESET_CHANGE
reset:
#endif
		powersave_enabled = 1;
		atbm_hw_vif_read_lock(&hw_priv->vif_list_lock);
		atbm_for_each_vif_safe(hw_priv, priv, i) 
		{
			if (!priv)
				continue;
			powersave_enabled &= !!priv->powersave_enabled;
		}
		atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
		if (!__ALL_HW_BUFS_USED
				&& powersave_enabled
				&& !hw_priv->device_can_sleep
				&& !atomic_read(&hw_priv->recent_scan)) {
			status = HZ/8;
			bh_printk(KERN_DEBUG "[BH] No Device wakedown.\n");
#ifdef PS_SETUP
			WARN_ON(atbm_reg_write_16(hw_priv,
						ATBM_HIFREG_CONTROL_REG_ID, 0));
			hw_priv->device_can_sleep = true;
#endif
		} else if (__ALL_HW_BUFS_USED)
			/* Interrupt loss detection */
			status = HZ/8;
		else
			status = HZ/4;
		/* If a packet has already been txed to the device then read the
		   control register for a probable interrupt miss before going
		   further to wait for interrupt; if the read length is non-zero
		   then it means there is some data to be received */
		if (__ALL_HW_BUFS_USED)
		{
			ctrl_reg= 0;
			ATBM_BH_CHECK_MISS_DATA(hw_priv,rx,txrx,atbm_bh_break);
		}
		atbm_bh_wake_unlock(hw_priv);
		status = wait_event_interruptible_timeout(hw_priv->bh_wq, ({
				rx = atomic_xchg(&hw_priv->bh_rx, 0);
				tx = atomic_xchg(&hw_priv->bh_tx, 0);
				term = atomic_xchg(&hw_priv->bh_term, 0);
				suspend = pending_tx ?
					0 : atomic_read(&hw_priv->bh_suspend);
				(rx || tx || term || suspend || hw_priv->bh_error);
			}), status);
		atbm_bh_wake_lock(hw_priv);
		if (status < 0 || term || hw_priv->bh_error){
			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			printk("%s BH thread break %ld %d %d ctrl_reg=%x\n",__func__,status,term,hw_priv->bh_error,ctrl_reg);
			break;
		}

		if (!status) {
			ctrl_reg = 0;
			ATBM_BH_CHECK_MISS_DATA(hw_priv,rx,txrx,atbm_bh_break);
		}
		if (!status && __ALL_HW_BUFS_USED)
		{
			unsigned long timestamp = jiffies;
			long timeout;
			bool pending = false;
			int i;

			wiphy_warn(hw_priv->hw->wiphy, "Missed interrupt?\n");
			rx = 1;
			printk("[bh] next wsm_rx_seq %d wsm_tx_seq %d\n",hw_priv->wsm_rx_seq,hw_priv->wsm_tx_seq);
			printk("[bh] wsm_txframe_num %d wsm_txconfirm_num %d\n",hw_priv->wsm_txframe_num,hw_priv->wsm_txconfirm_num);
			printk("[bh] num_pending[0]=%d num_pending[1]=%d pending[2]=%d pending[3]=%d\n",
															hw_priv->tx_queue[0].num_pending,
															hw_priv->tx_queue[1].num_pending,
															hw_priv->tx_queue[2].num_pending,
															hw_priv->tx_queue[3].num_pending);
			//atbm_monitor_pc(hw_priv);

			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			printk(" ++ctrl_reg= %x,\n",ctrl_reg);

			/* Get a timestamp of "oldest" frame */
			for (i = 0; i < 4; ++i)
				pending |= atbm_queue_get_xmit_timestamp(
						&hw_priv->tx_queue[i],
						&timestamp, -1,
						hw_priv->pending_frame_id);

			/* Check if frame transmission is timed out.
			 * Add an extra second with respect to possible
			 * interrupt loss. */
			timeout = timestamp +
					WSM_CMD_LAST_CHANCE_TIMEOUT +
					1 * HZ  -
					jiffies;

			/* And terminate BH tread if the frame is "stuck" */
			if (pending && timeout < 0) {
				printk("atbm_bh: Timeout waiting for TX confirm.\n");
				break;
			}
		} else if (!status) {
			if (!hw_priv->device_can_sleep
					&& !atomic_read(&hw_priv->recent_scan)) {
				bh_printk(KERN_DEBUG "[BH] Device wakedown. Timeout.\n");
#ifdef PS_SETUP
				WARN_ON(atbm_reg_write_16(hw_priv,
						ATBM_HIFREG_CONTROL_REG_ID, 0));
				hw_priv->device_can_sleep = true;
#endif//#ifdef PS_SETUP
			}
			continue;
		} else if (suspend) {
			bh_printk(KERN_DEBUG "[BH] Device suspend.\n");
			powersave_enabled = 1;
			atbm_hw_vif_read_lock(&hw_priv->vif_list_lock);
			atbm_for_each_vif_safe(hw_priv, priv, i) {
				if (!priv)
					continue;
				powersave_enabled &= !!priv->powersave_enabled;
			}
			atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
			if (powersave_enabled) {
				bh_printk(KERN_DEBUG "[BH] No Device wakedown. Suspend.\n");
			}

			atomic_set(&hw_priv->bh_suspend, ATBM_APOLLO_BH_SUSPENDED);
			wake_up(&hw_priv->bh_evt_wq);
			status = wait_event_interruptible(hw_priv->bh_wq,
					ATBM_APOLLO_BH_RESUME == atomic_read(
						&hw_priv->bh_suspend));
			if (status < 0) {
				wiphy_err(hw_priv->hw->wiphy,
					"%s: Failed to wait for resume: %ld.\n",
					__func__, status);
				break;
			}
			printk(KERN_ERR "[BH] Device resume.\n");
			atomic_set(&hw_priv->bh_suspend, ATBM_APOLLO_BH_RESUMED);
			wake_up(&hw_priv->bh_evt_wq);
			atomic_add(1, &hw_priv->bh_rx);
			continue;
		}
txrx:
		tx += pending_tx;
		pending_tx = 0;
		if (rx) {
rx:
			while ((skb_rx = atbm_skb_dequeue(&hw_priv->rx_frame_queue)) != NULL) {
				u8 *data;
				data = skb_rx->data;
				wsm = (struct wsm_hdr *)data;
				//frame_hexdump("dump sdio wsm rx ->",wsm,32);
				wsm_len = __le32_to_cpu(wsm->len);
				wsm_id  = __le32_to_cpu(wsm->id) & 0xFFF;
				wsm_seq = (__le32_to_cpu(wsm->id) >> 13) & 7;
				atbm_skb_trim(skb_rx, wsm_len);
				if (unlikely(wsm_id == 0x0800)) {
				wsm_handle_exception(hw_priv,
					 &data[sizeof(*wsm)],
					wsm_len - sizeof(*wsm));
					goto atbm_bh_break;
				} else if (unlikely(!rx_resync)) {
					if (WARN_ON(wsm_seq != hw_priv->wsm_rx_seq)) {
						goto atbm_bh_break;
					}
				}
				hw_priv->wsm_rx_seq = (wsm_seq + 1) & 7;
				rx_resync = 0;

				if (wsm_id & 0x0400) {
					int rc = wsm_release_tx_buffer(hw_priv, 1);
					if (WARN_ON(rc < 0))
						goto atbm_bh_break;
					else if (rc > 0)
						tx = 1;
				}
				/* atbm_wsm_rx takes care on SKB livetime */
				if (WARN_ON(atbm_rx_tasklet(hw_priv, wsm_id, wsm,
							  &skb_rx))){
					goto atbm_bh_break;
				}
				if (skb_rx) {
					atbm_dev_kfree_skb(skb_rx);
					skb_rx = NULL;
				}
			}
			
		}
tx:
		BUG_ON(hw_priv->hw_bufs_used > hw_priv->wsm_caps.numInpChBufs);
		tx_burst = (hw_priv->wsm_caps.numInpChBufs - hw_priv->hw_bufs_used);
		tx_allowed = tx_burst > 0;
		if (tx && tx_allowed) {
			#ifdef  ATBM_WSM_SDIO_TX_MULT
			#define WSM_SDIO_TX_MULT_BLOCK_SIZE	(6*SDIO_BLOCK_SIZE)
			#else
			#define WSM_SDIO_TX_MULT_BLOCK_SIZE	(SDIO_BLOCK_SIZE)
			#endif
			u32 tx_len=0;
			u32 putLen=0;
			u8 *data;
			int ret=0;
			int txMutiFrameCount=0;
			bool send_cmd = false;
#if (PROJ_TYPE>=ARES_A)
			u32 wsm_flag_u32 = 0;
			u16 wsm_len_u16[2];
			u16 wsm_len_sum;
#endif	//	(PROJ_TYPE==ARES_A)	
			if (hw_priv->device_can_sleep) {
				ret = atbm_device_wakeup(hw_priv);
				if (WARN_ON(ret < 0))
					break;
				else if (ret)
					hw_priv->device_can_sleep = false;
				else {
					/* Wait for "awake" interrupt */
					pending_tx = tx;
					continue;
				}
			}
			memset(&AggrBuffer[0],0,SDIO_TX_MAXLEN);
			do {
				
				tx_burst = (hw_priv->wsm_caps.numInpChBufs - hw_priv->hw_bufs_used); 

				if(tx_burst==0){
					if(txMutiFrameCount >0)
						break;
					else
						goto tx_end;
						
				}
				ret = wsm_get_tx(hw_priv, &data, &tx_len, &tx_burst,&vif_selected);
				//printk(KERN_ERR "tx_burst1 %d\n",tx_burst);
				if (ret <= 0) {
					//tx_burst=0;
					//printk(KERN_ERR "tx_burst2 %d\n",tx_burst);
					if(txMutiFrameCount >0)
						break;
					else
						goto tx_end;
				}
				else{
					txMutiFrameCount++;
					wsm_tx = (struct wsm_hdr_tx *)data;
					BUG_ON(tx_len < sizeof(*wsm_tx));
					BUG_ON(__le32_to_cpu(wsm_tx->len) != tx_len);
#if (PROJ_TYPE>=ARES_A)
					wsm_flag_u32 = (tx_len) & 0xffff;
					wsm_len_u16[0] = wsm_flag_u32 & 0xff;
					wsm_len_u16[1] = (wsm_flag_u32 >> 8)& 0xff;
					wsm_len_sum = wsm_len_u16[0] + wsm_len_u16[1];
					if (wsm_len_sum & BIT(8))
					{
						wsm_flag_u32 |= ((wsm_len_sum + 1) & 0xff) << 24;
					}else
					{
						wsm_flag_u32 |= (wsm_len_sum & 0xff) << 24;
					}
					wsm_tx->flag=__cpu_to_le32(wsm_flag_u32);
					//printk("wsm_tx,flag=0x%x\n", wsm_tx->flag);
#endif //(PROJ_TYPE==ARES_A)
					if (tx_len <= 8)
						tx_len = 16;

					if (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) ) {
						tx_len -= (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) );
						tx_len += WSM_SDIO_TX_MULT_BLOCK_SIZE;
					}

					/* Check if not exceeding CW1200
					capabilities */
					if (WARN_ON_ONCE(tx_len > EFFECTIVE_BUF_SIZE)) {
						printk(KERN_DEBUG "Write aligned len:"
						" %d\n", tx_len);
					}
#if (PROJ_TYPE<ARES_A)					
					wsm_tx->flag=(((tx_len/SDIO_BLOCK_SIZE)&0xff)-1);
#endif //(PROJ_TYPE==ARES_A)
					wsm_tx->id &= __cpu_to_le32(~WSM_TX_SEQ(WSM_TX_SEQ_MAX));
					wsm_tx->id |= cpu_to_le32(WSM_TX_SEQ(hw_priv->wsm_tx_seq));
					#ifdef  ATBM_WSM_SDIO_TX_MULT
					wsm_tx->tx_len = tx_len;
					wsm_tx->tx_id = wsm_tx->id;
					#endif
					wsm_alloc_tx_buffer(hw_priv);
					memcpy(&AggrBuffer[putLen],data,wsm_tx->len);
					putLen+=tx_len;
					hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq + 1) & WSM_TX_SEQ_MAX;

					if (vif_selected != -1) {
						hw_priv->hw_bufs_used_vif[vif_selected]++;
					}
					if(wsm_txed(hw_priv, data)){
						send_cmd = true;
						break;
					}else{
						hw_priv->wsm_txframe_num++;
					}
					if (putLen+hw_priv->wsm_caps.sizeInpChBuf>SDIO_TX_MAXLEN)
					{
						break;
					}
				}
			}while(0);
			hw_priv->buf_id_offset=txMutiFrameCount;
			atomic_add(1, &hw_priv->bh_tx);

			if (WARN_ON(atbm_data_write(hw_priv,&AggrBuffer[0], putLen))) {
				wsm_release_tx_buffer(hw_priv,txMutiFrameCount);
				break;
			}
			if (tx_burst > 1) {
				send_cmd = false;
				atbm_debug_tx_burst(hw_priv);
				++rx_burst;
				//printk(KERN_ERR "tx_burst %d\n",tx_burst);
				goto tx;
			}
		}
			
tx_end:
		ATBM_BH_CHECK_MISS_DATA(hw_priv,rx,rx,atbm_bh_break);
	}
atbm_bh_break:
	atbm_bh_wake_unlock(hw_priv);

	if (skb_rx) {
		ATBM_OS_WAKE_UNLOCK(hw_priv);
		atbm_put_skb(hw_priv, skb_rx);
		skb_rx = NULL;
	}
	atomic_set(&hw_priv->bh_term,term);
	down(&hw_priv->sdio_rx_process_lock);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	atbm_rx_bh_flush(hw_priv);
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	up(&hw_priv->sdio_rx_process_lock);
	if (!term) {
		int loop = 3;
		atbm_dbg(ATBM_APOLLO_DBG_ERROR, "[BH] Fatal error, exitting.\n");
		hw_priv->bh_error = 1;
		while(loop-->0){
			atbm_monitor_pc(hw_priv);
			msleep(10);
		}
#ifdef RESET_CHANGE
atbm_rest:
		WARN_ON((atomic_add_return(1, &hw_priv->reset_flag) != 1));
		WARN_ON((atomic_add_return(1, &hw_priv->fw_reloading) != 1));
		atomic_xchg(&hw_priv->bh_rx, 0);
		atomic_xchg(&hw_priv->bh_tx, 0);
		term = atomic_xchg(&hw_priv->bh_term, 0);
		atomic_xchg(&hw_priv->bh_suspend, 0);
		atomic_add_return(1,&hw_priv->reset_conter);
		/*set reset flag = 1,  not allow send packet to lmac*/
		hw_priv->init_done = 0;
		hw_priv->bh_error = 0;
		rx =0;
		tx=0;
		suspend =0;
		if(atbm_wtd_term(hw_priv))
		{
			atomic_set(&hw_priv->bh_term,term);
			goto bh_stop;
		}
		term=0;
		if(atomic_read(&hw_priv->reset_conter)>3)
		{
			goto bh_stop;
		}
		hw_priv->sbus_ops->wtd_wakeup(hw_priv->sbus_priv);
		printk("wtd_wakeup--\n");
wait_fw_loaded:
		status = wait_event_interruptible_timeout(hw_priv->bh_wq, hw_priv->init_done|hw_priv->bh_error,
	       50*HZ);

		if((!status)||(hw_priv->bh_error))
		{
			printk("fw loaded err\n");
			goto atbm_rest;
		}
		else if(status&&hw_priv->init_done)
		{
			printk("fw has been loaded,reset\n");
			goto reset;
		}

		goto wait_fw_loaded;
		
bh_stop:
		printk("sorry!!,we can not reload the fw !!!\n");
#else
		hw_priv->sbus_ops->wtd_wakeup(hw_priv->sbus_priv);
#endif
		atbm_hw_vif_read_lock(&hw_priv->vif_list_lock);
		atbm_for_each_vif_safe(hw_priv, priv, i) {
			if (!priv)
				continue;
//			ieee80211_driver_hang_notify(priv->vif, GFP_KERNEL);
		}
		atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
		#ifdef CONFIG_PM
		atbm_pm_stay_awake(&hw_priv->pm_state, 3*HZ);
		#endif
		/* TODO: schedule_work(recovery) */
#ifndef HAS_PUT_TASK_STRUCT
		/* The only reason of having this stupid code here is
		 * that __put_task_struct is not exported by kernel. */
		for (;;) {
			int status = wait_event_interruptible(hw_priv->bh_wq, ({
				term = atomic_xchg(&hw_priv->bh_term, 0);
				(term);
				}));

			if (status || term)
				break;
		}
#endif
	}
	printk("atbm_wifi_BH_thread stop ++\n");
	atbm_kfree(AggrBuffer);
	/*
	add this code just because 'linux kernel' need kthread not exit ,
	before kthread_stop func call,
	*/
	if(term)
	{
		//clear pendding cmd
		if(!mutex_trylock(&hw_priv->wsm_cmd_mux))
		{
			spin_lock_bh(&hw_priv->wsm_cmd.lock);
			if(hw_priv->wsm_cmd.cmd != 0xFFFF)
			{
				hw_priv->wsm_cmd.ret = -1;
				hw_priv->wsm_cmd.done = 1;
				spin_unlock_bh(&hw_priv->wsm_cmd.lock);
				printk(KERN_DEBUG "cancle current pendding cmd,release wsm_cmd.lock\n");
				wake_up(&hw_priv->wsm_cmd_wq);
				msleep(2);
				spin_lock_bh(&hw_priv->wsm_cmd.lock);
			}
			spin_unlock_bh(&hw_priv->wsm_cmd.lock);
		}
		else
		{
			mutex_unlock(&hw_priv->wsm_cmd_mux);
		}

		/*
		*cancle the current scanning process
		*/
		mutex_lock(&hw_priv->conf_mutex);
		if(atomic_read(&hw_priv->scan.in_progress))
		{
			struct atbm_vif *scan_priv = ABwifi_hwpriv_to_vifpriv(hw_priv,
						hw_priv->scan.if_id);
			bool scanto_running = false;
			atbm_priv_vif_list_read_unlock(&scan_priv->vif_lock);
			mutex_unlock(&hw_priv->conf_mutex);
			scanto_running = atbm_cancle_delayed_work(&hw_priv->scan.timeout,true);
			mutex_lock(&hw_priv->conf_mutex);
			if(scanto_running>0)
			{
				hw_priv->scan.curr = hw_priv->scan.end;
				mutex_unlock(&hw_priv->conf_mutex);
				atbm_scan_timeout(&hw_priv->scan.timeout.work);
				mutex_lock(&hw_priv->conf_mutex);
			}
		}
		mutex_unlock(&hw_priv->conf_mutex);
		{
			u8 i = 0;
			//cancel pendding work
			#define ATBM_CANCEL_PENDDING_WORK(work,work_func)			\
				do{														\
					if(atbm_cancle_queue_work(work,true)==true)			\
					{													\
						work_func(work);								\
					}													\
				}														\
				while(0)
					
			if(atbm_cancle_delayed_work(&hw_priv->scan.probe_work,true))
				atbm_probe_work(&hw_priv->scan.probe_work.work);
			if(atbm_cancle_delayed_work(&hw_priv->rem_chan_timeout,true))
				atbm_rem_chan_timeout(&hw_priv->rem_chan_timeout.work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.work,atbm_scan_work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->event_handler,atbm_event_handler);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->ba_work,atbm_ba_work);
			#ifdef ATBM_SUPPORT_WIDTH_40M
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->get_cca_work,atbm_get_cca_work);
			del_timer_sync(&hw_priv->chantype_timer);
			#endif
			#ifdef ATBM_SUPPORT_SMARTCONFIG
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.smartwork,atbm_smart_scan_work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.smartsetChanwork,atbm_smart_setchan_work);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->scan.smartstopwork,atbm_smart_stop_work);
			del_timer_sync(&hw_priv->smartconfig_expire_timer);
			#endif
			del_timer_sync(&hw_priv->ba_timer);
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->tx_policy_upload_work,tx_policy_upload_work);
#ifdef ATBM_SDIO_PATCH
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->wsm_sync_channl, wsm_sync_channl);
#else
			ATBM_CANCEL_PENDDING_WORK(&hw_priv->wsm_sync_channl, wsm_sync_channl_reset);
#endif
			
			atbm_for_each_vif(hw_priv, priv, i) {
				if(priv == NULL)
					continue;
				ATBM_CANCEL_PENDDING_WORK(&priv->wep_key_work,atbm_wep_key_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->join_work,atbm_join_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->unjoin_work,atbm_unjoin_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->offchannel_work,atbm_offchannel_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->link_id_work,atbm_link_id_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->tx_failure_work,atbm_tx_failure_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->set_tim_work, atbm_set_tim_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->multicast_start_work,atbm_multicast_start_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->multicast_stop_work, atbm_multicast_stop_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->linkid_reset_work, atbm_link_id_reset);
				ATBM_CANCEL_PENDDING_WORK(&priv->update_filtering_work, atbm_update_filtering_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->set_beacon_wakeup_period_work,
											atbm_set_beacon_wakeup_period_work);
				ATBM_CANCEL_PENDDING_WORK(&priv->ht_info_update_work, atbm_ht_info_update_work);
				#ifdef ATBM_SUPPORT_WIDTH_40M
				//ATBM_CANCEL_PENDDING_WORK(&priv->chantype_change_work, atbm_channel_type_change_work);
				if(atbm_cancle_delayed_work(&priv->chantype_change_work,true))
					atbm_channel_type_change_work(&priv->chantype_change_work.work);
				#endif
				
				atbm_cancle_delayed_work(&priv->dhcp_retry_work,true);					
				if(atbm_cancle_delayed_work(&priv->bss_loss_work,true))
					atbm_bss_loss_work(&priv->bss_loss_work.work);
				if(atbm_cancle_delayed_work(&priv->connection_loss_work,true))
					atbm_connection_loss_work(&priv->connection_loss_work.work);
				if(atbm_cancle_delayed_work(&priv->set_cts_work,true))
					atbm_set_cts_work(&priv->set_cts_work.work);
				if(atbm_cancle_delayed_work(&priv->link_id_gc_work,true))
					atbm_link_id_gc_work(&priv->link_id_gc_work.work);
				if(atbm_cancle_delayed_work(&priv->pending_offchanneltx_work,true))
					atbm_pending_offchanneltx_work(&priv->pending_offchanneltx_work.work);
				if(atbm_cancle_delayed_work(&priv->join_timeout,true))
					atbm_join_timeout(&priv->join_timeout.work);	
				del_timer_sync(&priv->mcast_timeout);
				
			}
		}
	}
	while(term){
		if(kthread_should_stop()){
			break;
		}
		schedule_timeout_uninterruptible(msecs_to_jiffies(100));
	}
	printk("atbm_wifi_BH_thread stop --\n");
	return 0;
}
#endif
