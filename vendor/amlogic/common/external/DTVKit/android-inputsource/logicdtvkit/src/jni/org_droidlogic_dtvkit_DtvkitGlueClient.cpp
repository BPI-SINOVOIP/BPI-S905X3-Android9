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
#define LOG_TAG "dtvkit-jni"

#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <android_runtime/android_view_Surface.h>
#include <android/native_window.h>
#include <gui/Surface.h>
#include <gui/IGraphicBufferProducer.h>
#include <ui/GraphicBuffer.h>
#include "amlogic/am_gralloc_ext.h"

#include "glue_client.h"
#include "org_droidlogic_dtvkit_DtvkitGlueClient.h"

using namespace android;

static JavaVM   *gJavaVM = NULL;
static jmethodID notifySubtitleCallback;
static jmethodID notifyDvbCallback;
static jmethodID gReadSysfsID;
static jmethodID gWriteSysfsID;
static jobject DtvkitObject;
sp<Surface> mSurface;
sp<NativeHandle> mSourceHandle;

native_handle_t *pTvStream = nullptr;

static JNIEnv* getJniEnv(bool *needDetach) {
    int ret = -1;
    JNIEnv *env = NULL;
    ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (ret < 0) {
        ret = gJavaVM->AttachCurrentThread(&env, NULL);
        if (ret < 0) {
            ALOGE("Can't attach thread ret = %d", ret);
            return NULL;
        }
        *needDetach = true;
    }
    return env;
}

static void DetachJniEnv() {
    int result = gJavaVM->DetachCurrentThread();
    if (result != JNI_OK) {
        ALOGE("thread detach failed: %#x", result);
    }
}


static void sendSubtitleData(int width, int height, int dst_x, int dst_y, int dst_width, int dst_height, uint8_t* data)
{
    //ALOGD("callback sendSubtitleData data = %p", data);
    bool attached = false;
    JNIEnv *env = getJniEnv(&attached);

    if (env != NULL) {
        if (width != 0 || height != 0) {
            ScopedLocalRef<jbyteArray> array (env, env->NewByteArray(width * height * 4));
            if (!array.get()) {
                ALOGE("Fail to new jbyteArray sharememory addr");
                return;
            }
            env->SetByteArrayRegion(array.get(), 0, width * height * 4, (jbyte*)data);
            env->CallVoidMethod(DtvkitObject, notifySubtitleCallback, width, height, dst_x, dst_y, dst_width, dst_height, array.get());
        } else {
            env->CallVoidMethod(DtvkitObject, notifySubtitleCallback, width, height, dst_x, dst_y, dst_width, dst_height, NULL);
        }
    }
    if (attached) {
        DetachJniEnv();
    }
}

static void sendDvbParam(const std::string& resource, const std::string json) {
    ALOGD("-callback senddvbparam resource:%s, json:%s", resource.c_str(), json.c_str());
    bool attached = false;
    JNIEnv *env = getJniEnv(&attached);

    if (env != NULL) {
        //ALOGD("-callback event get ok");
        ScopedLocalRef<jstring> jresource((env), (env)->NewStringUTF(resource.c_str()));
        ScopedLocalRef<jstring> jjson((env),  (env)->NewStringUTF(json.c_str()));
        (env)->CallVoidMethod(DtvkitObject, notifyDvbCallback, jresource.get(), jjson.get());
    }
    if (attached) {
        DetachJniEnv();
    }
}

//notify java read sysfs
void readBySysControl(int ftype, const char *name, char *buf, unsigned int len)
{
    jint f_type = ftype;
    bool attached = false;
    JNIEnv *env = getJniEnv(&attached);

    jstring jvalue = NULL;
    const char *utf_chars = NULL;
    jstring jname = env->NewStringUTF(name);
    jvalue = (jstring)env->CallObjectMethod(DtvkitObject, gReadSysfsID, f_type, jname);
    if (jvalue) {
        utf_chars = env->GetStringUTFChars(jvalue, NULL);
        if (utf_chars) {
            memset(buf, 0, len);
            if (len <= strlen(utf_chars) + 1) {
                memcpy(buf, utf_chars, len - 1);
                buf[strlen(buf)] = '\0';
            }else {
                strcpy(buf, utf_chars);
            }
        }
        env->ReleaseStringUTFChars(jvalue, utf_chars);
        env->DeleteLocalRef(jvalue);
    }
    env->DeleteLocalRef(jname);

    if (attached) {
        DetachJniEnv();
    }
}

//notify java write sysfs
void writeBySysControl(int ftype, const char *name, const char *cmd)
{
    jint f_type = ftype;
    bool attached = false;
    JNIEnv *env = getJniEnv(&attached);
    jstring jname = env->NewStringUTF(name);
    jstring jcmd = env->NewStringUTF(cmd);
    env->CallVoidMethod(DtvkitObject, gWriteSysfsID,f_type, jname, jcmd);
    env->DeleteLocalRef(jname);
    env->DeleteLocalRef(jcmd);
    if (attached) {
        DetachJniEnv();
    }
}


//notify java write sysfs
void write_sysfs_cb(const char *name, const char *cmd)
{
    writeBySysControl( SYSFS, name, cmd);
}

//notify java read sysfs
void read_sysfs_cb(const char *name, char *buf, int len)
{
    readBySysControl(SYSFS,name, buf, len);
}
//notify java write sysfs
void write_prop_cb(const char *name, const char *cmd)
{
    writeBySysControl( PROP, name, cmd);
}

//notify java read sysfs
void read_prop_cb(const char *name, char *buf, int len)
{
    readBySysControl(PROP, name, buf, len);
}

static void connectdtvkit(JNIEnv *env, jclass clazz __unused, jobject obj)
{
    ALOGD("connect dtvkit");
    DtvkitObject = env->NewGlobalRef(obj);
    Glue_client::getInstance()->RegisterRWSysfsCallback((void*)read_sysfs_cb, (void*)write_sysfs_cb);
    Glue_client::getInstance()->RegisterRWPropCallback((void*)read_prop_cb, (void*)write_prop_cb);
    Glue_client::getInstance()->setSignalCallback((SIGNAL_CB)sendDvbParam);
    Glue_client::getInstance()->setDisPatchDrawCallback((DISPATCHDRAW_CB)sendSubtitleData);
    Glue_client::getInstance()->addInterface();
}

static void disconnectdtvkit(JNIEnv *env, jclass clazz __unused)
{
    ALOGD("disconnect dtvkit");
    Glue_client::getInstance()->UnRegisterRWSysfsCallback();
    Glue_client::getInstance()->UnRegisterRWPropCallback();
    env->DeleteGlobalRef(DtvkitObject);
}

static jstring request(JNIEnv *env, jclass clazz __unused, jstring jresource, jstring jjson) {
    const char *resource = env->GetStringUTFChars(jresource, nullptr);
    const char *json = env->GetStringUTFChars(jjson, nullptr);

    ALOGD("request resource[%s]  json[%s]",resource, json);
    std::string result  = Glue_client::getInstance()->request(std::string(resource),std::string( json));
    env->ReleaseStringUTFChars(jresource, resource);
    env->ReleaseStringUTFChars(jjson, json);
    return env->NewStringUTF(result.c_str());
}

static int updateNative(sp<ANativeWindow> nativeWin) {
    ANativeWindowBuffer* buf;

    if (nativeWin.get() == NULL) {
        return 0;
    }

    int err = nativeWin->dequeueBuffer_DEPRECATED(nativeWin.get(), &buf);

    if (err != 0) {
        ALOGE("dequeueBuffer failed: %s (%d)", strerror(-err), -err);
        return -1;
    }

    return nativeWin->queueBuffer_DEPRECATED(nativeWin.get(), buf);
}


static int getTvStream() {
    int ret = -1;
    if (pTvStream == nullptr) {
        pTvStream = am_gralloc_create_sideband_handle(AM_TV_SIDEBAND, 1);
        if (pTvStream == nullptr) {
            ALOGE("tvstream can not be initialized");
            return ret;
        }
    }
    return 0;
}

static void SetSurface(JNIEnv *env, jclass thiz __unused, jobject jsurface) {
    ALOGD("SetSurface");
    sp<Surface> surface;
    if (jsurface) {
        surface = android_view_Surface_getSurface(env, jsurface);

        if (surface == NULL) {
            jniThrowException(env, "java/lang/IllegalArgumentException",
                              "The surface has been released");
            return;
        }
    }

    if (mSurface == surface) {
        // Nothing to do
        return;
    }

    if (mSurface != NULL) {
        if (Surface::isValid(mSurface)) {
            mSurface->setSidebandStream(NULL);
        }
        mSurface.clear();
    }

    if (getTvStream() == 0) {
        mSourceHandle = NativeHandle::create(pTvStream, false);
        mSurface = surface;
        if (mSurface != nullptr) {
            mSurface->setSidebandStream(mSourceHandle);
        }
    }
    return;
}

static JNINativeMethod gMethods[] = {
{
    "nativeconnectdtvkit", "(Lorg/droidlogic/dtvkit/DtvkitGlueClient;)V",
    (void *) connectdtvkit
},
{
    "nativedisconnectdtvkit", "()V",
    (void *) disconnectdtvkit
},
{
    "nativeSetSurface", "(Landroid/view/Surface;)V",
    (void *) SetSurface
},
{
    "nativerequest", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
    (void*) request
},

};


#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_org_droidlogic_dtvkit_DtvkitGlueClient(JNIEnv *env)
{
    static const char *const kClassPathName = "org/droidlogic/dtvkit/DtvkitGlueClient";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);
    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, gMethods, NELEM(gMethods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    GET_METHOD_ID(notifyDvbCallback, clazz, "notifyDvbCallback", "(Ljava/lang/String;Ljava/lang/String;)V");
    GET_METHOD_ID(notifySubtitleCallback, clazz, "notifySubtitleCallback", "(IIIIII[B)V");

    GET_METHOD_ID(gReadSysfsID, clazz, "readBySysControl", "(ILjava/lang/String;)Ljava/lang/String;");
    GET_METHOD_ID(gWriteSysfsID, clazz, "writeBySysControl", "(ILjava/lang/String;Ljava/lang/String;)V");

    return rc;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved __unused)
{
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);
    gJavaVM = vm;
    if (register_org_droidlogic_dtvkit_DtvkitGlueClient(env) < 0)
    {
        ALOGE("Can't register DtvkitGlueClient");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}


