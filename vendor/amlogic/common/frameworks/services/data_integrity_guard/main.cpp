/*
 * Copyright (C) 2008 The Android Open Source Project
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
 * frank.chen@amlogic.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <dirent.h>

#define LOG_TAG "Dig"

#include "cutils/klog.h"
#include "cutils/log.h"
#include "cutils/properties.h"

#include "CommandListener.h"
#include "NetlinkManager.h"

#include "DigManager.h"

int main() {

    SLOGI("Dig 2.0 firing up");

    /* For when cryptfs checks and mounts an encrypted filesystem */
    klog_set_level(6);

    DigManager *dm;
    if (!(dm = DigManager::Instance())) {
        SLOGE("Unable to create DigManager");
        exit(1);
    };

    if (dm->start()) {
        SLOGE("Unable to start DigManager (%s)", strerror(errno));
        exit(1);
    }

    // Eventually we'll become the monitoring thread
    while (1) {
        sleep(1000);
    }

    SLOGI("Dig exiting");
    exit(0);
}

