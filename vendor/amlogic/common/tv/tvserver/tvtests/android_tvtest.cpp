/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "Tv-JNI"

#include <utils/Log.h>
#include <TvClient.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>

using namespace android;

void usage(char *processname)
{
    fprintf(stderr, "Usage: %s <cmd num> [arg1]... [argn]\n", processname);
    return;
}


int main(int argc, char **argv)
{
    if (argc < 2)
        usage(argv[0]);
    sp<TvClient> tv = TvClient::connect();
    int cmd = atoi(argv[1]);
    int arg1 = atoi(argv[2]);
    //send cmd
    Parcel p, r;
    p.writeInt32(cmd);
    p.writeInt32(arg1);
    tv->processCmd(p, &r);
    //exit
    tv.clear();
    return 0;
}
