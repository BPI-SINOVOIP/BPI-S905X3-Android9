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

#define LOG_TAG "NetlinkListener"

#include <sstream>
#include <vector>

#include <linux/netfilter/nfnetlink.h>

#include <cutils/log.h>
#include <netdutils/Misc.h>
#include <netdutils/Syscalls.h>

#include "NetlinkListener.h"

namespace android {
namespace net {

using netdutils::Fd;
using netdutils::Slice;
using netdutils::Status;
using netdutils::UniqueFd;
using netdutils::findWithDefault;
using netdutils::forEachNetlinkMessage;
using netdutils::makeSlice;
using netdutils::sSyscalls;
using netdutils::status::ok;
using netdutils::statusFromErrno;

namespace {

constexpr int kNetlinkMsgErrorType = (NFNL_SUBSYS_NONE << 8) | NLMSG_ERROR;

constexpr sockaddr_nl kKernelAddr = {
    .nl_family = AF_NETLINK, .nl_pad = 0, .nl_pid = 0, .nl_groups = 0,
};

const NetlinkListener::DispatchFn kDefaultDispatchFn = [](const nlmsghdr& nlmsg, const Slice) {
    std::stringstream ss;
    ss << nlmsg;
    ALOGE("unhandled netlink message: %s", ss.str().c_str());
};

}  // namespace

NetlinkListener::NetlinkListener(UniqueFd event, UniqueFd sock)
    : mEvent(std::move(event)), mSock(std::move(sock)), mWorker([this]() { run(); }) {
    const auto rxErrorHandler = [](const nlmsghdr& nlmsg, const Slice msg) {
        std::stringstream ss;
        ss << nlmsg << " " << msg << " " << netdutils::toHex(msg, 32);
        ALOGE("unhandled netlink message: %s", ss.str().c_str());
    };
    expectOk(NetlinkListener::subscribe(kNetlinkMsgErrorType, rxErrorHandler));
}

NetlinkListener::~NetlinkListener() {
    const auto& sys = sSyscalls.get();
    const uint64_t data = 1;
    // eventfd should never enter an error state unexpectedly
    expectOk(sys.write(mEvent, makeSlice(data)).status());
    mWorker.join();
}

Status NetlinkListener::send(const Slice msg) {
    const auto& sys = sSyscalls.get();
    ASSIGN_OR_RETURN(auto sent, sys.sendto(mSock, msg, 0, kKernelAddr));
    if (sent != msg.size()) {
        return statusFromErrno(EMSGSIZE, "unexpect message size");
    }
    return ok;
}

Status NetlinkListener::subscribe(uint16_t type, const DispatchFn& fn) {
    std::lock_guard<std::mutex> guard(mMutex);
    mDispatchMap[type] = fn;
    return ok;
}

Status NetlinkListener::unsubscribe(uint16_t type) {
    std::lock_guard<std::mutex> guard(mMutex);
    mDispatchMap.erase(type);
    return ok;
}

Status NetlinkListener::run() {
    std::vector<char> rxbuf(4096);

    const auto rxHandler = [this](const nlmsghdr& nlmsg, const Slice& buf) {
        std::lock_guard<std::mutex> guard(mMutex);
        const auto& fn = findWithDefault(mDispatchMap, nlmsg.nlmsg_type, kDefaultDispatchFn);
        fn(nlmsg, buf);
    };

    const auto& sys = sSyscalls.get();
    const std::array<Fd, 2> fds{{{mEvent}, {mSock}}};
    const int events = POLLIN | POLLRDHUP | POLLERR | POLLHUP;
    const double timeout = 3600;
    while (true) {
        ASSIGN_OR_RETURN(auto revents, sys.ppoll(fds, events, timeout));
        // After mEvent becomes readable, we should stop servicing mSock and return
        if (revents[0] & POLLIN) {
            break;
        }
        if (revents[1] & POLLIN) {
            auto rx = sys.recvfrom(mSock, makeSlice(rxbuf), 0);
            if (rx.status().code() == ENOBUFS) {
                // Ignore ENOBUFS - the socket is still usable
                // TODO: Users other than NFLOG may need to know about this
                continue;
            }
            forEachNetlinkMessage(rx.value(), rxHandler);
        }
    }
    return ok;
}

}  // namespace net
}  // namespace android
