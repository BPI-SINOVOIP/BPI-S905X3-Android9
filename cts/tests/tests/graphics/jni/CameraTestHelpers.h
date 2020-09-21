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

#ifndef ANDROID_CAMERATESTHELPERS_H
#define ANDROID_CAMERATESTHELPERS_H

// Must come before NdkCameraCaptureSession.h
#include <camera/NdkCaptureRequest.h>

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>

// A helper class which can be used to initialize/control the camera.
class CameraHelper {
public:
  ~CameraHelper();

  int initCamera(ANativeWindow *imgReaderAnw);
  bool isCapabilitySupported(
      acamera_metadata_enum_android_request_available_capabilities_t cap);
  bool isCameraReady() { return mIsCameraReady; }
  void closeCamera();
  int takePicture();

  static void onDeviceDisconnected(void * /*obj*/, ACameraDevice * /*device*/) {
  }
  static void onDeviceError(void * /*obj*/, ACameraDevice * /*device*/,
                            int /*errorCode*/) {}
  static void onSessionClosed(void * /*obj*/,
                              ACameraCaptureSession * /*session*/) {}
  static void onSessionReady(void * /*obj*/,
                             ACameraCaptureSession * /*session*/) {}
  static void onSessionActive(void * /*obj*/,
                              ACameraCaptureSession * /*session*/) {}

private:
  ACameraDevice_StateCallbacks mDeviceCb{this, onDeviceDisconnected,
                                         onDeviceError};
  ACameraCaptureSession_stateCallbacks mSessionCb{
      this, onSessionClosed, onSessionReady, onSessionActive};

  ANativeWindow *mImgReaderAnw{nullptr}; // not owned by us.

  // Camera manager
  ACameraManager *mCameraManager{nullptr};
  ACameraIdList *mCameraIdList{nullptr};
  // Camera device
  ACameraMetadata *mCameraMetadata{nullptr};
  ACameraDevice *mDevice{nullptr};
  // Capture session
  ACaptureSessionOutputContainer *mOutputs{nullptr};
  ACaptureSessionOutput *mImgReaderOutput{nullptr};
  ACameraCaptureSession *mSession{nullptr};
  // Capture request
  ACaptureRequest *mCaptureRequest{nullptr};
  ACameraOutputTarget *mReqImgReaderOutput{nullptr};

  bool mIsCameraReady{false};
  const char *mCameraId{nullptr};
};

#endif // ANDROID_CAMERATHELPERS_H
