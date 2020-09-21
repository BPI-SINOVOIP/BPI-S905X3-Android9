/*
 * Copyright (C) 2017 Amlogic Corporation.
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

#include <stdint.h>
#include <sys/types.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <AudioPolicyManager.h>

#ifndef ANDROID_DLG_AUDIO_POLICY_MANAGER_H
#define ANDROID_DLG_AUDIO_POLICY_MANAGER_H

namespace android {

class DLGAudioPolicyManager: public AudioPolicyManager
{
public:
    DLGAudioPolicyManager(AudioPolicyClientInterface *clientInterface);
    virtual ~DLGAudioPolicyManager() {}

    virtual status_t setDeviceConnectionState(audio_devices_t device,
                                              audio_policy_dev_state_t state,
                                              const char *device_address,
                                              const char *device_name);

    virtual audio_devices_t getDeviceForInputSource(audio_source_t inputSource);

protected:
    virtual float computeVolume(audio_stream_type_t stream,
                                int index,
                                audio_devices_t device);

private:
    // Flag which indicates whether to record from the submix device.
    bool mForceSubmixInputSelection;
};

}  // namespace android
#endif  // DLG_ANDROID_AUDIO_POLICY_MANAGER_H
