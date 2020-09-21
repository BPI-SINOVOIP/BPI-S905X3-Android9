/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SSV6200_COMMON_H_
#define _SSV6200_COMMON_H_

#include <ssv6xxx_common.h>
#define FW_VERSION_REG   ADR_TX_SEG
/*
    Reference with firmware
*/

/* Hardware Offload Engine ID */
#define M_ENG_CPU                       0x00
#define M_ENG_HWHCI                     0x01
//#define M_ENG_FRAG                    0x02
#define M_ENG_EMPTY                     0x02
#define M_ENG_ENCRYPT                   0x03
#define M_ENG_MACRX                     0x04  
#define M_ENG_MIC                       0x05
#define M_ENG_TX_EDCA0                  0x06
#define M_ENG_TX_EDCA1                  0x07
#define M_ENG_TX_EDCA2                  0x08
#define M_ENG_TX_EDCA3                  0x09
#define M_ENG_TX_MNG                    0x0A
#define M_ENG_ENCRYPT_SEC               0x0B
#define M_ENG_MIC_SEC                   0x0C
#define M_ENG_RESERVED_1                0x0D
#define M_ENG_RESERVED_2                0x0E
#define M_ENG_TRASH_CAN                 0x0F
#define M_ENG_MAX                      (M_ENG_TRASH_CAN+1)


/* Software Engine ID: */
#define M_CPU_HWENG                     0x00
#define M_CPU_TXL34CS                   0x01
#define M_CPU_RXL34CS                   0x02
#define M_CPU_DEFRAG                    0x03
#define M_CPU_EDCATX                    0x04
#define M_CPU_RXDATA                    0x05
#define M_CPU_RXMGMT                    0x06
#define M_CPU_RXCTRL                    0x07
#define M_CPU_FRAG                      0x08
#define M_CPU_TXTPUT                    0x09

#ifndef ID_TRAP_SW_TXTPUT
#define ID_TRAP_SW_TXTPUT               50 //(ID_TRAP_SW_START + M_CPU_TXTPUT - 1)
#endif //ID_TRAP_SW_TXTPUT

#define TXPB_OFFSET		80
#define RXPB_OFFSET		80

//TX_PKT_RSVD(3) * unit(16)
#define SSV6200_PKT_HEADROOM_RSVD       (80)
#define SSV6200_TX_PKT_RSVD_SETTING     (0x3)
#define SSV6200_TX_PKT_RSVD             (SSV6200_TX_PKT_RSVD_SETTING*16)
#define SSV6200_ALLOC_RSVD              (TXPB_OFFSET+SSV6200_TX_PKT_RSVD+SSV6200_PKT_HEADROOM_RSVD)


#ifndef SSV_SUPPORT_HAL
// move define to HAL if SSV_SUPPORT_HAL defined
/**
* struct turismo6200_tx_desc - turismo6200 tx frame descriptor.
* This descriptor is shared with turismo6200 hardware and driver.
*/
struct turismo6200_tx_desc
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             RSVD_0:3;//used for rate control report event.
    u32             bc_que:1;
    u32             security:1;
    u32             more_data:1;
    u32             stype_b5b4:2;
    u32             extra_info:1;   /* 0: don't trap to cpu after parsing, 1: trap to cpu after parsing */

    /* The definition of WORD_2: */
    u32             fCmd;

    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             tx_report:1;
    u32             tx_burst:1;     /* 0: normal, 1: burst tx */
    u32             ack_policy:2;   /* See Table 8-6, IEEE 802.11 Spec. 2012 */
    u32             aggregation:1;
    u32             RSVD_1:3;//Used for AMPDU retry counter
    u32             do_rts_cts:2;   /* 0: no RTS/CTS, 1: need RTS/CTS */
                                    /* 2: CTS protection, 3: RSVD */
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             payload_offset:8;
    u32             RSVD_4:7;
    u32             RSVD_2:1;
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             txq_idx:3;
    u32             TxF_ID:6;

    /* The definition of WORD_5: */
    u32             rts_cts_nav:16;
    u32             frame_consume_time:10;  //32 units
    u32             crate_idx:6;

    /* The definition of WORD_6: */
    u32             drate_idx:6;
    u32             dl_length:12;
    u32             RSVD_3:14;
    /* The definition of WORD_7~15: */
    u32             RESERVED[8];
    /* The definition of WORD_16~20: */
    struct fw_rc_retry_params rc_params[SSV62XX_TX_MAX_RATES];
};

/**
* struct turismo6200_rx_desc - turismo6200 rx frame descriptor.
* This descriptor is shared with turismo6200 hardware and driver.
*/
struct turismo6200_rx_desc
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             l3cs_err:1;
    u32             l4cs_err:1;
    u32             align2:1;
    u32             RSVD_0:2;
    u32             psm:1;
    u32             stype_b5b4:2;
    u32             extra_info:1;  

    /* The definition of WORD_2: */
    u32             edca0_used:4;
    u32             edca1_used:5;
    u32             edca2_used:5;
    u32             edca3_used:5;
    u32             mng_used:4;
    u32             tx_page_used:9;

    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             RxResult:8;
    u32             wildcard_bssid:1;
    u32             RSVD_1:1;
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             payload_offset:8;
    u32             tx_id_used:8;
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             RSVD_3:3;
    u32             rate_idx:6;

};

struct turismo6200_rxphy_info {
    /* WORD 1: */
    u32             len:16;
    u32             rsvd0:16;

    /* WORD 2: */
    u32             mode:3;
    u32             ch_bw:3;
    u32             preamble:1;
    u32             ht_short_gi:1;
    u32             rate:7;
    u32             rsvd1:1;
    u32             smoothing:1;
    u32             no_sounding:1;
    u32             aggregate:1;
    u32             stbc:2;
    u32             fec:1;
    u32             n_ess:2;
    u32             rsvd2:8;

    /* WORD 3: */
    u32             l_length:12;
    u32             l_rate:3;
    u32             rsvd3:17;

    /* WORD 4: */
    u32             rsvd4;

    /* WORD 5: G, N mode only */
    u32             rpci:8;     /* RSSI */
    u32             snr:8;
    u32             service:16;

};

struct turismo6200_rxphy_info_padding {

/* WORD 1: for B, G, N mode */
u32             rpci:8;     /* RSSI */
u32             snr:8;
u32             RSVD:16;
};

struct turismo6200_txphy_info {
    u32             rsvd[7];

};
#endif


/* The maximal number of hardware offload STAs */
#define SSV_NUM_HW_STA  2


typedef enum{
//===========================================================================    
    SSV6XXX_RC_COUNTER_CLEAR                = 1                                                     ,
    SSV6XXX_RC_REPORT                                                                            ,
    
//===========================================================================    
} turismo_host_rate_control_event;


struct turismo62xx_tx_rate {
    s8 data_rate;
    u8 count;
} __attribute__((packed));

struct ampdu_ba_notify_data {
    //u16 retry_count;
    u8  wsid;
    struct turismo62xx_tx_rate tried_rates[SSV62XX_TX_MAX_RATES];
    u16 seq_no[MAX_AGGR_NUM_SETTING];    
} __attribute__((packed));

struct firmware_rate_control_report_data{
    u8 wsid;
    struct turismo62xx_tx_rate rates[SSV62XX_TX_MAX_RATES];
    u16 ampdu_len;
    u16 ampdu_ack_len;
    int ack_signal;
    /* 15 bytes free */
} __attribute__((packed));

#define RC_RETRY_PARAM_OFFSET  ((sizeof(struct fw_rc_retry_params))*SSV62XX_TX_MAX_RATES)
#define SSV_RC_RATE_MAX                     39


#endif /* _SSV6200_COMMON_H_ */
