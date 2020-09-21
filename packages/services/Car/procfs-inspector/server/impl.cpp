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
#include "server.h"

template<typename IntTy>
static bool asNumber(const std::string& s, IntTy *value) {
    IntTy v = 0;
    for (auto&& c : s) {
        if (c < '0' || c > '9') {
            return false;
        } else {
            v = v * 10 + (c - '0');
        }
    }

    if (value) *value = v;

    return true;
}

std::vector<procfsinspector::ProcessInfo> procfsinspector::Impl::readProcessTable() {
    std::vector<procfsinspector::ProcessInfo> processes;

    Directory dir("/proc");
    while (auto entry = dir.next()) {
        pid_t pid;
        if (asNumber(entry.getChild(), &pid)) {
            processes.push_back(ProcessInfo{pid, entry.getOwnerUserId()});
        }
    }

    return processes;
}
