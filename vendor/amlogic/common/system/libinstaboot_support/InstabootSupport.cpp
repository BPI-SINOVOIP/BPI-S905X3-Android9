/*
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#define LOG_TAG "Ibms"
#include "utils/Log.h"
#include "utils/misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Vector.h>
#include <binder/MemoryDealer.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>

#include "ISystemControlService.h"
#include "InstabootSupport.h"

#include "CTv.h"

using namespace android;

CTv * mCTv;

sp<ISystemControlService> mSysWrite = NULL;

int checkSystemControlService() {
    const String16 name("system_control");
    if (mSysWrite == NULL) {
       status_t status = getService(name, &mSysWrite);

       if (NO_ERROR != status) {
          ALOGE("Couldn't get connection to system_control service\n");
          mSysWrite = NULL;
          return -1;
       }
    }
    return 0;
}

int reInit(void) {
    ALOGI("InstabootSupport reinit");

    if (checkSystemControlService())
        return -1;

    mSysWrite->reInit();

    return 0;
}

int resetDisplay(void) {
    ALOGI("InstabootSupport resetDisplay");
    if (checkSystemControlService())
        return -1;

    mSysWrite->instabootResetDisplay();
    return 0;
}

int setOsdMouse(char *mode) {
    ALOGI("InstabootSupport setOsdMouse");
    if (checkSystemControlService())
        return -1;

    String16 mod(mode);
    mSysWrite->setOsdMouseMode(mod);
    return 0;
}

int getEnv(char *name, char *value, char const* def) {
    ALOGI("InstabootSupport getEnv");
    if (checkSystemControlService())
        return -1;

    String16 val;
    bool res = mSysWrite->getBootEnv(String16(name), val);
    if (res) {
        strcpy(value, String8(val).string());
    } else {
        strcpy(value, def);
    }
    return 0;
}

bool isSupportTv() {
    char prop[PROPERTY_VALUE_MAX] = {0};
    property_get("init.svc.tvd", prop, "stoped");
    if (strcmp(prop, "running") != 0) {
        ALOGD("no tvserver is running");
        return true;
    }
    return false;
}

int tvserver_suspend() {
    char prop[PROPERTY_VALUE_MAX] = {0};
    property_get("init.svc.tvd", prop, "stoped");
    if (strcmp(prop, "running") != 0) {
        ALOGD("no tvserver is running");
        return 1;
    }

    if (mCTv == NULL) {
        mCTv = new CTv();
    }
    ALOGD("suspend tvserver");
    mCTv->DoSuspend(1); //1  is instaboot suspend
    return 0;
}

int tvserver_resume() {
    char prop[PROPERTY_VALUE_MAX] = {0};
    property_get("init.svc.tvd", prop, "stoped");
    if (strcmp(prop, "running") != 0) {
        ALOGD("no tvserver is running");
        return 1;
    }

    if (mCTv != NULL) {
        ALOGD("resume tvserver");
        mCTv->DoResume(1);
        return 0;
    }
    ALOGD("didn't suspend tvserver, so just pass resume");
    return 1;
}
