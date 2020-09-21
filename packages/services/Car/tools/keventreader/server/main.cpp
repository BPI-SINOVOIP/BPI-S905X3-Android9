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
#include "defines.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <iostream>
#include <signal.h>
#include <utils/Log.h>
#include "eventgatherer.h"
#include "keymap.h"
#include "eventprovider.h"


using namespace android;
using namespace com::android::car::keventreader;

/**
 * This tool expects to be able to read input events in the Linux kernel input_event format
 * (see linux/input.h); that format is available by means of /dev/input/event* files (which are
 * mapped 1-to-1 to input sources that provide keypresses as input (e.g. keyboards)
 * The tool will hook up to each such file passed as input
 */
static const char* SYNTAX_INSTRUCTIONS =
    "invalid command line arguments - provide one or more /dev/input/event files";

static void error(int code) {
    ALOGE("%s", SYNTAX_INSTRUCTIONS);
    std::cerr << SYNTAX_INSTRUCTIONS << '\n';
    exit(code);
}

int main(int argc, const char** argv) {
    if (argc < 2 || argv[1] == nullptr || argv[1][0] == 0) {
        error(1);
    }

    EventGatherer gatherer(argc, argv);
    if (0 == gatherer.size()) {
        error(2);
    }

    ALOGI("starting " LOG_TAG);
    signal(SIGPIPE, SIG_IGN);

    sp<ProcessState> processSelf(ProcessState::self());
    sp<IServiceManager> serviceManager = defaultServiceManager();
    auto service = std::make_unique<EventProviderImpl>(std::move(gatherer));
    serviceManager->addService(String16(SERVICE_NAME), service.get());

    processSelf->startThreadPool();

    ALOGI(LOG_TAG " started");

    auto svcthread = service->startLoop();
    IPCThreadState::self()->joinThreadPool();

    ALOGW(LOG_TAG " joined and going down");
    exit(0);
}
