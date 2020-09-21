/*
 * Datapath implementation for altobeam APOLLO mac80211 drivers
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

#include <net/atbm_mac80211.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <net/ip.h>

#include "apollo.h"
#include "wsm.h"
#include "bh.h"
#include "ap.h"
#include "debug.h"
#include "sta.h"
#include "sbus.h"
#include "atbm_p2p.h"
#include "mac80211/ieee80211_i.h"


#if defined(CONFIG_ATBM_APOLLO_TX_POLICY_DEBUG)
#define tx_policy_printk(...) printk(__VA_ARGS__)
#else
#define tx_policy_printk(...)
#endif

#define ATBM_APOLLO_INVALID_RATE_ID (0xFF)

#ifdef CONFIG_ATBM_APOLLO_TESTMODE
#include "atbm_testmode.h"
#endif /* CONFIG_ATBM_APOLLO_TESTMODE */
static const struct ieee80211_rate *
atbm_get_tx_rate(const struct atbm_common *hw_priv,
		   const struct ieee80211_tx_rate *rate);
#ifdef ATBM_11W_TEST
static u8 broadcast_addr[6] = {0x01,0x00,0x02,0x03,0x04,0xff};
#endif
/* ******************************************************************** */
/* TX policy cache implementation					*/
#define IS_BOOTP_PORT(src_port,des_port) ((((src_port) == 67)&&((des_port) == 68)) || \
										   (((src_port) == 68)&&((des_port) == 67)))



#if 1

void dhcp_hexdump(char *prefix, u8 *data, int len)
{
	int i;

	for (i = 0; i < len; i++) {
	   if((i % 16)==0)
		   printk("\n");
	   printk("%02x ", data[i]);

	}
	printk("\n");
}

//extern int g_connetting;
static int tx_dhcp_retry_cnt=0;


void atbm_tx_udp(struct atbm_vif *priv,struct sk_buff *skb)
{
	//struct ieee80211_hdr *frame = (struct ieee80211_hdr *)skb->data;
	if (atomic_read(&priv->enabled) == 0)
		return;
	if (skb->protocol == htons(ETH_P_IP))
	{
		struct iphdr *iph; //= ip_hdr(skb);
		struct udphdr *udph;
		
		//u8 mac_hdr_len=ieee80211_hdrlen(hdr->frame_control);
		iph = ip_hdr(skb);
		if (iph->protocol==IPPROTO_UDP){
			/* Send all udp frames on VO. Accordingly set TID to 7. */
			//atbm_skb_set_queue_mapping(skb, IEEE80211_AC_VI);
			//skb->priority = 5;
		}
		udph = (struct udphdr *)((u8*)iph+(iph->ihl)*4);
		if(IS_BOOTP_PORT(ntohs(udph->source),ntohs(udph->dest)))
		{
			tx_dhcp_retry_cnt = 0;
			//printk("start tx_udp %d,dhcp_retry_skb %p\n",SDATA_IS_CONNECTTING(vif_to_sdata(priv->vif)),priv->dhcp_retry_skb);
			spin_lock_bh(&priv->dhcp_retry_spinlock);
			if(priv->dhcp_retry_skb){
				atbm_kfree_skb(priv->dhcp_retry_skb);
				priv->dhcp_retry_skb=NULL;
			}
			//if(g_connetting){
			if(SDATA_IS_CONNECTTING(vif_to_sdata(priv->vif))){
			    priv->dhcp_retry_skb = atbm_skb_copy(skb, GFP_ATOMIC);
				if(priv->dhcp_retry_skb == NULL){
					printk(KERN_ERR"%s:no mem\n",__func__);
					spin_unlock_bh(&priv->dhcp_retry_spinlock);
					return;
				}
				atbm_hw_priv_queue_delayed_work(priv->hw_priv,&priv->dhcp_retry_work, HZ/2);
				//memcpy(priv->dhcp_retry_skb->data,skb->data,skb->len);
				memcpy(IEEE80211_SKB_CB(priv->dhcp_retry_skb),IEEE80211_SKB_CB(skb),sizeof(struct ieee80211_tx_info));
				printk("start atbm_hw_priv_queue_delayed_work\n");
			}
			else {
				atbm_cancle_delayed_work(&priv->dhcp_retry_work,false);
			}
			spin_unlock_bh(&priv->dhcp_retry_spinlock);
			//dhcp_hexdump("Tx Dhcp ",(u8*)skb->data,skb->len);
			
		}
	}
}


int atbm_is_dhcp_frame(struct atbm_vif *priv,struct sk_buff *skb)
{
	//struct ieee80211_hdr *frame = (struct ieee80211_hdr *)skb->data;
	
	if (skb->protocol == htons(ETH_P_IP))	{
		struct iphdr *iph; //= ip_hdr(skb);
		struct udphdr *udph;
		
		iph = ip_hdr(skb);
		udph = (struct udphdr *)((u8*)iph+(iph->ihl)*4);
		if(IS_BOOTP_PORT(ntohs(udph->source),ntohs(udph->dest)))		{
			return 1;			
		}
	}

	return 0;
}

#else
void atbm_tx_udp(struct sk_buff *skb)
{
	const struct iphdr *ip;	
	u16 ether_type;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	//frame_hexdump("This is Rx hdr",(u8*)skb->data,300);
	u8 mac_hdr_len=ieee80211_hdrlen(hdr->frame_control);
	if (hdr->frame_control&__cpu_to_le32(IEEE80211_FCTL_PROTECTED))
	{
		mac_hdr_len+=8;
	}
	//printk("mac_hdr_len=%x\n",mac_hdr_len);
	ip =(struct iphdr *)((u8*)skb->data+mac_hdr_len +6+2);
	ether_type=*(u16*)((u8*)skb->data+mac_hdr_len+6);
	//printk("ether_type=%x:%x\n",ether_type,htons(ETH_P_IP));
	if (ether_type==htons(ETH_P_IP))
	{
	//	printk("protocol=%x\n",ip->protocol);
		if (IPPROTO_UDP==ip->protocol){
			struct udphdr *udph=(struct udphdr *)((u8*)ip+(ip->ihl<<2));
			if(IS_BOOTP_PORT(ntohs(udph->source),ntohs(udph->dest)))
			{
				//struct bootp_pkt *b;
				//int bootp_len=236;
				//int dhcp_magic_len=4;
				b=(struct bootp_pkt *)((u8*)(udph+8));
				printk("dhcp  tx msgId=%x\n",(u32*)(b+(bootp_len+dhcp_magic_len));
				
				if ((u8*)(udph+226+mac_hdr_len)==0x2){
					printk("******Rx dhcp Offer(%l) ms****\n",jiffies_to_msecs(jiffies));
				}else if ((u8*)(udph+226+mac_hdr_len)==0x5){
					printk("******Rx dhcp Ack(%l) ms****\n",jiffies_to_msecs(jiffies));

				}
			}
		}
	}
}
#endif


void atbm_rx_udp(struct atbm_vif *priv,struct sk_buff *skb)
{
	const struct iphdr *ip;	
	u16 ether_type;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	u8 mac_hdr_len=ieee80211_hdrlen(hdr->frame_control);

	if(!ieee80211_is_data(hdr->frame_control))
		return;
	if(ieee80211_is_nullfunc(hdr->frame_control))
		return;
	if(ieee80211_is_qos_nullfunc(hdr->frame_control))
		return;
	
	if (hdr->frame_control&__cpu_to_le32(IEEE80211_FCTL_PROTECTED))
	{
		mac_hdr_len+=8;
	}
	//printk("mac_hdr_len=%x\n",mac_hdr_len);
	ip =(struct iphdr *)((u8*)skb->data+mac_hdr_len +6+2);
	ether_type=*(u16*)((u8*)skb->data+mac_hdr_len+6);
	//printk("ether_type=%x:%x\n",ether_type,htons(ETH_P_IP));
	if (ether_type==htons(ETH_P_IP))
	{
	//	printk("protocol=%x\n",ip->protocol);
		if (IPPROTO_UDP==ip->protocol){
			struct udphdr *udph=(struct udphdr *)((u8*)ip+(ip->ihl<<2));
			if(IS_BOOTP_PORT(ntohs(udph->source),ntohs(udph->dest)))
			{
				printk("cancel dhcp_retry_work\n");
				atbm_cancle_delayed_work(&priv->dhcp_retry_work,false);
				
				//dhcp_hexdump("Rx Dhcp ",(u8*)udph+236+8,16);
			}
		}
	}
}
void __atbm_tx(struct ieee80211_hw *dev, struct sk_buff *skb);

void atbm_dhcp_retry_work(struct work_struct *work)
{	
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, dhcp_retry_work.work);
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	//int timeout; /* in beacons */
	struct sk_buff *skb;

	if(atbm_bh_is_term(hw_priv)){
		return;
	}
	printk("atbm_dhcp_retry_work priv->dhcp_retry_skb %p %d\n",priv->dhcp_retry_skb,SDATA_IS_CONNECTTING(vif_to_sdata(priv->vif)));
	spin_lock_bh(&priv->dhcp_retry_spinlock);
	if((priv->dhcp_retry_skb != NULL) &&(SDATA_IS_CONNECTTING(vif_to_sdata(priv->vif)))){
		//skb = priv->dhcp_retry_skb;
		//priv->dhcp_retry_skb = NULL;
		
		skb = atbm_skb_copy(priv->dhcp_retry_skb , GFP_ATOMIC);
		if(skb == NULL){
			printk(KERN_ERR "%s:no mem\n",__func__);
			spin_unlock_bh(&priv->dhcp_retry_spinlock);
			atbm_hw_priv_queue_delayed_work(priv->hw_priv,&priv->dhcp_retry_work, HZ/50);
			return;
		}
		memcpy(IEEE80211_SKB_CB(skb),IEEE80211_SKB_CB(priv->dhcp_retry_skb),sizeof(struct ieee80211_tx_info));
						
		spin_unlock_bh(&priv->dhcp_retry_spinlock);
		printk("atbm_dhcp_retry_work __atbm_tx\n");
		local_bh_disable();
		__atbm_tx(priv->hw,skb);
		local_bh_enable();
		spin_lock_bh(&priv->dhcp_retry_spinlock);
		tx_dhcp_retry_cnt++;
		printk("restart dhcp_retry_work %d\n",tx_dhcp_retry_cnt);
		if(tx_dhcp_retry_cnt <3){
			atbm_hw_priv_queue_delayed_work(priv->hw_priv,&priv->dhcp_retry_work, HZ/2);
		}
		else {
			atbm_kfree_skb(priv->dhcp_retry_skb);
			priv->dhcp_retry_skb = NULL;
		}
	}
	else {
		atbm_kfree_skb(priv->dhcp_retry_skb);
		priv->dhcp_retry_skb = NULL;
	}
	spin_unlock_bh(&priv->dhcp_retry_spinlock);
}

static void tx_policy_dump(struct tx_policy *policy)
{
	tx_policy_printk(KERN_DEBUG "[TX policy] "
		"%.1X%.1X%.1X%.1X%.1X%.1X%.1X%.1X"
		"%.1X%.1X%.1X%.1X%.1X%.1X%.1X%.1X"
		"%.1X%.1X%.1X%.1X%.1X%.1X%.1X%.1X: %d\n",
		policy->raw[0] & 0x0F,  policy->raw[0] >> 4,
		policy->raw[1] & 0x0F,  policy->raw[1] >> 4,
		policy->raw[2] & 0x0F,  policy->raw[2] >> 4,
		policy->raw[3] & 0x0F,  policy->raw[3] >> 4,
		policy->raw[4] & 0x0F,  policy->raw[4] >> 4,
		policy->raw[5] & 0x0F,  policy->raw[5] >> 4,
		policy->raw[6] & 0x0F,  policy->raw[6] >> 4,
		policy->raw[7] & 0x0F,  policy->raw[7] >> 4,
		policy->raw[8] & 0x0F,  policy->raw[8] >> 4,
		policy->raw[9] & 0x0F,  policy->raw[9] >> 4,
		policy->raw[10] & 0x0F,  policy->raw[10] >> 4,
		policy->raw[11] & 0x0F,  policy->raw[11] >> 4,
		policy->defined);
}

static void atbm_check_go_neg_conf_success(struct atbm_common *hw_priv,
						u8 *action)
{
	if (action[2] == 0x50 && action[3] == 0x6F && action[4] == 0x9A &&
		action[5] == 0x09 && action[6] == 0x02) {
		if(action[17] == 0) {
			hw_priv->is_go_thru_go_neg = true;
		}
		else {
			hw_priv->is_go_thru_go_neg = false;
		}
	}
}

static void atbm_check_prov_desc_req(struct atbm_common *hw_priv,
                                                u8 *action)
{
	if (action[2] == 0x50 && action[3] == 0x6F && action[4] == 0x9A &&
                action[5] == 0x09 && action[6] == 0x07) {
                        hw_priv->is_go_thru_go_neg = false;
        }
}

static void tx_policy_build(const struct atbm_common *hw_priv,
	/* [out] */ struct tx_policy *policy,
	struct ieee80211_tx_rate *rates, size_t count)
{
	int i, j;
	unsigned limit = hw_priv->short_frame_max_tx_count;
	unsigned total = 0;
	BUG_ON(rates[0].idx < 0);
	memset(policy, 0, sizeof(*policy));

	/* minstrel is buggy a little bit, so distille
	 * incoming rates first. */

	/* Sort rates in descending order. */
	for (i = 1; i < count; ++i) {
		if (rates[i].idx < 0) {
			count = i;
			break;
		}
		if (rates[i].idx > rates[i - 1].idx) {
			struct ieee80211_tx_rate tmp = rates[i - 1];
			rates[i - 1] = rates[i];
			rates[i] = tmp;
		}
	}

	/* Eliminate duplicates. */
	total = rates[0].count;
	for (i = 0, j = 1; j < count; ++j) {
		if (rates[j].idx == rates[i].idx) {
			rates[i].count += rates[j].count;
		} else if (rates[j].idx > rates[i].idx) {
			break;
		} else {
			++i;
			if (i != j)
				rates[i] = rates[j];
		}
		total += rates[j].count;
	}
	count = i + 1;

	/* Re-fill policy trying to keep every requested rate and with
	 * respect to the global max tx retransmission count. */
	if (limit < count)
		limit = count;
	if (total > limit) {
		for (i = 0; i < count; ++i) {
			int left = count - i - 1;
			if (rates[i].count > limit - left)
				rates[i].count = limit - left;
			limit -= rates[i].count;
		}
	}

	/* HACK!!! Device has problems (at least) switching from
	 * 54Mbps CTS to 1Mbps. This switch takes enormous amount
	 * of time (100-200 ms), leading to valuable throughput drop.
	 * As a workaround, additional g-rates are injected to the
	 * policy.
	 */
	if (count == 2 && !(rates[0].flags & IEEE80211_TX_RC_MCS) &&
			rates[0].idx > 4 && rates[0].count > 2 &&
			rates[1].idx < 2) {
		/* ">> 1" is an equivalent of "/ 2", but faster */
		int mid_rate = (rates[0].idx + 4) >> 1;

		/* Decrease number of retries for the initial rate */
		rates[0].count -= 2;

		if (mid_rate != 4) {
			/* Keep fallback rate at 1Mbps. */
			rates[3] = rates[1];

			/* Inject 1 transmission on lowest g-rate */
			rates[2].idx = 4;
			rates[2].count = 1;
			rates[2].flags = rates[1].flags;

			/* Inject 1 transmission on mid-rate */
			rates[1].idx = mid_rate;
			rates[1].count = 1;

			/* Fallback to 1 Mbps is a really bad thing,
			 * so let's try to increase probability of
			 * successful transmission on the lowest g rate
			 * even more */
			if (rates[0].count >= 3) {
				--rates[0].count;
				++rates[2].count;
			}

			/* Adjust amount of rates defined */
			count += 2;
		} else {
			/* Keep fallback rate at 1Mbps. */
			rates[2] = rates[1];

			/* Inject 2 transmissions on lowest g-rate */
			rates[1].idx = 4;
			rates[1].count = 2;

			/* Adjust amount of rates defined */
			count += 1;
		}
	}

	policy->defined = atbm_get_tx_rate(hw_priv, &rates[0])->hw_value + 1;

	for (i = 0; i < count; ++i) {
		register unsigned rateid, off, shift, retries;

		rateid = atbm_get_tx_rate(hw_priv, &rates[i])->hw_value;
		off = rateid >> 3;		/* eq. rateid / 8 */
		shift = (rateid & 0x07) << 2;	/* eq. (rateid % 8) * 4 */

		retries = rates[i].count;
		if (unlikely(retries > 0x0F))
			rates[i].count = retries = 0x0F;
		policy->tbl[off] |= __cpu_to_le32(retries << shift);
		policy->retry_count += retries;
	}

	tx_policy_printk(KERN_DEBUG "[TX policy] Policy (%d): " \
		"%d:%d, %d:%d, %d:%d, %d:%d, %d:%d\n",
		count,
		rates[0].idx, rates[0].count,
		rates[1].idx, rates[1].count,
		rates[2].idx, rates[2].count,
		rates[3].idx, rates[3].count,
		rates[4].idx, rates[4].count);
}

static inline bool tx_policy_is_equal(const struct tx_policy *wanted,
					const struct tx_policy *cached)
{
	size_t count = wanted->defined >> 1;
	if (wanted->defined > cached->defined)
		return false;
	if (count) {
		if (memcmp(wanted->raw, cached->raw, count))
			return false;
	}
	if (wanted->defined & 1) {
		if ((wanted->raw[count] & 0x0F) != (cached->raw[count] & 0x0F))
			return false;
	}
	return true;
}

static int tx_policy_find(struct tx_policy_cache *cache,
				const struct tx_policy *wanted)
{
	/* O(n) complexity. Not so good, but there's only 8 entries in
	 * the cache.
	 * Also lru helps to reduce search time. */
	struct tx_policy_cache_entry *it;
	/* Search for policy in "used" list */
	list_for_each_entry(it, &cache->used, link) {
		if (tx_policy_is_equal(wanted, &it->policy))
			return it - cache->cache;
	}
	/* Then - in "free list" */
	list_for_each_entry(it, &cache->free, link) {
		if (tx_policy_is_equal(wanted, &it->policy))
			return it - cache->cache;
	}
	return -1;
}

static inline void tx_policy_use(struct tx_policy_cache *cache,
				 struct tx_policy_cache_entry *entry)
{
	++entry->policy.usage_count;
	list_move(&entry->link, &cache->used);
}

static inline int tx_policy_release(struct tx_policy_cache *cache,
				    struct tx_policy_cache_entry *entry)
{
	int ret = --entry->policy.usage_count;
	if (!ret)
		list_move(&entry->link, &cache->free);
	return ret;
}

/* ******************************************************************** */
/* External TX policy cache API						*/

void tx_policy_init(struct atbm_common *hw_priv)
{
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	int i;

	memset(cache, 0, sizeof(*cache));

	spin_lock_init(&cache->lock);
	INIT_LIST_HEAD(&cache->used);
	INIT_LIST_HEAD(&cache->free);

	for (i = 0; i < TX_POLICY_CACHE_SIZE; ++i)
		list_add(&cache->cache[i].link, &cache->free);
}

static int tx_policy_get(struct atbm_common *hw_priv,
		  struct ieee80211_tx_rate *rates,
		  size_t count, bool *renew)
{
	int idx;
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	struct tx_policy wanted;

	tx_policy_build(hw_priv, &wanted, rates, count);

	spin_lock_bh(&cache->lock);
	
	idx = tx_policy_find(cache, &wanted);
	if (idx >= 0) {
		tx_policy_printk(KERN_DEBUG "[TX policy] Used TX policy: %d\n",
					idx);
		*renew = false;
	}
	else if (WARN_ON_ONCE(list_empty(&cache->free))) {
		spin_unlock_bh(&cache->lock);
		return ATBM_APOLLO_INVALID_RATE_ID;
	} else {
		struct tx_policy_cache_entry *entry;
		*renew = true;
		/* If policy is not found create a new one
		 * using the oldest entry in "free" list */
		entry = list_entry(cache->free.prev,
			struct tx_policy_cache_entry, link);
		entry->policy = wanted;
		idx = entry - cache->cache;
		tx_policy_printk(KERN_DEBUG "[TX policy] New TX policy: %d\n",
					idx);
		tx_policy_dump(&entry->policy);
	}
	tx_policy_use(cache, &cache->cache[idx]);
	if (unlikely(list_empty(&cache->free))) {
		/* Lock TX queues. */
		atbm_tx_queues_lock(hw_priv);
	}
	spin_unlock_bh(&cache->lock);

	return idx;
}

static void tx_policy_put(struct atbm_common *hw_priv, int idx)
{
	int usage, locked;
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;

	spin_lock_bh(&cache->lock);
	locked = list_empty(&cache->free);
	usage = tx_policy_release(cache, &cache->cache[idx]);
	if (unlikely(locked) && !usage) {
		/* Unlock TX queues. */
		atbm_tx_queues_unlock(hw_priv);
	}
	spin_unlock_bh(&cache->lock);
}

#ifdef RESET_CHANGE
void tx_policy_put_reset(struct atbm_common *hw_priv)
{
	int i;
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	spin_lock_bh(&cache->lock);
	memset(&cache->used,0,sizeof(struct list_head));
	memset(&cache->free,0,sizeof(struct list_head));
	memset(cache->cache,0,sizeof(struct tx_policy_cache_entry)*TX_POLICY_CACHE_SIZE);
	INIT_LIST_HEAD(&cache->used);
	INIT_LIST_HEAD(&cache->free);
	for (i = 0; i < TX_POLICY_CACHE_SIZE; ++i)
	{
		list_add(&cache->cache[i].link, &cache->free);
	}
	spin_unlock_bh(&cache->lock);
}
#endif
/*
bool tx_policy_cache_full(struct atbm_common *hw_priv)
{
	bool ret;
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	spin_lock_bh(&cache->lock);
	ret = list_empty(&cache->free);
	spin_unlock_bh(&cache->lock);
	return ret;
}
*/

static int tx_policy_upload(struct atbm_common *hw_priv)
{
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	int i;
	struct wsm_set_tx_rate_retry_policy arg = {
		.hdr = {
			.numTxRatePolicies = 0,
		}
	};
	int if_id = 0;
	spin_lock_bh(&cache->lock);

	/* Upload only modified entries. */
	for (i = 0; i < TX_POLICY_CACHE_SIZE; ++i) {
		struct tx_policy *src = &cache->cache[i].policy;
		if (src->retry_count && !src->uploaded) {
			struct wsm_set_tx_rate_retry_policy_policy *dst =
				&arg.tbl[arg.hdr.numTxRatePolicies];
			dst->policyIndex = i;
			dst->shortRetryCount =
				hw_priv->short_frame_max_tx_count;
			dst->longRetryCount = hw_priv->long_frame_max_tx_count;

			/* BIT(2) - Terminate retries when Tx rate retry policy
			 *          finishes.
			 * BIT(3) - Count initial frame transmission as part of
			 *          rate retry counting but not as a retry
			 *          attempt */
			dst->policyFlags = BIT(2) | BIT(3);

			memcpy(dst->rateCountIndices, src->tbl,
					sizeof(dst->rateCountIndices));
			src->uploaded = 1;
			++arg.hdr.numTxRatePolicies;
		}
	}
	spin_unlock_bh(&cache->lock);
	atbm_debug_tx_cache_miss(hw_priv);
	tx_policy_printk(KERN_DEBUG "[TX policy] Upload %d policies\n",
				arg.hdr.numTxRatePolicies);
	/*TODO: COMBO*/
	return wsm_set_tx_rate_retry_policy(hw_priv, &arg, if_id);
}
void tx_policy_upload_work(struct work_struct *work)
{
	struct atbm_common *hw_priv =
		container_of(work, struct atbm_common, tx_policy_upload_work);
	if(atbm_bh_is_term(hw_priv))
		goto out;
	tx_policy_printk(KERN_DEBUG "[TX] TX policy upload\n");
	if(tx_policy_upload(hw_priv)){
		if(!atbm_bh_is_term(hw_priv)){
			WARN_ON(1);
		}
	}
out:
	wsm_unlock_tx(hw_priv);
	atbm_tx_queues_unlock(hw_priv);
}

/* ******************************************************************** */
/* apollo TX implementation						*/

struct atbm_txinfo {
	struct sk_buff *skb;
	unsigned queue;
	struct ieee80211_tx_info *tx_info;
	const struct ieee80211_rate *rate;
	struct ieee80211_hdr *hdr;
	u32 hdrlen;
	const u8 *da;
	struct atbm_sta_priv *sta_priv;
	struct atbm_txpriv txpriv;
};

u32 atbm_rate_mask_to_wsm(struct atbm_common *hw_priv, u32 rates)
{
	u32 ret = 0;
	int i;
	struct ieee80211_rate * bitrates =
		hw_priv->hw->wiphy->bands[hw_priv->channel->band]->bitrates;
	for (i = 0; i < 32; ++i) {
		if (rates & BIT(i))
			ret |= BIT(bitrates[i].hw_value);
	}
	return ret;
}

static const struct ieee80211_rate *
atbm_get_tx_rate(const struct atbm_common *hw_priv,
		   const struct ieee80211_tx_rate *rate)
{
	if (rate->idx < 0)
		return NULL;
	if (rate->flags & IEEE80211_TX_RC_MCS)
		return &hw_priv->mcs_rates[rate->idx];
	return &hw_priv->hw->wiphy->bands[hw_priv->channel->band]->bitrates[rate->idx];
}
#if (OLD_RATE_POLICY==0)
static const u8
atbm_get_tx_rate_hw_value(const struct atbm_common *hw_priv,
		   const struct ieee80211_tx_rate *rate)
{
	if (rate->idx < 0){
		//WARN_ON(1);
		return 0xff;
	}
	if (rate->flags & IEEE80211_TX_RC_MCS)
		return hw_priv->mcs_rates[rate->idx].hw_value;
	return hw_priv->hw->wiphy->bands[hw_priv->channel->band]->bitrates[rate->idx].hw_value;
}
#endif
static int
atbm_tx_h_calc_link_ids(struct atbm_vif *priv,
			  struct atbm_txinfo *t)
{
#ifndef P2P_MULTIVIF
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	if ((t->tx_info->flags & IEEE80211_TX_CTL_TX_OFFCHAN) ||
			(hw_priv->roc_if_id == priv->if_id))
		t->txpriv.offchannel_if_id = 2;
	else
		t->txpriv.offchannel_if_id = 0;
#endif

	if (likely(t->tx_info->control.sta && t->sta_priv->link_id))
		t->txpriv.raw_link_id =
				t->txpriv.link_id =
				t->sta_priv->link_id;
	else if (priv->mode != NL80211_IFTYPE_AP)
		t->txpriv.raw_link_id =
				t->txpriv.link_id = 0;
	else if (is_multicast_ether_addr(t->da)) {
		if (priv->enable_beacon) {
			t->txpriv.raw_link_id = 0;
			t->txpriv.link_id = priv->link_id_after_dtim;
		} else {
			t->txpriv.raw_link_id = 0;
			t->txpriv.link_id = 0;
		}
	} else {
		t->txpriv.link_id =
			atbm_find_link_id(priv, t->da);
		/* Do not assign valid link id for deauth/disassoc frame being
		transmitted to an unassociated STA */
		if (!(t->txpriv.link_id) &&
			(ieee80211_is_deauth(t->hdr->frame_control) ||
			ieee80211_is_disassoc(t->hdr->frame_control))) {
					t->txpriv.link_id = 0;
		} else {
			if (!t->txpriv.link_id)
				t->txpriv.link_id = atbm_alloc_link_id(priv, t->da);
			if (!t->txpriv.link_id) {
				wiphy_err(priv->hw->wiphy,
					"%s: No more link IDs available.\n",
					__func__);
				return -ENOENT;
			}
		}
		t->txpriv.raw_link_id = t->txpriv.link_id;
	}
	if (t->txpriv.raw_link_id)
		priv->link_id_db[t->txpriv.raw_link_id - 1].timestamp =
				jiffies;

	if (t->tx_info->control.sta &&
			(t->tx_info->control.sta->uapsd_queues & BIT(t->queue)))
		t->txpriv.link_id = priv->link_id_uapsd;
	return 0;
}

static void
atbm_tx_h_pm(struct atbm_vif *priv,
	       struct atbm_txinfo *t)
{
	if (unlikely(ieee80211_is_auth(t->hdr->frame_control))) {
		u32 mask = ~BIT(t->txpriv.raw_link_id);
		spin_lock_bh(&priv->ps_state_lock);
		priv->sta_asleep_mask &= mask;
		priv->pspoll_mask &= mask;
		spin_unlock_bh(&priv->ps_state_lock);
	}
}

static void
atbm_tx_h_calc_tid(struct atbm_vif *priv,
		     struct atbm_txinfo *t)
{
	if (ieee80211_is_data_qos(t->hdr->frame_control)) {
		u8 *qos = ieee80211_get_qos_ctl(t->hdr);
		t->txpriv.tid = qos[0] & IEEE80211_QOS_CTL_TID_MASK;
	} else if (ieee80211_is_data(t->hdr->frame_control)) {
		t->txpriv.tid = 0;
	}
}

/* IV/ICV injection. */
/* TODO: Quite unoptimal. It's better co modify mac80211
 * to reserve space for IV */
static int
atbm_tx_h_crypt(struct atbm_vif *priv,
		  struct atbm_txinfo *t)
{
	u32 iv_len;
	u32 icv_len;
	u8 *icv;
	u8 *newhdr;	
	struct ieee80211_mmie *mmie;
#ifndef ATBM_11W_TEST
	if(t->tx_info->control.hw_key)
	{
		if (t->tx_info->control.hw_key->cipher == WLAN_CIPHER_SUITE_AES_CMAC){
			printk("11w WLAN_CIPHER_SUITE_AES_CMAC\n");
			mmie = (struct ieee80211_mmie *) atbm_skb_put(t->skb, sizeof(*mmie));
			memset(mmie,0,sizeof(struct ieee80211_mmie));
			mmie->element_id = WLAN_EID_MMIE;
			mmie->length = sizeof(*mmie) - 2;
			mmie->key_id = cpu_to_le16(t->tx_info->control.hw_key->keyidx);
			t->hdr->duration_id = 0x00;
		}
	}
#else
{
	 struct atbm_ieee80211_mgmt *mgmt = (struct atbm_ieee80211_mgmt *)t->skb->data;
	 if(ieee80211_is_deauth(mgmt->frame_control))
	 {
	 	printk("da mac %pM,protec(%d)\n",mgmt->da,t->hdr->frame_control&
	     __cpu_to_le32(IEEE80211_FCTL_PROTECTED));
	 	if(!memcmp(mgmt->da,broadcast_addr,sizeof(broadcast_addr)))
	 	{
	 		extern u8 aes_mac_key_index;
	 		printk("11w WLAN_CIPHER_SUITE_AES_CMAC,key_index(%d)\n",aes_mac_key_index);
			mmie = (struct ieee80211_mmie *) atbm_skb_put(t->skb, sizeof(*mmie));
			memset(mmie,0,sizeof(struct ieee80211_mmie));
			mmie->element_id = WLAN_EID_MMIE;
			mmie->length = sizeof(*mmie) - 2;
			mmie->key_id = aes_mac_key_index;
			t->hdr->frame_control =  t->hdr->frame_control &(~ __cpu_to_le16(IEEE80211_FCTL_PROTECTED));
			t->hdr->duration_id = 0x00;
	 	}
	 }
}

#endif
	if (!t->tx_info->control.hw_key ||
	    !(t->hdr->frame_control &
	     __cpu_to_le32(IEEE80211_FCTL_PROTECTED)))
		return 0;

	iv_len = t->tx_info->control.hw_key->iv_len;
	icv_len = t->tx_info->control.hw_key->icv_len;

	if (t->tx_info->control.hw_key->cipher == WLAN_CIPHER_SUITE_TKIP)
		icv_len += 8; /* MIC */

	if ((atbm_skb_headroom(t->skb) + atbm_skb_tailroom(t->skb) <
			 iv_len + icv_len + WSM_TX_EXTRA_HEADROOM) ||
			(atbm_skb_headroom(t->skb) <
			 iv_len + WSM_TX_EXTRA_HEADROOM)) {
		wiphy_err(priv->hw->wiphy,
			"Bug: no space allocated for crypto headers.\n"
			"headroom: %d, tailroom: %d, "
			"req_headroom: %d, req_tailroom: %d\n"
			"Please fix it in atbm_get_skb().\n",
			atbm_skb_headroom(t->skb), atbm_skb_tailroom(t->skb),
			iv_len + WSM_TX_EXTRA_HEADROOM, icv_len);
		return -ENOMEM;
	} else if (atbm_skb_tailroom(t->skb) < icv_len) {
		size_t offset = icv_len - atbm_skb_tailroom(t->skb);
		u8 *p;
		if(!(t->tx_info->control.hw_key->flags & IEEE80211_KEY_FLAG_ALLOC_IV))
		{
			p = atbm_skb_push(t->skb, offset);
			memmove(p, &p[offset], t->skb->len - offset);
			atbm_skb_trim(t->skb, t->skb->len - offset);
		}
		else
		{
			wiphy_warn(priv->hw->wiphy,
			"Slowpath: tailroom is not big enough. "
			"Req: %d, got: %d.\n",
			icv_len, atbm_skb_tailroom(t->skb));
			return -ENOMEM;
		}
		
	}

	newhdr = atbm_skb_push(t->skb, iv_len);
	memmove(newhdr, newhdr + iv_len, t->hdrlen);
	t->hdr = (struct ieee80211_hdr *) newhdr;
	t->hdrlen += iv_len;
	icv = atbm_skb_put(t->skb, icv_len);

	return 0;
}

static int
atbm_tx_h_align(struct atbm_vif *priv,
		  struct atbm_txinfo *t,
		  u8 *flags)
{
	u32 offset = (unsigned long)(t->skb->data) & (u32)3;

	if (!offset)
		return 0;

	if (offset & 1) {
		wiphy_err(priv->hw->wiphy,
			"Bug: attempt to transmit a frame "
			"with wrong alignment: %d\n",
			offset);
		return -EINVAL;
	}

	if (atbm_skb_headroom(t->skb) < offset) {
		wiphy_err(priv->hw->wiphy,
			"Bug: no space allocated "
			"for DMA alignment.\n"
			"headroom: %d\n",
			atbm_skb_headroom(t->skb));
		return -ENOMEM;
	}
	atbm_skb_push(t->skb, offset);
	t->hdrlen += offset;
	t->txpriv.offset += offset;
	*flags |= WSM_TX_2BYTES_SHIFT;
	atbm_debug_tx_align(priv);
	return 0;
}

static int
atbm_tx_h_action(struct atbm_vif *priv,
		   struct atbm_txinfo *t)
{
	struct atbm_ieee80211_mgmt *mgmt =
		(struct atbm_ieee80211_mgmt *)t->hdr;

	if (ieee80211_is_action(t->hdr->frame_control) &&
			mgmt->u.action.category == WLAN_CATEGORY_BACK)
		return 1;
	else
		return 0;
}

/* Add WSM header */
static struct wsm_tx *
atbm_tx_h_wsm(struct atbm_vif *priv,
		struct atbm_txinfo *t)
{
	struct wsm_tx *wsm;

	if (atbm_skb_headroom(t->skb) < sizeof(struct wsm_tx)) {
		wiphy_err(priv->hw->wiphy,
			"Bug: no space allocated "
			"for WSM header.\n"
			"headroom: %d\n",
			atbm_skb_headroom(t->skb));
		return NULL;
	}
	wsm = (struct wsm_tx *)atbm_skb_push(t->skb, sizeof(struct wsm_tx));
	t->txpriv.offset += sizeof(struct wsm_tx);
	memset(wsm, 0, sizeof(*wsm));
#ifdef USB_BUS_BUG
	if(__cpu_to_le16(t->skb->len) < 1500)
		wsm->hdr.usb_len = 1500;//__cpu_to_le16(t->skb->len);
	else 		
		wsm->hdr.usb_len = __cpu_to_le16(t->skb->len)+4;
	wsm->hdr.usb_id = __cpu_to_le16(WSM_TRANSMIT_REQ_MSG_ID);
#endif
	wsm->hdr.len = __cpu_to_le16(t->skb->len);
	wsm->hdr.id = __cpu_to_le16(WSM_TRANSMIT_REQ_MSG_ID);
	wsm->queueId =
		(t->txpriv.raw_link_id << 2) | wsm_queue_id_to_wsm(t->queue);
	return wsm;
}

/* BT Coex specific handling */
static void
atbm_tx_h_bt(struct atbm_vif *priv,
	       struct atbm_txinfo *t,
	       struct wsm_tx *wsm)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);

	u8 priority = 0;

	if (!hw_priv->is_BT_Present)
		return;

	if (unlikely(ieee80211_is_nullfunc(t->hdr->frame_control)))
		priority = WSM_EPTA_PRIORITY_MGT;
	else if (ieee80211_is_data(t->hdr->frame_control)) {
		/* Skip LLC SNAP header (+6) */
		u8 *payload = &t->skb->data[t->hdrlen];
		u16 *ethertype = (u16 *) &payload[6];
		if (unlikely(*ethertype == __be16_to_cpu(ETH_P_PAE)))
			priority = WSM_EPTA_PRIORITY_EAPOL;
	} else if (unlikely(ieee80211_is_assoc_req(t->hdr->frame_control) ||
		ieee80211_is_reassoc_req(t->hdr->frame_control))) {
		struct atbm_ieee80211_mgmt *mgt_frame =
				(struct atbm_ieee80211_mgmt *)t->hdr;

		if (mgt_frame->u.assoc_req.listen_interval <
						priv->listen_interval) {
			txrx_printk(KERN_DEBUG
				"Modified Listen Interval to %d from %d\n",
				priv->listen_interval,
				mgt_frame->u.assoc_req.listen_interval);
			/* Replace listen interval derieved from
			 * the one read from SDD */
			mgt_frame->u.assoc_req.listen_interval =
				priv->listen_interval;
		}
	}

	if (likely(!priority)) {
		if (ieee80211_is_action(t->hdr->frame_control))
			priority = WSM_EPTA_PRIORITY_ACTION;
		else if (ieee80211_is_mgmt(t->hdr->frame_control))
			priority = WSM_EPTA_PRIORITY_MGT;
		else if ((wsm->queueId == WSM_QUEUE_VOICE))
			priority = WSM_EPTA_PRIORITY_VOICE;
		else if ((wsm->queueId == WSM_QUEUE_VIDEO))
			priority = WSM_EPTA_PRIORITY_VIDEO;
		else
			priority = WSM_EPTA_PRIORITY_DATA;
	}

	txrx_printk(KERN_DEBUG "[TX] EPTA priority %d.\n",
		priority);

	wsm->flags |= priority << 1;
}
#if defined(CONFIG_NL80211_TESTMODE) || defined(CONFIG_ATBM_IOCTRL)

extern int atbm_tool_shortGi;
#endif
static int
atbm_tx_h_rate_policy(struct atbm_common *hw_priv,
			struct atbm_txinfo *t,
			struct wsm_tx *wsm)
{

	bool tx_policy_renew = false;
	struct atbm_vif *priv =
				ABwifi_get_vif_from_ieee80211(t->tx_info->control.vif);
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	static unsigned long g_printf_time = 0;
	if (!g_printf_time)
	{
		g_printf_time= jiffies;
	}
#endif //CONFIG_ATBM_APOLLO_TX_POLICY_DEBUG
	if (!hw_priv->channel)
	{
		return -EFAULT;
	}


#if OLD_RATE_POLICY
	t->txpriv.rate_id = tx_policy_get(hw_priv,
		t->tx_info->control.rates, IEEE80211_TX_MAX_RATES,
		&tx_policy_renew);
	if (t->txpriv.rate_id == ATBM_APOLLO_INVALID_RATE_ID)
		return -EFAULT;
#else //OLD_RATE_POLICY
	t->txpriv.rate_id = 0;
#endif //OLD_RATE_POLICY

	wsm->flags |= t->txpriv.rate_id << 4;
	t->rate = atbm_get_tx_rate(hw_priv,
		&t->tx_info->control.rates[0]);
	wsm->maxTxRate = t->rate->hw_value;
	//printk("recovery wsm->TxRateRetry[0] %d\n",wsm->maxTxRate);
#if (OLD_RATE_POLICY==0)
	wsm->TxRateRetry[0]= atbm_get_tx_rate_hw_value(hw_priv,
		&t->tx_info->control.rates[1]);
	//printk("1wsm->TxRateRetry[0] %d frame_control %x\n",wsm->TxRateRetry[0],t->hdr->frame_control);
	if(wsm->TxRateRetry[0] == 0xff){
		//printk("recovery wsm->TxRateRetry[0] %d\n",wsm->maxTxRate);
		wsm->TxRateRetry[0] = wsm->maxTxRate;
		wsm->TxRateRetry[1] = wsm->maxTxRate;
		wsm->TxRateRetry[2] = wsm->maxTxRate;
		wsm->TxRateRetry[3] = wsm->maxTxRate;
		goto __rate_set_end;
	}
	wsm->TxRateRetry[1]= atbm_get_tx_rate_hw_value(hw_priv,
		&t->tx_info->control.rates[2]);
	//printk("1wsm->TxRateRetry[1] %d\n",wsm->TxRateRetry[1]);
	if(wsm->TxRateRetry[1] == 0xff){
		//printk("recovery wsm->TxRateRetry[1] %d\n",wsm->TxRateRetry[0]);
		wsm->TxRateRetry[1] = wsm->TxRateRetry[0];
		wsm->TxRateRetry[2] = wsm->TxRateRetry[0];
		wsm->TxRateRetry[3] = wsm->TxRateRetry[0];
		goto __rate_set_end;
	}
	wsm->TxRateRetry[2]= atbm_get_tx_rate_hw_value(hw_priv,
		&t->tx_info->control.rates[3]);
	//printk("1wsm->TxRateRetry[2] %d\n",wsm->TxRateRetry[2]);
	if(wsm->TxRateRetry[2] == 0xff){
		//printk("recovery wsm->TxRateRetry[2] %d\n",wsm->TxRateRetry[1]);
		wsm->TxRateRetry[2] = wsm->TxRateRetry[1];
		wsm->TxRateRetry[3] = wsm->TxRateRetry[1];
		goto __rate_set_end;
	}
	wsm->TxRateRetry[3]= atbm_get_tx_rate_hw_value(hw_priv,
		&t->tx_info->control.rates[4]);
	//printk("1wsm->TxRateRetry[3] %d\n",wsm->TxRateRetry[3]);
	if(wsm->TxRateRetry[3] == 0xff){
		//printk("recovery wsm->TxRateRetry[3] %d\n",wsm->TxRateRetry[2]);
		wsm->TxRateRetry[3] = wsm->TxRateRetry[2];
	}
__rate_set_end:
#endif //(OLD_RATE_POLICY==0)
	if (t->rate->flags & IEEE80211_TX_RC_MCS) {
#ifndef CONFIG_ATBM_APOLLO_TESTMODE
		if (priv->association_mode.greenfieldMode)
			wsm->htTxParameters |=
				__cpu_to_le32(WSM_HT_TX_GREENFIELD);
		else
			wsm->htTxParameters |=
				__cpu_to_le32(WSM_HT_TX_MIXED);

#else
		if (t->tx_info->control.rates[0].flags & IEEE80211_TX_RC_GREEN_FIELD)
		{
			wsm->htTxParameters |=
				__cpu_to_le32(WSM_HT_TX_GREENFIELD);
			priv->association_mode.greenfieldMode = 1;
		}
		else
		{
			wsm->htTxParameters |=
				__cpu_to_le32(WSM_HT_TX_MIXED);
			priv->association_mode.greenfieldMode = 0;
		}
#endif //CONFIG_ATBM_APOLLO_TESTMODE
		if ((t->tx_info->control.rates[0].flags & IEEE80211_TX_RC_SHORT_GI)
#if defined(CONFIG_NL80211_TESTMODE) || defined(CONFIG_ATBM_IOCTRL)
			||(atbm_tool_shortGi == 1)
#endif
			)//IEEE80211_TX_RC_SHORT_GI
		{
			if(wsm->htTxParameters&(__cpu_to_le32(WSM_HT_TX_MIXED)))
				wsm->htTxParameters |=
					__cpu_to_le32(WSM_HT_TX_SGI);
		}
	}
#ifdef CONFIG_ATBM_APOLLO_TESTMODE1
	if(time_is_before_jiffies(g_printf_time+5*HZ)){
		printk("rate = %d, flags = %d\n", t->rate->bitrate, t->rate->flags);
		printk("wsm->maxTxRate = %d\n", wsm->maxTxRate);
		printk("greenfieldMode = %s,", (priv->association_mode.greenfieldMode)?"greenfield":"mixed");
		printk("%s GI\n", (t->tx_info->control.rates[0].flags & IEEE80211_TX_RC_SHORT_GI)?"short":"long");
		printk("queuePending :[VO]<%d>[Vi]<%d>[BE]<%d>[BK]<%d> \n",
			 hw_priv->tx_queue[0].num_pending,
			 hw_priv->tx_queue[1].num_pending,
			 hw_priv->tx_queue[2].num_pending,
		   	 hw_priv->tx_queue[3].num_pending);
		g_printf_time= jiffies;
	}
#endif //CONFIG_ATBM_APOLLO_TESTMODE


#if OLD_RATE_POLICY
	if (tx_policy_renew) {
		tx_policy_printk(KERN_DEBUG "[TX] TX policy renew.\n");
		/* It's not so optimal to stop TX queues every now and then.
		 * Maybe it's better to reimplement task scheduling with
		 * a counter. */
		/* atbm_tx_queues_lock(priv); */
		/* Definetly better. TODO. */
		wsm_lock_tx_async(hw_priv);
		atbm_tx_queues_lock(hw_priv);
		if (atbm_hw_priv_queue_work(hw_priv,
				&hw_priv->tx_policy_upload_work) <= 0) {
			atbm_tx_queues_unlock(hw_priv);
			wsm_unlock_tx(hw_priv);
		}
	}
#endif//#if OLD_RATE_POLICY
	return 0;
}

static bool
atbm_tx_h_pm_state(struct atbm_vif *priv,
		     struct atbm_txinfo *t)
{
	int was_buffered = 1;

	if (t->txpriv.link_id == priv->link_id_after_dtim &&
			!priv->buffered_multicasts) {
		priv->buffered_multicasts = true;
		if (priv->sta_asleep_mask)
			atbm_hw_priv_queue_work(priv->hw_priv,
				&priv->multicast_start_work);
	}

	if (t->txpriv.raw_link_id && t->txpriv.tid < ATBM_APOLLO_MAX_TID)
		was_buffered = priv->link_id_db[t->txpriv.raw_link_id - 1].buffered[t->txpriv.tid]++;

	return !was_buffered;
}

static void
atbm_tx_h_ba_stat(struct atbm_vif *priv,
		    struct atbm_txinfo *t)
{
	struct atbm_common *hw_priv = priv->hw_priv;

	if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_STA)
		return;
	if (!atbm_is_ht(&hw_priv->ht_info))
		return;
	if (!priv->setbssparams_done)
		return;
	if (!ieee80211_is_data(t->hdr->frame_control))
		return;

	spin_lock_bh(&hw_priv->ba_lock);
	hw_priv->ba_acc += t->skb->len - t->hdrlen;
	if (!(hw_priv->ba_cnt_rx || hw_priv->ba_cnt)) {
		mod_timer(&hw_priv->ba_timer,
			jiffies + ATBM_APOLLO_BLOCK_ACK_INTERVAL);
	}
	hw_priv->ba_cnt++;
	spin_unlock_bh(&hw_priv->ba_lock);
}

static int
atbm_tx_h_skb_pad(struct atbm_common *priv,
		    struct wsm_tx *wsm,
		    struct sk_buff *skb)
{
	u32 len = __le16_to_cpu(wsm->hdr.len);
	u32 padded_len = priv->sbus_ops->align_size(priv->sbus_priv, len);

	if (WARN_ON(atbm_skb_padto(skb, padded_len) != 0)) {
		return -EINVAL;
	}
	return 0;
}
/* ******************************************************************** */
extern 	int atbm_tcp_ack_offload(struct atbm_queue *queue,struct atbm_txpriv *txpriv,struct sk_buff *skb_new);

void __atbm_tx(struct ieee80211_hw *dev, struct sk_buff *skb)
{
	struct atbm_common *hw_priv = dev->priv;
	struct atbm_txinfo t = {
		.skb = skb,
		.queue = skb_get_queue_mapping(skb),
		.tx_info = IEEE80211_SKB_CB(skb),
		.hdr = (struct ieee80211_hdr *)skb->data,
		.txpriv.tid = ATBM_APOLLO_MAX_TID,
		.txpriv.rate_id = ATBM_APOLLO_INVALID_RATE_ID,
#ifdef P2P_MULTIVIF
		.txpriv.raw_if_id = 0,
#endif
	};
	struct ieee80211_sta *sta;
	struct wsm_tx *wsm;
	bool tid_update = 0;
	u8 flags = 0;
	int ret;
	struct atbm_vif *priv;
	struct ieee80211_hdr *frame = (struct ieee80211_hdr *)skb->data;
        struct atbm_ieee80211_mgmt *mgmt = (struct atbm_ieee80211_mgmt *)skb->data;
	if (!skb->data)
		BUG_ON(1);

	if (!(t.tx_info->control.vif)) {
	        goto drop;
	}
	priv = ABwifi_get_vif_from_ieee80211(t.tx_info->control.vif);
	if (!priv)
		goto drop;
	#ifdef RESET_CHANGE
	if(atomic_read(&hw_priv->reset_flag))
	{
		printk("%s:hw_priv->reset_flag lock \n",__func__);
		goto drop;
	}
	#endif
	if (atomic_read(&priv->enabled) == 0)
		goto drop;

#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	if (t.queue < 4)
	{

		spin_lock_bh(&hw_priv->tsm_lock);

		hw_priv->atbm_tsm_stats[t.queue].tid = t.queue;
		hw_priv->atbm_tsm_stats[t.queue].txed_msdu_count++;
		spin_unlock_bh(&hw_priv->tsm_lock);
	}
#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
#ifdef ICMP_HIGH_PRIO
	if(ieee80211_is_data_qos(frame->frame_control))
	{
		struct iphdr *skbiphdr = NULL;
		int iphdr_index = 0;
		iphdr_index = ieee80211_hdrlen(frame->frame_control)+8;
		skbiphdr = (struct iphdr *)(skb->data+iphdr_index);

		if(skbiphdr->protocol == 1)
		{
			static int send_rate[]={4,8,8,10,10,10,10,10};
			u8 *p = ieee80211_get_qos_ctl(frame);
			u8 ack_policy, tid;
			u8 index = 0;
			t.queue = 1;
			skb->priority = 5;
			tid = skb->priority & IEEE80211_QOS_CTL_TAG1D_MASK;
			ack_policy = *p & 0x0078;
			*p++ = ack_policy | tid;
			for(index = 0;index < IEEE80211_TX_MAX_RATES;index++)
			{
				if((t.tx_info->control.rates[index].flags&IEEE80211_TX_RC_MCS)
				    &&
				    (t.tx_info->control.rates[index].idx != -1)
				    &&
				    (t.tx_info->control.rates[index].idx <= 7)
				)
				{
					u8 tmp_rate_index = t.tx_info->control.rates[index].idx;
					t.tx_info->control.rates[index].flags &= ~IEEE80211_TX_RC_MCS;
					t.tx_info->control.rates[index].idx = send_rate[tmp_rate_index];
					t.tx_info->control.rates[index].count = 10;
				}
			}
		}
	}
#endif
	if ((ieee80211_is_action(frame->frame_control))
                        && (mgmt->u.action.category == WLAN_CATEGORY_PUBLIC)) {
                u8 *action = (u8*)&mgmt->u.action.category;				
				TxRxPublicActionFrame((u8*)mgmt,skb->len,1);
                atbm_check_go_neg_conf_success(hw_priv, action);
				atbm_check_prov_desc_req(hw_priv, action);
        }
#ifdef ATBM_P2P_CHANGE
	atbm_parase_p2p_action_frame(priv,skb,true);
#endif
#ifdef ATBM_11W_TEST
	if(ieee80211_is_deauth(frame->frame_control))
	{
		memcpy(mgmt->da,broadcast_addr,6);
		printk("ATBM_11W_TEST\n");
	}
#endif
#ifdef USB_BUS
	if(atomic_xchg(&hw_priv->bh_suspend_usb, 0)){
		extern 	int atbm_usb_pm_async(struct sbus_priv *self, bool	auto_suspend);
		atbm_usb_pm_async(hw_priv->sbus_priv, false);
	}
#endif
/*
* if the suppliacant and hostapd is in connecting state,
* systerm can not goto suspend.
*/
#ifdef CONFIG_PM
	if (ieee80211_is_auth(frame->frame_control)){
		printk(KERN_ERR "%s:authen, delay suspend\n",__func__);
		atbm_pm_stay_awake(&hw_priv->pm_state, 5*HZ);
	}
#endif
	t.txpriv.if_id = priv->if_id;
	t.hdrlen = ieee80211_hdrlen(t.hdr->frame_control);
	t.da = ieee80211_get_DA(t.hdr);
	t.sta_priv =
		(struct atbm_sta_priv *)&t.tx_info->control.sta->drv_priv;

	if (WARN_ON(t.queue >= 4))
		goto drop;
#ifdef MINSTREL_RSSI_USED //disable UDP drop when buffer fulled, debug UDP drop case...
    //#if 0, #endif
#else
	#if 0
  	#ifndef ATBM_WIFI_QUEUE_LOCK_BUG
	spin_lock_bh(&hw_priv->tx_queue[t.queue].lock);

	if ((priv->if_id == 0) &&
		(hw_priv->tx_queue[t.queue].num_queued_vif[0] >=
			hw_priv->vif0_throttle)) {
		spin_unlock_bh(&hw_priv->tx_queue[t.queue].lock);
		{
			extern unsigned long  atbm_queue_ttl(struct atbm_queue *queue);
		
			//printk("%s:drop %d,ttl %ld ms\n",__func__,__LINE__,atbm_queue_ttl(&hw_priv->tx_queue[t.queue]));
		}
		goto drop;
	} else if ((priv->if_id == 1) &&
		(hw_priv->tx_queue[t.queue].num_queued_vif[1] >=
			hw_priv->vif1_throttle)) {
		spin_unlock_bh(&hw_priv->tx_queue[t.queue].lock);
		goto drop;
	}

	spin_unlock_bh(&hw_priv->tx_queue[t.queue].lock);
   #endif
   #endif
#endif
	ret = atbm_tx_h_calc_link_ids(priv, &t);
	if (ret)
		goto drop;

	/*printk("=======>[TX] TX[fc=%x] %d bytes (if_id: %d,"
			" queue: %d, link_id: %d (%d)).\n",
			t.hdr->frame_control,skb->len, priv->if_id, t.queue, t.txpriv.link_id,
			t.txpriv.raw_link_id);
	*/

	atbm_tx_h_pm(priv, &t);
	atbm_tx_h_calc_tid(priv, &t);
	ret = atbm_tx_h_crypt(priv, &t);
	if (ret)
		goto drop;
	ret = atbm_tx_h_align(priv, &t, &flags);
	if (ret)
		goto drop;
	ret = atbm_tx_h_action(priv, &t);
	if (ret)
		goto drop;
	wsm = atbm_tx_h_wsm(priv, &t);
	if (!wsm) {
		ret = -ENOMEM;
		goto drop;
	}
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	flags |= WSM_TX_FLAG_EXPIRY_TIME;
#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
	wsm->flags |= flags;
	atbm_tx_h_bt(priv, &t, wsm);
	ret = atbm_tx_h_rate_policy(hw_priv, &t, wsm);
	if (ret){
		ret = -1;
		goto drop;
	}
#if defined (CONFIG_TX_NO_CONFIRM)||defined(ATBM_SDIO_PATCH)
	wsm->htTxParameters &= ~(__cpu_to_le32(WSM_NEED_TX_CONFIRM));
	if(skb->protocol == cpu_to_be16(ETH_P_PAE)){
		wsm->htTxParameters |=__cpu_to_le32(WSM_NEED_TX_CONFIRM);
		printk(KERN_DEBUG "%s:ETH_P_PAE(%p)\n",__func__,wsm);
	}

	if(!ieee80211_is_data(t.hdr->frame_control)){
		wsm->htTxParameters |=__cpu_to_le32(WSM_NEED_TX_CONFIRM);
		printk(KERN_DEBUG "%s:not data(%p)(%d)\n",__func__,wsm,wsm->htTxParameters&__cpu_to_le32(WSM_NEED_TX_CONFIRM));
	}
//	wsm->htTxParameters |=__cpu_to_le32(WSM_NEED_TX_CONFIRM);
#endif //CONFIG_TX_NO_CONFIRM	
	ret = atbm_tx_h_skb_pad(hw_priv, wsm, skb);
	if (ret){
		ret = -2;
		goto drop;
	}
	/*
	ret = atbm_tcp_ack_offload(&hw_priv->tx_queue[t.queue],&t.txpriv,skb);
	if (ret){
		ret = -3;
		goto drop;
	}
	*/

	rcu_read_lock();
	sta = rcu_dereference(t.tx_info->control.sta);
	
	//atbm_tx_udp(priv,t.skb,wsm);

	//printk("%s %d fc =%x\n",__func__,__LINE__,t.hdr->frame_control);
	atbm_tx_h_ba_stat(priv, &t);
	spin_lock_bh(&priv->ps_state_lock);
	{
		tid_update = atbm_tx_h_pm_state(priv, &t);
		BUG_ON(atbm_queue_put(&hw_priv->tx_queue[t.queue],
				t.skb, &t.txpriv));
	}
	spin_unlock_bh(&priv->ps_state_lock);

	if (tid_update && sta)
		ieee80211_sta_set_buffered(sta,
				t.txpriv.tid, true);

	rcu_read_unlock();
	atbm_bh_wakeup(hw_priv);

	return;

drop:
	//printk("%s drop tx ret = %d\n",__func__,ret);
//	struct ieee80211_local *local = hw_to_local(hw_priv->hw);
	atbm_skb_dtor(hw_priv, skb, &t.txpriv);
	return;
}

/* ******************************************************************** */
void atbm_tx(struct ieee80211_hw *dev, struct sk_buff *skb)
{
 	struct ieee80211_tx_info *tx_info = IEEE80211_SKB_CB(skb);
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(tx_info->control.vif);
	
	atbm_tx_udp(priv,skb);
	__atbm_tx(dev, skb);

}

static int atbm_handle_pspoll(struct atbm_vif *priv,
				struct sk_buff *skb)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct ieee80211_sta *sta;
	struct ieee80211_pspoll *pspoll =
		(struct ieee80211_pspoll *) skb->data;
	int link_id = 0;
	u32 pspoll_mask = 0;
	int drop = 1;
	int i;

	if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_AP)
		goto done;
	if (memcmp(priv->vif->addr, pspoll->bssid, ETH_ALEN))
		goto done;

	rcu_read_lock();
	sta = ieee80211_find_sta(priv->vif, pspoll->ta);
	if (sta) {
		struct atbm_sta_priv *sta_priv;
		sta_priv = (struct atbm_sta_priv *)&sta->drv_priv;
		link_id = sta_priv->link_id;
		pspoll_mask = BIT(sta_priv->link_id);
	}
	rcu_read_unlock();
	if (!link_id)
		goto done;

	priv->pspoll_mask |= pspoll_mask;
	drop = 0;

	/* Do not report pspols if data for given link id is
	 * queued already. */
	for (i = 0; i < 4; ++i) {
		if (atbm_queue_get_num_queued(priv,
				&hw_priv->tx_queue[i],
				pspoll_mask)) {
			atbm_bh_wakeup(hw_priv);
			drop = 1;
			break;
		}
	}
	txrx_printk(KERN_DEBUG "[RX] PSPOLL: %s\n", drop ? "local" : "fwd");
done:
	return drop;
}

/* ******************************************************************** */

void atbm_tx_confirm_cb(struct atbm_common *hw_priv,
			  struct wsm_tx_confirm *arg)
{
	u8 queue_id = atbm_queue_get_queue_id(arg->packetID);
	struct atbm_queue *queue = &hw_priv->tx_queue[queue_id];
	struct sk_buff *skb;
	const struct atbm_txpriv *txpriv;
	struct atbm_vif *priv;

	txrx_printk(KERN_DEBUG "[TX] TX confirm: %d, %d.\n",
		arg->status, arg->ackFailures);
	if(queue_id >= 4){
		printk("atbm_tx_confirm_cb %d\n",queue_id);
	}
	if(WARN_ON(!queue)){
		printk("atbm_tx_confirm_cb queue %p\n",queue);
		return;
	}

	priv = ABwifi_hwpriv_to_vifpriv(hw_priv, arg->if_id);
	if (unlikely(!priv)){
		printk("<warning>%s arg_if_id %d\n",__func__,arg->if_id);
		return;
	}
	if (unlikely(priv->mode == NL80211_IFTYPE_UNSPECIFIED)) {
		/* STA is stopped. */
		atbm_priv_vif_list_read_unlock(&priv->vif_lock);
		printk("<warning> %s %d\n",__func__,__LINE__);
		return;
	}

	if (WARN_ON(queue_id >= 4)) {
		atbm_priv_vif_list_read_unlock(&priv->vif_lock);
		return;
	}
	if(WARN_ON(arg ==NULL)){
		printk("atbm_tx_confirm_cb arg %p\n",arg);
		return;
	}
#ifdef ATBM_SDIO_PATCH
        if(arg->status == WSM_DATA_CRC_ERRO){
        #ifdef CONFIG_ATBM_APOLLO_TESTMODE
            atbm_queue_remove(hw_priv, queue, arg->packetID);
	    #else
            atbm_queue_remove(queue, arg->packetID);
        #endif /*CONFIG_ATBM_APOLLO_TESTMODE*/

                atbm_priv_vif_list_read_unlock(&priv->vif_lock);
                return ;
        }
#endif
	if (arg->status)
		txrx_printk(KERN_DEBUG "TX failed: %d.\n",
				arg->status);

#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	spin_lock_bh(&hw_priv->tsm_lock);
	if (arg->status)
	{
		hw_priv->atbm_tsm_stats[queue_id].msdu_failed_count++;
		hw_priv->atbm_tsm_stats[queue_id].tid = queue_id;
	}
	hw_priv->atbm_tsm_stats[queue_id].multi_retry_count += arg->ackFailures;

	if ((arg->status == WSM_STATUS_RETRY_EXCEEDED) ||
	    (arg->status == WSM_STATUS_TX_LIFETIME_EXCEEDED)) {
		hw_priv->tsm_stats.msdu_discarded_count++;
		hw_priv->atbm_tsm_stats[queue_id].msdu_discarded_count++;

	} else if ((hw_priv->start_stop_tsm.start) &&
		(arg->status == WSM_STATUS_SUCCESS)) {
		if (queue_id == hw_priv->tsm_info.ac) {
			struct timeval tmval;
			u16 pkt_delay;
			do_gettimeofday(&tmval);
			pkt_delay =
				hw_priv->start_stop_tsm.packetization_delay;
			if (hw_priv->tsm_info.sta_roamed &&
			    !hw_priv->tsm_info.use_rx_roaming) {
				hw_priv->tsm_info.roam_delay = tmval.tv_usec -
				hw_priv->tsm_info.txconf_timestamp_vo;
				if (hw_priv->tsm_info.roam_delay > pkt_delay)
					hw_priv->tsm_info.roam_delay -= pkt_delay;
				txrx_printk(KERN_DEBUG "[TX] txConf"
				"Roaming: roam_delay = %u\n",
				hw_priv->tsm_info.roam_delay);
				hw_priv->tsm_info.sta_roamed = 0;
			}
			hw_priv->tsm_info.txconf_timestamp_vo = tmval.tv_usec;
		}
	}
	spin_unlock_bh(&hw_priv->tsm_lock);
#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
	if ((arg->status == WSM_REQUEUE) &&
	    (arg->flags & WSM_TX_STATUS_REQUEUE)) {
		/* "Requeue" means "implicit suspend" */
		struct wsm_suspend_resume suspend = {
			.link_id = arg->link_id,
			.stop = 1,
			.multicast = !arg->link_id,
			.if_id = arg->if_id,
		};
		atbm_suspend_resume(priv, &suspend);
		wiphy_warn(priv->hw->wiphy, "Requeue for link_id %d (try %d)."
			" STAs asleep: 0x%.8X\n",
			arg->link_id,
			atbm_queue_get_generation(arg->packetID) + 1,
			priv->sta_asleep_mask);
		printk("<WARNING> %s 111\n",__func__);
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
		WARN_ON(atbm_queue_requeue(hw_priv, queue,
				arg->packetID, true));
#else
		WARN_ON(atbm_queue_requeue(queue,
				arg->packetID, true));
#endif
		spin_lock_bh(&priv->ps_state_lock);
		if (!arg->link_id) {
			priv->buffered_multicasts = true;
			if (priv->sta_asleep_mask) {
				atbm_hw_priv_queue_work(hw_priv,
					&priv->multicast_start_work);
			}
		}
		spin_unlock_bh(&priv->ps_state_lock);
		atbm_priv_vif_list_read_unlock(&priv->vif_lock);
	} else if (!WARN_ON(atbm_queue_get_skb(
			queue, arg->packetID, &skb, &txpriv))) {
		struct ieee80211_tx_info *tx = IEEE80211_SKB_CB(skb);
		int tx_count = arg->ackFailures;
		u8 ht_flags = 0;
		int i;
		/*
		*when in listen state,we send action again
		*
		*/
		while((arg->status)&&(arg->status != 1)){
			struct ieee80211_hdr *frame = (struct ieee80211_hdr *)(skb->data+txpriv->offset);
			if(!atomic_read(&hw_priv->remain_on_channel)){
				break;
			}

			if(txpriv->if_id != hw_priv->roc_if_id){
				printk(KERN_ERR "%s:txpriv->if_id(%d),hw_priv->roc_if_id(%d)\n",__func__,txpriv->if_id,hw_priv->roc_if_id);
				break;
			}
			
			if(!(tx->flags & IEEE80211_TX_INTFL_NL80211_FRAME_TX)){
				printk(KERN_ERR "%s:not NL80211_FRAME_TX\n",__func__);
				break;
			}
			if(!(tx->flags & IEEE80211_TX_CTL_REQ_TX_STATUS)){
				printk(KERN_ERR "%s:not REQ_TX_STATUS\n",__func__);
				break;
			}
			if(!time_is_after_jiffies(hw_priv->roc_start_time+((hw_priv->roc_duration+35)*HZ)/1000)){
				printk(KERN_ERR "%s:roc time is not enough to retry transmit\n",__func__);
				break;
			}

			if(!ieee80211_is_action(frame->frame_control)){
				printk(KERN_ERR "%s:not action(%x)\n",__func__,frame->frame_control);
				break;
			}
			#ifdef CONFIG_ATBM_APOLLO_TESTMODE
			WARN_ON(atbm_queue_requeue(hw_priv, queue,
					arg->packetID, false));
			#else
			WARN_ON(atbm_queue_requeue(queue,
					arg->packetID, false));
			#endif

			printk(KERN_ERR "%s:retry remain on channel pkg\n",__func__);
			atbm_priv_vif_list_read_unlock(&priv->vif_lock);

			atbm_bh_wakeup(hw_priv);
			return;
			
		}
		if (priv->association_mode.greenfieldMode)
			ht_flags |= IEEE80211_TX_RC_GREEN_FIELD;

		if (likely(!arg->status)) {
			tx->flags |= IEEE80211_TX_STAT_ACK;
			priv->cqm_tx_failure_count = 0;
			++tx_count;
			atbm_debug_txed(priv);
			if (arg->flags & WSM_TX_STATUS_AGGREGATION) {
				/* Should report aggregation to mac80211:
				 * it can help  minstrel_ht  calc fit rate*/
				//tx->flags |= IEEE80211_TX_STAT_AMPDU;
				//tx->status.ampdu_len=hw_priv->wsm_txframe_num;
				//tx->status.ampdu_ack_len=(hw_priv->wsm_txframe_num-arg->ackFailures);
				atbm_debug_txed_agg(priv);
			}
		} else {
			spin_lock_bh(&priv->bss_loss_lock);
			if (priv->bss_loss_status ==
					ATBM_APOLLO_BSS_LOSS_CONFIRMING &&
					priv->bss_loss_confirm_id ==
					arg->packetID) {
				priv->bss_loss_status =
					ATBM_APOLLO_BSS_LOSS_CONFIRMED;
				spin_unlock_bh(&priv->bss_loss_lock);
				cancel_delayed_work(&priv->bss_loss_work);
				atbm_hw_priv_queue_delayed_work(hw_priv,
						&priv->bss_loss_work, 0);
			} else
				spin_unlock_bh(&priv->bss_loss_lock);

			/* TODO: Update TX failure counters */
			if (unlikely(priv->cqm_tx_failure_thold &&
			     (++priv->cqm_tx_failure_count >
			      priv->cqm_tx_failure_thold))) {
				priv->cqm_tx_failure_thold = 0;
				atbm_hw_priv_queue_work(hw_priv,
						&priv->tx_failure_work);
			}
			if (tx_count)
				++tx_count;
		}
		atbm_priv_vif_list_read_unlock(&priv->vif_lock);
             //  printk("ackfailures:%d tx_count:%d status:%d\n", arg->ackFailures, tx_count, arg->status);
		for (i = 0; i < IEEE80211_TX_MAX_RATES; ++i) {
			if (tx->status.rates[i].count >= tx_count) {
				tx->status.rates[i].count = tx_count;
				break;
			}
			tx_count -= tx->status.rates[i].count;
			if (tx->status.rates[i].flags & IEEE80211_TX_RC_MCS)
				tx->status.rates[i].flags |= ht_flags;
		}
/*
#ifdef MINSTREL_RSSI_USED
		if(tx_count > 0)
		{
                     if(tx_count > 20)
                     printk(KERN_ERR "%s:Error LMAC retry too may times!! rate i:%d  tx_count:%d\n",__func__, i,tx_count);
                     //tx->status.rates[i].count =  tx->status.rates[i].count  + tx_count;    
                }
#endif */
		for (++i; i < IEEE80211_TX_MAX_RATES; ++i) {
			tx->status.rates[i].count = 0;
			tx->status.rates[i].idx = -1;
		}
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
		atbm_queue_remove(hw_priv, queue, arg->packetID);
#else
		atbm_queue_remove(queue, arg->packetID);
#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
	}else
	{
		printk("<warning> %s11112222\n",__func__);
		atbm_priv_vif_list_read_unlock(&priv->vif_lock);
	}
}

static void atbm_notify_buffered_tx(struct atbm_vif *priv,
			       struct sk_buff *skb, int link_id, int tid)
{
	struct ieee80211_sta *sta;
	struct ieee80211_hdr *hdr;
	u8 *buffered;
	u8 still_buffered = 0;

	if (link_id && tid < ATBM_APOLLO_MAX_TID) {
		buffered = priv->link_id_db[link_id - 1].buffered;

		spin_lock_bh(&priv->ps_state_lock);
		if (!WARN_ON(!buffered[tid]))
			still_buffered = --buffered[tid];
		spin_unlock_bh(&priv->ps_state_lock);

		if (!still_buffered && tid < ATBM_APOLLO_MAX_TID) {
			hdr = (struct ieee80211_hdr *) skb->data;
			rcu_read_lock();
			sta = ieee80211_find_sta(priv->vif, hdr->addr1);
			if (sta)
				ieee80211_sta_set_buffered(sta, tid, false);
			rcu_read_unlock();
		}
	}
}

void atbm_skb_dtor(struct atbm_common *hw_priv,
		     struct sk_buff *skb,
		     const struct atbm_txpriv *txpriv)
{
	struct atbm_vif *priv =
		__ABwifi_hwpriv_to_vifpriv(hw_priv, txpriv->if_id);

	atbm_skb_pull(skb, txpriv->offset);
	if (priv && txpriv->rate_id != ATBM_APOLLO_INVALID_RATE_ID) {
		atbm_notify_buffered_tx(priv, skb,
				txpriv->raw_link_id, txpriv->tid);
		tx_policy_put(hw_priv, txpriv->rate_id);
	}
//	spin_lock_bh(&priv->ps_state_lock);
	//ieee80211_tx_status(hw_priv->hw, skb);
	ieee80211_tx_status_irqsafe(hw_priv->hw,skb);
//	spin_unlock_bh(&priv->ps_state_lock);

}


static void
atbm_rx_h_ba_stat(struct atbm_vif *priv,
		    size_t hdrlen, size_t skb_len )
{
	struct atbm_common *hw_priv = priv->hw_priv;
	if (priv->join_status != ATBM_APOLLO_JOIN_STATUS_STA)
		return;
	if (!atbm_is_ht(&hw_priv->ht_info))
		return;
	if (!priv->setbssparams_done)
		return;

	spin_lock_bh(&hw_priv->ba_lock);
	hw_priv->ba_acc_rx += skb_len - hdrlen;
	if (!(hw_priv->ba_cnt_rx || hw_priv->ba_cnt)) {
		mod_timer(&hw_priv->ba_timer,
			jiffies + ATBM_APOLLO_BLOCK_ACK_INTERVAL);
	}
	hw_priv->ba_cnt_rx++;
	spin_unlock_bh(&hw_priv->ba_lock);
}


void atbm_rx_cb(struct atbm_vif *priv,
		  struct wsm_rx *arg,
		  struct sk_buff **skb_p)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	struct sk_buff *skb = *skb_p;
	struct ieee80211_rx_status *hdr = IEEE80211_SKB_RXCB(skb);
	struct ieee80211_hdr *frame = (struct ieee80211_hdr *)skb->data;
	struct atbm_ieee80211_mgmt *mgmt = (struct atbm_ieee80211_mgmt *)skb->data;
	struct atbm_link_entry *entry = NULL;
	#ifdef CONFIG_PM
	unsigned long grace_period;
	#endif
	bool early_data = false;
	size_t hdrlen = 0;

	hdr->flag = 0;
	//atbm_rx_udp(skb);
	#ifdef RESET_CHANGE
	if(atomic_read(&hw_priv->reset_flag))
	{
		printk("%s:hw_priv->reset_flag lock \n",__func__);
		goto drop;
	}
	#endif
	if (unlikely(priv->mode == NL80211_IFTYPE_UNSPECIFIED)) {
		/* STA is stopped. */
		printk(KERN_ERR "priv->mode == NL80211_IFTYPE_UNSPECIFIED,seq(%x)\n",frame->seq_ctrl);
		goto drop;
	}
	if ((ieee80211_is_action(frame->frame_control))
                        && (mgmt->u.action.category == WLAN_CATEGORY_PUBLIC)) {
		u8 *action = (u8*)&mgmt->u.action.category;
		TxRxPublicActionFrame((u8*)mgmt,skb->len,0);
		atbm_check_go_neg_conf_success(hw_priv, action);
	}
	
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	spin_lock_bh(&hw_priv->tsm_lock);
	if (hw_priv->start_stop_tsm.start) {
		unsigned queue_id = skb_get_queue_mapping(skb);
		if (queue_id == 0) {
			struct timeval tmval;
			do_gettimeofday(&tmval);
			if (hw_priv->tsm_info.sta_roamed &&
			    hw_priv->tsm_info.use_rx_roaming) {
				hw_priv->tsm_info.roam_delay = tmval.tv_usec -
					hw_priv->tsm_info.rx_timestamp_vo;
				txrx_printk(KERN_DEBUG "[RX] RxInd Roaming:"
				"roam_delay = %u\n", hw_priv->tsm_info.roam_delay);
				hw_priv->tsm_info.sta_roamed = 0;
			}
			hw_priv->tsm_info.rx_timestamp_vo = tmval.tv_usec;
		}
	}
	spin_unlock_bh(&hw_priv->tsm_lock);
#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
	if (arg->link_id && (arg->link_id != ATBM_APOLLO_LINK_ID_UNMAPPED)
			&& (arg->link_id <= ATBM_APOLLO_MAX_STA_IN_AP_MODE)) {
		entry =	&priv->link_id_db[arg->link_id - 1];
		if (entry->status == ATBM_APOLLO_LINK_SOFT &&
				ieee80211_is_data(frame->frame_control))
			early_data = true;
		entry->timestamp = jiffies;
	}
	else if ((arg->link_id == ATBM_APOLLO_LINK_ID_UNMAPPED)
			&& (priv->vif->p2p)
			&& (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
			&& ieee80211_is_action(frame->frame_control)
			&& (mgmt->u.action.category == WLAN_CATEGORY_PUBLIC)) {
		int action_linkid = 0;
		txrx_printk(KERN_DEBUG "[RX] Going to MAP&RESET link ID\n");
		printk("%s line(%d):if_id(%d),WSM_START_MODE_P2P_GO reset link id\n",__func__,__LINE__,priv->if_id);
		action_linkid = atbm_find_link_id(priv,ieee80211_get_SA(frame));
		if(action_linkid != 0)
		{
			printk("%s line(%d):if_id(%d),WSM_START_MODE_P2P_GO reset link id\n",__func__,__LINE__,priv->if_id);
			spin_lock_bh(&priv->ps_state_lock);
			priv->link_id_db[action_linkid - 1].prev_status =
				priv->link_id_db[action_linkid - 1].status;
			priv->link_id_db[action_linkid - 1].status =
				ATBM_APOLLO_LINK_RESET;
			spin_unlock_bh(&priv->ps_state_lock);
			atbm_hw_priv_queue_work(hw_priv,&priv->linkid_reset_work);
		}
	}

	if (arg->link_id && (arg->link_id != ATBM_APOLLO_LINK_ID_UNMAPPED)
			&& (priv->vif->p2p)
			&& (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
			&& ieee80211_is_action(frame->frame_control)
			&& (mgmt->u.action.category == WLAN_CATEGORY_PUBLIC)) {
		int action_linkid = 0;
		action_linkid = atbm_find_link_id(priv,ieee80211_get_SA(frame));
		/* Link ID already exists for the ACTION frame.
		 * Reset and Remap */
		if(action_linkid &&(arg->link_id != action_linkid))
		{
			printk("Link ID already exists for the ACTION frame\n");
			spin_lock_bh(&priv->ps_state_lock);
			priv->link_id_db[action_linkid - 1].prev_status =
				priv->link_id_db[action_linkid - 1].status;
			priv->link_id_db[action_linkid - 1].status =
				ATBM_APOLLO_LINK_RESET_REMAP;
			spin_unlock_bh(&priv->ps_state_lock);
			atbm_hw_priv_queue_work(hw_priv,&priv->linkid_reset_work);
		}
	}
	if (unlikely(arg->status)) {
		if (arg->status == WSM_STATUS_MICFAILURE) {
			printk(KERN_ERR "[RX] MIC failure. ENCRYPTION [%d]\n",WSM_RX_STATUS_ENCRYPTION(arg->flags));
			hdr->flag |= RX_FLAG_MMIC_ERROR;
		} else if (arg->status == WSM_STATUS_NO_KEY_FOUND) {
			printk(KERN_ERR "[RX] No key found.ENCRYPTION [%d]\n",WSM_RX_STATUS_ENCRYPTION(arg->flags));
			goto drop;
		} else {
//			printk(KERN_ERR "[RX] Receive failure: %d.ENCRYPTION [%d],fc(%x)\n",
//				arg->status,WSM_RX_STATUS_ENCRYPTION(arg->flags),mgmt->frame_control);
			goto drop;
		}
	}
#ifdef CHKSUM_HW_SUPPORT
	else {
		if (arg->flags & WSM_RX_STATUS_CHKSUM_ERROR){
			hdr->flag &=~RX_FLAG_HW_CHKSUM_ERROR;
			hdr->flag |= RX_FLAG_HW_CHKSUM_ERROR;
		}else{
			hdr->flag &=~RX_FLAG_HW_CHKSUM_ERROR;
		}
	}	
#endif
	if (skb->len < sizeof(struct ieee80211_pspoll)) {
		wiphy_warn(priv->hw->wiphy, "Mailformed SDU rx'ed. "
				"Size is lesser than IEEE header.\n");
		printk(KERN_ERR "(skb->len < sizeof(struct ieee80211_pspoll),seq(%x)\n",frame->seq_ctrl);
		goto drop;
	}

	if (unlikely(ieee80211_is_pspoll(frame->frame_control))){
		if (atbm_handle_pspoll(priv, skb))
		{
			printk(KERN_ERR "atbm_handle_pspoll\n");
			goto drop;
		}
	}
#ifdef ATBM_PKG_REORDER
	if(ieee80211_is_back_req(frame->frame_control))
	{
		struct ieee80211_bar *bar = (struct ieee80211_bar *)skb->data;
		struct atbm_ba_params ba_params;
		struct ieee80211_bar * bar_data = skb->data;
		ba_params.tid = le16_to_cpu(bar_data->control) >> 12;
		ba_params.ssn =  le16_to_cpu(bar_data->start_seq_num) >> 4;
		ba_params.action= ATBM_BA__ACTION_RX_BAR;
		ba_params.link_id = arg->link_id;
		printk("rx BAR:ssn(%x),tid(%d),link_id(%d)\n",ba_params.ssn,ba_params.tid,ba_params.link_id);
		atbm_updata_ba_tid_params(priv,&ba_params);
		goto drop;
	}
	else if((ieee80211_is_action(frame->frame_control))
                        && (mgmt->u.action.category == WLAN_CATEGORY_BACK)) 
	{
		struct atbm_ba_params ba_params;
		
		ba_params.link_id = arg->link_id;
		switch (mgmt->u.action.u.addba_req.action_code) {
		case WLAN_ACTION_ADDBA_REQ:
		{
			u16 capab;
			ba_params.action = ATBM_BA__ACTION_RX_ADDBR;
			ba_params.timeout = le16_to_cpu(mgmt->u.action.u.addba_req.timeout);
			capab = le16_to_cpu(mgmt->u.action.u.addba_req.capab);
			ba_params.tid =  (capab & IEEE80211_ADDBA_PARAM_TID_MASK) >> 2;
			ba_params.win_size =  (capab & IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK) >> 6;
			ba_params.ssn = le16_to_cpu(mgmt->u.action.u.addba_req.start_seq_num) >> 4;
			atbm_updata_ba_tid_params(priv,&ba_params);
			break;
		}
		case WLAN_ACTION_DELBA:
		{
			u16 params;
			params = le16_to_cpu(mgmt->u.action.u.delba.params);
			ba_params.tid = (params & IEEE80211_DELBA_PARAM_TID_MASK) >> 12;
			ba_params.action = ATBM_BA__ACTION_RX_DELBA;
			atbm_updata_ba_tid_params(priv,&ba_params);
			break;
		}
		default:
			break;
		}

		goto drop;
	}
#endif
	hdr->mactime = 0; /* Not supported by WSM */
	hdr->band = (arg->channelNumber > 14) ?
			IEEE80211_BAND_5GHZ : IEEE80211_BAND_2GHZ;
	hdr->freq = ieee80211_channel_to_frequency(
			arg->channelNumber,
			hdr->band);

	if (arg->rxedRate >= 14) {
		hdr->flag |= RX_FLAG_HT;
		hdr->rate_idx = arg->rxedRate - 14;
	} else if (arg->rxedRate >= 4) {
		if (hdr->band == IEEE80211_BAND_5GHZ)
			hdr->rate_idx = arg->rxedRate - 6;
		else
			hdr->rate_idx = arg->rxedRate - 2;
	} else {
		hdr->rate_idx = arg->rxedRate;
	}

	hdr->signal = (s8)arg->rcpiRssi;
	hdr->antenna = 0;
#ifdef	ATBM_SUPPORT_PKG_MONITOR
	if(priv->join_status == ATBM_APOLLO_JOIN_STATUS_SIMPLE_MONITOR){
		ieee80211_rx_irqsafe(priv->hw, skb);
		*skb_p = NULL;
		return;
	}
#endif
	hdrlen = ieee80211_hdrlen(frame->frame_control);
#ifndef ATBM_SUPPORT_WIDTH_40M
	if (WSM_RX_STATUS_ENCRYPTION(arg->flags)) {
		size_t iv_len = 0, icv_len = 0;

		hdr->flag |= RX_FLAG_DECRYPTED;

		/* Oops... There is no fast way to ask mac80211 about
		 * IV/ICV lengths. Even defineas are not exposed.*/
		switch (WSM_RX_STATUS_ENCRYPTION(arg->flags)) {
		case WSM_RX_STATUS_WEP:
			iv_len = 4 /* WEP_IV_LEN */;
			icv_len = 4 /* WEP_ICV_LEN */;
			break;
		case WSM_RX_STATUS_TKIP:
			iv_len = 8 /* TKIP_IV_LEN */;
			icv_len = 4 /* TKIP_ICV_LEN */
				+ 8 /*MICHAEL_MIC_LEN*/;
			//mic header is not stripped off by low mac,so it should not be set here
			//hdr->flag |= RX_FLAG_MMIC_STRIPPED;
			break;
		case WSM_RX_STATUS_AES:
			iv_len = 8 /* CCMP_HDR_LEN */;
			icv_len = 8 /* CCMP_MIC_LEN */;
			break;
		case WSM_RX_STATUS_WAPI:
			iv_len = 18 /* WAPI_HDR_LEN */;
			icv_len = 16 /* WAPI_MIC_LEN */;
			hdr->flag |= RX_FLAG_IV_STRIPPED;
			break;
		default:
			WARN_ON("Unknown encryption type");
			goto drop;
		}

		/* Firmware strips ICV in case of MIC failure. */
		if (arg->status == WSM_STATUS_MICFAILURE) {
			icv_len = 0;
			hdr->flag |= RX_FLAG_IV_STRIPPED;
		}

		if (skb->len < hdrlen + iv_len + icv_len) {
			wiphy_warn(priv->hw->wiphy, "Mailformed SDU rx'ed. "
				"Size is lesser than crypto headers.\n");
//			printk("skb->len < hdrlen + iv_len + icv_len,seq(%x)\n",frame->seq_ctrl);
			goto drop;
		}

		/* Protocols not defined in mac80211 should be
		stripped/crypted in driver/firmware */
		if (WSM_RX_STATUS_ENCRYPTION(arg->flags) ==
						WSM_RX_STATUS_WAPI) {
			/* Remove IV, ICV and MIC */
			atbm_skb_trim(skb, skb->len - icv_len);
			memmove(skb->data + iv_len, skb->data, hdrlen);
			atbm_skb_pull(skb, iv_len);
		}

	}
#else
	if(ieee80211_has_protected(frame->frame_control))
		hdr->flag |= RX_FLAG_DECRYPTED;
	if (arg->status == WSM_STATUS_MICFAILURE) {
			hdr->flag |= RX_FLAG_IV_STRIPPED;
	}
#endif

#ifdef ATBM_SUPPORT_WIDTH_40M
	if(priv->if_id == 0)
	{
		if(WSM_RX_STATUS_RX_40M&arg->flags)
		{
			hdr->flag |= RX_FLAG_40MHZ;
			hw_priv->rx_40M_pkg_cnt++;
			hw_priv->rx_20M_pkg_detect = 0;
			hw_priv->rx_40M_pkg_detect ++;
		}
		else if (ieee80211_is_data(frame->frame_control) )
		{
		
			hw_priv->rx_20M_pkg_cnt++;
			hw_priv->rx_20M_pkg_detect++;
			hw_priv->rx_40M_pkg_detect=0;
		}
	}
#endif
	atbm_debug_rxed(priv);
	if (arg->flags & WSM_RX_STATUS_AGGREGATE){
		printk_once("<WIFI> rx ampdu ++ \n");
		atbm_debug_rxed_agg(priv);
	}
#ifdef ATBM_SUPPORT_WIDTH_40M
#if 0
	if(
		(priv->if_id != 0)
	  )
	{
		
		if(ieee80211_is_mgmt(frame->frame_control))
		{
			printk("%s:atbm_clear_wpas_p2p_40M_ie\n",__func__);
			atbm_clear_wpas_p2p_40M_ie(mgmt, skb->len);
		}
	}
#endif
#endif
	if (ieee80211_is_beacon(frame->frame_control) &&
			!arg->status &&
			!memcmp(ieee80211_get_SA(frame), priv->join_bssid,
				ETH_ALEN)) {
		const u8 *tim_ie;
		u8 *ies;
		size_t ies_len;
		priv->disable_beacon_filter = false;
		atbm_hw_priv_queue_work(hw_priv, &priv->update_filtering_work);
		ies = ((struct atbm_ieee80211_mgmt *)
			  (skb->data))->u.beacon.variable;
		ies_len = skb->len - (ies - (u8 *)(skb->data));

		tim_ie = atbm_get_ie(ies, ies_len, ATBM_WLAN_EID_TIM);
		if (tim_ie) {
			struct ieee80211_tim_ie *tim =
				(struct ieee80211_tim_ie *)&tim_ie[2];

			if (priv->join_dtim_period != tim->dtim_period) {
				priv->join_dtim_period = tim->dtim_period;
				atbm_hw_priv_queue_work(hw_priv,
					&priv->set_beacon_wakeup_period_work);
			}
		}
		if (unlikely(priv->disable_beacon_filter)) {
			priv->disable_beacon_filter = false;
			atbm_hw_priv_queue_work(hw_priv,
				&priv->update_filtering_work);
		}
	}
#ifdef ATBM_PRIVATE_IE

typedef struct _Wifi_Recv_Info
{
	unsigned char Ssid[32];
	unsigned char channel;
	unsigned int Rssi;		//信号强度
	unsigned int Quality;	 //信号质量
	unsigned char phy_noise;	//噪声
	unsigned char Bssid[6];	//热点 WiFi MAC地址
	unsigned char User_data[255]; //存放收到广播包后要回复私有数据
}Wifi_Recv_Info_t;	  //接收结构体
extern u32 atbm_get_filter_mac_addr(char *mac_addr);
extern void atbm_set_ap_info(Wifi_Recv_Info_t *infor);
		u32 i;
		u8 *ies, *ie;
		size_t ies_len;
		u8 *priv_ie;
		u8 len;
		u8 *pdata;
		u8 atbm_oui[4]={0x41,0x54,0x42,0x4D};//ATBM
		u32 filter_sta = 0;

		char found_pkt = 0;
		char found_pro_req = 0;
		Wifi_Recv_Info_t recv_info;
		unsigned char filter_mac_g[8][ETH_ALEN];
		
		if (ieee80211_is_beacon(frame->frame_control)){
			//printk("[IE] recv beacon pkt\n");
			ies = ((struct atbm_ieee80211_mgmt *)
				  (skb->data))->u.beacon.variable;

			found_pkt = 1;
		}
		if(ieee80211_is_probe_resp(frame->frame_control)){
			printk("[IE] recv probe resp pkt\n");
			ies = ((struct atbm_ieee80211_mgmt *)
				  (skb->data))->u.probe_resp.variable;

			found_pkt = 1;
		}

		if(ieee80211_is_probe_req(frame->frame_control)){
			printk("[IE] recv probe req pkt\n");
			ies = ((struct atbm_ieee80211_mgmt *)
				  (skb->data))->u.probe_req.variable;

			found_pro_req = 1;
			found_pkt = 1;
		}

		if(found_pkt == 1){
			
			//ies = ((struct atbm_ieee80211_mgmt *)
			//	  (skb->data))->u.probe_resp.variable;
			ies_len = skb->len - (ies - (u8 *)(skb->data));

			memset(&recv_info, 0, sizeof(Wifi_Recv_Info_t));

			priv_ie = atbm_get_ie(ies, ies_len, ATBM_WLAN_EID_PRIVATE);
			//是否包含ATBM 私有字段
			if(priv_ie){
				struct atbm_private_ie *priv_ie_f= (struct atbm_private_ie *)priv_ie;
				//再次确认ATBM 的OUI
				if(0 == memcmp(priv_ie_f->Oui, atbm_oui, 4)){
					printk("[IE] found private ie\n");
					//获取用户设置的MAC 地址集
					filter_sta = atbm_get_filter_mac_addr(&filter_mac_g[0][0]);
					for(i=0;i<8;i++){
						//根据用户设置的MAC 地址，查找符合的MAC
						if((99 == filter_sta) || (0 == memcmp(filter_mac_g[i], frame->addr3, ETH_ALEN))){
							//SSID
							ie = atbm_get_ie(ies, ies_len, ATBM_WLAN_EID_SSID);
							//frame_hexdump("atbm",ie,16);
							len = *(++ie);
							pdata = (++ie);
							memcpy(recv_info.Ssid, pdata, len);
							
							//Channel
							recv_info.channel = arg->channelNumber;

							//Rssi
							recv_info.Rssi = arg->rcpiRssi;
							//printk("[IE]channel: Rssi =%d:%d:%s:%d\n",recv_info.channel,recv_info.Rssi,recv_info.Ssid,len);

							//Qual
							recv_info.Quality = 0;

							//Noise
							recv_info.phy_noise = 0;

							//MAC
							memcpy(recv_info.Bssid, frame->addr3, ETH_ALEN);

							//User Data
							memcpy(recv_info.User_data, priv_ie_f->UserData, priv_ie_f->Length-4);
							atbm_set_ap_info(&recv_info);

							break;
						}
					}
					
				}
			}
		}

		if((found_pro_req == 1)&&(priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP))
			goto drop;
#endif

    if (priv->mode == NL80211_IFTYPE_AP &&( priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP )&&
                    ieee80211_is_beacon(frame->frame_control) &&
                    !arg->status){

            u8 *ies;
            size_t ies_len;
            const u8 *ht_cap;
            ies = ((struct atbm_ieee80211_mgmt *)
                      (skb->data))->u.beacon.variable;
            ies_len = skb->len - (ies - (u8 *)(skb->data));
            ht_cap = atbm_get_ie(ies, ies_len, ATBM_WLAN_EID_HT_CAPABILITY);
            if(!ht_cap){
                    priv->ht_info |= 0x0011;
            }
            atbm_hw_priv_queue_work(hw_priv,
                            &priv->ht_info_update_work);

    }
#ifdef ATBM_SUPPORT_WIDTH_40M
	/*
	*Notify Channel Width frame format(8.5.12.2),handled there.
	*
	*this action frame sended when ap want to change its receive channel width.
	*
	*receive this frame , we do not send cmd to lmac to change the channel type.
	*
	*if ap only receive 20M,tx_20M_lock will be set,then frame send by 20M.stop cca check.
	*if ap can receive 40,then we will handle send 40M or 20M.
	*
	*recieve this frame , we also start chantype_change_work,and set channel_chaging.
	*/
	if((priv->mode == NL80211_IFTYPE_STATION)
		&&
		(priv->if_id == 0)
		&&
		(ieee80211_is_action(frame->frame_control))
		&&
		(!arg->status))
	{
		if(
			(mgmt->u.action.category == WLAN_CATEGORY_HT)
			&&
			(WLAN_HT_ACTION_NOTIFY_CHANWIDTH == mgmt->u.action.u.notify_chan_width.chan_width)
		  )
		{
			u8 prev_lock = !!atomic_read(&hw_priv->tx_20M_lock);
			u8 new_lock  = 0;
			printk("%s:WLAN_HT_ACTION_NOTIFY_CHANWIDTH revive,sendch(%d)\n",__func__,mgmt->u.action.u.notify_chan_width.chan_width); 
			if(mgmt->u.action.u.notify_chan_width.chan_width)
			{
				/*
				*if not surport ht40, we should not receive this frame.
				*/
				spin_lock_bh(&hw_priv->spinlock_channel_type);
				if(hw_priv->channel_type<=NL80211_CHAN_HT20)
					printk("%s:in 20M mode,we can not recive this frame,what happend",__func__); 
				spin_unlock_bh(&hw_priv->spinlock_channel_type);

				atomic_set(&hw_priv->tx_20M_lock,0);
			}
			else
			{
				atomic_set(&hw_priv->tx_20M_lock,1);
			}
			new_lock = !!atomic_read(&hw_priv->tx_20M_lock);
			
			if(prev_lock^new_lock)
			{
				set_bit(WSM_SET_CHANTYPECHANGE_FLAGS__CHANGE_PHY_CHTYPE,&hw_priv->change_bit);
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

			goto drop;
		}
	}
#endif

#ifdef ROAM_OFFLOAD
	if ((ieee80211_is_beacon(frame->frame_control)||ieee80211_is_probe_resp(frame->frame_control)) &&
			!arg->status ) {
		if (hw_priv->auto_scanning && !atomic_read(&hw_priv->scan.in_progress))
			hw_priv->frame_rcvd = 1;

		if (!memcmp(ieee80211_get_SA(frame), priv->join_bssid, ETH_ALEN)) {
			if (hw_priv->beacon)
				atbm_dev_kfree_skb(hw_priv->beacon);
			hw_priv->beacon = atbm_skb_copy(skb, GFP_ATOMIC);
			if (!hw_priv->beacon)
				printk("apollo: sched_scan: own beacon storing failed\n");
		}
	}
#endif /*ROAM_OFFLOAD*/
#ifdef CONFIG_PM

	/* Stay awake for 1sec. after frame is received to give
	 * userspace chance to react and acquire appropriate
	 * wakelock. */
	if (ieee80211_is_auth(frame->frame_control))
		grace_period = 5 * HZ;
	else if (ieee80211_is_deauth(frame->frame_control))
		grace_period = 5 * HZ;
	else if(ieee80211_is_data(frame->frame_control))
		grace_period = HZ/5;
	else 
		grace_period = 0;
	if (ieee80211_is_data(frame->frame_control))
		atbm_rx_h_ba_stat(priv, hdrlen, skb->len);
	if(grace_period != 0)
		atbm_pm_stay_awake(&hw_priv->pm_state, grace_period);
#endif

#ifdef ATBM_PKG_REORDER
	if(ieee80211_is_data_qos(frame->frame_control))
	{
		if((arg->link_id == 0)||(arg->link_id-1>=ATBM_APOLLO_LINK_ID_UNMAPPED))
		{
			printk("link_id err\n");
			goto direct_queue;
		}
		if(atbm_reorder_skb_queue(priv,skb,arg->link_id-1) == 0)
		{
			goto direct_queue;
		}
		else
		{
			*skb_p = NULL;
			return;
		}
	}
	else
	{
		
	}
direct_queue:
#endif

	//atbm_skb_trim(skb, skb->len);

#ifdef ATBM_P2P_CHANGE	
	atbm_parase_p2p_action_frame(priv,skb,false);
	atbm_parase_p2p_scan_resp(priv,skb);
#endif
	atbm_rx_udp(priv,skb);
	if (unlikely(early_data)) {
		spin_lock_bh(&priv->ps_state_lock);
		/* Double-check status with lock held */
		if (entry->status == ATBM_APOLLO_LINK_SOFT)
			atbm_skb_queue_tail(&entry->rx_queue, skb);
		else
			ieee80211_rx_irqsafe(priv->hw, skb);
		spin_unlock_bh(&priv->ps_state_lock);
	} else {
		ieee80211_rx_irqsafe(priv->hw, skb);
			
	}
	*skb_p = NULL;

	return;

drop:
	/* TODO: update failure counters */
	return;
}

/* ******************************************************************** */
/* Security								*/

int atbm_alloc_key(struct atbm_common *hw_priv)
{
	int idx;

	idx = ffs(~hw_priv->key_map) - 1;
	if (idx < 0 || idx > WSM_KEY_MAX_INDEX)
		return -1;

	hw_priv->key_map |= BIT(idx);
	hw_priv->keys[idx].entryIndex = idx;
	return idx;
}

void atbm_free_key(struct atbm_common *hw_priv, int idx)
{
	if((hw_priv->key_map & BIT(idx))){
		memset(&hw_priv->keys[idx], 0, sizeof(hw_priv->keys[idx]));
		hw_priv->key_map &= ~BIT(idx);
	}
}

void atbm_free_keys(struct atbm_common *hw_priv)
{
	memset(&hw_priv->keys, 0, sizeof(hw_priv->keys));
	hw_priv->key_map = 0;
}

int atbm_upload_keys(struct atbm_vif *priv)
{
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);
	int idx, ret = 0;

	for (idx = 0; idx <= WSM_KEY_MAX_IDX; ++idx)
		if (hw_priv->key_map & BIT(idx)) {
			ret = wsm_add_key(hw_priv, &hw_priv->keys[idx], priv->if_id);
			if (ret < 0)
				break;
		}
	return ret;
}
/* Workaround for WFD test case 6.1.10 */
void atbm_link_id_reset(struct work_struct *work)
{
	struct atbm_vif *priv =
		container_of(work, struct atbm_vif, linkid_reset_work);
	struct atbm_common *hw_priv = priv->hw_priv;
	
	//flush_workqueue(hw_priv->workqueue);
	if(atbm_bh_is_term(hw_priv)){
		return;
	}
	wsm_lock_tx_async(hw_priv);
	if (atbm_hw_priv_queue_work(hw_priv,
		       &priv->link_id_work) <= 0)
		wsm_unlock_tx(hw_priv);
}
