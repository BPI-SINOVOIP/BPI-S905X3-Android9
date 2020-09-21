/*
 * Copyright 2015 The Android Open Source Project
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

#define LOG_TAG "packet"

#include "packet.h"

#include <algorithm>

#include <base/logging.h>
#include "osi/include/log.h"

using std::vector;

namespace test_vendor_lib {

Packet::Packet(serial_data_type_t type, vector<uint8_t> header)
    : type_(type), header_(std::move(header)) {
  payload_ = {0};
}

bool Packet::AddPayloadOctets(size_t octets, const vector<uint8_t>& bytes) {
  if (GetPayloadSize() + octets > kMaxPayloadOctets) return false;

  if (octets != bytes.size()) return false;

  payload_.insert(payload_.end(), bytes.begin(), bytes.end());
  payload_[0] = payload_.size() - 1;

  return true;
}

bool Packet::AddPayloadOctets(size_t octets, uint64_t value) {
  vector<uint8_t> val_vector;

  uint64_t v = value;

  if (octets > sizeof(uint64_t)) return false;

  for (size_t i = 0; i < octets; i++) {
    val_vector.push_back(v & 0xff);
    v = v >> 8;
  }

  if (v != 0) return false;

  return AddPayloadOctets(octets, val_vector);
}

bool Packet::AddPayloadBtAddress(const BtAddress& address) {
  if (GetPayloadSize() + BtAddress::kOctets > kMaxPayloadOctets) return false;

  address.ToVector(payload_);

  payload_[0] = payload_.size() - 1;

  return true;
}

bool Packet::IncrementPayloadCounter(size_t index) {
  if (payload_.size() < index - 1) return false;

  payload_[index]++;
  return true;
}

bool Packet::IncrementPayloadCounter(size_t index, uint8_t max_val) {
  if (payload_.size() < index - 1) return false;

  if (payload_[index] + 1 > max_val) return false;

  payload_[index]++;
  return true;
}

const vector<uint8_t>& Packet::GetHeader() const {
  // Every packet must have a header.
  CHECK(GetHeaderSize() > 0);
  return header_;
}

uint8_t Packet::GetHeaderSize() const { return header_.size(); }

size_t Packet::GetPacketSize() const {
  // Add one for the type octet.
  return 1 + header_.size() + payload_.size();
}

const vector<uint8_t>& Packet::GetPayload() const { return payload_; }

size_t Packet::GetPayloadSize() const { return payload_.size(); }

serial_data_type_t Packet::GetType() const { return type_; }

}  // namespace test_vendor_lib
