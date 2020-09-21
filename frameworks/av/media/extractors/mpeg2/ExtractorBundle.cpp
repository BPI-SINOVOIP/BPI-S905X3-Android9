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

//#define LOG_NDEBUG 0
#define LOG_TAG "MPEG2ExtractorBundle"
#include <utils/Log.h>

#include <media/MediaExtractor.h>
#include "MPEG2PSExtractor.h"
#include "MPEG2TSExtractor.h"

namespace android {

extern "C" {
// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
MediaExtractor::ExtractorDef GETEXTRACTORDEF() {
    return {
        MediaExtractor::EXTRACTORDEF_VERSION,
        UUID("3d1dcfeb-e40a-436d-a574-c2438a555e5f"),
        1,
        "MPEG2-PS/TS Extractor",
        [](
                DataSourceBase *source,
                float *confidence,
                void **,
                MediaExtractor::FreeMetaFunc *) -> MediaExtractor::CreatorFunc {
            if (SniffMPEG2TS(source, confidence)) {
                return [](
                        DataSourceBase *source,
                        void *) -> MediaExtractor* {
                    return new MPEG2TSExtractor(source);};
            } else if (SniffMPEG2PS(source, confidence)) {
                        return [](
                                DataSourceBase *source,
                                void *) -> MediaExtractor* {
                            return new MPEG2PSExtractor(source);};
            }
            return NULL;
        }
    };
}

} // extern "C"

} // namespace android
