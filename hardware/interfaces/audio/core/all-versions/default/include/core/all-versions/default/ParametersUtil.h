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

#include <common/all-versions/IncludeGuard.h>

#include <functional>
#include <memory>

#include <hidl/HidlSupport.h>
#include <media/AudioParameter.h>

namespace android {
namespace hardware {
namespace audio {
namespace AUDIO_HAL_VERSION {
namespace implementation {

using ::android::hardware::audio::AUDIO_HAL_VERSION::DeviceAddress;
using ::android::hardware::audio::AUDIO_HAL_VERSION::ParameterValue;
using ::android::hardware::audio::AUDIO_HAL_VERSION::Result;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

class ParametersUtil {
   public:
    Result setParam(const char* name, const char* value);
    Result getParam(const char* name, bool* value);
    Result getParam(const char* name, int* value);
    Result getParam(const char* name, String8* value, AudioParameter context = {});
    void getParametersImpl(
        const hidl_vec<ParameterValue>& context, const hidl_vec<hidl_string>& keys,
        std::function<void(Result retval, const hidl_vec<ParameterValue>& parameters)> cb);
    std::unique_ptr<AudioParameter> getParams(const AudioParameter& keys);
    Result setParam(const char* name, bool value);
    Result setParam(const char* name, int value);
    Result setParam(const char* name, float value);
    Result setParametersImpl(const hidl_vec<ParameterValue>& context,
                             const hidl_vec<ParameterValue>& parameters);
    Result setParams(const AudioParameter& param);
    Result setParam(const char* name, const DeviceAddress& address);

   protected:
    virtual ~ParametersUtil() {}

    virtual char* halGetParameters(const char* keys) = 0;
    virtual int halSetParameters(const char* keysAndValues) = 0;
};

}  // namespace implementation
}  // namespace AUDIO_HAL_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
