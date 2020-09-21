/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define DEBUG false  // STOPSHIP if true
#include "Log.h"

#include "StatsdStats.h"

#include <android/util/ProtoOutputStream.h>
#include "../stats_log_util.h"
#include "statslog.h"
#include "storage/StorageManager.h"

namespace android {
namespace os {
namespace statsd {

using android::util::FIELD_COUNT_REPEATED;
using android::util::FIELD_TYPE_BOOL;
using android::util::FIELD_TYPE_FLOAT;
using android::util::FIELD_TYPE_INT32;
using android::util::FIELD_TYPE_INT64;
using android::util::FIELD_TYPE_MESSAGE;
using android::util::FIELD_TYPE_STRING;
using android::util::ProtoOutputStream;
using std::lock_guard;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

const int FIELD_ID_BEGIN_TIME = 1;
const int FIELD_ID_END_TIME = 2;
const int FIELD_ID_CONFIG_STATS = 3;
const int FIELD_ID_ATOM_STATS = 7;
const int FIELD_ID_UIDMAP_STATS = 8;
const int FIELD_ID_ANOMALY_ALARM_STATS = 9;
// const int FIELD_ID_PULLED_ATOM_STATS = 10; // The proto is written in stats_log_util.cpp
const int FIELD_ID_LOGGER_ERROR_STATS = 11;
const int FIELD_ID_PERIODIC_ALARM_STATS = 12;
const int FIELD_ID_LOG_LOSS_STATS = 14;
const int FIELD_ID_SYSTEM_SERVER_RESTART = 15;

const int FIELD_ID_ATOM_STATS_TAG = 1;
const int FIELD_ID_ATOM_STATS_COUNT = 2;

const int FIELD_ID_ANOMALY_ALARMS_REGISTERED = 1;
const int FIELD_ID_PERIODIC_ALARMS_REGISTERED = 1;

const int FIELD_ID_LOGGER_STATS_TIME = 1;
const int FIELD_ID_LOGGER_STATS_ERROR_CODE = 2;

const int FIELD_ID_CONFIG_STATS_UID = 1;
const int FIELD_ID_CONFIG_STATS_ID = 2;
const int FIELD_ID_CONFIG_STATS_CREATION = 3;
const int FIELD_ID_CONFIG_STATS_RESET = 19;
const int FIELD_ID_CONFIG_STATS_DELETION = 4;
const int FIELD_ID_CONFIG_STATS_METRIC_COUNT = 5;
const int FIELD_ID_CONFIG_STATS_CONDITION_COUNT = 6;
const int FIELD_ID_CONFIG_STATS_MATCHER_COUNT = 7;
const int FIELD_ID_CONFIG_STATS_ALERT_COUNT = 8;
const int FIELD_ID_CONFIG_STATS_VALID = 9;
const int FIELD_ID_CONFIG_STATS_BROADCAST = 10;
const int FIELD_ID_CONFIG_STATS_DATA_DROP = 11;
const int FIELD_ID_CONFIG_STATS_DUMP_REPORT_TIME = 12;
const int FIELD_ID_CONFIG_STATS_DUMP_REPORT_BYTES = 20;
const int FIELD_ID_CONFIG_STATS_MATCHER_STATS = 13;
const int FIELD_ID_CONFIG_STATS_CONDITION_STATS = 14;
const int FIELD_ID_CONFIG_STATS_METRIC_STATS = 15;
const int FIELD_ID_CONFIG_STATS_ALERT_STATS = 16;
const int FIELD_ID_CONFIG_STATS_METRIC_DIMENSION_IN_CONDITION_STATS = 17;
const int FIELD_ID_CONFIG_STATS_ANNOTATION = 18;
const int FIELD_ID_CONFIG_STATS_ANNOTATION_INT64 = 1;
const int FIELD_ID_CONFIG_STATS_ANNOTATION_INT32 = 2;

const int FIELD_ID_MATCHER_STATS_ID = 1;
const int FIELD_ID_MATCHER_STATS_COUNT = 2;
const int FIELD_ID_CONDITION_STATS_ID = 1;
const int FIELD_ID_CONDITION_STATS_COUNT = 2;
const int FIELD_ID_METRIC_STATS_ID = 1;
const int FIELD_ID_METRIC_STATS_COUNT = 2;
const int FIELD_ID_ALERT_STATS_ID = 1;
const int FIELD_ID_ALERT_STATS_COUNT = 2;

const int FIELD_ID_UID_MAP_CHANGES = 1;
const int FIELD_ID_UID_MAP_BYTES_USED = 2;
const int FIELD_ID_UID_MAP_DROPPED_CHANGES = 3;
const int FIELD_ID_UID_MAP_DELETED_APPS = 4;

const std::map<int, std::pair<size_t, size_t>> StatsdStats::kAtomDimensionKeySizeLimitMap = {
        {android::util::CPU_TIME_PER_UID_FREQ, {6000, 10000}},
};

// TODO: add stats for pulled atoms.
StatsdStats::StatsdStats() {
    mPushedAtomStats.resize(android::util::kMaxPushedAtomId + 1);
    mStartTimeSec = getWallClockSec();
}

StatsdStats& StatsdStats::getInstance() {
    static StatsdStats statsInstance;
    return statsInstance;
}

void StatsdStats::addToIceBoxLocked(shared_ptr<ConfigStats>& stats) {
    // The size of mIceBox grows strictly by one at a time. It won't be > kMaxIceBoxSize.
    if (mIceBox.size() == kMaxIceBoxSize) {
        mIceBox.pop_front();
    }
    mIceBox.push_back(stats);
}

void StatsdStats::noteConfigReceived(
        const ConfigKey& key, int metricsCount, int conditionsCount, int matchersCount,
        int alertsCount, const std::list<std::pair<const int64_t, const int32_t>>& annotations,
        bool isValid) {
    lock_guard<std::mutex> lock(mLock);
    int32_t nowTimeSec = getWallClockSec();

    // If there is an existing config for the same key, icebox the old config.
    noteConfigRemovedInternalLocked(key);

    shared_ptr<ConfigStats> configStats = std::make_shared<ConfigStats>();
    configStats->uid = key.GetUid();
    configStats->id = key.GetId();
    configStats->creation_time_sec = nowTimeSec;
    configStats->metric_count = metricsCount;
    configStats->condition_count = conditionsCount;
    configStats->matcher_count = matchersCount;
    configStats->alert_count = alertsCount;
    configStats->is_valid = isValid;
    for (auto& v : annotations) {
        configStats->annotations.emplace_back(v);
    }

    if (isValid) {
        mConfigStats[key] = configStats;
    } else {
        configStats->deletion_time_sec = nowTimeSec;
        addToIceBoxLocked(configStats);
    }
}

void StatsdStats::noteConfigRemovedInternalLocked(const ConfigKey& key) {
    auto it = mConfigStats.find(key);
    if (it != mConfigStats.end()) {
        int32_t nowTimeSec = getWallClockSec();
        it->second->deletion_time_sec = nowTimeSec;
        addToIceBoxLocked(it->second);
        mConfigStats.erase(it);
    }
}

void StatsdStats::noteConfigRemoved(const ConfigKey& key) {
    lock_guard<std::mutex> lock(mLock);
    noteConfigRemovedInternalLocked(key);
}

void StatsdStats::noteConfigResetInternalLocked(const ConfigKey& key) {
    auto it = mConfigStats.find(key);
    if (it != mConfigStats.end()) {
        it->second->reset_time_sec = getWallClockSec();
    }
}

void StatsdStats::noteConfigReset(const ConfigKey& key) {
    lock_guard<std::mutex> lock(mLock);
    noteConfigResetInternalLocked(key);
}

void StatsdStats::noteLogLost(int64_t timestampNs) {
    lock_guard<std::mutex> lock(mLock);
    if (mLogLossTimestampNs.size() == kMaxLoggerErrors) {
        mLogLossTimestampNs.pop_front();
    }
    mLogLossTimestampNs.push_back(timestampNs);
}

void StatsdStats::noteBroadcastSent(const ConfigKey& key) {
    noteBroadcastSent(key, getWallClockSec());
}

void StatsdStats::noteBroadcastSent(const ConfigKey& key, int32_t timeSec) {
    lock_guard<std::mutex> lock(mLock);
    auto it = mConfigStats.find(key);
    if (it == mConfigStats.end()) {
        ALOGE("Config key %s not found!", key.ToString().c_str());
        return;
    }
    if (it->second->broadcast_sent_time_sec.size() == kMaxTimestampCount) {
        it->second->broadcast_sent_time_sec.pop_front();
    }
    it->second->broadcast_sent_time_sec.push_back(timeSec);
}

void StatsdStats::noteDataDropped(const ConfigKey& key) {
    noteDataDropped(key, getWallClockSec());
}

void StatsdStats::noteDataDropped(const ConfigKey& key, int32_t timeSec) {
    lock_guard<std::mutex> lock(mLock);
    auto it = mConfigStats.find(key);
    if (it == mConfigStats.end()) {
        ALOGE("Config key %s not found!", key.ToString().c_str());
        return;
    }
    if (it->second->data_drop_time_sec.size() == kMaxTimestampCount) {
        it->second->data_drop_time_sec.pop_front();
    }
    it->second->data_drop_time_sec.push_back(timeSec);
}

void StatsdStats::noteMetricsReportSent(const ConfigKey& key, const size_t num_bytes) {
    noteMetricsReportSent(key, num_bytes, getWallClockSec());
}

void StatsdStats::noteMetricsReportSent(const ConfigKey& key, const size_t num_bytes,
                                        int32_t timeSec) {
    lock_guard<std::mutex> lock(mLock);
    auto it = mConfigStats.find(key);
    if (it == mConfigStats.end()) {
        ALOGE("Config key %s not found!", key.ToString().c_str());
        return;
    }
    if (it->second->dump_report_stats.size() == kMaxTimestampCount) {
        it->second->dump_report_stats.pop_front();
    }
    it->second->dump_report_stats.push_back(std::make_pair(timeSec, num_bytes));
}

void StatsdStats::noteUidMapDropped(int deltas) {
    lock_guard<std::mutex> lock(mLock);
    mUidMapStats.dropped_changes += mUidMapStats.dropped_changes + deltas;
}

void StatsdStats::noteUidMapAppDeletionDropped() {
    lock_guard<std::mutex> lock(mLock);
    mUidMapStats.deleted_apps++;
}

void StatsdStats::setUidMapChanges(int changes) {
    lock_guard<std::mutex> lock(mLock);
    mUidMapStats.changes = changes;
}

void StatsdStats::setCurrentUidMapMemory(int bytes) {
    lock_guard<std::mutex> lock(mLock);
    mUidMapStats.bytes_used = bytes;
}

void StatsdStats::noteConditionDimensionSize(const ConfigKey& key, const int64_t& id, int size) {
    lock_guard<std::mutex> lock(mLock);
    // if name doesn't exist before, it will create the key with count 0.
    auto statsIt = mConfigStats.find(key);
    if (statsIt == mConfigStats.end()) {
        return;
    }

    auto& conditionSizeMap = statsIt->second->condition_stats;
    if (size > conditionSizeMap[id]) {
        conditionSizeMap[id] = size;
    }
}

void StatsdStats::noteMetricDimensionSize(const ConfigKey& key, const int64_t& id, int size) {
    lock_guard<std::mutex> lock(mLock);
    // if name doesn't exist before, it will create the key with count 0.
    auto statsIt = mConfigStats.find(key);
    if (statsIt == mConfigStats.end()) {
        return;
    }
    auto& metricsDimensionMap = statsIt->second->metric_stats;
    if (size > metricsDimensionMap[id]) {
        metricsDimensionMap[id] = size;
    }
}

void StatsdStats::noteMetricDimensionInConditionSize(
        const ConfigKey& key, const int64_t& id, int size) {
    lock_guard<std::mutex> lock(mLock);
    // if name doesn't exist before, it will create the key with count 0.
    auto statsIt = mConfigStats.find(key);
    if (statsIt == mConfigStats.end()) {
        return;
    }
    auto& metricsDimensionMap = statsIt->second->metric_dimension_in_condition_stats;
    if (size > metricsDimensionMap[id]) {
        metricsDimensionMap[id] = size;
    }
}

void StatsdStats::noteMatcherMatched(const ConfigKey& key, const int64_t& id) {
    lock_guard<std::mutex> lock(mLock);

    auto statsIt = mConfigStats.find(key);
    if (statsIt == mConfigStats.end()) {
        return;
    }
    statsIt->second->matcher_stats[id]++;
}

void StatsdStats::noteAnomalyDeclared(const ConfigKey& key, const int64_t& id) {
    lock_guard<std::mutex> lock(mLock);
    auto statsIt = mConfigStats.find(key);
    if (statsIt == mConfigStats.end()) {
        return;
    }
    statsIt->second->alert_stats[id]++;
}

void StatsdStats::noteRegisteredAnomalyAlarmChanged() {
    lock_guard<std::mutex> lock(mLock);
    mAnomalyAlarmRegisteredStats++;
}

void StatsdStats::noteRegisteredPeriodicAlarmChanged() {
    lock_guard<std::mutex> lock(mLock);
    mPeriodicAlarmRegisteredStats++;
}

void StatsdStats::updateMinPullIntervalSec(int pullAtomId, long intervalSec) {
    lock_guard<std::mutex> lock(mLock);
    mPulledAtomStats[pullAtomId].minPullIntervalSec = intervalSec;
}

void StatsdStats::notePull(int pullAtomId) {
    lock_guard<std::mutex> lock(mLock);
    mPulledAtomStats[pullAtomId].totalPull++;
}

void StatsdStats::notePullFromCache(int pullAtomId) {
    lock_guard<std::mutex> lock(mLock);
    mPulledAtomStats[pullAtomId].totalPullFromCache++;
}

void StatsdStats::noteAtomLogged(int atomId, int32_t timeSec) {
    lock_guard<std::mutex> lock(mLock);

    if (atomId > android::util::kMaxPushedAtomId) {
        ALOGW("not interested in atom %d", atomId);
        return;
    }

    mPushedAtomStats[atomId]++;
}

void StatsdStats::noteSystemServerRestart(int32_t timeSec) {
    lock_guard<std::mutex> lock(mLock);

    if (mSystemServerRestartSec.size() == kMaxSystemServerRestarts) {
        mSystemServerRestartSec.pop_front();
    }
    mSystemServerRestartSec.push_back(timeSec);
}

void StatsdStats::noteLoggerError(int error) {
    lock_guard<std::mutex> lock(mLock);
    // grows strictly one at a time. so it won't > kMaxLoggerErrors
    if (mLoggerErrors.size() == kMaxLoggerErrors) {
        mLoggerErrors.pop_front();
    }
    mLoggerErrors.push_back(std::make_pair(getWallClockSec(), error));
}

void StatsdStats::reset() {
    lock_guard<std::mutex> lock(mLock);
    resetInternalLocked();
}

void StatsdStats::resetInternalLocked() {
    // Reset the historical data, but keep the active ConfigStats
    mStartTimeSec = getWallClockSec();
    mIceBox.clear();
    std::fill(mPushedAtomStats.begin(), mPushedAtomStats.end(), 0);
    mAnomalyAlarmRegisteredStats = 0;
    mPeriodicAlarmRegisteredStats = 0;
    mLoggerErrors.clear();
    mSystemServerRestartSec.clear();
    mLogLossTimestampNs.clear();
    for (auto& config : mConfigStats) {
        config.second->broadcast_sent_time_sec.clear();
        config.second->data_drop_time_sec.clear();
        config.second->dump_report_stats.clear();
        config.second->annotations.clear();
        config.second->matcher_stats.clear();
        config.second->condition_stats.clear();
        config.second->metric_stats.clear();
        config.second->metric_dimension_in_condition_stats.clear();
        config.second->alert_stats.clear();
    }
}

string buildTimeString(int64_t timeSec) {
    time_t t = timeSec;
    struct tm* tm = localtime(&t);
    char timeBuffer[80];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %I:%M%p", tm);
    return string(timeBuffer);
}

void StatsdStats::dumpStats(FILE* out) const {
    lock_guard<std::mutex> lock(mLock);
    time_t t = mStartTimeSec;
    struct tm* tm = localtime(&t);
    char timeBuffer[80];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %I:%M%p\n", tm);
    fprintf(out, "Stats collection start second: %s\n", timeBuffer);
    fprintf(out, "%lu Config in icebox: \n", (unsigned long)mIceBox.size());
    for (const auto& configStats : mIceBox) {
        fprintf(out,
                "Config {%d_%lld}: creation=%d, deletion=%d, reset=%d, #metric=%d, #condition=%d, "
                "#matcher=%d, #alert=%d,  valid=%d\n",
                configStats->uid, (long long)configStats->id, configStats->creation_time_sec,
                configStats->deletion_time_sec, configStats->reset_time_sec,
                configStats->metric_count,
                configStats->condition_count, configStats->matcher_count, configStats->alert_count,
                configStats->is_valid);

        for (const auto& broadcastTime : configStats->broadcast_sent_time_sec) {
            fprintf(out, "\tbroadcast time: %d\n", broadcastTime);
        }

        for (const auto& dataDropTime : configStats->data_drop_time_sec) {
            fprintf(out, "\tdata drop time: %d\n", dataDropTime);
        }
    }
    fprintf(out, "%lu Active Configs\n", (unsigned long)mConfigStats.size());
    for (auto& pair : mConfigStats) {
        auto& configStats = pair.second;
        fprintf(out,
                "Config {%d-%lld}: creation=%d, deletion=%d, #metric=%d, #condition=%d, "
                "#matcher=%d, #alert=%d,  valid=%d\n",
                configStats->uid, (long long)configStats->id, configStats->creation_time_sec,
                configStats->deletion_time_sec, configStats->metric_count,
                configStats->condition_count, configStats->matcher_count, configStats->alert_count,
                configStats->is_valid);
        for (const auto& annotation : configStats->annotations) {
            fprintf(out, "\tannotation: %lld, %d\n", (long long)annotation.first,
                    annotation.second);
        }

        for (const auto& broadcastTime : configStats->broadcast_sent_time_sec) {
            fprintf(out, "\tbroadcast time: %s(%lld)\n",
                    buildTimeString(broadcastTime).c_str(), (long long)broadcastTime);
        }

        for (const auto& dataDropTime : configStats->data_drop_time_sec) {
            fprintf(out, "\tdata drop time: %s(%lld)\n",
                    buildTimeString(dataDropTime).c_str(), (long long)dataDropTime);
        }

        for (const auto& dump : configStats->dump_report_stats) {
            fprintf(out, "\tdump report time: %s(%lld) bytes: %lld\n",
                    buildTimeString(dump.first).c_str(), (long long)dump.first,
                    (long long)dump.second);
        }

        for (const auto& stats : pair.second->matcher_stats) {
            fprintf(out, "matcher %lld matched %d times\n", (long long)stats.first, stats.second);
        }

        for (const auto& stats : pair.second->condition_stats) {
            fprintf(out, "condition %lld max output tuple size %d\n", (long long)stats.first,
                    stats.second);
        }

        for (const auto& stats : pair.second->condition_stats) {
            fprintf(out, "metrics %lld max output tuple size %d\n", (long long)stats.first,
                    stats.second);
        }

        for (const auto& stats : pair.second->alert_stats) {
            fprintf(out, "alert %lld declared %d times\n", (long long)stats.first, stats.second);
        }
    }
    fprintf(out, "********Disk Usage stats***********\n");
    StorageManager::printStats(out);
    fprintf(out, "********Pushed Atom stats***********\n");
    const size_t atomCounts = mPushedAtomStats.size();
    for (size_t i = 2; i < atomCounts; i++) {
        if (mPushedAtomStats[i] > 0) {
            fprintf(out, "Atom %lu->%d\n", (unsigned long)i, mPushedAtomStats[i]);
        }
    }

    fprintf(out, "********Pulled Atom stats***********\n");
    for (const auto& pair : mPulledAtomStats) {
        fprintf(out, "Atom %d->%ld, %ld, %ld\n", (int)pair.first, (long)pair.second.totalPull,
                (long)pair.second.totalPullFromCache, (long)pair.second.minPullIntervalSec);
    }

    if (mAnomalyAlarmRegisteredStats > 0) {
        fprintf(out, "********AnomalyAlarmStats stats***********\n");
        fprintf(out, "Anomaly alarm registrations: %d\n", mAnomalyAlarmRegisteredStats);
    }

    if (mPeriodicAlarmRegisteredStats > 0) {
        fprintf(out, "********SubscriberAlarmStats stats***********\n");
        fprintf(out, "Subscriber alarm registrations: %d\n", mPeriodicAlarmRegisteredStats);
    }

    fprintf(out, "UID map stats: bytes=%d, changes=%d, deleted=%d, changes lost=%d\n",
            mUidMapStats.bytes_used, mUidMapStats.changes, mUidMapStats.deleted_apps,
            mUidMapStats.dropped_changes);

    for (const auto& error : mLoggerErrors) {
        time_t error_time = error.first;
        struct tm* error_tm = localtime(&error_time);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M%p\n", error_tm);
        fprintf(out, "Logger error %d at %s\n", error.second, buffer);
    }

    for (const auto& restart : mSystemServerRestartSec) {
        fprintf(out, "System server restarts at %s(%lld)\n",
            buildTimeString(restart).c_str(), (long long)restart);
    }

    for (const auto& loss : mLogLossTimestampNs) {
        fprintf(out, "Log loss detected at %lld (elapsedRealtimeNs)\n", (long long)loss);
    }
}

void addConfigStatsToProto(const ConfigStats& configStats, ProtoOutputStream* proto) {
    uint64_t token =
            proto->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED | FIELD_ID_CONFIG_STATS);
    proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_UID, configStats.uid);
    proto->write(FIELD_TYPE_INT64 | FIELD_ID_CONFIG_STATS_ID, (long long)configStats.id);
    proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_CREATION, configStats.creation_time_sec);
    if (configStats.reset_time_sec != 0) {
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_RESET, configStats.reset_time_sec);
    }
    if (configStats.deletion_time_sec != 0) {
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_DELETION,
                     configStats.deletion_time_sec);
    }
    proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_METRIC_COUNT, configStats.metric_count);
    proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_CONDITION_COUNT,
                 configStats.condition_count);
    proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_MATCHER_COUNT, configStats.matcher_count);
    proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_ALERT_COUNT, configStats.alert_count);
    proto->write(FIELD_TYPE_BOOL | FIELD_ID_CONFIG_STATS_VALID, configStats.is_valid);

    for (const auto& broadcast : configStats.broadcast_sent_time_sec) {
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_BROADCAST | FIELD_COUNT_REPEATED,
                     broadcast);
    }

    for (const auto& drop : configStats.data_drop_time_sec) {
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_DATA_DROP | FIELD_COUNT_REPEATED,
                     drop);
    }

    for (const auto& dump : configStats.dump_report_stats) {
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_DUMP_REPORT_TIME |
                     FIELD_COUNT_REPEATED,
                     dump.first);
    }

    for (const auto& dump : configStats.dump_report_stats) {
        proto->write(FIELD_TYPE_INT64 | FIELD_ID_CONFIG_STATS_DUMP_REPORT_BYTES |
                     FIELD_COUNT_REPEATED,
                     (long long)dump.second);
    }

    for (const auto& annotation : configStats.annotations) {
        uint64_t token = proto->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                      FIELD_ID_CONFIG_STATS_ANNOTATION);
        proto->write(FIELD_TYPE_INT64 | FIELD_ID_CONFIG_STATS_ANNOTATION_INT64,
                     (long long)annotation.first);
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONFIG_STATS_ANNOTATION_INT32, annotation.second);
        proto->end(token);
    }

    for (const auto& pair : configStats.matcher_stats) {
        uint64_t tmpToken = proto->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                          FIELD_ID_CONFIG_STATS_MATCHER_STATS);
        proto->write(FIELD_TYPE_INT64 | FIELD_ID_MATCHER_STATS_ID, (long long)pair.first);
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_MATCHER_STATS_COUNT, pair.second);
        proto->end(tmpToken);
    }

    for (const auto& pair : configStats.condition_stats) {
        uint64_t tmpToken = proto->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                          FIELD_ID_CONFIG_STATS_CONDITION_STATS);
        proto->write(FIELD_TYPE_INT64 | FIELD_ID_CONDITION_STATS_ID, (long long)pair.first);
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_CONDITION_STATS_COUNT, pair.second);
        proto->end(tmpToken);
    }

    for (const auto& pair : configStats.metric_stats) {
        uint64_t tmpToken = proto->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                          FIELD_ID_CONFIG_STATS_METRIC_STATS);
        proto->write(FIELD_TYPE_INT64 | FIELD_ID_METRIC_STATS_ID, (long long)pair.first);
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_METRIC_STATS_COUNT, pair.second);
        proto->end(tmpToken);
    }
    for (const auto& pair : configStats.metric_dimension_in_condition_stats) {
        uint64_t tmpToken = proto->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                         FIELD_ID_CONFIG_STATS_METRIC_DIMENSION_IN_CONDITION_STATS);
        proto->write(FIELD_TYPE_INT64 | FIELD_ID_METRIC_STATS_ID, (long long)pair.first);
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_METRIC_STATS_COUNT, pair.second);
        proto->end(tmpToken);
    }

    for (const auto& pair : configStats.alert_stats) {
        uint64_t tmpToken = proto->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                          FIELD_ID_CONFIG_STATS_ALERT_STATS);
        proto->write(FIELD_TYPE_INT64 | FIELD_ID_ALERT_STATS_ID, (long long)pair.first);
        proto->write(FIELD_TYPE_INT32 | FIELD_ID_ALERT_STATS_COUNT, pair.second);
        proto->end(tmpToken);
    }

    proto->end(token);
}

void StatsdStats::dumpStats(std::vector<uint8_t>* output, bool reset) {
    lock_guard<std::mutex> lock(mLock);

    ProtoOutputStream proto;
    proto.write(FIELD_TYPE_INT32 | FIELD_ID_BEGIN_TIME, mStartTimeSec);
    proto.write(FIELD_TYPE_INT32 | FIELD_ID_END_TIME, (int32_t)getWallClockSec());

    for (const auto& configStats : mIceBox) {
        addConfigStatsToProto(*configStats, &proto);
    }

    for (auto& pair : mConfigStats) {
        addConfigStatsToProto(*(pair.second), &proto);
    }

    const size_t atomCounts = mPushedAtomStats.size();
    for (size_t i = 2; i < atomCounts; i++) {
        if (mPushedAtomStats[i] > 0) {
            uint64_t token =
                    proto.start(FIELD_TYPE_MESSAGE | FIELD_ID_ATOM_STATS | FIELD_COUNT_REPEATED);
            proto.write(FIELD_TYPE_INT32 | FIELD_ID_ATOM_STATS_TAG, (int32_t)i);
            proto.write(FIELD_TYPE_INT32 | FIELD_ID_ATOM_STATS_COUNT, mPushedAtomStats[i]);
            proto.end(token);
        }
    }

    for (const auto& pair : mPulledAtomStats) {
        android::os::statsd::writePullerStatsToStream(pair, &proto);
    }

    if (mAnomalyAlarmRegisteredStats > 0) {
        uint64_t token = proto.start(FIELD_TYPE_MESSAGE | FIELD_ID_ANOMALY_ALARM_STATS);
        proto.write(FIELD_TYPE_INT32 | FIELD_ID_ANOMALY_ALARMS_REGISTERED,
                    mAnomalyAlarmRegisteredStats);
        proto.end(token);
    }

    if (mPeriodicAlarmRegisteredStats > 0) {
        uint64_t token = proto.start(FIELD_TYPE_MESSAGE | FIELD_ID_PERIODIC_ALARM_STATS);
        proto.write(FIELD_TYPE_INT32 | FIELD_ID_PERIODIC_ALARMS_REGISTERED,
                    mPeriodicAlarmRegisteredStats);
        proto.end(token);
    }

    uint64_t uidMapToken = proto.start(FIELD_TYPE_MESSAGE | FIELD_ID_UIDMAP_STATS);
    proto.write(FIELD_TYPE_INT32 | FIELD_ID_UID_MAP_CHANGES, mUidMapStats.changes);
    proto.write(FIELD_TYPE_INT32 | FIELD_ID_UID_MAP_BYTES_USED, mUidMapStats.bytes_used);
    proto.write(FIELD_TYPE_INT32 | FIELD_ID_UID_MAP_DROPPED_CHANGES, mUidMapStats.dropped_changes);
    proto.write(FIELD_TYPE_INT32 | FIELD_ID_UID_MAP_DELETED_APPS, mUidMapStats.deleted_apps);
    proto.end(uidMapToken);

    for (const auto& error : mLoggerErrors) {
        uint64_t token = proto.start(FIELD_TYPE_MESSAGE | FIELD_ID_LOGGER_ERROR_STATS |
                                      FIELD_COUNT_REPEATED);
        proto.write(FIELD_TYPE_INT32 | FIELD_ID_LOGGER_STATS_TIME, error.first);
        proto.write(FIELD_TYPE_INT32 | FIELD_ID_LOGGER_STATS_ERROR_CODE, error.second);
        proto.end(token);
    }

    for (const auto& loss : mLogLossTimestampNs) {
        proto.write(FIELD_TYPE_INT64 | FIELD_ID_LOG_LOSS_STATS | FIELD_COUNT_REPEATED,
                    (long long)loss);
    }

    for (const auto& restart : mSystemServerRestartSec) {
        proto.write(FIELD_TYPE_INT32 | FIELD_ID_SYSTEM_SERVER_RESTART | FIELD_COUNT_REPEATED,
                    restart);
    }

    output->clear();
    size_t bufferSize = proto.size();
    output->resize(bufferSize);

    size_t pos = 0;
    auto it = proto.data();
    while (it.readBuffer() != NULL) {
        size_t toRead = it.currentToRead();
        std::memcpy(&((*output)[pos]), it.readBuffer(), toRead);
        pos += toRead;
        it.rp()->move(toRead);
    }

    if (reset) {
        resetInternalLocked();
    }

    VLOG("reset=%d, returned proto size %lu", reset, (unsigned long)bufferSize);
}

}  // namespace statsd
}  // namespace os
}  // namespace android
