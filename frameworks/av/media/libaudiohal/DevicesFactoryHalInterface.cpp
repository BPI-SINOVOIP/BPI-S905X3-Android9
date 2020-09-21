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

#include <android/hardware/audio/2.0/IDevicesFactory.h>
#include <android/hardware/audio/4.0/IDevicesFactory.h>

#include <DevicesFactoryHalHybrid.h>
#include <libaudiohal/4.0/DevicesFactoryHalHybrid.h>

namespace android {

// static
sp<DevicesFactoryHalInterface> DevicesFactoryHalInterface::create() {
    if (hardware::audio::V4_0::IDevicesFactory::getService() != nullptr) {
        return new V4_0::DevicesFactoryHalHybrid();
    }
    if (hardware::audio::V2_0::IDevicesFactory::getService() != nullptr) {
        return new DevicesFactoryHalHybrid();
    }
    return nullptr;
}

} // namespace android
