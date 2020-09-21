/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

#define LOG_TAG "Netd"

#include "cutils/log.h"
#include "utils/RWLock.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "CommandListener.h"
#include "Controllers.h"
#include "DnsProxyListener.h"
#include "FwmarkServer.h"
#include "MDnsSdListener.h"
#include "NFLogListener.h"
#include "NetdConstants.h"
#include "NetdHwService.h"
#include "NetdNativeService.h"
#include "NetlinkManager.h"
#include "Stopwatch.h"

using android::status_t;
using android::sp;
using android::IPCThreadState;
using android::ProcessState;
using android::defaultServiceManager;
using android::net::CommandListener;
using android::net::DnsProxyListener;
using android::net::FwmarkServer;
using android::net::NetdHwService;
using android::net::NetdNativeService;
using android::net::NetlinkManager;
using android::net::NFLogListener;
using android::net::makeNFLogListener;

static void remove_pid_file();
static bool write_pid_file();

const char* const PID_FILE_PATH = "/data/misc/net/netd_pid";
const int PID_FILE_FLAGS = O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW | O_CLOEXEC;
const mode_t PID_FILE_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  // mode 0644, rw-r--r--

android::RWLock android::net::gBigNetdLock;

int main() {
    using android::net::gCtls;
    Stopwatch s;

    ALOGI("Netd 1.0 starting");
    remove_pid_file();

    blockSigpipe();

    // Before we do anything that could fork, mark CLOEXEC the UNIX sockets that we get from init.
    // FrameworkListener does this on initialization as well, but we only initialize these
    // components after having initialized other subsystems that can fork.
    for (const auto& sock : { CommandListener::SOCKET_NAME,
                              DnsProxyListener::SOCKET_NAME,
                              FwmarkServer::SOCKET_NAME,
                              MDnsSdListener::SOCKET_NAME }) {
        setCloseOnExec(sock);
    }

    NetlinkManager *nm = NetlinkManager::Instance();
    if (nm == nullptr) {
        ALOGE("Unable to create NetlinkManager");
        exit(1);
    };

    gCtls = new android::net::Controllers();
    gCtls->init();

    CommandListener cl;
    nm->setBroadcaster((SocketListener *) &cl);

    if (nm->start()) {
        ALOGE("Unable to start NetlinkManager (%s)", strerror(errno));
        exit(1);
    }

    std::unique_ptr<NFLogListener> logListener;
    {
        auto result = makeNFLogListener();
        if (!isOk(result)) {
            ALOGE("Unable to create NFLogListener: %s", toString(result).c_str());
            exit(1);
        }
        logListener = std::move(result.value());
        auto status = gCtls->wakeupCtrl.init(logListener.get());
        if (!isOk(result)) {
            ALOGE("Unable to init WakeupController: %s", toString(result).c_str());
            // We can still continue without wakeup packet logging.
        }
    }

    // Set local DNS mode, to prevent bionic from proxying
    // back to this service, recursively.
    setenv("ANDROID_DNS_MODE", "local", 1);
    DnsProxyListener dpl(&gCtls->netCtrl, &gCtls->eventReporter);
    if (dpl.startListener()) {
        ALOGE("Unable to start DnsProxyListener (%s)", strerror(errno));
        exit(1);
    }

    MDnsSdListener mdnsl;
    if (mdnsl.startListener()) {
        ALOGE("Unable to start MDnsSdListener (%s)", strerror(errno));
        exit(1);
    }

    FwmarkServer fwmarkServer(&gCtls->netCtrl, &gCtls->eventReporter, &gCtls->trafficCtrl);
    if (fwmarkServer.startListener()) {
        ALOGE("Unable to start FwmarkServer (%s)", strerror(errno));
        exit(1);
    }

    Stopwatch subTime;
    status_t ret;
    if ((ret = NetdNativeService::start()) != android::OK) {
        ALOGE("Unable to start NetdNativeService: %d", ret);
        exit(1);
    }
    ALOGI("Registering NetdNativeService: %.1fms", subTime.getTimeAndReset());

    /*
     * Now that we're up, we can respond to commands. Starting the listener also tells
     * NetworkManagementService that we are up and that our binder interface is ready.
     */
    if (cl.startListener()) {
        ALOGE("Unable to start CommandListener (%s)", strerror(errno));
        exit(1);
    }
    ALOGI("Starting CommandListener: %.1fms", subTime.getTimeAndReset());

    write_pid_file();

    // Now that netd is ready to process commands, advertise service
    // availability for HAL clients.
    NetdHwService mHwSvc;
    if ((ret = mHwSvc.start()) != android::OK) {
        ALOGE("Unable to start NetdHwService: %d", ret);
        exit(1);
    }
    ALOGI("Registering NetdHwService: %.1fms", subTime.getTimeAndReset());

    ALOGI("Netd started in %dms", static_cast<int>(s.timeTaken()));

    IPCThreadState::self()->joinThreadPool();

    ALOGI("Netd exiting");

    remove_pid_file();

    exit(0);
}

static bool write_pid_file() {
    char pid_buf[INT32_STRLEN];
    snprintf(pid_buf, sizeof(pid_buf), "%d\n", (int) getpid());

    int fd = open(PID_FILE_PATH, PID_FILE_FLAGS, PID_FILE_MODE);
    if (fd == -1) {
        ALOGE("Unable to create pid file (%s)", strerror(errno));
        return false;
    }

    // File creation is affected by umask, so make sure the right mode bits are set.
    if (fchmod(fd, PID_FILE_MODE) == -1) {
        ALOGE("failed to set mode 0%o on %s (%s)", PID_FILE_MODE, PID_FILE_PATH, strerror(errno));
        close(fd);
        remove_pid_file();
        return false;
    }

    if (write(fd, pid_buf, strlen(pid_buf)) != (ssize_t)strlen(pid_buf)) {
        ALOGE("Unable to write to pid file (%s)", strerror(errno));
        close(fd);
        remove_pid_file();
        return false;
    }
    close(fd);
    return true;
}

static void remove_pid_file() {
    unlink(PID_FILE_PATH);
}
