/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Hugo Hong
 *  @version  1.0
 *  @date     2018/03/10
 *  @par function description:
 *  - 1 bluetooth rc jni
 */

#define LOG_TAG "remotecontrol_jni"

#include <jni.h>
#include <JNIHelp.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utils/Log.h>
#include "utils/misc.h"
#include "RemoteControlServer.h"

static JNIEnv* callbackEnv = NULL;// jni env
static jobject sJniObj = NULL;
static jmethodID method_onSearchStateChanged = NULL;
static jmethodID method_onDeviceStateChanged = NULL;

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// Re-run |fn| system call until the system call doesn't cause EINTR.
#define CALL_NO_INTR(fn) \
    do {                  \
    } while ((fn) == -1 && errno == EINTR)

enum {
    ENUM_PTHREAD_EXIT = -1,
    ENUM_MIC_DISABLE,
    ENUM_MIC_ENABLE,
    ENUM_DEV_INPUT,
    ENUM_DEV_REMOVE,
};

typedef struct {
    pthread_t tid; //main thread id
    int max_fd;
    int signal_fds[2];
    fd_set active_set;
}tMain_IPC;

tMain_IPC g_mainIPC;
static int uipc_init(JNIEnv *env);
static void uipc_deinit(void);

namespace android
{
    static void classInitNative(JNIEnv* env, jclass clazz) {
        //jclass jniCallbackClass = env->FindClass("com/droidlogic/DialogBluetoothService");
        method_onSearchStateChanged = env->GetMethodID(clazz, "onSearchStateChanged", "(I)V");
        method_onDeviceStateChanged = env->GetMethodID(clazz, "onDeviceStateChanged", "(I)V");
    }

    static bool initNative(JNIEnv* env, jobject obj) {
        sJniObj = env->NewGlobalRef(obj);
        uipc_init(env);
        return JNI_TRUE;
    }

    static bool cleanupNative(JNIEnv *env, jobject obj) {
        env->DeleteGlobalRef(sJniObj);
        uipc_deinit();
        return JNI_TRUE;
    }

    static void setMicEnableCallback(int flag) {
        if (callbackEnv != NULL && method_onSearchStateChanged != NULL)
            callbackEnv->CallVoidMethod(sJniObj, method_onSearchStateChanged, (jint)flag);
    }

    static void onDeviceChangedCallback(int flag) {
        if (callbackEnv != NULL && method_onDeviceStateChanged != NULL)
            callbackEnv->CallVoidMethod(sJniObj, method_onDeviceStateChanged, (jint)flag);
    }

    static JNINativeMethod sMethods[] = {
        {"classInitNative", "()V", (void *) classInitNative},
        {"initNative", "()Z", (void *) initNative},
        {"cleanupNative", "()Z", (void*) cleanupNative},
    };


    int register_android_RemoteControl(JNIEnv* env) {
        jclass clazz;
        const char *kClassPathName = "com/droidlogic/DialogBluetoothService";

        clazz = env->FindClass(kClassPathName);
        if (clazz == NULL) {
            ALOGE("Native registration unable to find class '%s'",
                    kClassPathName);
            return JNI_FALSE;
        }

        if (env->RegisterNatives(clazz, sMethods, NELEM(sMethods)) < 0) {
            env->DeleteLocalRef(clazz);
            ALOGE("RegisterNatives failed for '%s'", kClassPathName);
            return JNI_FALSE;
        }

        return JNI_TRUE;
    }
} // end namespace android

using namespace android;

void setMicEnable(int flag) {
    if (g_mainIPC.signal_fds[1] > 0) {
        char sig_tx = (flag == 0 ? ENUM_MIC_DISABLE : ENUM_MIC_ENABLE);
        send(g_mainIPC.signal_fds[1], &sig_tx, sizeof(sig_tx), MSG_NOSIGNAL);
    }
}

void onDeviceChanged(int flag) {
    if (g_mainIPC.signal_fds[1] > 0) {
        char sig_tx = (flag == 0 ? ENUM_DEV_REMOVE : ENUM_DEV_INPUT);
        send(g_mainIPC.signal_fds[1], &sig_tx, sizeof(sig_tx), MSG_NOSIGNAL);
    }
}

static rc_callbacks_t sRCCallbacks = {
    .setMicStatusCb = setMicEnable,
    .onDeviceStatusCb = onDeviceChanged,
};

static void *listen_task(void *arg) {
    int ret;
    fd_set read_fds;
    char sig_rX = 0;
    ssize_t rXlen = 0;

    ALOGD("%s:main thread run...", __func__);
    JNIEnv * env = static_cast<JNIEnv *> (arg);

    //attach java
    //JavaVM* vm = AndroidRuntime::getJavaVM();
    JavaVM* vm;
    env->GetJavaVM(&vm);

    JavaVMAttachArgs args;
    char name[] = "Remote control jni Callback Thread";
    args.version = JNI_VERSION_1_4;
    args.name = name;
    args.group = NULL;
    vm->AttachCurrentThread(&callbackEnv, &args);

    while (1) {
        read_fds = g_mainIPC.active_set;
        ret = select(g_mainIPC.max_fd + 1, &read_fds, NULL, NULL, NULL);
        ALOGD("%s:interrupt done, ret=%d err=%d", __func__, ret, errno);
        if (ret <= 0) continue;
        if (FD_ISSET(g_mainIPC.signal_fds[0], &read_fds)) {
            //wakeup interrupt
            CALL_NO_INTR(rXlen = recv(g_mainIPC.signal_fds[0], &sig_rX, sizeof(sig_rX), MSG_WAITALL));
            if (sig_rX == ENUM_PTHREAD_EXIT || rXlen == 0) break;
            else if (sig_rX == ENUM_MIC_DISABLE) {
                setMicEnableCallback(0);
            }
            else if (sig_rX == ENUM_MIC_ENABLE) {
                setMicEnableCallback(1);
            }
            else if (sig_rX == ENUM_DEV_INPUT) {
                onDeviceChangedCallback(1);
            }
            else if (sig_rX == ENUM_DEV_REMOVE) {
                onDeviceChangedCallback(0);
            }
        }
    }

    //detach java
    vm->DetachCurrentThread();

    //cleanup
    if (g_mainIPC.signal_fds[0] > 0) close(g_mainIPC.signal_fds[0]);
    if (g_mainIPC.signal_fds[1] > 0) close(g_mainIPC.signal_fds[1]);
    FD_ZERO(&g_mainIPC.active_set);

    ALOGD("%s:main thread exit...", __func__);

    return NULL;
}

pthread_t create_listen_thread(JNIEnv *env) {
    pthread_t tid;

    if (pthread_create(&tid, (const pthread_attr_t *) NULL, listen_task, env) < 0) {
        ALOGE("create_listen_thread pthread_create failed:%d", errno);
        return -1;
    }

    return tid;
}

static int uipc_init(JNIEnv *env) {
    ALOGD("%s...", __func__);

    memset(&g_mainIPC, 0, sizeof(g_mainIPC));

    FD_ZERO(&g_mainIPC.active_set);

     /* setup interrupt socket pair */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, g_mainIPC.signal_fds) < 0) {
        return -1;
    }
    FD_SET(g_mainIPC.signal_fds[0], &g_mainIPC.active_set);
    g_mainIPC.max_fd = MAX(g_mainIPC.max_fd, g_mainIPC.signal_fds[0]);

    //start remote control service
    RemoteControlServer* rcServer = RemoteControlServer::getInstance();
    rcServer->setCallback(&sRCCallbacks);

    //start listen thread
    g_mainIPC.tid = create_listen_thread(env);

    return 0;
}

static void uipc_deinit(void) {
    //exit thread
    if (g_mainIPC.signal_fds[1] > 0) {
        char sig_tx = ENUM_PTHREAD_EXIT;
        send(g_mainIPC.signal_fds[1], &sig_tx, sizeof(sig_tx), MSG_NOSIGNAL);
    }
    if (g_mainIPC.tid) pthread_join(g_mainIPC.tid, NULL);
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {

    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }

    register_android_RemoteControl(env);

    return JNI_VERSION_1_4;
}

