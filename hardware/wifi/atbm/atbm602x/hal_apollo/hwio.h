/*
 * Low-level API for mac80211 altobeam apollo wifi drivers
 *
 * Copyright (c) 2016, altobeam
 *
 * Based on:
 * Copyright (c) 2010, stericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * Based on:
 * ST-Ericsson UMAC CW1200 driver which is
 * Copyright (c) 2010, ST-Ericsson
 * Author: Ajitpal Singh <ajitpal.singh@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ATBM_APOLLO_HWIO_H_INCLUDED
#define ATBM_APOLLO_HWIO_H_INCLUDED


/* extern */ struct atbm_common;
/* Hardware Type Definitions */
#define HIF_1601_FPGA		(0)
#define HIF_1601_CHIP		(1)




/* Download error code */
#define DOWNLOAD_PENDING		(0xFFFFFFFF)
#define DOWNLOAD_SUCCESS		(0)
#define DOWNLOAD_EXCEPTION		(1)
#define DOWNLOAD_ERR_MEM_1		(2)
#define DOWNLOAD_ERR_MEM_2		(3)
#define DOWNLOAD_ERR_SOFTWARE		(4)
#define DOWNLOAD_ERR_FILE_SIZE		(5)
#define DOWNLOAD_ERR_CHECKSUM		(6)
#define DOWNLOAD_ERR_OVERFLOW		(7)
#define DOWNLOAD_ERR_IMAGE		(8)
#define DOWNLOAD_ERR_HOST		(9)
#define DOWNLOAD_ERR_ABORT		(10)


#ifdef USB_BUS
#include "hwio_usb.h"
#endif
#ifdef SDIO_BUS
#include "hwio_sdio.h"
#endif 
#ifdef SPI_BUS
#include "hwio_spi.h"
#endif 

int atbm_fw_write(struct atbm_common *priv, u32 addr, const void *buf,u32 buf_len);
int atbm_before_load_firmware(struct atbm_common *hw_priv);
int atbm_after_load_firmware(struct atbm_common *hw_priv);
void atbm_firmware_init_check(struct atbm_common *hw_priv);
int atbm_reset_lmc_cpu(struct atbm_common *hw_priv);
int atbm_reset_chip(struct atbm_common *hw_priv);
#endif //ATBM_APOLLO_HWIO_H_INCLUDED
