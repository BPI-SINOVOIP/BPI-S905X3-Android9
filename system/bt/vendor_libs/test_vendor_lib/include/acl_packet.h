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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace test_vendor_lib {

// ACL data packets are specified in the Bluetooth Core Specification Version
// 4.2, Volume 2, Part E, Section 5.4.2
class AclPacket {
 public:
  typedef enum {
    FirstNonAutomaticallyFlushable,
    Continuing,
    FirstAutomaticallyFlushable,
    Complete
  } PacketBoundaryFlags;
  typedef enum {
    PointToPoint,
    ActiveSlaveBroadcast,
    ParkedSlaveBroadcast,
    Reserved
  } BroadcastFlags;

  virtual ~AclPacket() = default;

  uint16_t GetChannel() const {
    return (raw_packet_[0] | (raw_packet_[1] << 8)) & 0xfff;
  }

  PacketBoundaryFlags GetPacketBoundaryFlags() const {
    return static_cast<PacketBoundaryFlags>((raw_packet_[1] & 0x30) >> 4);
  }

  BroadcastFlags GetBroadcastFlags() const {
    return static_cast<BroadcastFlags>((raw_packet_[1] & 0xC0) >> 6);
  }

  explicit AclPacket(uint16_t channel,
                     AclPacket::PacketBoundaryFlags boundary_flags,
                     AclPacket::BroadcastFlags broadcast);

  size_t GetPacketSize() const;

  const std::vector<uint8_t>& GetPacket() const;

  void AddPayloadOctets(size_t octets, const std::vector<uint8_t>& bytes);

 private:
  // Add |octets| bytes to the payload.
  void AddPayloadOctets(size_t octets, uint64_t value);

  static const size_t kHeaderSize = 4;

 public:
  // Add type-checking versions
  void AddPayloadOctets1(uint8_t value) { AddPayloadOctets(1, value); }
  void AddPayloadOctets2(uint16_t value) { AddPayloadOctets(2, value); }
  void AddPayloadOctets3(uint32_t value) { AddPayloadOctets(3, value); }
  void AddPayloadOctets4(uint32_t value) { AddPayloadOctets(4, value); }
  void AddPayloadOctets6(uint64_t value) { AddPayloadOctets(6, value); }
  void AddPayloadOctets8(uint64_t value) { AddPayloadOctets(8, value); }

 private:
  std::vector<uint8_t> raw_packet_;
};

}  // namespace test_vendor_lib
