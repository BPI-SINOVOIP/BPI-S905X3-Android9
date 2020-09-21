/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __CROS_EC_INCLUDE_APP_NUGGET_H
#define __CROS_EC_INCLUDE_APP_NUGGET_H
#include "application.h"
#include "flash_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*
 * APP_ID_NUGGET uses the Transport API
 */
/****************************************************************************/

/* App-specific errors */
enum {
  NUGGET_ERROR_LOCKED = APP_SPECIFIC_ERROR,
  NUGGET_ERROR_RETRY,
};

/****************************************************************************/
/* Application functions */

#define NUGGET_PARAM_VERSION 0x0000
/*
 * Return the one-line version string of the running image
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        Null-terminated ASCII string
 * @param reply_len    Max length to return
 *
 * @errors             APP_ERROR_TOO_MUCH
 */

/****************************************************************************/
/* Firmware upgrade stuff */

struct nugget_app_flash_block {
  uint32_t block_digest;                 /* first 4 bytes of sha1 of the rest */
  uint32_t offset;                       /* from start of flash */
  uint8_t payload[CHIP_FLASH_BANK_SIZE]; /* data to write */
} __packed;

#define NUGGET_PARAM_FLASH_BLOCK 0x0001
/*
 * Erase and write a single flash block.
 *
 * @param args         struct nugget_app_flash_block
 * @param arg_len      sizeof(struct nugget_app_flash_block)
 * @param reply        <none>
 * @param reply_len    0
 *
 * @errors             NUGGET_ERROR_LOCKED, NUGGET_ERROR_RETRY
 */

#define NUGGET_PARAM_REBOOT 0x0002
/*
 * Reboot Citadel
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        <none>
 * @param reply_len    0
 */

/*********
 * Firmware updates are written to flash with invalid headers. If an update
 * password exists, headers can only be marked valid by providing that
 * password.
 */

/*
 * An unassigned password is defined to be all 0xff, with a don't-care digest.
 * Anything else must have a valid digest over all password bytes. The password
 * length is chosen arbitrarily for now, but should always be a fixed size with
 * all bytes used, to resist brute-force guesses.
 */
#define NUGGET_UPDATE_PASSWORD_LEN 32
struct nugget_app_password {
  uint32_t digest;      /* first 4 bytes of sha1 of password (little endian) */
  uint8_t password[NUGGET_UPDATE_PASSWORD_LEN];
} __packed;


enum NUGGET_ENABLE_HEADER {
  NUGGET_ENABLE_HEADER_RO = 0x01,
  NUGGET_ENABLE_HEADER_RW = 0x02,
};
struct nugget_app_enable_update {
  struct nugget_app_password  password;
  uint8_t which_headers;                        /* bit 0 = RO, bit 1 = RW */
} __packed;
#define NUGGET_PARAM_ENABLE_UPDATE 0x0003
/*
 * Mark the specified image header(s) as valid, if the provided password
 * matches. Returns true if either header was CHANGED to valid, which is an
 * indication that the AP should request a reboot so that it can take effect.
 *
 * @param args         struct nugget_app_enable_update
 * @param arg_len      sizeof(struct nugget_app_enable_update)
 * @param reply        0 or 1
 * @param reply_len    1 byte
 *
 * @errors             APP_ERROR_BOGUS_ARGS
 */


struct nugget_app_change_update_password {
  struct nugget_app_password  old_password;
  struct nugget_app_password  new_password;
} __packed;
#define NUGGET_PARAM_CHANGE_UPDATE_PASSWORD 0x0004
/*
 * Change the update password.
 *
 * @param args         struct nugget_app_change_update_password
 * @param arg_len      sizeof(struct nugget_app_change_update_password)
 * @param reply        <none>
 * @param reply_len    0
 *
 * @errors             APP_ERROR_BOGUS_ARGS
 */

#define NUGGET_PARAM_NUKE_FROM_ORBIT 0x0005
#define ERASE_CONFIRMATION 0xc05fefee
/*
 * This will erase ALL user secrets and reboot.
 *
 * @param args         uint32_t containing the ERASE_CONFIRMATION value
 * @param arg_len      sizeof(uint32_t)
 * @param reply        <none>
 * @param reply_len    0
 *
 * @errors             APP_ERROR_BOGUS_ARGS
 */

#define NUGGET_PARAM_DEVICE_ID 0x0006
/*
 * Get the device ID of the chip.
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        Null-terminated ASCII string
 * @param reply_len    Max length to return
 */


#define NUGGET_PARAM_LONG_VERSION 0x0007
/*
 * Return the multi-line description of all images
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        Null-terminated ASCII string
 * @param reply_len    Max length to return
 *
 * @errors             APP_ERROR_TOO_MUCH
 */

#define NUGGET_PARAM_HEADER_RO_A 0x0008
/*
 * Return the signature header for RO_A
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        struct SignedHeader
 * @param reply_len    Max length to return
 *
 * @errors             APP_ERROR_TOO_MUCH
 */

#define NUGGET_PARAM_HEADER_RO_B 0x0009
/*
 * Return the signature header for RO_B
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        struct SignedHeader
 * @param reply_len    Max length to return
 *
 * @errors             APP_ERROR_TOO_MUCH
 */

#define NUGGET_PARAM_HEADER_RW_A 0x000a
/*
 * Return the signature header for RW_A
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        struct SignedHeader
 * @param reply_len    Max length to return
 *
 * @errors             APP_ERROR_TOO_MUCH
 */

#define NUGGET_PARAM_HEADER_RW_B 0x000b
/*
 * Return the signature header for RW_B
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        struct SignedHeader
 * @param reply_len    Max length to return
 *
 * @errors             APP_ERROR_TOO_MUCH
 */

#define NUGGET_PARAM_REPO_SNAPSHOT 0x000c
/*
 * Return the multi-line repo snapshot info for the current image
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        Null-terminated ASCII string
 * @param reply_len    Max length to return
 *
 * @errors             APP_ERROR_TOO_MUCH
 */

enum nugget_ap_uart_passthru_cfg {
  NUGGET_AP_UART_OFF,                   /* off */
  NUGGET_AP_UART_IS_USB,                /* USB CCD is in use over SBU */
  NUGGET_AP_UART_ENABLED,               /* AP UART is on SBU lines */
  NUGGET_AP_UART_SSC_UART,              /* This doesn't actually exist */
  NUGGET_AP_UART_CITADEL_UART,          /* Citadel UART on SBU lines (ew) */

  NUGGET_AP_UART_NUM_CFGS,
};
#define NUGGET_PARAM_AP_UART_PASSTHRU 0x000d
/*
 * Enable/Disable the AP UART PASSTHRU function
 *
 * This always returns the current state of the AP UART passthru feature. Even
 * if the AP UART is disabled, a SuzyQable may connected to use the SBU lines.
 *
 * The AP can only request that the AP UART passthru feature be enabled
 * (NUGGET_AP_UART_ENABLED), or disabled (NUGGET_AP_UART_OFF). The other enums
 * are for internal testing.
 *
 * @param args         <none>  OR  enum nugget_ap_uart_passthru_cfg
 * @param arg_len        0     OR   1 byte
 * @param reply        enum nugget_param_ap_uart_passthru
 * @param reply_len    1 byte
 *
 * @errors             APP_ERROR_BOGUS_ARGS
 */

/****************************************************************************/
/* Test related commands */

#define NUGGET_PARAM_CYCLES_SINCE_BOOT 0x0100
/*
 * Get the number of cycles since boot
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        uint32_t cycles
 * @param reply_len    sizeof(uint32_t)
 */

/****************************************************************************/
/* Support for Power 1.1 HAL */

/*
 * This struct is specific to Citadel and Nugget OS, but it's enough for the
 * AP-side implementation to translate into the info required for the HAL
 * structs.
 */
struct nugget_app_low_power_stats {
  /* All times in usecs */
  uint64_t hard_reset_count;                    /* Cleared by power loss */
  uint64_t time_since_hard_reset;
  /* Below are only since the last hard reset */
  uint64_t wake_count;
  uint64_t time_at_last_wake;
  uint64_t time_spent_awake;
  uint64_t deep_sleep_count;
  uint64_t time_at_last_deep_sleep;
  uint64_t time_spent_in_deep_sleep;
} __packed;

#define NUGGET_PARAM_GET_LOW_POWER_STATS 0x200
/*
 * Return information regarding deep sleep transitions
 *
 * @param args         <none>
 * @param arg_len      0
 * @param reply        struct nugget_param_get_low_power_stats
 * @param reply_len    sizeof(struct nugget_param_get_low_power_stats)
 */

/* UNIMPLEMENTED */
/* Reseved just in case we decide we need it */
#define NUGGET_PARAM_CLEAR_LOW_POWER_STATS 0x201
/* UNIMPLEMENTED */

/****************************************************************************/
/* These are bringup / debug functions only.
 *
 * TODO(b/65067435): Remove all of these.
 */

#define NUGGET_PARAM_READ32 0xF000
/*
 * Read a 32-bit value from memory.
 *
 * DANGER, WILL ROBINSON! DANGER! There is NO sanity checking on this AT ALL.
 * Read the wrong address, and Bad Things(tm) WILL happen.
 *
 * @param args         uint32_t address
 * @param arg_len      sizeof(uint32_t)
 * @param reply        uint32_t value
 * @param reply_len    sizeof(uint32_t)
 */

struct nugget_app_write32 {
  uint32_t address;
  uint32_t value;
} __packed;

#define NUGGET_PARAM_WRITE32 0xF001
/*
 * Write a 32-bit value to memory
 *
 * DANGER, WILL ROBINSON! DANGER! There is NO sanity checking on this AT ALL.
 * Write the wrong values to the wrong address, and Bad Things(tm) WILL happen.
 *
 * @param args         struct nugget_app_write32
 * @param arg_len      sizeof(struct nugget_app_write32)
 * @param reply        <none>
 * @param reply_len    0
 */

#ifdef __cplusplus
}
#endif

#endif  /* __CROS_EC_INCLUDE_APP_NUGGET_H */
