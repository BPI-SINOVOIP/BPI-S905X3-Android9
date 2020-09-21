/*
 * Copyright (C) 2006 The Android Open Source Project
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

#ifndef ANDROID_GUI_ISCREEN_CONTROL_SERVICE_H
#define ANDROID_GUI_ISCREEN_CONTROL_SERVICE_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Binder.h>

namespace android {

class IScreenControlService: public IInterface {
public:
    DECLARE_META_INTERFACE(ScreenControlService)

    virtual int startScreenCap(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t width, int32_t height, int32_t sourceType, const char* filename) = 0;

    virtual int startScreenRecord(int32_t width, int32_t height, int32_t frameRate, int32_t bitRate, int32_t limitTimeSec, int32_t sourceType, const char* filename) = 0;
};

class BnScreenControlService: public BnInterface<IScreenControlService> {
public:
    enum {
        SCREEN_CAP,
	SCREEN_REC
    };

    virtual status_t onTransact(uint32_t code, const Parcel& data,
	    Parcel* reply, uint32_t flags = 0);
};

};

#endif
