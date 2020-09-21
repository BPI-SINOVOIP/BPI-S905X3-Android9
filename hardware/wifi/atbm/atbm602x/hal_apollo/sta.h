/*
 * Mac80211 STA interface for altobeam APOLLO mac80211 drivers
 * *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 *Based on apollo code
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef STA_H_INCLUDED
#define STA_H_INCLUDED

/* ******************************************************************** */
/* mac80211 API								*/

int atbm_start(struct ieee80211_hw *dev);
void atbm_stop(struct ieee80211_hw *dev);
int atbm_add_interface(struct ieee80211_hw *dev,
			 struct ieee80211_vif *vif);
void atbm_remove_interface(struct ieee80211_hw *dev,
			     struct ieee80211_vif *vif);
int atbm_change_interface(struct ieee80211_hw *dev,
				struct ieee80211_vif *vif,
				enum nl80211_iftype new_type,
				bool p2p);

int atbm_config(struct ieee80211_hw *dev, u32 changed);
int atbm_change_interface(struct ieee80211_hw *dev,
                                struct ieee80211_vif *vif,
                                enum nl80211_iftype new_type,
                                bool p2p);
void atbm_configure_filter(struct ieee80211_hw *dev,
			     struct ieee80211_vif *vif,
			     unsigned int changed_flags,
			     unsigned int *total_flags,
			     u64 multicast);
int atbm_conf_tx(struct ieee80211_hw *dev, struct ieee80211_vif *vif,
		u16 queue, const struct ieee80211_tx_queue_params *params);
int atbm_get_stats(struct ieee80211_hw *dev,
		     struct ieee80211_low_level_stats *stats);
/* Not more a part of interface?
int atbm_get_tx_stats(struct ieee80211_hw *dev,
			struct ieee80211_tx_queue_stats *stats);
*/
int atbm_set_key(struct ieee80211_hw *dev, enum set_key_cmd cmd,
		   struct ieee80211_vif *vif, struct ieee80211_sta *sta,
		   struct ieee80211_key_conf *key);

int atbm_set_rts_threshold(struct ieee80211_hw *hw,
		struct ieee80211_vif *vif, u32 value);

void atbm_flush(struct ieee80211_hw *hw,
		  struct ieee80211_vif *vif,
		  bool drop);

int atbm_remain_on_channel(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif,
				struct ieee80211_channel *chan,
				enum nl80211_channel_type channel_type,
				int duration, u64 cookie);

int atbm_cancel_remain_on_channel(struct ieee80211_hw *hw);

int atbm_set_arpreply(struct ieee80211_hw *hw, struct ieee80211_vif *vif);

u64 atbm_prepare_multicast(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif,
			     struct netdev_hw_addr_list *mc_list);

int atbm_set_pm(struct atbm_vif *priv, const struct wsm_set_pm *arg);
void atbm_dhcp_retry_work(struct work_struct *work);

void atbm_set_data_filter(struct ieee80211_hw *hw,
			   struct ieee80211_vif *vif,
			   void *data,
			   int len);
/* ******************************************************************** */
/* WSM callbacks							*/

/* void atbm_set_pm_complete_cb(struct atbm_common *hw_priv,
	struct wsm_set_pm_complete *arg); */
void atbm_channel_switch_cb(struct atbm_common *hw_priv);

/* ******************************************************************** */
/* WSM events								*/

void atbm_free_event_queue(struct atbm_common *hw_priv);
void atbm_event_handler(struct work_struct *work);
void atbm_bss_loss_work(struct work_struct *work);
void atbm_connection_loss_work(struct work_struct *work);
void atbm_keep_alive_work(struct work_struct *work);
void atbm_tx_failure_work(struct work_struct *work);

/* ******************************************************************** */
/* Internal API								*/
void atbm_pending_offchanneltx_work(struct work_struct *work);
int atbm_setup_mac(struct atbm_common *hw_priv);
void atbm_join_work(struct work_struct *work);
void atbm_join_timeout(struct work_struct *work);
void atbm_unjoin_work(struct work_struct *work);
void atbm_offchannel_work(struct work_struct *work);
void atbm_wep_key_work(struct work_struct *work);
void atbm_update_filtering(struct atbm_vif *priv);
void atbm_update_filtering_work(struct work_struct *work);
int __atbm_flush(struct atbm_common *hw_priv, bool drop, int if_id);
void atbm_set_beacon_wakeup_period_work(struct work_struct *work);
int atbm_enable_listening(struct atbm_vif *priv,
			struct ieee80211_channel *chan);
int atbm_disable_listening(struct atbm_vif *priv);
int atbm_set_uapsd_param(struct atbm_vif *priv,
				const struct wsm_edca_params *arg);
void atbm_ba_work(struct work_struct *work);
void atbm_ba_timer(unsigned long arg);
const u8 *atbm_get_ie(u8 *start, size_t len, u8 ie);
int atbm_vif_setup(struct atbm_vif *priv);
void atbm_vif_setup_params(struct atbm_vif *priv);
int atbm_setup_mac_pvif(struct atbm_vif *priv);
void atbm_iterate_vifs(void *data, u8 *mac,
			 struct ieee80211_vif *vif);
void atbm_rem_chan_timeout(struct work_struct *work);
int atbm_set_macaddrfilter(struct atbm_common *hw_priv, struct atbm_vif *priv, u8 *data);

#ifdef ROAM_OFFLOAD
int atbm_testmode_event(struct wiphy *wiphy, const u32 msg_id,
				const void *data, int len, gfp_t gfp);
#endif /*ROAM_OFFLOAD*/
#ifdef IPV6_FILTERING
int atbm_set_na(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif);
#endif /*IPV6_FILTERING*/
#ifdef CONFIG_NL80211_TESTMODE

int atbm_altmtest_cmd(struct ieee80211_hw *hw, void *data, int len);
#endif
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
void atbm_device_power_calc(struct atbm_common *priv,
			      s16 max_output_power, s16 fe_cor, u32 band);
int atbm_testmode_cmd(struct ieee80211_hw *hw, void *data, int len);
int atbm_tesmode_event(struct wiphy *wiphy, const u32 msg_id,
			 const void *data, int len, gfp_t gfp);
int atbm_get_tx_power_range(struct ieee80211_hw *hw);
int atbm_get_tx_power_level(struct ieee80211_hw *hw);

#endif /* CONFIG_ATBM_APOLLO_TESTMODE */
#endif
