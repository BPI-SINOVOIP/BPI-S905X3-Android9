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
#include <vector>

#include "base/logging.h"
#include "bt_address.h"
#include "packet.h"

namespace test_vendor_lib {

// Event Packets are specified in the Bluetooth Core Specification Version 4.2,
// Volume 2, Part E, Section 5.4.4
class EventPacket : public Packet {
 public:
  virtual ~EventPacket() override = default;

  uint8_t GetEventCode() const;

  // Static functions for creating event packets:

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.1
  static std::unique_ptr<EventPacket> CreateInquiryCompleteEvent(
      uint8_t status);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.14
  // This should only be used for testing to send non-standard packets
  // Most code should use the more specific functions that follow
  static std::unique_ptr<EventPacket> CreateCommandCompleteEvent(
      uint16_t command_opcode,
      const std::vector<uint8_t>& event_return_parameters);

  static std::unique_ptr<EventPacket> CreateCommandCompleteOnlyStatusEvent(
      uint16_t command_opcode, uint8_t status);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.15
  static std::unique_ptr<EventPacket> CreateCommandStatusEvent(
      uint8_t status, uint16_t command_opcode);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.19
  static std::unique_ptr<EventPacket> CreateNumberOfCompletedPacketsEvent(
      uint16_t handle, uint16_t num_completed_packets);

  void AddCompletedPackets(uint16_t handle, uint16_t num_completed_packets);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.3.10
  static std::unique_ptr<EventPacket> CreateCommandCompleteDeleteStoredLinkKey(
      uint8_t status, uint16_t num_keys_deleted);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.3.12
  static std::unique_ptr<EventPacket> CreateCommandCompleteReadLocalName(
      uint8_t status, const std::string& local_name);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.4.1
  static std::unique_ptr<EventPacket>
  CreateCommandCompleteReadLocalVersionInformation(uint8_t status,
                                                   uint8_t hci_version,
                                                   uint16_t hci_revision,
                                                   uint8_t lmp_pal_version,
                                                   uint16_t manufacturer_name,
                                                   uint16_t lmp_pal_subversion);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.4.2
  static std::unique_ptr<EventPacket>
  CreateCommandCompleteReadLocalSupportedCommands(
      uint8_t status, const std::vector<uint8_t>& supported_commands);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.4.4
  static std::unique_ptr<EventPacket>
  CreateCommandCompleteReadLocalExtendedFeatures(
      uint8_t status, uint8_t page_number, uint8_t maximum_page_number,
      uint64_t extended_lmp_features);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.4.5
  static std::unique_ptr<EventPacket> CreateCommandCompleteReadBufferSize(
      uint8_t status, uint16_t hc_acl_data_packet_length,
      uint8_t hc_synchronous_data_packet_length,
      uint16_t hc_total_num_acl_data_packets,
      uint16_t hc_total_synchronous_data_packets);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.4.6
  static std::unique_ptr<EventPacket> CreateCommandCompleteReadBdAddr(
      uint8_t status, const BtAddress& bt_address);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.4.8
  static std::unique_ptr<EventPacket>
  CreateCommandCompleteReadLocalSupportedCodecs(
      uint8_t status, const std::vector<uint8_t>& supported_codecs,
      const std::vector<uint32_t>& vendor_specific_codecs);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.6.1
  static std::unique_ptr<EventPacket> CreateCommandCompleteReadLoopbackMode(
      uint8_t status, uint8_t mode);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.2
  static std::unique_ptr<EventPacket> CreateInquiryResultEvent();

  // Returns true if the result can be added to the event packet.
  bool AddInquiryResult(const BtAddress& bt_address,
                        uint8_t page_scan_repetition_mode,
                        uint32_t class_of_device, uint16_t clock_offset);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.3
  static std::unique_ptr<EventPacket> CreateConnectionCompleteEvent(
      uint8_t status, uint16_t handle, const BtAddress& address,
      uint8_t link_type, bool encryption_enabled);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.25
  static std::unique_ptr<EventPacket> CreateLoopbackCommandEvent(
      uint16_t opcode, const std::vector<uint8_t>& payload);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.7.38
  static std::unique_ptr<EventPacket> CreateExtendedInquiryResultEvent(
      const BtAddress& bt_address, uint8_t page_scan_repetition_mode,
      uint32_t class_of_device, uint16_t clock_offset, uint8_t rssi,
      const std::vector<uint8_t>& extended_inquiry_response);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section
  // 7.7.65.1
  static std::unique_ptr<EventPacket> CreateLeConnectionCompleteEvent(
      uint8_t status, uint16_t handle, uint8_t role, uint8_t peer_address_type,
      const BtAddress& peer, uint16_t interval, uint16_t latency,
      uint16_t supervision_timeout);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section
  // 7.7.65.2
  static std::unique_ptr<EventPacket> CreateLeAdvertisingReportEvent();

  // Returns true if the report can be added to the event packet.
  bool AddLeAdvertisingReport(uint8_t event_type, uint8_t addr_type,
                              const BtAddress& addr,
                              const std::vector<uint8_t>& data, uint8_t rssi);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section
  // 7.7.65.4
  static std::unique_ptr<EventPacket> CreateLeRemoteUsedFeaturesEvent(
      uint8_t status, uint16_t handle, uint64_t features);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.2
  static std::unique_ptr<EventPacket> CreateCommandCompleteLeReadBufferSize(
      uint8_t status, uint16_t hc_le_data_packet_length,
      uint8_t hc_total_num_le_data_packets);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.3
  static std::unique_ptr<EventPacket>
  CreateCommandCompleteLeReadLocalSupportedFeatures(uint8_t status,
                                                    uint64_t le_features);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.14
  static std::unique_ptr<EventPacket> CreateCommandCompleteLeReadWhiteListSize(
      uint8_t status, uint8_t white_list_size);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.23
  static std::unique_ptr<EventPacket> CreateCommandCompleteLeRand(
      uint8_t status, uint64_t random_val);

  // Bluetooth Core Specification Version 4.2, Volume 2, Part E, Section 7.8.27
  static std::unique_ptr<EventPacket>
  CreateCommandCompleteLeReadSupportedStates(uint8_t status,
                                             uint64_t le_states);

  // Vendor-specific commands (see hcidefs.h)

  static std::unique_ptr<EventPacket> CreateCommandCompleteLeVendorCap(
      uint8_t status, const std::vector<uint8_t>& vendor_cap);

  // Size of a data packet header, which consists of a 1 octet event code
  static const size_t kEventHeaderSize = 1;

 private:
  explicit EventPacket(uint8_t event_code);
  EventPacket(uint8_t event_code, const std::vector<uint8_t>& payload);
};

}  // namespace test_vendor_lib
