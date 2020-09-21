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
#include <android-base/stringprintf.h>
#include <timestatsproto/TimeStatsHelper.h>

#include <array>

#define HISTOGRAM_SIZE 85

using android::base::StringAppendF;
using android::base::StringPrintf;

namespace android {
namespace surfaceflinger {

// Time buckets for histogram, the calculated time deltas will be lower bounded
// to the buckets in this array.
static const std::array<int32_t, HISTOGRAM_SIZE> histogramConfig =
        {0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,
         17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
         34,  36,  38,  40,  42,  44,  46,  48,  50,  54,  58,  62,  66,  70,  74,  78,  82,
         86,  90,  94,  98,  102, 106, 110, 114, 118, 122, 126, 130, 134, 138, 142, 146, 150,
         200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000};

void TimeStatsHelper::Histogram::insert(int32_t delta) {
    if (delta < 0) return;
    // std::lower_bound won't work on out of range values
    if (delta > histogramConfig[HISTOGRAM_SIZE - 1]) {
        hist[histogramConfig[HISTOGRAM_SIZE - 1]]++;
        return;
    }
    auto iter = std::lower_bound(histogramConfig.begin(), histogramConfig.end(), delta);
    hist[*iter]++;
}

float TimeStatsHelper::Histogram::averageTime() const {
    int64_t ret = 0;
    int64_t count = 0;
    for (auto& ele : hist) {
        count += ele.second;
        ret += ele.first * ele.second;
    }
    return static_cast<float>(ret) / count;
}

std::string TimeStatsHelper::Histogram::toString() const {
    std::string result;
    for (int32_t i = 0; i < HISTOGRAM_SIZE; ++i) {
        int32_t bucket = histogramConfig[i];
        int32_t count = (hist.count(bucket) == 0) ? 0 : hist.at(bucket);
        StringAppendF(&result, "%dms=%d ", bucket, count);
    }
    result.back() = '\n';
    return result;
}

std::string TimeStatsHelper::TimeStatsLayer::toString() const {
    std::string result = "";
    StringAppendF(&result, "layerName = %s\n", layerName.c_str());
    StringAppendF(&result, "packageName = %s\n", packageName.c_str());
    StringAppendF(&result, "statsStart = %lld\n", static_cast<long long int>(statsStart));
    StringAppendF(&result, "statsEnd = %lld\n", static_cast<long long int>(statsEnd));
    StringAppendF(&result, "totalFrames= %d\n", totalFrames);
    auto iter = deltas.find("present2present");
    if (iter != deltas.end()) {
        StringAppendF(&result, "averageFPS = %.3f\n", 1000.0 / iter->second.averageTime());
    }
    for (auto& ele : deltas) {
        StringAppendF(&result, "%s histogram is as below:\n", ele.first.c_str());
        StringAppendF(&result, "%s", ele.second.toString().c_str());
    }

    return result;
}

std::string TimeStatsHelper::TimeStatsGlobal::toString(std::optional<uint32_t> maxLayers) const {
    std::string result = "SurfaceFlinger TimeStats:\n";
    StringAppendF(&result, "statsStart = %lld\n", static_cast<long long int>(statsStart));
    StringAppendF(&result, "statsEnd = %lld\n", static_cast<long long int>(statsEnd));
    StringAppendF(&result, "totalFrames= %d\n", totalFrames);
    StringAppendF(&result, "missedFrames= %d\n", missedFrames);
    StringAppendF(&result, "clientCompositionFrames= %d\n", clientCompositionFrames);
    StringAppendF(&result, "TimeStats for each layer is as below:\n");
    const auto dumpStats = generateDumpStats(maxLayers);
    for (auto& ele : dumpStats) {
        StringAppendF(&result, "%s", ele->toString().c_str());
    }

    return result;
}

SFTimeStatsLayerProto TimeStatsHelper::TimeStatsLayer::toProto() const {
    SFTimeStatsLayerProto layerProto;
    layerProto.set_layer_name(layerName);
    layerProto.set_package_name(packageName);
    layerProto.set_stats_start(statsStart);
    layerProto.set_stats_end(statsEnd);
    layerProto.set_total_frames(totalFrames);
    for (auto& ele : deltas) {
        SFTimeStatsDeltaProto* deltaProto = layerProto.add_deltas();
        deltaProto->set_delta_name(ele.first);
        for (auto& histEle : ele.second.hist) {
            SFTimeStatsHistogramBucketProto* histProto = deltaProto->add_histograms();
            histProto->set_render_millis(histEle.first);
            histProto->set_frame_count(histEle.second);
        }
    }
    return layerProto;
}

SFTimeStatsGlobalProto TimeStatsHelper::TimeStatsGlobal::toProto(
        std::optional<uint32_t> maxLayers) const {
    SFTimeStatsGlobalProto globalProto;
    globalProto.set_stats_start(statsStart);
    globalProto.set_stats_end(statsEnd);
    globalProto.set_total_frames(totalFrames);
    globalProto.set_missed_frames(missedFrames);
    globalProto.set_client_composition_frames(clientCompositionFrames);
    const auto dumpStats = generateDumpStats(maxLayers);
    for (auto& ele : dumpStats) {
        SFTimeStatsLayerProto* layerProto = globalProto.add_stats();
        layerProto->CopyFrom(ele->toProto());
    }
    return globalProto;
}

std::vector<TimeStatsHelper::TimeStatsLayer const*>
TimeStatsHelper::TimeStatsGlobal::generateDumpStats(std::optional<uint32_t> maxLayers) const {
    std::vector<TimeStatsLayer const*> dumpStats;
    for (auto& ele : stats) {
        dumpStats.push_back(&ele.second);
    }

    std::sort(dumpStats.begin(), dumpStats.end(),
              [](TimeStatsHelper::TimeStatsLayer const* l,
                 TimeStatsHelper::TimeStatsLayer const* r) {
                  return l->totalFrames > r->totalFrames;
              });

    if (maxLayers && (*maxLayers < dumpStats.size())) {
        dumpStats.resize(*maxLayers);
    }
    return dumpStats;
}

} // namespace surfaceflinger
} // namespace android
