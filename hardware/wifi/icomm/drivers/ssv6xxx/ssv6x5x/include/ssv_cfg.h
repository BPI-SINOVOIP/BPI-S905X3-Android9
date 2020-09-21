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

#ifndef _SSV_CFG_H_
#define _SSV_CFG_H_ 
#define SSV6200_HW_CAP_HT 0x00000001
#define SSV6200_HW_CAP_GF 0x00000002
#define SSV6200_HW_CAP_2GHZ 0x00000004
#define SSV6200_HW_CAP_5GHZ 0x00000008
#define SSV6200_HW_CAP_SECURITY 0x00000010
#define SSV6200_HW_CAP_SGI 0x00000020
#define SSV6200_HW_CAP_HT40 0x00000040
#define SSV6200_HW_CAP_AP 0x00000080
#define SSV6200_HW_CAP_P2P 0x00000100
#define SSV6200_HW_CAP_AMPDU_RX 0x00000200
#define SSV6200_HW_CAP_AMPDU_TX 0x00000400
#define SSV6200_HW_CAP_TDLS 0x00000800
#define SSV6200_HW_CAP_STBC 0x00001000
#define SSV6200_HW_CAP_HCI_RX_AGGR 0x00002000
#define SSV6200_HW_CAP_BEACON 0x00004000
#define EXTERNEL_CONFIG_SUPPORT 64
#define USB_HW_RESOURCE_CHK_NONE 0x00000000
#define USB_HW_RESOURCE_CHK_TXID 0x00000001
#define USB_HW_RESOURCE_CHK_TXPAGE 0x00000002
#define USB_HW_RESOURCE_CHK_SCAN 0x00000004
#define USB_HW_RESOURCE_CHK_FORCE_OFF 0x00000008
#define ONLINE_RESET_ENABLE 0x00000100
#define ONLINE_RESET_EDCA_THRESHOLD_MASK 0x000000ff
#define ONLINE_RESET_EDCA_THRESHOLD_SFT 0


enum ssv_reg_domain {
	DOMAIN_FCC = 0,
	DOMAIN_china,
	DOMAIN_ETSI,
	DOMAIN_Japan,
	DOMAIN_Japan2,
	DOMAIN_Israel,
	DOMAIN_Korea,
	DOMAIN_North_America,
	DOMAIN_Singapore,
    DOMAIN_Taiwan,
    DOMAIN_other = 0xff,
};




struct rc_setting{
    u16 aging_period;
    u16 target_success_67;
    u16 target_success_5;
    u16 target_success_4;
    u16 target_success;
    u16 up_pr;
    u16 up_pr3;
    u16 up_pr4;
    u16 up_pr5;
    u16 up_pr6;
    u16 forbid;
    u16 forbid3;
    u16 forbid4;
    u16 forbid5;
    u16 forbid6;
    u16 sample_pr_4;
    u16 sample_pr_5;
    u16 force_sample_pr;
};
struct ssv6xxx_cfg {
    u32 hw_caps;
    u32 def_chan;
    u32 crystal_type;
    u32 volt_regulator;
    u32 force_chip_identity;
    u32 ignore_firmware_version;
    u8 maddr[2][6];
    u32 n_maddr;
    u32 use_sw_cipher;
    u32 use_wpa2_only;
    u32 online_reset;
    bool tx_stuck_detect;
    u32 r_calbration_result;
    u32 sar_result;
    u32 crystal_frequency_offset;
    u32 tx_power_index_1;
    u32 tx_power_index_2;
    u32 chip_identity;
    u32 rate_table_1;
    u32 rate_table_2;
    u32 wifi_tx_gain_level_gn;
    u32 wifi_tx_gain_level_b;
    u32 configuration[EXTERNEL_CONFIG_SUPPORT+1][2];
    u8 firmware_path[128];
    u8 flash_bin_path[128];
    u8 external_firmware_name[128];
    u8 mac_address_path[128];
    u8 mac_output_path[128];
    u32 ignore_efuse_mac;
	u32 efuse_rate_gain_mask;
    u32 mac_address_mode;
    u32 beacon_rssi_minimal;
    u32 rc_ht_support_cck;
    u32 auto_rate_enable;
    u32 rc_rate_idx_set;
    u32 rc_retry_set;
    u32 rc_mf;
    u32 rc_long_short;
    u32 rc_ht40;
    u32 rc_phy_mode;
 u32 rc_log;
 u32 tx_id_threshold;
 u32 tx_page_threshold;
 u32 max_rx_aggr_size;
    bool rx_burstread;
    u32 hw_rx_agg_cnt;
    bool hw_rx_agg_method_3;
    u32 hw_rx_agg_timer_reload;
    u32 usb_hw_resource;
    u32 lpbk_pkt_cnt;
    u32 lpbk_type;
    u32 lpbk_sec;
    u32 lpbk_mode;
    bool clk_src_80m;
    u32 rts_thres_len;
    u32 cci;
    u32 bk_txq_size;
    u32 be_txq_size;
    u32 vi_txq_size;
    u32 vo_txq_size;
    u32 manage_txq_size;
    u32 aggr_size_sel_pr;
    u32 greentx;
    u32 gt_stepsize;
    u32 gt_max_attenuation;
    struct rc_setting rc_setting;
    u32 directly_ack_low_threshold;
    u32 directly_ack_high_threshold;
    u32 txrxboost_prio;
    u32 txrxboost_low_threshold;
    u32 txrxboost_high_threshold;
    u32 rx_threshold;
    bool force_xtal_fo;
    u32 auto_sgi;
    u32 disable_dpd;
    u32 mic_err_notify;
    u32 domain;
};
#endif
