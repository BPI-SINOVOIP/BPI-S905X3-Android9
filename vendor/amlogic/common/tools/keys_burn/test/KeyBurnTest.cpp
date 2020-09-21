/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#define LOG_TAG "KeyBurnTest"
#include "KeyBurn.h"
#include <utils/Log.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    for (int i=0;i<argc;i++)
    ALOGI("argv: %s\n", argv[i]);
    int type = atoi(argv[1]);
    char* path = argv[2];
    int ret = writeKey(type, path);
    if (ret) {
        ALOGD("Fail to burn key (%d)from %s \n", type, path);
        return -1;
    }
    ALOGD("burn key (%d)from %s success\n", type, path);
    return 0;
}
