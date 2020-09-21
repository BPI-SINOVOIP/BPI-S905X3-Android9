/*
 * Copyright 2018 The Android Open Source Project
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
 *
 */

#define LOG_TAG "MediaHelper"

#include "MediaTestHelpers.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <string.h>
#include <unistd.h>

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define ASSERT(a)                                                              \
  if (!(a)) {                                                                  \
    ALOGE("Failure: " #a " at " __FILE__ ":%d", __LINE__);                     \
    return false;                                                              \
  }

#define MEDIA_CALL(a) ASSERT(AMEDIA_OK == (a))

MediaHelper::~MediaHelper() {
  if (mCodec != nullptr) {
    AMediaCodec_stop(mCodec);
    AMediaCodec_delete(mCodec);
    mCodec = nullptr;
  }
  if (mExtractor != nullptr) {
    AMediaExtractor_delete(mExtractor);
    mExtractor = nullptr;
  }
}

bool MediaHelper::init(JNIEnv *env, jobject assetMgr, jstring jfilename,
                       ANativeWindow *window) {
  // Create a media extractor which will provide codec data from jfilename.
  ASSERT(createExtractor(env, assetMgr, jfilename));
  // Create a media codec and image reader which receives frames from the
  // codec.
  ASSERT(createMediaCodec(window));

  return true;
}

bool MediaHelper::processOneFrame() {
  ASSERT(mCodec);

  // Wait until the codec produces an output buffer, which we can read as
  // an image.
  bool gotOutputBuffer = false;
  do {
    ASSERT(processOneInputBuffer());
    ASSERT(processOneOutputBuffer(&gotOutputBuffer));
  } while (!gotOutputBuffer);

  return true;
}

bool MediaHelper::createExtractor(JNIEnv *env, jobject assetMgr,
                                  jstring jfilename) {
  ASSERT(nullptr == mExtractor);
  const char *filename = env->GetStringUTFChars(jfilename, NULL);
  ASSERT(filename);

  struct ReleaseFd {
    ~ReleaseFd() {
      if (fd >= 0)
        close(fd);
    }

    int fd = -1;
  } releaseFd;

  off_t outStart, outLen;
  releaseFd.fd = AAsset_openFileDescriptor(
      AAssetManager_open(AAssetManager_fromJava(env, assetMgr), filename, 0),
      &outStart, &outLen);
  env->ReleaseStringUTFChars(jfilename, filename);
  ASSERT(releaseFd.fd >= 0);

  mExtractor = AMediaExtractor_new();
  MEDIA_CALL(AMediaExtractor_setDataSourceFd(mExtractor, releaseFd.fd,
                                             static_cast<off64_t>(outStart),
                                             static_cast<off64_t>(outLen)));
  return true;
}

bool MediaHelper::createMediaCodec(ANativeWindow *window) {
  ASSERT(mExtractor);
  ASSERT(nullptr == mCodec);

  // Create a mCodec from the video track.
  int numTracks = AMediaExtractor_getTrackCount(mExtractor);
  AMediaFormat *format;
  const char *mime;
  for (int i = 0; i < numTracks; ++i) {
    format = AMediaExtractor_getTrackFormat(mExtractor, i);
    ASSERT(AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime));

    if (!strncmp(mime, "video/", 6)) {
      MEDIA_CALL(AMediaExtractor_selectTrack(mExtractor, i));
      break;
    }

    AMediaFormat_delete(format);
  }

  mCodec = AMediaCodec_createDecoderByType(mime);
  ASSERT(mCodec);
  MEDIA_CALL(AMediaCodec_configure(mCodec, format, window, NULL, 0));
  AMediaFormat_delete(format);
  MEDIA_CALL(AMediaCodec_start(mCodec));

  return true;
}

bool MediaHelper::processOneInputBuffer() {
  ssize_t bufferIndex = AMediaCodec_dequeueInputBuffer(mCodec, 2000);
  // No input buffer ready, just return, we'll try again.
  if (bufferIndex < 0)
    return true;

  size_t bufferSize = 0;
  uint8_t *buffer =
      AMediaCodec_getInputBuffer(mCodec, bufferIndex, &bufferSize);
  ASSERT(buffer);

  ssize_t sampleSize =
      AMediaExtractor_readSampleData(mExtractor, buffer, bufferSize);
  ASSERT(sampleSize >= 0);

  auto presentationTimeUs = AMediaExtractor_getSampleTime(mExtractor);
  MEDIA_CALL(AMediaCodec_queueInputBuffer(mCodec, bufferIndex, 0, sampleSize,
                                          presentationTimeUs, 0));

  ASSERT(AMediaExtractor_advance(mExtractor));

  return true;
}

// Returns true if we successfully got an output buffer.
bool MediaHelper::processOneOutputBuffer(bool *didProcessBuffer) {
  *didProcessBuffer = false;

  AMediaCodecBufferInfo info;
  ssize_t index = AMediaCodec_dequeueOutputBuffer(mCodec, &info, 0);
  if (index < 0) {
    switch (index) {
    case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
    case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:
    case AMEDIACODEC_INFO_TRY_AGAIN_LATER:
      return true;
    default:
      return false;
    }
  }

  *didProcessBuffer = true;
  MEDIA_CALL(AMediaCodec_releaseOutputBuffer(mCodec, index, info.size != 0));
  return true;
}
