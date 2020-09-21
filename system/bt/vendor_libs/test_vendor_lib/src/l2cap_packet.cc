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

#define LOG_TAG "l2cap_packet"

#include "l2cap_packet.h"

#include <algorithm>

#include "osi/include/log.h"

namespace test_vendor_lib {

const size_t kL2capHeaderLength = 4;
const size_t kSduFirstHeaderLength = 8;
const size_t kSduStandardHeaderLength = 6;
const size_t kFcsBytes = 2;
const uint16_t kSduTxSeqBits = 0x007E;
const uint8_t kSduFirstReqseq = 0x40;
const uint8_t kSduContinuationReqseq = 0xC0;
const uint8_t kSduEndReqseq = 0x80;

std::shared_ptr<L2capPacket> L2capPacket::assemble(
    const std::vector<std::shared_ptr<L2capSdu> >& sdu_packets) {
  std::shared_ptr<L2capPacket> built_l2cap_packet(new L2capPacket());
  uint16_t l2cap_payload_length = 0;
  uint16_t first_packet_channel_id = 0;
  uint16_t total_expected_l2cap_length;
  uint8_t txseq_start;

  if (sdu_packets.size() == 0) {
    LOG_DEBUG(LOG_TAG, "%s: No SDU received.", __func__);
    return nullptr;
  }
  if (sdu_packets.size() == 1 &&
      !L2capSdu::is_complete_l2cap(*sdu_packets[0])) {
    LOG_DEBUG(LOG_TAG, "%s: Unsegmented SDU has incorrect SAR bits.", __func__);
    return nullptr;
  }

  first_packet_channel_id = sdu_packets[0]->get_channel_id();

  built_l2cap_packet->l2cap_packet_.resize(kL2capHeaderLength);

  for (size_t i = 0; i < sdu_packets.size(); i++) {
    uint16_t payload_length = sdu_packets[i]->get_payload_length();

    // TODO(jruthe): Remove these checks when ACL packets have been
    // implemented. Once those are done, that will be the only way to create
    // L2capSdu objects and these checks will be moved there instead.
    //
    // Check the integrity of the packet length, if it is zero, it is invalid.
    // The maximum size of a single, segmented L2CAP payload is 1016 bytes.
    if ((payload_length <= 0) ||
        (payload_length != sdu_packets[i]->get_vector_size() - 4)) {
      LOG_DEBUG(LOG_TAG, "%s: SDU payload length incorrect.", __func__);
      return nullptr;
    }

    uint16_t fcs_check = sdu_packets[i]->get_fcs();

    if (sdu_packets[i]->calculate_fcs() != fcs_check) {
      LOG_DEBUG(LOG_TAG, "%s: SDU fcs is incorrect.", __func__);
      return nullptr;
    }

    uint16_t controls = sdu_packets[i]->get_controls();

    if (sdu_packets[i]->get_channel_id() != first_packet_channel_id) {
      LOG_DEBUG(LOG_TAG, "%s: SDU CID does not match expected.", __func__);
      return nullptr;
    }

    if (i == 0) txseq_start = controls & kSduTxSeqBits;

    // Bluetooth Specification version 4.2 volume 3 part A 3.3.2:
    // If there is only a single SDU, the first two bits of the control must be
    // set to 00b, representing an unsegmented SDU. If the SDU is segmented,
    // there is a begin and an end. The first segment must have the first two
    // control bits set to 01b and the ending segment must have them set to 10b.
    // Meanwhile all segments in between the start and end must have the bits
    // set to 11b.
    size_t starting_index;
    uint8_t txseq = controls & kSduTxSeqBits;
    if (sdu_packets.size() > 1 && i == 0 &&
        !L2capSdu::is_starting_sdu(*sdu_packets[i])) {
      LOG_DEBUG(LOG_TAG, "%s: First segmented SDU has incorrect SAR bits.",
                __func__);
      return nullptr;
    }
    if (i != 0 && L2capSdu::is_starting_sdu(*sdu_packets[i])) {
      LOG_DEBUG(LOG_TAG,
                "%s: SAR bits set to first segmented SDU on "
                "non-starting SDU.",
                __func__);
      return nullptr;
    }
    if (txseq != (txseq_start + (static_cast<uint8_t>(i) << 1))) {
      LOG_DEBUG(LOG_TAG, "%s: TxSeq incorrect.", __func__);
      return nullptr;
    }
    if (sdu_packets.size() > 1 && i == sdu_packets.size() - 1 &&
        !L2capSdu::is_ending_sdu(*sdu_packets[i])) {
      LOG_DEBUG(LOG_TAG, "%s: Final segmented SDU has incorrect SAR bits.",
                __func__);
      return nullptr;
    }

    // Subtract the control and fcs from every SDU payload length.
    l2cap_payload_length += (payload_length - 4);

    if (L2capSdu::is_starting_sdu(*sdu_packets[i])) {
      starting_index = kSduFirstHeaderLength;
      total_expected_l2cap_length = sdu_packets[i]->get_total_l2cap_length();

      // Subtract the additional two bytes from the first packet of a segmented
      // SDU.
      l2cap_payload_length -= 2;
    } else {
      starting_index = kSduStandardHeaderLength;
    }

    Iterator payload_begin = sdu_packets[i]->get_begin() + starting_index;
    Iterator payload_end = sdu_packets[i]->get_end() - kFcsBytes;

    built_l2cap_packet->l2cap_packet_.insert(
        built_l2cap_packet->l2cap_packet_.end(), payload_begin, payload_end);
  }

  if (l2cap_payload_length != total_expected_l2cap_length &&
      sdu_packets.size() > 1) {
    LOG_DEBUG(LOG_TAG, "%s: Total expected L2CAP payload length incorrect.",
              __func__);
    return nullptr;
  }

  built_l2cap_packet->l2cap_packet_[0] = l2cap_payload_length & 0xFF;
  built_l2cap_packet->l2cap_packet_[1] = (l2cap_payload_length & 0xFF00) >> 8;
  built_l2cap_packet->l2cap_packet_[2] = first_packet_channel_id & 0xFF;
  built_l2cap_packet->l2cap_packet_[3] =
      (first_packet_channel_id & 0xFF00) >> 8;

  return built_l2cap_packet;
}  // Assemble

uint16_t L2capPacket::get_l2cap_cid() const {
  return ((l2cap_packet_[3] << 8) | l2cap_packet_[2]);
}

std::vector<std::shared_ptr<L2capSdu> > L2capPacket::fragment(
    uint16_t maximum_sdu_size, uint8_t txseq, uint8_t reqseq) {
  std::vector<std::shared_ptr<L2capSdu> > sdu;
  if (!check_l2cap_packet()) return sdu;

  std::vector<uint8_t> current_sdu;

  Iterator current_iter = get_begin() + kL2capHeaderLength;
  Iterator end_iter = get_end();

  size_t number_of_packets = ceil((l2cap_packet_.size() - kL2capHeaderLength) /
                                  static_cast<float>(maximum_sdu_size));

  if (number_of_packets == 0) {
    current_sdu.resize(kSduStandardHeaderLength);

    set_sdu_header_length(current_sdu, kL2capHeaderLength);

    set_sdu_cid(current_sdu, get_l2cap_cid());

    reqseq = (reqseq & 0xF0) >> 4;
    set_sdu_control_bytes(current_sdu, txseq, reqseq);

    sdu.push_back(L2capSdu::L2capSduBuilder(current_sdu));

    return sdu;
  }

  uint16_t header_length = 0x0000;

  if (number_of_packets == 1) {
    current_sdu.clear();
    current_sdu.resize(kSduStandardHeaderLength);

    header_length = ((l2cap_packet_[1] & 0xFF) << 8) | l2cap_packet_[0];
    header_length += kL2capHeaderLength;
    set_sdu_header_length(current_sdu, header_length);

    set_sdu_cid(current_sdu, get_l2cap_cid());

    set_sdu_control_bytes(current_sdu, txseq, 0x00);

    current_sdu.insert(current_sdu.end(), current_iter, end_iter);

    sdu.push_back(L2capSdu::L2capSduBuilder(current_sdu));

    return sdu;
  }

  Iterator next_iter =
      current_iter + (maximum_sdu_size - (kSduFirstHeaderLength + 2));

  sdu.reserve(number_of_packets);
  sdu.clear();

  for (size_t i = 0; i <= number_of_packets; i++) {
    if (i == 0) {
      current_sdu.resize(kSduFirstHeaderLength);

      header_length = maximum_sdu_size - kL2capHeaderLength;

      reqseq = reqseq | kSduFirstReqseq;

      set_total_sdu_length(current_sdu, l2cap_packet_.size() - 4);
    } else {
      current_sdu.resize(kSduStandardHeaderLength);

      header_length = (next_iter - current_iter) + kL2capHeaderLength;

      reqseq = reqseq & 0x0F;

      if (i < number_of_packets) {
        reqseq |= kSduContinuationReqseq;
      } else {
        reqseq |= kSduEndReqseq;
      }
    }
    set_sdu_header_length(current_sdu, header_length);

    set_sdu_cid(current_sdu, get_l2cap_cid());

    set_sdu_control_bytes(current_sdu, txseq, reqseq);
    txseq += 2;

    // Txseq has a maximum of 0x3F. If it exceeds that, it restarts at 0x00.
    if (txseq > 0x3F) txseq = 0x00;

    current_sdu.insert(current_sdu.end(), current_iter, next_iter);
    current_iter = next_iter;

    next_iter = current_iter + (maximum_sdu_size - kSduFirstHeaderLength);

    sdu.push_back(L2capSdu::L2capSduBuilder(std::move(current_sdu)));
  }

  return sdu;
}  // fragment

void L2capPacket::set_sdu_header_length(std::vector<uint8_t>& sdu,
                                        uint16_t length) {
  sdu[0] = length & 0xFF;
  sdu[1] = (length & 0xFF00) >> 8;
}

void L2capPacket::set_total_sdu_length(std::vector<uint8_t>& sdu,
                                       uint16_t total_sdu_length) {
  sdu[6] = total_sdu_length & 0xFF;
  sdu[7] = (total_sdu_length & 0xFF00) >> 8;
}

void L2capPacket::set_sdu_cid(std::vector<uint8_t>& sdu, uint16_t cid) {
  sdu[2] = cid & 0xFF;
  sdu[3] = (cid & 0xFF00) >> 8;
}

void L2capPacket::set_sdu_control_bytes(std::vector<uint8_t>& sdu,
                                        uint8_t txseq, uint8_t reqseq) {
  sdu[4] = txseq;
  sdu[5] = reqseq;
}

bool L2capPacket::check_l2cap_packet() const {
  uint16_t payload_length = ((l2cap_packet_[1] & 0xFF) << 8) | l2cap_packet_[0];

  if (l2cap_packet_.size() < 4) return false;
  if (payload_length != (l2cap_packet_.size() - 4)) return false;

  return true;
}

// HciPacket Functions
size_t L2capPacket::get_length() { return l2cap_packet_.size(); }

uint8_t& L2capPacket::get_at_index(size_t index) {
  return l2cap_packet_[index];
}

}  // namespace test_vendor_lib
