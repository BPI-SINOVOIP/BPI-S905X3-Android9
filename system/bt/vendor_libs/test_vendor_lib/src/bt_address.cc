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

#include "bt_address.h"
#include <iomanip>
#include <vector>
using std::vector;

#include <base/logging.h>

namespace test_vendor_lib {

bool BtAddress::IsValid(const std::string& addr) {
  if (addr.length() < kStringLength) return false;

  for (size_t i = 0; i < kStringLength; i++) {
    if (i % 3 == 2) {  // Every third character must be ':'
      if (addr[i] != ':') return false;
    } else {  // The rest must be hexadecimal digits
      if (!isxdigit(addr[i])) return false;
    }
  }
  return true;
}

bool BtAddress::FromString(const std::string& str) {
  std::string tok;
  std::istringstream iss(str);

  if (!IsValid(str)) return false;

  address_ = 0;
  for (size_t i = 0; i < kOctets; i++) {
    getline(iss, tok, ':');
    uint64_t octet = std::stoi(tok, nullptr, 16);
    address_ |= (octet << (8 * ((kOctets - 1) - i)));
  }
  return true;
}

bool BtAddress::FromVector(const vector<uint8_t>& octets) {
  if (octets.size() < kOctets) return false;

  address_ = 0;
  for (size_t i = 0; i < kOctets; i++) {
    uint64_t to_shift = octets[i];
    address_ |= to_shift << (8 * i);
  }
  return true;
}

void BtAddress::ToVector(vector<uint8_t>& octets) const {
  for (size_t i = 0; i < kOctets; i++) {
    octets.push_back(address_ >> (8 * i));
  }
}

std::string BtAddress::ToString() const {
  std::stringstream ss;

  ss << std::hex << std::setfill('0') << std::setw(2);
  for (size_t i = 0; i < kOctets; i++) {
    uint64_t octet = (address_ >> (8 * ((kOctets - 1) - i))) & 0xff;
    ss << std::hex << std::setfill('0') << std::setw(2) << octet;
    if (i != kOctets - 1) ss << std::string(":");
  }

  return ss.str();
}

}  // namespace test_vendor_lib
