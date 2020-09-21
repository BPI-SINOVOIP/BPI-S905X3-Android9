/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <android/bluetooth/BnBluetoothSocketManager.h>
#include <android/bluetooth/IBluetoothSocketManager.h>
#include <base/macros.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>

namespace android {
namespace bluetooth {

using ::android::binder::Status;

class BluetoothSocketManagerBinderServer : public BnBluetoothSocketManager {
 public:
  explicit BluetoothSocketManagerBinderServer(
      const btsock_interface_t* socketInterface)
      : socketInterface(socketInterface) {}
  ~BluetoothSocketManagerBinderServer() override = default;

  Status connectSocket(
      const BluetoothDevice& device, int32_t type,
      const std::unique_ptr<::android::os::ParcelUuid>& uuid, int32_t port,
      int32_t flag,
      std::unique_ptr<::android::os::ParcelFileDescriptor>* _aidl_return);

  Status createSocketChannel(
      int32_t type, const std::unique_ptr<::android::String16>& serviceName,
      const std::unique_ptr<::android::os::ParcelUuid>& uuid, int32_t port,
      int32_t flag,
      std::unique_ptr<::android::os::ParcelFileDescriptor>* _aidl_return)
      override;

  Status requestMaximumTxDataLength(const BluetoothDevice& device);

 private:
  const btsock_interface_t* socketInterface;
  DISALLOW_COPY_AND_ASSIGN(BluetoothSocketManagerBinderServer);
};
}  // namespace bluetooth
}  // namespace android