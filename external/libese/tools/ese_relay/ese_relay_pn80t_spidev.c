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

/* Pull in NXP PN80T for use in the relay. */
#include <ese/ese.h>
#include <ese/hw/nxp/pn80t/boards/hikey-spidev.h>
ESE_INCLUDE_HW(ESE_HW_NXP_PN80T_SPIDEV);

/* From
 * http://stackoverflow.com/questions/27074551/how-to-list-applets-on-jcop-cards
 */
static const uint8_t kAtrBytes[] = {
    0x3B, 0xF8, 0x13, 0x00, 0x00, 0x81, 0x31, 0xFE, 0x45,
    0x4A, 0x43, 0x4F, 0x50, 0x76, 0x32, 0x34, 0x31, 0xB7,
};
const uint8_t *kAtr = &kAtrBytes[0];
const size_t kAtrLength = sizeof(kAtr);
const void *kEseOpenData = (void *)(&nxp_boards_hikey_spidev);

void ese_relay_init(struct EseInterface *ese) {
  ese_init(ese, ESE_HW_NXP_PN80T_SPIDEV);
}
