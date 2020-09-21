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

#ifndef AUDIO_PRESENTATION_INFO_H_
#define AUDIO_PRESENTATION_INFO_H_

#include <sstream>
#include <stdint.h>

#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>

namespace android {

enum mastering_indication {
    MASTERING_NOT_INDICATED,
    MASTERED_FOR_STEREO,
    MASTERED_FOR_SURROUND,
    MASTERED_FOR_3D,
    MASTERED_FOR_HEADPHONE,
};

struct AudioPresentation : public RefBase {
    int32_t mPresentationId;
    int32_t mProgramId;
    KeyedVector<String8, String8> mLabels;
    String8 mLanguage;
    int32_t mMasteringIndication;
    bool mAudioDescriptionAvailable;
    bool mSpokenSubtitlesAvailable;
    bool mDialogueEnhancementAvailable;

    AudioPresentation() {
        mPresentationId = -1;
        mProgramId = -1;
        mLanguage = "";
        mMasteringIndication = MASTERING_NOT_INDICATED;
        mAudioDescriptionAvailable = false;
        mSpokenSubtitlesAvailable = false;
        mDialogueEnhancementAvailable = false;
    }
};

typedef Vector<sp<AudioPresentation>> AudioPresentations;

class AudioPresentationInfo : public RefBase {
 public:
    AudioPresentationInfo();

    ~AudioPresentationInfo();

    void addPresentation(sp<AudioPresentation> presentation);

    size_t countPresentations() const;

    const sp<AudioPresentation> getPresentation(size_t index) const;

 private:
    AudioPresentations mPresentations;
};

}  // namespace android

#endif  // AUDIO_PRESENTATION_INFO_H_
