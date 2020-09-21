/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "IArcVideoBridge"
//#define LOG_NDEBUG 0

#include <stdint.h>
#include <sys/types.h>

#include "IArcVideoBridge.h"
#include <binder/Parcel.h>
#include <utils/Log.h>

namespace android {

enum {
    BOOTSTRAP_VIDEO_ACCELERATOR_FACTORY = IBinder::FIRST_CALL_TRANSACTION,
    HOST_VERSION,
};

class BpArcVideoBridge : public BpInterface<IArcVideoBridge> {
public:
    BpArcVideoBridge(const sp<IBinder>& impl) : BpInterface<IArcVideoBridge>(impl) { }

    virtual ::arc::MojoBootstrapResult bootstrapVideoAcceleratorFactory() {
        Parcel data, reply;
        ALOGV("bootstrapVideoAcceleratorFactory");
        data.writeInterfaceToken(IArcVideoBridge::getInterfaceDescriptor());
        status_t status = remote()->transact(
                BOOTSTRAP_VIDEO_ACCELERATOR_FACTORY, data, &reply, 0);
        if (status != 0) {
            ALOGE("transact failed: %d", status);
            return arc::MojoBootstrapResult();
        }
        return arc::MojoBootstrapResult::createFromParcel(reply);
    }

    virtual int32_t hostVersion() {
        Parcel data, reply;
        ALOGV("hostVersion");
        data.writeInterfaceToken(IArcVideoBridge::getInterfaceDescriptor());
        status_t status = remote()->transact(HOST_VERSION, data, &reply, 0);
        if (status != 0) {
            ALOGE("transact failed: %d", status);
            return false;
        }
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(ArcVideoBridge, "android.os.IArcVideoBridge");

status_t BnArcVideoBridge::onTransact(
        uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags) {
    switch(code) {
        case BOOTSTRAP_VIDEO_ACCELERATOR_FACTORY: {
            ALOGV("BOOTSTRAP_VIDEO_ACCELERATOR_FACTORY");
            CHECK_INTERFACE(IArcVideoBridge, data, reply);
            arc::MojoBootstrapResult result = bootstrapVideoAcceleratorFactory();
            return result.writeToParcel(reply);
        }
        case HOST_VERSION: {
            ALOGV("HOST_VERSION");
            CHECK_INTERFACE(IArcVideoBridge, data, reply);
            reply->writeInt32(hostVersion());
            return OK;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}  // namespace android
