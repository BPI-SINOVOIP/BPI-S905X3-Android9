/*
 * Copyright (C) 2015 The Android Open Source Project
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
#ifndef _ACAPTURE_REQUEST_H
#define _ACAPTURE_REQUEST_H

#include <camera/NdkCaptureRequest.h>
#include <set>

using namespace android;

struct ACameraOutputTarget {
    explicit ACameraOutputTarget(ANativeWindow* window) : mWindow(window) {};

    bool operator == (const ACameraOutputTarget& other) const {
        return mWindow == other.mWindow;
    }
    bool operator != (const ACameraOutputTarget& other) const {
        return mWindow != other.mWindow;
    }
    bool operator < (const ACameraOutputTarget& other) const {
        return mWindow < other.mWindow;
    }
    bool operator > (const ACameraOutputTarget& other) const {
        return mWindow > other.mWindow;
    }

    ANativeWindow* mWindow;
};

struct ACameraOutputTargets {
    std::set<ACameraOutputTarget> mOutputs;
};

struct ACaptureRequest {
    camera_status_t setContext(void* ctx) {
        context = ctx;
        return ACAMERA_OK;
    }

    camera_status_t getContext(void** ctx) const {
        *ctx = context;
        return ACAMERA_OK;
    }

    ACameraMetadata*      settings;
    ACameraOutputTargets* targets;
    void*                 context;
};

#endif // _ACAPTURE_REQUEST_H
