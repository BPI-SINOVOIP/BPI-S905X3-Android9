/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <linux/kd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <linux/loop.h>
#include <cutils/partition_utils.h>
#include <cutils/properties.h>
#include "log.h"

// 0: zygote_secondary service has stopped
// 1: zygote_secondary service has started
static int sIsZygoteSecondaryStart = 0;

static void
handleStartFailed(void) {
    // TODO: zygote_secondary start failed process
}

static void
handleStopFailed(void) {
    // TODO: zygote_secondary stop failed process
}

static int
supportZygoteSecondaryStart(void) {
    char value[PROPERTY_VALUE_MAX] = {0};

    property_get("ro.zygote", value, "zygote32");
    INFO("ro.zygote=%s\n", value);
    if (strncmp(value, "zygote64_32", 11) &&
        strncmp(value, "zygote32_64", 11)) {
        return 0;
    }
    return 1;
}

static int
enableZygoteSecondary() {
    char enable[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.dynamic.zygote_secondary", enable, "disable");
    INFO("ro.dynamic.zygote_secondary=%s\n", enable);
    if (!strncmp(enable, "enable", 6)) {
        return 1;
    }
    return 0;
}

static int
isFirstBoot(void) {
    int isFirstBoot = 0;
    char value[PROPERTY_VALUE_MAX] = {0};

    property_get("ro.firstboot", value, "0");
    if (!strncmp(value, "1", 1)) {
        isFirstBoot = 1;
    }

    INFO("ro.firstboot=%s.system %s first boot\n",
        value, isFirstBoot ? "is" : "isn't");
    return isFirstBoot;
}

static int
isBootCompleted(void) {
    int isBootCompleted = 0;
    char value[PROPERTY_VALUE_MAX] = {0};

    property_get("sys.boot_completed", value, "0");
    if (!strncmp(value, "1", 1)) {
        isBootCompleted = 1;
    }

    //INFO("sys.boot_completed=%s.system %s\n",
    //    value, isBootCompleted ? "boot completed" : "is booting");
    return isBootCompleted;
}

static int
ensureZygoteSecondaryStart(void) {
    if (sIsZygoteSecondaryStart)
        return 0;

    int ret = property_set("sys.zygote_secondary", "start");
    if (ret == 0) {
        sIsZygoteSecondaryStart = 1;
    }
    return ret;
}


static int
ensureZygoteSecondaryStop(void) {
    if (!sIsZygoteSecondaryStart)
        return 0;

    int ret = property_set("sys.zygote_secondary", "stop");
    if (ret == 0) {
        sIsZygoteSecondaryStart = 0;
    }
    return ret;
}


/**
 *  whether to start zygote_secondary service or not when system boot
 *  start zygote_secondary by following conditions:
 *   1.first boot;
 *   2.persist.sys.zygote_secondary=start;
 *   3.data partition has been erased before.
 *  return:
 *   0: start service successful
 * -1: start service failed
 *   1: don't start service
 */
static int
zygoteSecondaryStartOrNotWhenBoot(int isFirstBoot) {
    if (isFirstBoot) {
        return ensureZygoteSecondaryStart();
    }

    static int printCnt = 0;
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.sys.zygote_secondary", value, "");

    if (!printCnt) {
        INFO("persist.sys.zygote_secondary=%s\n", value);
        printCnt = 1;
    }

    if (!strncmp(value, "start", 5)) {
        return ensureZygoteSecondaryStart();
    } else if (!strncmp(value, "stop", 4)) {
        // Don't start when system is booting. Nothing to do!
    } else {
        /* If system isn't not first boot and /data/property/persist.sys.zygote_secondary
        *   file doesn't exist, we consider data partition has been erased, so we start
        *   zygote_secondary service
        */
        INFO("data partition may have been erased, start zygote_secondary\n");
        return ensureZygoteSecondaryStart();
    }

    return 1;
}

int main() {
    if (!enableZygoteSecondary()) {
        INFO("zygote_secondary disabled,zygote_proxy exit!\n");
        return 0;
    }
    INFO("dynamic start/stop zygote_secondary enable\n");

    if (!supportZygoteSecondaryStart()) {
        INFO("zygote_secondary unsupport,zygote_proxy exit!\n");
        return 0;
    }

    int status = 0;
    int printCnt = 0;
    int firstBoot = isFirstBoot();
    int preventOverflow = 1;
    const int PRINT_CNT_MAX = 3;

    while (1) {
        if (!isBootCompleted()) {
            if (!sIsZygoteSecondaryStart) {
                status = zygoteSecondaryStartOrNotWhenBoot(firstBoot);
                if (preventOverflow && (printCnt < PRINT_CNT_MAX)) {
                    INFO("zygote_secondary service %s\n",
                        !status ? "start successful" : ((status < 0) ? "start failed" : "don't start"));
                    if (++printCnt == PRINT_CNT_MAX)
                        preventOverflow = 0;
                }
                if (status < 0) {
                    handleStartFailed();
                }
            }
        } else {// exit after boot completed
            return 0;
        }

        usleep(100000);
    }

    return 0;
}


