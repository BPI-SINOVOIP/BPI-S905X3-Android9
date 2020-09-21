#include <config.h>
#ifdef _NO_OS_
#include <ssv_lib.h>
#include <drv_phy.h>
#define LOG_PRINTF printf
#else
#include <log.h>
#endif
#include "drv_pmu.h"

// #define __PMU_VERIFY__
// #define __LOG_DRV_PMU_TIME__

#ifdef __LOG_DRV_PMU_TIME__
#define _TSTAMP_MAX_ 20
#endif

#define _PAD_CFG_N_  60   // number-of pad configuration registers

#define _CLK_SEL_MAC_XTAL_  0
#define _CLK_SEL_MAC_PLL_   3

#define _CLK_SEL_PHY_XTAL_  0
#define _CLK_SEL_PHY_PLL_   1

#define _RF_MODE_SHUTDOWN_  0
#define _RF_MODE_STANDBY_   1
#define _RF_MODE_TRX_EN_    2

#define _DLDO_LEVEL_SLEEP_  0
#define _DLDO_LEVEL_ON_     7

#define _T_DCDC_2_LD0_      50
#define _T_LDO_2_DCDC_      50

#define _T_SLEEP_MIN_       200 
#define _T_PLL_ON_          200
#define _T_RF_STABLE_       100

#define _PMU_WAKE_TRIG_TMR_ 1
#define _PMU_WAKE_TRIG_INT_ 2

const u32 tbl_pmu_pad_cfg[][2] = {
    // Cabrio-E config, PMU wake-up by pin-15
    {0xC0000300, 0x00000000},
    {0xC0000304, 0x00000000},
    {0xC0000308, 0x00000000},
    {0xC000030C, 0x00000000},
    {0xC0000310, 0x00000000},
    {0xC0000314, 0x00000000},
    {0xC0000318, 0x00000000},
    {0xC000031C, 0x00000000},
    {0xC0000320, 0x00000000},
    {0xC0000324, 0x00000000},
    {0xC0000328, 0x00000000},
    {0xC000032C, 0x00000000},
    {0xC0000330, 0x00000000},
    {0xC0000334, 0x00000000},
    {0xC0000338, 0x00000000},
    {0xC000033C, 0x0000100a},
    {0xC0000340, 0x00000000},
    {0xC0000344, 0x00000000},
    {0xC0000348, 0x00000000},
    {0xC000034C, 0x00000000},
    {0xC0000350, 0x00000000},
    {0xC0000354, 0x00000000},
    {0xC0000358, 0x00000000},
    {0xC000035C, 0x00000000},
    {0xC0000360, 0x00000000},
    {0xC0000364, 0x00000000},
    {0xC0000368, 0x00000000},
    {0xC000036C, 0x00000000},
    {0xC0000370, 0x00100000},
    {0xC0000374, 0x00100808},
    {0xC0000378, 0x00100008},
    {0xC000037C, 0x00100008},
    {0xC0000380, 0x00100008},
    {0xC0000384, 0x00100000},
    {0xC0000388, 0x00000000},
    {0xC000038C, 0x00000002},
    {0xC0000390, 0x00000000},
    {0xC0000394, 0x00000000},
    {0xC0000398, 0x00000000},
    {0xC000039C, 0x00000000},
    {0xC00003A0, 0x00000000},
    {0xC00003A4, 0x00000000},
    {0xC00003A8, 0x00000000},
    {0xC00003AC, 0x00000000},
    {0xC00003B0, 0x00000000},
    {0xC00003B4, 0x00000000},
    {0xC00003BC, 0x00000000},
    {0xC00003C4, 0x00000000},
    {0xC00003B8, 0x00000000},
    {0xC00003C0, 0x00000000},
    {0xC00003CC, 0x08000000},
    {0xC00003D0, 0x00000000},
    // CabrioD config
    /*
    {0xC0000300, 0x00000000},
    {0xC0000304, 0x00000000},
    {0xC0000308, 0x00000000},
    {0xC000030C, 0x00000000},
    {0xC0000310, 0x00000000},
    {0xC0000314, 0x00000000},
    {0xC0000318, 0x00000000},
    {0xC000031C, 0x00000000},
    {0xC0000320, 0x00000000},
    {0xC0000324, 0x00000000},
    {0xC0000328, 0x00000000},
    {0xC000032C, 0x00000000},
    {0xC0000330, 0x00000000},
    {0xC0000334, 0x00000000},
    {0xC0000338, 0x00000000},
    {0xC000033C, 0x00000000},
    {0xC0000340, 0x00000000},
    {0xC0000344, 0x00000000},
    {0xC0000348, 0x00000000},
    {0xC000034C, 0x00000000},
    {0xC0000350, 0x00000000},
    {0xC0000354, 0x00000000},
    {0xC0000358, 0x00000000},
    {0xC000035C, 0x00000000},
    {0xC0000360, 0x00000000},
    {0xC0000364, 0x00000000},
    {0xC0000368, 0x00000000},
    {0xC000036C, 0x00000000},
    // {0xC0000370, 0x00000002},
    // {0xC0000374, 0x00000000},
    // {0xC0000378, 0x00000000},
    // {0xC000037C, 0x00000000},
    // {0xC0000380, 0x00000000},
    // {0xC0000384, 0x00000002},
    {0xC0000388, 0x00000002},
    {0xC000038C, 0x00000002},
    {0xC0000390, 0x00000000},
    {0xC0000394, 0x00000000},
    {0xC0000398, 0x00000000},
    {0xC000039C, 0x00000000},
    {0xC00003A0, 0x00000000},
    {0xC00003A4, 0x00000000},
    {0xC00003A8, 0x00000000},
    {0xC00003AC, 0x00000000},
    {0xC00003B0, 0x00000000},
    {0xC00003B4, 0x00000000},
    {0xC00003B8, 0x00000000},
    {0xC00003C4, 0x00000000},
    {0xC00003BC, 0x00000000},
    {0xC00003C0, 0x00000000}
    */
};

s32 drv_pmu_setwake_cnt (u32 t_10us)
{

    u32 wake_cnt;
    u64 tmp_long;

    // fprintf('%10d\n', round(32.768*10^3*10^-5*2^20))
    tmp_long = (u64)t_10us * 343597;
    tmp_long = tmp_long + (1 << (20-1));
    tmp_long = tmp_long >> 20;

    wake_cnt = (u32)tmp_long;

    if (wake_cnt >= 0x01000000) {
        LOG_PRINTF("Error: PMU wake-cnt overflow\n");
        return -1;
    }
    else {
        SET_SLEEP_WAKE_CNT(wake_cnt);
        return 0;
    }
}

void drv_pmu_tu0 (u32 t_us) {
    volatile u32 tu0_rdy = 0;

    if(t_us == 0)
        return;

    // set timer
    SET_TU0_TM_INIT_VALUE(t_us);

    // wait ready
    while(tu0_rdy == 0) {
        tu0_rdy = GET_TU0_TM_INT_STS_DONE;
    }

    // clear timer
    SET_TU0_TM_INT_STS_DONE(1);
}

void drv_pmu_tm0 (u32 t_ms) {
    volatile u32 tm0_rdy = 0;

    if(t_ms == 0)
        return;

    // set timer
    SET_TM0_TM_INIT_VALUE(t_ms);

    // wait ready
    while(tm0_rdy == 0) {
        tm0_rdy = GET_TM0_TM_INT_STS_DONE;
    }

    // clear timer
    SET_TM0_TM_INT_STS_DONE(1);
}

s32 drv_pmu_sleep (void) 
{
    u32 pad_cfg[_PAD_CFG_N_];
    u32 rf_mod_cfg;
    u32 pmu_wake_trig_event = 0;
    u32 reg_64m_switch;
    u32 loop_i;
    u32 use_dcdc = GET_RG_DCDC_MODE;

    #ifdef __LOG_DRV_PMU_TIME__
    u32 mcutime[_TSTAMP_MAX_];
    u32 mcutime_idx = 0;
    u32 rtctime[_TSTAMP_MAX_];
    u32 rtctime_idx = 0;
    #endif

    #ifdef __PMU_VERIFY__
    LOG_PRINTF("~");
    #endif

    #ifdef __LOG_DRV_PMU_TIME__
    // # start MCU timer, ms timer1
    SET_TU1_TM_INIT_VALUE(65535);
    #endif

    // # On -> Sleep
    // ## condition check
    // ### check if MASK set properly
    if(GET_DIGI_TOP_POR_MASK == 0) {
        #ifdef __PMU_VERIFY__
        LOG_PRINTF("Error: DIGI_TOP_POR_MASK not set\n");
        #endif
        return -1;
    }
    if(GET_PMU_ENTER_SLEEP_MODE == 1) {
        #ifdef __PMU_VERIFY__
        LOG_PRINTF("Error: PMU_ENTER_SLEEP_MODE already set\n");
        #endif
        return -2;
    }
    // ## time stamp
    #ifdef __LOG_DRV_PMU_TIME__
    mcutime[mcutime_idx++] = REG32(ADR_TU1_CURRENT_MICROSECOND_TIME_VALUE);
    rtctime[rtctime_idx++] = GET_RTC_SEC_CNT;
    #endif
    // ## latch rf-mode configuration
    rf_mod_cfg = GET_RG_MODE;
    // ## latch pad configuration, then set pads to tri-state
    for(loop_i=0; loop_i < ((sizeof(tbl_pmu_pad_cfg))/(sizeof(tbl_pmu_pad_cfg[0]))); loop_i++) {
        pad_cfg[loop_i]                   = REG32(tbl_pmu_pad_cfg[loop_i][0]);
        REG32(tbl_pmu_pad_cfg[loop_i][0]) = tbl_pmu_pad_cfg[loop_i][1];
    }
    #ifdef __LOG_DRV_PMU_TIME__
    mcutime[mcutime_idx++] = REG32(ADR_TU1_CURRENT_MICROSECOND_TIME_VALUE);
    rtctime[rtctime_idx++] = GET_RTC_SEC_CNT;
    #endif
    // ## should turn on INT or SDIO pads
    // ## clock switch
    SET_CK_SEL_1_0      (_CLK_SEL_MAC_XTAL_);
    SET_RG_RF_BB_CLK_SEL(_CLK_SEL_PHY_XTAL_);
    // ## RF configuration
    SET_RG_DP_BBPLL_PD(1);
    SET_RG_MODE       (_RF_MODE_SHUTDOWN_);
    SET_RG_DLDO_LEVEL (_DLDO_LEVEL_SLEEP_);
    // ## digital power handover
    if (use_dcdc)
    {
    SET_RG_DCDC_MODE(0);
    drv_pmu_tu0(_T_DCDC_2_LD0_);
    }
    #ifdef __LOG_DRV_PMU_TIME__
    mcutime[mcutime_idx++] = REG32(ADR_TU1_CURRENT_MICROSECOND_TIME_VALUE);
    rtctime[rtctime_idx++] = GET_RTC_SEC_CNT;
    #endif

    // ## Prevent power loss caused by voltage drop -> recharge -> drop -> recharge ...
    // REG32(0xce010004) = 0x00000000;
    // need double check by WM, then release
    SET_RG_EN_LDO_RX_FE (0);
    SET_RG_EN_LDO_ABB   (0);
    SET_RG_EN_LDO_AFE   (0);
    SET_RG_EN_SX_CHPLDO (0);
    SET_RG_EN_SX_LOBFLDO(0);
    SET_RG_EN_IREF_RX   (0);
    // ## sleep
    SET_PMU_ENTER_SLEEP_MODE(1);

    // # Sleep, in Cabrio-D, there is nothing to Polling if wake.
    // ## Is it necessary to wait a minimum time before wakeup ?
    drv_pmu_tu0(_T_SLEEP_MIN_);
    while(pmu_wake_trig_event == 0) {
        pmu_wake_trig_event = GET_PMU_WAKE_TRIG_EVENT;
    }
    #ifdef __LOG_DRV_PMU_TIME__
    mcutime[mcutime_idx++] = REG32(ADR_TU1_CURRENT_MICROSECOND_TIME_VALUE);
    rtctime[rtctime_idx++] = GET_RTC_SEC_CNT;
    #endif
    // # Sleep -> On
    // REG32(0xce010004) = 0x00000fc0;
    SET_RG_EN_LDO_RX_FE (1);
    SET_RG_EN_LDO_ABB   (1);
    SET_RG_EN_LDO_AFE   (1);
    SET_RG_EN_SX_CHPLDO (1);
    SET_RG_EN_SX_LOBFLDO(1);
    SET_RG_EN_IREF_RX   (1);
    // ## turn-off PMU
    SET_PMU_ENTER_SLEEP_MODE(0);
    // ## digital power handover
    if (use_dcdc)
    {
        SET_RG_DCDC_MODE(1);
        drv_pmu_tu0(_T_LDO_2_DCDC_);
    }
    // ## RF configuration
    SET_RG_DLDO_LEVEL (_DLDO_LEVEL_ON_);
    SET_RG_MODE       (_RF_MODE_STANDBY_);
    SET_RG_DP_BBPLL_PD(0);
    // ## wait PLL stable
    drv_pmu_tu0(_T_PLL_ON_);
    // Enable 64M before switching to PLL
    reg_64m_switch = REG32(0xce01008c);
    reg_64m_switch |= BIT(30);
    // ## clock switch
    SET_CK_SEL_1_0      (_CLK_SEL_MAC_PLL_);
    SET_RG_RF_BB_CLK_SEL(_CLK_SEL_PHY_PLL_);
    // Set back Enable 64M before switching to PLL
    REG32(0xce01008c) = reg_64m_switch;
    // ## re-toggle LCK-BIN
    SET_RG_EN_SX_LCK_BIN(0);
    SET_RG_EN_SX_LCK_BIN(1);
    drv_pmu_tu0(_T_RF_STABLE_);
    #ifdef __LOG_DRV_PMU_TIME__
    mcutime[mcutime_idx++] = REG32(ADR_TU1_CURRENT_MICROSECOND_TIME_VALUE);
    rtctime[rtctime_idx++] = GET_RTC_SEC_CNT;
    #endif
    // ## recover pad configuration
    for(loop_i=0; loop_i < ((sizeof(tbl_pmu_pad_cfg))/(sizeof(tbl_pmu_pad_cfg[0]))); loop_i++) {
        REG32(tbl_pmu_pad_cfg[loop_i][0]) = pad_cfg[loop_i];
    }
    #ifdef __LOG_DRV_PMU_TIME__
    mcutime[mcutime_idx++] = REG32(ADR_TU1_CURRENT_MICROSECOND_TIME_VALUE);
    rtctime[rtctime_idx++] = GET_RTC_SEC_CNT;
    #endif
    // ## finally, turn on RF
    SET_RG_MODE      (rf_mod_cfg);
    // ## time stamp
    #ifdef __LOG_DRV_PMU_TIME__
    mcutime[mcutime_idx++] = REG32(ADR_TU1_CURRENT_MICROSECOND_TIME_VALUE);
    rtctime[rtctime_idx++] = GET_RTC_SEC_CNT;
    #endif
    // ## report wakeup by which event

    #ifdef __PMU_VERIFY__
    LOG_PRINTF("~~~~\n");
    if (pmu_wake_trig_event == _PMU_WAKE_TRIG_TMR_) {
        LOG_PRINTF("## PMU wakeup by TMR event\n");
    }
    else if (pmu_wake_trig_event == _PMU_WAKE_TRIG_INT_) {
        LOG_PRINTF("## PMU wakeup by INT event\n");
    }
    else {
        LOG_PRINTF("## PMU wakeup by ?? event\n");
    }
    #endif


    // # report time stamps
    #ifdef __LOG_DRV_PMU_TIME__
    LOG_PRINTF("# clear output \n");
    LOG_PRINTF("# %2d tstamp: mcu %4d, rtc %4d\n", 0, 0, rtctime[0]);

    for(loop_i=1; loop_i<mcutime_idx; loop_i++) {
        LOG_PRINTF("# %2d tstamp: mcu %4d, rtc %4d\n", loop_i, (mcutime[0]-mcutime[loop_i]), rtctime[loop_i]);
    }
    #endif

    /*
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    asm volatile ( "NOP" );
    */

    return 0;
}

void drv_pmu_init (void)
{
    SET_DIGI_TOP_POR_MASK(1);

    // config from WM to optimize leakage
    REG32(0xc0001d0c) = 0x555f0010;
    REG32(0xce010090) = 0xaaaaaaae;
    REG32(0xce01008c) = 0xaaaaaaaf;

    /*
    if(config & _PMU_CFG_INT_EN_SDIO_CLK_) {
        #ifdef __PMU_VERIFY__
        LOG_PRINTF("## enable interupt from SDIO clock\n");
        #endif
        SET_WAKE_SOON_WITH_SCK(1);
    }
    else {
        #ifdef __PMU_VERIFY__
        LOG_PRINTF("## disable interupt from SDIO clock\n");
        #endif
        SET_WAKE_SOON_WITH_SCK(0);
    }
    */
}

void drv_pmu_chk (void)
{
    SET_DIGI_TOP_POR_MASK(0);
}

bool drv_pmu_check_interupt_event(void)
{
    u32 pmu_event = GET_PMU_WAKE_TRIG_EVENT;

    if(pmu_event == WAKEUP_EVENT_INTERRUPT)
        return true;
    else
        return false;
}
