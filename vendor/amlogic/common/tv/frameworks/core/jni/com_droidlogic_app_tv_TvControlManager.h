/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef __COM_DROIDLOGIC_APP_SYSTEMCTRL_H__
#define __COM_DROIDLOGIC_APP_SYSTEMCTRL_H__
#include <jni.h>
#include <utils/Log.h>
#include "TvServerHidlClient.h"

using namespace android;

class EventCallback : public TvListener {
public:
    EventCallback() {}
    ~EventCallback() {}
    void notify(const tv_parcel_t &parcel);
    mutable Mutex mLock;
};


#endif

