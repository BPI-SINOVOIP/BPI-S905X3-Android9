/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef _SCP_REGISTER_DEFINES_
#define _SCP_REGISTER_DEFINES_
#include "secure_apb.h"

/* CBUS Registers */
 /*TIMER*/
#define P_ISA_TIMER_MUX                              ISA_TIMER_MUX
#define P_ISA_TIMERA                                 ISA_TIMERA
#define P_ISA_TIMERB                                 ISA_TIMERB
#define P_ISA_TIMERC                                 ISA_TIMERC
#define P_ISA_TIMERD                                 ISA_TIMERD
#define P_ISA_TIMERE                                 ISA_TIMERE
#define P_ISA_TIMERE_HI                              ISA_TIMERE_HI
#define P_ISA_TIMER_MUX1                             ISA_TIMER_MUX1
#define P_ISA_TIMERF                                 ISA_TIMERF
#define P_ISA_TIMERG                                 ISA_TIMERG
#define P_ISA_TIMERH                                 ISA_TIMERH
#define P_ISA_TIMERI                                 ISA_TIMERI
/* 80K share SRAM base address*/
#define P_SHARE_SRAM_BASE	0xd9000000
/*Interrupt number list*/
#define IRQ_TIMERC_NUM      0
#define IRQ_TIMERA_NUM      1
#define IRQ_TIMERB_NUM      2
#define IRQ_TIMERD_NUM      3
#define IRQ_TIMERF_NUM      4
#define IRQ_TIMERG_NUM      5
#define IRQ_TIMERH_NUM      6
#define IRQ_TIMERI_NUM      7
#define IRQ_GPIO0_NUM       8
#define IRQ_GPIO1_NUM       9
#define IRQ_GPIO2_NUM       10
#define IRQ_GPIO3_NUM       11
#define IRQ_SAR_ADC_NUM     12
#define IRQ_ASIT_MBOX0_NUM  13
#define IRQ_ASIT_MBOX1_NUM  14
#define IRQ_ASIT_MBOX2_NUM  15
#define IRQ_MBOX3_NUM       16
//17 reserve
#define IRQ_ETH_GMAC_NUM    18
#define IRQ_ETH_PMT_NUM     19
#define IRQ_ETH_LIP_NUM     20
//21~31 reserve
#define IRQ_MBOX3_RECV_NUM  32
#define IRQ_MBOX4_RECV_NUM  33
#define IRQ_MBOX5_RECV_NUM  34
#define IRQ_MBOX0_SEND_NUM  35
#define IRQ_MBOX1_SEND_NUM  36
#define IRQ_MBOX2_SEND_NUM  37
//38 reserve
#define IRQ_AO_UART_NUM     39
#define IRQ_AO_I2C_S_NUM    40
#define IRQ_AO_I2C_M_NUM    41
#define IRQ_AO_IR_DEC_NUM   42
#define IRQ_AO_UART2_NUM    43
#define IRQ_AO_IR_BLST_NUM  44
#define IRQ_AO_CEC_NUM      45
#define IRQ_AO_GPIO0_NUM    46
#define IRQ_AO_GPIO1_NUM    47
//48 reserve
#define IRQ_JTGPWD_FST_NUM  49
#define IRQ_WATCHDOG_NUM    50
#define IRQ_AO_CECB_NUM     51
#define IRQ_AO_TIMERA_NUM   52
#endif				//_SCP_REGISTER_DEFINES_
