/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * tun_interface.cpp - creates tun interfaces for testing purposes
 */

#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <netutils/ifc.h>

#include "tun_interface.h"

#define TUN_DEV "/dev/tun"

using android::base::StringPrintf;

namespace android {
namespace net {

int TunInterface::init() {
    // Generate a random ULA address pair.
    arc4random_buf(&mSrcAddr, sizeof(mSrcAddr));
    mSrcAddr.s6_addr[0] = 0xfd;
    memcpy(&mDstAddr, &mSrcAddr, sizeof(mDstAddr));
    mDstAddr.s6_addr[15] ^= 1;

    // Convert the addresses to strings because that's what ifc_add_address takes.
    char srcStr[INET6_ADDRSTRLEN], dstStr[INET6_ADDRSTRLEN];
    sockaddr_in6 src6 = { .sin6_family = AF_INET6, .sin6_addr = mSrcAddr, };
    sockaddr_in6 dst6 = { .sin6_family = AF_INET6, .sin6_addr = mDstAddr, };
    int flags = NI_NUMERICHOST;
    if (getnameinfo((sockaddr *) &src6, sizeof(src6), srcStr, sizeof(srcStr), NULL, 0, flags) ||
        getnameinfo((sockaddr *) &dst6, sizeof(dst6), dstStr, sizeof(dstStr), NULL, 0, flags)) {
        return -EINVAL;
    }

    // Create a tun interface with a name based on our PID and some randomness.
    // iptables will only accept interfaces whose name is up to IFNAMSIZ - 1 bytes long.
    mIfName = StringPrintf("netd%u_%u", getpid(), arc4random());
    if (mIfName.size() >= IFNAMSIZ) {
        mIfName.resize(IFNAMSIZ - 1);
    }
    struct ifreq ifr = {
        .ifr_ifru = { .ifru_flags = IFF_TUN },
    };
    strlcpy(ifr.ifr_name, mIfName.c_str(), sizeof(ifr.ifr_name));

    mFd = open(TUN_DEV, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (mFd == -1) return -errno;

    int ret = ioctl(mFd, TUNSETIFF, &ifr, sizeof(ifr));
    if (ret == -1) {
        ret = -errno;
        close(mFd);
        return ret;
    }

    if (ifc_add_address(ifr.ifr_name, srcStr, 64) ||
        ifc_add_address(ifr.ifr_name, dstStr, 64)) {
        ret = -errno;
        close(mFd);
        return ret;
    }

    mIfIndex = if_nametoindex(ifr.ifr_name);

    return 0;
}

void TunInterface::destroy() {
    if (mFd != -1) {
        close(mFd);
        mFd = -1;
    }
}

}  // namespace net
}  // namespace android
