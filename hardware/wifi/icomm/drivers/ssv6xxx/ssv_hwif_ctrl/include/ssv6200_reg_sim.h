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

// -----------------------------------------------------------------------------
// 2014-03-19 17:55:50
// -----------------------------------------------------------------------------
//  : match the SVN field in reg IC_TIME_TAG field
// -----------------------------------------------------------------------------
#include "ssv6200_reg.h"

#define BANK_COUNT 49
static const u32 BASE_BANK_SSV6200[] = {
 SYS_REG_BASE           , // 0 
 WBOOT_REG_BASE         , // 1 
 TU0_US_REG_BASE        , // 2 
 TU1_US_REG_BASE        , // 3 
 TU2_US_REG_BASE        , // 4 
 TU3_US_REG_BASE        , // 5 
 TM0_MS_REG_BASE        , // 6 
 TM1_MS_REG_BASE        , // 7 
 TM2_MS_REG_BASE        , // 8 
 TM3_MS_REG_BASE        , // 9 
 MCU_WDT_REG_BASE       , // 10
 SYS_WDT_REG_BASE       , // 11
 GPIO_REG_BASE          , // 12
 SD_REG_BASE            , // 13
 SPI_REG_BASE           , // 14
 CSR_I2C_MST_BASE       , // 15
 UART_REG_BASE          , // 16
 DAT_UART_REG_BASE      , // 17
 INT_REG_BASE           , // 18
 DBG_SPI_REG_BASE       , // 19
 FLASH_SPI_REG_BASE     , // 20
 DMA_REG_BASE           , // 21
 CSR_PMU_BASE           , // 22
 CSR_RTC_BASE           , // 23
 RTC_RAM_BASE           , // 24
 D2_DMA_REG_BASE        , // 25
 HCI_REG_BASE           , // 26
 CO_REG_BASE            , // 27
 EFS_REG_BASE           , // 28
 SMS4_REG_BASE          , // 29
 MRX_REG_BASE           , // 30
 AMPDU_REG_BASE         , // 31
 MT_REG_CSR_BASE        , // 32
 TXQ0_MT_Q_REG_CSR_BASE , // 33
 TXQ1_MT_Q_REG_CSR_BASE , // 34
 TXQ2_MT_Q_REG_CSR_BASE , // 35
 TXQ3_MT_Q_REG_CSR_BASE , // 36
 TXQ4_MT_Q_REG_CSR_BASE , // 37
 HIF_INFO_BASE          , // 38
 PHY_RATE_INFO_BASE     , // 39
 MAC_GLB_SET_BASE       , // 40
 BTCX_REG_BASE          , // 41
 MIB_REG_BASE           , // 42
 CBR_A_REG_BASE         , // 43
 MB_REG_BASE            , // 44
 ID_MNG_REG_BASE        , // 45
 CSR_PHY_BASE           , // 46
 CSR_RF_BASE            , // 47
 MMU_REG_BASE           , // 48
 0x00000000
};
static const char* STR_BANK_SSV6200[] = {
 "SYS_REG"           , // 0 
 "WBOOT_REG"         , // 1 
 "TU0_US_REG"        , // 2 
 "TU1_US_REG"        , // 3 
 "TU2_US_REG"        , // 4 
 "TU3_US_REG"        , // 5 
 "TM0_MS_REG"        , // 6 
 "TM1_MS_REG"        , // 7 
 "TM2_MS_REG"        , // 8 
 "TM3_MS_REG"        , // 9 
 "MCU_WDT_REG"       , // 10
 "SYS_WDT_REG"       , // 11
 "GPIO_REG"          , // 12
 "SD_REG"            , // 13
 "SPI_REG"           , // 14
 "CSR_I2C_MST"       , // 15
 "UART_REG"          , // 16
 "DAT_UART_REG"      , // 17
 "INT_REG"           , // 18
 "DBG_SPI_REG"       , // 19
 "FLASH_SPI_REG"     , // 20
 "DMA_REG"           , // 21
 "CSR_PMU"           , // 22
 "CSR_RTC"           , // 23
 "RTC_RAM"           , // 24
 "D2_DMA_REG"        , // 25
 "HCI_REG"           , // 26
 "CO_REG"            , // 27
 "EFS_REG"           , // 28
 "SMS4_REG"          , // 29
 "MRX_REG"           , // 30
 "AMPDU_REG"         , // 31
 "MT_REG_CSR"        , // 32
 "TXQ0_MT_Q_REG_CSR" , // 33
 "TXQ1_MT_Q_REG_CSR" , // 34
 "TXQ2_MT_Q_REG_CSR" , // 35
 "TXQ3_MT_Q_REG_CSR" , // 36
 "TXQ4_MT_Q_REG_CSR" , // 37
 "HIF_INFO"          , // 38
 "PHY_RATE_INFO"     , // 39
 "MAC_GLB_SET"       , // 40
 "BTCX_REG"          , // 41
 "MIB_REG"           , // 42
 "CBR_A_REG"         , // 43
 "MB_REG"            , // 44
 "ID_MNG_REG"        , // 45
 "CSR_PHY"           , // 46
 "CSR_RF"            , // 47
 "MMU_REG"           , // 48
 ""
};
static const u32 SIZE_BANK_SSV6200[] = {
 SYS_REG_BANK_SIZE           , // 0 
 WBOOT_REG_BANK_SIZE         , // 1 
 TU0_US_REG_BANK_SIZE        , // 2 
 TU1_US_REG_BANK_SIZE        , // 3 
 TU2_US_REG_BANK_SIZE        , // 4 
 TU3_US_REG_BANK_SIZE        , // 5 
 TM0_MS_REG_BANK_SIZE        , // 6 
 TM1_MS_REG_BANK_SIZE        , // 7 
 TM2_MS_REG_BANK_SIZE        , // 8 
 TM3_MS_REG_BANK_SIZE        , // 9 
 MCU_WDT_REG_BANK_SIZE       , // 10
 SYS_WDT_REG_BANK_SIZE       , // 11
 GPIO_REG_BANK_SIZE          , // 12
 SD_REG_BANK_SIZE            , // 13
 SPI_REG_BANK_SIZE           , // 14
 CSR_I2C_MST_BANK_SIZE       , // 15
 UART_REG_BANK_SIZE          , // 16
 DAT_UART_REG_BANK_SIZE      , // 17
 INT_REG_BANK_SIZE           , // 18
 DBG_SPI_REG_BANK_SIZE       , // 19
 FLASH_SPI_REG_BANK_SIZE     , // 20
 DMA_REG_BANK_SIZE           , // 21
 CSR_PMU_BANK_SIZE           , // 22
 CSR_RTC_BANK_SIZE           , // 23
 RTC_RAM_BANK_SIZE           , // 24
 D2_DMA_REG_BANK_SIZE        , // 25
 HCI_REG_BANK_SIZE           , // 26
 CO_REG_BANK_SIZE            , // 27
 EFS_REG_BANK_SIZE           , // 28
 SMS4_REG_BANK_SIZE          , // 29
 MRX_REG_BANK_SIZE           , // 30
 AMPDU_REG_BANK_SIZE         , // 31
 MT_REG_CSR_BANK_SIZE        , // 32
 TXQ0_MT_Q_REG_CSR_BANK_SIZE , // 33
 TXQ1_MT_Q_REG_CSR_BANK_SIZE , // 34
 TXQ2_MT_Q_REG_CSR_BANK_SIZE , // 35
 TXQ3_MT_Q_REG_CSR_BANK_SIZE , // 36
 TXQ4_MT_Q_REG_CSR_BANK_SIZE , // 37
 HIF_INFO_BANK_SIZE          , // 38
 PHY_RATE_INFO_BANK_SIZE     , // 39
 MAC_GLB_SET_BANK_SIZE       , // 40
 BTCX_REG_BANK_SIZE          , // 41
 MIB_REG_BANK_SIZE           , // 42
 CBR_A_REG_BANK_SIZE         , // 43
 MB_REG_BANK_SIZE            , // 44
 ID_MNG_REG_BANK_SIZE        , // 45
 CSR_PHY_BANK_SIZE           , // 46
 CSR_RF_BANK_SIZE            , // 47
 MMU_REG_BANK_SIZE           , // 48
 0x00000000
};
