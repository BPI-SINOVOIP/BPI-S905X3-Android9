/*
 * Copyright (C) 2018 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "NdkMediaDataSource"

#include "NdkMediaDataSourcePriv.h"

#include <inttypes.h>
#include <jni.h>
#include <unistd.h>

#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/StrongPointer.h>
#include <media/NdkMediaError.h>
#include <media/NdkMediaDataSource.h>
#include <media/stagefright/InterfaceUtils.h>

#include "../../libstagefright/include/HTTPBase.h"
#include "../../libstagefright/include/NuCachedSource2.h"

using namespace android;

struct AMediaDataSource {
    void *userdata;
    AMediaDataSourceReadAt readAt;
    AMediaDataSourceGetSize getSize;
    AMediaDataSourceClose close;
};

NdkDataSource::NdkDataSource(AMediaDataSource *dataSource)
    : mDataSource(AMediaDataSource_new()) {
      AMediaDataSource_setReadAt(mDataSource, dataSource->readAt);
      AMediaDataSource_setGetSize(mDataSource, dataSource->getSize);
      AMediaDataSource_setClose(mDataSource, dataSource->close);
      AMediaDataSource_setUserdata(mDataSource, dataSource->userdata);
}

NdkDataSource::~NdkDataSource() {
    AMediaDataSource_delete(mDataSource);
}

status_t NdkDataSource::initCheck() const {
    return OK;
}

ssize_t NdkDataSource::readAt(off64_t offset, void *data, size_t size) {
    Mutex::Autolock l(mLock);
    if (mDataSource->getSize == NULL || mDataSource->userdata == NULL) {
        return -1;
    }
    return mDataSource->readAt(mDataSource->userdata, offset, data, size);
}

status_t NdkDataSource::getSize(off64_t *size) {
    Mutex::Autolock l(mLock);
    if (mDataSource->getSize == NULL || mDataSource->userdata == NULL) {
        return NO_INIT;
    }
    if (size != NULL) {
        *size = mDataSource->getSize(mDataSource->userdata);
    }
    return OK;
}

String8 NdkDataSource::toString() {
    return String8::format("NdkDataSource(pid %d, uid %d)", getpid(), getuid());
}

String8 NdkDataSource::getMIMEType() const {
    return String8("application/octet-stream");
}

void NdkDataSource::close() {
    if (mDataSource->close != NULL && mDataSource->userdata != NULL) {
        mDataSource->close(mDataSource->userdata);
    }
}

extern "C" {

EXPORT
AMediaDataSource* AMediaDataSource_new() {
    AMediaDataSource *mSource = new AMediaDataSource();
    mSource->userdata = NULL;
    mSource->readAt = NULL;
    mSource->getSize = NULL;
    mSource->close = NULL;
    return mSource;
}

EXPORT
void AMediaDataSource_delete(AMediaDataSource *mSource) {
    ALOGV("dtor");
    if (mSource != NULL) {
        delete mSource;
    }
}

EXPORT
void AMediaDataSource_setUserdata(AMediaDataSource *mSource, void *userdata) {
    mSource->userdata = userdata;
}

EXPORT
void AMediaDataSource_setReadAt(AMediaDataSource *mSource, AMediaDataSourceReadAt readAt) {
    mSource->readAt = readAt;
}

EXPORT
void AMediaDataSource_setGetSize(AMediaDataSource *mSource, AMediaDataSourceGetSize getSize) {
    mSource->getSize = getSize;
}

EXPORT
void AMediaDataSource_setClose(AMediaDataSource *mSource, AMediaDataSourceClose close) {
    mSource->close = close;
}

} // extern "C"

