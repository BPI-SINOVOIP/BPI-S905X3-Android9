/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#include <CTvLog.h>
#include <cutils/properties.h>
#include "CBootvideoStatusDetect.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CBootvideoStatusDetect"
#endif

CBootvideoStatusDetect::CBootvideoStatusDetect()
{
    LOGD("%s", __FUNCTION__);
    mpObserver = NULL;
    mIsRunning = false;
}

CBootvideoStatusDetect::~CBootvideoStatusDetect() {}

int CBootvideoStatusDetect::startDetect()
{
    LOGD("%s", __FUNCTION__);
    if (!mIsRunning)
        this->run("CBootvideoStatusDetect");

    return 0;
}

int CBootvideoStatusDetect::stopDetect()
{
    LOGD("%s", __FUNCTION__);
    requestExit();
    mIsRunning = false;

    return 0;
}


bool CBootvideoStatusDetect::isBootvideoStopped() {
    char prop_bootvideo_type[PROPERTY_VALUE_MAX];
    char prop_bootvideo[PROPERTY_VALUE_MAX];

    memset(prop_bootvideo_type, '\0', PROPERTY_VALUE_MAX);
    memset(prop_bootvideo, '\0', PROPERTY_VALUE_MAX);

    // 1: boot video;  other: boot animation
    property_get("service.bootvideo", prop_bootvideo_type, "null");
    // stopped: bootvideo stopped;  running: bootvideo is running
    // now bootanim and boot service use the same service name
    property_get("init.svc.bootanim", prop_bootvideo, "null");

    if ((strcmp(prop_bootvideo_type, "1") == 0 && strcmp(prop_bootvideo, "stopped") != 0))
        return false;
    else
        return true;
}

bool CBootvideoStatusDetect::threadLoop()
{
    if ( mpObserver == NULL ) {
        return false;
    }
    while (!exitPending()) {
        if (isBootvideoStopped()) {
            mpObserver->onBootvideoStopped();
        } else {
            mpObserver->onBootvideoRunning();
        }
        usleep(500 * 1000);
    }

    LOGD("%s, exiting...\n", "CBootvideoStatusDetect");
    return false;
}


