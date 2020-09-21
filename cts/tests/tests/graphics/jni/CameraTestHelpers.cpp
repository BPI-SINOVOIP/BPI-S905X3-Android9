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

#define LOG_TAG "CameraTestHelpers"

#include "CameraTestHelpers.h"

#include <android/log.h>

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

CameraHelper::~CameraHelper() { closeCamera(); }

int CameraHelper::initCamera(ANativeWindow *imgReaderAnw) {
  if (imgReaderAnw == nullptr) {
    ALOGE("Cannot initialize camera before image reader get initialized.");
    return -1;
  }

  mImgReaderAnw = imgReaderAnw;
  mCameraManager = ACameraManager_create();
  if (mCameraManager == nullptr) {
    ALOGE("Failed to create ACameraManager.");
    return -1;
  }

  int ret = ACameraManager_getCameraIdList(mCameraManager, &mCameraIdList);
  if (ret != AMEDIA_OK) {
    ALOGE("Failed to get cameraIdList: ret=%d", ret);
    return ret;
  }
  if (mCameraIdList->numCameras < 1) {
    ALOGW("Device has no NDK compatible camera.");
    return 0;
  }
  ALOGI("Found %d camera(s).", mCameraIdList->numCameras);

  // We always use the first camera.
  mCameraId = mCameraIdList->cameraIds[0];
  if (mCameraId == nullptr) {
    ALOGE("Failed to get cameraId.");
    return -1;
  }

  ret = ACameraManager_openCamera(mCameraManager, mCameraId, &mDeviceCb,
                                  &mDevice);
  if (ret != AMEDIA_OK || mDevice == nullptr) {
    ALOGE("Failed to open camera, ret=%d, mDevice=%p.", ret, mDevice);
    return -1;
  }

  ret = ACameraManager_getCameraCharacteristics(mCameraManager, mCameraId,
                                                &mCameraMetadata);
  if (ret != ACAMERA_OK || mCameraMetadata == nullptr) {
    ALOGE("Get camera %s characteristics failure. ret %d, metadata %p",
          mCameraId, ret, mCameraMetadata);
    return -1;
  }

  if (!isCapabilitySupported(
          ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE)) {
    ALOGW("Camera does not support BACKWARD_COMPATIBLE.");
    return 0;
  }

  // Create capture session
  ret = ACaptureSessionOutputContainer_create(&mOutputs);
  if (ret != AMEDIA_OK) {
    ALOGE("ACaptureSessionOutputContainer_create failed, ret=%d", ret);
    return ret;
  }
  ret = ACaptureSessionOutput_create(mImgReaderAnw, &mImgReaderOutput);
  if (ret != AMEDIA_OK) {
    ALOGE("ACaptureSessionOutput_create failed, ret=%d", ret);
    return ret;
  }
  ret = ACaptureSessionOutputContainer_add(mOutputs, mImgReaderOutput);
  if (ret != AMEDIA_OK) {
    ALOGE("ACaptureSessionOutputContainer_add failed, ret=%d", ret);
    return ret;
  }
  ret = ACameraDevice_createCaptureSession(mDevice, mOutputs, &mSessionCb,
                                           &mSession);
  if (ret != AMEDIA_OK) {
    ALOGE("ACameraDevice_createCaptureSession failed, ret=%d", ret);
    return ret;
  }

  // Create capture request
  ret = ACameraDevice_createCaptureRequest(mDevice, TEMPLATE_RECORD,
                                           &mCaptureRequest);
  if (ret != AMEDIA_OK) {
    ALOGE("ACameraDevice_createCaptureRequest failed, ret=%d", ret);
    return ret;
  }
  ret = ACameraOutputTarget_create(mImgReaderAnw, &mReqImgReaderOutput);
  if (ret != AMEDIA_OK) {
    ALOGE("ACameraOutputTarget_create failed, ret=%d", ret);
    return ret;
  }
  ret = ACaptureRequest_addTarget(mCaptureRequest, mReqImgReaderOutput);
  if (ret != AMEDIA_OK) {
    ALOGE("ACaptureRequest_addTarget failed, ret=%d", ret);
    return ret;
  }

  mIsCameraReady = true;
  return 0;
}

bool CameraHelper::isCapabilitySupported(
    acamera_metadata_enum_android_request_available_capabilities_t cap) {
  ACameraMetadata_const_entry entry;
  ACameraMetadata_getConstEntry(mCameraMetadata,
                                ACAMERA_REQUEST_AVAILABLE_CAPABILITIES, &entry);
  for (uint32_t i = 0; i < entry.count; i++) {
    if (entry.data.u8[i] == cap) {
      return true;
    }
  }
  return false;
}

void CameraHelper::closeCamera() {
  // Destroy capture request
  if (mReqImgReaderOutput) {
    ACameraOutputTarget_free(mReqImgReaderOutput);
    mReqImgReaderOutput = nullptr;
  }
  if (mCaptureRequest) {
    ACaptureRequest_free(mCaptureRequest);
    mCaptureRequest = nullptr;
  }
  // Destroy capture session
  if (mSession != nullptr) {
    ACameraCaptureSession_close(mSession);
    mSession = nullptr;
  }
  if (mImgReaderOutput) {
    ACaptureSessionOutput_free(mImgReaderOutput);
    mImgReaderOutput = nullptr;
  }
  if (mOutputs) {
    ACaptureSessionOutputContainer_free(mOutputs);
    mOutputs = nullptr;
  }
  // Destroy camera device
  if (mDevice) {
    ACameraDevice_close(mDevice);
    mDevice = nullptr;
  }
  if (mCameraMetadata) {
    ACameraMetadata_free(mCameraMetadata);
    mCameraMetadata = nullptr;
  }
  // Destroy camera manager
  if (mCameraIdList) {
    ACameraManager_deleteCameraIdList(mCameraIdList);
    mCameraIdList = nullptr;
  }
  if (mCameraManager) {
    ACameraManager_delete(mCameraManager);
    mCameraManager = nullptr;
  }
  mIsCameraReady = false;
}

int CameraHelper::takePicture() {
  return ACameraCaptureSession_capture(mSession, nullptr, 1, &mCaptureRequest,
                                       nullptr);
}
