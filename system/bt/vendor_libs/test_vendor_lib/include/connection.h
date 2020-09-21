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

#pragma once

#include <cstdint>
#include <queue>
#include <string>
#include <vector>

#include "async_manager.h"
#include "device.h"

#include "hci/include/hci_hal.h"

namespace test_vendor_lib {

// Model the connection of a device to the controller.
class Connection {
 public:
  Connection(std::shared_ptr<Device> dev, uint16_t handle)
      : dev_(dev), handle_(handle), connected_(true), encrypted_(false) {}

  virtual ~Connection() = default;

  // Return a string representing the connection for logging.
  const std::string ToString();

  // Return a pointer to the device in the connection.
  std::shared_ptr<Device> GetDevice() { return dev_; }

  // Return true if the handle matches and the device is connected.
  inline bool operator==(uint16_t handle) {
    return (handle_ == handle) && connected_;
  }

  // Return true if the handle doesn't match or the device is not connected.
  inline bool operator!=(uint16_t handle) {
    return (handle_ != handle) || !connected_;
  }

  void Disconnect() { connected_ = false; };
  bool Connected() { return connected_; };

  void Encrypt() { encrypted_ = true; };
  bool Encrypted() { return encrypted_; };

  // Add an action to the connection queue.
  void AddAction(const TaskCallback& task);

  // Execute the next action in the connection queue to simulate packet
  // exchange.
  void SendToDevice();

  // Add a message from the device.
  void AddMessage(const std::vector<uint8_t>& message);

  // Receive data from the device to simulate packet exchange.
  bool ReceiveFromDevice(std::vector<uint8_t>& data);

 private:
  // A shared pointer to the connected device
  std::shared_ptr<Device> dev_;

  // The connection handle
  uint16_t handle_;

  // State variables
  bool connected_;
  bool encrypted_;

  // Actions for the next packet exchange.
  std::queue<TaskCallback> actions_;

  // Messages from the device for the next packet exchange.
  std::queue<std::vector<uint8_t>> messages_;
};

}  // namespace test_vendor_lib
