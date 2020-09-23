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

#include <linux/kdev_t.h>

#define LOG_TAG "droidVold"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/logging.h>

#include "Iso9660.h"

#define UNUSED __attribute__((unused))

extern "C" int mount(
    const char *, const char *, const char *,
    unsigned long, const void *);

namespace android {
namespace droidvold {
namespace iso9660 {

int Check(const char *fsPath UNUSED) {
    LOG(WARNING) << "skipping iso9660 check";
    return 0;
}

int Mount(const char *fsPath, const char *mountPoint,
                 bool ro UNUSED, bool remount, int ownerUid, int ownerGid,
                 int permMask UNUSED, bool createLost UNUSED) {
    int rc;
    unsigned long flags;
    char mountData[255];

    flags = MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC;
    flags |= MS_RDONLY;
    flags |= (remount ? MS_REMOUNT : 0);

    sprintf(mountData, "utf8,uid=%d,gid=%d",ownerUid, ownerGid);

    rc = mount(fsPath, mountPoint, "iso9660", flags, mountData);
    if (rc !=0) {
        rc = mount(fsPath, mountPoint, "udf", flags, mountData);
    }

    return rc;
}

int Format(const char *fsPath UNUSED, unsigned int numSectors UNUSED) {
    LOG(WARNING) << "skipping iso9660 format";
    errno = EIO;
    return -1;
}

}  // namespace iso9660
}  // namespace vold
}  // namespace android
