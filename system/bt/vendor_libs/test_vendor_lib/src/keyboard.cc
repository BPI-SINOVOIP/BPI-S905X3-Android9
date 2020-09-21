/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "keyboard"

#include "keyboard.h"

using std::vector;

namespace test_vendor_lib {
Keyboard::Keyboard() {
  advertising_type_ = BTM_BLE_CONNECT_EVT;
  adv_data_ = {0x11,  // Length
               BTM_BLE_AD_TYPE_NAME_CMPL,
               'g',
               'D',
               'e',
               'v',
               'i',
               'c',
               'e',
               '-',
               'k',
               'e',
               'y',
               'b',
               'o',
               'a',
               'r',
               'd',
               0x03,  // Length
               0x19,
               0xC1,
               0x03,
               0x03,  // Length
               0x03,
               0x12,
               0x18,
               0x02,  // Length
               BTM_BLE_AD_TYPE_FLAG,
               BTM_BLE_BREDR_NOT_SPT | BTM_BLE_GEN_DISC_FLAG};

  scan_data_ = {0x04,  // Length
                BTM_BLE_AD_TYPE_NAME_SHORT, 'k', 'e', 'y'};
}

std::string Keyboard::GetTypeString() const { return "keyboard"; }

void Keyboard::Initialize(const vector<std::string>& args) {
  if (args.size() < 2) return;

  BtAddress addr;
  if (addr.FromString(args[1])) SetBtAddress(addr);

  if (args.size() < 3) return;

  SetAdvertisementInterval(std::chrono::milliseconds(std::stoi(args[2])));
}

bool Keyboard::LeConnect() { return true; }

bool Keyboard::IsPageScanAvailable() const { return false; }
}  // namespace test_vendor_lib
