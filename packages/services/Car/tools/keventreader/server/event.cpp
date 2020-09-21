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
#include "event.h"

#include <binder/Parcel.h>
#include <utils/String8.h>
#include <utils/String16.h>

using namespace com::android::car::keventreader;

KeypressEvent::KeypressEvent(const std::string source, uint32_t keycode, bool keydown) {
    this->source = source;
    this->keycode = keycode;
    this->keydown = keydown;
}

status_t KeypressEvent::writeToParcel(Parcel* parcel) const {
    String16 s16 = String16(source.c_str());
    parcel->writeString16(s16);
    parcel->writeUint32(keycode);
    parcel->writeBool(keydown);

    return OK;
}

status_t KeypressEvent::readFromParcel(const Parcel* parcel) {
    String16 s16 = parcel->readString16();
    source = std::string(String8(s16).c_str());
    keycode = parcel->readUint32();
    keydown = parcel->readBool();

    return OK;
}
