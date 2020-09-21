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

#include <common/all-versions/IncludeGuard.h>

#include <algorithm>
#include <vector>

#include <system/audio.h>

namespace android {
namespace hardware {
namespace audio {
namespace AUDIO_HAL_VERSION {
namespace implementation {

using ::android::hardware::audio::AUDIO_HAL_VERSION::Result;

/** @return true if gain is between 0 and 1 included. */
constexpr bool isGainNormalized(float gain) {
    return gain >= 0.0 && gain <= 1.0;
}

namespace util {

template <typename T>
inline bool element_in(T e, const std::vector<T>& v) {
    return std::find(v.begin(), v.end(), e) != v.end();
}

static inline Result analyzeStatus(status_t status) {
    switch (status) {
        case 0:
            return Result::OK;
        case -EINVAL:
            return Result::INVALID_ARGUMENTS;
        case -ENODATA:
            return Result::INVALID_STATE;
        case -ENODEV:
            return Result::NOT_INITIALIZED;
        case -ENOSYS:
            return Result::NOT_SUPPORTED;
        default:
            return Result::INVALID_STATE;
    }
}

static inline Result analyzeStatus(const char* className, const char* funcName, status_t status,
                                   const std::vector<int>& ignoreErrors = {}) {
    if (status != 0 && !element_in(-status, ignoreErrors)) {
        ALOGW("Error from HAL %s in function %s: %s", className, funcName, strerror(-status));
    }
    return analyzeStatus(status);
}

}  // namespace util
}  // namespace implementation
}  // namespace AUDIO_HAL_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
