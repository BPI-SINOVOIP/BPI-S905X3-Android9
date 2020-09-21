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

#define LOG_TAG "ImageReaderTestHelpers"

#include "ImageReaderTestHelpers.h"

#include <android/log.h>

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

ImageReaderHelper::ImageReaderHelper(int32_t width, int32_t height,
                                     int32_t format, uint64_t usage,
                                     int32_t maxImages)
    : mWidth(width), mHeight(height), mFormat(format), mUsage(usage),
      mMaxImages(maxImages) {}

ImageReaderHelper::~ImageReaderHelper() {
  mAcquiredImage.reset();
  if (mImgReaderAnw) {
    AImageReader_delete(mImgReader);
    // No need to call ANativeWindow_release on imageReaderAnw
  }
}

int ImageReaderHelper::initImageReader() {
  if (mImgReader != nullptr || mImgReaderAnw != nullptr) {
    ALOGE("Cannot re-initalize image reader, mImgReader=%p, mImgReaderAnw=%p",
          mImgReader, mImgReaderAnw);
    return -1;
  }

  int ret = AImageReader_newWithUsage(mWidth, mHeight, mFormat, mUsage,
                                      mMaxImages, &mImgReader);
  if (ret != AMEDIA_OK || mImgReader == nullptr) {
    ALOGE("Failed to create new AImageReader, ret=%d, mImgReader=%p", ret,
          mImgReader);
    return -1;
  }

  ret = AImageReader_setImageListener(mImgReader, &mReaderAvailableCb);
  if (ret != AMEDIA_OK) {
    ALOGE("Failed to set image available listener, ret=%d.", ret);
    return ret;
  }

  ret = AImageReader_getWindow(mImgReader, &mImgReaderAnw);
  if (ret != AMEDIA_OK || mImgReaderAnw == nullptr) {
    ALOGE("Failed to get ANativeWindow from AImageReader, ret=%d, "
          "mImgReaderAnw=%p.",
          ret, mImgReaderAnw);
    return -1;
  }

  return 0;
}

int ImageReaderHelper::getBufferFromCurrentImage(AHardwareBuffer **outBuffer) {
  std::lock_guard<std::mutex> lock(mMutex);

  int ret = 0;
  if (mAvailableImages > 0) {
    AImage *outImage = nullptr;

    mAvailableImages -= 1;

    ret = AImageReader_acquireNextImage(mImgReader, &outImage);
    if (ret != AMEDIA_OK || outImage == nullptr) {
      // When the BufferQueue is in async mode, it is still possible that
      // AImageReader_acquireNextImage returns nothing after onFrameAvailable.
      ALOGW("Failed to acquire image, ret=%d, outIamge=%p.", ret, outImage);
    } else {
      // Any exisitng in mAcquiredImage will be deleted and released
      // automatically.
      mAcquiredImage.reset(outImage);
    }
  }

  if (mAcquiredImage == nullptr) {
    return -EAGAIN;
  }

  // Note that AImage_getHardwareBuffer is not acquiring additional reference to
  // the buffer, so we can return it here any times we want without worrying
  // about releasing.
  AHardwareBuffer *buffer = nullptr;
  ret = AImage_getHardwareBuffer(mAcquiredImage.get(), &buffer);
  if (ret != AMEDIA_OK || buffer == nullptr) {
    ALOGE("Faild to get hardware buffer, ret=%d, outBuffer=%p.", ret, buffer);
    return -ENOMEM;
  }

  *outBuffer = buffer;
  return 0;
}

void ImageReaderHelper::handleImageAvailable() {
  std::lock_guard<std::mutex> lock(mMutex);

  mAvailableImages += 1;
}

void ImageReaderHelper::onImageAvailable(void *obj, AImageReader *) {
  ImageReaderHelper *thiz = reinterpret_cast<ImageReaderHelper *>(obj);
  thiz->handleImageAvailable();
}
