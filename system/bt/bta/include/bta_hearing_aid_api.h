/******************************************************************************
 *
 *  Copyright 2018 The Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <base/callback_forward.h>
#include <hardware/bt_hearing_aid.h>


/** Implementations of HearingAid will also implement this interface */
class HearingAidAudioReceiver {
 public:
  virtual ~HearingAidAudioReceiver() = default;
  virtual void OnAudioDataReady(const std::vector<uint8_t>& data) = 0;
  virtual void OnAudioSuspend();
  virtual void OnAudioResume();
};

class HearingAid {
 public:
  virtual ~HearingAid() = default;

  static void Initialize(bluetooth::hearing_aid::HearingAidCallbacks* callbacks,
                         base::Closure initCb);
  static void CleanUp();
  static bool IsInitialized();
  static HearingAid* Get();
  static void DebugDump(int fd);

  static void AddFromStorage(const RawAddress& address, uint16_t psm,
                             uint8_t capabilities, uint16_t codec,
                             uint16_t audioControlPointHandle,
                             uint16_t volumeHandle, uint64_t hiSyncId,
                             uint16_t render_delay, uint16_t preparation_delay,
                             uint16_t is_white_listed);

  static int GetDeviceCount();

  virtual void Connect(const RawAddress& address) = 0;
  virtual void Disconnect(const RawAddress& address) = 0;
  virtual void SetVolume(int8_t volume) = 0;
};

/* Represents configuration of audio codec, as exchanged between hearing aid and
 * phone.
 * It can also be passed to the audio source to configure its parameters.
 */
struct CodecConfiguration {
  /** sampling rate that the codec expects to receive from audio framework */
  uint32_t sample_rate;

  /** bitrate that codec expects to receive from audio framework in bits per
   * channel */
  uint32_t bit_rate;

  /** Data interval determines how often we send samples to the remote. This
   * should match how often we grab data from audio source, optionally we can
   * grab data every 2 or 3 intervals, but this would increase latency.
   *
   * Value is provided in ms, must be divisable by 1.25 to make sure the
   * connection interval is integer.
   */
  uint16_t data_interval_ms;
};

/** Represents source of audio for hearing aids */
class HearingAidAudioSource {
 public:
  static void Start(const CodecConfiguration& codecConfiguration,
                    HearingAidAudioReceiver* audioReceiver);
  static void Stop();
  static void Initialize();
  static void CleanUp();
  static void DebugDump(int fd);
};
