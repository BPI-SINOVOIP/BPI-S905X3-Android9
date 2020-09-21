/*
 * libjingle
 * Copyright 2014 Google Inc.
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

#ifndef TALK_MEDIA_BASE_VIDEOENGINE_UNITTEST_H_  // NOLINT
#define TALK_MEDIA_BASE_VIDEOENGINE_UNITTEST_H_

#include <string>
#include <vector>

#include "talk/media/base/fakenetworkinterface.h"
#include "talk/media/base/fakevideocapturer.h"
#include "talk/media/base/fakevideorenderer.h"
#include "talk/media/base/mediachannel.h"
#include "talk/media/base/streamparams.h"
#include "talk/media/webrtc/fakewebrtccall.h"
#include "webrtc/base/bytebuffer.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/call.h"

#define EXPECT_FRAME_WAIT(c, w, h, t) \
  EXPECT_EQ_WAIT((c), renderer_.num_rendered_frames(), (t)); \
  EXPECT_EQ((w), renderer_.width()); \
  EXPECT_EQ((h), renderer_.height()); \
  EXPECT_EQ(0, renderer_.errors()); \

#define EXPECT_FRAME_ON_RENDERER_WAIT(r, c, w, h, t) \
  EXPECT_EQ_WAIT((c), (r).num_rendered_frames(), (t)); \
  EXPECT_EQ((w), (r).width()); \
  EXPECT_EQ((h), (r).height()); \
  EXPECT_EQ(0, (r).errors()); \

#define EXPECT_GT_FRAME_ON_RENDERER_WAIT(r, c, w, h, t) \
  EXPECT_TRUE_WAIT((r).num_rendered_frames() >= (c) && \
                   (w) == (r).width() && \
                   (h) == (r).height(), (t)); \
  EXPECT_EQ(0, (r).errors());

static const uint32_t kTimeout = 5000U;
static const uint32_t kDefaultReceiveSsrc = 0;
static const uint32_t kSsrc = 1234u;
static const uint32_t kRtxSsrc = 4321u;
static const uint32_t kSsrcs4[] = {1, 2, 3, 4};

inline bool IsEqualRes(const cricket::VideoCodec& a, int w, int h, int fps) {
  return a.width == w && a.height == h && a.framerate == fps;
}

inline bool IsEqualCodec(const cricket::VideoCodec& a,
                         const cricket::VideoCodec& b) {
  return a.id == b.id && a.name == b.name &&
      IsEqualRes(a, b.width, b.height, b.framerate);
}

namespace std {
inline std::ostream& operator<<(std::ostream& s, const cricket::VideoCodec& c) {
  s << "{" << c.name << "(" << c.id << "), "
    << c.width << "x" << c.height << "x" << c.framerate << "}";
  return s;
}
}  // namespace std

inline int TimeBetweenSend(const cricket::VideoCodec& codec) {
  return static_cast<int>(
      cricket::VideoFormat::FpsToInterval(codec.framerate) /
      rtc::kNumNanosecsPerMillisec);
}

// Fake video engine that makes it possible to test enabling and disabling
// capturer (checking that the engine state is updated and that the capturer
// is indeed capturing) without having to create a channel. It also makes it
// possible to test that the media processors are indeed being called when
// registered.
template<class T>
class VideoEngineOverride : public T {
 public:
  VideoEngineOverride() : T() {
  }
  virtual ~VideoEngineOverride() {
  }
  bool is_camera_on() const { return T::GetVideoCapturer()->IsRunning(); }
  void set_has_senders(bool has_senders) {
    cricket::VideoCapturer* video_capturer = T::GetVideoCapturer();
    if (has_senders) {
      video_capturer->SignalVideoFrame.connect(this,
          &VideoEngineOverride<T>::OnLocalFrame);
    } else {
      video_capturer->SignalVideoFrame.disconnect(this);
    }
  }
  void OnLocalFrame(cricket::VideoCapturer*,
                    const cricket::VideoFrame*) {
  }
  void OnLocalFrameFormat(cricket::VideoCapturer*,
                          const cricket::VideoFormat*) {
  }

  void TriggerMediaFrame(uint32_t ssrc,
                         cricket::VideoFrame* frame,
                         bool* drop_frame) {
    T::SignalMediaFrame(ssrc, frame, drop_frame);
  }
};

template<class E, class C>
class VideoMediaChannelTest : public testing::Test,
                              public sigslot::has_slots<> {
 protected:
  VideoMediaChannelTest<E, C>()
      : call_(webrtc::Call::Create(webrtc::Call::Config())) {}

  virtual cricket::VideoCodec DefaultCodec() = 0;

  virtual cricket::StreamParams DefaultSendStreamParams() {
    return cricket::StreamParams::CreateLegacy(kSsrc);
  }

  virtual void SetUp() {
    cricket::Device device("test", "device");
    engine_.Init();
    channel_.reset(
        engine_.CreateChannel(call_.get(), cricket::VideoOptions()));
    EXPECT_TRUE(channel_.get() != NULL);
    network_interface_.SetDestination(channel_.get());
    channel_->SetInterface(&network_interface_);
    media_error_ = cricket::VideoMediaChannel::ERROR_NONE;
    cricket::VideoRecvParameters parameters;
    parameters.codecs = engine_.codecs();
    channel_->SetRecvParameters(parameters);
    EXPECT_TRUE(channel_->AddSendStream(DefaultSendStreamParams()));
    video_capturer_.reset(CreateFakeVideoCapturer());
    cricket::VideoFormat format(640, 480,
                                cricket::VideoFormat::FpsToInterval(30),
                                cricket::FOURCC_I420);
    EXPECT_EQ(cricket::CS_RUNNING, video_capturer_->Start(format));
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, video_capturer_.get()));
  }

  virtual cricket::FakeVideoCapturer* CreateFakeVideoCapturer() {
    return new cricket::FakeVideoCapturer();
  }

  // Utility method to setup an additional stream to send and receive video.
  // Used to test send and recv between two streams.
  void SetUpSecondStream() {
    SetUpSecondStreamWithNoRecv();
    // Setup recv for second stream.
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc + 2)));
    // Make the second renderer available for use by a new stream.
    EXPECT_TRUE(channel_->SetRenderer(kSsrc + 2, &renderer2_));
  }
  // Setup an additional stream just to send video. Defer add recv stream.
  // This is required if you want to test unsignalled recv of video rtp packets.
  void SetUpSecondStreamWithNoRecv() {
    // SetUp() already added kSsrc make sure duplicate SSRCs cant be added.
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc)));
    EXPECT_TRUE(channel_->SetRenderer(kSsrc, &renderer_));
    EXPECT_FALSE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(kSsrc)));
    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(kSsrc + 2)));
    // We dont add recv for the second stream.

    // Setup the receive and renderer for second stream after send.
    video_capturer_2_.reset(CreateFakeVideoCapturer());
    cricket::VideoFormat format(640, 480,
                                cricket::VideoFormat::FpsToInterval(30),
                                cricket::FOURCC_I420);
    EXPECT_EQ(cricket::CS_RUNNING, video_capturer_2_->Start(format));

    EXPECT_TRUE(channel_->SetCapturer(kSsrc + 2, video_capturer_2_.get()));
  }
  virtual void TearDown() {
    channel_.reset();
  }
  bool SetDefaultCodec() {
    return SetOneCodec(DefaultCodec());
  }

  bool SetOneCodec(int pt, const char* name, int w, int h, int fr) {
    return SetOneCodec(cricket::VideoCodec(pt, name, w, h, fr, 0));
  }
  bool SetOneCodec(const cricket::VideoCodec& codec) {
    cricket::VideoFormat capture_format(codec.width, codec.height,
        cricket::VideoFormat::FpsToInterval(codec.framerate),
        cricket::FOURCC_I420);

    if (video_capturer_) {
      EXPECT_EQ(cricket::CS_RUNNING, video_capturer_->Start(capture_format));
    }
    if (video_capturer_2_) {
      EXPECT_EQ(cricket::CS_RUNNING, video_capturer_2_->Start(capture_format));
    }

    bool sending = channel_->sending();
    bool success = SetSend(false);
    if (success) {
      cricket::VideoSendParameters parameters;
      parameters.codecs.push_back(codec);
      success = channel_->SetSendParameters(parameters);
    }
    if (success) {
      success = SetSend(sending);
    }
    return success;
  }
  bool SetSend(bool send) {
    return channel_->SetSend(send);
  }
  bool SetSendStreamFormat(uint32_t ssrc, const cricket::VideoCodec& codec) {
    return channel_->SetSendStreamFormat(ssrc, cricket::VideoFormat(
        codec.width, codec.height,
        cricket::VideoFormat::FpsToInterval(codec.framerate),
        cricket::FOURCC_ANY));
  }
  int DrainOutgoingPackets() {
    int packets = 0;
    do {
      packets = NumRtpPackets();
      // 100 ms should be long enough.
      rtc::Thread::Current()->ProcessMessages(100);
    } while (NumRtpPackets() > packets);
    return NumRtpPackets();
  }
  bool SendFrame() {
    if (video_capturer_2_) {
      video_capturer_2_->CaptureFrame();
    }
    return video_capturer_.get() &&
        video_capturer_->CaptureFrame();
  }
  bool WaitAndSendFrame(int wait_ms) {
    bool ret = rtc::Thread::Current()->ProcessMessages(wait_ms);
    ret &= SendFrame();
    return ret;
  }
  // Sends frames and waits for the decoder to be fully initialized.
  // Returns the number of frames that were sent.
  int WaitForDecoder() {
#if defined(HAVE_OPENMAX)
    // Send enough frames for the OpenMAX decoder to continue processing, and
    // return the number of frames sent.
    // Send frames for a full kTimeout's worth of 15fps video.
    int frame_count = 0;
    while (frame_count < static_cast<int>(kTimeout) / 66) {
      EXPECT_TRUE(WaitAndSendFrame(66));
      ++frame_count;
    }
    return frame_count;
#else
    return 0;
#endif
  }
  bool SendCustomVideoFrame(int w, int h) {
    if (!video_capturer_.get()) return false;
    return video_capturer_->CaptureCustomFrame(w, h, cricket::FOURCC_I420);
  }
  int NumRtpBytes() {
    return network_interface_.NumRtpBytes();
  }
  int NumRtpBytes(uint32_t ssrc) {
    return network_interface_.NumRtpBytes(ssrc);
  }
  int NumRtpPackets() {
    return network_interface_.NumRtpPackets();
  }
  int NumRtpPackets(uint32_t ssrc) {
    return network_interface_.NumRtpPackets(ssrc);
  }
  int NumSentSsrcs() {
    return network_interface_.NumSentSsrcs();
  }
  const rtc::Buffer* GetRtpPacket(int index) {
    return network_interface_.GetRtpPacket(index);
  }
  int NumRtcpPackets() {
    return network_interface_.NumRtcpPackets();
  }
  const rtc::Buffer* GetRtcpPacket(int index) {
    return network_interface_.GetRtcpPacket(index);
  }
  static int GetPayloadType(const rtc::Buffer* p) {
    int pt = -1;
    ParseRtpPacket(p, NULL, &pt, NULL, NULL, NULL, NULL);
    return pt;
  }
  static bool ParseRtpPacket(const rtc::Buffer* p,
                             bool* x,
                             int* pt,
                             int* seqnum,
                             uint32_t* tstamp,
                             uint32_t* ssrc,
                             std::string* payload) {
    rtc::ByteBuffer buf(*p);
    uint8_t u08 = 0;
    uint16_t u16 = 0;
    uint32_t u32 = 0;

    // Read X and CC fields.
    if (!buf.ReadUInt8(&u08)) return false;
    bool extension = ((u08 & 0x10) != 0);
    uint8_t cc = (u08 & 0x0F);
    if (x) *x = extension;

    // Read PT field.
    if (!buf.ReadUInt8(&u08)) return false;
    if (pt) *pt = (u08 & 0x7F);

    // Read Sequence Number field.
    if (!buf.ReadUInt16(&u16)) return false;
    if (seqnum) *seqnum = u16;

    // Read Timestamp field.
    if (!buf.ReadUInt32(&u32)) return false;
    if (tstamp) *tstamp = u32;

    // Read SSRC field.
    if (!buf.ReadUInt32(&u32)) return false;
    if (ssrc) *ssrc = u32;

    // Skip CSRCs.
    for (uint8_t i = 0; i < cc; ++i) {
      if (!buf.ReadUInt32(&u32)) return false;
    }

    // Skip extension header.
    if (extension) {
      // Read Profile-specific extension header ID
      if (!buf.ReadUInt16(&u16)) return false;

      // Read Extension header length
      if (!buf.ReadUInt16(&u16)) return false;
      uint16_t ext_header_len = u16;

      // Read Extension header
      for (uint16_t i = 0; i < ext_header_len; ++i) {
        if (!buf.ReadUInt32(&u32)) return false;
      }
    }

    if (payload) {
      return buf.ReadString(payload, buf.Length());
    }
    return true;
  }

  // Parse all RTCP packet, from start_index to stop_index, and count how many
  // FIR (PT=206 and FMT=4 according to RFC 5104). If successful, set the count
  // and return true.
  bool CountRtcpFir(int start_index, int stop_index, int* fir_count) {
    int count = 0;
    for (int i = start_index; i < stop_index; ++i) {
      rtc::scoped_ptr<const rtc::Buffer> p(GetRtcpPacket(i));
      rtc::ByteBuffer buf(*p);
      size_t total_len = 0;
      // The packet may be a compound RTCP packet.
      while (total_len < p->size()) {
        // Read FMT, type and length.
        uint8_t fmt = 0;
        uint8_t type = 0;
        uint16_t length = 0;
        if (!buf.ReadUInt8(&fmt)) return false;
        fmt &= 0x1F;
        if (!buf.ReadUInt8(&type)) return false;
        if (!buf.ReadUInt16(&length)) return false;
        buf.Consume(length * 4);  // Skip RTCP data.
        total_len += (length + 1) * 4;
        if ((192 == type) || ((206 == type) && (4 == fmt))) {
          ++count;
        }
      }
    }

    if (fir_count) {
      *fir_count = count;
    }
    return true;
  }

  void OnVideoChannelError(uint32_t ssrc,
                           cricket::VideoMediaChannel::Error error) {
    media_error_ = error;
  }

  // Test that SetSend works.
  void SetSend() {
    EXPECT_FALSE(channel_->sending());
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, video_capturer_.get()));
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    EXPECT_FALSE(channel_->sending());
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->sending());
    EXPECT_TRUE(SendFrame());
    EXPECT_TRUE_WAIT(NumRtpPackets() > 0, kTimeout);
    EXPECT_TRUE(SetSend(false));
    EXPECT_FALSE(channel_->sending());
  }
  // Test that SetSend fails without codecs being set.
  void SetSendWithoutCodecs() {
    EXPECT_FALSE(channel_->sending());
    EXPECT_FALSE(SetSend(true));
    EXPECT_FALSE(channel_->sending());
  }
  // Test that we properly set the send and recv buffer sizes by the time
  // SetSend is called.
  void SetSendSetsTransportBufferSizes() {
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    EXPECT_TRUE(SetSend(true));
    EXPECT_EQ(64 * 1024, network_interface_.sendbuf_size());
    EXPECT_EQ(64 * 1024, network_interface_.recvbuf_size());
  }
  // Tests that we can send frames and the right payload type is used.
  void Send(const cricket::VideoCodec& codec) {
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(SendFrame());
    EXPECT_TRUE_WAIT(NumRtpPackets() > 0, kTimeout);
    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    EXPECT_EQ(codec.id, GetPayloadType(p.get()));
  }
  // Tests that we can send and receive frames.
  void SendAndReceive(const cricket::VideoCodec& codec) {
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_WAIT(1, codec.width, codec.height, kTimeout);
    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    EXPECT_EQ(codec.id, GetPayloadType(p.get()));
  }
  // Tests that we only get a VideoRenderer::SetSize() callback when needed.
  void SendManyResizeOnce() {
    cricket::VideoCodec codec(DefaultCodec());
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    EXPECT_TRUE(WaitAndSendFrame(30));
    EXPECT_FRAME_WAIT(1, codec.width, codec.height, kTimeout);
    EXPECT_TRUE(WaitAndSendFrame(30));
    EXPECT_FRAME_WAIT(2, codec.width, codec.height, kTimeout);
    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    EXPECT_EQ(codec.id, GetPayloadType(p.get()));
    EXPECT_EQ(1, renderer_.num_set_sizes());

    codec.width /= 2;
    codec.height /= 2;
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(WaitAndSendFrame(30));
    EXPECT_FRAME_WAIT(3, codec.width, codec.height, kTimeout);
    EXPECT_EQ(2, renderer_.num_set_sizes());
  }
  void SendReceiveManyAndGetStats(const cricket::VideoCodec& codec,
                                  int duration_sec, int fps) {
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    for (int i = 0; i < duration_sec; ++i) {
      for (int frame = 1; frame <= fps; ++frame) {
        EXPECT_TRUE(WaitAndSendFrame(1000 / fps));
        EXPECT_FRAME_WAIT(frame + i * fps, codec.width, codec.height, kTimeout);
      }
    }
    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    EXPECT_EQ(codec.id, GetPayloadType(p.get()));
  }

  // Test that stats work properly for a 1-1 call.
  void GetStats() {
    const int kDurationSec = 3;
    const int kFps = 10;
    SendReceiveManyAndGetStats(DefaultCodec(), kDurationSec, kFps);

    cricket::VideoMediaInfo info;
    EXPECT_TRUE(channel_->GetStats(&info));

    ASSERT_EQ(1U, info.senders.size());
    // TODO(whyuan): bytes_sent and bytes_rcvd are different. Are both payload?
    // For webrtc, bytes_sent does not include the RTP header length.
    EXPECT_GT(info.senders[0].bytes_sent, 0);
    EXPECT_EQ(NumRtpPackets(), info.senders[0].packets_sent);
    EXPECT_EQ(0.0, info.senders[0].fraction_lost);
    EXPECT_EQ(0, info.senders[0].firs_rcvd);
    EXPECT_EQ(0, info.senders[0].plis_rcvd);
    EXPECT_EQ(0, info.senders[0].nacks_rcvd);
    EXPECT_EQ(DefaultCodec().width, info.senders[0].send_frame_width);
    EXPECT_EQ(DefaultCodec().height, info.senders[0].send_frame_height);
    EXPECT_GT(info.senders[0].framerate_input, 0);
    EXPECT_GT(info.senders[0].framerate_sent, 0);

    ASSERT_EQ(1U, info.receivers.size());
    EXPECT_EQ(1U, info.senders[0].ssrcs().size());
    EXPECT_EQ(1U, info.receivers[0].ssrcs().size());
    EXPECT_EQ(info.senders[0].ssrcs()[0], info.receivers[0].ssrcs()[0]);
    EXPECT_EQ(NumRtpBytes(), info.receivers[0].bytes_rcvd);
    EXPECT_EQ(NumRtpPackets(), info.receivers[0].packets_rcvd);
    EXPECT_EQ(0.0, info.receivers[0].fraction_lost);
    EXPECT_EQ(0, info.receivers[0].packets_lost);
    // TODO(asapersson): Not set for webrtc. Handle missing stats.
    // EXPECT_EQ(0, info.receivers[0].packets_concealed);
    EXPECT_EQ(0, info.receivers[0].firs_sent);
    EXPECT_EQ(0, info.receivers[0].plis_sent);
    EXPECT_EQ(0, info.receivers[0].nacks_sent);
    EXPECT_EQ(DefaultCodec().width, info.receivers[0].frame_width);
    EXPECT_EQ(DefaultCodec().height, info.receivers[0].frame_height);
    EXPECT_GT(info.receivers[0].framerate_rcvd, 0);
    EXPECT_GT(info.receivers[0].framerate_decoded, 0);
    EXPECT_GT(info.receivers[0].framerate_output, 0);
  }

  cricket::VideoSenderInfo GetSenderStats(size_t i) {
    cricket::VideoMediaInfo info;
    EXPECT_TRUE(channel_->GetStats(&info));
    return info.senders[i];
  }

  cricket::VideoReceiverInfo GetReceiverStats(size_t i) {
    cricket::VideoMediaInfo info;
    EXPECT_TRUE(channel_->GetStats(&info));
    return info.receivers[i];
  }

  // Test that stats work properly for a conf call with multiple recv streams.
  void GetStatsMultipleRecvStreams() {
    cricket::FakeVideoRenderer renderer1, renderer2;
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    cricket::VideoSendParameters parameters;
    parameters.codecs.push_back(DefaultCodec());
    parameters.options.conference_mode = rtc::Optional<bool>(true);
    EXPECT_TRUE(channel_->SetSendParameters(parameters));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(1)));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(2)));
    EXPECT_TRUE(channel_->SetRenderer(1, &renderer1));
    EXPECT_TRUE(channel_->SetRenderer(2, &renderer2));
    EXPECT_EQ(0, renderer1.num_rendered_frames());
    EXPECT_EQ(0, renderer2.num_rendered_frames());
    std::vector<uint32_t> ssrcs;
    ssrcs.push_back(1);
    ssrcs.push_back(2);
    network_interface_.SetConferenceMode(true, ssrcs);
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer1, 1, DefaultCodec().width, DefaultCodec().height, kTimeout);
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer2, 1, DefaultCodec().width, DefaultCodec().height, kTimeout);

    EXPECT_TRUE(channel_->SetSend(false));

    cricket::VideoMediaInfo info;
    EXPECT_TRUE(channel_->GetStats(&info));
    ASSERT_EQ(1U, info.senders.size());
    // TODO(whyuan): bytes_sent and bytes_rcvd are different. Are both payload?
    // For webrtc, bytes_sent does not include the RTP header length.
    EXPECT_GT(GetSenderStats(0).bytes_sent, 0);
    EXPECT_EQ_WAIT(NumRtpPackets(), GetSenderStats(0).packets_sent, kTimeout);
    EXPECT_EQ(DefaultCodec().width, GetSenderStats(0).send_frame_width);
    EXPECT_EQ(DefaultCodec().height, GetSenderStats(0).send_frame_height);

    ASSERT_EQ(2U, info.receivers.size());
    for (size_t i = 0; i < info.receivers.size(); ++i) {
      EXPECT_EQ(1U, GetReceiverStats(i).ssrcs().size());
      EXPECT_EQ(i + 1, GetReceiverStats(i).ssrcs()[0]);
      EXPECT_EQ_WAIT(NumRtpBytes(), GetReceiverStats(i).bytes_rcvd, kTimeout);
      EXPECT_EQ_WAIT(NumRtpPackets(), GetReceiverStats(i).packets_rcvd,
                     kTimeout);
      EXPECT_EQ(DefaultCodec().width, GetReceiverStats(i).frame_width);
      EXPECT_EQ(DefaultCodec().height, GetReceiverStats(i).frame_height);
    }
  }
  // Test that stats work properly for a conf call with multiple send streams.
  void GetStatsMultipleSendStreams() {
    // Normal setup; note that we set the SSRC explicitly to ensure that
    // it will come first in the senders map.
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    cricket::VideoSendParameters parameters;
    parameters.codecs.push_back(DefaultCodec());
    parameters.options.conference_mode = rtc::Optional<bool>(true);
    EXPECT_TRUE(channel_->SetSendParameters(parameters));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc)));
    EXPECT_TRUE(channel_->SetRenderer(kSsrc, &renderer_));
    channel_->UpdateAspectRatio(640, 400);
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(SendFrame());
    EXPECT_TRUE_WAIT(NumRtpPackets() > 0, kTimeout);
    EXPECT_FRAME_WAIT(1, DefaultCodec().width, DefaultCodec().height, kTimeout);

    // Add an additional capturer, and hook up a renderer to receive it.
    cricket::FakeVideoRenderer renderer2;
    rtc::scoped_ptr<cricket::FakeVideoCapturer> capturer(
        CreateFakeVideoCapturer());
    capturer->SetScreencast(true);
    const int kTestWidth = 160;
    const int kTestHeight = 120;
    cricket::VideoFormat format(kTestWidth, kTestHeight,
                                cricket::VideoFormat::FpsToInterval(5),
                                cricket::FOURCC_I420);
    EXPECT_EQ(cricket::CS_RUNNING, capturer->Start(format));
    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(5678)));
    EXPECT_TRUE(channel_->SetCapturer(5678, capturer.get()));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(5678)));
    EXPECT_TRUE(channel_->SetRenderer(5678, &renderer2));
    EXPECT_TRUE(capturer->CaptureCustomFrame(
        kTestWidth, kTestHeight, cricket::FOURCC_I420));
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer2, 1, kTestWidth, kTestHeight, kTimeout);

    // Get stats, and make sure they are correct for two senders. We wait until
    // the number of expected packets have been sent to avoid races where we
    // check stats before it has been updated.
    cricket::VideoMediaInfo info;
    for (uint32_t i = 0; i < kTimeout; ++i) {
      rtc::Thread::Current()->ProcessMessages(1);
      EXPECT_TRUE(channel_->GetStats(&info));
      ASSERT_EQ(2U, info.senders.size());
      if (info.senders[0].packets_sent + info.senders[1].packets_sent ==
          NumRtpPackets()) {
        // Stats have been updated for both sent frames, expectations can be
        // checked now.
        break;
      }
    }
    EXPECT_EQ(NumRtpPackets(),
              info.senders[0].packets_sent + info.senders[1].packets_sent)
        << "Timed out while waiting for packet counts for all sent packets.";
    EXPECT_EQ(1U, info.senders[0].ssrcs().size());
    EXPECT_EQ(1234U, info.senders[0].ssrcs()[0]);
    EXPECT_EQ(DefaultCodec().width, info.senders[0].send_frame_width);
    EXPECT_EQ(DefaultCodec().height, info.senders[0].send_frame_height);
    EXPECT_EQ(1U, info.senders[1].ssrcs().size());
    EXPECT_EQ(5678U, info.senders[1].ssrcs()[0]);
    EXPECT_EQ(kTestWidth, info.senders[1].send_frame_width);
    EXPECT_EQ(kTestHeight, info.senders[1].send_frame_height);
    // The capturer must be unregistered here as it runs out of it's scope next.
    EXPECT_TRUE(channel_->SetCapturer(5678, NULL));
  }

  // Test that we can set the bandwidth.
  void SetSendBandwidth() {
    cricket::VideoSendParameters parameters;
    parameters.codecs.push_back(DefaultCodec());
    parameters.max_bandwidth_bps = -1;  // <= 0 means unlimited.
    EXPECT_TRUE(channel_->SetSendParameters(parameters));
    parameters.max_bandwidth_bps = 128 * 1024;
    EXPECT_TRUE(channel_->SetSendParameters(parameters));
  }
  // Test that we can set the SSRC for the default send source.
  void SetSendSsrc() {
    EXPECT_TRUE(SetDefaultCodec());
    EXPECT_TRUE(SetSendStreamFormat(kSsrc, DefaultCodec()));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(SendFrame());
    EXPECT_TRUE_WAIT(NumRtpPackets() > 0, kTimeout);
    uint32_t ssrc = 0;
    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    ParseRtpPacket(p.get(), NULL, NULL, NULL, NULL, &ssrc, NULL);
    EXPECT_EQ(kSsrc, ssrc);
    // Packets are being paced out, so these can mismatch between the first and
    // second call to NumRtpPackets until pending packets are paced out.
    EXPECT_EQ_WAIT(NumRtpPackets(), NumRtpPackets(ssrc), kTimeout);
    EXPECT_EQ_WAIT(NumRtpBytes(), NumRtpBytes(ssrc), kTimeout);
    EXPECT_EQ(1, NumSentSsrcs());
    EXPECT_EQ(0, NumRtpPackets(kSsrc - 1));
    EXPECT_EQ(0, NumRtpBytes(kSsrc - 1));
  }
  // Test that we can set the SSRC even after codecs are set.
  void SetSendSsrcAfterSetCodecs() {
    // Remove stream added in Setup.
    EXPECT_TRUE(channel_->RemoveSendStream(kSsrc));
    EXPECT_TRUE(SetDefaultCodec());
    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(999)));
    EXPECT_TRUE(channel_->SetCapturer(999u, video_capturer_.get()));
    EXPECT_TRUE(SetSendStreamFormat(999u, DefaultCodec()));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(WaitAndSendFrame(0));
    EXPECT_TRUE_WAIT(NumRtpPackets() > 0, kTimeout);
    uint32_t ssrc = 0;
    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    ParseRtpPacket(p.get(), NULL, NULL, NULL, NULL, &ssrc, NULL);
    EXPECT_EQ(999u, ssrc);
    // Packets are being paced out, so these can mismatch between the first and
    // second call to NumRtpPackets until pending packets are paced out.
    EXPECT_EQ_WAIT(NumRtpPackets(), NumRtpPackets(ssrc), kTimeout);
    EXPECT_EQ_WAIT(NumRtpBytes(), NumRtpBytes(ssrc), kTimeout);
    EXPECT_EQ(1, NumSentSsrcs());
    EXPECT_EQ(0, NumRtpPackets(kSsrc));
    EXPECT_EQ(0, NumRtpBytes(kSsrc));
  }
  // Test that we can set the default video renderer before and after
  // media is received.
  void SetRenderer() {
    uint8_t data1[] = {
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    rtc::Buffer packet1(data1, sizeof(data1));
    rtc::SetBE32(packet1.data() + 8, kSsrc);
    channel_->SetRenderer(kDefaultReceiveSsrc, NULL);
    EXPECT_TRUE(SetDefaultCodec());
    EXPECT_TRUE(SetSend(true));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    channel_->OnPacketReceived(&packet1, rtc::PacketTime());
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_WAIT(1, DefaultCodec().width, DefaultCodec().height, kTimeout);
  }

  // Tests empty StreamParams is rejected.
  void RejectEmptyStreamParams() {
    // Remove the send stream that was added during Setup.
    EXPECT_TRUE(channel_->RemoveSendStream(kSsrc));

    cricket::StreamParams empty;
    EXPECT_FALSE(channel_->AddSendStream(empty));
    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(789u)));
  }

  // Tests setting up and configuring a send stream.
  void AddRemoveSendStreams() {
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_WAIT(1, DefaultCodec().width, DefaultCodec().height, kTimeout);
    EXPECT_GT(NumRtpPackets(), 0);
    uint32_t ssrc = 0;
    size_t last_packet = NumRtpPackets() - 1;
    rtc::scoped_ptr<const rtc::Buffer>
        p(GetRtpPacket(static_cast<int>(last_packet)));
    ParseRtpPacket(p.get(), NULL, NULL, NULL, NULL, &ssrc, NULL);
    EXPECT_EQ(kSsrc, ssrc);

    // Remove the send stream that was added during Setup.
    EXPECT_TRUE(channel_->RemoveSendStream(kSsrc));
    int rtp_packets = NumRtpPackets();

    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(789u)));
    EXPECT_TRUE(channel_->SetCapturer(789u, video_capturer_.get()));
    EXPECT_EQ(rtp_packets, NumRtpPackets());
    // Wait 30ms to guarantee the engine does not drop the frame.
    EXPECT_TRUE(WaitAndSendFrame(30));
    EXPECT_TRUE_WAIT(NumRtpPackets() > rtp_packets, kTimeout);

    last_packet = NumRtpPackets() - 1;
    p.reset(GetRtpPacket(static_cast<int>(last_packet)));
    ParseRtpPacket(p.get(), NULL, NULL, NULL, NULL, &ssrc, NULL);
    EXPECT_EQ(789u, ssrc);
  }

  // Tests setting up and configuring multiple incoming streams.
  void AddRemoveRecvStreams() {
    cricket::FakeVideoRenderer renderer1, renderer2;
    cricket::VideoSendParameters parameters;
    parameters.codecs.push_back(DefaultCodec());
    EXPECT_TRUE(channel_->SetSendParameters(parameters));

    // Ensure we can't set the renderer on a non-existent stream.
    EXPECT_FALSE(channel_->SetRenderer(1, &renderer1));
    EXPECT_FALSE(channel_->SetRenderer(2, &renderer2));
    cricket::VideoRenderer* renderer;
    EXPECT_FALSE(channel_->GetRenderer(1, &renderer));
    EXPECT_FALSE(channel_->GetRenderer(2, &renderer));

    // Ensure we can add streams.
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(1)));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(2)));
    EXPECT_TRUE(channel_->GetRenderer(1, &renderer));
    EXPECT_TRUE(renderer == NULL);
    EXPECT_TRUE(channel_->GetRenderer(2, &renderer));
    EXPECT_TRUE(NULL == renderer);

    // Ensure we can now set the renderers.
    EXPECT_TRUE(channel_->SetRenderer(1, &renderer1));
    EXPECT_TRUE(channel_->SetRenderer(2, &renderer2));
    EXPECT_TRUE(channel_->GetRenderer(1, &renderer));
    EXPECT_TRUE(&renderer1 == renderer);
    EXPECT_TRUE(channel_->GetRenderer(2, &renderer));
    EXPECT_TRUE(&renderer2 == renderer);

    // Ensure we can change the renderers if needed.
    EXPECT_TRUE(channel_->SetRenderer(1, &renderer2));
    EXPECT_TRUE(channel_->SetRenderer(2, &renderer1));
    EXPECT_TRUE(channel_->GetRenderer(1, &renderer));
    EXPECT_TRUE(&renderer2 == renderer);
    EXPECT_TRUE(channel_->GetRenderer(2, &renderer));
    EXPECT_TRUE(&renderer1 == renderer);

    EXPECT_TRUE(channel_->RemoveRecvStream(2));
    EXPECT_TRUE(channel_->RemoveRecvStream(1));
    EXPECT_FALSE(channel_->GetRenderer(1, &renderer));
    EXPECT_FALSE(channel_->GetRenderer(2, &renderer));
  }

  // Tests setting up and configuring multiple incoming streams in a
  // non-conference call.
  void AddRemoveRecvStreamsNoConference() {
    cricket::FakeVideoRenderer renderer1, renderer2;
    // Ensure we can't set the renderer on a non-existent stream.
    EXPECT_FALSE(channel_->SetRenderer(1, &renderer1));
    EXPECT_FALSE(channel_->SetRenderer(2, &renderer2));
    cricket::VideoRenderer* renderer;
    EXPECT_FALSE(channel_->GetRenderer(1, &renderer));
    EXPECT_FALSE(channel_->GetRenderer(2, &renderer));

    // Ensure we can add streams.
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(1)));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(2)));
    EXPECT_TRUE(channel_->GetRenderer(1, &renderer));
    // Verify the first AddRecvStream hook up to the default renderer.
    EXPECT_TRUE(renderer == NULL);
    EXPECT_TRUE(channel_->GetRenderer(2, &renderer));
    EXPECT_TRUE(NULL == renderer);

    // Ensure we can now set the renderers.
    EXPECT_TRUE(channel_->SetRenderer(1, &renderer1));
    EXPECT_TRUE(channel_->SetRenderer(2, &renderer2));
    EXPECT_TRUE(channel_->GetRenderer(1, &renderer));
    EXPECT_TRUE(&renderer1 == renderer);
    EXPECT_TRUE(channel_->GetRenderer(2, &renderer));
    EXPECT_TRUE(&renderer2 == renderer);

    // Ensure we can change the renderers if needed.
    EXPECT_TRUE(channel_->SetRenderer(1, &renderer2));
    EXPECT_TRUE(channel_->SetRenderer(2, &renderer1));
    EXPECT_TRUE(channel_->GetRenderer(1, &renderer));
    EXPECT_TRUE(&renderer2 == renderer);
    EXPECT_TRUE(channel_->GetRenderer(2, &renderer));
    EXPECT_TRUE(&renderer1 == renderer);

    EXPECT_TRUE(channel_->RemoveRecvStream(2));
    EXPECT_TRUE(channel_->RemoveRecvStream(1));
    EXPECT_FALSE(channel_->GetRenderer(1, &renderer));
    EXPECT_FALSE(channel_->GetRenderer(2, &renderer));
  }

  // Test that no frames are rendered after the receive stream have been
  // removed.
  void AddRemoveRecvStreamAndRender() {
    cricket::FakeVideoRenderer renderer1;
    EXPECT_TRUE(SetDefaultCodec());
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc)));
    EXPECT_TRUE(channel_->SetRenderer(kSsrc, &renderer1));

    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer1, 1, DefaultCodec().width, DefaultCodec().height, kTimeout);
    EXPECT_TRUE(channel_->RemoveRecvStream(kSsrc));
    // Send three more frames. This is to avoid that the test might be flaky
    // due to frame dropping.
    for (size_t i = 0; i < 3; ++i)
      EXPECT_TRUE(WaitAndSendFrame(100));

    // Test that no more frames have been rendered.
    EXPECT_EQ(1, renderer1.num_rendered_frames());

    // Re-add the stream again and make sure it renders.
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc)));
    // Force the next frame to be a key frame to make the receiving
    // decoder happy.
    EXPECT_TRUE(channel_->SendIntraFrame());

    EXPECT_TRUE(channel_->SetRenderer(kSsrc, &renderer1));
    EXPECT_TRUE(SendFrame());
    // Because the default channel is used, RemoveRecvStream above is not going
    // to delete the channel. As a result the engine will continue to receive
    // and decode the 3 frames sent above. So it is possible we will receive
    // some (e.g. 1) of these 3 frames after the renderer is set again.
    EXPECT_GT_FRAME_ON_RENDERER_WAIT(
        renderer1, 2, DefaultCodec().width, DefaultCodec().height, kTimeout);
    // Detach |renderer1| before exit as there might be frames come late.
    EXPECT_TRUE(channel_->SetRenderer(kSsrc, NULL));
  }

  // Tests the behavior of incoming streams in a conference scenario.
  void SimulateConference() {
    cricket::FakeVideoRenderer renderer1, renderer2;
    EXPECT_TRUE(SetDefaultCodec());
    cricket::VideoSendParameters parameters;
    parameters.codecs.push_back(DefaultCodec());
    parameters.options.conference_mode = rtc::Optional<bool>(true);
    EXPECT_TRUE(channel_->SetSendParameters(parameters));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(1)));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(2)));
    EXPECT_TRUE(channel_->SetRenderer(1, &renderer1));
    EXPECT_TRUE(channel_->SetRenderer(2, &renderer2));
    EXPECT_EQ(0, renderer1.num_rendered_frames());
    EXPECT_EQ(0, renderer2.num_rendered_frames());
    std::vector<uint32_t> ssrcs;
    ssrcs.push_back(1);
    ssrcs.push_back(2);
    network_interface_.SetConferenceMode(true, ssrcs);
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer1, 1, DefaultCodec().width, DefaultCodec().height, kTimeout);
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer2, 1, DefaultCodec().width, DefaultCodec().height, kTimeout);

    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    EXPECT_EQ(DefaultCodec().id, GetPayloadType(p.get()));
    EXPECT_EQ(DefaultCodec().width, renderer1.width());
    EXPECT_EQ(DefaultCodec().height, renderer1.height());
    EXPECT_EQ(DefaultCodec().width, renderer2.width());
    EXPECT_EQ(DefaultCodec().height, renderer2.height());
    EXPECT_TRUE(channel_->RemoveRecvStream(2));
    EXPECT_TRUE(channel_->RemoveRecvStream(1));
  }

  // Tests that we can add and remove capturers and frames are sent out properly
  void AddRemoveCapturer() {
    cricket::VideoCodec codec = DefaultCodec();
    codec.width = 320;
    codec.height = 240;
    const int time_between_send = TimeBetweenSend(codec);
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_WAIT(1, codec.width, codec.height, kTimeout);
    rtc::scoped_ptr<cricket::FakeVideoCapturer> capturer(
        CreateFakeVideoCapturer());
    capturer->SetScreencast(true);
    cricket::VideoFormat format(480, 360,
                                cricket::VideoFormat::FpsToInterval(30),
                                cricket::FOURCC_I420);
    EXPECT_EQ(cricket::CS_RUNNING, capturer->Start(format));
    // All capturers start generating frames with the same timestamp. ViE does
    // not allow the same timestamp to be used. Capture one frame before
    // associating the capturer with the channel.
    EXPECT_TRUE(capturer->CaptureCustomFrame(format.width, format.height,
                                             cricket::FOURCC_I420));

    int captured_frames = 1;
    for (int iterations = 0; iterations < 2; ++iterations) {
      EXPECT_TRUE(channel_->SetCapturer(kSsrc, capturer.get()));
      rtc::Thread::Current()->ProcessMessages(time_between_send);
      EXPECT_TRUE(capturer->CaptureCustomFrame(format.width, format.height,
                                               cricket::FOURCC_I420));
      ++captured_frames;
      // Wait until frame of right size is captured.
      EXPECT_TRUE_WAIT(renderer_.num_rendered_frames() >= captured_frames &&
                       format.width == renderer_.width() &&
                       format.height == renderer_.height() &&
                       !renderer_.black_frame(), kTimeout);
      EXPECT_GE(renderer_.num_rendered_frames(), captured_frames);
      EXPECT_EQ(format.width, renderer_.width());
      EXPECT_EQ(format.height, renderer_.height());
      captured_frames = renderer_.num_rendered_frames() + 1;
      EXPECT_FALSE(renderer_.black_frame());
      EXPECT_TRUE(channel_->SetCapturer(kSsrc, NULL));
      // Make sure a black frame is generated within the specified timeout.
      // The black frame should be the resolution of the previous frame to
      // prevent expensive encoder reconfigurations.
      EXPECT_TRUE_WAIT(renderer_.num_rendered_frames() >= captured_frames &&
                       format.width == renderer_.width() &&
                       format.height == renderer_.height() &&
                       renderer_.black_frame(), kTimeout);
      EXPECT_GE(renderer_.num_rendered_frames(), captured_frames);
      EXPECT_EQ(format.width, renderer_.width());
      EXPECT_EQ(format.height, renderer_.height());
      EXPECT_TRUE(renderer_.black_frame());

      // The black frame has the same timestamp as the next frame since it's
      // timestamp is set to the last frame's timestamp + interval. WebRTC will
      // not render a frame with the same timestamp so capture another frame
      // with the frame capturer to increment the next frame's timestamp.
      EXPECT_TRUE(capturer->CaptureCustomFrame(format.width, format.height,
                                               cricket::FOURCC_I420));
    }
  }

  // Tests that if RemoveCapturer is called without a capturer ever being
  // added, the plugin shouldn't crash (and no black frame should be sent).
  void RemoveCapturerWithoutAdd() {
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_WAIT(1, 640, 400, kTimeout);
    // Wait for one frame so they don't get dropped because we send frames too
    // tightly.
    rtc::Thread::Current()->ProcessMessages(30);
    // Remove the capturer.
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, NULL));
    // Wait for one black frame for removing the capturer.
    EXPECT_FRAME_WAIT(2, 640, 400, kTimeout);

    // No capturer was added, so this RemoveCapturer should
    // fail.
    EXPECT_FALSE(channel_->SetCapturer(kSsrc, NULL));
    rtc::Thread::Current()->ProcessMessages(300);
    // Verify no more frames were sent.
    EXPECT_EQ(2, renderer_.num_rendered_frames());
  }

  // Tests that we can add and remove capturer as unique sources.
  void AddRemoveCapturerMultipleSources() {
    // WebRTC implementation will drop frames if pushed to quickly. Wait the
    // interval time to avoid that.
    // WebRTC implementation will drop frames if pushed to quickly. Wait the
    // interval time to avoid that.
    // Set up the stream associated with the engine.
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc)));
    EXPECT_TRUE(channel_->SetRenderer(kSsrc, &renderer_));
    cricket::VideoFormat capture_format;  // default format
    capture_format.interval = cricket::VideoFormat::FpsToInterval(30);
    // Set up additional stream 1.
    cricket::FakeVideoRenderer renderer1;
    EXPECT_FALSE(channel_->SetRenderer(1, &renderer1));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(1)));
    EXPECT_TRUE(channel_->SetRenderer(1, &renderer1));
    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(1)));
    rtc::scoped_ptr<cricket::FakeVideoCapturer> capturer1(
        CreateFakeVideoCapturer());
    capturer1->SetScreencast(true);
    EXPECT_EQ(cricket::CS_RUNNING, capturer1->Start(capture_format));
    // Set up additional stream 2.
    cricket::FakeVideoRenderer renderer2;
    EXPECT_FALSE(channel_->SetRenderer(2, &renderer2));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(2)));
    EXPECT_TRUE(channel_->SetRenderer(2, &renderer2));
    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(2)));
    rtc::scoped_ptr<cricket::FakeVideoCapturer> capturer2(
        CreateFakeVideoCapturer());
    capturer2->SetScreencast(true);
    EXPECT_EQ(cricket::CS_RUNNING, capturer2->Start(capture_format));
    // State for all the streams.
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    // A limitation in the lmi implementation requires that SetCapturer() is
    // called after SetOneCodec().
    // TODO(hellner): this seems like an unnecessary constraint, fix it.
    EXPECT_TRUE(channel_->SetCapturer(1, capturer1.get()));
    EXPECT_TRUE(channel_->SetCapturer(2, capturer2.get()));
    EXPECT_TRUE(SetSend(true));
    // Test capturer associated with engine.
    const int kTestWidth = 160;
    const int kTestHeight = 120;
    EXPECT_TRUE(capturer1->CaptureCustomFrame(
        kTestWidth, kTestHeight, cricket::FOURCC_I420));
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer1, 1, kTestWidth, kTestHeight, kTimeout);
    // Capture a frame with additional capturer2, frames should be received
    EXPECT_TRUE(capturer2->CaptureCustomFrame(
        kTestWidth, kTestHeight, cricket::FOURCC_I420));
    EXPECT_FRAME_ON_RENDERER_WAIT(
        renderer2, 1, kTestWidth, kTestHeight, kTimeout);
    // Successfully remove the capturer.
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, NULL));
    // Fail to re-remove the capturer.
    EXPECT_FALSE(channel_->SetCapturer(kSsrc, NULL));
    // The capturers must be unregistered here as it runs out of it's scope
    // next.
    EXPECT_TRUE(channel_->SetCapturer(1, NULL));
    EXPECT_TRUE(channel_->SetCapturer(2, NULL));
  }

  void HighAspectHighHeightCapturer() {
    const int kWidth  = 80;
    const int kHeight = 10000;
    const int kScaledWidth = 20;
    const int kScaledHeight = 2500;

    cricket::VideoCodec codec(DefaultCodec());
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));

    cricket::FakeVideoRenderer renderer;
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc)));
    EXPECT_TRUE(channel_->SetRenderer(kSsrc, &renderer));
    EXPECT_EQ(0, renderer.num_rendered_frames());

    EXPECT_TRUE(SendFrame());
    EXPECT_GT_FRAME_ON_RENDERER_WAIT(
        renderer, 1, codec.width, codec.height, kTimeout);

    // Registering an external capturer is currently the same as screen casting
    // (update the test when this changes).
    rtc::scoped_ptr<cricket::FakeVideoCapturer> capturer(
        CreateFakeVideoCapturer());
    capturer->SetScreencast(true);
    const std::vector<cricket::VideoFormat>* formats =
        capturer->GetSupportedFormats();
    cricket::VideoFormat capture_format = (*formats)[0];
    EXPECT_EQ(cricket::CS_RUNNING, capturer->Start(capture_format));
    // Capture frame to not get same frame timestamps as previous capturer.
    capturer->CaptureFrame();
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, capturer.get()));
    EXPECT_TRUE(rtc::Thread::Current()->ProcessMessages(30));
    EXPECT_TRUE(capturer->CaptureCustomFrame(kWidth, kHeight,
                                             cricket::FOURCC_ARGB));
    EXPECT_GT_FRAME_ON_RENDERER_WAIT(
        renderer, 2, kScaledWidth, kScaledHeight, kTimeout);
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, NULL));
  }

  // Tests that we can adapt video resolution with 16:10 aspect ratio properly.
  void AdaptResolution16x10() {
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    cricket::VideoCodec codec(DefaultCodec());
    codec.width = 640;
    codec.height = 400;
    SendAndReceive(codec);
    codec.width /= 2;
    codec.height /= 2;
    // Adapt the resolution.
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(WaitAndSendFrame(30));
    EXPECT_FRAME_WAIT(2, codec.width, codec.height, kTimeout);
  }
  // Tests that we can adapt video resolution with 4:3 aspect ratio properly.
  void AdaptResolution4x3() {
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    cricket::VideoCodec codec(DefaultCodec());
    codec.width = 640;
    codec.height = 400;
    SendAndReceive(codec);
    codec.width /= 2;
    codec.height /= 2;
    // Adapt the resolution.
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(WaitAndSendFrame(30));
    EXPECT_FRAME_WAIT(2, codec.width, codec.height, kTimeout);
  }
  // Tests that we can drop all frames properly.
  void AdaptDropAllFrames() {
    // Set the channel codec's resolution to 0, which will require the adapter
    // to drop all frames.
    cricket::VideoCodec codec(DefaultCodec());
    codec.width = codec.height = codec.framerate = 0;
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    EXPECT_TRUE(SendFrame());
    EXPECT_TRUE(SendFrame());
    rtc::Thread::Current()->ProcessMessages(500);
    EXPECT_EQ(0, renderer_.num_rendered_frames());
  }
  // Tests that we can reduce the frame rate on demand properly.
  // TODO(fbarchard): This test is flakey on pulse.  Fix and re-enable
  void AdaptFramerate() {
    cricket::VideoCodec codec(DefaultCodec());
    int frame_count = 0;
    // The capturer runs at 30 fps. The channel requires 30 fps.
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_EQ(frame_count, renderer_.num_rendered_frames());
    EXPECT_TRUE(WaitAndSendFrame(0));  // Should be rendered.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be rendered.
    frame_count += 2;
    EXPECT_FRAME_WAIT(frame_count, codec.width, codec.height, kTimeout);
    rtc::scoped_ptr<const rtc::Buffer> p(GetRtpPacket(0));
    EXPECT_EQ(codec.id, GetPayloadType(p.get()));

    // The channel requires 15 fps.
    codec.framerate = 15;
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(WaitAndSendFrame(0));  // Should be rendered.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be dropped.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be rendered.
    frame_count += 2;
    EXPECT_EQ_WAIT(frame_count, renderer_.num_rendered_frames(), kTimeout);

    // The channel requires 10 fps.
    codec.framerate = 10;
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(WaitAndSendFrame(0));  // Should be rendered.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be dropped.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be dropped.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be rendered.
    frame_count += 2;
    EXPECT_EQ_WAIT(frame_count, renderer_.num_rendered_frames(), kTimeout);

    // The channel requires 8 fps. The adapter adapts to 10 fps, which is the
    // closest factor of 30.
    codec.framerate = 8;
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(WaitAndSendFrame(0));  // Should be rendered.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be dropped.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be dropped.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be rendered.
    frame_count += 2;
    EXPECT_EQ_WAIT(frame_count, renderer_.num_rendered_frames(), kTimeout);
  }
  // Tests that adapted frames won't be upscaled to a higher resolution.
  void SendsLowerResolutionOnSmallerFrames() {
    cricket::VideoCodec codec = DefaultCodec();
    codec.width = 320;
    codec.height = 240;
    EXPECT_TRUE(SetOneCodec(codec));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_WAIT(1, codec.width, codec.height, kTimeout);

    // Check that we send smaller frames at the new resolution.
    EXPECT_TRUE(rtc::Thread::Current()->ProcessMessages(33));
    EXPECT_TRUE(video_capturer_->CaptureCustomFrame(
        codec.width / 2, codec.height / 2, cricket::FOURCC_I420));
    EXPECT_FRAME_WAIT(2, codec.width / 2, codec.height / 2, kTimeout);
  }
  // Tests that we can set the send stream format properly.
  void SetSendStreamFormat() {
    cricket::VideoCodec codec(DefaultCodec());
    SendAndReceive(codec);
    int frame_count = 1;
    EXPECT_FRAME_WAIT(frame_count, codec.width, codec.height, kTimeout);

    // Adapt the resolution and frame rate to half.
    cricket::VideoFormat format(
        codec.width / 2,
        codec.height / 2,
        cricket::VideoFormat::FpsToInterval(codec.framerate / 2),
        cricket::FOURCC_I420);
    // The SSRC differs from the send SSRC.
    EXPECT_FALSE(channel_->SetSendStreamFormat(kSsrc - 1, format));
    EXPECT_TRUE(channel_->SetSendStreamFormat(kSsrc, format));

    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be dropped.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be rendered.
    EXPECT_TRUE(WaitAndSendFrame(30));  // Should be dropped.
    frame_count += 1;
    EXPECT_FRAME_WAIT(frame_count, format.width, format.height, kTimeout);

    // Adapt the resolution to 0x0, which should drop all frames.
    format.width = 0;
    format.height = 0;
    EXPECT_TRUE(channel_->SetSendStreamFormat(kSsrc, format));
    EXPECT_TRUE(SendFrame());
    EXPECT_TRUE(SendFrame());
    rtc::Thread::Current()->ProcessMessages(500);
    EXPECT_EQ(frame_count, renderer_.num_rendered_frames());
  }
  // Test that setting send stream format to 0x0 resolution will result in
  // frames being dropped.
  void SetSendStreamFormat0x0() {
    EXPECT_TRUE(SetOneCodec(DefaultCodec()));
    EXPECT_TRUE(SetSendStreamFormat(kSsrc, DefaultCodec()));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    // This frame should be received.
    EXPECT_TRUE(SendFrame());
    EXPECT_FRAME_WAIT(1, DefaultCodec().width, DefaultCodec().height, kTimeout);
    const int64_t interval =
        cricket::VideoFormat::FpsToInterval(DefaultCodec().framerate);
    cricket::VideoFormat format(
        0,
        0,
        interval,
        cricket::FOURCC_I420);
    EXPECT_TRUE(channel_->SetSendStreamFormat(kSsrc, format));
    // This frame should not be received.
    EXPECT_TRUE(WaitAndSendFrame(
        static_cast<int>(interval/rtc::kNumNanosecsPerMillisec)));
    rtc::Thread::Current()->ProcessMessages(500);
    EXPECT_EQ(1, renderer_.num_rendered_frames());
  }

  // Tests that we can mute and unmute the channel properly.
  void MuteStream() {
    EXPECT_TRUE(SetDefaultCodec());
    cricket::FakeVideoCapturer video_capturer;
    video_capturer.Start(
        cricket::VideoFormat(
            640, 480,
            cricket::VideoFormat::FpsToInterval(30),
            cricket::FOURCC_I420));
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, &video_capturer));
    EXPECT_TRUE(SetSend(true));
    EXPECT_TRUE(channel_->SetRenderer(kDefaultReceiveSsrc, &renderer_));
    EXPECT_EQ(0, renderer_.num_rendered_frames());
    // Mute the channel and expect black output frame.
    int frame_count = 0;
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, false, nullptr));
    EXPECT_TRUE(video_capturer.CaptureFrame());
    ++frame_count;
    EXPECT_EQ_WAIT(frame_count, renderer_.num_rendered_frames(), kTimeout);
    EXPECT_TRUE(renderer_.black_frame());
    // Unmute the channel and expect non-black output frame.
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, true, nullptr));
    EXPECT_TRUE(rtc::Thread::Current()->ProcessMessages(30));
    EXPECT_TRUE(video_capturer.CaptureFrame());
    ++frame_count;
    EXPECT_EQ_WAIT(frame_count, renderer_.num_rendered_frames(), kTimeout);
    EXPECT_FALSE(renderer_.black_frame());
    // Test that we can also Mute using the correct send stream SSRC.
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, false, nullptr));
    EXPECT_TRUE(rtc::Thread::Current()->ProcessMessages(30));
    EXPECT_TRUE(video_capturer.CaptureFrame());
    ++frame_count;
    EXPECT_EQ_WAIT(frame_count, renderer_.num_rendered_frames(), kTimeout);
    EXPECT_TRUE(renderer_.black_frame());
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, true, nullptr));
    EXPECT_TRUE(rtc::Thread::Current()->ProcessMessages(30));
    EXPECT_TRUE(video_capturer.CaptureFrame());
    ++frame_count;
    EXPECT_EQ_WAIT(frame_count, renderer_.num_rendered_frames(), kTimeout);
    EXPECT_FALSE(renderer_.black_frame());
    // Test that muting an existing stream succeeds even if it's muted.
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, false, nullptr));
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, false, nullptr));
    // Test that unmuting an existing stream succeeds even if it's not muted.
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, true, nullptr));
    EXPECT_TRUE(channel_->SetVideoSend(kSsrc, true, nullptr));
    // Test that muting an invalid stream fails.
    EXPECT_FALSE(channel_->SetVideoSend(kSsrc+1, false, nullptr));
    EXPECT_TRUE(channel_->SetCapturer(kSsrc, NULL));
  }

  // Test that multiple send streams can be created and deleted properly.
  void MultipleSendStreams() {
    // Remove stream added in Setup. I.e. remove stream corresponding to default
    // channel.
    EXPECT_TRUE(channel_->RemoveSendStream(kSsrc));
    const unsigned int kSsrcsSize = sizeof(kSsrcs4)/sizeof(kSsrcs4[0]);
    for (unsigned int i = 0; i < kSsrcsSize; ++i) {
      EXPECT_TRUE(channel_->AddSendStream(
          cricket::StreamParams::CreateLegacy(kSsrcs4[i])));
    }
    // Delete one of the non default channel streams, let the destructor delete
    // the remaining ones.
    EXPECT_TRUE(channel_->RemoveSendStream(kSsrcs4[kSsrcsSize - 1]));
    // Stream should already be deleted.
    EXPECT_FALSE(channel_->RemoveSendStream(kSsrcs4[kSsrcsSize - 1]));
  }

  // Two streams one channel tests.

  // Tests that we can send and receive frames.
  void TwoStreamsSendAndReceive(const cricket::VideoCodec& codec) {
    SetUpSecondStream();
    // Test sending and receiving on first stream.
    SendAndReceive(codec);
    // Test sending and receiving on second stream.
    EXPECT_EQ_WAIT(1, renderer2_.num_rendered_frames(), kTimeout);
    EXPECT_GT(NumRtpPackets(), 0);
    EXPECT_EQ(1, renderer2_.num_rendered_frames());
  }

  // Set up 2 streams where the first stream uses the default channel.
  // Then disconnect the first stream and verify default channel becomes
  // available.
  // Then add a new stream with |new_ssrc|. The new stream should re-use the
  // default channel.
  void TwoStreamsReUseFirstStream(const cricket::VideoCodec& codec) {
    SetUpSecondStream();
    // Default channel used by the first stream.
    EXPECT_EQ(kSsrc, channel_->GetDefaultSendChannelSsrc());
    EXPECT_TRUE(channel_->RemoveRecvStream(kSsrc));
    EXPECT_FALSE(channel_->RemoveRecvStream(kSsrc));
    EXPECT_TRUE(channel_->RemoveSendStream(kSsrc));
    EXPECT_FALSE(channel_->RemoveSendStream(kSsrc));
    // Default channel is no longer used by a stream.
    EXPECT_EQ(0u, channel_->GetDefaultSendChannelSsrc());
    uint32_t new_ssrc = kSsrc + 100;
    EXPECT_TRUE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(new_ssrc)));
    // Re-use default channel.
    EXPECT_EQ(new_ssrc, channel_->GetDefaultSendChannelSsrc());
    EXPECT_FALSE(channel_->AddSendStream(
        cricket::StreamParams::CreateLegacy(new_ssrc)));
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(new_ssrc)));
    EXPECT_TRUE(channel_->SetRenderer(new_ssrc, &renderer_));
    EXPECT_FALSE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(new_ssrc)));

    EXPECT_TRUE(channel_->SetCapturer(new_ssrc, video_capturer_.get()));

    SendAndReceive(codec);
    EXPECT_TRUE(channel_->RemoveSendStream(new_ssrc));
    EXPECT_EQ(0u, channel_->GetDefaultSendChannelSsrc());
  }

  // Tests that we can send and receive frames with early receive.
  void TwoStreamsSendAndUnsignalledRecv(const cricket::VideoCodec& codec) {
    cricket::VideoSendParameters parameters;
    parameters.options.conference_mode = rtc::Optional<bool>(true);
    parameters.options.unsignalled_recv_stream_limit = rtc::Optional<int>(1);
    EXPECT_TRUE(channel_->SetSendParameters(parameters));
    SetUpSecondStreamWithNoRecv();
    // Test sending and receiving on first stream.
    Send(codec);
    EXPECT_EQ_WAIT(2, NumRtpPackets(), kTimeout);
    EXPECT_EQ_WAIT(1, renderer_.num_rendered_frames(), kTimeout);
    // The first send is not expected to yield frames, because the ssrc
    // is not signalled yet. With unsignalled recv enabled, we will drop frames
    // instead of packets.
    EXPECT_EQ(0, renderer2_.num_rendered_frames());
    // Give a chance for the decoder to process before adding the receiver.
    rtc::Thread::Current()->ProcessMessages(100);
    // Test sending and receiving on second stream.
    EXPECT_TRUE(channel_->AddRecvStream(
        cricket::StreamParams::CreateLegacy(kSsrc + 2)));
    EXPECT_TRUE(channel_->SetRenderer(kSsrc + 2, &renderer2_));
    SendFrame();
    EXPECT_EQ_WAIT(2, renderer_.num_rendered_frames(), kTimeout);
    EXPECT_EQ(4, NumRtpPackets());
    // The second send is expected to yield frame as the ssrc is signalled now.
    // Decode should succeed here, though we received the key frame earlier.
    // Without early recv, we would have dropped it and decoding would have
    // failed.
    EXPECT_EQ_WAIT(1, renderer2_.num_rendered_frames(), kTimeout);
  }

  // Tests that we drop key frames when conference mode is enabled and we
  // receive rtp packets on unsignalled streams. Removal of a unsignalled recv
  // stream is successful.
  void TwoStreamsAddAndRemoveUnsignalledRecv(
      const cricket::VideoCodec& codec) {
    cricket::VideoOptions vmo;
    vmo.conference_mode = rtc::Optional<bool>(true);
    vmo.unsignalled_recv_stream_limit = rtc::Optional<int>(1);
    EXPECT_TRUE(channel_->SetOptions(vmo));
    SetUpSecondStreamWithNoRecv();
    // Sending and receiving on first stream.
    Send(codec);
    EXPECT_EQ_WAIT(2, NumRtpPackets(), kTimeout);
    EXPECT_EQ_WAIT(1, renderer_.num_rendered_frames(), kTimeout);
    // The first send is not expected to yield frames, because the ssrc
    // is no signalled yet. With unsignalled recv enabled, we will drop frames
    // instead of packets.
    EXPECT_EQ(0, renderer2_.num_rendered_frames());
    // Give a chance for the decoder to process before adding the receiver.
    rtc::Thread::Current()->ProcessMessages(100);
    // Ensure that we can remove the unsignalled recv stream that was created
    // when the first video packet with unsignalled recv ssrc is received.
    EXPECT_TRUE(channel_->RemoveRecvStream(kSsrc + 2));
  }

  const rtc::scoped_ptr<webrtc::Call> call_;
  VideoEngineOverride<E> engine_;
  rtc::scoped_ptr<cricket::FakeVideoCapturer> video_capturer_;
  rtc::scoped_ptr<cricket::FakeVideoCapturer> video_capturer_2_;
  rtc::scoped_ptr<C> channel_;
  cricket::FakeNetworkInterface network_interface_;
  cricket::FakeVideoRenderer renderer_;
  cricket::VideoMediaChannel::Error media_error_;

  // Used by test cases where 2 streams are run on the same channel.
  cricket::FakeVideoRenderer renderer2_;
};

#endif  // TALK_MEDIA_BASE_VIDEOENGINE_UNITTEST_H_  NOLINT
