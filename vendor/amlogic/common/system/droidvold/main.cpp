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
 */

#include "Disk.h"
#include "VolumeManager.h"
#include "NetlinkManager.h"
#include "DroidVold.h"

#define LOG_TAG "droidVold"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/klog.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <HidlTransportSupport.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <fcntl.h>
#include <dirent.h>
#include <fs_mgr.h>

#include "cutils/klog.h"
#include "cutils/log.h"
#include "cutils/properties.h"

static int process_config(VolumeManager *vm, bool* has_adoptable);

static void set_media_poll_time(void);

struct fstab *fstab;

using namespace android;
using ::android::base::StringPrintf;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::vendor::amlogic::hardware::droidvold::V1_0::implementation::DroidVold;
using ::vendor::amlogic::hardware::droidvold::V1_0::IDroidVold;
using ::vendor::amlogic::hardware::droidvold::V1_0::Result;

int main(int argc, char** argv) {
    setenv("ANDROID_LOG_TAGS", "*:v", 1);
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));

    LOG(INFO) << "doildVold 1.0 firing up";

    //android::ProcessState::initWithDriver("/dev/hwbinder");
    configureRpcThreadpool(4, true);

    VolumeManager *vm;
    NetlinkManager *nm;
    DroidVold *dv;

    /* Create our singleton managers */
    if (!(vm = VolumeManager::Instance())) {
        LOG(ERROR) << "Unable to create VolumeManager";
        exit(1);
    }

    if (!(nm = NetlinkManager::Instance())) {
        LOG(ERROR) << "Unable to create NetlinkManager";
        exit(1);
    }

    if (property_get_bool("droidvold.debug", false)) {
        vm->setDebug(true);
    }

    if (!(dv= DroidVold::Instance())) {
        LOG(ERROR) << "Unable to create DroidVold";
        exit(1);
    }

    vm->setBroadcaster(dv);
    nm->setBroadcaster(dv);

    if (vm->start()) {
        PLOG(ERROR) << "Unable to start VolumeManager";
        exit(1);
    }

    bool has_adoptable;

    if (process_config(vm, &has_adoptable)) {
        PLOG(ERROR) << "Error reading configuration... continuing anyways";
    }

    if (nm->start()) {
        PLOG(ERROR) << "Unable to start NetlinkManager";
        exit(1);
    }

    sp<IDroidVold> idv = DroidVold::Instance();
    if (idv == nullptr)
        ALOGE("Cannot create IDroidVold service");
    else if (idv->registerAsService() != OK)
        ALOGE("Cannot register IDroidVold service.");
    else
        ALOGI("IDroidVold service created.");

    set_media_poll_time();
    vm->coldboot("/sys/block");

    /*
     * This thread is just going to process Binder transactions.
     */
    joinRpcThreadpool();

    LOG(ERROR) << "droidVold exiting";
    exit(0);
}

static void set_media_poll_time(void) {
    int fd;

    fd = open ("/sys/module/block/parameters/events_dfl_poll_msecs", O_WRONLY);
    if (fd >= 0) {
        write(fd, "2000", 4);
        close (fd);
    } else {
        LOG(ERROR) << "kernel not support media poll uevent!";
    }
}

static int process_config(VolumeManager *vm, bool* has_adoptable) {
    std::string path(android::droidvold::DefaultFstabPath());
    fstab = fs_mgr_read_fstab(path.c_str());
    if (!fstab) {
        PLOG(ERROR) << "Failed to open default fstab " << path;
        return -1;
    }

    /* Loop through entries looking for ones that vold manages */
    *has_adoptable = false;
    for (int i = 0; i < fstab->num_entries; i++) {
        if (fs_mgr_is_voldmanaged(&fstab->recs[i])) {
            if (fs_mgr_is_nonremovable(&fstab->recs[i])) {
                LOG(WARNING) << "nonremovable no longer supported; ignoring volume";
                continue;
            }

            std::string sysPattern(fstab->recs[i].blk_device);
            std::string nickname(fstab->recs[i].label);
            int flags = 0;

            if (fs_mgr_is_encryptable(&fstab->recs[i])) {
                flags |= android::droidvold::Disk::Flags::kAdoptable;
                *has_adoptable = true;
            }
            if (fs_mgr_is_noemulatedsd(&fstab->recs[i])
                    || property_get_bool("vold.debug.default_primary", false)) {
                flags |= android::droidvold::Disk::Flags::kDefaultPrimary;
            }

            vm->addDiskSource(std::shared_ptr<VolumeManager::DiskSource>(
                    new VolumeManager::DiskSource(sysPattern, nickname, flags)));
        }
    }
    return 0;
}
