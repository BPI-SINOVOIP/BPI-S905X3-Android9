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

#ifndef _SSV6006_MAC_H_
#define _SSV6006_MAC_H_ 
#define FPGA_PHY_5 "5."
#define FPGA_PHY_4 ((u64)0x2015123100171848)
#define RF_GEMINA 0x2B9
#define RF_TURISMOA 0x06579597
#define ADR_GEMINA_TRX_VER 0xCBB0E008
#define ADR_TURISMO_TRX_VER 0xCBC0E008
#define CHIP_TYPE_CHIP 0xA0
#define CHIP_TYPE_FPGA 0x0F
#define SSV6006_NUM_HW_STA 8
#define SSV6006_NUM_HW_BSSID 2
#define SSV6006_RX_BA_MAX_SESSIONS 255
#define PBUF_BASE_ADDR 0x80000000
#define PBUF_ADDR_SHIFT 16
#define PBUF_MapPkttoID(_pkt) (((u32)_pkt&0x0FFF0000)>>PBUF_ADDR_SHIFT)
#define PBUF_MapIDtoPkt(_id) (PBUF_BASE_ADDR|((_id)<<PBUF_ADDR_SHIFT))
struct ssv6006_rc_idx
{
    u8 rate_idx:3;
    u8 mf:1;
    u8 long_short:1;
    u8 ht40:1;
    u8 phy_mode:2;
};
#define SSV6006RC_MAX_RATE_RETRY 10
#define SSV6006RC_MAX_RATE_SERIES 4
#define SSV6006RC_PHY_MODE_MSK 0xc0
#define SSV6006RC_PHY_MODE_SFT 6
#define SSV6006RC_B_MODE 0
#define SSV6006RC_G_MODE 2
#define SSV6006RC_N_MODE 3
#define SSV6006RC_20_40_MSK 0x20
#define SSV6006RC_20_40_SFT 5
#define SSV6006RC_HT20 0
#define SSV6006RC_HT40 1
#define SSV6006RC_LONG_SHORT_MSK 0x10
#define SSV6006RC_LONG_SHORT_SFT 4
#define SSV6006RC_LONG 0
#define SSV6006RC_SHORT 1
#define SSV6006RC_MF_MSK 0x08
#define SSV6006RC_MF_SFT 3
#define SSV6006RC_MIX 0
#define SSV6006RC_GREEN 1
#define SSV6006RC_B_RATE_MSK 0x3
#define SSV6006RC_RATE_MSK 0x7
#define SSV6006RC_RATE_SFT 0
#define SSV6006RC_B_1M 0
#define SSV6006RC_B_2M 1
#define SSV6006RC_B_5_5M 2
#define SSV6006RC_B_11M 3
#define SSV6006RC_B_MAX_RATE 4
#define DOT11_G_RATE_IDX_OFFSET 4
#define SSV6006RC_G_6M 0
#define SSV6006RC_G_9M 1
#define SSV6006RC_G_12M 2
#define SSV6006RC_G_18M 3
#define SSV6006RC_G_24M 4
#define SSV6006RC_G_36M 5
#define SSV6006RC_G_48M 6
#define SSV6006RC_G_54M 7
#define SSV6006RC_N_MCS0 0
#define SSV6006RC_N_MCS1 1
#define SSV6006RC_N_MCS2 2
#define SSV6006RC_N_MCS3 3
#define SSV6006RC_N_MCS4 4
#define SSV6006RC_N_MCS5 5
#define SSV6006RC_N_MCS6 6
#define SSV6006RC_N_MCS7 7
#define SSV6006RC_MAX_RATE 8
#define SSV6006_EDCCA_AVG_T_6US 0x0
#define SSV6006_EDCCA_AVG_T_12US 0x1
#define SSV6006_EDCCA_AVG_T_25US 0x2
#define SSV6006_EDCCA_AVG_T_51US 0x3
#define SSV6006_EDCCA_AVG_T_102US 0x4
#define SSV6006_EDCCA_AVG_T_204US 0x5
#define SSV6006_EDCCA_AVG_T_409US 0x6
#define SSV6006_EDCCA_AVG_T_819US 0x7
enum ssv6006_lpbk_sec {
    SSV6006_CMD_LPBK_SEC_AUTO = 0,
    SSV6006_CMD_LPBK_SEC_WEP64,
    SSV6006_CMD_LPBK_SEC_WEP128,
    SSV6006_CMD_LPBK_SEC_TKIP,
    SSV6006_CMD_LPBK_SEC_AES,
    SSV6006_CMD_LPBK_SEC_OPEN,
    MAX_SSV6006_CMD_LPBK_SEC
};
enum ssv6006_lpbk_type {
    SSV6006_CMD_LPBK_TYPE_PHY = 0,
    SSV6006_CMD_LPBK_TYPE_MAC,
    SSV6006_CMD_LPBK_TYPE_HCI,
    SSV6006_CMD_LPBK_TYPE_2GRF,
    SSV6006_CMD_LPBK_TYPE_5GRF,
    MAX_SSV6006_CMD_LPBK_TYPE
};
#define DEFAULT_CFG_BIN_NAME "/tmp/flash.bin"
#define SEC_CFG_BIN_NAME "/system/etc/wifi/ssv6x5x/flash.bin"
#define SSV6006_HW_SEC_TABLE_SIZE sizeof(struct ssv6006_hw_sec)
#define SSV6006_HW_KEY_SIZE 32
#define SSV6006_PAIRWISE_KEY_OFFSET 12
#define SSV6006_GROUP_KEY_OFFSET 12
#define SSV6006_SECURITY_KEY_LEN (32)
struct ssv6006_hw_key {
    u8 key[SSV6006_SECURITY_KEY_LEN];
}__attribute__((packed));
struct ssv6006_hw_sta_key {
    u8 pair_key_idx;
    u8 pair_cipher_type;
    u8 valid;
    u8 reserve[1];
    u32 tx_pn_l;
    u32 tx_pn_h;
    struct ssv6006_hw_key pair;
}__attribute__((packed));
struct ssv6006_bss {
    u8 group_key_idx;
    u8 group_cipher_type;
    u8 reserve[2];
    u32 tx_pn_l;
    u32 tx_pn_h;
 struct ssv6006_hw_key group_key[4];
}__attribute__((packed));
struct ssv6006_hw_sec {
    struct ssv6006_bss bss_group[2];
    struct ssv6006_hw_sta_key sta_key[8];
}__attribute__((packed));
struct ssv6006_flash_layout_table
{
    u16 ic;
    u16 sid;
    u32 date;
    u32 version:24;
    u32 RSVD0:8;
    u8 dcdc;
    u8 padpd;
    u16 RSVD1;
    u16 xtal_offset_rt_config;
    u16 xtal_offset_lt_config;
    u16 xtal_offset_ht_config;
    u16 RSVD2;
    u8 xtal_offset_low_boundary;
    u8 xtal_offset_high_boundary;
    u8 band_gain_low_boundary;
    u8 band_gain_high_boundary;
    u32 RSVD3;
    u8 g_band_gain_rt[7];
    u8 RSVD4;
    u32 g_band_pa_bias0;
    u32 g_band_pa_bias1;
    u8 g_band_gain_lt[7];
    u8 RSVD5;
    u8 g_band_gain_ht[7];
    u8 RSVD6;
    u8 rate_delta[13];
    u8 RSVD7;
    u16 RSVD8;
    u8 a_band_gain_rt[4];
    u32 RSVD9;
    u32 a_band_pa_bias0;
    u32 a_band_pa_bias1;
    u8 a_band_gain_lt[4];
    u8 a_band_gain_ht[4];
    u32 RSVD10;
    u32 RSVD11;
} __attribute__((packed));
#endif
