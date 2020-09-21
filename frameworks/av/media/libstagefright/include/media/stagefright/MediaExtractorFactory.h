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

#ifndef MEDIA_EXTRACTOR_FACTORY_H_

#define MEDIA_EXTRACTOR_FACTORY_H_

#include <stdio.h>

#include <media/IMediaExtractor.h>
#include <media/MediaExtractor.h>
#include <utils/List.h>

namespace android {

class DataSource;
struct ExtractorPlugin;

class MediaExtractorFactory {
public:
    static sp<IMediaExtractor> Create(
            const sp<DataSource> &source, const char *mime = NULL);
    static sp<IMediaExtractor> CreateFromService(
            const sp<DataSource> &source, const char *mime = NULL);
    static void LoadPlugins(const ::std::string& apkPath);
    static status_t dump(int fd, const Vector<String16>& args);

private:
    static Mutex gPluginMutex;
    static std::shared_ptr<List<sp<ExtractorPlugin>>> gPlugins;
    static bool gPluginsRegistered;

    static void RegisterExtractorsInApk(
            const char *apkPath, List<sp<ExtractorPlugin>> &pluginList);
    static void RegisterExtractorsInSystem(
            const char *libDirPath, List<sp<ExtractorPlugin>> &pluginList);
    static void RegisterExtractor(
            const sp<ExtractorPlugin> &plugin, List<sp<ExtractorPlugin>> &pluginList);

    static MediaExtractor::CreatorFunc sniff(DataSourceBase *source,
            float *confidence, void **meta, MediaExtractor::FreeMetaFunc *freeMeta,
            sp<ExtractorPlugin> &plugin);

    static void UpdateExtractors(const char *newUpdateApkPath);
};

}  // namespace android

#endif  // MEDIA_EXTRACTOR_FACTORY_H_
