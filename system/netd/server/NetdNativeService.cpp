/**
 * Copyright (c) 2016, The Android Open Source Project
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

#define LOG_TAG "Netd"

#include <set>
#include <vector>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <utils/Errors.h>
#include <utils/String16.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "android/net/BnNetd.h"

#include <openssl/base64.h>

#include "Controllers.h"
#include "DumpWriter.h"
#include "EventReporter.h"
#include "InterfaceController.h"
#include "NetdConstants.h"
#include "NetdNativeService.h"
#include "RouteController.h"
#include "SockDiag.h"
#include "UidRanges.h"

using android::base::StringPrintf;
using android::os::PersistableBundle;

namespace android {
namespace net {

namespace {

const char CONNECTIVITY_INTERNAL[] = "android.permission.CONNECTIVITY_INTERNAL";
const char NETWORK_STACK[] = "android.permission.NETWORK_STACK";
const char DUMP[] = "android.permission.DUMP";

binder::Status toBinderStatus(const netdutils::Status s) {
    if (isOk(s)) {
        return binder::Status::ok();
    }
    return binder::Status::fromServiceSpecificError(s.code(), s.msg().c_str());
}

binder::Status checkPermission(const char *permission) {
    pid_t pid;
    uid_t uid;

    if (checkCallingPermission(String16(permission), (int32_t *) &pid, (int32_t *) &uid)) {
        return binder::Status::ok();
    } else {
        auto err = StringPrintf("UID %d / PID %d lacks permission %s", uid, pid, permission);
        return binder::Status::fromExceptionCode(binder::Status::EX_SECURITY, String8(err.c_str()));
    }
}

#define ENFORCE_DEBUGGABLE() {                              \
    char value[PROPERTY_VALUE_MAX + 1];                     \
    if (property_get("ro.debuggable", value, NULL) != 1     \
            || value[0] != '1') {                           \
        return binder::Status::fromExceptionCode(           \
            binder::Status::EX_SECURITY,                    \
            String8("Not available in production builds.")  \
        );                                                  \
    }                                                       \
}

#define ENFORCE_PERMISSION(permission) {                    \
    binder::Status status = checkPermission((permission));  \
    if (!status.isOk()) {                                   \
        return status;                                      \
    }                                                       \
}

#define NETD_LOCKING_RPC(permission, lock)                  \
    ENFORCE_PERMISSION(permission);                         \
    android::RWLock::AutoWLock _lock(lock);

#define NETD_BIG_LOCK_RPC(permission) NETD_LOCKING_RPC((permission), gBigNetdLock)

inline binder::Status statusFromErrcode(int ret) {
    if (ret) {
        return binder::Status::fromServiceSpecificError(-ret, strerror(-ret));
    }
    return binder::Status::ok();
}

}  // namespace


status_t NetdNativeService::start() {
    IPCThreadState::self()->disableBackgroundScheduling(true);
    status_t ret = BinderService<NetdNativeService>::publish();
    if (ret != android::OK) {
        return ret;
    }
    sp<ProcessState> ps(ProcessState::self());
    ps->startThreadPool();
    ps->giveThreadPoolName();
    return android::OK;
}

status_t NetdNativeService::dump(int fd, const Vector<String16> &args) {
    const binder::Status dump_permission = checkPermission(DUMP);
    if (!dump_permission.isOk()) {
        const String8 msg(dump_permission.toString8());
        write(fd, msg.string(), msg.size());
        return PERMISSION_DENIED;
    }

    // This method does not grab any locks. If individual classes need locking
    // their dump() methods MUST handle locking appropriately.

    DumpWriter dw(fd);

    if (!args.isEmpty() && args[0] == TcpSocketMonitor::DUMP_KEYWORD) {
      dw.blankline();
      gCtls->tcpSocketMonitor.dump(dw);
      dw.blankline();
      return NO_ERROR;
    }

    if (!args.isEmpty() && args[0] == TrafficController::DUMP_KEYWORD) {
        dw.blankline();
        gCtls->trafficCtrl.dump(dw, true);
        dw.blankline();
        return NO_ERROR;
    }

    dw.blankline();
    gCtls->netCtrl.dump(dw);
    dw.blankline();

    gCtls->trafficCtrl.dump(dw, false);
    dw.blankline();

    return NO_ERROR;
}

binder::Status NetdNativeService::isAlive(bool *alive) {
    NETD_BIG_LOCK_RPC(CONNECTIVITY_INTERNAL);

    *alive = true;
    return binder::Status::ok();
}

binder::Status NetdNativeService::firewallReplaceUidChain(const android::String16& chainName,
        bool isWhitelist, const std::vector<int32_t>& uids, bool *ret) {
    NETD_LOCKING_RPC(CONNECTIVITY_INTERNAL, gCtls->firewallCtrl.lock);

    android::String8 name = android::String8(chainName);
    int err = gCtls->firewallCtrl.replaceUidChain(name.string(), isWhitelist, uids);
    *ret = (err == 0);
    return binder::Status::ok();
}

binder::Status NetdNativeService::bandwidthEnableDataSaver(bool enable, bool *ret) {
    NETD_LOCKING_RPC(CONNECTIVITY_INTERNAL, gCtls->bandwidthCtrl.lock);

    int err = gCtls->bandwidthCtrl.enableDataSaver(enable);
    *ret = (err == 0);
    return binder::Status::ok();
}

binder::Status NetdNativeService::networkCreatePhysical(int32_t netId,
        const std::string& permission) {
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    int ret = gCtls->netCtrl.createPhysicalNetwork(netId, stringToPermission(permission.c_str()));
    return statusFromErrcode(ret);
}

binder::Status NetdNativeService::networkCreateVpn(int32_t netId, bool hasDns, bool secure) {
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    int ret = gCtls->netCtrl.createVirtualNetwork(netId, hasDns, secure);
    return statusFromErrcode(ret);
}

binder::Status NetdNativeService::networkDestroy(int32_t netId) {
    ENFORCE_PERMISSION(NETWORK_STACK);
    // Both of these functions manage their own locking internally.
    const int ret = gCtls->netCtrl.destroyNetwork(netId);
    gCtls->resolverCtrl.clearDnsServers(netId);
    return statusFromErrcode(ret);
}

binder::Status NetdNativeService::networkAddInterface(int32_t netId, const std::string& iface) {
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    int ret = gCtls->netCtrl.addInterfaceToNetwork(netId, iface.c_str());
    return statusFromErrcode(ret);
}

binder::Status NetdNativeService::networkRemoveInterface(int32_t netId, const std::string& iface) {
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    int ret = gCtls->netCtrl.removeInterfaceFromNetwork(netId, iface.c_str());
    return statusFromErrcode(ret);
}

binder::Status NetdNativeService::networkAddUidRanges(int32_t netId,
        const std::vector<UidRange>& uidRangeArray) {
    // NetworkController::addUsersToNetwork is thread-safe.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    int ret = gCtls->netCtrl.addUsersToNetwork(netId, UidRanges(uidRangeArray));
    return statusFromErrcode(ret);
}

binder::Status NetdNativeService::networkRemoveUidRanges(int32_t netId,
        const std::vector<UidRange>& uidRangeArray) {
    // NetworkController::removeUsersFromNetwork is thread-safe.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    int ret = gCtls->netCtrl.removeUsersFromNetwork(netId, UidRanges(uidRangeArray));
    return statusFromErrcode(ret);
}

binder::Status NetdNativeService::networkRejectNonSecureVpn(bool add,
        const std::vector<UidRange>& uidRangeArray) {
    // TODO: elsewhere RouteController is only used from the tethering and network controllers, so
    // it should be possible to use the same lock as NetworkController. However, every call through
    // the CommandListener "network" command will need to hold this lock too, not just the ones that
    // read/modify network internal state (that is sufficient for ::dump() because it doesn't
    // look at routes, but it's not enough here).
    NETD_BIG_LOCK_RPC(CONNECTIVITY_INTERNAL);

    UidRanges uidRanges(uidRangeArray);

    int err;
    if (add) {
        err = RouteController::addUsersToRejectNonSecureNetworkRule(uidRanges);
    } else {
        err = RouteController::removeUsersFromRejectNonSecureNetworkRule(uidRanges);
    }

    return statusFromErrcode(err);
}

binder::Status NetdNativeService::socketDestroy(const std::vector<UidRange>& uids,
        const std::vector<int32_t>& skipUids) {

    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    SockDiag sd;
    if (!sd.open()) {
        return binder::Status::fromServiceSpecificError(EIO,
                String8("Could not open SOCK_DIAG socket"));
    }

    UidRanges uidRanges(uids);
    int err = sd.destroySockets(uidRanges, std::set<uid_t>(skipUids.begin(), skipUids.end()),
                                true /* excludeLoopback */);

    if (err) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("destroySockets: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

// Parse a base64 encoded string into a vector of bytes.
// On failure, return an empty vector.
static std::vector<uint8_t> parseBase64(const std::string& input) {
    std::vector<uint8_t> decoded;
    size_t out_len;
    if (EVP_DecodedLength(&out_len, input.size()) != 1) {
        return decoded;
    }
    // out_len is now an upper bound on the output length.
    decoded.resize(out_len);
    if (EVP_DecodeBase64(decoded.data(), &out_len, decoded.size(),
            reinterpret_cast<const uint8_t*>(input.data()), input.size()) == 1) {
        // Possibly shrink the vector if the actual output was smaller than the bound.
        decoded.resize(out_len);
    } else {
        decoded.clear();
    }
    if (out_len != SHA256_SIZE) {
        decoded.clear();
    }
    return decoded;
}

binder::Status NetdNativeService::setResolverConfiguration(int32_t netId,
        const std::vector<std::string>& servers, const std::vector<std::string>& domains,
        const std::vector<int32_t>& params, const std::string& tlsName,
        const std::vector<std::string>& tlsServers,
        const std::vector<std::string>& tlsFingerprints) {
    // This function intentionally does not lock within Netd, as Bionic is thread-safe.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    std::set<std::vector<uint8_t>> decoded_fingerprints;
    for (const std::string& fingerprint : tlsFingerprints) {
        std::vector<uint8_t> decoded = parseBase64(fingerprint);
        if (decoded.empty()) {
            return binder::Status::fromServiceSpecificError(EINVAL,
                    String8::format("ResolverController error: bad fingerprint"));
        }
        decoded_fingerprints.emplace(decoded);
    }

    int err = gCtls->resolverCtrl.setResolverConfiguration(netId, servers, domains, params,
            tlsName, tlsServers, decoded_fingerprints);
    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("ResolverController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::getResolverInfo(int32_t netId,
        std::vector<std::string>* servers, std::vector<std::string>* domains,
        std::vector<int32_t>* params, std::vector<int32_t>* stats) {
    // This function intentionally does not lock within Netd, as Bionic is thread-safe.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    int err = gCtls->resolverCtrl.getResolverInfo(netId, servers, domains, params, stats);
    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("ResolverController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::tetherApplyDnsInterfaces(bool *ret) {
    NETD_LOCKING_RPC(NETWORK_STACK, gCtls->tetherCtrl.lock)

    *ret = gCtls->tetherCtrl.applyDnsInterfaces();
    return binder::Status::ok();
}

namespace {

void tetherAddStats(PersistableBundle *bundle, const TetherController::TetherStats& stats) {
    String16 iface = String16(stats.extIface.c_str());
    std::vector<int64_t> statsVector(INetd::TETHER_STATS_ARRAY_SIZE);

    bundle->getLongVector(iface, &statsVector);
    if (statsVector.size() == 0) {
        for (int i = 0; i < INetd::TETHER_STATS_ARRAY_SIZE; i++) statsVector.push_back(0);
    }

    statsVector[INetd::TETHER_STATS_RX_BYTES]   += stats.rxBytes;
    statsVector[INetd::TETHER_STATS_RX_PACKETS] += stats.rxPackets;
    statsVector[INetd::TETHER_STATS_TX_BYTES]   += stats.txBytes;
    statsVector[INetd::TETHER_STATS_TX_PACKETS] += stats.txPackets;

    bundle->putLongVector(iface, statsVector);
}

}  // namespace

binder::Status NetdNativeService::tetherGetStats(PersistableBundle *bundle) {
    NETD_LOCKING_RPC(NETWORK_STACK, gCtls->tetherCtrl.lock)

    const auto& statsList = gCtls->tetherCtrl.getTetherStats();
    if (!isOk(statsList)) {
        return toBinderStatus(statsList);
    }

    for (const auto& stats : statsList.value()) {
        tetherAddStats(bundle, stats);
    }

    return binder::Status::ok();
}

binder::Status NetdNativeService::interfaceAddAddress(const std::string &ifName,
        const std::string &addrString, int prefixLength) {
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    const int err = InterfaceController::addAddress(
            ifName.c_str(), addrString.c_str(), prefixLength);
    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("InterfaceController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::interfaceDelAddress(const std::string &ifName,
        const std::string &addrString, int prefixLength) {
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    const int err = InterfaceController::delAddress(
            ifName.c_str(), addrString.c_str(), prefixLength);
    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("InterfaceController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::setProcSysNet(
        int32_t family, int32_t which, const std::string &ifname, const std::string &parameter,
        const std::string &value) {
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    const char *familyStr;
    switch (family) {
        case INetd::IPV4:
            familyStr = "ipv4";
            break;
        case INetd::IPV6:
            familyStr = "ipv6";
            break;
        default:
            return binder::Status::fromServiceSpecificError(EAFNOSUPPORT, String8("Bad family"));
    }

    const char *whichStr;
    switch (which) {
        case INetd::CONF:
            whichStr = "conf";
            break;
        case INetd::NEIGH:
            whichStr = "neigh";
            break;
        default:
            return binder::Status::fromServiceSpecificError(EINVAL, String8("Bad category"));
    }

    const int err = InterfaceController::setParameter(
            familyStr, whichStr, ifname.c_str(), parameter.c_str(),
            value.c_str());
    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("ResolverController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::getMetricsReportingLevel(int *reportingLevel) {
    // This function intentionally does not lock, since the only thing it does is one read from an
    // atomic_int.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    ENFORCE_DEBUGGABLE();

    *reportingLevel = gCtls->eventReporter.getMetricsReportingLevel();
    return binder::Status::ok();
}

binder::Status NetdNativeService::setMetricsReportingLevel(const int reportingLevel) {
    // This function intentionally does not lock, since the only thing it does is one write to an
    // atomic_int.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    ENFORCE_DEBUGGABLE();

    return (gCtls->eventReporter.setMetricsReportingLevel(reportingLevel) == 0)
            ? binder::Status::ok()
            : binder::Status::fromExceptionCode(binder::Status::EX_ILLEGAL_ARGUMENT);
}

binder::Status NetdNativeService::ipSecSetEncapSocketOwner(const android::base::unique_fd& socket,
                                                      int newUid) {
    ENFORCE_PERMISSION(NETWORK_STACK)
    ALOGD("ipSecSetEncapSocketOwner()");

    uid_t callerUid = IPCThreadState::self()->getCallingUid();
    return asBinderStatus(gCtls->xfrmCtrl.ipSecSetEncapSocketOwner(socket, newUid, callerUid));
}

binder::Status NetdNativeService::ipSecAllocateSpi(
        int32_t transformId,
        const std::string& sourceAddress,
        const std::string& destinationAddress,
        int32_t inSpi,
        int32_t* outSpi) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    ALOGD("ipSecAllocateSpi()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecAllocateSpi(
                    transformId,
                    sourceAddress,
                    destinationAddress,
                    inSpi,
                    outSpi));
}

binder::Status NetdNativeService::ipSecAddSecurityAssociation(
        int32_t transformId,
        int32_t mode,
        const std::string& sourceAddress,
        const std::string& destinationAddress,
        int32_t underlyingNetId,
        int32_t spi,
        int32_t markValue,
        int32_t markMask,
        const std::string& authAlgo, const std::vector<uint8_t>& authKey, int32_t authTruncBits,
        const std::string& cryptAlgo, const std::vector<uint8_t>& cryptKey, int32_t cryptTruncBits,
        const std::string& aeadAlgo, const std::vector<uint8_t>& aeadKey, int32_t aeadIcvBits,
        int32_t encapType,
        int32_t encapLocalPort,
        int32_t encapRemotePort) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    ALOGD("ipSecAddSecurityAssociation()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecAddSecurityAssociation(
              transformId, mode, sourceAddress, destinationAddress,
              underlyingNetId,
              spi, markValue, markMask,
              authAlgo, authKey, authTruncBits,
              cryptAlgo, cryptKey, cryptTruncBits,
              aeadAlgo, aeadKey, aeadIcvBits,
              encapType, encapLocalPort, encapRemotePort));
}

binder::Status NetdNativeService::ipSecDeleteSecurityAssociation(
        int32_t transformId,
        const std::string& sourceAddress,
        const std::string& destinationAddress,
        int32_t spi,
        int32_t markValue,
        int32_t markMask) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    ALOGD("ipSecDeleteSecurityAssociation()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecDeleteSecurityAssociation(
                    transformId,
                    sourceAddress,
                    destinationAddress,
                    spi,
                    markValue,
                    markMask));
}

binder::Status NetdNativeService::ipSecApplyTransportModeTransform(
        const android::base::unique_fd& socket,
        int32_t transformId,
        int32_t direction,
        const std::string& sourceAddress,
        const std::string& destinationAddress,
        int32_t spi) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    ALOGD("ipSecApplyTransportModeTransform()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecApplyTransportModeTransform(
                    socket,
                    transformId,
                    direction,
                    sourceAddress,
                    destinationAddress,
                    spi));
}

binder::Status NetdNativeService::ipSecRemoveTransportModeTransform(
            const android::base::unique_fd& socket) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);
    ALOGD("ipSecRemoveTransportModeTransform()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecRemoveTransportModeTransform(
                    socket));
}

binder::Status NetdNativeService::ipSecAddSecurityPolicy(
        int32_t transformId,
        int32_t direction,
        const std::string& sourceAddress,
        const std::string& destinationAddress,
        int32_t spi,
        int32_t markValue,
        int32_t markMask){
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(NETWORK_STACK);
    ALOGD("ipSecAddSecurityPolicy()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecAddSecurityPolicy(
                    transformId,
                    direction,
                    sourceAddress,
                    destinationAddress,
                    spi,
                    markValue,
                    markMask));
}

binder::Status NetdNativeService::ipSecUpdateSecurityPolicy(
        int32_t transformId,
        int32_t direction,
        const std::string& sourceAddress,
        const std::string& destinationAddress,
        int32_t spi,
        int32_t markValue,
        int32_t markMask){
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(NETWORK_STACK);
    ALOGD("ipSecAddSecurityPolicy()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecUpdateSecurityPolicy(
                    transformId,
                    direction,
                    sourceAddress,
                    destinationAddress,
                    spi,
                    markValue,
                    markMask));
}

binder::Status NetdNativeService::ipSecDeleteSecurityPolicy(
        int32_t transformId,
        int32_t direction,
        const std::string& sourceAddress,
        const std::string& destinationAddress,
        int32_t markValue,
        int32_t markMask){
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(NETWORK_STACK);
    ALOGD("ipSecAddSecurityPolicy()");
    return asBinderStatus(gCtls->xfrmCtrl.ipSecDeleteSecurityPolicy(
                    transformId,
                    direction,
                    sourceAddress,
                    destinationAddress,
                    markValue,
                    markMask));
}

binder::Status NetdNativeService::addVirtualTunnelInterface(
        const std::string& deviceName,
        const std::string& localAddress,
        const std::string& remoteAddress,
        int32_t iKey,
        int32_t oKey) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(NETWORK_STACK);
    ALOGD("addVirtualTunnelInterface()");
    int ret = gCtls->xfrmCtrl.addVirtualTunnelInterface(
                             deviceName,
                             localAddress,
                             remoteAddress,
                             iKey,
                             oKey,
                             false);

    return (ret == 0) ? binder::Status::ok() :
                        asBinderStatus(netdutils::statusFromErrno(
                                       ret, "Error in creating virtual tunnel interface."));
}

binder::Status NetdNativeService::updateVirtualTunnelInterface(
        const std::string& deviceName,
        const std::string& localAddress,
        const std::string& remoteAddress,
        int32_t iKey,
        int32_t oKey) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(NETWORK_STACK);
    ALOGD("updateVirtualTunnelInterface()");
    int ret = gCtls->xfrmCtrl.addVirtualTunnelInterface(
                             deviceName,
                             localAddress,
                             remoteAddress,
                             iKey,
                             oKey,
                             true);

    return (ret == 0) ? binder::Status::ok() :
                        asBinderStatus(netdutils::statusFromErrno(
                                       ret, "Error in updating virtual tunnel interface."));
}

binder::Status NetdNativeService::removeVirtualTunnelInterface(const std::string& deviceName) {
    // Necessary locking done in IpSecService and kernel
    ENFORCE_PERMISSION(NETWORK_STACK);
    ALOGD("removeVirtualTunnelInterface()");
    int ret = gCtls->xfrmCtrl.removeVirtualTunnelInterface(deviceName);

    return (ret == 0) ? binder::Status::ok() :
                        asBinderStatus(netdutils::statusFromErrno(
                                       ret, "Error in removing virtual tunnel interface."));
}

binder::Status NetdNativeService::setIPv6AddrGenMode(const std::string& ifName,
                                                     int32_t mode) {
    ENFORCE_PERMISSION(NETWORK_STACK);
    return toBinderStatus(InterfaceController::setIPv6AddrGenMode(ifName, mode));
}

binder::Status NetdNativeService::wakeupAddInterface(const std::string& ifName,
                                                     const std::string& prefix, int32_t mark,
                                                     int32_t mask) {
    ENFORCE_PERMISSION(NETWORK_STACK);
    return toBinderStatus(gCtls->wakeupCtrl.addInterface(ifName, prefix, mark, mask));
}

binder::Status NetdNativeService::wakeupDelInterface(const std::string& ifName,
                                                     const std::string& prefix, int32_t mark,
                                                     int32_t mask) {
    ENFORCE_PERMISSION(NETWORK_STACK);
    return toBinderStatus(gCtls->wakeupCtrl.delInterface(ifName, prefix, mark, mask));
}

binder::Status NetdNativeService::trafficCheckBpfStatsEnable(bool* ret) {
    ENFORCE_PERMISSION(NETWORK_STACK);
    *ret = gCtls->trafficCtrl.checkBpfStatsEnable();
    return binder::Status::ok();
}

}  // namespace net
}  // namespace android
