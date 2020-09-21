/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "connection"

#include "connection.h"

#include "base/logging.h"

#include "osi/include/log.h"

using std::vector;

namespace test_vendor_lib {

// Add an action from the controller.
void Connection::AddAction(const TaskCallback& task) { actions_.push(task); }

// Model the quality of the downstream connection
void Connection::SendToDevice() {
  while (actions_.size() > 0) {
    // Execute the first action
    actions_.front()();
    actions_.pop();
  }
}

// Add a message from the device to the controller.
void Connection::AddMessage(const vector<uint8_t>& data) {
  messages_.push(data);
}

// Model the quality of the upstream connection
bool Connection::ReceiveFromDevice(vector<uint8_t>& data) {
  if (messages_.size() > 0) {
    data = messages_.front();
    messages_.pop();

    return true;
  }
  return false;
}

const std::string Connection::ToString() {
  return "connection " + std::to_string(handle_) + " to " + dev_->ToString();
}

}  // namespace test_vendor_lib
