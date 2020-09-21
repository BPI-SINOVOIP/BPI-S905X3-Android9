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

#ifndef BPF_BPFUTILS_H
#define BPF_BPFUTILS_H

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/unistd.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "android-base/unique_fd.h"
#include "netdutils/Slice.h"
#include "netdutils/StatusOr.h"

#define BPF_PASS 1
#define BPF_DROP 0

namespace android {
namespace bpf {

struct UidTag {
    uint32_t uid;
    uint32_t tag;
};

struct StatsKey {
    uint32_t uid;
    uint32_t tag;
    uint32_t counterSet;
    uint32_t ifaceIndex;
};

struct StatsValue {
    uint64_t rxPackets;
    uint64_t rxBytes;
    uint64_t txPackets;
    uint64_t txBytes;
};

struct Stats {
    uint64_t rxBytes;
    uint64_t rxPackets;
    uint64_t txBytes;
    uint64_t txPackets;
    uint64_t tcpRxPackets;
    uint64_t tcpTxPackets;
};

struct IfaceValue {
    char name[IFNAMSIZ];
};

#ifndef DEFAULT_OVERFLOWUID
#define DEFAULT_OVERFLOWUID 65534
#endif

#define BPF_PATH "/sys/fs/bpf"

// Since we cannot garbage collect the stats map since device boot, we need to make these maps as
// large as possible. The maximum size of number of map entries we can have is depend on the rlimit
// of MEM_LOCK granted to netd. The memory space needed by each map can be calculated by the
// following fomula:
//      elem_size = 40 + roundup(key_size, 8) + roundup(value_size, 8)
//      cost = roundup_pow_of_two(max_entries) * 16 + elem_size * max_entries +
//              elem_size * number_of_CPU
// And the cost of each map currently used is(assume the device have 8 CPUs):
// cookie_tag_map:      key:  8 bytes, value:  8 bytes, cost:  822592 bytes    =   823Kbytes
// uid_counter_set_map: key:  4 bytes, value:  1 bytes, cost:  145216 bytes    =   145Kbytes
// app_uid_stats_map:   key:  4 bytes, value: 32 bytes, cost: 1062784 bytes    =  1063Kbytes
// uid_stats_map:       key: 16 bytes, value: 32 bytes, cost: 1142848 bytes    =  1143Kbytes
// tag_stats_map:       key: 16 bytes, value: 32 bytes, cost: 1142848 bytes    =  1143Kbytes
// iface_index_name_map:key:  4 bytes, value: 16 bytes, cost:   80896 bytes    =    81Kbytes
// iface_stats_map:     key:  4 bytes, value: 32 bytes, cost:   97024 bytes    =    97Kbytes
// dozable_uid_map:     key:  4 bytes, value:  1 bytes, cost:  145216 bytes    =   145Kbytes
// standby_uid_map:     key:  4 bytes, value:  1 bytes, cost:  145216 bytes    =   145Kbytes
// powersave_uid_map:   key:  4 bytes, value:  1 bytes, cost:  145216 bytes    =   145Kbytes
// total:                                                                         4930Kbytes
// It takes maximum 4.9MB kernel memory space if all maps are full, which requires any devices
// running this module to have a memlock rlimit to be larger then 5MB. In the old qtaguid module,
// we don't have a total limit for data entries but only have limitation of tags each uid can have.
// (default is 1024 in kernel);

constexpr const int COOKIE_UID_MAP_SIZE = 10000;
constexpr const int UID_COUNTERSET_MAP_SIZE = 2000;
constexpr const int UID_STATS_MAP_SIZE = 10000;
constexpr const int TAG_STATS_MAP_SIZE = 10000;
constexpr const int IFACE_INDEX_NAME_MAP_SIZE = 1000;
constexpr const int IFACE_STATS_MAP_SIZE = 1000;
constexpr const int UID_OWNER_MAP_SIZE = 2000;

constexpr const char* BPF_EGRESS_PROG_PATH = BPF_PATH "/egress_prog";
constexpr const char* BPF_INGRESS_PROG_PATH = BPF_PATH "/ingress_prog";
constexpr const char* XT_BPF_INGRESS_PROG_PATH = BPF_PATH "/xt_bpf_ingress_prog";
constexpr const char* XT_BPF_EGRESS_PROG_PATH = BPF_PATH "/xt_bpf_egress_prog";

constexpr const char* CGROUP_ROOT_PATH = "/dev/cg2_bpf";

constexpr const char* COOKIE_TAG_MAP_PATH = BPF_PATH "/traffic_cookie_tag_map";
constexpr const char* UID_COUNTERSET_MAP_PATH = BPF_PATH "/traffic_uid_counterSet_map";
constexpr const char* APP_UID_STATS_MAP_PATH = BPF_PATH "/traffic_app_uid_stats_map";
constexpr const char* UID_STATS_MAP_PATH = BPF_PATH "/traffic_uid_stats_map";
constexpr const char* TAG_STATS_MAP_PATH = BPF_PATH "/traffic_tag_stats_map";
constexpr const char* IFACE_INDEX_NAME_MAP_PATH = BPF_PATH "/traffic_iface_index_name_map";
constexpr const char* IFACE_STATS_MAP_PATH = BPF_PATH "/traffic_iface_stats_map";
constexpr const char* DOZABLE_UID_MAP_PATH = BPF_PATH "/traffic_dozable_uid_map";
constexpr const char* STANDBY_UID_MAP_PATH = BPF_PATH "/traffic_standby_uid_map";
constexpr const char* POWERSAVE_UID_MAP_PATH = BPF_PATH "/traffic_powersave_uid_map";

constexpr const int OVERFLOW_COUNTERSET = 2;

constexpr const uint64_t NONEXISTENT_COOKIE = 0;

constexpr const int MINIMUM_API_REQUIRED = 28;

int createMap(bpf_map_type map_type, uint32_t key_size, uint32_t value_size,
              uint32_t max_entries, uint32_t map_flags);
int writeToMapEntry(const base::unique_fd& map_fd, void* key, void* value, uint64_t flags);
int findMapEntry(const base::unique_fd& map_fd, void* key, void* value);
int deleteMapEntry(const base::unique_fd& map_fd, void* key);
int getNextMapKey(const base::unique_fd& map_fd, void* key, void* next_key);
int getFirstMapKey(const base::unique_fd& map_fd, void* firstKey);
int bpfProgLoad(bpf_prog_type prog_type, netdutils::Slice bpf_insns, const char* license,
                uint32_t kern_version, netdutils::Slice bpf_log);
int mapPin(const base::unique_fd& map_fd, const char* pathname);
int mapRetrieve(const char* pathname, uint32_t flags);
int attachProgram(bpf_attach_type type, uint32_t prog_fd, uint32_t cg_fd);
int detachProgram(bpf_attach_type type, uint32_t cg_fd);
uint64_t getSocketCookie(int sockFd);
bool hasBpfSupport();

#define SKIP_IF_BPF_NOT_SUPPORTED     \
    do {                              \
        if (!hasBpfSupport()) return; \
    } while (0)

constexpr int BPF_CONTINUE = 0;
constexpr int BPF_DELETED = 1;

bool operator==(const StatsValue& lhs, const StatsValue& rhs);
bool operator==(const UidTag& lhs, const UidTag& rhs);
bool operator==(const StatsKey& lhs, const StatsKey& rhs);
}  // namespace bpf
}  // namespace android

#endif
