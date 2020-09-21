/* Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_FLASH_LAYOUT_H
#define __CROS_EC_FLASH_LAYOUT_H

/*
 * The flash memory is implemented in two halves. The SoC bootrom will look for
 * a first-stage bootloader (aka "RO firmware") at the beginning of each of the
 * two halves and prefer the newer one if both are valid. The chosen bootloader
 * also looks in each half of the flash for a valid application image (("RW
 * firmware"), so we have two possible RW images as well. The RO and RW images
 * are not tightly coupled, so either RO image can choose to boot either RW
 * image. RO images are provided by the SoC team, and can be updated separately
 * from the RW images.
 */

/* Flash is directly addressable */
#define CHIP_FLASH_BASE              0x40000
#define CHIP_FLASH_SIZE              (512 * 1024)
#define CHIP_FLASH_HALF              (CHIP_FLASH_SIZE >> 1)

/* Each half has to leave room for the image's signed header */
#define CHIP_SIG_HEADER_SIZE	     1024

/* This isn't optional, since the bootrom will always look for both */
#define CHIP_HAS_RO_B

/* The RO images start at the very beginning of each flash half */
#define CHIP_RO_A_MEM_OFF 0
#define CHIP_RO_B_MEM_OFF CHIP_FLASH_HALF

/* Size reserved for each RO image */
#define CHIP_RO_SIZE 0x4000

/*
 * RW images start right after the reserved-for-RO areas in each half, but only
 * because that's where the RO images look for them. It's not a HW constraint.
 */
#define CHIP_RW_A_MEM_OFF CHIP_RO_SIZE
#define CHIP_RW_B_MEM_OFF (CHIP_FLASH_HALF + CHIP_RW_A_MEM_OFF)

/*
 * Any reserved flash storage is placed after the RW image. It makes A/B
 * updates MUCH simpler if both RW images are the same size, so we reserve the
 * same amount in each half.
 */
#define CHIP_RW_SIZE							\
	(CHIP_FLASH_HALF - CHIP_RW_A_MEM_OFF - CONFIG_FLASH_TOP_SIZE)

/* Reserved flash offset starts here. */
#define CHIP_FLASH_TOP_A_OFF (CHIP_FLASH_HALF - CONFIG_FLASH_TOP_SIZE)
#define CHIP_FLASH_TOP_B_OFF (CHIP_FLASH_SIZE - CONFIG_FLASH_TOP_SIZE)


/* Internal flash specifics */
#define CHIP_FLASH_BANK_SIZE         0x800	/* protect bank size */
#define CHIP_FLASH_ERASE_SIZE        0x800	/* erase bank size */

/* This flash can only be written as 4-byte words (aligned properly, too). */
#define CHIP_FLASH_ERASED_VALUE32    0xffffffff
#define CHIP_FLASH_WRITE_SIZE        4	/* min write size (bytes) */

/* But we have a 32-word buffer for writing multiple adjacent cells */
#define CHIP_FLASH_WRITE_IDEAL_SIZE  128	/* best write size (bytes) */

/* The flash controller prevents bulk writes that cross row boundaries */
#define CHIP_FLASH_ROW_SIZE          256	/* row size */

/* Manufacturing related data. */
/* Certs in the RO region are written as 4-kB + 3-kB blocks to the A &
 * B banks respectively.
 */
#define RO_CERTS_A_OFF                     (CHIP_RO_A_MEM_OFF + 0x2800)
#define RO_CERTS_B_OFF                     (CHIP_RO_B_MEM_OFF + 0x2800)
#define RO_CERTS_A_SIZE                     0x01000
#define RO_CERTS_B_SIZE                     0x00c00
/* We have an unused 3-kB region in the B bank, for future proofing. */
#define RO_CERTS_PAD_B_SIZE                 0x00c00
/* Factory provision data is written as a 2-kB block to the A bank. */
#define RO_PROVISION_DATA_A_OFF             0x3800
#define RO_PROVISION_DATA_A_SIZE            0x0800

#endif	/* __CROS_EC_FLASH_LAYOUT_H */
