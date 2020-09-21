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
#include <iterator>
#include <vector>

#include "hci_packet.h"

namespace test_vendor_lib {

// Abstract representation of an SDU packet that contains an L2CAP
// payload. This class is meant to be used in collaboration with
// the L2cap class defined in l2cap.h. For example, an SDU packet
// may look as follows:
//
// vector<uint8_t> sdu = {0x04, 0x00, 0x48, 0x00, 0x04, 0x00, 0xab,
//                       0xcd, 0x78, 0x56}
//
// The first two bytes (in little endian) should be read as 0x0004
// and is the length of the payload of the SDU packet.
//
// The next two bytes (also in little endian) are the channel ID
// and should be read as 0x0048. These should remain the same for
// any number of SDUs that are a part of the same packet stream.
//
// Following the CID bytes, are the total length bytes. Since this
// SDU only requires a single packet, the length here is the same
// as the length in the first two bytes of the packet. Again stored
// in little endian.
//
// Next comes the two control bytes. These begin the L2CAP payload
// of the SDU packet; however, they will not be added to the L2CAP
// packet that is being constructed in the assemble function that
// will be creating an L2CAP packet from a stream of L2capSdu
// objects.
//
// The final two bytes are the frame check sequence that should be
// calculated from the start of the vector to the end of the
// payload.
//
// Thus, calling assemble on this example would create a
// zero-length L2CAP packet because the information payload of the
// L2CAP packet will not include either of the control or FCS
// bytes.
//
class L2capSdu : public HciPacket {
 public:
  // Returns a unique_ptr to an L2capSdu object that is constructed with the
  // assumption that the SDU packet is complete and correct.
  static std::shared_ptr<L2capSdu> L2capSduConstructor(
      std::vector<uint8_t> create_from);

  // Adds an FCS to create_from and returns a unique_ptr to an L2capSdu object.
  static std::shared_ptr<L2capSdu> L2capSduBuilder(
      std::vector<uint8_t> create_from);

  // Get the FCS bytes from the end of the L2CAP payload of an SDU
  // packet.
  uint16_t get_fcs() const;

  uint16_t get_payload_length() const;

  uint16_t calculate_fcs() const;

  // Get the two control bytes that begin the L2CAP payload. These
  // bytes will contain information such as the Segmentation and
  // Reassembly bits, and the TxSeq/ReqSeq numbers.
  uint16_t get_controls() const;

  uint16_t get_total_l2cap_length() const;

  size_t get_vector_size() const;

  uint16_t get_channel_id() const;

  // Returns true if the SDU control sequence for Segmentation and
  // Reassembly is 00b, false otherwise.
  static bool is_complete_l2cap(const L2capSdu& sdu);

  // Returns true if the SDU control sequence for Segmentation and
  // Reassembly is 01b, false otherwise.
  static bool is_starting_sdu(const L2capSdu& sdu);

  // Returns true if the SDU control sequence for Segmentation and
  // Reasembly is 10b, false otherwise.
  static bool is_ending_sdu(const L2capSdu& sdu);

  // HciPacket functions
  size_t get_length();
  uint8_t& get_at_index(size_t index);

 private:
  // This is the SDU packet in bytes.
  std::vector<uint8_t> sdu_data_;

  // Returns a completed L2capSdu object.
  L2capSdu(std::vector<uint8_t>&& create_from);

  // Table for precalculated lfsr values.
  static const uint16_t lfsr_table_[256];

  uint16_t convert_from_little_endian(const unsigned int starting_index) const;

};  // L2capSdu

}  // namespace test_vendor_lib
