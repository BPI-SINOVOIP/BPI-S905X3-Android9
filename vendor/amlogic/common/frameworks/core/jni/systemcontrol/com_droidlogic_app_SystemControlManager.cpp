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
#define LOG_TAG "systemcontrol-jni"
#include "com_droidlogic_app_SystemControlManager.h"
static sp<SystemControlClient> spSysctrl = NULL;
sp<EventCallback> spEventCB;
static jobject SysCtrlObject;
static jmethodID notifyCallback;
static jmethodID FBCUpgradeCallback;
static jmethodID notifyDisplayModeCallback;
static JavaVM   *g_JavaVM = NULL;

const sp<SystemControlClient>& getSystemControlClient()
{
    if (spSysctrl == NULL)
        spSysctrl = new SystemControlClient();
    return spSysctrl;
}

static JNIEnv* getJniEnv(bool *needDetach) {
    int ret = -1;
    JNIEnv *env = NULL;
    ret = g_JavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (ret < 0) {
        ret = g_JavaVM->AttachCurrentThread(&env, NULL);
        if (ret < 0) {
            ALOGE("Can't attach thread ret = %d", ret);
            return NULL;
        }
        *needDetach = true;
    }
    return env;
}

static void DetachJniEnv() {
    int result = g_JavaVM->DetachCurrentThread();
    if (result != JNI_OK) {
        ALOGE("thread detach failed: %#x", result);
    }
}


void EventCallback::notify (const int event) {
    ALOGD("eventcallback notify event = %d", event);
    bool needDetach = false;
    JNIEnv *env = getJniEnv(&needDetach);
    if (env != NULL && SysCtrlObject != NULL) {
        env->CallVoidMethod(SysCtrlObject, notifyCallback, event);
    } else {
        ALOGE("env or SysCtrlObject is NULL");
    }
    if (needDetach) {
        DetachJniEnv();
    }
}

void EventCallback::notifyFBCUpgrade(int state, int param) {
    bool needDetach = false;
    JNIEnv *env = getJniEnv(&needDetach);
    if (env != NULL && SysCtrlObject != NULL) {
        env->CallVoidMethod(SysCtrlObject, FBCUpgradeCallback, state, param);
    } else {
        ALOGE("g_env or SysCtrlObject is NULL");
    }
    if (needDetach) {
        DetachJniEnv();
    }
}

void EventCallback::onSetDisplayMode(int mode) {
    bool needDetach = false;
    JNIEnv *env = getJniEnv(&needDetach);
    if (env != NULL && SysCtrlObject != NULL) {
        env->CallVoidMethod(SysCtrlObject, notifyDisplayModeCallback, mode);
    } else {
        ALOGE("g_env or SysCtrlObject is NULL");
    }
    if (needDetach) {
        DetachJniEnv();
    }
}

static void ConnectSystemControl(JNIEnv *env, jclass clazz __unused, jobject obj)
{
    ALOGI("Connect System Control");
    const sp<SystemControlClient>& scc = getSystemControlClient();
    //spSysctrl = SystemControlClient::GetInstance();
    if (scc != NULL) {
        spEventCB = new EventCallback();
        scc->setListener(spEventCB);
        scc->setDisplayModeListener(spEventCB);
        SysCtrlObject = env->NewGlobalRef(obj);
    }
}

static jstring GetProperty(JNIEnv* env, jclass clazz __unused, jstring jkey) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        std::string val;
        scc->getProperty(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        return env->NewStringUTF(val.c_str());
    } else {
        return env->NewStringUTF("");
    }
}

static jstring GetPropertyString(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *def = env->GetStringUTFChars(jdef, nullptr);
        std::string val;
        scc->getPropertyString(key, val, def);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jdef, def);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jint GetPropertyInt(JNIEnv* env, jclass clazz __unused, jstring jkey, jint jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        signed int result = scc->getPropertyInt(key, jdef);
        env->ReleaseStringUTFChars(jkey, key);
        return result;
    } else
        return -1;
}

static jlong GetPropertyLong(JNIEnv* env, jclass clazz __unused, jstring jkey, jlong jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        jlong result = scc->getPropertyLong(key, jdef);
        env->ReleaseStringUTFChars(jkey, key);
        return result;
    } else
        return -1;
}

static jboolean GetPropertyBoolean(JNIEnv* env, jclass clazz __unused, jstring jkey, jboolean jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        unsigned char result = scc->getPropertyBoolean(key, jdef);
        env->ReleaseStringUTFChars(jkey, key);
        return result;
    } else
        return -1;
}

static void SetProperty(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        scc->setProperty(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jval, val);
    }
}

static jstring ReadSysfs(JNIEnv* env, jclass clazz __unused, jstring jpath) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        std::string val;
        scc->readSysfs(path, val);
        env->ReleaseStringUTFChars(jpath, path);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jboolean WriteSysfs(JNIEnv* env, jclass clazz __unused, jstring jpath, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        unsigned char result = scc->writeSysfs(path, val);
        env->ReleaseStringUTFChars(jpath, path);
        env->ReleaseStringUTFChars(jval, val);
        return result;
    } else
        return false;
}

static jboolean WriteSysfsSize(JNIEnv* env, jclass clazz __unused, jstring jpath, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        jint* jints = env->GetIntArrayElements(jval, NULL);
        int len = env->GetArrayLength(jval);
        char buf[len+1];
        memset(buf,0,len + 1);
        for (int i = 0; i < len; i++) {
            buf[i] = jints[i];
        }
        buf[len] = '\0';
        unsigned char result = scc->writeSysfs(path, buf, size);
        env->ReleaseStringUTFChars(jpath, path);
        env->ReleaseIntArrayElements(jval, jints, 0);
        return result;
    } else {
        return false;
    }
}

static jboolean WriteUnifyKey(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        unsigned char result = scc->writeUnifyKey(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jval, val);
        return result;
    } else
        return false;
}

static jstring ReadUnifyKey(JNIEnv* env, jclass clazz __unused, jstring jkey) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        std::string val;
        scc->readUnifyKey(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jboolean WritePlayreadyKey(JNIEnv* env, jclass clazz __unused, jstring jkey, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        jint* jints = env->GetIntArrayElements(jval, NULL);
        int len = env->GetArrayLength(jval);
        char buf[len+1];
        memset(buf,0,len + 1);
        for (int i = 0; i < len; i++) {
            buf[i] = jints[i];
        }
        buf[len] = '\0';
        unsigned char result = scc->writePlayreadyKey(key, buf, size);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseIntArrayElements(jval, jints, 0);
        return result;
    } else
        return false;
}

static jint ReadPlayreadyKey(JNIEnv* env, jclass clazz __unused, jstring jpath, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         const char *path = env->GetStringUTFChars(jpath, nullptr);
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readPlayreadyKey(path, buf, size);
         env->ReleaseStringUTFChars(jpath, path);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jint ReadAttestationKey(JNIEnv* env, jclass clazz __unused, jstring jnode, jstring jname, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         const char *node = env->GetStringUTFChars(jnode, nullptr);
         const char *name = env->GetStringUTFChars(jname, nullptr);
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readAttestationKey(node, name, buf, size);
         env->ReleaseStringUTFChars(jnode, node);
         env->ReleaseStringUTFChars(jname, name);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jboolean WriteAttestationKey(JNIEnv* env, jclass clazz __unused, jstring jnode, jstring jname, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         const char *node = env->GetStringUTFChars(jnode, nullptr);
         const char *name = env->GetStringUTFChars(jname, nullptr);
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         unsigned char result = scc->writeAttestationKey(node, name, buf, size);
         env->ReleaseStringUTFChars(jnode, node);
         env->ReleaseStringUTFChars(jname, name);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return false;
}

static jint CheckAttestationKey(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         int result = scc->checkAttestationKey();
         return result;
     } else
         return -1;
}

static jint ReadHdcpRX22Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readHdcpRX22Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jboolean WriteHdcpRX22Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         unsigned char result = scc->writeHdcpRX22Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return false;
}

static jint ReadHdcpRX14Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readHdcpRX14Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jboolean WriteHdcpRX14Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         unsigned char result = scc->writeHdcpRX14Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return false;
}

static jboolean WriteHdcpRXImg(JNIEnv* env, jclass clazz __unused, jstring jpath) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        unsigned char result = scc->writeHdcpRXImg(path);
        env->ReleaseStringUTFChars(jpath, path);
        return result;
    } else
        return false;
}

static jstring GetBootEnv(JNIEnv* env, jclass clazz __unused, jstring jkey) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        std::string val;
        scc->getBootEnv(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static void SetBootEnv(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        scc->setBootEnv(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jval, val);
    }
}

static void setHdrStrategy(JNIEnv* env, jclass clazz __unused, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *val = env->GetStringUTFChars(jval, nullptr);
        scc->setHdrStrategy(val);
        env->ReleaseStringUTFChars(jval, val);
    }
}
static jboolean GetModeSupportDeepColorAttr(JNIEnv* env, jclass clazz __unused, jstring jmode, jstring jcolor) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        const char *color = env->GetStringUTFChars(jcolor, nullptr);
        unsigned char result = scc->getModeSupportDeepColorAttr(mode, color);
        env->ReleaseStringUTFChars(jmode, mode);
        env->ReleaseStringUTFChars(jcolor, color);
        return result;
    } else
        return false;
}

static void LoopMountUnmount(JNIEnv* env, jclass clazz __unused, jboolean isMount, jstring jpath)  {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        scc->loopMountUnmount(isMount, path);
        env->ReleaseStringUTFChars(jpath, path);
    }
}

static void SetMboxOutputMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setMboxOutputMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetSinkOutputMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setSinkOutputMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetDigitalMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setDigitalMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetOsdMouseMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setOsdMouseMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetOsdMousePara(JNIEnv* env __unused, jclass clazz __unused, jint x, jint y, jint w, jint h) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->setOsdMousePara(x, y, w, h);
    }
}

static void SetPosition(JNIEnv* env __unused, jclass clazz __unused, jint left, jint top, jint width, jint height)  {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->setPosition(left, top, width, height);
    }
}

static jintArray GetPosition(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jintArray result;
    result = env->NewIntArray(4);
    jint curPosition[4] = { 0, 0, 1280, 720 };
    env->SetIntArrayRegion(result, 0, 4, curPosition);
    if (scc != NULL) {
        int outx = 0;
        int outy = 0;
        int outw = 0;
        int outh = 0;
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->getPosition(mode, outx, outy, outw, outh);
        curPosition[0] = outx;
        curPosition[1] = outy;
        curPosition[2] = outw;
        curPosition[3] = outh;
        env->SetIntArrayRegion(result, 0, 4, curPosition);
        env->ReleaseStringUTFChars(jmode, mode);
    }
    return result;
}

static jstring GetDeepColorAttr(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        std::string val;
        scc->getDeepColorAttr(mode, val);
        env->ReleaseStringUTFChars(jmode, mode);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jlong ResolveResolutionValue(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    jlong value = 0;
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        value = scc->resolveResolutionValue(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
    return value;
}

static jstring IsTvSupportDolbyVision(JNIEnv* env, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        std::string mode;
        scc->isTvSupportDolbyVision(mode);
        return env->NewStringUTF(mode.c_str());
    } else
        return env->NewStringUTF("");
}

static void InitDolbyVision(JNIEnv* env __unused, jclass clazz __unused, jint state) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->initDolbyVision(state);
    }
}

static void SetDolbyVisionEnable(JNIEnv* env __unused, jclass clazz __unused, jint state) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->setDolbyVisionEnable(state);
    }
}

static void SaveDeepColorAttr(JNIEnv* env, jclass clazz __unused, jstring jmode, jstring jdcValue) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        const char *dcValue = env->GetStringUTFChars(jdcValue, nullptr);
        scc->saveDeepColorAttr(mode, dcValue);
        env->ReleaseStringUTFChars(jmode, mode);
        env->ReleaseStringUTFChars(jdcValue, dcValue);
    }
}

static void SetHdrMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setHdrMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetSdrMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setSdrMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static jint GetDolbyVisionType(JNIEnv* env __unused, jclass clazz __unused) {
    jint result = -1;
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL)
        result = scc->getDolbyVisionType();
    return result;
}

static void SetGraphicsPriority(JNIEnv* env, jclass clazz __unused, jstring jmode) {
   const sp<SystemControlClient>& scc = getSystemControlClient();
   if (scc != NULL) {
       const char *mode = env->GetStringUTFChars(jmode, nullptr);
       scc->setGraphicsPriority(mode);
       env->ReleaseStringUTFChars(jmode, mode);
   }
}

static jstring GetGraphicsPriority(JNIEnv* env, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        std::string val;
        scc->getGraphicsPriority(val);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static void SetAppInfo(JNIEnv* env, jclass clazz __unused, jstring jpkg, jstring jcls, jobjectArray jproc) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    std::vector<std::string> proc;
    if (scc != NULL && jproc != NULL) {
        const char *pkg = env->GetStringUTFChars(jpkg, nullptr);
        const char *cls = env->GetStringUTFChars(jcls, nullptr);
        size_t count = env->GetArrayLength(jproc);
        proc.resize(count);
        for (size_t i = 0; i < count; ++i) {
            jstring jstr = (jstring)env->GetObjectArrayElement(jproc, i);
            const char *cString = env->GetStringUTFChars(jstr, nullptr);
            proc[i] = cString;
            env->ReleaseStringUTFChars(jstr, cString);
        }

        scc->setAppInfo(pkg, cls, proc);
        env->ReleaseStringUTFChars(jpkg, pkg);
        env->ReleaseStringUTFChars(jcls, cls);
    }
}

//3D
static jint Set3DMode(JNIEnv* env, jclass clazz __unused, jstring jmode3d) {
    int result = -1;
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode3d = env->GetStringUTFChars(jmode3d, nullptr);
        result = scc->set3DMode(mode3d);
        env->ReleaseStringUTFChars(jmode3d, mode3d);
    }
    return result;
}

static void Init3DSetting(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL)
        scc->init3DSetting();
}

static jint GetVideo3DFormat(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getVideo3DFormat();
    return result;
}

static jint GetDisplay3DTo2DFormat(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getDisplay3DTo2DFormat();
    return result;
}

static jboolean SetDisplay3DTo2DFormat(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->setDisplay3DTo2DFormat(format);
    return result;
}

static jboolean SetDisplay3DFormat(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->setDisplay3DFormat(format);
    return result;
}

static jint GetDisplay3DFormat(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getDisplay3DFormat();
    return result;
}

static jboolean SetOsd3DFormat(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->setOsd3DFormat(format);
    return result;
}

static jboolean Switch3DTo2D(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->switch3DTo2D(format);
    return result;
}

static jboolean Switch2DTo3D(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->switch2DTo3D(format);
    return result;
}

static jint SetPQmode(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave, jint is_autoswitch) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setPQmode(mode, isSave, is_autoswitch);
    return result;
}

static jint GetPQmode(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getPQmode();
    return result;
}

static jint SavePQmode(JNIEnv* env __unused, jclass clazz __unused, jint mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->savePQmode(mode);
    return result;
}

static jint SetColorTemperature(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setColorTemperature(mode, isSave);
    return result;
}

static jint GetColorTemperature(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getColorTemperature();
    return result;
}

static jint SaveColorTemperature(JNIEnv* env __unused, jclass clazz __unused, jint mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveColorTemperature(mode);
    return result;
}

static jint SetColorTemperatureUserParam(JNIEnv* env __unused, jclass clazz __unused, jint mode,
                jint isSave, jint type, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setColorTemperatureUserParam(mode, isSave, type, value);
    return result;
}

static jobject GetColorTemperatureUserParam(JNIEnv *env, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jobject whiteBalanceParam = NULL;
    if (scc != NULL) {
        tcon_rgb_ogo_t param = scc->getColorTemperatureUserParam();
        jclass objectClass = (env)->FindClass("com/droidlogic/app/SystemControlManager$WhiteBalanceParams");
        jmethodID consID = (env)->GetMethodID(objectClass, "<init>", "()V");

        jfieldID jgainR = (env)->GetFieldID(objectClass, "r_gain", "I");
        jfieldID jgainG = (env)->GetFieldID(objectClass, "g_gain", "I");
        jfieldID jgainB = (env)->GetFieldID(objectClass, "b_gain", "I");
        jfieldID joffsetR = (env)->GetFieldID(objectClass, "r_offset", "I");
        jfieldID joffsetG = (env)->GetFieldID(objectClass, "g_offset", "I");
        jfieldID joffsetB = (env)->GetFieldID(objectClass, "b_offset", "I");
        whiteBalanceParam = (env)->NewObject(objectClass, consID);
        (env)->SetIntField(whiteBalanceParam, jgainR, param.r_gain);
        (env)->SetIntField(whiteBalanceParam, jgainG, param.g_gain);
        (env)->SetIntField(whiteBalanceParam, jgainB, param.b_gain);
        (env)->SetIntField(whiteBalanceParam, joffsetR, param.r_post_offset);
        (env)->SetIntField(whiteBalanceParam, joffsetG, param.g_post_offset);
        (env)->SetIntField(whiteBalanceParam, joffsetB, param.b_post_offset);
    }
    return whiteBalanceParam;

}

static jint SetBrightness(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setBrightness(value, isSave);
    return result;
}

static jint GetBrightness(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getBrightness();
    return result;
}

static jint SaveBrightness(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveBrightness(value);
    return result;
}

static jint SetContrast(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setContrast(value, isSave);
    return result;
}

static jint GetContrast(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getContrast();
    return result;
}

static jint SaveContrast(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveContrast(value);
    return result;
}

static jint SetSaturation(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setSaturation(value, isSave);
    return result;
}

static jint GetSaturation(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getSaturation();
    return result;
}

static jint SaveSaturation(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveSaturation(value);
    return result;
}

static jint SetHue(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setHue(value, isSave);
    return result;
}

static jint GetHue(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getHue();
    return result;
}

static jint SaveHue(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveHue(value);
    return result;
}

static jint SetSharpness(JNIEnv* env __unused, jclass clazz __unused, jint value, jint is_enable, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setSharpness(value, is_enable, isSave);
    return result;
}

static jint GetSharpness(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getSharpness();
    return result;
}

static jint SaveSharpness(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveSharpness(value);
    return result;
}

static jint SetNoiseReductionMode(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setNoiseReductionMode(value, isSave);
    return result;
}

static jint GetNoiseReductionMode(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getNoiseReductionMode();
    return result;
}

static jint SaveNoiseReductionMode(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveNoiseReductionMode(value);
    return result;
}

static jint SetEyeProtectionMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input, jint enable, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setEyeProtectionMode(source_input, enable, isSave);
    return result;
}

static jint GetEyeProtectionMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getEyeProtectionMode(source_input);
    return result;
}

static jint SetGammaValue(JNIEnv* env __unused, jclass clazz __unused, jint gamma_curve, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setGammaValue(gamma_curve, isSave);
    return result;
}

static jint GetGammaValue(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getGammaValue();
    return result;
}

static jint SetDisplayMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setDisplayMode(source_input, mode, isSave);
    return result;
}

static jint GetDisplayMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getDisplayMode(source_input);
    return result;
}

static jint SaveDisplayMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input, jint mode)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveDisplayMode(source_input, mode);
    return result;
}

static jint SetBacklight(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setBacklight(value, isSave);
    return result;
}

static jint GetBacklight(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getBacklight();
    return result;
}

static jint SaveBacklight(JNIEnv* env __unused, jclass clazz __unused, jint value)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveBacklight(value);
    return result;
}

static jint CheckLdimExist(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->checkLdimExist();
    return result;
}

static jint SetDynamicBacklight(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setDynamicBacklight(mode, isSave);
    return result;
}

static jint GetDynamicBacklight(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getDynamicBacklight();
    return result;
}

static jint SetLocalContrastMode(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setLocalContrastMode(mode, isSave);
    return result;
}

static jint GetLocalContrastMode(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getLocalContrastMode();
    return result;
}

static jint SetColorBaseMode(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setColorBaseMode(mode, isSave);
    return result;
}

static jint GetColorBaseMode(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getColorBaseMode();
    return result;
}

static jint SysSSMReadNTypes(JNIEnv* env __unused, jclass clazz __unused, jint id, jint data_len, jint offset) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->sysSSMReadNTypes(id, data_len, offset);
    return result;
}

static jint SysSSMWriteNTypes(JNIEnv* env __unused, jclass clazz __unused, jint id, jint data_len, jint data_buf, jint offset) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->sysSSMWriteNTypes(id, data_len, data_buf, offset);
    return result;
}

static jint GetActualAddr(JNIEnv* env __unused, jclass clazz __unused, jint id) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getActualAddr(id);
    return result;
}

static jint GetActualSize(JNIEnv* env __unused, jclass clazz __unused, jint id) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getActualSize(id);
    return result;
}

static jint SSMRecovery(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->SSMRecovery();
    return result;
}

static jint SetCVD2Values(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setCVD2Values();
    return result;
}

static jint GetSSMStatus(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getSSMStatus();
    return result;
}

static jint SetCurrentSourceInfo(JNIEnv* env __unused, jclass clazz __unused, jint sourceInput, jint sigFmt, jint transFmt) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setCurrentSourceInfo(sourceInput, sigFmt, transFmt);
    return result;
}

static jintArray GetCurrentSourceInfo(JNIEnv* env, jclass clazz __unused)
{
    jintArray result;
    result = env->NewIntArray(3);
    jint curSourceInfo[3] = { 10, 1034, 0 };//default MPEG 1920*1080p source
    env->SetIntArrayRegion(result, 0, 3, curSourceInfo);

    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        int sourceInput = 0;
        int sigFmt = 0;
        int transFmt = 0;
        scc->getCurrentSourceInfo(sourceInput, sigFmt, transFmt);
        curSourceInfo[0] = sourceInput;
        curSourceInfo[1] = sigFmt;
        curSourceInfo[2] = transFmt;
        env->SetIntArrayRegion(result, 0, 3, curSourceInfo);
    }

    return result;
}

static jobject GetPQDatabaseInfo(JNIEnv* env, jclass clazz __unused, jint dataBaseName) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jobject jpqdatabaseinfo = NULL;
    if (scc != NULL) {
        PQDatabaseInfo databaseinfo = scc->getPQDatabaseInfo(dataBaseName);
        jclass objectClass = env->FindClass("com/droidlogic/app/SystemControlManager$PQDatabaseInfo");
        jmethodID consID = (env)->GetMethodID(objectClass, "<init>", "()V");
        jfieldID jToolVersion = (env)->GetFieldID(objectClass, "ToolVersion", "Ljava/lang/String;");
        jfieldID jProjectVersion = (env)->GetFieldID(objectClass, "ProjectVersion", "Ljava/lang/String;");
        jfieldID jGenerateTime = (env)->GetFieldID(objectClass, "GenerateTime", "Ljava/lang/String;");

        jpqdatabaseinfo = (env)->NewObject(objectClass, consID);
        (env)->SetObjectField(jpqdatabaseinfo, jToolVersion, (env)->NewStringUTF(databaseinfo .ToolVersion.c_str()));
        (env)->SetObjectField(jpqdatabaseinfo, jProjectVersion, (env)->NewStringUTF(databaseinfo .ProjectVersion.c_str()));
        (env)->SetObjectField(jpqdatabaseinfo, jGenerateTime, (env)->NewStringUTF(databaseinfo .GenerateTime.c_str()));
    }
    return jpqdatabaseinfo;
}

static jint SetDtvKitSourceEnable(JNIEnv* env __unused, jclass clazz __unused, jint isEnable)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
       result = scc->setDtvKitSourceEnable(isEnable);
    return result;
}

//FBC
static jint StartUpgradeFBC(JNIEnv* env, jclass clazz __unused, jstring jfile_name, jint mode, jint upgrade_blk_size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL) {
        const char *file_name = env->GetStringUTFChars(jfile_name, nullptr);
        result = spSysctrl->StartUpgradeFBC(file_name, mode, upgrade_blk_size);
        env->ReleaseStringUTFChars(jfile_name, file_name);
    }
    return result;
}

static jint FactorySetPQMode_Brightness(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetPQMode_Brightness(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
    return result;
}

static jint FactoryGetPQMode_Brightness(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetPQMode_Brightness(inputSrc, sig_fmt, trans_fmt, pq_mode);
    return result;
}

static jint FactorySetPQMode_Contrast(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetPQMode_Contrast(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
    return result;
}

static jint FactoryGetPQMode_Contrast(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetPQMode_Contrast(inputSrc, sig_fmt, trans_fmt, pq_mode);
    return result;
}

static jint FactorySetPQMode_Saturation(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetPQMode_Saturation(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
    return result;
}

static jint FactoryGetPQMode_Saturation(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetPQMode_Saturation(inputSrc, sig_fmt, trans_fmt, pq_mode);
    return result;
}

static jint FactorySetPQMode_Hue(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetPQMode_Hue(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
    return result;
}

static jint FactoryGetPQMode_Hue(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetPQMode_Hue(inputSrc, sig_fmt, trans_fmt, pq_mode);
    return result;
}

static jint FactorySetPQMode_Sharpness(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetPQMode_Sharpness(inputSrc, sig_fmt, trans_fmt, pq_mode, value);
    return result;
}

static jint FactoryGetPQMode_Sharpness(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt,
jint pq_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetPQMode_Sharpness(inputSrc, sig_fmt, trans_fmt, pq_mode);
    return result;
}

static jint FactoryResetPQMode(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryResetPQMode();
    return result;
}

static jint FactoryResetColorTemp(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryResetColorTemp();
    return result;
}

static jint FactorySetParamsDefault(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetParamsDefault();
    return result;
}

static jint FactorySetNolineParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt,
jint type, jint osd0_value, jint osd25_value,  jint osd50_value, jint osd75_value, jint osd100_value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetNolineParams(inputSrc, sigFmt, transFmt, type, osd0_value, osd25_value,
                                            osd50_value, osd75_value, osd100_value);
    return result;
}

static jobject FactoryGetNolineParams(JNIEnv* env, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint type) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jobject jnolineParam = NULL;
    if (scc != NULL) {
        noline_params_t nolineParam = scc->factoryGetNolineParams(inputSrc, sigFmt, transFmt, type);
        jclass objectClass = env->FindClass("com/droidlogic/app/SystemControlManager$noline_params_t");
        jmethodID consID = env->GetMethodID(objectClass, "<init>", "()V");
        jfieldID josd0   = env->GetFieldID(objectClass, "osd0", "I");
        jfieldID josd25  = env->GetFieldID(objectClass, "osd25", "I");
        jfieldID josd50  = env->GetFieldID(objectClass, "osd50", "I");
        jfieldID josd75  = env->GetFieldID(objectClass, "osd75", "I");
        jfieldID josd100 = env->GetFieldID(objectClass, "osd100", "I");

        jnolineParam = env->NewObject(objectClass, consID);
        env->SetIntField(jnolineParam, josd0, nolineParam.osd0);
        env->SetIntField(jnolineParam, josd25, nolineParam.osd25);
        env->SetIntField(jnolineParam, josd50, nolineParam.osd50);
        env->SetIntField(jnolineParam, josd75, nolineParam.osd75);
        env->SetIntField(jnolineParam, josd100, nolineParam.osd100);
    }
    return jnolineParam;
}

static jint FactorySetOverscan(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint he_value, jint hs_value, jint ve_value, jint vs_value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetOverscan(inputSrc, sigFmt, transFmt, he_value, hs_value, ve_value, vs_value);
    return result;
}

static jobject FactoryGetOverscan(JNIEnv* env, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jobject joverscanParam = NULL;
    if (scc != NULL) {
        tvin_cutwin_t overscanParam = scc->factoryGetOverscan(inputSrc, sigFmt, transFmt);
        jclass objectClass = env->FindClass("com/droidlogic/app/SystemControlManager$tvin_cutwin_t");
        jmethodID consID = env->GetMethodID(objectClass, "<init>", "()V");
        jfieldID jhs   = env->GetFieldID(objectClass, "hs", "I");
        jfieldID jhe   = env->GetFieldID(objectClass, "he", "I");
        jfieldID jvs   = env->GetFieldID(objectClass, "vs", "I");
        jfieldID jve   = env->GetFieldID(objectClass, "ve", "I");

        joverscanParam = env->NewObject(objectClass, consID);
        env->SetIntField(joverscanParam, jhs, overscanParam.hs);
        env->SetIntField(joverscanParam, jhe, overscanParam.he);
        env->SetIntField(joverscanParam, jvs, overscanParam.vs);
        env->SetIntField(joverscanParam, jve, overscanParam.ve);
    }

    return joverscanParam;
}

static jint SetwhiteBalanceGainRed(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setwhiteBalanceGainRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
    return result;
}

static jint SetwhiteBalanceGainGreen(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setwhiteBalanceGainGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
    return result;
}

static jint SetwhiteBalanceGainBlue(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setwhiteBalanceGainBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
    return result;
}

static jint GetwhiteBalanceGainRed(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getwhiteBalanceGainRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
    return result;
}

static jint GetwhiteBalanceGainGreen(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getwhiteBalanceGainGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
    return result;
}

static jint GetwhiteBalanceGainBlue(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getwhiteBalanceGainBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
    return result;
}


static jint SetwhiteBalanceOffsetRed(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setwhiteBalanceOffsetRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
    return result;
}

static jint SetwhiteBalanceOffsetGreen(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setwhiteBalanceOffsetGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
    return result;
}

static jint SetwhiteBalanceOffsetBlue(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colortemp_mode, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setwhiteBalanceOffsetBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode, value);
    return result;
}

static jint GetwhiteBalanceOffsetRed(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint
trans_fmt, jint colortemp_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getwhiteBalanceOffsetRed(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
    return result;
}

static jint GetwhiteBalanceOffsetGreen(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint
trans_fmt, jint colortemp_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getwhiteBalanceOffsetGreen(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
    return result;
}

static jint GetwhiteBalanceOffsetBlue(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint
trans_fmt, jint colortemp_mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getwhiteBalanceOffsetBlue(inputSrc, sig_fmt, trans_fmt, colortemp_mode);
    return result;
}

static jint SaveWhiteBalancePara(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint colorTemp_mode,
jint r_gain, jint g_gain, jint b_gain, jint r_offset, jint g_offset, jint b_offset) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->saveWhiteBalancePara(inputSrc, sig_fmt, trans_fmt, colorTemp_mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
    return result;
}

static jint FactoryfactoryGetColorTemperatureParams(JNIEnv* env __unused, jclass clazz __unused, jint colorTemp_mode){
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryfactoryGetColorTemperatureParams(colorTemp_mode);
    return result;
}

static jint FactorySSMRestore(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySSMRestore();
    return result;
}

static jint FactoryResetNonlinear(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryResetNonlinear();
    return result;
}

static jint FactorySetGamma(JNIEnv* env __unused, jclass clazz __unused, jint gamma_r, jint gamma_g, jint gamma_b) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetGamma(gamma_r, gamma_g, gamma_b);
    return result;
}

static jint GetRGBPattern(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getRGBPattern();
    return result;
}

static jint SetRGBPattern(JNIEnv* env __unused, jclass clazz __unused, jint r, jint g, jint b) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setRGBPattern(r, g, b);
    return result;
}

static jint FactorySetDDRSSC(JNIEnv* env __unused, jclass clazz __unused, jint step) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetDDRSSC(step);
    return result;
}

static jint FactoryGetDDRSSC(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetDDRSSC();
    return result;
}

static jint FactorySetLVDSSSC(JNIEnv* env __unused, jclass clazz __unused, jint step) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetLVDSSSC(step);
    return result;
}

static jint FactoryGetLVDSSSC(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetLVDSSSC();
    return result;
}

static jint whiteBalanceGrayPatternOpen(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->whiteBalanceGrayPatternOpen();
    return result;
}

static jint WhiteBalanceGrayPatternClose(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->whiteBalanceGrayPatternClose();
    return result;
}

static jint WhiteBalanceGrayPatternSet(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->whiteBalanceGrayPatternSet(value);
    return result;
}

static jint WhiteBalanceGrayPatternGet(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->whiteBalanceGrayPatternGet();
    return result;
}

static jint FactorySetHdrMode(JNIEnv* env __unused, jclass clazz __unused, jint mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetHdrMode(mode);
    return result;
}

static jint FactoryGetHdrMode(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetHdrMode();
    return result;
}

static jint SetDnlpParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint level) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->setDnlpParams(inputSrc, sigFmt, transFmt, level);
    return result;
}

static jint GetDnlpParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getDnlpParams(inputSrc, sigFmt, transFmt);
    return result;
}

static jint FactorySetDnlpParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint level, jint final_gain) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetDnlpParams(inputSrc, sigFmt, transFmt, level, final_gain);
    return result;
}

static jint FactoryGetDnlpParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint level) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetDnlpParams(inputSrc, sigFmt, transFmt, level);
    return result;
}

static jint FactorySetBlackExtRegParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint val) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetBlackExtRegParams(inputSrc, sigFmt, transFmt, val);
    return result;
}

static jint FactoryGetBlackExtRegParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetBlackExtRegParams(inputSrc, sigFmt, transFmt);
    return result;
}

static jint FactorySetColorParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint color_type, jint color_param, jint val) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetColorParams(inputSrc, sigFmt, transFmt, color_type, color_param, val);
    return result;
}

static jint FactoryGetColorParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sigFmt, jint transFmt, jint color_type, jint color_param) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetColorParams(inputSrc, sigFmt, transFmt, color_type, color_param);
    return result;
}

static jint FactorySetNoiseReductionParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint nr_mode, jint param_type, jint val) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetNoiseReductionParams(inputSrc, sig_fmt, trans_fmt, nr_mode, param_type, val);
    return result;
}

static jint FactoryGetNoiseReductionParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint nr_mode, jint param_type) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetNoiseReductionParams(inputSrc, sig_fmt, trans_fmt, nr_mode, param_type);
    return result;
}

static jint FactorySetCTIParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint param_type, jint val) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetCTIParams(inputSrc, sig_fmt, trans_fmt, param_type, val);
    return result;
}

static jint FactoryGetCTIParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint param_type) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetCTIParams(inputSrc, sig_fmt, trans_fmt, param_type);
    return result;
}


static jint FactorySetDecodeLumaParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint param_type, jint val) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetDecodeLumaParams(inputSrc, sig_fmt, trans_fmt, param_type, val);
    return result;
}

static jint FactoryGetDecodeLumaParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint param_type) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetDecodeLumaParams(inputSrc, sig_fmt, trans_fmt, param_type);
    return result;
}

static jint FactorySetSharpnessParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint isHD, jint param_type, jint val) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factorySetSharpnessParams(inputSrc, sig_fmt, trans_fmt, isHD, param_type, val);
    return result;
}

static jint FactoryGetSharpnessParams(JNIEnv* env __unused, jclass clazz __unused, jint inputSrc, jint sig_fmt, jint trans_fmt, jint isHD, jint param_type) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->factoryGetSharpnessParams(inputSrc, sig_fmt, trans_fmt, isHD, param_type);
    return result;
}


static JNINativeMethod SystemControl_Methods[] = {
{"native_ConnectSystemControl", "(Lcom/droidlogic/app/SystemControlManager;)V", (void *) ConnectSystemControl },
{"native_GetProperty", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetProperty },
{"native_GetPropertyString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void *) GetPropertyString },
{"native_GetPropertyInt", "(Ljava/lang/String;I)I", (void *) GetPropertyInt },
{"native_GetPropertyLong", "(Ljava/lang/String;J)J", (void *) GetPropertyLong },
{"native_GetPropertyBoolean", "(Ljava/lang/String;Z)Z", (void *) GetPropertyBoolean },
{"native_SetProperty", "(Ljava/lang/String;Ljava/lang/String;)V", (void *) SetProperty },
{"native_ReadSysfs", "(Ljava/lang/String;)Ljava/lang/String;", (void *) ReadSysfs },
{"native_WriteSysfs", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) WriteSysfs },
{"native_WriteSysfsSize", "(Ljava/lang/String;[II)Z", (void *) WriteSysfsSize },
{"native_WriteUnifyKey", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) WriteUnifyKey },
{"native_ReadUnifyKey", "(Ljava/lang/String;)Ljava/lang/String;", (void *) ReadUnifyKey },
{"native_WritePlayreadyKey", "(Ljava/lang/String;[II)Z", (void *) WritePlayreadyKey },
{"native_ReadPlayreadyKey", "(Ljava/lang/String;[II)I", (void *) ReadPlayreadyKey },
{"native_ReadAttestationKey", "(Ljava/lang/String;Ljava/lang/String;[II)I", (void *) ReadAttestationKey },
{"native_WriteAttestationKey", "(Ljava/lang/String;Ljava/lang/String;[II)Z", (void *) WriteAttestationKey },
{"native_CheckAttestationKey", "()I", (void *) CheckAttestationKey },
{"native_ReadHdcpRX22Key", "([II)I", (void *) ReadHdcpRX22Key },
{"native_WriteHdcpRX22Key", "([II)Z", (void *) WriteHdcpRX22Key },
{"native_ReadHdcpRX14Key", "([II)I", (void *) ReadHdcpRX14Key },
{"native_WriteHdcpRX14Key", "([II)Z", (void *) WriteHdcpRX14Key },
{"native_WriteHdcpRXImg", "(Ljava/lang/String;)Z", (void *) WriteHdcpRXImg },
{"native_GetBootEnv", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetBootEnv },
{"native_SetBootEnv", "(Ljava/lang/String;Ljava/lang/String;)V", (void *) SetBootEnv },
{"native_LoopMountUnmount", "(ZLjava/lang/String;)V", (void *) LoopMountUnmount },
{"native_SetMboxOutputMode", "(Ljava/lang/String;)V", (void *) SetMboxOutputMode },
{"native_SetSinkOutputMode", "(Ljava/lang/String;)V", (void *) SetSinkOutputMode },
{"native_SetDigitalMode", "(Ljava/lang/String;)V", (void *) SetDigitalMode },
{"native_SetOsdMouseMode", "(Ljava/lang/String;)V", (void *) SetOsdMouseMode },
{"native_SetOsdMousePara", "(IIII)V", (void *) SetOsdMousePara },
{"native_SetPosition", "(IIII)V", (void *) SetPosition },
{"native_GetPosition", "(Ljava/lang/String;)[I", (void *) GetPosition },
{"native_GetDeepColorAttr", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetDeepColorAttr },
{"native_ResolveResolutionValue", "(Ljava/lang/String;)J", (void *) ResolveResolutionValue },
{"native_IsTvSupportDolbyVision", "()Ljava/lang/String;", (void *) IsTvSupportDolbyVision },
{"native_InitDolbyVision", "(I)V", (void *) InitDolbyVision },
{"native_SetDolbyVisionEnable", "(I)V", (void *) SetDolbyVisionEnable },
{"native_SaveDeepColorAttr", "(Ljava/lang/String;Ljava/lang/String;)V", (void *) SaveDeepColorAttr },
{"native_SetHdrMode", "(Ljava/lang/String;)V", (void *) SetHdrMode },
{"native_SetSdrMode", "(Ljava/lang/String;)V", (void *) SetSdrMode },
{"native_GetDolbyVisionType", "()I", (void *) GetDolbyVisionType },
{"native_SetGraphicsPriority", "(Ljava/lang/String;)V", (void *) SetGraphicsPriority },
{"native_GetGraphicsPriority", "()Ljava/lang/String;", (void *) GetGraphicsPriority },
{"native_SetAppInfo", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V", (void *) SetAppInfo },
{"native_Set3DMode", "(Ljava/lang/String;)I", (void *) Set3DMode },
{"native_Init3DSetting", "()V", (void *) Init3DSetting },
{"native_GetVideo3DFormat", "()I", (void *) GetVideo3DFormat },
{"native_GetDisplay3DTo2DFormat", "()I", (void *) GetDisplay3DTo2DFormat },
{"native_SetDisplay3DTo2DFormat", "(I)Z", (void *) SetDisplay3DTo2DFormat },
{"native_SetDisplay3DFormat", "(I)Z", (void *) SetDisplay3DFormat },
{"native_GetDisplay3DFormat", "()I", (void *) GetDisplay3DFormat },
{"native_SetOsd3DFormat", "(I)Z", (void *) SetOsd3DFormat },
{"native_Switch3DTo2D", "(I)Z", (void *) Switch3DTo2D },
{"native_Switch2DTo3D", "(I)Z", (void *) Switch2DTo3D },
{"native_SetPQmode", "(III)I", (void *) SetPQmode },
{"native_GetPQmode", "()I", (void *) GetPQmode },
{"native_SavePQmode", "(I)I", (void *) SavePQmode },
{"native_SetColorTemperature", "(II)I", (void *) SetColorTemperature },
{"native_GetColorTemperature", "()I", (void *) GetColorTemperature },
{"native_SaveColorTemperature", "(I)I", (void *) SaveColorTemperature },
{"native_SetColorTemperatureUserParam", "(IIII)I", (void *) SetColorTemperatureUserParam},
{"native_GetColorTemperatureUserParam", "()Lcom/droidlogic/app/SystemControlManager$WhiteBalanceParams;", (void *) GetColorTemperatureUserParam},
{"native_SetBrightness", "(II)I", (void *) SetBrightness },
{"native_GetBrightness", "()I", (void *) GetBrightness },
{"native_SaveBrightness", "(I)I", (void *) SaveBrightness },
{"native_SetContrast", "(II)I", (void *) SetContrast },
{"native_GetContrast", "()I", (void *) GetContrast },
{"native_SaveContrast", "(I)I", (void *) SaveContrast },
{"native_SetSaturation", "(II)I", (void *) SetSaturation },
{"native_GetSaturation", "()I", (void *) GetSaturation },
{"native_SaveSaturation", "(I)I", (void *) SaveSaturation },
{"native_SetHue", "(II)I", (void *) SetHue },
{"native_GetHue", "()I", (void *) GetHue },
{"native_SaveHue", "(I)I", (void *) SaveHue },
{"native_SetSharpness", "(III)I", (void *) SetSharpness },
{"native_GetSharpness", "()I", (void *) GetSharpness },
{"native_SaveSharpness", "(I)I", (void *) SaveSharpness },
{"native_SetNoiseReductionMode", "(II)I", (void *) SetNoiseReductionMode },
{"native_GetNoiseReductionMode", "()I", (void *) GetNoiseReductionMode },
{"native_SaveNoiseReductionMode", "(I)I", (void *) SaveNoiseReductionMode },
{"native_SetEyeProtectionMode", "(III)I", (void *) SetEyeProtectionMode },
{"native_GetEyeProtectionMode", "(I)I", (void *) GetEyeProtectionMode },
{"native_SetGammaValue", "(II)I", (void *) SetGammaValue },
{"native_GetGammaValue", "()I", (void *) GetGammaValue },
{"native_SetDisplayMode", "(III)I", (void *) SetDisplayMode },
{"native_GetDisplayMode", "(I)I", (void *) GetDisplayMode },
{"native_SaveDisplayMode", "(II)I", (void *) SaveDisplayMode },
{"native_SetBacklight", "(II)I", (void *) SetBacklight },
{"native_GetBacklight", "()I", (void *) GetBacklight },
{"native_SaveBacklight", "(I)I", (void *) SaveBacklight },
{"native_CheckLdimExist", "()I", (void *) CheckLdimExist },
{"native_SetDynamicBacklight", "(II)I", (void *) SetDynamicBacklight },
{"native_GetDynamicBacklight", "()I", (void *) GetDynamicBacklight },
{"native_SetLocalContrastMode", "(II)I", (void *) SetLocalContrastMode },
{"native_GetLocalContrastMode", "()I", (void *) GetLocalContrastMode },
{"native_SetColorBaseMode", "(II)I", (void *) SetColorBaseMode },
{"native_GetColorBaseMode", "()I", (void *) GetColorBaseMode },
{"native_SysSSMReadNTypes", "(III)I", (void *) SysSSMReadNTypes },
{"native_SysSSMWriteNTypes", "(IIII)I", (void *) SysSSMWriteNTypes },
{"native_GetActualAddr", "(I)I", (void *) GetActualAddr },
{"native_GetActualSize", "(I)I", (void *) GetActualSize },
{"native_SSMRecovery", "()I", (void *) SSMRecovery },
{"native_SetCVD2Values", "()I", (void *) SetCVD2Values },
{"native_GetSSMStatus", "()I", (void *) GetSSMStatus },
{"native_SetCurrentSourceInfo", "(III)I", (void *) SetCurrentSourceInfo },
{"native_GetCurrentSourceInfo", "()[I", (void *) GetCurrentSourceInfo },
{"native_GetPQDatabaseInfo", "(I)Lcom/droidlogic/app/SystemControlManager$PQDatabaseInfo;", (void *) GetPQDatabaseInfo },
{"native_StartUpgradeFBC", "(Ljava/lang/String;II)I", (void *) StartUpgradeFBC },
{"native_FactorySetPQMode_Brightness", "(IIIII)I", (void *) FactorySetPQMode_Brightness },
{"native_FactoryGetPQMode_Brightness", "(IIII)I", (void *) FactoryGetPQMode_Brightness },
{"native_FactorySetPQMode_Contrast", "(IIIII)I", (void *) FactorySetPQMode_Contrast },
{"native_FactoryGetPQMode_Contrast", "(IIII)I", (void *) FactoryGetPQMode_Contrast },
{"native_FactorySetPQMode_Saturation", "(IIIII)I", (void *) FactorySetPQMode_Saturation },
{"native_FactoryGetPQMode_Saturation", "(IIII)I", (void *) FactoryGetPQMode_Saturation },
{"native_FactorySetPQMode_Hue", "(IIIII)I", (void *) FactorySetPQMode_Hue },
{"native_FactoryGetPQMode_Hue", "(IIII)I", (void *) FactoryGetPQMode_Hue },
{"native_FactorySetPQMode_Sharpness", "(IIIII)I", (void *) FactorySetPQMode_Sharpness },
{"native_FactoryGetPQMode_Sharpness", "(IIII)I", (void *) FactoryGetPQMode_Sharpness },
{"native_FactoryResetPQMode", "()I", (void *) FactoryResetPQMode },
{"native_FactoryResetColorTemp", "()I", (void *) FactoryResetColorTemp },
{"native_FactorySetParamsDefault", "()I", (void *) FactorySetParamsDefault },
{"native_FactorySetNolineParams", "(IIIIIIIII)I", (void *) FactorySetNolineParams },
{"native_FactoryGetNolineParams", "(IIII)Lcom/droidlogic/app/SystemControlManager$noline_params_t;", (void *) FactoryGetNolineParams },
{"native_FactorySetOverscan", "(IIIIIII)I", (void *) FactorySetOverscan },
{"native_FactoryGetOverscan", "(III)Lcom/droidlogic/app/SystemControlManager$tvin_cutwin_t;", (void *) FactoryGetOverscan },
{"native_SetwhiteBalanceGainRed", "(IIIII)I", (void *) SetwhiteBalanceGainRed },
{"native_SetwhiteBalanceGainGreen", "(IIIII)I", (void *) SetwhiteBalanceGainGreen },
{"native_SetwhiteBalanceGainBlue", "(IIIII)I", (void *) SetwhiteBalanceGainBlue },
{"native_GetwhiteBalanceGainRed", "(IIII)I", (void *) GetwhiteBalanceGainRed },
{"native_GetwhiteBalanceGainGreen", "(IIII)I", (void *) GetwhiteBalanceGainGreen },
{"native_GetwhiteBalanceGainBlue", "(IIII)I", (void *) GetwhiteBalanceGainBlue },
{"native_SetwhiteBalanceOffsetRed", "(IIIII)I", (void *) SetwhiteBalanceOffsetRed },
{"native_SetwhiteBalanceOffsetGreen", "(IIIII)I", (void *) SetwhiteBalanceOffsetGreen },
{"native_SetwhiteBalanceOffsetBlue", "(IIIII)I", (void *) SetwhiteBalanceOffsetBlue },
{"native_GetwhiteBalanceOffsetRed", "(IIII)I", (void *) GetwhiteBalanceOffsetRed },
{"native_GetwhiteBalanceOffsetGreen", "(IIII)I", (void *) GetwhiteBalanceOffsetGreen },
{"native_GetwhiteBalanceOffsetBlue", "(IIII)I", (void *) GetwhiteBalanceOffsetBlue },
{"native_SaveWhiteBalancePara", "(IIIIIIIIII)I", (void *) SaveWhiteBalancePara },
{"native_FactoryfactoryGetColorTemperatureParams", "(I)I", (void *) FactoryfactoryGetColorTemperatureParams },
{"native_FactorySSMRestore", "()I", (void *) FactorySSMRestore },
{"native_FactoryResetNonlinear", "()I", (void *) FactoryResetNonlinear },
{"native_FactorySetGamma", "(III)I", (void *) FactorySetGamma },
{"native_GetRGBPattern", "()I", (void *) GetRGBPattern },
{"native_SetRGBPattern", "(III)I", (void *) SetRGBPattern },
{"native_FactorySetDDRSSC", "(I)I", (void *) FactorySetDDRSSC },
{"native_FactoryGetDDRSSC", "()I", (void *) FactoryGetDDRSSC },
{"native_FactorySetLVDSSSC", "(I)I", (void *) FactorySetLVDSSSC },
{"native_FactoryGetLVDSSSC", "()I", (void *) FactoryGetLVDSSSC },
{"native_whiteBalanceGrayPatternOpen", "()I", (void *) whiteBalanceGrayPatternOpen },
{"native_WhiteBalanceGrayPatternClose", "()I", (void *) WhiteBalanceGrayPatternClose },
{"native_WhiteBalanceGrayPatternSet", "(I)I", (void *) WhiteBalanceGrayPatternSet },
{"native_WhiteBalanceGrayPatternGet", "()I", (void *) WhiteBalanceGrayPatternGet },
{"native_FactorySetHdrMode", "(I)I", (void *) FactorySetHdrMode },
{"native_FactoryGetHdrMode", "()I", (void *) FactoryGetHdrMode },
{"native_SetDnlpParams", "(IIII)I", (void *) SetDnlpParams },
{"native_GetDnlpParams", "(III)I", (void *) GetDnlpParams },
{"native_FactorySetDnlpParams", "(IIIII)I", (void *) FactorySetDnlpParams },
{"native_FactoryGetDnlpParams", "(IIII)I", (void *) FactoryGetDnlpParams },
{"native_FactorySetBlackExtRegParams", "(IIII)I", (void *) FactorySetBlackExtRegParams },
{"native_FactoryGetBlackExtRegParams", "(III)I", (void *) FactoryGetBlackExtRegParams },
{"native_FactorySetColorParams", "(IIIIII)I", (void *) FactorySetColorParams },
{"native_FactoryGetColorParams", "(IIIII)I", (void *) FactoryGetColorParams },
{"native_FactorySetNoiseReductionParams", "(IIIIII)I", (void *) FactorySetNoiseReductionParams },
{"native_FactoryGetNoiseReductionParams", "(IIIII)I", (void *) FactoryGetNoiseReductionParams },
{"native_FactorySetCTIParams", "(IIIII)I", (void *) FactorySetCTIParams },
{"native_FactoryGetCTIParams", "(IIII)I", (void *) FactoryGetCTIParams },
{"native_FactorySetDecodeLumaParams", "(IIIII)I", (void *) FactorySetDecodeLumaParams },
{"native_FactoryGetDecodeLumaParams", "(IIII)I", (void *) FactoryGetDecodeLumaParams },
{"native_FactorySetSharpnessParams", "(IIIIII)I", (void *) FactorySetSharpnessParams },
{"native_FactoryGetSharpnessParams", "(IIIII)I", (void *) FactoryGetSharpnessParams },
{"native_SetDtvKitSourceEnable", "(I)I", (void *)SetDtvKitSourceEnable },
{"native_GetModeSupportDeepColorAttr", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) GetModeSupportDeepColorAttr },
{"native_setHdrStrategy", "(Ljava/lang/String;)V", (void *) setHdrStrategy },
};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_com_droidlogic_app_SystemControlManager(JNIEnv *env)
{
    static const char *const kClassPathName = "com/droidlogic/app/SystemControlManager";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, SystemControl_Methods, NELEM(SystemControl_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    GET_METHOD_ID(notifyCallback, clazz, "notifyCallback", "(I)V");
    GET_METHOD_ID(FBCUpgradeCallback, clazz, "FBCUpgradeCallback", "(II)V");
    GET_METHOD_ID(notifyDisplayModeCallback, clazz, "notifyDisplayModeCallback", "(I)V");
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
    g_JavaVM = vm;

    if (register_com_droidlogic_app_SystemControlManager(env) < 0)
    {
        ALOGE("Can't register DtvkitGlueClient");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}
