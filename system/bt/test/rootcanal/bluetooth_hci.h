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

#pragma once

#include <android/hardware/bluetooth/1.0/IBluetoothHci.h>

#include <hidl/MQDescriptor.h>

#include "hci_packetizer.h"

#include "async_manager.h"
#include "dual_mode_controller.h"
#include "test_channel_transport.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace sim {

class BluetoothDeathRecipient;

class BluetoothHci : public IBluetoothHci {
 public:
  BluetoothHci();

  ::android::hardware::Return<void> initialize(
      const sp<IBluetoothHciCallbacks>& cb) override;

  ::android::hardware::Return<void> sendHciCommand(
      const ::android::hardware::hidl_vec<uint8_t>& packet) override;

  ::android::hardware::Return<void> sendAclData(
      const ::android::hardware::hidl_vec<uint8_t>& packet) override;

  ::android::hardware::Return<void> sendScoData(
      const ::android::hardware::hidl_vec<uint8_t>& packet) override;

  ::android::hardware::Return<void> close() override;

  static void OnPacketReady();

  static BluetoothHci* get();

 private:
  sp<BluetoothDeathRecipient> death_recipient_;

  std::function<void(sp<BluetoothDeathRecipient>&)> unlink_cb_;

  void HandleIncomingPacket();

  test_vendor_lib::AsyncManager async_manager_;

  void SetUpTestChannel(int port);

  test_vendor_lib::DualModeController controller_;

  test_vendor_lib::TestChannelTransport test_channel_transport_;
};

extern "C" IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* name);

}  // namespace sim
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
