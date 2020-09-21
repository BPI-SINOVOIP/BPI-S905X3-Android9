/*
 * Copyright 2017 The Android Open Source Project
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

#define LOG_TAG "acl_packet"

#include "acl_packet.h"

#include "base/logging.h"
#include "osi/include/log.h"
#include "stack/include/hcidefs.h"

using std::vector;

namespace test_vendor_lib {

AclPacket::AclPacket(uint16_t channel,
                     AclPacket::PacketBoundaryFlags packet_boundary,
                     AclPacket::BroadcastFlags broadcast)
    : raw_packet_({static_cast<uint8_t>(channel & 0xff),
                   static_cast<uint8_t>(((channel >> 8) & 0x0f) |
                                        ((packet_boundary & 0x3) << 4) |
                                        (broadcast & 0x3) << 6),
                   0x00, 0x00}) {}

void AclPacket::AddPayloadOctets(size_t octets, const vector<uint8_t>& bytes) {
  CHECK(bytes.size() == octets);

  raw_packet_.insert(raw_packet_.end(), bytes.begin(), bytes.end());

  raw_packet_[2] =
      static_cast<uint8_t>((raw_packet_.size() - kHeaderSize) & 0xff);
  raw_packet_[3] =
      static_cast<uint8_t>(((raw_packet_.size() - kHeaderSize) >> 8) & 0xff);
}

void AclPacket::AddPayloadOctets(size_t octets, uint64_t value) {
  vector<uint8_t> val_vector;

  uint64_t v = value;

  CHECK(octets <= sizeof(uint64_t));

  for (size_t i = 0; i < octets; i++) {
    val_vector.push_back(v & 0xff);
    v = v >> 8;
  }

  CHECK(v == 0);

  AddPayloadOctets(octets, val_vector);
}

size_t AclPacket::GetPacketSize() const { return raw_packet_.size(); }

const std::vector<uint8_t>& AclPacket::GetPacket() const { return raw_packet_; }

}  // namespace test_vendor_lib
