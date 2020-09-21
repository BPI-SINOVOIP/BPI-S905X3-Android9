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

#ifndef ESE_HW_NXP_PN80T_PLATFORM_H_
#define ESE_HW_NXP_PN80T_PLATFORM_H_ 1

typedef void *(pn80t_platform_initialize_t)(void *);
typedef int (pn80t_platform_release_t)(void *);
typedef int (pn80t_platform_toggle_t)(void *, int);
typedef int (pn80t_platform_wait_t)(void *, long usec);

/* Pn80tPlatform
 *
 * Provides the callbacks necessary to interface with the platform, be it the Linux
 * kernel or a bootloader in pn80t_common.c
 *
 * All "required" functions must be provided.
 * All "optional" functions may be set to NULL.
 *
 */
struct Pn80tPlatform {
  /* Required: Initializes the hardware and platform opaque handle. */
  pn80t_platform_initialize_t *const initialize;
  /* Required: free memory and release resources as needed. */
  pn80t_platform_release_t *const release;
  /* Required: determines eSE specific power. 1 = on, 0 = off. */
  pn80t_platform_toggle_t *const toggle_reset;  /* ESE_RST or other power control. */
  /* Optional: determines global NFC power: 1 = on, 0 = off */
  pn80t_platform_toggle_t *const toggle_ven;  /* NFC_VEN */
  /* Optional: enables eSE power control via |toggle_reset|. 1 = on, 0 = off */
  pn80t_platform_toggle_t *const toggle_power_req;  /* SVDD_PWR_REQ */
  /* Optional: toggles the in bootloader gpio */
  pn80t_platform_toggle_t *const toggle_bootloader;  /* CLEAR_N */
  /* Required: provides a usleep() equivalent. */
  pn80t_platform_wait_t *const wait;
};

#endif
