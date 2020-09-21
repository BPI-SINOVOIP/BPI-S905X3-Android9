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

#include <linux/kernel.h>
#include <linux/version.h>
#include "ssv_skb.h"
#include <ssv6200.h>
struct sk_buff *ssv_skb_alloc(void *app_param, s32 len)
{
    struct sk_buff *skb;
    skb = __dev_alloc_skb(len + SSV6200_ALLOC_RSVD , GFP_KERNEL);
    if (skb != NULL) {
        skb_reserve(skb, SSV_SKB_info_size);
    }
    return skb;
}
struct sk_buff *ssv_skb_alloc_ex(void *app_param, s32 len, gfp_t gfp_mask)
{
    struct sk_buff *skb;
    skb = __dev_alloc_skb(len + SSV6200_ALLOC_RSVD , gfp_mask);
    if (skb != NULL) {
        skb_reserve(skb, SSV_SKB_info_size);
    }
    return skb;
}
void ssv_skb_free(void *app_param, struct sk_buff *skb)
{
    dev_kfree_skb_any(skb);
}
