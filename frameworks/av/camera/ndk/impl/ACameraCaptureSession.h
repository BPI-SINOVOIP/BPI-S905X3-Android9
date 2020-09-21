/*
 * Copyright (C) 2016 The Android Open Source Project
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
 */
#ifndef _ACAMERA_CAPTURE_SESSION_H
#define _ACAMERA_CAPTURE_SESSION_H

#include <set>
#include <hardware/camera3.h>
#include <camera/NdkCameraDevice.h>
#include "ACameraDevice.h"

using namespace android;

struct ACaptureSessionOutput {
    explicit ACaptureSessionOutput(ANativeWindow* window, bool isShared = false) :
            mWindow(window), mIsShared(isShared) {};

    bool operator == (const ACaptureSessionOutput& other) const {
        return mWindow == other.mWindow;
    }
    bool operator != (const ACaptureSessionOutput& other) const {
        return mWindow != other.mWindow;
    }
    bool operator < (const ACaptureSessionOutput& other) const {
        return mWindow < other.mWindow;
    }
    bool operator > (const ACaptureSessionOutput& other) const {
        return mWindow > other.mWindow;
    }

    ANativeWindow* mWindow;
    std::set<ANativeWindow *> mSharedWindows;
    bool           mIsShared;
    int            mRotation = CAMERA3_STREAM_ROTATION_0;
};

struct ACaptureSessionOutputContainer {
    std::set<ACaptureSessionOutput> mOutputs;
};

/**
 * ACameraCaptureSession opaque struct definition
 * Leave outside of android namespace because it's NDK struct
 */
struct ACameraCaptureSession : public RefBase {
  public:
    ACameraCaptureSession(
            int id,
            const ACaptureSessionOutputContainer* outputs,
            const ACameraCaptureSession_stateCallbacks* cb,
            CameraDevice* device) :
            mId(id), mOutput(*outputs), mUserSessionCallback(*cb),
            mDevice(device) {}

    // This can be called in app calling close() or after some app callback is finished
    // Make sure the caller does not hold device or session lock!
    ~ACameraCaptureSession();

    // No API except Session_Close will work if device is closed
    // A session will enter closed state when one of the following happens:
    //     1. Explicitly closed by app
    //     2. Replaced by a newer session
    //     3. Device is closed
    bool isClosed() { Mutex::Autolock _l(mSessionLock); return mIsClosed; }

    // Close the session and mark app no longer need this session.
    void closeByApp();

    camera_status_t stopRepeating();

    camera_status_t abortCaptures();

    camera_status_t setRepeatingRequest(
            /*optional*/ACameraCaptureSession_captureCallbacks* cbs,
            int numRequests, ACaptureRequest** requests,
            /*optional*/int* captureSequenceId);

    camera_status_t capture(
            /*optional*/ACameraCaptureSession_captureCallbacks* cbs,
            int numRequests, ACaptureRequest** requests,
            /*optional*/int* captureSequenceId);

    camera_status_t updateOutputConfiguration(ACaptureSessionOutput *output);

    ACameraDevice* getDevice();

  private:
    friend class CameraDevice;

    // Close session because app close camera device, camera device got ERROR_DISCONNECTED,
    // or a new session is replacing this session.
    void closeByDevice();

    sp<CameraDevice> getDeviceSp();

    const int mId;
    const ACaptureSessionOutputContainer mOutput;
    const ACameraCaptureSession_stateCallbacks mUserSessionCallback;
    const wp<CameraDevice> mDevice;
    bool  mIsClosed = false;
    bool  mClosedByApp = false;
    Mutex mSessionLock;
};

#endif // _ACAMERA_CAPTURE_SESSION_H
