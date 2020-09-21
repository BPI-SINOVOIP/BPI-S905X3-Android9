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

#define LOG_TAG "TrafficController"
#include <inttypes.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <linux/sock_diag.h>
#include <linux/unistd.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <logwrap/logwrap.h>
#include <netdutils/StatusOr.h>

#include <netdutils/Misc.h>
#include <netdutils/Syscalls.h>
#include "TrafficController.h"
#include "bpf/BpfMap.h"
#include "bpf/bpf_shared.h"

#include "DumpWriter.h"
#include "FirewallController.h"
#include "InterfaceController.h"
#include "NetlinkListener.h"
#include "qtaguid/qtaguid.h"

using namespace android::bpf;

namespace android {
namespace net {

using base::StringPrintf;
using base::unique_fd;
using base::Join;
using netdutils::extract;
using netdutils::Slice;
using netdutils::sSyscalls;
using netdutils::Status;
using netdutils::statusFromErrno;
using netdutils::StatusOr;
using netdutils::status::ok;

constexpr int kSockDiagMsgType = SOCK_DIAG_BY_FAMILY;
constexpr int kSockDiagDoneMsgType = NLMSG_DONE;

StatusOr<std::unique_ptr<NetlinkListenerInterface>> makeSkDestroyListener() {
    const auto& sys = sSyscalls.get();
    ASSIGN_OR_RETURN(auto event, sys.eventfd(0, EFD_CLOEXEC));
    const int domain = AF_NETLINK;
    const int type = SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK;
    const int protocol = NETLINK_INET_DIAG;
    ASSIGN_OR_RETURN(auto sock, sys.socket(domain, type, protocol));

    sockaddr_nl addr = {
        .nl_family = AF_NETLINK,
        .nl_groups = 1 << (SKNLGRP_INET_TCP_DESTROY - 1) | 1 << (SKNLGRP_INET_UDP_DESTROY - 1) |
                     1 << (SKNLGRP_INET6_TCP_DESTROY - 1) | 1 << (SKNLGRP_INET6_UDP_DESTROY - 1)};
    RETURN_IF_NOT_OK(sys.bind(sock, addr));

    const sockaddr_nl kernel = {.nl_family = AF_NETLINK};
    RETURN_IF_NOT_OK(sys.connect(sock, kernel));

    std::unique_ptr<NetlinkListenerInterface> listener =
        std::make_unique<NetlinkListener>(std::move(event), std::move(sock));

    return listener;
}

Status changeOwnerAndMode(const char* path, gid_t group, const char* debugName, bool netdOnly) {
    int ret = chown(path, AID_ROOT, group);
    if (ret != 0) return statusFromErrno(errno, StringPrintf("change %s group failed", debugName));

    if (netdOnly) {
        ret = chmod(path, S_IRWXU);
    } else {
        // Allow both netd and system server to obtain map fd from the path.
        // chmod doesn't grant permission to all processes in that group to
        // read/write the bpf map. They still need correct sepolicy to
        // read/write the map.
        ret = chmod(path, S_IRWXU | S_IRGRP);
    }
    if (ret != 0) return statusFromErrno(errno, StringPrintf("change %s mode failed", debugName));
    return netdutils::status::ok;
}

TrafficController::TrafficController() {
    ebpfSupported = hasBpfSupport();
}

Status initialOwnerMap(BpfMap<uint32_t, uint8_t>& map) {
    // Check and delete all the entries from the map in case it is a runtime
    // restart
    const auto deleteAllEntries = [](const uint32_t& key, BpfMap<uint32_t, uint8_t>& map) {
        Status res = map.deleteValue(key);
        if (!isOk(res) && (res.code() == ENOENT)) {
            ALOGE("Failed to delete data(uid=%u): %s\n", key, strerror(res.code()));
        }
        return netdutils::status::ok;
    };
    // It is safe to delete from this map because nothing will concurrently iterate over it:
    // - Nothing in netd will iterate over it because we're holding mOwnerMatchMutex.
    // - Nothing outside netd iterates over it.
    map.iterate(deleteAllEntries);
    uint32_t mapSettingKey = UID_MAP_ENABLED;
    uint8_t defaultMapState = 0;
    return map.writeValue(mapSettingKey, defaultMapState, BPF_NOEXIST);
}

Status TrafficController::initMaps() {
    std::lock_guard<std::mutex> ownerMapGuard(mOwnerMatchMutex);
    RETURN_IF_NOT_OK(
        mCookieTagMap.getOrCreate(COOKIE_UID_MAP_SIZE, COOKIE_TAG_MAP_PATH, BPF_MAP_TYPE_HASH));

    RETURN_IF_NOT_OK(changeOwnerAndMode(COOKIE_TAG_MAP_PATH, AID_NET_BW_ACCT, "CookieTagMap",
                                        false));

    RETURN_IF_NOT_OK(mUidCounterSetMap.getOrCreate(UID_COUNTERSET_MAP_SIZE, UID_COUNTERSET_MAP_PATH,
                                                   BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(UID_COUNTERSET_MAP_PATH, AID_NET_BW_ACCT,
                                        "UidCounterSetMap", false));

    RETURN_IF_NOT_OK(
        mAppUidStatsMap.getOrCreate(UID_STATS_MAP_SIZE, APP_UID_STATS_MAP_PATH, BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(
        changeOwnerAndMode(APP_UID_STATS_MAP_PATH, AID_NET_BW_STATS, "AppUidStatsMap", false));

    RETURN_IF_NOT_OK(
        mUidStatsMap.getOrCreate(UID_STATS_MAP_SIZE, UID_STATS_MAP_PATH, BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(UID_STATS_MAP_PATH, AID_NET_BW_STATS, "UidStatsMap",
                                        false));

    RETURN_IF_NOT_OK(
        mTagStatsMap.getOrCreate(TAG_STATS_MAP_SIZE, TAG_STATS_MAP_PATH, BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(TAG_STATS_MAP_PATH, AID_NET_BW_STATS, "TagStatsMap",
                                        false));

    RETURN_IF_NOT_OK(mIfaceIndexNameMap.getOrCreate(IFACE_INDEX_NAME_MAP_SIZE,
                                                    IFACE_INDEX_NAME_MAP_PATH, BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(IFACE_INDEX_NAME_MAP_PATH, AID_NET_BW_STATS,
                                        "IfaceIndexNameMap", false));

    RETURN_IF_NOT_OK(
        mDozableUidMap.getOrCreate(UID_OWNER_MAP_SIZE, DOZABLE_UID_MAP_PATH, BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(DOZABLE_UID_MAP_PATH, AID_ROOT, "DozableUidMap", true));
    RETURN_IF_NOT_OK(initialOwnerMap(mDozableUidMap));

    RETURN_IF_NOT_OK(
        mStandbyUidMap.getOrCreate(UID_OWNER_MAP_SIZE, STANDBY_UID_MAP_PATH, BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(STANDBY_UID_MAP_PATH, AID_ROOT, "StandbyUidMap", true));
    RETURN_IF_NOT_OK(initialOwnerMap(mStandbyUidMap));

    RETURN_IF_NOT_OK(mPowerSaveUidMap.getOrCreate(UID_OWNER_MAP_SIZE, POWERSAVE_UID_MAP_PATH,
                                                  BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(POWERSAVE_UID_MAP_PATH, AID_ROOT, "PowerSaveUidMap", true));
    RETURN_IF_NOT_OK(initialOwnerMap(mPowerSaveUidMap));

    RETURN_IF_NOT_OK(
        mIfaceStatsMap.getOrCreate(IFACE_STATS_MAP_SIZE, IFACE_STATS_MAP_PATH, BPF_MAP_TYPE_HASH));
    RETURN_IF_NOT_OK(changeOwnerAndMode(IFACE_STATS_MAP_PATH, AID_NET_BW_STATS, "IfaceStatsMap",
                                        false));
    return netdutils::status::ok;
}

Status TrafficController::start() {

    if (!ebpfSupported) {
        return netdutils::status::ok;
    }

    /* When netd restart from a crash without total system reboot, the program
     * is still attached to the cgroup, detach it so the program can be freed
     * and we can load and attach new program into the target cgroup.
     *
     * TODO: Scrape existing socket when run-time restart and clean up the map
     * if the socket no longer exist
     */

    RETURN_IF_NOT_OK(initMaps());

    // Fetch the list of currently-existing interfaces. At this point NetlinkHandler is
    // already running, so it will call addInterface() when any new interface appears.
    std::map<std::string, uint32_t> ifacePairs;
    ASSIGN_OR_RETURN(ifacePairs, InterfaceController::getIfaceList());
    for (const auto& ifacePair:ifacePairs) {
        addInterface(ifacePair.first.c_str(), ifacePair.second);
    }


    auto result = makeSkDestroyListener();
    if (!isOk(result)) {
        ALOGE("Unable to create SkDestroyListener: %s", toString(result).c_str());
    } else {
        mSkDestroyListener = std::move(result.value());
    }
    // Rx handler extracts nfgenmsg looks up and invokes registered dispatch function.
    const auto rxHandler = [this](const nlmsghdr&, const Slice msg) {
        inet_diag_msg diagmsg = {};
        if (extract(msg, diagmsg) < sizeof(inet_diag_msg)) {
            ALOGE("unrecognized netlink message: %s", toString(msg).c_str());
            return;
        }
        uint64_t sock_cookie = static_cast<uint64_t>(diagmsg.id.idiag_cookie[0]) |
                               (static_cast<uint64_t>(diagmsg.id.idiag_cookie[1]) << 32);

        mCookieTagMap.deleteValue(sock_cookie);
    };
    expectOk(mSkDestroyListener->subscribe(kSockDiagMsgType, rxHandler));

    // In case multiple netlink message comes in as a stream, we need to handle the rxDone message
    // properly.
    const auto rxDoneHandler = [](const nlmsghdr&, const Slice msg) {
        // Ignore NLMSG_DONE  messages
        inet_diag_msg diagmsg = {};
        extract(msg, diagmsg);
    };
    expectOk(mSkDestroyListener->subscribe(kSockDiagDoneMsgType, rxDoneHandler));

    int* status = nullptr;

    std::vector<const char*> prog_args{
        "/system/bin/bpfloader",
    };
    int ret = access(BPF_INGRESS_PROG_PATH, R_OK);
    if (ret != 0 && errno == ENOENT) {
        prog_args.push_back((char*)"-i");
    }
    ret = access(BPF_EGRESS_PROG_PATH, R_OK);
    if (ret != 0 && errno == ENOENT) {
        prog_args.push_back((char*)"-e");
    }
    ret = access(XT_BPF_INGRESS_PROG_PATH, R_OK);
    if (ret != 0 && errno == ENOENT) {
        prog_args.push_back((char*)"-p");
    }
    ret = access(XT_BPF_EGRESS_PROG_PATH, R_OK);
    if (ret != 0 && errno == ENOENT) {
        prog_args.push_back((char*)"-m");
    }

    if (prog_args.size() == 1) {
        // all program are loaded already.
        return netdutils::status::ok;
    }

    prog_args.push_back(nullptr);
    ret = android_fork_execvp(prog_args.size(), (char**)prog_args.data(), status, false, true);
    if (ret) {
        ret = errno;
        ALOGE("failed to execute %s: %s", prog_args[0], strerror(errno));
        return statusFromErrno(ret, "run bpf loader failed");
    }
    return netdutils::status::ok;
}

int TrafficController::tagSocket(int sockFd, uint32_t tag, uid_t uid) {
    if (!ebpfSupported) {
        if (legacy_tagSocket(sockFd, tag, uid)) return -errno;
        return 0;
    }

    uint64_t sock_cookie = getSocketCookie(sockFd);
    if (sock_cookie == NONEXISTENT_COOKIE) return -errno;
    UidTag newKey = {.uid = (uint32_t)uid, .tag = tag};

    // Update the tag information of a socket to the cookieUidMap. Use BPF_ANY
    // flag so it will insert a new entry to the map if that value doesn't exist
    // yet. And update the tag if there is already a tag stored. Since the eBPF
    // program in kernel only read this map, and is protected by rcu read lock. It
    // should be fine to cocurrently update the map while eBPF program is running.
    Status res = mCookieTagMap.writeValue(sock_cookie, newKey, BPF_ANY);
    if (!isOk(res)) {
        ALOGE("Failed to tag the socket: %s, fd: %d", strerror(res.code()),
              mCookieTagMap.getMap().get());
    }
    return -res.code();
}

int TrafficController::untagSocket(int sockFd) {
    if (!ebpfSupported) {
        if (legacy_untagSocket(sockFd)) return -errno;
        return 0;
    }
    uint64_t sock_cookie = getSocketCookie(sockFd);

    if (sock_cookie == NONEXISTENT_COOKIE) return -errno;
    Status res = mCookieTagMap.deleteValue(sock_cookie);
    if (!isOk(res)) {
        ALOGE("Failed to untag socket: %s\n", strerror(res.code()));
    }
    return -res.code();
}

int TrafficController::setCounterSet(int counterSetNum, uid_t uid) {
    if (counterSetNum < 0 || counterSetNum >= OVERFLOW_COUNTERSET) return -EINVAL;
    Status res;
    if (!ebpfSupported) {
        if (legacy_setCounterSet(counterSetNum, uid)) return -errno;
        return 0;
    }

    // The default counter set for all uid is 0, so deleting the current counterset for that uid
    // will automatically set it to 0.
    if (counterSetNum == 0) {
        Status res = mUidCounterSetMap.deleteValue(uid);
        if (isOk(res) || (!isOk(res) && res.code() == ENOENT)) {
            return 0;
        } else {
            ALOGE("Failed to delete the counterSet: %s\n", strerror(res.code()));
            return -res.code();
        }
    }
    uint8_t tmpCounterSetNum = (uint8_t)counterSetNum;
    res = mUidCounterSetMap.writeValue(uid, tmpCounterSetNum, BPF_ANY);
    if (!isOk(res)) {
        ALOGE("Failed to set the counterSet: %s, fd: %d", strerror(res.code()),
              mUidCounterSetMap.getMap().get());
        return -res.code();
    }
    return 0;
}

int TrafficController::deleteTagData(uint32_t tag, uid_t uid) {

    if (!ebpfSupported) {
        if (legacy_deleteTagData(tag, uid)) return -errno;
        return 0;
    }

    // First we go through the cookieTagMap to delete the target uid tag combination. Or delete all
    // the tags related to the uid if the tag is 0.
    const auto deleteMatchedCookieEntries = [uid, tag](const uint64_t& key, const UidTag& value,
                                                       BpfMap<uint64_t, UidTag>& map) {
        if (value.uid == uid && (value.tag == tag || tag == 0)) {
            Status res = map.deleteValue(key);
            if (isOk(res) || (res.code() == ENOENT)) {
                return netdutils::status::ok;
            }
            ALOGE("Failed to delete data(cookie = %" PRIu64 "): %s\n", key, strerror(res.code()));
        }
        // Move forward to next cookie in the map.
        return netdutils::status::ok;
    };
    mCookieTagMap.iterateWithValue(deleteMatchedCookieEntries);
    // Now we go through the Tag stats map and delete the data entry with correct uid and tag
    // combination. Or all tag stats under that uid if the target tag is 0.
    const auto deleteMatchedUidTagEntries = [uid, tag](const StatsKey& key,
                                                       BpfMap<StatsKey, StatsValue>& map) {
        if (key.uid == uid && (key.tag == tag || tag == 0)) {
            Status res = map.deleteValue(key);
            if (isOk(res) || (res.code() == ENOENT)) {
                //Entry is deleted, use the current key to get a new nextKey;
                return netdutils::status::ok;
            }
            ALOGE("Failed to delete data(uid=%u, tag=%u): %s\n", key.uid, key.tag,
                  strerror(res.code()));
        }
        return netdutils::status::ok;
    };
    mTagStatsMap.iterate(deleteMatchedUidTagEntries);
    // If the tag is not zero, we already deleted all the data entry required. If tag is 0, we also
    // need to delete the stats stored in uidStatsMap and counterSet map.
    if (tag != 0) return 0;

    Status res = mUidCounterSetMap.deleteValue(uid);
    if (!isOk(res) && res.code() != ENOENT) {
        ALOGE("Failed to delete counterSet data(uid=%u, tag=%u): %s\n", uid, tag,
              strerror(res.code()));
    }
    mUidStatsMap.iterate(deleteMatchedUidTagEntries);

    auto deleteAppUidStatsEntry = [uid](const uint32_t& key, BpfMap<uint32_t, StatsValue>& map) {
        if (key == uid) {
            Status res = map.deleteValue(key);
            if (isOk(res) || (res.code() == ENOENT)) {
                return netdutils::status::ok;
            }
            ALOGE("Failed to delete data(uid=%u): %s", key, strerror(res.code()));
        }
        return netdutils::status::ok;
    };
    mAppUidStatsMap.iterate(deleteAppUidStatsEntry);
    return 0;
}

int TrafficController::addInterface(const char* name, uint32_t ifaceIndex) {
    if (!ebpfSupported) return 0;

    IfaceValue iface;
    if (ifaceIndex == 0) {
        ALOGE("Unknown interface %s(%d)", name, ifaceIndex);
        return -1;
    }

    strlcpy(iface.name, name, sizeof(IfaceValue));
    Status res = mIfaceIndexNameMap.writeValue(ifaceIndex, iface, BPF_ANY);
    if (!isOk(res)) {
        ALOGE("Failed to add iface %s(%d): %s", name, ifaceIndex, strerror(res.code()));
        return -res.code();
    }
    return 0;
}

Status TrafficController::updateOwnerMapEntry(BpfMap<uint32_t, uint8_t>& map, uid_t uid,
                                              FirewallRule rule, FirewallType type) {
    if (uid == UID_MAP_ENABLED) {
        return statusFromErrno(-EINVAL, "This uid is reserved for map state");
    }

    if ((rule == ALLOW && type == WHITELIST) || (rule == DENY && type == BLACKLIST)) {
        uint8_t flag = (type == WHITELIST) ? BPF_PASS : BPF_DROP;
        RETURN_IF_NOT_OK(map.writeValue(uid, flag, BPF_ANY));
    } else if ((rule == ALLOW && type == BLACKLIST) || (rule == DENY && type == WHITELIST)) {
        RETURN_IF_NOT_OK(map.deleteValue(uid));
    } else {
        //Cannot happen.
        return statusFromErrno(-EINVAL, "");
    }
    return netdutils::status::ok;
}

int TrafficController::changeUidOwnerRule(ChildChain chain, uid_t uid, FirewallRule rule,
                                          FirewallType type) {
    std::lock_guard<std::mutex> guard(mOwnerMatchMutex);
    if (!ebpfSupported) {
        ALOGE("bpf is not set up, should use iptables rule");
        return -ENOSYS;
    }
    Status res;
    switch (chain) {
        case DOZABLE:
            res = updateOwnerMapEntry(mDozableUidMap, uid, rule, type);
            break;
        case STANDBY:
            res = updateOwnerMapEntry(mStandbyUidMap, uid, rule, type);
            break;
        case POWERSAVE:
            res = updateOwnerMapEntry(mPowerSaveUidMap, uid, rule, type);
            break;
        case NONE:
        default:
            return -EINVAL;
    }
    if (!isOk(res)) {
        ALOGE("change uid(%u) rule of %d failed: %s, rule: %d, type: %d", uid, chain,
              res.msg().c_str(), rule, type);
        return -res.code();
    }
    return 0;
}

Status TrafficController::replaceUidsInMap(BpfMap<uint32_t, uint8_t>& map,
                                           const std::vector<int32_t>& uids, FirewallRule rule,
                                           FirewallType type) {
    std::set<int32_t> uidSet(uids.begin(), uids.end());
    std::vector<uint32_t> uidsToDelete;
    auto getUidsToDelete = [&uidsToDelete, &uidSet](const uint32_t& key,
                                                    const BpfMap<uint32_t, uint8_t>&) {
        if (key != UID_MAP_ENABLED && uidSet.find((int32_t)key) == uidSet.end()) {
            uidsToDelete.push_back(key);
        }
        return netdutils::status::ok;
    };
    RETURN_IF_NOT_OK(map.iterate(getUidsToDelete));

    for(auto uid : uidsToDelete) {
        RETURN_IF_NOT_OK(map.deleteValue(uid));
    }

    for (auto uid : uids) {
        RETURN_IF_NOT_OK(updateOwnerMapEntry(map, uid, rule, type));
    }
    return netdutils::status::ok;
}

int TrafficController::replaceUidOwnerMap(const std::string& name, bool isWhitelist,
                                          const std::vector<int32_t>& uids) {
    std::lock_guard<std::mutex> guard(mOwnerMatchMutex);
    FirewallRule rule;
    FirewallType type;
    if (isWhitelist) {
        type = WHITELIST;
        rule = ALLOW;
    } else {
        type = BLACKLIST;
        rule = DENY;
    }
    Status res;
    if (!name.compare(FirewallController::LOCAL_DOZABLE)) {
        res = replaceUidsInMap(mDozableUidMap, uids, rule, type);
    } else if (!name.compare(FirewallController::LOCAL_STANDBY)) {
        res = replaceUidsInMap(mStandbyUidMap, uids, rule, type);
    } else if (!name.compare(FirewallController::LOCAL_POWERSAVE)) {
        res = replaceUidsInMap(mPowerSaveUidMap, uids, rule, type);
    } else {
        ALOGE("unknown chain name: %s", name.c_str());
        return -EINVAL;
    }
    if (!isOk(res)) {
        ALOGE("Failed to clean up chain: %s: %s", name.c_str(), res.msg().c_str());
        return -res.code();
    }
    return 0;
}

int TrafficController::toggleUidOwnerMap(ChildChain chain, bool enable) {
    std::lock_guard<std::mutex> guard(mOwnerMatchMutex);
    uint32_t keyUid = UID_MAP_ENABLED;
    uint8_t mapState = enable ? 1 : 0;
    Status res;
    switch (chain) {
        case DOZABLE:
            res = mDozableUidMap.writeValue(keyUid, mapState, BPF_EXIST);
            break;
        case STANDBY:
            res = mStandbyUidMap.writeValue(keyUid, mapState, BPF_EXIST);
            break;
        case POWERSAVE:
            res = mPowerSaveUidMap.writeValue(keyUid, mapState, BPF_EXIST);
            break;
        default:
            return -EINVAL;
    }
    if (!isOk(res)) {
        ALOGE("Failed to toggleUidOwnerMap(%d): %s", chain, res.msg().c_str());
    }
    return -res.code();
}

bool TrafficController::checkBpfStatsEnable() {
    return ebpfSupported;
}

std::string getProgramStatus(const char *path) {
    int ret = access(path, R_OK);
    if (ret == 0) {
        return StringPrintf("OK");
    }
    if (ret != 0 && errno == ENOENT) {
        return StringPrintf("program is missing at: %s", path);
    }
    return StringPrintf("check Program %s error: %s", path, strerror(errno));
}

std::string getMapStatus(const base::unique_fd& map_fd, const char* path) {
    if (map_fd.get() < 0) {
        return StringPrintf("map fd lost");
    }
    if (access(path, F_OK) != 0) {
        return StringPrintf("map not pinned to location: %s", path);
    }
    return StringPrintf("OK");
}

void dumpBpfMap(std::string mapName, DumpWriter& dw, const std::string& header) {
    dw.blankline();
    dw.println("%s:", mapName.c_str());
    if(!header.empty()) {
        dw.println(header.c_str());
    }
}

const String16 TrafficController::DUMP_KEYWORD = String16("trafficcontroller");

void TrafficController::dump(DumpWriter& dw, bool verbose) {
    std::lock_guard<std::mutex> ownerMapGuard(mOwnerMatchMutex);
    dw.incIndent();
    dw.println("TrafficController");

    dw.incIndent();
    dw.println("BPF module status: %s", ebpfSupported? "ON" : "OFF");

    if (!ebpfSupported)
        return;

    dw.blankline();
    dw.println("mCookieTagMap status: %s",
               getMapStatus(mCookieTagMap.getMap(), COOKIE_TAG_MAP_PATH).c_str());
    dw.println("mUidCounterSetMap status: %s",
               getMapStatus(mUidCounterSetMap.getMap(), UID_COUNTERSET_MAP_PATH).c_str());
    dw.println("mAppUidStatsMap status: %s",
               getMapStatus(mAppUidStatsMap.getMap(), APP_UID_STATS_MAP_PATH).c_str());
    dw.println("mUidStatsMap status: %s",
               getMapStatus(mUidStatsMap.getMap(), UID_STATS_MAP_PATH).c_str());
    dw.println("mTagStatsMap status: %s",
               getMapStatus(mTagStatsMap.getMap(), TAG_STATS_MAP_PATH).c_str());
    dw.println("mIfaceIndexNameMap status: %s",
               getMapStatus(mIfaceIndexNameMap.getMap(), IFACE_INDEX_NAME_MAP_PATH).c_str());
    dw.println("mIfaceStatsMap status: %s",
               getMapStatus(mIfaceStatsMap.getMap(), IFACE_STATS_MAP_PATH).c_str());
    dw.println("mDozableUidMap status: %s",
               getMapStatus(mDozableUidMap.getMap(), DOZABLE_UID_MAP_PATH).c_str());
    dw.println("mStandbyUidMap status: %s",
               getMapStatus(mStandbyUidMap.getMap(), STANDBY_UID_MAP_PATH).c_str());
    dw.println("mPowerSaveUidMap status: %s",
               getMapStatus(mPowerSaveUidMap.getMap(), POWERSAVE_UID_MAP_PATH).c_str());

    dw.blankline();
    dw.println("Cgroup ingress program status: %s",
               getProgramStatus(BPF_INGRESS_PROG_PATH).c_str());
    dw.println("Cgroup egress program status: %s", getProgramStatus(BPF_EGRESS_PROG_PATH).c_str());
    dw.println("xt_bpf ingress program status: %s",
               getProgramStatus(XT_BPF_INGRESS_PROG_PATH).c_str());
    dw.println("xt_bpf egress program status: %s",
               getProgramStatus(XT_BPF_EGRESS_PROG_PATH).c_str());

    if(!verbose) return;

    dw.blankline();
    dw.println("BPF map content:");

    dw.incIndent();

    // Print CookieTagMap content.
    dumpBpfMap("mCookieTagMap", dw, "");
    const auto printCookieTagInfo = [&dw](const uint64_t& key, const UidTag& value,
                                          const BpfMap<uint64_t, UidTag>&) {
        dw.println("cookie=%" PRIu64 " tag=0x%x uid=%u", key, value.tag, value.uid);
        return netdutils::status::ok;
    };
    Status res = mCookieTagMap.iterateWithValue(printCookieTagInfo);
    if (!isOk(res)) {
        dw.println("mCookieTagMap print end with error: %s", res.msg().c_str());
    }

    // Print UidCounterSetMap Content
    dumpBpfMap("mUidCounterSetMap", dw, "");
    const auto printUidInfo = [&dw](const uint32_t& key, const uint8_t& value,
                                    const BpfMap<uint32_t, uint8_t>&) {
        dw.println("%u %u", key, value);
        return netdutils::status::ok;
    };
    res = mUidCounterSetMap.iterateWithValue(printUidInfo);
    if (!isOk(res)) {
        dw.println("mUidCounterSetMap print end with error: %s", res.msg().c_str());
    }

    // Print AppUidStatsMap content
    std::string appUidStatsHeader = StringPrintf("uid rxBytes rxPackets txBytes txPackets");
    dumpBpfMap("mAppUidStatsMap:", dw, appUidStatsHeader);
    auto printAppUidStatsInfo = [&dw](const uint32_t& key, const StatsValue& value,
                                      const BpfMap<uint32_t, StatsValue>&) {
        dw.println("%u %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64, key, value.rxBytes,
                   value.rxPackets, value.txBytes, value.txPackets);
        return netdutils::status::ok;
    };
    res = mAppUidStatsMap.iterateWithValue(printAppUidStatsInfo);
    if (!res.ok()) {
        dw.println("mAppUidStatsMap print end with error: %s", res.msg().c_str());
    }

    // Print uidStatsMap content
    std::string statsHeader = StringPrintf("ifaceIndex ifaceName tag_hex uid_int cnt_set rxBytes"
                                           " rxPackets txBytes txPackets");
    dumpBpfMap("mUidStatsMap", dw, statsHeader);
    const auto printStatsInfo = [&dw, this](const StatsKey& key, const StatsValue& value,
                                            const BpfMap<StatsKey, StatsValue>&) {
        uint32_t ifIndex = key.ifaceIndex;
        auto ifname = mIfaceIndexNameMap.readValue(ifIndex);
        if (!isOk(ifname)) {
            strlcpy(ifname.value().name, "unknown", sizeof(IfaceValue));
        }
        dw.println("%u %s 0x%x %u %u %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64, ifIndex,
                   ifname.value().name, key.tag, key.uid, key.counterSet, value.rxBytes,
                   value.rxPackets, value.txBytes, value.txPackets);
        return netdutils::status::ok;
    };
    res = mUidStatsMap.iterateWithValue(printStatsInfo);
    if (!isOk(res)) {
        dw.println("mUidStatsMap print end with error: %s", res.msg().c_str());
    }

    // Print TagStatsMap content.
    dumpBpfMap("mTagStatsMap", dw, statsHeader);
    res = mTagStatsMap.iterateWithValue(printStatsInfo);
    if (!isOk(res)) {
        dw.println("mTagStatsMap print end with error: %s", res.msg().c_str());
    }

    // Print ifaceIndexToNameMap content.
    dumpBpfMap("mIfaceIndexNameMap", dw, "");
    const auto printIfaceNameInfo = [&dw](const uint32_t& key, const IfaceValue& value,
                                          const BpfMap<uint32_t, IfaceValue>&) {
        const char* ifname = value.name;
        dw.println("ifaceIndex=%u ifaceName=%s", key, ifname);
        return netdutils::status::ok;
    };
    res = mIfaceIndexNameMap.iterateWithValue(printIfaceNameInfo);
    if (!isOk(res)) {
        dw.println("mIfaceIndexNameMap print end with error: %s", res.msg().c_str());
    }

    // Print ifaceStatsMap content
    std::string ifaceStatsHeader = StringPrintf("ifaceIndex ifaceName rxBytes rxPackets txBytes"
                                                " txPackets");
    dumpBpfMap("mIfaceStatsMap:", dw, ifaceStatsHeader);
    const auto printIfaceStatsInfo = [&dw, this](const uint32_t& key, const StatsValue& value,
                                                 const BpfMap<uint32_t, StatsValue>&) {
        auto ifname = mIfaceIndexNameMap.readValue(key);
        if (!isOk(ifname)) {
            strlcpy(ifname.value().name, "unknown", sizeof(IfaceValue));
        }
        dw.println("%u %s %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64, key, ifname.value().name,
                   value.rxBytes, value.rxPackets, value.txBytes, value.txPackets);
        return netdutils::status::ok;
    };
    res = mIfaceStatsMap.iterateWithValue(printIfaceStatsInfo);
    if (!isOk(res)) {
        dw.println("mIfaceStatsMap print end with error: %s", res.msg().c_str());
    }

    // Print owner match uid maps
    dumpBpfMap("mDozableUidMap", dw, "");
    res = mDozableUidMap.iterateWithValue(printUidInfo);
    if (!isOk(res)) {
        dw.println("mDozableUidMap print end with error: %s", res.msg().c_str());
    }

    dumpBpfMap("mStandbyUidMap", dw, "");
    res = mStandbyUidMap.iterateWithValue(printUidInfo);
    if (!isOk(res)) {
        dw.println("mDozableUidMap print end with error: %s", res.msg().c_str());
    }

    dumpBpfMap("mPowerSaveUidMap", dw, "");
    res = mPowerSaveUidMap.iterateWithValue(printUidInfo);
    if (!isOk(res)) {
        dw.println("mDozableUidMap print end with error: %s", res.msg().c_str());
    }

    dw.decIndent();

    dw.decIndent();

    dw.decIndent();

}

}  // namespace net
}  // namespace android
