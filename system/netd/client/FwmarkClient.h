/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef NETD_CLIENT_FWMARK_CLIENT_H
#define NETD_CLIENT_FWMARK_CLIENT_H

#include <sys/socket.h>

struct FwmarkCommand;
struct FwmarkConnectInfo;

class FwmarkClient {
public:
    // Returns true if a socket of the given |family| should be sent to the fwmark server to have
    // its SO_MARK set.
    static bool shouldSetFwmark(int family);

    // Returns true if an additional call should be made after ON_CONNECT calls, to log extra
    // information like latency and source IP.
    static bool shouldReportConnectComplete(int family);

    FwmarkClient();
    ~FwmarkClient();

    // Sends |data| to the fwmark server, along with |fd| as ancillary data using cmsg(3).
    // For ON_CONNECT_COMPLETE |data| command, |connectInfo| should be provided.
    // Returns 0 on success or a negative errno value on failure.
    int send(FwmarkCommand* data, int fd, FwmarkConnectInfo* connectInfo);

    // Env flag to control whether FwmarkClient sends any information at all about network events
    // back to the system server through FwmarkServer.
    static constexpr const char* ANDROID_NO_USE_FWMARK_CLIENT = "ANDROID_NO_USE_FWMARK_CLIENT";

    // Env flag to control whether FwmarkClient should exclude detailed information like IP
    // addresses and only send basic information necessary for marking sockets.
    // Has no effect if ANDROID_NO_USE_FWMARK_CLIENT is set.
    static constexpr const char* ANDROID_FWMARK_METRICS_ONLY = "ANDROID_FWMARK_METRICS_ONLY";

private:
    int mChannel;
};

#endif  // NETD_CLIENT_FWMARK_CLIENT_H
