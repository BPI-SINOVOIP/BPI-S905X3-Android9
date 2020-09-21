/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_VERSION_UTILS_4_0_H
#define ANDROID_HARDWARE_VERSION_UTILS_4_0_H

#include <android/hardware/audio/4.0/types.h>
#include <hidl/HidlSupport.h>

using ::android::hardware::audio::V4_0::ParameterValue;
using ::android::hardware::audio::V4_0::Result;
using ::android::hardware::Return;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;

namespace android {
namespace V4_0 {
namespace utils {

template <class T, class Callback>
Return<void> getParameters(T& object, hidl_vec<ParameterValue> context,
                           hidl_vec<hidl_string> keys, Callback callback) {
    return object->getParameters(context, keys, callback);
}

template <class T>
Return<Result> setParameters(T& object, hidl_vec<ParameterValue> context,
                             hidl_vec<ParameterValue> keys) {
    return object->setParameters(context, keys);
}

} // namespace utils
} // namespace V4_0
} // namespace android

#endif // ANDROID_HARDWARE_VERSION_UTILS_4_0_H
