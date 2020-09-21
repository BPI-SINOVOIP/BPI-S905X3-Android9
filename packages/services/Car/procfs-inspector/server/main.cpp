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

#include "server.h"

#include <signal.h>

#include <binder/IServiceManager.h>

#include <utils/Log.h>

using namespace android;

int main(int, char**)
{
    ALOGI("starting " LOG_TAG);
    signal(SIGPIPE, SIG_IGN);

    sp<ProcessState> processSelf(ProcessState::self());
    sp<IServiceManager> serviceManager = defaultServiceManager();
    std::unique_ptr<procfsinspector::Impl> server(new procfsinspector::Impl());

    serviceManager->addService(String16(SERVICE_NAME), server.get());

    processSelf->startThreadPool();

    ALOGI(LOG_TAG " started");

    IPCThreadState::self()->joinThreadPool();

    ALOGW(LOG_TAG " joined and going down");
    return 0;
}
