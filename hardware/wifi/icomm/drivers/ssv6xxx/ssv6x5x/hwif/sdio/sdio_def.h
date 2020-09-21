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

#ifndef _SDIO_DEF_H_
#define _SDIO_DEF_H_ 
#include <linux/scatterlist.h>
#define BASE_SDIO 0
#define SD_REG_BASE 0xc0000800
#define REG_DATA_IO_PORT_0 (BASE_SDIO + 0x00)
#define REG_DATA_IO_PORT_1 (BASE_SDIO + 0x01)
#define REG_DATA_IO_PORT_2 (BASE_SDIO + 0x02)
#define REG_INT_MASK (BASE_SDIO + 0x04)
#define REG_INT_STATUS (BASE_SDIO + 0x08)
#define REG_INT_TRIGGER (BASE_SDIO + 0x09)
#define REG_Fn1_STATUS (BASE_SDIO + 0x0c)
#define REG_SD_READY_FLAG (BASE_SDIO + 0x0f)
#define REG_CARD_PKT_LEN_0 (BASE_SDIO + 0x10)
#define REG_CARD_PKT_LEN_1 (BASE_SDIO + 0x11)
#define REG_CARD_FW_DL_STATUS (BASE_SDIO + 0x12)
#define REG_CARD_SELF_TEST (BASE_SDIO + 0x13)
#define REG_CARD_RCA_0 (BASE_SDIO + 0x20)
#define REG_CARD_RCA_1 (BASE_SDIO + 0x21)
#define REG_SDIO_FIFO_WR_THLD_0 (BASE_SDIO + 0x24)
#define REG_SDIO_FIFO_WR_THLD_1 (BASE_SDIO + 0x25)
#define REG_OUTPUT_TIMING_REG (BASE_SDIO + 0x55)
#define MCU_NOTIFY_HOST_CFG (BASE_SDIO + 0x56)
#define REG_SDIO_DAT3_DELAY (BASE_SDIO + 0x59)
#define REG_SDIO_DAT2_DELAY (BASE_SDIO + 0x5a)
#define REG_SDIO_DAT1_DELAY (BASE_SDIO + 0x5b)
#define REG_SDIO_DAT0_DELAY (BASE_SDIO + 0x5c)
#define REG_PMU_WAKEUP (BASE_SDIO + 0x67)
#define REG_REG_IO_PORT_0 (BASE_SDIO + 0x70)
#define REG_REG_IO_PORT_1 (BASE_SDIO + 0x71)
#define REG_REG_IO_PORT_2 (BASE_SDIO + 0x72)
#define REG_SDIO_TX_ALLOC_SIZE (BASE_SDIO + 0x98)
#define REG_SDIO_TX_ALLOC_SHIFT (BASE_SDIO + 0x99)
#define REG_SDIO_TX_ALLOC_STATE (BASE_SDIO + 0x9a)
#define REG_SDIO_TX_INFORM_0 (BASE_SDIO + 0x9c)
#define REG_SDIO_TX_INFORM_1 (BASE_SDIO + 0x9d)
#define REG_SDIO_TX_INFORM_2 (BASE_SDIO + 0x9e)
#if 0
#define SDIO_TX_ALLOC_SUCCESS 0x01
#define SDIO_TX_NO_ALLOC 0x02
#define SDIO_TX_DULPICATE_ALLOC 0x04
#define SDIO_TX_TX_DONE 0x08
#define SDIO_TX_AHB_HANG 0x10
#define SDIO_TX_MB_FULL 0x80
#define SDIO_HCI_IN_QUEUE_EMPTY 0x04
#define SDIO_EDCA0_SHIFT 4
#define SDIO_TX_ALLOC_SIZE_SHIFT 0x07
#define SDIO_TX_ALLOC_ENABLE 0x10
#endif
#define SDIO_DEF_BLOCK_SIZE 0x80
#if (SDIO_DEF_BLOCK_SIZE % 8)
#error Wrong SDIO_DEF_BLOCK_SIZE value!! Should be the multiple of 8 bytes!!!!!!!!!!!!!!!!!!!!!!
#endif
#define SDIO_DEF_OUTPUT_TIMING 0
#define SDIO_DEF_BLOCK_MODE_THRD 128
#if (SDIO_DEF_BLOCK_MODE_THRD % 8)
#error Wrong SDIO_DEF_BLOCK_MODE_THRD value!! Should be the multiple of 8 bytes!!!!!!!!!!!!!!!!!!!!!!
#endif
#define SDIO_DEF_FORCE_BLOCK_MODE 0
#define MAX_SCATTER_ENTRIES_PER_REQ 8
struct sdio_scatter_item {
 u8 *buf;
 int len;
};
struct sdio_scatter_req {
 u32 req;
 u32 len;
 int scat_entries;
 struct sdio_scatter_item scat_list[MAX_SCATTER_ENTRIES_PER_REQ];
 struct scatterlist sgentries[MAX_SCATTER_ENTRIES_PER_REQ];
};
#define SDIO_READ 0x00000001
#define SDIO_WRITE 0x00000002
#define CMD53_ARG_READ 0
#define CMD53_ARG_WRITE 1
#define CMD53_ARG_BLOCK_BASIS 1
#define CMD53_ARG_FIXED_ADDRESS 0
#define CMD53_ARG_INCR_ADDRESS  1


#if defined(CONFIG_FW_ALIGNMENT_CHECK)
#define SDIO_DMA_BUFFER_LEN			2048
#endif
#ifdef CONFIG_PM
#define SDIO_COMMAND_BUFFER_LEN 256
#endif
#define IO_REG_BURST_RD_PORT_REG 0x10080
#define IO_REG_BURST_WR_PORT_REG 0x10040
#define MAX_BURST_READ_REG_AMOUNT 2
#define MAX_BURST_WRITE_REG_AMOUNT 2
#define SDIO_INPUT_DELAY_MSK 0x04
#define SDIO_INPUT_DELAY_SFT 2
#define SDIO_INPUT_DELAY_LEVEL_MSK 0x03
#define SDIO_INPUT_DELAY_LEVEL_SFT 0
#define SDIO_OUTPUT_DELAY_MSK 0x40
#define SDIO_OUTPUT_DELAY_SFT 6
#define SDIO_OUTPUT_DELAY_LEVEL_MSK 0x30
#define SDIO_OUTPUT_DELAY_LEVEL_SFT 4
#define SDIO_DELAY_LEVEL_OFF 0
#define SDIO_DELAY_LEVEL_0 1
#define SDIO_DELAY_LEVEL_1 2
#define SDIO_DELAY_LEVEL_2 3
#define SDIO_DELAY_LEVEL_3 4
#define SDIO_READY_FLAG_BUSY 0x0
#define SDIO_READY_FLAG_IDLE 0x2
#define SDIO_READY_FLAG_BUSY_THRESHOLD 10000
#define SDIO_READY_FLAG_BUSY_DELAY 5
#define PLATFORM_DEF_DMA_ALIGN_SIZE 32
#define PLATFORM_DMA_ALIGNED __attribute__ ((aligned(PLATFORM_DEF_DMA_ALIGN_SIZE)))
#endif
