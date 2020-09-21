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
#include "inputsource.h"
#include "defines.h"
#include <utils/Log.h>

#include <fcntl.h>
#include <sstream>
#include <string.h>
#include <unistd.h>

using namespace com::android::car::keventreader;

InputSource::kevent::kevent() {
    bzero(this, sizeof(*this));
}

bool InputSource::kevent::isKeypress() const {
    return type == EV_KEY;
}

bool InputSource::kevent::isKeydown() const {
    return isKeypress() && (value == 1);
}

bool InputSource::kevent::isKeyup() const {
    return isKeypress() && (value == 0);
}

InputSource::InputSource(const char* file) : mFilePath(file), mDescriptor(open(file, O_RDONLY)) {}

InputSource::operator bool() const {
    return descriptor() >= 0;
}

int InputSource::descriptor() const {
    return mDescriptor;
}

std::optional<com::android::car::keventreader::KeypressEvent> InputSource::read() const {
    kevent evt;

    auto cnt = ::read(mDescriptor, &evt, sizeof(evt));

    // the kernel guarantees that we will always be able to read a whole number of events
    if (cnt < static_cast<decltype(cnt)>(sizeof(evt))) {
        return std::nullopt;
    }
    if (!evt.isKeypress()) {
        return std::nullopt;
    }

    ALOGD("input source %s generated code %u (down = %s)",
      mFilePath.c_str(), evt.code, evt.isKeydown() ? "true" : "false");
    return com::android::car::keventreader::KeypressEvent(mFilePath, evt.code, evt.isKeydown());
}

InputSource::~InputSource() {
    if (mDescriptor) ::close(mDescriptor);
}
