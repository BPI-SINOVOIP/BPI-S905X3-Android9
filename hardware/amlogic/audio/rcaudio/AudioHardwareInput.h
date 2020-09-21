/*
**
** Copyright 2014, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_AUDIO_HARDWARE_INPUT_H
#define ANDROID_AUDIO_HARDWARE_INPUT_H

#include <hardware/audio.h>
#include <system/audio.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include <utils/Vector.h>

#include "AudioHotplugThread.h"

namespace android {

class AudioStreamIn;

class AudioHardwareInput : public AudioHotplugThread::Callback {
  public:
    AudioHardwareInput();
    ~AudioHardwareInput();

    status_t    initCheck();
    status_t    setParameters(const char* kvpairs);
    char*       getParameters(const char* keys);
    status_t    setMicMute(bool mute);
    status_t    getMicMute(bool* mute);
    status_t    getInputBufferSize(const audio_config* config);

    AudioStreamIn* openInputStream(struct audio_stream_in* stream,
                                   audio_format_t* format,
                                   uint32_t* channelMask,
                                   uint32_t* sampleRate,
                                   status_t* status);
    void           closeInputStream(AudioStreamIn* in);

    void setRemoteControlMicEnabled(bool flag);
    bool setRemoteControlDeviceStatus(bool flag);

    // AudioHotplugThread callbacks
    virtual void onDeviceFound(const AudioHotplugThread::DeviceInfo& devInfo, bool fgHidraw = false);
    virtual void onDeviceRemoved(unsigned int pcmCard, unsigned int pcmDevice);
    virtual void onDeviceRemoved(unsigned int hidrawIndex);
    virtual bool onDeviceNotify();

    static size_t calculateInputBufferSize(uint32_t outputSampleRate,
                                           audio_format_t format,
                                           uint32_t channelCount);

    static const uint32_t kPeriodMsec;

    /**
     * Decide which device to use for the given input.
     */
    const AudioHotplugThread::DeviceInfo* getBestDevice(int inputSource);

    enum {
      SOUNDCARD_DEVICE,
      HIDRAW_DEVICE,
      MAX_DEVICE_TYPE
    };

  private:
    static const int    kMaxDevices = 8; // TODO review strategy for adding more devices
    void                closeAllInputStreams();
    // Place all input streams using the specified device into standby. If deviceInfo is NULL,
    // all input streams are placed into standby.
    void                standbyAllInputStreams(const AudioHotplugThread::DeviceInfo* deviceInfo);
    Mutex               mLock;
    bool                mMicMute;
    Vector<AudioStreamIn*> mInputStreams;

    sp<AudioHotplugThread> mHotplugThread;

    AudioHotplugThread::DeviceInfo mDeviceInfos[MAX_DEVICE_TYPE][kMaxDevices]; //0:sound card, 1: hidraw
};

}; // namespace android

#endif  // ANDROID_AUDIO_HARDWARE_INPUT_H
