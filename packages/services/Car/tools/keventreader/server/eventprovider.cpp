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
#include "defines.h"
#include "eventprovider.h"
#include <algorithm>
#include <utils/Log.h>

using namespace com::android::car::keventreader;

EventProviderImpl::EventProviderImpl(EventGatherer&& g) : mGatherer(std::move(g)) {}

std::thread EventProviderImpl::startLoop() {
    auto t = std::thread( [this] () -> void {
        while(true) {
            auto events = mGatherer.read();
            {
                std::scoped_lock lock(mMutex);
                for (auto&& cb : mCallbacks) {
                    for (const auto& event : events) {
                        cb->onEvent(event);
                    }
                }
            }
        }
    });
    return t;
}

Status EventProviderImpl::registerCallback(const sp<IEventCallback>& cb) {
    std::scoped_lock lock(mMutex);
    mCallbacks.push_back(cb);
    return Status::ok();
}

Status EventProviderImpl::unregisterCallback(const sp<IEventCallback>& cb) {
    std::scoped_lock lock(mMutex);
    std::remove(mCallbacks.begin(), mCallbacks.end(), cb);
    return Status::ok();
}
