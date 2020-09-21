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

#pragma once

#include <cstdint>
#include <vector>

#include "bt_address.h"
#include "hci/include/hci_hal.h"

namespace test_vendor_lib {

const size_t kReservedZero = 0;

// Abstract base class that is subclassed to provide type-specifc accessors on
// data. Manages said data's memory and guarantees the data's persistence for IO
// operations.
class Packet {
 public:
  virtual ~Packet() = default;

  // Returns the size in octets of the entire packet, which consists of the type
  // octet, the header, and the payload.
  size_t GetPacketSize() const;

  const std::vector<uint8_t>& GetPayload() const;

  size_t GetPayloadSize() const;

  const std::vector<uint8_t>& GetHeader() const;

  uint8_t GetHeaderSize() const;

  serial_data_type_t GetType() const;

  // Add |octets| bytes to the payload.  Return true if:
  // - the size of |bytes| is equal to |octets| and
  // - the new size of the payload is still < |kMaxPayloadOctets|
  bool AddPayloadOctets(size_t octets, const std::vector<uint8_t>& bytes);

 private:
  // Add |octets| bytes to the payload.  Return true if:
  // - the value of |value| fits in |octets| bytes and
  // - the new size of the payload is still < |kMaxPayloadOctets|
  bool AddPayloadOctets(size_t octets, uint64_t value);

 public:
  // Add type-checking versions
  bool AddPayloadOctets1(uint8_t value) { return AddPayloadOctets(1, value); }
  bool AddPayloadOctets2(uint16_t value) { return AddPayloadOctets(2, value); }
  bool AddPayloadOctets3(uint32_t value) { return AddPayloadOctets(3, value); }
  bool AddPayloadOctets4(uint32_t value) { return AddPayloadOctets(4, value); }
  bool AddPayloadOctets6(uint64_t value) { return AddPayloadOctets(6, value); }
  bool AddPayloadOctets8(uint64_t value) { return AddPayloadOctets(8, value); }

  // Add |address| to the payload.  Return true if:
  // - the new size of the payload is still < |kMaxPayloadOctets|
  bool AddPayloadBtAddress(const BtAddress& address);

  // Return true if |num_bytes| can be added to the payload.
  bool CanAddPayloadOctets(size_t num_bytes) const {
    return GetPayloadSize() + num_bytes <= kMaxPayloadOctets;
  }

 protected:
  // Constructs an empty packet of type |type| and header |header|
  Packet(serial_data_type_t type, std::vector<uint8_t> header);

  bool IncrementPayloadCounter(size_t index);
  bool IncrementPayloadCounter(size_t index, uint8_t max_val);

 private:
  const size_t kMaxPayloadOctets = 256;  // Includes the size byte.

  // Underlying containers for storing the actual packet

  // The packet type is one of DATA_TYPE_ACL, DATA_TYPE_COMMAND,
  // DATA_TYPE_EVENT, or DATA_TYPE_SCO.
  serial_data_type_t type_;

  std::vector<uint8_t> header_;

  std::vector<uint8_t> payload_;
};

}  // namespace test_vendor_lib
