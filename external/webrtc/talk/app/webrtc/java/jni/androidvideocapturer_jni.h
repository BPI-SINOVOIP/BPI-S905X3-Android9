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
 *
 */

#ifndef TALK_APP_WEBRTC_JAVA_JNI_ANDROIDVIDEOCAPTURER_JNI_H_
#define TALK_APP_WEBRTC_JAVA_JNI_ANDROIDVIDEOCAPTURER_JNI_H_

#include <string>

#include "talk/app/webrtc/androidvideocapturer.h"
#include "talk/app/webrtc/java/jni/jni_helpers.h"
#include "webrtc/base/asyncinvoker.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/thread_checker.h"
#include "webrtc/common_video/include/i420_buffer_pool.h"

namespace webrtc_jni {

struct NativeHandleImpl;
class SurfaceTextureHelper;

// AndroidVideoCapturerJni implements AndroidVideoCapturerDelegate.
// The purpose of the delegate is to hide the JNI specifics from the C++ only
// AndroidVideoCapturer.
class AndroidVideoCapturerJni : public webrtc::AndroidVideoCapturerDelegate {
 public:
  static int SetAndroidObjects(JNIEnv* jni, jobject appliction_context);

  AndroidVideoCapturerJni(JNIEnv* jni,
                          jobject j_video_capturer,
                          jobject j_surface_texture_helper);

  void Start(int width, int height, int framerate,
             webrtc::AndroidVideoCapturer* capturer) override;
  void Stop() override;

  std::string GetSupportedFormats() override;

  // Called from VideoCapturerAndroid::NativeObserver on a Java thread.
  void OnCapturerStarted(bool success);
  void OnMemoryBufferFrame(void* video_frame, int length, int width,
                           int height, int rotation, int64_t timestamp_ns);
  void OnTextureFrame(int width, int height, int rotation, int64_t timestamp_ns,
                      const NativeHandleImpl& handle);
  void OnOutputFormatRequest(int width, int height, int fps);

 protected:
  ~AndroidVideoCapturerJni();

 private:
  JNIEnv* jni();

  // To avoid deducing Args from the 3rd parameter of AsyncCapturerInvoke.
  template <typename T>
  struct Identity {
    typedef T type;
  };

  // Helper function to make safe asynchronous calls to |capturer_|. The calls
  // are not guaranteed to be delivered.
  template <typename... Args>
  void AsyncCapturerInvoke(
      const char* method_name,
      void (webrtc::AndroidVideoCapturer::*method)(Args...),
      typename Identity<Args>::type... args);

  const ScopedGlobalRef<jobject> j_video_capturer_;
  const ScopedGlobalRef<jclass> j_video_capturer_class_;
  const ScopedGlobalRef<jclass> j_observer_class_;

  // Used on the Java thread running the camera.
  webrtc::I420BufferPool buffer_pool_;
  rtc::scoped_refptr<SurfaceTextureHelper> surface_texture_helper_;
  rtc::ThreadChecker thread_checker_;

  // |capturer| is a guaranteed to be a valid pointer between a call to
  // AndroidVideoCapturerDelegate::Start
  // until AndroidVideoCapturerDelegate::Stop.
  rtc::CriticalSection capturer_lock_;
  webrtc::AndroidVideoCapturer* capturer_ GUARDED_BY(capturer_lock_);
  // |invoker_| is used to communicate with |capturer_| on the thread Start() is
  // called on.
  rtc::scoped_ptr<rtc::GuardedAsyncInvoker> invoker_ GUARDED_BY(capturer_lock_);

  static jobject application_context_;

  RTC_DISALLOW_COPY_AND_ASSIGN(AndroidVideoCapturerJni);
};

}  // namespace webrtc_jni

#endif  // TALK_APP_WEBRTC_JAVA_JNI_ANDROIDVIDEOCAPTURER_JNI_H_
