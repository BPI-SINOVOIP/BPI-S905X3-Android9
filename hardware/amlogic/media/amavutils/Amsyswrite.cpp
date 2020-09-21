/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef NO_USE_SYSWRITE
#define LOG_TAG "amSystemWrite"


#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <Amsyswrite.h>
#include <unistd.h>
#if ANDROID_PLATFORM_SDK_VERSION >= 21 //5.0
#include <systemcontrol/ISystemControlService.h>
#else
#include <systemwrite/ISystemWriteService.h>
#endif

using namespace android;

class DeathNotifier: public IBinder::DeathRecipient
{
    public:
        DeathNotifier() {
        }

        void binderDied(const wp<IBinder>& who) {
            ALOGW("system_write died!");
        }
};


#if ANDROID_PLATFORM_SDK_VERSION >= 21 //5.0
//used ISystemControlService
#define SYST_SERVICES_NAME "system_control"
#else
//used amSystemWriteService
#define ISystemControlService ISystemWriteService
#define SYST_SERVICES_NAME "system_write"
#endif

static sp<ISystemControlService> amSystemWriteService;
static sp<DeathNotifier> amDeathNotifier;
static  Mutex            amLock;
static  Mutex            amgLock;

const sp<ISystemControlService>& getSystemWriteService()
{
    Mutex::Autolock _l(amgLock);
    if (amSystemWriteService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
#if 0
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("system_write"));
            if (binder != 0)
                break;
            ALOGW("SystemWriteService not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (amDeathNotifier == NULL) {
            amDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(amDeathNotifier);
        amSystemWriteService = interface_cast<ISystemWriteService>(binder);
#endif


        amSystemWriteService = interface_cast<ISystemControlService>(sm->getService(String16(SYST_SERVICES_NAME)));

    }
    ALOGE_IF(amSystemWriteService==0, "no SystemWrite Service!?");

    return amSystemWriteService;
}

int amSystemWriteGetProperty(const char* key, char* value)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        String16 v;
        if (sws->getProperty(String16(key), v)) {
            strcpy(value, String8(v).string());
            return 0;
        }
    }
    return -1;

}

int amSystemWriteGetPropertyStr(const char* key, char* def, char* value)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        String16 v;
        String16 d(def);
        sws->getPropertyString(String16(key), v, d);
        strcpy(value, String8(v).string());
    }
    strcpy(value, def);
    return -1;
}

int amSystemWriteGetPropertyInt(const char* key, int def)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        return sws->getPropertyInt(String16(key), def);
    }
    return def;
}


long amSystemWriteGetPropertyLong(const char* key, long def)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        return	sws->getPropertyLong(String16(key), def);
    }
    return def;
}


int amSystemWriteGetPropertyBool(const char* key, int def)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        if (sws->getPropertyBoolean(String16(key), def)) {
            return 1;
        } else {
            return 0;
        }
    }
    return def;

}

void amSystemWriteSetProperty(const char* key, const char* value)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        sws->setProperty(String16(key), String16(value));
    }
}

int amSystemWriteReadSysfs(const char* path, char* value)
{
    //ALOGD("amSystemWriteReadNumSysfs:%s",path);
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        String16 v;
        if (sws->readSysfs(String16(path), v)) {
            strcpy(value, String8(v).string());
            return 0;
        }
    }
    return -1;
}

int amSystemWriteReadNumSysfs(const char* path, char* value, int size)
{
    //ALOGD("amSystemWriteReadNumSysfs:%s",path);

    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0 && value != NULL && access(path, 0) != -1) {
        String16 v;
        if (sws->readSysfs(String16(path), v)) {
            if (v.size() != 0) {
                //ALOGD("readSysfs ok:%s,%s,%d", path, String8(v).string(), String8(v).size());
                memset(value, 0, size);
                if (size <= String8(v).size() + 1) {
                    memcpy(value, String8(v).string(), size - 1);
                    value[strlen(value)] = '\0';

                } else {
                    strcpy(value, String8(v).string());

                }
                return 0;
            }
        }

    }
    //ALOGD("[false]amSystemWriteReadNumSysfs%s,",path);
    return -1;
}

int amSystemWriteWriteSysfs(const char* path, char* value)
{
    //ALOGD("amSystemWriteWriteSysfs:%s",path);
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        String16 v(value);
        if (sws->writeSysfs(String16(path), v)) {
            //ALOGD("writeSysfs ok");
            return 0;
        }
    }
    //ALOGD("[false]amSystemWriteWriteSysfs%s,",path);
    return -1;
}

#if ANDROID_PLATFORM_SDK_VERSION >= 21 //5.0
extern "C" int amSystemControlSetNativeWindowRect(int x, int y, int w, int h) {
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        sws->setNativeWindowRect(x, y, w, h);
        return 0;
    }

    return -1;
}
#endif

#endif
