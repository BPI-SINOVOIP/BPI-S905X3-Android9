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

#define LOG_TAG "device_factory"

#include "device_factory.h"
#include "beacon.h"
#include "beacon_swarm.h"
#include "broken_adv.h"
#include "classic.h"
#include "device.h"
#include "keyboard.h"

#include "base/logging.h"

using std::vector;

namespace test_vendor_lib {

std::shared_ptr<Device> DeviceFactory::Create(const vector<std::string>& args) {
  CHECK(!args.empty());

  std::shared_ptr<Device> new_device = nullptr;

  if (args[0] == "beacon") new_device = std::make_shared<Beacon>();
  if (args[0] == "beacon_swarm") new_device = std::make_shared<BeaconSwarm>();
  if (args[0] == "broken_adv") new_device = std::make_shared<BrokenAdv>();
  if (args[0] == "classic") new_device = std::make_shared<Classic>();
  if (args[0] == "keyboard") new_device = std::make_shared<Keyboard>();

  if (new_device != nullptr) new_device->Initialize(args);

  return new_device;
}

}  // namespace test_vendor_lib
