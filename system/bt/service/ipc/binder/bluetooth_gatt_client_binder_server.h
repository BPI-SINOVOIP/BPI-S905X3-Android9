//
//  Copyright 2015 Google, Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at:
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#pragma once

#include <base/macros.h>

#include <android/bluetooth/BnBluetoothGattClient.h>
#include <android/bluetooth/IBluetoothGattClientCallback.h>

#include "service/gatt_client.h"
#include "service/ipc/binder/interface_with_instances_base.h"

using android::bluetooth::BnBluetoothGattClient;
using android::bluetooth::IBluetoothGattClientCallback;

using ::android::binder::Status;

namespace bluetooth {
class Adapter;
}  // namespace bluetooth

namespace ipc {
namespace binder {

// Implements the server side of the IBluetoothGattClient interface.
class BluetoothGattClientBinderServer : public BnBluetoothGattClient,
                                        public InterfaceWithInstancesBase {
 public:
  explicit BluetoothGattClientBinderServer(bluetooth::Adapter* adapter);
  ~BluetoothGattClientBinderServer() override = default;

  // IBluetoothGattClient overrides:
  Status RegisterClient(
      const android::sp<IBluetoothGattClientCallback>& callback,
      bool* _aidl_return) override;
  Status UnregisterClient(int client_id) override;
  Status UnregisterAll() override;

 private:
  // Returns a pointer to the IBluetoothGattClientCallback instance
  // associated with |client_id|. Returns NULL if such a callback cannot be
  // found.
  android::sp<IBluetoothGattClientCallback> GetGattClientCallback(
      int client_id);

  // Returns a pointer to the GattClient instance associated with |client_id|.
  // Returns NULL if such a client cannot be found.
  std::shared_ptr<bluetooth::GattClient> GetGattClient(int client_id);

  // InterfaceWithInstancesBase override:
  void OnRegisterInstanceImpl(bluetooth::BLEStatus status,
                              android::sp<IInterface> callback,
                              bluetooth::BluetoothInstance* instance) override;

  bluetooth::Adapter* adapter_;  // weak

  DISALLOW_COPY_AND_ASSIGN(BluetoothGattClientBinderServer);
};

}  // namespace binder
}  // namespace ipc
