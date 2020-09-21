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

#ifndef __ORG_DTVKIT_INPUTSOURCE_CLIENT_H__
#define __ORG_DTVKIT_INPUTSOURCE_CLIENT_H__
#include <jni.h>
#include <JNIHelp.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <ScopedLocalRef.h>
#include "android_runtime/Log.h"

using namespace android;

enum {
    REQUEST = 0,
    DRAW = 1,
};

enum {
    SYSFS = 0,
    PROP = 1,
};

typedef struct datablock_s {
    int width;
    int height;
    int dst_x;
    int dst_y;
    int dst_width;
    int dst_height;
    void * mem;
} datablock_t;

typedef struct dvbparam_s {
    std::string resource;
    std::string json;
}dvbparam_t;

#endif/*__ORG_DTVKIT_INPUTSOURCE_CLIENT_H__*/

