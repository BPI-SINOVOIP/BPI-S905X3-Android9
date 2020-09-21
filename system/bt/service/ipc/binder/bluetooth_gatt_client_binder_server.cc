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

#include "service/ipc/binder/bluetooth_gatt_client_binder_server.h"

#include <base/logging.h>

#include "service/adapter.h"

using android::bluetooth::IBluetoothGattClientCallback;

namespace ipc {
namespace binder {

namespace {
const int kInvalidInstanceId = -1;
}  // namespace

BluetoothGattClientBinderServer::BluetoothGattClientBinderServer(
    bluetooth::Adapter* adapter)
    : adapter_(adapter) {
  CHECK(adapter_);
}

Status BluetoothGattClientBinderServer::RegisterClient(
    const android::sp<IBluetoothGattClientCallback>& callback,
    bool* _aidl_return) {
  VLOG(2) << __func__;

  bluetooth::GattClientFactory* gatt_client_factory =
      adapter_->GetGattClientFactory();

  *_aidl_return = RegisterInstanceBase(callback, gatt_client_factory);
  return Status::ok();
}

Status BluetoothGattClientBinderServer::UnregisterClient(int client_id) {
  VLOG(2) << __func__;
  UnregisterInstanceBase(client_id);
  return Status::ok();
}

Status BluetoothGattClientBinderServer::UnregisterAll() {
  VLOG(2) << __func__;
  UnregisterAllBase();
  return Status::ok();
}

android::sp<IBluetoothGattClientCallback>
BluetoothGattClientBinderServer::GetGattClientCallback(int client_id) {
  auto cb = GetCallback(client_id);
  return android::sp<IBluetoothGattClientCallback>(
      static_cast<IBluetoothGattClientCallback*>(cb.get()));
}

std::shared_ptr<bluetooth::GattClient>
BluetoothGattClientBinderServer::GetGattClient(int client_id) {
  return std::static_pointer_cast<bluetooth::GattClient>(
      GetInstance(client_id));
}

void BluetoothGattClientBinderServer::OnRegisterInstanceImpl(
    bluetooth::BLEStatus status, android::sp<IInterface> callback,
    bluetooth::BluetoothInstance* instance) {
  VLOG(1) << __func__ << " client ID: " << instance->GetInstanceId()
          << " status: " << status;

  android::sp<IBluetoothGattClientCallback> cb(
      static_cast<IBluetoothGattClientCallback*>(callback.get()));
  cb->OnClientRegistered(status, (status == bluetooth::BLE_STATUS_SUCCESS)
                                     ? instance->GetInstanceId()
                                     : kInvalidInstanceId);
}

}  // namespace binder
}  // namespace ipc
