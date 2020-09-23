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
#include <android-base/stringprintf.h>
#include <android-base/logging.h>

#include "logwrap.h"
#include "Hfsplus.h"

#define UNUSED __attribute__((unused))

static char FSCK_HFSPLUS_PATH[] = "/system/bin/fsck_hfsplus";

extern "C" int mount(
    const char *, const char *, const char *,
    unsigned long, const void *);

namespace android {
namespace droidvold {
namespace hfsplus {

int Check(const char *fsPath) {
    bool rw = true;
    if (access(FSCK_HFSPLUS_PATH, X_OK)) {
        LOG(WARNING) << "skipping hfsplus check";
        return 0;
    }

    int pass = 1;
    int rc = 0;
    int status;

    do {
        const char *args[5];
        args[0] = FSCK_HFSPLUS_PATH;
        args[1] = "-p";
        args[2] = "-f";
        args[3] = fsPath;
        args[4] = NULL;

        rc = android_fork_execvp(4, (char **)args, &status, false, true);
        if (rc != 0) {
            LOG(ERROR) << "hfsplus check failed due to logwrap error";
            errno = EIO;
            return -1;
        }

        if (!WIFEXITED(status)) {
            LOG(ERROR) << "hfsplus chedk did not exit properly";
            errno = EIO;
            return -1;
        }

        status = WEXITSTATUS(status);
        switch (status) {
        case 0:
            LOG(INFO) << "hfsplus check completed ok";
            return 0;

        case 8:
            LOG(ERROR) << "hfsplus check failed (not hfsplus filesystem)";
            errno = ENODATA;
            return -1;

        case 4:
            if (pass++ <= 3) {
                LOG(INFO) << "hfsplus modified - rechecking pass " << pass;
                continue;
            }
            LOG(ERROR) << "failed check hfsplus after too many rechecks";
            errno = EIO;
            return -1;

        default:
            LOG(ERROR) << "hfsplus check failed.unknown exit code " << rc;
            errno = EIO;
            return -1;
        }
    } while (0);

    return 0;
}

int Mount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, int ownerUid, int ownerGid,
                 int permMask UNUSED, bool createLost UNUSED) {
    int rc;
    unsigned long flags;
    char mountData[255];

    flags = MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC;
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

    permMask = 0;
    sprintf(mountData, "nls=utf8,uid=%d,gid=%d", ownerUid, ownerGid);

    rc = mount(fsPath, mountPoint, "hfsplus", flags, mountData);
    if (rc && errno == EROFS) {
        LOG(ERROR) << fsPath <<
            " appears to be a read only filesystem - retrying mount ro";
        flags |= MS_RDONLY;
        rc = mount(fsPath, mountPoint, "hfsplus", flags, mountData);
    }

    return rc;
}

int Format(const char *fsPath UNUSED, unsigned int numSectors UNUSED) {
    LOG(WARNING) << "skipping hfsplus format";
    errno = EIO;
    return -1;
}

}  // namespace hfsplus
}  // namespace vold
}  // namespace android
