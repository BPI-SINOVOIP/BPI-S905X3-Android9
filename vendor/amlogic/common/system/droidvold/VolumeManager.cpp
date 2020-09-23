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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <sys/sysmacros.h>

#include <linux/kdev_t.h>
#include <linux/loop.h>

#define LOG_TAG "droidVold"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/fs.h>
#include <cutils/log.h>

#include <selinux/android.h>

#include <sysutils/NetlinkEvent.h>
#include <private/android_filesystem_config.h>

#include "VolumeManager.h"
#include "NetlinkManager.h"
#include "DroidVold.h"

#include "fs/Ext4.h"
#include "fs/Vfat.h"
#include "Utils.h"
#include "Process.h"
#include "fs/Iso9660.h"

#ifdef HAS_VIRTUAL_CDROM
#define LOOP_DEV "/dev/block/loop0"
#define LOOP_MOUNTPOINT "/mnt/loop"
#endif

using android::base::StringPrintf;

static const unsigned int kMajorBlockMmc = 179;
static const unsigned int kMajorBlockExperimentalMin = 240;
static const unsigned int kMajorBlockExperimentalMax = 254;

VolumeManager *VolumeManager::sInstance = NULL;

VolumeManager *VolumeManager::Instance() {
    if (!sInstance)
        sInstance = new VolumeManager();
    return sInstance;
}

VolumeManager::VolumeManager() {
    mDebug = false;
    mBroadcaster = NULL;
#ifdef HAS_VIRTUAL_CDROM
    mLoopPath = NULL;
#endif
}

VolumeManager::~VolumeManager() {
}

int VolumeManager::setDebug(bool enable) {
    mDebug = enable;
    return 0;
}

int VolumeManager::start() {
    // Always start from a clean slate by unmounting everything in
    // directories that we own, in case we crashed.
    unmountAll();

    return 0;
}

int VolumeManager::stop() {
    return 0;
}

void VolumeManager::handleBlockEvent(NetlinkEvent *evt) {
    std::lock_guard<std::mutex> lock(mLock);

    if (mDebug) {
        LOG(VERBOSE) << "----------------";
        LOG(VERBOSE) << "handleBlockEvent with action " << (int) evt->getAction();
        evt->dump();
    }

    std::string devType(evt->findParam("DEVTYPE")?evt->findParam("DEVTYPE"):"");


    if (devType == "disk") {
        std::string eventPath(evt->findParam("DEVPATH")?evt->findParam("DEVPATH"):"");
        std::string devName(evt->findParam("DEVNAME")?evt->findParam("DEVNAME"):"");

        int major = atoi(evt->findParam("MAJOR"));
        int minor = atoi(evt->findParam("MINOR"));
        dev_t device = makedev(major, minor);

        switch (evt->getAction()) {
        case NetlinkEvent::Action::kAdd: {
            for (auto source : mDiskSources) {
                if (source->matches(eventPath)) {
                    // For now, assume that MMC and virtio-blk (the latter is
                    // emulator-specific; see Disk.cpp for details) devices are SD,
                    // and that everything else is USB
                    int flags = source->getFlags();
                    if (major == kMajorBlockMmc
                        || (android::droidvold::IsRunningInEmulator()
                        && major >= (int) kMajorBlockExperimentalMin
                        && major <= (int) kMajorBlockExperimentalMax)) {
                        flags |= android::droidvold::Disk::Flags::kSd;
                    } else {
                        flags |= android::droidvold::Disk::Flags::kUsb;
                    }

                    auto disk = new android::droidvold::Disk(eventPath, device,
                            source->getNickname(), devName, flags);
                    disk->create();
                    mDisks.push_back(std::shared_ptr<android::droidvold::Disk>(disk));
                    break;
                }
            }
            break;
        }
        case NetlinkEvent::Action::kChange: {
            LOG(DEBUG) << "Disk at " << major << ":" << minor << " changed";
            for (auto disk : mDisks) {
                if (disk->getDevice() == device) {
                    if (disk->isSrdiskMounted()) {
                        LOG(DEBUG) << "srdisk  ejected";
                        disk->destroyAllVolumes();
                        break;
                    }

                    //disk->readMetadata();
                    //disk->readPartitions();
                }
            }
            break;
        }
        case NetlinkEvent::Action::kRemove: {
            auto i = mDisks.begin();
            while (i != mDisks.end()) {
                if ((*i)->getDevice() == device) {
                    (*i)->destroy();
                    i = mDisks.erase(i);
                } else {
                    ++i;
                }
            }
            break;
        }
        default: {
            LOG(WARNING) << "Unexpected block event action " << (int) evt->getAction();
            break;
        }
        }

    } else {
        for (auto disk : mDisks) {
            disk->handleBlockEvent(evt);
        }
    }

}

void VolumeManager::addDiskSource(const std::shared_ptr<DiskSource>& diskSource) {
    mDiskSources.push_back(diskSource);
}

std::shared_ptr<android::droidvold::VolumeBase> VolumeManager::findVolume(const std::string& id) {

    for (auto disk : mDisks) {
        auto vol = disk->findVolume(id);
        if (vol != nullptr) {
            return vol;
        }
    }
    return nullptr;
}

void VolumeManager::listVolumes(android::droidvold::VolumeBase::Type type,
        std::list<std::string>& list) {
    list.clear();
    for (auto disk : mDisks) {
        disk->listVolumes(type, list);
    }
}

int VolumeManager::unmountAll() {
    std::lock_guard<std::mutex> lock(mLock);

    for (auto disk : mDisks) {
        disk->unmountAll();
    }

    return 0;
}

#ifdef HAS_VIRTUAL_CDROM
int VolumeManager::loopsetfd(const char * path)
{
    int fd,file_fd;

    if ((fd = open(LOOP_DEV, O_RDWR)) < 0) {
        SLOGE("Unable to open loop0 device (%s)",strerror(errno));
        return -1;
    }

    if ((file_fd = open(path, O_RDWR)) < 0) {
        SLOGE("Unable to open %s (%s)", path, strerror(errno));
        close(fd);
        return -1;
    }

    if (ioctl(fd, LOOP_SET_FD, file_fd) < 0) {
        SLOGE("Error setting up loopback interface (%s)", strerror(errno));
        close(file_fd);
        close(fd);
        return  -1;
    }

    close(fd);
    close(file_fd);

    SLOGD("loopsetfd (%s) ok\n", path);
    return 0;
}

int VolumeManager::loopclrfd()
{
    int fd;
    int rc=0;

    if ((fd = open(LOOP_DEV, O_RDWR)) < 0) {
        SLOGE("Unable to open loop0 device (%s)",strerror(errno));
        return -1;
    }

    if (ioctl(fd, LOOP_CLR_FD, 0) < 0) {
        SLOGE("Error setting up loopback interface (%s)", strerror(errno));
        rc = -1;
    }
    close(fd);

    SLOGD("loopclrfd ok\n");
    return rc;
}

int VolumeManager::mountloop(const char * path) {
    if (isMountpointMounted(LOOP_MOUNTPOINT)) {
        SLOGW("loop file already mounted,please umount fist,then mount this file!");
        errno = EBUSY;
        return -1;
    }

    if (loopsetfd(path) < 0) {
        return -1;
    }

    if (fs_prepare_dir(LOOP_MOUNTPOINT, 0700, AID_ROOT, AID_SDCARD_R)) {
        SLOGE("failed to create loop mount points");
        return -errno;
    }

    if (android::droidvold::iso9660::Mount(LOOP_DEV, LOOP_MOUNTPOINT, false, false,
                AID_SDCARD_R, AID_SDCARD_R, 0007, true)) {
        loopclrfd();
        SLOGW("%s failed to mount via ISO9660(%s)", LOOP_DEV, strerror(errno));
        return -1;
    } else {
        mLoopPath = strdup(path);
        SLOGI("Successfully mount %s as ISO9660", LOOP_DEV);
    }

    return 0;
}

int VolumeManager::unmountloop(bool unused) {
    if (!isMountpointMounted(LOOP_MOUNTPOINT)) {
        SLOGW("no loop file mounted");
        errno = ENOENT;
        return -1;
    }

    android::droidvold::ForceUnmount(LOOP_MOUNTPOINT);
    loopclrfd();
    rmdir(LOOP_MOUNTPOINT);

    if (mLoopPath != NULL) {
        free(mLoopPath);
        mLoopPath = NULL;
    }

    return 0;
}

void VolumeManager::unmountLoopIfNeed(const char *label) {
    if (mLoopPath != NULL && strstr(mLoopPath, label)) {
        SLOGD("umount loop");
        unmountloop(true);
    }
}

#endif

bool VolumeManager::isMountpointMounted(const char *mp)
{
    FILE *fp = setmntent("/proc/mounts", "r");
    if (fp == NULL) {
        SLOGE("Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    bool found_mp = false;
    mntent* mentry;
    while ((mentry = getmntent(fp)) != NULL) {
        if (strcmp(mentry->mnt_dir, mp) == 0) {
            found_mp = true;
            break;
        }
    }
    endmntent(fp);
    return found_mp;
}

int VolumeManager::reset() {
    // Tear down all existing disks/volumes and start from a blank slate so
    // newly connected framework hears all events.
    for (auto disk : mDisks) {
        disk->destroy();
        disk->reset();
    }

    return 0;
}

int VolumeManager::shutdown() {
    for (auto disk : mDisks) {
        disk->destroy();
    }
    mDisks.clear();
    return 0;
}

int VolumeManager::mkdirs(char* path) {
    // Only offer to create directories for paths managed by vold
    if (strncmp(path, "/mnt/media_rw/", 13) == 0) {
        // fs_mkdirs() does symlink checking and relative path enforcement
        return fs_mkdirs(path, 0700);
    } else {
        SLOGE("Failed to find mounted volume for %s", path);
        return -EINVAL;
    }
}

static void do_coldboot(DIR *d, int lvl) {
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while ((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if (fd < 0)
            continue;

        d2 = fdopendir(fd);
        if (d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}

void VolumeManager::coldboot(const char *path) {
    DIR *d = opendir(path);

    if (d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}
int VolumeManager::getDiskFlag(const std::string &path) {
    if (mDebug)
        LOG(DEBUG) << "getdisk flag  path=" << path;
    for (auto disk : mDisks) {
        auto vol = disk->findVolumeByPath(path);
        if (vol != nullptr) {
            return disk->getFlags();
        }
    }
    return 0;
}
