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
#include <stdlib.h>
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
#include <linux/fs.h>
#include <sys/ioctl.h>

#include <linux/kdev_t.h>

#define LOG_TAG "droidVold"

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-base/file.h>
#include <android-base/macros.h>
#include <android-base/stringprintf.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/multiuser.h>

#include <private/android_filesystem_config.h>

#include "Utils.h"

#define PROP_SDCARDFS_DEVICE "ro.sys.sdcardfs"
#define PROP_SDCARDFS_USER "persist.sys.sdcardfs"

using android::base::StringPrintf;

namespace android {
namespace droidvold {
namespace sdcardfs {

static bool sdcardfs_setup(const std::string& source_path, const std::string& dest_path, uid_t fsuid,
                        gid_t fsgid, bool multi_user, userid_t userid, gid_t gid, mode_t mask) {
    std::string opts = android::base::StringPrintf("fsuid=%d,fsgid=%d,%smask=%d,userid=%d,gid=%d",
            fsuid, fsgid, multi_user?"multiuser,":"", mask, userid, gid);

    if (mount(source_path.c_str(), dest_path.c_str(), "sdcardfs",
              MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_NOATIME, opts.c_str()) == -1) {
        PLOG(ERROR) << "failed to mount sdcardfs filesystem";
        return false;
    }

    return true;
}

static bool sdcardfs_setup_bind_remount(const std::string& source_path, const std::string& dest_path,
                                        gid_t gid, mode_t mask) {
    std::string opts = android::base::StringPrintf("mask=%d,gid=%d", mask, gid);

    if (mount(source_path.c_str(), dest_path.c_str(), nullptr,
            MS_BIND, nullptr) != 0) {
        PLOG(ERROR) << "failed to bind mount sdcardfs filesystem";
        return false;
    }

    if (mount(source_path.c_str(), dest_path.c_str(), "none",
            MS_REMOUNT | MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_NOATIME, opts.c_str()) != 0) {
        PLOG(ERROR) << "failed to mount sdcardfs filesystem";
        if (umount2(dest_path.c_str(), MNT_DETACH))
            PLOG(WARNING) << "Failed to unmount bind";
        return false;
    }

    return true;
}


int run_sdcardfs(const std::string& source_path, const std::string& label, uid_t uid,
        gid_t gid, userid_t userid, bool multi_user, bool full_write) {
    std::string dest_path_default = "/mnt/runtime/default/" + label;
    std::string dest_path_read = "/mnt/runtime/read/" + label;
    std::string dest_path_write = "/mnt/runtime/write/" + label;

    umask(0);
    if (multi_user) {
        // Multi-user storage is fully isolated per user, so "other"
        // permissions are completely masked off.
        if (!sdcardfs_setup(source_path, dest_path_default, uid, gid, multi_user, userid,
                                                      AID_SDCARD_RW, 0006)
                || !sdcardfs_setup_bind_remount(dest_path_default, dest_path_read, AID_EVERYBODY, 0027)
                || !sdcardfs_setup_bind_remount(dest_path_default, dest_path_write,
                                                      AID_EVERYBODY, full_write ? 0007 : 0027)) {
            PLOG(ERROR) << "failed to sdcardfs_setup";
        }
    } else {
        // Physical storage is readable by all users on device, but
        // the Android directories are masked off to a single user
        // deep inside attr_from_stat().
        if (!sdcardfs_setup(source_path, dest_path_default, uid, gid, multi_user, userid,
                                                      AID_SDCARD_RW, 0006)
                || !sdcardfs_setup_bind_remount(dest_path_default, dest_path_read,
                                                      AID_EVERYBODY, full_write ? 0027 : 0022)
                || !sdcardfs_setup_bind_remount(dest_path_default, dest_path_write,
                                                      AID_EVERYBODY, full_write ? 0007 : 0022)) {
            PLOG(ERROR) << "failed to sdcardfs_setup";
        }
    }

    return 0;
}


}  // namespace sdcardfs
}  // namespace droidvold
}  // namespace android
