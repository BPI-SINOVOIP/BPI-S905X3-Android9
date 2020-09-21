/*
 * libjingle
 * Copyright 2015 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_APP_WEBRTC_MEDIACONTROLLER_H_
#define TALK_APP_WEBRTC_MEDIACONTROLLER_H_

#include "webrtc/base/thread.h"

namespace cricket {
class ChannelManager;
}  // namespace cricket

namespace webrtc {
class Call;
class VoiceEngine;

// The MediaController currently owns shared state between media channels, but
// in the future will create and own RtpSenders and RtpReceivers.
class MediaControllerInterface {
 public:
  static MediaControllerInterface* Create(
      rtc::Thread* worker_thread,
      cricket::ChannelManager* channel_manager);

  virtual ~MediaControllerInterface() {}
  virtual webrtc::Call* call_w() = 0;
  virtual cricket::ChannelManager* channel_manager() const = 0;
};
}  // namespace webrtc

#endif  // TALK_APP_WEBRTC_MEDIACONTROLLER_H_
