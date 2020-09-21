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
#include <net/ip.h>
#include "ieee80211_i.h"

#if (ATBM_ALLOC_SKB_DEBUG == 1)
#define IEEE80211_ATBM_SKB_HEAD_SIZE sizeof(struct ieee80211_atbm_skb_hdr)
#define IEEE80211_ATBM_CHECK_SKB(_skb)	BUG_ON((_skb->data-_skb->head<IEEE80211_ATBM_SKB_HEAD_SIZE)	\
	||(((struct ieee80211_atbm_skb_hdr*)_skb->head)->masker != ATBM_SKB_MASKER))

static struct list_head ieee80211_atbm_skb_list;
static spinlock_t ieee80211_atbm_skb_spin_lock;
#define ATBM_SKB_MASKER 0xAAFFFF55

void ieee80211_atbm_skb_exit(void)
{
	struct ieee80211_atbm_skb_hdr *atbm_hdr = NULL;
	unsigned long flags;
	printk(KERN_ERR"ieee80211_atbm_skb_exit\n");
	spin_lock_irqsave(&ieee80211_atbm_skb_spin_lock, flags);
	while (!list_empty(&ieee80211_atbm_skb_list)) {
		atbm_hdr = list_first_entry(
			&ieee80211_atbm_skb_list, struct ieee80211_atbm_skb_hdr, head);

		printk(KERN_ERR "%s:skb addr(%s)\n",__func__,atbm_hdr->call_addr);
		list_del(&atbm_hdr->head);
	}
	spin_unlock_irqrestore(&ieee80211_atbm_skb_spin_lock, flags);
}

void ieee80211_atbm_skb_int(void)
{
	spin_lock_init(&ieee80211_atbm_skb_spin_lock);
	INIT_LIST_HEAD(&ieee80211_atbm_skb_list);
}

static void ieee80211_atbm_add_skb_hdr_to_list(struct ieee80211_atbm_skb_hdr *atbm_skb_hdr,const char *func)
{
	unsigned long flags;
	BUG_ON(atbm_skb_hdr==NULL);
	memset(atbm_skb_hdr,0,sizeof(struct ieee80211_atbm_skb_hdr));
	atbm_skb_hdr->call_addr = func;
	atbm_skb_hdr->masker = ATBM_SKB_MASKER;
	spin_lock_irqsave(&ieee80211_atbm_skb_spin_lock, flags);
	list_add_tail(&atbm_skb_hdr->head, &ieee80211_atbm_skb_list);
	spin_unlock_irqrestore(&ieee80211_atbm_skb_spin_lock, flags);
	
}

static void ieee80211_atbm_remove_skb_hdr_from_list(struct ieee80211_atbm_skb_hdr *atbm_skb_hdr)
{
	unsigned long flags;
	
	if(atbm_skb_hdr->masker != ATBM_SKB_MASKER){
		return;
	}

	spin_lock_irqsave(&ieee80211_atbm_skb_spin_lock, flags);
	list_del(&atbm_skb_hdr->head);
	spin_unlock_irqrestore(&ieee80211_atbm_skb_spin_lock, flags);
	memset(atbm_skb_hdr,0,sizeof(struct ieee80211_atbm_skb_hdr));
	
}

void ieee80211_atbm_add_skb_to_debug_list(struct sk_buff *skb,const char *func)
{
	struct ieee80211_atbm_skb_hdr *atbm_hdr = (struct ieee80211_atbm_skb_hdr *)(skb->head);
	
	if((skb_headroom(skb) < IEEE80211_ATBM_SKB_HEAD_SIZE)
		||(atbm_hdr->masker == ATBM_SKB_MASKER)){		
		return;
	}

	ieee80211_atbm_add_skb_hdr_to_list(atbm_hdr,func);
}
struct sk_buff *__ieee80211_atbm_dev_alloc_skb(unsigned int length,gfp_t gfp_mask,const char *func)
{
	struct sk_buff * atbm_skb = NULL;
	
	atbm_skb = __dev_alloc_skb(length+IEEE80211_ATBM_SKB_HEAD_SIZE,gfp_mask);

	if(atbm_skb == NULL){
		return atbm_skb;
	}

	skb_reserve(atbm_skb, IEEE80211_ATBM_SKB_HEAD_SIZE);

	ieee80211_atbm_add_skb_hdr_to_list((struct ieee80211_atbm_skb_hdr *)(atbm_skb->head),func);
	return atbm_skb;
}

struct sk_buff *ieee80211_atbm_dev_alloc_skb(unsigned int length,const char *func)
{
	struct sk_buff * atbm_skb = NULL;
	
	atbm_skb = dev_alloc_skb(length+IEEE80211_ATBM_SKB_HEAD_SIZE);

	if(atbm_skb == NULL){
		return atbm_skb;
	}

	skb_reserve(atbm_skb, IEEE80211_ATBM_SKB_HEAD_SIZE);

	ieee80211_atbm_add_skb_hdr_to_list((struct ieee80211_atbm_skb_hdr *)(atbm_skb->head),func);
	return atbm_skb;
}
struct sk_buff *ieee80211_atbm_alloc_skb(unsigned int size,gfp_t priority,const char *func)
{
	struct sk_buff * atbm_skb = NULL;
	atbm_skb = alloc_skb(size+IEEE80211_ATBM_SKB_HEAD_SIZE,priority);

	if(atbm_skb){
		skb_reserve(atbm_skb, IEEE80211_ATBM_SKB_HEAD_SIZE);
		ieee80211_atbm_add_skb_hdr_to_list((struct ieee80211_atbm_skb_hdr *)(atbm_skb->head),func);
	}

	return atbm_skb;
}
void ieee80211_atbm_dev_kfree_skb_any(struct sk_buff *skb)
{
	struct ieee80211_atbm_skb_hdr *atbm_hdr;
	
	if(!skb){
		return;
	}

	atbm_hdr = (struct ieee80211_atbm_skb_hdr *)(skb->head);
	if((skb_headroom(skb)>=IEEE80211_ATBM_SKB_HEAD_SIZE)&&(atbm_hdr->masker == ATBM_SKB_MASKER))
		ieee80211_atbm_remove_skb_hdr_from_list(atbm_hdr);
	else {
		printk(KERN_ERR "%s:no atbm header\n",__func__);
	}
	dev_kfree_skb_any(skb);
}

void ieee80211_atbm_dev_kfree_skb(struct sk_buff *skb)
{
	struct ieee80211_atbm_skb_hdr *atbm_hdr;
	
	if(!skb){
		return;
	}

	atbm_hdr = (struct ieee80211_atbm_skb_hdr *)(skb->head);
	if((skb_headroom(skb)>=IEEE80211_ATBM_SKB_HEAD_SIZE)&&(atbm_hdr->masker == ATBM_SKB_MASKER))
		ieee80211_atbm_remove_skb_hdr_from_list(atbm_hdr);
	else {
		printk(KERN_ERR "%s:no atbm header\n",__func__);
	}
	dev_kfree_skb(skb);
}

void ieee80211_atbm_kfree_skb(struct sk_buff *skb)
{
	struct ieee80211_atbm_skb_hdr *atbm_hdr;
	
	if(!skb){
		return;
	}

	atbm_hdr = (struct ieee80211_atbm_skb_hdr *)(skb->head);
	if((skb_headroom(skb)>=IEEE80211_ATBM_SKB_HEAD_SIZE)&&(atbm_hdr->masker == ATBM_SKB_MASKER))
		ieee80211_atbm_remove_skb_hdr_from_list(atbm_hdr);
	else {
		printk(KERN_ERR "%s:no atbm header\n",__func__);
	}
	
	kfree_skb(skb);
}
void ieee80211_atbm_dev_kfree_skb_irq(struct sk_buff *skb)
{
	struct ieee80211_atbm_skb_hdr *atbm_hdr;
	
	if(!skb){
		return;
	}

	atbm_hdr = (struct ieee80211_atbm_skb_hdr *)(skb->head);
	if((skb_headroom(skb)>=IEEE80211_ATBM_SKB_HEAD_SIZE)&&(atbm_hdr->masker == ATBM_SKB_MASKER))
		ieee80211_atbm_remove_skb_hdr_from_list(atbm_hdr);
	else {
		printk(KERN_ERR "%s:no atbm header\n",__func__);
	}
	
	dev_kfree_skb_irq(skb);
}

void ieee80211_atbm_skb_reserve(struct sk_buff *skb, int len)
{
	skb_reserve(skb,len);
}

void ieee80211_atbm_skb_trim(struct sk_buff *skb, unsigned int len)
{
	skb_trim(skb,len);
}

unsigned char *ieee80211_atbm_skb_put(struct sk_buff *skb, unsigned int len)
{
	return skb_put(skb,len);
}

void ieee80211_atbm_skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
	skb_queue_tail(list,newsk);
}
struct sk_buff *__ieee80211_atbm_skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff * skb = __skb_dequeue(list);

	if(skb) IEEE80211_ATBM_CHECK_SKB(skb);

	return skb;
}

struct sk_buff *ieee80211_atbm_skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff * skb = skb_dequeue(list);
	
	if(skb) IEEE80211_ATBM_CHECK_SKB(skb);

	return skb;
}

void __ieee80211_atbm_skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb = __ieee80211_atbm_skb_dequeue(list)) != NULL)
		ieee80211_atbm_kfree_skb(skb);
}

void ieee80211_atbm_skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb = ieee80211_atbm_skb_dequeue(list)) != NULL){
		ieee80211_atbm_kfree_skb(skb);
	}
}

int ieee80211_atbm_skb_tailroom(const struct sk_buff *skb)
{
	return skb_tailroom(skb);
}

int ieee80211_atbm_skb_queue_empty(const struct sk_buff_head *list)
{
	return skb_queue_empty(list);
}

void ieee80211_atbm_skb_queue_splice_tail_init(struct sk_buff_head *list,
					      struct sk_buff_head *head)
{
	skb_queue_splice_tail_init(list,head);
}

void ieee80211_atbm_skb_queue_head_init(struct sk_buff_head *list)
{
	skb_queue_head_init(list);
}

void __ieee80211_atbm_skb_queue_tail(struct sk_buff_head *list,struct sk_buff *newsk,const char *func)
{
//	IEEE80211_ATBM_CHECK_SKB(newsk);
	struct ieee80211_atbm_skb_hdr *atbm_hdr = (struct ieee80211_atbm_skb_hdr *)(newsk->head);

	if((skb_headroom(newsk)<IEEE80211_ATBM_SKB_HEAD_SIZE)||(atbm_hdr->masker != ATBM_SKB_MASKER)){
		printk(KERN_ERR"%s:[%d][%lx][%s]\n",__func__,skb_headroom(newsk),atbm_hdr->masker,func);
		WARN_ON(1);
	}
	__skb_queue_tail(list,newsk);
}

__u32 ieee80211_atbm_skb_queue_len(const struct sk_buff_head *list_)
{
	return skb_queue_len(list_);
}

void __ieee80211_atbm_skb_queue_head_init(struct sk_buff_head *list)
{
	__skb_queue_head_init(list);
}
void __ieee80211_atbm_skb_queue_head(struct sk_buff_head *list,
				    struct sk_buff *newsk)
{
	IEEE80211_ATBM_CHECK_SKB(newsk);
	__skb_queue_head(list,newsk);
}
struct sk_buff *ieee80211_atbm_skb_copy(const struct sk_buff *skb, gfp_t gfp_mask,const char *func)
{
	struct sk_buff *new_skb = NULL;
	
	new_skb = skb_copy(skb,gfp_mask);

	if(new_skb)
		ieee80211_atbm_add_skb_hdr_to_list((struct ieee80211_atbm_skb_hdr *)(new_skb->head),func);

	return new_skb;
	
}
unsigned char *ieee80211_atbm_skb_pull(struct sk_buff *skb, unsigned int len)
{
	return skb_pull(skb,len);
}

unsigned char *ieee80211_atbm_skb_push(struct sk_buff *skb, unsigned int len)
{
	char *p;
	p = skb_push(skb,len);
	return p;
}
int ieee80211_atbm_dev_queue_xmit(struct sk_buff *skb)
{
	IEEE80211_ATBM_CHECK_SKB(skb);
	ieee80211_atbm_remove_skb_hdr_from_list((struct ieee80211_atbm_skb_hdr *)(skb->head));
	return dev_queue_xmit(skb);
}

int ieee80211_atbm_netif_rx_ni(struct sk_buff *skb)
{
	IEEE80211_ATBM_CHECK_SKB(skb);	
	ieee80211_atbm_remove_skb_hdr_from_list((struct ieee80211_atbm_skb_hdr *)(skb->head));
	return netif_rx_ni(skb);
}
void ieee80211_atbm_skb_set_queue_mapping(struct sk_buff *skb, u16 queue_mapping)
{
	skb_set_queue_mapping(skb,queue_mapping);
}

void ieee80211_atbm_skb_set_tail_pointer(struct sk_buff *skb, const int offset)
{
	skb_set_tail_pointer(skb,offset);
}
void ieee80211_atbm_skb_reset_tail_pointer(struct sk_buff *skb)
{
	skb_reset_tail_pointer(skb);
}
void __ieee80211_atbm_skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
	IEEE80211_ATBM_CHECK_SKB(skb);
	__skb_unlink(skb,list);
}

void ieee80211_atbm_skb_set_mac_header(struct sk_buff *skb, const int offset)
{
	skb_set_mac_header(skb,offset);
}
void ieee80211_atbm_skb_set_network_header(struct sk_buff *skb, int offset)
{
	skb_set_network_header(skb,offset);
}

void ieee80211_atbm_skb_set_transport_header(struct sk_buff *skb, int offset)
{
	skb_set_transport_header(skb,offset);
}

void ieee80211_atbm_skb_queue_splice(const struct sk_buff_head *list,
				    struct sk_buff_head *head)
{
	skb_queue_splice(list,head);
}

void ieee80211_atbm_skb_queue_splice_init(struct sk_buff_head *list,
					 struct sk_buff_head *head)
{
	skb_queue_splice_init(list,head);
}

int __ieee80211_atbm_pskb_trim(struct sk_buff *skb, unsigned int len)
{
	return __pskb_trim(skb,len);
}

int ieee80211_atbm_pskb_may_pull(struct sk_buff *skb, unsigned int len)
{
	return pskb_may_pull(skb,len);
}
unsigned int ieee80211_atbm_skb_headroom(const struct sk_buff *skb)
{
	if(skb_headroom(skb)<IEEE80211_ATBM_SKB_HEAD_SIZE){
		WARN_ON(1);
		return 0;
	}
	return skb_headroom(skb)-IEEE80211_ATBM_SKB_HEAD_SIZE;
}
int ieee80211_atbm_pskb_expand_head(struct sk_buff *skb, int nhead, int ntail,
		     gfp_t gfp_mask,const char *func)
{
	int ret = 0;
	ieee80211_atbm_remove_skb_hdr_from_list((struct ieee80211_atbm_skb_hdr *)(skb->head));
	ret = pskb_expand_head(skb,nhead+IEEE80211_ATBM_SKB_HEAD_SIZE,ntail,gfp_mask);

	if(ret == 0){
		ieee80211_atbm_add_skb_hdr_to_list((struct ieee80211_atbm_skb_hdr *)(skb->head),func);
	}

	return ret;
}

struct sk_buff *ieee80211_atbm_skb_copy_expand(const struct sk_buff *skb,
				int newheadroom, int newtailroom,
				gfp_t gfp_mask,const char *func)
{
	struct sk_buff *skb_copy = NULL;
	
	skb_copy = skb_copy_expand(skb,newheadroom+IEEE80211_ATBM_SKB_HEAD_SIZE,newtailroom,gfp_mask);

	if(skb_copy){
		ieee80211_atbm_add_skb_hdr_to_list((struct ieee80211_atbm_skb_hdr *)(skb_copy->head),func);
	}

	return skb_copy;
	
}
int ieee80211_atbm_skb_cloned(const struct sk_buff *skb)
{
	return skb_cloned(skb);
}
struct sk_buff *ieee80211_atbm_skb_clone(struct sk_buff *skb, gfp_t gfp_mask,const char *func)
{
	struct sk_buff *cloned_skb = NULL;

	cloned_skb = skb_clone(skb,gfp_mask);

	if(cloned_skb){		
		ieee80211_atbm_add_skb_hdr_to_list((struct ieee80211_atbm_skb_hdr *)(cloned_skb->head),func);
	}

	return cloned_skb;
}
int ieee80211_atbm_netif_rx(struct sk_buff *skb)
{	
	ieee80211_atbm_remove_skb_hdr_from_list((struct ieee80211_atbm_skb_hdr *)(skb->head));
	return netif_rx(skb);
}
int ieee80211_atbm_netif_receive_skb(struct sk_buff *skb)
{	
	ieee80211_atbm_remove_skb_hdr_from_list((struct ieee80211_atbm_skb_hdr *)(skb->head));
	return netif_receive_skb(skb);
}
int ieee80211_atbm_skb_linearize(struct sk_buff *skb)
{
	return skb_linearize(skb);
}

struct sk_buff *ieee80211_atbm_skb_peek(struct sk_buff_head *list_)
{
	return skb_peek(list_);
}

int ieee80211_atbm_skb_shared(const struct sk_buff *skb)
{
	return skb_shared(skb);
}

int ieee80211_atbm_skb_padto(struct sk_buff *skb, unsigned int len)
{
	return skb_padto(skb,len);
}

struct sk_buff *ieee80211_atbm_skb_get(struct sk_buff *skb)
{
	return skb_get(skb);
}
#endif
