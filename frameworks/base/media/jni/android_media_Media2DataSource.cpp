/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "JMedia2DataSource-JNI"
#include <utils/Log.h>

#include "android_media_Media2DataSource.h"

#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/Log.h"
#include "jni.h"
#include <nativehelper/JNIHelp.h>

#include <drm/drm_framework_common.h>
#include <media/stagefright/foundation/ADebug.h>
#include <nativehelper/ScopedLocalRef.h>

namespace android {

static const size_t kBufferSize = 64 * 1024;

JMedia2DataSource::JMedia2DataSource(JNIEnv* env, jobject source)
    : mJavaObjStatus(OK),
      mSizeIsCached(false),
      mCachedSize(0) {
    mMedia2DataSourceObj = env->NewGlobalRef(source);
    CHECK(mMedia2DataSourceObj != NULL);

    ScopedLocalRef<jclass> media2DataSourceClass(env, env->GetObjectClass(mMedia2DataSourceObj));
    CHECK(media2DataSourceClass.get() != NULL);

    mReadAtMethod = env->GetMethodID(media2DataSourceClass.get(), "readAt", "(J[BII)I");
    CHECK(mReadAtMethod != NULL);
    mGetSizeMethod = env->GetMethodID(media2DataSourceClass.get(), "getSize", "()J");
    CHECK(mGetSizeMethod != NULL);
    mCloseMethod = env->GetMethodID(media2DataSourceClass.get(), "close", "()V");
    CHECK(mCloseMethod != NULL);

    ScopedLocalRef<jbyteArray> tmp(env, env->NewByteArray(kBufferSize));
    mByteArrayObj = (jbyteArray)env->NewGlobalRef(tmp.get());
    CHECK(mByteArrayObj != NULL);
}

JMedia2DataSource::~JMedia2DataSource() {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    env->DeleteGlobalRef(mMedia2DataSourceObj);
    env->DeleteGlobalRef(mByteArrayObj);
}

status_t JMedia2DataSource::initCheck() const {
    return OK;
}

ssize_t JMedia2DataSource::readAt(off64_t offset, void *data, size_t size) {
    Mutex::Autolock lock(mLock);

    if (mJavaObjStatus != OK) {
        return -1;
    }
    if (size > kBufferSize) {
        size = kBufferSize;
    }

    JNIEnv* env = AndroidRuntime::getJNIEnv();
    jint numread = env->CallIntMethod(mMedia2DataSourceObj, mReadAtMethod,
            (jlong)offset, mByteArrayObj, (jint)0, (jint)size);
    if (env->ExceptionCheck()) {
        ALOGW("An exception occurred in readAt()");
        LOGW_EX(env);
        env->ExceptionClear();
        mJavaObjStatus = UNKNOWN_ERROR;
        return -1;
    }
    if (numread < 0) {
        if (numread != -1) {
            ALOGW("An error occurred in readAt()");
            mJavaObjStatus = UNKNOWN_ERROR;
            return -1;
        } else {
            // numread == -1 indicates EOF
            return 0;
        }
    }
    if ((size_t)numread > size) {
        ALOGE("readAt read too many bytes.");
        mJavaObjStatus = UNKNOWN_ERROR;
        return -1;
    }

    ALOGV("readAt %lld / %zu => %d.", (long long)offset, size, numread);
    env->GetByteArrayRegion(mByteArrayObj, 0, numread, (jbyte*)data);
    return numread;
}

status_t JMedia2DataSource::getSize(off64_t* size) {
    Mutex::Autolock lock(mLock);

    if (mJavaObjStatus != OK) {
        return UNKNOWN_ERROR;
    }
    if (mSizeIsCached) {
        *size = mCachedSize;
        return OK;
    }

    JNIEnv* env = AndroidRuntime::getJNIEnv();
    *size = env->CallLongMethod(mMedia2DataSourceObj, mGetSizeMethod);
    if (env->ExceptionCheck()) {
        ALOGW("An exception occurred in getSize()");
        LOGW_EX(env);
        env->ExceptionClear();
        // After returning an error, size shouldn't be used by callers.
        *size = UNKNOWN_ERROR;
        mJavaObjStatus = UNKNOWN_ERROR;
        return UNKNOWN_ERROR;
    }

    // The minimum size should be -1, which indicates unknown size.
    if (*size < 0) {
        *size = -1;
    }

    mCachedSize = *size;
    mSizeIsCached = true;
    return OK;
}

void JMedia2DataSource::close() {
    Mutex::Autolock lock(mLock);

    JNIEnv* env = AndroidRuntime::getJNIEnv();
    env->CallVoidMethod(mMedia2DataSourceObj, mCloseMethod);
    // The closed state is effectively the same as an error state.
    mJavaObjStatus = UNKNOWN_ERROR;
}

String8 JMedia2DataSource::toString() {
    return String8::format("JMedia2DataSource(pid %d, uid %d)", getpid(), getuid());
}

String8 JMedia2DataSource::getMIMEType() const {
    return String8("application/octet-stream");
}

}  // namespace android
