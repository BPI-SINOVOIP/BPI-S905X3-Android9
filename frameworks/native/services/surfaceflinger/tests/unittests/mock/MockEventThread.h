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

#pragma once

#include <gmock/gmock.h>

#include "EventThread.h"

namespace android {
namespace mock {

class EventThread : public android::EventThread {
public:
    EventThread();
    ~EventThread() override;

    MOCK_CONST_METHOD0(createEventConnection, sp<BnDisplayEventConnection>());
    MOCK_METHOD0(onScreenReleased, void());
    MOCK_METHOD0(onScreenAcquired, void());
    MOCK_METHOD2(onHotplugReceived, void(int, bool));
    MOCK_CONST_METHOD1(dump, void(String8&));
    MOCK_METHOD1(setPhaseOffset, void(nsecs_t phaseOffset));
};

} // namespace mock
} // namespace android
