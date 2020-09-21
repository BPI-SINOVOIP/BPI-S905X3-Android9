/*
**
** Copyright 2012, The Android Open Source Project
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

#ifndef ANDROID_AUDIO_HOTPLUG_THREAD_H
#define ANDROID_AUDIO_HOTPLUG_THREAD_H

#include <utils/threads.h>

namespace android {

class AudioHotplugThread : public Thread {
  public:
    struct DeviceInfo {
        unsigned int pcmCard;
        unsigned int pcmDevice;
        unsigned int minSampleBits, maxSampleBits;
        unsigned int minChannelCount, maxChannelCount;
        unsigned int minSampleRate, maxSampleRate;
        unsigned int hidraw_index; //hidraw index
        unsigned int hidraw_device; //hidraw bus type
        bool valid;
        bool forVoiceRecognition;
    };

    class Callback {
      public:
        virtual ~Callback() {}
        virtual void onDeviceFound(const DeviceInfo& devInfo, bool fgHidraw = false) = 0;
        virtual void onDeviceRemoved(unsigned int pcmCard, unsigned int pcmDevice) = 0;
        virtual void onDeviceRemoved(unsigned int hidrawIndex) = 0;
        virtual bool onDeviceNotify() = 0;
    };

    AudioHotplugThread(Callback& callback);
    virtual ~AudioHotplugThread();

    bool        start();
    void        shutdown();
    void        polling(bool flag);

  protected:
    void scanForDevice();
    void scanHidrawDevice();
    void scanSoundCardDevice();
    void handleHidrawEvent(struct inotify_event *event);
    void handleSoundCardEvent(struct inotify_event *event);
    int handleDeviceEvent(int inotifyFD, int wfds[]);
    int handleSignalEvent(int fd, int& param);

  private:
    static const uint64_t kShutdown;
    static const uint64_t kStartPoll;
    static const uint64_t kStopPoll;
    static const char* kThreadName;
    static const char* kDeviceDir;
    static const char* kAlsaDeviceDir;
    static const char* kHidrawDeviceDir;
    static const char  kDeviceTypeCapture;
    static bool parseCaptureDeviceName(const char *name, unsigned int *pcmCard,
                                       unsigned int *pcmDevice);
    static bool getDeviceInfo(unsigned int pcmCard, unsigned int pcmDevice,
                              DeviceInfo* info);
    bool getDeviceInfo(const unsigned int hidrawIndex, DeviceInfo* info);

    virtual bool threadLoop();

    Callback& mCallback;
    int mSignalEventFD;
};

}; // namespace android

#endif  // ANDROID_AUDIO_HOTPLUG_THREAD_H
