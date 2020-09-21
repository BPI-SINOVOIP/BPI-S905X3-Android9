/*
 * Copyright (C) 2016 The Android Open Source Project
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
#define DEBUG false
#include "Log.h"

#include "report_directory.h"

#include <private/android_filesystem_config.h>
#include <utils/String8.h>

#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

namespace android {
namespace os {
namespace incidentd {

status_t create_directory(const char* directory) {
    struct stat st;
    status_t err = NO_ERROR;
    char* dir = strdup(directory);

    // Skip first slash
    char* d = dir + 1;

    // Create directories, assigning them to the system user
    bool last = false;
    while (!last) {
        d = strchr(d, '/');
        if (d != NULL) {
            *d = '\0';
        } else {
            last = true;
        }
        if (stat(dir, &st) == 0) {
            if (!S_ISDIR(st.st_mode)) {
                err = ALREADY_EXISTS;
                goto done;
            }
        } else {
            ALOGE("No such directory %s, something wrong.", dir);
            err = -1;
            goto done;
        }
        if (!last) {
            *d++ = '/';
        }
    }

    // Ensure that the final directory is owned by the system with 0770. If it isn't
    // we won't write into it.
    if (stat(directory, &st) != 0) {
        ALOGE("No incident reports today. Can't stat: %s", directory);
        err = -errno;
        goto done;
    }
    if ((st.st_mode & 0777) != 0770) {
        ALOGE("No incident reports today. Mode is %0o on report directory %s", st.st_mode,
              directory);
        err = BAD_VALUE;
        goto done;
    }
    if (st.st_uid != AID_INCIDENTD || st.st_gid != AID_INCIDENTD) {
        ALOGE("No incident reports today. Owner is %d and group is %d on report directory %s",
              st.st_uid, st.st_gid, directory);
        err = BAD_VALUE;
        goto done;
    }

done:
    free(dir);
    return err;
}

static bool stat_mtime_cmp(const std::pair<String8, struct stat>& a,
                           const std::pair<String8, struct stat>& b) {
    return a.second.st_mtime < b.second.st_mtime;
}

void clean_directory(const char* directory, off_t maxSize, size_t maxCount) {
    DIR* dir;
    struct dirent* entry;
    struct stat st;

    std::vector<std::pair<String8, struct stat>> files;

    if ((dir = opendir(directory)) == NULL) {
        ALOGE("Couldn't open incident directory: %s", directory);
        return;
    }

    String8 dirbase(directory);
    if (directory[dirbase.size() - 1] != '/') dirbase += "/";

    off_t totalSize = 0;
    size_t totalCount = 0;

    // Enumerate, count and add up size
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        String8 filename = dirbase + entry->d_name;
        if (stat(filename.string(), &st) != 0) {
            ALOGE("Unable to stat file %s", filename.string());
            continue;
        }
        if (!S_ISREG(st.st_mode)) {
            continue;
        }
        files.push_back(std::pair<String8, struct stat>(filename, st));

        totalSize += st.st_size;
        totalCount++;
    }

    closedir(dir);

    // Count or size is less than max, then we're done.
    if (totalSize < maxSize && totalCount < maxCount) {
        return;
    }

    // Oldest files first.
    sort(files.begin(), files.end(), stat_mtime_cmp);

    // Remove files until we're under our limits.
    for (std::vector<std::pair<String8, struct stat>>::iterator it = files.begin();
         it != files.end() && totalSize >= maxSize && totalCount >= maxCount; it++) {
        remove(it->first.string());
        totalSize -= it->second.st_size;
        totalCount--;
    }
}

}  // namespace incidentd
}  // namespace os
}  // namespace android