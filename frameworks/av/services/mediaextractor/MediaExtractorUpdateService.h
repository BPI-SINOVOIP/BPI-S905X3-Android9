/*
 * Copyright 2018 The Android Open Source Project
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

#ifndef ANDROID_MEDIA_EXTRACTOR_UPDATE_SERVICE_H
#define ANDROID_MEDIA_EXTRACTOR_UPDATE_SERVICE_H

#include <binder/BinderService.h>
#include <android/media/BnMediaExtractorUpdateService.h>

namespace android {
namespace media {

class MediaExtractorUpdateService
    : public BinderService<MediaExtractorUpdateService>, public BnMediaExtractorUpdateService
{
    friend class BinderService<MediaExtractorUpdateService>;
public:
    MediaExtractorUpdateService() : BnMediaExtractorUpdateService() { }
    virtual ~MediaExtractorUpdateService() { }
    static const char* getServiceName() { return "media.extractor.update"; }
    binder::Status loadPlugins(const ::std::string& apkPath);
};

}   // namespace media
}   // namespace android

#endif  // ANDROID_MEDIA_EXTRACTOR_UPDATE_SERVICE_H
