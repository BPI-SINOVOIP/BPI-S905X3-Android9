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

#define LOG_TAG "sco_packet"

#include "sco_packet.h"

#include "stack/include/hcidefs.h"

using std::vector;

namespace test_vendor_lib {

ScoPacket::ScoPacket(uint16_t channel,
                     ScoPacket::PacketStatusFlags packet_status)
    : Packet(DATA_TYPE_SCO,
             {static_cast<uint8_t>(channel & 0xff),
              static_cast<uint8_t>(((channel >> 8) & 0x0f) |
                                   (packet_status & 0x3 << 4))}) {}

uint16_t ScoPacket::GetChannel() const {
  return (GetHeader()[0] | (GetHeader()[1] << 8)) & 0xfff;
}

ScoPacket::PacketStatusFlags ScoPacket::GetPacketStatusFlags() const {
  return static_cast<ScoPacket::PacketStatusFlags>((GetHeader()[1] >> 4) & 0x3);
}

}  // namespace test_vendor_lib
