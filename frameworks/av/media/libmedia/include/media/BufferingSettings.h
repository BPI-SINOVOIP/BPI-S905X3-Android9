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

#ifndef ANDROID_BUFFERING_SETTINGS_H
#define ANDROID_BUFFERING_SETTINGS_H

#include <binder/Parcelable.h>

namespace android {

struct BufferingSettings : public Parcelable {
    static const int kNoMark = -1;

    int mInitialMarkMs;

    // When cached data is above this mark, playback will be resumed if it has been paused
    // due to low cached data.
    int mResumePlaybackMarkMs;

    BufferingSettings();

    status_t writeToParcel(Parcel* parcel) const override;
    status_t readFromParcel(const Parcel* parcel) override;

    String8 toString() const;
};

} // namespace android

// ---------------------------------------------------------------------------

#endif // ANDROID_BUFFERING_SETTINGS_H
