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

#include <string.h>

#include <ese/ese.h>
ESE_INCLUDE_HW(ESE_HW_FAKE);

/* Minimal ATR */
const uint8_t kAtr[] = {0x00, 0x00};
const size_t kAtrLength = sizeof(kAtr);
const void *kEseOpenData = NULL;

void ese_relay_init(struct EseInterface *ese) { ese_init(ese, ESE_HW_FAKE); }
