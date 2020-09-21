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

#pragma once

#include <timestatsproto/TimeStatsHelper.h>
#include <timestatsproto/TimeStatsProtoHeader.h>

#include <ui/FenceTime.h>

#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/Vector.h>

#include <deque>
#include <mutex>
#include <optional>
#include <unordered_map>

using namespace android::surfaceflinger;

namespace android {
class String8;

class TimeStats {
    // TODO(zzyiwei): Bound the timeStatsTracker with weighted LRU
    // static const size_t MAX_NUM_LAYER_RECORDS = 200;
    static const size_t MAX_NUM_TIME_RECORDS = 64;

    struct TimeRecord {
        bool ready = false;
        uint64_t frameNumber = 0;
        nsecs_t postTime = 0;
        nsecs_t latchTime = 0;
        nsecs_t acquireTime = 0;
        nsecs_t desiredTime = 0;
        nsecs_t presentTime = 0;
        std::shared_ptr<FenceTime> acquireFence;
        std::shared_ptr<FenceTime> presentFence;
    };

    struct LayerRecord {
        // This is the index in timeRecords, at which the timestamps for that
        // specific frame are still not fully received. This is not waiting for
        // fences to signal, but rather waiting to receive those fences/timestamps.
        int32_t waitData = -1;
        TimeRecord prevTimeRecord;
        std::deque<TimeRecord> timeRecords;
    };

public:
    static TimeStats& getInstance();
    void parseArgs(bool asProto, const Vector<String16>& args, size_t& index, String8& result);
    void incrementTotalFrames();
    void incrementMissedFrames();
    void incrementClientCompositionFrames();

    void setPostTime(const std::string& layerName, uint64_t frameNumber, nsecs_t postTime);
    void setLatchTime(const std::string& layerName, uint64_t frameNumber, nsecs_t latchTime);
    void setDesiredTime(const std::string& layerName, uint64_t frameNumber, nsecs_t desiredTime);
    void setAcquireTime(const std::string& layerName, uint64_t frameNumber, nsecs_t acquireTime);
    void setAcquireFence(const std::string& layerName, uint64_t frameNumber,
                         const std::shared_ptr<FenceTime>& acquireFence);
    void setPresentTime(const std::string& layerName, uint64_t frameNumber, nsecs_t presentTime);
    void setPresentFence(const std::string& layerName, uint64_t frameNumber,
                         const std::shared_ptr<FenceTime>& presentFence);
    void onDisconnect(const std::string& layerName);
    void clearLayerRecord(const std::string& layerName);
    void removeTimeRecord(const std::string& layerName, uint64_t frameNumber);

private:
    TimeStats() = default;

    bool recordReadyLocked(const std::string& layerName, TimeRecord* timeRecord);
    void flushAvailableRecordsToStatsLocked(const std::string& layerName);

    void enable();
    void disable();
    void clear();
    bool isEnabled();
    void dump(bool asProto, std::optional<uint32_t> maxLayers, String8& result);

    std::atomic<bool> mEnabled = false;
    std::mutex mMutex;
    TimeStatsHelper::TimeStatsGlobal timeStats;
    std::unordered_map<std::string, LayerRecord> timeStatsTracker;
};

} // namespace android
