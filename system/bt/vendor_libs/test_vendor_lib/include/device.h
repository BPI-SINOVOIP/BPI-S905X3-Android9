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

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "bt_address.h"

#include "hci/include/hci_hal.h"
#include "stack/include/btm_ble_api.h"

namespace test_vendor_lib {

// Represent a Bluetooth Device
//  - Provide Get*() and Set*() functions for device attributes.
class Device {
 public:
  Device() : time_stamp_(std::chrono::steady_clock::now()) {}
  virtual ~Device() = default;

  // Initialize the device based on the values of |args|.
  virtual void Initialize(const std::vector<std::string>& args) = 0;

  // Return a string representation of the type of device.
  virtual std::string GetTypeString() const = 0;

  // Return the string representation of the device.
  virtual std::string ToString() const;

  // Return a reference to the address.
  const BtAddress& GetBtAddress() const { return address_; }

  // Set the address to the |addr|.
  void SetBtAddress(const BtAddress& addr) { address_ = addr; }

  // Return the address type.
  uint8_t GetAddressType() const { return address_type_; }

  // Decide whether to accept a connection request
  // May need to be extended to check peer address & type, and other
  // connection parameters.
  // Return true if the device accepts the connection request.
  virtual bool LeConnect() { return false; }

  // Return the advertisement data.
  const std::vector<uint8_t>& GetAdvertisement() const { return adv_data_; }

  // Return the advertisement type.
  uint8_t GetAdvertisementType() const { return advertising_type_; }

  // Set the advertisement interval in milliseconds.
  void SetAdvertisementInterval(std::chrono::milliseconds ms) {
    advertising_interval_ms_ = ms;
  }

  // Return true if there is a scan response (allows for empty responses).
  bool HasScanResponse() const { return scan_response_present_; }

  // Return the scan response data.
  const std::vector<uint8_t>& GetScanResponse() const { return scan_data_; }

  // Returns true if the host could see an advertisement in the next
  // |scan_time| milliseconds.
  virtual bool IsAdvertisementAvailable(
      std::chrono::milliseconds scan_time) const;

  // Returns true if the host could see a page scan now.
  virtual bool IsPageScanAvailable() const;

  // Return the device class.
  // The device class is a 3-byte value.  Look for DEV_CLASS in
  // stack/include/bt_types.h
  uint32_t GetDeviceClass() const { return device_class_; }

  // Return the clock offset, which is a defined in the Spec as:
  // (CLKN_16-2 slave - CLKN_16-2 master ) mod 2**15.
  // Bluetooth Core Specification Version 4.2, Volume 2, Part C, Section 4.3.2
  uint16_t GetClockOffset() const { return clock_offset_; }

  // Set the clock offset.
  void SetClockOffset(uint16_t offset) { clock_offset_ = offset; }

  // Return the page scan repetition mode.
  // Bluetooth Core Specification Version 4.2, Volume 2, Part B, Section 8.3.1
  // The values are:
  // 0 - R0 T_page_scan <= 1.28s and T_page_scan == T_window and
  // 1 - R1 T_page_scan <= 1.28s
  // 2 - R2 T_page_scan <= 2.56s
  uint8_t GetPageScanRepetitionMode() const {
    return page_scan_repetition_mode_;
  }

  // Return the extended inquiry data.
  const std::vector<uint8_t>& GetExtendedInquiryData() const {
    return extended_inquiry_data_;
  }

  // Let the device know that time has passed.
  virtual void TimerTick() {}

 protected:
  BtAddress address_;

  // Address type is defined in the spec:
  // 0x00 Public Device Address
  // 0x01 Random Device Address
  // 0x02 Public Identity Address
  // 0x03 Random (static) Identity Address
  // 0x04 – 0xFF Reserved for future use
  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.12
  uint8_t address_type_;

  std::chrono::steady_clock::time_point time_stamp_;

  // Return the device class.
  // The device class is a 3-byte value.  Look for DEV_CLASS in
  // stack/include/bt_types.h
  uint32_t device_class_;

  // Return the page scan repetition mode.
  // Bluetooth Core Specification Version 4.2, Volume 2, Part B, Section 8.3.1
  // The values are:
  // 0 - R0 T_page_scan <= 1.28s and T_page_scan == T_window and
  // 1 - R1 T_page_scan <= 1.28s
  // 2 - R2 T_page_scan <= 2.56s
  uint8_t page_scan_repetition_mode_;

  // The time between page scans.
  std::chrono::milliseconds page_scan_delay_ms_;

  std::vector<uint8_t> extended_inquiry_data_;

  // Classic Bluetooth CLKN_slave[16..2] - CLKN_master[16..2]
  // Bluetooth Core Specification Version 4.2, Volume 2, Part C, Section 4.3.2
  uint16_t clock_offset_;

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.5
  uint8_t advertising_type_;

  // The spec defines the advertising interval as a 16-bit value, but since it
  // is never sent in packets, we use std::chrono::milliseconds.
  std::chrono::milliseconds advertising_interval_ms_;

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.7

  // Bluetooth Core Specification Version 4.2, Volume 3, Part C, Section
  // 11.1
  // https://www.bluetooth.com/specifications/assigned-numbers
  // Supplement to Bluetooth Core Specification | CSSv6, Part A
  std::vector<uint8_t> adv_data_ = {0x07,  // Length
                                    BTM_BLE_AD_TYPE_NAME_CMPL,
                                    'd',
                                    'e',
                                    'v',
                                    'i',
                                    'c',
                                    'e'};

  bool scan_response_present_ = true;
  std::vector<uint8_t> scan_data_ = {0x04,  // Length
                                     BTM_BLE_AD_TYPE_NAME_SHORT, 'd', 'e', 'v'};

 public:
  static const uint8_t kBtAddressTypePublic = 0x00;
  static const uint8_t kBtAddressTypeRandom = 0x01;
  static const uint8_t kBtAddressTypePublicIdentity = 0x02;
  static const uint8_t kBtAddressTypeRandomIdentity = 0x03;
};

}  // namespace test_vendor_lib
