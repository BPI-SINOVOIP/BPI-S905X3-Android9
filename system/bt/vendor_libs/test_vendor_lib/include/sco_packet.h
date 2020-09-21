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

#include "packet.h"

namespace test_vendor_lib {

// SCO data packets are specified in the Bluetooth Core Specification Version
// 4.2, Volume 2, Part E, Section 5.4.3
class ScoPacket : public Packet {
 public:
  virtual ~ScoPacket() override = default;
  typedef enum {
    CorrectlyReceived,
    PossiblyIncomplete,
    NoData,
    PartiallyLost
  } PacketStatusFlags;

  uint16_t GetChannel() const;

  PacketStatusFlags GetPacketStatusFlags() const;

  explicit ScoPacket(uint16_t channel, PacketStatusFlags status_flags);
};

}  // namespace test_vendor_lib
