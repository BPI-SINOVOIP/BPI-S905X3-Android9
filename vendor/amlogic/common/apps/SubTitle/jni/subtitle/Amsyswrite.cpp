/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c++ file
*/
#define LOG_TAG "amSystemControl"

//#include "SystemControlClient.h"

#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <Amsyswrite.h>
#include <unistd.h>

#include <MemoryLeakTrackUtilTmp.h>
#include <fcntl.h>

#include <stdio.h>

using namespace android;

static Mutex amLock;
/*SystemControlClient* sysctrlClient;

SystemControlClient *getSystemControlService()
{
    ALOGD("[getSystemControlService]");
    Mutex::Autolock _l(amLock);
    if (sysctrlClient == NULL) {
        sysctrlClient = new SystemControlClient();
    }
    ALOGE_IF(sysctrlClient == NULL, "no System Control Service!?");

    return sysctrlClient;
}*/


int amSystemControlGetProperty(const char *key, char *value)
{
    /*const sp < ISystemControlService > &scs = getSystemControlService();
    if (scs != 0)
    {
        String16 v;
        if (scs->getProperty(String16(key), v))
        {
            strcpy(value, String8(v).string());
            return 0;
        }
    }*/
    return -1;
}

int amSystemControlGetPropertyStr(const char *key, char *def, char *value)
{
    /*const sp < ISystemControlService > &scs = getSystemControlService();
    if (scs != 0)
    {
        String16 v;
        String16 d(def);
        scs->getPropertyString(String16(key), v, d);
        strcpy(value, String8(v).string());
    }
    strcpy(value, def);*/
    return -1;
}

int amSystemControlGetPropertyInt(const char *key, int def)
{
    /*const sp < ISystemControlService > &scs = getSystemControlService();
    if (scs != 0)
    {
        return scs->getPropertyInt(String16(key), def);
    }*/
    return def;
}

long amSystemControlGetPropertyLong(const char *key, long def)
{
    /*const sp < ISystemControlService > &scs = getSystemControlService();
    if (scs != 0)
    {
        return scs->getPropertyLong(String16(key), def);
    }*/
    return def;
}

int amSystemControlGetPropertyBool(const char *key, int def)
{
    /*const sp < ISystemControlService > &scs = getSystemControlService();
    if (scs != 0)
    {
        if (scs->getPropertyBoolean(String16(key), def))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }*/
    return def;
}

void amSystemControlSetProperty(const char *key, const char *value)
{
    /*const sp < ISystemControlService > &scs = getSystemControlService();
    if (scs != 0)
    {
        scs->setProperty(String16(key), String16(value));
    }*/
}

int amSystemControlReadSysfs(const char *path, char *value)
{
    /*ALOGD("amSystemControlReadSysfs:%s",path);
    SystemControlClient *scs = getSystemControlService();
    if (scs  != NULL) {
        std::string v;
        if (scs->readSysfs(path, v))
        {
            strcpy(value, v.c_str());
            return 0;
        }
    }*/
    return -1;
}

int amSystemControlReadNumSysfs(const char *path, char *value, int size)
{
    //ALOGD("amSystemControlReadNumSysfs:%s",path);
    /*const sp<SystemControlClient> &scs = getSystemControlService();
    if (scs != 0 && value != NULL && access(path, 0) != -1)
    {
        std::string v;
        if (scs->readSysfs(path, v))
        {
            if (v.size() != 0)
            {
                //ALOGD("readSysfs ok:%s,%s,%d", path, String8(v).string(), String8(v).size());
                memset(value, 0, size);
                if (size <= v.c_str().size() + 1)
                {
                    memcpy(value, v.c_str(),
                           size - 1);
                    value[strlen(value)] = '\0';
                }
                else
                {
                    strcpy(value, v.c_str());
                }
                return 0;
            }
        }
    }*/
    //ALOGD("[false]amSystemControlReadNumSysfs%s,",path);
    return -1;
}

int amSystemControlWriteSysfs(const char *path, char *value)
{
    //ALOGD("amSystemControlWriteSysfs:%s",path);
    /*SystemControlClient *scs = getSystemControlService();
    if (scs != 0)
    {
        if (scs->writeSysfs(path, value))
        {
            //ALOGD("writeSysfs ok");
            return 0;
        }
    }*/
    //ALOGD("[false]amSystemControlWriteSysfs%s,",path);
    return -1;
}

void amDumpMemoryAddresses(int fd)
{
    ALOGE("[amDumpMemoryAddresses]fd:%d\n", fd);
    //dumpMemoryAddresses(fd);
    //close(fd);
}
