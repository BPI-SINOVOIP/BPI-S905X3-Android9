#include <config.h>
#ifdef _NO_OS_
#include <ssv_lib.h>
#define LOG_PRINTF printf
#else
#include <soc_global.h>
#include <log.h>
#endif
#include "drv_phy.h"
#include <pbuf.h>
#include <mbox/drv_mbox.h>
#include <dbg_timer/dbg_timer.h>

// Tx Cali related setting
#define _CALI_TXLO_ROUND_       2
#define _CALI_TXIQ_ROUND_       2

// Rx Iq Cali related setting
#define _CALI_RXIQ_ROUND_       2
#define _CALI_RXIQ_ALPHA_RPT_N_ 4
#define _CALI_RXIQ_THETA_RPT_N_ 16
#define _CALI_RXIQ_ALPHA_LB_    -12
#define _CALI_RXIQ_ALPHA_UB_    12
#define _CALI_RXIQ_ALPHA_WRAP_  32

// Papd Cali reltated setting
#define _CALI_PAPD_TX_GAIN_     12      // 1x4: high power, 3x4: low current
#define _CALI_PAPD_PABIAS_      6       // 14 high power mode, 6: low current mode
#define _CALI_PAPD_PACASCODE_   0       // 1: high power mode, 0: low current mode
#define _CALI_PAPD_OBSR_N_      26      // number of observation points

#define _CALI_PAPD_HA_THD_          0x0d0    
#define _CALI_PAPD_LA_RPT_N_LOG_    3
#define _CALI_PAPD_HA_RPT_N_LOG_    2
#define _CALI_PAPD_LA_RPT_N_        (1 << _CALI_PAPD_LA_RPT_N_LOG_)
#define _CALI_PAPD_HA_RPT_N_        (1 << _CALI_PAPD_HA_RPT_N_LOG_)
#define _CALI_PAPD_RPT_N_           2

// #define _CALI_PAPD_GAIN_REF_POINT_   1       // 1: 0x040
#define _CALI_PAPD_GAIN_REF_POINT_      2       // 2: 0x060
#define _CALI_PAPD_PH_REF_POINT_        2
#define _CALI_PAPD_GAIN_UNIT_   512
#define _CALI_PAPD_GAIN_MAX_    800

#define _CALI_PAPD_PH_MAX_      4095
#define _CALI_PAPD_PH_MIN_      -4096
#define _CALI_PAPD_PH_NULL_     0
#define _CALI_PAPD_PH_WRAP_     8192

#define _CALI_PAPD_AM_MAX_      511
#define _CALI_PAPD_AM_BD_       _CALI_PAPD_AM_MAX_ - 20
#define _CALI_PAPD_YDIF_MIN_    2

#define _RO_IQCAL_O_MAX_    524287
#define _RO_IQCAL_O_WRAP_   1048576

#define _RO_IQCAL_O_ABS_MAX_    _RO_IQCAL_O_MAX_+1
#define _RO_ADC_ABS_MAX_        128

#define _CALI_PAPD_PHO_SWF_ 18          // phase read out swf
#define _CALI_PAPD_PHI_SWF_ 12          // phase set swf

#define _CALI_PAPD_X_W_     9
#define _CALI_PAPD_X_WF_    9
#define _CALI_PAPD_X_MAX_   ((1 << _CALI_PAPD_X_W_) -1)
#define _CALI_PAPD_Y_W_     9
#define _CALI_PAPD_Y_WF_    9
#define _CALI_PAPD_Y_MAX_   ((1 << _CALI_PAPD_Y_W_) -1)

#define _CALI_PAPD_Q_SW_    11
#define _CALI_PAPD_Q_SWF_   9

#define _CALI_PAPD_GAIN_W_  10
#define _CALI_PAPD_GAIN_WF_ 9

#define _CALI_PAPD_RTBL_SEL_    0xfff
#define _CALI_PAPD_RTBL_SIZE_CFG_EN_    2
#define _CALI_PAPD_RTBL_SIZE_CFG_SCALE_ 4
#define _CALI_PAPD_RTBL_SIZE_GAIN_  26
#define _CALI_PAPD_RTBL_SIZE_PH_    26

#define _CALI_PAPD_RTBL_IDX_CFG_EN_     0
#define _CALI_PAPD_RTBL_IDX_CFG_SCALE_  _CALI_PAPD_RTBL_IDX_CFG_EN_    + _CALI_PAPD_RTBL_SIZE_CFG_EN_
#define _CALI_PAPD_RTBL_IDX_GAIN_       _CALI_PAPD_RTBL_IDX_CFG_SCALE_ + _CALI_PAPD_RTBL_SIZE_CFG_SCALE_
#define _CALI_PAPD_RTBL_IDX_PH_         _CALI_PAPD_RTBL_IDX_GAIN_      + _CALI_PAPD_RTBL_SIZE_GAIN_

// Rx DC Cali related setting
#define _CALI_RXRC_SETTLE_N_    10
#define _CALI_RXDC_RTBL_SIZE_I_ 16
#define _CALI_RXDC_RTBL_SIZE_Q_ 16
#define _CALI_RXDC_RTBL_IDX_Q_  _CALI_RXDC_RTBL_SIZE_I_ // Q-part table index offset

#define _CALI_TSSI_SAR_TARGET_  38      // RF: -3dB, BB: -9 dB 
#define _CALI_TSSI_DIV_         3
#define _CALI_TSSI_OBSR_N_      9
#define _CALI_TSSI_SAR_DIF_MAX_ 128
#define _CALI_TSSI_BB_DEF_GAIN_ 128

// Rx RC Cali related setting
#define _CALI_RXRC_TX_ISCALE_   0x0B    // u8.7f
#define _CALI_RXRC_TX_OSCALE_   0x30    // u8.7f
#define _CALI_RXRC_RX_ISCALE_   0x02
#define _CALI_RXRC_RX_OSCALE_   0x02
#define _CALI_RXRC_TX_IFREQ_    0x08    //  8*0.3125Mhz =  2.5Mhz
// #define _CALI_RXRC_TX_OFREQ_    0x28    // 40*0.3125Mhz = 12.5Mhz
// #define _CALI_RXRC_RX_OFREQ_    0x18    // 24*0.3125Mhz =  7.5Mhz
// #define _CALI_RXRC_TX_OFREQ_    0x38    // 40*0.3125Mhz = 17.5Mhz
// #define _CALI_RXRC_RX_OFREQ_    0x08    //  8*0.3125Mhz =  2.5Mhz
#define _CALI_RXRC_TX_OFREQ_    0x30    // 48*0.3125Mhz = 15.0Mhz
#define _CALI_RXRC_RX_OFREQ_    0x10    // 16*0.3125Mhz =  5.0Mhz

/// #define _CALI_RXRC_M21DBC_NUME_ 0xF2003EDA  // Numerator of -21dBc, and consider the TX scale diff between inband/outband
                                            // fprintf('0x%X\n', ...
                                            // round(10^-2.1*2^32* ((_CALI_RXRC_TX_OSCALE_*_CALI_RXRC_RX_OSCALE_) / (_CALI_RXRC_TX_ISCALE_*_CALI_RXRC_RX_ISCALE_))^2));
#define _CALI_RXRC_M21DBC_NUME_ 0x9A25D026  // Numerator of -15dBc, and consider the TX scale diff between inband/outband
                                            // fprintf('0x%X\n', ...
                                            // round(10^-1.5*2^32* ((_CALI_RXRC_TX_OSCALE_*_CALI_RXRC_RX_OSCALE_) / (_CALI_RXRC_TX_ISCALE_*_CALI_RXRC_RX_ISCALE_))^2));
#define _CALI_RXRC_M21DBC_DENO_ 32          // Denominator, 2^32

#define _CALI_RXRC_ABBCTUNE_MIN_    0
#define _CALI_RXRC_ABBCTUNE_MAX_    63

#define _CALI_RXRC_RTBL_SEL_        0xfff
#define _CALI_RXRC_RESULT0_DEF_     27  // rg_abbctune

#define _CALI_INIT_RTBL_SEL_    0xfff

#define _CALI_ADDR_BASE_        0xce000000

#define _CALI_ANT_CTRL_         REG32(0xCA000408) = 0x00000003

#define _CALI_WAIT_REG_CNT_MAX_ 10000

#define WAIT_RDY(REG_RDY)  drv_phy_wait_rdy(REG_RDY##_BIT, REG_RDY##_ADDR)
#define RG_TBL_LOAD(TBL)   drv_phy_tbl_load(TBL, sizeof(TBL))

#define _BIST_SETTLE_N_PGAG_    10
#define _BIST_SETTLE_N_RFG_     10

static const ssv_cabrio_reg rfreg_tbl_rxdck[] = {
    {0xCE010000,0x4160B0F1},
    {0xCE010004,0x00000FC0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x000C0CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_rxrck[] = {
    {0xCE010000,0x4360BFFB},
    {0xCE010004,0x00000FC8},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01015A88},
    {0xCE01002C,0x000C0CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_txlo[] = {
    {0xCE010000,0x5F60BF3B},
    {0xCE010004,0x00000FD0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01015A88},
    {0xCE01002C,0x000C0CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_txiqk[] = {
    {0xCE010000,0x5F60BF3B},
    {0xCE010004,0x00000FD0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01015A88},
    {0xCE01002C,0x000C0CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_rxiqk[] = {
    {0xCE010000,0x7F7EB8FB},
    {0xCE010004,0x00000FC4},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x000C0CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_tssi[] = {
    {0xCE010000,0x5E00F00F},
    {0xCE010004,0x00000FC2},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x00030CA8},
    {0xCE010030,0x28EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_papd_bypa[][8] = {
    // _RF_CONFIG_PA_DEF_       // Note: not released by RF yet
    {
    {0xCE010000,0x7F7EF43F},
    {0xCE010004,0x00000FC1},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE010014,0x3D3E84FE},
    {0xCE010018,0x01457D79},
    {0xCE01002C,0x00030CA8},
    {0xCE010030,0x20EA0224}
    },
    // _RF_CONFIG_PA_LI_MPB_    
    {
    {0xCE010000,0x7F7EF53F},
    {0xCE010004,0x00000FC1},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE010014,0x3CBE84FE},
    {0xCE010018,0x004507F9},
    {0xCE01002C,0x00030CA8},
    {0xCE010030,0x20EA0224}
    },
    // _RF_CONFIG_PA_LI_EVB_    // Note: not released by RF yet
    {
    {0xCE010000,0x7F7EF43F},
    {0xCE010004,0x00000FC1},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE010014,0x3D3E84FE},
    {0xCE010018,0x01457D79},
    {0xCE01002C,0x00030CA8},
    {0xCE010030,0x20EA0224}
    },
    // _RF_CONFIG_PA_HP_         // Note: not released by RF yet
    {
    {0xCE010000,0x7F7EF43F},
    {0xCE010004,0x00000FC1},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE010014,0x3D3E84FE},
    {0xCE010018,0x01457D79},
    {0xCE01002C,0x00030CA8},
    {0xCE010030,0x20EA0224}
    }
};

static const ssv_cabrio_reg phyreg_tbl_papd[] = { // Table from Eden
    // {0xC0000304,0x00000001},
    // {0xC0000308,0x00000001},
    {0xce000004,0x00000000},
    {0xce000000,0x80000016},
    {0xce000020,0x20000400},
    {0xce000010,0x00007FFF},
    {0xce000004,0x0000017F},
    // {0xCE0043FC,0x000100E1},   // turn-off 32-point correlation, to prevent trigger signal detection
    {0xCE004010,0x3F7F4905},    // CCA threshold
    // {0xce0023F8,0x20000000},
    // {0xce0043F8,0x00000001},
    // {0xce0023F8,0x20100000},
    // {0xce0043F8,0x00100001},
};

#ifdef _BIST_RF_
static const ssv_cabrio_reg rfreg_tbl_bist_rf_txpsat[] = {
    {0xCE010000,0x5E00F00F},
    {0xCE010004,0x00000FC2},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x00000CA8},
    {0xCE010030,0x28EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_bist_rf_txgain[] = {
    {0xCE010000,0x7FFFB37F},
    {0xCE010004,0x00000FC0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x001E0CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_bist_rf_rxgain[] = {
    {0xCE010000,0x7FFFB43F},
    {0xCE010004,0x00000FC0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x00120CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_bist_rf_rfg_hg2mg[] = {
    {0xCE010000,0x7FFFB0FF},
    {0xCE010004,0x00000FC0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x00240CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_bist_rf_rfg_mg2lg[] = {
    {0xCE010000,0x7FFFB0BF},
    {0xCE010004,0x00000FC0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x00240CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_bist_rf_rfg_lg2ulg[] = {
    {0xCE010000,0x7FFFB07F},
    {0xCE010004,0x00000FC0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x00170CA8},
    {0xCE010030,0x20EA0224}
};

static const ssv_cabrio_reg rfreg_tbl_bist_rf_rssi[] = {
    {0xCE010000,0x7FFFB0FF},
    {0xCE010004,0x00000FC0},
    {0xCE01000C,0x151558C4},
    {0xCE010010,0x01011A88},
    {0xCE01002C,0x00240CA8},
    {0xCE010030,0x20EA0224}
};
#endif

// recovery: register that's in rfreg_tbl_init / phyreg_tbl_init
static ssv_cabrio_reg phyreg_tbl_recovery[] = {
    {0xce000000, 0x0000017f},
    {0xca000408, 0x00000000},   // this row may need modification, for eden specify [3:0] only.
};

static const rfreg_byxtal rfreg_tbl_config_byxtal[] = {
    // _RF_CONFIG_XTAL_26_ 
    {
        0x06,       // rg_dp_fodiv
        0x01,       // rg_dp_refdiv
        0x24,       // rg_dpl_rfctrl_ch
        //0xCE0100A0
        0xEC4CC5,   // rg_dpl_rfctrl_f
        1,          // rg_en_dpl_mod
        {
        // {rg_sx_rfctrl_f, rg_sx_rfctrl_ch, rg_sx_target_cnt, rg_sx_refbytwo}
           {      0x89D89E,           0xB9,           0xF13,              0},
           {      0xEC4EC5,           0xB9,           0xF1B,              0},
           {      0x4EC4EC,           0xBA,           0xF23,              0},
           {      0xB13B14,           0xBA,           0xF2B,              0},
           {      0x13B13B,           0xBB,           0xF33,              0},
           {      0x762762,           0xBB,           0xF3B,              0},
           {      0xD89D8A,           0xBB,           0xF43,              0},
           {      0x3B13B1,           0xBC,           0xF4B,              0},
           {      0x9D89D9,           0xBC,           0xF53,              0},
           {      0x000000,           0xBD,           0xF5B,              0},
           {      0x627627,           0xBD,           0xF63,              0},
           {      0xC4EC4F,           0xBD,           0xF6B,              0},
           {      0x276276,           0xBE,           0xF73,              0},
           {      0x13B13B,           0xBF,           0xF86,              0}
        }
    },
    // _RF_CONFIG_XTAL_40_ 
    {
        0x6,        // rg_dp_fodiv
        0x1,        // rg_dp_refdiv
        0x30,       // rg_dpl_rfctrl_ch
        //0xCE0100A0
        0xEC4CC5,   // rg_dpl_rfctrl_f
        0x0,        // rg_en_dpl_mod
        {
        // {rg_sx_rfctrl_f, rg_sx_rfctrl_ch, rg_sx_target_cnt, rg_sx_refbytwo}
           {    0x333333,              0xF1,            0xF13,            1},
           {    0xB33333,              0xF1,            0xF1B,            1},
           {    0x333333,              0xF2,            0xF23,            1},
           {    0xB33333,              0xF2,            0xF2B,            1},
           {    0x333333,              0xF3,            0xF33,            1},
           {    0xB33333,              0xF3,            0xF3B,            1},
           {    0x333333,              0xF4,            0xF43,            1},
           {    0xB33333,              0xF4,            0xF4B,            1},
           {    0x333333,              0xF5,            0xF53,            1},
           {    0xB33333,              0xF5,            0xF5B,            1},
           {    0x333333,              0xF6,            0xF63,            1},
           {    0xB33333,              0xF6,            0xF6B,            1},
           {    0x333333,              0xF7,            0xF73,            1},
           {    0x066666,              0xF8,            0xF86,            1}
        }
    },
    // _RF_CONFIG_XTAL_24_ 
    {
        0x06,       // rg_dp_fodiv
        0x01,       // rg_dp_refdiv
        0x28,       // rg_dpl_rfctrl_ch
        //0xCE0100A0
        0x00,   // rg_dpl_rfctrl_f
        1,          // rg_en_dpl_mod
        {
        // {rg_sx_rfctrl_f, rg_sx_rfctrl_ch, rg_sx_target_cnt, rg_sx_refbytwo}
           {      0x000000,           0xC9,           0xF13,              0},
           {      0x6AAAAB,           0xC9,           0xF1B,              0},
           {      0xD55555,           0xC9,           0xF23,              0},
           {      0x400000,           0xCA,           0xF2B,              0},
           {      0xAAAAAB,           0xCA,           0xF33,              0},
           {      0x155555,           0xCB,           0xF3B,              0},
           {      0x800000,           0xCB,           0xF43,              0},
           {      0xEAAAAB,           0xCB,           0xF4B,              0},
           {      0x555555,           0xCC,           0xF53,              0},
           {      0xC00000,           0xCC,           0xF5B,              0},
           {      0x2AAAAB,           0xCD,           0xF63,              0},
           {      0x955555,           0xCD,           0xF6B,              0},
           {      0x000000,           0xCE,           0xF73,              0},
           {      0x000000,           0xCF,           0xF86,              0}
        }
    },
};

void rg_idacai_pgag0_load  (u32 rg_rxdc);
void rg_idacai_pgag1_load  (u32 rg_rxdc);
void rg_idacai_pgag2_load  (u32 rg_rxdc);
void rg_idacai_pgag3_load  (u32 rg_rxdc);
void rg_idacai_pgag4_load  (u32 rg_rxdc);
void rg_idacai_pgag5_load  (u32 rg_rxdc);
void rg_idacai_pgag6_load  (u32 rg_rxdc);
void rg_idacai_pgag7_load  (u32 rg_rxdc);
void rg_idacai_pgag8_load  (u32 rg_rxdc);
void rg_idacai_pgag9_load  (u32 rg_rxdc);
void rg_idacai_pgag10_load (u32 rg_rxdc);
void rg_idacai_pgag11_load (u32 rg_rxdc);
void rg_idacai_pgag12_load (u32 rg_rxdc);
void rg_idacai_pgag13_load (u32 rg_rxdc);
void rg_idacai_pgag14_load (u32 rg_rxdc);
void rg_idacai_pgag15_load (u32 rg_rxdc);

void rg_idacaq_pgag0_load  (u32 rg_rxdc);
void rg_idacaq_pgag1_load  (u32 rg_rxdc);
void rg_idacaq_pgag2_load  (u32 rg_rxdc);
void rg_idacaq_pgag3_load  (u32 rg_rxdc);
void rg_idacaq_pgag4_load  (u32 rg_rxdc);
void rg_idacaq_pgag5_load  (u32 rg_rxdc);
void rg_idacaq_pgag6_load  (u32 rg_rxdc);
void rg_idacaq_pgag7_load  (u32 rg_rxdc);
void rg_idacaq_pgag8_load  (u32 rg_rxdc);
void rg_idacaq_pgag9_load  (u32 rg_rxdc);
void rg_idacaq_pgag10_load (u32 rg_rxdc);
void rg_idacaq_pgag11_load (u32 rg_rxdc);
void rg_idacaq_pgag12_load (u32 rg_rxdc);
void rg_idacaq_pgag13_load (u32 rg_rxdc);
void rg_idacaq_pgag14_load (u32 rg_rxdc);
void rg_idacaq_pgag15_load (u32 rg_rxdc);

RG_RXDC_SET rg_rxdc_set_i[] = {
    rg_idacai_pgag0_load,
    rg_idacai_pgag1_load,
    rg_idacai_pgag2_load,
    rg_idacai_pgag3_load,
    rg_idacai_pgag4_load,
    rg_idacai_pgag5_load,
    rg_idacai_pgag6_load,
    rg_idacai_pgag7_load,
    rg_idacai_pgag8_load,
    rg_idacai_pgag9_load,
    rg_idacai_pgag10_load,
    rg_idacai_pgag11_load,
    rg_idacai_pgag12_load,
    rg_idacai_pgag13_load,
    rg_idacai_pgag14_load,
    rg_idacai_pgag15_load   
};

RG_RXDC_SET rg_rxdc_set_q[] = {
    rg_idacaq_pgag0_load,
    rg_idacaq_pgag1_load,
    rg_idacaq_pgag2_load,
    rg_idacaq_pgag3_load,
    rg_idacaq_pgag4_load,
    rg_idacaq_pgag5_load,
    rg_idacaq_pgag6_load,
    rg_idacaq_pgag7_load,
    rg_idacaq_pgag8_load,
    rg_idacaq_pgag9_load,
    rg_idacaq_pgag10_load,
    rg_idacaq_pgag11_load,
    rg_idacaq_pgag12_load,
    rg_idacaq_pgag13_load,
    rg_idacaq_pgag14_load,
    rg_idacaq_pgag15_load   
};

static const u16 papd_tbl_x[] = {  // in DAC interface notation
    0x020,
    0x040,
    0x060,
    0x080,
    0x0a0,
    0x0c0,
    0x0d0,
    0x0e0,
    0x0f0,
    0x100,
    0x110,
    0x120,
    0x130,
    0x140,
    0x150,
    0x160,
    0x170,
    0x180,
    0x190,
    0x1a0,
    0x1b0,
    0x1c0,
    0x1d0,
    0x1e0,
    0x1f0,
    0x200
};

rtbl_cali rtbl_cali_txlo = {
    0,      // ever_cali
    2,      // rtbl_size
    0,      // rsv
    {
    //addr_base //LSB   //MSB   //rsv   // default  //result
    {0x10034,    23,     26,     0,      0x0008,     0x0000}, // rg_tx_dac_ioffset
    {0x10034,    27,     30,     0,      0x0008,     0x0000}  // rg_tx_dac_qoffset
    }
};

rtbl_cali rtbl_cali_txiq = {
    0,      // ever_cali
    2,      // rtbl_size
    0,      // rsv
    {
    //addr_base //LSB   //MSB   //rsv   // default  //result
    {0x0704c,    0,      4,      0,      0x0000,     0x0000}, // rg_tx_iq_theta
    {0x0704c,    8,      12,     0,      0x0000,     0x0000}  // rg_tx_iq_alpha 
    }
};

rtbl_cali rtbl_cali_rxiq = {
    0,      // ever_cali
    2,      // rtbl_size
    0,      // rsv
    {
    //addr_base //LSB   //MSB   //rsv   // default  //result
    {0x07050,    8,      12,     0,      0x0000,     0x0000}, // rg_rx_iq_alpha 
    {0x07050,    0,      4,      0,      0x0000,     0x0000}  // rg_rx_iq_theta
    }
};

rtbl_cali rtbl_cali_papd = {
    0,      // ever_cali
    _CALI_PAPD_RTBL_SIZE_CFG_EN_ + _CALI_PAPD_RTBL_SIZE_CFG_SCALE_ + _CALI_PAPD_RTBL_SIZE_GAIN_ + _CALI_PAPD_RTBL_SIZE_PH_, // rtbl_size   
    0,      // rsv
    {
    //addr_base //LSB   //MSB   //rsv   //default               //result
    //phy-part, configuration
    {0x0711c,   0,      0,      0,      0x0000,     0x0000}, // rg_dpd_am_en
    {0x0711c,   1,      1,      0,      0x0000,     0x0000}, // rg_dpd_pm_en
    {0x071bc,   0,      7,      0,      0x0080,     0x0000}, // rg_tx_scale_11b
    {0x071bc,   8,      15,     0,      0x0080,     0x0000}, // rg_tx_scale_11b_p0d5
    {0x071bc,   16,     23,     0,      0x0080,     0x0000}, // rg_tx_scale_11g
    {0x071bc,   24,     31,     0,      0x0080,     0x0000}, // rg_tx_scale_11g_p0d5
    //phy-part
    {0x07120,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_020_gain
    {0x07120,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_040_gain 
    {0x07124,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_060_gain
    {0x07124,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_080_gain 
    {0x07128,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_0a0_gain
    {0x07128,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_0c0_gain 
    {0x07130,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_0d0_gain
    {0x07130,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_0e0_gain 
    {0x07134,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_0f0_gain
    {0x07134,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_100_gain 
    {0x07138,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_110_gain
    {0x07138,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_120_gain 
    {0x0713c,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_130_gain
    {0x0713c,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_140_gain 
    {0x07140,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_150_gain
    {0x07140,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_160_gain 
    {0x07144,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_170_gain
    {0x07144,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_180_gain 
    {0x07148,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_190_gain
    {0x07148,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_1a0_gain 
    {0x0714c,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_1b0_gain
    {0x0714c,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_1c0_gain 
    {0x07150,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_1d0_gain
    {0x07150,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_1e0_gain 
    {0x07154,   0,      9,      0,      0x0200,     0x0000}, // rg_dpd_1f0_gain
    {0x07154,   16,     25,     0,      0x0200,     0x0000}, // rg_dpd_200_gain 
    {0x07170,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_020_ph
    {0x07170,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_040_ph
    {0x07174,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_060_ph
    {0x07174,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_080_ph
    {0x07178,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_0a0_ph
    {0x07178,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_0c0_ph
    {0x07180,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_0d0_ph
    {0x07180,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_0e0_ph
    {0x07184,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_0f0_ph
    {0x07184,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_100_ph
    {0x07188,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_110_ph
    {0x07188,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_120_ph
    {0x0718c,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_130_ph
    {0x0718c,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_140_ph
    {0x07190,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_150_ph
    {0x07190,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_160_ph
    {0x07194,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_170_ph
    {0x07194,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_180_ph
    {0x07198,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_190_ph
    {0x07198,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_1a0_ph
    {0x0719c,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_1b0_ph
    {0x0719c,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_1c0_ph
    {0x071a0,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_1d0_ph
    {0x071a0,   16,     28,     0,      0x0000,     0x0000}, // rg_dpd_1e0_ph
    {0x071a4,   0,      12,     0,      0x0000,     0x0000}, // rg_dpd_1f0_ph
    {0x071a4,   16,     28,     0,      0x0000,     0x0000}  // rg_dpd_200_ph
    }
};

rtbl_cali rtbl_cali_rxdc = {
    0,      // ever_cali
    _CALI_RXDC_RTBL_SIZE_I_ + _CALI_RXDC_RTBL_SIZE_Q_,
    0,      // rsv
    {
    //addr_base //LSB   //MSB   //rsv   // default  //result
    {0x10080,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG0 
    {0x10080,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG1 
    {0x1007c,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG2 
    {0x1007c,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG3 
    {0x10078,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG4 
    {0x10078,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG5 
    {0x10074,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG6 
    {0x10074,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG7 
    {0x10070,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG8 
    {0x10070,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG9 
    {0x1006c,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG10 
    {0x1006c,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG11 
    {0x10068,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG12 
    {0x10068,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG13 
    {0x10064,   12,     17,     0,      0x0020,     0x0000}, // RG_IDACAI_PGAG14 
    {0x10064,   0,      5,      0,      0x0020,     0x0000}, // RG_IDACAI_PGAG15 
    {0x10080,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG0 
    {0x10080,   6,      11,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG1 
    {0x1007c,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG2 
    {0x1007c,   6,      11,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG3 
    {0x10078,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG4 
    {0x10078,   6,      11,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG5 
    {0x10074,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG6 
    {0x10074,   6,      11,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG7 
    {0x10070,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG8 
    {0x10070,   6,      11,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG9 
    {0x1006c,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG10 
    {0x1006c,   6,      11,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG11 
    {0x10068,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG12 
    {0x10068,   6,      11,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG13 
    {0x10064,   18,     23,     0,      0x0020,     0x0000}, // RG_IDACAQ_PGAG14 
    {0x10064,   6,      11,     0,      0x0020,     0x0000}  // RG_IDACAQ_PGAG15 
    }
};

rtbl_cali rtbl_cali_tssi = {
    0,      // ever_cali
    1,      // rtbl_size
    0,      // rsv
    {
    //rtbl
    //addr_base //LSB   //MSB   //rsv   // default  //result
    {0x1002c,   23,     26,     0,      0x0000,     0x0000}, // RG_TX_GAIN_OFFSET
    }
};

rtbl_cali rtbl_cali_rxrc = {
    0,      // ever_cali
    1,      // rtbl_size
    0,      // rsv
    {
    //addr_base //LSB   //MSB   //rsv   // default  //result
    {0x1000c,   3,      8,      0,      0x001b,     0x0000}, // RG_RX_ABBCTUNE
    }
};

u32 papd_ltbl_am[_CALI_PAPD_OBSR_N_];   //ltbl: Learning table
s32 papd_ltbl_pm[_CALI_PAPD_OBSR_N_];

#ifdef _BIST_RF_
const u16 tbl_bist_rf_txgain[] = {  // RG_TX_GAIN
    0x16 << 2,  // -22 dB
    0x19 << 2,  // -25 dB
    0x1c << 2   // -28 dB
};

const u16 tbl_bist_rf_rxgain[] = {  // RG_PGAG
    0x0b,
    0x0a,
    0x09,
    0x08
};

rtbl_measure rtbl_measure_txgain = {
    0,      // valid
    3,      // rtbl_size
    0,      // rsv
    {
        0,
        0,
        0
    }
};

rtbl_measure rtbl_measure_pgag = {
    0,      // valid
    4,      // rtbl_size
    0,      // rsv
    {
        0,
        0,
        0,
        0
    }
};

rtbl_measure rtbl_measure_rssi = {
    0,      // valid
    1,      // rtbl_size
    0,      // rsv
    {
        0
    }
};

rtbl_measure rtbl_measure_rfg_hg2mg = {
    0,      // valid
    4,      // rtbl_size
    0,      // rsv
    {
        0,  // signal @MG
        0,  // noise  @MG
        0,  // signal @HG
        0   // noise  @HG
    }
};

rtbl_measure rtbl_measure_rfg_mg2lg = {
    0,      // valid
    4,      // rtbl_size
    0,      // rsv
    {
        0,  // signal @LG
        0,  // noise  @LG
        0,  // signal @MG
        0   // noise  @MG
    }
};

rtbl_measure rtbl_measure_rfg_lg2ulg = {
    0,      // valid
    4,      // rtbl_size
    0,      // rsv
    {
        0,  // signal @ULG
        0,  // noise  @ULG
        0,  // signal @LG
        0   // noise  @LG
    }
};

rtbl_measure rtbl_measure_txpsat = {
    0,      // valid
    1,      // rtbl_size
    0,      // rsv
    {
        0
    }
};
#endif

const phy_cali_config phy_cali_config_rxdc = {
    0x80,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0,      // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0,      // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x100,  // cfg_scale_fft; u10.10f

    (const ssv_cabrio_reg *)rfreg_tbl_rxdck,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_rxdck),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_cali_config_rxrc = {
    _CALI_RXRC_TX_ISCALE_,  // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    _CALI_RXRC_RX_ISCALE_,  // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    _CALI_RXRC_TX_IFREQ_,   // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    8,                      // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff,                 // cfg_rtbl_sel
    7,                      // cfg_ch                   used by drv_phy_channel_change()
    1,                      // cfg_intg_prd             SET_RG_INTG_PRD
    0,                      // cfg_selq                 used by drv_phy_sprm()
    0,                      // rsv
    0x100,                  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_rxrck,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_rxrck),
    sizeof(phyreg_tbl_papd),
};

const phy_cali_config phy_cali_config_txlo = {
    0x40,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x06,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x08,   // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x08,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    1,      // cfg_intg_prd             SET_RG_INTG_PRD
    1,      // cfg_selq
    0,      // rsv
    0x100,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_txlo,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_txlo),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_cali_config_txiq = {
    0x40,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x06,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x08,   // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    1,      // cfg_intg_prd             SET_RG_INTG_PRD
    1,      // cfg_selq
    0,      // rsv
    0x100,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_txiqk,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_txiqk),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_cali_config_rxiq = {
    0x40,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10,   // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x30,   // cfg_freq_rx;  312.5K*n   used by drv_phy_fft_enrg()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x1f0,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_rxiqk,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_rxiqk),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_cali_config_tssi = {
    0x80,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x05,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10,   // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x100,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_tssi,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_tssi),
    sizeof(phyreg_tbl_papd)
};

phy_cali_config phy_cali_config_papd = {
    0x80,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10,   // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x100,  // cfg_scale_fft

    (const ssv_cabrio_reg *)NULL,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    0,
    sizeof(phyreg_tbl_papd)

};

#ifdef _BIST_RF_
const phy_cali_config phy_measure_config_bist_rf_txgain = {
    0x28,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10 ,  // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x00F,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_bist_rf_txgain,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_bist_rf_txgain),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_measure_config_bist_rf_pgag = {
    0x28,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10 ,  // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x0F,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_bist_rf_rxgain,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_bist_rf_rxgain),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_measure_config_bist_rf_rssi = {
    0x50,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10 ,  // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x080,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_bist_rf_rssi,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_bist_rf_rssi),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_measure_config_bist_rf_rfg_hg2mg = {
    0x0d,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10 ,  // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x00F,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_bist_rf_rfg_hg2mg,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_bist_rf_rfg_hg2mg),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_measure_config_bist_rf_rfg_mg2lg = {
    0x33,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10 ,  // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x00F,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_bist_rf_rfg_mg2lg,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_bist_rf_rfg_mg2lg),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_measure_config_bist_rf_rfg_lg2ulg = {
    0x33,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x02,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10 ,  // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x00F,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_bist_rf_rfg_lg2ulg,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_bist_rf_rfg_lg2ulg),
    sizeof(phyreg_tbl_papd)
};

const phy_cali_config phy_measure_config_bist_rf_txpsat = {
    0x80,   // cfg_scale_tx; u8.7f;     SET_RG_TX_*_SCALE
    0x05,   // cfg_scale_rx; u8.1f;     SET_RG_RX_*_SCALE 
    0x10 ,  // cfg_freq_tx;  312.5K*n;  SET_RG_TONEGEN_FREQ
    0x10,   // cfg_freq_rx;  312.5K*n   used by drv_phy_sprm()
    0xffff, // cfg_rtbl_sel
    7,      // cfg_ch                   used by drv_phy_channel_change()
    2,      // cfg_intg_prd             SET_RG_INTG_PRD
    0,      // cfg_selq
    0,      // rsv
    0x100,  // cfg_scale_fft

    (const ssv_cabrio_reg *)rfreg_tbl_bist_rf_txpsat,
    (const ssv_cabrio_reg *)phyreg_tbl_papd,

    sizeof(rfreg_tbl_bist_rf_txpsat),
    sizeof(phyreg_tbl_papd)
};

#endif

struct temperature_st rtbl_temperature = {
    0,
    0
};

const u16 tssi_tbl_gain[][2] = {    // u8.7f
    /*
    {0xc, 0x1d},   //  -13   dB
    {0xd, 0x1e},   //  -12.5 dB
    {0xe, 0x20},   //  -12   dB
    {0xf, 0x22},   //  -11.5 dB
    {0x0, 0x24},   //  -11   dB
    {0x1, 0x26},   //  -10.5 dB
    {0x2, 0x28},   //  -10   dB
    {0x3, 0x2b},   //  -9.5  dB
    */
    /*
    {0x4, 0x24},   //  -11   dB
    {0x3, 0x26},   //  -10.5 dB
    {0x2, 0x28},   //  -10   dB
    {0x1, 0x2b},   //  -9.5  dB
    {0x0, 0x2d},   //  -9    dB
    {0xf, 0x30},   //  -8.5  dB
    {0xe, 0x33},   //  -8    dB
    {0xd, 0x36},   //  -7.5  dB
    {0xc, 0x39}    //  -7    dB
    */
    {0x4, 0x39},   //  -7    dB
    {0x3, 0x3d},   //  -6.5  dB
    {0x2, 0x40},   //  -6    dB
    {0x1, 0x44},   //  -5.5  dB
    {0x0, 0x48},   //  -5    dB
    {0xf, 0x4c},   //  -4.5  dB
    {0xe, 0x51},   //  -4    dB
    {0xd, 0x56},   //  -3.5  dB
    {0xc, 0x5b}    //  -3    dB
};

cali_info cali_info_tbl[] = {
    {drv_phy_cali_rcal,     NULL,               _CALI_SEQ_RCAL_,        "RCAL",     NULL},
    {drv_phy_temperature,   NULL,               _CALI_SEQ_TEMPERATURE_, "THERMAL",  NULL},
    {drv_phy_cali_rxdc,     &rtbl_cali_rxdc,    _CALI_SEQ_RXDC_,        "RXDC",     &phy_cali_config_rxdc},
    {drv_phy_cali_rxrc,     &rtbl_cali_rxrc,    _CALI_SEQ_RXRC_,        "RXRC",     &phy_cali_config_rxrc},
    {drv_phy_cali_txlo,     &rtbl_cali_txlo,    _CALI_SEQ_TXDC_,        "TXDC",     &phy_cali_config_txlo},
    {drv_phy_cali_txiq,     &rtbl_cali_txiq,    _CALI_SEQ_TXIQ_,        "TXIQ",     &phy_cali_config_txiq},
    {drv_phy_cali_rxiq_fft, &rtbl_cali_rxiq,    _CALI_SEQ_RXIQ_,        "RXIQ",     &phy_cali_config_rxiq},
    {drv_phy_cali_tssi,     &rtbl_cali_tssi,    _CALI_SEQ_TSSI_,        "TSSI",     &phy_cali_config_tssi},
    {drv_phy_cali_papd,     &rtbl_cali_papd,    _CALI_SEQ_PAPD_,        "PAPD",     &phy_cali_config_papd},
    //
    {drv_phy_cali_papd_learning,    NULL,       _CALI_SEQ_PAPD_LEARNING_,   "PAPD-LEARNING",    &phy_cali_config_papd}
};

#ifdef _BIST_RF_
bist_info bist_info_tbl[] = {
    {drv_phy_bist_rf_txgain, &rtbl_measure_txgain,  _MEASURE_SEQ_TXGAIN_, "TXGAIN", &phy_measure_config_bist_rf_txgain},
    {drv_phy_bist_rf_pgag,   &rtbl_measure_pgag,    _MEASURE_SEQ_PGAG_,   "RXPGAG", &phy_measure_config_bist_rf_pgag},
    {drv_phy_bist_rf_rssi,   &rtbl_measure_rssi,    _MEASURE_SEQ_RSSI_,   "RSSI",   &phy_measure_config_bist_rf_rssi},
    //
    {drv_phy_bist_rf_rfg_hg2mg,     &rtbl_measure_rfg_hg2mg,    _MEASURE_SEQ_RFG_HG2MG_,    "RFG-HG2MG",    &phy_measure_config_bist_rf_rfg_hg2mg},
    {drv_phy_bist_rf_rfg_mg2lg,     &rtbl_measure_rfg_mg2lg,    _MEASURE_SEQ_RFG_MG2LG_,    "RFG-MG2LG",    &phy_measure_config_bist_rf_rfg_mg2lg},
    {drv_phy_bist_rf_rfg_lg2ulg,    &rtbl_measure_rfg_lg2ulg,   _MEASURE_SEQ_RFG_LG2ULG_,   "RFG-LG2ULG",   &phy_measure_config_bist_rf_rfg_lg2ulg},
    //
    {drv_phy_bist_rf_txpsat, &rtbl_measure_txpsat,  _MEASURE_SEQ_TXPSAT_, "TXPSAT", &phy_measure_config_bist_rf_txpsat},
};
#endif

/*
    Default 40M --- For FPGA
    This table is replaced by SSV6XXX_HOST_CMD_INIT_CALI(Host command) after power on.
*/
rf_config g_rf_config = {
    // cfg_xtal
#ifdef CONFIG_SSV_CABRIO_A
    SSV6XXX_IQK_CFG_XTAL_40M,
#else
    SSV6XXX_IQK_CFG_XTAL_26M,
#endif
    // cfg_pa
    0,
    // _RF_CONFIG_PA_LI_MPB_,
    // cfg_tssi_trgt
    _CALI_TSSI_SAR_TARGET_,
    // cfg_tssi_div
    _CALI_TSSI_DIV_,
    // rsv
    0,
    // cfg_def_tx_scale_11b
    0,
    // cfg_def_tx_scale_11b_p0d5
    0,
    // cfg_def_tx_scale_11g
    0,
    // cfg_def_tx_scale_11g_p0d5
    0,
};

#ifdef __DBG_DUMP__
extern u32* sreg_ft_dbg;
#endif

void rfreg_byxtal_load(const rfreg_byxtal reg) {

    SET_RG_DP_FODIV     (reg.rg_dp_fodiv     );
    SET_RG_DP_REFDIV    (reg.rg_dp_refdiv    );
    SET_RG_DPL_RFCTRL_CH(reg.rg_dpl_rfctrl_ch);
    SET_RG_DPL_RFCTRL_F (reg.rg_dpl_rfctrl_f );
    SET_RG_EN_DPL_MOD   (reg.rg_en_dpl_mod   );

}

void rfreg_bych_load(const rfreg_bych reg) {

    #ifdef CONFIG_SSV_CABRIO_A
    SET_CBR_RG_SX_RFCTRL_F (reg.rg_sx_rfctrl_f  );
    SET_CBR_RG_SX_RFCTRL_CH(reg.rg_sx_rfctrl_ch );
    SET_CBR_RG_SX_REFBYTWO (reg.rg_sx_refbytwo  );
    #endif
    #ifdef CONFIG_SSV_CABRIO_E
    SET_RG_SX_RFCTRL_F  (reg.rg_sx_rfctrl_f  );
    SET_RG_SX_RFCTRL_CH (reg.rg_sx_rfctrl_ch );
    SET_RG_SX_TARGET_CNT(reg.rg_sx_target_cnt);
    SET_RG_SX_REFBYTWO  (reg.rg_sx_refbytwo  );
    #endif

}

void drv_phy_tbl_load (const ssv_cabrio_reg tbl[], u32 tbl_size) {

    u32 loop_i;

    if(tbl == NULL)
        return;

    for(loop_i=0; loop_i < tbl_size/sizeof(ssv_cabrio_reg); loop_i++) {
        REG32(tbl[loop_i].address) = tbl[loop_i].data;
    }

}

void phy_cali_config_load(const phy_cali_config *cfg) {

    // T/R switch
    _CALI_ANT_CTRL_;

    // RF setup
    if(cfg->rfreg_tbl != NULL) {
        drv_phy_tbl_load(cfg->rfreg_tbl, cfg->rfreg_tbl_size);
    }

    // phy setup
    if (cfg->phyreg_tbl != NULL) {
        drv_phy_tbl_load(cfg->phyreg_tbl, cfg->phyreg_tbl_size);
    }

    // load phy config for cali.
    SET_RG_DAC_EN_MAN(1);
    SET_RG_IQC_FFT_EN(1);

    // load previous cali. result
    drv_phy_cali_rtbl_load(cfg->cfg_rtbl_sel);

    // load config
    SET_RG_TX_I_SCALE(cfg->cfg_scale_tx);
    SET_RG_TX_Q_SCALE(cfg->cfg_scale_tx);
    //
    SET_RG_RX_I_SCALE(cfg->cfg_scale_rx);
    SET_RG_RX_Q_SCALE(cfg->cfg_scale_rx);
    //
    SET_RG_INTG_PRD(cfg->cfg_intg_prd);
    //
    SET_RG_RX_FFT_SCALE(cfg->cfg_scale_fft);    // u10.10f

    // lock channel
    drv_phy_channel_change(cfg->cfg_ch);

    // turn-on tonegen
    SET_RG_TONEGEN_FREQ(cfg->cfg_freq_tx);
    SET_RG_TX_IQ_SRC   (2               );
    SET_RG_TONEGEN_EN  (1               );
}

u32 drv_phy_wait_rdy (u32 bit, u32 addr) {

    u32 rdy;  // c: current
    u32 loop_i;

    for(loop_i=0; loop_i < _CALI_WAIT_REG_CNT_MAX_; loop_i++) {

        rdy = REG32(addr);
        rdy = (rdy >> bit) & 1;

        if (rdy) {
            break;
        }
    }

    return rdy;
}


//#define CALIBRATION_DEBUG
s32 drv_phy_channel_change(s32 channel_id)
{
//calibration

u32 cfg_xtal;
u32 cfg_ch  ;

#ifdef CONFIG_SSV_CABRIO_A
u8 fail_cnt=0;
u32 retry_cnt;
#endif
#ifdef CONFIG_SSV_CABRIO_E
u32 phy_sx_lck_bin_rdy;
#endif

#ifdef CONFIG_SSV_CABRIO_A

    //FPGA xtal 40M
    cfg_xtal = g_rf_config.cfg_xtal;
    //printf("xtal type[%d]\n",p_iqk_cfg->cfg_xtal);

    // Note: this is not a good implementation style, should be fixed in later stage.
    cfg_ch   = channel_id -1;

    SET_CBR_RG_MODE(_RF_MODE_STANDBY_);

    rfreg_bych_load(rfreg_tbl_config_byxtal[cfg_xtal].tbl_bych[cfg_ch]);

    SET_CBR_RG_EN_SX_VT_MON(1);
    do
    {
        #ifdef __LOG_DRV_PHY_CHANNEL_CHANGE__
        LOG_PRINTF("Change to channel:%d\n",channel_id);
        #endif

        SET_CBR_RG_SX_SUB_SEL_CWD(64);
        SET_CBR_RG_SX_SUB_SEL_CWR(1);
        SET_CBR_RG_SX_SUB_SEL_CWR(0);

        SET_CBR_RG_EN_SX_VT_MON_DG(0);
        SET_CBR_RG_EN_SX_VT_MON_DG(1);

        for(retry_cnt=0; retry_cnt<20000; retry_cnt++)
        {
            if(GET_CBR_VT_MON_RDY == 1) {
                #ifdef __LOG_DRV_PHY_CHANNEL_CHANGE__
                LOG_PRINTF("vt-mon rdy on loop %d\n", retry_cnt);
                #endif
                break;
            }
        }

        if(GET_CBR_AD_SX_VT_MON_Q == 0) {

            #ifdef __LOG_DRV_PHY_CHANNEL_CHANGE__
            LOG_PRINTF("Lock (VT) of ch %d!!!\n", channel_id);
            #endif

            SET_CBR_RG_EN_SX_VT_MON(0);
            SET_CBR_RG_EN_SX_VT_MON_DG(0);
            SET_CBR_RG_MODE(_RF_MODE_TRX_);
            return 0;
        }

        #ifdef __LOG_DRV_PHY_CHANNEL_CHANGE__
        LOG_PRINTF("ch-lock fail[%d] rounds!!\n",fail_cnt);
        #endif
        fail_cnt++;

        if(fail_cnt == 100)
        {
            return -1;
        }
    }while(1);
#endif
#ifdef CONFIG_SSV_CABRIO_E
    #ifdef __LOG_DRV_PHY_CHANNEL_CHANGE__
    LOG_PRINTF("Change to channel:%d\n",channel_id);
    #endif

    cfg_xtal = g_rf_config.cfg_xtal;
    //printf("xtal type[%d]\n",p_iqk_cfg->cfg_xtal);
    cfg_ch   = channel_id -1;

    rfreg_bych_load(rfreg_tbl_config_byxtal[cfg_xtal].tbl_bych[cfg_ch]);

    SET_RG_EN_SX_LCK_BIN(1);

    phy_sx_lck_bin_rdy = WAIT_RDY(LCK_BIN_RDY);

    SET_RG_EN_SX_LCK_BIN(0);
    
    if(phy_sx_lck_bin_rdy)
    {
        #ifdef __LOG_DRV_PHY_CHANNEL_CHANGE__
        LOG_PRINTF("Lock of ch %d!!!\n", channel_id);
        #endif
        return 0;
    }
    else
    {
        #ifdef __LOG_DRV_PHY_CHANNEL_CHANGE__
        LOG_PRINTF("Lock failed!!!\n");
        #endif
        return -1;
    }
#endif
}

s32 drv_phy_sprm(u32 freq, u32 sel_q) {

    u32 rdy;

    SET_RG_IQCAL_SPRM_FREQ(freq );      
    SET_RG_IQCAL_SPRM_SELQ(sel_q);

    SET_RG_IQCAL_SPRM_EN(1);

    rdy = WAIT_RDY(RO_IQCAL_SPRM_RDY);

    SET_RG_IQCAL_SPRM_EN(0);

    if (rdy) {
        return 0;
    }
    else {
        LOG_PRINTF("## SPRM fail!\n");
        return -1;
    }
}

s32 drv_phy_fft_enrg(u32 freq) {

    u32 rdy;

    SET_RG_FFT_ENRG_FREQ(freq);

    SET_RG_FFT_EN(1);

    rdy = WAIT_RDY(RO_FFT_ENRG_RDY);

    SET_RG_FFT_EN(0);

    if (rdy) {
        return 0;
    }
    else {
        #ifdef __LOG_DRV_PHY_INIT_CALI__
        LOG_PRINTF("## FFT fail!\n");
        #endif
        return -1;
    }
}

void rtbl_cali_reg_rmw (u32 lsb, u32 msb, u32 val_w, u32 addr) {

    u32 msk;
    u32 val;

    msk = (0xffffffff << lsb);
    msk = msk & (0xffffffff >> (31 - msb));

    val = REG32(addr);
    val = val & ~msk;
    val = val | ((val_w << lsb) & msk);

    REG32(addr) = val;

}

void rtbl_cali_pre_load (rtbl_cali *rtbl_p) {

    u32 loop_i;
    u32 msb;
    u32 lsb;
    u32 val_w;
    u32 addr;

    for(loop_i = 0; loop_i < rtbl_p->rtbl_size; loop_i++) {

        lsb   = rtbl_p->rtbl[loop_i].sht        ;
        msb   = rtbl_p->rtbl[loop_i].hi         ;
        val_w = rtbl_p->rtbl[loop_i].result     ;
        addr  = rtbl_p->rtbl[loop_i].addr_offset + _CALI_ADDR_BASE_;

        rtbl_cali_reg_rmw(lsb, msb, val_w, addr);
    }
}

s32 rtbl_cali_load (rtbl_cali *rtbl_p) {

    if (rtbl_p->ever_cali == 0)
        return -1;

    rtbl_cali_pre_load(rtbl_p);
    return 0;

}

s32 rtbl_cali_export (rtbl_cali *rtbl_p) {

    u32 loop_i;

    if (rtbl_p->ever_cali == 0)
        return -1;

    LOG_PRINTF("address(H), lsb(D), length(D), val(H)\n");

    for(loop_i = 0; loop_i < rtbl_p->rtbl_size; loop_i++) {
        LOG_PRINTF("%08x, %2d, %2d, %08x\n", (rtbl_p->rtbl[loop_i].addr_offset + _CALI_ADDR_BASE_),
                                              rtbl_p->rtbl[loop_i].sht,
                                             (rtbl_p->rtbl[loop_i].hi - rtbl_p->rtbl[loop_i].sht + 1),
                                              rtbl_p->rtbl[loop_i].result);
    }

    return 0;

}
void rtbl_cali_load_def (rtbl_cali *rtbl_p) {

    u32 loop_i;
    u32 msb;
    u32 lsb;
    u32 val_w;
    u32 addr;

    for(loop_i = 0; loop_i < rtbl_p->rtbl_size; loop_i++) {

        lsb   = rtbl_p->rtbl[loop_i].sht        ;
        msb   = rtbl_p->rtbl[loop_i].hi         ;
        val_w = rtbl_p->rtbl[loop_i].def        ;
        addr  = rtbl_p->rtbl[loop_i].addr_offset + _CALI_ADDR_BASE_;

        rtbl_cali_reg_rmw(lsb, msb, val_w, addr);
    }

}

void rtbl_cali_reset (rtbl_cali *rtbl_p) {

    u32 loop_i;

    rtbl_p->ever_cali = 0;

    for(loop_i=0; loop_i < rtbl_p->rtbl_size; loop_i++) {
        rtbl_p->rtbl[loop_i].result = 0;
    }

    rtbl_cali_load_def(rtbl_p);
}

void rtbl_cali_set (rtbl_cali *rtbl_p) {

    u32 loop_i;
    u32 msb;
    u32 lsb;
    u32 msk;
    u32 val;
    u32 addr;

    for(loop_i = 0; loop_i < rtbl_p->rtbl_size; loop_i++) {

        lsb   = rtbl_p->rtbl[loop_i].sht        ;
        msb   = rtbl_p->rtbl[loop_i].hi         ;
        addr  = rtbl_p->rtbl[loop_i].addr_offset + _CALI_ADDR_BASE_;

        msk = (0xffffffff >> (31 - msb));

        val = REG32(addr);
        val = val & msk;
        val = val >> lsb;

        rtbl_p->rtbl[loop_i].result = (u16)val;
    }
    rtbl_p->ever_cali = 1;
}

void drv_phy_init(void)
{
    // initialize cali. result table
    rtbl_cali_txlo.ever_cali = 0;
    rtbl_cali_txiq.ever_cali = 0;
    rtbl_cali_rxiq.ever_cali = 0;
    rtbl_cali_papd.ever_cali = 0;
    rtbl_cali_rxdc.ever_cali = 0;
    rtbl_cali_tssi.ever_cali = 0;
    rtbl_cali_rxrc.ever_cali = 0;
}

void drv_phy_off(void) {

    SET_RG_PHY_MD_EN(0);

}

void drv_phy_b_only(void) {
	(*(volatile u32 *) 0xCE000004) = 0x07e;// B only
}

void drv_phy_bgn(void) {
	//(*(volatile u32 *) 0xCE000004) = 0x07e;// B only
	//(*(volatile u32 *) 0xCE000004) = 0x06e;// G/N only
	(*(volatile u32 *) 0xCE000004) = 0x17e;// B/G/N only

}

void drv_phy_on(void) {

    SET_RG_PHY_MD_EN(1);

}

s32 drv_phy_cali_rcal (const phy_cali_config *phy_cfg)   // Add by Bernie
{

    u32 ro_rcal_rdy;

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[10];
    u32 ctime_idx = 0;
    u32 loop_i;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    if (GET_RG_EN_RCAL == 1) {
        #ifdef __LOG_DRV_PHY_CALI_RCAL__
        LOG_PRINTF("DRV_PHY_CALI_RCAL fail: RG_EN_RCAL is already 1\n");
        #endif
        return -1;
    }

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    SET_RG_EN_RCAL(1);
    ro_rcal_rdy = WAIT_RDY(RCAL_RDY);

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    SET_RG_EN_RCAL(0);
    if(ro_rcal_rdy) {
        #ifdef __LOG_DRV_PHY_CALI_RCAL__
        LOG_PRINTF("DRV_PHY_CALI_RCAL pass: {r_cal_code, r_code_lut} = {%d, %d}.\n", GET_DA_R_CAL_CODE, GET_DA_R_CODE_LUT);
        #endif

        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif

        #ifdef __LOG_DRV_PHY_MCU_TIME__
        for(loop_i=0; loop_i < ctime_idx; loop_i++) {
            LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
        }
        #endif
        return 0;
    }
    else {
        // error case 3: rcal fail to ready.
        #ifdef __LOG_DRV_PHY_CALI_RCAL__
        LOG_PRINTF("DRV_PHY_CALI_RCAL fail: rcal fail to ready.\n");
        #endif
        return -3;
    }

}
struct ssv6xxx_tx_loopback tx_loopback[] = {
    /* reg                          val    re_val  re delay_ms */
    {ADR_PHY_PKT_GEN_0,             0x07010400, 0, 1, 0},   //0xCE00000C 
    {ADR_PHY_EN_1,                  0x0000017E, 0, 1, 0},   //0xCE000004
    {ADR_MANUAL_ENABLE_REGISTER,    0x00022FE0, 0, 1, 0},   //0xCE010004
    {ADR_ABB_REGISTER_1,            0x15155AC5, 0, 1, 0},   //0xCE01000C
    {ADR_HARD_WIRE_PIN_REGISTER,    0x4300B6CB, 0, 1, 0},   //0xCE010000
    {ADR_PAD64,                     0x00000000, 0, 1, 0},   //0xC00003AC
    {ADR_PAD65,                     0x00000000, 0, 1, 0},   //0xC00003B0
    {ADR_PAD66,                     0x00000000, 0, 1, 0},   //0xC00003B4
    {ADR_PAD67,                     0x00000000, 0, 1, 0},   //0xC00003BC
    {ADR_PHY_REG_00,                0x20000400, 0, 1, 0},   //0xCE000020
    {ADR_PHY_PKT_GEN_4,             0x0000000f, 0, 0, 0},   //0xCE00001C
    {ADR_PHY_EN_0,                  0x80000217, 0, 1, 2},   //0xCE000000
    {ADR_RX_11GN_STAT_EN,           0x00000001, 0, 1, 0},   //0xCE0043F8
    {ADR_RX_11B_PKT_STAT_EN,        0x00000000, 0, 1, 0},   //0xCE0023F8
    {ADR_RX_11GN_STAT_EN,           0x00100001, 0, 0, 0},   //0xCE0043F8
    {ADR_RX_11B_PKT_STAT_EN,        0x00100000, 0, 0, 0},   //0xCE0023F8
    {ADR_PHY_EN_1,                  0x0000017F, 0, 0, 0},   //0xCE000004
    {ADR_PHY_PKT_GEN_3,             0x0055083D, 0, 1, 10},  //0xCE000018
    {ADR_PHY_PKT_GEN_3,             0x0055083C, 0, 0, 0},   //0xCE000018
    {ADR_PHY_EN_1,                  0x0000017E, 0, 0, 0}    //0xCE000004
};

void drv_phy_txloopbk_result(u32 result)
{
    u32 notification_data;
    HDR_HostEvent *host_evt;
    u32 evt_size = sizeof(HDR_HostEvent);

    do{
        notification_data = (u32)PBUF_MAlloc_Raw(evt_size, 0, RX_BUF);
    }while(notification_data == 0);

    host_evt = (HDR_HostEvent *)notification_data;
    host_evt->c_type = HOST_EVENT;
    host_evt->h_event = SOC_EVT_TXLOOPBK_RESULT;
    host_evt->len = evt_size;
    host_evt->evt_seq_no = result;

    // Send out to host
    TX_FRAME(notification_data);
}

void drv_phy_delay(s32 delay_ms)
{
    Time_T delay_time; 
    s32 delayed;
    dbg_get_time(&delay_time);
    do{
        delayed = dbg_get_elapsed(&delay_time);
    }while((delayed/1000) < delay_ms);
}

s32 drv_phy_tx_loopback(void)
{
    u32 ret = SSV6XXX_STATE_NG;
    u32 err_count, pkt_count;
    s32 loop_time = 20, i, size;
    u32 reg_64m = REG32(ADR_TRX_DUMMY_REGISTER);
  
    size = sizeof(tx_loopback)/sizeof(tx_loopback[0]);

    /* save original register values */
    for (i = 0; i < size; i++) {
        if (tx_loopback[i].restore) {
            tx_loopback[i].restore_val = REG32(tx_loopback[i].reg);
        }
    }
    // Enable 64M to ensure MAC stability of clock switch from/to DPLL.
    REG32(ADR_TRX_DUMMY_REGISTER) = reg_64m | BIT(30);

    pkt_count = GET_GN_PACKET_CNT;
    err_count = GET_GN_PACKET_ERR_CNT;
    while (loop_time > 0) {
        u32 post_err_count, post_pkt_count;
        for (i = 0; i < size; i++) {
            REG32(tx_loopback[i].reg) = tx_loopback[i].val;
            if (tx_loopback[i].delay_ms) {
                drv_phy_delay(tx_loopback[i].delay_ms);
            }
        }
        post_err_count = GET_GN_PACKET_ERR_CNT;
        post_pkt_count = GET_GN_PACKET_CNT;
        if (   (err_count != post_err_count) 
            || (post_pkt_count != (pkt_count + 15))) {
            printf("redo tx loopback\n");
        }
        else {
            printf("tx loopback OK (%d)\n", post_pkt_count);
            ret = SSV6XXX_STATE_OK;
            // if (loop_time < 10) // Test condition to force loop at least 10+ times.
            break;
        }
        pkt_count = post_pkt_count;
        err_count = post_err_count;
    
        /* do phy clock switch & pmu sleep/wakeup */
        SET_RG_RF_BB_CLK_SEL(0); // Switch PHY clock from DPLL to XO.
        drv_phy_delay(1);
        SET_CK_SEL_1_0(0); // Switch MAC clock from DPLL to XO.
        SET_PMU_ENTER_SLEEP_MODE(0);
        SET_PMU_ENTER_SLEEP_MODE(1);
        SET_PMU_ENTER_SLEEP_MODE(0);
        SET_RG_RF_BB_CLK_SEL(1); // Switch PHY clock from XO to DPLL.
        SET_CK_SEL_1_0(3); // Switch MAC clock from XO to DPLL.
        drv_phy_delay(2);
        
        loop_time--;
    }
    
    /* roll back original register values */
    for (i = 0; i < size; i++) {
        if (tx_loopback[i].restore) {
            //printf("R reg: %x, val: %x\n", tx_loopback[i].reg, tx_loopback[i].val);
            REG32(tx_loopback[i].reg) = tx_loopback[i].restore_val;
        }
    }
    // Send host the result of PHY loopback test
    drv_phy_txloopbk_result(ret);
    
    REG32(ADR_TRX_DUMMY_REGISTER) = reg_64m;
    
    return ret;
}

s32 drv_phy_cali_ro_iqcal_o (u32 ro_iqcal_o) {  // convert ro_iqcal_o from unsigned to signed value
    if (ro_iqcal_o > _RO_IQCAL_O_MAX_)
        return ro_iqcal_o - _RO_IQCAL_O_WRAP_;
    else
        return ro_iqcal_o;
}

/*
s32 drv_phy_cali_ro_adc (void) {

    s32 ro_adc;

    ro_adc = (s32)GET_RG_RX_ADC_I_RO_BIT;       // dummy read
    ro_adc = (s32)GET_RG_RX_ADC_I_RO_BIT -128; 

    return ro_adc;

}
*/

s32 drv_phy_cali_txlo (const phy_cali_config *phy_cfg) {

    u32 loop_i;
    u32 loop_j;
    s32 ro_iqcal_o;

    s32 ro_iqcal_o_min;
    u32 rg_tx_dac_ioffset_min_lo;
    u32 rg_tx_dac_qoffset_min_lo;

    #ifdef _STATISTIC_DRV_PHY_CALI_TXLO_
    static u32 sidx = 1;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[10];
    u32 ctime_idx = 0;
    #endif

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp_i = sreg_ft_dbg;     // debug dump pointer
    u32* dbg_dumpp_j = sreg_ft_dbg + 16;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    for(loop_j=0; loop_j <_CALI_TXLO_ROUND_; loop_j++) {

        // cali. I-path
        ro_iqcal_o_min           = _RO_IQCAL_O_MAX_;
        rg_tx_dac_ioffset_min_lo = 0;
        //
        for (loop_i=0 ; loop_i <16; loop_i++) {
            SET_RG_TX_DAC_IOFFSET(loop_i);
            //
            if (drv_phy_sprm(phy_cfg->cfg_freq_rx, phy_cfg->cfg_selq) != 0) {
                return -3;
            }
            ro_iqcal_o = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

            #ifdef __LOG_DRV_PHY_CALI_TXLO__
            LOG_PRINTF("# RG_TX_DAC_IOFFSET = %d, got iqcal_o = %d\n", loop_i, ro_iqcal_o);
            #endif
    
            #ifdef __DBG_DUMP__
            *(dbg_dumpp_i + loop_i) = ro_iqcal_o;
            #endif
    
            if (ro_iqcal_o < ro_iqcal_o_min) {
                rg_tx_dac_ioffset_min_lo = loop_i    ;
                ro_iqcal_o_min           = ro_iqcal_o;
            }
        }
        SET_RG_TX_DAC_IOFFSET(rg_tx_dac_ioffset_min_lo);
        #ifdef __LOG_DRV_PHY_CALI_TXLO__
        LOG_PRINTF("## TX LO min when RG_TX_DAC_IOFFSET = %d\n", rg_tx_dac_ioffset_min_lo);
        #endif
    
        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif
    
        // cali. Q-path
        ro_iqcal_o_min           = _RO_IQCAL_O_MAX_;
        rg_tx_dac_qoffset_min_lo = 0;
        //
        for (loop_i=0 ; loop_i <16; loop_i++) {
            SET_RG_TX_DAC_QOFFSET(loop_i);
            //
            if (drv_phy_sprm(phy_cfg->cfg_freq_rx, phy_cfg->cfg_selq) != 0) {
                return -3;
            }
            ro_iqcal_o = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
    
            #ifdef __LOG_DRV_PHY_CALI_TXLO__
            LOG_PRINTF("# RG_TX_DAC_QOFFSET = %d, got iqcal_o = %d\n", loop_i, ro_iqcal_o);
            #endif
    
            #ifdef __DBG_DUMP__
            *(dbg_dumpp_j+loop_i)= ro_iqcal_o;
            #endif
    
            if (ro_iqcal_o < ro_iqcal_o_min) {
                rg_tx_dac_qoffset_min_lo = loop_i    ;
                ro_iqcal_o_min           = ro_iqcal_o;
            }
        }
        SET_RG_TX_DAC_QOFFSET(rg_tx_dac_qoffset_min_lo);
        #ifdef __LOG_DRV_PHY_CALI_TXLO__
        LOG_PRINTF("## TX LO min when RG_TX_DAC_QOFFSET = %d\n", rg_tx_dac_qoffset_min_lo);
        #endif
    
        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif

    }

    // recovery register setting
    RG_TBL_LOAD(phyreg_tbl_recovery);

    rtbl_cali_txlo.ever_cali = 1;
    rtbl_cali_txlo.rtbl[0].result = rg_tx_dac_ioffset_min_lo;
    rtbl_cali_txlo.rtbl[1].result = rg_tx_dac_qoffset_min_lo;

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    #ifdef _STATISTIC_DRV_PHY_CALI_TXLO_
    LOG_PRINTF("txlo_rtbl(%d,:) = [%d, %d];\n", 
        sidx++, rtbl_cali_txlo.rtbl[0].result, rtbl_cali_txlo.rtbl[1].result);
    #endif

    return 0;
}

s32 drv_phy_cali_txiq  (const phy_cali_config *phy_cfg) {

    u32 loop_i;
    u32 loop_j;
    s32 ro_iqcal_o;

    s32 ro_iqcal_o_min;

    u32 rg_tx_iq_theta_min_img;
    u32 rg_tx_iq_alpha_min_img;

    #ifdef _STATISTIC_DRV_PHY_CALI_TXIQ_
    static u32 sidx=1;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[10];
    u32 ctime_idx = 0;
    #endif

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp_theta = sreg_ft_dbg;     // debug dump pointer
    u32* dbg_dumpp_alpha = sreg_ft_dbg + 32;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    for(loop_j=0; loop_j < _CALI_TXIQ_ROUND_; loop_j++) {

        // cali theta
        ro_iqcal_o_min = _RO_IQCAL_O_MAX_;
        rg_tx_iq_theta_min_img = 0;
        //
        for (loop_i=0 ; loop_i <32; loop_i++) {
            SET_RG_TX_IQ_THETA(loop_i);
            //
            if (drv_phy_sprm(phy_cfg->cfg_freq_rx, phy_cfg->cfg_selq) != 0) {
                return -3;
            }
            ro_iqcal_o = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
    
            #ifdef __LOG_DRV_PHY_CALI_TXIQ__
            LOG_PRINTF("# RG_TX_IQ_THETA = %d, got iqcal_o = %d\n", loop_i, ro_iqcal_o);
            #endif
            #ifdef __DBG_DUMP__
            *(dbg_dumpp_theta + loop_i) = ro_iqcal_o;
            #endif
    
            if (ro_iqcal_o < ro_iqcal_o_min) {
                rg_tx_iq_theta_min_img = loop_i    ;
                ro_iqcal_o_min         = ro_iqcal_o;
            }
        }
        SET_RG_TX_IQ_THETA(rg_tx_iq_theta_min_img);
    
        #ifdef __LOG_DRV_PHY_CALI_TXIQ__
        LOG_PRINTF("## TX IMG min when RG_TX_IQ_THETA = %d\n", rg_tx_iq_theta_min_img);
        #endif
    
    
        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif
    
        // cali alpha
        ro_iqcal_o_min = _RO_IQCAL_O_MAX_;
        rg_tx_iq_alpha_min_img = 0;
        //
        for (loop_i=0 ; loop_i <32; loop_i++) {
            SET_RG_TX_IQ_ALPHA(loop_i);
            //
            if (drv_phy_sprm(phy_cfg->cfg_freq_rx, phy_cfg->cfg_selq) != 0) {
                return -3;
            }
            ro_iqcal_o = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
    
            #ifdef __LOG_DRV_PHY_CALI_TXIQ__
            LOG_PRINTF("# RG_TX_IQ_ALPHA = %d, got iqcal_o = %d\n", loop_i, ro_iqcal_o);
            #endif
            #ifdef __DBG_DUMP__
            *(dbg_dumpp_alpha + loop_i) = ro_iqcal_o;
            #endif
    
    
            if (ro_iqcal_o < ro_iqcal_o_min) {
                rg_tx_iq_alpha_min_img = loop_i    ;
                ro_iqcal_o_min         = ro_iqcal_o;
            }
        }
        SET_RG_TX_IQ_ALPHA(rg_tx_iq_alpha_min_img);
        #ifdef __LOG_DRV_PHY_CALI_TXIQ__
        LOG_PRINTF("## TX IMG min when RG_TX_IQ_ALPHA = %d\n", rg_tx_iq_alpha_min_img);
        #endif
    
    
        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif
    }

    // recovery register setting
    RG_TBL_LOAD(phyreg_tbl_recovery);

    rtbl_cali_txiq.ever_cali = 1;
    rtbl_cali_txiq.rtbl[0].result = rg_tx_iq_theta_min_img;
    rtbl_cali_txiq.rtbl[1].result = rg_tx_iq_alpha_min_img;

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    #ifdef _STATISTIC_DRV_PHY_CALI_TXIQ_
    LOG_PRINTF("txiq_rtbl(%d,:) = [%d, %d];\n", 
        sidx++, rtbl_cali_txiq.rtbl[0].result, rtbl_cali_txiq.rtbl[1].result);
    #endif

    return 0;
}

s32 drv_phy_cali_rxiq_fft (const phy_cali_config *phy_cfg) {

    u32 loop_i;

    s32 loop_j_sgn;
    s32 loop_j;
    u32 loop_k;

    u32 rg_rx_iq_alpha_min_enrg;
    u32 rg_rx_iq_theta_min_enrg;

    s32 fft_enrg;
    s32 fft_enrg_min_alpha;
    s32 fft_enrg_min_theta;

    #ifdef _STATISTIC_DRV_PHY_CALI_RXIQ_
    static u32 sidx = 1;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[10];
    u32 ctime_idx = 0;
    #endif

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp_alpha = sreg_ft_dbg;     // debug dump pointer
    u32* dbg_dumpp_theta = sreg_ft_dbg + 32;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // dbg config begin
    // SET_RG_RX_IQ_SRC(1);    // lbk
    // dbg config end 

    for(loop_i=0; loop_i < _CALI_RXIQ_ROUND_; loop_i++) {

        fft_enrg_min_alpha      = _RO_IQCAL_O_MAX_;
        rg_rx_iq_alpha_min_enrg = 0;

        for (loop_j_sgn = _CALI_RXIQ_ALPHA_LB_; loop_j_sgn <= _CALI_RXIQ_ALPHA_UB_; loop_j_sgn++) {
            // loop index conversion
            if(loop_j_sgn < 0)
                loop_j = loop_j_sgn + _CALI_RXIQ_ALPHA_WRAP_;
            else
                loop_j = loop_j_sgn;
            //
            SET_RG_RX_IQ_ALPHA(loop_j);

            fft_enrg = 0;
            for(loop_k = 0; loop_k < _CALI_RXIQ_ALPHA_RPT_N_; loop_k++) {
                if (drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
                    return -3;
                }
                fft_enrg += drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
            }

            #ifdef __LOG_DRV_PHY_CALI_RXIQ__
            LOG_PRINTF("# RG_RX_IQ_ALPHA = %d, got fft_enrg = %d\n", loop_j, fft_enrg);
            #endif

            #ifdef __DBG_DUMP__
            *(dbg_dumpp_alpha + loop_j) = fft_enrg;
            #endif

            if (fft_enrg < fft_enrg_min_alpha) {
                rg_rx_iq_alpha_min_enrg = loop_j          ;
                fft_enrg_min_alpha      = fft_enrg;
            }
        }
        #ifdef __LOG_DRV_PHY_CALI_RXIQ__
        SET_RG_RX_IQ_ALPHA(rg_rx_iq_alpha_min_enrg);
        LOG_PRINTF("## RX negative freq min when RG_RX_IQ_ALPHA = %d\n", rg_rx_iq_alpha_min_enrg); 
        #endif

        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif

        fft_enrg_min_theta      = _RO_IQCAL_O_MAX_;
        rg_rx_iq_theta_min_enrg = 0;

        for (loop_j=0 ; loop_j <32; loop_j++) {

            SET_RG_RX_IQ_THETA(loop_j);

            fft_enrg = 0;
            for(loop_k = 0; loop_k < _CALI_RXIQ_THETA_RPT_N_; loop_k++) {
                if (drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
                    return -3;
                }
                fft_enrg += drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
            }

            #ifdef __LOG_DRV_PHY_CALI_RXIQ__
            LOG_PRINTF("# RG_RX_IQ_THETA = %d, got fft_enrg = %d\n", loop_j, fft_enrg);
            #endif

            #ifdef __DBG_DUMP__
            *(dbg_dumpp_theta + loop_j) = fft_enrg;
            #endif

            if (fft_enrg < fft_enrg_min_theta) {
                rg_rx_iq_theta_min_enrg = loop_j          ;
                fft_enrg_min_theta      = fft_enrg;
            }
        }
        #ifdef __LOG_DRV_PHY_CALI_RXIQ__
        SET_RG_RX_IQ_THETA(rg_rx_iq_theta_min_enrg);
        LOG_PRINTF("## IQ negative freq min when RG_RX_IQ_THETA = %d\n", rg_rx_iq_theta_min_enrg);
        #endif

        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif

    }

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // recovery register setting
    RG_TBL_LOAD(phyreg_tbl_recovery);
    rtbl_cali_rxiq.ever_cali = 1;
    rtbl_cali_rxiq.rtbl[0].result = rg_rx_iq_alpha_min_enrg;
    rtbl_cali_rxiq.rtbl[1].result = rg_rx_iq_theta_min_enrg;

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_j=0; loop_j < ctime_idx; loop_j++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_j, ctime[loop_j]);
    }
    #endif

    #ifdef _STATISTIC_DRV_PHY_CALI_RXIQ_
    LOG_PRINTF("rxiq_rtbl(%d,:) = [%d, %d];\n", 
        sidx++, rtbl_cali_rxiq.rtbl[0].result, rtbl_cali_rxiq.rtbl[1].result);    
    #endif

    return 0;

}

// s32 drv_phy_cali_rxiq (const phy_cali_config *phy_cfg) {
// 
//     s32 loop_i_sgn;
//     s32 loop_i;
//     u32 ro_iqcol_rdy;
// 
//     u32 rg_rx_iq_alpha_min_diff;
//     u32 rg_rx_iq_theta_min_col;
// 
//     s32 i_sprm;
//     s32 q_sprm;
// 
//     s32 iq_sprm_diff;
//     s32 iq_sprm_diff_abs;
//     s32 iq_sprm_diff_min;
// 
//     s32 iq_col;
//     s32 iq_col_min;
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     u32 ctime[10];
//     u32 ctime_idx = 0;
//     #endif
// 
//     #ifdef __DBG_DUMP__
//     u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
//     #endif
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     SET_TU0_TM_INT_MASK  (1    );
//     SET_TU0_TM_MODE      (0    );
//     SET_TU0_TM_INIT_VALUE(65535);
//     #endif
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     ctime[ctime_idx++] = REG32(0xc0000204);
//     #endif
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     ctime[ctime_idx++] = REG32(0xc0000204);
//     #endif
// 
//     // dbg config begin
//     // SET_RG_RX_IQ_SRC(1);    // lbk
//     // dbg config end 
// 
//     // cali alpha
//     iq_sprm_diff_min = 2*_RO_IQCAL_O_MAX_;
//     rg_rx_iq_alpha_min_diff = 0;
//     //
//     for (loop_i_sgn = _CALI_RXIQ_ALPHA_LB_; loop_i_sgn <= _CALI_RXIQ_ALPHA_UB_; loop_i_sgn++) {
//         // loop index conversion
//         if(loop_i_sgn < 0)
//             loop_i = loop_i_sgn + _CALI_RXIQ_ALPHA_WRAP_;
//         else
//             loop_i = loop_i_sgn;
//         //
//         SET_RG_RX_IQ_ALPHA(loop_i);
// 
//         //// I-part
//         if (drv_phy_sprm(phy_cfg->cfg_freq_rx, 0) != 0) {
//             return -3;
//         }
//         ////
//         i_sprm = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
// 
//         //// Q-part
//         if (drv_phy_sprm(phy_cfg->cfg_freq_rx, 1) != 0) {
//             return -3;
//         }
//         ////
//         q_sprm = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
// 
//         iq_sprm_diff     = i_sprm - q_sprm;
//         iq_sprm_diff_abs = (iq_sprm_diff < 0) ? -iq_sprm_diff : iq_sprm_diff;
// 
//         #ifdef __LOG_DRV_PHY_CALI_RXIQ__
//         LOG_PRINTF("# RG_RX_IQ_ALPHA = %d, got i_sprm = %d, q_sprm = %d, diff = %d\n", loop_i, i_sprm, q_sprm, iq_sprm_diff_abs);
//         #endif
//         #ifdef __DBG_DUMP__
//         *(dbg_dumpp+loop_i) = iq_sprm_diff_abs;
//         #endif
// 
//         if (iq_sprm_diff_abs < iq_sprm_diff_min ) {
//             rg_rx_iq_alpha_min_diff = loop_i          ;
//             iq_sprm_diff_min        = iq_sprm_diff_abs;
//         }
//     }
//     SET_RG_RX_IQ_ALPHA(rg_rx_iq_alpha_min_diff);
//     #ifdef __LOG_DRV_PHY_CALI_RXIQ__
//     LOG_PRINTF("## RX IQ SPRM DIFF min when RG_RX_IQ_ALPHY = %d\n",rg_rx_iq_alpha_min_diff); 
//     #endif
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     ctime[ctime_idx++] = REG32(0xc0000204);
//     #endif
// 
//     // cali theta
//     iq_col_min = _RO_IQCAL_O_MAX_;
//     rg_rx_iq_theta_min_col = 0;
//     #ifdef __DBG_DUMP__
//     dbg_dumpp += 32;
//     #endif
//     //
//     for (loop_i=0 ; loop_i <32; loop_i++) {
//         SET_RG_RX_IQ_THETA(loop_i);
//         //
//         SET_RG_IQCAL_IQCOL_EN(1);
//         // 
//         ro_iqcol_rdy = WAIT_RDY(RO_IQCAL_IQCOL_RDY);
// 
//         // error case 3: sprm circuit fail
//         if(ro_iqcol_rdy == 0) {
//             #ifdef __LOG_DRV_PHY_CALI_RXIQ__
//             LOG_PRINTF("DRV_PHY_CALI_RXIQ fail: IQ COL circuit fail\n");
//             #endif
//             return -3;
//         }
// 
//         iq_col = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
//         SET_RG_IQCAL_IQCOL_EN(0);
// 
//         #ifdef __LOG_DRV_PHY_CALI_RXIQ__
//         LOG_PRINTF("# RG_RX_IQ_THETA = %d, got iq_col = %d\n", loop_i, iq_col);
//         #endif
// 
//         #ifdef __DBG_DUMP__
//         *(dbg_dumpp++) = iq_col;
//         #endif
// 
//         if (iq_col < iq_col_min) {
//             rg_rx_iq_theta_min_col = loop_i;
//             iq_col_min             = iq_col;
//         }
//     }
//     SET_RG_RX_IQ_THETA(rg_rx_iq_theta_min_col);
//     #ifdef __LOG_DRV_PHY_CALI_RXIQ__
//     LOG_PRINTF("## IQ COL min when RG_RX_IQ_THETA = %d\n", rg_rx_iq_theta_min_col);
//     #endif
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     ctime[ctime_idx++] = REG32(0xc0000204);
//     #endif
// 
//     // recovery register setting
//     RG_TBL_LOAD(phyreg_tbl_recovery);
//     rtbl_cali_rxiq.ever_cali = 1;
//     rtbl_cali_rxiq.rtbl[0].result = rg_rx_iq_alpha_min_diff;
//     rtbl_cali_rxiq.rtbl[1].result = rg_rx_iq_theta_min_col;
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     ctime[ctime_idx++] = REG32(0xc0000204);
//     #endif
// 
//     #ifdef __LOG_DRV_PHY_MCU_TIME__
//     for(loop_i=0; loop_i < ctime_idx; loop_i++) {
//         LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
//     }
//     #endif
// 
//     return 0;
// 
// }

// u32 drv_phy_cali_papd_div(s32 dividend, s32 divider) {
// 
//     // IMP: to emulator the dividor Hw in iqcal:
//     // dividend, divider: s10.9
//     // quotient         : s11.9
// 
//     u32 dividend_abs;
//     u32 divider_abs;
// 
//     bool dividend_neg;
//     bool divider_neg ;
// 
//     s32 quotient;
//     u32 quotient_abs;
// 
//     if(dividend < 0) {
//         dividend_abs = (u32)-dividend;
//         dividend_neg = true;
//     }
//     else {
//         dividend_abs = (u32)dividend;
//         dividend_neg = false;
//     }
// 
//     if(divider < 0) {
//         divider_abs = (u32)-divider;
//         divider_neg = true;
//     }
//     else {
//         divider_abs = (u32)divider;
//         divider_neg = false;
//     }
// 
//     SET_DIV_OP1(dividend_abs << 9);
//     SET_DIV_OP2(divider_abs);
// 
//     quotient_abs = GET_QUOTIENT;
// 
//     if(dividend_neg ^ divider_neg) {
//         quotient = - (s32)quotient_abs;
//     }
//     else {
//         quotient = (s32) quotient_abs;
//     }
// 
//     // IMP: saturation behavior is not emulated here.
//     return quotient;
// }

s32 drv_phy_cali_papd_div(s32 dividend, s32 divider) {

    // IMP: to work with gain estimation circuit in Phy
    // dividend, divider: s10.9
    // quotient         : s11.9

    s32 quotient = 512 << 9;

    u32 ro_gp_div_rdy;

    SET_RG_GP_DIV_EN(1);

    SET_RG_IQCAL_DIV_DIVIDEND(dividend);
    SET_RG_IQCAL_DIV_DIVIDER (divider);

    #ifdef __LOG_DRV_PHY_CALI_PAPD__
    // LOG_PRINTF("### papd_div fx called ###\n");
    #endif

    ro_gp_div_rdy = WAIT_RDY(RO_GP_DIV_RDY);
    if(ro_gp_div_rdy) {
        quotient = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
        #ifdef __LOG_DRV_PHY_CALI_PAPD__
        // LOG_PRINTF("### %d / %d = %d ##\n", dividend, divider, quotient);
        #endif
    }

    SET_RG_GP_DIV_EN(0);

    quotient = quotient >> _CALI_PAPD_Q_SWF_;

    return quotient;
}
// co-processor mode gain estimation
/*
u32 drv_phy_cali_papd_gain_est (s32 loop_gain, u32 y_1, u32 y_0, u32 x_1, u32 x_0) {
    // IMP: in final implementation, this function would be replaced by a Hw engine.
    s32 L_i;
    s32 m_i;
    s32 delta_y;
    s32 delta_x;
    s32 gain;

    // # L_i:
    L_i = drv_phy_cali_papd_div((s32)y_0, (s32)x_0);
    L_i = loop_gain - L_i;

    // # M_i
    delta_y = (s32)y_1 - (s32)y_0;
    delta_x = (s32)x_1 - (s32)x_0;
    m_i = drv_phy_cali_papd_div((s32)delta_y, (s32)delta_x);

    gain = drv_phy_cali_papd_div(L_i, m_i);
    gain = gain + 512;

    return gain;
}
*/

/*
u32 drv_phy_cali_papd_gain_est (u32 loop_gain, u32 y_1, u32 y_0, u32 x_1, u32 x_0) {

    u32 ro_gain_est_rdy;
    u32 gain = 512;

    u32 y_1_sat = y_1;

    // input boundary condition check
    if (y_1 >= y_0) {
        if(y_1 < (y_0 + _CALI_PAPD_YDIF_MIN_))
            y_1_sat = y_0 + _CALI_PAPD_YDIF_MIN_;
    }
    else {
        if(y_1 > (y_0 - _CALI_PAPD_YDIF_MIN_))
            y_1_sat = y_0 - _CALI_PAPD_YDIF_MIN_;
    }

    // set registers
    SET_RG_DPD_GAIN_EST_Y0(y_0);
    SET_RG_DPD_GAIN_EST_Y1(y_1_sat);
    SET_RG_DPD_GAIN_EST_X0(x_0);
    SET_RG_DPD_LOOP_GAIN(loop_gain);

    SET_RG_DPD_GAIN_EST_EN(1);

    ro_gain_est_rdy = WAIT_RDY(RO_GAIN_EST_RDY);
    if(ro_gain_est_rdy) {
        gain = GET_RO_DPD_GAIN;
        #ifdef __LOG_DRV_PHY_CALI_PAPD__
        LOG_PRINTF("## loop_gain %d, x_0 %d, y_1 %d, y_0 %d, gain %d\n", GET_RG_DPD_LOOP_GAIN, GET_RG_DPD_GAIN_EST_X0, GET_RG_DPD_GAIN_EST_Y1, GET_RG_DPD_GAIN_EST_Y0, gain);
        #endif
    }
    SET_RG_DPD_GAIN_EST_EN(0);

    return gain;
}
*/

// # gain estimation method without extrapolation
// ## x_0:          current_x
// ## loop_gain:    target linearity
u32 drv_phy_cali_papd_gain_est (u32 loop_gain, u32 y_1, u32 y_0, u32 x_1, u32 x_0) {

    u32 y_trgt;     // y_trgt = x_0 * loop_gain
    u32 x_trgt;     // x_trgt -> y_trgt
    u32 gain  ;     // x_trgt / x_0

    u32 x0_trgt;
    u32 x1_trgt;
    u32 y0_trgt;
    u32 y1_trgt;

    u32 x0_gain;
    u32 x1_gain;

    s32 xdif_trgt;
    s32 ydif_trgt;
    s32 x0_dif_trgt;
    s32 y0_dif_trgt;
    
    s32 ydif_trgt_abs;

    u32 x_idx;
    u32 x_idx_vld;

    u64 multi_tmp;

    // ## y_trgt (target-y)
    multi_tmp = (u64) loop_gain * x_0;
    // ### bit-width
    multi_tmp = multi_tmp +  (1 << (_CALI_PAPD_X_WF_ + _CALI_PAPD_GAIN_WF_ - _CALI_PAPD_Y_WF_ -1));
    multi_tmp = multi_tmp >> (_CALI_PAPD_X_WF_ + _CALI_PAPD_GAIN_WF_ - _CALI_PAPD_Y_WF_);
    // ### sat.
    if (multi_tmp >= _CALI_PAPD_Y_MAX_) {
        multi_tmp = _CALI_PAPD_Y_MAX_;
    }
    y_trgt = (u32) multi_tmp;
    #ifdef __LOG_DRV_PHY_CALI_PAPD__
    // LOG_PRINTF("## {x_0, loop_gain, y_trgt} = {%4d, %4d, %4d}\n", x_0, loop_gain, y_trgt);
    #endif

    // ## find the interpolation range
    x_idx_vld = 0;
    for(x_idx=0; x_idx < (_CALI_PAPD_OBSR_N_-1); x_idx++) {
        if ((y_trgt >= papd_ltbl_am[x_idx]) && (y_trgt <= papd_ltbl_am[x_idx+1])) {
            x_idx_vld = 1;
            break;
        }
    }

    if(x_idx_vld == 1) {

        x0_trgt = papd_tbl_x[x_idx];
        x1_trgt = papd_tbl_x[x_idx+1];

        x0_gain = (u32) rtbl_cali_papd.rtbl[_CALI_PAPD_RTBL_IDX_GAIN_ +x_idx  ].result;
        x1_gain = (u32) rtbl_cali_papd.rtbl[_CALI_PAPD_RTBL_IDX_GAIN_ +x_idx+1].result;

        y0_trgt = papd_ltbl_am[x_idx];
        y1_trgt = papd_ltbl_am[x_idx+1];

        // ### cali x*_trgt in iteration runs

        // #### x0
        if (x0_gain != 0) {
            // ##### not the first round
            multi_tmp = (u64) x0_gain * x0_trgt;
            // ##### bit-width
            multi_tmp = multi_tmp +  (1 << (_CALI_PAPD_X_WF_ + _CALI_PAPD_GAIN_WF_ - _CALI_PAPD_X_WF_ -1));
            multi_tmp = multi_tmp >> (_CALI_PAPD_X_WF_ + _CALI_PAPD_GAIN_WF_ - _CALI_PAPD_X_WF_);

            x0_trgt = (u32)multi_tmp;
        }

        // #### x1
        if (x1_gain != 0) {
            // ##### not the first round
            multi_tmp = (u64) x1_gain * x1_trgt;
            // ##### bit-width
            multi_tmp = multi_tmp +  (1 << (_CALI_PAPD_X_WF_ + _CALI_PAPD_GAIN_WF_ - _CALI_PAPD_X_WF_ -1));
            multi_tmp = multi_tmp >> (_CALI_PAPD_X_WF_ + _CALI_PAPD_GAIN_WF_ - _CALI_PAPD_X_WF_);

            x1_trgt = (u32)multi_tmp;
        }

        // ### interpolation, got x_trgt
        // #### condition check
        ydif_trgt_abs = y1_trgt - y0_trgt;
        if (ydif_trgt_abs < 0)
            ydif_trgt_abs = -ydif_trgt_abs;

        // #### not necessary use interpolation case
        if (ydif_trgt_abs <= _CALI_PAPD_YDIF_MIN_) {
            x_trgt = x0_trgt + x1_trgt +1;
            x_trgt = x_trgt > 1;
        }
        // #### do interpolation
        else {

            ydif_trgt = (s32) y1_trgt - y0_trgt;
            xdif_trgt = (s32) x1_trgt - x0_trgt;

            y0_dif_trgt = (s32) y_trgt - y0_trgt;

            x0_dif_trgt = drv_phy_cali_papd_div(y0_dif_trgt, ydif_trgt);

            multi_tmp = (u64) xdif_trgt * x0_dif_trgt;
            multi_tmp = multi_tmp + (1 << (_CALI_PAPD_Q_SWF_ + _CALI_PAPD_X_WF_ - _CALI_PAPD_X_WF_ -1));
            multi_tmp = multi_tmp >> (_CALI_PAPD_Q_SWF_ + _CALI_PAPD_X_WF_ - _CALI_PAPD_X_WF_);

            x0_dif_trgt = (u32) multi_tmp; 

            // #### real x_trgt
            x_trgt = (u32) x0_trgt + x0_dif_trgt;

            // #### in case some ...
            if (x_trgt >= (u32) x1_trgt) {
                x_trgt = (u32) x1_trgt;
            }
        }
        // ##
        gain = (u32)drv_phy_cali_papd_div((s32)x_trgt, (s32)x_0);
    }
    else {
        x_trgt = 0; // as a flag to signal out-of-table 
        gain   = _CALI_PAPD_GAIN_MAX_;
    }

    #ifdef __LOG_DRV_PHY_CALI_PAPD__
    // LOG_PRINTF("### {x-gain, y_trgt, x_trgt, x_0} = {%4d, %4d, %4d, %4d}\n", gain, y_trgt, x_trgt, x_0);
    #endif

    return gain;

}

s32 drv_phy_cali_ph_wrap (s32 ph) {
    if (ph > _CALI_PAPD_PH_MAX_)
        return ph - _CALI_PAPD_PH_WRAP_;
    else if (ph < _CALI_PAPD_PH_MIN_)
        return ph + _CALI_PAPD_PH_WRAP_;
    else
        return ph;
}

s32 drv_phy_cali_papd_learning (const phy_cali_config *phy_cfg) {

    u32 loop_i;
    u32 loop_k;
    u32 loop_n;
    u32 loop_n_log;
    u32 ro_phest_rdy;

    u32 cfg_pa;

    #ifdef _STATISTIC_DRV_PHY_CALI_PAPD_
    static u32 sidx = 1;
    #endif

    cfg_pa = g_rf_config.cfg_pa;
    #ifdef __LOG_DRV_PHY_CALI_PAPD__
    LOG_PRINTF("# cfg_pa = %d\n", cfg_pa);
    #endif
    RG_TBL_LOAD(rfreg_tbl_papd_bypa[cfg_pa]);

    SET_RG_IQCAL_SPRM_FREQ (phy_cfg->cfg_freq_rx);      
    SET_RG_IQCAL_SPRM_SELQ (phy_cfg->cfg_selq   );

    // ## phase estimation
    SET_RG_PHEST_STBY(1);

    // ## DPD AM/PM learning
    for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
        // ### set Tx scale
        SET_RG_TX_I_SCALE(papd_tbl_x[loop_i] >> 2);
        SET_RG_TX_Q_SCALE(papd_tbl_x[loop_i] >> 2);

        if (papd_tbl_x[loop_i] >= _CALI_PAPD_HA_THD_) {
            loop_n     = _CALI_PAPD_HA_RPT_N_;
            loop_n_log = _CALI_PAPD_HA_RPT_N_LOG_;
        }
        else {
            loop_n     = _CALI_PAPD_LA_RPT_N_;
            loop_n_log = _CALI_PAPD_LA_RPT_N_LOG_;
        }

        papd_ltbl_am[loop_i] = 0;
        papd_ltbl_pm[loop_i] = 0;

        for(loop_k=0; loop_k <loop_n; loop_k++) {

            // ### enable phase-estimation/cordic 
            SET_RG_PHEST_EN(1);
            // ### polling cordic ready
            ro_phest_rdy = WAIT_RDY(RO_PHEST_RDY);
            if(ro_phest_rdy == 0) {
                #ifdef __LOG_DRV_PHY_CALI_PAPD__
                LOG_PRINTF("DRV_PHY_CALI_PAPD fail: phest circuit fail\n");
                #endif
                return -3;
            }
            // ### latch cordic results
            papd_ltbl_am[loop_i] += GET_RO_AMP_O;
            papd_ltbl_pm[loop_i] += drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
            // ### disable phase-estimation/cordic
            SET_RG_PHEST_EN(0);
        }
        papd_ltbl_am[loop_i] = papd_ltbl_am[loop_i] >> loop_n_log;
        papd_ltbl_pm[loop_i] = papd_ltbl_pm[loop_i] >> loop_n_log;
    }

    SET_RG_PHEST_STBY(0);

    #ifdef _STATISTIC_DRV_PHY_CALI_PAPD_
    LOG_PRINTF("papdl_ltbl_am(%d,:) = [", sidx);
    for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
        LOG_PRINTF(" %d", papd_ltbl_am[loop_i]);
    }
    LOG_PRINTF("];\n");
    //
    LOG_PRINTF("papdl_ltbl_pm(%d,:) = [", sidx++);
    for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
        LOG_PRINTF(" %d", papd_ltbl_pm[loop_i]);
    }
    LOG_PRINTF("];\n");
    #endif

    // recovery register setting
    RG_TBL_LOAD(phyreg_tbl_recovery);

    return 0;
}

s32 drv_phy_cali_papd (const phy_cali_config *phy_cfg) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_k;
    u32 loop_n;
    u32 loop_n_log;
    u32 ro_phest_rdy;

    u16 rg_dpd_gain[_CALI_PAPD_OBSR_N_];
    s16 rg_dpd_ph  [_CALI_PAPD_OBSR_N_];
    u32 rg_dpd_ph_vld;

    u32 loop_gain     = _CALI_PAPD_GAIN_UNIT_;
    u32 loop_gain_vld = 0;
    u32 cfg_pa;

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[10];
    u32 ctime_idx = 0;
    #endif

    #ifdef _STATISTIC_DRV_PHY_CALI_PAPD_
    static u32 l_sidx = 1;
    static u32 r_sidx = 1;
    #endif

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp_am = sreg_ft_dbg     ;     // debug dump pointer
    u32* dbg_dumpp_pm = sreg_ft_dbg + 32;     // debug dump pointer
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    cfg_pa = g_rf_config.cfg_pa;
    RG_TBL_LOAD(rfreg_tbl_papd_bypa[cfg_pa]);

    SET_RG_IQCAL_SPRM_FREQ (phy_cfg->cfg_freq_rx);      
    SET_RG_IQCAL_SPRM_SELQ (phy_cfg->cfg_selq   );

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    rg_dpd_ph_vld = 0;
    for (loop_j = 0; loop_j < _CALI_PAPD_RPT_N_; loop_j++) {

        // # DPD AM    // ## phase estimation
        SET_RG_PHEST_STBY(1);
    
        // ## DPD AM/PM learning
        for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            // ### set Tx scale
            SET_RG_TX_I_SCALE(papd_tbl_x[loop_i] >> 2);
            SET_RG_TX_Q_SCALE(papd_tbl_x[loop_i] >> 2);

            if (papd_tbl_x[loop_i] >= _CALI_PAPD_HA_THD_) {
                loop_n     = _CALI_PAPD_HA_RPT_N_;
                loop_n_log = _CALI_PAPD_HA_RPT_N_LOG_;
            }
            else {
                loop_n     = _CALI_PAPD_LA_RPT_N_;
                loop_n_log = _CALI_PAPD_LA_RPT_N_LOG_;
            }

            papd_ltbl_am[loop_i] = 0;
            papd_ltbl_pm[loop_i] = 0;

            for(loop_k=0; loop_k <loop_n; loop_k++) {

                // ### enable phase-estimation/cordic 
                SET_RG_PHEST_EN(1);
                // ### polling cordic ready
                ro_phest_rdy = WAIT_RDY(RO_PHEST_RDY);
                if(ro_phest_rdy == 0) {
                    #ifdef __LOG_DRV_PHY_CALI_PAPD__
                    LOG_PRINTF("DRV_PHY_CALI_PAPD fail: phest circuit fail\n");
                    #endif
                    return -3;
                }
                // ### latch cordic results
                papd_ltbl_am[loop_i] += GET_RO_AMP_O;
                papd_ltbl_pm[loop_i] += drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
                // ### disable phase-estimation/cordic
                SET_RG_PHEST_EN(0);
            }
            papd_ltbl_am[loop_i] = papd_ltbl_am[loop_i] >> loop_n_log;
            papd_ltbl_pm[loop_i] = papd_ltbl_pm[loop_i] >> loop_n_log;

            #ifdef __DBG_DUMP__
            *(dbg_dumpp_am +loop_i) = papd_ltbl_am[loop_i];
            *(dbg_dumpp_pm +loop_i) = papd_ltbl_pm[loop_i];
            #endif
        }
    
        SET_RG_PHEST_STBY(0);
    
        #ifdef _STATISTIC_DRV_PHY_CALI_PAPD_
        LOG_PRINTF("papd_ltbl_am(%d,:) = [", l_sidx);
        for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            LOG_PRINTF(" %d", papd_ltbl_am[loop_i]);
        }
        LOG_PRINTF("];\n");
        //
        LOG_PRINTF("papd_ltbl_pm(%d,:) = [", l_sidx);
        for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            LOG_PRINTF(" %d", papd_ltbl_pm[loop_i]);
        }
        LOG_PRINTF("];\n");
        #endif
    
        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif
    
        /*
        // debug code
        papd_ltbl_am[0]  =  26;
        papd_ltbl_am[1]  =  52;
        papd_ltbl_am[2]  =  78;
        papd_ltbl_am[3]  = 105;
        papd_ltbl_am[4]  = 132;
        papd_ltbl_am[5]  = 162;
        papd_ltbl_am[6]  = 177;
        papd_ltbl_am[7]  = 192;
        papd_ltbl_am[8]  = 207;
        papd_ltbl_am[9]  = 223;
        papd_ltbl_am[10] = 236;
        papd_ltbl_am[11] = 253;
        papd_ltbl_am[12] = 267;
        papd_ltbl_am[13] = 283;
        papd_ltbl_am[14] = 297;
        papd_ltbl_am[15] = 311;
        papd_ltbl_am[16] = 324;
        papd_ltbl_am[17] = 336;
        papd_ltbl_am[18] = 350;
        papd_ltbl_am[19] = 361;
        papd_ltbl_am[20] = 372;
        papd_ltbl_am[21] = 384;
        papd_ltbl_am[22] = 395;
        papd_ltbl_am[23] = 406;
        papd_ltbl_am[24] = 417;
        papd_ltbl_am[25] = 429;
        // 
        papd_ltbl_pm[0]  = -173136;
        papd_ltbl_pm[1]  = -175760;
        papd_ltbl_pm[2]  = -175936;
        papd_ltbl_pm[3]  = -177408;
        papd_ltbl_pm[4]  = -179304;
        papd_ltbl_pm[5]  = -180376;
        papd_ltbl_pm[6]  = -180720;
        papd_ltbl_pm[7]  = -181728;
        papd_ltbl_pm[8]  = -182688;
        papd_ltbl_pm[9]  = -183328;
        papd_ltbl_pm[10] = -183824;
        papd_ltbl_pm[11] = -185056;
        papd_ltbl_pm[12] = -185456;
        papd_ltbl_pm[13] = -185952;
        papd_ltbl_pm[14] = -186416;
        papd_ltbl_pm[15] = -186736;
        papd_ltbl_pm[16] = -186880;
        papd_ltbl_pm[17] = -186688;
        papd_ltbl_pm[18] = -185920;
        papd_ltbl_pm[19] = -184672;
        papd_ltbl_pm[20] = -183296;
        papd_ltbl_pm[21] = -181088;
        papd_ltbl_pm[22] = -178928;
        papd_ltbl_pm[23] = -176656;
        papd_ltbl_pm[24] = -173904;
        papd_ltbl_pm[25] = -171184;
        // 
        */
    
        #ifdef __LOG_DRV_PHY_CALI_PAPD__
        LOG_PRINTF("## tx x_i %3d, am %3d, pm %3d\n", papd_tbl_x[0], papd_ltbl_am[0], papd_ltbl_pm[0]);
        for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            LOG_PRINTF("## tx x_i %3d, am %3d (+ %3d), pm %3d (+ %3d)\n", 
                    papd_tbl_x[loop_i], papd_ltbl_am[loop_i], (papd_ltbl_am[loop_i] - papd_ltbl_am[loop_i-1]),
                                        papd_ltbl_pm[loop_i], (papd_ltbl_pm[loop_i] - papd_ltbl_pm[loop_i-1]));
        }
        #endif
    
        // ## DPD AM cali./compensation 
        // ### set reference point
        for (loop_i=0; loop_i <= _CALI_PAPD_GAIN_REF_POINT_; loop_i++) {
            rg_dpd_gain[loop_i] = _CALI_PAPD_GAIN_UNIT_;
        }
        // ### set loop gain
        if (loop_gain_vld == 0) {
            loop_gain     = drv_phy_cali_papd_div(papd_ltbl_am[_CALI_PAPD_GAIN_REF_POINT_], papd_tbl_x[_CALI_PAPD_GAIN_REF_POINT_]);
            loop_gain_vld = 1;
        }

        #ifdef _STATISTIC_DRV_PHY_CALI_PAPD_
        LOG_PRINTF("papd_loop_gain(%d) = %d;\n", l_sidx++, loop_gain);
        #endif

        #ifdef __LOG_DRV_PHY_CALI_PAPD__
        LOG_PRINTF("# loop_gain is %3d\n", loop_gain);
        #endif
    
        // ###
        for (loop_i = (_CALI_PAPD_GAIN_REF_POINT_ +1); loop_i < (_CALI_PAPD_OBSR_N_ -1); loop_i++ ) {
            if (papd_ltbl_am[loop_i] >= _CALI_PAPD_AM_BD_) {
                break;
            }
            rg_dpd_gain[loop_i] = (u16) drv_phy_cali_papd_gain_est(loop_gain, papd_ltbl_am[loop_i+1], papd_ltbl_am[loop_i], papd_tbl_x[loop_i+1], papd_tbl_x[loop_i]);
        }
        for (; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            rg_dpd_gain[loop_i] = rg_dpd_gain[loop_i-1];
        }
    
        #ifdef __LOG_DRV_PHY_CALI_PAPD__
        for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            LOG_PRINTF("## tx x_i %3d, gain %3d\n", papd_tbl_x[loop_i], rg_dpd_gain[loop_i]);
        }
        #endif
        // ### saturation
        for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            if (rg_dpd_gain[loop_i] > _CALI_PAPD_GAIN_MAX_)
                rg_dpd_gain[loop_i] = _CALI_PAPD_GAIN_MAX_;
        }
    
        #ifdef __LOG_DRV_PHY_MCU_TIME__
        ctime[ctime_idx++] = REG32(0xc0000204);
        #endif
    
        // ## DPD PM cali./compensation 
        if (rg_dpd_ph_vld == 0) {
            // ### adjust bitwidth
            for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
                papd_ltbl_pm[loop_i] = papd_ltbl_pm[loop_i] >> (_CALI_PAPD_PHO_SWF_ - _CALI_PAPD_PHI_SWF_);
            }
        
            for (loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++ ) {
                if (loop_i <= _CALI_PAPD_PH_REF_POINT_) {
                    rg_dpd_ph[loop_i] = _CALI_PAPD_PH_NULL_;
                }
                else {
                    rg_dpd_ph[loop_i] = (s16)drv_phy_cali_ph_wrap(papd_ltbl_pm[loop_i] - papd_ltbl_pm[_CALI_PAPD_PH_REF_POINT_]);
                }
            }
        
            #ifdef __LOG_DRV_PHY_CALI_PAPD__
            for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
                LOG_PRINTF("## tx x_i %d, phase %d\n", papd_tbl_x[loop_i], rg_dpd_ph[loop_i]);
            }
            #endif
        
            // ### saturation
            for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
                if (rg_dpd_ph[loop_i] > _CALI_PAPD_PH_MAX_)
                    rg_dpd_ph[loop_i] = _CALI_PAPD_PH_MAX_;
                else if (rg_dpd_ph[loop_i] < _CALI_PAPD_PH_MIN_)
                    rg_dpd_ph[loop_i] = _CALI_PAPD_PH_MIN_;
            }
            rg_dpd_ph_vld = 1;
        }


        #ifdef _STATISTIC_DRV_PHY_CALI_PAPD_
        LOG_PRINTF("papd_rtbl_am(%d,:) = [", r_sidx);
        for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            LOG_PRINTF(" %d", rg_dpd_gain[loop_i]);
        }
        LOG_PRINTF("];\n");
        //
        LOG_PRINTF("papd_rtbl_pm(%d,:) = [", r_sidx++);
        for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            LOG_PRINTF(" %d", rg_dpd_ph[loop_i]);
        }
        LOG_PRINTF("];\n");
        #endif

        // ### load gain
        for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
            rtbl_cali_papd.rtbl[loop_i + _CALI_PAPD_RTBL_IDX_GAIN_].result =      rg_dpd_gain[loop_i];
            rtbl_cali_papd.rtbl[loop_i + _CALI_PAPD_RTBL_IDX_PH_  ].result = (u16)rg_dpd_ph  [loop_i];
        }

        // write config
        rtbl_cali_papd.rtbl[0].result = 1;    // rg_dpd_am_en
        rtbl_cali_papd.rtbl[1].result = 1;    // rg_dpd_pm_en
        rtbl_cali_papd.rtbl[2].result = g_rf_config.cfg_def_tx_scale_11b;      // tx_scale_11b
        rtbl_cali_papd.rtbl[3].result = g_rf_config.cfg_def_tx_scale_11b_p0d5; // tx_scale_11b_p0d5
        rtbl_cali_papd.rtbl[4].result = g_rf_config.cfg_def_tx_scale_11g;      // tx_scale_11g
        rtbl_cali_papd.rtbl[5].result = g_rf_config.cfg_def_tx_scale_11g_p0d5; // tx_scale_11g_p0d5

        // Write signed value to registers
        rtbl_cali_pre_load(&rtbl_cali_papd);

    }

    for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
        rtbl_cali_papd.rtbl[loop_i + _CALI_PAPD_RTBL_IDX_GAIN_].result =      rg_dpd_gain[loop_i];
        rtbl_cali_papd.rtbl[loop_i + _CALI_PAPD_RTBL_IDX_PH_  ].result = (u16)rg_dpd_ph  [loop_i];
    }

    // write config
    rtbl_cali_papd.rtbl[0].result = 1;    // rg_dpd_am_en
    rtbl_cali_papd.rtbl[1].result = 1;    // rg_dpd_pm_en
    rtbl_cali_papd.rtbl[2].result = g_rf_config.cfg_def_tx_scale_11b;      // tx_scale_11b
    rtbl_cali_papd.rtbl[3].result = g_rf_config.cfg_def_tx_scale_11b_p0d5; // tx_scale_11b_p0d5
    rtbl_cali_papd.rtbl[4].result = g_rf_config.cfg_def_tx_scale_11g;      // tx_scale_11g
    rtbl_cali_papd.rtbl[5].result = g_rf_config.cfg_def_tx_scale_11g_p0d5; // tx_scale_11g_p0d5

    // Write signed value to registers
    rtbl_cali_pre_load(&rtbl_cali_papd);

    rtbl_cali_papd.ever_cali = 1;

    #ifdef _STATISTIC_DRV_PHY_CALI_PAPD_
    LOG_PRINTF("papd_rtbl_am(%d,:) = [", r_sidx);
    for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
        LOG_PRINTF(" %d", rg_dpd_gain[loop_i]);
    }
    LOG_PRINTF("];\n");
    //
    LOG_PRINTF("papd_rtbl_pm(%d,:) = [", r_sidx++);
    for(loop_i = 0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
        LOG_PRINTF(" %d", rg_dpd_ph[loop_i]);
    }
    LOG_PRINTF("];\n");
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // // ## do verifications
    // SET_RG_DPD_AM_EN(1);
    // SET_RG_DPD_PM_EN(1);
    // LOG_PRINTF("after dpd cali.\n");
    // // ## DPD AM/PM learning
    // for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
    //     // ### set Tx scale
    //     SET_RG_TX_I_SCALE(papd_tbl_x[loop_i] >> 2);
    //     SET_RG_TX_Q_SCALE(papd_tbl_x[loop_i] >> 2);
    //     // ### enable phase-estimation/cordic 
    //     SET_RG_PHEST_EN(1);
    //     // ### polling cordic ready
    //     ro_phest_rdy = 0;
    //     for(phest_pcnt=0; phest_pcnt < _PHEST_CNT_MAX_; phest_pcnt++) {
    //         ro_phest_rdy = GET_RO_PHEST_RDY;
    //         if(ro_phest_rdy) break;
    //     }
    //     if(ro_phest_rdy == 0) {
    //         #ifdef __LOG_DRV_PHY_CALI_PAPD__
    //         LOG_PRINTF("DRV_PHY_CALI_PAPD fail: phest circuit fail\n");
    //         #endif
    //         return -3;
    //     }
    //     // ### latch cordic results
    //     papd_ltbl_am[loop_i] = GET_RO_AMP_O;
    //     papd_ltbl_pm[loop_i] = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);
    //     // ### disable phase-estimation/cordic
    //     SET_RG_PHEST_EN(0);
    // }
    // for (loop_i=0; loop_i < _CALI_PAPD_OBSR_N_; loop_i++) {
    //     LOG_PRINTF("## tx x_i %d, am %d, pm %d\n", papd_tbl_x[loop_i], papd_ltbl_am[loop_i], papd_ltbl_pm[loop_i]);
    // }
    // SET_RG_DPD_AM_EN(0);
    // SET_RG_DPD_PM_EN(0);

    // recovery register setting
    RG_TBL_LOAD(phyreg_tbl_recovery);

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    return 0;

}

void rg_idacai_pgag0_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG0(rg_rxdc);  }
void rg_idacai_pgag1_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG1(rg_rxdc);  }
void rg_idacai_pgag2_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG2(rg_rxdc);  }
void rg_idacai_pgag3_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG3(rg_rxdc);  }
void rg_idacai_pgag4_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG4(rg_rxdc);  }
void rg_idacai_pgag5_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG5(rg_rxdc);  }
void rg_idacai_pgag6_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG6(rg_rxdc);  }
void rg_idacai_pgag7_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG7(rg_rxdc);  }
void rg_idacai_pgag8_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG8(rg_rxdc);  }
void rg_idacai_pgag9_load  (u32 rg_rxdc) { SET_RG_IDACAI_PGAG9(rg_rxdc);  }
void rg_idacai_pgag10_load (u32 rg_rxdc) { SET_RG_IDACAI_PGAG10(rg_rxdc); }
void rg_idacai_pgag11_load (u32 rg_rxdc) { SET_RG_IDACAI_PGAG11(rg_rxdc); }
void rg_idacai_pgag12_load (u32 rg_rxdc) { SET_RG_IDACAI_PGAG12(rg_rxdc); }
void rg_idacai_pgag13_load (u32 rg_rxdc) { SET_RG_IDACAI_PGAG13(rg_rxdc); }
void rg_idacai_pgag14_load (u32 rg_rxdc) { SET_RG_IDACAI_PGAG14(rg_rxdc); }
void rg_idacai_pgag15_load (u32 rg_rxdc) { SET_RG_IDACAI_PGAG15(rg_rxdc); }

void rg_idacaq_pgag0_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG0(rg_rxdc);  }
void rg_idacaq_pgag1_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG1(rg_rxdc);  }
void rg_idacaq_pgag2_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG2(rg_rxdc);  }
void rg_idacaq_pgag3_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG3(rg_rxdc);  }
void rg_idacaq_pgag4_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG4(rg_rxdc);  }
void rg_idacaq_pgag5_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG5(rg_rxdc);  }
void rg_idacaq_pgag6_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG6(rg_rxdc);  }
void rg_idacaq_pgag7_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG7(rg_rxdc);  }
void rg_idacaq_pgag8_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG8(rg_rxdc);  }
void rg_idacaq_pgag9_load  (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG9(rg_rxdc);  }
void rg_idacaq_pgag10_load (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG10(rg_rxdc); }
void rg_idacaq_pgag11_load (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG11(rg_rxdc); }
void rg_idacaq_pgag12_load (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG12(rg_rxdc); }
void rg_idacaq_pgag13_load (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG13(rg_rxdc); }
void rg_idacaq_pgag14_load (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG14(rg_rxdc); }
void rg_idacaq_pgag15_load (u32 rg_rxdc) { SET_RG_IDACAQ_PGAG15(rg_rxdc); }

s32 drv_phy_cali_rxdc (const phy_cali_config *phy_cfg) {

    u32 loop_i;
    u32 idaca_dc_min;

    u32 idaca_dc_pre;
    u32 idaca_dc_mid;

    u32 loop_pgag;

    // for ADC version of RX DC offset estimation
    s32 ro_adc;
    u32 ro_adc_abs;
    u32 ro_adc_abs_min;

    #ifdef _STATISTIC_DRV_PHY_CALI_RXDC_
    static u32 sidx = 1; 
    static u32 loop_s;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[10];
    u32 ctime_idx = 0;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // I path cali.
    for(loop_pgag = 0; loop_pgag < 16; loop_pgag++) {
        SET_RG_PGAG(loop_pgag);

        // use binary search to decide bit bit-4 ~ bit-1
        idaca_dc_pre = 0;
        for (loop_i=0; loop_i <6; loop_i++) {
            idaca_dc_mid = idaca_dc_pre + (1 << (5-loop_i));
            
            /*
            rtbl_cali_rxdc.rtbl[loop_pgag].result = idaca_dc_mid;
            rtbl_cali_pre_load(&rtbl_cali_rxdc);
            */
            (*rg_rxdc_set_i[loop_pgag])(idaca_dc_mid);

            ro_adc = (s32)GET_RG_RX_ADC_I_RO_BIT;       // dummy read
            ro_adc = (s32)GET_RG_RX_ADC_I_RO_BIT -128; 

            if (ro_adc < 0)
                idaca_dc_pre = idaca_dc_mid;
        }

        idaca_dc_min       =  idaca_dc_pre       ; 
        ro_adc_abs_min     = _RO_ADC_ABS_MAX_    ;
        for(loop_i = 0; loop_i < 2; loop_i++) {

            /*
            rtbl_cali_rxdc.rtbl[loop_pgag].result = idaca_dc_pre + loop_i;
            rtbl_cali_pre_load(&rtbl_cali_rxdc);
            */
            (*rg_rxdc_set_i[loop_pgag])(idaca_dc_pre + loop_i);

            //
            ro_adc = (s32)GET_RG_RX_ADC_I_RO_BIT;       // dummy read
            ro_adc = (s32)GET_RG_RX_ADC_I_RO_BIT -128; 

            if (ro_adc < 0) {
                ro_adc_abs = (u32) -ro_adc;
            }
            else {
                ro_adc_abs = (u32) ro_adc;
            }
            if(ro_adc_abs < ro_adc_abs_min) {
                ro_adc_abs_min = ro_adc_abs;
                idaca_dc_min   = idaca_dc_pre + loop_i    ;
            }
        }

        #ifdef __LOG_DRV_PHY_CALI_RXDC__
        LOG_PRINTF("# I pgag = %d, idaca = %d, got rx-dc min = %d\n", loop_pgag, idaca_dc_min, ro_adc_abs_min);
        #endif
        (*rg_rxdc_set_i[loop_pgag])(idaca_dc_min);
        rtbl_cali_rxdc.rtbl[loop_pgag].result = idaca_dc_min;
    }

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // Q path cali.
    for(loop_pgag = 0; loop_pgag < 16; loop_pgag++) {
        SET_RG_PGAG(loop_pgag);

        // use binary search to decide bit bit-4 ~ bit-1
        idaca_dc_pre = 0;
        for (loop_i=0; loop_i <6; loop_i++) {
            idaca_dc_mid = idaca_dc_pre + (1 << (5-loop_i));
            
            /*
            rtbl_cali_rxdc.rtbl[loop_pgag + _CALI_RXDC_RTBL_IDX_Q_].result = idaca_dc_mid;
            rtbl_cali_pre_load(&rtbl_cali_rxdc);
            */
            (*rg_rxdc_set_q[loop_pgag])(idaca_dc_mid);

            //////
            ro_adc     = GET_RG_RX_ADC_Q_RO_BIT;
            ro_adc     = GET_RG_RX_ADC_Q_RO_BIT - 128;

            if (ro_adc < 0)
                idaca_dc_pre = idaca_dc_mid;
        }

        idaca_dc_min       =  idaca_dc_pre       ; 
        ro_adc_abs_min     = _RO_ADC_ABS_MAX_    ;
        for(loop_i = 0; loop_i < 2; loop_i++) {
            /*
            rtbl_cali_rxdc.rtbl[loop_pgag + _CALI_RXDC_RTBL_IDX_Q_].result = idaca_dc_pre + loop_i;
            rtbl_cali_pre_load(&rtbl_cali_rxdc);
            */
            (*rg_rxdc_set_q[loop_pgag])(idaca_dc_pre + loop_i);
            //

            ro_adc = (s32)GET_RG_RX_ADC_Q_RO_BIT;       // dummy read
            ro_adc = (s32)GET_RG_RX_ADC_Q_RO_BIT -128; 

            if (ro_adc < 0) {
                ro_adc_abs = (u32) -ro_adc;
            }
            else {
                ro_adc_abs = (u32) ro_adc;
            }
            if(ro_adc_abs < ro_adc_abs_min) {
                ro_adc_abs_min = ro_adc_abs;
                idaca_dc_min   = idaca_dc_pre + loop_i    ;
            }
        }

        #ifdef __LOG_DRV_PHY_CALI_RXDC__
        LOG_PRINTF("# Q pgag = %d, idaca = %d, got rx-dc min = %d\n", loop_pgag, idaca_dc_min, ro_adc_abs_min);
        #endif
        (*rg_rxdc_set_q[loop_pgag])(idaca_dc_mid);
        rtbl_cali_rxdc.rtbl[loop_pgag + _CALI_RXDC_RTBL_IDX_Q_].result = idaca_dc_min;
    }

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    rtbl_cali_pre_load(&rtbl_cali_rxdc);
    rtbl_cali_rxdc.ever_cali = 1;

    // recovery register setting
    RG_TBL_LOAD(phyreg_tbl_recovery);

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    #ifdef _STATISTIC_DRV_PHY_CALI_RXDC_
    LOG_PRINTF("rxdc_rtbl(%d,:) = [", sidx++);
    for(loop_s=0; loop_s < 32; loop_s++) {
        LOG_PRINTF("%d ", rtbl_cali_rxdc.rtbl[loop_s].result);
    }
    LOG_PRINTF("];\n");
    #endif

    return 0;
}

s32 drv_phy_cali_tssi(const phy_cali_config *phy_cfg) {

    u32 loop_i    ;
    u32 saradc_rdy;

    s32 sar_bit_diff        ;
    u32 sar_bit_diff_abs    ;
    u32 sar_bit_diff_abs_min;
    u32 gain_trgt           ;   // gain target
    u32 tssi_orig           ;

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[20];
    u32 ctime_idx = 0;
    #endif

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // tssi
    tssi_orig = GET_RG_TX_TSSI_DIV;
    SET_RG_TX_TSSI_DIV(g_rf_config.cfg_tssi_div);

    SET_RG_EN_TX_TSSI (1);
    SET_RG_SARADC_TSSI(1);

    gain_trgt            = 0;
    sar_bit_diff_abs_min = _CALI_TSSI_SAR_DIF_MAX_;

    for(loop_i=0; loop_i < _CALI_TSSI_OBSR_N_; loop_i++) {

        // set baseband gain
        SET_RG_TX_I_SCALE(tssi_tbl_gain[loop_i][1]);
        SET_RG_TX_Q_SCALE(tssi_tbl_gain[loop_i][1]);

        SET_RG_EN_SARADC(1);
        saradc_rdy = WAIT_RDY(SAR_ADC_FSM_RDY);

        if(saradc_rdy == 0) {
            #ifdef __LOG_DRV_PHY_CALI_TSSI__
            LOG_PRINTF("DRV_PHY_CALI_TSSI fail: saradc circuit fail\n");
            #endif
            return -2;
        }
        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = GET_RG_SARADC_BIT;
        #endif

        sar_bit_diff = (s32)GET_RG_SARADC_BIT  - g_rf_config.cfg_tssi_trgt;
        SET_RG_EN_SARADC(0);

        #ifdef __LOG_DRV_PHY_CALI_TSSI__
        LOG_PRINTF("## gain %d got sar_bit diff %d\n", loop_i, sar_bit_diff);
        #endif

        if (sar_bit_diff < 0) {
            sar_bit_diff_abs = (u32)-sar_bit_diff;
        }
        else {
            sar_bit_diff_abs = (u32)sar_bit_diff;
        }

        if (sar_bit_diff_abs < sar_bit_diff_abs_min) {
            sar_bit_diff_abs_min = sar_bit_diff_abs;
            gain_trgt            = loop_i;
        }

    }
    #ifdef __LOG_DRV_PHY_CALI_TSSI__
    LOG_PRINTF("# gain %d got sar_bit diff abs min %d\n", gain_trgt, sar_bit_diff_abs_min);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // record result
    rtbl_cali_tssi.ever_cali = 1;
    rtbl_cali_tssi.rtbl[0].result = tssi_tbl_gain[gain_trgt][0];

    // load gain offset
    SET_RG_TX_GAIN_OFFSET(tssi_tbl_gain[gain_trgt][0]);

    SET_RG_TX_I_SCALE(_CALI_TSSI_BB_DEF_GAIN_);
    SET_RG_TX_Q_SCALE(_CALI_TSSI_BB_DEF_GAIN_);

    // recovery register setting
    SET_RG_TX_TSSI_DIV (tssi_orig             );
    SET_RG_EN_TX_TSSI  (0                     );
    SET_RG_SARADC_TSSI (0                     );

    RG_TBL_LOAD(phyreg_tbl_recovery);

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    return 0;
}

s32 drv_phy_cali_rxrc (const phy_cali_config *phy_cfg) {

    u32 loop_i;
    u32 loop_j;
    u32 ro_iqcal_o;

    u32 pwr_inband;
    u32 pwr_outband_exp;        // expected outband power
    u64 pwr_outband_exp_tmp;

    u32 abbctune_pre;
    u32 abbctune_mid;
    u32 abbctune_min;

    s32 loop_pwr_diff;
    s32 loop_pwr_diff_abs;
    s32 loop_pwr_diff_min;

    #ifdef _STATISTIC_DRV_PHY_CALI_RXRC_
    static u32 sidx=1;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 ctime[10];
    u32 ctime_idx = 0;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __DBG_DUMP__
    u32 *dbg_dumpp_inband  = sreg_ft_dbg;
    u32 *dbg_dumpp_outband = sreg_ft_dbg + 8;
    #endif

    // initial setup
    for(loop_i=0; loop_i < _CALI_RXRC_SETTLE_N_; loop_i++) {   // use this loop to get some settle time before sprm.
        SET_RG_BYPASS_ACI(1);
    }

    // # do in-band signal observation
    if (drv_phy_sprm(phy_cfg->cfg_freq_rx, phy_cfg->cfg_selq) != 0) {
        return -3;
    }
    pwr_inband = (u32)drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

    #ifdef __DBG_DUMP__
    *(dbg_dumpp_inband) = pwr_inband;
    #endif

    SET_RG_TONEGEN_EN(0);

    // # cali target out-band signal strength
    pwr_outband_exp_tmp = (u64) _CALI_RXRC_M21DBC_NUME_ * pwr_inband;
    pwr_outband_exp     = (u32)(pwr_outband_exp_tmp >> _CALI_RXRC_M21DBC_DENO_);

    #ifdef __LOG_DRV_PHY_CALI_RXRC__
    LOG_PRINTF("## got inband power = %d, expected outband power = %d\n", pwr_inband, pwr_outband_exp);
    #endif

    // # find abbctune with proper out-band signal strength
    SET_RG_TONEGEN_FREQ(_CALI_RXRC_TX_OFREQ_);
    SET_RG_TX_I_SCALE  (_CALI_RXRC_TX_OSCALE_);
    SET_RG_TX_Q_SCALE  (_CALI_RXRC_TX_OSCALE_);
    SET_RG_RX_I_SCALE  (_CALI_RXRC_RX_OSCALE_);
    SET_RG_RX_Q_SCALE  (_CALI_RXRC_RX_OSCALE_);
    SET_RG_TONEGEN_EN  (1                   );

    abbctune_pre = 0;
    for(loop_i = 0; loop_i < 6; loop_i++) {
        abbctune_mid = abbctune_pre + (1 << (5-loop_i));

        for(loop_j = 0; loop_j < _CALI_RXRC_SETTLE_N_; loop_j++) {  // use this loop to get settle time before sprm
            SET_RG_RX_ABBCTUNE(abbctune_mid);
        }
        if (drv_phy_sprm(_CALI_RXRC_RX_OFREQ_, phy_cfg->cfg_selq) != 0) {
            return -3;
        }
        ro_iqcal_o = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        #ifdef __LOG_DRV_PHY_CALI_RXRC__
        LOG_PRINTF("## abbctune = %d, outband power = %d\n", abbctune_mid, ro_iqcal_o);
        #endif

        #ifdef __DBG_DUMP__
        *(dbg_dumpp_outband++) = ro_iqcal_o;
        #endif

        if(ro_iqcal_o > pwr_outband_exp)
            abbctune_pre = abbctune_mid;
    }

    abbctune_min      = abbctune_pre;
    loop_pwr_diff_min = _RO_IQCAL_O_MAX_;
    for(loop_i = 0; loop_i < 2; loop_i++) {

        abbctune_mid = abbctune_pre + loop_i;

        for(loop_j = 0; loop_j < _CALI_RXRC_SETTLE_N_; loop_j++) {  // use this loop to get settle time before sprm
            SET_RG_RX_ABBCTUNE(abbctune_mid);
        }

        if (drv_phy_sprm(_CALI_RXRC_RX_OFREQ_, phy_cfg->cfg_selq) != 0) {
            return -3;
        }
        ro_iqcal_o = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        loop_pwr_diff     = ro_iqcal_o - pwr_outband_exp;
        loop_pwr_diff_abs = (loop_pwr_diff < 0) ? -loop_pwr_diff : loop_pwr_diff;

        if(loop_pwr_diff_abs < loop_pwr_diff_min) {
            abbctune_min      = abbctune_mid;
            loop_pwr_diff_min = loop_pwr_diff_abs;
        }

        #ifdef __LOG_DRV_PHY_CALI_RXRC__
        LOG_PRINTF("## abbctune = %d, outband power = %d, diff = %d\n", abbctune_mid, ro_iqcal_o, loop_pwr_diff_abs);
        #endif

        #ifdef __DBG_DUMP__
        *(dbg_dumpp_outband++) = ro_iqcal_o;
        #endif

    }

    /*
    for(loop_i = _CALI_RXRC_ABBCTUNE_MIN_; loop_i <= _CALI_RXRC_ABBCTUNE_MAX_; loop_i++) {
        SET_RG_RX_ABBCTUNE(loop_i);

        // ## do power observation
        if (drv_phy_sprm(phy_cfg->cfg_freq_rx, phy_cfg->cfg_selq) != 0) {
            return -3;
        }
        ro_iqcal_o = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        loop_pwr_diff     = ro_iqcal_o - pwr_outband_exp;
        loop_pwr_diff_abs = (loop_pwr_diff < 0) ? -loop_pwr_diff : loop_pwr_diff;

        if(loop_pwr_diff_abs < loop_pwr_diff_min) {
            abbctune_min = loop_i;
            loop_pwr_diff_min = loop_pwr_diff_abs;
        }

#ifdef __LOG_DRV_PHY_CALI_RXRC__
        LOG_PRINTF("## abbctune = %d, outband power = %d, diff = %d\n", loop_i, ro_iqcal_o, loop_pwr_diff_abs);
#endif
    } 
    */
    SET_RG_TONEGEN_EN(0);

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # store cali. result
    #ifdef __LOG_DRV_PHY_CALI_RXRC__
    LOG_PRINTF("### select abbctune = %d\n", abbctune_min);
    #endif
    SET_RG_RX_ABBCTUNE(abbctune_min);

    rtbl_cali_rxrc.ever_cali      = 1;
    rtbl_cali_rxrc.rtbl[0].result = abbctune_min;

    // recovery register setting
    RG_TBL_LOAD(phyreg_tbl_recovery);

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    #ifdef _STATISTIC_DRV_PHY_CALI_RXRC_
    LOG_PRINTF("rxrc_rtbl(%d) = %d;\n", 
            sidx++, rtbl_cali_rxrc.rtbl[0].result);
    #endif

    return 0;
}

ssv_cabrio_reg firmware_phy_setting[MAX_PHY_SETTING_TABLE_SIZE];
ssv_cabrio_reg firmware_rf_setting[MAX_RF_SETTING_TABLE_SIZE];
u32 firmware_phy_setting_len = 0;
u32 firmware_rf_setting_len = 0;

void drv_phy_reg_init (void) {

    /*
    u32 loop_i;
    */

    u32 cfg_xtal;

    // ## switch clock to XTAL
    /*
    SET_RG_RF_BB_CLK_SEL(0);
    */

    if(firmware_rf_setting_len)
        drv_phy_tbl_load(firmware_rf_setting, firmware_rf_setting_len);

    // ## by-xtal
    cfg_xtal = g_rf_config.cfg_xtal;
    rfreg_byxtal_load(rfreg_tbl_config_byxtal[cfg_xtal]);

    // ## by-pa
    SET_RG_PABIAS_CTRL(g_rf_config.cfg_pabias_ctrl);
    SET_RG_PACASCODE_CTRL(g_rf_config.cfg_pacascode_ctrl);

    // ## Phy register default
    if(firmware_phy_setting_len)
        drv_phy_tbl_load(firmware_phy_setting, firmware_phy_setting_len);

    // ## TX baseband gain
    SET_TX_SCALE_11B     (g_rf_config.cfg_def_tx_scale_11b     );
    SET_TX_SCALE_11B_P0D5(g_rf_config.cfg_def_tx_scale_11b_p0d5);
    SET_TX_SCALE_11G     (g_rf_config.cfg_def_tx_scale_11g     );
    SET_TX_SCALE_11G_P0D5(g_rf_config.cfg_def_tx_scale_11g_p0d5);
    //
    rtbl_cali_papd.rtbl[_CALI_PAPD_RTBL_IDX_CFG_SCALE_    ].def = g_rf_config.cfg_def_tx_scale_11b     ;
    rtbl_cali_papd.rtbl[_CALI_PAPD_RTBL_IDX_CFG_SCALE_ + 1].def = g_rf_config.cfg_def_tx_scale_11b_p0d5;
    rtbl_cali_papd.rtbl[_CALI_PAPD_RTBL_IDX_CFG_SCALE_ + 2].def = g_rf_config.cfg_def_tx_scale_11g     ;
    rtbl_cali_papd.rtbl[_CALI_PAPD_RTBL_IDX_CFG_SCALE_ + 3].def = g_rf_config.cfg_def_tx_scale_11g_p0d5;

    // ## switch clock to XTAL
    /*
    SET_RG_RF_BB_CLK_SEL(1);
    */
}

s32 drv_phy_temperature (const phy_cali_config *phy_cfg) {

    u32 saradc_rdy;

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    u32 loop_i;
    u32 ctime[20];
    u32 ctime_idx = 0;
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // ##
    SET_RG_SARADC_THERMAL(1);
    SET_RG_EN_SARADC     (1);

    saradc_rdy = WAIT_RDY(SAR_ADC_FSM_RDY);
    if(saradc_rdy == 0) {
        #ifdef __LOG_DRV_PHY_TEMPERATURE__
        LOG_PRINTF("DRV_PHY_TEMPERATURE fail: saradc circuit fail\n");
        #endif
        return -2;
    }

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    rtbl_temperature.valid = 1;
    rtbl_temperature.temperature = GET_RG_SARADC_BIT;

    // ## 
    SET_RG_EN_SARADC     (0);
    SET_RG_SARADC_THERMAL(0);

    #ifdef __LOG_DRV_PHY_TEMPERATURE__
    LOG_PRINTF("## Temperature reading: %d\n", rtbl_temperature.temperature);
    #endif

    #ifdef __LOG_DRV_PHY_MCU_TIME__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    return 0;

}

// # load calibration result
void drv_phy_cali_rtbl_load (u32 rtbl_sel) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_rtbl_sel;

    // # cali.
    for(loop_i = 0; loop_i < _CALI_SEQ_N_; loop_i++) {
        loop_rtbl_sel = rtbl_sel &  (1 << loop_i);

        for (loop_j = 0; loop_j < sizeof(cali_info_tbl)/sizeof(cali_info); loop_j++) {
            if (cali_info_tbl[loop_j].seq == loop_rtbl_sel) {
                if(cali_info_tbl[loop_j].rtbl != NULL) {
                    if(rtbl_cali_load(cali_info_tbl[loop_j].rtbl) == 0) {
                        #ifdef __LOG_DRV_PHY_INIT_CALI__
                        LOG_PRINTF("# %s # load result\n", cali_info_tbl[loop_j].name);
                        #endif
                    }
                }
            }
        }
    }
}

void drv_phy_cali_rtbl_load_def (u32 rtbl_sel) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_rtbl_sel;

    // # cali.
    for(loop_i = 0; loop_i < _CALI_SEQ_N_; loop_i++) {
        loop_rtbl_sel = rtbl_sel &  (1 << loop_i);

        for (loop_j = 0; loop_j < sizeof(cali_info_tbl)/sizeof(cali_info); loop_j++) {
            if (cali_info_tbl[loop_j].seq == loop_rtbl_sel) {
                if(cali_info_tbl[loop_j].rtbl != NULL) {
                    #ifdef __LOG_DRV_PHY_INIT_CALI__
                    LOG_PRINTF("# %s # load default\n", cali_info_tbl[loop_j].name);
                    #endif
                    rtbl_cali_load_def(cali_info_tbl[loop_j].rtbl);
                }
            }
        }
    }

}

// reset calibration flag, and load default value
void drv_phy_cali_rtbl_reset (u32 rtbl_sel) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_rtbl_sel;

    // # cali.
    for(loop_i = 0; loop_i < _CALI_SEQ_N_; loop_i++) {
        loop_rtbl_sel = rtbl_sel &  (1 << loop_i);

        for (loop_j = 0; loop_j < sizeof(cali_info_tbl)/sizeof(cali_info); loop_j++) {
            if (cali_info_tbl[loop_j].seq == loop_rtbl_sel) {
                if(cali_info_tbl[loop_j].rtbl != NULL) {
                    #ifdef __LOG_DRV_PHY_INIT_CALI__
                    LOG_PRINTF("# %s # reset\n", cali_info_tbl[loop_j].name);
                    #endif
                    rtbl_cali_reset(cali_info_tbl[loop_j].rtbl);
                }
            }
        }
    }

}

void drv_phy_cali_rtbl_set (u32 rtbl_sel) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_rtbl_sel;

    // # cali.
    for(loop_i = 0; loop_i < _CALI_SEQ_N_; loop_i++) {
        loop_rtbl_sel = rtbl_sel &  (1 << loop_i);

        for (loop_j = 0; loop_j < sizeof(cali_info_tbl)/sizeof(cali_info); loop_j++) {
            if (cali_info_tbl[loop_j].seq == loop_rtbl_sel) {
                if(cali_info_tbl[loop_j].rtbl != NULL) {
                    #ifdef __LOG_DRV_PHY_INIT_CALI__
                    LOG_PRINTF("# %s # set\n", cali_info_tbl[loop_j].name);
                    #endif
                    rtbl_cali_set(cali_info_tbl[loop_j].rtbl);
                }
            }
        }
    }

}

void drv_phy_cali_rtbl_export (u32 rtbl_sel) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_rtbl_sel;

    /* Use CRITICAL to make print-out could be put into another sw */
    #ifndef _NO_OS_
    taskENTER_CRITICAL();
    #endif

    // # cali.
    for(loop_i = 0; loop_i < _CALI_SEQ_N_; loop_i++) {
        loop_rtbl_sel = rtbl_sel &  (1 << loop_i);

        for (loop_j = 0; loop_j < sizeof(cali_info_tbl)/sizeof(cali_info); loop_j++) {
            if (cali_info_tbl[loop_j].seq == loop_rtbl_sel) {
                if(cali_info_tbl[loop_j].rtbl != NULL) {
                    LOG_PRINTF("# %s # export\n", cali_info_tbl[loop_j].name);
                    rtbl_cali_export(cali_info_tbl[loop_j].rtbl);
                }
            }
        }
    }

    #ifndef _NO_OS_
    taskEXIT_CRITICAL();
    #endif


}

s32 drv_phy_init_cali(u32 fx_sel) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_fx_sel;

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    u32 ctime[40];
    u32 ctime_idx = 0;
    #endif

    #ifndef _NO_OS_
    taskENTER_CRITICAL();
    #endif

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    if (GET_RG_RF_BB_CLK_SEL == 0) {
        #ifdef __LOG_DRV_PHY_INIT_CALI__
        LOG_PRINTF("init_cali fail: CLK of cali. Hw isn't correct\n");
        #endif
        return -1;
    }

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # reset cali result
    drv_phy_cali_rtbl_reset(fx_sel); 

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # load phy/rf default register
    drv_phy_reg_init();

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # latch recovery register default value
    for(loop_i=0; loop_i < sizeof(phyreg_tbl_recovery)/sizeof(ssv_cabrio_reg); loop_i++) {
        phyreg_tbl_recovery[loop_i].data = REG32(phyreg_tbl_recovery[loop_i].address);
    }

    // # cali.
    for(loop_i = 0; loop_i < _CALI_SEQ_N_; loop_i++) {
        loop_fx_sel = fx_sel &  (1 << loop_i);

        for (loop_j = 0; loop_j < sizeof(cali_info_tbl)/sizeof(cali_info); loop_j++) {

            if (cali_info_tbl[loop_j].seq == loop_fx_sel) {
                #ifdef __LOG_DRV_PHY_INIT_CALI__
                LOG_PRINTF("# %s # running\n", cali_info_tbl[loop_j].name);
                #endif

                #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
                ctime[ctime_idx++] = REG32(0xc0000204);
                #endif

                if(cali_info_tbl[loop_j].phy_cfg != NULL) {
                    phy_cali_config_load(cali_info_tbl[loop_j].phy_cfg);
                }

                #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
                ctime[ctime_idx++] = REG32(0xc0000204);
                #endif

                if ((*cali_info_tbl[loop_j].fx_cali)(cali_info_tbl[loop_j].phy_cfg) != 0) {
                    return -1;
                }

                #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
                ctime[ctime_idx++] = REG32(0xc0000204);
                #endif

            }

        }
    }

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # load phy/rf default register
    drv_phy_reg_init();

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # load cali. result
    drv_phy_cali_rtbl_load(_CALI_INIT_RTBL_SEL_);

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_INIT_CALI_TSTAMP__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    #ifndef _NO_OS_
    taskEXIT_CRITICAL();
    #endif

    return 0;
}

void drv_phy_evm_tk(u32 enable) {

    // 54M, MCS-7(normal GI), 11M-long preamble
    switch(enable) {
        case 3:   // 11M, long preamble
            REG32(0xCE00000C) = 0x03000400;	//default: 0x00000064
            break;
        case 6:   // 11M, short preamble
            REG32(0xCE00000C) = 0x03400400;	//default: 0x00000064
            break;
        case 14:  // 11G, 54M
            REG32(0xCE00000C) = 0x07010400;	//default: 0x00000064
            break;
        case 22:  // MCS-7, normal GI
            REG32(0xCE00000C) = 0x07020400;	//default: 0x00000064
            break;
        case 30:  // MCS-7, short GI
            REG32(0xCE00000C) = 0x07820400;	//default: 0x00000064
            break;
        default: 
            LOG_PRINTF("# pkt-gen disable\n");
            REG32(0xCE000018) = 0x0055003C; // PacketGen disable
            REG32(0xce000020) = 0x20000000; // tx path
            REG32(0xCE000004) = 0x00000000; // turn on PHY, pulse
            REG32(0xCE000004) = 0x0000017F; // 
            return;
    }

    LOG_PRINTF("# pkt-gen enable\n");
    REG32(0xCE000020) = 0x20000400; // tx path
    REG32(0xCE000004) = 0x00000000; // turn on PHY, pulse
    REG32(0xCE000004) = 0x0000017F; // 
    REG32(0xCE00001C) = 0xFFFFFFFF; // packet number
    REG32(0xCE000018) = 0x0055003E; // TX CNT reset pulse
    REG32(0xCE000018) = 0x0055003C;
    REG32(0xCE000018) = 0x0055083D; // Trigger packeten
}

void drv_phy_tone_tk(u32 tx_freq) {

    LOG_PRINTF("tone-gen enable with freq %d", tx_freq);

    SET_RG_TONEGEN_FREQ(tx_freq);
    SET_RG_TX_IQ_SRC   (2      );
    SET_RG_TONEGEN_EN  (1      );

}

#ifdef _BIST_RF_
s32 drv_phy_bist_rf_txgain (const phy_cali_config *phy_cfg)
{

    u32 loop_i;
    s32 fft_enrg;

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
    #endif

    rtbl_measure_txgain.valid = 0;

    for(loop_i=0; loop_i < (sizeof(tbl_bist_rf_txgain)/sizeof(u16)); loop_i++) {
        SET_RG_TX_GAIN(tbl_bist_rf_txgain[loop_i]);

        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_txgain.rtbl[loop_i] = fft_enrg;
        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif

        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## observed fft energy = %0d when txgain = %d\n", fft_enrg, tbl_bist_rf_txgain[loop_i]);
        #endif
    }
    rtbl_measure_txgain.valid = 1;

    return 0;
}

s32 drv_phy_bist_rf_pgag (const phy_cali_config *phy_cfg)
{

    u32 loop_i;
    s32 fft_enrg;

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
    #endif

    rtbl_measure_pgag.valid = 0;

    for(loop_i=0; loop_i < (sizeof(tbl_bist_rf_rxgain)/sizeof(u16)); loop_i++) {
        SET_RG_PGAG(tbl_bist_rf_rxgain[loop_i]);

        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_pgag.rtbl[loop_i] = fft_enrg;
        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif
        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## observed fft energy = %0d when rxgain = %d\n", fft_enrg, tbl_bist_rf_rxgain[loop_i]);
        #endif

    }
    rtbl_measure_pgag.valid = 1;
    
    return 0;
}

s32 drv_phy_bist_rf_rssi (const phy_cali_config *phy_cfg)
{

    volatile u32 rssi;

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
    #endif

    rtbl_measure_rssi.valid = 0;

    rssi = GET_RG_RSSIADC_RO_BIT;
    rssi = GET_RG_RSSIADC_RO_BIT;

    rtbl_measure_rssi.rtbl[0] = rssi;
    #ifdef __DBG_DUMP__
    *(dbg_dumpp++) = rssi;
    #endif

    #ifdef __LOG_DRV_PHY_BIST_RF__
    LOG_PRINTF("## got rssi reading: %d\n", rssi);
    #endif

    rtbl_measure_rssi.valid = 1;

    return 0;
}

s32 drv_phy_bist_rf_rfg_hg2mg (const phy_cali_config *phy_cfg)
{

    s32 fft_enrg;
    u32 rtbl_idx = 0;
    u32 loop_i;
    u32 loop_j;

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
    #endif

    rtbl_measure_rfg_hg2mg.valid = 0;

    for(loop_i=0; loop_i< 2; loop_i++) {

        for(loop_j=0; loop_j < _BIST_SETTLE_N_RFG_; loop_j++) {
            SET_RG_RFG(loop_i + 2);     // from MG to HG
        }

        // turn on tonegen
        SET_RG_TONEGEN_EN(1);

        // measure signal
        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_rfg_hg2mg.rtbl[rtbl_idx++] = fft_enrg;

        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif

        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## rfg %d observed signal fft energy = %0d\n", GET_RG_RFG, fft_enrg);
        #endif

        // turn off tonegen
        SET_RG_TONEGEN_EN(0);

        // measure noise
        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_rfg_hg2mg.rtbl[rtbl_idx++] = fft_enrg;

        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif

        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## rfg %d observed noise fft energy = %0d\n", GET_RG_RFG, fft_enrg);
        #endif
    }
    rtbl_measure_rfg_hg2mg.valid = 1;

    return 0;
}

s32 drv_phy_bist_rf_rfg_mg2lg (const phy_cali_config *phy_cfg)
{

    s32 fft_enrg;
    u32 rtbl_idx = 0;
    u32 loop_i;
    u32 loop_j;

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
    #endif

    rtbl_measure_rfg_mg2lg.valid = 0;

    for(loop_i=0; loop_i< 2; loop_i++) {

        for(loop_j=0; loop_j < _BIST_SETTLE_N_RFG_; loop_j++) {
            SET_RG_RFG(loop_i + 1);     // from LG to MG
        }

        // turn on tonegen
        SET_RG_TONEGEN_EN(1);

        // measure signal
        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_rfg_mg2lg.rtbl[rtbl_idx++] = fft_enrg;

        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif

        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## rfg %d observed signal fft energy = %0d\n", GET_RG_RFG, fft_enrg);
        #endif

        // turn off tonegen
        SET_RG_TONEGEN_EN(0);

        // measure noise
        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_rfg_mg2lg.rtbl[rtbl_idx++] = fft_enrg;

        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif

        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## rfg %d observed noise fft energy = %0d\n", GET_RG_RFG, fft_enrg);
        #endif
    }
    rtbl_measure_rfg_mg2lg.valid = 1;

    return 0;
}

s32 drv_phy_bist_rf_rfg_lg2ulg (const phy_cali_config *phy_cfg)
{

    s32 fft_enrg;
    u32 rtbl_idx = 0;
    u32 loop_i;
    u32 loop_j;

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
    #endif

    rtbl_measure_rfg_lg2ulg.valid = 0;

    for(loop_i=0; loop_i< 2; loop_i++) {

        for(loop_j=0; loop_j < _BIST_SETTLE_N_RFG_; loop_j++) {
            SET_RG_RFG(loop_i + 0);     // from ULG to LG
        }

        // turn on tonegen
        SET_RG_TONEGEN_EN(1);

        // measure signal
        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_rfg_lg2ulg.rtbl[rtbl_idx++] = fft_enrg;

        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif

        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## rfg %d observed signal fft energy = %0d\n", GET_RG_RFG, fft_enrg);
        #endif

        // turn off tonegen
        SET_RG_TONEGEN_EN(0);

        // measure noise
        if(drv_phy_fft_enrg(phy_cfg->cfg_freq_rx) != 0) {
            return -1;
        }
        fft_enrg = drv_phy_cali_ro_iqcal_o(GET_RO_IQCAL_O);

        rtbl_measure_rfg_lg2ulg.rtbl[rtbl_idx++] = fft_enrg;

        #ifdef __DBG_DUMP__
        *(dbg_dumpp++) = fft_enrg;
        #endif

        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("## rfg %d observed noise fft energy = %0d\n", GET_RG_RFG, fft_enrg);
        #endif
    }
    rtbl_measure_rfg_lg2ulg.valid = 1;

    return 0;
}

s32 drv_phy_bist_rf_txpsat (const phy_cali_config *phy_cfg)
{

    u32 saradc_rdy;
    u32 sar_bit;
    u32 tssi_orig;

    #ifdef __DBG_DUMP__
    u32* dbg_dumpp = sreg_ft_dbg;     // debug dump pointer
    #endif

    // config
    tssi_orig = GET_RG_TX_TSSI_DIV;

    SET_RG_TX_TSSI_DIV(7);
    SET_RG_EN_TX_TSSI (1);
    SET_RG_SARADC_TSSI(1);

    rtbl_measure_txgain.valid = 0;
    
    SET_RG_EN_SARADC(1);
    saradc_rdy = WAIT_RDY(SAR_ADC_FSM_RDY);

    if(saradc_rdy == 0) {
        return -1;
    }
    sar_bit = GET_RG_SARADC_BIT;
    SET_RG_EN_SARADC(0);

    #ifdef __DBG_DUMP__
    *(dbg_dumpp++) = sar_bit;
    #endif

    #ifdef __LOG_DRV_PHY_BIST_RF__
    LOG_PRINTF("## Got TSSI reading %d\n", sar_bit);
    #endif

    rtbl_measure_txpsat.rtbl[0] = sar_bit;
    rtbl_measure_txpsat.valid   = 1;

    // recovery 
    SET_RG_TX_TSSI_DIV(tssi_orig);
    SET_RG_EN_TX_TSSI (0);
    SET_RG_SARADC_TSSI(0);

    return 0;
}

s32 drv_phy_bist_rf (u32 fx_sel) {

    u32 loop_i;
    u32 loop_j;
    u32 loop_fx_sel;

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    u32 ctime[40];
    u32 ctime_idx = 0;
    #endif

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    SET_TU0_TM_INT_MASK  (1    );
    SET_TU0_TM_MODE      (0    );
    SET_TU0_TM_INIT_VALUE(65535);
    #endif

    if (GET_RG_RF_BB_CLK_SEL == 0) {
        #ifdef __LOG_DRV_PHY_BIST_RF__
        LOG_PRINTF("BIST-RF fail: CLK of cali. Hw isn't correct\n");
        #endif
        return -1;
    }

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # load phy/rf default register
    drv_phy_reg_init();

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # bist.
    for(loop_i = 0; loop_i < _CALI_SEQ_N_; loop_i++) {
        loop_fx_sel = fx_sel &  (1 << loop_i);

        for (loop_j = 0; loop_j < sizeof(bist_info_tbl)/sizeof(bist_info); loop_j++) {

            if (bist_info_tbl[loop_j].seq == loop_fx_sel) {
                #ifdef __LOG_DRV_PHY_BIST_RF__
                LOG_PRINTF("# %s # running\n", bist_info_tbl[loop_j].name);
                #endif

                #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
                ctime[ctime_idx++] = REG32(0xc0000204);
                #endif

                if(bist_info_tbl[loop_j].phy_cfg != NULL) {
                    phy_cali_config_load(bist_info_tbl[loop_j].phy_cfg);
                }

                #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
                ctime[ctime_idx++] = REG32(0xc0000204);
                #endif

                if ((*bist_info_tbl[loop_j].fx_bist)(bist_info_tbl[loop_j].phy_cfg) != 0) {
                    return -1;
                }

                #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
                ctime[ctime_idx++] = REG32(0xc0000204);
                #endif

            }

        }
    }

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # load phy/rf default register
    drv_phy_reg_init();

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    // # load cali. result
    drv_phy_cali_rtbl_load(_CALI_INIT_RTBL_SEL_);

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    ctime[ctime_idx++] = REG32(0xc0000204);
    #endif

    #ifdef __LOG_DRV_PHY_BIST_RF_TSTAMP__
    for(loop_i=0; loop_i < ctime_idx; loop_i++) {
        LOG_PRINTF(" time stamp %d = %d\n", loop_i, ctime[loop_i]);
    }
    #endif

    return 0;
}
#endif
