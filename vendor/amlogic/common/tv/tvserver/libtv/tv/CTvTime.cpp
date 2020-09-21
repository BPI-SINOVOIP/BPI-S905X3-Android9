/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvTime"

#include "CTvTime.h"
#include <CTvLog.h>

long CTvTime::getSysUTCTime()
{
    //struct tm tm;
    //time_t t;
    //int64_t r = mktime_tz(&(tm), NULL);
    //time_t t = time(NULL);
    //LOGD("---------utc t = %ld time t=%ld", r, t);
    return 0;
}

void CTvTime::setTime(long t)
{
    //long utcMS;
    //time(&utcMS);
    //nsecs_t ns = systemTime(CLOCK_REALTIME);
    //nsecs_t tm = ns2s(ns);
    //unsigned long ticks = times(NULL);
    //long tm = ticks/mHZ;
    struct sysinfo s_info;
    int error;
    error = sysinfo(&s_info);

    mDiff = t - s_info.uptime;
    LOGD("--- mDiff=%ld", mDiff);
}

long CTvTime::getTime()
{
    //long utcMS;
    //time(&utcMS);
    //nsecs_t ns = systemTime(CLOCK_REALTIME);
    //nsecs_t sec = ns2s(ns);

    //unsigned long ticks = times(NULL);
    //long sec = ticks/mHZ;
    struct sysinfo s_info;
    int error;
    error = sysinfo(&s_info);


    LOGD("--- mDiff=%ld, sec=%ld", mDiff, s_info.uptime);
    return s_info.uptime + mDiff;
}
