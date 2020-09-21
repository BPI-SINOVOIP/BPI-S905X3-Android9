/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <set>

#include <cstdint>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "FirewallController"
#define LOG_NDEBUG 0

#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <cutils/log.h>

#include "Controllers.h"
#include "FirewallController.h"
#include "NetdConstants.h"
#include "bpf/BpfUtils.h"

using android::base::Join;
using android::base::StringAppendF;
using android::base::StringPrintf;
using android::bpf::DOZABLE_UID_MAP_PATH;
using android::bpf::POWERSAVE_UID_MAP_PATH;
using android::bpf::STANDBY_UID_MAP_PATH;
using android::net::gCtls;

auto FirewallController::execIptablesRestore = ::execIptablesRestore;

const char* FirewallController::TABLE = "filter";

const char* FirewallController::LOCAL_INPUT = "fw_INPUT";
const char* FirewallController::LOCAL_OUTPUT = "fw_OUTPUT";
const char* FirewallController::LOCAL_FORWARD = "fw_FORWARD";

const char* FirewallController::LOCAL_DOZABLE = "fw_dozable";
const char* FirewallController::LOCAL_STANDBY = "fw_standby";
const char* FirewallController::LOCAL_POWERSAVE = "fw_powersave";

// ICMPv6 types that are required for any form of IPv6 connectivity to work. Note that because the
// fw_dozable chain is called from both INPUT and OUTPUT, this includes both packets that we need
// to be able to send (e.g., RS, NS), and packets that we need to receive (e.g., RA, NA).
const char* FirewallController::ICMPV6_TYPES[] = {
    "packet-too-big",
    "router-solicitation",
    "router-advertisement",
    "neighbour-solicitation",
    "neighbour-advertisement",
    "redirect",
};

bool getBpfOwnerStatus() {
    return gCtls->trafficCtrl.checkBpfStatsEnable();
}

FirewallController::FirewallController(void) {
    // If no rules are set, it's in BLACKLIST mode
    mFirewallType = BLACKLIST;
    mIfaceRules = {};
}

int FirewallController::setupIptablesHooks(void) {
    int res = 0;
    mUseBpfOwnerMatch = getBpfOwnerStatus();
    if (mUseBpfOwnerMatch) {
        return res;
    }
    res |= createChain(LOCAL_DOZABLE, getFirewallType(DOZABLE));
    res |= createChain(LOCAL_STANDBY, getFirewallType(STANDBY));
    res |= createChain(LOCAL_POWERSAVE, getFirewallType(POWERSAVE));
    return res;
}

int FirewallController::enableFirewall(FirewallType ftype) {
    int res = 0;
    if (mFirewallType != ftype) {
        // flush any existing rules
        disableFirewall();

        if (ftype == WHITELIST) {
            // create default rule to drop all traffic
            std::string command =
                "*filter\n"
                "-A fw_INPUT -j DROP\n"
                "-A fw_OUTPUT -j REJECT\n"
                "-A fw_FORWARD -j REJECT\n"
                "COMMIT\n";
            res = execIptablesRestore(V4V6, command.c_str());
        }

        // Set this after calling disableFirewall(), since it defaults to WHITELIST there
        mFirewallType = ftype;
    }
    return res;
}

int FirewallController::disableFirewall(void) {
    mFirewallType = WHITELIST;
    mIfaceRules.clear();

    // flush any existing rules
    std::string command =
        "*filter\n"
        ":fw_INPUT -\n"
        ":fw_OUTPUT -\n"
        ":fw_FORWARD -\n"
        "COMMIT\n";

    return execIptablesRestore(V4V6, command.c_str());
}

int FirewallController::enableChildChains(ChildChain chain, bool enable) {
    int res = 0;
    const char* name;
    switch(chain) {
        case DOZABLE:
            name = LOCAL_DOZABLE;
            break;
        case STANDBY:
            name = LOCAL_STANDBY;
            break;
        case POWERSAVE:
            name = LOCAL_POWERSAVE;
            break;
        default:
            return res;
    }

    if (mUseBpfOwnerMatch) {
        return gCtls->trafficCtrl.toggleUidOwnerMap(chain, enable);
    }

    std::string command = "*filter\n";
    for (const char *parent : { LOCAL_INPUT, LOCAL_OUTPUT }) {
        StringAppendF(&command, "%s %s -j %s\n", (enable ? "-A" : "-D"), parent, name);
    }
    StringAppendF(&command, "COMMIT\n");

    return execIptablesRestore(V4V6, command);
}

int FirewallController::isFirewallEnabled(void) {
    // TODO: verify that rules are still in place near top
    return -1;
}

int FirewallController::setInterfaceRule(const char* iface, FirewallRule rule) {
    if (mFirewallType == BLACKLIST) {
        // Unsupported in BLACKLIST mode
        return -1;
    }

    if (!isIfaceName(iface)) {
        errno = ENOENT;
        return -1;
    }

    // Only delete rules if we actually added them, because otherwise our iptables-restore
    // processes will terminate with "no such rule" errors and cause latency penalties while we
    // spin up new ones.
    const char* op;
    if (rule == ALLOW && mIfaceRules.find(iface) == mIfaceRules.end()) {
        op = "-I";
        mIfaceRules.insert(iface);
    } else if (rule == DENY && mIfaceRules.find(iface) != mIfaceRules.end()) {
        op = "-D";
        mIfaceRules.erase(iface);
    } else {
        return 0;
    }

    std::string command = Join(std::vector<std::string> {
        "*filter",
        StringPrintf("%s fw_INPUT -i %s -j RETURN", op, iface),
        StringPrintf("%s fw_OUTPUT -o %s -j RETURN", op, iface),
        "COMMIT\n"
    }, "\n");
    return execIptablesRestore(V4V6, command);
}

FirewallType FirewallController::getFirewallType(ChildChain chain) {
    switch(chain) {
        case DOZABLE:
            return WHITELIST;
        case STANDBY:
            return BLACKLIST;
        case POWERSAVE:
            return WHITELIST;
        case NONE:
            return mFirewallType;
        default:
            return BLACKLIST;
    }
}

int FirewallController::setUidRule(ChildChain chain, int uid, FirewallRule rule) {
    const char* op;
    const char* target;
    FirewallType firewallType = getFirewallType(chain);
    if (firewallType == WHITELIST) {
        target = "RETURN";
        // When adding, insert RETURN rules at the front, before the catch-all DROP at the end.
        op = (rule == ALLOW)? "-I" : "-D";
    } else { // BLACKLIST mode
        target = "DROP";
        // When adding, append DROP rules at the end, after the RETURN rule that matches TCP RSTs.
        op = (rule == DENY)? "-A" : "-D";
    }

    std::vector<std::string> chainNames;
    switch(chain) {
        case DOZABLE:
            chainNames = { LOCAL_DOZABLE };
            break;
        case STANDBY:
            chainNames = { LOCAL_STANDBY };
            break;
        case POWERSAVE:
            chainNames = { LOCAL_POWERSAVE };
            break;
        case NONE:
            chainNames = { LOCAL_INPUT, LOCAL_OUTPUT };
            break;
        default:
            ALOGW("Unknown child chain: %d", chain);
            return -1;
    }
    if (mUseBpfOwnerMatch) {
        return gCtls->trafficCtrl.changeUidOwnerRule(chain, uid, rule, firewallType);
    }

    std::string command = "*filter\n";
    for (std::string chainName : chainNames) {
        StringAppendF(&command, "%s %s -m owner --uid-owner %d -j %s\n",
                      op, chainName.c_str(), uid, target);
    }
    StringAppendF(&command, "COMMIT\n");

    return execIptablesRestore(V4V6, command);
}

int FirewallController::createChain(const char* chain, FirewallType type) {
    static const std::vector<int32_t> NO_UIDS;
    return replaceUidChain(chain, type == WHITELIST, NO_UIDS);
}

/* static */
std::string FirewallController::makeCriticalCommands(IptablesTarget target, const char* chainName) {
    // Allow ICMPv6 packets necessary to make IPv6 connectivity work. http://b/23158230 .
    std::string commands;
    if (target == V6) {
        for (size_t i = 0; i < ARRAY_SIZE(ICMPV6_TYPES); i++) {
            StringAppendF(&commands, "-A %s -p icmpv6 --icmpv6-type %s -j RETURN\n",
                   chainName, ICMPV6_TYPES[i]);
        }
    }
    return commands;
}

std::string FirewallController::makeUidRules(IptablesTarget target, const char *name,
        bool isWhitelist, const std::vector<int32_t>& uids) {
    std::string commands;
    StringAppendF(&commands, "*filter\n:%s -\n", name);

    // Whitelist chains have UIDs at the beginning, and new UIDs are added with '-I'.
    if (isWhitelist) {
        for (auto uid : uids) {
            StringAppendF(&commands, "-A %s -m owner --uid-owner %d -j RETURN\n", name, uid);
        }

        // Always whitelist system UIDs.
        StringAppendF(&commands,
                "-A %s -m owner --uid-owner %d-%d -j RETURN\n", name, 0, MAX_SYSTEM_UID);

        // This rule inverts the match for all UIDs; ie, if there is no UID match here,
        // there is no socket to be found
        StringAppendF(&commands,
                "-A %s -m owner ! --uid-owner %d-%u -j RETURN\n", name, 0, UINT32_MAX-1);

        // Always whitelist traffic with protocol ESP, or no known socket - required for IPSec
        StringAppendF(&commands, "-A %s -p esp -j RETURN\n", name);
    }

    // Always allow networking on loopback.
    StringAppendF(&commands, "-A %s -i lo -j RETURN\n", name);
    StringAppendF(&commands, "-A %s -o lo -j RETURN\n", name);

    // Allow TCP RSTs so we can cleanly close TCP connections of apps that no longer have network
    // access. Both incoming and outgoing RSTs are allowed.
    StringAppendF(&commands, "-A %s -p tcp --tcp-flags RST RST -j RETURN\n", name);

    if (isWhitelist) {
        commands.append(makeCriticalCommands(target, name));
    }

    // Blacklist chains have UIDs at the end, and new UIDs are added with '-A'.
    if (!isWhitelist) {
        for (auto uid : uids) {
            StringAppendF(&commands, "-A %s -m owner --uid-owner %d -j DROP\n", name, uid);
        }
    }

    // If it's a whitelist chain, add a default DROP at the end. This is not necessary for a
    // blacklist chain, because all user-defined chains implicitly RETURN at the end.
    if (isWhitelist) {
        StringAppendF(&commands, "-A %s -j DROP\n", name);
    }

    StringAppendF(&commands, "COMMIT\n");

    return commands;
}

int FirewallController::replaceUidChain(
        const char *name, bool isWhitelist, const std::vector<int32_t>& uids) {
   if (mUseBpfOwnerMatch) {
       return gCtls->trafficCtrl.replaceUidOwnerMap(name, isWhitelist, uids);
   }
   std::string commands4 = makeUidRules(V4, name, isWhitelist, uids);
   std::string commands6 = makeUidRules(V6, name, isWhitelist, uids);
   return execIptablesRestore(V4, commands4.c_str()) | execIptablesRestore(V6, commands6.c_str());
}
