/*
 * libjingle
 * Copyright 2004 Google Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_WEBRTC_VOICE

#include "talk/media/webrtc/webrtcvoiceengine.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "talk/media/base/audioframe.h"
#include "talk/media/base/audiorenderer.h"
#include "talk/media/base/constants.h"
#include "talk/media/base/streamparams.h"
#include "talk/media/webrtc/webrtcmediaengine.h"
#include "talk/media/webrtc/webrtcvoe.h"
#include "webrtc/audio/audio_sink.h"
#include "webrtc/base/arraysize.h"
#include "webrtc/base/base64.h"
#include "webrtc/base/byteorder.h"
#include "webrtc/base/common.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/stringencode.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/call/rtc_event_log.h"
#include "webrtc/common.h"
#include "webrtc/modules/audio_coding/acm2/rent_a_codec.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/system_wrappers/include/field_trial.h"
#include "webrtc/system_wrappers/include/trace.h"

namespace cricket {
namespace {

const int kDefaultTraceFilter = webrtc::kTraceNone | webrtc::kTraceTerseInfo |
                                webrtc::kTraceWarning | webrtc::kTraceError |
                                webrtc::kTraceCritical;
const int kElevatedTraceFilter = kDefaultTraceFilter | webrtc::kTraceStateInfo |
                                 webrtc::kTraceInfo;

// On Windows Vista and newer, Microsoft introduced the concept of "Default
// Communications Device". This means that there are two types of default
// devices (old Wave Audio style default and Default Communications Device).
//
// On Windows systems which only support Wave Audio style default, uses either
// -1 or 0 to select the default device.
#ifdef WIN32
const int kDefaultAudioDeviceId = -1;
#else
const int kDefaultAudioDeviceId = 0;
#endif

// Parameter used for NACK.
// This value is equivalent to 5 seconds of audio data at 20 ms per packet.
const int kNackMaxPackets = 250;

// Codec parameters for Opus.
// draft-spittka-payload-rtp-opus-03

// Recommended bitrates:
// 8-12 kb/s for NB speech,
// 16-20 kb/s for WB speech,
// 28-40 kb/s for FB speech,
// 48-64 kb/s for FB mono music, and
// 64-128 kb/s for FB stereo music.
// The current implementation applies the following values to mono signals,
// and multiplies them by 2 for stereo.
const int kOpusBitrateNb = 12000;
const int kOpusBitrateWb = 20000;
const int kOpusBitrateFb = 32000;

// Opus bitrate should be in the range between 6000 and 510000.
const int kOpusMinBitrate = 6000;
const int kOpusMaxBitrate = 510000;

// Default audio dscp value.
// See http://tools.ietf.org/html/rfc2474 for details.
// See also http://tools.ietf.org/html/draft-jennings-rtcweb-qos-00
const rtc::DiffServCodePoint kAudioDscpValue = rtc::DSCP_EF;

// Ensure we open the file in a writeable path on ChromeOS and Android. This
// workaround can be removed when it's possible to specify a filename for audio
// option based AEC dumps.
//
// TODO(grunell): Use a string in the options instead of hardcoding it here
// and let the embedder choose the filename (crbug.com/264223).
//
// NOTE(ajm): Don't use hardcoded paths on platforms not explicitly specified
// below.
#if defined(CHROMEOS)
const char kAecDumpByAudioOptionFilename[] = "/tmp/audio.aecdump";
#elif defined(ANDROID)
const char kAecDumpByAudioOptionFilename[] = "/sdcard/audio.aecdump";
#else
const char kAecDumpByAudioOptionFilename[] = "audio.aecdump";
#endif

// Constants from voice_engine_defines.h.
const int kMinTelephoneEventCode = 0;           // RFC4733 (Section 2.3.1)
const int kMaxTelephoneEventCode = 255;
const int kMinTelephoneEventDuration = 100;
const int kMaxTelephoneEventDuration = 60000;   // Actual limit is 2^16

bool ValidateStreamParams(const StreamParams& sp) {
  if (sp.ssrcs.empty()) {
    LOG(LS_ERROR) << "No SSRCs in stream parameters: " << sp.ToString();
    return false;
  }
  if (sp.ssrcs.size() > 1) {
    LOG(LS_ERROR) << "Multiple SSRCs in stream parameters: " << sp.ToString();
    return false;
  }
  return true;
}

// Dumps an AudioCodec in RFC 2327-ish format.
std::string ToString(const AudioCodec& codec) {
  std::stringstream ss;
  ss << codec.name << "/" << codec.clockrate << "/" << codec.channels
     << " (" << codec.id << ")";
  return ss.str();
}

std::string ToString(const webrtc::CodecInst& codec) {
  std::stringstream ss;
  ss << codec.plname << "/" << codec.plfreq << "/" << codec.channels
     << " (" << codec.pltype << ")";
  return ss.str();
}

bool IsCodec(const AudioCodec& codec, const char* ref_name) {
  return (_stricmp(codec.name.c_str(), ref_name) == 0);
}

bool IsCodec(const webrtc::CodecInst& codec, const char* ref_name) {
  return (_stricmp(codec.plname, ref_name) == 0);
}

bool FindCodec(const std::vector<AudioCodec>& codecs,
               const AudioCodec& codec,
               AudioCodec* found_codec) {
  for (const AudioCodec& c : codecs) {
    if (c.Matches(codec)) {
      if (found_codec != NULL) {
        *found_codec = c;
      }
      return true;
    }
  }
  return false;
}

bool VerifyUniquePayloadTypes(const std::vector<AudioCodec>& codecs) {
  if (codecs.empty()) {
    return true;
  }
  std::vector<int> payload_types;
  for (const AudioCodec& codec : codecs) {
    payload_types.push_back(codec.id);
  }
  std::sort(payload_types.begin(), payload_types.end());
  auto it = std::unique(payload_types.begin(), payload_types.end());
  return it == payload_types.end();
}

bool IsNackEnabled(const AudioCodec& codec) {
  return codec.HasFeedbackParam(FeedbackParam(kRtcpFbParamNack,
                                              kParamValueEmpty));
}

// Return true if codec.params[feature] == "1", false otherwise.
bool IsCodecFeatureEnabled(const AudioCodec& codec, const char* feature) {
  int value;
  return codec.GetParam(feature, &value) && value == 1;
}

// Use params[kCodecParamMaxAverageBitrate] if it is defined, use codec.bitrate
// otherwise. If the value (either from params or codec.bitrate) <=0, use the
// default configuration. If the value is beyond feasible bit rate of Opus,
// clamp it. Returns the Opus bit rate for operation.
int GetOpusBitrate(const AudioCodec& codec, int max_playback_rate) {
  int bitrate = 0;
  bool use_param = true;
  if (!codec.GetParam(kCodecParamMaxAverageBitrate, &bitrate)) {
    bitrate = codec.bitrate;
    use_param = false;
  }
  if (bitrate <= 0) {
    if (max_playback_rate <= 8000) {
      bitrate = kOpusBitrateNb;
    } else if (max_playback_rate <= 16000) {
      bitrate = kOpusBitrateWb;
    } else {
      bitrate = kOpusBitrateFb;
    }

    if (IsCodecFeatureEnabled(codec, kCodecParamStereo)) {
      bitrate *= 2;
    }
  } else if (bitrate < kOpusMinBitrate || bitrate > kOpusMaxBitrate) {
    bitrate = (bitrate < kOpusMinBitrate) ? kOpusMinBitrate : kOpusMaxBitrate;
    std::string rate_source =
        use_param ? "Codec parameter \"maxaveragebitrate\"" :
            "Supplied Opus bitrate";
    LOG(LS_WARNING) << rate_source
                    << " is invalid and is replaced by: "
                    << bitrate;
  }
  return bitrate;
}

// Returns kOpusDefaultPlaybackRate if params[kCodecParamMaxPlaybackRate] is not
// defined. Returns the value of params[kCodecParamMaxPlaybackRate] otherwise.
int GetOpusMaxPlaybackRate(const AudioCodec& codec) {
  int value;
  if (codec.GetParam(kCodecParamMaxPlaybackRate, &value)) {
    return value;
  }
  return kOpusDefaultMaxPlaybackRate;
}

void GetOpusConfig(const AudioCodec& codec, webrtc::CodecInst* voe_codec,
                          bool* enable_codec_fec, int* max_playback_rate,
                          bool* enable_codec_dtx) {
  *enable_codec_fec = IsCodecFeatureEnabled(codec, kCodecParamUseInbandFec);
  *enable_codec_dtx = IsCodecFeatureEnabled(codec, kCodecParamUseDtx);
  *max_playback_rate = GetOpusMaxPlaybackRate(codec);

  // If OPUS, change what we send according to the "stereo" codec
  // parameter, and not the "channels" parameter.  We set
  // voe_codec.channels to 2 if "stereo=1" and 1 otherwise.  If
  // the bitrate is not specified, i.e. is <= zero, we set it to the
  // appropriate default value for mono or stereo Opus.

  voe_codec->channels = IsCodecFeatureEnabled(codec, kCodecParamStereo) ? 2 : 1;
  voe_codec->rate = GetOpusBitrate(codec, *max_playback_rate);
}

webrtc::AudioState::Config MakeAudioStateConfig(VoEWrapper* voe_wrapper) {
  webrtc::AudioState::Config config;
  config.voice_engine = voe_wrapper->engine();
  return config;
}

class WebRtcVoiceCodecs final {
 public:
  // TODO(solenberg): Do this filtering once off-line, add a simple AudioCodec
  // list and add a test which verifies VoE supports the listed codecs.
  static std::vector<AudioCodec> SupportedCodecs() {
    LOG(LS_INFO) << "WebRtc VoiceEngine codecs:";
    std::vector<AudioCodec> result;
    for (webrtc::CodecInst voe_codec : webrtc::acm2::RentACodec::Database()) {
      // Change the sample rate of G722 to 8000 to match SDP.
      MaybeFixupG722(&voe_codec, 8000);
      // Skip uncompressed formats.
      if (IsCodec(voe_codec, kL16CodecName)) {
        continue;
      }

      const CodecPref* pref = NULL;
      for (size_t j = 0; j < arraysize(kCodecPrefs); ++j) {
        if (IsCodec(voe_codec, kCodecPrefs[j].name) &&
            kCodecPrefs[j].clockrate == voe_codec.plfreq &&
            kCodecPrefs[j].channels == voe_codec.channels) {
          pref = &kCodecPrefs[j];
          break;
        }
      }

      if (pref) {
        // Use the payload type that we've configured in our pref table;
        // use the offset in our pref table to determine the sort order.
        AudioCodec codec(
            pref->payload_type, voe_codec.plname, voe_codec.plfreq,
            voe_codec.rate, voe_codec.channels,
            static_cast<int>(arraysize(kCodecPrefs)) - (pref - kCodecPrefs));
        LOG(LS_INFO) << ToString(codec);
        if (IsCodec(codec, kIsacCodecName)) {
          // Indicate auto-bitrate in signaling.
          codec.bitrate = 0;
        }
        if (IsCodec(codec, kOpusCodecName)) {
          // Only add fmtp parameters that differ from the spec.
          if (kPreferredMinPTime != kOpusDefaultMinPTime) {
            codec.params[kCodecParamMinPTime] =
                rtc::ToString(kPreferredMinPTime);
          }
          if (kPreferredMaxPTime != kOpusDefaultMaxPTime) {
            codec.params[kCodecParamMaxPTime] =
                rtc::ToString(kPreferredMaxPTime);
          }
          codec.SetParam(kCodecParamUseInbandFec, 1);

          // TODO(hellner): Add ptime, sprop-stereo, and stereo
          // when they can be set to values other than the default.
        }
        result.push_back(codec);
      } else {
        LOG(LS_WARNING) << "Unexpected codec: " << ToString(voe_codec);
      }
    }
    // Make sure they are in local preference order.
    std::sort(result.begin(), result.end(), &AudioCodec::Preferable);
    return result;
  }

  static bool ToCodecInst(const AudioCodec& in,
                          webrtc::CodecInst* out) {
    for (webrtc::CodecInst voe_codec : webrtc::acm2::RentACodec::Database()) {
      // Change the sample rate of G722 to 8000 to match SDP.
      MaybeFixupG722(&voe_codec, 8000);
      AudioCodec codec(voe_codec.pltype, voe_codec.plname, voe_codec.plfreq,
                       voe_codec.rate, voe_codec.channels, 0);
      bool multi_rate = IsCodecMultiRate(voe_codec);
      // Allow arbitrary rates for ISAC to be specified.
      if (multi_rate) {
        // Set codec.bitrate to 0 so the check for codec.Matches() passes.
        codec.bitrate = 0;
      }
      if (codec.Matches(in)) {
        if (out) {
          // Fixup the payload type.
          voe_codec.pltype = in.id;

          // Set bitrate if specified.
          if (multi_rate && in.bitrate != 0) {
            voe_codec.rate = in.bitrate;
          }

          // Reset G722 sample rate to 16000 to match WebRTC.
          MaybeFixupG722(&voe_codec, 16000);

          // Apply codec-specific settings.
          if (IsCodec(codec, kIsacCodecName)) {
            // If ISAC and an explicit bitrate is not specified,
            // enable auto bitrate adjustment.
            voe_codec.rate = (in.bitrate > 0) ? in.bitrate : -1;
          }
          *out = voe_codec;
        }
        return true;
      }
    }
    return false;
  }

  static bool IsCodecMultiRate(const webrtc::CodecInst& codec) {
    for (size_t i = 0; i < arraysize(kCodecPrefs); ++i) {
      if (IsCodec(codec, kCodecPrefs[i].name) &&
          kCodecPrefs[i].clockrate == codec.plfreq) {
        return kCodecPrefs[i].is_multi_rate;
      }
    }
    return false;
  }

  // If the AudioCodec param kCodecParamPTime is set, then we will set it to
  // codec pacsize if it's valid, or we will pick the next smallest value we
  // support.
  // TODO(Brave): Query supported packet sizes from ACM when the API is ready.
  static bool SetPTimeAsPacketSize(webrtc::CodecInst* codec, int ptime_ms) {
    for (const CodecPref& codec_pref : kCodecPrefs) {
      if ((IsCodec(*codec, codec_pref.name) &&
          codec_pref.clockrate == codec->plfreq) ||
          IsCodec(*codec, kG722CodecName)) {
        int packet_size_ms = SelectPacketSize(codec_pref, ptime_ms);
        if (packet_size_ms) {
          // Convert unit from milli-seconds to samples.
          codec->pacsize = (codec->plfreq / 1000) * packet_size_ms;
          return true;
        }
      }
    }
    return false;
  }

 private:
  static const int kMaxNumPacketSize = 6;
  struct CodecPref {
    const char* name;
    int clockrate;
    size_t channels;
    int payload_type;
    bool is_multi_rate;
    int packet_sizes_ms[kMaxNumPacketSize];
  };
  // Note: keep the supported packet sizes in ascending order.
  static const CodecPref kCodecPrefs[12];

  static int SelectPacketSize(const CodecPref& codec_pref, int ptime_ms) {
    int selected_packet_size_ms = codec_pref.packet_sizes_ms[0];
    for (int packet_size_ms : codec_pref.packet_sizes_ms) {
      if (packet_size_ms && packet_size_ms <= ptime_ms) {
        selected_packet_size_ms = packet_size_ms;
      }
    }
    return selected_packet_size_ms;
  }

  // Changes RTP timestamp rate of G722. This is due to the "bug" in the RFC
  // which says that G722 should be advertised as 8 kHz although it is a 16 kHz
  // codec.
  static void MaybeFixupG722(webrtc::CodecInst* voe_codec, int new_plfreq) {
    if (IsCodec(*voe_codec, kG722CodecName)) {
      // If the ASSERT triggers, the codec definition in WebRTC VoiceEngine
      // has changed, and this special case is no longer needed.
      RTC_DCHECK(voe_codec->plfreq != new_plfreq);
      voe_codec->plfreq = new_plfreq;
    }
  }
};

const WebRtcVoiceCodecs::CodecPref WebRtcVoiceCodecs::kCodecPrefs[12] = {
  { kOpusCodecName,   48000, 2, 111, true,  { 10, 20, 40, 60 } },
  { kIsacCodecName,   16000, 1, 103, true,  { 30, 60 } },
  { kIsacCodecName,   32000, 1, 104, true,  { 30 } },
  // G722 should be advertised as 8000 Hz because of the RFC "bug".
  { kG722CodecName,   8000,  1, 9,   false, { 10, 20, 30, 40, 50, 60 } },
  { kIlbcCodecName,   8000,  1, 102, false, { 20, 30, 40, 60 } },
  { kPcmuCodecName,   8000,  1, 0,   false, { 10, 20, 30, 40, 50, 60 } },
  { kPcmaCodecName,   8000,  1, 8,   false, { 10, 20, 30, 40, 50, 60 } },
  { kCnCodecName,     32000, 1, 106, false, { } },
  { kCnCodecName,     16000, 1, 105, false, { } },
  { kCnCodecName,     8000,  1, 13,  false, { } },
  { kRedCodecName,    8000,  1, 127, false, { } },
  { kDtmfCodecName,   8000,  1, 126, false, { } },
};
} // namespace {

bool WebRtcVoiceEngine::ToCodecInst(const AudioCodec& in,
                                    webrtc::CodecInst* out) {
  return WebRtcVoiceCodecs::ToCodecInst(in, out);
}

WebRtcVoiceEngine::WebRtcVoiceEngine()
    : voe_wrapper_(new VoEWrapper()),
      audio_state_(webrtc::AudioState::Create(MakeAudioStateConfig(voe()))) {
  Construct();
}

WebRtcVoiceEngine::WebRtcVoiceEngine(VoEWrapper* voe_wrapper)
    : voe_wrapper_(voe_wrapper) {
  Construct();
}

void WebRtcVoiceEngine::Construct() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_VERBOSE) << "WebRtcVoiceEngine::WebRtcVoiceEngine";

  signal_thread_checker_.DetachFromThread();
  std::memset(&default_agc_config_, 0, sizeof(default_agc_config_));
  voe_config_.Set<webrtc::VoicePacing>(new webrtc::VoicePacing(true));

  webrtc::Trace::set_level_filter(kDefaultTraceFilter);
  webrtc::Trace::SetTraceCallback(this);

  // Load our audio codec list.
  codecs_ = WebRtcVoiceCodecs::SupportedCodecs();
}

WebRtcVoiceEngine::~WebRtcVoiceEngine() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_VERBOSE) << "WebRtcVoiceEngine::~WebRtcVoiceEngine";
  if (adm_) {
    voe_wrapper_.reset();
    adm_->Release();
    adm_ = NULL;
  }
  webrtc::Trace::SetTraceCallback(nullptr);
}

bool WebRtcVoiceEngine::Init(rtc::Thread* worker_thread) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  RTC_DCHECK(worker_thread == rtc::Thread::Current());
  LOG(LS_INFO) << "WebRtcVoiceEngine::Init";
  bool res = InitInternal();
  if (res) {
    LOG(LS_INFO) << "WebRtcVoiceEngine::Init Done!";
  } else {
    LOG(LS_ERROR) << "WebRtcVoiceEngine::Init failed";
    Terminate();
  }
  return res;
}

bool WebRtcVoiceEngine::InitInternal() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  // Temporarily turn logging level up for the Init call
  webrtc::Trace::set_level_filter(kElevatedTraceFilter);
  LOG(LS_INFO) << webrtc::VoiceEngine::GetVersionString();
  if (voe_wrapper_->base()->Init(adm_) == -1) {
    LOG_RTCERR0_EX(Init, voe_wrapper_->error());
    return false;
  }
  webrtc::Trace::set_level_filter(kDefaultTraceFilter);

  // Save the default AGC configuration settings. This must happen before
  // calling ApplyOptions or the default will be overwritten.
  if (voe_wrapper_->processing()->GetAgcConfig(default_agc_config_) == -1) {
    LOG_RTCERR0(GetAgcConfig);
    return false;
  }

  // Print our codec list again for the call diagnostic log
  LOG(LS_INFO) << "WebRtc VoiceEngine codecs:";
  for (const AudioCodec& codec : codecs_) {
    LOG(LS_INFO) << ToString(codec);
  }

  SetDefaultDevices();

  initialized_ = true;
  return true;
}

void WebRtcVoiceEngine::Terminate() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "WebRtcVoiceEngine::Terminate";
  initialized_ = false;

  StopAecDump();

  voe_wrapper_->base()->Terminate();
}

rtc::scoped_refptr<webrtc::AudioState>
    WebRtcVoiceEngine::GetAudioState() const {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  return audio_state_;
}

VoiceMediaChannel* WebRtcVoiceEngine::CreateChannel(webrtc::Call* call,
    const AudioOptions& options) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  return new WebRtcVoiceMediaChannel(this, options, call);
}

bool WebRtcVoiceEngine::ApplyOptions(const AudioOptions& options_in) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "ApplyOptions: " << options_in.ToString();

  // Default engine options.
  AudioOptions options;
  options.echo_cancellation = rtc::Optional<bool>(true);
  options.auto_gain_control = rtc::Optional<bool>(true);
  options.noise_suppression = rtc::Optional<bool>(true);
  options.highpass_filter = rtc::Optional<bool>(true);
  options.stereo_swapping = rtc::Optional<bool>(false);
  options.audio_jitter_buffer_max_packets = rtc::Optional<int>(50);
  options.audio_jitter_buffer_fast_accelerate = rtc::Optional<bool>(false);
  options.typing_detection = rtc::Optional<bool>(true);
  options.adjust_agc_delta = rtc::Optional<int>(0);
  options.experimental_agc = rtc::Optional<bool>(false);
  options.extended_filter_aec = rtc::Optional<bool>(false);
  options.delay_agnostic_aec = rtc::Optional<bool>(false);
  options.experimental_ns = rtc::Optional<bool>(false);
  options.aec_dump = rtc::Optional<bool>(false);

  // Apply any given options on top.
  options.SetAll(options_in);

  // kEcConference is AEC with high suppression.
  webrtc::EcModes ec_mode = webrtc::kEcConference;
  webrtc::AecmModes aecm_mode = webrtc::kAecmSpeakerphone;
  webrtc::AgcModes agc_mode = webrtc::kAgcAdaptiveAnalog;
  webrtc::NsModes ns_mode = webrtc::kNsHighSuppression;
  if (options.aecm_generate_comfort_noise) {
    LOG(LS_VERBOSE) << "Comfort noise explicitly set to "
                    << *options.aecm_generate_comfort_noise
                    << " (default is false).";
  }

#if defined(WEBRTC_IOS)
  // On iOS, VPIO provides built-in EC and AGC.
  options.echo_cancellation = rtc::Optional<bool>(false);
  options.auto_gain_control = rtc::Optional<bool>(false);
  LOG(LS_INFO) << "Always disable AEC and AGC on iOS. Use built-in instead.";
#elif defined(ANDROID)
  ec_mode = webrtc::kEcAecm;
#endif

#if defined(WEBRTC_IOS) || defined(ANDROID)
  // Set the AGC mode for iOS as well despite disabling it above, to avoid
  // unsupported configuration errors from webrtc.
  agc_mode = webrtc::kAgcFixedDigital;
  options.typing_detection = rtc::Optional<bool>(false);
  options.experimental_agc = rtc::Optional<bool>(false);
  options.extended_filter_aec = rtc::Optional<bool>(false);
  options.experimental_ns = rtc::Optional<bool>(false);
#endif

  // Delay Agnostic AEC automatically turns on EC if not set except on iOS
  // where the feature is not supported.
  bool use_delay_agnostic_aec = false;
#if !defined(WEBRTC_IOS)
  if (options.delay_agnostic_aec) {
    use_delay_agnostic_aec = *options.delay_agnostic_aec;
    if (use_delay_agnostic_aec) {
      options.echo_cancellation = rtc::Optional<bool>(true);
      options.extended_filter_aec = rtc::Optional<bool>(true);
      ec_mode = webrtc::kEcConference;
    }
  }
#endif

  webrtc::VoEAudioProcessing* voep = voe_wrapper_->processing();

  if (options.echo_cancellation) {
    // Check if platform supports built-in EC. Currently only supported on
    // Android and in combination with Java based audio layer.
    // TODO(henrika): investigate possibility to support built-in EC also
    // in combination with Open SL ES audio.
    const bool built_in_aec = voe_wrapper_->hw()->BuiltInAECIsAvailable();
    if (built_in_aec) {
      // Built-in EC exists on this device and use_delay_agnostic_aec is not
      // overriding it. Enable/Disable it according to the echo_cancellation
      // audio option.
      const bool enable_built_in_aec =
          *options.echo_cancellation && !use_delay_agnostic_aec;
      if (voe_wrapper_->hw()->EnableBuiltInAEC(enable_built_in_aec) == 0 &&
          enable_built_in_aec) {
        // Disable internal software EC if built-in EC is enabled,
        // i.e., replace the software EC with the built-in EC.
        options.echo_cancellation = rtc::Optional<bool>(false);
        LOG(LS_INFO) << "Disabling EC since built-in EC will be used instead";
      }
    }
    if (voep->SetEcStatus(*options.echo_cancellation, ec_mode) == -1) {
      LOG_RTCERR2(SetEcStatus, *options.echo_cancellation, ec_mode);
      return false;
    } else {
      LOG(LS_INFO) << "Echo control set to " << *options.echo_cancellation
                   << " with mode " << ec_mode;
    }
#if !defined(ANDROID)
    // TODO(ajm): Remove the error return on Android from webrtc.
    if (voep->SetEcMetricsStatus(*options.echo_cancellation) == -1) {
      LOG_RTCERR1(SetEcMetricsStatus, *options.echo_cancellation);
      return false;
    }
#endif
    if (ec_mode == webrtc::kEcAecm) {
      bool cn = options.aecm_generate_comfort_noise.value_or(false);
      if (voep->SetAecmMode(aecm_mode, cn) != 0) {
        LOG_RTCERR2(SetAecmMode, aecm_mode, cn);
        return false;
      }
    }
  }

  if (options.auto_gain_control) {
    const bool built_in_agc = voe_wrapper_->hw()->BuiltInAGCIsAvailable();
    if (built_in_agc) {
      if (voe_wrapper_->hw()->EnableBuiltInAGC(*options.auto_gain_control) ==
              0 &&
          *options.auto_gain_control) {
        // Disable internal software AGC if built-in AGC is enabled,
        // i.e., replace the software AGC with the built-in AGC.
        options.auto_gain_control = rtc::Optional<bool>(false);
        LOG(LS_INFO) << "Disabling AGC since built-in AGC will be used instead";
      }
    }
    if (voep->SetAgcStatus(*options.auto_gain_control, agc_mode) == -1) {
      LOG_RTCERR2(SetAgcStatus, *options.auto_gain_control, agc_mode);
      return false;
    } else {
      LOG(LS_INFO) << "Auto gain set to " << *options.auto_gain_control
                   << " with mode " << agc_mode;
    }
  }

  if (options.tx_agc_target_dbov || options.tx_agc_digital_compression_gain ||
      options.tx_agc_limiter) {
    // Override default_agc_config_. Generally, an unset option means "leave
    // the VoE bits alone" in this function, so we want whatever is set to be
    // stored as the new "default". If we didn't, then setting e.g.
    // tx_agc_target_dbov would reset digital compression gain and limiter
    // settings.
    // Also, if we don't update default_agc_config_, then adjust_agc_delta
    // would be an offset from the original values, and not whatever was set
    // explicitly.
    default_agc_config_.targetLeveldBOv = options.tx_agc_target_dbov.value_or(
        default_agc_config_.targetLeveldBOv);
    default_agc_config_.digitalCompressionGaindB =
        options.tx_agc_digital_compression_gain.value_or(
            default_agc_config_.digitalCompressionGaindB);
    default_agc_config_.limiterEnable =
        options.tx_agc_limiter.value_or(default_agc_config_.limiterEnable);
    if (voe_wrapper_->processing()->SetAgcConfig(default_agc_config_) == -1) {
      LOG_RTCERR3(SetAgcConfig,
                  default_agc_config_.targetLeveldBOv,
                  default_agc_config_.digitalCompressionGaindB,
                  default_agc_config_.limiterEnable);
      return false;
    }
  }

  if (options.noise_suppression) {
    const bool built_in_ns = voe_wrapper_->hw()->BuiltInNSIsAvailable();
    if (built_in_ns) {
      if (voe_wrapper_->hw()->EnableBuiltInNS(*options.noise_suppression) ==
              0 &&
          *options.noise_suppression) {
        // Disable internal software NS if built-in NS is enabled,
        // i.e., replace the software NS with the built-in NS.
        options.noise_suppression = rtc::Optional<bool>(false);
        LOG(LS_INFO) << "Disabling NS since built-in NS will be used instead";
      }
    }
    if (voep->SetNsStatus(*options.noise_suppression, ns_mode) == -1) {
      LOG_RTCERR2(SetNsStatus, *options.noise_suppression, ns_mode);
      return false;
    } else {
      LOG(LS_INFO) << "Noise suppression set to " << *options.noise_suppression
                   << " with mode " << ns_mode;
    }
  }

  if (options.highpass_filter) {
    LOG(LS_INFO) << "High pass filter enabled? " << *options.highpass_filter;
    if (voep->EnableHighPassFilter(*options.highpass_filter) == -1) {
      LOG_RTCERR1(SetHighpassFilterStatus, *options.highpass_filter);
      return false;
    }
  }

  if (options.stereo_swapping) {
    LOG(LS_INFO) << "Stereo swapping enabled? " << *options.stereo_swapping;
    voep->EnableStereoChannelSwapping(*options.stereo_swapping);
    if (voep->IsStereoChannelSwappingEnabled() != *options.stereo_swapping) {
      LOG_RTCERR1(EnableStereoChannelSwapping, *options.stereo_swapping);
      return false;
    }
  }

  if (options.audio_jitter_buffer_max_packets) {
    LOG(LS_INFO) << "NetEq capacity is "
                 << *options.audio_jitter_buffer_max_packets;
    voe_config_.Set<webrtc::NetEqCapacityConfig>(
        new webrtc::NetEqCapacityConfig(
            *options.audio_jitter_buffer_max_packets));
  }

  if (options.audio_jitter_buffer_fast_accelerate) {
    LOG(LS_INFO) << "NetEq fast mode? "
                 << *options.audio_jitter_buffer_fast_accelerate;
    voe_config_.Set<webrtc::NetEqFastAccelerate>(
        new webrtc::NetEqFastAccelerate(
            *options.audio_jitter_buffer_fast_accelerate));
  }

  if (options.typing_detection) {
    LOG(LS_INFO) << "Typing detection is enabled? "
                 << *options.typing_detection;
    if (voep->SetTypingDetectionStatus(*options.typing_detection) == -1) {
      // In case of error, log the info and continue
      LOG_RTCERR1(SetTypingDetectionStatus, *options.typing_detection);
    }
  }

  if (options.adjust_agc_delta) {
    LOG(LS_INFO) << "Adjust agc delta is " << *options.adjust_agc_delta;
    if (!AdjustAgcLevel(*options.adjust_agc_delta)) {
      return false;
    }
  }

  if (options.aec_dump) {
    LOG(LS_INFO) << "Aec dump is enabled? " << *options.aec_dump;
    if (*options.aec_dump)
      StartAecDump(kAecDumpByAudioOptionFilename);
    else
      StopAecDump();
  }

  webrtc::Config config;

  if (options.delay_agnostic_aec)
    delay_agnostic_aec_ = options.delay_agnostic_aec;
  if (delay_agnostic_aec_) {
    LOG(LS_INFO) << "Delay agnostic aec is enabled? " << *delay_agnostic_aec_;
    config.Set<webrtc::DelayAgnostic>(
        new webrtc::DelayAgnostic(*delay_agnostic_aec_));
  }

  if (options.extended_filter_aec) {
    extended_filter_aec_ = options.extended_filter_aec;
  }
  if (extended_filter_aec_) {
    LOG(LS_INFO) << "Extended filter aec is enabled? " << *extended_filter_aec_;
    config.Set<webrtc::ExtendedFilter>(
        new webrtc::ExtendedFilter(*extended_filter_aec_));
  }

  if (options.experimental_ns) {
    experimental_ns_ = options.experimental_ns;
  }
  if (experimental_ns_) {
    LOG(LS_INFO) << "Experimental ns is enabled? " << *experimental_ns_;
    config.Set<webrtc::ExperimentalNs>(
        new webrtc::ExperimentalNs(*experimental_ns_));
  }

  // We check audioproc for the benefit of tests, since FakeWebRtcVoiceEngine
  // returns NULL on audio_processing().
  webrtc::AudioProcessing* audioproc = voe_wrapper_->base()->audio_processing();
  if (audioproc) {
    audioproc->SetExtraOptions(config);
  }

  if (options.recording_sample_rate) {
    LOG(LS_INFO) << "Recording sample rate is "
                 << *options.recording_sample_rate;
    if (voe_wrapper_->hw()->SetRecordingSampleRate(
            *options.recording_sample_rate)) {
      LOG_RTCERR1(SetRecordingSampleRate, *options.recording_sample_rate);
    }
  }

  if (options.playout_sample_rate) {
    LOG(LS_INFO) << "Playout sample rate is " << *options.playout_sample_rate;
    if (voe_wrapper_->hw()->SetPlayoutSampleRate(
            *options.playout_sample_rate)) {
      LOG_RTCERR1(SetPlayoutSampleRate, *options.playout_sample_rate);
    }
  }

  return true;
}

void WebRtcVoiceEngine::SetDefaultDevices() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
#if !defined(WEBRTC_IOS)
  int in_id = kDefaultAudioDeviceId;
  int out_id = kDefaultAudioDeviceId;
  LOG(LS_INFO) << "Setting microphone to (id=" << in_id
               << ") and speaker to (id=" << out_id << ")";

  bool ret = true;
  if (voe_wrapper_->hw()->SetRecordingDevice(in_id) == -1) {
    LOG_RTCERR1(SetRecordingDevice, in_id);
    ret = false;
  }
  webrtc::AudioProcessing* ap = voe()->base()->audio_processing();
  if (ap) {
    ap->Initialize();
  }

  if (voe_wrapper_->hw()->SetPlayoutDevice(out_id) == -1) {
    LOG_RTCERR1(SetPlayoutDevice, out_id);
    ret = false;
  }

  if (ret) {
    LOG(LS_INFO) << "Set microphone to (id=" << in_id
                 << ") and speaker to (id=" << out_id << ")";
  }
#endif  // !WEBRTC_IOS
}

bool WebRtcVoiceEngine::GetOutputVolume(int* level) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  unsigned int ulevel;
  if (voe_wrapper_->volume()->GetSpeakerVolume(ulevel) == -1) {
    LOG_RTCERR1(GetSpeakerVolume, level);
    return false;
  }
  *level = ulevel;
  return true;
}

bool WebRtcVoiceEngine::SetOutputVolume(int level) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  RTC_DCHECK(level >= 0 && level <= 255);
  if (voe_wrapper_->volume()->SetSpeakerVolume(level) == -1) {
    LOG_RTCERR1(SetSpeakerVolume, level);
    return false;
  }
  return true;
}

int WebRtcVoiceEngine::GetInputLevel() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  unsigned int ulevel;
  return (voe_wrapper_->volume()->GetSpeechInputLevel(ulevel) != -1) ?
      static_cast<int>(ulevel) : -1;
}

const std::vector<AudioCodec>& WebRtcVoiceEngine::codecs() {
  RTC_DCHECK(signal_thread_checker_.CalledOnValidThread());
  return codecs_;
}

RtpCapabilities WebRtcVoiceEngine::GetCapabilities() const {
  RTC_DCHECK(signal_thread_checker_.CalledOnValidThread());
  RtpCapabilities capabilities;
  capabilities.header_extensions.push_back(RtpHeaderExtension(
      kRtpAudioLevelHeaderExtension, kRtpAudioLevelHeaderExtensionDefaultId));
  capabilities.header_extensions.push_back(
      RtpHeaderExtension(kRtpAbsoluteSenderTimeHeaderExtension,
                         kRtpAbsoluteSenderTimeHeaderExtensionDefaultId));
  return capabilities;
}

int WebRtcVoiceEngine::GetLastEngineError() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  return voe_wrapper_->error();
}

void WebRtcVoiceEngine::Print(webrtc::TraceLevel level, const char* trace,
                              int length) {
  // Note: This callback can happen on any thread!
  rtc::LoggingSeverity sev = rtc::LS_VERBOSE;
  if (level == webrtc::kTraceError || level == webrtc::kTraceCritical)
    sev = rtc::LS_ERROR;
  else if (level == webrtc::kTraceWarning)
    sev = rtc::LS_WARNING;
  else if (level == webrtc::kTraceStateInfo || level == webrtc::kTraceInfo)
    sev = rtc::LS_INFO;
  else if (level == webrtc::kTraceTerseInfo)
    sev = rtc::LS_INFO;

  // Skip past boilerplate prefix text
  if (length < 72) {
    std::string msg(trace, length);
    LOG(LS_ERROR) << "Malformed webrtc log message: ";
    LOG_V(sev) << msg;
  } else {
    std::string msg(trace + 71, length - 72);
    LOG_V(sev) << "webrtc: " << msg;
  }
}

void WebRtcVoiceEngine::RegisterChannel(WebRtcVoiceMediaChannel* channel) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  RTC_DCHECK(channel);
  channels_.push_back(channel);
}

void WebRtcVoiceEngine::UnregisterChannel(WebRtcVoiceMediaChannel* channel) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  auto it = std::find(channels_.begin(), channels_.end(), channel);
  RTC_DCHECK(it != channels_.end());
  channels_.erase(it);
}

// Adjusts the default AGC target level by the specified delta.
// NB: If we start messing with other config fields, we'll want
// to save the current webrtc::AgcConfig as well.
bool WebRtcVoiceEngine::AdjustAgcLevel(int delta) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  webrtc::AgcConfig config = default_agc_config_;
  config.targetLeveldBOv -= delta;

  LOG(LS_INFO) << "Adjusting AGC level from default -"
               << default_agc_config_.targetLeveldBOv << "dB to -"
               << config.targetLeveldBOv << "dB";

  if (voe_wrapper_->processing()->SetAgcConfig(config) == -1) {
    LOG_RTCERR1(SetAgcConfig, config.targetLeveldBOv);
    return false;
  }
  return true;
}

bool WebRtcVoiceEngine::SetAudioDeviceModule(webrtc::AudioDeviceModule* adm) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  if (initialized_) {
    LOG(LS_WARNING) << "SetAudioDeviceModule can not be called after Init.";
    return false;
  }
  if (adm_) {
    adm_->Release();
    adm_ = NULL;
  }
  if (adm) {
    adm_ = adm;
    adm_->AddRef();
  }
  return true;
}

bool WebRtcVoiceEngine::StartAecDump(rtc::PlatformFile file) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  FILE* aec_dump_file_stream = rtc::FdopenPlatformFileForWriting(file);
  if (!aec_dump_file_stream) {
    LOG(LS_ERROR) << "Could not open AEC dump file stream.";
    if (!rtc::ClosePlatformFile(file))
      LOG(LS_WARNING) << "Could not close file.";
    return false;
  }
  StopAecDump();
  if (voe_wrapper_->processing()->StartDebugRecording(aec_dump_file_stream) !=
      webrtc::AudioProcessing::kNoError) {
    LOG_RTCERR0(StartDebugRecording);
    fclose(aec_dump_file_stream);
    return false;
  }
  is_dumping_aec_ = true;
  return true;
}

void WebRtcVoiceEngine::StartAecDump(const std::string& filename) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  if (!is_dumping_aec_) {
    // Start dumping AEC when we are not dumping.
    if (voe_wrapper_->processing()->StartDebugRecording(
        filename.c_str()) != webrtc::AudioProcessing::kNoError) {
      LOG_RTCERR1(StartDebugRecording, filename.c_str());
    } else {
      is_dumping_aec_ = true;
    }
  }
}

void WebRtcVoiceEngine::StopAecDump() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  if (is_dumping_aec_) {
    // Stop dumping AEC when we are dumping.
    if (voe_wrapper_->processing()->StopDebugRecording() !=
        webrtc::AudioProcessing::kNoError) {
      LOG_RTCERR0(StopDebugRecording);
    }
    is_dumping_aec_ = false;
  }
}

bool WebRtcVoiceEngine::StartRtcEventLog(rtc::PlatformFile file) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  return voe_wrapper_->codec()->GetEventLog()->StartLogging(file);
}

void WebRtcVoiceEngine::StopRtcEventLog() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  voe_wrapper_->codec()->GetEventLog()->StopLogging();
}

int WebRtcVoiceEngine::CreateVoEChannel() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  return voe_wrapper_->base()->CreateChannel(voe_config_);
}

class WebRtcVoiceMediaChannel::WebRtcAudioSendStream
    : public AudioRenderer::Sink {
 public:
  WebRtcAudioSendStream(int ch, webrtc::AudioTransport* voe_audio_transport,
                        uint32_t ssrc, const std::string& c_name,
                        const std::vector<webrtc::RtpExtension>& extensions,
                        webrtc::Call* call)
      : voe_audio_transport_(voe_audio_transport),
        call_(call),
        config_(nullptr) {
    RTC_DCHECK_GE(ch, 0);
    // TODO(solenberg): Once we're not using FakeWebRtcVoiceEngine anymore:
    // RTC_DCHECK(voe_audio_transport);
    RTC_DCHECK(call);
    audio_capture_thread_checker_.DetachFromThread();
    config_.rtp.ssrc = ssrc;
    config_.rtp.c_name = c_name;
    config_.voe_channel_id = ch;
    RecreateAudioSendStream(extensions);
  }

  ~WebRtcAudioSendStream() override {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    Stop();
    call_->DestroyAudioSendStream(stream_);
  }

  void RecreateAudioSendStream(
      const std::vector<webrtc::RtpExtension>& extensions) {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    if (stream_) {
      call_->DestroyAudioSendStream(stream_);
      stream_ = nullptr;
    }
    config_.rtp.extensions = extensions;
    RTC_DCHECK(!stream_);
    stream_ = call_->CreateAudioSendStream(config_);
    RTC_CHECK(stream_);
  }

  bool SendTelephoneEvent(int payload_type, uint8_t event,
                          uint32_t duration_ms) {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    RTC_DCHECK(stream_);
    return stream_->SendTelephoneEvent(payload_type, event, duration_ms);
  }

  webrtc::AudioSendStream::Stats GetStats() const {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    RTC_DCHECK(stream_);
    return stream_->GetStats();
  }

  // Starts the rendering by setting a sink to the renderer to get data
  // callback.
  // This method is called on the libjingle worker thread.
  // TODO(xians): Make sure Start() is called only once.
  void Start(AudioRenderer* renderer) {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    RTC_DCHECK(renderer);
    if (renderer_) {
      RTC_DCHECK(renderer_ == renderer);
      return;
    }
    renderer->SetSink(this);
    renderer_ = renderer;
  }

  // Stops rendering by setting the sink of the renderer to nullptr. No data
  // callback will be received after this method.
  // This method is called on the libjingle worker thread.
  void Stop() {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    if (renderer_) {
      renderer_->SetSink(nullptr);
      renderer_ = nullptr;
    }
  }

  // AudioRenderer::Sink implementation.
  // This method is called on the audio thread.
  void OnData(const void* audio_data,
              int bits_per_sample,
              int sample_rate,
              size_t number_of_channels,
              size_t number_of_frames) override {
    RTC_DCHECK(!worker_thread_checker_.CalledOnValidThread());
    RTC_DCHECK(audio_capture_thread_checker_.CalledOnValidThread());
    RTC_DCHECK(voe_audio_transport_);
    voe_audio_transport_->OnData(config_.voe_channel_id,
                                 audio_data,
                                 bits_per_sample,
                                 sample_rate,
                                 number_of_channels,
                                 number_of_frames);
  }

  // Callback from the |renderer_| when it is going away. In case Start() has
  // never been called, this callback won't be triggered.
  void OnClose() override {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    // Set |renderer_| to nullptr to make sure no more callback will get into
    // the renderer.
    renderer_ = nullptr;
  }

  // Accessor to the VoE channel ID.
  int channel() const {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    return config_.voe_channel_id;
  }

 private:
  rtc::ThreadChecker worker_thread_checker_;
  rtc::ThreadChecker audio_capture_thread_checker_;
  webrtc::AudioTransport* const voe_audio_transport_ = nullptr;
  webrtc::Call* call_ = nullptr;
  webrtc::AudioSendStream::Config config_;
  // The stream is owned by WebRtcAudioSendStream and may be reallocated if
  // configuration changes.
  webrtc::AudioSendStream* stream_ = nullptr;

  // Raw pointer to AudioRenderer owned by LocalAudioTrackHandler.
  // PeerConnection will make sure invalidating the pointer before the object
  // goes away.
  AudioRenderer* renderer_ = nullptr;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(WebRtcAudioSendStream);
};

class WebRtcVoiceMediaChannel::WebRtcAudioReceiveStream {
 public:
  WebRtcAudioReceiveStream(int ch, uint32_t remote_ssrc, uint32_t local_ssrc,
                           bool use_combined_bwe, const std::string& sync_group,
                           const std::vector<webrtc::RtpExtension>& extensions,
                           webrtc::Call* call)
      : call_(call),
        config_() {
    RTC_DCHECK_GE(ch, 0);
    RTC_DCHECK(call);
    config_.rtp.remote_ssrc = remote_ssrc;
    config_.rtp.local_ssrc = local_ssrc;
    config_.voe_channel_id = ch;
    config_.sync_group = sync_group;
    RecreateAudioReceiveStream(use_combined_bwe, extensions);
  }

  ~WebRtcAudioReceiveStream() {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    call_->DestroyAudioReceiveStream(stream_);
  }

  void RecreateAudioReceiveStream(
      const std::vector<webrtc::RtpExtension>& extensions) {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    RecreateAudioReceiveStream(config_.combined_audio_video_bwe, extensions);
  }
  void RecreateAudioReceiveStream(bool use_combined_bwe) {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    RecreateAudioReceiveStream(use_combined_bwe, config_.rtp.extensions);
  }

  webrtc::AudioReceiveStream::Stats GetStats() const {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    RTC_DCHECK(stream_);
    return stream_->GetStats();
  }

  int channel() const {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    return config_.voe_channel_id;
  }

  void SetRawAudioSink(rtc::scoped_ptr<webrtc::AudioSinkInterface> sink) {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    stream_->SetSink(std::move(sink));
  }

 private:
  void RecreateAudioReceiveStream(bool use_combined_bwe,
      const std::vector<webrtc::RtpExtension>& extensions) {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    if (stream_) {
      call_->DestroyAudioReceiveStream(stream_);
      stream_ = nullptr;
    }
    config_.rtp.extensions = extensions;
    config_.combined_audio_video_bwe = use_combined_bwe;
    RTC_DCHECK(!stream_);
    stream_ = call_->CreateAudioReceiveStream(config_);
    RTC_CHECK(stream_);
  }

  rtc::ThreadChecker worker_thread_checker_;
  webrtc::Call* call_ = nullptr;
  webrtc::AudioReceiveStream::Config config_;
  // The stream is owned by WebRtcAudioReceiveStream and may be reallocated if
  // configuration changes.
  webrtc::AudioReceiveStream* stream_ = nullptr;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(WebRtcAudioReceiveStream);
};

WebRtcVoiceMediaChannel::WebRtcVoiceMediaChannel(WebRtcVoiceEngine* engine,
                                                 const AudioOptions& options,
                                                 webrtc::Call* call)
    : engine_(engine), call_(call) {
  LOG(LS_VERBOSE) << "WebRtcVoiceMediaChannel::WebRtcVoiceMediaChannel";
  RTC_DCHECK(call);
  engine->RegisterChannel(this);
  SetOptions(options);
}

WebRtcVoiceMediaChannel::~WebRtcVoiceMediaChannel() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_VERBOSE) << "WebRtcVoiceMediaChannel::~WebRtcVoiceMediaChannel";
  // TODO(solenberg): Should be able to delete the streams directly, without
  //                  going through RemoveNnStream(), once stream objects handle
  //                  all (de)configuration.
  while (!send_streams_.empty()) {
    RemoveSendStream(send_streams_.begin()->first);
  }
  while (!recv_streams_.empty()) {
    RemoveRecvStream(recv_streams_.begin()->first);
  }
  engine()->UnregisterChannel(this);
}

bool WebRtcVoiceMediaChannel::SetSendParameters(
    const AudioSendParameters& params) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "WebRtcVoiceMediaChannel::SetSendParameters: "
               << params.ToString();
  // TODO(pthatcher): Refactor this to be more clean now that we have
  // all the information at once.

  if (!SetSendCodecs(params.codecs)) {
    return false;
  }

  if (!ValidateRtpExtensions(params.extensions)) {
    return false;
  }
  std::vector<webrtc::RtpExtension> filtered_extensions =
      FilterRtpExtensions(params.extensions,
                          webrtc::RtpExtension::IsSupportedForAudio, true);
  if (send_rtp_extensions_ != filtered_extensions) {
    send_rtp_extensions_.swap(filtered_extensions);
    for (auto& it : send_streams_) {
      it.second->RecreateAudioSendStream(send_rtp_extensions_);
    }
  }

  if (!SetMaxSendBandwidth(params.max_bandwidth_bps)) {
    return false;
  }
  return SetOptions(params.options);
}

bool WebRtcVoiceMediaChannel::SetRecvParameters(
    const AudioRecvParameters& params) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "WebRtcVoiceMediaChannel::SetRecvParameters: "
               << params.ToString();
  // TODO(pthatcher): Refactor this to be more clean now that we have
  // all the information at once.

  if (!SetRecvCodecs(params.codecs)) {
    return false;
  }

  if (!ValidateRtpExtensions(params.extensions)) {
    return false;
  }
  std::vector<webrtc::RtpExtension> filtered_extensions =
      FilterRtpExtensions(params.extensions,
                          webrtc::RtpExtension::IsSupportedForAudio, false);
  if (recv_rtp_extensions_ != filtered_extensions) {
    recv_rtp_extensions_.swap(filtered_extensions);
    for (auto& it : recv_streams_) {
      it.second->RecreateAudioReceiveStream(recv_rtp_extensions_);
    }
  }

  return true;
}

bool WebRtcVoiceMediaChannel::SetOptions(const AudioOptions& options) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "Setting voice channel options: "
               << options.ToString();

  // Check if DSCP value is changed from previous.
  bool dscp_option_changed = (options_.dscp != options.dscp);

  // We retain all of the existing options, and apply the given ones
  // on top.  This means there is no way to "clear" options such that
  // they go back to the engine default.
  options_.SetAll(options);
  if (!engine()->ApplyOptions(options_)) {
    LOG(LS_WARNING) <<
        "Failed to apply engine options during channel SetOptions.";
    return false;
  }

  if (dscp_option_changed) {
    rtc::DiffServCodePoint dscp = rtc::DSCP_DEFAULT;
    if (options_.dscp.value_or(false)) {
      dscp = kAudioDscpValue;
    }
    if (MediaChannel::SetDscp(dscp) != 0) {
      LOG(LS_WARNING) << "Failed to set DSCP settings for audio channel";
    }
  }

  // TODO(solenberg): Don't recreate unless options changed.
  for (auto& it : recv_streams_) {
    it.second->RecreateAudioReceiveStream(
        options_.combined_audio_video_bwe.value_or(false));
  }

  LOG(LS_INFO) << "Set voice channel options.  Current options: "
               << options_.ToString();
  return true;
}

bool WebRtcVoiceMediaChannel::SetRecvCodecs(
    const std::vector<AudioCodec>& codecs) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());

  // Set the payload types to be used for incoming media.
  LOG(LS_INFO) << "Setting receive voice codecs.";

  if (!VerifyUniquePayloadTypes(codecs)) {
    LOG(LS_ERROR) << "Codec payload types overlap.";
    return false;
  }

  std::vector<AudioCodec> new_codecs;
  // Find all new codecs. We allow adding new codecs but don't allow changing
  // the payload type of codecs that is already configured since we might
  // already be receiving packets with that payload type.
  for (const AudioCodec& codec : codecs) {
    AudioCodec old_codec;
    if (FindCodec(recv_codecs_, codec, &old_codec)) {
      if (old_codec.id != codec.id) {
        LOG(LS_ERROR) << codec.name << " payload type changed.";
        return false;
      }
    } else {
      new_codecs.push_back(codec);
    }
  }
  if (new_codecs.empty()) {
    // There are no new codecs to configure. Already configured codecs are
    // never removed.
    return true;
  }

  if (playout_) {
    // Receive codecs can not be changed while playing. So we temporarily
    // pause playout.
    PausePlayout();
  }

  bool result = true;
  for (const AudioCodec& codec : new_codecs) {
    webrtc::CodecInst voe_codec;
    if (WebRtcVoiceEngine::ToCodecInst(codec, &voe_codec)) {
      LOG(LS_INFO) << ToString(codec);
      voe_codec.pltype = codec.id;
      for (const auto& ch : recv_streams_) {
        if (engine()->voe()->codec()->SetRecPayloadType(
                ch.second->channel(), voe_codec) == -1) {
          LOG_RTCERR2(SetRecPayloadType, ch.second->channel(),
                      ToString(voe_codec));
          result = false;
        }
      }
    } else {
      LOG(LS_WARNING) << "Unknown codec " << ToString(codec);
      result = false;
      break;
    }
  }
  if (result) {
    recv_codecs_ = codecs;
  }

  if (desired_playout_ && !playout_) {
    ResumePlayout();
  }
  return result;
}

bool WebRtcVoiceMediaChannel::SetSendCodecs(
    int channel, const std::vector<AudioCodec>& codecs) {
  // Disable VAD, FEC, and RED unless we know the other side wants them.
  engine()->voe()->codec()->SetVADStatus(channel, false);
  engine()->voe()->rtp()->SetNACKStatus(channel, false, 0);
  engine()->voe()->rtp()->SetREDStatus(channel, false);
  engine()->voe()->codec()->SetFECStatus(channel, false);

  // Scan through the list to figure out the codec to use for sending, along
  // with the proper configuration for VAD.
  bool found_send_codec = false;
  webrtc::CodecInst send_codec;
  memset(&send_codec, 0, sizeof(send_codec));

  bool nack_enabled = nack_enabled_;
  bool enable_codec_fec = false;
  bool enable_opus_dtx = false;
  int opus_max_playback_rate = 0;

  // Set send codec (the first non-telephone-event/CN codec)
  for (const AudioCodec& codec : codecs) {
    // Ignore codecs we don't know about. The negotiation step should prevent
    // this, but double-check to be sure.
    webrtc::CodecInst voe_codec;
    if (!WebRtcVoiceEngine::ToCodecInst(codec, &voe_codec)) {
      LOG(LS_WARNING) << "Unknown codec " << ToString(codec);
      continue;
    }

    if (IsCodec(codec, kDtmfCodecName) || IsCodec(codec, kCnCodecName)) {
      // Skip telephone-event/CN codec, which will be handled later.
      continue;
    }

    // We'll use the first codec in the list to actually send audio data.
    // Be sure to use the payload type requested by the remote side.
    // "red", for RED audio, is a special case where the actual codec to be
    // used is specified in params.
    if (IsCodec(codec, kRedCodecName)) {
      // Parse out the RED parameters. If we fail, just ignore RED;
      // we don't support all possible params/usage scenarios.
      if (!GetRedSendCodec(codec, codecs, &send_codec)) {
        continue;
      }

      // Enable redundant encoding of the specified codec. Treat any
      // failure as a fatal internal error.
      LOG(LS_INFO) << "Enabling RED on channel " << channel;
      if (engine()->voe()->rtp()->SetREDStatus(channel, true, codec.id) == -1) {
        LOG_RTCERR3(SetREDStatus, channel, true, codec.id);
        return false;
      }
    } else {
      send_codec = voe_codec;
      nack_enabled = IsNackEnabled(codec);
      // For Opus as the send codec, we are to determine inband FEC, maximum
      // playback rate, and opus internal dtx.
      if (IsCodec(codec, kOpusCodecName)) {
        GetOpusConfig(codec, &send_codec, &enable_codec_fec,
                      &opus_max_playback_rate, &enable_opus_dtx);
      }

      // Set packet size if the AudioCodec param kCodecParamPTime is set.
      int ptime_ms = 0;
      if (codec.GetParam(kCodecParamPTime, &ptime_ms)) {
        if (!WebRtcVoiceCodecs::SetPTimeAsPacketSize(&send_codec, ptime_ms)) {
          LOG(LS_WARNING) << "Failed to set packet size for codec "
                          << send_codec.plname;
          return false;
        }
      }
    }
    found_send_codec = true;
    break;
  }

  if (nack_enabled_ != nack_enabled) {
    SetNack(channel, nack_enabled);
    nack_enabled_ = nack_enabled;
  }

  if (!found_send_codec) {
    LOG(LS_WARNING) << "Received empty list of codecs.";
    return false;
  }

  // Set the codec immediately, since SetVADStatus() depends on whether
  // the current codec is mono or stereo.
  if (!SetSendCodec(channel, send_codec))
    return false;

  // FEC should be enabled after SetSendCodec.
  if (enable_codec_fec) {
    LOG(LS_INFO) << "Attempt to enable codec internal FEC on channel "
                 << channel;
    if (engine()->voe()->codec()->SetFECStatus(channel, true) == -1) {
      // Enable codec internal FEC. Treat any failure as fatal internal error.
      LOG_RTCERR2(SetFECStatus, channel, true);
      return false;
    }
  }

  if (IsCodec(send_codec, kOpusCodecName)) {
    // DTX and maxplaybackrate should be set after SetSendCodec. Because current
    // send codec has to be Opus.

    // Set Opus internal DTX.
    LOG(LS_INFO) << "Attempt to "
                 << (enable_opus_dtx ? "enable" : "disable")
                 << " Opus DTX on channel "
                 << channel;
    if (engine()->voe()->codec()->SetOpusDtx(channel, enable_opus_dtx)) {
      LOG_RTCERR2(SetOpusDtx, channel, enable_opus_dtx);
      return false;
    }

    // If opus_max_playback_rate <= 0, the default maximum playback rate
    // (48 kHz) will be used.
    if (opus_max_playback_rate > 0) {
      LOG(LS_INFO) << "Attempt to set maximum playback rate to "
                   << opus_max_playback_rate
                   << " Hz on channel "
                   << channel;
      if (engine()->voe()->codec()->SetOpusMaxPlaybackRate(
          channel, opus_max_playback_rate) == -1) {
        LOG_RTCERR2(SetOpusMaxPlaybackRate, channel, opus_max_playback_rate);
        return false;
      }
    }
  }

  // Always update the |send_codec_| to the currently set send codec.
  send_codec_.reset(new webrtc::CodecInst(send_codec));

  if (send_bitrate_setting_) {
    SetSendBitrateInternal(send_bitrate_bps_);
  }

  // Loop through the codecs list again to config the CN codec.
  for (const AudioCodec& codec : codecs) {
    // Ignore codecs we don't know about. The negotiation step should prevent
    // this, but double-check to be sure.
    webrtc::CodecInst voe_codec;
    if (!WebRtcVoiceEngine::ToCodecInst(codec, &voe_codec)) {
      LOG(LS_WARNING) << "Unknown codec " << ToString(codec);
      continue;
    }

    if (IsCodec(codec, kCnCodecName)) {
      // Turn voice activity detection/comfort noise on if supported.
      // Set the wideband CN payload type appropriately.
      // (narrowband always uses the static payload type 13).
      webrtc::PayloadFrequencies cn_freq;
      switch (codec.clockrate) {
        case 8000:
          cn_freq = webrtc::kFreq8000Hz;
          break;
        case 16000:
          cn_freq = webrtc::kFreq16000Hz;
          break;
        case 32000:
          cn_freq = webrtc::kFreq32000Hz;
          break;
        default:
          LOG(LS_WARNING) << "CN frequency " << codec.clockrate
                          << " not supported.";
          continue;
      }
      // Set the CN payloadtype and the VAD status.
      // The CN payload type for 8000 Hz clockrate is fixed at 13.
      if (cn_freq != webrtc::kFreq8000Hz) {
        if (engine()->voe()->codec()->SetSendCNPayloadType(
                channel, codec.id, cn_freq) == -1) {
          LOG_RTCERR3(SetSendCNPayloadType, channel, codec.id, cn_freq);
          // TODO(ajm): This failure condition will be removed from VoE.
          // Restore the return here when we update to a new enough webrtc.
          //
          // Not returning false because the SetSendCNPayloadType will fail if
          // the channel is already sending.
          // This can happen if the remote description is applied twice, for
          // example in the case of ROAP on top of JSEP, where both side will
          // send the offer.
        }
      }
      // Only turn on VAD if we have a CN payload type that matches the
      // clockrate for the codec we are going to use.
      if (codec.clockrate == send_codec.plfreq && send_codec.channels != 2) {
        // TODO(minyue): If CN frequency == 48000 Hz is allowed, consider the
        // interaction between VAD and Opus FEC.
        LOG(LS_INFO) << "Enabling VAD";
        if (engine()->voe()->codec()->SetVADStatus(channel, true) == -1) {
          LOG_RTCERR2(SetVADStatus, channel, true);
          return false;
        }
      }
    }
  }
  return true;
}

bool WebRtcVoiceMediaChannel::SetSendCodecs(
    const std::vector<AudioCodec>& codecs) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  // TODO(solenberg): Validate input - that payload types don't overlap, are
  //                  within range, filter out codecs we don't support,
  //                  redundant codecs etc.

  // Find the DTMF telephone event "codec" payload type.
  dtmf_payload_type_ = rtc::Optional<int>();
  for (const AudioCodec& codec : codecs) {
    if (IsCodec(codec, kDtmfCodecName)) {
      dtmf_payload_type_ = rtc::Optional<int>(codec.id);
      break;
    }
  }

  // Cache the codecs in order to configure the channel created later.
  send_codecs_ = codecs;
  for (const auto& ch : send_streams_) {
    if (!SetSendCodecs(ch.second->channel(), codecs)) {
      return false;
    }
  }

  // Set nack status on receive channels and update |nack_enabled_|.
  for (const auto& ch : recv_streams_) {
    SetNack(ch.second->channel(), nack_enabled_);
  }

  return true;
}

void WebRtcVoiceMediaChannel::SetNack(int channel, bool nack_enabled) {
  if (nack_enabled) {
    LOG(LS_INFO) << "Enabling NACK for channel " << channel;
    engine()->voe()->rtp()->SetNACKStatus(channel, true, kNackMaxPackets);
  } else {
    LOG(LS_INFO) << "Disabling NACK for channel " << channel;
    engine()->voe()->rtp()->SetNACKStatus(channel, false, 0);
  }
}

bool WebRtcVoiceMediaChannel::SetSendCodec(
    int channel, const webrtc::CodecInst& send_codec) {
  LOG(LS_INFO) << "Send channel " << channel <<  " selected voice codec "
               << ToString(send_codec) << ", bitrate=" << send_codec.rate;

  webrtc::CodecInst current_codec;
  if (engine()->voe()->codec()->GetSendCodec(channel, current_codec) == 0 &&
      (send_codec == current_codec)) {
    // Codec is already configured, we can return without setting it again.
    return true;
  }

  if (engine()->voe()->codec()->SetSendCodec(channel, send_codec) == -1) {
    LOG_RTCERR2(SetSendCodec, channel, ToString(send_codec));
    return false;
  }
  return true;
}

bool WebRtcVoiceMediaChannel::SetPlayout(bool playout) {
  desired_playout_ = playout;
  return ChangePlayout(desired_playout_);
}

bool WebRtcVoiceMediaChannel::PausePlayout() {
  return ChangePlayout(false);
}

bool WebRtcVoiceMediaChannel::ResumePlayout() {
  return ChangePlayout(desired_playout_);
}

bool WebRtcVoiceMediaChannel::ChangePlayout(bool playout) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  if (playout_ == playout) {
    return true;
  }

  for (const auto& ch : recv_streams_) {
    if (!SetPlayout(ch.second->channel(), playout)) {
      LOG(LS_ERROR) << "SetPlayout " << playout << " on channel "
                    << ch.second->channel() << " failed";
      return false;
    }
  }
  playout_ = playout;
  return true;
}

bool WebRtcVoiceMediaChannel::SetSend(SendFlags send) {
  desired_send_ = send;
  if (!send_streams_.empty()) {
    return ChangeSend(desired_send_);
  }
  return true;
}

bool WebRtcVoiceMediaChannel::PauseSend() {
  return ChangeSend(SEND_NOTHING);
}

bool WebRtcVoiceMediaChannel::ResumeSend() {
  return ChangeSend(desired_send_);
}

bool WebRtcVoiceMediaChannel::ChangeSend(SendFlags send) {
  if (send_ == send) {
    return true;
  }

  // Apply channel specific options when channel is enabled for sending.
  if (send == SEND_MICROPHONE) {
    engine()->ApplyOptions(options_);
  }

  // Change the settings on each send channel.
  for (const auto& ch : send_streams_) {
    if (!ChangeSend(ch.second->channel(), send)) {
      return false;
    }
  }

  send_ = send;
  return true;
}

bool WebRtcVoiceMediaChannel::ChangeSend(int channel, SendFlags send) {
  if (send == SEND_MICROPHONE) {
    if (engine()->voe()->base()->StartSend(channel) == -1) {
      LOG_RTCERR1(StartSend, channel);
      return false;
    }
  } else {  // SEND_NOTHING
    RTC_DCHECK(send == SEND_NOTHING);
    if (engine()->voe()->base()->StopSend(channel) == -1) {
      LOG_RTCERR1(StopSend, channel);
      return false;
    }
  }

  return true;
}

bool WebRtcVoiceMediaChannel::SetAudioSend(uint32_t ssrc,
                                           bool enable,
                                           const AudioOptions* options,
                                           AudioRenderer* renderer) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  // TODO(solenberg): The state change should be fully rolled back if any one of
  //                  these calls fail.
  if (!SetLocalRenderer(ssrc, renderer)) {
    return false;
  }
  if (!MuteStream(ssrc, !enable)) {
    return false;
  }
  if (enable && options) {
    return SetOptions(*options);
  }
  return true;
}

int WebRtcVoiceMediaChannel::CreateVoEChannel() {
  int id = engine()->CreateVoEChannel();
  if (id == -1) {
    LOG_RTCERR0(CreateVoEChannel);
    return -1;
  }
  if (engine()->voe()->network()->RegisterExternalTransport(id, *this) == -1) {
    LOG_RTCERR2(RegisterExternalTransport, id, this);
    engine()->voe()->base()->DeleteChannel(id);
    return -1;
  }
  return id;
}

bool WebRtcVoiceMediaChannel::DeleteVoEChannel(int channel) {
  if (engine()->voe()->network()->DeRegisterExternalTransport(channel) == -1) {
    LOG_RTCERR1(DeRegisterExternalTransport, channel);
  }
  if (engine()->voe()->base()->DeleteChannel(channel) == -1) {
    LOG_RTCERR1(DeleteChannel, channel);
    return false;
  }
  return true;
}

bool WebRtcVoiceMediaChannel::AddSendStream(const StreamParams& sp) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "AddSendStream: " << sp.ToString();

  uint32_t ssrc = sp.first_ssrc();
  RTC_DCHECK(0 != ssrc);

  if (GetSendChannelId(ssrc) != -1) {
    LOG(LS_ERROR) << "Stream already exists with ssrc " << ssrc;
    return false;
  }

  // Create a new channel for sending audio data.
  int channel = CreateVoEChannel();
  if (channel == -1) {
    return false;
  }

  // Save the channel to send_streams_, so that RemoveSendStream() can still
  // delete the channel in case failure happens below.
  webrtc::AudioTransport* audio_transport =
      engine()->voe()->base()->audio_transport();
  send_streams_.insert(std::make_pair(ssrc, new WebRtcAudioSendStream(
      channel, audio_transport, ssrc, sp.cname, send_rtp_extensions_, call_)));

  // Set the current codecs to be used for the new channel. We need to do this
  // after adding the channel to send_channels_, because of how max bitrate is
  // currently being configured by SetSendCodec().
  if (!send_codecs_.empty() && !SetSendCodecs(channel, send_codecs_)) {
    RemoveSendStream(ssrc);
    return false;
  }

  // At this point the channel's local SSRC has been updated. If the channel is
  // the first send channel make sure that all the receive channels are updated
  // with the same SSRC in order to send receiver reports.
  if (send_streams_.size() == 1) {
    receiver_reports_ssrc_ = ssrc;
    for (const auto& stream : recv_streams_) {
      int recv_channel = stream.second->channel();
      if (engine()->voe()->rtp()->SetLocalSSRC(recv_channel, ssrc) != 0) {
        LOG_RTCERR2(SetLocalSSRC, recv_channel, ssrc);
        return false;
      }
      engine()->voe()->base()->AssociateSendChannel(recv_channel, channel);
      LOG(LS_INFO) << "VoiceEngine channel #" << recv_channel
                   << " is associated with channel #" << channel << ".";
    }
  }

  return ChangeSend(channel, desired_send_);
}

bool WebRtcVoiceMediaChannel::RemoveSendStream(uint32_t ssrc) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "RemoveSendStream: " << ssrc;

  auto it = send_streams_.find(ssrc);
  if (it == send_streams_.end()) {
    LOG(LS_WARNING) << "Try to remove stream with ssrc " << ssrc
                    << " which doesn't exist.";
    return false;
  }

  int channel = it->second->channel();
  ChangeSend(channel, SEND_NOTHING);

  // Clean up and delete the send stream+channel.
  LOG(LS_INFO) << "Removing audio send stream " << ssrc
               << " with VoiceEngine channel #" << channel << ".";
  delete it->second;
  send_streams_.erase(it);
  if (!DeleteVoEChannel(channel)) {
    return false;
  }
  if (send_streams_.empty()) {
    ChangeSend(SEND_NOTHING);
  }
  return true;
}

bool WebRtcVoiceMediaChannel::AddRecvStream(const StreamParams& sp) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "AddRecvStream: " << sp.ToString();

  if (!ValidateStreamParams(sp)) {
    return false;
  }

  const uint32_t ssrc = sp.first_ssrc();
  if (ssrc == 0) {
    LOG(LS_WARNING) << "AddRecvStream with ssrc==0 is not supported.";
    return false;
  }

  // Remove the default receive stream if one had been created with this ssrc;
  // we'll recreate it then.
  if (IsDefaultRecvStream(ssrc)) {
    RemoveRecvStream(ssrc);
  }

  if (GetReceiveChannelId(ssrc) != -1) {
    LOG(LS_ERROR) << "Stream already exists with ssrc " << ssrc;
    return false;
  }

  // Create a new channel for receiving audio data.
  const int channel = CreateVoEChannel();
  if (channel == -1) {
    return false;
  }

  // Turn off all supported codecs.
  // TODO(solenberg): Remove once "no codecs" is the default state of a stream.
  for (webrtc::CodecInst voe_codec : webrtc::acm2::RentACodec::Database()) {
    voe_codec.pltype = -1;
    if (engine()->voe()->codec()->SetRecPayloadType(channel, voe_codec) == -1) {
      LOG_RTCERR2(SetRecPayloadType, channel, ToString(voe_codec));
      DeleteVoEChannel(channel);
      return false;
    }
  }

  // Only enable those configured for this channel.
  for (const auto& codec : recv_codecs_) {
    webrtc::CodecInst voe_codec;
    if (WebRtcVoiceEngine::ToCodecInst(codec, &voe_codec)) {
      voe_codec.pltype = codec.id;
      if (engine()->voe()->codec()->SetRecPayloadType(
          channel, voe_codec) == -1) {
        LOG_RTCERR2(SetRecPayloadType, channel, ToString(voe_codec));
        DeleteVoEChannel(channel);
        return false;
      }
    }
  }

  const int send_channel = GetSendChannelId(receiver_reports_ssrc_);
  if (send_channel != -1) {
    // Associate receive channel with first send channel (so the receive channel
    // can obtain RTT from the send channel)
    engine()->voe()->base()->AssociateSendChannel(channel, send_channel);
    LOG(LS_INFO) << "VoiceEngine channel #" << channel
                 << " is associated with channel #" << send_channel << ".";
  }

  recv_streams_.insert(std::make_pair(ssrc, new WebRtcAudioReceiveStream(
      channel, ssrc, receiver_reports_ssrc_,
      options_.combined_audio_video_bwe.value_or(false), sp.sync_label,
      recv_rtp_extensions_, call_)));

  SetNack(channel, nack_enabled_);
  SetPlayout(channel, playout_);

  return true;
}

bool WebRtcVoiceMediaChannel::RemoveRecvStream(uint32_t ssrc) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "RemoveRecvStream: " << ssrc;

  const auto it = recv_streams_.find(ssrc);
  if (it == recv_streams_.end()) {
    LOG(LS_WARNING) << "Try to remove stream with ssrc " << ssrc
                    << " which doesn't exist.";
    return false;
  }

  // Deregister default channel, if that's the one being destroyed.
  if (IsDefaultRecvStream(ssrc)) {
    default_recv_ssrc_ = -1;
  }

  const int channel = it->second->channel();

  // Clean up and delete the receive stream+channel.
  LOG(LS_INFO) << "Removing audio receive stream " << ssrc
               << " with VoiceEngine channel #" << channel << ".";
  it->second->SetRawAudioSink(nullptr);
  delete it->second;
  recv_streams_.erase(it);
  return DeleteVoEChannel(channel);
}

bool WebRtcVoiceMediaChannel::SetLocalRenderer(uint32_t ssrc,
                                               AudioRenderer* renderer) {
  auto it = send_streams_.find(ssrc);
  if (it == send_streams_.end()) {
    if (renderer) {
      // Return an error if trying to set a valid renderer with an invalid ssrc.
      LOG(LS_ERROR) << "SetLocalRenderer failed with ssrc "<< ssrc;
      return false;
    }

    // The channel likely has gone away, do nothing.
    return true;
  }

  if (renderer) {
    it->second->Start(renderer);
  } else {
    it->second->Stop();
  }

  return true;
}

bool WebRtcVoiceMediaChannel::GetActiveStreams(
    AudioInfo::StreamList* actives) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  actives->clear();
  for (const auto& ch : recv_streams_) {
    int level = GetOutputLevel(ch.second->channel());
    if (level > 0) {
      actives->push_back(std::make_pair(ch.first, level));
    }
  }
  return true;
}

int WebRtcVoiceMediaChannel::GetOutputLevel() {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  int highest = 0;
  for (const auto& ch : recv_streams_) {
    highest = std::max(GetOutputLevel(ch.second->channel()), highest);
  }
  return highest;
}

int WebRtcVoiceMediaChannel::GetTimeSinceLastTyping() {
  int ret;
  if (engine()->voe()->processing()->TimeSinceLastTyping(ret) == -1) {
    // In case of error, log the info and continue
    LOG_RTCERR0(TimeSinceLastTyping);
    ret = -1;
  } else {
    ret *= 1000;  // We return ms, webrtc returns seconds.
  }
  return ret;
}

void WebRtcVoiceMediaChannel::SetTypingDetectionParameters(int time_window,
    int cost_per_typing, int reporting_threshold, int penalty_decay,
    int type_event_delay) {
  if (engine()->voe()->processing()->SetTypingDetectionParameters(
          time_window, cost_per_typing,
          reporting_threshold, penalty_decay, type_event_delay) == -1) {
    // In case of error, log the info and continue
    LOG_RTCERR5(SetTypingDetectionParameters, time_window,
                cost_per_typing, reporting_threshold, penalty_decay,
                type_event_delay);
  }
}

bool WebRtcVoiceMediaChannel::SetOutputVolume(uint32_t ssrc, double volume) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  if (ssrc == 0) {
    default_recv_volume_ = volume;
    if (default_recv_ssrc_ == -1) {
      return true;
    }
    ssrc = static_cast<uint32_t>(default_recv_ssrc_);
  }
  int ch_id = GetReceiveChannelId(ssrc);
  if (ch_id < 0) {
    LOG(LS_WARNING) << "Cannot find channel for ssrc:" << ssrc;
    return false;
  }

  if (-1 == engine()->voe()->volume()->SetChannelOutputVolumeScaling(ch_id,
                                                                     volume)) {
    LOG_RTCERR2(SetChannelOutputVolumeScaling, ch_id, volume);
    return false;
  }
  LOG(LS_INFO) << "SetOutputVolume to " << volume
               << " for channel " << ch_id << " and ssrc " << ssrc;
  return true;
}

bool WebRtcVoiceMediaChannel::CanInsertDtmf() {
  return dtmf_payload_type_ ? true : false;
}

bool WebRtcVoiceMediaChannel::InsertDtmf(uint32_t ssrc, int event,
                                         int duration) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "WebRtcVoiceMediaChannel::InsertDtmf";
  if (!dtmf_payload_type_) {
    return false;
  }

  // Figure out which WebRtcAudioSendStream to send the event on.
  auto it = ssrc != 0 ? send_streams_.find(ssrc) : send_streams_.begin();
  if (it == send_streams_.end()) {
    LOG(LS_WARNING) << "The specified ssrc " << ssrc << " is not in use.";
    return false;
  }
  if (event < kMinTelephoneEventCode ||
      event > kMaxTelephoneEventCode) {
    LOG(LS_WARNING) << "DTMF event code " << event << " out of range.";
    return false;
  }
  if (duration < kMinTelephoneEventDuration ||
      duration > kMaxTelephoneEventDuration) {
    LOG(LS_WARNING) << "DTMF event duration " << duration << " out of range.";
    return false;
  }
  return it->second->SendTelephoneEvent(*dtmf_payload_type_, event, duration);
}

void WebRtcVoiceMediaChannel::OnPacketReceived(
    rtc::Buffer* packet, const rtc::PacketTime& packet_time) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());

  uint32_t ssrc = 0;
  if (!GetRtpSsrc(packet->data(), packet->size(), &ssrc)) {
    return;
  }

  // If we don't have a default channel, and the SSRC is unknown, create a
  // default channel.
  if (default_recv_ssrc_ == -1 && GetReceiveChannelId(ssrc) == -1) {
    StreamParams sp;
    sp.ssrcs.push_back(ssrc);
    LOG(LS_INFO) << "Creating default receive stream for SSRC=" << ssrc << ".";
    if (!AddRecvStream(sp)) {
      LOG(LS_WARNING) << "Could not create default receive stream.";
      return;
    }
    default_recv_ssrc_ = ssrc;
    SetOutputVolume(default_recv_ssrc_, default_recv_volume_);
  }

  // Forward packet to Call. If the SSRC is unknown we'll return after this.
  const webrtc::PacketTime webrtc_packet_time(packet_time.timestamp,
                                              packet_time.not_before);
  webrtc::PacketReceiver::DeliveryStatus delivery_result =
      call_->Receiver()->DeliverPacket(webrtc::MediaType::AUDIO,
          reinterpret_cast<const uint8_t*>(packet->data()), packet->size(),
          webrtc_packet_time);
  if (webrtc::PacketReceiver::DELIVERY_OK != delivery_result) {
    // If the SSRC is unknown here, route it to the default channel, if we have
    // one. See: https://bugs.chromium.org/p/webrtc/issues/detail?id=5208
    if (default_recv_ssrc_ == -1) {
      return;
    } else {
      ssrc = default_recv_ssrc_;
    }
  }

  // Find the channel to send this packet to. It must exist since webrtc::Call
  // was able to demux the packet.
  int channel = GetReceiveChannelId(ssrc);
  RTC_DCHECK(channel != -1);

  // Pass it off to the decoder.
  engine()->voe()->network()->ReceivedRTPPacket(
      channel, packet->data(), packet->size(), webrtc_packet_time);
}

void WebRtcVoiceMediaChannel::OnRtcpReceived(
    rtc::Buffer* packet, const rtc::PacketTime& packet_time) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());

  // Forward packet to Call as well.
  const webrtc::PacketTime webrtc_packet_time(packet_time.timestamp,
                                              packet_time.not_before);
  call_->Receiver()->DeliverPacket(webrtc::MediaType::AUDIO,
      reinterpret_cast<const uint8_t*>(packet->data()), packet->size(),
      webrtc_packet_time);

  // Sending channels need all RTCP packets with feedback information.
  // Even sender reports can contain attached report blocks.
  // Receiving channels need sender reports in order to create
  // correct receiver reports.
  int type = 0;
  if (!GetRtcpType(packet->data(), packet->size(), &type)) {
    LOG(LS_WARNING) << "Failed to parse type from received RTCP packet";
    return;
  }

  // If it is a sender report, find the receive channel that is listening.
  if (type == kRtcpTypeSR) {
    uint32_t ssrc = 0;
    if (!GetRtcpSsrc(packet->data(), packet->size(), &ssrc)) {
      return;
    }
    int recv_channel_id = GetReceiveChannelId(ssrc);
    if (recv_channel_id != -1) {
      engine()->voe()->network()->ReceivedRTCPPacket(
          recv_channel_id, packet->data(), packet->size());
    }
  }

  // SR may continue RR and any RR entry may correspond to any one of the send
  // channels. So all RTCP packets must be forwarded all send channels. VoE
  // will filter out RR internally.
  for (const auto& ch : send_streams_) {
    engine()->voe()->network()->ReceivedRTCPPacket(
        ch.second->channel(), packet->data(), packet->size());
  }
}

bool WebRtcVoiceMediaChannel::MuteStream(uint32_t ssrc, bool muted) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  int channel = GetSendChannelId(ssrc);
  if (channel == -1) {
    LOG(LS_WARNING) << "The specified ssrc " << ssrc << " is not in use.";
    return false;
  }
  if (engine()->voe()->volume()->SetInputMute(channel, muted) == -1) {
    LOG_RTCERR2(SetInputMute, channel, muted);
    return false;
  }
  // We set the AGC to mute state only when all the channels are muted.
  // This implementation is not ideal, instead we should signal the AGC when
  // the mic channel is muted/unmuted. We can't do it today because there
  // is no good way to know which stream is mapping to the mic channel.
  bool all_muted = muted;
  for (const auto& ch : send_streams_) {
    if (!all_muted) {
      break;
    }
    if (engine()->voe()->volume()->GetInputMute(ch.second->channel(),
                                                all_muted)) {
      LOG_RTCERR1(GetInputMute, ch.second->channel());
      return false;
    }
  }

  webrtc::AudioProcessing* ap = engine()->voe()->base()->audio_processing();
  if (ap) {
    ap->set_output_will_be_muted(all_muted);
  }
  return true;
}

// TODO(minyue): SetMaxSendBandwidth() is subject to be renamed to
// SetMaxSendBitrate() in future.
bool WebRtcVoiceMediaChannel::SetMaxSendBandwidth(int bps) {
  LOG(LS_INFO) << "WebRtcVoiceMediaChannel::SetMaxSendBandwidth.";
  return SetSendBitrateInternal(bps);
}

bool WebRtcVoiceMediaChannel::SetSendBitrateInternal(int bps) {
  LOG(LS_INFO) << "WebRtcVoiceMediaChannel::SetSendBitrateInternal.";

  send_bitrate_setting_ = true;
  send_bitrate_bps_ = bps;

  if (!send_codec_) {
    LOG(LS_INFO) << "The send codec has not been set up yet. "
                 << "The send bitrate setting will be applied later.";
    return true;
  }

  // Bitrate is auto by default.
  // TODO(bemasc): Fix this so that if SetMaxSendBandwidth(50) is followed by
  // SetMaxSendBandwith(0), the second call removes the previous limit.
  if (bps <= 0)
    return true;

  webrtc::CodecInst codec = *send_codec_;
  bool is_multi_rate = WebRtcVoiceCodecs::IsCodecMultiRate(codec);

  if (is_multi_rate) {
    // If codec is multi-rate then just set the bitrate.
    codec.rate = bps;
    for (const auto& ch : send_streams_) {
      if (!SetSendCodec(ch.second->channel(), codec)) {
        LOG(LS_INFO) << "Failed to set codec " << codec.plname
                     << " to bitrate " << bps << " bps.";
        return false;
      }
    }
    return true;
  } else {
    // If codec is not multi-rate and |bps| is less than the fixed bitrate
    // then fail. If codec is not multi-rate and |bps| exceeds or equal the
    // fixed bitrate then ignore.
    if (bps < codec.rate) {
      LOG(LS_INFO) << "Failed to set codec " << codec.plname
                   << " to bitrate " << bps << " bps"
                   << ", requires at least " << codec.rate << " bps.";
      return false;
    }
    return true;
  }
}

bool WebRtcVoiceMediaChannel::GetStats(VoiceMediaInfo* info) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  RTC_DCHECK(info);

  // Get SSRC and stats for each sender.
  RTC_DCHECK(info->senders.size() == 0);
  for (const auto& stream : send_streams_) {
    webrtc::AudioSendStream::Stats stats = stream.second->GetStats();
    VoiceSenderInfo sinfo;
    sinfo.add_ssrc(stats.local_ssrc);
    sinfo.bytes_sent = stats.bytes_sent;
    sinfo.packets_sent = stats.packets_sent;
    sinfo.packets_lost = stats.packets_lost;
    sinfo.fraction_lost = stats.fraction_lost;
    sinfo.codec_name = stats.codec_name;
    sinfo.ext_seqnum = stats.ext_seqnum;
    sinfo.jitter_ms = stats.jitter_ms;
    sinfo.rtt_ms = stats.rtt_ms;
    sinfo.audio_level = stats.audio_level;
    sinfo.aec_quality_min = stats.aec_quality_min;
    sinfo.echo_delay_median_ms = stats.echo_delay_median_ms;
    sinfo.echo_delay_std_ms = stats.echo_delay_std_ms;
    sinfo.echo_return_loss = stats.echo_return_loss;
    sinfo.echo_return_loss_enhancement = stats.echo_return_loss_enhancement;
    sinfo.typing_noise_detected =
        (send_ == SEND_NOTHING ? false : stats.typing_noise_detected);
    info->senders.push_back(sinfo);
  }

  // Get SSRC and stats for each receiver.
  RTC_DCHECK(info->receivers.size() == 0);
  for (const auto& stream : recv_streams_) {
    webrtc::AudioReceiveStream::Stats stats = stream.second->GetStats();
    VoiceReceiverInfo rinfo;
    rinfo.add_ssrc(stats.remote_ssrc);
    rinfo.bytes_rcvd = stats.bytes_rcvd;
    rinfo.packets_rcvd = stats.packets_rcvd;
    rinfo.packets_lost = stats.packets_lost;
    rinfo.fraction_lost = stats.fraction_lost;
    rinfo.codec_name = stats.codec_name;
    rinfo.ext_seqnum = stats.ext_seqnum;
    rinfo.jitter_ms = stats.jitter_ms;
    rinfo.jitter_buffer_ms = stats.jitter_buffer_ms;
    rinfo.jitter_buffer_preferred_ms = stats.jitter_buffer_preferred_ms;
    rinfo.delay_estimate_ms = stats.delay_estimate_ms;
    rinfo.audio_level = stats.audio_level;
    rinfo.expand_rate = stats.expand_rate;
    rinfo.speech_expand_rate = stats.speech_expand_rate;
    rinfo.secondary_decoded_rate = stats.secondary_decoded_rate;
    rinfo.accelerate_rate = stats.accelerate_rate;
    rinfo.preemptive_expand_rate = stats.preemptive_expand_rate;
    rinfo.decoding_calls_to_silence_generator =
        stats.decoding_calls_to_silence_generator;
    rinfo.decoding_calls_to_neteq = stats.decoding_calls_to_neteq;
    rinfo.decoding_normal = stats.decoding_normal;
    rinfo.decoding_plc = stats.decoding_plc;
    rinfo.decoding_cng = stats.decoding_cng;
    rinfo.decoding_plc_cng = stats.decoding_plc_cng;
    rinfo.capture_start_ntp_time_ms = stats.capture_start_ntp_time_ms;
    info->receivers.push_back(rinfo);
  }

  return true;
}

void WebRtcVoiceMediaChannel::SetRawAudioSink(
    uint32_t ssrc,
    rtc::scoped_ptr<webrtc::AudioSinkInterface> sink) {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  LOG(LS_VERBOSE) << "WebRtcVoiceMediaChannel::SetRawAudioSink";
  const auto it = recv_streams_.find(ssrc);
  if (it == recv_streams_.end()) {
    LOG(LS_WARNING) << "SetRawAudioSink: no recv stream" << ssrc;
    return;
  }
  it->second->SetRawAudioSink(std::move(sink));
}

int WebRtcVoiceMediaChannel::GetOutputLevel(int channel) {
  unsigned int ulevel = 0;
  int ret = engine()->voe()->volume()->GetSpeechOutputLevel(channel, ulevel);
  return (ret == 0) ? static_cast<int>(ulevel) : -1;
}

int WebRtcVoiceMediaChannel::GetReceiveChannelId(uint32_t ssrc) const {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  const auto it = recv_streams_.find(ssrc);
  if (it != recv_streams_.end()) {
    return it->second->channel();
  }
  return -1;
}

int WebRtcVoiceMediaChannel::GetSendChannelId(uint32_t ssrc) const {
  RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
  const auto it = send_streams_.find(ssrc);
  if (it != send_streams_.end()) {
    return it->second->channel();
  }
  return -1;
}

bool WebRtcVoiceMediaChannel::GetRedSendCodec(const AudioCodec& red_codec,
    const std::vector<AudioCodec>& all_codecs, webrtc::CodecInst* send_codec) {
  // Get the RED encodings from the parameter with no name. This may
  // change based on what is discussed on the Jingle list.
  // The encoding parameter is of the form "a/b"; we only support where
  // a == b. Verify this and parse out the value into red_pt.
  // If the parameter value is absent (as it will be until we wire up the
  // signaling of this message), use the second codec specified (i.e. the
  // one after "red") as the encoding parameter.
  int red_pt = -1;
  std::string red_params;
  CodecParameterMap::const_iterator it = red_codec.params.find("");
  if (it != red_codec.params.end()) {
    red_params = it->second;
    std::vector<std::string> red_pts;
    if (rtc::split(red_params, '/', &red_pts) != 2 ||
        red_pts[0] != red_pts[1] ||
        !rtc::FromString(red_pts[0], &red_pt)) {
      LOG(LS_WARNING) << "RED params " << red_params << " not supported.";
      return false;
    }
  } else if (red_codec.params.empty()) {
    LOG(LS_WARNING) << "RED params not present, using defaults";
    if (all_codecs.size() > 1) {
      red_pt = all_codecs[1].id;
    }
  }

  // Try to find red_pt in |codecs|.
  for (const AudioCodec& codec : all_codecs) {
    if (codec.id == red_pt) {
      // If we find the right codec, that will be the codec we pass to
      // SetSendCodec, with the desired payload type.
      if (WebRtcVoiceEngine::ToCodecInst(codec, send_codec)) {
        return true;
      } else {
        break;
      }
    }
  }
  LOG(LS_WARNING) << "RED params " << red_params << " are invalid.";
  return false;
}

bool WebRtcVoiceMediaChannel::SetPlayout(int channel, bool playout) {
  if (playout) {
    LOG(LS_INFO) << "Starting playout for channel #" << channel;
    if (engine()->voe()->base()->StartPlayout(channel) == -1) {
      LOG_RTCERR1(StartPlayout, channel);
      return false;
    }
  } else {
    LOG(LS_INFO) << "Stopping playout for channel #" << channel;
    engine()->voe()->base()->StopPlayout(channel);
  }
  return true;
}
}  // namespace cricket

#endif  // HAVE_WEBRTC_VOICE
