/*
 * Copyright (C) 2017 Amlogic Corporation.
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

#define LOG_TAG "bridge"
#include <android/log.h>
#include <cutils/properties.h>
#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/threads.h>
#include <systemcontrol/ISystemControlService.h>

#define SYST_SERVICES_NAME "system_control"


using namespace android;
extern "C" int amSystemWriteSetProperty(const char* key, const char* value, int lengthval);
extern "C" int amSystemWriteGetProperty(const char* key, char* value);
class DeathNotifier: public IBinder::DeathRecipient
{
    public:
        DeathNotifier() {
        }

        void binderDied(const wp<IBinder>& who __unused) {
            ALOGW("system_write died!");
        }
};
static sp<ISystemControlService> amSystemWriteService;
static sp<DeathNotifier> amDeathNotifier;
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
int amSystemWriteSetProperty(const char* key, const char* value, int leth)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    char *buf = new char[leth+1];
    strncpy(buf, value, leth);
    if (sws != 0) {
        sws->writeSysfs(String16(key), String16(buf));
        delete[] buf;
        ALOGE("write value %s %s\n",key,buf);
        return 0;
    }
    delete[] buf;
    return -1;
}
int amSystemWriteGetProperty(const char* key, char* value)
{
    const sp<ISystemControlService>& sws = getSystemWriteService();
    if (sws != 0) {
        String16 read_value;
        sws->readSysfs(String16(key), read_value);
        String8 name8 = String8(read_value);
        const char *C_name8 = (char *)name8.string();
        strcpy(value,C_name8);
        ALOGE("read_value(%s) %s %zu;\n",key,value, strlen(value));
        return strlen(value);
    }else{
        return -1;
    }
}
