/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "Utils.h"
#include "Process.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/fs.h>
#include <cutils/properties.h>
#include <private/android_filesystem_config.h>

#include <mutex>
#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdlib.h>
#include <sys/mount.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "logwrap.h"
#include "ext2fs/ext2fs.h"
#include "lib/blkid/blkid.h"

#ifndef UMOUNT_NOFOLLOW
#define UMOUNT_NOFOLLOW    0x00000008  /* Don't follow symlink on umount */
#endif

using android::base::ReadFileToString;
using android::base::StringPrintf;

namespace android {
namespace droidvold {

static const char* kProcFilesystems = "/proc/filesystems";

status_t PrepareDir(const std::string& path, mode_t mode, uid_t uid, gid_t gid) {
    const char* cpath = path.c_str();
    int res = fs_prepare_dir(cpath, mode, uid, gid);

    if (res == 0) {
        return OK;
    } else {
        return -errno;
    }
}

status_t ForceUnmount(const std::string& path) {
    const char* cpath = path.c_str();
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }
    // Apps might still be handling eject request, so wait before
    // we start sending signals
    sleep(5);

    Process::killProcessesWithOpenFiles(cpath, SIGINT);
    sleep(5);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }

    Process::killProcessesWithOpenFiles(cpath, SIGTERM);
    sleep(5);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }

    Process::killProcessesWithOpenFiles(cpath, SIGKILL);
    sleep(5);
    if (!umount2(cpath, UMOUNT_NOFOLLOW) || errno == EINVAL || errno == ENOENT) {
        return OK;
    }

    return -errno;
}

status_t KillProcessesUsingPath(const std::string& path) {
    const char* cpath = path.c_str();
    if (Process::killProcessesWithOpenFiles(cpath, SIGINT) == 0) {
        return OK;
    }
    sleep(5);

    if (Process::killProcessesWithOpenFiles(cpath, SIGTERM) == 0) {
        return OK;
    }
    sleep(5);

    if (Process::killProcessesWithOpenFiles(cpath, SIGKILL) == 0) {
        return OK;
    }
    sleep(5);

    // Send SIGKILL a second time to determine if we've
    // actually killed everyone with open files
    if (Process::killProcessesWithOpenFiles(cpath, SIGKILL) == 0) {
        return OK;
    }
    PLOG(ERROR) << "Failed to kill processes using " << path;
    return -EBUSY;
}

status_t BindMount(const std::string& source, const std::string& target) {
    if (::mount(source.c_str(), target.c_str(), "", MS_BIND, NULL)) {
        PLOG(ERROR) << "Failed to bind mount " << source << " to " << target;
        return -errno;
    }
    return OK;
}

status_t ReadPartMetadata(const std::string& path, std::string& fsType,
        std::string& fsUuid, std::string& fsLabel) {
    blkid_cache cache = NULL;
    const char *devices = path.c_str();

#if 1
    if (blkid_get_cache(&cache, "/dev/null") < 0) {
        PLOG(ERROR) << "blkid get cache failed path=" << path;
        blkid_put_cache(cache);
        return  -errno;
    }

    blkid_dev dev = blkid_get_dev(cache, devices, BLKID_DEV_NORMAL);
    if (dev) {
        blkid_tag_iterate iter;
        const char *type, *value;

        iter = blkid_tag_iterate_begin(dev);
        while (blkid_tag_next(iter, &type, &value) == 0) {
            LOG(DEBUG) << "type=" << type << " value=" << value;

            if (!strcmp(type, "TYPE")) {
                fsType = StringPrintf("%s", value);
            } else if (!strcmp(type, "UUID")) {
                fsUuid = StringPrintf("%s", value);
            } else if (!strcmp(type, "LABEL")) {
                fsLabel = StringPrintf("%s", value);
            }
        }
        blkid_tag_iterate_end(iter);
    }
#endif

    return OK;
}


status_t ForkExecvp(const std::vector<std::string>& args) {
    return ForkExecvp(args, nullptr);
}

status_t ForkExecvp(const std::vector<std::string>& args, security_context_t context) {
    size_t argc = args.size();
    char** argv = (char**) calloc(argc, sizeof(char*));
    for (size_t i = 0; i < argc; i++) {
        argv[i] = (char*) args[i].c_str();
        if (i == 0) {
            LOG(VERBOSE) << args[i];
        } else {
            LOG(VERBOSE) << "    " << args[i];
        }
    }

    if (setexeccon(context)) {
        LOG(ERROR) << "Failed to setexeccon";
        abort();
    }
    status_t res = android_fork_execvp(argc, argv, NULL, false, true);
    if (setexeccon(nullptr)) {
        LOG(ERROR) << "Failed to setexeccon";
        abort();
    }

    free(argv);
    return res;
}

status_t ForkExecvp(const std::vector<std::string>& args,
        std::vector<std::string>& output) {
    return ForkExecvp(args, output, nullptr);
}

status_t ForkExecvp(const std::vector<std::string>& args,
        std::vector<std::string>& output, security_context_t context) {
    std::string cmd;
    for (size_t i = 0; i < args.size(); i++) {
        cmd += args[i] + " ";
        if (i == 0) {
            LOG(VERBOSE) << args[i];
        } else {
            LOG(VERBOSE) << "    " << args[i];
        }
    }
    output.clear();

    if (setexeccon(context)) {
        LOG(ERROR) << "Failed to setexeccon";
        abort();
    }
    FILE* fp = popen(cmd.c_str(), "r");
    if (setexeccon(nullptr)) {
        LOG(ERROR) << "Failed to setexeccon";
        abort();
    }

    if (!fp) {
        PLOG(ERROR) << "Failed to popen " << cmd;
        return -errno;
    }
    char line[1024];
    while (fgets(line, sizeof(line), fp) != nullptr) {
        LOG(VERBOSE) << line;
        output.push_back(std::string(line));
    }
    if (pclose(fp) != 0) {
        PLOG(ERROR) << "Failed to pclose " << cmd;
        return -errno;
    }

    return OK;
}

pid_t ForkExecvpAsync(const std::vector<std::string>& args) {
    size_t argc = args.size();
    char** argv = (char**) calloc(argc + 1, sizeof(char*));
    for (size_t i = 0; i < argc; i++) {
        argv[i] = (char*) args[i].c_str();
        if (i == 0) {
            LOG(VERBOSE) << args[i];
        } else {
            LOG(VERBOSE) << "    " << args[i];
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (execvp(argv[0], argv)) {
            PLOG(ERROR) << "Failed to exec";
        }

        _exit(1);
    }

    if (pid == -1) {
        PLOG(ERROR) << "Failed to exec";
    }

    free(argv);
    return pid;
}

status_t ReadRandomBytes(size_t bytes, std::string& out) {
    out.clear();

    int fd = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (fd == -1) {
        return -errno;
    }

    char buf[BUFSIZ];
    size_t n;
    while ((n = TEMP_FAILURE_RETRY(read(fd, &buf[0], std::min(sizeof(buf), bytes)))) > 0) {
        out.append(buf, n);
        bytes -= n;
    }
    close(fd);

    if (bytes == 0) {
        return OK;
    } else {
        return -EIO;
    }
}

status_t HexToStr(const std::string& hex, std::string& str) {
    str.clear();
    bool even = true;
    char cur = 0;
    for (size_t i = 0; i < hex.size(); i++) {
        int val = 0;
        switch (hex[i]) {
        case ' ': case '-': case ':': continue;
        case 'f': case 'F': val = 15; break;
        case 'e': case 'E': val = 14; break;
        case 'd': case 'D': val = 13; break;
        case 'c': case 'C': val = 12; break;
        case 'b': case 'B': val = 11; break;
        case 'a': case 'A': val = 10; break;
        case '9': val = 9; break;
        case '8': val = 8; break;
        case '7': val = 7; break;
        case '6': val = 6; break;
        case '5': val = 5; break;
        case '4': val = 4; break;
        case '3': val = 3; break;
        case '2': val = 2; break;
        case '1': val = 1; break;
        case '0': val = 0; break;
        default: return -EINVAL;
        }

        if (even) {
            cur = val << 4;
        } else {
            cur += val;
            str.push_back(cur);
            cur = 0;
        }
        even = !even;
    }
    return even ? OK : -EINVAL;
}

static const char* kLookup = "0123456789abcdef";

status_t StrToHex(const std::string& str, std::string& hex) {
    hex.clear();
    for (size_t i = 0; i < str.size(); i++) {
        hex.push_back(kLookup[(str[i] & 0xF0) >> 4]);
        hex.push_back(kLookup[str[i] & 0x0F]);
    }
    return OK;
}

status_t NormalizeHex(const std::string& in, std::string& out) {
    std::string tmp;
    if (HexToStr(in, tmp)) {
        return -EINVAL;
    }
    return StrToHex(tmp, out);
}

uint64_t GetFreeBytes(const std::string& path) {
    struct statvfs sb;
    if (statvfs(path.c_str(), &sb) == 0) {
        return (uint64_t)sb.f_bfree * sb.f_bsize;
    } else {
        return -1;
    }
}

// TODO: borrowed from frameworks/native/libs/diskusage/ which should
// eventually be migrated into system/
static int64_t stat_size(struct stat *s) {
    int64_t blksize = s->st_blksize;
    // count actual blocks used instead of nominal file size
    int64_t size = s->st_blocks * 512;

    if (blksize) {
        /* round up to filesystem block size */
        size = (size + blksize - 1) & (~(blksize - 1));
    }

    return size;
}

// TODO: borrowed from frameworks/native/libs/diskusage/ which should
// eventually be migrated into system/
int64_t calculate_dir_size(int dfd) {
    int64_t size = 0;
    struct stat s;
    DIR *d;
    struct dirent *de;

    d = fdopendir(dfd);
    if (d == NULL) {
        close(dfd);
        return 0;
    }

    while ((de = readdir(d))) {
        const char *name = de->d_name;
        if (fstatat(dfd, name, &s, AT_SYMLINK_NOFOLLOW) == 0) {
            size += stat_size(&s);
        }
        if (de->d_type == DT_DIR) {
            int subfd;

            /* always skip "." and ".." */
            if (name[0] == '.') {
                if (name[1] == 0)
                    continue;
                if ((name[1] == '.') && (name[2] == 0))
                    continue;
            }

            subfd = openat(dfd, name, O_RDONLY | O_DIRECTORY);
            if (subfd >= 0) {
                size += calculate_dir_size(subfd);
            }
        }
    }
    closedir(d);
    return size;
}

uint64_t GetTreeBytes(const std::string& path) {
    int dirfd = open(path.c_str(), O_DIRECTORY, O_RDONLY);
    if (dirfd < 0) {
        PLOG(WARNING) << "Failed to open " << path;
        return -1;
    } else {
        uint64_t res = calculate_dir_size(dirfd);
        close(dirfd);
        return res;
    }
}

bool IsFilesystemSupported(const std::string& fsType) {
    std::string supported;
    if (!ReadFileToString(kProcFilesystems, &supported)) {
        PLOG(ERROR) << "Failed to read supported filesystems";
        return false;
    }
    return supported.find(fsType + "\n") != std::string::npos;
}

status_t WipeBlockDevice(const std::string& path) {
    status_t res = -1;
    const char* c_path = path.c_str();
    unsigned long nr_sec = 0;
    unsigned long long range[2];

    int fd = TEMP_FAILURE_RETRY(open(c_path, O_RDWR | O_CLOEXEC));
    if (fd == -1) {
        PLOG(ERROR) << "Failed to open " << path;
        goto done;
    }

    if ((ioctl(fd, BLKGETSIZE, &nr_sec)) == -1) {
        PLOG(ERROR) << "Failed to determine size of " << path;
        goto done;
    }

    range[0] = 0;
    range[1] = (unsigned long long) nr_sec * 512;

    LOG(INFO) << "About to discard " << range[1] << " on " << path;
    if (ioctl(fd, BLKDISCARD, &range) == 0) {
        LOG(INFO) << "Discard success on " << path;
        res = 0;
    } else {
        PLOG(ERROR) << "Discard failure on " << path;
    }

done:
    close(fd);
    return res;
}

dev_t GetDevice(const std::string& path) {
    struct stat sb;
    if (stat(path.c_str(), &sb)) {
        PLOG(WARNING) << "Failed to stat " << path;
        return 0;
    } else {
        return sb.st_dev;
    }
}

std::string DefaultFstabPath() {
    char hardware[PROPERTY_VALUE_MAX];
    property_get("ro.hardware", hardware, "");
    for (const char* prefix : {"/odm/etc/fstab", "/vendor/etc/fstab", "/fstab"}) {
        std::string fstab_path = StringPrintf("%s.%s", prefix, hardware);
        if (access(fstab_path.c_str(), F_OK) == 0) {
            return fstab_path;
        }
    }
    return std::string();
}

status_t RestoreconRecursive(const std::string& path) {
    LOG(VERBOSE) << "Starting restorecon of " << path;

    // TODO: find a cleaner way of waiting for restorecon to finish
    const char* cpath = path.c_str();
    property_set("selinux.restorecon_recursive", "");
    property_set("selinux.restorecon_recursive", cpath);

    char value[PROPERTY_VALUE_MAX];
    while (true) {
        property_get("selinux.restorecon_recursive", value, "");
        if (strcmp(cpath, value) == 0) {
            break;
        }
        usleep(100000); // 100ms
    }

    LOG(VERBOSE) << "Finished restorecon of " << path;
    return OK;
}

status_t SaneReadLinkAt(int dirfd, const char* path, char* buf, size_t bufsiz) {
    ssize_t len = readlinkat(dirfd, path, buf, bufsiz);
    if (len < 0) {
        return -1;
    } else if (len == (ssize_t) bufsiz) {
        return -1;
    } else {
        buf[len] = '\0';
        return 0;
    }
}

ScopedFd::ScopedFd(int fd) : fd_(fd) {}

ScopedFd::~ScopedFd() {
    close(fd_);
}

ScopedDir::ScopedDir(DIR* dir) : dir_(dir) {}

ScopedDir::~ScopedDir() {
    if (dir_ != nullptr) {
        closedir(dir_);
    }
}

bool IsRunningInEmulator() {
    return property_get_bool("ro.kernel.qemu", 0);
}

}  // namespace vold
}  // namespace android
