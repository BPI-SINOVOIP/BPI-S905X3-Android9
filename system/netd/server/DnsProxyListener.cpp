/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <linux/if.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <resolv_netid.h>
#include <net/if.h>

#define LOG_TAG "DnsProxyListener"
#define DBG 0
#define VDBG 0

#include <algorithm>
#include <chrono>
#include <list>
#include <vector>

#include <cutils/log.h>
#include <cutils/misc.h>
#include <netdutils/Slice.h>
#include <netdutils/OperationLimiter.h>
#include <utils/String16.h>
#include <sysutils/SocketClient.h>

#include <binder/IServiceManager.h>

#include "Controllers.h"
#include "Fwmark.h"
#include "DnsProxyListener.h"
#include "dns/DnsTlsDispatcher.h"
#include "dns/DnsTlsTransport.h"
#include "dns/DnsTlsServer.h"
#include "NetdClient.h"
#include "NetdConstants.h"
#include "NetworkController.h"
#include "ResponseCode.h"
#include "Stopwatch.h"
#include "thread_util.h"
#include "android/net/metrics/INetdEventListener.h"

using android::String16;
using android::net::metrics::INetdEventListener;

namespace android {
namespace net {

namespace {

// TODO: move to a separate file (with other constants from FwmarkService and NetdNativeService)
constexpr const char CONNECTIVITY_USE_RESTRICTED_NETWORKS[] =
    "android.permission.CONNECTIVITY_USE_RESTRICTED_NETWORKS";
constexpr const char NETWORK_BYPASS_PRIVATE_DNS[] =
    "android.permission.NETWORK_BYPASS_PRIVATE_DNS";

// Limits the number of outstanding DNS queries by client UID.
constexpr int MAX_QUERIES_PER_UID = 256;
android::netdutils::OperationLimiter<uid_t> queryLimiter(MAX_QUERIES_PER_UID);

void logArguments(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        ALOGD("argv[%i]=%s", i, argv[i]);
    }
}

template<typename T>
void tryThreadOrError(SocketClient* cli, T* handler) {
    cli->incRef();

    const int rval = threadLaunch(handler);
    if (rval == 0) {
        // SocketClient decRef() happens in the handler's run() method.
        return;
    }

    char* msg = NULL;
    asprintf(&msg, "%s (%d)", strerror(-rval), -rval);
    cli->sendMsg(ResponseCode::OperationFailed, msg, false);
    free(msg);

    delete handler;
    cli->decRef();
}

bool checkAndClearUseLocalNameserversFlag(unsigned* netid) {
    if (netid == nullptr || ((*netid) & NETID_USE_LOCAL_NAMESERVERS) == 0) {
        return false;
    }
    *netid = (*netid) & ~NETID_USE_LOCAL_NAMESERVERS;
    return true;
}

thread_local android_net_context thread_netcontext = {};

DnsTlsDispatcher dnsTlsDispatcher;

void catnap() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
}

res_sendhookact qhook(sockaddr* const * /*ns*/, const u_char** buf, int* buflen,
                      u_char* ans, int anssiz, int* resplen) {
    if (!thread_netcontext.qhook) {
        ALOGE("qhook abort: thread qhook is null");
        return res_goahead;
    }
    if (thread_netcontext.flags & NET_CONTEXT_FLAG_USE_LOCAL_NAMESERVERS) {
        return res_goahead;
    }
    if (!net::gCtls) {
        ALOGE("qhook abort: gCtls is null");
        return res_goahead;
    }

    const auto privateDnsStatus =
            net::gCtls->resolverCtrl.getPrivateDnsStatus(thread_netcontext.dns_netid);

    if (privateDnsStatus.mode == PrivateDnsMode::OFF) return res_goahead;

    if (privateDnsStatus.validatedServers.empty()) {
        if (privateDnsStatus.mode == PrivateDnsMode::OPPORTUNISTIC) {
            return res_goahead;
        } else {
            // Sleep and iterate some small number of times checking for the
            // arrival of resolved and validated server IP addresses, instead
            // of returning an immediate error.
            catnap();
            return res_modified;
        }
    }

    if (DBG) ALOGD("Performing query over TLS");

    Slice query = netdutils::Slice(const_cast<u_char*>(*buf), *buflen);
    Slice answer = netdutils::Slice(const_cast<u_char*>(ans), anssiz);
    const auto response = dnsTlsDispatcher.query(
            privateDnsStatus.validatedServers, thread_netcontext.dns_mark,
            query, answer, resplen);
    if (response == DnsTlsTransport::Response::success) {
        if (DBG) ALOGD("qhook success");
        return res_done;
    }

    if (DBG) {
        ALOGW("qhook abort: TLS query failed: %d", (int)response);
    }

    if (privateDnsStatus.mode == PrivateDnsMode::OPPORTUNISTIC) {
        // In opportunistic mode, handle falling back to cleartext in some
        // cases (DNS shouldn't fail if a validated opportunistic mode server
        // becomes unreachable for some reason).
        switch (response) {
            case DnsTlsTransport::Response::network_error:
            case DnsTlsTransport::Response::internal_error:
                // Note: this will cause cleartext queries to be emitted, with
                // all of the EDNS0 goodness enabled. Fingers crossed.  :-/
                return res_goahead;
            default:
                break;
        }
    }

    // There was an internal error.  Fail hard.
    return res_error;
}

constexpr bool requestingUseLocalNameservers(unsigned flags) {
    return (flags & NET_CONTEXT_FLAG_USE_LOCAL_NAMESERVERS) != 0;
}

inline bool queryingViaTls(unsigned dns_netid) {
    const auto privateDnsStatus = net::gCtls->resolverCtrl.getPrivateDnsStatus(dns_netid);
    switch (privateDnsStatus.mode) {
        case PrivateDnsMode::OPPORTUNISTIC:
           return !privateDnsStatus.validatedServers.empty();
        case PrivateDnsMode::STRICT:
            return true;
        default:
            return false;
    }
}

bool hasPermissionToBypassPrivateDns(uid_t uid) {
    static_assert(AID_SYSTEM >= 0 && AID_SYSTEM < FIRST_APPLICATION_UID,
        "Calls from AID_SYSTEM must not result in a permission check to avoid deadlock.");
    if (uid >= 0 && uid < FIRST_APPLICATION_UID) {
        return true;
    }

    for (auto& permission : {CONNECTIVITY_USE_RESTRICTED_NETWORKS, NETWORK_BYPASS_PRIVATE_DNS}) {
        if (checkCallingPermission(String16(permission))) {
            return true;
        }
    }
    return false;
}

void maybeFixupNetContext(android_net_context* ctx) {
    if (requestingUseLocalNameservers(ctx->flags) && !hasPermissionToBypassPrivateDns(ctx->uid)) {
        // Not permitted; clear the flag.
        ctx->flags &= ~NET_CONTEXT_FLAG_USE_LOCAL_NAMESERVERS;
    }

    if (!requestingUseLocalNameservers(ctx->flags)) {
        // If we're not explicitly bypassing DNS-over-TLS servers, check whether
        // DNS-over-TLS is in use as an indicator for when to use more modern
        // DNS resolution mechanics.
        if (queryingViaTls(ctx->dns_netid)) {
            ctx->flags |= NET_CONTEXT_FLAG_USE_EDNS;
        }
    }

    // Always set the qhook. An opportunistic mode server might have finished
    // validating by the time the qhook runs. Note that this races with the
    // queryingViaTls() check above, resulting in possibly sending queries over
    // TLS without taking advantage of features like EDNS; c'est la guerre.
    ctx->qhook = &qhook;

    // Store the android_net_context instance in a thread_local variable
    // so that the static qhook can access other fields of the struct.
    thread_netcontext = *ctx;
}

}  // namespace

DnsProxyListener::DnsProxyListener(const NetworkController* netCtrl, EventReporter* eventReporter) :
        FrameworkListener(SOCKET_NAME), mNetCtrl(netCtrl), mEventReporter(eventReporter) {
    registerCmd(new GetAddrInfoCmd(this));
    registerCmd(new GetHostByAddrCmd(this));
    registerCmd(new GetHostByNameCmd(this));
}

DnsProxyListener::GetAddrInfoHandler::GetAddrInfoHandler(
        SocketClient *c, char* host, char* service, struct addrinfo* hints,
        const android_net_context& netcontext, const int reportingLevel,
        const android::sp<android::net::metrics::INetdEventListener>& netdEventListener)
        : mClient(c),
          mHost(host),
          mService(service),
          mHints(hints),
          mNetContext(netcontext),
          mReportingLevel(reportingLevel),
          mNetdEventListener(netdEventListener) {
}

DnsProxyListener::GetAddrInfoHandler::~GetAddrInfoHandler() {
    free(mHost);
    free(mService);
    free(mHints);
}

static bool sendBE32(SocketClient* c, uint32_t data) {
    uint32_t be_data = htonl(data);
    return c->sendData(&be_data, sizeof(be_data)) == 0;
}

// Sends 4 bytes of big-endian length, followed by the data.
// Returns true on success.
static bool sendLenAndData(SocketClient* c, const int len, const void* data) {
    return sendBE32(c, len) && (len == 0 || c->sendData(data, len) == 0);
}

// Returns true on success
static bool sendhostent(SocketClient *c, struct hostent *hp) {
    bool success = true;
    int i;
    if (hp->h_name != NULL) {
        success &= sendLenAndData(c, strlen(hp->h_name)+1, hp->h_name);
    } else {
        success &= sendLenAndData(c, 0, "") == 0;
    }

    for (i=0; hp->h_aliases[i] != NULL; i++) {
        success &= sendLenAndData(c, strlen(hp->h_aliases[i])+1, hp->h_aliases[i]);
    }
    success &= sendLenAndData(c, 0, ""); // null to indicate we're done

    uint32_t buf = htonl(hp->h_addrtype);
    success &= c->sendData(&buf, sizeof(buf)) == 0;

    buf = htonl(hp->h_length);
    success &= c->sendData(&buf, sizeof(buf)) == 0;

    for (i=0; hp->h_addr_list[i] != NULL; i++) {
        success &= sendLenAndData(c, 16, hp->h_addr_list[i]);
    }
    success &= sendLenAndData(c, 0, ""); // null to indicate we're done
    return success;
}

static bool sendaddrinfo(SocketClient* c, struct addrinfo* ai) {
    // struct addrinfo {
    //      int     ai_flags;       /* AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST */
    //      int     ai_family;      /* PF_xxx */
    //      int     ai_socktype;    /* SOCK_xxx */
    //      int     ai_protocol;    /* 0 or IPPROTO_xxx for IPv4 and IPv6 */
    //      socklen_t ai_addrlen;   /* length of ai_addr */
    //      char    *ai_canonname;  /* canonical name for hostname */
    //      struct  sockaddr *ai_addr;      /* binary address */
    //      struct  addrinfo *ai_next;      /* next structure in linked list */
    // };

    // Write the struct piece by piece because we might be a 64-bit netd
    // talking to a 32-bit process.
    bool success =
            sendBE32(c, ai->ai_flags) &&
            sendBE32(c, ai->ai_family) &&
            sendBE32(c, ai->ai_socktype) &&
            sendBE32(c, ai->ai_protocol);
    if (!success) {
        return false;
    }

    // ai_addrlen and ai_addr.
    if (!sendLenAndData(c, ai->ai_addrlen, ai->ai_addr)) {
        return false;
    }

    // strlen(ai_canonname) and ai_canonname.
    if (!sendLenAndData(c, ai->ai_canonname ? strlen(ai->ai_canonname) + 1 : 0, ai->ai_canonname)) {
        return false;
    }

    return true;
}

void DnsProxyListener::GetAddrInfoHandler::run() {
    if (DBG) {
        ALOGD("GetAddrInfoHandler, now for %s / %s / {%u,%u,%u,%u,%u,%u}", mHost, mService,
                mNetContext.app_netid, mNetContext.app_mark,
                mNetContext.dns_netid, mNetContext.dns_mark,
                mNetContext.uid, mNetContext.flags);
    }

    struct addrinfo* result = NULL;
    Stopwatch s;
    maybeFixupNetContext(&mNetContext);
    const uid_t uid = mClient->getUid();
    uint32_t rv = 0;
    if (queryLimiter.start(uid)) {
        rv = android_getaddrinfofornetcontext(mHost, mService, mHints, &mNetContext, &result);
        queryLimiter.finish(uid);
    } else {
        // Note that this error code is currently not passed down to the client.
        // android_getaddrinfo_proxy() returns EAI_NODATA on any error.
        rv = EAI_MEMORY;
        ALOGE("getaddrinfo: from UID %d, max concurrent queries reached", uid);
    }
    const int latencyMs = lround(s.timeTaken());

    if (rv) {
        // getaddrinfo failed
        mClient->sendBinaryMsg(ResponseCode::DnsProxyOperationFailed, &rv, sizeof(rv));
    } else {
        bool success = !mClient->sendCode(ResponseCode::DnsProxyQueryResult);
        struct addrinfo* ai = result;
        while (ai && success) {
            success = sendBE32(mClient, 1) && sendaddrinfo(mClient, ai);
            ai = ai->ai_next;
        }
        success = success && sendBE32(mClient, 0);
        if (!success) {
            ALOGW("Error writing DNS result to client");
        }
    }
    std::vector<String16> ip_addrs;
    int total_ip_addr_count = 0;
    if (result) {
        if (mNetdEventListener != nullptr
                && mReportingLevel == INetdEventListener::REPORTING_LEVEL_FULL) {
            for (addrinfo* ai = result; ai; ai = ai->ai_next) {
                sockaddr* ai_addr = ai->ai_addr;
                if (ai_addr) {
                    addIpAddrWithinLimit(ip_addrs, ai_addr, ai->ai_addrlen);
                    total_ip_addr_count++;
                }
            }
        }
        freeaddrinfo(result);
    }
    mClient->decRef();
    if (mNetdEventListener != nullptr) {
        switch (mReportingLevel) {
            case INetdEventListener::REPORTING_LEVEL_NONE:
                // Skip reporting.
                break;
            case INetdEventListener::REPORTING_LEVEL_METRICS:
                // Metrics reporting is on. Send metrics.
                mNetdEventListener->onDnsEvent(mNetContext.dns_netid,
                                               INetdEventListener::EVENT_GETADDRINFO, (int32_t) rv,
                                               latencyMs, String16(""), {}, -1, -1);
                break;
            case INetdEventListener::REPORTING_LEVEL_FULL:
                // Full event info reporting is on. Send full info.
                mNetdEventListener->onDnsEvent(mNetContext.dns_netid,
                                               INetdEventListener::EVENT_GETADDRINFO, (int32_t) rv,
                                               latencyMs, String16(mHost), ip_addrs,
                                               total_ip_addr_count, mNetContext.uid);
                break;
        }
    } else {
        ALOGW("Netd event listener is not available; skipping.");
    }
}

void DnsProxyListener::addIpAddrWithinLimit(std::vector<android::String16>& ip_addrs,
        const sockaddr* addr, socklen_t addrlen) {
    // ipAddresses array is limited to first INetdEventListener::DNS_REPORTED_IP_ADDRESSES_LIMIT
    // addresses for A and AAAA. Total count of addresses is provided, to be able to tell whether
    // some addresses didn't get logged.
    if (ip_addrs.size() < INetdEventListener::DNS_REPORTED_IP_ADDRESSES_LIMIT) {
        char ip_addr[INET6_ADDRSTRLEN];
        if (getnameinfo(addr, addrlen, ip_addr, sizeof(ip_addr), nullptr, 0, NI_NUMERICHOST) == 0) {
            ip_addrs.push_back(String16(ip_addr));
        }
    }
}

DnsProxyListener::GetAddrInfoCmd::GetAddrInfoCmd(DnsProxyListener* dnsProxyListener) :
    NetdCommand("getaddrinfo"),
    mDnsProxyListener(dnsProxyListener) {
}

int DnsProxyListener::GetAddrInfoCmd::runCommand(SocketClient *cli,
                                            int argc, char **argv) {
    if (DBG) logArguments(argc, argv);

    if (argc != 8) {
        char* msg = NULL;
        asprintf( &msg, "Invalid number of arguments to getaddrinfo: %i", argc);
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return -1;
    }

    char* name = argv[1];
    if (strcmp("^", name) == 0) {
        name = NULL;
    } else {
        name = strdup(name);
    }

    char* service = argv[2];
    if (strcmp("^", service) == 0) {
        service = NULL;
    } else {
        service = strdup(service);
    }

    struct addrinfo* hints = NULL;
    int ai_flags = atoi(argv[3]);
    int ai_family = atoi(argv[4]);
    int ai_socktype = atoi(argv[5]);
    int ai_protocol = atoi(argv[6]);
    unsigned netId = strtoul(argv[7], NULL, 10);
    const bool useLocalNameservers = checkAndClearUseLocalNameserversFlag(&netId);
    const uid_t uid = cli->getUid();

    android_net_context netcontext;
    mDnsProxyListener->mNetCtrl->getNetworkContext(netId, uid, &netcontext);
    if (useLocalNameservers) {
        netcontext.flags |= NET_CONTEXT_FLAG_USE_LOCAL_NAMESERVERS;
    }

    if (ai_flags != -1 || ai_family != -1 ||
        ai_socktype != -1 || ai_protocol != -1) {
        hints = (struct addrinfo*) calloc(1, sizeof(struct addrinfo));
        hints->ai_flags = ai_flags;
        hints->ai_family = ai_family;
        hints->ai_socktype = ai_socktype;
        hints->ai_protocol = ai_protocol;
    }

    if (DBG) {
        ALOGD("GetAddrInfoHandler for %s / %s / {%u,%u,%u,%u,%u}",
             name ? name : "[nullhost]",
             service ? service : "[nullservice]",
             netcontext.app_netid, netcontext.app_mark,
             netcontext.dns_netid, netcontext.dns_mark,
             netcontext.uid);
    }

    const int metricsLevel = mDnsProxyListener->mEventReporter->getMetricsReportingLevel();

    DnsProxyListener::GetAddrInfoHandler* handler =
            new DnsProxyListener::GetAddrInfoHandler(cli, name, service, hints, netcontext,
                    metricsLevel, mDnsProxyListener->mEventReporter->getNetdEventListener());
    tryThreadOrError(cli, handler);
    return 0;
}

/*******************************************************
 *                  GetHostByName                      *
 *******************************************************/
DnsProxyListener::GetHostByNameCmd::GetHostByNameCmd(DnsProxyListener* dnsProxyListener) :
      NetdCommand("gethostbyname"),
      mDnsProxyListener(dnsProxyListener) {
}

int DnsProxyListener::GetHostByNameCmd::runCommand(SocketClient *cli,
                                            int argc, char **argv) {
    if (DBG) logArguments(argc, argv);

    if (argc != 4) {
        char* msg = NULL;
        asprintf(&msg, "Invalid number of arguments to gethostbyname: %i", argc);
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return -1;
    }

    uid_t uid = cli->getUid();
    unsigned netId = strtoul(argv[1], NULL, 10);
    const bool useLocalNameservers = checkAndClearUseLocalNameserversFlag(&netId);
    char* name = argv[2];
    int af = atoi(argv[3]);

    if (strcmp(name, "^") == 0) {
        name = NULL;
    } else {
        name = strdup(name);
    }

    android_net_context netcontext;
    mDnsProxyListener->mNetCtrl->getNetworkContext(netId, uid, &netcontext);
    if (useLocalNameservers) {
        netcontext.flags |= NET_CONTEXT_FLAG_USE_LOCAL_NAMESERVERS;
    }

    const int metricsLevel = mDnsProxyListener->mEventReporter->getMetricsReportingLevel();

    DnsProxyListener::GetHostByNameHandler* handler =
            new DnsProxyListener::GetHostByNameHandler(cli, name, af, netcontext, metricsLevel,
                    mDnsProxyListener->mEventReporter->getNetdEventListener());
    tryThreadOrError(cli, handler);
    return 0;
}

DnsProxyListener::GetHostByNameHandler::GetHostByNameHandler(SocketClient* c, char* name, int af,
        const android_net_context& netcontext, const int metricsLevel,
        const android::sp<android::net::metrics::INetdEventListener>& netdEventListener)
        : mClient(c),
          mName(name),
          mAf(af),
          mNetContext(netcontext),
          mReportingLevel(metricsLevel),
          mNetdEventListener(netdEventListener) {
}

DnsProxyListener::GetHostByNameHandler::~GetHostByNameHandler() {
    free(mName);
}

void DnsProxyListener::GetHostByNameHandler::run() {
    if (DBG) {
        ALOGD("DnsProxyListener::GetHostByNameHandler::run");
    }

    Stopwatch s;
    maybeFixupNetContext(&mNetContext);
    const uid_t uid = mClient->getUid();
    struct hostent* hp = nullptr;
    if (queryLimiter.start(uid)) {
        hp = android_gethostbynamefornetcontext(mName, mAf, &mNetContext);
        queryLimiter.finish(uid);
    } else {
        ALOGE("gethostbyname: from UID %d, max concurrent queries reached", uid);
    }
    const int latencyMs = lround(s.timeTaken());

    if (DBG) {
        ALOGD("GetHostByNameHandler::run gethostbyname errno: %s hp->h_name = %s, name_len = %zu",
                hp ? "success" : strerror(errno),
                (hp && hp->h_name) ? hp->h_name : "null",
                (hp && hp->h_name) ? strlen(hp->h_name) + 1 : 0);
    }

    bool success = true;
    if (hp) {
        success = mClient->sendCode(ResponseCode::DnsProxyQueryResult) == 0;
        success &= sendhostent(mClient, hp);
    } else {
        success = mClient->sendBinaryMsg(ResponseCode::DnsProxyOperationFailed, NULL, 0) == 0;
    }

    if (!success) {
        ALOGW("GetHostByNameHandler: Error writing DNS result to client");
    }

    if (mNetdEventListener != nullptr) {
        std::vector<String16> ip_addrs;
        int total_ip_addr_count = 0;
        if (mReportingLevel == INetdEventListener::REPORTING_LEVEL_FULL) {
            if (hp != nullptr && hp->h_addrtype == AF_INET) {
                in_addr** list = (in_addr**) hp->h_addr_list;
                for (int i = 0; list[i] != NULL; i++) {
                    sockaddr_in sin = { .sin_family = AF_INET, .sin_addr = *list[i] };
                    addIpAddrWithinLimit(ip_addrs, (sockaddr*) &sin, sizeof(sin));
                    total_ip_addr_count++;
                }
            } else if (hp != nullptr && hp->h_addrtype == AF_INET6) {
                in6_addr** list = (in6_addr**) hp->h_addr_list;
                for (int i = 0; list[i] != NULL; i++) {
                    sockaddr_in6 sin6 = { .sin6_family = AF_INET6, .sin6_addr = *list[i] };
                    addIpAddrWithinLimit(ip_addrs, (sockaddr*) &sin6, sizeof(sin6));
                    total_ip_addr_count++;
                }
            }
        }
        switch (mReportingLevel) {
            case INetdEventListener::REPORTING_LEVEL_NONE:
                // Reporting is off.
                break;
            case INetdEventListener::REPORTING_LEVEL_METRICS:
                // Metrics reporting is on. Send metrics.
                mNetdEventListener->onDnsEvent(mNetContext.dns_netid,
                                               INetdEventListener::EVENT_GETHOSTBYNAME,
                                               h_errno, latencyMs, String16(""), {}, -1, -1);
                break;
            case INetdEventListener::REPORTING_LEVEL_FULL:
                // Full event info reporting is on. Send full info.
                mNetdEventListener->onDnsEvent(mNetContext.dns_netid,
                                               INetdEventListener::EVENT_GETHOSTBYNAME,
                                               h_errno, latencyMs, String16(mName), ip_addrs,
                                               total_ip_addr_count, uid);
                break;
        }
    }

    mClient->decRef();
}


/*******************************************************
 *                  GetHostByAddr                      *
 *******************************************************/
DnsProxyListener::GetHostByAddrCmd::GetHostByAddrCmd(const DnsProxyListener* dnsProxyListener) :
        NetdCommand("gethostbyaddr"),
        mDnsProxyListener(dnsProxyListener) {
}

int DnsProxyListener::GetHostByAddrCmd::runCommand(SocketClient *cli,
                                            int argc, char **argv) {
    if (DBG) logArguments(argc, argv);

    if (argc != 5) {
        char* msg = NULL;
        asprintf(&msg, "Invalid number of arguments to gethostbyaddr: %i", argc);
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return -1;
    }

    char* addrStr = argv[1];
    int addrLen = atoi(argv[2]);
    int addrFamily = atoi(argv[3]);
    uid_t uid = cli->getUid();
    unsigned netId = strtoul(argv[4], NULL, 10);
    const bool useLocalNameservers = checkAndClearUseLocalNameserversFlag(&netId);

    void* addr = malloc(sizeof(struct in6_addr));
    errno = 0;
    int result = inet_pton(addrFamily, addrStr, addr);
    if (result <= 0) {
        char* msg = NULL;
        asprintf(&msg, "inet_pton(\"%s\") failed %s", addrStr, strerror(errno));
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::OperationFailed, msg, false);
        free(addr);
        free(msg);
        return -1;
    }

    android_net_context netcontext;
    mDnsProxyListener->mNetCtrl->getNetworkContext(netId, uid, &netcontext);
    if (useLocalNameservers) {
        netcontext.flags |= NET_CONTEXT_FLAG_USE_LOCAL_NAMESERVERS;
    }

    DnsProxyListener::GetHostByAddrHandler* handler =
            new DnsProxyListener::GetHostByAddrHandler(cli, addr, addrLen, addrFamily, netcontext);
    tryThreadOrError(cli, handler);
    return 0;
}

DnsProxyListener::GetHostByAddrHandler::GetHostByAddrHandler(
          SocketClient* c,
          void* address,
          int addressLen,
          int addressFamily,
          const android_net_context& netcontext)
        : mClient(c),
          mAddress(address),
          mAddressLen(addressLen),
          mAddressFamily(addressFamily),
          mNetContext(netcontext) {
}

DnsProxyListener::GetHostByAddrHandler::~GetHostByAddrHandler() {
    free(mAddress);
}

void DnsProxyListener::GetHostByAddrHandler::run() {
    if (DBG) {
        ALOGD("DnsProxyListener::GetHostByAddrHandler::run");
    }

    maybeFixupNetContext(&mNetContext);
    const uid_t uid = mClient->getUid();
    struct hostent* hp = nullptr;
    if (queryLimiter.start(uid)) {
        hp = android_gethostbyaddrfornetcontext(
                mAddress, mAddressLen, mAddressFamily, &mNetContext);
        queryLimiter.finish(uid);
    } else {
        ALOGE("gethostbyaddr: from UID %d, max concurrent queries reached", uid);
    }

    if (DBG) {
        ALOGD("GetHostByAddrHandler::run gethostbyaddr errno: %s hp->h_name = %s, name_len = %zu",
                hp ? "success" : strerror(errno),
                (hp && hp->h_name) ? hp->h_name : "null",
                (hp && hp->h_name) ? strlen(hp->h_name) + 1 : 0);
    }

    bool success = true;
    if (hp) {
        success = mClient->sendCode(ResponseCode::DnsProxyQueryResult) == 0;
        success &= sendhostent(mClient, hp);
    } else {
        success = mClient->sendBinaryMsg(ResponseCode::DnsProxyOperationFailed, NULL, 0) == 0;
    }

    if (!success) {
        ALOGW("GetHostByAddrHandler: Error writing DNS result to client");
    }
    mClient->decRef();
}

}  // namespace net
}  // namespace android
