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

// All static variables go here, to control initialization and
// destruction order in the library.

#include <hidl/Static.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <utils/Mutex.h>

namespace android {
namespace hardware {
namespace details {

Mutex gDefaultServiceManagerLock;
sp<android::hidl::manager::V1_0::IServiceManager> gDefaultServiceManager;

// Deprecated; kept for ABI compatibility. Use getBnConstructorMap.
BnConstructorMap gBnConstructorMap{};

ConcurrentMap<const ::android::hidl::base::V1_0::IBase*, wp<::android::hardware::BHwBinder>>
    gBnMap{};

ConcurrentMap<wp<::android::hidl::base::V1_0::IBase>, SchedPrio> gServicePrioMap{};

// Deprecated; kept for ABI compatibility. Use getBsConstructorMap.
BsConstructorMap gBsConstructorMap{};

// For static executables, it is not guaranteed that gBnConstructorMap are initialized before
// used in HAL definition libraries.
BnConstructorMap& getBnConstructorMap() {
    static BnConstructorMap map{};
    return map;
}

BsConstructorMap& getBsConstructorMap() {
    static BsConstructorMap map{};
    return map;
}

}  // namespace details
}  // namespace hardware
}  // namespace android
