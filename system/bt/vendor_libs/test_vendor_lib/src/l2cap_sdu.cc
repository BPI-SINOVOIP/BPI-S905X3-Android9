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

#include "l2cap_sdu.h"

#include <base/logging.h>

namespace test_vendor_lib {

const uint16_t kSduSarBits = 0xe000;

// Define the LFSR table of precalculated values defined by the
// Bluetooth specification version 4.2 volume 3 part A section 3.3.5.
const uint16_t L2capSdu::lfsr_table_[256] = {
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241, 0xc601,
    0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440, 0xcc01, 0x0cc0,
    0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40, 0x0a00, 0xcac1, 0xcb81,
    0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841, 0xd801, 0x18c0, 0x1980, 0xd941,
    0x1b00, 0xdbc1, 0xda81, 0x1a40, 0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01,
    0x1dc0, 0x1c80, 0xdc41, 0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0,
    0x1680, 0xd641, 0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081,
    0x1040, 0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441, 0x3c00,
    0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41, 0xfa01, 0x3ac0,
    0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840, 0x2800, 0xe8c1, 0xe981,
    0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41, 0xee01, 0x2ec0, 0x2f80, 0xef41,
    0x2d00, 0xedc1, 0xec81, 0x2c40, 0xe401, 0x24c0, 0x2580, 0xe541, 0x2700,
    0xe7c1, 0xe681, 0x2640, 0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0,
    0x2080, 0xe041, 0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281,
    0x6240, 0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41, 0xaa01,
    0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840, 0x7800, 0xb8c1,
    0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41, 0xbe01, 0x7ec0, 0x7f80,
    0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40, 0xb401, 0x74c0, 0x7580, 0xb541,
    0x7700, 0xb7c1, 0xb681, 0x7640, 0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101,
    0x71c0, 0x7080, 0xb041, 0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0,
    0x5280, 0x9241, 0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481,
    0x5440, 0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841, 0x8801,
    0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40, 0x4e00, 0x8ec1,
    0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41, 0x4400, 0x84c1, 0x8581,
    0x4540, 0x8701, 0x47c0, 0x4680, 0x8641, 0x8201, 0x42c0, 0x4380, 0x8341,
    0x4100, 0x81c1, 0x8081, 0x4040,
};  // lfsr_table

L2capSdu::L2capSdu(std::vector<uint8_t>&& create_from) {
  sdu_data_ = create_from;
}

std::shared_ptr<L2capSdu> L2capSdu::L2capSduConstructor(
    std::vector<uint8_t> create_from) {
  L2capSdu packet(std::move(create_from));

  return std::make_shared<L2capSdu>(packet);
}

std::shared_ptr<L2capSdu> L2capSdu::L2capSduBuilder(
    std::vector<uint8_t> create_from) {
  L2capSdu packet(std::move(create_from));

  packet.sdu_data_.resize(packet.sdu_data_.size() + 2, 0x00);

  uint16_t fcs = packet.calculate_fcs();

  packet.sdu_data_[packet.sdu_data_.size() - 2] = fcs & 0xFF;
  packet.sdu_data_[packet.sdu_data_.size() - 1] = (fcs & 0xFF00) >> 8;

  return std::make_shared<L2capSdu>(packet);
}

uint16_t L2capSdu::convert_from_little_endian(
    const unsigned int starting_index) const {
  uint16_t convert = sdu_data_[starting_index + 1];
  convert = convert << 8;
  convert = convert | sdu_data_[starting_index];

  return convert;
}

uint16_t L2capSdu::calculate_fcs() const {
  // Starting state as directed by Bluetooth specification version 4.2 volume 3
  // part A 3.3.5.
  uint16_t lfsr = 0x0000;

  for (size_t i = 0; i < sdu_data_.size() - 2; i++) {
    uint8_t current_byte = sdu_data_[i];
    lfsr = ((lfsr >> 8) & 0xff) ^ lfsr_table_[(lfsr & 0xff) ^ current_byte];
  }
  return lfsr;
}

uint16_t L2capSdu::get_payload_length() const {
  return convert_from_little_endian(0);
}

uint16_t L2capSdu::get_fcs() const {
  return convert_from_little_endian(sdu_data_.size() - 2);
}

uint16_t L2capSdu::get_controls() const {
  return convert_from_little_endian(4);
}

uint16_t L2capSdu::get_total_l2cap_length() const {
  return convert_from_little_endian(6);
}

uint16_t L2capSdu::get_channel_id() const {
  return convert_from_little_endian(2);
}

size_t L2capSdu::get_vector_size() const { return sdu_data_.size(); }

bool L2capSdu::is_complete_l2cap(const L2capSdu& sdu) {
  uint16_t sar_bits = (sdu.get_controls() & kSduSarBits);

  return (sar_bits == 0x0000);
}

bool L2capSdu::is_starting_sdu(const L2capSdu& sdu) {
  uint16_t sar_bits = (sdu.get_controls() & kSduSarBits);

  return (sar_bits == 0x4000);
}

bool L2capSdu::is_ending_sdu(const L2capSdu& sdu) {
  uint16_t sar_bits = (sdu.get_controls() & kSduSarBits);

  return (sar_bits == 0x8000);
}

// HciPacket functions.
size_t L2capSdu::get_length() { return sdu_data_.size(); }
uint8_t& L2capSdu::get_at_index(size_t index) { return sdu_data_[index]; }

}  // namespace test_vendor_lib
