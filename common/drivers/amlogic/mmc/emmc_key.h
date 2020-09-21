/*
 * drivers/amlogic/mmc/emmc_key.h
 *
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

#ifndef __EMMC_KEY_H__
#define __EMMC_KEY_H__

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>

#define EMMC_KEY_AREA_SIGNAL		"emmckeys"
#define EMMC_KEY_AREA_SIGNAL_LEN	16

#define EMMC_KEYAREA_SIZE		(128*1024)
#define EMMC_KEYAREA_COUNT		2

/* we store partition table in the previous 16KB space */
#define EMMCKEY_RESERVE_OFFSET          0x4000
#define EMMCKEY_AREA_PHY_SIZE           (EMMC_KEYAREA_COUNT * EMMC_KEYAREA_SIZE)

struct emmckey_valid_node_t {
	u64 phy_addr;
	u64 phy_size;
	struct emmckey_valid_node_t *next;
};

struct aml_emmckey_info_t {
	/* struct memory_card *card; */
	struct emmckey_valid_node_t *key_valid_node;
	u64    keyarea_phy_addr;
	u64    keyarea_phy_size;
	u64    lba_start;
	u64    lba_end;
	u32    blk_size;
	u32    blk_shift;
	u8     key_init;
	u8     key_valid;
	u8     key_part_count;
};

#define EMMCKEY_DATA_VALID_LEN		\
	(EMMC_KEYAREA_SIZE - EMMC_KEY_AREA_SIGNAL_LEN - 4 - 4 - 4)
struct emmckey_data_t {
	u8     keyarea_mark[EMMC_KEY_AREA_SIGNAL_LEN];
	u32	   keyarea_mark_checksum;
	u32    checksum;
	u32    reserve;
	u8     data[EMMCKEY_DATA_VALID_LEN];
};

int emmc_key_init(struct mmc_card *card);

int32_t emmc_key_read(uint8_t *buffer,
	uint32_t length, uint32_t *actual_length);
int32_t emmc_key_write(uint8_t *buffer,
	uint32_t length, uint32_t *actual_length);

#endif

