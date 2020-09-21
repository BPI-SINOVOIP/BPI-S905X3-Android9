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

#include "bluetooth_socket_manager.h"
#include "permission_helpers.h"

#include <base/logging.h>
#include <binder/IPCThreadState.h>

using ::android::OK;
using ::android::String8;
using ::android::binder::Status;
using ::android::os::ParcelFileDescriptor;
using ::android::os::ParcelUuid;

namespace android {
namespace bluetooth {

Status BluetoothSocketManagerBinderServer::connectSocket(
    const BluetoothDevice& device, int32_t type,
    const std::unique_ptr<ParcelUuid>& uuid, int32_t port, int32_t flag,
    std::unique_ptr<ParcelFileDescriptor>* _aidl_return) {
  if (!isCallerActiveUserOrManagedProfile()) {
    LOG(WARNING) << "connectSocket() - Not allowed for non-active users";
    return Status::fromExceptionCode(
        Status::EX_SECURITY, String8("Not allowed for non-active users"));
  }

  ENFORCE_PERMISSION(PERMISSION_BLUETOOTH);

  IPCThreadState* ipc = IPCThreadState::self();

  int socket_fd = -1;
  bt_status_t status = socketInterface->connect(
      &device.address, (btsock_type_t)type, uuid ? &uuid->uuid : nullptr, port,
      &socket_fd, flag, ipc->getCallingUid());
  if (status != BT_STATUS_SUCCESS) {
    LOG(ERROR) << "Socket connection failed: " << +status;
    socket_fd = -1;
  }

  if (socket_fd < 0) {
    LOG(ERROR) << "Fail to create file descriptor on socket fd";
    return Status::ok();
  }

  _aidl_return->reset(new ParcelFileDescriptor());
  (*_aidl_return)->setFileDescriptor(socket_fd, true);
  return Status::ok();
}

Status BluetoothSocketManagerBinderServer::createSocketChannel(
    int32_t type, const std::unique_ptr<::android::String16>& serviceName,
    const std::unique_ptr<ParcelUuid>& uuid, int32_t port, int32_t flag,
    std::unique_ptr<ParcelFileDescriptor>* _aidl_return) {
  if (!isCallerActiveUserOrManagedProfile()) {
    LOG(WARNING) << "createSocketChannel() - Not allowed for non-active users";
    return Status::fromExceptionCode(
        Status::EX_SECURITY, String8("Not allowed for non-active users"));
  }

  ENFORCE_PERMISSION(PERMISSION_BLUETOOTH);

  VLOG(1) << __func__ << ": SOCK FLAG=" << flag;

  IPCThreadState* ipc = IPCThreadState::self();
  int socket_fd = -1;

  const std::string payload_url{};

  bt_status_t status = socketInterface->listen(
      (btsock_type_t)type,
      serviceName ? String8{*serviceName}.c_str() : nullptr,
      uuid ? &uuid->uuid : nullptr, port, &socket_fd, flag,
      ipc->getCallingUid());

  if (status != BT_STATUS_SUCCESS) {
    LOG(ERROR) << "Socket listen failed: " << +status;
    socket_fd = -1;
  }

  if (socket_fd < 0) {
    LOG(ERROR) << "Failed to create file descriptor on socket fd";
    return Status::ok();
  }

  _aidl_return->reset(new ParcelFileDescriptor());
  (*_aidl_return)->setFileDescriptor(socket_fd, true);
  return Status::ok();
}

Status BluetoothSocketManagerBinderServer::requestMaximumTxDataLength(
    const BluetoothDevice& device) {
  if (!isCallerActiveUserOrManagedProfile()) {
    LOG(WARNING) << __func__ << ": Not allowed for non-active users";
    return Status::fromExceptionCode(
        Status::EX_SECURITY, String8("Not allowed for non-active users"));
  }

  ENFORCE_PERMISSION(PERMISSION_BLUETOOTH);

  VLOG(1) << __func__;

  socketInterface->request_max_tx_data_length(device.address);
  return Status::ok();
}

}  // namespace bluetooth
}  // namespace android
