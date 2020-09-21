/*
 * Copyright (C) 2018 The Android Open Source Project
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
#define DEBUG false  // STOPSHIP if true
#include "Log.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>
#include <private/android_logger.h>
#include <unordered_map>

#include "StatsSocketListener.h"
#include "guardrail/StatsdStats.h"
#include "stats_log_util.h"

namespace android {
namespace os {
namespace statsd {

static const int kLogMsgHeaderSize = 28;

StatsSocketListener::StatsSocketListener(const sp<LogListener>& listener)
    : SocketListener(getLogSocket(), false /*start listen*/), mListener(listener) {
}

StatsSocketListener::~StatsSocketListener() {
}

bool StatsSocketListener::onDataAvailable(SocketClient* cli) {
    static bool name_set;
    if (!name_set) {
        prctl(PR_SET_NAME, "statsd.writer");
        name_set = true;
    }

    // + 1 to ensure null terminator if MAX_PAYLOAD buffer is received
    char buffer[sizeof_log_id_t + sizeof(uint16_t) + sizeof(log_time) + LOGGER_ENTRY_MAX_PAYLOAD +
                1];
    struct iovec iov = {buffer, sizeof(buffer) - 1};

    alignas(4) char control[CMSG_SPACE(sizeof(struct ucred))];
    struct msghdr hdr = {
            NULL, 0, &iov, 1, control, sizeof(control), 0,
    };

    int socket = cli->getSocket();

    // To clear the entire buffer is secure/safe, but this contributes to 1.68%
    // overhead under logging load. We are safe because we check counts, but
    // still need to clear null terminator
    // memset(buffer, 0, sizeof(buffer));
    ssize_t n = recvmsg(socket, &hdr, 0);
    if (n <= (ssize_t)(sizeof(android_log_header_t))) {
        return false;
    }

    buffer[n] = 0;

    struct ucred* cred = NULL;

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&hdr);
    while (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_CREDENTIALS) {
            cred = (struct ucred*)CMSG_DATA(cmsg);
            break;
        }
        cmsg = CMSG_NXTHDR(&hdr, cmsg);
    }

    struct ucred fake_cred;
    if (cred == NULL) {
        cred = &fake_cred;
        cred->pid = 0;
        cred->uid = DEFAULT_OVERFLOWUID;
    }

    char* ptr = ((char*)buffer) + sizeof(android_log_header_t);
    n -= sizeof(android_log_header_t);

    log_msg msg;

    msg.entry.len = n;
    msg.entry.hdr_size = kLogMsgHeaderSize;
    msg.entry.sec = time(nullptr);
    msg.entry.pid = cred->pid;
    msg.entry.uid = cred->uid;

    memcpy(msg.buf + kLogMsgHeaderSize, ptr, n + 1);
    LogEvent event(msg);

    // Call the listener
    mListener->OnLogEvent(&event, false /*reconnected, N/A in statsd socket*/);

    return true;
}

int StatsSocketListener::getLogSocket() {
    static const char socketName[] = "statsdw";
    int sock = android_get_control_socket(socketName);

    if (sock < 0) {  // statsd started up in init.sh
        sock = socket_local_server(socketName, ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_DGRAM);

        int on = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on))) {
            return -1;
        }
    }
    return sock;
}

}  // namespace statsd
}  // namespace os
}  // namespace android
