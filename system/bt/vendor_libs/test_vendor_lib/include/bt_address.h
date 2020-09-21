/*
 * Copyright 2016 The Android Open Source Project
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

// Encapsulate handling for Bluetooth Addresses:
// - store the address
// - convert to/from strings and vectors of bytes
// - check strings to see if they represent a valid address
class BtAddress {
 public:
  // Conversion constants
  static const size_t kStringLength = 17;  // "XX:XX:XX:XX:XX:XX"
  static const size_t kOctets = 6;         // "X0:X1:X2:X3:X4:X5"

  BtAddress() : address_(0){};
  virtual ~BtAddress() = default;

  // Returns true if |addr| has the form "XX:XX:XX:XX:XX:XX":
  // - the length of |addr| is >= kStringLength
  // - every third character is ':'
  // - each remaining character is a hexadecimal digit
  static bool IsValid(const std::string& addr);

  inline bool operator==(const BtAddress& right) const {
    return address_ == right.address_;
  }
  inline bool operator!=(const BtAddress& right) const {
    return address_ != right.address_;
  }
  inline bool operator<(const BtAddress& right) const {
    return address_ < right.address_;
  }
  inline bool operator>(const BtAddress& right) const {
    return address_ > right.address_;
  }
  inline bool operator<=(const BtAddress& right) const {
    return address_ <= right.address_;
  }
  inline bool operator>=(const BtAddress& right) const {
    return address_ >= right.address_;
  }

  inline void operator=(const BtAddress& right) { address_ = right.address_; }
  inline void operator|=(const BtAddress& right) { address_ |= right.address_; }
  inline void operator&=(const BtAddress& right) { address_ &= right.address_; }

  // Set the address to the address represented by |str|.
  // returns true if |str| represents a valid address, otherwise false.
  bool FromString(const std::string& str);

  // Set the address to the address represented by |str|.
  // returns true if octets.size() >= kOctets, otherwise false.
  bool FromVector(const std::vector<uint8_t>& octets);

  // Appends the Bluetooth address to the vector |octets|.
  void ToVector(std::vector<uint8_t>& octets) const;

  // Return a string representation of the Bluetooth address, in this format:
  // "xx:xx:xx:xx:xx:xx", where x represents a lowercase hexadecimal digit.
  std::string ToString() const;

 private:
  // The Bluetooth Address is stored in the lower 48 bits of a 64-bit value
  uint64_t address_;
};

}  // namespace test_vendor_lib
