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

#define LOG_TAG "classic"

#include "classic.h"
#include "osi/include/log.h"
#include "stack/include/hcidefs.h"

using std::vector;

namespace test_vendor_lib {

Classic::Classic() {
  advertising_interval_ms_ = std::chrono::milliseconds(0);
  device_class_ = 0x30201;

  extended_inquiry_data_ = {0x10,  // Length
                            BT_EIR_COMPLETE_LOCAL_NAME_TYPE,
                            'g',
                            'D',
                            'e',
                            'v',
                            'i',
                            'c',
                            'e',
                            '-',
                            'c',
                            'l',
                            'a',
                            's',
                            's',
                            'i',
                            'c'};
  page_scan_repetition_mode_ = 0;
  page_scan_delay_ms_ = std::chrono::milliseconds(600);
}

void Classic::Initialize(const vector<std::string>& args) {
  if (args.size() < 2) return;

  BtAddress addr;
  if (addr.FromString(args[1])) SetBtAddress(addr);

  if (args.size() < 3) return;

  SetClockOffset(std::stoi(args[2]));
}

}  // namespace test_vendor_lib
