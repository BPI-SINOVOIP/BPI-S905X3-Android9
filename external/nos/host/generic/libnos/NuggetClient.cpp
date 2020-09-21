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

#include <nos/NuggetClient.h>

#include <limits>

#include <nos/transport.h>

#include <application.h>

namespace nos {

NuggetClient::NuggetClient()
    : NuggetClient("") {
}

NuggetClient::NuggetClient(const std::string& device_name)
    : device_name_(device_name), open_(false) {
}

NuggetClient::~NuggetClient() {
  Close();
}

void NuggetClient::Open() {
  if (!open_) {
    open_ = nos_device_open(
        device_name_.empty() ? nullptr : device_name_.c_str(), &device_) == 0;
  }
}

void NuggetClient::Close() {
  if (open_) {
    device_.ops.close(device_.ctx);
    open_ = false;
  }
}

bool NuggetClient::IsOpen() const {
  return open_;
}

uint32_t NuggetClient::CallApp(uint32_t appId, uint16_t arg,
                               const std::vector<uint8_t>& request,
                               std::vector<uint8_t>* response) {
  if (!open_) {
    return APP_ERROR_IO;
  }

  if (request.size() > std::numeric_limits<uint32_t>::max()) {
    return APP_ERROR_TOO_MUCH;
  }

  const uint32_t requestSize = request.size();
  uint32_t replySize = 0;
  uint8_t* replyData = nullptr;

  if (response != nullptr) {
    response->resize(response->capacity());
    replySize = response->size();
    replyData = response->data();
  }

  uint32_t status_code = nos_call_application(&device_, appId, arg,
                                              request.data(), requestSize,
                                              replyData, &replySize);

  if (response != nullptr) {
    response->resize(replySize);
  }

  return status_code;
}

nos_device* NuggetClient::Device() {
  return open_ ? &device_ : nullptr;
}

const nos_device* NuggetClient::Device() const {
  return open_ ? &device_ : nullptr;
}

const std::string& NuggetClient::DeviceName() const {
  return device_name_;
}

}  // namespace nos
