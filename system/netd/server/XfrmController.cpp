/*
 *
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

#include <random>
#include <string>
#include <vector>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/in.h>
#include <linux/ipsec.h>
#include <linux/netlink.h>
#include <linux/xfrm.h>

#define LOG_TAG "XfrmController"
#include "InterfaceController.h"
#include "NetdConstants.h"
#include "NetlinkCommands.h"
#include "ResponseCode.h"
#include "XfrmController.h"
#include "netdutils/Fd.h"
#include "netdutils/Slice.h"
#include "netdutils/Syscalls.h"
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <android/net/INetd.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <logwrap/logwrap.h>

using android::net::INetd;
using android::netdutils::Fd;
using android::netdutils::Slice;
using android::netdutils::Status;
using android::netdutils::StatusOr;
using android::netdutils::Syscalls;

namespace android {
namespace net {

// Exposed for testing
constexpr uint32_t ALGO_MASK_AUTH_ALL = ~0;
// Exposed for testing
constexpr uint32_t ALGO_MASK_CRYPT_ALL = ~0;
// Exposed for testing
constexpr uint32_t ALGO_MASK_AEAD_ALL = ~0;
// Exposed for testing
constexpr uint8_t REPLAY_WINDOW_SIZE = 4;

namespace {

constexpr uint32_t RAND_SPI_MIN = 256;
constexpr uint32_t RAND_SPI_MAX = 0xFFFFFFFE;

constexpr uint32_t INVALID_SPI = 0;

static inline bool isEngBuild() {
    static const std::string sBuildType = android::base::GetProperty("ro.build.type", "user");
    return sBuildType == "eng";
}

#define XFRM_MSG_TRANS(x)                                                                          \
    case x:                                                                                        \
        return #x;

const char* xfrmMsgTypeToString(uint16_t msg) {
    switch (msg) {
        XFRM_MSG_TRANS(XFRM_MSG_NEWSA)
        XFRM_MSG_TRANS(XFRM_MSG_DELSA)
        XFRM_MSG_TRANS(XFRM_MSG_GETSA)
        XFRM_MSG_TRANS(XFRM_MSG_NEWPOLICY)
        XFRM_MSG_TRANS(XFRM_MSG_DELPOLICY)
        XFRM_MSG_TRANS(XFRM_MSG_GETPOLICY)
        XFRM_MSG_TRANS(XFRM_MSG_ALLOCSPI)
        XFRM_MSG_TRANS(XFRM_MSG_ACQUIRE)
        XFRM_MSG_TRANS(XFRM_MSG_EXPIRE)
        XFRM_MSG_TRANS(XFRM_MSG_UPDPOLICY)
        XFRM_MSG_TRANS(XFRM_MSG_UPDSA)
        XFRM_MSG_TRANS(XFRM_MSG_POLEXPIRE)
        XFRM_MSG_TRANS(XFRM_MSG_FLUSHSA)
        XFRM_MSG_TRANS(XFRM_MSG_FLUSHPOLICY)
        XFRM_MSG_TRANS(XFRM_MSG_NEWAE)
        XFRM_MSG_TRANS(XFRM_MSG_GETAE)
        XFRM_MSG_TRANS(XFRM_MSG_REPORT)
        XFRM_MSG_TRANS(XFRM_MSG_MIGRATE)
        XFRM_MSG_TRANS(XFRM_MSG_NEWSADINFO)
        XFRM_MSG_TRANS(XFRM_MSG_GETSADINFO)
        XFRM_MSG_TRANS(XFRM_MSG_GETSPDINFO)
        XFRM_MSG_TRANS(XFRM_MSG_NEWSPDINFO)
        XFRM_MSG_TRANS(XFRM_MSG_MAPPING)
        default:
            return "XFRM_MSG UNKNOWN";
    }
}

// actually const but cannot be declared as such for reasons
uint8_t kPadBytesArray[] = {0, 0, 0};
void* kPadBytes = static_cast<void*>(kPadBytesArray);

#define LOG_HEX(__desc16__, __buf__, __len__)                                                      \
    do {                                                                                           \
        if (isEngBuild()) {                                                                        \
            logHex(__desc16__, __buf__, __len__);                                                  \
        }                                                                                          \
    } while (0)

#define LOG_IOV(__iov__)                                                                           \
    do {                                                                                           \
        if (isEngBuild()) {                                                                        \
            logIov(__iov__);                                                                       \
        }                                                                                          \
    } while (0)

void logHex(const char* desc16, const char* buf, size_t len) {
    char* printBuf = new char[len * 2 + 1 + 26]; // len->ascii, +newline, +prefix strlen
    int offset = 0;
    if (desc16) {
        sprintf(printBuf, "{%-16s}", desc16);
        offset += 18; // prefix string length
    }
    sprintf(printBuf + offset, "[%4.4u]: ", (len > 9999) ? 9999 : (unsigned)len);
    offset += 8;

    for (uint32_t j = 0; j < (uint32_t)len; j++) {
        sprintf(&printBuf[j * 2 + offset], "%0.2x", (unsigned char)buf[j]);
    }
    ALOGD("%s", printBuf);
    delete[] printBuf;
}

void logIov(const std::vector<iovec>& iov) {
    for (const iovec& row : iov) {
        logHex(0, reinterpret_cast<char*>(row.iov_base), row.iov_len);
    }
}

size_t fillNlAttr(__u16 nlaType, size_t valueSize, nlattr* nlAttr) {
    size_t dataLen = valueSize;
    int padLength = NLMSG_ALIGN(dataLen) - dataLen;
    nlAttr->nla_len = (__u16)(dataLen + sizeof(nlattr));
    nlAttr->nla_type = nlaType;
    return padLength;
}

size_t fillNlAttrIpAddress(__u16 nlaType, int family, const std::string& value, nlattr* nlAttr,
                           Slice ipAddress) {
    inet_pton(family, value.c_str(), ipAddress.base());
    return fillNlAttr(nlaType, (family == AF_INET) ? sizeof(in_addr) : sizeof(in6_addr), nlAttr);
}

size_t fillNlAttrU32(__u16 nlaType, int32_t value, nlattr* nlAttr, uint32_t* u32Value) {
    *u32Value = htonl(value);
    return fillNlAttr(nlaType, sizeof((*u32Value)), nlAttr);
}

// returns the address family, placing the string in the provided buffer
StatusOr<uint16_t> convertStringAddress(std::string addr, uint8_t* buffer) {
    if (inet_pton(AF_INET, addr.c_str(), buffer) == 1) {
        return AF_INET;
    } else if (inet_pton(AF_INET6, addr.c_str(), buffer) == 1) {
        return AF_INET6;
    } else {
        return Status(EAFNOSUPPORT);
    }
}

// TODO: Need to consider a way to refer to the sSycalls instance
inline Syscalls& getSyscallInstance() { return netdutils::sSyscalls.get(); }

class XfrmSocketImpl : public XfrmSocket {
private:
    static constexpr int NLMSG_DEFAULTSIZE = 8192;

    union NetlinkResponse {
        nlmsghdr hdr;
        struct _err_ {
            nlmsghdr hdr;
            nlmsgerr err;
        } err;

        struct _buf_ {
            nlmsghdr hdr;
            char buf[NLMSG_DEFAULTSIZE];
        } buf;
    };

public:
    netdutils::Status open() override {
        mSock = openNetlinkSocket(NETLINK_XFRM);
        if (mSock < 0) {
            ALOGW("Could not get a new socket, line=%d", __LINE__);
            return netdutils::statusFromErrno(-mSock, "Could not open netlink socket");
        }

        return netdutils::status::ok;
    }

    static netdutils::Status validateResponse(NetlinkResponse response, size_t len) {
        if (len < sizeof(nlmsghdr)) {
            ALOGW("Invalid response message received over netlink");
            return netdutils::statusFromErrno(EBADMSG, "Invalid message");
        }

        switch (response.hdr.nlmsg_type) {
            case NLMSG_NOOP:
            case NLMSG_DONE:
                return netdutils::status::ok;
            case NLMSG_OVERRUN:
                ALOGD("Netlink request overran kernel buffer");
                return netdutils::statusFromErrno(EBADMSG, "Kernel buffer overrun");
            case NLMSG_ERROR:
                if (len < sizeof(NetlinkResponse::_err_)) {
                    ALOGD("Netlink message received malformed error response");
                    return netdutils::statusFromErrno(EBADMSG, "Malformed error response");
                }
                return netdutils::statusFromErrno(
                    -response.err.err.error,
                    "Error netlink message"); // Netlink errors are negative errno.
            case XFRM_MSG_NEWSA:
                break;
        }

        if (response.hdr.nlmsg_type < XFRM_MSG_BASE /*== NLMSG_MIN_TYPE*/ ||
            response.hdr.nlmsg_type > XFRM_MSG_MAX) {
            ALOGD("Netlink message responded with an out-of-range message ID");
            return netdutils::statusFromErrno(EBADMSG, "Invalid message ID");
        }

        // TODO Add more message validation here
        return netdutils::status::ok;
    }

    netdutils::Status sendMessage(uint16_t nlMsgType, uint16_t nlMsgFlags, uint16_t nlMsgSeqNum,
                                  std::vector<iovec>* iovecs) const override {
        nlmsghdr nlMsg = {
            .nlmsg_type = nlMsgType,
            .nlmsg_flags = nlMsgFlags,
            .nlmsg_seq = nlMsgSeqNum,
        };

        (*iovecs)[0].iov_base = &nlMsg;
        (*iovecs)[0].iov_len = NLMSG_HDRLEN;
        for (const iovec& iov : *iovecs) {
            nlMsg.nlmsg_len += iov.iov_len;
        }

        ALOGD("Sending Netlink XFRM Message: %s", xfrmMsgTypeToString(nlMsgType));
        LOG_IOV(*iovecs);

        StatusOr<size_t> writeResult = getSyscallInstance().writev(mSock, *iovecs);
        if (!isOk(writeResult)) {
            ALOGE("netlink socket writev failed (%s)", toString(writeResult).c_str());
            return writeResult;
        }

        if (nlMsg.nlmsg_len != writeResult.value()) {
            ALOGE("Invalid netlink message length sent %d", static_cast<int>(writeResult.value()));
            return netdutils::statusFromErrno(EBADMSG, "Invalid message length");
        }

        NetlinkResponse response = {};

        StatusOr<Slice> readResult =
            getSyscallInstance().read(Fd(mSock), netdutils::makeSlice(response));
        if (!isOk(readResult)) {
            ALOGE("netlink response error (%s)", toString(readResult).c_str());
            return readResult;
        }

        LOG_HEX("netlink msg resp", reinterpret_cast<char*>(readResult.value().base()),
                readResult.value().size());

        Status validateStatus = validateResponse(response, readResult.value().size());
        if (!isOk(validateStatus)) {
            ALOGE("netlink response contains error (%s)", toString(validateStatus).c_str());
        }

        return validateStatus;
    }
};

StatusOr<int> convertToXfrmAddr(const std::string& strAddr, xfrm_address_t* xfrmAddr) {
    if (strAddr.length() == 0) {
        memset(xfrmAddr, 0, sizeof(*xfrmAddr));
        return AF_UNSPEC;
    }

    if (inet_pton(AF_INET6, strAddr.c_str(), reinterpret_cast<void*>(xfrmAddr))) {
        return AF_INET6;
    } else if (inet_pton(AF_INET, strAddr.c_str(), reinterpret_cast<void*>(xfrmAddr))) {
        return AF_INET;
    } else {
        return netdutils::statusFromErrno(EAFNOSUPPORT, "Invalid address family");
    }
}

void fillXfrmNlaHdr(nlattr* hdr, uint16_t type, uint16_t len) {
    hdr->nla_type = type;
    hdr->nla_len = len;
}

void fillXfrmCurLifetimeDefaults(xfrm_lifetime_cur* cur) {
    memset(reinterpret_cast<char*>(cur), 0, sizeof(*cur));
}
void fillXfrmLifetimeDefaults(xfrm_lifetime_cfg* cfg) {
    cfg->soft_byte_limit = XFRM_INF;
    cfg->hard_byte_limit = XFRM_INF;
    cfg->soft_packet_limit = XFRM_INF;
    cfg->hard_packet_limit = XFRM_INF;
}

/*
 * Allocate SPIs within an (inclusive) range of min-max.
 * returns 0 (INVALID_SPI) once the entire range has been parsed.
 */
class RandomSpi {
public:
    RandomSpi(int min, int max) : mMin(min) {
        // Re-seeding should be safe because the seed itself is
        // sufficiently random and we don't need secure random
        std::mt19937 rnd = std::mt19937(std::random_device()());
        mNext = std::uniform_int_distribution<>(1, INT_MAX)(rnd);
        mSize = max - min + 1;
        mCount = mSize;
    }

    uint32_t next() {
        if (!mCount)
            return 0;
        mCount--;
        return (mNext++ % mSize) + mMin;
    }

private:
    uint32_t mNext;
    uint32_t mSize;
    uint32_t mMin;
    uint32_t mCount;
};

} // namespace

//
// Begin XfrmController Impl
//
//
XfrmController::XfrmController(void) {}

netdutils::Status XfrmController::Init() {
    RETURN_IF_NOT_OK(flushInterfaces());
    XfrmSocketImpl sock;
    RETURN_IF_NOT_OK(sock.open());
    RETURN_IF_NOT_OK(flushSaDb(sock));
    return flushPolicyDb(sock);
}

netdutils::Status XfrmController::flushInterfaces() {
    const auto& ifaces = InterfaceController::getIfaceNames();
    RETURN_IF_NOT_OK(ifaces);
    const String8 ifPrefix8 = String8(INetd::IPSEC_INTERFACE_PREFIX().string());

    for (const std::string& iface : ifaces.value()) {
        int status = 0;
        // Look for the reserved interface prefix, which must be in the name at position 0
        if (!iface.compare(0, ifPrefix8.length(), ifPrefix8.c_str()) &&
            (status = removeVirtualTunnelInterface(iface)) < 0) {
            ALOGE("Failed to delete ipsec tunnel %s.", iface.c_str());
            return netdutils::statusFromErrno(status, "Failed to remove ipsec tunnel.");
        }
    }
    return netdutils::status::ok;
}

netdutils::Status XfrmController::flushSaDb(const XfrmSocket& s) {
    struct xfrm_usersa_flush flushUserSa = {.proto = IPSEC_PROTO_ANY};

    std::vector<iovec> iov = {{NULL, 0}, // reserved for the eventual addition of a NLMSG_HDR
                              {&flushUserSa, sizeof(flushUserSa)}, // xfrm_usersa_flush structure
                              {kPadBytes, NLMSG_ALIGN(sizeof(flushUserSa)) - sizeof(flushUserSa)}};

    return s.sendMessage(XFRM_MSG_FLUSHSA, NETLINK_REQUEST_FLAGS, 0, &iov);
}

netdutils::Status XfrmController::flushPolicyDb(const XfrmSocket& s) {
    std::vector<iovec> iov = {{NULL, 0}}; // reserved for the eventual addition of a NLMSG_HDR
    return s.sendMessage(XFRM_MSG_FLUSHPOLICY, NETLINK_REQUEST_FLAGS, 0, &iov);
}

netdutils::Status XfrmController::ipSecSetEncapSocketOwner(const android::base::unique_fd& socket,
                                                           int newUid, uid_t callerUid) {
    ALOGD("XfrmController:%s, line=%d", __FUNCTION__, __LINE__);

    const int fd = socket.get();
    struct stat info;
    if (fstat(fd, &info)) {
        return netdutils::statusFromErrno(errno, "Failed to stat socket file descriptor");
    }
    if (info.st_uid != callerUid) {
        return netdutils::statusFromErrno(EPERM, "fchown disabled for non-owner calls");
    }
    if (S_ISSOCK(info.st_mode) == 0) {
        return netdutils::statusFromErrno(EINVAL, "File descriptor was not a socket");
    }

    int optval;
    socklen_t optlen;
    netdutils::Status status =
        getSyscallInstance().getsockopt(Fd(socket), IPPROTO_UDP, UDP_ENCAP, &optval, &optlen);
    if (status != netdutils::status::ok) {
        return status;
    }
    if (optval != UDP_ENCAP_ESPINUDP && optval != UDP_ENCAP_ESPINUDP_NON_IKE) {
        return netdutils::statusFromErrno(EINVAL, "Socket did not have UDP-encap sockopt set");
    }
    if (fchown(fd, newUid, -1)) {
        return netdutils::statusFromErrno(errno, "Failed to fchown socket file descriptor");
    }

    return netdutils::status::ok;
}

netdutils::Status XfrmController::ipSecAllocateSpi(int32_t transformId,
                                                   const std::string& sourceAddress,
                                                   const std::string& destinationAddress,
                                                   int32_t inSpi, int32_t* outSpi) {
    ALOGD("XfrmController:%s, line=%d", __FUNCTION__, __LINE__);
    ALOGD("transformId=%d", transformId);
    ALOGD("sourceAddress=%s", sourceAddress.c_str());
    ALOGD("destinationAddress=%s", destinationAddress.c_str());
    ALOGD("inSpi=%0.8x", inSpi);

    XfrmSaInfo saInfo{};
    netdutils::Status ret =
        fillXfrmId(sourceAddress, destinationAddress, INVALID_SPI, 0, 0, transformId, &saInfo);
    if (!isOk(ret)) {
        return ret;
    }

    XfrmSocketImpl sock;
    netdutils::Status socketStatus = sock.open();
    if (!isOk(socketStatus)) {
        ALOGD("Sock open failed for XFRM, line=%d", __LINE__);
        return socketStatus;
    }

    int minSpi = RAND_SPI_MIN, maxSpi = RAND_SPI_MAX;

    if (inSpi)
        minSpi = maxSpi = inSpi;

    ret = allocateSpi(saInfo, minSpi, maxSpi, reinterpret_cast<uint32_t*>(outSpi), sock);
    if (!isOk(ret)) {
        // TODO: May want to return a new Status with a modified status string
        ALOGD("Failed to Allocate an SPI, line=%d", __LINE__);
        *outSpi = INVALID_SPI;
    }

    return ret;
}

netdutils::Status XfrmController::ipSecAddSecurityAssociation(
    int32_t transformId, int32_t mode, const std::string& sourceAddress,
    const std::string& destinationAddress, int32_t underlyingNetId, int32_t spi, int32_t markValue,
    int32_t markMask, const std::string& authAlgo, const std::vector<uint8_t>& authKey,
    int32_t authTruncBits, const std::string& cryptAlgo, const std::vector<uint8_t>& cryptKey,
    int32_t cryptTruncBits, const std::string& aeadAlgo, const std::vector<uint8_t>& aeadKey,
    int32_t aeadIcvBits, int32_t encapType, int32_t encapLocalPort, int32_t encapRemotePort) {
    ALOGD("XfrmController::%s, line=%d", __FUNCTION__, __LINE__);
    ALOGD("transformId=%d", transformId);
    ALOGD("mode=%d", mode);
    ALOGD("sourceAddress=%s", sourceAddress.c_str());
    ALOGD("destinationAddress=%s", destinationAddress.c_str());
    ALOGD("underlyingNetworkId=%d", underlyingNetId);
    ALOGD("spi=%0.8x", spi);
    ALOGD("markValue=%x", markValue);
    ALOGD("markMask=%x", markMask);
    ALOGD("authAlgo=%s", authAlgo.c_str());
    ALOGD("authTruncBits=%d", authTruncBits);
    ALOGD("cryptAlgo=%s", cryptAlgo.c_str());
    ALOGD("cryptTruncBits=%d,", cryptTruncBits);
    ALOGD("aeadAlgo=%s", aeadAlgo.c_str());
    ALOGD("aeadIcvBits=%d,", aeadIcvBits);
    ALOGD("encapType=%d", encapType);
    ALOGD("encapLocalPort=%d", encapLocalPort);
    ALOGD("encapRemotePort=%d", encapRemotePort);

    XfrmSaInfo saInfo{};
    netdutils::Status ret = fillXfrmId(sourceAddress, destinationAddress, spi, markValue, markMask,
                                       transformId, &saInfo);
    if (!isOk(ret)) {
        return ret;
    }

    saInfo.auth = XfrmAlgo{
        .name = authAlgo, .key = authKey, .truncLenBits = static_cast<uint16_t>(authTruncBits)};

    saInfo.crypt = XfrmAlgo{
        .name = cryptAlgo, .key = cryptKey, .truncLenBits = static_cast<uint16_t>(cryptTruncBits)};

    saInfo.aead = XfrmAlgo{
        .name = aeadAlgo, .key = aeadKey, .truncLenBits = static_cast<uint16_t>(aeadIcvBits)};

    switch (static_cast<XfrmMode>(mode)) {
        case XfrmMode::TRANSPORT:
        case XfrmMode::TUNNEL:
            saInfo.mode = static_cast<XfrmMode>(mode);
            break;
        default:
            return netdutils::statusFromErrno(EINVAL, "Invalid xfrm mode");
    }

    XfrmSocketImpl sock;
    netdutils::Status socketStatus = sock.open();
    if (!isOk(socketStatus)) {
        ALOGD("Sock open failed for XFRM, line=%d", __LINE__);
        return socketStatus;
    }

    switch (static_cast<XfrmEncapType>(encapType)) {
        case XfrmEncapType::ESPINUDP:
        case XfrmEncapType::ESPINUDP_NON_IKE:
            if (saInfo.addrFamily != AF_INET) {
                return netdutils::statusFromErrno(EAFNOSUPPORT, "IPv6 encap not supported");
            }
            // The ports are not used on input SAs, so this is OK to be wrong when
            // direction is ultimately input.
            saInfo.encap.srcPort = encapLocalPort;
            saInfo.encap.dstPort = encapRemotePort;
        // fall through
        case XfrmEncapType::NONE:
            saInfo.encap.type = static_cast<XfrmEncapType>(encapType);
            break;
        default:
            return netdutils::statusFromErrno(EINVAL, "Invalid encap type");
    }

    saInfo.netId = underlyingNetId;

    ret = updateSecurityAssociation(saInfo, sock);
    if (!isOk(ret)) {
        ALOGD("Failed updating a Security Association, line=%d", __LINE__);
    }

    return ret;
}

netdutils::Status XfrmController::ipSecDeleteSecurityAssociation(
    int32_t transformId, const std::string& sourceAddress, const std::string& destinationAddress,
    int32_t spi, int32_t markValue, int32_t markMask) {
    ALOGD("XfrmController:%s, line=%d", __FUNCTION__, __LINE__);
    ALOGD("transformId=%d", transformId);
    ALOGD("sourceAddress=%s", sourceAddress.c_str());
    ALOGD("destinationAddress=%s", destinationAddress.c_str());
    ALOGD("spi=%0.8x", spi);
    ALOGD("markValue=%x", markValue);
    ALOGD("markMask=%x", markMask);

    XfrmId saId{};
    netdutils::Status ret =
        fillXfrmId(sourceAddress, destinationAddress, spi, markValue, markMask, transformId, &saId);
    if (!isOk(ret)) {
        return ret;
    }

    XfrmSocketImpl sock;
    netdutils::Status socketStatus = sock.open();
    if (!isOk(socketStatus)) {
        ALOGD("Sock open failed for XFRM, line=%d", __LINE__);
        return socketStatus;
    }

    ret = deleteSecurityAssociation(saId, sock);
    if (!isOk(ret)) {
        ALOGD("Failed to delete Security Association, line=%d", __LINE__);
    }

    return ret;
}

netdutils::Status XfrmController::fillXfrmId(const std::string& sourceAddress,
                                             const std::string& destinationAddress, int32_t spi,
                                             int32_t markValue, int32_t markMask,
                                             int32_t transformId, XfrmId* xfrmId) {
    // Fill the straightforward fields first
    xfrmId->transformId = transformId;
    xfrmId->spi = htonl(spi);
    xfrmId->mark.v = markValue;
    xfrmId->mark.m = markMask;

    // Use the addresses to determine the address family and do validation
    xfrm_address_t sourceXfrmAddr{}, destXfrmAddr{};
    StatusOr<int> sourceFamily, destFamily;
    sourceFamily = convertToXfrmAddr(sourceAddress, &sourceXfrmAddr);
    destFamily = convertToXfrmAddr(destinationAddress, &destXfrmAddr);
    if (!isOk(sourceFamily) || !isOk(destFamily)) {
        return netdutils::statusFromErrno(EINVAL, "Invalid address " + sourceAddress + "/" +
                                                      destinationAddress);
    }

    if (destFamily.value() == AF_UNSPEC ||
        (sourceFamily.value() != AF_UNSPEC && sourceFamily.value() != destFamily.value())) {
        ALOGD("Invalid or Mismatched Address Families, %d != %d, line=%d", sourceFamily.value(),
              destFamily.value(), __LINE__);
        return netdutils::statusFromErrno(EINVAL, "Invalid or mismatched address families");
    }

    xfrmId->addrFamily = destFamily.value();

    xfrmId->dstAddr = destXfrmAddr;
    xfrmId->srcAddr = sourceXfrmAddr;
    return netdutils::status::ok;
}

netdutils::Status XfrmController::ipSecApplyTransportModeTransform(
    const android::base::unique_fd& socket, int32_t transformId, int32_t direction,
    const std::string& sourceAddress, const std::string& destinationAddress, int32_t spi) {
    ALOGD("XfrmController::%s, line=%d", __FUNCTION__, __LINE__);
    ALOGD("transformId=%d", transformId);
    ALOGD("direction=%d", direction);
    ALOGD("sourceAddress=%s", sourceAddress.c_str());
    ALOGD("destinationAddress=%s", destinationAddress.c_str());
    ALOGD("spi=%0.8x", spi);

    StatusOr<sockaddr_storage> ret = getSyscallInstance().getsockname<sockaddr_storage>(Fd(socket));
    if (!isOk(ret)) {
        ALOGE("Failed to get socket info in %s", __FUNCTION__);
        return ret;
    }
    struct sockaddr_storage saddr = ret.value();

    XfrmSaInfo saInfo{};
    netdutils::Status status =
        fillXfrmId(sourceAddress, destinationAddress, spi, 0, 0, transformId, &saInfo);
    if (!isOk(status)) {
        ALOGE("Couldn't build SA ID %s", __FUNCTION__);
        return status;
    }

    if (saddr.ss_family == AF_INET && saInfo.addrFamily != AF_INET) {
        ALOGE("IPV4 socket address family(%d) should match IPV4 Transform "
              "address family(%d)!",
              saddr.ss_family, saInfo.addrFamily);
        return netdutils::statusFromErrno(EINVAL, "Mismatched address family");
    }

    struct {
        xfrm_userpolicy_info info;
        xfrm_user_tmpl tmpl;
    } policy{};

    fillTransportModeUserSpInfo(saInfo, static_cast<XfrmDirection>(direction), &policy.info);
    fillUserTemplate(saInfo, &policy.tmpl);

    LOG_HEX("XfrmUserPolicy", reinterpret_cast<char*>(&policy), sizeof(policy));

    int sockOpt, sockLayer;
    switch (saddr.ss_family) {
        case AF_INET:
            sockOpt = IP_XFRM_POLICY;
            sockLayer = SOL_IP;
            break;
        case AF_INET6:
            sockOpt = IPV6_XFRM_POLICY;
            sockLayer = SOL_IPV6;
            break;
        default:
            return netdutils::statusFromErrno(EAFNOSUPPORT, "Invalid address family");
    }

    status = getSyscallInstance().setsockopt(Fd(socket), sockLayer, sockOpt, policy);
    if (!isOk(status)) {
        ALOGE("Error setting socket option for XFRM! (%s)", toString(status).c_str());
    }

    return status;
}

netdutils::Status
XfrmController::ipSecRemoveTransportModeTransform(const android::base::unique_fd& socket) {
    ALOGD("XfrmController::%s, line=%d", __FUNCTION__, __LINE__);

    StatusOr<sockaddr_storage> ret = getSyscallInstance().getsockname<sockaddr_storage>(Fd(socket));
    if (!isOk(ret)) {
        ALOGE("Failed to get socket info in %s! (%s)", __FUNCTION__, toString(ret).c_str());
        return ret;
    }

    int sockOpt, sockLayer;
    switch (ret.value().ss_family) {
        case AF_INET:
            sockOpt = IP_XFRM_POLICY;
            sockLayer = SOL_IP;
            break;
        case AF_INET6:
            sockOpt = IPV6_XFRM_POLICY;
            sockLayer = SOL_IPV6;
            break;
        default:
            return netdutils::statusFromErrno(EAFNOSUPPORT, "Invalid address family");
    }

    // Kernel will delete the security policy on this socket for both direction
    // if optval is set to NULL and optlen is set to 0.
    netdutils::Status status =
        getSyscallInstance().setsockopt(Fd(socket), sockLayer, sockOpt, NULL, 0);
    if (!isOk(status)) {
        ALOGE("Error removing socket option for XFRM! (%s)", toString(status).c_str());
    }

    return status;
}

netdutils::Status XfrmController::ipSecAddSecurityPolicy(int32_t transformId, int32_t direction,
                                                         const std::string& localAddress,
                                                         const std::string& remoteAddress,
                                                         int32_t spi, int32_t markValue,
                                                         int32_t markMask) {
    return processSecurityPolicy(transformId, direction, localAddress, remoteAddress, spi,
                                 markValue, markMask, XFRM_MSG_NEWPOLICY);
}

netdutils::Status XfrmController::ipSecUpdateSecurityPolicy(int32_t transformId, int32_t direction,
                                                            const std::string& localAddress,
                                                            const std::string& remoteAddress,
                                                            int32_t spi, int32_t markValue,
                                                            int32_t markMask) {
    return processSecurityPolicy(transformId, direction, localAddress, remoteAddress, spi,
                                 markValue, markMask, XFRM_MSG_UPDPOLICY);
}

netdutils::Status XfrmController::ipSecDeleteSecurityPolicy(int32_t transformId, int32_t direction,
                                                            const std::string& localAddress,
                                                            const std::string& remoteAddress,
                                                            int32_t markValue, int32_t markMask) {
    return processSecurityPolicy(transformId, direction, localAddress, remoteAddress, 0, markValue,
                                 markMask, XFRM_MSG_DELPOLICY);
}

netdutils::Status XfrmController::processSecurityPolicy(int32_t transformId, int32_t direction,
                                                        const std::string& localAddress,
                                                        const std::string& remoteAddress,
                                                        int32_t spi, int32_t markValue,
                                                        int32_t markMask, int32_t msgType) {
    ALOGD("XfrmController::%s, line=%d", __FUNCTION__, __LINE__);
    ALOGD("transformId=%d", transformId);
    ALOGD("direction=%d", direction);
    ALOGD("localAddress=%s", localAddress.c_str());
    ALOGD("remoteAddress=%s", remoteAddress.c_str());
    ALOGD("spi=%0.8x", spi);
    ALOGD("markValue=%d", markValue);
    ALOGD("markMask=%d", markMask);
    ALOGD("msgType=%d", msgType);

    XfrmSaInfo saInfo{};
    saInfo.mode = XfrmMode::TUNNEL;

    XfrmSocketImpl sock;
    RETURN_IF_NOT_OK(sock.open());

    RETURN_IF_NOT_OK(
        fillXfrmId(localAddress, remoteAddress, spi, markValue, markMask, transformId, &saInfo));

    if (msgType == XFRM_MSG_DELPOLICY) {
        return deleteTunnelModeSecurityPolicy(saInfo, sock, static_cast<XfrmDirection>(direction));
    } else {
        return updateTunnelModeSecurityPolicy(saInfo, sock, static_cast<XfrmDirection>(direction),
                                              msgType);
    }
}

void XfrmController::fillXfrmSelector(const XfrmSaInfo& record, xfrm_selector* selector) {
    selector->family = record.addrFamily;
    selector->proto = AF_UNSPEC; // TODO: do we need to match the protocol? it's
                                 // possible via the socket
}

netdutils::Status XfrmController::updateSecurityAssociation(const XfrmSaInfo& record,
                                                            const XfrmSocket& sock) {
    xfrm_usersa_info usersa{};
    nlattr_algo_crypt crypt{};
    nlattr_algo_auth auth{};
    nlattr_algo_aead aead{};
    nlattr_xfrm_mark xfrmmark{};
    nlattr_xfrm_output_mark xfrmoutputmark{};
    nlattr_encap_tmpl encap{};

    enum {
        NLMSG_HDR,
        USERSA,
        USERSA_PAD,
        CRYPT,
        CRYPT_PAD,
        AUTH,
        AUTH_PAD,
        AEAD,
        AEAD_PAD,
        MARK,
        MARK_PAD,
        OUTPUT_MARK,
        OUTPUT_MARK_PAD,
        ENCAP,
        ENCAP_PAD,
    };

    std::vector<iovec> iov = {
        {NULL, 0},            // reserved for the eventual addition of a NLMSG_HDR
        {&usersa, 0},         // main usersa_info struct
        {kPadBytes, 0},       // up to NLMSG_ALIGNTO pad bytes of padding
        {&crypt, 0},          // adjust size if crypt algo is present
        {kPadBytes, 0},       // up to NLATTR_ALIGNTO pad bytes
        {&auth, 0},           // adjust size if auth algo is present
        {kPadBytes, 0},       // up to NLATTR_ALIGNTO pad bytes
        {&aead, 0},           // adjust size if aead algo is present
        {kPadBytes, 0},       // up to NLATTR_ALIGNTO pad bytes
        {&xfrmmark, 0},       // adjust size if xfrm mark is present
        {kPadBytes, 0},       // up to NLATTR_ALIGNTO pad bytes
        {&xfrmoutputmark, 0}, // adjust size if xfrm output mark is present
        {kPadBytes, 0},       // up to NLATTR_ALIGNTO pad bytes
        {&encap, 0},          // adjust size if encapsulating
        {kPadBytes, 0},       // up to NLATTR_ALIGNTO pad bytes
    };

    if (!record.aead.name.empty() && (!record.auth.name.empty() || !record.crypt.name.empty())) {
        return netdutils::statusFromErrno(EINVAL, "Invalid xfrm algo selection; AEAD is mutually "
                                                  "exclusive with both Authentication and "
                                                  "Encryption");
    }

    if (record.aead.key.size() > MAX_KEY_LENGTH || record.auth.key.size() > MAX_KEY_LENGTH ||
        record.crypt.key.size() > MAX_KEY_LENGTH) {
        return netdutils::statusFromErrno(EINVAL, "Key length invalid; exceeds MAX_KEY_LENGTH");
    }

    int len;
    len = iov[USERSA].iov_len = fillUserSaInfo(record, &usersa);
    iov[USERSA_PAD].iov_len = NLMSG_ALIGN(len) - len;

    len = iov[CRYPT].iov_len = fillNlAttrXfrmAlgoEnc(record.crypt, &crypt);
    iov[CRYPT_PAD].iov_len = NLA_ALIGN(len) - len;

    len = iov[AUTH].iov_len = fillNlAttrXfrmAlgoAuth(record.auth, &auth);
    iov[AUTH_PAD].iov_len = NLA_ALIGN(len) - len;

    len = iov[AEAD].iov_len = fillNlAttrXfrmAlgoAead(record.aead, &aead);
    iov[AEAD_PAD].iov_len = NLA_ALIGN(len) - len;

    len = iov[MARK].iov_len = fillNlAttrXfrmMark(record, &xfrmmark);
    iov[MARK_PAD].iov_len = NLA_ALIGN(len) - len;

    len = iov[OUTPUT_MARK].iov_len = fillNlAttrXfrmOutputMark(record.netId, &xfrmoutputmark);
    iov[OUTPUT_MARK_PAD].iov_len = NLA_ALIGN(len) - len;

    len = iov[ENCAP].iov_len = fillNlAttrXfrmEncapTmpl(record, &encap);
    iov[ENCAP_PAD].iov_len = NLA_ALIGN(len) - len;

    return sock.sendMessage(XFRM_MSG_UPDSA, NETLINK_REQUEST_FLAGS, 0, &iov);
}

int XfrmController::fillNlAttrXfrmAlgoEnc(const XfrmAlgo& inAlgo, nlattr_algo_crypt* algo) {
    if (inAlgo.name.empty()) { // Do not fill anything if algorithm not provided
        return 0;
    }

    int len = NLA_HDRLEN + sizeof(xfrm_algo);
    // Kernel always changes last char to null terminator; no safety checks needed.
    strncpy(algo->crypt.alg_name, inAlgo.name.c_str(), sizeof(algo->crypt.alg_name));
    algo->crypt.alg_key_len = inAlgo.key.size() * 8; // bits
    memcpy(algo->key, &inAlgo.key[0], inAlgo.key.size());
    len += inAlgo.key.size();
    fillXfrmNlaHdr(&algo->hdr, XFRMA_ALG_CRYPT, len);
    return len;
}

int XfrmController::fillNlAttrXfrmAlgoAuth(const XfrmAlgo& inAlgo, nlattr_algo_auth* algo) {
    if (inAlgo.name.empty()) { // Do not fill anything if algorithm not provided
        return 0;
    }

    int len = NLA_HDRLEN + sizeof(xfrm_algo_auth);
    // Kernel always changes last char to null terminator; no safety checks needed.
    strncpy(algo->auth.alg_name, inAlgo.name.c_str(), sizeof(algo->auth.alg_name));
    algo->auth.alg_key_len = inAlgo.key.size() * 8; // bits

    // This is the extra field for ALG_AUTH_TRUNC
    algo->auth.alg_trunc_len = inAlgo.truncLenBits;

    memcpy(algo->key, &inAlgo.key[0], inAlgo.key.size());
    len += inAlgo.key.size();

    fillXfrmNlaHdr(&algo->hdr, XFRMA_ALG_AUTH_TRUNC, len);
    return len;
}

int XfrmController::fillNlAttrXfrmAlgoAead(const XfrmAlgo& inAlgo, nlattr_algo_aead* algo) {
    if (inAlgo.name.empty()) { // Do not fill anything if algorithm not provided
        return 0;
    }

    int len = NLA_HDRLEN + sizeof(xfrm_algo_aead);
    // Kernel always changes last char to null terminator; no safety checks needed.
    strncpy(algo->aead.alg_name, inAlgo.name.c_str(), sizeof(algo->aead.alg_name));
    algo->aead.alg_key_len = inAlgo.key.size() * 8; // bits

    // This is the extra field for ALG_AEAD. ICV length is the same as truncation length
    // for any AEAD algorithm.
    algo->aead.alg_icv_len = inAlgo.truncLenBits;

    memcpy(algo->key, &inAlgo.key[0], inAlgo.key.size());
    len += inAlgo.key.size();

    fillXfrmNlaHdr(&algo->hdr, XFRMA_ALG_AEAD, len);
    return len;
}

int XfrmController::fillNlAttrXfrmEncapTmpl(const XfrmSaInfo& record, nlattr_encap_tmpl* tmpl) {
    if (record.encap.type == XfrmEncapType::NONE) {
        return 0;
    }

    int len = NLA_HDRLEN + sizeof(xfrm_encap_tmpl);
    tmpl->tmpl.encap_type = static_cast<uint16_t>(record.encap.type);
    tmpl->tmpl.encap_sport = htons(record.encap.srcPort);
    tmpl->tmpl.encap_dport = htons(record.encap.dstPort);
    fillXfrmNlaHdr(&tmpl->hdr, XFRMA_ENCAP, len);
    return len;
}

int XfrmController::fillUserSaInfo(const XfrmSaInfo& record, xfrm_usersa_info* usersa) {
    fillXfrmSelector(record, &usersa->sel);

    usersa->id.proto = IPPROTO_ESP;
    usersa->id.spi = record.spi;
    usersa->id.daddr = record.dstAddr;

    usersa->saddr = record.srcAddr;

    fillXfrmLifetimeDefaults(&usersa->lft);
    fillXfrmCurLifetimeDefaults(&usersa->curlft);
    memset(&usersa->stats, 0, sizeof(usersa->stats)); // leave stats zeroed out
    usersa->reqid = record.transformId;
    usersa->family = record.addrFamily;
    usersa->mode = static_cast<uint8_t>(record.mode);
    usersa->replay_window = REPLAY_WINDOW_SIZE;

    if (record.mode == XfrmMode::TRANSPORT) {
        usersa->flags = 0; // TODO: should we actually set flags, XFRM_SA_XFLAG_DONT_ENCAP_DSCP?
    } else {
        usersa->flags = XFRM_STATE_AF_UNSPEC;
    }

    return sizeof(*usersa);
}

int XfrmController::fillUserSaId(const XfrmId& record, xfrm_usersa_id* said) {
    said->daddr = record.dstAddr;
    said->spi = record.spi;
    said->family = record.addrFamily;
    said->proto = IPPROTO_ESP;

    return sizeof(*said);
}

netdutils::Status XfrmController::deleteSecurityAssociation(const XfrmId& record,
                                                            const XfrmSocket& sock) {
    xfrm_usersa_id said{};
    nlattr_xfrm_mark xfrmmark{};

    enum { NLMSG_HDR, USERSAID, USERSAID_PAD, MARK, MARK_PAD };

    std::vector<iovec> iov = {
        {NULL, 0},      // reserved for the eventual addition of a NLMSG_HDR
        {&said, 0},     // main usersa_info struct
        {kPadBytes, 0}, // up to NLMSG_ALIGNTO pad bytes of padding
        {&xfrmmark, 0}, // adjust size if xfrm mark is present
        {kPadBytes, 0}, // up to NLATTR_ALIGNTO pad bytes
    };

    int len;
    len = iov[USERSAID].iov_len = fillUserSaId(record, &said);
    iov[USERSAID_PAD].iov_len = NLMSG_ALIGN(len) - len;

    len = iov[MARK].iov_len = fillNlAttrXfrmMark(record, &xfrmmark);
    iov[MARK_PAD].iov_len = NLA_ALIGN(len) - len;

    return sock.sendMessage(XFRM_MSG_DELSA, NETLINK_REQUEST_FLAGS, 0, &iov);
}

netdutils::Status XfrmController::allocateSpi(const XfrmSaInfo& record, uint32_t minSpi,
                                              uint32_t maxSpi, uint32_t* outSpi,
                                              const XfrmSocket& sock) {
    xfrm_userspi_info spiInfo{};

    enum { NLMSG_HDR, USERSAID, USERSAID_PAD };

    std::vector<iovec> iov = {
        {NULL, 0},      // reserved for the eventual addition of a NLMSG_HDR
        {&spiInfo, 0},  // main userspi_info struct
        {kPadBytes, 0}, // up to NLMSG_ALIGNTO pad bytes of padding
    };

    int len;
    if (fillUserSaInfo(record, &spiInfo.info) == 0) {
        ALOGE("Failed to fill transport SA Info");
    }

    len = iov[USERSAID].iov_len = sizeof(spiInfo);
    iov[USERSAID_PAD].iov_len = NLMSG_ALIGN(len) - len;

    RandomSpi spiGen = RandomSpi(minSpi, maxSpi);
    int spi;
    netdutils::Status ret;
    while ((spi = spiGen.next()) != INVALID_SPI) {
        spiInfo.min = spi;
        spiInfo.max = spi;
        ret = sock.sendMessage(XFRM_MSG_ALLOCSPI, NETLINK_REQUEST_FLAGS, 0, &iov);

        /* If the SPI is in use, we'll get ENOENT */
        if (netdutils::equalToErrno(ret, ENOENT))
            continue;

        if (isOk(ret)) {
            *outSpi = spi;
            ALOGD("Allocated an SPI: %x", *outSpi);
        } else {
            *outSpi = INVALID_SPI;
            ALOGE("SPI Allocation Failed with error %d", ret.code());
        }

        return ret;
    }

    // Should always be -ENOENT if we get here
    return ret;
}

netdutils::Status XfrmController::updateTunnelModeSecurityPolicy(const XfrmSaInfo& record,
                                                                 const XfrmSocket& sock,
                                                                 XfrmDirection direction,
                                                                 uint16_t msgType) {
    xfrm_userpolicy_info userpolicy{};
    nlattr_user_tmpl usertmpl{};
    nlattr_xfrm_mark xfrmmark{};

    enum {
        NLMSG_HDR,
        USERPOLICY,
        USERPOLICY_PAD,
        USERTMPL,
        USERTMPL_PAD,
        MARK,
        MARK_PAD,
    };

    std::vector<iovec> iov = {
        {NULL, 0},        // reserved for the eventual addition of a NLMSG_HDR
        {&userpolicy, 0}, // main xfrm_userpolicy_info struct
        {kPadBytes, 0},   // up to NLMSG_ALIGNTO pad bytes of padding
        {&usertmpl, 0},   // adjust size if xfrm_user_tmpl struct is present
        {kPadBytes, 0},   // up to NLATTR_ALIGNTO pad bytes
        {&xfrmmark, 0},   // adjust size if xfrm mark is present
        {kPadBytes, 0},   // up to NLATTR_ALIGNTO pad bytes
    };

    int len;
    len = iov[USERPOLICY].iov_len = fillTransportModeUserSpInfo(record, direction, &userpolicy);
    iov[USERPOLICY_PAD].iov_len = NLMSG_ALIGN(len) - len;

    len = iov[USERTMPL].iov_len = fillNlAttrUserTemplate(record, &usertmpl);
    iov[USERTMPL_PAD].iov_len = NLA_ALIGN(len) - len;

    len = iov[MARK].iov_len = fillNlAttrXfrmMark(record, &xfrmmark);
    iov[MARK_PAD].iov_len = NLA_ALIGN(len) - len;

    return sock.sendMessage(msgType, NETLINK_REQUEST_FLAGS, 0, &iov);
}

netdutils::Status XfrmController::deleteTunnelModeSecurityPolicy(const XfrmSaInfo& record,
                                                                 const XfrmSocket& sock,
                                                                 XfrmDirection direction) {
    xfrm_userpolicy_id policyid{};
    nlattr_xfrm_mark xfrmmark{};

    enum {
        NLMSG_HDR,
        USERPOLICYID,
        USERPOLICYID_PAD,
        MARK,
        MARK_PAD,
    };

    std::vector<iovec> iov = {
        {NULL, 0},      // reserved for the eventual addition of a NLMSG_HDR
        {&policyid, 0}, // main xfrm_userpolicy_id struct
        {kPadBytes, 0}, // up to NLMSG_ALIGNTO pad bytes of padding
        {&xfrmmark, 0}, // adjust size if xfrm mark is present
        {kPadBytes, 0}, // up to NLATTR_ALIGNTO pad bytes
    };

    int len = iov[USERPOLICYID].iov_len = fillUserPolicyId(record, direction, &policyid);
    iov[USERPOLICYID_PAD].iov_len = NLMSG_ALIGN(len) - len;

    len = iov[MARK].iov_len = fillNlAttrXfrmMark(record, &xfrmmark);
    iov[MARK_PAD].iov_len = NLA_ALIGN(len) - len;

    return sock.sendMessage(XFRM_MSG_DELPOLICY, NETLINK_REQUEST_FLAGS, 0, &iov);
}

int XfrmController::fillTransportModeUserSpInfo(const XfrmSaInfo& record, XfrmDirection direction,
                                                xfrm_userpolicy_info* usersp) {
    fillXfrmSelector(record, &usersp->sel);
    fillXfrmLifetimeDefaults(&usersp->lft);
    fillXfrmCurLifetimeDefaults(&usersp->curlft);
    /* if (index) index & 0x3 == dir -- must be true
     * xfrm_user.c:verify_newpolicy_info() */
    usersp->index = 0;
    usersp->dir = static_cast<uint8_t>(direction);
    usersp->action = XFRM_POLICY_ALLOW;
    usersp->flags = XFRM_POLICY_LOCALOK;
    usersp->share = XFRM_SHARE_UNIQUE;
    return sizeof(*usersp);
}

int XfrmController::fillUserTemplate(const XfrmSaInfo& record, xfrm_user_tmpl* tmpl) {
    tmpl->id.daddr = record.dstAddr;
    tmpl->id.spi = record.spi;
    tmpl->id.proto = IPPROTO_ESP;

    tmpl->family = record.addrFamily;
    tmpl->saddr = record.srcAddr;
    tmpl->reqid = record.transformId;
    tmpl->mode = static_cast<uint8_t>(record.mode);
    tmpl->share = XFRM_SHARE_UNIQUE;
    tmpl->optional = 0; // if this is true, then a failed state lookup will be considered OK:
                        // http://lxr.free-electrons.com/source/net/xfrm/xfrm_policy.c#L1492
    tmpl->aalgos = ALGO_MASK_AUTH_ALL;  // TODO: if there's a bitmask somewhere of
                                        // algos, we should find it and apply it.
                                        // I can't find one.
    tmpl->ealgos = ALGO_MASK_CRYPT_ALL; // TODO: if there's a bitmask somewhere...
    return sizeof(xfrm_user_tmpl*);
}

int XfrmController::fillNlAttrUserTemplate(const XfrmSaInfo& record, nlattr_user_tmpl* tmpl) {
    fillUserTemplate(record, &tmpl->tmpl);

    int len = NLA_HDRLEN + sizeof(xfrm_user_tmpl);
    fillXfrmNlaHdr(&tmpl->hdr, XFRMA_TMPL, len);
    return len;
}

int XfrmController::fillNlAttrXfrmMark(const XfrmId& record, nlattr_xfrm_mark* mark) {
    mark->mark.v = record.mark.v; // set to 0 if it's not used
    mark->mark.m = record.mark.m; // set to 0 if it's not used
    int len = NLA_HDRLEN + sizeof(xfrm_mark);
    fillXfrmNlaHdr(&mark->hdr, XFRMA_MARK, len);
    return len;
}

int XfrmController::fillNlAttrXfrmOutputMark(const __u32 output_mark_value,
                                             nlattr_xfrm_output_mark* output_mark) {
    // Do not set if we were not given an output mark
    if (output_mark_value == 0) {
        return 0;
    }

    output_mark->outputMark = output_mark_value;
    int len = NLA_HDRLEN + sizeof(__u32);
    fillXfrmNlaHdr(&output_mark->hdr, XFRMA_OUTPUT_MARK, len);
    return len;
}

int XfrmController::fillUserPolicyId(const XfrmSaInfo& record, XfrmDirection direction,
                                     xfrm_userpolicy_id* usersp) {
    // For DELPOLICY, when index is absent, selector is needed to match the policy
    fillXfrmSelector(record, &usersp->sel);
    usersp->dir = static_cast<uint8_t>(direction);
    return sizeof(*usersp);
}

int XfrmController::addVirtualTunnelInterface(const std::string& deviceName,
                                              const std::string& localAddress,
                                              const std::string& remoteAddress, int32_t ikey,
                                              int32_t okey, bool isUpdate) {
    ALOGD("XfrmController::%s, line=%d", __FUNCTION__, __LINE__);
    ALOGD("deviceName=%s", deviceName.c_str());
    ALOGD("localAddress=%s", localAddress.c_str());
    ALOGD("remoteAddress=%s", remoteAddress.c_str());
    ALOGD("ikey=%0.8x", ikey);
    ALOGD("okey=%0.8x", okey);
    ALOGD("isUpdate=%d", isUpdate);

    if (deviceName.empty() || localAddress.empty() || remoteAddress.empty()) {
        return EINVAL;
    }

    const char* INFO_KIND_VTI6 = "vti6";
    const char* INFO_KIND_VTI = "vti";
    uint8_t PADDING_BUFFER[] = {0, 0, 0, 0};

    // Find address family.
    uint8_t remAddr[sizeof(in6_addr)];

    StatusOr<uint16_t> statusOrRemoteFam = convertStringAddress(remoteAddress, remAddr);
    if (!isOk(statusOrRemoteFam)) {
        return statusOrRemoteFam.status().code();
    }

    uint8_t locAddr[sizeof(in6_addr)];
    StatusOr<uint16_t> statusOrLocalFam = convertStringAddress(localAddress, locAddr);
    if (!isOk(statusOrLocalFam)) {
        return statusOrLocalFam.status().code();
    }

    if (statusOrLocalFam.value() != statusOrRemoteFam.value()) {
        return EINVAL;
    }

    uint16_t family = statusOrLocalFam.value();

    ifinfomsg ifInfoMsg{};

    // Construct IFLA_IFNAME
    nlattr iflaIfName;
    char iflaIfNameStrValue[deviceName.length() + 1];
    size_t iflaIfNameLength =
        strlcpy(iflaIfNameStrValue, deviceName.c_str(), sizeof(iflaIfNameStrValue));
    size_t iflaIfNamePad = fillNlAttr(IFLA_IFNAME, iflaIfNameLength, &iflaIfName);

    // Construct IFLA_INFO_KIND
    // Constants "vti6" and "vti" enable the kernel to call different code paths,
    // (ip_tunnel.c, ip6_tunnel), based on the family.
    const std::string infoKindValue = (family == AF_INET6) ? INFO_KIND_VTI6 : INFO_KIND_VTI;
    nlattr iflaIfInfoKind;
    char infoKindValueStrValue[infoKindValue.length() + 1];
    size_t iflaIfInfoKindLength =
        strlcpy(infoKindValueStrValue, infoKindValue.c_str(), sizeof(infoKindValueStrValue));
    size_t iflaIfInfoKindPad = fillNlAttr(IFLA_INFO_KIND, iflaIfInfoKindLength, &iflaIfInfoKind);

    // Construct IFLA_VTI_LOCAL
    nlattr iflaVtiLocal;
    uint8_t binaryLocalAddress[sizeof(in6_addr)];
    size_t iflaVtiLocalPad =
        fillNlAttrIpAddress(IFLA_VTI_LOCAL, family, localAddress, &iflaVtiLocal,
                            netdutils::makeSlice(binaryLocalAddress));

    // Construct IFLA_VTI_REMOTE
    nlattr iflaVtiRemote;
    uint8_t binaryRemoteAddress[sizeof(in6_addr)];
    size_t iflaVtiRemotePad =
        fillNlAttrIpAddress(IFLA_VTI_REMOTE, family, remoteAddress, &iflaVtiRemote,
                            netdutils::makeSlice(binaryRemoteAddress));

    // Construct IFLA_VTI_OKEY
    nlattr iflaVtiIKey;
    uint32_t iKeyValue;
    size_t iflaVtiIKeyPad = fillNlAttrU32(IFLA_VTI_IKEY, ikey, &iflaVtiIKey, &iKeyValue);

    // Construct IFLA_VTI_IKEY
    nlattr iflaVtiOKey;
    uint32_t oKeyValue;
    size_t iflaVtiOKeyPad = fillNlAttrU32(IFLA_VTI_OKEY, okey, &iflaVtiOKey, &oKeyValue);

    int iflaInfoDataPayloadLength = iflaVtiLocal.nla_len + iflaVtiLocalPad + iflaVtiRemote.nla_len +
                                    iflaVtiRemotePad + iflaVtiIKey.nla_len + iflaVtiIKeyPad +
                                    iflaVtiOKey.nla_len + iflaVtiOKeyPad;

    // Construct IFLA_INFO_DATA
    nlattr iflaInfoData;
    size_t iflaInfoDataPad = fillNlAttr(IFLA_INFO_DATA, iflaInfoDataPayloadLength, &iflaInfoData);

    // Construct IFLA_LINKINFO
    nlattr iflaLinkInfo;
    size_t iflaLinkInfoPad = fillNlAttr(IFLA_LINKINFO,
                                        iflaInfoData.nla_len + iflaInfoDataPad +
                                            iflaIfInfoKind.nla_len + iflaIfInfoKindPad,
                                        &iflaLinkInfo);

    iovec iov[] = {
        {NULL, 0},
        {&ifInfoMsg, sizeof(ifInfoMsg)},

        {&iflaIfName, sizeof(iflaIfName)},
        {iflaIfNameStrValue, iflaIfNameLength},
        {&PADDING_BUFFER, iflaIfNamePad},

        {&iflaLinkInfo, sizeof(iflaLinkInfo)},

        {&iflaIfInfoKind, sizeof(iflaIfInfoKind)},
        {infoKindValueStrValue, iflaIfInfoKindLength},
        {&PADDING_BUFFER, iflaIfInfoKindPad},

        {&iflaInfoData, sizeof(iflaInfoData)},

        {&iflaVtiLocal, sizeof(iflaVtiLocal)},
        {&binaryLocalAddress, (family == AF_INET) ? sizeof(in_addr) : sizeof(in6_addr)},
        {&PADDING_BUFFER, iflaVtiLocalPad},

        {&iflaVtiRemote, sizeof(iflaVtiRemote)},
        {&binaryRemoteAddress, (family == AF_INET) ? sizeof(in_addr) : sizeof(in6_addr)},
        {&PADDING_BUFFER, iflaVtiRemotePad},

        {&iflaVtiIKey, sizeof(iflaVtiIKey)},
        {&iKeyValue, sizeof(iKeyValue)},
        {&PADDING_BUFFER, iflaVtiIKeyPad},

        {&iflaVtiOKey, sizeof(iflaVtiOKey)},
        {&oKeyValue, sizeof(oKeyValue)},
        {&PADDING_BUFFER, iflaVtiOKeyPad},

        {&PADDING_BUFFER, iflaInfoDataPad},

        {&PADDING_BUFFER, iflaLinkInfoPad},
    };

    uint16_t action = RTM_NEWLINK;
    uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;

    if (!isUpdate) {
        flags |= NLM_F_EXCL | NLM_F_CREATE;
    }

    // sendNetlinkRequest returns -errno
    int ret = -1 * sendNetlinkRequest(action, flags, iov, ARRAY_SIZE(iov), nullptr);
    if (ret) {
        ALOGE("Error in %s virtual tunnel interface. Error Code: %d",
              isUpdate ? "updating" : "adding", ret);
    }
    return ret;
}

int XfrmController::removeVirtualTunnelInterface(const std::string& deviceName) {
    ALOGD("XfrmController::%s, line=%d", __FUNCTION__, __LINE__);
    ALOGD("deviceName=%s", deviceName.c_str());

    if (deviceName.empty()) {
        return EINVAL;
    }

    uint8_t PADDING_BUFFER[] = {0, 0, 0, 0};

    ifinfomsg ifInfoMsg{};
    nlattr iflaIfName;
    char iflaIfNameStrValue[deviceName.length() + 1];
    size_t iflaIfNameLength =
        strlcpy(iflaIfNameStrValue, deviceName.c_str(), sizeof(iflaIfNameStrValue));
    size_t iflaIfNamePad = fillNlAttr(IFLA_IFNAME, iflaIfNameLength, &iflaIfName);

    iovec iov[] = {
        {NULL, 0},
        {&ifInfoMsg, sizeof(ifInfoMsg)},

        {&iflaIfName, sizeof(iflaIfName)},
        {iflaIfNameStrValue, iflaIfNameLength},
        {&PADDING_BUFFER, iflaIfNamePad},
    };

    uint16_t action = RTM_DELLINK;
    uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;

    // sendNetlinkRequest returns -errno
    int ret = -1 * sendNetlinkRequest(action, flags, iov, ARRAY_SIZE(iov), nullptr);
    if (ret) {
        ALOGE("Error in removing virtual tunnel interface %s. Error Code: %d", iflaIfNameStrValue,
              ret);
    }
    return ret;
}

} // namespace net
} // namespace android
