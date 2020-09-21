/*
 ****************************************************************************************
 *
 * Copyright (C)  altobeam 2015-2018
 *
 * Project: wifi station bridge
 *
 * Description:
 *     wifi station bridge
 *
 *
 ****************************************************************************************
 */
#ifdef CONFIG_MAC80211_BRIDGE


#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/bitmap.h>
#include <linux/rcupdate.h>
#include <linux/export.h>
#include <net/net_namespace.h>
#include <net/ieee80211_radiotap.h>
#include <net/cfg80211.h>
#include <net/atbm_mac80211.h>
#include <asm/unaligned.h>
#include <linux/udp.h>
#include <net/ip.h>
	 
	 
#include "ieee80211_i.h"

#include <linux/if_arp.h>
#include <net/ip.h>
#include <net/ipx.h>
#include <linux/atalk.h>
#include <linux/udp.h>
#include <linux/if_pppox.h>

int my_mod_use = 0;

typedef struct list_head atbm_qentry;

enum
{
    BR0_POLICY_OPEN     = 0,
    BR0_POLICY_ALLOW    = 1,
    BR0_POLICY_DENY     = 2,
};

#define BR0_HASHSIZE    32
#define NETINFO__MAX_LEN		12
struct br0_info{
	u8   macaddr[6];
	u8   ipaddr[4];
};
struct NET_BR0_INFO_ENTRY
{
    atbm_qentry   br0_list;
    atbm_qentry       br0_hash;
	unsigned long ageing_timer;
	union {
    	u8     br0_netinfo[NETINFO__MAX_LEN];
 		struct br0_info info;
	}u;
};
struct NETWIFI_S_BRIDGE
{
	spinlock_t						br_ext_lock;
    int         					as_policy;
    atbm_qentry   					as_list;
    atbm_qentry       				as_hash[BR0_HASHSIZE];
	unsigned long 					ageing_timer;

	unsigned char					fast_mac[6];
	unsigned char					fast_ip[4];
	struct NET_BR0_INFO_ENTRY		*fast_entry;
	unsigned char					br_mac[6];
	unsigned char					br_ip[4];

};



static  void br0_free_all_locked(struct NETWIFI_S_BRIDGE *);
static int br0_free_all(struct ieee80211_sub_if_data *sdata);

void br0_info_expire(struct ieee80211_sub_if_data *sdata);

void frame_hexdump(char *prefix, u8 *data, int len);

#if defined(CONFIG_ATBM_APOLLO_BR0_DEBUG)
#define br_printk  printk
#else
#define br_printk(...)
#endif
/**
 * br0_en_queue - add a new atbm_qentry in queue tail
 * @list: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new atbm_qentry in queue tail.
 */
static inline void br0_en_queue(atbm_qentry *list, atbm_qentry *head)
{
    list_add_tail(list, head);
}


static inline void br0_en_queue_head(atbm_qentry *list, atbm_qentry *head)
{
    list_add(list, head);
}
/**
 * br0_de_queue - deletes atbm_qentry from queue and reinitialize it.
 * @list: the element to delete from the queue.
 */

static inline void br0_de_queue(atbm_qentry *list)
{
    list_del_init(list);
}



/**
 * br0_queue_empty - tests whether a queue is empty
 * @head: the head of the queue.
 */
static inline int br0_queue_empty(atbm_qentry *head)
{
    return list_empty(head);
}


#define BR0_LOCK_DESTROY(_as)
#define BR0_LOCK(_as)           spin_lock_bh(&(_as)->br_ext_lock)
#define BR0_UNLOCK(_as)         spin_unlock_bh(&(_as)->br_ext_lock)


int br0_attach(struct ieee80211_sub_if_data *sdata)
{
    struct NETWIFI_S_BRIDGE *br0_priv;
    int Num=0;

    br0_priv= atbm_kmalloc(sizeof(struct NETWIFI_S_BRIDGE),GFP_KERNEL); 
    if (br0_priv == NULL)
    {
    	printk("ERROR br0_attach\n\n");
        return 0;
    }
	memset(br0_priv,0,sizeof(struct NETWIFI_S_BRIDGE));
	spin_lock_init(&br0_priv->br_ext_lock);
    INIT_LIST_HEAD(&br0_priv->as_list);
    for (Num = 0; Num < BR0_HASHSIZE; Num++)
    {
        INIT_LIST_HEAD(&br0_priv->as_hash[Num]);
    }
	br0_priv->ageing_timer=jiffies;
    br0_priv->as_policy = BR0_POLICY_OPEN;
    sdata->bridge_priv = br0_priv;
    return 1;
}

void br0_detach(struct ieee80211_sub_if_data *sdata)
{
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;

	if(br0_priv == NULL)
		return;

    br0_free_all(sdata);
    BR0_LOCK_DESTROY(br0_priv);
    atbm_kfree(sdata->bridge_priv);
    sdata->bridge_priv = NULL;

}

static  int BR0_HASH(const u8 *ipaddr)
{

		unsigned long x;

		x = ipaddr[3]  ^ ipaddr[2];

		return x % BR0_HASHSIZE;

}



static __inline struct NET_BR0_INFO_ENTRY *
_br0_find_netinfo(struct NETWIFI_S_BRIDGE *br0_priv, const u8 *macaddr,const u8 *ipaddr)
{
    struct NET_BR0_INFO_ENTRY *br_info;
    int hash;

    hash = BR0_HASH(ipaddr);
    list_for_each_entry(br_info, &br0_priv->as_hash[hash], br0_hash)
    {
        if(memcmp(br_info->u.info.macaddr, macaddr,4)==0)
            return br_info;
    }
    return NULL;
}
static __inline struct NET_BR0_INFO_ENTRY *
_br0_find_netinfo_ip(struct NETWIFI_S_BRIDGE *br0_priv, const u8 *ipaddr)
{
    struct NET_BR0_INFO_ENTRY *br_info;
    int hash;

    hash = BR0_HASH(ipaddr);
    list_for_each_entry(br_info, &br0_priv->as_hash[hash], br0_hash)
    {
        if(memcmp(br_info->u.info.ipaddr, ipaddr,4)==0)
            return br_info;
    }
    return NULL;
}

static void
_br0_free(struct NETWIFI_S_BRIDGE *br0_priv, struct NET_BR0_INFO_ENTRY *br_info)
{
   // BR0_LOCK_ASSERT(br0_priv);

    br0_de_queue(&br_info->br0_list);
    br0_de_queue(&br_info->br0_hash);
//        LIST_REMOVE(NET_BR0_INFO_ENTRY, br0_hash);
    atbm_kfree(br_info);
}



static int
br0_add(struct ieee80211_sub_if_data *sdata, const u8* mac,const u8 *ipaddr)
{
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;
    struct NET_BR0_INFO_ENTRY *br_info, *new;
    int hash;

	
	br0_info_expire(sdata);
    new= atbm_kmalloc(sizeof(struct NET_BR0_INFO_ENTRY),GFP_KERNEL); 
    if (new == NULL)
    {
    	printk("ERROR br0_add\n\n");
        return 0;
    }

	memset(new,0,sizeof(struct NET_BR0_INFO_ENTRY));

    BR0_LOCK(br0_priv);
    hash = BR0_HASH(ipaddr);
    list_for_each_entry(br_info, &br0_priv->as_hash[hash], br0_hash)
    {
        if (memcmp(br_info->u.info.ipaddr, ipaddr,4)==0)
        {
			br_info->ageing_timer = jiffies;
			memcpy(br_info->u.info.macaddr,mac,6);
            BR0_UNLOCK(br0_priv);
            atbm_kfree(new);
            return EEXIST;
        }
    }
	br_printk("atbmBr0:add new node %x:%x:%x:%x:%x:%x, ip %d.%d.%d.%d\n",
									mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],ipaddr[0],ipaddr[1],ipaddr[2],ipaddr[3]);
    memcpy(new->u.info.macaddr,mac,6);
    memcpy(new->u.info.ipaddr,ipaddr,4);
    br0_en_queue(&new->br0_list, &br0_priv->as_list);

    br0_en_queue_head(&new->br0_hash, &br0_priv->as_hash[hash]);
    BR0_UNLOCK(br0_priv);

    return 0;
}

static int
br0_remove(struct ieee80211_sub_if_data *sdata, const u8 *ipaddr)
{
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;
    struct NET_BR0_INFO_ENTRY *br_info;

    BR0_LOCK(br0_priv);
    br_info = _br0_find_netinfo_ip(br0_priv, ipaddr);
    if (br_info != NULL)
        _br0_free(br0_priv, br_info);
    BR0_UNLOCK(br0_priv);

    return (br_info == NULL ? ENOENT : 0);
}

static void
br0_free_all_locked(struct NETWIFI_S_BRIDGE *br0_priv)
{
    struct NET_BR0_INFO_ENTRY *br_info;

    BR0_LOCK(br0_priv);
    while (!br0_queue_empty(&br0_priv->as_list))
    {
        br_info = list_first_entry(&br0_priv->as_list, struct NET_BR0_INFO_ENTRY, br0_list);
        _br0_free(br0_priv, br_info);
    }
	br0_priv->fast_entry = NULL;
	
	memset(br0_priv->fast_ip,0,4);
	memset(br0_priv->fast_mac,0,6);
    BR0_UNLOCK(br0_priv);
}

static int
br0_free_all(struct ieee80211_sub_if_data *sdata)
{

    br0_free_all_locked(sdata->bridge_priv);

    return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#define __vlan_hdr_del() \
{\
	struct ethhdr *ehdr = (struct ethhdr *)skb->data; \
	if (ehdr->h_proto == __constant_htons(ETH_P_8021Q)) { \
		b_vlan_frame = 1;									\
		vlan_hdr = *((unsigned short *)(&ehdr[1]));			\
		memmove(skb->data+4,skb->data,ETH_ALEN*2);			\
		atbm_skb_pull(skb, 4);									\
		ehdr = (struct ethhdr *)skb->data;					\
		}	\
}


#define __vlan_hdr_add() \
{ \
	if (b_vlan_frame) {\
		atbm_skb_push(skb, 4);\
		ehdr = (struct ethhdr *)skb->data;			\
		memmove(skb->data,skb->data+4,ETH_ALEN*2);		\
		ehdr->h_proto = __constant_htons(ETH_P_8021Q);\
		*((unsigned short *)(&ehdr[1]))= vlan_hdr;\
	}\
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static  unsigned long __br0_timeout()
{
	unsigned long timeout;

	timeout = jiffies - NET_BR0_AGEING_TIME*HZ;

	return timeout;
}
static  int  __br0_has_expired(struct NET_BR0_INFO_ENTRY * fdb)
{
	if(time_before_eq(fdb->ageing_timer, __br0_timeout()))
		return 1;

	return 0;
}

void br0_info_expire(struct ieee80211_sub_if_data *sdata)
{
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;
    struct NET_BR0_INFO_ENTRY *br_info,*br_info_next;
	if(time_before_eq(br0_priv->ageing_timer, __br0_timeout())==0){
		return;
	}

    BR0_LOCK(br0_priv);
    list_for_each_entry_safe(br_info,br_info_next, &br0_priv->as_list, br0_list)
    {
		if(__br0_has_expired(br_info)){
			u8 *mac = br_info->u.info.macaddr;
			u8 *ipaddr = br_info->u.info.ipaddr;
			printk("atbmBr0:<WARNING> br0_info_expire %x:%x:%x:%x:%x:%x, ip %d.%d.%d.%d\n",
									mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],ipaddr[0],ipaddr[1],ipaddr[2],ipaddr[3]);
			_br0_free(br0_priv, br_info);
			if(br_info == br0_priv->fast_entry){
				br0_priv->fast_entry = NULL;				
				memset(br0_priv->fast_ip,0,4);
				memset(br0_priv->fast_mac,0,6);
			}
		}
    }
    BR0_UNLOCK(br0_priv);
	br0_priv->ageing_timer = jiffies;
}

static int ieee80211_brigde_network_find_and_replace(struct ieee80211_sub_if_data *sdata,
				struct sk_buff *skb, unsigned char *ipaddr)
{
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;
	struct NET_BR0_INFO_ENTRY * entry;
	int ret =0;

    BR0_LOCK(br0_priv);
	entry = _br0_find_netinfo_ip(br0_priv,ipaddr);
	if(entry)
	{
		if(!__br0_has_expired(entry))
		{
			// replace the destination mac address
			memcpy(skb->data, entry->u.info.macaddr, ETH_ALEN);
			
			br_printk("atbmBr0:rx change node [%x:%x:%x:%x:%x:%x]->[%x:%x:%x:%x:%x:%x], ip %d.%d.%d.%d\n",
				skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],				
											 entry->u.info.macaddr[0], 
											 entry->u.info.macaddr[1], 
											 entry->u.info.macaddr[2], 
											 entry->u.info.macaddr[3], 
											 entry->u.info.macaddr[4], 
											 entry->u.info.macaddr[5],
											 ipaddr[0],
											 ipaddr[1],
											 ipaddr[2],
											 ipaddr[3]);
			entry->ageing_timer = jiffies;
		}
		else {
			printk("<WARNING>__br0_has_expired\n");	
		}
		ret = 1;
	}
    BR0_UNLOCK(br0_priv);
	return ret;
}

#define ieee80211_brigde_network_insert br0_add



int __ieee80211_brigde_change_rxhdr(struct ieee80211_sub_if_data *sdata, struct sk_buff *skb)
{
	struct NETWIFI_S_BRIDGE *priv=sdata->bridge_priv;
	struct ethhdr *ehdr = (struct ethhdr *)skb->data;

	if(skb == NULL)
		return -1;


	switch(ehdr->h_proto ){

		case   __constant_htons(ETH_P_IP):
		{
			struct iphdr* iph = (struct iphdr *)(ehdr +1);

			if (!ieee80211_brigde_network_find_and_replace(sdata, skb,(unsigned char *) &iph->daddr)) {
				if (*((unsigned char *)&iph->daddr + 3) == 0xff) {
					// L2 is unicast but L3 is broadcast, make L2 bacome broadcast
					memset(skb->data, 0xff, ETH_ALEN);
				}
				else {
					u8 * ipaddr = (unsigned char *) &iph->daddr;
					// forward unknow IP packet to upper TCP/IP
					br_printk("atbm_br0: Replace DA with BR's MAC [%x:%x:%x:%x:%x:%x]->[%x:%x:%x:%x:%x:%x], ip %d.%d.%d.%d\n",
						skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],				
											 priv->br_mac[0], 
											 priv->br_mac[1], 
											 priv->br_mac[2], 
											 priv->br_mac[3], 
											 priv->br_mac[4], 
											 priv->br_mac[5],
											 ipaddr[0],
											 ipaddr[1],
											 ipaddr[2],
											 ipaddr[3]);
					
					memcpy(skb->data, priv->br_mac, ETH_ALEN);			

				}
			}
			break;
		}
		case   __constant_htons(ETH_P_ARP):
		{
			struct arphdr *arp = (struct arphdr *)(ehdr +1);
			__be32 src_ipaddr, tgt_ipaddr;
			char *src_devaddr, *tgt_devaddr;
			const char *arpptr = (char *)(arp + 1);
			
			src_devaddr = arpptr;
			arpptr += ETH_ALEN;
			memcpy(&src_ipaddr, arpptr, sizeof(u32));
			arpptr += sizeof(u32);
			tgt_devaddr = arpptr;
			arpptr += ETH_ALEN;
			memcpy(&tgt_ipaddr, arpptr, sizeof(u32));


			ieee80211_brigde_network_find_and_replace(sdata, skb,(unsigned char *) &tgt_ipaddr);
			//if(memcmp(tgt_devaddr, skb->data, ETH_ALEN)){
				//record sourc
				//frame_hexdump("\nrx beforce replace ARP:", ((char *)(arp + 1))-2,22);
				// change to ARP target mac address to Lookup result
				memcpy(tgt_devaddr, skb->data, ETH_ALEN);
			//	frame_hexdump("\nrx after replace ARP:", ((char *)(arp + 1))-2,22);
			//}

			break;
		}

		default:
			break;
	}
	
	return 0;
}

int ieee80211_brigde_hash_update(struct ieee80211_sub_if_data *sdata, struct sk_buff *skb)
{
	struct ethhdr *ehdr = (struct ethhdr *)skb->data;

	if(skb == NULL)
		return -1;


	switch(ehdr->h_proto ){

		case   __constant_htons(ETH_P_IP):
		{
			struct iphdr* iph = (struct iphdr *)(ehdr +1);

			if(((unsigned char*)(iph) + (iph->ihl<<2)) >= (skb->data + ETH_HLEN + skb->len))
			{
				printk(" IP packet len error!!\n");
				return -1;
			}


			//some muticast with source IP is all zero, maybe other case is illegal
			//in class A, B, C, host address is all zero or all one is illegal
			if (iph->saddr == 0)
				break;
			//record source IP address and , source mac address into db
			ieee80211_brigde_network_insert(sdata, skb->data+ETH_ALEN, &iph->saddr);

			break;			
		}

		case   __constant_htons(ETH_P_ARP):
		{
			struct arphdr *arp = (struct arphdr *)(ehdr +1);
			__be32 src_ipaddr, tgt_ipaddr;
			char *src_devaddr, *tgt_devaddr;
			const char *arpptr = (char *)(arp + 1);
			
			src_devaddr = arpptr;
			arpptr += ETH_ALEN;
			memcpy(&src_ipaddr, arpptr, sizeof(u32));
			arpptr += sizeof(u32);
			tgt_devaddr = arpptr;
			arpptr += ETH_ALEN;
			memcpy(&tgt_ipaddr, arpptr, sizeof(u32));

			if(arp->ar_pro != __constant_htons(ETH_P_IP))
			{
				printk("BR0: arp protocol unknown (%4x)!\n", htons(arp->ar_pro));
				return -1;
			}
			//some muticast with source IP is all zero, 
			if (src_ipaddr == 0)
				break;
			//if(memcmp(src_devaddr,NETDEV_HWADDR(sdata),6)){
				//record sourc
				//frame_hexdump("\nbeforce replace ARP:", ((char *)(arp + 1))-2,22);
				// change to ARP sender mac address to wlan STA address
		        memcpy(src_devaddr, NETDEV_HWADDR(sdata), ETH_ALEN);	
				//frame_hexdump("\nafter replace ARP:", ((char *)(arp + 1))-2,22);
			//}
			ieee80211_brigde_network_insert(sdata, skb->data+ETH_ALEN, &src_ipaddr);

			break;
		}

		default:
			break;
	}
	
	return 0;
}

int ieee80211_brigde_change_rxhdr(struct ieee80211_sub_if_data *sdata, struct sk_buff *skb)
{
	struct ethhdr *ehdr = (struct ethhdr *)skb->data;
	int b_vlan_frame=0;
	int ret=0;
	u16 vlan_hdr=0;
	void *br_port = NULL;
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;
	int need_look=1;
	
	if(br0_priv==0)
		return -2;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
	br_port =sdata->dev->br_port;
#else   // (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
	rcu_read_lock();
	br_port = rcu_dereference(sdata->dev->rx_handler_data);
	rcu_read_unlock();
#endif  // (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))

	if(br_port &&
		( (*(u32 *)br0_priv->br_mac) == 0 && (*(u16 *)(br0_priv->br_mac+4)) == 0 )) {
		printk("Re-init br0_netdev_open() due to br_mac==0!\n");
		br0_netdev_open(sdata->dev);
	}

	if( br_port ){

		if((sdata->vif.type == NL80211_IFTYPE_STATION) &&	
			(!is_multicast_ether_addr(ehdr->h_dest)))
		{

				__vlan_hdr_del();
				
				/*
				 *	This function look up the destination network address from
				 *	the NAT2.5 database. Return value = -1 means that the
				 *	corresponding network protocol is NOT support.
				 */
				if (ehdr->h_proto == __constant_htons(ETH_P_IP)){
					struct iphdr* iph = (struct iphdr *)(ehdr +1);
					if(br0_priv->fast_entry && 
						!memcmp(br0_priv->fast_ip, &iph->daddr, 4)){ 
						memcpy(skb->data, br0_priv->fast_mac, ETH_ALEN);
						br0_priv->fast_entry->ageing_timer = jiffies;
						need_look = 0;			
					}
				}	
				if(need_look) {
					ret = __ieee80211_brigde_change_rxhdr(sdata, skb);
				}
				
				__vlan_hdr_add();
				
		}
	}

	///

	if(memcmp(br0_priv->br_mac,ehdr->h_source,6)==0){

		{	
			struct ieee80211_local *local = sdata->local;
			struct ieee80211_sub_if_data *deliver_sdata;
			struct ethhdr *ehdr_deliver = NULL;
			struct sk_buff * deliver_skb = NULL;

			list_for_each_entry_rcu(deliver_sdata, &local->interfaces, list) {
				if (!ieee80211_sdata_running(deliver_sdata))
					continue;
				if (deliver_sdata==sdata)
					continue;
				if ((deliver_sdata->vif.type != NL80211_IFTYPE_AP)&&
					(deliver_sdata->vif.type != NL80211_IFTYPE_STATION)){
					continue;
				}
			
				/*
				 * send multicast frames both to higher layers in
				 * local net stack and back to the wireless medium
				 */
				deliver_skb = atbm_skb_copy(skb, GFP_ATOMIC);
				deliver_skb->dev = deliver_sdata->dev;
				ehdr_deliver = (struct ethhdr *) deliver_skb->data;
				memcpy(ehdr_deliver->h_source,deliver_sdata->vif.addr,6);
				if (!deliver_skb && net_ratelimit())
					printk(KERN_DEBUG "%s: failed to clone "
					       "multicast frame\n", deliver_sdata->name);
				break;
			}	
			if (deliver_skb) {
				/* send to wireless media */
				deliver_skb->protocol = htons(ETH_P_802_3);
				skb_reset_network_header(deliver_skb);
				skb_reset_mac_header(deliver_skb);
				//printk("deliver skb iftype22 %d \n",deliver_sdata->vif.type);
				atbm_dev_queue_xmit(deliver_skb);
			}
		}

		
		printk("BR0:  sourceHw==br0Hw (%2x:%2x)!h_proto %x,iftype %d\n",br0_priv->br_mac[4],br0_priv->br_mac[5],ehdr->h_proto ,sdata->vif.type);
		//frame_hexdump("dump:",ehdr,16);
		return -1;
	}
	return ret;
}



void *ieee80211_brigde_updata_fast_info(struct ieee80211_sub_if_data *sdata,unsigned char *mac,unsigned char *ipaddr)
{
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;
	struct NET_BR0_INFO_ENTRY * entry;


	entry = _br0_find_netinfo_ip(br0_priv,ipaddr);

	if(entry){
		//if ip addr is same ,but mac addr is not the same ,return NULL;
		if(memcmp(mac,entry->u.info.macaddr,6))
			entry=NULL;
	}
	return entry;
}
/////////////////////////////////////////////////////////////////////////
//#include "dhcpd.h"

#define DHCP_MAGIC			0x63825363
#define BROADCAST_FLAG		0x8000
struct dhcpMessage {
	u_int8_t op;
	u_int8_t htype;
	u_int8_t hlen;
	u_int8_t hops;
	u_int32_t xid;
	u_int16_t secs;
	u_int16_t flags;
	u_int32_t ciaddr;
	u_int32_t yiaddr;
	u_int32_t siaddr;
	u_int32_t giaddr;
	u_int8_t chaddr[16];
	u_int8_t sname[64];
	u_int8_t file[128];
	u_int32_t cookie;
	u_int8_t options[308]; /* 312 - cookie */
};
int ieee80211_is_dhcp_frame(struct sk_buff *skb);

void ieee80211_tx_set_dhcp_bcast_flag(struct ieee80211_sub_if_data *sdata, struct sk_buff *skb)
{
	//struct ethhdr *ehdr = (struct ethhdr *)skb->data;
	if(skb == NULL)
		return;


	if(ieee80211_is_dhcp_frame(skb)) // DHCP request
	{
		struct iphdr* iph = (struct iphdr *)(skb->data + ETH_HLEN);
		struct udphdr *udph = (struct udphdr *)((u8 *)iph + (iph->ihl << 2));
		struct dhcpMessage *dhcph =	(struct dhcpMessage *)((u8 *)udph + sizeof(struct udphdr));

		if(dhcph->cookie == __constant_htonl(DHCP_MAGIC)) // match magic word
		{
			if(!(dhcph->flags & htons(BROADCAST_FLAG))) // if not broadcast
			{
				register int sum = 0;

				dhcph->flags |= htons(BROADCAST_FLAG);
				// recalculate checksum
				sum = ~(udph->check) & 0xffff;
				sum += dhcph->flags;
				while(sum >> 16)
					sum = (sum & 0xffff) + (sum >> 16);
				udph->check = ~sum;
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////
int ieee80211_brigde_change_txhdr(struct ieee80211_sub_if_data *sdata, struct sk_buff *skb)
{
	struct net_device *dev = sdata->dev;
	struct ethhdr *ehdr = (struct ethhdr *)skb->data;
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;
	void *br_port = NULL;
	u16 vlan_hdr=0;
	int b_vlan_frame = 0;
	int need_insert =1;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
	br_port = dev->br_port;
#else   // (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
	rcu_read_lock();
	br_port = rcu_dereference(dev->rx_handler_data);
	rcu_read_unlock();
#endif  // (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))

	if(br_port)
	{
		if(br0_priv ==NULL)
			return 0;
	     /*
			printk("tx ehdr->h_source %x:%x:%x:%x:%x:%x h_proto %x (%x %x)\n",ehdr->h_source[0],
														ehdr->h_source[1],
														ehdr->h_source[2],
														ehdr->h_source[3],
														ehdr->h_source[4],
														ehdr->h_source[5],ehdr->h_proto ,ETH_P_8021Q,ETH_P_IP
														);
			
			printk("tx if %x:%x:%x:%x:%x:\n",!is_multicast_ether_addr(ehdr->h_dest),
				memcmp(ehdr->h_source, br0_priv->br_mac, ETH_ALEN) ,
				!memcmp(ehdr->h_source, br0_priv->fast_mac,ETH_ALEN) ,
				ehdr->h_proto != __constant_htons(ETH_P_8021Q) ,
				br0_priv->fast_entry);

		*/
		
		ehdr = (struct ethhdr *)skb->data;

#if 0
		if ((is_multicast_ether_addr(ehdr->h_dest))||
			(memcmp(ehdr->h_source, br0_priv->br_mac, ETH_ALEN) &&
			memcmp(ehdr->h_source, br0_priv->fast_mac,ETH_ALEN)) ||
			/*ehdr->h_proto != __constant_htons(ETH_P_IP) ||*/
			!br0_priv->fast_entry) 
#endif//0
		{			
			__vlan_hdr_del();

			if (ehdr->h_proto == __constant_htons(ETH_P_IP)) {
				struct iphdr* iph = (struct iphdr *)(ehdr +1);
				if((memcmp(ehdr->h_source, br0_priv->fast_mac,ETH_ALEN)==0) 
					&&(memcmp(br0_priv->fast_ip, &iph->saddr, 4)==0)){
					if (br0_priv->fast_entry) {
						br0_priv->fast_entry->ageing_timer = jiffies;
						need_insert = 0;
					}
					else {
						memset(br0_priv->fast_mac, 0, ETH_ALEN);
						memset(br0_priv->fast_ip, 0, 4);
					}
				}
				else {
					br0_priv->fast_entry = (struct NET_BR0_INFO_ENTRY *)ieee80211_brigde_updata_fast_info(sdata,ehdr->h_source,&iph->saddr);
					if (br0_priv->fast_entry != NULL) {
						memcpy(br0_priv->fast_mac, ehdr->h_source, ETH_ALEN);
						memcpy(br0_priv->fast_ip, &iph->saddr, 4);
						br0_priv->fast_entry->ageing_timer = jiffies;
						br_printk("atbm_br0: fast_mac [%x:%x:%x:%x:%x:%x], ip %d.%d.%d.%d\n",			
																br0_priv->fast_mac[0], 
																br0_priv->fast_mac[1], 
																br0_priv->fast_mac[2], 
																br0_priv->fast_mac[3], 
																br0_priv->fast_mac[4], 
																br0_priv->fast_mac[5],
																br0_priv->fast_ip[0],
																br0_priv->fast_ip[1],
																br0_priv->fast_ip[2],
																br0_priv->fast_ip[3]);
						need_insert = 0;							
					}
				}
			}

			if(need_insert){
				if(ieee80211_brigde_hash_update(sdata, skb) <0) {
					printk("TX DROP: ieee80211_brigde_change_txhdr fail!\n");
					return -1;

				}
			}
			__vlan_hdr_add();

		}
		//if SA == br_mac && skb== IP  => copy SIP to br_ip 
		if (!memcmp(ehdr->h_source, br0_priv->br_mac, ETH_ALEN) &&
			(ehdr->h_proto == __constant_htons(ETH_P_IP)))
			memcpy(br0_priv->br_ip, skb->data+WLAN_ETHHDR_LEN+12, 4);

		if(memcmp(ehdr->h_dest,sdata->dev->dev_addr,6)==0){
			//frame_hexdump("BR0:DA error,drop\n",ehdr,14);
			printk("may error!\n");
			return 0;
		}
		//change source mac to station macaddr
		memcpy(ehdr->h_source, sdata->dev->dev_addr, ETH_ALEN);
		
		ieee80211_tx_set_dhcp_bcast_flag(sdata, skb);
	}

	return 0;
}

void ieee80211_brigde_flush(struct ieee80211_sub_if_data *sdata)
{
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;

	if(br0_priv ==NULL)
		return;

    br0_free_all_locked(br0_priv);

}
#define CONFIG_BR_EXT_BRNAME "br0"

void br0_netdev_open(struct net_device *netdev)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(netdev);
    struct NETWIFI_S_BRIDGE *br0_priv = sdata->bridge_priv;

	if(br0_priv ==NULL)
		return;
	
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	rcu_read_lock();
#endif	// (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))

	{
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
		if (netdev->br_port) 
#else   // (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
		if (rcu_dereference(netdev->rx_handler_data))
#endif  // (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
		{
			struct net_device *br_netdev;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
			br_netdev = dev_get_by_name(CONFIG_BR_EXT_BRNAME);
#else	// (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
			struct net *devnet = dev_net(netdev);
			br_netdev = dev_get_by_name(devnet, CONFIG_BR_EXT_BRNAME);
#endif	// (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))

			if (br_netdev) {
				memcpy(br0_priv->br_mac, br_netdev->dev_addr, ETH_ALEN);
				dev_put(br_netdev);
			} else {
				printk("%s()-%d: dev_get_by_name(%s) failed!", __FUNCTION__, __LINE__, CONFIG_BR_EXT_BRNAME);
			}
		}
		else {
			printk("%s()-%d: dev_get_by_name(%s) failed2!", __FUNCTION__, __LINE__, CONFIG_BR_EXT_BRNAME);
		}
		
	}//

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	rcu_read_unlock();
#endif	// (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
}

#endif  //CONFIG_MAC80211_BRIDGE
