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

#include "service/ipc/binder/bluetooth_binder_server.h"

#include <base/logging.h>

#include "service/ipc/binder/bluetooth_gatt_client_binder_server.h"
#include "service/ipc/binder/bluetooth_gatt_server_binder_server.h"
#include "service/ipc/binder/bluetooth_le_advertiser_binder_server.h"
#include "service/ipc/binder/bluetooth_le_scanner_binder_server.h"
#include "service/ipc/binder/bluetooth_low_energy_binder_server.h"

#include "service/hal/bluetooth_interface.h"

using android::sp;
using android::String8;
using android::String16;

using android::bluetooth::IBluetoothCallback;
using android::bluetooth::IBluetoothGattClient;
using android::bluetooth::IBluetoothGattServer;

namespace ipc {
namespace binder {

BluetoothBinderServer::BluetoothBinderServer(bluetooth::Adapter* adapter)
    : adapter_(adapter) {
  CHECK(adapter_);
  adapter_->AddObserver(this);
}

BluetoothBinderServer::~BluetoothBinderServer() {
  adapter_->RemoveObserver(this);
}

// binder::BnBluetooth overrides:
Status BluetoothBinderServer::IsEnabled(bool* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = adapter_->IsEnabled();
  return Status::ok();
}

Status BluetoothBinderServer::GetState(int32_t* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = adapter_->GetState();
  return Status::ok();
}

Status BluetoothBinderServer::Enable(bool start_restricted,
                                     bool* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = adapter_->Enable(start_restricted);
  return Status::ok();
}

Status BluetoothBinderServer::EnableNoAutoConnect(bool* _aidl_return) {
  VLOG(2) << __func__;
  // TODO(armansito): Implement.
  *_aidl_return = false;
  return Status::ok();
}

Status BluetoothBinderServer::Disable(bool* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = adapter_->Disable();
  return Status::ok();
}

Status BluetoothBinderServer::GetAddress(::android::String16* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = String16(String8(adapter_->GetAddress().c_str()));
  return Status::ok();
}

Status BluetoothBinderServer::GetUUIDs(
    ::std::vector<::android::bluetooth::UUID>* _aidl_return) {
  VLOG(2) << __func__;
  // TODO(armansito): Implement.
  *_aidl_return = std::vector<android::bluetooth::UUID>();
  return Status::ok();
}

Status BluetoothBinderServer::SetName(const ::android::String16& name,
                                      bool* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = adapter_->SetName(std::string(String8(name).string()));
  return Status::ok();
}

Status BluetoothBinderServer::GetName(::android::String16* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = String16(String8(adapter_->GetName().c_str()));
  return Status::ok();
}

Status BluetoothBinderServer::RegisterCallback(
    const ::android::sp<IBluetoothCallback>& callback) {
  VLOG(2) << __func__;
  if (!callback.get()) {
    LOG(ERROR) << "RegisterCallback called with NULL binder. Ignoring.";
    return Status::ok();
  }
  callbacks_.Register(callback);
  return Status::ok();
}

Status BluetoothBinderServer::UnregisterCallback(
    const ::android::sp<IBluetoothCallback>& callback) {
  VLOG(2) << __func__;
  if (!callback.get()) {
    LOG(ERROR) << "UnregisterCallback called with NULL binder. Ignoring.";
    return Status::ok();
  }
  callbacks_.Unregister(callback);
  return Status::ok();
}

Status BluetoothBinderServer::IsMultiAdvertisementSupported(
    bool* _aidl_return) {
  VLOG(2) << __func__;
  *_aidl_return = adapter_->IsMultiAdvertisementSupported();
  return Status::ok();
}

Status BluetoothBinderServer::GetLowEnergyInterface(
    ::android::sp<IBluetoothLowEnergy>* _aidl_return) {
  VLOG(2) << __func__;

  if (!adapter_->IsEnabled()) {
    LOG(ERROR) << "Cannot obtain IBluetoothLowEnergy interface while disabled";
    *_aidl_return = NULL;
    return Status::ok();
  }

  if (!low_energy_interface_.get())
    low_energy_interface_ = new BluetoothLowEnergyBinderServer(adapter_);

  *_aidl_return = low_energy_interface_;
  return Status::ok();
}

Status BluetoothBinderServer::GetLeAdvertiserInterface(
    ::android::sp<IBluetoothLeAdvertiser>* _aidl_return) {
  VLOG(2) << __func__;

  if (!adapter_->IsEnabled()) {
    LOG(ERROR)
        << "Cannot obtain IBluetoothLeAdvertiser interface while disabled";
    *_aidl_return = NULL;
    return Status::ok();
  }

  if (!le_advertiser_interface_.get())
    le_advertiser_interface_ = new BluetoothLeAdvertiserBinderServer(adapter_);

  *_aidl_return = le_advertiser_interface_;
  return Status::ok();
}

Status BluetoothBinderServer::GetLeScannerInterface(
    ::android::sp<IBluetoothLeScanner>* _aidl_return) {
  VLOG(2) << __func__;

  if (!adapter_->IsEnabled()) {
    LOG(ERROR) << "Cannot obtain IBluetoothLeScanner interface while disabled";
    *_aidl_return = NULL;
    return Status::ok();
  }

  if (!le_scanner_interface_.get())
    le_scanner_interface_ = new BluetoothLeScannerBinderServer(adapter_);

  *_aidl_return = le_scanner_interface_;
  return Status::ok();
}

Status BluetoothBinderServer::GetGattClientInterface(
    ::android::sp<IBluetoothGattClient>* _aidl_return) {
  VLOG(2) << __func__;

  if (!adapter_->IsEnabled()) {
    LOG(ERROR) << "Cannot obtain IBluetoothGattClient interface while disabled";
    *_aidl_return = NULL;
    return Status::ok();
  }

  if (!gatt_client_interface_.get())
    gatt_client_interface_ = new BluetoothGattClientBinderServer(adapter_);

  *_aidl_return = gatt_client_interface_;
  return Status::ok();
}

Status BluetoothBinderServer::GetGattServerInterface(
    ::android::sp<IBluetoothGattServer>* _aidl_return) {
  VLOG(2) << __func__;

  if (!adapter_->IsEnabled()) {
    LOG(ERROR) << "Cannot obtain IBluetoothGattServer interface while disabled";
    *_aidl_return = NULL;
    return Status::ok();
  }

  if (!gatt_server_interface_.get())
    gatt_server_interface_ = new BluetoothGattServerBinderServer(adapter_);

  *_aidl_return = gatt_server_interface_;
  return Status::ok();
}

android::status_t BluetoothBinderServer::dump(
    int fd, const android::Vector<android::String16>& args) {
  VLOG(2) << __func__ << " called with fd " << fd;
  if (args.size() > 0) {
    // TODO (jamuraa): Parse arguments and switch on --proto, --proto_text
    for (const auto& x : args) {
      VLOG(2) << __func__ << "argument: " << x.string();
    }
  }
  // TODO (jamuraa): enumerate profiles and dump profile information
  const bt_interface_t* iface =
      bluetooth::hal::BluetoothInterface::Get()->GetHALInterface();
  iface->dump(fd, NULL);
  return android::NO_ERROR;
}

void BluetoothBinderServer::OnAdapterStateChanged(
    bluetooth::Adapter* adapter, bluetooth::AdapterState prev_state,
    bluetooth::AdapterState new_state) {
  CHECK_EQ(adapter, adapter_);
  VLOG(2) << "Received adapter state update - prev: " << prev_state
          << " new: " << new_state;
  callbacks_.ForEach([prev_state, new_state](IBluetoothCallback* callback) {
    callback->OnBluetoothStateChange(prev_state, new_state);
  });
}

}  // namespace binder
}  // namespace ipc
