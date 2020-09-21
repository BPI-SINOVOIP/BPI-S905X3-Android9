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

#ifndef _SSV6200_COMMON_H_
#define _SSV6200_COMMON_H_ 
#include <ssv6xxx_common.h>
#define FW_VERSION_REG ADR_TX_SEG
#define M_ENG_CPU 0x00
#define M_ENG_HWHCI 0x01
#define M_ENG_EMPTY 0x02
#define M_ENG_ENCRYPT 0x03
#define M_ENG_MACRX 0x04
#define M_ENG_MIC 0x05
#define M_ENG_TX_EDCA0 0x06
#define M_ENG_TX_EDCA1 0x07
#define M_ENG_TX_EDCA2 0x08
#define M_ENG_TX_EDCA3 0x09
#define M_ENG_TX_MNG 0x0A
#define M_ENG_ENCRYPT_SEC 0x0B
#define M_ENG_MIC_SEC 0x0C
#define M_ENG_RESERVED_1 0x0D
#define M_ENG_RESERVED_2 0x0E
#define M_ENG_TRASH_CAN 0x0F
#define M_ENG_MAX (M_ENG_TRASH_CAN+1)
#define M_CPU_HWENG 0x00
#define M_CPU_TXL34CS 0x01
#define M_CPU_RXL34CS 0x02
#define M_CPU_DEFRAG 0x03
#define M_CPU_EDCATX 0x04
#define M_CPU_RXDATA 0x05
#define M_CPU_RXMGMT 0x06
#define M_CPU_RXCTRL 0x07
#define M_CPU_FRAG 0x08
#define M_CPU_TXTPUT 0x09
#ifndef ID_TRAP_SW_TXTPUT
#define ID_TRAP_SW_TXTPUT 50
#endif
#define TXPB_OFFSET 80
#define RXPB_OFFSET 80
#define SSV6200_TX_PKT_RSVD_SETTING 0x3
#define SSV6200_TX_PKT_RSVD SSV6200_TX_PKT_RSVD_SETTING*16
#define SSV6200_ALLOC_RSVD TXPB_OFFSET+SSV6200_TX_PKT_RSVD
#ifndef SSV_SUPPORT_HAL
struct ssv6200_tx_desc
{
    u32 len:16;
    u32 c_type:3;
    u32 f80211:1;
    u32 qos:1;
    u32 ht:1;
    u32 use_4addr:1;
    u32 RSVD_0:3;
    u32 bc_que:1;
    u32 security:1;
    u32 more_data:1;
    u32 stype_b5b4:2;
    u32 extra_info:1;
    u32 fCmd;
    u32 hdr_offset:8;
    u32 frag:1;
    u32 unicast:1;
    u32 hdr_len:6;
    u32 tx_report:1;
    u32 tx_burst:1;
    u32 ack_policy:2;
    u32 aggregation:1;
    u32 RSVD_1:3;
    u32 do_rts_cts:2;
    u32 reason:6;
    u32 payload_offset:8;
    u32 RSVD_4:7;
    u32 RSVD_2:1;
    u32 fCmdIdx:3;
    u32 wsid:4;
    u32 txq_idx:3;
    u32 TxF_ID:6;
    u32 rts_cts_nav:16;
    u32 frame_consume_time:10;
    u32 crate_idx:6;
    u32 drate_idx:6;
    u32 dl_length:12;
    u32 RSVD_3:14;
    u32 RESERVED[8];
    struct fw_rc_retry_params rc_params[SSV62XX_TX_MAX_RATES];
};
struct ssv6200_rx_desc
{
    u32 len:16;
    u32 c_type:3;
    u32 f80211:1;
    u32 qos:1;
    u32 ht:1;
    u32 use_4addr:1;
    u32 l3cs_err:1;
    u32 l4cs_err:1;
    u32 align2:1;
    u32 RSVD_0:2;
    u32 psm:1;
    u32 stype_b5b4:2;
    u32 extra_info:1;
    u32 edca0_used:4;
    u32 edca1_used:5;
    u32 edca2_used:5;
    u32 edca3_used:5;
    u32 mng_used:4;
    u32 tx_page_used:9;
    u32 hdr_offset:8;
    u32 frag:1;
    u32 unicast:1;
    u32 hdr_len:6;
    u32 RxResult:8;
    u32 wildcard_bssid:1;
    u32 RSVD_1:1;
    u32 reason:6;
    u32 payload_offset:8;
    u32 tx_id_used:8;
    u32 fCmdIdx:3;
    u32 wsid:4;
    u32 RSVD_3:3;
    u32 rate_idx:6;
};
struct ssv6200_rxphy_info {
    u32 len:16;
    u32 rsvd0:16;
    u32 mode:3;
    u32 ch_bw:3;
    u32 preamble:1;
    u32 ht_short_gi:1;
    u32 rate:7;
    u32 rsvd1:1;
    u32 smoothing:1;
    u32 no_sounding:1;
    u32 aggregate:1;
    u32 stbc:2;
    u32 fec:1;
    u32 n_ess:2;
    u32 rsvd2:8;
    u32 l_length:12;
    u32 l_rate:3;
    u32 rsvd3:17;
    u32 rsvd4;
    u32 rpci:8;
    u32 snr:8;
    u32 service:16;
};
struct ssv6200_rxphy_info_padding {
u32 rpci:8;
u32 snr:8;
u32 RSVD:16;
};
struct ssv6200_txphy_info {
    u32 rsvd[7];
};
#endif
#define SSV_NUM_HW_STA 2
typedef enum{
    SSV6XXX_RC_COUNTER_CLEAR = 1 ,
    SSV6XXX_RC_REPORT ,
} ssv6xxx_host_rate_control_event;
struct ssv62xx_tx_rate {
    s8 data_rate;
    u8 count;
} __attribute__((packed));
struct ampdu_ba_notify_data {
    u8 wsid;
    struct ssv62xx_tx_rate tried_rates[SSV62XX_TX_MAX_RATES];
    u16 seq_no[MAX_AGGR_NUM];
} __attribute__((packed));
struct firmware_rate_control_report_data{
    u8 wsid;
    struct ssv62xx_tx_rate rates[SSV62XX_TX_MAX_RATES];
    u16 ampdu_len;
    u16 ampdu_ack_len;
    int ack_signal;
} __attribute__((packed));
#define RC_RETRY_PARAM_OFFSET ((sizeof(struct fw_rc_retry_params))*SSV62XX_TX_MAX_RATES)
#define SSV_RC_RATE_MAX 39
#endif
