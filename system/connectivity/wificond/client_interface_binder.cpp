/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "wificond/client_interface_binder.h"

#include <vector>

#include <binder/Status.h>

#include "wificond/client_interface_impl.h"

using android::binder::Status;
using android::net::wifi::IWifiScannerImpl;
using std::vector;

namespace android {
namespace wificond {

ClientInterfaceBinder::ClientInterfaceBinder(ClientInterfaceImpl* impl)
    : impl_(impl) {
}

ClientInterfaceBinder::~ClientInterfaceBinder() {
}

Status ClientInterfaceBinder::getPacketCounters(
    vector<int32_t>* out_packet_counters) {
  if (impl_ == nullptr) {
    return Status::ok();
  }
  impl_->GetPacketCounters(out_packet_counters);
  return Status::ok();
}

Status ClientInterfaceBinder::signalPoll(
    vector<int32_t>* out_signal_poll_results) {
  if (impl_ == nullptr) {
    return Status::ok();
  }
  impl_->SignalPoll(out_signal_poll_results);
  return Status::ok();
}

Status ClientInterfaceBinder::getMacAddress(vector<uint8_t>* out_mac_address) {
  if (impl_ == nullptr) {
    return Status::ok();
  }
  *out_mac_address = impl_->GetMacAddress();
  return Status::ok();
}

Status ClientInterfaceBinder::getInterfaceName(std::string* out_name) {
  if (impl_ == nullptr) {
    return Status::ok();
  }
  *out_name = impl_->GetInterfaceName();
  return Status::ok();
}

Status ClientInterfaceBinder::getWifiScannerImpl(
    sp<IWifiScannerImpl>* out_wifi_scanner_impl) {
  if (impl_ == nullptr) {
    *out_wifi_scanner_impl = nullptr;
    return Status::ok();
  }
  *out_wifi_scanner_impl = impl_->GetScanner();
  return Status::ok();
}


Status ClientInterfaceBinder::setMacAddress(const vector<uint8_t>& mac, bool* success) {
  *success = impl_ && impl_->SetMacAddress(mac);
  return Status::ok();
}

}  // namespace wificond
}  // namespace android
