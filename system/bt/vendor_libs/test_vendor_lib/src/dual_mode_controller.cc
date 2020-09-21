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

#define LOG_TAG "dual_mode_controller"

#include "dual_mode_controller.h"
#include "device_factory.h"

#include <memory>

#include <base/logging.h>
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/values.h"

#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "stack/include/hcidefs.h"

using std::vector;

namespace {

// Included in certain events to indicate success (specific to the event
// context).
const uint8_t kSuccessStatus = 0;

const uint8_t kUnknownHciCommand = 1;

// The location of the config file loaded to populate controller attributes.
const std::string kControllerPropertiesFile =
    "/etc/bluetooth/controller_properties.json";

void LogCommand(const char* command) {
  LOG_INFO(LOG_TAG, "Controller performing command: %s", command);
}

}  // namespace

namespace test_vendor_lib {

void DualModeController::AddControllerEvent(std::chrono::milliseconds delay,
                                            const TaskCallback& task) {
  controller_events_.push_back(schedule_task_(delay, task));
}

void DualModeController::AddConnectionAction(const TaskCallback& task,
                                             uint16_t handle) {
  for (size_t i = 0; i < connections_.size(); i++)
    if (*connections_[i] == handle) connections_[i]->AddAction(task);
}

void DualModeController::SendCommandCompleteSuccess(
    uint16_t command_opcode) const {
  send_event_(EventPacket::CreateCommandCompleteOnlyStatusEvent(
      command_opcode, kSuccessStatus));
}

void DualModeController::SendCommandCompleteOnlyStatus(uint16_t command_opcode,
                                                       uint8_t status) const {
  send_event_(EventPacket::CreateCommandCompleteOnlyStatusEvent(command_opcode,
                                                                status));
}

void DualModeController::SendCommandStatus(uint8_t status,
                                           uint16_t command_opcode) const {
  send_event_(EventPacket::CreateCommandStatusEvent(status, command_opcode));
}

void DualModeController::SendCommandStatusSuccess(
    uint16_t command_opcode) const {
  SendCommandStatus(kSuccessStatus, command_opcode);
}

DualModeController::DualModeController()
    : state_(kStandby), properties_(kControllerPropertiesFile) {
  devices_ = {};

  vector<std::string> beacon = {"beacon", "be:ac:10:00:00:01", "1000"};
  TestChannelAdd(beacon);

  vector<std::string> classic = {std::string("classic"),
                                 std::string("c1:a5:51:c0:00:01")};
  TestChannelAdd(classic);

  vector<std::string> keyboard = {std::string("keyboard"),
                                  std::string("cc:1c:eb:0a:12:d1"),
                                  std::string("500")};
  TestChannelAdd(keyboard);

  le_scan_enable_ = 0;
  le_connect_ = false;

  loopback_mode_ = 0;

#define SET_HANDLER(opcode, method)                                     \
  active_hci_commands_[opcode] = [this](const vector<uint8_t>& param) { \
    method(param);                                                      \
  };
  SET_HANDLER(HCI_RESET, HciReset);
  SET_HANDLER(HCI_READ_BUFFER_SIZE, HciReadBufferSize);
  SET_HANDLER(HCI_HOST_BUFFER_SIZE, HciHostBufferSize);
  SET_HANDLER(HCI_READ_LOCAL_VERSION_INFO, HciReadLocalVersionInformation);
  SET_HANDLER(HCI_READ_BD_ADDR, HciReadBdAddr);
  SET_HANDLER(HCI_READ_LOCAL_SUPPORTED_CMDS, HciReadLocalSupportedCommands);
  SET_HANDLER(HCI_READ_LOCAL_SUPPORTED_CODECS, HciReadLocalSupportedCodecs);
  SET_HANDLER(HCI_READ_LOCAL_EXT_FEATURES, HciReadLocalExtendedFeatures);
  SET_HANDLER(HCI_WRITE_SIMPLE_PAIRING_MODE, HciWriteSimplePairingMode);
  SET_HANDLER(HCI_WRITE_LE_HOST_SUPPORT, HciWriteLeHostSupport);
  SET_HANDLER(HCI_SET_EVENT_MASK, HciSetEventMask);
  SET_HANDLER(HCI_WRITE_INQUIRY_MODE, HciWriteInquiryMode);
  SET_HANDLER(HCI_WRITE_PAGESCAN_TYPE, HciWritePageScanType);
  SET_HANDLER(HCI_WRITE_INQSCAN_TYPE, HciWriteInquiryScanType);
  SET_HANDLER(HCI_WRITE_CLASS_OF_DEVICE, HciWriteClassOfDevice);
  SET_HANDLER(HCI_WRITE_PAGE_TOUT, HciWritePageTimeout);
  SET_HANDLER(HCI_WRITE_DEF_POLICY_SETTINGS, HciWriteDefaultLinkPolicySettings);
  SET_HANDLER(HCI_READ_LOCAL_NAME, HciReadLocalName);
  SET_HANDLER(HCI_CHANGE_LOCAL_NAME, HciWriteLocalName);
  SET_HANDLER(HCI_WRITE_EXT_INQ_RESPONSE, HciWriteExtendedInquiryResponse);
  SET_HANDLER(HCI_WRITE_VOICE_SETTINGS, HciWriteVoiceSetting);
  SET_HANDLER(HCI_WRITE_CURRENT_IAC_LAP, HciWriteCurrentIacLap);
  SET_HANDLER(HCI_WRITE_INQUIRYSCAN_CFG, HciWriteInquiryScanActivity);
  SET_HANDLER(HCI_WRITE_SCAN_ENABLE, HciWriteScanEnable);
  SET_HANDLER(HCI_SET_EVENT_FILTER, HciSetEventFilter);
  SET_HANDLER(HCI_INQUIRY, HciInquiry);
  SET_HANDLER(HCI_INQUIRY_CANCEL, HciInquiryCancel);
  SET_HANDLER(HCI_DELETE_STORED_LINK_KEY, HciDeleteStoredLinkKey);
  SET_HANDLER(HCI_RMT_NAME_REQUEST, HciRemoteNameRequest);
  SET_HANDLER(HCI_BLE_SET_EVENT_MASK, HciLeSetEventMask);
  SET_HANDLER(HCI_BLE_READ_BUFFER_SIZE, HciLeReadBufferSize);
  SET_HANDLER(HCI_BLE_READ_LOCAL_SPT_FEAT, HciLeReadLocalSupportedFeatures);
  SET_HANDLER(HCI_BLE_WRITE_RANDOM_ADDR, HciLeSetRandomAddress);
  SET_HANDLER(HCI_BLE_WRITE_ADV_DATA, HciLeSetAdvertisingData);
  SET_HANDLER(HCI_BLE_WRITE_ADV_PARAMS, HciLeSetAdvertisingParameters);
  SET_HANDLER(HCI_BLE_WRITE_SCAN_PARAMS, HciLeSetScanParameters);
  SET_HANDLER(HCI_BLE_WRITE_SCAN_ENABLE, HciLeSetScanEnable);
  SET_HANDLER(HCI_BLE_CREATE_LL_CONN, HciLeCreateConnection);
  SET_HANDLER(HCI_BLE_CREATE_CONN_CANCEL, HciLeConnectionCancel);
  SET_HANDLER(HCI_BLE_READ_WHITE_LIST_SIZE, HciLeReadWhiteListSize);
  SET_HANDLER(HCI_BLE_RAND, HciLeRand);
  SET_HANDLER(HCI_BLE_READ_SUPPORTED_STATES, HciLeReadSupportedStates);
  SET_HANDLER((HCI_GRP_VENDOR_SPECIFIC | 0x27), HciBleVendorSleepMode);
  SET_HANDLER(HCI_BLE_VENDOR_CAP_OCF, HciBleVendorCap);
  SET_HANDLER(HCI_BLE_MULTI_ADV_OCF, HciBleVendorMultiAdv);
  SET_HANDLER((HCI_GRP_VENDOR_SPECIFIC | 0x155), HciBleVendor155);
  SET_HANDLER(HCI_BLE_ADV_FILTER_OCF, HciBleAdvertisingFilter);
  SET_HANDLER(HCI_BLE_ENERGY_INFO_OCF, HciBleEnergyInfo);
  SET_HANDLER(HCI_BLE_EXTENDED_SCAN_PARAMS_OCF, HciBleExtendedScanParams);
  // Testing Commands
  SET_HANDLER(HCI_READ_LOOPBACK_MODE, HciReadLoopbackMode);
  SET_HANDLER(HCI_WRITE_LOOPBACK_MODE, HciWriteLoopbackMode);
#undef SET_HANDLER

#define SET_TEST_HANDLER(command_name, method)  \
  active_test_channel_commands_[command_name] = \
      [this](const vector<std::string>& param) { method(param); };
  SET_TEST_HANDLER("add", TestChannelAdd);
  SET_TEST_HANDLER("del", TestChannelDel);
  SET_TEST_HANDLER("list", TestChannelList);
#undef SET_TEST_HANDLER
}

void DualModeController::RegisterTaskScheduler(
    std::function<AsyncTaskId(std::chrono::milliseconds, const TaskCallback&)>
        oneshotScheduler) {
  schedule_task_ = oneshotScheduler;
}

void DualModeController::RegisterPeriodicTaskScheduler(
    std::function<AsyncTaskId(std::chrono::milliseconds,
                              std::chrono::milliseconds, const TaskCallback&)>
        periodicScheduler) {
  schedule_periodic_task_ = periodicScheduler;
}

void DualModeController::RegisterTaskCancel(
    std::function<void(AsyncTaskId)> task_cancel) {
  cancel_task_ = task_cancel;
}

void DualModeController::HandleTestChannelCommand(
    const std::string& name, const vector<std::string>& args) {
  if (active_test_channel_commands_.count(name) == 0) return;
  active_test_channel_commands_[name](args);
}

void DualModeController::HandleAcl(std::unique_ptr<AclPacket> acl_packet) {
  if (loopback_mode_ == HCI_LOOPBACK_MODE_LOCAL) {
    uint16_t channel = acl_packet->GetChannel();
    send_acl_(std::move(acl_packet));
    send_event_(EventPacket::CreateNumberOfCompletedPacketsEvent(channel, 1));
    return;
  }
}

void DualModeController::HandleSco(std::unique_ptr<ScoPacket> sco_packet) {
  if (loopback_mode_ == HCI_LOOPBACK_MODE_LOCAL) {
    uint16_t channel = sco_packet->GetChannel();
    send_sco_(std::move(sco_packet));
    send_event_(EventPacket::CreateNumberOfCompletedPacketsEvent(channel, 1));
    return;
  }
}

void DualModeController::HandleCommand(
    std::unique_ptr<CommandPacket> command_packet) {
  uint16_t opcode = command_packet->GetOpcode();
  LOG_INFO(LOG_TAG, "Command opcode: 0x%04X, OGF: 0x%04X, OCF: 0x%04X", opcode,
           command_packet->GetOGF(), command_packet->GetOCF());

  if (loopback_mode_ == HCI_LOOPBACK_MODE_LOCAL &&
      // Loopback exceptions.
      opcode != HCI_RESET && opcode != HCI_SET_HC_TO_HOST_FLOW_CTRL &&
      opcode != HCI_HOST_BUFFER_SIZE && opcode != HCI_HOST_NUM_PACKETS_DONE &&
      opcode != HCI_READ_BUFFER_SIZE && opcode != HCI_READ_LOOPBACK_MODE &&
      opcode != HCI_WRITE_LOOPBACK_MODE) {
    send_event_(EventPacket::CreateLoopbackCommandEvent(
        opcode, command_packet->GetPayload()));
  } else if (active_hci_commands_.count(opcode) > 0) {
    active_hci_commands_[opcode](command_packet->GetPayload());
  } else {
    SendCommandCompleteOnlyStatus(opcode, kUnknownHciCommand);
  }
}

void DualModeController::RegisterEventChannel(
    const std::function<void(std::unique_ptr<EventPacket>)>& callback) {
  send_event_ = callback;
}

void DualModeController::RegisterAclChannel(
    const std::function<void(std::unique_ptr<AclPacket>)>& callback) {
  send_acl_ = callback;
}

void DualModeController::RegisterScoChannel(
    const std::function<void(std::unique_ptr<ScoPacket>)>& callback) {
  send_sco_ = callback;
}

void DualModeController::HandleTimerTick() {
  if (state_ == kInquiry) PageScan();
  if (le_scan_enable_ || le_connect_) LeScan();
  Connections();
  for (size_t dev = 0; dev < devices_.size(); dev++) devices_[dev]->TimerTick();
}

void DualModeController::SetTimerPeriod(std::chrono::milliseconds new_period) {
  timer_period_ = new_period;

  if (timer_tick_task_ == kInvalidTaskId) return;

  // Restart the timer with the new period
  StopTimer();
  StartTimer();
}

static uint8_t GetRssi(size_t dev) {
  // TODO: Model rssi somehow
  return -((dev * 16) % 127);
}

static uint8_t LeGetHandle() {
  static int handle = 0;
  return handle++;
}

static uint8_t LeGetConnInterval() { return 1; }

static uint8_t LeGetConnLatency() { return 2; }

static uint8_t LeGetSupervisionTimeout() { return 3; }

void DualModeController::Connections() {
  for (size_t i = 0; i < connections_.size(); i++) {
    if (connections_[i]->Connected()) {
      connections_[i]->SendToDevice();
      vector<uint8_t> data;
      connections_[i]->ReceiveFromDevice(data);
      // HandleConnectionData(data);
    }
  }
}

void DualModeController::LeScan() {
  std::unique_ptr<EventPacket> le_adverts =
      EventPacket::CreateLeAdvertisingReportEvent();
  vector<uint8_t> ad;
  for (size_t dev = 0; dev < devices_.size(); dev++) {
    uint8_t adv_type;
    const BtAddress addr = devices_[dev]->GetBtAddress();
    uint8_t addr_type = devices_[dev]->GetAddressType();
    ad.clear();

    // Listen for Advertisements
    if (devices_[dev]->IsAdvertisementAvailable(
            std::chrono::milliseconds(le_scan_window_))) {
      ad = devices_[dev]->GetAdvertisement();
      adv_type = devices_[dev]->GetAdvertisementType();
      if (le_scan_enable_ && !le_adverts->AddLeAdvertisingReport(
                                 adv_type, addr_type, addr, ad, GetRssi(dev))) {
        send_event_(std::move(le_adverts));
        le_adverts = EventPacket::CreateLeAdvertisingReportEvent();
        CHECK(le_adverts->AddLeAdvertisingReport(adv_type, addr_type, addr, ad,
                                                 GetRssi(dev)));
      }

      // Connect
      if (le_connect_ && (adv_type == BTM_BLE_CONNECT_EVT ||
                          adv_type == BTM_BLE_CONNECT_DIR_EVT)) {
        LOG_INFO(LOG_TAG, "Connecting to device %d", static_cast<int>(dev));
        if (peer_address_ == addr && peer_address_type_ == addr_type &&
            devices_[dev]->LeConnect()) {
          uint16_t handle = LeGetHandle();
          std::unique_ptr<EventPacket> event =
              EventPacket::CreateLeConnectionCompleteEvent(
                  kSuccessStatus, handle, HCI_ROLE_MASTER, addr_type, addr,
                  LeGetConnInterval(), LeGetConnLatency(),
                  LeGetSupervisionTimeout());
          send_event_(std::move(event));
          le_connect_ = false;

          connections_.push_back(
              std::make_shared<Connection>(devices_[dev], handle));
        }

        // TODO: Handle the white list (if (InWhiteList(dev)))
      }

      // Active scanning
      if (le_scan_enable_ && le_scan_type_ == 1) {
        ad.clear();
        if (devices_[dev]->HasScanResponse()) {
          ad = devices_[dev]->GetScanResponse();
          if (!le_adverts->AddLeAdvertisingReport(
                  BTM_BLE_SCAN_RSP_EVT, addr_type, addr, ad, GetRssi(dev))) {
            send_event_(std::move(le_adverts));
            le_adverts = EventPacket::CreateLeAdvertisingReportEvent();
            CHECK(le_adverts->AddLeAdvertisingReport(
                BTM_BLE_SCAN_RSP_EVT, addr_type, addr, ad, GetRssi(dev)));
          }
        }
      }
    }
  }

  if (le_scan_enable_) send_event_(std::move(le_adverts));
}

void DualModeController::PageScan() {
  // Inquiry modes for specifiying inquiry result formats.
  static const uint8_t kStandardInquiry = 0x00;
  static const uint8_t kRssiInquiry = 0x01;
  static const uint8_t kExtendedOrRssiInquiry = 0x02;

  switch (inquiry_mode_) {
    case (kStandardInquiry): {
      std::unique_ptr<EventPacket> inquiry_result =
          EventPacket::CreateInquiryResultEvent();
      for (size_t dev = 0; dev < devices_.size(); dev++)
        // Scan for devices
        if (devices_[dev]->IsPageScanAvailable()) {
          bool result_added = inquiry_result->AddInquiryResult(
              devices_[dev]->GetBtAddress(),
              devices_[dev]->GetPageScanRepetitionMode(),
              devices_[dev]->GetDeviceClass(), devices_[dev]->GetClockOffset());
          if (!result_added) {
            send_event_(std::move(inquiry_result));
            inquiry_result = EventPacket::CreateInquiryResultEvent();
            result_added = inquiry_result->AddInquiryResult(
                devices_[dev]->GetBtAddress(),
                devices_[dev]->GetPageScanRepetitionMode(),
                devices_[dev]->GetDeviceClass(),
                devices_[dev]->GetClockOffset());
            CHECK(result_added);
          }
        }
    } break;

    case (kRssiInquiry):
      LOG_INFO(LOG_TAG, "RSSI Inquiry Mode currently not supported.");
      break;

    case (kExtendedOrRssiInquiry):
      for (size_t dev = 0; dev < devices_.size(); dev++)
        if (devices_[dev]->IsPageScanAvailable()) {
          send_event_(EventPacket::CreateExtendedInquiryResultEvent(
              devices_[dev]->GetBtAddress(),
              devices_[dev]->GetPageScanRepetitionMode(),
              devices_[dev]->GetDeviceClass(), devices_[dev]->GetClockOffset(),
              GetRssi(dev), devices_[dev]->GetExtendedInquiryData()));
        }
      break;
  }
}

void DualModeController::StartTimer() {
  LOG_ERROR(LOG_TAG, "StartTimer");
  timer_tick_task_ = schedule_periodic_task_(
      std::chrono::milliseconds(0), timer_period_,
      [this]() { DualModeController::HandleTimerTick(); });
}

void DualModeController::StopTimer() {
  LOG_ERROR(LOG_TAG, "StopTimer");
  cancel_task_(timer_tick_task_);
  timer_tick_task_ = kInvalidTaskId;
}

void DualModeController::SetEventDelay(int64_t delay) {
  if (delay < 0) delay = 0;
}

void DualModeController::TestChannelAdd(const vector<std::string>& args) {
  LogCommand("TestChannel 'add'");

  std::shared_ptr<Device> new_dev = DeviceFactory::Create(args);

  if (new_dev == NULL) {
    LOG_ERROR(LOG_TAG, "TestChannel 'add' failed!");
    return;
  }

  devices_.push_back(new_dev);
}

void DualModeController::TestChannelDel(const vector<std::string>& args) {
  LogCommand("TestChannel 'del'");

  size_t dev_index = std::stoi(args[0]);

  if (dev_index >= devices_.size()) {
    LOG_INFO(LOG_TAG, "TestChannel 'del': index %d out of range!",
             static_cast<int>(dev_index));
  } else {
    devices_.erase(devices_.begin() + dev_index);
  }
}

void DualModeController::TestChannelList(
    UNUSED_ATTR const vector<std::string>& args) const {
  LogCommand("TestChannel 'list'");
  LOG_INFO(LOG_TAG, "Devices:");
  for (size_t dev = 0; dev < devices_.size(); dev++) {
    LOG_INFO(LOG_TAG, "%d:", static_cast<int>(dev));
    devices_[dev]->ToString();
  }
}

void DualModeController::HciReset(const vector<uint8_t>& args) {
  LogCommand("Reset");
  CHECK(args[0] == 0);  // No arguments
  state_ = kStandby;
  if (timer_tick_task_ != kInvalidTaskId) {
    LOG_INFO(LOG_TAG, "The timer was already running!");
    StopTimer();
  }
  LOG_INFO(LOG_TAG, "Starting timer.");
  StartTimer();

  SendCommandCompleteSuccess(HCI_RESET);
}

void DualModeController::HciReadBufferSize(const vector<uint8_t>& args) {
  LogCommand("Read Buffer Size");
  CHECK(args[0] == 0);  // No arguments
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadBufferSize(
          kSuccessStatus, properties_.GetAclDataPacketSize(),
          properties_.GetSynchronousDataPacketSize(),
          properties_.GetTotalNumAclDataPackets(),
          properties_.GetTotalNumSynchronousDataPackets());

  send_event_(std::move(command_complete));
}

void DualModeController::HciHostBufferSize(const vector<uint8_t>& args) {
  LogCommand("Host Buffer Size");
  CHECK(args[0] == 7);  // No arguments
  SendCommandCompleteSuccess(HCI_HOST_BUFFER_SIZE);
}

void DualModeController::HciReadLocalVersionInformation(
    const vector<uint8_t>& args) {
  LogCommand("Read Local Version Information");
  CHECK(args[0] == 0);  // No arguments
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadLocalVersionInformation(
          kSuccessStatus, properties_.GetVersion(), properties_.GetRevision(),
          properties_.GetLmpPalVersion(), properties_.GetManufacturerName(),
          properties_.GetLmpPalSubversion());
  send_event_(std::move(command_complete));
}

void DualModeController::HciReadBdAddr(const vector<uint8_t>& args) {
  CHECK(args[0] == 0);  // No arguments
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadBdAddr(kSuccessStatus,
                                                   properties_.GetAddress());
  send_event_(std::move(command_complete));
}

void DualModeController::HciReadLocalSupportedCommands(
    const vector<uint8_t>& args) {
  LogCommand("Read Local Supported Commands");
  CHECK(args[0] == 0);  // No arguments
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadLocalSupportedCommands(
          kSuccessStatus, properties_.GetLocalSupportedCommands());
  send_event_(std::move(command_complete));
}

void DualModeController::HciReadLocalSupportedCodecs(
    const vector<uint8_t>& args) {
  LogCommand("Read Local Supported Codecs");
  CHECK(args[0] == 0);  // No arguments
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadLocalSupportedCodecs(
          kSuccessStatus, properties_.GetSupportedCodecs(),
          properties_.GetVendorSpecificCodecs());
  send_event_(std::move(command_complete));
}

void DualModeController::HciReadLocalExtendedFeatures(
    const vector<uint8_t>& args) {
  LogCommand("Read Local Extended Features");
  CHECK(args.size() == 2);
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadLocalExtendedFeatures(
          kSuccessStatus, args[1],
          properties_.GetLocalExtendedFeaturesMaximumPageNumber(),
          properties_.GetLocalExtendedFeatures(args[1]));
  send_event_(std::move(command_complete));
}

void DualModeController::HciWriteSimplePairingMode(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Simple Pairing Mode");
  SendCommandCompleteSuccess(HCI_WRITE_SIMPLE_PAIRING_MODE);
}

void DualModeController::HciWriteLeHostSupport(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Le Host Support");
  SendCommandCompleteSuccess(HCI_WRITE_LE_HOST_SUPPORT);
}

void DualModeController::HciSetEventMask(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Set Event Mask");
  SendCommandCompleteSuccess(HCI_SET_EVENT_MASK);
}

void DualModeController::HciWriteInquiryMode(const vector<uint8_t>& args) {
  LogCommand("Write Inquiry Mode");
  CHECK(args.size() == 2);
  inquiry_mode_ = args[1];
  SendCommandCompleteSuccess(HCI_WRITE_INQUIRY_MODE);
}

void DualModeController::HciWritePageScanType(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Page Scan Type");
  SendCommandCompleteSuccess(HCI_WRITE_PAGESCAN_TYPE);
}

void DualModeController::HciWriteInquiryScanType(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Inquiry Scan Type");
  SendCommandCompleteSuccess(HCI_WRITE_INQSCAN_TYPE);
}

void DualModeController::HciWriteClassOfDevice(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Class Of Device");
  SendCommandCompleteSuccess(HCI_WRITE_CLASS_OF_DEVICE);
}

void DualModeController::HciWritePageTimeout(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Page Timeout");
  SendCommandCompleteSuccess(HCI_WRITE_PAGE_TOUT);
}

void DualModeController::HciWriteDefaultLinkPolicySettings(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Default Link Policy Settings");
  SendCommandCompleteSuccess(HCI_WRITE_DEF_POLICY_SETTINGS);
}

void DualModeController::HciReadLocalName(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Get Local Name");
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadLocalName(
          kSuccessStatus, properties_.GetLocalName());
  send_event_(std::move(command_complete));
}

void DualModeController::HciWriteLocalName(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Local Name");
  SendCommandCompleteSuccess(HCI_CHANGE_LOCAL_NAME);
}

void DualModeController::HciWriteExtendedInquiryResponse(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Extended Inquiry Response");
  SendCommandCompleteSuccess(HCI_WRITE_EXT_INQ_RESPONSE);
}

void DualModeController::HciWriteVoiceSetting(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Voice Setting");
  SendCommandCompleteSuccess(HCI_WRITE_VOICE_SETTINGS);
}

void DualModeController::HciWriteCurrentIacLap(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Current IAC LAP");
  SendCommandCompleteSuccess(HCI_WRITE_CURRENT_IAC_LAP);
}

void DualModeController::HciWriteInquiryScanActivity(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Inquiry Scan Activity");
  SendCommandCompleteSuccess(HCI_WRITE_INQUIRYSCAN_CFG);
}

void DualModeController::HciWriteScanEnable(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Write Scan Enable");
  SendCommandCompleteSuccess(HCI_WRITE_SCAN_ENABLE);
}

void DualModeController::HciSetEventFilter(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Set Event Filter");
  SendCommandCompleteSuccess(HCI_SET_EVENT_FILTER);
}

void DualModeController::HciInquiry(const vector<uint8_t>& args) {
  LogCommand("Inquiry");
  CHECK(args.size() == 6);
  state_ = kInquiry;
  SendCommandStatusSuccess(HCI_INQUIRY);
  inquiry_lap_[0] = args[1];
  inquiry_lap_[1] = args[2];
  inquiry_lap_[2] = args[3];

  AddControllerEvent(std::chrono::milliseconds(args[4] * 1280),
                     [this]() { DualModeController::InquiryTimeout(); });

  if (args[5] > 0) {
    inquiry_responses_limited_ = true;
    inquiry_num_responses_ = args[5];
  }
}

void DualModeController::HciInquiryCancel(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Inquiry Cancel");
  CHECK(state_ == kInquiry);
  state_ = kStandby;
  SendCommandCompleteSuccess(HCI_INQUIRY_CANCEL);
}

void DualModeController::InquiryTimeout() {
  LOG_INFO(LOG_TAG, "InquiryTimer fired");
  if (state_ == kInquiry) {
    state_ = kStandby;
    inquiry_responses_limited_ = false;
    send_event_(EventPacket::CreateInquiryCompleteEvent(kSuccessStatus));
  }
}

void DualModeController::HciDeleteStoredLinkKey(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Delete Stored Link Key");
  /* Check the last octect in |args|. If it is 0, delete only the link key for
   * the given BD_ADDR. If is is 1, delete all stored link keys. */
  uint16_t deleted_keys = 1;

  send_event_(EventPacket::CreateCommandCompleteDeleteStoredLinkKey(
      kSuccessStatus, deleted_keys));
}

void DualModeController::HciRemoteNameRequest(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("Remote Name Request");
  SendCommandStatusSuccess(HCI_RMT_NAME_REQUEST);
}

void DualModeController::HciLeSetEventMask(const vector<uint8_t>& args) {
  LogCommand("LE SetEventMask");
  le_event_mask_ = args;
  SendCommandCompleteSuccess(HCI_BLE_SET_EVENT_MASK);
}

void DualModeController::HciLeReadBufferSize(
    UNUSED_ATTR const vector<uint8_t>& args) {
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteLeReadBufferSize(
          kSuccessStatus, properties_.GetLeDataPacketLength(),
          properties_.GetTotalNumLeDataPackets());
  send_event_(std::move(command_complete));
}

void DualModeController::HciLeReadLocalSupportedFeatures(
    UNUSED_ATTR const vector<uint8_t>& args) {
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteLeReadLocalSupportedFeatures(
          kSuccessStatus, properties_.GetLeLocalSupportedFeatures());
  send_event_(std::move(command_complete));
}

void DualModeController::HciLeSetRandomAddress(const vector<uint8_t>& args) {
  LogCommand("LE SetRandomAddress");
  CHECK(args.size() == 7);
  vector<uint8_t> new_addr = {args[1], args[2], args[3],
                              args[4], args[5], args[6]};
  CHECK(le_random_address_.FromVector(new_addr));
  SendCommandCompleteSuccess(HCI_BLE_WRITE_RANDOM_ADDR);
}

void DualModeController::HciLeSetAdvertisingParameters(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("LE SetAdvertisingParameters");
  SendCommandCompleteSuccess(HCI_BLE_WRITE_ADV_PARAMS);
}

void DualModeController::HciLeSetAdvertisingData(
    UNUSED_ATTR const vector<uint8_t>& args) {
  LogCommand("LE SetAdvertisingData");
  SendCommandCompleteSuccess(HCI_BLE_WRITE_ADV_DATA);
}

void DualModeController::HciLeSetScanParameters(const vector<uint8_t>& args) {
  LogCommand("LE SetScanParameters");
  CHECK(args.size() == 8);
  le_scan_type_ = args[1];
  le_scan_interval_ = args[2] | (args[3] << 8);
  le_scan_window_ = args[4] | (args[5] << 8);
  own_address_type_ = args[6];
  scanning_filter_policy_ = args[7];
  SendCommandCompleteSuccess(HCI_BLE_WRITE_SCAN_PARAMS);
}

void DualModeController::HciLeSetScanEnable(const vector<uint8_t>& args) {
  LogCommand("LE SetScanEnable");
  CHECK(args.size() == 3);
  le_scan_enable_ = args[1];
  filter_duplicates_ = args[2];
  SendCommandCompleteSuccess(HCI_BLE_WRITE_SCAN_ENABLE);
}

void DualModeController::HciLeCreateConnection(const vector<uint8_t>& args) {
  LogCommand("LE CreateConnection");
  le_connect_ = true;
  le_scan_interval_ = args[1] | (args[2] << 8);
  le_scan_window_ = args[3] | (args[4] << 8);
  initiator_filter_policy_ = args[5];

  if (initiator_filter_policy_ == 0) {  // White list not used
    peer_address_type_ = args[6];
    vector<uint8_t> peer_addr = {args[7],  args[8],  args[9],
                                 args[10], args[11], args[12]};
    peer_address_.FromVector(peer_addr);
  }

  SendCommandStatusSuccess(HCI_BLE_CREATE_LL_CONN);
}

void DualModeController::HciLeConnectionCancel(const vector<uint8_t>& args) {
  LogCommand("LE ConnectionCancel");
  CHECK(args[0] == 0);  // No arguments
  le_connect_ = false;
  SendCommandStatusSuccess(HCI_BLE_CREATE_CONN_CANCEL);
}

void DualModeController::HciLeReadWhiteListSize(const vector<uint8_t>& args) {
  LogCommand("LE ReadWhiteListSize");
  CHECK(args[0] == 0);  // No arguments
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteLeReadWhiteListSize(
          kSuccessStatus, properties_.GetLeWhiteListSize());
  send_event_(std::move(command_complete));
}

void DualModeController::HciLeReadRemoteUsedFeaturesB(uint16_t handle) {
  uint64_t features;
  LogCommand("LE ReadRemoteUsedFeatures Bottom half");

  for (size_t i = 0; i < connections_.size(); i++)
    if (*connections_[i] == handle)
      // TODO:
      // features = connections_[i]->GetDevice()->GetUsedFeatures();
      features = 0xffffffffffffffff;

  std::unique_ptr<EventPacket> event =
      EventPacket::CreateLeRemoteUsedFeaturesEvent(kSuccessStatus, handle,
                                                   features);
  send_event_(std::move(event));
}

void DualModeController::HciLeReadRemoteUsedFeatures(
    const vector<uint8_t>& args) {
  LogCommand("LE ReadRemoteUsedFeatures");
  CHECK(args.size() == 3);

  uint16_t handle = args[1] | (args[2] << 8);

  AddConnectionAction(
      [this, handle]() {
        DualModeController::HciLeReadRemoteUsedFeaturesB(handle);
      },
      handle);

  SendCommandStatusSuccess(HCI_BLE_READ_REMOTE_FEAT);
}

void DualModeController::HciLeRand(UNUSED_ATTR const vector<uint8_t>& args) {
  uint64_t random_val = rand();
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteLeRand(kSuccessStatus, random_val);
  send_event_(std::move(command_complete));
}

void DualModeController::HciLeReadSupportedStates(
    UNUSED_ATTR const vector<uint8_t>& args) {
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteLeReadSupportedStates(
          kSuccessStatus, properties_.GetLeSupportedStates());
  send_event_(std::move(command_complete));
}

void DualModeController::HciBleVendorSleepMode(
    UNUSED_ATTR const vector<uint8_t>& args) {
  SendCommandCompleteOnlyStatus(HCI_GRP_VENDOR_SPECIFIC | 0x27,
                                kUnknownHciCommand);
}

void DualModeController::HciBleVendorCap(
    UNUSED_ATTR const vector<uint8_t>& args) {
  vector<uint8_t> caps = properties_.GetLeVendorCap();
  if (caps.size() == 0) {
    SendCommandCompleteOnlyStatus(HCI_BLE_VENDOR_CAP_OCF, kUnknownHciCommand);
    return;
  }

  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteLeVendorCap(
          kSuccessStatus, properties_.GetLeVendorCap());
  send_event_(std::move(command_complete));
}

void DualModeController::HciBleVendorMultiAdv(
    UNUSED_ATTR const vector<uint8_t>& args) {
  SendCommandCompleteOnlyStatus(HCI_BLE_MULTI_ADV_OCF, kUnknownHciCommand);
}

void DualModeController::HciBleVendor155(
    UNUSED_ATTR const vector<uint8_t>& args) {
  SendCommandCompleteOnlyStatus(HCI_GRP_VENDOR_SPECIFIC | 0x155,
                                kUnknownHciCommand);
}

void DualModeController::HciBleAdvertisingFilter(
    UNUSED_ATTR const vector<uint8_t>& args) {
  SendCommandCompleteOnlyStatus(HCI_BLE_ADV_FILTER_OCF, kUnknownHciCommand);
}

void DualModeController::HciBleEnergyInfo(
    UNUSED_ATTR const vector<uint8_t>& args) {
  SendCommandCompleteOnlyStatus(HCI_BLE_ENERGY_INFO_OCF, kUnknownHciCommand);
}

void DualModeController::HciBleExtendedScanParams(
    UNUSED_ATTR const vector<uint8_t>& args) {
  SendCommandCompleteOnlyStatus(HCI_BLE_EXTENDED_SCAN_PARAMS_OCF,
                                kUnknownHciCommand);
}

void DualModeController::HciReadLoopbackMode(const vector<uint8_t>& args) {
  CHECK(args[0] == 0);  // No arguments
  std::unique_ptr<EventPacket> command_complete =
      EventPacket::CreateCommandCompleteReadLoopbackMode(kSuccessStatus,
                                                         loopback_mode_);
  send_event_(std::move(command_complete));
}

void DualModeController::HciWriteLoopbackMode(const vector<uint8_t>& args) {
  CHECK(args[0] == 1);
  loopback_mode_ = args[1];
  // ACL channel
  uint16_t acl_handle = 0x123;
  send_event_(EventPacket::CreateConnectionCompleteEvent(
      kSuccessStatus, acl_handle, properties_.GetAddress(), HCI_LINK_TYPE_ACL,
      false));
  // SCO channel
  uint16_t sco_handle = 0x345;
  send_event_(EventPacket::CreateConnectionCompleteEvent(
      kSuccessStatus, sco_handle, properties_.GetAddress(), HCI_LINK_TYPE_SCO,
      false));
  SendCommandCompleteSuccess(HCI_WRITE_LOOPBACK_MODE);
}

}  // namespace test_vendor_lib
