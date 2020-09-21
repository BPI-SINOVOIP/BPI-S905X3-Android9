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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "acl_packet.h"
#include "async_manager.h"
#include "base/json/json_value_converter.h"
#include "base/time/time.h"
#include "bt_address.h"

namespace test_vendor_lib {

// Emulates a dual mode BR/EDR + LE controller by maintaining the link layer
// state machine detailed in the Bluetooth Core Specification Version 4.2,
// Volume 6, Part B, Section 1.1 (page 30). Provides methods corresponding to
// commands sent by the HCI. These methods will be registered as callbacks from
// a controller instance with the HciHandler. To implement a new Bluetooth
// command, simply add the method declaration below, with return type void and a
// single const std::vector<uint8_t>& argument. After implementing the
// method, simply register it with the HciHandler using the SET_HANDLER macro in
// the controller's default constructor. Be sure to name your method after the
// corresponding Bluetooth command in the Core Specification with the prefix
// "Hci" to distinguish it as a controller command.
class DeviceProperties {
 public:
  explicit DeviceProperties(const std::string& file_name);

  // Access private configuration data

  // Specification Version 4.2, Volume 2, Part E, Section 7.4.1
  const std::vector<uint8_t>& GetLocalVersionInformation() const;

  // Specification Version 4.2, Volume 2, Part E, Section 7.4.2
  const std::vector<uint8_t>& GetLocalSupportedCommands() const {
    return local_supported_commands_;
  }

  // Specification Version 4.2, Volume 2, Part E, Section 7.4.3
  uint64_t GetLocalSupportedFeatures() const {
    return local_extended_features_[0];
  };

  // Specification Version 4.2, Volume 2, Part E, Section 7.4.4
  uint8_t GetLocalExtendedFeaturesMaximumPageNumber() const {
    return local_extended_features_.size() - 1;
  };

  uint64_t GetLocalExtendedFeatures(uint8_t page_number) const {
    CHECK(page_number < local_extended_features_.size());
    return local_extended_features_[page_number];
  };

  // Specification Version 4.2, Volume 2, Part E, Section 7.4.5
  uint16_t GetAclDataPacketSize() const { return acl_data_packet_size_; }

  uint8_t GetSynchronousDataPacketSize() const { return sco_data_packet_size_; }

  uint16_t GetTotalNumAclDataPackets() const { return num_acl_data_packets_; }

  uint16_t GetTotalNumSynchronousDataPackets() const {
    return num_sco_data_packets_;
  }

  const BtAddress& GetAddress() const { return address_; }

  // Specification Version 4.2, Volume 2, Part E, Section 7.4.8
  const std::vector<uint8_t>& GetSupportedCodecs() const {
    return supported_codecs_;
  }

  const std::vector<uint32_t>& GetVendorSpecificCodecs() const {
    return vendor_specific_codecs_;
  }

  const std::string& GetLocalName() const { return local_name_; }

  uint8_t GetVersion() const { return version_; }

  uint16_t GetRevision() const { return revision_; }

  uint8_t GetLmpPalVersion() const { return lmp_pal_version_; }

  uint16_t GetLmpPalSubversion() const { return lmp_pal_subversion_; }

  uint16_t GetManufacturerName() const { return manufacturer_name_; }

  // Specification Version 4.2, Volume 2, Part E, Section 7.8.2
  uint16_t GetLeDataPacketLength() const { return le_data_packet_length_; }

  uint8_t GetTotalNumLeDataPackets() const { return num_le_data_packets_; }

  // Specification Version 4.2, Volume 2, Part E, Section 7.8.3
  uint64_t GetLeLocalSupportedFeatures() const {
    return le_supported_features_;
  }

  // Specification Version 4.2, Volume 2, Part E, Section 7.8.14
  uint8_t GetLeWhiteListSize() const { return le_white_list_size_; }

  // Specification Version 4.2, Volume 2, Part E, Section 7.8.27
  uint64_t GetLeSupportedStates() const { return le_supported_states_; }

  // Vendor-specific commands (see hcidefs.h)
  const std::vector<uint8_t>& GetLeVendorCap() const { return le_vendor_cap_; }

  static void RegisterJSONConverter(
      base::JSONValueConverter<DeviceProperties>* converter);

 private:
  uint16_t acl_data_packet_size_;
  uint8_t sco_data_packet_size_;
  uint16_t num_acl_data_packets_;
  uint16_t num_sco_data_packets_;
  uint8_t version_;
  uint16_t revision_;
  uint8_t lmp_pal_version_;
  uint16_t manufacturer_name_;
  uint16_t lmp_pal_subversion_;
  std::vector<uint8_t> supported_codecs_;
  std::vector<uint32_t> vendor_specific_codecs_;
  std::vector<uint8_t> local_supported_commands_;
  std::string local_name_;
  std::vector<uint64_t> local_extended_features_;
  BtAddress address_;

  uint16_t le_data_packet_length_;
  uint8_t num_le_data_packets_;
  uint8_t le_white_list_size_;
  uint64_t le_supported_features_;
  uint64_t le_supported_states_;
  std::vector<uint8_t> le_vendor_cap_;
};

}  // namespace test_vendor_lib
