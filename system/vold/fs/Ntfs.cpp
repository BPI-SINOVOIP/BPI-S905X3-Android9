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

#include <cutils/log.h>
#include <cutils/properties.h>
#include <logwrap/logwrap.h>
#include <android-base/stringprintf.h>
#include <android-base/logging.h>

#include "Ntfs.h"
#include "Utils.h"


#define UNUSED __attribute__((unused))

using android::base::StringPrintf;

namespace android {
namespace vold {
namespace ntfs {

static const char* NTFS_3G_PATH = "/system/bin/ntfs-3g";
static const char* NTFSFIX_3G_PATH = "/system/bin/ntfsfix";
static const char* MKNTFS_3G_PATH = "/system/bin/mkntfs";

bool IsSupported() {
    LOG(INFO) << "IsSupported() -ntfs-3g-> " << access(NTFS_3G_PATH, X_OK);
    LOG(INFO) << "IsSupported() -ntfsfix-> " << access(NTFSFIX_3G_PATH, X_OK);
    LOG(INFO) << "IsSupported() -mkntfs-> " << access(MKNTFS_3G_PATH, X_OK);
    LOG(INFO) << "IsSupported() --> " << IsFilesystemSupported("ntfs");
    return IsFilesystemSupported("ntfs");
}


status_t Check(const std::string& source) {
    std::vector<std::string> cmd;
    cmd.push_back(NTFSFIX_3G_PATH);
    cmd.push_back("-n");
    cmd.push_back(source);

    int rc = ForkExecvp(cmd, sFsckUntrustedContext);
    if (rc == 0) {
        LOG(INFO) << "Check OK";
        return 0;
    } else {
        LOG(ERROR) << "Check failed (code " << rc << ")";
        errno = EIO;
        return -1;
    }
}

status_t Mount(const std::string& source, const std::string& target, bool ro,
                bool remount, int ownerUid, int ownerGid, int permMask,
                bool createLost) {

    auto mountData = android::base::StringPrintf(
        "locale=utf8,uid=%d,gid=%d,fmask=%o,dmask=%o",
            ownerUid, ownerGid, permMask, permMask);

    std::vector<std::string> cmd;
    cmd.push_back(NTFS_3G_PATH);
    cmd.push_back(source.c_str());
    cmd.push_back(target.c_str());
    cmd.push_back("-o");
    cmd.push_back(mountData.c_str());

    int rc = ForkExecvp(cmd, sFsckUntrustedContext);
    if (rc != 0) {
        LOG(ERROR) << "ntfs Mount error";
        errno = EIO;
        return -1;
    }
    return rc;
}


status_t Format(const std::string& source/*, unsigned int numSectors UNUSED*/) {

    std::vector<std::string> cmd;
    cmd.push_back(MKNTFS_3G_PATH);
    cmd.push_back("-f");

    /*
    if (numSectors) {
        auto tmp = android::base::StringPrintf("%u", numSectors);
        cmd.push_back("-s");
        cmd.push_back(tmp.c_str());
    }*/

    cmd.push_back(source.c_str());

    int rc = ForkExecvp(cmd);
    if (rc == 0) {
        LOG(INFO) << "Format OK";
        return 0;
    } else {
        LOG(ERROR) << "Format failed (code " << rc << ")";
        errno = EIO;
        return -1;
    }
    return 0;
}

}  // namespace ntfs
}  // namespace vold
}  // namespace android
