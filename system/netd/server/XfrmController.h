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
#ifndef _XFRM_CONTROLLER_H
#define _XFRM_CONTROLLER_H

#include <atomic>
#include <list>
#include <map>
#include <string>
#include <utility> // for pair

#include <linux/if_link.h>
#include <linux/if_tunnel.h>
#include <linux/netlink.h>
#include <linux/udp.h>
#include <linux/xfrm.h>
#include <sysutils/SocketClient.h>

#include "NetdConstants.h"
#include "netdutils/Slice.h"
#include "netdutils/Status.h"

namespace android {
namespace net {

// Exposed for testing
extern const uint32_t ALGO_MASK_AUTH_ALL;
// Exposed for testing
extern const uint32_t ALGO_MASK_CRYPT_ALL;
// Exposed for testing
extern const uint32_t ALGO_MASK_AEAD_ALL;
// Exposed for testing
extern const uint8_t REPLAY_WINDOW_SIZE;

// Suggest we avoid the smallest and largest ints
class XfrmMessage;
class TransportModeSecurityAssociation;

class XfrmSocket {
public:
    virtual void close() {
        if (mSock >= 0) {
            ::close(mSock);
        }
        mSock = -1;
    }

    virtual netdutils::Status open() = 0;

    virtual ~XfrmSocket() { close(); }

    // Sends the netlink message contained in iovecs. This populates iovecs[0] with
    // a valid netlink message header.
    virtual netdutils::Status sendMessage(uint16_t nlMsgType, uint16_t nlMsgFlags,
                                          uint16_t nlMsgSeqNum,
                                          std::vector<iovec>* iovecs) const = 0;

protected:
    int mSock;
};

enum struct XfrmDirection : uint8_t {
    IN = XFRM_POLICY_IN,
    OUT = XFRM_POLICY_OUT,
    FORWARD = XFRM_POLICY_FWD,
    MASK = XFRM_POLICY_MASK,
};

enum struct XfrmMode : uint8_t {
    TRANSPORT = XFRM_MODE_TRANSPORT,
    TUNNEL = XFRM_MODE_TUNNEL,
};

enum struct XfrmEncapType : uint16_t {
    NONE = 0,
    ESPINUDP_NON_IKE = UDP_ENCAP_ESPINUDP_NON_IKE,
    ESPINUDP = UDP_ENCAP_ESPINUDP
};

struct XfrmAlgo {
    std::string name;
    std::vector<uint8_t> key;
    uint16_t truncLenBits;
};

struct XfrmEncap {
    XfrmEncapType type;
    uint16_t srcPort;
    uint16_t dstPort;
};

// minimally sufficient structure to match either an SA or a Policy
struct XfrmId {
    xfrm_address_t dstAddr; // network order
    xfrm_address_t srcAddr;
    int addrFamily;  // AF_INET or AF_INET6
    int transformId; // requestId
    int spi;
    xfrm_mark mark;
};

struct XfrmSaInfo : XfrmId {
    XfrmAlgo auth;
    XfrmAlgo crypt;
    XfrmAlgo aead;
    int netId;
    XfrmMode mode;
    XfrmEncap encap;
};

class XfrmController {
public:
    XfrmController();

    static netdutils::Status Init();

    static netdutils::Status ipSecSetEncapSocketOwner(const android::base::unique_fd& socket,
                                                      int newUid, uid_t callerUid);

    static netdutils::Status ipSecAllocateSpi(int32_t transformId, const std::string& localAddress,
                                              const std::string& remoteAddress, int32_t inSpi,
                                              int32_t* outSpi);

    static netdutils::Status ipSecAddSecurityAssociation(
        int32_t transformId, int32_t mode, const std::string& sourceAddress,
        const std::string& destinationAddress, int32_t underlyingNetId, int32_t spi,
        int32_t markValue, int32_t markMask, const std::string& authAlgo,
        const std::vector<uint8_t>& authKey, int32_t authTruncBits, const std::string& cryptAlgo,
        const std::vector<uint8_t>& cryptKey, int32_t cryptTruncBits, const std::string& aeadAlgo,
        const std::vector<uint8_t>& aeadKey, int32_t aeadIcvBits, int32_t encapType,
        int32_t encapLocalPort, int32_t encapRemotePort);

    static netdutils::Status ipSecDeleteSecurityAssociation(int32_t transformId,
                                                            const std::string& sourceAddress,
                                                            const std::string& destinationAddress,
                                                            int32_t spi, int32_t markValue,
                                                            int32_t markMask);

    static netdutils::Status
    ipSecApplyTransportModeTransform(const android::base::unique_fd& socket, int32_t transformId,
                                     int32_t direction, const std::string& localAddress,
                                     const std::string& remoteAddress, int32_t spi);

    static netdutils::Status
    ipSecRemoveTransportModeTransform(const android::base::unique_fd& socket);

    static netdutils::Status ipSecAddSecurityPolicy(int32_t transformId, int32_t direction,
                                                    const std::string& sourceAddress,
                                                    const std::string& destinationAddress,
                                                    int32_t spi, int32_t markValue,
                                                    int32_t markMask);

    static netdutils::Status ipSecUpdateSecurityPolicy(int32_t transformId, int32_t direction,
                                                       const std::string& sourceAddress,
                                                       const std::string& destinationAddress,
                                                       int32_t spi, int32_t markValue,
                                                       int32_t markMask);

    static netdutils::Status ipSecDeleteSecurityPolicy(int32_t transformId, int32_t direction,
                                                       const std::string& sourceAddress,
                                                       const std::string& destinationAddress,
                                                       int32_t markValue, int32_t markMask);

    static int addVirtualTunnelInterface(const std::string& deviceName,
                                         const std::string& localAddress,
                                         const std::string& remoteAddress, int32_t ikey,
                                         int32_t okey, bool isUpdate);

    static int removeVirtualTunnelInterface(const std::string& deviceName);

    // Some XFRM netlink attributes comprise a header, a struct, and some data
    // after the struct. We wrap all of those in one struct for easier
    // marshalling. The structs below must be ABI compatible with the kernel and
    // are composed from kernel structures; thus, they use the kernel naming
    // convention.

    // Exposed for testing
    static constexpr size_t MAX_KEY_LENGTH = 128;

    // Container for the content of an XFRMA_ALG_CRYPT netlink attribute.
    // Exposed for testing
    struct nlattr_algo_crypt {
        nlattr hdr;
        xfrm_algo crypt;
        uint8_t key[MAX_KEY_LENGTH];
    };

    // Container for the content of an XFRMA_ALG_AUTH_TRUNC netlink attribute.
    // Exposed for testing
    struct nlattr_algo_auth {
        nlattr hdr;
        xfrm_algo_auth auth;
        uint8_t key[MAX_KEY_LENGTH];
    };

    // Container for the content of an XFRMA_TMPL netlink attribute.
    // Exposed for testing
    struct nlattr_algo_aead {
        nlattr hdr;
        xfrm_algo_aead aead;
        uint8_t key[MAX_KEY_LENGTH];
    };

    // Exposed for testing
    struct nlattr_user_tmpl {
        nlattr hdr;
        xfrm_user_tmpl tmpl;
    };

    // Container for the content of an XFRMA_ENCAP netlink attribute.
    // Exposed for testing
    struct nlattr_encap_tmpl {
        nlattr hdr;
        xfrm_encap_tmpl tmpl;
    };

    // Container for the content of an XFRMA_MARK netlink attribute.
    // Exposed for testing
    struct nlattr_xfrm_mark {
        nlattr hdr;
        xfrm_mark mark;
    };

    // Container for the content of an XFRMA_OUTPUT_MARK netlink attribute.
    // Exposed for testing
    struct nlattr_xfrm_output_mark {
        nlattr hdr;
        __u32 outputMark;
    };

private:
/*
 * Below is a redefinition of the xfrm_usersa_info struct that is part
 * of the Linux uapi <linux/xfrm.h> to align the structures to a 64-bit
 * boundary.
 */
#ifdef NETLINK_COMPAT32
    // Shadow the kernel definition of xfrm_usersa_info with a 64-bit aligned version
    struct xfrm_usersa_info : ::xfrm_usersa_info {
    } __attribute__((aligned(8)));
    // Shadow the kernel's version, using the aligned version of xfrm_usersa_info
    struct xfrm_userspi_info {
        struct xfrm_usersa_info info;
        __u32 min;
        __u32 max;
    };

    /*
     * Anyone who encounters a failure when sending netlink messages should look here
     * first. Hitting the static_assert() below should be a strong hint that Android
     * IPsec will probably not work with your current settings.
     *
     * Again, experimentally determined, the "flags" field should be the first byte in
     * the final word of the xfrm_usersa_info struct. The check validates the size of
     * the padding to be 7.
     *
     * This padding is verified to be correct on gcc/x86_64 kernel, and clang/x86 userspace.
     */
    static_assert(sizeof(::xfrm_usersa_info) % 8 != 0, "struct xfrm_usersa_info has changed "
                                                       "alignment. Please consider whether this "
                                                       "patch is needed.");
    static_assert(sizeof(xfrm_usersa_info) - offsetof(xfrm_usersa_info, flags) == 8,
                  "struct xfrm_usersa_info probably misaligned with kernel struct.");
    static_assert(sizeof(xfrm_usersa_info) % 8 == 0, "struct xfrm_usersa_info_t is not 64-bit  "
                                                     "aligned. Please consider whether this patch "
                                                     "is needed.");
    static_assert(sizeof(::xfrm_userspi_info) - sizeof(::xfrm_usersa_info) ==
                      sizeof(xfrm_userspi_info) - sizeof(xfrm_usersa_info),
                  "struct xfrm_userspi_info has changed and does not match the kernel struct.");
#endif

    // helper function for filling in the XfrmId (and XfrmSaInfo) structure
    static netdutils::Status fillXfrmId(const std::string& sourceAddress,
                                        const std::string& destinationAddress, int32_t spi,
                                        int32_t markValue, int32_t markMask, int32_t transformId,
                                        XfrmId* xfrmId);

    // Top level functions for managing a Transport Mode Transform
    static netdutils::Status addTransportModeTransform(const XfrmSaInfo& record);
    static int removeTransportModeTransform(const XfrmSaInfo& record);

    // TODO(messagerefactor): FACTOR OUT ALL MESSAGE BUILDING CODE BELOW HERE
    // Shared between SA and SP
    static void fillXfrmSelector(const XfrmSaInfo& record, xfrm_selector* selector);

    // Shared between Transport and Tunnel Mode
    static int fillNlAttrXfrmAlgoEnc(const XfrmAlgo& in_algo, nlattr_algo_crypt* algo);
    static int fillNlAttrXfrmAlgoAuth(const XfrmAlgo& in_algo, nlattr_algo_auth* algo);
    static int fillNlAttrXfrmAlgoAead(const XfrmAlgo& in_algo, nlattr_algo_aead* algo);
    static int fillNlAttrXfrmEncapTmpl(const XfrmSaInfo& record, nlattr_encap_tmpl* tmpl);

    // Functions for updating a Transport Mode SA
    static netdutils::Status updateSecurityAssociation(const XfrmSaInfo& record,
                                                       const XfrmSocket& sock);
    static int fillUserSaInfo(const XfrmSaInfo& record, xfrm_usersa_info* usersa);

    // Functions for deleting a Transport Mode SA
    static netdutils::Status deleteSecurityAssociation(const XfrmId& record,
                                                       const XfrmSocket& sock);
    static int fillUserSaId(const XfrmId& record, xfrm_usersa_id* said);
    static int fillUserTemplate(const XfrmSaInfo& record, xfrm_user_tmpl* tmpl);

    static int fillTransportModeUserSpInfo(const XfrmSaInfo& record, XfrmDirection direction,
                                           xfrm_userpolicy_info* usersp);
    static int fillNlAttrUserTemplate(const XfrmSaInfo& record, nlattr_user_tmpl* tmpl);
    static int fillUserPolicyId(const XfrmSaInfo& record, XfrmDirection direction,
                                xfrm_userpolicy_id* policy_id);
    static int fillNlAttrXfrmMark(const XfrmId& record, nlattr_xfrm_mark* mark);
    static int fillNlAttrXfrmOutputMark(const __u32 output_mark_value,
                                        nlattr_xfrm_output_mark* output_mark);

    static netdutils::Status allocateSpi(const XfrmSaInfo& record, uint32_t minSpi, uint32_t maxSpi,
                                         uint32_t* outSpi, const XfrmSocket& sock);

    static netdutils::Status processSecurityPolicy(int32_t transformId, int32_t direction,
                                                   const std::string& localAddress,
                                                   const std::string& remoteAddress, int32_t spi,
                                                   int32_t markValue, int32_t markMask,
                                                   int32_t msgType);
    static netdutils::Status updateTunnelModeSecurityPolicy(const XfrmSaInfo& record,
                                                            const XfrmSocket& sock,
                                                            XfrmDirection direction,
                                                            uint16_t msgType);
    static netdutils::Status deleteTunnelModeSecurityPolicy(const XfrmSaInfo& record,
                                                            const XfrmSocket& sock,
                                                            XfrmDirection direction);
    static netdutils::Status flushInterfaces();
    static netdutils::Status flushSaDb(const XfrmSocket& s);
    static netdutils::Status flushPolicyDb(const XfrmSocket& s);

    // END TODO(messagerefactor)
};

} // namespace net
} // namespace android

#endif /* !defined(XFRM_CONTROLLER_H) */
