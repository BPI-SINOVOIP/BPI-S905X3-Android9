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

#ifndef ESE_HW_NXP_PN80T_COMMON_H_
#define ESE_HW_NXP_PN80T_COMMON_H_ 1

#include "../../libese-teq1/include/ese/teq1.h"
#include "../../libese/include/ese/ese.h"
#include "../../libese/include/ese/log.h"
#include "platform.h"

/* Card state is _required_ to be at the front of eSE pad. */
struct NxpState {
  void *handle;
};

/* pad[0] is reserved for T=1. Lazily go to the middle. */
#define NXP_PN80T_STATE(ese)                                                   \
  ((struct NxpState *)(&ese->pad[ESE_INTERFACE_STATE_PAD / 2]))

void nxp_pn80t_close(struct EseInterface *ese);
uint32_t nxp_pn80t_transceive(struct EseInterface *ese,
                              const struct EseSgBuffer *tx_buf, uint32_t tx_len,
                              struct EseSgBuffer *rx_buf, uint32_t rx_len);
int nxp_pn80t_poll(struct EseInterface *ese, uint8_t poll_for, float timeout,
                   int complete);
int nxp_pn80t_reset(struct EseInterface *ese);
int nxp_pn80t_open(struct EseInterface *ese, void *board);

enum NxpPn80tError {
  kNxpPn80tError = kTeq1ErrorMax,
  kNxpPn80tErrorPlatformInit,
  kNxpPn80tErrorPollRead,
  kNxpPn80tErrorResetToggle,
  kNxpPn80tErrorTransmit,
  kNxpPn80tErrorTransmitSize,
  kNxpPn80tErrorReceive,
  kNxpPn80tErrorReceiveSize,
  kNxpPn80tErrorMax,  /* sizeof(kNxpPn80tErrorMessages) */
};

extern const char *kNxpPn80tErrorMessages[];
#endif  /* ESE_HW_NXP_PN80T_COMMON_H_ */
