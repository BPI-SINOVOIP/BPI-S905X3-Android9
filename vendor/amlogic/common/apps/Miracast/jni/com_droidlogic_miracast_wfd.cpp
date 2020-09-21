/*
 * Copyright 2012, The Android Open Source Project
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
#define LOG_TAG "amlMiracast-jni"

#include <utils/Log.h>
#include <jni.h>
#include <nativehelper/JNIHelp.h>
#include <android_runtime/AndroidRuntime.h>
#include "sink/AmANetworkSession.h"
#include "sink/WifiDisplaySink.h"
//#include "source/WifiDisplaySource.h"

#include <media/IRemoteDisplay.h>
#include <media/IRemoteDisplayClient.h>

#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <media/IMediaPlayerService.h>
//#include <media/stagefright/DataSource.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>


using namespace android;

sp<ALooper> mSinkLooper = new ALooper;

sp<AmANetworkSession> mSession = new AmANetworkSession;
sp<WifiDisplaySink> mSink;
bool mInit = false;

static android::sp<android::Surface> native_surface;

struct SinkHandler : public AHandler
{
    SinkHandler() {};
protected:
    virtual ~SinkHandler() {};
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    enum
    {
        kWhatSinkNotify,
        kWhatStopCompleted,
    };
};

sp<SinkHandler> mHandler;
static jmethodID notifyWfdError;
static jobject sinkObject;

static void report_wfd_error(void)
{
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    env->CallVoidMethod(sinkObject, notifyWfdError);
}

void SinkHandler::onMessageReceived(const sp<AMessage> &msg)
{
    switch (msg->what())
    {
        case kWhatSinkNotify:
        {
            AString reason;
            msg->findString("reason", &reason);
            ALOGI("SinkHandler received : %s", reason.c_str());
            report_wfd_error();
            break;
        }
        case kWhatStopCompleted:
        {
            ALOGI("SinkHandler received stop completed");
            mSinkLooper->unregisterHandler(mSink->id());
            mSinkLooper->unregisterHandler(mHandler->id());
            mSinkLooper->stop();
            mSink.clear();
            JNIEnv* env = AndroidRuntime::getJNIEnv();
            env->DeleteGlobalRef(sinkObject);
            native_surface.clear();
            mInit = false;
            break;
        }
        default:
            TRESPASS();
    }
}

static android::Surface* getNativeSurface(JNIEnv* env, jobject jsurface)
{
    jclass clazz = env->FindClass("android/view/Surface");
    jfieldID field_surface;
    field_surface = env->GetFieldID(clazz, "mNativeObject", "J");
    if (field_surface == NULL) {
        ALOGE("field_surface get failed");
        return NULL;
    }
    return (android::Surface *)env->GetLongField(jsurface, field_surface);
}

static int connect(const char *sourceHost, int32_t sourcePort)
{
    ProcessState::self()->startThreadPool();
    if (!mInit)
    {
        mSession->start();
        if (native_surface.get()) {
            ALOGE("native_surface is not null we use it");
            mSink = new WifiDisplaySink(mSession, native_surface.get()->getIGraphicBufferProducer());
        } else {
            ALOGE("native_surface is null");
            mSink = new WifiDisplaySink(mSession);
        }
        mHandler = new SinkHandler();
        mSinkLooper->registerHandler(mSink);
        mSinkLooper->registerHandler(mHandler);
        mSink->setSinkHandler(mHandler);
        ALOGI("SinkHandler mSink=%d, mHandler=%d", mSink->id(), mHandler->id());
        if (sourcePort >= 0)
            mSink->start(sourceHost, sourcePort);
        else
            mSink->start(sourceHost);
        mSinkLooper->start(false, true, PRIORITY_DEFAULT);
        mInit = true;
        ALOGI("connected");
    }
    return 0;
}

static void connect_to_wifi_source(JNIEnv *env, jclass clazz, jobject sinkobj, jobject surface, jstring jip, jint jport)
{
   if (mInit) {
        ALOGI("We should be stop WifiDisplaySink first");
        mSession->stop();
        mSink->stop();
        if (native_surface.get())
            native_surface.clear();
        int times = 5;
        do {
            usleep(50000);
            times--;
            if (mInit == false)
                break;
        } while(times-- > 0);
    }
    const char *ip = env->GetStringUTFChars(jip, NULL);
    ALOGI("ref sinkobj");
    sinkObject = env->NewGlobalRef(sinkobj);
    native_surface = getNativeSurface(env, surface);
    if (android::Surface::isValid(native_surface)) {
        ALOGE("native_surface is valid");
    } else {
        ALOGE("native_surface is Invalid");
    }
    ALOGI("connect to wifi source %s:%d native_surface.get() is %p", ip, jport, native_surface.get());
    connect(ip, jport);
    env->ReleaseStringUTFChars(jip, ip);
}

static void connect_to_rtsp_uri(JNIEnv *env, jclass clazz, jstring juri)
{
    const char *ip = env->GetStringUTFChars(juri, NULL);
    ALOGI("connect to rtsp uri %s", ip);
    connect(ip, -1);
    env->ReleaseStringUTFChars(juri, ip);
}

static void resolutionSettings(JNIEnv *env, jclass clazz, jboolean isHD)
{
    unsigned long b = isHD;
    ALOGI("c-boolean: %lu  ", b);
    if (!mSink.get()) {
        ALOGE("mSink is null");
        return;
    }
    if (b)
        mSink->setResolution(WifiDisplaySink::High);
    else
        mSink->setResolution(WifiDisplaySink::Normal);
}

static void disconnectSink(JNIEnv *env, jclass clazz)
{
    ALOGI("disconnect sink mInit:%d", mInit);
    if (mInit == false)
        return;
    ALOGI("stop WifiDisplaySink");
    mSession->stop();
    mSink->stop();
    native_surface.clear();
}

static void setPlay(JNIEnv* env, jclass clazz)
{
    ALOGI("setPlay");
    mSink->setPlay();
}

static void setPause(JNIEnv* env, jclass clazz)
{
    ALOGI("setPause");
    mSink->setPause();
}

static void setTeardown(JNIEnv* env, jclass clazz)
{
    ALOGI("setTeardown");
    mSink->setTeardown();
}

// ----------------------------------------------------------------------------
static JNINativeMethod gMethods[] =
{
    {
        "nativeConnectWifiSource", "(Lcom/droidlogic/miracast/SinkActivity;Landroid/view/Surface;Ljava/lang/String;I)V",
        (void *)connect_to_wifi_source
    },
    //{ "nativeConnectRTSPUri", "(Ljava/lang/String;)V",
    //        (void*) connect_to_rtsp_uri },
    {
        "nativeDisconnectSink", "()V",
        (void *)disconnectSink
    },
    {
        "nativeResolutionSettings", "(Z)V",
        (void *)resolutionSettings
    },
    {
        "nativeSetPlay", "()V",
        (void*)setPlay
    },
    {
        "nativeSetPause", "()V",
        (void*)setPause
    },
    {
        "nativeSetTeardown", "()V",
        (void*) setTeardown
    },
    //{ "nativeSourceStart", "(Ljava/lang/String;)V",
    //        (void*) run_as_source },
    //{ "nativeSourceStop", "()V",
    //        (void*) source_stop },
};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_com_droidlogic_miracast_WiFiDirectActivity(JNIEnv *env)
{
    jint rc;
    static const char *const kClassPathName = "com/droidlogic/miracast/SinkActivity";
    jclass clazz;
    FIND_CLASS(clazz, kClassPathName);
    GET_METHOD_ID(notifyWfdError, clazz, "notifyWfdError", "()V");
    //return jniRegisterNativeMethods(env, kClassPathName, gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
    if (rc = (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0]))) < 0) {
        env->DeleteLocalRef(clazz);
        return -1;
    }

    env->DeleteLocalRef(clazz);
    return 0;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    /*
      if (AndroidRuntime::registerNativeMethods(env, "com/android/server/am/ActivityStack", gMethods, NELEM(gMethods)) < 0) {
          LOGE("Can't register ActivityStack");
          goto bail;
      }*/

    if (register_com_droidlogic_miracast_WiFiDirectActivity(env) < 0)
    {
        ALOGE("Can't register WiFiDirectActivity");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}
