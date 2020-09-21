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

#ifndef ANDROID_HARDWARE_DEVICES_FACTORY_HAL_HYBRID_H
#define ANDROID_HARDWARE_DEVICES_FACTORY_HAL_HYBRID_H

#include <media/audiohal/DevicesFactoryHalInterface.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>

namespace android {

class DevicesFactoryHalHybrid : public DevicesFactoryHalInterface
{
  public:
    // Opens a device with the specified name. To close the device, it is
    // necessary to release references to the returned object.
    virtual status_t openDevice(const char *name, sp<DeviceHalInterface> *device);

  private:
    friend class DevicesFactoryHalInterface;

    // Can not be constructed directly by clients.
    DevicesFactoryHalHybrid();

    virtual ~DevicesFactoryHalHybrid();

    sp<DevicesFactoryHalInterface> mLocalFactory;
    sp<DevicesFactoryHalInterface> mHidlFactory;
};

} // namespace android

#endif // ANDROID_HARDWARE_DEVICES_FACTORY_HAL_HYBRID_H
