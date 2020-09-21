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

#include <cmath>
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "hci_packet.h"
#include "l2cap_sdu.h"

namespace test_vendor_lib {

const int kSduHeaderLength = 4;

class L2capPacket : public HciPacket {
 public:
  // Returns an assembled L2cap object if successful, nullptr if failure.
  static std::shared_ptr<L2capPacket> assemble(
      const std::vector<std::shared_ptr<L2capSdu> >& sdu_packet);

  // Returns a fragmented vector of L2capSdu objects if successful
  // Returns an empty vector of L2capSdu objects if unsuccessful
  std::vector<std::shared_ptr<L2capSdu> > fragment(uint16_t maximum_sdu_size,
                                                   uint8_t txseq,
                                                   uint8_t reqseq);

  uint16_t get_l2cap_cid() const;

  // HciPacket Functions
  size_t get_length();
  uint8_t& get_at_index(size_t index);

 private:
  L2capPacket() = default;

  // Entire L2CAP packet: length, CID, and payload in that order.
  std::vector<uint8_t> l2cap_packet_;

  DISALLOW_COPY_AND_ASSIGN(L2capPacket);

  // Helper functions for fragmenting.
  static void set_sdu_header_length(std::vector<uint8_t>& sdu, uint16_t length);

  static void set_total_sdu_length(std::vector<uint8_t>& sdu,
                                   uint16_t total_sdu_length);

  static void set_sdu_cid(std::vector<uint8_t>& sdu, uint16_t cid);

  static void set_sdu_control_bytes(std::vector<uint8_t>& sdu, uint8_t txseq,
                                    uint8_t reqseq);

  bool check_l2cap_packet() const;

};  // L2capPacket

}  // namespace test_vendor_lib
