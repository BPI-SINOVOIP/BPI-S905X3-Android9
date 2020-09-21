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

#ifndef _SSV_EFUSE_H_
#define _SSV_EFUSE_H_ 
#include "dev.h"
struct efuse_map {
    u8 offset;
    u8 byte_cnts;
    u16 value;
};
enum efuse_data_item {
   EFUSE_R_CALIBRATION_RESULT = 1,
   EFUSE_SAR_RESULT,
   EFUSE_MAC,
   EFUSE_CRYSTAL_FREQUENCY_OFFSET,
   EFUSE_TX_POWER_INDEX_1,
   EFUSE_TX_POWER_INDEX_2,
   EFUSE_CHIP_ID,
   NO_USE,
   EFUSE_VID,
   EFUSE_PID,
   EFUSE_MAC_NEW,
   EFUSE_RATE_TABLE_1,
   EFUSE_RATE_TABLE_2
};
#ifdef SSV_SUPPORT_HAL
#define SSV_READ_EFUSE(_sh,_table) HAL_READ_EFUSE(_sh, _table)
#define SSV_WRITE_EFUSE(_sh,_pbuf,_len) HAL_WRITE_EFUSE(_sh, _pbuf, _len)
#else
#define EFUSE_HWSET_MAX_SIZE (256-32)
#define EFUSE_MAX_SECTION_MAP (EFUSE_HWSET_MAX_SIZE>>5)
#define SSV_EFUSE_ID_READ_SWITCH 0xC2000128
#define SSV_EFUSE_ID_RAW_DATA_BASE 0xC200014C
#define SSV_EFUSE_READ_SWITCH 0xC200012C
#define SSV_EFUSE_RAW_DATA_BASE 0xC2000150
u8 read_efuse(struct ssv_hw *sh, u8 *pbuf);
void write_efuse(struct ssv_hw *sh, u8 *data, u8 data_length);
#define SSV_READ_EFUSE(_sh,_table) read_efuse(_sh, _table)
#define SSV_WRITE_EFUSE(_sh,_data,_len) write_efuse(_sh, _data, _len)
#endif
u16 parser_efuse(struct ssv_hw *sh, u8 *pbuf, u8 *mac_addr, u8 *new_mac_addr, struct efuse_map *efuse_tbl);
void efuse_read_all_map(struct ssv_hw *sh);
#endif
