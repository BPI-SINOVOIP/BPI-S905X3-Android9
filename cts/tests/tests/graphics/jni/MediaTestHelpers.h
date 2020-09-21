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

#ifndef ANDROID_MEDIATESTHELPERS_H
#define ANDROID_MEDIATESTHELPERS_H

#include <jni.h>
#include <media/NdkImageReader.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>

class MediaHelper {
public:
  ~MediaHelper();
  bool init(JNIEnv *env, jobject assetMgr, jstring jfilename,
            ANativeWindow *window);
  bool processOneFrame();

private:
  bool createExtractor(JNIEnv *env, jobject assetMgr, jstring jfilename);
  bool createMediaCodec(ANativeWindow *window);
  bool processOneInputBuffer();
  bool processOneOutputBuffer(bool *didProcessedBuffer);

  AMediaExtractor *mExtractor = nullptr;
  AMediaCodec *mCodec = nullptr;
};

#endif // ANDROID_MEDIATESTHELPERS_H
