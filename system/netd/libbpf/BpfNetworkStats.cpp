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

#include <inttypes.h>
#include <net/if.h>
#include <string.h>
#include <unordered_set>

#include <utils/Log.h>
#include <utils/misc.h>

#include "android-base/file.h"
#include "android-base/strings.h"
#include "android-base/unique_fd.h"
#include "bpf/BpfMap.h"
#include "bpf/BpfNetworkStats.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "BpfNetworkStats"

namespace android {
namespace bpf {

using netdutils::Status;

static constexpr uint32_t BPF_OPEN_FLAGS = BPF_F_RDONLY;

int bpfGetUidStatsInternal(uid_t uid, Stats* stats,
                           const BpfMap<uint32_t, StatsValue>& appUidStatsMap) {
    auto statsEntry = appUidStatsMap.readValue(uid);
    if (isOk(statsEntry)) {
        stats->rxPackets = statsEntry.value().rxPackets;
        stats->txPackets = statsEntry.value().txPackets;
        stats->rxBytes = statsEntry.value().rxBytes;
        stats->txBytes = statsEntry.value().txBytes;
    }
    return -statsEntry.status().code();
}

int bpfGetUidStats(uid_t uid, Stats* stats) {
    BpfMap<uint32_t, StatsValue> appUidStatsMap(
        mapRetrieve(APP_UID_STATS_MAP_PATH, BPF_OPEN_FLAGS));

    if (!appUidStatsMap.isValid()) {
        int ret = -errno;
        ALOGE("Opening appUidStatsMap(%s) failed: %s", UID_STATS_MAP_PATH, strerror(errno));
        return ret;
    }
    return bpfGetUidStatsInternal(uid, stats, appUidStatsMap);
}

int bpfGetIfaceStatsInternal(const char* iface, Stats* stats,
                             const BpfMap<uint32_t, StatsValue>& ifaceStatsMap,
                             const BpfMap<uint32_t, IfaceValue>& ifaceNameMap) {
    int64_t unknownIfaceBytesTotal = 0;
    stats->tcpRxPackets = -1;
    stats->tcpTxPackets = -1;
    const auto processIfaceStats = [iface, stats, &ifaceNameMap, &unknownIfaceBytesTotal]
                                   (const uint32_t& key,
                                    const BpfMap<uint32_t, StatsValue>& ifaceStatsMap) {
        char ifname[IFNAMSIZ];
        if (getIfaceNameFromMap(ifaceNameMap, ifaceStatsMap, key, ifname, key,
                                &unknownIfaceBytesTotal)) {
            return netdutils::status::ok;
        }
        if (!iface || !strcmp(iface, ifname)) {
            StatsValue statsEntry;
            ASSIGN_OR_RETURN(statsEntry, ifaceStatsMap.readValue(key));
            stats->rxPackets += statsEntry.rxPackets;
            stats->txPackets += statsEntry.txPackets;
            stats->rxBytes += statsEntry.rxBytes;
            stats->txBytes += statsEntry.txBytes;
        }
        return netdutils::status::ok;
    };
    return -ifaceStatsMap.iterate(processIfaceStats).code();
}

int bpfGetIfaceStats(const char* iface, Stats* stats) {
    BpfMap<uint32_t, StatsValue> ifaceStatsMap(mapRetrieve(IFACE_STATS_MAP_PATH, BPF_OPEN_FLAGS));
    int ret;
    if (!ifaceStatsMap.isValid()) {
        ret = -errno;
        ALOGE("get ifaceStats map fd failed: %s", strerror(errno));
        return ret;
    }
    BpfMap<uint32_t, IfaceValue> ifaceIndexNameMap(
        mapRetrieve(IFACE_INDEX_NAME_MAP_PATH, BPF_OPEN_FLAGS));
    if (!ifaceIndexNameMap.isValid()) {
        ret = -errno;
        ALOGE("get ifaceIndexName map fd failed: %s", strerror(errno));
        return ret;
    }
    return bpfGetIfaceStatsInternal(iface, stats, ifaceStatsMap, ifaceIndexNameMap);
}

stats_line populateStatsEntry(const StatsKey& statsKey, const StatsValue& statsEntry,
                              const char* ifname) {
    stats_line newLine;
    strlcpy(newLine.iface, ifname, sizeof(newLine.iface));
    newLine.uid = (int32_t)statsKey.uid;
    newLine.set = (int32_t)statsKey.counterSet;
    newLine.tag = (int32_t)statsKey.tag;
    newLine.rxPackets = statsEntry.rxPackets;
    newLine.txPackets = statsEntry.txPackets;
    newLine.rxBytes = statsEntry.rxBytes;
    newLine.txBytes = statsEntry.txBytes;
    return newLine;
}

int parseBpfNetworkStatsDetailInternal(std::vector<stats_line>* lines,
                                       const std::vector<std::string>& limitIfaces, int limitTag,
                                       int limitUid, const BpfMap<StatsKey, StatsValue>& statsMap,
                                       const BpfMap<uint32_t, IfaceValue>& ifaceMap) {
    int64_t unknownIfaceBytesTotal = 0;
    const auto processDetailUidStats = [lines, &limitIfaces, &limitTag, &limitUid,
                                        &unknownIfaceBytesTotal,
                                        &ifaceMap](const StatsKey& key,
                                                   const BpfMap<StatsKey, StatsValue>& statsMap) {
        char ifname[IFNAMSIZ];
        if (getIfaceNameFromMap(ifaceMap, statsMap, key.ifaceIndex, ifname, key,
                                &unknownIfaceBytesTotal)) {
            return netdutils::status::ok;
        }
        std::string ifnameStr(ifname);
        if (limitIfaces.size() > 0 &&
            std::find(limitIfaces.begin(), limitIfaces.end(), ifnameStr) == limitIfaces.end()) {
            // Nothing matched; skip this line.
            return netdutils::status::ok;
        }
        if (limitTag != TAG_ALL && uint32_t(limitTag) != key.tag) {
            return netdutils::status::ok;
        }
        if (limitUid != UID_ALL && uint32_t(limitUid) != key.uid) {
            return netdutils::status::ok;
        }
        StatsValue statsEntry;
        ASSIGN_OR_RETURN(statsEntry, statsMap.readValue(key));
        lines->push_back(populateStatsEntry(key, statsEntry, ifname));
        return netdutils::status::ok;
    };
    Status res = statsMap.iterate(processDetailUidStats);
    if (!isOk(res)) {
        ALOGE("failed to iterate per uid Stats map for detail traffic stats: %s",
              strerror(res.code()));
        return -res.code();
    }
    return 0;
}

int parseBpfNetworkStatsDetail(std::vector<stats_line>* lines,
                               const std::vector<std::string>& limitIfaces, int limitTag,
                               int limitUid) {
    int ret = 0;
    BpfMap<uint32_t, IfaceValue> ifaceIndexNameMap(
        mapRetrieve(IFACE_INDEX_NAME_MAP_PATH, BPF_OPEN_FLAGS));
    if (!ifaceIndexNameMap.isValid()) {
        ret = -errno;
        ALOGE("get ifaceIndexName map fd failed: %s", strerror(errno));
        return ret;
    }

    // If the caller did not pass in TAG_NONE, read tag data.
    if (limitTag != TAG_NONE) {
        BpfMap<StatsKey, StatsValue> tagStatsMap(mapRetrieve(TAG_STATS_MAP_PATH, BPF_OPEN_FLAGS));
        if (!tagStatsMap.isValid()) {
            ret = -errno;
            ALOGE("get tagStats map fd failed: %s", strerror(errno));
            return ret;
        }
        ret = parseBpfNetworkStatsDetailInternal(lines, limitIfaces, limitTag, limitUid,
                                                 tagStatsMap, ifaceIndexNameMap);
        if (ret) return ret;
    }

    // If the caller did not pass in a specific tag (i.e., if limitTag is TAG_NONE(0) or
    // TAG_ALL(-1)) read UID data.
    if (limitTag == TAG_NONE || limitTag == TAG_ALL) {
        BpfMap<StatsKey, StatsValue> uidStatsMap(mapRetrieve(UID_STATS_MAP_PATH, BPF_OPEN_FLAGS));
        if (!uidStatsMap.isValid()) {
            ret = -errno;
            ALOGE("Opening map fd from %s failed: %s", UID_STATS_MAP_PATH, strerror(errno));
            return ret;
        }
        ret = parseBpfNetworkStatsDetailInternal(lines, limitIfaces, limitTag, limitUid,
                                                 uidStatsMap, ifaceIndexNameMap);
    }
    return ret;
}

int parseBpfNetworkStatsDevInternal(std::vector<stats_line>* lines,
                                    const BpfMap<uint32_t, StatsValue>& statsMap,
                                    const BpfMap<uint32_t, IfaceValue>& ifaceMap) {
    int64_t unknownIfaceBytesTotal = 0;
    const auto processDetailIfaceStats = [lines, &unknownIfaceBytesTotal, &ifaceMap, &statsMap](
                                             const uint32_t& key, const StatsValue& value,
                                             const BpfMap<uint32_t, StatsValue>&) {
        char ifname[IFNAMSIZ];
        if (getIfaceNameFromMap(ifaceMap, statsMap, key, ifname, key, &unknownIfaceBytesTotal)) {
            return netdutils::status::ok;
        }
        StatsKey fakeKey = {
            .uid = (uint32_t)UID_ALL, .counterSet = (uint32_t)SET_ALL, .tag = (uint32_t)TAG_NONE};
        lines->push_back(populateStatsEntry(fakeKey, value, ifname));
        return netdutils::status::ok;
    };
    Status res = statsMap.iterateWithValue(processDetailIfaceStats);
    if (!isOk(res)) {
        ALOGE("failed to iterate per uid Stats map for detail traffic stats: %s",
              strerror(res.code()));
        return -res.code();
    }
    return 0;
}

int parseBpfNetworkStatsDev(std::vector<stats_line>* lines) {
    int ret = 0;
    BpfMap<uint32_t, IfaceValue> ifaceIndexNameMap(
        mapRetrieve(IFACE_INDEX_NAME_MAP_PATH, BPF_OPEN_FLAGS));
    if (!ifaceIndexNameMap.isValid()) {
        ret = -errno;
        ALOGE("get ifaceIndexName map fd failed: %s", strerror(errno));
        return ret;
    }

    BpfMap<uint32_t, StatsValue> ifaceStatsMap(mapRetrieve(IFACE_STATS_MAP_PATH, BPF_OPEN_FLAGS));
    if (!ifaceStatsMap.isValid()) {
        ret = -errno;
        ALOGE("get ifaceStats map fd failed: %s", strerror(errno));
        return ret;
    }
    return parseBpfNetworkStatsDevInternal(lines, ifaceStatsMap, ifaceIndexNameMap);
}

uint64_t combineUidTag(const uid_t uid, const uint32_t tag) {
    return (uint64_t)uid << 32 | tag;
}

}  // namespace bpf
}  // namespace android
