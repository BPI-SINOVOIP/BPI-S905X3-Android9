#ifndef _SSV_PKTDEF_H_
#define _SSV_PKTDEF_H_

#include <types.h>
#include "../../../include/ssv6200_common.h"






#define M0_HDR_LEN                          4
#define M1_HDR_LEN                          8
#define M2_HDR_LEN                          16

#define RX_M0_HDR_LEN                       24


/**
 *
 *  Offset Table (register):
 *
 *    c_type            hdr_len     
 *  ----------     ----------
 *  M0_TXREQ         8-bytes
 *  M1_TXREQ         12-bytes   
 *  M2_TXREQ         sizeof(PKT_TXInfo)
 *  M0_RXEVENT      
 *  M1_RXEVENT
 *  HOST_CMD
 *  HOSt_EVENT
 *
 *
 */

#define IS_RX_PKT(_p)       ((_p)->c_type==M0_RXEVENT)
#define IS_TX_PKT(_p)       (/*((_p)->c_type>=M0_TXREQ)&&*/((_p)->c_type<=M2_TXREQ))
#define IS_TX_TEST_CMD(_p)  ((_p)->c_type==TEST_CMD)


/*  WMM_Specification_1-1 : Table 14  802.1D Priority to AC mappings

    UP      Access Category
    -------------------------
    1, 2    AC_BK
    0, 3    AC_BE
    4, 5    AC_VI
    6, 7    AC_VO
*/
#define AC_BK    0
#define AC_BE    1
#define AC_VI    2
#define AC_VO    3


#define PBUF_HDR80211(p, i)             (*((u8 *)(p)+(p)->hdr_offset + (i)))

#define GET_SC_SEQNUM(sc)               (((sc) & 0xfff0) >> 4)
#define GET_SC_FRAGNUM(sc)              (((sc) & 0x000f)     )

#define GET_QC_TID(qc)                  ((qc) & 0x000f)
#define GET_QC_UP(qc)                   ((qc) & 0x0007)
#define GET_QC_AC(qc)                   ((GET_QC_UP(qc) == 0) ? AC_BE : \
                                         (GET_QC_UP(qc) == 1) ? AC_BK : \
                                         (GET_QC_UP(qc) == 2) ? AC_BK : \
                                         (GET_QC_UP(qc) == 3) ? AC_BE : \
                                         (GET_QC_UP(qc) == 4) ? AC_VI : \
                                         (GET_QC_UP(qc) == 5) ? AC_VI : \
                                         (GET_QC_UP(qc) == 6) ? AC_VO : AC_VO)


#define GET_HDR80211_FC(p)              (((p)->f80211==1) ? (((u16)PBUF_HDR80211(p, 1) << 8) | PBUF_HDR80211(p, 0)) : 0)
#define GET_HDR80211_FC_TYPE(p)         ((GET_HDR80211_FC(p) & 0x0c) >> 2)
#define GET_HDR80211_FC_TYPE_STR(t)     ((t == 0) ? "Mgmt" : ((t == 1) ? "Control" : ((t == 2) ? "Data" : "Reserved")))
#define GET_HDR80211_FC_VER(p)          ((GET_HDR80211_FC(p) & M_FC_VER))
#define GET_HDR80211_FC_TODS(p)         ((GET_HDR80211_FC(p) & M_FC_TODS)      >>  8)
#define GET_HDR80211_FC_FROMDS(p)       ((GET_HDR80211_FC(p) & M_FC_FROMDS)    >>  9)
#define GET_HDR80211_FC_MOREFRAG(p)     ((GET_HDR80211_FC(p) & M_FC_MOREFRAGS) >> 10)
#define GET_HDR80211_FC_RETRY(p)        ((GET_HDR80211_FC(p) & M_FC_RETRY)     >> 11)
#define GET_HDR80211_FC_PWRMGNT(p)      ((GET_HDR80211_FC(p) & M_FC_PWRMGMT)   >> 12)
#define GET_HDR80211_FC_MOREDATA(p)     ((GET_HDR80211_FC(p) & M_FC_MOREDATA)  >> 13)
#define GET_HDR80211_FC_PROTECTED(p)    ((GET_HDR80211_FC(p) & M_FC_PROTECTED) >> 14)
#define GET_HDR80211_FC_ORDER(p)        ((GET_HDR80211_FC(p) & M_FC_ORDER)     >> 15)

#define SET_HDR80211_FC_MOREFRAG(p)     (PBUF_HDR80211(p, 1) |= 0x04)
#define UNSET_HDR80211_FC_MOREFRAG(p)   (PBUF_HDR80211(p, 1) &= 0xfb)

#define GET_HDR80211_SC(p)              ((u16)PBUF_HDR80211(p, 23) << 8 | (PBUF_HDR80211(p, 22)))
#define GET_HDR80211_SC_SEQNUM(p)       ((GET_HDR80211_SC(p) & 0xfff0) >> 4)
#define GET_HDR80211_SC_FRAGNUM(p)      ((GET_HDR80211_SC(p) & 0x000f))

//
//  Function            ToDS    FromDS  Addr1   Addr2   Addr3   Addr4
//  -------------------------------------------------------------------------
//  IBSS                0       0       DA      SA      BSSID   Not_Used
//  To AP (infra.)      1       0       BSSID   SA      DA      Not_Used
//  From AP (infra.)    0       1       DA      BSSID   SA      Not_Used
//  WDS (bridge)        1       1       RA      TA      DA      SA
#define HAS_HDR80211_ADDRESS_4(p)       (GET_HDR80211_FC_TODS(p) & GET_HDR80211_FC_FROMDS(p))

// QoS Control Field
#define GET_HDR80211_QC(p)              (((p)->qos == 1) ? (((u16)PBUF_HDR80211(p, 25 + (HAS_HDR80211_ADDRESS_4(p)*6)) << 8) | PBUF_HDR80211(p, 24 + (HAS_HDR80211_ADDRESS_4(p)*6))) : 0)
#define GET_HDR80211_ADDRESS_1(a, p)    memcpy((a), ((u8 *)(p)+(p)->hdr_offset +  4), 6)
#define GET_HDR80211_ADDRESS_2(a, p)    memcpy((a), ((u8 *)(p)+(p)->hdr_offset + 10), 6)
#define GET_HDR80211_ADDRESS_3(a, p)    memcpy((a), ((u8 *)(p)+(p)->hdr_offset + 16), 6)

struct cfg_host_rxpkt {   

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
    u32             fCmd;

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
    u32             RSVD_4:7;
    u32             RSVD_2:1;
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             RSVD_3:3;
    u32             drate_idx:6;

    /* The definition of WORD_5: */
    u32             seq_num:16;
    u32             tid:16;
    
    /* The definition of WORD_6: */
    u32             pkt_type:8;
    u32             RCPI:8;
    u32             SNR:8;
    u32             RSVD:8;
};


typedef struct PKT_TxPhyInfo_st
{
    /* The definition of WORD_1: */
    u32 Llength:8;

    u32 Mlength:8;

    u32 RESV1:8;

    u32 RESV2:8;
    
    /* The definition of WORD_2: */
    u32 mode:3;
    u32 ch_bw:3;
    u32 preamble_option:1;
    u32 HTshortGI:1;

    u32 rate:7;
    u32 RESV3:1;

    u32 smoothing:1;
    u32 no_sounding:1;
    u32 aggregation:1;
    u32 stbc:2;
    u32 fec:1;
    u32 n_ess:2;

    u32 txpwrlvl:8;

    /* The definition of WORD_3: */
    u32 Ll_length:8;

    u32 Ml_length:4;
    u32 l_rate:3;
    u32 RESV4:1;

    u32 RESV5:16;
    
    /* The definition of WORD_4: */
    u32 RESV6:32;
      
    /* The definition of WORD_5: */
    u32 RESV7:16;

    u32 Lservice:8;
    
    u32 Mservice:8;

    /* The definition of WORD_6: */
    u32 RESV8:32;

    /* The definition of WORD_7: */
    u32 RESV9:32;
}PKT_TxPhyInfo, *PPKT_TxPhyInfo;


typedef struct PKT_RxPhyInfo_st
{
    /* The definition of WORD_1: */
    u32 Llength:8;

    u32 Mlength:8;

    u32 RESV1:8;

    u32 RESV2:8;
    
    /* The definition of WORD_2: */
    u32 mode:3;
    u32 ch_bw:3;
    u32 preamble_option:1;
    u32 HTshortGI:1;

    u32 rate:7;
    u32 RESV3:1;

    u32 smoothing:1;
    u32 no_sounding:1;
    u32 aggregation:1;
    u32 stbc:2;
    u32 fec:1;
    u32 n_ess:2;

    u32 RESV4:8;

    /* The definition of WORD_3: */
    u32 Ll_length:8;

    u32 Ml_length:4;
    u32 l_rate:3;
    u32 RESV5:1;

    u32 RESV6:16;
    
    /* The definition of WORD_4: */
    u32 RESV7:32;
      
    /* The definition of WORD_5: */
    u32 RESV8:16;

    u32 Lservice:8;
    
    u32 Mservice:8;
} PKT_RxPhyInfo, *PPKT_RxPhyInfo;


typedef struct PKT_TxInfo_st
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             RSVD_0:3;
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
    u32             ack_policy:2;   /* 0: normal ack, 1: no_ack, 2: PSMP Ack(Reserved), 3:Block Ack. See Table 8-6, IEEE 802.11 Spec. 2012 p.490*/
    u32             aggregation:1;
    u32             RSVD_1:3;
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

        /* The definition of WORD_7~13: */
	union{
		PKT_TxPhyInfo  tx_phy_info;		
		u8 			   phy_info[28];
	};

        /* The definition of WORD_14: */
        u32                            RESERVED;
        
        /* The definition of WORD_15~20: */
	struct fw_rc_retry_params rc_params[SSV62XX_TX_MAX_RATES];
		
		

} PKT_TxInfo, *PPKT_TxInfo;


typedef struct PKT_RxInfo_st
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
    u32             fCmd;

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
    u32             RSVD_4:7;
    u32             RSVD_2:1;
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             RSVD_3:3;
    u32             drate_idx:6;

    /* The definition of WORD_5: */
    u32             word5;

    /* The definition of WORD_6: */
    u32             word6;

    /* The definition of WORD_7: */
    u32             word7;
    
    /* The definition of WORD_8: */
    u32             time_l;

    /* The definition of WORD_9: */
    u32             word9;

    union{
        PKT_RxPhyInfo  rx_phy_info;
        u8             phy_info[20];
    };
    
} PKT_RxInfo, *PPKT_RxInfo;




typedef struct PKT_Info_st
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             RSVD_0:5;
    u32             more_data:1;
    u32             stype_b5b4:2;
    u32             extra_info:1;  

    /* The definition of WORD_2: */
    u32             fCmd;

    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             RSVD_1:10;
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             payload_offset:8;
    u32             RSVD_4:7;
    u32             RSVD_2:1;
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             RSVD_3:3;
    u32             drate_idx:6;
    
} PKT_Info, *PPKT_Info;



typedef struct PHY_Info_st 
{
    u32             WORD1;
    u32             WORD2;
    u32             WORD3;
    u32             WORD4;
    u32             WORD5;
    u32             WORD6;
    u32             WORD7;
    
} PHY_Info, *PPHY_Info;





/**
 * Define constants for do_rts_cts field of PKT_TxInfo structure 
 * 
 * @ TX_NO_RTS_CTS
 * @ TX_RTS_CTS
 * @ TX_CTS
 */
#define TX_NO_RTS_CTS                   0
#define TX_RTS_CTS                      1
#define TX_CTS                          2

enum fcmd_seek_type {
    FCMD_SEEK_PREV  = 0,
    FCMD_SEEK_CUR,
    FCMD_SEEK_NEXT,
};


#endif /* _SSV_PKTDEF_H_ */

