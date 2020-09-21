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

#ifndef _SSV6200_H_
#define _SSV6200_H_ 
#include <linux/device.h>
#include <linux/interrupt.h>
#ifdef SSV_MAC80211
#include "ssv_mac80211.h"
#else
#include <net/mac80211.h>
#endif
#ifdef ECLIPSE
#include <ssv_mod_conf.h>
#endif
#ifndef SSV_SUPPORT_HAL
#include <ssv6200_reg.h>
#include <ssv6200_aux.h>
#endif
#include <hwif/hwif.h>
#include <hci/ssv_hci.h>
#include "ssv6200_common.h"
#ifdef SSV6200_ECO
#define SSV6200_TOTAL_PAGE (256)
#define SSV6200_TOTAL_ID (128)
#ifndef HUW_DRV
#define SSV6200_ID_TX_THRESHOLD 19
#define SSV6200_ID_RX_THRESHOLD 60
#define SSV6200_RESERVED_PAGE (26)
#define SSV6200_PAGE_TX_THRESHOLD 115
#define SSV6200_PAGE_RX_THRESHOLD (SSV6200_TOTAL_PAGE - SSV6200_PAGE_TX_THRESHOLD - SSV6200_RESERVED_PAGE)
#define SSV6200_AMPDU_DIVIDER (2)
#define SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER (SSV6200_PAGE_TX_THRESHOLD - (SSV6200_PAGE_TX_THRESHOLD/SSV6200_AMPDU_DIVIDER))
#define SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER 2
#else
#undef SSV6200_ID_TX_THRESHOLD
#undef SSV6200_ID_RX_THRESHOLD
#undef SSV6200_PAGE_TX_THRESHOLD
#undef SSV6200_PAGE_RX_THRESHOLD
#undef SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER
#undef SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER
#define SSV6200_ID_TX_THRESHOLD 31
#define SSV6200_ID_RX_THRESHOLD 31
#define SSV6200_PAGE_TX_THRESHOLD 61
#define SSV6200_PAGE_RX_THRESHOLD 61
#define SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER 45
#define SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER 2
#endif
#else
#undef SSV6200_ID_TX_THRESHOLD
#undef SSV6200_ID_RX_THRESHOLD
#undef SSV6200_PAGE_TX_THRESHOLD
#undef SSV6200_PAGE_RX_THRESHOLD
#undef SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER
#undef SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER
#define SSV6200_ID_TX_THRESHOLD 63
#define SSV6200_ID_RX_THRESHOLD 63
#ifdef PREFER_RX
#define SSV6200_PAGE_TX_THRESHOLD (126-24)
#define SSV6200_PAGE_RX_THRESHOLD (126+24)
#else
#undef SSV6200_PAGE_TX_THRESHOLD
#undef SSV6200_PAGE_RX_THRESHOLD
#define SSV6200_PAGE_TX_THRESHOLD 126
#define SSV6200_PAGE_RX_THRESHOLD 126
#endif
#define SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER (SSV6200_PAGE_TX_THRESHOLD/2)
#define SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER 2
#endif
#define SSV6200_ID_NUMBER (SSV6200_TOTAL_ID)
#define PACKET_ADDR_2_ID(addr) ((addr >> 16) & 0x7F)
#define SSV6200_ID_AC_RESERVED 1
#define SSV6200_ID_AC_BK_OUT_QUEUE 8
#define SSV6200_ID_AC_BE_OUT_QUEUE 15
#define SSV6200_ID_AC_VI_OUT_QUEUE 16
#define SSV6200_ID_AC_VO_OUT_QUEUE 16
#define SSV6200_ID_MANAGER_QUEUE 8
#define HW_MMU_PAGE_SHIFT 0x8
#define HW_MMU_PAGE_MASK 0xff
#define SSV6200_BT_PRI_SMP_TIME 0
#define SSV6200_BT_STA_SMP_TIME (SSV6200_BT_PRI_SMP_TIME+0)
#define SSV6200_WLAN_REMAIN_TIME 0
#define BT_2WIRE_EN_MSK 0x00000400
struct txResourceControl {
    u32 txUsePage:8;
    u32 txUseID:6;
    u32 edca0:4;
    u32 edca1:4;
    u32 edca2:5;
    u32 edca3:5;
};
#define SSV_SKB_info_size (sizeof(struct SKB_info_st))
#include "ssv_cfg.h"
static inline void txrxboost_init(void) {
    struct sched_param param = { .sched_priority = 0 };
    sched_setscheduler(current, SCHED_NORMAL, &param);
}
static inline void txrxboost_change(u32 tx_frame_qlen, u32 low_threshold, u32 high_threshold, u32 prio) {
    struct sched_param param;
    if (tx_frame_qlen > high_threshold) {
        param.sched_priority = (int)prio;
        sched_setscheduler(current, (prio != 0)?SCHED_RR:SCHED_NORMAL, &param);
    } else if (tx_frame_qlen < low_threshold) {
        param.sched_priority = 0;
        sched_setscheduler(current, SCHED_NORMAL, &param);
    }
}
#endif
