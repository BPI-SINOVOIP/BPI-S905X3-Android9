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

#define LOG_TAG "APM::AudioCollections"
//#define LOG_NDEBUG 0

#include "AudioCollections.h"
#include "AudioPort.h"
#include "AudioRoute.h"
#include "HwModule.h"
#include "AudioGain.h"

namespace android {

sp<AudioPort> AudioPortVector::findByTagName(const String8 &tagName) const
{
    for (const auto& port : *this) {
        if (port->getTagName() == tagName) {
            return port;
        }
    }
    return nullptr;
}

status_t AudioRouteVector::dump(int fd, int spaces) const
{
    if (isEmpty()) {
        return NO_ERROR;
    }
    const size_t SIZE = 256;
    char buffer[SIZE];

    snprintf(buffer, SIZE, "\n%*sAudio Routes (%zu):\n", spaces, "", size());
    write(fd, buffer, strlen(buffer));
    for (size_t i = 0; i < size(); i++) {
        snprintf(buffer, SIZE, "%*s- Route %zu:\n", spaces, "", i + 1);
        write(fd, buffer, strlen(buffer));
        itemAt(i)->dump(fd, 4);
    }
    return NO_ERROR;
}

} // namespace android
