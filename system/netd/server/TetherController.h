/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _TETHER_CONTROLLER_H
#define _TETHER_CONTROLLER_H

#include <list>
#include <set>
#include <string>

#include <netdutils/StatusOr.h>
#include <sysutils/SocketClient.h>

#include "NetdConstants.h"

namespace android {
namespace net {

using android::netdutils::StatusOr;

class TetherController {
private:
    struct ForwardingDownstream {
        std::string iface;
        bool active;
    };

    std::list<std::string> mInterfaces;

    // Map upstream iface -> downstream iface. A pair is in the map if forwarding was enabled at
    // some point since the controller was initialized.
    std::multimap<std::string, ForwardingDownstream> mFwdIfaces;

    // NetId to use for forwarded DNS queries. This may not be the default
    // network, e.g., in the case where we are tethering to a DUN APN.
    unsigned               mDnsNetId;
    std::list<std::string> mDnsForwarders;
    pid_t                  mDaemonPid;
    int                    mDaemonFd;
    std::set<std::string>  mForwardingRequests;

public:

    TetherController();
    virtual ~TetherController();

    bool enableForwarding(const char* requester);
    bool disableForwarding(const char* requester);
    size_t forwardingRequestCount();

    int startTethering(int num_addrs, char **dhcp_ranges);
    int stopTethering();
    bool isTetheringStarted();

    unsigned getDnsNetId();
    int setDnsForwarders(unsigned netId, char **servers, int numServers);
    const std::list<std::string> &getDnsForwarders() const;

    int tetherInterface(const char *interface);
    int untetherInterface(const char *interface);
    const std::list<std::string> &getTetheredInterfaceList() const;
    bool applyDnsInterfaces();

    int enableNat(const char* intIface, const char* extIface);
    int disableNat(const char* intIface, const char* extIface);
    int setupIptablesHooks();

    class TetherStats {
    public:
        TetherStats() = default;
        TetherStats(std::string intIfn, std::string extIfn,
                int64_t rxB, int64_t rxP,
                int64_t txB, int64_t txP)
                        : intIface(intIfn), extIface(extIfn),
                            rxBytes(rxB), rxPackets(rxP),
                            txBytes(txB), txPackets(txP) {};
        std::string intIface;
        std::string extIface;
        int64_t rxBytes = -1;
        int64_t rxPackets = -1;
        int64_t txBytes = -1;
        int64_t txPackets = -1;

        bool addStatsIfMatch(const TetherStats& other) {
            if (intIface == other.intIface && extIface == other.extIface) {
                rxBytes   += other.rxBytes;
                rxPackets += other.rxPackets;
                txBytes   += other.txBytes;
                txPackets += other.txPackets;
                return true;
            }
            return false;
        }
    };

    typedef std::vector<TetherStats> TetherStatsList;

    StatusOr<TetherStatsList> getTetherStats();

    /*
     * extraProcessingInfo: contains raw parsed data, and error info.
     * This strongly requires that setup of the rules is in a specific order:
     *  in:intIface out:extIface
     *  in:extIface out:intIface
     * and the rules are grouped in pairs when more that one tethering was setup.
     */
    static int addForwardChainStats(TetherStatsList& statsList, const std::string& iptOutput,
                                    std::string &extraProcessingInfo);

    static constexpr const char* LOCAL_FORWARD               = "tetherctrl_FORWARD";
    static constexpr const char* LOCAL_MANGLE_FORWARD        = "tetherctrl_mangle_FORWARD";
    static constexpr const char* LOCAL_NAT_POSTROUTING       = "tetherctrl_nat_POSTROUTING";
    static constexpr const char* LOCAL_RAW_PREROUTING        = "tetherctrl_raw_PREROUTING";
    static constexpr const char* LOCAL_TETHER_COUNTERS_CHAIN = "tetherctrl_counters";

    android::RWLock lock;

private:
    bool setIpFwdEnabled();

    int setupIPv6CountersChain();
    static std::string makeTetherCountingRule(const char *if1, const char *if2);
    ForwardingDownstream* findForwardingDownstream(const std::string& intIface,
        const std::string& extIface);
    void addForwardingPair(const std::string& intIface, const std::string& extIface);
    void markForwardingPairDisabled(const std::string& intIface, const std::string& extIface);

    bool isForwardingPairEnabled(const std::string& intIface, const std::string& extIface);
    bool isAnyForwardingEnabledOnUpstream(const std::string& extIface);
    bool isAnyForwardingPairEnabled();
    bool tetherCountingRuleExists(const std::string& iface1, const std::string& iface2);

    int setDefaults();
    int setForwardRules(bool set, const char *intIface, const char *extIface);
    int setTetherCountingRules(bool add, const char *intIface, const char *extIface);

    static void addStats(TetherStatsList& statsList, const TetherStats& stats);

    // For testing.
    friend class TetherControllerTest;
    static int (*iptablesRestoreFunction)(IptablesTarget, const std::string&, std::string *);
};

}  // namespace net
}  // namespace android

#endif
