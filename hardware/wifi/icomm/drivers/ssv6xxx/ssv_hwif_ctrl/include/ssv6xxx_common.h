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

/*
 * Common defines and declarations for host driver and firmware for all 
 * platforms.
 */

#ifndef __SSV6XXX_COMMON_H__
#define __SSV6XXX_COMMON_H__
#include <ssv_chip_id.h>

#define SSV_RC_MAX_HARDWARE_SUPPORT         1
#define RC_FIRMWARE_REPORT_FLAG             0x80

#define FIRWARE_NOT_MATCH_CODE                  0xF1F1F1F1

/* reserved max sdio align */
#define MAX_RX_PKT_RSVD                     512
/* 30 byte 4 addr hdr, 2 byte QoS, 2304 byte MSDU, 12 byte crypt, 4 byte FCS, 80 byte rx_desc */
#define MAX_FRAME_SIZE                      2432
/* 802.11ad extends maximum MSDU size for DMG (freq > 40Ghz) networks
 * to 3839 or 7920 bytes, see 8.2.3 General frame format
 */
#define MAX_FRAME_SIZE_DMG            		4096

//HCI RX AGG
#define HCI_RX_AGGR_SIZE                   0x2710
#define MAX_HCI_RX_AGGR_SIZE                (HCI_RX_AGGR_SIZE+MAX_FRAME_SIZE)  //AGGR_SIZE+MPDU

// RX mode
#define RX_NORMAL_MODE                  0x0001
#define RX_HW_AGG_MODE                  0x0002
#define RX_HW_AGG_MODE_METH3            0x0004
#define RX_BURSTREAD_MODE               0x0008
// RX BURSTREAD READ SIZE
#define RX_BURSTREAD_SZ_FROM_CMD        0x0001
#define RX_BURSTREAD_SZ_MAX_FRAME       0x0002
#define RX_BURSTREAD_SZ_MAX_FRAME_DMG   0x0003
//
#define MAX_RX_BURSTREAD_CNT                   3
#define MAX_RX_BURSTREAD_LENGTH                1024

// Log category
#define LOG_TX_DESC     		0x0001
#define LOG_AMPDU_SSN   		0x0002
#define LOG_AMPDU_DBG   		0x0004
#define LOG_AMPDU_ERR   		0x0008
#define LOG_BEACON      		0x0010
#define LOG_RATE_CONTROL      	0x0020
#define LOG_RATE_REPORT      	0x0040
#define LOG_TX_FRAME            0x0080
#define LOG_RX_DESC             0x0100
#define LOG_HCI             	0x0200
#define LOG_HWIF             	0x0400
#define LOG_HAL                 0x0800
#define LOG_REGW                0x1000
#define LOG_FLASH_BIN           0x2000
#define LOG_KRACK               0x4000

// Maximum number of frames in AMPDU
#define MAX_AGGR_NUM_SETTING                    (24)
#define MAX_AGGR_NUM(_sh)                       (_sh->cfg.max_aggr_num)

// Freddie ToDo: Move firmware multi-rate retry to turismo6200.
#define SSV62XX_TX_MAX_RATES    3
struct fw_rc_retry_params {
    u32 count:4;
    u32 drate:6;
    u32 crate:6;
    u32 rts_cts_nav:16;
    u32 frame_consume_time:10;
    u32 dl_length:12;
    u32 RSVD:10;
} __attribute__((packed));

#define TXPB_OFFSET		80
#define RXPB_OFFSET		80
/* Maximum chip ID length */
#define SSV6XXX_CHIP_ID_LENGTH                  (24)
#define SSV6XXX_CHIP_ID_SHORT_LENGTH            (8)

/**
 *  The flag definition for c_type (command type) field of PKTInfo:
 *
 *      @ M0_TXREQ:
 *      @ M1_TXREQ
 *      @ M2_TXREQ
 *      @ M0_RXEVENT
 *      @ M1_RXEVENT
 *      @ HOST_CMD
 *      @ HOST_EVENT
 *
 */
#define M0_TXREQ                            0
#define M1_TXREQ                            1
#define M2_TXREQ                            2
#define M0_RXEVENT                          3
#define M2_RXEVENT                          4
#define HOST_CMD                            5
#define HOST_EVENT                          6
#define RATE_RPT                            7
#ifndef SSV_SUPPORT_HAL
#define SSV6XXX_RX_DESC_LEN                     \
        (sizeof(struct turismo6200_rx_desc) +       \
         sizeof(struct turismo6200_rxphy_info))

#define SSV6XXX_TX_DESC_LEN                     \
        (sizeof(struct turismo6200_tx_desc) + 0)
#endif

#define SSV6XXX_PKT_RUN_TYPE_NOTUSED                0x0     /* 0 */
#define SSV6XXX_PKT_RUN_TYPE_AMPDU_START            0x1     /* 0x1 ~ 0x7f */
#define SSV6XXX_PKT_RUN_TYPE_AMPDU_END              0x7f    
#define SSV6XXX_PKT_RUN_TYPE_NULLFUN                0x80    /* 0x80 */

typedef enum __PBuf_Type_E {
    NOTYPE_BUF  = 0,
    TX_BUF      = 1,
    RX_BUF      = 2    
} PBuf_Type_E;

/*************************************************************************
 * Host Command
 *************************************************************************/
/**
 *  struct cfg_host_cmd - Host Command Header Format description
 * 
 */
typedef struct cfg_host_cmd {
    u32             len:16;
    u32             c_type:3;
    u32             RSVD0:5;//It will be used as command index eg.  STA-WSID[0]-->RSVD0=0, STA-WSID[1]-->RSVD0=1
    u32             h_cmd:8;//------------------------->turismo_host_cmd/command id
    u32             sub_h_cmd;
    u32             cmd_seq_no;
    u32             blocking_seq_no; // If block seq is not zero, it will use blocking mode
    union { /*lint -save -e157 */
    u32             dummy; // Put a u32 dummy to make MSVC and GCC treat HDR_HostCmd as the same size.
    u8              dat8[0];
    u16             dat16[0];
    u32             dat32[0];
    }; /*lint -restore */
} HDR_HostCmd;

// Use 100 instead of 0 to get header size to avoid lint from reporting null pointer access.
#define HOST_CMD_HDR_LEN        ((size_t)(((HDR_HostCmd *)100)->dat8)-100U)
#define HOST_CMD_DUMMY_LEN 4

struct sdio_rxtput_cfg {
    u32 size_per_frame;
	u32 total_frames;
};


typedef enum {
//===========================================================================    
    //Public command        
    SSV6XXX_HOST_CMD_START                  = 0,
    SSV6XXX_HOST_CMD_LOG                    = 1, // Firmware log message
    SSV6XXX_HOST_CMD_PS                     = 2, // Power saving
    SSV6XXX_HOST_CMD_INIT_CALI              = 3, // Initial calibration
    SSV6XXX_HOST_CMD_RX_TPUT                = 4, // Interface RX throughput test
    SSV6XXX_HOST_CMD_TX_TPUT		    = 5, // Interface TX throughput test														,
    SSV6XXX_HOST_CMD_SMART_ICOMM            = 6, // iComm Smartlink
    SSV6XXX_HOST_CMD_WSID_OP                = 7, // Setting software WSID mapping. For Cabrio, only.
    SSV6XXX_HOST_CMD_SET_NOA                = 8, // P2P NoA (Notice of Absence)
    SSV6XXX_HOST_CMD_TX_POLL                = 9, // Regular tx polling
    SSV6XXX_HOST_CMD_SOFT_BEACON            = 10, // software beacon
    SSV6XXX_HOST_CMD_MRX_MODE               = 11, // mrx normal, promiscuous
    SSV6XXX_HOST_CMD_EXTERNAL_PA            = 12, // Set external PA
    SSV6XXX_HOST_SOC_CMD_MAXID              = 13, // Not valid host command
//===========================================================================    
} turismo_host_cmd_id;

//-------------------------------------------------------------------------

/*************************************************************************
 * Host Event
 *************************************************************************/

/**
 *  struct cfg_host_event - Host Event Header Format description
 * 
 */
typedef struct cfg_host_event {
    u32             len      :16;
    u32             c_type   :3;
    u32             RSVD0    :5;
    u32             h_event  :8;//------------------>turismo_host_evt
    u32             evt_seq_no;
    u32             blocking_seq_no;
    u8              dat[0];
} HDR_HostEvent;


typedef enum{
//===========================================================================    
    //Public event
    SOC_EVT_CMD_RESP                        = 0, // Response of a host command.
    SOC_EVT_SCAN_RESULT                     = 1, // Scan result from probe response or beacon
    SOC_EVT_DEAUTH                          = 2, // Deauthentication received but not for leave command
    SOC_EVT_GET_REG_RESP                    = 3,
    SOC_EVT_NO_BA                           = 4, // BA of an AMPDU is not received.
    SOC_EVT_RC_MPDU_REPORT                  = 5,
    SOC_EVT_RC_AMPDU_REPORT                 = 6,
    SOC_EVT_LOG                             = 7, // Firmware log module soc event
    SOC_EVT_NOA                             = 8,
    SOC_EVT_USER_END                        = 9,
    SOC_EVT_SDIO_TEST_COMMAND               = 10,
    SOC_EVT_RESET_HOST                      = 11,
    SOC_EVT_SDIO_TXTPUT_RESULT              = 12,
    SOC_EVT_TXLOOPBK_RESULT                 = 13,
    SOC_EVT_SMART_ICOMM                     = 14,
    SOC_EVT_BEACON_LOSS                     = 15,
    SOC_EVT_TX_STUCK_RESP                   = 16,
    SOC_EVT_SW_BEACON_RESP                  = 17,
    SOC_EVT_INCORRECT_CHAN_BW               = 18,
    SOC_EVT_PS                              = 19,
//===========================================================================    
    //Private    event
    SOC_EVT_MAXID                           = 20,
} turismo_soc_event;


#ifdef CONFIG_P2P_NOA
typedef enum {
//===========================================================================    
    SSV6XXX_NOA_START         = 0 ,
    SSV6XXX_NOA_STOP              ,
    
//===========================================================================    
} turismo_host_noa_event;


struct turismo62xx_noa_evt {
    u8 evt_id;
    u8 vif;
} __attribute__((packed));
#endif // CONFIG_P2P_NOA

enum SSV6XXX_IF_TYPE {
    SSV6XXX_IF_NONE = 0,
    SSV6XXX_IF_USB = 1,
    SSV6XXX_IF_SDIO = 2,
    SSV6XXX_IF_SPI = 3
};

typedef enum {
    SSV6XXX_PS_DOZE,
    SSV6XXX_PS_WAKEUP,                                                                                                                         
    SSV6XXX_PS_WAKEUP_FINISH,
    SSV6XXX_PS_WAKEUP_FIN_ACK,
    SSV6XXX_PS_HOLD_ON3_ACK,
} SSV6XXX_PS_OPS;

enum SSV6XXX_PS_WOWLAN {
    SSV6XXX_PS_WOWLAN_ANY = (1<<0),
    SSV6XXX_PS_WOWLAN_MAGIC_PKT = (1<<1)
};

struct turismo_ps_params {
    int              if_type;
    u16              ops;
    u16              aid;
    u32              wowlan_type;
    u8               chan;
    u8               chan_type;
    u8               sta_mac[6];
    u8               bssid[6];
} __attribute__((packed));

enum SSV6XXX_WSID_SEC
{
    SSV6XXX_WSID_SEC_NONE = 0,
    SSV6XXX_WSID_SEC_PAIRWISE = 1<<0,
    SSV6XXX_WSID_SEC_GROUP = 1<<1,    
    //SSV6XXX_WSID_SEC_MAX = 1<<2
};

enum SSV6XXX_RETURN_STATE
{
    SSV6XXX_STATE_OK,
    SSV6XXX_STATE_NG,
    SSV6XXX_STATE_MAX
};

#ifdef FW_WSID_WATCH_LIST
enum SSV6XXX_WSID_OPS
{
    SSV6XXX_WSID_OPS_ADD,
    SSV6XXX_WSID_OPS_DEL,
    SSV6XXX_WSID_OPS_RESETALL,
    SSV6XXX_WSID_OPS_ENABLE_CAPS,
    SSV6XXX_WSID_OPS_DISABLE_CAPS,
    SSV6XXX_WSID_OPS_HWWSID_PAIRWISE_SET_TYPE,
    SSV6XXX_WSID_OPS_HWWSID_GROUP_SET_TYPE,
    SSV6XXX_WSID_OPS_MAX
};

enum SSV6XXX_WSID_SEC_TYPE
{
    SSV6XXX_WSID_SEC_SW,
    SSV6XXX_WSID_SEC_HW,
    SSV6XXX_WSID_SEC_TYPE_MAX  
};

struct turismo_wsid_params
{
    u8 cmd;
    u8 wsid_idx;
    u8 target_wsid[6];
    u8 hw_security; 
    //identify if the target need hw security. So far it only decribes
    //the rx hw security support. It may extend to TX/RX for different security methods
}; 
#endif // FW_WSID_WATCH_LIST

enum SSV6XXX_TX_POLL_TYPE
{
    SSV6XXX_TX_POLL_START = 0,
    SSV6XXX_TX_POLL_RESET = 1,
    SSV6XXX_TX_POLL_STOP = 2
};

enum SSV6XXX_SOFT_BEACON_TYPE
{
    SSV6XXX_SOFT_BEACON_START = 0,
    SSV6XXX_SOFT_BEACON_STOP = 1
};

enum SSV6XXX_MRX_MODE_TYPE
{
    SSV6XXX_MRX_NORMAL = 0,
    SSV6XXX_MRX_PROMISCUOUS = 1
};

struct turismo_iqk_cfg {
    u32 cfg_xtal                  :8;
    u32 cfg_pa                    :8;
    u32 cfg_pabias_ctrl           :8;
    u32 cfg_pacascode_ctrl        :8;
    u32 cfg_tssi_trgt             :8;
    u32 cfg_tssi_div              :8;
    u32 cfg_def_tx_scale_11b      :8;
    u32 cfg_def_tx_scale_11b_p0d5 :8;
    u32 cfg_def_tx_scale_11g      :8;
    u32 cfg_def_tx_scale_11g_p0d5 :8;
    u32 cmd_sel;
    union {
        u32 fx_sel;
        u32 argv;
    };
    u32 phy_tbl_size;
    u32 rf_tbl_size;
};
#define PHY_SETTING_SIZE sizeof(phy_setting)

struct turismo_ch_cfg {
	u32 reg_addr;
	u32 ch1_12_value;
	u32 ch13_14_value;
};

#define IQK_CFG_LEN         (sizeof(struct turismo_iqk_cfg))
#define RF_SETTING_SIZE     (sizeof(asic_rf_setting))

/*
    If change defallt value .please recompiler firmware image.
*/
#define MAX_PHY_SETTING_TABLE_SIZE    1920
#define MAX_RF_SETTING_TABLE_SIZE    512

typedef enum {
    SSV6XXX_VOLT_DCDC_CONVERT = 0,
    SSV6XXX_VOLT_LDO_CONVERT = 1,
} turismo_cfg_volt;

typedef enum {
    SSV6XXX_VOLT_33V = 0,
    SSV6XXX_VOLT_42V,
} turismo_cfg_volt_value;

typedef enum {
    SSV6XXX_IQK_CFG_XTAL_26M = 0,
    SSV6XXX_IQK_CFG_XTAL_40M,
    SSV6XXX_IQK_CFG_XTAL_24M,
    SSV6XXX_IQK_CFG_XTAL_25M,
    SSV6XXX_IQK_CFG_XTAL_12M,
    SSV6XXX_IQK_CFG_XTAL_16M,
    SSV6XXX_IQK_CFG_XTAL_20M,
    SSV6XXX_IQK_CFG_XTAL_32M,    
    SSV6XXX_IQK_CFG_XTAL_MAX,
} turismo_iqk_cfg_xtal;

typedef enum {
    SSV6XXX_IQK_CFG_PA_DEF = 0,
    SSV6XXX_IQK_CFG_PA_LI_MPB,
    SSV6XXX_IQK_CFG_PA_LI_EVB,
    SSV6XXX_IQK_CFG_PA_HP,
} turismo_iqk_cfg_pa;

typedef enum {
    SSV6XXX_IQK_CMD_INIT_CALI = 0,
    SSV6XXX_IQK_CMD_RTBL_LOAD,
    SSV6XXX_IQK_CMD_RTBL_LOAD_DEF,
    SSV6XXX_IQK_CMD_RTBL_RESET,
    SSV6XXX_IQK_CMD_RTBL_SET,
    SSV6XXX_IQK_CMD_RTBL_EXPORT,
    SSV6XXX_IQK_CMD_TK_EVM,
    SSV6XXX_IQK_CMD_TK_TONE,
    SSV6XXX_IQK_CMD_TK_CHCH,
} turismo_iqk_cmd_sel;

#define SSV6XXX_IQK_TEMPERATURE 0x00000004
#define SSV6XXX_IQK_RXDC        0x00000008
#define SSV6XXX_IQK_RXRC        0x00000010
#define SSV6XXX_IQK_TXDC        0x00000020
#define SSV6XXX_IQK_TXIQ        0x00000040
#define SSV6XXX_IQK_RXIQ        0x00000080
#define SSV6XXX_IQK_TSSI        0x00000100
#define SSV6XXX_IQK_PAPD        0x00000200


typedef struct turismo_cabrio_reg_st {
    u32 address;
    u32 data;
} turismo_cabrio_reg;

#ifdef MULTI_THREAD_ENCRYPT
enum turismo_pkt_crypt_status 
{
    PKT_CRYPT_ST_DEC_PRE,
    PKT_CRYPT_ST_ENC_PRE,
    PKT_CRYPT_ST_DEC_DONE,
    PKT_CRYPT_ST_ENC_DONE,    
    PKT_CRYPT_ST_FAIL,
    PKT_CRYPT_ST_NOT_SUPPORT
};

#endif

#ifdef CONFIG_DEBUG_SKB_TIMESTAMP
#define SKB_DURATION_TIMEOUT_MS 100
enum turismo_debug_skb_timestamp
{
	SKB_DURATION_STAGE_TX_ENQ,
	SKB_DURATION_STAGE_TO_SDIO,
	SKB_DURATION_STAGE_IN_HWQ,
	SKB_DURATION_STAGE_END
};
#endif //CONFIG_DEBUG_SKB_TIMESTAMP

#define SSV_FIRMWARE_PATH_MAX   256
#define SSV_FIRMWARE_MAX        32

#ifdef CONFIG_SSV_SMARTLINK
enum turismo_smart_icomm_cmd
{
    STOP_SMART_ICOMM,
    START_SMART_ICOMM,
    RESET_SMART_ICOMM,
    MAX_SMART_ICOMM
};

struct turismo_si_cfg {
    u8 ssid[32];
    u8 password[64];
    s32 ssid_len;
    s32 password_len;
} __attribute__((packed));
#endif //CONFIG_SSV_SMARTLINK

#ifdef CONFIG_SSV_CABRIO_E
struct turismo_tx_loopback {
    u32 reg;
    u32 val;
    u32 restore_val;
    u8  restore;
    u8  delay_ms;    
};
#endif


/*
 * struct hci_rx_aggr_info - HCI RX Aggregation Format description
 */
struct hci_rx_aggr_info {
    u32             jmp_mpdu_len:16;
    u32             accu_rx_len:16;

    u32             RSVD0:15;
    u32             tx_page_remain:9;
    u32             tx_id_remain:8;

    u32             edca0:4;
    u32             edca1:5;
    u32             edca2:5;
    u32             edca3:5;
    u32             edca4:4;
    u32             edca5:5;
    u32             RSVD1:4;
} __attribute__((packed));

struct turismo_tx_hw_info {
	u32 tx_id_threshold;
	u32 tx_page_threshold;
	u32 tx_lowthreshold_page_trigger;
	u32 tx_lowthreshold_id_trigger;
	u32 bk_txq_size;
	u32 be_txq_size;
	u32 vi_txq_size;
	u32 vo_txq_size;
	u32 manage_txq_size;
};

struct turismo_rx_hw_info {
	u32 rx_id_threshold;
	u32 rx_page_threshold;
	u32 rx_ba_ma_sessions;
};

/* Firmware Checksum */
#define ENABLE_FW_SELF_CHECK                1
#define FW_START_SRAM_ADDR                  0x00000000
#define FW_BLOCK_SIZE                       0x800
#define CHECKSUM_BLOCK_SIZE                 1024
#define FW_CHECKSUM_INIT                    (0x12345678)
#define FW_STATUS_MASK                      (0x00FF0000)

//SRAM mode selection
enum SSV_SRAM_MODE{
    SRAM_MODE_ILM_64K_DLM_128K = 0,
    SRAM_MODE_ILM_160K_DLM_32K,
};

#endif /* __SSV6XXX_COMMON_H__ */
