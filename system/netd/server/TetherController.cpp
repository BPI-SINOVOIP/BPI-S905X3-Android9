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

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <array>
#include <cstdlib>
#include <string>
#include <vector>

#define LOG_TAG "TetherController"
#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <netdutils/StatusOr.h>

#include "Fwmark.h"
#include "NetdConstants.h"
#include "Permission.h"
#include "InterfaceController.h"
#include "NetworkController.h"
#include "ResponseCode.h"
#include "TetherController.h"

using android::base::Join;
using android::base::StringPrintf;
using android::base::StringAppendF;
using android::netdutils::StatusOr;
using android::netdutils::statusFromErrno;

namespace {

const char BP_TOOLS_MODE[] = "bp-tools";
const char IPV4_FORWARDING_PROC_FILE[] = "/proc/sys/net/ipv4/ip_forward";
const char IPV6_FORWARDING_PROC_FILE[] = "/proc/sys/net/ipv6/conf/all/forwarding";
const char SEPARATOR[] = "|";
constexpr const char kTcpBeLiberal[] = "/proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal";

// Chosen to match AID_DNS_TETHER, as made "friendly" by fs_config_generator.py.
constexpr const char kDnsmasqUsername[] = "dns_tether";

bool writeToFile(const char* filename, const char* value) {
    int fd = open(filename, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        ALOGE("Failed to open %s: %s", filename, strerror(errno));
        return false;
    }

    const ssize_t len = strlen(value);
    if (write(fd, value, len) != len) {
        ALOGE("Failed to write %s to %s: %s", value, filename, strerror(errno));
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

// TODO: Consider altering TCP and UDP timeouts as well.
void configureForTethering(bool enabled) {
    writeToFile(kTcpBeLiberal, enabled ? "1" : "0");
}

bool configureForIPv6Router(const char *interface) {
    return (InterfaceController::setEnableIPv6(interface, 0) == 0)
            && (InterfaceController::setAcceptIPv6Ra(interface, 0) == 0)
            && (InterfaceController::setAcceptIPv6Dad(interface, 0) == 0)
            && (InterfaceController::setIPv6DadTransmits(interface, "0") == 0)
            && (InterfaceController::setEnableIPv6(interface, 1) == 0);
}

void configureForIPv6Client(const char *interface) {
    InterfaceController::setAcceptIPv6Ra(interface, 1);
    InterfaceController::setAcceptIPv6Dad(interface, 1);
    InterfaceController::setIPv6DadTransmits(interface, "1");
    InterfaceController::setEnableIPv6(interface, 0);
}

bool inBpToolsMode() {
    // In BP tools mode, do not disable IP forwarding
    char bootmode[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.bootmode", bootmode, "unknown");
    return !strcmp(BP_TOOLS_MODE, bootmode);
}

}  // namespace

namespace android {
namespace net {

auto TetherController::iptablesRestoreFunction = execIptablesRestoreWithOutput;

const int MAX_IPT_OUTPUT_LINE_LEN = 256;

const std::string GET_TETHER_STATS_COMMAND = StringPrintf(
    "*filter\n"
    "-nvx -L %s\n"
    "COMMIT\n", android::net::TetherController::LOCAL_TETHER_COUNTERS_CHAIN);

TetherController::TetherController() {
    mDnsNetId = 0;
    mDaemonFd = -1;
    mDaemonPid = 0;
    if (inBpToolsMode()) {
        enableForwarding(BP_TOOLS_MODE);
    } else {
        setIpFwdEnabled();
    }
}

TetherController::~TetherController() {
    mInterfaces.clear();
    mDnsForwarders.clear();
    mForwardingRequests.clear();
    mFwdIfaces.clear();
}

bool TetherController::setIpFwdEnabled() {
    bool success = true;
    const char* value = mForwardingRequests.empty() ? "0" : "1";
    ALOGD("Setting IP forward enable = %s", value);
    success &= writeToFile(IPV4_FORWARDING_PROC_FILE, value);
    success &= writeToFile(IPV6_FORWARDING_PROC_FILE, value);
    return success;
}

bool TetherController::enableForwarding(const char* requester) {
    // Don't return an error if this requester already requested forwarding. Only return errors for
    // things that the caller caller needs to care about, such as "couldn't write to the file to
    // enable forwarding".
    mForwardingRequests.insert(requester);
    return setIpFwdEnabled();
}

bool TetherController::disableForwarding(const char* requester) {
    mForwardingRequests.erase(requester);
    return setIpFwdEnabled();
}

size_t TetherController::forwardingRequestCount() {
    return mForwardingRequests.size();
}

int TetherController::startTethering(int num_addrs, char **dhcp_ranges) {
    if (mDaemonPid != 0) {
        ALOGE("Tethering already started");
        errno = EBUSY;
        return -1;
    }

    ALOGD("Starting tethering services");

    pid_t pid;
    int pipefd[2];

    if (pipe(pipefd) < 0) {
        ALOGE("pipe failed (%s)", strerror(errno));
        return -1;
    }

    /*
     * TODO: Create a monitoring thread to handle and restart
     * the daemon if it exits prematurely
     */
    if ((pid = fork()) < 0) {
        ALOGE("fork failed (%s)", strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (!pid) {
        close(pipefd[1]);
        if (pipefd[0] != STDIN_FILENO) {
            if (dup2(pipefd[0], STDIN_FILENO) != STDIN_FILENO) {
                ALOGE("dup2 failed (%s)", strerror(errno));
                return -1;
            }
            close(pipefd[0]);
        }

        Fwmark fwmark;
        fwmark.netId = NetworkController::LOCAL_NET_ID;
        fwmark.explicitlySelected = true;
        fwmark.protectedFromVpn = true;
        fwmark.permission = PERMISSION_SYSTEM;
        char markStr[UINT32_HEX_STRLEN];
        snprintf(markStr, sizeof(markStr), "0x%x", fwmark.intValue);

        std::vector<const std::string> argVector = {
            "/system/bin/dnsmasq",
            "--keep-in-foreground",
            "--no-resolv",
            "--no-poll",
            "--dhcp-authoritative",
            // TODO: pipe through metered status from ConnService
            "--dhcp-option-force=43,ANDROID_METERED",
            "--pid-file",
            "--listen-mark", markStr,
            "--user", kDnsmasqUsername,
        };

        for (int addrIndex = 0; addrIndex < num_addrs; addrIndex += 2) {
            argVector.push_back(
                    StringPrintf("--dhcp-range=%s,%s,1h",
                                 dhcp_ranges[addrIndex], dhcp_ranges[addrIndex+1]));
        }

        auto args = (char**)std::calloc(argVector.size() + 1, sizeof(char*));
        for (unsigned i = 0; i < argVector.size(); i++) {
            args[i] = (char*)argVector[i].c_str();
        }

        if (execv(args[0], args)) {
            ALOGE("execv failed (%s)", strerror(errno));
        }
        ALOGE("Should never get here!");
        _exit(-1);
    } else {
        close(pipefd[0]);
        mDaemonPid = pid;
        mDaemonFd = pipefd[1];
        configureForTethering(true);
        applyDnsInterfaces();
        ALOGD("Tethering services running");
    }

    return 0;
}

int TetherController::stopTethering() {
    configureForTethering(false);

    if (mDaemonPid == 0) {
        ALOGE("Tethering already stopped");
        return 0;
    }

    ALOGD("Stopping tethering services");

    kill(mDaemonPid, SIGTERM);
    waitpid(mDaemonPid, NULL, 0);
    mDaemonPid = 0;
    close(mDaemonFd);
    mDaemonFd = -1;
    ALOGD("Tethering services stopped");
    return 0;
}

bool TetherController::isTetheringStarted() {
    return (mDaemonPid == 0 ? false : true);
}

#define MAX_CMD_SIZE 1024

int TetherController::setDnsForwarders(unsigned netId, char **servers, int numServers) {
    int i;
    char daemonCmd[MAX_CMD_SIZE];

    Fwmark fwmark;
    fwmark.netId = netId;
    fwmark.explicitlySelected = true;
    fwmark.protectedFromVpn = true;
    fwmark.permission = PERMISSION_SYSTEM;

    snprintf(daemonCmd, sizeof(daemonCmd), "update_dns%s0x%x", SEPARATOR, fwmark.intValue);
    int cmdLen = strlen(daemonCmd);

    mDnsForwarders.clear();
    for (i = 0; i < numServers; i++) {
        ALOGD("setDnsForwarders(0x%x %d = '%s')", fwmark.intValue, i, servers[i]);

        addrinfo *res, hints = { .ai_flags = AI_NUMERICHOST };
        int ret = getaddrinfo(servers[i], NULL, &hints, &res);
        freeaddrinfo(res);
        if (ret) {
            ALOGE("Failed to parse DNS server '%s'", servers[i]);
            mDnsForwarders.clear();
            errno = EINVAL;
            return -1;
        }

        cmdLen += (strlen(servers[i]) + 1);
        if (cmdLen + 1 >= MAX_CMD_SIZE) {
            ALOGD("Too many DNS servers listed");
            break;
        }

        strcat(daemonCmd, SEPARATOR);
        strcat(daemonCmd, servers[i]);
        mDnsForwarders.push_back(servers[i]);
    }

    mDnsNetId = netId;
    if (mDaemonFd != -1) {
        ALOGD("Sending update msg to dnsmasq [%s]", daemonCmd);
        if (write(mDaemonFd, daemonCmd, strlen(daemonCmd) +1) < 0) {
            ALOGE("Failed to send update command to dnsmasq (%s)", strerror(errno));
            mDnsForwarders.clear();
            errno = EREMOTEIO;
            return -1;
        }
    }
    return 0;
}

unsigned TetherController::getDnsNetId() {
    return mDnsNetId;
}

const std::list<std::string> &TetherController::getDnsForwarders() const {
    return mDnsForwarders;
}

bool TetherController::applyDnsInterfaces() {
    char daemonCmd[MAX_CMD_SIZE];

    strcpy(daemonCmd, "update_ifaces");
    int cmdLen = strlen(daemonCmd);
    bool haveInterfaces = false;

    for (const auto &ifname : mInterfaces) {
        cmdLen += (ifname.size() + 1);
        if (cmdLen + 1 >= MAX_CMD_SIZE) {
            ALOGD("Too many DNS ifaces listed");
            break;
        }

        strcat(daemonCmd, SEPARATOR);
        strcat(daemonCmd, ifname.c_str());
        haveInterfaces = true;
    }

    if ((mDaemonFd != -1) && haveInterfaces) {
        ALOGD("Sending update msg to dnsmasq [%s]", daemonCmd);
        if (write(mDaemonFd, daemonCmd, strlen(daemonCmd) +1) < 0) {
            ALOGE("Failed to send update command to dnsmasq (%s)", strerror(errno));
            return false;
        }
    }
    return true;
}

int TetherController::tetherInterface(const char *interface) {
    ALOGD("tetherInterface(%s)", interface);
    if (!isIfaceName(interface)) {
        errno = ENOENT;
        return -1;
    }

    if (!configureForIPv6Router(interface)) {
        configureForIPv6Client(interface);
        return -1;
    }
    mInterfaces.push_back(interface);

    if (!applyDnsInterfaces()) {
        mInterfaces.pop_back();
        configureForIPv6Client(interface);
        return -1;
    } else {
        return 0;
    }
}

int TetherController::untetherInterface(const char *interface) {
    ALOGD("untetherInterface(%s)", interface);

    for (auto it = mInterfaces.cbegin(); it != mInterfaces.cend(); ++it) {
        if (!strcmp(interface, it->c_str())) {
            mInterfaces.erase(it);

            configureForIPv6Client(interface);
            return applyDnsInterfaces() ? 0 : -1;
        }
    }
    errno = ENOENT;
    return -1;
}

const std::list<std::string> &TetherController::getTetheredInterfaceList() const {
    return mInterfaces;
}

int TetherController::setupIptablesHooks() {
    int res;
    res = setDefaults();
    if (res < 0) {
        return res;
    }

    // Used to limit downstream mss to the upstream pmtu so we don't end up fragmenting every large
    // packet tethered devices send. This is IPv4-only, because in IPv6 we send the MTU in the RA.
    // This is no longer optional and tethering will fail to start if it fails.
    std::string mssRewriteCommand = StringPrintf(
        "*mangle\n"
        "-A %s -p tcp --tcp-flags SYN SYN -j TCPMSS --clamp-mss-to-pmtu\n"
        "COMMIT\n", LOCAL_MANGLE_FORWARD);

    // This is for tethering counters. This chain is reached via --goto, and then RETURNS.
    std::string defaultCommands = StringPrintf(
        "*filter\n"
        ":%s -\n"
        "COMMIT\n", LOCAL_TETHER_COUNTERS_CHAIN);

    res = iptablesRestoreFunction(V4, mssRewriteCommand, nullptr);
    if (res < 0) {
        return res;
    }

    res = iptablesRestoreFunction(V4V6, defaultCommands, nullptr);
    if (res < 0) {
        return res;
    }

    mFwdIfaces.clear();

    return 0;
}

int TetherController::setDefaults() {
    std::string v4Cmd = StringPrintf(
        "*filter\n"
        ":%s -\n"
        "-A %s -j DROP\n"
        "COMMIT\n"
        "*nat\n"
        ":%s -\n"
        "COMMIT\n", LOCAL_FORWARD, LOCAL_FORWARD, LOCAL_NAT_POSTROUTING);

    std::string v6Cmd = StringPrintf(
        "*filter\n"
        ":%s -\n"
        "COMMIT\n"
        "*raw\n"
        ":%s -\n"
        "COMMIT\n", LOCAL_FORWARD, LOCAL_RAW_PREROUTING);

    int res = iptablesRestoreFunction(V4, v4Cmd, nullptr);
    if (res < 0) {
        return res;
    }

    res = iptablesRestoreFunction(V6, v6Cmd, nullptr);
    if (res < 0) {
        return res;
    }

    return 0;
}

int TetherController::enableNat(const char* intIface, const char* extIface) {
    ALOGV("enableNat(intIface=<%s>, extIface=<%s>)",intIface, extIface);

    if (!isIfaceName(intIface) || !isIfaceName(extIface)) {
        errno = ENODEV;
        return -1;
    }

    /* Bug: b/9565268. "enableNat wlan0 wlan0". For now we fail until java-land is fixed */
    if (!strcmp(intIface, extIface)) {
        ALOGE("Duplicate interface specified: %s %s", intIface, extIface);
        errno = EINVAL;
        return -1;
    }

    if (isForwardingPairEnabled(intIface, extIface)) {
        return 0;
    }

    // add this if we are the first enabled nat for this upstream
    if (!isAnyForwardingEnabledOnUpstream(extIface)) {
        std::vector<std::string> v4Cmds = {
            "*nat",
            StringPrintf("-A %s -o %s -j MASQUERADE", LOCAL_NAT_POSTROUTING, extIface),
            "COMMIT\n"
        };

        if (iptablesRestoreFunction(V4, Join(v4Cmds, '\n'), nullptr) ||
            setupIPv6CountersChain()) {
            ALOGE("Error setting postroute rule: iface=%s", extIface);
            if (!isAnyForwardingPairEnabled()) {
                // unwind what's been done, but don't care about success - what more could we do?
                setDefaults();
            }
            return -1;
        }
    }

    if (setForwardRules(true, intIface, extIface) != 0) {
        ALOGE("Error setting forward rules");
        if (!isAnyForwardingPairEnabled()) {
            setDefaults();
        }
        errno = ENODEV;
        return -1;
    }

    return 0;
}

int TetherController::setupIPv6CountersChain() {
    // Only add this if we are the first enabled nat
    if (isAnyForwardingPairEnabled()) {
        return 0;
    }

    /*
     * IPv6 tethering doesn't need the state-based conntrack rules, so
     * it unconditionally jumps to the tether counters chain all the time.
     */
    std::vector<std::string> v6Cmds = {
        "*filter",
        StringPrintf("-A %s -g %s", LOCAL_FORWARD, LOCAL_TETHER_COUNTERS_CHAIN),
        "COMMIT\n"
    };

    return iptablesRestoreFunction(V6, Join(v6Cmds, '\n'), nullptr);
}

// Gets a pointer to the ForwardingDownstream for an interface pair in the map, or nullptr
TetherController::ForwardingDownstream* TetherController::findForwardingDownstream(
        const std::string& intIface, const std::string& extIface) {
    auto extIfaceMatches = mFwdIfaces.equal_range(extIface);
    for (auto it = extIfaceMatches.first; it != extIfaceMatches.second; ++it) {
        if (it->second.iface == intIface) {
            return &(it->second);
        }
    }
    return nullptr;
}

void TetherController::addForwardingPair(const std::string& intIface, const std::string& extIface) {
    ForwardingDownstream* existingEntry = findForwardingDownstream(intIface, extIface);
    if (existingEntry != nullptr) {
        existingEntry->active = true;
        return;
    }

    mFwdIfaces.insert(std::pair<std::string, ForwardingDownstream>(extIface, {
        .iface = intIface,
        .active = true
    }));
}

void TetherController::markForwardingPairDisabled(
        const std::string& intIface, const std::string& extIface) {
    ForwardingDownstream* existingEntry = findForwardingDownstream(intIface, extIface);
    if (existingEntry == nullptr) {
        return;
    }

    existingEntry->active = false;
}

bool TetherController::isForwardingPairEnabled(
        const std::string& intIface, const std::string& extIface) {
    ForwardingDownstream* existingEntry = findForwardingDownstream(intIface, extIface);
    return existingEntry != nullptr && existingEntry->active;
}

bool TetherController::isAnyForwardingEnabledOnUpstream(const std::string& extIface) {
    auto extIfaceMatches = mFwdIfaces.equal_range(extIface);
    for (auto it = extIfaceMatches.first; it != extIfaceMatches.second; ++it) {
        if (it->second.active) {
            return true;
        }
    }
    return false;
}

bool TetherController::isAnyForwardingPairEnabled() {
    for (auto& it : mFwdIfaces) {
        if (it.second.active) {
            return true;
        }
    }
    return false;
}

bool TetherController::tetherCountingRuleExists(
        const std::string& iface1, const std::string& iface2) {
    // A counting rule exists if NAT was ever enabled for this interface pair, so if the pair
    // is in the map regardless of its active status. Rules are added both ways so we check with
    // the 2 combinations.
    return findForwardingDownstream(iface1, iface2) != nullptr
        || findForwardingDownstream(iface2, iface1) != nullptr;
}

/* static */
std::string TetherController::makeTetherCountingRule(const char *if1, const char *if2) {
    return StringPrintf("-A %s -i %s -o %s -j RETURN", LOCAL_TETHER_COUNTERS_CHAIN, if1, if2);
}

int TetherController::setForwardRules(bool add, const char *intIface, const char *extIface) {
    const char *op = add ? "-A" : "-D";

    std::string rpfilterCmd = StringPrintf(
        "*raw\n"
        "%s %s -i %s -m rpfilter --invert ! -s fe80::/64 -j DROP\n"
        "COMMIT\n", op, LOCAL_RAW_PREROUTING, intIface);
    if (iptablesRestoreFunction(V6, rpfilterCmd, nullptr) == -1 && add) {
        return -1;
    }

    std::vector<std::string> v4 = {
        "*filter",
        StringPrintf("%s %s -i %s -o %s -m state --state ESTABLISHED,RELATED -g %s",
                     op, LOCAL_FORWARD, extIface, intIface, LOCAL_TETHER_COUNTERS_CHAIN),
        StringPrintf("%s %s -i %s -o %s -m state --state INVALID -j DROP",
                     op, LOCAL_FORWARD, intIface, extIface),
        StringPrintf("%s %s -i %s -o %s -g %s",
                     op, LOCAL_FORWARD, intIface, extIface, LOCAL_TETHER_COUNTERS_CHAIN),
    };

    std::vector<std::string> v6 = {
        "*filter",
    };

    // We only ever add tethering quota rules so that they stick.
    if (add && !tetherCountingRuleExists(intIface, extIface)) {
        v4.push_back(makeTetherCountingRule(intIface, extIface));
        v4.push_back(makeTetherCountingRule(extIface, intIface));
        v6.push_back(makeTetherCountingRule(intIface, extIface));
        v6.push_back(makeTetherCountingRule(extIface, intIface));
    }

    // Always make sure the drop rule is at the end.
    // TODO: instead of doing this, consider just rebuilding LOCAL_FORWARD completely from scratch
    // every time, starting with ":tetherctrl_FORWARD -\n". This would likely be a bit simpler.
    if (add) {
        v4.push_back(StringPrintf("-D %s -j DROP", LOCAL_FORWARD));
        v4.push_back(StringPrintf("-A %s -j DROP", LOCAL_FORWARD));
    }

    v4.push_back("COMMIT\n");
    v6.push_back("COMMIT\n");

    // We only add IPv6 rules here, never remove them.
    if (iptablesRestoreFunction(V4, Join(v4, '\n'), nullptr) == -1 ||
        (add && iptablesRestoreFunction(V6, Join(v6, '\n'), nullptr) == -1)) {
        // unwind what's been done, but don't care about success - what more could we do?
        if (add) {
            setForwardRules(false, intIface, extIface);
        }
        return -1;
    }

    if (add) {
        addForwardingPair(intIface, extIface);
    } else {
        markForwardingPairDisabled(intIface, extIface);
    }

    return 0;
}

int TetherController::disableNat(const char* intIface, const char* extIface) {
    if (!isIfaceName(intIface) || !isIfaceName(extIface)) {
        errno = ENODEV;
        return -1;
    }

    setForwardRules(false, intIface, extIface);
    if (!isAnyForwardingPairEnabled()) {
        setDefaults();
    }
    return 0;
}

void TetherController::addStats(TetherStatsList& statsList, const TetherStats& stats) {
    for (TetherStats& existing : statsList) {
        if (existing.addStatsIfMatch(stats)) {
            return;
        }
    }
    // No match. Insert a new interface pair.
    statsList.push_back(stats);
}

/*
 * Parse the ptks and bytes out of:
 *   Chain tetherctrl_counters (4 references)
 *       pkts      bytes target     prot opt in     out     source               destination
 *         26     2373 RETURN     all  --  wlan0  rmnet0  0.0.0.0/0            0.0.0.0/0
 *         27     2002 RETURN     all  --  rmnet0 wlan0   0.0.0.0/0            0.0.0.0/0
 *       1040   107471 RETURN     all  --  bt-pan rmnet0  0.0.0.0/0            0.0.0.0/0
 *       1450  1708806 RETURN     all  --  rmnet0 bt-pan  0.0.0.0/0            0.0.0.0/0
 * or:
 *   Chain tetherctrl_counters (0 references)
 *       pkts      bytes target     prot opt in     out     source               destination
 *          0        0 RETURN     all      wlan0  rmnet_data0  ::/0                 ::/0
 *          0        0 RETURN     all      rmnet_data0 wlan0   ::/0                 ::/0
 *
 */
int TetherController::addForwardChainStats(TetherStatsList& statsList,
                                           const std::string& statsOutput,
                                           std::string &extraProcessingInfo) {
    int res;
    std::string statsLine;
    char iface0[MAX_IPT_OUTPUT_LINE_LEN];
    char iface1[MAX_IPT_OUTPUT_LINE_LEN];
    char rest[MAX_IPT_OUTPUT_LINE_LEN];

    TetherStats stats;
    const TetherStats empty;
    const char *buffPtr;
    int64_t packets, bytes;

    std::stringstream stream(statsOutput);

    // Skip headers.
    for (int i = 0; i < 2; i++) {
        std::getline(stream, statsLine, '\n');
        extraProcessingInfo += statsLine + "\n";
        if (statsLine.empty()) {
            ALOGE("Empty header while parsing tethering stats");
            return -EREMOTEIO;
        }
    }

    while (std::getline(stream, statsLine, '\n')) {
        buffPtr = statsLine.c_str();

        /* Clean up, so a failed parse can still print info */
        iface0[0] = iface1[0] = rest[0] = packets = bytes = 0;
        if (strstr(buffPtr, "0.0.0.0")) {
            // IPv4 has -- indicating what to do with fragments...
            //       26     2373 RETURN     all  --  wlan0  rmnet0  0.0.0.0/0            0.0.0.0/0
            res = sscanf(buffPtr, "%" SCNd64" %" SCNd64" RETURN all -- %s %s 0.%s",
                    &packets, &bytes, iface0, iface1, rest);
        } else {
            // ... but IPv6 does not.
            //       26     2373 RETURN     all      wlan0  rmnet0  ::/0                 ::/0
            res = sscanf(buffPtr, "%" SCNd64" %" SCNd64" RETURN all %s %s ::/%s",
                    &packets, &bytes, iface0, iface1, rest);
        }
        ALOGV("parse res=%d iface0=<%s> iface1=<%s> pkts=%" PRId64" bytes=%" PRId64" rest=<%s> orig line=<%s>", res,
             iface0, iface1, packets, bytes, rest, buffPtr);
        extraProcessingInfo += buffPtr;
        extraProcessingInfo += "\n";

        if (res != 5) {
            return -EREMOTEIO;
        }
        /*
         * The following assumes that the 1st rule has in:extIface out:intIface,
         * which is what TetherController sets up.
         * The 1st matches rx, and sets up the pair for the tx side.
         */
        if (!stats.intIface[0]) {
            ALOGV("0Filter RX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
            stats.intIface = iface0;
            stats.extIface = iface1;
            stats.txPackets = packets;
            stats.txBytes = bytes;
        } else if (stats.intIface == iface1 && stats.extIface == iface0) {
            ALOGV("0Filter TX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
            stats.rxPackets = packets;
            stats.rxBytes = bytes;
        }
        if (stats.rxBytes != -1 && stats.txBytes != -1) {
            ALOGV("rx_bytes=%" PRId64" tx_bytes=%" PRId64, stats.rxBytes, stats.txBytes);
            addStats(statsList, stats);
            stats = empty;
        }
    }

    /* It is always an error to find only one side of the stats. */
    if (((stats.rxBytes == -1) != (stats.txBytes == -1))) {
        return -EREMOTEIO;
    }
    return 0;
}

StatusOr<TetherController::TetherStatsList> TetherController::getTetherStats() {
    TetherStatsList statsList;
    std::string parsedIptablesOutput;

    for (const IptablesTarget target : {V4, V6}) {
        std::string statsString;
        if (int ret = iptablesRestoreFunction(target, GET_TETHER_STATS_COMMAND, &statsString)) {
            return statusFromErrno(-ret, StringPrintf("failed to fetch tether stats (%d): %d",
                                                      target, ret));
        }

        if (int ret = addForwardChainStats(statsList, statsString, parsedIptablesOutput)) {
            return statusFromErrno(-ret, StringPrintf("failed to parse %s tether stats:\n%s",
                                                      target == V4 ? "IPv4": "IPv6",
                                                      parsedIptablesOutput.c_str()));
        }
    }

    return statsList;
}

}  // namespace net
}  // namespace android
