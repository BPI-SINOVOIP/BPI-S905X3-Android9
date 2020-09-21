/*
 * Copyright (C) 2018 The Android Open Source Project
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
#include "eventgatherer.h"
#include "defines.h"
#include <utils/Log.h>

using namespace com::android::car::keventreader;

EventGatherer::EventGatherer(int argc, const char** argv) {
    for (auto i = 1; i < argc; ++i) {
        auto dev = std::make_unique<InputSource>(argv[i]);
        if (dev && *dev) {
            ALOGD("opened input source file %s", argv[i]);
            mFds.push_back({dev->descriptor(), POLLIN, 0});
            mDevices.emplace(dev->descriptor(), std::move(dev));
        } else {
            ALOGW("failed to open input source file %s", argv[i]);
        }
    }
}

size_t EventGatherer::size() const {
    return mDevices.size();
}

std::vector<com::android::car::keventreader::KeypressEvent> EventGatherer::read() {
    constexpr int FOREVER = -1;
    std::vector<com::android::car::keventreader::KeypressEvent> result;

    int count = poll(&mFds[0], mFds.size(), FOREVER);
    if (count < 0) {
        ALOGE("poll failed: errno = %d", errno);
        return result;
    }

    for (auto& fd : mFds) {
        if (fd.revents != 0) {
            if (fd.revents == POLLIN) {
                if (auto dev = mDevices[fd.fd].get()) {
                    while(auto evt = dev->read()) {
                        result.push_back(*evt);
                    }
                    --count; // found one source of events we care abour
                }
            }
            fd.revents = 0;
            if (count == 0) break; // if we read all events, no point in continuing the scan
        }
    }

    return result;
}
