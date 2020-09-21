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

#ifndef __SSV6006_H___
#define __SSV6006_H__ 
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
#include <hwif/hwif.h>
#include <hci/ssv_hci.h>
#include "ssv_cfg.h"
#include "ssv6006_common.h"
#define SSV6006_TOTAL_PAGE (256)
#define SSV6006_TOTAL_ID (128)
#ifndef HUW_DRV
#define SSV6006_ID_SEC 1
#define SSV6006_ID_TX_USB 1
#define SSV6006_ID_TX_THRESHOLD (63 - SSV6006_ID_TX_USB)
#define SSV6006_ID_RX_THRESHOLD (SSV6006_TOTAL_ID - (SSV6006_ID_TX_THRESHOLD + SSV6006_ID_TX_USB + SSV6006_ID_SEC))
#define SSV6006_ID_USB_TX_THRESHOLD (17)
#define SSV6006_ID_USB_RX_THRESHOLD (SSV6006_TOTAL_ID - (SSV6006_ID_USB_TX_THRESHOLD + SSV6006_ID_TX_USB + SSV6006_ID_SEC))
#define SSV6006_USB_FIFO (8)
#define SSV6006_RESERVED_SEC_PAGE (3)
#define SSV6006_RESERVED_USB_PAGE (4)
#define SSV6006_PAGE_TX_THRESHOLD (SSV6006_TOTAL_PAGE*3/4)
#define SSV6006_PAGE_RX_THRESHOLD (SSV6006_TOTAL_PAGE*1/4)
#define SSV6006_AMPDU_DIVIDER (2)
#define SSV6006_TX_LOWTHRESHOLD_PAGE_TRIGGER (SSV6006_PAGE_TX_THRESHOLD - (SSV6006_PAGE_TX_THRESHOLD/SSV6006_AMPDU_DIVIDER))
#define SSV6006_TX_LOWTHRESHOLD_ID_TRIGGER (SSV6006_ID_TX_THRESHOLD - 1)
#else
#undef SSV6006_ID_TX_THRESHOLD
#undef SSV6006_ID_RX_THRESHOLD
#undef SSV6006_PAGE_TX_THRESHOLD
#undef SSV6006_PAGE_RX_THRESHOLD
#undef SSV6006_TX_LOWTHRESHOLD_PAGE_TRIGGER
#undef SSV6006_TX_LOWTHRESHOLD_ID_TRIGGER
#define SSV6006_ID_TX_THRESHOLD 31
#define SSV6006_ID_RX_THRESHOLD 31
#define SSV6006_PAGE_TX_THRESHOLD 61
#define SSV6006_PAGE_RX_THRESHOLD 61
#define SSV6006_TX_LOWTHRESHOLD_PAGE_TRIGGER 45
#define SSV6006_TX_LOWTHRESHOLD_ID_TRIGGER 2
#endif
#define SSV6006_ID_NUMBER (SSV6006_TOTAL_ID)
#define PACKET_ADDR_2_ID(addr) ((addr >> 16) & 0x7F)
#define SSV6006_ID_AC_RESERVED 1
#define SSV6006_ID_AC_BK_OUT_QUEUE 8
#define SSV6006_ID_AC_BE_OUT_QUEUE 15
#define SSV6006_ID_AC_VI_OUT_QUEUE 16
#define SSV6006_ID_AC_VO_OUT_QUEUE 16
#define SSV6006_ID_MANAGER_QUEUE 8
#define HW_MMU_PAGE_SHIFT 0x8
#define HW_MMU_PAGE_MASK 0xff
#define SSV6006_TX_PKT_RSVD_SETTING (0x3)
#define SSV6006_TX_PKT_RSVD (SSV6006_TX_PKT_RSVD_SETTING * 16)
#define SSV6006_ALLOC_RSVD (TXPB_OFFSET+SSV6006_TX_PKT_RSVD)
#define SSV6006_BT_PRI_SMP_TIME 0
#define SSV6006_BT_STA_SMP_TIME (SSV6006_BT_PRI_SMP_TIME+0)
#define SSV6006_WLAN_REMAIN_TIME 0
#define BT_2WIRE_EN_MSK 0x00000400
#endif
