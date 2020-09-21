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

#ifndef __SSV6006_COMMON_H__
#define __SSV6006_COMMON_H__ 
#include <ssv6xxx_common.h>
#define FW_VERSION_REG ADR_TX_SEG
#define M_ENG_CPU 0x00
#define M_ENG_HWHCI 0x01
#define M_ENG_EMPTY 0x02
#define M_ENG_ENCRYPT 0x03
#define M_ENG_MACRX 0x04
#define M_ENG_MIC 0x05
#define M_ENG_TX_EDCA0 0x06
#define M_ENG_TX_EDCA1 0x07
#define M_ENG_TX_EDCA2 0x08
#define M_ENG_TX_EDCA3 0x09
#define M_ENG_TX_MNG 0x0A
#define M_ENG_ENCRYPT_SEC 0x0B
#define M_ENG_MIC_SEC 0x0C
#define M_ENG_RESERVED_1 0x0D
#define M_ENG_RESERVED_2 0x0E
#define M_ENG_TRASH_CAN 0x0F
#define M_ENG_MAX (M_ENG_TRASH_CAN+1)
#define M_CPU_HWENG 0x00
#define M_CPU_TXL34CS 0x01
#define M_CPU_RXL34CS 0x02
#define M_CPU_DEFRAG 0x03
#define M_CPU_EDCATX 0x04
#define M_CPU_RXDATA 0x05
#define M_CPU_RXMGMT 0x06
#define M_CPU_RXCTRL 0x07
#define M_CPU_FRAG 0x08
#define M_CPU_TXTPUT 0x09
#ifndef ID_TRAP_SW_TXTPUT
#define ID_TRAP_SW_TXTPUT 50
#endif
#define M0_TXREQ 0
#define M1_TXREQ 1
#define M2_TXREQ 2
#define M0_RXEVENT 3
#define M2_RXEVENT 4
#define HOST_CMD 5
#define HOST_EVENT 6
#define RATE_RPT 7
#define SSV_NUM_HW_STA 8
#endif
