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

#ifndef ESE_APP_BOOT_PRIVATE_H_
#define ESE_APP_BOOT_PRIVATE_H_ 1

typedef enum {
  LOCK_STATE_PROVISION =  (0x1 << 0),
  LOCK_STATE_CARRIER = (0x1 << 1),
  LOCK_STATE_DEVICE = (0x1 << 2),
  LOCK_STATE_BOOT = (0x1 << 3),
} LockState;

extern const uint8_t kManageChannelOpen[];
extern const uint32_t kManageChannelOpenLength;
extern const uint8_t kManageChannelClose[];
extern const uint8_t kSelectApplet[];
extern const uint32_t kSelectAppletLength;
extern const uint8_t kStoreCmd[];
extern const uint8_t kLoadCmd[];
extern const uint8_t kGetLockState[];
extern const uint8_t kSetLockState[];
extern const uint8_t kGetState[];
extern const uint8_t kSetProduction[];
extern const uint8_t kCarrierLockTest[];

#endif  /* ESE_APP_BOOT_PRIVATE_H_ */
