/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "ResolverController"
#define DBG 0

#include <algorithm>
#include <cstdlib>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <cutils/log.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netdb.h>

#include <arpa/inet.h>
// NOTE: <resolv_netid.h> is a private C library header that provides
//       declarations for _resolv_set_nameservers_for_net and
//       _resolv_flush_cache_for_net
#include <resolv_netid.h>
#include <resolv_params.h>
#include <resolv_stats.h>

#include <android-base/strings.h>
#include <android-base/thread_annotations.h>
#include <android/net/INetd.h>
#include <android/net/metrics/INetdEventListener.h>

#include "DumpWriter.h"
#include "EventReporter.h"
#include "NetdConstants.h"
#include "ResolverController.h"
#include "ResolverStats.h"
#include "dns/DnsTlsTransport.h"
#include "dns/DnsTlsServer.h"
#include "netdutils/BackoffSequence.h"

namespace android {

using netdutils::BackoffSequence;

namespace net {
namespace {

std::string addrToString(const sockaddr_storage* addr) {
    char out[INET6_ADDRSTRLEN] = {0};
    getnameinfo((const sockaddr*)addr, sizeof(sockaddr_storage), out,
            INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
    return std::string(out);
}

bool parseServer(const char* server, sockaddr_storage* parsed) {
    addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_flags = AI_NUMERICHOST | AI_NUMERICSERV
    };
    addrinfo* res;

    int err = getaddrinfo(server, "853", &hints, &res);
    if (err != 0) {
        ALOGW("Failed to parse server address (%s): %s", server, gai_strerror(err));
        return false;
    }

    memcpy(parsed, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    return true;
}

const char* getPrivateDnsModeString(PrivateDnsMode mode) {
    switch (mode) {
        case PrivateDnsMode::OFF: return "OFF";
        case PrivateDnsMode::OPPORTUNISTIC: return "OPPORTUNISTIC";
        case PrivateDnsMode::STRICT: return "STRICT";
    }
}

constexpr const char* validationStatusToString(ResolverController::Validation value) {
    switch (value) {
        case ResolverController::Validation::in_process:     return "in_process";
        case ResolverController::Validation::success:        return "success";
        case ResolverController::Validation::fail:           return "fail";
        case ResolverController::Validation::unknown_server: return "unknown_server";
        case ResolverController::Validation::unknown_netid:  return "unknown_netid";
        default: return "unknown_status";
    }
}


class PrivateDnsConfiguration {
  public:
    typedef ResolverController::PrivateDnsStatus PrivateDnsStatus;
    typedef ResolverController::Validation Validation;
    typedef std::map<DnsTlsServer, Validation, AddressComparator> PrivateDnsTracker;

    int set(int32_t netId, const std::vector<std::string>& servers, const std::string& name,
            const std::set<std::vector<uint8_t>>& fingerprints) {
        if (DBG) {
            ALOGD("PrivateDnsConfiguration::set(%u, %zu, %s, %zu)",
                    netId, servers.size(), name.c_str(), fingerprints.size());
        }

        const bool explicitlyConfigured = !name.empty() || !fingerprints.empty();

        // Parse the list of servers that has been passed in
        std::set<DnsTlsServer> tlsServers;
        for (size_t i = 0; i < servers.size(); ++i) {
            sockaddr_storage parsed;
            if (!parseServer(servers[i].c_str(), &parsed)) {
                return -EINVAL;
            }
            DnsTlsServer server(parsed);
            server.name = name;
            server.fingerprints = fingerprints;
            tlsServers.insert(server);
        }

        std::lock_guard<std::mutex> guard(mPrivateDnsLock);
        if (explicitlyConfigured) {
            mPrivateDnsModes[netId] = PrivateDnsMode::STRICT;
        } else if (!tlsServers.empty()) {
            mPrivateDnsModes[netId] = PrivateDnsMode::OPPORTUNISTIC;
        } else {
            mPrivateDnsModes[netId] = PrivateDnsMode::OFF;
            mPrivateDnsTransports.erase(netId);
            return 0;
        }

        // Create the tracker if it was not present
        auto netPair = mPrivateDnsTransports.find(netId);
        if (netPair == mPrivateDnsTransports.end()) {
            // No TLS tracker yet for this netId.
            bool added;
            std::tie(netPair, added) = mPrivateDnsTransports.emplace(netId, PrivateDnsTracker());
            if (!added) {
                ALOGE("Memory error while recording private DNS for netId %d", netId);
                return -ENOMEM;
            }
        }
        auto& tracker = netPair->second;

        // Remove any servers from the tracker that are not in |servers| exactly.
        for (auto it = tracker.begin(); it != tracker.end();) {
            if (tlsServers.count(it->first) == 0) {
                it = tracker.erase(it);
            } else {
                ++it;
            }
        }

        // Add any new or changed servers to the tracker, and initiate async checks for them.
        for (const auto& server : tlsServers) {
            // Don't probe a server more than once.  This means that the only way to
            // re-check a failed server is to remove it and re-add it from the netId.
            if (tracker.count(server) == 0) {
                validatePrivateDnsProvider(server, tracker, netId);
            }
        }
        return 0;
    }

    PrivateDnsStatus getStatus(unsigned netId) {
        PrivateDnsStatus status{PrivateDnsMode::OFF, {}};

        // This mutex is on the critical path of every DNS lookup.
        //
        // If the overhead of mutex acquisition proves too high, we could reduce
        // it by maintaining an atomic_int32_t counter of TLS-enabled netids, or
        // by using an RWLock.
        std::lock_guard<std::mutex> guard(mPrivateDnsLock);

        const auto mode = mPrivateDnsModes.find(netId);
        if (mode == mPrivateDnsModes.end()) return status;
        status.mode = mode->second;

        const auto netPair = mPrivateDnsTransports.find(netId);
        if (netPair != mPrivateDnsTransports.end()) {
            for (const auto& serverPair : netPair->second) {
                if (serverPair.second == Validation::success) {
                    status.validatedServers.push_back(serverPair.first);
                }
            }
        }

        return status;
    }

    void clear(unsigned netId) {
        if (DBG) {
            ALOGD("PrivateDnsConfiguration::clear(%u)", netId);
        }
        std::lock_guard<std::mutex> guard(mPrivateDnsLock);
        mPrivateDnsModes.erase(netId);
        mPrivateDnsTransports.erase(netId);
    }

    void dump(DumpWriter& dw, unsigned netId) {
        std::lock_guard<std::mutex> guard(mPrivateDnsLock);

        const auto& mode = mPrivateDnsModes.find(netId);
        dw.println("Private DNS mode: %s", getPrivateDnsModeString(
                (mode != mPrivateDnsModes.end()) ? mode->second : PrivateDnsMode::OFF));
        const auto& netPair = mPrivateDnsTransports.find(netId);
        if (netPair == mPrivateDnsTransports.end()) {
            dw.println("No Private DNS servers configured");
        } else {
            const auto& tracker = netPair->second;
            dw.println("Private DNS configuration (%zu entries)", tracker.size());
            dw.incIndent();
            for (const auto& kv : tracker) {
                const auto& server = kv.first;
                const auto status = kv.second;
                dw.println("%s name{%s} status{%s}",
                        addrToString(&(server.ss)).c_str(),
                        server.name.c_str(),
                        validationStatusToString(status));
            }
            dw.decIndent();
        }
    }

  private:
    void validatePrivateDnsProvider(const DnsTlsServer& server, PrivateDnsTracker& tracker,
            unsigned netId) REQUIRES(mPrivateDnsLock) {
        if (DBG) {
            ALOGD("validatePrivateDnsProvider(%s, %u)", addrToString(&(server.ss)).c_str(), netId);
        }

        tracker[server] = Validation::in_process;
        if (DBG) {
            ALOGD("Server %s marked as in_process.  Tracker now has size %zu",
                    addrToString(&(server.ss)).c_str(), tracker.size());
        }
        // Note that capturing |server| and |netId| in this lambda create copies.
        std::thread validate_thread([this, server, netId] {
            // cat /proc/sys/net/ipv4/tcp_syn_retries yields "6".
            //
            // Start with a 1 minute delay and backoff to once per hour.
            //
            // Assumptions:
            //     [1] Each TLS validation is ~10KB of certs+handshake+payload.
            //     [2] Network typically provision clients with <=4 nameservers.
            //     [3] Average month has 30 days.
            //
            // Each validation pass in a given hour is ~1.2MB of data. And 24
            // such validation passes per day is about ~30MB per month, in the
            // worst case. Otherwise, this will cost ~600 SYNs per month
            // (6 SYNs per ip, 4 ips per validation pass, 24 passes per day).
            auto backoff = BackoffSequence<>::Builder()
                    .withInitialRetransmissionTime(std::chrono::seconds(60))
                    .withMaximumRetransmissionTime(std::chrono::seconds(3600))
                    .build();

            while (true) {
                // ::validate() is a blocking call that performs network operations.
                // It can take milliseconds to minutes, up to the SYN retry limit.
                const bool success = DnsTlsTransport::validate(server, netId);
                if (DBG) {
                    ALOGD("validateDnsTlsServer returned %d for %s", success,
                            addrToString(&(server.ss)).c_str());
                }

                const bool needs_reeval = this->recordPrivateDnsValidation(server, netId, success);
                if (!needs_reeval) {
                    break;
                }

                if (backoff.hasNextTimeout()) {
                    std::this_thread::sleep_for(backoff.getNextTimeout());
                } else {
                    break;
                }
            }
        });
        validate_thread.detach();
    }

    bool recordPrivateDnsValidation(const DnsTlsServer& server, unsigned netId, bool success) {
        constexpr bool NEEDS_REEVALUATION = true;
        constexpr bool DONT_REEVALUATE = false;

        std::lock_guard<std::mutex> guard(mPrivateDnsLock);

        auto netPair = mPrivateDnsTransports.find(netId);
        if (netPair == mPrivateDnsTransports.end()) {
            ALOGW("netId %u was erased during private DNS validation", netId);
            return DONT_REEVALUATE;
        }

        bool reevaluationStatus = success ? DONT_REEVALUATE : NEEDS_REEVALUATION;

        auto& tracker = netPair->second;
        auto serverPair = tracker.find(server);
        if (serverPair == tracker.end()) {
            ALOGW("Server %s was removed during private DNS validation",
                    addrToString(&(server.ss)).c_str());
            success = false;
            reevaluationStatus = DONT_REEVALUATE;
        } else if (!(serverPair->first == server)) {
            // TODO: It doesn't seem correct to overwrite the tracker entry for
            // |server| down below in this circumstance... Fix this.
            ALOGW("Server %s was changed during private DNS validation",
                    addrToString(&(server.ss)).c_str());
            success = false;
            reevaluationStatus = DONT_REEVALUATE;
        }

        // Send a validation event to NetdEventListenerService.
        if (mNetdEventListener == nullptr) {
            mNetdEventListener = mEventReporter.getNetdEventListener();
        }
        if (mNetdEventListener != nullptr) {
            const String16 ipLiteral(addrToString(&(server.ss)).c_str());
            const String16 hostname(server.name.empty() ? "" : server.name.c_str());
            mNetdEventListener->onPrivateDnsValidationEvent(netId, ipLiteral, hostname, success);
            if (DBG) {
                ALOGD("Sending validation %s event on netId %u for %s with hostname %s",
                        success ? "success" : "failure", netId,
                        addrToString(&(server.ss)).c_str(), server.name.c_str());
            }
        } else {
            ALOGE("Validation event not sent since NetdEventListenerService is unavailable.");
        }

        if (success) {
            tracker[server] = Validation::success;
            if (DBG) {
                ALOGD("Validation succeeded for %s! Tracker now has %zu entries.",
                        addrToString(&(server.ss)).c_str(), tracker.size());
            }
        } else {
            // Validation failure is expected if a user is on a captive portal.
            // TODO: Trigger a second validation attempt after captive portal login
            // succeeds.
            tracker[server] = Validation::fail;
            if (DBG) {
                ALOGD("Validation failed for %s!", addrToString(&(server.ss)).c_str());
            }
        }

        return reevaluationStatus;
    }

    EventReporter mEventReporter;

    std::mutex mPrivateDnsLock;
    std::map<unsigned, PrivateDnsMode> mPrivateDnsModes GUARDED_BY(mPrivateDnsLock);
    // Structure for tracking the validation status of servers on a specific netId.
    // Using the AddressComparator ensures at most one entry per IP address.
    std::map<unsigned, PrivateDnsTracker> mPrivateDnsTransports GUARDED_BY(mPrivateDnsLock);
    android::sp<android::net::metrics::INetdEventListener>
            mNetdEventListener GUARDED_BY(mPrivateDnsLock);
} privateDnsConfiguration;

}  // namespace

int ResolverController::setDnsServers(unsigned netId, const char* searchDomains,
        const char** servers, int numservers, const __res_params* params) {
    if (DBG) {
        ALOGD("setDnsServers netId = %u, numservers = %d", netId, numservers);
    }
    return -_resolv_set_nameservers_for_net(netId, servers, numservers, searchDomains, params);
}

ResolverController::PrivateDnsStatus
ResolverController::getPrivateDnsStatus(unsigned netId) const {
    return privateDnsConfiguration.getStatus(netId);
}

int ResolverController::clearDnsServers(unsigned netId) {
    _resolv_set_nameservers_for_net(netId, NULL, 0, "", NULL);
    if (DBG) {
        ALOGD("clearDnsServers netId = %u\n", netId);
    }
    privateDnsConfiguration.clear(netId);
    return 0;
}

int ResolverController::flushDnsCache(unsigned netId) {
    if (DBG) {
        ALOGD("flushDnsCache netId = %u\n", netId);
    }

    _resolv_flush_cache_for_net(netId);

    return 0;
}

int ResolverController::getDnsInfo(unsigned netId, std::vector<std::string>* servers,
        std::vector<std::string>* domains, __res_params* params,
        std::vector<android::net::ResolverStats>* stats) {
    using android::net::ResolverStats;
    using android::net::INetd;
    static_assert(ResolverStats::STATS_SUCCESSES == INetd::RESOLVER_STATS_SUCCESSES &&
            ResolverStats::STATS_ERRORS == INetd::RESOLVER_STATS_ERRORS &&
            ResolverStats::STATS_TIMEOUTS == INetd::RESOLVER_STATS_TIMEOUTS &&
            ResolverStats::STATS_INTERNAL_ERRORS == INetd::RESOLVER_STATS_INTERNAL_ERRORS &&
            ResolverStats::STATS_RTT_AVG == INetd::RESOLVER_STATS_RTT_AVG &&
            ResolverStats::STATS_LAST_SAMPLE_TIME == INetd::RESOLVER_STATS_LAST_SAMPLE_TIME &&
            ResolverStats::STATS_USABLE == INetd::RESOLVER_STATS_USABLE &&
            ResolverStats::STATS_COUNT == INetd::RESOLVER_STATS_COUNT,
            "AIDL and ResolverStats.h out of sync");
    int nscount = -1;
    sockaddr_storage res_servers[MAXNS];
    int dcount = -1;
    char res_domains[MAXDNSRCH][MAXDNSRCHPATH];
    __res_stats res_stats[MAXNS];
    servers->clear();
    domains->clear();
    *params = __res_params{};
    stats->clear();
    int revision_id = android_net_res_stats_get_info_for_net(netId, &nscount, res_servers, &dcount,
            res_domains, params, res_stats);

    // If the netId is unknown (which can happen for valid net IDs for which no DNS servers have
    // yet been configured), there is no revision ID. In this case there is no data to return.
    if (revision_id < 0) {
        return 0;
    }

    // Verify that the returned data is sane.
    if (nscount < 0 || nscount > MAXNS || dcount < 0 || dcount > MAXDNSRCH) {
        ALOGE("%s: nscount=%d, dcount=%d", __FUNCTION__, nscount, dcount);
        return -ENOTRECOVERABLE;
    }

    // Determine which servers are considered usable by the resolver.
    bool valid_servers[MAXNS];
    std::fill_n(valid_servers, MAXNS, false);
    android_net_res_stats_get_usable_servers(params, res_stats, nscount, valid_servers);

    // Convert the server sockaddr structures to std::string.
    stats->resize(nscount);
    for (int i = 0 ; i < nscount ; ++i) {
        char hbuf[NI_MAXHOST];
        int rv = getnameinfo(reinterpret_cast<const sockaddr*>(&res_servers[i]),
                sizeof(res_servers[i]), hbuf, sizeof(hbuf), nullptr, 0, NI_NUMERICHOST);
        std::string server_str;
        if (rv == 0) {
            server_str.assign(hbuf);
        } else {
            ALOGE("getnameinfo() failed for server #%d: %s", i, gai_strerror(rv));
            server_str.assign("<invalid>");
        }
        servers->push_back(std::move(server_str));
        android::net::ResolverStats& cur_stats = (*stats)[i];
        android_net_res_stats_aggregate(&res_stats[i], &cur_stats.successes, &cur_stats.errors,
                &cur_stats.timeouts, &cur_stats.internal_errors, &cur_stats.rtt_avg,
                &cur_stats.last_sample_time);
        cur_stats.usable = valid_servers[i];
    }

    // Convert the stack-allocated search domain strings to std::string.
    for (int i = 0 ; i < dcount ; ++i) {
        domains->push_back(res_domains[i]);
    }
    return 0;
}

int ResolverController::setResolverConfiguration(int32_t netId,
        const std::vector<std::string>& servers, const std::vector<std::string>& domains,
        const std::vector<int32_t>& params, const std::string& tlsName,
        const std::vector<std::string>& tlsServers,
        const std::set<std::vector<uint8_t>>& tlsFingerprints) {
    using android::net::INetd;
    if (params.size() != INetd::RESOLVER_PARAMS_COUNT) {
        ALOGE("%s: params.size()=%zu", __FUNCTION__, params.size());
        return -EINVAL;
    }

    const int err = privateDnsConfiguration.set(netId, tlsServers, tlsName, tlsFingerprints);
    if (err != 0) {
        return err;
    }

    // Convert network-assigned server list to bionic's format.
    auto server_count = std::min<size_t>(MAXNS, servers.size());
    std::vector<const char*> server_ptrs;
    for (size_t i = 0 ; i < server_count ; ++i) {
        server_ptrs.push_back(servers[i].c_str());
    }

    std::string domains_str;
    if (!domains.empty()) {
        domains_str = domains[0];
        for (size_t i = 1 ; i < domains.size() ; ++i) {
            domains_str += " " + domains[i];
        }
    }

    __res_params res_params;
    res_params.sample_validity = params[INetd::RESOLVER_PARAMS_SAMPLE_VALIDITY];
    res_params.success_threshold = params[INetd::RESOLVER_PARAMS_SUCCESS_THRESHOLD];
    res_params.min_samples = params[INetd::RESOLVER_PARAMS_MIN_SAMPLES];
    res_params.max_samples = params[INetd::RESOLVER_PARAMS_MAX_SAMPLES];

    return setDnsServers(netId, domains_str.c_str(), server_ptrs.data(), server_ptrs.size(),
            &res_params);
}

int ResolverController::getResolverInfo(int32_t netId, std::vector<std::string>* servers,
        std::vector<std::string>* domains, std::vector<int32_t>* params,
        std::vector<int32_t>* stats) {
    using android::net::ResolverStats;
    using android::net::INetd;
    __res_params res_params;
    std::vector<ResolverStats> res_stats;
    int ret = getDnsInfo(netId, servers, domains, &res_params, &res_stats);
    if (ret != 0) {
        return ret;
    }

    // Serialize the information for binder.
    ResolverStats::encodeAll(res_stats, stats);

    params->resize(INetd::RESOLVER_PARAMS_COUNT);
    (*params)[INetd::RESOLVER_PARAMS_SAMPLE_VALIDITY] = res_params.sample_validity;
    (*params)[INetd::RESOLVER_PARAMS_SUCCESS_THRESHOLD] = res_params.success_threshold;
    (*params)[INetd::RESOLVER_PARAMS_MIN_SAMPLES] = res_params.min_samples;
    (*params)[INetd::RESOLVER_PARAMS_MAX_SAMPLES] = res_params.max_samples;
    return 0;
}

void ResolverController::dump(DumpWriter& dw, unsigned netId) {
    // No lock needed since Bionic's resolver locks all accessed data structures internally.
    using android::net::ResolverStats;
    std::vector<std::string> servers;
    std::vector<std::string> domains;
    __res_params params;
    std::vector<ResolverStats> stats;
    time_t now = time(nullptr);
    int rv = getDnsInfo(netId, &servers, &domains, &params, &stats);
    dw.incIndent();
    if (rv != 0) {
        dw.println("getDnsInfo() failed for netid %u", netId);
    } else {
        if (servers.empty()) {
            dw.println("No DNS servers defined");
        } else {
            dw.println("DNS servers: # IP (total, successes, errors, timeouts, internal errors, "
                    "RTT avg, last sample)");
            dw.incIndent();
            for (size_t i = 0 ; i < servers.size() ; ++i) {
                if (i < stats.size()) {
                    const ResolverStats& s = stats[i];
                    int total = s.successes + s.errors + s.timeouts + s.internal_errors;
                    if (total > 0) {
                        int time_delta = (s.last_sample_time > 0) ? now - s.last_sample_time : -1;
                        dw.println("%s (%d, %d, %d, %d, %d, %dms, %ds)%s", servers[i].c_str(),
                                total, s.successes, s.errors, s.timeouts, s.internal_errors,
                                s.rtt_avg, time_delta, s.usable ? "" : " BROKEN");
                    } else {
                        dw.println("%s <no data>", servers[i].c_str());
                    }
                } else {
                    dw.println("%s <no stats>", servers[i].c_str());
                }
            }
            dw.decIndent();
        }
        if (domains.empty()) {
            dw.println("No search domains defined");
        } else {
            std::string domains_str = android::base::Join(domains, ", ");
            dw.println("search domains: %s", domains_str.c_str());
        }
        if (params.sample_validity != 0) {
            dw.println("DNS parameters: sample validity = %us, success threshold = %u%%, "
                    "samples (min, max) = (%u, %u)", params.sample_validity,
                    static_cast<unsigned>(params.success_threshold),
                    static_cast<unsigned>(params.min_samples),
                    static_cast<unsigned>(params.max_samples));
        }

        privateDnsConfiguration.dump(dw, netId);
    }
    dw.decIndent();
}

}  // namespace net
}  // namespace android
