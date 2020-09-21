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

#ifndef CAR_PROCFS_SERVER
#define CAR_PROCFS_SERVER

#define LOG_TAG "com.android.car.procfsinspector"
#define SERVICE_NAME "com.android.car.procfsinspector"

#include <vector>

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <utils/Log.h>
#include <utils/String16.h>

#include "process.h"

using namespace android;

namespace procfsinspector {
    class IProcfsInspector : public IInterface {
    public:
        DECLARE_META_INTERFACE(ProcfsInspector);

        enum class Call : uint32_t {
            READ_PROCESS_TABLE = IBinder::FIRST_CALL_TRANSACTION,
        };

        // API declarations start here
        virtual std::vector<ProcessInfo> readProcessTable() = 0;
    };

    class Impl : public BnInterface<IProcfsInspector> {
    public:
        virtual status_t onTransact(uint32_t code,
            const Parcel& data,
            Parcel *reply,
            uint32_t flags) override;
        virtual std::vector<ProcessInfo> readProcessTable() override;
    };
}

#endif // CAR_PROCFS_SERVER
