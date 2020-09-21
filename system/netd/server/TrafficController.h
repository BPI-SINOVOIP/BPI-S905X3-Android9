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

#ifndef NETD_SERVER_TRAFFIC_CONTROLLER_H
#define NETD_SERVER_TRAFFIC_CONTROLLER_H

#include <linux/bpf.h>

#include <netdutils/StatusOr.h>
#include "FirewallController.h"
#include "NetlinkListener.h"
#include "Network.h"
#include "android-base/thread_annotations.h"
#include "android-base/unique_fd.h"
#include "bpf/BpfMap.h"

using android::bpf::BpfMap;
using android::bpf::IfaceValue;
using android::bpf::StatsKey;
using android::bpf::StatsValue;
using android::bpf::UidTag;

namespace android {
namespace net {

class DumpWriter;

class TrafficController {
  public:
    TrafficController();
    /*
     * Initialize the whole controller
     */
    netdutils::Status start();
    /*
     * Tag the socket with the specified tag and uid. In the qtaguid module, the
     * first tag request that grab the spinlock of rb_tree can update the tag
     * information first and other request need to wait until it finish. All the
     * tag request will be addressed in the order of they obtaining the spinlock.
     * In the eBPF implementation, the kernel will try to update the eBPF map
     * entry with the tag request. And the hashmap update process is protected by
     * the spinlock initialized with the map. So the behavior of two modules
     * should be the same. No additional lock needed.
     */
    int tagSocket(int sockFd, uint32_t tag, uid_t uid);

    /*
     * The untag process is similiar to tag socket and both old qtaguid module and
     * new eBPF module have spinlock inside the kernel for concurrent update. No
     * external lock is required.
     */
    int untagSocket(int sockFd);

    /*
     * Similiar as above, no external lock required.
     */
    int setCounterSet(int counterSetNum, uid_t uid);

    /*
     * When deleting a tag data, the qtaguid module will grab the spinlock of each
     * related rb_tree one by one and delete the tag information, counterSet
     * information, iface stats information and uid stats information one by one.
     * The new eBPF implementation is done similiarly by removing the entry on
     * each map one by one. And deleting processes are also protected by the
     * spinlock of the map. So no additional lock is required.
     */
    int deleteTagData(uint32_t tag, uid_t uid);

    /*
     * Check if the current device have the bpf traffic stats accounting service
     * running.
     */
    bool checkBpfStatsEnable();

    /*
     * Add the interface name and index pair into the eBPF map.
     */
    int addInterface(const char* name, uint32_t ifaceIndex);

    int changeUidOwnerRule(ChildChain chain, const uid_t uid, FirewallRule rule, FirewallType type);

    int removeUidOwnerRule(const uid_t uid);

    int replaceUidOwnerMap(const std::string& name, bool isWhitelist,
                           const std::vector<int32_t>& uids);

    netdutils::Status updateOwnerMapEntry(BpfMap<uint32_t, uint8_t>& map, uid_t uid,
                                          FirewallRule rule, FirewallType type);

    void dump(DumpWriter& dw, bool verbose);

    netdutils::Status replaceUidsInMap(BpfMap<uint32_t, uint8_t>& map,
                                       const std::vector<int32_t>& uids, FirewallRule rule,
                                       FirewallType type);

    static const String16 DUMP_KEYWORD;

    int toggleUidOwnerMap(ChildChain chain, bool enable);

  private:
    /*
     * mCookieTagMap: Store the corresponding tag and uid for a specific socket.
     * DO NOT hold any locks when modifying this map, otherwise when the untag
     * operation is waiting for a lock hold by other process and there are more
     * sockets being closed than can fit in the socket buffer of the netlink socket
     * that receives them, then the kernel will drop some of these sockets and we
     * won't delete their tags.
     * Map Key: uint64_t socket cookie
     * Map Value: struct UidTag, contains a uint32 uid and a uint32 tag.
     */
    BpfMap<uint64_t, UidTag> mCookieTagMap;

    /*
     * mUidCounterSetMap: Store the counterSet of a specific uid.
     * Map Key: uint32 uid.
     * Map Value: uint32 counterSet specifies if the traffic is a background
     * or foreground traffic.
     */
    BpfMap<uint32_t, uint8_t> mUidCounterSetMap;

    /*
     * mAppUidStatsMap: Store the total traffic stats for a uid regardless of
     * tag, counterSet and iface. The stats is used by TrafficStats.getUidStats
     * API to return persistent stats for a specific uid since device boot.
     */
    BpfMap<uint32_t, StatsValue> mAppUidStatsMap;

    /*
     * mUidStatsMap: Store the traffic statistics for a specific combination of
     * uid, iface and counterSet. We maintain this map in addition to
     * mTagStatsMap because we want to be able to track per-UID data usage even
     * if mTagStatsMap is full.
     * Map Key: Struct StatsKey contains the uid, counterSet and ifaceIndex
     * information. The Tag in the StatsKey should always be 0.
     * Map Value: struct Stats, contains packet count and byte count of each
     * transport protocol on egress and ingress direction.
     */
    BpfMap<StatsKey, StatsValue> mUidStatsMap;

    /*
     * mTagStatsMap: Store the traffic statistics for a specific combination of
     * uid, tag, iface and counterSet. Only tagged socket stats should be stored
     * in this map.
     * Map Key: Struct StatsKey contains the uid, counterSet and ifaceIndex
     * information. The tag field should not be 0.
     * Map Value: struct Stats, contains packet count and byte count of each
     * transport protocol on egress and ingress direction.
     */
    BpfMap<StatsKey, StatsValue> mTagStatsMap;

    /*
     * mIfaceIndexNameMap: Store the index name pair of each interface show up
     * on the device since boot. The interface index is used by the eBPF program
     * to correctly match the iface name when receiving a packet.
     */
    BpfMap<uint32_t, IfaceValue> mIfaceIndexNameMap;

    /*
     * mIfaceStataMap: Store per iface traffic stats gathered from xt_bpf
     * filter.
     */
    BpfMap<uint32_t, StatsValue> mIfaceStatsMap;

    /*
     * mDozableUidMap: Store uids that have related rules in dozable mode owner match
     * chain.
     */
    BpfMap<uint32_t, uint8_t> mDozableUidMap GUARDED_BY(mOwnerMatchMutex);

    /*
     * mStandbyUidMap: Store uids that have related rules in standby mode owner match
     * chain.
     */
    BpfMap<uint32_t, uint8_t> mStandbyUidMap GUARDED_BY(mOwnerMatchMutex);

    /*
     * mPowerSaveUidMap: Store uids that have related rules in power save mode owner match
     * chain.
     */
    BpfMap<uint32_t, uint8_t> mPowerSaveUidMap GUARDED_BY(mOwnerMatchMutex);

    std::unique_ptr<NetlinkListenerInterface> mSkDestroyListener;

    bool ebpfSupported;

    std::mutex mOwnerMatchMutex;

    netdutils::Status loadAndAttachProgram(bpf_attach_type type, const char* path, const char* name,
                                           base::unique_fd& cg_fd);

    netdutils::Status initMaps();
    // For testing
    friend class TrafficControllerTest;
};

}  // namespace net
}  // namespace android

#endif  // NETD_SERVER_TRAFFIC_CONTROLLER_H
