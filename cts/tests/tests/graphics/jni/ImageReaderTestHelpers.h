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

#ifndef ANDROID_IMAGEREADERTESTHELPERS_H
#define ANDROID_IMAGEREADERTESTHELPERS_H

#include <deque>
#include <mutex>

#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>

class ImageReaderHelper {
public:
  using ImagePtr = std::unique_ptr<AImage, decltype(&AImage_delete)>;

  ImageReaderHelper(int32_t width, int32_t height, int32_t format,
                    uint64_t usage, int32_t maxImages);
  ~ImageReaderHelper();
  int initImageReader();
  ANativeWindow *getNativeWindow() { return mImgReaderAnw; }
  int getBufferFromCurrentImage(AHardwareBuffer **outBuffer);
  void handleImageAvailable();
  static void onImageAvailable(void *obj, AImageReader *);

private:
  int32_t mWidth;
  int32_t mHeight;
  int32_t mFormat;
  uint64_t mUsage;
  uint32_t mMaxImages;

  std::mutex mMutex;
  // Number of images that's avaiable for acquire.
  size_t mAvailableImages{0};
  // Although AImageReader supports acquiring multiple images at a time, we
  // don't really need it in this test. We only acquire one image that a time.
  ImagePtr mAcquiredImage{nullptr, AImage_delete};

  AImageReader *mImgReader{nullptr};
  ANativeWindow *mImgReaderAnw{nullptr};

  AImageReader_ImageListener mReaderAvailableCb{this, onImageAvailable};
};

#endif // ANDROID_IMAGEREADERTESTHELPERS_H
