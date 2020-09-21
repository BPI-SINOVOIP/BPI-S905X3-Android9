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

#ifndef CAR_PROCFS_DIRECTORY
#define CAR_PROCFS_DIRECTORY

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <string>

namespace procfsinspector {

class Directory {
public:
    class Entry {
    public:
        Entry(std::string parent = "", std::string child = "");

        const std::string& getChild() { return mChild; }
        std::string str();

        bool isEmpty() const;

        operator bool() const {
            return !isEmpty();
        }

        uid_t getOwnerUserId();

    private:
        std::string mParent;
        std::string mChild;
    };

    Directory(const char* path);

    Entry next(unsigned char type = DT_UNKNOWN);

private:
    class Deleter {
    public:
        void operator()(DIR* dir) {
            if (dir) closedir(dir);
        }
    };
    std::string mPath;
    std::unique_ptr<DIR, Deleter> mDirectory;
};

}

#endif // CAR_PROCFS_DIRECTORY