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

#define LOG_TAG "broken_adv"

#include "broken_adv.h"

#include "osi/include/log.h"
#include "stack/include/hcidefs.h"

using std::vector;

namespace test_vendor_lib {

BrokenAdv::BrokenAdv() {
  advertising_interval_ms_ = std::chrono::milliseconds(1280);
  advertising_type_ = BTM_BLE_NON_CONNECT_EVT;
  adv_data_ = {
      0x02,  // Length
      BTM_BLE_AD_TYPE_FLAG,
      BTM_BLE_BREDR_NOT_SPT | BTM_BLE_GEN_DISC_FLAG,
      0x13,  // Length
      BTM_BLE_AD_TYPE_NAME_CMPL,
      'g',
      'D',
      'e',
      'v',
      'i',
      'c',
      'e',
      '-',
      'b',
      'r',
      'o',
      'k',
      'e',
      'n',
      '_',
      'a',
      'd',
      'v',
  };

  constant_adv_data_ = adv_data_;

  scan_response_present_ = true;
  scan_data_ = {0x0b,  // Length
                BTM_BLE_AD_TYPE_NAME_SHORT,
                'b',
                'r',
                'o',
                'k',
                'e',
                'n',
                'n',
                'e',
                's',
                's'};

  extended_inquiry_data_ = {0x07,  // Length
                            BT_EIR_COMPLETE_LOCAL_NAME_TYPE,
                            'B',
                            'R',
                            '0',
                            'K',
                            '3',
                            'N'};
  page_scan_repetition_mode_ = 0;
  page_scan_delay_ms_ = std::chrono::milliseconds(600);
}

void BrokenAdv::Initialize(const vector<std::string>& args) {
  if (args.size() < 2) return;

  BtAddress addr;
  if (addr.FromString(args[1])) SetBtAddress(addr);

  if (args.size() < 3) return;

  SetAdvertisementInterval(std::chrono::milliseconds(std::stoi(args[2])));
}

// Mostly return the correct length
static uint8_t random_length(size_t bytes_remaining) {
  uint32_t randomness = rand();

  switch ((randomness & 0xf000000) >> 24) {
    case (0):
      return bytes_remaining + (randomness & 0xff);
    case (1):
      return bytes_remaining - (randomness & 0xff);
    case (2):
      return bytes_remaining + (randomness & 0xf);
    case (3):
      return bytes_remaining - (randomness & 0xf);
    case (5):
    case (6):
      return bytes_remaining + (randomness & 0x3);
    case (7):
    case (8):
      return bytes_remaining - (randomness & 0x3);
    default:
      return bytes_remaining;
  }
}

static size_t random_adv_type() {
  uint32_t randomness = rand();

  switch ((randomness & 0xf000000) >> 24) {
    case (0):
      return BTM_EIR_MANUFACTURER_SPECIFIC_TYPE;
    case (1):
      return (randomness & 0xff);
    default:
      return (randomness & 0x1f);
  }
}

static size_t random_data_length(size_t length, size_t bytes_remaining) {
  uint32_t randomness = rand();

  switch ((randomness & 0xf000000) >> 24) {
    case (0):
      return bytes_remaining;
    case (1):
      return (length + (randomness & 0xff)) % bytes_remaining;
    default:
      return (length <= bytes_remaining ? length : bytes_remaining);
  }
}

static void RandomizeAdvertisement(vector<uint8_t>& ad, size_t max) {
  uint8_t length = random_length(max);
  uint8_t data_length = random_data_length(length, max);

  ad.push_back(random_adv_type());
  ad.push_back(length);
  for (size_t i = 0; i < data_length; i++) ad.push_back(rand() & 0xff);
}

void BrokenAdv::UpdateAdvertisement() {
  adv_data_.clear();
  for (size_t i = 0; i < constant_adv_data_.size(); i++)
    adv_data_.push_back(constant_adv_data_[i]);

  RandomizeAdvertisement(adv_data_, 31 - adv_data_.size());

  RandomizeAdvertisement(scan_data_, 31);

  std::vector<uint8_t> curr_addr;
  BtAddress next_addr;
  GetBtAddress().ToVector(curr_addr);
  curr_addr[0]++;
  next_addr.FromVector(curr_addr);

  SetBtAddress(next_addr);
}

std::string BrokenAdv::ToString() const {
  std::string str = Device::ToString() + std::string(": Interval = ") +
                    std::to_string(advertising_interval_ms_.count());
  return str;
}

void BrokenAdv::UpdatePageScan() {
  std::vector<uint8_t> broken_addr;
  RandomizeAdvertisement(scan_data_, 31);

  BtAddress next_addr;
  GetBtAddress().ToVector(broken_addr);
  broken_addr[1]++;
  next_addr.FromVector(broken_addr);

  SetBtAddress(next_addr);
}

void BrokenAdv::TimerTick() {
  UpdatePageScan();
  UpdateAdvertisement();
}

}  // namespace test_vendor_lib
