//
// Copyright 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#define LOG_TAG "android.hardware.bluetooth@1.0.sim"

#include "bluetooth_hci.h"

#include <base/logging.h>
#include <string.h>
#include <utils/Log.h>

#include "acl_packet.h"
#include "event_packet.h"
#include "hci_internals.h"
#include "sco_packet.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace sim {

using android::hardware::hidl_vec;
using test_vendor_lib::AclPacket;
using test_vendor_lib::AsyncManager;
using test_vendor_lib::AsyncTaskId;
using test_vendor_lib::CommandPacket;
using test_vendor_lib::DualModeController;
using test_vendor_lib::EventPacket;
using test_vendor_lib::ScoPacket;
using test_vendor_lib::TaskCallback;
using test_vendor_lib::TestChannelTransport;

class BluetoothDeathRecipient : public hidl_death_recipient {
 public:
  BluetoothDeathRecipient(const sp<IBluetoothHci> hci) : mHci(hci) {}

  virtual void serviceDied(
      uint64_t /* cookie */,
      const wp<::android::hidl::base::V1_0::IBase>& /* who */) {
    ALOGE("BluetoothDeathRecipient::serviceDied - Bluetooth service died");
    has_died_ = true;
    mHci->close();
  }
  sp<IBluetoothHci> mHci;
  bool getHasDied() const { return has_died_; }
  void setHasDied(bool has_died) { has_died_ = has_died; }

 private:
  bool has_died_;
};

BluetoothHci::BluetoothHci()
    : death_recipient_(new BluetoothDeathRecipient(this)) {}

Return<void> BluetoothHci::initialize(const sp<IBluetoothHciCallbacks>& cb) {
  ALOGI("%s", __func__);

  if (cb == nullptr) {
    ALOGE("cb == nullptr! -> Unable to call initializationComplete(ERR)");
    return Void();
  }

  death_recipient_->setHasDied(false);
  cb->linkToDeath(death_recipient_, 0);

  test_channel_transport_.RegisterCommandHandler(
      [this](const std::string& name, const std::vector<std::string>& args) {
        async_manager_.ExecAsync(
            std::chrono::milliseconds(0), [this, name, args]() {
              controller_.HandleTestChannelCommand(name, args);
            });
      });

  controller_.RegisterEventChannel([cb](std::unique_ptr<EventPacket> event) {
    size_t header_bytes = event->GetHeaderSize();
    size_t payload_bytes = event->GetPayloadSize();
    hidl_vec<uint8_t> hci_event;
    hci_event.resize(header_bytes + payload_bytes);
    memcpy(hci_event.data(), event->GetHeader().data(), header_bytes);
    memcpy(hci_event.data() + header_bytes, event->GetPayload().data(),
           payload_bytes);

    cb->hciEventReceived(hci_event);
  });

  controller_.RegisterAclChannel([cb](std::unique_ptr<AclPacket> packet) {
    std::vector<uint8_t> acl_vector = packet->GetPacket();
    hidl_vec<uint8_t> acl_packet = acl_vector;

    cb->aclDataReceived(acl_packet);
  });

  controller_.RegisterScoChannel([cb](std::unique_ptr<ScoPacket> packet) {
    size_t header_bytes = packet->GetHeaderSize();
    size_t payload_bytes = packet->GetPayloadSize();
    hidl_vec<uint8_t> sco_packet;
    sco_packet.resize(header_bytes + payload_bytes);
    memcpy(sco_packet.data(), packet->GetHeader().data(), header_bytes);
    memcpy(sco_packet.data() + header_bytes, packet->GetPayload().data(),
           payload_bytes);

    cb->scoDataReceived(sco_packet);
  });

  controller_.RegisterTaskScheduler(
      [this](std::chrono::milliseconds delay, const TaskCallback& task) {
        return async_manager_.ExecAsync(delay, task);
      });

  controller_.RegisterPeriodicTaskScheduler(
      [this](std::chrono::milliseconds delay, std::chrono::milliseconds period,
             const TaskCallback& task) {
        return async_manager_.ExecAsyncPeriodically(delay, period, task);
      });

  controller_.RegisterTaskCancel(
      [this](AsyncTaskId task) { async_manager_.CancelAsyncTask(task); });

  SetUpTestChannel(6111);

  unlink_cb_ = [cb](sp<BluetoothDeathRecipient>& death_recipient) {
    if (death_recipient->getHasDied())
      ALOGI("Skipping unlink call, service died.");
    else
      cb->unlinkToDeath(death_recipient);
  };

  cb->initializationComplete(Status::SUCCESS);
  return Void();
}

Return<void> BluetoothHci::close() {
  ALOGI("%s", __func__);
  return Void();
}

Return<void> BluetoothHci::sendHciCommand(const hidl_vec<uint8_t>& packet) {
  async_manager_.ExecAsync(std::chrono::milliseconds(0), [this, packet]() {
    uint16_t opcode = packet[0] | (packet[1] << 8);
    std::unique_ptr<CommandPacket> command =
        std::unique_ptr<CommandPacket>(new CommandPacket(opcode));
    for (size_t i = 3; i < packet.size(); i++)
      command->AddPayloadOctets1(packet[i]);

    controller_.HandleCommand(std::move(command));
  });
  return Void();
}

Return<void> BluetoothHci::sendAclData(const hidl_vec<uint8_t>& packet) {
  async_manager_.ExecAsync(std::chrono::milliseconds(0), [this, packet]() {
    uint16_t channel = (packet[0] | (packet[1] << 8)) & 0xfff;
    AclPacket::PacketBoundaryFlags boundary_flags =
        static_cast<AclPacket::PacketBoundaryFlags>((packet[1] & 0x30) >> 4);
    AclPacket::BroadcastFlags broadcast_flags =
        static_cast<AclPacket::BroadcastFlags>((packet[1] & 0xC0) >> 6);

    std::unique_ptr<AclPacket> acl = std::unique_ptr<AclPacket>(
        new AclPacket(channel, boundary_flags, broadcast_flags));
    for (size_t i = 4; i < packet.size(); i++)
      acl->AddPayloadOctets1(packet[i]);

    controller_.HandleAcl(std::move(acl));
  });
  return Void();
}

Return<void> BluetoothHci::sendScoData(const hidl_vec<uint8_t>& packet) {
  async_manager_.ExecAsync(std::chrono::milliseconds(0), [this, packet]() {
    uint16_t channel = (packet[0] | (packet[1] << 8)) & 0xfff;
    ScoPacket::PacketStatusFlags packet_status =
        static_cast<ScoPacket::PacketStatusFlags>((packet[1] & 0x30) >> 4);
    std::unique_ptr<ScoPacket> sco =
        std::unique_ptr<ScoPacket>(new ScoPacket(channel, packet_status));
    for (size_t i = 3; i < packet.size(); i++)
      sco->AddPayloadOctets1(packet[i]);

    controller_.HandleSco(std::move(sco));
  });
  return Void();
}

void BluetoothHci::SetUpTestChannel(int port) {
  int socket_fd = test_channel_transport_.SetUp(port);

  if (socket_fd == -1) {
    ALOGE("Test channel SetUp(%d) failed.", port);
    return;
  }

  ALOGI("Test channel SetUp() successful");
  async_manager_.WatchFdForNonBlockingReads(socket_fd, [this](int socket_fd) {
    int conn_fd = test_channel_transport_.Accept(socket_fd);
    if (conn_fd < 0) {
      ALOGE("Error watching test channel fd.");
      return;
    }
    ALOGI("Test channel connection accepted.");
    async_manager_.WatchFdForNonBlockingReads(conn_fd, [this](int conn_fd) {
      test_channel_transport_.OnCommandReady(conn_fd, [this, conn_fd]() {
        async_manager_.StopWatchingFileDescriptor(conn_fd);
      });
    });
  });
}

/* Fallback to shared library if there is no service. */
IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* /* name */) {
  return new BluetoothHci();
}

}  // namespace gce
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
