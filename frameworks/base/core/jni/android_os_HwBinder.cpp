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

//#define LOG_NDEBUG 0
#define LOG_TAG "android_os_HwBinder"
#include <android-base/logging.h>

#include "android_os_HwBinder.h"

#include "android_os_HwParcel.h"
#include "android_os_HwRemoteBinder.h"

#include <cstring>

#include <nativehelper/JNIHelp.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/base/1.0/IBase.h>
#include <android/hidl/base/1.0/BpHwBase.h>
#include <android_runtime/AndroidRuntime.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Status.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/ProcessState.h>
#include <nativehelper/ScopedLocalRef.h>
#include <nativehelper/ScopedUtfChars.h>
#include <vintf/parse_string.h>
#include <utils/misc.h>

#include "core_jni_helpers.h"

using android::AndroidRuntime;
using android::hardware::hidl_vec;
using android::hardware::hidl_string;
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
template<typename T>
using Return = android::hardware::Return<T>;

#define PACKAGE_PATH    "android/os"
#define CLASS_NAME      "HwBinder"
#define CLASS_PATH      PACKAGE_PATH "/" CLASS_NAME

namespace android {

static jclass gErrorClass;

static struct fields_t {
    jfieldID contextID;
    jmethodID onTransactID;
} gFields;

struct JHwBinderHolder : public RefBase {
    JHwBinderHolder() {}

    sp<JHwBinder> get(JNIEnv *env, jobject obj) {
        Mutex::Autolock autoLock(mLock);

        sp<JHwBinder> binder = mBinder.promote();

        if (binder == NULL) {
            binder = new JHwBinder(env, obj);
            mBinder = binder;
        }

        return binder;
    }

private:
    Mutex mLock;
    wp<JHwBinder> mBinder;

    DISALLOW_COPY_AND_ASSIGN(JHwBinderHolder);
};

// static
void JHwBinder::InitClass(JNIEnv *env) {
    ScopedLocalRef<jclass> clazz(
            env, FindClassOrDie(env, CLASS_PATH));

    gFields.contextID =
        GetFieldIDOrDie(env, clazz.get(), "mNativeContext", "J");

    gFields.onTransactID =
        GetMethodIDOrDie(
                env,
                clazz.get(),
                "onTransact",
                "(IL" PACKAGE_PATH "/HwParcel;L" PACKAGE_PATH "/HwParcel;I)V");
}

// static
sp<JHwBinderHolder> JHwBinder::SetNativeContext(
        JNIEnv *env, jobject thiz, const sp<JHwBinderHolder> &context) {
    sp<JHwBinderHolder> old =
        (JHwBinderHolder *)env->GetLongField(thiz, gFields.contextID);

    if (context != NULL) {
        context->incStrong(NULL /* id */);
    }

    if (old != NULL) {
        old->decStrong(NULL /* id */);
    }

    env->SetLongField(thiz, gFields.contextID, (long)context.get());

    return old;
}

// static
sp<JHwBinder> JHwBinder::GetNativeBinder(
        JNIEnv *env, jobject thiz) {
    JHwBinderHolder *holder =
        reinterpret_cast<JHwBinderHolder *>(
                env->GetLongField(thiz, gFields.contextID));

    return holder->get(env, thiz);
}

JHwBinder::JHwBinder(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    CHECK(clazz != NULL);

    mObject = env->NewGlobalRef(thiz);
}

JHwBinder::~JHwBinder() {
    JNIEnv *env = AndroidRuntime::getJNIEnv();

    env->DeleteGlobalRef(mObject);
    mObject = NULL;
}

status_t JHwBinder::onTransact(
        uint32_t code,
        const hardware::Parcel &data,
        hardware::Parcel *reply,
        uint32_t flags,
        TransactCallback callback) {
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    bool isOneway = (flags & TF_ONE_WAY) != 0;
    ScopedLocalRef<jobject> replyObj(env, nullptr);
    sp<JHwParcel> replyContext = nullptr;

    ScopedLocalRef<jobject> requestObj(env, JHwParcel::NewObject(env));
    JHwParcel::GetNativeContext(env, requestObj.get())->setParcel(
            const_cast<hardware::Parcel *>(&data), false /* assumeOwnership */);


    if (!isOneway) {
        replyObj.reset(JHwParcel::NewObject(env));

        replyContext = JHwParcel::GetNativeContext(env, replyObj.get());

        replyContext->setParcel(reply, false /* assumeOwnership */);
        replyContext->setTransactCallback(callback);
    }

    env->CallVoidMethod(
            mObject,
            gFields.onTransactID,
            code,
            requestObj.get(),
            replyObj.get(),
            flags);

    if (env->ExceptionCheck()) {
        jthrowable excep = env->ExceptionOccurred();
        env->ExceptionDescribe();
        env->ExceptionClear();

        // It is illegal to call IsInstanceOf if there is a pending exception.
        // Attempting to do so results in a JniAbort which crashes the entire process.
        if (env->IsInstanceOf(excep, gErrorClass)) {
            /* It's an error */
            LOG(ERROR) << "Forcefully exiting";
            exit(1);
        } else {
            LOG(ERROR) << "Uncaught exception!";
        }

        env->DeleteLocalRef(excep);
    }

    status_t err = OK;

    if (!isOneway) {
        if (!replyContext->wasSent()) {
            // The implementation never finished the transaction.
            err = UNKNOWN_ERROR;  // XXX special error code instead?

            reply->setDataPosition(0 /* pos */);
        }

        // Release all temporary storage now that scatter-gather data
        // has been consolidated, either by calling the TransactCallback,
        // if wasSent() == true or clearing the reply parcel (setDataOffset above).
        replyContext->getStorage()->release(env);

        // We cannot permanently pass ownership of "data" and "reply" over to their
        // Java object wrappers (we don't own them ourselves).
        replyContext->setParcel(
                NULL /* parcel */, false /* assumeOwnership */);

    }

    JHwParcel::GetNativeContext(env, requestObj.get())->setParcel(
            NULL /* parcel */, false /* assumeOwnership */);

    return err;
}

}  // namespace android

////////////////////////////////////////////////////////////////////////////////

using namespace android;

static void releaseNativeContext(void *nativeContext) {
    sp<JHwBinderHolder> context = static_cast<JHwBinderHolder *>(nativeContext);

    if (context != NULL) {
        context->decStrong(NULL /* id */);
    }
}

static jlong JHwBinder_native_init(JNIEnv *env) {
    JHwBinder::InitClass(env);

    return reinterpret_cast<jlong>(&releaseNativeContext);
}

static void JHwBinder_native_setup(JNIEnv *env, jobject thiz) {
    sp<JHwBinderHolder> context = new JHwBinderHolder;
    JHwBinder::SetNativeContext(env, thiz, context);
}

static void JHwBinder_native_transact(
        JNIEnv * /* env */,
        jobject /* thiz */,
        jint /* code */,
        jobject /* requestObj */,
        jobject /* replyObj */,
        jint /* flags */) {
    CHECK(!"Should not be here");
}

static void JHwBinder_native_registerService(
        JNIEnv *env,
        jobject thiz,
        jstring serviceNameObj) {
    ScopedUtfChars str(env, serviceNameObj);
    if (str.c_str() == nullptr) {
        return;  // NPE will be pending.
    }

    sp<hardware::IBinder> binder = JHwBinder::GetNativeBinder(env, thiz);

    /* TODO(b/33440494) this is not right */
    sp<hidl::base::V1_0::IBase> base = new hidl::base::V1_0::BpHwBase(binder);

    auto manager = hardware::defaultServiceManager();

    if (manager == nullptr) {
        LOG(ERROR) << "Could not get hwservicemanager.";
        signalExceptionForError(env, UNKNOWN_ERROR, true /* canThrowRemoteException */);
        return;
    }

    Return<bool> ret = manager->add(str.c_str(), base);

    bool ok = ret.isOk() && ret;

    if (ok) {
        LOG(INFO) << "HwBinder: Starting thread pool for " << str.c_str();
        ::android::hardware::ProcessState::self()->startThreadPool();
    }

    signalExceptionForError(env, (ok ? OK : UNKNOWN_ERROR), true /* canThrowRemoteException */);
}

static jobject JHwBinder_native_getService(
        JNIEnv *env,
        jclass /* clazzObj */,
        jstring ifaceNameObj,
        jstring serviceNameObj,
        jboolean retry) {

    using ::android::hidl::base::V1_0::IBase;
    using ::android::hardware::details::getRawServiceInternal;

    std::string ifaceName;
    {
        ScopedUtfChars str(env, ifaceNameObj);
        if (str.c_str() == nullptr) {
            return nullptr;  // NPE will be pending.
        }
        ifaceName = str.c_str();
    }

    std::string serviceName;
    {
        ScopedUtfChars str(env, serviceNameObj);
        if (str.c_str() == nullptr) {
            return nullptr;  // NPE will be pending.
        }
        serviceName = str.c_str();
    }

    sp<IBase> ret = getRawServiceInternal(ifaceName, serviceName, retry /* retry */, false /* getStub */);
    sp<hardware::IBinder> service = hardware::toBinder<hidl::base::V1_0::IBase>(ret);

    if (service == NULL) {
        signalExceptionForError(env, NAME_NOT_FOUND);
        return NULL;
    }

    LOG(INFO) << "HwBinder: Starting thread pool for " << serviceName << "::" << ifaceName;
    ::android::hardware::ProcessState::self()->startThreadPool();

    return JHwRemoteBinder::NewObject(env, service);
}

void JHwBinder_native_configureRpcThreadpool(JNIEnv *, jclass,
        jlong maxThreads, jboolean callerWillJoin) {
    CHECK(maxThreads > 0);
    ProcessState::self()->setThreadPoolConfiguration(maxThreads, callerWillJoin /*callerJoinsPool*/);
}

void JHwBinder_native_joinRpcThreadpool() {
    IPCThreadState::self()->joinThreadPool();
}

static void JHwBinder_report_sysprop_change(JNIEnv * /*env*/, jclass /*clazz*/)
{
    report_sysprop_change();
}

static JNINativeMethod gMethods[] = {
    { "native_init", "()J", (void *)JHwBinder_native_init },
    { "native_setup", "()V", (void *)JHwBinder_native_setup },

    { "transact",
        "(IL" PACKAGE_PATH "/HwParcel;L" PACKAGE_PATH "/HwParcel;I)V",
        (void *)JHwBinder_native_transact },

    { "registerService", "(Ljava/lang/String;)V",
        (void *)JHwBinder_native_registerService },

    { "getService", "(Ljava/lang/String;Ljava/lang/String;Z)L" PACKAGE_PATH "/IHwBinder;",
        (void *)JHwBinder_native_getService },

    { "configureRpcThreadpool", "(JZ)V",
        (void *)JHwBinder_native_configureRpcThreadpool },

    { "joinRpcThreadpool", "()V",
        (void *)JHwBinder_native_joinRpcThreadpool },

    { "native_report_sysprop_change", "()V",
        (void *)JHwBinder_report_sysprop_change },
};

namespace android {

int register_android_os_HwBinder(JNIEnv *env) {
    jclass errorClass = FindClassOrDie(env, "java/lang/Error");
    gErrorClass = MakeGlobalRefOrDie(env, errorClass);

    return RegisterMethodsOrDie(env, CLASS_PATH, gMethods, NELEM(gMethods));
}

}  // namespace android
