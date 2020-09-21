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

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fs_mgr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define LOG_TAG "DigCmdListener"

#include <android-base/stringprintf.h>
#include <cutils/fs.h>
#include <cutils/log.h>

#include <sysutils/SocketClient.h>
#include <private/android_filesystem_config.h>
#include <sysutils/FrameworkCommand.h>

#include "ResponseCode.h"
#include "DigManager.h"
#include "DigCommandListener.h"

DigCommandListener::DigCommandListener() :
                FrameworkListener("dig", true) {
    registerCmd(new DigCmd());
}

DigCommandListener::DigCmd::DigCmd() :
                FrameworkCommand("dig") {
}

int DigCommandListener::DigCmd::runCommand(
                SocketClient *cli, int argc, char **argv) {
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument", false);
        return 0;
    }

    std::string cmd(argv[1]);
    if (cmd == "connect" && argc > 2) {
        std::string status(argv[2]);
        if (status == "ok") {
            DigManager *dm = DigManager::Instance();
            dm->setConnect(true);
            if (dm->isConnected()) {
                cli->sendMsg(ResponseCode::CommandOkay, "Command succeeded", false);
            } else {
                cli->sendMsg(ResponseCode::OperationFailed, "Command failed", false);
            }
        }
    }
    return 0;
}
