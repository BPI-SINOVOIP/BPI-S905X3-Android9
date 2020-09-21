/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SSV_SKB_H_
#define _SSV_SKB_H_ 
#include <linux/skbuff.h>
#include <ssv6xxx_common.h>
struct SKB_info_st
{
    struct ieee80211_sta *sta;
    u16 mpdu_retry_counter;
    unsigned long aggr_timestamp;
    u16 ampdu_tx_status;
    u16 ampdu_tx_final_retry_count;
    u16 lowest_rate;
    struct fw_rc_retry_params rates[SSV62XX_TX_MAX_RATES];
#ifdef CONFIG_DEBUG_SKB_TIMESTAMP
    ktime_t timestamp;
#endif
#ifdef MULTI_THREAD_ENCRYPT
    volatile u8 crypt_st;
#endif
    bool directly_ack;
	bool no_update_rpt;
};
typedef struct SKB_info_st SKB_info;
typedef struct SKB_info_st *p_SKB_info;
struct ampdu_hdr_st
{
    u32 first_sn;
    struct sk_buff_head mpdu_q;
    u32 max_size;
    u32 size;
    struct AMPDU_TID_st *ampdu_tid;
    struct ieee80211_sta *sta;
    u16 mpdu_num;
    u16 ssn[MAX_AGGR_NUM];
    struct fw_rc_retry_params rates[SSV62XX_TX_MAX_RATES];
};
struct sk_buff *ssv_skb_alloc(void *app_param, s32 len);
struct sk_buff *ssv_skb_alloc_ex(void *app_param, s32 len, gfp_t gfp_mask);
void ssv_skb_free(void *app_param, struct sk_buff *skb);
#endif
