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
#ifndef CAR_KEVENTREADER_EVENT
#define CAR_KEVENTREADER_EVENT

#include <binder/Parcelable.h>
#include <string>
#include <string_view>

using namespace android;

namespace com::android::car::keventreader {
    struct KeypressEvent : public Parcelable {
        // required to be callable as KeypressEvent() because Parcelable
        KeypressEvent(const std::string source = "", uint32_t keycode = 0, bool keydown = false);

        std::string source;
        uint32_t keycode;
        bool keydown;

        virtual status_t writeToParcel(Parcel* parcel) const override;
        virtual status_t readFromParcel(const Parcel* parcel) override;
    };
}

#endif //CAR_KEVENTREADER_EVENT
