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

#include <binder/IPCThreadState.h>

#include <private/android_filesystem_config.h>

static bool isSystemUser() {
    uid_t uid =  IPCThreadState::self()->getCallingUid();
    switch (uid) {
        case AID_ROOT:
        case AID_SYSTEM: {
            return true;
        } break;
        default: {
            ALOGE("uid %u is not root nor system - access denied", uid);
        } break;
    }
    return false;
}

namespace procfsinspector {
class BpProcfsInspector: public BpInterface<IProcfsInspector> {
    public:
        BpProcfsInspector(sp<IBinder> binder) : BpInterface<IProcfsInspector>(binder) {}

        virtual std::vector<ProcessInfo> readProcessTable() override {
            Parcel data, reply;
            remote()->transact((uint32_t)IProcfsInspector::Call::READ_PROCESS_TABLE, data, &reply);

            std::vector<procfsinspector::ProcessInfo> result;
            reply.readParcelableVector(&result);
            return result;
        }

};

IMPLEMENT_META_INTERFACE(ProcfsInspector, "com.android.car.procfsinspector.IProcfsInspector");

status_t Impl::onTransact(uint32_t code,
    const Parcel& data, Parcel* reply, uint32_t flags) {

    if (code == (uint32_t)IProcfsInspector::Call::READ_PROCESS_TABLE) {
        CHECK_INTERFACE(IProcfsInspector, data, reply);
        if (isSystemUser()) {
            reply->writeNoException();
            reply->writeParcelableVector(readProcessTable());
            return NO_ERROR;
        } else {
            return PERMISSION_DENIED;
        }
    }

    return BBinder::onTransact(code, data, reply, flags);
}

}
