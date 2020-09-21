/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "directory.h"

#include <android-base/stringprintf.h>

procfsinspector::Directory::Directory(const char* path) {
    if (path && path[0]) {
        mPath.assign(path);
        mDirectory.reset(opendir(path));
    }
}

bool procfsinspector::Directory::Entry::isEmpty() const {
    return mParent.empty() && mChild.empty();
}

procfsinspector::Directory::Entry::Entry(std::string parent, std::string child) :
    mParent(parent), mChild(child) {
    if (!isEmpty()) {
        if (mParent.back() != '/') {
            mParent += '/';
        }
    }
}

std::string procfsinspector::Directory::Entry::str() {
    return mParent + mChild;
}

uid_t procfsinspector::Directory::Entry::getOwnerUserId() {
    if (isEmpty()) {
        return -1;
    }
    struct stat buf;
    // fill in stat info for this entry, or return invalid UID on failure
    if (stat(str().c_str(), &buf)) {
        return -1;
    }
    return buf.st_uid;
}

procfsinspector::Directory::Entry procfsinspector::Directory::next(unsigned char type) {
    if (auto dir = mDirectory.get()) {
        dirent *entry = readdir(dir);
        if (entry) {
            // only return entries of the right type (regular file, directory, ...)
            // but always return UNKNOWN entries as it is an allowed wildcard entry
            if (entry->d_type == DT_UNKNOWN ||
                type == DT_UNKNOWN ||
                entry->d_type == type) {
                return Entry(mPath, entry->d_name);
            }
        }
    }

    return Entry();
}
