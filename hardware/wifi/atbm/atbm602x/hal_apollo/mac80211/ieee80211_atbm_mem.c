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

#if (ATBM_ALLOC_MEM_DEBUG == 1)
struct ieee80211_atbm_mem
{
	struct list_head head;
	const char *call_addr;
	u8 mem[0] __attribute__((__aligned__(sizeof(void *))));
};
static struct list_head ieee80211_atbm_mem_list;
static spinlock_t ieee80211_atbm_mem_spin_lock;
static void *__ieee80211_atbm_kmalloc(size_t s, gfp_t gfp,const char *call_addr)
{
	struct ieee80211_atbm_mem *atbm_mem = NULL;
	void *p = NULL;
	unsigned long flags;
	
	atbm_mem = kmalloc(s+sizeof(struct ieee80211_atbm_mem),gfp);

	if(atbm_mem){
		p = (void*)atbm_mem->mem;
		atbm_mem->call_addr = call_addr;
		spin_lock_irqsave(&ieee80211_atbm_mem_spin_lock, flags);
		list_add_tail(&atbm_mem->head, &ieee80211_atbm_mem_list);
		spin_unlock_irqrestore(&ieee80211_atbm_mem_spin_lock, flags);
	}

	return p;
}
void *ieee80211_atbm_kmalloc(size_t s, gfp_t gfp,const char *func)
{
	void *p = __ieee80211_atbm_kmalloc(s, gfp,func);
	return p;
}

//void *ieee80211_atbm_kzalloc(size_t s, gfp_t gfp,void* call_addr)
void *ieee80211_atbm_kzalloc(size_t s, gfp_t gfp,const char *func)
{
	void *p = __ieee80211_atbm_kmalloc(s, gfp,func);
	if(p)
		memset(p, 0, s);
	return p;
}
void *ieee80211_atbm_kcalloc(size_t n, size_t size, gfp_t gfp,const char *func)
{
	if((n == 0) || (size==0))
		return NULL;
	return ieee80211_atbm_kzalloc(n*size, gfp | __GFP_ZERO,func);
}

void *ieee80211_atbm_krealloc(void *p, size_t new_size, gfp_t gfp,const char *func)
{
	struct ieee80211_atbm_mem *atbm_mem = NULL;
	struct ieee80211_atbm_mem *atbm_mem_new = NULL;
	void *p_new = NULL;
	unsigned long flags;
	
	if((p == NULL)||(new_size == 0))
		return NULL;

	atbm_mem = container_of(p, struct ieee80211_atbm_mem, mem);

	
	spin_lock_irqsave(&ieee80211_atbm_mem_spin_lock, flags);	
	list_del(&atbm_mem->head);
	spin_unlock_irqrestore(&ieee80211_atbm_mem_spin_lock, flags);

	atbm_mem_new = krealloc(atbm_mem,new_size+sizeof(struct ieee80211_atbm_mem),gfp);

	if(atbm_mem_new){
		p_new = (void*)atbm_mem_new->mem;
		atbm_mem_new->call_addr = func;
		spin_lock_irqsave(&ieee80211_atbm_mem_spin_lock, flags);		
		list_add_tail(&atbm_mem_new->head, &ieee80211_atbm_mem_list);
		spin_unlock_irqrestore(&ieee80211_atbm_mem_spin_lock, flags);
	}

	return p_new;
}

void ieee80211_atbm_kfree(void *p)
{
	struct ieee80211_atbm_mem *atbm_mem = NULL;
	unsigned long flags;
	
	if(p == NULL)
		return;
	atbm_mem = container_of(p, struct ieee80211_atbm_mem, mem);

	spin_lock_irqsave(&ieee80211_atbm_mem_spin_lock, flags);	
	list_del(&atbm_mem->head);
	spin_unlock_irqrestore(&ieee80211_atbm_mem_spin_lock, flags);

	kfree(atbm_mem);
}

void ieee80211_atbm_mem_exit(void)
{
	struct ieee80211_atbm_mem *atbm_mem = NULL;
	unsigned long flags;
	printk(KERN_ERR"ieee80211_atbm_mem_exit\n");
	spin_lock_irqsave(&ieee80211_atbm_mem_spin_lock, flags);
	while (!list_empty(&ieee80211_atbm_mem_list)) {
		atbm_mem = list_first_entry(
			&ieee80211_atbm_mem_list, struct ieee80211_atbm_mem, head);

		printk(KERN_ERR "%s:malloc addr(%s)\n",__func__,atbm_mem->call_addr);
		list_del(&atbm_mem->head);
		kfree(atbm_mem);
	}
	spin_unlock_irqrestore(&ieee80211_atbm_mem_spin_lock, flags);
}

void ieee80211_atbm_mem_int(void)
{
	spin_lock_init(&ieee80211_atbm_mem_spin_lock);
	INIT_LIST_HEAD(&ieee80211_atbm_mem_list);
}
#endif
