/*
 * Copyright 2016 The Android Open Source Project
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
#include <unistd.h>

#define LOG_TAG "VtsSyscall"
#include <utils/Log.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 2) {
        ALOGE("syscall_exists: invalid argument");
        return -EINVAL;
    }
    int syscall_id = atoi(argv[1]);
    ALOGI("testing syscall [%d]", syscall_id);
    int ret = syscall(syscall_id);
    bool exists = !(ret == -1 && errno == ENOSYS);
    ALOGI("syscall [%d] is %s", syscall_id, exists ? "enabled" : "disabled");
    std::cout << (exists ? 'y' : 'n');
    return 0;
}
