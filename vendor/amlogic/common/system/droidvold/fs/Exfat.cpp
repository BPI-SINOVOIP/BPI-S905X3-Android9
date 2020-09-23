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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <linux/kdev_t.h>

#define LOG_TAG "droidVold"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <logwrap/logwrap.h>
#include <android-base/stringprintf.h>
#include <android-base/logging.h>

#include "Exfat.h"

#define UNUSED __attribute__((unused))

static char FSCK_EXFAT_PATH[] = "/system/bin/fsck.exfat";
static char MKEXFAT_PATH[] = "/system/bin/mkfs.exfat";
static char MOUNT_EXFAT_PATH[] = "/system/bin/mount.exfat";

extern "C" int mount(
    const char *, const char *, const char *,
    unsigned long, const void *);

namespace android {
namespace droidvold {
namespace exfat {

int Check(const char *fsPath) {
    bool rw = true;
    if (access(FSCK_EXFAT_PATH, X_OK)) {
        LOG(WARNING) << "skipping exfat check";
        return 0;
    }

    int pass = 1;
    int rc = 0;
    int status;
    do {
        const char *args[3];
        args[0] = FSCK_EXFAT_PATH;
        args[1] = fsPath;
        args[2] = NULL;

        rc = android_fork_execvp(2, (char **)args, &status, false, true);
        if (rc != 0) {
            LOG(ERROR) << "exfat check failed due to logwrap error";
            errno = EIO;
            return -1;
        }

        if (!WIFEXITED(status)) {
            LOG(ERROR) << "exfat check did not exit properly";
            errno = EIO;
            return -1;
        }

        status = WEXITSTATUS(status);
        switch (status) {
        case 0:
            LOG(INFO) << "exfat check completed ok";
            return 0;

        case 2:
            LOG(ERROR) << "exfat check failed (not exfat filesystem)";
            errno = ENODATA;
            return -1;

        default:
            LOG(ERROR) << "exfat check failed.unknown exit code " << status;
            //errno = EIO;
            //We can still mount some disks even if fs check failed.
            return 0;
        }
    } while (0);

    return 0;
}

int Mount(const char *fsPath, const char *mountPoint,
                 bool ro UNUSED, bool remount UNUSED, int ownerUid UNUSED,
                 int ownerGid UNUSED, int permMask UNUSED, bool createLost UNUSED) {
#ifdef HAS_EXFAT_FUSE
    int rc;
    int status;
    do {
        const char *args[4];
        args[0] = MOUNT_EXFAT_PATH;
        args[1] = fsPath;
        args[2] = mountPoint;
        args[3] = NULL;

        rc = android_fork_execvp(3, (char **)args, &status, false, true);
        if (rc != 0) {
            LOG(ERROR) << "exfat mount failed due to logwrap error";
            errno = EIO;
            return -1;
        }

        if (!WIFEXITED(status)) {
            LOG(ERROR) << "exfat mount did not exit properly";
            errno = EIO;
            return -1;
        }

        status = WEXITSTATUS(status);
        switch (status) {
        case 0:
            return 0;   // mount ok

        default:
            LOG(ERROR) << "exfat mount failed.unknown exit code " << rc;
            errno = EIO;
            return -1;
        }
    } while (0);
#else
    int rc;
    unsigned long flags;
    char mountData[255];

    flags = MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC;
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

    sprintf(mountData, "uid=%d,gid=%d,fmask=%o,dmask=%o",
            ownerUid, ownerGid, permMask, permMask);

    rc = mount(fsPath, mountPoint, "exfat", flags, mountData);
    if (rc && errno == EROFS) {
        LOG(ERROR) << fsPath <<
            " appears to be a read only filesystem - retry mount ro";
        flags |= MS_RDONLY;
        rc = mount(fsPath, mountPoint, "exfat", flags, mountData);
    }
#endif
    return rc;
}

int Format(const char *fsPath, unsigned int numSectors) {
    const char *args[11];
    int rc;
    int status;
    args[0] = MKEXFAT_PATH;

    if (numSectors) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%u", numSectors);
        const char *size = tmp;
        args[1] = "-s";
        args[2] = size;
        args[3] = fsPath;
        args[4] = NULL;
        rc = android_fork_execvp(4, (char **)args, &status, false, true);
    } else {
        args[7] = fsPath;
        args[8] = NULL;
        rc = android_fork_execvp(8, (char **)args, &status, false, true);
    }

    if (rc != 0) {
        LOG(ERROR) << "exfat check failed due to logwrap error";
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        LOG(ERROR) << "exfat check did not exit properly";
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);
    if (status == 0) {
        LOG(INFO) << "exfat format ok";
        return 0;
    } else {
        LOG(ERROR) << "exfat format failed.unknown exit code " << rc;
        errno = EIO;
        return -1;
    }
}

}  // namespace exfat
}  // namespace vold
}  // namespace android
