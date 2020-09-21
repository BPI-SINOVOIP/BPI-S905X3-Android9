/*
 * libjingle
 * Copyright 2013 Google Inc.
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

// Hints for future visitors:
// This entire file is an implementation detail of the org.webrtc Java package,
// the most interesting bits of which are org.webrtc.PeerConnection{,Factory}.
// The layout of this file is roughly:
// - various helper C++ functions & classes that wrap Java counterparts and
//   expose a C++ interface that can be passed to the C++ PeerConnection APIs
// - implementations of methods declared "static" in the Java package (named
//   things like Java_org_webrtc_OMG_Can_This_Name_Be_Any_Longer, prescribed by
//   the JNI spec).
//
// Lifecycle notes: objects are owned where they will be called; in other words
// FooObservers are owned by C++-land, and user-callable objects (e.g.
// PeerConnection and VideoTrack) are owned by Java-land.
// When this file allocates C++ RefCountInterfaces it AddRef()s an artificial
// ref simulating the jlong held in Java-land, and then Release()s the ref in
// the respective free call.  Sometimes this AddRef is implicit in the
// construction of a scoped_refptr<> which is then .release()d.
// Any persistent (non-local) references from C++ to Java must be global or weak
// (in which case they must be checked before use)!
//
// Exception notes: pretty much all JNI calls can throw Java exceptions, so each
// call through a JNIEnv* pointer needs to be followed by an ExceptionCheck()
// call.  In this file this is done in CHECK_EXCEPTION, making for much easier
// debugging in case of failure (the alternative is to wait for control to
// return to the Java frame that called code in this file, at which point it's
// impossible to tell which JNI call broke).

#include <jni.h>
#undef JNIEXPORT
#define JNIEXPORT __attribute__((visibility("default")))

#include <limits>
#include <utility>

#include "talk/app/webrtc/java/jni/classreferenceholder.h"
#include "talk/app/webrtc/java/jni/jni_helpers.h"
#include "talk/app/webrtc/java/jni/native_handle_impl.h"
#include "talk/app/webrtc/dtlsidentitystore.h"
#include "talk/app/webrtc/mediaconstraintsinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "talk/app/webrtc/rtpreceiverinterface.h"
#include "talk/app/webrtc/rtpsenderinterface.h"
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/media/base/videocapturer.h"
#include "talk/media/base/videorenderer.h"
#include "talk/media/devices/videorendererfactory.h"
#include "talk/media/webrtc/webrtcvideodecoderfactory.h"
#include "talk/media/webrtc/webrtcvideoencoderfactory.h"
#include "webrtc/base/bind.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/event_tracer.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/logsinks.h"
#include "webrtc/base/messagequeue.h"
#include "webrtc/base/networkmonitor.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/system_wrappers/include/field_trial_default.h"
#include "webrtc/system_wrappers/include/trace.h"
#include "webrtc/voice_engine/include/voe_base.h"

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
#include "talk/app/webrtc/androidvideocapturer.h"
#include "talk/app/webrtc/java/jni/androidmediadecoder_jni.h"
#include "talk/app/webrtc/java/jni/androidmediaencoder_jni.h"
#include "talk/app/webrtc/java/jni/androidvideocapturer_jni.h"
#include "talk/app/webrtc/java/jni/androidnetworkmonitor_jni.h"
#include "webrtc/modules/video_render/video_render_internal.h"
#include "webrtc/system_wrappers/include/logcat_trace_context.h"
using webrtc::LogcatTraceContext;
#endif

using cricket::WebRtcVideoDecoderFactory;
using cricket::WebRtcVideoEncoderFactory;
using rtc::Bind;
using rtc::Thread;
using rtc::ThreadManager;
using rtc::scoped_ptr;
using webrtc::AudioSourceInterface;
using webrtc::AudioTrackInterface;
using webrtc::AudioTrackVector;
using webrtc::CreateSessionDescriptionObserver;
using webrtc::DataBuffer;
using webrtc::DataChannelInit;
using webrtc::DataChannelInterface;
using webrtc::DataChannelObserver;
using webrtc::IceCandidateInterface;
using webrtc::MediaConstraintsInterface;
using webrtc::MediaSourceInterface;
using webrtc::MediaStreamInterface;
using webrtc::MediaStreamTrackInterface;
using webrtc::PeerConnectionFactoryInterface;
using webrtc::PeerConnectionInterface;
using webrtc::PeerConnectionObserver;
using webrtc::RtpReceiverInterface;
using webrtc::RtpSenderInterface;
using webrtc::SessionDescriptionInterface;
using webrtc::SetSessionDescriptionObserver;
using webrtc::StatsObserver;
using webrtc::StatsReport;
using webrtc::StatsReports;
using webrtc::VideoRendererInterface;
using webrtc::VideoSourceInterface;
using webrtc::VideoTrackInterface;
using webrtc::VideoTrackVector;
using webrtc::kVideoCodecVP8;

namespace webrtc_jni {

// Field trials initialization string
static char *field_trials_init_string = NULL;

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
// Set in PeerConnectionFactory_initializeAndroidGlobals().
static bool factory_static_initialized = false;
static bool video_hw_acceleration_enabled = true;
#endif

// Return the (singleton) Java Enum object corresponding to |index|;
// |state_class_fragment| is something like "MediaSource$State".
static jobject JavaEnumFromIndex(
    JNIEnv* jni, const std::string& state_class_fragment, int index) {
  const std::string state_class = "org/webrtc/" + state_class_fragment;
  return JavaEnumFromIndex(jni, FindClass(jni, state_class.c_str()),
                           state_class, index);
}

static DataChannelInit JavaDataChannelInitToNative(
    JNIEnv* jni, jobject j_init) {
  DataChannelInit init;

  jclass j_init_class = FindClass(jni, "org/webrtc/DataChannel$Init");
  jfieldID ordered_id = GetFieldID(jni, j_init_class, "ordered", "Z");
  jfieldID max_retransmit_time_id =
      GetFieldID(jni, j_init_class, "maxRetransmitTimeMs", "I");
  jfieldID max_retransmits_id =
      GetFieldID(jni, j_init_class, "maxRetransmits", "I");
  jfieldID protocol_id =
      GetFieldID(jni, j_init_class, "protocol", "Ljava/lang/String;");
  jfieldID negotiated_id = GetFieldID(jni, j_init_class, "negotiated", "Z");
  jfieldID id_id = GetFieldID(jni, j_init_class, "id", "I");

  init.ordered = GetBooleanField(jni, j_init, ordered_id);
  init.maxRetransmitTime = GetIntField(jni, j_init, max_retransmit_time_id);
  init.maxRetransmits = GetIntField(jni, j_init, max_retransmits_id);
  init.protocol = JavaToStdString(
      jni, GetStringField(jni, j_init, protocol_id));
  init.negotiated = GetBooleanField(jni, j_init, negotiated_id);
  init.id = GetIntField(jni, j_init, id_id);

  return init;
}

class ConstraintsWrapper;

// Adapter between the C++ PeerConnectionObserver interface and the Java
// PeerConnection.Observer interface.  Wraps an instance of the Java interface
// and dispatches C++ callbacks to Java.
class PCOJava : public PeerConnectionObserver {
 public:
  PCOJava(JNIEnv* jni, jobject j_observer)
      : j_observer_global_(jni, j_observer),
        j_observer_class_(jni, GetObjectClass(jni, *j_observer_global_)),
        j_media_stream_class_(jni, FindClass(jni, "org/webrtc/MediaStream")),
        j_media_stream_ctor_(GetMethodID(
            jni, *j_media_stream_class_, "<init>", "(J)V")),
        j_audio_track_class_(jni, FindClass(jni, "org/webrtc/AudioTrack")),
        j_audio_track_ctor_(GetMethodID(
            jni, *j_audio_track_class_, "<init>", "(J)V")),
        j_video_track_class_(jni, FindClass(jni, "org/webrtc/VideoTrack")),
        j_video_track_ctor_(GetMethodID(
            jni, *j_video_track_class_, "<init>", "(J)V")),
        j_data_channel_class_(jni, FindClass(jni, "org/webrtc/DataChannel")),
        j_data_channel_ctor_(GetMethodID(
            jni, *j_data_channel_class_, "<init>", "(J)V")) {
  }

  virtual ~PCOJava() {
    ScopedLocalRefFrame local_ref_frame(jni());
    while (!remote_streams_.empty())
      DisposeRemoteStream(remote_streams_.begin());
  }

  void OnIceCandidate(const IceCandidateInterface* candidate) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    std::string sdp;
    RTC_CHECK(candidate->ToString(&sdp)) << "got so far: " << sdp;
    jclass candidate_class = FindClass(jni(), "org/webrtc/IceCandidate");
    jmethodID ctor = GetMethodID(jni(), candidate_class,
        "<init>", "(Ljava/lang/String;ILjava/lang/String;)V");
    jstring j_mid = JavaStringFromStdString(jni(), candidate->sdp_mid());
    jstring j_sdp = JavaStringFromStdString(jni(), sdp);
    jobject j_candidate = jni()->NewObject(
        candidate_class, ctor, j_mid, candidate->sdp_mline_index(), j_sdp);
    CHECK_EXCEPTION(jni()) << "error during NewObject";
    jmethodID m = GetMethodID(jni(), *j_observer_class_,
                              "onIceCandidate", "(Lorg/webrtc/IceCandidate;)V");
    jni()->CallVoidMethod(*j_observer_global_, m, j_candidate);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnSignalingChange(
      PeerConnectionInterface::SignalingState new_state) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jmethodID m = GetMethodID(
        jni(), *j_observer_class_, "onSignalingChange",
        "(Lorg/webrtc/PeerConnection$SignalingState;)V");
    jobject new_state_enum =
        JavaEnumFromIndex(jni(), "PeerConnection$SignalingState", new_state);
    jni()->CallVoidMethod(*j_observer_global_, m, new_state_enum);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnIceConnectionChange(
      PeerConnectionInterface::IceConnectionState new_state) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jmethodID m = GetMethodID(
        jni(), *j_observer_class_, "onIceConnectionChange",
        "(Lorg/webrtc/PeerConnection$IceConnectionState;)V");
    jobject new_state_enum = JavaEnumFromIndex(
        jni(), "PeerConnection$IceConnectionState", new_state);
    jni()->CallVoidMethod(*j_observer_global_, m, new_state_enum);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnIceConnectionReceivingChange(bool receiving) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jmethodID m = GetMethodID(
        jni(), *j_observer_class_, "onIceConnectionReceivingChange", "(Z)V");
    jni()->CallVoidMethod(*j_observer_global_, m, receiving);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnIceGatheringChange(
      PeerConnectionInterface::IceGatheringState new_state) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jmethodID m = GetMethodID(
        jni(), *j_observer_class_, "onIceGatheringChange",
        "(Lorg/webrtc/PeerConnection$IceGatheringState;)V");
    jobject new_state_enum = JavaEnumFromIndex(
        jni(), "PeerConnection$IceGatheringState", new_state);
    jni()->CallVoidMethod(*j_observer_global_, m, new_state_enum);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnAddStream(MediaStreamInterface* stream) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    // Java MediaStream holds one reference. Corresponding Release() is in
    // MediaStream_free, triggered by MediaStream.dispose().
    stream->AddRef();
    jobject j_stream =
        jni()->NewObject(*j_media_stream_class_, j_media_stream_ctor_,
                         reinterpret_cast<jlong>(stream));
    CHECK_EXCEPTION(jni()) << "error during NewObject";

    for (const auto& track : stream->GetAudioTracks()) {
      jstring id = JavaStringFromStdString(jni(), track->id());
      // Java AudioTrack holds one reference. Corresponding Release() is in
      // MediaStreamTrack_free, triggered by AudioTrack.dispose().
      track->AddRef();
      jobject j_track =
          jni()->NewObject(*j_audio_track_class_, j_audio_track_ctor_,
                           reinterpret_cast<jlong>(track.get()), id);
      CHECK_EXCEPTION(jni()) << "error during NewObject";
      jfieldID audio_tracks_id = GetFieldID(jni(),
                                            *j_media_stream_class_,
                                            "audioTracks",
                                            "Ljava/util/LinkedList;");
      jobject audio_tracks = GetObjectField(jni(), j_stream, audio_tracks_id);
      jmethodID add = GetMethodID(jni(),
                                  GetObjectClass(jni(), audio_tracks),
                                  "add",
                                  "(Ljava/lang/Object;)Z");
      jboolean added = jni()->CallBooleanMethod(audio_tracks, add, j_track);
      CHECK_EXCEPTION(jni()) << "error during CallBooleanMethod";
      RTC_CHECK(added);
    }

    for (const auto& track : stream->GetVideoTracks()) {
      jstring id = JavaStringFromStdString(jni(), track->id());
      // Java VideoTrack holds one reference. Corresponding Release() is in
      // MediaStreamTrack_free, triggered by VideoTrack.dispose().
      track->AddRef();
      jobject j_track =
          jni()->NewObject(*j_video_track_class_, j_video_track_ctor_,
                           reinterpret_cast<jlong>(track.get()), id);
      CHECK_EXCEPTION(jni()) << "error during NewObject";
      jfieldID video_tracks_id = GetFieldID(jni(),
                                            *j_media_stream_class_,
                                            "videoTracks",
                                            "Ljava/util/LinkedList;");
      jobject video_tracks = GetObjectField(jni(), j_stream, video_tracks_id);
      jmethodID add = GetMethodID(jni(),
                                  GetObjectClass(jni(), video_tracks),
                                  "add",
                                  "(Ljava/lang/Object;)Z");
      jboolean added = jni()->CallBooleanMethod(video_tracks, add, j_track);
      CHECK_EXCEPTION(jni()) << "error during CallBooleanMethod";
      RTC_CHECK(added);
    }
    remote_streams_[stream] = NewGlobalRef(jni(), j_stream);

    jmethodID m = GetMethodID(jni(), *j_observer_class_, "onAddStream",
                              "(Lorg/webrtc/MediaStream;)V");
    jni()->CallVoidMethod(*j_observer_global_, m, j_stream);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnRemoveStream(MediaStreamInterface* stream) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    NativeToJavaStreamsMap::iterator it = remote_streams_.find(stream);
    RTC_CHECK(it != remote_streams_.end()) << "unexpected stream: " << std::hex
                                           << stream;
    jobject j_stream = it->second;
    jmethodID m = GetMethodID(jni(), *j_observer_class_, "onRemoveStream",
                              "(Lorg/webrtc/MediaStream;)V");
    jni()->CallVoidMethod(*j_observer_global_, m, j_stream);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
    DisposeRemoteStream(it);
  }

  void OnDataChannel(DataChannelInterface* channel) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jobject j_channel = jni()->NewObject(
        *j_data_channel_class_, j_data_channel_ctor_, (jlong)channel);
    CHECK_EXCEPTION(jni()) << "error during NewObject";

    jmethodID m = GetMethodID(jni(), *j_observer_class_, "onDataChannel",
                              "(Lorg/webrtc/DataChannel;)V");
    jni()->CallVoidMethod(*j_observer_global_, m, j_channel);

    // Channel is now owned by Java object, and will be freed from
    // DataChannel.dispose().  Important that this be done _after_ the
    // CallVoidMethod above as Java code might call back into native code and be
    // surprised to see a refcount of 2.
    int bumped_count = channel->AddRef();
    RTC_CHECK(bumped_count == 2) << "Unexpected refcount OnDataChannel";

    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnRenegotiationNeeded() override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jmethodID m =
        GetMethodID(jni(), *j_observer_class_, "onRenegotiationNeeded", "()V");
    jni()->CallVoidMethod(*j_observer_global_, m);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void SetConstraints(ConstraintsWrapper* constraints) {
    RTC_CHECK(!constraints_.get()) << "constraints already set!";
    constraints_.reset(constraints);
  }

  const ConstraintsWrapper* constraints() { return constraints_.get(); }

 private:
  typedef std::map<MediaStreamInterface*, jobject> NativeToJavaStreamsMap;

  void DisposeRemoteStream(const NativeToJavaStreamsMap::iterator& it) {
    jobject j_stream = it->second;
    remote_streams_.erase(it);
    jni()->CallVoidMethod(
        j_stream, GetMethodID(jni(), *j_media_stream_class_, "dispose", "()V"));
    CHECK_EXCEPTION(jni()) << "error during MediaStream.dispose()";
    DeleteGlobalRef(jni(), j_stream);
  }

  JNIEnv* jni() {
    return AttachCurrentThreadIfNeeded();
  }

  const ScopedGlobalRef<jobject> j_observer_global_;
  const ScopedGlobalRef<jclass> j_observer_class_;
  const ScopedGlobalRef<jclass> j_media_stream_class_;
  const jmethodID j_media_stream_ctor_;
  const ScopedGlobalRef<jclass> j_audio_track_class_;
  const jmethodID j_audio_track_ctor_;
  const ScopedGlobalRef<jclass> j_video_track_class_;
  const jmethodID j_video_track_ctor_;
  const ScopedGlobalRef<jclass> j_data_channel_class_;
  const jmethodID j_data_channel_ctor_;
  // C++ -> Java remote streams. The stored jobects are global refs and must be
  // manually deleted upon removal. Use DisposeRemoteStream().
  NativeToJavaStreamsMap remote_streams_;
  scoped_ptr<ConstraintsWrapper> constraints_;
};

// Wrapper for a Java MediaConstraints object.  Copies all needed data so when
// the constructor returns the Java object is no longer needed.
class ConstraintsWrapper : public MediaConstraintsInterface {
 public:
  ConstraintsWrapper(JNIEnv* jni, jobject j_constraints) {
    PopulateConstraintsFromJavaPairList(
        jni, j_constraints, "mandatory", &mandatory_);
    PopulateConstraintsFromJavaPairList(
        jni, j_constraints, "optional", &optional_);
  }

  virtual ~ConstraintsWrapper() {}

  // MediaConstraintsInterface.
  const Constraints& GetMandatory() const override { return mandatory_; }

  const Constraints& GetOptional() const override { return optional_; }

 private:
  // Helper for translating a List<Pair<String, String>> to a Constraints.
  static void PopulateConstraintsFromJavaPairList(
      JNIEnv* jni, jobject j_constraints,
      const char* field_name, Constraints* field) {
    jfieldID j_id = GetFieldID(jni,
        GetObjectClass(jni, j_constraints), field_name, "Ljava/util/List;");
    jobject j_list = GetObjectField(jni, j_constraints, j_id);
    jmethodID j_iterator_id = GetMethodID(jni,
        GetObjectClass(jni, j_list), "iterator", "()Ljava/util/Iterator;");
    jobject j_iterator = jni->CallObjectMethod(j_list, j_iterator_id);
    CHECK_EXCEPTION(jni) << "error during CallObjectMethod";
    jmethodID j_has_next = GetMethodID(jni,
        GetObjectClass(jni, j_iterator), "hasNext", "()Z");
    jmethodID j_next = GetMethodID(jni,
        GetObjectClass(jni, j_iterator), "next", "()Ljava/lang/Object;");
    while (jni->CallBooleanMethod(j_iterator, j_has_next)) {
      CHECK_EXCEPTION(jni) << "error during CallBooleanMethod";
      jobject entry = jni->CallObjectMethod(j_iterator, j_next);
      CHECK_EXCEPTION(jni) << "error during CallObjectMethod";
      jmethodID get_key = GetMethodID(jni,
          GetObjectClass(jni, entry), "getKey", "()Ljava/lang/String;");
      jstring j_key = reinterpret_cast<jstring>(
          jni->CallObjectMethod(entry, get_key));
      CHECK_EXCEPTION(jni) << "error during CallObjectMethod";
      jmethodID get_value = GetMethodID(jni,
          GetObjectClass(jni, entry), "getValue", "()Ljava/lang/String;");
      jstring j_value = reinterpret_cast<jstring>(
          jni->CallObjectMethod(entry, get_value));
      CHECK_EXCEPTION(jni) << "error during CallObjectMethod";
      field->push_back(Constraint(JavaToStdString(jni, j_key),
                                  JavaToStdString(jni, j_value)));
    }
    CHECK_EXCEPTION(jni) << "error during CallBooleanMethod";
  }

  Constraints mandatory_;
  Constraints optional_;
};

static jobject JavaSdpFromNativeSdp(
    JNIEnv* jni, const SessionDescriptionInterface* desc) {
  std::string sdp;
  RTC_CHECK(desc->ToString(&sdp)) << "got so far: " << sdp;
  jstring j_description = JavaStringFromStdString(jni, sdp);

  jclass j_type_class = FindClass(
      jni, "org/webrtc/SessionDescription$Type");
  jmethodID j_type_from_canonical = GetStaticMethodID(
      jni, j_type_class, "fromCanonicalForm",
      "(Ljava/lang/String;)Lorg/webrtc/SessionDescription$Type;");
  jstring j_type_string = JavaStringFromStdString(jni, desc->type());
  jobject j_type = jni->CallStaticObjectMethod(
      j_type_class, j_type_from_canonical, j_type_string);
  CHECK_EXCEPTION(jni) << "error during CallObjectMethod";

  jclass j_sdp_class = FindClass(jni, "org/webrtc/SessionDescription");
  jmethodID j_sdp_ctor = GetMethodID(
      jni, j_sdp_class, "<init>",
      "(Lorg/webrtc/SessionDescription$Type;Ljava/lang/String;)V");
  jobject j_sdp = jni->NewObject(
      j_sdp_class, j_sdp_ctor, j_type, j_description);
  CHECK_EXCEPTION(jni) << "error during NewObject";
  return j_sdp;
}

template <class T>  // T is one of {Create,Set}SessionDescriptionObserver.
class SdpObserverWrapper : public T {
 public:
  SdpObserverWrapper(JNIEnv* jni, jobject j_observer,
                     ConstraintsWrapper* constraints)
      : constraints_(constraints),
        j_observer_global_(jni, j_observer),
        j_observer_class_(jni, GetObjectClass(jni, j_observer)) {
  }

  virtual ~SdpObserverWrapper() {}

  // Can't mark override because of templating.
  virtual void OnSuccess() {
    ScopedLocalRefFrame local_ref_frame(jni());
    jmethodID m = GetMethodID(jni(), *j_observer_class_, "onSetSuccess", "()V");
    jni()->CallVoidMethod(*j_observer_global_, m);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  // Can't mark override because of templating.
  virtual void OnSuccess(SessionDescriptionInterface* desc) {
    ScopedLocalRefFrame local_ref_frame(jni());
    jmethodID m = GetMethodID(
        jni(), *j_observer_class_, "onCreateSuccess",
        "(Lorg/webrtc/SessionDescription;)V");
    jobject j_sdp = JavaSdpFromNativeSdp(jni(), desc);
    jni()->CallVoidMethod(*j_observer_global_, m, j_sdp);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

 protected:
  // Common implementation for failure of Set & Create types, distinguished by
  // |op| being "Set" or "Create".
  void DoOnFailure(const std::string& op, const std::string& error) {
    jmethodID m = GetMethodID(jni(), *j_observer_class_, "on" + op + "Failure",
                              "(Ljava/lang/String;)V");
    jstring j_error_string = JavaStringFromStdString(jni(), error);
    jni()->CallVoidMethod(*j_observer_global_, m, j_error_string);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  JNIEnv* jni() {
    return AttachCurrentThreadIfNeeded();
  }

 private:
  scoped_ptr<ConstraintsWrapper> constraints_;
  const ScopedGlobalRef<jobject> j_observer_global_;
  const ScopedGlobalRef<jclass> j_observer_class_;
};

class CreateSdpObserverWrapper
    : public SdpObserverWrapper<CreateSessionDescriptionObserver> {
 public:
  CreateSdpObserverWrapper(JNIEnv* jni, jobject j_observer,
                           ConstraintsWrapper* constraints)
      : SdpObserverWrapper(jni, j_observer, constraints) {}

  void OnFailure(const std::string& error) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    SdpObserverWrapper::DoOnFailure(std::string("Create"), error);
  }
};

class SetSdpObserverWrapper
    : public SdpObserverWrapper<SetSessionDescriptionObserver> {
 public:
  SetSdpObserverWrapper(JNIEnv* jni, jobject j_observer,
                        ConstraintsWrapper* constraints)
      : SdpObserverWrapper(jni, j_observer, constraints) {}

  void OnFailure(const std::string& error) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    SdpObserverWrapper::DoOnFailure(std::string("Set"), error);
  }
};

// Adapter for a Java DataChannel$Observer presenting a C++ DataChannelObserver
// and dispatching the callback from C++ back to Java.
class DataChannelObserverWrapper : public DataChannelObserver {
 public:
  DataChannelObserverWrapper(JNIEnv* jni, jobject j_observer)
      : j_observer_global_(jni, j_observer),
        j_observer_class_(jni, GetObjectClass(jni, j_observer)),
        j_buffer_class_(jni, FindClass(jni, "org/webrtc/DataChannel$Buffer")),
        j_on_buffered_amount_change_mid_(GetMethodID(
            jni, *j_observer_class_, "onBufferedAmountChange", "(J)V")),
        j_on_state_change_mid_(
            GetMethodID(jni, *j_observer_class_, "onStateChange", "()V")),
        j_on_message_mid_(GetMethodID(jni, *j_observer_class_, "onMessage",
                                      "(Lorg/webrtc/DataChannel$Buffer;)V")),
        j_buffer_ctor_(GetMethodID(jni, *j_buffer_class_, "<init>",
                                   "(Ljava/nio/ByteBuffer;Z)V")) {}

  virtual ~DataChannelObserverWrapper() {}

  void OnBufferedAmountChange(uint64_t previous_amount) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jni()->CallVoidMethod(*j_observer_global_, j_on_buffered_amount_change_mid_,
                          previous_amount);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnStateChange() override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jni()->CallVoidMethod(*j_observer_global_, j_on_state_change_mid_);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

  void OnMessage(const DataBuffer& buffer) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jobject byte_buffer = jni()->NewDirectByteBuffer(
        const_cast<char*>(buffer.data.data<char>()), buffer.data.size());
    jobject j_buffer = jni()->NewObject(*j_buffer_class_, j_buffer_ctor_,
                                        byte_buffer, buffer.binary);
    jni()->CallVoidMethod(*j_observer_global_, j_on_message_mid_, j_buffer);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

 private:
  JNIEnv* jni() {
    return AttachCurrentThreadIfNeeded();
  }

  const ScopedGlobalRef<jobject> j_observer_global_;
  const ScopedGlobalRef<jclass> j_observer_class_;
  const ScopedGlobalRef<jclass> j_buffer_class_;
  const jmethodID j_on_buffered_amount_change_mid_;
  const jmethodID j_on_state_change_mid_;
  const jmethodID j_on_message_mid_;
  const jmethodID j_buffer_ctor_;
};

// Adapter for a Java StatsObserver presenting a C++ StatsObserver and
// dispatching the callback from C++ back to Java.
class StatsObserverWrapper : public StatsObserver {
 public:
  StatsObserverWrapper(JNIEnv* jni, jobject j_observer)
      : j_observer_global_(jni, j_observer),
        j_observer_class_(jni, GetObjectClass(jni, j_observer)),
        j_stats_report_class_(jni, FindClass(jni, "org/webrtc/StatsReport")),
        j_stats_report_ctor_(GetMethodID(
            jni, *j_stats_report_class_, "<init>",
            "(Ljava/lang/String;Ljava/lang/String;D"
            "[Lorg/webrtc/StatsReport$Value;)V")),
        j_value_class_(jni, FindClass(
            jni, "org/webrtc/StatsReport$Value")),
        j_value_ctor_(GetMethodID(
            jni, *j_value_class_, "<init>",
            "(Ljava/lang/String;Ljava/lang/String;)V")) {
  }

  virtual ~StatsObserverWrapper() {}

  void OnComplete(const StatsReports& reports) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jobjectArray j_reports = ReportsToJava(jni(), reports);
    jmethodID m = GetMethodID(jni(), *j_observer_class_, "onComplete",
                              "([Lorg/webrtc/StatsReport;)V");
    jni()->CallVoidMethod(*j_observer_global_, m, j_reports);
    CHECK_EXCEPTION(jni()) << "error during CallVoidMethod";
  }

 private:
  jobjectArray ReportsToJava(
      JNIEnv* jni, const StatsReports& reports) {
    jobjectArray reports_array = jni->NewObjectArray(
        reports.size(), *j_stats_report_class_, NULL);
    int i = 0;
    for (const auto* report : reports) {
      ScopedLocalRefFrame local_ref_frame(jni);
      jstring j_id = JavaStringFromStdString(jni, report->id()->ToString());
      jstring j_type = JavaStringFromStdString(jni, report->TypeToString());
      jobjectArray j_values = ValuesToJava(jni, report->values());
      jobject j_report = jni->NewObject(*j_stats_report_class_,
                                        j_stats_report_ctor_,
                                        j_id,
                                        j_type,
                                        report->timestamp(),
                                        j_values);
      jni->SetObjectArrayElement(reports_array, i++, j_report);
    }
    return reports_array;
  }

  jobjectArray ValuesToJava(JNIEnv* jni, const StatsReport::Values& values) {
    jobjectArray j_values = jni->NewObjectArray(
        values.size(), *j_value_class_, NULL);
    int i = 0;
    for (const auto& it : values) {
      ScopedLocalRefFrame local_ref_frame(jni);
      // Should we use the '.name' enum value here instead of converting the
      // name to a string?
      jstring j_name = JavaStringFromStdString(jni, it.second->display_name());
      jstring j_value = JavaStringFromStdString(jni, it.second->ToString());
      jobject j_element_value =
          jni->NewObject(*j_value_class_, j_value_ctor_, j_name, j_value);
      jni->SetObjectArrayElement(j_values, i++, j_element_value);
    }
    return j_values;
  }

  JNIEnv* jni() {
    return AttachCurrentThreadIfNeeded();
  }

  const ScopedGlobalRef<jobject> j_observer_global_;
  const ScopedGlobalRef<jclass> j_observer_class_;
  const ScopedGlobalRef<jclass> j_stats_report_class_;
  const jmethodID j_stats_report_ctor_;
  const ScopedGlobalRef<jclass> j_value_class_;
  const jmethodID j_value_ctor_;
};

// Adapter presenting a cricket::VideoRenderer as a
// webrtc::VideoRendererInterface.
class VideoRendererWrapper : public VideoRendererInterface {
 public:
  static VideoRendererWrapper* Create(cricket::VideoRenderer* renderer) {
    if (renderer)
      return new VideoRendererWrapper(renderer);
    return NULL;
  }

  virtual ~VideoRendererWrapper() {}

  // This wraps VideoRenderer which still has SetSize.
  void RenderFrame(const cricket::VideoFrame* video_frame) override {
    ScopedLocalRefFrame local_ref_frame(AttachCurrentThreadIfNeeded());
    const cricket::VideoFrame* frame =
      video_frame->GetCopyWithRotationApplied();
    if (width_ != frame->GetWidth() || height_ != frame->GetHeight()) {
      width_ = frame->GetWidth();
      height_ = frame->GetHeight();
      renderer_->SetSize(width_, height_, 0);
    }
    renderer_->RenderFrame(frame);
  }

 private:
  explicit VideoRendererWrapper(cricket::VideoRenderer* renderer)
    : width_(0), height_(0), renderer_(renderer) {}
  int width_, height_;
  scoped_ptr<cricket::VideoRenderer> renderer_;
};

// Wrapper dispatching webrtc::VideoRendererInterface to a Java VideoRenderer
// instance.
class JavaVideoRendererWrapper : public VideoRendererInterface {
 public:
  JavaVideoRendererWrapper(JNIEnv* jni, jobject j_callbacks)
      : j_callbacks_(jni, j_callbacks),
        j_render_frame_id_(GetMethodID(
            jni, GetObjectClass(jni, j_callbacks), "renderFrame",
            "(Lorg/webrtc/VideoRenderer$I420Frame;)V")),
        j_frame_class_(jni,
                       FindClass(jni, "org/webrtc/VideoRenderer$I420Frame")),
        j_i420_frame_ctor_id_(GetMethodID(
            jni, *j_frame_class_, "<init>", "(III[I[Ljava/nio/ByteBuffer;J)V")),
        j_texture_frame_ctor_id_(GetMethodID(
            jni, *j_frame_class_, "<init>",
            "(IIII[FJ)V")),
        j_byte_buffer_class_(jni, FindClass(jni, "java/nio/ByteBuffer")) {
    CHECK_EXCEPTION(jni);
  }

  virtual ~JavaVideoRendererWrapper() {}

  void RenderFrame(const cricket::VideoFrame* video_frame) override {
    ScopedLocalRefFrame local_ref_frame(jni());
    jobject j_frame = (video_frame->GetNativeHandle() != nullptr)
                          ? CricketToJavaTextureFrame(video_frame)
                          : CricketToJavaI420Frame(video_frame);
    // |j_callbacks_| is responsible for releasing |j_frame| with
    // VideoRenderer.renderFrameDone().
    jni()->CallVoidMethod(*j_callbacks_, j_render_frame_id_, j_frame);
    CHECK_EXCEPTION(jni());
  }

 private:
  // Make a shallow copy of |frame| to be used with Java. The callee has
  // ownership of the frame, and the frame should be released with
  // VideoRenderer.releaseNativeFrame().
  static jlong javaShallowCopy(const cricket::VideoFrame* frame) {
    return jlongFromPointer(frame->Copy());
  }

  // Return a VideoRenderer.I420Frame referring to the data in |frame|.
  jobject CricketToJavaI420Frame(const cricket::VideoFrame* frame) {
    jintArray strides = jni()->NewIntArray(3);
    jint* strides_array = jni()->GetIntArrayElements(strides, NULL);
    strides_array[0] = frame->GetYPitch();
    strides_array[1] = frame->GetUPitch();
    strides_array[2] = frame->GetVPitch();
    jni()->ReleaseIntArrayElements(strides, strides_array, 0);
    jobjectArray planes = jni()->NewObjectArray(3, *j_byte_buffer_class_, NULL);
    jobject y_buffer =
        jni()->NewDirectByteBuffer(const_cast<uint8_t*>(frame->GetYPlane()),
                                   frame->GetYPitch() * frame->GetHeight());
    jobject u_buffer = jni()->NewDirectByteBuffer(
        const_cast<uint8_t*>(frame->GetUPlane()), frame->GetChromaSize());
    jobject v_buffer = jni()->NewDirectByteBuffer(
        const_cast<uint8_t*>(frame->GetVPlane()), frame->GetChromaSize());
    jni()->SetObjectArrayElement(planes, 0, y_buffer);
    jni()->SetObjectArrayElement(planes, 1, u_buffer);
    jni()->SetObjectArrayElement(planes, 2, v_buffer);
    return jni()->NewObject(
        *j_frame_class_, j_i420_frame_ctor_id_,
        frame->GetWidth(), frame->GetHeight(),
        static_cast<int>(frame->GetVideoRotation()),
        strides, planes, javaShallowCopy(frame));
  }

  // Return a VideoRenderer.I420Frame referring texture object in |frame|.
  jobject CricketToJavaTextureFrame(const cricket::VideoFrame* frame) {
    NativeHandleImpl* handle =
        reinterpret_cast<NativeHandleImpl*>(frame->GetNativeHandle());
    jfloatArray sampling_matrix = jni()->NewFloatArray(16);
    jni()->SetFloatArrayRegion(sampling_matrix, 0, 16, handle->sampling_matrix);
    return jni()->NewObject(
        *j_frame_class_, j_texture_frame_ctor_id_,
        frame->GetWidth(), frame->GetHeight(),
        static_cast<int>(frame->GetVideoRotation()),
        handle->oes_texture_id, sampling_matrix, javaShallowCopy(frame));
  }

  JNIEnv* jni() {
    return AttachCurrentThreadIfNeeded();
  }

  ScopedGlobalRef<jobject> j_callbacks_;
  jmethodID j_render_frame_id_;
  ScopedGlobalRef<jclass> j_frame_class_;
  jmethodID j_i420_frame_ctor_id_;
  jmethodID j_texture_frame_ctor_id_;
  ScopedGlobalRef<jclass> j_byte_buffer_class_;
};


static DataChannelInterface* ExtractNativeDC(JNIEnv* jni, jobject j_dc) {
  jfieldID native_dc_id = GetFieldID(jni,
      GetObjectClass(jni, j_dc), "nativeDataChannel", "J");
  jlong j_d = GetLongField(jni, j_dc, native_dc_id);
  return reinterpret_cast<DataChannelInterface*>(j_d);
}

JOW(jlong, DataChannel_registerObserverNative)(
    JNIEnv* jni, jobject j_dc, jobject j_observer) {
  scoped_ptr<DataChannelObserverWrapper> observer(
      new DataChannelObserverWrapper(jni, j_observer));
  ExtractNativeDC(jni, j_dc)->RegisterObserver(observer.get());
  return jlongFromPointer(observer.release());
}

JOW(void, DataChannel_unregisterObserverNative)(
    JNIEnv* jni, jobject j_dc, jlong native_observer) {
  ExtractNativeDC(jni, j_dc)->UnregisterObserver();
  delete reinterpret_cast<DataChannelObserverWrapper*>(native_observer);
}

JOW(jstring, DataChannel_label)(JNIEnv* jni, jobject j_dc) {
  return JavaStringFromStdString(jni, ExtractNativeDC(jni, j_dc)->label());
}

JOW(jobject, DataChannel_state)(JNIEnv* jni, jobject j_dc) {
  return JavaEnumFromIndex(
      jni, "DataChannel$State", ExtractNativeDC(jni, j_dc)->state());
}

JOW(jlong, DataChannel_bufferedAmount)(JNIEnv* jni, jobject j_dc) {
  uint64_t buffered_amount = ExtractNativeDC(jni, j_dc)->buffered_amount();
  RTC_CHECK_LE(buffered_amount, std::numeric_limits<int64_t>::max())
      << "buffered_amount overflowed jlong!";
  return static_cast<jlong>(buffered_amount);
}

JOW(void, DataChannel_close)(JNIEnv* jni, jobject j_dc) {
  ExtractNativeDC(jni, j_dc)->Close();
}

JOW(jboolean, DataChannel_sendNative)(JNIEnv* jni, jobject j_dc,
                                      jbyteArray data, jboolean binary) {
  jbyte* bytes = jni->GetByteArrayElements(data, NULL);
  bool ret = ExtractNativeDC(jni, j_dc)->Send(DataBuffer(
      rtc::Buffer(bytes, jni->GetArrayLength(data)),
      binary));
  jni->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
  return ret;
}

JOW(void, DataChannel_dispose)(JNIEnv* jni, jobject j_dc) {
  CHECK_RELEASE(ExtractNativeDC(jni, j_dc));
}

JOW(void, Logging_nativeEnableTracing)(
    JNIEnv* jni, jclass, jstring j_path, jint nativeLevels,
    jint nativeSeverity) {
  std::string path = JavaToStdString(jni, j_path);
  if (nativeLevels != webrtc::kTraceNone) {
    webrtc::Trace::set_level_filter(nativeLevels);
#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
    if (path != "logcat:") {
#endif
      RTC_CHECK_EQ(0, webrtc::Trace::SetTraceFile(path.c_str(), false))
          << "SetTraceFile failed";
#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
    } else {
      // Intentionally leak this to avoid needing to reason about its lifecycle.
      // It keeps no state and functions only as a dispatch point.
      static LogcatTraceContext* g_trace_callback = new LogcatTraceContext();
    }
#endif
  }
  if (nativeSeverity >= rtc::LS_SENSITIVE && nativeSeverity <= rtc::LS_ERROR) {
    rtc::LogMessage::LogToDebug(
        static_cast<rtc::LoggingSeverity>(nativeSeverity));
  }
}

JOW(void, Logging_nativeEnableLogThreads)(JNIEnv* jni, jclass) {
  rtc::LogMessage::LogThreads(true);
}

JOW(void, Logging_nativeEnableLogTimeStamps)(JNIEnv* jni, jclass) {
  rtc::LogMessage::LogTimestamps(true);
}

JOW(void, Logging_nativeLog)(
    JNIEnv* jni, jclass, jint j_severity, jstring j_tag, jstring j_message) {
  std::string message = JavaToStdString(jni, j_message);
  std::string tag = JavaToStdString(jni, j_tag);
  LOG_TAG(static_cast<rtc::LoggingSeverity>(j_severity), tag) << message;
}

JOW(void, PeerConnection_freePeerConnection)(JNIEnv*, jclass, jlong j_p) {
  CHECK_RELEASE(reinterpret_cast<PeerConnectionInterface*>(j_p));
}

JOW(void, PeerConnection_freeObserver)(JNIEnv*, jclass, jlong j_p) {
  PCOJava* p = reinterpret_cast<PCOJava*>(j_p);
  delete p;
}

JOW(void, MediaSource_free)(JNIEnv*, jclass, jlong j_p) {
  CHECK_RELEASE(reinterpret_cast<MediaSourceInterface*>(j_p));
}

JOW(void, VideoCapturer_free)(JNIEnv*, jclass, jlong j_p) {
  delete reinterpret_cast<cricket::VideoCapturer*>(j_p);
}

JOW(void, VideoRenderer_freeGuiVideoRenderer)(JNIEnv*, jclass, jlong j_p) {
  delete reinterpret_cast<VideoRendererWrapper*>(j_p);
}

JOW(void, VideoRenderer_freeWrappedVideoRenderer)(JNIEnv*, jclass, jlong j_p) {
  delete reinterpret_cast<JavaVideoRendererWrapper*>(j_p);
}

JOW(void, VideoRenderer_releaseNativeFrame)(
    JNIEnv* jni, jclass, jlong j_frame_ptr) {
  delete reinterpret_cast<const cricket::VideoFrame*>(j_frame_ptr);
}

JOW(void, MediaStreamTrack_free)(JNIEnv*, jclass, jlong j_p) {
  reinterpret_cast<MediaStreamTrackInterface*>(j_p)->Release();
}

JOW(jboolean, MediaStream_nativeAddAudioTrack)(
    JNIEnv* jni, jclass, jlong pointer, jlong j_audio_track_pointer) {
  return reinterpret_cast<MediaStreamInterface*>(pointer)->AddTrack(
      reinterpret_cast<AudioTrackInterface*>(j_audio_track_pointer));
}

JOW(jboolean, MediaStream_nativeAddVideoTrack)(
    JNIEnv* jni, jclass, jlong pointer, jlong j_video_track_pointer) {
  return reinterpret_cast<MediaStreamInterface*>(pointer)
      ->AddTrack(reinterpret_cast<VideoTrackInterface*>(j_video_track_pointer));
}

JOW(jboolean, MediaStream_nativeRemoveAudioTrack)(
    JNIEnv* jni, jclass, jlong pointer, jlong j_audio_track_pointer) {
  return reinterpret_cast<MediaStreamInterface*>(pointer)->RemoveTrack(
      reinterpret_cast<AudioTrackInterface*>(j_audio_track_pointer));
}

JOW(jboolean, MediaStream_nativeRemoveVideoTrack)(
    JNIEnv* jni, jclass, jlong pointer, jlong j_video_track_pointer) {
  return reinterpret_cast<MediaStreamInterface*>(pointer)->RemoveTrack(
      reinterpret_cast<VideoTrackInterface*>(j_video_track_pointer));
}

JOW(jstring, MediaStream_nativeLabel)(JNIEnv* jni, jclass, jlong j_p) {
  return JavaStringFromStdString(
      jni, reinterpret_cast<MediaStreamInterface*>(j_p)->label());
}

JOW(void, MediaStream_free)(JNIEnv*, jclass, jlong j_p) {
  CHECK_RELEASE(reinterpret_cast<MediaStreamInterface*>(j_p));
}

JOW(jlong, PeerConnectionFactory_nativeCreateObserver)(
    JNIEnv * jni, jclass, jobject j_observer) {
  return (jlong)new PCOJava(jni, j_observer);
}

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
JOW(jboolean, PeerConnectionFactory_initializeAndroidGlobals)(
    JNIEnv* jni, jclass, jobject context,
    jboolean initialize_audio, jboolean initialize_video,
    jboolean video_hw_acceleration) {
  bool failure = false;
  video_hw_acceleration_enabled = video_hw_acceleration;
  AndroidNetworkMonitor::SetAndroidContext(jni, context);
  if (!factory_static_initialized) {
    if (initialize_video) {
      failure |= webrtc::SetRenderAndroidVM(GetJVM());
      failure |= AndroidVideoCapturerJni::SetAndroidObjects(jni, context);
    }
    if (initialize_audio)
      failure |= webrtc::VoiceEngine::SetAndroidObjects(GetJVM(), context);
    factory_static_initialized = true;
  }
  return !failure;
}
#endif  // defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)

JOW(void, PeerConnectionFactory_initializeFieldTrials)(
    JNIEnv* jni, jclass, jstring j_trials_init_string) {
  field_trials_init_string = NULL;
  if (j_trials_init_string != NULL) {
    const char* init_string =
        jni->GetStringUTFChars(j_trials_init_string, NULL);
    int init_string_length = jni->GetStringUTFLength(j_trials_init_string);
    field_trials_init_string = new char[init_string_length + 1];
    rtc::strcpyn(field_trials_init_string, init_string_length + 1, init_string);
    jni->ReleaseStringUTFChars(j_trials_init_string, init_string);
    LOG(LS_INFO) << "initializeFieldTrials: " << field_trials_init_string;
  }
  webrtc::field_trial::InitFieldTrialsFromString(field_trials_init_string);
}

JOW(void, PeerConnectionFactory_initializeInternalTracer)(JNIEnv* jni, jclass) {
  rtc::tracing::SetupInternalTracer();
}

JOW(jboolean, PeerConnectionFactory_startInternalTracingCapture)(
    JNIEnv* jni, jclass, jstring j_event_tracing_filename) {
  if (!j_event_tracing_filename)
    return false;

  const char* init_string =
      jni->GetStringUTFChars(j_event_tracing_filename, NULL);
  LOG(LS_INFO) << "Starting internal tracing to: " << init_string;
  bool ret = rtc::tracing::StartInternalCapture(init_string);
  jni->ReleaseStringUTFChars(j_event_tracing_filename, init_string);
  return ret;
}

JOW(void, PeerConnectionFactory_stopInternalTracingCapture)(
    JNIEnv* jni, jclass) {
  rtc::tracing::StopInternalCapture();
}

JOW(void, PeerConnectionFactory_shutdownInternalTracer)(JNIEnv* jni, jclass) {
  rtc::tracing::ShutdownInternalTracer();
}

// Helper struct for working around the fact that CreatePeerConnectionFactory()
// comes in two flavors: either entirely automagical (constructing its own
// threads and deleting them on teardown, but no external codec factory support)
// or entirely manual (requires caller to delete threads after factory
// teardown).  This struct takes ownership of its ctor's arguments to present a
// single thing for Java to hold and eventually free.
class OwnedFactoryAndThreads {
 public:
  OwnedFactoryAndThreads(Thread* worker_thread,
                         Thread* signaling_thread,
                         WebRtcVideoEncoderFactory* encoder_factory,
                         WebRtcVideoDecoderFactory* decoder_factory,
                         rtc::NetworkMonitorFactory* network_monitor_factory,
                         PeerConnectionFactoryInterface* factory)
      : worker_thread_(worker_thread),
        signaling_thread_(signaling_thread),
        encoder_factory_(encoder_factory),
        decoder_factory_(decoder_factory),
        network_monitor_factory_(network_monitor_factory),
        factory_(factory) {}

  ~OwnedFactoryAndThreads() {
    CHECK_RELEASE(factory_);
    if (network_monitor_factory_ != nullptr) {
      rtc::NetworkMonitorFactory::ReleaseFactory(network_monitor_factory_);
    }
  }

  PeerConnectionFactoryInterface* factory() { return factory_; }
  WebRtcVideoEncoderFactory* encoder_factory() { return encoder_factory_; }
  WebRtcVideoDecoderFactory* decoder_factory() { return decoder_factory_; }
  rtc::NetworkMonitorFactory* network_monitor_factory() {
    return network_monitor_factory_;
  }
  void clear_network_monitor_factory() { network_monitor_factory_ = nullptr; }
  void InvokeJavaCallbacksOnFactoryThreads();

 private:
  void JavaCallbackOnFactoryThreads();

  const scoped_ptr<Thread> worker_thread_;
  const scoped_ptr<Thread> signaling_thread_;
  WebRtcVideoEncoderFactory* encoder_factory_;
  WebRtcVideoDecoderFactory* decoder_factory_;
  rtc::NetworkMonitorFactory* network_monitor_factory_;
  PeerConnectionFactoryInterface* factory_;  // Const after ctor except dtor.
};

void OwnedFactoryAndThreads::JavaCallbackOnFactoryThreads() {
  JNIEnv* jni = AttachCurrentThreadIfNeeded();
  ScopedLocalRefFrame local_ref_frame(jni);
  jclass j_factory_class = FindClass(jni, "org/webrtc/PeerConnectionFactory");
  jmethodID m = nullptr;
  if (Thread::Current() == worker_thread_) {
    LOG(LS_INFO) << "Worker thread JavaCallback";
    m = GetStaticMethodID(jni, j_factory_class, "onWorkerThreadReady", "()V");
  }
  if (Thread::Current() == signaling_thread_) {
    LOG(LS_INFO) << "Signaling thread JavaCallback";
    m = GetStaticMethodID(
        jni, j_factory_class, "onSignalingThreadReady", "()V");
  }
  if (m != nullptr) {
    jni->CallStaticVoidMethod(j_factory_class, m);
    CHECK_EXCEPTION(jni) << "error during JavaCallback::CallStaticVoidMethod";
  }
}

void OwnedFactoryAndThreads::InvokeJavaCallbacksOnFactoryThreads() {
  LOG(LS_INFO) << "InvokeJavaCallbacksOnFactoryThreads.";
  worker_thread_->Invoke<void>(
      Bind(&OwnedFactoryAndThreads::JavaCallbackOnFactoryThreads, this));
  signaling_thread_->Invoke<void>(
      Bind(&OwnedFactoryAndThreads::JavaCallbackOnFactoryThreads, this));
}

JOW(jlong, PeerConnectionFactory_nativeCreatePeerConnectionFactory)(
    JNIEnv* jni, jclass) {
  // talk/ assumes pretty widely that the current Thread is ThreadManager'd, but
  // ThreadManager only WrapCurrentThread()s the thread where it is first
  // created.  Since the semantics around when auto-wrapping happens in
  // webrtc/base/ are convoluted, we simply wrap here to avoid having to think
  // about ramifications of auto-wrapping there.
  rtc::ThreadManager::Instance()->WrapCurrentThread();
  webrtc::Trace::CreateTrace();
  Thread* worker_thread = new Thread();
  worker_thread->SetName("worker_thread", NULL);
  Thread* signaling_thread = new Thread();
  signaling_thread->SetName("signaling_thread", NULL);
  RTC_CHECK(worker_thread->Start() && signaling_thread->Start())
      << "Failed to start threads";
  WebRtcVideoEncoderFactory* encoder_factory = nullptr;
  WebRtcVideoDecoderFactory* decoder_factory = nullptr;
  rtc::NetworkMonitorFactory* network_monitor_factory = nullptr;

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
  if (video_hw_acceleration_enabled) {
    encoder_factory = new MediaCodecVideoEncoderFactory();
    decoder_factory = new MediaCodecVideoDecoderFactory();
  }
  network_monitor_factory = new AndroidNetworkMonitorFactory();
  rtc::NetworkMonitorFactory::SetFactory(network_monitor_factory);
#endif
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      webrtc::CreatePeerConnectionFactory(worker_thread,
                                          signaling_thread,
                                          NULL,
                                          encoder_factory,
                                          decoder_factory));
  RTC_CHECK(factory) << "Failed to create the peer connection factory; "
                     << "WebRTC/libjingle init likely failed on this device";
  OwnedFactoryAndThreads* owned_factory = new OwnedFactoryAndThreads(
      worker_thread, signaling_thread,
      encoder_factory, decoder_factory,
      network_monitor_factory, factory.release());
  owned_factory->InvokeJavaCallbacksOnFactoryThreads();
  return jlongFromPointer(owned_factory);
}

JOW(void, PeerConnectionFactory_nativeFreeFactory)(JNIEnv*, jclass, jlong j_p) {
  delete reinterpret_cast<OwnedFactoryAndThreads*>(j_p);
  if (field_trials_init_string) {
    webrtc::field_trial::InitFieldTrialsFromString(NULL);
    delete field_trials_init_string;
    field_trials_init_string = NULL;
  }
  webrtc::Trace::ReturnTrace();
}

static PeerConnectionFactoryInterface* factoryFromJava(jlong j_p) {
  return reinterpret_cast<OwnedFactoryAndThreads*>(j_p)->factory();
}

JOW(void, PeerConnectionFactory_nativeThreadsCallbacks)(
    JNIEnv*, jclass, jlong j_p) {
  OwnedFactoryAndThreads *factory =
      reinterpret_cast<OwnedFactoryAndThreads*>(j_p);
  factory->InvokeJavaCallbacksOnFactoryThreads();
}

JOW(jlong, PeerConnectionFactory_nativeCreateLocalMediaStream)(
    JNIEnv* jni, jclass, jlong native_factory, jstring label) {
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  rtc::scoped_refptr<MediaStreamInterface> stream(
      factory->CreateLocalMediaStream(JavaToStdString(jni, label)));
  return (jlong)stream.release();
}

JOW(jlong, PeerConnectionFactory_nativeCreateVideoSource)(
    JNIEnv* jni, jclass, jlong native_factory, jlong native_capturer,
    jobject j_constraints) {
  scoped_ptr<ConstraintsWrapper> constraints(
      new ConstraintsWrapper(jni, j_constraints));
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  rtc::scoped_refptr<VideoSourceInterface> source(
      factory->CreateVideoSource(
          reinterpret_cast<cricket::VideoCapturer*>(native_capturer),
          constraints.get()));
  return (jlong)source.release();
}

JOW(jlong, PeerConnectionFactory_nativeCreateVideoTrack)(
    JNIEnv* jni, jclass, jlong native_factory, jstring id,
    jlong native_source) {
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  rtc::scoped_refptr<VideoTrackInterface> track(
      factory->CreateVideoTrack(
          JavaToStdString(jni, id),
          reinterpret_cast<VideoSourceInterface*>(native_source)));
  return (jlong)track.release();
}

JOW(jlong, PeerConnectionFactory_nativeCreateAudioSource)(
    JNIEnv* jni, jclass, jlong native_factory, jobject j_constraints) {
  scoped_ptr<ConstraintsWrapper> constraints(
      new ConstraintsWrapper(jni, j_constraints));
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  rtc::scoped_refptr<AudioSourceInterface> source(
      factory->CreateAudioSource(constraints.get()));
  return (jlong)source.release();
}

JOW(jlong, PeerConnectionFactory_nativeCreateAudioTrack)(
    JNIEnv* jni, jclass, jlong native_factory, jstring id,
    jlong native_source) {
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  rtc::scoped_refptr<AudioTrackInterface> track(factory->CreateAudioTrack(
      JavaToStdString(jni, id),
      reinterpret_cast<AudioSourceInterface*>(native_source)));
  return (jlong)track.release();
}

JOW(jboolean, PeerConnectionFactory_nativeStartAecDump)(
    JNIEnv* jni, jclass, jlong native_factory, jint file) {
#if defined(ANDROID)
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  return factory->StartAecDump(file);
#else
  return false;
#endif
}

JOW(void, PeerConnectionFactory_nativeStopAecDump)(
    JNIEnv* jni, jclass, jlong native_factory) {
#if defined(ANDROID)
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  factory->StopAecDump();
#endif
}

JOW(jboolean, PeerConnectionFactory_nativeStartRtcEventLog)(
    JNIEnv* jni, jclass, jlong native_factory, jint file) {
#if defined(ANDROID)
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  return factory->StartRtcEventLog(file);
#else
  return false;
#endif
}

JOW(void, PeerConnectionFactory_nativeStopRtcEventLog)(
    JNIEnv* jni, jclass, jlong native_factory) {
#if defined(ANDROID)
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  factory->StopRtcEventLog();
#endif
}

JOW(void, PeerConnectionFactory_nativeSetOptions)(
    JNIEnv* jni, jclass, jlong native_factory, jobject options) {
  rtc::scoped_refptr<PeerConnectionFactoryInterface> factory(
      factoryFromJava(native_factory));
  jclass options_class = jni->GetObjectClass(options);
  jfieldID network_ignore_mask_field =
      jni->GetFieldID(options_class, "networkIgnoreMask", "I");
  int network_ignore_mask =
      jni->GetIntField(options, network_ignore_mask_field);

  jfieldID disable_encryption_field =
      jni->GetFieldID(options_class, "disableEncryption", "Z");
  bool disable_encryption =
      jni->GetBooleanField(options, disable_encryption_field);

  jfieldID disable_network_monitor_field =
      jni->GetFieldID(options_class, "disableNetworkMonitor", "Z");
  bool disable_network_monitor =
      jni->GetBooleanField(options, disable_network_monitor_field);

  PeerConnectionFactoryInterface::Options options_to_set;

  // This doesn't necessarily match the c++ version of this struct; feel free
  // to add more parameters as necessary.
  options_to_set.network_ignore_mask = network_ignore_mask;
  options_to_set.disable_encryption = disable_encryption;
  options_to_set.disable_network_monitor = disable_network_monitor;
  factory->SetOptions(options_to_set);

  if (disable_network_monitor) {
    OwnedFactoryAndThreads* owner =
        reinterpret_cast<OwnedFactoryAndThreads*>(native_factory);
    if (owner->network_monitor_factory()) {
      rtc::NetworkMonitorFactory::ReleaseFactory(
          owner->network_monitor_factory());
      owner->clear_network_monitor_factory();
    }
  }
}

JOW(void, PeerConnectionFactory_nativeSetVideoHwAccelerationOptions)(
    JNIEnv* jni, jclass, jlong native_factory, jobject local_egl_context,
    jobject remote_egl_context) {
#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
  OwnedFactoryAndThreads* owned_factory =
      reinterpret_cast<OwnedFactoryAndThreads*>(native_factory);

  jclass j_eglbase14_context_class =
      FindClass(jni, "org/webrtc/EglBase14$Context");

  MediaCodecVideoEncoderFactory* encoder_factory =
      static_cast<MediaCodecVideoEncoderFactory*>
          (owned_factory->encoder_factory());
  if (encoder_factory &&
      jni->IsInstanceOf(local_egl_context, j_eglbase14_context_class)) {
    LOG(LS_INFO) << "Set EGL context for HW encoding.";
    encoder_factory->SetEGLContext(jni, local_egl_context);
  }

  MediaCodecVideoDecoderFactory* decoder_factory =
      static_cast<MediaCodecVideoDecoderFactory*>
          (owned_factory->decoder_factory());
  if (decoder_factory &&
      jni->IsInstanceOf(remote_egl_context, j_eglbase14_context_class)) {
    LOG(LS_INFO) << "Set EGL context for HW decoding.";
    decoder_factory->SetEGLContext(jni, remote_egl_context);
  }
#endif
}

static std::string
GetJavaEnumName(JNIEnv* jni, const std::string& className, jobject j_enum) {
  jclass enumClass = FindClass(jni, className.c_str());
  jmethodID nameMethod =
      GetMethodID(jni, enumClass, "name", "()Ljava/lang/String;");
  jstring name =
      reinterpret_cast<jstring>(jni->CallObjectMethod(j_enum, nameMethod));
  CHECK_EXCEPTION(jni) << "error during CallObjectMethod for "
                       << className << ".name";
  return JavaToStdString(jni, name);
}

static PeerConnectionInterface::IceTransportsType
JavaIceTransportsTypeToNativeType(JNIEnv* jni, jobject j_ice_transports_type) {
  std::string enum_name = GetJavaEnumName(
      jni, "org/webrtc/PeerConnection$IceTransportsType",
      j_ice_transports_type);

  if (enum_name == "ALL")
    return PeerConnectionInterface::kAll;

  if (enum_name == "RELAY")
    return PeerConnectionInterface::kRelay;

  if (enum_name == "NOHOST")
    return PeerConnectionInterface::kNoHost;

  if (enum_name == "NONE")
    return PeerConnectionInterface::kNone;

  RTC_CHECK(false) << "Unexpected IceTransportsType enum_name " << enum_name;
  return PeerConnectionInterface::kAll;
}

static PeerConnectionInterface::BundlePolicy
JavaBundlePolicyToNativeType(JNIEnv* jni, jobject j_bundle_policy) {
  std::string enum_name = GetJavaEnumName(
      jni, "org/webrtc/PeerConnection$BundlePolicy",
      j_bundle_policy);

  if (enum_name == "BALANCED")
    return PeerConnectionInterface::kBundlePolicyBalanced;

  if (enum_name == "MAXBUNDLE")
    return PeerConnectionInterface::kBundlePolicyMaxBundle;

  if (enum_name == "MAXCOMPAT")
    return PeerConnectionInterface::kBundlePolicyMaxCompat;

  RTC_CHECK(false) << "Unexpected BundlePolicy enum_name " << enum_name;
  return PeerConnectionInterface::kBundlePolicyBalanced;
}

static PeerConnectionInterface::RtcpMuxPolicy
JavaRtcpMuxPolicyToNativeType(JNIEnv* jni, jobject j_rtcp_mux_policy) {
  std::string enum_name = GetJavaEnumName(
      jni, "org/webrtc/PeerConnection$RtcpMuxPolicy",
      j_rtcp_mux_policy);

  if (enum_name == "NEGOTIATE")
    return PeerConnectionInterface::kRtcpMuxPolicyNegotiate;

  if (enum_name == "REQUIRE")
    return PeerConnectionInterface::kRtcpMuxPolicyRequire;

  RTC_CHECK(false) << "Unexpected RtcpMuxPolicy enum_name " << enum_name;
  return PeerConnectionInterface::kRtcpMuxPolicyNegotiate;
}

static PeerConnectionInterface::TcpCandidatePolicy
JavaTcpCandidatePolicyToNativeType(
    JNIEnv* jni, jobject j_tcp_candidate_policy) {
  std::string enum_name = GetJavaEnumName(
      jni, "org/webrtc/PeerConnection$TcpCandidatePolicy",
      j_tcp_candidate_policy);

  if (enum_name == "ENABLED")
    return PeerConnectionInterface::kTcpCandidatePolicyEnabled;

  if (enum_name == "DISABLED")
    return PeerConnectionInterface::kTcpCandidatePolicyDisabled;

  RTC_CHECK(false) << "Unexpected TcpCandidatePolicy enum_name " << enum_name;
  return PeerConnectionInterface::kTcpCandidatePolicyEnabled;
}

static rtc::KeyType JavaKeyTypeToNativeType(JNIEnv* jni, jobject j_key_type) {
  std::string enum_name = GetJavaEnumName(
      jni, "org/webrtc/PeerConnection$KeyType", j_key_type);

  if (enum_name == "RSA")
    return rtc::KT_RSA;
  if (enum_name == "ECDSA")
    return rtc::KT_ECDSA;

  RTC_CHECK(false) << "Unexpected KeyType enum_name " << enum_name;
  return rtc::KT_ECDSA;
}

static PeerConnectionInterface::ContinualGatheringPolicy
    JavaContinualGatheringPolicyToNativeType(
        JNIEnv* jni, jobject j_gathering_policy) {
  std::string enum_name = GetJavaEnumName(
      jni, "org/webrtc/PeerConnection$ContinualGatheringPolicy",
      j_gathering_policy);
  if (enum_name == "GATHER_ONCE")
    return PeerConnectionInterface::GATHER_ONCE;

  if (enum_name == "GATHER_CONTINUALLY")
    return PeerConnectionInterface::GATHER_CONTINUALLY;

  RTC_CHECK(false) << "Unexpected ContinualGatheringPolicy enum name "
                   << enum_name;
  return PeerConnectionInterface::GATHER_ONCE;
}

static void JavaIceServersToJsepIceServers(
    JNIEnv* jni, jobject j_ice_servers,
    PeerConnectionInterface::IceServers* ice_servers) {
  jclass list_class = GetObjectClass(jni, j_ice_servers);
  jmethodID iterator_id = GetMethodID(
      jni, list_class, "iterator", "()Ljava/util/Iterator;");
  jobject iterator = jni->CallObjectMethod(j_ice_servers, iterator_id);
  CHECK_EXCEPTION(jni) << "error during CallObjectMethod";
  jmethodID iterator_has_next = GetMethodID(
      jni, GetObjectClass(jni, iterator), "hasNext", "()Z");
  jmethodID iterator_next = GetMethodID(
      jni, GetObjectClass(jni, iterator), "next", "()Ljava/lang/Object;");
  while (jni->CallBooleanMethod(iterator, iterator_has_next)) {
    CHECK_EXCEPTION(jni) << "error during CallBooleanMethod";
    jobject j_ice_server = jni->CallObjectMethod(iterator, iterator_next);
    CHECK_EXCEPTION(jni) << "error during CallObjectMethod";
    jclass j_ice_server_class = GetObjectClass(jni, j_ice_server);
    jfieldID j_ice_server_uri_id =
        GetFieldID(jni, j_ice_server_class, "uri", "Ljava/lang/String;");
    jfieldID j_ice_server_username_id =
        GetFieldID(jni, j_ice_server_class, "username", "Ljava/lang/String;");
    jfieldID j_ice_server_password_id =
        GetFieldID(jni, j_ice_server_class, "password", "Ljava/lang/String;");
    jstring uri = reinterpret_cast<jstring>(
        GetObjectField(jni, j_ice_server, j_ice_server_uri_id));
    jstring username = reinterpret_cast<jstring>(
        GetObjectField(jni, j_ice_server, j_ice_server_username_id));
    jstring password = reinterpret_cast<jstring>(
        GetObjectField(jni, j_ice_server, j_ice_server_password_id));
    PeerConnectionInterface::IceServer server;
    server.uri = JavaToStdString(jni, uri);
    server.username = JavaToStdString(jni, username);
    server.password = JavaToStdString(jni, password);
    ice_servers->push_back(server);
  }
  CHECK_EXCEPTION(jni) << "error during CallBooleanMethod";
}

static void JavaRTCConfigurationToJsepRTCConfiguration(
    JNIEnv* jni,
    jobject j_rtc_config,
    PeerConnectionInterface::RTCConfiguration* rtc_config) {
  jclass j_rtc_config_class = GetObjectClass(jni, j_rtc_config);

  jfieldID j_ice_transports_type_id = GetFieldID(
      jni, j_rtc_config_class, "iceTransportsType",
      "Lorg/webrtc/PeerConnection$IceTransportsType;");
  jobject j_ice_transports_type = GetObjectField(
      jni, j_rtc_config, j_ice_transports_type_id);

  jfieldID j_bundle_policy_id = GetFieldID(
      jni, j_rtc_config_class, "bundlePolicy",
      "Lorg/webrtc/PeerConnection$BundlePolicy;");
  jobject j_bundle_policy = GetObjectField(
      jni, j_rtc_config, j_bundle_policy_id);

  jfieldID j_rtcp_mux_policy_id = GetFieldID(
      jni, j_rtc_config_class, "rtcpMuxPolicy",
      "Lorg/webrtc/PeerConnection$RtcpMuxPolicy;");
  jobject j_rtcp_mux_policy = GetObjectField(
      jni, j_rtc_config, j_rtcp_mux_policy_id);

  jfieldID j_tcp_candidate_policy_id = GetFieldID(
      jni, j_rtc_config_class, "tcpCandidatePolicy",
      "Lorg/webrtc/PeerConnection$TcpCandidatePolicy;");
  jobject j_tcp_candidate_policy = GetObjectField(
      jni, j_rtc_config, j_tcp_candidate_policy_id);

  jfieldID j_ice_servers_id = GetFieldID(
      jni, j_rtc_config_class, "iceServers", "Ljava/util/List;");
  jobject j_ice_servers = GetObjectField(jni, j_rtc_config, j_ice_servers_id);

  jfieldID j_audio_jitter_buffer_max_packets_id =
      GetFieldID(jni, j_rtc_config_class, "audioJitterBufferMaxPackets", "I");
  jfieldID j_audio_jitter_buffer_fast_accelerate_id = GetFieldID(
      jni, j_rtc_config_class, "audioJitterBufferFastAccelerate", "Z");

  jfieldID j_ice_connection_receiving_timeout_id =
      GetFieldID(jni, j_rtc_config_class, "iceConnectionReceivingTimeout", "I");

  jfieldID j_ice_backup_candidate_pair_ping_interval_id = GetFieldID(
      jni, j_rtc_config_class, "iceBackupCandidatePairPingInterval", "I");

  jfieldID j_continual_gathering_policy_id =
      GetFieldID(jni, j_rtc_config_class, "continualGatheringPolicy",
                 "Lorg/webrtc/PeerConnection$ContinualGatheringPolicy;");
  jobject j_continual_gathering_policy =
      GetObjectField(jni, j_rtc_config, j_continual_gathering_policy_id);

  rtc_config->type =
      JavaIceTransportsTypeToNativeType(jni, j_ice_transports_type);
  rtc_config->bundle_policy =
      JavaBundlePolicyToNativeType(jni, j_bundle_policy);
  rtc_config->rtcp_mux_policy =
      JavaRtcpMuxPolicyToNativeType(jni, j_rtcp_mux_policy);
  rtc_config->tcp_candidate_policy =
      JavaTcpCandidatePolicyToNativeType(jni, j_tcp_candidate_policy);
  JavaIceServersToJsepIceServers(jni, j_ice_servers, &rtc_config->servers);
  rtc_config->audio_jitter_buffer_max_packets =
      GetIntField(jni, j_rtc_config, j_audio_jitter_buffer_max_packets_id);
  rtc_config->audio_jitter_buffer_fast_accelerate = GetBooleanField(
      jni, j_rtc_config, j_audio_jitter_buffer_fast_accelerate_id);
  rtc_config->ice_connection_receiving_timeout =
      GetIntField(jni, j_rtc_config, j_ice_connection_receiving_timeout_id);
  rtc_config->ice_backup_candidate_pair_ping_interval = GetIntField(
      jni, j_rtc_config, j_ice_backup_candidate_pair_ping_interval_id);
  rtc_config->continual_gathering_policy =
      JavaContinualGatheringPolicyToNativeType(
          jni, j_continual_gathering_policy);
}

JOW(jlong, PeerConnectionFactory_nativeCreatePeerConnection)(
    JNIEnv *jni, jclass, jlong factory, jobject j_rtc_config,
    jobject j_constraints, jlong observer_p) {
  rtc::scoped_refptr<PeerConnectionFactoryInterface> f(
      reinterpret_cast<PeerConnectionFactoryInterface*>(
          factoryFromJava(factory)));

  PeerConnectionInterface::RTCConfiguration rtc_config;
  JavaRTCConfigurationToJsepRTCConfiguration(jni, j_rtc_config, &rtc_config);

  jclass j_rtc_config_class = GetObjectClass(jni, j_rtc_config);
  jfieldID j_key_type_id = GetFieldID(jni, j_rtc_config_class, "keyType",
                                      "Lorg/webrtc/PeerConnection$KeyType;");
  jobject j_key_type = GetObjectField(jni, j_rtc_config, j_key_type_id);

  // Create ECDSA certificate.
  if (JavaKeyTypeToNativeType(jni, j_key_type) == rtc::KT_ECDSA) {
    scoped_ptr<rtc::SSLIdentity> ssl_identity(
        rtc::SSLIdentity::Generate(webrtc::kIdentityName, rtc::KT_ECDSA));
    if (ssl_identity.get()) {
      rtc_config.certificates.push_back(
          rtc::RTCCertificate::Create(std::move(ssl_identity)));
      LOG(LS_INFO) << "ECDSA certificate created.";
    } else {
      // Failing to create certificate should not abort peer connection
      // creation. Instead default encryption (currently RSA) will be used.
      LOG(LS_WARNING) <<
          "Failed to generate SSLIdentity. Default encryption will be used.";
    }
  }

  PCOJava* observer = reinterpret_cast<PCOJava*>(observer_p);
  observer->SetConstraints(new ConstraintsWrapper(jni, j_constraints));
  rtc::scoped_refptr<PeerConnectionInterface> pc(f->CreatePeerConnection(
      rtc_config, observer->constraints(), NULL, NULL, observer));
  return (jlong)pc.release();
}

static rtc::scoped_refptr<PeerConnectionInterface> ExtractNativePC(
    JNIEnv* jni, jobject j_pc) {
  jfieldID native_pc_id = GetFieldID(jni,
      GetObjectClass(jni, j_pc), "nativePeerConnection", "J");
  jlong j_p = GetLongField(jni, j_pc, native_pc_id);
  return rtc::scoped_refptr<PeerConnectionInterface>(
      reinterpret_cast<PeerConnectionInterface*>(j_p));
}

JOW(jobject, PeerConnection_getLocalDescription)(JNIEnv* jni, jobject j_pc) {
  const SessionDescriptionInterface* sdp =
      ExtractNativePC(jni, j_pc)->local_description();
  return sdp ? JavaSdpFromNativeSdp(jni, sdp) : NULL;
}

JOW(jobject, PeerConnection_getRemoteDescription)(JNIEnv* jni, jobject j_pc) {
  const SessionDescriptionInterface* sdp =
      ExtractNativePC(jni, j_pc)->remote_description();
  return sdp ? JavaSdpFromNativeSdp(jni, sdp) : NULL;
}

JOW(jobject, PeerConnection_createDataChannel)(
    JNIEnv* jni, jobject j_pc, jstring j_label, jobject j_init) {
  DataChannelInit init = JavaDataChannelInitToNative(jni, j_init);
  rtc::scoped_refptr<DataChannelInterface> channel(
      ExtractNativePC(jni, j_pc)->CreateDataChannel(
          JavaToStdString(jni, j_label), &init));
  // Mustn't pass channel.get() directly through NewObject to avoid reading its
  // vararg parameter as 64-bit and reading memory that doesn't belong to the
  // 32-bit parameter.
  jlong nativeChannelPtr = jlongFromPointer(channel.get());
  RTC_CHECK(nativeChannelPtr) << "Failed to create DataChannel";
  jclass j_data_channel_class = FindClass(jni, "org/webrtc/DataChannel");
  jmethodID j_data_channel_ctor = GetMethodID(
      jni, j_data_channel_class, "<init>", "(J)V");
  jobject j_channel = jni->NewObject(
      j_data_channel_class, j_data_channel_ctor, nativeChannelPtr);
  CHECK_EXCEPTION(jni) << "error during NewObject";
  // Channel is now owned by Java object, and will be freed from there.
  int bumped_count = channel->AddRef();
  RTC_CHECK(bumped_count == 2) << "Unexpected refcount";
  return j_channel;
}

JOW(void, PeerConnection_createOffer)(
    JNIEnv* jni, jobject j_pc, jobject j_observer, jobject j_constraints) {
  ConstraintsWrapper* constraints =
      new ConstraintsWrapper(jni, j_constraints);
  rtc::scoped_refptr<CreateSdpObserverWrapper> observer(
      new rtc::RefCountedObject<CreateSdpObserverWrapper>(
          jni, j_observer, constraints));
  ExtractNativePC(jni, j_pc)->CreateOffer(observer, constraints);
}

JOW(void, PeerConnection_createAnswer)(
    JNIEnv* jni, jobject j_pc, jobject j_observer, jobject j_constraints) {
  ConstraintsWrapper* constraints =
      new ConstraintsWrapper(jni, j_constraints);
  rtc::scoped_refptr<CreateSdpObserverWrapper> observer(
      new rtc::RefCountedObject<CreateSdpObserverWrapper>(
          jni, j_observer, constraints));
  ExtractNativePC(jni, j_pc)->CreateAnswer(observer, constraints);
}

// Helper to create a SessionDescriptionInterface from a SessionDescription.
static SessionDescriptionInterface* JavaSdpToNativeSdp(
    JNIEnv* jni, jobject j_sdp) {
  jfieldID j_type_id = GetFieldID(
      jni, GetObjectClass(jni, j_sdp), "type",
      "Lorg/webrtc/SessionDescription$Type;");
  jobject j_type = GetObjectField(jni, j_sdp, j_type_id);
  jmethodID j_canonical_form_id = GetMethodID(
      jni, GetObjectClass(jni, j_type), "canonicalForm",
      "()Ljava/lang/String;");
  jstring j_type_string = (jstring)jni->CallObjectMethod(
      j_type, j_canonical_form_id);
  CHECK_EXCEPTION(jni) << "error during CallObjectMethod";
  std::string std_type = JavaToStdString(jni, j_type_string);

  jfieldID j_description_id = GetFieldID(
      jni, GetObjectClass(jni, j_sdp), "description", "Ljava/lang/String;");
  jstring j_description = (jstring)GetObjectField(jni, j_sdp, j_description_id);
  std::string std_description = JavaToStdString(jni, j_description);

  return webrtc::CreateSessionDescription(
      std_type, std_description, NULL);
}

JOW(void, PeerConnection_setLocalDescription)(
    JNIEnv* jni, jobject j_pc,
    jobject j_observer, jobject j_sdp) {
  rtc::scoped_refptr<SetSdpObserverWrapper> observer(
      new rtc::RefCountedObject<SetSdpObserverWrapper>(
          jni, j_observer, reinterpret_cast<ConstraintsWrapper*>(NULL)));
  ExtractNativePC(jni, j_pc)->SetLocalDescription(
      observer, JavaSdpToNativeSdp(jni, j_sdp));
}

JOW(void, PeerConnection_setRemoteDescription)(
    JNIEnv* jni, jobject j_pc,
    jobject j_observer, jobject j_sdp) {
  rtc::scoped_refptr<SetSdpObserverWrapper> observer(
      new rtc::RefCountedObject<SetSdpObserverWrapper>(
          jni, j_observer, reinterpret_cast<ConstraintsWrapper*>(NULL)));
  ExtractNativePC(jni, j_pc)->SetRemoteDescription(
      observer, JavaSdpToNativeSdp(jni, j_sdp));
}

JOW(jboolean, PeerConnection_setConfiguration)(
    JNIEnv* jni, jobject j_pc, jobject j_rtc_config) {
  PeerConnectionInterface::RTCConfiguration rtc_config;
  JavaRTCConfigurationToJsepRTCConfiguration(jni, j_rtc_config, &rtc_config);
  return ExtractNativePC(jni, j_pc)->SetConfiguration(rtc_config);
}

JOW(jboolean, PeerConnection_nativeAddIceCandidate)(
    JNIEnv* jni, jobject j_pc, jstring j_sdp_mid,
    jint j_sdp_mline_index, jstring j_candidate_sdp) {
  std::string sdp_mid = JavaToStdString(jni, j_sdp_mid);
  std::string sdp = JavaToStdString(jni, j_candidate_sdp);
  scoped_ptr<IceCandidateInterface> candidate(
      webrtc::CreateIceCandidate(sdp_mid, j_sdp_mline_index, sdp, NULL));
  return ExtractNativePC(jni, j_pc)->AddIceCandidate(candidate.get());
}

JOW(jboolean, PeerConnection_nativeAddLocalStream)(
    JNIEnv* jni, jobject j_pc, jlong native_stream) {
  return ExtractNativePC(jni, j_pc)->AddStream(
      reinterpret_cast<MediaStreamInterface*>(native_stream));
}

JOW(void, PeerConnection_nativeRemoveLocalStream)(
    JNIEnv* jni, jobject j_pc, jlong native_stream) {
  ExtractNativePC(jni, j_pc)->RemoveStream(
      reinterpret_cast<MediaStreamInterface*>(native_stream));
}

JOW(jobject, PeerConnection_nativeCreateSender)(
    JNIEnv* jni, jobject j_pc, jstring j_kind, jstring j_stream_id) {
  jclass j_rtp_sender_class = FindClass(jni, "org/webrtc/RtpSender");
  jmethodID j_rtp_sender_ctor =
      GetMethodID(jni, j_rtp_sender_class, "<init>", "(J)V");

  std::string kind = JavaToStdString(jni, j_kind);
  std::string stream_id = JavaToStdString(jni, j_stream_id);
  rtc::scoped_refptr<RtpSenderInterface> sender =
      ExtractNativePC(jni, j_pc)->CreateSender(kind, stream_id);
  if (!sender.get()) {
    return nullptr;
  }
  jlong nativeSenderPtr = jlongFromPointer(sender.get());
  jobject j_sender =
      jni->NewObject(j_rtp_sender_class, j_rtp_sender_ctor, nativeSenderPtr);
  CHECK_EXCEPTION(jni) << "error during NewObject";
  // Sender is now owned by the Java object, and will be freed from
  // RtpSender.dispose(), called by PeerConnection.dispose() or getSenders().
  sender->AddRef();
  return j_sender;
}

JOW(jobject, PeerConnection_nativeGetSenders)(JNIEnv* jni, jobject j_pc) {
  jclass j_array_list_class = FindClass(jni, "java/util/ArrayList");
  jmethodID j_array_list_ctor =
      GetMethodID(jni, j_array_list_class, "<init>", "()V");
  jmethodID j_array_list_add =
      GetMethodID(jni, j_array_list_class, "add", "(Ljava/lang/Object;)Z");
  jobject j_senders = jni->NewObject(j_array_list_class, j_array_list_ctor);
  CHECK_EXCEPTION(jni) << "error during NewObject";

  jclass j_rtp_sender_class = FindClass(jni, "org/webrtc/RtpSender");
  jmethodID j_rtp_sender_ctor =
      GetMethodID(jni, j_rtp_sender_class, "<init>", "(J)V");

  auto senders = ExtractNativePC(jni, j_pc)->GetSenders();
  for (const auto& sender : senders) {
    jlong nativeSenderPtr = jlongFromPointer(sender.get());
    jobject j_sender =
        jni->NewObject(j_rtp_sender_class, j_rtp_sender_ctor, nativeSenderPtr);
    CHECK_EXCEPTION(jni) << "error during NewObject";
    // Sender is now owned by the Java object, and will be freed from
    // RtpSender.dispose(), called by PeerConnection.dispose() or getSenders().
    sender->AddRef();
    jni->CallBooleanMethod(j_senders, j_array_list_add, j_sender);
    CHECK_EXCEPTION(jni) << "error during CallBooleanMethod";
  }
  return j_senders;
}

JOW(jobject, PeerConnection_nativeGetReceivers)(JNIEnv* jni, jobject j_pc) {
  jclass j_array_list_class = FindClass(jni, "java/util/ArrayList");
  jmethodID j_array_list_ctor =
      GetMethodID(jni, j_array_list_class, "<init>", "()V");
  jmethodID j_array_list_add =
      GetMethodID(jni, j_array_list_class, "add", "(Ljava/lang/Object;)Z");
  jobject j_receivers = jni->NewObject(j_array_list_class, j_array_list_ctor);
  CHECK_EXCEPTION(jni) << "error during NewObject";

  jclass j_rtp_receiver_class = FindClass(jni, "org/webrtc/RtpReceiver");
  jmethodID j_rtp_receiver_ctor =
      GetMethodID(jni, j_rtp_receiver_class, "<init>", "(J)V");

  auto receivers = ExtractNativePC(jni, j_pc)->GetReceivers();
  for (const auto& receiver : receivers) {
    jlong nativeReceiverPtr = jlongFromPointer(receiver.get());
    jobject j_receiver = jni->NewObject(j_rtp_receiver_class,
                                        j_rtp_receiver_ctor, nativeReceiverPtr);
    CHECK_EXCEPTION(jni) << "error during NewObject";
    // Receiver is now owned by Java object, and will be freed from there.
    receiver->AddRef();
    jni->CallBooleanMethod(j_receivers, j_array_list_add, j_receiver);
    CHECK_EXCEPTION(jni) << "error during CallBooleanMethod";
  }
  return j_receivers;
}

JOW(bool, PeerConnection_nativeGetStats)(
    JNIEnv* jni, jobject j_pc, jobject j_observer, jlong native_track) {
  rtc::scoped_refptr<StatsObserverWrapper> observer(
      new rtc::RefCountedObject<StatsObserverWrapper>(jni, j_observer));
  return ExtractNativePC(jni, j_pc)->GetStats(
      observer,
      reinterpret_cast<MediaStreamTrackInterface*>(native_track),
      PeerConnectionInterface::kStatsOutputLevelStandard);
}

JOW(jobject, PeerConnection_signalingState)(JNIEnv* jni, jobject j_pc) {
  PeerConnectionInterface::SignalingState state =
      ExtractNativePC(jni, j_pc)->signaling_state();
  return JavaEnumFromIndex(jni, "PeerConnection$SignalingState", state);
}

JOW(jobject, PeerConnection_iceConnectionState)(JNIEnv* jni, jobject j_pc) {
  PeerConnectionInterface::IceConnectionState state =
      ExtractNativePC(jni, j_pc)->ice_connection_state();
  return JavaEnumFromIndex(jni, "PeerConnection$IceConnectionState", state);
}

JOW(jobject, PeerConnection_iceGatheringState)(JNIEnv* jni, jobject j_pc) {
  PeerConnectionInterface::IceGatheringState state =
      ExtractNativePC(jni, j_pc)->ice_gathering_state();
  return JavaEnumFromIndex(jni, "PeerConnection$IceGatheringState", state);
}

JOW(void, PeerConnection_close)(JNIEnv* jni, jobject j_pc) {
  ExtractNativePC(jni, j_pc)->Close();
  return;
}

JOW(jobject, MediaSource_nativeState)(JNIEnv* jni, jclass, jlong j_p) {
  rtc::scoped_refptr<MediaSourceInterface> p(
      reinterpret_cast<MediaSourceInterface*>(j_p));
  return JavaEnumFromIndex(jni, "MediaSource$State", p->state());
}

JOW(jobject, VideoCapturer_nativeCreateVideoCapturer)(
    JNIEnv* jni, jclass, jstring j_device_name) {
// Since we can't create platform specific java implementations in Java, we
// defer the creation to C land.
#if defined(ANDROID)
  // TODO(nisse): This case is intended to be deleted.
  jclass j_video_capturer_class(
      FindClass(jni, "org/webrtc/VideoCapturerAndroid"));
  const int camera_id = jni->CallStaticIntMethod(
      j_video_capturer_class,
      GetStaticMethodID(jni, j_video_capturer_class, "lookupDeviceName",
                        "(Ljava/lang/String;)I"),
      j_device_name);
  CHECK_EXCEPTION(jni) << "error during VideoCapturerAndroid.lookupDeviceName";
  if (camera_id == -1)
    return nullptr;
  jobject j_video_capturer = jni->NewObject(
      j_video_capturer_class,
      GetMethodID(jni, j_video_capturer_class, "<init>", "(I)V"), camera_id);
  CHECK_EXCEPTION(jni) << "error during creation of VideoCapturerAndroid";
  jfieldID helper_fid = GetFieldID(jni, j_video_capturer_class, "surfaceHelper",
                                   "Lorg/webrtc/SurfaceTextureHelper;");

  rtc::scoped_refptr<webrtc::AndroidVideoCapturerDelegate> delegate =
      new rtc::RefCountedObject<AndroidVideoCapturerJni>(
          jni, j_video_capturer,
          GetObjectField(jni, j_video_capturer, helper_fid));
  rtc::scoped_ptr<cricket::VideoCapturer> capturer(
      new webrtc::AndroidVideoCapturer(delegate));

#else
  std::string device_name = JavaToStdString(jni, j_device_name);
  scoped_ptr<cricket::DeviceManagerInterface> device_manager(
      cricket::DeviceManagerFactory::Create());
  RTC_CHECK(device_manager->Init()) << "DeviceManager::Init() failed";
  cricket::Device device;
  if (!device_manager->GetVideoCaptureDevice(device_name, &device)) {
    LOG(LS_ERROR) << "GetVideoCaptureDevice failed for " << device_name;
    return 0;
  }
  scoped_ptr<cricket::VideoCapturer> capturer(
      device_manager->CreateVideoCapturer(device));

  jclass j_video_capturer_class(
      FindClass(jni, "org/webrtc/VideoCapturer"));
  const jmethodID j_videocapturer_ctor(GetMethodID(
      jni, j_video_capturer_class, "<init>", "()V"));
  jobject j_video_capturer =
      jni->NewObject(j_video_capturer_class,
                     j_videocapturer_ctor);
  CHECK_EXCEPTION(jni) << "error during creation of VideoCapturer";

#endif
  const jmethodID j_videocapturer_set_native_capturer(GetMethodID(
      jni, j_video_capturer_class, "setNativeCapturer", "(J)V"));
  jni->CallVoidMethod(j_video_capturer,
                      j_videocapturer_set_native_capturer,
                      jlongFromPointer(capturer.release()));
  CHECK_EXCEPTION(jni) << "error during setNativeCapturer";
  return j_video_capturer;
}

JOW(jlong, VideoRenderer_nativeCreateGuiVideoRenderer)(
    JNIEnv* jni, jclass, int x, int y) {
  scoped_ptr<VideoRendererWrapper> renderer(VideoRendererWrapper::Create(
      cricket::VideoRendererFactory::CreateGuiVideoRenderer(x, y)));
  return (jlong)renderer.release();
}

JOW(jlong, VideoRenderer_nativeWrapVideoRenderer)(
    JNIEnv* jni, jclass, jobject j_callbacks) {
  scoped_ptr<JavaVideoRendererWrapper> renderer(
      new JavaVideoRendererWrapper(jni, j_callbacks));
  return (jlong)renderer.release();
}

JOW(void, VideoRenderer_nativeCopyPlane)(
    JNIEnv *jni, jclass, jobject j_src_buffer, jint width, jint height,
    jint src_stride, jobject j_dst_buffer, jint dst_stride) {
  size_t src_size = jni->GetDirectBufferCapacity(j_src_buffer);
  size_t dst_size = jni->GetDirectBufferCapacity(j_dst_buffer);
  RTC_CHECK(src_stride >= width) << "Wrong source stride " << src_stride;
  RTC_CHECK(dst_stride >= width) << "Wrong destination stride " << dst_stride;
  RTC_CHECK(src_size >= src_stride * height)
      << "Insufficient source buffer capacity " << src_size;
  RTC_CHECK(dst_size >= dst_stride * height)
      << "Isufficient destination buffer capacity " << dst_size;
  uint8_t *src =
      reinterpret_cast<uint8_t*>(jni->GetDirectBufferAddress(j_src_buffer));
  uint8_t *dst =
      reinterpret_cast<uint8_t*>(jni->GetDirectBufferAddress(j_dst_buffer));
  if (src_stride == dst_stride) {
    memcpy(dst, src, src_stride * height);
  } else {
    for (int i = 0; i < height; i++) {
      memcpy(dst, src, width);
      src += src_stride;
      dst += dst_stride;
    }
  }
}

JOW(void, VideoSource_stop)(JNIEnv* jni, jclass, jlong j_p) {
  reinterpret_cast<VideoSourceInterface*>(j_p)->Stop();
}

JOW(void, VideoSource_restart)(
    JNIEnv* jni, jclass, jlong j_p_source, jlong j_p_format) {
  reinterpret_cast<VideoSourceInterface*>(j_p_source)->Restart();
}

JOW(jstring, MediaStreamTrack_nativeId)(JNIEnv* jni, jclass, jlong j_p) {
  return JavaStringFromStdString(
      jni, reinterpret_cast<MediaStreamTrackInterface*>(j_p)->id());
}

JOW(jstring, MediaStreamTrack_nativeKind)(JNIEnv* jni, jclass, jlong j_p) {
  return JavaStringFromStdString(
      jni, reinterpret_cast<MediaStreamTrackInterface*>(j_p)->kind());
}

JOW(jboolean, MediaStreamTrack_nativeEnabled)(JNIEnv* jni, jclass, jlong j_p) {
  return reinterpret_cast<MediaStreamTrackInterface*>(j_p)->enabled();
}

JOW(jobject, MediaStreamTrack_nativeState)(JNIEnv* jni, jclass, jlong j_p) {
  return JavaEnumFromIndex(
      jni,
      "MediaStreamTrack$State",
      reinterpret_cast<MediaStreamTrackInterface*>(j_p)->state());
}

JOW(jboolean, MediaStreamTrack_nativeSetState)(
    JNIEnv* jni, jclass, jlong j_p, jint j_new_state) {
  MediaStreamTrackInterface::TrackState new_state =
      (MediaStreamTrackInterface::TrackState)j_new_state;
  return reinterpret_cast<MediaStreamTrackInterface*>(j_p)
      ->set_state(new_state);
}

JOW(jboolean, MediaStreamTrack_nativeSetEnabled)(
    JNIEnv* jni, jclass, jlong j_p, jboolean enabled) {
  return reinterpret_cast<MediaStreamTrackInterface*>(j_p)
      ->set_enabled(enabled);
}

JOW(void, VideoTrack_nativeAddRenderer)(
    JNIEnv* jni, jclass,
    jlong j_video_track_pointer, jlong j_renderer_pointer) {
  reinterpret_cast<VideoTrackInterface*>(j_video_track_pointer)->AddRenderer(
      reinterpret_cast<VideoRendererInterface*>(j_renderer_pointer));
}

JOW(void, VideoTrack_nativeRemoveRenderer)(
    JNIEnv* jni, jclass,
    jlong j_video_track_pointer, jlong j_renderer_pointer) {
  reinterpret_cast<VideoTrackInterface*>(j_video_track_pointer)->RemoveRenderer(
      reinterpret_cast<VideoRendererInterface*>(j_renderer_pointer));
}

JOW(jlong, CallSessionFileRotatingLogSink_nativeAddSink)(
    JNIEnv* jni, jclass,
    jstring j_dirPath, jint j_maxFileSize, jint j_severity) {
  std::string dir_path = JavaToStdString(jni, j_dirPath);
  rtc::CallSessionFileRotatingLogSink* sink =
      new rtc::CallSessionFileRotatingLogSink(dir_path, j_maxFileSize);
  if (!sink->Init()) {
    LOG_V(rtc::LoggingSeverity::LS_WARNING) <<
        "Failed to init CallSessionFileRotatingLogSink for path " << dir_path;
    delete sink;
    return 0;
  }
  rtc::LogMessage::AddLogToStream(
      sink, static_cast<rtc::LoggingSeverity>(j_severity));
  return (jlong) sink;
}

JOW(void, CallSessionFileRotatingLogSink_nativeDeleteSink)(
    JNIEnv* jni, jclass, jlong j_sink) {
  rtc::CallSessionFileRotatingLogSink* sink =
      reinterpret_cast<rtc::CallSessionFileRotatingLogSink*>(j_sink);
  rtc::LogMessage::RemoveLogToStream(sink);
  delete sink;
}

JOW(jbyteArray, CallSessionFileRotatingLogSink_nativeGetLogData)(
    JNIEnv* jni, jclass, jstring j_dirPath) {
  std::string dir_path = JavaToStdString(jni, j_dirPath);
  rtc::scoped_ptr<rtc::CallSessionFileRotatingStream> stream(
      new rtc::CallSessionFileRotatingStream(dir_path));
  if (!stream->Open()) {
    LOG_V(rtc::LoggingSeverity::LS_WARNING) <<
        "Failed to open CallSessionFileRotatingStream for path " << dir_path;
    return jni->NewByteArray(0);
  }
  size_t log_size = 0;
  if (!stream->GetSize(&log_size) || log_size == 0) {
    LOG_V(rtc::LoggingSeverity::LS_WARNING) <<
        "CallSessionFileRotatingStream returns 0 size for path " << dir_path;
    return jni->NewByteArray(0);
  }

  size_t read = 0;
  rtc::scoped_ptr<jbyte> buffer(static_cast<jbyte*>(malloc(log_size)));
  stream->ReadAll(buffer.get(), log_size, &read, nullptr);

  jbyteArray result = jni->NewByteArray(read);
  jni->SetByteArrayRegion(result, 0, read, buffer.get());

  return result;
}

JOW(jboolean, RtpSender_nativeSetTrack)(JNIEnv* jni,
                                    jclass,
                                    jlong j_rtp_sender_pointer,
                                    jlong j_track_pointer) {
  return reinterpret_cast<RtpSenderInterface*>(j_rtp_sender_pointer)
      ->SetTrack(reinterpret_cast<MediaStreamTrackInterface*>(j_track_pointer));
}

JOW(jlong, RtpSender_nativeGetTrack)(JNIEnv* jni,
                                  jclass,
                                  jlong j_rtp_sender_pointer,
                                  jlong j_track_pointer) {
  return jlongFromPointer(
      reinterpret_cast<RtpSenderInterface*>(j_rtp_sender_pointer)
          ->track()
          .release());
}

JOW(jstring, RtpSender_nativeId)(
    JNIEnv* jni, jclass, jlong j_rtp_sender_pointer) {
  return JavaStringFromStdString(
      jni, reinterpret_cast<RtpSenderInterface*>(j_rtp_sender_pointer)->id());
}

JOW(void, RtpSender_free)(JNIEnv* jni, jclass, jlong j_rtp_sender_pointer) {
  reinterpret_cast<RtpSenderInterface*>(j_rtp_sender_pointer)->Release();
}

JOW(jlong, RtpReceiver_nativeGetTrack)(JNIEnv* jni,
                                    jclass,
                                    jlong j_rtp_receiver_pointer,
                                    jlong j_track_pointer) {
  return jlongFromPointer(
      reinterpret_cast<RtpReceiverInterface*>(j_rtp_receiver_pointer)
          ->track()
          .release());
}

JOW(jstring, RtpReceiver_nativeId)(
    JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer) {
  return JavaStringFromStdString(
      jni,
      reinterpret_cast<RtpReceiverInterface*>(j_rtp_receiver_pointer)->id());
}

JOW(void, RtpReceiver_free)(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer) {
  reinterpret_cast<RtpReceiverInterface*>(j_rtp_receiver_pointer)->Release();
}

}  // namespace webrtc_jni
