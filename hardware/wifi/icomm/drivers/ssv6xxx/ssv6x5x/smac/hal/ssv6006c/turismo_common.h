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

#ifndef TU_COMMON_H
#define TU_COMMON_H 
#define SSV6006_TURISMOC_COMMON_CODE_VER "1.15"
#ifdef COMMON_FOR_SMAC
struct ssv6006_tx_desc
{
    u32 len:16;
    u32 c_type:3;
    u32 f80211:1;
    u32 qos:1;
    u32 ht:1;
    u32 use_4addr:1;
    u32 rvdtx_0:3;
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
    u32 no_pkt_buf_reduction:1;
    u32 tx_burst_obsolete:1;
    u32 ack_policy_obsolete:2;
    u32 aggr:2;
    u32 rsvdtx_1:1;
    u32 is_rate_stat_sample_pkt:1;
    u32 bssidx:2;
    u32 reason:6;
    u32 payload_offset_obsolete:8;
    u32 tx_pkt_run_no:8;
    u32 fCmdIdx:3;
    u32 wsid:4;
    u32 txq_idx:3;
    u32 TxF_ID:6;
    u32 rateidx1_data_duration:16;
    u32 rateidx2_data_duration:16;
    u32 rateidx3_data_duration:16;
    u32 rsvd_tx05 :2;
    u32 rate_rpt_mode:2;
    u32 ampdu_tx_ssn:12;
    u32 drate_idx0:8;
    u32 crate_idx0:8;
    u32 rts_cts_nav0:16;
    u32 dl_length0:12;
    u32 try_cnt0:4;
    u32 ack_policy0:2;
    u32 do_rts_cts0:2;
    u32 is_last_rate0:1;
    u32 rsvdtx_07b:1;
    u32 rpt_result0:2;
    u32 rpt_trycnt0:4;
    u32 rpt_noctstrycnt0:4;
    u32 drate_idx1:8;
    u32 crate_idx1:8;
    u32 rts_cts_nav1:16;
    u32 dl_length1:12;
    u32 try_cnt1:4;
    u32 ack_policy1:2;
    u32 do_rts_cts1:2;
    u32 is_last_rate1:1;
    u32 rsvdtx_09b:1;
    u32 rpt_result1:2;
    u32 rpt_trycnt1:4;
    u32 rpt_noctstrycnt1:4;
    u32 drate_idx2:8;
    u32 crate_idx2:8;
    u32 rts_cts_nav2:16;
    u32 dl_length2:12;
    u32 try_cnt2:4;
    u32 ack_policy2:2;
    u32 do_rts_cts2:2;
    u32 is_last_rate2:1;
    u32 rsvdtx_11b:1;
    u32 rpt_result2:2;
    u32 rpt_trycnt2:4;
    u32 rpt_noctstrycnt2:4;
    u32 drate_idx3:8;
    u32 crate_idx3:8;
    u32 rts_cts_nav3:16;
    u32 dl_length3:12;
    u32 try_cnt3:4;
    u32 ack_policy3:2;
    u32 do_rts_cts3:2;
    u32 is_last_rate3:1;
    u32 rsvdtx_13b:1;
    u32 rpt_result3:2;
    u32 rpt_trycnt3:4;
    u32 rpt_noctstrycnt3:4;
    u32 ampdu_whole_length:16;
    u32 ampdu_next_pkt:8;
    u32 ampdu_last_pkt:1;
    u32 rsvdtx_14a:3;
    u32 ampdu_dmydelimiter_num:4;
    u32 ampdu_tx_bitmap_lw;
    u32 ampdu_tx_bitmap_hw;
    u32 dummy0;
    u32 dummy1;
    u32 dummy2;
};
struct ssv6006_rx_desc
{
    u32 len:16;
    u32 c_type:3;
    u32 f80211:1;
    u32 qos:1;
    u32 ht:1;
    u32 use_4addr:1;
    u32 rsvdrx0_1:1;
    u32 running_no:4;
    u32 psm:1;
    u32 stype_b5b4:2;
    u32 sec_decode_err:1;
    union{
        u32 fCmd;
        u32 edca0_used:4;
        u32 edca1_used:5;
        u32 edca2_used:5;
        u32 edca3_used:5;
        u32 mng_used:4;
        u32 tx_page_used:9;
    };
    u32 hdr_offset:8;
    u32 frag:1;
    u32 unicast:1;
    u32 hdr_len:6;
    u32 RxResult:8;
    u32 bssid:2;
    u32 reason:6;
    u32 channel:8;
    u32 rx_pkt_run_no:8;
    u32 fCmdIdx:3;
    u32 wsid:4;
    u32 tkip_mmic_err:1;
    u32 rsvd_rx_3b:8;
};
struct ssv6006_rxphy_info {
    u32 len:16;
    u32 phy_rate:8;
    u32 smoothing:1;
    u32 no_sounding:1;
    u32 aggregate:1;
    u32 stbc:2;
    u32 fec:1;
    u32 n_ess:2;
    u32 l_length:12;
    u32 l_rate:3;
    u32 mrx_seqn:1;
    u32 rssi:8;
    u32 snr:8;
    u32 rx_freq_offset:16;
    u32 service:16;
    u32 rx_time_stamp;
};
#endif
#ifndef COMMON_FOR_SMAC
#define PADPDBAND 5
#define MAX_PADPD_TONE 26
struct ssv6006dpd{
    u32 am[MAX_PADPD_TONE/2];
    u32 pm[MAX_PADPD_TONE/2];
};
struct ssv6006_padpd{
    bool dpd_done[PADPDBAND];
    bool dpd_disable[PADPDBAND];
    bool pwr_mode;
    u8 current_band;
    struct ssv6006dpd val[PADPDBAND];
    u8 spur_patched;
    u8 bbscale[PADPDBAND];
};
struct ssv6006_cal_result{
    bool cal_done;
    bool cal_iq_done[PADPDBAND];
    u32 rxdc_2g[21];
    u8 rxrc_bw20;
    u8 rxrc_bw40;
    u8 txdc_i_2g;
    u8 txdc_q_2g;
    u32 rxdc_5g[21];
    u8 rxiq_alpha[PADPDBAND];
    u8 rxiq_theta[PADPDBAND];
    u8 txdc_i_5g;
    u8 txdc_q_5g;
    u8 txiq_alpha[PADPDBAND];
    u8 txiq_theta[PADPDBAND];
};
typedef void SSV_HW;
#else
typedef struct ssv_hw SSV_HW;
#endif
struct ssv6006_patch{
    bool dcdc;
    u16 xtal;
    u16 cpu_clk;
};
#ifdef COMMON_FOR_REDBULL
#define PAPDP_GAIN_SETTING 0x0
#define PAPDP_GAIN_SETTING_F2 0x0
#define PAPDP_GAIN_SETTING_2G 0x6
#define DEFAULT_DPD_BBSCALE_2500 0x72
#define DEFAULT_DPD_BBSCALE_5100 0x80
#define DEFAULT_DPD_BBSCALE_5500 0x6C
#define DEFAULT_DPD_BBSCALE_5700 0x6C
#define DEFAULT_DPD_BBSCALE_5900 0x66
#endif
#define CLK_32K 1
#define CLK_XTAL 2
#define CLK_40M 4
#define CLK_80M 8
enum {
    XTAL16M = 0,
    XTAL24M,
    XTAL26M,
    XTAL40M,
    XTAL12M,
    XTAL20M,
    XTAL25M,
    XTAL32M,
    XTAL19P2M,
    XTAL38P4M,
    XTAL52M,
    XTALMAX,
};
enum{
    G_BAND_ONLY = 0,
    AG_BAND_BOTH = 1,
};
enum {
    CAL_IDX_NONE,
    CAL_IDX_WIFI2P4G_RXDC,
    CAL_IDX_BT_RXDC,
    CAL_IDX_BW20_RXRC,
    CAL_IDX_WIFI2P4G_TXLO,
    CAL_IDX_WIFI2P4G_TXIQ,
    CAL_IDX_WIFI2P4G_RXIQ,
    CAL_IDX_WIFI2P4G_PADPD,
    CAL_IDX_5G_NONE,
    CAL_IDX_WIFI5G_RXDC,
    CAL_IDX_5G_NONE2,
    CAL_IDX_BW40_RXRC,
    CAL_IDX_WIFI5G_TXLO,
    CAL_IDX_WIFI5G_TXIQ,
    CAL_IDX_WIFI5G_RXIQ,
    CAL_IDX_WIFI5G_PADPD,
};
enum {
    MODE_STANDBY,
    MODE_CALIBRATION,
    MODE_WIFI2P4G_TX,
    MODE_WIFI2P4G_RX,
    MODE_BT_TX,
    MODE_BT_RX,
    MODE_WIFI5G_TX,
    MODE_WIFI5G_RX,
};
enum {
    BAND_2G,
    BAND_5100,
    BAND_5500,
    BAND_5700,
    BAND_5900,
    MAX_BAND,
};
#ifndef MAX_PADPD_TONE
#define MAX_PADPD_TONE 26
#endif
struct padpd_table{
    u32 addr;
    u32 mask0;
    u32 mask1;
};
extern const int cal_ch_5g[4];
extern const struct padpd_table padpd_am_table[];
extern const struct padpd_table padpd_pm_table[];
extern const u8 xtal_sx_cp_isel_wf[];
extern const char *xtal_type[];
extern const u32 am_mask[];
extern const u32 padpd_am_addr_table[][13];
extern const u32 pm_mask[];
extern const u32 padpd_pm_addr_table[][13];
extern int ssv6006_get_pa_band(int ch);
extern void turismo_pre_cal(SSV_HW *sh);
extern void turismo_post_cal(SSV_HW *sh);
extern void turismo_inter_cal(SSV_HW *sh);
#ifndef COMMON_FOR_SMAC
#ifndef COMMON_FOR_REDBULL
extern int printf_null(const char *fmt, ...);
#define MSLEEP(_val) drv_pmu_tu3(_val * 1000)
#define MDELAY MSLEEP
#define UDELAY(_val) drv_pmu_tu3(_val)
#define PRINT printf_null
#else
extern void printf_null(const char *fmt, ...);
#endif
#define PRINT_ERR printf
#define PRINT_INFO printf
#else
extern void printf_null(const char *fmt, ...);
#endif
#define TU_SET_CHANNEL(_ch) \
do{ \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
    MSLEEP(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX_RFCH_MAP_EN(1); \
    MSLEEP(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX_CHANNEL(_ch); \
    MSLEEP(1); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
    MSLEEP(1); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(3); \
    MSLEEP(1); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
    MSLEEP(1); \
                                                                                            \
} while(0)
#define TU_SET_GEMINIA_BW(_ch_type) \
do{ \
                                                                                            \
    switch (_ch_type){ \
      case NL80211_CHAN_HT20: \
      case NL80211_CHAN_NO_HT: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(1); \
                                                                                            \
            REG32_W(ADR_GEMINIA_WIFI_RX_FILTER_REGISTER, 0x271556db); \
            MSLEEP(1); \
                                                                                            \
            SET_REG(ADR_GEMINIA_DIGITAL_ADD_ON_R0, \
                (0 << RG_GEMINIA_40M_MODE_SFT) | (0 << RG_GEMINIA_LO_UP_CH_SFT), 0, \
                (RG_GEMINIA_LO_UP_CH_I_MSK & RG_GEMINIA_40M_MODE_I_MSK)); \
            MSLEEP(1); \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (0 << RG_SYSTEM_BW_SFT) | (0 << RG_PRIMARY_CH_SIDE_SFT), 0, \
                (RG_SYSTEM_BW_I_MSK & RG_PRIMARY_CH_SIDE_I_MSK)); \
                                                                                            \
            break; \
                                                                                            \
   case NL80211_CHAN_HT40MINUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            REG32_W(ADR_GEMINIA_WIFI_RX_FILTER_REGISTER, 0x2725534d); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_GEMINIA_DIGITAL_ADD_ON_R0, \
                (1 << RG_GEMINIA_40M_MODE_SFT) | (0 << RG_GEMINIA_LO_UP_CH_SFT), 0, \
                (RG_GEMINIA_LO_UP_CH_I_MSK & RG_GEMINIA_40M_MODE_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (1 << RG_SYSTEM_BW_SFT) | (1 << RG_PRIMARY_CH_SIDE_SFT), 0, \
                (RG_SYSTEM_BW_I_MSK & RG_PRIMARY_CH_SIDE_I_MSK)); \
                                                                                            \
         break; \
                                                                                            \
   case NL80211_CHAN_HT40PLUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            REG32_W(ADR_GEMINIA_WIFI_RX_FILTER_REGISTER, 0x2725534d); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_GEMINIA_DIGITAL_ADD_ON_R0, \
                (1 << RG_GEMINIA_40M_MODE_SFT) | (1 << RG_GEMINIA_LO_UP_CH_SFT), 0, \
                (RG_GEMINIA_LO_UP_CH_I_MSK & RG_GEMINIA_40M_MODE_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (1 << RG_SYSTEM_BW_SFT) | (0 << RG_PRIMARY_CH_SIDE_SFT), 0, \
                (RG_SYSTEM_BW_I_MSK & RG_PRIMARY_CH_SIDE_I_MSK)); \
                                                                                            \
            break; \
      default: \
            break; \
    } \
    MSLEEP(1); \
} while(0)
#define TU_CHANGE_GEMINIA_CHANNEL(_ch,_ch_type) \
do{ \
    const char *chan_type[]={"NL80211_CHAN_NO_HT", \
     "NL80211_CHAN_HT20", \
     "NL80211_CHAN_HT40MINUS", \
     "NL80211_CHAN_HT40PLUS"}; \
                                                                                            \
    PRINT("%s: ch %d, type %s\r\n", __func__, _ch, _chan_type[_ch_type]); \
    TU_SET_GEMINIA_BW(_ch_type) \
    TU_SET_CHANNEL(_ch); \
} while(0)
#define TU_INIT_PLL \
do{ \
    u32 regval , count = 0; \
                                                                                            \
    MSLEEP(1); \
                                                                                            \
    REG32_W(ADR_PMU_REG_2, 0xa51a8800); \
    do \
    { \
        MSLEEP(1); \
        regval = REG32_R(ADR_PMU_STATE_REG); \
        count ++ ; \
        if (regval == 3) \
            break; \
        if (count > 100){ \
            PRINT(" PLL initial fails \r\n"); \
            break; \
        } \
    } while (1); \
                                                                                            \
    MSLEEP(1); \
                                                                                            \
    REG32_W(ADR_WIFI_PHY_COMMON_SYS_REG, 0x80000000); \
                                                                                            \
    REG32_W(ADR_CLOCK_SELECTION, 0x00000004); \
    MSLEEP(1); \
} while(0)
#define TU_INIT_GEMINIA_CAL \
do{ \
    int i ; \
    u32 wifi_dc_addr, reg_val; \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_HS_3WIRE_MANUAL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_HW_PINSEL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_GEMINIA_MANUAL_ENABLE_REGISTER, \
        ((1 << RG_GEMINIA_EN_RX_ADC_SFT) | (0 <<RG_GEMINIA_RX_ADC_MANUAL_SFT)), 0, \
        (RG_GEMINIA_EN_RX_ADC_I_MSK & RG_GEMINIA_RX_ADC_MANUAL_I_MSK)); \
    UDELAY(50); \
                                                                                            \
    PRINT("--------- reset Calibration result----------------"); \
    for (i = 0; i < 22; i++) { \
        if (i %4 == 0) \
            PRINT("\r\n"); \
        wifi_dc_addr = (ADR_GEMINIA_WF_DCOC_IDAC_REGISTER1)+ (i << 2); \
                                                                                            \
        UDELAY(50); \
        reg_val = REG32_R(wifi_dc_addr); \
        PRINT("addr %x : val %x, ", wifi_dc_addr, reg_val); \
    } \
                                                                                            \
    PRINT("\r\nStart WiFi Rx DC calibration...\r\n"); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_MODE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_MODE(6); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_CAL_INDEX(1); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- Calibration result----------------"); \
    for (i = 0; i < 22; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_GEMINIA_WF_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
    SET_RG_GEMINIA_CAL_INDEX(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_MODE(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_MODE_MANUAL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_HS_3WIRE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_HW_PINSEL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_GEMINIA_MANUAL_ENABLE_REGISTER, \
        ((0 << RG_GEMINIA_EN_RX_ADC_SFT) | (1 <<RG_GEMINIA_RX_ADC_MANUAL_SFT)), 0, \
        (RG_GEMINIA_EN_RX_ADC_I_MSK & RG_GEMINIA_RX_ADC_MANUAL_I_MSK)); \
    UDELAY(50); \
} while(0)
#define TU_INIT_GEMINIA_TRX \
do{ \
   int val, mask; \
                                                                                            \
    SET_RG_GEMINIA_LOAD_RFTABLE_RDY(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_HS_3WIRE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_HW_PINSEL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_TXGAIN_PHYCTRL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_TX_GAIN(3); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_GEMINIA_MANUAL_ENABLE_REGISTER, \
        ((0 << RG_GEMINIA_EN_TX_DAC_SFT) | (1 <<RG_GEMINIA_TX_DAC_MANUAL_SFT)), 0, \
        (RG_GEMINIA_EN_TX_DAC_I_MSK & RG_GEMINIA_TX_DAC_MANUAL_I_MSK)); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_GEMINIA_MANUAL_ENABLE_REGISTER, \
        ((0 << RG_GEMINIA_EN_RX_ADC_SFT) | (1 <<RG_GEMINIA_RX_ADC_MANUAL_SFT)), 0, \
        (RG_GEMINIA_EN_RX_ADC_I_MSK & RG_GEMINIA_RX_ADC_MANUAL_I_MSK)); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_EN_TX_VTOI_2ND(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_EN_RX_PADSW(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_SX_FREF_DOUB(0); \
                                                                                            \
    MSLEEP(1); \
                                                                                            \
                                                                                            \
    SET_RG_GEMINIA_PAD_MUX_SEL(1); \
    UDELAY(50); \
                                                                                            \
    val = RG_GEMINIA_GPIO07_OE_MSK|RG_GEMINIA_GPIO06_OE_MSK|RG_GEMINIA_GPIO05_OE_MSK \
         |RG_GEMINIA_GPIO04_OE_MSK|RG_GEMINIA_GPIO03_OE_MSK; \
    mask = RG_GEMINIA_GPIO07_OE_I_MSK&RG_GEMINIA_GPIO06_OE_I_MSK&RG_GEMINIA_GPIO05_OE_I_MSK \
         & RG_GEMINIA_GPIO04_OE_I_MSK & RG_GEMINIA_GPIO03_OE_I_MSK; \
    SET_REG(ADR_GEMINIA_IO_REG_2, val, 0, mask); \
    UDELAY(50); \
    SET_RG_GEMINIA_FPGA_CLK_REF_40M_DS(1); \
    UDELAY(50); \
                                                                                            \
    SET_RG_GEMINIA_FPGA_CLK_REF_40M_OE(1); \
    MSLEEP(1); \
} while(0)
#define LOAD_TURISMOA_RF_TABLE \
do{ \
    u32 i = 0; \
                                                                                            \
    for( i = 0; i < sizeof(ssv6006_turismoA_rf_setting)/sizeof(ssv_cabrio_reg); i++) { \
       REG32_W(ssv6006_turismoA_rf_setting[i].address, \
           ssv6006_turismoA_rf_setting[i].data ); \
       UDELAY(50); \
    } \
} while(0)
#define LOAD_TURISMOA_PHY_TABLE \
do{ \
    u32 i = 0; \
                                                                                            \
    for( i = 0; i < sizeof(ssv6006_turismoA_phy_setting)/sizeof(ssv_cabrio_reg); i++) { \
       REG32_W(ssv6006_turismoA_phy_setting[i].address, \
           ssv6006_turismoA_phy_setting[i].data ); \
    } \
} while(0)
#define TU_SET_TURISMOA_BW(_ch_type) \
do{ \
                                                                                            \
    switch (_ch_type){ \
      case NL80211_CHAN_HT20: \
      case NL80211_CHAN_NO_HT: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(1); \
                                                                                            \
            SET_REG(ADR_TURISMO_TRX_MODE_REGISTER, \
                (0 << RG_TURISMO_TRX_BW_HT40_SFT) | (1 << RG_TURISMO_TRX_BW_MANUAL_SFT), 0, \
                (RG_TURISMO_TRX_BW_HT40_I_MSK & RG_TURISMO_TRX_BW_MANUAL_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_TURISMO_TRX_DIGITAL_ADD_ON_0, \
                (0 << RG_TURISMO_TRX_40M_MODE_SFT) | (0 << RG_TURISMO_TRX_LO_UP_CH_SFT), 0, \
                (RG_TURISMO_TRX_LO_UP_CH_I_MSK & RG_TURISMO_TRX_40M_MODE_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (0 << RG_SYSTEM_BW_SFT) | (0 << RG_PRIMARY_CH_SIDE_SFT), 0, \
                (RG_SYSTEM_BW_I_MSK & RG_PRIMARY_CH_SIDE_I_MSK)); \
                                                                                            \
            break; \
                                                                                            \
   case NL80211_CHAN_HT40MINUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            SET_REG(ADR_TURISMO_TRX_MODE_REGISTER, \
                (1 << RG_TURISMO_TRX_BW_HT40_SFT) | (1 << RG_TURISMO_TRX_BW_MANUAL_SFT), 0, \
                (RG_TURISMO_TRX_BW_HT40_I_MSK & RG_TURISMO_TRX_BW_MANUAL_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_TURISMO_TRX_DIGITAL_ADD_ON_0, \
                (1 << RG_TURISMO_TRX_40M_MODE_SFT) | (0 << RG_TURISMO_TRX_LO_UP_CH_SFT), 0, \
                (RG_TURISMO_TRX_LO_UP_CH_I_MSK & RG_TURISMO_TRX_40M_MODE_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (1 << RG_SYSTEM_BW_SFT) | (1<< RG_PRIMARY_CH_SIDE_SFT), 0, \
                (RG_SYSTEM_BW_I_MSK & RG_PRIMARY_CH_SIDE_I_MSK)); \
                                                                                            \
            break; \
   case NL80211_CHAN_HT40PLUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            SET_REG(ADR_TURISMO_TRX_MODE_REGISTER, \
                (1 << RG_TURISMO_TRX_BW_HT40_SFT) | (1 << RG_TURISMO_TRX_BW_MANUAL_SFT), 0, \
                (RG_TURISMO_TRX_BW_HT40_I_MSK & RG_TURISMO_TRX_BW_MANUAL_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_TURISMO_TRX_DIGITAL_ADD_ON_0, \
                (1 << RG_TURISMO_TRX_40M_MODE_SFT) | (1 << RG_TURISMO_TRX_LO_UP_CH_SFT), 0, \
                (RG_TURISMO_TRX_LO_UP_CH_I_MSK | RG_TURISMO_TRX_40M_MODE_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (1 << RG_SYSTEM_BW_SFT) | (0 << RG_PRIMARY_CH_SIDE_SFT), 0, \
                (RG_SYSTEM_BW_I_MSK & RG_PRIMARY_CH_SIDE_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            break; \
      default: \
            break; \
    } \
    UDELAY(50); \
} while(0)
#define TURISMOA_SET_5G_TXPWR(_ch) \
do{ \
                                                                                            \
                                                                                            \
    u8 pwr_paras[3][7] = {{ 7, 8, 7, 7, 7}, \
                           { 4, 8, 4, 4, 7}, \
                           { 3, 8, 3, 3, 3}}; \
    enum band { FRQ5400LO, FRQ5500LO, FRQ5500HI}; \
    int idx; \
                                                                                            \
    if (_ch <=64) { \
        idx = FRQ5400LO; \
    } else if (_ch < 100) { \
        idx = FRQ5500LO; \
    } else { \
        idx = FRQ5500HI; \
    } \
    SET_RG_TURISMO_TRX_TX_GAIN_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_5G_TX_FE_REGISTER, \
        ((1 << RG_TURISMO_TRX_5G_TXPGA_CAPSW_MANUAL_SFT) | \
         (pwr_paras[idx][0] << RG_TURISMO_TRX_5G_TXPGA_CAPSW_SFT) | \
         (pwr_paras[idx][1] << RG_TURISMO_TRX_5G_PABIAS_CTRL_SFT) | \
         (pwr_paras[idx][2] << RG_TURISMO_TRX_5G_TX_PA1_VCAS_SFT) | \
         (pwr_paras[idx][3] << RG_TURISMO_TRX_5G_TX_PA2_VCAS_SFT) | \
         (pwr_paras[idx][4] << RG_TURISMO_TRX_5G_TX_PA3_VCAS_SFT)), 0, \
        (RG_TURISMO_TRX_5G_TXPGA_CAPSW_MANUAL_I_MSK & RG_TURISMO_TRX_5G_TXPGA_CAPSW_I_MSK & \
         RG_TURISMO_TRX_5G_PABIAS_CTRL_I_MSK & RG_TURISMO_TRX_5G_TX_PA1_VCAS_I_MSK & \
         RG_TURISMO_TRX_5G_TX_PA2_VCAS_I_MSK & RG_TURISMO_TRX_5G_TX_PA3_VCAS_I_MSK)); \
    UDELAY(50); \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_5G_TX_REGISTER, \
        ((0x3f <<RG_TURISMO_TRX_5G_TXPGA_MAIN_SFT) | \
         (0 << RG_TURISMO_TRX_5G_TXPGA_STEER_SFT) | \
         (2 << RG_TURISMO_TRX_5G_TXMOD_GMCELL_SFT) | \
         (3<< RG_TURISMO_TRX_5G_TX_GAIN_SFT)), 0, \
        ( RG_TURISMO_TRX_5G_TXPGA_MAIN_I_MSK & RG_TURISMO_TRX_5G_TXPGA_STEER_I_MSK & \
          RG_TURISMO_TRX_5G_TXMOD_GMCELL_I_MSK & RG_TURISMO_TRX_5G_TX_GAIN_I_MSK)); \
    UDELAY(50); \
                                                                                            \
    SET_RG_TURISMO_TRX_5G_TX_GAIN(3); \
    UDELAY(50); \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_2_4G_TX_REGISTER, \
        ((0 << RG_TURISMO_TRX_TX_VTOI_CURRENT_SFT | 3 << RG_TURISMO_TRX_TX_VTOI_GM_SFT)), 0,\
        (RG_TURISMO_TRX_TX_VTOI_CURRENT_I_MSK & RG_TURISMO_TRX_TX_VTOI_GM_I_MSK)); \
    UDELAY(50); \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_5G_TX_DAC_REGISTER, \
        (( 0 << RG_TURISMO_TRX_5G_TX_DACLPF_ICOARSE_SFT \
        | 0xc << RG_TURISMO_TRX_5G_TX_DAC_QOFFSET_SFT \
        |0xc << RG_TURISMO_TRX_5G_TX_DAC_IOFFSET_SFT)), 0, \
        ( RG_TURISMO_TRX_5G_TX_DACLPF_ICOARSE_I_MSK&RG_TURISMO_TRX_5G_TX_DAC_QOFFSET_I_MSK& \
          RG_TURISMO_TRX_5G_TX_DAC_IOFFSET_I_MSK)); \
    UDELAY(50); \
                                                                                            \
} while(0)
#define TURISMOA_SET_5G_CHANNEL(_ch) \
do{ \
                                                                                            \
    TURISMOA_SET_5G_TXPWR(_ch); \
                                                                                            \
    SET_RG_RF_5G_BAND(1); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_SX5GB_RFCH_MAP_EN(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_SX5GB_CHANNEL(_ch); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(7); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
    UDELAY(50); \
                                                                                            \
} while(0)
#define TURISMOA_SET_2G_TXPWR \
do{ \
    SET_RG_TURISMO_TRX_TX_GAIN_MANUAL(0); \
    UDELAY(50); \
                                                                                            \
    SET_RG_TURISMO_TRX_TX_GAIN(3); \
    UDELAY(50); \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_2_4G_TX_REGISTER, \
        ((0 << RG_TURISMO_TRX_TX_VTOI_CURRENT_SFT|1 << RG_TURISMO_TRX_TX_VTOI_GM_SFT)), 0, \
        (RG_TURISMO_TRX_TX_VTOI_CURRENT_I_MSK & RG_TURISMO_TRX_TX_VTOI_GM_I_MSK) ); \
    UDELAY(50); \
} while(0)
#define TURISMOA_SET_2G_CHANNEL(_ch) \
do{ \
                                                                                            \
    TURISMOA_SET_2G_TXPWR; \
                                                                                            \
    SET_RG_RF_5G_BAND(0); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_SX_RFCH_MAP_EN(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_SX_CHANNEL(_ch); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(3); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
    UDELAY(50); \
                                                                                            \
} while(0)
#define TU_CHANGE_TURISMOA_CHANNEL(_ch,_ch_type) \
do{ \
    const char *chan_type[]={"NL80211_CHAN_NO_HT", \
     "NL80211_CHAN_HT20", \
     "NL80211_CHAN_HT40MINUS", \
     "NL80211_CHAN_HT40PLUS"}; \
                                                                                            \
    PRINT("%s: ch %d, type %s\r\n", __func__, _ch, chan_type[_ch_type]); \
    TU_SET_TURISMOA_BW(_ch_type); \
    if ( _ch <=14 && _ch >=1){ \
        SET_MTX_DUR_RSP_SIFS_G(10); \
     SET_MTX_DUR_RSP_SIFS_G(0); \
     SET_TX2TX_SIFS(13); \
        TURISMOA_SET_2G_CHANNEL( _ch); \
    } else if (_ch >=34){ \
     SET_MTX_DUR_RSP_SIFS_G(16); \
     SET_MTX_DUR_RSP_SIFS_G(6); \
     SET_TX2TX_SIFS(19); \
                                                                                            \
                                                                                            \
        if ((_ch_type == NL80211_CHAN_HT40MINUS)||(_ch_type == NL80211_CHAN_HT40PLUS)){ \
                                                                                            \
            SET_REG(ADR_TURISMO_TRX_DIGITAL_ADD_ON_0, \
                (0 << RG_TURISMO_TRX_40M_MODE_SFT) | (0 << RG_TURISMO_TRX_LO_UP_CH_SFT), 0, \
                (RG_TURISMO_TRX_LO_UP_CH_I_MSK & RG_TURISMO_TRX_40M_MODE_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            if (_ch_type == NL80211_CHAN_HT40MINUS) { \
                _ch = _ch - 2; \
            } else { \
                _ch = _ch + 2; \
            } \
        } \
        TURISMOA_SET_5G_CHANNEL(_ch); \
    } else { \
        PRINT("invalid channel %d\r\n", _ch); \
    } \
} while(0)
#define TU_INIT_TURISMOA_CALI \
do{ \
    int i ; \
    u32 wifi_dc_addr; \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_HS_3WIRE_MANUAL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE_BY_HS_3WIRE(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_HW_PINSEL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_2_4G_TRX_MANUAL_ENABLE_REGISTER, \
        ((1 << RG_TURISMO_TRX_EN_RX_ADC_SFT) | (0 <<RG_TURISMO_TRX_RX_ADC_MANUAL_SFT)), 0, \
        (RG_TURISMO_TRX_EN_RX_ADC_I_MSK & RG_TURISMO_TRX_RX_ADC_MANUAL_I_MSK)); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_CAL_INDEX(1); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- 2.4 G Calibration result----------------"); \
    for (i = 0; i < 22; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_TURISMO_TRX_WF_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
    SET_RG_TURISMO_TRX_CAL_INDEX(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_CAL_INDEX(9); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- 5 G Calibration result----------------"); \
    for (i = 0; i < 21; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_TURISMO_TRX_5G_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
    SET_RG_TURISMO_TRX_CAL_INDEX(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE_MANUAL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_HS_3WIRE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE_BY_HS_3WIRE(1); \
    UDELAY(50); \
                                                                                            \
    SET_RG_TURISMO_TRX_HW_PINSEL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_2_4G_TRX_MANUAL_ENABLE_REGISTER, \
        ((0 << RG_TURISMO_TRX_EN_RX_ADC_SFT) | (1 <<RG_TURISMO_TRX_RX_ADC_MANUAL_SFT)), 0, \
        (RG_TURISMO_TRX_EN_RX_ADC_I_MSK & RG_TURISMO_TRX_RX_ADC_MANUAL_I_MSK)); \
    UDELAY(50); \
} while(0)
#define TU_INIT_TURISMOA_TRX \
do{ \
   int val, mask; \
                                                                                            \
    SET_RG_TURISMO_TRX_LOAD_RFTABLE_RDY(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_HS_3WIRE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_MODE_BY_HS_3WIRE(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_HW_PINSEL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_TXGAIN_PHYCTRL(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_TX_GAIN(3); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_2_4G_TRX_MANUAL_ENABLE_REGISTER, \
        ((0 << RG_TURISMO_TRX_EN_TX_DAC_SFT) | (1 <<RG_TURISMO_TRX_TX_DAC_MANUAL_SFT)), 0, \
        (RG_TURISMO_TRX_EN_TX_DAC_I_MSK & RG_TURISMO_TRX_TX_DAC_MANUAL_I_MSK)); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_REG(ADR_TURISMO_TRX_2_4G_TRX_MANUAL_ENABLE_REGISTER, \
        ((0 << RG_TURISMO_TRX_EN_RX_ADC_SFT) | (1 <<RG_TURISMO_TRX_RX_ADC_MANUAL_SFT)), 0, \
        (RG_TURISMO_TRX_EN_RX_ADC_I_MSK & RG_TURISMO_TRX_RX_ADC_MANUAL_I_MSK)); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_EN_TX_VTOI_2ND(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_EN_RX_PADSW(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_TURISMO_TRX_PAD_MUX_SEL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    val = RG_TURISMO_TRX_GPIO07_OE_MSK|RG_TURISMO_TRX_GPIO06_OE_MSK \
         |RG_TURISMO_TRX_GPIO05_OE_MSK|RG_TURISMO_TRX_GPIO04_OE_MSK \
         |RG_TURISMO_TRX_GPIO03_OE_MSK; \
    mask = RG_TURISMO_TRX_GPIO07_OE_I_MSK&RG_TURISMO_TRX_GPIO06_OE_I_MSK \
         &RG_TURISMO_TRX_GPIO05_OE_I_MSK&RG_TURISMO_TRX_GPIO04_OE_I_MSK \
         &RG_TURISMO_TRX_GPIO03_OE_I_MSK; \
    SET_REG(ADR_TURISMO_TRX_IO_REG_2, val, 0, mask); \
    UDELAY(50); \
                                                                                            \
    SET_RG_TURISMO_TRX_FPGA_CLK_REF_40M_EN(1); \
    UDELAY(50); \
                                                                                            \
    SET_RG_TURISMO_TRX_GPIO17_DS(1); \
    UDELAY(50); \
                                                                                            \
    SET_RG_TURISMO_TRX_GPIO17_OE(1); \
    MSLEEP(1); \
} while(0)
#define INIT_TURISMOA_SYS \
do{ \
    LOAD_TURISMOA_RF_TABLE; \
    TU_INIT_TURISMOA_TRX; \
    TU_INIT_PLL; \
    REG32_W(ADR_WIFI_PHY_COMMON_ENABLE_REG, 0); \
    LOAD_TURISMOA_PHY_TABLE; \
    TU_INIT_TURISMOA_CALI; \
} while(0)
#define TU_INIT_TURISMOB_PLL \
do{ \
    u32 regval , count = 0; \
                                                                                            \
                                                                                            \
    SET_RG_LOAD_RFTABLE_RDY(0x1); \
    do \
    { \
        MSLEEP(1); \
        regval = REG32_R(ADR_PMU_STATE_REG); \
        count ++ ; \
        if (regval == 0x13) \
            break; \
        if (count > 100){ \
            PRINT(" PLL initial fails \r\n"); \
            break; \
        } \
    } while (1); \
                                                                                            \
    MSLEEP(1); \
                                                                                            \
    REG32_W(ADR_WIFI_PHY_COMMON_SYS_REG, 0x80010000); \
                                                                                            \
    REG32_W(ADR_CLOCK_SELECTION, 0x00000004); \
    MSLEEP(1); \
} while(0)
#define TU_INIT_TURISMOB_CALI_ORG \
do{ \
    int i ; \
    u32 wifi_dc_addr; \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(1); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- 2.4 G Rx DC Calibration result----------------"); \
    for (i = 0; i < 22; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_WF_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(9); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- 5 G Rx DC Calibration result----------------"); \
    for (i = 0; i < 21; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_5G_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
    UDELAY(50); \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
    UDELAY(50); \
} while(0)
#define TU_INIT_TURISMOB_2G_CALI_ORG \
do{ \
    int i ; \
    u32 wifi_dc_addr; \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(1); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(1); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- 2.4 G Rx DC Calibration result----------------"); \
    for (i = 0; i < 22; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_WF_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
    UDELAY(50); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
    UDELAY(50); \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
    UDELAY(50); \
} while(0)
#define TURISMOB_PRE_CAL \
do { \
                                                                                            \
    SET_RG_MODE(MODE_STANDBY); \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
                                                                                            \
    SET_RG_MODE(MODE_CALIBRATION); \
} while(0)
#define TURISMOB_POST_CAL \
do { \
                                                                                            \
    SET_RG_MODE(MODE_STANDBY); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
} while(0)
#define TURISMO_INTER_CAL \
do { \
                                                                                            \
    SET_RG_MODE(MODE_STANDBY); \
                                                                                            \
                                                                                            \
                                                                                            \
    UDELAY(100); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_MODE(MODE_CALIBRATION); \
                                                                                            \
} while(0)
#define TURISMOB_2P4G_RXDC_CAL \
do { \
    int i ; \
    u32 wifi_dc_addr; \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_RXDC); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- 2.4 G Rx DC Calibration result----------------"); \
    for (i = 0; i < 21; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_WF_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
                                                                                            \
                                                                                            \
} while(0)
#define TURISMOB_5G_RXDC_CAL \
do{ \
    int i ; \
    u32 wifi_dc_addr; \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_RXDC); \
                                                                                            \
    MSLEEP(10); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("--------- 5 G Rx DC Calibration result----------------"); \
    for (i = 0; i < 21; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_5G_DCOC_IDAC_REGISTER1)+ (i << 2); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, REG32_R(wifi_dc_addr)); \
    } \
    PRINT("\r\n"); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
                                                                                            \
                                                                                            \
} while(0)
#define TURISMOB_BW20_RXRC_CAL \
do{ \
    int count = 0; \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before WiFi BW20 RG_WF_RX_ABBCTUNE: %d\r\n", GET_RG_WF_RX_ABBCTUNE); \
                                                                                            \
    SET_RG_RX_RCCAL_DELAY(2); \
                                                                                            \
    SET_REG(ADR_RF_D_CAL_TOP_6, \
        (0xe5 << RG_RX_RCCAL_TARG_SFT) | (0 << RG_RCCAL_POLAR_INV_SFT), 0, \
        (RG_RX_RCCAL_TARG_I_MSK & RG_RCCAL_POLAR_INV_I_MSK)); \
                                                                                            \
    SET_RG_PGAG_RCCAL(3); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_BW20_RXRC); \
                                                                                            \
    while (GET_RO_RCCAL_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("WiFi BW20 RG_WF_RX_ABBCTUNE CAL RESULT: %d\r\n", GET_RG_WF_RX_ABBCTUNE); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
} while(0)
#define TURISMOB_BW40_RXRC_CAL \
do { \
    int count = 0; \
                                                                                            \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before WiFi BW40 RG_WF_RX_N_ABBCTUNE: %d\r\n", GET_RG_WF_N_RX_ABBCTUNE); \
                                                                                            \
    SET_RG_RX_N_RCCAL_DELAY(2); \
                                                                                            \
    SET_RG_PHASE_2P5M(0x800); \
                                                                                            \
    SET_REG(ADR_RF_D_CAL_TOP_6, \
        (0x197 << RG_RX_RCCAL_40M_TARG_SFT) | (0 << RG_RCCAL_POLAR_INV_SFT), 0, \
        (RG_RX_RCCAL_40M_TARG_I_MSK & RG_RCCAL_POLAR_INV_I_MSK)); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
    SET_RG_PHASE_35M(0x5800); \
                                                                                            \
    SET_RG_PGAG_RCCAL(3); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_BW40_RXRC); \
                                                                                            \
    while (GET_RO_RCCAL_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("WiFi BW40 RG_WF_N_RX_ABBCTUNE CAL RESULT: %d\r\n", GET_RG_WF_N_RX_ABBCTUNE); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
} while(0)
#define TURISMOB_TXDC_CAL \
do { \
    int count = 0; \
                                                                                            \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before txdc calibration WiFi 2P4G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         GET_RG_WF_TX_DAC_IOFFSET, GET_RG_WF_TX_DAC_QOFFSET); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_REG(ADR_CALIBRATION_GAIN_REGISTER0, \
        (0x6 << RG_TX_GAIN_TXCAL_SFT) | (0x0 << RG_PGAG_TXCAL_SFT), 0, \
        (RG_TX_GAIN_TXCAL_I_MSK & RG_PGAG_TXCAL_I_MSK)); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0x7FF); \
    SET_RG_PHASE_RXIQ_1M(0x7FF); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_TXLO); \
                                                                                            \
    while (GET_RO_TXDC_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("After txdc calibration WiFi 2P4G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         GET_RG_WF_TX_DAC_IOFFSET, GET_RG_WF_TX_DAC_QOFFSET); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
} while(0)
#define TURISMOB_TXIQ_CAL \
do { \
    int count = 0; \
                                                                                            \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("before tx iq calibration, tx alpha: %d, tx theta %d\r\n", \
         GET_RO_TX_IQ_ALPHA, GET_RO_TX_IQ_THETA); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_REG(ADR_CALIBRATION_GAIN_REGISTER0, \
        (0x6 << RG_TX_GAIN_TXCAL_SFT) | (0x0 << RG_PGAG_TXCAL_SFT), 0, \
        (RG_TX_GAIN_TXCAL_I_MSK & RG_PGAG_TXCAL_I_MSK)); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0x7FF); \
    SET_RG_PHASE_RXIQ_1M(0x7FF); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_TXIQ); \
                                                                                            \
    while (GET_RO_TXIQ_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("After tx iq calibration, tx alpha: %d, tx theta %d\r\n", \
         GET_RO_TX_IQ_ALPHA, GET_RO_TX_IQ_THETA); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
} while(0)
#define TURISMOB_RXIQ_CAL \
do { \
    int count = 0; \
                                                                                            \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before rx iq calibration, rx alpha: %d, rx theta %d\r\n", \
         GET_RO_RX_IQ_ALPHA, GET_RO_RX_IQ_THETA); \
                                                                                            \
    SET_RG_RFG_RXIQCAL(0x0); \
                                                                                            \
    SET_RG_PGAG_RXIQCAL(0x3); \
                                                                                            \
    SET_RG_TX_GAIN_RXIQCAL(0x6); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0x7FF); \
    SET_RG_PHASE_RXIQ_1M(0x7FF); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_RXIQ); \
                                                                                            \
    while (GET_RO_RXIQ_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("After rx iq calibration, rx alpha: %d, rx theta %d\r\n", \
         GET_RO_RX_IQ_ALPHA, GET_RO_RX_IQ_THETA); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
} while(0)
#define TURISMOB_5G_TXDC_CAL \
do { \
    int count = 0; \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before 5G txdc calibration WiFi 5G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         GET_RG_5G_TX_DAC_IOFFSET, GET_RG_5G_TX_DAC_QOFFSET); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_REG(ADR_5G_CALIBRATION_TIMER_GAIN_REGISTER, \
        (0xe << RG_5G_TX_GAIN_TXCAL_SFT) | (0x3 << RG_5G_PGAG_TXCAL_SFT), 0, \
        (RG_5G_TX_GAIN_TXCAL_I_MSK & RG_5G_PGAG_TXCAL_I_MSK)); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0x7FF); \
    SET_RG_PHASE_RXIQ_1M(0x7FF); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_TXLO); \
                                                                                            \
    while (GET_RO_5G_TXDC_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("After 5G txdc calibration WiFi 5G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         GET_RG_5G_TX_DAC_IOFFSET, GET_RG_5G_TX_DAC_QOFFSET); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
} while(0)
#define TURISMOB_5G_TXIQ_CAL \
{ \
    int count = 0; \
                                                                                            \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("before 5G tx iq calibration, tx alpha: %d, tx theta %d\r\n", \
         GET_RO_TX_IQ_ALPHA, GET_RO_TX_IQ_THETA); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_REG(ADR_5G_CALIBRATION_TIMER_GAIN_REGISTER, \
        (0xe << RG_5G_TX_GAIN_TXCAL_SFT) | (0x3 << RG_5G_PGAG_TXCAL_SFT), 0, \
        (RG_5G_TX_GAIN_TXCAL_I_MSK & RG_5G_PGAG_TXCAL_I_MSK)); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0x7FF); \
    SET_RG_PHASE_RXIQ_1M(0x7FF); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_TXIQ); \
                                                                                            \
    while (GET_RO_5G_TXIQ_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("After 5G tx iq calibration, tx alpha: %d, tx theta %d\r\n", \
         GET_RO_TX_IQ_ALPHA, GET_RO_TX_IQ_THETA); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
} while(0)
#define TURISMOB_5G_RXIQ_CAL \
do { \
    int count = 0; \
                                                                                            \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before 5G rx iq calibration, rx alpha: %d, rx theta %d\r\n", \
         GET_RO_RX_IQ_ALPHA, GET_RO_RX_IQ_THETA); \
                                                                                            \
    SET_RG_5G_RFG_RXIQCAL(0x0); \
                                                                                            \
    SET_RG_5G_PGAG_RXIQCAL(0x3); \
                                                                                            \
    SET_RG_5G_TX_GAIN_RXIQCAL(0xe); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0x7FF); \
    SET_RG_PHASE_RXIQ_1M(0x7FF); \
                                                                                            \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_RXIQ); \
                                                                                            \
    while (GET_RO_5G_RXIQ_DONE == 0){ \
        count ++; \
        if (count >100) { \
            break; \
        } \
        MSLEEP(1); \
    } \
                                                                                            \
    MSLEEP(10); \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("After 5G rx iq calibration, rx alpha: %d, rx theta %d\r\n", \
         GET_RO_RX_IQ_ALPHA, GET_RO_RX_IQ_THETA); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(0); \
                                                                                            \
} while(0)
#define TU_INIT_TURISMOB_CALI \
do{ \
    TURISMOB_PRE_CAL; \
                                                                                            \
    TURISMOB_2P4G_RXDC_CAL; \
                                                                                            \
    TURISMOB_BW20_RXRC_CAL; \
                                                                                            \
    TURISMOB_BW40_RXRC_CAL; \
                                                                                            \
    TURISMOB_TXDC_CAL; \
                                                                                            \
    TURISMOB_TXIQ_CAL; \
                                                                                            \
    TURISMOB_RXIQ_CAL; \
                                                                                            \
    TURISMOB_5G_RXDC_CAL; \
                                                                                            \
    TURISMOB_5G_TXDC_CAL; \
                                                                                            \
    TURISMOB_5G_TXIQ_CAL; \
                                                                                            \
    TURISMOB_5G_RXIQ_CAL; \
                                                                                            \
    TURISMOB_POST_CAL; \
} while(0)
#define TU_INIT_TURISMOB_2G_CALI \
do{ \
    TURISMOB_PRE_CAL; \
                                                                                            \
    TURISMOB_2P4G_RXDC_CAL; \
                                                                                            \
    TURISMOB_BW20_RXRC_CAL; \
                                                                                            \
    TURISMOB_BW40_RXRC_CAL; \
                                                                                            \
    TURISMOB_TXDC_CAL; \
                                                                                            \
    TURISMOB_TXIQ_CAL; \
                                                                                            \
    TURISMOB_RXIQ_CAL; \
                                                                                            \
    TURISMOB_POST_CAL; \
} while(0)
#define LOAD_TURISMOB_RF_TABLE \
do{ \
    u32 i = 0; \
                                                                                            \
    for( i = 0; i < sizeof(ssv6006_turismoB_rf_setting)/sizeof(ssv_cabrio_reg); i++) { \
       REG32_W(ssv6006_turismoB_rf_setting[i].address, \
           ssv6006_turismoB_rf_setting[i].data ); \
       UDELAY(50); \
    } \
} while(0)
#define LOAD_TURISMOB_PHY_TABLE \
do{ \
    u32 i = 0; \
                                                                                            \
    for( i = 0; i < sizeof(ssv6006_turismoB_phy_setting)/sizeof(ssv_cabrio_reg); i++) { \
       REG32_W(ssv6006_turismoB_phy_setting[i].address, \
           ssv6006_turismoB_phy_setting[i].data ); \
    } \
} while(0)
#define INIT_TURISMOB_SYS(_xtal,_band) \
do{ \
    LOAD_TURISMOB_RF_TABLE; \
    if (_xtal != XTAL26M){ \
        SET_RG_DP_XTAL_FREQ(_xtal); \
        SET_RG_SX_XTAL_FREQ(_xtal); \
    } \
    TU_INIT_TURISMOB_PLL; \
    REG32_W(ADR_WIFI_PHY_COMMON_ENABLE_REG, 0); \
    LOAD_TURISMOB_PHY_TABLE; \
    if (_band == G_BAND_ONLY){ \
        TU_INIT_TURISMOB_2G_CALI; \
    } else { \
        TU_INIT_TURISMOB_CALI; \
    } \
} while(0)
#define TU_SET_TURISMOB_BW(_ch_type) \
do{ \
                                                                                            \
    switch (_ch_type){ \
      case NL80211_CHAN_HT20: \
      case NL80211_CHAN_NO_HT: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(1); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (0 << RG_PRIMARY_CH_SIDE_SFT) | (0 << RG_SYSTEM_BW_SFT), 0, \
                (RG_PRIMARY_CH_SIDE_I_MSK & RG_SYSTEM_BW_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_DIGITAL_ADD_ON_0, \
                (0 << RG_40M_MODE_SFT) | (0 << RG_LO_UP_CH_SFT), 0, \
                (RG_40M_MODE_I_MSK & RG_LO_UP_CH_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            break; \
                                                                                            \
   case NL80211_CHAN_HT40MINUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (1 << RG_PRIMARY_CH_SIDE_SFT) | (1 << RG_SYSTEM_BW_SFT), 0, \
                (RG_PRIMARY_CH_SIDE_I_MSK & RG_SYSTEM_BW_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_DIGITAL_ADD_ON_0, \
                (1 << RG_40M_MODE_SFT) | (0 << RG_LO_UP_CH_SFT), 0, \
                (RG_40M_MODE_I_MSK & RG_LO_UP_CH_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            break; \
   case NL80211_CHAN_HT40PLUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (0 << RG_PRIMARY_CH_SIDE_SFT) | (1 << RG_SYSTEM_BW_SFT), 0, \
                (RG_PRIMARY_CH_SIDE_I_MSK & RG_SYSTEM_BW_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            SET_REG(ADR_DIGITAL_ADD_ON_0, \
                (1 << RG_40M_MODE_SFT) | (1 << RG_LO_UP_CH_SFT), 0, \
                (RG_40M_MODE_I_MSK & RG_LO_UP_CH_I_MSK)); \
            UDELAY(50); \
                                                                                            \
            break; \
      default: \
            break; \
    } \
} while(0)
#define TURISMOB_SET_5G_CHANNEL(_ch) \
do{ \
                                                                                            \
    SET_RG_RF_5G_BAND(1); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX5GB_RFCH_MAP_EN(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX5GB_CHANNEL(_ch); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(7); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
                                                                                            \
} while(0)
#define TURISMOB_SET_2G_CHANNEL(_ch) \
do{ \
    SET_RG_RF_5G_BAND(0); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX_RFCH_MAP_EN(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX_CHANNEL(_ch); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(3); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
                                                                                            \
} while(0)
#define TU_CHANGE_TURISMOB_CHANNEL(_ch,_ch_type) \
do{ \
    const char *chan_type[]={"NL80211_CHAN_NO_HT", \
     "NL80211_CHAN_HT20", \
     "NL80211_CHAN_HT40MINUS", \
     "NL80211_CHAN_HT40PLUS"}; \
                                                                                            \
    PRINT("%s: ch %d, type %s\r\n", __func__, _ch, chan_type[_ch_type]); \
    TU_SET_TURISMOB_BW(_ch_type); \
    if ( _ch <=14 && _ch >=1){ \
        SET_MTX_DUR_RSP_SIFS_G(10); \
     SET_MTX_DUR_RSP_SIFS_G(0); \
     SET_TX2TX_SIFS(13); \
        TURISMOB_SET_2G_CHANNEL( _ch); \
    } else if (_ch >=34){ \
     SET_MTX_DUR_RSP_SIFS_G(16); \
     SET_MTX_DUR_RSP_SIFS_G(6); \
     SET_TX2TX_SIFS(19); \
        TURISMOB_SET_5G_CHANNEL(_ch); \
    } else { \
        PRINT("invalid channel %d\r\n", _ch); \
    } \
} while(0)
#define TU_INIT_TURISMOC_PLL \
do{ \
    u32 regval , count = 0; \
                                                                                            \
                                                                                            \
    SET_RG_LOAD_RFTABLE_RDY(0x1); \
    do \
    { \
        MSLEEP(1); \
        regval = REG32_R(ADR_PMU_STATE_REG); \
        count ++ ; \
        if (regval == 0x13) \
            break; \
        if (count > 100){ \
            PRINT(" PLL initial fails \r\n"); \
            break; \
        } \
    } while (1); \
                                                                                            \
    MSLEEP(1); \
                                                                                            \
    REG32_W(ADR_WIFI_PHY_COMMON_SYS_REG, 0x80010000); \
                                                                                            \
    REG32_W(ADR_CLOCK_SELECTION, 0x00000008); \
    MSLEEP(1); \
} while(0)
#define LOAD_TURISMOC_RF_TABLE(_patch) \
do{ \
    u32 i = 0; \
                                                                                            \
    for( i = 0; i < sizeof(ssv6006_turismoC_rf_setting)/sizeof(ssv_cabrio_reg); i++) { \
       REG32_W(ssv6006_turismoC_rf_setting[i].address, \
           ssv6006_turismoC_rf_setting[i].data ); \
       UDELAY(50); \
    } \
    if (_patch.dcdc){ \
                                                                                            \
        SET_RG_DCDC_MODE(0x0); \
        SET_RG_BUCK_EN_PSM(0x1); \
        SET_RG_DCDC_MODE(0x1); \
        UDELAY(100); \
        SET_RG_BUCK_EN_PSM(0x0); \
   } \
} while(0)
#define LOAD_TURISMOC_PHY_TABLE \
do{ \
    u32 i = 0; \
                                                                                            \
    for( i = 0; i < sizeof(ssv6006_turismoC_phy_setting)/sizeof(ssv_cabrio_reg); i++) { \
       REG32_W(ssv6006_turismoC_phy_setting[i].address, \
           ssv6006_turismoC_phy_setting[i].data ); \
    } \
} while(0)
#define DEBUG_2P4G_RXDC_CAL \
do { \
    int rg_rfg, rg_pgag, i, j; \
    int adc_out_sum_i, adc_out_sumQ; \
                                                                                            \
    SET_RG_RX_GAIN_MANUAL(1); \
                                                                                            \
    for(i = 1;i >= 0; i--){ \
        for(j = 15;j >= 0; j--){ \
            rg_rfg = i; \
            rg_pgag = j; \
            SET_RG_RFG(rg_rfg); \
            SET_RG_PGAG(rg_pgag); \
            adc_out_sum_i = GET_RO_DC_CAL_I; \
            if (adc_out_sum_i>63) { \
                adc_out_sum_i -= 128; \
            } \
            adc_out_sumQ = GET_RO_DC_CAL_Q; \
            if(adc_out_sumQ>63){ \
                adc_out_sumQ -= 128; \
            } \
            PRINT("lna gain is %d, pga gain is %d, ADC_OUT_I is %d, ADC_OUT_Q is %d\r\n", \
               rg_rfg, rg_pgag, adc_out_sum_i, adc_out_sumQ); \
        } \
        PRINT("------------------------------------------------------------\r\n"); \
    } \
                                                                                            \
    SET_RG_RX_GAIN_MANUAL(0); \
} while(0)
#define TURISMOC_2P4G_RXDC_CAL(_cal) \
do { \
    int i = 0; \
    u32 wifi_dc_addr; \
                                                                                            \
    SET_REG(ADR_SX_2_4GB_5GB_REGISTER_INT3BIT___CH_TABLE, \
        (0x6 << RG_SX_CHANNEL_SFT) | (0x1 << RG_SX_RFCH_MAP_EN_SFT), 0, \
        (RG_SX_CHANNEL_I_MSK & RG_SX_RFCH_MAP_EN_I_MSK)); \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_RXDC); \
    UDELAY(100); \
    while (GET_RO_WF_DCCAL_DONE == 0){ \
        i ++; \
        if (i >10000) { \
            PRINT_ERR("%s: 2.4 RXDC cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(1); \
    } \
    PRINT("------------------------------------------------%d\r\n",i); \
    PRINT("--------- 2.4 G Rx DC Calibration result----------------"); \
    for (i = 0; i < 21; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_WF_DCOC_IDAC_REGISTER1)+ (i << 2); \
       _cal->rxdc_2g[i] = REG32_R(wifi_dc_addr); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, _cal->rxdc_2g[i]); \
    } \
    PRINT("\r\n"); \
                                                                                            \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
                                                                                            \
                                                                                            \
                                                                                            \
} while (0)
#define DEBUG_5G_RXDC_CAL \
do { \
    int rg_rfg, rg_pgag, i, j; \
    int adc_out_sum_i, adc_out_sumQ; \
                                                                                            \
    SET_RG_RX_GAIN_MANUAL(1); \
                                                                                            \
    for(i = 1;i >= 0; i--){ \
        for(j = 15;j >= 0; j--){ \
            rg_rfg = i; \
            rg_pgag = j; \
            SET_RG_RFG(rg_rfg); \
            SET_RG_PGAG(rg_pgag); \
            adc_out_sum_i = GET_RO_DC_CAL_I; \
            if (adc_out_sum_i>63) { \
                adc_out_sum_i -= 128; \
            } \
            adc_out_sumQ = GET_RO_DC_CAL_Q; \
            if(adc_out_sumQ>63){ \
                adc_out_sumQ -= 128; \
            } \
            PRINT("lna gain is %d, pga gain is %d, ADC_OUT_I is %d, ADC_OUT_Q is %d\r\n", \
               rg_rfg, rg_pgag, adc_out_sum_i, adc_out_sumQ); \
        } \
        PRINT("------------------------------------------------------------\r\n"); \
    } \
                                                                                            \
    SET_RG_RX_GAIN_MANUAL(0); \
} while(0)
#define TURISMOC_5G_RXDC_CAL(_cal) \
do { \
    int i = 0; \
    u32 wifi_dc_addr; \
                                                                                            \
    SET_REG(ADR_SX_5GB_REGISTER_INT3BIT___CH_TABLE, \
        (100 << RG_SX5GB_CHANNEL_SFT) | (0x1 << RG_SX5GB_RFCH_MAP_EN_SFT), 0, \
        (RG_SX5GB_CHANNEL_I_MSK & RG_SX5GB_RFCH_MAP_EN_I_MSK)); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_RXDC); \
    UDELAY(100); \
    while (GET_RO_5G_DCCAL_DONE == 0){ \
        i ++; \
        if (i >10000) { \
            PRINT_ERR("%s: 5G RXDC cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(1); \
    } \
                                                                                            \
    PRINT("------------------------------------------------%d\r\n",i); \
    PRINT("--------- 5 G Rx DC Calibration result----------------"); \
    for (i = 0; i < 21; i++) { \
       if (i %4 == 0) \
          PRINT("\r\n"); \
       wifi_dc_addr = (ADR_5G_DCOC_IDAC_REGISTER1)+ (i << 2); \
       _cal->rxdc_5g[i] = REG32_R(wifi_dc_addr); \
       PRINT("addr %x : val %x, ", wifi_dc_addr, _cal->rxdc_5g[i]); \
    } \
    PRINT("\r\n"); \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
                                                                                            \
} while (0)
#define TURISMOC_BW20_RXRC_CAL(_cal) \
do { \
    int count = 0; \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before WiFi BW20 RG_WF_RX_ABBCTUNE: %d\r\n", GET_RG_WF_RX_ABBCTUNE); \
                                                                                            \
    SET_RG_RX_RCCAL_DELAY(2); \
                                                                                            \
    SET_RG_PHASE_17P5M(0x20d0); \
                                                                                            \
    SET_REG(ADR_RF_D_CAL_TOP_6, \
        (0x22c << RG_RX_RCCAL_TARG_SFT) | (0 << RG_RCCAL_POLAR_INV_SFT), 0, \
        (RG_RX_RCCAL_TARG_I_MSK & RG_RCCAL_POLAR_INV_I_MSK)); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
    SET_RG_PGAG_RCCAL(3); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_BW20_RXRC); \
                                                                                            \
    UDELAY(250); \
                                                                                            \
    while (GET_RO_RCCAL_DONE == 0){ \
        count ++; \
        if (count >100000) { \
            PRINT_ERR("%s: bw20 RXRC cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(1); \
    } \
                                                                                            \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->rxrc_bw20 = GET_RG_WF_RX_ABBCTUNE; \
    PRINT("WiFi BW20 RG_WF_RX_ABBCTUNE CAL RESULT: %d\r\n", _cal->rxrc_bw20); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
                                                                                            \
} while (0)
#define TURISMOC_BW40_RXRC_CAL(_cal) \
do { \
    int count = 0; \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before WiFi BW40 RG_WF_RX_N_ABBCTUNE: %d\r\n", GET_RG_WF_N_RX_ABBCTUNE); \
                                                                                            \
    SET_RG_RX_N_RCCAL_DELAY(2); \
                                                                                            \
    SET_RG_PHASE_35M(0x3fff); \
                                                                                            \
    SET_REG(ADR_RF_D_CAL_TOP_6, \
        (0x213 << RG_RX_RCCAL_40M_TARG_SFT) | (0 << RG_RCCAL_POLAR_INV_SFT), 0, \
        (RG_RX_RCCAL_40M_TARG_I_MSK & RG_RCCAL_POLAR_INV_I_MSK)); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
    SET_RG_PGAG_RCCAL(3); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_BW40_RXRC); \
    UDELAY(250); \
                                                                                            \
    while (GET_RO_RCCAL_DONE == 0){ \
        count ++; \
        if (count >100000) { \
            PRINT_ERR("%s: bw40 RXRC cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(1); \
    } \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->rxrc_bw40 = GET_RG_WF_N_RX_ABBCTUNE; \
    PRINT("WiFi BW40 RG_WF_N_RX_ABBCTUNE CAL RESULT: %d\r\n", _cal->rxrc_bw40); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
                                                                                            \
} while (0)
#define TURISMOC_TXDC_CAL(_cal) \
do { \
    int count = 0; \
                                                                                        \
    SET_REG(ADR_SX_2_4GB_5GB_REGISTER_INT3BIT___CH_TABLE, \
        (0x6 << RG_SX_CHANNEL_SFT) | (0x1 << RG_SX_RFCH_MAP_EN_SFT), 0, \
        (RG_SX_CHANNEL_I_MSK & RG_SX_RFCH_MAP_EN_I_MSK)); \
                                                                                        \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before txdc calibration WiFi 2P4G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         GET_RG_WF_TX_DAC_IOFFSET, GET_RG_WF_TX_DAC_QOFFSET); \
                                                                                        \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                        \
    SET_REG(ADR_CALIBRATION_GAIN_REGISTER0, \
        (0x6 << RG_TX_GAIN_TXCAL_SFT) | (0x3 << RG_PGAG_TXCAL_SFT), 0, \
        (RG_TX_GAIN_TXCAL_I_MSK & RG_PGAG_TXCAL_I_MSK)); \
                                                                                        \
    SET_RG_TONE_SCALE(0x80); \
                                                                                        \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                        \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                        \
    SET_RG_PHASE_1M(0x0ccc); \
                                                                                        \
    SET_RG_PHASE_RXIQ_1M(0x0ccc); \
                                                                                        \
    SET_RG_ALPHA_SEL(2); \
                                                                                        \
                                                                                        \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_TXLO); \
    UDELAY(250); \
    while (GET_RO_TXDC_DONE == 0){ \
        count ++; \
        if (count >100000) { \
            PRINT_ERR("%s: 2.4G TXDC cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(1); \
    } \
                                                                                        \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->txdc_i_2g = GET_RG_WF_TX_DAC_IOFFSET; \
    _cal->txdc_q_2g = GET_RG_WF_TX_DAC_QOFFSET; \
    PRINT("After txdc calibration WiFi 2P4G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         _cal->txdc_i_2g, _cal->txdc_q_2g); \
                                                                                        \
                                                                                        \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
                                                                                        \
} while(0)
#define TURISMOC_TXIQ_CAL(_cal) \
do { \
    int count = 0; \
                                                                                        \
    SET_REG(ADR_SX_2_4GB_5GB_REGISTER_INT3BIT___CH_TABLE, \
        (0x6 << RG_SX_CHANNEL_SFT) | (0x1 << RG_SX_RFCH_MAP_EN_SFT), 0, \
        (RG_SX_CHANNEL_I_MSK & RG_SX_RFCH_MAP_EN_I_MSK)); \
                                                                                        \
                                                                                        \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("before tx iq 2.4G calibration, tx alpha: %d, tx theta %d\r\n", \
         GET_RG_TX_IQ_2500_ALPHA, GET_RG_TX_IQ_2500_THETA); \
                                                                                        \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                        \
    SET_REG(ADR_CALIBRATION_GAIN_REGISTER0, \
        (0x6 << RG_TX_GAIN_TXCAL_SFT) | (0x3 << RG_PGAG_TXCAL_SFT), 0, \
        (RG_TX_GAIN_TXCAL_I_MSK & RG_PGAG_TXCAL_I_MSK)); \
                                                                                        \
    SET_RG_TONE_SCALE(0x80); \
                                                                                        \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                        \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                        \
    SET_RG_PHASE_1M(0xccc); \
    SET_RG_PHASE_RXIQ_1M(0xccc); \
                                                                                        \
    SET_RG_ALPHA_SEL(2); \
                                                                                        \
                                                                                        \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_TXIQ); \
    UDELAY(250); \
    while (GET_RO_TXIQ_DONE == 0){ \
        count ++; \
        if (count >1000) { \
            PRINT_ERR("%s: 2.4G txiq cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(100); \
    } \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->txiq_alpha[BAND_2G] = GET_RG_TX_IQ_2500_ALPHA; \
    _cal->txiq_theta[BAND_2G] = GET_RG_TX_IQ_2500_THETA; \
    PRINT("After tx iq calibration, tx alpha: %d, tx theta %d\r\n", \
         _cal->txiq_alpha[BAND_2G], _cal->txiq_theta[BAND_2G]); \
                                                                                        \
                                                                                        \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
} while (0)
#define DEBUG_RX_IQ_CAL \
do { \
    u32 regval, regval1; \
    SET_RG_PHASE_STEP_VALUE(0xccc); \
    SET_RG_SPECTRUM_EN(1); \
                                                                                        \
    SET_REG(ADR_RF_D_CAL_TOP_7, \
        (0x1 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
        (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                        \
    MDELAY(10); \
                                                                                        \
    SET_REG(ADR_RF_D_CAL_TOP_7, \
        (0x0 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
        (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                        \
    regval1 = GET_RG_SPECTRUM_PWR_UPDATE; \
    regval = GET_RO_SPECTRUM_IQ_PWR_31_0; \
    PRINT("The spectrum power is 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n", \
        ((regval1 >> 4) & 0xf), (regval1 & 0xf), ((regval >> 28) & 0xf), ((regval >> 24) & 0xf), \
        ((regval >> 20) & 0xf), ((regval >> 16) & 0xf), ((regval >> 12) & 0xf), ((regval >> 8) & 0xf), \
        ((regval >> 4) & 0xf), (regval & 0xf)); \
                                                                                                        \
    SET_RG_PHASE_STEP_VALUE(0xF334); \
    SET_RG_SPECTRUM_EN(1); \
                                                                                                        \
    SET_REG(ADR_RF_D_CAL_TOP_7, \
        (0x1 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
        (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                                        \
    MDELAY(10); \
                                                                                                        \
    SET_REG(ADR_RF_D_CAL_TOP_7, \
        (0x0 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
        (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                                        \
    regval1 = GET_RG_SPECTRUM_PWR_UPDATE; \
    regval = GET_RO_SPECTRUM_IQ_PWR_31_0; \
    PRINT("The spectrum power is 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n", \
        ((regval1 >> 4) & 0xf), (regval1 & 0xf), ((regval >> 28) & 0xf), ((regval >> 24) & 0xf), \
        ((regval >> 20) & 0xf), ((regval >> 16) & 0xf), ((regval >> 12) & 0xf), ((regval >> 8) & 0xf), \
        ((regval >> 4) & 0xf), (regval & 0xf)); \
                                                                                                        \
    SET_RG_SPECTRUM_EN(0); \
} while(0)
#define TURISMOC_RXIQ_CAL(_cal) \
do { \
    int count = 0; \
                                                                                        \
    SET_REG(ADR_SX_2_4GB_5GB_REGISTER_INT3BIT___CH_TABLE, \
        (0x6 << RG_SX_CHANNEL_SFT) | (0x1 << RG_SX_RFCH_MAP_EN_SFT), 0, \
        (RG_SX_CHANNEL_I_MSK & RG_SX_RFCH_MAP_EN_I_MSK)); \
                                                                                        \
                                                                                        \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before rx iq calibration, rx alpha: %d, rx theta %d\r\n", \
         GET_RG_RX_IQ_2500_ALPHA, GET_RG_RX_IQ_2500_THETA); \
                                                                                        \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                        \
    SET_RG_RFG_RXIQCAL(0x0); \
                                                                                        \
    SET_RG_PGAG_RXIQCAL(0x3); \
                                                                                        \
    SET_RG_TX_GAIN_RXIQCAL(0x6); \
                                                                                        \
    SET_RG_TONE_SCALE(0x80); \
                                                                                        \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                        \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                        \
    SET_RG_PHASE_1M(0xccc); \
    SET_RG_PHASE_RXIQ_1M(0xccc); \
                                                                                        \
    SET_RG_ALPHA_SEL(2); \
                                                                                        \
                                                                                        \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_RXIQ); \
    UDELAY(250); \
    while (GET_RO_RXIQ_DONE == 0){ \
        count ++; \
        if (count >100000) { \
            PRINT_ERR("%s: 2.4G RXIQ cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(1); \
    } \
                                                                                        \
                                                                                        \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->rxiq_alpha[BAND_2G] = GET_RG_RX_IQ_2500_ALPHA; \
    _cal->rxiq_theta[BAND_2G] = GET_RG_RX_IQ_2500_THETA; \
    PRINT("After rx iq calibration, rx alpha: %d, rx theta %d\r\n", \
         _cal->rxiq_alpha[BAND_2G], _cal->rxiq_theta[BAND_2G]); \
                                                                                        \
                                                                                        \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
} while(0)
#define TURISMOC_5G_TXDC_CAL(_cal) \
do{ \
    int count = 0; \
                                                                                            \
    SET_REG(ADR_SX_5GB_REGISTER_INT3BIT___CH_TABLE, \
        (100 << RG_SX5GB_CHANNEL_SFT) | (0x1 << RG_SX5GB_RFCH_MAP_EN_SFT), 0, \
        (RG_SX5GB_CHANNEL_I_MSK & RG_SX5GB_RFCH_MAP_EN_I_MSK)); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("Before 5G txdc calibration WiFi 5G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         GET_RG_5G_TX_DAC_IOFFSET, GET_RG_5G_TX_DAC_QOFFSET); \
                                                                                            \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_REG(ADR_5G_CALIBRATION_TIMER_GAIN_REGISTER, \
        (0x2 << RG_5G_TX_GAIN_TXCAL_SFT) | (0x3 << RG_5G_PGAG_TXCAL_SFT), 0, \
        (RG_5G_TX_GAIN_TXCAL_I_MSK & RG_5G_PGAG_TXCAL_I_MSK)); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0xCCC); \
    SET_RG_PHASE_RXIQ_1M(0xCCC); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_TXLO); \
    UDELAY(250); \
    while (GET_RO_5G_TXDC_DONE == 0){ \
        count ++; \
        if (count >100000) { \
            PRINT_ERR("%s: 5G TXDC cal failed\r\n",__func__); \
            break; \
        } \
        UDELAY(1); \
    } \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->txdc_i_5g = GET_RG_5G_TX_DAC_IOFFSET; \
    _cal->txdc_q_5g = GET_RG_5G_TX_DAC_QOFFSET; \
    PRINT("After 5G txdc calibration WiFi 5G Tx DAC IOFFSET: %d, QOFFSET %d\r\n", \
         _cal->txdc_i_5g, _cal->txdc_q_5g); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
} while(0)
#define TURISMOC_5G_TXIQ_CAL(_cal) \
{ \
    int count = 0; \
    int band; \
                                                                                            \
                                                                                            \
    SET_RG_SX5GB_RFCH_MAP_EN(1); \
                                                                                            \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("before 5G tx iq calibration, tx alpha: %d %d %d %d, tx theta %d %d %d %d\r\n", \
         GET_RG_TX_IQ_5100_ALPHA, GET_RG_TX_IQ_5100_THETA, \
         GET_RG_TX_IQ_5500_ALPHA, GET_RG_TX_IQ_5500_THETA, \
         GET_RG_TX_IQ_5700_ALPHA, GET_RG_TX_IQ_5700_THETA, \
         GET_RG_TX_IQ_5900_ALPHA, GET_RG_TX_IQ_5900_THETA); \
                                                                                            \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_RG_5G_PGAG_TXCAL(0x3); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0xccc); \
    SET_RG_PHASE_RXIQ_1M(0xccc); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
    for (band = 0; band < 4; band ++){ \
        count=0; \
        SET_RG_SX5GB_CHANNEL(cal_ch_5g[band]); \
                                                                                            \
        if( band == 2 ) { \
            SET_RG_5G_TX_GAIN_TXCAL(0x2); \
        } else { \
            SET_RG_5G_TX_GAIN_TXCAL(0x0); \
        } \
        UDELAY(1); \
                                                                                            \
                                                                                            \
        SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_TXIQ); \
        UDELAY(250); \
        while (GET_RO_5G_TXIQ_DONE == 0){ \
            count ++; \
            if (count >100000) { \
                PRINT_ERR("%s: 5G band %d TXIQ cal failed\r\n",__func__, band); \
                break; \
            } \
            UDELAY(1); \
        } \
                                                                                            \
        SET_RG_CAL_INDEX(CAL_IDX_NONE); \
    } \
                                                                                            \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->txiq_alpha[BAND_5100] = GET_RG_TX_IQ_5100_ALPHA; \
    _cal->txiq_theta[BAND_5100] = GET_RG_TX_IQ_5100_THETA; \
    _cal->txiq_alpha[BAND_5500] = GET_RG_TX_IQ_5500_ALPHA; \
    _cal->txiq_theta[BAND_5500] = GET_RG_TX_IQ_5500_THETA; \
    _cal->txiq_alpha[BAND_5700] = GET_RG_TX_IQ_5700_ALPHA; \
    _cal->txiq_theta[BAND_5700] = GET_RG_TX_IQ_5700_THETA; \
    _cal->txiq_alpha[BAND_5900] = GET_RG_TX_IQ_5900_ALPHA; \
    _cal->txiq_theta[BAND_5900] = GET_RG_TX_IQ_5900_THETA; \
    PRINT("after 5G tx iq calibration, tx alpha: %d %d %d %d, tx theta %d %d %d %d\r\n", \
        _cal->txiq_alpha[BAND_5100], _cal->txiq_alpha[BAND_5500], \
        _cal->txiq_alpha[BAND_5700], _cal->txiq_alpha[BAND_5900], \
        _cal->txiq_theta[BAND_5100], _cal->txiq_theta[BAND_5500], \
        _cal->txiq_theta[BAND_5700], _cal->txiq_theta[BAND_5900]); \
} while(0)
#define TURISMOC_5G_TXIQ_CAL_BAND(_cal,_pa_band) \
{ \
    int count = 0, alpha = 0, theta = 0; \
                                                                                            \
    SET_RG_SX5GB_RFCH_MAP_EN(1); \
                                                                                            \
    SET_RG_SX5GB_CHANNEL(cal_ch_5g[pa_band-1]); \
    if( pa_band == BAND_5700 ) { \
        SET_RG_5G_TX_GAIN_TXCAL(0x2); \
    } else { \
        SET_RG_5G_TX_GAIN_TXCAL(0x0); \
    } \
                                                                                            \
    switch (_pa_band){ \
        case BAND_5100: \
            alpha = GET_RG_TX_IQ_5100_ALPHA; \
            theta = GET_RG_TX_IQ_5100_THETA; \
            break; \
        case BAND_5500: \
            alpha = GET_RG_TX_IQ_5500_ALPHA; \
            theta = GET_RG_TX_IQ_5500_THETA; \
            break; \
        case BAND_5700: \
            alpha = GET_RG_TX_IQ_5700_ALPHA; \
            theta = GET_RG_TX_IQ_5700_THETA; \
            break; \
        case BAND_5900: \
            alpha = GET_RG_TX_IQ_5900_ALPHA; \
            theta = GET_RG_TX_IQ_5900_THETA; \
            break; \
        default: \
            break; \
    } \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("before 5G band %d tx iq calibration, tx alpha: %d, tx theta %d\n", \
         _pa_band, alpha, theta); \
                                                                                            \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_RG_5G_PGAG_TXCAL(0x3); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0xccc); \
    SET_RG_PHASE_RXIQ_1M(0xccc); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_TXIQ); \
    UDELAY(250); \
    while (GET_RO_5G_TXIQ_DONE == 0){ \
        count ++; \
        if (count >100000){ \
            PRINT_ERR("%s: 5G band %d TXIQ cal failed\r\n",__func__, _pa_band); \
            break; \
        } \
        UDELAY(1); \
    } \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
    switch (_pa_band){ \
        case BAND_5100: \
            _cal->txiq_alpha[_pa_band] = GET_RG_TX_IQ_5100_ALPHA; \
            _cal->txiq_theta[_pa_band] = GET_RG_TX_IQ_5100_THETA; \
            break; \
        case BAND_5500: \
            _cal->txiq_alpha[_pa_band] = GET_RG_TX_IQ_5500_ALPHA; \
            _cal->txiq_theta[_pa_band] = GET_RG_TX_IQ_5500_THETA; \
            break; \
        case BAND_5700: \
            _cal->txiq_alpha[_pa_band] = GET_RG_TX_IQ_5700_ALPHA; \
            _cal->txiq_theta[_pa_band] = GET_RG_TX_IQ_5700_THETA; \
            break; \
        case BAND_5900: \
            _cal->txiq_alpha[_pa_band] = GET_RG_TX_IQ_5900_ALPHA; \
            _cal->txiq_theta[_pa_band] = GET_RG_TX_IQ_5900_THETA; \
            break; \
        default: \
            break; \
    } \
    PRINT("--------------------------------------------%d\r\n", count); \
    PRINT("after 5G band %d tx iq calibration, tx alpha: %d, tx theta %d\n", \
        _pa_band, _cal->txiq_alpha[_pa_band], _cal->txiq_theta[_pa_band]); \
} while(0)
#define DEBUG_5G_RXIQ_CAL \
do{ \
    int regval, regval1; \
                                                                                            \
        SET_RG_PHASE_STEP_VALUE(0xccc); \
        SET_RG_SPECTRUM_EN(1); \
                                                                                            \
        SET_REG(ADR_RF_D_CAL_TOP_7, \
            (0x1 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
            (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                            \
        MDELAY(10); \
                                                                                            \
        SET_REG(ADR_RF_D_CAL_TOP_7, \
            (0x0 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
            (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                            \
        regval1 = GET_RG_SPECTRUM_PWR_UPDATE; \
        regval = GET_RO_SPECTRUM_IQ_PWR_31_0; \
        PRINT("The spectrum power is 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n", \
            ((regval1 >> 4) & 0xf), (regval1 & 0xf), ((regval >> 28) & 0xf), ((regval >> 24) & 0xf), \
            ((regval >> 20) & 0xf), ((regval >> 16) & 0xf), ((regval >> 12) & 0xf), ((regval >> 8) & 0xf), \
            ((regval >> 4) & 0xf), (regval & 0xf)); \
                                                                                                            \
        SET_RG_PHASE_STEP_VALUE(0xF334); \
        SET_RG_SPECTRUM_EN(1); \
                                                                                                            \
        SET_REG(ADR_RF_D_CAL_TOP_7, \
            (0x1 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
            (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                                            \
        MDELAY(10); \
                                                                                                            \
        SET_REG(ADR_RF_D_CAL_TOP_7, \
            (0x0 << RG_SPECTRUM_PWR_UPDATE_SFT) | (0x1 << RG_SPECTRUM_LO_FIX_SFT), 0, \
            (RG_SPECTRUM_PWR_UPDATE_I_MSK & RG_SPECTRUM_LO_FIX_I_MSK)); \
                                                                                                            \
        regval1 = GET_RG_SPECTRUM_PWR_UPDATE; \
        regval = GET_RO_SPECTRUM_IQ_PWR_31_0; \
        PRINT("The spectrum power is 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n", \
            ((regval1 >> 4) & 0xf), (regval1 & 0xf), ((regval >> 28) & 0xf), ((regval >> 24) & 0xf), \
            ((regval >> 20) & 0xf), ((regval >> 16) & 0xf), ((regval >> 12) & 0xf), ((regval >> 8) & 0xf), \
            ((regval >> 4) & 0xf), (regval & 0xf)); \
                                                                                                            \
        SET_RG_SPECTRUM_EN(0); \
} while(0)
#define TURISMOC_5G_RXIQ_CAL(_cal) \
do{ \
    int count = 0; \
    int band; \
                                                                                            \
    SET_RG_SX5GB_RFCH_MAP_EN(1); \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("before 5G rx iq calibration, rx alpha: %d %d %d %d, rx theta %d %d %d %d\r\n", \
         GET_RG_RX_IQ_5100_ALPHA, GET_RG_RX_IQ_5100_THETA, \
         GET_RG_RX_IQ_5500_ALPHA, GET_RG_RX_IQ_5500_THETA, \
         GET_RG_RX_IQ_5700_ALPHA, GET_RG_RX_IQ_5700_THETA, \
         GET_RG_RX_IQ_5900_ALPHA, GET_RG_RX_IQ_5900_THETA); \
                                                                                            \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    SET_RG_5G_RFG_RXIQCAL(0x0); \
                                                                                            \
    SET_RG_5G_PGAG_RXIQCAL(0x3); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0xccc); \
    SET_RG_PHASE_RXIQ_1M(0xccc); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
    for (band = 0; band < 4; band ++){ \
        count=0; \
        SET_RG_SX5GB_CHANNEL(cal_ch_5g[band]); \
                                                                                            \
        if( band == 2 ) { \
            SET_RG_5G_TX_GAIN_RXIQCAL(0x2); \
        } else { \
            SET_RG_5G_TX_GAIN_RXIQCAL(0x0); \
        } \
        UDELAY(1); \
                                                                                            \
                                                                                            \
        SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_RXIQ); \
        UDELAY(250); \
        while (GET_RO_5G_RXIQ_DONE == 0){ \
            count ++; \
            if (count >100000) { \
                PRINT_ERR("%s: 5G band %d RXDC cal failed\r\n",__func__, band); \
                break; \
            } \
            UDELAY(1); \
        } \
                                                                                            \
                                                                                            \
                                                                                            \
        SET_RG_CAL_INDEX(CAL_IDX_NONE); \
    } \
    PRINT("--------------------------------------------%d\r\n", count); \
    _cal->rxiq_alpha[BAND_5100] = GET_RG_RX_IQ_5100_ALPHA; \
    _cal->rxiq_theta[BAND_5100] = GET_RG_RX_IQ_5100_THETA; \
    _cal->rxiq_alpha[BAND_5500] = GET_RG_RX_IQ_5500_ALPHA; \
    _cal->rxiq_theta[BAND_5500] = GET_RG_RX_IQ_5500_THETA; \
    _cal->rxiq_alpha[BAND_5700] = GET_RG_RX_IQ_5700_ALPHA; \
    _cal->rxiq_theta[BAND_5700] = GET_RG_RX_IQ_5700_THETA; \
    _cal->rxiq_alpha[BAND_5900] = GET_RG_RX_IQ_5900_ALPHA; \
    _cal->rxiq_theta[BAND_5900] = GET_RG_RX_IQ_5900_THETA; \
    PRINT("After 5G rx iq calibration, rx alpha: %d %d %d %d, rx theta %d %d %d %d\r\n", \
        _cal->rxiq_alpha[BAND_5100], _cal->rxiq_alpha[BAND_5500], \
        _cal->rxiq_alpha[BAND_5700], _cal->rxiq_alpha[BAND_5900], \
        _cal->rxiq_theta[BAND_5100], _cal->rxiq_theta[BAND_5500], \
        _cal->rxiq_theta[BAND_5700], _cal->rxiq_theta[BAND_5900]); \
} while (0)
#define TURISMOC_5G_RXIQ_CAL_BAND(_cal,_pa_band) \
do{ \
    int count = 0, alpha = 0, theta = 0; \
                                                                                            \
    SET_RG_SX5GB_RFCH_MAP_EN(1); \
                                                                                            \
    SET_RG_SX5GB_CHANNEL(cal_ch_5g[_pa_band-1]); \
    if( _pa_band == BAND_5700 ) { \
        SET_RG_5G_TX_GAIN_RXIQCAL(0x2); \
    } else { \
        SET_RG_5G_TX_GAIN_RXIQCAL(0x0); \
    } \
                                                                                            \
    switch (_pa_band){ \
        case BAND_5100: \
            alpha = GET_RG_RX_IQ_5100_ALPHA; \
            theta = GET_RG_RX_IQ_5100_THETA; \
            break; \
        case BAND_5500: \
            alpha = GET_RG_RX_IQ_5500_ALPHA; \
            theta = GET_RG_RX_IQ_5500_THETA; \
            break; \
        case BAND_5700: \
            alpha = GET_RG_RX_IQ_5700_ALPHA; \
            theta = GET_RG_RX_IQ_5700_THETA; \
            break; \
        case BAND_5900: \
            alpha = GET_RG_RX_IQ_5900_ALPHA; \
            theta = GET_RG_RX_IQ_5900_THETA; \
            break; \
        default: \
            break; \
    } \
    PRINT("--------------------------------------------\r\n"); \
    PRINT("before 5G band%d rx iq calibration, rx alpha: %d, rx theta %d\n", \
         _pa_band, alpha, theta); \
                                                                                            \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    SET_RG_5G_RFG_RXIQCAL(0x0); \
                                                                                            \
    SET_RG_5G_PGAG_RXIQCAL(0x3); \
                                                                                            \
    SET_RG_TONE_SCALE(0x80); \
                                                                                            \
    SET_RG_PRE_DC_AUTO(1); \
                                                                                            \
    SET_RG_TX_IQCAL_TIME(1); \
                                                                                            \
    SET_RG_PHASE_1M(0xccc); \
    SET_RG_PHASE_RXIQ_1M(0xccc); \
                                                                                            \
    SET_RG_ALPHA_SEL(2); \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_RXIQ); \
    UDELAY(250); \
    while (GET_RO_5G_RXIQ_DONE == 0){ \
        count ++; \
        if (count >100000) { \
            PRINT_ERR("%s: 5G band %d RXIQ cal failed\r\n",__func__, _pa_band); \
            break; \
        } \
        UDELAY(1); \
    } \
                                                                                            \
                                                                                            \
                                                                                            \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
                                                                                            \
    switch (_pa_band){ \
        case BAND_5100: \
            _cal->rxiq_alpha[_pa_band] = GET_RG_RX_IQ_5100_ALPHA; \
            _cal->rxiq_theta[_pa_band] = GET_RG_RX_IQ_5100_THETA; \
            break; \
        case BAND_5500: \
            _cal->rxiq_alpha[_pa_band] = GET_RG_RX_IQ_5500_ALPHA; \
            _cal->rxiq_theta[_pa_band] = GET_RG_RX_IQ_5500_THETA; \
            break; \
        case BAND_5700: \
            _cal->rxiq_alpha[_pa_band] = GET_RG_RX_IQ_5700_ALPHA; \
            _cal->rxiq_theta[_pa_band] = GET_RG_RX_IQ_5700_THETA; \
            break; \
        case BAND_5900: \
            _cal->rxiq_alpha[_pa_band] = GET_RG_RX_IQ_5900_ALPHA; \
            _cal->rxiq_theta[_pa_band] = GET_RG_RX_IQ_5900_THETA; \
            break; \
        default: \
            break; \
    } \
                                                                                            \
                                                                                            \
    PRINT("--------------------------------------------%d\r\n", count); \
                                                                                            \
    PRINT("After 5G band%d rx iq calibration, rx alpha: %d, rx theta %d\n", \
        _pa_band, _cal->rxiq_alpha[pa_band], _cal->rxiq_theta[pa_band]); \
} while (0)
#define TU_INIT_TURISMOC_CALI(_cal) \
do{ int i; \
    SET_RG_DPD_AM_EN(0); \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    REG32_W(ADR_WIFI_PADPD_5G_BB_GAIN_REG, 0x80808080); \
                                                                                            \
    TURISMOB_PRE_CAL; \
                                                                                            \
    TURISMOC_2P4G_RXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW20_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW40_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_TXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_TXIQ_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_RXIQ_CAL(_cal); \
    _cal->cal_iq_done[BAND_2G] = true; \
    TURISMO_INTER_CAL; \
    TURISMOC_5G_RXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_5G_TXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_5G_TXIQ_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_5G_RXIQ_CAL(_cal); \
    for (i = BAND_5100; i < MAX_BAND; i++) \
         _cal->cal_iq_done[i] = true; \
                                                                                            \
    TURISMOB_POST_CAL; \
    _cal->cal_done = true; \
} while(0)
#define TU_INIT_TURISMOC_2G_CALI(_cal) \
do{ \
    SET_RG_DPD_AM_EN(0); \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    TURISMOB_PRE_CAL; \
                                                                                            \
    TURISMOC_2P4G_RXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW20_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW40_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_TXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_TXIQ_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_RXIQ_CAL(_cal); \
    _cal->cal_iq_done[BAND_2G] = true; \
                                                                                            \
    TURISMOB_POST_CAL; \
    _cal->cal_done = true; \
} while(0)
#define TU_INIT_TURISMOC_FAST_CALI(_cal) \
do{ \
    SET_RG_DPD_AM_EN(0); \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    REG32_W(ADR_WIFI_PADPD_5G_BB_GAIN_REG, 0x80808080); \
                                                                                            \
    TURISMOB_PRE_CAL; \
                                                                                            \
    TURISMOC_2P4G_RXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW20_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW40_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_TXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_5G_RXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_5G_TXDC_CAL(_cal); \
                                                                                            \
    TURISMOB_POST_CAL; \
    _cal->cal_done = true; \
} while(0)
#define TU_INIT_TURISMOC_2G_FAST_CALI(_cal) \
do{ \
    SET_RG_DPD_AM_EN(0); \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    TURISMOB_PRE_CAL; \
                                                                                            \
    TURISMOC_2P4G_RXDC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW20_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_BW40_RXRC_CAL(_cal); \
    TURISMO_INTER_CAL; \
    TURISMOC_TXDC_CAL(_cal); \
                                                                                            \
    TURISMOB_POST_CAL; \
    _cal->cal_done = true; \
} while(0)
#define DUAL_BAND_ID 0x30303643
#define SINGLE_BAND_ID 0x30303644
#define SINGLE_BAND_PATCH(_xtal) \
do{ \
    if (REG32_R(ADR_CHIP_ID_2) == SINGLE_BAND_ID){ \
        SET_RG_SX_LPF_C2_WF(0xE); \
        SET_RG_XO_LDO_LEVEL(0x6); \
        SET_RG_SX_LDO_LO_LEVEL(0x3); \
        SET_RG_SX_VCO_RXOB_AW(0x1); \
        SET_RG_SX_VCO_TXOB_AW(0x1); \
        SET_RG_SX_CP_ISEL_WF(xtal_sx_cp_isel_wf[_xtal]); \
    } \
} while (0)
#define _RESTORE_CAL(_band,_cal) \
do{ \
     int i, wifi_dc_addr; \
                                                                                            \
    PRINT("Restore calibration result\n"); \
                                                                                            \
    for (i = 0; i < 21; i++) { \
       wifi_dc_addr = ADR_WF_DCOC_IDAC_REGISTER1+ (i << 2); \
       REG32_W(wifi_dc_addr,_cal->rxdc_2g[i]); \
       wifi_dc_addr = (ADR_5G_DCOC_IDAC_REGISTER1)+ (i << 2); \
       REG32_W(wifi_dc_addr,_cal->rxdc_5g[i]); \
    } \
                                                                                            \
                                                                                            \
    SET_RG_WF_RX_ABBCTUNE(_cal->rxrc_bw20); \
    SET_RG_WF_N_RX_ABBCTUNE(_cal->rxrc_bw40); \
                                                                                            \
                                                                                            \
    SET_RG_WF_TX_DAC_IOFFSET(_cal->txdc_i_2g); \
    SET_RG_WF_TX_DAC_QOFFSET(_cal->txdc_q_2g); \
                                                                                            \
                                                                                            \
    SET_RG_TX_IQ_2500_ALPHA(_cal->txiq_alpha[BAND_2G]); \
    SET_RG_TX_IQ_2500_THETA(_cal->txiq_theta[BAND_2G]); \
                                                                                            \
                                                                                            \
    SET_RG_RX_IQ_2500_ALPHA(_cal->rxiq_alpha[BAND_2G]); \
    SET_RG_RX_IQ_2500_THETA(_cal->rxiq_theta[BAND_2G]); \
                                                                                            \
                                                                                            \
    if ( _band == AG_BAND_BOTH) { \
                                                                                            \
                                                                                            \
        for (i = 0; i < 21; i++) { \
           wifi_dc_addr = (ADR_5G_DCOC_IDAC_REGISTER1)+ (i << 2); \
           REG32_W(wifi_dc_addr,_cal->rxdc_5g[i]); \
        } \
                                                                                            \
                                                                                            \
        SET_RG_5G_TX_DAC_IOFFSET(_cal->txdc_i_5g); \
        SET_RG_5G_TX_DAC_QOFFSET(_cal->txdc_q_5g); \
                                                                                            \
                                                                                            \
        SET_RG_TX_IQ_5100_ALPHA(_cal->txiq_alpha[BAND_5100]); \
        SET_RG_TX_IQ_5100_THETA(_cal->txiq_theta[BAND_5100]); \
        SET_RG_TX_IQ_5500_ALPHA(_cal->txiq_alpha[BAND_5500]); \
        SET_RG_TX_IQ_5500_THETA(_cal->txiq_theta[BAND_5500]); \
        SET_RG_TX_IQ_5700_ALPHA(_cal->txiq_alpha[BAND_5700]); \
        SET_RG_TX_IQ_5700_THETA(_cal->txiq_theta[BAND_5700]); \
        SET_RG_TX_IQ_5900_ALPHA(_cal->txiq_alpha[BAND_5900]); \
        SET_RG_TX_IQ_5900_THETA(_cal->txiq_theta[BAND_5900]); \
                                                                                            \
                                                                                            \
        SET_RG_RX_IQ_5100_ALPHA(_cal->rxiq_alpha[BAND_5100]); \
        SET_RG_RX_IQ_5100_THETA(_cal->rxiq_theta[BAND_5100]); \
        SET_RG_RX_IQ_5500_ALPHA(_cal->rxiq_alpha[BAND_5500]); \
        SET_RG_RX_IQ_5500_THETA(_cal->rxiq_theta[BAND_5500]); \
        SET_RG_RX_IQ_5700_ALPHA(_cal->rxiq_alpha[BAND_5700]); \
        SET_RG_RX_IQ_5700_THETA(_cal->rxiq_theta[BAND_5700]); \
        SET_RG_RX_IQ_5900_ALPHA(_cal->rxiq_alpha[BAND_5900]); \
        SET_RG_RX_IQ_5900_THETA(_cal->rxiq_theta[BAND_5900]); \
    } \
} while(0)
#define _INIT_TURISMOC_SYS(_patch,_cal) \
do{ \
    PRINT_INFO("RF table ver %s PHY table ver %s, common code ver %s \n", \
        SSV6006_TURISMOC_RF_TABLE_VER, SSV6006_TURISMOC_PHY_TABLE_VER, \
        SSV6006_TURISMOC_COMMON_CODE_VER); \
    LOAD_TURISMOC_RF_TABLE(_patch); \
    SET_RG_EN_IOTADC_160M(0); \
    PRINT("Set XTAL to %sM\n", xtal_type[_patch.xtal]); \
    SET_RG_DP_XTAL_FREQ(_patch.xtal); \
    SET_RG_SX_XTAL_FREQ(_patch.xtal); \
    TU_INIT_TURISMOC_PLL; \
    REG32_W(ADR_WIFI_PHY_COMMON_ENABLE_REG, 0); \
    LOAD_TURISMOC_PHY_TABLE; \
    SINGLE_BAND_PATCH(_patch.xtal); \
                                                                                            \
    SET_CLK_DIGI_SEL(_patch.cpu_clk); \
    UDELAY(1); \
} while(0)
#define INIT_TURISMOC_SYS(_patch,_band,_cal) \
do{ \
    _INIT_TURISMOC_SYS(_patch, _cal); \
                                                                                            \
    if (_cal->cal_done){ \
        _RESTORE_CAL(_band, _cal); \
    } else { \
        if (_band == G_BAND_ONLY){ \
            TU_INIT_TURISMOC_2G_FAST_CALI(_cal); \
        } else { \
            TU_INIT_TURISMOC_FAST_CALI(_cal); \
        } \
    } \
} while(0)
#define INIT_TURISMOC_SYS_IQK(_patch,_band,_cal) \
do{ \
    _INIT_TURISMOC_SYS(_patch, _cal); \
                                                                                            \
    if (_cal->cal_done){ \
        _RESTORE_CAL(_band, _cal); \
    } else { \
        if (_band == G_BAND_ONLY){ \
            TU_INIT_TURISMOC_2G_CALI(_cal); \
        } else { \
            TU_INIT_TURISMOC_CALI(_cal); \
        } \
    } \
} while(0)
#define SET_HT20_G_RESP_RATE \
do{ \
    SET_MTX_RESPFRM_RATE_90_B0(0x9090); \
    SET_MTX_RESPFRM_RATE_91_B1(0x9090); \
    SET_MTX_RESPFRM_RATE_92_B2(0x9090); \
    SET_MTX_RESPFRM_RATE_93_B3(0x9292); \
    SET_MTX_RESPFRM_RATE_94_B4(0x9292); \
    SET_MTX_RESPFRM_RATE_95_B5(0x9494); \
    SET_MTX_RESPFRM_RATE_96_B6(0x9494); \
    SET_MTX_RESPFRM_RATE_97_B7(0x9494); \
} while(0)
#define SET_HT40_G_RESP_RATE \
do { \
    SET_MTX_RESPFRM_RATE_90_B0(0xB0B0); \
    SET_MTX_RESPFRM_RATE_91_B1(0xB0B0); \
    SET_MTX_RESPFRM_RATE_92_B2(0xB0B0); \
    SET_MTX_RESPFRM_RATE_93_B3(0xB2B2); \
    SET_MTX_RESPFRM_RATE_94_B4(0xB2B2); \
    SET_MTX_RESPFRM_RATE_95_B5(0xB4B4); \
    SET_MTX_RESPFRM_RATE_96_B6(0xB4B4); \
    SET_MTX_RESPFRM_RATE_97_B7(0xB4B4); \
} while (0)
#define TU_SET_TURISMOC_BW(_ch_type) \
do{ \
                                                                                            \
    switch (_ch_type){ \
      case NL80211_CHAN_HT20: \
      case NL80211_CHAN_NO_HT: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(1); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (0 << RG_PRIMARY_CH_SIDE_SFT) | (0 << RG_SYSTEM_BW_SFT), 0, \
                (RG_PRIMARY_CH_SIDE_I_MSK & RG_SYSTEM_BW_I_MSK)); \
                                                                                            \
            SET_REG(ADR_DIGITAL_ADD_ON_0, \
                (0 << RG_40M_MODE_SFT) | (0 << RG_LO_UP_CH_SFT), 0, \
                (RG_40M_MODE_I_MSK & RG_LO_UP_CH_I_MSK)); \
                                                                                            \
            SET_HT20_G_RESP_RATE; \
            break; \
                                                                                            \
   case NL80211_CHAN_HT40MINUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (1 << RG_PRIMARY_CH_SIDE_SFT) | (1 << RG_SYSTEM_BW_SFT), 0, \
                (RG_PRIMARY_CH_SIDE_I_MSK & RG_SYSTEM_BW_I_MSK)); \
                                                                                            \
            SET_REG(ADR_DIGITAL_ADD_ON_0, \
                (1 << RG_40M_MODE_SFT) | (0 << RG_LO_UP_CH_SFT), 0, \
                (RG_40M_MODE_I_MSK & RG_LO_UP_CH_I_MSK)); \
                                                                                            \
            SET_HT40_G_RESP_RATE; \
            break; \
   case NL80211_CHAN_HT40PLUS: \
                                                                                            \
            SET_MTX_BLOCKTX_IGNORE_TOMAC_CCA_ED_SECONDARY(0); \
                                                                                            \
            SET_REG(ADR_WIFI_PHY_COMMON_SYS_REG, \
                (0 << RG_PRIMARY_CH_SIDE_SFT) | (1 << RG_SYSTEM_BW_SFT), 0, \
                (RG_PRIMARY_CH_SIDE_I_MSK & RG_SYSTEM_BW_I_MSK)); \
                                                                                            \
            SET_REG(ADR_DIGITAL_ADD_ON_0, \
                (1 << RG_40M_MODE_SFT) | (1 << RG_LO_UP_CH_SFT), 0, \
                (RG_40M_MODE_I_MSK & RG_LO_UP_CH_I_MSK)); \
                                                                                            \
            SET_HT40_G_RESP_RATE; \
            break; \
      default: \
            break; \
    } \
} while(0)
#define TURISMOC_SET_5G_CHANNEL(_ch) \
do{ \
    int regval; \
                                                                                            \
    SET_RG_RF_5G_BAND(1); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX5GB_RFCH_MAP_EN(1); \
                                                                                            \
    regval = GET_RG_SX5GB_CHANNEL; \
                                                                                            \
    if (regval == _ch){ \
        if (_ch != 36) \
           SET_RG_SX5GB_CHANNEL(36); \
        else \
           SET_RG_SX5GB_CHANNEL(40); \
                                                                                            \
    } \
    UDELAY(100); \
                                                                                            \
                                                                                            \
    SET_RG_SX5GB_CHANNEL(_ch); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(7); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
                                                                                            \
                                                                                            \
    SET_RG_SOFT_RST_N_11GN_RX(1); \
} while(0)
#define TURISMOC_SET_2G_CHANNEL(_ch) \
do{ \
    int regval; \
                                                                                            \
    SET_RG_RF_5G_BAND(0); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
                                                                                            \
                                                                                            \
    SET_RG_SX_RFCH_MAP_EN(1); \
                                                                                            \
    regval = GET_RG_SX_CHANNEL; \
                                                                                            \
    if (regval == _ch){ \
        if (_ch != 1) \
           SET_RG_SX_CHANNEL(1); \
        else \
           SET_RG_SX_CHANNEL(11); \
                                                                                            \
    } \
    UDELAY(100); \
                                                                                            \
                                                                                            \
    SET_RG_SX_CHANNEL(_ch); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(0); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(3); \
                                                                                            \
                                                                                            \
    SET_RG_MODE_MANUAL(0); \
                                                                                            \
                                                                                            \
    SET_RG_SOFT_RST_N_11B_RX(1); \
    SET_RG_SOFT_RST_N_11GN_RX(1); \
} while(0)
#define MULTIPLIER 1024
#define DPD_SET_TXSCALE_GET_RESULT(_rg_tx_scale,_am,_pm) \
do { \
    int regval; \
                                                                                            \
    SET_RG_TX_SCALE(_rg_tx_scale); \
    UDELAY(200); \
    SET_RG_RX_PADPD_LATCH(1); \
    UDELAY(10); \
    SET_RG_RX_PADPD_LATCH(0); \
    regval= REG32_R(ADR_WIFI_PADPD_CAL_RX_RO); \
    _am = (regval >>16) & 0x1ff; \
    _pm = regval & 0x1fff; \
} while(0)
#define PRE_PADPD(_pa_band,_init_gain) \
do { \
    SET_RG_DPD_AM_EN(0); \
    SET_RG_TXGAIN_PHYCTRL(1); \
                                                                                            \
    SET_RG_MODE_MANUAL(1); \
    SET_RG_MODE(MODE_STANDBY); \
                                                                                            \
    if (_pa_band == 0){ \
        SET_REG(ADR_SX_2_4GB_5GB_REGISTER_INT3BIT___CH_TABLE, \
            (0x6 << RG_SX_CHANNEL_SFT) | (0x1 << RG_SX_RFCH_MAP_EN_SFT), 0, \
        (RG_SX_CHANNEL_I_MSK & RG_SX_RFCH_MAP_EN_I_MSK)); \
        SET_RG_TX_GAIN_DPDCAL(6); \
        SET_RG_PGAG_DPDCAL(_init_gain); \
        SET_RG_RFG_DPDCAL(0); \
        SET_RG_TX_GAIN(PAPDP_GAIN_SETTING_2G); \
        SET_RG_DPD_BB_SCALE_2500(0x80); \
    } else { \
        SET_RG_SX5GB_RFCH_MAP_EN(1); \
        SET_RG_SX5GB_CHANNEL(cal_ch_5g[_pa_band-1]); \
                                                                                            \
        SET_RG_5G_PGAG_DPDCAL(_init_gain); \
        SET_RG_5G_RFG_DPDCAL(0); \
                                                                                            \
        switch (_pa_band){ \
            case BAND_5100: \
                SET_RG_5G_TX_PAFB_EN_F0(0); \
                SET_RG_5G_TX_GAIN_F0(PAPDP_GAIN_SETTING); \
                SET_RG_5G_TX_GAIN_DPDCAL(PAPDP_GAIN_SETTING); \
                SET_RG_DPD_BB_SCALE_5100(0x80); \
                break; \
            case BAND_5500: \
                SET_RG_5G_TX_PAFB_EN_F1(0); \
                SET_RG_5G_TX_GAIN_F1(PAPDP_GAIN_SETTING); \
                SET_RG_5G_TX_GAIN_DPDCAL(PAPDP_GAIN_SETTING); \
                SET_RG_DPD_BB_SCALE_5500(0x80); \
                break; \
            case BAND_5700: \
                SET_RG_5G_TX_PAFB_EN_F2(0); \
                SET_RG_5G_TX_GAIN_F2(PAPDP_GAIN_SETTING_F2); \
                SET_RG_5G_TX_GAIN_DPDCAL(PAPDP_GAIN_SETTING_F2); \
                SET_RG_DPD_BB_SCALE_5700(0x80); \
                break; \
            case BAND_5900: \
                SET_RG_5G_TX_PAFB_EN_F3(0); \
                SET_RG_5G_TX_GAIN_F3(PAPDP_GAIN_SETTING); \
                SET_RG_5G_TX_GAIN_DPDCAL(PAPDP_GAIN_SETTING); \
                SET_RG_DPD_BB_SCALE_5900(0x80); \
                break; \
            default: \
                break; \
        } \
    } \
    SET_RG_BB_SIG_EN(1); \
                                                                                            \
    SET_RG_DC_RM_BYP(1); \
                                                                                            \
    SET_RG_TX_IQ_SRC(2); \
    SET_RG_TX_BB_SCALE_MANUAL(1); \
                                                                                            \
    SET_RG_TX_SCALE(0x80); \
    SET_RG_TONE_1_RATE(0xccc); \
    SET_RG_TONE_SEL(1); \
                                                                                            \
    SET_RG_RX_PADPD_TONE_SEL(0); \
    SET_RG_RX_PADPD_DATA_SEL(0); \
    SET_RG_RX_PADPD_LEAKY_FACTOR(7); \
    SET_RG_RX_PADPD_EN(1); \
                                                                                            \
                                                                                            \
    SET_RG_MODE(MODE_CALIBRATION); \
                                                                                            \
    if (_pa_band == 0) { \
       SET_RG_CAL_INDEX(CAL_IDX_WIFI2P4G_PADPD); \
    } else { \
       SET_RG_CAL_INDEX(CAL_IDX_WIFI5G_PADPD); \
    } \
                                                                                            \
    UDELAY(100); \
} while(0)
#define POST_PADPD \
do { \
    SET_RG_CAL_INDEX(CAL_IDX_NONE); \
    SET_RG_MODE(MODE_STANDBY); \
    SET_RG_MODE_MANUAL(0); \
                                                                                            \
    SET_RG_BB_SIG_EN(0); \
    SET_RG_DC_RM_BYP(0); \
    SET_RG_TX_IQ_SRC(0); \
    SET_RG_TX_BB_SCALE_MANUAL(0); \
                                                                                            \
    SET_RG_TX_SCALE(0x80); \
                                                                                            \
    SET_RG_TONE_SEL(0); \
                                                                                            \
    SET_RG_RX_PADPD_EN(0); \
} while(0)
#define CHECK_PADPD_GAIN(_pa_band,_init_gain,_ret) \
do{ \
    int rg_tx_scale; \
    int am, pm; \
                                                                                            \
    PRINT("check PA DPD on band %d\n", pa_band); \
                                                                                            \
    PRE_PADPD(_pa_band, _init_gain); \
                                                                                            \
    rg_tx_scale = 48+(MAX_PADPD_TONE - 1-5)*4; \
    DPD_SET_TXSCALE_GET_RESULT( rg_tx_scale, am, pm); \
    if (am >=510) { \
        *_ret = 1; \
        PRINT("gain probe fail, am %d\n", am); \
    } \
                                                                                            \
    POST_PADPD; \
                                                                                            \
} while(0)
#define START_PADPD(_val,_pa_band,_init_gain,dpd_bbscale,_ret) \
do { \
    int i, rg_tx_scale, regval; \
    int am, pm; \
    int slope_ini = 0, phase_ini = 0; \
    int padpd_am = 0, padpd_pm = 0; \
    u32 addr_am = 0, addr_pm = 0 , mask_am = 0, mask_pm = 0; \
                                                                                            \
    PRINT("start PA DPD on band %d\r\n", _pa_band); \
                                                                                            \
    PRE_PADPD(_pa_band, _init_gain); \
                                                                                            \
    for (i = 0; i < MAX_PADPD_TONE; i++) { \
                                                                                            \
        if( i < 6 ){ \
            rg_tx_scale = (i+1)*8; \
        } else { \
            rg_tx_scale = 48+(i-5)*4; \
        } \
        DPD_SET_TXSCALE_GET_RESULT( rg_tx_scale, am, pm); \
        if (am >=510) { \
            *_ret = 1; \
            break; \
        } \
        if ( i == 0) { \
            slope_ini = (am * MULTIPLIER) / rg_tx_scale; \
            phase_ini = pm; \
                                                                                                \
        } \
                                                                                                \
        if (am != 0) \
           padpd_am = (512 * rg_tx_scale * slope_ini ) / (am*MULTIPLIER); \
                                                                                            \
        if (padpd_am > 1023){ \
            padpd_am = 1023; \
        } \
                                                                                            \
        padpd_pm = (phase_ini >= pm) ? (phase_ini - pm) : (phase_ini - pm + 8192); \
                                                                                            \
        PRINT("index %d, padpd_am %d, padpd_pm 0x%04x, ", i, padpd_am, padpd_pm); \
        if ((i%2 == 1) || (i == (MAX_PADPD_TONE -1))) PRINT("\r\n"); \
                                                                                            \
        addr_am = padpd_am_addr_table[pa_band][(i >> 1)]; \
        mask_am = am_mask[i%2]; \
        addr_pm = padpd_pm_addr_table[pa_band][(i >> 1)]; \
        mask_pm = pm_mask[i%2]; \
                                                                                            \
        regval = REG32_R(addr_am); \
        REG32_W(addr_am, (regval & mask_am) | ((padpd_am) << ((i & 0x1)*16)) ); \
        if (i & 0x1){ \
            _val->am[ (i >> 1)] = (regval & mask_am) | ((padpd_am) << ((i & 0x1)*16)); \
        } \
                                                                                            \
        regval = REG32_R(addr_pm); \
        REG32_W(addr_pm, (regval & mask_pm) | ((padpd_pm) << ((i & 0x1)*16)) ); \
                                                                                            \
        if (i & 0x1){ \
            _val->pm[ (i >> 1)] = (regval & mask_pm) | ((padpd_pm) << ((i & 0x1)*16)); \
        } \
    } \
                                                                                            \
    POST_PADPD; \
                                                                                            \
    if (*_ret != 1){ \
        switch (_pa_band){ \
            case BAND_2G: \
                SET_RG_DPD_BB_SCALE_2500(dpd_bbscale); \
                break; \
            case BAND_5100: \
                SET_RG_DPD_BB_SCALE_5100(dpd_bbscale); \
                break; \
            case BAND_5500: \
                 SET_RG_DPD_BB_SCALE_5500(dpd_bbscale); \
                                                                                            \
                break; \
            case BAND_5700: \
                 SET_RG_DPD_BB_SCALE_5700(dpd_bbscale); \
                                                                                            \
                break; \
            case BAND_5900: \
                 SET_RG_DPD_BB_SCALE_5900(dpd_bbscale); \
                                                                                            \
                break; \
            default: \
                break; \
        } \
                                                                                            \
        SET_RG_DPD_AM_EN(1); \
        SET_RG_TXGAIN_PHYCTRL(0); \
    } \
} while(0)
#define TU_RESTORE_DPD(_dpd) \
do{ \
    int i; \
    u32 addr_am, addr_pm; \
                                                                                            \
    for (i = 0; i < (MAX_PADPD_TONE/2) ; i++) { \
                                                                                            \
        addr_am = padpd_am_addr_table[_dpd->current_band][i]; \
        addr_pm = padpd_pm_addr_table[_dpd->current_band][i]; \
                                                                                            \
        REG32_W(addr_am, _dpd->val[_dpd->current_band].am[i] ); \
        REG32_W(addr_pm, _dpd->val[_dpd->current_band].pm[i] ); \
                                                                                            \
    } \
                                                                                            \
    switch (_dpd->current_band){ \
        case BAND_2G: \
            SET_RG_DPD_BB_SCALE_2500(_dpd->bbscale[0]); \
            break; \
        case BAND_5100: \
            SET_RG_DPD_BB_SCALE_5100(_dpd->bbscale[1]); \
            break; \
        case BAND_5500: \
            SET_RG_DPD_BB_SCALE_5500(_dpd->bbscale[2]); \
            break; \
        case BAND_5700: \
            SET_RG_DPD_BB_SCALE_5700(_dpd->bbscale[3]); \
            break; \
        case BAND_5900: \
            SET_RG_DPD_BB_SCALE_5900(_dpd->bbscale[4]); \
            break; \
        default: \
            break; \
    } \
    SET_RG_DPD_AM_EN(1); \
    SET_RG_TXGAIN_PHYCTRL(0); \
} while(0)
#define VERIFY_DPD(_dpd,_pa_band) \
do { \
    u8 hw_dpd_bbscale; \
                                                                                            \
    switch (_pa_band){ \
        case BAND_2G: \
            hw_dpd_bbscale = GET_RG_DPD_BB_SCALE_2500; \
            break; \
        case BAND_5100: \
            hw_dpd_bbscale = GET_RG_DPD_BB_SCALE_5100; \
            break; \
        case BAND_5500: \
            hw_dpd_bbscale = GET_RG_DPD_BB_SCALE_5500; \
            break; \
        case BAND_5700: \
            hw_dpd_bbscale = GET_RG_DPD_BB_SCALE_5700; \
            break; \
        case BAND_5900: \
            hw_dpd_bbscale = GET_RG_DPD_BB_SCALE_5900; \
            break; \
        default: \
            hw_dpd_bbscale = 0; \
            break; \
    } \
                                                                                            \
    if (hw_dpd_bbscale == 0x80){ \
                                                                                            \
        PRINT("HW DPD value changed, restore DPD\n"); \
        TU_RESTORE_DPD(_dpd); \
    } \
} while(0)
#ifdef COMMON_FOR_REDBULL
#define _VERIFY_DPD(_dpd,_pa_band) 
#else
#define _VERIFY_DPD(_dpd,_pa_band) VERIFY_DPD(_dpd, _pa_band)
#endif
#define TURN_OFF_PADPD(_dpd,_ch) \
do { \
    int pa_band; \
                                                                                            \
    pa_band = ssv6006_get_pa_band(_ch); \
    _dpd->dpd_disable[pa_band] = true; \
} while(0)
#define TURN_ON_PADPD(_dpd,_ch) \
do { \
    int pa_band; \
                                                                                            \
    pa_band = ssv6006_get_pa_band(_ch); \
    _dpd->dpd_disable[pa_band] = false; \
} while(0)
#define CHECK_PADPD(_dpd,_pa_band) \
do { \
    int ret = 0; \
    struct ssv6006dpd *val; \
    int *pret = &ret; \
    u8 dpd_bbscale = 0; \
                                                                                            \
    val = &_dpd->val[_pa_band]; \
    dpd_bbscale = _dpd->bbscale[_pa_band]; \
                                                                                            \
    if (_dpd->dpd_disable[_pa_band] == true) { \
        if (_dpd->current_band != _pa_band){ \
                                                                                            \
            SET_RG_DPD_AM_EN(0); \
            SET_RG_TXGAIN_PHYCTRL(0); \
        } \
    } else { \
                                                                                            \
        if ( _dpd->dpd_done[_pa_band] == false) { \
            int init_gain = 4; \
                                                                                            \
            SET_RG_DPD_AM_EN(0); \
            if (_pa_band != 0) init_gain = 2; \
            PRINT("Start PADPD on band %d ,init gain %d\r\n", _pa_band, init_gain); \
            while (1){ \
                ret = 0; \
                START_PADPD(val, _pa_band, init_gain, dpd_bbscale, pret); \
                if (!ret){ \
                    PRINT("PA DPD done!!\r\n"); \
                    _dpd->dpd_done[_pa_band] = true; \
                    break; \
                } \
                init_gain--; \
                PRINT("\r\nFailed on band %d, Lower gain to %d\r\n", pa_band, init_gain); \
                if (init_gain < 0) { \
                    SET_RG_DPD_AM_EN(0); \
                    PRINT_ERR("WARNING:PADPD FAIL on band %d\r\n", _pa_band); \
                    break; \
                } \
            } \
        } else { \
            _VERIFY_DPD(_dpd, _pa_band); \
        } \
    } \
    _dpd->current_band = _pa_band; \
} while(0)
#define START_IQK(_sh,_cal,_pa_band) \
do { \
                                                                                            \
   SET_RG_DPD_AM_EN(0); \
   SET_RG_TXGAIN_PHYCTRL(1); \
   if (_pa_band == BAND_2G){ \
       turismo_pre_cal(_sh); \
       TURISMOC_TXIQ_CAL(_cal); \
       turismo_inter_cal(_sh); \
       TURISMOC_RXIQ_CAL(_cal); \
       turismo_post_cal(_sh); \
   } else { \
       turismo_pre_cal(_sh); \
       TURISMOC_5G_TXIQ_CAL_BAND(_cal, _pa_band); \
       turismo_inter_cal(_sh); \
       TURISMOC_5G_RXIQ_CAL_BAND(_cal, _pa_band); \
       turismo_post_cal(_sh); \
   } \
} while(0)
#ifdef COMMON_FOR_REDBULL
#define CHECK_IQK(_cal,_pa_band) 
#else
#ifdef COMMON_FOR_SMAC
#define CHECK_IQK(_cal,_pa_band) \
do { \
    if (_cal->cal_iq_done[_pa_band] == false) { \
        PRINT("do iqk on band %d\n", _pa_band); \
                                                                                            \
        START_IQK(sh, _cal, _pa_band); \
        _cal->cal_iq_done[_pa_band] = true; \
    } \
} while(0)
#else
#define CHECK_IQK(_cal,_pa_band) \
do { \
    SSV_HW *sh = NULL; \
    if (_cal->cal_iq_done[_pa_band] == false) { \
        PRINT("do iqk on band %d\n", _pa_band); \
                                                                                            \
        START_IQK(sh, _cal, _pa_band); \
        _cal->cal_iq_done[_pa_band] = true; \
    } \
} while (0)
#endif
#endif
#define CHECK_PADPD_IQK(_dpd,_cal,_ch) \
do { \
    int pa_band = 0; \
                                                                                            \
    pa_band = ssv6006_get_pa_band(_ch); \
                                                                                            \
    CHECK_IQK(_cal, pa_band); \
    CHECK_PADPD(_dpd, pa_band); \
} while(0)
#define REMOVE_SPUR_PATCH(_dpd,_cal) \
do{ \
    if (_dpd->spur_patched){ \
        SET_RG_EN_RX_PADSW(0); \
        SET_RG_SC_CTRL0(0); \
        SET_RG_ERASE_SC_NUM0(0x7f); \
        SET_RG_SC_CTRL1(0); \
        SET_RG_ERASE_SC_NUM1(0x7f); \
                                                                                            \
        SET_RG_ATCOR16_RATIO_CCD(0x30); \
        SET_RG_ATCOR16_CCA_GAIN(0x0); \
        _dpd->spur_patched = false; \
    } \
} while (0)
#define CHECK_SPUR(_dpd,_cal,_ch,_ch_type) \
do { \
    int pa_band = 0; \
    int xtal, chip_id; \
                                                                                            \
    xtal = GET_RG_DP_XTAL_FREQ; \
    chip_id = REG32_R(ADR_CHIP_ID_2); \
                                                                                            \
    if (chip_id == DUAL_BAND_ID){ \
        if (xtal != XTAL40M) \
            break; \
    } \
    pa_band = ssv6006_get_pa_band(_ch); \
    if (_cal->cal_iq_done[pa_band] == true){ \
                                                                                            \
     if ((_ch >=13) || ((_ch >= 9) && (_ch_type == NL80211_CHAN_HT40PLUS))){ \
                                                                                         \
         SET_RG_EN_RX_PADSW(1); \
         _dpd->spur_patched = true; \
        } \
                                                                                            \
        if (chip_id == SINGLE_BAND_ID){ \
            if ((_ch == 13) \
                && ((_ch_type == NL80211_CHAN_NO_HT) || (_ch_type == NL80211_CHAN_HT20))){ \
                SET_RG_SC_CTRL0(1); \
                SET_RG_ERASE_SC_NUM0(22); \
                SET_RG_SC_CTRL1(1); \
                SET_RG_ERASE_SC_NUM1(23); \
            } else if (((_ch == 9) && (_ch_type == NL80211_CHAN_HT40PLUS)) \
                        || ((_ch == 13) && (_ch_type == NL80211_CHAN_HT40MINUS))) { \
                SET_RG_SC_CTRL0(1); \
                SET_RG_ERASE_SC_NUM0(52); \
                SET_RG_SC_CTRL1(1); \
                SET_RG_ERASE_SC_NUM1(53); \
            } \
        } \
        if ((ch == 13) && \
            ((_ch_type == NL80211_CHAN_NO_HT) || (_ch_type == NL80211_CHAN_HT20) || \
                (_ch_type == NL80211_CHAN_HT40MINUS))){ \
                                                                                            \
         SET_RG_ATCOR16_RATIO_CCD(0x18); \
            SET_RG_ATCOR16_CCA_GAIN(0x1); \
        } \
    } \
} while (0)
#define TU_CHANGE_TURISMOC_CHANNEL(_ch,_ch_type,_dpd,_cal) \
do{ \
    const char *chan_type[]={"NL80211_CHAN_NO_HT", \
     "NL80211_CHAN_HT20", \
     "NL80211_CHAN_HT40MINUS", \
     "NL80211_CHAN_HT40PLUS"}; \
                                                                                            \
    PRINT("%s: ch %d, type %s\r\n", __func__, _ch, chan_type[_ch_type]); \
    SET_RG_SOFT_RST_N_11B_RX(0); \
    SET_RG_SOFT_RST_N_11GN_RX(0); \
                                                                                            \
    REMOVE_SPUR_PATCH(_dpd, _cal); \
                                                                                            \
    if (REG32_R(ADR_WIFI_PHY_COMMON_ENABLE_REG) != 0) { \
                                                                                            \
        CHECK_PADPD_IQK(_dpd, _cal, _ch); \
    } \
                                                                                            \
    TU_SET_TURISMOC_BW(_ch_type); \
    if ( _ch <=14 && _ch >=1){ \
        SET_SIFS(10); \
     SET_SIGEXT(6); \
        CHECK_SPUR(_dpd, _cal, _ch, _ch_type); \
        TURISMOC_SET_2G_CHANNEL( _ch); \
    } else if (_ch >=34){ \
     SET_SIFS(16); \
     SET_SIGEXT(0); \
        TURISMOC_SET_5G_CHANNEL(_ch); \
    } else { \
        PRINT("invalid channel %d\r\n", _ch); \
    } \
                                                                                            \
} while(0)
#define TU_DISABLE_TURISMOC_DPD(_dpd) \
do { \
    int pa_band; \
                                                                                            \
    for (pa_band = 0; pa_band <= 4; pa_band++){ \
                                                                                            \
        _dpd->dpd_done[pa_band] = true; \
    } \
    SET_RG_DPD_AM_EN(0); \
    SET_RG_TXGAIN_PHYCTRL(0); \
} while (0)
#ifdef COMMON_FOR_REDBULL
#define TU_ENABLE_TURISMOC_DPD(_ch,_dpd) \
do { \
    int pa_band; \
                                                                                            \
    for (pa_band = 0; pa_band <= 4; pa_band++){ \
                                                                                            \
        _dpd->dpd_done[pa_band] = false; \
    } \
} while (0)
#else
#define TU_ENABLE_TURISMOC_DPD(_ch,_dpd) \
do { \
    int pa_band; \
                                                                                            \
    for (pa_band = 0; pa_band <= 4; pa_band++){ \
                                                                                            \
        _dpd->dpd_done[pa_band] = false; \
    } \
    CHECK_PADPD(_dpd, _ch); \
                                                                                            \
} while (0)
#endif
#define TU_RESTART_RF_PHY(_band,_cal,_patch) \
do { \
    LOAD_TURISMOC_RF_TABLE(_patch); \
    REG32_W(ADR_WIFI_PHY_COMMON_ENABLE_REG, 0); \
    LOAD_TURISMOC_PHY_TABLE; \
    SINGLE_BAND_SPUR_PATCH(_patch.xtal); \
    if (_cal->cal_done){ \
        _RESTORE_CAL(_band, _cal); \
    } else { \
        if (_band == G_BAND_ONLY){ \
            TU_INIT_TURISMOC_2G_FAST_CALI(_cal); \
        } else { \
            TU_INIT_TURISMOC_FAST_CALI(_cal); \
        } \
    } \
} while(0)
#endif
